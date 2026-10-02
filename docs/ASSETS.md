# Assets

Everything in the ROM that isn't code, what format it is in, how the code
finds it, and how the build turns it into editable files and back. The
offsets are us.v11's; the other versions have the same pieces in the same
order (tools/assetlib/romlayout.py finds them in each ROM).

## Flow

```
make VERSION=us.v11 extract   # splat splits the ROM into assets/<segment>.bin,
                              # then tools/assets.py extract writes the editable files
make VERSION=us.v11           # tools/assets.py build -> build/assets/<segment>.bin,
                              # tools/rom_syms.py places them, ld links the ROM
```

`tools/assets.py extract` checks, before it finishes, that building from
the files it wrote gives back every segment of the ROM byte for byte; all
four versions do. The ROM that `make` links is built from the editable
files (PNGs, YAML, ctl/tbl/seq, inflated gzip members), not from splat's
segment bins; only boot, init, the code modules and the trailer are copied.

An edited asset may come out a different size. Each built segment is
padded out to where the next one starts (keeping the alignment the ROM
had: 16 for most, 8 inside the texture data, none between a model and its
display list), `tools/rom_syms.py` places every segment after init by the
size of its built file, and the texture table and the model table are
regenerated from where everything ended up. The code only reaches assets
through linker symbols, so after an edit that moves things, rebuild stage 2
too (it takes the positions from `build/assets`, see `blastcorps/Makefile`)
and it follows. That isn't sha1-checked, so it is done as a `SHIFT=1` build:

```
make VERSION=us.v11 -C blastcorps SHIFT=1 all compress   # also rebuilds build/assets
make VERSION=us.v11 SHIFT=1
```

## The ROM, in order

