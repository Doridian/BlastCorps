# Fonts and text

How the game draws its text, what the typefaces are, what may replace them
in a free port, and how the port draws text at high resolution
(`--hd-text`).  Measured on us.v10 and us.v11; the font data is the same
in all four ROMs.  No image here is taken from the ROM.

## How the game draws text

**One drawing routine.**  All text the game spells out, in the front end
and in the levels, goes through drawtext.c (hd_code `14B30`; the name is
its assert's):

| function | what |
| --- | --- |
| `func_80259EC4` | draws a string: per character a quad of four `Vtx` (z -10) with its own colour per corner, texture coordinates s 0..31 and t 31..0 (s10.5 `0x3E0`); the advance is the quad's width times a per-character factor (0.21-0.36; `D_802E8C84` is 0.6/0.95 for a monospaced layout); appended to a list of at most `D_80365350` characters (0x200 in the front end, 0x9C or 0xAC in a level).  jp's is still asm (`GLOBAL_ASM`): it also takes `u16` strings |
| `func_80259CCC` | one colour (`r, g, b, a`) for all four corners |
| `func_80259DC8` | two colours: the top corners and the bottom corners, the vertical gradients of the titles |
| `func_80259824` | the display list: the characters sorted by glyph (a shell sort), then per glyph `gDPLoadTextureBlock_4b(..., G_IM_FMT_I, 32, 32, ...)` and two triangles a character |
| `func_8025946C` | the mode: 1-cycle, `G_RM_XLU_SURF`, `G_CC_MODULATERGBA` (texel times the vertex colour, alpha too), bilinear filtering |

Callers: hd_code `00000`, `1D990`, `26570` (the HUD and the panels: the
money, the timer, the counters, the hint panels' text, the floating
amounts), `42240`, `45BB0`, `55D40`; hd_front_end `00000`, `1C40`,
`6790`, `11530`, `17990` (name entry, menus, the results screen).  A drop
shadow is the same string drawn first in black, a few pixels down and to
one side (26570's panels: `x - 3, y + 3`).  So colour, gradient, shadow
and translucency all come from the vertices and the combiner, not from
the glyphs, which are plain coverage.

**One glyph cache.**  font.c (hd_code `168B0`, `func_8025B0B8`) keeps 0x50
glyphs decoded in RDRAM: slot *i* is 512 bytes at `D_8039CAF0 + 0x200 *
i`, `D_803653B0[i]` is the texture table index it holds (0: free) and
`D_80365360[i]` a frame countdown that frees it.  A glyph not cached is
decoded into a free slot by `func_802A1040` (hd_code 5BF40's texture
loader, handwritten).  Characters map to texture table entries in
`func_80259EC4`'s switch: font 0 (byte strings) is `0xF4C + n`, font 1
(jp's `u16` strings) `0xD4C + code` for codes below 0x200.

**The glyph textures** are entries `0xD4C`-`0xF7E` of the texture table,
type 6 (563 entries, the only ones of that type).  Each decodes to 512
bytes, which the game loads as a **32x32 I4** texture: two 3-bit
intensities a byte (the decoder writes `0..14` per nibble).  The rows are
stored bottom up; the quad's t runs from 31 at the top to 0 at the bottom,
which turns them upright.  (`tools/assetlib` extracts these as 16x32 "IA8"
PNGs, the same bytes read as one grey and one alpha per byte, which is
why the PNGs look like halves of letters, mirrored.)

| entries | glyphs | what |
| --- | ---: | --- |
| `0xD4C`-`0xF4B` | 512 | font 1: a Japanese gothic.  Punctuation, digits, A-Z, hiragana, katakana (both sizes, with the voiced forms), then about 360 kanji.  Only jp's text uses it |
| `0xF4C`-`0xF7E` | 51 | font 0: the stencil face.  `0-9`, `A-D`, a small "DEL" ligature (14, the name entry's delete), `E-Z`, `& ' ) : , $ ! . - ( % ?`, a solid block (49, the name entry's cursor; `#` in strings), `/` |

The stencil face has capitals only, and no accented letters, in every
version (eu's text included).

**What uses which.**  In us and eu every string is the stencil font:
the level names on the results and intro screens (yellow/white or blue
gradients), the results' orange numbers, "LEVEL COMPLETE!",
"CONGRATULATIONS! / MISSION COMPLETE", "DESTROY BUILDINGS IN ...", the
HUD's money, timer and counters, the floating amounts, the hint panels'
text, the name entry's ring of letters and the name, the results' scrolling
line.  jp draws its messages with the gothic.

**Text that isn't text.**  The results screen's GO, REPLAY and EXIT
buttons, the title logo, "(c)1997 Nintendo/Rare. Game by Rare." on the
title, and the like are ordinary RGBA images (texture table and the gzip
images), not glyphs: they don't go through drawtext.c and a font can't
redraw them (see "Options" (c)).

## The typefaces

### Font 0: Stencil (Gerry Powell, ATF, 1937), condensed

**Confidence: high.**  Every glyph is the classic *Stencil*: the
Clarendon-like slab capitals with the stencil breaks, ball terminals on
2, 3, 5, 6, 9, J and the 7's bulb, the open 4, Q's tail, the R's curled
leg.  Set against a specimen of Stencil ([Wikipedia's](https://en.wikipedia.org/wiki/Stencil_(typeface)),
`File:StencilSp.svg`, CC BY 3.0) narrowed to the game's proportions, the
digits match stroke for stroke.  The game's glyphs are Stencil squeezed to
about 60% of its width inside the 32x32 cell (Stencil itself is a wide
face), and rasterized with 8 grey levels.  Powell's design (ATF, July
1937) won over R. Hunter Middleton's Ludlow Stencil of the same year,
and it is the one every digital Stencil follows; which digitization Rare
used (Adobe's, Linotype's, URW's, Monotype's, the one Microsoft Office
installed) can't be told at 32 pixels.

**License situation.**  Every digital Stencil is commercial font
software (Adobe *Stencil Std*, Linotype, URW, Elsner+Flake, Monotype); the
name "Stencil" is a trademark (Linotype's notices: Heidelberger
Druckmaschinen AG).  In the US a typeface design itself isn't copyrightable
(37 CFR 202.1(e)), only the font program, but no free re-drawing of
Powell's Stencil turned up (Google Fonts, the Open Font Library,
FontSpace's "stencil, serif" category), and the port shouldn't depend on
the design-right question anyway: the game's own glyphs are ROM data,
which the project may not ship (docs/DISTRIBUTION.md).

**Lookalike: Stardos Stencil (Vernon Adams, 2011), SIL OFL 1.1.**  A
stencilled serif display face, the only one among the free stencil fonts
checked with Stencil's serifs and contrast.  Compared glyph by glyph with
the game's (each fitted to the game glyph's ink box), on `0 2 3 5 A B G J
M Q R S &`:

| font | license | against the game's glyphs |
| --- | --- | --- |
| **Stardos Stencil Bold** | OFL 1.1 | the closest: the same serif skeleton, ball terminals, stencil breaks in the same places (A's bar, B, R, 0); lighter strokes (made up for by the weight matching below), a flatter 2 and 7 |
| Stardos Stencil Regular | OFL 1.1 | the same, lighter |
| Stint Ultra Condensed | OFL 1.1 | right proportions and slab serifs, but no stencil breaks |
| Sirin Stencil | OFL 1.1 | a humanist brush stencil: too light, wrong terminals |
| Emblema One | OFL 1.1 | heavy and italic-ish, wedge serifs |
| Black Ops One, Saira Stencil One, Allerta Stencil, Big Shoulders Stencil | OFL 1.1 | sans-serif stencils: wrong genre |

Sources: the fonts and their `OFL.txt`/`METADATA.pb` from
[google/fonts](https://github.com/google/fonts/tree/main/ofl/stardosstencil)
(`ofl/stardosstencil`: designer Vernon Adams, license OFL, "Copyright (c)
2011 by vernon adams ... with Reserved Font Names "Stardos" and "Stardos
Stencil""), the
[SIL OFL 1.1 and its FAQ](https://openfontlicense.org/).

**Bundling with an AGPL program.**  OFL 1.1's permission covers it
directly: the font "may be bundled, embedded, redistributed and/or sold
with any software provided that any copy contains the above copyright
notice and this license" (conditions 1-2; `port/fonts/OFL.txt`).  The
[OFL FAQ](https://openfontlicense.org/ofl-faq/): "Only the portions based
on the Font Software are required to be released under the OFL. The
intent of the license is to allow aggregation or bundling with software
under restricted licensing as well" (1.3), games named among the examples
(1.4).  So the font stays OFL and the port AGPL-3.0-or-later; the two
licenses never have to merge, since the port doesn't contain code derived
from the font.  What it asks of the port: the copyright notice and
`OFL.txt` with every copy (in the repo: `port/fonts/OFL.txt`; a binary
release must carry it in its notices, docs/DISTRIBUTION.md), don't sell
the font by itself, and a *modified* font (a subset counts, FAQ 2.6) may
not use the Reserved Font Names "Stardos"/"Stardos Stencil".  The port
renders the font unmodified; the built-in copy is the whole file,
embedded as an array in the executable (`port/tools/embed.cmake`).

### Font 1: a Japanese gothic

**Confidence: low on the exact face.**  A medium-weight Japanese gothic
of the kind Japanese systems and phototypesetters had in 1997 (round-ish
kana, Latin with a single-storey G and a straight-legged R); nothing in it
narrows it to one foundry's face at 32 pixels.

**Lookalikes, all SIL OFL 1.1:** Noto Sans JP (weight 500, the closest in
weight), BIZ UDGothic Bold, M PLUS 1p Bold.  All three cover every
character of the set.  Noto Sans JP is 9.6 MB as a variable font; the
512 glyphs the game has would be a subset of tens of kilobytes (the OFL
allows subsetting; a subset is a modified version, OFL FAQ 2.6, so it
can't carry a Reserved Font Name: check the chosen font's `OFL.txt` for
one before naming the subset).

## Options for high-resolution text in the port

The port's OpenGL renderer draws at `--scale` times 320x240, and textures
are sampled in the shader at the tile's own resolution.  So the text,
32x32 glyphs drawn about 16-26 pixels tall at 1x, is blurry at any scale.
Three ways to do better, all render-side (the game, its memory and its
timing must not change: the TAS and the quick tier's saves and sound are
the check).

### (a) Glyph substitution in the renderer (implemented: `--hd-text`)

The renderer recognizes a glyph texture as it loads it, and uses a
high-resolution version of the same glyph drawn from a font instead.  The
quads, their colours, the gradients, the shadows, the sorting and the
spacing stay the game's, so it works on every screen at once.

- *Recognizing them*: by where the texture came from.  The front end
  records, for each TMEM word, the RDRAM address its LoadBlock read
  (`gfx_tmem_src`, only with the option on).  A 32x32 I4 tile whose 512
  bytes came in one piece from one of font.c's 0x50 slots is a glyph, and
  `D_803653B0[slot]` says which (`hdtext_glyph_at`, read through
  `PORT_VAR`).  No game code changes; the game's variables are only
  read.  Hashing the texels instead would need a table of the ROM's glyph
  hashes, which is ROM-derived.
- *Making the image*: stb_truetype (public domain or MIT, Sean Barrett,
  v1.26, `port/third_party/stb/`) makes a signed distance field for each
  character once: one a retrace from the first (the boot screens have no
  text), about 0.2 s for the 50 characters in all natively and 0.3 s in a
  browser, and at once for a glyph wanted before its turn.  (stb's SDF
  starts its search from the distance where the value clamps to 0 or 255
  anyway, which lets it skip more of the outline's curves: the same bytes,
  1.3-1.9 times faster.)  When
  a glyph is first drawn its 32x32 texels give the box its ink covers (to
  a fraction of a texel, from the coverage of the edge texels), its ink
  area and its peak intensity.  The font's glyph is stretched to that box
  (the game's Stencil is a condensed one, so the fit does the condensing),
  its outline moved in or out until its ink area is the game glyph's (a
  bisection on the distance field: Stardos is lighter than Stencil), and
  written at the game's peak intensity (14/15: the I4 glyphs' 3 bits), at
  256x256 (`HDTEXT_K` = 8 times the tile), rows bottom up like the
  game's.  Under 1 ms a glyph.
- *Uploading it*: one GL texture a glyph, whatever TMEM it came through
  (the game loads the same glyph into many places, each a texture-cache
  entry of its own), kept across the cache's flushes, holding the coverage
  in one channel (R8; the shader reads `.rrrr`, which samples the same as
  the four equal channels did).  Before that, a screen of new text was
  hundreds of 256x256 RGBA uploads with their mipmaps: in a browser on
  SwiftShader one retrace took 2.8 s (`PORT_PERF`, retraces 600-1200 of the
  web page's `PORT_AUTOSTART=3` run: max 2,825 ms and 9.1 ms of presenting
  a retrace; now 712 ms and 1.6 ms, against 605 ms and 1.3 ms without
  `--hd-text`).
- *Drawing it*: the cache entry is marked high-resolution, the draw's
  program gets a `texel_hd` that samples it with GL's filtering and
  mipmaps (so it stays smooth when the game draws small text) at the
  tile's coordinates times `HDTEXT_K`, the texel centres lined up with
  the RDP's (texel *c* at *c*, not *c* + 0.5).  Programs without such a
  texture are unchanged, so with the option off every program and every
  pixel is the same as before.

The small "DEL" glyph is split into its three pieces of ink (connected
texels), and D, E and L are each fitted to theirs; the cursor block is
drawn as an exact box.  Not covered: jp's gothic
keeps its textures: it needs a table from the 512 glyph indices to
Unicode (a character table, which could be written by hand from the
glyphs) and a Japanese font (above).  The software renderer is 320x240
and unchanged.

### (b) String-level rendering

Hook `func_80259EC4` (under `TARGET_PC`, or from the port's glue) to
record each string with its position, size, colours and spacing, and draw
real text from a font at the end of the frame.  It would give kerning and
the font's own spacing, but: the game's spacing is part of its layout
(boxes, the name entry's ring, the scroller's clipping), the characters are
sorted by glyph and drawn into the frame between other things (the order
against the panels and the shadows matters), the name entry's letters are
quads under a 3D matrix, and the text is sometimes clipped by the
scissor.  Re-doing all that outside the display list is far more
invasive than (a) for a result that (a) already gets: the glyphs are
fitted to the game's own boxes, so the spacing is the game's either way.
Worth it only for text the game doesn't have: a different language, or
a new menu.

### (c) Texture packs by hash

The usual N64-port mechanism: hash each texture as loaded, look the hash
up in a directory of user-provided PNGs.  It would cover what (a)
can't: the GO/REPLAY/EXIT buttons, the logos, the HUD icons.  But the
replacement images for those would be redrawings of ROM art, so the
project can only ship the mechanism, never a pack made by upscaling the
ROM's images (docs/DISTRIBUTION.md).  For the fonts (a) is better than a
pack: it's generated at run time from a font the project may ship, needs
no files, and follows the font when a user passes another one.  A pack
mechanism can reuse (a)'s hook (`tile_texture`'s high-resolution cache
entries and `texel_hd`).

## The prototype: `--hd-text`

```
build/port-us.v10/blastcorps --renderer gl --scale 4 --hd-text baserom.us.v10.z64
build/port-us.v10/blastcorps --hd-text MyFont.ttf ...      # another font (any .ttf/.otf/.ttc)
PORT_HD_TEXT=1                                             # the same from the environment (the page: ?env=PORT_HD_TEXT=1)
PORT_HD_TEXT=path/to/font.ttf
PORT_HD_TEXT_WEIGHT=1.1                                    # the ink area against the game's (default 1)
```

OpenGL only; on by default with a window (`--no-hd-text` turns it off),
off headless and in compared runs.  `-v` logs each glyph's box and outline
offset.  Code: `port/host/hdtext.c` (the glyphs), `gfx.c`
(`gfx_tmem_src`), `gfx_gl.c` (`tile_glyph`, `tile_texture`'s
high-resolution entries, `fs_hd`/`texel_hd`, the `hd` bit of a
program's key), `main.c` (the options).  The built-in font is embedded at
build time from `port/fonts/StardosStencil-Bold.ttf`.

What it looks like (the TAS at `--scale 4`, and `PORT_AUTOSTART=2` for the
hint panel), against the same frames without it:

- **Results screen** (frame 4740): "SIMIAN ACRES" in its yellow-white
  gradient, the orange "18 (100%)", "$1838010", "100 (100%)" and the
  scrolling white line: sharp serifs and stencil breaks instead of
  bilinear-smeared 32-pixel glyphs; same positions, colours, gradient and
  black drop shadows.  GO/REPLAY/EXIT are images and unchanged.
- **Name entry** (1200): the ring of letters (quads under a rotation)
  sharp at every angle, "DEL" as three small letters in the game's
  places.
- **Hint panel** (`PORT_AUTOSTART=2`, 2100): "PRESSING START WILL ALLOW
  YOU TO VIEW THE MISSILE CARRIER'S PATH." crisp at the small size, the
  panel's soft shadow (drawn in software into an 8-bit texture) as before.
- **Level intro** (1500) "SIMIAN ACRES" in the blue gradient,
  "CONGRATULATIONS! / MISSION COMPLETE" (4500), "DESTROY BUILDINGS IN 1
  MINUTE 0 SECONDS" (5100), the HUD's "$..." (1800), "2/6" and the timer
  (5550), the translucent floating amounts (2400): all redrawn, with the
  same translucency.

Checks: without the option every screenshot (GL, `--scale 4`, eight frames
of the TAS) is byte for byte the build's before the change, and the quick
tier passes; with it the saves are the same, and the quick tier has two
scenarios for it (`auto2.gl.hdtext`, `auto3.gl.hdtext` with
`--interpolate`) whose save and sound must equal the plain runs' (all
seven standard variants pass them, and their hint-box screenshots
change with the option: the glyphs are found in every memory layout).
The TAS with `--hd-text --renderer gl --scale 2` on the 64-bit build:
125297 of the log's 125297 reads matched, none skipped, no mode forced,
57 platinum, the same report as without it.

## What's next

- jp: the gothic's index-to-Unicode table and a Japanese OFL font (Noto
  Sans JP 500, subset), same mechanism.
- Optional: a texture-pack directory (c) on the same hook, for the image
  text, with packs made by users.
