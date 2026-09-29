# Roadmap

The end goal is a PC port that builds from this source. A matching decomp is
the route there, not the goal in itself: the sha1 checks are what tell us each C
function is right. The port then needs three more things: C that doesn't depend
on addresses, data with real types, and a platform layer to stand in for
libultra and the RCP.

`us.v11` is the reference version. The other three build from the same C
(`#if`'d where they differ, see CLAUDE.md "Versions"); new work lands in
`us.v11` first and then has to match the others too.

## Where things stand

| module         | IDO-compiled code | handwritten asm | us.v11 (2026-09-28) | us.v10 | jp     | eu     |
| ---            | ---:              | ---:            | ---:                | ---:   | ---:   | ---:   |
| `init`         | ~10 KB            | ~4 KB           | 100%                | 100%   | 100%   | 100%   |
| `hd_code`      | ~420 KB           | ~220 KB         | 99.9% (2 left)      | 99.9%  | 94.0%  | 73.2%  |
| `hd_front_end` | ~130 KB           | ~2 KB           | 100%                | 100%   | 92.5%  | 29.8%  |

Run `tools/progress.py --version <v>` (after building that version) for
current numbers. Its percentages are of the IDO-compiled code only. What
isn't C in jp and eu is the functions they have differently (jp's Japanese
text, eu's three languages and PAL timings), as `GLOBAL_ASM` for that
version, and eu's objects with multilingual data, all asm there.

What's known that affects the port:

- **Graphics use SGI's Fast3D, not F3DEX.** The string is
  `RSP SW Version: 2.0D, 04-01-96`. That's the GBI family SM64 uses, so
  existing Fast3D-to-OpenGL/Vulkan translators (sm64-port's `gfx_pc`,
  libultraship's Fast3D) are a realistic starting point.
- **Audio uses libultra's `libaudio`** (`alAudioFrame`, `AL_MAX_RSP_SAMPLES`)
  with the stock audio microcode. The PC side needs a C implementation of that
  microcode (HLE, as the Perfect Dark port does) plus the decompiled `libaudio`.
- **The scheduler is SGI's sample `sched.c`** (the `sc->curRSPTask` asserts).
- **Compiler:** IDO 5.3, with `-O1` for the code seen so far (every local is
  spilled to the stack). Per-file flags still have to be confirmed.
- **About a third of hd_code is handwritten asm:** everything from `0x56040`
  to the libultra at `0x8F860`, around 220 KB and 500 functions, including
  the code around the data islands. It's Rare's own: every function saves
  `$ra` and everything else it touches with `sd`, arguments and results pass
  in `$v0`/`$v1`/`$t*`/`$s0`, it uses `$gp`, and it does 64-bit math
  (`dmult`, `dadd`). `gen_code_yaml.py` detects it (sd `$ra`, never sw) and
  emits it as `asm`. It can't be decompiled to matching C; for the port it has
  to be rewritten or mechanically translated (see Phase 5). The game's
  simulation (vehicles, collision, destruction) is likely in it.
- **`.data`, `.rodata` and `.bss` are per object**, every C file defines its
  own (typed: structs, `Vtx`, display lists as GBI macros), and every address
  is a symbol: the build can move (`SHIFT=1`, tried with us.v11). The other
  versions' layouts are us.v11's moved through `tools/vermap.py`.

## Phase 0: workflow

Tooling that makes decompiling one function cheap. Do this before large-scale
decompilation.

- [x] `tools/progress.py`: C vs `GLOBAL_ASM` bytes per module and version.
- [x] `tools/fdiff.py`: diff one function between the original and rebuilt
      module (asm-differ without its dependencies).
- [x] Objects depend on their headers and `GLOBAL_ASM` files.
- [x] `diff_settings.py`: `--module`/`--version`, for asm-differ.
- [x] mips_to_c bumped to current m2c. It needs `pycparser<3`.
- [x] `tools/m2c.sh <module> <function>`.
- [x] Per-file flag overrides in `blastcorps/Makefile` (`ll.c` is `-mips3`).
- [x] `tools/sync_stub_names.py`: keep stubs in step with new names.
- [ ] m2c context from `include/`, so its output uses real types.
- [x] decomp-permuter wiring (`tools/permute.sh`; see `docs/DECOMPILING.md`).
- [ ] `common.h`/`include/`: the base types, the `GLOBAL_ASM` macro, and
      libultra headers from the matching SDK version.

## Phase 1: data, bss, and a shiftable build

A PC port needs every address to be symbolic. The same work lets the N64 build
change size (a "shiftable" build), which is how we test it.

- [x] `init`: `.data`/`.rodata` split per object. Each C file defines its
      own data (the gzip tables, `osClockRate`, the thread queues); the asm
      files' data are splat `data`/`rodata` segments. The layout, one 16-byte
      aligned block per object in link order, is in `init.us.v11.yaml`.
- [x] `init` `.bss` (`0x802229E0`-`0x80224D80`, cleared by the entry
      point): gzip's two blocks, the boot code's 0x2000-byte stack and two
      words, then initialize.c, kdebugserver.c and seteventmesg.c. The C
      files define theirs; the boot stack is an asm `.bss.s`. IDO turned out
      to emit uninitialized globals into `.bss`, not COMMON, so no linker
      tricks were needed (see DECOMPILING.md for its ordering rules).
- [x] `hd_code` us.v11: `.data`/`.rodata` split per object, generated by
      `gen_code_yaml.py --rodata/--tail` (see `split_data` there). Layout
      notes below.
- [x] Every hd_code C file owns its `.rodata`. The files this list used to
      name (168B0, 17E10, 20460, 26570, 30C70, 4EBE0, 53220, 91830, 92130,
      95C40, 983F0, 9E700, 9CC30) were done in the data-owning and straggler
      passes; the two "two objects each" were split by then too
      (`ghostdigger.c` is 50670, gzip's inflate 53820, from `huft_build`).
- [x] Every C file owns its `.data` (31 more in hd_code, all 10 with `.data`
      in hd_front_end), written by `tools/data_c.py` from the asm blocks with
      the types the C declares (structs, `Vtx`, `Lights2`, `Gfx` as gs*
      macros, strings, pointers as `&table[i]`). What's still asm `data` is
      handwritten objects' and data-only objects': hd_code A44D0 and BC8E0
      (0x0FFE-terminated u16 text, reached from 14B30/26570, from 26570's
      menu tables, 23C20, 53220, 00000 and hd_front_end), AFF00, C1CC0 (display
      lists and data for the handwritten 77E20/5CB60), BAF70 (display lists
      and vertices 39050 uses), C26B0 (2C560 reads one word), C3030 (the VI
      mode tables), `.rodata` C9650; hd_front_end 25070 (the menu object) and
      its `.rodata` 28480. They could become data-only C files.
      One block moved: hd_front_end 9570's `.data` starts at `0x214F0`
      (`--data-split 214F0:9570`), because the texture there runs past the
      old start. Display lists in `.data` hold physical addresses of vertices
      in `.data` (4B5E0, 4DA80); `STATIC_K0_TO_PHYS()` makes them relocations,
      so they now move in a shifted build.
- [x] `hd_front_end` us.v11: split the same way (`.rodata` from
      `0x8020E440`, then the 0x800-byte DMEM image of its microcode at
      `0x80210690`). pfsHandler.c / bestTimes.c are split at `0x10850`
      (`--split`), with a data-only menu object between them (`.data`
      `0x8020C070`, `.rodata` `0x8020F480`). 9570.c is two files, split at `0xC450`
      (`missed_boundaries()` finds it; 9570's `.rodata` ends in padding at
      `0x8020F088`). Every C file owns its `.rodata` and `.data`.
- [x] `hd_code` and `hd_front_end` for us.v10/jp/eu: same thing, from
      us.v11's configs (`gen_code_yaml.py --like`, CLAUDE.md "Versions").
      Each C file owns its data there too; jp's and eu's differences are
      `#if`'d in the definitions (jp's entry tables lack some entries, eu's
      credits have three more lines, its audio heap is smaller, ...), and
      eu's objects with text in three languages are asm there.
- [x] `hd_code` `.bss` (`0x8030F660`-`0x803FF600`, 77 objects) and
      `hd_front_end`'s (`0x80210E90`-`0x8021AC30`, 16), laid out by
      `gen_code_yaml.py --bss` like `.data`. The boot code clears from the end
      of hd_code's `.data` to `0x80400000`; `0x803FF600` is where the heap
      starts (`D_803FF600`, now the end-of-`.bss` symbol), and the last two
      words below `0x80400000` are init's hand-over of hd_front_end's ROM
      range, not `.bss`. The big buffers in it (00000's 0x55A50 bytes, 60F60's
      and 46F60's ~0x28000) are ordinary variables, addressed by symbol.
      Every C file defines its `.bss` (`tools/bss_c.py`); the 27 handwritten
      objects have `.bss.s` files.

### hd_code us.v11 data layout

What `split_data` in `tools/gen_code_yaml.py` assumes, and why:

- Same as init: all objects' `.data` in link (= `.text`) order from
  `0x802E8BD0`, then all objects' `.rodata` from `0x80307B90`, each block
  16-aligned. After the last `.rodata` (9FE20's, ending `0x8030E390`) come
  three RSP microcode DMEM images (`0x800`, `0x2D0`, `0x800` bytes), kept as
  `bin`. The asm objects (Rare's engine, libultra's `.s`) sit in the same
  order: their data is interleaved with the C objects', not separate.
- Within one object's `.rodata`: first its strings and other early data in
  source order, then its "late" rodata (jump tables, float and double
  literals) in function order. That is what asm-processor's `.rdata` /
  `.late_rodata` model, and it's why the last jump table or constant of an
  object marks its end exactly. A string after a late constant inside one
  subsegment's range means a new object: that is how `ghostdigger.c`
  (50670, after 4EBE0) and gzip's inflate (53820, after 53220's dialogue
  tables) were found, `.text` boundaries splat missed.
- Why "the data only one object references, ordered by object" is not
  monotonic: objects reach into each other's globals (the 0x44-byte records
  at `D_802E8F94` are 1D990's but 41930 uses one field; exceptasm touches
  thread.c's `__osRunningThread`); a u64's halves show up separately; tables
  and strings are reached through pointers in `.data` rather than code (the
  menu strings at `0x803099D0` are 26570's, reached only from its menu
  structs at `D_802F5804`; 53220's dialogue table points at 0x7A0 bytes of
  strings); scheduling splits `lui`/`addiu` pairs, which splat leaves as bare
  numbers (hence `undefined_syms`); and there are data-only objects with no
  `.text` (libultra's VI mode tables at `0x803077F0`/`0x80307840`, libm's
  NaN at `0x8030DE10`, `__osRcpImTable`-style tables). The generator takes
  the longest link-ordered chain of single-user references, then anchors the
  remaining objects on shared references, `lui` pairs and `.data` pointers
  that fall between their neighbours, and gives leftovers their own
  subsegment (named by offset).
- `.data` block starts are the weakest part: an object's first global may be
  one nobody references (45BB0's block may really start in 43A60's float
  table). `.rodata` starts are solid wherever the previous object ends in a
  late constant. With the data typed, a variable that would run across a
  block start shows a wrong one: hd_front_end's 7800/9570 boundary was
  `0x21FA0`, inside a texture that a display list of 9570's loads whole, and
  `--data-split <off>:<object>` moves a block. In the other blocks every
  variable's users were checked; those used only by another object (168B0's
  `D_802E8C8C`, 43A60's `D_802FDAC0`) sit where 16-byte block alignment
  leaves no other owner.
- Some libultra C subsegments hold several objects (97BB0 is thread.c,
  piacs.c and getthreadpri.c); their data blocks follow the same order.
- Splitting needed fixes to splat's data output, done by `tools/split.py`
  (the Makefile's extract runs it): `dlabel` instead of the aligning
  `glabel`, byte-exact string escapes, `1e8` floats, pointer words across
  subsegments, and rodata migration into GLOBAL_ASM files for C-owned
  `.rodata`. No bare pointers are left (`tools/find_abs.py`).
- [x] Make the ROM offsets that code uses to find assets and the compressed
      modules into linker symbols. init's boot code takes hd_code's and
      hd_front_end's ROM range from `tools/rom_syms.py` (placed by the
      compressed sizes), and every ROM address the code uses (99 in hd_code,
      14 in hd_front_end) is a segment's `_ROM_START`. The ones that used to
      be left as numbers are named by the top-level split now (Phase 4): the
      sound banks and sequences (hd_code `0x350950`, `0x3539A0`, `0x3A1920`,
      `0x3A48C0`, `0x44F5C0`), the background images (`0x6AD3F0`,
      `0x6BF2F0`, `0x6D3D30`), the model table (`0x6EC4C0`, also reached by
      hd_code 5CB60's handwritten loader) and static_data (`0x787F40`, with
      `0x788000` its `_DMA_END`); and the texture table at `0x4CE0`, which
      5BF40's handwritten loaders build with `lui 0; addiu` (postsplit
      symbolizes it). The link reports list no "ROM offset" left.
- [x] Shiftability test: `SHIFT=1` (CLAUDE.md) pads the start of
      hd_code's and hd_front_end's `.text` and `.data`. In mupen64plus
      (rice video plugin, SDL's offscreen driver, a scripted input plugin)
      the shifted ROM's screenshots match the original's pixel for pixel up
      to frame 900 (the emulator itself isn't reproducible after that), and
      with Start/A pressed it goes through the name entry (hd_front_end),
      the Simian Acres briefing, gameplay and the pause menus like the
      original, with no core errors. Before the pointer-bearing data islands
      and pointers inside `.byte` records were symbolized it crashed within
      600 frames. After the `.data` pass (display lists' physical addresses
      now relocate) the shifted ROM still matches to frame 600 and plays the
      attract mode to frame 1500 with no input; the front end wasn't
      re-driven.

## Phase 2: decompile the code

Matching C for every non-handwritten function, in this order:

1. **`init`.** Done. The rest of `init` is handwritten and split out as
   `asm`: the entry point, the boot code at `0x1A30`, and libultra's `.s`
   files.
2. **libultra everywhere.** Most functions match ultralib's source directly.
   The ones that don't (`lib:fuzzy`) are an older revision and get their own
   copies. Share one source file across modules where the code is identical.
   In us.v11 only guRotateF and guRotate (hd_code/90C50.c) are left; one
   `lui` is out of place (see `docs/DECOMPILING.md`, "The permuter").
3. **`hd_code`**, the game engine. Work in file-sized units (one file per
   splat split), leaves first. Generate context with m2c and grind the
   stragglers with the permuter.
4. **`hd_front_end`**, the menus and front end.

5. **The other versions**, from the same C. Done for everything that
   matches: us.v10 is us.v11's C with a handful of `#if`s (00000's and
   10850's v1.1 fixes); jp's 19 functions with Japanese text handling and
   eu's ~40 changed functions are that version's `GLOBAL_ASM`, and eu's
   hd_code 1C460, 1D990, 26570, 30C70, 45BB0, 53220 and hd_front_end 00000,
   1C40, 6790, 7800, E7B0, 196F0, 1A240, 11530 are all asm there: their
   `.data`/`.rodata` hold eu's text in three languages (German strings, and
   tables of three text pointers where the US versions have one), which
   needs writing as eu's own data before their C can build eu.

Rules for the C, so it ports cleanly later:

- No hardcoded addresses, not even in casts. Use `extern`s and linker symbols.
- Structs over pointer-plus-offset as soon as the layout is known. m2c's
  `*(s32 *)(p + 0x1C)` output is a first draft, not the finished C.
- Keep matching hacks (`do {} while (0)`, register pinning) behind
  `#ifdef` only when the port would otherwise be affected.

## Phase 3: types, names, headers

Mostly done alongside Phase 2, but it's its own effort.

- [x] Shared headers (`blastcorps/include/game/`, conventions in
      `docs/DECOMPILING.md`, "Shared types"): one definition for each
      structure more than one file uses, with a size check. us.v11's C files
      had 266 struct typedefs, 61 of them defined again in other files (the
      player record 22 times, the level table 15); now 84 are left, each
      used by one file, and the shared ones are in the headers: the player
      record and save layout (`player.h`), the level table, level file header,
      RDUs and level results (`level.h`), the vehicles (`vehicle.h`), the
      window system (`yoshi.h`), the scheduler (`sched.h`), the audio
      manager and sound player (`audio.h`), the per-frame buffer (`frame.h`),
      the camera (`camera.h`), and the game state most files share
      (`game.h`). The scheduler, the level results and the best times are
      typed in `.bss` too, where they had been separate variables or byte
      arrays, and the per-frame buffer's byte offsets are fields.
- [ ] Structs for the level's objects at run time (buildings, the carrier,
      the vehicles' state beyond position and type), the camera's state
      (still separate variables in hd.c), and the rest of the front end.
      `docs/blast_corps_levels.txt` and `docs/blast_corps_vehicles.txt` are
      the starting points; most of this code is in the handwritten engine.
- [x] Name functions from evidence only (strings, asserts, known source, call
      graph), and write down the evidence in `symbols_known.txt`, as for gzip.
      The scheduler's functions are named after SGI's sched.c; gen_symbols.py
      now also gives other modules the hand-identified names they call.
- [x] Mark every field that holds a pointer or a ROM offset in data loaded from
      ROM (`ROMPTR`, `AssetOffset`, `RomAddr` in `types.h`): so far the level
      header's offsets and the sound bank's pointers. The port has to widen
      or rebase exactly those fields; more will come with Phase 4's assets.
- [ ] Rename the `D_` symbols the headers identify (`playerNumber`, `levelno`,
      `saveIt`, `frontEndPresent`, ...). They keep their address names for
      now; the port refers to some of them by name.

## Phase 4: assets

docs/ASSETS.md has the inventory and the formats.

- [x] Name every piece of the ROM in the top-level config (still generated,
      `tools/gen_build_yaml.py` over `tools/assetlib/romlayout.py`, which
      finds each boundary from the data): the texture table and textures,
      the two sound banks (LZSS `.ctl`, `.tbl`) and the sequence bank, the
      738 gzip members by their header names, the LZSS background images and
      static_data, the model table.
- [x] Extract them into editable files with a byte-exact rebuild, checked
      by `make extract` for all four versions (`tools/assets.py`): textures
      as PNGs, levels as YAML following `LevelHeader` (records where the
      layout is known, hex elsewhere), the sound banks and sequences as
      libultra's `.ctl`/`.tbl`/`.seq`, the background images as PNGs, the
      other gzip members inflated. The ROM link builds from them and
      regenerates the texture and model tables.
- [x] Rare's LZSS identified (Nelson's, BREAK_EVEN 2) and reproduced; gzip
      1.2.4 reproduces every member.
- [x] Shifted assets: `SHIFT=1` also puts `ASSET_SHIFT_PAD` (0x10) bytes
      before the texture table and every 16-aligned asset (92 places), so
      every asset moves. In mupen64plus (rice, the scripted input of
      port/tools/m64p_pace.c "play", screenshots every 300 polls to 3000),
      the shifted ROM reaches Simian Acres like the original: the same mode
      sequence, every screenshot the original reproduces is identical, and
      the audio is sample-identical for the first 13.1 s, where two runs of
      the original also part. An edited texture PNG (recompressed by our
      encoder, moving everything after it by 0xD90 bytes) shows in the game.
- [ ] The blast encoder can't reproduce Rare's streams (the compression is
      lossy and Rare matched on the unquantized source), so the build keeps
      the ROM's stream for each texture and only recompresses changed ones.
- [ ] Which LUT each type 4/5 texture uses (set up by the loader, not in the
      table), and the image sizes of the ~170 textures no display list or
      old table gives.
- [ ] Types for what's still binary: the models and their display lists
      (and the levels' `_dl`), the level sections left as hex, the title and
      texture-set images.
- Describe level/model data as typed records rather than blobs. The PC build
  reads the same assets but byteswaps them and widens their pointers at load
  time.

## Phase 5: the PC port

A second build target (host compiler, CMake or meson) over the same `src/`,
selected with `TARGET_PC`. The N64 build stays matching.

- **Platform layer** in place of libultra: threads and messages (on a single
  thread, as sm64-port does), PI DMA reads from the ROM file or extracted
  assets, VI swap to the window, SDL controllers, EEPROM to a save file,
  `osGetTime` from the host clock. Look at libultraship before writing this
  from scratch.
- **Graphics:** feed the Fast3D display lists to a Fast3D renderer. Look for
  Rare-specific tricks: direct RDP commands, framebuffer reads, CPU-drawn
  textures.
- **Audio:** decompiled `libaudio` plus a C version of the audio microcode.
- **64-bit cleanliness:** replace `K0_TO_PHYS`/TLB/`osVirtualToPhysical` use,
  make `u32` pointer fields in structs from ROM data into real pointers, and
  convert on load.
- **Handwritten asm (hd_code's 220 KB engine block, plus init's boot code):**
  the biggest port task. Two routes, which can be mixed per function:
  - Mechanical translation: a tool that turns each asm function into C
    operating on a register file, as N64Recomp does for whole games. It's
    correct by construction and quick to get running, but the output is
    unreadable and ties the port to 32-bit N64 addresses in data.
  - Hand-written C equivalents, checked on the N64 build against the asm
    (same results for the same inputs, e.g. by running both in an emulator
    or a MIPS interpreter over recorded inputs).
  A sensible order: translate mechanically first to get the port running,
  then replace functions with readable C as they're understood.
- [x] Mechanical translator for Rare's engine block and hd_front_end's
      handwritten run (`tools/recomp/`, `make -C tools/recomp`): one C file
      per object over a register-file context and an emulated RDRAM arena,
      symbols kept. A differential test runs every function against the
      original in unicorn, on random states and on RDRAM snapshots from
      real play. All 688 functions match. See docs/PORT.md for the
      interface, results and what's left.
- [x] First PC build (`port/`, docs/PORT.md): CMake, 32-bit x86 with RDRAM
      at `0x80000000` and every game variable at its N64 address, the game's
      C built big-endian by an LLVM pass (BEPass), libultra replaced
      (fibers, PI from the ROM file, SDL input, EEPROM file, host clock),
      generated glue to the translated engine, a software Fast3D renderer.
      Runs the Rare logo, the title screen and the attract mode, and with
      Start/A the name entry, the world map and Simian Acres.
- [ ] Pacing: the RSP/RDP finish instantly, so the intro runs too fast and
      gameplay at 60 fps (docs/PORT.md, "Status").
- [ ] Audio (audio-ucode HLE, SDL output) and an accelerated renderer.
- Later: widescreen, higher framerate (the game loop is tied to VI retrace),
  and mod support.

## Milestones

1. `init` fully in C, all four versions still matching.
2. Data split and a shiftable ROM that boots.
3. libultra fully in C.
4. 50% of `hd_code` by bytes.
5. 100% matching.
6. The PC build reaches the title screen. (Done: docs/PORT.md.)
7. The PC build is playable start to finish.

A partial port can start before 100%: once data is symbolic, a PC build could
run the decompiled C and statically recompile what's left (N64Recomp-style).
Whether that's worth it depends on where Phase 2 stands then.
