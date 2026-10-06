#!/usr/bin/env python3
"""The game's data files as LLVM IR, for the movable build's arena link.

  asm2ll.py [--split] OUT.ll IN.s SYMS.txt [TRIPLE DATALAYOUT]

IN.s is asm2x86.py's output (x86 assembly, one section per data file, in
the build's byte order but for the symbolic words, which BEPass's
`port_bswap32` fixes at run time); OUT.ll has each section as a global of
the same name, a packed struct of byte runs and the symbolic words as
`i32 ptrtoint (ptr @sym to i32)`.  port-arena (bepass/Arena.cpp) places it
at its N64 address, resolves the symbols to theirs and writes the words in
the build's byte order, so no fixup is left for run time.  SYMS.txt
(gen_syms.py table) says which names are functions.  (docs/PORT.md,
"Movable memory".)  TRIPLE and DATALAYOUT are the N64 side's (CMake asks
clang), so that llvm-link finds the modules alike.  --split (the scattered
layout, PORT_SCATTER) makes each global label a global of its own, from
it to the next one, instead of each file one.
"""

import re
import sys


def unescape(s):
    """a GNU as string literal's bytes"""
    out = bytearray()
    k = 0
    while k < len(s):
        c = s[k]
        if c != "\\":
            out += c.encode("latin-1")
            k += 1
            continue
        k += 1
        c = s[k]
        simple = {"n": 10, "t": 9, "r": 13, "b": 8, "f": 12, "v": 11, "\\": 92, '"': 34, "'": 39}
        if c in simple:
            out.append(simple[c])
            k += 1
        elif c in "01234567":
            m = re.match(r"[0-7]{1,3}", s[k:])
            out.append(int(m.group(0), 8) & 0xFF)
            k += len(m.group(0))
        elif c == "x":
            m = re.match(r"[0-9A-Fa-f]+", s[k + 1:])
            out.append(int(m.group(0), 16) & 0xFF)
            k += 1 + len(m.group(0))
        else:
            out += c.encode("latin-1")
            k += 1
    return bytes(out)


def strings(rest):
    """the string literals of an .ascii line"""
    return [unescape(m.group(1)) for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', rest)]


class Section:
    def __init__(self, name, bss, start=0):
        self.name, self.bss = name, bss
        self.items = []     # bytes or ("sym", name)
        self.size = 0
        self.start = start  # (--split: where in the file's section it starts)

    def add(self, b):
        if not b:
            return
        if self.items and isinstance(self.items[-1], bytearray):
            self.items[-1] += b
        else:
            self.items.append(bytearray(b))
        self.size += len(b)

    def sym(self, name):
        self.items.append(("sym", name))
        self.size += 4


def parse(src, split=False):
    secs = []
    cur = None
    skip = 0
    globl = set()
    for raw in open(src):
        line = raw.strip()
        if line.startswith("/*") or not line:
            continue
        if line.startswith(".pushsection"):
            skip += 1
            continue
        if line.startswith(".popsection"):
            skip -= 1
            continue
        if skip:
            continue
        m = re.match(r"^\.section\s+\.(n64b?)\.([^,\s]+)", line)
        if m:
            cur = Section(m.group(2), m.group(1) == "n64b")
            secs.append(cur)
            continue
        m = re.match(r"^([.\w$]+):\s*(.*)$", line)
        if m:
            if split and cur is not None and m.group(1) in globl and m.group(1) != cur.name:
                cur = Section(m.group(1), cur.bss, cur.start + cur.size)
                secs.append(cur)
            line = m.group(2)
            if not line:
                continue
        m = re.match(r"^(\.\w+)\s*(.*)$", line)
        if not m:
            sys.exit(f"asm2ll.py: {src}: can't parse {raw!r}")
        d, rest = m.group(1), m.group(2)
        if d == ".globl":
            globl.add(rest.strip())
            continue
        if cur is None:
            sys.exit(f"asm2ll.py: {src}: data outside a section")
        if d == ".byte":
            cur.add(bytes(int(a, 0) & 0xFF for a in rest.split(",")))
        elif d == ".space":
            cur.add(bytes(int(rest.split(",")[0], 0)))
        elif d in (".ascii", ".asciz", ".string"):
            for b in strings(rest):
                cur.add(b + (b"\0" if d != ".ascii" else b""))
        elif d == ".incbin":
            cur.add(open(strings(rest)[0], "rb").read())
        elif d == ".balign":
            n = int(rest, 0)
            cur.add(bytes(-(cur.start + cur.size) % n))
        elif d == ".p2align":
            n = 1 << int(rest, 0)
            cur.add(bytes(-(cur.start + cur.size) % n))
        elif d == ".long":
            for a in rest.split(","):
                a = a.strip()
                if re.match(r"^-?(0x[0-9A-Fa-f]+|\d+)$", a):
                    sys.exit(f"asm2ll.py: {src}: numeric .long {a} (asm2x86.py writes numbers as bytes)")
                cur.sym(a)
        else:
            sys.exit(f"asm2ll.py: {src}: unsupported directive {d}")
    return secs


def ll_bytes(b):
    return 'c"' + "".join(chr(x) if 32 <= x < 127 and x not in (34, 92) else f"\\{x:02X}" for x in b) + '"'


def main():
    split = "--split" in sys.argv
    argv = [a for a in sys.argv if a != "--split"]
    dst, src, syms = argv[1], argv[2], argv[3]
    kinds = {}
    for line in open(syms):
        p = line.split()
        kinds[p[0]] = p[2]
    secs = parse(src, split)
    defined = {s.name for s in secs}
    out = [f"; generated by port/tools/asm2ll.py from {src}"]
    if len(argv) > 5:
        out += [f'target datalayout = "{argv[5]}"', f'target triple = "{argv[4]}"']
    decls = set()
    for s in secs:
        types, vals = [], []
        for it in s.items:
            if isinstance(it, bytearray):
                types.append(f"[{len(it)} x i8]")
                vals.append(f"[{len(it)} x i8] " + (ll_bytes(it) if any(it) else "zeroinitializer"))
            else:
                n = it[1]
                if n not in defined:
                    decls.add(n)
                vals.append(f"i32 ptrtoint (ptr @\"{n}\" to i32)")
                types.append("i32")
        if not types:
            types, vals = ["[0 x i8]"], ["[0 x i8] zeroinitializer"]
        init = "<{ " + ", ".join(vals) + " }>"
        if s.bss:
            init = "zeroinitializer"
        out.append(f"@\"{s.name}\" = global <{{ {', '.join(types)} }}> {init}, section \"port.asmdata\", align 1")
    for n in sorted(decls):
        if kinds.get(n) == "F":
            out.append(f"declare void @\"{n}\"()")
        else:
            out.append(f"@\"{n}\" = external global i8")
    with open(dst, "w") as f:
        f.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
