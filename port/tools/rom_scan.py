#!/usr/bin/env python3
"""What of the ROM a built port carries (docs/DISTRIBUTION.md).

    rom_scan.py FILE [FILE...] [--rom ROM] [--version V] [--min N] [--top N]
                [--max-bytes N]

Searches each FILE (an executable, a .wasm, a .js, a build's arena image)
for the ROM's bytes: the ROM as it is, and its code modules as the game
loads them (init; hd_code and hd_front_end inflated from their gzip
members, which a search of the ROM alone can't see), each as it is and
with every 2- and 4-byte unit reversed (native-endian data).  Every common
stretch of at least --min bytes (default 24) is reported, merged, with
where it is in the ROM's modules (the N64 link's symbols, from
blastcorps/build/*.elf) and in FILE (its symbols, where llvm-nm reads
them).  Stretches of one repeated byte or of a short repeated pattern
don't count, nor stretches of counting numbers (an alphabet, a table of
0, 1, 2, ...), which any program has.

--max-bytes: exit 1 when a file carries more than N bytes of it (the test
suite's check; default: report only).
"""
import argparse
import bisect
import os
import re
import shutil
import subprocess
import sys
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
INIT, HD_CODE, HD_FRONT_END = 0x21ED00, 0x2447C0, 0x1E7000
K = 16                          # the index's chunk


def rom_ranges(version):
    tag = version.replace(".", "_")
    m = os.path.join(ROOT, "build", f"blastcorps.{version}.map")
    syms = {}
    for line in open(m):
        r = re.match(r"\s+0x([0-9a-f]+)\s+(\w+)_ROM_(START|END)\b", line)
        if r:
            syms[(r.group(2), r.group(3))] = int(r.group(1), 16)
    return {n: (syms[(f"{n}_{tag}", "START")], syms[(f"{n}_{tag}", "END")])
            for n in ("init", "hd_code_text", "hd_code_data", "hd_front_end_text", "hd_front_end_data")}


def inflate(b):
    d = zlib.decompressobj(31)
    return d.decompress(b)


def sources(rom, version):
    """[(name, base address or None, bytes)]"""
    out = [("rom", None, rom)]
    try:
        rr = rom_ranges(version)
    except (OSError, KeyError):
        print(f"rom_scan.py: no top-level map for {version}: the ROM only", file=sys.stderr)
        return out
    out.append(("init", 0x80000000 | INIT, rom[slice(*rr["init"])]))
    for base, m in ((HD_CODE, "hd_code"), (HD_FRONT_END, "hd_front_end")):
        t = inflate(rom[slice(*rr[m + "_text"])])
        d = inflate(rom[slice(*rr[m + "_data"])])
        out.append((m, 0x80000000 | base, t + d))
    return out


def reverse_units(b, w):
    if w == 1:
        return b
    b = b[:len(b) - len(b) % w]
    o = bytearray(b)
    for k in range(w):
        o[k::w] = b[w - 1 - k::w]
    return bytes(o)


def progression(b):
    """1-, 2- or 4-byte numbers counting by a constant step, in either byte
    order, from any phase (a table of 0, 1, 2, ..., an alphabet)"""
    for u in (1, 2, 4):
        for phase in range(u):
            c = b[phase:]
            c = c[:len(c) - len(c) % u]
            if len(c) < 4 * u:
                continue
            for order in ("big", "little"):
                v = [int.from_bytes(c[k:k + u], order) for k in range(0, len(c), u)]
                d = [(v[k + 1] - v[k]) % (1 << (8 * u)) for k in range(len(v) - 1)]
                # (a step or two out of line: "0-9A-Z")
                if len(d) - max(d.count(x) for x in set(d)) <= 2:
                    return True
    return False


def repeated(b):
    """one byte or a pattern of up to 4 repeated, or at most two values"""
    for p in (1, 2, 4):
        if b == b[:p] * (len(b) // p) + b[:len(b) % p]:
            return True
    return len(set(b)) <= 2


def boring(b):
    """nothing that says where it came from: repeated, mostly one byte, or
    counting"""
    if repeated(b) or max(b.count(x) for x in set(b)) * 10 >= len(b) * 9:
        return True
    return progression(b)


def n64_symbols(version):
    """{module: (sorted addresses, names)} from the N64 link"""
    nm = shutil.which("mips-linux-gnu-nm") or shutil.which("llvm-nm")
    out = {}
    for m in ("init", "hd_code", "hd_front_end"):
        elf = os.path.join(ROOT, "blastcorps", "build", f"{m}.{version}.elf")
        if not nm or not os.path.exists(elf):
            continue
        syms = []
        for line in subprocess.run([nm, elf], capture_output=True, text=True).stdout.splitlines():
            p = line.split()
            if len(p) == 3 and p[1] in "tTdDrRbB" and not p[2].startswith("$"):
                syms.append((int(p[0], 16), p[2]))
        syms.sort()
        out[m] = ([a for a, _ in syms], [n for _, n in syms])
    return out


def file_symbols(path):
    """(sorted file offsets, names) of an ELF's defined symbols, via llvm-nm and the section headers"""
    nm = shutil.which("llvm-nm")
    ro = shutil.which("llvm-readelf") or shutil.which("readelf")
    if not nm or not ro:
        return [], []
    with open(path, "rb") as f:
        if f.read(4) != b"\x7fELF":
            return [], []
    secs = []
    for line in subprocess.run([ro, "-SW", path], capture_output=True, text=True).stdout.splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)", line)
        if m and m.group(2) != "NOBITS":
            secs.append((int(m.group(3), 16), int(m.group(4), 16), int(m.group(5), 16), m.group(1)))
    syms = []
    for line in subprocess.run([nm, "-n", "--defined-only", path], capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) != 3:
            continue
        a = int(p[0], 16)
        for addr, off, size, name in secs:
            if addr and addr <= a < addr + size:
                syms.append((off + a - addr, p[2]))
                break
    syms.sort()
    return [a for a, _ in syms], [n for _, n in syms]


