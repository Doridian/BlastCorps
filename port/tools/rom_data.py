#!/usr/bin/env python3
"""The movable build's arena image from the user's ROM (PORT_ROM_DATA,
docs/PORT.md "The data from the ROM", docs/DISTRIBUTION.md).

    rom_data.py IMAGE ROM MAP OUT.c [--arena-map FILE] [--report FILE]

IMAGE is what port-arena would put in the arena at startup
(-port-arena-image: bytes from N64 physical address 0); ROM the version's
baserom; MAP the top-level link map (build/blastcorps.<version>.map), for
where init and the four gzip members of hd_code and hd_front_end are.

The ROM's code modules, laid out at their N64 addresses as the game loads
them (init as it is at 0x8021ED00, hd_code's .text and .data inflated at
0x802447C0, hd_front_end's at 0x801E7000), are "the source".  Most of the
image is the source where it is, in big-endian builds byte for byte, in
native-endian ones with each datum in host order; what port-arena moved
(the C's string literals and statics, LP64's outgrown variables) is the
source somewhere else.  This writes OUT.c: a list of operations that makes
the image from the source, each a copy of a range of it (as it is, or with
every 2-, 4- or 8-byte unit reversed: host order) or a few literal bytes
(what isn't the ROM's: the port's own data, addresses that aren't the
N64's, the game's data that the port changed), plus the image's hash.
port/host/romdata.c does the same at startup from the user's ROM.

It checks first that the operations, applied to the source made from ROM,
give IMAGE exactly (the same decoder as the host's), and fails otherwise.
--report lists the literal bytes by variable (--arena-map: port-arena's
PORT_ARENA_MAP), for the audit.
"""
import argparse
import os
import re
import sys
import zlib

# the modules' load addresses (CLAUDE.md, "Layout notes"), physical
INIT, HD_CODE, HD_FRONT_END = 0x21ED00, 0x2447C0, 0x1E7000
RDRAM = 0x400000
UNITS = (1, 2, 4, 8)            # the transforms: a unit of this many bytes reversed
MIN_FOUND = 8                   # a copy from elsewhere than its own address: at least this long
LITERAL = 4                     # op kinds: 0-3 a copy with UNITS[k], 4 literal bytes


def rom_ranges(map_path, version_tag):
    """{segment: (start, end)} for init and the four gzip members"""
    names = {"init": "init", "hd_code_text": "hd_code_text", "hd_code_data": "hd_code_data",
             "hd_front_end_text": "hd_front_end_text", "hd_front_end_data": "hd_front_end_data"}
    syms = {}
    for line in open(map_path):
        m = re.match(r"\s+0x([0-9a-f]+)\s+(\w+)_ROM_(START|END)\b", line)
        if m:
            syms[(m.group(2), m.group(3))] = int(m.group(1), 16)
    out = {}
    for k, n in names.items():
        full = f"{n}_{version_tag}"
        if (full, "START") not in syms:
            sys.exit(f"rom_data.py: no {full}_ROM_START in {map_path}")
        out[k] = (syms[(full, "START")], syms[(full, "END")])
    return out


def inflate(data):
    d = zlib.decompressobj(31)
    out = d.decompress(data)
    if not d.eof:
        sys.exit("rom_data.py: a gzip member doesn't end")
    return out


def source(rom, ranges):
    """the modules at their physical addresses (zeros elsewhere), and the
    ranges they cover"""
    r = bytearray(RDRAM)
    cover = []

    def put(at, b):
        r[at:at + len(b)] = b
        cover.append((at, at + len(b)))

    a, e = ranges["init"]
    put(INIT, rom[a:e])
    for base, mod in ((HD_CODE, "hd_code"), (HD_FRONT_END, "hd_front_end")):
        text = inflate(rom[slice(*ranges[mod + "_text"])])
        data = inflate(rom[slice(*ranges[mod + "_data"])])
        put(base, text)
        put(base + len(text), data)
    return bytes(r), cover


def src_index(x, w):
    """the source byte a copy with unit w puts at x (relative): x's unit reversed"""
    return (x & ~(w - 1)) + (w - 1 - (x & (w - 1)))


