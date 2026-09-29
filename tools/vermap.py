#!/usr/bin/env python3
"""Where each us.v11 address is in another version, and the names that follow.

us.v11 is the reference version: its names (`func_80245000`, `D_802E8BD0`,
the address of the thing in us.v11) are the names of the same things in
every version, so that one C source compiles for all four.  This module
works out the correspondence and the naming rule built on it:

- In us.v11 nothing changes: a name's address is the one it ends in.
- In another version, `func_X`/`D_X`/`jtbl_X` is whatever sits where
  us.v11 has X, and a thing with no us.v11 counterpart keeps its own
  address with the version appended (`D_80301234_jp`).  tools/split.py
  names splat's symbols that way, tools/postsplit.py the ones it makes, and
  tools/link_syms.py reads original addresses back through the map.

The map (blastcorps/vermap.<version>.txt, written by `vermap.py <version>`,
see tools/regen_code_yaml.sh) is a list of ranges `us.v11 start, end,
version start`, found from the module binaries alone:

1. .text: both versions' code is cut into functions (a start after every
   return, and every jal target) and the two lists are aligned on their
   bodies with the linker-filled fields masked (difflib); where they differ
   the words are aligned inside the changed stretch.  The data islands
   (the regen script's --bin regions) pair up in order.
2. Every lui/lo pair and jal in aligned code gives a (us.v11 address,
   version address) pair.  Those in .data/.rodata and .bss are anchors:
   the pairs most code agrees on, rising in both versions.  Between two
   anchors the same distance apart in both versions the range maps as a
   whole; elsewhere .data/.rodata are aligned word by word (a pointer
   compared through the map), and .bss keeps only the anchors themselves.
   Words in aligned data that point anywhere are anchors too (a second
   round).  ROM offsets that code builds are single points.

Where the map guesses wrong (text that changed between versions can pair
with the wrong neighbour), --pair corrects it by hand.

Usage:
  vermap.py <version> [--layout <module>:<data>:<end>[:<bin off>/<len>,...]] ...
            [--pair <us.v11 addr>:<version addr>[:<len>]] ...
"""
import argparse
import bisect
import collections
import difflib
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
CODE = HERE.parent / "blastcorps"
REF = "us.v11"
MODULES = ("init", "hd_code", "hd_front_end")
TAGS = {"us.v10": "v10", "us.v11": "", "jp": "jp", "eu": "eu"}
ROM_LIMIT = 0x01000000
NAME = re.compile(r"^(\w+?)_([0-9A-F]{8})(?:_(v10|jp|eu))?$")


def tag(version):
    return TAGS[version]


# ---------------------------------------------------------------------------
# The map itself

