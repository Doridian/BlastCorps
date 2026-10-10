#!/usr/bin/env python3
"""The handwritten objects' data as typed C, for the port.

  asm2c.py OUTDIR INPUT...

INPUT is one of
  .../asm/data/<module>/<object>.<kind>.s    a data file splat wrote
                                             (.data, .rodata, .bss, and
                                             the two pointer-bearing .text
                                             islands, <offset>.bin.s)
  LABEL=.../assets/<module>/<offset>.bin     a data island kept as .bin
  --global-asm .../asm/nonmatchings/...s     a GLOBAL_ASM function's data
                                             (asm-processor's .rdata and
                                             .late_rodata: jp's strings,
                                             floats and jump tables)

OUTDIR gets one C file per input (its path from asm/ with `/` as `_`, .c
for .s or .bin) and asm_data.h, which they all include: the types and a
declaration of every variable.  The port compiles them like the game's C
(the same flags, BEPass), so they are big-endian or native as the build is,
and gen_ld.py (or port-arena) puts each variable at its N64 address, as it
does the game's (docs/PORT.md, "The asm data as C").  The N64 build keeps
its asm; nothing here is committed, the data being the ROM's.

Each label is a variable, typed by the type inventory
(blastcorps/include/game/inventory.json, `symbols`) or, where that has
nothing, by its directives: u32 for .word, u16 for .short, u8 for .byte,
f32 for .float, a pointer for a symbol.  A variable runs to the next one:
what is left after its typed elements (a record cut short, a terminator,
the padding before the next) is a `tail` beside them in a struct of its
own.  A label inside another variable's type (D_8020C070[]'s, which runs
over the labels splat made inside it) is no variable: in the builds at
fixed addresses it is an alias, `.set` at its offset; the movable build's
arena link resolves its name to its N64 address, which is where it is.
A name the N64 link has inside a file with no label there (a data
island's, the strings the game's C names in the menus' text) is a label
that starts a variable of its own (n64_labels), so that the code reaches
what it names as a variable, not as an alias of an offset into another.
Where the code reads one table across several labels (a record per wheel,
a walk into the next label: LABEL_TYPES), the first label's type runs
over the others, which are names inside it.

A symbolic .word is a pointer initializer (`&D_X`, a cast where the
field's type is another, or `(u8 *)&D_X + n` into a variable).  A pointer
is native, as the C that reads it declares it (8 bytes in the LP64 build,
which lays out what holds one as the host does): the inventory's pointers
and POINTERS' (the words the C has as pointers), but where PTR32_* say
the C's are still 4 bytes.  A name inside a variable past a native
pointer is none in the LP64 build (its offset isn't the N64's there): the
C reaches it as an expression.  (Where pointers are 4 bytes it is the
alias, for the check build's translations.)

Where the data's units in the native-endian build come from (asm2x86.py
before; docs/PORT.md, "The native-endian build"): the type inventory's
widths, the islands' readers (ISLANDS, BLOBS), the u16 text of HALF_FILES;
each variable's C type is checked against them, unit for unit, so that the
native-endian builds keep exactly the bytes they had; a type that would
change them gives way to the directives' runs (asm_data_report.txt says
which).
"""

import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")
VERSION = os.environ.get("PORT_VERSION", "us.v11")
N64_NM = os.environ.get("PORT_N64_NM", "mips-linux-gnu-nm")

# ---- the type inventory -------------------------------------------------------

_GAME = os.path.join(BLAST, "include", "game")
INVENTORY = os.path.join(_GAME, f"inventory.{VERSION}.json")
if not os.path.exists(INVENTORY):
    INVENTORY = os.path.join(_GAME, "inventory.json")
_inv = None


def inventory():
    global _inv
    if _inv is None:
        _inv = json.load(open(INVENTORY))
        # (the engine's structures, TYPES, beside the inventory's)
        _inv["types"].update(TYPES)
        # (LABEL_TYPES over the inventory's types for those labels)
        for name, (t, count) in LABEL_TYPES.items():
            sym = _inv["symbols"].get(name)
            if sym is None or "storage" not in sym:
                continue
            _inv["symbols"][name] = {"type": t, "count": count, "storage": sym["storage"],
                                     "derived": "asm2c.py's LABEL_TYPES"}
    return _inv


SCALAR = {"s8": 1, "u8": 1, "s16": 2, "u16": 2, "s32": 4, "u32": 4, "f32": 4, "s64": 8, "u64": 8, "f64": 8,
          "ptr": 4, "ptr?": 4, "romoff": 4, "segptr": 4, "offset": 4, "phys": 4, "gfx": 4, "bytes": 1}
# the inventory's scalar kinds as C (an address kind that isn't a pointer is
# the number it is)
C_SCALAR = {"s8": "s8", "u8": "u8", "s16": "s16", "u16": "u16", "s32": "s32", "u32": "u32", "f32": "f32",
            "s64": "s64", "u64": "u64", "f64": "f64", "romoff": "u32", "segptr": "u32", "offset": "u32",
            "phys": "u32", "gfx": "u32", "ptr?": "u32", "bytes": "u8"}


def type_size(t):
    types = inventory()["types"]
    return types[t]["size"] if t in types else SCALAR[t]


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


def symbol_widths(rel):
    """{label: (size, [(offset, width)], clip)} for the symbols of the data
    file rel (asm/data/...) the inventory types"""
    inv = inventory()
    out = {}
    for name, sym in inv["symbols"].items():
        if sym.get("storage") != rel:
            continue
        ws = type_widths(sym["type"], inv["types"])
        if ws is None:
            continue
        size = type_size(sym["type"])
        count = sym.get("count", 1)
        # (fieldscan's records repeat up to the next symbol, the last one
        # possibly cut short: they stop at the next label)
        clip = "fieldscan" in sym.get("derived", "")
        out[name] = (size * count, [(k * size + o, w) for k in range(count) for o, w in ws], clip)
    return out


# ---- the islands' layouts -------------------------------------------------------

# the data islands kept as .bin, by label: [(offset, length, width)], the
# rest bytes.  (hd_code 68810 and 690C0 are the u16 tables embedded in
# Rare's math routines.)
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


# the two pointer-bearing data islands of hd_code's .text, which splat left
# as bytes and pointers (asm/data/hd_code/*.bin.s), laid out by how their
# readers walk them: [(offset, width)] from the file's start, the rest
# bytes.  `flat` is the file's bytes (a pointer's four are None).
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

# data objects that are u16 text, however splat wrote them (`.word`): the
# menus' Japanese text, which only jp shows (26570.c's D_803010A0 on)
HALF_FILES = ("hd_code/BC8E0.data.s",)

# The RSP microcode's text (hd_code's after ldiv, hd_front_end's at the end
# of its .text): the port's HLE never runs it and nothing else reads it, so
# the port carries zeros of its size (docs/DISTRIBUTION.md).  (Its data
# segments are kept: aspmain.c reads the audio microcode's resampling table.)
RSP_TEXT = {"hd_code_A0C30_bin", "hd_front_end_20090_bin"}

