#!/usr/bin/env python3
"""Turn splat's MIPS data files into x86 GNU as input.

The port keeps memory in the N64's byte order (docs/PORT.md), so every
number is written out as big-endian bytes.  A `.word` that names a symbol
can only be filled in by the host linker, in host order: it becomes a
`.long`, and its address goes into the `port_bswap32` section, which the
runtime swaps once at startup (port/src/startup.c).

  asm2x86.py OUT.s IN.s            a .data/.rodata/.bss file (asm/data, asm/bss)
  asm2x86.py OUT.s IN.bin LABEL    a binary blob (a data island), as LABEL
  --native (first)                 host order instead (PORT_NATIVE_ENDIAN,
                                   docs/PORT.md "Native-endian memory")

With --native, every datum is written in host order at the width its
directive gives, or the width the type inventory
(blastcorps/include/game/inventory.json, `symbols`) gives the symbol it is
in, where that differs (a `.word` holding two s16s); a blob is converted by
the widths `BLOBS` below gives it.  A `.double` becomes two host-order
words, the high one first, as the port keeps every 64-bit scalar
(BEPass).  Symbolic words are plain `.long`s, and nothing is swapped at
startup.  Each run of one width is listed in the `port_widths` section
({address, length << 4 | width}), which the access-width profiler seeds
its map of RDRAM from.

Each file becomes one section, `.n64.<first label>` (".n64b." for .bss),
which gen_ld.py places at that label's N64 address: the game's data keeps
its N64 layout (docs/PORT.md).  .rodata is writable too, since fixups
write to it.
"""

import json
import os
import re
import struct
import sys

fix_count = 0


def be_bytes(values, size):
    out = []
    for v in values:
        v &= (1 << (8 * size)) - 1
        out += list(v.to_bytes(size, "big"))
    return out


def emit_bytes(out, bs):
    for k in range(0, len(bs), 32):
        out.append(".byte " + ", ".join(f"0x{b:02X}" for b in bs[k:k + 32]))


def split_args(s):
    return [a.strip() for a in s.split(",") if a.strip()]


def is_number(a):
    try:
        int(a, 0)
        return True
    except ValueError:
        return False


def open_section(out, section, label):
    if section == ".bss":
        out.append(f".section .n64b.{label},\"aw\",@nobits")
    else:
        out.append(f".section .n64.{label},\"aw\",@progbits")


def convert(src, label=None):
    global fix_count
    out = []
    section = None
    pending_section = None
    base = re.sub(r"[^A-Za-z0-9_]", "_", src)
    for raw in open(src):
        line = raw.split("#", 1)[0].rstrip() if not raw.lstrip().startswith(".ascii") else raw.rstrip()
        s = line.strip()
        if not s or s.startswith("/*"):
            continue
        if s.startswith(".include"):
            continue
        m = re.match(r"\.section\s+(\S+)", s)
        if m:
            sec = m.group(1).rstrip(",")
            section = ".bss" if sec == ".bss" else ".data"
            pending_section = section
            if label:
                open_section(out, section, label)
                out.append(f".globl {label}")
                out.append(f"{label}:")
                label = None
                pending_section = None
            continue
        m = re.match(r"(dlabel|glabel)\s+(\S+)", s)
        if m and pending_section:
            open_section(out, pending_section, m.group(2))
            pending_section = None
        if m:
            if m.group(1) == "glabel":
                out.append(".balign 4")
            out.append(f".globl {m.group(2)}")
            out.append(f"{m.group(2)}:")
            continue
        m = re.match(r"(\.L\w+):$", s)
        if m:
            out.append(s)
            continue
        m = re.match(r"(\.\w+)\s*(.*)$", s)
        if not m:
            sys.exit(f"asm2x86.py: {src}: can't parse {s!r}")
        d, rest = m.group(1), m.group(2)
        if d == ".byte":
            emit_bytes(out, be_bytes([int(a, 0) for a in split_args(rest)], 1))
        elif d in (".short", ".half"):
            emit_bytes(out, be_bytes([int(a, 0) for a in split_args(rest)], 2))
        elif d in (".word", ".word32"):
            args = split_args(rest)
            pend = []
            for a in args:
                if is_number(a):
                    pend.append(int(a, 0))
                    continue
                if pend:
                    emit_bytes(out, be_bytes(pend, 4))
                    pend = []
                lab = f".Lfix_{base}_{fix_count}"
                fix_count += 1
                out.append(f"{lab}: .long {a}")
                out.append(".pushsection port_bswap32, \"aw\"")
                out.append(f".long {lab}")
                out.append(".popsection")
            if pend:
                emit_bytes(out, be_bytes(pend, 4))
        elif d == ".float":
            bs = []
            for a in split_args(rest):
                bs += list(struct.pack(">f", float(a)))
            emit_bytes(out, bs)
        elif d == ".double":
            bs = []
            for a in split_args(rest):
                bs += list(struct.pack(">d", float(a)))
            emit_bytes(out, bs)
        elif d in (".space", ".skip"):
            out.append(f".space {rest}")
        elif d in (".ascii", ".asciz", ".string"):
            out.append(f"{d} {rest}")
        elif d == ".align":
            out.append(f".p2align {rest}")
        elif d == ".balign":
            out.append(f".balign {rest}")
        elif d in (".size", ".type", ".set", ".global", ".globl", ".local"):
            if d in (".global", ".globl"):
                out.append(f".globl {rest}")
        else:
            sys.exit(f"asm2x86.py: {src}: unsupported directive {d}")
    return out


