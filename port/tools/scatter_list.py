#!/usr/bin/env python3
"""What depends on the N64's layout, from scattered-layout runs
(docs/PORT.md, "The scattered layout").

    port/tools/scatter_list.py --static BUILD [--run NAME BUILD LOG ...]

Each --run is a run's output (LOG, the "scatter: " lines) from a
PORT_SCATTER_CHECK build (BUILD: its gen/scatter_report.txt says what it
placed where).  --static is the PORT_SCATTER build whose names inside
other variables are listed.  Writes Markdown to stdout: every site the runs saw reach where
nothing should be, by kind, with the symbols and where the N64's layout
has what it reached (a site that walks on, through padding after padding,
is one row: the first place it reached, and how many more); then the
names inside other variables the N64 side declares, whether a run saw
them or not.

Kinds:
  alias     a name inside another variable, declared as its own (D_803156C4
            for Sched.frameCount): its own room in the scattered layout
  past end  an access past a C variable's end (a T x[1] placeholder that is
            bigger, a read past an array): the padding after it
  label     an access past an asm data label's end into the next label of
            the same file (a walk across labels, a table's end pointer)
  before    an access before a variable's start
  address   an N64 place a variable left: an address made otherwise than
            from the variable (a range by address, a constant)
"""
import argparse
import bisect
import collections
import os
import re
import sys

LINE = re.compile(r"^scatter: (?P<site>.*?) (?P<op>load|store|atomic|memset|copy|copy to|copy from|by value|to \S+): "
                  r"(?P<size>\d+) bytes at (?P<addr>[0-9A-F]{8}) reach (?P<what>.*)$")
WALK = 4
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FN = re.compile(r"^(?P<fn>\S+)(?: \((?P<loc>[^)]*)\))?$")


def squeeze(users):
    """"f (file:1)", "f (file:2)" as "f (file:1, 2)" """
    by = collections.OrderedDict()
    for u in users:
        m = re.match(r"^(\S+) \((.*):(\d+)\)$", u)
        if m:
            by.setdefault((m.group(1), m.group(2)), []).append(m.group(3))
        else:
            by.setdefault((u, None), [])
    return "; ".join(f if file is None else f"{f} ({file}:{', '.join(ls)})" for (f, file), ls in by.items()) or "(none)"


def load_report(path):
    items, aliases, hosts = {}, [], []
    for line in open(path):
        p = line.split()
        if not p or p[0].startswith("#"):
            continue
        if p[0] == "item":
            items[p[1]] = dict(n64=int(p[2], 16), n64size=int(p[3], 16), addr=int(p[4], 16), size=int(p[5], 16),
                               kind=p[6])
        elif p[0] == "alias":
            head, _, users = line.partition(" | ")
            h = head.split()
            cont, off = h[3].split("+")
            rec = dict(name=h[1], n64=int(h[2], 16), cont=cont, off=int(off, 16), ckind=h[4], size=int(h[5]),
                       users=[u.strip() for u in line.split(" | ")[1:]])
            aliases.append(rec)
        elif p[0] == "host":
            cont, off = p[3].split("+")
            hosts.append(dict(name=p[1], n64=int(p[2], 16), cont=cont, off=int(off, 16)))
    return items, aliases, hosts


def load_syms(path):
    """the N64 names by address (data only), for naming where a byte was"""
    rows = []
    for line in open(path):
        p = line.split()
        if len(p) >= 3 and p[2] == "D":
            a = int(p[1], 16)
            if 0x80000000 <= a < 0x80400000:
                rows.append((a, int(p[3], 16) if len(p) > 3 else 0, p[0]))
    rows.sort()
    return rows