| segment | us.v11 | format | editable as | the code finds it by |
| --- | --- | --- | --- | --- |
| `header`, `boot` | `0x0` | N64 header, IPL3 | (as is) | |
| `init.<v>` | `0x1000` | code, uncompressed | (stage 2) | the boot code |
| `texture_table` | `0x4CE0` | 4096 x {u32 offset from the table, u16 length, u16 type} | generated | `texture_table_ROM_START` (hd_code 5BF40's `lui 0; addiu 0x4ce0`, in `func_802A0700` and six loaders) |
| `textures` | `0xCCE0` | 3639 textures, mostly "blast" compressed | `textures/` PNGs | the table |
| `music_ctl`, `sfx_ctl` | `0x350950`, `0x3A1920` | libaudio ALBankFile, LZSS (13) | `audio/*.ctl` | hd_code 1C460.c (`func_8028B4C4` then `alBnkfNew`) |
| `music_tbl`, `sfx_tbl` | `0x3539A0`, `0x3A48C0` | ADPCM samples | `audio/*.tbl` | 1C460.c, as `alBnkfNew`'s base |
| `sequences` | `0x44F5C0` | libaudio ALSeqFile ("S1"), 66 .seq | `audio/seq/NN.seq` | 1C460.c (`alSeqFileNew`) |
| 5 images | `0x487050` | gzip (title logo and picture, Nintendo logo, 64K, copyright) | `gzip/*.bin` | `_ROM_START` symbols (hd_code 17E10.c, ...) |
| 21 vehicles and objects | `0x48FE90` | gzip pairs: `<name>` (model) + `<name>_dl` (display list) | `gzip/*.bin` | `_ROM_START` symbols (hd_code 5CB60's `func_802A32CC`, `func_802A396C`, ...) |
| 60 levels | `0x4A5660` | gzip pairs: `<level>` + `<level>_dl` | `levels/<level>.yaml` + `gzip/<level>_dl.bin` | `func_8025615C` (a level's range runs to the next level's start) |
| 4 texture sets, `attract` | `0x66C900` | gzip | `gzip/*.bin` | `_ROM_START` symbols (hd_front_end 9570.c, hd_code 17210.c, ...) |
| `bg_image_1..3` | `0x6AD3F0` | 320x240 RGBA16, LZSS (13) | `images/*.png` | hd_front_end 196F0.c (`func_80200714`); hd_code 17210.c's attract range ends at `bg_image_1` |
| `usa_star`, `ninlogo`, `reflectlogo` | `0x6E8980` | gzip pairs | `gzip/*.bin` | hd_front_end DE70.c (the last range ends at `model_table`) |
| `model_table` | `0x6EC4C0` | 512 x u32 offset from the table | generated | `model_table_ROM_START` (hd_code 5CB60: `func_802A2BB0` DMAs it, `func_802A2A98` loads model *i* from entry *i* to *i+1*) |
| 278 models | `0x6ECCC0` | gzip pairs (`la*`, `ch*`, `co*`, ... and `endsid1..5`) | `gzip/*.bin` | the model table |
| `static_data` | `0x787F40` | 0xC0 bytes (a viewport, then a display list), LZSS (10) | `static_data.bin` | hd_code 00000.c `func_80255DC8` (DMAs 0xC0 bytes: `static_data_DMA_END`) |
| `hd_code_text` ... `hd_front_end_data` | `0x787FD0` | gzip, code and data | (stage 2) | init; `func_8028B3E0` |
| `trailer` | `0x7F9BE0` | 0xFF to 8 MiB | (as is) | |

Every ROM address the code uses is a linker symbol: the stage-2 link
reports (`blastcorps/build/<module>.<v>.syms.txt`) list 99 in hd_code and
14 in hd_front_end, all "ROM segment (linker symbol)", and no "ROM offset"
left. The symbols are `<segment>_ROM_START` (names with the version and
anything that isn't a C identifier character dropped, `64k` is `_64k`),
plus `static_data_DMA_END`; the C and asm still say `D_00350950` and
`tools/link_syms.py` maps each to its symbol by the original address.

## Formats

### Texture table and textures (`textures/`)

Entry *i* of the table is `u32 offset` (from the table's start), `u16
length`, `u16 type`. `func_802A0700` (hd_code 5BF40) DMAs the whole table;
the loaders (`func_802A08E4` and five more there) DMA from the entry's offset to the next entry's, then call
`func_802A57DC` to decode by type. In the ROM the data is in entry order,
each piece 8-aligned. The bytes between one piece and the next are not
zeros but leftovers of Rare's tool; they are kept (`pad`) and written back
as long as the gap is still the same size. 457 entries hold nothing: they
point at the current position with a stale length and type (0x7FFF, 0xFBC,
0, ...) and are kept as `slot: [length, type]`.

`textures/textures.yaml` has one line per entry; data entries are
`NNN.png` (or `NNN.0.png`, `NNN.1.png`, ... when the entry holds several
images, mipmaps), `NNN.rest.bin` for bytes past the last image, `NNN.bin`
for type 0 (uncompressed: the LUTs, and a few raw images), and `NNN.blast`,
the ROM's compressed stream. The types (tools/assetlib/blast.py):

| type | count | texels | notes |
| --- | ---: | --- | --- |
| 0 | 40 | raw bytes | LUTs for 4 and 5, some raw RGBA16/IA16 images |
| 1 | 1259 | RGBA16 | green's low bit dropped |
| 2 | 59 | RGBA32 | 4 bits per channel, 3 of alpha |
| 3 | 122 | IA16 | 7-bit I and A (the display lists load them as IA 16b) |
| 4 | 176 | RGBA16 | 7-bit indices into a 64-entry colour LUT, 1-bit alpha |
| 5 | 1420 | RGBA32 | 11-bit index into a colour LUT, 4-bit alpha |
| 6 | 563 | IA8 | 3-bit I and A: the fonts |

A code with the top bit set copies 1-31 texels from up to 511 texels back;
otherwise it is a literal. The PNGs are 8-bit RGBA (RGBA16/32) or
grey+alpha (IA), widened by bit replication, so they read back exactly.

**The compression is lossy, and not reproducible from the texels.** Rare's
encoder chose its copies by comparing the source image before quantizing
it: of the 1.15M codes, 45k copies are shorter than the longest match the
decoded texture has, and 28k literals had a match (where it did take the
longest, it took the farthest). So the build keeps `NNN.blast` and uses it
as long as the PNG still decodes from it; a changed PNG is compressed by
`blast.encode` (greedy, farthest first), which decodes to the same texels
in the game but is not what Rare's tool would have made. (A test: two of
Simian Acres' bridge textures repainted, the ROM rebuilt with everything
after them 0xD90 bytes earlier, and the bridge in the game shows them.)

Not known yet:

- **Which LUT a type 4/5 texture uses.** The decoders take it from a
  pointer (`$t4`) the loader sets up, not from the entry. Extraction guesses
  the nearest earlier type-0 entry of 0x80 bytes (type 4) or 0x100 (type 5)
  and records it as `lut:`. Only the PNGs' colours depend on the guess: the
  round trip uses the same LUT both ways.
- **Image sizes.** The table has no dimensions. `tools/assetlib/texture_dims.txt`
  has them for 3465 textures: 909 from the display lists that load them
  (`G_SETTIMG` with the index, then `G_SETTILESIZE`; mipmap chains where the
  data is longer than the tile), the rest from gen_splat_yaml's per-offset
  table (queueRAM's guesses by size). The others are written 32 texels wide.
  A wrong size gives a scrambled PNG, never a wrong ROM.

### LZSS (`lzss.py`)

`func_8028B4C4(rom, dst, &size, bits, bits2, method)` DMAs `size` bytes
and, for method 1, inflates gzip; for method 2 it calls hd_code's
handwritten `func_802C41C0` with `bits`. That is Mark Nelson's LZSS from
*The Data Compression Book*: a bit stream, a 1 and eight bits for a
literal, a 0, a `bits`-bit window position and a `16 - bits`-bit length
for a match, position 0 to end; the window starts empty at position 1.
Rare changed BREAK_EVEN to 2, so matches are 3 bytes and up. Nelson's own
binary-tree encoder with that change reproduces all of the ROMs' LZSS data.

### gzip (`gz.py`)

gzip 1.2.4 (`tools/gzip`) at `-6` with Rare's header (FNAME, the file's
own name and mtime, OS 3) recompresses all 738 members of each ROM exactly,
as it does the code modules. `layout.yaml` keeps each member's header name
and mtime. Members are packed back to back; the ones that follow padding
(the first of each group, every level file, the images) are 16-aligned.
One model appears twice (`cohut1` and `cohut1_dl`); the second copies get
their offset in the name.

### Levels (`levels/`, `level.py`)

A level file is `LevelHeader` (blastcorps/include/game/level.h), the
display data segment 8 points at (`<level>.display.bin`: vertices and the
like), then 31 sections in the order of the header's offsets (0x20-0x74,
then 0xA0-0xC0), each running to the next one's offset. The ten
`displayLists` offsets at 0x78 point past the end of the file, into the
`_dl` file loaded after it, and are kept relative to the end. In the YAML,
sections whose records are understood are lists of numbers (fields named
after docs/blast_corps_levels.txt) and the rest hex; a section is only
written as records if they turn back into the same bytes. Offsets are
recomputed on build, so records can be added and removed.

As records: ammo boxes, collision fixes, the communication point, the
terrain (groups of triangles), RDUs, TNT crates, both bounding-box lists,
`unk48`, the level bounds, vehicles, the missile carrier, `unk58`,
buildings, the X/Z collision (groups), and the player collision where it
has the same group layout (3 levels). As hex: the animated textures, the
blocks and holes, `unk60`, `unk64`, the train stops, the player collision
elsewhere, `unk74` and the nine sections at 0xA0 (docs/blast_corps_levels.txt
has partial descriptions of most).

The `_dl` files are Fast3D display lists whose `G_SETTIMG` words hold
texture indices, patched to addresses at load; they stay binary.

### Audio (`audio/`, `audio.py`)

The `.ctl` and `.tbl` are libultra's formats as they are (the `.ctl`
inflated; its wavetables' `base` fields are offsets into the `.tbl`, which
is where the `.tbl` segment's size comes from). The ALSeqFile is split into
its 66 sequences (`audio/seq/NN.seq`, libaudio's format: 16 track offsets,
then the MIDI data); the header is rebuilt, each sequence 4-aligned.

### Everything else

Other gzip members are written inflated (`gzip/<name>.bin`): the vehicle
and object models and their display lists, the title images, the texture
sets, `attract`. Their formats aren't described yet, so they stay binary;
the models are the next thing to type (Phase 3/4 in ROADMAP.md).

## For the port

The PC build (docs/PORT.md) reads the ROM file itself through its PI DMA,
at the addresses the linked code has, so it works with any ROM this build
makes, including a shifted one with its own link. The data that holds
offsets into itself or other assets is: the texture table (from its own
start), the model table (from its own start), the level header
(`AssetOffset`, from the level file's start, and into the `_dl` for the
display lists), the ALBankFile (from the `.ctl`, and into the `.tbl`) and
the ALSeqFile (from its start).

A resource pack (below) is the other way in: the port builds the ROM's
image from the editable files at startup.

## The pack

A resource pack is this tree as a zip, made from the user's ROM by
`port/make_pack.py ROM` (it runs `tools/assets.py extract --assets DIR
--rom ROM` and adds the rest), and the port plays from it instead of the
ROM (docs/PORT.md, "Resource packs").  Everything in it is the ROM's: it is
the user's, never shipped.

```
pack.yaml           format 1, the version, the ROM's sha1, and what isn't an asset (below)
layout.yaml         the asset segments in ROM order, as above
textures/ audio/ levels/ gzip/ images/ static_data.bin      the assets, as above
init.<version>.bin  init as the ROM has it (layout.yaml's first segment; only with the code)
rom/                the header, the boot code and the four gzip members of hd_code and
                    hd_front_end as the ROM has them (only with the code)
data/               the code modules' data: <module>.<offset>.bin
README.txt          a short note for whoever opens the zip
```

`pack.yaml`'s lists:

- `rom:` what isn't an asset, each `{name, file}` (the segment's name in the
  top-level link, its bytes) or `{name, fill}` (the trailer's 0xFF).
- `code: yes|no`: whether `rom/` and `init.<version>.bin` are there.
- `modules:` init, hd_code and hd_front_end: `{name, base, size, text}`,
  the physical load address, the size laid out (`.text` then `.data`) and
  the `.text` member's size.
- `data:` `{module, offset, file}`: every part of a module that isn't code
  (tools/assetlib/codemask.py: all but the `c` and `asm` subsegments of the
  stage-2 config, and the RSP microcode's text): `.data`, `.rodata`, and the
  tables inside `.text` (the sines, the per-level tables, the pointer-bearing
  blobs, the RSP's data).  us.v10: 7 pieces of hd_code (171 K), its .data
  member for hd_front_end (36 K), 2 of init (0.7 K).

**What the port does with it** (port/host/pack.c, in C, at startup):

- Every segment of `layout.yaml` is built as `tools/assets.py build` builds
  it: gzip members through gzip 1.2.4's own deflate at -6 (vendored as
  `port/third_party/gzip-1.2.4`, which gives back all 738 members of every
  ROM; zlib's deflate differs in 86), the LZSS through Nelson's encoder, the
  levels from their YAML, the sequence bank from `seq/`, and the texture and
  model tables from where everything went.
- A segment the code names by its `_ROM_START` symbol goes where the port's
  link has it (`romtab.h`).  The display lists (`*_dl`) follow their files
  and the models after the model table follow each other, each at its
  alignment, wherever that comes to: the game DMAs a level from its start to
  the next level's and inflates its display list from right after it, and
  reaches the models through the table.  So an edited asset may grow into
  the room its group had (a level and its display list, all the models), not
  further: what doesn't fit is an error naming it.  A grown gzip member is
  deflated at -9 first, which usually makes the room a small edit needs.
- `rom/`'s pieces go where the link has them, the trailer is 0xFF.
- From an unmodified ROM's pack the image is the ROM, byte for byte (the
  port says so with `-v`; `PORT_PACK_DUMP=FILE` writes the image).

**Editing:**

| what | how | the game |
| --- | --- | --- |
| a texture | its PNG, any 8-bit or 16-bit PNG of the same size (RGBA, grey, palette; quantized to the texture's format as `texel.py` does) | the ROM's stream stays in the image and the game decodes it as before; the port puts the PNG's texels over what it decoded (docs/PORT.md).  The same time, only the picture changes, and the format's limits are the texture's own (an RGBA16 texture's green keeps its low bit, which Rare's compression dropped) |
| a texture, at a higher resolution | a PNG 2, 3, 4... times as wide and high | averaged down to the texture's size for the game (and the software renderer); the OpenGL renderer draws the full image, wrapped as the tile is (docs/PORT.md) |
| a texture without a stream | no `NNN.blast` (a pack of new art) | compressed by the port (`blast.encode`'s greedy encoder): the texture data then moves, and the table with it |
| a level | its YAML: records, groups, hex, the header | rebuilt and deflated; the records can be added and removed (offsets are recomputed) |
| a sound bank, a sequence | `audio/*.ctl`, `*.tbl`, `seq/NN.seq` | rebuilt (the `.ctl` LZSS'd again) |
| a model, an image, a display list | `gzip/*.bin`, `images/*.png` | deflated or LZSS'd again |
| `data/` | not editable | the movable builds check what they make of it against the image they were built with |
| `rom/` | not editable | |

A texture's size is its `textures.yaml` entry's (`png: [[w, h], ...]`); the
game's buffers and display lists are made for it.  The YAML is read by the
port's own parser (port/host/pack_yaml.c): block and flow style, plain and
quoted scalars, `|`/`>` block scalars, comments; not anchors, aliases, tags
or multi-line plain scalars.  Numbers are YAML 1.1's, as PyYAML reads them
(a leading 0 is octal).

**Without the code** (`make_pack.py --no-code`): no `rom/` code modules and
no init.  The port needs none of the code: the movable builds make the
game's data from `data/`, and the front end's slot, which the game still
DMAs and inflates when it loads the front end (port/src/overlay.c, for the
time it takes), gets a stand-in: its `.text` as zeros and its `.data`,
gzipped.  The game is the same, but that load takes other CPU time than
inflating Rare's code does, so those loads' timing isn't the ROM's (the TAS
still replays exactly from such a pack: docs/PORT.md).
