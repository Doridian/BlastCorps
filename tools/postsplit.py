"""What tools/split.py does after splat has run, so that nothing in the
module is left at an absolute address:

- symbolize_pairs(): lui/addiu and lui/load pairs that splat left as numbers
  (scheduling split them, they are 64-bit loads, or Rare's asm uses
  add/addi) get %hi/%lo of a symbol when they point into any module, or at
  a ROM segment's start.
- pointer_bins(): a data island inside .text that holds pointers becomes a
  .s with those words as .word <symbol>.
- write_bss(): one .bss .s file per object from the config's `bss:` list
  (tools/modmap.py), a label at every address anything refers to.  A C
  file's block is written to asm/bss/ as a reference for tools/bss_c.py.
- patch_ld(): the linker script gets a NOLOAD .bss section with those objects
  in link order, and sections that follow each other are placed after one
  another rather than at fixed addresses, with room for SHIFT_PAD bytes of
  padding at the start of .text and .data (see the Makefile's SHIFT).
- rewrite_undefined(): undefined_syms_auto loses what the .bss files now
  define and gains the symbols used here; whatever lies in a module moves to
  module_syms_auto (tools/link_syms.py resolves both).
"""
import os
import re
from pathlib import Path

from util import options, symbols

import modmap
import vermap

# set by tools/split.py: the version's map to us.v11's names
VERMAP = vermap.VerMap("us.v11")

LINE_RE = re.compile(r"^(/\* [0-9A-F]+ ([0-9A-F]{8}) [0-9A-F]{8} \*/\s+)(\S+)(\s*)(.*)$")
NUM = r"-?(?:0x[0-9A-Fa-f]+|\d+)"
MEM_RE = re.compile(rf"^(\$\w+), ({NUM})?\((\$\w+)\)$")
IMM_RE = re.compile(rf"^(\$\w+), (\$\w+), ({NUM})$")
LUI_RE = re.compile(r"^(\$\w+), (0x[0-9A-Fa-f]+|\d+)$")
MEM_OPS = {"lb", "lbu", "lh", "lhu", "lw", "lwu", "ld", "sb", "sh", "sw", "sd",
           "lwc1", "swc1", "ldc1", "sdc1", "lwl", "lwr", "swl", "swr", "ll", "sc"}
STORES = {"sb", "sh", "sw", "sd", "swc1", "sdc1", "swl", "swr", "sc"}
LIKELY = {"beql", "bnel", "blezl", "bgtzl", "bltzl", "bgezl", "bltzall", "bgezall", "bc1fl", "bc1tl"}
NO_DEST = re.compile(r"^(s[bhwd]|swc1|sdc1|swl|swr|b\w*|j|jr|jal|jalr|mt\w+|ctc1|nop|syscall|break|cache|sync|eret|c\.\w+\.\w+)$")


def version_of(cfg):
    return cfg["options"]["basename"].split(".", 1)[1]


def rom_starts(ver):
    """{ROM offset: name} of the named top-level segments (tools/rom_syms.py
    gives them linker symbols)."""
    path = Path(options.get_base_path()) / ".." / f"blastcorps.{ver}.yaml"
    out = {}
    if not path.exists():
        return out
    import yaml
    for seg in yaml.safe_load(path.read_text())["segments"]:
        if isinstance(seg, list) and len(seg) >= 3:
            out.setdefault(seg[0], seg[2])
        elif isinstance(seg, dict) and "start" in seg and "name" in seg:
            out.setdefault(seg["start"], seg["name"])
    return out


