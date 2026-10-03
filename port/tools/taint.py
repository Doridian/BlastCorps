#!/usr/bin/env python3
"""Where the native engine's leftovers are read (docs/PORT.md, "Replacing
the engine"): reads the taint.<pid>.txt files a PORT_ENGINE_TAINT build
writes (ENGINE_TAINT=DIR) and prints, for each reader (engine_ctx,
ENGINE_REG, engine_frame_lw), the chains of sites its values came from:
an ENGINE_LEAVE (L), an engine_restore (R) of a saved register, a frame
store of a register (D, sd; F, sdc1) or of a value (W, sw).  --live lists
every site that is in some chain: the scaffolding a reader depends on.

    port/tools/taint.py BUILD/blastcorps DIR... [--live] [--file 56040.c]
"""
import argparse
import collections
import glob
import os
import subprocess
import sys


def load(paths):
    runs = []
    for p in paths:
        files = sorted(glob.glob(os.path.join(p, "taint.*.txt"))) if os.path.isdir(p) else [p]
        for f in files:
            nodes, reads, base = {}, [], 0
            for line in open(f):
                w = line.split()
                if not w:
                    continue
                if w[0] == "B":
                    base = int(w[2], 16)
                elif w[0] == "N":
                    nodes[int(w[1])] = (w[2], int(w[3], 16), int(w[4]))
                elif w[0] == "R":
                    reads.append((int(w[1], 16), w[2], int(w[3]), int(w[4]), int(w[5])))
            runs.append((nodes, reads, base))
    return runs


def symbolize(exe, addrs, base):
    addrs = sorted(addrs)
    if not addrs:
        return {}
    pie = subprocess.run(["file", "-L", exe], capture_output=True, text=True).stdout
    off = base if "pie" in pie.lower() or "shared object" in pie.lower() else 0
    res = {}
    todo = list(addrs)
    # the return address less 1, or where the call starts (a call that the
    # compiler gave no line: line 0)
    for back in (1, 5, 2, 6):
        if not todo:
            break
        out = subprocess.run(["addr2line", "-e", exe, "-f", "-C"] + [hex(a - back - off) for a in todo],
                             capture_output=True, text=True).stdout.splitlines()
        left = []
        for i, a in enumerate(todo):
            fn, loc = out[2 * i], out[2 * i + 1]
            loc = loc.split(" (")[0]
            line = loc.split(":")[1] if ":" in loc else "?"
            if line in ("?", "0") and back != 6:
                left.append(a)
                continue
            res[a] = f"{os.path.basename(loc.split(':')[0])}:{line} {fn}"
        todo = left
    return res


REGN = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("dirs", nargs="+")
    ap.add_argument("--live", action="store_true", help="list the sites some reader depends on")
    ap.add_argument("--file", help="only readers or live sites in this source file")
    a = ap.parse_args()
    runs = load(a.dirs)
    # per run the tags are its own: chains as tuples of (kind, site)
    readers = collections.defaultdict(collections.Counter)
    sites = set()
    base = 0
    for nodes, reads, b in runs:
        base = b or base

        def chain(t):
            c = []
            while t:
                k, s, p = nodes[t]
                c.append((k, s))
                sites.add(s)
                t = p
            return tuple(c)
        for site, kind, reg, tag, n in reads:
            sites.add(site)
            readers[(site, kind, reg)][chain(tag)] += n
    names = symbolize(a.exe, sites, base)
    if a.live:
        live = collections.Counter()
        for key, chains in readers.items():
            for c, n in chains.items():
                for k, s in c:
                    live[(k, names[s])] += n
        for (k, nm), n in sorted(live.items(), key=lambda x: x[0][1]):
            if a.file and not nm.startswith(a.file + ":"):
                continue
            print(f"{k} {nm}  ({n})")
        return
    for (site, kind, reg), chains in sorted(readers.items(), key=lambda x: names[x[0][0]]):
        nm = names[site]
        if a.file and not nm.startswith(a.file + ":"):
            continue
        what = {"c": "engine_ctx", "r": "ENGINE_REG", "l": "engine_frame_lw"}[kind]
        rn = (REGN[reg] if reg < 32 else f"f{reg - 34}") if kind != "l" else hex(reg)
        print(f"{nm}: {what}({rn})")
        for c, n in chains.most_common():
            print(f"    {n:9d}  " + (" <- ".join(f"{k} {names[s]}" for k, s in c) or "(nothing left there)"))


if __name__ == "__main__":
    main()
