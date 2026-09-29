#!/usr/bin/env python3
"""Make a C file define its object's .data.

Usage (from blastcorps/):
  data_c.py <module> <object> [--version us.v11] [--dry-run] [--ref <.data.s>]
  data_c.py --check <module> <object> [--version us.v11] [--ref <.data.s>]

The object's block must still be an asm `data` subsegment, extracted to
asm/data/<module>/<object>.data.s (or --ref names a saved copy of that file,
to redo a file whose block is `.data` already): that file gives the labels
and the symbols in pointer words, the module binary the bytes.  This writes
one definition per variable, in address order, after the file's .bss block
(or before its first function), and drops the file's `extern` declarations
of them.  Then switch the block to `.data` in the config and re-extract.

- A variable the file declares keeps its declared type; so does one only a
  header or another C file (of any module) declares (that file's typedefs are
  copied along when this one lacks them).  An array of unknown size runs to
  the next label something uses; labels in its first element are fields.
- The initializer comes from the bytes, laid out by the type: structs
  (bitfields laid out, not initialized), arrays, floats, strings for `char`
  arrays, pointers from the symbols (`&table[3]` for one into a variable
  here), and Gfx as the Fast3D gs* macros.  A display list's physical
  address of something in a module becomes STATIC_K0_TO_PHYS(symbol).
- One nobody declares gets a type from the directive splat used: `.word`
  s32 (void * if it holds pointers), `.short` s16, `.byte` u8, `.float` f32,
  `.ascii` char.  Give it its real type by declaring it (a struct, a Vtx
  array, ...) and running this again.  Nonzero bytes in a struct's padding
  are an error: the struct is missing a field.
- A label inside a variable (another object's reference to a field) is not
  defined.  tools/link_syms.py makes it relative, but a name only C uses
  needs a line in undefined_syms.<module>.<version>.txt, as does a pointer
  into another object's data that no asm names.
- Typedefs the definitions need that the file only has further down move
  up, above the .bss block.

IDO lays .data out in definition order, not first use (unlike .bss), aligns
every variable to at least 4 (8 for doubles, 64-bit integers and the GBI
unions) and pads the section to 16, so a label at an address that isn't
4-aligned can only be inside a variable, and zero bytes the alignment
doesn't account for become padding arrays.

--check compiles nothing: it compares the object the last build made
(build/src/<module>/<object>.c.o) with the block.

For a version other than us.v11, names are us.v11's (tools/vermap.py): a
variable no label names is D_<its us.v11 address>, or D_<address>_<version>
if us.v11 doesn't have it.
"""
import argparse
import codecs
import collections
import copy
import decimal
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "mips_to_c"))
import bss_c  # noqa: E402
import modmap  # noqa: E402
import vermap  # noqa: E402

try:
    from pycparser import c_ast, c_generator, c_parser
except ImportError:  # the project venv has it (m2c needs it)
    sys.exit("data_c.py needs pycparser: run it with the project venv's python")


# --- the reference: labels, bytes and pointer words ---------------------------

def subsegments(cfg_path):
    """[(offset, kind, name)] of the code segment's subsegments."""
    return [(int(o, 16), k, n) for o, k, n in
            re.findall(r"- \[0x([0-9A-F]+), (\.?\w+), ([\w/]+)\]", Path(cfg_path).read_text())]


def block(cfg_path, mod, obj):
    subs = subsegments(cfg_path)
    for k, (off, kind, name) in enumerate(subs):
        if name == f"{mod}/{obj}" and kind in ("data", ".data"):
            return off, subs[k + 1][0], kind
    sys.exit(f"{obj} has no .data block in {cfg_path}")


def unescape(s):
    return codecs.decode(s.encode("latin-1"), "unicode_escape").encode("latin-1")


def reference(path, start):
    """([(offset, name)], {offset: symbol}) of an asm .data file, offsets
    from the block's start."""
    labels, syms, pos = [], {}, 0
    for line in Path(path).read_text().splitlines():
        line = line.strip()
        m = re.match(r"dlabel (\w+)", line)
        if m:
            labels.append((pos, m.group(1)))
            continue
        m = re.match(r"\.(byte|short|word|float|double|ascii|asciz|space)\s+(.*)$", line)
        if not m:
            continue
        d, rest = m.groups()
        if d in ("ascii", "asciz"):
            s = unescape(re.match(r'"(.*)"$', rest).group(1))
            pos += len(s) + (d == "asciz")
            continue
        if d == "space":
            pos += int(rest, 0)
            continue
        items = [x.strip() for x in rest.split(",")]
        size = {"byte": 1, "short": 2, "word": 4, "float": 4, "double": 8}[d]
        for x in items:
            if d == "word" and re.match(r"[A-Za-z_]", x):
                syms[pos] = x
            pos += size
    return labels, syms, pos


# --- C types ------------------------------------------------------------------

BASE = {  # resolved base type names: (size, kind)
    "char": (1, "s"), "signed char": (1, "s"), "unsigned char": (1, "u"),
    "short": (2, "s"), "short int": (2, "s"), "signed short": (2, "s"), "unsigned short": (2, "u"),
    "unsigned short int": (2, "u"),
    "int": (4, "s"), "signed int": (4, "s"), "signed": (4, "s"), "unsigned": (4, "u"),
    "unsigned int": (4, "u"), "long": (4, "s"), "long int": (4, "s"), "signed long": (4, "s"),
    "unsigned long": (4, "u"), "unsigned long int": (4, "u"),
    "long long": (8, "s"), "long long int": (8, "s"), "signed long long": (8, "s"),
    "unsigned long long": (8, "u"), "unsigned long long int": (8, "u"),
    "float": (4, "f"), "double": (8, "d"), "long double": (8, "d"),
}


