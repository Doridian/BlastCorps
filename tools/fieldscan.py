#!/usr/bin/env python3
"""Collect every memory access the game's code makes, and infer which
memory holds pointers, ROM addresses and segment addresses.

    tools/fieldscan.py [--version us.v11] [--out DIR]

Reads the linked stage-2 ELFs (blastcorps/build/<module>.<v>.elf: init,
hd_code, hd_front_end) and analyses all of their code the same way: the
IDO-compiled C and Rare's handwritten asm alike, decoded from the words in
the image (tools/recomp/mips.py), with the data islands and microcode
(the `bin` subsegments) left out.

Each function gets a forward dataflow pass over its registers and stack
frame.  A register holds either nothing known or a linear value

    a * X + k + (some multiple of s)

where X is an origin: a word loaded from a memory location, a register
the function was entered with, a register a callee returned, or the entry
stack pointer.  With no origin it is a constant (an address when it comes
from a %hi/%lo relocation).  Loop joins keep the difference of the two
constants in s, so a pointer stepping through an array, or an index scaled
by an element size, comes out as a base plus a stride.

Every load and store is then resolved to a location: an absolute address
(a global, maybe indexed with a stride), or an offset from what some other
location / argument / return value points to.  The locations, arguments
and return values are "slots"; a store (or a call, or a return) of a value
that came from one slot into another is a flow from the first to the
second.  A slot holds a pointer when its value is dereferenced, when an
address constant is stored into it, when a pointer flows into it, or when
it flows into a slot whose value is dereferenced; the few functions that
pass a value of any type along (varargs printfs, message queues) are left
out of the flows.  A word added to the pointer to the record it sits in is
an offset (the level file's section offsets).  Where registers from two
paths meet, a value keeps every origin it may have (up to 12).  Calls to
the C only return what the C definition says they return, so a void
function's leftover $v0 flows nowhere.

The output (in --out, default blastcorps/build/fieldscan):

  fields.tsv     every field of every region the code reaches: region (a
                 global, or *(slot) for what a pointer slot points to),
                 offset (folded by the region's element size), the access
                 widths and signedness, the verdict (ptr, offset, rom, seg,
                 phys, static for relocated initialized data), how many
                 loads and stores, which files, and (with --dumps) what the
                 RDRAM snapshots showed there
  types.txt      structures by their instances: the addresses that flow
                 into the same argument or pointer are one type; every field
                 any view of it touches
  drafts.txt     a C struct draft for each of those

tools/inventory.py builds the committed inventory from the same analysis.
"""

import argparse
import math
import os
import re
import struct
import sys
from collections import defaultdict

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "recomp"))
import mips  # noqa: E402

RAM_LO, RAM_HI = 0x80000000, 0x80800000
# linker and section marker symbols, not variables
MARKER_RE = re.compile(r"^(_binary_|.*_(ROM_START|ROM_END|VRAM|VRAM_END|DATA_START|DATA_END|BSS_START|BSS_END|"
                       r"TEXT_START|TEXT_END|RODATA_START|RODATA_END|DMA_END)$|\w+_[0-9A-F]{4,5}_(c|s)$|"
                       r"\w+_[0-9A-F]{4,5}$)")
MODULES = ["init", "hd_code", "hd_front_end"]

GP_ZERO, GP_SP, GP_RA = 0, 29, 31
ABI_CLOBBER = frozenset([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 31])

# Functions that pass a value of any type along: their arguments aren't
# linked to the callers' slots (a message is an int as often as a pointer).
GENERIC_ARGS = {
    "osSendMesg": {5}, "osJamMesg": {5}, "osSetEventMesg": {6},
    "osViSetEvent": {5}, "osCreateThread": {7}, "osSetTimer": set(),
    "func_8029A7E4": {5, 6, 7}, "func_8029A7D0": {5, 6, 7},
    "sprintf": {6, 7}, "osSyncPrintf": {5, 6, 7}, "_Printf": {6, 7},
    "osCreateMesgQueue": set(),
}
# The allocator's pointers: everything the game allocates comes from them,
# so a value read from them says nothing about its type.
HEAP_POINTERS = ["D_80358070", "D_8035806C"]
# Game functions that take a pointer to anything (copies, clears, DMA).
GENERIC_FUNCS = set()

# Stack arguments of the varargs functions are never linked either.
VARARGS = {"func_8029A7E4", "func_8029A7D0", "sprintf", "osSyncPrintf", "_Printf"}


# ---------------------------------------------------------------- ELF

class Elf:
    def __init__(self, path):
        self.path = path
        data = open(path, "rb").read()
        self.data = data
        (e_shoff,) = struct.unpack_from(">I", data, 0x20)
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(">HHH", data, 0x2E)
        self.sections = []
        for i in range(e_shnum):
            sh = struct.unpack_from(">IIIIIIIIII", data, e_shoff + i * e_shentsize)
            self.sections.append(dict(name_off=sh[0], type=sh[1], flags=sh[2], addr=sh[3],
                                      offset=sh[4], size=sh[5], link=sh[6], info=sh[7]))
        strtab = self.sections[e_shstrndx]
        for s in self.sections:
            s["name"] = self._str(strtab, s["name_off"])
        self.symbols = []
        for s in self.sections:
            if s["type"] == 2:  # SYMTAB
                st = self.sections[s["link"]]
                self.symtab_index = len(self.symbols)
                for off in range(s["offset"], s["offset"] + s["size"], 16):
                    name, value, size, info, other, shndx = struct.unpack_from(">IIIBBH", data, off)
                    self.symbols.append(dict(name=self._str(st, name), value=value, size=size,
                                             type=info & 15, bind=info >> 4, shndx=shndx))
        self.relocs = []  # (section index it applies to, offset address, type, symbol value, symbol name)
        for s in self.sections:
            if s["type"] == 9:  # REL
                target = self.sections[s["info"]]
                for off in range(s["offset"], s["offset"] + s["size"], 8):
                    r_off, r_info = struct.unpack_from(">II", data, off)
                    sym = self.symbols[r_info >> 8]
                    self.relocs.append((s["info"], r_off, r_info & 0xFF, sym["value"], sym["name"],
                                        target["name"]))

    def _str(self, sec, off):
        base = sec["offset"] + off
        end = self.data.index(b"\0", base)
        return self.data[base:end].decode()

    def progbits(self):
        for s in self.sections:
            if s["type"] == 1 and s["addr"]:
                yield s["name"], s["addr"], self.data[s["offset"]:s["offset"] + s["size"]]


# ---------------------------------------------------------------- C definitions

DEF_RE = re.compile(r"(?:(?<=[;{}])|^)\s*((?:[A-Za-z_][\w]*[\s\*]+)+?)(\**)\s*(\w+)\s*"
                    r"\(([^()]*(?:\([^()]*\)[^()]*)*)\)\s*\{", re.S | re.M)


def c_definitions():
    """name -> return type of every function defined in blastcorps/src (the
    IDO-compiled C): which registers a call to it really returns."""
    out = {}
    src = os.path.join(ROOT, "blastcorps", "src")
    for d, _, files in os.walk(src):
        for fn in files:
            if not fn.endswith(".c"):
                continue
            text = open(os.path.join(d, fn), errors="replace").read()
            text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
            text = re.sub(r"//[^\n]*", " ", text)
            text = re.sub(r"^\s*#.*$", " ", text, flags=re.M)
            for m in DEF_RE.finditer(text):
                ret = (m.group(1) + m.group(2)).strip()
                words = ret.replace("*", " * ").split()
                if not words or words[0] in ("return", "else", "if", "while", "case", "goto", "do", "switch"):
                    continue
                words = [w for w in words if w not in ("static", "extern", "inline", "const", "volatile",
                                                       "register", "struct", "union", "enum")]
                if "*" in words:
                    kind = "int"
                elif words in (["void"],):
                    kind = "void"
                elif words and words[-1] in ("f32", "float", "f64", "double"):
                    kind = "float"
                elif words and (words[-1] in ("s64", "u64") or words[-2:] == ["long", "long"]):
                    kind = "int64"
                else:
                    kind = "int"
                out.setdefault(m.group(3), kind)
    return out


RET_REGS = {"void": (), "float": (), "int": (2,), "int64": (2, 3)}


# ---------------------------------------------------------------- program

