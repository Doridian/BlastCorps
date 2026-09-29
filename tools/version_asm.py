#!/usr/bin/env python3
"""Give a function its own asm in some versions.

The C of one function builds every version.  Where a version's function
differs and has no C of its own yet, it is that version's GLOBAL_ASM:

  #ifdef VERSION_JP
  #pragma GLOBAL_ASM("asm/nonmatchings/hd_code/26570/func_8026BCE0.s")
  #else
  s32 func_8026BCE0(...) { ... }
  #endif

Run from the repo root:

  tools/version_asm.py <module> <func> <version>...        # asm in these
  tools/version_asm.py --only <module> <func> --after <func> <version>...
      # a function only these versions have: its GLOBAL_ASM after <func>
      # (or --before <func>) in the file of that function
  tools/version_asm.py --not <module> <func> <version>...  # absent there
  tools/version_asm.py --as <name> <module> <func> <version>...
      # these versions have other code, under another name, in its place

A function already wrapped this way gets the versions added to its
condition.  Re-extract afterwards: splat writes the .s of every GLOBAL_ASM
a stub names, whatever the #if around it.
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "blastcorps" / "src"


def macro(v):
    return "VERSION_" + v.upper().replace(".", "_")


def cond(versions, negate=False):
    ms = [macro(v) for v in versions]
    if len(ms) == 1:
        return f"#ifndef {ms[0]}" if negate else f"#ifdef {ms[0]}"
    expr = " || ".join(f"defined({m})" for m in ms)
    return f"#if !({expr})" if negate else f"#if {expr}"


def versions_in(line):
    return [m.lower().replace("_", ".", 1) if m.startswith("US_") else m.lower()
            for m in re.findall(r"VERSION_(\w+)", line)]


def find(module, func):
    """(path, lines, first line, last line) of func's definition."""
    head = re.compile(rf"^[A-Za-z_][\w \*]*\b{func}\(")
    for path in sorted((SRC / module).glob("*.c")):
        lines = path.read_text().split("\n")
        for i, line in enumerate(lines):
            if not head.match(line):
                continue
            # a prototype can run over several lines too: the definition's
            # head ends in `{` (or a K&R parameter list) before any `;`
            k = i
            while k < len(lines) and not lines[k].rstrip().endswith((";", "{", ")")):
                k += 1
            if k < len(lines) and lines[k].rstrip().endswith(";"):
                continue
            if True:
                j = i
                while j < len(lines) and lines[j] != "}":
                    j += 1
                if j == len(lines):
                    sys.exit(f"{path}: no end of {func}")
                return path, lines, i, j
    sys.exit(f"{func}: not defined in src/{module}")


def asm_line(module, path, func):
    return f'#pragma GLOBAL_ASM("asm/nonmatchings/{module}/{path.stem}/{func}.s")'


def wrap(module, func, versions, asm_name=None):
    path, lines, i, j = find(module, func)
    # comments right above the function stay with the C
    start = i
    while start > 0 and (lines[start - 1].startswith("/*") or lines[start - 1].startswith(" *")
                         or lines[start - 1].startswith("//")):
        start -= 1
    asm = asm_line(module, path, asm_name or func)
    if start >= 3 and lines[start - 1] == "#else" and lines[start - 2] == asm:
        have = versions_in(lines[start - 3])
        lines[start - 3] = cond(sorted(set(have) | set(versions), key=have_order))
    else:
        lines[j + 1:j + 1] = ["#endif"]
        lines[start:start] = [cond(versions), asm, "#else"]
    path.write_text("\n".join(lines))
    print(f"{path.relative_to(ROOT)}: {func} is asm in {', '.join(versions)}")


def have_order(v):
    return ["us.v10", "us.v11", "jp", "eu"].index(v)


def only(module, func, versions, after=None, before=None):
    ref = after or before
    path, lines, i, j = find(module, ref)
    asm = f'#pragma GLOBAL_ASM("asm/nonmatchings/{module}/{path.stem}/{func}.s")'
    block = [cond(versions), asm, "#endif", ""]
    if after:
        # past a version wrapper that ends the function
        k = j + 1
        if k < len(lines) and lines[k] == "#endif":
            k += 1
        while k < len(lines) and lines[k] == "":
            k += 1
        lines[k:k] = block
    else:
        k = i
        while k > 0 and (lines[k - 1].startswith("/*") or lines[k - 1].startswith(" *")):
            k -= 1
        if k >= 3 and lines[k - 1] == "#else" and lines[k - 2].startswith("#pragma GLOBAL_ASM"):
            k -= 3
        lines[k:k] = block
    path.write_text("\n".join(lines))
    print(f"{path.relative_to(ROOT)}: {func}, only in {', '.join(versions)}")


def absent(module, func, versions):
    path, lines, i, j = find(module, func)
    start = i
    while start > 0 and (lines[start - 1].startswith("/*") or lines[start - 1].startswith(" *")):
        start -= 1
    lines[j + 1:j + 1] = ["#endif"]
    lines[start:start] = [cond(versions, negate=True)]
    path.write_text("\n".join(lines))
    print(f"{path.relative_to(ROOT)}: {func} is not in {', '.join(versions)}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--only", action="store_true")
    ap.add_argument("--not", dest="absent", action="store_true")
    ap.add_argument("--as", dest="asm_name", help="the name of the version's function in its place")
    ap.add_argument("--after")
    ap.add_argument("--before")
    ap.add_argument("module")
    ap.add_argument("func")
    ap.add_argument("versions", nargs="+", choices=["us.v10", "us.v11", "jp", "eu"])
    args = ap.parse_args()
    if args.only:
        only(args.module, args.func, args.versions, args.after, args.before)
    elif args.absent:
        absent(args.module, args.func, args.versions)
    else:
        wrap(args.module, args.func, args.versions, args.asm_name)


if __name__ == "__main__":
    main()
