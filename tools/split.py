#!/usr/bin/env python3
"""Run splat (tools/splat/split.py) with local fixes to its data output.

The submodule's splat is old; these patch it rather than fork it:

- Data symbols are labelled with `dlabel`, not `glabel`: glabel aligns to 4,
  which pads after any symbol at an odd address.
- Strings are escaped byte by byte.  splat writes a NUL as "\\0", so a NUL
  followed by a digit becomes a different octal escape, and it writes
  quotes, backslashes and non-ASCII (EUC-JP) bytes unescaped.
- A float like 1e8 came out as "1e8.0", which gas rejects.
- A word in one data subsegment that points into another becomes a symbol
  reference.  splat only symbolizes pointers within the same subsegment, so
  once .data/.rodata are split per object, pointers between objects (a
  table in one object's .data listing another's strings, say) would stay
  bare constants and the build would not be shiftable.  So does a word that
  points anywhere else in a module (this one's .text or .bss, or another
  module): tools/link_syms.py resolves those at link time.
- Aligned data that splat would print as bytes or halfwords (because code
  reads it with lbu or lh) is printed as words when it holds a pointer to a
  symbol, so the pointer is a relocation too.
- Outside us.v11, splat's default names are us.v11's for the same thing
  (tools/vermap.py): `D_X` is what us.v11 has at X, and something us.v11
  doesn't have keeps its own address with the version appended.
- After splat: tools/postsplit.py symbolizes the lui pairs splat left as
  numbers, writes the .bss of each object and adds it to the linker script.
- A `.rodata` subsegment (owned by the C file of the same name) is handed to
  that file's GLOBAL_ASM functions: each symbol goes into the .s of the first
  function that references it, as `.late_rodata` (jump tables, floats and
  doubles, which IDO emits after the rest of an object's .rodata) or
  `.rdata` (strings and other constants).  asm-processor then puts them where
  IDO would, so a C file can own its .rodata while some of its functions are
  still asm.  Symbols no GLOBAL_ASM function references are left to the C.
"""
import os
import re
import runpy
import sys
from pathlib import Path

SPLAT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "splat")
sys.path.insert(0, SPLAT)

from util import floats  # noqa: E402
from segtypes.common.code import CommonSegCode  # noqa: E402
from segtypes.common.codesubsegment import CommonSegCodeSubsegment as Sub  # noqa: E402
from segtypes.common.data import CommonSegData  # noqa: E402
from segtypes.common.group import CommonSegGroup  # noqa: E402

_format_f32_imm = floats.format_f32_imm


def format_f32_imm(num: int) -> str:
    ret = _format_f32_imm(num)
    if "e" in ret and ret.endswith(".0"):
        ret = ret[:-2]
    return ret


floats.format_f32_imm = format_f32_imm


_DATA_TYPES = ("data", "rodata", ".data", ".rodata")
_group_scan = CommonSegGroup.scan


def group_scan(self, rom_bytes):
    if isinstance(self, CommonSegCode):
        datas = [sub for sub in self.subsegments if sub.type in _DATA_TYPES]
        for sub in datas:
            for i in range(sub.rom_start, sub.rom_end - 3, 4):
                bits = int.from_bytes(rom_bytes[i : i + 4], "big")
                owner = next((d for d in datas if d.contains_vram(bits)), None)
                if owner is not None and owner.type.startswith("."):
                    # in a C file's data: the C defines it, or (a field) the
                    # link makes it relative, so it goes on the undefined list
                    self.get_symbol(bits, create=True, reference=True, local_only=True)
                elif owner is not None:
                    self.get_symbol(bits, create=True, define=True, local_only=True)
                elif sub.type in ("data", "rodata") and postsplit.movable(MODULES, bits):
                    self.get_symbol(bits, create=True, reference=True)
    _group_scan(self, rom_bytes)


CommonSegGroup.scan = group_scan


_disassemble_symbol = CommonSegData.disassemble_symbol

_ESCAPES = {0x0A: "\\n", 0x22: '\\"', 0x5C: "\\\\"}


def disassemble_symbol(self, sym_bytes, sym_type):
    if sym_type != "ascii":
        return _disassemble_symbol(self, sym_bytes, sym_type)
    out = []
    for i, b in enumerate(sym_bytes):
        if b == 0 and not (i + 1 < len(sym_bytes) and 0x30 <= sym_bytes[i + 1] <= 0x37):
            out.append("\\0")
        elif b in _ESCAPES:
            out.append(_ESCAPES[b])
        elif 0x20 <= b < 0x7F:
            out.append(chr(b))
        else:
            out.append(f"\\{b:03o}")
    return '.ascii "' + "".join(out) + '"'


