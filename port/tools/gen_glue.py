#!/usr/bin/env python3
"""Generate the glue between native C and the translated handwritten code.

  entry.c    one native function per translated function that C calls by
             name (167): it moves the arguments into the thread's
             recomp_context by the o32 rules (a0-a3, f12/f14, the stack),
             calls recomp_func_X and returns v0/f0.
  externs.c  recomp_extern_X for every call out of the translated code (53):
             it takes the arguments out of the context by the same rules,
             calls the native function and puts its result in v0/f0.
  fn_values.txt  every name the translated code takes the address of (for
             the movable build's port-arena: those of them that are
             functions are values, called through port_fn).

Prototypes come from the C: the definition for a C function, the
declarations for a translated one (the most common spelling where files
disagree).  Only the o32 class of each argument matters (int, float,
double, 64-bit int): integer arguments are passed as 32-bit words, which is
what IDO puts in a register for any narrower type as well.  Between native
functions they are `uintptr_t` where the callee may take a pointer (the
32-bit value, zero-extended: the same as uint32_t in the 32-bit build, a
whole register in the 64-bit one, where a pointer argument must not have
the upper half undefined) and are read as `uint32_t`.  The live-in
registers of each translated function (liveness.py) are checked against
the prototype: a float argument has to be read from f12/f14 when it comes
first, and an int one from a0-a3.

The functions the port's link wraps (`--wrap FUNC,...`, CMakeLists.txt's
PORT_WRAPS) are called by their `__wrap_` names, as the link would make
them: the movable build has no `--wrap` (ld64 has none), and in the others
it is the same call.

usage: gen_glue.py OUTDIR [--wrap FUNC,...]
"""

import collections
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")
sys.path.insert(0, HERE)

import liveness  # noqa: E402

C_DIRS = [os.path.join(BLAST, "src", m) for m in ("hd_code", "hd_front_end")]
EXTRA_C = [os.path.join(BLAST, "src", f) for f in ("gzip_inflate.inc.c", "gzip_unzip.inc.c")]
HEADERS = [os.path.join(BLAST, "include")]

# libultra functions the translated code calls; prototypes as in os.h
LIBULTRA = {
    "osInvalDCache": ("void", ["ptr", "s32"]),
    "osWritebackDCache": ("void", ["ptr", "s32"]),
    "osRecvMesg": ("s32", ["ptr", "ptr", "s32"]),
    "osSendMesg": ("s32", ["ptr", "ptr", "s32"]),
    "osPiStartDma": ("s32", ["ptr", "s32", "s32", "u32", "ptr", "u32", "ptr"]),
}

# the LP64 build (CMake runs this with PORT_LP64=1 then)
LP64 = os.environ.get("PORT_LP64") == "1"

INT_TYPES = {"s8", "u8", "s16", "u16", "s32", "u32", "int", "unsigned", "char", "short", "long",
             "signed", "OSPri", "OSId", "OSMesg", "size_t", "Gfx", "Vtx", "Mtx"}


def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    s = re.sub(r"//[^\n]*", " ", s)
    s = re.sub(r"^\s*#.*$", " ", s, flags=re.M)
    return s


def classify(t):
    """o32 class of one parameter or return type."""
    t = t.strip()
    t = re.sub(r"\b(const|volatile|register|extern|static|struct|union|enum)\b", " ", t)
    t = " ".join(t.split())
    if "*" in t or "[" in t:
        return "I"
    words = t.split()
    if not words or words == ["void"]:
        return "V"
    base = " ".join(words)
    if base in ("f32", "float"):
        return "F"
    if base in ("f64", "double"):
        return "D"
    if base in ("s64", "u64", "long long", "unsigned long long", "signed long long"):
        return "L"
    return "I"


def narrow_ctype(t):
    """C type to use in the wrapper for an integer return type."""
    t = " ".join(re.sub(r"\b(const|volatile|extern|static)\b", " ", t).split())
    if "*" in t:
        return "uintptr_t"
    return {"u8": "uint8_t", "s8": "int8_t", "u16": "uint16_t", "s16": "int16_t",
            "unsigned char": "uint8_t", "char": "int8_t", "signed char": "int8_t",
            "unsigned short": "uint16_t", "short": "int16_t"}.get(t, "uintptr_t")


def split_params(p):
    p = p.strip()
    if p in ("", "void"):
        return []
    out, depth, cur = [], 0, ""
    for ch in p:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    out.append(cur)
    return [x.strip() for x in out]


def param_type(p):
    """Drop the parameter name, keep the type."""
    if p == "...":
        return "..."
    if "(" in p:
        return "void *"          # function pointer
    m = re.match(r"^(.*?)(\w+)\s*((?:\[[^\]]*\])*)$", p)
    if m and m.group(1).strip() and m.group(2) not in INT_TYPES and not re.match(
            r"^(const\s+|volatile\s+)*$", m.group(1)):
        t = m.group(1) + ("*" if m.group(3) else "")
        return t
    return p


DECL_RE = re.compile(r"(?:(?<=[;{}])|^)\s*((?:[A-Za-z_][\w]*[\s\*]+)+?)(\**)\s*(\w+)\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\)\s*([;{])",
                     re.S | re.M)


KR_RE = re.compile(r"(?:(?<=[;{}])|^)\s*((?:[A-Za-z_]\w*[\s\*]+)+?)(\w+)\s*\(((?:\s*\w+\s*,)*\s*\w+\s*)\)\s*((?:[^;{}()]+;\s*)+)\{",
                   re.S | re.M)

# defined under another name (a #define before a shared source)
KNOWN = {
    "func_8025C230": ("void", ("void *", "void *", "void *")),   # GZIP_UNZIP
}


