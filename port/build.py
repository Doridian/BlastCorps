#!/usr/bin/env python3
"""Build the PC port from your own ROM, in one go.

    port/build.py ROM              the port for this machine
    port/build.py --wasm ROM       the browser page (WebAssembly)
    port/build.py --wasm --serve ROM   ... and serve it on localhost

It names the ROM by its sha1 (us.v10, us.v11 or jp; .z64, .v64 and .n64 byte
orders are all fine), copies it to baserom.<version>.z64, sets up the venv and
the submodules if they aren't there yet, builds the decompilation and the
translated engine for that version (README, "The PC port"), and then the port
into build/port-<version> (or build/web-<version>, build/node-<version> with
--node).  Run it again after a `git pull` and it only redoes what changed.

The decompilation's output holds one version at a time: building another
version clears the old one's (not the port builds already made).

Options:
    --wasm           the WebAssembly page (needs emsdk: --emsdk DIR, $EMSDK,
                     emcmake on PATH, or --install-emsdk to fetch it into
                     .emsdk/)
    --node           the WebAssembly build for node (headless) instead
    --serve [PORT]   after --wasm, serve the page (default port 8000)
    --variant V      native only: 64 (default on Linux), 32, lp64, movable
                     (LP64, no fixed addresses; macOS's), native (native-endian
                     64-bit)
    -j N             parallel jobs
    --cmake ARG      pass a -D... to CMake (repeatable)

Needs: the mips-linux-gnu- binutils, Python 3, clang and LLVM with its
headers (llvm-config, opt), CMake, Ninja, SDL2 and libepoxy (README,
"Requirements").
"""
import argparse
import hashlib
import http.server
import os
import platform
import shutil
import socketserver
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VERSIONS = {
    "185a6ef7ba1adb243278062c81a7d4e119bda58c": "us.v10",
    "483f7161aea39de8b45c9fbc70a2c3883c4dea8c": "us.v11",
    "b147fdbeb661c89107c440b00dc4810508f58636": "jp",
    "460212600f8b9f0da95219c4c7330f2e626d9a7e": "eu",
}
PORTED = ("us.v10", "us.v11", "jp")
VARIANTS = {
    "64": ["-DPORT_64BIT=ON"],
    "32": [],
    "lp64": ["-DPORT_LP64=ON"],
    "movable": ["-DPORT_LP64=ON", "-DPORT_MOVABLE=ON"],
    "native": ["-DPORT_64BIT=ON", "-DPORT_NATIVE_ENDIAN=ON"],
}


def say(msg):
    print(f"\n== {msg}", flush=True)


def die(msg):
    sys.exit(f"build.py: {msg}")


def run(cmd, env=None, cwd=ROOT, shell=False):
    print("$ " + (cmd if shell else " ".join(str(c) for c in cmd)), flush=True)
    r = subprocess.run(cmd, cwd=cwd, env=env, shell=shell,
                       executable="/bin/bash" if shell else None)
    if r.returncode:
        die(f"that failed (exit {r.returncode})")


def rom_z64(path):
    """The ROM in .z64 (big-endian) byte order."""
    data = bytearray(Path(path).read_bytes())
    magic = bytes(data[:4])
    if magic == b"\x80\x37\x12\x40":
        return bytes(data)
    if magic == b"\x37\x80\x40\x12":                    # .v64: 16-bit swapped
        data[0::2], data[1::2] = data[1::2], data[0::2]
        return bytes(data)
    if magic == b"\x40\x12\x37\x80":                    # .n64: 32-bit swapped
        return b"".join(bytes(reversed(data[i:i + 4])) for i in range(0, len(data), 4))
    die(f"{path} doesn't look like an N64 ROM")


