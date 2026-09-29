#!/usr/bin/env python3
"""Summarise a Blast Corps EEPROM save (the EepromSave in game/player.h).

    tas_check.py SAVE.eep [--platinum N]

Prints the player record: name, level, game state, units and the medal
count per kind.  With --platinum N it exits 1 unless at least N levels have
a platinum medal, which is how port/tools/tas.sh tells that the movie beat
the game.
"""
import argparse
import struct
import sys

MEDALS = {0: "none", 1: "bronze", 2: "silver", 3: "gold", 4: "platinum", 5: "done"}
SEMAPHORE_OK = 0x87569AB6CD076AEC


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("save")
    ap.add_argument("--platinum", type=int)
    args = ap.parse_args()
    data = open(args.save, "rb").read()[:0x200]
    if len(data) < 0x200:
        sys.exit(f"{args.save}: shorter than the EEPROM's 512 bytes")
    name = data[0:8].split(b"\0")[0].decode("ascii", "replace")
    levelno, units, game_state = data[8], struct.unpack(">H", data[0xA:0xC])[0], data[0x91]
    medal = data[0x18:0x18 + 60]
    semaphore = struct.unpack(">Q", data[0x1F8:0x200])[0]
    counts = {k: sum(1 for m in medal if m == k) for k in MEDALS}
    print(f"name {name!r} level {levelno} gameState {game_state} units {units} "
          f"semaphore {'ok' if semaphore == SEMAPHORE_OK else hex(semaphore)}")
    print("medals: " + ", ".join(f"{MEDALS[k]} {n}" for k, n in counts.items() if n and k))
    print("per level: " + " ".join(str(m) for m in medal))
    if args.platinum is not None and counts[4] < args.platinum:
        sys.exit(f"only {counts[4]} platinum medals, expected {args.platinum}")


if __name__ == "__main__":
    main()