class Program:
    def __init__(self, version):
        self.version = version
        self.mem = []            # (lo, hi, bytes)
        self.symbols = {}        # addr -> [names]
        self.named = {}          # name -> addr
        self.sized = []          # (addr, size, name) of data objects with sizes
        self.data_syms = []      # sorted data symbol addresses
        self.funcs = []          # (start, end, name, module, file, kind)
        self.relhi = {}          # text address -> symbol value (HI16/LO16 relocations)
        self.data_relocs = []    # (address, value, symbol) R_MIPS_32 in data
        self.bss = []            # (lo, hi, module)
        self.datasecs = []       # (lo, hi, module, section)
        for m in MODULES:
            self._load(m)
        self.data_syms.sort()
        self.funcs.sort()
        self.func_at = {f[0]: f for f in self.funcs}
        self.func_by_name = {}
        for f in self.funcs:
            self.func_by_name.setdefault(f[2], f)

    def _subsegments(self, module):
        """[(vram, kind, name)] of the module's code segment, from the yaml."""
        path = os.path.join(ROOT, "blastcorps", f"{module}.{self.version}.yaml")
        vram = None
        out = []
        for line in open(path):
            m = re.match(r"\s*vram:\s*(0x[0-9A-Fa-f]+)", line)
            if m and vram is None:
                vram = int(m.group(1), 16)
            m = re.match(r"\s*-\s*\[(0x[0-9A-Fa-f]+),\s*([\w.]+)(?:,\s*([^\]\s]+))?", line)
            if m and vram is not None:
                out.append((vram + int(m.group(1), 16), m.group(2), m.group(3) or ""))
        return out

    def _load(self, module):
        elf = Elf(os.path.join(ROOT, "blastcorps", "build", f"{module}.{self.version}.elf"))
        text = None
        for s in elf.sections:
            if s["type"] == 1 and s["addr"] and s["flags"] & 4:
                text = s
        for name, addr, blob in elf.progbits():
            self.mem.append((addr, addr + len(blob), blob))
            if not (text and name == text["name"]):
                self.datasecs.append((addr, addr + len(blob), module, name))
        for s in elf.sections:
            if s["type"] == 8 and s["addr"]:
                self.bss.append((s["addr"], s["addr"] + s["size"], module))
        tlo, thi = text["addr"], text["addr"] + text["size"]
        subs = self._subsegments(module)
        # code ranges: the text minus bin subsegments
        spans = []
        for i, (va, kind, name) in enumerate(subs):
            if va >= thi:
                break
            end = subs[i + 1][0] if i + 1 < len(subs) else thi
            end = min(end, thi)
            spans.append((va, end, kind, name))
        codespans = [(va, end) for va, end, kind, name in spans if kind in ("c", "asm", "hasm")]

        def in_code(v):
            return any(a <= v < b for a, b in codespans)

        funcstarts = set()
        for sym in elf.symbols:
            v = sym["value"]
            if sym["name"]:
                self.symbols.setdefault(v, [])
                if sym["name"] not in self.symbols[v]:
                    self.symbols[v].append(sym["name"])
                self.named.setdefault(sym["name"], v)
            if tlo <= v < thi and sym["type"] in (0, 2) and sym["shndx"] != 0 and sym["name"] \
                    and not sym["name"].startswith((".", "$")) and sym["type"] != 3:
                funcstarts.add(v)
            elif RAM_LO <= v < RAM_HI and not in_code(v) and sym["name"] \
                    and sym["type"] in (0, 1) and sym["shndx"] not in (0, 0xFFF1) \
                    and not MARKER_RE.match(sym["name"]):
                self.data_syms.append(v)
                if sym["size"]:
                    self.sized.append((v, sym["size"], sym["name"]))
        # call targets start functions too (IDO's static functions have no
        # symbol of their own in some objects)
        for va, end, kind, name in spans:
            if kind in ("c", "asm", "hasm"):
                for pc in range(va, end, 4):
                    blob_w = None
                    for lo, hi, blob in self.mem:
                        if lo <= pc < hi:
                            blob_w = struct.unpack_from(">I", blob, pc - lo)[0]
                            break
                    if blob_w is not None and blob_w >> 26 == 3:
                        t = ((pc + 4) & 0xF0000000) | ((blob_w & 0x3FFFFFF) << 2)
                        if tlo <= t < thi:
                            funcstarts.add(t)
        for va, end, kind, name in spans:
            if kind in ("c", "asm", "hasm"):
                starts = sorted([va] + [f for f in funcstarts if va < f < end])
                for i, s in enumerate(starts):
                    e = starts[i + 1] if i + 1 < len(starts) else end
                    names = self.symbols.get(s, ["func_%08X" % s])
                    fname = sorted(names, key=lambda n: (n.startswith(("D_", ".")), n))[0]
                    self.funcs.append((s, e, fname, module, name, kind))
        for sec_index, off, rtype, value, sname, secname in elf.relocs:
            if rtype in (5, 6) and tlo <= off < thi:   # HI16, LO16
                self.relhi[off] = value
            elif rtype == 2 and secname != (text["name"] if text else None):
                w = self.read32(off)
                self.data_relocs.append((off, w, sname))
            elif rtype == 2:
                w = self.read32(off)
                self.data_relocs.append((off, w, sname))

    def read32(self, addr):
        for lo, hi, blob in self.mem:
            if lo <= addr and addr + 4 <= hi:
                return struct.unpack_from(">I", blob, addr - lo)[0]
        return None

    def var_of(self, addr):
        """(name, start) of the variable an address is in: the sized data
        symbol containing it, else the data symbol below it."""
        import bisect
        if not hasattr(self, "_sized_sorted"):
            self._sized_sorted = sorted(self.sized)
            self._sized_starts = [x[0] for x in self._sized_sorted]
        i = bisect.bisect_right(self._sized_starts, addr) - 1
        # a sized symbol can hold smaller ones: take the widest that contains it
        best = None
        j = i
        while j >= 0 and j > i - 64:
            st, size, name = self._sized_sorted[j]
            if st <= addr < st + size and (best is None or size > best[1]):
                best = (st, size, name)
            j -= 1
        if best is not None:
            return best[2], best[0]
        return self.sym_containing(addr)

    def var_size(self, start):
        """The size of the variable at start: its symbol's, else up to the
        next data symbol."""
        import bisect
        for st, size, name in self.sized:
            pass
        if not hasattr(self, "_size_of"):
            self._size_of = {}
            for st, size, name in self.sized:
                self._size_of[st] = max(size, self._size_of.get(st, 0))
        if start in self._size_of:
            return self._size_of[start]
        i = bisect.bisect_right(self.data_syms, start)
        return (self.data_syms[i] - start) if i < len(self.data_syms) else 0

    def sym_containing(self, addr):
        """(name, start) of the data symbol at or below addr."""
        import bisect
        i = bisect.bisect_right(self.data_syms, addr) - 1
        if i < 0:
            return None, None
        start = self.data_syms[i]
        names = self.symbols.get(start, ["D_%08X" % start])
        return names[0], start

    def section_of(self, addr):
        for lo, hi, m in self.bss:
            if lo <= addr < hi:
                return m, ".bss"
        for lo, hi, m, s in self.datasecs:
            if lo <= addr < hi:
                return m, s
        for f in self.funcs:
            pass
        return None, None


# ---------------------------------------------------------------- values

# A value is None (unknown) or a tuple (origin, a, k, s, tag):
#   origin None: the constant k (plus a multiple of s); tag 'ram', 'rom',
#   'seg' for relocated addresses, 'phys' for K0_TO_PHYS of a pointer.
#   origin set: a * X(origin) + k (+ multiple of s).
# Origins: ('L', loc) the word loaded from loc; ('I', loc) a narrower load;
# ('A', func, reg) the register at entry; ('R', func, reg) a callee's result;
# ('SP', func) the entry stack pointer.

def V(origin, a, k, s=0, tag=None):
    return (origin, a, k, s, tag)


def const(k, tag=None):
    return (None, 0, k & 0xFFFFFFFF, 0, tag)


def gcd(*xs):
    g = 0
    for x in xs:
        g = math.gcd(g, abs(x))
    return g


def s32(k):
    k &= 0xFFFFFFFF
    return k - 0x100000000 if k & 0x80000000 else k


def v_add(x, y):
    r = _v_add(x, y)
    if r is not None and r[0] is not None:
        r = (r[0], r[1], s32(r[2]), r[3], r[4])
    return r


def _v_add(x, y):
    if x is None or y is None:
        return None
    ox, ax, kx, sx, tx = x
    oy, ay, ky, sy, ty = y
    tag = tx or ty
    if ox is None and oy is None:
        k = kx + ky
        if tag in ("ram", "rom", "seg"):
            return (None, 0, k & 0xFFFFFFFF, gcd(sx, sy), tag)
        return (None, 0, k & 0xFFFFFFFF, gcd(sx, sy), None)
    if ox is None:
        if is_addr(x):
            # an address constant plus an index (IDO's lui + addu + lw %lo)
            return (None, 0, (kx + ky) & 0xFFFFFFFF, gcd(sx, sy, ay, 1 if ay == 1 else 0), tx)
        return (oy, ay, ky + s32(kx), gcd(sx, sy), None)
    if oy is None:
        return _v_add(y, x)
    if ox == oy:
        a = ax + ay
        if a == 0:
            return (None, 0, (kx + ky) & 0xFFFFFFFF, gcd(sx, sy), None)
        return (ox, a, kx + ky, gcd(sx, sy), None)
    # two different origins: keep the one that can be a pointer
    px = ax == 1 and ox[0] not in ("I", "U")
    py = ay == 1 and oy[0] not in ("I", "U")
    if px and not py:
        return (ox, ax, kx + ky, gcd(sx, sy, ay), None)
    if py and not px:
        return (oy, ay, kx + ky, gcd(sx, sy, ax), None)
    if px and py:
        # a pointer plus a word: one of them is an offset (a level file's
        # section offsets are added to the file's address)
        ADDPAIRS.add((ox, oy))
        return (ox, ax, kx + ky, gcd(sx, sy, 1), None)
    return None


