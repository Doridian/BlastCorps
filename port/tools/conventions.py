#!/usr/bin/env python3
"""The calling convention each translated function actually has.

    port/tools/conventions.py [OBJECT|FUNC ...]     print them
    (import) conventions(funcs) -> {name: Conv}

Rare's handwritten engine passes arguments and results in whatever
registers suit it (`func_802ABCDC` takes t3-t7 and s0 and answers in s1).
A native replacement of one function (docs/PORT.md, "Replacing the
engine") has to be given every register the original reads before writing
it (its inputs), and has to give back every register the original may
write that some translated caller reads afterwards (its outputs); the glue
restores everything else in the context around the call.  This works that
out from the code:

- **Values** (forward, per function): which registers still hold their
  value from the entry at each return.  Saves and restores through the
  stack (the slots are tracked by their offset from the entry $sp, which
  the code only moves by constants) and plain moves keep a value; so a
  register a function saves and restores isn't one it writes, whichever
  register it is.  A register written on some paths and not others is a
  "pass-through" output: the native code needs it as an input too.
- **Liveness** (backward, over registers and stack slots): a store to a
  local slot uses the stored register only if the slot is read again, a
  store anywhere else always does, and a call uses what the callee reads
  (its live-in registers, and its stack arguments).  What is live right
  after a call is what the caller reads of the callee's results.

Calls out to IDO C or libultra follow o32: a0-a3 and the stack arguments
in, the caller-saved registers clobbered.  The game's C reads v0/v1 and
f0/f1 of what it calls (`c_called`).
"""

import os
import sys
from dataclasses import dataclass

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from liveness import (effects, load_functions, _nodes, regname, CALLER_SAVED,  # noqa: E402
                      CALLER_SAVED_F)

ALLREGS = frozenset(set(range(1, 32)) | {f"f{n}" for n in range(32)} | {"hi", "lo", "fcc"})
EXTERN_CLOBBER = frozenset(CALLER_SAVED | CALLER_SAVED_F | {"hi", "lo", "fcc"})
EXTERN_STACK_ARGS = range(16, 48, 4)        # what an o32 callee may read above its $sp
SP_MOVES = ("addiu", "addi", "daddiu", "daddi")
STORE_W = {"sb": 1, "sh": 2, "sw": 4, "swl": 1, "swr": 1, "sd": 8, "swc1": 4, "sdc1": 8}
LOAD_W = {"lb": 1, "lbu": 1, "lh": 2, "lhu": 2, "lw": 4, "lwu": 4, "lwl": 1, "lwr": 1,
          "ld": 8, "lwc1": 4, "ldc1": 8}

W_ = "W"        # a value computed here
T_ = "T"        # this register's entry value on some paths, another on others


@dataclass
class Conv:
    inputs: frozenset       # registers read before they are written (+ pass-through)
    outputs: frozenset      # registers written that a caller reads afterwards
    passthrough: frozenset  # outputs not written on every path (inputs too)
    may_write: frozenset
    must_write: frozenset
    stack_in: frozenset     # offsets above the entry $sp it loads (stack arguments)
    sp_known: bool


def reg_order(r):
    return (0, r) if isinstance(r, int) else (1, r[0], int(r[1:]) if r[1:].isdigit() else 0)


def fmt_regs(s):
    return " ".join(regname(r) for r in sorted(s, key=reg_order)) or "-"


def node_insn(fn, key):
    return fn.lines[key[1]] if key[0] == "i" else fn.lines[key[1] + 1]


def store_rt(i):
    """the register a store stores ('fN' for FPRs, a pair for sdc1)"""
    if i.op == "swc1":
        return [f"f{i.rt}"]
    if i.op == "sdc1":
        return [f"f{i.rt}", f"f{i.rt + 1}"]
    return [i.rt]


