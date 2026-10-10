#!/usr/bin/env python3
"""Whether the N64 side's declarations of each D_/func_ name agree (P6:
docs/PORT.md, "The LP64 build").

    decl_check.py BUILD [--version V|all] [--allow FILE] [--list [CLASS]] [--update]

BUILD is a configured LP64 port build (-DPORT_LP64=ON), where PTR32 is
__ptr32 and a pointer without it is 8 bytes: clang's AST of every N64-side
file (n64_tus.py: the game's C, port/src, port/engine, port/libaudio and,
for the build's own version, asm2c's data), each declaration and definition
of a name with its type.  Declarations in different files that the
compiler never sees together can disagree, and in the LP64 build such a
pair can be 4 bytes against 8: dropping PTR32 on one declaration of a
variable and not the others is silent.  Each name whose declarations
differ is put in a class:

  mix     a pointer in one, an integer in another (a variable, or a
          function's parameter or result)
  width   a PTR32 pointer in one, a native one in another
  asm     a pointer in the C, a word in a structure of asm2c's in its
          data (port/tools/asm2c.py: 4 bytes, whatever the C's pointer)
  type    any other difference (an integer's width or sign, a struct
          against bytes); not pointer-width, only listed

mix, width and asm are checked against the allow-list (port/tools/
decl_allow.txt, `class name`: P6-E fixes them and empties it); type is
counted.  Also counted: the pointer variables declared in more than one
file, asm2c's own declarations aside (P6-E declares each once, in a
header).  --list prints the names (of a class: mix, width, asm, type,
multi) with each declaration's type and place; --update writes the
allow-list from what is found.
"""
import argparse
import collections
import concurrent.futures as cf
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import n64_tus  # noqa: E402

ROOT = n64_tus.ROOT
VERSIONS = n64_tus.VERSIONS
ALLOW = os.path.join(ROOT, "port", "tools", "decl_allow.txt")
NAME = re.compile(r"^(?:D_|func_)[0-9A-Fa-f]{8}(?:_\w+)?$")
LOC = re.compile(r"(/[^\s:<>,']+|<built-in>|<scratch space>|line|col):(\d+)(?::(\d+))?")
DECL = re.compile(r"^([| `]*)[|`]-(VarDecl|FunctionDecl|ParmVarDecl) 0x[0-9a-f]+ (.*)$")
INTS = re.compile(r"^(?:(?:const|volatile|signed|unsigned|char|short|int|long|_Bool)\s*)+$|^enum ")


def norm(t):
    """a type with its PTR32 pointers marked: `*32` for a __ptr32 one"""
    t = re.sub(r"\*\s*__ptr32\s+__uptr", "*\x01", t)          # the spelled type: after its *
    while "__uptr __ptr32" in t:                               # the canonical one: before it
        i = t.index("__uptr __ptr32")
        t = t[:i] + t[i + len("__uptr __ptr32"):]
        j = t.find("*", i)
        if j >= 0:
            t = t[:j + 1] + "\x01" + t[j + 1:]
    t = t.replace("\x01", "32")
    t = re.sub(r"\s+", " ", t).strip()
    return re.sub(r"\s*\*", " *", t).replace("( *", "(*").strip()


def category(t):
    """ptr32, ptr, int or other, of a (normalized) object type"""
    t = re.sub(r"(\s*\[\d*\])+$", "", t).strip()
    if re.search(r"\*32$", t) or re.search(r"\(\*32\)\s*\(", t):
        return "ptr32"
    if t.endswith("*") or re.search(r"\(\*\)\s*\(", t):
        return "ptr"
    if INTS.match(t) or t in ("u8", "s8", "u16", "s16", "u32", "s32", "u64", "s64"):
        return "int"
    return "other"


def unbound(t):
    return re.sub(r"\[\d+\]", "[]", t)