ADDPAIRS = set()


def v_neg(x):
    if x is None:
        return None
    o, a, k, s, t = x
    if o is None:
        return (None, 0, (-k) & 0xFFFFFFFF, s, None)
    return (o, -a, -k, s, None)


def v_mul(x, c):
    if x is None:
        return None
    o, a, k, s, t = x
    if o is None:
        return (None, 0, (k * c) & 0xFFFFFFFF, s * abs(c), None)
    if a * c == 0:
        return const(0)
    return (o, a * c, k * c, s * abs(c), None)


def is_const(x):
    return x is not None and x[0] is None and x[3] == 0


def is_addr(x):
    return x is not None and x[0] is None and RAM_LO <= x[2] < RAM_HI


JOIN_INT = (("U", "join"), 1, 0, 0, None)
MAX_MAY = 12


def _intlike(x):
    """A value that can't be a pointer: a small constant or an integer."""
    o = x[0]
    if o is None:
        return not (RAM_LO <= x[2] < RAM_HI) and x[4] is None
    return o[0] in ("I", "U") or x[1] != 1


def _members(o):
    return o[1] if o[0] == "M" else frozenset([o])


def v_join(x, y):
    if x == y:
        return x
    if x is None or y is None:
        return None
    ox, ax, kx, sx, tx = x
    oy, ay, ky, sy, ty = y
    if ox is not None and oy is not None and ox != oy and ax == 1 and ay == 1 \
            and ox[0] in ("L", "A", "R", "M") and oy[0] in ("L", "A", "R", "M"):
        # two pointers meet (a Gfx * a helper returns, a list walked from a
        # global then from a field): the value may come from either
        m = _members(ox) | _members(oy)
        if len(m) > MAX_MAY:
            return None
        return (("M", m), 1, kx, gcd(sx, sy, kx - ky), None)
    if ox != oy or ax != ay:
        if _intlike(x) and _intlike(y):
            return JOIN_INT
        return None
    if ox is None:
        d = (kx - ky) & 0xFFFFFFFF
        if d >= 0x80000000:
            d -= 0x100000000
    else:
        d = kx - ky
    s = gcd(sx, sy, d)
    if ox is None and tx != ty:
        return None
    return (ox, ax, kx, s, tx if tx == ty else None)


# ---------------------------------------------------------------- analysis

class FuncInfo:
    def __init__(self, f):
        self.start, self.end, self.name, self.module, self.file, self.kind = f
        self.insns = {}
        self.writes = set()
        self.saved = set()
        self.callees = set()
        self.clobber = None


class State:
    __slots__ = ("regs", "frame", "lo", "esc")

    def __init__(self, regs, frame, lo=None, esc=None):
        self.regs = regs
        self.frame = frame
        self.lo = lo
        self.esc = esc

    def copy(self):
        return State(list(self.regs), dict(self.frame), self.lo, self.esc)

    def join(self, other):
        regs = [v_join(a, b) for a, b in zip(self.regs, other.regs)]
        frame = {}
        for k, v in self.frame.items():
            o = other.frame.get(k)
            if o is not None and o[1] == v[1]:
                j = v_join(v[0], o[0])
                if j is not None:
                    frame[k] = (j, v[1])
        esc = self.esc if other.esc is None else (other.esc if self.esc is None else self.esc | other.esc)
        return State(regs, frame, v_join(self.lo, other.lo), esc)

    def key(self):
        return (tuple(self.regs), tuple(sorted(self.frame.items(), key=lambda kv: kv[0])), self.lo, self.esc)


LOAD_W = {"lb": 1, "lbu": 1, "lh": 2, "lhu": 2, "lw": 4, "lwu": 4, "ld": 8, "ll": 4, "lld": 8,
          "lwl": 4, "lwr": 4, "ldl": 8, "ldr": 8, "lwc1": 4, "ldc1": 8}
STORE_W = {"sb": 1, "sh": 2, "sw": 4, "sd": 8, "sc": 4, "scd": 8, "swl": 4, "swr": 4,
           "sdl": 8, "sdr": 8, "swc1": 4, "sdc1": 8}
SIGNED = {"lb", "lh", "lw"}


