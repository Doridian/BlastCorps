#!/usr/bin/env python3
"""Name the rows of the access-width profiler's table (port/host/access.c).

  access_report.py TABLE.tsv EXE [--all]

TABLE.tsv is what a PORT_ACCESS_PROFILE build writes to PORT_ACCESS=FILE;
EXE is that build (for the names of the C's functions, whose sites are
hashes of them).  Translated-code sites are N64 addresses, named from the
N64 ELFs; ROM offsets are named by the top-level ROM split.  Each row is
one (reader, writer, widths) pair: a read at a width other than the one
the bytes were written at, which native-endian memory would get wrong
(docs/PORT.md, "Native-endian memory").  The report groups them:

  rom:      data DMA'd from the ROM and read at a width > 1: what a
            loader has to convert, by ROM segment;
  inflated: the same for data a decompressor wrote (gzip's inflate,
            Rare's func_802C41C0), by reader;
  punning:  memory written at one width and read at another, by writer
            and reader function;
and within each the rows are sorted by count.  --all lists every row.
Reads that only copy (the same bytes stored again at the same width) are
left out by the profiler itself.
"""

import collections
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")
# the version of the stage-2 build the port is made from (CMake sets it)
VERSION = os.environ.get("PORT_VERSION", "us.v11")


# writers whose output is ROM data: gzip's inflate (the code modules, the
# levels) and Rare's own decompressor (hd_code 7F8B0, func_802C41C0)
DECOMPRESSORS = r"C (inflate|flush_window|huft_|unzip|func_8028B4C4)|asm func_802C41C0"


def fnv(name):
    h = 2166136261
    for c in name.encode():
        h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h


def c_names(exe):
    names = {}
    for line in subprocess.run(["nm", exe], capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "Tt":
            names[fnv(p[2])] = p[2]
    return names


def n64_syms():
    funcs, data = [], []
    for m in ("hd_code", "hd_front_end", "init"):
        elf = os.path.join(BLAST, "build", f"{m}.{VERSION}.elf")
        out = subprocess.run(["mips-linux-gnu-nm", "-n", elf], capture_output=True, text=True).stdout
        for line in out.splitlines():
            p = line.split()
            if len(p) != 3 or p[2].startswith((".L", "$")):
                continue
            a = int(p[0], 16)
            if not 0x80000000 <= a < 0x80800000:
                continue
            (funcs if p[1] in "Tt" else data).append((a, p[2]))
    funcs.sort()
    data.sort()
    return funcs, data


def rom_segments():
    segs = []
    mp = os.path.join(ROOT, "build", f"blastcorps.{VERSION}.map")
    for line in open(mp):
        m = re.match(r"\s+0x([0-9a-f]+)\s+(\w+)_ROM_START = ", line)
        if m:
            segs.append((int(m.group(1), 16), m.group(2)))
    segs.sort()
    return segs


def lookup(table, a):
    lo, hi = 0, len(table)
    while lo < hi:
        mid = (lo + hi) // 2
        if table[mid][0] <= a:
            lo = mid + 1
        else:
            hi = mid
    if lo == 0:
        return None
    base, name = table[lo - 1]
    return name if a == base else f"{name}+0x{a - base:X}"


def main():
    lines = list(open(sys.argv[1]))[1:]
    assets = {int(l.split("\t")[1]): l.split("\t")[2].strip() for l in lines if l.startswith("#asset")}
    rows = [l for l in lines if not l.startswith("#")]
    cn = c_names(sys.argv[2])
    funcs, data = n64_syms()
    segs = rom_segments()

    def site(kind, s):
        s = int(s, 16)
        if kind == "c":
            return "C " + cn.get(s, f"?{s:08X}")
        if kind == "asm":
            return "asm " + (lookup(funcs, s) or f"{s:08X}")
        if kind == "host":
            return "host (RSP/RDP/AI)"
        if kind == "dma":
            seg = lookup(segs, s)
            return f"ROM {seg}" if seg else f"ROM {s:06X}"
        if kind == "conv":          # native-endian loads: asset << 20 | offset, or the address
            if s < 0x80000000 and (s >> 20) in assets:
                return f"load {assets[s >> 20]}+0x{s & 0xFFFFF:X}"
            return f"load {lookup(data, s) or hex(s)}"
        if kind == "img":
            return f"data {lookup(data, s) or hex(s)}"
        return f"{kind} {s:08X}"

    rom = collections.defaultdict(lambda: [0, set(), 0xFFFFFFFF, 0])
    infl = collections.defaultdict(lambda: [0, 0xFFFFFFFF, 0])
    byfn = collections.defaultdict(lambda: [0, 0xFFFFFFFF, 0])
    pun = []

    def fn(name):
        return re.sub(r"\+0x[0-9A-F]+$", "", name)
    for line in rows:
        rk, rs, rw, wk, ws, ww, cnt, lo, hi = line.split()
        cnt, lo, hi = int(cnt), int(lo, 16), int(hi, 16)
        reader = site(rk, rs)
        if wk == "dma":
            seg = site(wk, ws).split(" +")[0].split("+0x")[0]
            e = rom[(seg, reader, rw)]
            e[0] += cnt
            e[1].add(ws)
            e[2] = min(e[2], lo)
            e[3] = max(e[3], hi)
        else:
            writer = site(wk, ws)
            if re.match(DECOMPRESSORS, writer):
                e = infl[(fn(reader), rw)]
            else:
                pun.append((cnt, reader, rw, writer, ww, lo, hi))
                e = byfn[(fn(writer), ww, fn(reader), rw)]
            e[0] += cnt
            e[1] = min(e[1], lo)
            e[2] = max(e[2], hi)

    def where(lo, hi):
        a, b = lookup(data, lo), lookup(data, hi)
        return f"{lo:08X}-{hi:08X} ({a}{' .. ' + b if b != a else ''})"

    print("== ROM data read at a width > 1 (a loader has to convert these) ==")
    for (seg, reader, rw), (cnt, _, lo, hi) in sorted(rom.items(), key=lambda kv: -kv[1][0]):
        print(f"{cnt:9d}  {seg:28s} read {rw} by {reader:40s} {where(lo, hi)}")
    print()
    print("== decompressed ROM data read at a width > 1, by reader ==")
    for (reader, rw), (cnt, lo, hi) in sorted(infl.items(), key=lambda kv: -kv[1][0]):
        print(f"{cnt:9d}  read {rw} by {reader:40s} {where(lo, hi)}")
    print()
    print("== written at one width, read at another, by function ==")
    for (writer, ww, reader, rw), (cnt, lo, hi) in sorted(byfn.items(), key=lambda kv: -kv[1][0]):
        print(f"{cnt:9d}  {writer:30s} w{ww} -> {reader:30s} r{rw}  {where(lo, hi)}")
    if "--all" in sys.argv:
        print()
        print("== every row ==")
        pun.sort(key=lambda r: -r[0])
        for cnt, reader, rw, writer, ww, lo, hi in pun:
            print(f"{cnt:9d}  {writer:36s} w{ww} -> {reader:36s} r{rw}  {where(lo, hi)}")


if __name__ == "__main__":
    main()
