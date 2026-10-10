# Blast Corps decompilation

A matching decompilation of Blast Corps / Blastdozer, aimed at a PC port. All
four ROMs rebuild byte for byte, from one source tree (`blastcorps/src/`).
us.v11 is the working version: most of its IDO-compiled code is C
(`tools/progress.py`); about a third of hd_code is Rare's handwritten asm,
which stays asm. us.v10, jp and eu build from the same C, with `#if`'d
differences (see "Versions" below).

## Build

Needs `mips-linux-gnu-` binutils on `PATH` and splat's Python deps in a venv.
`VERSION` is one of `us.v10 us.v11 jp eu`, default `us.v11`.

```
make VERSION=us.v11 extract                  # stage 1: split the ROM
make VERSION=us.v11                          # stage 1: relink, sha1-checked
make VERSION=us.v11 decompress               # inflate the gzipped code modules
make VERSION=us.v11 -C blastcorps extract    # stage 2: disassemble
make VERSION=us.v11 -C blastcorps            # stage 2: reassemble, sha1-checked
make VERSION=us.v11 -C blastcorps compress   # re-deflate into assets/
make VERSION=us.v11                          # relink the ROM from that
```

`compress` also copies the rebuilt `init` into `assets/`, so the ROM is made
entirely from rebuilt modules. The ROM link places everything from
`hd_code_text` on by the compressed modules' sizes (`tools/rom_syms.py`), and
the header checksum is recomputed (`tools/n64crc.py`); for the original
modules both are no-ops.

A shifted build proves nothing depends on absolute addresses: `SHIFT=1` puts
`SHIFT_PAD` (default `0x10`) bytes of zeros at the start of hd_code's and
hd_front_end's `.text` and `.data`, and `ASSET_SHIFT_PAD` (default `0x10`)
before the texture table and every 16-aligned asset, and nothing is
sha1-checked (stage 2 runs the top level's asset build itself, so the order
below is enough):

```
make VERSION=us.v11 -C blastcorps SHIFT=1 all compress
make VERSION=us.v11 SHIFT=1
```

Rebuild without `SHIFT` (both steps) to get the matching assets back.
`SHIFT_LDFLAGS="--defsym SHIFT_PAD_hd_code_data=0x20 ..."` pads single
sections (`hd_code`, `hd_code_data`, `hd_code_bss`, and the same for
`hd_front_end`), which is how to bisect a shift that breaks something.

`asm/` and `assets/` hold one version at a time. Switching `VERSION` without
`make clean` (both directories) is rejected by the `stamp` target — that guard
exists because stale output from another version otherwise links in silently and
surfaces only as a confusing sha1 mismatch.

The PC port (docs/PORT.md) is a separate CMake build over the same source,
after the N64 build and `make -C tools/recomp`:

```
cmake -S port -B build/port -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_ASM_COMPILER=clang
cmake --build build/port && build/port/blastcorps baserom.us.v11.z64
```