def n64_name(syms, addrs, a):
    k = bisect.bisect_right(addrs, a) - 1
    if k < 0:
        return f"{a:08X}"
    s, _, n = syms[k]
    return n if a == s else f"{n}+0x{a - s:X}"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--static", required=True)
    ap.add_argument("--run", nargs=3, action="append", default=[], metavar=("NAME", "BUILD", "LOG"))
    ap.add_argument("--title", default="Layout dependencies")
    args = ap.parse_args()
    gen = os.path.join(args.static, "gen")
    _, aliases, hosts = load_report(os.path.join(gen, "scatter_report.txt"))
    syms = load_syms(os.path.join(gen, "n64_syms.txt"))
    addrs = [s[0] for s in syms]
    reports = {}

    # every hit, in the order the runs saw them: (site, kind, symbol, where, access, run)
    hits = []
    for run, build, path in args.run:
        if build not in reports:
            reports[build] = load_report(os.path.join(build, "gen", "scatter_report.txt"))
        items, b_aliases = reports[build][0], reports[build][1]
        alias_of = {a["name"]: a for a in b_aliases}
        for line in open(path, errors="replace"):
            m = LINE.match(line.rstrip("\n"))
            if not m:
                continue
            what, site = m.group("what"), m.group("site")
            f = FN.match(site)
            fn, loc = (f.group("fn"), f.group("loc") or "") if f else (site, "")
            if loc.startswith("src/"):      # (an older build's: blastcorps/src or port/src)
                rest = loc[4:].rpartition(":")[0]
                loc = ("blastcorps/" if os.path.exists(os.path.join(ROOT, "blastcorps", "src", rest)) else "port/") + loc
            addr = int(m.group("addr"), 16)
            if (a := re.match(r"^(\S+) \((\S+)\+0x([0-9A-F]+)\), \+0x([0-9A-F]+)$", what)):
                kind, sym = "alias", a.group(1)
                where = f"{a.group(2)}+0x{int(a.group(3), 16) + int(a.group(4), 16):X}"
            elif (a := re.match(r"^(\S+)'s N64 place, \+0x([0-9A-F]+)$", what)):
                kind, sym = "address", a.group(1)
                where = f"{a.group(1)}+0x{int(a.group(2), 16):X}"
            elif (a := re.match(r"^padding, \+0x([0-9A-F]+) after (\S+)$", what)):
                sym, k = a.group(2), int(a.group(1), 16)
                it = items.get(sym)
                if it:
                    n64 = it["n64"] + it["n64size"] + k
                    where = n64_name(syms, addrs, n64)
                    kind = "label" if it["kind"] == "asm" else "past end"
                elif sym in alias_of:
                    al = alias_of[sym]
                    kind, where = "past end", f"{al['cont']}+0x{al['off'] + al['size'] + k:X} (past the alias)"
                else:
                    kind, where = "past end", "?"
                sym = f"{sym} +0x{it['size'] + k if it else k:X}"
            elif (a := re.match(r"^padding, -0x([0-9A-F]+) before (\S+)$", what)):
                sym, k = a.group(2), int(a.group(1), 16)
                it = items.get(sym)
                kind = "before"
                where = n64_name(syms, addrs, it["n64"] - k) if it else "?"
                sym = f"{sym} -0x{k:X}"
            else:
                kind, sym, where = "?", what, ""
            hits.append(((fn, loc), kind, sym, where, f"{m.group('op')} {m.group('size')}", run))

    # a site that reaches more than WALK places walks on (from a table into
    # what follows it in the scattered layout, aliases' rooms included): one
    # row, the first place it reached (the cause), and how many more
    places = collections.defaultdict(list)
    for site, kind, sym, *_ in hits:
        if (kind, sym) not in places[site]:
            places[site].append((kind, sym))
    # one row a variable, kind and function: its lines, offsets, accesses
    rows = collections.OrderedDict()
    for (fn, loc), kind, sym, where, op, run in hits:
        walk = len(places[(fn, loc)]) > WALK
        if walk:
            first = places[(fn, loc)][0]
            if (kind, sym) != first:
                continue
        base, _, off = sym.partition(" ")
        file, _, line = loc.rpartition(":")
        r = rows.setdefault((kind, base, fn, file), dict(lines=[], offs=[], wheres=[], ops=set(), runs=[], more=0))
        for lst, v in ((r["lines"], line), (r["offs"], off), (r["wheres"], where)):
            if v and v not in lst:
                lst.append(v)
        if walk:
            r["more"] = max(r["more"], len(places[(fn, loc)]) - 1)
        r["ops"].add(op)
        if run not in r["runs"]:
            r["runs"].append(run)

    def runs(names):
        """GROUP:RUN names as "GROUP n, ..." (with a GROUP's "tas" named)"""
        groups = collections.OrderedDict()
        for n in names:
            g, _, run = n.rpartition(":")
            groups.setdefault(g, []).append(run)
        return ", ".join((f"{g} {len(v)}" + (" +tas" if "tas" in v else "")) if g else ", ".join(v)
                         for g, v in groups.items())

    def few(xs, n=6):
        return ", ".join(xs[:n]) + (f", ... ({len(xs)})" if len(xs) > n else "")

    out = [f"# {args.title}", ""]
    order = ["alias", "past end", "label", "before", "address", "?"]
    counts = collections.Counter(k[0] for k in rows)
    out.append("| kind | rows |")
    out.append("| --- | --- |")
    for k in order:
        if counts[k]:
            out.append(f"| {k} | {counts[k]} |")
    out.append("")
    for k in order:
        if not counts[k]:
            continue
        out += [f"## {k}", "", "| symbol | N64 layout has | site | access | runs |", "| --- | --- | --- | --- | --- |"]
        for key, r in sorted(((kk, v) for kk, v in rows.items() if kk[0] == k), key=lambda x: (x[0][1], x[0][3], x[0][2])):
            _, base, fn, file = key
            sym = f"`{base}`" + (f" {few(r['offs'])}" if r["offs"] else "")
            if r["more"]:
                sym += f" (a walk: then {r['more']} more places)"
            lines = sorted(r["lines"], key=lambda l: int(l) if l.isdecimal() else 0)
            site = f"{file or '?'}:{few(lines, 8)} `{fn}`" if lines else f"? `{fn}`"
            where = few([f"`{w}`" for w in r["wheres"]], 4)
            out.append(f"| {sym} | {where} | {site} | {', '.join(sorted(r['ops']))} | {runs(r['runs'])} |")
        out.append("")
    seen = {k[1] for k in rows if k[0] == "alias"}
    out += ["## Names inside other variables (static)", "",
            f"{len(aliases)} names the N64 side declares that are inside another variable "
            "(port-arena's report), whether a run reached them or not; `seen` marks those a run did.", "",
            "| name | inside | C or asm | users | seen |", "| --- | --- | --- | --- | --- |"]
    for a in sorted(aliases, key=lambda a: (a["cont"], a["off"])):
        users = squeeze(a["users"])
        out.append(f"| `{a['name']}` | `{a['cont']}+0x{a['off']:X}` | {a['ckind']} | {users} | "
                   f"{'seen' if a['name'] in seen else ''} |")
    out.append("")
    used = set()
    root = ROOT
    for d in ("port/host", "port/src"):
        for f in os.listdir(os.path.join(root, d)):
            if f.endswith((".c", ".h")):
                text = open(os.path.join(root, d, f), errors="replace").read()
                used |= {x for t in re.findall(r"PORT_(?:VAR|ADDR)\((\w+)\)|PORT_N64_(\w+)", text) for x in t if x}
    hostuse = [h for h in hosts if h["name"] in used] + [a for a in aliases if a["name"] in used]
    if hostuse:
        out += ["## Names the host reads inside other variables", "",
                "The host names game variables by their N64 addresses (`PORT_VAR`, `n64_syms.h`); these are "
                "inside another variable.", "", "| name | inside |", "| --- | --- |"]
        for h in sorted(hostuse, key=lambda h: h["name"]):
            out.append(f"| `{h['name']}` | `{h['cont']}+0x{h['off']:X}` |")
        out.append("")
    sys.stdout.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