# libultra (and libc) that the IDO code a version still has as asm calls
# (jp's GLOBAL_ASM, tools/recomp/config.py), with their types from os.h,
# gu.h and libaudio.h: some take floats
_F = "float"
LIBULTRA_TYPED = {
    "alCSPGetTempo": ("s32", ("void *",)),
    "alCSPSetTempo": ("void", ("void *", "s32")),
    "bcopy": ("void", ("void *", "void *", "s32")),
    "guLookAtReflect": ("void", ("void *", "void *") + (_F,) * 9),
    "guMtxIdent": ("void", ("void *",)),
    "guOrtho": ("void", ("void *",) + (_F,) * 7),
    "guPerspective": ("void", ("void *", "void *") + (_F,) * 5),
    "guRotate": ("void", ("void *",) + (_F,) * 4),
    "guScale": ("void", ("void *",) + (_F,) * 3),
    "guTranslate": ("void", ("void *",) + (_F,) * 3),
    "osCreateMesgQueue": ("void", ("void *", "void *", "s32")),
    "osCreateThread": ("void", ("void *", "s32", "void *", "void *", "void *", "s32")),
    "osStartThread": ("void", ("void *",)),
    "osVirtualToPhysical": ("u32", ("void *",)),
    "sins": ("s16", ("u16",)),
    "sprintf": ("s32", ("char *", "char *", "...")),
}


def scan(names):
    """name -> {'def': (ret, params) or None, 'decls': Counter}"""
    files = []
    for d in C_DIRS:
        files += sorted(os.path.join(d, f) for f in os.listdir(d) if f.endswith(".c"))
    files += EXTRA_C
    for d in HEADERS:
        files += sorted(os.path.join(d, f) for f in os.listdir(d) if f.endswith(".h"))
    info = {n: {"def": None, "decls": collections.Counter()} for n in names}
    for path in files:
        text = strip_comments(open(path, errors="replace").read())
        for m in DECL_RE.finditer(text):
            ret, star, name, params, end = (m.group(k) for k in range(1, 6))
            if name not in info:
                continue
            ret = ret.strip()
            if ret.split()[0] in ("return", "else", "if", "while", "case", "goto"):
                continue
            ret = (ret + " " + star).strip()
            sig = (ret, tuple(param_type(p) for p in split_params(params)))
            if end == "{":
                info[name]["def"] = sig
            else:
                info[name]["decls"][sig] += 1
        # K&R definitions: every argument is a promoted int (or a double)
        for m in KR_RE.finditer(text):
            ret, name, names, decls = m.group(1), m.group(2), m.group(3), m.group(4)
            if name not in info or info[name]["def"]:
                continue
            types = {}
            for d in decls.split(";"):
                d = d.strip()
                if not d:
                    continue
                w = d.replace(",", " ").split()
                base = "double" if w[0] in ("float", "f32", "f64", "double") else "s32"
                for v in re.findall(r"\*?\s*(\w+)", d[len(w[0]):]):
                    types[v] = base
            ps = tuple(types.get(a.strip(), "s32") for a in names.split(",") if a.strip())
            info[name]["def"] = (ret.strip(), ps)
    for name, sig in list(KNOWN.items()) + list(LIBULTRA_TYPED.items()):
        if name in info and not info[name]["def"]:
            info[name]["def"] = sig
    return info


def signature(name, entry):
    d = entry["def"]
    if d:
        return d, []
    if not entry["decls"]:
        return None, []
    common = entry["decls"].most_common()
    sig = common[0][0]
    notes = []
    classes = {(classify(s[0]), tuple(classify(p) if p != "..." else "..." for p in s[1]))
               for s, _ in common}
    if len(classes) > 1:
        notes.append("declarations disagree: " + " | ".join(
            f"{r} ({', '.join(p) or 'void'})" for (r, p), _ in common))
    # widest integer return type wins (over void, too)
    rets = {s[0] for s, _ in common}
    if len(rets) > 1:
        widths = {r: (4 if narrow_ctype(r) in ("uintptr_t",) else 2 if "16" in narrow_ctype(r) else 1)
                  for r in rets if classify(r) == "I"}
        if widths:
            sig = (max(widths, key=lambda r: widths[r]), sig[1])
    return sig, notes


