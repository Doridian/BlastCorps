#!/usr/bin/env python3
"""A node --cpu-prof profile (the WebAssembly build) as self time by function.

  node --cpu-prof --cpu-prof-interval 200 --cpu-prof-dir DIR build/wasm/blastcorps.js ROM --headless ...
  cpuprofile_report.py DIR/*.cpuprofile [--top N] [--area] [--only AREA]

(the build linked with --profiling-funcs, e.g. -DCMAKE_EXE_LINKER_FLAGS=--profiling-funcs,
so that the functions have their names)

Self time from the samples' leaf nodes, by function name (the module's
name section: the C functions' own names), with the idle and the garbage
collector as node reports them.
"""
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sprof_report import defined_names  # noqa: E402


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    top = 40
    if "--top" in sys.argv:
        top = int(sys.argv[sys.argv.index("--top") + 1])
        args.remove(str(top))
    only = sys.argv[sys.argv.index("--only") + 1] if "--only" in sys.argv else None
    if only:
        args.remove(only)
    self_us = collections.Counter()
    total = 0
    for path in args:
        prof = json.load(open(path))
        nodes = {n["id"]: n for n in prof["nodes"]}
        deltas = prof.get("timeDeltas", [])
        samples = prof["samples"]
        for k, nid in enumerate(samples):
            dt = deltas[k + 1] if k + 1 < len(deltas) else 0
            name = nodes[nid]["callFrame"]["functionName"] or "(anonymous)"
            self_us[name] += dt
            total += dt
    print("%.2f s sampled" % (total / 1e6))
    names = defined_names()
    if "--area" in sys.argv:
        areas = collections.Counter()
        for name, us in self_us.items():
            areas[names.get(name, "other")] += us
        for a, us in areas.most_common():
            print("  %-10s %6.2f%%" % (a, 100.0 * us / total))
    for name, us in self_us.most_common():
        if only and names.get(name, "other") != only:
            continue
        print("%7.2f%%  %s" % (100.0 * us / total, name))
        top -= 1
        if top == 0:
            break


if __name__ == "__main__":
    main()