# Labels the code reads as one table, over the labels splat made inside it
# (docs/LAYOUT.md, "label"): {first label: (type, count)}, the others names
# inside it, so that the access goes through the variable it is in, wherever
# the variables go (PORT_SCATTER).
LABEL_TYPES = {
    # 62740's wheels' bookkeeping, three of each, wheel i at [i]:
    "D_803ED398": ("s32", 3),       # .. D_803ED3A0: the wheel's height
    "D_803ED3A8": ("s32", 3),       # .. D_803ED3B0: the ground's height under it
    "D_803ED3EA": ("bytes", 3),     # .. D_803ED3EC: the moving object it stands on
    "D_803ED3EE": ("bytes", 3),     # .. D_803ED3F0: in the air
    "D_803ED3F2": ("bytes", 3),     # .. D_803ED3F4: the material under it
    # 83910's barges, three of each (barge n's at [n], its position at [3n])
    "D_803F8748": ("s32", 9),
    "D_803F876C": ("ptr", 3),
    "D_803F8778": ("ptr", 6),
    "D_803F8790": ("bytes", 3),
    # 5FD50's visibility task: a SchedTask (0x60 bytes) whose last 0x20
    # bytes are the first of its DRAM stack, D_803BE780
    "D_803BE740": ("u64", 0x88),
    # func_802BF978 and func_802BFDAC walk the 12-byte chance records of
    # D_80306344, D_80306350 and D_803063D4 until a number is below a
    # threshold, and at 100 run off their end into D_803063E0 (kept
    # big-endian, as bytes)
    "D_80306344": ("bytes", 0xAC),
    # the list at D_803F3910 (ten pairs, func_802BEA30) runs over its end
    # into its end pointer D_803F3960, D_803F3964 and D_803F3968[0]
    "D_803F3910": ("HitList", 1),
    # the engine's types for the inventory's words (TYPES)
    "D_803BD310": ("Wall", 8),
    "D_803F7690": ("DelayedHit", 40),
    "D_803A7440": ("TexSlot", 12),
    "D_803B3500": ("TexPatch", 20),
    "D_803B35F8": ("UnkStruct_803ED460", 13),
    "D_803FBBE0": ("UnkStruct_803ED460", 32),
    "D_803FBEE0": ("UnkStruct_803ED460", 32),
    "D_803B7FC8": ("MtxCopy", 0x78),
    "D_803C4B70": ("EffectSlot", 16),
    "D_803EB7A0": ("VehicleSave", 1),
}


def _fields(*fs):
    out = []
    for f in fs:
        off, name, t = f[:3]
        d = {"off": off, "name": name, "type": t, "count": f[3] if len(f) > 3 else 1}
        if len(f) > 4:
            d["to"] = f[4]
        out.append(d)
    return out


# The engine's structures of the handwritten .bss whose pointers are
# native (port/engine's buildings.h and collision.h, 56040's texture slots
# and records, 60F60's sprite slots, 62740's saved vehicle), as inventory
# types (offsets and sizes the N64's): what LABEL_TYPES gives their
# variables.
TYPES = {
    "HitPair": {"size": 8, "align": 4, "fields": _fields((0, "b", "ptr", 1, "struct Building"), (4, "group", "s32"))},
    "HitList": {"size": 0x6E8, "align": 4, "fields": _fields(
        (0, "pairs", "HitPair", 10), (0x50, "end", "ptr", 1, "struct HitPair"), (0x54, "fall_mtx", "ptr", 1, "u8"),
        (0x58, "fx", "bytes", 30 * 0x38))},
    "Wall": {"size": 0xFC, "align": 4, "fields": _fields(
        (0, "nkinds", "u8"), (1, "kinds", "u8", 7), (8, "tris", "ptr", 0x3C, "struct CollisionTri"), (0xF8, "info", "u32"))},
    "DelayedHit": {"size": 8, "align": 4, "fields": _fields(
        (0, "b", "ptr", 1, "struct Building"), (4, "amount", "u16"), (6, "group", "u8"), (7, "frames", "u8"))},
    "TexSlot": {"size": 0x1010, "align": 4, "fields": _fields(
        (0, "owner", "ptr", 1, "struct TexAnim"), (4, "kind", "u16"), (6, "used", "u8"), (7, "refs", "u8"),
        (8, "pad8", "bytes", 8), (0x10, "data", "bytes", 0x1000))},
    "TexPatch": {"size": 0xC, "align": 4, "fields": _fields(
        (0, "anim", "ptr", 1, "struct TexAnim"), (4, "off", "s32"), (8, "idx", "s32"))},
    "MtxCopy": {"size": 0xC, "align": 4, "fields": _fields(
        (0, "from", "ptr", 1, "u32"), (4, "to", "ptr", 1, "u32"), (8, "frame", "u32"))},
    "EffectSlot": {"size": 0x3C, "align": 4, "fields": _fields(
        (0, "anim", "ptr", 1, "u8"), (4, "unk4", "s32"), (8, "pos", "s32", 3), (0x14, "unk14", "s32", 3),
        (0x20, "vel", "s32", 3), (0x2C, "unk2C", "s32"), (0x30, "unk30", "u8"), (0x31, "unk31", "u8"),
        (0x32, "frame", "u8"), (0x33, "active", "u8"), (0x34, "mode", "u8"), (0x35, "unk35", "u8"),
        (0x36, "unk36", "u8"), (0x37, "cells", "u8", 4), (0x3B, "unk3B", "u8"))},
    # (pos: three unaligned words, as bytes)
    "VehicleSave": {"size": 0x3B8, "align": 4, "fields": _fields(
        (0, "parts", "UnkStruct_803ED460", 32), (0x300, "state", "bytes", 0xA6), (0x3A6, "pos", "bytes", 12))},
}


