#!/usr/bin/env python3
"""A translated function's asm as the engine's replacement needs it.

    port/tools/engine_asm.py FUNC|OBJECT ...  [--no-conv]

Prints each function's instructions with its basic blocks
(`== 802AC1E4  7 ==` before each: its us.v11 address and its
instructions), and its register convention (conventions.py: what it
reads, what callers read of what it writes).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from liveness import load_functions  # noqa: E402


def leaders(fn):
    lines = fn.lines
    lab = set(fn.labels.values())
    lead = {0} | lab
    for i, ln in enumerate(lines):
        if ln.insn.kind in ("branch", "jump") and i + 2 <= len(lines):
            lead.add(i + 2)
    return sorted(x for x in lead if x < len(lines))


def show(fn, conv=None):
    if conv is not None:
        from conventions import fmt_regs
        c = conv[fn.name]
        s = f"   in: {fmt_regs(c.inputs - {29})}   out: {fmt_regs(c.outputs)}"
        if c.passthrough:
            s += f"   pass-through: {fmt_regs(c.passthrough)}"
        if c.stack_in:
            s += "   stack: " + " ".join(f"{o:#x}" for o in sorted(c.stack_in))
        print(f"{fn.name}  ({fn.obj.name}){s}")
    else:
        print(f"{fn.name}  ({fn.obj.name})")
    lines = fn.lines
    lead = leaders(fn)
    labels = {}
    for name, idx in fn.labels.items():
        labels.setdefault(idx, []).append(name)
    skip = set()        # a leader in a delay slot: the translator charges no block there
    for i, ln in enumerate(lines):
        if ln.insn.kind in ("branch", "jump"):
            skip.add(i + 1)
    for i, ln in enumerate(lines):
        for name in labels.get(i, []):
            print(f"  {name}:")
        if i in lead and i not in skip:
            j = lead.index(i)
            nxt = lead[j + 1] if j + 1 < len(lead) else len(lines)
            # (by us.v11's address in every version, a version's own
            # function by its address and suffix)
            m = re.fullmatch(r"func_([0-9A-F]{8})(_\w+)?", fn.name)
            if m and not m.group(2):
                key = f"{int(m.group(1), 16) + ln.vram - fn.vram:08X}"
            else:
                key = f"{ln.vram:08X}{m.group(2) if m else ''}"
            print(f"  == {key}  {nxt - i} ==")
        d = "  " if not ln.delay else "   "
        print(f"    {ln.vram:08X}: {d}{ln.text_op:<10} {ln.operands}")
    print()


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    funcs = load_functions()
    conv = None
    if "--no-conv" not in sys.argv:
        from conventions import conventions
        conv = conventions(funcs)
    for a in args:
        sel = [f for f in funcs.values() if f.name == a or f.obj.name == a]
        for f in sorted(sel, key=lambda f: f.vram):
            show(f, conv)


if __name__ == "__main__":
    main()
