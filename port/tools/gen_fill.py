#!/usr/bin/env python3
"""The table the port fills the ROM's assets by (port/host/romdata.c's
port_rom_fill; docs/DISTRIBUTION.md).

    gen_fill.py OUT.c OUT.tsv [--assets port/tools/rom_assets.txt]

For each line of rom_assets.txt (images, vertices, the display lists that
draw them), from the version's N64 link (blastcorps/build/<module>.
<version>.elf, the version from PORT_VERSION, default us.v11): where the
game loads it (the physical address: the modules' data as the game has it,
port_rom_fill's source) and how many bytes, and of a display list, the
words the link relocates, with what they point at.  OUT.c, compiled with
the game's C (its headers, the defining files' types), is the table:

  - port_fill[]: {the variable or field, N64 address, N64 bytes, kind};
  - port_fill_relocs[]: {entry, command, the host address its second word
    holds}: a relocated word is an address into an asset (a list's
    vertices, a texture), which the host has where the compiler put it;
  - per entry, a check that the declaration's size is what the ROM has
    (the C's type: an image is bytes whatever it's declared as).

OUT.tsv lists the entries, one a line: what the port fills, its N64
range, kind (for port/tools/rom_scan.py's gate).
"""
import argparse
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")
VERSION = os.environ.get("PORT_VERSION", "us.v11")
UNIT = {"bytes": 1, "vtx": 16, "gfx": 8, "ucode": 1}
KIND = {"bytes": "PORT_FILL_BYTES", "vtx": "PORT_FILL_VTX", "gfx": "PORT_FILL_GFX"}
R_MIPS_32 = 2
K0BASE = 0x80000000
# ucode: the RSP microcode's data (not filled: zeros in the port, asm2c.py's RSP_DATA);
# only the .tsv lists it, for rom_scan.py's gate


def die(msg):
    sys.exit(f"gen_fill.py: {msg}")


# ---- the N64 link (an ELF32 big-endian with --emit-relocs) -----------------------