# --native: the data islands kept as .bin, by label: [(offset, length,
# width)], the rest bytes.  (hd_code 68810 and 690C0 are the u16 tables
# embedded in Rare's math routines.)
def blob_8e910(data):
    """hd_code 8E910: a table per level (func_802A1EC8 picks one into
    D_803BE708, func_802BD064 walks it): {u32 n; n x {u32 key (set at run
    time), u32 count; u8 [16]}}, one after another"""
    out, p = [], 0
    while p + 4 <= len(data):
        n = int.from_bytes(data[p:p + 4], "big")
        out.append((p, 4, 4))
        p += 4
        for _ in range(n):
            out.append((p, 8, 4))
            p += 0x18
    return out


BLOBS = {
    "hd_code_68810_bin": [(0, None, 2)],
    "hd_code_690C0_bin": [(0, None, 2)],
    "hd_code_8E910_bin": blob_8e910,
}

# --native: the two pointer-bearing data islands of hd_code's .text, which
# splat left as bytes and pointers (asm/data/hd_code/*.bin.s), laid out by
# how their readers walk them: [(offset, width)] from the file's start, the
# rest bytes.  `flat` is the file's bytes (a pointer's four are None).
def island_7d9d0(flat):
    """hd_code 7D9D0: the animated textures (func_8029DF78: {u8 id, n, u8,
    u8; u16 duration[n], texture[n]}) up to the first pointer table, then the
    sprites (func_802A6274, func_802A64A4: {u16 texture; u8 columns, rows;
    u16; u32 -1; u16 w, h, n; u16 texture[n * columns * rows]}, each 4-byte
    aligned) up to the second"""
    out, p = [], 0
    be16 = lambda k: flat[k] << 8 | flat[k + 1]
    while p + 4 <= len(flat) and flat[p] is not None:
        n = flat[p + 1]
        out += [(p + 4 + 2 * k, 2) for k in range(2 * n)]
        p += 4 + 4 * n
    while p < len(flat) and flat[p] is None:
        p += 4                          # the pointer table
    out.append((p, 4))                  # its -1
    p += 12                             # and 8 bytes
    while p + 16 <= len(flat) and flat[p] is not None:
        a, b, n = flat[p + 2], flat[p + 3], be16(p + 14)
        out += [(p, 2), (p + 4, 2), (p + 6, 4), (p + 10, 2), (p + 12, 2), (p + 14, 2)]
        out += [(p + 16 + 2 * k, 2) for k in range(n * a * b)]
        p = (p + 16 + 2 * n * a * b + 3) & ~3
    return out


def island_800dc(flat):
    """hd_code 800DC: 43A60.c's UnkStruct_8036EC30s (three s16 * lists, s32,
    s16, s16, bytes), each followed by its lists ({s16 n; s16 [n]}, 4-byte
    aligned), then D_802C4A20[], -1"""
    out, p = [(0, 4)], 4
    be16 = lambda k: flat[k] << 8 | flat[k + 1]
    while p + 12 <= len(flat) and flat[p] is None:
        if not (flat[p + 4] is None and flat[p + 8] is None):
            break                       # the last pointer table
        out += [(p + 12, 4), (p + 16, 2), (p + 18, 2)]
        p += 0x24
        while p + 2 <= len(flat) and flat[p] is not None:
            n = be16(p)
            out += [(p + 2 * k, 2) for k in range(n + 1)]
            p = (p + 2 + 2 * n + 3) & ~3
    while p < len(flat) and flat[p] is None:
        p += 4
    out.append((p, 4))
    return out


