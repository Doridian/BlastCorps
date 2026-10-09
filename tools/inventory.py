#!/usr/bin/env python3
"""Write the type and pointer inventory: blastcorps/include/game/inventory.json.

    tools/inventory.py [--version us.v11] [--out FILE] [--dumps rdram_*.bin] [--check]

What the 64-bit port needs to know about the game's memory, in one file
(the format is described in docs/TYPES.md):

  types     every structure in include/game/ (and the libultra structures
            they contain), with each field's offset, name, type and count.
            The layout comes from clang (-m32 -malign-double, which the
            headers' SIZE_CHECKs hold to the N64's); the kinds of address
            come from the header markers (ROMPTR, AssetOffset, RomAddr,
            SegAddr), pointer types and Gfx.
  symbols   the variables whose declaration isn't their type: the ones the
            handwritten code owns (asm/bss, asm/data: only .space and
            .word there), which the headers type, and the C placeholders
            (u8 blobs and T x[1] arrays tools/bss_c.py wrote for memory no C
            names).
  loads     every call that brings bytes in from the ROM (PI DMA, the gzip
            inflate, Rare's LZSS), with what it loads and where to.
  punning   memory read or written at two widths or as two types.
  packed    the handwritten code's stores and loads that carry several
            narrower fields in one register.
  asm_uses  per type, the handwritten functions that touch it and at which
            offsets.
  pointers  every field or variable that holds an N64 address, ROM offset,
            segment address or asset offset, with the evidence.

The data flow analysis is tools/fieldscan.py's.  --check compares the
file with what would be written and fails if it is out of date.
"""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
BLAST = os.path.join(ROOT, "blastcorps")
GAME_INC = os.path.join(BLAST, "include", "game")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import fieldscan as F  # noqa: E402

SCALARS = {
    "u8": "u8", "s8": "s8", "u16": "u16", "s16": "s16", "u32": "u32", "s32": "s32",
    "u64": "u64", "s64": "s64", "f32": "f32", "f64": "f64",
    "unsigned char": "u8", "signed char": "s8", "char": "s8", "unsigned short": "u16", "short": "s16",
    "unsigned int": "u32", "int": "s32", "unsigned long": "u32", "long": "s32",
    "unsigned long long": "u64", "long long": "s64", "float": "f32", "double": "f64",
    "OSPri": "s32", "OSId": "s32", "OSTime": "u64", "OSIntMask": "u32", "OSYieldResult": "u32",
    "ALMicroTime": "s32", "ALFxId": "s32",
}
# typedefs that mean an address kind
MARKERS = {"AssetOffset": "offset", "RomAddr": "romoff", "SegAddr": "segptr"}
# what an AssetOffset field is relative to, per structure (default: itself)
OFFSET_BASE = {
    "LevelHeader": "the level file's start (LevelHeader); displayLists and unkA0 point into the _dl file after it",
}


# the version the headers are read for (main sets it): eu's layouts differ
VERSION_DEFINE = ["-DVERSION_US_V11"]


def clang_cmd():
    rd = subprocess.run(["clang", "-print-resource-dir"], capture_output=True, text=True).stdout.strip()
    inc = [os.path.join(ROOT, "port", "include"), BLAST, os.path.join(BLAST, "include"),
           os.path.join(BLAST, "include", "2.0I"), os.path.join(BLAST, "include", "2.0I", "PR")]
    return (["clang", "-m32", "-malign-double", "-std=gnu89", "-fsyntax-only", "-nostdinc", "-isystem",
             os.path.join(rd, "include")] + ["-I" + i for i in inc] +
            ["-D_LANGUAGE_C", "-D_FINALROM", "-DTARGET_PC"] + VERSION_DEFINE + ["-D_MIPS_SZLONG=32",
             "-D_MIPS_SZINT=32", "-Wno-everything"])


def headers():
    return sorted(f for f in os.listdir(GAME_INC) if f.endswith(".h"))


def run_clang(tmpdir, extra=""):
    tu = os.path.join(tmpdir, "inventory_tu.c")
    with open(tu, "w") as f:
        f.write('#include "common.h"\n')
        for h in headers():
            f.write('#include "game/%s"\n' % h)
        f.write(extra)
    ast = subprocess.run(clang_cmd() + ["-Xclang", "-ast-dump=json", tu], capture_output=True, text=True)
    if ast.returncode:
        sys.exit("clang failed:\n" + ast.stderr)
    lay = subprocess.run(clang_cmd() + ["-Xclang", "-fdump-record-layouts-simple", tu],
                         capture_output=True, text=True)
    return json.loads(ast.stdout), lay.stdout