# The pointer variables of the handwritten objects' data that the inventory
# has as words (fieldscan saw a word loaded and stored): {name: (what it
# points to, count)}, count None for as many as its room holds (a table to
# a sentinel, whose other words are 0 or -1).  A pointer is native, as the
# C declares it (port/engine, game/*.h: decl_check.py compares them); the
# room the N64 has after it is left out (.bss, nothing there).  The
# pointee is the C's type, a structure by its tag where asm_data.h has no
# typedef of it.
POINTERS = {
    # 56040
    "D_803A7408": ("s8", 1), "D_803B35F0": ("struct TexPatch", 1), "D_803B3770": ("u8", 1),
    "D_803B8568": ("struct MtxCopy", 2),
    # 5BF40, 5CB60
    "D_803B8D40": ("struct TexCacheEntry", 1), "D_803B8D44": ("struct TextureEntry", 1),
    "D_803BD300": ("struct CollisionTri", 1), "D_803BD304": ("struct TriSwitch", 1),
    "D_803BD308": ("struct CollisionTri", 1), "D_803BD30C": ("struct CollisionTri", 1),
    "D_803BDFD4": ("struct LevelLight", 1), "D_803BE6F4": ("u8", 1), "D_803BE6F8": ("void", 1),
    "D_803BE704": ("u8", 1),
    "D_803BE708": ("u8", 1),
    # the level's grids (game/level.h): D_803BDE40's cells and their end
    # need 102, the N64's 102nd is D_803BDFD4 (set after): the port has one
    # more where pointers aren't the N64's 4 bytes (count, extra)
    "D_803BDB08": ("void", 1), "D_803BDB10": ("u8", 102), "D_803BDCA8": ("struct CollisionTri", 102),
    "D_803BDE40": ("struct CollisionTri", (101, 1)),
    # 5FD50, 60F60
    "D_803C2B88": ("struct QuadNode", 1), "D_803C3170": ("s16", 1),
    "D_803C4B50": ("struct TexDecode", 1), "D_803C4B54": ("struct TexDecode", 1),
    "D_803EB780": ("Gfx", 1), "D_803EB784": ("Gfx", 1), "D_803EB788": ("u8", 1), "D_803EB78C": ("u8", 1),
    # 62740
    "D_803EBBEC": ("u8", 1), "D_803EBC00": ("u8", 1), "D_803EBC04": ("u8", 1), "D_803EBC08": ("u8", 1),
    # the vehicle modules' model files and buffers, and their sounds (below)
    "D_803ED82C": ("u8", 1), "D_803ED830": ("u8", 1),
    "D_803EDBF4": ("u8", 1), "D_803EDBF8": ("u8", 1), "D_803EDBFC": ("u8", 1),
    "D_803EDFC4": ("u8", 1), "D_803EDFC8": ("u8", 1), "D_803EDFCC": ("u8", 1), "D_803EDFD0": ("struct SndState", 1),
    "D_803EE388": ("struct SndState", 1), "D_803EE398": ("u8", 1), "D_803EE39C": ("u8", 1),
    "D_803EE3A0": ("u8", 1), "D_803EE774": ("u8", 1), "D_803EE778": ("u8", 1), "D_803EE77C": ("u8", 1),
    "D_803EEB44": ("u8", 1), "D_803EEB48": ("u8", 1), "D_803EEB4C": ("u8", 1),
    "D_803EEF24": ("u8", 1), "D_803EEF28": ("u8", 1), "D_803EEF2C": ("u8", 1), "D_803EF2E8": ("struct SndState", 1),
    "D_803EF2F8": ("u8", 1), "D_803EF2FC": ("u8", 1), "D_803EF300": ("u8", 1),
    "D_803EF6D8": ("struct SndState", 1), "D_803EF704": ("u8", 1), "D_803EF708": ("u8", 1),
    "D_803EF70C": ("u8", 1), "D_803EFAD4": ("u8", 1), "D_803EFAD8": ("struct SndState", 1),
    "D_803EFADC": ("struct SndState", 1), "D_803EFAE0": ("struct SndState", 1), "D_803EFAE4": ("u8", 1),
    "D_803EFAE8": ("u8", 1), "D_803EFEA4": ("u8", 1), "D_803EFEA8": ("u8", 1), "D_803EFEAC": ("u8", 1),
    # 77E20
    "D_803F7658": ("Mtx", 1), "D_803F765C": ("Mtx", 1), "D_803F77D0": ("UnkStruct_803ED460", 1),
    "D_803F77D4": ("u8", 1), "D_803F77E4": ("u8", 1), "D_803F7820": ("u8", 1), "D_803F7824": ("u8", 1),
    "D_803F7828": ("void", 1), "D_803F782C": ("void", 1),
    # 7F8B0, 80280
    "D_803F7830": ("u8", 1), "D_803F7834": ("u8", 1), "D_803F7844": ("struct SndState", 1),
    "D_803F7848": ("struct SndState", 1), "D_803F7C04": ("u8", 1), "D_803F7C08": ("u8", 1),
    "D_803F7C0C": ("u8", 1), "D_803F7C18": ("struct SndState", 1), "D_803F7C1C": ("struct SndState", 1),
    # 83910 (LABEL_TYPES), 853D0, 86F60, 88160
    "D_803F876C": ("u8", 3), "D_803F8778": ("u8", 6),
    "D_803F8B54": ("u8", 1), "D_803F8B58": ("u8", 1), "D_803F8B5C": ("u8", 1),
    "D_803F8F34": ("u8", 1), "D_803F8F38": ("u8", 1), "D_803F8F3C": ("u8", 1),
    "D_803F9304": ("u8", 1), "D_803F9308": ("u8", 1), "D_803F930C": ("u8", 1),
    # 8A080, 8A2E0, 8AEE0, 8DDB0
    "D_803FB8B0": ("struct CollisionTri", 1), "D_803FBBD8": ("u8", 1),
    "D_803FC1E0": ("u8", 1), "D_803FC1E4": ("u8", 1), "D_803FC1E8": ("u8", 1), "D_803FC1EC": ("u8", 1),
    "D_803FC5B4": ("u8", 1), "D_803FC5B8": ("u8", 1), "D_803FC5BC": ("u8", 1),
    "D_803FC984": ("u8", 1), "D_803FC988": ("u8", 1), "D_803FC98C": ("u8", 1), "D_803FC990": ("struct SndState", 1),
    "D_803FCD54": ("u8", 1), "D_803FCD58": ("u8", 1), "D_803FCD5C": ("u8", 1), "D_803FCD64": ("struct SndState", 1),
    # .data: tables of pointers to a sentinel (60F60's, 77E20's, 7D9D0's, 800DC's)
    "D_80305C10": ("void", None), "D_80306270": ("u8", None),
    "D_802C23B4": ("struct TexAnim", None), "D_802C3FFC": ("u8", None),
    "D_802C4A20": ("struct UnkStruct_8036EC30", None),
}

# Pointers kept 4 bytes (PTR32) while the C's are: the display list's
# words (P8).
PTR32_POINTEES = set()
PTR32_TYPES = set()
PTR32_VARS = {"D_80300A68"}


def keep32(t):
    """t with its pointers PTR32"""
    if isinstance(t, Scalar):
        t.ptr32 = t.kind == "ptr"
    elif isinstance(t, Array):
        keep32(t.elem)
    elif isinstance(t, Struct):
        for _, _, m in t.members:
            keep32(m)
    return t


# ---- parsing --------------------------------------------------------------------

def split_args(s):
    return [a.strip() for a in s.split(",") if a.strip()]


def is_number(a):
    try:
        int(a, 0)
        return True
    except ValueError:
        return False


def unescape(s):
    """a GNU as string literal's bytes"""
    out = bytearray()
    k = 0
    while k < len(s):
        c = s[k]
        if c != "\\":
            out += c.encode("latin-1")
            k += 1
            continue
        k += 1
        c = s[k]
        simple = {"n": 10, "t": 9, "r": 13, "b": 8, "f": 12, "v": 11, "\\": 92, '"': 34, "'": 39}
        if c in simple:
            out.append(simple[c])
            k += 1
        elif c in "01234567":
            m = re.match(r"[0-7]{1,3}", s[k:])
            out.append(int(m.group(0), 8) & 0xFF)
            k += len(m.group(0))
        elif c == "x":
            m = re.match(r"[0-9A-Fa-f]+", s[k + 1:])
            out.append(int(m.group(0), 16) & 0xFF)
            k += 1 + len(m.group(0))
        else:
            out += c.encode("latin-1")
            k += 1
    return bytes(out)


class Blob:
    """A data file as bytes: val[i] (None in a symbol's word), the
    directive each byte came from, the labels and the symbolic words; then
    the width of each byte's unit in the native-endian build (resolve)."""

    def __init__(self, name, src, bss=False):
        self.name, self.src, self.bss = name, src, bss
        self.val, self.dirw, self.dstart, self.dkind = [], [], [], []
        self.syms = {}          # offset -> symbol
        self.labels = []        # [(offset, name)]
        self.named = set()      # the labels the N64 link names (n64_labels)
        self.breaks = set()     # offsets where asm2x86 started a new run (labels, symbols)

    def data(self, bs, width, kind):
        for k, b in enumerate(bs):
            self.val.append(b)
            self.dirw.append(width)
            self.dstart.append(k % width == 0)
            self.dkind.append(kind)

    def sym(self, name):
        self.syms[len(self.val)] = name
        self.breaks.add(len(self.val))
        self.breaks.add(len(self.val) + 4)
        for k in range(4):
            self.val.append(None)
            self.dirw.append(4)
            self.dstart.append(k == 0)
            self.dkind.append("sym")

    def label(self, name):
        self.labels.append((len(self.val), name))
        self.breaks.add(len(self.val))

    def __len__(self):
        return len(self.val)