ISLANDS = {"7D9D0.bin.s": island_7d9d0, "800DC.bin.s": island_800dc}

INVENTORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "blastcorps", "include",
                         "game", "inventory.json")
_inv = None


def inventory():
    global _inv
    if _inv is None:
        _inv = json.load(open(INVENTORY))
    return _inv


SCALAR = {"s8": 1, "u8": 1, "s16": 2, "u16": 2, "s32": 4, "u32": 4, "f32": 4, "s64": 8, "u64": 8, "f64": 8,
          "ptr": 4, "ptr?": 4, "romoff": 4, "segptr": 4, "offset": 4, "phys": 4, "gfx": 4, "bytes": 1}


def type_widths(t, types, depth=0):
    """[(offset, width)] of every scalar in type t (None: unknown/union)"""
    if t in SCALAR:
        return [(0, SCALAR[t])]
    ty = types.get(t)
    if ty is None or ty.get("union") or depth > 8:
        return None
    out = []
    for f in ty["fields"]:
        sub = type_widths(f["type"], types, depth + 1)
        if sub is None:
            return None
        esz = f.get("size") or (types[f["type"]]["size"] if f["type"] in types else SCALAR[f["type"]])
        for k in range(f.get("count", 1)):
            out += [(f["off"] + k * esz + o, w) for o, w in sub]
    return out


def symbol_widths(src):
    """{label: [(offset, width)]} for the symbols of this file the inventory types"""
    inv = inventory()
    rel = os.path.relpath(src, os.path.join(os.path.dirname(INVENTORY), "..", ".."))
    out = {}
    for name, sym in inv["symbols"].items():
        if sym.get("storage") != rel:
            continue
        ws = type_widths(sym["type"], inv["types"])
        if ws is None:
            continue
        size = inv["types"][sym["type"]]["size"] if sym["type"] in inv["types"] else SCALAR[sym["type"]]
        count = sym.get("count", 1)
        # (fieldscan's records repeat up to the next symbol, the last one
        # possibly cut short: they stop at the next label, emit's `clip`)
        clip = "fieldscan" in sym.get("derived", "")
        out[name] = (size * count, [(k * size + o, w) for k in range(count) for o, w in ws], clip)
    return out


def native_bytes(bs, width):
    """big-endian bytes -> host order, in `width` units (8: two words)"""
    width = min(width, 4)
    if width == 1:
        return list(bs)
    out = []
    for k in range(0, len(bs), width):
        out += list(reversed(bs[k:k + width]))
    return out


