#!/usr/bin/env python3
"""The TAS's levels, timed: two replays' gameplay digests against each other.

    tas_times.py REF.digest OTHER.digest [--markdown] [--names A,B] [--each]

For the levels' play (mode 4) in both digests (PORT_DIGEST, in order; the
replays must have played the same modes, as digest_cmp.py checks): the
game frames, the retraces each run took for them, and so the time the
level clock ("TIME IN LEVEL", the medal times: retraces / 6 tenths) ran
for them, and its difference.  A frame that takes more than the minimum 2
retraces is a lag frame; REF's lag retraces are its retraces over 2 a frame
(the frames are polls, the retraces from the first to the last).
By level, its stretches of mode 4 added up (a pause or a second try splits
one); --each lists every stretch instead.  (docs/PORT.md, "Lag
frames".)
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import digest_cmp  # noqa: E402


def plays(path):
    out = []
    for s in digest_cmp.parse(path):
        if s.mode == 4:
            fr, rt, _, _ = digest_cmp.times(s)
            out.append((s.name().split(" (")[0], s.level, fr, rt or 0))
    return out


def fmt(retraces):
    t = retraces // 6                   # tenths, as the level clock
    return f"{t // 600}:{t // 10 % 60:02d}.{t % 10}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ref")
    ap.add_argument("other")
    ap.add_argument("--markdown", action="store_true")
    ap.add_argument("--names", default="reference,other")
    ap.add_argument("--each", action="store_true")
    args = ap.parse_args()
    na, nb = args.names.split(",")
    a, b = plays(args.ref), plays(args.other)
    if [(x[1], x[2]) for x in a] != [(x[1], x[2]) for x in b]:
        print("the digests didn't play the same levels' frames: digest_cmp.py says where", file=sys.stderr)
        return 1
    rows = {}
    order = []
    for i, (x, y) in enumerate(zip(a, b)):
        key = i if args.each else x[1]
        if key not in rows:
            rows[key] = [x[0], 0, 0, 0, 0]
            order.append(key)
        r = rows[key]
        r[1] += 1
        r[2] += x[2]
        r[3] += x[3]
        r[4] += y[3]
    head = ["level", "stretches", "frames", f"lag retraces ({na})", na, nb, "difference"]
    out = []
    tot = [0, 0, 0, 0]
    for k in order:
        name, n, fr, ra, rb = rows[k]
        out.append([name, str(n), str(fr), str(max(0, ra - 2 * (fr - n))), fmt(ra), fmt(rb),
                    f"{(rb // 6 - ra // 6) / 10:+.1f} s ({100 * (rb - ra) / max(1, ra):+.0f}%)"])
        tot[0] += n
        tot[1] += fr
        tot[2] += ra
        tot[3] += rb
    out.append(["all", str(tot[0]), str(tot[1]), str(max(0, tot[2] - 2 * (tot[1] - tot[0]))), fmt(tot[2]), fmt(tot[3]),
                f"{(tot[3] // 6 - tot[2] // 6) / 10:+.1f} s ({100 * (tot[3] - tot[2]) / max(1, tot[2]):+.0f}%)"])
    if args.markdown:
        print("| " + " | ".join(head) + " |")
        print("|" + "---|" * len(head))
        for r in out:
            print("| " + " | ".join(r) + " |")
    else:
        w = [max(len(r[i]) for r in out + [head]) for i in range(len(head))]
        for r in [head] + out:
            print("  ".join(c.rjust(w[i]) if i else c.ljust(w[i]) for i, c in enumerate(r)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
