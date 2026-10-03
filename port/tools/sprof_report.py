#!/usr/bin/env python3
"""sprof.c's samples as self time by function.

  sprof_report.py EXE SAMPLES... [--top N] [--thread main|all] [--area] [--callers]

The executable's own functions by its symbols (nm), shared libraries by
name.  --area adds the share of port/engine's functions, the game's C,
the renderer (port/host/gfx*.c) and the rest of the host, by where each
function is defined (its source, from nm -l on the first run: cached next
to the samples).  The main thread is the one with the most samples (the
game and the renderer run there, the audio callback in SDL's thread).
"""
import bisect
import collections
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def symbols(exe):
    out = subprocess.run(["nm", "-n", "--defined-only", "-l", exe], capture_output=True, text=True).stdout
    addrs, names, files = [], [], []
    for l in out.splitlines():
        m = re.match(r"([0-9a-f]+) [tTwW] (\S+)(?:\s+(\S+))?", l)
        if m:
            addrs.append(int(m.group(1), 16))
            names.append(m.group(2))
            files.append(m.group(3) or "")
    return addrs, names, files


def dynsyms(lib):
    """a library's exported functions by file offset (its text's address is
    its offset in the usual shared objects)"""
    out = subprocess.run(["nm", "-D", "-n", "--defined-only", lib], capture_output=True, text=True).stdout
    la, ln = [], []
    for l in out.splitlines():
        m = re.match(r"([0-9a-f]+) [tTiW] (\S+)", l)
        if m:
            la.append(int(m.group(1), 16))
            ln.append(m.group(2))
    return la, ln


def defined_names():
    """{function name: area} from where the sources define them (for the
    WebAssembly profile, which has names but no lines)"""
    import glob
    areas = {}
    groups = [("engine", "port/engine/*.c"), ("renderer", "port/host/gfx*.c"), ("host", "port/host/*.c"),
              ("game C", "blastcorps/src/**/*.c"), ("game C", "port/src/*.c")]
    for a, pat in groups:
        for path in glob.glob(os.path.join(ROOT, pat), recursive=True):
            if os.path.basename(path).startswith(("fuzz", "jp_")):
                continue
            for m in re.finditer(r"^[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;{]*\)\s*\{", open(path, errors="replace").read(),
                                 re.M):
                areas.setdefault(m.group(1), a)
    return areas


def area(name, src):
    if "/port/engine/" in src and not os.path.basename(src.split(":")[0]).startswith(("fuzz", "jp_")) or name.startswith("recomp_"):
        return "engine"
    if "/blastcorps/src/" in src or "/port/src/" in src:
        return "game C"
    if re.search(r"/port/host/gfx", src):
        return "renderer"
    if "/port/host/" in src:
        return "host"
    return "other"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    top = 40
    if "--top" in sys.argv:
        top = int(sys.argv[sys.argv.index("--top") + 1])
        args.remove(str(top))
    if "--thread" in sys.argv:
        args.remove(sys.argv[sys.argv.index("--thread") + 1])
    exe, samples = args[0], args[1:]
    addrs, names, files = symbols(exe)
    counts = collections.Counter()
    areas = collections.Counter()
    total = 0
    libsyms = {}
    for path in samples:
        maps, pcs = [], []
        for l in open(path):
            if l.startswith("M "):
                f = l[2:].split()
                lo, hi = (int(x, 16) for x in f[0].split("-"))
                maps.append((lo, hi, f[5] if len(f) > 5 else "[anon]", int(f[2], 16)))
            else:
                f = l.split()
                pcs.append((int(f[0], 16), f[1], int(f[2], 16) if len(f) > 2 else 0))
        bytid = collections.Counter(t for _, t, _ in pcs)
        main_tid = bytid.most_common(1)[0][0] if bytid else None
        exe_real = os.path.realpath(exe)
        for pc, tid, caller in pcs:
            if "--thread" in sys.argv and sys.argv[sys.argv.index("--thread") + 1] == "main" and tid != main_tid:
                continue
            total += 1
            mp = next((m for m in maps if m[0] <= pc < m[1]), None)
            lib = mp[2] if mp else "?"
            if os.path.realpath(lib) == exe_real or lib.endswith(os.path.basename(exe)):
                i = bisect.bisect_right(addrs, pc) - 1
                name = names[i] if i >= 0 else "?"
                counts[name] += 1
                areas[area(name, files[i] if i >= 0 else "")] += 1
            else:
                name = "[%s]" % os.path.basename(lib)
                if mp and lib.startswith("/"):
                    if lib not in libsyms:
                        libsyms[lib] = dynsyms(lib)
                    la, ln = libsyms[lib]
                    i = bisect.bisect_right(la, pc - mp[0] + mp[3]) - 1
                    if i >= 0:
                        name = "%s [%s]" % (ln[i], os.path.basename(lib))
                if "--callers" in sys.argv and caller:
                    i = bisect.bisect_right(addrs, caller) - 1
                    name += " <- " + (names[i] if i >= 0 else "?")
                counts[name] += 1
                areas["libraries"] += 1
    print("%d samples" % total)
    if "--area" in sys.argv:
        for a, c in areas.most_common():
            print("  %-10s %6.2f%%" % (a, 100.0 * c / total))
    for name, c in counts.most_common(top):
        print("%7.2f%%  %s" % (100.0 * c / total, name))


if __name__ == "__main__":
    main()