class Symbols:
    """splat's symbols by address, plus the ones made here."""

    def __init__(self, hand=None, mods=None):
        self.movable = (lambda a: movable(mods, a)) if mods else (lambda a: False)
        self.by_addr = {}
        for s in symbols.all_symbols:
            cur = self.by_addr.get(s.vram_start)
            if cur is None or (s.given_name and not cur.given_name):
                self.by_addr[s.vram_start] = s
        self.new = {}  # addr -> name
        self.used = {}  # name -> addr
        # names only the C uses, from undefined_syms.<module>.<version>.txt
        self.hand = {}
        if hand and Path(hand).exists():
            for m in re.finditer(r"^\s*(\w+)\s*=\s*0x([0-9A-Fa-f]+);", Path(hand).read_text(), re.M):
                self.hand.setdefault(int(m.group(2), 16), m.group(1))

    def name(self, addr, make=True):
        """The name for addr, made up if there is none.  A name used here
        that nothing defines is listed in undefined() for the link."""
        s = self.by_addr.get(addr)
        if s is not None:
            if not s.defined:
                self.used[s.name] = addr
            return s.name
        if addr in self.hand:
            return self.hand[addr]
        if addr in self.new or make:
            return self.new.setdefault(addr, VERMAP.name("D", addr, addr < 0x01000000 or self.movable(addr)))
        return None

    def undefined(self):
        """{name: addr} of the names this made or used that nothing defines."""
        out = {n: a for a, n in self.new.items()}
        out.update(self.used)
        return out

    def names(self):
        """{addr: [names]}: every name anything uses for the address."""
        out = {}
        for s in symbols.all_symbols:
            out.setdefault(s.vram_start, set()).add(s.name)
        for a, n in [*self.hand.items(), *self.new.items()]:
            out.setdefault(a, set()).add(n)
        return {a: sorted(v, key=lambda n: (n.startswith("D_"), n)) for a, v in out.items()}


def movable(mods, addr):
    """True if addr is in a module and not a module's load address (those
    are the fixed memory map: init inflates hd_code to 0x802447C0 whatever
    its first function is)."""
    m = modmap.owner(mods, addr)
    return m is not None and addr != m.vram


SAVED = {"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$gp", "$fp", "$sp"}


def symbolize_pairs(paths, mods, roms, syms):
    """Rewrite bare lui pairs in the given .s files.  Returns how many lo
    halves were rewritten, and the lui instructions left that could still
    hold the high half of a module address (printed, for a look by hand).

    A register's lui is followed through labels (a lui before a branch is
    still there where the paths join) and calls (for the registers a call
    preserves), and forgotten at a function's start and after a jump."""
    changed = 0
    leftover = []
    for path in paths:
        lines = path.read_text().split("\n")
        edits = {}  # line index -> new text
        hi = {}  # reg -> (value << 16, lui line index)
        likely = False
        after = None  # what the instruction in a delay slot is followed by
        for n, line in enumerate(lines):
            s = line.strip()
            if s.startswith("glabel ") and not re.match(r"glabel L[0-9A-F]{8}_", s) or s.startswith(".section") \
                    or s in (".text", ".data", ".rdata", ".late_rodata"):
                hi = {}
                after = None
                continue
            m = LINE_RE.match(line)
            if not m:
                continue
            pre, vram, op, sp, args = m.groups()
            in_likely, likely = likely, op in LIKELY
            then, after = after, None
            if op in ("jal", "jalr"):
                after = "call"
            elif op in ("j", "jr", "b"):
                after = "jump"
            regs = re.findall(r"\$\w+", args)
            dest = regs[0] if regs and not NO_DEST.match(op) else None
            if op == "lui" and "%" not in args:
                mm = LUI_RE.match(args)
                if mm and not in_likely:
                    hi[mm.group(1)] = (int(mm.group(2), 0) << 16, n)
                dest = None
            elif "%" not in args:
                addr = base = None
                mm = MEM_RE.match(args) if op in MEM_OPS else None
                if mm and mm.group(3) in hi:
                    base, imm = mm.group(3), int(mm.group(2) or "0", 0)
                    addr = (hi[base][0] + imm) & 0xFFFFFFFF
                mi = IMM_RE.match(args) if op in ("addiu", "addi", "daddiu", "ori") else None
                if mi and mi.group(2) in hi:
                    base, imm = mi.group(2), int(mi.group(3), 0)
                    v = hi[base][0]
                    addr = (v | imm) if op == "ori" else (v + imm) & 0xFFFFFFFF
                    if op == "ori" and imm >= 0x8000:
                        addr = None  # %lo with ori only works below 0x8000
                # a ROM segment's start, above the small numbers (but the
                # texture table is at 0x4CE0: 5BF40's `lui 0; addiu 0x4ce0`)
                if addr is not None and (movable(mods, addr) or (addr in roms and (
                        addr >= 0x10000 or roms[addr] == "texture_table"))):
                    name = syms.name(addr)
                    lui_n = hi[base][1]
                    if lui_n not in edits:
                        lm = LINE_RE.match(lines[lui_n])
                        reg = LUI_RE.match(lm.group(5)).group(1)
                        edits[lui_n] = f"{lm.group(1)}{lm.group(3)}{lm.group(4)}{reg}, %hi({name})"
                    if mm:
                        new = f"{mm.group(1)}, %lo({name})({mm.group(3)})"
                    else:
                        new = f"{mi.group(1)}, {mi.group(2)}, %lo({name})"
                    edits[n] = f"{pre}{op}{sp}{new}"
                    changed += 1
            # what this instruction does to the tracked registers
            if dest and not in_likely and op != "lui":
                srcs = [r for r in regs[1:] if r in hi]
                if op in ("addu", "add", "daddu", "dadd") and len(srcs) == 1:
                    hi[dest] = hi[srcs[0]]  # base + index
                else:
                    hi.pop(dest, None)
            if then == "call":
                hi = {r: v for r, v in hi.items() if r in SAVED}
            elif then == "jump":
                hi = {}
        for n, text in edits.items():
            lines[n] = text
        if edits:
            path.write_text("\n".join(lines))
        for line in lines:
            m = LINE_RE.match(line)
            if m and m.group(3) == "lui":
                mm = LUI_RE.match(m.group(5))
                if mm:
                    v = int(mm.group(2), 0) << 16
                    if any(movable(mods, v + k) for k in (-0x8000, 0, 0x7FFF)):
                        leftover.append(f"{path}: {line.strip()}")
    return changed, leftover


