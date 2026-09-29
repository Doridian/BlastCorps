#!/usr/bin/env python3
"""Replace a C file's extern .rodata symbols with the literals they hold.

  tools/inline_rodata.py <module> <file> [version]    e.g. hd_code 1D990

For switching a C file to own its .rodata: while the .rodata is still an asm
`rodata` subsegment, every string or float constant the C uses is an
`extern` naming an address in blastcorps/asm/data/<module>/<file>.rodata.s.
This rewrites each use of an `extern char x[]`, `extern f32 x` or
`extern f64 x` as the literal at that address (a string, `1.5f` or `1.5`)
and drops the extern, so IDO emits the constant itself.  Then change the
subsegment to `.rodata` and re-extract; the GLOBAL_ASM functions get the
rest (see tools/split.py).  The sha1 decides whether that was right.

A symbol used more than once is reported and left alone: IDO emits one
constant per use, so which uses are literals depends on the original.
"""
import re
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent / "blastcorps"


def image(rodata):
    """{address: byte} for everything in a splat rodata .s."""
    mem = {}
    for name, body in re.findall(r"^dlabel (\w+)\n(.*)$", rodata, re.M):
        m = re.search(r"_([0-9A-F]{8})$", name)
        if not m:
            continue
        addr = int(m.group(1), 16)
        kind, _, rest = body.partition(" ")
        if kind == ".ascii":
            raw = rest[1:-1].encode().decode("unicode_escape").encode("latin-1")
        else:
            vals = [v.strip() for v in rest.split(",")]
            fmt = {".word": ">I", ".float": ">f", ".double": ">d", ".short": ">H", ".byte": ">B"}.get(kind)
            if fmt is None or any(not re.fullmatch(r"-?[0-9A-Fa-fx.e+-]+|inf|nan", v) for v in vals):
                continue  # pointers
            conv = int if kind in (".word", ".short", ".byte") else float
            raw = b"".join(struct.pack(fmt, conv(v, 0) if conv is int else conv(v)) for v in vals)
        for k, b in enumerate(raw):
            mem[addr + k] = b
    return mem


def literal(mem, addr, ctype):
    get = lambda a, n: bytes(mem[a + k] for k in range(n)) if all(a + k in mem for k in range(n)) else None
    if ctype == "char":
        out, a = [], addr
        while a in mem and mem[a] != 0:
            out.append(mem[a])
            a += 1
        if a not in mem or not out:
            return None
        esc = {0x0A: "\\n", 0x22: '\\"', 0x5C: "\\\\"}
        s = ""
        for i, c in enumerate(out):
            if c in esc:
                s += esc[c]
            elif 0x20 <= c < 0x7F:
                s += chr(c)
            else:
                s += f"\\{c:03o}"
        return f'"{s}"'
    if ctype in ("f32", "f64"):
        raw = get(addr, 4 if ctype == "f32" else 8)
        if raw is None:
            return None
        if ctype == "f32":
            v = struct.unpack(">f", raw)[0]
            for prec in range(1, 20):
                r = f"{v:.{prec}g}"
                if struct.pack(">f", float(r)) == raw:
                    break
        else:
            v = struct.unpack(">d", raw)[0]
            r = repr(v)
        if not re.search(r"[.e]", r):
            r += ".0"
        return r + ("f" if ctype == "f32" else "")
    return None


def main():
    module, name = sys.argv[1:3]
    version = sys.argv[3] if len(sys.argv) > 3 else "us.v11"
    mem = image((ROOT / "asm" / "data" / module / f"{name}.rodata.s").read_text())
    c_path = ROOT / "src" / module / f"{name}.c"
    src = c_path.read_text()

    externs = {m.group(2): m.group(1) for m in re.finditer(
        r"^extern (?:const )?(char|f32|f64) (D_[0-9A-F]{8})(?:\[\])?;(?:\s*/\*.*\*/)?$", src, re.M)}
    body = re.sub(r"^extern [^;]*;[^\n]*\n", "", src, flags=re.M)
    uses = Counter(re.findall(r"\b(D_[0-9A-F]{8})\b", body))
    done = []
    for sym, ctype in externs.items():
        if not uses[sym]:
            continue
        lit = literal(mem, int(sym[2:], 16), ctype)
        if lit is None:
            continue
        if uses[sym] > 1:
            print(f"{sym}: used {uses[sym]} times, left as extern", file=sys.stderr)
            continue
        src = re.sub(rf"^extern [^;]*\b{sym}\b[^;]*;[^\n]*\n", "", src, flags=re.M)
        src = re.sub(rf"\b{sym}\b", lambda m: lit, src)
        done.append(sym)
    c_path.write_text(src)
    print(f"{c_path}: inlined {len(done)}: {' '.join(done)}")


main()
