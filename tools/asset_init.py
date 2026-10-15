#!/usr/bin/env python3
"""The ROM's asset initializers, out of the repository (docs/DECOMPILING.md,
"ASSET_INIT").

  asset_init.py generate VERSION   write blastcorps/asset_init/VERSION/<module>/<object>/<NAME>.inc
                                   for every asset variable in port/tools/rom_assets.txt, from
                                   the ROM (data_c.py's dry run: the object's .data, typed by the
                                   file's declarations).  Then the normal split again.
  asset_init.py apply FILE...      rewrite a src file so that each asset variable's
                                   ASSET_INIT initializer is an #include of its .inc (and the
                                   #else keeps the zeroed form).  Idempotent.

A variable keeps its place in the file, so the compiler lays out .data as before.  The
include is "<module>/<object>/<NAME>.inc", found through -I asset_init/VERSION (the N64
build's Makefile), so the source is the same for every version.
"""
import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BLAST = ROOT / "blastcorps"
ASSETS = ROOT / "port/tools/rom_assets.txt"
MARK = "#ifdef ASSET_INIT"

DECL = re.compile(r"^[A-Za-z_][\w \*]*?[ \*]+(\w+)\s*(?:\[[^\]]*\]\s*)*=")


def die(msg):
    sys.exit(f"asset_init.py: {msg}")


PARTIAL = {}    # {(module, object): {NAME, ...}}: the variables only partly assets (sym+offset)


def read_assets():
    """{(module, object): {NAME, ...}} from rom_assets.txt (the base names of its entries)"""
    names = {}
    for line in ASSETS.read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        cols = line.split()
        module, sym, src = cols[0], cols[1], cols[4]
        if not src.startswith("src/"):      # (asm: asm2c.py's, zeroed in the port already)
            continue
        obj = Path(src).stem
        names.setdefault((module, obj), set()).add(sym.split("+")[0])
        if "+" in sym:
            PARTIAL.setdefault((module, obj), set()).add(sym.split("+")[0])
    return names


def strip_code(line):
    """the line without comments (for brace counting)"""
    line = re.sub(r"/\*.*?\*/", "", line)
    return re.sub(r"//.*", "", line)


def top_defs(lines):
    """[(start, end, NAME)]: the file's top-level variable definitions (start and end are
    line indexes, the end line holds the ';'); preprocessor lines don't count"""
    defs, depth, i, n = [], 0, 0, len(lines)
    while i < n:
        ln = lines[i]
        m = DECL.match(ln) if depth == 0 else None
        if m and not ln.lstrip().startswith("#"):
            start, d, opened, j = i, 0, False, i
            while j < n:
                t = strip_code(lines[j])
                if not lines[j].lstrip().startswith("#"):
                    d += t.count("{") - t.count("}")
                    opened = opened or "{" in t
                    if d == 0 and t.rstrip().endswith(";"):
                        break
                j += 1
            defs.append((start, j, m.group(1)))
            i = j + 1
            continue
        if not ln.lstrip().startswith("#"):
            depth += strip_code(ln).count("{") - strip_code(ln).count("}")
        i += 1
    return defs


def zeroed(block):
    """the block with each `#ifdef ASSET_INIT A #else B #endif` replaced by B"""
    out, k = [], 0
    while k < len(block):
        if block[k].strip() == MARK:
            e = next(x for x in range(k, len(block)) if block[x].strip() == "#else")
            f = next(x for x in range(e, len(block)) if block[x].strip() == "#endif")
            out.extend(block[e + 1:f])
            k = f + 1
            continue
        out.append(block[k])
        k += 1
    return out


def zero_elements(block):
    """an unguarded partial definition (a structure array whose first two fields in each
    element are the asset, dl and vtx): the same lines with those two fields zeroed"""
    text = "\n".join(block)
    i = text.index("=") + 1
    out, groups = [], []
    # the outer braces, then each element's first two groups
    j = text.index("{", i)
    stack = []
    for k in range(j, len(text)):
        c = text[k]
        if c == "{":
            stack.append(k)
        elif c == "}":
            o = stack.pop()
            groups.append((len(stack), o, k))
    # children of the outer group are the elements (depth 1); their first two children
    # (depth 2) are the fields
    by_elem = {}
    for d, o, c in groups:
        if d == 1:
            by_elem[o] = [(oo, cc) for dd, oo, cc in groups if dd == 2 and o < oo < cc]
    for elem, fields in sorted(by_elem.items()):
        fields = sorted(fields)[:2]
        for o, c in fields:
            out.append((o, c))
    for o, c in sorted(out, reverse=True):
        text = text[:o] + "{ 0 }" + text[c + 1:]
    return text.split("\n")


