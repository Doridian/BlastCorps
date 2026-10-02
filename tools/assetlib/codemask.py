"""What in the code modules is code and what is data, from the stage-2
configs (blastcorps/<module>.<version>.yaml, committed for every version).

A module is laid out as the game loads it: init at 0x8021ED00 as the ROM
has it, hd_code and hd_front_end with their .data member right after their
.text member.  Its code is its c and asm subsegments (the functions; the C
files' data are subsegments of their own) and the RSP microcode's text,
which the port never runs (port/tools/asm2x86.py's RSP_TEXT: zeros in every
build).  Everything else is data: .data, .rodata, the bins (the handwritten
code's tables, the RSP's data).  The port's movable builds make their data
from the data alone (port/tools/rom_data.py), and a resource pack carries it
apart from the code (port/make_pack.py, docs/ASSETS.md "The pack").
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
# physical load addresses (CLAUDE.md, "Layout notes")
BASES = {"init": 0x21ED00, "hd_code": 0x2447C0, "hd_front_end": 0x1E7000}
CODE_TYPES = {"c", "asm", "hasm"}
RSP_TEXT = {"hd_code/A0C30", "hd_front_end/20090"}
SUB = re.compile(r"^\s+- \[(0x[0-9A-Fa-f]+)(?:,\s*([A-Za-z.]+)(?:,\s*([\w/]+))?)?\]")


def subsegments(module, version):
    """[(offset, type, name)] of the module's code segment, and its size"""
    subs, size = [], None
    inside = False
    for line in (ROOT / "blastcorps" / f"{module}.{version}.yaml").read_text().splitlines():
        if re.match(r"^\s+subsegments:", line):
            inside = True
            continue
        if not inside:
            continue
        m = SUB.match(line)
        if m and line.startswith("    -"):
            subs.append((int(m.group(1), 16), m.group(2), m.group(3)))
        elif re.match(r"^  - \[(0x[0-9A-Fa-f]+)\]", line):
            size = int(re.match(r"^  - \[(0x[0-9A-Fa-f]+)\]", line).group(1), 16)
            break
    if size is None:
        raise ValueError(f"{module}.{version}.yaml: no end of the code segment")
    return subs, size


def pieces(module, version):
    """[(offset, end, is_code)] covering the module, neighbours of a kind merged"""
    subs, size = subsegments(module, version)
    out = []
    for k, (off, typ, name) in enumerate(subs):
        end = subs[k + 1][0] if k + 1 < len(subs) else size
        if end <= off:
            continue
        code = typ in CODE_TYPES or name in RSP_TEXT
        if out and out[-1][2] == code and out[-1][1] == off:
            out[-1] = (out[-1][0], end, code)
        else:
            out.append((off, end, code))
    if out and out[0][0] != 0:
        out.insert(0, (0, out[0][0], False))
    return out, size


def code_ranges(version):
    """[(physical start, end)] of every module's code"""
    out = []
    for mod, base in BASES.items():
        ps, _ = pieces(mod, version)
        out += [(base + a, base + e) for a, e, code in ps if code]
    return sorted(out)
