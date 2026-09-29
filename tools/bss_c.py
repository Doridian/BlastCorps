#!/usr/bin/env python3
"""Make a C file define its object's .bss.

Usage (from blastcorps/):
  bss_c.py <module> <object> [--version us.v11] [--dry-run]
  bss_c.py --check <module> <object> [--version us.v11]

The object's block in the config's `bss:` list (tools/modmap.py) must be
`.bss`, and extraction must have written the block as it is in the ROM to
asm/bss/<module>/<object>.bss.s (tools/postsplit.py does, for every `.bss`
block).  This writes one definition per variable, in address order, before
the first function (or whatever else in the file names them first):

- a variable the file (or a header) declares keeps its declared type
  (`extern` dropped; an array of unknown size gets the size its block has,
  or less where IDO would align it past its address);
- one it doesn't declare, and any gap IDO's alignment doesn't leave by
  itself, is a `u8` array named by its address;
- a label inside a variable (another object's reference to a field) or a
  second name for one is not defined; tools/link_syms.py makes it relative.

IDO lays a file's .bss out in the order it first sees each variable
(defined or used; an extern declaration doesn't count), aligns each by its
size (8 from 8 bytes up, else 4, 2, 1) and pads the section to 16.  Sizes
come from compiling the file once with IDO.  --check compares the built
object's .bss with the block.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import modmap  # noqa: E402
import vermap  # noqa: E402

CC = str(HERE / "ido5.3_recomp" / "cc")
CFLAGS = ["-c", "-G", "0", "-Xfullwarn", "-Xcpluscomm", "-signed", "-nostdinc", "-non_shared",
          "-Wab,-r4300_mul", "-D_LANGUAGE_C", "-D_FINALROM", "-woff", "649,838",
          "-I", ".", "-I", "include", "-I", "include/2.0I", "-I", "include/2.0I/PR",
          "-mips2", "-o32", "-O1"]
VERSION_FLAGS = ["-DVERSION_US_V11"]  # main() sets the version's


def ido_align(size):
    for a in (8, 4, 2):
        if size >= a:
            return a
    return 1


def block(mod, obj):
    for k, (start, kind, name) in enumerate(mod.bss):
        if name == f"{mod.name}/{obj}":
            end = mod.bss[k + 1][0] if k + 1 < len(mod.bss) else mod.bss_end
            return start, end, kind
    sys.exit(f"{obj} has no .bss block in {mod.cfg_path}")


def reference(mod, obj, start, kind):
    sub = "data" if kind == "bss" else "bss"
    path = Path("asm") / sub / mod.name / f"{obj}.bss.s"
    out, pos = [], start
    names = []
    for line in path.read_text().splitlines():
        m = re.match(r"dlabel (\w+)", line)
        if m:
            names.append(m.group(1))
        m = re.match(r"\.space (0x[0-9A-Fa-f]+)", line)
        if m:
            out.append((pos, names, int(m.group(1), 16)))
            pos += int(m.group(1), 16)
            names = []
    return out


DECL_RE = r"^(?P<ext>extern\s+)?(?P<type>(?:(?:const|volatile|unsigned|signed|struct|union|enum)\s+)*\w+(?:\s+\w+)*?)\s*(?P<ptr>[\s\*]*)\b(?P<name>{})\b(?P<dims>(?:\s*\[[^\]]*\])*)\s*;(?P<rest>.*)$"


def find_decls(src, names):
    """{name: (line index, type text, ptr, dims)} of file-scope declarations."""
    lines = src.split("\n")
    out = {}
    depth = 0
    for n, line in enumerate(lines):
        if depth == 0:
            m = re.match(DECL_RE.format("|".join(map(re.escape, names))), line)
            if m and "(" not in m.group("type") and m.group("type").split()[0] not in ("return", "typedef"):
                out.setdefault(m.group("name"), []).append(
                    (n, m.group("type"), m.group("ptr").strip(), m.group("dims").replace(" ", ""), m.group("ext")))
        depth += line.count("{") - line.count("}")
    return out


def sizes(c_path, decls):
    """IDO's sizeof of each declared variable (of one element for [])."""
    probe = Path("build") / "bss_c_probe.c"
    probe.parent.mkdir(exist_ok=True)
    text = c_path.read_text() + "\n"
    order = sorted(decls)
    for i, name in enumerate(order):
        dims = decls[name][-1][3]
        expr = f"sizeof({name}[0])" if dims.startswith("[]") else f"sizeof({name})"
        text += f"unsigned int __bss_c_size_{i} = {expr};\n"
    probe.write_text(text)
    obj = probe.with_suffix(".o")
    r = subprocess.run([CC, *CFLAGS, *VERSION_FLAGS, "-o", str(obj), str(probe)], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"probe compile failed:\n{r.stderr}")
    data = subprocess.run(["mips-linux-gnu-objcopy", "-O", "binary", "--only-section", ".data", str(obj), "/dev/stdout"],
                          capture_output=True).stdout
    # the probe's own words are the last len(order) of .data (padded to 16)
    syms = subprocess.run(["mips-linux-gnu-nm", str(obj)], capture_output=True, text=True).stdout
    offs = {}
    for line in syms.splitlines():
        p = line.split()
        if len(p) == 3 and p[2].startswith("__bss_c_size_"):
            offs[int(p[2][13:])] = int(p[0], 16)
    return {name: int.from_bytes(data[offs[i]:offs[i] + 4], "big") for i, name in enumerate(order)}


