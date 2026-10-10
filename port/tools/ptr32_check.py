#!/usr/bin/env python3
"""The structures whose pointers are still PTR32, against an allow-list
(P6: docs/PORT.md, "The LP64 build").

    ptr32_check.py BUILD [--allow FILE] [--list] [--update]

BUILD is an LP64 port build (-DPORT_LP64=ON): from its executable's DWARF,
every structure or union of the game's (declared under blastcorps/, port/
or the build's gen/: asm2c's types) with a member that is a 4-byte
pointer, or an array of them.  DWARF doesn't say __ptr32, so a pointer is
taken as one by the room it has: less than 8 bytes to the next member (or
the end), or an offset that isn't a multiple of 8; where it has 8 bytes on
an 8-byte boundary (the last member, before padding) its declaration's
line (or its typedef's) decides, by PTR32/__ptr32/ROMPTR.

The allow-list (port/tools/ptr32_allow.txt) names a structure a line.  A
structure with PTR32 members that isn't in it fails; one in it that the
build has without any fails too (the list must shrink with each P6
packet: what is left at the end is P7's and P8's).  A name the build
doesn't have (another version's types) is passed over.  --list prints
the members; --update writes the allow-list from the build.
"""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ALLOW = os.path.join(ROOT, "port", "tools", "ptr32_allow.txt")
PTR32_TEXT = re.compile(r"\bPTR32\b|__ptr32|\bROMPTR\b")


def dies_of(exe):
    out = subprocess.run(["llvm-dwarfdump", "--debug-info", exe], capture_output=True, text=True, check=True).stdout
    dies, stack, cur = {}, [], None
    for line in out.splitlines():
        m = re.match(r"^0x([0-9a-f]+):(\s+)(DW_TAG_\w+|NULL)", line)
        if m:
            off, depth, tag = int(m.group(1), 16), len(m.group(2)), m.group(3)
            while stack and stack[-1][1] >= depth:
                stack.pop()
            if tag == "NULL":
                cur = None
                continue
            cur = {"tag": tag, "kids": []}
            dies[off] = cur
            if stack:
                dies[stack[-1][0]]["kids"].append(off)
            stack.append((off, depth))
            continue
        m = re.match(r"^\s+(DW_AT_\w+)\s+\((.*)\)\s*$", line)
        if m and cur is not None:
            cur[m.group(1)] = m.group(2)
    return dies


def ref(d, attr="DW_AT_type"):
    m = re.match(r"0x([0-9a-f]+)", d.get(attr, ""))
    return int(m.group(1), 16) if m else None


def num(v):
    return int(v, 0) if v is not None and re.match(r"^(0x[0-9a-f]+|\d+)$", v) else None


def strip(v):
    return v.strip().strip('"') if v else v


_srclines = {}


def decl_text(d):
    f, n = strip(d.get("DW_AT_decl_file")), num(d.get("DW_AT_decl_line"))
    if not f or not n:
        return ""
    if f not in _srclines:
        try:
            _srclines[f] = open(f, errors="replace").read().split("\n")
        except OSError:
            _srclines[f] = []
    ls = _srclines[f]
    return ls[n - 1] if n <= len(ls) else ""


def pointer_of(dies, t):
    """(is a pointer, count of them, the typedefs on the way) of a member's type"""
    count, typedefs = 1, []
    seen = 0
    while t is not None and t in dies and seen < 32:
        seen += 1
        d = dies[t]
        tag = d["tag"]
        if tag == "DW_TAG_pointer_type":
            return True, count, typedefs
        if tag == "DW_TAG_typedef":
            typedefs.append(d)
        elif tag == "DW_TAG_array_type":
            for k in d["kids"]:
                c = dies[k]
                if c["tag"] == "DW_TAG_subrange_type":
                    n = num(c.get("DW_AT_count"))
                    if n is None and num(c.get("DW_AT_upper_bound")) is not None:
                        n = num(c.get("DW_AT_upper_bound")) + 1
                    count *= n or 0
        elif tag not in ("DW_TAG_const_type", "DW_TAG_volatile_type", "DW_TAG_restrict_type",
                         "DW_TAG_atomic_type"):
            return False, 0, typedefs
        t = ref(d)
    return False, 0, typedefs


def ours(path, build):
    if not path:
        return False
    p = os.path.abspath(path)
    return (p.startswith(os.path.join(ROOT, "blastcorps") + os.sep) or
            p.startswith(os.path.join(ROOT, "port") + os.sep) or p.startswith(build + os.sep)) and \
        "third_party" not in p and os.sep + "host" + os.sep not in p