class Native:
    """--native: the file as a sequence of items; data bytes carry the width
    they are converted at, so that a symbol's type can override it"""

    def __init__(self, src):
        self.items = []         # ("line", text) | ("data", [[byte, width]...]) | ("sym", name)
        self.labels = {}        # label -> (item index, byte offset in the flat data)
        self.flat = []          # [item index, position] of every data byte, in order
        self.overrides = symbol_widths(src) if src.endswith(".s") else {}
        self.island = ISLANDS.get(os.path.basename(src)) if src.endswith(".bin.s") else None
        self.nwidth = 0

    def line(self, text):
        self.items.append(["line", text])

    def label(self, name):
        self.labels[name] = len(self.flat)

    def data(self, bs, width, directive=None):
        # each byte: [value, width, starts a unit, the directive]
        item = ["data", [[b, width, k % width == 0, directive] for k, b in enumerate(bs)]]
        self.items.append(item)
        for k in range(len(bs)):
            self.flat.append((item, k))

    def sym(self, name):
        item = ["sym", name]
        self.items.append(item)
        for k in range(4):
            self.flat.append((item, k))

    def emit(self):
        if self.island:
            flat = [item[1][k][0] if item[0] == "data" else None for item, k in self.flat]
            ws = self.island(flat)
            for b in range(len(self.flat)):
                item, k = self.flat[b]
                if item[0] == "data":
                    item[1][k][1:3] = [1, True]
            for off, w in ws:
                if off + w <= len(self.flat) and all(self.flat[off + j][0][0] == "data" for j in range(w)):
                    for j in range(w):
                        item, k = self.flat[off + j]
                        item[1][k][1:3] = [w, j == 0]
        # (smallest first: an array typed over labels splat made inside it,
        # D_8020C070's, has the last word)
        for name, (size, ws, clip) in sorted(self.overrides.items(), key=lambda kv: (kv[1][0], kv[0])):
            if name not in self.labels:
                continue
            base = self.labels[name]
            if clip:
                # D_80305D74's 21st record would take D_80305DF0's first
                # two bytes, an 0xFF-terminated text, as a u16
                size = min([size] + [p - base for p in self.labels.values() if p > base])
                ws = [(o, w) for o, w in ws if o + w <= size]
            # its fields at their widths; the rest of its .words are bytes
            # (splat writes .word for whatever it knows nothing about, where
            # its .half and .byte come from what it was told), its .halfs,
            # .bytes and .floats as they are
            for b in range(min(size, len(self.flat) - base)):
                item, k = self.flat[base + b]
                if item[0] == "data" and item[1][k][3] == "word":
                    item[1][k][1:3] = [1, True]
            for off, w in ws:
                if base + off + w > len(self.flat):
                    continue
                if any(self.flat[base + off + b][0][0] != "data" for b in range(w)):
                    continue
                for b in range(w):
                    item, k = self.flat[base + off + b]
                    item[1][k][1:3] = [w, b == 0]
                # (what is left of a directive's unit the field cuts into is
                # no unit any more: bytes, as emit's unit() finds)
        merged = []
        for item in self.items:
            if item[0] == "data" and merged and merged[-1][0] == "data":
                merged[-1] = ["data", merged[-1][1] + item[1]]
            else:
                merged.append(item)
        out = []
        for item in merged:
            if item[0] == "line":
                out.append(item[1])
            elif item[0] == "sym":
                self.run(out, 4)
                out.append(f".long {item[1]}")
            else:
                bs = item[1]

                def unit(k):
                    """the width of the unit at k, 1 if the bytes there aren't one"""
                    w = bs[k][1]
                    if w > 1 and bs[k][2] and k + w <= len(bs) and \
                            all(bs[k + j][1] == w and not bs[k + j][2] for j in range(1, w)):
                        return w
                    return 1
                k = 0
                while k < len(bs):
                    w = unit(k)
                    n = w
                    while k + n < len(bs) and unit(k + n) == w and bs[k + n][1] == bs[k][1]:
                        n += w
                    self.run(out, min(w, 4), n)
                    emit_bytes(out, native_bytes([b[0] for b in bs[k:k + n]], w))
                    k += n
        return out

    def run(self, out, width, n=4):
        lab = f".Lw{self.nwidth}"
        self.nwidth += 1
        out.append(f"{lab}:")
        out.append(".pushsection port_widths, \"a\"")
        out.append(f".long {lab}, {n << 4 | width}")
        out.append(".popsection")


# data objects that are u16 text, however splat wrote them (`.word`): the
# menus' Japanese text, which only jp shows (26570.c's D_803010A0 on)
HALF_FILES = ("hd_code/BC8E0.data.s",)


def convert_native(src, label=None):
    nat = Native(src)
    halves = src.endswith(HALF_FILES)
    section = None
    pending_section = None
    for raw in open(src):
        line = raw.split("#", 1)[0].rstrip() if not raw.lstrip().startswith(".ascii") else raw.rstrip()
        s = line.strip()
        if not s or s.startswith("/*") or s.startswith(".include"):
            continue
        m = re.match(r"\.section\s+(\S+)", s)
        if m:
            sec = m.group(1).rstrip(",")
            section = ".bss" if sec == ".bss" else ".data"
            pending_section = section
            if label:
                o = []
                open_section(o, section, label)
                for t in o + [f".globl {label}", f"{label}:"]:
                    nat.line(t)
                label = None
                pending_section = None
            continue
        m = re.match(r"(dlabel|glabel)\s+(\S+)", s)
        if m:
            if pending_section:
                o = []
                open_section(o, pending_section, m.group(2))
                for t in o:
                    nat.line(t)
                pending_section = None
            if m.group(1) == "glabel":
                nat.line(".balign 4")
            nat.line(f".globl {m.group(2)}")
            nat.line(f"{m.group(2)}:")
            nat.label(m.group(2))
            continue
        m = re.match(r"(\.L\w+):$", s)
        if m:
            nat.line(s)
            continue
        m = re.match(r"(\.\w+)\s*(.*)$", s)
        if not m:
            sys.exit(f"asm2x86.py: {src}: can't parse {s!r}")
        d, rest = m.group(1), m.group(2)
        if d == ".byte":
            nat.data(be_bytes([int(a, 0) for a in split_args(rest)], 1), 1)
        elif d in (".short", ".half"):
            nat.data(be_bytes([int(a, 0) for a in split_args(rest)], 2), 2)
        elif d in (".word", ".word32"):
            for a in split_args(rest):
                if is_number(a) and halves:
                    nat.data(be_bytes([int(a, 0)], 4), 2)
                elif is_number(a):
                    nat.data(be_bytes([int(a, 0)], 4), 4, "word")
                else:
                    nat.sym(a)
        elif d == ".float":
            for a in split_args(rest):
                nat.data(list(struct.pack(">f", float(a))), 4)
        elif d == ".double":
            for a in split_args(rest):
                nat.data(list(struct.pack(">d", float(a))), 8)
        elif d in (".space", ".skip"):
            if section == ".bss":
                nat.line(f".space {rest}")
            else:
                nat.data([0] * int(rest, 0), 1)
        elif d in (".ascii", ".asciz", ".string"):
            nat.line(f"{d} {rest}")
        elif d == ".align":
            nat.line(f".p2align {rest}")
        elif d == ".balign":
            nat.line(f".balign {rest}")
        elif d in (".size", ".type", ".set", ".global", ".globl", ".local"):
            if d in (".global", ".globl"):
                nat.line(f".globl {rest}")
        else:
            sys.exit(f"asm2x86.py: {src}: unsupported directive {d}")
    return nat.emit()