class Differ:
    def __init__(self, img, r, cover):
        self.img, self.r = img, r
        self.cover = cover
        # the source with every w-unit reversed, for the searches
        self.t = {}
        for w in UNITS:
            if w == 1:
                self.t[w] = r
                continue
            b = bytearray(r)
            for k in range(w):
                b[k::w] = r[w - 1 - k::w]
            self.t[w] = bytes(b)

    def run(self, p, w, d, limit):
        """how many bytes from p a copy (w, source p+d) gets right"""
        img, r = self.img, self.r
        n = 0
        # whole stretches at once where the unit is 1
        if w == 1:
            q = p + d
            while p + n < limit and q + n < RDRAM and img[p + n] == r[q + n]:
                n += 1
            return n
        while p + n < limit:
            x = p + n
            s = src_index(x, w) + d
            if s < 0 or s >= RDRAM or img[x] != r[s]:
                break
            n += 1
        return n

    def ops(self):
        img = self.img
        n = len(img)
        # pass 1: copies from the image's own address only, to find what
        # needs searching
        home = self.greedy(lambda p: [])
        want = {}
        for kind, dst, ln, d, lit in home:
            if kind != LITERAL:
                continue
            for p in range(dst, dst + ln):
                g = img[p:p + MIN_FOUND]
                if len(g) == MIN_FOUND and any(g):
                    want.setdefault(g, []).append(p)
        # where those grams are in the source, by transform (positions
        # whose unit phase matches the destination's)
        found = {}
        if want:
            for w in UNITS:
                t = self.t[w]
                for a, e in self.cover:
                    for k in range(a, e - MIN_FOUND + 1):
                        g = t[k:k + MIN_FOUND]
                        if g in want:
                            l = found.setdefault(g, [])
                            if len(l) < 16:
                                l.append((w, k))

        def cands(p):
            g = img[p:p + MIN_FOUND]
            out = []
            for w, k in found.get(g, ()):
                if (k - p) % w:
                    continue        # (the unit's phase must be the destination's)
                out.append((w, k - p))
            return out
        return self.greedy(cands)

    def greedy(self, searched):
        img = self.img
        n = len(img)
        out = []
        lit = bytearray()
        lit_at = None
        p = 0
        cur = None                  # (w, d) of the copy being extended
        cur_at = 0

        def flush_lit():
            nonlocal lit, lit_at
            if lit:
                out.append((LITERAL, lit_at, len(lit), 0, bytes(lit)))
                lit = bytearray()
                lit_at = None

        def end_copy(at):
            nonlocal cur
            if cur is not None and at > cur_at:
                out.append((UNITS.index(cur[0]), cur_at, at - cur_at, cur[1], None))
            cur = None

        while p < n:
            if cur is not None:
                w, d = cur
                s = src_index(p, w) + d
                if 0 <= s < RDRAM and img[p] == self.r[s]:
                    p += 1
                    continue
                end_copy(p)
            if img[p] == 0:
                flush_lit()
                p += 1
                continue
            best, blen = None, 0
            limit = min(n, p + 4096)
            if p < RDRAM:
                for w in UNITS:
                    ln = self.run(p, w, 0, limit)
                    if ln > blen:
                        best, blen = (w, 0), ln
            for c in searched(p):
                ln = self.run(p, c[0], c[1], limit)
                if ln >= MIN_FOUND and ln > blen:
                    best, blen = c, ln
            # a copy costs about four bytes of operation: shorter than that
            # (and not its own address), literal
            if best is None or (blen < 3 and best[1] != 0) or blen == 0:
                if lit_at is None:
                    lit_at = p
                lit.append(img[p])
                p += 1
                continue
            flush_lit()
            cur, cur_at = best, p
            p += 1
        end_copy(p)
        flush_lit()
        return out


# ---- the encoding: what port/host/romdata.c reads ----------------------------
#
# A byte stream of operations, each
#   header byte: bits 0-2 the kind (0-3 a copy with unit 1, 2, 4, 8; 4
#                literal bytes), bit 3 "the source is elsewhere" (a signed
#                distance follows, else the copy is from its own address)
#   varint: the distance from the end of the previous operation
#   varint: the length
#   [svarint: the source's distance from the destination]
#   [the literal bytes]
# varints are LEB128; svarint zigzag.

def uvar(v):
    out = bytearray()
    while True:
        b = v & 0x7F
        v >>= 7
        if v:
            out.append(b | 0x80)
        else:
            out.append(b)
            return out


def encode(ops):
    out = bytearray()
    end = 0
    for kind, dst, ln, d, lit in ops:
        assert dst >= end
        out.append(kind | (8 if d else 0))
        out += uvar(dst - end)
        out += uvar(ln)
        if d:
            out += uvar((d << 1) ^ (d >> 63) if d >= 0 else ((-d) << 1) - 1)
        if kind == LITERAL:
            out += lit
        end = dst + ln
    return bytes(out)