def be(values, size):
    out = []
    for v in values:
        out += list((v & ((1 << (8 * size)) - 1)).to_bytes(size, "big"))
    return out


def parse_s(src, rel, label=None, lines=None):
    """a data file (or its lines) as a Blob"""
    halves = bool(rel) and rel.endswith(HALF_FILES)
    blob = None
    name = None
    for raw in lines if lines is not None else open(src):
        line = raw.split("#", 1)[0].rstrip() if not raw.lstrip().startswith(".ascii") else raw.rstrip()
        s = line.strip()
        if not s or s.startswith("/*") or s.startswith(".include"):
            continue
        m = re.match(r"\.section\s+(\S+)", s)
        if m:
            sec = m.group(1).rstrip(",")
            if blob is None:
                blob = Blob(name, src, sec == ".bss")
            if label:
                blob.label(label)
                label = None
            continue
        if blob is None:
            sys.exit(f"asm2c.py: {src}: data before a .section")
        m = re.match(r"(dlabel|glabel)\s+(\S+)", s)
        if m:
            blob.label(m.group(2))
            continue
        if re.match(r"(\.L\w+):$", s):
            continue
        m = re.match(r"(\.\w+)\s*(.*)$", s)
        if not m:
            sys.exit(f"asm2c.py: {src}: can't parse {s!r}")
        d, rest = m.group(1), m.group(2)
        if d == ".byte":
            blob.data(be([int(a, 0) for a in split_args(rest)], 1), 1, "byte")
        elif d in (".short", ".half"):
            blob.data(be([int(a, 0) for a in split_args(rest)], 2), 2, "half")
        elif d in (".word", ".word32"):
            for a in split_args(rest):
                if is_number(a) and halves:
                    blob.data(be([int(a, 0)], 4), 2, "half")
                elif is_number(a):
                    blob.data(be([int(a, 0)], 4), 4, "word")
                else:
                    blob.sym(a)
        elif d == ".float":
            for a in split_args(rest):
                blob.data(list(struct.pack(">f", float(a))), 4, "float")
        elif d == ".double":
            for a in split_args(rest):
                blob.data(list(struct.pack(">d", float(a))), 8, "double")
        elif d in (".space", ".skip"):
            blob.data([0] * int(rest, 0), 1, "byte")
        elif d in (".ascii", ".asciz", ".string"):
            for m2 in re.finditer(r'"((?:[^"\\]|\\.)*)"', rest):
                bs = unescape(m2.group(1)) + (b"" if d == ".ascii" else b"\0")
                blob.data(list(bs), 1, "char")
        elif d in (".align", ".balign"):
            n = 1 << int(rest, 0) if d == ".align" else int(rest, 0)
            blob.data([0] * (-len(blob) % n), 1, "byte")
        elif d in (".size", ".type", ".set", ".global", ".globl", ".local"):
            pass
        else:
            sys.exit(f"asm2c.py: {src}: unsupported directive {d}")
    return blob


BARE_SECTION = {".text": None, ".rdata": ".rodata", ".rodata": ".rodata", ".late_rodata": ".rodata",
                ".data": ".data", ".bss": ".bss"}


def parse_global_asm(src):
    """The data of a GLOBAL_ASM function's .s (asm-processor's `.rdata`,
    `.late_rodata`: its strings, floats and jump tables), which the N64
    build puts in the C file's object and the port has nowhere else: a
    variable per label.  A jump table's `.word L<vram>_<rom>` becomes that
    address, which is what the translated `jr` compares with
    (tools/recomp/translate.py)."""
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
            out.append(".section .data")
        m = re.match(r"\.word\s+(.*)$", s)
        if m:
            s = ".word " + ", ".join(
                f"0x{w.strip()[1:9]}" if re.match(r"L[0-9A-F]{8}_[0-9A-F]+$", w.strip()) else w.strip()
                for w in m.group(1).split(","))
        out.append(s)
    # each label its own section, as asm-processor has them: the gaps between
    # them (the alignment) are no one's
    blobs = []
    cur = []
    for line in out:
        if line == ".section .data":
            if cur:
                blobs.append(cur)
            cur = []
        cur.append(line)
    if cur:
        blobs.append(cur)
    res = []
    for lines in blobs:
        b = parse_s(src, None, lines=lines)
        if b is not None and len(b):
            res.append(b)
    return res


def parse_bin(path, label):
    data = open(path, "rb").read()
    if label in RSP_TEXT:
        data = bytes(len(data))
    blob = Blob(label, path)
    blob.label(label)
    blob.data(list(data), 1, "byte")
    return blob


def n64_labels(blob, nm):
    """The names the N64 link has inside the blob that its file has no
    label for, as labels: a data island's (splat keeps it as bytes, the code
    names what it reads in it), and what the game's C names inside an asm
    data file (module_syms_auto: the menus' Japanese strings, a text after a
    display list).  Each starts a variable (Gen.partition): the code that
    names it reads it as a thing of its own, so it is one in the port, not a
    name for an offset into the variable before it."""
    base = next((nm[n][0] - o for o, n in blob.labels if n in nm), None)
    if base is None:
        return
    have = {n for _, n in blob.labels}
    end = base + len(blob)
    for name, (a, fn) in sorted(nm.items(), key=lambda kv: kv[1][0]):
        if base <= a < end and not fn and name.startswith("D_") and name not in have:
            off = a - base
            if off == 0:
                # (an island's own name, where one of the code's is: the code's)
                blob.labels = [(o, n) for o, n in blob.labels if o or not n.endswith("_bin")]
            blob.labels.append((off, name))
            blob.breaks.add(off)
            blob.named.add(name)
    blob.labels.sort(key=lambda x: x[0])


# ---- the native-endian units (what asm2x86.py --native wrote) -------------------

def resolve(blob, rel, island=None, blob_layout=None):
    """blob.w[i], blob.st[i]: the width of byte i's unit (1, 2, 4 or 8) and
    whether it starts it"""
    n = len(blob)
    w = list(blob.dirw)
    st = list(blob.dstart)
    data = [v is not None for v in blob.val]
    if blob_layout is not None:
        w = [1] * n
        st = [True] * n
        lay = blob_layout(bytes(blob.val)) if callable(blob_layout) else blob_layout
        for off, cnt, wd in lay:
            cnt = n - off if cnt is None else cnt
            for k in range(off, off + cnt - cnt % wd):
                w[k] = wd
                st[k] = (k - off) % wd == 0
    if island:
        for k in range(n):
            if data[k]:
                w[k], st[k] = 1, True
        for off, wd in island(blob.val):
            if off + wd <= n and all(data[off + j] for j in range(wd)):
                for j in range(wd):
                    w[off + j], st[off + j] = wd, j == 0
    overrides = symbol_widths(rel) if rel else {}
    labels = {nm: off for off, nm in blob.labels}
    for name, (size, ws, clip) in sorted(overrides.items(), key=lambda kv: (kv[1][0], kv[0])):
        if name not in labels:
            continue
        base = labels[name]
        if clip:
            size = min([size] + [p - base for p in labels.values() if p > base])
            ws = [(o, x) for o, x in ws if o + x <= size]
        for b in range(min(size, n - base)):
            if data[base + b] and blob.dkind[base + b] == "word":
                w[base + b], st[base + b] = 1, True
        for off, x in ws:
            if base + off + x > n:
                continue
            if not all(data[base + off + b] for b in range(x)):
                continue
            for b in range(x):
                w[base + off + b], st[base + off + b] = x, b == 0
    # a unit is one only if all its bytes say so, in one run (labels and
    # symbols start runs): else its bytes are bytes
    eff_w = [1] * n
    eff_st = [True] * n
    k = 0
    while k < n:
        if not data[k]:
            eff_w[k], eff_st[k] = 4, k in blob.syms
            k += 1
            continue
        x = w[k]
        ok = x > 1 and st[k] and k + x <= n and \
            all(data[k + j] and w[k + j] == x and not st[k + j] and (k + j) not in blob.breaks
                for j in range(1, x))
        if ok:
            for j in range(x):
                eff_w[k + j], eff_st[k + j] = x, j == 0
            k += x
        else:
            k += 1
    blob.w, blob.st = eff_w, eff_st