class Analyzer:
    def __init__(self, prog):
        self.prog = prog
        self.infos = {}
        self.cdefs = c_definitions()
        self.events = []     # ('acc', ...), ('call', ...), ('ret', ...)
        for f in prog.funcs:
            fi = FuncInfo(f)
            self.infos[fi.start] = fi
            for pc in range(fi.start, fi.end, 4):
                w = prog.read32(pc)
                try:
                    fi.insns[pc] = mips.decode(w)
                except mips.DecodeError:
                    fi.insns[pc] = None
        self._clobbers()

    # ---- which registers a call can change

    def _clobbers(self):
        for fi in self.infos.values():
            written_before = set()
            for pc in sorted(fi.insns):
                i = fi.insns[pc]
                if i is None:
                    continue
                if i.kind == "store" and i.rs == GP_SP and i.op in ("sw", "sd") and i.rt not in written_before:
                    fi.saved.add(i.rt)
                d = defs(i)
                if d is not None:
                    fi.writes.add(d)
                    written_before.add(d)
                if i.op == "jal":
                    fi.callees.add(jal_target(pc, i))
                if i.op == "jalr":
                    fi.callees.add(None)
            fi.saved.discard(GP_RA)
            fi.clobber = (fi.writes - fi.saved) | {GP_RA} if fi.insns else set(ABI_CLOBBER)
        changed = True
        while changed:
            changed = False
            for fi in self.infos.values():
                c = set(fi.clobber)
                for t in fi.callees:
                    cf = self.infos.get(t)
                    extra = ABI_CLOBBER if cf is None else cf.clobber
                    c |= (set(extra) - fi.saved)
                c.discard(GP_SP)
                c.discard(GP_ZERO)
                if c != fi.clobber:
                    fi.clobber = c
                    changed = True

    def returns_of(self, target):
        """The registers a callee hands back: from the C definition for the
        IDO-compiled code, everything it changes for Rare's."""
        fi = self.infos.get(target)
        if fi is None:
            return (2, 3)
        if fi.kind == "c":
            k = self.cdefs.get(fi.name)
            if k is not None:
                return RET_REGS[k]
            return (2, 3)
        return tuple(sorted(r for r in fi.clobber if r != GP_RA))

    def clobber_of(self, target):
        fi = self.infos.get(target)
        return ABI_CLOBBER if fi is None else fi.clobber

    # ---- one function

    def run(self):
        for n, fi in enumerate(self.infos.values()):
            self.analyze(fi)

    def analyze(self, fi):
        insns = fi.insns
        leaders = {fi.start}
        for pc, i in insns.items():
            if i is None:
                continue
            if i.kind == "branch":
                t = pc + 4 + (i.imm << 2)
                leaders.add(t)
                leaders.add(pc + 8)
            elif i.op == "j":
                leaders.add(jal_target(pc, i))
                leaders.add(pc + 8)
            elif i.op in ("jr", "jal", "jalr"):
                leaders.add(pc + 8)
        entry = State([None] * 32, {})
        for r in range(1, 32):
            entry.regs[r] = V(("A", fi.start, r), 1, 0)
        entry.regs[GP_SP] = V(("SP", fi.start), 1, 0)
        entry.regs[0] = const(0)
        states = {fi.start: entry}
        work = [fi.start]
        seen_count = defaultdict(int)
        jumptables = {}
        while work:
            b = work.pop()
            st = states[b].copy()
            seen_count[b] += 1
            if seen_count[b] > 60:
                continue
            for succ, sst in self.block(fi, b, st, leaders, None, jumptables):
                if not (fi.start <= succ < fi.end) or succ not in insns:
                    continue
                old = states.get(succ)
                new = sst if old is None else old.join(sst)
                if old is None or new.key() != old.key():
                    states[succ] = new
                    if succ not in work:
                        work.append(succ)
        # final pass: record events with the fixpoint states
        for b in sorted(states):
            self.block(fi, b, states[b].copy(), leaders, self.events, jumptables)

    def block(self, fi, b, st, leaders, ev, jumptables):
        pc = b
        insns = fi.insns
        out = []
        while True:
            if pc != b and pc in leaders:
                out.append((pc, st))
                return out
            i = insns.get(pc)
            if i is None:
                return out
            if i.kind in ("branch",) or i.op in ("j", "jal", "jalr", "jr"):
                if i.op == "jal" or i.op == "jalr" or (i.kind == "branch" and i.link):
                    self.step(fi, pc + 4, insns.get(pc + 4), st, ev)
                    self.call(fi, pc, i, st, ev)
                    out.append((pc + 8, st))
                    return out
                if i.op == "jr":
                    self.step(fi, pc + 4, insns.get(pc + 4), st, ev)
                    if i.rs == GP_RA:
                        if ev is not None:
                            ev.append(("ret", fi.start, pc, {r: st.regs[r] for r in self.returns_of(fi.start)}))
                        return out
                    # jump table
                    v = st.regs[i.rs]
                    targets = jumptables.get(pc)
                    if targets is None:
                        targets = self.jumptable(fi, v)
                        jumptables[pc] = targets
                    for t in targets:
                        out.append((t, st.copy()))
                    return out
                if i.op == "j":
                    self.step(fi, pc + 4, insns.get(pc + 4), st, ev)
                    out.append((jal_target(pc, i), st))
                    return out
                # conditional branch
                target = pc + 4 + (i.imm << 2)
                if i.likely:
                    before = st.copy()
                    self.step(fi, pc + 4, insns.get(pc + 4), st, ev)
                    out.append((target, st))
                    if not always_taken(i):
                        out.append((pc + 8, before))
                else:
                    self.step(fi, pc + 4, insns.get(pc + 4), st, ev)
                    out.append((target, st))
                    if not always_taken(i):
                        out.append((pc + 8, st.copy()))
                return out
            self.step(fi, pc, i, st, ev)
            if i.op in ("break", "syscall") and False:
                return out
            pc += 4
            if pc >= fi.end:
                return out

    def jumptable(self, fi, v):
        if v is None or v[0] is None or v[0][0] != "L":
            return []
        loc = v[0][1]
        if loc[0] != "G":
            return []
        base = loc[1]
        # the index may be biased (x - 1): find the first entry
        for j in range(0, 300):
            w = self.prog.read32(base + 4 * j)
            if w is not None and fi.start <= w < fi.end:
                base += 4 * j
                break
        out = []
        for n in range(512):
            w = self.prog.read32(base + 4 * n)
            if w is None or not (fi.start <= w < fi.end):
                break
            out.append(w)
        return out

    def call(self, fi, pc, i, st, ev):
        if i.op == "jal":
            target = jal_target(pc, i)
        elif i.kind == "branch":
            target = pc + 4 + (i.imm << 2)
        else:
            target = None
        if ev is not None:
            regs = {r: st.regs[r] for r in range(1, 32) if st.regs[r] is not None and r not in (GP_RA,)}
            sp = st.regs[GP_SP]
            stk = {}
            if sp is not None and sp[0] == ("SP", fi.start) and sp[3] == 0:
                for off in range(0x10, 0x60, 4):
                    slot = sp[2] + off
                    if slot in st.frame:
                        stk[off] = st.frame[slot][0]
            ev.append(("call", fi.start, pc, target, regs, stk))
        passed = {}
        for r in (4, 5, 6, 7, 2, 3, 8, 9, 10, 11, 12):
            v = st.regs[r]
            if v is not None and v[0] is not None and v[0][0] == "SP" and v[1] == 1:
                st.esc = frozenset([v[2]]) if st.esc is None else st.esc | {v[2]}
                if v[3] == 0:
                    passed.setdefault(v[2], r)
        # the callee's results; the rest of what it changes is garbage
        rets = self.returns_of(target) if target is not None else ()
        for r in self.clobber_of(target):
            if r in (GP_SP, 0):
                continue
            st.regs[r] = V(("R", target, r), 1, 0) if r in rets else None
        st.lo = None
        # frame slots a callee may write
        sp = st.regs[GP_SP]
        if st.esc is not None:
            # a local whose address escaped: assume the callee may write
            # the 0x40 bytes from there
            for k in [k for k in st.frame if any(e <= k < e + 0x40 for e in st.esc)]:
                del st.frame[k]
        # a local passed by address holds what the callee stored through it
        if target is not None:
            for off, r in passed.items():
                st.frame[off] = (V(("L", ("*", ("A", target, r), 0, 0)), 1, 0), 4)
        if sp is not None and sp[0] == ("SP", fi.start):
            for k in [k for k in st.frame if sp[2] <= k < sp[2] + 0x10]:
                del st.frame[k]

    def step(self, fi, pc, i, st, ev):
        if i is None:
            return
        R = st.regs
        U = (("U", pc), 1, 0, 0, None)
        op = i.op
        rs, rt, rd = i.rs, i.rt, i.rd
        rel = self.prog.relhi.get(pc)
        reltag = None
        if rel is not None:
            reltag = "ram" if RAM_LO <= rel < RAM_HI else ("seg" if 0x01000000 <= rel < 0x10000000 else "rom")

        def setr(r, v):
            if r != 0:
                R[r] = v

        k = i.kind
        if k == "load":
            base = R[rs]
            addr = v_add(base, const(i.imm)) if base is not None else None
            if base is not None and base[0] is None:
                addr = (None, 0, (base[2] + i.imm) & 0xFFFFFFFF, base[3], reltag or base[4])
            loc = self.locate(fi, addr)
            if ev is not None:
                ev.append(("acc", fi.start, pc, op, LOAD_W[op], op in SIGNED, False, loc, addr, None))
            if op in ("lwc1", "ldc1"):
                return
            if op in ("lwl", "lwr", "ldl", "ldr"):
                setr(rt, U)
                return
            if loc is not None and loc[0] == "S":
                slot = loc[2]
                if loc[3] == 0 and slot in st.frame:
                    v, w = st.frame[slot]
                    if w == LOAD_W[op] or (w >= LOAD_W[op] and is_const(v)):
                        setr(rt, v)
                        return
                if loc[3] == 0 and slot >= 0 and loc[1] == fi.start:
                    setr(rt, V(("A", fi.start, "stk%x" % slot), 1, 0) if LOAD_W[op] >= 4 else U)
                    return
                setr(rt, U)
                return
            if loc is None:
                setr(rt, U)
                return
            w = LOAD_W[op]
            if w >= 4:
                setr(rt, V(("L", loc), 1, 0))
            else:
                setr(rt, V(("I", loc), 1, 0))
            return
        if k == "store":
            base = R[rs]
            addr = v_add(base, const(i.imm)) if base is not None else None
            if base is not None and base[0] is None:
                addr = (None, 0, (base[2] + i.imm) & 0xFFFFFFFF, base[3], reltag or base[4])
            loc = self.locate(fi, addr)
            val = None if op in ("swc1", "sdc1") else R[rt]
            if ev is not None:
                ev.append(("acc", fi.start, pc, op, STORE_W[op], None, True, loc, addr, val))
            if val is not None and val[0] is not None and val[0][0] == "SP" and val[1] == 1 \
                    and not (loc is not None and loc[0] == "S"):
                # a stack address escapes
                st.esc = frozenset([val[2]]) if st.esc is None else st.esc | {val[2]}
            if loc is not None and loc[0] == "S" and loc[1] == fi.start:
                w = STORE_W[op]
                slot = loc[2]
                if loc[3] != 0:
                    return
                for kk in [kk for kk in st.frame if kk < slot + w and kk + st.frame[kk][1] > slot]:
                    del st.frame[kk]
                if op not in ("swc1", "sdc1", "swl", "swr", "sdl", "sdr"):
                    st.frame[slot] = (val, w)
            return
        if op == "lui":
            setr(rt, const(i.uimm << 16, reltag))
            return
        if op in ("addiu", "addi", "daddiu", "daddi"):
            x = R[rs]
            if x is not None and x[0] is None:
                setr(rt, (None, 0, (x[2] + i.imm) & 0xFFFFFFFF, x[3], x[4] or reltag))
            else:
                setr(rt, v_add(x, const(i.imm)) if x is not None else None)
            return
        if op == "ori":
            x = R[rs]
            if is_const(x):
                setr(rt, const(x[2] | i.uimm, x[4] or reltag))
            elif x is not None and i.uimm == 0:
                setr(rt, x)
            else:
                setr(rt, U)
            return
        if op == "andi":
            x = R[rs]
            setr(rt, const(x[2] & i.uimm) if is_const(x) else U)
            return
        if op in ("xori", "slti", "sltiu"):
            setr(rt, U)
            return
        if op in ("addu", "add", "daddu", "dadd", "or"):
            x, y = R[rs], R[rt]
            if op == "or":
                if rt == 0:
                    setr(rd, x)
                    return
                if rs == 0:
                    setr(rd, y)
                    return
                if is_const(x) and is_const(y):
                    setr(rd, const(x[2] | y[2]))
                    return
                setr(rd, U)
                return
            if rt == 0:
                setr(rd, x)
                return
            if rs == 0:
                setr(rd, y)
                return
            setr(rd, v_add(x, y))
            return
        if op in ("subu", "sub", "dsubu", "dsub"):
            setr(rd, v_add(R[rs], v_neg(R[rt])))
            return
        if op == "and":
            x, y = R[rs], R[rt]
            if is_const(x) and is_const(y):
                setr(rd, const(x[2] & y[2]))
            elif (is_const(y) and y[2] == 0x1FFFFFFF and x is not None) or \
                    (is_const(x) and x[2] == 0x1FFFFFFF and y is not None):
                p = x if not is_const(x) else y
                setr(rd, (p[0], p[1], p[2], p[3], "phys"))
            else:
                setr(rd, U)
            return
        if op in ("sll", "dsll"):
            x = R[rt]
            if i.word == 0:
                return
            setr(rd, v_mul(x, 1 << i.sa) if x is not None else U)
            return
        if op == "dsll32":
            setr(rd, U)
            return
        if op in ("sra", "srl", "dsra", "dsrl", "dsra32", "dsrl32"):
            x = R[rt]
            if is_const(x):
                if op == "srl":
                    setr(rd, const(x[2] >> i.sa))
                elif op == "sra":
                    v = x[2] - (1 << 32) if x[2] & 0x80000000 else x[2]
                    setr(rd, const(v >> i.sa))
                else:
                    setr(rd, U)
            else:
                setr(rd, U)
            return
        if op in ("mult", "multu", "dmult", "dmultu"):
            x, y = R[rs], R[rt]
            if is_const(y) and x is not None:
                c = y[2] if y[2] < 0x80000000 else y[2] - (1 << 32)
                st.lo = v_mul(x, c)
            elif is_const(x) and y is not None:
                c = x[2] if x[2] < 0x80000000 else x[2] - (1 << 32)
                st.lo = v_mul(y, c)
            else:
                st.lo = None
            return
        if op == "mflo":
            setr(rd, st.lo)
            return
        if op in ("div", "divu", "ddiv", "ddivu", "mthi", "mtlo"):
            st.lo = None
            return
        if op == "mfhi":
            setr(rd, U)
            return
        if k == "cop1":
            if op in ("mfc1", "dmfc1", "cfc1"):
                setr(rt, U)
            return
        if k == "cop0":
            if op in ("mfc0", "dmfc0"):
                setr(rt, U)
            return
        d = defs(i)
        if d is not None:
            setr(d, U)

    def locate(self, fi, addr):
        """A location key for an access at the abstract address."""
        if addr is None:
            return None
        o, a, k, s, tag = addr
        if o is None:
            if RAM_LO <= k < RAM_HI or 0xA0000000 <= k < 0xC0000000:
                return ("G", k, s)
            return None
        if a != 1:
            return None
        if o[0] == "SP":
            return ("S", o[1], k, s)
        if o[0] in ("I", "U"):
            return None
        return ("*", o, k, s)