def bss_symbols(mod, syms):
    """{addr: [names]} of every .bss symbol, with one at each block start."""
    names = {a: n for a, n in syms.names().items() if mod.bss_start <= a < mod.bss_end}
    for start, _, _ in mod.bss:
        if start not in names:
            names[start] = [syms.name(start)]
    return names


def write_bss(mod, syms):
    """Write each object's .bss.  Returns the names now defined by asm."""
    names = bss_symbols(mod, syms)
    defined = set()
    for k, (start, kind, obj) in enumerate(mod.bss):
        end = mod.bss[k + 1][0] if k + 1 < len(mod.bss) else mod.bss_end
        addrs = sorted(a for a in names if start <= a < end)
        out = ['.include "macro.inc"', "", ".section .bss"]
        for a, b in zip(addrs, addrs[1:] + [end]):
            out += [""] + [f"dlabel {n}" for n in names[a]] + [f".space 0x{b - a:X}"]
        # C files' blocks are written too, as a reference for tools/bss_c.py,
        # but not where the build picks up asm.
        sub = "data" if kind == "bss" else "bss"
        path = Path(options.get_asm_path()) / sub / f"{obj}.bss.s"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(out) + "\n")
        if kind == "bss":
            defined |= {n for a in addrs for n in names[a]}
    return defined


def pad_line(section):
    """Padding at the start of an output section: SHIFT_PAD_<section> if
    it's defined, else SHIFT_PAD for .text and .data (--defsym, see the
    Makefile's SHIFT), else none."""
    default = "0" if section.endswith("_bss") else "(DEFINED(SHIFT_PAD) ? SHIFT_PAD : 0)"
    return f"        . += DEFINED(SHIFT_PAD_{section}) ? SHIFT_PAD_{section} : {default};"


