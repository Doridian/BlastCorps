#!/usr/bin/env python3
"""Compare the struct layouts of two builds of the port, from their DWARF.

  layout_cmp.py A.exe B.exe [--only-game]

Every struct and union type (by its name, or the typedef naming an anonymous
one) whose size or member offsets differ between the two, with the members
that moved.  Used for the LP64 build (docs/PORT.md, "The LP64 build")
against a 64-bit one, whose layout is the N64's: what the LP64 build lays
out as the host does, and only that, should be listed.
"""
import re
import subprocess
import sys


def layouts(exe, only_game):
    out = subprocess.run(["llvm-dwarfdump", "--debug-info", exe], capture_output=True, text=True, check=True).stdout
    dies = {}           # offset -> [tag, attrs, children offsets, depth]
    stack = []
    cu_file = None
    cur = None
    for line in out.splitlines():
        m = re.match(r"^0x([0-9a-f]+):(\s+)(DW_TAG_\w+|NULL)", line)
        if m:
            off, depth, tag = int(m.group(1), 16), len(m.group(2)), m.group(3)
            while stack and stack[-1][1] >= depth:
                stack.pop()
            if tag == "NULL":
                cur = None
                continue
            cur = {"tag": tag, "kids": [], "cu": cu_file}
            dies[off] = cur
            if stack:
                dies[stack[-1][0]]["kids"].append(off)
            stack.append((off, depth))
            continue
        m = re.match(r"^\s+(DW_AT_\w+)\s+\((.*)\)\s*$", line)
        if m and cur is not None:
            k, v = m.group(1), m.group(2)
            cur[k] = v
            if cur["tag"] == "DW_TAG_compile_unit" and k == "DW_AT_name":
                cu_file = v.strip('"')
                cur["cu"] = cu_file
    names = {}
    for off, d in dies.items():
        if d["tag"] == "DW_TAG_typedef" and "DW_AT_type" in d:
            t = re.match(r"0x([0-9a-f]+)", d["DW_AT_type"])
            if t:
                names.setdefault(int(t.group(1), 16), d.get("DW_AT_name", "").strip('"'))
    res = {}
    for off, d in dies.items():
        if d["tag"] not in ("DW_TAG_structure_type", "DW_TAG_union_type") or "DW_AT_byte_size" not in d:
            continue
        if only_game and not re.search(r"blastcorps/src/|port/src/", d["cu"] or ""):
            continue
        name = d.get("DW_AT_name", "").strip('"') or names.get(off)
        if not name:
            continue
        mem = []
        for k in d["kids"]:
            c = dies[k]
            if c["tag"] == "DW_TAG_member":
                loc = c.get("DW_AT_data_member_location", "0")
                mem.append((c.get("DW_AT_name", "?").strip('"'), int(loc, 0) if re.match(r"^(0x)?[0-9a-f]+$", loc) else loc))
        res.setdefault(name, (int(d["DW_AT_byte_size"], 0), mem))
    return res


def main():
    only = "--only-game" in sys.argv
    a, b = [x for x in sys.argv[1:] if not x.startswith("--")]
    la, lb = layouts(a, only), layouts(b, only)
    n = 0
    for name in sorted(set(la) & set(lb)):
        (sa, ma), (sb, mb) = la[name], lb[name]
        if sa == sb and ma == mb:
            continue
        n += 1
        moved = [f"{x[0]} {x[1]:#x}->{y[1]:#x}" for x, y in zip(ma, mb) if x != y and isinstance(x[1], int) and isinstance(y[1], int)]
        print(f"{name}: {sa:#x} -> {sb:#x}  " + ", ".join(moved[:6]) + (" ..." if len(moved) > 6 else ""))
    print(f"{n} types differ ({len(set(la) & set(lb))} in both)")


if __name__ == "__main__":
    main()