def defs(i):
    """The GPR an instruction writes, or None."""
    op = i.op
    if i.kind == "load":
        return None if op in ("lwc1", "ldc1") else i.rt
    if i.kind in ("store", "branch", "trap", "misc"):
        return 31 if (i.kind == "branch" and i.link) else None
    if op in ("jal", "jalr"):
        return 31 if op == "jal" else i.rd
    if op in ("j", "jr"):
        return None
    if i.kind == "cop1":
        return i.rt if op in ("mfc1", "dmfc1", "cfc1") else None
    if i.kind == "cop0":
        return i.rt if op in ("mfc0", "dmfc0") else None
    if op in ("mult", "multu", "dmult", "dmultu", "div", "divu", "ddiv", "ddivu", "mthi", "mtlo"):
        return None
    if op in ("lui", "addiu", "addi", "daddiu", "daddi", "andi", "ori", "xori", "slti", "sltiu"):
        return i.rt
    if op in ("sync",):
        return None
    return i.rd


def jal_target(pc, i):
    return ((pc + 4) & 0xF0000000) | (i.target << 2)


def always_taken(i):
    return i.op in ("beq", "beql") and i.rs == 0 and i.rt == 0 or (i.op == "bgez" and i.rs == 0)


# ---------------------------------------------------------------- slots

class UF:
    def __init__(self):
        self.p = {}

    def find(self, x):
        p = self.p
        if x not in p:
            p[x] = x
            return x
        r = x
        while p[r] != r:
            r = p[r]
        while p[x] != r:
            p[x], x = r, p[x]
        return r

    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra != rb:
            self.p[ra] = rb


def origin_slots(o):
    """The slots a value's origin stands for."""
    if o is None:
        return ()
    if o[0] == "M":
        out = []
        for m in o[1]:
            out.extend(origin_slots(m))
        return out
    if o[0] == "L":
        return [o[1]]
    if o[0] in ("A", "R"):
        return [o]
    return ()


def ptr_origin(v):
    """The origin of a value that can be a pointer (a == 1), or None."""
    if v is None or v[0] is None or v[1] != 1:
        return None
    if v[0][0] in ("I", "U", "SP"):
        return None
    return v[0]


