#!/usr/bin/env python3
"""The ROM's assets as editable files, and back.

  assets.py extract <version>            # baserom -> assets/ (run by make extract)
  assets.py build <version> [--out build/assets] [--shift PAD]

--assets DIR and --rom FILE use another tree and ROM (port/make_pack.py
extracts a resource pack's tree this way).

extract writes, under assets/ (next to splat's segment .bins):

  layout.yaml         the asset segments in ROM order: kind, alignment, the
                      gzip names and mtimes, the model table's entries
  textures/           textures.yaml and a PNG (or .bin) per texture table
                      entry, with the ROM's compressed streams (textures.py)
  audio/              music.ctl/.tbl, sfx.ctl/.tbl, seq/NN.seq (audio.py)
  levels/             one YAML per level file, plus its display data (level.py)
  gzip/               every other gzip member, inflated (.bin)
  images/             the three LZSS background images (PNG)
  static_data.bin     the LZSS blob before hd_code_text, inflated

and checks that building from them gives back every segment byte for byte.

build writes <out>/<segment>.bin for every top-level segment the ROM link
uses: the assets rebuilt (compressed again, the texture and model tables
regenerated from where everything ended up), each padded out to where the
next one starts; the rest (boot, init, the code modules, the trailer)
copied from assets/.  tools/rom_syms.py then places each segment by the
size of its file.  Unchanged assets build to the original bytes; edited
ones may change size, and everything after them moves.

--shift PAD adds PAD bytes (a multiple of 16) before every 16-aligned asset
segment and before the texture table, which moves every asset (the SHIFT=1
test, see CLAUDE.md).
"""
import argparse
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import yaml  # noqa: E402

from assetlib import audio, gz, level, lzss, romlayout, texel, textures  # noqa: E402

ASSETS = Path("assets")
LOADER = getattr(yaml, "CSafeLoader", yaml.SafeLoader)


def load_hints():
    hints = {}
    for line in (HERE / "assetlib" / "texture_dims.txt").read_text().splitlines():
        if line and not line.startswith("#"):
            idx, *dims = line.split()
            hints[int(idx, 16)] = [tuple(int(v) for v in d.split("x")) for d in dims]
    return hints


def config_segments(ver):
    cfg = yaml.load((HERE.parent / f"blastcorps.{ver}.yaml").read_text(), Loader=LOADER)
    out = []
    for s in cfg["segments"]:
        if isinstance(s, list) and len(s) >= 3:
            out.append((s[2], s[0]))
    return out


def flow(d):
    """A one-line YAML mapping."""
    return yaml.safe_dump(d, default_flow_style=True, width=1 << 20, sort_keys=False).strip()


# --- extract ----------------------------------------------------------------

def extract(ver, rom_path=None):
    rom = Path(rom_path or f"baserom.{ver}.z64").read_bytes()
    segs, _ = romlayout.discover(rom, ver)
    names = {s.name: s.start for s in segs}
    for name, start in config_segments(ver):
        if names.get(name) != start:
            sys.exit(f"blastcorps.{ver}.yaml: {name} at 0x{start:X}, the ROM has it at "
                     f"{'0x%X' % names[name] if name in names else 'nothing'}; regenerate it (tools/gen_build_yaml.py)")
    for d in ("textures", "audio/seq", "levels", "gzip", "images"):
        (ASSETS / d).mkdir(parents=True, exist_ok=True)
    first = next(k for k, s in enumerate(segs) if s.name.startswith("init."))
    last = max(k for k, s in enumerate(segs) if s.kind not in ("module", "copy"))
    hints = load_hints()
    lines = [f"# the asset segments of {ver}, in ROM order (tools/assets.py); init comes",
             "# first because the texture table follows it", f"version: {ver}", "segments:"]
    for k in range(first, last + 1):
        s = segs[k]
        nxt = segs[k + 1].start
        d = s.params
        ent = {"name": s.name, "kind": s.kind}
        if s.align > 1:
            ent["align"] = s.align
        data = rom[s.start:s.end]
        if s.kind == "copy":
            ent["file"] = f"{s.name}.bin"
        elif s.kind == "textures":
            textures.extract(rom, ASSETS / "textures", hints, s.start, s.end)
        elif s.kind in ("lzss", "lzss_image"):
            raw, n = lzss.decode(rom, s.start, d["bits"])
            if n != s.end - s.start or lzss.encode(raw, d["bits"]) != data:
                sys.exit(f"{s.name}: LZSS doesn't recompress to the ROM's bytes")
            ent["bits"] = d["bits"]
            ent["file"] = d["file"]
            if s.kind == "lzss_image":
                ent["width"], ent["height"] = d["width"], d["height"]
                texel.write_png(ASSETS / d["file"], "rgba16", raw, d["width"], d["height"])
            else:
                (ASSETS / d["file"]).write_bytes(raw)
        elif s.kind == "raw":
            ent["file"] = d["file"]
            (ASSETS / d["file"]).write_bytes(data)
        elif s.kind == "sequences":
            ent["dir"] = d["dir"]
            audio.seq_extract(data, ASSETS / d["dir"])
        elif s.kind in ("gzip", "level"):
            raw = gz.decompress(data)
            if gz.compress(raw, d["gzname"], d["mtime"]) != data:
                sys.exit(f"{s.name}: gzip doesn't recompress to the ROM's bytes")
            ent["gzname"], ent["mtime"] = d["gzname"], d["mtime"]
            if s.kind == "level":
                level.extract(s.name, raw, ASSETS / "levels")
                ent["file"] = f"levels/{s.name}.yaml"
            else:
                ent["file"] = f"gzip/{s.name}.bin"
                (ASSETS / ent["file"]).write_bytes(raw)
        elif s.kind == "model_table":
            ent["end_of"] = d["end_of"]
            ent["entries"] = d["entries"]
        pad = rom[s.end:nxt] if k < last else rom[s.end:romlayout.align(s.end, 16)]
        if any(pad):
            ent["pad"] = pad.hex()
        if "entries" in ent:
            entries = ent.pop("entries")
            lines.append("- " + flow(ent)[:-1] + ", entries: [")
            lines += [f"    {e if e else 'null'}," for e in entries]
            lines[-1] = lines[-1].rstrip(",")
            lines.append("  ]}")
        else:
            lines.append("- " + flow(ent))
    (ASSETS / "layout.yaml").write_text("\n".join(lines) + "\n")

    # the round trip: every segment must come back as it was
    built = build_all(ver, 0)
    bad = [s.name for s in segs[first:last + 1]
           if built[s.name] != rom[s.start:(segs[segs.index(s) + 1].start if s is not segs[last]
                                           else romlayout.align(s.end, 16))]]
    if bad:
        sys.exit(f"assets: {len(bad)} segments don't rebuild: {', '.join(bad[:10])}")
    print(f"assets: extracted {last - first + 1} segments to {ASSETS}/, all rebuild to the ROM's bytes")


