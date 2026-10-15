#!/usr/bin/env python3
"""The front end's variables, for the plain build's overlay (port/src/overlay.c).

  gen_fevars.py OUTDIR --nm NM --sources SRC... --objects OBJ...

On the N64 the front end's .data and .bss are inflated and cleared again
whenever the game loads it (hd_code 46C20.c, func_8028B3E0), so its variables
start over as they were.  In the plain build they are ordinary globals,
wherever the linker put them, and overlay.c puts each back from a copy made at
startup.  This lists them, in C, so that the compiler gives each one's
address and size (sizeof: right with any compiler, any layout, and under the
sanitizers, whose objects' symbol sizes take in their redzones):

  - the front end's sources (its C, src/hd_front_end/*.c, and its asm data
    as C, asm2c.py's *hd_front_end_* files) are compiled once as they are
    (OBJ, CMake's fe_scan): their symbol tables, read with NM (any nm), name
    the data each defines (D/B: written; R: read-only);
  - OUTDIR/<source> is each source again, #included by its basename (CMake's
    fe_c has the source directories on its include path, so the file is
    found wherever the build and the checkout are, and the snapshot holds no
    absolute paths), followed by its table:
    a row {address, sizeof, N64 address, N64 room, its copy} a variable,
    then a row of zeros, port_fe_vars_<n>[]; the copies are the file's
    own buffer (none for read-only data).  These are what the build links;
  - OUTDIR/fe_vars.c lists the tables, port_fe_tables[], NULL-terminated.

The N64 address and room come from the version's N64 link
(hd_front_end.<version>.elf, through PORT_N64_NM): overlay.c's copy of the
image in the front end's area puts as much of a variable as its N64 room
holds where the N64 has it (its N64 size, or up to the next N64 variable, as
port-arena's scattered layout has it); 0 where it has none.
"""

import argparse
import bisect
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")
VERSION = os.environ.get("PORT_VERSION", "us.v11")
N64_NM = os.environ.get("PORT_N64_NM", "mips-linux-gnu-nm")
NAME = re.compile(r"^_?[A-Za-z_]\w*$")
# what the compiler makes for itself in an object's symbol table: Mach-O's
# section labels (ltmp0) and constant pools (lCPI0_0); never a variable's name (its
# string literals, l_.str, have a dot and are no name at all)
COMPILER_LOCAL = re.compile(r"^(ltmp\d+|lCPI\d+_\d+)$")


