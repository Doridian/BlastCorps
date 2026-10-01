#!/usr/bin/env python3
"""Compare two shared-memory dumps (replay --dump-mem): the bytes that
differ outside the library's own heap blocks and the players' handlers.

    mem_diff.py A B [--all]     (--all: inside the masks too)
"""
import argparse
import struct
import sys


def load(path):
    data = open(path, "rb").read()
    ranges, masks = [], []
    i = 0
    while i < len(data):
        a, n = struct.unpack_from("<II", data, i)
        if a == 0xFFFFFFFF:
            ma, mn = struct.unpack_from("<II", data, i + 4)
            masks.append((ma, mn))
            i += 12
        else:
            ranges.append((a, data[i + 8:i + 8 + n]))
            i += 8 + n
    return ranges, masks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--all", action="store_true")
    args = ap.parse_args()
    ra, ma = load(args.a)
    rb, _ = load(args.b)
    shown = 0
    for (a, da), (b, db) in zip(ra, rb):
        assert a == b and len(da) == len(db)
        run = None
        for k in range(len(da) + 1):
            diff = k < len(da) and da[k] != db[k]
            addr = a + k
            if diff and not args.all and any(addr - m < n for m, n in ma):
                diff = False
            if diff and run is None:
                run = k
            elif not diff and run is not None:
                print("%08X +%-4X %s | %s" % (a + run, k - run, da[run:k][:32].hex(), db[run:k][:32].hex()))
                shown += 1
                run = None
                if shown > 60:
                    return 1
    print("%d differing runs" % shown)
    return 1 if shown else 0


if __name__ == "__main__":
    sys.exit(main())
