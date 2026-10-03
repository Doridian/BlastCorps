#!/usr/bin/env python3
"""asyncify_names.py BUILD_DIR: check port/web/asyncify_remove.txt against a
WebAssembly build: every name in it should be a function the link defines
exactly once (Binaryen tells two statics of one name apart by a suffix, so
the list would name only one of them, whichever).  A name no object defines
(clang inlined it) only draws a warning from the link; it is listed here too.
Names from the libraries (libc's printing, SDL's) aren't in the build's
objects: those are looked for in the linked module instead.

With binaryen's wasm-dis (emsdk's, in upstream/bin) it also reads the
linked module (BUILD_DIR/blastcorps.wasm, linked with --profiling-funcs to
have the names): how many functions Asyncify instruments (those that read
its state), and any listed one that still is.

    port/tools/asyncify_names.py build/web
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter

here = os.path.dirname(os.path.abspath(__file__))
lst = os.path.join(here, '..', 'web', 'asyncify_remove.txt')
names = [l.strip() for l in open(lst) if l.strip() and not l.startswith('#')]
nm = shutil.which('llvm-nm') or shutil.which('emnm')
if not nm:
    sys.exit('llvm-nm (emsdk\'s, in upstream/bin) is not on PATH')
objs = glob.glob(os.path.join(sys.argv[1], '**', '*.o'), recursive=True)
objs += glob.glob(os.path.join(sys.argv[1], '**', '*.a'), recursive=True)
defs = Counter()
for o in objs:
    out = subprocess.run([nm, '--defined-only', o], capture_output=True, text=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in 'tT':
            defs[p[2]] += 1

# the linked module, as text: its functions, and which read Asyncify's state
funcs = {}
wasm = os.path.join(sys.argv[1], 'blastcorps.wasm')
dis = shutil.which('wasm-dis')
if dis and os.path.exists(wasm):
    with tempfile.TemporaryDirectory() as tmp:
        wat_path = os.path.join(tmp, 'm.wat')
        subprocess.run([dis, wasm, '-o', wat_path], check=True)
        wat = open(wat_path).read()
    e = re.search(r'\(export "asyncify_get_state" \(func \$(\S+?)\)\)', wat)
    g = e and re.search(r'\(func \$' + re.escape(e.group(1)) + r' [^\n]*\n\s*\(global\.get \$(\S+?)\)', wat)
    if g:
        state = '(global.get $' + g.group(1) + ')'
        for fm in re.finditer(r'^ \(func \$(\S+)(.*?)(?=^ \(func |\Z)', wat, re.S | re.M):
            funcs[fm.group(1)] = state in fm.group(2)

bad = 0
for n in names:
    if defs[n] > 1:
        print(f'{n}: defined {defs[n]} times')
        bad += 1
    elif defs[n] == 0 and n not in funcs:
        print(f'{n}: defined nowhere (inlined)')
    if funcs.get(n):
        print(f'{n}: still instrumented')
        bad += 1
print(f'{len(names)} names checked')
if funcs:
    print(f'Asyncify instruments {sum(funcs.values())} of the module\'s {len(funcs)} functions')
elif not dis:
    print('(no wasm-dis on PATH: the module not read)')
sys.exit(1 if bad else 0)
