#!/usr/bin/env python3
"""Resolve a C file's version conditionals before asm-processor sees it.

One source tree builds every version (`-DVERSION_US_V10`, `VERSION_US_V11`,
`VERSION_JP` or `VERSION_EU`), and a function that differs in one version
is `#if`'d.  asm-processor reads every `GLOBAL_ASM` line in a file,
preprocessor or not, and opens its .s, which exists only for the version
that was extracted.  So the Makefile runs the file through this first: an
`#if`/`#ifdef`/`#ifndef` whose condition names only `VERSION_*` macros is
decided here, its directives and the branches not taken become blank lines
(line numbers stay), and every other line, other conditionals included, is
left for the compiler.

Usage:
  version_ifs.py <VERSION_X> <in.c> [-o <out.c>]
"""
import argparse
import re
import sys

DIRECTIVE = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")
TOKEN = re.compile(r"\s*(defined|VERSION_\w+|\w+|&&|\|\||!|\(|\)|\d+)")


def version_only(expr):
    """True if expr is built from VERSION_* macros alone."""
    names = re.findall(r"[A-Za-z_]\w*", expr)
    return bool(names) and all(n == "defined" or n.startswith("VERSION_") for n in names)


def evaluate(expr, defined):
    """Evaluate a condition over VERSION_* macros (defined ones are 1)."""
    expr = re.sub(r"/\*.*?\*/|//.*$", "", expr).strip()
    py = re.sub(r"defined\s*\(\s*(VERSION_\w+)\s*\)|defined\s+(VERSION_\w+)",
                lambda m: str(int((m.group(1) or m.group(2)) in defined)), expr)
    py = re.sub(r"VERSION_\w+", lambda m: str(int(m.group(0) in defined)), py)
    py = py.replace("&&", " and ").replace("||", " or ")
    py = re.sub(r"!(?!=)", " not ", py)
    return bool(eval(py, {"__builtins__": {}}))


def process(lines, version):
    defined = {version}
    out = []
    # stack of [kind, active, taken]: kind "v" is a version conditional
    # (decided here), "o" any other (passed on)
    stack = []

    def live():
        return all(e[1] for e in stack if e[0] == "v")

    for line in lines:
        m = DIRECTIVE.match(line)
        if not m:
            out.append(line if live() else "")
            continue
        d, rest = m.group(1), m.group(2).strip()
        rest_nc = re.sub(r"/\*.*?\*/|//.*$", "", rest).strip()
        if d in ("if", "ifdef", "ifndef"):
            outer = live()
            cond = rest_nc if d == "if" else f"defined({rest_nc})"
            if version_only(cond):
                v = evaluate(cond, defined)
                if d == "ifndef":
                    v = not v
                stack.append(["v", v, v])
                out.append("")
            else:
                stack.append(["o", True, True])
                out.append(line if outer else "")
            continue
        if not stack:
            sys.exit(f"#{d} without #if")
        top = stack[-1]
        if top[0] == "o":
            if d == "elif" and version_only(rest_nc):
                sys.exit("#elif on VERSION_* inside another kind of #if; nest a separate #if")
            if d == "endif":
                stack.pop()
            ok = live()
            out.append(line if ok else "")
            continue
        if d == "elif":
            if not version_only(rest_nc):
                sys.exit("#elif mixing VERSION_* and other macros")
            v = not top[2] and evaluate(rest_nc, defined)
            top[1] = v
            top[2] = top[2] or v
        elif d == "else":
            top[1] = not top[2]
            top[2] = True
        elif d == "endif":
            stack.pop()
        out.append("")
    if stack:
        sys.exit("unterminated #if")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("version", help="the macro that is defined, e.g. VERSION_US_V11")
    ap.add_argument("src")
    ap.add_argument("-o", "--out")
    args = ap.parse_args()
    with open(args.src, encoding="latin-1") as f:
        lines = f.read().split("\n")
    text = "\n".join(process(lines, args.version))
    if args.out:
        with open(args.out, "w", encoding="latin-1") as f:
            f.write(text)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