`port/build.py ROM` (or `--wasm ROM`) does all of the above for a user in one
go (README, "Build it").  `-DPORT_VERSION=us.v10` builds the us.v10 port instead (in its own build
directory, from a us.v10 stage 2 and `make -C tools/recomp VERSION=us.v10`),
which is what the TAS replays on (docs/PORT.md, "The TAS").
`-DPORT_VERSION=jp` builds Blastdozer the same way (`VERSION=jp`
throughout); jp's remaining `GLOBAL_ASM` functions are C in
`port/engine/jp_*.c` like the handwritten code (docs/PORT.md, "Other versions").
`-DPORT_VERSION=eu` builds the PAL version (its C is complete; `port.h`'s
`PORT_RETRACE_HZ`/`PORT_VI_CLOCK` are PAL's there).

`-DPORT_64BIT=ON`, `-DPORT_NATIVE_ENDIAN=ON` and `-DPORT_LP64=ON` build the
64-bit, native-endian and LP64 variants (docs/PORT.md); compare any two with
`port/tools/build_cmp.py` and `layout_cmp.py`.  `-DPORT_MOVABLE=ON` (with
any of them) puts game memory at any base and links a PIE, the way to
macOS and WebAssembly: the N64 side is one LLVM module there, through
port-arena (docs/PORT.md, "Movable memory").  `emcmake cmake` (emsdk)
builds that for WebAssembly, headless under node or as a page
(`-DPORT_WASM_TARGET=web`); the N64 side is still the system clang's
i386 IR, retargeted at the arena link (docs/PORT.md, "WebAssembly").  The game's threads switch by ucontext or by host threads one at
a time (`port/host/fiber.h`); on Linux one executable has both and
`PORT_THREADS=ucontext|pthread` picks, and the two must give identical
deterministic runs (docs/PORT.md, "Threads without ucontext").
`port/tools/cross-aarch64.cmake` cross-builds the 64-bit port for AArch64
Linux, which runs under qemu-aarch64 ("Other hosts").

Port-only source changes in `src/` go under `#ifdef TARGET_PC`.  A
busy-wait in the game's C (a loop spinning on what another thread or an
interrupt changes) calls `port_spin_wait()` in its body there: the port
runs one thread at a time with no polls in loops, so without it the loop
spins forever (docs/PORT.md, "Memory model").  Pointers in
memory are native (8 bytes in the LP64 port); `T *PTR32 p` (4 bytes there,
nothing to IDO) is only for what the ROM's data, libaudio or the display
lists fix, and a variable's declarations all agree (docs/PORT.md, "The
LP64 build").

The port's improvements for the player (`--interpolate` with
`--display-hz auto`, `--hd-text`, `--model-icons`, and any added later) are
on by default with a window, each with a `--no-` option to turn it off;
headless runs and the compared ones (`--deterministic`, `--replay`) keep
them off unless asked for, so the test references and the TAS don't move
(`port/host/main.c`, after the options: `windowed`).  A new one follows
the same rule, and gets a box on the page (`port/web/shell.html`).

## Assets

The top-level config names every piece of the ROM (docs/ASSETS.md has the
inventory and formats). `make extract` runs `tools/assets.py extract`, which
writes editable files under `assets/` (texture PNGs, level YAML, ctl/tbl/seq,
inflated gzip members) and fails unless they rebuild to the ROM's bytes;
`make` links the ROM from `tools/assets.py build`'s output in
`build/assets/`, not from splat's bins. `tools/rom_syms.py` places every
segment from init on by the size of its built file, and the texture and
model tables are regenerated from where things ended up, so an edited
asset may change size. The stage-2 links take the asset positions from
`build/assets` too.

- **Blast textures keep the ROM's stream.** The compression is lossy and
  Rare's encoder isn't reproducible from the texels, so `NNN.blast` is used
  while the PNG still decodes from it; only a changed PNG is recompressed.
- **The LZSS is Mark Nelson's with BREAK_EVEN 2** (`tools/assetlib/lzss.py`);
  his tree encoder reproduces the ROM's data. gzip members use `tools/gzip`
  at `-6`, like the code.
