#!/usr/bin/env python3
"""Turn a module's absolute symbol files into what the link should use.

Run from blastcorps/ by the Makefile:
  link_syms.py <module> <version> -o <out.ld> [--layout <module>=<elf>] ...
               [--rom <rom.ld>] [--report <file>] <symbol files...>
  link_syms.py --check <module> <version> <elf> <symbol files...>

The symbol files (undefined_syms_auto, undefined_funcs_auto, the
hand-written undefined_syms) give every name an address in the original
ROM.  Here each one becomes:

- nothing, if the module's own objects define it;
- relative to the nearest symbol below it that the module defines, if it
  lies in this module (an address inside a function, a table or a .bss
  variable; a second name for a variable);
- its current address in another module, if it lies in one: the nearest
  symbol below it there, looked up in that module's --layout ELF.  Without
  one (the layout link itself) it keeps its original address;
- the ROM symbol from --rom, if it is where a top-level ROM segment starts
  (or one of the other ROM addresses tools/rom_syms.py names);
- else its original address.  That is the fixed memory map: hardware
  registers, the boot globals at 0x80000300, segment addresses, the
  framebuffers, each module's load address (unless it's a func_ name: the
  function there moves), and anything marked
  `/* fixed */` in the hand-written file.

An original address is known for the names in
symbol_addrs.<module>.<version>.txt and for a name that ends in one
(`D_802E8BD0`, `func_80244930`): in us.v11 that address, in another version
wherever tools/vermap.py puts it (`D_80301234_jp` is jp's own address).

--check compares a linked ELF against the original addresses: in a build
that isn't shifted, every name must be where it was.
"""
import argparse
import bisect
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import modmap  # noqa: E402
import vermap  # noqa: E402

ASSIGN = re.compile(r"^\s*([\w.$]+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;(.*)$")
SUFFIX = re.compile(r"_([0-9A-F]{8})$")


def read_syms(paths):
    """[(name, value, fixed)] in file order; a later file overrides."""
    out = {}
    for path in paths:
        for line in Path(path).read_text().splitlines():
            m = ASSIGN.match(line)
            if m:
                out[m.group(1)] = (int(m.group(2), 0), "fixed" in m.group(3))
    return [(n, v, f) for n, (v, f) in out.items()]


def elf_symbols(path, defined_only=True):
    """{name: value} of the global symbols an ELF or objects define.  With
    defined_only, not the absolute ones: those come from linker scripts."""
    args = ["mips-linux-gnu-nm", "-g", "--defined-only"]
    paths = [path] if isinstance(path, (str, Path)) else list(path)
    out = {}
    for i in range(0, len(paths), 500):
        r = subprocess.run(args + [str(p) for p in paths[i:i + 500]], capture_output=True, text=True)
        for line in r.stdout.splitlines():
            p = line.split()
            if len(p) == 3 and (p[1] not in "aAU" or not defined_only):
                out[p[2]] = int(p[0], 16)
    return out