def check_tools(wasm):
    missing = []
    need = ["mips-linux-gnu-ld", "mips-linux-gnu-as", "clang", "clang++", "llvm-config", "opt",
            "cmake", "ninja", "git", "make"]
    if not wasm:
        need.append("pkg-config")
    for t in need:
        if not shutil.which(t):
            missing.append(t)
    if not wasm and shutil.which("pkg-config"):
        for lib in ("sdl2",):
            if subprocess.run(["pkg-config", "--exists", lib]).returncode:
                missing.append(f"{lib} (development files)")
    if missing:
        die("missing: " + ", ".join(missing) + "\n(README, \"Requirements\", lists what to install)")


def setup_python():
    env = dict(os.environ)
    venv = ROOT / ".env"
    if not (ROOT / "tools/splat/requirements.txt").exists():
        say("submodules")
        run(["git", "submodule", "update", "--init", "--recursive"])
    if not (venv / "bin/python").exists():
        say("Python venv (.env)")
        run([sys.executable, "-m", "venv", str(venv)])
        run([str(venv / "bin/pip"), "install", "-q", "-r", "tools/splat/requirements.txt"])
    env["PATH"] = f"{venv / 'bin'}:{env['PATH']}"
    env["VIRTUAL_ENV"] = str(venv)
    return env


def clear_version(old):
    """What `make clean` removes, but not build/'s port builds."""
    say(f"clearing the decompilation's {old} output")
    for p in ("asm", "assets", "build/asm", "build/assets", "build/asset_shift.stamp", "build/.extracted", ".version"):
        q = ROOT / p
        if q.is_dir():
            shutil.rmtree(q)
        elif q.exists():
            q.unlink()
    for q in list((ROOT / "build").glob(f"*.{old}.*")) + list(ROOT.glob("*auto*.txt")) + list(ROOT.glob("*.ld")):
        if q.is_file():                                 # (not port builds named after it)
            q.unlink()
    for m in ("init", "hd_code", "hd_front_end"):
        q = ROOT / f"blastcorps/{m}.{old}.bin"
        if q.exists():
            q.unlink()
    run(["make", "-s", "-C", "blastcorps", "clean"])


def build_decomp(v, env, jobs):
    marker = ROOT / "build/.extracted"                  # written once both extractions finished
    done = marker.read_text().strip() if marker.exists() else None
    old = next((f.read_text().strip() for f in (ROOT / ".version", ROOT / "blastcorps/.version") if f.exists()), None)
    if old and (old != v or done != v):                 # another version, or an extraction cut short
        clear_version(old)
    fresh = done != v
    V = f"VERSION={v}"
    say(f"the decompilation ({v})" + (", extracting first (a minute or two)" if fresh else ""))
    if fresh:
        run(["make", V, "extract"], env)
    run(["make", V], env)
    if fresh:
        run(["make", V, "decompress"], env)
        run(["make", V, "-C", "blastcorps", "extract"], env)
        marker.write_text(v + "\n")
    run(["make", f"-j{jobs}", V, "-C", "blastcorps"], env)
    run(["make", V, "-C", "blastcorps", "compress"], env)
    run(["make", V], env)
    say("translating the handwritten engine")
    run(["make", f"-j{jobs}", "-C", "tools/recomp", V, f"PYTHON={ROOT / '.env/bin/python'}"], env)


def find_emsdk(args):
    if args.emsdk:
        return Path(args.emsdk)
    if os.environ.get("EMSDK"):
        return Path(os.environ["EMSDK"])
    local = ROOT / ".emsdk"
    if (local / "emsdk_env.sh").exists():
        return local
    if shutil.which("emcmake"):
        return None                                     # already in this shell
    if not args.install_emsdk:
        die("no emsdk found: pass --emsdk DIR, set $EMSDK, or --install-emsdk to fetch it into .emsdk/")
    say("emsdk (into .emsdk/)")
    run(["git", "clone", "--depth", "1", "https://github.com/emscripten-core/emsdk.git", str(local)])
    run(["./emsdk", "install", "latest"], cwd=local)
    run(["./emsdk", "activate", "latest"], cwd=local)
    return local