CommonSegData.disassemble_symbol = disassemble_symbol


def holds_pointer(self, sym_bytes):
    for i in range(0, len(sym_bytes) - 3, 4):
        bits = int.from_bytes(sym_bytes[i : i + 4], "big")
        if bits >= 0x80000000 and self.get_most_parent().get_symbol(bits) is not None:
            return True
    return False


def with_pointers(self, sym_bytes, vram):
    """Bytes that splat would print as .byte/.short, with the aligned words
    among them that point at a symbol printed as .word (a pointer in a table
    of odd-sized records).  None if there are none."""
    parent = self.get_most_parent()
    words = {}
    for i in range(-vram % 4, len(sym_bytes) - 3, 4):
        bits = int.from_bytes(sym_bytes[i : i + 4], "big")
        sym = parent.get_symbol(bits, reference=True) if bits >= 0x80000000 else None
        if sym is not None:
            words[i] = sym.name
    if not words:
        return None
    out, run, i = [], [], 0
    while i < len(sym_bytes):
        if i in words:
            if run:
                out.append(".byte " + ", ".join(run))
                run = []
            out.append(f".word {words[i]}")
            i += 4
        else:
            run.append(f"0x{sym_bytes[i]:02X}")
            i += 1
    if run:
        out.append(".byte " + ", ".join(run))
    return "\n".join(out)


def disassemble_data(self, rom_bytes):
    """splat's CommonSegData.disassemble_data, with dlabel and pointer words."""
    rodata_encountered = "rodata" in self.type
    ret = '.include "macro.inc"\n\n'
    ret += f".section {self.get_linker_section()}"

    if self.size == 0:
        return None

    syms = self.get_symbols(rom_bytes)
    parent = self.get_most_parent()

    for i in range(len(syms) - 1):
        mnemonic = syms[i].access_mnemonic
        sym = parent.get_symbol(syms[i].vram_start, create=True, define=True, local_only=True)

        sym_str = f"\n\ndlabel {sym.name}\n"
        dis_start = parent.ram_to_rom(syms[i].vram_start)
        dis_end = parent.ram_to_rom(syms[i + 1].vram_start)
        sym_len = dis_end - dis_start

        if self.type == "bss":
            ret += f".space 0x{sym_len:X}"
            continue

        sym_bytes = rom_bytes[dis_start:dis_end]
        aligned = sym_len % 4 == 0 and sym.vram_start % 4 == 0

        if self.is_valid_ascii(sym_bytes) and mnemonic == "addiu":
            stype = "ascii"
        elif syms[i].type == "jtbl":
            stype = "jtbl"
        elif sym_len % 8 == 0 and mnemonic in Sub.double_mnemonics:
            stype = "double"
        elif sym_len % 4 == 0 and mnemonic in Sub.float_mnemonics:
            stype = "float"
        elif aligned and (mnemonic in Sub.word_mnemonics or not mnemonic):
            stype = "word"
        elif aligned and holds_pointer(self, sym_bytes):
            stype = "word"
        elif sym_len % 2 == 0 and sym.vram_start % 2 == 0 and (mnemonic in Sub.short_mnemonics or not mnemonic):
            stype = "short"
        else:
            stype = "byte"

        # If we're starting from a weird place, make sure our container size is correct
        if dis_start % 4 != 0 and stype != "byte" and sym_len > 1:
            stype = "short"
        if dis_start % 2 != 0:
            stype = "byte"

        if not rodata_encountered and mnemonic == "jtbl":
            rodata_encountered = True
            ret += "\n\n\n.section .rodata"

        mixed = with_pointers(self, sym_bytes, sym.vram_start) if stype in ("byte", "short") else None
        sym_str += mixed if mixed else self.disassemble_symbol(sym_bytes, stype)
        sym.disasm_str = sym_str
        ret += sym_str

    ret += "\n"
    return ret


CommonSegData.disassemble_data = disassemble_data

from util import options  # noqa: E402

_LATE_MNEMONICS = ("lwc1", "ldc1", "swc1", "sdc1")
_data_split = CommonSegData.split