# --- build ------------------------------------------------------------------

def load_layout():
    return yaml.load((ASSETS / "layout.yaml").read_text(), Loader=LOADER)


def seg_data(ent, start, starts, stats):
    """The bytes of one segment placed at start (tables are filled in later)."""
    kind = ent["kind"]
    if kind == "copy":
        return (ASSETS / ent["file"]).read_bytes()
    if kind == "texture_table":
        return bytes(textures.TABLE_SIZE)
    if kind == "model_table":
        return bytes(0x800)
    if kind == "textures":
        table, blob = textures.build(ASSETS / "textures", start, starts["texture_table"], stats=stats)
        ent["_table"] = table
        return blob
    if kind == "lzss":
        return lzss.encode((ASSETS / ent["file"]).read_bytes(), ent["bits"])
    if kind == "lzss_image":
        raw, w, h = texel.read_png(ASSETS / ent["file"], "rgba16")
        if (w, h) != (ent["width"], ent["height"]):
            raise ValueError(f"{ent['file']}: {w}x{h}, want {ent['width']}x{ent['height']}")
        return lzss.encode(raw, ent["bits"])
    if kind == "raw":
        return (ASSETS / ent["file"]).read_bytes()
    if kind == "sequences":
        return audio.seq_build(ASSETS / ent["dir"])
    if kind == "level":
        return gz.compress(level.build(ASSETS / ent["file"]), ent["gzname"], ent["mtime"])
    if kind == "gzip":
        return gz.compress((ASSETS / ent["file"]).read_bytes(), ent["gzname"], ent["mtime"])
    raise ValueError(f"unknown asset kind {kind}")


def build_all(ver, shift, stats=None):
    """{segment name: bytes, padded to the next segment's start}."""
    lay = load_layout()
    ents = lay["segments"]
    starts = {}
    ends = {}
    datas = []
    pos = None
    for k, ent in enumerate(ents):
        al = ent.get("align", 1)
        if pos is None:
            start = romlayout.INIT_START
        else:
            start = romlayout.align(pos, al)
            if shift and (al == 16 or ent["kind"] == "texture_table"):
                start += shift
        if k:
            datas[-1][1] = start - pos     # padding after the previous one
        starts[ent["name"]] = start
        data = seg_data(ent, start, starts, stats)
        ends[ent["name"]] = start + len(data)
        datas.append([data, 0])
        pos = start + len(data)
    datas[-1][1] = romlayout.align(pos, 16) - pos
    out = {}
    for ent, (data, gap) in zip(ents, datas):
        if ent["kind"] == "texture_table":
            data = next(e for e in ents if e["kind"] == "textures")["_table"]
        elif ent["kind"] == "model_table":
            me = starts[ent["name"]]
            end = ends[ent["end_of"]]
            data = b"".join(struct.pack(">I", (starts[e] if e else end) - me) for e in ent["entries"])
        pad = bytes(gap)
        if "pad" in ent and len(bytes.fromhex(ent["pad"])) == gap:
            pad = bytes.fromhex(ent["pad"])
        out[ent["name"]] = data + pad
    return out


def build(ver, out_dir, shift):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    stats = []
    built = build_all(ver, shift, stats)
    # everything else the ROM link uses comes as it is
    for p in sorted(ASSETS.glob("*.bin")):
        name = p.name[:-4]
        if name not in built:
            built[name] = p.read_bytes()
    changed = 0
    for name, data in built.items():
        path = out_dir / f"{name}.bin"
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
            changed += 1
    note = f", {len(stats)} textures recompressed ({', '.join(stats[:8])}{'...' if len(stats) > 8 else ''})" if stats else ""
    print(f"assets: {len(built)} segments in {out_dir}/ ({changed} rewritten){note}"
          + (f", shifted by 0x{shift:X}" if shift else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("action", choices=("extract", "build"))
    ap.add_argument("version")
    ap.add_argument("--out", default="build/assets")
    ap.add_argument("--shift", default="0", help="bytes before each aligned asset (SHIFT=1 test)")
    ap.add_argument("--assets", default="assets", help="the editable tree (default assets/)")
    ap.add_argument("--rom", help="extract: the ROM (default baserom.<version>.z64)")
    args = ap.parse_args()
    global ASSETS
    ASSETS = Path(args.assets)
    shift = int(args.shift, 0)
    if shift % 16:
        sys.exit("--shift must be a multiple of 16")
    if args.action == "extract":
        extract(args.version, args.rom)
    else:
        build(args.version, args.out, shift)


main()
