#!/usr/bin/env python3
"""Generate the glue between native C and the translated handwritten code.

  entry.c    one native function per translated function that C calls by
             name (167): it moves the arguments into the thread's
             recomp_context by the o32 rules (a0-a3, f12/f14, the stack),
             calls recomp_func_X and returns v0/f0.
  externs.c  recomp_extern_X for every call out of the translated code (53):
             it takes the arguments out of the context by the same rules,
             calls the native function and puts its result in v0/f0.

Prototypes come from the C: the definition for a C function, the
declarations for a translated one (the most common spelling where files
disagree).  Only the o32 class of each argument matters (int, float,
double, 64-bit int): integer arguments are passed as 32-bit words, which is
what IDO puts in a register for any narrower type as well.  The live-in
registers of each translated function (liveness.py) are checked against
the prototype: a float argument has to be read from f12/f14 when it comes
first, and an int one from a0-a3.

usage: gen_glue.py OUTDIR
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
        return "uint32_t"
    return {"u8": "uint8_t", "s8": "int8_t", "u16": "uint16_t", "s16": "int16_t",
            "unsigned char": "uint8_t", "char": "int8_t", "signed char": "int8_t",
            "unsigned short": "uint16_t", "short": "int16_t"}.get(t, "uint32_t")


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
    for name, sig in KNOWN.items():
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
        widths = {r: (4 if narrow_ctype(r) in ("uint32_t",) else 2 if "16" in narrow_ctype(r) else 1)
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
    args = ", ".join(f"{cty(c)} p{k}" for k, c in enumerate(pcs)) or "void"
    if variadic:
        args += ", ..."
    lines = []
    for n in notes:
        lines.append(f"/* {name}: {n} */")
    lines.append(f"{rty} {name}({args}) {{")
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
        lines.append(f"    return ({rty})ctx->v0;")
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


def gen_extern(name, sig, is_lib):
    ret, params = sig
    rc = classify(ret) if not is_lib else ("V" if ret == "void" else "I")
    variadic = "..." in params
    pcs = [("I" if is_lib else classify(p)) for p in params if p != "..."]
    placed, _ = slots(pcs)
    rty = "void" if rc == "V" else cty(rc)
    decl_args = ", ".join(cty(c) for c in pcs) or "void"
    if variadic:
        decl_args += ", ..."
    lines = [f"extern {rty} {name}({decl_args});",
             f"void recomp_extern_{name}(uint8_t *rdram, recomp_context *ctx) {{"]
    vals = []
    for k, (c, loc, off) in enumerate(placed):
        if loc[0] == "f":
            vals.append(f"fpr_{'s' if c == 'F' else 'd'}(ctx, {loc[1]})")
        elif loc[0] == "a":
            r = 4 + loc[1]
            if c == "I":
                vals.append(f"(uint32_t)ctx->r[{r}]")
            elif c == "F":
                vals.append(f"f32_of((uint32_t)ctx->r[{r}])")
            elif c == "D":
                vals.append(f"f64_of(((uint64_t)(uint32_t)ctx->r[{r}] << 32) | (uint32_t)ctx->r[{r + 1}])")
            else:
                vals.append(f"(((uint64_t)(uint32_t)ctx->r[{r}] << 32) | (uint32_t)ctx->r[{r + 1}])")
        else:
            a = f"(uint32_t)ctx->sp + {loc[1]}"
            if c == "I":
                vals.append(f"mem_r32(rdram, {a})")
            elif c == "F":
                vals.append(f"f32_of(mem_r32(rdram, {a}))")
            elif c == "D":
                vals.append(f"f64_of(mem_r64(rdram, {a}))")
            else:
                vals.append(f"mem_r64(rdram, {a})")
    if variadic:
        # the translated code only calls the game's (empty) debug printf
        for r in range(len(placed), 4):
            vals.append(f"(uint32_t)ctx->r[{4 + r}]")
    call = f"{name}({', '.join(vals)})"
    lines.append("    PORT_CALLEE_SAVE(ctx);")
    if rc == "V":
        lines.append(f"    {call};")
        lines.append("    PORT_CALLEE_RESTORE(ctx);")
    else:
        lines.append(f"    {rty} r = {call};")
        lines.append("    PORT_CALLEE_RESTORE(ctx);")
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
"""


def main():
    outdir = sys.argv[1]
    os.makedirs(outdir, exist_ok=True)
    gen = os.path.join(BLAST, "build", "recomp", "src")
    rfuncs = re.findall(r"recomp_(func_[0-9A-F]{8})\(", open(os.path.join(gen, "recomp_funcs.h")).read())
    externs = re.findall(r"recomp_extern_(\w+)\(", open(os.path.join(gen, "recomp_externs.h")).read())
    cnames = set()
    for d in C_DIRS:
        for f in os.listdir(d):
            if f.endswith(".c"):
                cnames |= set(re.findall(r"\bfunc_[0-9A-F]{8}\b", open(os.path.join(d, f)).read()))
    entries = sorted(set(rfuncs) & cnames)
    info = scan(set(entries) | set(externs))
    funcs = liveness.load_functions()
    live = {n: {liveness.regname(r) for r in s} for n, s in liveness.liveness(funcs).items()}

    out = [HEADER]
    report = []
    for name in entries:
        sig, notes = signature(name, info[name])
        if sig is None:
            sys.exit(f"gen_glue.py: no prototype for {name}")
        lines, warn = gen_entry(name, sig, notes, live.get(name, set()))
        out += lines + [""]
        for w in notes + warn:
            report.append(f"{name}: {w}")
    with open(os.path.join(outdir, "entry.c"), "w") as f:
        f.write("\n".join(out))

    out = [HEADER]
    for name in externs:
        if name in LIBULTRA:
            ret, ps = LIBULTRA[name]
            out += gen_extern(name, (ret, ps), True) + [""]
            continue
        sig, notes = signature(name, info[name])
        if sig is None:
            sys.exit(f"gen_glue.py: no prototype for {name}")
        if info[name]["def"] is None:
            report.append(f"{name}: extern without a C definition")
        out += gen_extern(name, sig, False) + [""]
    with open(os.path.join(outdir, "externs.c"), "w") as f:
        f.write("\n".join(out))
    with open(os.path.join(outdir, "glue_report.txt"), "w") as f:
        f.write("\n".join(report) + "\n")
    print(f"gen_glue.py: {len(entries)} entries, {len(externs)} externs, {len(report)} notes")


if __name__ == "__main__":
    main()
