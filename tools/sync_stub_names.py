#!/usr/bin/env python3
"""Rename GLOBAL_ASM entries in the C files after new names land.

Splat only writes a function's .s when the existing stub already names it, so
a function that gains a name in symbol_addrs (func_80220C40 becoming
osInitialize) would keep pointing at the old, stale func_80220C40.s.  This
rewrites each `func_XXXXXXXX` GLOBAL_ASM line to the name us.v11's
symbol_addrs now gives that address (the names are us.v11's in every
version, tools/vermap.py).  Re-extract afterwards (with asm/ removed, so the
stale files go too).

    tools/sync_stub_names.py
"""
import re
import sys
from pathlib import Path

CODE = Path(__file__).resolve().parent.parent / "blastcorps"
VERSIONS = ["us.v10", "us.v11", "jp", "eu"]
SYMBOL = re.compile(r"^(\w+)\s*=\s*0x([0-9A-Fa-f]+);.*type:func")
STUB = re.compile(r'(GLOBAL_ASM\("asm/nonmatchings/(\w+)/[^"]*/)func_([0-9A-F]{8})(\.s"\))')


def names(module, version):
    path = CODE / f"symbol_addrs.{module}.{version}.txt"
    out = {}
    for line in path.read_text().splitlines():
        m = SYMBOL.match(line)
        if m:
            out.setdefault(int(m.group(2), 16), m.group(1))
    return out


def main():
    for version in ["us.v11"]:
        tables = {}
        for c in sorted((CODE / "src").glob("*/*.c")):
            text = c.read_text()

            def rename(m):
                module = m.group(2)
                if module not in tables:
                    tables[module] = names(module, version)
                new = tables[module].get(int(m.group(3), 16), f"func_{m.group(3)}")
                return f"{m.group(1)}{new}{m.group(4)}"

            new_text = STUB.sub(rename, text)
            if new_text != text:
                c.write_text(new_text)
                print(c.relative_to(CODE))


if __name__ == "__main__":
    main()