def symbol_addrs(mod, ver):
    out = {}
    path = Path(f"symbol_addrs.{mod}.{ver}.txt")
    if path.exists():
        for line in path.read_text().splitlines():
            m = re.match(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", line)
            if m:
                out.setdefault(m.group(1), int(m.group(2), 16))
    return out


VERMAP = None  # set in main(): names outside us.v11 are us.v11's (tools/vermap.py)


def original(name, known):
    if name in known:
        return known[name]
    if VERMAP is not None and not VERMAP.identity:
        return VERMAP.original(name)
    m = SUFFIX.search(name)
    if m:
        return int(m.group(1), 16)
    return None


def bin_symbols(mod):
    """{name: original address} of the symbols splat's linker script puts at
    each bin (`<module>_<name>_bin = .;`): a data island has no other."""
    out = {}
    for x in mod.subsegments:
        if isinstance(x, list) and len(x) >= 3 and x[1] == "bin":
            out[f"{mod.name}_{x[2].split('/')[-1]}_bin"] = mod.vram + x[0]
    return out


class Containers:
    """The symbols a module's objects define, by original address, and the
    linker script's symbols at its bins."""

    def __init__(self, mod, names, known, script=False):
        """script: the bin symbols are there even if names (a module's
        objects, before the link) lacks them."""
        pts = {}
        for n, a in bin_symbols(mod).items():
            if script or n in names:
                pts[a] = n
        for n in names:
            a = original(n, known)
            if a is not None and mod.contains(a):
                pts.setdefault(a, n)
        self.addrs = sorted(pts)
        self.names = [pts[a] for a in self.addrs]

    def below(self, addr):
        i = bisect.bisect_right(self.addrs, addr) - 1
        return (self.names[i], self.addrs[i]) if i >= 0 else (None, None)


def ld_objects(ld_path):
    text = Path(ld_path).read_text()
    return sorted(set(re.findall(r"(build/\S+?\.o)\(", text)))


def rom_symbols(path):
    """{original ROM offset: symbol} of the segment starts in --rom (and of
    the few other addresses tools/rom_syms.py names, marked "originally")."""
    out = {}
    if path and Path(path).exists():
        for line in Path(path).read_text().splitlines():
            m = ASSIGN.match(line)
            if m and (m.group(1).endswith("_ROM_START") or "originally" in m.group(3)):
                o = re.search(r"originally (0x[0-9A-Fa-f]+)", m.group(3))
                out.setdefault(int(o.group(1) if o else m.group(2), 0), m.group(1))
    return out


def classify(v, mods):
    if 0xA0000000 <= v < 0xC0000000 or v >= 0xC0000000:
        return "hardware / uncached"
    if 0x80000000 <= v < 0x80000400:
        return "boot globals"
    if v < 0x01000000:
        return "ROM offset"
    if v < 0x80000000:
        return "segment address"
    for m in mods.values():
        if v == m.vram:
            return f"{m.name}'s load address"
    return "fixed memory map (framebuffers, heap, top of RAM)"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("version")
    ap.add_argument("files", nargs="*")
    ap.add_argument("-o", dest="out")
    ap.add_argument("--layout", action="append", default=[], metavar="MODULE=ELF")
    ap.add_argument("--rom")
    ap.add_argument("--report")
    ap.add_argument("--check", metavar="ELF")
    args = ap.parse_args()

    global VERMAP
    VERMAP = vermap.load(args.version, Path("."))
    mods = modmap.modules(args.version)
    mod = mods[args.module]
    ver = args.version
    syms = read_syms(args.files)

    if args.check:
        have = elf_symbols(args.check, defined_only=False)
        # (nm leaves out .L names)
        bad = [f"{n}: 0x{have.get(n, 0):08X}, was 0x{v:08X}" for n, v, _ in syms
               if have.get(n) != v and not n.startswith(".L")]
        for line in bad[:20]:
            print(f"{args.check}: {line}", file=sys.stderr)
        if bad:
            sys.exit(f"{args.check}: {len(bad)} symbols moved in a build that shouldn't move anything")
        return

    known = {m: symbol_addrs(m, ver) for m in mods}
    own = elf_symbols(ld_objects(f"{mod.name}.{ver}.ld"))
    here = Containers(mod, own, known[mod.name], script=True)
    layouts = {}
    for spec in args.layout:
        name, _, elf = spec.partition("=")
        cur = elf_symbols(elf)
        layouts[name] = (Containers(mods[name], cur, known[name]), cur)
    roms = rom_symbols(args.rom)

    out = [f"/* generated by tools/link_syms.py for {mod.name}.{ver}; see there */"]
    report = {}
    for name, v, fixed in syms:
        if name in own:
            continue
        owner = modmap.owner(mods, v)
        # a module's load address is fixed, unless it names the function there
        base = owner is not None and v == owner.vram and not name.startswith("func_")
        if fixed or owner is None or base:
            if v in roms and not fixed:
                out.append(f"{name} = {roms[v]};")
                report.setdefault("ROM segment (linker symbol)", []).append(name)
                continue
            out.append(f"{name} = 0x{v:X};")
            report.setdefault(classify(v, mods) + (" (marked fixed)" if fixed else ""), []).append(name)
        elif owner is mod:
            if mod.bss and v == mod.bss_end:
                out.append(f"{name} = {mod.name}_BSS_END;")
                report.setdefault("relative (own module)", []).append(name)
                continue
            c, a = here.below(v)
            if c is None:
                sys.exit(f"{name} (0x{v:08X}): nothing in {mod.name} below it")
            out.append(f"{name} = {c}" + (f" + 0x{v - a:X};" if v != a else ";"))
            report.setdefault("relative (own module)", []).append(name)
        elif owner.name in layouts:
            cont, cur = layouts[owner.name]
            c, a = cont.below(v)
            if c is None:
                sys.exit(f"{name} (0x{v:08X}): nothing in {owner.name} below it")
            out.append(f"{name} = 0x{cur[c] + v - a:X}; /* {c} + 0x{v - a:X} in {owner.name} */")
            report.setdefault(f"from {owner.name}'s link", []).append(name)
        else:
            out.append(f"{name} = 0x{v:X}; /* {owner.name}'s, for the layout link */")
            report.setdefault(f"{owner.name}'s (layout link: original address)", []).append(name)
    Path(args.out).write_text("\n".join(out) + "\n")
    if args.report:
        lines = []
        for k in sorted(report):
            lines.append(f"{k}: {len(report[k])}")
            lines += [f"    {n}" for n in sorted(report[k])]
        Path(args.report).write_text("\n".join(lines) + "\n")


main()
