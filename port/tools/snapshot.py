#!/usr/bin/env python3
"""The ROM-free build's snapshot (port/snapshot/<version>, committed): the
generated sources the port's build makes from the N64 link, and the few
addresses it reads from the link's ELFs (layout.cmake).  With the snapshot,
the port builds from the repository alone: no ROM, no asm/, no assets/, no
build/ of the decompilation (docs/PORT.md, "The snapshot").

    port/tools/snapshot.py [us.v11] [--gen DIR]

Run it with the ROM, after the decompilation builds (README, "Build it"):
it copies from build/port-<version>/gen (or build/web-<version>/gen; --gen
names another; the native build's, not the WebAssembly one's, since the
latter's nm reads read-only data as writable) and reads blastcorps/build's ELFs.  Re-run it whenever the
decompilation's output changes.  Nothing it copies holds a ROM asset: the
assets are zeros in data_c (asm2c.py's ROM_ASSETS) and their initializers
are in src/ under ASSET_INIT, and the fill table holds addresses and sizes.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BLAST = ROOT / "blastcorps"
FILES = ["rom_addrs.h", "rom_fill.c", "rom_fill.tsv", "romtab.h", "n64_funcs.txt", "data_order.txt"]
DIRS = ["data_c", "fe"]


def die(msg):
    sys.exit(f"snapshot.py: {msg}")


def layout(version):
    """the addresses CMake reads from the ELFs (port/CMakeLists.txt, "Addresses
    that differ between versions")"""
    nm = os.environ.get("PORT_N64_NM", "mips-linux-gnu-nm")
    readelf = os.environ.get("PORT_N64_READELF", "mips-linux-gnu-readelf")
    hd = BLAST / "build" / f"hd_code.{version}.elf"
    fe = BLAST / "build" / f"hd_front_end.{version}.elf"
    for p in (hd, fe):
        if not p.exists():
            die(f"{p} is missing: build blastcorps ({version}) first")
    out = subprocess.run([nm, str(hd)], capture_output=True, text=True, check=True).stdout
    m = re.search(r"([0-9a-f]+) B D_803FF600\n", out)
    if not m:
        die(f"no D_803FF600 in {hd.name}")
    vals = {"PORT_STATIC_START": "0x" + m.group(1)}
    sections = subprocess.run([readelf, "-SW", str(fe)], capture_output=True, text=True, check=True).stdout
    for sec, name in (("data", "PORT_FE_DATA"), ("bss", "PORT_FE_BSS")):
        m = re.search(rf"\.hd_front_end_{sec} +[A-Z]+ +([0-9a-f]+)", sections)
        if not m:
            die(f"no .hd_front_end_{sec} in {fe.name}")
        vals[name] = "0x" + m.group(1)
    m = re.search(r"\.hd_front_end +[A-Z]+ +([0-9a-f]+)", sections)
    if not m:
        die(f"no .hd_front_end in {fe.name}")
    vals["PORT_FE_TEXT"] = "0x" + m.group(1)
    return "".join(f"set({k} {v})\n" for k, v in vals.items())


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("version", nargs="?", default="us.v11")
    ap.add_argument("--gen", type=Path)
    args = ap.parse_args()
    v = args.version
    gen = args.gen.resolve() if args.gen else None
    if gen is None:
        # the native build first: its objects put a const table in .rodata
        # ('R', read-only), where the WebAssembly ones have no such section
        # and nm reports it writable (gen_fevars.py would then restore into
        # read-only memory on the native build: overlay.c)
        for cand in (ROOT / f"build/port-{v}/gen", ROOT / f"build/web-{v}/gen"):
            if cand.is_dir():
                gen = cand
                break
    if gen is None or not gen.is_dir():
        die(f"no generated sources for {v}: build it first (or --gen DIR)")
    dst = ROOT / "port/snapshot" / v
    if dst.exists():
        shutil.rmtree(dst)
    dst.mkdir(parents=True)
    for f in FILES:
        if not (gen / f).is_file():
            die(f"{gen / f} is missing")
        shutil.copy2(gen / f, dst / f)
    for d in DIRS:
        if not (gen / d).is_dir():
            die(f"{gen / d} is missing")
        shutil.copytree(gen / d, dst / d)
    (dst / "layout.cmake").write_text(
        "# the N64 link's addresses CMake reads from the ELFs (port/tools/snapshot.py)\n"
        + layout(v))
    n = sum(1 for _ in dst.rglob("*") if _.is_file())
    print(f"snapshot.py: {n} files into {dst.relative_to(ROOT)} from {gen.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
