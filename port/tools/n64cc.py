#!/usr/bin/env python3
"""Compile the N64 side with another compiler than CMake's.

CMake's compiler launcher for the WebAssembly build (docs/PORT.md,
"WebAssembly"):

  n64cc.py CC -- CMAKE_CC ARGS...

runs CC ARGS instead of CMAKE_CC ARGS.  There CMake's compiler is emcc,
which builds the host side; the N64 side is compiled by the clang that can
load BEPass (emsdk's clang ships no LLVM headers to build a plugin for),
for wasm32-unknown-emscripten, to bitcode for the arena link.
"""

import os
import sys


def main():
    argv = sys.argv[1:]
    if len(argv) < 3 or argv[1] != "--":
        sys.exit("usage: n64cc.py CC -- CMAKE_CC ARGS...")
    cc, args = argv[0], argv[3:]
    os.execvp(cc, [cc] + args)


if __name__ == "__main__":
    main()
