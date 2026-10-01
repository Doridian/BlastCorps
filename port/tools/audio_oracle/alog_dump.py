#!/usr/bin/env python3
"""Print a libaudio oracle log (port/include/audio_log.h) as text.

    alog_dump.py LOG [--from CALL] [--to CALL] [--mem]

Calls are numbered as the replay numbers them (every CALL, nested ones too).
--mem prints MEM, IN and OUT records' bytes (the first 64).
"""
import argparse
import re
import struct
import sys
from pathlib import Path

HDR = Path(__file__).resolve().parent.parent.parent / "include" / "audio_log.h"


def fn_names():
    text = HDR.read_text()
    block = text[text.index("#define ALOG_FUNCS(X)"):text.index("#define ALOG_ENUM")]
    return ["-"] + re.findall(r"X\((\w+)\)", block)


def records(path):
    data = Path(path).read_bytes()
    w = struct.unpack("<%dI" % (len(data) // 4), data)
    if w[0] != 0x474F4C41:
        sys.exit("not an audio log")
    i = 2
    names = fn_names()
    call = 0
    while i < len(w):
        t = w[i]
        if t in (1, 11):
            yield ("TRACK" if t == 1 else "UNTRACK", call, "%08X +%X" % (w[i + 1], w[i + 2]), None)
            i += 3
        elif t in (2, 4, 5):
            n = w[i + 2]
            raw = data[(i + 3) * 4:(i + 3) * 4 + n]
            yield ({2: "MEM", 4: "IN", 5: "OUT"}[t], call, "%08X +%X" % (w[i + 1], n), raw)
            i += 3 + (n + 3) // 4
        elif t == 3:
            call += 1
            yield ("CALL", call, "%s(%s)" % (names[w[i + 1]], ", ".join("%X" % x for x in w[i + 2:i + 8])), None)
            i += 8
        elif t == 7:
            yield ("RET", call, "%08X" % w[i + 1], None)
            i += 2
        elif t == 8:
            yield ("CBENTER", call, "kind %d (%s)" % (w[i + 1], ", ".join("%X" % x for x in w[i + 2:i + 5])), None)
            i += 5
        elif t == 9:
            yield ("CBRET", call, "%08X extra %08X" % (w[i + 1], w[i + 2]), None)
            i += 3
        elif t == 6:
            yield ("ACMD", call, "%d bytes, hash %08X%08X" % (w[i + 1], w[i + 3], w[i + 2]), None)
            i += 4
        elif t == 10:
            yield ("END", call, "", None)
            return
        else:
            sys.exit("unknown record %d at word %d" % (t, i))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--from", dest="frm", type=int, default=0)
    ap.add_argument("--to", type=int, default=1 << 60)
    ap.add_argument("--mem", action="store_true")
    a = ap.parse_args()
    depth = 0
    for kind, call, text, raw in records(a.log):
        if kind == "CBRET":
            depth -= 1
        if a.frm <= call <= a.to:
            line = "%7d %s%-8s %s" % (call, "  " * depth, kind, text)
            if raw is not None and a.mem:
                line += "  " + raw[:64].hex()
            print(line)
        if kind == "CBENTER":
            depth += 1
        if call > a.to:
            break


if __name__ == "__main__":
    main()