# ---- C types --------------------------------------------------------------------

class Scalar:
    """A scalar; its size and alignment are the N64's (a pointer's 4: the
    data's units), which a native pointer's C type has only where pointers
    are 4 bytes (native() says where they aren't)."""

    def __init__(self, kind, pointee=None, ptr32=False):
        self.kind = kind            # u8 .. f64, or "ptr"
        self.pointee = pointee      # C type the pointer points to
        self.ptr32 = ptr32          # a pointer kept 4 bytes (PTR32) in the LP64 build
        self.size = {"char": 1, "s8": 1, "u8": 1, "s16": 2, "u16": 2, "s32": 4, "u32": 4, "f32": 4,
                     "s64": 8, "u64": 8, "f64": 8, "ptr": 4}[kind]
        self.align = self.size

    def decl(self, name):
        if self.kind == "ptr":
            return f"{self.pointee} *{'PTR32 ' if self.ptr32 else ''}{name}"
        return f"{self.kind} {name}"

    def units(self):
        return [(0, self.size, self)]

    def native(self):
        """the offset of its first native pointer (None: none)"""
        return 0 if self.kind == "ptr" and not self.ptr32 else None


class Array:
    def __init__(self, elem, n, extra=0):
        self.elem, self.n = elem, n
        self.extra = extra          # more elements where pointers aren't 4 bytes (a top-level .bss variable)
        self.size = elem.size * n
        self.align = elem.align

    def decl(self, name):
        if self.extra:
            return self.elem.decl(f"{name}[{self.n} + {self.extra} * (sizeof(void *) != 4)]")
        return self.elem.decl(f"{name}[{self.n}]")

    def units(self):
        sub = self.elem.units()
        return [(k * self.elem.size + o, w, s) for k in range(self.n) for o, w, s in sub]

    def native(self):
        return self.elem.native()


class Struct:
    def __init__(self, name, members, size, union=False):
        self.name = name            # the C type's name
        self.members = members      # [(offset, name, type)]
        self.size = size
        self.union = union
        self.packed = False
        self.align = max([m[2].align for m in members] or [1])
        if not union:
            off = 0
            for o, _, t in members:
                if o % t.align or o < off:
                    self.packed = True
                off = o + t.size
            if self.size % self.align:
                self.packed = True
        if self.packed:
            self.align = 1

    def decl(self, name):
        return f"{self.name} {name}"

    def units(self):
        out = []
        if self.union:
            return self.members[0][2].units() if self.members else []
        for o, _, t in self.members:
            out += [(o + uo, w, s) for uo, w, s in t.units()]
        return out

    def native(self):
        offs = [o + n for o, _, t in self.members for n in [t.native()] if n is not None]
        return min(offs) if offs else None

    def definition(self):
        kw = "union" if self.union else "struct"
        lines = [f"typedef {kw} {self.name} {{"]
        for o, n, t in self.members:
            lines.append(f"    /* 0x{o:02X} */ {t.decl(n)};")
        if self.packed and self.native() is not None:
            # (a packed layout is the N64's, which native pointers don't keep)
            raise SystemExit(f"asm2c.py: {self.name}: a packed structure with native pointers")
        attr = " __attribute__((packed))" if self.packed else ""
        lines.append(f"}}{attr} {self.name};")
        if not self.union:
            # (the N64's size where pointers are 4 bytes: with native ones, the
            # LP64 build lays it out as the C does, from the same members)
            cond = "sizeof(void *) != 4 || " if self.native() is not None else ""
            lines.append(f"typedef char {self.name}_size_check[{cond}sizeof({self.name}) == 0x{self.size:X} ? 1 : -1];")
        return "\n".join(lines)


IDENT = re.compile(r"^[A-Za-z_]\w*$")


class Types:
    """The C types of the inventory's types, made as they are needed."""

    def __init__(self):
        self.inv = inventory()
        self.made = {}          # name -> Struct
        self.order = []         # their definitions, dependencies first
        self.forward = set()    # names only pointed to

    def pointee(self, to):
        if to and to.startswith("struct ") and IDENT.match(to[7:]):
            return to           # (by its tag: TYPES', whose definition may come after)
        if to in (None, "void") or not IDENT.match(to or ""):
            return "void"
        if to in C_SCALAR:
            return C_SCALAR[to]
        if to == "char":
            return "char"
        self.forward.add(to)
        return to

    def field(self, f):
        t = f["type"]
        n = f.get("count", 1)
        if t == "ptr":
            e = Scalar("ptr", self.pointee(f.get("to")))
        elif t in C_SCALAR:
            e = Scalar(C_SCALAR[t])
        elif t in self.inv["types"]:
            e = self.struct(t)
        else:
            raise KeyError(t)
        return Array(e, n) if n != 1 or t == "bytes" else e

    def struct(self, name, fill=None):
        """the inventory's type `name` as a Struct; fill(off, size) gives the
        members for the bytes no field covers (default: bytes)"""
        if name in self.made:
            return self.made[name]
        ty = self.inv["types"][name]
        members = []
        covered = 0
        fields = sorted(ty["fields"], key=lambda f: f["off"])
        if ty.get("union"):
            for f in fields:
                members.append((0, f["name"], self.field(f)))
            st = Struct(name, members, ty["size"], union=True)
        else:
            for f in fields:
                if f["off"] < covered:
                    continue            # (overlapping fields: the first)
                if f["off"] > covered:
                    members += (fill or bytes_fill)(covered, f["off"] - covered)
                t = self.field(f)
                members.append((f["off"], f["name"], t))
                covered = f["off"] + t.size
            if covered < ty["size"]:
                members += (fill or bytes_fill)(covered, ty["size"] - covered)
            st = Struct(name, members, ty["size"])
        if name in PTR32_TYPES:
            keep32(st)
        self.made[name] = st
        self.order.append(st)
        return st


def bytes_fill(off, size):
    return [(off, f"pad{off:X}", Array(Scalar("u8"), size))]


# ---- the variables ----------------------------------------------------------------

class Var:
    def __init__(self, blob, off, end, name):
        self.blob, self.off, self.end, self.name = blob, off, end, name
        self.ctype = None
        self.inner = []         # [(offset from the variable, name)]


def kind_at(blob, k, w):
    """the C scalar for the unit of width w at byte k, by its directive"""
    if k in blob.syms:
        return Scalar("ptr", "void")
    d = blob.dkind[k]
    if w == 8:
        return Scalar("f64" if d == "double" else "u64")
    if w == 4:
        return Scalar("f32" if d == "float" else "u32")
    if w == 2:
        return Scalar("u16")
    return Scalar("char" if d == "char" else "u8")


class UnitError(Exception):
    pass


