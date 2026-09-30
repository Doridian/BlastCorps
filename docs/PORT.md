# The PC port

The PC build (`port/`, milestone 1 of ROADMAP Phase 5) and the translator
it runs Rare's handwritten asm through.  The build is described first; the
translator and its test follow from "The handwritten asm".

## Building and running

```
make VERSION=us.v11 -C blastcorps          # the N64 build: the port reads its ELFs and asm/
make -C tools/recomp                       # translate the handwritten engine
cmake -S port -B build/port -G Ninja -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_ASM_COMPILER=clang
cmake --build build/port
build/port/blastcorps baserom.us.v11.z64   # the user's own ROM
```

`-DPORT_VERSION=us.v10` or `-DPORT_VERSION=jp` builds another version,
from that version's stage 2 and `make -C tools/recomp VERSION=...`, in a
build directory of its own ("Other versions").

It needs clang/LLVM with its development headers (BEPass is an LLVM
plugin), a 32-bit (multilib) libc and SDL2, and Python 3; the OpenGL
renderer also needs 32-bit libepoxy and an OpenGL 3.3 driver (without
libepoxy only the software renderer is built).  `-DPORT_64BIT=ON` builds
a 64-bit program instead (x86-64 now, AArch64 in principle; see "The
64-bit build"), which needs LLVM's `opt` and the ordinary 64-bit SDL2 and
libepoxy rather than the multilib ones; `-DPORT_NATIVE_ENDIAN=ON` keeps game
memory in host order ("The native-endian build"), and `-DPORT_LP64=ON`
(both of those and more) compiles the game's C as an ordinary LP64 program
("The LP64 build"):

```
cmake -S port -B build/port64 -G Ninja -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_ASM_COMPILER=clang -DPORT_64BIT=ON
cmake --build build/port64
```

Keys: arrows or
WASD for the stick, X = A, C = B, Z = Z, Enter = Start, Q/E = L/R, IJKL = C
buttons, TFGH = D-pad; an SDL game controller works too.  `--help` lists
the options: `--headless`, `--deterministic` (virtual time: as fast as the
host can, identical every run), `--frames N`, `--screenshot PREFIX`,
`--save PATH` (the 4 Kbit EEPROM, default `blastcorps.eep`; runs meant to
be compared should each start from the same one, or none),
`--renderer gl|sw` (default: OpenGL with a window, software headless),
`--scale N` (OpenGL: render at 320x240 times N; by default it follows the
window's height, so resizing the window changes it) and
`--filter n64|bilinear|point` (textures: the N64's 3-point filter where
the game asks for bilinear filtering, which is the default, a 4-tap
bilinear one, or point sampling throughout), `--wav PATH` (everything the game plays, at the AI's rate) and `--no-audio`; sound goes
to SDL unless the run is `--headless` or `--deterministic`.
`PORT_AUTOSTART=1` taps Start and A, which is enough to get from the title
through the name entry into Simian Acres (`=2` taps by the game's own
retrace count and then drives forward in the level, as
`port/tools/m64p_pace.c` does in mupen64plus); `PORT_DUMP=N,...` writes RDRAM at
the Nth controller read, counted as `tools/recomp/test/snapshot.c` counts
them in mupen64plus, so the two can be compared byte for byte.
`PORT_SHOT_EVERY=N` saves a screenshot every N frames (at the internal
resolution with OpenGL); `PORT_GL_QUANT=1` makes the OpenGL renderer write
5-bit color, as the RDRAM framebuffer holds it, and `PORT_GL_READBACK=1`
copies its framebuffers back to RDRAM after every task (see "Graphics").
`PORT_PACE=FILE` logs the pacing at every controller read (see "Timing").

## Memory model

The decompiled C, the translated asm, the game's data and whatever the ROM
holds all have to agree on memory.  The port keeps the N64's view of it
exactly, and adapts the host to that, rather than the other way round:

- **32-bit addresses.**  The whole program is a 32-bit x86 program
  (`-m32 -malign-double`), so a pointer is 4 bytes and every struct has its
  N64 layout.  The executable is linked at `0x80400000`.  (The 64-bit build
  keeps the layout and the addresses and changes only the code: see "The
  64-bit build".)
- **RDRAM at `0x80000000`.**  The first 4 MB of the address space is the
  image's `.rdram` section (`tools/gen_ld.py`), so a KSEG0 address *is* the
  host address.  The translated code's `rdram + (addr & 0x1FFFFFFF)` with
  `rdram = 0x80000000` is the identity; `K0_TO_PHYS`, `PHYS_TO_K0`,
  `osVirtualToPhysical` and segment addresses round-trip; the game's fixed
  buffers (framebuffers at `0x80000400`, the level pool from `0x8004B400`,
  the heap after `.bss` at `0x803FF600`) are where it expects them.  The
  hardware registers (`0xA4000000`) are mapped as plain memory, and the
  fibers' host stacks sit at `0x90000000`, inside the KSEG0 window, so the
  address of a local means the same to the translated code.  A 64-bit
  kernel puts the brk heap anywhere up to 1 GB above a non-PIE image (a
  32-bit program's within 32 MB), so in a 64-bit build it is, a few starts
  in a hundred, already in one of those two windows when `main` maps them;
  then the port runs itself again with `ADDR_NO_RANDOMIZE`, which puts the
  heap right above the image (`map_fixed`, `host/main.c`).
- **Every game variable at its N64 address.**  The decompiled C relies on
  the layout of the data, not just its contents: code reads fields through
  names of their own (other files read the scheduler's frame counter,
  `Sched.frameCount`, as `D_803156C4`), tables are indexed from a
  neighbour, the front end's area is cleared by address.  The game's C is
  built with `-fdata-sections`, the data files are converted one section per
  file (`tools/asm2x86.py`), and `gen_ld.py` places each at its address in
  the N64 link; the build checks all 3,937 symbols afterwards.  Anything in
  RDRAM the port doesn't define is simply there, at its N64 address
  (`tools/gen_syms.py`).
- **Big-endian memory.**  The data files, the ROM's assets and most of the
  C-owned data are untyped bytes: a native little-endian reading would need
  every structure typed first (Phase 3/4).  Instead the game's C is compiled
  by clang with **BEPass** (`port/bepass/`), an LLVM pass that byte-swaps
  every multi-byte load and store, so memory holds exactly the N64's bytes.
  Type punning, unions, `u64` halves, reading a struct from the ROM: all
  behave as on the N64.  The translated asm stays in its tested big-endian
  mode, and DMA is a plain copy.  Initialized data is fixed at startup: the
  pass records the scalars in C initializers and a constructor swaps them
  (`__bepass_fixup`), and symbolic `.word`s in the data files are listed in
  a `port_bswap32` section and swapped by `port_fixups()`.  Swaps on locals
  fold away; what's left costs a `bswap` or `movbe`.  Only code built with
  the pass is on the "N64 side"; host code (`port/host/`) that touches game
  memory does so through `port_be32()` and friends (`port/include/port.h`).
  Functions that use `va_start` are left unswapped (the argument area is
  the host's); the game's are empty debug printfs, and `sprintf` is the
  host's `vsprintf`.
- **Why this and not native pointers.**  It is the quickest way to run the
  code as it is: no data needs typing, the translated engine is used exactly
  as tested, and the game's own assumptions about addresses hold.  `-m32`
  and BEPass turned out to be separable: the 64-bit build (below) drops
  the first and keeps the second.  Native-endian memory is the harder
  half and needs the data typed (see "Native-endian memory").

BEPass also puts a call to `__port_poll()` on every loop back edge.  The
game busy-waits on counters that another thread or an interrupt advances
(`while (D_803156C4 - sp60 < 15) {}` waits for retraces); the port runs its
threads one at a time, and the poll is where it lets a due event in.  As an
opaque call it also stops clang from hoisting the load out of the loop, as
IDO never would.

## The 64-bit build

`-DPORT_64BIT=ON` makes the port an ordinary 64-bit program (x86-64; the
same route works for AArch64) that keeps everything the memory model above
depends on: RDRAM at `0x80000000`, every game variable at its N64 address,
the N64's struct layouts, big-endian memory, the translated engine as
tested.  Only the code changes.  The game's C (and `port/src`) is compiled
for **i386's layout and 64-bit code**:

1. clang compiles it for `i386-pc-linux-gnu -malign-double` to unoptimised
   IR, so the frontend lays every type out as the N64 does (4-byte
   pointers and `long`s, 8-byte aligned `u64`s and doubles);
2. `opt -passes=port-ilp32` (`port/bepass/ILP32.cpp`, in BEPass's plugin)
   rewrites the module for the 64-bit target without changing any layout;
3. clang optimises it and generates 64-bit code, running BEPass as usual.

`port/tools/ilp32cc.py` does the three steps as CMake's compiler launcher
for the N64-side targets.  Everything else (the host side, the translated
engine, the data files) is compiled natively; none of it depended on
32-bit pointers, since the translated code and the host already handle game
memory through 32-bit N64 addresses.

What port-ilp32 does to the i386 module:

- **Pointers are 8 bytes in registers and 4 in memory.**  A load of a
  pointer is a 32-bit load and a zero extension, a store truncates, a
  pointer in an initializer is a 32-bit relocation.  This is sound because
  every address the game's C ever holds is below 4 GB: RDRAM, the image
  (non-PIE, at `0x80400000`, linked with the PIE start files and
  `--no-pie`, since the non-PIE `crtbegin.o` addresses its data with
  sign-extended 32-bit immediates), and the fibers' host stacks at
  `0x90000000`.  `-DPORT_ILP32_CHECK=ON` checks every pointer store for
  that; 12,000 frames of the attract mode and 6,000 of each
  `PORT_AUTOSTART` mode stored none above 4 GB.
- **The layout is made explicit.**  GEPs become byte offsets computed by
  the i386 layout, allocas and globals whose type holds a pointer become
  packed structs of the same size with explicit padding, `byval`/`sret`
  types become byte arrays; then the module gets the 64-bit triple and data
  layout (`-port-ilp32-triple`), and loses the i386 target attributes.
- **Pointer arithmetic wraps at 32 bits**, as on the N64.  The game relies
  on it: libaudio's `alBnkfNew` relocates a bank's offsets with
  `(u8 *)offset + (s32)base`, which on a 64-bit GEP sign-extends the
  KSEG0 base and lands below zero.  A GEP by a variable (or large) offset
  is computed on the 32-bit address; one by a small constant (a field)
  stays a GEP, so it still folds into the access.
- **Accesses outside the named variable** go through an integer, so LLVM
  can't assume they don't alias the neighbour they really touch (the
  decompiled C's `T x[1]` placeholders and `extern T x[]` read and write
  past their end on purpose).  Variable offsets already do, by the rule
  above; the constant ones are all on incomplete arrays (84 accesses in 11
  files, `PORT_ILP32_STATS=2` lists them).
- **K&R calls behave as on the N64.**  The decompiled C declares the same
  function differently in different files, which on MIPS (and i386) is
  harmless and on x86-64 isn't.  Three rules make it so again: pointer
  arguments are re-zero-extended on entry (a caller may have passed an
  `int`); a `u8`/`s8`/`u16`/`s16` result is returned extended to 32 bits
  (`00000.c` declares `s32 func_8028653C(void)`, `41930.c` defines it
  `u8`: x86-64 leaves the upper bits of `eax` undefined, the menu took the
  wrong branch and the game crashed two screens later); a callee
  re-extends its own narrow arguments, as IDO's code does.  A pointer
  result is zero-extended at the call.
- **No varargs definitions** (the `va_list` would be i386's): the pass
  refuses them.  The game's own are empty; `sprintf` and the copies
  (`bcopy`, `bzero`, `memcpy`, which call the host's with a `size_t`) are
  host code in the 64-bit build (`port/host/libc64.c`).

The glue changes for both builds: `gen_glue.py` passes integer arguments
into native functions as `uintptr_t` (the 32-bit value zero-extended, so a
pointer parameter isn't left with an undefined upper half) and returns
integer results the same way, narrow ones extended as MIPS returns them.
In the 32-bit build `uintptr_t` is `uint32_t`, and its runs are unchanged
(all ten screenshots of the run below are identical to before).

The fixed addresses stay as they are: `gen_ld.py` and `gen_syms.py` work
unchanged on the 64-bit objects, and the link's check finds all 3,432
symbols in RDRAM at their N64 addresses.

**Result.**  `--headless --deterministic --renderer gl --frames 3000` with
`PORT_AUTOSTART=1` plays the same sequence as the 32-bit build: the N64
and Rare logos, the title, the name entry, the world map, Simian Acres and
its pause menus.  The screenshots at 300 and 600 frames are identical;
later ones are a few frames apart, because the C's instruction count (the
timing model's ICount) is of x86-64 IR, not i386's; the pacing per mode is
the same (2.00 retraces a frame in Simian Acres, 1.99-2.00 on the title).
With the timing taken out of the comparison (see "Comparing builds") the
two builds are identical: every screenshot of 3,000 frames, and RDRAM at
every fifth controller read up to the 500th except the addresses of the
port's own code and stacks.  The first difference, by the 505th, is a
miscompile in the 32-bit build, not the 64-bit one: `2B3F0.c` stores
`D_8036BBB0[n] = D_8036BBB0[n + 1] = i` into a `u16 D_8036BBB0[1]`, the
N64 code stores both halves, the 64-bit build does, and the 32-bit build's
clang drops the out-of-bounds one.  The next one is the same with
`26570.c`'s `D_8036BB48[sp28]` (the 64-bit build writes the index, the
32-bit one element 0), which puts a `#` into the text of an attract-mode
panel.  4,500 deterministic frames take 2.3 s, against 2.6 s for the
32-bit build.

**AArch64.**  The whole N64 side (122 files) compiles for
`aarch64-unknown-linux-gnu` through the same pass; the host code has no x86
code left (`gfx.c`'s `-msse4.1` is x86-only now), but neither is linked or
run here (no AArch64 sysroot).  Linux on AArch64 can link a non-PIE image
at `0x80000000` (ADRP reaches ±4 GB).  macOS on arm64 can't (executables
must be PIE); there the game's variables would have to move out of the
image into the arena, which needs every one of them reached through a
symbol the port can relocate.

**Alternatives.**  Kept for the record, since each was a candidate:

- clang's `__ptr32 __uptr` (`-fms-extensions`): it works on Linux, for
  x86-64 and (clang 22) AArch64 alike (a struct `{int *__ptr32 __uptr p;
  int x;}` is 8 bytes, `p` a zero-extended 32-bit load).  But every
  pointer declaration in the game and its headers would need the
  annotation (an empty macro for IDO), `long` would still be 8 bytes, and
  an unannotated pointer would silently be 8.  It is the natural spelling
  for the few shared fields that stage 3 keeps 32-bit.
- A `PTR32(T)` type (a pointer for IDO, a 32-bit handle on the PC): the
  same annotation everywhere, plus a conversion at every use.  Worth it
  later only for the structs that stay shared with the asm or the ROM
  (stage 3 below).
- The x32 ABI: exactly this model, but it needs `CONFIG_X86_X32_ABI`
  (off in most kernels) and x32 builds of SDL and the GL driver.
- Native LP64 C: changes every struct with a pointer in it, which the
  translated asm and the ROM's data read at N64 offsets.  That is stage 3,
  after the types are known.
- Narrowing pointers in native LP64 IR: needs the frontend's layout to be
  the N64's to begin with, which is what the i386 frontend gives.

## Comparing builds

Two builds of the port can be compared run for run when the timing
doesn't depend on the code: `--deterministic`, the same `--save` (or
none), and `PORT_COUNT_PER_OP=0` (the CPU takes no time, so the C's and
the asm's instruction counts don't move events).  Then:

- `PORT_DUMP=N,...` in both and `port/tools/build_cmp.py rdram A B N...`
  compares RDRAM at those controller reads, leaving out words that are
  addresses in the port's image or on the host stacks in both;
- a `-DPORT_TRACE_CALLS=ON` build calls `__port_trace` on every entry to a
  function of the game's C; `PORT_TRACE=FILE,FROM,TO` writes them between
  two controller reads, and `build_cmp.py trace A.bin B.bin EXE` shows
  where two traces part.

That is how the 32-bit build's miscompiles above were found (and the
64-bit build's early bugs: the libaudio relocation, the narrow returns).
Against a native-endian build ("Native-endian memory"):

- `build_cmp.py rdram --native BE NATIVE N...` compares RDRAM through the
  byte orders a word can have; when the native run is a
  `-DPORT_ACCESS_PROFILE=ON` build's, each dump comes with the width every
  byte was last written or converted at (`rdram_N.widths`) and the compare
  is exact, byte by byte;
- a `-DPORT_TRACE_ASM=ON` build (the translated code built with
  `RECOMP_TRACE`) writes, for `PORT_ITRACE=FILE,FROM,TO`, every translated
  instruction's address and the registers before it, and `build_cmp.py
  itrace A.bin B.bin` finds the first instruction whose registers differ
  but for byte order (a word copy of halves or bytes, which native memory
  holds in its own order).  The registers hold the same values in both
  builds, so that instruction is the one after the load that read
  something typed wrongly.  (Much slower; one or two controller reads at
  a time.)

## Native-endian memory

BEPass costs little (a `movbe` or `bswap` per access), so big-endian
memory isn't a speed problem.  It is what keeps the port from being
ordinary C: host code has to read game memory byte by byte, the ROM's
assets are untyped bytes, and native C can't share a struct with the
game.  The plan, in stages, and what each needs:

- **Word-swapped memory** (emulator style: 32-bit words native, bytes and
  halves at `addr ^ 3` and `addr ^ 2`) needs no types, but every byte
  and halfword access of the C would have to be rewritten and native C
  still couldn't read a struct with small fields.  Rejected: no gain over
  BEPass.
- **Native scalars at the N64 offsets** is the target: every datum in
  host order where it is, the layout unchanged.  Then only data produced
  as untyped bytes need converting, once, where they're produced: PI DMA
  from the ROM, the two decompressors' output (gzip's inflate, which
  loads the code modules and the levels, and Rare's `func_802C41C0` in
  hd_code 7F8B0), and the asm data files (`asm2x86.py` knows each
  directive's width).  What stays wrong is every place that reads bytes at
  another width than they were written at: those need fixing one by one.

**The translated code** (`RECOMP_NATIVE_ENDIAN`) fits this model: bytes,
halves and words are accessed at their own addresses, as the C does.  Its
unaligned pairs were written for big-endian memory; they are mirrored now
in that mode, so `lwl`/`lwr` loads the host-order word at an unaligned
address and `swl`/`swr` stores one (checked against unaligned native
accesses at every alignment).  That is right for both ways the game uses
them: unaligned copies (`func_802AC7DC`, `func_802AC85C`, `func_802A75DC`,
`func_802A768C`, `bzero`) and unaligned 32-bit offsets read out of level
data (`func_802A3D54`, `func_802A3DF8`, `func_802A3F80`, `func_802A4464`),
once those data are converted.  Doublewords stay two words, high word
first, which is not what a host-order `u64` is (low word first on a
little-endian host): any `u64` or double shared between the C and the asm
needs one convention.  The differential test can't check this mode as it
is, since unicorn runs a big-endian image; with a type map it can: convert
each trial's memory to host order by the map before the translated run and
back after, then compare as now.  So the map is the test's input too.

**What has to be converted: the access-width profiler.**  A
`-DPORT_ACCESS_PROFILE=ON` build (either width) records, for every byte of
RDRAM and of the host stacks, who wrote it last (the C by function, the
asm by instruction, DMA by ROM offset) and at what width, and counts every
read at another width by reader and writer (`port/host/access.c`, fed by
BEPass's `BEPASS_ACCESS` hooks, recomp.h's `RECOMP_ACCESS` and the host's
`port_be16`/`port_be32`).  A read whose bytes are then stored again at the
same width is a copy, which is fine in either order, and carries the
source's writers along.  `PORT_ACCESS=FILE` writes the table;
`port/tools/access_report.py FILE EXE` names it.  From the attract mode
(12,000 frames) and Simian Acres (6,000, `PORT_AUTOSTART=2`):

- ROM data DMA'd and read at 2 or 4 bytes: 760 (reader, segment, width)
  rows in the attract mode, 340 in gameplay, over 110 ROM segments in
  all.  Most are the uncompressed models and level
  pieces (`chbar25`, `lagrage4`, `mostep1`, ...), read by the handwritten
  engine and, as display lists and vertices, by the RSP; then the texture
  table (`func_802A08E4`, `func_802A0B34`, `func_802A0CFC`, `func_802A1074`,
  `func_802A11C4` read its records as words and halves), texture palettes
  (`func_802A5958`-`func_802A5D34`, as halves), the sound banks and
  sequence tables (libaudio's `alBnkfNew`/`alSeqFileNew`/`alCSeqNew`
  read them in place), and `reflectlogo_dl`.
- Decompressed data read at 2 or 4 bytes: 129 reader functions in
  gameplay (100 of them handwritten), 184 in the attract mode (150).
- Written at one width and read at another: 56 (writer, reader) function
  pairs in gameplay, 71 in the attract mode.  The big ones are the RSP reading `Mtx` as halves after
  `guMtxF2L` and friends wrote it as words (the renderer should read it as
  words), and the handwritten engine building vertices by storing two
  `s16`s packed in one word (`func_802ACA60`, `func_802ACC68`,
  `func_802ACCCC`, `func_8029F4B8`-`func_8029F85C`, `func_8029E938`-
  `func_8029EC68`) that the RSP and `func_802AA890`/`func_802AC8CC` read as
  halves: in host order those stores need their halves swapped, per
  instruction.  Then the game mode `D_80364A90` (a `u64` the port's own
  `video.c` reads as two words) and a few asm/C locals.

So the stages are:

1. Loaders that convert what DMA and the decompressors produce, by type
   (the inventory's `loads`), and the asm data files by directive; the
   translated code in native mode, with the packed-store sites
   (`packed`) translated as two halfword stores; the host's renderer and
   audio reading native types (`Mtx` as words, `Vtx` fields, the audio
   command list).  Checked against the big-endian build with
   `build_cmp.py` through the type map.
2. BEPass without the swaps (it keeps the loop polls and the instruction
   count), with the C's punning sites fixed under `TARGET_PC`.
3. Native LP64 structs where a struct's pointers are only the C's: those
   the asm or the ROM data share keep 32-bit fields (`PTR32`), and the
   `-m32` layout, port-ilp32 and the fixed addresses can go once nothing
   depends on them.  The first two are gone in the LP64 build ("The LP64
   build"); the fixed addresses are what's left.

### The native-endian build

Stages 1 and 2 are done for us.v11: `-DPORT_NATIVE_ENDIAN=ON` (with either
`PORT_64BIT`) builds the port with every datum in host order at its N64
offset.  The big-endian build stays the default and is unchanged by it.

**Conventions.**  Scalars are host-order where the N64 has them.  A 64-bit
scalar (`u64`, `s64`, `double`) is two host-order words, the high one
first: that is what the asm's `ld`/`sd` and the FPU's register pairs see,
so the C (BEPass rotates its 64-bit loads and stores by 32 bits) and the
asm share `D_80364A90`, `D_803649D8` and the rest without a case each.  An
`Mtx` is sixteen host-order words (element 2n the high half of word n), a
`Vtx` its fields, a display list pairs of words.  What only the RDP, the
RSP's DMA or the audio reads as bytes stays in the N64's byte order:
texels, palettes, framebuffers, samples, the microcode's data.

**BEPass** (`BEPASS_NATIVE=1`) swaps nothing but those 64-bit halves, and
keeps the loop polls and the instruction count, so both builds keep the
same time and can be compared run for run.  The N64 side is built without
the vectorizers there (`ilp32cc.py`): they raise globals' alignment, which
would move them off their N64 addresses.

**What arrives as bytes is converted by type where it arrives**
(`port/host/native.c`; `port/src/loads.c` wraps the decompressors' entry
points, `host_rom_read` reports every PI DMA; `port/tools/gen_romtab.py`
names the ROM's segments from the link map):

- the vehicles (19 sections: parts, their matrices and triangles, the
  animation tables), the models (header, vertices, triangles, animated
  textures, the effect records at 0x30/0x34, the damage states), the
  levels (header, vertices and every section the game reads at a width:
  ammo, TNT, RDUs, bounds, buildings, animated textures, the groups of
  terrain triangles, blocks, holes, train stops, 0x74, 0xA0..0xC0), their
  display lists, the attract recordings, the sound banks and sequences,
  `static_data`, the texture and model tables;
- the fields the handwritten code reads as big-endian byte pairs (the
  vehicles, the carrier, the buildings' first bytes, collision triangles)
  stay bytes;
- the save: the EEPROM file keeps the N64's bytes (`host_save_order`
  converts each block in and out), and `__osContDataCrc` is taken of them.

**The asm data files** (`asm2x86.py --native`) are typed by the inventory
(`blastcorps/include/game/inventory.json`, `symbols`): header types, and
for the handwritten objects' own data the widths their code reads it at
(`tools/inventory.py` from fieldscan's events), with `bytes` for text (C
strings; 0xFF-terminated text, `func_802BEEF0`), for the targets of byte
pointer tables (`D_80306270`), and for tables kept big-endian because a
walk runs off their end into byte data (`D_80306344`, `D_80306350`,
`D_803063D4`, `ASM_DATA_BE`).  An array of unknown length (`D_8020C070[]`)
runs to its file's end, over the labels splat made inside it.  The data
islands in hd_code's `.text` are laid out by their readers
(`7D9D0`, `800DC`, `8E910`'s per-level tables); the rest stays as its
directives say.

**The translated code** (`RECOMP_NATIVE_ENDIAN`) takes, from
`tools/recomp/native_sites.txt`, the instructions that move data at
another width than its type: `be` (the texture decoders' streams, LUTs and
texels; the chance tables), `x1`/`x2`/`x3` (a byte or half inside a wider
field: the matrix routines, the effect records' 0x2E), `h2` (a word that
is two halves).

**The C** needed three changes of its own: `func_80200714` (hd_front_end
196F0.c) tints big-endian texels, through `IMG_RD`/`IMG_WR` under
`TARGET_PC && PORT_NATIVE_ENDIAN`; the textures the C declares as `u16` or
`Vtx` arrays are put back into the N64's order at boot, and
`YoshiIcon.unk6` (declared bytes, read as `u16`s) into host order
(`port_native_fixups`, `port/src/loads.c`).  The host reads game variables
through `port_var32` and the renderer and audio through `port_g16`/
`port_g32`/`port_g16_of32` (`port.h`), which are the big-endian accessors
in the default build.

`-DPORT_EA_GUARD=ON` makes the translated code trap on any address the
port doesn't map (a pointer read from mistyped data shows up there, at the
instruction, rather than as a crash somewhere later).

**How it was checked**, against the big-endian build with
`PORT_COUNT_PER_OP=0 --deterministic`, 64-bit:

- call traces (`PORT_TRACE_CALLS`) identical: attract mode, 12,000 frames
  (23,199,462 calls); `PORT_AUTOSTART=1` and `=2`, 6,000 frames
  (7,841,424 and 7,901,391);
- `--renderer gl --scale 1` screenshots every 300 frames (40, 20 and 20)
  and `--wav` byte for byte identical, and the saves; the 32-bit native
  build the same as the 32-bit big-endian one;
- RDRAM by type at controller reads 2,000 and 5,000 of the attract mode and
  1,000 and 2,500 of each autostart: no word differs at its width but the
  game thread's N64 stack (registers spilled by the texture decoders,
  which hold word copies of texels), and one heap halfword that
  `func_80278E3C` rewrites by mistake (it passes `D_80358070`'s value to
  `func_80257490`, which rounds the stale word there up: the N64 does it
  too, in its byte order);
- the profiler's table for all three runs holds only rows known to be
  harmless: the RSP reading stale heap through segment 9, `D_803ED3B8`'s
  bytes compared with a -1 sentinel as a word, `func_8026A5CC`'s `u64`
  copy, `osContGetQuery`'s stale display-list words;
- the translated code's differential test passes 688/688.

Speed is the same: 6,000 frames take 3.94 s of user time native against
3.96 s big-endian (attract mode), 4.53 s against 4.44 s in Simian Acres,
and 4.27/4.28 s and 4.95/5.21 s in the 32-bit builds; the frame is the
renderer's more than the game's.

The TAS ("The TAS") plays on it too, us.v10's build with us.v11's
inventory, site table and island layouts (the handwritten code is the same
in both, and every site's instruction is): 57 platinum medals, with the
checkpoints restored by type.  It found two tables of texture ids the C
declared as bytes (`D_802E8BF4`, `D_802E8CB0`, now the `u16`s they are,
which the N64 build doesn't see) and a port bug of every build (the polls
ignored `osSetIntMask`, "The platform layer").  It isn't exact yet: from
the movie's read 35,680 the player drifts by a unit and the checkpoints
put the port back at the next mode change.

What's left: jp and eu build no port yet (their functions still in asm);
levels and vehicles the runs don't reach are typed from the code but
untested; and the profiler still leaves out the C's byte reads of wider
data (its byte copies, often into locals it can't see).

**What the type inventory needs to give**, machine-readable (JSON, one
file), for all of this:

- `types`: every struct shared between the C, the asm and the ROM's data:
  `size`, `align`, and `fields` as `{off, name, type, count}`, where
  `type` is a scalar (`s8 u8 s16 u16 s32 u32 f32 s64 u64 f64`), another
  type's name, `bytes` (endian-free: texels, strings, samples), `gfx` (a
  display-list word pair), or one of the address kinds: `ptr` (an N64
  address, with `to`), `romoff` (a ROM offset), `segptr` (a segmented
  address), `offset` (relative to `base`, as the level data and libaudio's
  banks hold them).  Unions list their members; unknown bytes are `bytes`
  with a note.
- `symbols`: `{name: {type, count}}` for every variable whose declared type
  isn't its real one (the `T x[1]` placeholders above, the pools and
  buffers), and for the asm data files' symbols where their directives
  aren't the type (a `.word` that is two `s16`s).
- `loads`: every place bytes arrive untyped: `{site, rom (segment or
  offset), dst, type, count, via (dma, inflate, rare), reloc}`, `reloc`
  saying which `offset`/`romoff` fields the game then turns into pointers
  (and where).
- `punning`: every datum read at two widths or as two types: `{where
  (symbol or type.field), as: [types], sites: [function or asm
  function+offset]}`, including `u64`/double shared between the C and the
  asm (which half first).
- `packed`: the asm stores (and loads) that move several smaller fields in
  one register: `{site (func+offset), fields}`.
- `asm_uses`: per type, the handwritten functions that read or write it
  and at which field offsets; this decides what stays `PTR32` in stage 3.

`access_report.py` gives the observed side of `loads`, `punning` and
`packed` with the exact sites; the inventory is the complete, reasoned
side, and each checks the other.  Stage 1 needs `types`, `symbols` and
`loads` for the ROM segments above and `packed`; stage 2 needs
`punning`; stage 3 needs `asm_uses` and the `ptr` fields.

### The LP64 build

`-DPORT_LP64=ON` (which turns on `PORT_64BIT` and `PORT_NATIVE_ENDIAN`)
compiles the game's C for the host as it is: no i386 frontend, no
port-ilp32, 8-byte pointers, and every struct laid out by the host but
where the N64's layout is shared.  BEPass still runs (the polls, the
instruction count, the u64 words).

**`PTR32`** (`T *PTR32 p`, `ultratypes.h`: clang's `__ptr32 __uptr` in this
build, nothing elsewhere, so IDO sees the same code) is a pointer that stays
4 bytes in memory, zero-extended when loaded.  The structs that keep the
N64's layout have it on every pointer field:

- what the translated code reads at the N64's offsets: `Vehicle`,
  `Building`, `TntCrate`, `UnkStruct_803ED460`, and `SchedTask`, which
  hd_code 5FD50 builds itself (the inventory's `asm_uses` misses that, so
  it isn't proof on its own; the runs below are);
- what the ROM's data, the asm data files and hd_code's islands hold:
  libaudio's banks and sequences, Rare's `SndBank`/`SndInstrument`
  (`ROMPTR` is `PTR32` now), `YoshiEntry` (hd_front_end 25070's tables),
  `UnkStruct_8036EC30` (800DC's);
- structs whose instances other files reach through labels inside them
  (`D_803156C4` is `Sched.frameCount`, the audio DMA state's, `ALHeap`'s),
  and so what those embed: libultra's `OSThread`, `OSMesgQueue` and
  `OSMesg` (a `PTR32` typedef), `OSIoMesg`, `OSTimer`, `OSTask`;
- all of libaudio and Rare's sound player: libaudio casts between its
  parameter records and allocates them at `sizeof(ALParam)`, which holds
  only in the N64's layout; its function-pointer typedefs are `PTR32` too;
- gzip's `huft` (the table pool is sized for the N64's), and the pointer
  slots the handwritten code passes to the C (the unzip's `src`/`dst`,
  `func_80278BF0`'s `out`, the sound handles of `func_80260650` and
  `SndState.unk30`), and the C's `extern`s of pointers the asm's data
  define (`level.h`'s `D_803BDAF0`..., `D_803F7654`, `D_802C4A20`).

What is native then: pointer variables, and the structs only the C uses
(`SchedClient` and six of the front end's and the game's
`UnkStruct_*`, in both versions).  `port/tools/layout_cmp.py A B` lists every struct whose
layout differs between two builds from their DWARF.  The
`SIZE_CHECK`s hold on the N64 for a native struct (`SIZE_CHECK_C`) and
everywhere for the rest.

**The variables that grew.**  `gen_ld.py` leaves a C variable to the host
linker when it no longer fits where the N64 has it (its room up to the next
sized N64 symbol, or its alignment): 120 in us.v10, pointers and the native
structs' tables, listed in `gen/port_rdram_moved.txt`.  The translated
code finds them through `SYM()` as before.  They live in the image, above
RDRAM, so what reads game memory by address takes the whole KSEG0 window:
the audio HLE (`aspmain.c`, as `gfx.c` already did) and `port_in_rdram`
(RDRAM to the image's `_end`).

**Also needed:**

- `s32`/`u32` are `int` in the port (`ultratypes.h`), `Mtx_t`, `Hilite`,
  `Gsetcolor` and `TexRect` in `gbi.h` 32-bit, `size_t` the host's;
- display lists in static data: clang won't truncate a 64-bit address in a
  constant, so `gbi.h` puts its words through `_GBI_W` (a cast through a
  32-bit pointer, which is a 32-bit relocation), and `STATIC_K0_TO_PHYS`
  the same;
- **port-lp64** (`bepass/LP64.cpp`, before BEPass): what port-ilp32 does
  that isn't layout.  Pointer arithmetic by a variable or large offset
  wraps at 32 bits, an integer made a pointer is zero-extended, accesses
  outside a variable's name go through an integer, K&R calls are repaired
  (pointer arguments and results re-zero-extended, narrow results
  widened, narrow arguments extended by the callee), calls through a
  32-bit function pointer are made through its value (clang calls through
  the `__ptr32` operand, and the backend then loads 8 bytes), and a
  `PTR32` field in a static initializer becomes the address's low 32 bits;
- BEPass's native mode swaps the words of u64s but not of the 8-byte
  pointers.

**How it was checked**, us.v10, against the native-endian 64-bit build,
`PORT_COUNT_PER_OP=0 --deterministic`, Simian Acres (`PORT_AUTOSTART=2`):
4,000 frames with the save, `--wav` and 16 screenshots identical; RDRAM at
controller reads 300 and 600 the same but for the native structs' own
layout and pointers to what moved (`build_cmp.py rdram --moved`); call
traces identical for 631,000 calls, up to an interrupt that the LP64 build
takes one call earlier at read 655.

us.v11, the same way (the LP64 build against the native-endian 64-bit one,
`PORT_COUNT_PER_OP=0 --deterministic`, `--renderer sw`):

- `PORT_AUTOSTART=2`, 4,000 frames (in us.v11 it stops at the first
  level's hint box), `PORT_AUTOSTART=1`, 6,000 frames (the first level,
  driving and the pause menus), and the attract mode, 12,000 frames: the
  saves, `--wav` and the screenshots every 250 frames (16, 24 and 48)
  byte for byte identical;
- call traces (`PORT_TRACE_CALLS`) identical over the whole runs:
  5,098,000, 7,845,941 and 23,216,880 calls;
- RDRAM (`build_cmp.py rdram --moved`) at controller reads 300 and 600 of
  `=2`, 300, 1,000, 2,000 and 3,000 of `=1`, and 2,000 and 5,000 of the
  attract mode: the same but for `D_8020E430` (hd_front_end 1A240's
  `char *[4]`, a native pointer array that still fits where the N64 has
  it, so it isn't moved; only the C reads it);
- `layout_cmp.py`: the same seven native types as us.v10;
- `--renderer gl --headless` (SDL's offscreen driver on Mesa): the
  `PORT_AUTOSTART=1` screenshots every 500 frames identical to the
  native-endian build's.

The one thing the runs found wasn't the LP64 build's: a 64-bit build of
either kind now and then failed to start, with `can't map thread stacks`
or `hardware registers ... File exists` (the brk heap, "Memory model").

What's left for it: the fixed addresses.  The image is still non-PIE at
`0x80400000` and RDRAM at `0x80000000`, which `PTR32` (and the translated
code's 32-bit addresses) depend on; Linux on x86-64 and AArch64 can do
that, macOS on arm64 and WebAssembly can't.  Getting there means the game's
variables reached by relocatable symbols only, RDRAM an arena at any base
with the 32-bit addresses relative to it, and `PTR32` a base-relative
pointer; and for the browser, threads without `ucontext`.

## The platform layer

- **Threads** (`port/host/threads.c`): each OSThread is a fiber (ucontext) on
  the one OS thread, scheduled as libultra does: the highest-priority
  runnable thread runs until it blocks, yields, or wakes a thread of higher
  priority.  The idle thread parks when it drops to priority 0.  Each thread
  has its own `recomp_context`, whose `$sp` is the N64 stack the game gave
  `osCreateThread` (only the translated code uses it).  A loop's poll
  (BEPass's `__port_poll`) can yield to the loop, and so let a higher
  thread run, which on the N64 an interrupt would; with every interrupt
  masked (`osSetIntMask(OS_IM_NONE)`) it doesn't.  The game walks its list
  of playing sounds that way while the audio thread frees from it; before
  the polls honoured the mask, the walk once reached a freed state (the
  TAS on the 64-bit build, read 28,965).
- **The loop** (`port/host/main.c`) runs when no thread can: it raises the
  events the hardware would (VI retrace at 60 Hz, timers, PI, SP and DP task
  completion) and sleeps until the next one, or until a busy thread's work
  is done (see "Timing").
- **libultra** (`port/src/ultra.c`, N64 side): messages, events, timers,
  `osGetTime` from the host clock (at 46.875 MHz), PI DMA from the ROM
  file, the controller from SDL, EEPROM to a file, no Controller Pak, VI
  swap, SP tasks, the RDP's freeze bit, AI (`port/host/audio.c`).  The
  waits libultra itself does are kept: `osContInit`'s half second after
  power-on, `osEepromLongWrite`'s 12 ms per block.  The stubs of `src/` that include only
  libultra's os/io (and libc's printf and string functions) aren't built;
  gu, libaudio and the rest of libc are.  hd_code's entry point
  (`func_802447C0`) runs on a boot thread (`port/src/boot.c`).
- **The overlay** (`port/src/overlay.c`): the front end is linked in, and
  "loading" it (`func_8028B3E0`, wrapped at link time) restores its `.data`
  from a startup copy and clears its `.bss`, which is what the N64's inflate
  leaves behind.  It runs the N64's DMA and inflate first, into the same
  memory, for the time they take.
- **Graphics**: see "Graphics" below.
- **Audio**: the game's audio thread and libaudio run, the audio task goes
  through an HLE of the audio microcode, and the AI plays it through SDL
  (see "Audio").

## Timing

How the game paces itself, and what the port does about each part:

- **Retraces.**  The scheduler (`hd_code/2C560.c`, a variant of the SDK's
  sample `sched.c`) gets a message every retrace (`osViSetEvent(..., 1)`)
  and counts them (`sc->unk284`, i.e. `D_803156C4`, which the game's own
  waits read).  The VI is raised at 60 Hz, as mupen64plus does.
- **The swap.**  When a frame's RDP work is done (OS_EVENT_DP), the
  scheduler swaps at once if a retrace has gone by since the last swap
  showed, or if `D_80364A90` (the game mode) is one of the front-end modes
  in `0xC9FD0FE79BFF80B0`; otherwise it holds the frame to the next
  retrace.  After a swap it freezes the RDP (`DPC_SET_FREEZE`) and thaws it
  at the retrace that shows the new frame.  The port keeps the freeze: a
  graphics task started while frozen runs at the thaw (drawn, then its
  SP and DP events), as on the hardware, where the RSP stalls behind the
  frozen RDP.  Drawing it at once drew into the buffer still on screen,
  which showed the new frame, the old one, then the new one again.  That is what
  keeps the intro modes at one frame per retrace (they ran about 15 times
  too fast without it) and gameplay at two.
- **The game's frame** waits for its task's done message
  (`func_80285110`), and in the levels for the texture DMAs of the frame.
  How long a frame takes is then the CPU's time, which on the port is
  charged, not spent: the translated engine counts the MIPS instructions it
  executes (the translator's `BB()` per basic block, built with
  `RECOMP_COUNT`), the game's C counts its optimised IR instructions
  (BEPass's second pass, ICount, into `__port_icount_c`), and libultra's
  own work that the port doesn't run is charged at rough fixed costs
  (`COST_*` in `port.h`, and the copies in `port/src/libc.c`).  At each
  call into libultra the running thread is charged for what it did since
  (`host_cpu_sync`, `port/host/threads.c`), at mupen64plus's CountPerOp = 2
  (42.7 ns an instruction; `PORT_COUNT_PER_OP` changes it, 0 turns the model
  off) and 1.6 MIPS instructions per IR instruction for the C
  (`PORT_C_SCALE`; IDO's `-O1` code is bigger than clang's `-O2`).  A
  thread that is ahead of the clock goes "busy": it holds the CPU against
  lower priorities, higher ones preempt it (and push its end back), and
  `--deterministic` jumps its virtual clock through it.
- **The RDP** is instant, as in mupen64plus.  The renderer's estimate of
  its time (`host_charge`) delays OS_EVENT_DP by that much times
  `PORT_RDP_SCALE` (default 0).  It used to advance `--deterministic`'s
  clock directly, which stalled every thread, VI included.
- **PI DMA** completes at mupen64plus's rate for the cartridge (a count tick
  per 8 bytes), one transfer after another.  **Audio** is paced by the AI
  (see "Audio").  `osGetTime`/`osGetCount` are the same clock; the game uses
  them for profiling and for random seeds.

Measured against mupen64plus 2.6 (rsp-hle, rice), with
`port/tools/m64p_pace.c` (a headless front end that logs the same things
the port's `PORT_PACE=FILE` does: at each controller read, the scheduler's
retrace count, the mode and the samples played) and
`port/tools/pace_cmp.py`, both from no save, `--deterministic`:

| mode (D_80364A90)            | port: retraces a frame | mupen64plus |
| ---                          | ---                    | ---         |
| N64 logo (0x10), 250 frames  | 1.01                   | 1.00        |
| Rare logo (0x20), 250 frames | 1.30                   | 1.27        |
| title and intro story (0x2)  | 2.31, 2.61, 2.43       | 2.34, 2.69, 2.47 |
| "leaders of" screens (1<<48) | 1.14, 1.11             | 1.13, 1.10  |
| world map (0x4000)           | 3.45                   | 3.51        |
| Simian Acres, driving (0x4)  | 2.00 (30 fps)          | 2.00        |

The title comes up at retrace 639 (mupen64plus: 617), and the attract
sequence's fourth mode change is at 9802 (9937).  Before, the logos took
11 retraces for their 250 frames and gameplay ran at a frame a retrace.
In real time the pace is the same, except where the host's own work (the
software renderer, above all) doesn't fit in the time the model gives it:
the intro story ran about 3% slower than `--deterministic` here.
`--deterministic` stays identical from run to run.

## Audio

- **The microcode** (`port/host/aspmain.c`): the audio task's command list
  runs at `osSpTaskStartGo`.  This game's libaudio is older than
  ultralib's (`src/libultra/audio`), and so is its microcode: the sixteen
  commands of PR/abi.h, with a linear envelope (env.c's `_getRate` is a
  straight line, the rate a step per eight samples).  ADPCM (with the loop
  state), the 4-tap resampler, the envelope mixer with its dry and wet
  sends, the mixer, interleave, load/save, DMEM moves, the reverb's
  one-pole filter.  It is written from what libaudio asks of each command
  and the SDK's description of the formats, not from another HLE (none was
  copied; mupen64plus-rsp-hle, which is GPL, was the reference it was
  compared against, only by output); the resampler's filter table is read
  from the microcode's own data in RDRAM (`D_8030EB90 + 0xD0`).
- **The AI** (`port/host/audio.c`): `osAiSetFrequency(22050)` gives the
  DAC divider libultra would (2208: 22047 Hz); the two-deep DMA queue drains
  at that rate on the same clock as the retraces and timers, so
  `osAiGetLength`, which the audio thread sizes each frame by, reads what it
  would on the hardware.  Each queued buffer goes to SDL (at 22048 Hz, at
  most 0.2 s queued) and to `--wav`.
- **Checked** against mupen64plus's own output (its rsp-hle runs the audio
  lists; `m64p_pace` is also its audio plugin and writes `audio.wav`): the
  attract music has the same RMS second by second, correlates 0.997-0.9998
  with it over half-second windows (the difference is -22 to -36 dB: a
  sub-sample timing offset and rounding), and gameplay's engine and
  destruction sounds correlate about 0.8-0.9 at a steady lag, with equal
  RMS.  Steady state the port plays 367.5 samples a retrace, what 22047 Hz
  at 60 Hz is.
- **What's approximate**: rounding in the envelope and mixer; the envelope
  state layout is the port's own (the game never reads it).  In real time
  SDL's device clock and the host clock drift apart slowly; the queue cap
  drops a buffer if the host fell far behind.

## Graphics

The RSP's graphics tasks are Fast3D (gbi 2.0D) display lists.
`port/host/gfx.c` is the front end both renderers share and the software
renderer; `port/host/gfx_gl.c` is the OpenGL 3.3 one; `gfx.h` is the
interface between them.

- **Front end** (RSP and RDP state): matrices, vertices, lighting,
  texgen, fog (into shade alpha, as the RSP does it; the game doesn't use
  it), clipping against the near plane, culling, `gSPModifyVertex` (the
  game rewrites texture coordinates with it: `G_MW_POINTS`), TMEM loads
  laid out as the RDP lays them out (the odd-row swizzle, which LoadBlock
  applies by counting `dxt` and the sampler undoes, and RGBA32 split
  across the two halves; the game's LoadBlock textures only look right with
  it), tiles, texel decoding for every format, level of detail (the tile
  pair and LOD_FRACTION from texels per pixel, which the terrain's
  mip-mapped 2-cycle mode uses), and the combiner and blender decoding.
  OS_EVENT_DP is raised only for a display list that ends in a full sync,
  as the game's scheduler expects.  The RDP's time is estimated from the
  geometry (the covered area), not from what a back end drew, so both
  renderers see the same timeline; it delays OS_EVENT_DP only when
  `PORT_RDP_SCALE` is set (see "Timing").
- **Software renderer**: draws into the game's color image in RDRAM, as
  the RDP would, and the VI shows RDRAM.  The combiner (both cycles), the
  blender (P * A + M * B over A + B, per cycle; without FORCE_BL the last
  cycle only passes the color, as full-coverage pixels do on the
  hardware), alpha compare and coverage-from-alpha, a float z-buffer tied to
  the z image (a fill rectangle into it clears it), N64 3-point filtering,
  8/16/32-bit color images.  It is the reference, and it runs headless.
- **OpenGL renderer**: the front end hands it clipped screen-space
  polygons and rectangles.  The game's two 320-wide 16-bit framebuffers
  become render targets at the internal resolution, sharing one depth
  buffer (the z image).  Textures are decoded from TMEM with the software
  renderer's own decoder, into the texels a tile can address, and cached by
  a hash of the TMEM bytes and parameters they come from; wrapping,
  mirroring, clamping, shifts, filtering and LOD are done in the shader
  with `texelFetch`, as the RDP does them, so they behave the same at any
  resolution.  Each combiner/blender mode compiles to a fragment shader
  (both cycles, alpha compare, coverage from alpha); the blender cycle that
  reads the framebuffer becomes the GL blend (ONE, SRC_ALPHA) with the
  source premultiplied, which gives (P * A + M * B) / (A + B) exactly for
  any P, M, A, B.  Triangles are batched per draw state.  The frame shown
  is the target the VI points at, scaled to the window with its aspect
  kept.
- **Where the game uses the framebuffer itself.**  Traced by write- and
  read-protecting the framebuffers between tasks (a SIGSEGV hook, not kept):
  the CPU never touches the two color framebuffers in the attract mode,
  the front end or Simian Acres.  The z image's memory (the start of
  `init`'s, `0x8021ED00`) is used by the gzip inflate as scratch while a
  level loads, which doesn't matter to either renderer.  What the game
  does render to texture is 64-wide 8-bit images at `0x803580C0`-
  `0x8035E240` (the soft shadows behind the text panels and under the
  vehicles), which it then loads as IA16 textures: the OpenGL renderer
  leaves every color image that isn't a 320-wide 16-bit one to the
  software rasterizer, in RDRAM, so those work unchanged.  A texture load
  from a GPU framebuffer (none happens) reads it back first;
  `PORT_GL_READBACK=1` also copies the framebuffers to RDRAM after every
  task, for code that reads them with the CPU.  A framebuffer the GPU never
  drew is shown from RDRAM.

Comparing the two (`--deterministic`, `PORT_SHOT_EVERY=150`, 1x, the attract
mode and 4,500 frames of `PORT_AUTOSTART=1` into Simian Acres): 0.01-0.02%
of pixels differ by more than 24 (of 255) in any channel, and the mean
difference is 2.9, which is the software renderer's 5-bit framebuffer
(1.5 with `PORT_GL_QUANT=1`, where what's left is translucent surfaces
blended with 5-bit or 8-bit memory).  Host time per graphics task (the
display list, the RSP work and the draw calls; about four tasks a frame),
Simian Acres: software 3.7 ms, OpenGL 0.42 ms, the same at 1x and 4x
since the GPU's work is asynchronous (a Radeon RX 7900 XTX).  4,500
deterministic frames take about 8 s with OpenGL at 4x against about 62 s
in software.  In real time both hold 60 frames a second; the process's CPU
use says little either way, since the game's threads spin on counters
while they wait for the retrace.

In 2-cycle mode the combiner's second cycle sees the texels shifted, as
the RDP does: TEXEL0 is tile + 1's texel and TEXEL1 the next pixel's tile
texel (this pixel's here).  The TVs' video (a 40x40 RGBA16 image the CPU
decodes each frame, drawn with TEXEL1 * SHADE in the second cycle plus
NOISE * PRIM_ALPHA static) depends on it; without it they showed tile 1.

Not done: anti-aliasing (the coverage the blender uses on edges; rendering
above 1x and scaling down is the substitute) and the VI's filters, dither,
the combiner's chroma key (its noise input is a hash, not the RDP's
generator), texgen from the look-at
vectors (the port uses the modelview's axes), the far plane (depth is
clamped instead), and triangle edges follow GL's and the software
renderer's pixel-center rule rather than the RDP's.

## The glue to the translated code

`port/tools/gen_glue.py` generates both directions from the C:

- `entry.c`: for each of the 167 translated functions the C calls, a native
  function with the C's prototype that puts the arguments where the o32 ABI
  does (`a0`-`a3`, `f12`/`f14` when the first argument is a float, the
  stack from `sp+16`, doubles and `u64`s as register pairs), calls
  `recomp_func_X` with the thread's context, and returns `v0` or `f0`.
- `externs.c`: `recomp_extern_X` for the 53 calls out, the reverse; it saves
  and restores the callee-saved registers around the native call, as the
  IDO-compiled callee would.

Prototypes come from the definition for a C function (K&R ones included)
and from the most common declaration for a translated one.
`port/tools/liveness.py` computes each translated function's live-in
registers; the generator checks the prototypes against them (no entry reads
an argument outside `a0`-`a3`/`f12`/`f14`) and lists disagreements in
`glue_report.txt`.

## Other versions

`PORT_VERSION` is `us.v11` (the default), `us.v10` or `jp` (Blastdozer).
`blastcorps/` and the translated engine hold one version at a time, so
switching means `make clean` in both directories, stages 1 and 2 for the
other version, and `make -C tools/recomp VERSION=<v>`; CMake refuses a
stage 2 of another version.  What differs between versions in the port:

- **Addresses from the version's link.**  CMake reads the heap start (the
  end of hd_code's `.bss`, `D_803FF600`: `0x803FF600` in us.v11,
  `0x803FF550` in us.v10, `0x803FF6E0` in jp) and the front end's `.data`
  and `.bss` (`src/overlay.c`) out of the version's ELFs.  The generators
  (`gen_ld.py`, `gen_syms.py`, `gen_romtab.py`, `liveness.py`, the
  replay's `n64_funcs.txt`) read the version's ELFs and link map; names
  are us.v11's in every version, so the rest of the port is the same.
  `gen_romtab.py` finds the logos by the star (`usa_star`, `jap_star`).
  The host's default ROM and its header check follow the version.
- **Compiled code that is still asm.**  us.v10's C is complete, but jp has
  17 IDO functions that differ from the US versions and are still that
  version's `GLOBAL_ASM` (`#if defined(VERSION_JP)`): the text renderer
  `func_80259EC4` and `func_8025B498`, `func_8026BCE0` (the menus), four
  in 1D990, `func_802860F0`, `func_802979E0`, and in the front end the
  pak/EEPROM thread's `func_801F57B0`, `func_801F6F18`, `func_801F7410`
  and five more.  clang ignores the pragma, so the port's C lacks them;
  `tools/recomp/config.py` finds each version's `GLOBAL_ASM` (through
  `tools/version_ifs.py`, as the N64 build resolves the conditionals,
  libultra's `gu` in 90C50 excepted: the port has its own) and translates
  each as an object of its own.  The C calls them and they call the C, so
  the glue covers them like the rest: jp has 184 entries and 118 externs.
  IDO's code needed three things Rare's doesn't:
  - **Jump tables.**  A `switch` is `jr $reg` through a table in
    `.rodata` of `L<vram>_<rom>` labels (splat marks them `glabel`).  The
    parser keeps those as labels of the function and reads the tables'
    words; the translation compares the register with each label's
    address (`pc_base + offset`, the address in the linked image) and
    jumps there (`RECOMP_TRAP_JUMP` for anything else).
  - **The FCSR.**  IDO's float-to-unsigned conversion sets the rounding
    mode to truncate (`ctc1`), does `cvt.w.s` and reads the invalid flag
    back (`cfc1`, `andi 0x78`) to handle values of 2^31 and up.  In a
    function with a `ctc1`, `cvt.w` goes through `recomp_cvt_w_fcsr`
    (`recomp.h`), which follows the FCSR's rounding mode and sets its V
    and I cause and flag bits as QEMU does.  (The first differential test
    run caught this: `func_80259EC4` took the other path.)
  - **Their `.rodata`.**  A `GLOBAL_ASM` function's `.s` carries its
    strings, floats and jump tables (asm-processor puts them in the C
    file's object); `asm2x86.py --global-asm` turns each label into a data
    section, placed at its N64 address like the rest, with a jump table's
    labels as their addresses.
  - libultra calls from them (the `gu` matrix functions with float
    arguments, `osCreateThread`, `sprintf`, `alCSPGetTempo`...) have
    their prototypes in `gen_glue.py` (`LIBULTRA_TYPED`).
- **The differential test** takes the version too:
  `make -C tools/recomp VERSION=jp test` (`RECOMP_VERSION` for
  `difftest.py`; the `.text` sizes come from the ELFs).  The snapshots are
  us.v11's (`snapshot.c` plays it), so other versions run on the random
  fills only.

jp runs as us.v11 does: headless it plays the N64 and Rare logos, the
title and the attract mode (the story, the demo levels), and
`PORT_AUTOSTART=1` goes through the name entry and the map into Simian
Acres, whose hints and pause menus show their Japanese text.  (`=2` stops
at the level's first hint: jp shows one as the level starts and waits for
A, which `=2` doesn't press once it is in a level.)  The 32-bit and 64-bit
builds are identical with `PORT_COUNT_PER_OP=0` (every 1,000th frame of
8,000 with `PORT_AUTOSTART=1`).

The native-endian build (`-DPORT_NATIVE_ENDIAN=ON`) builds and runs for jp
too, with two more pieces: IDO's code reads a narrow argument out of its
word slot (`lbu 0x5B($sp)` for a `u8` the glue stored as a word), so a
byte or half access through `$sp` to a word it or its caller stores whole
is XORed as a site (`x3`/`x2`, translate.py); and the menus' `u16` text
(`hd_code/BC8E0`, `D_803010A0` on, which only jp shows) is converted as
halves (`asm2x86.py`, `HALF_FILES`).  With those it plays the same way
into Simian Acres with the right text, but it isn't checked against the
big-endian build as us.v11's is: with `PORT_COUNT_PER_OP=0` the two part
by frame 4,000 of `PORT_AUTOSTART=1`.  The type inventory, `native_sites.txt`
and the islands' layouts are us.v11's; what jp's own data (its `_jp`
symbols, its text) needs beyond that hasn't been gone through.

## Source changes for the port

Guarded with `#ifdef TARGET_PC`; the N64 build still matches.

- `hd_code/26570.c`: `func_8026BBD0` is `void`, but its callers use the
  result, which on the N64 is `v0` left over from `func_8026BCE0`; the port
  returns that.  Two string copies whose unsequenced `a[i] = b[i++]` IDO
  evaluates with the old index.
- `hd_code/168B0.c`: the same unsequenced copy.
- `hd_front_end/196F0.c`: `func_80200714`'s texel reads and writes go
  through `IMG_RD`/`IMG_WR`, byte-swapping only in the native-endian build
  (`TARGET_PC && PORT_NATIVE_ENDIAN`; plain accesses otherwise, so IDO's
  output is unchanged).
- `tools/recomp/runtime/recomp.h`: `dmult`/`dmultu` without `__int128` on a
  32-bit host (checked against the `__int128` version); the
  `RECOMP_ACCESS` hooks for the profiler and the mirrored unaligned pairs
  of `RECOMP_NATIVE_ENDIAN`, both compiled out by default (the
  differential test passes 688/688 after them).

The 64-bit build needs no change in `blastcorps/src`.  The LP64 build's are
`PTR32`s (nothing to IDO), `_GBI_W` in `gbi.h`, and the port's `int`
typedefs in `ultratypes.h` ("The LP64 build").

## The TAS

For a check that the game can still be beaten, there is a full-game movie:
TASVideos submission #8170, WarHippy's "all platinum medals" (1:16:02,
273,723 frames, BizHawk 2.7, us.v10, from power-on with a blank EEPROM).
`port/tools/tas.sh` downloads it, plays it headless and reads the medals
from the save the game wrote (`tas_check.py`): 57 platinum, the three
slots that give no medal done, gameState 13, 360 units.  It takes about
four minutes.

It syncs only on the emulator it was made on, so the script builds that:
BizHawk 2.7's mupen64plus core (2.0 from 2013, with BizHawk's changes) and
its rsp-hle, for Linux, with `tas_bizhawk.patch` and `tas_bizhawk_compat.h`.  `m64p_tas.c` is the
front end, and the core's input, audio and gfx plugin in one.  What it
took to sync:

- **The core.**  mupen64plus 2.6 doesn't: its VI period comes from the VI
  clock, where 2.0's is `(V_SYNC + 1) * 1500` counts, about 3% apart, and
  every lag frame moves.  2.6 also randomizes PI/SI interrupt timing unless
  `RandomizeInterrupt` is off.
- **The frames.**  A BizHawk frame is one VI, and BizHawk runs two at
  power-on before the movie's first line (`N64.cs`), so line N is the pad
  from VI N+2 to N+3.  The log's four `A Up/Down/Left/Right` columns push
  the stick all the way (`N64Input.cs`).
- **MSVC's rounding.**  BizHawk builds the core with MSVC, where `fpu.h`
  defines `round` as `floor(x + 0.5)` and `trunc` through `int`.  With C99's
  functions the movie drifts out after about nine minutes.
- **No renderer.**  The gfx plugin draws nothing but sets the DP bit where a
  renderer would, at the list's `G_RDPFULLSYNC` (walking the F3D lists).
  Raising DP for every list wedges the scheduler after the Rare logo, since
  some lists don't end in a full sync.  The CPU never reads the
  framebuffers, so the renderer's pixels don't matter.

`build/tas/run/polls.csv` has, at every controller read (125,297 of them),
the pad and the VI, the scheduler's retrace count, the game's frame count,
the mode, the random number generator's state and the player's position.

### Replaying it on the port

The movie is us.v10, so this needs a us.v10 port: extract and build stage
2 for us.v10, `make -C tools/recomp VERSION=us.v10`, then configure with
`-DPORT_VERSION=us.v10` (its own build directory; `blastcorps/` and the
translated engine hold one version at a time, and CMake refuses a mismatch).
Then

```
build/port.us.v10/blastcorps --headless --replay build/tas/run/polls.csv --save run.eep baserom.us.v10.z64
port/tools/tas_check.py run.eep --platinum 57
```

`--replay` (`port/host/replay.c`, implies `--deterministic`) doesn't try to
make the port time things as mupen64plus does; it follows the port's own
frames and gives each the movie's input for that frame:

- **Pads by frame.**  A read gets the pad of the log's read at the same mode
  and frame (`D_80358064`), the first one after the last match.  The port
  leaves the title a frame early (its fade, `func_80274BF0`, reads the
  retrace count mid-frame), and matching by frame lines the next mode up
  again at its frame 0.  A read with no match gets no buttons.
- **Retraces by frame.**  Each frame gets the log's retraces for it,
  counted from the read before: the read's SI completion waits for them, and
  a VI is held once the next frame's are there.  Within the frame they come
  by the CPU model.  A VI the game waits for that the log's frame doesn't
  have is given anyway and counted.
- **The game's reads of the count.**  The game reads the scheduler's retrace
  count and level timer (`D_803156C4`, `D_803156C0`) in the middle of its
  frames: the title's fade (`func_80274BF0`), the messages' and the music's
  timing (`func_8026BCE0`), "TIME IN LEVEL".  mupen64plus's game often sees
  one to three retraces past the read there.  So `m64p_tas`
  (`TAS_COUNTER_READS=1`) logs every such read with its PC, through a read
  breakpoint in the core's debugger (`counter_reads.csv`), and on the port
  those reads are calls (`port_game.h`; `00000.c`'s `SC_FRAMECOUNT` and
  `SC_TIMER` for its reads through the struct): in a matched frame they
  return what the movie's game read in the same function in that frame.
  By function, not by order, because IDO and clang load a global a different
  number of times; within a function and a frame the movie's values agree
  in all but 18 of 27,784 cases.  The scheduler and the game's two waits on
  the count read the real one.
- **The audio thread's answers.**  The music manager and the ends of the
  results screens wait on the sequence player (`alCSPGetState`,
  `func_802D4E10`) and the sequence's position (`alCSeqGetLoc`'s
  `lastTicks`), which the audio thread moves on at its own pace.
  `m64p_tas` logs every call with its caller (exec breakpoints,
  `audio_reads.csv`); on the port the link wraps both
  (`port/src/replay_hooks.c`) and gives the caller the movie's answer for
  that function and frame, naming it by its return address in the port's
  own symbol table.  The sound itself may drift; the game logic doesn't.
- **The save thread.**  The pak/EEPROM thread (E7B0.c) changes the save
  record the game plays on (the map picks the next level from it), and it
  spins on the scheduler before it takes a command, so it gets to one a
  varying number of frames after the game sent it.  `m64p_tas` logs the
  read at which each command started (`save_starts.csv`); on the port the
  thread waits until the read after that one, unless the game waits for
  its reply (then it runs at once, as it did in the movie).  While it has
  the SI, the game skips the frame's pad read (45BB0.c, `D_8039C4B0`), so
  whether a frame reads the pad follows the movie too
  (`port_pad_read_due`).  Having got its command, the thread takes the
  completion of a pad read the game hasn't fetched (`func_8028A42C`), so
  it isn't let go while the game is receiving that completion itself
  (`host_receiving`: a thread in `osRecvMesg` on the SI's queue, blocked,
  woken or preempted at the call).  The movie's thread (priority 11, the
  game's 10) runs straight on from its wakeup and never lands between the
  game's check of `D_80370C10` and its receive; the port's, waking every
  millisecond, can, and then takes the message and leaves the game waiting
  for it forever.  That hung the 64-bit build at the log's read 58,325,
  where its CPU model put the wakeup in that window.
- **Checkpoints at the mode changes.**  The menus aren't worth making
  exact (the world map's cursor picks its target from the paths it draws,
  which depends on sound effects and more), so where the port goes its own
  way the log goes on without it: after 60 frames with no match, each frame
  moves the log on by one, and at the log's next mode change the port is
  put there.  `m64p_tas` writes the state to carry over at every mode change
  (`checkpoints.csv`: the save records, best times, level, player and random
  state); the replay writes it into the port, clears any fade and sets the
  mode as the next one, and the game's own switch (and the new mode's init,
  a level's loading included) does the rest.  Levels play exactly from
  their start, so this checks every level even where the menus between
  them don't match.  The state is the N64's bytes; the native-endian build
  writes each datum at its own width.
- **The seed.**  The random number generator (`D_8036B968`) is seeded from
  `osGetCount`, which is the port's clock: after a seeding (23C20.c,
  20460.c call `port_replay_seeded` under `TARGET_PC`) the next read sets
  it to the log's state.  So does a read after the port spent a different
  number of frames in a mode than the movie (a level's loading intro: the
  port loads faster).
- **Rounding.**  `cvt.w`/`round.w` round halves up, as the movie's emulator
  does (`recomp_round_half_up`, recomp.h); the VR4300 rounds them to even,
  and with that the first level goes elsewhere at its frame 370.
  `PORT_REPLAY_VR4300_ROUNDING=1` uses the hardware's.

What says the replay still plays the movie: in a level, every read compares
the player's position with the log's and reports the first few that differ;
the mode changes with no match, the log's reads skipped and the retraces
given anyway are reported at the end; and the save has the medals.
`PORT_REPLAY_DUMP=N,...` writes RDRAM as the read matching the log's Nth
starts (`rdram_N.bin`, big-endian), and `TAS_DUMP=N,...` makes `m64p_tas`
write the same at its Nth read, to compare the two.

Where it stands: the port beats the game with the movie's input, 57
platinum medals like the movie, in about 20 minutes (`port/tools/tas_port.sh`
after `tas.sh`).  All 125,297 of the movie's reads are matched, none
skipped, and the player is where the movie has it at every frame of every
level and on the map.  Two of the game's mode switches are the movie's
rather than the port's: twice, at the world map, the port's cursor picks
the level it is on where the movie's picks the next one (the cursor's
target comes from the paths the map draws, which the replay doesn't make
exact), and the switch hook sends it where the movie went.  The 64-bit
build plays it as exactly (all 125,297 reads matched, the player always
where the movie has it, 57 platinum), with no mode forced at all; its
timing differs (373 retraces given anyway to the 32-bit build's 80, one
save command let go early to three).

- **The mode switches.**  `m64p_tas` logs every mode the game's loop
  switches to (`switches.csv`, an exec breakpoint where it prints "game mode
  switch"); a change at the reads can be several, the game passing through
  some modes within one switch.  On the port, `port_replay_mode_switch`
  (00000.c's loop, before the new mode's init, under `TARGET_PC`) compares
  each switch with the movie's next one and, where they differ, makes it
  the movie's, with the movie's state (`checkpoints.csv`).

`PORT_REPLAY_TRACE=FROM,TO` logs every read in that range; after 50
retraces given at one read the replay dumps the threads.

The TAS also found a port bug on the way: the game writes a digit into a
string literal ("0 OF THE OTHERS", `53220.c`), which the port had in
read-only memory; `gen_ld.py` now links the game's remaining `.rodata`
writable, as the N64 has it.

## Status
## Status

- Boots, runs every thread, and plays the Rare logo, the title screen and
  the attract mode (the story sequence and the demo levels) for as long as
  it is left running; with Start/A it goes through the save-erase prompt,
  the name entry, the world map and into Simian Acres, which plays (the
  bulldozer, the carrier, the pause menu).
- Pacing matches mupen64plus to a few percent in every mode measured
  ("Timing"), and the music and sound effects play ("Audio").
- Known problems: the CPU's time is a model (instruction counts, a scale for
  the C, fixed costs for libultra), and the reference is mupen64plus, not
  the hardware: its CPU is CountPerOp = 2, its RDP instant.  The loading
  screens (the level's drop-in, 0x800) differ most: they are short and
  their frames are all loading.  The boot before hd_code (IPL3, init's
  inflate of hd_code) isn't run or charged; by the scheduler's count the
  N64 logo comes 14 retraces later than in mupen64plus and the title 22.
  Rendering: no anti-aliasing (see "Graphics").  Non-void functions that fall off the
  end (`func_8024B4B8`, `func_80271F48`, `func_8027E164`, `func_801F6160`,
  `func_801F61C8`, `func_801F6ED4`) return whatever the host leaves.

## What's left for the port

- **Timing.**  A better RDP estimate would let `PORT_RDP_SCALE` default to
  1; the C's instruction scale could come from IDO's actual code size per
  function instead of one number.
- **Real call states** for the translated code's test.  Recording `ctx` and
  memory at each translated function's entry during play would replace the
  random registers, and reach the 30% of blocks the random states don't.
- **Toward native code**: the 64-bit build is done ("The 64-bit build"),
  native-endian memory ("The native-endian build") and the LP64 build ("The
  LP64 build").  Next are the fixed addresses: the game's C and data should
  work at any base, which arm64 macOS and a WebAssembly build (both wanted
  eventually) need, and the latter also threads without `ucontext`.  The
  32-bit build still has the out-of-bounds miscompiles the 64-bit one
  avoids; typing those arrays fixes both.
- **Readable C.** Replace translated functions with hand-written C one at a
  time. Each replacement can be checked with the same harness: point the
  test library at the new C instead of the generated file.

# The handwritten asm

About a third of hd_code (us.v11 `0x56040`–`0x8E910`, 31 objects, 688
functions, 55k instructions) and hd_front_end's `0x1B100` run are Rare's
handwritten MIPS. It can't become matching C, so the port runs a mechanical
translation of it first (ROADMAP, Phase 5). This is that translator, and the
test that shows it's right.

## Approach

Our own translator over the symbolic `.s` files, in `tools/recomp/`, rather
than N64Recomp over the linked ELF:

- **Symbols survive.** Splat's `.s` already names every address (`%hi(D_…)`,
  `jal func_…`, `.L` labels), so the C says `SYM(D_803A7410)`, not
  `0x803A7410`. Its address comes from a generated header, so it follows the
  shiftable build and, later, wherever the port puts the variable. N64Recomp
  works from relocations and emits addresses.
- **It sees exactly the code we classified.** The set of objects comes from
  the `asm` subsegments in `hd_code.us.v11.yaml` (minus libultra's). It isn't
  a whole-ELF pass, so the decompiled C is never pulled in again, and the data
  islands inside `.text` are never mistaken for code.
- **Rare's code is simple to translate.** Every `jr` is `jr $ra`, and there is
  no `jalr`, no `j`, no jump table and no function pointer into it (none in
  the data, none in the islands). Control flow is branches and `jal`, which
  become `goto` and direct C calls. The only irregular spots are one
  branch into another function's epilogue (`func_8029CB04` into
  `func_8029CF04`), handled by merging those functions into one C body with an
  entry switch, and 19 branches into delay slots.
- **Small and deterministic.** It is about 1,100 lines of Python with no
  dependencies, and it gives the same output for the same input.

The instructions are decoded from the word in each line's comment
(`tools/recomp/mips.py`), not from the mnemonic text, and cross-checked
against the text. Only the symbolic operands come from the text.

## Interface

```c
typedef struct recomp_context {
    union { uint64_t r[32]; struct { uint64_t zero, at, v0, v1, a0, ... ra; }; };
    uint64_t hi, lo;
    uint32_t f[32];        /* COP1, Status.FR = 0 as the game runs it */
    uint32_t fcr31;
    int64_t budget;        /* test only */
} recomp_context;

void recomp_func_8029A800(uint8_t *rdram, recomp_context *ctx);
```

- **State.** All of it is N64 state. The registers live in `ctx`, 64 bits
  wide. Memory is reached by 32-bit N64 addresses: `rdram + (addr &
  0x1FFFFFFF)` for KSEG0 and KSEG1. The translated code never holds a host
  pointer, and one `ctx` per game thread carries `$gp`/`$sp` between calls.
- **Addresses** are `SYM(name)` (`syms_<module>.h`, generated from the linked
  ELF). A port that places data elsewhere redefines those, as long as each
  datum is still inside the arena that `rdram` points at.
- **Calls into translated code** (IDO C calls 167 of the 688 functions by
  name): set `a0`–`a3` (and `$sp` into the arena, since the functions build
  frames there) in the thread's context, call `recomp_func_X(rdram, ctx)`,
  and read `v0`. Many of these functions also take arguments in
  `v0`/`v1`/`t*`/`s0`, and the wrapper for each one has to set those, so the
  wrappers are hand-written as the C callers get decompiled. No function
  pointer reaches this code, but `recomp_lookup(addr)` (`funcs.c`) resolves
  one if that ever changes.
- **Calls out of translated code** (53 callees: IDO C and libultra) go
  through `recomp_extern_<name>(rdram, ctx)`. The generated default
  (`externs.c`) forwards to `recomp_call_external(rdram, ctx, SYM(name))`.
  The port replaces each one with glue that unpacks `a0`–`a3` (and stack
  arguments at `sp+0x10`), converts address arguments with `rdram + (a &
  0x1FFFFFFF)`, calls the native function, and puts the result in `v0`.
- **Hooks the embedder provides:** `recomp_trap` (break/syscall, which are
  Rare's asserts, plus faults in the test build), `recomp_call_external`, and
  `recomp_mfc0`/`recomp_mtc0` (one Status read and one Compare write).
- **Memory layout.** By default memory is a byte-exact big-endian image (what
  the ROM's data is), and every access byte-swaps on the host. That is what
  the test verifies. `-DRECOMP_NATIVE_ENDIAN` switches to host-order words for
  a port whose native C shares those structs. Words, halves and bytes then
  read the same from both sides, but `lwl`/`lwr`/`swl`/`swr` (32 uses) and
  anything that reads one datum at two widths would need checking. Doubles
  and `ld`/`sd` are two words, high word first, in both modes.

How the PC build meets this interface (RDRAM at `0x80000000`, big-endian
memory, the glue generated from the C) is under "Memory model" and "The
glue to the translated code" above.

## Semantics worth knowing

These are the choices where the VR4300, the MIPS spec and QEMU disagree, or
where the obvious C is wrong:

- **`sra`/`srav`** shift the whole 64-bit register and sign-extend the low
  word. Rare does this after `dmult`/`mflo`, so it matters for fixed point.
  QEMU leaves such inputs unextended; the test build follows QEMU there
  (`RECOMP_QEMU_COMPAT`).
- **FR = 0.** Doubles and 64-bit integers are even/odd pairs, and the code
  builds a double with `mtc1 $at, $f9`. The L-format conversions (`cvt.s.l`,
  `cvt.d.l`, `cvt.l.s`, `cvt.l.d`, 44 uses) are legal on the VR4300 with FR =
  0. QEMU rejects them, so the test harness executes those itself.
- **Float to int** out of range or NaN gives `0x7FFFFFFF`/`0x7FFF…F`. `cvt`
  rounds to nearest-even (the FCSR mode the game leaves), with
  `-ffp-contract=off` so nothing fuses.
- **NaNs** follow MIPS legacy encoding: a quiet NaN operand propagates, and a
  signaling NaN or an invalid operation gives `0x7FBFFFFF`. `abs`/`neg`/`mov`
  only touch bits. The FCSR's flag and cause bits aren't kept; nothing but
  libultra's thread switch reads them.
- **`add`/`addi`/`sub`/`dadd`** trap on overflow in the test build and wrap
  in the port. The game never overflows them, since that would crash on
  hardware.
- **Divide by zero** gives the VR4300's result (`LO = n<0 ? 1 : -1`,
  `HI = n`), or QEMU's in the test build. The code only reaches it on paths
  that then `break 7`.

## Running it

```
make -C tools/recomp                  # translate -> blastcorps/build/recomp/src
make -C tools/recomp port             # compile it as the port would (-Werror)
make -C tools/recomp snapshots        # optional: RDRAM from real play (mupen64plus)
make -C tools/recomp test PYTHON=.env/bin/python TRIALS=100
```

These need a built stage 2 (us.v11 by default, `VERSION=` for another),
because the translator reads `blastcorps/asm/` and
`blastcorps/build/*.elf`. The test needs `numpy` and
`unicorn` in the venv. `tools/recomp/analyze.py` prints the opcode
inventory and any control flow the translator would refuse (there is none
now).

### The differential test (`tools/recomp/test/difftest.py`)

Each translated function runs many trials, and each trial does this:

1. Build 4 MB of RDRAM from the linked `hd_code`, `hd_front_end` and `init`
   images, plus one of these fills:
   - one of 8 seeded random fills, mostly zeros, small integers, floats and
     pointers back into RDRAM;
   - one of 16 snapshots of real play. `test/snapshot.c` is a headless
     mupen64plus front-end that is its own scripted input plugin. It plays
     through the title, the name entry, Simian Acres, the pause menu and the
     world map. A snapshot keeps the linked `.text` on top.
2. Randomize the registers the same way. Registers the function
   dereferences before writing them get pointers, taken from pointer-looking
   words of the snapshot when there is one. Words that the code loads from a
   symbol and then dereferences are also given pointers.
3. Run the original in unicorn (MIPS64 big-endian, R4000 model, FR = 0) until
   it returns, traps, or uses up its budget of 400k instructions.
4. Reset memory and run the translated C, built into a shared library with
   `RECOMP_TEST`, on the same state. Calls out of it run the original callee
   in unicorn on the same buffer. So everything that isn't translated
   executes identically in both runs, which is also why `hd_code`'s C doesn't
   need to be available as C here. A second pass (`--stub-externs`) makes
   every callee return a hash of its arguments in both runs instead, which
   gets further into code whose callees fault on random state.
5. Compare all GPRs, HI/LO, all 32 FPR words, the FCSR condition bit, every
   byte of RDRAM, and how the run ended (return, break, syscall, overflow,
   or address fault). Code pages are write-protected in both runs, so
   random pointers can't rewrite instructions that only the original would
   then execute.

A trial where either side hit the budget is inconclusive. So is one where
the random state overwrote the saved `$ra` (the translated code refuses to
return anywhere but its caller). `--trace FUNC:TRIAL` with the
`librecomp_trace.so` build (`make trace-lib`) replays one trial with a
per-instruction register trace on both sides and prints where they diverge.

### Results (100 trials per function, with snapshots)

- **688/688 functions pass.** Across both passes, 0 of about 137,000 trials
  differ.
- **jp: 706/706 pass** (its 689 handwritten functions and the 17 IDO
  ones, random fills only), 0 of 141,200 trials differing; 490 functions
  returned normally in a trial, 517 with the callees stubbed; 53.3% of
  the blocks and 60.4% of the instructions were executed.  The 17 alone,
  at 200 trials: all pass, 14 returned normally.
- 548 functions returned normally in at least one trial, and 584 when
  callees are stubbed. The other 104 only ever faulted or trapped,
  identically on both sides, which still compares state up to the fault.
  100 of those dereference a register or global that neither the random
  state nor the snapshots make valid: 62 null or small-integer pointers and
  38 other bad addresses. Of the last four, one always overflows, one
  always faults on alignment, one always runs past the budget, and one is
  an assert (`syscall`, hd_front_end's `func_80202100`) that always fires.
- Coverage of the translated code, over both passes: 69.4% of basic blocks
  (6385 of 9203) and 75.7% of instructions executed at least once. All 102
  opcodes the code uses are executed.
- Mutation check: of 11 bugs planted in the runtime or the translator (slti
  signedness, sra as logical, `lb` zero-extending, `cvt.w.s` truncating,
  mult HI/LO swapped, dmult HI dropped, `c.le` as `c.lt`, `lwr` shift,
  `swl` bytes, branch condition read after the delay slot), the test catches
  10, each with 2 to 188 failing functions. The eleventh, `lwl` keeping the
  wrong low bytes, is masked by the code: every `lwl` is paired with an
  `lwr` that overwrites those bytes.

What this can't test:

- Code that reads hardware or needs another thread (libultra's message
  queues) only gets as far as its first fault in these runs.
- The other 30% of the blocks were never reached. Getting there
  needs realistic arguments, not only realistic memory.
- Undefined-behaviour cases where QEMU and the VR4300 differ (`sra` on
  unextended values, divide by zero) are checked against QEMU's behaviour in
  the test build. The port build's VR4300 behaviour for them is not
  exercised.