def pieces(addr, size):
    """Split [addr, addr+size) into u8 arrays that IDO leaves where they are."""
    out = []
    while size:
        amax = next(a for a in (8, 4, 2, 1) if addr % a == 0)
        n = size if amax == 8 or size < 2 * amax else amax
        out.append((addr, n))
        addr += n
        size -= n
    return out


def preprocess(c_path):
    r = subprocess.run(["cpp", "-P", "-undef", "-nostdinc", "-I", ".", "-I", "include", "-I", "include/2.0I",
                        "-I", "include/2.0I/PR", "-D_LANGUAGE_C", "-D_FINALROM", "-D_MIPS_SZLONG=32",
                        "-D_MIPS_SZINT=32", "-D__sgi", *VERSION_FLAGS, str(c_path)], capture_output=True, text=True)
    return r.stdout


VM = vermap.VerMap("us.v11")  # the version's names (tools/vermap.py)


def plan(ref, start, end, decls, size_of):
    """[(addr, name, text)] definitions, and the names left to the linker
    (inside a variable, or a second name for one)."""
    out, skipped = [], []
    pos = start

    def pad(a, n, name=None):
        for i, (pa, pn) in enumerate(pieces(a, n)):
            nm = name if (i == 0 and name) else VM.name("D", pa, True)
            out.append((pa, nm, f"u8 {nm}[{pn}];" if pn < 10 else f"u8 {nm}[0x{pn:X}];"))

    def align_up(a, n):
        return (a + n - 1) // n * n

    for addr, names, gap in ref:
        if addr < pos:
            skipped += names
            continue
        name = next((n for n in names if n in decls), names[0])
        skipped += [n for n in names if n != name]
        if name in decls:
            _, typ, ptr, dims, _ = decls[name][-1]
            size = size_of[name]
            if dims.startswith("[]"):
                n = max(1, gap // size)
                # IDO aligns by size: a smaller array here, and padding after
                amax = next(a for a in (8, 4, 2, 1) if addr % a == 0)
                while n > 1 and addr % ido_align(n * size):
                    n = min(n - 1, (2 * amax - 1) // size)
                dims = (f"[{n:#x}]" if n > 9 else f"[{n}]") + dims[2:]
                size *= n
            text = f"{typ} {ptr}{name}{dims};"
            first = size  # (the whole array)
        else:
            text = None
            first = pieces(addr, gap)[0][1]
        # a gap IDO's own alignment leaves needs no padding
        if addr > pos and align_up(pos, ido_align(first)) != addr:
            pad(pos, addr - pos)
        pos = addr
        if text is None:
            pad(addr, gap, name)
            pos = addr + gap
            continue
        if addr % ido_align(size):
            sys.exit(f"{name} at 0x{addr:08X}: IDO would align its {size} bytes to {ido_align(size)}")
        out.append((addr, name, text))
        pos = addr + size
    if align_up(pos, 16) != end:
        pad(pos, end - pos)
    return out, skipped


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("object")
    ap.add_argument("--version", default="us.v11")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    mod = modmap.Module(f"{args.module}.{args.version}.yaml")
    start, end, kind = block(mod, args.object)
    ref = reference(mod, args.object, start, kind)
    global VM
    VM = vermap.load(args.version, Path("."))
    VERSION_FLAGS[:] = ["-DVERSION_" + args.version.upper().replace(".", "_")]
    c_path = Path("src") / args.module / f"{args.object}.c"

    if args.check:
        obj = Path("build") / "src" / args.module / f"{args.object}.c.o"
        out = subprocess.run(["mips-linux-gnu-readelf", "-sSW", str(obj)], capture_output=True, text=True).stdout
        idx = re.search(r"\[\s*(\d+)\] \.bss\s", out)
        have = {}
        for line in out.splitlines():
            p = line.split()
            if idx and len(p) >= 8 and p[6] == idx.group(1) and p[3] == "OBJECT":
                have[p[7]] = int(p[1], 16)
        size = re.search(r"\] \.bss\s+NOBITS\s+\w+ \w+ (\w+)", out)
        bad = 0
        for addr, names, _ in ref:
            for name in names:
                if name in have and have[name] != addr - start:
                    print(f"{name}: at +0x{have[name]:X}, should be +0x{addr - start:X}")
                    bad += 1
        sz = int(size.group(1), 16) if size else 0
        if sz != end - start:
            print(f".bss is 0x{sz:X} bytes, the block 0x{end - start:X}")
            bad += 1
        sys.exit(1 if bad else 0)

    if kind != ".bss":
        sys.exit(f"{args.object}'s block is `{kind}`; make it `.bss` in the config first")
    src = c_path.read_text()
    names = [n for _, ns, _ in ref for n in ns]
    local = find_decls(src, names)
    decls = dict(find_decls(preprocess(c_path), names))
    decls.update(local)  # the file's own spelling, macros and all
    size_of = sizes(c_path, decls) if decls else {}
    defs, skipped = plan(ref, start, end, decls, size_of)
    if args.dry_run:
        for a, _, t in defs:
            print(f"/* {a:08X} */ {t}")
        if skipped:
            print("left to the linker:", " ".join(skipped))
        return

    lines = src.split("\n")
    defined = {n for _, n, _ in defs}
    decl_lines = sorted(n for name, v in local.items() if name in defined for n, *_ in v if v)
    # IDO lays .bss out in the order it first sees each variable, defined or
    # used (an extern declaration doesn't count), so the definitions go
    # before any use: before the first function or anything else that
    # names them (a pointer in .data, say).  The file's declarations above
    # that point go; any further down stay, harmlessly.
    word = re.compile(r"\b(" + "|".join(map(re.escape, defined)) + r")\b")
    at = next((n for n, l in enumerate(lines)
               if re.match(r"^\w[\w\s\*]*\([^;]*$", l) or "GLOBAL_ASM" in l or l.startswith('#include "src/')
               or (n not in decl_lines and word.search(l)
                   and not l.lstrip().startswith(("#define", "/*", "*", "//")))), len(lines))
    while at > 0 and (lines[at - 1].startswith("/*") or lines[at - 1].startswith(" *")):
        at -= 1  # above the function's comment
    drop = {n for n in decl_lines if n < at}
    # A stub that includes the code: the includes that declare the types
    inc = re.match(r'#include "(src/[^"]+)"', lines[at]) if at < len(lines) else None
    extra = []
    if inc and Path(inc.group(1)).exists():
        have = set(re.findall(r'#include [<"]([^>"]+)[>"]', src))
        extra = [l for l in Path(inc.group(1)).read_text().splitlines()
                 if re.match(r'#include "[^"/]+\.h"', l) and l.split('"')[1] not in have]
    block_text = extra + [f"/* .bss, 0x{start:08X}-0x{end:08X} (tools/bss_c.py) */"] + [t for _, _, t in defs] + [""]
    if at == len(lines) or not lines[at - 1].strip() == "":
        block_text = [""] + block_text
    keep = [l for n, l in enumerate(lines) if n not in drop]
    shift = sum(1 for n in drop if n < at)
    keep[at - shift:at - shift] = block_text
    c_path.write_text("\n".join(keep))
    print(f"{c_path}: {len(defs)} definitions" + (f"; left to the linker: {' '.join(skipped)}" if skipped else ""))


if __name__ == "__main__":
    main()
