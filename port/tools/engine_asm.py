#!/usr/bin/env python3
"""A translated function's asm as the engine's replacement needs it.

    port/tools/engine_asm.py FUNC|OBJECT ...  [--no-conv]

Prints each function's instructions with the basic blocks the translator
charges (`== ENGINE_BLK(802AC1E4)  7 ==` before each: what the native code
calls where that block would run; docs/PORT.md, "Replacing the engine"),
and its register convention (conventions.py: what it reads, what callers
read of what it writes).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from liveness import load_functions  # noqa: E402


# the translator's own table, where it is
SIZES = {}
_h = os.path.join(HERE, "..", "..", "blastcorps", "build", "recomp", "src", "engine_blocks.h")
if os.path.exists(_h):
    for _l in open(_h):
        if _l.startswith("#define ENGINE_BLK_"):
            _a, _v = _l.split()[1:3]
            SIZES[int(_a[11:], 16)] = int(_l.split(",")[1])


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
            # (keyed by us.v11's address in every version: translate.py)
            key = int(fn.name[5:13], 16) + ln.vram - fn.vram if re.fullmatch(r"func_[0-9A-F]{8}", fn.name) \
                else ln.vram
            n = SIZES.get(key, nxt - i)
            print(f"  == ENGINE_BLK({key:08X})  {n} ==")
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
