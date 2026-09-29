#!/usr/bin/env python3
"""Compare two builds of the port run for run (docs/PORT.md, "Comparing builds").

  build_cmp.py rdram DIR_A DIR_B N...
      rdram_N.bin (PORT_DUMP=N,...) of two runs, word by word, leaving out
      words that are addresses in the port's image or on the fibers' host
      stacks in both (they differ between builds by construction); prints
      the first differing words of each dump.

  build_cmp.py trace A.bin B.bin EXE...
      two PORT_TRACE=FILE call traces (a PORT_TRACE_CALLS build): the first
      call where they part, with the calls around it, named from EXE's
      symbols.

Runs to be compared need the same inputs and a timing that doesn't depend
on the code: --deterministic, the same --save (or none), PORT_COUNT_PER_OP=0.
"""

import subprocess
import sys

import numpy as np


def image(v):
    return (v >= 0x80400000) & (v < 0x81000000)


def stack(v):
    return (v >= 0x90000000) & (v < 0x91000000)


def rdram(a, b, ns):
    for n in ns:
        x = np.fromfile(f"{a}/rdram_{n}.bin", ">u4")
        y = np.fromfile(f"{b}/rdram_{n}.bin", ">u4")
        d = np.nonzero(x != y)[0]
        real = [w for w in d if not (image(x[w]) and image(y[w])) and not (stack(x[w]) and stack(y[w]))]
        print(f"rdram_{n}: {len(real)} words differ",
              " ".join(f"{0x80000000 + 4 * w:08X}:{x[w]:08X}/{y[w]:08X}" for w in real[:8]))


def fnv(name):
    h = 2166136261
    for c in name.encode():
        h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h


def trace(a, b, exes):
    names = {0: "-- controller read --"}
    for exe in exes:
        for line in subprocess.run(["nm", exe], capture_output=True, text=True).stdout.splitlines():
            p = line.split()
            if len(p) == 3 and p[1] in "Tt":
                names[fnv(p[2])] = p[2]
    x = np.fromfile(a, "<u4")
    y = np.fromfile(b, "<u4")
    n = min(len(x), len(y))
    d = np.nonzero(x[:n] != y[:n])[0]
    if not len(d):
        print(f"identical for {n} calls ({len(x)} and {len(y)} in all)")
        return
    i = int(d[0])
    print(f"first difference at call {i}, after {int((x[:i] == 0).sum())} controller reads of the trace")
    for k in range(max(0, i - 12), min(n, i + 8)):
        print(f"{k:9d} {'>' if k == i else ' '} {names.get(int(x[k]), hex(x[k])):32s} {names.get(int(y[k]), hex(y[k]))}")


if __name__ == "__main__":
    if len(sys.argv) > 4 and sys.argv[1] == "rdram":
        rdram(sys.argv[2], sys.argv[3], sys.argv[4:])
    elif len(sys.argv) > 4 and sys.argv[1] == "trace":
        trace(sys.argv[2], sys.argv[3], sys.argv[4:])
    else:
        sys.exit(__doc__)
