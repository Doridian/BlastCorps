#!/usr/bin/env bash
# Play the TASVideos Blast Corps movie (submission #8170, WarHippy's "all
# platinum medals", BizHawk 2.7) from power-on and check that it beats the
# game:
#
#   port/tools/tas.sh [VIS]
#
# It syncs only on the core it was made with, so this builds BizHawk 2.7's
# mupen64plus core (2.0 with BizHawk's changes) and rsp-hle for Linux, with
# tas_bizhawk.patch (MSVC's round/trunc, which the movie depends on, and the
# saveram exports) and tas_bizhawk_compat.h (Windows stubs).  Then m64p_tas
# plays the movie (or its first VIS VIs) into build/tas/run/: polls.csv has
# the pad and the VI at every controller read (what the port's --replay
# plays), eeprom.bin the save, which
# tas_check.py reads the medals from.  port/tools/tas_port.sh then replays it
# on the port.  See docs/PORT.md, "The TAS".
#
# Needs baserom.us.v10.z64 (the movie's ROM), git, curl, SDL 1.2 (or
# sdl12-compat), zlib, libpng and minizip.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$ROOT/build/tas
ROM=$ROOT/baserom.us.v10.z64
MOVIE_URL='https://tasvideos.org/8170S?handler=Download'
MOVIE_SHA1=4125faa289592cee15e332e764d885265f890dd5
BIZHAWK_COMMIT=dbaf2595625f79093eeec37d2d4a7a9a4d37f370   # tag 2.7
BH=$OUT/bizhawk
M64P=$BH/libmupen64plus
mkdir -p "$OUT"
echo "185a6ef7ba1adb243278062c81a7d4e119bda58c  $ROM" | sha1sum -c --quiet

# the movie
LOG="$OUT/movie/Input Log.txt"
if [ ! -f "$LOG" ]; then
    [ -f "$OUT/8170S.zip" ] || curl -fsSL -o "$OUT/8170S.zip" "$MOVIE_URL"
    echo "$MOVIE_SHA1  $OUT/8170S.zip" | sha1sum -c --quiet
    rm -rf "$OUT/movie"
    unzip -q -o "$OUT/8170S.zip" -d "$OUT/movie"
    unzip -q -o "$OUT/movie/Blast Corps Complete Final.bk2" -d "$OUT/movie"
fi
grep -q '^SHA1 185A6EF7BA1ADB243278062C81A7D4E119BDA58C' "$OUT/movie/Header.txt"

# BizHawk's core
if [ ! -f "$BH/.patched" ]; then
    rm -rf "$BH"
    git clone -q --filter=blob:none --no-checkout https://github.com/TASEmulators/BizHawk.git "$BH"
    git -C "$BH" sparse-checkout set libmupen64plus/mupen64plus-core libmupen64plus/mupen64plus-rsp-hle
    git -C "$BH" -c advice.detachedHead=false checkout -q "$BIZHAWK_COMMIT"
    git -C "$BH" apply "$ROOT/port/tools/tas_bizhawk.patch"
    touch "$BH/.patched"
fi
CORE=$M64P/mupen64plus-core/projects/unix/libmupen64plus.so.2
RSP=$M64P/mupen64plus-rsp-hle/projects/unix/mupen64plus-rsp-hle.so
LAX="-fcommon -std=gnu99 -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-implicit-function-declaration"
[ -f "$CORE" ] || make -s -j"$(nproc)" -C "$M64P/mupen64plus-core/projects/unix" all \
    DEBUGGER=1 OSD=0 NO_ASM=1 \
    OPTFLAGS="-O2 $LAX -include $ROOT/port/tools/tas_bizhawk_compat.h -I$M64P/mupen64plus-core/src/main/zip"
[ -f "$RSP" ] || make -s -j"$(nproc)" -C "$M64P/mupen64plus-rsp-hle/projects/unix" all \
    APIDIR="$M64P/mupen64plus-core/src/api" OPTFLAGS="-O2 $LAX"

cc -O1 -Wall -I"$M64P/mupen64plus-core/src/api" -rdynamic -o "$OUT/m64p_tas" "$ROOT/port/tools/m64p_tas.c" -ldl
rm -rf "$OUT/run"
TAS_COUNTER_READS=1 "$OUT/m64p_tas" "$CORE" "$RSP" "$M64P/mupen64plus-core/data" "$ROM" "$LOG" "$OUT/run" "$@"
if [ $# -eq 0 ]; then
    # every level that gives a medal (three of the 60 slots don't)
    python3 "$ROOT/port/tools/tas_check.py" "$OUT/run/eeprom.bin" --platinum 57
else
    python3 "$ROOT/port/tools/tas_check.py" "$OUT/run/eeprom.bin"
fi