class Types:
    def __init__(self, text):
        self.ast = c_parser.CParser().parse(text)
        self.typedefs, self.structs, self.vars = {}, {}, {}
        for node in self.ast.ext:
            if isinstance(node, c_ast.Typedef):
                self.typedefs[node.name] = node.type
                self._note_struct(node.type)
            elif isinstance(node, c_ast.Decl):
                self._note_struct(node.type)
                if node.name and not isinstance(node.type, c_ast.FuncDecl):
                    old = self.vars.get(node.name)
                    # prefer a declaration with array bounds
                    if old is None or (isinstance(node.type, c_ast.ArrayDecl) and node.type.dim is not None):
                        self.vars[node.name] = node.type
                elif node.name:
                    self.vars.setdefault(node.name, node.type)

    def _note_struct(self, t):
        while isinstance(t, (c_ast.TypeDecl, c_ast.PtrDecl, c_ast.ArrayDecl)):
            t = t.type
        if isinstance(t, (c_ast.Struct, c_ast.Union)) and t.decls is not None and t.name:
            self.structs[(type(t).__name__, t.name)] = t
        if isinstance(t, (c_ast.Struct, c_ast.Union)) and t.decls:
            for d in t.decls:
                self._note_struct(d.type)

    def resolve(self, t):
        """Strip TypeDecls and typedefs down to a PtrDecl, ArrayDecl,
        FuncDecl, Struct, Union, Enum or ('base', size, kind)."""
        while True:
            if isinstance(t, c_ast.TypeDecl):
                t = t.type
            elif isinstance(t, c_ast.IdentifierType):
                names = [n for n in t.names if n not in ("const", "volatile")]
                key = " ".join(names)
                if key in BASE:
                    return ("base",) + BASE[key]
                if len(names) == 1 and names[0] in self.typedefs:
                    t = self.typedefs[names[0]]
                    continue
                if key == "void":
                    return ("void",)
                raise ValueError(f"unknown type {key}")
            elif isinstance(t, (c_ast.Struct, c_ast.Union)) and t.decls is None:
                full = self.structs.get((type(t).__name__, t.name))
                if full is None:
                    raise ValueError(f"incomplete {type(t).__name__.lower()} {t.name}")
                return full
            else:
                return t

    def const(self, e):
        if isinstance(e, c_ast.Constant):
            v = e.value.rstrip("uUlL")
            return int(v, 0) if not v.startswith("0") or v.startswith("0x") or v == "0" else int(v, 8)
        if isinstance(e, c_ast.BinaryOp):
            a, b = self.const(e.left), self.const(e.right)
            return {"+": a + b, "-": a - b, "*": a * b, "/": a // b, "<<": a << b, ">>": a >> b,
                    "|": a | b, "&": a & b}[e.op]
        if isinstance(e, c_ast.UnaryOp):
            if e.op == "sizeof":
                return self.size(e.expr.type if isinstance(e.expr, c_ast.Typename) else self.vars[e.expr.name])
            if e.op == "-":
                return -self.const(e.expr)
        raise ValueError(f"can't evaluate {e}")

    def size(self, t):
        return self.layout(t)[0]

    def layout(self, t):
        """(size, alignment)"""
        r = self.resolve(t)
        if isinstance(r, tuple):
            if r[0] == "void":
                raise ValueError("void object")
            return r[1], r[1]
        if isinstance(r, (c_ast.PtrDecl, c_ast.Enum)):
            return 4, 4
        if isinstance(r, c_ast.ArrayDecl):
            n = self.const(r.dim) if r.dim is not None else 0
            s, a = self.layout(r.type)
            return n * s, a
        if isinstance(r, (c_ast.Struct, c_ast.Union)):
            bits, align, size = 0, 1, 0  # bits: the bit offset in a struct
            for d in r.decls:
                s, a = self.layout(d.type)
                align = max(align, a)
                if isinstance(r, c_ast.Union):
                    size = max(size, s)
                    continue
                if d.bitsize is not None:
                    n = self.const(d.bitsize)
                    if bits // (8 * s) != (bits + n - 1) // (8 * s):
                        bits = (bits + 8 * s - 1) // (8 * s) * (8 * s)
                    bits += n
                else:
                    off = ((bits + 7) // 8 + a - 1) // a * a
                    bits = 8 * (off + s)
                size = (bits + 7) // 8
            return (size + align - 1) // align * align, align
        raise ValueError(f"no layout for {type(r).__name__}")

    def fields(self, r):
        """[(offset, name, type)] of a struct."""
        out, off = [], 0
        for d in r.decls:
            if d.bitsize is not None:
                raise ValueError(f"bitfields in struct {r.name}: write this initializer by hand")
            s, a = self.layout(d.type)
            off = (off + a - 1) // a * a
            out.append((off, d.name, d.type))
            off += s
        return out


# --- Fast3D display lists (2.0D; include/2.0I/PR/gbi.h without F3DEX_GBI) -------

GEOMETRY = [(0x1, "G_ZBUFFER"), (0x2, "G_TEXTURE_ENABLE"), (0x4, "G_SHADE"), (0x200, "G_SHADING_SMOOTH"),
            (0x1000, "G_CULL_FRONT"), (0x2000, "G_CULL_BACK"), (0x10000, "G_FOG"), (0x20000, "G_LIGHTING"),
            (0x40000, "G_TEXTURE_GEN"), (0x80000, "G_TEXTURE_GEN_LINEAR"), (0x100000, "G_LOD")]
IMFMT = ["G_IM_FMT_RGBA", "G_IM_FMT_YUV", "G_IM_FMT_CI", "G_IM_FMT_IA", "G_IM_FMT_I"]
IMSIZ = ["G_IM_SIZ_4b", "G_IM_SIZ_8b", "G_IM_SIZ_16b", "G_IM_SIZ_32b"]


def flags(v, names):
    out = [n for b, n in names if v & b]
    rest = v & ~sum(b for b, _ in names)
    if rest:
        out.append(f"0x{rest:X}")
    return " | ".join(out) or "0"


def gfx_macro(w0, w1, ptr):
    """The gs* macro that builds (w0, w1), or None.  ptr() gives the
    pointer in w1 as C (None if w1 isn't one)."""
    op = w0 >> 24
    fmt = lambda f: IMFMT[f] if f < len(IMFMT) else str(f)
    if op == 0x04:
        n, v0, size = ((w0 >> 20) & 0xF) + 1, (w0 >> 16) & 0xF, w0 & 0xFFFF
        p = ptr()
        if size == n * 16 and p:
            return f"gsSPVertex({p}, {n}, {v0})"
    elif op == 0x06 and w0 in (0x06000000, 0x06010000):
        p = ptr()
        if p:
            return f"gsSP{'DisplayList' if w0 == 0x06000000 else 'BranchList'}({p})"
    elif op == 0x01 and w0 & 0xFFFF == 0x40 and ((w0 >> 16) & 0xFF) < 8:
        p = ptr()
        if p:
            m = (w0 >> 16) & 0xFF
            parts = ["G_MTX_PROJECTION" if m & 1 else "G_MTX_MODELVIEW", "G_MTX_LOAD" if m & 2 else "G_MTX_MUL",
                     "G_MTX_PUSH" if m & 4 else "G_MTX_NOPUSH"]
            return f"gsSPMatrix({p}, {' | '.join(parts)})"
    elif op == 0xBC:
        index, offset = w0 & 0xFF, (w0 >> 8) & 0xFFFF
        where = {0x10: "G_MWO_POINT_RGBA", 0x14: "G_MWO_POINT_ST", 0x18: "G_MWO_POINT_XYSCREEN",
                 0x1C: "G_MWO_POINT_ZSCREEN"}
        if index == 0x0C and offset % 40 in where:
            return f"gsSPModifyVertex({offset // 40}, {where[offset % 40]}, 0x{w1:X})"
        return f"gsMoveWd({index}, 0x{offset:X}, 0x{w1:X})"
    elif w0 == 0xB8000000 and w1 == 0:
        return "gsSPEndDisplayList()"
    elif w0 == 0xBF000000 and all(((w1 >> s) & 0xFF) % 10 == 0 for s in (0, 8, 16)):
        return f"gsSP1Triangle({(w1 >> 16 & 0xFF) // 10}, {(w1 >> 8 & 0xFF) // 10}, {(w1 & 0xFF) // 10}, {w1 >> 24})"
    elif w0 == 0xB7000000:
        return f"gsSPSetGeometryMode({flags(w1, GEOMETRY)})"
    elif w0 == 0xB6000000:
        return f"gsSPClearGeometryMode({flags(w1, GEOMETRY)})"
    elif w1 == 0 and w0 in (0xE7000000, 0xE6000000, 0xE8000000, 0xE9000000):
        return {0xE7: "gsDPPipeSync()", 0xE6: "gsDPLoadSync()", 0xE8: "gsDPTileSync()", 0xE9: "gsDPFullSync()"}[op]
    elif op in (0xB9, 0xBA) and not w0 & 0xFF0000:
        return (f"gsSPSetOtherMode(G_SETOTHERMODE_{'L' if op == 0xB9 else 'H'}, {(w0 >> 8) & 0xFF}, "
                f"{w0 & 0xFF}, 0x{w1:X})")
    elif op == 0xFC:
        return f"gsDPSetCombine(0x{w0 & 0xFFFFFF:06X}, 0x{w1:08X})"
    elif op in (0xFA,) and not w0 & 0xFF0000:
        return (f"gsDPSetPrimColor({(w0 >> 8) & 0xFF}, {w0 & 0xFF}, {w1 >> 24}, {(w1 >> 16) & 0xFF}, "
                f"{(w1 >> 8) & 0xFF}, {w1 & 0xFF})")
    elif op in (0xFB, 0xF8, 0xF9) and w0 == op << 24:
        name = {0xFB: "EnvColor", 0xF8: "FogColor", 0xF9: "BlendColor"}[op]
        return f"gsDPSet{name}({w1 >> 24}, {(w1 >> 16) & 0xFF}, {(w1 >> 8) & 0xFF}, {w1 & 0xFF})"
    elif w0 == 0xF7000000:
        return f"gsDPSetFillColor(0x{w1:08X})"
    elif op == 0xBB and not w0 & 0xFFC000:
        return (f"gsSPTexture(0x{w1 >> 16:X}, 0x{w1 & 0xFFFF:X}, {(w0 >> 11) & 7}, {(w0 >> 8) & 7}, "
                f"{w0 & 0xFF})")
    elif op == 0xFD and not w0 & 0x7F000:
        p = ptr()
        if p:
            return (f"gsDPSetTextureImage({fmt((w0 >> 21) & 7)}, {IMSIZ[(w0 >> 19) & 3]}, "
                    f"{(w0 & 0xFFF) + 1}, {p})")
    elif op == 0xF5 and not w0 & 0x40000 and not w1 & 0xF8000000:
        return (f"gsDPSetTile({fmt((w0 >> 21) & 7)}, {IMSIZ[(w0 >> 19) & 3]}, {(w0 >> 9) & 0x1FF}, "
                f"0x{w0 & 0x1FF:X}, {(w1 >> 24) & 7}, {(w1 >> 20) & 0xF}, {(w1 >> 18) & 3}, {(w1 >> 14) & 0xF}, "
                f"{(w1 >> 10) & 0xF}, {(w1 >> 8) & 3}, {(w1 >> 4) & 0xF}, {w1 & 0xF})")
    elif op in (0xF2, 0xF4) and not w1 & 0xF8000000:
        name = "gsDPSetTileSize" if op == 0xF2 else "gsDPLoadTile"
        return (f"{name}({(w1 >> 24) & 7}, {(w0 >> 12) & 0xFFF}, {w0 & 0xFFF}, {(w1 >> 12) & 0xFFF}, "
                f"{w1 & 0xFFF})")
    elif op == 0xF3 and not w1 & 0xF8000000 and ((w1 >> 12) & 0xFFF) <= 2047:
        return (f"gsDPLoadBlock({(w1 >> 24) & 7}, {(w0 >> 12) & 0xFFF}, {w0 & 0xFFF}, {(w1 >> 12) & 0xFFF}, "
                f"{w1 & 0xFFF})")
    return None


# --- initializers -------------------------------------------------------------

def type_text(t):
    """C for a type node, without the declarator's name."""
    t = copy.deepcopy(t)
    n = t
    while n is not None and not isinstance(n, c_ast.IdentifierType):
        if isinstance(n, c_ast.TypeDecl):
            n.declname = None
        n = getattr(n, "type", None)
    return c_generator.CGenerator()._generate_type(t).strip()


def fmt_int(v, size, kind):
    if kind == "s" and v >= 1 << (8 * size - 1):
        v -= 1 << (8 * size)
    if (kind == "s" and -0x10000 < v < 0x10000) or 0 <= v < 0x100 or (size == 1 and v >= 0):
        return str(v)
    return f"-0x{-v:X}" if v < 0 else f"0x{v:X}"


def fmt_float(bits, double):
    if double:
        v = struct.unpack(">d", bits.to_bytes(8, "big"))[0]
        pack = lambda x: struct.pack(">d", x)
    else:
        v = struct.unpack(">f", bits.to_bytes(4, "big"))[0]
        pack = lambda x: struct.pack(">f", x)
    if v != v or v in (float("inf"), float("-inf")):
        raise ValueError("NaN/inf in a float")
    for p in range(1, 18):
        s = f"{v:.{p}g}"
        if pack(float(s)) == pack(v):
            break
    if "e" in s and 1e-4 <= abs(v) < 1e10:
        s = format(decimal.Decimal(s), "f")
    if "e" not in s and "." not in s:
        s += ".0"
    return s + ("" if double else "f")


class Gen:
    def __init__(self, types, blob, syms, arrays, need, inner=None):
        self.t, self.blob, self.syms = types, blob, syms
        self.arrays = arrays  # names to use bare (arrays and functions)
        self.need = need      # symbols the initializers use: {name}
        self.inner = inner or {}  # labels inside this block's variables: {name: expression}
        self.ptypes = {}          # undeclared symbols: {name: type they point to}
        self.literals = {}        # const strings to write as literals: {name: "text"}
        self.consts = set()       # names of const arrays (strings in .rodata)
        self.ends = {}            # {variable: (array before it, its length, element type)}
        self.arrays_of = {}       # {element type: names of this block's arrays of it}
        self.used_literals = set()
        self.phys_ok = None       # address -> bool: may a physical address point there
        self.phys_name = None     # address -> C expression for it

    def ptr(self, off, pointee=None):
        """A pointer word as C.  pointee: the type text it points to, for
        the extern declaration of a symbol the file doesn't declare."""
        sym = self.syms.get(off)
        if sym in self.literals and pointee in ("char", "u8"):
            self.used_literals.add(sym)
            return self.literals[sym]
        if sym in self.ends and pointee == self.ends[sym][2] and sym not in self.arrays_of.get(pointee, ()):
            # one past the end of the array before it (a table's end pointer)
            self.need.add(self.ends[sym][0])
            return f"&{self.ends[sym][0]}[{self.ends[sym][1]}]"
        if sym in self.consts and pointee and not pointee.startswith("const"):
            self.need.add(sym)
            return f"({pointee} *){sym}"  # a string in .rodata: const
        if sym is not None and pointee and sym not in self.t.vars:
            self.ptypes.setdefault(sym, pointee)
        if sym in self.inner:
            self.need.add(self.inner[sym][1])
            return self.inner[sym][0]
        if sym is None:
            v = int.from_bytes(self.blob[off:off + 4], "big")
            if v and VM.name("D", v, True) in self.t.vars:
                # a fixed address the file names (undefined_syms)
                sym = VM.name("D", v, True)
                return sym if sym in self.arrays else f"&{sym}"
            if v:
                raise ValueError(f"pointer 0x{v:08X} at +0x{off:X} has no symbol")
            return "NULL"
        self.need.add(sym)
        return sym if sym in self.arrays or sym not in self.t.vars else f"&{sym}"

    def value(self, t, off, count=None, depth=0):
        """Initializer text for type t at blob offset off."""
        if self.is_named(t, "Gfx"):
            w0, w1 = struct.unpack(">II", self.blob[off:off + 8])
            pointee = {0x04: "Vtx", 0x06: "Gfx", 0x01: "Mtx"}.get(w0 >> 24)
            m = gfx_macro(w0, w1, lambda: self.gfx_ptr(off + 4, pointee))
            if m:
                return m
            return "{ { " + ", ".join(self.value(c_ast.TypeDecl(None, [], None, c_ast.IdentifierType(["unsigned", "int"])), o)
                                    for o in (off, off + 4)) + " } }"
        r = self.t.resolve(t)
        if isinstance(r, tuple):
            _, size, kind = r
            if off in self.syms:
                self.need.add(self.syms[off])
                return f"(u32){self.ptr(off)}"
            v = int.from_bytes(self.blob[off:off + size], "big")
            if kind in "fd":
                return fmt_float(v, kind == "d")
            return fmt_int(v, size, kind)
        if isinstance(r, c_ast.Enum):
            return fmt_int(int.from_bytes(self.blob[off:off + 4], "big"), 4, "s")
        if isinstance(r, c_ast.PtrDecl):
            pointee = type_text(r.type)
            return self.ptr(off, pointee if pointee != "void" else None)
        if isinstance(r, c_ast.ArrayDecl):
            n = count if count is not None else self.t.const(r.dim)
            es = self.t.size(r.type)
            e = self.t.resolve(r.type)
            if isinstance(e, tuple) and e[1] == 1 and self.is_char(r.type):
                s = bytes(self.blob[off:off + n])
                text = s.rstrip(b"\0")
                if all(32 <= c < 127 for c in text) and len(text) < n:
                    return '"' + text.decode().replace("\\", "\\\\").replace('"', '\\"') + '"'
            items = [self.value(r.type, off + i * es, depth=depth + 1) for i in range(n)]
            while items and self.zero(r.type, off + (len(items) - 1) * es):
                items.pop()
            if not items:
                items = ["0"] if isinstance(e, tuple) or isinstance(e, (c_ast.PtrDecl, c_ast.Enum)) else ["{ 0 }"]
            return self.join(items, depth, isinstance(e, (c_ast.Struct, c_ast.Union, c_ast.ArrayDecl)))
        if isinstance(r, c_ast.Union):
            first = r.decls[0]
            s = self.t.size(first.type)
            if any(self.blob[off + s:off + self.t.size(r)]):
                raise ValueError(f"union {r.name} ({first.name}, {s} of {self.t.size(r)}) at +0x{off:X}: bytes past its first member")
            return "{ " + self.value(first.type, off, depth=depth + 1) + " }"
        if isinstance(r, c_ast.Struct):
            fs = self.t.fields(r)
            covered = set()
            for fo, _, ft in fs:
                covered |= set(range(fo, fo + self.t.size(ft)))
            loose = [o for o in range(self.t.size(r)) if o not in covered and (self.blob[off + o] or off + o in self.syms)]
            if loose:
                raise ValueError(f"struct {r.name} at +0x{off:X}: nonzero padding at +0x{loose[0]:X}")
            items = [self.value(ft, off + fo, depth=depth + 1) for fo, _, ft in fs]
            return self.join(items, depth, False)
        raise ValueError(f"no initializer for {type(r).__name__}")

    @staticmethod
    def is_named(t, name):
        while isinstance(t, c_ast.TypeDecl):
            t = t.type
        return isinstance(t, c_ast.IdentifierType) and t.names == [name]

    def gfx_ptr(self, off, pointee=None):
        """A display list's pointer word as C: a symbol, or a physical
        address (KSEG0 minus K0BASE) of something in the module."""
        if off in self.syms:
            return self.ptr(off, pointee)
        v = int.from_bytes(self.blob[off:off + 4], "big")
        a = v + 0x80000000
        if self.phys_ok and self.phys_ok(a):
            expr = self.phys_name(a)
            return f"STATIC_K0_TO_PHYS({expr})"
        return None

    def is_char(self, t):
        while isinstance(t, c_ast.TypeDecl):
            t = t.type
        return isinstance(t, c_ast.IdentifierType) and t.names == ["char"]

    def zero(self, t, off):
        s = self.t.size(t)
        return not any(self.blob[off:off + s]) and not any(o in self.syms for o in range(off, off + s, 4))

    @staticmethod
    def join(items, depth=0, aggregates=False):
        """Brace a list of initializers: on one line if it fits, else one
        aggregate (or display list command) per line, or rows of scalars."""
        ind, ind1 = "    " * depth, "    " * (depth + 1)
        one = "{ " + ", ".join(items) + " }"
        if len(one) + len(ind1) <= 100 and "\n" not in one:
            return one
        lines, cur = [], ""
        for it in items:
            alone = aggregates or it.startswith("gs") or "\n" in it
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

    def path(self, t, off):
        """The C for the object at byte off inside a variable of type t, as a
        suffix to the variable's name (`[1].unk280`), or None."""
        r = self.t.resolve(t)
        if off == 0:
            return ""
        if isinstance(r, c_ast.ArrayDecl):
            es = self.t.size(r.type)
            rest = self.path(r.type, off % es)
            return None if rest is None else f"[{off // es}]{rest}"
        if isinstance(r, c_ast.Struct):
            for fo, fname, ft in reversed(self.t.fields(r)):
                if fo <= off < fo + self.t.size(ft):
                    rest = self.path(ft, off - fo)
                    return None if rest is None else f".{fname}{rest}"
        return None


# --- the plan -----------------------------------------------------------------

GUESS = {  # splat's directive -> (type text, C spelling for the parser, size)
    "void *": ("void *", "void *", 4), "u32": ("u32", "unsigned int", 4), "s32": ("s32", "int", 4),
    "f32": ("f32", "float", 4), "s16": ("s16", "short", 2), "u16": ("u16", "unsigned short", 2),
    "u8": ("u8", "unsigned char", 1), "char": ("char", "char", 1),
}


def guess(asm_text, name, span, off, syms):
    """A type for a variable nobody declares, from the directive splat used."""
    m = re.search(r"dlabel %s\n\.(\w+)" % re.escape(name), asm_text)
    d = m.group(1) if m else "word"
    words = [o for o in range(off, off + span, 4) if o in syms]
    if d == "word" and words and len(words) == span // 4:
        return "void *"
    if d == "word" and words:
        return "u32"
    return {"float": "f32", "short": "s16", "ascii": "char", "asciz": "char",
            "byte": "s32" if span == 4 else "u8"}.get(d, "s32")


def parse_one(text):
    return c_parser.CParser().parse(text).ext[-1].type


def dims_text(n):
    return f"[{n:#x}]" if n > 9 else f"[{n}]"


def plan(labels, syms, blob, types, decls, asm_text, used):
    """[(offset, name, (type, ptr, dims), type node, count)] and the labels
    left inside variables.  Only labels something uses (or that a C file
    declares) start a variable."""
    size_total = len(blob)
    labels = [(o, n) for o, n in labels if n in used or n in decls or o == 0]
    starts = sorted({o for o, _ in labels if o % 4 == 0})
    names = {}
    for o, n in labels:
        names.setdefault(o, []).append(n)
    out, inner = [], []
    pos = 0

    def add_guessed(off, span, name):
        g = guess(asm_text, name, span, off, syms)
        typ, spelled, esz = GUESS[g]
        n = span // esz
        ptr = ""
        if typ == "void *":
            typ, ptr = "void", "*"
        node = parse_one(f"{spelled} x{dims_text(n) if n > 1 else ''};")
        out.append((off, name, (typ, ptr, dims_text(n) if n > 1 else ""), node, None))
        return off + n * esz

    def gap(pos, off, align=4):
        """Cover [pos, off) where no label is: alignment, zeros or data.
        align is what IDO aligns the variable at off to."""
        q = (pos + 3) // 4 * 4
        if q >= off or ((pos + align - 1) // align * align == off and not any(blob[pos:off])):
            return
        if any(blob[q:off]) or any(o in syms for o in range(q, off)):
            first = next(o for o in range(q, off, 4) if any(blob[o:o + 4]) or o in syms)
            if first > q:
                out.append((q, None, ("u8", "", dims_text(first - q)), parse_one(f"unsigned char x[{first - q}];"), None))
            add_guessed(first, off - first, f"D_{first:X}@")
            return
        else:
            out.append((q, None, ("u8", "", dims_text(off - q)), parse_one(f"unsigned char x[{off - q}];"), None))

    k = 0
    while k < len(starts):
        off = starts[k]
        if off < pos:
            inner += names[off]
            k += 1
            continue
        cand = names[off]
        name = next((n for n in cand if n in decls), cand[0])
        gap(pos, off, max(4, types.layout(decls[name][3])[1]) if name in decls else 4)
        inner += [n for n in cand if n != name]
        nxt = next((s for s in starts[k + 1:]), size_total)
        if name in decls:
            r = types.resolve(decls[name][3])
            if isinstance(r, c_ast.ArrayDecl) and r.dim is None:
                # labels in an unbounded array's first element are its fields
                es = types.size(r.type)
                nxt = next((s for s in starts[k + 1:] if s >= off + es), size_total)
        span = nxt - off
        if name in decls:
            t = decls[name][3]
            r = types.resolve(t)
            typ, ptr, dims = decls[name][:3]
            count = None
            if isinstance(r, c_ast.ArrayDecl) and r.dim is None:
                es = types.size(r.type)
                count = max(1, span // es)
                dims = dims_text(count) + dims[2:]
                size = count * es
            else:
                size = types.size(t)
            if off % types.layout(t)[1]:
                raise ValueError(f"{name} at +0x{off:X} is misaligned for its type")
            out.append((off, name, (typ, ptr, dims), t, count))
            pos = off + size
        else:
            pos = add_guessed(off, span, name)
        k += 1
    inner += [n for o, ns in names.items() if o % 4 for n in ns]
    if (pos + 15) // 16 * 16 < size_total:
        gap(pos, size_total)
    return out, inner


VM = vermap.VerMap("us.v11")


def render(entries, types, blob, syms, block_vram, labels=(), version="us.v11", literals=None):
    arrays = set()
    for name, t in types.vars.items():
        r = types.resolve(t) if not isinstance(t, c_ast.FuncDecl) else t
        if isinstance(r, (c_ast.ArrayDecl, c_ast.FuncDecl)):
            arrays.add(name)
    for off, name, (typ, ptr, dims), node, count in entries:
        if name and dims:
            arrays.add(name)
    # a pointer to an element of one of the block's arrays
    inner = {}
    for lo, ln in labels:
        for off, name, (typ, ptr, dims), node, count in entries:
            if not name or name.endswith("@"):
                continue
            size = types.size(node) if count is None else count * types.size(types.resolve(node).type)
            if not off <= lo < off + size or ln == name:
                continue
            full = node if count is None else c_ast.ArrayDecl(types.resolve(node).type,
                                                               c_ast.Constant("int", str(count)), [])
            p = Gen(types, blob, syms, set(), set()).path(full, lo - off)
            if p:
                inner[ln] = (f"&{name}{p}", name)
    need = set()
    g = Gen(types, blob, syms, arrays, need, inner)
    g.literals = literals or {}
    prev = None
    for off, name, dims_info, node, count in entries:
        r = types.resolve(node)
        if name and isinstance(r, c_ast.ArrayDecl):
            et = type_text(r.type)
            n = count if count is not None else types.const(r.dim)
            g.arrays_of.setdefault(et, set()).add(name)
            cur = (off, name, et, n, off + n * types.size(r.type))
        else:
            cur = (off, name, None, None, off + (types.size(node) if name else 0))
        if prev is not None and prev[2] is not None and prev[4] == off and name:
            g.ends[name] = (prev[1], prev[3], prev[2])
        prev = cur
    g.consts = {n for n, tt in types.vars.items() if "const" in type_text(tt)}
    mods = modmap.modules(version)
    g.phys_ok = lambda a: any(m.contains(a) for m in mods.values())

    def phys_name(a):
        o = a - block_vram
        for off, name, (typ, ptr, dims), node, count in entries:
            size = types.size(node) if count is None else count * types.size(types.resolve(node).type)
            if off <= o < off + size:
                name = VM.name("D", block_vram + off, True) if name is None or name.endswith("@") else name
                p = g.path(node, o - off) if count is None else g.path(
                    c_ast.ArrayDecl(types.resolve(node).type, c_ast.Constant("int", str(count)), []), o - off)
                if p is not None:
                    return f"&{name}{p}" if p or not dims else name
                return f"(u8 *){name if dims else '&' + name} + 0x{o - off:X}"
        name = VM.name("D", a, True)
        need.add(name)
        return name if name in arrays or name not in types.vars else f"&{name}"
    g.phys_name = phys_name
    lines = []
    for off, name, (typ, ptr, dims), node, count in entries:
        name = VM.name("D", block_vram + off, True) if name is None or name.endswith("@") else name
        init = g.value(node, off, count)
        lines.append(f"{typ} {ptr}{name}{dims} = {init};")
    return lines, need, g.ptypes, g.used_literals


def uncomment(text):
    return re.sub(r"/\*.*?\*/|//[^\n]*", " ", text, flags=re.S)


def find_typedefs(lines):
    """{name: (start, end)} of the file-scope typedefs in a list of lines
    (entries that aren't strings are skipped), comments above included."""
    blocks, i = {}, 0
    while i < len(lines):
        l = lines[i] if isinstance(lines[i], str) else ""
        if re.match(r"^typedef\s+(struct|union)\b[^;]*$", l):
            j = i
            while j < len(lines) and not (isinstance(lines[j], str) and re.match(r"^}\s*\w+\s*;", lines[j])):
                j += 1
            if j == len(lines):
                break
            name = re.match(r"^}\s*(\w+)", lines[j]).group(1)
            s = i
            while s > 0 and isinstance(lines[s - 1], str) and lines[s - 1].lstrip().startswith(("/*", "*")) \
                    and not lines[s - 1].rstrip().endswith("};"):
                s -= 1
            blocks[name] = (s, j + 1)
            i = j + 1
            continue
        m = re.match(r"^typedef\s[^;{]*\b(\w+)\s*;", l)
        if m:
            blocks[m.group(1)] = (i, i + 1)
        i += 1
    return blocks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("object")
    ap.add_argument("--version", default="us.v11")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--literals", action="store_true",
                    help="write the strings only this block points to as literals in its initializers")
    ap.add_argument("--literals-in", metavar="VAR[,VAR...]",
                    help="--literals, but only for the pointers in these variables")
    ap.add_argument("--ref", help="the block's asm (default asm/data/<module>/<object>.data.s), to redo a "
                    "file whose block is `.data` already from a saved extract")
    args = ap.parse_args()

    cfg = f"{args.module}.{args.version}.yaml"
    start, end, kind = block(cfg, args.module, args.object)
    vram = int(re.search(r"vram:\s+(0x[0-9A-Fa-f]+)", Path(cfg).read_text()).group(1), 16)
    blob = Path(f"{args.module}.{args.version}.bin").read_bytes()[start:end]
    asm = Path(args.ref) if args.ref else Path("asm") / "data" / args.module / f"{args.object}.data.s"
    global VM
    VM = vermap.load(args.version, Path("."))
    bss_c.VERSION_FLAGS[:] = ["-DVERSION_" + args.version.upper().replace(".", "_")]
    c_path = Path("src") / args.module / f"{args.object}.c"

    if args.check:
        obj = Path("build") / "src" / args.module / f"{args.object}.c.o"
        data = subprocess.run(["mips-linux-gnu-objcopy", "-O", "binary", "--only-section", ".data", str(obj),
                               "/dev/stdout"], capture_output=True).stdout
        rel = subprocess.run(["mips-linux-gnu-readelf", "-rW", str(obj)], capture_output=True, text=True).stdout
        sect = re.search(r"Relocation section '\.rel\.data'.*?\n\n", rel + "\n\n", re.S)
        relocs = {}
        if sect:
            for line in sect.group(0).splitlines():
                p = line.split()
                if len(p) >= 5 and re.match(r"[0-9a-f]{8}$", p[0]):
                    relocs[int(p[0], 16)] = p[4]
        bad = 0
        if len(data) != len(blob):
            print(f".data is 0x{len(data):X} bytes, the block 0x{len(blob):X}")
            bad += 1
        ref_syms = reference(asm, start)[1] if asm.exists() else {}
        for o in range(0, min(len(data), len(blob)), 4):
            if o in relocs or o in ref_syms:
                if relocs.get(o) != ref_syms.get(o) and not (o in relocs and relocs[o].startswith(".")):
                    print(f"+0x{o:X}: pointer to {relocs.get(o)}, should be {ref_syms.get(o)}")
                    bad += 1
            elif data[o:o + 4] != blob[o:o + 4]:
                print(f"+0x{o:X} (0x{vram + start + o:08X}): {data[o:o+4].hex()} should be {blob[o:o+4].hex()}")
                bad += 1
                if bad > 20:
                    break
        sys.exit(1 if bad else 0)

    if kind != "data" and not args.ref:
        sys.exit(f"{args.object}'s block is `{kind}`; this needs the asm `data` block extracted")
    labels, syms, size = reference(asm, start)
    if size != len(blob):
        sys.exit(f"{asm} is 0x{size:X} bytes, the block 0x{len(blob):X}")
    src = c_path.read_text()
    pre = bss_c.preprocess(c_path)
    pre = "\n".join(l for l in pre.splitlines() if not l.startswith("#") and "GLOBAL_ASM" not in l)

    # what uses each name: every module's asm and C, and the hand-written
    # symbol files (a label nobody uses doesn't start a variable)
    uses = collections.Counter()
    for p in [*Path("asm").rglob("*.s"), *Path("src").rglob("*.c"),
              *Path(".").glob(f"undefined_syms.*.{args.version}.txt")]:
        if p == asm:
            continue
        uses.update(set(re.findall(r"\b[A-Za-z_]\w*\b", p.read_text())))
    self_uses = collections.Counter(re.findall(r"\b[A-Za-z_]\w*\b", asm.read_text()))
    used = {n for n, c in self_uses.items() if uses[n] or c > 1}
    # names the C uses for addresses in the block that splat never labelled
    have = {n for _, n in labels}
    for p in Path("src").rglob("*.c"):
        for n in set(re.findall(r"\bD_([0-9A-F]{8})\b", p.read_text())):
            a = VM.original(f"D_{n}")
            if a is None:
                continue
            a -= vram + start
            if 0 <= a < end - start and f"D_{n}" not in have:
                labels.append((a, f"D_{n}"))
                have.add(f"D_{n}")
                used.add(f"D_{n}")
    labels.sort(key=lambda x: x[0])

    names = [n for _, n in labels]
    local = bss_c.find_decls(src, names)
    # names this file doesn't declare: another file's declaration, if its
    # type makes sense here
    borrowed = {}
    want = [x for x in names if x not in local]
    for other in sorted(c_path.parent.parent.glob("*/*.c")) if want else []:
        if other == c_path:
            continue
        for n, v in bss_c.find_decls(other.read_text(), want).items():
            _, typ, ptr, dims, _ = v[-1]
            # prefer a declaration with bounds, then the most detailed struct
            ol = other.read_text().split("\n")
            ob = find_typedefs(ol)
            w = next((x for x in re.findall(r"\b\w+\b", typ) if x in ob), None)
            score = (bool(dims), sum(l.count(";") for l in ol[ob[w][0]:ob[w][1]]) if w else 0)
            if n not in borrowed or score > borrowed[n][4]:
                borrowed[n] = (typ, ptr, dims, other, score)
    plain = Types(pre)
    ok, copied = {}, []  # copied: typedefs taken from the other file, in order
    for n, (typ, ptr, dims, other, _) in borrowed.items():
        if n in plain.vars:
            continue  # a header declares it
        decl = f"\nextern {typ} {ptr}{n}{dims};\n"
        extra = []
        olines = other.read_text().split("\n")
        oblocks = find_typedefs(olines)
        todo = [w for w in re.findall(r"\b\w+\b", typ) if w in oblocks and w not in plain.typedefs]
        seen = set()
        while todo:
            w = todo.pop()
            if w in seen or w in plain.typedefs:
                continue
            seen.add(w)
            s, e = oblocks[w]
            body = "\n".join(olines[s:e])
            extra.insert(0, body)
            todo += [x for x in re.findall(r"\b\w+\b", body) if x in oblocks and x != w]
        extra = [b for b in extra if b not in copied]
        try:
            c_parser.CParser().parse(pre + "\n" + uncomment("\n".join(copied + extra)) + decl)
        except Exception:
            continue
        ok[n] = (typ, ptr, dims)
        if extra and os.environ.get("DATA_C_DEBUG"):
            print(f"{n}: {typ} from {other}, copying {len(extra)} typedefs", file=sys.stderr)
        copied += extra
    pre = pre + "\n" + uncomment("\n".join(copied))
    types = Types(pre + "".join(f"\nextern {typ} {ptr}{n}{dims};" for n, (typ, ptr, dims) in ok.items()))
    decls = {}
    gen = c_generator.CGenerator()
    for n in names:
        if n in local and n in types.vars:
            _, typ, ptr, dims, _ = local[n][-1]
            decls[n] = (typ, ptr, dims, types.vars[n])
        elif n in ok:
            decls[n] = ok[n] + (types.vars[n],)
        elif n in types.vars and not isinstance(types.vars[n], c_ast.FuncDecl):
            # declared by a header: its spelling there
            node = next(d for d in types.ast.ext if isinstance(d, c_ast.Decl) and d.name == n)
            text = gen.visit(c_ast.Decl(n, [], [], [], [], node.type, None, None))
            before, after = re.split(r"\b%s\b" % n, text, maxsplit=1)
            decls[n] = (before.rstrip(), "", after.replace(" ", ""), types.vars[n])
    entries, inner = plan(labels, syms, blob, types, decls, asm.read_text(), used)
    # --literals: a string only this block points to, once, was a literal in
    # the initializer: IDO emits it into .rodata where the definition is
    literals, literal_line = {}, {}
    if args.literals or args.literals_in:
        counts = collections.Counter(syms.values())
        where = {s: o for o, s in syms.items()}
        spans = []
        for off, name, _, node, count in entries:
            if name in (args.literals_in or "").split(","):
                size = types.size(node) if count is None else count * types.size(types.resolve(node).type)
                spans.append((off, off + size))
        for i, l in enumerate(src.split("\n")):
            m = re.match(r'^const char (\w+)\[\] = ("(?:[^"\\]|\\.)*");\s*$', l)
            if m and counts[m.group(1)] == 1 and uses[m.group(1)] == 1 \
                    and len(re.findall(r"\b%s\b" % m.group(1), src)) == 1 \
                    and (not spans or any(a <= where[m.group(1)] < b for a, b in spans)):
                literals[m.group(1)] = m.group(2)
                literal_line[m.group(1)] = i
    lines, need, ptypes, used_literals = render(entries, types, blob, syms, vram + start, labels, args.version,
                                                literals)
    defined = {e[1] for e in entries if e[1]}
    undeclared = sorted(n for n in need if n not in types.vars and n not in defined)
    header = f"/* .data, 0x{vram + start:08X}-0x{vram + end:08X} (tools/data_c.py) */"

    out = src.split("\n")
    # after the .bss block if there is one, else before the first function
    at = None
    for n, l in enumerate(out):
        if "(tools/bss_c.py) */" in l:
            at = n + 1
            while at < len(out) and out[at].strip():
                at += 1
            break
    blank_after = at is None
    if at is None:
        at = next((n for n, l in enumerate(out)
                   if re.match(r"^\w[\w\s\*]*\([^;]*$", l) or "GLOBAL_ASM" in l or l.startswith('#include "src/')),
                  len(out))
        while at > 0 and (out[at - 1].startswith("/*") or out[at - 1].startswith(" *")):
            at -= 1

    # what the initializers point to must be declared before them
    externs = []
    for n in undeclared:
        externs.append(f"void {n}();" if n.startswith("func_") else f"extern {ptypes.get(n, 'u8')} {n}[];")
    depth, first = 0, {}
    for i, l in enumerate(out):
        if depth == 0 and not l.startswith(("#", "/*", " *", "//")) and re.match(r"^[A-Za-z_]", l):
            for n in set(re.findall(r"\b\w+\b", l)) & need:
                first.setdefault(n, (i, l))
        depth += l.count("{") - l.count("}")
    for n in sorted(need - defined - set(undeclared)):
        if n in first and first[n][0] >= at:
            d = re.split(r"\s*[=;]", first[n][1])[0].strip()
            d = re.sub(r"^(extern|static)\s+", "", d)
            if "(" in d:
                d = d.rstrip("{ ").rstrip()
                externs.append(f"{d};")
            else:
                externs.append(f"extern {d};")
    text = [l for b in copied for l in b.split("\n") + [""]] + externs + [header] + lines
    if args.dry_run:
        print("\n".join(text))
        if inner:
            print("left to the linker:", " ".join(inner))
        return

    # drop the file-scope extern declarations of what's now defined, and the
    # strings that are literals now
    drop = {n for name, v in local.items() if name in defined for n, *_ in v}
    if used_literals and min(literal_line[n] for n in used_literals) > at:
        # the definitions go where the strings were, so .rodata keeps its
        # order; the code above them keeps its extern declarations
        at = min(literal_line[n] for n in used_literals)
        drop = {n for n in drop if n > at}
        blank_after = True
    drop |= {literal_line[n] for n in used_literals}
    text = text + [""] if blank_after else [""] + text
    MARK = object()
    lines_ = [None if n in drop else l for n, l in enumerate(out)]
    lines_.insert(at, MARK)

    # typedefs the definitions need that the file only has further down
    # move up above the .bss block, together with what they need in turn
    typedef_blocks = lambda: find_typedefs(lines_)

    def top():
        i = lines_.index(MARK)
        while i > 0 and isinstance(lines_[i - 1], str) and lines_[i - 1].strip() \
                and "(tools/bss_c.py) */" not in lines_[i - 1]:
            i -= 1
        if i > 0 and isinstance(lines_[i - 1], str) and "(tools/bss_c.py) */" in lines_[i - 1]:
            i -= 1
        return i

    want = set(re.findall(r"\b\w+\b", "\n".join(text)))
    while True:
        blocks = typedef_blocks()
        tp = top()
        late = [(s, e) for n, (s, e) in blocks.items() if n in want and s >= tp]
        if not late:
            break
        s, e = min(late)
        body = lines_[s:e]
        del lines_[s:e]
        lines_[tp:tp] = body + [""]
        want |= set(re.findall(r"\b\w+\b", "\n".join(l for l in body if isinstance(l, str))))

    new = []
    for l in lines_:
        if l is MARK:
            new += text
        elif l is not None:
            new.append(l)
    text_out = "\n".join(new)
    if used_literals:  # the strings' lines went: no runs of blank lines where they were
        text_out = re.sub(r"\n{3,}", "\n\n", text_out)
    c_path.write_text(text_out)
    print(f"{c_path}: {len(lines)} definitions" + (f"; left to the linker: {' '.join(inner)}" if inner else ""))


if __name__ == "__main__":
    main()
