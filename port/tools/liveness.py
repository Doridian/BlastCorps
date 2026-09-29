"""Register liveness over the translated handwritten code.

For each translated function: the registers it reads before writing them
(its register arguments, whatever the ABI says).  The port's glue uses it to
check the prototypes the C gives these functions, and to find the ones that
take arguments outside a0-a3/f12/f14.

Stores don't count as uses of the stored register (Rare's code saves and
restores registers all over, to the stack and to globals; a stored value
is at most passed through).  A call to another translated function uses that
function's live-in registers and kills whatever it may write; a call out
(IDO C, libultra) uses a0-a3, f12 and f14 and kills the caller-saved
registers.  Odd FPRs (the high halves of doubles) come out noisy.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
# the version of the stage-2 build the port is made from (CMake sets it)
VERSION = os.environ.get("PORT_VERSION", "us.v11")
sys.path.insert(0, os.path.join(ROOT, "tools", "recomp"))

from config import translated_objects  # noqa: E402
from parse import parse_file  # noqa: E402
from mips import GPR_NAMES  # noqa: E402

SAVED = {16, 17, 18, 19, 20, 21, 22, 23, 28, 30}          # s0-s7, gp, fp
CALLER_SAVED = ({1, 2, 3, 4, 5, 6, 7} | set(range(8, 16)) | {24, 25, 31})
CALLER_SAVED_F = {f"f{n}" for n in range(0, 20)}
ARGS_OUT = {4, 5, 6, 7, 29}      # no call out passes a float

R_READ_RS_RT = {"add", "addu", "sub", "subu", "and", "or", "xor", "nor", "slt",
                "sltu", "dadd", "daddu", "dsub", "dsubu", "sllv", "srlv", "srav",
                "dsllv", "dsrlv", "dsrav"}
R_SHIFT = {"sll", "srl", "sra", "dsll", "dsrl", "dsra", "dsll32", "dsrl32", "dsra32"}
MULDIV = {"mult", "multu", "div", "divu", "dmult", "dmultu", "ddiv", "ddivu"}
I_ALU = {"addi", "addiu", "slti", "sltiu", "andi", "ori", "xori", "daddi", "daddiu"}


def fregs(n, fmt):
    return {f"f{n}", f"f{n + 1}"} if fmt in ("d", "l") else {f"f{n}"}


def effects(i):
    """(uses, defs) of one instruction, GPRs as ints, FPRs as 'fN'."""
    op = i.op
    u, d = set(), set()
    if op in R_READ_RS_RT:
        u |= {i.rs, i.rt}; d.add(i.rd)
    elif op in R_SHIFT:
        u.add(i.rt); d.add(i.rd)
    elif op in MULDIV:
        u |= {i.rs, i.rt}; d |= {"hi", "lo"}
    elif op in ("mfhi", "mflo"):
        u.add(op[2:]); d.add(i.rd)
    elif op in ("mthi", "mtlo"):
        u.add(i.rs); d.add(op[2:])
    elif op in I_ALU:
        u.add(i.rs); d.add(i.rt)
    elif op == "lui":
        d.add(i.rt)
    elif i.kind == "load":
        u.add(i.rs)
        if op in ("lwc1",):
            d.add(f"f{i.rt}")
        elif op == "ldc1":
            d |= {f"f{i.rt}", f"f{i.rt + 1}"}
        else:
            d.add(i.rt)
            if op in ("lwl", "lwr", "ldl", "ldr"):
                u.add(i.rt)
    elif i.kind == "store":
        u.add(i.rs)
        if op == "swc1":
            u.add(f"f{i.rt}")
        elif op == "sdc1":
            u |= {f"f{i.rt}", f"f{i.rt + 1}"}
        # the stored register isn't a use (see the module comment)
    elif i.kind == "branch":
        if op.startswith("bc1"):
            u.add("fcc")
        elif op in ("beq", "bne", "beql", "bnel"):
            u |= {i.rs, i.rt}
        else:
            u.add(i.rs)
        if i.link:
            d.add(31)
    elif op == "jr":
        if i.rs != 31:
            u.add(i.rs)
    elif op == "jalr":
        u.add(i.rs); d.add(i.rd)
    elif op in ("jal", "j"):
        if op == "jal":
            d.add(31)
    elif op in ("mfc1", "dmfc1"):
        u.add(f"f{i.fs}"); d.add(i.rt)
    elif op in ("mtc1", "dmtc1"):
        u.add(i.rt); d.add(f"f{i.fs}")
    elif op == "cfc1":
        d.add(i.rt)
    elif op == "ctc1":
        u.add(i.rt)
    elif i.kind == "cop1":
        base = op.split(".")[0]
        fmt = i.fmt
        if base == "c":
            u |= fregs(i.fs, fmt) | fregs(i.ft, fmt); d.add("fcc")
        elif base in ("add", "sub", "mul", "div"):
            u |= fregs(i.fs, fmt) | fregs(i.ft, fmt); d |= fregs(i.fd, fmt)
        else:
            # one-operand: sqrt/abs/mov/neg/cvt/round...; the result format
            # of a conversion is in the name
            u |= fregs(i.fs, fmt)
            parts = op.split(".")
            dfmt = fmt
            if base in ("cvt", "round", "trunc", "ceil", "floor"):
                dfmt = parts[1]
            d |= fregs(i.fd, dfmt)
    elif op == "mfc0":
        d.add(i.rt)
    elif op == "mtc0":
        u.add(i.rt)
    u.discard(0); d.discard(0)
    return u, d


def load_functions(version=VERSION):
    funcs = {}
    for module, name, path in translated_objects(version):
        o = parse_file(path, module, name)
        for fn in o.functions:
            funcs[fn.name] = fn
    return funcs


def _nodes(fn, funcs):
    """Instruction-level CFG of one function, with delay slots split per edge."""
    lines = fn.lines
    n = len(lines)
    label_idx = dict(fn.labels)
    nodes = {}          # key -> (uses, defs, succs)

    def tgt(ln):
        if ln.target in label_idx:
            return ("i", label_idx[ln.target])
        if ln.target in funcs and funcs[ln.target] is not fn:
            return ("tail", ln.target)
        if ln.target == fn.name:
            return ("i", 0)
        return ("exit",)

    for k, ln in enumerate(lines):
        if ln.delay:
            continue
        i = ln.insn
        u, d = effects(i)
        has_delay = k + 1 < n and lines[k + 1].delay
        if i.kind == "branch" or i.op in ("jal", "j", "jr", "jalr"):
            du, dd = effects(lines[k + 1].insn) if has_delay else (set(), set())
            after = ("i", k + 2)
            if i.op == "jal":
                callee = ln.target
                nodes[("i", k)] = (u, d, [("d", k, "call")])
                nodes[("d", k, "call")] = (du, dd, [("call", k)])
                nodes[("call", k)] = ("call", callee, [after])
            elif i.op == "jr":
                nodes[("i", k)] = (u, d, [("d", k, "x")])
                nodes[("d", k, "x")] = (du, dd, [("exit",)])
            elif i.op == "j":
                nodes[("i", k)] = (u, d, [("d", k, "t")])
                nodes[("d", k, "t")] = (du, dd, [tgt(ln)])
            else:
                t = tgt(ln)
                if i.likely:
                    nodes[("i", k)] = (u, d, [("d", k, "t"), after])
                else:
                    nodes[("i", k)] = (u, d, [("d", k, "t"), ("d", k, "f")])
                    nodes[("d", k, "f")] = (du, dd, [after])
                nodes[("d", k, "t")] = (du, dd, [t])
        else:
            nxt = ("i", k + 1) if k + 1 < n else ("exit",)
            nodes[("i", k)] = (u, d, [nxt])
    return nodes


def may_def(funcs):
    """name -> registers the function (or anything it calls) may write,
    not counting restores of callee-saved registers from the stack."""
    md = {}
    for n, fn in funcs.items():
        s = set()
        for ln in fn.lines:
            i = ln.insn
            u, d = effects(i)
            if i.kind == "load" and i.rs == 29 and i.rt in SAVED | {31}:
                continue
            s |= d
        md[n] = s
    calls = {n: {ln.target for ln in fn.lines if ln.insn.op == "jal" and ln.target in funcs}
             for n, fn in funcs.items()}
    changed = True
    while changed:
        changed = False
        for n in funcs:
            for c in calls[n]:
                if not md[c] <= md[n]:
                    md[n] |= md[c]
                    changed = True
    return md


def liveness(funcs):
    """name -> set of live-in registers."""
    cfgs = {name: _nodes(fn, funcs) for name, fn in funcs.items()}
    mdef = may_def(funcs)
    live_in_fn = {name: set() for name in funcs}
    changed = True
    while changed:
        changed = False
        for name, nodes in cfgs.items():
            live = {k: set() for k in nodes}

            def lin(key):
                if key[0] == "exit":
                    return set()
                if key[0] == "tail":
                    return live_in_fn[key[1]]
                if key not in nodes:
                    return set()
                return live[key]

            it = True
            while it:
                it = False
                for key in reversed(list(nodes)):
                    a, b, succs = nodes[key]
                    out = set()
                    for s in succs:
                        out |= lin(s)
                    if a == "call":
                        callee = b
                        if callee in funcs:
                            new = live_in_fn[callee] | (out - mdef[callee] - {31})
                        else:
                            new = set(ARGS_OUT) | (out - CALLER_SAVED - CALLER_SAVED_F - {"hi", "lo", "fcc"})
                    else:
                        new = a | (out - b)
                    if new != live[key]:
                        live[key] = new
                        it = True
            entry = lin(("i", 0))
            if entry != live_in_fn[name]:
                live_in_fn[name] = entry
                changed = True
    return live_in_fn


def regname(r):
    return GPR_NAMES[r] if isinstance(r, int) else r


if __name__ == "__main__":
    funcs = load_functions()
    live = liveness(funcs)
    only = set(sys.argv[1:])
    for name in sorted(live):
        if only and name not in only:
            continue
        regs = sorted(live[name], key=lambda r: (isinstance(r, str), r if isinstance(r, int) else r))
        print(name, " ".join(regname(r) for r in regs))
