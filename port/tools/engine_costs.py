#!/usr/bin/env python3
"""The engine's charges for the CPU model, one a function (O3, docs/PORT.md
"The engine made readable"): measure the blocks a run charges, then replace
a file's block-by-block ENGINE_BLKs by one ENGINE_COST(addr, n) at each
function's entry, n its original's average instructions a call.

  engine_costs.py report COUNTS... [--files F.c ...]
  engine_costs.py rewrite COUNTS... --files F.c ...
  engine_costs.py extra COUNTS... ENTRY BLK [BLK...]

COUNTS: the files a counting build wrote, one "id count" line a block.  The
counting build is a PORT_BLKLOG one (-DPORT_BLKLOG=ON) whose port_blklog()
(port/host/engine.c) counts instead of logging:

    static unsigned long long *cnt;
    static void cnt_dump(void) { ... fprintf(f, "%u %llu\\n", id, cnt[id]) for each id ... }
    void port_blklog(unsigned id) {
        if (!cnt) { cnt = calloc(0x10000, 8); atexit(cnt_dump); }
        if (id < 0x10000) cnt[id]++;
    }

(writing to an environment-named file a process; test.py passes on a
variable whose name doesn't start with PORT_).  Run it over the TAS
(test.py tas) and the quick tier, then:

- report: each function of the files (default: every port/engine/*.c)
  with its calls (its first block's count), its average and its blocks'
  static size; ENTRY? marks a function whose first charge isn't
  unconditional (the average would be wrong), DUP a block charged in two
  functions; the totals before and after, which should agree closely.
- rewrite: the files' first ENGINE_BLK in a function becomes
  ENGINE_COST(addr, average); its others become empty statements, and the
  lone `;` lines that leaves are taken out where that is safe (FLAG lines
  name the rest, after an `else` or an unbraced `if`, to fix by hand).
  A function no run reaches gets half its blocks' size.  Blocks a macro
  charges (ENGINE_DIV, a file's own) are left; fold them with `extra`.
- extra: what blocks BLK... (a macro's, say) add to function ENTRY's
  average, to add to its ENGINE_COST by hand when the macro goes.

The ids are the build's version's (blastcorps/build/recomp/src/
engine_blocks.h); the averages are instructions, so one version's do for
the others.  Variant macros naming a block per version (69BB0's
BLK_V10(v11, v10) and the like) are read as us.v10's: list them in
SITE_RE.
"""
import collections
import glob
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ENG = os.path.join(ROOT, "port", "engine")
BLOCKS = os.path.join(ROOT, "blastcorps", "build", "recomp", "src", "engine_blocks.h")

# a charge: ENGINE_BLK(addr), or a version macro's (v11, v10) / (us, jp)
SITE_RE = re.compile(r"\b(ENGINE_BLK|BLK_V10|A8CCC_BLK|B9C50_BLK)\(([0-9A-F]{8})(?:,\s*([0-9A-F]{8}))?\)")
CTRL_RE = re.compile(r"\b(if|for|while|switch|goto|return|do)\b|^\s*\w+:\s*$|\?")


def functions(path):
    """a file's function definitions at column 0: (name, line of the {, line of the })"""
    lines = open(path).read().split("\n")
    out = []
    i, n = 0, len(lines)
    while i < n:
        l = lines[i]
        if l.startswith(("#", " ", "\t", "/*", " *")) or not l.strip():
            i += 1
            continue
        j, text = i, l
        while not text.rstrip().endswith(("{", ";")) and j + 1 < n and not lines[j + 1].startswith("#"):
            j += 1
            text += " " + lines[j]
        if text.rstrip().endswith("{") and "(" in text and \
                not re.match(r"^\s*(typedef|struct|union|enum|static const|const)\b", text) and \
                "=" not in text.split("(")[0]:
            m = re.search(r"(\w+)\s*\(", re.sub(r"^REGS\([^)]*\)", "", text))
            k = j + 1
            while k < n and lines[k] != "}":
                k += 1
            out.append((m.group(1), j, k))
            i = k + 1
            continue
        i = j + 1
    return lines, out


def load_blocks():
    ids, sizes = {}, {}
    for l in open(BLOCKS):
        m = re.match(r"#define ENGINE_BLK_([0-9A-F]{8}\w*) (\d+), (\d+)", l)
        if m:
            ids[m.group(1)] = int(m.group(2))
            sizes[m.group(1)] = int(m.group(3))
    return ids, sizes


