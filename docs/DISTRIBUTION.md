# Distributing the port

For now the PC port is **built by its users** from their own ROM (README, "The PC port"): no binary or
hosted page is published. This file records what a built port contains, so that publishing later can be
planned. It is an inventory by origin, not legal advice. Measured on us.v10 builds (32-bit, LP64,
movable 64-bit, wasm web) on 2026-09-30; us.v11's sizes are near-identical.  `port/tools/rom_scan.py
FILE...` checks a build for the ROM's data.

**Estimates** in this file are in agent-hours: the wall-clock time one agent takes, including
running the checks (a full `test.py variants --tas` is about half an hour of that).  They come from
this project's own pace (2026-09-29/30), not from human-calendar guesses:

| Piece | Earlier estimate | Took |
|---|---|---|
| The data from the ROM at run time (`PORT_ROM_DATA`, `rom_scan.py`) | 1–2 weeks | 1.1 agent-hours, TAS on all variants included |
| The movable build, from the arena link to LP64 as a PIE | — | about 2 hours |
| The WebAssembly build, from node to the page | — | about 1.5 hours |
| The results screen's black boxes and the medal's seam | — | 2 agent-hours, with a 20,000-frame screenshot sweep |
| Typing the name on the name entry | — | 1 agent-hour |
| The path-tracing investigation | — | 8 agent-minutes |
| The picture following the window's shape, and the HUD to the sides (`--hud`) | — | 1 agent-hour, with the TAS at 21:9 |
| libaudio's oracle (recorder and replay) | in the 6–12 below | 0.6 agent-hours |
| libaudio, exact (the port's own, `port/libaudio`) | 6–12 agent-hours exact | 0.6 agent-hours to exact on every log, 1.5 more for the suites (three versions' references again, the TAS twice) |
| The SDK's gu, sinf/fcos, sins/coss, crc and `ll.c` out of the port (`port/src/gu.c`, `sdk_check`) | about 2 agent-hours | 1.5 agent-hours, the exhaustive and traced checks and the quick tier included; the TAS another half hour |
| The SDK headers and gbi.h (`port/include/sdk`, `sdk_identity.py`) | 1–2 agent-hours | half an agent-hour, the identity check on five variants included |
| `--hd-text` made fast (shared R8 glyph textures, distance fields between retraces) | — | 40 agent-minutes, measured natively and in headless Chromium |
| The engine's last pieces: 8AEE0's three, jp's 18 (9,500 IDO instructions), the check build's fuzz, no translation in the default build | — | about 3 hours of wall clock, about 7 agent-hours: three helper agents wrote and fuzzed most of jp's in parallel |
| The engine's loaders, terrain and textures (7F8B0, 5BF40, 60D50, 8A080, 5FD50, 60F60; most of 5CB60; 103 functions) | — | about 11 agent-hours: 2.5 drafting before the mechanism, 1.5 for the native-endian, LP64 and port-arena fixes, 7 converting and checking (5CB60's leftovers most of that) |
| Resource packs (issue #4): the ROM as editable files, the port building its image from them in C, edited textures over the game's decode, higher-resolution textures in OpenGL, packs without the code, the page's upload | — | about 1.5 agent-hours of wall clock, the TAS runs (32, wasm, 32 without the code) and three versions' quick tiers included |

So a piece with an exact oracle (the difftest, the TAS, object identity) takes about an agent-hour
where a person's estimate said a week.  Work checked by eye or ear, or whose design is still open, goes
slower.  Several agents can run at once (about four has worked here), so calendar time is shorter.

## What a built port contains

**Not embedded:**
- The ROM file itself: 3,000 random 32-byte chunks of it and its IPL3 were searched for in the
  executables, the .wasm and the .js, with no hits.
- **In the movable builds (64-bit, LP64, wasm), none of the ROM's data** (1a–1e below): with
  `PORT_ROM_DATA`, their default, the arena image is made from the user's ROM at startup (docs/PORT.md,
  "The data from the ROM").  `port/tools/rom_scan.py` searches a build for every stretch of 24 bytes or
  more of the ROM and of its code modules as the game loads them (hd_code and hd_front_end inflated from
  their gzip members, as they are and with 2- or 4-byte units reversed for native-endian data): on
  2026-09-30, us.v10, none in the m64, mn32 and mlp64 executables or in the web build's .wasm, .js and
  .html.  `test.py quick` runs it on every such build.
- The TAS data (`polls.csv`, `checkpoints.csv`, `switches.csv`...), which `--replay` reads beside its file
  at run time.
- `test_refs.json` and the mupen64plus tools.
- Base64 or other blobs in the page. `?rom=` only fetches the URL the user gives.
- A resource pack (docs/PORT.md, "Resource packs"): the user makes it from their ROM with
  `port/make_pack.py` and the port reads it at run time.  It is the ROM's assets and data as files,
  ROM-derived like the ROM itself, with or without the code modules (`--no-code`), and is never
  shipped.  The port's pack code (row 4i) holds the formats' layouts only.

| # | Origin | What | Size (us.v10) | Where it enters |
|---|---|---|---|---|
| 1a | ROM bytes | hd_code and hd_front_end `.data`/`.rodata` from splat's extraction: menu structures, display lists, text tables, libultra's VI modes | ~38 K (+353 K bss) | `blastcorps/asm/data/**` → `asm2x86.py` / `asm2ll.py` |
| 1b | ROM bytes | the two pointer-bearing data islands (7D9D0, 800DC) | 8.6 K | same |
| 1c | ROM bytes | bin islands: sine and u16 tables, per-level tables, RSP microcode data (6.8 K); the microcode's text (18.3 K) is zeros in every build now (the HLE never runs it) | 15 K | `blastcorps/assets/<module>/*.bin`, `.incbin`'d |
| 1d | ROM bytes | the decompiled C's data initializers (`tools/data_c.py`): textures/TLUTs ~62 K, struct tables ~46 K, strings, Vtx/Gfx | ~206 K of sections | `blastcorps/src/**/*.c` |
| 1e | all of 1a–1d as carried | the movable/wasm build's arena image, 106 runs; **not carried with `PORT_ROM_DATA`** (the default), which carries operations instead (row 4e) | 192 K (`PORT_ROM_DATA=OFF`) | `bepass/Arena.cpp` `writeImage()` |
| 2 | translated ROM code | **none in the default build** (2026-10-02): Rare's handwritten engine (688 functions, 220 K of MIPS) and jp's 17 IDO `GLOBAL_ASM` functions and `func_802BA3E8_jp` are hand-written C in `port/engine` (row 4g); the translation is compiled only into the check build (`PORT_ENGINE_CHECK`), which runs it against the native code, and `test.py quick` fails any other build that compiles or links it | — (was ~1.1–1.4 MB of host code) | (`tools/recomp` → `blastcorps/build/recomp/src`, check build only) |
| 3a | decompiled C | the game's own code | 370 K x86 | `blastcorps/src/**` |
| 3b | decompiled SDK | **none built into the port** (2026-10-01): gu, sinf/fcos, sins/coss and `ll.c` are `port/src/gu.c` (row 4a), libaudio is `port/libaudio` (row 4f); the decompiled libaudio is built only with `-DPORT_LIBAUDIO_ORIGINAL=ON` and by the libaudio oracle | — | (`blastcorps/src/libultra/audio/**`) |
| 3c | SDK source | **gone** (2026-10-01): `port/src/gu_extra.c` (guRotate(F), the SDK's `mtxcatl.c` and `io/crc.c`) is replaced by `port/src/gu.c` | — | — |
| 3d | SDK headers | **none** (2026-10-01): the port's own, `port/include/sdk/` (rows 4a, 4f); only macros, layouts and constants reach the binary | — | compile time |
| 3e | gzip 1.2.4 as Rare linked it | `inflate` (public domain) and unzip/get_method/bi_reverse/clear_bufs, which are GPLv2+ upstream | 10 K x86 | `src/gzip_*.inc.c` |
| 4a | the port's own code | host renderer, audio HLE, loaders, replay, fibers; the graphics utilities, sines and pak CRC (`port/src/gu.c`, with SGI's four sine coefficients, which the owner allowed; its sine table generated by `port/tools/gen_sintable.py`) | ~140 K | `port/host`, `port/src` |
| 4b | third-party, native | SDL2 (zlib), libepoxy (MIT), glibc (LGPL), libgcc_s: dynamic | — | system |
| 4c | third-party, wasm | emscripten's SDL2 (zlib), musl (MIT), emscripten runtime (MIT/UIUC), compiler-rt (Apache-2.0 with LLVM exception) | in .wasm/.js | emsdk |
| 4d | third-party, all builds | Stardos Stencil Bold (SIL OFL 1.1, Vernon Adams; `--hd-text`'s built-in font, docs/FONTS.md), stb_truetype 1.26 (public domain or MIT) | 33 K font in the executable; hdtext.c with stb_truetype ~28 K x86 | `port/fonts/` via `port/tools/embed.cmake`, `port/third_party/stb/` |
| 4e | the port's own, movable builds | `PORT_ROM_DATA`'s operations: copies from the ROM's modules by address, and 2.9–3.4 K of literal bytes, all the port's (pointer words to data port-arena moved, clang's switch tables of the game's C, `port/src`'s strings; `gen/romdata_report.txt` lists them); an inflate of its own | 11–26 K, inflate ~3 K x86 | `port/tools/rom_data.py` → `gen/romdata_ops.c`, `port/host/romdata.c` |
| 4f | the port's own | libaudio (`port/libaudio`, header `port/include/sdk/PR/libaudio.h`), exact; the eqpower table is computed at startup | 35 K x86-64 (the 64-bit build; the decompiled one was 44 K) | `port/libaudio` |
| 4g | the port's own | the engine: Rare's handwritten code and jp's IDO asm as hand-written C, charged the original's instructions block by block (docs/PORT.md, "Replacing the engine") | ~42,000 lines of C | `port/engine` |
| 4h | third-party, all builds | gzip 1.2.4's deflate (`deflate.c`, `trees.c`, `bits.c`, unchanged; GPLv2 or later, Jean-loup Gailly; its `COPYING`), the only deflate that gives back the ROM's gzip members; stb_image 2.30 (public domain or MIT; PNGs and the zip's inflate) | 16 K and 37 K x86-64 | `port/third_party/gzip-1.2.4`, `port/third_party/stb/stb_image.h` |
| 4i | the port's own | resource packs: the zip reader, the YAML parser, building the ROM's image from the pack, the texture overrides (`host/pack*.c`), `port/make_pack.py`, `tools/assetlib/codemask.py` | 53 K x86-64 | `port/host` |

In the movable builds, rows 1a–1d reach the executable only through 1e, so with `PORT_ROM_DATA` none of
them does.  The non-movable builds (32, 64, n64, lp64) still carry 1a–1d: `rom_scan.py` finds 158 K of
the ROM's data in the 32-bit executable (the C's data in host order there, which BEPass's constructors
swap at startup).  Sizes, us.v10, `PORT_ROM_DATA` off and on (`.text`, which holds the image): m64 2,231 K
to 2,072 K, mn32 2,369 K to 2,226 K, mlp64 2,149 K to 2,002 K, the web build's .wasm 3,568 K to 3,450 K.

**Also:**
- The native builds carry about 7.5 MB of DWARF and symbols, with the decomp's names and build paths.
  `llvm-strip` removes them (9.7 MB to 2.2 MB).
- `PORT_N64_FUNCS` compiles one absolute build path into every binary.
- The repository has no root LICENSE yet.

## What publishing binaries would take

1. **Movable builds only** (wasm, 64-bit native). They are the macOS and wasm path anyway, and their
   data is a single image.
2. **The data from the user's ROM at run time: done** for the movable builds (`PORT_ROM_DATA`, their
   default; docs/PORT.md, "The data from the ROM").
   - At startup the port inflates hd_code and hd_front_end from the ROM (an inflate of its own, not the
     game's, whose tables are themselves the ROM's data), lays them and init out at their N64 addresses,
     and applies operations `port/tools/rom_data.py` made at build time from port-arena's image: copies
     from the same address (as they are, or with 2-, 4- or 8-byte units reversed for native endian),
     copies from elsewhere (what port-arena moved above 0x400000, LP64's outgrown variables), and literal
     bytes.
   - The build fails unless the operations, applied to the build machine's ROM, give the image byte for
     byte; the port checks a hash of what it made.  The literal bytes (2.9–3.4 K) are listed by variable
     in `gen/romdata_report.txt`, and are all the port's (row 4e).
   - The ROM's sha1 is checked at startup, in every build (`PORT_ROM_ANY=1` downgrades it to a warning).
   - The RSP microcode text is zeros, in every build.
   - It costs about 7 ms at startup.
   - **Not done: the non-movable builds**, which keep carrying 1a–1d.  There the game's variables are the
     executable's own sections at their N64 addresses, filled by the loader, and the C's are byte-swapped
     by BEPass's constructors before `main` has read the ROM, so taking them from the ROM would mean
     blanking ranges of the linked executable after the link and reordering the startup.  Since only the
     movable builds would be published (item 1), it wasn't worth it.
3. **The engine rewrite (category 2): done** (2026-10-02).  The translation is ROM-derived by
   construction, so a published binary needed Rare's engine as hand-written C; it is (`port/engine`, row
   4g), in every version, and the default build compiles none of the translation (see below).
4. **`gu_extra.c`** replaced by the port's own implementation: **done** (2026-10-01, `port/src/gu.c`, with gu, sinf/fcos, sins/coss and the pak CRC; `ll.c` dropped).  The gzip driver's GPLv2+ is fine: the project
   is to be licensed AGPL-3.0 (GPL otherwise, below), which GPLv2-or-later code can join.
5. **Strip releases**, and drop `PORT_N64_FUNCS`.
6. **Notices** for SDL2, libepoxy, musl and emscripten, the bundled font (its `port/fonts/OFL.txt`), stb_truetype and stb_image,
   gzip 1.2.4's deflate (GPLv2 or later: its `port/third_party/gzip-1.2.4/COPYING` and README), and a root LICENSE for the project's code: the
   owner's intent is **AGPL-3.0**, or plain GPL where that can't work.  The third-party licenses above
   (zlib, MIT, public domain, Apache-2.0 with the LLVM exception, LGPL for dynamic glibc, GPLv2-or-later
   for gzip, which joins AGPL-3.0-or-later through GPL-3.0) are all compatible with it.
   What can't be relicensed by the project is what isn't its own: the decompiled SDK parts (3b/3c/3d,
   ultralib has no license either) and anything ROM-derived.
7. **The decompiled game and SDK C (3a/3b)** is the same material as the repository's source. It is the
   owner's decision. If libaudio had to go, a clean-room synthesizer and sequencer is several agent-hours and
   gives up exact output. gu is about 12 small functions.

## The engine rewrite

What it covers, hd_code us.v10, in MIPS bytes:

| Area | Objects |
|---|---|
| Vehicle parts, shared physics, math | 56040 vehicle parts (24.3 K, 101 functions); 62740 shared vehicle physics on `VehicleState` via `$gp` (21.0 K, 83); 679E0 math and matrices (3.6 K); 69014, 69944 |
| Per-vehicle modules | 69BB0 driver, 6B4A0 Sideswipe, 6C5E0 Magoo, 6E200 buggy/Skyfall/bulldozer, 71140 truck, 72B80 hotrod, 75490 carrier and crane, 772A0 train, 80280 J-Bomb, 83910 barges, 853D0 Ballista, 86F60 police, 88160 A-Team, 8AEE0 Starski, 8DDB0 chopper (~130 K together) |
| Buildings and destruction | 77E20 (23.5 K, 68) |
| Terrain, collision, textures | 5FD50 terrain quadtree walk and the visibility task; 60D50; 60F60; 89250 collision tests; 8A080 |
| Loaders | 5CB60 level and model loader (12.8 K); 5BF40 texture loader; 7F8B0 Rare's LZSS |
| Other | 8A2E0 communication point; front end 1B100 |

Area labels come from `vehicle.h`, `docs/TYPES.md` and `ROADMAP.md`; 69014, 69944, 86ED0 and 8A080 are
unidentified.

**Timing.**
- The engine never reads the retrace counts. It steps a fixed amount per game frame, with per-frame
  constants for vehicle physics, animation, the carrier's route and camera smoothing. Physics at 60 ticks
  therefore means re-deriving those constants in 62740, 56040, the vehicle modules, 77E20 and 5FD50, and
  the TAS no longer syncs in that mode.
- The game's pace depends on the engine's CPU cost. `RECOMP_COUNT` charges its MIPS instructions, and
  that decides how many retraces a frame takes. A rewrite needs a cost model, e.g. each function charged
  its original instruction counts, or the 30 fps pacing and the TAS drift.

**Effort:** 688 functions (220 K of MIPS).  The mechanism is `port/engine` (docs/PORT.md, "Replacing
the engine"): one function at a time, each charged its original blocks and checked call by call
against its translation.
- The 679E0 pilot (18 functions of math and matrices) took about 25 minutes, checks included.
- The pace since then: about 70 functions an hour for straight math, and 25–30 an hour for branchy
  physics with Rare's register conventions.  Most of the time goes into the registers the original
  leaves behind for its translated callers (`ENGINE_LEAVE`).
- That puts the whole engine at roughly 15–20 agent-hours.  The vehicle modules and the loaders are
  slower per function than the math, but each is checked the same way.
- Native now: 204 functions. That's all of 69014, 69944 and 56040 (101: the parts, their planes and
  headings, the animation splines and key frames, the sphere, edge and triangle tests, the collision
  walks over the buildings' pieces and the texture animations), 62740 but four and 679E0 but four.
  The last 21 took about 1.5 agent-hours, most of it in the collision walks' shared tests.
- Still translated in 62740: the recursive dispatchers into the vehicle modules (`func_802AB478` to
  `func_802AB714`), which call 17 of the vehicle modules' functions and save every register for
  them; they belong with those modules.  679E0's four are dead (`func_802ACDB8`, `ACEB8`) or go with
  71140 (`func_802AC284`, `AC2A4`).
- Native too: all of 77E20 (68 functions), 89250 (13), 8A2E0 (7) and the front end's 1B100 (6),
  94 functions in about 3 agent-hours with the checks and the TAS (2026-10-01).  77E20 is long,
  branchy code that charges its blocks through tables of display-list copies and saves most of
  its callers' registers, so it went at about 30 an hour; the block trace (`PORT_BLKLOG`, below)
  is what found the last differences, where the cost logs only showed a frame.
- The loaders, terrain and textures (engine-B's): native now are all of 7F8B0 (the LZSS, the
  loaders' gzip call, the engine sound), 5BF40 (the texture loader), 60D50 (the height boxes), 8A080
  (the collision objects' table) and 5FD50 (the visibility walk and the terrain's display lists),
  all of 60F60 (the texture decoders and their queue, the effects' sprite slots and their drawing)
  and 34 of 5CB60's 41 (the level tables, the collision triangles, the buildings, the vehicle
  records, the vehicle and cargo model loaders, the starting vehicle): 89 functions and 14 inlined
  into them, about 6,750 instructions.  Still translated: 5CB60's loader itself (`func_802A1674`),
  `func_802A19F4`, the carrier, chopper and extra models (`func_802A303C` to `func_802A3198`)
  and `func_802A350C`: they call the vehicle modules' inits (and the loader the game's C), which
  read and leave dozens of registers, so each wants its REGS() settled with the vehicles' owner.
- Native too: 13 of the vehicle modules (181 functions): 86ED0, 8DDB0, 772A0, 88160, 6B4A0, 86F60,
  83910, 853D0, 71140, 6C5E0, 75490, 72B80 and 8AEE0, in about 14 agent-hours including the
  infrastructure (`engine_save()`, `engine_ctx()`, `PORT_ICOUNT_LOG`, `PORT_BLKLOG`), so about 13 an
  hour.  Many modules come in twins: 8AEE0's hotrod is 72B80's with other numbers, its Cyclone Suit is
  Thunderfist with a third leg, and the police car is the A-Team van.  Those went at about 40 an hour,
  through block-by-block diffs of the two dumps.  The slow part is registers: whenever a callee goes
  native, the registers the glue used to write disappear (a native-to-native call leaves nothing in the
  context).  So each caller puts back what the original left for the translated code after it
  (`$s0`/`$s4`/`$s7` for `func_802A8768`'s and `func_8029C454`'s hidden reads, `func_802AA764`'s `$s2`).
- The rest of the vehicles (engine-E, 2026-10-01): 6E200 (Skyfall and Ramdozer, 28), 80280 (the
  J-Bomb and the level's Controller Pak status, 30), 69BB0 (the driver, 14), 62740's carrying (the four
  dispatchers and the vehicles' 18 callbacks for them, two templates in shared.h) and 5CB60's loader with
  its vehicle parts (7): 101 functions in about 5 agent-hours, each checked over the whole TAS by its
  icount log and its block trace (`PORT_BLKLOG`) against main's.  Every vehicle module is native now.
  The check build found one older slip: the Sideswipe's "can it be left" returned 0 where the original
  leaves the part number 5 in `$v0` (a path the TAS doesn't take).
- The chopper's C-cost drift (us.v10's TAS, read 5591, `__port_icount_c` 12 instructions low, a 7x8
  patch of pixels) was the dead N64 stack.  The driver's shadow (69BB0's `func_802AF340`), which hd.c
  runs right after the chopper at the same depth, passes `func_802582C4` three stack arguments it never
  stores; the shadow's pitch is whatever `func_802ABBEC` saved there for the chopper (its `$t3`, the
  model).  With the chopper native nothing wrote that slot, and the shadow tilted.  The fix keeps the
  original's frames where the read reaches them (`engine_frame()`, docs/PORT.md), and native code now
  calls translated code at the context's `$sp`, as Rare's `jal` does, not 32 bytes below it.  Both costs
  are now the same as main's over the whole TAS.
- After the rebase the same slot came out wrong at read 108453 (`__port_icount_c` 23 low, then the
  J-Bomb's sound a frame off at read 122932): there it holds the `$ra` that the carrying's
  `func_802AB714` saved, which the native carrying didn't write.  The native code on the paths to
  `func_802ABBEC` (the loader, the vehicles' setups, their steps each frame, put-backs and matrix
  functions, the carrying and its callbacks) now keeps the original's frames, and sets `$ra` for a
  call as the `jal` would (`ENGINE_RA`, from each block's address, which `tools/recomp` now writes).

- The last ones (engine-F, 2026-10-02): 8AEE0's three that nothing calls (the suit put back, its state
  saved, an `mtc0`) as native C; 679E0's `func_802ACDB8` and `func_802ACEB8`, which nothing reaches in
  any version (no call, no address in code or data), marked `unused` in replaced.txt instead; and jp's 18
  (its IDO `GLOBAL_ASM` functions, 9,500 instructions: the menus' window renderer, the text renderer,
  the HUD's counters, the medal and promotion screens, the Pak's menus; and `func_802BA3E8_jp`).  jp has
  no TAS, so the check build learned to run the IDO functions' C callees twice (`check_repeatable.txt`)
  and a fuzz (`PORT_ENGINE_FUZZ`) calls them on varied state: every block but `break`s and unreachable
  sign fixups, 0 differences.  About 3 hours of wall clock (7 agent-hours, three helpers in parallel).
- **No translated code in the default build**: the `recomp` library is `entry.c` alone (empty), so a
  function missing from `port/engine` fails the link; `test.py quick`'s "engine" check fails any build
  but the check build that compiles or links translated code (`rom_scan.py` still finds none of the
  ROM's data in the movable builds).

**What remains for publishing** (the list above): stripping and `PORT_N64_FUNCS` (item 5), the notices
and the root LICENSE (6), and the owner's decision on the decompiled game and SDK C (7); with only the
movable builds published (1), the non-movable builds' data (2) doesn't matter.  A published build still
reads the ROM's data from the user's ROM at run time, or from a resource pack the user made from it
(docs/PORT.md, "Resource packs"), which may leave out the game's code: the port needs none of it.
What it still takes from the user is the assets and the code modules' data (`data/`, 207 K in us.v10:
the game's tables, strings and display lists, the RSP's data); replacing those is what a fully free
game would need besides new assets.

**Checking it:** function by function with the unicorn difftest (`tools/recomp/test/difftest.py`),
which is how the translation was checked, and as a whole with the TAS suite (`port/tools/test.py`).

**Side note: path-traced lighting** (desktop only; looked into 2026-09-30, parked until after a full
rewrite).
- **What the renderer has to work with:**
  - Capturing the scene is easy: the camera is its own matrix, and terrain and buildings arrive in world
    coordinates. The game sends the whole level every frame (about 4.5 K triangles, 95% off screen; it
    relies on `G_CULLDL`).
  - The lighting is the hard part. It is almost all painted into the textures or given as flat vertex
    tints: 64% of world triangles are white, there is one fixed light used by about 9 triangles a
    frame, and no fog. Vehicle shadows are top-down silhouettes drawn as ground decals.
  - Ray-traced light added on top would be counted twice.
  - Unlit replacement textures would be ROM-derived, so they couldn't be shipped.
- **Possible now, but not worth it yet:** hooking `gfx.c`'s `do_mtx`/`do_vtx` and adding shadows, AO
  and a bounce over the GL rasterizer. That is about 6–12 agent-hours, more for full path tracing.
- **Worth doing with a rewritten engine,** whose renderer gets a scene API: instances, material IDs and
  emitters such as explosions.
- **First step then:** a debug dump of one frame as glTF, rendered offline (e.g. Blender Cycles) to
  check that it looks better and not just muddier (about an agent-hour; a scene API in the rewritten
  renderer, 3–6 more).
- **Hardware:** OpenGL 4.3 compute with our own BVH is enough at this size; Vulkan ray queries are the
  alternative. Denoising: NRD or SVGF.

## Replacing the SDK parts

The goal is for a published port to contain none of Nintendo's SDK. The N64 build keeps
`src/libultra` and `include/2.0I` as they are, because it must match the ROM. The port instead builds
clean replacements from `port/`, over the same interface: the same names and signatures, and the same
struct sizes and offsets wherever the game's C, the ROM's data or the handwritten code rely on them.
This scoping was done on 2026-09-30, with sizes from the us.v11 32-bit build's x86 `.text`.

**What reaches the port:**
- libaudio's 40-odd files, and its header (`2.0I`'s libaudio.h).

(gu, sinf/fcos, sins/coss, `gu_extra.c` with `mtxcatl.c` and `io/crc.c`, `ll.c` and the other `2.0I`
headers reached it until 2026-10-01; `port/src/gu.c` and `port/include/sdk/` replace them, below.)

libultra's OS/IO part and its printf are already the port's own (`port/src/ultra.c`, `libc.c`). Rare's
handwritten engine calls no gu or libaudio function, only six OS functions, which `port/src` already
replaces. So the interface to keep is the one the game's C uses.

| Piece | Size | What the game uses | Approach | Exactness and check | Effort |
|---|---|---|---|---|---|
| `ll.c` (`__ll_*`, `__ull_*`) | 0.7 K | nothing in the port (clang does 64-bit natively) | drop it; `alCSPGetState` comes with the new libaudio | — | minutes |
| `io/crc.c` | 0.6 K | `__osContDataCrc`, the save checksum (E7B0.c) | from the public joybus/pak description | exact: random inputs against the original, and the save hashes | ¼ agent-hour |
| `sins`/`coss`, `sintable` | 0.2 K + 2 K | 1C40.c, 6790.c, 20460.c | generate the table: `(int)(32767*sin(i*pi/2046))` gives all 1,024 entries | exhaustive over 65,536 inputs | ¼ agent-hour |
| `sinf`/`fcos` | 2.3 K | direct calls, and inside guRotateF, guLookAt*, guPerspective | range reduction plus an odd polynomial. Only SGI's constants are exact: a correctly rounded sinf differs on about 0.9% of inputs | exhaustive over all 2^32 floats against the original (in a test tool only) | ½ agent-hour, plus a decision on the constants |
| gu matrices (F2L/L2F, Ident, Translate, Scale, Rotate(F), Ortho, Perspective, LookAt(Reflect), MtxCatF/L, XFMF/L, Normalize) | ~7 K | about 20 files; the order of float operations and `FTOFIX32`'s rounding matter | from the function reference's formulas, computed in the same order and precision | bit for bit on random inputs and on arguments traced from the suite's runs; then the quick tier | about 1 agent-hour |
| Headers (types; os.h's `OS*` structs and constants; rcp.h, sptask.h, abi.h, gu.h, mbi.h; libaudio.h's public structs) | layouts only | struct layouts, the `.ctl`/`.seq` formats, the port's `PTR32`/`_GBI_W` edits | `port/include/sdk/`, written from the documentation and what the code needs, ahead of `2.0I` | exact by construction: every game object's `.text`/`.data`/`.rodata` is byte-identical with either header set, plus `layout_cmp.py` | 1–2 agent-hours |
| gbi.h (F3D: 64 macros, 95 `G_` constants) | macros only | display-list encoding | from the public command formats (n64brew), as facts only. Not GLideN64's (GPL-2.0-only) and not libultraship's gbi.h (the SDK's, with the notices removed) | the same object-identity check; the renderer's decoder is already the port's own | in the row above |
| libaudio | 44 K (≈2 K dead: `seq.c`, `alSeqpNew`) | below | from the reference manual and the observed behaviour, with a differential oracle | below | **done** (2026-10-01): `port/libaudio`, exact (docs/PORT.md, "libaudio"); about 1.2 agent-hours, the oracle included |

**Done (2026-10-01): the first five rows.**  `port/src/gu.c` has the port's own gu, sinf/fcos,
sins/coss and `__osContDataCrc`; the stubs that built the SDK's (hd_code 90760, 90800, 909C0, 90E40,
91050, 912C0, 91360, 91830, 92130, 950E0, 95070, 97140) and `gu_extra.c` are out of the port, and so
is `ll.c` (`#ifndef TARGET_PC` in 90390.c, whose other half is libaudio's).
- Each function computes in the original's precision and order (the look-at's reciprocals in double,
  the perspective's degrees in double, the rotation's in float, guMtxCatF's sums from 0), and writes
  game memory at the original's widths (the fixed-point matrix a word at a time, as the native-endian
  builds need).
- It also runs exactly as many loop back edges: BEPass puts a poll on each, and every 64 polls take 2
  µs of virtual time, so a loop the original didn't have moved the quick tier's hashes (the look-at
  functions are straight-line for that reason).
- `port/tools/sdk_check/sdk_check.py` checks it against the originals, which only it builds (from
  `src/libultra`, and `orig_rotate.c` for guRotate(F)), for i386 and for LP64's `long`: sinf and fcos
  over all 2^32 floats, sins and coss over all 65,536 angles, every function on 1,000,000 random
  argument sets, and the 2.6 million calls a `-DPORT_SDK_TRACE=ON` build logged in `PORT_AUTOSTART=3`
  and the attract mode's 12,000 frames.  All identical; a NaN result may differ in sign or payload
  (the compiler's choice of operand order), which only random NaN arguments reach.
- Not a strict clean room: the same agent read the originals for the order of operations.

**Done (2026-10-01): the headers and gbi.h.**  `port/include/sdk/` has `ultra64.h` and the `PR/`
headers the game's C and `port/src` use: the types, os.h's structs and constants and the functions'
prototypes, rcp.h's and R4300.h's registers and segments, sptask.h, mbi.h, abi.h's audio commands, gu.h,
gbi.h's Fast3D commands (from the documented command formats: the opcodes, each command's fields, the
other-mode, combiner and blender encodings, the structs the commands point at), os_internal.h,
string.h and stdarg.h.  Only what is used is there (gbi.h is 600 lines where 2.0I's is 4,400).  The
port builds with them instead of `2.0I`, all but libaudio.h, which stays `2.0I`'s (through a generated
include) until the port's own libaudio.h is in `port/include/sdk/PR/` (it is, since the port's own libaudio).
- Where it matters for the code, the macros keep the C types of the SDK's forms: a field is an
  `unsigned int` (`_SHIFTL`), the RSP's immediate opcodes are negative `int`s, `FTOFIX32` converts
  through `long`, and every function the game calls has its prototype (an implicit declaration would
  pass floats as doubles).
- `port/tools/sdk_identity.py` builds the N64 side with either header set (`-DPORT_SDK_2_0I=ON` puts
  `2.0I` back) and compares every object: on us.v10, 112 objects in each of 32, 64, n64 and lp64
  (stripped of debug information: sections, symbols and relocations) and mn32 (LLVM bitcode, its IR
  without debug information) are the same, and with `--link` so are the struct layouts of the linked
  32 and lp64 programs (`layout_cmp.py`'s, counting the members both sides have: 2.0I's `Gfx` and
  `Acmd` unions have more alternatives).

**libaudio's interface:**
- The calls made by `1A630.c` (Rare's sound player), `1C460.c` (music), `22EE0.c` (audio manager),
  `17E10.c` and `1D990.c`.
- The sizes and offsets of the structs the game embeds, holds or saves: `ALLink`, `ALVoice` (inside
  `SndState` at 0x0C), `ALPlayer`, `ALEvent`/`ALEventQueue`, `ALCSeq`, `ALCSeqMarker`, `ALCSPlayer`.
- The ROM's bank and sequence file formats, with `alBnkfNew`'s relocation.
- `PTR32` on every pointer field.
- The game uses custom reverb parameters, so `initfx.h`'s presets aren't needed.

**Exact libaudio means:**
- the same Acmd list every audio frame;
- the same `alCSPGetState`/`alCSeqGetLoc` answers at every call. The music manager and the results
  screens wait on these, and `--replay` answers them from the movie's log;
- the same voice-allocation results.

The oracle is a shim that logs every libaudio call the game makes. A harness replays that log into the
original (built only in a test tool) and into the replacement, and compares each frame's Acmd list and
the answers, as the engine's difftest does. `test.py variants --tas` then checks the whole.

**Order inside libaudio:** the heap, links and events; the bank and sequence files; the synth driver and
voice allocation; the physical-voice chain (ADPCM load, resample, envelope and mix, buses, save), where
the envelopes' float ramps are the hard part; the custom reverb; the cseq parser; the compact sequence
player (MIDI state, voice mapping, tempo, markers, loops).

**If exactness were given up** (e.g. a synthesizer of the host's own):
- The wav hashes change.
- The quick tier's saves and screenshots may change too, where screens wait on the music.
- The TAS should still sync, since the replay answers the two audio queries from the log, provided voice
  allocation doesn't reach level logic (not verified).
- The references would be recorded again.

**Timing.** libaudio is N64-side C, so ICount charges the replacement's own instructions. That leaves
the quick tier alone (`PORT_COUNT_PER_OP=0`) and moves only the TAS's "retraces given anyway", which
the suite doesn't check.

**Third-party code checked** (2026-09-30):
- Nothing libaudio-compatible and open exists.
- libdragon (Unlicense) has its own mixer and no GBI, and its `fm_sinf` isn't exact.
- N64ModernRuntime (GPL-3.0, compatible) has OS glue only.
- libultraship's (MIT) `gbi.h` and `gu.h` are the SDK's headers.
- GLideN64 is GPL-2.0-only, which is incompatible with AGPL-3.0.
- mupen64plus-rsp-hle is GPLv2-or-later.
- ultralib and libreultra have no license.

Borrow none of them for these parts.

**Process.** Anyone who has read `src/libultra` or `2.0I` is "dirty". For a strict clean room, the dirty
side writes the specification: this interface, the layouts and the behaviours. Someone who hasn't read
them implements it from the specification, the public manuals and the oracle's pass/fail results.
Whether numeric constants such as sinf's may be reused is for the owner or counsel to decide.

**Order of work:**
1. `ll.c`, crc, sintable, gu and sinf: **done**, exact (about 1.5 agent-hours, the checks included).
2. The headers and gbi.h: **done**, exact by object identity (about half an agent-hour).
3. libaudio's oracle, then libaudio from the bottom up: 6–12 agent-hours. It could be scheduled alongside the
   engine rewrite, which blocks publishing anyway and is far larger.

`2C560.c` (the scheduler), `22EE0.c` and `1A630.c` are Rare's adaptations of the SDK's sample code.
They are part of the game's C (3a), not of this list.