- **The texture table and the model table hold offsets from their own
  start;** the handwritten loaders reach them through
  `texture_table_ROM_START`/`model_table_ROM_START` (postsplit symbolizes
  the `lui`/`addiu` pairs, including 5BF40's `lui 0`).
- **Regenerate the top-level configs** with `tools/gen_build_yaml.py` after
  changing `tools/assetlib/romlayout.py`; `assets.py extract` refuses a
  config whose segments aren't where the ROM has them.

## What "matching" depends on

The sha1 checks are the arbiter. Everything below is a way to keep them passing.

- **gzip mtimes are part of the compressed stream.** The `*_TIMESTAMP` values in
  `blastcorps/Makefile` come from each ROM's own gzip headers. A wrong one makes
  a rebuilt section differ even when the code inside it is identical.
- **Padding must stay outside the code sections.** `hd_code_*` and
  `hd_front_end_*` get inflated and re-deflated, so a recompressed section is the
  bare gzip stream. Any inter-section padding folded into a segment shifts
  everything after it. An asset segment in the config runs to the next one's
  start; `tools/assets.py` pads each built asset out to that (the ROM's pad
  bytes are kept where they aren't zeros).
  This was a live bug in `blastcorps.us.v10.yaml` (`0x7f9c76` should have been
  `0x7f9c75`).
- **Every name must still be where it was.** Outside `SHIFT`, each link is
  followed by `tools/link_syms.py --check`, which fails if any name in the
  symbol files moved; that points at a layout problem before the sha1 does.
- **Config offsets are generated, not hand-written.** `tools/gen_build_yaml.py`
  reads the top-level split straight out of the ROM (gzip and LZSS streams
  are self-delimiting, the tables and sound banks give their own sizes); `tools/gen_code_yaml.py` runs splat once for file boundaries
  and emits the stage-2 module configs. Both reproduce the hand-maintained US
  configs, so prefer regenerating over editing offsets by hand.

## Layout notes

Load addresses, identical across all four revisions:

| module         | VRAM         | notes                                          |
| ---            | ---          | ---                                            |
| `init`         | `0x8021ED00` | uncompressed at ROM `0x1000`                   |
| `hd_code`      | `0x802447C0` | staged at `0x80000400`, inflated by `init`     |
| `hd_front_end` | `0x801E7000` | inflated by `func_8028B3E0` in `hd_code`       |

Regions kept as `bin` because they are not r4300 code:

- The last `0xFB0` bytes of every `hd_front_end` `.text` are RSP microcode
  (`sp = 0x110`, loads from DMEM `0xFC4`, exactly IMEM-sized).
- Six data islands inside each `hd_code` `.text`, listed with their offsets in
  `tools/regen_code_yaml.sh`: two u16 tables embedded in Rare's handwritten
  math routines, two pointer-bearing blobs around another block of handwritten
  routines, a block of per-level tables the handwritten code reads (it looked
  unreferenced: the only reference is a scheduled `lui`/`addiu` pair in
  `func_802A1EC8`), and the RSP microcode after libultra's
  `ldiv`. Four are byte-identical across versions; the pointer-bearing two were
  bounded by following control flow from every call into them. Capstone decodes
  data as Octeon opcodes (`bbit0`, `synci`, `dlsa`) that `-march=vr4300`
  rejects, so an island left as `asm` fails to assemble.

The islands are not 16-aligned, so the code between them starts and ends at odd
offsets. That is why the code segments use `subalign: 4` and stage 2 assembles
with `--no-pad-sections`; either default pads those objects and shifts
everything after them. The same code also trips splat's file-boundary
suggestions, so `gen_code_yaml.py` drops any boundary that a branch crosses.

The two pointer-bearing islands are written as `.s` files with those words
as `.word <symbol>` (`tools/postsplit.py`, `pointer_bins`), so they move with
the code they point at.

The jp/eu configs used to lump about 0x38000 bytes of real code into these bins
(the generator ignored bin lengths). It still matched, because a bin is exact,
so a too-wide `bin` goes unnoticed: only the sha1 decides, and it cannot tell.

## Functions and symbols

- One `.s` per function under `asm/nonmatchings/`, pulled into the committed
  `src/**/*.c` files by `GLOBAL_ASM`. Splat writes a stub only when it is
  missing, so after anything that moves function boundaries (a new
  `symbol_addrs` entry, a config change) delete the untouched stubs and
  re-extract, or they list the old split.
- `symbol_addrs.<module>.<VERSION>.txt` are generated by `tools/gen_symbols.py`
  (below the marker line). Only libultra is named; everything else keeps
  `func_`/`D_` address names on purpose. Don't guess names. The address in a
  name is us.v11's in every version (see "Versions").
- libultra here predates every version ultralib builds. Its libc (`sprintf`,
  `_Litob`, `_Ldtob`) is `-O1` where ultralib's 2.0I uses `-O3`, hence the
  extra `-O1` build in `tools/build_ultralib.sh`. A few functions
  (`__osDispatchThread`, `__osSyncPutChars`, `osPiStartDma`, `osSpTaskLoad`,
  `alAudioFrame`) differ outright and are placed by similarity (`lib:fuzzy`).
- Hand-identified code goes in `blastcorps/symbols_known.txt` (us.v11 init
  addresses, with evidence). gen_symbols copies each name to every
  masked-identical copy, pairing globals by lui/lo position. Rare's gzip 1.2.4
  inflate lives there: one copy in init (inflates hd_code), one in hd_code
  (inflates hd_front_end). `bi_reverse` and `clear_bufs` match as gzip's own
  source, which pins down the global names.
- Each module links its own copies of libultra, so a name like `__osDisableInt`
  exists in both `init` and `hd_code` at different addresses. Cross-module names
  (`from:`) are only added where they don't collide.
- Splat starts a function only after a `jr $ra` that no branch spans. Every call
  target it would miss is listed as `func_XXXXXXXX ... called`; a new one that
  isn't shows up as a jal into the middle of a function.

## Decompiling

`docs/ROADMAP.md` has the plan and `docs/DECOMPILING.md` the matching notes. The loop for one function:

- `tools/m2c.sh <module> <function>` for a first draft (m2c, run from the
  project venv). Replace the `GLOBAL_ASM` line with the C.
- `make VERSION=us.v11 -C blastcorps <module>`, then
  `tools/fdiff.py <module> <function>` to diff it against the original. It
  refuses to run when the last link failed, since the binary would be stale.
  Then build the other versions too (below): the same C has to match them.
- `tools/progress.py` for totals.
- For a draft that's only registers or scheduling away,
  `tools/permute.sh <module> <function> [draft.c]` sets up decomp-permuter
  (`docs/DECOMPILING.md`, "The permuter").

IDO quirks that have mattered so far (all `-O1`):

- `register` variables get `$s` registers in declaration order, and each
  still gets a stack slot at the point where it's declared. Declaration order
  moves both the register assignment and the offsets of the other locals.
- Statement spelling changes the scheduling: `p = c + 1, xp = x + 2;` and the
  same two statements separated by `;` don't compile the same.
- m2c drops some early returns; check the function's size.
- A global defined in the same file is addressed differently from an
  `extern` one (a `u64`'s two halves share one `lui`). Such functions only
  match with their data defined in C, in the file that owns it (`.data`,
  `.rodata` and `.bss` are per object in us.v11).
- libultra was built with unsigned `char`; this build passes `-signed`.
- Per-file flags go under "Optimisation Overrides" in `blastcorps/Makefile`
  (`ll.c` is `-mips3 -32`, relabelled mips2 by `tools/elf_mips2.py` so ld
  will link it).

When symbols_known.txt names a function, run `tools/gen_symbols.py` for each
version (with the ultralib objects from `tools/build_ultralib.sh`), then
`tools/sync_stub_names.py`, then remove `blastcorps/asm` and re-extract.
Splat only writes a `.s` for names the existing stub lists, so without the
middle step the stubs keep pointing at stale `func_` files.

Handwritten code (the game's own asm, and libultra's `.s` files) is an `asm`
subsegment, not a `c` file full of `GLOBAL_ASM`. That keeps
`tools/progress.py` honest and marks what a port has to replace.
`gen_code_yaml.py` classifies it (see `docs/DECOMPILING.md`). About a third
of hd_code is Rare's handwritten engine; it can't become matching C. For
the port it is C written by hand in `port/engine/`; `tools/recomp/`'s
mechanical translation remains only as the check build's reference, checked
against the original in unicorn (`docs/PORT.md`).

## Versions

One source tree builds all four versions; the Makefile passes one of
`-DVERSION_US_V10`, `-DVERSION_US_V11`, `-DVERSION_JP`, `-DVERSION_EU`
(`common.h` defaults to us.v11, for the port). us.v11 is the reference:

- **Names are us.v11's.** In another version, `func_X`/`D_X` is whatever
  sits where us.v11 has X, and a thing us.v11 doesn't have keeps its own
  address with the version appended (`D_80301234_jp`, `_v10`, `_eu`).
  `tools/vermap.py` works out the correspondence from the binaries alone
  (functions aligned on their masked bodies, data by the references aligned
  code makes, see its docstring) and writes `blastcorps/vermap.<v>.txt`;
  `split.py`/`postsplit.py` name splat's symbols through it and
  `link_syms.py` reads addresses back through it. Where it guesses wrong,
  the version's `port` line in `tools/regen_code_yaml.sh` corrects it with
  `vermap:--pair <us.v11>:<version>[:len]`.
- **Objects are us.v11's.** The other versions' configs are us.v11's with
  every offset moved through the map (`gen_code_yaml.py --like us.v11`,
  run by `regen_code_yaml.sh`'s `port`): same objects, names (us.v11
  offsets) and kinds, in the version's own link order. `--at` places a
  start the map can't, `--asm-object` makes an object all asm in a version
  (eu's multilingual text, below). The version's `undefined_syms` are
  us.v11's moved through the map (below a marker; lines above it are the
  version's own). After a regen, run `gen_symbols.py` for the version.
- **Differences are `#if`s in the C.** A function a version has differently
  is that version's `GLOBAL_ASM` until it's written: `tools/version_asm.py
  <module> <func> <version>` wraps it (`--only`, `--not`, `--as` for
  functions only one side has). Asm-processor opens every `GLOBAL_ASM`'s .s,
  so `tools/version_ifs.py` decides the `VERSION_*` conditionals first (the
  build does). Prefer a macro where a difference is a number: `FE_ENTRY`/
  `YOSHI_ENTRY` (entry table indices, `yoshi.h`), `LINE_EU` (assert line
  numbers), `PAK_GAME_CODE`, `AUDIO_HEAP_SIZE`, `MEDAL_TIME0`.
- **Checking a version:** after `make VERSION=<v> -C blastcorps` (the link
  may fail), `tools/vdiff.py <v>` lists the C functions that don't match the
  version, the ones it has that the C doesn't, block size differences, and
  symbols the matching code puts elsewhere than the map (with the `--pair`
  that fixes it); `vdiff.py <v> --module m --func f` shows one side by side.
  `tools/progress.py --version <v>` counts what's C there.
- Where version differences showed object boundaries us.v11's layout
  hides (jp and eu pad `.text` to 16 before a function), those are object
  splits in us.v11 too (`hd_front_end/8380`, `hd_code/2B3F0`, `4B450`).
- eu's text is in three languages (`YoshiEntry` has `text2`/`text3` there),
  picked by the language `D_80366F70_eu` (0 English, 1 German, 2 French):
  `ENTRY_TEXT`/`TEXT_EU` (`yoshi.h`), `LEVEL_NAME` (`level.h`). Its
  libultra is newer in places (vi.c's PAL `__osViInit`, built `-mips1`;
  abi.h's pole filter), and its timings are PAL's (`FRAMES_PER_SECOND`,
  `common.h`, and files' own macros where eu rounds a count its own way).
  A data block only eu has is `gen_code_yaml.py --add` (1A240's crew
  names); `--asm-object` makes a whole object asm in a version (none is).

## Data and symbols at link time

Extraction goes through `tools/split.py`, a wrapper around the splat
submodule that fixes its data output (exact string escapes, `dlabel`, pointer
words between subsegments as symbols, working rodata migration). `init`,
`hd_code` us.v11 and `hd_front_end` us.v11 have their `.data`/`.rodata` split per object, in link order:
all `.data`, then all `.rodata`, each block 16-aligned. `gen_code_yaml.py`
works out hd_code's split (see `docs/ROADMAP.md`, Phase 1). A C file owns
its data with a `.data`/`.rodata` subsegment; its remaining `GLOBAL_ASM`
functions then carry their own rodata into the `.s`, and asm-processor places
it. `tools/inline_rodata.py` turns a file's `extern` string/float uses into
literals when switching it to own its `.rodata`. Every C file owns its
`.data` and `.rodata`, in every version;
what is still asm `data`/`rodata` belongs to handwritten objects or to
data-only objects (named by offset: text tables, display lists, libultra's
VI modes). `tools/data_c.py <module> <object> [--version v]` writes a C
file's `.data` definitions from the asm block, typed by the file's
declarations (see `docs/DECOMPILING.md`); it needs the block still `data` and
extracted, or `--ref` with a saved copy of its `.data.s`.

`.bss` is laid out per object too, in the top-level `bss:` list of
each module's config (`tools/modmap.py` reads it; splat ignores it):
`[vram, bss|.bss, <module>/<object>]` in link order, then `[end]`.
`gen_code_yaml.py --bss` works it out for hd_code and hd_front_end as it does
`.data`; init's is hand-written. `postsplit.py` writes an asm object's block
as `asm/data/<module>/<object>.bss.s` and adds a NOLOAD `.<module>_bss`
section to the linker script. A `.bss` block belongs to a C file, which
defines its variables itself: `tools/bss_c.py <module> <object>` writes those
definitions from the block (reference copy in `asm/bss/`), and
`bss_c.py --check` compares an object with it. Every C file owns its `.bss`;
only the handwritten objects use `.bss.s`. IDO emits uninitialized globals
into `.bss` (not COMMON) in the order it first sees them, so the tool puts
the definitions before any use; see `docs/DECOMPILING.md`.

`postsplit.py` also symbolizes the lui pairs splat leaves as numbers (split
by scheduling, 64-bit loads, `add`/`addi` in Rare's asm) wherever they point
into a module or at a ROM segment, and data words (even inside `.byte`
records) that point into any module. What no object here defines by name goes
into `module_syms_auto.<module>.<VERSION>.txt` if it lies in a module, else
`undefined_syms_auto` (only hardware registers, the boot globals at
`0x800003xx`, segment addresses, ROM offsets and the fixed memory map are left
there). At link time `tools/link_syms.py` turns all the symbol files into
`build/<module>.<VERSION>.syms.ld`: names an object defines are dropped, the
rest of this module's become relative to the nearest symbol below them, other
modules' come from their layout links (each module is linked twice, see the
Makefile), ROM segment starts become `rom_syms.py`'s symbols, and the rest
stays absolute. `build/*.syms.txt` lists which is which. A hand-written line
in `undefined_syms.<module>.<VERSION>.txt` marked `/* fixed */` stays
absolute even inside a module (hd_code's allocator limit `0x8020ED00`).

A C file can still name an address splat never saw with a line in
`undefined_syms.<module>.<VERSION>.txt`; inside a module that is resolved
like the rest, so don't use constant-address casts for it.
`tools/find_abs.py <module> <VERSION>` lists what a linked module still
points at without a relocation (the final links keep `--emit-relocs`); what's
left is the fixed memory map: the modules' load addresses, the framebuffers
at `0x80000400` and `0x8021ED00`, `0x80400000`, and the ROM range init leaves
at `0x803FFFF8`.

## Conventions

- Straight to `main`; this project does not use branches.
- `docs/sm64tools-configs/` are queueRAM's original configs, kept for reference.
  Their offsets are not always right (the `jp` `hd_front_end_data` end is wrong),
  so verify against the ROM before trusting one.