def parse_layouts(text):
    """{type name: (size bytes, align bytes, [field offsets in bytes])}"""
    out = {}
    for block in text.split("*** Dumping AST Record Layout")[1:]:
        m = re.search(r"Type: (.*)", block)
        s = re.search(r"Size:(\d+)", block)
        a = re.search(r"Alignment:(\d+)", block)
        fo = re.search(r"FieldOffsets: \[([^\]]*)\]", block)
        if not (m and s and a and fo):
            continue
        name = m.group(1).strip()
        offs = [int(x) // 8 for x in fo.group(1).replace(" ", "").split(",") if x]
        out[name] = (int(s.group(1)) // 8, int(a.group(1)) // 8, offs)
    return out


class Types:
    def __init__(self, tmpdir):
        ast, _ = run_clang(tmpdir)
        self.records = {}       # record id -> decl
        self.typedefs = {}      # typedef name -> qualType
        self.tag_of = {}        # typedef name -> record key ("struct X" / "union X" / anon id)
        self.file_of = {}
        self.vars = {}          # extern name -> (qualType, header)
        cur = None
        anon = {}
        for d in ast["inner"]:
            loc = d.get("loc", {})
            if "file" in loc:
                cur = loc["file"]
            if d.get("kind") == "RecordDecl" and d.get("completeDefinition"):
                self.records[d["id"]] = d
                self.file_of[d["id"]] = cur
            if d.get("kind") == "VarDecl" and cur and "/include/game/" in cur:
                self.vars[d["name"]] = (d["type"]["qualType"], os.path.basename(cur))
            if d.get("kind") == "TypedefDecl":
                qt = d["type"]["qualType"]
                self.typedefs[d["name"]] = d["type"]
                self.file_of[d["name"]] = cur
                for inner in d.get("inner", []):
                    # typedef struct {...} Name: the elaborated type's decl
                    rid = find_decl_id(inner)
                    if rid:
                        anon[d["name"]] = rid
        self.anon = anon
        # force a layout for every typedef'd record
        extra = "".join("char __inv_sz_%d[sizeof(%s)];\n" % (i, n)
                        for i, n in enumerate(sorted(self.typedefs))
                        if self._is_record_typedef(n))
        _, lay = run_clang(tmpdir, extra)
        self.layouts = parse_layouts(lay)

    def _is_record_typedef(self, name):
        qt = self.typedefs[name]["qualType"]
        return qt.startswith(("struct ", "union ")) and "*" not in qt and "(" not in qt

    def record_for(self, name):
        """The RecordDecl a typedef name stands for."""
        rid = self.anon.get(name)
        if rid and rid in self.records:
            return self.records[rid]
        qt = self.typedefs.get(name, {}).get("qualType", "")
        m = re.match(r"(struct|union) (\w+)$", qt)
        if m:
            for r in self.records.values():
                if r.get("name") == m.group(2) and r.get("tagUsed") == m.group(1):
                    return r
        return None

    def layout_for(self, name):
        rec = self.record_for(name)
        if rec is None:
            return None
        for key in (name, "%s %s" % (rec.get("tagUsed", "struct"), rec.get("name", ""))):
            if key in self.layouts:
                return self.layouts[key]
        return None


def find_decl_id(node):
    if isinstance(node, dict):
        if node.get("kind") in ("RecordType", "ElaboratedType") and "decl" in node:
            return node["decl"].get("id")
        if "ownedTagDecl" in node:
            return node["ownedTagDecl"].get("id")
        for v in node.values():
            r = find_decl_id(v)
            if r:
                return r
    elif isinstance(node, list):
        for v in node:
            r = find_decl_id(v)
            if r:
                return r
    return None


def classify_type(qt, typedefs):
    """(spec type, pointee or None, count) for a field's qualType."""
    count = 1
    dims = re.findall(r"\[(\d+)\]", qt)
    for d in dims:
        count *= int(d)
    base = re.sub(r"\[\d*\]", "", qt).strip()
    base = re.sub(r"\b(const|volatile)\b", "", base).strip()
    if "(" in base:
        return "ptr", "code", count
    if base.endswith("*"):
        to = base.rstrip("*").strip()
        to = re.sub(r"^(struct|union|enum) ", "", to).strip()
        if base.count("*") > 1:
            to = to + " *" * (base.count("*") - 1)
        return "ptr", to or "void", count
    name = re.sub(r"^(struct|union|enum) ", "", base).strip()
    if base.startswith("enum "):
        return "s32", None, count
    if name in MARKERS:
        return MARKERS[name], None, count
    if name == "Gfx":
        return "gfx", None, count
    if name in SCALARS:
        return SCALARS[name], None, count
    td = typedefs.get(name)
    if td is not None:
        inner = td.get("qualType", "")
        if inner.startswith(("struct ", "union ")) and "*" not in inner:
            return name, None, count
        t, to, n = classify_type(inner, typedefs)
        return t, to, count * n
    return name, None, count


def rom_marked_fields():
    """(struct, field) pairs declared with ROMPTR() in the headers."""
    out = set()
    for h in headers():
        text = open(os.path.join(GAME_INC, h)).read()
        for m in re.finditer(r"typedef\s+(?:struct|union)\s*(\w*)\s*\{(.*?)\}\s*(\w+)\s*;", text, re.S):
            name = m.group(3)
            for fm in re.finditer(r"ROMPTR\([^)]*\)\s*(\w+)", m.group(2)):
                out.add((name, fm.group(1)))
    return out


def build_types(T):
    romf = rom_marked_fields()
    want = set()
    for name, td in T.typedefs.items():
        if T.file_of.get(name, "") and "/include/game/" in T.file_of[name] and T._is_record_typedef(name):
            want.add(name)
    out = {}
    todo = sorted(want)
    seen = set()
    while todo:
        name = todo.pop()
        if name in seen:
            continue
        seen.add(name)
        rec = T.record_for(name)
        lay = T.layout_for(name)
        if rec is None or lay is None:
            continue
        size, align, offs = lay
        fields = []
        fdecls = [f for f in rec.get("inner", []) if f.get("kind") == "FieldDecl"]
        for f, off in zip(fdecls, offs):
            qt = f["type"]["qualType"]
            t, to, count = classify_type(qt, T.typedefs)
            e = {"off": off, "name": f.get("name", ""), "type": t, "count": count}
            if t == "ptr":
                e["to"] = to
                if (name, f.get("name")) in romf:
                    e["rom"] = True
            if t == "offset":
                e["base"] = OFFSET_BASE.get(name, name)
            if t not in F_SCALAR_NAMES and t not in ("ptr", "offset", "romoff", "segptr", "gfx", "bytes"):
                todo.append(t)
            if t == "s8" and re.search(r"\bchar\b", qt) and count > 1:
                e["type"] = "bytes"
                e["note"] = "text"
            if re.match(r"(u8|s8)$", t) and count > 1 and f.get("name", "").startswith("pad"):
                e["type"] = "bytes"
                e["note"] = "padding"
            fields.append(e)
        kind = rec.get("tagUsed", "struct")
        entry = {"size": size, "align": align, "fields": fields}
        if kind == "union":
            entry["union"] = True
        hdr = T.file_of.get(name, "")
        entry["header"] = os.path.relpath(hdr, BLAST) if hdr else ""
        if name in TYPE_NOTES:
            entry["note"] = TYPE_NOTES[name]
        out[name] = entry
    return out


TYPE_NOTES = {
    "Mtx": "fixed point: each s32 word of m holds two s16 halves, the integer parts in m[0..1], the fractions "
           "in m[2..3]; the handwritten matrix builders write it by words (see packed)",
    "Vtx": "Fast3D vertex; the handwritten vertex builders and ROM data write the s16 pairs as words",
    "Gfx": "a display-list command: two words, the second an address (segmented, physical or K0) for some opcodes",
    "LevelHeader": "ROM data (the level file); every offset is added to the file's address where it is used",
    "VehicleModel": "ROM data (a vehicle's model file)",
    "Model": "ROM data (a model table entry), written into after loading",
}

F_SCALAR_NAMES = {"u8", "s8", "u16", "s16", "u32", "s32", "u64", "s64", "f32", "f64"}


# ---------------------------------------------------------------- symbols

DECL = re.compile(r"^extern\s+([^;()]*?)\s*\b(D_[0-9A-Fa-f]{8}|\w+)\b\s*((?:\[[^\]]*\])*)\s*;\s*(?:/\*\s*(.*?)\s*\*/)?")


def header_externs(T):
    """name -> (type text, dims text, comment, header), from clang's view
    of the headers' extern declarations (so macros in sizes are resolved)."""
    comments = {}
    for h in headers():
        for line in open(os.path.join(GAME_INC, h)):
            m = DECL.match(line.strip())
            if m:
                comments[m.group(2)] = m.group(4) or ""
    out = {}
    for name, (qt, h) in T.vars.items():
        if name.startswith("func_"):
            continue
        dims = "".join(re.findall(r"\[\d*\]", qt))
        base = re.sub(r"\[\d*\]", "", qt).strip()
        out[name] = (base, dims, comments.get(name, ""), h)
    return out


def asm_owned(prog):
    """Data symbols the handwritten objects define (their .bss.s/.data.s)."""
    out = {}
    # asm/data/<module>/ holds the handwritten objects' own .data/.rodata/
    # .bss (asm/bss/ only has the references tools/bss_c.py checks the C
    # against)
    for mod in ("hd_code", "hd_front_end", "init"):
        p = os.path.join(BLAST, "asm", "data", mod)
        if not os.path.isdir(p):
            continue
        for fn in sorted(os.listdir(p)):
            for line in open(os.path.join(p, fn)):
                m = re.match(r"\s*(?:dlabel|glabel)\s+(\w+)", line)
                if m:
                    out[m.group(1)] = "asm/data/%s/%s" % (mod, fn)
    return out


def c_placeholders():
    """C definitions tools/bss_c.py wrote for memory no C names: u8 blobs
    and T x[1] arrays."""
    out = {}
    src = os.path.join(BLAST, "src")
    for d, _, files in os.walk(src):
        for fn in files:
            if not fn.endswith(".c"):
                continue
            text = open(os.path.join(d, fn)).read()
            for m in re.finditer(r"^(\w[\w ]*?\*?)\s*\b(D_[0-9A-F]{8})\[(0x[0-9A-Fa-f]+|\d+)\];", text, re.M):
                t, name, n = m.group(1).strip(), m.group(2), int(m.group(3), 0)
                if t == "u8" or n == 1:
                    out[name] = (t, n, os.path.relpath(os.path.join(d, fn), BLAST))
    return out


def data_to_end(owned, name):
    """the bytes from a data file's label to the file's end"""
    f = owned.get(name, "")
    if not f.endswith((".data.s", ".rodata.s")):
        return 0
    n, on = 0, False
    for lab, (b, refs) in asm_data_contents({name: f}).items():
        on = on or lab == name
        if on:
            n += len(b)
    return n


def build_symbols(T, prog, res, types):
    ext = header_externs(T)
    owned = asm_owned(prog)
    out = {}
    for name, (t, dims, comment, h) in sorted(ext.items()):
        if name not in owned:
            continue
        count = 1
        for dd in re.findall(r"\[(\w*)\]", dims):
            count *= int(dd, 0) if dd else 0
        ty, to, _ = classify_type(t.strip() + dims, T.typedefs)
        if dims and count == 0 and ty in types:
            addr = prog.named.get(name)
            count = prog.var_size(addr) // types[ty]["size"] if addr else 0
            # an array of unknown length in a data file: to the file's end,
            # over the labels splat made inside it (D_8020C070's entries
            # are reached through several, yoshi.h)
            ext_n = data_to_end(owned, name) // types[ty]["size"]
            if ext_n > count:
                count = ext_n
        e = {"type": ty, "count": count if dims else 1, "declared": h, "storage": owned[name]}
        if to:
            e["to"] = to
        if comment:
            e["note"] = comment
        out[name] = e
    for name, (t, n, where) in sorted(c_placeholders().items()):
        if name in out:
            continue
        addr = prog.named.get(name)
        e = {"type": "bytes" if t == "u8" else classify_type(t, T.typedefs)[0],
             "count": n, "declared": where, "placeholder": True}
        if t != "u8":
            e["note"] = "declared %s[1]: only its first element is named; see the uses for the real count" % t
        else:
            e["note"] = "memory the C doesn't name (tools/bss_c.py filler)"
        out[name] = e
    # fixed memory the game uses outside the modules' sections
    out.update(FIXED)
    return out


# The handwritten objects' data the analysis doesn't reach field by field
# (read through an index it can't follow), as the port's access-width
# profiler (port/host/access.c) found them read: {symbol: (record fields
# [(offset, type)], record size, evidence)}
ASM_DATA_LAYOUTS = {
    "D_80305B90": ([(0, "s16"), (2, "s16"), (4, "s16")], 6,
                   "func_802A56C4: lh 0, 2, 4 of record i * j (hd_code 60D50)"),
    "D_80305E38": ([(k, "s16") for k in range(0, 12, 2)], 24,
                   "func_802C0E8C reads s16 0..0xA (hd_code 77E20)"),
    # (fieldscan takes it for a list of words, a pointer walking it; only
    # the RSP reads it, as halves, and native-endian words put its x and
    # y, z and the flag, s and t in each other's places: a flat dome)
    "D_802FFF38": ([(0, "s16"), (2, "s16"), (4, "s16"), (6, "u16"), (8, "s16"), (10, "s16"),
                    (12, "u8"), (13, "u8"), (14, "u8"), (15, "u8")], 16,
                   "Vtx[179], the carrier's explosion's dome: 39050.c's segment 6 for D_80300A68's G_VTXs"),
    # (read by the C and port/engine, which the analysis doesn't follow)
    "D_803059F0": ([(0, "u32"), (4, "s32"), (8, "u32")], 12,
                   "{level (0xFFFF: any), building kind, a bit per vehicle type}, to a zero mask: "
                   "port/engine 56040.c's BuildingRule, 77E20.c's Immune"),
    "D_80306E70": ([(0, "u8")] + [(k, "u32") for k in range(4, 0x50, 4)], 0x50,
                   "OSViMode[]: u8 type, then the common and the two fields' registers (2C560.c's "
                   "osViSetMode(&D_80306E70[mode]))"),
}

# The rest of the handwritten objects' data that no code reads at a width
# the analysis sees, typed by its readers in the C and port/engine:
# {symbol: (type, evidence)}; the count is the symbol's size over the type's
ASM_DATA_TYPES = {
    "D_80301098": ("u16", "text in the 0x0FFE-terminated u16 encoding, as BC8E0's (26570.c)"),
    "D_80305C10": ("ptr", "the animations kept in the effects' heap, 0 at the end (port/engine 60F60.c)"),
    "D_80305C34": ("bytes", "a C string (port/engine 60F60.c)"),
    "D_80305C50": ("f64", "a constant of the height search (port/engine 62740.c)"),
    "D_80305CB0": ("bytes", "its parts' collision, read as bytes (port/engine 69BB0.c, 56040's func_8029A800)"),
    "D_80305D40": ("bytes", "a C string (port/engine 72B80.c)"),
    "D_803063F0": ("bytes", "the share of the groups a medal wants, by medal (port/engine 80280.c)"),
    "D_802FF730": ("gfx", "a display list: gsSPEndDisplayList()"),
    "__osHwIntTable": ("ptr", "libultra's interrupt handlers (the port's libultra has its own)"),
    "D_803C2B80": ("u64", "the effects' task's output size (port/engine 5FD50.c: output_buff_size)"),
    "D_803C4F70": ("Mtx", "the effects' sprites' matrices, one frame's (port/engine 60F60.c), as D_803C5370"),
    "D_803C8770": ("Vtx", "the effects' sprites' vertices, one frame's (port/engine 60F60.c), as D_803C9770"),
    "D_803CA770": ("bytes", "the effects' sprites' texture cells, one frame's (port/engine 60F60.c)"),
    "D_803DA770": ("bytes", "the other frame's"),
    "D_803EA770": ("bytes", "the effects' sprites' cells, 0x100 bytes per slot (port/engine 60F60.c)"),
    "D_803F2ED0": ("Mtx", "the falling groups' matrices, one frame's (port/engine 77E20.c), as D_803F24D0"),
    # YoshiEntry.text of hd_front_end 25070's tables
    "D_8020F49C": ("bytes", "a C string, empty (YoshiEntry.text)"),
    "D_8020F4A0": ("bytes", "a C string, empty (YoshiEntry.text)"),
    "D_8020F4A4": ("bytes", "a C string, empty (YoshiEntry.text)"),
    "D_8020F4A8": ("bytes", "a C string, empty (YoshiEntry.text)"),
    "D_8020F4AC": ("bytes", "a C string, empty (YoshiEntry.text)"),
    "D_8020F788": ("bytes", "a C string, \" \" (YoshiEntry.text)"),
    "D_8020FD90": ("bytes", "a C string, \" \" (YoshiEntry.text)"),
    "D_8020FDE4": ("bytes", "a C string, \" \" (YoshiEntry.text)"),
}

# data files that are all text in the 0x0FFE-terminated u16 encoding, every
# label a u16 array: {file: evidence}
ASM_DATA_TEXT16 = {
    "asm/data/hd_code/BC8E0.data.s": "the menus' Japanese text, which only jp shows (26570.c's D_803010A0 on; "
                                     "yoshi.h's YoshiEntry.unk10)",
}

# data kept in the N64's byte order, whatever its fields: the code that
# reads it at a width is listed in tools/recomp/native_sites.txt (`be`)
ASM_DATA_BE = {
    # 12-byte records {u16 threshold, value; u32 lo, hi}, walked (lhu 0)
    # until a random number is below the threshold: at 100 the walk runs off
    # the end into D_803063E0, which the same function reads as byte pairs,
    # and takes its bytes for a record.  Kept big-endian so that both reads
    # see what the N64 does.
    "D_80306344": "func_802BF978 (lhu 0, 2; lw 4, 8): records running into D_803063E0",
    "D_80306350": "func_802BF978, as D_80306344",
    "D_803063D4": "func_802BFDAC, as D_80306344",
    # texels, which the renderer reads as bytes in either build
    "D_802FF738": "the carrier's explosion's dome's texture: D_80300A68's G_SETTIMG",
}


# data the code reaches through a pointer table, where fieldscan can't
# follow: {table: why}; every symbol the table points at is bytes
ASM_DATA_BYTE_TABLES = {
    "D_80306270": "func_802C0574 reads *D_80306270[i] with lbu 0 and lbu 1 + k (hd_code 77E20)",
}


def asm_data_contents(owned):
    """{label: (bytes, [labels its .words point at])} of the asm data files,
    as far as the directives are numbers"""
    out, files = {}, sorted(set(owned.values()))
    size = {".word": 4, ".short": 2, ".half": 2, ".byte": 1}
    for f in files:
        lab = None
        for line in open(os.path.join(BLAST, f)):
            m = re.match(r"\s*(?:dlabel|glabel)\s+(\w+)", line)
            if m:
                lab = m.group(1)
                out[lab] = (bytearray(), [])
                continue
            m = re.match(r"\s*(\.\w+)\s+(.*)", line)
            if not lab or not m or m.group(1) not in size:
                continue
            for v in m.group(2).split(","):
                v = v.strip()
                try:
                    n = int(v, 0)
                except ValueError:
                    out[lab][1].append(v)
                    n = 0
                out[lab][0].extend((n & (1 << 8 * size[m.group(1)]) - 1).to_bytes(size[m.group(1)], "big"))
    return out


def text_kind(b):
    """what makes these bytes text, or None: a C string (printable ASCII,
    a NUL, zeros), or the game's 0xFF-terminated text (bytes below 0x80,
    0xFF, then zeros: read with lb until negative, func_802BEEF0)"""
    n = b.find(0xFF)
    if n >= 0 and len(b) >= n + 2 and all(c < 0x80 for c in b[:n]) and not any(b[n + 1:]):
        return "a 0xFF-terminated text (func_802BEEF0 reads it with lb until negative)" if n else \
            "an empty 0xFF-terminated text"
    n = b.find(0)
    if n >= 2 and all(0x20 <= c < 0x7F or c == 0x0A for c in b[:n]) and not any(b[n:]):
        return "a C string"
    return None


def build_asm_data_types(prog, an, res, elem, symbols, types):
    """The handwritten objects' data files (asm/data: splat wrote their
    contents as .word, .half, .byte by guesswork) typed by how the code
    reaches them, for every symbol the headers don't type: each field at
    the width the code accesses it with (tools/fieldscan.py's view, copy
    loops left out), bytes where nothing reads more than one.  Where a field
    is read at two widths, the one used most wins and the type says so.
    The native-endian port converts these files by it (port/tools/
    asm2x86.py --native)."""
    owned = asm_owned(prog)
    widths = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    floats = defaultdict(set)
    # (accesses in loops that step 1, 2 or 4 bytes count too: here they are
    # walks through the records of a table, which is read-only data)
    for (r, fo), kind, w, store, fn, pc, asm in event_fields(prog, an, res, elem, strided=True):
        if not isinstance(r, str) or r not in owned or r in symbols:
            continue
        widths[r][fo][w] += 1
        if kind != "int":
            floats[r].add(fo)
    out_types, out_syms = {}, {}
    for name, (flds, esize, why) in ASM_DATA_LAYOUTS.items():
        addr = prog.named.get(name)
        if addr is None or name not in owned:
            continue
        size = prog.var_size(addr)
        out_types["asm_" + name] = {"size": esize, "align": 4, "header": "(tools/inventory.py ASM_DATA_LAYOUTS)",
                                    "fields": [{"off": o, "name": "unk%X" % o, "type": t, "count": 1} for o, t in flds],
                                    "note": why}
        out_syms[name] = {"type": "asm_" + name, "count": (size + esize - 1) // esize, "storage": owned[name],
                          "derived": "the port's access-width profiler"}
    scalar = {"u8": 1, "s8": 1, "u16": 2, "s16": 2, "u32": 4, "s32": 4, "f32": 4, "u64": 8, "s64": 8, "f64": 8,
              "ptr": 4, "gfx": 4, "bytes": 1}
    for name, (t, why) in ASM_DATA_TYPES.items():
        addr = prog.named.get(name)
        if addr is None or name not in owned or name in symbols or name in out_syms:
            continue
        esize = types[t]["size"] if t in types else scalar[t]
        out_syms[name] = {"type": t, "count": max(1, prog.var_size(addr) // esize), "storage": owned[name],
                          "derived": why}
        if t == "ptr":
            out_syms[name]["to"] = "void"
    for name, f in sorted(owned.items()):
        if f in ASM_DATA_TEXT16 and name not in symbols and name not in out_syms \
                and prog.named.get(name) is not None and prog.var_size(prog.named[name]) >= 2:
            out_syms[name] = {"type": "u16", "count": prog.var_size(prog.named[name]) // 2, "storage": f,
                              "derived": ASM_DATA_TEXT16[f]}
    # bytes by what they hold or how a table reaches them, where no code
    # reads them at a width
    contents = asm_data_contents(owned)
    byte_syms = dict(ASM_DATA_BE)
    for tab, why in ASM_DATA_BYTE_TABLES.items():
        for t in contents.get(tab, (b"", []))[1]:
            byte_syms[t] = why
    for name, (b, refs) in contents.items():
        if name not in symbols and name not in widths and not refs and name not in byte_syms:
            k = text_kind(bytes(b))
            if k:
                byte_syms[name] = k
    for name, why in sorted(byte_syms.items()):
        if name in owned and name not in symbols and name not in out_syms and prog.named.get(name) is not None:
            out_syms[name] = {"type": "bytes", "count": prog.var_size(prog.named[name]), "storage": owned[name],
                              "derived": why}
    for name in sorted(widths):
        if name in out_syms:
            continue
        addr = prog.named.get(name)
        if addr is None:
            continue
        size = prog.var_size(addr)
        e = elem.get(name) or 0
        if not size:
            continue
        esize = e if e and e <= size else size     # (a last record may be cut short)
        fields, end, puns = [], 0, []
        for off in sorted(widths[name]):
            if off >= esize:
                continue
            ws = widths[name][off]
            w = max(ws, key=lambda k: (ws[k], k))
            if len(ws) > 1:
                puns.append("+0x%X as %s" % (off, "/".join(str(k) for k in sorted(ws))))
            if off < end or off % min(w, 4) or off + w > esize:
                continue
            t = {1: "u8", 2: "u16", 4: "f32" if off in floats[name] else "u32",
                 8: "f64" if off in floats[name] else "u64"}[w]
            fields.append({"off": off, "name": "unk%X" % off, "type": t, "count": 1})
            end = off + w
        if all(f["type"] == "u8" for f in fields):
            # read only as bytes: bytes, whatever the directives say
            out_syms[name] = {"type": "bytes", "count": size, "storage": owned[name],
                              "derived": "the handwritten code's accesses (tools/fieldscan.py): bytes only"}
            continue
        # no stride found, but fields packed from the start (bytes too): a
        # list the code walks with a pointer (fieldscan sees its first
        # record only), so the record repeats over the symbol
        note = None
        seen = {}
        for off in sorted(widths[name]):
            ws = widths[name][off]
            seen[off] = max(ws, key=lambda k: (ws[k], k))
        extent, k = 0, 0
        while k in seen:
            k += seen[k]
        extent = k
        if not e and esize == size and 0 < extent < size and extent >= max(seen) + 1:
            esize = extent
            fields = [f for f in fields if f["off"] < esize]
            note = "a list walked with a pointer: the first record's fields, repeated"
        tname = "asm_" + name
        ty = {"size": esize, "align": 4, "fields": fields, "header": "(tools/fieldscan.py)"}
        if puns:
            ty["note"] = "read at two widths: " + ", ".join(puns)
        if note:
            ty["note"] = (ty["note"] + "; " if "note" in ty else "") + note
        out_types[tname] = ty
        out_syms[name] = {"type": tname, "count": (size + esize - 1) // esize, "storage": owned[name],
                          "derived": "the handwritten code's accesses (tools/fieldscan.py)"}
    return out_types, out_syms


FIXED = {
    "0x80000400": {"type": "bytes", "count": 2 * 320 * 240 * 2,
                   "note": "the two 320x240 RGBA16 framebuffers (D_80358050); hd_code is staged here before init inflates it"},
    "0x8004B400": {"type": "bytes", "count": 0x8021ED00 - 0x8004B400,
                   "note": "the heap (D_80358070 starts here): the level file, models, textures, allocations"},
    "0x80055400": {"type": "bytes", "count": 0x10000,
                   "note": "ghostdigger.c's (50670.c's) ghost buffers, fixed addresses in the heap (D_8039CA68)"},
    "0x8021ED00": {"type": "bytes", "count": 0x80224D80 - 0x8021ED00,
                   "note": "init's memory, reused as the depth buffer and gzip's inflate scratch once the game runs"},
    "0x803FF600": {"type": "bytes", "count": 0x80400000 - 0x803FF600 - 8,
                   "note": "static data (segment 1, D_8035806C) loaded from ROM to the end of .bss"},
}


# ---------------------------------------------------------------- loads

LOADERS = {
    # name: (via, rom arg, dst arg, size arg)
    "osPiStartDma": ("dma", "a3", "stk10", "stk14"),
    "func_8028B4C4": ("dma+inflate|lzss", "a0", "a1", "a2*"),
    "func_802C4108": ("inflate", "a0", "a1", None),
    "func_802C41C0": ("lzss", None, None, None),
}

# what each ROM segment holds, by its linker symbol's segment name
SEGMENT_TYPES = [
    (r"^texture_table", "struct {u32 offset (from the table); u16 length; u16 type}", 4096, "offsets turned into ROM addresses by the loaders (5BF40)"),
    (r"^model_table", "u32 (offset from the table)", 512, "offsets to model_table_ROM_START (func_802A2A98)"),
    (r"^(chimp|lagp|valley|fact|dip|beetle|bonus\d|level\d+)$", "LevelHeader", 1, "the offsets are added where used (never stored as pointers); the _dl file's G_SETTIMG words are texture indices patched to addresses"),
    (r"_dl$", "gfx", 0, "G_SETTIMG words patched from texture indices (func_802A08E4)"),
    (r"^static_data", "Vp, then gfx", 1, ""),
    (r"^(music|sfx)_ctl", "ALBankFile", 1, "alBnkfNew turns its offsets into pointers in place"),
    (r"^(music|sfx)_tbl", "bytes (ADPCM)", 0, ""),
    (r"^sequences", "ALSeqFile", 1, "alSeqFileNew turns its offsets into pointers in place"),
    (r"^bg_image", "bytes (RGBA16 320x240)", 1, ""),
    (r"^textures", "bytes (texels)", 0, ""),
]


def rom_segment(prog, addr):
    """The ROM segment (by its _ROM_START symbol) an address is in."""
    best = None
    for name, a in prog.named.items():
        if name.endswith("_ROM_START") and a <= addr and (best is None or a > best[1]):
            best = (name[:-len("_ROM_START")], a)
    return best


def build_loads(prog, an, res):
    out = []
    fname = res.funcname
    finfo = {fi.start: fi for fi in an.infos.values()}
    for e in an.events:
        if e[0] != "call":
            continue
        _, f, pc, target, regs, stk = e
        tn = fname.get(target, "")
        if tn not in LOADERS:
            continue
        via, ra, rd, rs = LOADERS[tn]

        def val(key):
            if key is None:
                return None
            if key.startswith("stk"):
                return stk.get(int(key[3:], 16))
            return regs.get(F.mips.GPR_NAMES.index(key.rstrip("*")))
        rom = val(ra)
        dst = val(rd)
        entry = {"site": "%s+0x%X" % (fname.get(f, "?"), pc - f), "via": via if tn != "func_8028B4C4" else "rom load",
                 "func": tn, "file": finfo[f].file.split("/")[-1], "asm": finfo[f].kind == "asm"}
        if rom is not None and rom[0] is None and not rom[3]:
            seg = rom_segment(prog, rom[2])
            entry["rom"] = ("%s+0x%X" % (seg[0], rom[2] - seg[1])) if seg and rom[2] != seg[1] else (seg[0] if seg else "0x%X" % rom[2])
            for pat, ty, count, reloc in SEGMENT_TYPES:
                if seg and re.search(pat, seg[0]):
                    entry["type"] = ty
                    if count:
                        entry["count"] = count
                    if reloc:
                        entry["reloc"] = reloc
                    break
        elif rom is not None:
            entry["rom"] = describe(prog, res, rom)
        if dst is not None:
            entry["dst"] = describe(prog, res, dst)
        out.append(entry)
    out.extend(KNOWN_LOADS)
    return out


KNOWN_LOADS = [
    {"site": "func_8025615C", "via": "rom load (func_8028B4C4, method 1: gzip)", "rom": "the level's gzip member (one per LevelId)",
     "dst": "the heap (D_80358074 = D_80358070)", "type": "LevelHeader", "count": 1,
     "reloc": "none stored: the handwritten loader adds the offsets as it reads them (D_803BE6FC, D_803BDB10 ... keep the sums)"},
    {"site": "func_802A396C", "via": "dma + inflate twice (func_802C4108) + func_802A44E4", "rom": "a vehicle's gzip pair (docs/blast_corps_vehicles.txt)",
     "dst": "the heap", "type": "VehicleModel", "count": 1,
     "reloc": "func_802A1388 stores file + offset into Vehicle (D_80364460[])"},
    {"site": "func_802A2A98", "via": "dma + inflate twice + func_802A44E4", "rom": "model_table entry i to i+1",
     "dst": "the heap", "type": "Model", "count": 1, "reloc": "func_802A44E4 (5CB60), not analysed"},
    {"site": "func_802A08E4 and five more (5BF40)", "via": "dma + func_802A57DC (Rare's texture decoders)",
     "rom": "texture_table entry i to i+1", "dst": "the texture cache (D_803C4B70, D_803C3250 ...)", "type": "bytes (texels)"},
]


def describe(prog, res, v):
    if v is None:
        return None
    o, a, k, s, tag = v
    if o is None:
        if F.RAM_LO <= k < F.RAM_HI:
            return F.fmt_addr(prog, k, s)
        return "0x%X" % k
    slots = F.origin_slots(o)
    if slots:
        return " | ".join(F.fmt_slot(prog, res, sl) for sl in slots[:3]) + ("+0x%X" % k if k else "")
    return str(o[0])


# ---------------------------------------------------------------- punning

def event_fields(prog, an, res, elem, strided=False):
    """Every access that isn't part of a copy loop (stride 1, 2 or 4), as
    (key, kind, width, store, function, pc, asm): key is a fields.tsv
    (region, offset) with the offset folded by the region's element."""
    finfo = {fi.start: fi for fi in an.infos.values()}
    for e in an.events:
        if e[0] != "acc":
            continue
        _, f, pc, op, w, signed, store, loc, addr, val = e
        if loc is None or loc[0] == "S":
            continue
        fi = finfo[f]
        for c in ([loc] if loc[0] == "G" else res.canon(loc)):
            if c[-1] in (1, 2, 4) and not strided:
                continue
            key = F.region_key(prog, c)
            if key is None:
                continue
            r, off, st = key
            el = elem.get(r)
            fo = off % el if el else off
            kind = "f32" if op in ("lwc1", "swc1") else "f64" if op in ("ldc1", "sdc1") else "int"
            yield (r, fo), kind, w, store, fi.name, pc - fi.start, fi.kind == "asm"


def build_punning(prog, an, res, regions, elem):
    """Memory accessed at two widths (at one offset), or as float and integer."""
    seen = defaultdict(lambda: defaultdict(set))
    for key, kind, w, store, fn, off, is_asm in event_fields(prog, an, res, elem):
        tag = kind if kind != "int" else {1: "8", 2: "16", 4: "32", 8: "64"}[w]
        seen[key][tag].add("%s+0x%X" % (fn, off))
    out = []
    for (r, fo), kinds in sorted(seen.items(), key=lambda x: (str(x[0][0]), x[0][1])):
        ints = [k for k in kinds if k in ("8", "16", "32", "64")]
        floats = [k for k in kinds if k in ("f32", "f64")]
        if len(ints) > 1 or (floats and ints) or len(floats) > 1:
            out.append({"where": F.field_name(r, fo, elem.get(r, 0)),
                        "as": sorted(("u" + k if k.isdigit() else k) for k in kinds),
                        "sites": {("u" + k if k.isdigit() else k): sorted(v)[:6] for k, v in sorted(kinds.items())}})
    out.extend(KNOWN_PUNNING)
    return out


KNOWN_PUNNING = [
    {"where": "D_80364A90 (u64 game_mode), D_80364A98", "as": ["u64", "u32 halves"],
     "note": "the C uses ld/sd; the handwritten code reads and writes the words (high word first, at the lower address)"},
    {"where": "D_8021A830", "as": ["u64 (hd.c, 17990.c)", "u8[4] (10850.c's definition)"],
     "note": "10850.c defines 4 bytes; the u64 view covers the next variable too"},
    {"where": "D_8039C4B8", "as": ["u8[0x40] (the Controller Pak block)", "u64 (409D0.c: the 0x1234567887654321 marker)"]},
    {"where": "D_02000000 / D_803156F8", "as": ["FrameBuf (front end)", "FrameGame (gameplay)", "Mtx[]", "Vtx[]"],
     "note": "one per-frame buffer with two layouts (game/frame.h)"},
    {"where": "D_80367750, D_8036AFB0", "as": ["u8[] (1C460.c, 22EE0.c)", "u64[] (405F0.c)"]},
    {"where": "Mtx", "as": ["s32 words (guMtxF2L)", "u16 halves (the handwritten matrix code)"],
     "note": "libultra's fixed-point matrix: integer halves then fraction halves"},
]


# ---------------------------------------------------------------- packed

def build_packed(prog, an, res, elem):
    """The handwritten code's word and doubleword accesses that carry
    narrower fields: (1) where other code reaches the same bytes as
    narrower fields (a Vtx's s16s, a record's bytes), (2) the stores that
    build a libultra Mtx (fixed point: each word two s16 halves, the
    integer parts first), recognised by the 0x10000 (1.0) they put on the
    diagonal."""
    narrow = defaultdict(set)     # (region, offset) -> widths seen
    events = list(event_fields(prog, an, res, elem))
    for key, kind, w, store, fn, off, is_asm in events:
        if kind == "int":
            narrow[key].add(w)
    out = {}
    for (r, fo), kind, w, store, fn, off, is_asm in events:
        if not is_asm or kind != "int" or w < 4:
            continue
        parts = []
        for k in range(w):
            ws = narrow.get((r, fo + k), set())
            for nw in sorted(ws):
                if nw < w and k + nw <= w:
                    parts.append("+%d:%d" % (k, nw))
        if parts:
            site = "%s+0x%X" % (fn, off)
            out.setdefault(site, {"site": site, "op": ("store" if store else "load") + " %d" % w,
                                  "where": F.field_name(r, fo, elem.get(r, 0)),
                                  "fields": "other code reaches these bytes as " + " ".join(sorted(set(parts)))})
    # Mtx builders
    for fi in an.infos.values():
        if fi.kind != "asm":
            continue
        consts = {}
        diag = defaultdict(set)
        stores = defaultdict(list)
        for pc in sorted(fi.insns):
            i = fi.insns[pc]
            if i is None:
                continue
            if i.op == "lui":
                consts[i.rt] = i.uimm << 16
            elif i.kind == "store" and i.op == "sw":
                stores[i.rs].append((pc, i.imm))
                if consts.get(i.rt) == 0x10000 and i.imm in (0, 0x14, 0x28, 0x3C):
                    diag[i.rs].add(i.imm)
            else:
                d = F.defs(i)
                if d is not None:
                    consts.pop(d, None)
        for reg, offs in diag.items():
            if len(offs) >= 3:
                for pc, imm in stores[reg]:
                    if 0 <= imm < 0x40:
                        site = "%s+0x%X" % (fi.name, pc - fi.start)
                        out[site] = {"site": site, "op": "store 4", "where": "a Mtx (base in $%s)" % F.mips.GPR_NAMES[reg],
                                     "fields": "Mtx word +0x%X: two s16 halves (%s parts)" % (
                                         imm, "integer" if imm < 0x20 else "fraction")}
    return sorted(out.values(), key=lambda e: e["site"])


# ---------------------------------------------------------------- asm_uses and pointers

def instance_map(T, types, prog):
    """Where each header-typed variable lives: [(start, size, elem size,
    type)] for globals, {pointer var: type} for pointers."""
    ext = header_externs(T)
    inst = []
    ptrs = {}
    for name, (t, dims, comment, h) in ext.items():
        addr = prog.named.get(name)
        if addr is None:
            continue
        base = re.sub(r"\b(struct|union)\b", "", t).strip()
        if base.endswith("*"):
            pt = base.rstrip("*").strip()
            if pt in types:
                ptrs[name] = pt
            continue
        if base in types:
            esz = types[base]["size"]
            n = 1
            for dd in re.findall(r"\[(\w*)\]", dims):
                n *= int(dd, 0) if dd else 0
            if dims and n == 0:
                n = max(1, prog.var_size(addr) // esz)
            inst.append((addr, esz * (n if dims else 1), esz, base, name))
    inst.sort()
    return inst, ptrs


def field_at(types, t, off):
    """The field of type t that holds byte off: (field, offset within it)."""
    for f in types.get(t, {}).get("fields", []):
        size = elem_size(types, f) * f["count"]
        if f["off"] <= off < f["off"] + max(size, 1):
            return f, off - f["off"]
    return None, None


def elem_size(types, f):
    t = f["type"]
    if t in ("u8", "s8", "bytes"):
        return 1
    if t in ("u16", "s16"):
        return 2
    if t in ("u32", "s32", "f32", "ptr", "offset", "romoff", "segptr"):
        return 4
    if t in ("u64", "s64", "f64", "gfx"):
        return 8
    return types.get(t, {}).get("size", 1)


def locate_type(region, off, inst, ptrs, prog, types=None, depth=0):
    """(type, field offset, instance) of a fields.tsv region + offset: a
    header-typed global, what a header-typed pointer points to, or what a
    pointer field of either points to."""
    if isinstance(region, str):
        addr = prog.named.get(region)
        if addr is None:
            return None
        a = addr + off
        for start, size, esz, t, name in inst:
            if start <= a < start + size:
                return t, (a - start) % esz, name
        return None
    if region[0] != "*" or len(region) < 4 or depth > 4:
        return None
    inner, ioff = region[1], region[2]
    if isinstance(inner, str) and inner in ptrs and ioff == 0:
        return ptrs[inner], off, "*" + inner
    if types is None:
        return None
    hit = locate_type(inner, ioff, inst, ptrs, prog, types, depth + 1)
    if hit is None:
        return None
    t, fo, name = hit
    f, within = field_at(types, t, fo)
    while f is not None and f["type"] in types and f["type"] not in ("ptr",):
        f, within = field_at(types, f["type"], within % types[f["type"]]["size"])
    if f is None or f["type"] != "ptr" or f.get("to") not in types:
        return None
    return f["to"], off, "*%s.%s" % (name, f["name"])


def build_asm_uses_and_pointers(prog, an, res, regions, elem, kinds, T, types):
    inst, ptrs = instance_map(T, types, prog)
    uses = defaultdict(lambda: defaultdict(set))
    pointers = []
    finfo = {fi.start: fi for fi in an.infos.values()}
    # accesses by function, mapped to types
    for e in an.events:
        if e[0] != "acc":
            continue
        _, f, pc, op, w, signed, store, loc, addr, val = e
        fi = finfo[f]
        if fi.kind != "asm" or loc is None or loc[0] == "S":
            continue
        for c in ([loc] if loc[0] == "G" else res.canon(loc)):
            key = F.region_key(prog, c)
            if key is None:
                continue
            r, off, s = key
            hit = locate_type(r, off, inst, ptrs, prog, types)
            if hit:
                uses[hit[0]][fi.name].add(hit[1])
    asm_uses = {t: {fn: sorted(o) for fn, o in sorted(fs.items())} for t, fs in sorted(uses.items())}
    # the pointer list: every field the analysis or the dumps say holds an address
    for region, flist in sorted(regions.items(), key=lambda x: str(x[0])):
        e = elem.get(region, 0)
        for off, a in sorted(flist, key=lambda x: x[0]):
            k = set(a["kind"])
            dyn = a.get("dyn", "")
            kind = None
            for cand in ("ptr", "offset", "rom", "seg", "phys", "static"):
                if cand in k:
                    kind = cand
                    break
            if kind is None and dyn == "ptr":
                kind = "ptr?"
            if kind is None:
                continue
            hit = locate_type(region, off, inst, ptrs, prog, types)
            entry = {"where": F.field_name(region, off, e), "kind": {"rom": "romoff", "seg": "segptr",
                                                                    "static": "ptr (initialized data)"}.get(kind, kind),
                     "evidence": sorted(k - {"static"}) + (["reloc"] if "static" in k else []) +
                     (["dump:" + dyn] if dyn else [])}
            if hit:
                entry["field"] = "%s+0x%X" % (hit[0], hit[1])
            pointers.append(entry)
    return asm_uses, pointers


# ---------------------------------------------------------------- main

def main():
    # set iteration order decides some tie-breaks (and the analysis's cycle
    # cuts): fix the hash seed so every run writes the same thing
    if os.environ.get("PYTHONHASHSEED") != "0":
        os.environ["PYTHONHASHSEED"] = "0"
        os.execv(sys.executable, [sys.executable] + sys.argv)
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--version", default="us.v11")
    ap.add_argument("--dumps", nargs="*", default=[])
    ap.add_argument("--out", help="default: inventory.<version>.json where the version has one, else inventory.json"
                    " (the one asm2c.py, gen_romtab.py and the port's CMakeLists read)")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    if args.out is None:
        args.out = os.path.join(GAME_INC, "inventory.%s.json" % args.version)
        if not os.path.exists(args.out):
            args.out = os.path.join(GAME_INC, "inventory.json")
    VERSION_DEFINE[:] = ["-DVERSION_" + args.version.upper().replace(".", "_")]
    tmp = os.path.join(BLAST, "build", "fieldscan")
    os.makedirs(tmp, exist_ok=True)
    T = Types(tmp)
    types = build_types(T)
    F.DUMPS.extend(args.dumps)
    prog = F.Program(args.version)
    an = F.Analyzer(prog)
    an.run()
    res = F.Results(prog, an)
    res.classify()
    F.FUNCNAMES.update(res.funcname)
    dynpath = os.path.join(GAME_INC, "inventory_dumps.json")
    if not args.dumps and os.path.exists(dynpath):
        F.DYN_CACHE.update(json.load(open(dynpath)))
    regions, elem, kinds = F.write_text(prog, an, res, tmp)
    if args.dumps:
        seen = {}
        for region, flist in regions.items():
            for off, a in flist:
                if a.get("dyn"):
                    seen[F.field_name(region, off, elem.get(region, 0))] = a["dyn"]
        with open(dynpath, "w") as f:
            json.dump({k: seen[k] for k in sorted(seen)}, f, indent=0)
            f.write("\n")
    inv = {
        "about": "tools/inventory.py; see docs/TYPES.md. Addresses are us.v11's.",
        "types": types,
        "symbols": build_symbols(T, prog, res, types),
        "loads": build_loads(prog, an, res),
        "punning": build_punning(prog, an, res, regions, elem),
        "packed": build_packed(prog, an, res, elem),
    }
    inv["asm_uses"], inv["pointers"] = build_asm_uses_and_pointers(prog, an, res, regions, elem, kinds, T, types)
    atypes, asyms = build_asm_data_types(prog, an, res, elem, inv["symbols"], types)
    types.update(atypes)
    inv["symbols"].update(asyms)
    text = json.dumps(inv, indent=1, sort_keys=False) + "\n"
    if args.check:
        old = open(args.out).read() if os.path.exists(args.out) else ""
        if old != text:
            sys.exit("%s is out of date: run tools/inventory.py" % os.path.relpath(args.out, ROOT))
        print("inventory up to date")
        return
    open(args.out, "w").write(text)
    print("%s: %d types, %d symbols, %d loads, %d punned, %d packed, %d asm-used types, %d pointer fields" % (
        os.path.relpath(args.out, ROOT), len(types), len(inv["symbols"]), len(inv["loads"]), len(inv["punning"]),
        len(inv["packed"]), len(inv["asm_uses"]), len(inv["pointers"])))


if __name__ == "__main__":
    main()