def structs(build):
    """{name: [(member, offset, 'ptr32' or 'native', how)]} of every
    structure of ours with a pointer member"""
    exe = os.path.join(build, "blastcorps")
    dies = dies_of(exe)
    tdname = {}
    for off, d in dies.items():
        if d["tag"] == "DW_TAG_typedef" and ref(d) is not None:
            tdname.setdefault(ref(d), strip(d.get("DW_AT_name")))
    res, done = {}, set()
    for off, d in dies.items():
        if d["tag"] not in ("DW_TAG_structure_type", "DW_TAG_union_type"):
            continue
        size = num(d.get("DW_AT_byte_size"))
        if size is None or not ours(strip(d.get("DW_AT_decl_file")), build):
            continue
        name = strip(d.get("DW_AT_name")) or tdname.get(off)
        if not name:
            continue
        key = (name, strip(d.get("DW_AT_decl_file")), d.get("DW_AT_decl_line"))
        if key in done:
            continue
        done.add(key)
        mems = [dies[k] for k in d["kids"] if dies[k]["tag"] == "DW_TAG_member"]
        offs = sorted({num(m.get("DW_AT_data_member_location")) or 0 for m in mems})
        union = d["tag"] == "DW_TAG_union_type"
        out = []
        for m in mems:
            is_ptr, count, typedefs = pointer_of(dies, ref(m))
            if not is_ptr or not count:
                continue
            o = num(m.get("DW_AT_data_member_location")) or 0
            nxt = size if union else next((x for x in offs if x > o), size)
            room = (nxt - o) // count
            if room < 8:
                kind, how = "ptr32", f"{room} bytes"
            elif o % 8:
                kind, how = "ptr32", f"at {o:#x}"
            elif PTR32_TEXT.search(decl_text(m)) or any(PTR32_TEXT.search(decl_text(t)) for t in typedefs):
                kind, how = "ptr32", "its declaration"
            else:
                kind, how = "native", ""
            out.append((strip(m.get("DW_AT_name")) or "?", o, kind, how))
        if out:
            prev = res.setdefault(name, [])
            for x in out:
                if x not in prev:
                    prev.append(x)
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build")
    ap.add_argument("--allow", default=ALLOW)
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--update", action="store_true")
    a = ap.parse_args()
    build = os.path.abspath(a.build)
    cache = open(os.path.join(build, "CMakeCache.txt")).read()
    if not re.search(r"^PORT_LP64:BOOL=(ON|1|TRUE|YES)$", cache, re.M | re.I):
        raise SystemExit(f"{a.build}: not an LP64 build (-DPORT_LP64=ON): every pointer is 4 bytes there")
    res = structs(build)
    with32 = {n: ms for n, ms in res.items() if any(k == "ptr32" for _, _, k, _ in ms)}
    native = set(res) - set(with32)
    nmem = sum(sum(1 for m in ms if m[2] == "ptr32") for ms in with32.values())
    print(f"{len(with32)} structures with PTR32 members ({nmem} members), "
          f"{len(native)} with native pointers only")
    if a.list:
        for n in sorted(with32):
            ms = with32[n]
            print(f"  {n}: " + ", ".join(f"{m}@{o:#x}" + (f" ({how})" if how != f"{4} bytes" else "")
                                         for m, o, k, how in ms if k == "ptr32"))
    allow = set()
    head = []
    if os.path.exists(a.allow):
        for line in open(a.allow):
            if line.startswith("#"):
                head.append(line)
            w = line.split("#", 1)[0].split()
            if w:
                allow.add(w[0])
    if a.update:
        keep = {n for n in allow if n not in res}      # (other versions' types)
        with open(a.allow, "w") as f:
            f.writelines(head)
            for n in sorted(set(with32) | keep):
                f.write(n + "\n")
        print(f"wrote {os.path.relpath(a.allow)} ({len(set(with32) | keep)} structures)")
        return 0
    new = sorted(set(with32) - allow)
    stale = sorted(n for n in allow if n in native)
    if new:
        print(f"FAIL: {len(new)} not in the allow-list: " + ", ".join(new))
    if stale:
        print(f"FAIL: {len(stale)} in the allow-list without PTR32 now (take them out): " + ", ".join(stale))
    if not new and not stale:
        print(f"the allow-list's ({len(allow)} names, {len(set(with32) & allow)} of them in this build)")
    return 1 if new or stale else 0


if __name__ == "__main__":
    sys.exit(main())