def decode(stream, r, size):
    """the image from the source, as the host does it"""
    img = bytearray(size)
    k = 0
    end = 0

    def rd():
        nonlocal k
        v, sh = 0, 0
        while True:
            b = stream[k]
            k += 1
            v |= (b & 0x7F) << sh
            sh += 7
            if not b & 0x80:
                return v

    while k < len(stream):
        h = stream[k]
        k += 1
        kind = h & 7
        dst = end + rd()
        ln = rd()
        d = 0
        if h & 8:
            z = rd()
            d = (z >> 1) ^ -(z & 1)
        if kind == LITERAL:
            img[dst:dst + ln] = stream[k:k + ln]
            k += ln
        else:
            w = UNITS[kind]
            for x in range(dst, dst + ln):
                img[x] = r[src_index(x, w) + d]
        end = dst + ln
    return bytes(img)


def fnv64(b):
    """FNV-1a over little-endian 8-byte words, the last one zero-padded (romdata.c)"""
    h = 0xcbf29ce484222325
    b = bytes(b) + bytes(-len(b) % 8)
    for k in range(0, len(b), 8):
        h = ((h ^ int.from_bytes(b[k:k + 8], "little")) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return h


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("image")
    ap.add_argument("rom")
    ap.add_argument("map")
    ap.add_argument("out")
    ap.add_argument("--version", required=True)
    ap.add_argument("--arena-map")
    ap.add_argument("--report")
    a = ap.parse_args()
    img = open(a.image, "rb").read()
    rom = open(a.rom, "rb").read()
    tag = a.version.replace(".", "_")
    ranges = rom_ranges(a.map, tag)
    r, cover = source(rom, ranges)
    ops = Differ(img, r, cover).ops()
    stream = encode(ops)
    if decode(stream, r, len(img)) != img:
        sys.exit("rom_data.py: the operations don't make the image from the ROM")
    lit = sum(o[2] for o in ops if o[0] == LITERAL)
    copied = sum(o[2] for o in ops if o[0] != LITERAL)
    moved = sum(o[2] for o in ops if o[0] != LITERAL and o[3])
    print(f"rom_data.py: {len(ops)} operations, {len(stream)} bytes: {copied} bytes from the ROM "
          f"({moved} from elsewhere), {lit} literal")
    write_c(a.out, stream, ranges, len(img), img)
    if a.report:
        report(a.report, ops, a.arena_map, img)


def write_c(path, stream, ranges, size, img):
    h = fnv64(img)
    with open(path + ".tmp", "w") as f:
        f.write("/* Generated by port/tools/rom_data.py: the arena's initial contents as\n"
                "   operations on the ROM's code modules (port/host/romdata.c). */\n"
                "#include <stdint.h>\n\n")
        for k in ("init", "hd_code_text", "hd_code_data", "hd_front_end_text", "hd_front_end_data"):
            s, e = ranges[k]
            f.write(f"const uint32_t port_romdata_{k}[2] = {{0x{s:X}, 0x{e:X}}};\n")
        f.write(f"const uint32_t port_romdata_size = 0x{size:X};\n")
        f.write(f"const uint64_t port_romdata_hash = 0x{h:016X}ull;\n")
        f.write(f"const uint32_t port_romdata_ops_n = {len(stream)};\n")
        f.write("const uint8_t port_romdata_ops[] = {\n")
        for k in range(0, len(stream), 24):
            f.write("    " + ", ".join(str(b) for b in stream[k:k + 24]) + ",\n")
        f.write("};\n")
    os.replace(path + ".tmp", path)


def report(path, ops, arena_map, img):
    syms = []
    if arena_map:
        for line in open(arena_map):
            p = line.split()
            if len(p) == 3:
                syms.append((int(p[0], 16) & 0x1FFFFFFF, int(p[1]), p[2]))
    syms.sort()
    import bisect
    starts = [s[0] for s in syms]
    per = {}
    for kind, dst, ln, d, lit in ops:
        if kind != 4:
            continue
        for x in range(dst, dst + ln):
            i = bisect.bisect_right(starts, x) - 1
            name = syms[i][2] if i >= 0 and x < syms[i][0] + max(syms[i][1], 1) else "?"
            per.setdefault(name, []).append(x)
    with open(path, "w") as f:
        f.write("# literal bytes of the arena image, by variable: count, first address, name, bytes\n")
        for name, xs in sorted(per.items(), key=lambda kv: -len(kv[1])):
            f.write(f"{len(xs):6d} {0x80000000 | xs[0]:08X} {name} {bytes(img[x] for x in xs[:32]).hex()}\n")


if __name__ == "__main__":
    main()