def slots(pclasses):
    """o32 placement: list of (class, location) with location ('a', n),
    ('f', 12|14) or ('stack', offset)."""
    out = []
    off = 0
    fp_ok = bool(pclasses) and pclasses[0] in ("F", "D")
    nfp = 0
    for k, c in enumerate(pclasses):
        if c == "...":
            break
        size = 8 if c in ("D", "L") else 4
        if size == 8 and off % 8:
            off += 4
        if fp_ok and c in ("F", "D") and nfp < 2 and k == nfp:
            out.append((c, ("f", 12 + 2 * nfp), off))
            nfp += 1
        elif off < 16:
            out.append((c, ("a", off // 4), off))
        else:
            out.append((c, ("stack", off), off))
        off += size
    return out, max(off, 16)


def cty(c):
    return {"I": "uint32_t", "F": "float", "D": "double", "L": "uint64_t"}[c]


def gen_entry(name, sig, notes, live):
    ret, params = sig
    rc = classify(ret)
    variadic = "..." in params
    pcs = [classify(p) for p in params if p != "..."]
    placed, frame = slots(pcs)
    frame = (frame + 7) & ~7
    rty = "void" if rc == "V" else (narrow_ctype(ret) if rc == "I" else cty(rc))
    # a narrow result is returned extended in a whole register, as MIPS
    # returns it (the C that calls it may have declared an int)
    fty = "uintptr_t" if rc == "I" else rty
    args = ", ".join(f"{cty(c)} p{k}" for k, c in enumerate(pcs)) or "void"
    if variadic:
        args += ", ..."
    lines = []
    for n in notes:
        lines.append(f"/* {name}: {n} */")
    lines.append(f"{fty} {name}({args}) {{")
    lines.append("    recomp_context *ctx = port_ctx();")
    lines.append("    uint64_t sp = ctx->sp;")
    lines.append(f"    ctx->sp = sp - {frame};")
    for k, (c, loc, off) in enumerate(placed):
        v = f"p{k}"
        if loc[0] == "f":
            lines.append(f"    set_fpr_{'s' if c == 'F' else 'd'}(ctx, {loc[1]}, {v});")
        elif loc[0] == "a":
            r = 4 + loc[1]
            if c == "I":
                lines.append(f"    ctx->r[{r}] = S32({v});")
            elif c == "F":
                lines.append(f"    ctx->r[{r}] = S32(bits_of_f32({v}));")
            else:
                w = f"bits_of_f64({v})" if c == "D" else v
                lines.append(f"    ctx->r[{r}] = S32((uint32_t)({w} >> 32));")
                lines.append(f"    ctx->r[{r + 1}] = S32((uint32_t){w});")
        else:
            a = f"(uint32_t)ctx->sp + {loc[1]}"
            if c == "I":
                lines.append(f"    mem_w32(RDRAM, {a}, {v});")
            elif c == "F":
                lines.append(f"    mem_w32(RDRAM, {a}, bits_of_f32({v}));")
            elif c == "D":
                lines.append(f"    mem_w64(RDRAM, {a}, bits_of_f64({v}));")
            else:
                lines.append(f"    mem_w64(RDRAM, {a}, {v});")
    lines.append(f"    recomp_{name}(RDRAM, ctx);")
    lines.append("    ctx->sp = sp;")
    if rc == "I":
        lines.append(f"    return ({rty})(uint32_t)ctx->v0;" if rty == "uintptr_t" else
                     f"    return (uintptr_t)(uint32_t)({rty})ctx->v0;")
    elif rc == "F":
        lines.append("    return fpr_s(ctx, 0);")
    elif rc == "D":
        lines.append("    return fpr_d(ctx, 0);")
    elif rc == "L":
        lines.append("    return ((uint64_t)(uint32_t)ctx->v0 << 32) | (uint32_t)ctx->v1;")
    lines.append("}")
    # check the prototype against the registers the code reads
    warn = []
    for k, (c, loc, off) in enumerate(placed):
        if loc[0] == "f" and f"f{loc[1]}" not in live:
            warn.append(f"arg {k} in f{loc[1]} is never read")
    fl = {"f12", "f14"} & live
    if fl and not any(loc[0] == "f" for _, loc, _ in placed):
        warn.append(f"reads {', '.join(sorted(fl))} but the prototype passes no float in an FPR")
    for n in range(4):
        if (4 + n) in live and not any(loc == ("a", n) or (loc[0] == "a" and c in "DL" and loc[1] + 1 == n)
                                       for c, loc, _ in placed):
            warn.append(f"reads a{n}, which the prototype leaves unset")
    return lines, warn


def pty(c):
    """a native callee's parameter: an int may be a pointer"""
    return "uintptr_t" if c == "I" else cty(c)


def gen_extern(name, sig, is_lib, wraps=()):
    ret, params = sig
    rc = classify(ret) if not is_lib else ("V" if ret == "void" else "I")
    variadic = "..." in params
    pcs = [("I" if is_lib else classify(p)) for p in params if p != "..."]
    placed, _ = slots(pcs)
    rty = "void" if rc == "V" else cty(rc)
    if rc == "I" and not is_lib and narrow_ctype(ret) != "uintptr_t":
        # a u8/s16... result by its own type: i386 leaves the rest of eax
        # as it was, where IDO's v0 has it extended
        rty = narrow_ctype(ret)
    decl_args = ", ".join(pty(c) for c in pcs) or "void"
    if variadic:
        decl_args += ", ..."
    # a libc name the host's headers declare otherwise: by its symbol
    cname = f"port_libc_{name}" if name in LIBULTRA_TYPED else name
    asm = f' GLUE_SYM("{name}")' if cname != name else ""
    if name in wraps:
        cname, asm = f"__wrap_{name}", ""
    lines = [f"extern {rty} {cname}({decl_args}){asm};",
             f"void recomp_extern_{name}(uint8_t *rdram, recomp_context *ctx) {{"]
    vals = []
    for k, (c, loc, off) in enumerate(placed):
        if loc[0] == "f":
            vals.append(f"fpr_{'s' if c == 'F' else 'd'}(ctx, {loc[1]})")
        elif loc[0] == "a":
            r = 4 + loc[1]
            if c == "I":
                vals.append(f"(uintptr_t)(uint32_t)ctx->r[{r}]")
            elif c == "F":
                vals.append(f"f32_of((uint32_t)ctx->r[{r}])")
            elif c == "D":
                vals.append(f"f64_of(((uint64_t)(uint32_t)ctx->r[{r}] << 32) | (uint32_t)ctx->r[{r + 1}])")
            else:
                vals.append(f"(((uint64_t)(uint32_t)ctx->r[{r}] << 32) | (uint32_t)ctx->r[{r + 1}])")
        else:
            a = f"(uint32_t)ctx->sp + {loc[1]}"
            if c == "I":
                vals.append(f"(uintptr_t)mem_r32(rdram, {a})")
            elif c == "F":
                vals.append(f"f32_of(mem_r32(rdram, {a}))")
            elif c == "D":
                vals.append(f"f64_of(mem_r64(rdram, {a}))")
            else:
                vals.append(f"mem_r64(rdram, {a})")
    if variadic:
        # the translated code only calls the game's (empty) debug printf
        for r in range(len(placed), 4):
            vals.append(f"(uintptr_t)(uint32_t)ctx->r[{4 + r}]")
    # the LP64 build: a pointer to a native pointer (Gfx **), where the
    # translated code passes a 32-bit slot of its own (jp's IDO asm calls
    # func_80259BD4 with its display list pointer's): the callee gets a
    # native slot, and the translated code's slot gets its value back.  The
    # native slot is in game memory, below the N64 stack pointer (the
    # movable build's C reaches memory by N64 address): 8 bytes, the
    # pointer as the C has it (LP64 is native-endian)
    pre, post = [], []
    if LP64 and not is_lib:
        n = 0
        for k, p in enumerate(q for q in params if q != "..."):
            if p.count("*") >= 2 and "PTR32" not in p and k < len(placed):
                n += 1
                pre += [f"    uint32_t a{k} = (uint32_t){vals[k]};",
                        f"    uint32_t s{k} = (uint32_t)ctx->sp - {8 * n};",
                        f"    uint64_t v{k} = a{k} ? mem_r32_(rdram, a{k}) : 0;",
                        f"    memcpy(HOST(s{k}), &v{k}, 8);"]
                post += [f"    memcpy(&v{k}, HOST(s{k}), 8);",
                         f"    if (a{k}) mem_w32_(rdram, a{k}, (uint32_t)v{k});"]
                vals[k] = f"a{k} ? (uintptr_t)s{k} : 0"
        if n:       # (the arguments first: some are read off the stack)
            hoist = []
            for k, v in enumerate(vals):
                if not v.startswith(f"a{k} ?"):
                    hoist.append(f"    {pty(placed[k][0]) if k < len(placed) else 'uintptr_t'} p{k} = {v};")
                    vals[k] = f"p{k}"
            pre = hoist + pre + [f"    ctx->sp -= {(8 * n + 15) // 16 * 16};"]
    call = f"{cname}({', '.join(vals)})"
    lines.append("    PORT_CALLEE_SAVE(ctx);")
    lines += pre
    if rc == "V":
        lines.append(f"    {call};")
        lines.append("    PORT_CALLEE_RESTORE(ctx);")
        lines += post
    else:
        lines.append(f"    {rty} r = {call};")
        lines.append("    PORT_CALLEE_RESTORE(ctx);")
        lines += post
        if rc == "I":
            lines.append("    ctx->v0 = S32(r);")
        elif rc == "F":
            lines.append("    set_fpr_s(ctx, 0, r);")
        elif rc == "D":
            lines.append("    set_fpr_d(ctx, 0, r);")
        else:
            lines.append("    ctx->v0 = S32((uint32_t)(r >> 32)); ctx->v1 = S32((uint32_t)r);")
    lines.append("}")
    return lines


HEADER = """/* Generated by port/tools/gen_glue.py; do not edit. */
#include "recomp.h"
#include "recomp_funcs.h"
#include "recomp_externs.h"
#include "port_recomp.h"
/* a C name's symbol, with the target's prefix (Mach-O's "_") */
#define GLUE_STR2(x) #x
#define GLUE_STR(x) GLUE_STR2(x)
#define GLUE_SYM(n) __asm__(GLUE_STR(__USER_LABEL_PREFIX__) n)
"""

# libc calls out of jp's IDO asm (tools/recomp/config.py) that can't take
# the N64's addresses as the host's (the movable build's are in the arena)
# or their arguments from a0-a3 alone.  bcopy is the game's C's (n64_bcopy,
# port_game.h).  sprintf takes its arguments by the format, from a2 on and
# then off the N64 stack (jp's func_80263358 has "%d MINUTE%c %d SECONDS"),
# each conversion formatted by the host's, and copies the result in, as
# n64_sprintf does (port/host/libc64.c).
BY_HAND = {
    "bcopy": """extern void n64_bcopy(const void *, void *, int);
void recomp_extern_bcopy(uint8_t *rdram, recomp_context *ctx) {
    PORT_CALLEE_SAVE(ctx);
    n64_bcopy(HOST(ctx->r[4]), HOST(ctx->r[5]), (int)(uint32_t)ctx->r[6]);
    PORT_CALLEE_RESTORE(ctx);
}""",
    "sprintf": """#include <stdio.h>
#include <string.h>
extern void host_fatal(const char *fmt, ...);
void recomp_extern_sprintf(uint8_t *rdram, recomp_context *ctx) {
    const char *fmt = (const char *)HOST(ctx->r[5]);
    char out[1024], spec[32];
    size_t len = 0;
    uint32_t arg = 2;       /* the next argument's word: a2, a3, then sp+16... */
    while (*fmt) {
        const char *s = fmt;
        if (*fmt != '%' || fmt[1] == '%') {
            fmt += *fmt == '%' ? 2 : 1;
            if (len < sizeof out - 1)
                out[len++] = *s;
            continue;
        }
        size_t sl = 0;
        spec[sl++] = *fmt++;
        while (*fmt && strchr("-+ #0123456789.", *fmt) && sl < sizeof spec - 2)
            spec[sl++] = *fmt++;
        while (*fmt == 'h' || *fmt == 'l')      /* (every argument is a word) */
            fmt++;
        char conv = *fmt ? *fmt++ : 0;
        spec[sl++] = conv;
        spec[sl] = 0;
        uint32_t a = arg < 4 ? (uint32_t)ctx->r[4 + arg] : mem_r32_(rdram, (uint32_t)ctx->sp + 4 * arg);
        arg++;
        int k;
        if (conv == 's')
            k = snprintf(out + len, sizeof out - len, spec, a ? (const char *)HOST(a) : "(null)");
        else if (conv && strchr("diouxXc", conv))
            k = snprintf(out + len, sizeof out - len, spec, (int)a);
        else
            host_fatal("sprintf from the translated code: %%%c in \\"%s\\"", conv, (const char *)HOST(ctx->r[5]));
        if (k > 0)
            len += (size_t)k < sizeof out - len ? (size_t)k : sizeof out - 1 - len;
    }
    out[len] = 0;
    memcpy(HOST(ctx->r[4]), out, len + 1);
    ctx->v0 = S32((uint32_t)len);
}""",
}


# ---- the engine's replacement (docs/PORT.md, "Replacing the engine") ------
#
# port/engine/replaced.txt lists the translated functions that are native C
# now (port/engine/*.c).  The translated code calls each of them through
# recomp_extern_X like any call out, and the glue for it is made here from
# its definition and its register convention: REGS(in... -> out...) before
# the definition, or o32 by the prototype.  The native code calls the
# translated functions through entry.c's wrappers, whose conventions come
# the same way from its declarations (a REGS() before one).  With --check
# (PORT_ENGINE_CHECK), every call of a replaced function from the
# translated code or the game's C also runs the original's translation on
# the same state and compares (port/host/engine_check.c).

ENGINE = os.path.join(ROOT, "port", "engine")
GPR = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
       "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"]
REGS_RE = re.compile(r"\bREGS\(([^)]*)\)")


def engine_files():
    if not os.path.isdir(ENGINE):
        return []
    return [os.path.join(ENGINE, f) for f in sorted(os.listdir(ENGINE)) if f.endswith((".c", ".h"))]


def load_replaced(funcs):
    path = os.path.join(ENGINE, "replaced.txt")
    out = []
    if os.path.exists(path):
        for line in open(path):
            line = line.split("#", 1)[0].split()
            if line and line[0] in funcs:
                out.append(line[0])
    return out


def reg_index(r):
    """a REGS() name: ('g', n), ('f', n), ('hi',), ('lo',), ('stack', off)"""
    r = r.strip()
    if r in GPR:
        return ("g", GPR.index(r))
    if re.fullmatch(r"f([0-9]|[12][0-9]|3[01])", r):
        return ("f", int(r[1:]))
    if r in ("hi", "lo"):
        return (r,)
    m = re.fullmatch(r"sp\+(0x[0-9A-Fa-f]+|\d+)", r)
    if m:
        return ("stack", int(m.group(1), 0))
    sys.exit(f"gen_glue.py: REGS(): no register {r!r}")


def parse_regs(text):
    """name -> (inputs, outputs) of every REGS() before a declaration"""
    out = {}
    for m in REGS_RE.finditer(text):
        spec = m.group(1)
        ins, _, outs = spec.partition("->")
        ins = [x for x in (s.strip() for s in ins.split(",")) if x]
        outs = [x for x in (s.strip() for s in outs.split(",")) if x]
        d = DECL_RE.search(text, m.end())
        if not d:
            sys.exit(f"gen_glue.py: REGS({spec}) before no declaration")
        name = d.group(3)
        conv = ([reg_index(r) for r in ins], [reg_index(r) for r in outs])
        if name in out and out[name] != conv:
            sys.exit(f"gen_glue.py: {name}: two different REGS()")
        out[name] = conv
    return out


def engine_conventions():
    regs = {}
    for path in engine_files():
        for k, v in parse_regs(strip_comments_keep_regs(open(path).read())).items():
            if k in regs and regs[k] != v:
                sys.exit(f"gen_glue.py: {k}: two different REGS() in port/engine")
            regs[k] = v
    return regs


def strip_comments_keep_regs(s):
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    s = re.sub(r"//[^\n]*", " ", s)
    return re.sub(r"^\s*#.*$", " ", s, flags=re.M)


def o32_convention(sig):
    """(inputs, outputs) as REGS() would give them, for an o32 prototype"""
    ret, params = sig
    pcs = [classify(p) for p in params if p != "..."]
    placed, _ = slots(pcs)
    ins = []
    for c, loc, off in placed:
        if loc[0] == "f":
            ins.append(("f", loc[1]))
        elif loc[0] == "a":
            ins.append(("pair", 4 + loc[1]) if c in "DL" else ("g", 4 + loc[1]))
        else:
            ins.append(("stack", loc[1]) if c not in "DL" else ("stack64", loc[1]))
    rc = classify(ret)
    outs = {"V": [], "I": [("g", 2)], "F": [("f", 0)], "D": [("f", 0)], "L": [("pair", 2)]}[rc]
    return ins, outs


def get_reg(reg, c, rd="rdram"):
    """C expression: the native value of class c in a register of the context"""
    k = reg[0]
    if k == "g":
        r = f"ctx->r[{reg[1]}]"
        return {"I": f"(uintptr_t)(uint32_t){r}", "L": r, "F": f"f32_of((uint32_t){r})",
                "D": f"f64_of({r})"}[c]
    if k == "pair":         # o32's 64-bit argument in two registers
        v = f"(((uint64_t)(uint32_t)ctx->r[{reg[1]}] << 32) | (uint32_t)ctx->r[{reg[1] + 1}])"
        return v if c == "L" else f"f64_of({v})"
    if k == "f":
        return {"F": f"fpr_s(ctx, {reg[1]})", "D": f"fpr_d(ctx, {reg[1]})",
                "I": f"(uintptr_t)ctx->f[{reg[1]}]", "L": f"fpr_l(ctx, {reg[1]})"}[c]
    if k in ("hi", "lo"):
        return f"(uintptr_t)(uint32_t)ctx->{k}" if c == "I" else f"ctx->{k}"
    a = f"(uint32_t)ctx->sp + {reg[1]}"
    return {"I": f"(uintptr_t)mem_r32({rd}, {a})", "F": f"f32_of(mem_r32({rd}, {a}))",
            "D": f"f64_of(mem_r64({rd}, {a}))", "L": f"mem_r64({rd}, {a})"}[c]


def set_reg(reg, c, v, rd="RDRAM"):
    """C statement: put the native value v of class c in a register"""
    k = reg[0]
    if k == "g":
        r = f"ctx->r[{reg[1]}]"
        return {"I": f"{r} = S32({v});", "L": f"{r} = {v};", "F": f"{r} = S32(bits_of_f32({v}));",
                "D": f"{r} = bits_of_f64({v});"}[c]
    if k == "pair":
        w = f"bits_of_f64({v})" if c == "D" else v
        return (f"ctx->r[{reg[1]}] = S32((uint32_t)({w} >> 32)); "
                f"ctx->r[{reg[1] + 1}] = S32((uint32_t)({w}));")
    if k == "f":
        return {"F": f"set_fpr_s(ctx, {reg[1]}, {v});", "D": f"set_fpr_d(ctx, {reg[1]}, {v});",
                "I": f"ctx->f[{reg[1]}] = (uint32_t)({v});", "L": f"set_fpr_l(ctx, {reg[1]}, {v});"}[c]
    if k in ("hi", "lo"):
        return f"ctx->{k} = S32({v});" if c == "I" else f"ctx->{k} = {v};"
    a = f"(uint32_t)ctx->sp + {reg[1]}"
    return {"I": f"mem_w32({rd}, {a}, (uint32_t)({v}));", "F": f"mem_w32({rd}, {a}, bits_of_f32({v}));",
            "D": f"mem_w64({rd}, {a}, bits_of_f64({v}));", "L": f"mem_w64({rd}, {a}, {v});"}[c]


def split_sig(name, sig, conv):
    """the parameters' classes, which are inputs and which are results
    through a pointer; the return's class"""
    ret, params = sig
    ins, outs = conv
    ps = [p for p in params if p != "..."]
    rc = classify(ret)
    nres = len(outs) - (rc != "V")
    if len(ps) != len(ins) + nres:
        sys.exit(f"gen_glue.py: {name}: REGS() has {len(ins)} inputs and {len(outs)} outputs, "
                 f"the prototype {len(ps)} parameters and returns {ret}")
    in_c = [classify(p) for p in ps[:len(ins)]]
    out_c = ([rc] if rc != "V" else []) + [classify(p.replace("*", "", 1)) for p in ps[len(ins):]]
    for p in ps[len(ins):]:
        if "*" not in p:
            sys.exit(f"gen_glue.py: {name}: result parameter {p!r} isn't a pointer")
    return in_c, out_c, rc


def reg_bits(reg, c):
    """the check's register numbers (engine_check.c): 0-31 GPRs, 32 hi, 33 lo,
    34-65 FPR words"""
    k = reg[0]
    if k == "g":
        return [reg[1]]
    if k == "pair":
        return [reg[1], reg[1] + 1]
    if k == "f":
        return [34 + reg[1]] + ([35 + reg[1]] if c in "DL" else [])
    if k in ("hi", "lo"):
        return [32 if k == "hi" else 33]
    return []


def native_decl(name, sig, cname=None):
    ret, params = sig
    rc = classify(ret)
    rty = "void" if rc == "V" else cty(rc)
    if rc == "I" and narrow_ctype(ret) != "uintptr_t":
        rty = narrow_ctype(ret)
    elif rc == "I":
        rty = "uintptr_t"
    args = ", ".join(pty(classify(p)) for p in params if p != "...") or "void"
    return f"extern {rty} {cname or name}({args});", rty


def gen_adapter(name, sig, conv, cname, check=None):
    """recomp_extern_X: the translated code calls the native X"""
    in_c, out_c, rc = split_sig(name, sig, conv)
    ins, outs = conv
    decl, rty = native_decl(name, sig, cname)
    lines = [decl, f"void recomp_extern_{name}(uint8_t *rdram, recomp_context *ctx) {{"]
    body = ["    recomp_context saved = *ctx;"]
    args = [get_reg(r, c) for r, c in zip(ins, in_c)]
    res_slots = out_c[1:] if rc != "V" else out_c
    if res_slots:
        # results through pointers: slots below the N64 stack, which the
        # native code's own calls into translated code then stay below
        body.append(f"    uint32_t res = (uint32_t)ctx->sp - {8 * len(res_slots)};")
        body.append(f"    ctx->sp -= {(8 * len(res_slots) + 15) // 16 * 16};")
        args += [f"(uintptr_t)(res + {8 * k})" for k in range(len(res_slots))]
    call = f"{cname}({', '.join(args)})"
    body.append(f"    {call};" if rc == "V" else f"    {rty} r = {call};")
    body.append("    *ctx = saved;")
    vals = []
    if rc != "V":
        vals.append("r")
    for k, c in enumerate(res_slots):
        vals.append({"I": f"mem_r32(rdram, res + {8 * k})", "F": f"f32_of(mem_r32(rdram, res + {8 * k}))",
                     "D": f"f64_of(mem_r64(rdram, res + {8 * k}))", "L": f"mem_r64(rdram, res + {8 * k})"}[c])
    for reg, c, v in zip(outs, out_c, vals):
        body.append("    " + set_reg(reg, c, v, "rdram"))
    if check is None:
        return lines + body + ["}"]
    cid, mask = check
    lines += [f"    if (engine_check_begin({cid}, ctx)) {{",
              f"        recomp_orig_{name}(rdram, ctx);",
              f"        engine_check_mid({cid}, ctx);",
              "    }"]
    lines += body
    lines += [f"    engine_check_end({cid}, ctx, {mask[0]:#x}ull, {mask[1]:#x}ull, {mask[2]:#x}ull);", "}"]
    return lines


def gen_entry_regs(name, sig, conv, cname=None, orig=False):
    """the native code calls a translated function (or, with orig, the
    check's wrapper of a replaced one runs its translation) with REGS()"""
    in_c, out_c, rc = split_sig(name, sig, conv)
    ins, outs = conv
    ret, params = sig
    ps = [p for p in params if p != "..."]
    rty = "void" if rc == "V" else (narrow_ctype(ret) if rc == "I" else cty(rc))
    fty = "uintptr_t" if rc == "I" else rty
    args = ", ".join(f"{pty(classify(p)) if k < len(ins) else 'uintptr_t'} p{k}" for k, p in enumerate(ps)) or "void"
    lines = [f"{fty} {cname or name}({args}) {{",
             "    recomp_context *ctx = port_ctx();",
             "    uint64_t sp = ctx->sp;",
             "    ctx->sp = sp - 32;"]
    stack = [r for r in ins if r[0] == "stack"]
    if stack:
        top = max(r[1] for r in stack) + 8
        lines[-1] = f"    ctx->sp = sp - {(top + 15) // 16 * 16};"
    for k, (reg, c) in enumerate(zip(ins, in_c)):
        lines.append("    " + set_reg(reg, c, f"p{k}"))
    lines.append(f"    recomp_{'orig_' if orig else ''}{name}(RDRAM, ctx);")
    lines.append("    ctx->sp = sp;")
    res_ps = list(range(len(ins), len(ps)))
    res_regs = outs[1:] if rc != "V" else outs
    res_cls = out_c[1:] if rc != "V" else out_c
    for k, reg, c in zip(res_ps, res_regs, res_cls):
        v = get_reg(reg, c)
        w = {"I": f"mem_w32(RDRAM, (uint32_t)p{k}, (uint32_t){v});",
             "F": f"mem_w32(RDRAM, (uint32_t)p{k}, bits_of_f32({v}));",
             "D": f"mem_w64(RDRAM, (uint32_t)p{k}, bits_of_f64({v}));",
             "L": f"mem_w64(RDRAM, (uint32_t)p{k}, {v});"}[c]
        lines.append(f"    if (p{k}) {w}")
    if rc != "V":
        v = get_reg(outs[0], out_c[0])
        if rc == "I":
            lines.append(f"    return ({rty})(uint32_t){v};" if rty == "uintptr_t" else
                         f"    return (uintptr_t)(uint32_t)({rty}){v};")
        else:
            lines.append(f"    return {v};")
    lines.append("}")
    return lines


def check_mask(name, conv_regs, analysis):
    """the registers the check compares: the declared results and what the
    analysis says a caller reads"""
    bits = set()
    for reg, c in conv_regs:
        bits |= set(reg_bits(reg, c))
    if analysis is not None:
        for r in analysis.outputs:
            if isinstance(r, int):
                bits.add(r)
            elif r in ("hi", "lo"):
                bits.add(32 if r == "hi" else 33)
            elif r.startswith("f"):
                bits.add(34 + int(r[1:]))
    m = [0, 0, 0]
    for b in bits:
        m[b // 64] |= 1 << (b % 64)
    return m


def reaches_extern(funcs):
    """name -> whether its translation calls (at any depth) something that
    isn't translated: the check only runs those that don't (an original
    that calls the game's C, or libultra, can't be run twice)"""
    calls = {n: {ln.target for ln in fn.lines if ln.insn.op in ("jal", "j")} for n, fn in funcs.items()}
    memo = {}

    def go(n, stack):
        if n in memo:
            return memo[n]
        if n not in funcs:
            return True
        if n in stack:
            return False
        stack.add(n)
        r = any(go(c, stack) for c in calls[n])
        stack.discard(n)
        memo[n] = r
        return r
    return {n: go(n, set()) for n in funcs}


def gen_engine(check, funcs, report):
    """the engine's glue: returns (adapters by name, C-side lines, names the
    native code calls, the check's header lines)"""
    replaced = load_replaced(funcs)
    regs = engine_conventions()
    if not replaced and not regs:
        return {}, [], set(), [], {}, {}
    import conventions as convmod
    analysis = convmod.conventions(funcs) if replaced else {}
    defs = scan_engine(set(replaced) | set(regs))
    adapters, cside, check_hdr = {}, [], []
    ext = reaches_extern(funcs)
    ids = []
    for cid, name in enumerate(replaced):
        sig = defs.get(name)
        if sig is None:
            sys.exit(f"gen_glue.py: {name} is in port/engine/replaced.txt but port/engine defines it nowhere")
        conv = regs.get(name) or o32_convention(sig)
        in_c, out_c, rc = split_sig(name, sig, conv)
        a = analysis.get(name)
        if a is not None:
            declared = set()
            for reg, c in zip(conv[1], out_c):
                declared |= set(reg_bits(reg, c))
            missing = []
            for r in sorted(a.outputs, key=convmod.reg_order):
                b = r if isinstance(r, int) else (32 if r == "hi" else 33 if r == "lo" else
                                                   34 + int(r[1:]) if r[0] == "f" and r[1:].isdigit() else None)
                if b is not None and b not in declared:
                    missing.append(convmod.regname(r))
            if missing:
                report.append(f"{name}: a translated caller may read {', '.join(missing)} afterwards, "
                              f"which REGS() doesn't give back")
            ins_decl = {r for r in conv[0]}
            extra_in = [convmod.regname(r) for r in sorted(a.inputs - {29}, key=convmod.reg_order)
                        if (("g", r) if isinstance(r, int) else ("f", int(r[1:])) if r[0] == "f" and r[1:].isdigit()
                            else (r,)) not in ins_decl and r != "fcc"]
            if extra_in:
                report.append(f"{name}: reads {', '.join(extra_in)} as well (left as the caller has them)")
        mask = check_mask(name, list(zip(conv[1], out_c)), a)
        checked = check and not ext.get(name, True)
        cname = f"native_{name}" if check else name
        adapters[name] = gen_adapter(name, sig, conv, cname, (cid, mask) if checked else None)
        ids.append((cid, name, checked))
        if check:
            check_hdr.append(f"#define {name} native_{name}")
            # the game's C (and the translated code's glue) calls func_X: here
            # it runs the original first when the check wants it
            if checked and conv == o32_convention(sig):
                cside += gen_c_check(name, sig, cid, mask)
            else:
                decl, rty = native_decl(name, sig, cname)
                ps = [p for p in sig[1] if p != "..."]
                args = ", ".join(f"{pty(classify(p))} p{k}" for k, p in enumerate(ps)) or "void"
                call = f"{cname}({', '.join(f'p{k}' for k in range(len(ps)))})"
                cside += [decl, f"{rty} {name}({args}) {{",
                          f"    {'return ' if classify(sig[0]) != 'V' else ''}{call};", "}"]
    if check:
        cside.insert(0, "const char *const engine_check_names[] = {" +
                     ", ".join(f'"{n}"' for _, n, _ in ids) + "};")
        cside.insert(1, f"const unsigned engine_check_count = {len(ids)};")
    called = set()
    for path in engine_files():
        called |= set(re.findall(r"\bfunc_[0-9A-F]{8}\b", open(path).read()))
    return adapters, cside, called, check_hdr, regs, defs


def gen_c_check(name, sig, cid, mask):
    """the check's func_X for the game's C: the translation on the same
    state first, then the native function"""
    ret, params = sig
    rc = classify(ret)
    decl, rty = native_decl(name, sig, f"native_{name}")
    ps = [p for p in params if p != "..."]
    args = ", ".join(f"{pty(classify(p))} p{k}" for k, p in enumerate(ps)) or "void"
    conv = o32_convention(sig)
    ent = gen_entry_regs(name, sig, conv, f"orig_{name}", orig=True)
    lines = [decl] + ["static " + ent[0]] + ent[1:]
    call = f"native_{name}({', '.join(f'p{k}' for k in range(len(ps)))})"
    lines += [f"{rty} {name}({args}) {{",
              "    recomp_context *ctx = port_ctx();",
              f"    if (engine_check_begin({cid}, ctx)) {{",
              f"        orig_{name}({', '.join(f'p{k}' for k in range(len(ps)))});",
              f"        engine_check_mid({cid}, ctx);",
              "    }"]
    if rc == "V":
        lines.append(f"    {call};")
    else:
        lines.append(f"    {rty} r = {call};")
        lines.append("    if (engine_checking())")
        lines.append("        " + set_reg(conv[1][0], rc, "r"))
    lines.append(f"    engine_check_end({cid}, ctx, {mask[0]:#x}ull, {mask[1]:#x}ull, {mask[2]:#x}ull);")
    if rc != "V":
        lines.append("    return r;")
    lines.append("}")
    return lines


def scan_engine(names):
    """name -> (ret, params) from port/engine's definitions and declarations"""
    out = {}
    for path in engine_files():
        text = strip_comments(open(path).read())
        text = REGS_RE.sub(" ", text)
        for m in DECL_RE.finditer(text):
            ret, star, name, params, end = (m.group(k) for k in range(1, 6))
            if name not in names:
                continue
            ret = (ret.strip() + " " + star).strip()
            ret = re.sub(r"^(static|extern|inline)\s+", "", ret)
            sig = (ret, tuple(param_type(p) for p in split_params(params)))
            if end == "{" or name not in out:
                out[name] = sig
    return out


def main():
    outdir = sys.argv[1]
    wraps = set()
    check = "--check" in sys.argv
    if "--wrap" in sys.argv:
        k = sys.argv.index("--wrap")
        wraps = {w for w in sys.argv[k + 1].split(",") if w}
    os.makedirs(outdir, exist_ok=True)
    gen = os.path.join(BLAST, "build", "recomp", "src")
    rfuncs = re.findall(r"recomp_(func_[0-9A-F]{8})\(", open(os.path.join(gen, "recomp_funcs.h")).read())
    externs = re.findall(r"recomp_extern_(\w+)\(", open(os.path.join(gen, "recomp_externs.h")).read())
    cnames = set()
    for d in C_DIRS:
        for f in os.listdir(d):
            if f.endswith(".c"):
                cnames |= set(re.findall(r"\bfunc_[0-9A-F]{8}\b", open(os.path.join(d, f)).read()))
    funcs = liveness.load_functions()
    report = []
    adapters, cside, ecalled, check_hdr, eregs, edefs = gen_engine(check, funcs, report) \
        if engine_files() else ({}, [], set(), [], {}, {})
    entries = sorted(set(rfuncs) & (cnames | ecalled))
    info = scan(set(entries) | set(externs))
    live = {n: {liveness.regname(r) for r in s} for n, s in liveness.liveness(funcs).items()}

    out = [HEADER]
    for name in entries:
        if name in eregs:
            # the native code calls it with a convention of its own
            sig = edefs.get(name)
            if sig is None:
                sys.exit(f"gen_glue.py: REGS() for {name} but no declaration")
            out += gen_entry_regs(name, sig, eregs[name]) + [""]
            continue
        sig, notes = signature(name, info[name])
        if sig is None:
            sys.exit(f"gen_glue.py: no prototype for {name}")
        lines, warn = gen_entry(name, sig, notes, live.get(name, set()))
        out += lines + [""]
        for w in notes + warn:
            report.append(f"{name}: {w}")
    if check:
        out += ["/* PORT_ENGINE_CHECK (port/host/engine_check.c) */",
                "int engine_check_begin(unsigned id, recomp_context *ctx);",
                "void engine_check_mid(unsigned id, recomp_context *ctx);",
                "void engine_check_end(unsigned id, recomp_context *ctx, uint64_t m0, uint64_t m1, uint64_t m2);",
                "int engine_checking(void);", ""] + cside + [""]
    with open(os.path.join(outdir, "entry.c"), "w") as f:
        f.write("\n".join(out))
    with open(os.path.join(outdir, "engine_check_names.h"), "w") as f:
        f.write("/* Generated by port/tools/gen_glue.py: with PORT_ENGINE_CHECK the native\n"
                "   functions get other names, and the glue's func_X runs the check. */\n")
        f.write("\n".join(check_hdr) + "\n")

    out = [HEADER]
    if check:
        out += ["int engine_check_begin(unsigned id, recomp_context *ctx);",
                "void engine_check_mid(unsigned id, recomp_context *ctx);",
                "void engine_check_end(unsigned id, recomp_context *ctx, uint64_t m0, uint64_t m1, uint64_t m2);",
                ""]
    for name in externs:
        if name in adapters:
            out += adapters[name] + [""]
            continue
        if name in BY_HAND:
            out += [BY_HAND[name], ""]
            continue
        if name in LIBULTRA:
            ret, ps = LIBULTRA[name]
            out += gen_extern(name, (ret, ps), True, wraps) + [""]
            continue
        sig, notes = signature(name, info[name])
        if sig is None:
            sys.exit(f"gen_glue.py: no prototype for {name}")
        if info[name]["def"] is None:
            report.append(f"{name}: extern without a C definition")
        out += gen_extern(name, sig, False, wraps) + [""]
    with open(os.path.join(outdir, "externs.c"), "w") as f:
        f.write("\n".join(out))
    # the names the translated code takes the address of (a lui/addiu pair):
    # the movable build's functions used as values (-port-arena-fn-values)
    taken = set()
    for f in sorted(os.listdir(gen)):
        if f.endswith(".c"):
            taken |= set(re.findall(r"\bHI16\(SYM\((\w+)\)\)", open(os.path.join(gen, f)).read()))
    with open(os.path.join(outdir, "fn_values.txt"), "w") as f:
        f.write("".join(n + "\n" for n in sorted(taken)))
    with open(os.path.join(outdir, "glue_report.txt"), "w") as f:
        f.write("\n".join(report) + "\n")
    print(f"gen_glue.py: {len(entries)} entries, {len(externs)} externs, {len(report)} notes")


if __name__ == "__main__":
    main()
