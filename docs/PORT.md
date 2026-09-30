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
("The LP64 build"); `-DPORT_MOVABLE=ON`, with any of them, puts game
memory wherever the host allocates it and links an ordinary PIE
("Movable memory"), which is the build for macOS (untested there: "Other
hosts", "macOS", and `port/tools/macos_build.sh`):

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
bilinear one, or point sampling throughout), `--interpolate` (OpenGL:
60 frames a second where the game draws 30, see "Frame rate"), `--aspect
W:H`, `--widescreen` (16:9) and `--aspect window` (see "Widescreen"),
`--wav PATH` (everything the game plays, at the AI's rate) and `--no-audio`; sound goes
to SDL unless the run is `--headless` or `--deterministic`.
`PORT_AUTOSTART=1` taps Start and A, which is enough to get from the title
through the name entry into Simian Acres (`=2` taps by the game's own
retrace count and then drives forward in the level, as
`port/tools/m64p_pace.c` does in mupen64plus; `=3` also taps A in the
level, which clears the hint panels, so the vehicle keeps moving);
`PORT_DUMP=N,...` writes RDRAM at
the Nth controller read, counted as `tools/recomp/test/snapshot.c` counts
them in mupen64plus, so the two can be compared byte for byte.
`PORT_SHOT_EVERY=N` saves a screenshot every N frames (at the internal
resolution with OpenGL; `PORT_SHOT_FROM=M`: from the Mth on); `PORT_GL_QUANT=1` makes the OpenGL renderer write
5-bit color, as the RDRAM framebuffer holds it, and `PORT_GL_READBACK=1`
copies its framebuffers back to RDRAM after every task (see "Graphics").
`PORT_PACE=FILE` logs the pacing at every controller read (see "Timing").
`PORT_THREADS=ucontext|pthread` picks how the game's threads switch
(`-DPORT_THREADS=` sets the default; see "Threads without ucontext").

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
symbol the port can relocate.  (That is the movable build, "Movable
memory"; macOS takes it: "Other hosts", "macOS".)

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

## Testing the port

`port/tools/test.py` runs the checks above as a suite, and each build
directory has them as CTest tests:

```
ctest --test-dir build/port.us.v10 -L quick -V    # about a minute
ctest --test-dir build/port.us.v10 -L tas -V      # the TAS, 10-20 minutes
ctest --test-dir build/port.us.v10 -L recomp -V   # tools/recomp's difftest, a few minutes
port/tools/test.py variants [--tas]               # the standard variants, built and checked
```

or `test.py quick BUILD...`, `test.py tas BUILD...` (several at once, in
parallel), `test.py recomp`.  Every line says PASS, FAIL, XFAIL (a known
failure, below) or SKIP; the exit status is 1 on any FAIL, 77 (CTest's
skip) when nothing could run: no ROM, no TAS log (`build/tas/run/polls.csv`,
`port/tools/tas.sh`), no venv with numpy and unicorn.  The outputs stay in
`BUILD/test/` for a closer look.

**quick.**  Deterministic runs (`PORT_COUNT_PER_OP=0 --deterministic`,
`--headless`, the software renderer, no save to start from): the attract
mode for 4,000 frames and `PORT_AUTOSTART=1`, `=2` and `=3` for 3,000,
2,000 and 3,000, each with `--save`, `--wav` and a screenshot every 250
frames.  Their hashes (16 digits of sha1) are compared with
`port/tools/test_refs.json`'s for the build's version; with `--against
BUILD` also with another build's last results.  Within the build, where
the executable is the same, `=3` again must give the same:

- with the other thread backend (`PORT_THREADS`): every hash;
- with `--widescreen`, with `--renderer gl --scale 1`, and with
  `--renderer gl --interpolate --widescreen` (SDL's offscreen driver; left
  out without libepoxy): the save and the sound.

What's exact: with the timing taken out, every variant plays the same game,
so the save and the sound are the same in all of them, and so are the
screenshots, except where the references say otherwise:

- **layout-dependent** (`layout`): the hint box's portrait static at
  frame 1500 of `=2` and `=3` differs between executables (the software
  renderer's static samples memory that holds addresses of the image; see
  "Threads without ucontext").  It is compared only within a build; the
  OpenGL renderer draws it the same everywhere.
- **known failures** (`known`, by variant): the build passes as XFAIL
  while it gives exactly the hashes recorded there, and fails on anything
  else; when the failure goes away it passes with a note to take the
  entry out.  `test.py quick BUILD --known NOTE` records a build's
  differences as one.

`test.py quick BUILD --update` writes a build's hashes as its version's
references; take them from a build that is right (a 64-bit big-endian one
now), after a change meant to change what the game draws or plays.  There
are references for us.v10 only (the version the TAS is), and other
versions still get the comparisons within the build.

**tas.**  The replay of "The TAS", each build in its own copy of the
executable: `tas_check.py`'s 57 platinum, and the replay's report: all of
the log's reads matched, none skipped, no mode forced, and the save the
reference's (every exact build writes the same one).  A variant with a
known drift (`test_refs.json`'s `tas`) passes as XFAIL while its matched,
skipped and forced counts and its save are exactly the known ones.  The
retraces given anyway are printed but not checked (they follow the CPU
model's timing: 78 in the 32-bit build, 123 in the 64-bit one, 384 in the
movable one).  `--again` checks the last replays' results again without
running them.

**variants.**  Configures and builds, from the stage 2 in `blastcorps/`
(us.v10 by default, `--version`), the standard set into `build/test-*/`:
`32`, `64`, `n64` (native-endian 64-bit), `lp64`, `m64` (movable 64-bit),
`mlp64` (movable LP64) and `mn32` (movable native-endian 32-bit); runs quick
on all of them at once, then prints a table of every variant's scenarios
against the references; `--tas` then replays the TAS on all seven in
parallel.  `--no-build` uses the directories as they are, `--only` picks
variants.  `test.py table BUILD...` shows every hash of some builds' last
quick runs side by side.

Where it stands (us.v10, main at the time of writing): quick passes in
all seven, with these known failures:

- **32-bit builds (`32`, `mn32`): the glue's narrow results.**  The
  translated engine calls three of the game's C functions that return
  `u8` (`func_802794F0`, `func_8026FE6C`, `func_80286090`), and
  `gen_glue.py`'s `externs.c` declares them `uint32_t`.  i386 returns a
  `u8` in `al` and leaves the rest of `eax` as it was (`func_802794F0` is
  `cmpb ...; setne %al; ret` after an address computation in `eax`), so the
  engine sees a nonzero result where the function returned 0.  The 64-bit
  builds are right, since port-ilp32 widens narrow results in the callee;
  the movable build's call repair covers only the game's C calling itself.
  In the attract mode's demo the engine (71140.s) then skips
  `func_80278EB0` from the 1,563rd controller read (retrace 2,676) on:
  `D_802FBEE0`'s vertices stay zero, an explosion is drawn otherwise, and
  the sound differs later.  Declaring the narrow type in `externs.c`
  (`narrow_ctype`, as `entry.c`'s wrappers already do) makes the 32-bit
  attract mode the 64-bit build's, hash for hash.
- **Native-endian builds (`n64`, `lp64`, `mlp64`, `mn32`): the carrier on
  the globe.**  From retrace 1,228 to 1,285 of every `PORT_AUTOSTART`
  run (the world map's intro), about 22 pixels of the carrier over the
  globe come out slightly different in color, with both renderers; the
  save and the sound don't change.  Not tracked down.

The TAS: `32`, `64` and `m64` exact (all 125,297 reads matched, none
skipped, no mode forced, the same save); every native-endian build
(`n64`, `lp64`, `mlp64`, `mn32`) drifts the same way, from the log's read
38,289 (level 29) on: 121,304 of the reads matched, 3,993 skipped, 11
modes forced, still 57 platinum, and the same save in all four (a known
drift).  Seven replays at once take about 25 minutes on a 32-thread
machine.

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
  `SndState.unk30`), the pointer arrays the front end's model loader
  (hd_front_end 1B100's `func_80202100`, a word per slot) fills and
  `func_80202270` reads (00000.c's `D_80210E90`/`EE0`/`F78`, DE70.c's
  `D_80218350`..`60`; the TAS's first front-end model, at read 1,620,
  crashed on them), and the C's `extern`s of pointers the asm's data
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
- no over-alignment on declarations either: x86-64 clang gives an
  `extern` array of 16 bytes or more 16-byte alignment, as it does a
  definition, and the optimiser then clears the low bits of addresses
  made from it; BEPass puts both at the type's own alignment (the TAS's
  `&D_802F49F4[i]`, at ...944, lost its 4 and divided by zero);
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

## Movable memory

Every build so far keeps the N64's addresses as host addresses: RDRAM is
the image's `.rdram` section at `0x80000000`, `gen_ld.py`'s linker script
puts each game variable and each asm data file at its N64 address, the
rest of the image follows at `0x80400000` (non-PIE), the fibers' stacks
are mapped at `0x90000000` and the hardware registers at `0xA4000000`.
The host linker fills a `.word sym` with the host address, which is the
N64 one only because of that map, and the translated engine, the renderer
and the audio HLE use N64 addresses as pointers.  macOS on arm64 can't do
any of it (executables are PIE, ld64 has no linker scripts, the low 4 GB
are `__PAGEZERO`), and neither can WebAssembly (no linker scripts, no
fixed mappings, 32-bit pointers into one linear memory).  This is the
design for a build where game memory is wherever the host puts it and the
game still sees N64 addresses, and the build that does it, as far as it
goes ("The movable build", below).

### The model

**Pointer values are N64 addresses; memory is an arena at any base.**  On
the N64 side (the game's C and `port/src`), every pointer the code holds,
compares, masks with K0/K1, stores or hands to the asm is the N64
address, exactly as now.  Only the accesses change: a load, store or
memory intrinsic through `p` goes to `arena + (p & 0x1FFFFFFF)`.  The
translated engine already works that way (`HOST(a)`), and so does the
host (`port_ptr`); the C joins them.  The other direction, for what the
host allocated inside the arena (the fibers' stacks), is
`0x80000000 | (h - arena)`.

The alternative is host pointers in registers and N64 addresses in
memory, converting where a pointer becomes an integer, a `PTR32` or a
word of game memory (at `ptrtoint`, `inttoptr` and `addrspacecast`).
That is what native C will be one day, but not before the data and the
asm are typed: `(u32)p` and back has to round-trip for segment addresses,
sentinels and KSEG1 addresses (so the map can't mask, so a KSEG1
dereference misses), NULL has to stay 0 in both directions (a select at
every conversion), and every address the asm writes as a word and the C
reads as an integer has to go through a conversion someone found.  With
N64 values throughout, none of that is a question: there are no
conversions in value space, so the C's arithmetic on addresses is the
N64's by construction.  What it costs is an `and` and an add per access
(the base in a register; a variable's address folds: `ldr w0, [x8,
#0x364A94]`), as the translated engine already pays.  Low mappings
(`MAP_32BIT`, a small `__PAGEZERO`, x32) don't exist on arm64 macOS or in
the browser.

**What lives in the arena**, by physical (N64) address:

- `0x000000-0x3FFFFF`: RDRAM, as now: every N64-named variable of the C
  and of the asm data at its N64 address, the heap, the framebuffers, the
  level pool, the boot globals at `0x80000300`.
- from `0x400000`: the N64 side's data that has no N64 address but whose
  address escapes (string literals passed on, `__func__` for
  `port_counter`, `port/src`'s queues, threads and boot stack), and in the
  LP64 build the variables that outgrew their N64 room.  About 150 KB in
  the 64-bit build, less in LP64.  What never escapes (the switch tables,
  the front end's startup copy of its `.data`, constants only copied from)
  stays in the image.
- the fibers' host stacks, so that a local whose address the C passes on
  has an N64 address.  (The main loop's calls into the N64 side,
  `port_irq_*`, have no escaping locals; `n64()` of an address outside
  the arena traps in a checking build.)
- nothing else: the hardware-register window goes (neither the C nor the
  translated code reads it; `PORT_EA_GUARD` traps what's outside the
  arena).

About 21 MB with today's 16 stacks of 1 MB, less with smaller ones.
Code, the host's data and the ROM stay where the host has them.

### The game's C: one pass over the whole program

The front half of the N64 side's build stays per file and parallel
(clang `-O2` with port-ilp32/port-lp64, BEPass and ICount, as now), but
ends in bitcode.  Then one "arena link": `llvm-link` of the N64 side (the
game's C, `port/src` and the asm data, below), `opt -passes=port-arena`,
`llc`, one object.  Measured on us.v10's N64 side: `llvm-link` 0.5 s,
`llc -O2` of the one module 1.7 s (`llvm-split -j8` then parallel `llc`:
0.3 s), so it costs nothing next to the front half.  The pass needs the
whole program: per file it can't tell a host function from a translated
one or from another file's C, nor place the data without N64 names that
other files reference; the per-file alternatives (a fixed window of the
arena per file; a per-file base loaded at run time and relocations at
startup) are more machinery for less.

What port-arena does, in order:

1. **Resolve every global.**  An N64-named one (the table `gen_ld.py`
   already builds from the version's ELFs) is at its N64 address; the
   escaping rest is laid out from `0x80400000`; an undefined name the
   N64 link resolves through its symbol files (ROM segment starts, names
   inside islands: `gen_syms.py script`'s table) is its constant.  Every
   reference becomes the constant (`inttoptr`), and any data symbol left
   undefined is an error.  This replaces `gen_ld.py`'s script and check,
   `port_syms.ld` and `port_fixed.ld` (`D_803FF600` is a constant).
2. **Build the arena image.**  The initializers of the arena's globals,
   serialized at their addresses in the build's byte order (big-endian:
   swapped at build time), pointers as N64 values, functions as their N64
   addresses: one constant blob with no relocations, copied into the
   arena at startup like a cartridge's boot.  BEPass's `__bepass_fixup`
   tables and `port_bswap32` go with it.
3. **Map the accesses.**  Every load, store, atomic and memory intrinsic
   through a pointer that isn't a host-resident global or a local gets
   `arena + (p & 0x1FFFFFFF)` (after ICount, so the timing is the same).
4. **Locals.**  An alloca whose address escapes becomes `n64()` of its
   host address (it is on an arena stack); one used only by loads and
   stores stays as it is.  `byval` arguments (x86-64 passes big structs
   by a hidden copy) are host pointers: the callee copies an escaping one
   into its frame, the caller maps what it passes.
5. **Functions as values** become N64 addresses (the game's functions
   their vram; the port's own address-taken ones, `boot_entry` and the
   like, a reserved range), and an indirect call goes through
   `port_fn(addr)`, a table the pass emits (33 address-taken functions
   and 52 indirect calls in us.v10: libaudio's handlers, thread entries,
   callbacks).  The host starts a thread through an N64-side trampoline,
   so it never calls a game function itself.
6. **What the host linker did.**  The `--wrap`s (ld64 has none) are done
   by renaming in the module; the replay hooks' `__builtin_return_address`
   (WebAssembly has none) becomes a caller id the pass passes.
7. **Calls whose type isn't the callee's.**  The decompiled C's K&R
   declarations disagree between files: 405 direct calls to 70 functions
   in us.v10 (`s32 f()` called for a `u8 f()`, a `u8` parameter passed an
   `int`, a pointer passed where the definition has an `s32`), plus 19 to
   translated functions and a variadic declaration of a function defined
   without (`func_8029A7E4`).  x86-64 and AArch64 run them (port-lp64's
   rules make them behave); WebAssembly can't call a function through
   another type at all (the backend makes a trap).  The pass rewrites
   each to the definition's type, converting the arguments as the N64's
   registers carry them, and the glue's prototypes are the declarations
   for the translated code.
8. **Pointer arguments to the host** are an error: the host's interface
   takes N64 addresses (`uint32_t`), as most of it already does.  What
   doesn't yet: `host_input`, `host_save_order`, `libc64.c`'s copies and
   `sprintf` (whose `%s` arguments are N64 addresses too),
   `port_counter`'s `__func__`.  (As built, the pass maps those
   arguments at the call instead, by the functions' names, and the host
   keeps its pointers: less to change, and just as portable.  Only a
   pointer the host keeps and hands back, `host_thread_create`'s, is left
   as it is.)

Nothing in `blastcorps/src` changes for this; `port/src` and the host
interface do.

### Pointers in memory

- **ILP32 layouts** (the 64-bit build through port-ilp32, the 32-bit
  build, and WebAssembly natively): a pointer in memory is 4 bytes and
  holds the N64 address.  Nothing changes, and nothing moves: every
  N64-named variable keeps its N64 address.
- **LP64**: `PTR32` fields hold the same 4-byte N64 value as now; native
  8-byte pointer fields hold it zero-extended.  The moved variables get
  addresses after RDRAM, and the translated code reads their `SYM()`s
  from a header the arena link writes (the translated code is compiled
  after it).
- **wasm32** is ILP32 natively: the i386 `-malign-double` layout the
  32-bit build uses is wasm32's (4-byte pointers and `long`s, 8-aligned
  `u64` and `double`), and `PTR32` is empty there as in the 32-bit build.
  So the WebAssembly build is the 32-bit native-endian build compiled for
  wasm32, plus movable memory.  Its Linux proxy exists: the 32-bit
  native-endian build (`-DPORT_NATIVE_ENDIAN=ON` without `PORT_64BIT`)
  builds and runs, and does with `PORT_MOVABLE` too (below).
- In the browser the arena can be linear memory itself: with emscripten's
  `GLOBAL_BASE` above the arena (and its stack in linear memory, below
  512 MB), an N64 address is `p & 0x1FFFFFFF` and every host address in
  linear memory has an N64 alias, the C stack's included; the map is one
  `and`, with the constant folded into the load's offset.

### The translated engine

- `rdram` is the arena, `HOST(a)` is `rdram + (a & 0x1FFFFFFF)` as in the
  differential test's build, and `SYM(name)` is the N64 address from
  `syms_<module>.h` (the ELF's), so `port_recomp_syms.h` goes; LP64's
  moved names come from the arena link's header.
- Jump tables: `pc_base` is `SYM(func)`, the vram, and the tables hold
  the labels' vrams, which the asm data's conversion (below) writes.
- The glue passes and returns N64 values already: no change.
- **The asm data files** become LLVM IR instead of x86 assembly
  (`asm2x86.py`'s parser and typing, a new writer): per file a global of
  byte runs and `ptrtoint (ptr @sym to i32)` words, the inner labels as
  aliases into it, which port-arena resolves like the C's.  Mach-O's `as`
  and wasm have no use for the x86 directives, and the arena image needs
  them as data anyway.  The same goes for the islands and the jp
  `GLOBAL_ASM` rodata.

### The host side

- `port_ptr(a)` is `arena + (a & 0x1FFFFFFF)`; the dumps (`PORT_DUMP`,
  `PORT_REPLAY_DUMP`, the crash dump) go through it and keep their format.
- Game variables the host names (the replay's checkpoints and counts,
  `PORT_PACE`, `PORT_AUTOSTART`) by their N64 addresses, from a generated
  header, not by linking to the game's symbols (`PORT_VAR`, `n64_syms.h`).
- The fibers' stacks come from the arena (`threads.c`); `map_fixed`, the
  `ADDR_NO_RANDOMIZE` re-exec, `-Ttext-segment`, `--no-pie` and the
  linker scripts go: the port links as an ordinary PIE.
- The renderer, the audio HLE and the loaders (`native.c`) already reach
  game memory through `port_ptr`; `port_in_rdram` becomes the arena's
  range.
- The access profiler's `RD_BASE` and stack ranges become arena offsets;
  `build_cmp.py rdram`'s exclusions of "addresses in the port's image or
  on the host stacks" become the arena's extra data and stack ranges (and
  there are fewer: a pointer word is an N64 value in both builds);
  `layout_cmp.py` doesn't care.  (Not done yet: the movable build refuses
  `PORT_ACCESS_PROFILE`, and `build_cmp.py rdram` against it shows the
  stacks' addresses, which it leaves in.)
- The replay names the functions behind the counts by caller id (above)
  where it now looks the return address up in the port's symbol table.

### The movable build (`-DPORT_MOVABLE=ON`)

The plan below is done, behind the option; the default builds are as
they were.  The movable 64-bit build is an ordinary PIE: nothing of the
port is at a fixed address, and ASLR moves the image and the arena every
run.  It takes `PORT_64BIT` (big-endian or native), `PORT_LP64`, or
neither (the 32-bit build).

**The arena** is one `mmap` of 28 MB (`port_arena_init`, `host/runtime.c`,
before anything else runs), at an offset into its page
(`PORT_ARENA_OFFSET`, default `0x5670`, so nothing can rely on more than
16-byte alignment).  It holds N64 physical memory from 0, and an N64
address `a`, KSEG0 or KSEG1, is at `port_arena + (a & 0x1FFFFFFF)`
(`port_arena.h`):

- `0x000000`: RDRAM, every N64-named variable of the C and of the asm data
  where the N64 has it;
- `0x400000`: the N64 side's data that has no N64 address (string
  literals, statics, the switch tables, `port/src`'s own, which keeps
  its own place even where its names are N64 ones: `osViClock`,
  `D_803FDF60`): about 70 KB in us.v10;
- `0xC00000`: the fibers' host stacks, 16 of 1 MB (`PORT_STACK_BASE` is
  `0x80C00000`; `host_thread_stack()` is where a threads backend gets
  one).

**The arena link** (`CMakeLists.txt`): the N64 side (the game's C and
`port/src`) is compiled per file as before, to bitcode; the asm data
files become LLVM IR (`tools/asm2ll.py`, from `asm2x86.py`'s output: each
file a global of byte runs and its symbolic words as `ptrtoint`s); then
`llvm-link`, `opt -passes=port-arena` (`bepass/Arena.cpp`) and `llc`
make one object.  port-arena, over the whole program:

- resolves every global: a definition with an N64 name in RDRAM (the
  version's names: `gen_syms.py table`, from the three ELFs) is at its
  N64 address, every other definition is laid out from `0x400000`, and a
  declaration with an N64 name (a variable of the asm or of another
  module, a name inside a data island, a ROM offset, a fixed address such
  as `D_803FF600`) is its value; every reference becomes the constant.
  What is left is the host's (`__port_icount_c`, `port_ints_masked`...),
  accessed where it is;
- writes the arena's initial contents from the initializers, in host
  order, then swaps what BEPass's `__bepass_table`s list (the C's
  scalars in the big-endian build, the words of 64-bit ones in the native
  one) and drops the tables and their constructors: the image has the
  build's byte order, with the asm data's symbolic words in memory order.
  It goes into the image as runs of non-zero bytes (`__port_arena_runs`),
  which `port_arena_init` copies; a word it can't resolve (a pointer to
  the host's data) would be left to startup (`__port_arena_relocs`), and
  there are none;
- maps every load, store, atomic and memory intrinsic, the pointer
  arguments of the host's functions (by name: `host_*`, the `n64_*`
  copies and `sprintf`, `port_counter`; `host_thread_create`'s argument
  is handed back to the thread, so it isn't) and a struct passed by value
  (the call copies it from its host address), to `port_arena + (p &
  0x1FFFFFFF)`, unless it is a local that doesn't escape or the host's;
- gives an escaping local (an alloca, or a struct passed by value, whose
  address goes to a call, into memory or into an integer) its N64 address
  on the fiber's stack, and stops the port (`port_arena_bad_local`) if it
  isn't on one: 369 in us.v10.  (Not in a variadic function, whose
  `va_list` is the host's.);
- makes a function used as a value its N64 address, the game's by its
  vram and the port's own from `0x7F000000`, and a call through a value a
  call through `port_fn(address)` (the pass's `__port_fns`, `runtime.c`):
  30 functions and 50 calls in us.v10.  The host calls a thread's entry
  the same way (`PORT_FN`, `threads.c`);
- names the caller of the replay's hooks (`port_replay_set_caller`, which
  `replay.c` uses instead of looking the return address up in the port's
  symbol table, as the other builds do);
- first of all, makes every direct call with its callee's type: the
  decompiled C's K&R declarations disagree between files (449 calls in
  us.v10's 64-bit build, 417 in the 32-bit one).  The arguments are
  truncated or extended as the N64's registers would carry them (a
  missing one is zero), a variadic callee gets the rest as they are
  (34430.c declares `func_8029A7E4` with four parameters), and the result
  is converted by the callee's extension.  x86-64 and AArch64 ran them
  anyway; WebAssembly can't call a function through another type, and
  i386 got narrow results wrong (below);
- in the LP64 build, lays out after RDRAM the C's variables that outgrew
  their N64 room (as `gen_ld.py` decides it: bigger than the N64 symbol's
  size and the distance to the next, or aligned where it isn't: 115 in
  us.v10, its native pointer variables and tables), and writes where they
  went as a header (`gen/arena_syms.h`: `SYM_` for the translated code,
  which reads seven of them, `PORT_N64_` for the host, both ahead of the
  N64's addresses).

port-wrap, per file (`BEPASS_WRAP`), does the link's `--wrap` for the N64
side, which is one object now, and the glue calls the `__wrap_`s itself
(`gen_glue.py --wrap`, CMake's `PORT_WRAPS`: `externs.c` calls the
inflate), so the movable build links without `--wrap` (ld64 has none; the
other builds keep it for the N64 side's objects).  `port/src`'s data gets
sections of its own (`BEPASS_PORT_SRC`), which is how the pass knows it;
the pass drops every such section (and the asm data's `port.asmdata`) as it
places the variables, so none reaches `llc`, where Mach-O would want
`__DATA,__name`.  The asm data's `.ll` files carry the N64 side's triple and
data layout (CMake asks clang for them with the N64 side's flags), so
`llvm-link` links them without a warning.
`PORT_ARENA_STATS=1` makes `opt` print the counts, `PORT_ARENA_MAP=FILE`
write where each variable went.

The translated code's `rdram` is the arena and its `SYM()`s are the N64
addresses (`syms_<module>.h`'s own; no `port_recomp_syms.h`).  The host
reaches game memory through `port_ptr`, names the game's variables by
their N64 addresses (`PORT_VAR`, `PORT_ADDR`: `n64_syms.h`, from the same
table), and uses the host versions of the copies and `sprintf`
(`libc64.c`) in either width.  The movable build links no linker script
(`gen_ld.py`'s, `port_syms.ld`, `port_fixed.ld`) and maps nothing fixed
(RDRAM, the stacks, the hardware registers, which neither the C nor the
translated code reads).

**How it was checked**, us.v10, a step at a time (the commits say which):

| step | TAS (64-bit big-endian, `--replay`) | 3,000 frames of `PORT_AUTOSTART=1` against the build without it |
| --- | --- | --- |
| 1, RDRAM moves | 57 platinum, all reads matched; save and replay log identical | `--wav` and save identical (LP64, 32-bit native) |
| 2, the stacks | the same, identical | identical |
| 3, the arena link | 57 platinum, all matched, no mode forced; 164 retraces given anyway (373) | identical with `PORT_COUNT_PER_OP=0` |
| 4, the arena, globals by constant | 57 platinum, all matched, no mode forced; 142 given anyway | identical with `PORT_COUNT_PER_OP=0`; RDRAM the same but the stacks' addresses |
| 5, functions, PIE | 57 platinum, all matched, no mode forced; 126 given anyway | identical with `PORT_COUNT_PER_OP=0` |
| 1-5 on main's fibers | 57 platinum, all matched, no mode forced, 395 given anyway; `PORT_THREADS=ucontext` and `pthread` the same save and replay log | identical with `PORT_COUNT_PER_OP=0`; `--interpolate --widescreen` (GL, headless) the same screenshots |
| 6, call types | 57 platinum, all matched, no mode forced, 384 given anyway | identical with `PORT_COUNT_PER_OP=0`, 64-bit and 32-bit native |
| 7, LP64 | LP64 movable: 57 platinum, the same save and the same report as the LP64 build without it (both drift: 3,993 of the log's reads skipped, 11 modes forced) | LP64: identical with `PORT_COUNT_PER_OP=0` |

Where the arena is doesn't matter: the step-5 TAS played again with
another arena offset (`PORT_ARENA_OFFSET=0x10`, and whatever ASLR gave
both runs) wrote the same save and the same replay log, line for line.
From step 3 on the timing differs a little from the build without it:
the string literals are laid out differently, and some of the game's
loops over them take other paths by their alignment; the game logic
doesn't see it (the pace log is identical over those 3,000 frames).
(The retraces given anyway went from 126 to 395 with main's fiber API
underneath: the build without the option went from 373 to 376 then.)

The call repair found a bug of the 32-bit builds: `func_80264BA4` is
defined `u8` (1D990.c) and declared `s32` in four other files, and i386
leaves the upper bytes of `eax` as they were, so the map's `!= 3`
compared garbage (IDO's `v0` has a `u8` zero-extended; the 64-bit builds
widen narrow results, port-ilp32).  Those declarations are `u8` now
(the ROMs don't change: IDO's caller doesn't look at the upper bits), so the default 32-bit builds are right too, and the
32-bit native-endian movable build plays as the one without (before, the
two had parted, on different garbage).  The 32-bit big-endian build's
TAS gains by it: 57 platinum and all reads matched as before, but no
mode forced any more (two were), 78 retraces given anyway (85).

**What's left** for the movable build itself: the access profiler
(refused with it), `build_cmp.py rdram`'s view of the stacks, and the
glue's prototypes against the host's definitions for wasm-ld (the pass
fixes the N64 side's calls to the glue by the declaration it has, which
wasm-ld will check against `entry.c`'s).

For **macOS** (arm64) the movable build is all there is; what it took and
how it was checked without a Mac is under "Other hosts", "macOS".

And for **WebAssembly** (emscripten): step 6 first; the N64 side through
the system clang and the plugin for `wasm32-unknown-emscripten` (emsdk's
clang can't load the plugin: it ships no LLVM headers), the arena link
as it is, `llc` to a wasm object for emcc's link; the host through emcc
(`-sUSE_SDL=2`, the OpenGL renderer on WebGL 2 or the software one, the
ROM and the save through the browser's files and IndexedDB, the main loop
yielding to the browser, threads without `ucontext`: Asyncify's fibers or
the pthread backend's structure); the arena from `malloc` (28 MB of
linear memory), or at linear address 0 with `GLOBAL_BASE` above it.

Tried for wasm32 with step 6: the movable 32-bit native-endian build's
commands with `--target=wasm32-unknown-emscripten` for `-m32` compile all
124 files, and the arena link as the build does it (the asm data's `.ll`
files, `-port-arena-native`) gives `llc` a module it makes a wasm object
of, with no signature thunk left.  (Before the call repair `llc` failed on
the mismatched calls.)

The LP64 build doesn't finish the TAS at the moment, with or without
this: it stops with SIGFPE at about the movie's read 1,620 (main's
`build/p10lp` too), which is being looked at separately.

## The platform layer

- **Threads** (`port/host/threads.c`): each OSThread is a fiber
  (`port/host/fiber.h`: ucontext on the one OS thread, or a host thread
  each, one running at a time; see "Threads without ucontext"), scheduled
  as libultra does: the highest-priority runnable thread runs until it
  blocks, yields, or wakes a thread of higher priority.  The idle thread parks when it drops to priority 0.  Each thread
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

### Threads without ucontext

The context switch is behind a small interface, `port/host/fiber.h`, and
`threads.c` does all the scheduling above it.  The switches form a star:
the loop (`main.c`) runs a fiber (`fiber_run`) until it yields back
(`fiber_yield`, `fiber_exit`); a fiber never switches to another fiber.
There are two backends:

- **ucontext** (`fiber_ucontext.c`): `makecontext`/`swapcontext` on the one
  OS thread, as before.  The default where it exists (Linux with glibc).
- **pthread** (`fiber_pthread.c`): an OS thread per fiber, and exactly one
  of them, or the loop's, running at any time.  The CPU is a baton handed
  over under one mutex, with a condition variable per fiber and one for
  the loop; every handover is a lock/unlock pair, so what one side wrote
  the other sees, and the game's state (touched only by the baton's
  holder) needs no locks of its own.  A fiber whose frames are dropped
  (`fiber_free` of a suspended one, a thread that ends) `longjmp`s back to
  its thread's start routine and returns from it: nothing unwinds through
  the game's frames.  The default on macOS (where ucontext is deprecated),
  emscripten and musl (which have none).

Where both are built, the environment's `PORT_THREADS=ucontext|pthread`
picks one at startup (CMake's `PORT_THREADS` is the default), so one
executable runs either and a comparison isn't confounded by two builds'
layouts.  With the pthread backend the host calls that are tied to the
main thread go back to it: `fiber_call_on_loop` runs the renderer
(`host_gfx_task`: the GL context) and the pad (`host_input`: SDL) on the
loop's thread while the fiber waits; in the ucontext backend it is a plain
call.  Everything else the game threads call into (the audio queue, the
EEPROM file, the ROM) doesn't care which thread it is on.

**The stacks.**  A fiber's native C runs on a host stack in the KSEG0
window, `0x90000000` + 1 MB per thread slot (`PORT_STACK_BASE`, port.h),
in both backends (`pthread_attr_setstack`).  It has to be there as things
are, not for the game's sake but for the port's: the address of a local
reaches game memory and the translated code (a message queue on a
thread's stack, an argument block the game's C passes to the engine), the
translated code reaches memory as `rdram + (addr & 0x1FFFFFFF)` with
`rdram = 0x80000000`, and the 64-bit build stores pointers in 32 bits.  So
the stacks move with RDRAM when the fixed addresses go (a base-relative
RDRAM arena would hold them too); nothing in the backends depends on the
address itself (the pthread backend takes a NULL stack for one of its
own).  The backends do have to agree on where the frames are: the game
stores the addresses of its locals in RDRAM, and glibc puts a new thread's
descriptor and TLS at the top of a stack it's given (0x1210 bytes here),
which moved every frame down by as much.  In the LP64 build the hint
box's portrait static at frame 1500 of `PORT_AUTOSTART=2` came out
different (the rest of that run was identical).  So both enter a fiber
through `fiber_enter`, which `alloca`s down to 64 KB below the top of the
stack first; RDRAM is then identical between the two, word for word, at
every dump compared.  (The same run also showed that the LP64 build's
pixels depend on the executable's layout: two LP64 builds whose host code
differs draw that portrait differently, presumably from the pointers into
the image the game stores in RDRAM.  Compare LP64 runs with one executable.)

**How it was checked**, us.v10, one executable each, `PORT_THREADS=ucontext`
against `=pthread`, `PORT_COUNT_PER_OP=0 --deterministic --renderer sw`,
`PORT_AUTOSTART=2`, 4,000 frames with the save, `--wav` and a screenshot
every 250 frames: identical in the 32-bit and the LP64 build (and the
ucontext run identical to the build before the change); RDRAM at
controller reads 100 and 700 identical in both.  The TAS: the 32-bit build
on either backend gives the log's "125297 of the log's 125297 matched (0
skipped)", 57 platinum, with the log and the save byte for byte those of
the build before the change; the 64-bit build the same (57 platinum, log
and save identical between the backends).  The LP64 build doesn't finish
the TAS on either backend, nor did it before the change: it dies of a
SIGFPE after the log's read 1,620, identically on both.  Speed: the same
to within a few percent (4,000 frames of `PORT_AUTOSTART=2`, 32-bit:
22.4 s both; LP64: 18.1 s ucontext, 18.8 s pthread).

**Returning to the host every frame.**  A browser's main loop can't block:
the page gets control back once per `requestAnimationFrame`.  The loop in
`main.c` already has the shape for that, because of the star: between two
`fiber_run`s no fiber is in the middle of anything the loop has to wait
for, and the loop's own state (`next_vi`, `vi_force`, the pending events)
is a handful of variables.  Made into `static`s, the body of its `for (;;)`
becomes a function that runs until it has delivered a retrace
(`host_video_frame`, `port_irq_vi`) and returns; `main` calls it in a loop,
and an emscripten build hands it to `emscripten_set_main_loop` instead,
with the `nanosleep` replaced by returning (the browser paces the frames).
Not done yet, so as not to move the loop under the other work on it.  What
the fibers need there:

- **pthreads in the browser** (`-pthread`, a Web Worker per thread, a
  `SharedArrayBuffer` and so the COOP/COEP headers): the pthread backend
  as it is, but the page's main thread may not block, and `fiber_run`
  waits on a condition variable while the fiber runs (emscripten turns
  that into a busy-wait on the main thread).  Either the whole loop runs on
  a worker (`-sPROXY_TO_PTHREAD`, with the renderer's WebGL context on an
  OffscreenCanvas, and `fiber_call_on_loop` then means "on the worker
  with the context"), or the frame function's waits stay short.
- **emscripten's fibers** (`emscripten_fiber_init`/`_swap`, which need
  Asyncify): a third backend in the same shape as the ucontext one, all on
  the page's thread.  Asyncify instruments every function that can be on
  the stack at a switch, which here is nearly all of the game (any loop's
  `__port_poll` can yield), for size and speed.
- The replay's audio answers name their caller by its return address
  (`replay.c`, `replay_hooks.c`); WebAssembly has none to look up, so
  `--replay` there needs the callers passed explicitly.

### Other hosts

What wasn't portable in the host code, and what became of it:

- `main.c`'s ASLR re-exec (`personality(ADDR_NO_RANDOMIZE)` and `execv` of
  `/proc/self/exe`, for a brk heap in the fixed windows) and
  `prctl(PR_SET_TIMERSLACK)` are Linux-only and guarded by `__linux__`;
  `MAP_FIXED_NOREPLACE` is used where it exists, and elsewhere the fixed
  map is a hint that has to be taken (or the port stops); the movable
  build maps nothing fixed and has none of it.  The crash
  handler (`sigaction`, `sigaltstack`, per thread in the pthread backend)
  is POSIX, left out under emscripten.
- `replay.c` reads its own ELF symbol table from `/proc/self/exe` for the
  audio answers; elsewhere it would ask `dladdr`.  The movable build does
  neither: the arena link names the callers (`port_replay_set_caller`).
- `ucontext` is only used where `swapcontext` links (not macOS, emscripten
  or musl), and the instruction count `__port_icount` is a plain global the
  translated code and BEPass add to: nothing x86-specific (no inline asm,
  `rdtsc` or intrinsics in the host code; `gfx.c` gets `-msse4.1` only on
  x86).
- What the fixed builds need of the link can't be had on macOS (arm64
  macOS reserves the low 4 GB as `__PAGEZERO`, and ld64 has no
  `-Ttext-segment`, linker scripts, `--wrap` or `__start_`/`__stop_`
  section symbols); the movable build needs none of it ("macOS", below).

Checked: every file in `port/host/` compiles for `aarch64-linux-musl`
(musl's headers: no ucontext, no glibc extensions), also with `__linux__`
undefined, which takes the non-Linux branches.  There is no macOS SDK or
emscripten here to try those.

**AArch64.**  `port/tools/cross-aarch64.cmake` cross-builds the 64-bit port
(`-DPORT_64BIT=ON`) for AArch64 Linux with clang, given the target's glibc
and gcc runtime (Arch: `aarch64-linux-gnu-glibc`, `-gcc`, `-binutils`), an
aarch64 SDL2 (through `PKG_CONFIG_LIBDIR`/`PKG_CONFIG_SYSROOT_DIR`), and
the host's BEPass from a native build (`-DPORT_BEPASS_PLUGIN`, since clang
loads it).  What it took: `ilp32cc.py` keeps stage 1 at i386 whatever
`--target` CMake adds; and off x86 the optimiser raises the alignment of
globals (AArch64 prefers 4 for `i8`/`i16` and 16 for float arrays it
vectorizes), which moved them off their N64 addresses, so port-ilp32
gives each global a section of its own there, named as `-fdata-sections`
would (the x86-64 build is byte for byte what it was).  It builds, passes
`gen_ld.py check` (3,419 symbols, 0 misplaced) and runs under
`qemu-aarch64` (headless, `--renderer sw`): 4,000 frames of
`PORT_AUTOSTART=2` in about 7 minutes, the same on both thread backends,
and against the x86-64 build the save, `--wav` and 15 of the 16
screenshots identical.  The 16th is the hint box's portrait static at
frame 1500 again.  The only differences in RDRAM before it are host
addresses the game stores (pointers into the image, in tables at
`0x80209DF4`, `0x802F5760`-`0x802F8B1C` and more in us.v10), which move
with the executable's layout, between architectures as between two LP64
builds, so the static presumably samples memory that holds some (not
tracked down further).  Removing the fixed addresses will move them too.

The movable LP64 build (`-DPORT_LP64=ON -DPORT_MOVABLE=ON`) cross-builds
the same way and runs under `qemu-aarch64`: 3,000 frames of
`PORT_AUTOSTART=2` with the save, `--wav` and all 12 screenshots identical
to the x86-64 build's, on either thread backend (no host addresses in game
memory there, so the portrait static is the same too), with and without
`-fsigned-char`.  The N64 side has that flag now: the game is built with
IDO's `-signed`, which is x86's default for plain `char` (and Apple's
arm64 one) but not AArch64 Linux's, where the LP64 build's code changed
with it (the 64-bit build compiles its stage 1 for i386, and x86-64's code
is byte for byte the same).

### macOS

**Untested**: prepared for macOS on Apple silicon (arm64) and checked from
Linux as far as that goes (below), but not built or run on a Mac yet.

macOS takes the movable 64-bit build, LP64 first (`-DPORT_LP64=ON
-DPORT_MOVABLE=ON`; `-DPORT_64BIT=ON -DPORT_MOVABLE=ON` goes too, and CMake
refuses the fixed-address builds there).  With Homebrew:

```
brew install llvm sdl2 libepoxy pkgconf cmake ninja python
port/tools/macos_build.sh build/port-macos -DPORT_VERSION=us.v10
```

which is CMake with `$(brew --prefix llvm)/bin/clang` (and `clang++`) as the
compilers and the two options.  What the build does differently there:

- **LLVM.**  Apple's clang loads no pass plugins and ships no LLVM headers
  (CMake stops on `AppleClang`), so the compilers are Homebrew's LLVM, and
  `llvm-config`, `opt`, `llc` and `llvm-link` are looked for next to the
  compiler first (Homebrew's LLVM is keg-only, not on `PATH`).  BEPass is
  a bundle linked with `-undefined dynamic_lookup`: LLVM's symbols are
  those of the clang or opt that loads it.
- **The N64 side** is compiled for the compiler's own triple
  (`arm64-apple-macosx...`; `-nostdinc`, so no SDK is involved), through the
  arena link, and `llc` makes a Mach-O object of it.  Nothing in that
  module has a section name left, and every name is an IR name, which
  Mach-O prefixes with `_` for host and N64 side alike; the glue's
  `__asm__` names (`gen_glue.py`, for the libultra calls of jp's
  `GLOBAL_ASM`) take `__USER_LABEL_PREFIX__`.  No `--wrap`, no
  `__start_`/`__stop_` symbols, no `-pie` (every arm64 executable is one).
- **Threads**: the pthread backend (ucontext is deprecated there, and not
  looked for), which hands `pthread_attr_setstack` the whole pages inside a
  fiber's arena stack: macOS takes nothing else, and the arena sits at
  `PORT_ARENA_OFFSET` into its page (`fiber_enter` still starts the frames
  `FIBER_TOP_GAP` below the stack's own top, so the runs are the same;
  Linux does it the same way, identical runs on both backends).
- **OpenGL**: macOS's core profile is 4.1 and only forward-compatible,
  which `gfx_gl.c` asks for there; the renderer needs 3.3 (GLSL `330
  core`, nothing newer), through libepoxy.
- **The N64 link's ELFs** are read with `llvm-nm`/`llvm-readelf` where
  there are no MIPS binutils (`N64_NM`, `PORT_N64_NM` for `gen_syms.py` and
  `tools/recomp`): the same names and addresses.  The stage-2 build itself
  still needs the MIPS binutils and the Linux IDO recompilation, so the
  simplest is to make it (and `make -C tools/recomp`) on Linux and copy
  `blastcorps/build`, `asm`, `assets`, `.version` and
  `build/blastcorps.<version>.map`.

**How it was checked from Linux.**  `port/tools/cross-macos-check.cmake`
configures the port for `arm64-apple-macos11` (CMake's Darwin branches,
`APPLE`), with musl's aarch64 headers standing in for the SDK's libc and
stand-ins for the two SDK headers SDL2 includes (`port/tools/macos-check/`),
SDL2's and libepoxy's headers through `PKG_CONFIG_LIBDIR`, and the host's
BEPass (`PORT_BEPASS_PLUGIN`; the file's comment has the commands);
building its `n64_link`, `host` and `recomp` targets compiles everything,
the N64 side to a Mach-O arm64 `n64.o`.  `port/tools/macos_check.sh`
links the objects with `ld64.lld` (`-undefined dynamic_lookup`, there
being no libraries) and lists what is left undefined that libSystem, SDL2
or libepoxy wouldn't give: nothing, in the LP64 and the 64-bit build (158
names: libc, pthreads, SDL, epoxy's GL, `___stack_chk_*`, `bzero`, which
LLVM makes of `memset` on Darwin, `sqrtf`).

On Linux the same changes leave the builds as they were: the movable
64-bit build (no `--wrap` now, no section symbols) plays the TAS to the
same save and the same replay log, line for line, as before them (57
platinum, all 125,297 reads matched, no mode forced), the LP64 movable one
gives its report as before (57 platinum; 3,993 reads skipped, 11 modes
forced, "The movable build"), and the default 32-bit build its 57
platinum with every read matched and no mode forced; the x86 objects'
code is byte for byte what it was.

What only a Mac can tell: whether Homebrew's clang loads the bundle (and
`opt` does), the SDK's headers against the host code (only musl's were
tried, and `__linux__` undefined), the link with ld64 against the real
libraries, the pthread backend with 16 KB pages, SDL's window and GL
context on the main thread (the loop is on it, the game's threads call
over through `fiber_call_on_loop`), and then the TAS.

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

## Frame rate

Gameplay runs at 30 frames a second at most, and `--interpolate` shows it
at 60 by drawing a frame between each two.  The game itself can't be made
to run at 60, because its logic doesn't scale with time.

### How the game paces itself

- **Retraces.**  The scheduler counts every retrace (`Sched.frameCount`,
  `D_803156C4`) and, while the game isn't paused (`D_802E8BD0`), the level
  clock (`unk280`, `D_803156C0`).
- **The swap** (`__scHandleRDP`, 2C560.c).  In the modes of the mask
  `0xC9FD0FE79BFF80B0` (the logos, the title's story, the "leaders of"
  screens, ...) a frame is shown as soon as it is drawn, up to 60 a second.
  In every other mode, the levels and the world map among them, a frame
  is held until a retrace has gone by since the last one showed.  So each
  frame is on screen for two retraces at least: 30 frames a second at
  most, and fewer when the CPU takes longer (the world map averages 3.45
  retraces a frame, see "Timing").
- **The game's frame.**  One pass of the mode's frame function (the
  level's is `func_802475D8`, 00000.c) runs one step of the game and builds
  one display list.  The step is the same size however long the frame
  took.  There is no time delta anywhere in it: the game adds constants
  (the zoom `D_802E8BE0 += 0.05`, the pause menu's scroll
  `D_80364A80 += 100`) and counts frames (`D_80358060`, `D_80358064`).
  The handwritten engine (vehicles, collisions, the carrier) never reads
  the counts: its only use of the scheduler is sending it a task
  (5FD50.s).
- **What does count retraces**: the level clock and so the medal times
  (`func_8028604C`: tenths of a second, retraces / 6, "TIME IN LEVEL"),
  the countdowns on the results screens (1D990.c), the HUD's blinking
  (`D_803156C4 % 20`, 3E4C0.c, 2E490.c), the fades (30430.c,
  `func_80274BF0`), the messages' scrolling and the music cues
  (`func_8026BCE0` scales by the retraces since its last call), and the
  front end's timeouts (`SC_FRAMECOUNT - D_80364A54 > 160`).  A frame that
  takes three retraces slows the game down while the clock runs on, on
  the N64 as on the port.
- **Audio** doesn't depend on the frame rate.  The scheduler starts the
  audio thread's frame on every second retrace (`frameCount & 1`), and the
  AI paces it.
- **The TAS** gives one pad per game frame.  The retraces a frame takes
  change where the game is by the next pad (`--replay` gives each frame
  the log's retraces: "The TAS").

### Running the game at 60: a turbo, not 60 fps

This was tried by making `__scHandleRDP` swap at once in every mode
(`|| port_fps_unlocked()` in its condition, for an experiment only), with
`PORT_COUNT_PER_OP=0` so that the CPU model doesn't hold frames back.
Simian Acres (`PORT_AUTOSTART=3`, `--deterministic`) then ran at 1.00
retraces a frame instead of 2.00.  The bulldozer covered the same distance
per frame as before: from rest, 21.7 units a frame over its first 50
frames, against 21.0 at 30 fps.  That is twice the distance per second:
the game ran at double speed while the clock ran at its normal rate.  To
run at 60 the game would need every per-frame constant halved, in the C
and in the handwritten engine (physics, animation, the carrier's route,
the camera's smoothing), and a halved step doesn't give the same states at
the frames the two rates share, so no movie would sync.  The experiment
wasn't kept.

### `--interpolate`: in-between frames

The logic runs as before, and the renderer makes the extra frames
(`port/host/gfx.c`, "in-between frames"; `gfx_gl.c`, the twins):

- **Two passes.**  Where the game holds its frames (the mode is outside
  the swap's mask, `host_frame_held`), each graphics task's display list
  runs twice, back to back, while RDRAM is as the task found it.  The
  first pass is the real one.  The second draws into a *twin* of each GPU
  target, which has its own depth buffer.  The RSP/RDP state (and TMEM)
  before the second pass is what the first started from, and after it,
  what the first ended with.  The second pass draws nothing the software
  rasterizer owns (the 8-bit render-to-texture images and the RDRAM
  z-image stay the first pass's), skips the read-backs, and charges no
  RDP time.
- **Blending vertices.**  The first pass records where each vertex load
  (`G_VTX`) put its vertices in clip space.  The second pass moves each
  vertex halfway from where the previous frame put the same vertex.  For
  model data that stays put this is the same as interpolating the
  matrices, since a vertex's clip position is linear in its MVP.  It also
  follows vertices the CPU writes each frame, and it needs no tags in the
  game's code: the sm64 and Zelda ports record the transforms in their
  C, which isn't possible for the handwritten engine.  A load is identified
  by its address and by how many loads from that address came before it
  in the frame.  Inside the double-buffered per-frame buffer (`D_803156F8`,
  segment 2) the address is the offset into it, so the two buffers' loads
  pair up.  A load whose vertices moved by more than a quarter of their
  distance from the eye (in clip space) is taken for different vertices
  (an object that appeared, or a camera cut), and all of it is drawn where
  the frame has it.
- **Showing it.**  When the VI moves to another buffer
  (`host_gfx_frame_shown`), the frame in it is complete: the next frame's
  tasks wait for the RDP's thaw, which comes after.  Its twin is then
  marked ready if every task of the frame had its second pass.  The hook
  is on the host side on purpose.  A call in `osViSwapBuffer` would be
  counted as the game's CPU time (BEPass's ICount), and that alone moved
  the TAS replay's timing a little (76 retraces given anyway instead of 80,
  with the option off).  The first retrace that shows the frame shows the twin instead,
  and the second shows the frame.  The frame is therefore on screen one
  retrace later than before, and in-between images take the retraces a
  frame used to repeat on.
- **The game sees none of it.**  With `--interpolate` the game's memory is
  identical: RDRAM dumps (`PORT_DUMP`) at the 1,200th and 1,500th
  controller reads (in Simian Acres, `PORT_AUTOSTART=3`, OpenGL,
  deterministic) are byte for byte the same with and without it.  The TAS
  gives the same replay report and the same save either way (below).
  Without the option, host_gfx_task runs as before.

Measured with `PORT_AUTOSTART=3`, `--deterministic`, OpenGL, from boot to
retrace 3,000 (the attract, the menus, Simian Acres):

- **Images.**  Over retraces 1,600-2,700 (the level's drop-in and
  driving), 944 of 1,100 retraces showed a new image, 51.5 a second.
  Without the option it is 474, 25.9 a second.  In steady driving
  (retraces 2,400-3,000) every retrace is new, 60 a second against 30,
  and every second one is exactly the run without the option's.
  The report at exit counts them: "3000 retraces presented: 1639 showed a
  new frame of the game's, 865 an in-between one".
- **Matching.**  Of 647,016 vertex loads in the second passes, 96.2% were
  blended with the previous frame's, 3.6% had no match there, and 0.18%
  of the vertices were in loads that moved too far.
- **Host cost.**  Each graphics task costs twice the GPU work: 0.86 ms
  of host time a task against 0.45 ms (4x, a Radeon RX 7900 XTX, about
  two tasks a frame).
- **Screenshots** (`PORT_SHOT_EVERY=1 PORT_SHOT_FROM=N`) show the
  in-between frame at the frame's first retrace.  Of each pair of
  retraces, the second matches a run without the option exactly.

**The TAS** (us.v10, 32-bit build, `--headless --replay`) with
`--renderer gl --scale 1 --interpolate` gives the same replay report
(all 125,297 of the log's reads matched, none skipped) and a byte-identical
save as `--renderer gl` without it: 57 platinum.  Over its 275,446
retraces it showed 125,325 new frames of the game's and 120,490 in-between
ones, and blended 91% of the vertex loads.  With the option off, the
default software renderer's replay is exactly what it was before the
option existed: a byte-identical log and save.

What it doesn't do:

- **Only OpenGL**: the software renderer draws into RDRAM, which the second
  pass mustn't touch, so `--interpolate` is ignored with `--renderer sw`
  (and says so).
- **Rectangles aren't interpolated.**  Texture and fill rectangles (text
  and other 2D pieces) move at the game's rate.  So do
  texture animations (the TVs, texture coordinates rewritten with
  `G_MW_POINTS`) and the soft shadows' render-to-texture images, whose
  images are the frame's (their geometry is blended).
- **A fixed halfway point.**  Frames that take three retraces or more
  (lag, the world map) show one in-between image and then the frame for
  the rest.  The front end's one-retrace modes need none and get none.
- **Mispairings.**  When objects that share vertex data appear or
  disappear, the count of earlier loads from an address shifts, and
  instances can pair with each other.  If they are near each other, the
  distance test lets them through, and for one image an instance sits
  between two places.

Beyond 60 (a 120 or 144 Hz display) takes two changes.  The first is k-1
second passes at t = i/k, each into its own twin, which multiplies the GPU
work.  The second is presents between retraces: at present the loop
presents at the VI (`host_video_frame`), and it would need a display clock
of its own, as the audio has in SDL.  Neither is done.

## Widescreen

`--aspect W:H` (`16:9`, `21:9`, up to 32:9), `--widescreen` (16:9) or
`--aspect window` (the window's shape, followed as it is resized) shows
the 3D world wider than the N64's 4:3, with either renderer.  The default
is 4:3, and then the output is what it was before, pixel for pixel.

The game doesn't know: it still draws its 320x240 frame with its own
projection.  The renderers' framebuffers are `off` columns wider on each
side (`off` = (240 * aspect - 320) / 2, rounded: 53 at 16:9, a 426x240
frame), with the game's 320 in the middle, which is then exactly the 4:3
picture.  A perspective triangle's screen position goes past 0 and 320 as
it is (the N64 scissors it there), so the world continues into the sides.
What else decides what is drawn out there (`gfx.c`, shared by both):

- **Scissors and fills.**  A scissor that covers all 320 columns covers
  the wide frame; a narrower one stays where it is.  So does a fill that
  covers them (the game's 1-cycle fills stop at 319): the background, the
  z-buffer clear, the fades and the sepia overlay of the story screens.
- **2D.**  A polygon with w = 1 throughout (an orthographic projection)
  that spans the game's frame from edge to edge is stretched to the edges
  of the wide one: the sky's gradient behind the levels.  The rest of the
  2D (the HUD and its arrows at the edges pointing at what is off screen,
  text, menus, the pause screen) stays in the middle, as does every
  texture rectangle.  A texture rectangle at an edge (the tiles of a
  full-screen picture: the story, results and promotion screens) blacks
  out the side beyond it, in its rows, so those screens are pillarboxed
  instead of framed by whatever the sides held.
- **`--interpolate`** composes with it: the twins are the wide size too,
  and the in-between pass goes through the same rules (the positions are
  blended in clip space, before the frame is widened).  Every second image
  is then exactly the widescreen run without `--interpolate`'s.
- **The software renderer** draws the 320-wide 16-bit color images (the
  framebuffers) wide on the host instead of into RDRAM, the z-buffer with
  them, and the window shows those.  A texture load from one would copy
  its middle back to RDRAM first (none happens), as the OpenGL renderer
  reads its targets back.

The game doesn't cull at the view's edges.  Over 40,000 frames of the TAS,
the perspective triangles it sends fall off smoothly with their distance
from the middle, out to ten times the 4:3 view's half-width, with no step
at its edge, so nothing pops in at the sides.  What bounds the world there
is the level's list of active cells (`D_803C30A8`, made from the camera's
position by `func_802A470C` in `5FD50.s`, or from an area's list by
`func_80295C70`): the terrain and the objects drawn come from those cells.
The game logic reads the list too (`func_80267614`, `func_80270A54`), so
widening it would change the game, and it is left alone.  At 16:9 the
missing cells show now and then, as a small black patch at the far edge
of a side; at 32:9 more often and larger.

None of this reaches the game: the display lists are the game's, the
RDP's time comes from the 4:3 geometry, and RDRAM is the same but for the
two color framebuffers, which the OpenGL renderer never writes anyway.
With `--widescreen` the TAS still beats the game (OpenGL, with and
without `--interpolate`: all 125,297 reads matched, none skipped, 57
platinum, the same report and save as without widescreen; also with the
software renderer at 21:9).  The LP64 build's replay, which drifts on its
own (3,993 of the log's reads skipped), gives the same report and save
with widescreen as without, either renderer.  `PORT_COUNT_PER_OP=0
--deterministic` runs (the TAS's first 30,000 frames, `PORT_AUTOSTART=1`,
and `PORT_AUTOSTART=3` with `--interpolate`) write the same save and
sound with and without it, 32-bit and LP64, and the same RDRAM outside
the framebuffers (with OpenGL, the same RDRAM).

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
level and on the map.  Two of the game's mode switches used to be the
movie's rather than the port's: twice, at the world map, the 32-bit
port's cursor picked the level it was on where the movie's picked the
next one, and the switch hook sent it where the movie went.  That was
the 32-bit build reading garbage in a `u8` result (`func_80264BA4`,
"The movable build"); since that's fixed, no mode is forced (78 retraces
given anyway, one save command let go early).  The 64-bit build plays
it as exactly (all 125,297 reads matched, the player always where the
movie has it, 57 platinum), with no mode forced at all; its timing
differs (376 retraces given anyway).

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
- `--interpolate` shows gameplay at 60 frames a second (OpenGL; "Frame
  rate"), without changing what the game does.
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
- **More than 60 frames a second** for faster displays: more in-between
  passes and presents between retraces ("Frame rate").
- **Real call states** for the translated code's test.  Recording `ctx` and
  memory at each translated function's entry during play would replace the
  random registers, and reach the 30% of blocks the random states don't.
- **Toward native code**: the 64-bit build is done ("The 64-bit build"),
  native-endian memory ("The native-endian build") and the LP64 build ("The
  LP64 build").  Next are the fixed addresses: the game's C and data should
  work at any base, which arm64 macOS and a WebAssembly build (both wanted
  eventually) need; the movable build ("Movable memory") is that.  Threads without `ucontext` are done
  (the pthread backend, "Threads without ucontext"); the browser also
  needs the loop to return every frame, and a way for the fibers not to
  block its main thread (the same section).  The
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
