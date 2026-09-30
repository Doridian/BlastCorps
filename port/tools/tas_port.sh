#!/usr/bin/env bash
# Replay the TAS on a us.v10 port build and check that it beats the game:
#
#   port/tools/tas_port.sh [BUILD_DIR]      (default build/port.us.v10)
#
# Needs port/tools/tas.sh's log (build/tas/run/) and a port configured with
# -DPORT_VERSION=us.v10 (docs/PORT.md, "The TAS").  The replay's report and
# the save go to build/tas/port/.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${1:-$ROOT/build/port.us.v10}
OUT=$ROOT/build/tas/port
[ -f "$ROOT/build/tas/run/polls.csv" ] || { echo "no log: run port/tools/tas.sh first" >&2; exit 1; }
rm -rf "$OUT"
mkdir -p "$OUT"
"$BUILD/blastcorps" --headless --replay "$ROOT/build/tas/run/polls.csv" --save "$OUT/save.eep" \
    "$ROOT/baserom.us.v10.z64" > "$OUT/log.txt" 2>&1
grep "^replay: [0-9]* reads," "$OUT/log.txt"
python3 "$ROOT/port/tools/tas_check.py" "$OUT/save.eep" --platinum 57
