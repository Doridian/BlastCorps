#!/usr/bin/env python3
"""Where the native engine's reads of the context come from.

A build with -DCMAKE_C_FLAGS=-DPORT_PROV (port/host/engine.c) records, for
every read of the thread's context (engine_ctx, ENGINE_REG) and of the dead
stack (engine_frame_lw), which ENGINE_LEAVE (or engine_frame store) wrote
the value last, through which engine_restore it came back if one did
(only a restore that changed the register counts), and the frames around a
stack store.  With PROV_OUT=PREFIX in the environment each process writes
PREFIX.<pid> at exit (test.py passes it on: it isn't a PORT_ variable):

    PROV_OUT=$PWD/prov/q port/tools/test.py quick build/prov
    PROV_OUT=$PWD/prov/t port/tools/test.py tas build/prov

    engine_prov.py EXE LOG...                  every reader and its writers
    engine_prov.py EXE LOG... --feeds FILE...  the writes in these files
                                               that a read elsewhere takes

EXE is the executable the logs came from (the TAS runs a copy of it:
BUILD/test/tas/blastcorps), for the sites' file:line.  docs/PORT.md,
"Replacing the engine", has how the O2 pass used it.
"""
import argparse
import collections
import re
import subprocess


def load(exe, logs):
    rows = collections.Counter()
    vals = {}
    addrs = set()
    for fn in logs:
        for line in open(fn):
            if line.startswith("PROBE"):
                continue
            t = line.split()
            d = dict(zip(t[0:16:2], t[1:16:2]))
            ch = tuple(t[17:])
            key = (d["R"], d["K"], d["P"], d["RS"], d["F"], d["REG"], ch)
            rows[key] += int(d["N"])
            vals.setdefault(key, d["V"])
            for a in (d["R"], d["P"], d["RS"], d["F"]) + ch:
                if int(a, 16):
                    addrs.add(a)
    addrs = sorted(addrs)
    out = subprocess.run(["addr2line", "-f", "-s", "-e", exe] + ["%x" % (int(a, 16) - 1) for a in addrs],
                         capture_output=True, text=True).stdout.split("\n")
    sym = {a: "%s@%s" % (out[2 * i], out[2 * i + 1].split(" ")[0]) for i, a in enumerate(addrs)}
    return rows, vals, lambda a: sym.get(a, "-") if int(a, 16) else "-"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("exe")
    ap.add_argument("logs", nargs="+")
    ap.add_argument("--feeds", nargs="+", metavar="FILE", help="engine files (56040.c ...) whose writes to list")
    a = ap.parse_args()
    rows, vals, S = load(a.exe, a.logs)
    lines = []
    for key, n in rows.items():
        r, k, p, rs, f, reg, ch = key
        k = int(k)
        what = ("frame+%#x" % (k - 1000)) if k >= 1000 else ("REG %d" % (k - 200)) if k >= 200 else "ctx %d" % k
        line = "%-40s %-12s <- %-40s" % (S(r), what, S(p))
        if int(rs, 16):
            line += " restored@%s" % S(rs)
        if int(f, 16):
            line += " frame@%s reg %s chain %s" % (S(f), reg, " / ".join(S(c) for c in ch if int(c, 16)))
        lines.append((S(r), k, "%8d %s  v=%s" % (n, line, vals[key])))
    if not a.feeds:
        for _, _, line in sorted(lines):
            print(line)
        return
    mine = set(a.feeds)
    keep = collections.defaultdict(set)
    for _, _, line in lines:
        m = re.match(r"\s*\d+ (\S+)@(\S+?):(\S+)\s+(.*?)\s+<-\s+(.*)$", line)
        if not m or m.group(2) in mine:
            continue
        for _, file, ln in re.findall(r"(\S+)@(\S+?):(\d+|\?)", m.group(5)):
            if file in mine:
                keep[file + ":" + ln].add("%s@%s:%s %s" % (m.group(1), m.group(2), m.group(3), m.group(4)))
    for k in sorted(keep, key=lambda s: (s.split(":")[0], int(s.split(":")[1]) if s.split(":")[1].isdigit() else 0)):
        print(k, " <- for ", "; ".join(sorted(keep[k])))


if __name__ == "__main__":
    main()