def patch_ld(mod, ld_path, ends, replace=None):
    """Chain the module's sections, add SHIFT_PAD and the .bss section.
    ends: names of symbols exactly at the end of .bss; replace: objects to
    swap (pointer_bins)."""
    out = []
    replace = replace or {}
    shift = mod.name != "init"
    prev = None  # (section, its ROM_START symbol)
    closed = None
    src = options.get_src_path()
    for line in Path(ld_path).read_text().split("\n"):
        if re.match(r"^\s*\w+_BSS_(START|END) = \.;$", line):
            continue
        if line.strip() in replace:
            line = line.replace(line.strip(), replace[line.strip()])
        m = re.match(r"^    \.(\w+) (0x[0-9A-Fa-f]+) : AT\((\w+)\)(.*)$", line)
        if m:
            name, addr, rom, rest = m.groups()
            if prev is not None:
                line = f"    .{name} ADDR(.{prev[0]}) + SIZEOF(.{prev[0]}) : AT({rom}){rest}"
            prev = (name, rom)
            out.append(line)
            continue
        if line == "    {" and shift and prev is not None and out[-1].startswith(f"    .{prev[0]} "):
            out += [line, pad_line(prev[0])]
            continue
        if line == "    }" and prev is not None:
            closed = prev
        elif closed is not None:
            m = re.match(r"^    (__romPos|\w+_ROM_END) = 0x[0-9A-Fa-f]+;$", line)
            if m:
                line = f"    {m.group(1)} = {closed[1]} + SIZEOF(.{closed[0]});"
            elif line.strip():
                closed = None
        if line == "    /DISCARD/ :" and mod.bss:
            last = prev[0]
            out += [f"    .{mod.name}_bss ADDR(.{last}) + SIZEOF(.{last}) (NOLOAD) : SUBALIGN(4)", "    {"]
            if shift:
                out.append(pad_line(f"{mod.name}_bss"))
            out.append(f"        {mod.name}_BSS_START = .;")
            for start, kind, obj in mod.bss:
                if kind == "bss":
                    out.append(f"        build/asm/data/{obj}.bss.s.o(.bss);")
                else:
                    out.append(f"        build/{src}/{obj}.c.o(.bss);")
            out.append(f"        {mod.name}_BSS_END = .;")
            out += [f"        {n} = .;" for n in ends]
            out += ["    }", ""]
        out.append(line)
    Path(ld_path).write_text("\n".join(out))


def pointer_bins(mod, mods, syms, blob):
    """The data islands inside .text (`bin`s) that hold pointers get a .s
    in place of the bin, with those words as .word <symbol>.  Returns
    {bin object: .s object} for the linker script."""
    subs = [x for x in mod.subsegments if isinstance(x, list)]
    text_end = next((x[0] for x in subs if len(x) >= 2 and x[1] == "linker"), mod.size)
    out = {}
    for k, x in enumerate(subs):
        if len(x) < 3 or x[1] != "bin" or x[0] >= text_end or k + 1 >= len(subs):
            continue
        start, end = x[0], subs[k + 1][0]
        data = blob[start:end]
        vram = mod.vram + start
        words = {}
        for i in range(-vram % 4, len(data) - 3, 4):
            w = int.from_bytes(data[i:i + 4], "big")
            if movable(mods, w):
                words[i] = syms.name(w)
        if not words:
            continue
        lines, run, i = ['.include "macro.inc"', "", ".section .data", ""], [], 0
        while i < len(data):
            if i in words:
                if run:
                    lines.append(".byte " + ", ".join(run))
                    run = []
                lines.append(f".word {words[i]}")
                i += 4
            else:
                run.append(f"0x{data[i]:02X}")
                if len(run) == 16:
                    lines.append(".byte " + ", ".join(run))
                    run = []
                i += 1
        if run:
            lines.append(".byte " + ", ".join(run))
        path = Path(options.get_asm_path()) / "data" / f"{x[2]}.bin.s"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(lines) + "\n")
        out[f"build/assets/{x[2]}.bin.o(.data);"] = f"build/asm/data/{x[2]}.bin.s.o(.data);"
        print(f"postsplit: {x[2]}: a bin with {len(words)} pointers, now {path}")
    return out