class Elf:
    def __init__(self, path):
        b = open(path, "rb").read()
        if b[:6] != b"\x7fELF\x01\x02":
            die(f"{path}: not a 32-bit big-endian ELF")
        shoff, = struct.unpack_from(">I", b, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from(">HHH", b, 0x2E)
        secs = [struct.unpack_from(">IIIIIIIIII", b, shoff + k * shentsize) for k in range(shnum)]
        names = secs[shstrndx][4]

        def name(off):
            return b[off:b.index(b"\0", off)].decode()

        self.sections = []      # (name, addr, file offset, size, type)
        for s in secs:
            self.sections.append((name(names + s[0]), s[3], s[4], s[5], s[1]))
        self.data = b
        self.syms = {}
        self.relocs = set()     # the addresses of R_MIPS_32 words
        for k, s in enumerate(secs):
            if s[1] == 2:                       # SHT_SYMTAB
                strs = secs[s[6]][4]
                for o in range(s[4], s[4] + s[5], 16):
                    st_name, value, size, info, other, shndx = struct.unpack_from(">IIIBBH", b, o)
                    if st_name and shndx != 0:
                        self.syms[name(strs + st_name)] = value
            elif s[1] == 9:                     # SHT_REL
                for o in range(s[4], s[4] + s[5], 8):
                    r_offset, r_info = struct.unpack_from(">II", b, o)
                    if r_info & 0xFF == R_MIPS_32:
                        self.relocs.add(r_offset)
        self.by_addr = sorted((v, n) for n, v in self.syms.items())

    def word(self, addr):
        for name, a, off, size, typ in self.sections:
            if typ == 1 and a <= addr and addr + 4 <= a + size:     # SHT_PROGBITS
                return struct.unpack_from(">I", self.data, off + addr - a)[0]
        die(f"0x{addr:08X} isn't in the link's data")

    def next_symbol(self, addr):
        """the first symbol after addr"""
        for v, n in self.by_addr:
            if v > addr:
                return v, n
        return None, None


# ---- the list --------------------------------------------------------------------

class Entry:
    def __init__(self, module, sym, off, kind, count, source, lvalue):
        self.module, self.sym, self.off, self.kind, self.count = module, sym, off, kind, count
        self.source, self.lvalue = source, lvalue
        self.size = count * UNIT[kind]


def read_assets(path):
    out = []
    for n, line in enumerate(open(path), 1):
        line = line.split("#", 1)[0].split()
        if not line:
            continue
        if len(line) not in (5, 6):
            die(f"{path}:{n}: module symbol[+offset] kind count source [field]")
        module, symoff, kind, count, source = line[:5]
        sym, _, off = symoff.partition("+")
        if kind not in UNIT:
            die(f"{path}:{n}: kind {kind}: one of {', '.join(UNIT)}")
        lvalue = line[5] if len(line) == 6 else sym
        if off and len(line) != 6:
            die(f"{path}:{n}: an offset into {sym} needs the field it is")
        out.append(Entry(module, sym, int(off, 0) if off else 0, kind, int(count, 0), source, lvalue))
    return out


# ---- the C's declarations --------------------------------------------------------

def declaration(text, sym, path):
    """(type, dimensions) of sym's definition in the file"""
    m = re.search(r"^[ \t]*((?:const\s+)?[A-Za-z_]\w*)\s+" + sym + r"\s*((?:\[[^\]]*\]\s*)+)(?:=|;)", text, re.M)
    if not m:
        die(f"{path}: no definition of {sym} (one of its own, not static)")
    return m.group(1), re.sub(r"\s+", "", m.group(2))


def local_typedef(text, tname):
    """a typedef the file makes itself, as its text (or None)"""
    m = re.search(r"^\}\s*" + tname + r"\s*;", text, re.M)
    if not m:
        return None
    start = text.rfind("typedef struct", 0, m.start())
    if start < 0:
        return None
    end = text.index(";", m.start()) + 1
    return text[start:end]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out_c")
    ap.add_argument("out_tsv")
    ap.add_argument("--assets", default=os.path.join(HERE, "rom_assets.txt"))
    a = ap.parse_args()

    entries = read_assets(a.assets)
    elves = {}
    for e in entries:
        if e.module not in elves:
            elves[e.module] = Elf(os.path.join(BLAST, "build", f"{e.module}.{VERSION}.elf"))
        elf = elves[e.module]
        if e.sym not in elf.syms:
            die(f"{e.sym}: not in {e.module}.{VERSION}.elf")
        e.addr = elf.syms[e.sym] + e.off
        e.end = e.addr + e.size
        if e.addr & 3:
            die(f"{e.sym}+0x{e.off:X}: not word-aligned")

    # each top-level asset ends before the next symbol (the structures' fields
    # inside their own)
    for e in entries:
        nxt, nn = elves[e.module].next_symbol(e.addr)
        if e.lvalue == e.sym and nxt is not None and nxt < e.end:
            die(f"{e.sym}: 0x{e.size:X} bytes run into {nn} (0x{nxt - e.addr:X} on)")
    spans = sorted((e.addr, e.end, e) for e in entries)
    for (a0, a1, e0), (b0, b1, e1) in zip(spans, spans[1:]):
        if b0 < a1:
            die(f"{e0.lvalue} and {e1.lvalue} overlap")

    # (the microcode's data is in the .tsv alone)
    ucode = [e for e in entries if e.kind == "ucode"]
    entries = [e for e in entries if e.kind != "ucode"]

    def target(module, to):
        """the C expression for the host's address of N64 address to"""
        for e in entries:
            if e.module == module and e.addr <= to < e.end:
                o = to - e.addr
                if o % UNIT[e.kind]:
                    die(f"0x{to:08X}: inside an element of {e.lvalue}")
                if e.kind == "bytes":
                    return f"(u8 *){e.lvalue} + 0x{o:X}" if o else f"(u8 *){e.lvalue}"
                t = "Vtx" if e.kind == "vtx" else "Gfx"
                return f"({t} *){e.lvalue} + {o // UNIT[e.kind]}" if o else f"({t} *){e.lvalue}"
        die(f"0x{to:08X}: a list points at it, and it isn't an asset (rom_assets.txt)")

    # the display lists' relocated words
    relocs = []         # (entry index, command, expression)
    for k, e in enumerate(entries):
        if e.kind != "gfx":
            continue
        elf = elves[e.module]
        for c in range(e.count):
            w = e.addr + 8 * c
            if w in elf.relocs:
                die(f"{e.lvalue}[{c}]: its first word is relocated")
            if w + 4 in elf.relocs:
                v = elf.word(w + 4)
                relocs.append((k, c, target(e.module, v if v >= K0BASE else v + K0BASE)))

    # the declarations, from the files that define them
    decls = []          # extern lines
    includes = ['"common.h"']
    typedefs = []
    checks = []
    seen = set()
    for k, e in enumerate(entries):
        if e.sym in seen:
            continue
        seen.add(e.sym)
        if e.source == "asm":
            # (asm2c.py's: asm_data.h's types, as the C's that have their layout)
            t = {"bytes": "u8", "vtx": "Vtx", "gfx": "Gfx"}[e.kind]
            decls.append(f"extern {t} {e.sym}[{e.count}];")
            continue
        path = os.path.join(BLAST, e.source)
        text = open(path).read()
        t, dims = declaration(text, e.sym, path)
        for inc in re.findall(r'^#include\s+("[^"]+")', text, re.M):
            if inc not in includes:
                includes.append(inc)
        td = local_typedef(text, t)
        if td and td not in typedefs:
            typedefs.append(td)
        decls.append(f"extern {t} {e.sym}{dims};")
    for k, e in enumerate(entries):
        host = {"bytes": str(e.size), "vtx": f"{e.count} * sizeof(Vtx)", "gfx": f"{e.count} * sizeof(Gfx)"}
        checks.append(f"typedef char port_fill_check_{k}[sizeof({e.lvalue}) == {host[e.kind]} ? 1 : -1];")

    phys = lambda x: x - K0BASE    # noqa: E731
    out = [f"/* Generated by port/tools/gen_fill.py from port/tools/rom_assets.txt and the {VERSION} N64 link:",
           "   not to be committed.  The ROM's assets the port fills at startup (host/romdata.c). */"]
    out += [f"#include {i}" for i in includes]
    out += ['#include "port.h"', ""]
    out += typedefs + [""] if typedefs else []
    out += decls + [""] + checks + [""]
    out.append("const PortFill port_fill[] = {")
    for e in entries:
        unit = f"sizeof({e.lvalue}[0])" if e.kind == "bytes" else "0"
        out.append(f'    {{ {e.lvalue}, 0x{phys(e.addr):06X}, 0x{e.size:X}, {KIND[e.kind]} '
                   f'PORT_FILL_CHECK_FIELDS("{e.lvalue}", {unit}) }},')
    out.append("};")
    out.append(f"const unsigned int port_fill_n = {len(entries)};")
    out.append("")
    out.append("const PortFillReloc port_fill_relocs[] = {")
    for k, c, x in relocs:
        out.append(f"    {{ {k}, {c}, {x} }},")
    if not relocs:
        out.append("    { 0, 0, 0 },")
    out.append("};")
    out.append(f"const unsigned int port_fill_relocs_n = {len(relocs)};")
    text = "\n".join(out) + "\n"
    open(a.out_c, "w").write(text)
    tsv = "".join(f"{e.lvalue}\t{e.module}\t0x{e.addr:08X}\t0x{e.end:08X}\t{e.kind}\n" for e in entries + ucode)
    open(a.out_tsv, "w").write(tsv)


if __name__ == "__main__":
    main()