def load_rt(i):
    if i.op == "lwc1":
        return [f"f{i.rt}"]
    if i.op == "ldc1":
        return [f"f{i.rt}", f"f{i.rt + 1}"]
    return [i.rt]


def words(a, w):
    """the 4-byte words a w-byte access at a covers"""
    return range(a & ~3, a + w, 4) if w >= 4 else [a & ~3]


class Func:
    def __init__(self, fn, funcs):
        self.fn = fn
        self.nodes = _nodes(fn, funcs)
        self.keys = list(self.nodes)
        self.preds = {k: [] for k in self.keys}
        for k in self.keys:
            for s in self.nodes[k][2]:
                if s in self.preds:
                    self.preds[s].append(k)
        self.sp_deltas()
        self.callees = {v[1] for v in self.nodes.values() if v[0] == "call"}

    def sp_deltas(self):
        """the $sp's offset from its entry value before each node, if the code
        only ever moves it by constants"""
        delta = {("i", 0): 0}
        ok = True
        work = [("i", 0)]
        while work:
            k = work.pop()
            a, b, succs = self.nodes[k]
            d = delta[k]
            if a != "call" and d is not None:
                i = node_insn(self.fn, k).insn
                if 29 in effects(i)[1]:
                    d = d + i.imm if i.op in SP_MOVES and i.rs == 29 else None
            for s in succs:
                if s not in self.nodes:
                    continue
                if s not in delta:
                    delta[s] = d
                    work.append(s)
                elif delta[s] != d:
                    ok = False
                    if delta[s] is not None:
                        delta[s] = None
                        work.append(s)
        self.delta = delta
        self.sp_known = ok and all(v is not None for v in delta.values())

    def slot(self, k, i):
        """(entry-relative address, width) of an $sp-relative access at node k"""
        if i.rs != 29 or node_insn(self.fn, k).lo is not None:
            return None
        d = self.delta.get(k)
        return None if d is None else d + i.imm


def join(a, b):
    return a if a == b else T_


def values(F, may, must):
    """per return path: register -> its value there (('e', r), W_ or T_)"""
    fn = F.fn
    start = {r: ("e", r) for r in ALLREGS}
    state_in = {("i", 0): (start, {})}
    work = [("i", 0)]
    exits = []          # (regs at exit, tail target or None)
    seen_out = {}
    while work:
        k = work.pop()
        regs, slots = state_in[k]
        regs, slots = dict(regs), dict(slots)
        a, b, succs = F.nodes[k]
        if a == "call":
            if b in may:
                for r in may[b]:
                    regs[r] = W_ if r in must[b] else join(regs[r], W_)
            else:
                for r in EXTERN_CLOBBER:
                    regs[r] = join(regs[r], W_)
            d = F.delta.get(k)
            # the callee's frames are below the $sp
            slots = {s: v for s, v in slots.items() if d is not None and s >= d}
        else:
            ln = node_insn(fn, k)
            i = ln.insn
            u, dd = effects(i)
            if i.kind == "store" and i.op in STORE_W:
                s = F.slot(k, i)
                w = STORE_W[i.op]
                if s is not None:
                    for x in words(s, w):
                        slots.pop(x, None)
                    if w in (4, 8) and s % 4 == 0:
                        rts = store_rt(i)
                        if w == 8 and len(rts) == 1:
                            slots[s] = (regs.get(rts[0], W_) if rts[0] else ("zero",), 8)
                        else:
                            for n, r in enumerate(rts):
                                slots[s + 4 * n] = (regs.get(r, W_) if r else ("zero",), 4)
            elif i.kind == "load" and i.op in LOAD_W:
                s = F.slot(k, i)
                w = LOAD_W[i.op]
                rts = load_rt(i)
                got = None
                if s is not None and w in (4, 8) and i.op not in ("lwu",):
                    if w == 8 and len(rts) == 1:
                        v = slots.get(s)
                        got = [v[0] if v and v[1] == 8 else W_]
                    else:
                        got = []
                        for n in range(len(rts)):
                            v = slots.get(s + 4 * n)
                            # (a word reloaded as a word: lw sign-extends what sw
                            # stored, which a 32-bit value already is)
                            got.append(v[0] if v and v[1] == 4 and isinstance(rts[n], str) == (
                                isinstance(v[0], tuple) and isinstance(v[0][1], str)) else W_)
                for n, r in enumerate(rts):
                    if r == 0:
                        continue
                    regs[r] = got[n] if got else W_
            elif i.op in ("or", "daddu", "dadd") and (i.rs == 0 or i.rt == 0) and i.rd:
                src = i.rt if i.rs == 0 else i.rs
                regs[i.rd] = regs[src] if src else W_
            elif i.op in ("mov.s", "mov.d"):
                n = 2 if i.op == "mov.d" else 1
                for m in range(n):
                    regs[f"f{i.fd + m}"] = regs[f"f{i.fs + m}"]
            else:
                for r in dd:
                    regs[r] = W_
        for s in succs:
            if s[0] == "exit":
                exits.append((regs, None))
                continue
            if s[0] == "tail":
                exits.append((regs, s[1]))
                continue
            if s not in F.nodes:
                continue
            old = state_in.get(s)
            if old is None:
                state_in[s] = (regs, slots)
                work.append(s)
                continue
            oregs, oslots = old
            nregs = {r: join(oregs[r], regs[r]) for r in ALLREGS}
            nslots = {x: oslots[x] for x in oslots if x in slots and slots[x] == oslots[x]}
            for x in oslots:
                if x in slots and x not in nslots:
                    pass
            if nregs != oregs or nslots != oslots:
                state_in[s] = (nregs, nslots)
                work.append(s)
    return exits


