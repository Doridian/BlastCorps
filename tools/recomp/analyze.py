#!/usr/bin/env python3
"""Inventory and control-flow checks over the translated objects.

    tools/recomp/analyze.py            # opcode counts and any problems

It also lists the irregular control flow the translator handles specially
(branches into another function, which make translate.py merge the functions
into one C body; branches into a delay slot; functions that fall through
into the next) and, as problems, what it refuses: jr through anything but
$ra, jalr, and branches whose target it can't find.
"""

import collections
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from config import translated_objects
from parse import parse_file, is_terminator


def load_all(version=None):
    # the version of the stage-2 build (the Makefile's VERSION)
    version = version or os.environ.get("RECOMP_VERSION", "us.v11")
    objs = []
    for module, name, path in translated_objects(version):
        objs.append(parse_file(path, module, name))
    return objs


def check(objs):
    funcs = {fn.name: fn for o in objs for fn in o.functions}
    problems, notes = [], []
    externs = collections.Counter()
    for o in objs:
        labels = {lab for fn in o.functions for lab in fn.labels}
        for fn in o.functions:
            for ln in fn.lines:
                ins = ln.insn
                if ins.op in ("jal", "j"):
                    if ln.target not in funcs:
                        externs[ln.target] += 1
                elif ins.kind == "branch":
                    if ln.target in fn.labels:
                        if fn.lines[fn.labels[ln.target]].delay:
                            notes.append(f"{fn.name}: branch at {ln.vram:08X} into a delay slot")
                    elif ln.target in labels or ln.target in funcs:
                        notes.append(f"{fn.name}: branch at {ln.vram:08X} to {ln.target} "
                                     "in another function (merged)")
                    else:
                        problems.append(f"{fn.name}: branch at {ln.vram:08X} to unknown {ln.target}")
                elif ins.op in ("jr", "jalr") and not (ins.op == "jr" and ins.rs == 31):
                    problems.append(f"{fn.name}: {ins.op} at {ln.vram:08X}")
            last = fn.lines[-1]
            prev = fn.lines[-2] if len(fn.lines) > 1 else None
            if not (prev is not None and last.delay and is_terminator(prev)):
                notes.append(f"{fn.name}: falls through at {last.vram:08X} (merged with the next)")
    return problems, notes, externs


def main():
    objs = load_all()
    counts = collections.Counter()
    nfunc = ninsn = 0
    for o in objs:
        for fn in o.functions:
            nfunc += 1
            for ln in fn.lines:
                ninsn += 1
                counts[ln.insn.op] += 1
    print(f"{len(objs)} objects, {nfunc} functions, {ninsn} instructions")
    print(" ".join(f"{k}:{v}" for k, v in counts.most_common()))
    problems, notes, externals = check(objs)
    print(f"{len(externals)} external call targets:",
          " ".join(sorted(externals)))
    for n in notes:
        print("note", n)
    for p in problems:
        print("PROBLEM", p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