def runs(blob, a, b):
    """[(offset from a, Scalar, count)]: the units of [a, b) in runs"""
    out = []
    k = a
    while k < b:
        if k >= len(blob) or not blob.st[k] or k + blob.w[k] > b:
            raise UnitError(f"{blob.src}: a unit across +0x{k:X}")
        x = blob.w[k]
        sc = kind_at(blob, k, x)
        if out and (out[-1][1].kind, out[-1][1].size, out[-1][1].pointee) == (sc.kind, sc.size, sc.pointee):
            out[-1][2] += 1
        else:
            out.append([k - a, sc, 1])
        k += x
    return [tuple(r) for r in out]


def runs_type(blob, a, b, name):
    rs = runs(blob, a, b)
    if len(rs) == 1:
        o, sc, n = rs[0]
        return Array(sc, n) if n > 1 or sc.kind in ("u8", "char") else sc
    members = [(o, f"x{o:X}", Array(sc, n) if n > 1 else sc) for o, sc, n in rs]
    return Struct(name, members, b - a)


def units_match(blob, base, ctype):
    """does ctype at blob offset base have the units the native-endian build
    had there, and pointers where the symbols are?  (None if so, else why)"""
    end = base + ctype.size
    if end > len(blob):
        return "past the file's end"
    have = {base + o: (w, sc) for o, w, sc in ctype.units()}
    for k in range(base, end):
        h = have.get(k)
        if not blob.st[k]:
            if h is not None:
                return f"+0x{k - base:X}: a {h[0]}-byte unit inside one of {blob.w[k]}"
            continue
        x = blob.w[k]
        if h is None:
            return f"+0x{k - base:X}: a {x}-byte unit inside the type's"
        if h[0] != x:
            return f"+0x{k - base:X}: {h[0]} bytes where the data has {x}"
        if k in blob.syms and h[1].kind != "ptr":
            return f"+0x{k - base:X}: a pointer in a {h[1].kind}"
        if h[1].kind == "ptr" and k not in blob.syms and blob.val[k] is not None:
            v = int.from_bytes(bytes(blob.val[k:k + 4]), "big")
            if v:
                return f"+0x{k - base:X}: a pointer field holding 0x{v:08X}"
    return None


class Gen:
    def __init__(self, nm):
        self.types = Types()
        self.nm = nm            # N64 name -> (address, is a function)
        self.vars = []          # every top-level variable, all files
        self.extern = {}        # names the initializers use that no file here defines
        self.log = []
        self.stats = {"inventory": 0, "directives": 0, "bss": 0, "pointers": 0}

    # -- partition a blob into variables
    def partition(self, blob, rel):
        """the blob's variables: a label starts one unless it is inside the
        one before by that one's type (the inventory's records repeat over
        the labels splat made inside them, but fieldscan's stop at the next
        label, as asm2x86.py had them), or has nothing after it but another
        label (then it is the next one's alias); each runs to the next"""
        inv = inventory()["symbols"]
        labels = sorted(blob.labels, key=lambda x: x[0])
        # (u16 text: a string a label, wherever the inventory's count, which
        # is us.v11's, runs)
        text = bool(rel) and rel.endswith(HALF_FILES)
        n = len(blob)
        vars_, pending = [], []
        cur, cur_end = None, 0
        for k, (off, name) in enumerate(labels):
            nxt = next((o for o, _ in labels[k + 1:] if o > off), n)
            if cur is not None and (off < cur_end or off >= n) and \
                    not (off < n and (name in blob.named or text)):
                cur.inner.append((off - cur.off, name))
                continue
            if off >= n:
                continue
            if any(o == off for o, _ in labels[k + 1:]):
                pending.append(name)
                continue
            cur = Var(blob, off, nxt, name)
            cur.inner += [(0, p) for p in pending]
            pending = []
            cur_end = nxt
            sym = inv.get(name)
            if sym and sym.get("storage") == rel and "fieldscan" not in sym.get("derived", ""):
                cur_end = max(nxt, min(off + type_size(sym["type"]) * sym.get("count", 1), n))
            vars_.append(cur)
        for a, b in zip(vars_, vars_[1:]):
            a.end = b.off
        if vars_:
            vars_[-1].end = n
        return vars_

    # -- a variable's C type
    def var_type(self, v, rel):
        if v.name in POINTERS:
            return self.pointer_type(v), False
        t, wrapped = self.var_type_(v, rel)
        if v.name in PTR32_VARS:
            keep32(t)
        return t, wrapped

    def pointer_type(self, v):
        """POINTERS' variable: its pointers, each word of the data a symbol,
        0 or -1 (a sentinel)"""
        to, n = POINTERS[v.name]
        n, extra = n if isinstance(n, tuple) else (n, 0)
        blob, span = v.blob, v.end - v.off
        cnt = span // 4 if n is None else n
        if cnt == 0 or cnt * 4 > span:
            sys.exit(f"asm2c.py: {v.name}: {cnt} pointers in its 0x{span:X} bytes")
        if not blob.bss:
            for a in range(v.off, v.off + 4 * cnt, 4):
                if a in blob.syms:
                    continue
                w = blob.val[a:a + 4]
                if None in w or int.from_bytes(bytes(w), "big") not in (0, 0xFFFFFFFF):
                    sys.exit(f"asm2c.py: {v.name}+0x{a - v.off:X}: neither a symbol, 0 nor -1")
        self.stats["inventory"] += 1
        if blob.bss:
            self.stats["bss"] += 1
        sc = Scalar("ptr", to, ptr32=to in PTR32_POINTEES)
        return Array(sc, cnt, extra) if n != 1 else sc

    def var_type_(self, v, rel):
        blob = v.blob
        span = v.end - v.off
        sym = inventory()["symbols"].get(v.name)
        if blob.bss:
            self.stats["bss"] += 1
            return self.bss_type(v, sym, span, rel)
        why = None
        if sym and sym.get("storage") == rel:
            t = sym["type"]
            try:
                elem = self.elem_type(t, sym, blob, v.off)
            except KeyError as e:
                elem, why = None, f"no type {e}"
            if elem is not None:
                cnt = span // elem.size
                tail = span - cnt * elem.size
                if cnt == 0:
                    why = f"{t} (0x{elem.size:X} bytes) is bigger than its 0x{span:X}"
                else:
                    body = Array(elem, cnt) if (cnt > 1 or t == "bytes" or sym.get("count", 1) > 1) else elem
                    why = units_match(blob, v.off, body)
                    if why is None and tail:
                        tt = runs_type(blob, v.off + body.size, v.end, f"{v.name}_tail")
                        if isinstance(tt, Struct):
                            tt.name = f"{v.name}_tail"
                            self.types.order.append(tt)
                        st = Struct(f"{v.name}_t", [(0, v.name, body), (body.size, "tail", tt)], span)
                        self.types.order.append(st)
                        self.stats["inventory"] += 1
                        return st, True
                    if why is None:
                        self.stats["inventory"] += 1
                        return body, False
            if why:
                self.log.append(f"{v.name} ({rel}): {t} doesn't fit ({why}): typed by its directives")
        self.stats["directives"] += 1
        t = runs_type(blob, v.off, v.end, f"{v.name}_t")
        if isinstance(t, Struct):
            self.types.order.append(t)
        return t, False

    def elem_type(self, t, sym, blob, off):
        if t == "ptr":
            return Scalar("ptr", self.types.pointee(sym.get("to")))
        if t == "bytes":
            return Scalar("char") if blob.dkind[off] == "char" else Scalar("u8")
        if t in C_SCALAR:
            return Scalar(C_SCALAR[t])
        if t.startswith("asm_"):
            ty = self.types.inv["types"][t]
            fs = ty["fields"]
            if len(fs) == 1 and fs[0]["off"] == 0 and fs[0]["type"] in C_SCALAR and \
                    SCALAR[fs[0]["type"]] * fs[0].get("count", 1) == ty["size"]:
                # one field: the field's type
                sc = Scalar("ptr", "void") if off in blob.syms else Scalar(C_SCALAR[fs[0]["type"]])
                return Array(sc, fs[0]["count"]) if fs[0].get("count", 1) > 1 else sc
            # its own type: the holes as the data has them, a pointer
            # where a symbol is
            def fill(o, size):
                try:
                    rs = runs(blob, off + o, off + o + size)
                except UnitError:
                    return bytes_fill(o, size)
                return [(o + ro, f"x{o + ro:X}", Array(sc, cnt) if cnt > 1 else sc) for ro, sc, cnt in rs]
            st = self.types.struct(t, fill)
            # a field holding a symbol is a pointer
            for i, (mo, mn, mt) in enumerate(st.members):
                if isinstance(mt, Scalar) and mt.size == 4 and (off + mo) in blob.syms and mt.kind != "ptr":
                    st.members[i] = (mo, mn, Scalar("ptr", "void"))
            return st
        return self.types.struct(t)

    def bss_type(self, v, sym, span, rel):
        if sym and sym.get("storage") == rel:
            try:
                t = sym["type"]
                elem = Scalar("ptr", self.types.pointee(sym.get("to"))) if t == "ptr" else \
                    Scalar(C_SCALAR[t]) if t in C_SCALAR else self.types.struct(t)
            except KeyError:
                elem = None
            if elem is not None and elem.size <= span:
                cnt = span // elem.size
                tail = span - cnt * elem.size
                body = Array(elem, cnt) if (cnt > 1 or t == "bytes" or sym.get("count", 1) > 1) else elem
                if not tail:
                    self.stats["inventory"] += 1
                    return body, False
                st = Struct(f"{v.name}_t", [(0, v.name, body), (body.size, "tail", Array(Scalar("u8"), tail))],
                            span)
                self.types.order.append(st)
                self.stats["inventory"] += 1
                return st, True
            if elem is not None:
                self.log.append(f"{v.name} ({rel}): {t} (0x{elem.size:X}) is bigger than its 0x{span:X}: bytes")
        self.stats["directives"] += 1
        return Array(Scalar("u8"), span), False

    # -- initializers
    def value(self, blob, k, sc):
        if sc.kind == "ptr":
            if k in blob.syms:
                return self.pointer(blob.syms[k], sc.pointee)
            v = int.from_bytes(bytes(blob.val[k:k + 4]), "big")
            if v == 0xFFFFFFFF:
                return f"({sc.pointee} *)-1"        # (a sentinel: all ones in the LP64 build too)
            return "0" if not v else f"({sc.pointee} *)0x{v:08X}"
        raw = bytes(blob.val[k:k + sc.size])
        v = int.from_bytes(raw, "big")
        if sc.kind == "f32":
            return c_float(v, 4)
        if sc.kind == "f64":
            return c_float(v, 8)
        if sc.kind[0] == "s" and v >= 1 << (8 * sc.size - 1):
            v -= 1 << (8 * sc.size)
        if sc.kind == "char":
            if v >= 0x80:
                v -= 0x100
            return str(v)
        if v < 0:
            return f"-0x{-v:X}" if v < -9 else str(v)
        return f"0x{v:X}" if v > 9 else str(v)

    def pointer(self, sym, pointee):
        """a symbolic word as C"""
        cast = f"({pointee} *)"
        if sym in self.var_at_name:
            self.stats["pointers"] += 1
            return f"{cast}&{self.var_at_name[sym].cname}"
        if sym in self.nm:
            a = self.nm[sym][0]
            for v in self.vars:
                if v.addr is not None and v.addr <= a < v.addr + (v.end - v.off):
                    o = a - v.addr
                    self.stats["pointers"] += 1
                    return f"{cast}((u8 *)&{v.name} + 0x{o:X})" if o else f"{cast}&{v.name}"
        self.extern[sym] = self.nm.get(sym, (None, sym.startswith("func_")))[1]
        self.stats["pointers"] += 1
        return f"{cast}{sym}"

    def init(self, blob, base, ctype, depth=0):
        if isinstance(ctype, Scalar):
            return self.value(blob, base, ctype)
        if isinstance(ctype, Array):
            e = ctype.elem
            if isinstance(e, Scalar) and e.kind == "char":
                return c_string(bytes(blob.val[base:base + ctype.n]))
            items = [self.init(blob, base + i * e.size, e, depth + 1) for i in range(ctype.n)]
            while items and items[-1] in ("0", "{ 0 }") and not isinstance(e, Struct):
                items.pop()
            if not items:
                return "{ 0 }"
            return join(items, depth, isinstance(e, Struct))
        if ctype.union:
            return "{ " + self.init(blob, base, ctype.members[0][2], depth + 1) + " }"
        items = [self.init(blob, base + o, t, depth + 1) for o, _, t in ctype.members]
        nested = any(isinstance(t, Struct) or (isinstance(t, Array) and isinstance(t.elem, (Struct, Array)))
                     for _, _, t in ctype.members)
        return join(items, depth, nested)