class Results:
    """Everything the events say about slots, and the pointer verdicts."""

    def __init__(self, prog, an):
        self.prog = prog
        self.an = an
        self.funcname = {fi.start: fi.name for fi in an.infos.values()}
        self.deref = defaultdict(set)        # slot -> {pc} where a value from it is dereferenced
        self.addr_in = defaultdict(set)      # slot -> {(tag, value, stride)} address constants stored
        self.int_in = defaultdict(set)       # slot -> {nonzero integer constants stored}
        self.phys_in = defaultdict(set)      # slot -> {pc}: stores of K0_TO_PHYS(pointer)
        self.stack_in = defaultdict(set)     # slot -> {pc}: stores of a stack address
        self.flows = defaultdict(set)        # src slot -> {(dst slot, k)}: dst = src + k (k None: + index)
        self.rflows = defaultdict(set)       # dst -> {(src, k)}
        self.stores = defaultdict(set)       # slot -> {pc} stores into it (memory slots)
        self.used = defaultdict(set)         # func -> entry registers / stack args it really uses
        # the allocator's pointers start at fixed addresses; what they point
        # to is the heap, not a variable there
        self.heap_slots = {("G", prog.named[n], 0) for n in HEAP_POINTERS if n in prog.named}
        self._used_args()
        self._collect()

    # which entry registers a function uses as inputs
    def _used_args(self):
        an = self.an
        direct = defaultdict(set)
        passes = defaultdict(set)   # (callee, reg) -> {(caller, reg)}
        for e in an.events:
            if e[0] == "acc":
                _, f, pc, op, w, signed, store, loc, addr, val = e
                o = ptr_origin(addr)
                if o is not None:
                    for m in _members(o):
                        if m[0] == "A":
                            direct[m[1]].add(m[2])
                if store and loc is not None and loc[0] != "S" and val is not None and val[0] is not None:
                    for m in _members(val[0]) if val[0][0] in ("A", "M") else ():
                        if m[0] == "A":
                            direct[m[1]].add(m[2])
            elif e[0] == "call":
                _, f, pc, target, regs, stk = e
                if target is None:
                    for r, v in regs.items():
                        if r in (4, 5, 6, 7) and v is not None and v[0] is not None and v[0][0] in ("A", "M"):
                            for m in _members(v[0]):
                                if m[0] == "A":
                                    direct[m[1]].add(m[2])
                    continue
                for r, v in list(regs.items()) + [("stk%x" % o, v) for o, v in stk.items()]:
                    if v is not None and v[0] is not None and v[0][0] in ("A", "M"):
                        for m in _members(v[0]):
                            if m[0] == "A":
                                passes[(target, r)].add((m[1], m[2]))
            elif e[0] == "ret":
                _, f, pc, regs = e
                for r, v in regs.items():
                    if v is not None and v[0] is not None and v[0][0] in ("A", "M"):
                        for m in _members(v[0]):
                            if m[0] == "A" and not (m[1] == f and m[2] == r):
                                direct[m[1]].add(m[2])
        used = defaultdict(set)
        work = []
        for f, rs in direct.items():
            for r in rs:
                used[f].add(r)
                work.append((f, r))
        while work:
            f, r = work.pop()
            for g, r2 in passes.get((f, r), ()):
                if r2 not in used[g]:
                    used[g].add(r2)
                    work.append((g, r2))
        self.used = used

    def _collect(self):
        an = self.an
        fname = self.funcname
        for e in an.events:
            if e[0] == "acc":
                _, f, pc, op, w, signed, store, loc, addr, val = e
                o = ptr_origin(addr)
                if o is not None:
                    for s in origin_slots(o):
                        self.deref[s].add(pc)
                if store and loc is not None and loc[0] != "S":
                    self.stores[loc].add(pc)
                    self.store_value(loc, val, w, pc)
            elif e[0] == "ret":
                _, f, pc, regs = e
                for r, v in regs.items():
                    self.store_value(("R", f, r), v, 4, pc, ret_of=(f, r))
            elif e[0] == "call":
                _, f, pc, target, regs, stk = e
                if target is None:
                    continue
                tname = fname.get(target, "")
                generic = GENERIC_ARGS.get(tname, set())
                used = self.used.get(target, set())
                for r, v in regs.items():
                    if r in used and r not in generic:
                        self.store_value(("A", target, r), v, 4, pc)
                if tname in VARARGS:
                    continue
                for off, v in stk.items():
                    key = "stk%x" % off
                    if key in used:
                        self.store_value(("A", target, key), v, 4, pc)

    def store_value(self, dst, v, w, pc, ret_of=None):
        if v is None or w != 4:
            return
        o, a, k, s, tag = v
        if o is None:
            if tag in ("rom", "seg") and not s:
                self.addr_in[dst].add((tag, k, 0))
            elif RAM_LO <= k < RAM_HI and (tag == "ram" or (k != 0x80000000 and not s)):
                self.addr_in[dst].add(("ram", k, s))
            elif tag == "phys":
                self.phys_in[dst].add(pc)
            elif k != 0 and not s:
                self.int_in[dst].add(k)
            return
        if tag == "phys":
            self.phys_in[dst].add(pc)
            for src in origin_slots(o):
                self.deref[src].add(("phys", pc))
            return
        if a != 1:
            return
        if o[0] == "SP":
            self.stack_in[dst].add(pc)
            return
        if o[0] in ("I", "U"):
            return
        for src in origin_slots(o):
            if src == dst:
                continue
            if ret_of is not None and src == ("A", ret_of[0], ret_of[1]):
                continue   # a register a function leaves as it was
            kk = k if not s else None
            self.flows[src].add((dst, kk))
            self.rflows[dst].add((src, kk))

    # ---- concrete locations

    def concrete(self, slot, _busy=None):
        """What a slot's value may be: {("G", addr, stride)} for addresses of
        globals, {("P", memslot, delta)} for a pointer read from memory."""
        memo = self.__dict__.setdefault("_conc", {})
        if slot in memo:
            return memo[slot]
        busy = _busy if _busy is not None else set()
        if slot in busy:
            return set()
        busy.add(slot)
        out = set()
        if slot not in self.heap_slots:
            for tag, k, s in self.addr_in.get(slot, ()):
                if tag == "ram":
                    out.add(("G", k, s))
        if slot[0] in ("G", "*"):
            for m in self.canon(slot, busy):
                out.add(("P", m, 0))
        else:
            for src, k in self.rflows.get(slot, ()):
                if src in self.heap_slots:
                    # a fresh allocation: known by the argument or result
                    # that carries it
                    out.add(("P", slot, 0))
                    continue
                for b in self.concrete(src, busy):
                    if k is None:
                        if b[0] == "G":
                            out.add(("G", b[1], gcd(b[2], 1)))
                        else:
                            out.add((b[0], b[1], None))
                    elif b[0] == "G":
                        out.add(("G", (b[1] + k) & 0xFFFFFFFF, b[2]))
                    else:
                        out.add(("P", b[1], None if b[2] is None else b[2] + k))
                if len(out) > 64:
                    break
        busy.discard(slot)
        if _busy is None or not busy:
            memo[slot] = out
        return out

    def canon(self, loc, _busy=None):
        """The concrete locations a location stands for: ("G", addr, stride)
        or ("*", ("L", memslot), off, stride) with memslot concrete."""
        memo = self.__dict__.setdefault("_canon", {})
        if loc in memo:
            return memo[loc]
        if loc[0] == "G":
            return {loc}
        if loc[0] != "*":
            return set()
        busy = _busy if _busy is not None else set()
        key = ("canon", loc)
        if key in busy:
            return set()
        busy.add(key)
        _, o, off, s = loc
        out = set()
        for sl in origin_slots(o):
            for b in self.concrete(sl, busy):
                if b[0] == "G":
                    out.add(("G", (b[1] + off) & 0xFFFFFFFF, gcd(b[2], s)))
                elif b[2] is None:
                    out.add(("*", ("L", b[1]), off, gcd(s, 1)))
                else:
                    out.add(("*", ("L", b[1]), off + b[2], s))
            if len(out) > 64:
                break
        busy.discard(key)
        if _busy is None or not busy:
            memo[loc] = out
        return out

    # ---- pointer verdicts

    def classify(self):
        """Propagate pointer evidence: forward along flows (a slot that
        receives a pointer holds one) and backward from dereferences (what
        flows into a dereferenced slot is dereferenced)."""
        slots = set(self.deref) | set(self.addr_in) | set(self.flows) | set(self.rflows) \
            | set(self.int_in) | set(self.phys_in) | set(self.stack_in) | set(self.stores)
        self.allslots = slots
        kind = defaultdict(set)
        why = {}
        for s in slots:
            if self.deref.get(s):
                kind[s].add("deref")
                why[(s, "deref")] = ("deref", sorted(p for p in self.deref[s] if isinstance(p, int))[:3])
            for tag, k, _ in self.addr_in.get(s, ()):
                kind[s].add(tag)
                why.setdefault((s, tag), ("const", k))
            if self.stack_in.get(s):
                kind[s].add("stack")
                why[(s, "stack")] = ("stack", sorted(self.stack_in[s])[:3])
            if self.phys_in.get(s):
                kind[s].add("phys")
                why[(s, "phys")] = ("phys", sorted(self.phys_in[s])[:3])
        # canonical aliases share their evidence
        alias = defaultdict(set)
        for s in slots:
            if s[0] == "*":
                for c in self.canon(s):
                    alias[c].add(s)
        for rnd in range(4):
            # backward: what flows into a dereferenced slot is dereferenced
            work = [s for s in list(kind) if "deref" in kind[s]]
            while work:
                s = work.pop()
                for src, _ in self.rflows.get(s, ()):
                    if "deref" not in kind[src]:
                        kind[src].add("deref")
                        why[(src, "deref")] = ("flows into", s)
                        work.append(src)
            # forward: a slot that receives a pointer holds one
            for tag in ("deref", "ram", "stack", "rom", "seg", "phys"):
                ftag = "ram" if tag == "deref" else tag
                work = [s for s in list(kind) if tag in kind[s]]
                while work:
                    s = work.pop()
                    for dst, _ in self.flows.get(s, ()):
                        kd = kind[dst]
                        if ftag not in kd and tag not in kd:
                            kd.add(ftag)
                            why[(dst, ftag)] = ("receives from", s)
                            work.append(dst)
            # canonical aliases share their evidence
            changed = False
            for c, al in alias.items():
                u = set(kind.get(c, set()))
                for a in al:
                    u |= kind.get(a, set())
                for a in list(al) + [c]:
                    for t in u - kind[a]:
                        kind[a].add(t)
                        src = next((x for x in list(al) + [c] if t in kind.get(x, ()) and x != a), None)
                        why.setdefault((a, t), ("alias of", src))
                        changed = True
            if not changed:
                break
        self.kind = kind
        self.why = why
        # offsets: a word added to the pointer to the record it sits in
        self.offsets = set()
        for ox, oy in ADDPAIRS:
            for a, b in ((ox, oy), (oy, ox)):
                for sa in origin_slots(a):
                    for sb in origin_slots(b):
                        if sa[0] == "*" and sb in origin_slots(sa[1]):
                            self.offsets.add(sa)
        for sl in list(self.offsets):
            for c in self.canon(sl):
                self.offsets.add(c)

    def explain(self, s, tag=None, depth=12):
        """The chain of evidence for a slot's verdict."""
        out = []
        seen = set()
        while s is not None and depth > 0 and s not in seen:
            seen.add(s)
            k = self.kind.get(s, set())
            t = tag if tag in k else next((x for x in ("deref", "ram", "stack", "rom", "seg", "phys") if x in k), None)
            if t is None:
                break
            w = self.why.get((s, t))
            out.append((s, t, w))
            if w is None or w[0] in ("deref", "const", "stack", "phys"):
                break
            s = w[1]
            tag = "deref" if w[0] == "flows into" else None
            depth -= 1
        return out

    def verdict(self, s):
        k = self.kind.get(s, ())
        if s in self.offsets and "ram" not in {t for t, _, _ in self.addr_in.get(s, ())}:
            return "offset"
        if not k:
            return None
        if k & {"deref", "ram", "stack"}:
            return "ptr"
        if "rom" in k:
            return "rom"
        if "seg" in k:
            return "seg"
        if "phys" in k:
            return "phys"
        return None


def fmt_slot(prog, res, s, depth=0):
    if s is None:
        return "?"
    if s[0] == "G":
        return fmt_addr(prog, s[1], s[2])
    if s[0] == "*":
        o = s[1]
        inner = fmt_origin(prog, res, o, depth + 1) if depth < 4 else "..."
        st = " [stride 0x%X]" % s[3] if s[3] else ""
        return "(*%s)+0x%X%s" % (inner, s[2], st) if s[2] >= 0 else "(*%s)-0x%X%s" % (inner, -s[2], st)
    if s[0] == "A":
        return "arg(%s,%s)" % (res.funcname.get(s[1], "%08X" % s[1]), regname(s[2]))
    if s[0] == "R":
        return "ret(%s,%s)" % (res.funcname.get(s[1], "%08X" % (s[1] or 0)), regname(s[2]))
    if s[0] == "S":
        return "stack"
    return str(s)


def fmt_origin(prog, res, o, depth):
    if o[0] in ("L", "I"):
        return fmt_slot(prog, res, o[1], depth)
    return fmt_slot(prog, res, o, depth)


def regname(r):
    return mips.GPR_NAMES[r] if isinstance(r, int) else r


def fmt_addr(prog, addr, stride=0):
    name, start = prog.sym_containing(addr)
    st = " [stride 0x%X]" % stride if stride else ""
    if name is None:
        return "0x%08X%s" % (addr, st)
    if addr == start:
        return name + st
    return "%s+0x%X%s" % (name, addr - start, st)


