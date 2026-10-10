#!/usr/bin/env python3
"""The N64 side's pointer/integer conversions, classified, against an
allow-list (P6's inventory of them: docs/PORT.md, "The LP64 build").

    ptrcast_scan.py BUILD [--version V|all] [--allow FILE] [--list [CLASS]] [--update]

BUILD is a configured LP64 port build (-DPORT_LP64=ON): its commands for
the N64 side's files (n64_tus.py), run by clang with only
-Wpointer-to-int-cast, -Wint-to-pointer-cast and -Wint-conversion on.
With 8-byte pointers each is a place where a pointer goes through a 32-bit
integer, or one comes from it.  Another version's diagnostics are the same
commands with its VERSION_ define (all: the four), so each version's `#if`
branches are seen.  A diagnostic is counted once however many files
include the line.

Each is put in a class by where it is and the macros it comes through:

  P8-gbi      gbi.h's macros (the display list words) and K0_TO_PHYS and
              the like (segment addresses)
  P7-rom      a ROM address: (u32)D_00xxxxxx, ROM_RANGE, TABLE_ROM, *_ROM_START
  P7-rebase   ROM data rebased by an offset: LEVEL_PTR, MODEL_PTR,
              VMODEL_PTR, `p->offset + (u32)p` (DE70.c)
  P7-libaudio port/libaudio
  P10-host    port/src: the host interface takes N64 addresses (u32)
  P6-x3       the native-endian X3's `^ 3` on an address (uintptr_t)
  P6-game     the game's C (blastcorps/src, blastcorps/include)
  P6-engine   port/engine
  other       anything else

The allow-list (port/tools/ptrcast_allow.txt) has a line per class and
file: `class file count-us.v10 count-us.v11 count-jp count-eu` (- where a
version wasn't counted).  A count above it fails; one below it is a note
that the list can shrink (--update writes the counts scanned).  --list
prints the diagnostics (of a class).
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
ALLOW = os.path.join(ROOT, "port", "tools", "ptrcast_allow.txt")
CLASSES = ("P8-gbi", "P7-rom", "P7-rebase", "P7-libaudio", "P10-host", "P6-x3", "P6-game", "P6-engine", "other")
FLAGS = ["-fsyntax-only", "-Wno-everything", "-Wpointer-to-int-cast", "-Wint-to-pointer-cast",
         "-Wint-conversion", "-Wno-error", "-fno-caret-diagnostics", "-fmacro-backtrace-limit=0",
         "-fno-color-diagnostics"]
DIAG = re.compile(r"^(.*?):(\d+):(\d+): (warning|error): (.*?)(?: \[(-W[\w-]+)\])?$")
NOTE_MACRO = re.compile(r"^(.*?):(\d+):(\d+): note: expanded from macro '(\w+)'$")

K0_MACROS = {"K0_TO_PHYS", "K1_TO_PHYS", "PHYS_TO_K0", "PHYS_TO_K1", "OS_K0_TO_PHYSICAL",
             "OS_K1_TO_PHYSICAL", "OS_PHYSICAL_TO_K0", "OS_PHYSICAL_TO_K1", "STATIC_K0_TO_PHYS",
             "IO_READ", "IO_WRITE"}
ROM_MACROS = {"ROM_RANGE", "TABLE_ROM"}
REBASE_MACROS = {"LEVEL_PTR", "MODEL_PTR", "VMODEL_PTR"}
ROM_TEXT = re.compile(r"\bD_00[0-9A-Fa-f]{6}\b|_ROM_(START|END)\b")
X3_TEXT = re.compile(r"\^\s*3\b")
REBASE_TEXT = re.compile(r"->\s*\w+\s*\+\s*\(u32\)\s*\w+")

_lines = {}


def source_line(path, n):
    if path not in _lines:
        try:
            _lines[path] = open(path, errors="replace").read().split("\n")
        except OSError:
            _lines[path] = []
    ls = _lines[path]
    return ls[n - 1] if 0 < n <= len(ls) else ""


def scan_one(argv):
    r = subprocess.run(argv + FLAGS, capture_output=True, text=True)
    out = []
    cur = None
    for line in r.stderr.splitlines():
        m = DIAG.match(line)
        if m:
            if m.group(4) == "error":
                out.append(("error", line, []))
                cur = None
                continue
            cur = [os.path.normpath(m.group(1)), int(m.group(2)), int(m.group(3)), m.group(5), m.group(6) or "", []]
            out.append(("warning", cur, None))
            continue
        m = NOTE_MACRO.match(line)
        if m and cur is not None:
            cur[5].append((os.path.normpath(m.group(1)), int(m.group(2)), m.group(4)))
    return out, r.returncode


def classify(path, line, macros):
    rel = os.path.relpath(path, ROOT)
    names = [m[2] for m in macros]
    files = [os.path.basename(m[0]) for m in macros]
    text = source_line(path, line)
    if "gbi.h" in files or os.path.basename(path) == "gbi.h" or any(n in K0_MACROS for n in names):
        return "P8-gbi"
    if any(n in ROM_MACROS for n in names) or ROM_TEXT.search(text):
        return "P7-rom"
    if any(n in REBASE_MACROS for n in names) or REBASE_TEXT.search(text):
        return "P7-rebase"
    if rel.startswith("port/libaudio/"):
        return "P7-libaudio"
    if rel.startswith("port/src/"):
        return "P10-host"
    if "X3" in names or X3_TEXT.search(text):
        return "P6-x3"
    if rel.startswith("blastcorps/"):
        return "P6-game"
    if rel.startswith("port/engine/"):
        return "P6-engine"
    return "other"


def scan(build, version, jobs):
    """{(file, line, col, message): (class, flag)} for one version"""
    units = n64_tus.tus(build, version)
    res, errors = {}, []
    with cf.ThreadPoolExecutor(jobs) as ex:
        for out, rc in ex.map(lambda u: scan_one(u[2]), units):
            for kind, d, _ in out:
                if kind == "error":
                    errors.append(d)
                    continue
                path, line, col, msg, flag, macros = d
                key = (os.path.relpath(path, ROOT), line, col, msg)
                if key not in res:
                    res[key] = (classify(path, line, macros), flag)
    return res, errors


def read_allow(path):
    allow = {}
    if not os.path.exists(path):
        return allow
    for line in open(path):
        line = line.split("#", 1)[0].split()
        if len(line) != 2 + len(VERSIONS):
            continue
        allow[(line[0], line[1])] = {v: (None if c == "-" else int(c)) for v, c in zip(VERSIONS, line[2:])}
    return allow


def write_allow(path, allow):
    head = []
    if os.path.exists(path):
        for line in open(path):
            if not line.startswith("#"):
                break
            head.append(line)
    with open(path, "w") as f:
        f.writelines(head)
        for (cls, fn) in sorted(allow, key=lambda k: (CLASSES.index(k[0]) if k[0] in CLASSES else 99, k[1])):
            counts = allow[(cls, fn)]
            if not any(counts.get(v) for v in VERSIONS):
                continue
            f.write(f"{cls} {fn} " + " ".join("-" if counts.get(v) is None else str(counts[v]) for v in VERSIONS) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build")
    ap.add_argument("--version", default=None, help="a version, or all (default: the build's)")
    ap.add_argument("--allow", default=ALLOW)
    ap.add_argument("--list", nargs="?", const="all", default=None, metavar="CLASS")
    ap.add_argument("--update", action="store_true", help="write the counts scanned into the allow-list")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    a = ap.parse_args()
    if not n64_tus.is_lp64(a.build):
        raise SystemExit(f"{a.build}: not an LP64 build (-DPORT_LP64=ON): its pointers are 4 bytes, no cast is seen")
    versions = VERSIONS if a.version == "all" else [a.version or n64_tus.build_version(a.build)]
    allow = read_allow(a.allow)
    fails = 0
    totals, scanned = {}, {}
    for v in versions:
        res, errors = scan(a.build, v, a.jobs)
        if errors:
            print(f"{v}: clang failed:\n  " + "\n  ".join(errors[:10]))
            fails += 1
        per = collections.Counter((cls, k[0]) for k, (cls, _) in res.items())
        totals[v] = collections.Counter(cls for k, (cls, _) in res.items())
        lines = collections.Counter(cls for cls, _ in {(c, (k[0], k[1])) for k, (c, _) in res.items()})
        print(f"{v}: {len(res)} diagnostics: " + ", ".join(f"{c} {totals[v][c]} ({lines[c]} lines)"
                                                         for c in CLASSES if totals[v][c]))
        if a.list:
            for k in sorted(res):
                cls, flag = res[k]
                if a.list in ("all", cls):
                    print(f"  {cls:11} {k[0]}:{k[1]}:{k[2]}: {k[3]} [{flag}]")
        keys = set(per) | {k for k in allow if allow[k].get(v) is not None}
        over, under = [], []
        for key in sorted(keys):
            got, ok = per.get(key, 0), allow.get(key, {}).get(v)
            if ok is None:
                ok = 0
            if got > ok:
                over.append(f"{key[0]} {key[1]}: {got} > {ok}")
            elif got < ok:
                under.append(f"{key[0]} {key[1]}: {got} < {ok}")
        scanned[v] = per
        if a.update:
            pass
        elif over:
            fails += 1
            print(f"{v}: FAIL, {len(over)} over the allow-list (--list shows them):\n  " + "\n  ".join(over))
        else:
            print(f"{v}: within the allow-list" + (f"; it can shrink ({len(under)}): " + "; ".join(under)
                                                   if under else ""))
    if a.update:
        for key in set(allow) | {k for per in scanned.values() for k in per}:
            for v, per in scanned.items():
                allow.setdefault(key, {})[v] = per.get(key, 0)
        write_allow(a.allow, allow)
        print(f"wrote {os.path.relpath(a.allow)}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