def serve(d, port):
    say(f"serving {d} on http://localhost:{port}/blastcorps.html (Ctrl-C stops it)")
    handler = lambda *a: http.server.SimpleHTTPRequestHandler(*a, directory=str(d))
    with socketserver.TCPServer(("127.0.0.1", port), handler) as s:
        try:
            s.serve_forever()
        except KeyboardInterrupt:
            pass


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("rom")
    ap.add_argument("--wasm", action="store_true")
    ap.add_argument("--node", action="store_true")
    ap.add_argument("--serve", nargs="?", const=8000, type=int)
    ap.add_argument("--emsdk")
    ap.add_argument("--install-emsdk", action="store_true")
    ap.add_argument("--variant", choices=sorted(VARIANTS))
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--cmake", action="append", default=[])
    args = ap.parse_args()
    wasm = args.wasm or args.node
    if args.serve and not args.wasm:
        die("--serve goes with --wasm")

    say("the ROM")
    z64 = rom_z64(args.rom)
    v = VERSIONS.get(hashlib.sha1(z64).hexdigest())
    if not v:
        die(f"{args.rom} isn't a ROM this project knows (README lists the four sha1s)")
    if v not in PORTED:
        die(f"{args.rom} is {v}, which the port doesn't support yet (us.v10, us.v11 and jp)")
    dst = ROOT / f"baserom.{v}.z64"
    if not dst.exists() or dst.read_bytes() != z64:
        dst.write_bytes(z64)
    print(f"{args.rom}: {v} -> {dst.name}")

    darwin = platform.system() == "Darwin"
    if darwin and not wasm:
        die("on macOS, build the decompilation on Linux and use port/tools/macos_build.sh (README, \"macOS\")")
    check_tools(wasm)
    env = setup_python()
    build_decomp(v, env, args.jobs)

    if not wasm:
        variant = args.variant or "64"
        out = ROOT / f"build/port-{v}" if variant == "64" else ROOT / f"build/port-{v}-{variant}"
        say(f"the port ({variant}) into {out.relative_to(ROOT)}")
        run(["cmake", "-S", "port", "-B", str(out), "-G", "Ninja", "-DCMAKE_C_COMPILER=clang",
             "-DCMAKE_CXX_COMPILER=clang++", "-DCMAKE_ASM_COMPILER=clang", f"-DPORT_VERSION={v}"]
            + VARIANTS[variant] + args.cmake, env)
        run(["cmake", "--build", str(out), "-j", str(args.jobs)], env)
        rel = out.relative_to(ROOT)
        say(f"done: run it with\n   {rel}/blastcorps {dst.name}\n(from the repo's root; README, \"Run it\", has the keys and options)")
        return

    target = "node" if args.node else "web"
    out = ROOT / f"build/{target}-{v}"
    emsdk = find_emsdk(args)
    source = f"source {emsdk / 'emsdk_env.sh'} > /dev/null && " if emsdk else ""
    say(f"the WebAssembly build ({target}) into {out.relative_to(ROOT)}")
    cmake = " ".join(["emcmake", "cmake", "-S", "port", "-B", str(out), "-G", "Ninja",
                      f"-DPORT_VERSION={v}", f"-DPORT_WASM_TARGET={target}"] + args.cmake)
    run(source + cmake, env, shell=True)
    run(source + f"cmake --build {out} -j {args.jobs}", env, shell=True)
    rel = out.relative_to(ROOT)
    if args.node:
        say(f"done: run it with\n   node {rel}/blastcorps.js --headless --frames 600 --screenshot shot {dst.name}")
        return
    if args.serve:
        serve(out, args.serve)
    else:
        say(f"done: serve {rel} over HTTP, e.g.\n   python3 -m http.server -d {rel} 8000\n"
            f"and open http://localhost:8000/blastcorps.html (the page asks for your ROM once);\n"
            f"or run this again with --serve")


if __name__ == "__main__":
    main()