def main():
    # set iteration order decides some tie-breaks (and the analysis's cycle
    # cuts): fix the hash seed so every run writes the same thing
    if os.environ.get("PYTHONHASHSEED") != "0":
        os.environ["PYTHONHASHSEED"] = "0"
        os.execv(sys.executable, [sys.executable] + sys.argv)
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--version", default="us.v11")
    ap.add_argument("--out", default=os.path.join(ROOT, "blastcorps", "build", "fieldscan"))
    ap.add_argument("--dumps", nargs="*", default=[],
                    help="RDRAM snapshots (PORT_DUMP) to check the verdicts against")
    args = ap.parse_args()
    DUMPS.extend(args.dumps)
    prog = Program(args.version)
    an = Analyzer(prog)
    an.run()
    res = Results(prog, an)
    res.classify()
    FUNCNAMES.update(res.funcname)
    os.makedirs(args.out, exist_ok=True)
    regions, elem, kinds = write_text(prog, an, res, args.out)
    write_types(prog, an, res, args.out, elem, kinds)


def write_reports(prog, an, res, out):
    """Group the accesses by concrete location and write the reports."""
    finfo = {fi.start: fi for fi in an.infos.values()}
    fields = defaultdict(lambda: {"w": defaultdict(int), "ld": 0, "st": 0, "files": set(),
                                  "pcs": [], "slots": set(), "stride": set()})
    rows = []
    for e in an.events:
        if e[0] != "acc":
            continue
        _, f, pc, op, w, signed, store, loc, addr, val = e
        if loc is None or loc[0] == "S":
            continue
        fi = finfo[f]
        locs = [loc] if loc[0] == "G" else sorted(res.canon(loc), key=str)
        if not locs:
            locs = [("?", loc)]
        for c in locs:
            key = region_key(prog, c)
            if key is None:
                continue
            d = fields[key]
            t = op_type(op, signed)
            d["w"][t] += 1
            d["st" if store else "ld"] += 1
            d["files"].add(("asm:" if fi.kind == "asm" else "c:") + fi.file.split("/")[-1])
            if len(d["pcs"]) < 6:
                d["pcs"].append(pc)
            d["slots"].add(loc)
            if c[0] in ("G", "*") and c[-1]:
                d["stride"].add(c[-1])
        rows.append((pc, fi.name, fi.file, op, w, loc))
    # pointer verdicts per canonical location
    verdict = defaultdict(set)
    for s in res.allslots:
        v = res.verdict(s)
        if v is None:
            continue
        cs = [s] if s[0] == "G" else (res.canon(s) if s[0] == "*" else [])
        for c in cs:
            key = region_key(prog, c)
            if key is not None:
                verdict[key].add(v)
    # static pointers: relocations in initialized data
    static = {}
    for addr, value, sym in prog.data_relocs:
        if any(lo <= addr < hi for lo, hi, m, sec in prog.datasecs):
            key = region_key(prog, ("G", addr, 0))
            if key is not None:
                static[key] = (value, sym)
    return fields, verdict, static


def op_type(op, signed):
    if op in ("lwc1", "swc1"):
        return "f32"
    if op in ("ldc1", "sdc1"):
        return "f64"
    w = LOAD_W.get(op) or STORE_W.get(op)
    if op in ("lwl", "lwr", "swl", "swr"):
        return "u32?"
    if signed is None:
        return {1: "8", 2: "16", 4: "32", 8: "64"}[w]
    return ("s" if signed else "u") + {1: "8", 2: "16", 4: "32", 8: "64"}[w]


def region_key(prog, c):
    """(region name, offset within an element, element stride) of a concrete
    location: a global (by the sized symbol or data symbol it is in) or what
    a memory slot points to."""
    if c[0] == "G":
        addr, s = c[1], c[2]
        base, start = prog.var_of(addr)
        if base is None:
            return None
        off = addr - start
        return (base, off, s)
    if c[0] == "*":
        o = c[1]
        if o[0] != "L":
            return None
        inner = o[1]
        if inner[0] in ("A", "R"):
            return (("*", ("new", inner)), c[2], c[3])
        ik = region_key(prog, inner) if inner[0] in ("G", "*") else None
        if ik is None:
            return None
        return (("*",) + ik, c[2], c[3])
    return None


FUNCNAMES = {}
DUMPS = []
DYN_CACHE = {}     # field name -> what the RDRAM dumps showed (tools/inventory.py keeps it)


def field_width(t):
    return {"8": 1, "16": 2, "32": 4, "64": 8, "f32": 4, "f64": 8, "u32?": 4}.get(t.lstrip("su"), 0)


class Dynamic:
    """RDRAM snapshots (the port's PORT_DUMP files) as a check on the static
    verdicts: what a field actually held while the game ran."""

    def __init__(self, prog, paths):
        self.prog = prog
        self.dumps = [open(p, "rb").read() for p in paths]

    @staticmethod
    def rd(d, a):
        o = a - 0x80000000
        if 0 <= o <= len(d) - 4 and not a & 3:
            return struct.unpack_from(">I", d, o)[0]
        return None

    def bases(self, region, d, depth=0):
        if depth > 6:
            return []
        if isinstance(region, str):
            a = self.prog.named.get(region)
            return [] if a is None else [a]
        if region[0] == "*" and region[1][0] == "new":
            return []
        if region[0] == "*":
            inner, ioff, istride = region[1], region[2], region[3]
            out = []
            for b in self.bases(inner, d, depth + 1):
                addrs = [b + ioff]
                if istride and isinstance(inner, str):
                    size = self.prog.var_size(b)
                    addrs = list(range(b + ioff % istride, b + size, istride))[:64]
                for a in addrs:
                    v = self.rd(d, a)
                    if v is not None and RAM_LO <= v < RAM_HI:
                        out.append(v)
            return out[:64]
        return []

    def field(self, region, off, elem):
        """'ptr' if the word only ever held RAM addresses (or 0), 'int' if it
        held something else, '' if it was never seen nonzero."""
        vals = []
        for d in self.dumps:
            for b in self.bases(region, d):
                if isinstance(region, str) and elem:
                    size = self.prog.var_size(b)
                    addrs = range(b + off % elem, b + max(size, off + 4), elem)
                else:
                    addrs = [b + off]
                for a in list(addrs)[:256]:
                    if 0x8021ED00 <= a < 0x80224D80:
                        continue   # init's memory, reused once the game runs
                    v = self.rd(d, a)
                    if v is not None:
                        vals.append(v)
        nz = [v for v in vals if v]
        if not nz:
            return ""
        if all(RAM_LO <= v < RAM_HI for v in nz):
            return "ptr"
        return "int"


def region_name(r):
    if r[0] == "*" and r[1][0] == "new":
        sl = r[1][1]
        return "new(%s %s,%s)" % ("arg" if sl[0] == "A" else "ret", FUNCNAMES.get(sl[1], "%08X" % (sl[1] or 0)),
                                  regname(sl[2]))
    if r[0] == "*":
        inner = r[1:]
        return "*(%s)" % field_name(inner[0], inner[1], inner[2])
    return r


def field_name(region, off, stride):
    base = region_name(region)
    st = "[0x%X]" % stride if stride else ""
    return "%s%s+0x%X" % (base, st, off) if off >= 0 else "%s%s-0x%X" % (base, st, -off)


def write_text(prog, an, res, out):
    fields, verdict, static = write_reports(prog, an, res, out)
    # element size per region: the most common stride of 8 or more
    strides = defaultdict(lambda: defaultdict(int))
    for (region, off, s), d in fields.items():
        if s and s >= 8:
            strides[region][s] += d["ld"] + d["st"]
    elem = {r: max(c, key=lambda s: (c[s], s)) for r, c in strides.items()}
    agg = defaultdict(lambda: {"w": defaultdict(int), "ld": 0, "st": 0, "files": set(), "pcs": [],
                               "kind": set()})
    for (region, off, s), d in fields.items():
        e = elem.get(region)
        fo = off % e if e and (not s or s % e == 0 or e % max(s, 1) == 0) else off
        a = agg[(region, fo)]
        for t, n in d["w"].items():
            a["w"][t] += n
        a["ld"] += d["ld"]
        a["st"] += d["st"]
        a["files"] |= d["files"]
        a["pcs"] += d["pcs"][:3]
        a["kind"] |= verdict.get((region, off, s), set())
    for (region, off, s), v in verdict.items():
        e = elem.get(region)
        fo = off % e if e else off
        if (region, fo) in agg:
            agg[(region, fo)]["kind"] |= v
    for (region, off, s), (value, sym) in static.items():
        e = elem.get(region)
        fo = off % e if e else off
        agg[(region, fo)]["kind"].add("static")
    regions = defaultdict(list)
    for (region, off), a in agg.items():
        regions[region].append((off, a))
    dyn = Dynamic(prog, DUMPS) if DUMPS else None
    with open(os.path.join(out, "fields.tsv"), "w") as f:
        f.write("# region\toffset\telement\ttypes\tkind\tloads\tstores\tfiles\tdynamic\n")
        for region in sorted(regions, key=lambda r: str(r)):
            e = elem.get(region, 0)
            for off, a in sorted(regions[region], key=lambda x: x[0]):
                types = ",".join("%s:%d" % (t, n) for t, n in sorted(a["w"].items()))
                kind = ",".join(sorted(a["kind"]))
                dk = dyn.field(region, off, e) if dyn and max([field_width(t) for t in a["w"]] or [4]) >= 4 else \
                    DYN_CACHE.get(field_name(region, off, e), "")
                a["dyn"] = dk
                f.write("%s\t0x%X\t0x%X\t%s\t%s\t%d\t%d\t%s\t%s\n" % (
                    region_name(region), off, e, types, kind, a["ld"], a["st"],
                    " ".join(sorted(a["files"])), dk))
    kinds = {key: a["kind"] for key, a in agg.items()}
    return regions, elem, kinds