def c_float(bits, size):
    if size == 4:
        f = struct.unpack(">f", bits.to_bytes(4, "big"))[0]
        sfx = "f"
        exp_mask, frac_bits = 0xFF, 23
        e = (bits >> 23) & 0xFF
    else:
        f = struct.unpack(">d", bits.to_bytes(8, "big"))[0]
        sfx = ""
        exp_mask, frac_bits = 0x7FF, 52
        e = (bits >> 52) & 0x7FF
    sign = bits >> (8 * size - 1)
    frac = bits & ((1 << frac_bits) - 1)
    if e == exp_mask:
        if frac == 0:
            return ("-" if sign else "") + ("__builtin_inff()" if size == 4 else "__builtin_inf()")
        quiet = frac >> (frac_bits - 1)
        payload = frac & ((1 << (frac_bits - 1)) - 1)
        fn = ("__builtin_nan" if quiet else "__builtin_nans") + ("f" if size == 4 else "")
        return ("-" if sign else "") + f'{fn}("0x{payload:X}")'
    if f == 0:
        return ("-0.0" if sign else "0.0") + sfx
    r = repr(f)
    if float(r) == f and struct.pack(">f" if size == 4 else ">d", float(r)) == bits.to_bytes(size, "big"):
        if "e" not in r and "." not in r and "inf" not in r:
            r += ".0"
        return r + sfx
    return f.hex() + sfx


def c_string(bs):
    """a char array's bytes as a C string literal (octal escapes)"""
    out = '"'
    for b in bs:
        c = chr(b)
        if b in (0x22, 0x5C):
            out += "\\" + c
        elif 0x20 <= b < 0x7F and not (c == "?" and out.endswith("?")):
            out += c
        else:
            out += f"\\{b:03o}"
    return out + '"'


def join(items, depth, aggregates):
    ind, ind1 = "    " * depth, "    " * (depth + 1)
    one = "{ " + ", ".join(items) + " }"
    if len(one) + len(ind1) <= 100 and "\n" not in one:
        return one
    lines, cur = [], ""
    for it in items:
        alone = aggregates or "\n" in it
        if cur and (alone or len(ind1) + len(cur) + len(it) + 2 > 100):
            lines.append(cur.rstrip())
            cur = ""
        if alone:
            lines.append(it + ",")
        else:
            cur += it + ", "
    if cur:
        lines.append(cur.rstrip())
    return "{\n" + "".join(f"{ind1}{l}\n" for l in lines) + ind + "}"


