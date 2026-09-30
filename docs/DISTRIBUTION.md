# Distributing the port

For now the PC port is **built by its users** from their own ROM (README, "The PC port"): no binary or
hosted page is published. This file records what a built port contains, so that publishing later can be
planned. It is an inventory by origin, not legal advice. Measured on us.v10 builds (32-bit, LP64,
movable 64-bit, wasm web) on 2026-09-30; us.v11's sizes are near-identical.

## What a built port contains

**Not embedded:**
- The ROM file itself: 3,000 random 32-byte chunks of it and its IPL3 were searched for in the
  executables, the .wasm and the .js, with no hits.
- The TAS data (`polls.csv`, `checkpoints.csv`, `switches.csv`...), which `--replay` reads beside its file
  at run time.
- `test_refs.json` and the mupen64plus tools.
- Base64 or other blobs in the page. `?rom=` only fetches the URL the user gives.

| # | Origin | What | Size (us.v10) | Where it enters |
|---|---|---|---|---|
| 1a | ROM bytes | hd_code and hd_front_end `.data`/`.rodata` from splat's extraction: menu structures, display lists, text tables, libultra's VI modes | ~38 K (+353 K bss) | `blastcorps/asm/data/**` → `asm2x86.py` / `asm2ll.py` |
| 1b | ROM bytes | the two pointer-bearing data islands (7D9D0, 800DC) | 8.6 K | same |
| 1c | ROM bytes | bin islands: sine and u16 tables, per-level tables, RSP microcode text (18.3 K) and ucode data (6.8 K) | 33 K | `blastcorps/assets/<module>/*.bin`, `.incbin`'d |
| 1d | ROM bytes | the decompiled C's data initializers (`tools/data_c.py`): textures/TLUTs ~62 K, struct tables ~46 K, strings, Vtx/Gfx | ~206 K of sections | `blastcorps/src/**/*.c` |
| 1e | all of 1a–1d as carried | the movable/wasm build's arena image, 106 runs | 192 K | `bepass/Arena.cpp` `writeImage()` |
| 2 | translated ROM code | Rare's handwritten engine, 688 functions (220 K of MIPS), and jp's 17 IDO `GLOBAL_ASM` functions | ~1.1–1.4 MB of host code | `tools/recomp` → `blastcorps/build/recomp/src` |
| 3a | decompiled C | the game's own code | 370 K x86 | `blastcorps/src/**` |
| 3b | decompiled SDK | libaudio (~40 files), gu, `ll.c`, decompiled from/with ultralib | 53 K x86 | `blastcorps/src/libultra/**` |
| 3c | SDK source | `port/src/gu_extra.c` (guRotate etc., SDK source) | small | `port/src` |
| 3d | SDK headers | `blastcorps/include/2.0I/**` (SGI notices); only macros, layouts and constants reach the binary | — | compile time |
| 3e | gzip 1.2.4 as Rare linked it | `inflate` (public domain) and unzip/get_method/bi_reverse/clear_bufs, which are GPLv2+ upstream | 10 K x86 | `src/gzip_*.inc.c` |
| 4a | the port's own code | host renderer, audio HLE, loaders, replay, fibers | ~140 K | `port/host`, `port/src` |
| 4b | third-party, native | SDL2 (zlib), libepoxy (MIT), glibc (LGPL), libgcc_s: dynamic | — | system |
| 4c | third-party, wasm | emscripten's SDL2 (zlib), musl (MIT), emscripten runtime (MIT/UIUC), compiler-rt (Apache-2.0 with LLVM exception) | in .wasm/.js | emsdk |

**Also:**
- The native builds carry about 7.5 MB of DWARF and symbols, with the decomp's names and build paths.
  `llvm-strip` removes them (9.7 MB to 2.2 MB).
- `PORT_N64_FUNCS` compiles one absolute build path into every binary.
- The repository has no root LICENSE yet.

## What publishing binaries would take

1. **Movable builds only** (wasm, 64-bit native). They are the macOS and wasm path anyway, and their
   data is a single image.
2. **The data from the user's ROM at run time.**
   - At startup, inflate hd_code and hd_front_end from the ROM with the game's own inflate, as
     `port/src/overlay.c` already does for the front end, and copy their data and island ranges into the
     arena.
   - Then apply a patch list made at build time by port-arena. It holds only:
     - copies: the ~20.6 K moved above 0x400000;
     - byte swaps for native endian (BEPass's tables);
     - words set to symbol addresses;
     - zero ranges.
   - The build fails unless every difference between that reconstruction and the real image is one of
     those operations. The ~1.8 K of differing bytes each need explaining; there is a short audited list of
     port-owned constants.
   - The ROM's sha1 is checked at startup.
   - The RSP microcode text can be zero-filled, since the HLE never runs it.
   - Estimate: 1–2 weeks. The inflate costs under 5 ms at startup.
3. **The engine rewrite (category 2).** The translation is ROM-derived by construction, so a published
   binary needs Rare's engine as hand-written C. The alternative is an interpreter, estimated 5–20× slower
   on that third of the code. See below.
4. **`gu_extra.c`** replaced by a clean implementation.  The gzip driver's GPLv2+ is fine: the project
   is to be licensed AGPL-3.0 (GPL otherwise, below), which GPLv2-or-later code can join.
5. **Strip releases**, and drop `PORT_N64_FUNCS`.
6. **Notices** for SDL2, libepoxy, musl and emscripten, and a root LICENSE for the project's code: the
   owner's intent is **AGPL-3.0**, or plain GPL where that can't work.  The third-party licenses above
   (zlib, MIT, Apache-2.0 with the LLVM exception, LGPL for dynamic glibc) are all compatible with it.
   What can't be relicensed by the project is what isn't its own: the decompiled SDK parts (3b/3c/3d,
   ultralib has no license either) and anything ROM-derived.
7. **The decompiled game and SDK C (3a/3b)** is the same material as the repository's source. It is the
   owner's decision. If libaudio had to go, a clean-room synthesizer and sequencer is weeks of work and
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

**Checking it:** function by function with the unicorn difftest (`tools/recomp/test/difftest.py`),
which is how the translation was checked, and as a whole with the TAS suite (`port/tools/test.py`).