def split_params(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def parse(text):
    """[(name, kind, type, file, line, storage)] from clang's AST dump"""
    out = []
    cur_file, cur_line = None, 0
    fn = None           # the FunctionDecl whose parameters follow: [entry, depth, params]
    for raw in text.split("\n"):
        raw = re.sub(r"'[^']*'", "''", raw) if "(unnamed" in raw or "(anonymous" in raw else raw
        m = DECL.match(raw)
        begin = None
        for lm in LOC.finditer(raw[:raw.find("> ") + 1] if m else raw):
            what, a = lm.group(1), int(lm.group(2))
            if what.startswith("/"):
                cur_file, cur_line = what, a
            elif what == "line":
                cur_line = a
            if begin is None:
                begin = (cur_file, cur_line)
        # (the name's location after "> ", and anything later on the line)
        if m:
            rest = raw[raw.find("> ") + 2:] if "> " in raw else ""
            for lm in LOC.finditer(rest):
                what, a = lm.group(1), int(lm.group(2))
                if what.startswith("/"):
                    cur_file, cur_line = what, a
                elif what == "line":
                    cur_line = a
        if not m:
            continue
        depth, kind, tail = len(m.group(1)), m.group(2), m.group(3)
        tm = re.search(r"(?:^|\s)(\w+) '([^']*)'(?::'([^']*)')?(.*)$", tail)
        if kind == "ParmVarDecl":
            if fn is not None and depth == fn[1] + 2:
                t = re.search(r"'([^']*)'(?::'([^']*)')?", tail)
                if t:
                    fn[2].append(norm(t.group(2) or t.group(1)))
            continue
        if fn is not None:
            fn = None
        if not tm:
            continue
        name, spelled, canon, attrs = tm.group(1), tm.group(2), tm.group(3), tm.group(4)
        if not NAME.match(name) or begin is None or begin[0] is None:
            continue
        if kind == "VarDecl" and depth > 0 and not re.search(r" extern\b", attrs):
            continue        # (a local)
        storage = "def" if (kind == "VarDecl" and not re.search(r" extern\b", attrs)) else "decl"
        if kind == "VarDecl":
            entry = [name, "var", norm(canon or spelled), begin[0], begin[1], storage]
            out.append(entry)
        else:
            # the result from the spelled type, the parameters from their own lines
            ret = norm(spelled.split("(", 1)[0]) if "(" in spelled else norm(spelled)
            params_spelled = spelled[spelled.find("(") + 1:spelled.rfind(")")] if "(" in spelled else ""
            entry = [name, "func", ret, begin[0], begin[1], storage, [norm(p) for p in split_params(params_spelled)]]
            out.append(entry)
            fn = [entry, depth, []]
            entry.append(fn[2])
    res = []
    for e in out:
        if e[1] == "func":
            params = e[7] if len(e) > 7 and e[7] else e[6]
            if params == ["void"]:
                params = []
            res.append((e[0], "func", e[2] + " (" + ", ".join(params) + ")", e[3], e[4], e[5], tuple([e[2]] + params)))
        else:
            res.append((e[0], "var", e[2], e[3], e[4], e[5], (e[2],)))
    return res


def dump(argv):
    r = subprocess.run(argv + ["-fsyntax-only", "-w", "-Xclang", "-ast-dump", "-fno-color-diagnostics"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return None, r.stderr[-2000:]
    return parse(r.stdout), None


def check(build, version, jobs):
    units = n64_tus.tus(build, version)
    decls = collections.defaultdict(set)    # name -> {(kind, type, file, line, storage, parts)}
    errors = []
    with cf.ThreadPoolExecutor(jobs) as ex:
        for (k, f, _), (res, err) in zip(units, ex.map(lambda u: dump(u[2]), units)):
            if res is None:
                errors.append(f"{os.path.relpath(f, ROOT)}: {err}")
                continue
            for name, kind, t, file, line, storage, parts in res:
                decls[name].add((kind, t, os.path.relpath(file, ROOT), line, storage, parts))
    found = {}
    for name, ds in decls.items():
        kinds = {d[0] for d in ds}
        if len({unbound(d[1]) for d in ds}) <= 1:
            continue
        cls = "type"
        if kinds == {"var"} or kinds == {"func"}:
            width = max(len(d[5]) for d in ds)
            for i in range(width):
                cats = {category(d[5][i]) for d in ds if i < len(d[5])}
                if "int" in cats and cats & {"ptr", "ptr32"}:
                    cls = "mix"
                    break
                if {"ptr", "ptr32"} <= cats:
                    cls = "width"
            if cls == "type" and kinds == {"var"}:
                data = {category(d[1]) for d in ds if "/gen/data_c/" in "/" + d[2]}
                c = {category(d[1]) for d in ds if "/gen/data_c/" not in "/" + d[2]}
                if "other" in data and c & {"ptr", "ptr32"}:
                    cls = "asm"
        found[name] = (cls, sorted(ds, key=lambda d: (d[2], d[3])))
    # the pointer variables declared in more than one place
    multi = {}
    for name, ds in decls.items():
        vs = [d for d in ds if d[0] == "var"]
        if not vs or not any(category(d[1]) in ("ptr", "ptr32") for d in vs):
            continue
        places = {d[2] for d in vs if d[4] == "decl" and "/gen/" not in "/" + d[2]}
        if len(places) > 1:
            multi[name] = sorted(vs, key=lambda d: (d[2], d[3]))
    return found, multi, errors


def read_allow(path):
    allow = {}
    if os.path.exists(path):
        for line in open(path):
            w = line.split("#", 1)[0].split()
            if len(w) >= 2:
                allow[w[1]] = w[0]
    return allow


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build")
    ap.add_argument("--version", default=None, help="a version, or all (default: the build's)")
    ap.add_argument("--allow", default=ALLOW)
    ap.add_argument("--list", nargs="?", const="all", default=None, metavar="CLASS")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    a = ap.parse_args()
    if not n64_tus.is_lp64(a.build):
        raise SystemExit(f"{a.build}: not an LP64 build (-DPORT_LP64=ON): PTR32 is empty there")
    versions = VERSIONS if a.version == "all" else [a.version or n64_tus.build_version(a.build)]
    allow = read_allow(a.allow)
    fails, seen = 0, {}
    for v in versions:
        found, multi, errors = check(a.build, v, a.jobs)
        if errors:
            fails += 1
            print(f"{v}: clang failed on {len(errors)} files:\n  " + "\n  ".join(errors[:5]))
        n = collections.Counter(c for c, _ in found.values())
        print(f"{v}: {len(found)} names declared differently: mix {n['mix']}, width {n['width']}, asm {n['asm']}, "
              f"type {n['type']}; "
              f"{len(multi)} pointer variables declared in more than one file")
        if a.list:
            items = sorted(found.items()) if a.list != "multi" else []
            for name, (cls, ds) in items:
                if a.list in ("all", cls):
                    print(f"  {cls:5} {name}")
                    for kind, t, f, line, storage, _ in ds:
                        print(f"          {t:40} {f}:{line}" + (" (definition)" if storage == "def" else ""))
            if a.list in ("all", "multi"):
                for name, ds in sorted(multi.items()):
                    print(f"  multi {name}: " + "; ".join(f"{t} {f}:{line}" + ("*" if s == "def" else "")
                                                         for _, t, f, line, s, _ in ds))
        bad = {name: cls for name, (cls, _) in found.items() if cls in ("mix", "width", "asm")}
        for name, cls in bad.items():
            seen[name] = cls
        new = sorted(name for name, cls in bad.items() if allow.get(name) != cls)
        if new and not a.update:
            fails += 1
            print(f"{v}: FAIL, {len(new)} not in the allow-list: " + ", ".join(f"{bad[x]} {x}" for x in new))
        elif not a.update:
            print(f"{v}: the {len(bad)} pointer disagreements are the allow-list's")
    if a.update:
        head = [l for l in (open(a.allow).readlines() if os.path.exists(a.allow) else []) if l.startswith("#")]
        with open(a.allow, "w") as f:
            f.writelines(head)
            for name in sorted(seen, key=lambda x: (seen[x], x)):
                f.write(f"{seen[name]} {name}\n")
        print(f"wrote {os.path.relpath(a.allow)} ({len(seen)} names)")
    elif set(allow) - set(seen) and a.version == "all":
        print("the allow-list can shrink: " + ", ".join(sorted(set(allow) - set(seen))))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