def apply_text(text, names, module, obj):
    """the rewritten text of a src file (None if nothing to do): its asset variables are
    the names; their definitions (guarded, or not) become the include form"""
    partial = PARTIAL.get((module, obj), set())
    lines = text.split("\n")
    spans = top_defs(lines)
    inside = set()
    for s, e, _ in spans:
        inside.update(range(s, e + 1))
    edits = []      # (first, last, new lines)

    def include_for(name):
        return f'#include "{module}/{obj}/{name}.inc"'

    def extern_for(first_line):
        # the declaration stays where data_c.py finds the variable's type (its initializer
        # is the .inc's)
        return "extern " + re.sub(r"\s*=.*$", "", first_line).rstrip() + ";"

    # top-level guards: #ifdef ASSET_INIT <definition> #else ... #endif
    guards = []     # (#ifdef, #endif) lines: a definition in one is the first pass's
    for k, ln in enumerate(lines):
        if ln.strip() != MARK or k in inside:
            continue
        e = next(x for x in range(k, len(lines)) if lines[x].strip() == "#else")
        f = next(x for x in range(e, len(lines)) if lines[x].strip() == "#endif")
        guards.append((k, f))
        branch = lines[k + 1:e]
        d = top_defs(branch)
        if len(d) == 1 and d[0][2] in names and not any(b.strip() == MARK for b in branch):
            edits.append((k, e - 1, [extern_for(branch[d[0][0]]), MARK, include_for(d[0][2])]))

    # definitions: guarded inside (a partial structure's fields), or not guarded at all
    for s, e, name in spans:
        if name not in names or any(k < s <= f for k, f in guards):
            continue        # (in a guard: the first pass has it)
        block = lines[s:e + 1]
        if any(b.strip() == MARK for b in block):
            new = [extern_for(block[0]), MARK, include_for(name), "#else"] + zeroed(block) + ["#endif"]
        elif name in partial:
            new = [extern_for(block[0]), MARK, include_for(name), "#else"] + zero_elements(block) + ["#endif"]
        else:
            new = [extern_for(block[0]), MARK, include_for(name), "#else",
                   extern_for(block[0])[len("extern "):-1] + " = { 0 };", "#endif"]
        edits.append((s, e, new))

    if not edits:
        return None
    for first, last, new in sorted(edits, reverse=True):
        lines[first:last + 1] = new
    return "\n".join(lines)


def apply_file(path, names, module, obj):
    return apply_text(path.read_text(), names[(module, obj)], module, obj)


def cmd_apply(args):
    names = read_assets()
    for f in args.files:
        p = Path(f).resolve()
        rel = p.relative_to(BLAST / "src")
        module, obj = rel.parts[0], rel.stem
        if (module, obj) not in names:
            print(f"asset_init.py: {f}: no asset variables", file=sys.stderr)
            continue
        new = apply_file(p, names, module, obj)
        if new is None:
            print(f"asset_init.py: {f}: already applied")
        else:
            p.write_text(new)
            print(f"asset_init.py: {f}: rewritten")


def switched_yaml(src_yaml, dst_yaml, objects):
    """a copy of a splat config with these objects' .data as asm (`data`)"""
    text = src_yaml.read_text()
    for obj in objects:
        text = re.sub(r"(\[0x[0-9A-Fa-f]+, )\.data(, [\w]+/%s\])" % re.escape(obj), r"\1data\2", text)
    dst_yaml.write_text(text)


def cmd_generate(args):
    version = args.version
    names = read_assets()
    out_root = BLAST / "asset_init" / version
    py = sys.executable
    for module in sorted({m for m, _ in names}):
        objects = sorted(o for m, o in names if m == module)
        main = BLAST / f"{module}.{version}.yaml"
        tmp = BLAST / f".asset_init.{module}.{version}.yaml"
        switched_yaml(main, tmp, objects)
        try:
            run = dict(cwd=BLAST, check=True)
            subprocess.run([py, str(ROOT / "tools/split.py"), tmp.name], **run)
            for obj in objects:
                ref = BLAST / "asm/data" / module / f"{obj}.data.s"
                r = subprocess.run([py, str(ROOT / "tools/data_c.py"), module, obj,
                                    "--dry-run", "--version", version, "--ref", str(ref)],
                                   cwd=BLAST, capture_output=True, text=True)
                if r.returncode:
                    die(f"data_c.py {module}/{obj} failed:\n{r.stderr[-2000:]}")
                lines = r.stdout.split("\n")
                defs = {name: (s, e) for s, e, name in top_defs(lines)}
                for name in sorted(names[(module, obj)]):
                    if name not in defs:
                        die(f"{module}/{obj}: {name} isn't in the object's .data (data_c.py)")
                    s, e = defs[name]
                    dst = out_root / module / obj / f"{name}.inc"
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    text = "\n".join(lines[s:e + 1]) + "\n"
                    if not dst.exists() or dst.read_text() != text:    # (a rewrite rebuilds its users)
                        dst.write_text(text)
                print(f"asset_init.py: {module}/{obj}: {len(names[(module, obj)])} variables")
        finally:
            tmp.unlink(missing_ok=True)
            for obj in objects:
                (BLAST / "asm/data" / module / f"{obj}.data.s").unlink(missing_ok=True)
            # the normal split again (also when generating failed): the asm of these
            # objects is C's again
            subprocess.run([py, str(ROOT / "tools/split.py"), main.name], **run)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    g = sub.add_parser("generate")
    g.add_argument("version")
    g.set_defaults(fn=cmd_generate)
    a = sub.add_parser("apply")
    a.add_argument("files", nargs="+")
    a.set_defaults(fn=cmd_apply)
    args = ap.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
