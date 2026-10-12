#!/bin/sh
# macos_build.sh [BUILD_DIR [cmake -D...]]: configure and build the port on
# macOS (arm64, or x86-64) with the system's clang (docs/PORT.md, "macOS").
# UNTESTED: no one has run it on a Mac yet.
#
#   brew install sdl2 libepoxy pkgconf cmake ninja python
#   port/tools/macos_build.sh build/port-macos -DPORT_VERSION=us.v10
#
# Needs the stage-2 build of the version (blastcorps/build, asm and assets,
# build/blastcorps.<version>.map), made here or copied from a Linux checkout
# (docs/PORT.md, "macOS").
set -e
B=${1:-build/port-macos}
[ $# -gt 0 ] && shift
cd "$(dirname "$0")/../.."
command -v brew > /dev/null || { echo "macos_build.sh: needs Homebrew (https://brew.sh)"; exit 1; }
missing=
for p in sdl2 libepoxy pkgconf cmake ninja; do
    brew list --versions "$p" > /dev/null 2>&1 || missing="$missing $p"
done
[ -z "$missing" ] || { echo "macos_build.sh: brew install$missing"; exit 1; }
cmake -S port -B "$B" -G Ninja -DCMAKE_C_COMPILER=cc "$@"
cmake --build "$B"
echo "built $B/blastcorps"
