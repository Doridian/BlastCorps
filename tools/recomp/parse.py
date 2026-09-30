"""Parse splat's symbolic .s output into objects, functions and instructions.

Each instruction line looks like

    /* 56048 8029A808 3C0F803A */  lui        $t7, %hi(D_803A7410)

The word in the comment is decoded (mips.py); from the operand text only the
symbolic parts are taken: `%hi(sym)`, `%lo(sym)`, a branch label, or a jal
target.  Addresses in the comments are those of the original ROM and are used
only to key labels and for diagnostics, never emitted as constants: the
generated C refers to every address by its symbol, like the .s does.
"""

import re
from dataclasses import dataclass, field

from mips import decode, text_matches, DecodeError

LINE_RE = re.compile(
    r"^\s*/\*\s*([0-9A-F]+)\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/\s+(\S+)\s*(.*?)\s*$")
GLABEL_RE = re.compile(r"^\s*glabel\s+(\S+)")
LOCAL_RE = re.compile(r"^\s*(\.L[0-9A-Za-z_]+):")
HI_RE = re.compile(r"%hi\(([^)]+)\)")
LO_RE = re.compile(r"%lo\(([^)]+)\)")
SECTION_RE = re.compile(r"^\s*\.section\s+(\S+?)[,\s]")
# asm-processor's section directives in a GLOBAL_ASM function's .s
BARE_SECTION_RE = re.compile(r"^\s*(\.text|\.data|\.bss|\.rdata|\.rodata|\.late_rodata)\s*$")
# a jump table's target in IDO code: splat labels those `glabel L<vram>_<rom>`
JUMP_LABEL_RE = re.compile(r"^L[0-9A-F]{8}_[0-9A-F]+$")
WORD_RE = re.compile(r"^\s*\.word\s+(.*)$")


@dataclass
class Line:
    rom: int
    vram: int
    insn: object
    text_op: str
    operands: str
    hi: str = None          # symbol of a %hi() operand
    lo: str = None          # symbol of a %lo() operand
    target: str = None      # branch label / jal target name
    delay: bool = False     # sits in a delay slot


@dataclass
class Function:
    name: str
    vram: int
    obj: object
    lines: list = field(default_factory=list)
    labels: dict = field(default_factory=dict)   # label name -> index in lines

    def end(self):
        return self.lines[-1].vram + 4 if self.lines else self.vram


@dataclass
class Object:
    module: str
    name: str
    path: str
    functions: list = field(default_factory=list)
    jump_labels: set = field(default_factory=set)   # labels a jump table holds


class ParseError(Exception):
    pass


def parse_file(path, module, name):
    obj = Object(module=module, name=name, path=path)
    func = None
    pending_labels = []
    section = ".text"
    with open(path) as f:
        for lineno, raw in enumerate(f, 1):
            s = raw.split("#", 1)[0] if raw.lstrip().startswith("#") else raw
            m = SECTION_RE.match(s) or BARE_SECTION_RE.match(s)
            if m:
                section = m.group(1)
                continue
            if section != ".text":
                # a jump table (IDO's switch): the labels a `jr` may reach
                m = WORD_RE.match(s)
                if m:
                    obj.jump_labels |= {w.strip() for w in m.group(1).split(",")
                                        if JUMP_LABEL_RE.match(w.strip())}
                continue
            m = GLABEL_RE.match(s)
            if m and func is not None and JUMP_LABEL_RE.match(m.group(1)):
                pending_labels.append(m.group(1))
                continue
            if m:
                label = m.group(1)
                func = Function(name=label, vram=None, obj=obj)
                obj.functions.append(func)
                pending_labels = []
                continue
            m = LOCAL_RE.match(s)
            if m:
                pending_labels.append(m.group(1))
                continue
            m = LINE_RE.match(raw)
            if not m:
                t = s.strip()
                if t and not t.startswith((".include", ".set", "#", "/*",
                                           "nonmatching", "endlabel",
                                           ".size", ".type", ".align")):
                    raise ParseError(f"{path}:{lineno}: can't parse {t!r}")
                continue
            if func is None:
                raise ParseError(f"{path}:{lineno}: instruction before a glabel")
            rom, vram, word = (int(m.group(i), 16) for i in (1, 2, 3))
            text_op, operands = m.group(4), m.group(5)
            try:
                insn = decode(word)
            except DecodeError as e:
                raise ParseError(f"{path}:{lineno}: {e}")
            if not text_matches(text_op, insn):
                raise ParseError(
                    f"{path}:{lineno}: decoded {insn.op} but listing says {text_op}")
            ln = Line(rom=rom, vram=vram, insn=insn, text_op=text_op,
                      operands=operands)
            mh = HI_RE.search(operands)
            if mh:
                ln.hi = mh.group(1)
            ml = LO_RE.search(operands)
            if ml:
                ln.lo = ml.group(1)
            if insn.kind == "branch" or insn.op in ("j", "jal"):
                ln.target = operands.split(",")[-1].strip()
            if func.vram is None:
                func.vram = vram
            for lab in pending_labels:
                func.labels[lab] = len(func.lines)
            pending_labels = []
            func.lines.append(ln)
    for fn in obj.functions:
        for i, ln in enumerate(fn.lines):
            if ln.insn.kind in ("branch", "jump") and i + 1 < len(fn.lines):
                fn.lines[i + 1].delay = True
    obj.functions = [fn for fn in obj.functions if fn.lines]
    for fn in obj.functions:
        trim_padding(fn)
    return obj


def is_terminator(ln):
    """An unconditional control transfer that doesn't come back here."""
    i = ln.insn
    return (i.op == "jr" or i.op == "j" or
            (i.op == "beq" and i.rs == 0 and i.rt == 0))


def trim_padding(fn):
    """Drop the nops that pad a function to 16 bytes after its return."""
    labelled = set(fn.labels.values())
    n = len(fn.lines)
    while (n > 2 and fn.lines[n - 1].insn.word == 0 and n - 1 not in labelled
           and not fn.lines[n - 1].delay):
        n -= 1
    fn.padding = len(fn.lines) - n
    del fn.lines[n:]