def is_macho(obj):
    with open(obj, "rb") as f:
        return f.read(4) in (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xfe\xed\xfa\xce")


def defined_data(nm, obj):
    """[(name, writable)] of the data the object defines, in the order nm
    lists them; fails on a variable of its own (static) the table can't
    name"""
    res = subprocess.run([nm, "--defined-only", obj], capture_output=True, text=True, check=True).stdout
    out, bad = [], []
    macho = is_macho(obj)
    for line in res.splitlines():
        p = line.split()
        if len(p) < 2 or p[-2] not in "dDbBrRvVsS":
            continue
        name, t = p[-1], p[-2]
        if name.startswith(("__odr_asan", "__asan")) or not NAME.match(name):
            continue        # (the compiler's own: string literals, the sanitizers' tables)
        if t.islower():
            if t in "dbs" and not COMPILER_LOCAL.match(name):
                bad.append(name)
            continue
        if macho and name.startswith("_"):
            name = name[1:]     # (Mach-O's leading underscore: of a C name, whatever the host)
        out.append((name, t in "DBVS"))
    if bad:
        sys.exit(f"gen_fevars.py: {obj} has variables of its own (static) the overlay can't put back: "
                 + ", ".join(bad))
    return out


def n64_symbols():
    elf = os.path.join(BLAST, "build", f"hd_front_end.{VERSION}.elf")
    res = subprocess.run([N64_NM, "-S", elf], capture_output=True, text=True, check=True).stdout
    out = {}
    for line in res.splitlines():
        p = line.split()
        if len(p) == 4:
            out.setdefault(p[3], (int(p[0], 16), int(p[1], 16)))
        elif len(p) == 3:
            out.setdefault(p[2], (int(p[0], 16), 0))
    return out


def write_if_changed(path, text):
    try:
        if open(path).read() == text:
            return
    except OSError:
        pass
    with open(path, "w") as f:
        f.write(text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--nm", default="nm")
    ap.add_argument("--sources", nargs="+", required=True)
    ap.add_argument("--objects", nargs="+", required=True)
    a = ap.parse_args()
    srcs = [s for arg in a.sources for s in arg.split(";") if s]
    objs = [o for arg in a.objects for o in arg.split(";") if o]
    n64 = n64_symbols()
    per_src = []
    for s in srcs:
        b = os.path.basename(s)
        o = [x for x in objs if os.path.basename(x) in (b + ".o", b + ".obj")]
        if len(o) != 1:
            sys.exit(f"gen_fevars.py: {len(o)} objects for {s}")
        # (a name asm2c.py puts inside another variable, `.set NAME, VAR +
        # OFF`, is that variable's, which is put back whole: not one of its
        # own, and not a C declaration's size)
        inner = set(re.findall(r'\.set " PORT_ULP "([A-Za-z_]\w*),', open(s).read()))
        per_src.append((s, [(n, w) for n, w in defined_data(a.nm, o[0]) if n not in inner]))

    # an N64 variable's room, as port-arena has it (bepass/Arena.cpp): its
    # size or up to the next sized one, and never past the next variable
    # defined here (a data island's labels have no size)
    sized = sorted(at for at, n in n64.values() if n)
    defined = sorted(n64[n][0] for _, vs in per_src for n, _ in vs if n in n64)

    def room(name):
        at, n = n64[name]
        k = bisect.bisect_right(sized, at)
        nxt = sized[k] if k < len(sized) else at + n
        k = bisect.bisect_right(defined, at)
        dfn = defined[k] if k < len(defined) else max(at + n, nxt)
        return min(max(n, nxt - at), dfn - at)

    os.makedirs(a.outdir, exist_ok=True)
    tables, nvars, nw = [], 0, 0
    for i, (s, vs) in enumerate(per_src):
        lines = ["/* Generated by port/tools/gen_fevars.py: not to be committed.  The front end's",
                 f"   {os.path.relpath(s, ROOT)}, and its variables for port/src/overlay.c. */",
                 f'#include <{os.path.basename(s)}>', ""]
        w = [n for n, wr in vs if wr]
        if w:
            lines.append(f"static u8 port_fe_snap_{i}[" + " + ".join(f"sizeof {n}" for n in w) + "];")
        lines.append(f"const PortFeVar port_fe_vars_{i}[] = {{")
        off = []
        for n, wr in vs:
            at, rm = (n64[n][0], room(n)) if n in n64 else (0, 0)
            if wr:
                snap = f"port_fe_snap_{i}" + "".join(f" + sizeof {x}" for x in off)
                off.append(n)
            else:
                snap = "0"
            lines.append(f"    {{ (u8 *)&{n}, sizeof {n}, 0x{at:08X}, 0x{rm:X}, {snap} }},")
        lines += ["    { 0, 0, 0, 0, 0 },", "};", ""]
        write_if_changed(os.path.join(a.outdir, os.path.basename(s)), "\n".join(lines))
        tables.append(f"port_fe_vars_{i}")
        nvars += len(vs)
        nw += len(w)
    lines = ["/* Generated by port/tools/gen_fevars.py: not to be committed.  The front end's",
             "   variables' tables (port/src/overlay.c). */", '#include "common.h"', ""]
    lines += [f"extern const PortFeVar {t}[];" for t in tables]
    lines += ["", "const PortFeVar *const port_fe_tables[] = {"] + [f"    {t}," for t in tables] + ["    0,", "};", ""]
    write_if_changed(os.path.join(a.outdir, "fe_vars.c"), "\n".join(lines))
    print(f"gen_fevars.py: {nvars} variables of the front end in {len(per_src)} files ({nw} written)")


if __name__ == "__main__":
    main()
