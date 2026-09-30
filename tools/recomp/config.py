"""Which objects get translated, and where the build puts things.

Rare's handwritten engine is translated, and so is whatever IDO-compiled
code a version still has as asm: a function the C has under a `VERSION_*`
#if as that version's GLOBAL_ASM (jp's 17, say) is missing from the port's
C, so it is translated like the handwritten code (each one is an object of
its own, `<object>/<function>`).  libultra's own .s files and
init's boot code are exception vectors, TLB/cache maintenance and thread
switching: the port's platform layer replaces them, so a call from translated
code into one of them is an external call like any call into C.
"""

import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
BLAST = os.path.join(ROOT, "blastcorps")

# module -> VRAM of its .text (CLAUDE.md, "Layout notes")
MODULE_VRAM = {
    "init": 0x8021ED00,
    "hd_code": 0x802447C0,
    "hd_front_end": 0x801E7000,
}

# Objects libultra or the boot code own; never translated.
LIBULTRA_ASM = {
    "hd_code": {"906E0", "91820", "91F50", "91F80", "92EA0", "95060",
                "96F70", "96FF0", "971D0", "971F0", "972A0", "97B70",
                "98330", "9DB80", "9E6F0", "A0A30"},
    "hd_front_end": set(),
}


def handwritten_objects(module, version="us.v11"):
    """The `asm` subsegments of a module's config, in link order."""
    path = os.path.join(BLAST, f"{module}.{version}.yaml")
    out = []
    for line in open(path):
        m = re.match(r"\s*- \[(0x[0-9A-Fa-f]+), asm, ([^\]]+)\]", line)
        if m:
            out.append((int(m.group(1), 16), m.group(2).strip()))
    return out


# GLOBAL_ASM the port replaces rather than translates: libultra's gu
# (port/src/gu_extra.c)
PORT_REPLACED_ASM = {"hd_code/90C50"}

VERSION_MACRO = {"us.v10": "VERSION_US_V10", "us.v11": "VERSION_US_V11",
                 "jp": "VERSION_JP", "eu": "VERSION_EU"}
GLOBAL_ASM_RE = re.compile(r'^\s*#pragma\s+GLOBAL_ASM\("asm/nonmatchings/([^"]+)\.s"\)')


def global_asm_functions(version="us.v11"):
    """(module, object, function) of every GLOBAL_ASM the C has for `version`,
    by the version conditionals as the N64 build resolves them."""
    import sys
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    from version_ifs import process
    out = []
    for module in ("hd_code", "hd_front_end"):
        d = os.path.join(BLAST, "src", module)
        for f in sorted(os.listdir(d)):
            if not f.endswith(".c"):
                continue
            obj = f[:-2]
            if f"{module}/{obj}" in PORT_REPLACED_ASM:
                continue
            lines = open(os.path.join(d, f), encoding="latin-1").read().split("\n")
            for line in process(lines, VERSION_MACRO[version]):
                m = GLOBAL_ASM_RE.match(line)
                if m:
                    mod, o, fn = m.group(1).split("/")
                    out.append((mod, o, fn))
    return out


def translated_objects(version="us.v11"):
    """(module, object name, .s path) for every object the translator covers."""
    out = []
    for module in ("hd_code", "hd_front_end"):
        for _, name in handwritten_objects(module, version):
            obj = name.split("/")[-1]
            if obj in LIBULTRA_ASM.get(module, ()):
                continue
            out.append((module, obj, os.path.join(BLAST, "asm", name + ".s")))
    for module, obj, fn in global_asm_functions(version):
        out.append((module, f"{obj}/{fn}",
                    os.path.join(BLAST, "asm", "nonmatchings", module, obj, fn + ".s")))
    return out
