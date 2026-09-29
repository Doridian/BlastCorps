#!/usr/bin/env bash
# Set up a decomp-permuter directory for one function:
#
#   tools/permute.sh <module> <function> [draft.c] [-- import.py args]
#
# base.c is the function's C file, preprocessed, with the function's
# GLOBAL_ASM line replaced by the draft (a file holding the function's
# definition, plus any declarations or macros it needs), or without one by
# m2c's output.  target.o is the function's .s assembled on its own, so the
# function must still be GLOBAL_ASM (splat writes no .s for C functions).  compile.sh is the command make
# would use for that file (IDO recomp, the Makefile's flags, and the file's
# overrides such as -O3 or -mips3).
#
# The directory is permuter/nonmatchings/<function>[-N]/; the script prints
# it.  Then, from the repo root:
#
#   .env/bin/python tools/decomp-permuter/permuter.py -j32 permuter/nonmatchings/<function>
#
# VERSION selects the version (default: the one blastcorps/ is extracted
# for, else us.v11).  The asm must be extracted.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
usage="usage: $0 <module> <function> [draft.c] [-- import.py args]"
MOD=${1:?$usage}
FN=${2:?$usage}
shift 2
DRAFT=
if [ $# -gt 0 ] && [ "$1" != "--" ]; then DRAFT=$(realpath "$1"); shift; fi
[ "${1:-}" = "--" ] && shift
VERSION=${VERSION:-$(cat "$ROOT/blastcorps/.version" 2>/dev/null || echo us.v11)}
S2=$ROOT/blastcorps
PY=$ROOT/.env/bin/python
PERMUTER=$ROOT/tools/decomp-permuter
OUT=$ROOT/permuter

ASM=$(find "$S2/asm/nonmatchings/$MOD" -name "$FN.s" 2>/dev/null | head -1 || true)
[ -n "$ASM" ] || { echo "no $FN.s under blastcorps/asm/nonmatchings/$MOD (not GLOBAL_ASM, or not extracted)" >&2; exit 1; }
OBJ=$(basename "$(dirname "$ASM")")
CFILE=src/$MOD/$OBJ.c
[ -f "$S2/$CFILE" ] || { echo "no $CFILE" >&2; exit 1; }

# The compile command make would run for this file, without asm-processor
# (PERMUTER=1 selects the plain recipe); the per-file flags come with it.
# Paths are made absolute, so that compile.sh can run anywhere.
CC_LINE=$(cd "$S2" && make -n -B VERSION="$VERSION" PERMUTER=1 "build/$CFILE.o" |
          grep -m1 '^\.\./tools/ido5\.3_recomp/cc ')
COMPILER=$("$PY" - "$CC_LINE" "$CFILE" "$S2" <<'PYEOF'
import os, shlex, sys
line, cfile, s2 = sys.argv[1:]
out = []
parts = iter(shlex.split(line))
for p in parts:
    if p in ("-o", "-I"):
        arg = next(parts)
        if p == "-I":
            out += ["-I", os.path.normpath(os.path.join(s2, arg))]
        continue
    if p == cfile:
        continue
    if p.startswith("../"):
        p = os.path.normpath(os.path.join(s2, p))
    out.append(p)
print(shlex.join(out))
PYEOF
)

WORK=$S2/build/permuter
mkdir -p "$WORK" "$OUT"

# The C file with the draft in place of the function's GLOBAL_ASM.  Other
# GLOBAL_ASM lines are dropped (the permuter only compiles this function),
# after tools/version_ifs.py has picked the version's code.
BODY=$WORK/$FN.draft.c
if [ -n "$DRAFT" ]; then
    cp "$DRAFT" "$BODY"
else
    "$ROOT/tools/m2c.sh" "$MOD" "$FN" > "$BODY"
fi
TMPC=$WORK/$OBJ.$FN.c
VERC=$WORK/$OBJ.$FN.ver.c
MACRO=VERSION_$(echo "$VERSION" | tr a-z. A-Z_)
"$PY" "$ROOT/tools/version_ifs.py" "$MACRO" "$S2/$CFILE" -o "$VERC"
"$PY" - "$VERC" "$BODY" "$FN" "$TMPC" <<'PYEOF'
import re, sys
src, body, fn, out = sys.argv[1:]
body = open(body).read()
lines = []
for line in open(src).read().splitlines(keepends=True):
    m = re.match(r'\s*#pragma\s+GLOBAL_ASM\("([^"]*)"\)', line)
    if m:
        if m.group(1).endswith("/" + fn + ".s"):
            lines.append(body if body.endswith("\n") else body + "\n")
        continue
    lines.append(line)
open(out, "w").write("".join(lines))
PYEOF

# The target: the function's .s alone.  .late_rodata_alignment is
# asm-processor's, not the assembler's.
TMPS=$WORK/$FN.s
grep -v '^\s*\.late_rodata_alignment' "$ASM" > "$TMPS"

SETTINGS=$WORK/$FN.toml
cat > "$SETTINGS" <<EOF
compiler_type = "ido"
compiler_command = $("$PY" -c 'import json,sys; print(json.dumps(sys.argv[1]))' "$COMPILER")
assembler_command = "mips-linux-gnu-as -EB -march=vr4300 -mabi=32"
EOF

cd "$OUT"
LOG=$WORK/$FN.import.log
"$PY" "$PERMUTER/import.py" --settings "$SETTINGS" "$TMPC" "$TMPS" "$@" 2>&1 | tee "$LOG"
DIR=$(sed -n 's/^Done. Imported into //p' "$LOG")
[ -n "$DIR" ] || exit 1
DIR=$OUT/$DIR

# IDO leaves its temporaries in the working directory when the permuter
# kills a compile, so compile in a directory of its own.
mkdir -p "$DIR/tmp"
sed -i "s|^cd .*|cd $(printf '%q' "$DIR/tmp")|" "$DIR/compile.sh"

# pycparser reads the `sizeof(Vtx)*(n)` in gSPVertex as sizeof((Vtx)(*n)),
# and writes it back that way.  Put the operands the other way round.
"$PY" - "$DIR/base.c" <<'PYEOF'
import re, sys
p = sys.argv[1]
s = open(p).read()
s = re.sub(r"sizeof\(\((\w+)\) \(\*(\w+)\)\)", r"(\2 * sizeof(\1))", s)
if re.search(r"sizeof\(\(\w+\) \(\*", s):
    print("warning: base.c still has a misparsed sizeof(T) * (expr); fix it by hand",
          file=sys.stderr)
open(p, "w").write(s)
PYEOF
"$DIR/compile.sh" "$DIR/base.c" -o "$DIR/base.o" >/dev/null 2>&1 ||
    echo "warning: base.c does not compile; check it with $DIR/compile.sh" >&2
echo "permuter directory: ${DIR#"$ROOT"/}"
