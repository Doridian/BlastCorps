#!/usr/bin/env python3
"""Compare two gameplay digests (PORT_DIGEST=FILE, port/host/digest.c;
docs/PORT.md, "The gameplay digest").

    port/tools/digest_cmp.py A B [--all] [--max N]
    port/tools/digest_cmp.py --hash A
    port/tools/digest_cmp.py --levels A

A digest has a line per controller poll: the poll, the game's mode, the
level, the game's frame in the mode, and the fields that changed since the
last line (a line marked * has all of them).  Two runs are aligned by what
the game did, not by when: the modes and levels in order (a mode the one
run visits and the other doesn't is reported), and within a mode by the
game's frame (a stretch of frames each time the count starts again).  So
runs that took different numbers of retraces (the movie's lag frames, a
build's own pace) line up frame by frame.

What's compared, by the fields' kind:

  gameplay  everything but the below: the player's and every vehicle's
            position, headings and speed, the buildings (destroyed, where,
            each group's damage), the RDUs, crates, blocks and ammo boxes,
            the level's stats and totals, won and lost, the medals, units
            and game state.  In a level, the first frame where one differs
            is a FAILURE, with its field; after that the level isn't
            compared further (everything follows from it).  At the end of
            every mode (a level, a menu) the outcome is compared too: the
            stats, won/lost, the medals, units and game state.
  clock     clk (retraces), t ("TIME IN LEVEL"'s retraces), tc (the level's
            time): reported as each level's time against the other's,
            never a failure.
  internal  rng (the random state), fc (a frame count): reported where it
            first differs while the gameplay doesn't, never a failure.

--hash prints a hash of the gameplay alone, the same for two runs that play
the same (whatever their timing): what port/tools/test.py compares with
test_refs.json.  --levels lists the levels a digest played, with frames and
times.

Exit status: 0 the same gameplay, 1 a difference in it, 2 bad input.
"""
import argparse
import difflib
import hashlib
import sys

CLOCK = {"clk", "t", "tc"}
INTERNAL = {"rng", "fc"}
# the outcome of a mode, compared at its end whatever its length
OUTCOME = ("st", "won", "lost", "of", "med", "units", "gs")
FRONT_END_MODES = 0xC9FD8FE7DBFF8080 | 0x4000 | 0x20000000 | 0x30     # digest.c's

LEVELS = ["Simian Acres", "Angel City", "Outland Farm", "Blackridge Works", "Glory Crossing",
          "Shuttle Gully", "Salvage Wharf", "Skyfall", "Twilight Foundry", "Crystal Rift", "Argent Towers",
          "Skerries", "Diamond Sands", "Ebony Coast", "Oyster Harbor", "Carrick Point", "Havoc District",
          "Ironstone Mine", "Beeton Tracks", "J-Bomb", "Jade Plateau", "Marine Quarter", "Cooter Creek",
          "Gibbons Gate", "Baboon Catacomb", "Sleek Streets", "Obsidian Mile", "Corvine Bluff", "Sideswipe",
          "Echo Marches", "Kipling Plant", "Falchion Field", "Morgan Hall", "Tempest City", "Orion Plaza",
          "Glanders Ranch", "Dagger Pass", "Geode Square", "Shuttle Island", "Mica Park", "Moon",
          "Cobalt Quarry", "Moraine Chase", "Mercury", "Venus", "Mars", "Neptune", "CMO intro",
          "Silver Junction", "end sequence", "Shuttle clear", "Dark Heartland", "Magma Peak", "Thunderfist",
          "Saline Watch", "Backlash", "Bison Ridge", "Ember Hamlet", "Cromlech Court", "Lizard Island"]


def kind(key):
    return "clock" if key in CLOCK else "internal" if key in INTERNAL else "gameplay"


class Segment:
    """a stretch of one mode at one level: its lines as (epoch, frame,
    poll, changes)"""

    def __init__(self, mode, level, index):
        self.mode, self.level, self.index = mode, level, index
        self.lines = []
        self.end = {}           # the state at its last line

    @property
    def in_level(self):
        return self.mode != 0 and not (self.mode & FRONT_END_MODES)

    def name(self):
        lv = LEVELS[self.level] if 0 <= self.level < len(LEVELS) else f"level {self.level}"
        what = lv if self.in_level else "menus"
        return f"{what} (mode {self.mode:X}, level {self.level}, #{self.index})"

    def states(self):
        """(epoch, frame, poll, state) per line; the state is one dict,
        updated in place"""
        st = {}
        for epoch, frame, poll, full, ch in self.lines:
            if full:
                st.clear()
            for k, v in ch.items():
                if v == "~":
                    st.pop(k, None)
                else:
                    st[k] = v
            yield epoch, frame, poll, st


