#!/usr/bin/env python3
"""Mechanical translation of Rare's handwritten MIPS into C.

    tools/recomp/translate.py OUTDIR [--elf module=path ...]

Writes, for every translated object (config.py):

    OUTDIR/<module>_<object>.c   one C function per asm function
    OUTDIR/recomp_funcs.h        prototypes of the translated functions
    OUTDIR/recomp_externs.h      prototypes of the calls out (externs)
    OUTDIR/externs.c             default externs: all go to recomp_call_external
    OUTDIR/funcs.c               address -> function table, coverage size
    OUTDIR/syms_<module>.h       SYM() values, from the module's linked ELF

The C works on the register file and emulated RDRAM described in
runtime/recomp.h.  Output is deterministic and keeps the .s file's symbols:
no address in it is a literal.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from config import translated_objects, BLAST
from mips import GPR_NAMES
from parse import parse_file, is_terminator


class TranslateError(Exception):
    pass


def R(n):
    return "0" if n == 0 else f"ctx->{GPR_NAMES[n]}"


def W(n, expr):
    """Assignment to GPR n; writes to $zero are dropped."""
    if n == 0:
        return f"(void)({expr});"
    return f"ctx->{GPR_NAMES[n]} = {expr};"


def sym(name):
    return f"SYM({name})"


# Native-endian memory (RECOMP_NATIVE_ENDIAN; docs/PORT.md "Native-endian
# memory"): the accesses that move data at another width than it has.
# native_sites.txt lists them by function and offset, with a kind:
#   h2   a word that is two 16-bit fields (lw/sw/lwc1-free): halves exchanged
#   be   a big-endian datum (texels, what the RDP reads as bytes): the
#        access byte-reversed; `func be` alone covers every access of the
#        function that isn't through $sp
#   x2 x1 x3   a halfword (x2) or byte (x1, x3: into a halfword, a word) of
#        a wider field: the address XORed, as a little-endian host has it
# In the big-endian build (the default, and the differential test) the
# macros are the identity, so the translation is the same for both.
SITES_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "native_sites.txt")
SITE_KINDS = ("h2", "be", "x1", "x2", "x3")


def load_sites(version="us.v11"):
    """a line with a third field (versions, comma-separated) is only those
    versions' (a version's own IDO asm, at its offsets)"""
    sites, whole = {}, {}
    if not os.path.exists(SITES_FILE):
        return sites, whole
    for n, line in enumerate(open(SITES_FILE), 1):
        line = line.split("#", 1)[0].split()
        if not line:
            continue
        if len(line) not in (2, 3) or line[1] not in SITE_KINDS:
            raise TranslateError(f"{SITES_FILE}:{n}: expected FUNC[+0xOFF] KIND [VERSION,...]")
        if len(line) == 3 and version not in line[2].split(","):
            continue
        where, kind = line[:2]
        if "+" in where:
            fn, off = where.split("+")
            sites[(fn, int(off, 16))] = kind
        else:
            whole[where] = kind
    return sites, whole


NATIVE_SITES, NATIVE_FUNCS = load_sites()
USED_SITES = set()

# The engine's replacement (docs/PORT.md, "Replacing the engine"): functions
# written as native C in port/engine.  The translated code calls them as it
# calls the game's C (recomp_extern_X, the port's glue), and their
# translation is kept as recomp_orig_X under RECOMP_ORIG, for the port's
# check build (PORT_ENGINE_CHECK) and the differential test.
REPLACED_FILE = os.path.join(os.path.dirname(BLAST), "port", "engine", "replaced.txt")


def load_replaced():
    out = []
    if not os.path.exists(REPLACED_FILE):
        return out
    for n, line in enumerate(open(REPLACED_FILE), 1):
        line = line.split("#", 1)[0].split()
        if not line:
            continue
        if len(line) != 1 or not re.match(r"^func_[0-9A-F]{8}(_\w+)?$", line[0]):
            raise TranslateError(f"{REPLACED_FILE}:{n}: expected one function name")
        out.append(line[0])
    return out


REPLACED = set()


class FuncEmitter:
    def __init__(self, group, funcs_by_name, externs, cov, blocks):
        self.blocks = blocks            # (id, function, [ops]) per block
        self.group = group              # list of Function, contiguous
        self.base = group[0]
        self.funcs = funcs_by_name
        self.externs = externs
        self.cov = cov                  # [next block id]
        self.out = []
        # IDO code that sets the FCSR (its float -> unsigned conversions):
        # cvt.w follows its rounding mode and sets its V/I bits
        self.fcsr = any(ln.insn.op == "ctc1" for f in group for ln in f.lines)

    # --- helpers ---------------------------------------------------------
    def pc(self, ln):
        return f"pc_base + 0x{ln.vram - self.base.vram:X}u"

    def label_of(self, fn, name):
        return name.lstrip(".")        # .L8029A8E0 -> L8029A8E0

    def imm_lo(self, ln):
        """The 16-bit immediate as a sign-extended uint64 expression."""
        if ln.lo is not None:
            return f"LO16({sym(ln.lo)})"
        return f"(uint64_t){ln.insn.imm}ll" if ln.insn.imm < 0 else f"0x{ln.insn.imm:X}u"

    def ea(self, ln):
        i = ln.insn
        off = self.imm_lo(ln)
        base = R(i.rs)
        if ln.lo is None and i.imm == 0:
            return f"(uint64_t){base}"
        return f"((uint64_t){base} + {off})"

    def site_kind(self, ln):
        """native_sites.txt's kind for a load or store, or None"""
        i = ln.insn
        if i.op not in ("lb", "lbu", "lh", "lhu", "lw", "lwu", "sb", "sh", "sw", "lwc1", "swc1"):
            return None
        fn = next(f for f in self.group if f.vram <= ln.vram < f.end())
        key = (fn.name, ln.vram - fn.vram)
        if key in NATIVE_SITES:
            USED_SITES.add(key)
            return NATIVE_SITES[key]
        if fn.name in NATIVE_FUNCS and GPR_NAMES[i.rs] != "sp":
            # a whole function's kind applies to the accesses it fits:
            # halves for x2, bytes for x1/x3, words for h2, 2 and 4 for be
            kind = NATIVE_FUNCS[fn.name]
            width = {"lb": 1, "lbu": 1, "sb": 1, "lh": 2, "lhu": 2, "sh": 2}.get(i.op, 4)
            fits = {"x2": (2,), "x1": (1,), "x3": (1,), "h2": (4,), "be": (2, 4)}[kind]
            if width not in fits or i.op in ("lwc1", "swc1"):
                return None
            USED_SITES.add((fn.name, None))
            return kind
        if "/" in fn.obj.name and GPR_NAMES[i.rs] == "sp" and i.op in ("lb", "lbu", "sb", "lh", "lhu", "sh") \
                and ln.lo is None and (i.imm & ~3) in self.stack_words(fn):
            # IDO code (a version's GLOBAL_ASM, config.py) reads a narrow
            # argument out of its word slot: the words the glue or the
            # caller stores, its own `sw` of a0-a3.  A byte or half of a
            # host-order word is XORed.  (Narrow locals that are only ever
            # narrow, which may be a buffer passed to C, are left alone.)
            return "x3" if i.op in ("lb", "lbu", "sb") else "x2"
        return None

    def stack_words(self, fn):
        """The $sp offsets of fn's argument words (the caller's frame) and of
        the words it reads or writes whole (lw/sw)"""
        if not hasattr(fn, "_stack_words"):
            frame = 0
            for ln in fn.lines[:8]:
                i = ln.insn
                if i.op == "addiu" and i.rs == i.rt == 29 and i.imm < 0:
                    frame = -i.imm
                    break
            words = set()
            for ln in fn.lines:
                i = ln.insn
                if i.op in ("lw", "sw") and i.rs == 29 and ln.lo is None:
                    words.add(i.imm & ~3)
            fn._stack_words = words | set(range(frame, frame + 0x100, 4))
        return fn._stack_words

    def native_site(self, ln, kind, ea, rt, pc):
        i = ln.insn
        op = i.op
        width = {"lb": 1, "lbu": 1, "sb": 1, "lh": 2, "lhu": 2, "sh": 2}.get(op, 4)
        store = op in ("sb", "sh", "sw", "swc1")
        if kind in ("x1", "x2", "x3"):
            if (kind == "x2") != (width == 2) or width == 4:
                raise TranslateError(f"{kind} on {op} at {ln.vram:08X}")
            k = kind[1]
        elif kind == "h2" and width != 4 or kind == "be" and width == 1:
            if kind == "be" and width == 1:
                return self.insn_plain(ln)
            raise TranslateError(f"{kind} on {op} at {ln.vram:08X}")
        conv = {"h2": "NE_ROT16", "be": "NE_BS32" if width == 4 else "NE_BS16"}.get(kind)
        a = f"recomp_ea{'_w' if store else ''}(ctx, {ea}, {width}, {pc})"
        if kind[0] == "x":
            a = f"NE_XOR({a}, {k})"
        site = f"RECOMP_SITE({'NE_' + kind.upper()});"
        if store:
            val = {"sb": f"(uint8_t){rt}", "sh": f"(uint16_t){rt}", "sw": f"(uint32_t){rt}",
                   "swc1": f"ctx->f[{i.ft}]"}[op]
            if conv:
                val = f"{conv}({val})"
            fn = {1: "mem_w8", 2: "mem_w16", 4: "mem_w32"}[width]
            return [f"{{ uint32_t a = {a}; {site} {fn}(rdram, a, {val}); }}"]
        fn = {1: "mem_r8", 2: "mem_r16", 4: "mem_r32"}[width]
        v = f"{fn}(rdram, a)"
        if conv:
            v = f"{conv}({v})"
        expr = {"lb": f"S32((int8_t){v})", "lbu": f"(uint64_t){v}", "lh": f"S32((int16_t){v})",
                "lhu": f"(uint64_t)(uint16_t){v}", "lw": f"S32({v})", "lwu": f"(uint64_t)(uint32_t){v}"}.get(op)
        if op == "lwc1":
            return [f"{{ uint32_t a = {a}; {site} ctx->f[{i.ft}] = {v}; }}"]
        return [f"{{ uint32_t a = {a}; {site} {W(i.rt, expr)} }}"]

    def insn_plain(self, ln):
        global NATIVE_FUNCS
        saved = NATIVE_FUNCS
        NATIVE_FUNCS = {}
        try:
            return self.insn(ln)
        finally:
            NATIVE_FUNCS = saved

    # --- one instruction (not a branch/jump) -------------------------------
    def insn(self, ln):
        i = ln.insn
        op = i.op
        rs, rt, rd = R(i.rs), R(i.rt), R(i.rd)
        pc = self.pc(ln)
        if op == "sll" and i.word == 0:
            return ["/* nop */"]
        # ALU, register
        simple3 = {
            "addu": f"S32(U32({rs}) + U32({rt}))",
            "subu": f"S32(U32({rs}) - U32({rt}))",
            "daddu": f"{rs} + {rt}",
            "dsubu": f"{rs} - {rt}",
            "and": f"{rs} & {rt}", "or": f"{rs} | {rt}", "xor": f"{rs} ^ {rt}",
            "nor": f"~({rs} | {rt})",
            "slt": f"(int64_t){rs} < (int64_t){rt}",
            "sltu": f"(uint64_t){rs} < (uint64_t){rt}",
            "add": f"recomp_add32(ctx, {rs}, {rt}, {pc})",
            "sub": f"recomp_sub32(ctx, {rs}, {rt}, {pc})",
            "dadd": f"recomp_add64(ctx, {rs}, {rt}, {pc})",
            "dsub": f"recomp_sub64(ctx, {rs}, {rt}, {pc})",
            "sllv": f"S32(U32({rt}) << ({rs} & 31))",
            "srlv": f"S32(U32({rt}) >> ({rs} & 31))",
            "srav": f"recomp_sra({rt}, {rs} & 31)",
            "dsllv": f"(uint64_t){rt} << ({rs} & 63)",
            "dsrlv": f"(uint64_t){rt} >> ({rs} & 63)",
            "dsrav": f"(uint64_t)((int64_t){rt} >> ({rs} & 63))",
        }
        if op in simple3:
            return [W(i.rd, simple3[op])]
        sa = i.sa
        shifts = {
            "sll": f"S32(U32({rt}) << {sa})",
            "srl": f"S32(U32({rt}) >> {sa})",
            "sra": f"recomp_sra({rt}, {sa})",
            "dsll": f"(uint64_t){rt} << {sa}",
            "dsrl": f"(uint64_t){rt} >> {sa}",
            "dsra": f"(uint64_t)((int64_t){rt} >> {sa})",
            "dsll32": f"(uint64_t){rt} << {sa + 32}",
            "dsrl32": f"(uint64_t){rt} >> {sa + 32}",
            "dsra32": f"(uint64_t)((int64_t){rt} >> {sa + 32})",
        }
        if op in shifts:
            return [W(i.rd, shifts[op])]
        # ALU, immediate
        imm = self.imm_lo(ln)
        if op == "lui":
            if ln.hi is not None:
                return [W(i.rt, f"S32(HI16({sym(ln.hi)}))")]
            return [W(i.rt, f"S32(0x{i.uimm << 16:X}u)")]
        immops = {
            "addiu": f"S32(U32({rs}) + U32({imm}))",
            "daddiu": f"{rs} + {imm}",
            "addi": f"recomp_add32(ctx, {rs}, {imm}, {pc})",
            "daddi": f"recomp_add64(ctx, {rs}, {imm}, {pc})",
            "slti": f"(int64_t){rs} < (int64_t){imm}",
            "sltiu": f"(uint64_t){rs} < (uint64_t){imm}",
            "andi": f"{rs} & 0x{i.uimm:X}u",
            "ori": f"{rs} | 0x{i.uimm:X}u",
            "xori": f"{rs} ^ 0x{i.uimm:X}u",
        }
        if op in immops:
            if ln.lo is not None and op in ("andi", "ori", "xori"):
                raise TranslateError(f"%lo on {op} at {ln.vram:08X}")
            return [W(i.rt, immops[op])]
        # HI/LO
        if op in ("mult", "multu", "div", "divu", "dmult", "dmultu", "ddiv", "ddivu"):
            return [f"recomp_{op}(ctx, {rs}, {rt});"]
        if op == "mfhi":
            return [W(i.rd, "ctx->hi")]
        if op == "mflo":
            return [W(i.rd, "ctx->lo")]
        if op == "mthi":
            return [f"ctx->hi = {rs};"]
        if op == "mtlo":
            return [f"ctx->lo = {rs};"]
        # loads / stores
        ea = self.ea(ln)
        kind = self.site_kind(ln)
        if kind:
            return self.native_site(ln, kind, ea, rt, pc)
        loads = {
            "lb": (1, "S32((int8_t)mem_r8(rdram, a))"),
            "lbu": (1, "(uint64_t)mem_r8(rdram, a)"),
            "lh": (2, "S32((int16_t)mem_r16(rdram, a))"),
            "lhu": (2, "(uint64_t)mem_r16(rdram, a)"),
            "lw": (4, "S32(mem_r32(rdram, a))"),
            "lwu": (4, "(uint64_t)mem_r32(rdram, a)"),
            "ld": (8, "mem_r64(rdram, a)"),
            "lwl": (1, f"mem_lwl(rdram, a, {rt})"),
            "lwr": (1, f"mem_lwr(rdram, a, {rt})"),
        }
        if op in loads:
            al, expr = loads[op]
            return [f"{{ uint32_t a = recomp_ea(ctx, {ea}, {al}, {pc}); {W(i.rt, expr)} }}"]
        stores = {
            "sb": (1, f"mem_w8(rdram, a, (uint8_t){rt})"),
            "sh": (2, f"mem_w16(rdram, a, (uint16_t){rt})"),
            "sw": (4, f"mem_w32(rdram, a, (uint32_t){rt})"),
            "sd": (8, f"mem_w64(rdram, a, {rt})"),
            "swl": (1, f"mem_swl(rdram, a, {rt})"),
            "swr": (1, f"mem_swr(rdram, a, {rt})"),
        }
        if op in stores:
            al, expr = stores[op]
            return [f"{{ uint32_t a = recomp_ea_w(ctx, {ea}, {al}, {pc}); {expr}; }}"]
        ft, fs, fd = i.ft, i.fs, i.fd
        if op == "lwc1":
            return [f"{{ uint32_t a = recomp_ea(ctx, {ea}, 4, {pc}); ctx->f[{ft}] = mem_r32(rdram, a); }}"]
        if op == "swc1":
            return [f"{{ uint32_t a = recomp_ea_w(ctx, {ea}, 4, {pc}); mem_w32(rdram, a, ctx->f[{ft}]); }}"]
        if op == "ldc1":
            self.even(ln, ft)
            return [f"{{ uint32_t a = recomp_ea(ctx, {ea}, 8, {pc}); set_fpr_l(ctx, {ft}, mem_r64(rdram, a)); }}"]
        if op == "sdc1":
            self.even(ln, ft)
            return [f"{{ uint32_t a = recomp_ea_w(ctx, {ea}, 8, {pc}); mem_w64(rdram, a, fpr_l(ctx, {ft})); }}"]
        if op == "cache":
            return ["/* cache: no-op */"]
        if op == "sync":
            return ["/* sync */"]
        # traps
        if op == "break":
            return [f"recomp_trap(ctx, RECOMP_TRAP_BREAK, {pc}, {i.code});"]
        if op == "syscall":
            return [f"recomp_trap(ctx, RECOMP_TRAP_SYSCALL, {pc}, {i.code});"]
        # COP0
        if op == "mfc0":
            return [W(i.rt, f"S32(recomp_mfc0(ctx, {i.rd}))")]
        if op == "mtc0":
            return [f"recomp_mtc0(ctx, {i.rd}, {rt});"]
        # COP1 moves
        if op == "mtc1":
            return [f"ctx->f[{fs}] = U32({rt});"]
        if op == "mfc1":
            return [W(i.rt, f"S32(ctx->f[{fs}])")]
        if op == "dmtc1":
            self.even(ln, fs)
            return [f"set_fpr_l(ctx, {fs}, {rt});"]
        if op == "dmfc1":
            self.even(ln, fs)
            return [W(i.rt, f"fpr_l(ctx, {fs})")]
        if op == "cfc1":
            if i.rd != 31:
                raise TranslateError(f"cfc1 ${i.rd} at {ln.vram:08X}")
            return [W(i.rt, "S32(ctx->fcr31)")]
        if op == "ctc1":
            if i.rd != 31:
                raise TranslateError(f"ctc1 ${i.rd} at {ln.vram:08X}")
            return [f"ctx->fcr31 = U32({rt});"]
        if op.startswith("c."):
            return self.fcmp(ln)
        if "." in op:
            return self.fpu(ln)
        raise TranslateError(f"unsupported {op} at {ln.vram:08X}")

    def even(self, ln, n):
        if n & 1:
            raise TranslateError(f"odd FPR pair ${n} at {ln.vram:08X} (FR=0)")

    def fpu(self, ln):
        i = ln.insn
        base, fmt = i.op.rsplit(".", 1)
        fs, ft, fd = i.fs, i.ft, i.fd
        if fmt in ("d", "l"):
            self.even(ln, fs)
        get = {"s": lambda n: f"fpr_s(ctx, {n})", "d": lambda n: f"fpr_d(ctx, {n})",
               "w": lambda n: f"(int32_t)ctx->f[{n}]",
               "l": lambda n: f"(int64_t)fpr_l(ctx, {n})"}[fmt]
        a, b = get(fs), get(ft)
        raw = (lambda n: f"ctx->f[{n}]") if fmt == "s" else (lambda n: f"fpr_l(ctx, {n})")
        put = (lambda v: f"ctx->f[{fd}] = {v};") if fmt == "s" else (lambda v: f"set_fpr_l(ctx, {fd}, {v});")
        if base in ("add", "sub", "mul", "div"):
            if fmt not in ("s", "d"):
                raise TranslateError(f"{i.op} at {ln.vram:08X}")
            if fmt == "d":
                self.even(ln, ft); self.even(ln, fd)
            return [put(f"recomp_{base}_{fmt}({raw(fs)}, {raw(ft)})")]
        if base == "sqrt":
            if fmt == "d":
                self.even(ln, fd)
            return [put(f"recomp_sqrt_{fmt}({raw(fs)})")]
        # abs/neg only touch the sign bit, NaN or not
        if base in ("abs", "neg"):
            opr = "& ~" if base == "abs" else "^ "
            if fmt == "s":
                return [f"ctx->f[{fd}] = ctx->f[{fs}] {opr}0x80000000u;"]
            self.even(ln, fd)
            return [f"set_fpr_l(ctx, {fd}, fpr_l(ctx, {fs}) {opr}0x8000000000000000ull);"]
        if base == "mov":
            if fmt == "s":
                return [f"ctx->f[{fd}] = ctx->f[{fs}];"]
            self.even(ln, fd)
            return [f"set_fpr_l(ctx, {fd}, fpr_l(ctx, {fs}));"]
        if base in ("cvt.s", "cvt.d"):
            to = base[-1]
            if to == fmt:
                raise TranslateError(f"{i.op} at {ln.vram:08X}")
            if to == "d":
                self.even(ln, fd)
            if fmt == "s":
                return [f"set_fpr_l(ctx, {fd}, recomp_cvt_d_s(ctx->f[{fs}]));"]
            if fmt == "d":
                return [f"ctx->f[{fd}] = recomp_cvt_s_d(fpr_l(ctx, {fs}));"]
            cast = "(float)" if to == "s" else "(double)"
            return [f"set_fpr_{to}(ctx, {fd}, {cast}{a});"]
        if base in ("cvt.w", "cvt.l", "round.w", "round.l", "trunc.w",
                    "trunc.l", "ceil.w", "ceil.l", "floor.w", "floor.l"):
            kind, to = base.split(".")
            if fmt not in ("s", "d"):
                raise TranslateError(f"{i.op} at {ln.vram:08X}")
            f = "f" if fmt == "s" else ""
            # cvt uses the FCSR rounding mode; the game leaves it at
            # round-to-nearest-even (recomp_rint, recomp.h).
            rnd = {"cvt": "recomp_rint", "round": "recomp_rint", "trunc": "trunc",
                   "ceil": "ceil", "floor": "floor"}[kind] + f
            if to == "w" and kind == "cvt" and self.fcsr:
                return [f"ctx->f[{fd}] = recomp_cvt_w_fcsr(ctx, {a});"]
            if to == "w":
                return [f"ctx->f[{fd}] = recomp_f2w({rnd}({a}));"]
            self.even(ln, fd)
            conv = "recomp_f2l_s" if fmt == "s" else "recomp_f2l"
            return [f"set_fpr_l(ctx, {fd}, {conv}({rnd}({a})));"]
        raise TranslateError(f"unsupported {i.op} at {ln.vram:08X}")

    def fcmp(self, ln):
        i = ln.insn
        fmt = i.fmt
        if fmt == "d":
            self.even(ln, i.fs); self.even(ln, i.ft)
        get = (lambda n: f"fpr_s(ctx, {n})") if fmt == "s" else (lambda n: f"fpr_d(ctx, {n})")
        return [f"SET_FCC(recomp_fcmp({get(i.fs)}, {get(i.ft)}, {i.cond & 7}));"]

    # --- control flow ------------------------------------------------------
    def cond(self, ln):
        i = ln.insn
        rs, rt = R(i.rs), R(i.rt)
        base = {"beql": "beq", "bnel": "bne", "blezl": "blez", "bgtzl": "bgtz",
                "bltzl": "bltz", "bgezl": "bgez", "bc1fl": "bc1f",
                "bc1tl": "bc1t"}.get(i.op, i.op)
        if base == "beq":
            if i.rs == i.rt:
                return "1"
            return f"{rs} == {rt}"
        if base == "bne":
            return f"{rs} != {rt}"
        if base == "blez":
            return f"(int64_t){rs} <= 0"
        if base == "bgtz":
            return f"(int64_t){rs} > 0"
        if base == "bltz":
            return f"(int64_t){rs} < 0"
        if base == "bgez":
            return f"(int64_t){rs} >= 0"
        if base == "bc1t":
            return "FCC"
        if base == "bc1f":
            return "!FCC"
        raise TranslateError(f"unsupported branch {i.op} at {ln.vram:08X}")

    def call(self, name):
        if name in self.funcs and name not in REPLACED:
            return f"recomp_{name}(rdram, ctx);"
        self.externs.add(name)
        return f"recomp_extern_{name}(rdram, ctx);"

    def target_label(self, fn, ln):
        t = ln.target
        for f in self.group:
            if t in f.labels:
                return self.label_of(f, t)
            if t == f.name:
                return f"F_{f.name}"
        raise TranslateError(f"{fn.name}: branch at {ln.vram:08X} to {t} not in group")

    def body(self):
        out = self.out
        for fn in self.group:
            lines = fn.lines
            labels_at = {}
            for name, idx in fn.labels.items():
                labels_at.setdefault(idx, []).append(name)
            leaders = {0} | set(labels_at)
            for i, ln in enumerate(lines):
                if ln.insn.kind in ("branch", "jump") and i + 2 <= len(lines):
                    leaders.add(i + 2)
            leaders = sorted(x for x in leaders if x < len(lines))
            out.append(f"F_{fn.name}: ;")
            i = 0
            n = len(lines)
            while i < n:
                ln = lines[i]
                if i in labels_at:
                    for name in sorted(labels_at[i]):
                        out.append(f"{self.label_of(fn, name)}: ;")
                if i in leaders:
                    j = leaders.index(i)
                    nxt = leaders[j + 1] if j + 1 < len(leaders) else n
                    out.append(f"    BB({self.cov[0]}, {nxt - i});")
                    self.blocks.append((self.cov[0], fn.name, [l.insn.op for l in lines[i:nxt]],
                                        ln.vram, nxt - i))
                    self.cov[0] += 1
                out.append(f"    /* {ln.vram:08X}: {ln.text_op} {ln.operands} */")
                out.append(f"    TR({self.pc(ln)});")
                ins = ln.insn
                if ins.kind in ("branch", "jump"):
                    if i + 1 >= n:
                        raise TranslateError(f"{fn.name}: control transfer without delay slot")
                    dl = lines[i + 1]
                    if dl.insn.kind in ("branch", "jump"):
                        raise TranslateError(f"{fn.name}: branch in delay slot at {dl.vram:08X}")
                    delay = self.insn(dl)
                    dcomment = f"    /* {dl.vram:08X}: {dl.text_op} {dl.operands} (delay) */ TR({self.pc(dl)});"
                    stmts = self.control(fn, ln, dl, delay, dcomment)
                    out.extend("    " + s for s in stmts)
                    if (i + 1) in labels_at:
                        # something branches to the delay slot itself
                        if not is_terminator(ln):
                            out.append(f"    goto L_seq_{lines[i + 2].vram:08X};" if i + 2 < n else "    /* end */")
                        for name in sorted(labels_at[i + 1]):
                            out.append(f"{self.label_of(fn, name)}: ;")
                        out.append(dcomment)
                        out.extend("    " + s for s in delay)
                        if i + 2 < n:
                            out.append(f"L_seq_{lines[i + 2].vram:08X}: ;")
                    i += 2
                    continue
                out.extend("    " + s for s in self.insn(ln))
                i += 1
            last = lines[-1]
            prev = lines[-2] if len(lines) > 1 else None
            if not (prev is not None and last.delay and is_terminator(prev)):
                nxt = self.group.index(fn) + 1
                if nxt >= len(self.group):
                    raise TranslateError(f"{fn.name}: falls off the end of its group")
                out.append(f"    /* falls through into {self.group[nxt].name} */")
        return out

    def control(self, fn, ln, dl, delay, dcomment):
        i = ln.insn
        pc = self.pc(ln)
        if i.op == "jal":
            ret = f"pc_base + 0x{ln.vram + 8 - self.base.vram:X}u"
            return [f"ctx->ra = S32({ret});", dcomment.strip()] + delay + [self.call(ln.target)]
        if i.op == "j":
            return [dcomment.strip()] + delay + [self.call(ln.target), "return;"]
        if i.op == "jr" and i.rs != 31:
            return self.jump_table(fn, ln, dl, delay, dcomment)
        if i.op == "jr":
            return [f"{{ uint64_t target = ctx->ra;", dcomment.strip()] + delay + \
                   [f"ctx->ra = target; CHECK_RA({pc}); return; }}"] \
                if self.writes_ra(dl) else \
                   [dcomment.strip()] + delay + [f"CHECK_RA({pc}); return;"]
        if i.op == "jalr":
            raise TranslateError(f"jalr at {ln.vram:08X}")
        if i.link:
            raise TranslateError(f"{i.op} at {ln.vram:08X}")
        target = self.target_label(fn, ln)
        cond = self.cond(ln)
        if cond == "1":
            return [dcomment.strip()] + delay + [f"goto {target};"]
        if i.likely:
            return [f"if ({cond}) {{", dcomment.strip()] + delay + [f"goto {target}; }}"]
        return [f"{{ int c = {cond};", dcomment.strip()] + delay + [f"if (c) goto {target}; }}"]

    def jump_table(self, fn, ln, dl, delay, dcomment):
        """`jr $reg` in IDO code: a switch through a jump table in .rodata,
        whose words are addresses of labels in this function (the .s file's
        `.word L<vram>_<rom>` list).  The register is compared with each of
        them, as the address it would be in the linked image."""
        i = ln.insn
        pc = self.pc(ln)
        targets = []
        for f in self.group:
            for name in sorted(f.labels, key=lambda n: f.labels[n]):
                if name in f.obj.jump_labels:
                    targets.append((f.lines[f.labels[name]].vram, self.label_of(f, name)))
        if not targets:
            raise TranslateError(f"jr ${GPR_NAMES[i.rs]} at {ln.vram:08X} with no jump table")
        out = [f"{{ uint32_t target = U32({R(i.rs)});", dcomment.strip()] + delay
        for vram, lab in targets:
            out.append(f"if (target == pc_base + 0x{vram - self.base.vram:X}u) goto {lab};")
        out.append(f"recomp_trap(ctx, RECOMP_TRAP_JUMP, {pc}, target); }}")
        return out

    def writes_ra(self, ln):
        """Whether a jr $ra's delay slot overwrites $ra (the jump uses the
        old value)."""
        i = ln.insn
        return (i.kind in ("load", "alu") and i.rt == 31) or (i.kind == "alu" and i.rd == 31)

    def emit(self):
        hdr = []
        body = self.body()
        # a replaced function's translation: recomp_orig_X, for the checks
        orig = self.base.name in REPLACED
        pre = "recomp_orig_" if orig else "recomp_"
        if len(self.group) == 1:
            fn = self.base
            hdr.append(f"void {pre}{fn.name}(uint8_t *rdram, recomp_context *ctx) {{")
            hdr.append(f"    const uint32_t pc_base = {sym(fn.name)}; (void)pc_base;")
            hdr.append("    ENTRY_RA;")
            tail = ["}", ""]
        else:
            gname = f"{pre}group_{self.base.name}"
            hdr.append(f"/* {', '.join(f.name for f in self.group)} share code "
                       "(a branch or fall-through crosses between them) */")
            hdr.append(f"static void {gname}(uint8_t *rdram, recomp_context *ctx, int entry) {{")
            hdr.append(f"    const uint32_t pc_base = {sym(self.base.name)}; (void)pc_base;")
            hdr.append("    ENTRY_RA;")
            hdr.append("    switch (entry) {")
            for k, f in enumerate(self.group):
                hdr.append(f"    case {k}: goto F_{f.name};")
            hdr.append("    }")
            tail = ["}", ""]
            for k, f in enumerate(self.group):
                tail.append(f"void {pre}{f.name}(uint8_t *rdram, recomp_context *ctx) {{ {gname}(rdram, ctx, {k}); }}")
            tail.append("")
        if orig:
            return ["#ifdef RECOMP_ORIG"] + hdr + body + tail + ["#endif", ""]
        return hdr + body + tail