class VerMap:
    """us.v11 -> version address ranges.  Identity for us.v11."""

    def __init__(self, version, ranges=()):
        self.version = version
        self.tag = TAGS[version]
        self.ranges = sorted(ranges)  # (ref start, ref end, target start)
        self.ref_starts = [r[0] for r in self.ranges]
        inv = sorted((t, t + (e - s), s) for s, e, t in self.ranges)
        self.tgt_starts = [r[0] for r in inv]
        self.inv = inv
        self.identity = version == REF
        self._ref_mods = None

    def to_target(self, a):
        """The version's address for us.v11 address a, or None."""
        if self.identity:
            return a
        i = bisect.bisect_right(self.ref_starts, a) - 1
        if i >= 0:
            s, e, t = self.ranges[i]
            if s <= a < e:
                return t + (a - s)
        return None

    def to_ref(self, x):
        """The us.v11 address for the version's address x, or None."""
        if self.identity:
            return x
        i = bisect.bisect_right(self.tgt_starts, x) - 1
        if i >= 0:
            s, e, r = self.inv[i]
            if s <= x < e:
                return r + (x - s)
        return None

    def ref_modules(self):
        if self._ref_mods is None:
            sys.path.insert(0, str(HERE))
            import modmap
            self._ref_mods = modmap.modules(REF, CODE)
        return self._ref_mods

    def movable_ref(self, a):
        """True if us.v11 address a is one the map translates: in a module
        (not its load address) or a ROM offset."""
        if a < ROM_LIMIT:
            return a >= 0x1000
        for m in self.ref_modules().values():
            if m.contains(a):
                return a != m.vram
        return False

    def name(self, prefix, x, movable):
        """The name for the version's address x (`movable`: whether x is in
        a module or a ROM offset, i.e. not the fixed memory map)."""
        if self.identity:
            return f"{prefix}_{x:08X}"
        a = self.to_ref(x)
        if a is not None:
            return f"{prefix}_{a:08X}"
        if not movable:
            return f"{prefix}_{x:08X}"
        return f"{prefix}_{x:08X}_{self.tag}"

    def original(self, name):
        """The version's address of a name, as the naming rule has it, or
        None if it names a us.v11 address the version doesn't have."""
        m = NAME.match(name)
        if not m:
            return None
        a = int(m.group(2), 16)
        if m.group(3):
            return a if m.group(3) == self.tag else None
        if self.identity:
            return a
        t = self.to_target(a)
        if t is not None:
            return t
        if a < ROM_LIMIT or not self.movable_ref(a):
            return a  # the fixed memory map, or a ROM offset no code pairs up
        return None

    def write(self, path, comment=""):
        lines = [f"# generated by tools/vermap.py: where us.v11's addresses are in {self.version}.",
                 "# us.v11 start, us.v11 end, start in this version (hex)"]
        if comment:
            lines += [f"# {c}" for c in comment.splitlines()]
        lines += [f"{s:08X} {e:08X} {t:08X}" for s, e, t in self.ranges]
        Path(path).write_text("\n".join(lines) + "\n")


_cache = {}


def load(version, base=CODE):
    """The map for version (identity for us.v11; empty if none is written)."""
    key = (version, str(base))
    if key not in _cache:
        ranges = []
        path = Path(base) / f"vermap.{version}.txt"
        if version != REF and path.exists():
            for line in path.read_text().splitlines():
                if line and not line.startswith("#"):
                    s, e, t = (int(x, 16) for x in line.split())
                    ranges.append((s, e, t))
        _cache[key] = VerMap(version, ranges)
    return _cache[key]


# ---------------------------------------------------------------------------
# Building it

# addiu, lb, lh, lw, lbu, lhu, sb, sh, sw, lwc1, ldc1, ld, swc1, sdc1, sd
LO_OPS = {0x09, 0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2B, 0x31, 0x35, 0x37, 0x39, 0x3D, 0x3F}


def mask(w):
    op = w >> 26
    if op in (2, 3):
        return w & 0xFC000000
    if op == 0x0F or op in LO_OPS:
        return w & 0xFFFF0000
    return w