# ---- the N64's names ------------------------------------------------------------

def n64_names():
    """{name: (address, is a function)} of the version's N64 link"""
    import subprocess
    out = {}
    for m in ("hd_code", "hd_front_end"):
        elf = os.path.join(BLAST, "build", f"{m}.{VERSION}.elf")
        res = subprocess.run([N64_NM, elf], capture_output=True, text=True, check=True).stdout
        for line in res.splitlines():
            p = line.split()
            if len(p) != 3 or p[2].startswith((".L", "$")):
                continue
            island = p[2].startswith(("D_", "_binary_")) or p[2].endswith("_bin")
            out.setdefault(p[2], (int(p[0], 16), p[1] in "tT" and not island))
    return out


# names port_game.h makes macros of (the game's reads of the scheduler's counts)
MACROS = ("sinf", "cosf", "bcopy", "bzero", "sprintf", "memcpy", "strlen", "strchr")


def out_name(src):
    """a data file's C file: its path from asm/, `/` as `_` (as CMake has it)"""
    return os.path.relpath(src, os.path.join(BLAST, "asm")).replace("/", "_")[:-2] + ".c"


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        sys.exit(__doc__)
    outdir = args.pop(0)
    os.makedirs(outdir, exist_ok=True)
    nm = n64_names()
    g = Gen(nm)
    files = []          # (out file, rel, [blobs], source)
    k = 0
    while k < len(args):
        a = args[k]
        if a == "--global-asm":
            src = args[k + 1]
            k += 2
            rel = os.path.relpath(src, os.path.join(BLAST, "asm"))
            blobs = parse_global_asm(src)
            for b in blobs:
                resolve(b, None)
            out = "global_asm_" + re.sub(r"^nonmatchings/", "", rel).replace("/", "_")[:-2] + ".c"
            files.append((out, None, blobs, src))
            continue
        k += 1
        if "=" in a:
            label, path = a.split("=", 1)
            b = parse_bin(path, label)
            n64_labels(b, nm)
            resolve(b, None, blob_layout=BLOBS.get(label, []))
            files.append((f"{label}.c", None, [b], path))
            continue
        rel = os.path.relpath(a, BLAST)
        label = None
        mm = re.search(r"/([a-z_]+)/([0-9A-F]+)\.bin\.s$", a)
        if mm:
            label = f"{mm.group(1)}_{mm.group(2)}_bin"
        b = parse_s(a, rel, label)
        n64_labels(b, nm)
        resolve(b, rel, island=ISLANDS.get(os.path.basename(a)) if a.endswith(".bin.s") else None)
        files.append((out_name(a), rel, [b], a))

    # the variables, their types and addresses
    per_file = []
    for out, rel, blobs, src in files:
        fvars = []
        for b in blobs:
            vs = g.partition(b, rel)
            for v in vs:
                v.ctype, v.wrapped = g.var_type(v, rel)
                v.cname = v.name
                v.addr = nm[v.name][0] if v.name in nm else None
            fvars += vs
        for v in fvars:
            if v.wrapped:
                v.cname = f"{v.name}.{v.name}"
        per_file.append((out, rel, src, fvars, fvars))
        g.vars += fvars
    g.var_at_name = {}
    for out, rel, src, top, allv in per_file:
        for v in allv:
            g.var_at_name[v.name] = v
        for v in top:
            g.var_at_name.setdefault(v.name, v)

    # the C files
    defined = set()
    dropped = []
    bodies = {}
    for out, rel, src, top, allv in per_file:
        lines = [f"/* Generated by port/tools/asm2c.py from {os.path.relpath(src, ROOT)}: not to be committed. */",
                 '#include "asm_data.h"', ""]
        aliases, aliases32 = [], []
        for v in top:
            if v.blob.bss:
                lines.append(f"{v.ctype.decl(v.name)};")
            elif all(b == 0 for b in v.blob.val[v.off:v.end]):
                # (zeros, but data: in .data, as the N64 has them, not first
                # in .rdram as .bss)
                lines.append(f"ASM_DATA_ZEROS({v.name}) {v.ctype.decl(v.name)};")
            else:
                lines.append(f"{v.ctype.decl(v.name)} = {g.init(v.blob, v.off, v.ctype)};")
            defined.add(v.name)
            nat = v.ctype.native()
            for o, n in v.inner:
                alias = f".globl {n}\\n.set {n}, {v.name} + 0x{o:X}\\n"
                if nat is not None and o >= nat:
                    # (past a native pointer, where the LP64 build's offset
                    # isn't the N64's: no name there, the C reaches it as
                    # an expression; where pointers are 4 bytes it is
                    # where the N64 has it, for the check build's
                    # translations)
                    dropped.append(f"{n} ({v.name} + 0x{o:X})")
                    aliases32.append(alias)
                    continue
                aliases.append(alias)
        if aliases or aliases32:
            lines += ["", "/* names inside the variables (the movable build's arena link has them at their",
                      "   N64 addresses, where they are) */", "#ifndef PORT_MOVABLE"]
            if aliases:
                lines += ["__asm__("] + [f'    "{a}"' for a in aliases] + [");"]
            if aliases32:
                lines += ["#ifndef PORT_LP64", "__asm__("] + [f'    "{a}"' for a in aliases32] + [");", "#endif"]
            lines += ["#endif"]
        bodies[out] = (lines, top)
    # asm_data.h
    h = ["/* Generated by port/tools/asm2c.py: the types and declarations of the", "   handwritten objects' data. */",
         "#ifndef ASM_DATA_H", "#define ASM_DATA_H", "", '#include "ultratypes.h"', ""]
    for m in MACROS:
        h.append(f"#undef {m}")
    h += ["", "/* data that is zeros: in .data all the same where sections place it */", "#ifdef PORT_MOVABLE",
          "#define ASM_DATA_ZEROS(name)", "#else",
          "#define ASM_DATA_ZEROS(name) __attribute__((section(\".data.\" #name)))", "#endif", ""]
    for f in sorted(g.types.forward - set(g.types.made)):
        h.append(f"typedef struct {f} {f};")
    h.append("")
    seen = set()
    for st in g.types.order:
        if st.name in seen:
            continue
        seen.add(st.name)
        h.append(st.definition())
        h.append("")
    for out, (lines, top) in bodies.items():
        for v in top:
            h.append(f"extern {v.ctype.decl(v.name)};")
    h.append("")
    for sym, fn in sorted(g.extern.items()):
        if sym in defined:
            continue
        h.append(f"void {sym}();" if fn else f"extern u8 {sym}[];")
    h += ["", "#endif"]
    with open(os.path.join(outdir, "asm_data.h"), "w") as f:
        f.write("\n".join(h) + "\n")
    for out, (lines, top) in bodies.items():
        p = os.path.join(outdir, out)
        text = "\n".join(lines) + "\n"
        with open(p, "w") as f:
            f.write(text)
    g.log += [f"{d}: inside a variable past a native pointer: no alias in the LP64 build" for d in dropped]
    with open(os.path.join(outdir, "asm_data_report.txt"), "w") as f:
        f.write("\n".join(g.log) + "\n")
    nv = sum(len(allv) for _, _, _, _, allv in per_file)
    ninner = sum(len(v.inner) for v in g.vars)
    print(f"asm2c.py: {len(per_file)} files, {nv} variables ({g.stats['inventory']} typed by the inventory, "
          f"{g.stats['directives']} by their directives; {g.stats['bss']} of them .bss), {ninner} names inside them, "
          f"{g.stats['pointers']} pointers")


if __name__ == "__main__":
    main()
