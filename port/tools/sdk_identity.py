#!/usr/bin/env python3
"""The port's own SDK headers against 2.0I: every object the same.

    port/tools/sdk_identity.py [--variants 32,64,n64,lp64,mn32] [--version V] [-j N]

For each variant, configures two builds of the port's N64 side,
build/sdkid-<variant>-own (port/include/sdk) and build/sdkid-<variant>-2.0I
(-DPORT_SDK_2_0I=ON), compiles the game's C and port/src (the game_c and
port_n64 objects) and compares every object with its twin, its debug
information left out: the sections, the symbols and the relocations must
be byte for byte the same (a movable build's objects are LLVM bitcode,
compared as their IR without the debug metadata).  Then layout_cmp.py
compares the struct layouts the two programs' DWARF has, for the builds it
links (--link).

Needs a stage 2 of the version in blastcorps/ and its tools/recomp output,
like any port build.
"""
import argparse
import concurrent.futures as cf
import filecmp
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
VARIANTS = {
    "32": [],
    "64": ["-DPORT_64BIT=ON"],
    "n64": ["-DPORT_64BIT=ON", "-DPORT_NATIVE_ENDIAN=ON"],
    "lp64": ["-DPORT_LP64=ON"],
    "mn32": ["-DPORT_NATIVE_ENDIAN=ON", "-DPORT_MOVABLE=ON"],
}
CMAKE = ["-G", "Ninja", "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++", "-DCMAKE_ASM_COMPILER=clang"]


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode:
        sys.exit(f"sdk_identity: failed: {' '.join(cmd)}\n{r.stdout[-3000:]}{r.stderr[-3000:]}")
    return r.stdout


def build(d, opts, version, jobs, link):
    if not os.path.exists(os.path.join(d, "CMakeCache.txt")):
        run(["cmake", "-S", os.path.join(ROOT, "port"), "-B", d, f"-DPORT_VERSION={version}"] + CMAKE + opts)
    else:
        run(["cmake", d] + opts)
    run(["ninja", "-C", d, f"-j{jobs}"] + ([] if link else ["game_c", "port_n64"]))


def objects(d):
    out = {}
    for sub in ("game_c.dir", "port_n64.dir"):
        base = os.path.join(d, "CMakeFiles", sub)
        for dp, _, fs in os.walk(base):
            for f in fs:
                if f.endswith(".o"):
                    p = os.path.join(dp, f)
                    out[os.path.relpath(p, d)] = p
    return out


def normalized(path, tmp):
    """the object without its debug information: an ELF stripped of it, or
    bitcode's IR without the metadata"""
    with open(path, "rb") as f:
        magic = f.read(4)
    if magic == b"BC\xc0\xde":
        stripped = os.path.join(tmp, "o.bc")
        run(["opt", "--strip-debug", "-o", stripped, path])
        ir = run(["llvm-dis", "-o", "-", stripped])
        ir = re.sub(r"(, )?!dbg ![0-9]+", "", ir)
        ir = "\n".join(l for l in ir.splitlines()
                       if not l.startswith("!") and not l.startswith("source_filename")
                       and "llvm.dbg." not in l and not l.startswith("; ModuleID")
                       and not l.lstrip().startswith("#dbg_"))
        return ir.encode()
    out = os.path.join(tmp, "o")
    run(["llvm-objcopy", "--strip-debug", "--remove-section=.comment", path, out])
    with open(out, "rb") as f:
        return f.read()


def compare(a, b):
    oa, ob = objects(a), objects(b)
    missing = sorted(set(oa) ^ set(ob))
    differ = []
    with tempfile.TemporaryDirectory() as tmp:
        for k in sorted(set(oa) & set(ob)):
            if filecmp.cmp(oa[k], ob[k], shallow=False):
                continue
            if normalized(oa[k], tmp) != normalized(ob[k], tmp):
                differ.append(k)
    return len(set(oa) & set(ob)), missing, differ


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--variants", default="32,64,n64,lp64,mn32")
    ap.add_argument("--version", default=None)
    ap.add_argument("--link", action="store_true", help="link both and compare their layouts (layout_cmp.py)")
    ap.add_argument("-j", "--jobs", type=int, default=8)
    a = ap.parse_args()
    version = a.version or open(os.path.join(ROOT, "blastcorps", ".version")).read().strip()
    bad = 0
    for v in a.variants.split(","):
        own = os.path.join(ROOT, "build", f"sdkid-{v}-own")
        sdk = os.path.join(ROOT, "build", f"sdkid-{v}-2.0I")
        with cf.ThreadPoolExecutor(2) as ex:
            list(ex.map(lambda x: build(x[0], VARIANTS[v] + x[1], version, a.jobs, a.link),
                        [(own, ["-DPORT_SDK_2_0I=OFF"]), (sdk, ["-DPORT_SDK_2_0I=ON"])]))
        n, missing, differ = compare(own, sdk)
        ok = not missing and not differ
        print(f"{'PASS' if ok else 'FAIL'}  {v}: {n} objects, {len(differ)} differ, {len(missing)} in one build only",
              flush=True)
        for k in differ[:20] + missing[:20]:
            print("      " + k)
        if a.link:
            sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
            import layout_cmp
            la = layout_cmp.layouts(os.path.join(own, "blastcorps"), False)
            lb = layout_cmp.layouts(os.path.join(sdk, "blastcorps"), False)
            moved = []
            for name in sorted(set(la) & set(lb)):
                (sa, ma), (sb, mb) = la[name], lb[name]
                # (a union of 2.0I's may have more alternatives than the
                # port's: only the members both have count)
                oa, ob = dict(ma), dict(mb)
                if sa != sb or any(oa[m] != ob[m] for m in set(oa) & set(ob)):
                    moved.append(name)
            print(f"      layout_cmp: {len(set(la) & set(lb))} types in both, {len(moved)} laid out differently"
                  + (": " + ", ".join(moved[:10]) if moved else ""))
            ok = ok and not moved
        bad |= not ok
    print("== sdk_identity: " + ("FAILED" if bad else "every object the same"))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