class Layout:
    """One version's module: vram, the whole image, .text words, the
    islands in .text, and where .data and .bss start and end."""

    def __init__(self, mod, version, data=None, end=None, bins=None):
        yml = (CODE / f"{mod}.{version}.yaml").read_text()
        self.mod, self.version = mod, version
        self.vram = int(re.search(r"vram:\s*(0x[0-9A-Fa-f]+)", yml)[1], 16)
        self.blob = (CODE / f"{mod}.{version}.bin").read_bytes()
        subs = [(int(o, 16), t) for o, t in re.findall(r"- \[(0x[0-9A-Fa-f]+),\s*(\.?\w+)", yml)]
        if data is None:
            data = next((o for o, t in subs if t.lstrip(".") in ("linker", "data", "rodata")), None)
            if data is None:  # the unsplit configs: .data is one bin at the end
                data = [o for o, t in subs if t == "bin"][-1]
        if end is None:
            end = int(re.findall(r"^  - \[(0x[0-9A-Fa-f]+)\]", yml, re.M)[0], 16)
        self.data, self.end = data, end
        if bins is None:
            offs = [o for o, t in subs if t == "bin" and o < data]
            nxt = {o: next((p for p, _ in subs if p > o), data) for o in offs}
            bins = [(o, nxt[o] - o) for o in offs]
        # zero padding kept as a bin (hd_code's after createmesgqueue.o) is
        # no island: the other versions have the same zeros as code
        self.bins = sorted((o, n) for o, n in bins if any(self.blob[o:o + n]))
        n = data // 4
        self.words = struct.unpack(f">{n}I", self.blob[:4 * n])
        bss = re.findall(r"^  - \[(0x[0-9A-Fa-f]+)(?:, \.?bss, \S+)?\]", yml.split("\nbss:", 1)[1], re.M) \
            if "\nbss:" in yml else []
        self.bss = (int(bss[0], 16), int(bss[-1], 16)) if bss else None

    def in_bin(self, off):
        return any(o <= off < o + n for o, n in self.bins)

    def word(self, a):
        o = a - self.vram
        return struct.unpack(">I", self.blob[o:o + 4])[0]

    def chunks(self):
        """[(start, end, token)] word ranges of .text: one per function, one
        per island."""
        w = self.words
        starts = {0}
        for i, x in enumerate(w):
            if self.in_bin(4 * i):
                continue
            if x >> 26 == 3:
                t = (((self.vram & 0xF0000000) | ((x & 0x3FFFFFF) << 2)) - self.vram) // 4
                if 0 <= t < len(w):
                    starts.add(t)
            if x in (0x03E00008, 0x42000018) and i + 2 < len(w):
                j = i + 2
                while j < len(w) and w[j] == 0:
                    j += 1
                starts.add(j)
        for o, n in self.bins:
            starts.add(o // 4)
            starts.add((o + n) // 4)
        heads = {o // 4 for o, _ in self.bins}
        starts = sorted(s for s in starts if s < len(w) and (s in heads or not self.in_bin(4 * s)))
        out = []
        for k, s in enumerate(starts):
            e = starts[k + 1] if k + 1 < len(starts) else len(w)
            isl = next((i for i, (o, _) in enumerate(self.bins) if o == 4 * s), None)
            if isl is not None:
                tok = ("island", isl)
            else:
                tok = tuple(mask(x) for x in w[s:e])
            out.append((s, e, tok))
        return out


def shape(w):
    """An instruction without its registers and linker-filled fields: a
    changed function's registers are renumbered after the change."""
    op = w >> 26
    if op == 0:
        return (0, w & 0x3F, (w >> 6) & 31)
    if op in (2, 3) or op == 0x0F or op in LO_OPS:
        return (op,)
    if op == 1:
        return (1, (w >> 16) & 31)
    if op == 0x11:
        return (0x11, (w >> 21) & 31, w & 0x3F)
    return (op, w & 0xFFFF)


def writes(w):
    """The integer register an instruction writes, or None."""
    op = w >> 26
    if op == 0:
        return (w >> 11) & 31
    if op == 3:
        return 31
    if 0x08 <= op <= 0x0F or op in (0x18, 0x19, 0x1A, 0x1B, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x37):
        return (w >> 16) & 31
    if op == 0x11 and (w >> 21) & 31 in (0, 1, 2):  # mfc1, dmfc1, cfc1
        return (w >> 16) & 31
    return None


def lo_pairs(words):
    """{index of a lo instruction: (index of its lui, address)}: the loads,
    stores and addius based on a lui's register until something else
    writes it."""
    out = {}
    for i, w in enumerate(words):
        if w >> 26 != 0x0F:
            continue
        reg, hi = (w >> 16) & 31, w & 0xFFFF
        if reg == 0:
            continue
        for j in range(i + 1, min(i + 32, len(words))):
            x = words[j]
            if x >> 26 in LO_OPS and (x >> 21) & 31 == reg:
                lo = x & 0xFFFF
                out[j] = (i, ((hi << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF)
            if x >> 26 == 0 and x & 0x3F in (0x21, 0x2D) and (x >> 11) & 31 == reg \
                    and reg in ((x >> 21) & 31, (x >> 16) & 31):
                continue  # addu reg, reg, index: still the address's base
            if writes(x) == reg or x == 0x03E00008 or x >> 26 == 2:
                break
    return out


def align_text(ref, tgt, min_block=4):
    """{ref word index: target word index} for .text.

    Functions pair up on their masked bodies, in order first and then, for
    what is left, again among the leftovers: an object can sit elsewhere in
    another version's link order.  Between paired functions the words of
    the unpaired ones are aligned directly."""
    rc, tc = ref.chunks(), tgt.chunks()
    rt, tt = [c[2] for c in rc], [c[2] for c in tc]
    cp = {}  # ref chunk -> target chunk
    sm = difflib.SequenceMatcher(None, rt, tt, autojunk=False)
    blocks = [b for b in sm.get_matching_blocks() if b.size]
    size = lambda b: sum(rc[b.a + k][1] - rc[b.a + k][0] for k in range(b.size))
    delta = lambda b: tc[b.b][0] - rc[b.a][0]
    big = [b for b in blocks if size(b) >= 16]
    for b in blocks:
        if size(b) < 16:
            # a few small functions alike (an epilogue, `jr $ra`) are only
            # kept where they sit as the code around them does
            before = [x for x in big if x.a < b.a]
            after = [x for x in big if x.a > b.a]
            near = ([delta(before[-1])] if before else []) + ([delta(after[0])] if after else [])
            if delta(b) not in near:
                continue
        for k in range(b.size):
            cp[b.a + k] = b.b + k
    while True:
        rl = [i for i in range(len(rc)) if i not in cp]
        used = set(cp.values())
        tl = [j for j in range(len(tc)) if j not in used]
        sm = difflib.SequenceMatcher(None, [rt[i] for i in rl], [tt[j] for j in tl], autojunk=False)
        new = 0
        for blk in sm.get_matching_blocks():
            words = sum(rc[rl[blk.a + k]][1] - rc[rl[blk.a + k]][0] for k in range(blk.size))
            if words < 32:
                continue
            for k in range(blk.size):
                cp[rl[blk.a + k]] = tl[blk.b + k]
            new += 1
        if not new:
            break
    pairs = {}
    for i, j in cp.items():
        a, b = rc[i], tc[j]
        if a[2][0] == "island":
            continue
        for k in range(a[1] - a[0]):
            pairs[a[0] + k] = b[0] + k
    # the changed stretches: the words between two paired ones, where both
    # versions have some
    for rnd in range(3):
        key = mask if rnd < 2 else shape
        pts = sorted(pairs.items())
        pts = [(-1, -1)] + pts + [(len(ref.words), len(tgt.words))]
        for (a1, b1), (a2, b2) in zip(pts, pts[1:]):
            if a2 - a1 < 2 or b2 - b1 < 2:
                continue
            rs, re_, ts, te = a1 + 1, a2, b1 + 1, b2
            if ref.in_bin(4 * rs) or tgt.in_bin(4 * ts):
                continue
            ra = [key(x) for x in ref.words[rs:re_]]
            ta = [key(x) for x in tgt.words[ts:te]]
            if len(ra) * len(ta) > 4e8:
                print(f"vermap: {ref.mod}: skipping a changed stretch of {len(ra)}/{len(ta)} words",
                      file=sys.stderr)
                continue
            inner = difflib.SequenceMatcher(None, ra, ta, autojunk=False)
            # by shape only where the stretch is the same code with its
            # registers renumbered, not code that was rewritten
            if key is shape and inner.ratio() < 0.9:
                continue
            near = (ts - rs, te - re_)
            for blk in inner.get_matching_blocks():
                if blk.size >= 16 or (blk.size >= min_block and blk.b + ts - blk.a - rs in near):
                    for k in range(blk.size):
                        pairs[rs + blk.a + k] = ts + blk.b + k
    # islands, in order, where both versions have them the same size
    for (ro, rn), (to, tn) in zip(ref.bins, tgt.bins):
        if rn == tn:
            for k in range(rn // 4):
                pairs[ro // 4 + k] = to // 4 + k
    return pairs


def runs(pairs):
    """Ranges (ref start, ref end, target start) of consecutive pairs."""
    out = []
    for a in sorted(pairs):
        b = pairs[a]
        if out and out[-1][1] == a and out[-1][2] + (a - out[-1][0]) == b:
            out[-1][1] = a + 1
        else:
            out.append([a, a + 1, b])
    return [tuple(r) for r in out]


def code_pairs(ref, tgt, wpairs):
    """(ref address, target address) from aligned lui/lo pairs and jals."""
    rlo, tlo = lo_pairs(ref.words), lo_pairs(tgt.words)
    out = []
    for i, j in wpairs.items():
        if ref.in_bin(4 * i):
            continue
        if i in rlo and j in tlo and wpairs.get(rlo[i][0]) == tlo[j][0]:
            out.append((rlo[i][1], tlo[j][1]))
        w, x = ref.words[i], tgt.words[j]
        if w >> 26 == 3 and x >> 26 == 3:
            out.append((((ref.vram & 0xF0000000) | ((w & 0x3FFFFFF) << 2)),
                        ((tgt.vram & 0xF0000000) | ((x & 0x3FFFFFF) << 2))))
    return out


def anchors(pairs, lo, hi):
    """The (ref, target) pairs in [lo, hi) that most references agree on.
    In ref address order, an anchor stays if its distance (target - ref)
    is a neighbour's, or if its target falls between its neighbours': a
    block can move (another link order), but a lone anchor out of place is
    a reference that only looks like one."""
    votes = collections.defaultdict(collections.Counter)
    for a, b in pairs:
        if lo <= a < hi:
            votes[a][b] += 1
    best = {a: c.most_common(1)[0][0] for a, c in votes.items()}
    by_t = collections.defaultdict(list)
    for a, b in best.items():
        by_t[b].append(a)
    pts = sorted((a, b) for a, b in best.items() if len(by_t[b]) == 1)
    while True:
        keep = []
        for k, (a, b) in enumerate(pts):
            d = b - a
            prev = pts[k - 1] if k else None
            nxt = pts[k + 1] if k + 1 < len(pts) else None
            if (prev and prev[1] - prev[0] == d) or (nxt and nxt[1] - nxt[0] == d):
                keep.append((a, b))
            elif (prev is None or prev[1] < b) and (nxt is None or b < nxt[1]):
                keep.append((a, b))
        if len(keep) == len(pts):
            return keep
        pts = keep


def fill(anch, lo, hi, tlo, thi, compare=None, first=None):
    """Ranges between anchors: whole where the gap is the same size in both
    versions, else by compare(ref lo, ref hi, target lo, target hi) (a
    list of ranges) if given, else just the anchors.  (lo, tlo) and (hi,
    thi) bound the section, where the anchors next to them agree (in
    another link order the first object can be another one)."""
    inner = [p for p in anch if lo < p[0] < hi and tlo < p[1] < thi]
    pts = inner
    # first: whether both versions link the same object first
    if first if first is not None else (not inner or inner[0][1] - inner[0][0] == tlo - lo):
        pts = [(lo, tlo)] + pts
    if not inner or inner[-1][1] - inner[-1][0] == thi - hi:
        pts = pts + [(hi, thi)]
    out = []
    for (a1, b1), (a2, b2) in zip(pts, pts[1:]):
        if a2 - a1 == b2 - b1:
            out.append((a1, a2, b1))
        else:
            out.append((a1, a1 + 1, b1))
            if compare:
                out += compare(a1, a2, b1, b2)
    return out


def merge(ranges):
    """Ranges joined where they continue each other; where two overlap
    (in us.v11 or in the version), the earlier-listed longer one wins."""
    out = []
    for s, e, t in sorted(set(ranges), key=lambda r: (r[0], -(r[1] - r[0]))):
        if out and s < out[-1][1]:
            ps, pe, pt = out[-1]
            if t - s == pt - ps:
                out[-1][1] = max(pe, e)
                continue
            t += pe - s
            s = pe
            if s >= e:
                continue
        if out and out[-1][1] == s and out[-1][2] + (s - out[-1][0]) == t:
            out[-1][1] = e
        else:
            out.append([s, e, t])
    # the version's side must not overlap either
    res, taken = [], []
    for s, e, t in sorted(out, key=lambda r: -(r[1] - r[0])):
        te = t + (e - s)
        k = bisect.bisect_left(taken, (t,))
        clash = any(x < te and t < y for x, y in taken[max(0, k - 1):k + 2])
        if not clash:
            bisect.insort(taken, (t, te))
            res.append((s, e, t))
    return sorted(res)


def build(version, layouts, manual=()):
    """The VerMap for version.  layouts: {module: (data, end, bins)} for
    the version (None: as its config has it).  manual: (ref start, ref end,
    target start) ranges given by hand, which win over what's found."""
    refs = {m: Layout(m, REF) for m in MODULES}
    tgts = {m: Layout(m, version, *(layouts.get(m) or ())) for m in MODULES}
    ranges, pairs = [], []
    for m in MODULES:
        r, t = refs[m], tgts[m]
        wp = align_text(r, t)
        ranges += [(r.vram + 4 * a, r.vram + 4 * b, t.vram + 4 * c) for a, b, c in runs(wp)]
        pairs += code_pairs(r, t, wp)
    # a call into a function that changed: its start, as a point
    known = VerMap(version, merge(ranges))
    calls = collections.defaultdict(collections.Counter)
    for m in MODULES:
        for a, b in pairs:
            if refs[m].vram <= a < refs[m].vram + refs[m].data and \
                    tgts[m].vram <= b < tgts[m].vram + tgts[m].data:
                calls[a][b] += 1
    for a, c in calls.items():
        b = c.most_common(1)[0][0]
        if known.to_target(a) is None and known.to_ref(b) is None:
            ranges.append((a, a + 1, b))
    text = VerMap(version, merge(ranges))

    def same_first(m):
        return text.to_target(refs[m].vram) == tgts[m].vram

    def data_ranges(m, extra):
        r, t = refs[m], tgts[m]
        lo, hi = r.vram + r.data, r.vram + r.end
        tlo, thi = t.vram + t.data, t.vram + t.end
        anch = anchors(pairs + extra, lo, hi)

        def compare(a1, a2, b1, b2):
            # a pointer is a pointer: what it points at moves between versions
            norm = lambda w: "ptr" if 0x80000000 <= w < 0x80400000 else w
            ra = [norm(r.word(a)) for a in range(a1 & ~3, a2 & ~3, 4)]
            ta = [norm(t.word(b)) for b in range(b1 & ~3, b2 & ~3, 4)]
            sm = difflib.SequenceMatcher(None, ra, ta, autojunk=False)
            return [((a1 & ~3) + 4 * blk.a, (a1 & ~3) + 4 * (blk.a + blk.size), (b1 & ~3) + 4 * blk.b)
                    for blk in sm.get_matching_blocks() if blk.size >= 2]
        return fill(anch, lo, hi, tlo, thi, compare, same_first(m))

    def bss_ranges(m, extra):
        r, t = refs[m], tgts[m]
        if r.bss is None:
            return []
        lo, hi = r.bss
        tlo = t.vram + t.end
        anch = anchors(pairs + extra, lo, hi + 1)
        # the end is referenced as the heap's start: an anchor like any other
        end = next((b for a, b in anch if a == hi), None)
        anch = [p for p in anch if p[0] < hi]
        if end is None:
            # nothing refers to the end: the last object's .bss is taken to
            # be the same size in both
            a, b = anch[-1] if anch else (lo, tlo)
            end = hi + (b - a)
        return fill(anch, lo, hi, tlo, end, None, same_first(m)) + [(hi, hi + 1, end)]

    cur = text
    extra = []
    for _ in range(2):
        ranges2 = list(text.ranges)
        for m in MODULES:
            ranges2 += data_ranges(m, extra)
            ranges2 += bss_ranges(m, extra)
        cur = VerMap(version, merge(ranges2))
        # pointers in aligned .data: more pairs for the next round
        extra = []
        for m in MODULES:
            r, t = refs[m], tgts[m]
            for a in range(r.vram + r.data, r.vram + r.end - 3, 4):
                b = cur.to_target(a)
                if b is None or (b - t.vram) + 4 > len(t.blob) or a % 4 or b % 4:
                    continue
                w, x = r.word(a), t.word(b)
                if w >= 0x80000000 and x >= 0x80000000:
                    extra.append((w, x))
    # ROM offsets built by code: single points
    rom = collections.defaultdict(collections.Counter)
    for a, b in pairs:
        if 0x1000 <= a < ROM_LIMIT and 0x1000 <= b < ROM_LIMIT:
            rom[a][b] += 1
    points = [(a, a + 1, c.most_common(1)[0][0]) for a, c in rom.items()]
    found = merge(list(cur.ranges) + points)
    # a correction wins: cut what it overlaps (on either side) out of the rest
    for ms, me, mt in manual:
        cut = []
        for s, e, t in found:
            pieces = [(s, e, t)]
            for lo, hi in ((ms, me), (s + (mt - t), s + (mt - t) + (me - ms))):
                nxt = []
                for a, b, c in pieces:
                    if b <= lo or hi <= a:
                        nxt.append((a, b, c))
                        continue
                    if a < lo:
                        nxt.append((a, lo, c))
                    if hi < b:
                        nxt.append((hi, b, c + (hi - a)))
                pieces = nxt
            cut += pieces
        found = cut
    return VerMap(version, merge(found + list(manual)))


UNDEF_MARKER = "/* ---- generated by tools/vermap.py from us.v11's file below this line; edits there are lost ---- */"
ASSIGN = re.compile(r"^(\s*)([\w.$]+)(\s*=\s*)(0x[0-9A-Fa-f]+)(\s*;.*)$")


def port_undefined(vm, mod):
    """undefined_syms.<mod>.<version>.txt: the names only C uses (see
    tools/postsplit.py), us.v11's list moved through the map.  Lines above
    the marker are the version's own."""
    src = CODE / f"undefined_syms.{mod}.{REF}.txt"
    dst = CODE / f"undefined_syms.{mod}.{vm.version}.txt"
    manual = []
    if dst.exists():
        text = dst.read_text().splitlines()
        manual = text[:text.index(UNDEF_MARKER)] if UNDEF_MARKER in text else []
    out = manual + [UNDEF_MARKER]
    missing = 0
    for line in src.read_text().splitlines() if src.exists() else []:
        m = ASSIGN.match(line)
        if not m:
            out.append(line)
            continue
        a = int(m.group(4), 16)
        t = a if not vm.movable_ref(a) or "fixed" in m.group(5) else vm.to_target(a)
        if t is None:
            out.append(f"/* {m.group(2)}: not in {vm.version} */")
            missing += 1
            continue
        out.append(f"{m.group(1)}{m.group(2)}{m.group(3)}0x{t:08X}{m.group(5)}")
    dst.write_text("\n".join(out) + "\n")
    return missing


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("version")
    ap.add_argument("--layout", action="append", default=[],
                    help="module:data:end[:binoff/len,...] (hex) for this version")
    ap.add_argument("--pair", action="append", default=[], metavar="REF:TARGET[:LEN]",
                    help="us.v11 address REF is TARGET in this version (for LEN bytes, default 1): "
                    "a correction where the map guesses wrong")
    ap.add_argument("-o", "--out")
    args = ap.parse_args()
    layouts = {}
    for spec in args.layout:
        parts = spec.split(":")
        bins = None
        if len(parts) > 3 and parts[3]:
            bins = [tuple(int(x, 16) for x in b.split("/")) for b in parts[3].split(",")]
        layouts[parts[0]] = (int(parts[1], 16), int(parts[2], 16), bins)
    manual = []
    for spec in args.pair:
        p = [int(x, 16) for x in spec.split(":")]
        manual.append((p[0], p[0] + (p[2] if len(p) > 2 else 1), p[1]))
    vm = build(args.version, layouts, manual)
    out = args.out or CODE / f"vermap.{args.version}.txt"
    vm.write(out)
    n = sum(e - s for s, e, _ in vm.ranges)
    print(f"{Path(out).name}: {len(vm.ranges)} ranges, 0x{n:X} bytes")
    if not args.out:
        for m in MODULES:
            missing = port_undefined(vm, m)
            if missing:
                print(f"undefined_syms.{m}.{args.version}.txt: {missing} names not in the map")


if __name__ == "__main__":
    main()
