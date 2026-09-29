#!/usr/bin/env python3
"""Diff one function between the original and the rebuilt module.

A lighter stand-in for asm-differ: looks the function up in the stage-2 map,
disassembles the same range out of both binaries, and prints a unified diff.
Exits 0 when the bytes are identical.

    tools/fdiff.py init inflate_codes [--version us.v11] [--make]

The version defaults to the one blastcorps/ is extracted for.
"""

import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STAGE2 = ROOT / "blastcorps"
OBJDUMP = "mips-linux-gnu-objdump"

SECTION = re.compile(r"^\.(\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+) load address 0x([0-9a-f]+)")
SYMBOL = re.compile(r"^\s+0x([0-9a-f]{8,16})\s+(\S+)$")


def read_map(path: Path):
    """Return (sections, symbols): [(vram, size, rom)], {name: vram}."""
    sections, symbols = [], {}
    lines = path.read_text().splitlines()
    for i, line in enumerate(lines):
        m = SECTION.match(line)
        if not m and i + 1 < len(lines) and re.match(r"^\.\S+$", line):
            m = SECTION.match(line + lines[i + 1])
        if m:
            sections.append((int(m.group(2), 16), int(m.group(3), 16),
                             int(m.group(4), 16)))
            continue
        m = SYMBOL.match(line)
        if m and not m.group(2).startswith("."):
            symbols.setdefault(m.group(2), int(m.group(1), 16))
    return sections, symbols


def disasm(image: Path, start: int, end: int) -> list[str]:
    out = subprocess.run(
        [OBJDUMP, "-D", "-z", "-b", "binary", "-m", "mips:4300", "-EB",
         f"--start-address={start}", f"--stop-address={end}", str(image)],
        check=True, capture_output=True, text=True).stdout
    lines = []
    for line in out.splitlines():
        m = re.match(r"^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+(.*)$", line)
        if m:
            lines.append(f"{int(m.group(1), 16) - start:5x}: {m.group(2)}  {m.group(3)}")
    return lines


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("module")
    ap.add_argument("function")
    extracted = STAGE2 / ".version"
    ap.add_argument("--version", default=extracted.read_text().strip() if extracted.exists() else "us.v11")
    ap.add_argument("--make", action="store_true",
                    help="rebuild the module first (ignoring the sha1 check)")
    args = ap.parse_args()

    name = f"{args.module}.{args.version}"
    if args.make:
        subprocess.run(["make", "-s", f"VERSION={args.version}", args.module],
                       cwd=STAGE2, check=True)

    # A failed link leaves the previous binary in place, which would diff
    # clean against code that no longer builds.
    image = STAGE2 / "build" / f"{name}.bin"
    inputs = [*(STAGE2 / "build").glob(f"src/{args.module}/*.o"),
              *(STAGE2 / "src" / args.module).glob("*.c"),
              *(STAGE2 / "include").rglob("*.h")]
    if any(f.stat().st_mtime > image.stat().st_mtime for f in inputs):
        sys.exit(f"{image} is older than its sources; the last build failed")

    sections, symbols = read_map(STAGE2 / "build" / f"{name}.map")
    if args.function not in symbols:
        sys.exit(f"{args.function} is not in the {name} map")
    vram = symbols[args.function]
    later = sorted(v for v in symbols.values() if v > vram)
    for s_vram, s_size, s_rom in sections:
        if s_vram <= vram < s_vram + s_size:
            break
    else:
        sys.exit(f"{args.function} at {vram:#x} is in no loaded section")
    end_vram = min([s_vram + s_size] + later[:1])
    start, end = vram - s_vram + s_rom, end_vram - s_vram + s_rom

    base = disasm(STAGE2 / f"{name}.bin", start, end)
    mine = disasm(image, start, end)
    if base == mine:
        print(f"{args.function}: OK ({end - start:#x} bytes)")
        return
    sys.stdout.writelines(l + "\n" for l in difflib.unified_diff(
        base, mine, "original", "rebuilt", lineterm="", n=3))
    sys.exit(1)


if __name__ == "__main__":
    main()
