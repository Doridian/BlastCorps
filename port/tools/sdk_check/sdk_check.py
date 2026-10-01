#!/usr/bin/env python3
"""The port's own SDK pieces against the SDK's originals, bit for bit.

    port/tools/sdk_check/sdk_check.py [--out DIR] [--random N] [--trace FILE...]
                                      [--no-exhaustive] [--arch m32,m64]

Builds check.c (this directory) twice over, for i386 and for x86-64 with the
LP64 port's `long`: once against port/src/gu.c, the port's own graphics
utilities, sine tables and pak CRC, and once against the originals in
blastcorps/src/libultra (and orig_rotate.c here), compiled as the port
compiled them before (clang, -ffp-contract=off, the port's headers).  Then:

  - sinf and fcos over all 2^32 floats, sins and coss over all 65,536 angles;
  - every gu function and the CRC on --random N argument sets (default 200,000);
  - the arguments of every call a port run made, for each --trace FILE (a
    -DPORT_SDK_TRACE=ON build writes them to $PORT_SDK_TRACE).

Each must give the same bits.  The originals are only ever built here.
"""
import argparse
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")

ORIGINALS = ["gu/sinf.c", "gu/cosf.c", "gu/sins.c", "gu/coss.c", "gu/mtxutil.c", "gu/mtxcatf.c",
             "gu/mtxcatl.c", "gu/normalize.c", "gu/ortho.c", "gu/perspective.c", "gu/scale.c",
             "gu/translate.c", "gu/lookat.c", "gu/lookatref.c", "io/crc.c"]
NAMES = ["sinf", "fcos", "sins", "coss", "guMtxIdentF", "guMtxIdent", "guTranslateF", "guTranslate",
         "guScaleF", "guScale", "guOrthoF", "guOrtho", "guPerspectiveF", "guPerspective", "guLookAtF",
         "guLookAt", "guLookAtReflectF", "guLookAtReflect", "guNormalize", "guRotateF", "guRotate",
         "guMtxCatF", "guMtxXFMF", "guMtxF2L", "guMtxL2F", "guMtxCatL", "guMtxXFML", "__osContDataCrc",
         "__osContAddressCrc", "__libm_qnan_f", "D_803FDF60", "port_sdk_trace"]


def run(cmd):
    r = subprocess.run(cmd)
    if r.returncode:
        sys.exit(f"sdk_check: failed: {' '.join(cmd)}")


def build(out, arch, cc):
    os.makedirs(out, exist_ok=True)
    for side in ("orig", "new"):
        with open(os.path.join(out, f"rename_{side}.h"), "w") as f:
            for n in NAMES:
                f.write(f"#define {n} {side}_{n}\n")
    gen = os.path.join(out, "gen")
    os.makedirs(gen, exist_ok=True)
    run([sys.executable, os.path.join(ROOT, "port", "tools", "gen_sintable.py"), os.path.join(gen, "sintable.h")])
    resdir = subprocess.run([cc, "-print-resource-dir"], capture_output=True, text=True).stdout.strip()
    flags = ["-m32" if arch == "m32" else "-m64", "-msse2", "-mfpmath=sse", "-std=gnu89", "-O2",
             "-fsigned-char", "-fno-strict-aliasing", "-fwrapv", "-ffp-contract=off", "-fno-builtin",
             "-fno-common", "-nostdinc", "-isystem", os.path.join(resdir, "include"), "-w",
             "-D_LANGUAGE_C", "-D_FINALROM", "-DTARGET_PC", "-D_MIPS_SZLONG=32", "-D_MIPS_SZINT=32",
             "-I", os.path.join(ROOT, "port", "include"), "-I", BLAST, "-I", os.path.join(BLAST, "include"),
             "-I", gen]
    if arch == "m64":
        flags += ["-DPORT_64BIT", "-DPORT_LP64", "-fms-extensions"]
    hdrs = {"orig": ["-I", os.path.join(BLAST, "include", "2.0I"), "-I", os.path.join(BLAST, "include", "2.0I", "PR")]}
    hdrs["new"] = hdrs["orig"]
    objs = []
    srcs = [("orig", os.path.join(BLAST, "src", "libultra", s)) for s in ORIGINALS]
    srcs += [("orig", os.path.join(HERE, "orig_rotate.c")), ("new", os.path.join(ROOT, "port", "src", "gu.c"))]
    for side, src in srcs:
        o = os.path.join(out, f"{side}_{os.path.basename(src)}.o")
        if src.endswith(("sinf.c", "cosf.c")):
            # their doubles are {hi, lo} word pairs: the N64's order, which the
            # port's BEPass reads as one big-endian double; on this host the
            # words go the other way round
            text = open(src).read()
            text = re.sub(r"\{ (0x[0-9a-f]{8}), (0x[0-9a-f]{8}) \}", r"{ \2, \1 }", text)
            src = os.path.join(out, "le_" + os.path.basename(src))
            with open(src, "w") as f:
                f.write(text)
        run([cc] + flags + hdrs[side] + ["-include", os.path.join(out, f"rename_{side}.h"), "-c", src, "-o", o])
        objs.append(o)
    exe = os.path.join(out, "check")
    run([cc, flags[0], "-O2", "-ffp-contract=off", os.path.join(HERE, "check.c")] + objs + ["-o", exe, "-lm", "-lpthread"])
    return exe


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=os.path.join(ROOT, "build", "sdk_check"))
    ap.add_argument("--cc", default="clang")
    ap.add_argument("--arch", default="m32,m64")
    ap.add_argument("--random", type=int, default=200000)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--trace", nargs="*", default=[])
    ap.add_argument("--no-exhaustive", action="store_true")
    a = ap.parse_args()
    bad = 0
    for arch in a.arch.split(","):
        exe = build(os.path.join(a.out, arch), arch, a.cc)
        steps = []
        if not a.no_exhaustive:
            steps.append([exe, "exhaustive"])
        if a.random:
            steps.append([exe, "random", str(a.random), str(a.seed)])
        steps += [[exe, "trace", t] for t in a.trace]
        for s in steps:
            print(f"== {arch}: {' '.join(s[1:])}", flush=True)
            if subprocess.run(s).returncode:
                bad = 1
    print("== sdk_check: " + ("FAILED" if bad else "all identical"))
    sys.exit(bad)


if __name__ == "__main__":
    main()
