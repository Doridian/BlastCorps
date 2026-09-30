#!/bin/sh
# macos_build.sh [BUILD_DIR [cmake -D...]]: configure and build the port on
# macOS (arm64) with Homebrew's LLVM: the LP64 movable build (docs/PORT.md,
# "macOS").  UNTESTED: no one has run it on a Mac yet.
#
#   brew install llvm sdl2 libepoxy pkgconf cmake ninja python
#   port/tools/macos_build.sh build/port-macos -DPORT_VERSION=us.v10
#
# Needs the stage-2 build of the version and its translated engine
# (blastcorps/build, asm and assets, build/blastcorps.<version>.map), made
# here or copied from a Linux checkout (docs/PORT.md, "macOS").  Apple's
# clang can't load BEPass, so the compilers are Homebrew LLVM's; opt, llc
# and llvm-link come from there too.
set -e
B=${1:-build/port-macos}
[ $# -gt 0 ] && shift
cd "$(dirname "$0")/../.."
command -v brew > /dev/null || { echo "macos_build.sh: needs Homebrew (https://brew.sh)"; exit 1; }
missing=
for p in llvm sdl2 libepoxy pkgconf cmake ninja; do
    brew list --versions "$p" > /dev/null 2>&1 || missing="$missing $p"
done
[ -z "$missing" ] || { echo "macos_build.sh: brew install$missing"; exit 1; }
LLVM=$(brew --prefix llvm)
cmake -S port -B "$B" -G Ninja \
    -DCMAKE_C_COMPILER="$LLVM/bin/clang" -DCMAKE_CXX_COMPILER="$LLVM/bin/clang++" \
    -DCMAKE_ASM_COMPILER="$LLVM/bin/clang" \
    -DPORT_LP64=ON -DPORT_MOVABLE=ON "$@"
cmake --build "$B"
echo "built $B/blastcorps"