def summarize(F, may, must):
    """(may write, must write) of one function"""
    mw, mu = set(), None
    exits = values(F, may, must)
    for regs, tail in exits:
        m = {r for r in ALLREGS if regs[r] != ("e", r)}
        u = {r for r in ALLREGS if regs[r] not in (("e", r), T_)}
        if tail is not None:
            m |= may.get(tail, ALLREGS)
            u |= must.get(tail, set())
        mw |= m
        mu = u if mu is None else mu & u
    mu = mu or set()
    return frozenset(mw - {0, 29}), frozenset(mu - {0, 29})


def stack_reads(F):
    """offsets above the entry $sp that the function loads (stack arguments)"""
    out = set()
    for k in F.keys:
        if F.nodes[k][0] == "call":
            continue
        i = node_insn(F.fn, k).insn
        if i.kind == "load":
            s = F.slot(k, i)
            if s is not None and s >= 0:
                out |= set(words(s, LOAD_W.get(i.op, 4)))
    return out


def liveness(F, conv_in, stack_in, must, live_out):
    """live registers at the entry; adds what is live after each call to
    live_out[callee].  Returns (live-in, changed live_out)"""
    fn = F.fn
    live = {k: frozenset() for k in F.keys}
    changed_out = False
    it = True
    while it:
        it = False
        for k in reversed(F.keys):
            a, b, succs = F.nodes[k]
            out = set()
            for s in succs:
                if s[0] == "exit":
                    # what the callers read afterwards (so that a call just
                    # before the return gives it its callers' readers too)
                    out |= live_out[F.fn.name]
                    continue
                if s[0] == "tail":
                    out |= conv_in.get(s[1], set())
                    continue
                out |= live.get(s, frozenset())
            if a == "call":
                d = F.delta.get(k)
                regs_out = {r for r in out if not (isinstance(r, tuple))}
                if b in conv_in:
                    if not regs_out <= live_out[b]:
                        live_out[b] |= regs_out
                        changed_out = True
                    new = set(conv_in[b]) | (out - must[b] - {31})
                    args = stack_in.get(b, ())
                else:
                    new = {4, 5, 6, 7, 29} | (out - EXTERN_CLOBBER)
                    args = EXTERN_STACK_ARGS
                if d is not None:
                    new |= {("s", d + o) for o in args}
                # the callee's frames are below the $sp: the slots there die
                if d is not None:
                    new = {x for x in new if not (isinstance(x, tuple) and x[1] < d)}
            else:
                i = node_insn(fn, k).insn
                u, dd = effects(i)
                new = set(out)
                if i.kind == "store" and i.op in STORE_W:
                    s = F.slot(k, i)
                    w = STORE_W[i.op]
                    rts = [r for r in store_rt(i) if r != 0]
                    if s is not None and s < 0:
                        ws = [("s", x) for x in words(s, w)]
                        full = w >= 4 and s % 4 == 0
                        used = any(x in out for x in ws)
                        if full:
                            new -= set(ws)
                        if used or not full:
                            new |= set(rts)
                    else:
                        new |= set(rts)
                    new |= {i.rs} - {0}
                elif i.kind == "load" and i.op in LOAD_W:
                    s = F.slot(k, i)
                    rts = [r for r in load_rt(i) if r != 0]
                    needed = any(r in out for r in rts)
                    new -= set(rts)
                    if i.op in ("lwl", "lwr"):
                        new |= set(rts)
                    new |= {i.rs} - {0}
                    # (a reload nothing reads, the epilogue's of a register
                    # the caller doesn't read, doesn't need its slot)
                    if s is not None and s < 0 and needed:
                        new |= {("s", x) for x in words(s, LOAD_W[i.op])}
                else:
                    new = set(u) | (out - dd)
            new = frozenset(new)
            if new != live[k]:
                live[k] = new
                it = True
    entry = {r for r in live[("i", 0)] if not isinstance(r, tuple)}
    return entry, changed_out