def make_groups(obj):
    """Contiguous runs of functions that must share one C body."""
    fns = obj.functions
    index = {fn.name: k for k, fn in enumerate(fns)}
    label_owner = {}
    for k, fn in enumerate(fns):
        for lab in fn.labels:
            label_owner[lab] = k
    spans = []
    for k, fn in enumerate(fns):
        for ln in fn.lines:
            if ln.insn.kind == "branch" and ln.target not in fn.labels:
                other = label_owner.get(ln.target, index.get(ln.target))
                if other is None:
                    raise TranslateError(f"{fn.name}: branch target {ln.target} unknown")
                spans.append((min(k, other), max(k, other)))
        last = fn.lines[-1]
        prev = fn.lines[-2] if len(fn.lines) > 1 else None
        if not (prev is not None and last.delay and is_terminator(prev)):
            spans.append((k, k + 1))
    groups = []
    k = 0
    while k < len(fns):
        end = k
        changed = True
        while changed:
            changed = False
            for a, b in spans:
                if a <= end and b > end and a >= k:
                    end = b
                    changed = True
        groups.append(fns[k:end + 1])
        k = end + 1
    return groups


def elf_symbols(path):
    # MIPS binutils' nm, or LLVM's where there are none (macOS)
    nm = os.environ.get("PORT_N64_NM") or shutil.which("mips-linux-gnu-nm") or "llvm-nm"
    out = subprocess.run([nm, path], capture_output=True,
                         text=True, check=True).stdout
    syms = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) != 3:
            continue
        val, typ, name = parts
        if name.startswith(".L") or name.startswith("$"):
            continue
        syms.setdefault(name, int(val, 16))
    return syms


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--version", default="us.v11")
    args = ap.parse_args()
    os.makedirs(args.outdir, exist_ok=True)
    global NATIVE_SITES, NATIVE_FUNCS
    NATIVE_SITES, NATIVE_FUNCS = load_sites(args.version)

    objs = [parse_file(path, module, name)
            for module, name, path in translated_objects(args.version)]
    funcs = {}
    for o in objs:
        for fn in o.functions:
            if fn.name in funcs:
                raise TranslateError(f"duplicate function {fn.name}")
            funcs[fn.name] = fn
    # (a name another version doesn't have is that version's business)
    REPLACED.update(n for n in load_replaced() if n in funcs)
    for o in objs:
        for group in make_groups(o):
            names = {f.name for f in group}
            if names & REPLACED and not names <= REPLACED:
                raise TranslateError(f"{REPLACED_FILE}: {', '.join(sorted(names))} share code "
                                     f"(a branch or fall-through between them): replace all or none")

    cov = [0]
    blocks = []
    externs = set()
    used_syms = {}
    written = []
    for o in objs:
        em_out = [
            f"/* Generated by tools/recomp/translate.py from {os.path.relpath(o.path, BLAST)}. */",
            "#include \"recomp.h\"",
            "#include \"recomp_funcs.h\"",
            "#include \"recomp_externs.h\"",
            f"#include \"syms_{o.module}.h\"",
            "",
        ]
        for group in make_groups(o):
            em = FuncEmitter(group, funcs, externs, cov, blocks)
            em_out.extend(em.emit())
        text = "\n".join(em_out) + "\n"
        fname = f"{o.module}_{o.name.replace('/', '_')}.c"
        written.append(fname)
        with open(os.path.join(args.outdir, fname), "w") as f:
            f.write(text)
        us = used_syms.setdefault(o.module, set())
        us.update(re.findall(r"SYM\(([A-Za-z0-9_]+)\)", text))

    unused = [k for k in NATIVE_SITES if k not in USED_SITES] + \
        [k for k in NATIVE_FUNCS if (k, None) not in USED_SITES]
    if unused:
        raise TranslateError(f"native_sites.txt: no such load or store: {unused}")

    allf = sorted(funcs.values(), key=lambda fn: (fn.obj.module, fn.vram))
    with open(os.path.join(args.outdir, "recomp_funcs.h"), "w") as f:
        f.write("/* Generated by tools/recomp/translate.py. */\n#pragma once\n#include \"recomp.h\"\n")
        for fn in allf:
            if fn.name in REPLACED:
                continue
            f.write(f"void recomp_{fn.name}(uint8_t *rdram, recomp_context *ctx);\n")
        f.write("/* the replaced functions' translations (port/engine/replaced.txt) */\n"
                "#ifdef RECOMP_ORIG\n")
        for fn in allf:
            if fn.name in REPLACED:
                f.write(f"void recomp_orig_{fn.name}(uint8_t *rdram, recomp_context *ctx);\n")
        f.write("#endif\n")
        f.write(f"#define RECOMP_NUM_FUNCS {len(allf)}\n#define RECOMP_NUM_BLOCKS {cov[0]}\n")
    ext = sorted(externs)
    with open(os.path.join(args.outdir, "recomp_externs.h"), "w") as f:
        f.write("/* Generated by tools/recomp/translate.py: calls out of the translated code. */\n")
        f.write("#pragma once\n#include \"recomp.h\"\n")
        for n in ext:
            f.write(f"void recomp_extern_{n}(uint8_t *rdram, recomp_context *ctx);\n")
    # Which module's symbol table resolves each extern: the first module
    # that calls it.
    ext_module = {}
    for o in objs:
        for fn in o.functions:
            for ln in fn.lines:
                if ln.insn.op in ("jal", "j") and ln.target in externs:
                    ext_module.setdefault(ln.target, o.module)
    with open(os.path.join(args.outdir, "externs.c"), "w") as f:
        f.write("/* Generated by tools/recomp/translate.py.  Default glue for calls out of\n"
                "   the translated code: every callee goes to recomp_call_external().  The\n"
                "   port replaces these with calls to the native functions. */\n")
        f.write("#include \"recomp.h\"\n#include \"recomp_externs.h\"\n")
        for module in sorted(set(ext_module.values())):
            f.write(f"#undef SYM\n#include \"syms_{module}.h\"\n")
        for n in ext:
            f.write(f"void recomp_extern_{n}(uint8_t *rdram, recomp_context *ctx) "
                    f"{{ recomp_call_external(rdram, ctx, SYM({n})); }}\n")
    for n, m in ext_module.items():
        used_syms[m].add(n)
    with open(os.path.join(args.outdir, "funcs.c"), "w") as f:
        f.write("/* Generated by tools/recomp/translate.py. */\n")
        f.write("#include <stddef.h>\n#include \"recomp.h\"\n#include \"recomp_funcs.h\"\n")
        mods = sorted({fn.obj.module for fn in allf})
        for m in mods:
            f.write(f"#include \"syms_{m}.h\"\n")
        f.write("const struct recomp_func_entry { uint32_t addr; recomp_func_t fn; const char *name; } "
                "recomp_func_table[] = {\n")
        for fn in allf:
            if fn.name in REPLACED:
                f.write(f"#ifdef RECOMP_ORIG\n    {{ SYM({fn.name}), recomp_orig_{fn.name}, \"{fn.name}\" }},\n#endif\n")
            else:
                f.write(f"    {{ SYM({fn.name}), recomp_{fn.name}, \"{fn.name}\" }},\n")
        f.write("};\n")
        f.write("const unsigned recomp_num_funcs = sizeof recomp_func_table / sizeof recomp_func_table[0];\n")
        f.write("recomp_func_t recomp_lookup(uint32_t addr) {\n"
                "    for (size_t i = 0; i < recomp_num_funcs; i++)\n"
                "        if (recomp_func_table[i].addr == addr) return recomp_func_table[i].fn;\n"
                "    return NULL;\n}\n")
        f.write("#ifdef RECOMP_TEST\nuint32_t recomp_cov[RECOMP_NUM_BLOCKS];\n"
                "const unsigned recomp_num_blocks = RECOMP_NUM_BLOCKS;\n#endif\n")
    # funcs.c needs every function symbol
    for fn in allf:
        used_syms.setdefault(fn.obj.module, set()).add(fn.name)
    for module, names in sorted(used_syms.items()):
        elf = os.path.join(BLAST, "build", f"{module}.{args.version}.elf")
        syms = elf_symbols(elf)
        missing = sorted(n for n in names if n not in syms)
        if missing:
            raise TranslateError(f"{module}: symbols not in {elf}: {missing[:10]}")
        with open(os.path.join(args.outdir, f"syms_{module}.h"), "w") as f:
            f.write(f"/* Generated from {os.path.relpath(elf, BLAST)}. */\n")
            f.write("#ifndef SYM\n#define SYM(name) SYM_##name\n#endif\n")
            for n in sorted(names):
                f.write(f"#ifndef SYM_{n}\n#define SYM_{n} 0x{syms[n]:08X}u\n#endif\n")
    # block id -> function and opcodes, for difftest.py's coverage report
    with open(os.path.join(args.outdir, "blocks.tsv"), "w") as f:
        for bid, name, ops, vram, n in blocks:
            f.write(f"{bid}\t{name}\t{' '.join(ops)}\n")
    # The same blocks by address, for the engine's replacement: native C
    # charges the original's instructions with ENGINE_BLK(802AC1E4) where the
    # original's block at 0x802AC1E4 would run (port/engine/engine.h)
    with open(os.path.join(args.outdir, "engine_blocks.h"), "w") as f:
        f.write("/* Generated by tools/recomp/translate.py: each translated block's id and\n"
                "   instructions, by the address it starts at. */\n#pragma once\n")
        # (by us.v11's address, which a function's name holds in every
        # version, so that the same C builds them all)
        for bid, name, ops, vram, n in blocks:
            m = re.fullmatch(r"func_([0-9A-F]{8})", name)
            key = int(m.group(1), 16) + vram - funcs[name].vram if m else vram
            f.write(f"#define ENGINE_BLK_{key:08X} {bid}, {n}\n")
    print(f"translated {len(funcs)} functions in {len(objs)} objects, "
          f"{cov[0]} blocks, {len(ext)} externs -> {args.outdir}")


if __name__ == "__main__":
    try:
        main()
    except TranslateError as e:
        sys.exit(f"translate.py: {e}")
