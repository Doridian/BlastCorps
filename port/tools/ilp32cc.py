#!/usr/bin/env python3
"""Compile N64-side C for a 64-bit host with the N64's data layout.

CMake's compiler launcher for the 64-bit build (PORT_64BIT, docs/PORT.md
"64-bit"):

  ilp32cc.py --opt OPT --plugin LIBBEPASS.so --target TRIPLE [--check] -- CC ARGS...

ARGS is the compile command CMake would run (i386 flags included).  It runs
in three steps, keeping the intermediate files next to the object:

  1. CC ARGS for i386 to unoptimised IR (X.i386.bc): the frontend lays out
     every type as the N64 does;
  2. opt -passes=port-ilp32 (X.bc): the module becomes TRIPLE's, 64-bit
     pointers in registers, 32-bit ones in memory, the layout unchanged;
  3. CC for TRIPLE, optimising and generating code with the plugin's own
     passes (BEPass, the polls, the instruction count).
"""

import subprocess
import sys

STAGE3_PREFIXES = ("-O", "-g", "-fdata-sections", "-ffunction-sections", "-ffp-contract", "-fpass-plugin",
                   "-fno-builtin", "-fPIE", "-fPIC", "-fpie", "-fpic", "-fno-pic", "-fno-pie",
                   "-fno-omit-frame-pointer", "-fomit-frame-pointer", "-fsanitize",
                   "-fno-vectorize", "-fno-slp-vectorize", "-emit-llvm")


def main():
    argv = sys.argv[1:]
    opts = {}
    while argv and argv[0] != "--":
        k = argv.pop(0)
        if k == "--check":
            opts["check"] = True
        else:
            opts[k] = argv.pop(0)
    argv.pop(0)
    cc, args = argv[0], argv[1:]

    out = None
    src = None
    s1 = []
    k = 0
    while k < len(args):
        a = args[k]
        if a == "-o":
            out = args[k + 1]
            k += 2
            continue
        if a == "-c":
            src = args[k + 1]
            k += 2
            continue
        if a in ("-target", "--target"):          # a cross build's: stage 1 is i386
            k += 2
            continue
        if not a.startswith(("-fpass-plugin", "--target=")):
            s1.append(a)
        k += 1
    if not out or not src:
        sys.exit("ilp32cc.py: need -o and -c")
    ir32 = out + ".i386.bc"
    ir64 = out + ".bc"
    run([cc, "--target=i386-pc-linux-gnu"] + s1 +
        ["-emit-llvm", "-Xclang", "-disable-llvm-passes", "-o", ir32, "-c", src])
    passes = ["-port-ilp32-triple=" + opts["--target"]]
    if opts.get("check"):
        passes.append("-port-ilp32-check")
    run([opts["--opt"], "-load-pass-plugin=" + opts["--plugin"], "-passes=port-ilp32"] + passes +
        [ir32, "-o", ir64])
    s3 = [a for a in args if a.startswith(STAGE3_PREFIXES + ("--sysroot", "--gcc-toolchain"))]
    run([cc, "--target=" + opts["--target"], "-Wno-override-module"] + s3 + ["-c", ir64, "-o", out])


def run(cmd):
    r = subprocess.run(cmd)
    if r.returncode:
        sys.exit(r.returncode)


if __name__ == "__main__":
    main()
