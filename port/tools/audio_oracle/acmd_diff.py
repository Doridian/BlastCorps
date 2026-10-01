#!/usr/bin/env python3
"""Compare two audio command lists (replay --dump-acmd), command by command.

    acmd_diff.py A B [--context N]

The lists are as the replay's i386 build has them: each command two
little-endian words.
"""
import argparse
import struct
import sys

NAMES = {0: "SPNOOP", 1: "ADPCM", 2: "CLEARBUFF", 3: "ENVMIXER", 4: "LOADBUFF", 5: "RESAMPLE",
         6: "SAVEBUFF", 7: "SEGMENT", 8: "SETBUFF", 9: "SETVOL", 10: "DMEMMOVE", 11: "LOADADPCM",
         12: "MIXER", 13: "INTERLEAVE", 14: "POLEF", 15: "SETLOOP"}


def load(path):
    data = open(path, "rb").read()
    return [struct.unpack_from("<II", data, i) for i in range(0, len(data), 8)]


def show(c):
    w0, w1 = c
    op = w0 >> 24
    return "%-10s %08X %08X" % (NAMES.get(op, "op%d" % op), w0, w1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--context", type=int, default=12)
    args = ap.parse_args()
    a, b = load(args.a), load(args.b)
    n = min(len(a), len(b))
    first = next((i for i in range(n) if a[i] != b[i]), None)
    if first is None:
        print("the same for %d commands%s" % (n, "" if len(a) == len(b) else
                                                 " (lengths %d and %d)" % (len(a), len(b))))
        return 0
    print("first difference at command %d of %d/%d" % (first, len(a), len(b)))
    for i in range(max(0, first - args.context), min(max(len(a), len(b)), first + args.context)):
        sa = show(a[i]) if i < len(a) else ""
        sb = show(b[i]) if i < len(b) else ""
        mark = "  " if sa == sb else "!!"
        print("%5d %s %-32s | %s" % (i, mark, sa, sb))
    return 1


if __name__ == "__main__":
    sys.exit(main())
