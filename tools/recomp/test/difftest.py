#!/usr/bin/env python3
"""Differential test: translated C against the original code in unicorn.

    tools/recomp/test/difftest.py LIB [--trials N] [--jobs J] [--only NAME ...]

LIB is the RECOMP_TEST build of the translated code plus harness.c
(`make -C tools/recomp test` builds it and runs this).

For each translated function and each trial:

  1. RDRAM (4 MB) gets the linked hd_code, hd_front_end and init images at
     their VRAM, and everything else either a seeded random fill (mostly
     zeros, small integers, floats and pointers back into RDRAM) or, with
     --snapshots, RDRAM from real play (snapshot.c) with the linked .text
     on top.  Registers get the same kind of values, pointers where the
     function dereferences them; $ra a magic return address.
  2. The original function runs in unicorn (MIPS64 big-endian, the R4000
     model: MIPS III, FR=0 as the game runs it) until it returns to the
     magic address, traps, or runs out of instructions.
  3. RDRAM and registers are reset and the translated function runs on the
     same state.  Calls out of the translated code (IDO C, libultra) run
     the original callee in unicorn on the same RDRAM buffer, so both runs
     execute identical code for everything that isn't translated; with
     --stub-externs, callees instead return a hash of their arguments in
     both runs.
  4. GPRs, HI/LO, the FPRs, the FCSR condition bit, every byte of RDRAM and
     the way the run ended (return, break, syscall, overflow, address fault)
     must match.  Runs where either side hit the instruction budget, or
     where the random state overwrote the saved $ra, are inconclusive.

Where unicorn can't stand in for the VR4300 the harness fills in: it
executes the L-format conversions itself (QEMU rejects them with FR=0), it
clears the FCSR flag bits at calls out (the translation doesn't keep them),
it write-protects the code pages in both runs, and it starts a fresh unicorn
after any exception (unicorn keeps a pending delay slot across runs).

Prints one line per function that fails, and a summary.
"""

import argparse
import ctypes
import multiprocessing as mp
import os
import subprocess
import sys
import time

import numpy as np
from unicorn import Uc, UcError, UC_ARCH_MIPS, UC_MODE_MIPS64, UC_MODE_BIG_ENDIAN, \
    UC_PROT_ALL, UC_PROT_READ, UC_PROT_EXEC, UC_HOOK_INTR, UC_HOOK_MEM_UNMAPPED, UC_HOOK_CODE
import unicorn.mips_const as MC

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, ".."))
from mips import decode, DecodeError  # noqa: E402
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")

RDRAM_SIZE = 0x400000
MAGIC_RET = 0x80000100          # where the tested function returns to
BUDGET = 400_000                # instructions per run
EXT_BUDGET = 400_000
STATUS = 0x20000000             # CU1, FR=0, kernel mode, interrupts off

# the version of the stage-2 build (the Makefile's VERSION)
VERSION = os.environ.get("RECOMP_VERSION", "us.v11")


def text_size(module):
    """the size of a module's .text in the linked ELF"""
    out = subprocess.run(["mips-linux-gnu-readelf", "-SW", os.path.join(BLAST, "build", f"{module}.{VERSION}.elf")],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        p = line.replace("[ ", "[").split()
        if len(p) > 5 and p[1] == "." + module:
            return int(p[5], 16)
    raise KeyError(module)


IMAGES = [  # (module, vram, .text size)
    ("hd_code", 0x802447C0, text_size("hd_code")),
    ("hd_front_end", 0x801E7000, text_size("hd_front_end")),
    ("init", 0x8021ED00, 0),
]

# outcome classes
RETURN, BREAK, SYSCALL, OVERFLOW, FAULT, TIMEOUT, OTHER = \
    "return", "break", "syscall", "overflow", "fault", "timeout", "other"
# RECOMP_TRAP_* (recomp.h) -> class
TRAP_CLASS = {0: RETURN, 1: BREAK, 2: SYSCALL, 3: OVERFLOW, 4: FAULT, 5: FAULT,
              6: TIMEOUT, 7: "ra-changed", 8: OTHER, 9: OTHER, 10: OTHER}
# QEMU MIPS exception numbers (target/mips/cpu.h, QEMU 5.0 as in unicorn 2)
EXCP_CLASS = {17: SYSCALL, 18: BREAK, 21: OVERFLOW, 12: FAULT, 13: FAULT,
              14: FAULT, 15: FAULT, 25: FAULT, 26: FAULT, 27: FAULT,
              28: FAULT, 29: FAULT}

GPR_UC = [getattr(MC, "UC_MIPS_REG_" + str(i)) for i in range(32)]
FPR_UC = [getattr(MC, f"UC_MIPS_REG_F{i}") for i in range(32)]


class Ctx(ctypes.Structure):
    _fields_ = [("r", ctypes.c_uint64 * 32), ("hi", ctypes.c_uint64),
                ("lo", ctypes.c_uint64), ("f", ctypes.c_uint32 * 32),
                ("fcr31", ctypes.c_uint32), ("budget", ctypes.c_int64)]


EXTERN_CB = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(Ctx), ctypes.c_uint32)
MFC0_CB = ctypes.CFUNCTYPE(ctypes.c_uint64, ctypes.c_int)