def conventions(funcs, c_called=()):
    Fs = {n: Func(fn, funcs) for n, fn in funcs.items()}
    may = {n: ALLREGS - {0, 29} for n in funcs}
    must = {n: frozenset() for n in funcs}
    for _ in range(60):
        changed = False
        for n, F in Fs.items():
            m, u = summarize(F, may, must)
            if (m, u) != (may[n], must[n]):
                may[n], must[n] = m, u
                changed = True
        if not changed:
            break
    stack_in = {n: stack_reads(F) for n, F in Fs.items()}
    conv_in = {n: set() for n in funcs}
    live_out = {n: set() for n in funcs}
    for _ in range(200):
        changed = False
        for n, F in Fs.items():
            entry, ch = liveness(F, conv_in, stack_in, must, live_out)
            if entry != conv_in[n]:
                conv_in[n] = entry
                changed = True
            changed |= ch
        if not changed:
            break
    res = {}
    for n in funcs:
        lo = set(live_out[n])
        if n in c_called:
            lo |= {2, 3, "f0", "f1"}
        outs = frozenset((lo & may[n]) - {0, 29, 31})
        pt = frozenset(outs - must[n])
        ins = frozenset((conv_in[n] | pt) - {0, 31})
        res[n] = Conv(ins, outs, pt, may[n], must[n], frozenset(stack_in[n]), Fs[n].sp_known)
    return res


def main():
    funcs = load_functions()
    conv = conventions(funcs)
    only = set(sys.argv[1:])
    for name in sorted(conv):
        if only and name not in only and funcs[name].obj.name not in only:
            continue
        c = conv[name]
        line = f"{name} in: {fmt_regs(c.inputs - {29})} | out: {fmt_regs(c.outputs)}"
        if c.passthrough:
            line += f" | pass: {fmt_regs(c.passthrough)}"
        if c.stack_in:
            line += f" | stack: {' '.join(f'{o:#x}' for o in sorted(c.stack_in))}"
        if not c.sp_known:
            line += " | sp moves"
        print(line)


if __name__ == "__main__":
    main()
