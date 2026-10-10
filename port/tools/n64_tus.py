#!/usr/bin/env python3
"""The N64 side's translation units of a configured port build, with the
commands that compile them (for decl_check.py and ptrcast_scan.py).

    n64_tus.py BUILD [--version V]      lists them, one "kind file" a line

The commands are the build's own (ninja -t compdb): the game's C, port/src,
port/engine, port/libaudio and asm2c's data (gen/data_c), the files that
are compiled with port_game.h.  Without the launcher, the output and the
plugin, so that clang only parses.  Another version's are the same
commands with its VERSION_ define, which is how one build (us.v10's, say)
answers for all four: the sources are one tree, and what differs is
`#if`s.  asm2c's data is the build's version's alone (it is generated
from that version's asm), so it is left out for another.
"""
import argparse
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
VERSIONS = ("us.v10", "us.v11", "jp", "eu")


def version_define(v):
    return "VERSION_" + v.upper().replace(".", "_")


def cache(build):
    out = {}
    with open(os.path.join(build, "CMakeCache.txt")) as f:
        for line in f:
            m = re.match(r"([A-Za-z0-9_]+):[A-Z]+=(.*)", line.strip())
            if m:
                out[m.group(1)] = m.group(2)
    return out


def build_version(build):
    return cache(build).get("PORT_VERSION", "us.v11")


def is_lp64(build):
    return cache(build).get("PORT_LP64", "OFF").upper() in ("ON", "1", "TRUE", "YES")


def kind(path):
    p = os.path.relpath(path, ROOT)
    if "/gen/data_c/" in path:
        return "data"
    if p.startswith("blastcorps/"):
        return "game"
    if p.startswith("port/engine/"):
        return "engine"
    if p.startswith("port/src/"):
        return "port"
    if p.startswith("port/libaudio/"):
        return "libaudio"
    return "other"


def compdb(build):
    r = subprocess.run(["ninja", "-C", build, "-t", "compdb"], capture_output=True, text=True)
    if r.returncode == 0 and r.stdout.strip():
        return json.loads(r.stdout)
    path = os.path.join(build, "compile_commands.json")
    if os.path.exists(path):
        return json.load(open(path))
    raise SystemExit(f"{build}: no compile commands (a Ninja build, or CMAKE_EXPORT_COMPILE_COMMANDS)")


def clean(argv):
    """the compiler's own arguments: no launcher, output, dependency file
    or plugin"""
    i = next((k for k, a in enumerate(argv) if re.search(r"(^|/)clang(-\d+)?$", a)), None)
    if i is None:
        return None
    out, args = [argv[i]], argv[i + 1:]
    k = 0
    while k < len(args):
        a = args[k]
        if a in ("-o", "-MF", "-MT", "-MQ"):
            k += 2
            continue
        if a in ("-c", "-MD", "-MMD", "-emit-llvm") or a.startswith("-fpass-plugin="):
            k += 1
            continue
        out.append(a)
        k += 1
    return out


def tus(build, version=None):
    """[(kind, file, argv)] of the N64 side, for `version` (default the
    build's)"""
    build = os.path.abspath(build)
    own = build_version(build)
    version = version or own
    res, seen = [], set()
    for e in compdb(build):
        f = e["file"]
        if not os.path.isabs(f):
            f = os.path.normpath(os.path.join(e.get("directory", build), f))
        if not f.endswith(".c") or f in seen:
            continue
        argv = shlex.split(e["command"]) if "command" in e else list(e["arguments"])
        if not any(a.endswith("port_game.h") for a in argv):
            continue
        k = kind(f)
        if k == "data" and version != own:
            continue
        argv = clean(argv)
        if argv is None:
            continue
        argv = [("-D" + version_define(version)) if a == "-D" + version_define(own) else a for a in argv]
        seen.add(f)
        res.append((k, f, argv))
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build")
    ap.add_argument("--version", choices=VERSIONS)
    a = ap.parse_args()
    for k, f, _ in tus(a.build, a.version):
        print(k, os.path.relpath(f, ROOT))


if __name__ == "__main__":
    sys.exit(main())
