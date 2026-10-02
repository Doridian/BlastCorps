#!/usr/bin/env python3
"""Turn your ROM into a resource pack: its assets as editable files, zipped.

    port/make_pack.py ROM                 writes blastcorps-<version>-pack.zip
    port/make_pack.py ROM -o FILE.zip
    port/make_pack.py ROM --dir DIR       the unpacked tree instead of a zip

The port plays from the pack instead of the ROM (`blastcorps PACK.zip`, or
`--pack`; the page takes it like a ROM).  A pack from an unmodified ROM
makes the same game byte for byte.  Edit the files (a texture's PNG, a
level's YAML, a sound bank, a sequence) and zip them up again, or point the
port at the directory: docs/ASSETS.md ("The pack") has the format and what
can be edited how, docs/PORT.md ("Resource packs") how the port reads it.

The tree is tools/assets.py's (make extract's assets/): textures/,
levels/, audio/, gzip/, images/, layout.yaml, plus pack.yaml and rom/, the
pieces that aren't assets (the boot code and the code modules, as the ROM
has them).  Everything in it comes from your ROM: don't share it.

It needs the project's Python venv (.env, which port/build.py sets up) for
PyYAML and pypng, and runs itself again under it.
"""
import argparse
import hashlib
import io
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VERSIONS = {
    "185a6ef7ba1adb243278062c81a7d4e119bda58c": "us.v10",
    "483f7161aea39de8b45c9fbc70a2c3883c4dea8c": "us.v11",
    "b147fdbeb661c89107c440b00dc4810508f58636": "jp",
    "460212600f8b9f0da95219c4c7330f2e626d9a7e": "eu",
}
VENV_PY = ROOT / ".env" / "bin" / "python"
FORMAT = 1

README = """\
A Blast Corps resource pack ({version}), made from your ROM by port/make_pack.py.

Everything here comes from the ROM: keep it to yourself.

  textures/   textures.yaml and a PNG per texture (NNN.png, NNN.0.png ... for
              mipmaps); NNN.blast is the ROM's compressed stream, used while the
              PNG is unchanged
  levels/     a YAML per level (header, sections, records) and its display data
  audio/      the sound banks (.ctl, .tbl) and the sequences (seq/NN.seq)
  gzip/       the other compressed files, inflated: models, images, display lists
  images/     the three 320x240 backgrounds
  layout.yaml the order of it all in the ROM; pack.yaml and rom/: what isn't an asset

Edit a file, zip the folder again (or point the port at the folder), and play it:
    blastcorps --pack mypack.zip
A PNG may be replaced by one the same size, or 2, 3, 4... times as wide and high.
docs/ASSETS.md ("The pack") in the source has the details.
"""


def rom_z64(data):
    magic = data[:4]
    if magic == b"\x37\x80\x40\x12":
        b = bytearray(data)
        b[0::2], b[1::2] = data[1::2], data[0::2]
        return bytes(b)
    if magic == b"\x40\x12\x37\x80":
        b = bytearray(data)
        for k in range(4):
            b[k::4] = data[3 - k::4]
        return bytes(b)
    if magic != b"\x80\x37\x12\x40":
        sys.exit("make_pack.py: that isn't an N64 ROM")
    return data


def make_tree(rom, version, out):
    """the editable tree of `rom` in `out`"""
    sys.path.insert(0, str(ROOT / "tools"))
    from assetlib import romlayout
    out.mkdir(parents=True, exist_ok=True)
    segs, end = romlayout.discover(rom, version)
    for s in segs:
        if s.kind == "copy" and s.name.startswith("init."):
            (out / f"{s.name}.bin").write_bytes(rom[s.start:s.end])     # layout.yaml's copy
    with tempfile.TemporaryDirectory() as td:
        rp = Path(td) / f"baserom.{version}.z64"
        rp.write_bytes(rom)
        subprocess.run([sys.executable, str(ROOT / "tools" / "assets.py"), "extract", version,
                        "--assets", str(out), "--rom", str(rp)], check=True)
    pieces = []
    (out / "rom").mkdir(exist_ok=True)
    for s in segs:
        if s.name in ("header", "boot") or s.kind == "module":
            fn = f"rom/{s.name}.bin" if s.kind != "module" else f"rom/{s.name}.gz"
            (out / fn).write_bytes(rom[s.start:s.end])
            pieces.append(f"- {{name: {s.name}, file: {fn}}}")
        elif s.name == "trailer":
            fill = set(rom[s.start:s.end])
            if fill != {0xFF}:
                sys.exit("make_pack.py: the trailer isn't all 0xFF")
            pieces.append(f"- {{name: trailer, fill: 0xFF}}")
    sha1 = hashlib.sha1(rom).hexdigest()
    (out / "pack.yaml").write_text(
        "# a Blast Corps resource pack: the ROM's assets as editable files\n"
        "# (docs/ASSETS.md, \"The pack\"; made by port/make_pack.py)\n"
        f"format: {FORMAT}\n"
        f"version: {version}\n"
        f"rom_sha1: {sha1}   # the ROM it was made from\n"
        "# what isn't an asset, as the ROM has it, at the segment the port's link names\n"
        "rom:\n" + "\n".join(pieces) + "\n")
    (out / "README.txt").write_text(README.format(version=version))


def zip_tree(tree, path):
    """a zip of the tree, the same bytes every time (sorted, fixed dates)"""
    names = sorted(str(p.relative_to(tree)).replace(os.sep, "/") for p in tree.rglob("*") if p.is_file())
    with zipfile.ZipFile(path, "w") as z:
        for n in names:
            info = zipfile.ZipInfo(n, date_time=(1997, 3, 1, 0, 0, 0))
            info.external_attr = 0o644 << 16
            # PNGs and the ROM's own streams don't deflate much
            stored = n.endswith((".png", ".blast", ".gz"))
            info.compress_type = zipfile.ZIP_STORED if stored else zipfile.ZIP_DEFLATED
            z.writestr(info, (tree / n).read_bytes(), compresslevel=None if stored else 9)
    return len(names)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rom")
    ap.add_argument("-o", "--out", help="the zip (default blastcorps-<version>-pack.zip)")
    ap.add_argument("--dir", help="write the unpacked tree here instead of a zip")
    args = ap.parse_args()
    try:
        import png  # noqa: F401
        import yaml  # noqa: F401
    except ImportError:
        if VENV_PY.exists() and not os.environ.get("MAKE_PACK_VENV"):
            os.environ["MAKE_PACK_VENV"] = "1"
            os.execv(str(VENV_PY), [str(VENV_PY), __file__] + sys.argv[1:])
        sys.exit("make_pack.py: needs PyYAML and pypng (the venv port/build.py makes: .env)")
    rom = rom_z64(Path(args.rom).read_bytes())
    version = VERSIONS.get(hashlib.sha1(rom).hexdigest())
    if not version:
        sys.exit("make_pack.py: not a Blast Corps ROM this project knows (us.v10, us.v11, jp, eu)")
    if args.dir:
        out = Path(args.dir)
        if out.exists() and any(out.iterdir()):
            sys.exit(f"make_pack.py: {out} isn't empty")
        make_tree(rom, version, out)
        print(f"make_pack.py: {version}'s pack in {out}/")
        return
    zpath = Path(args.out or f"blastcorps-{version}-pack.zip")
    with tempfile.TemporaryDirectory() as td:
        tree = Path(td) / "pack"
        make_tree(rom, version, tree)
        n = zip_tree(tree, zpath)
    print(f"make_pack.py: {version}'s pack, {n} files, {zpath.stat().st_size // 1024} KB: {zpath}")


if __name__ == "__main__":
    main()
