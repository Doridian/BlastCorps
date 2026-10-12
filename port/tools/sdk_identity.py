#!/usr/bin/env python3
"""The port's own SDK headers against 2.0I: every object the same.

    port/tools/sdk_identity.py [--kinds plain,m32] [--version V] [-j N]

For each kind (plain: the host's pointers; m32: i386's, 4 bytes), configures
two builds of the port's N64 side, build/sdkid-<kind>-own (port/include/sdk)
and build/sdkid-<kind>-2.0I (-DPORT_SDK_2_0I=ON), compiles the game's C and
port/src (the game_c and port_n64 objects) and compares every object with
its twin, its debug information left out: the sections, the symbols and
the relocations must be byte for byte the same.

Needs a stage 2 of the version in blastcorps/, like any port build.
"""
import argparse
import concurrent.futures as cf
import filecmp
import os
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KINDS = {
    "plain": [],
    "m32": ["-DCMAKE_C_FLAGS=-m32"],
}
CMAKE = ["-G", "Ninja", "-DCMAKE_C_COMPILER=clang"]


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode:
        sys.exit(f"sdk_identity: failed: {' '.join(cmd)}\n{r.stdout[-3000:]}{r.stderr[-3000:]}")
    return r.stdout


def build(d, opts, version, jobs):
    if not os.path.exists(os.path.join(d, "CMakeCache.txt")):
        run(["cmake", "-S", os.path.join(ROOT, "port"), "-B", d, f"-DPORT_VERSION={version}"] + CMAKE + opts)
    else:
        run(["cmake", d] + opts)
    run(["ninja", "-C", d, f"-j{jobs}", "game_c", "port_n64"])


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
    """the object without its debug information"""
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
    ap.add_argument("--kinds", default="plain,m32")
    ap.add_argument("--version", default=None)
    ap.add_argument("-j", "--jobs", type=int, default=8)
    a = ap.parse_args()
    version = a.version or open(os.path.join(ROOT, "blastcorps", ".version")).read().strip()
    bad = 0
    for v in a.kinds.split(","):
        own = os.path.join(ROOT, "build", f"sdkid-{v}-own")
        sdk = os.path.join(ROOT, "build", f"sdkid-{v}-2.0I")
        with cf.ThreadPoolExecutor(2) as ex:
            list(ex.map(lambda x: build(x[0], KINDS[v] + x[1], version, a.jobs),
                        [(own, ["-DPORT_SDK_2_0I=OFF"]), (sdk, ["-DPORT_SDK_2_0I=ON"])]))
        n, missing, differ = compare(own, sdk)
        ok = not missing and not differ
        print(f"{'PASS' if ok else 'FAIL'}  {v}: {n} objects, {len(differ)} differ, {len(missing)} in one build only",
              flush=True)
        for k in differ[:20] + missing[:20]:
            print("      " + k)
        bad |= not ok
    print("== sdk_identity: " + ("FAILED" if bad else "every object the same"))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
