#!/usr/bin/env python3
"""asyncify_names.py BUILD_DIR: check port/web/asyncify_remove.txt against a
WebAssembly build: every name in it should be a function the link defines
exactly once (Binaryen tells two statics of one name apart by a suffix, so
the list would name only one of them, whichever).  A name no object defines
(clang inlined it) only draws a warning from the link; it is listed here too.

    port/tools/asyncify_names.py build/web
"""
import glob
import os
import shutil
import subprocess
import sys
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
bad = 0
for n in names:
    if defs[n] != 1:
        print(f'{n}: defined {defs[n]} times')
        bad += defs[n] > 1
print(f'{len(names)} names checked')
sys.exit(1 if bad else 0)