def parse(path):
    segs = []
    seg = None
    epoch = last_frame = 0
    with open(path, errors="replace") as f:
        for n, line in enumerate(f, 1):
            if line.startswith("#") or not line.strip():
                continue
            p = line.split()
            try:
                poll, mode, level, frame = int(p[0]), int(p[1], 16), int(p[2]), int(p[3])
            except (IndexError, ValueError):
                raise SystemExit(f"{path}:{n}: not a digest line")
            full = len(p) > 4 and p[4] == "*"
            ch = dict(x.split("=", 1) for x in p[5 if full else 4:] if "=" in x)
            if seg is None or full and (mode, level) != (seg.mode, seg.level):
                seg = Segment(mode, level, len(segs))
                segs.append(seg)
                epoch, last_frame = 0, frame
            elif (mode, level) != (seg.mode, seg.level):
                raise SystemExit(f"{path}:{n}: a new mode without a full line")
            if frame < last_frame:
                epoch += 1
            last_frame = frame
            seg.lines.append((epoch, frame, poll, full, ch))
    for s in segs:
        st = {}
        for _, _, _, st in s.states():
            pass
        s.end = dict(st)
    return segs


def frames(seg):
    """((epoch, frame), poll, state) at the last line of each frame, in
    order (the state is updated in place)"""
    lines = seg.lines
    for i, (epoch, frame, poll, st) in enumerate(seg.states()):
        if i + 1 == len(lines) or lines[i + 1][:2] != (epoch, frame):
            yield (epoch, frame), poll, st


def compare_segment(a, b, report, cut=False):
    """one aligned pair; returns the first gameplay difference or None.
    cut: a run ends in it (--frames), so its outcome isn't compared"""
    first = internal = None
    n = only_a = only_b = 0
    clock = clock0 = [None] * 4
    ga, gb = frames(a), frames(b)
    x, y = next(ga, None), next(gb, None)
    while x is not None and y is not None:
        if x[0] < y[0]:
            only_a += 1
            x = next(ga, None)
            continue
        if y[0] < x[0]:
            only_b += 1
            y = next(gb, None)
            continue
        n += 1
        (key, pa, fa), (_, pb, fb) = x, y
        clock = [fa.get("clk"), fb.get("clk"), fa.get("t"), fb.get("t")]
        if n == 1:
            clock0 = clock
        if a.in_level and first is None:
            for k in sorted(set(fa) | set(fb)):
                va, vb = fa.get(k), fb.get(k)
                if va == vb:
                    continue
                kd = kind(k)
                if kd == "gameplay":
                    first = (key, pa, pb, k, va, vb)
                    break
                if kd == "internal" and internal is None:
                    internal = (key, pa, pb, k, va, vb)
        x, y = next(ga, None), next(gb, None)
    only_a += sum(1 for _ in ga) + (x is not None)
    only_b += sum(1 for _ in gb) + (y is not None)
    if first is None and not cut:
        for k in OUTCOME:
            va, vb = a.end.get(k), b.end.get(k)
            if va != vb:
                la, lb = a.lines[-1], b.lines[-1]
                first = ("end", la[2], lb[2], k, va, vb)
                break
    # the clock over the frames both have: retraces from the first to the
    # last, and "TIME IN LEVEL" at the last
    span = None
    if n and None not in clock0[:2] + clock[:2]:
        span = (int(clock[0]) - int(clock0[0]), int(clock[1]) - int(clock0[1]), clock[2], clock[3])
    report.append(dict(a=a, b=b, first=first, internal=internal, n=n, only_a=only_a, only_b=only_b,
                       span=span, cut=cut))
    return first


def times(seg):
    """(frames, retraces, TIME IN LEVEL at the end, tc at the end)"""
    clk0 = clk1 = None
    for _, _, _, st in seg.states():
        if "clk" in st:
            if clk0 is None:
                clk0 = int(st["clk"])
            clk1 = int(st["clk"])
    return (len(seg.lines), (clk1 - clk0) if clk0 is not None else None,
            seg.end.get("t"), seg.end.get("tc"))


def fmt_diff(d):
    key, pa, pb, k, va, vb = d
    where = "at its end" if key == "end" else f"frame {key[1]}" + (f" (count restarted {key[0]}x)" if key[0] else "")
    return f"{where} (poll {pa} against {pb}): {k} = {va} against {vb}"