def sx(v):
    """32-bit value -> 64-bit sign-extended, as unicorn wants addresses."""
    v &= 0xFFFFFFFF
    return v | 0xFFFFFFFF00000000 if v & 0x80000000 else v


def elf_symbol(module, name):
    out = subprocess.run(["mips-linux-gnu-nm", os.path.join(BLAST, "build", f"{module}.{VERSION}.elf")],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[2] == name:
            return int(p[0], 16)
    raise KeyError(name)


def load_images():
    img = np.zeros(RDRAM_SIZE, dtype=np.uint8)
    keep = np.zeros(RDRAM_SIZE, dtype=bool)
    for module, vram, _ in IMAGES:
        data = open(os.path.join(BLAST, "build", f"{module}.{VERSION}.bin"), "rb").read()
        off = vram - 0x80000000
        img[off:off + len(data)] = np.frombuffer(data, dtype=np.uint8)
        keep[off:off + len(data)] = True
    return img, keep


# Value mixes, as weights for: zero, small int, small negative, pointer
# (4-aligned), pointer (16-aligned), float, any word.  Trials cycle
# through them; some code only gets anywhere with mostly-pointers, some
# with mostly-small counts.
PROFILES = [
    (30, 15, 5, 15, 10, 15, 10),
    (25, 10, 0, 15, 45, 5, 0),
    (45, 40, 5, 0, 10, 0, 0),
    (20, 10, 5, 10, 15, 40, 0),
]
N_IMAGES = 8
SPECIAL_FLOATS = [0x7FC00000, 0x7F800001, 0xFF85FFC7, 0x7F800000, 0xFF800000,
                  0x00000001, 0x80000000, 0x7FBFFFFF]


def random_words(rng, n, profile=0):
    """Mostly-plausible 32-bit values: zeros, small ints, floats, pointers."""
    w = np.cumsum(PROFILES[profile % len(PROFILES)])
    kind = rng.integers(0, w[-1], n)
    small = rng.integers(0, 256 if profile != 2 else 16, n, dtype=np.uint32)
    neg = (-rng.integers(1, 64, n)).astype(np.int64).astype(np.uint32)
    ptr = (0x80000000 + (rng.integers(0x200000, RDRAM_SIZE - 0x100, n) & ~3)).astype(np.uint32)
    ptr16 = (ptr & ~np.uint32(15)).astype(np.uint32)
    flt = rng.uniform(-512.0, 512.0, n).astype(np.float32).view(np.uint32)
    anyw = rng.integers(0, 1 << 32, n, dtype=np.uint64).astype(np.uint32)
    choices = [np.zeros(n, dtype=np.uint32), small, neg, ptr, ptr16, flt, anyw]
    out = np.select([kind < x for x in w], choices)
    return out.astype(np.uint32)


def make_memory(seed, img, keep, ptr_globals=()):
    rng = np.random.default_rng(seed)
    words = random_words(rng, RDRAM_SIZE // 4, seed)
    for a in ptr_globals:
        if rng.integers(0, 10) < 9:
            words[(a - 0x80000000) // 4] = 0x80000000 + (int(rng.integers(0x200000, RDRAM_SIZE - 0x1000)) & ~15)
    words = words.astype(">u4").view(np.uint8)
    mem = np.where(keep, img, words)
    return mem.astype(np.uint8)


def random_state(rng, gp_value, profile=0, ptr_regs=(), pool=None):
    """(gprs[32] as u64, hi, lo, fpr[32] u32, fcr31).  Registers in
    `ptr_regs` (the function dereferences them on entry) mostly get
    pointers: with a snapshot, ones that some word in it holds."""
    g = random_words(rng, 32, profile).astype(np.uint64)
    p = random_words(rng, 32, 1).astype(np.uint64)
    regs = [sx(int(v)) for v in g]
    for k in range(1, 32):
        if rng.integers(0, 10) == 0:
            regs[k] = int(rng.integers(0, 1 << 63)) * 2 + int(rng.integers(0, 2))
    for k in ptr_regs or ():
        if rng.integers(0, 10) < 8:
            if pool is not None and len(pool):
                regs[k] = sx(int(pool[rng.integers(0, len(pool))]))
            else:
                regs[k] = sx(0x80000000 + (int(rng.integers(0x200000, RDRAM_SIZE - 0x1000)) & ~15))
    regs[0] = 0
    regs[28] = sx(gp_value) if rng.integers(0, 2) else regs[28]
    regs[29] = sx(0x803F0000 - int(rng.integers(0, 0x1000)) * 16)
    regs[31] = sx(MAGIC_RET)
    hi = sx(int(random_words(rng, 1)[0]))
    lo = sx(int(random_words(rng, 1)[0]))
    fk = rng.integers(0, 100, 32)
    fvals = rng.uniform(-1000.0, 1000.0, 32).astype(np.float32).view(np.uint32)
    ivals = random_words(rng, 32)
    fpr = [int(fvals[k] if fk[k] < 70 else ivals[k]) for k in range(32)]
    # now and then a special value: NaNs of both kinds (legacy encoding:
    # top mantissa bit set = signaling), infinities, a denormal, -0
    for k in range(32):
        if fk[k] >= 97:
            fpr[k] = SPECIAL_FLOATS[int(rng.integers(0, len(SPECIAL_FLOATS)))]
    fcr31 = (1 << 23) if rng.integers(0, 2) else 0
    return regs, hi, lo, fpr, fcr31


def pointer_args():
    """Per translated function, the registers it (or a translated callee)
    uses as a load/store base before writing them: its pointer arguments,
    as far as a flow-insensitive scan can tell."""
    from analyze import load_all
    funcs = {fn.name: fn for o in load_all() for fn in o.functions}
    direct = {}
    calls = {}
    for name, fn in funcs.items():
        written, ptrs, cl = {0, 29}, set(), []
        for ln in fn.lines:
            ins = ln.insn
            if ins.kind in ("load", "store") and ins.rs not in written:
                ptrs.add(ins.rs)
            if ins.op == "jal":
                cl.append((ln.target, frozenset(written)))
            # registers this instruction writes
            if ins.kind == "load" and not ins.op.endswith("c1"):
                written.add(ins.rt)
            elif ins.kind == "alu" and ins.op not in ("mult", "multu", "div", "divu", "dmult",
                                                      "dmultu", "ddiv", "ddivu", "mthi", "mtlo"):
                written.add(ins.rt if ins.op[-1] == "i" or ins.op in ("lui", "addiu", "daddiu",
                            "sltiu", "andi", "ori", "xori") else ins.rd)
            elif ins.op in ("mfc1", "dmfc1", "cfc1", "mfc0"):
                written.add(ins.rt)
            elif ins.op == "jal":
                written.add(31)
        direct[name] = ptrs
        calls[name] = cl
    result = {n: set(p) for n, p in direct.items()}
    changed = True
    while changed:
        changed = False
        for name, cl in calls.items():
            for callee, w in cl:
                extra = result.get(callee, set()) - w - result[name]
                if extra:
                    result[name] |= extra
                    changed = True
    return {n: sorted(p - {0, 29, 31}) for n, p in result.items()}


def pointer_globals():
    """Addresses of words that translated code loads from a symbol and then
    uses as a load/store base: pointer variables.  The random fill gives
    them pointers, so that code gets past its first dereference."""
    from analyze import load_all
    from translate import elf_symbols
    objs = load_all()
    syms = {}
    found = set()
    for o in objs:
        if o.module not in syms:
            syms[o.module] = elf_symbols(os.path.join(BLAST, "build", f"{o.module}.{VERSION}.elf"))
        sy = syms[o.module]
        for fn in o.functions:
            addr_of, val_of = {}, {}      # reg -> address of / value at symbol
            labels = set(fn.labels.values())
            for k, ln in enumerate(fn.lines):
                if k in labels:
                    addr_of.clear()
                    val_of.clear()
                ins = ln.insn
                if ins.kind in ("load", "store") and ins.rs in val_of:
                    found.add(val_of[ins.rs])
                dest = None
                if ins.op == "addiu" and ln.lo and ins.rs == ins.rt:
                    addr_of[ins.rt] = sy.get(ln.lo)
                    val_of.pop(ins.rt, None)
                    continue
                if ins.op == "lw":
                    a = None
                    if ln.lo:
                        a = sy.get(ln.lo)
                    elif ins.rs in addr_of and addr_of[ins.rs] is not None:
                        a = addr_of[ins.rs] + ins.imm
                    addr_of.pop(ins.rt, None)
                    if a is not None:
                        val_of[ins.rt] = a
                    else:
                        val_of.pop(ins.rt, None)
                    continue
                if ins.kind == "load":
                    dest = ins.rt
                elif ins.kind == "alu":
                    dest = ins.rt if ins.op in ("lui", "addiu", "daddiu", "addi", "daddi", "slti",
                                                "sltiu", "andi", "ori", "xori") else ins.rd
                elif ins.op in ("mfc1", "dmfc1", "cfc1", "mfc0"):
                    dest = ins.rt
                elif ins.op == "jal":
                    addr_of.clear()
                    val_of.clear()
                if dest is not None:
                    addr_of.pop(dest, None)
                    val_of.pop(dest, None)
    return sorted(a for a in found if a is not None and a % 4 == 0)


def extern_addrs():
    """Addresses of everything translated code calls that isn't translated."""
    from analyze import load_all
    from translate import elf_symbols
    objs = load_all()
    funcs = {fn.name for o in objs for fn in o.functions}
    syms = {}
    out = set()
    for o in objs:
        if o.module not in syms:
            syms[o.module] = elf_symbols(os.path.join(BLAST, "build", f"{o.module}.{VERSION}.elf"))
        for fn in o.functions:
            for ln in fn.lines:
                if ln.insn.op in ("jal", "j") and ln.target not in funcs:
                    out.add(syms[o.module][ln.target])
    return sorted(out)


def stub_result(addr, a0, a1, a2, a3):
    """--stub-externs: what a stubbed callee leaves in $v0 (a deterministic
    function of the callee and its arguments; half of the time a pointer)."""
    h = (addr * 0x9E3779B1) ^ a0 ^ (a1 << 1) ^ (a2 << 2) ^ (a3 << 3)
    h = (h ^ (h >> 29) ^ (h >> 47)) & 0xFFFFFFFF
    if h & 1:
        return sx(0x80200000 + (h & 0x1FFFF0))
    return (h >> 8) & 0xFF


def is_lconv(op):
    """COP1 conversions from/to 64-bit integers ("L" format)."""
    return (op.startswith("cvt.") and op.endswith(".l")) or \
        op.split(".")[0] in ("round", "trunc", "ceil", "floor") and op.split(".")[1] == "l" or \
        op.startswith("cvt.l.")


def f32_bits(x):
    return int(np.array([x], dtype=np.float32).view(np.uint32)[0])


def f64_bits(x):
    return int(np.array([x], dtype=np.float64).view(np.uint64)[0])


def bits_f32(v):
    return float(np.array([v], dtype=np.uint32).view(np.float32)[0])


def bits_f64(v):
    return float(np.array([v], dtype=np.uint64).view(np.float64)[0])


class Tester:
    def __init__(self, lib_path, stub_externs=False, snapshots=()):
        self.stub_externs = stub_externs
        self.snapshots = list(snapshots)
        self.pool_cache = {}
        self.lib = ctypes.CDLL(lib_path)
        lib = self.lib
        lib.recomp_test_func_addr.restype = ctypes.c_uint32
        lib.recomp_test_func_name.restype = ctypes.c_char_p
        lib.recomp_test_cov.restype = ctypes.POINTER(ctypes.c_uint32)
        lib.recomp_test_call.argtypes = [ctypes.c_uint, ctypes.POINTER(Ctx),
                                         ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
        self.nfuncs = lib.recomp_test_num_funcs()
        self.nblocks = lib.recomp_test_num_blocks()
        self.names = [lib.recomp_test_func_name(i).decode() for i in range(self.nfuncs)]
        self.addrs = [lib.recomp_test_func_addr(i) for i in range(self.nfuncs)]
        self.img, self.keep = load_images()
        self.ptr_args = pointer_args()
        self.ptr_globals = pointer_globals()
        self.extern_list = extern_addrs()
        self.gp = elf_symbol("hd_code", "D_802E8BE4")
        self.buf = ctypes.create_string_buffer(RDRAM_SIZE)
        self.bufaddr = ctypes.addressof(self.buf)
        self.view = np.frombuffer(self.buf, dtype=np.uint8, count=RDRAM_SIZE)
        # whole 4 KB pages inside each module's .text
        self.ro_ranges = []
        for module, vram, size in IMAGES:
            if size:
                lo = (vram - 0x80000000 + 0xFFF) & ~0xFFF
                hi = (vram - 0x80000000 + size) & ~0xFFF
                self.ro_ranges.append((lo, hi))
        for k, (lo, hi) in enumerate(self.ro_ranges):
            lib.recomp_test_set_ro(k, lo, hi)
        self.new_uc()
        self._ext = EXTERN_CB(self._extern)
        self._mfc0 = MFC0_CB(self._mfc0_cb)
        lib.recomp_test_set_callbacks(self._ext, self._mfc0)
        self.mem_cache = {}

    # --- unicorn --------------------------------------------------------
    def new_uc(self):
        self.uc = Uc(UC_ARCH_MIPS, UC_MODE_MIPS64 | UC_MODE_BIG_ENDIAN)
        self.uc.mem_map_ptr(0, RDRAM_SIZE, UC_PROT_ALL, self.bufaddr)
        # Code is read-only in both runs.  Random pointers otherwise let the
        # original overwrite instructions it (or a later call) then
        # executes, which translated code can't see.
        for lo, hi in self.ro_ranges:
            self.uc.mem_protect(lo, hi - lo, UC_PROT_READ | UC_PROT_EXEC)
        self.uc_dirty = False
        self.uc.hook_add(UC_HOOK_INTR, self._on_intr)
        self.uc.hook_add(UC_HOOK_MEM_UNMAPPED, self._on_unmapped)
        self._hook_lconv()
        for a in self.extern_list:
            self.uc.hook_add(UC_HOOK_CODE, self._on_stub if self.stub_externs else self._on_call_out,
                             a, begin=sx(a), end=sx(a))

    # QEMU only accepts the L-format conversions (cvt.s.l, cvt.l.d, ...)
    # with Status.FR = 1 and raises Reserved Instruction otherwise.  The
    # VR4300 executes them with FR = 0 too, and the game relies on that, so
    # the harness executes those instructions itself in a code hook.
    def _hook_lconv(self):
        self.lconv_sites = 0
        for module, vram, size in IMAGES:
            off = vram - 0x80000000
            words = self.img[off:off + size].view(">u4")
            for k in range(len(words)):
                w = int(words[k])
                if (w >> 26) != 0x11:
                    continue
                try:
                    ins = decode(w)
                except DecodeError:
                    continue
                if not is_lconv(ins.op):
                    continue
                addr = vram + 4 * k
                prev = None
                if k > 0:
                    try:
                        prev = decode(int(words[k - 1]))
                    except DecodeError:
                        pass
                site = addr
                if prev is not None and prev.kind in ("branch", "jump"):
                    site = addr - 4
                self.uc.hook_add(UC_HOOK_CODE, self._on_lconv, (addr, ins, prev if site != addr else None),
                                 begin=sx(site), end=sx(site))
                self.lconv_sites += 1

    def _fpr(self, n):
        return self.uc.reg_read(FPR_UC[n]) & 0xFFFFFFFF

    def _lconv(self, ins):
        uc = self.uc
        fs, fd = ins.fs, ins.fd
        src = ins.op.split(".")[-1]
        if src == "l":
            v = (self._fpr(fs + 1) << 32) | self._fpr(fs)
            if v >= 1 << 63:
                v -= 1 << 64
            if ins.op == "cvt.s.l":
                uc.reg_write(FPR_UC[fd], f32_bits(np.float32(np.int64(v))))
            else:
                d = f64_bits(float(v))
                uc.reg_write(FPR_UC[fd], d & 0xFFFFFFFF)
                uc.reg_write(FPR_UC[fd + 1], d >> 32)
            return
        x = bits_f32(self._fpr(fs)) if src == "s" else \
            bits_f64((self._fpr(fs + 1) << 32) | self._fpr(fs))
        kind = ins.op.split(".")[0]
        import math
        if x != x or math.isinf(x):
            r = None
        else:
            r = {"cvt": round, "round": round, "trunc": math.trunc,
                 "ceil": math.ceil, "floor": math.floor}[kind](x)
        if r is None or not (-(1 << 63) <= r < (1 << 63)):
            r = (1 << 63) - 1
        r &= (1 << 64) - 1
        uc.reg_write(FPR_UC[fd], r & 0xFFFFFFFF)
        uc.reg_write(FPR_UC[fd + 1], r >> 32)

    def _on_lconv(self, uc, addr, size, data):
        at, ins, branch = data
        if branch is None:
            self._lconv(ins)
            uc.reg_write(MC.UC_MIPS_REG_PC, sx(at + 4))
            return
        # the conversion sits in this branch's delay slot
        rs = uc.reg_read(GPR_UC[branch.rs]) if branch.rs else 0
        rt = uc.reg_read(GPR_UC[branch.rt]) if branch.rt else 0
        to_s = lambda v: v - (1 << 64) if v >= 1 << 63 else v
        base = branch.op.rstrip("l") if branch.likely else branch.op
        taken = {"beq": rs == rt, "bne": rs != rt, "blez": to_s(rs) <= 0,
                 "bgtz": to_s(rs) > 0, "bltz": to_s(rs) < 0,
                 "bgez": to_s(rs) >= 0}.get(base)
        if taken is None:
            # not code the translator handles (or not code at all): let
            # unicorn raise its Reserved Instruction
            return
        if taken or not branch.likely:
            self._lconv(ins)
        tgt = (addr + 4 + (branch.imm << 2)) & 0xFFFFFFFF if taken else (addr + 8) & 0xFFFFFFFF
        uc.reg_write(MC.UC_MIPS_REG_PC, sx(tgt))

    def _on_intr(self, uc, intno, data):
        self.uc_trap = EXCP_CLASS.get(intno, f"excp{intno}")
        uc.emu_stop()

    def _on_unmapped(self, uc, access, addr, size, value, data):
        self.uc_trap = FAULT
        return False

    def uc_run(self, entry, stop, budget, sp=None):
        """Run unicorn from entry until pc == `stop` (with $sp == sp, if
        given); returns the outcome class."""
        self.uc_trap = None
        # unicorn compiles the `until` check into the translation block, so
        # a block cached from a run with another stop address would run on
        self.uc.ctl_remove_cache(sx(stop), sx(stop) + 4)
        try:
            self.uc.emu_start(sx(entry), sx(stop), count=budget)
        except UcError:
            if self.uc_trap is None:
                self.uc_trap = FAULT
        self.uc_dirty = True
        if self.uc_trap is not None:
            return self.uc_trap
        if self.uc.reg_read(MC.UC_MIPS_REG_PC) & 0xFFFFFFFF == stop:
            self.uc_dirty = False
            if sp is not None and self.uc.reg_read(GPR_UC[29]) != sp:
                return "reentered"
            return RETURN
        return TIMEOUT

    def uc_set(self, regs, hi, lo, fpr, fcr31):
        if self.uc_dirty:
            # After an exception unicorn keeps internal CPU state (e.g. a
            # pending delay slot) that register writes don't reset, and the
            # next run would inherit it: start from a fresh instance.
            self.new_uc()
        uc = self.uc
        uc.reg_write(MC.UC_MIPS_REG_CP0_STATUS, STATUS)
        for k in range(1, 32):
            uc.reg_write(GPR_UC[k], regs[k])
        uc.reg_write(MC.UC_MIPS_REG_HI, hi)
        uc.reg_write(MC.UC_MIPS_REG_LO, lo)
        for k in range(32):
            uc.reg_write(FPR_UC[k], fpr[k])
        uc.reg_write(MC.UC_MIPS_REG_FCSR, fcr31)

    def uc_get(self):
        uc = self.uc
        regs = [0] + [uc.reg_read(GPR_UC[k]) & 0xFFFFFFFFFFFFFFFF for k in range(1, 32)]
        hi = uc.reg_read(MC.UC_MIPS_REG_HI) & 0xFFFFFFFFFFFFFFFF
        lo = uc.reg_read(MC.UC_MIPS_REG_LO) & 0xFFFFFFFFFFFFFFFF
        fpr = [uc.reg_read(FPR_UC[k]) & 0xFFFFFFFF for k in range(32)]
        fcr31 = uc.reg_read(MC.UC_MIPS_REG_FCSR) & (1 << 23)
        return regs, hi, lo, fpr, fcr31

    # --- callbacks from the translated code ------------------------------
    # The translated code doesn't keep the FCSR's exception flag and cause
    # bits (nothing in the game reads them but libultra's thread switch,
    # which saves the FCSR); clear them in both runs where control leaves
    # the translated code.
    FCSR_FLAGS = 0x0003F07C

    def _on_call_out(self, uc, addr, size, a):
        uc.reg_write(MC.UC_MIPS_REG_FCSR, uc.reg_read(MC.UC_MIPS_REG_FCSR) & ~self.FCSR_FLAGS)

    def _on_stub(self, uc, addr, size, a):
        r = [uc.reg_read(GPR_UC[k]) & 0xFFFFFFFFFFFFFFFF for k in (4, 5, 6, 7, 31)]
        uc.reg_write(GPR_UC[2], stub_result(a, *r[:4]))
        uc.reg_write(MC.UC_MIPS_REG_PC, r[4])

    def _extern(self, pctx, addr):
        try:
            c = pctx.contents
            if self.stub_externs:
                c.r[2] = stub_result(addr, c.r[4], c.r[5], c.r[6], c.r[7])
                return 0
            # the callee runs until it comes back to the jal's return address
            self.uc_set(list(c.r), c.hi, c.lo, list(c.f), c.fcr31)
            out = self.uc_run(addr, c.r[31] & 0xFFFFFFFF, EXT_BUDGET, sp=c.r[29])
            regs, hi, lo, fpr, fcr31 = self.uc_get()
            for k in range(32):
                c.r[k] = regs[k]
                c.f[k] = fpr[k]
            c.hi, c.lo = hi, lo
            c.fcr31 = (c.fcr31 & ~(1 << 23)) | fcr31
            self.ext_trap = out
            return {RETURN: 0, BREAK: 1, SYSCALL: 2, OVERFLOW: 3, FAULT: 4,
                    TIMEOUT: 6}.get(out, 8)
        except Exception as e:  # never let an exception unwind through C
            print("extern callback:", e, file=sys.stderr)
            return 8

    def _mfc0_cb(self, reg):
        if reg == 12:
            return STATUS
        return 0

    # --- one trial -------------------------------------------------------
    def n_images(self):
        return N_IMAGES + len(self.snapshots)

    def memory(self, k):
        """Memory image k: a random fill (k < N_IMAGES) or a snapshot."""
        m = self.mem_cache.get(k)
        if m is None:
            if k < N_IMAGES:
                m = make_memory(k, self.img, self.keep, self.ptr_globals)
            else:
                m = np.fromfile(self.snapshots[k - N_IMAGES], dtype=np.uint8, count=RDRAM_SIZE)
                # the code has to be the code that was translated, whatever
                # the game had loaded there at the time (hd_front_end isn't
                # resident during a level)
                for module, vram, size in IMAGES:
                    off = vram - 0x80000000
                    m[off:off + size] = self.img[off:off + size]
            self.mem_cache[k] = m
        return m

    def pointer_pool(self, k):
        """Pointer-looking words of snapshot k, to seed pointer arguments."""
        if k < N_IMAGES:
            return None
        p = self.pool_cache.get(k)
        if p is None:
            w = self.memory(k).view(">u4").astype(np.uint32)
            w = w[(w >= 0x80000400) & (w < 0x80000000 + RDRAM_SIZE) & (w % 4 == 0)]
            p = self.pool_cache[k] = np.unique(w)
        return p

    def state(self, rng, i, t):
        k = t % self.n_images()
        return k, random_state(rng, self.gp, t, self.ptr_args.get(self.names[i]),
                               self.pointer_pool(k))

    def trial(self, i, k, state):
        regs, hi, lo, fpr, fcr31 = state
        mem = self.memory(k)
        entry = self.addrs[i]
        # original
        self.view[:] = mem
        self.uc_set(regs, hi, lo, fpr, fcr31)
        o1 = self.uc_run(entry, MAGIC_RET, BUDGET)
        s1 = self.uc_get()
        m1 = self.view.copy()
        # translated
        self.view[:] = mem
        c = Ctx()
        for k in range(32):
            c.r[k] = regs[k]
            c.f[k] = fpr[k]
        c.hi, c.lo, c.fcr31, c.budget = hi, lo, fcr31, BUDGET
        out = (ctypes.c_uint32 * 2)()
        self.ext_trap = None
        kind = self.lib.recomp_test_call(i, ctypes.byref(c), self.bufaddr, out)
        o2 = TRAP_CLASS.get(kind, OTHER)
        if kind and self.ext_trap not in (None, RETURN):
            o2 = self.ext_trap          # it happened inside a callee, in unicorn
        s2 = ([c.r[k] for k in range(32)], c.hi, c.lo, [c.f[k] for k in range(32)],
              c.fcr31 & (1 << 23))
        if TIMEOUT in (o1, o2):
            return "inconclusive", o1, None
        if o2 == "ra-changed" and o1 != RETURN:
            # the random state let a store overwrite the saved $ra: the
            # original then jumps somewhere arbitrary, which the translated
            # code refuses to follow
            return "inconclusive", o1, None
        diffs = []
        if o1 != o2:
            diffs.append(f"outcome {o1} vs {o2}" +
                         (f" (translated trap pc {out[0]:08X} code {out[1]:X})" if kind else ""))
        r1, h1, l1, f1, c1 = s1
        r2, h2, l2, f2, c2 = s2
        for k in range(1, 32):
            if r1[k] != r2[k]:
                diffs.append(f"r{k} {r1[k]:016X} vs {r2[k]:016X}")
        if h1 != h2:
            diffs.append(f"hi {h1:016X} vs {h2:016X}")
        if l1 != l2:
            diffs.append(f"lo {l1:016X} vs {l2:016X}")
        for k in range(32):
            if f1[k] != f2[k]:
                diffs.append(f"f{k} {f1[k]:08X} vs {f2[k]:08X}")
        if c1 != c2:
            diffs.append("fcc")
        if not np.array_equal(m1, self.view):
            d = np.nonzero(m1 != self.view)[0]
            diffs.append(f"memory differs at {len(d)} bytes, first 0x{0x80000000 + int(d[0]):08X}")
        if diffs:
            return "fail", o1, diffs
        return "pass", o1, None

    def test_function(self, i, trials, seed0):
        res = {"pass": 0, "fail": 0, "inconclusive": 0}
        outcomes = {}
        first_fail = None
        rng = np.random.default_rng([seed0, i])
        for t in range(trials):
            k, state = self.state(rng, i, t)
            r, o, diffs = self.trial(i, k, state)
            res[r] += 1
            outcomes[o] = outcomes.get(o, 0) + 1
            if r == "fail" and first_fail is None:
                first_fail = (t, diffs)
        return res, outcomes, first_fail

    def trace(self, i, t, seed0):
        """Replay trial t of function i with instruction traces on both sides
        and print where they first diverge (needs a RECOMP_TRACE build)."""
        from analyze import load_all
        lines = {}
        for o in load_all():
            for fn in o.functions:
                for ln in fn.lines:
                    lines[ln.vram] = (fn.name, ln)
        rng = np.random.default_rng([seed0, i])
        for tt in range(t + 1):
            k, state = self.state(rng, i, tt)
        regs, hi, lo, fpr, fcr31 = state
        mem = self.memory(k)
        utrace = []
        self.tracing = True

        def hook(uc, addr, size, data):
            a = addr & 0xFFFFFFFF
            if self.tracing and a in lines:
                utrace.append((a, [0] + [uc.reg_read(GPR_UC[k]) & 0xFFFFFFFFFFFFFFFF
                                         for k in range(1, 32)]))
        h = self.uc.hook_add(UC_HOOK_CODE, hook)
        self.view[:] = mem
        self.uc_set(regs, hi, lo, fpr, fcr31)
        o1 = self.uc_run(self.addrs[i], MAGIC_RET, BUDGET)
        self.uc.hook_del(h)
        self.tracing = False

        cap = 2_000_000
        rec = np.zeros((cap, 33), dtype=np.uint64)
        self.lib.recomp_test_trace(rec.ctypes.data_as(ctypes.c_void_p), cap)
        self.view[:] = mem
        c = Ctx()
        for k in range(32):
            c.r[k] = regs[k]
            c.f[k] = fpr[k]
        c.hi, c.lo, c.fcr31, c.budget = hi, lo, fcr31, BUDGET
        out = (ctypes.c_uint32 * 2)()
        kind = self.lib.recomp_test_call(i, ctypes.byref(c), self.bufaddr, out)
        n = self.lib.recomp_test_trace_len()
        self.lib.recomp_test_trace(None, 0)
        ctrace = [(int(rec[k, 0]) & 0xFFFFFFFF, [int(x) for x in rec[k, 1:]]) for k in range(n)]
        print(f"original: {o1}, {len(utrace)} steps; translated: {TRAP_CLASS.get(kind)} "
              f"(pc {out[0]:08X}), {len(ctrace)} steps")
        # unicorn calls the code hook for a branch-likely's delay slot even
        # when the branch isn't taken and the slot is nullified; drop those
        filt = []
        for k, (pa, ra) in enumerate(utrace):
            if filt and k + 1 < len(utrace) and pa == filt[-1][0] + 4 and \
                    lines[filt[-1][0]][1].insn.likely and utrace[k + 1][0] == pa + 4 \
                    and not (0 < len(filt) < len(ctrace) and ctrace[len(filt)][0] == pa):
                continue
            filt.append((pa, ra))
        utrace = filt
        for k in range(min(len(utrace), len(ctrace))):
            (pa, ra), (pb, rb) = utrace[k], ctrace[k]
            if pa != pb or ra != rb:
                print(f"diverge at step {k}:")
                for j in range(max(0, k - 6), k):
                    fn, ln = lines[utrace[j][0]]
                    print(f"   {utrace[j][0]:08X} {fn}: {ln.text_op} {ln.operands}")
                print(f"   pc {pa:08X} vs {pb:08X}")
                for r in range(32):
                    if ra[r] != rb[r]:
                        print(f"   r{r} {ra[r]:016X} vs {rb[r]:016X}")
                return
        print("traces agree on their common prefix; last steps:")
        for name, tr in (("original", utrace), ("translated", ctrace)):
            for pa, _ in tr[-4:]:
                fn, ln = lines[pa]
                print(f"   {name:10} {pa:08X} {fn}: {ln.text_op} {ln.operands}")

    def coverage(self):
        cov = self.lib.recomp_test_cov()
        return np.ctypeslib.as_array(cov, shape=(self.nblocks,)).copy()


_tester = None


def _init(lib, stub, snaps):
    global _tester
    _tester = Tester(lib, stub, snaps)


def _work(args):
    i, trials, seed = args
    before = _tester.coverage()
    res, outcomes, ff = _tester.test_function(i, trials, seed)
    blocks = np.nonzero(_tester.coverage() != before)[0]
    return i, res, outcomes, ff, blocks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lib")
    ap.add_argument("--trials", type=int, default=40)
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--only", nargs="*")
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--snapshots", nargs="*", metavar="DIR",
                    help="also use RDRAM snapshots (rdram_*.bin from snapshot.c) as memory images")
    ap.add_argument("--blocks", help="blocks.tsv from translate.py, for opcode coverage")
    ap.add_argument("--save-coverage", metavar="FILE.npy",
                    help="save the executed-block mask (to merge runs with --merge-coverage)")
    ap.add_argument("--merge-coverage", nargs="*", metavar="FILE.npy", default=[],
                    help="count these runs' executed blocks too")
    ap.add_argument("--stub-externs", action="store_true",
                    help="calls out of the translated code return a hash of their "
                         "arguments in both runs, instead of running the callee")
    ap.add_argument("--trace", metavar="FUNC:TRIAL",
                    help="replay one trial with instruction traces (RECOMP_TRACE build)")
    args = ap.parse_args()

    snaps = []
    for d in args.snapshots or ():
        snaps += sorted(os.path.join(d, f) for f in os.listdir(d)
                        if f.startswith("rdram_") and f.endswith(".bin"))
    probe = Tester(args.lib, args.stub_externs, snaps)
    if args.trace:
        name, t = args.trace.split(":")
        probe.trace(probe.names.index(name), int(t), args.seed)
        return 0
    idx = list(range(probe.nfuncs))
    if args.only:
        want = set(args.only)
        idx = [i for i in idx if probe.names[i] in want]
    nblocks = probe.nblocks
    names = probe.names
    del probe

    t0 = time.time()
    results = {}
    ctx = mp.get_context("fork")
    with ctx.Pool(args.jobs, initializer=_init, initargs=(args.lib, args.stub_externs, snaps)) as pool:
        covered = np.zeros(nblocks, dtype=bool)
        for i, res, outcomes, ff, blocks in pool.imap_unordered(
                _work, [(i, args.trials, args.seed) for i in idx], chunksize=1):
            results[i] = (res, outcomes, ff)
            covered[blocks] = True
            if ff or args.verbose:
                status = "FAIL" if ff else "ok"
                print(f"{status} {names[i]} {res} {outcomes}")
                if ff:
                    t, diffs = ff
                    print(f"     trial {t}: " + "; ".join(diffs[:8]))

    for f in args.merge_coverage:
        covered |= np.load(f)
    npass = sum(1 for r, o, ff in results.values() if not ff and r["pass"] > 0)
    nfail = sum(1 for r, o, ff in results.values() if ff)
    ninc = sum(1 for r, o, ff in results.values() if not ff and r["pass"] == 0)
    returned = sum(1 for r, o, ff in results.values() if not ff and o.get(RETURN, 0) > 0)
    trapped_only = npass - returned
    tot = {}
    for r, o, ff in results.values():
        for k, v in o.items():
            tot[k] = tot.get(k, 0) + v
    print(f"\n{len(results)} functions, {args.trials} trials each, {time.time() - t0:.0f}s")
    print(f"  pass: {npass} ({returned} returned normally in at least one trial, "
          f"{trapped_only} only ever trapped/faulted identically)")
    print(f"  fail: {nfail}")
    print(f"  no conclusive trial (budget exhausted): {ninc}")
    print(f"  trial outcomes (original): {tot}")
    print(f"  translated blocks executed: {int(covered.sum())}/{nblocks} "
          f"({100.0 * covered.sum() / max(nblocks, 1):.1f}%)")
    if args.blocks:
        ops_all, ops_run = {}, {}
        insns = insns_run = 0
        for line in open(args.blocks):
            bid, name, ops = line.rstrip("\n").split("\t")
            bid = int(bid)
            for op in ops.split():
                ops_all[op] = ops_all.get(op, 0) + 1
                insns += 1
                if covered[bid]:
                    ops_run[op] = ops_run.get(op, 0) + 1
                    insns_run += 1
        never = sorted(op for op in ops_all if op not in ops_run)
        print(f"  translated instructions executed: {insns_run}/{insns} "
              f"({100.0 * insns_run / max(insns, 1):.1f}%)")
        print(f"  opcodes executed: {len(ops_run)}/{len(ops_all)}; never: {' '.join(never) or '-'}")
    if args.save_coverage:
        np.save(args.save_coverage, covered)
    return 1 if nfail else 0


if __name__ == "__main__":
    sys.exit(main())
