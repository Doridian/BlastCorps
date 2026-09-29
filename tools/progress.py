#!/usr/bin/env python3
"""Report how much of each code module is decompiled.

Reads the stage-2 link maps for the .text size of every object, then
subtracts what each C file still pulls in through GLOBAL_ASM for the version
(the build's copy of the file after tools/version_ifs.py, so a function that
is asm only in this version counts).  Handwritten asm (asm/<module>/*.s) and
the data islands carved out of .text are counted separately: they are not
meant to become C for the matching build.  An object that is C in us.v11 but
the whole of which is asm in this version (gen_code_yaml.py --asm-object)
counts as C still to do.

Run from the repo root after `make VERSION=<v> -C blastcorps` (one version's
build is there at a time):

    tools/progress.py [--version us.v11] [--json]
"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STAGE2 = ROOT / "blastcorps"
MODULES = ["init", "hd_code", "hd_front_end"]
REF = "us.v11"

TEXT_LINE = re.compile(r"^ \.text\s+0x[0-9a-f]+\s+0x([0-9a-f]+)\s+(\S+)$")
GLOBAL_ASM = re.compile(r'#pragma GLOBAL_ASM\("([^"]+)"\)')
# One disassembled instruction: /* ROM VRAM WORD */
INSN = re.compile(r"^\s*/\* [0-9A-F]+ [0-9A-F]{8} [0-9A-F]{8} \*/")


def asm_bytes(path: Path) -> int:
    with path.open() as f:
        return 4 * sum(1 for line in f if INSN.match(line))


def ref_c_objects(module: str) -> set:
    """The objects that are C files in us.v11's config."""
    cfg = (STAGE2 / f"{module}.{REF}.yaml").read_text()
    return set(re.findall(r"- \[0x[0-9A-Fa-f]+, c, (\S+?)\]", cfg))


def module_progress(module: str, version: str) -> dict:
    map_path = STAGE2 / "build" / f"{module}.{version}.map"
    if not map_path.exists():
        sys.exit(f"{map_path} is missing; build {version} first")

    c_objects = ref_c_objects(module)
    c_total = asm_left = handwritten = islands = 0
    funcs_left = objects_asm = 0
    with map_path.open() as f:
        for line in f:
            m = TEXT_LINE.match(line.rstrip())
            if not m:
                continue
            size = int(m.group(1), 16)
            obj = m.group(2).removeprefix("build/")
            if obj.endswith(".c.o"):
                c_total += size
                src = obj.removesuffix(".c.o")
                ver = STAGE2 / "build" / f"{src}.ver.c"
                text = (ver if ver.exists() else STAGE2 / f"{src}.c").read_text()
                for inc in GLOBAL_ASM.findall(text):
                    asm_left += asm_bytes(STAGE2 / inc)
                    funcs_left += 1
            elif obj.endswith(".s.o"):
                name = obj.removeprefix("asm/").removesuffix(".s.o")
                if name in c_objects:
                    c_total += size
                    asm_left += size
                    objects_asm += 1
                else:
                    handwritten += size
            elif obj.endswith(".bin.o"):
                islands += size

    done = c_total - asm_left
    return {
        "module": module,
        "decompilable": c_total,
        "decompiled": done,
        "functions_left": funcs_left,
        "objects_asm": objects_asm,
        "handwritten_asm": handwritten,
        "text_islands": islands,
        "percent": 100.0 * done / c_total if c_total else 0.0,
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--version", default=REF)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    rows = [module_progress(m, args.version) for m in MODULES]
    total = sum(r["decompilable"] for r in rows)
    done = sum(r["decompiled"] for r in rows)

    if args.json:
        print(json.dumps({"version": args.version, "modules": rows,
                          "percent": 100.0 * done / total}, indent=2))
        return

    print(f"{args.version}:")
    print(f"{'module':<14}{'C bytes':>10}{'total':>10}{'%':>8}"
          f"{'funcs left':>12}{'objs asm':>10}{'hand asm':>10}")
    for r in rows:
        print(f"{r['module']:<14}{r['decompiled']:>10}{r['decompilable']:>10}"
              f"{r['percent']:>7.2f}%{r['functions_left']:>12}"
              f"{r['objects_asm']:>10}{r['handwritten_asm']:>10}")
    print(f"{'all':<14}{done:>10}{total:>10}{100.0 * done / total:>7.2f}%")


if __name__ == "__main__":
    main()