def native_blob(src, label):
    data = open(src, "rb").read()
    widths = [1] * len(data)
    blob = BLOBS.get(label, [])
    for off, n, w in blob(data) if callable(blob) else blob:
        n = len(data) - off if n is None else n
        for k in range(off, off + n - n % w):
            widths[k] = w
    nat = Native(src)
    for t in [f".section .n64.{label},\"aw\",@progbits", f".globl {label}", f"{label}:"]:
        nat.line(t)
    k = 0
    while k < len(data):
        e = k
        while e < len(data) and widths[e] == widths[k]:
            e += 1
        nat.data(list(data[k:e]), widths[k])
        k = e
    return nat.emit()


BARE_SECTION = {".text": None, ".rdata": ".rodata", ".rodata": ".rodata", ".late_rodata": ".rodata",
                ".data": ".data", ".bss": ".bss"}


def global_asm_data(src, dst):
    """The data of a GLOBAL_ASM function's .s (asm-processor's `.rdata`,
    `.late_rodata`: its strings, floats and jump tables), which the N64
    build puts in the C file's object and the port has nowhere else, as a
    data file this script converts: each label a section of its own, since
    asm-processor puts `.rdata` and `.late_rodata` apart.  A jump table's
    `.word L<vram>_<rom>` becomes that address, which is what the
    translated `jr` compares with (tools/recomp/translate.py)."""
    out = []
    section = None
    for raw in open(src):
        s = raw.strip()
        if s in BARE_SECTION:
            section = BARE_SECTION[s]
            continue
        if section is None or not s or s.startswith(".late_rodata_alignment"):
            continue
        if re.match(r"(dlabel|glabel)\s", s):
            out.append(f".section {section}")
        m = re.match(r"\.word\s+(.*)$", s)
        if m:
            s = ".word " + ", ".join(
                f"0x{w.strip()[1:9]}" if re.match(r"L[0-9A-F]{8}_[0-9A-F]+$", w.strip()) else w.strip()
                for w in m.group(1).split(","))
        out.append(s)
    with open(dst, "w") as f:
        f.write("\n".join(out) + "\n")


def main():
    native = sys.argv[1] == "--native"
    if native:
        del sys.argv[1]
    if sys.argv[1] == "--global-asm":
        # asm2x86.py [--native] --global-asm OUT.s FUNC.s
        del sys.argv[1]
        tmp = sys.argv[1] + ".in.s"
        global_asm_data(sys.argv[2], tmp)
        sys.argv[2] = tmp
    dst, src = sys.argv[1], sys.argv[2]
    if native:
        if src.endswith(".bin"):
            out = native_blob(src, sys.argv[3])
        else:
            out = convert_native(src, sys.argv[3] if len(sys.argv) > 3 else None)
        with open(dst, "w") as f:
            f.write("/* generated by port/tools/asm2x86.py --native from " + src + " */\n")
            f.write("\n".join(out) + "\n")
        return
    if src.endswith(".bin"):
        label = sys.argv[3]
        out = [f".section .n64.{label},\"aw\",@progbits", f".globl {label}", f"{label}:",
               f".incbin \"{src}\""]
    else:
        label = sys.argv[3] if len(sys.argv) > 3 else None
        out = convert(src, label)
    with open(dst, "w") as f:
        f.write("/* generated by port/tools/asm2x86.py from " + src + " */\n")
        f.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