def c_type(types, kinds):
    """(C type, size) of a field from how it is accessed."""
    if "ptr" in kinds:
        return ("void *", 4)
    if "offset" in kinds:
        return ("AssetOffset", 4)
    if "rom" in kinds:
        return ("RomAddr", 4)
    if "seg" in kinds:
        return ("SegAddr", 4)
    width = 0
    for t in types:
        w = {"8": 1, "16": 2, "32": 4, "64": 8, "f32": 4, "f64": 8, "u32?": 4}.get(t.lstrip("su"), 0)
        width = max(width, w)
    if any(t == "f64" for t in types) and width == 8:
        return ("f64", 8)
    if any(t == "f32" for t in types) and width == 4:
        return ("f32", 4)
    signed = sum(n for t, n in types.items() if t.startswith("s"))
    unsigned = sum(n for t, n in types.items() if t.startswith("u") and t != "u32?")
    sign = "u" if unsigned > signed else "s"
    return ({1: sign + "8", 2: sign + "16", 4: sign + "32", 8: sign + "64"}.get(width, "u8"), width or 1)


def c_draft(fields, size=None):
    out = ["typedef struct {"]
    pos = 0
    for off, (ty, w), nview in fields:
        if off < 0:
            continue
        if off < pos:
            out.append("    /* 0x%02X    %s (overlaps) */" % (off, ty))
            continue
        if off > pos:
            out.append("    /* 0x%02X */ u8 pad%X[0x%X];" % (pos, pos, off - pos))
        align = min(w, 4) if ty not in ("f64", "s64", "u64") else 8
        if off % align:
            out.append("    /* 0x%02X */ u8 unk%X[%d]; /* %s, unaligned */" % (off, off, w, ty))
        else:
            out.append("    /* 0x%02X */ %s unk%X;" % (off, ty, off))
        pos = off + w
    if size and size > pos:
        out.append("    /* 0x%02X */ u8 pad%X[0x%X];" % (pos, pos, size - pos))
        pos = size
    out.append("} T; /* size 0x%X */" % pos)
    return "\n".join(out)


def write_types(prog, an, res, out, elem=None, kinds=None):
    """Structures by their instances: the concrete addresses (or pointed-to
    regions) that flow into the same argument or pointer slot are instances
    of one type.  Each type lists every field any view of it touches."""
    finfo = {fi.start: fi for fi in an.infos.values()}
    # views: slot -> {offset: {types}} for accesses through a pointer slot
    view_fields = defaultdict(lambda: defaultdict(lambda: {"t": defaultdict(int), "files": set()}))
    abs_fields = defaultdict(lambda: {"t": defaultdict(int), "files": set()})
    for e in an.events:
        if e[0] != "acc":
            continue
        _, f, pc, op, w, signed, store, loc, addr, val = e
        if loc is None or loc[0] == "S":
            continue
        fi = finfo[f]
        tag = ("asm:" if fi.kind == "asm" else "c:") + fi.file.split("/")[-1]
        t = op_type(op, signed)
        if loc[0] == "G":
            d = abs_fields[loc[1]]
            d["t"][t] += 1
            d["files"].add(tag)
        else:
            for sl in origin_slots(loc[1]):
                d = view_fields[sl][loc[2]]
                d["t"][t] += 1
                d["files"].add(tag)
    # instances: union the (base - delta) of every target of each view
    uf = UF()
    inst_of = defaultdict(set)
    heap = {("G", prog.named[n], 0) for n in HEAP_POINTERS if n in prog.named}
    for sl in view_fields:
        if sl[0] in ("A", "R"):
            fn = res.funcname.get(sl[1], "")
            if not fn.startswith("func_") or fn in GENERIC_FUNCS:
                continue   # libultra/libc and the game's generic helpers take anything
        conc = res.concrete(sl)
        if any(b[0] == "P" and b[1] in heap for b in conc):
            continue       # a pointer straight from the allocator
        tg = [b for b in conc if b[0] == "G" or (b[0] == "P" and b[2] is not None)]
        keys = []
        for b in tg:
            if b[0] == "G":
                keys.append(("G", b[1]))
            else:
                keys.append(("P", b[1], b[2]))
        for k in keys:
            uf.find(k)
            inst_of[sl].add(k)
        for a, b in zip(keys, keys[1:]):
            uf.union(a, b)
    classes = defaultdict(set)
    for sl, ks in inst_of.items():
        for k in ks:
            classes[uf.find(k)].add(k)
    views_of = defaultdict(set)
    for sl, ks in inst_of.items():
        for k in ks:
            views_of[uf.find(k)].add(sl)
    # verdicts by absolute address, strided slots spread over their variable
    gverdict = defaultdict(set)
    for sl in res.allslots:
        v = res.verdict(sl)
        if not v:
            continue
        for c in ([sl] if sl[0] == "G" else (res.canon(sl) if sl[0] == "*" else [])):
            if c[0] != "G":
                continue
            addr, st = c[1], c[2]
            if st and st >= 8:
                name, start = prog.var_of(addr)
                size = prog.var_size(start) if start is not None else 0
                if start is not None and size:
                    for a in range(start + (addr - start) % st, start + size, st):
                        gverdict[a].add(v)
                    continue
            if not st or st >= 8:
                gverdict[addr].add(v)
    lines = []
    drafts = []
    order = sorted(classes, key=lambda c: -sum(len(view_fields[s]) for s in views_of[c]))
    for c in order:
        insts = classes[c]
        views = views_of[c]
        fld = defaultdict(lambda: {"t": defaultdict(int), "files": set(), "ptr": set(), "via": set(),
                                   "nview": 0})
        for sl in views:
            for inst in inst_of[sl]:
                pass
            for off, d in view_fields[sl].items():
                x = fld[off]
                for t, n in d["t"].items():
                    x["t"][t] += n
                x["files"] |= d["files"]
                x["via"].add(fmt_slot(prog, res, sl))
                x["nview"] += 1
                v = res.verdict(("*", ("A",) + sl[1:], off, 0)) if sl[0] == "A" else None
                for ms in res.allslots:
                    pass
        # pointer verdicts of the fields, from the canonical locations
        ext = max(fld) + 8 if fld else 0
        gaddrs = sorted(i[1] for i in insts if i[0] == "G")
        gaps = [b - a for a, b in zip(gaddrs, gaddrs[1:]) if b > a]
        maxsize = min(gaps) if gaps else None
        if maxsize:
            ext = min(ext, maxsize)
        for inst in insts:
            if inst[0] == "G":
                for off in range(0, ext):
                    a = inst[1] + off
                    if a in abs_fields:
                        x = fld[off]
                        for t, n in abs_fields[a]["t"].items():
                            x["t"][t] += n
                        x["files"] |= abs_fields[a]["files"]
        names = sorted(fmt_addr(prog, i[1]) if i[0] == "G" else "*%s%+d" % (fmt_slot(prog, res, i[1]), i[2])
                       for i in insts)
        lines.append("== type: %d instances, %d views, %d fields, view extent 0x%X%s" % (
            len(insts), len(views), len(fld), max([o for o in fld if fld[o]["nview"]] or [0]),
            ", instances 0x%X apart" % maxsize if maxsize else ""))
        lines.append("   instances: " + " ".join(names[:40]) + (" ..." if len(names) > 40 else ""))
        lines.append("   views: " + " ".join(sorted(fmt_slot(prog, res, s) for s in views)[:30]))
        draft = []
        for off in sorted(fld):
            x = fld[off]
            ptr = set()
            for inst in insts:
                if inst[0] == "G":
                    ptr |= gverdict.get(inst[1] + off, set())
                    if kinds is not None:
                        name, start = prog.var_of(inst[1] + off)
                        if name is not None:
                            e = elem.get(name)
                            fo = inst[1] + off - start
                            ptr |= {k for k in kinds.get((name, fo % e if e else fo), set()) if k != "static"}
            for sl in views:
                o = ("L", sl) if sl[0] in ("G", "*") else sl
                v = res.verdict(("*", o if o[0] != "L" else o, off, 0))
                if v:
                    ptr.add(v)
            draft.append((off, c_type(x["t"], ptr), x["nview"]))
            lines.append("   +0x%03X %s %-4s %-24s %s" % (
                off, "v" if x["nview"] else " ", ",".join(sorted(ptr)), " ".join("%s:%d" % kv for kv in sorted(x["t"].items())),
                " ".join(sorted(x["files"]))))
        drafts.append("/* %s */" % " ".join(names[:12]))
        drafts.append(c_draft(draft, maxsize))
    with open(os.path.join(out, "types.txt"), "w") as f:
        f.write("\n".join(lines) + "\n")
    with open(os.path.join(out, "drafts.txt"), "w") as f:
        f.write("\n".join(drafts) + "\n")



if __name__ == "__main__":
    main()