def name_at(table, x):
    addrs, names = table
    i = bisect.bisect_right(addrs, x) - 1
    if i < 0:
        return "?"
    return f"{names[i]}+0x{x - addrs[i]:X}" if x != addrs[i] else names[i]


def scan(data, srcs, minlen):
    """[(file offset, length, source name, source offset, unit)]"""
    found = []
    for name, base, raw in srcs:
        for w in (1, 2, 4):
            if w > 1 and name == "rom":
                continue            # (the ROM file's own data is big-endian)
            s = reverse_units(raw, w)
            # chunks of the source at every 8 bytes: any common stretch of
            # K + 7 bytes holds one
            idx = {}
            for k in range(0, len(s) - K + 1, 8):
                c = s[k:k + K]
                if not repeated(c):
                    idx.setdefault(c, k)
            hits = []
            n = len(data)
            for b in range(0, n - K + 1):
                k = idx.get(data[b:b + K])
                if k is None:
                    continue
                hits.append((b, k))
            # extend each hit both ways, merge the ones it covers
            covered_to = -1
            for b, k in hits:
                if b < covered_to:
                    continue
                lo = 0
                while b - lo > 0 and k - lo > 0 and data[b - lo - 1] == s[k - lo - 1]:
                    lo += 1
                hi = K
                while b + hi < n and k + hi < len(s) and data[b + hi] == s[k + hi]:
                    hi += 1
                start, ln = b - lo, lo + hi
                covered_to = b + hi
                if ln >= minlen and not boring(data[start:start + ln]):
                    found.append((start, ln, name, k - lo, w))
    # a stretch found in several sources (the ROM and a module, or two
    # unit sizes): the longest
    found.sort(key=lambda f: (f[0], -f[1]))
    out = []
    end = -1
    for f in found:
        if f[0] + f[1] <= end:
            continue
        out.append(f)
        end = max(end, f[0] + f[1])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--version", default=os.environ.get("PORT_VERSION", "us.v10"))
    ap.add_argument("--rom")
    ap.add_argument("--min", type=int, default=24)
    ap.add_argument("--top", type=int, default=20)
    ap.add_argument("--max-bytes", type=int)
    a = ap.parse_args()
    rom_path = a.rom or os.path.join(ROOT, f"baserom.{a.version}.z64")
    if not os.path.exists(rom_path):
        print(f"rom_scan.py: no {rom_path}", file=sys.stderr)
        sys.exit(77)
    rom = open(rom_path, "rb").read()
    srcs = sources(rom, a.version)
    n64 = n64_symbols(a.version)
    bad = False
    for path in a.files:
        data = open(path, "rb").read()
        found = scan(data, srcs, a.min)
        total = sum(f[1] for f in found)
        fsyms = file_symbols(path)
        print(f"{path}: {len(data)} bytes, {total} of them the ROM's in {len(found)} stretches of {a.min} or more")
        by = {}
        for f in found:
            by[(f[2], f[4])] = by.get((f[2], f[4]), 0) + f[1]
        for (src, w), t in sorted(by.items(), key=lambda kv: -kv[1]):
            print(f"    {t:8d} bytes from {src}" + (f" ({w}-byte units reversed)" if w > 1 else ""))
        for start, ln, src, off, w in sorted(found, key=lambda f: -f[1])[:a.top]:
            if src == "rom":
                where = f"ROM 0x{off:X}"
            else:
                base = dict((s[0], s[1]) for s in srcs)[src]
                addr = base + off
                where = f"{src} {addr:08X}"
                if src in n64:
                    where += f" {name_at(n64[src], addr)}"
            w_s = f" ({w}-byte units reversed)" if w > 1 else ""
            fs = f" {name_at(fsyms, start)}" if fsyms[0] else ""
            print(f"    {ln:7d} at 0x{start:X}{fs}: {where}{w_s}")
        if a.max_bytes is not None and total > a.max_bytes:
            print(f"rom_scan.py: {path} carries {total} bytes of the ROM, more than {a.max_bytes}")
            bad = True
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