def rewrite_undefined(path, mods, drop, add):
    """Rewrite splat's undefined_syms_auto: without what the .bss files now
    define, with the symbols made here, and with everything that lies in a
    module moved to module_syms_auto (tools/link_syms.py resolves those
    against the module's own symbols or another module's link).  What stays
    is outside every module: hardware, the boot globals, segment addresses,
    the fixed memory map, ROM offsets."""
    path = Path(path)
    entries = {}
    for line in path.read_text().splitlines() if path.exists() else []:
        m = re.match(r"^(\S+) = (0x[0-9A-Fa-f]+);", line)
        if m and m.group(1) not in drop:
            entries.setdefault(m.group(1), int(m.group(2), 16))
    for name, addr in add.items():
        if name not in drop:
            entries.setdefault(name, addr)
    ext = [(n, a) for n, a in entries.items() if not movable(mods, a)]
    local = [(n, a) for n, a in entries.items() if movable(mods, a)]
    path.write_text("".join(f"{n} = 0x{a:X};\n" for n, a in ext))
    other = path.with_name(path.name.replace("undefined_syms_auto", "module_syms_auto"))
    other.write_text("".join(f"{n} = 0x{a:X};\n" for n, a in sorted(local, key=lambda x: x[1])))


def asm_uses(asm, mod, mods, skip):
    """{name: addr} of the module's symbols the asm uses and no asm defines:
    fields inside a C file's data, which splat doesn't list (a version's
    GLOBAL_ASM reaching into the C's .data)."""
    text = "\n".join(p.read_text() for p in sorted(asm.rglob("*.s")))
    defined = set(re.findall(r"^\s*(?:glabel|dlabel)\s+(\w+)", text, re.M))
    used = set(re.findall(r"%(?:hi|lo)\((\w+)\)", text)) | set(re.findall(r"\.word\s+(\w+)", text))
    addr = {}
    for s in symbols.all_symbols:
        addr.setdefault(s.name, s.vram_start)
    out = {}
    for n in sorted(used - defined - skip):
        a = addr.get(n)
        if a is not None and mod.contains(a) and movable(mods, a):
            out[n] = a
    return out


def run(config_path, cfg):
    ver = version_of(cfg)
    base = Path(options.get_base_path())
    mods = modmap.modules(ver, base)
    mod = modmap.Module(config_path)
    mods[mod.name] = mod
    syms = Symbols(base / f"undefined_syms.{mod.name}.{ver}.txt", mods)
    asm = Path(options.get_asm_path())
    paths = sorted((asm / mod.name).glob("*.s")) + sorted((asm / "nonmatchings" / mod.name).rglob("*.s"))
    changed, leftover = symbolize_pairs(paths, mods, rom_starts(ver), syms)
    print(f"postsplit: {changed} lui pairs symbolized in {mod.name}; {len(leftover)} bare lui left"
          " that could be module addresses (POSTSPLIT_VERBOSE=1 lists them; tools/find_abs.py checks a link)")
    if os.environ.get("POSTSPLIT_VERBOSE"):
        for line in leftover:
            print(f"postsplit: bare lui left: {line}")
    replace = pointer_bins(mod, mods, syms, Path(options.get_target_path()).read_bytes())
    defined = write_bss(mod, syms) if mod.bss else set()
    ends = sorted(n for a, ns in syms.names().items() if mod.bss and a == mod.bss_end for n in ns)
    patch_ld(mod, options.get_ld_script_path(), ends, replace)
    add = {n: a for n, a in syms.undefined().items() if n not in defined and n not in ends}
    add.update(asm_uses(asm, mod, mods, defined | set(ends) | set(add)))
    rewrite_undefined(options.get_undefined_syms_auto_path(), mods, defined | set(ends), add)