def load_counts(paths):
    c = collections.Counter()
    for p in paths:
        for f in (glob.glob(os.path.join(p, "*")) if os.path.isdir(p) else [p]):
            for l in open(f):
                a, b = l.split()
                c[int(a)] += int(b)
    return c


def count_name(m):
    mac, a, b = m.group(1), m.group(2), m.group(3)
    if mac in ("BLK_V10", "A8CCC_BLK"):
        return b
    return a


def analyse(files, counts):
    ids, sizes = load_blocks()
    res, owner = {}, collections.defaultdict(set)
    for fn in files:
        lines, funcs = functions(fn)
        for name, s, e in funcs:
            sites = [(k, m) for k in range(s, e) for m in SITE_RE.finditer(lines[k])]
            if not sites:
                continue
            names_all = [count_name(m) for _, m in sites]
            names = [x for x in names_all if x in ids]
            if not names:
                continue
            for x in set(names):
                owner[x].add((fn, name))
            first = sites[0][0]
            pre = re.sub(r"/\*.*?\*/", "", "\n".join(lines[s + 1:first]), flags=re.S)
            ok = not CTRL_RE.search(pre) and names_all[0] == names[0]
            entry, calls = names[0], counts[ids[names[0]]]
            tot = sum(counts[ids[x]] * sizes[x] for x in set(names))
            static = sum(sizes[x] for x in set(names))
            cost = max(1, round(tot / calls)) if calls else max(sizes[entry], round(static / 2))
            res[(fn, name)] = dict(sites=sites, entry=entry, ok=ok, calls=calls, cost=cost, static=static, total=tot)
    dup = {x: o for x, o in owner.items() if len(o) > 1}
    return res, dup


def rewrite(res):
    byfile = collections.defaultdict(list)
    for (fn, _), r in res.items():
        byfile[fn].append(r)
    flags = []
    for fn, rs in byfile.items():
        lines = open(fn).read().split("\n")
        for r in rs:
            for idx, (k, m) in enumerate(r["sites"]):
                txt = m.group(0)
                if idx == 0:
                    lines[k] = lines[k].replace(txt, "ENGINE_COST(%s, %d)" % (r["entry"], r["cost"]), 1)
                else:
                    lines[k] = lines[k].replace(txt + ";", ";", 1)
        out = []
        for i, l in enumerate(lines):
            if l.strip() == ";":
                prev = next((x for x in reversed(out) if x.strip()), "").rstrip()
                nxt = next((x for x in lines[i + 1:] if x.strip()), "")
                label = re.match(r"^\s*\w+:\s*$", prev) is not None
                if label and nxt.strip().startswith("}"):
                    out.append(l)
                    continue
                if prev.endswith(("{", ";", "}", "*/")) or label:
                    continue
                flags.append((fn, i + 1, prev))
            out.append(l)
        open(fn, "w").write("\n".join(out))
    return flags


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("report", "rewrite", "extra"):
        sys.exit(__doc__)
    cmd, args = sys.argv[1], sys.argv[2:]
    files = []
    if "--files" in args:
        k = args.index("--files")
        files, args = args[k + 1:], args[:k]
    if cmd == "extra":
        blk = [a for a in args if re.fullmatch(r"[0-9A-F]{8}\w*", a)]
        counts = load_counts([a for a in args if a not in blk])
        ids, sizes = load_blocks()
        entry, rest = blk[0], blk[1:]
        calls = counts[ids[entry]]
        extra = sum(counts[ids[b]] * sizes[b] for b in rest if b in ids)
        print("%s: %d calls, %d instructions in those blocks, %s a call" %
              (entry, calls, extra, round(extra / calls) if calls else "-"))
        return
    counts = load_counts(args)
    files = files or sorted(glob.glob(os.path.join(ENG, "*.c")))
    res, dup = analyse(files, counts)
    if cmd == "report":
        for (fn, name), r in sorted(res.items()):
            print("%-14s %-26s calls=%10d cost=%6d static=%6d sites=%d%s" %
                  (os.path.basename(fn), name, r["calls"], r["cost"], r["static"], len(r["sites"]),
                   "" if r["ok"] else "  ENTRY?"))
        for x, o in sorted(dup.items()):
            print("DUP", x, sorted(n for _, n in o))
        before = sum(r["total"] for r in res.values())
        after = sum(r["calls"] * r["cost"] for r in res.values())
        print("total %d before, %d after (%.4f)" % (before, after, after / max(1, before)))
    else:
        if not files:
            sys.exit("rewrite needs --files")
        for f in rewrite(res):
            print("FLAG", *f)


if __name__ == "__main__":
    main()