def gameplay_hash(segs):
    """the gameplay alone: each mode and level in order; in a level, each
    frame's gameplay changes (a frame with none adds nothing, so a level
    that waits longer at its end hashes the same); every mode's outcome"""
    h = hashlib.sha1()
    for s in segs:
        h.update(f"S {s.mode:X} {s.level}\n".encode())
        if s.in_level:
            for epoch, frame, poll, full, ch in s.lines:
                g = sorted((k, v) for k, v in ch.items() if kind(k) == "gameplay")
                if g:
                    h.update(f"{epoch} {frame} {g}\n".encode())
        h.update(f"E {[(k, s.end.get(k)) for k in OUTCOME]}\n".encode())
    return h.hexdigest()[:16]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    ap.add_argument("a")
    ap.add_argument("b", nargs="?")
    ap.add_argument("--hash", action="store_true", help="print A's gameplay hash")
    ap.add_argument("--levels", action="store_true", help="list A's levels, with frames and times")
    ap.add_argument("--all", action="store_true", help="every level's first difference, not only the first")
    ap.add_argument("--max", type=int, default=10, help="differences listed at most (default 10)")
    ap.add_argument("-q", "--quiet", action="store_true", help="only the verdict and the first difference")
    args = ap.parse_args()
    sa = parse(args.a)
    if args.hash:
        print(gameplay_hash(sa))
        return 0
    if args.levels:
        for s in sa:
            if s.in_level:
                fr, rt, t, tc = times(s)
                print(f"{s.name()}: {fr} frames, {rt} retraces, TIME IN LEVEL {t}, tc {tc}, "
                      + ", ".join(f"{k} {s.end.get(k)}" for k in ("st", "won", "lost")))
        return 0
    if not args.b:
        ap.error("two digests to compare (or --hash, --levels)")
    sb = parse(args.b)
    ka = [(s.mode, s.level) for s in sa]
    kb = [(s.mode, s.level) for s in sb]
    report, missing = [], []
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, ka, kb, autojunk=False).get_opcodes():
        if op == "equal":
            for i, j in zip(range(i1, i2), range(j1, j2)):
                compare_segment(sa[i], sb[j], report, i == len(sa) - 1 or j == len(sb) - 1)
        else:
            missing += [("A", s) for s in sa[i1:i2]] + [("B", s) for s in sb[j1:j2]]
    fails = [r for r in report if r["first"]]
    lost = [(w, s) for w, s in missing if s.in_level]
    levels = [r for r in report if r["a"].in_level]
    print(f"digest_cmp: {len(report)} modes aligned ({len(levels)} in a level), "
          f"{len(missing)} in one run only ({len(lost)} of them levels)")
    for r in fails[:args.max if args.all else 1]:
        print(f"DIFFERS  {r['a'].name()}, {fmt_diff(r['first'])}")
    for w, s in lost[:args.max]:
        print(f"DIFFERS  {s.name()} only in {w} (poll {s.lines[0][2]})")
    if not args.quiet:
        for r in report:
            if r["internal"] and not r["first"]:
                print(f"note     {r['a'].name()}: internal state differs from {fmt_diff(r['internal'])}")
        for w, s in missing:
            if not s.in_level:
                print(f"note     {s.name()} only in {w} (poll {s.lines[0][2]}, {len(s.lines)} frames)")
        # the clock: each level's time, over the frames both runs have
        rows = [r for r in levels if r["span"] and (r["span"][0] != r["span"][1] or r["span"][2] != r["span"][3]
                                                    or r["only_a"] or r["only_b"])]
        if rows:
            print("note     the clock, A against B: per level, the frames both have, the retraces they took, "
                  "TIME IN LEVEL at the last of them; the frames only one has; the level's time (tc) at the end")
            total = 0
            for r in rows[:args.max * 5]:
                ra, rb, ta, tb = r["span"]
                total += rb - ra
                print(f"           {r['a'].name()}: {r['n']} frames, {ra}/{rb} retraces ({rb - ra:+d}), "
                      f"t {ta}/{tb}; {r['only_a']}/{r['only_b']} frames in one only; "
                      f"tc {r['a'].end.get('tc')}/{r['b'].end.get('tc')}" + (" (a run ends here)" if r["cut"] else ""))
            print(f"           {len(rows)} levels' times differ, {total:+d} retraces in all")
    if fails or lost:
        print(f"FAIL     the gameplay differs in {len(fails)} modes" + (f", {len(lost)} levels in one run only" if lost else ""))
        return 1
    print(f"same     gameplay: {sum(r['n'] for r in levels)} level frames compared")
    return 0


if __name__ == "__main__":
    sys.exit(main())