def migrate_rodata(self):
    """Distribute this .rodata subsegment's symbols over the GLOBAL_ASM .s
    files of its C file."""
    if not self.file_text or not self.sibling:
        return
    asm_dir = options.get_asm_path() / "nonmatchings" / self.dir / self.name
    funcs = []  # (vram, path, {sym: mnemonic})
    for path in sorted(asm_dir.glob("*.s")) if asm_dir.exists() else []:
        text = path.read_text()
        m = re.search(r"^/\* [0-9A-F]+ ([0-9A-F]{8}) ", text, re.M)
        if not m:
            continue
        refs = {}
        for line in text.splitlines():
            for sym in re.findall(r"%lo\((\w+)\)", line):
                refs.setdefault(sym, line.split("*/")[-1].split()[0])
        funcs.append((int(m.group(1), 16), path, refs))
    funcs.sort()

    blocks = re.findall(r"\n\ndlabel (\w+)\n(.*)", self.file_text)
    moved = {}  # path -> {"rdata": [...], "late": [...], "first": vram}
    for i, (name, body) in enumerate(blocks):
        user = next((f for f in funcs if name in f[2]), None)
        if user is None:
            continue
        vram, path, refs = user
        late = name.startswith("jtbl_") or refs[name] in _LATE_MNEMONICS
        if i == len(blocks) - 1:
            # The last symbol carries the padding to the next object; IDO
            # pads the section itself.
            while body.endswith(", 0") or body.endswith(", 0x00000000") or body.endswith(", 0.0"):
                body = body.rsplit(", ", 1)[0]
        ent = moved.setdefault(path, {"rdata": [], "late": [], "align": None})
        sect = "late" if late else "rdata"
        if late and ent["align"] is None:
            addr = VERMAP.original(name)
            if addr is None:
                addr = next(s.vram_start for ss in self.get_most_parent().seg_symbols.values()
                            for s in ss if s.name == name)
            ent["align"] = 8 if addr % 8 == 0 else 4
        ent[sect].append(f"glabel {name}\n{body}\n")

    for path, ent in moved.items():
        head = []
        if ent["rdata"]:
            head += [".rdata"] + ent["rdata"]
        if ent["late"]:
            head += [".late_rodata", f".late_rodata_alignment {ent['align']}"] + ent["late"]
        head += [".text"]
        path.write_text("\n".join(head) + "\n" + path.read_text())


# splat's own attempt at this writes every referenced symbol, with its data
# directives unchanged, into one .rodata block; asm-processor rejects that.
CommonSegCode.check_rodata_sym = lambda self, func_addr, sym: None


def data_split(self, rom_bytes):
    _data_split(self, rom_bytes)
    if self.type == ".rodata":
        migrate_rodata(self)


CommonSegData.split = data_split

import yaml  # noqa: E402

import modmap  # noqa: E402
import postsplit  # noqa: E402

CONFIG_PATH = sys.argv[1]
CONFIG = yaml.safe_load(Path(CONFIG_PATH).read_text())
MODULES = modmap.modules(postsplit.version_of(CONFIG), Path(CONFIG_PATH).parent)

# Names: in versions other than us.v11, what splat and postsplit call a
# thing is us.v11's name for it (tools/vermap.py), so that one C source
# builds every version.
import vermap  # noqa: E402
from util import symbols as _symbols  # noqa: E402

VERMAP = vermap.load(postsplit.version_of(CONFIG), Path(CONFIG_PATH).parent)
_default_name = _symbols.Symbol.default_name.fget


def default_name(self):
    if VERMAP.identity or self.in_overlay:
        return _default_name(self)
    prefix = "func" if self.type == "func" else "jtbl" if self.type == "jtbl" else "D"
    return VERMAP.name(prefix, self.vram_start, postsplit.movable(MODULES, self.vram_start))


_symbols.Symbol.default_name = property(default_name)
postsplit.VERMAP = VERMAP

# splat writes undefined_funcs_auto/undefined_syms_auto only when they
# would be non-empty, which leaves a stale one from an earlier split in
# place, or none at all in a fresh clone (init has no undefined functions,
# and the build depends on the file).  Start them empty.
_base = Path(CONFIG_PATH).parent / CONFIG["options"].get("base_path", ".")
for key in ("undefined_funcs_auto_path", "undefined_syms_auto_path"):
    if CONFIG["options"].get(key):
        (_base / CONFIG["options"][key]).write_text("")

sys.argv[0] = os.path.join(SPLAT, "split.py")
runpy.run_path(sys.argv[0], run_name="__main__")
if any(s.get("type") == "code" for s in CONFIG["segments"] if isinstance(s, dict)):
    postsplit.run(CONFIG_PATH, CONFIG)
