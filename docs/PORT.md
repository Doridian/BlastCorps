# The PC port

The PC build (`port/`, milestone 1 of ROADMAP Phase 5).  It runs the
decompiled C natively and Rare's handwritten engine as C written from it
(`port/engine`, "Replacing the engine"); the translator that first ran the
handwritten asm (`tools/recomp`, "The handwritten asm") now gives the
engine its block sizes and is the check the native code is compared with.
The build is described first; the translator and its test follow from "The
handwritten asm".

## Building and running

```
make VERSION=us.v11 -C blastcorps          # the N64 build: the port reads its ELFs and asm/
make -C tools/recomp                       # the engine's block sizes (and the check build's translation)
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
hosts", "macOS", and `port/tools/macos_build.sh`), and which `emcmake cmake`
builds for WebAssembly, under node or in a browser ("WebAssembly"):

```
cmake -S port -B build/port64 -G Ninja -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_ASM_COMPILER=clang -DPORT_64BIT=ON
cmake --build build/port64
```

Keys: they follow what is on the screen ("Keyboard, mouse and touch",
below): WASD or the arrows drive in a level (W/S the pedals, A/D the
steering), and move in the menus and on the world map, where the mouse
and touch work too; X, C, Z and IJKL are A, B, Z and the C buttons
everywhere; an SDL game controller works too.  On the name entry the
keyboard types ("Typing the name", below).  `--help` lists
the options: `--headless`, `--deterministic` (virtual time: as fast as the
host can, identical every run), `--frames N`, `--screenshot PREFIX`,
`--save PATH` (the 4 Kbit EEPROM, default `blastcorps.eep`; runs meant to
be compared should each start from the same one, or none),
`--renderer gl|sw` (default: OpenGL with a window, software headless),
`--scale N` (OpenGL: render at 320x240 times N; by default it follows the
picture's height in the window, so resizing the window changes it;
`--max-pixels N` caps it) and
`--filter n64|bilinear|point` (textures: the N64's 3-point filter where
the game asks for bilinear filtering, which is the default, a 4-tap
bilinear one, or point sampling throughout), `--interpolate` (60 frames a
second where the game draws 30, and `--display-hz N|auto` for faster
displays, see "Frame rate"), `--aspect window` (the window's shape, the
default with a window), `--aspect W:H`, `--widescreen` (16:9) and `--hud
edges|centre` (see "Widescreen"), `--model-icons`/`--no-model-icons`
(the icons that are pictures of the game's models drawn as the models;
default with a window, see "Model icons"),
`--wav PATH` (everything the game plays, at the AI's rate) and `--no-audio`; sound goes
to SDL unless the run is `--headless` or `--deterministic`.
`PORT_AUTOSTART=1` taps Start and A, which is enough to get from the title
through the name entry into Simian Acres (`=2` taps by the game's own
retrace count and then drives forward in the level, as
`port/tools/m64p_pace.c` does in mupen64plus; `=3` also taps A in the
level, which clears the hint panels, so the vehicle keeps moving; with
either, `PORT_LEVEL=N` goes into level N instead, by the IDs of
`docs/blast_corps_levels.txt`: `=1` is Angel City, `=3` Blackridge Works);
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

### Typing the name

The name entry (`hd_front_end/1C40.c`: `func_801EA93C` sets it up,
`func_801EAA7C` runs it each frame) is a wheel of 33 characters turned
with the stick: A to Z, 1 to 4, `/`, `.` and a delete entry (0x7F; the
characters are `D_80208314[i]` for i below `D_80208350[0]`, then
`D_802082FC`).  `D_802154B4` is the wheel's angle, `D_802154B2` the one it
eases to (0x7FFF − i·1986 for character i), and the game works out the
character in front, `D_802154B6`, from the angle each frame before it
looks at the pad.  A takes that character (`D_802154BE` is 1 while it
flies into the name, `D_802154BC` counts the name's characters, at most
`D_80215924`), B takes the last one back or leaves the screen when there
is none, Start confirms.

In the port the keyboard types there (`port/host/video.c`, "typing the
name"), from the host alone: the game's C is unchanged, so the CPU
model's counts and the TAS are too.  The screen is up while the menu's
screen `D_8036BB18` is 0xB and its state `D_8036BB1C` isn't 1 (when
`17990.c` calls `func_801EAA7C`); it takes the pad in state 2.  There
SDL's text events (and Backspace and Escape) fill a queue, and each
controller read takes the next character: the host turns the wheel to it
(writes both angles at once) and presses A in the same read, then waits
until the character is in the name (`D_802154BE` back to 0) before the
next, which it presses in the read that sees it land; if the game didn't
take the press (the length didn't change and nothing flew within 10
reads) it tries again.  The character's flight into the name, which the
game eases a fifth of the way a frame (`D_80215944`/`48` to
`D_8021594C`/`50`, its size `D_80215440`/`44` to `D_80215448`/`4C`:
about a second), the host hurries: half the way more each read while it
is the last one typed (a quarter of a second), and all of it while more
are queued, so that each character takes two reads (1/30 s) and the
typing keeps up with a typist.
Backspace presses B, but only while the name has a character, so it
never leaves the screen; Escape presses B whatever the name holds.  A
character the wheel hasn't (0, 5-9, space, anything else) or one past the
seventh is dropped.  While the queue is busy the typing has the pad (the
stick still, no other button); on the screen the letter and digit keys
aren't their pad buttons at all, and the arrows, Enter and a controller
work as before.  `--replay` and `--deterministic` runs never type from the
keyboard.

`PORT_TYPE=TEXT` types TEXT when the name entry first comes up (`<` is a
Backspace), in any run, headless too; with `PORT_AUTOSTART` its taps of A
and B are dropped on that screen, so its Start confirms what was typed:

```
PORT_AUTOSTART=1 PORT_TYPE='blXy<<ast' build/port-us.v10/blastcorps --headless \
    --deterministic --frames 2000 --save /tmp/t.eep
port/tools/tas_check.py /tmp/t.eep            # name 'BLAST'
```

Checked that way in us.v10, us.v11 and jp (jp's wheel has the same Latin
characters; only the screen's title is in kana), with doubled letters,
Backspaces before anything was typed (the screen stays), and more than
seven characters; and in the page (headless Chromium, Playwright's
`page.keyboard`): typed, deleted, skipped characters, Escape deleting and
then going back, and the save holding the name.  The variants' quick tier
and the TAS on all of them, wasm's included, are unchanged.  Not tried: a
person at a real keyboard, an IME.  (A key pressed and released between
two controller reads is missed, as before: the pad comes from SDL's
keyboard state, and Playwright's `press` without `delay` is that fast; a
typed character isn't, since it goes through the queue.)

### Keyboard, mouse and touch

`port/host/ui.c` makes the pad from the keyboard, the mouse and touch by
what is on the screen, from the host alone like the typing (the game's C
has one hook, below).  Every controller read it decides which of these is
up:

- **a level** (mode 4, or 0x100, the carrier's overview, and not paused,
  `D_802E8BD0` 0): W/S press A/B, which is what the pad code
  (`45BB0.c`, `func_8028A470`) drives with in the default control mode:
  it drops the stick's Y there and makes it from A and B.  Where the game
  reads the stick's direction instead, W/S are the stick's Y: the player's
  vehicle (`D_80364456`) on foot (0), the Thunderfist (2), the J-Bomb (9)
  and the Cyclone Suit (16) in their default 360-degree modes, and any
  vehicle with its control-method bit set ("speed on 3D stick",
  `PlayerInfo.unkF0` bit `type`).  A/D steer (the stick's X, ±80, which
  past 50 also pulls the levers, as a pad does), Space is Z (the brake,
  and, held when stopped, out of the vehicle: `00000.c`), Shift R (each
  vehicle's special), Q/E C-left/right (the camera), R/F and the mouse
  wheel C-up/down (the zoom), Escape and Enter Start.  The mouse pointer
  is hidden there.
- **one of the game's windows taking input** (yoshi.c, `26570.c`: the
  window `D_8036BB18` in state 2, `D_8036BB1C`, with input, flag 0x20),
  in the front end or paused: the arrows and WASD move, Enter and Space
  are A, Escape and Backspace B.  The pointer finds the entry under it as
  yoshi.c draws them: the window at `unk4`, `unk6`, an entry at its x, y
  in it (plus the scroll `D_8036BB30` for those with flag 0x1000, when
  not faded out), its text's cells `unk6` wide and 0.6 of that apart, an
  icon about 64x32 from its `YoshiIcon` offset, a list's entries the
  window's whole width.  The table is the one `func_8026FB50` picks by
  the window's flags (the front end's sorted lists, flag 0x8000, are left
  to the keys).  Pointing at an entry moves the selection there by the
  window's own previous and next buttons (`D_8036BB3C`/`3E`, the D-pad
  as yoshi.c set them up), one press every other read, so the sound and
  the scrolling are the game's; a click does that and then presses A; a
  click in a message box (no flag 0x10000000) pages on; the right button
  is B.
- **the world map** (mode 0x4000, `11530.c`): keys as in a window.  A
  click (or tap) on a level picks it, a click on the picked one presses A
  (in, or the level's menu).  The levels are hit where the game draws
  them: their points on the globe (`D_8020D810`'s `unk24`-`2C`) through
  the last frame's matrices for Earth, as `func_8027690C` (`30C70.c`)
  does for the paths, on the globe's near side; the open ones only (the
  selected one, medals 1-5, 8; not 47 and 49, which aren't places).  To
  pick one, the host writes it as the stick's next level (`D_8021A907`,
  which the game works out from the paths each frame) and pushes the
  stick (x 40: past the game's 1500 for x²+y², and the camera at its usual
  pace) after a neutral read, while the map takes the stick
  (`D_8021A924` 1, the name in full, no window, no fade); the game then
  moves there itself, the carrier and the sound with it.  A drag turns the
  globe: the camera's latitude and longitude (`D_8021A920`/`1C`) follow
  the pointer, so that the ground under it stays under it (the screen
  pixels a degree moves the ground are measured with the same matrices
  when the drag starts), and go on a little after it, slowing.  The
  game's camera always eases back to the selected level, so this is the
  one hook in the game's C: `port_globe_view` (`port_game.h`), which
  `func_801F8980` asks each frame (under `TARGET_PC`) for a view to head
  for instead, and how fast.  The turned view stays until a level is
  picked or the keys or a controller are used.  Touch is the same with
  a finger (SDL's finger events; the mouse events SDL makes from touch
  are left out), so a phone's browser turns the globe and taps levels
  (`shell.html` gives the canvas `touch-action: none`).
- **the name entry**: the typing above; a click on a character of the
  wheel types it (its place from the wheel's angle as `func_801EAA7C`
  draws it), a click inside the wheel (where the name is) is Start, the
  right button Escape.
- **anything else** (the title, the intro, the briefings): the arrows and
  WASD are the stick, Enter Start, Space A, Escape and Backspace B, Shift
  R; a click is A.

X, C, Z and IJKL are A, B, Z and the C buttons everywhere but the name
entry.  A key that is a button keeps the button it went down as until it
comes up (Enter as the world map's A doesn't turn into the level's Start
when the level starts under it), and is held for a read at least, so a
tap between two reads isn't lost; the arrows and WASD follow the screen.
A click's press is one read, after a read without it.  The window's
points become the game's 320x240 screen (`host_window_to_n64`,
`video.c`) as the picture is placed: as large as fits, in the middle,
`gfx_wide_off` columns more on each side when it is wide, or the whole
window for the software renderer's 4:3 picture.

`--replay` never takes any of it; `PORT_POINTER=READ:WHAT:X:Y,...` is
the pointer for a test, in any run (headless too): at the READth
controller read, WHAT is `d` down, `m` move (pointing, with no button
down), `u` up, `b` the right button, `w`/`W` the wheel, at X, Y on the
320x240 screen.  `-v` logs what is up at each change; `-v -v` also the
world map's levels on the screen and where the pointer takes a window's
selection (`-v` alone logs each click on the map: where it landed, the
level it took and the nearest open one); in a level `-v` logs the player's vehicle and what W/S are
(A/B or the stick) when either changes.  Checked that way in us.v11, headless: from the title to the
world map by clicks (the intro, the name typed by clicks on the wheel
and confirmed by one in the middle), and with a save: a level picked by
a click and gone into by another, its menu's EXIT pointed at and
clicked, back on the map, and the globe dragged (the paths followed the
pointer to the right); the same log, byte for byte, from all seven
variants.  The port's tests are unchanged: the quick tier on every
variant in us.v11 and us.v10, the TAS on every us.v10 variant.  Not
tried yet: jp, the page (wasm), and by hand a real mouse, keyboard and
touchscreen, driving with the keys among them.

`PORT_MAPCHECK=1` checks the world map's clicks from a save: from wherever
the camera comes to rest it clicks each open level on the screen, each
from every level it can (up to 400 clicks), and logs `mapcheck:` lines:
each reached or `NOT reached`, and at each stop every open level on the
screen with its click zone (the pixels of a 41x41 box around its marker
that take it; a whole 14-pixel circle is 616) and `NOT HIT` where a click
on the marker doesn't take it (only right at the globe's rim, `near the
edge`, where map_hit leaves it out).  `=turn` turns the globe (as a drag
leaves it) to near each level before clicking; `=all` opens every level
first (it writes the save: use a copy); `=fresh` takes the levels marked
open (medal 8) back to found-but-not-marked (0), as they are on the map
after the level that found them until another level is gone into (only
`func_801FE018(8)` marks them, as a level is chosen or at the first map
after the boot), which the clicks once missed: such a level is open to
them by `func_801FE018`'s own test.  The check takes "open" from the same
test the clicks use, so it shows what is open can be clicked, not that
the test is right.  With a save of 8 levels open, in us.v11: 108 clicks,
all reached, plain and turned, deterministic and not, and with
`--interpolate --display-hz 144 --hd-text`; `=fresh`, 3 of them only
found: all reached; with all 58 open, every level reached, the smallest
zone 150 pixels (at the rim).

### Skipping the logos and the introductions

The original lets none of the boot's screens be cut short: the N64 and
Rare logos play their 250 frames each, the title takes Start only 130
frames in (`D_80366A18`, 17E10.c), and the crew's introductions between
the attract mode's demos (mode `1<<48`, a TV and a caption) take nothing.
The port (`port_intro_skip`, under `TARGET_PC`):

- **the logos** (8380.c, `func_801EF4AC`): A, B or Start jumps to the
  logo's fade-out (frame 221, the camera where it ends), so a press
  goes on to the next logo, or from the Rare logo to the title, in 30
  frames;
- **the title**: Start or A from its first frame (00000.c's mode 2),
  to the menu or the name entry as before;
- **the introductions** (17990.c): Start or A goes to the menu, the way
  the demos already do (the same sound and music stop, and a fade).  A
  click there is A (`ui.c` counts the mode as a plain screen; its window
  has nothing to pick).

None of it applies with `--replay` or `PORT_AUTOSTART`, whose input is the
original game's, so the TAS and the quick tier are unchanged.  Checked
headless in us.v11 with `PORT_POINTER` clicks: during the N64 logo (it
ended at its 131st frame, the Rare logo at its 100th), on the title 59
frames in (the name entry), and on Amber's introduction (the name entry,
without a save); before the change each of those clicks did nothing.

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

Every 64th poll also moves `--deterministic`'s clock on by 2 µs (with
the CPU model off, the default and the quick tier's, it is the only thing that does
while the game computes), so the number of loop iterations the N64 side
runs is part of the game's timing: one poll more per audio frame changes
the attract mode's sound.  Code that never waits on another thread is
built without the polls (`BEPASS_NOPOLL=1`; the `nopoll_c` objects in
`port/CMakeLists.txt`, libaudio's for now), so that a replacement of it
needn't loop the way the original does.  Add a file to that list only
after checking it doesn't busy-wait.  (The quick tier's references were
recorded again when libaudio's objects went in, on 2026-10-01.)

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

**quick.**  Deterministic runs (`--deterministic`, the CPU model off,
`--headless`, the software renderer, no save to start from): the attract
mode for 4,000 frames and `PORT_AUTOSTART=1`, `=2` and `=3` for 3,000,
2,000 and 3,000, each with `--save`, `--wav` and a screenshot every 250
frames; for us.v11 and jp, which the TAS doesn't cover, also the attract
mode for 12,000 frames (`attract.long`: the story and several of its demo
levels, about 70 seconds).  Every run also writes the gameplay digest
("The gameplay digest", below), which is compared too.  The same scenarios also run from a resource pack
made from the ROM ("Resource packs": `attract.pack`, `auto3.pack`,
`auto3.pack.nocode`, every hash as from the ROM) and from edited copies of
it (`auto3.pack.edit`, `auto3.pack.hires`, `auto3.gl.pack.hires`).  Their hashes (16 digits of sha1) are compared with
`port/tools/test_refs.json`'s for the build's version; with `--against
BUILD` also with another build's last results.  A `PORT_ROM_DATA` build is also
searched for the ROM's data (`rom_scan.py`, "The data from the ROM"), and
fails on any.  Within the build, where
the executable is the same, `=3` again must give the same:

- with the other thread backend (`PORT_THREADS`): every hash;
- with `--widescreen`, with `--interpolate --widescreen`, with
  `--renderer gl --scale 1`, with `--renderer gl --interpolate
  --widescreen`, with `--renderer gl --aspect 32:9` (the HUD at the
  sides) and with `--aspect 21:9 --hud centre` (SDL's offscreen driver;
  the OpenGL ones left out without libepoxy): the save and the sound.

What's exact: with the timing taken out, every variant plays the same game,
so the save and the sound are the same in all of them, and so are the
screenshots, except where the references say otherwise:

- **layout-dependent** (`layout`, by version and scenario): screenshots
  compared only within a build.  The software renderer's static (the hint
  box's portrait, the story's TV) samples memory that holds addresses of
  the image (see "Threads without ucontext"), so it differs between
  executables; the OpenGL renderer draws it the same everywhere.  None is
  listed now: the lists were the portrait at frame 1500 of `=2` and `=3`
  and the story's TV in `attract.long` (5500 and 8250 in us.v11; 5000,
  5250, 7500, 8000 and 9500 in jp), and since "The front end's waits"
  every scenario is 30 retraces further along at each of those frames
  and no screenshot falls on the static (the portrait is gone by 1500):
  all eight variants of all three versions give every screenshot the
  same.  A change of timing can put one back on it.
- **known failures** (`known`, by variant): the build passes as XFAIL
  while it gives exactly the hashes recorded there, and fails on anything
  else; when the failure goes away it passes with a note to take the
  entry out.  `test.py quick BUILD --known NOTE` records a build's
  differences as one.

`test.py quick BUILD --update` writes a build's hashes as its version's
references; take them from a build that is right (a 64-bit big-endian one
now), after a change meant to change what the game draws or plays.  There
are references for all four versions.

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
(us.v10 by default, `--version`: the stage 2 has to be that version's),
the standard set into `build/test-*/`:
`32`, `64`, `n64` (native-endian 64-bit), `lp64`, `m64` (movable 64-bit),
`mlp64` (movable LP64), `mn32` (movable native-endian 32-bit) and, where
emsdk is (`--emsdk DIR`, `$EMSDK`, or `emcmake` on the `PATH`; a SKIP
otherwise), `wasm` (the WebAssembly build under node, "WebAssembly"); runs
quick on all of them at once, then checks `wasm` against `mn32` in every
hash, the layout-dependent screenshots included (the two have one
layout: "WebAssembly"), and prints a table of every variant's scenarios
against the references; `--tas` then replays the TAS on all of them in
parallel.  `--no-build` uses the directories as they are, `--only` picks
variants.  `test.py table BUILD...` shows every hash of some builds' last
quick runs side by side.  `quick` and `tas` take a WebAssembly build
directory like any other (they run `blastcorps.js` with the node CMake
found, and `quick --against` between it and an `mn32` compares every
hash).  On a 32-thread machine, from empty directories: the seven Linux
builds 38 seconds, their quick tiers 67 seconds together, and with
`--tas` 24 minutes in all; `wasm`'s quick tier takes about 2 minutes
beside them and its TAS 28 to 38 minutes, so the eight with `--tas` take
30 to 40.

Where it stands (main at the time of writing): quick passes in all seven
for us.v10, us.v11 and jp, and in `wasm` for us.v10 (the version it was
run for), with no known failures.  The last one was the native-endian
builds' carrier on the globe: from retrace 1,228 to 1,285 of every
`PORT_AUTOSTART` run (the world map's intro), about 22 pixels of the
carrier came out slightly different in color, with both renderers.  The
display lists and RDRAM were the same by type; the renderer's log of
what it loaded showed five textures differing, hd_front_end 9570.c's
RGBA16 mipmaps (`D_802084F0`, `D_80209028`, `D_80209B60`, `D_8020A698`,
`D_8020B1D0`), which the C declares as `u16` arrays and only hands the
RDP: they were left in host order.  `port_native_fixups` puts them back
into the N64's, as it does hd_code's (below).

With the tier running under the variants' table, us.v11 and jp: every
variant equal to the 64-bit big-endian references but for the
layout-dependent static (`~`), in all five scenarios.  The
quick tiers take about two minutes together with `attract.long`.
(Since "The front end's waits", all eight variants, `wasm` included,
equal the references in every hash, `=`, for all three versions.)

(jp's first runs found its IDO asm, translated, doing what Rare's code
doesn't: the movable builds stopped at the pak thread's entry, which only
the asm takes the address of; the LP64 build crashed on display list slots
passed as `Gfx **` and on pointer arrays read by words, and lost the
Japanese text; `sprintf` from the asm read four arguments of five; and the
native-endian builds read a byte of an `s16` and byte pairs declared as
halves at their big-endian places.  "Other versions" has what changed.)

(The suite's first run also found the 32-bit builds reading the glue's
narrow results as all of `eax`: three of the game's C functions the
translated engine calls return `u8`, and i386 leaves the rest of the
register as it was, which changed the attract demo from its 1,563rd
controller read.  `gen_glue.py` declares them by their type now.)

The TAS: all eight exact, all 125,297 reads matched, none skipped, no
mode forced, 57 platinum and the same save (the native-endian builds'
late drift is gone: "The native-endian build"); `wasm`'s replay log is
`mn32`'s line for line.  Eight replays at once take 30 to 40 minutes on
a 32-thread machine (`wasm`'s is the slowest; the Linux ones 18 to 25).

### The gameplay digest

The hashes above check that nothing changed at all: RDRAM, the save, the
sound and the pictures follow the CPU model's timing (lag frames, the
retrace count, `osGetTime`), so a change that keeps the game and changes
its timing (dropping the CPU model, stripping the engine's scaffolding:
DISTRIBUTION.md's optimization pass) changes them all.  The gate for such a
change is what a player could notice, which is what the gameplay digest
holds.

`PORT_DIGEST=FILE` (`port/host/digest.c`) writes, at every controller poll
(`host_controller_poll`, once per game frame), a line with the poll, the
mode (`D_80364A90`), the level (`D_802E8BDC`) and the game's frame in the
mode (`D_80358064`), then the fields that changed since the line before (a
line marked `*`, at the first poll of each mode or level, has them all;
`key=~` is a field gone).  It only reads game memory, at the poll, from the
host side, so it charges nothing to the CPU model and changes no other
output (the quick tier's hashes are the same with it).  The fields, read
from the variables the headers in `blastcorps/include/game` and
`port/engine` describe:

| field | what | from |
|---|---|---|
| `p` | the player's position (`<< 5`) | `D_803643E0..E8` (game.h) |
| `pv` | the player's vehicle type | `D_80364456` (vehicle.h) |
| `vT` | every vehicle module the level's vehicles use (type `T`, hex): position, headings, speed; `vFF` the missile carrier, `vB.0..2` the barges | the `Vehicle` list `D_80364460` to `D_803649D0` for which types; each module's `VehicleState` (`unk4C`, `unk4E`, `unk76`) and position (vehicle.h's table, `port/engine/62740.c`, `75490.c`) |
| `bN` | building N: destroyed, position, each damage group's 0..100 | `Building.unkEA`, `x..z`, the bytes from `0xEC` (`unkE9` groups), `D_803F4030` to `D_803F7654` (objects.h, `port/engine/77E20.c`, `buildings.h`) |
| `st` | the level's stats: damage `$`, buildings down, `cr`, RDUs, medal | `D_8036EA70` (`LevelStats`: `ip`, `bd`, `cr`, `rt`, `coin`; level.h) |
| `of` | the totals: buildings, `cr`, RDUs | `D_8036EB92`, `D_8036EB93`, `D_8036EB90` |
| `won`, `lost` | the carrier got through (or the goal), the level lost | `D_803643DA`, `D_803643D9` (`port/engine/75490.c`) |
| `dmg` | the damage so far | `D_803649F0` |
| `ammo` | the Ballista's missiles, the Sideswipe's hydraulics | `D_803F8B72`, `D_803EDC00` |
| `rdu`, `box` | the RDUs and ammo boxes collected, a bit each | `Rdu.collected` of `D_8036BED8[D_8036EB90]`; `AmmoBox.collected` of `D_8039AF00[D_8039B068]` (objects.h) |
| `tnt`, `blk` | the TNT crates (active, position, fuse) and the blocks (position, in its hole) | `D_8039B070[D_8039B610]`, `D_8039C550[D_8039C710]` |
| `med`, `units`, `gs` | the current player's medals (60, hex), units and game state | `PlayerInfo` `D_80364AF0[D_80364AE8]` (player.h) |
| `clk` | the scheduler's retraces (the clock) | `D_803156C4` |
| `t`, `tc` | "TIME IN LEVEL"'s retraces, the level's time (the clock) | `D_803156C0` (`Sched.unk280`), `D_8036EA70.tc` |
| `rng`, `fc` | the random state, a frame count (internal) | `D_8036B968`, `D_80358060` |

Everything but `clk` and `rng` is written only in the modes that run the
level's frame (`func_802475D8`: the levels, the attract demos and the
intros; not the menus, the map, the logos or the results).

`port/tools/digest_cmp.py A B` compares two digests by what the game did,
not when: the modes and levels in order (difflib, so a mode one run visits
and the other doesn't is reported), and within a mode by the game's frame
(a new stretch each time the count starts again).  Runs whose timing
differs line up frame by frame.  In a level, the first frame where a
gameplay field differs is the failure, reported with the level, the frame,
both runs' polls and the field's two values; at the end of every mode its
outcome (`st`, `won`, `lost`, `of`, `med`, `units`, `gs`) is compared too
(not where a run ends inside it, as `--frames` cuts it off).  The clock's
fields are never a failure: each level's retraces over the frames both
runs have, its "TIME IN LEVEL" and `tc` are listed side by side, with the
total; the internal ones are a note where they differ first.  Exit status
1 on a gameplay difference.  `--hash` prints a digest's gameplay hash (the
gameplay fields only, a level's frames that change none of them adding
nothing, so a level that waits longer at its end hashes the same), and
`--levels` lists the levels a digest played, with their frames and times.

The reference digests themselves (`build/digest-refs/<version>/`) are made
again, on a build that plays as the references, with `--save-digests`:
`port/tools/test.py quick BUILD --save-digests` (the quick scenarios) and
`port/tools/test.py tas BUILD --save-digests` (`tas.digest`) copy each
run's digest that hashes as `test_refs.json` says, and change nothing else.

`test.py` writes a digest in every quick scenario and in the TAS replay,
and compares its gameplay hash with `test_refs.json`'s `digest` (by
version; the TAS's is `tas`); a scenario run against a base one (`auto3.wide`
against `auto3`, ...) must have the base's.  The digests themselves are the
game's state, the ROM's data at work, so like the TAS's logs they stay out
of the repository: `quick --update` and `tas --update` record the hash in
`test_refs.json` and copy the digest to `build/digest-refs/<version>/`, and
where a hash differs and that copy is there, `digest_cmp.py` says where.
`--gameplay` (quick, tas, variants) makes the digest the gate: a hash that
differs from the references is reported (NOTE) instead of failed, and so is
the TAS's save, which holds the levels' times.  (A build's own scenarios
are still compared with each other by every hash: they share its timing.)

What it showed (us.v10, the 32-bit build, main as of 2026-10-02):

- Two TAS replays give the same digest, byte for byte (125,582 polls,
  14 MB, 468 modes of which 232 run the level's frame, 106,922 level
  frames; `digest_cmp.py` takes a second over two of them, and the replay
  took its usual 21 minutes); so do two runs of a quick scenario.  The
  TAS's gameplay hash is `7ec6ef73a5265afa`.
- The other variants give the same gameplay hashes as the 32-bit build in
  every quick scenario (`lp64`, `mn32` checked: native-endian memory,
  LP64 pointers, the movable layout).
- A one-frame change, the Ramdozer's x moved by one unit (`<< 5`) at poll
  1,400 of `auto3`, is found at the next poll: Simian Acres, frame 363,
  field `p` (and everything after it, not reported).
- The attract mode with the CPU model on (`PORT_COUNT_PER_OP=2`, lag
  frames) against the reference (off): the same gameplay in the 1,477
  level frames both have, while Argent Towers' demo took 1,385 retraces for
  533 frames where the reference took 1,064 (and `rng` differs from the
  first frame: it is seeded from the clock).  The demos read their input
  by frame, so they are what lag can't change.
- us.v11 and jp (references since "The front end's waits"): `auto1` to
  `auto3` have us.v10's gameplay hashes (the same Simian Acres, the same
  input); the attract mode differs from us.v10's in jp (`attract` and
  `attract.long` only match within a version).
- `PORT_AUTOSTART=2` and `=3` press their buttons by the retrace count, so
  a change of timing changes their input and their gameplay: their digests
  will differ after such a change by design, and have to be recorded
  again; the attract mode and the TAS (input by frame) are the scenarios
  that check it.

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
`D_803063D4`, `ASM_DATA_BE`).  What only the RSP and the RDP read is
typed by hand: the carrier's explosion's dome, `D_802FFF38` (`Vtx[179]`,
`ASM_DATA_LAYOUTS`), which fieldscan took for words, so that in host
order each vertex's x and y traded places and z read the flag: the dome
was a flat sheet (issue #1); and its texture `D_802FF738` (bytes).  The
display lists that use asm data directly or through a segment were
found by logging every `G_VTX`'s and `G_SETTIMG`'s address over the
attract mode and three levels, and by a scan of the asm data for
display-list words: those two are all.  An array of unknown length (`D_8020C070[]`)
runs to its file's end, over the labels splat made inside it.  The data
islands in hd_code's `.text` are laid out by their readers
(`7D9D0`, `800DC`, `8E910`'s per-level tables); the rest stays as its
directives say.  A record type fieldscan found repeats up to the next
label and no further (`D_80305D74`'s 21st record would otherwise take
`D_80305DF0`'s 0xFF terminator as the high byte of a `u16`).

**The translated code** (`RECOMP_NATIVE_ENDIAN`) takes, from
`tools/recomp/native_sites.txt`, the instructions that move data at
another width than its type: `be` (the texture decoders' streams, LUTs and
texels; the chance tables), `x1`/`x2`/`x3` (a byte or half inside a wider
field: the matrix routines, the effect records' halves and bytes, the
bytes of words the N64 reads at the word's address), `h2` (a word that
is two halves).

Some data is a word in one place and smaller fields in another because
the handwritten code stores words over it: those are kept as host-order
words, and the smaller accesses go to the address ^ 2 or ^ 3, so that
every access sees what it sees on the N64.  The effect records (a
model's 0x30/0x34 records, `D_803F3FF8`, `D_803F3968[30]`) are fourteen
words: the list at `D_803F3910` (ten pairs of words, `func_802BEA30`)
runs over its end into `D_803F3968[0]`, and the effect code then reads
the halves and bytes of what it stored.  The same goes for the count
and flag at 0xF8/0xF9 of the walls (`D_803BD310`, 0xFC-byte records; a
record's 61st wall is stored over them), the level `D_802E8BDC` that
`func_802AF4BC` reads with `lbu` (its high byte: 0 on the N64) and
`D_803649E8`, which `func_802BC5E0` sets with `sb` and the C reads as
an `s32`.

**The C** needed three changes of its own: `func_80200714` (hd_front_end
196F0.c) tints big-endian texels, through `IMG_RD`/`IMG_WR` under
`TARGET_PC && PORT_NATIVE_ENDIAN`; the textures the C declares as `u16` or
`Vtx` arrays (hd_code's six and hd_front_end's five, before `overlay.c`
keeps the front end's `.data` for its reloads) are put back into the
N64's order at boot, and
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
  (23,216,880 calls); `PORT_AUTOSTART=1` and `=2`, 6,000 frames
  (7,845,941 and 7,905,194);
- the TAS (us.v10), replayed with `PORT_COUNT_PER_OP=0`: the same log, the
  same pace (`PORT_PACE`: the retraces, frame, mode and audio samples at
  every controller poll) and the same save, all 125,897 reads;
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
- the profiler's table for all three runs and the TAS holds only rows
  known to be harmless: the RSP reading stale heap through segment 9,
  `D_803ED3B8`'s bytes compared with a -1 sentinel as a word (and those
  words spilled to the N64 stack and loaded back), `func_8026A5CC`'s `u64`
  copy, `osContGetQuery`'s stale display-list words, the C's byte reads that
  clang merges into a halfword (`func_8026BCE0`'s of texels, at odd
  addresses too), and the host stack's locals the C writes without the
  profiler seeing it (`guNormalize`'s arguments);
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
ignored `osSetIntMask`, "The platform layer").  It plays it as exactly as
the big-endian build: all of the movie's reads matched, none skipped, the
player where the movie has it at every read.  Getting there took the
train stops' `u32` that `func_8029DA90` reads as two halves (`x2`; read
as a host-order word, the stop's range was wrong, and from the movie's
read 35,680 the player drifted), their byte pairs stored as halves and
read as bytes (`be`), the words stored over smaller fields above (with
`D_803F3968[0]` misread, the effect the N64 starts in it wasn't, nor did
it take its random numbers), and the asm data's `D_80305DF0`.  The two
builds' CPU models count the C differently (the IR is optimised with and
without the byte swaps), so with the default `PORT_COUNT_PER_OP` their
timing differs a little (more retraces given anyway, no mode switch the
movie's rather than the port's); with `PORT_COUNT_PER_OP=0` they are the
same run.

What's left: levels and vehicles the runs don't reach are typed from the code but
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
  `&D_802F49F4[i]`, at ...944, lost its 4 and divided by zero); and
  clang 23's InferAlignment raises a definition's alignment again after
  the SLP vectoriser has read it in wide pieces (23C20.c's `float[4][4]`
  `D_8036B8C8` went to 32, which the 32-bit link couldn't place at
  ...818), so KeepAlign, at the optimiser's end, puts every global back
  to what BEPass gave it and caps the accesses in the functions that use
  one it moved at 8 bytes' alignment;
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

**The arena** is one `mmap` of 29 MB (`port_arena_init`, `host/runtime.c`,
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
  one);
- `0x1C00000`: the stacks of the C's locals that live in memory, 16 of
  64 KB
  (`PORT_ARENA_LOCALS`, below).

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
- lays out the locals left in memory (an alloca, or a struct passed by
  value whose address goes to a call, into memory or into an integer: the
  escaping ones, and the few arrays and structs `-O2` kept in memory
  without their escaping) on a stack of its own: each function's are one
  frame, at offsets the pass decides from the IR (the target's frame
  layout has no say), taken from the running thread's slot of the locals'
  stacks at the entry and given back at every return.  `port_locals_sp`
  is the pointer (an N64 address; `threads.c` keeps each thread's while
  it doesn't run, and a thread starts at the top of its slot), and a
  frame outside the running thread's slot (`port_locals_end`), or outside
  a thread, stops the port (`port_arena_bad_local`).  348 locals (9 that
  don't escape) in 160 frames in us.v10's 32-bit build, the biggest 1,360
  bytes; 370 in 167 in LP64.  So the addresses of locals the game stores
  in RDRAM, and what is left in the frames between calls (which the
  decompiled C reads where IDO's code read an uninitialized local), are
  the same wherever the module is code-generated: the WebAssembly build is
  the Linux 32-bit native-endian one's to the byte ("WebAssembly").  (Not
  in a variadic function, whose `va_list` is the host's.);
- makes what is left undefined 0 (an `undef` or `poison` operand: an
  argument the caller never set, a function that falls off its end, a phi
  from a path that set nothing: 22 in us.v10's 32-bit build, 183 in the
  64-bit one), which each backend would make something of its own, i386
  whatever was in the register or on the stack;
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
  i386 got narrow results wrong (below).  A call of the callee's type
  whose narrow result it says is extended the other way has the caller's
  extension of the result made the callee's: the N64 used `v0` as the
  callee left it, and so does x86, which takes the call's word for it,
  where WebAssembly extends again.  There were three, 1D990.c's `s16
  func_8028604C(s32)` for 409D0.c's `u16 (u32)` (the level time's cap,
  59,999, came out negative in wasm, and a countdown ran out); that
  declaration is the definition's now, as are `func_80272C5C`'s (`s8` in
  seven files for a `u8`, which the call repair already covered), and the
  pass is the safety net (none left in us.v10);
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
(refused with it) and `build_cmp.py rdram`'s view of the stacks.  (wasm-ld
found no call to the glue whose type isn't `entry.c`'s.)

For **macOS** (arm64) the movable build is all there is; what it took and
how it was checked without a Mac is under "Other hosts", "macOS".

**WebAssembly** is done on top of this: see "WebAssembly" below.

The LP64 build doesn't finish the TAS at the moment, with or without
this: it stops with SIGFPE at about the movie's read 1,620 (main's
`build/p10lp` too), which is being looked at separately.

### The data from the ROM (`PORT_ROM_DATA`)

The arena image is all of the game's data a movable build carries (the
asm data, the islands, the decompiled C's initializers: docs/DISTRIBUTION.md,
1a-1e), and nearly all of it is the ROM's.  With `PORT_ROM_DATA` (on by
default in a movable build; `-DPORT_ROM_DATA=OFF` carries the image as
before) the executable has none of it: it carries a list of operations
that make the image from the user's ROM at startup, which the port needs
anyway.

- **The build.**  port-arena writes the image to `gen/arena.img`
  (`-port-arena-image`, in every movable build) and leaves its runs out of
  the module (`-port-arena-no-runs`).  `tools/rom_data.py` lays the ROM's
  code modules out at their N64 addresses as the game loads them (init as
  it is at `0x8021ED00`; hd_code's `.text` and `.data` gzip members
  inflated at `0x802447C0`, hd_front_end's at `0x801E7000`, each `.data`
  right after its `.text`), from the version's `baserom.<version>.z64`
  (`PORT_BASEROM`, sha1-checked) and the top-level map's `_ROM_START`s.
  It covers the image greedily with copies from that source: from the
  same address first, as it is or with every 2-, 4- or 8-byte unit
  reversed (native-endian data, 64-bit scalars' words); from elsewhere
  (at least 8 bytes, searched: the string literals and statics port-arena
  moved above `0x400000`, LP64's outgrown variables); and literal bytes
  for what neither gives.  It decodes the result as the host will, fails
  the build unless that is the image to the byte, and writes
  `gen/romdata_ops.c` (the operations, the members' ROM ranges and a hash
  of the image) and `gen/romdata_report.txt` (the literal bytes by
  variable, from `PORT_ARENA_MAP`).
- **At startup** (`port_arena_init`, `host/romdata.c`): the ROM's sha1
  has been checked (`main.c`, every build: `PORT_ROM_ANY=1` lets another
  ROM through with a warning); the four members are inflated (an inflate
  of the port's own, RFC 1951, its tables computed rather than written
  out), the operations applied into the arena, and the hash (FNV-1a over
  8-byte words) compared, so a ROM that inflates to other data stops the
  port there rather than playing wrong.  7 ms on a desktop.
- **What is literal**, us.v10 (64-bit big-endian, 32-bit native-endian,
  LP64): 2,932, 3,164 and 3,444 bytes, in operations of 11 K, 24 K and
  26 K (the image's runs were about 190 K); 1.24 MB of the image comes from
  the ROM (zeros between included), 21-24 K of it from other addresses.
  The literal bytes are the port's: pointer words to the data port-arena
  moved (the game's string tables, 1.7 K), clang's lookup tables for the
  game's switches (`switch.table.*`, 0.5-0.7 K), `port/src`'s strings and
  `__func__`s (0.7 K), LP64's native pointer tables (0.4 K) and a few
  words of `port/src`'s own variables.
- **The RSP microcode's text** (hd_code's after `ldiv`, hd_front_end's
  last 0xFB0 bytes of `.text`, 18.3 K) is zeros in every build now
  (`asm2x86.py`'s `RSP_TEXT`): the HLE never runs it and nothing reads
  it.  Its data segments stay, as the ROM's: `aspmain.c` reads the audio
  microcode's resampling table.
- **Checked** with `tools/rom_scan.py` (`test.py quick` runs it on every
  `PORT_ROM_DATA` build and fails on any hit): no stretch of 24 bytes or
  more of the ROM, or of its inflated modules as they are or with 2- or
  4-byte units reversed, is in the executables, the .wasm or the .js.  The
  quick tier and the TAS play as without it (the image is the same, byte
  for byte).

The non-movable builds still carry the data (about 158 K in stretches of
24 bytes or more in the 32-bit build's executable): their game variables
are the executable's own sections at N64 addresses, initialized by the
loader, and the C's are swapped by constructors before `main` has the ROM;
see docs/DISTRIBUTION.md.

## Resource packs

The port can take a resource pack instead of the ROM: a zip of the ROM's
assets as editable files (docs/ASSETS.md, "The pack"), which the user makes
from their own ROM and may edit.  The point is twofold: artists can change
textures, levels and sounds without the Python tooling, and the port shows
that it needs the ROM's bytes only through those formats.

```
port/make_pack.py baserom.us.v10.z64            # blastcorps-us.v10-pack.zip (17 s, 12 MB, 8,413 files)
port/make_pack.py baserom.us.v10.z64 --no-code  # ... without the code modules
port/make_pack.py baserom.us.v10.z64 --dir mypack   # the tree, unzipped
build/port-us.v10/blastcorps blastcorps-us.v10-pack.zip   # or --pack FILE; a directory works too
```

A file is a pack when it starts with a zip's signature or is a directory
with a `pack.yaml`; anything else is taken as a ROM, as before.  The page
(port/web/shell.html) takes a pack in the same file picker as the ROM (or
`?pack=URL`), keeps it in IndexedDB at `/save/pack.zip` beside the ROM, and
plays it when "play the resource pack" is ticked (the default once there is
one; "Forget the pack" drops it); with a pack and no ROM it never asks for
the ROM.

**At startup** (`host/pack.c`, before anything reads the ROM) the port makes
the ROM's image from the pack, as `tools/assets.py build` does, all in C:
`host/pack_zip.c` reads the zip (stored and deflated members, through
stb_image's zlib decoder, CRCs checked), `host/pack_yaml.c` the YAML,
stb_image the PNGs, and `third_party/gzip-1.2.4` (gzip 1.2.4's
`deflate.c`, `trees.c` and `bits.c` as they are, with a memory front end)
deflates the gzip members exactly as Rare's tool did.  Every segment goes
where the port's link has it (`romtab.h`), but the display lists and the
models, which the code doesn't name, follow what comes before them (docs/
ASSETS.md).  Then the rest of the port reads the image as it reads a ROM:
the PI DMAs, `PORT_ROM_DATA`, the ROM's sha1 (an edited pack's image isn't
the ROM's; the port says what was edited instead of refusing it).  It takes
about 570 ms natively and 650 ms in wasm (us.v10, `-v` prints it), most of
it deflating the 738 members.

**Edited textures** keep the ROM's stream.  Rare's compression is lossy and
can't be reproduced (docs/ASSETS.md), so a changed PNG can't become the
same kind of stream at the same size, and a stream of another size moves the
texture data and takes the decoder another time.  Instead the image keeps
the ROM's stream, the game DMAs and decodes it as always, and the host puts
the PNG's texels over what it decoded: `host_tex_decoded(id, dst, size)`
after each of 5BF40's three decodes (`port/engine/5BF40.c`), and for the
decode queue `host_tex_queued(slot, id)` when func_802A1074 queues one and
`host_tex_decoded_slot()` when 60F60's func_802A57AC does it.  These are
host calls: they cost the game nothing (`ENGINE_BLK` charges what the
original ran, as before), so the game takes the same time and makes the same
choices; only the pictures change.  The texels are in the N64's byte order
in every build (`texture.h`), so they are copied as they are.  A PNG with
no stream beside it (new art) is compressed by the port instead, with
`blast.encode`'s greedy encoder.

**Higher resolutions.**  A texture's PNG may be k times as wide and high
(k = 2, 3, ...).  The game gets it averaged down by k x k boxes, so the
game and the software renderer see a texture of its own size; the OpenGL
renderer draws the full image.  `host_tex_decoded` remembers where in RDRAM
such a texture was decoded, and `gfx_gl.c`'s `tile_texture()` asks
(`host_tex_hires()`) for the address its TMEM came from (`gfx_tmem_src`,
which `--hd-text` uses for its glyphs): if a tile of the texture's size reads
an edited texture that is still there (its texels compared), the unit gets
the high-resolution image (one GL texture per texture, kept) and `u_hd =
-k`.  `texel_hdc()` then samples it texel by texel: each sample's texel of
the tile's own size is wrapped, mirrored and clamped as the tile says, and
the k x k texels under it are the image's, filtered between neighbours
(bilinearly, for the N64's three-point filter too) or not, as the tile is.
Only the first image of a texture (not its mipmaps) and only whole-texture
tiles are drawn this way; anything else draws the averaged texels.
Checked by eye (a 4x grass with lines a quarter of a texel wide, tiled
across Simian Acres' fields at `--scale 4`); the quick tier checks that it
changes neither the save nor the sound (`auto3.gl.pack.hires`).

**Other edits** (levels, models, sounds, images) are rebuilt into the image
and change what the game loads: they have to fit the room their group had
(docs/ASSETS.md); a grown gzip member gets deflated at -9 first.  Checked by
hand: Simian Acres' starting vehicle moved 300 units in `levels/chimp.yaml`
(the level from another place on; the save the same), half of
`audio/music.tbl` zeroed (only the sound differs).

**Without the code.**  The port's engine is its own now, so the code
modules' bytes are needed for nothing but the data in them.  A pack keeps
that data apart (`data/`, docs/ASSETS.md; `tools/assetlib/codemask.py` says
what is code), and the movable builds make their data from it
(`pack_data_source()` in `host/romdata.c`, which zeroes the code ranges of
the ROM's modules too, so the operations `rom_data.py` writes never
depended on code: us.v10 m64 3,141 literal bytes where it had 2,932).
`make_pack.py --no-code` leaves `rom/`'s code out, and the port stands in
for the front end's members (its `.text` as zeros, its `.data`): the game
still DMAs and inflates them when it loads the front end, for the time that
takes (port/src/overlay.c), and that is the one difference.  Inflating
zeros takes less CPU time than inflating Rare's code, so the front end's
loads are quicker in a run the CPU model times.  The quick tier, which
doesn't count CPU time, is the same in every hash; the TAS from the pack
without the code (us.v10, the 32-bit build) still matches all of the log's
125,297 reads with none skipped and no mode forced, 57 platinum and the
reference save, with the same report as from the ROM (50 retraces given
anyway): the loads happen at menus, where the replay waits for the log's
reads anyway.  A free-running game's frame timing in those loads is what
differs.  What a pack without the code still holds of the ROM: the assets
and the modules' data (`data/`: 207 K in us.v10, the game's tables,
strings, display lists and the RSP's data).  Making the front-end load cost
exactly what it did would need the inflate's cost recorded per version (its
instruction counts and poll points), about 2-4 agent-hours.

**Checked:**

- `make_pack.py` on all four ROMs: every gzip member deflates back through
  `gzip124_compress` (738 of 738 each); `host/pack_yaml.c` reads the 64 YAML
  files of a us.v10 pack exactly as PyYAML's `BaseLoader` does.
- `test.py quick` (every variant, as part of its scenarios): `attract.pack`
  and `auto3.pack` play from the pack and must give every hash the ROM's
  `attract` and `auto3` give; `auto3.pack.nocode` from the pack without the
  code, the same; `auto3.pack.edit` from a copy with texture 60F (Simian
  Acres' grass) painted magenta: the same save and sound, at least three
  screenshots changed, and every pixel that changed moved toward magenta
  (more red and blue than green), nothing else (7 screenshots, about 21,000
  pixels at frame 3000); `auto3.pack.hires`, the same with the painted
  PNG four times as wide and high, the same as that, and `auto3.gl.pack.hires`
  draws it with OpenGL: the save and sound as `auto3`'s.  The packs are made
  once into `build/pack/`.
- The TAS from the pack (`test.py tas BUILD --pack`, or `variants --tas
  --tas-pack`): us.v10, 32-bit and wasm: all of the log's 125,297 reads
  matched, none skipped, no mode forced, 57 platinum, the reference save
  (21 and 28 minutes); and the 32-bit build from the pack without the code,
  the same.
- Where the quick tier ran with these scenarios: us.v11 all eight variants
  (`variants --version us.v11`, the wasm one included), jp m64, us.v10 32
  and m64: every one passed.  eu (all eight variants) passes them too.
- The page in headless Chromium (Playwright, SwiftShader): with no ROM in
  IndexedDB, the edited pack chosen in the picker plays into Simian Acres
  with the magenta grass, and after a reload the pack is still there and
  "Play" starts it.

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
There are three backends:

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
  the game's frames.  The default on macOS (where ucontext is deprecated)
  and musl (which has none); emscripten can build it (node only).
- **asyncify** (`fiber_asyncify.c`): emscripten's fibers
  (`emscripten_fiber_swap`, Asyncify), all on the one thread, in the
  ucontext backend's shape.  The WebAssembly build's default ("WebAssembly").

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
every dump compared.  (In the movable build the C's locals that live in
memory aren't on these stacks at all: port-arena gives them a stack of
its own, laid out the same on every target, "The movable build".)  (The
same run also showed that the LP64 build's
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
Not done: the WebAssembly build has Asyncify anyway (for the fibers), and
so the loop gives the page its thread back where it would wait
("WebAssembly", and "Performance" for how it waits there); the loop is
as it was.  What the fibers needed there
(as it was planned; "WebAssembly" has what became of it):

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
undefined, which takes the non-Linux branches.  There is no macOS SDK
here to try that; emscripten: "WebAssembly".

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

## WebAssembly

The movable 32-bit native-endian build, compiled for wasm32 with
emscripten: headless under node (for the TAS and comparisons), or a page
that plays in a browser (WebGL 2, WebAudio, the save in IndexedDB).  Under
node it plays exactly as the Linux build it is compiled like (`mn32`):
the same RDRAM, save, sound and screenshots, and the TAS replayed read
for read (below; `test.py variants` checks it).

```
# emsdk anywhere (git clone https://github.com/emscripten-core/emsdk;
# ./emsdk install latest && ./emsdk activate latest), then in its shell:
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -S port -B build/wasm -G Ninja -DPORT_VERSION=us.v10
cmake --build build/wasm
node build/wasm/blastcorps.js --headless --deterministic --frames 3000 --screenshot shot baserom.us.v10.z64

emcmake cmake -S port -B build/wasm-web -G Ninja -DPORT_VERSION=us.v10 -DPORT_WASM_TARGET=web
cmake --build build/wasm-web          # blastcorps.html, .js, .wasm: serve them over HTTP
```

**The build.**  `emcmake` makes CMake's compiler emcc, which builds the
host side and the translated engine.  The N64 side can't go through
emsdk's clang, which can't load BEPass (it ships no LLVM headers), so it
goes through the clang of the system's `llvm-config` (`PORT_N64_CC`,
`port/tools/n64cc.py` as the compiler launcher), and BEPass is built with
that LLVM's clang++.  Its bitcode is **i386's**, exactly as the Linux
32-bit native-endian movable build compiles it, and the arena link makes
the linked module wasm32's (`-port-arena-triple`, `-datalayout`: the same
layout, 4-byte pointers and longs, 8-aligned doubles and long longs as
`-malign-double` has them; the i386 CPU, feature and stack-protector
attributes go).  Compiled for wasm32 from the start, `-O2` made other IR
(switch tables, where x86 has them and wasm doesn't), so other
instruction counts and another layout of the arena's extra data, and a
replay parted from the Linux build's in the first level.  `llc` (the
system's, LLVM 22; emsdk's wasm-ld is 24 and links it) makes a wasm
object.  Two more things port-arena does there:

- `-port-arena-typed-calls`: WebAssembly traps on a call through another
  type than the callee's, and the game calls through values of other
  types (a thread entry defined without its argument, K&R handlers).
  Each function used as a value gets a thunk per type it is called
  through (converting as the call repair does), and `port_fn_typed(address,
  type)` (`runtime.c`) picks it; `port_fn`'s table has the host's type,
  `void(ptr)`: 30 functions, 8 types, 236 thunks in us.v10.
- `-port-arena-x86-fptoint`: a float to integer conversion out of range
  is poison to LLVM, and each target gives what its instruction does.
  x86's `cvtt*` give 0x80000000 (and i386's narrow and unsigned
  conversions go through them: a negative float to an unsigned type
  wraps), which is also what mupen64plus on x86 gave the game when the
  TAS was made; WebAssembly's saturate.  The conversions are made to give
  x86's results on any target.

No `-Wl,--wrap`: wasm-ld's would wrap the N64 object's own references
too (the `__wrap_`s' calls of the real functions among them, which
recursed); the glue calls the `__wrap_`s itself, as in every movable
build (`gen_glue.py --wrap`, "macOS").  The arena is `mmap`'s (emscripten's comes from
`malloc`), with the fibers' stacks in it as everywhere.

What emscripten's libc does differently and the game saw: `sprintf(buf,
"%s ...", buf, ...)`, the world map's way of building its lines
(`hd_front_end/1C40.c`), which libultra's `_Printf` and glibc's do as
they go and musl's garbles: `n64_sprintf` (`libc64.c`) formats apart and
copies, the same output on glibc.  And `port_in_rdram` is false before
the arena exists (the ROM file, read first, lies below 28 MB in linear
memory, where an unset arena said it was RDRAM, so the ROM got
byte-swapped as a `.n64` one).

**The threads** (`PORT_THREADS`): `asyncify` by default, emscripten's
fibers on the one thread (`fiber_asyncify.c`, `-sASYNCIFY`; a suspended
fiber's wasm frames go to a 512 KB buffer of its own, its C frames stay
on its arena stack), or `pthread` (node only: a worker per fiber, and the
loop on one too, `-sPROXY_TO_PTHREAD`, so that the page's thread is free
to start them).  Asyncify is the faster of the two (the handovers between
workers cost more than the instrumentation), and needs no
`SharedArrayBuffer`, so no COOP/COEP headers: the page is plain files.
It doubles the module (6.9 MB against 3.1 MB; 3.5 MB at `-O2`).  The
loop hands the page its thread back where it would wait (Asyncify
unwinds the loop's own frames, which are few: the fibers are
elsewhere).  In the page it runs `PORT_PACED` ("Performance"): the time
inside a retrace is virtual, and each retrace waits for the display's
next frame (`requestAnimationFrame`); under node, and with
`PORT_PACED=0`, it `emscripten_sleep`s in place of its `nanosleep`s, and
after a retrace when it hasn't for 12 ms, so a busy game doesn't hold
the page; with `--deterministic` it never sleeps, and under node that
costs a millisecond now and then.  It is still the loop of `main.c`, not
a frame callback (`emscripten_set_main_loop`), which Asyncify made
unnecessary.

**Headless under node** (`PORT_WASM_TARGET=node`, the default): the
files are node's (`-sNODERAWFS`), and SDL isn't started (it has no
offscreen driver there): the software renderer draws, and screenshots,
`--wav`, `--save`, `PORT_DUMP`/`PORT_REPLAY_DUMP` and `--replay` work as
on Linux.

**In a browser** (`PORT_WASM_TARGET=web`): `blastcorps.html` from
`port/web/shell.html`.  The ROM comes from a file picker (or `?rom=URL`,
fetched) and is kept in IndexedDB with the save (IDBFS at `/save`,
synced after the game writes the EEPROM), so the next visit only needs
"Play"; `?args=` and `?env=` pass options and environment (`?args=-v`,
`?env=PORT_AUTOSTART=3`), and the page has `--interpolate` and
`--hd-text` checkboxes and a Full screen button.  The OpenGL renderer runs on WebGL 2
(`gfx_gl.c`: GLSL ES 3.00, `EXT_depth_clamp` where the browser has it),
SDL's keyboard and gamepads are the input, and the sound goes to an
`AudioWorklet` of the port's own (`audio.c`: on the browser's audio
thread, the game's samples as they are, about 37 ms queued, steered
against the drift between the display and the sound device; "The page
outside the renderer"), or where the page has none (no secure context:
plain HTTP from another host than localhost) through SDL's WebAudio (a
`ScriptProcessorNode`, whose callback runs on the page's thread between
the loop's turns: 1024-sample buffers).  The page resumes a suspended
`AudioContext` on any key or click.  The canvas
fills the page above its panel, by CSS; SDL (`SDL_WINDOW_ALLOW_HIGHDPI`)
makes its pixels the display's for that size on each of the window's
resize events, and the page passes on any other change of its stage
(the panel's, full screen's) as one (a `ResizeObserver`).  The picture
then takes the canvas's shape (`--aspect window`, "Widescreen") and as
many times 240 lines as the canvas has, as far as about 1.3 million
pixels (`--max-pixels 1300000`, the page's; "Performance").

**How it was checked**, us.v10:

- `test.py`'s quick tier under node against the Linux 32-bit
  native-endian movable build (`mn32`): the attract mode and the three
  `PORT_AUTOSTART` runs, 2,000 to 4,000 frames each with the save,
  `--wav` and a screenshot every 250 frames, and the widescreen run's
  save and sound, every hash the same, the hint box's layout-dependent
  static included ("Testing the port").  Linux took 12.2 s for 3,000
  frames of `PORT_AUTOSTART=2`, the asyncify build 16.7 s (5.6 ms a
  frame, 180 a second; about 1.4 times native), the pthread one about
  20 s.
- The TAS under node (`--replay`, the asyncify build): 57 platinum, the
  reference save, and the Linux build's report: "125297 of the log's
  125297 matched (0 skipped)", no mode forced, 37 retraces given anyway,
  the replay log `mn32`'s line for line.  RDRAM the same as `mn32`'s at
  every `PORT_REPLAY_DUMP` compared (the log's reads 2,000 to 125,000),
  and the CPU model's instruction counts and clock the same at every one
  of the 125,582 controller reads.  28 to 38 minutes, beside the other
  variants' replays; Linux takes 18 to 25.
- Before the locals had a stack of port-arena's own ("The movable
  build"), what differed from Linux was where the fibers' frames are: the
  addresses of the C's locals (in RDRAM wherever the game stores one)
  and what is left on the stacks between calls.  Between the log's reads
  4,000 and 5,000 some frames began to be drawn differently, and the
  timing parted by a retrace near read 13,413.  With it, RDRAM was the
  same up to the log's read 112,784, where a level's countdown was out
  from its start in wasm: `func_8028604C`'s capped level time, a `u16`
  result its caller declares `s16`, which x86 took as the callee left it and
  wasm extended again.  (The TAS still matched every read: the replay
  gives the log's retraces and reads, so a game drifting in the last
  levels didn't show there; the instruction counts parted a few frames
  earlier, where the countdown's branch first went the other way.)
  The declaration is fixed now (the ROMs don't change), and port-arena
  gives any such result the callee's extension, as the N64 had it.
- The locals' stack costs the page nothing: `web_perf.mjs` (headless
  Chromium, SwiftShader, into Simian Acres with the sound running) against
  main's build just before it, the game's mean per retrace in the level
  was 0.49-0.65 ms against 0.58-0.70 at 1x and 1.5-2.2 against 2.1-2.7 at
  4x CPU throttling, the work per retrace at 1x median 1.8 ms against 1.9.
  (At 4x `PORT_ADAPT` gave up the in-between pictures in one build and not
  in the other, on when its trial fell in the level's loading.)
- In headless chromium (Playwright's `playwright-core` with the system
  chromium, WebGL 2 through SwiftShader): the page loads, takes the ROM
  from the file picker or `?rom=`, plays the logos, the title, the name
  entry and the world map into Simian Acres with the keyboard (driven by
  the test), the pause menu, at 60 retraces a second (`-v`'s frame
  counts against the page's clock, SwiftShader's GL included),
  widescreen and `--interpolate` too; after a reload the ROM and the
  save are there.

- Headless Chromium on the GPU and headless Firefox 153 (Playwright's):
  the level at 60 new pictures a second, the sound playing on
  ("Performance", with the numbers).

Untested: a real browser with a person (the sound heard, a gamepad, the
feel of the pacing, Safari), and the jp and us.v11 builds (the same
code; only us.v10 was built).

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
  charged, not spent: the engine charges the MIPS instructions its
  original executes (`ENGINE_BLK` per basic block, or a function's average,
  `ENGINE_COST`: "Replacing the engine", "The engine made readable"), the
  game's C counts its optimised IR instructions
  (BEPass's second pass, ICount, into `__port_icount_c`), and libultra's
  own work that the port doesn't run is charged at rough fixed costs
  (`COST_*` in `port.h`, and the copies in `port/src/libc.c`).  At each
  call into libultra the running thread is charged for what it did since
  (`host_cpu_sync`, `port/host/threads.c`), at mupen64plus's CountPerOp = 2
  (42.7 ns an instruction; `PORT_COUNT_PER_OP` changes it) and 1.6 MIPS
  instructions per IR instruction for the C (`PORT_C_SCALE`; IDO's `-O1`
  code is bigger than clang's `-O2`).  A thread that is ahead of the clock
  goes "busy": it holds the CPU against lower priorities, higher ones
  preempt it (and push its end back), and `--deterministic` jumps its
  virtual clock through it.  That is the CPU model, and since 2026-10-02
  it is **off by default** (`--cpu-model n64` or `PORT_CPU_MODEL=n64` turn
  it on): the game's work takes no time, so no frame is held for it and
  the game never lags ("Lag frames").  What follows measures the model.
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

| mode (D_80364A90)            | port (the CPU model on): retraces a frame | mupen64plus |
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
  most 0.2 s queued), in a page to an `AudioWorklet` ("WebAssembly"), and
  to `--wav`.
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
  drops a buffer if the host fell far behind.  (The page's worklet steers
  against that drift instead.)

### libaudio

The port's libaudio is its own (`port/libaudio`), written from the library's
public description (the programming manual's audio chapters and function
reference), what the game uses of it, and the oracle's results (below):
the synthesizer and its voices (`synth.c`), a voice's decoding, resampling
and envelope (`voice.c`), the reverb (`reverb.c`), compact sequences
(`cseq.c`) and their player (`cseqplayer.c`), the heap, event queues and
bank files (`core.c`).  Its public header is `port/include/sdk/PR/libaudio.h`.
`-DPORT_LIBAUDIO_ORIGINAL=ON` builds the port with the decompiled original
(`blastcorps/src/libultra/audio`) instead; the oracle's recordings are made
with it.

It is exact: every audio frame's command list, every answer the game gets
(`alCSPGetState`, `alCSeqGetLoc`, voice allocation, the events it reads
back) and every byte of the memory the game shares with it are the
original's.  What that takes:
- the heap blocks in the original's order and sizes (the RSP's state
  blocks' addresses are in the command list, and the game's own
  allocations come after the library's), and the public structures' layouts
  (the game copies `ALChanState`s as words and reads `ALCSPlayer.state`);
- the same float and double steps where the original rounds (the
  envelope's ramp, the resampler's pitch quantization, the tick and sample
  conversions, the reverb's chorus);
- the original's corner cases, kept: a stolen voice's updates wait 512
  samples, the percussion setup writes the channel state just past the
  last, the reverb's "same tap" test compares with the previous section's
  output tap ahead of the write position, a tempo change re-queues the note
  offs in the order the original's relinking leaves them.

Like the original it is built without loop polls (`BEPASS_NOPOLL`, "Memory
model"), and ICount charges its own instructions: the TAS's "retraces given
anyway" move, nothing the suite checks does.

### The libaudio oracle

`port/tools/audio_oracle`: a recorder in the port and a replay outside it.

- **Recording.**  `-DPORT_AUDIO_RECORD=ON` (a native-endian ILP32 build:
  `-DPORT_64BIT=ON -DPORT_NATIVE_ENDIAN=ON`, with
  `-DPORT_LIBAUDIO_ORIGINAL=ON`) wraps every libaudio function the game
  calls (`port/src/audio_record.c`), and `PORT_AUDIO_LOG=FILE` writes the
  log (`port/host/audiolog.c`, format in `port/include/audio_log.h`):
  each call's arguments and the structures handed over and back, its
  result, each frame's command list (by hash), the library's calls back
  into the game (the sound player's voice handler, the DMA routine) with
  what they returned, and everything the game, the RSP and the PI changed
  in the memory they share (the audio heap and the audio objects' data)
  since the library last had it.  Interrupts are masked while the library
  runs, so that no thread runs in the middle of a call.
- **Replaying.**  `make -C port/tools/audio_oracle VERSION=...` builds
  `replay` and the two libraries for i386 (the N64's layouts) with the
  game's memory at its N64 addresses; `replay LIB.so LOG` plays the game's
  side from the log and checks everything the library gives back against
  the recording, stopping at the first difference (`-k` goes on).
  `--hashes FILE` writes a hash of the shared memory after each call (the
  library's own heap blocks left out) for comparing two libraries;
  `--dump-acmd N FILE` and `--dump-mem N FILE` write a frame's command list
  or the memory after a call, which `acmd_diff.py` and `mem_diff.py`
  compare; `alog_dump.py` prints a log.

Checked on 2026-10-01 (us.v10): the quick tier's four scenarios and the
attract mode's 12,000 frames (8,940 audio frames, 359,000 calls) and the
whole TAS (3,578,841 calls, 137,421 audio frames), the port's libaudio identical to the original in
every frame's commands, every answer and every call's shared memory.
What those runs never reach (line coverage of `port/libaudio` over them:
86%) is what the game's data and calls don't use: 16-bit wave tables, unity
pitch, loops with a count, oscillators, the sustain pedal and aftertouch,
and the API the game doesn't call.

## Graphics

The RSP's graphics tasks are Fast3D (gbi 2.0D) display lists.
`port/host/gfx.c` is the front end both renderers share and the software
renderer; `port/host/gfx_gl.c` is the OpenGL 3.3 one; `gfx.h` is the
interface between them.

- **Front end** (RSP and RDP state): matrices, vertices, lighting,
  texgen, fog (into shade alpha, as the RSP does it; the game doesn't use
  it), clipping against the near plane, culling, `gSPModifyVertex` (the
  game rewrites texture coordinates with it: `G_MW_POINTS`), `G_LINE3D`
  (the world map's routes between the levels, from hd_front_end's own
  line microcode: a quad 1.5 + wd / 2 pixels wide along the projected
  segment, through the triangles' path; it used to be skipped, as some
  emulators' plugins skip it), TMEM loads
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
- **The terrain's visibility test** (`func_8024B8F4`, and
  `func_802A467C` in hd_code 5FD50, while a level is set up: in
  `PORT_AUTOSTART=1`, 87 tasks between retraces 558 and 1,201) sends
  the RSP one box of 12 triangles per terrain cell, split in four while
  it's visible.  It runs a second Fast3D 2.0D (`D_802E77B0`) that writes
  the RDP's commands to an output buffer (`D_803BEB80`) instead of
  handing them to the RDP, and its SchedTask asks for the RSP only
  (flags 1, the frames' are 3).  The CPU then reads the buffer's first
  word: 0xE8000000 (the list's closing tile sync) means every triangle
  was rejected.  So `ultra.c` tells the front end that task's commands
  go to memory, and nothing of it is drawn.  The commands aren't
  written either.  The movie's emulator (mupen64plus's rsp-hle, which
  hands every graphics task to its graphics plugin) doesn't write them,
  so the game there finds the buffer as it was (zeros) and takes every
  cell as visible, and so does the port.  (The RSP would reject 26 of
  the 87 boxes; writing its answer changes the attract mode's save and
  `PORT_AUTOSTART=1`'s sound, though no screenshot: the game's timing
  moves.)
  The box is on the C's stack, which the RSP addresses as physical
  `0x10xxxxxx` on the fibers' host stacks (at `0x90000000`): the front
  end and the audio HLE keep bit 28 of a segmented address and of a
  segment's base, which the RSP's 24 bits would drop (they read, and the
  front end drew, whatever RDRAM held at `0x800EFCC0`: a dark sliver on
  the title's road, frames 848 to 857).  The movable builds' stacks are
  in the arena (`0x80C00000` and up), which fits.

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

`--hd-text` (OpenGL) draws the game's text from a font at the internal
resolution: the stencil font's glyph textures are recognized by the
font.c slot their LoadBlock came from and replaced, in the texture cache,
by glyphs drawn from Stardos Stencil (SIL OFL) fitted to the game's; the
quads, colours and shadows stay the game's.  Each glyph is one GL texture
(R8, shared by every cache entry that holds it), and the font's distance
fields are made one a retrace from the first (`hdtext_idle`), so neither
the start nor a screen of new text waits on them.  docs/FONTS.md has the
inventory of the game's fonts and the details, with the timings.

Coverage from alpha (`CVG_X_ALPHA`) multiplies the pixel's coverage by
the combined alpha, and with `AA_EN` the RDP doesn't write a pixel left
with none: alpha below 32, for a fully covered pixel, whatever the blender
says (`gfx_cvg_alpha_min`).  The game's 2D sprites rely on it once they
have faded in: they fade in translucent (`0x00504240`, the blend with
memory hides their transparent texels) and then switch to
`G_RM_OPA_SURF` with `FORCE_BL`, `AA_EN`, `CVG_X_ALPHA` and
`ALPHA_CVG_SEL` (`0x0F0A7008`), where only the coverage drops those
texels.  Without the rule the results screen's money and waving-man
icons, the medals, the bottom bar's medal and star icons and the
highlighted EXIT were drawn on black rectangles, in both renderers (the
GL program key keeps `AA_EN` where it matters).  It also drops the
starfield's faintest texels, which were drawn black over brighter stars.

Above 1x the OpenGL renderer keeps a texture rectangle's s and t within
the range its 1x pixels sample (`v_box`): the pixels on the edges would
otherwise filter the first or last texel with the one beyond it, which the
RDP never reads there.  The results screen's medal is two 64x32
rectangles whose tile wraps at 64 rows, so its halves' last rows were
filtered with their first, a dark seam across it; the title's tiled logos
had fainter seams at every tile edge.

Reflection mapping (`G_TEXTURE_GEN`, the Rare logo, the chrome of the
vehicles, the promotion badge) takes the vertex normal's projections on
the look-at vectors (`gSPLookAt`: the camera's x and y axes, which the
game sets with `guLookAtReflect`), brought into model space by the
modelview and normalized as the lights are, and maps their -1..1 to 0..1
of the texture scale in 1/64 texels: the game's maps are 32x32 sphere
maps with `gSPTexture(0x07C0, 0x07C0)`, texels 0 to 31.  The port used to
take the modelview's x and y axes (the world's, since the game keeps its
view in the projection) and twice the range, so the coordinates wrapped
around the map and reached its dark corners outside the sphere: the back
of the Rare logo went black with gold streaks (issue #3), the J-Bomb's
silver boosters dark blue (#2), the promotion badge grey.  In
mupen64plus (rice) the back of the logo is plain gold and the boosters
silver, as they are now.  (`G_TEXTURE_GEN_LINEAR` isn't used and is
mapped the same way.)

A triangle with a vertex behind the eye is culled once it is clipped,
by the winding of what is left of it on the screen, as the RSP culls
the polygons its clipper hands on.  The port used to draw every such
triangle, so the back faces of walls and girders that reach behind the
camera were drawn as sheets over the screen, with a straight edge where
the triangle's side went behind the eye: the road's wall over the
carrier at the end of a level (frame 108,600 of the TAS), dark slivers
over the roads, and a red girder over half the screen while the carrier
blew up among them (issue #1; the explosion's own shockwave isn't culled
and was drawn right).  Over the TAS, 11 of the 918 frames a sweep looks
at change.

Not done: anti-aliasing (the coverage the blender uses on edges; rendering
above 1x and scaling down is the substitute) and the VI's filters, dither,
the combiner's chroma key (its noise input is a hash, not the RDP's
generator), the far plane (depth is clamped instead; without
`EXT_depth_clamp`, in Firefox, the GPU clips at the far plane, which over
the whole TAS changes a few hundred pixels in a handful of frames:
`PORT_GL_DEPTH_CLAMP=0` draws that way anywhere, and the log says `gl: no
depth clamp` where it happens), and triangle edges follow GL's and the
software renderer's pixel-center rule rather than the RDP's.

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
  most, and fewer when the CPU takes longer: on the N64, and on the port
  with its CPU model on (the world map then averages 3.45 retraces a
  frame, see "Timing"), which it no longer is by default ("Lag frames").
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

### Lag frames

On the N64 a level frame takes two retraces when the CPU is done in time
and three or more when it isn't: a lag frame.  The game slows down (its
step per frame is fixed) while the level clock, which counts retraces,
runs on.  The port used to reproduce that with its CPU model ("Timing"):
the game's work was charged at the N64's speed and frames were held for
it.  Since 2026-10-02 the model is **off by default**, natively and in the
page: the game's work takes no time, every level frame takes the minimum
two retraces (30 a second), and the front end goes as fast as its own
waits let it.  The bar is the owner's: the port may differ wherever a
player, even a perfect one, couldn't tell, and beyond that smoother play is
preferred over the N64's in-game times to the tenth.

- **The option.**  `--cpu-model n64` (or `PORT_CPU_MODEL=n64`, which the
  page takes as `?env=PORT_CPU_MODEL=n64`) turns the model back on, for
  the N64's lag and timing; `PORT_COUNT_PER_OP=N` still sets its rate (2,
  mupen64plus's, is what `n64` means; 0 is off).  `--cpu-model off` is the
  default.
- **Nothing else depends on it.**  The quick tier always ran without it
  (its references are unchanged), and the TAS replays the same with it on
  or off.  `D_803649D8 = osGetTime()`, the per-frame "random" number of the
  screen shake, the dust and the debris, is from the virtual clock in
  `--deterministic` and the page: without the model it is still spread
  (`PORT_AUTOSTART=1`'s Simian Acres, 1,018 frames: all 40 values of
  `% 40`, the four of `>> 8 & 3` within 250-265 each, dust on 27 frames;
  with the model 887 frames, dust on 27), so it was left as it is.

**The TAS without lag.**  `--replay` gives every frame the movie's
retraces (its lag) unless `PORT_REPLAY_TIMING=free`: then each frame takes
what the port gives it, two retraces in a level, and the level timer runs
at the port's pace (`test.py tas` plays it so by default, `--timing movie`
the old way, `both` both).  The game's reads of the counts in the middle
of a frame and the audio thread's answers stay the movie's.  They decide
when a message window closes (`func_8026BCE0`, `(count - start) / 60.0f >
unkC`), and the game holds the player while one is up, so a window that
closes a frame early or late moves all the movie's input after it.  Which
frame that is depends on where in the N64's frame the retraces fell, so
even with the movie's lag the port's own counts put Simian Acres' first
window out by a frame (the player elsewhere from frame 1,101,
`PORT_REPLAY_FREE=counts`); no timing but the movie's emulator's gives
them.  (A player who isn't replaying a movie can't tell: the window stays
up as long, in seconds.)

Measured on us.v10, the 32-bit build, the whole TAS:

| | the movie's timing (`--cpu-model n64`) | the movie's timing (no model) | free timing (no model) |
|---|---|---|---|
| the log's reads matched | 125,297 of 125,297, none skipped | the same | the same |
| modes forced | 0 | 0 | 0 |
| retraces given anyway | 58 | 0 | 0 |
| medals | 57 platinum, 3 done | the same | the same |
| save | `0db40f5e7068d3e3` (the reference) | the same | the same (its times are what the game read: the movie's) |
| gameplay digest | `7ec6ef73a5265afa` (the reference) | | the same: 106,922 level frames, every field |
| retraces in all | 275,459 (76:30.9) | | 249,023 (69:10.4): -26,436, -7:20.5, -9.6% |
| retraces in the levels (mode 4) | 182,526 for 86,798 frames: 9,096 over 2 a frame | | 173,476: 2 a frame but for 46 |
| one replay | 21 minutes | 19.4 | 19.4 |

The free-timing replay through `test.py tas` passes the same way on the
32-bit build (22 minutes) and on `wasm` under node (30 minutes); the
quick tier passes unchanged on all eight us.v10 variants, and on us.v11's
and jp's 32-bit and `mn32` builds.

The levels' time by the level clock, the movie's (the N64's, by the
gated replay) against no lag (`port/tools/tas_times.py REF.digest
OTHER.digest`, from the two runs' digests; a level's stretches of mode 4
added up).  Levels with no lag in the movie don't change; the busiest
lose up to 12%.  The medal times and the save's best times are this
clock, so without lag they come out shorter: a medal is easier to make,
never harder, and a time is not the N64's for the same play (in the
replay the game reads the movie's counts, so its save is the movie's):

| level | stretches | frames | lag retraces (the movie's) | the movie's | no lag | difference |
|---|---|---|---|---|---|---|
| Simian Acres | 2 | 1525 | 385 | 0:57.1 | 0:50.8 | -6.3 s (-11%) |
| Sideswipe | 1 | 261 | 1 | 0:08.6 | 0:08.6 | +0.0 s (-0%) |
| J-Bomb | 1 | 369 | 26 | 0:12.7 | 0:12.2 | -0.5 s (-3%) |
| Backlash | 1 | 168 | 1 | 0:05.5 | 0:05.5 | +0.0 s (+0%) |
| Argent Towers | 3 | 3486 | 514 | 2:04.6 | 1:56.1 | -8.5 s (-7%) |
| Orion Plaza | 1 | 386 | 36 | 0:13.4 | 0:12.8 | -0.6 s (-4%) |
| Blackridge Works | 2 | 1295 | 104 | 0:44.8 | 0:43.1 | -1.7 s (-4%) |
| Carrick Point | 2 | 2805 | 246 | 1:37.5 | 1:33.4 | -4.1 s (-4%) |
| Kipling Plant | 1 | 212 | 7 | 0:07.1 | 0:07.0 | -0.1 s (-1%) |
| Sleek Streets | 1 | 974 | 1 | 0:32.4 | 0:32.4 | +0.0 s (+0%) |
| Havoc District | 2 | 4268 | 657 | 2:33.1 | 2:22.2 | -10.9 s (-7%) |
| Twilight Foundry | 1 | 693 | 0 | 0:23.0 | 0:23.0 | +0.0 s (+0%) |
| Skyfall | 1 | 88 | 1 | 0:02.9 | 0:02.9 | +0.0 s (+0%) |
| Shuttle Gully | 2 | 1957 | 290 | 1:10.0 | 1:05.1 | -4.9 s (-7%) |
| Cobalt Quarry | 1 | 838 | 1 | 0:27.9 | 0:27.9 | +0.0 s (+0%) |
| Tempest City | 3 | 1339 | 251 | 0:48.7 | 0:44.5 | -4.2 s (-8%) |
| Beeton Tracks | 2 | 3276 | 123 | 1:51.1 | 1:49.1 | -2.0 s (-2%) |
| Silver Junction | 1 | 700 | 1 | 0:23.3 | 0:23.3 | +0.0 s (+0%) |
| Echo Marches | 2 | 4629 | 973 | 2:50.4 | 2:34.2 | -16.2 s (-10%) |
| Mica Park | 1 | 307 | 20 | 0:10.5 | 0:10.2 | -0.3 s (-3%) |
| Salvage Wharf | 1 | 528 | 1 | 0:17.5 | 0:17.5 | +0.0 s (+0%) |
| Thunderfist | 1 | 241 | 12 | 0:08.2 | 0:08.0 | -0.2 s (-2%) |
| Cromlech Court | 2 | 1098 | 115 | 0:38.4 | 0:36.5 | -1.9 s (-5%) |
| Ironstone Mine | 3 | 4414 | 377 | 2:33.3 | 2:27.0 | -6.3 s (-4%) |
| Ebony Coast | 3 | 3845 | 1039 | 2:25.3 | 2:08.1 | -17.2 s (-12%) |
| Morgan Hall | 1 | 456 | 46 | 0:15.9 | 0:15.1 | -0.8 s (-5%) |
| Outland Farm | 2 | 3118 | 141 | 1:46.2 | 1:43.9 | -2.3 s (-2%) |
| Geode Square | 1 | 393 | 0 | 0:13.0 | 0:13.0 | +0.0 s (+0%) |
| Lizard Island | 1 | 1716 | 0 | 0:57.1 | 0:57.1 | +0.0 s (+0%) |
| Saline Watch | 1 | 732 | 151 | 0:26.8 | 0:24.3 | -2.5 s (-9%) |
| Dagger Pass | 1 | 934 | 0 | 0:31.1 | 0:31.1 | +0.0 s (+0%) |
| Magma Peak | 2 | 1136 | 1 | 0:37.8 | 0:37.8 | +0.0 s (-0%) |
| Baboon Catacomb | 1 | 1080 | 1 | 0:35.9 | 0:35.9 | +0.0 s (+0%) |
| Crystal Rift | 2 | 4405 | 871 | 2:41.2 | 2:26.7 | -14.5 s (-9%) |
| Falchion Field | 1 | 513 | 22 | 0:17.4 | 0:17.0 | -0.4 s (-2%) |
| Dark Heartland | 1 | 319 | 4 | 0:10.6 | 0:10.6 | +0.0 s (-1%) |
| Corvine Bluff | 2 | 466 | 2 | 0:15.5 | 0:15.5 | +0.0 s (+0%) |
| Bison Ridge | 1 | 734 | 1 | 0:24.4 | 0:24.4 | +0.0 s (+0%) |
| Cooter Creek | 2 | 812 | 2 | 0:27.0 | 0:27.0 | +0.0 s (+0%) |
| Skerries | 1 | 1048 | 0 | 0:34.9 | 0:34.9 | +0.0 s (+0%) |
| Obsidian Mile | 1 | 1543 | 197 | 0:54.6 | 0:51.4 | -3.2 s (-6%) |
| Marine Quarter | 1 | 612 | 1 | 0:20.3 | 0:20.3 | +0.0 s (+0%) |
| Diamond Sands | 1 | 2462 | 228 | 1:25.8 | 1:22.0 | -3.8 s (-4%) |
| Oyster Harbor | 3 | 6042 | 571 | 3:30.8 | 3:21.3 | -9.5 s (-5%) |
| Gibbons Gate | 1 | 1894 | 1 | 1:03.1 | 1:03.1 | +0.0 s (+0%) |
| Jade Plateau | 1 | 483 | 0 | 0:16.0 | 0:16.0 | +0.0 s (+0%) |
| Moraine Chase | 1 | 1402 | 1 | 0:46.7 | 0:46.7 | +0.0 s (+0%) |
| Angel City | 2 | 2521 | 309 | 1:29.1 | 1:23.9 | -5.2 s (-6%) |
| Ember Hamlet | 1 | 1026 | 175 | 0:37.0 | 0:34.1 | -2.9 s (-8%) |
| Glanders Ranch | 1 | 1551 | 0 | 0:51.6 | 0:51.6 | +0.0 s (+0%) |
| Glory Crossing | 3 | 2362 | 653 | 1:29.5 | 1:18.6 | -10.9 s (-12%) |
| Shuttle clear | 1 | 2879 | 492 | 1:44.1 | 1:35.9 | -8.2 s (-8%) |
| Moon | 1 | 1157 | 21 | 0:38.8 | 0:38.5 | -0.3 s (-1%) |
| Mercury | 1 | 960 | 1 | 0:31.9 | 0:31.9 | +0.0 s (+0%) |
| Venus | 1 | 830 | 20 | 0:27.9 | 0:27.6 | -0.3 s (-1%) |
| Mars | 1 | 490 | 1 | 0:16.3 | 0:16.3 | +0.0 s (+0%) |
| Neptune | 1 | 1020 | 1 | 0:33.9 | 0:33.9 | +0.0 s (+0%) |
| all | 83 | 86798 | 9096 | 50:42.1 | 48:11.2 | -150.9 s (-5%) |

What else a player sees, measured `--deterministic` from no save (us.v10,
the 32-bit build; `PORT_PACE`, retraces a frame by mode):

| | CPU model on | off (the default) |
|---|---|---|
| the N64 and Rare logos | 1.01, 1.30 | 1.00 |
| the title comes up at retrace | 638 | 548 (1.5 s sooner; 518 since "The front end's waits") |
| the title and the attract story (mode 2, 7 stretches of 321 to 1,164 frames) | 2.09-2.64 | 2.00 |
| "leaders of" screens | 1.11-1.14 | 1.00 |
| a level's load (mode 0x800, `PORT_AUTOSTART=1`'s Simian Acres) | 95 retraces, 8 frames | 36, 17 frames (1 s sooner) |
| the level's first 29 frames | 2.02 | 2.07 |
| driving (Simian Acres) | 2.00 | 2.00 |
| the CMO intro, the map, the results | 1.96-2.04 | 2.00-2.04 |

(Retraces a game frame: a controller poll each.  This table first had
3.63 and 4.60 for the title and the story and 4.10 for the level's start,
which were retraces over the game's frame count across the points where
the count starts again in the same mode, not slow frames.  The level's
load isn't a fixed number of frames: with the model it has fewer, longer
ones.)  So the loads
and the front end's busier screens are quicker (what was left above the
minimum, the loads' waits, is the next section's), and in the TAS the
time outside the levels drops by 4:50 (92,933 retraces to 75,547).  Sound
doesn't change: the audio thread runs every second retrace either way.
A frame still takes more than two retraces where the host can't keep up
in real time (natively without `--deterministic`, and in the page), which
is the host's own lag, not the N64's.

### The front end's waits

With the CPU model off, every screen already ran at its minimum (the
table above): what was left were stalls between screens, where the game
draws nothing and waits.  Found by sampling, at every retrace, what each
thread was blocked on and where the polls' time went (us.v10, the 32-bit
build, `--deterministic`, no save):

| wait | what it is | retraces | now |
|---|---|---|---|
| the boot | `osContInit`'s half second for the controllers after power-on (libultra; `port/src/ultra.c` waited it on a timer) | 30, once | gone |
| Start on the title, before the name entry | the pak thread (hd_front_end `E7B0.c`, `func_801F58E8`) waits for the scheduler's next retrace message after every command, done or not; the front end sends it one command at a time (15 reads of the save's slots, a probe, a check) and waits for each reply | 1 a command: 29 there | the wait after a command that is done is left out; a retry still waits (the "insert a pak" loops) |
| the name entered, the save written | `osEepromLongWrite`'s 12 ms after each 8-byte block, the EEPROM's write cycle (32 blocks a save) | 23 a save | gone |
| every load (the front end, the title, the map, each level) | the decompressors' loops (gzip's inflate, the LZSS): without the CPU model the polls on loop back edges still moved virtual time on (2 us for each 64), so a load took about two retraces in `--deterministic` and the page | about 2 a load | no time while `func_8025C230` or `func_8028B4C4` runs (`host_loading`, `port/src/loads.c`) |

Kept, as the game's own (a player sees them as its pacing):

- the 15 retraces before the N64 logo (`func_80244930`, mode 0x10: a
  spin on the scheduler's count after starting the logo);
- the logos' 250 frames each, the "leaders of" screens' 740;
- a mode switch's one or two frames with the picture blacked out
  (`osViBlack`), which show as 3 or 4 retraces between two polls;
- the level's load screen (mode 0x800, 17 frames, 36 retraces).

And the cartridge: a PI DMA completes at mupen64plus's rate (a count tick
per 8 bytes, 1 MB in 2.8 ms), well inside a frame everywhere, so it was
left as it is.

The title now comes up at retrace 518 (548 before, 638 with the model);
`PORT_AUTOSTART=1` from no save reaches Simian Acres' first frame at
retrace 1,205 instead of 1,324 (2 seconds sooner: 30 at the boot, 27 at
the name entry's reads, 22 at its save, the rest the loads).  In the TAS
(free timing) the time outside the levels drops from 75,280 retraces to
73,799 (25 s): there the pak thread's commands go at the movie's frames
(`port_replay_save_started`) and the boot's half second is the same, so
most of it is the loads.  The replay passes as before: every read
matched, 57 platinum, the reference's save and gameplay digest.

`--load-waits n64` (or `PORT_LOAD_WAITS=n64`) keeps all four, and
`--cpu-model n64` implies it: with it the pacing log is the one before,
byte for byte.  The quick tier's references were re-recorded (us.v10):
their scripted input (`PORT_AUTOSTART`) is keyed to retraces, so it
lands on other frames once the boot is 30 retraces shorter.  With the
in-level taps' phase moved by the same retraces (119), auto1's and
auto3's digests are the old runs' frame for frame; attract's and auto2's
are the same gameplay without that (`digest_cmp.py` against
`--load-waits n64` runs).  The same for us.v11 and jp (their 32-bit
builds): with `--load-waits n64` every hash of every scenario is the old
references' (the layout-dependent static aside), the digests compare as
us.v10's do (`attract.long` too: 4,611 and 4,805 level frames, the same),
and their references were recorded again, with gameplay digests for the
first time.  `PORT_AUTOSTART` taps nothing in the first
30 retraces now: a button held at the game's first read asks to erase
the save (mode 0x40000000000000), and that read now comes at once.

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
(`port/host/gfx.c`, "in-between frames"; `gfx_gl.c`, the twins), with
either renderer:

- **More passes.**  Where the game holds its frames (the mode is outside
  the swap's mask, `host_frame_held`), each graphics task's display list
  runs once more for each in-between image, back to back, while RDRAM is
  as the task found it.  The first pass is the real one.  In-between pass
  k draws into *twin* k of each framebuffer, which has its own depth
  buffer: with OpenGL a twin of the GPU target, with the software
  renderer a host-side buffer (as wide as the widescreen frame) with a
  z-buffer of its own, so that RDRAM stays the first pass's.  The RSP/RDP
  state (and TMEM, and the combiner's noise) before each in-between pass
  is what the first started from, and after them, what the first ended
  with.  The in-between passes draw nothing but the twins (the 8-bit
  render-to-texture images and the RDRAM z-image stay the first pass's),
  skip the read-backs, and charge no RDP time.  With OpenGL an in-between
  pass doesn't run the display list again: it replays what the first
  pass recorded (Performance, "The third round"), to the same pictures.
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
  the frame has it.  Twin k of K is at t = (k + 1) / (K + 1) between the
  two frames.
- **Blending rectangles.**  Texture and fill rectangles (text, the HUD's
  pieces, menus, the title's logo) are paired the same way, by what they
  draw instead of an address: a texture rectangle by its texture image,
  tile, texture coordinates, steps and size, a fill by its colors and
  modes (a fill into the z image, a clear, is left alone), and by how many
  of the same came before it in the frame.  The in-between passes move the
  four corners, unless one moved by more than 64 pixels.  Their texture
  coordinates stay the frame's: a rectangle whose texture coordinates
  changed (a counter's digit) is another rectangle and isn't paired.
- **How many.**  A frame's twins are chosen as its first task runs, from
  how long it will be on screen, D retraces, taken as the shorter of the
  last two frames' holds (2 to 4): D - 1 twins at 60 Hz, D * N / 60 - 1
  with `--display-hz N` (at most 7; 3 with the software renderer, which
  draws the frame again for each, `PORT_INTERP_MAX` to change).  A frame
  held three retraces is then shown at 1/3, 2/3 and itself, one held two
  at 1/2 and itself.  The shorter hold, because a frame held longer than
  its twins were made for shows itself for the rest (the motion pauses a
  retrace), while one held shorter never gets to itself (the motion jumps
  by a third).
- **Showing it.**  When the VI moves to another buffer
  (`host_gfx_frame_shown`), the frame in it is complete: the next frame's
  tasks wait for the RDP's thaw, which comes after.  Its twins are then
  marked ready if every task of the frame had its passes.  The hook
  is on the host side on purpose.  A call in `osViSwapBuffer` would be
  counted as the game's CPU time (BEPass's ICount), and that alone moved
  the TAS replay's timing a little (76 retraces given anyway instead of 80,
  with the option off).  The retraces that show the frame show its twins
  in turn and then the frame (`gfx_interp_image`, for both renderers).
  The frame is therefore on screen one retrace later than before, and
  in-between images take the retraces a frame used to repeat on.
- **Faster displays** (`--display-hz N`, or `auto` for the display's
  rate).  With N above 60 the loop presents between the retraces too, on
  a display clock of its own (host time, a tick every 1/N second; a tick
  within half a period of a retrace is that retrace's present), and each
  present shows the twin for its phase in the frame
  (`gfx_interp_image_at`).  Not with `--deterministic`, whose clock isn't
  the host's.  The swaps aren't synchronized to the display (the swap
  interval stays 0, as the loop paces itself), so a present can tear.
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

The software renderer gives the same: its in-between images are its
twins, and every second image of steady driving is exactly its run
without the option's.  Its cost is the frame drawn again: the quick
suite's `PORT_AUTOSTART=3` run (3,000 retraces, `--widescreen` too) takes
40 s against 18 s.

**The TAS** (us.v10, 32-bit build, `--headless --replay`) with
`--renderer gl --scale 1 --interpolate` gives the same replay report
(all 125,297 of the log's reads matched, none skipped) and a byte-identical
save as `--renderer gl` without it: 57 platinum.  With the rectangles and
the holds' twins, and `--widescreen` too: all 125,297 matched, none
skipped, no mode forced, the reference save; over its 275,482 retraces it
showed 125,338 new frames of the game's and 126,742 in-between images,
blended 91% of the vertex loads and moved 6,938 of 1,042,139 rectangles
(5 moved too far).  114,536 frames had one twin, 5,418 two and 462
three; 6,525 frames were held longer than their twins were made for and
1,084 shorter.  The software renderer's replay with `--interpolate
--widescreen` is the same: every read matched, the reference save, the
same counts.  With the option off, the default software renderer's
replay is exactly what it was before the option existed: a
byte-identical log and save.

What it doesn't do:

- **Texture animations and scrolls** move at the game's rate: the TVs'
  pictures, texture coordinates rewritten each frame (with `G_MW_POINTS`
  or in the vertices) and tiles moved over their texture.  A change of
  texture coordinates can't be told apart from a flip to another cell of
  a texture (or a scroll wrapping round), and blending one would show
  texels from between two cells.  So do the soft shadows'
  render-to-texture images, whose images are the frame's (their geometry
  is blended).
- **A guessed hold.**  The twins are made before the frame's hold is
  known (it ends when the next frame is drawn).  A frame held longer
  than the guess pauses a retrace, one held shorter jumps; the front
  end's one-retrace modes need none and get none.
- **Mispairings.**  When objects that share vertex data appear or
  disappear, the count of earlier loads from an address shifts, and
  instances can pair with each other.  If they are near each other, the
  distance test lets them through, and for one image an instance sits
  between two places.  Rectangles can mispair the same way (the same
  glyph twice in a line of text that changes).

## Widescreen

With a window the picture takes the window's shape, and follows it as
the window is resized or goes full screen (`--aspect window`, the
default there); `--aspect W:H` (`16:9`, `21:9`, up to 32:9) or
`--widescreen` (16:9) fix it instead.  Wider than the N64's 4:3, it shows
more of the 3D world, with either renderer.  Headless runs and
`--deterministic` and `--replay` ones (the suite's and the TAS's) are 4:3
unless asked, and at 4:3 the output is what it was before, pixel for
pixel.

A window narrower than 4:3 (a portrait one, a phone's page held
upright) gets the 4:3 picture, as wide as the window, with black bars
above and below: the internal resolution is then the picture's lines,
not the window's (`want_scale`).  Showing more of the world above and
below instead would be the same trick on the other axis (taller
framebuffers, full-height scissors and fills widened, the 2D picked out
the same way), but every row of the game's 240 is spoken for by its
320x240 framebuffer addresses (the software renderer and the
framebuffer reads), the levels' cameras look down at the player from a
fixed distance, so what a taller view would show above and below is
mostly ground the game doesn't expect to be seen, and the HUD's top and
bottom rows would then need the same treatment as its sides; it isn't
done.  Wider than 32:9 is pillarboxed.

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
  that spans the game's frame from edge to edge (to within a pixel) is
  widened to the edges of the wide one: the sky behind the levels, its
  gradient or its picture.  Its attributes (depth, shade, texture
  coordinates) continue across the sides as the polygon has them (a plane
  over the screen), so the sky's panorama goes on past the 4:3 edges
  instead of being stretched; a texture that doesn't wrap across (it would
  show what is beside it in TMEM) is stretched.  The sky of the levels'
  intros, whose camera looks at the horizon, is such a picture 0 to 319
  pixels wide (a hair short of 320): the test used to want 0 and 319
  exactly, so it wasn't widened, and the sides above the horizon were
  black or showed what was behind.  The rest of the
  2D (text, menus, the pause screen, the hint panels) stays in the
  middle, as does every texture rectangle, but for the HUD (below).  A
  texture rectangle at an edge (the tiles of a
  full-screen picture: the story, results and promotion screens) blacks
  out the side beyond it, in its rows, so those screens are pillarboxed
  instead of framed by whatever the sides held.  Not once the frame has
  drawn its sides itself (a wide fill or widened 2D since the color
  image was set): the world map's vehicles slide in and out at the
  bottom corners as texture rectangles at the edges, and the bands would
  be black boxes over its stars there.
- **The HUD** (`--hud edges`, the default) moves to the sides: the
  levels' radar and its arrow, the money, the counters and the timer,
  the TV in a corner and the bonus amounts keep their distance from the
  wide picture's edges instead of staying where the 4:3 edges were
  ("The HUD at the sides", below).  `--hud centre` leaves it in the
  middle.
- **`--interpolate`** composes with it: the twins are the wide size too,
  and the in-between pass goes through the same rules (the positions are
  blended in clip space, before the frame is widened).  Every second image
  is then exactly the widescreen run without `--interpolate`'s.
- **The software renderer** draws the 320-wide 16-bit color images (the
  framebuffers) wide on the host instead of into RDRAM, the z-buffer with
  them, and the window shows those.  A texture load from one would copy
  its middle back to RDRAM first (none happens), as the OpenGL renderer
  reads its targets back.

The game doesn't cull at the view's edges, on the port or in the
emulator the TAS was made on.  Over 40,000 frames of the TAS, the
perspective triangles it sends fall off smoothly with their distance from
the middle, out to ten times the 4:3 view's half-width, with no step at
its edge, so nothing pops in at the sides.  The level's list of active
cells (`D_803C30A8`), which the terrain and the objects drawn come from
(and which the game logic reads too, `func_80267614`, `func_80270A54`),
has every cell: Simian Acres's is all 18 of its 3x6.  It is built by a
quadtree walk (`func_802A470C` and on, `5FD50.s`, a node a frame) that
asks the RSP whether each node's bounding box is in view: `func_802A4B0C`
sends a graphics task with a microcode of its own (`D_802E77B0`) that
draws the box and writes the RDP commands to an output buffer
(`D_803BEB80`), whose first word stays `0xE8000000` when nothing of the
box was drawn.  Neither the port nor mupen64plus's HLE writes the output
buffer, so every node is in view.  (On the hardware it would cull at the
4:3 frustum.  Answering it for a wider one would change the list and so
the game.)  The one other writer, `func_80295C70`, puts an area's fixed
list there (`D_802FF150`, one area in one level).  So what shows at the
sides is the world itself, and where a level's map ends (or the sky
isn't drawn) the background: over the TAS at 32:9 (a screenshot every 600
retraces, the first 90,000) no black patch showed at the sides but the
intros' sky above.

**As defaults.**  In a run with a window that is neither
`--deterministic` nor `--replay`, `--aspect window` is the default
(`--aspect 4:3` turns it off): nothing the suite or the TAS checks
changes (they run headless and deterministic), and the game doesn't see
it.  The default window is still 640x480, so nothing changes until it is
made wider; then it shows what the window has room for, and its
internal resolution follows (the window's lines, as before, or
`--max-pixels`' cap).  The browser's page is such a run: its canvas
fills the page, so the picture takes the page's shape ("WebAssembly").
`--interpolate --display-hz auto` could be a default the same way (with a
`--no-interpolate`), but isn't: it costs a retrace (17 ms) more latency,
the rare mispaired instance or rectangle for one image, and with the
software renderer (headless and where OpenGL is missing) twice the
drawing.

### The HUD at the sides

In a wide picture the game's HUD would stay where the game puts it,
inside the 4:3 middle, further from the edges the wider the picture: at
32:9 the money sits a third of the way in from the right.  With `--hud
edges` (the default) the front end moves each HUD element by `off`
columns towards the side it is on, so that it keeps its distance from
the picture's edge, as the window is resized too (`off` is the current
frame's).  It is all the renderers' front end (`gfx.c`, "the HUD at the
sides"): the display lists, RDRAM and the RDP's time (charged from the
geometry before it is moved) are the game's, so the game's timeline and
the TAS are as they were.

What is HUD is told from what the game draws, not from where it is on
the screen (a position rule alone would split a centred element made of
several pieces, and move the markers the game places in the world):

- **The mode** (`D_80364A90`, read as the replay and the name entry
  read it): a level being played (4, and the bonus levels' 0x4000000),
  its intro with the traffic lights (0x2000), its end (LEVEL COMPLETE,
  0x8; MISSION COMPLETE, 0x200) and 0x1000000000.  The pause screen
  (0x100), the map, the world map (0x4000), the front end, the results,
  the promotion and the stories are never moved: their 2D is drawn with
  the same projection, but they stay centred.
- **The projection** a draw is made with (the address of the last
  `G_MTX` that loaded one, against segment 2, which points at the frame's
  buffer, `frame.h`): the gameplay frame's 2D projection (its `mtx[3]`,
  + 0xC0), and its `mtx[2]` (+ 0x80), a perspective one that only the
  radar's 3D arrow is drawn with (the world is `mtx[5]`).  The hint
  panels and the levels' goals ("DESTROY BUILDINGS IN ...") come from
  the panel code (26570.c), through `mtx[73]`: they stay centred.
- **The vertices' address** says what the element is.  The frame's own
  (in segment 2's buffer) are the green markers around targets, placed
  where the target is in the world: they stay put, and in a wide picture
  they mark targets out at the sides where the game put them.  Text
  comes from drawtext.c's vertex buffers (`D_80365348`, a quad of four
  vertices a character, in the order the strings were laid out; the
  display list draws them sorted by glyph): an element is the string a
  character is in, its neighbours in the buffer on the same line and of
  the same size, an advance or two apart (a right-aligned string, the
  money, is laid out backwards).  Anything else is a model (the radar,
  the TV, the counters' icons, from hd_code's data or the level's): an
  element is the vertex load it comes in.
- **Texture rectangles** (the counters' icons, the traffic lights) go by
  groups of touching ones, found by a pass over the display list before
  it runs, which follows the segments and the projection as it will:
  the traffic lights' strip over the top is eight rectangles from 32 to
  288, one element, and stays centred.

An element then moves by `-off` if its middle is left of x = 112, by
`+off` if right of 208, and in between by a ramp: a centred element
(the banners) doesn't move, and one that slides or types across (the
LEVEL COMPLETE and RDUS COLLECTED banners) doesn't jump.  The money and
the bonus amounts go right and left; the radar, the counters, the timer
and the TV in the top left corner left; the TV of the levels' intros,
bottom right, right.  `--interpolate` composes: the in-between passes
move the same elements (the texture rectangles' groups are the current
frame's), and so does the software renderer, whose frames are drawn wide
on the host (a moved rectangle is clipped by the wide frame's scissor
and blacks out no side band).

Not moved: the pause screen's map and its arrows (the yellow one points
at the carrier from where it is on the map), and nothing of the world
map or the front end.  The game never draws a HUD arrow clamped to the
4:3 edges for something off screen: the markers it places stay with what
they mark, and so show it in the wide sides.

None of this reaches the game: the display lists are the game's, the
RDP's time comes from the 4:3 geometry, and RDRAM is the same but for the
two color framebuffers, which the OpenGL renderer never writes anyway.
With `--widescreen` the TAS still beats the game (OpenGL, with and
without `--interpolate`: all 125,297 reads matched, none skipped, 57
platinum, the same report and save as without widescreen; also with the
software renderer at 21:9, and with OpenGL at 21:9 with `--interpolate`
and the HUD at the sides).  The LP64 build's replay, which drifts on its
own (3,993 of the log's reads skipped), gives the same report and save
with widescreen as without, either renderer.  `PORT_COUNT_PER_OP=0
--deterministic` runs (the TAS's first 30,000 frames, `PORT_AUTOSTART=1`,
and `PORT_AUTOSTART=3` with `--interpolate`) write the same save and
sound with and without it, 32-bit and LP64, and the same RDRAM outside
the framebuffers (with OpenGL, the same RDRAM).

## Model icons

Many of the game's icons are 64x64 pictures Rare rendered from its own
models: the missile carrier and the levels' goals (crates, rafts, gas
plants, containers, spheres) in the hint panels and the level's goal,
the vehicles of a level on the world map and the best times, the
communication point.  `--model-icons` (`PORT_MODEL_ICONS=1`; the default
with a window, off headless and with `--deterministic` or `--replay`)
draws the models instead, at the internal resolution
(`port/host/micons.c`).

- **Which pictures.**  An icon is one or two 64x32 (or 32x32) textures
  that `func_80272C5C` (hd_code 2E490.c) loads and `func_80272ED8` draws,
  as texture rectangles or as quads, its drop shadow (the picture in one
  colour, moved down and left) first.  The port's C there tells the host
  where each texture went and which one it is (`port_icon_texture`, a
  call: nothing in game memory changes); micons.c's table names the
  model for each picture, by its textures: a building of the model table
  (the goals: `D_802F9934`'s numbers, but the containers, whose entry is
  0, are `moconw1`) or a vehicle's model file (`D_8020E350`'s vehicles by
  type, `cmo` the carrier).  The question mark, the turning building and
  the coins stay pictures; the scientist's file isn't the man the
  picture shows.
- **Drawing.**  The renderer (gfx.c) skips any draw from such a texture
  (`micon_at`, by `G_SETTIMG`'s address, the texels checked against what
  was loaded there) and, once the picture's last row is in, runs a
  display list micons.c makes in memory of the renderer's own (gfx.h's
  `GFX_HOST_BASE`, which `gfx_ptr` reads; segment 15 points at it): a
  viewport over the rectangle the picture covered, a modelview that fits
  the model's box in it (turned and tilted as the picture is), the
  z-buffer cleared there, and the model's display list.  It runs from
  the state the game's list left and puts that state back, so the game's
  next draw is as it was.  The quads' rectangle is the screen box of their
  vertices; the HUD's rectangles (`--hud edges`) move with the HUD.
- **The models.**  Read from the ROM (or the pack's image) when first
  needed and kept: a building's head and the intact look's two pieces
  (`func_802BD1F8` copies them out of the model the same way), its
  vertices through segment 9; a vehicle's opaque and translucent passes,
  its vertices through segment 6 and its parts' matrices (segment 7) all
  identity, the parts at rest.  Their textures are decoded from the
  texture table (a pack's edited texels if it has them).
- **Shadow and fading.**  The picture's drop shadow, the model's outline
  moved, would show as slivers around a 3D model, and the buildings
  carry their own shadow on the ground (the pass-1 piece's fans in
  `G_CC_SHADE`, left out): instead the model throws one shadow, the
  convex hull of the vertices its triangles use, projected on the ground
  as by a light from above and to the right (at most 15 points, one flat
  polygon, so it is evenly as dark as the picture's), in the shadow's
  colour and alpha, under it.  A picture fading in or dimmed (the
  windows' unselected entries) fades the model: a copy of its list whose
  combiners take alpha from the environment colour, with a blending
  render mode.
- **Checking.**  `PORT_MODEL_ICONS_AS=N` makes the hint panels' question
  mark (and the goal's turning building) the Nth model of the table:
  `PORT_AUTOSTART=2 --model-icons --frames 1400` then shows each one in
  Simian Acres' first hint.  Nothing of it reaches the game; the
  references are made without it.

## The optimization pass, for the player

DISTRIBUTION.md's O0-O5 (2026-10-02/03), in short, for us.v10's TAS as the
yardstick (the same 125,297 reads, 57 platinum, the same gameplay digest
throughout):

- **No lag frames** (O1, "Lag frames").  The CPU model is off by default:
  the game's work takes no time, so a level frame is always two retraces
  (1/30 s) where the N64 dropped to 20 fps and below in busy scenes.  The
  level timer counts retraces, so the levels' times are shorter: 5% over
  the TAS, up to 12% in the busiest levels, and the whole run 9.6%;
  medals and best times are a little easier.  The front end's waits for
  the hardware (the controllers' power-on, the EEPROM's writes, the
  decompression) are gone too ("The front end's waits").
  `--cpu-model n64` and `--load-waits n64` bring both back.
- **The engine is ordinary C** (O2, O3: "Replacing the engine", "The
  engine made readable"): no translated code in any default build, no
  register or dead-stack leftovers (the few garbage values the original
  read are defined values a player can't tell apart, "Garbage made
  values"), typed records and named per-frame constants for a later
  60-tick engine.  Replaying the TAS headless, the whole process (game,
  engine and the OpenGL renderer at 1x) takes about 0.1 ms of CPU a
  retrace natively ("Engine hot spots").
- **A faster renderer** (O4, "The second round" and after): the page's
  work at 4x on a GPU from 6.5 to 3.8 ms a retrace, the in-between pass
  (`--interpolate`) about half as costly natively and a third less in the
  page, the same pictures byte for byte.
- **The digest** (O0, "The gameplay digest") is what checked each step:
  what a player sees, compared by the game's frame, so a change of timing
  that keeps the game passes.

## Performance

The budget is the N64's: 60 retraces a second, 16.7 ms each, into which
the game's CPU work (the fibers), the renderer (both passes with
`--interpolate`), the audio microcode, presenting and, in a browser,
Asyncify and the page's own work must fit.  The game draws 30 frames a
second in the levels; with `--interpolate` every retrace shows a new
picture.

### Measuring

`PORT_PERF=N` (`port/host/perf.c`) logs a line every N retraces (1: 600):
the work per retrace (the time from one retrace to the next less what the
loop gave away) as median, 95th and 99th percentile and maximum; the
retraces that took over 16.7 ms and how late they came; new pictures and
game frames a second; the mean of each part (`game`: the fibers, less
what they call below; `gfx`/`gfx2`: the two passes; `shaders`: compiling;
`gl-draws`: submitting the batches; `audio`; `present`; `loop`; `idle`);
the loop's sleeps and how far they overshot; SDL's audio queue; and how
often `PORT_PACED` dropped time.  In a page the lines go to the console,
and the port also hands each new picture's time to the page
(`Module.shown`).

`port/tools/web_perf.mjs BUILD ROM [--browser chromium|firefox] [--gpu]
[--throttle N]` serves a web build on a free port, plays it headless
(Playwright's `playwright-core`; `PLAYWRIGHT_CORE` names its
`node_modules` if node can't find it) with `PORT_AUTOSTART=3
--interpolate --widescreen` into Simian Acres, prints the `PORT_PERF`
lines and, every 5 s, what the display showed: of its frames
(`requestAnimationFrame`), how many had a new picture, how many repeated
the last, and how many came with two (one of them never seen); then a
summary of the gameplay windows.  Chromium runs WebGL on SwiftShader (the
CPU's GL: a pessimistic GPU) or with `--gpu` on the real one (ANGLE on
Vulkan: a Radeon RX 7900 XTX here), and `--throttle N` is CDP's CPU
throttling, standing for a laptop's slower CPU.  Firefox (Playwright's
build; any recent `playwright-core` drives it) can't be throttled, and
its `performance.now()` has 1 ms steps, so its per-part means are rough;
it also runs WebAssembly with the baseline compiler only (below).
`--profile-at S` takes a Chromium CPU profile; a `--profiling-funcs`
build (`-DCMAKE_EXE_LINKER_FLAGS=--profiling-funcs`) names the wasm
functions in it, and Firefox's own profiler runs headless with
`MOZ_PROFILER_STARTUP=1 MOZ_PROFILER_SHUTDOWN=FILE`.

### What was slow

In the page, before (`c0c99a5`), Chromium on the GPU at 1x: 8.3 ms of
work a retrace, of which 4.1 ms was presenting and 2.5 ms the loop;
neither was work.

- **The loop spun through the CPU model's waits.**  A game thread that
  has run ahead of the clock goes busy until the clock catches up
  (threads.c), for tens of microseconds at a time, dozens of times a
  retrace (natively the loop `nanosleep`s through them: 10,000 sleeps
  per 5 s).  In the browser a wait under 1 ms was spun (the page's
  timers can't do better than 4 ms), and the page got its thread back
  only on longer ones, or after 12 ms without.
- **SDL's `SDL_GL_SwapWindow` slept**: with Asyncify, SDL's emscripten
  driver calls `emscripten_sleep(0)` after each swap (a clamped
  `setTimeout`, 4 ms, and one more unwind): the 4 ms of "present".
- **Firefox's clock stretched the game.**  `performance.now()` there
  moves in 1 ms steps, and the CPU model's waits, measured with it, came
  out long: after the Rare logo the screen stayed black for 20 s and
  more, with no graphics task at all (reproduced natively by rounding
  the clock to 1 ms).  In the levels the game made 8 frames a second.
- **Presenting wasn't the display's**: a picture drawn went to the screen
  whenever the loop next yielded, and two drawn between two of the
  display's frames meant one never seen.
- **The sound wedged.**  More than 0.8 s queued in SDL made every new
  buffer be dropped; when the page's thread was held long enough for
  the `ScriptProcessorNode` to stop draining (Firefox), the queue played
  out and the sound stopped for good.
- **The renderer's GL calls**: about 360 draws a pass in the levels (a
  new batch at each texture change, which is every few triangles), each
  setting all of its state and uniforms again (25 GL calls, each a trip
  through JavaScript and WebGL's checks) and uploading its own vertex
  buffer; and a program was found by a linear search at each change of
  draw state.
- **Compiling shaders when first drawn**: up to 165 ms for one retrace.
- **The front end**: TMEM loads a byte at a time (the largest item in a
  profile of the level), every triangle copied through both clip planes,
  `fminf`/`fmaxf` as calls; and in WebAssembly every display-list command
  inlined into `run()` (binaryen inlines a function with one caller
  whatever clang is told).
- **Resolution**: the page asked for up to 6 times `devicePixelRatio`,
  4 million pixels, twice a frame, on a high-DPI laptop.

### What changed

- **`PORT_PACED=1`, the page's default** (main.c): virtual time inside a
  retrace, real time at it.  Between two retraces the clock jumps from
  event to event, as `--deterministic`'s does, so the CPU model's waits
  cost nothing and the clock's resolution doesn't matter; the next
  retrace waits until real time is there.  In the page it waits for the
  first animation frame from a quarter of a retrace before its time
  (`wait_display`, an `EM_ASYNC_JS` promise on `requestAnimationFrame`,
  with a timer in case none comes), and the retraces follow the frames
  that show them by at most 0.2 ms a retrace (a 59.94 Hz display, or
  120 Hz); a retrace already late is delivered at once.  After each
  picture, and every 8 ms of work, the page gets its turn through a
  `MessageChannel` message (no timer clamp): the picture goes to the
  screen before the next one's work, and the sound's callbacks run.  The
  host falling more than 4 retraces behind drops the time rather than
  catching up.  SDL's swap doesn't sleep (`SDL_HINT_EMSCRIPTEN_ASYNCIFY`
  off).  With `--display-hz` above 60 the presents between retraces wait
  for real time (and the display's frame) the same way, without the lock
  (the page turns `--display-hz auto` into the rate its animation frames
  run at, which SDL can't tell there).  Natively `PORT_PACED=1` works too
  (a `nanosleep` a retrace), but isn't the default.
- **Audio** (audio.c): more than 0.25 s queued clears the queue and
  starts over from the new buffer; the page resumes a suspended
  `AudioContext` on input; 1024-sample buffers in a browser.  Reproduced
  in headless Chromium on the GPU at 4x CPU throttling, `PORT_AUTOSTART=1`
  (an `AnalyserNode` on SDL's output): before, the sound went silent
  during the name entry, 15 s in, and stayed so for the rest of the
  two minutes; after, it plays throughout, into the level (the queue at
  40-210 ms, started over once).
- **GL state** (gfx_gl.c): a cache of what was last set (framebuffer,
  viewport, scissor, masks, depth, blend, program, textures) and each
  program's last uniform values; the batches of a task wait until
  something needs them drawn and then share one vertex upload, each a
  `glDrawArrays` of its range; programs are found by hash.
- **Shaders ahead**: the 110 programs the TAS and the attract mode use
  (`gfx_gl_progs.h`, written by `PORT_GL_PROGS=FILE`) are compiled while
  the logos run, a few a retrace (up to 4 ms).  The longest retrace of
  the run went from 165 ms to 11 ms.
- **The front end** (gfx.c): TMEM loads a word at a time, no clipping for
  triangles inside both planes, `min_f`/`max_f` inline (musl's
  semantics), the commands out of line.
- **`PORT_ADAPT=1`, the page's default**: a fence after each picture says
  whether the GPU had finished the one before last when the next is
  presented; if not in a quarter of a two-second window's retraces, the
  internal resolution goes down a step (it may go up again after a
  minute).  When more than 5% of a window's retraces come over 2 ms late
  and the GPU isn't behind, the in-between images go first, half of them
  at a time (with `--display-hz 144`'s 4 a frame: 2, 1, then none, the
  frame shown for all its retraces; `host_interp_limit` caps gfx.c's
  choice of twins), and come back twice as many at a time after a while
  without (a step holds 20 s, twice as long each time it had to be taken
  again soon after the step back): the game never slows for them.  The page caps its picture at about 1.3 million
  pixels (`--max-pixels`: 3x at 16:9, 4x at 4:3, 2x at 32:9).
- All of this leaves the game as it was: the GL screenshots (20 over
  3,000 frames of `PORT_AUTOSTART=3 --interpolate --widescreen`) and the
  software ones are byte for byte what they were; under node the wasm
  build matches the Linux `mn32` build's save, sound and 12 screenshots
  over 3,000 frames of `PORT_AUTOSTART=2`; the variants and the TAS as in
  "Testing the port".

Tried and not kept: `-O3 -msimd128` for the host side (no measurable
change), and taking the renderer out of Asyncify's instrumentation
(`ASYNCIFY_REMOVE`: Asyncify instruments 81% of the code, the renderer
included, since printf's indirect calls might unwind, but that cost
nothing measurable, in Firefox either; taking the pad's and the audio
microcode's calls out as well crashed).

### Results

us.v10, Simian Acres (`PORT_AUTOSTART=3 --interpolate --widescreen`),
1278x720 where not lowered, the windows of retraces 3,000-4,200; work per
retrace in ms (median / 95th / 99th percentile), and what the display
showed (frames a second with a new picture, of 60):

| browser, GPU, CPU     | before: work     | before: game, pictures | after: work      | after: on the display |
| ---                   | ---              | ---                    | ---              | ---                   |
| Chromium, GPU, 1x     | 8.3 / 12.8 / 13.3 | 28.5 fps, 57 drawn/s  | 1.8 / 3.0 / 3.3  | 59.8 new               |
| Chromium, GPU, 4x     | 11.6 / 22.5 / 25.2 | 6 fps (behind)       | 7.0 / 11.6 / 12.3 | 59.9 new              |
| Chromium, GPU, 6x     | 11.8 / 15.3 / 28.8 | 3.5 fps (behind)     | 10.0 / 16.7 / 18.1 | 51.2 new, 8.9 repeated |
| Chromium, SwiftShader, 1x | 5.6 / 51 / 70 | 20 fps, 11.5 display frames/s | 1.4 / 3.0 / 3.3 | 30 new (852x480, no in-between) |
| Chromium, SwiftShader, 4x | 11.6 / 24.7 / 30.1 | 6 fps (behind)   | 5.1 / 9.8 / 10.8 | 30 new (the same)     |
| Chromium, SwiftShader, 6x | 11.8 / 15.3 / 36.5 | 4 fps (behind)   | 7.6 / 14.7 / 15.6 | 30 new (the same)    |
| Firefox 153, GPU, 1x  | 12 / 17 / 22      | 8 fps, 9.6 drawn/s    | 10 / 14 / 14     | 59.9 new               |

"Behind": the game made far fewer frames than its 30, and the window
wasn't even the level by then.  After, the game makes its 30 frames a
second in every row; on SwiftShader the resolution goes to 852x480 and
the in-between pictures stay off most of the time, so each frame is
shown for two retraces.  The parts at 4x on the GPU, per retrace: game
2.1 ms, `gfx` 1.5, `gfx2` 1.3, audio 0.3 (before: presenting 4.3 and the
loop 4.8).

Natively (Linux, the `mn32` build, OpenGL headless, `--scale 6
--interpolate --widescreen`, 2556x1440): 2.2 ms of work a retrace before,
1.8 after, the two passes 1.06 ms against 0.73.

The module: 3.50 MB before, 3.52 MB after (0.93 MB gzipped).

### The second round (O4: measured hot spots)

DISTRIBUTION.md's optimization pass, phase O4, outside the engine
(`port/engine`, being rewritten at the time) and the CPU model: the
renderer, the WebAssembly build, the host.  How it was measured: the page
by `web_perf.mjs` with `PORT_ADAPT=0` (the in-between pictures always
drawn, so that the parts compare), the two builds alternately, twice each,
on a machine three other agents were building on (so: the means of each
part over the windows from retrace 3,000, and the ratios between the
parts, not single samples); natively by deterministic runs
(`--deterministic --renderer gl --scale 4 --interpolate --widescreen`,
the same work every run, three times each) and a sampling profiler
(`setitimer` on the main thread's CPU time, symbolized with inline frames:
`perf` and valgrind aren't there, or can't run this machine's libc).
The TAS with `PORT_PERF` and the OpenGL renderer gave the levels by cost:
Simian Acres, which `PORT_AUTOSTART=3` plays, is among the heaviest on
average (Argent Towers, Carrick Point and Havoc District with it); the
heaviest single windows are Tempest City's (its buildings coming down:
game 0.23 ms, `gfx` 0.58 natively).

**Where the time went** (us.v10, Simian Acres, Chromium on the GPU at 4x
CPU throttling, ms a retrace; at the start of the round, with the CPU
model still on): game 2.1-2.6 (of which the game's main loop alone, by
Chromium's profile, 0.8: see below), `gfx` 1.7, `gfx2` 1.4, GL calls
0.35, audio 0.3, the browser's own work about 0.5, the loop 0.2; about
6.5 ms of work.  Natively everything is small (0.75 ms a retrace, the
renderer three quarters of it): TMEM loads (`load_block`, 25% of the
main thread), the textures' hashing (6%) and lookup, `tri`, `do_vtx`, a
`memset`/`memcmp` per triangle in `build_state`, then audio (9%) and the
GL driver.  The TMEM loads were 3.8 GB over 4,200 retraces: about 300
LoadBlocks of 3 KB a task, half of them in the in-between pass.

What changed:

- **WebAssembly: Binaryen's inlining.**  `wasm-opt` inlines every
  function with one caller whatever its size: the game's main loop,
  `func_80244930`, had taken in most of the game's C (300,000 lines of
  wat, 272 locals), `main()` most of the host.  Under Asyncify each fiber
  switch unwinds and rewinds every frame on the stack, and rewinding one
  walks its body to the call it left from.  The link now passes
  `--one-caller-inline-max-function-size=200` (20 and 2,000 were slower):
  the game's C 2.6 → 1.3 ms at 4x in the page with the CPU model on,
  1.0 → 0.8 with it off (fewer switches); under node, deterministic,
  0.37 → 0.20 ms (native: 0.13).  The module grows 3.5% (1.5% gzipped).
  (Running `opt -O2` over the whole N64 side after `port-arena`, which
  only `llc` follows now, was tried too: 6% off the game natively, nothing
  measurable in WebAssembly, not kept.)
- **TMEM loads a row at a time**: a LoadBlock's words are copied a row
  (the run of words between two of `dxt`'s row changes) at a time, a
  `memcpy` or a swap of the halves of each word, not a call per word.
- **Textures found again by TMEM's generation** (`gs.tmem_gen`, bumped
  by every load): the lookup (`tile_texture`) keeps, per generation, the
  tiles it found and their GL textures, so the same tile over the same
  loads isn't hashed again.  The in-between passes run the first pass's
  loads from the same generation on and find their textures there (all
  but a few hundred of 900,000 lookups in a run); their loads aren't
  even copied then (`tload`, `gfx_tmem_sync`): only a lookup that misses
  (a triangle the first pass culled) copies, and only the loads that
  write what it reads and those beneath them (9% of the loads).
- **The interpolation tables** (64K entries each, cleared for every
  frame: 768 KB of `memset` a frame) are cleared by the entries the frame
  used.
- **A draw's state** is made again only after a display-list command that
  can change it (`gfx_state_serial`), not for every triangle.
- **The software fills** (the z-buffer's clear in RDRAM, every frame) a
  row at a time, out of line, and the rasterizer out of line (WebAssembly
  engines compile one huge function badly).

The pictures are byte for byte what they were: the OpenGL screenshots
of `PORT_AUTOSTART=3` (84 over 4,200 retraces, with and without
`--hd-text`) and the software renderer's, against the build before; the
quick tier passes in all eight variants (wasm equal to mn32 in every
hash); the TAS (free timing) on 32 and wasm as in "Testing the port".

Results, the same windows, ms a retrace (means of the parts; `PORT_ADAPT=0`):

| | game | `gfx` | `gfx2` | GL calls | audio | work (median) |
| --- | --- | --- | --- | --- | --- | --- |
| Chromium, GPU, 4x, before (CPU model off) | 1.0 | 1.66 | 1.37 | 0.33 | 0.31 | 6.8 |
| Chromium, GPU, 4x, after | 0.78 | 1.38 | 0.84 | 0.36 | 0.31 | 5.1 |
| Chromium, SwiftShader, 4x, before | 1.6 | 2.85 | 2.14 | 8.8 | 0.5 | 7.6 |
| Chromium, SwiftShader, 4x, after | 1.05 | 2.2 | 1.42 | 9.0 | 0.48 | 5.5 |
| native (mlp64), before | 0.04 | 0.28 | 0.25 | 0.07 | 0.05 | |
| native (mlp64), after | 0.04 | 0.19 | 0.11 | 0.07 | 0.05 | |

(SwiftShader at 1278x720 with every in-between picture is the GPU's
time, "GL calls"; `PORT_ADAPT` lowers the resolution there.)  From the
start of the round (CPU model on, before O1) to now, the page's work at
4x on the GPU went from about 6.5 ms a retrace to 3.8.

**Where it goes now** (Chromium's profile, GPU, 4x, 15 s in the level,
4.6 ms of busy time a retrace): the renderer 2.8 ms (`tri` 0.69, `run`
0.33, `do_vtx` 0.29, TMEM loads 0.17, `begin` 0.15, texture lookups and
hashing 0.27, the GL calls and WebGL's JavaScript about 0.3), the game
0.71 (below), the browser's own work 0.5, audio 0.31 (the microcode's
commands, the ADPCM decoder and the envelope mixer first) and SDL's
audio callback 0.1, the loop 0.17 (half of it `PORT_PERF`'s own clock
reads, then SDL's gamepad polling).

**The engine's hot spots** (for the engine round, O3/O4; by self time,
natively and in the page, which agree): the collision tests
(`func_8029C160`, a point against a piece's plane, and `func_8029B02C`,
`func_8029BD0C`, 56040.c; `func_802AA5E0`, a point in a triangle's box,
and `func_802AA094`, the ground's height under a point, 62740.c), the
buildings' display lists (`func_802BD1F8`, 77E20.c, the biggest single
function of the game in the page), the scaffolding the translation left
(`engine_leave`/`engine_leave64`, `engine_ctx`: as much as any one of
the above; O2 takes it out), the polls (`__port_poll`, 6% of the game in
the page), the matrices (`guMtxCatF`, `func_802ACCCC`), and libaudio's
voice mixing (`al_voice_mix`, `pull_table`).  None is more than 6% of
the game; the game as a whole is a sixth of the page's work now.

### The third round (the in-between pass replayed; `tri` in the page)

The two renderer items the second round left.  Measured as it was: the
page by `web_perf.mjs` (Chromium on the GPU, 4x throttling,
`PORT_ADAPT=0`, Simian Acres, the builds alternately), natively by the
deterministic runs, a sampling profiler and, new, the main thread's
instructions retired (`perf_event_open` from an `LD_PRELOAD`, 4,200
retraces) and `rdtsc` around `tri` (a scratch build).

- **The in-between pass replays the first** (`gfx.c`, "replaying the
  first pass"; OpenGL).  The first pass, when in-between passes will
  follow, records what their drawing depends on, as it runs: each vertex
  load's vertices as the transform and lighting left them (before
  blending), the point edits (`G_MW_POINTS`), each triangle and
  rectangle command, the TMEM loads, the 2D projection's changes (the
  HUD's), and, where a display-list command may have changed it
  (`gfx_state_serial`), the part of the state the drawing reads (the
  geometry and other modes, scissor, viewport, the color and z images,
  the texture's wrap) and the OpenGL draw state the first pass drew
  with (`gfx_gl_rec_state`: the program, the textures, the uniforms).
  An in-between pass then goes through the record: each load's vertices
  back into the vertex buffer and blended (`iblend`, the same records of
  the previous frame in the same order), each triangle and rectangle
  through the same functions as before (`tri`, `fill_rect`, `tex_rect`:
  the culling and clipping, the HUD at the sides, widescreen), drawn with
  the recorded draw state (`gfx_gl_rec_force`).  No display list is
  decoded, no matrix multiplied, no vertex transformed or lit, no
  texture looked up.  A triangle the first pass culled (behind the
  camera there, in view in between) may need a state the first pass
  never made: for those a snapshot of the state is kept (the second half
  of `GfxState`, taken only where everything of a state was culled) and
  the state made from it as the full pass would, TMEM brought up to date
  by the recorded loads as before (`tload_lazy`).  The full pass runs
  where a recorded state may have gone stale (a texture-cache flush, a
  new render target or resolution while the first pass ran:
  `gfx_gl_gen`; 46 of 2,673 passes in `PORT_AUTOSTART=3`, 316 of 203,855
  in the TAS), with the software renderer (whose cost is the pixels, not
  the list) and with `PORT_INTERP_REPLAY=0`.
- **`tri` in the page isn't three to four times native.**  The second
  round compared the page's profile at 4x throttling (0.69 ms a retrace)
  with native time.  At 1x, Chromium's profile gives `tri` 0.16 ms a
  retrace before (0.11 after); natively, from `rdtsc` around it, 0.14 ms
  (0.12 after) for the same triangles: WebAssembly costs 1.0-1.2 times
  native there, and the generated code has nothing to fix (no 64-bit
  arithmetic, the struct copies as `i64` loads and stores, the calls
  `to_screen`, `clip_poly`, `gfx_gl_tri` only).  What did cost: Asyncify
  instrumented 26 of the renderer's functions (`tri`, `run`, `begin`,
  `tile_texture`, `gfx_gl_tri`...: everything that can reach a GL call
  or `host_log`, whose indirect calls might unwind), a check of its state
  after every call and a save/restore path in each.  A graphics task
  never switches fibers (it runs on the loop's stack to its end), so
  `port/web/asyncify_remove.txt` takes them out (`-sASYNCIFY_REMOVE`;
  `port/tools/asyncify_names.py` checks the names are each defined once,
  `-DPORT_WASM_ASYNCIFY_ALL=ON` puts the instrumentation back).  Only
  `gfx_gl_init`, `gfx_gl_present`, `set_geometry` and the task's entry
  stay instrumented.  The second round's try at this measured nothing
  among the in-between pass's larger cost; now it is about a tenth of
  the renderer in the page (below).

The pictures are byte for byte the full pass's: the OpenGL screenshots
of `PORT_AUTOSTART=3` (every 250 retraces to 4,200, and every retrace
of 2,500-2,700, which alternate between frames and in-between images),
with `--hd-text`, at 4:3, with `--display-hz 144` (every retrace and
present of 2,500-2,650), `PORT_AUTOSTART=1` and `2` (6,000 retraces,
every 53rd), without `--interpolate`, and the software renderer's, all
against main's build, with the same sound and save; and the TAS replayed
with `--renderer gl --interpolate --widescreen` (137 screenshots over
275,373 retraces, the same save and replay report).  The quick tier in
all eight variants, the TAS (free timing) on 32 and wasm as in "Testing
the port".

Results, ms a retrace in the level (means of the parts from retrace
3,000; the page's runs scaled to the same audio time, 0.33 ms, as the
machine was shared and the runs drifted together by up to 30%):

| | `gfx` | `gfx2` | the two |
| --- | --- | --- | --- |
| Chromium, GPU, 4x, main | 1.36 | 0.88 | 2.24 |
| Chromium, GPU, 4x, the replay, Asyncify as before | 1.54 | 0.50 | 2.04 |
| Chromium, GPU, 4x, the replay, the renderer out of Asyncify | 1.43 | 0.45 | 1.88 |
| Chromium, GPU, 1x, profile (the renderer's own time, `gfx_task_call` down) | 0.64 before | 0.49 after | |
| native (mlp64, `--scale 4`), main | 0.188 | 0.110 | 0.298 |
| native (mlp64, `--scale 4`), after | 0.197 | 0.065 | 0.262 |

The in-between pass costs a third of the first now (it was two thirds),
and in the page the two passes together about a sixth less; the
recording costs the first pass 5-10%.  Natively the run's main thread
retires 10% fewer instructions (32.8 → 29.4 billion over 4,200
retraces); `tri` itself, by `rdtsc`, is 210 cycles a triangle in the
first pass and 59 in a replay (111 in a full in-between pass before,
which also culled what wasn't drawn).  Asyncify's checks were a tenth
of the renderer in the page.

### Engine hot spots

Measured after O3 (main d85d741), the default build (us.v10, the CPU
model off), for the engine's next round.  Natively: the 32-bit build
replaying the whole TAS (free timing) headless with `--renderer gl
--scale 1`, under `port/tools/sprof.c` (a sampling profiler preloaded
into the process: `perf` and valgrind can't be used here), every 250 µs
of CPU time, 106,671 samples (26.7 s of CPU for the run's 250,000
retraces, about 107 µs a retrace); `port/tools/sprof_report.py` gives
self time by function, a library's samples by a guess at their caller.
In WebAssembly: the `wasm` build linked with `--profiling-funcs` (the
names; otherwise the same module) under node's `--cpu-prof`, the
attract mode and `PORT_AUTOSTART=1` and `=3` (`port/tools/
cpuprofile_report.py`); node has no OpenGL, so there the renderer is the
software one.

Where the time goes, natively (the main thread, 91% of the samples; the
other is the GL driver's): the renderer 44% (`tri` 7.0, `hash_words` 6.9,
`run` 4.9, `tload_copy` 4.7 and its `memcpy` 1.5, `do_vtx` 3.1, `raster`
2.8, `tile_texture` 2.7, `gfx_fetch_texel` 2.0), the audio microcode
(`host_audio_task`, its commands inlined) 20.6%, the GL driver's calls
5.5%, the engine 3.8% with 7% more in the libm calls it makes (below),
the game's C 2.5%.  In node: the software rasterizer 91% (`sample` 30,
`raster` 17, `combine` 17, `gfx_fetch_texel` 16, `blend` 6), the host
3.5%, the game's C 0.6%, the engine 0.3-0.4% (about 16 µs a frame).

So the engine is no hot spot anywhere but in one place: natively, its
float-to-integer rounding.  The VR4300's conversions (`engine_cvt_w_s`,
`_l_s`, `_l_d`, recomp.h's `recomp_rint`/`recomp_rintf`) are `floor(x +
0.5)` in a replay (the TAS's emulator rounded halves up) and `nearbyint`
otherwise, and in the 32-bit build (SSE2, no SSE4.1 `round`) both are
calls into libm, `nearbyint` saving and restoring the FP environment
besides.  That is 7.2% of the whole process's CPU, two thirds of all the
engine's time.  In WebAssembly they are instructions (`f64.floor`,
`f32.nearest`) and cost nothing.

The engine's functions, by the share of the native TAS's samples (their
own and the libm calls under them):

| | function | what it does | share | why it costs | possible win | agent-hours |
|---|---|---|---|---|---|---|
| 1 | the rounding (`engine_cvt_*`, `recomp_rint*`; done, below) | float to integer as the VR4300 rounds | 7.2% (of it under `func_8029C160` 4.8, `engine_cvt_w_s` 1.6, `func_802AE160`/`AE104` 0.3, `piece_hit` 0.3) | a libm call each (`floor`, `nearbyint`) | all of it: exact inline versions (an `int64` conversion and a correction for `floor(x + 0.5)`; adding and subtracting 2^23 / 2^52 for round-to-even, which is `nearbyint` in the default mode) | 0.5-1 with the TAS (port/host/engine.c, recomp.h) |
| 2 | `func_8029C160` (56040) | a point against a collision triangle's plane: the distance (a 64-bit dot product over the normal's length) and the nearest point | 0.4 (+4.4 rounding) | called for every piece of every vehicle near something, a frame | after 1, little; skipping the projection when the distance already misses (its callers ignore the point then) | 1 |
| 3 | `func_802A57DC` (60F60) | a texture's decode: the stream copied aside, then decoded over where it was | 0.65 | the copy, and a branch a word | perhaps half: decode the literals' runs a word pair at a time | 1-2 |
| 4 | `func_802AA094` (62740) | a wheel's ground in the level's grid cell: each triangle's box, edges and plane (64-bit) | 0.34 | every wheel every frame walks its cell's triangles from scratch | most of it: each static triangle's box and plane worked out once at the level's load | 2-3 (the layout the C shares) |
| 5 | `piece_hit`, `func_8029BEE4`, `func_8029BD0C`, `func_8029B02C` (56040) | the parts' collision walks over the buildings' pieces | 0.5 together | many pieces per vehicle per frame | a coarse box test first | 2 |
| 6 | `func_802A4CDC`, `span_visible` (5FD50) | the terrain's visibility walk and its display lists | 0.33 | the quadtree walked every frame | little without changing the walk | - |
| 7 | `func_802ACCCC`, `func_802AA890` (679E0, 62740) and the game's `guMtxCatF` | matrix products (16.16, 64-bit sums), the vehicles' and the parts' | 0.6 together | many small products a frame | some: the product built in registers, not through D_803ED420 (kept only if nothing reads it) | 1 |
| 8 | `func_802BD1F8`, `copy_dl` (77E20) | the buildings' display lists | 0.19 | copies of each building's list | little | - |
| 9 | `func_802ABCDC` (62740) | a distance (`sqrt` of a 64-bit sum, rounded) | 0.11 | the rounding (1) and `sqrt` | 1 takes most | (in 1) |
| 10 | `func_802C41C0` (7F8B0) | the vehicles' engine sound | 0.10 | | none worth it | - |

Outside the engine the big ones were the audio microcode natively (a fifth
of the main thread before the changes below: the ADPCM decoder, the resampler and the envelope
mixer, sample by sample) and, in the page, the renderer's first pass
("What's left").

The tools: `sprof.c` and `sprof_report.py` (native, any build;
`--area` splits the engine, the game's C, the renderer and the host by
where each function is defined, `--callers` names a library's samples by
their caller), `cpuprofile_report.py` (node's profiles of a
`--profiling-funcs` WebAssembly build, the same areas by name), and
`engine_costs.py` (O3's per-function charges: measuring the blocks,
writing `ENGINE_COST`).

**Done since** (the same measure, the TAS natively with `--renderer gl
--scale 1`; the runs' own totals vary with the machine's load, so the
shares and the ratio to `tri`, which didn't change, are what compare):

- **The rounding (1).**  recomp.h's `recomp_rint` and `recomp_rintf`, and
  so `engine_cvt_w_s`/`_l_s`/`_l_d` and the translated code's
  conversions, no longer call libm: round-to-even is adding and taking
  away 2^52 (2^23 for a float) in the default rounding mode, `floor` that
  less one where it rounded up, a zero result with the input's sign.
  `port/tools/rint_check.c` compares them with `nearbyint`,
  `nearbyintf` and `floor (x + 0.5)` bit for bit over the edge cases
  (halves, the 2^23, 2^31, 2^52 and 2^63 boundaries, both zeros, the
  infinities, NaN), every quarter from -1024 to 1024 and 120 million
  random inputs: no difference natively in 32 and 64 bits, nor under
  emcc and node.  The engine with its rounding went from 11% of the main
  thread to 4% (3.8% of its own and 7% in libm before), the main thread
  about 10% less in all.
- **The audio microcode.**  `port/host/aspmain.c` reads and writes DMEM's
  halves as one access (a byte pair only at 0xFFF, where one wraps),
  walks the buffers that don't wrap with pointers (the envelope mixer,
  the mixer, the resampler), copies and clears with `memcpy` and `memset`
  (a move only where it doesn't overlap: the microcode's byte loop
  repeats an overlap), converts the resampler's taps once a task, unrolls
  the envelope mixer's outputs and, once both volumes are at rest (no
  ramp, or at the target every step lands back on), works its gains out
  once.  Nothing it computes changed: the whole TAS's sound (`--wav`,
  364 MB) is byte for byte what it was, and the quick tier's.
  `host_audio_task` went from 20% of the main thread to under 10% (from
  2.5 times `tri` to 1.1); what is left is spread over the ADPCM
  decoder, the reverb's filter, the mixer and the resampler.

### The page outside the renderer

The page's main thread less the renderer: what the rest costs, and what
makes the sound or the picture stutter.  Measured with `web_perf.mjs`
(Chromium 153 on the GPU, into Simian Acres, a 15 s CPU profile in the
level from 60 s on, a `--profiling-funcs` build) at 1x and at CDP's 4x
CPU throttling, and sorted by `cpuprofile_report.py --kinds` (the kind of
work from each sample's stack: under `gfx_*` is the renderer's, under
the microcode or the AI the sound's, Asyncify's JavaScript the loop's
sleeps and the fiber switches, and so on).

At 1x the main thread is busy 5.5% of the time, 0.9 ms a retrace (4x:
19%, 3.2 ms).  Of that, after the changes below (before them in
brackets where they differ):

| | 1x | 4x |
|---|---|---|
| the renderer | 59% | 51% |
| the browser's own work ("(program)": tasks, GL's command buffer) | 12% | 14% |
| the sound: the microcode (`host_audio_task`) and handing the buffers on | 8% (9.6) | 10% (11.8) |
| the game's C | 6.5% | 8% |
| the engine | 3.5% | 4% |
| Asyncify, the loop's turns (`wait_display`, `yield_now`, the rewinds) | 3.5% | 3.7% |
| the input (SDL's joysticks: `navigator.getGamepads` every retrace) | 2.9% | 2.3% |
| the host | 2.4% | 3.5% |
| other wasm (libc, SDL, libaudio) | 1.6% | 1.7% |
| fiber switches (the JavaScript side) | 0.1-0.9% | 0.2-0.9% |
| other JavaScript, the garbage collector | 1% | 1.2% |

- **Fiber switches are cheap.**  About 8 a retrace into a game thread and
  as many back (23,543 of each in 3,000 retraces of `PORT_AUTOSTART=3`
  under node), 1.5 µs into a fiber and 0.85 µs out (timed around
  `emscripten_fiber_swap` in `fiber_asyncify.c`): 20 µs a retrace.
- **Asyncify's instrumentation** is in the game's own functions' time, not
  apart; the loop's turns are what it shows.  Asyncify had instrumented
  1,333 of the module's 2,229 functions, among them every one that can
  reach `host_fatal` or a `printf` (their indirect calls might, as far as
  it can tell, switch fibers) and the audio microcode (it logs).  The
  printing, fatal and allocation leaves (libc's and SDL's), and
  `host_audio_task`, now go in `asyncify_remove.txt` with the renderer's
  functions: 989 left instrumented, the module 8.6% smaller (2.68 to 2.45
  MB with names).  The game's speed under node didn't change (14.4 s for
  those 3,000 retraces either way): the functions freed were ones
  without calls in their loops.  `asyncify_names.py` now reads the linked
  module too, and fails if a listed function is still instrumented.
- **The sound** was the stutter.  SDL's WebAudio driver is a
  `ScriptProcessorNode`, deprecated, whose callback runs on the page's
  thread and resampled there (SDL's `SDL_ResampleAudio`, 2% of the busy
  time at 4x); worse, the game makes its sound at the retraces' rate,
  which follows the display (`paced_wait`), and the device plays at its
  own: a display 0.1% off (59.94 Hz) drains SDL's queue for good or
  fills it, and SDL fills a queue run dry with silence.  With the device
  made 0.1% fast (`PORT_AUDIO_SKEW=1.001`, `PORT_AUDIO_SDL=1`), SDL's
  queue ran dry 2-4 times every 5 s at 1x, a few ms of silence each
  (and 0-1 times every 5 s at the display's own 60 Hz); 0.1% slow, it
  grew without end (121 ms queued after 90 s, until the 0.25 s cap throws
  it away).  The page now plays through an `AudioWorklet` of its own
  (`audio.c`): the browser's audio thread, a ring of the game's samples
  (s16 posted as they are, made floats there), an `AudioContext` at the
  DAC's 22048 Hz where the browser allows it (Chromium and Firefox do: the
  browser resamples to the device).  It steers the queue's low points,
  the level just before each of the game's buffers, averaged over a
  second, to 20 ms: 0.5% faster or slower (Catmull-Rom) while they are
  more than 5 ms off, the game's samples exactly as they are otherwise
  (after a correction it creeps a ten-thousandth fast to the next whole
  sample).  Played back and recorded in the page, 15 s of the level
  came out sample for sample the game's `--wav`.  With the device 0.1%
  fast or slow: no underruns in 90 s either way (the low points at 14-18
  or 20-26 ms, a correction of 0.2-0.3% now and then), about 37 ms queued
  on average (through SDL: 8-53 ms in its queue, and the
  `ScriptProcessorNode`'s buffers of 21 ms on top).  What is left are the underruns at the start
  (the logos and the level's loading hold the page for 50-90 ms; one of
  18 ms in the first 10 s), and in Firefox (its baseline compiler is
  slower) two or three there.  Without a secure context (plain HTTP from
  another host than localhost) there is no `AudioWorklet`, and the sound
  goes through SDL as before.  `web_perf.mjs` prints the sound's
  underruns and queue every 5 s, for either path.
- **The rest is small**: the input's `getGamepads` (2-3%) is SDL's poll a
  retrace, and a retrace's latency is what it buys; `host_loaded_dma`'s
  `snprintf` of a segment's name a DMA, 0.1%.  The renderer and the
  browser's own work are 65-70% of the busy time; of the rest the sound's
  microcode is the largest part, and the runs' totals vary by 10-20%
  between runs (the fiber switches' share among them).

### The fourth round (SIMD vertices; the draw calls measured)

The vertex transform in four lanes, and what batching the draw calls
would take.  Measured as before, and, new, by amplification: a scratch
build that runs `do_vtx`'s vertex loop N times over each load (the
results are the same each time) makes its cost stand out of the
machine's noise, natively, under node (deterministic, `PORT_VTX_REP`)
and in the page; and a micro-benchmark of the two loops, taken from
`gfx.c` as they are, under node and in Chromium.

- **`PORT_SIMD`** (CMake option, on by default; gfx.c): `do_vtx`'s loop
  (`vtx_simd`) and `mtx_mul` in four-lane vectors, through the
  compiler's vector extensions (`__attribute__((vector_size(16)))`,
  `__builtin_shufflevector`, `__builtin_convertvector`), which lower to
  SSE on x86, NEON on AArch64 and SIMD128 in WebAssembly, where gfx.c
  is built with `-msimd128`.  A vertex's 16 bytes come in one load (the
  halfwords swapped from big-endian by a shuffle, not in the
  native-endian builds), its position is the matrix's rows times x, y
  and z summed, its color one vector; with `G_LIGHTING` the lights' dot
  products four lights at a time and the colors summed as one vector.
  The results are the scalar code's to the bit: each lane does the
  scalar code's operations in its order, the host is built with
  `-ffp-contract=off` (no FMA: clang for AArch64 would otherwise fuse
  them), a light facing away adds +0 where the scalar code skips it
  (the colors are never -0), and the clamp at 255 is the scalar
  comparison's select.  A test (scratch: both loops on random matrices,
  vertices, lights and flags, NaN and infinities included, 300,000
  trials on x86-64, i386, wasm with and without SIMD128, big- and
  native-endian) finds no differing bit, and finds them at once when
  built with `-ffp-contract=fast -mfma`.  Off, or with the access
  profiler, the scalar loop is what it was.  In WebAssembly gfx.c is
  also built with `-fno-slp-vectorize`: with SIMD128 the SLP vectorizer
  turned the scalar loop (and code like it) into vector code V8 ran 30%
  slower.
- **What the browsers have**: WebAssembly SIMD128 is in Chrome and Edge
  91, Firefox 89 and Safari 16.4 (all of 2021-2023), so the page needs
  no fallback build for the browsers it is for; an older browser fails
  to compile the module (`-DPORT_SIMD=OFF` builds one without).
- **A store that cost three times the loop**: the first version stored
  the color as one 16-byte store, and in the game (under node and in
  Chromium) the loop ran three times slower than the scalar one, though
  1.5-2 times faster in the micro-benchmark; stored a float at a time
  it is 1.4 times faster than the scalar loop in the game.  Natively it
  is a wash: clang at `-O3 -msse4.1` had vectorized the scalar loop
  already.

The pictures are byte for byte main's: the OpenGL screenshots of
`PORT_AUTOSTART=3` (every 250 retraces to 4,200, every retrace of
2,500-2,700), with `--hd-text`, at 4:3, `--display-hz 144`,
`PORT_AUTOSTART=1` and `2`, without `--interpolate`, and the software
renderer's, with the same sound and save, in the `mlp64` and the 32-bit
builds; the variants and the TAS (free timing) on 32 and wasm as in
"Testing the port".

Results (us.v10, Simian Acres):

| | before | after |
| --- | --- | --- |
| vertex loop, node, ms a retrace (amplified, per pass over the loads) | 0.0137 | 0.0094 |
| `do_vtx` (with `vtx_simd`), Chromium, GPU, 4x, profile, ms a retrace | 0.084-0.104 | 0.074-0.075 |
| `gfx`, Chromium, GPU, 4x | 1.19-1.36 | 1.17-1.28 |
| native (mlp64, `--scale 4`), `gfx` / `gfx2` | 0.218 / 0.075 | 0.196 / 0.068 (the machine's noise) |

So the vertices were never much of the cost: about 3,400 a retrace in
the level, 2% of them lit (texture-mapped by `G_TEXTURE_GEN` mostly),
none fogged; the transform is a few microseconds a retrace at 1x.  The
page's renderer gains about 0.02 ms a retrace at 4x, half a percent of
its work.  Tried and not kept: `to_screen` and the culling's divisions
in vectors (no difference measurable).

**The draw calls** (a scratch count over 4,200 retraces): about 300
draws a pass in the level (1.24 million in the run, 28 vertices each),
and of two batches one after the other, 99% differ in their textures,
71% in their textures and tile parameters only (`tex`, `tA`, `tB`,
`tul`), 29% in their program, 4% in their colors (`prim`, `env`...),
3% in their depth state, none in their scissor or target.  Batches of
the same state are merged already (`begin` compares the state with the
batch's).  What batching could still take is the texture changes: all
textures in an atlas (or arrays by size), each vertex carrying its
tile's place and parameters as flat attributes in place of the
uniforms, the shader's `texelFetch` offset by them (the HD glyphs,
drawn filtered by `texture()`, kept apart); that would leave about 90
draws a pass.  What it would save: the GL part is 0.08-0.09 ms a
retrace at 1x and 0.30-0.39 at 4x in the page (of 2.6-3.1), of which
WebGL's binding, uniform and draw calls are about 0.05 ms at 1x by the
profile: at most 0.1-0.2 ms at 4x, 3-6% of the page's work, for
4-6 agent-hours and the shaders' texture addressing redone.  Not done.

### The fifth round (a late retrace's picture held; the first pass cheaper)

Smoothness first, then the first pass's hot spots.  Measured as before
(the page by `web_perf.mjs`, Chromium on the GPU, Simian Acres; here
with `PORT_ADAPT` on, the page's default, since what the display shows
is the point), and, new, by what each picture did on the display:
`web_perf.mjs` now tells how many display frames each new picture stayed
(0: drawn but never seen; 1 is what a 60-picture stream wants, 2 a
30-picture one), and how many retraces took over 16.7 ms of work or
were delivered 4 ms late or more.  Natively by the main thread's
instructions and cycles (`perf_event_open` from an `LD_PRELOAD`, the
deterministic run of 4,200 retraces).

**A measuring error first**: `web_perf.mjs`'s display count gave one
repeated picture and one never seen in every 5-second window, from how
it cut the windows (the last frame's picture was pushed after the
window was read).  The 0.2 repeats and 0.2 lost pictures a second in
the earlier rounds' tables were that; at 1x the display had 60 new
pictures a second, every one shown one frame.

**How often a retrace runs over** (the page, before):

| CPU (CDP throttling) | work median / p99 / max per retrace, ms | over 16.7 ms | pictures held 0/1/2/3/4+ frames | new on the display |
| --- | --- | --- | --- | --- |
| 1x | 0.9 / 1.7 / | 0 of 3,600 | 0 / 3,884 / 2 / 0 / 2 | 59.8/s |
| 4x | 3.7 / 6.1 / | 2 | 2 / 3,881 / 4 / 0 / 2 | 59.8/s |
| 8x | 7.7 / 21.7 / | 16 | 16 / 2,952 / 464 / 3 / 4 | 52.7/s (`PORT_ADAPT` drops in-between pictures now and then) |
| 12x | 8.1 / 29.8 / | 242 of 3,000 | 21 / 275 / 1,672 / 70 / 13 | 31.2/s (most in-between pictures dropped) |
| 16x | 12.9 / 38.4 / | 1,106 of 2,400 | 0 / 136 / 1,059 / 436 / 81 | 26.3/s (the game slows) |

So on this machine (a Ryzen 9950X3D) nothing runs over up to 4x, and
natively nothing ever does (work per retrace p99 under 5 ms, no
retrace 4 ms late).  From 8x on, the retrace with the game's frame (the
game's work and both passes) takes over a frame while the next one is
short: the frame before is shown three frames instead of two, this one
one (the 3s and 1s above).

- **`PORT_QUEUE`** (main.c, video.c; `PORT_PACED`, the OpenGL renderer):
  a retrace's picture is held (`host_video_frame_hold`: which
  framebuffer and in-between image, decided at the retrace) and
  presented one retrace later, at its own display frame: by the loop
  between events once the frame has come (`present_due`, then the
  page gets its turn so that the browser shows it), even in the middle
  of the next retrace's work.  A retrace's work may then take up to two
  frames as long as the next is short, and no picture is shown late.
  The game's timing is as it was (its retraces at the same times); what
  is seen comes a retrace (16.7 ms) later.  The framebuffer a picture
  is held from stays as it is until the game swaps it out, at a later
  retrace, which waits for the present.  `PORT_QUEUE=1`, the page's
  default, holds the pictures only when 3 retraces of 2 s came over
  2 ms late, and stops after 20 s in which no retrace's work took over
  80% of a frame, so that a machine that keeps up never gets the
  latency; `2` always holds them, `0` never.  While it holds them,
  `PORT_ADAPT` counts a retrace as late when its picture was presented
  after its frame.  `PORT_PERF`'s lines say whether the queue is on, and
  how many of its presents came after their frame.  Not with
  `--deterministic` (the tests) or `--display-hz` above 60.

The same build with `PORT_QUEUE=0` and with the default, alternately:

| CPU | `PORT_QUEUE` | pictures held 0/1/2/3/4+ | new on the display |
| --- | --- | --- | --- |
| 12x | 0 | 26 / 308 / 1,743 / 28 / 4 | 32.0/s |
| 12x | 1 | 21 / 2,934 / 471 / 1 / 4 | 52.5/s |
| 16x | 0 | 0 / 543 / 856 / 539 / 5 | 29.9/s |
| 16x | 1 | 16 / 115 / 1,845 / 24 / 4 | 30.6/s |
| 1x, 4x | 2 (always) | 0 / 3,889 / 0 / 0 / 2; 1 / 3,887 / 1 / 0 / 2 | 59.9/s, 59.8/s |

At 12x the pictures held three frames go from 28 to 1 and the
in-between pictures come back (52.5 new pictures a second against 32:
`PORT_ADAPT` no longer sees late retraces); at 16x, where the game can
just keep its 30 frames, the frames go from half held one or three
display frames (a stutter at every frame) to even twos.  Of the queue's
presents 6-11% still come after their frame at 12-16x: the loop can't
present in the middle of a graphics task, and the work of the two
retraces together runs over.

**The first pass** (gfx.c), all byte for byte:

- **Each vertex's divisions once**: the culling divided x and y by w
  for the three vertices of every triangle, and the screen transform 1
  by w for each again; now `vtx_tail` keeps each vertex's x / w, y / w
  and 1 / w as it is loaded (or blended, in an in-between pass) in
  `gs.vd`, the same quotients, and `tri` reads them (a vertex is used by
  about two triangles).
- **No cover for the RDP's time when nothing uses it**: `charge_poly`
  (the polygon's area and its bounding box, `min_f`/`max_f`, every
  triangle) only adds to what `host_charge` multiplies by
  `PORT_RDP_SCALE`, 0 by default: it runs when that isn't 0 (or for
  `-v`'s count).
- **TMEM loads a word at a time** for the even rows too (a rotation by
  0 or 32 where the odd rows exchange their halves), not a `memcpy` a
  row.

Results: natively (mlp64, 4,200 retraces) the main thread's
instructions 25.88 → 25.03 billion (-3.3%), its cycles 7.12 → 6.68
billion (-6%); in the page at 4x on the GPU (`PORT_ADAPT=0`, profile)
the renderer 1.97-2.03 → 1.78-1.85 ms a retrace, `tri` 0.42-0.44 →
0.25, `gfx` 1.25-1.28 → 1.06-1.13.

Tried and not kept: the textures found by comparing TMEM's bytes with a
kept copy of each texture's (a sample of 16 words as the key) instead
of hashing them all: exact, and 3.4% fewer instructions natively, but
in the page `tile_texture` took longer than it and `hash_words` had
(0.23 ms a retrace at 4x against 0.20): the kept copies, 16 MB of them,
come from memory, where the hash reads TMEM only.  The lookups hash
2.2 GB over the 4,200 retraces (a million lookups, 2.2 KB each; the
memo by TMEM's generation finds 4%); a load leaves TMEM as it was only
2.7% of the time, so a cache of what loads wrote wouldn't spare them.

### What's left

- **The first pass's own cost** is still most of the renderer: `tri`
  (a seventh of the renderer in the page after the fifth round), the
  TMEM loads (`tload_copy`) and texture lookups (`tile_texture`,
  `hash_words`: 2.2 KB hashed a lookup), `vtx_tail` (the in-between
  pass's records), `run`.  A digest of each load's bytes computed as it
  is copied, used by the lookups whose bytes one load wrote, would
  spare most of the hashing (about 3-5 agent-hours).
- **Firefox ran the front end three to five times slower** than
  Chromium here (`load_block`, `tri`, `do_vtx` in its profile), but that
  Firefox (Playwright's) has no optimizing WebAssembly compiler: with
  `javascript.options.wasm_baselinejit` off (and `wasm_optimizingjit`
  on) the page fails with "no WebAssembly compiler available".  Its
  numbers are the baseline compiler's; a Firefox with Ion should do
  better, unmeasured.
- **Draw calls**: one per texture change, about 300 a pass; an atlas
  would make them about 90, for 3-6% of the page's work at 4x (the
  fourth round).
- **A retrace whose work runs over**: `PORT_QUEUE` holds its picture
  for a retrace (the fifth round); a present in the middle of a long
  graphics task (the loop can only present between events) would take
  the rest of the late ones.
- **SwiftShader** trades resolution for in-between pictures; which of
  the two to give up first could be the player's choice.
- Presents between retraces (`--display-hz`) in the page come from the
  loop's own clock, rounded to the display's frames by `wait_display`,
  not from the frames themselves; checked natively only (144 Hz: the
  same 90 new pictures a second as without `PORT_PACED`).

## The glue to the translated code

The translated code is the check build's only now ("Replacing the
engine"); in the default build `entry.c` is empty and nothing else of this
is compiled.  `port/tools/gen_glue.py` generates both directions from the C:

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

## Replacing the engine

The translated engine is derived from the ROM, so a port that's published
needs Rare's handwritten engine written as C (docs/DISTRIBUTION.md, "The engine
rewrite").  That rewrite happened one function at a time, and every step was
checked and had to keep the TAS.

**It is done** (2026-10-02): every function `tools/recomp` translates is
native in every version, Rare's 688 and jp's 18 (its 17 IDO functions the
C still has as `GLOBAL_ASM`, and `func_802BA3E8_jp`; "Other versions").
The default build compiles none of the translation: the `recomp` library is
`entry.c` alone, which is empty, and a function missing from `port/engine`
fails the link (`recomp_func_X` undefined) where it used to fall back to
its translation.  Only the check build (`PORT_ENGINE_CHECK`, below) still
compiles the translation, to run it against the native code.  `test.py
quick` fails a build that compiles or links any of it ("engine": no
`recomp_*func_` symbol, no translated object in the build's graph).

**The mechanism.**
- `port/engine/replaced.txt` lists the functions that are native now, and
  `port/engine/<object>.c` defines them.  The code is readable C over the
  game's types (`#include "engine.h"`; `PTR32` on pointers that live in
  shared memory), built like the game's C: the N64 side, through BEPass,
  port-ilp32 and port-arena, so it works in every variant.
- `tools/recomp/translate.py` keeps a replaced function's translation as
  `recomp_orig_X` under `RECOMP_ORIG`, for the checks.  The port doesn't
  compile it.  Translated code calls the native function as `recomp_extern_X`,
  the same way it calls the game's C.  Functions that share code (one
  branches into another) are replaced together.
- The game's C calls the native function directly.  `gen_glue.py` makes
  the translated code's adapter from the C definition.  It saves the
  context, passes the inputs, and calls the native function.  Then it puts
  the context back, except for the registers the original may write
  (conventions.py), which keep what the native code's calls and leaves
  put there.  Last it sets the declared outputs.  For the other direction,
  native code calling a translated function goes through `entry.c`'s
  wrappers, as the game's C does.
- `port/engine/shared.h` declares every native function that another
  object calls, with its `REGS()`, so the call sites in all the objects
  agree.
- A function nothing reaches, in any version (no `jal` or `j`, no address
  in code or data: 679E0's `func_802ACDB8` and `func_802ACEB8`), is
  `func_X unused` in replaced.txt: no definition, and only the check build
  has its translation; `translate.py` fails if any translated code calls
  it.

**Register conventions.**
- Rare's code passes values in whatever registers suit it.  `REGS(t0, t3
  -> v0, a1)` before a definition or declaration names them, inputs then
  outputs.  The parameters map to the inputs in order.  The first output
  is the return value, and the others come back through trailing pointer
  parameters.
- A register name can be any GPR, `f0`-`f31` (`f64` takes the pair), `hi`,
  `lo`, or `sp+0x10` (a stack word).  `s64`/`u64` types move whole 64-bit
  registers, which Rare's fixed point uses (`dmult`, `dsra`).  An output
  `s1+f0` puts one value in both registers.  Without `REGS()`, the
  convention is o32's from the prototype.
- Leftovers are what the original leaves in registers besides its results
  (a float temporary, a loop pointer, a constant in `$at`) when some
  translated code reads them afterwards.  The native code puts them in
  the context itself: `ENGINE_LEAVE(gpr, v)`, `ENGINE_LEAVE_F(fpr, float)`,
  `ENGINE_LEAVE_FW(fpr, word)` and `ENGINE_LEAVE64`.  This works wherever
  the native code is called from.  gen_glue reads these, through the
  native callees too, so a register left that way doesn't count as a
  missing output.  A pointer left this way is `(u32)p`, its N64 address
  in every variant.
- `port/tools/conventions.py FUNC|OBJECT` works out what each function
  really takes and gives back: its inputs, and its outputs, the
  registers it may write that a translated caller reads afterwards.  It
  tracks stack slots, so saves and restores don't count, and it counts
  stores as uses.  It also lists pass-through outputs, which aren't
  written on every path.
- `gen_glue.py` reports in `glue_report.txt` the outputs a `REGS()` leaves
  out, and the extra inputs.  An input the C doesn't take keeps the value
  the caller left in the context, and so does every register the native
  code's calls into translated code don't set.  So a translated callee
  sees what it would have.
- Some functions have conventions that are all pass-through: everything
  their translated caller saves and restores looks like an output.  These
  go native together with their callers, and then the conventions don't
  matter.  679E0's `func_802AC284` and `func_802AC2A4` went native with
  the truck's 71140 this way, and 62740's carrying dispatchers with the
  vehicles' callbacks.

**The cost model.**  The game's pace, and with it the TAS, depends on the
engine's CPU time to the instruction ("Timing").
- The native code charges exactly what the original would have.
  `ENGINE_BLK(802AC1E4)` adds the size of the translator's basic block at
  that address, which is us.v11's in every version (the function's name
  plus the offset).  The size comes from the generated `engine_blocks.h`.
  The code calls it once each time the original would have run that
  block, and before any call where the original charges before the call.
- Blocks start at the function's entry, at labels, and after each branch's
  delay slot.  A block includes its branch and the delay slot, likely or
  not.
- `port/tools/engine_asm.py FUNC|OBJECT` prints the asm with each block
  and its size, plus the convention.  That's what to write from.
- BEPass neither counts (ICount) nor polls in this code (`BEPASS_ENGINE=1`):
  the translation polls nowhere, and a poll is a `host_cpu_sync`.  It also
  leaves `__port_` globals unswapped, which is what lets
  `__port_icount += n` work.
- Float to int: `engine_cvt_w_s` and its siblings, which follow
  `recomp_round_half_up` as the translation does.
- IDO's checked division is `ENGINE_DIV`, and a `break` or Rare's
  "can't happen" `syscall` is `engine_break`/`engine_syscall`.  They stop
  the port as the translation's `recomp_trap` does.

**Native-endian memory.**  The native code accesses memory at the widths
the original does, and in native-endian builds that is right everywhere
except where `tools/recomp/native_sites.txt` lists the function.  A
listed `x2` is a halfword inside a word (an Mtx's elements, a word stored
over two halves), and the native code reaches it through the word:
`func_802AC8CC`, `func_802AA890`'s `mtx_half`, `func_8029F85C`'s
identity matrices, `func_8029DA90`.  Look the function up there before
writing it.  The 32-bit check build can't see this, but `test.py quick` on
a native-endian build (n64, mn32) does.

**Checking.**
- `-DPORT_ENGINE_CHECK=ON` (32-bit builds only) checks every call of a
  replaced function from translated code or the game's C, if the
  original's translation calls nothing but translated code, or the game's
  C and libultra that `port/engine/check_repeatable.txt` lists: functions
  that change nothing but game memory, which the check puts back.  While a
  check runs, `port/host/threads.c` neither switches threads nor advances
  the clock (`host_time_stopped()`), a PI DMA completes at once
  (`port/src/ultra.c`, in the check build only), and its pending
  completions are put back with RDRAM; a blocking receive under a check
  stops the port.  So jp's IDO functions, which call the text and drawing
  C, are checked call by call.
- On each checked call, RDRAM, the host stack above the glue and the
  context are saved.  The translation runs, its results are kept and
  everything is put back.  Then the native function runs.
- The check compares RDRAM (except the dead stack below `$sp`), the host
  stack, the output registers (both the declared ones and the analysis's;
  for an IDO function called from the C, the declared result only) and the
  instructions charged.  A word that is a host stack address in both runs
  is no difference: the C they call keeps its locals there at another
  depth.  The translation of a function the C calls runs where `entry.c`
  ran it, the o32 frame below the C's `$sp`, so what IDO's code stores in
  its caller's argument slots is dead stack again.
- On a difference it prints where, and the first block where the two
  runs part (ids from `blocks.tsv`).  The game continues with the native
  results.
- `PORT_ENGINE_CHECK=N` in the environment checks the first N calls of
  each function and every 64th after that (default 2000).  `=0` checks
  every call.  It prints a summary at exit, and the first 10 differences
  of each function (`PORT_ENGINE_CHECK_SHOW=N`).  `PORT_ENGINE_COV=FILE`
  writes the ids of the blocks the checked calls ran.
- The fuzz reaches what no run does.  `PORT_ENGINE_FUZZ=READ:TRIALS:SEED`
  has a version's driver (`port/engine/fuzz_<version>.c`, `fuzz.h`) call
  replaced functions at controller read READ, TRIALS times each, on game
  memory it varies (the windows' states, a level's goal and rectangles,
  the HUD's kinds, the promotion screen's phases...), every call checked;
  memory, the context, the counts and the PI are put back before each
  trial and after the last, so the run goes on as without it.  jp's has a
  fuzzer for each of the 14 IDO functions that can run twice: with 500
  trials at read 1000 of `PORT_AUTOSTART=3`, over four seeds and with the
  autostart runs' own calls, they reach every block but IDO's `break`s,
  the sign fixups of unsigned values and a switch's case nothing sets
  (func_8026BCE0 418 of 446 blocks, func_80259EC4 137 of 138, func_802633E0
  41 of 47, func_802639B4 73 of 77, func_8025B498 11 of 13, the other nine
  all), and the native code agrees with the translation on every call
  (44,370 a fuzz run, a million more in the autostart runs).  The rest
  call C that can't run twice: func_801F57B0 (the Pak thread's start,
  which every run reaches), func_801F6F18 (the Pak's files) and
  func_802860F0 (a level's start) were reviewed against the asm twice.
- The quick tier runs with the CPU model off, so it can't see cost
  errors.  The check build sees them, and so does the TAS with
  `--cpu-model n64` (its "retraces given anyway").
- For functions that call C that can't run twice (the Pak thread's
  messages, the music's start), the TAS is the check.  Comparing `__port_icount` at every controller
  read between a build with the replacement and one without also finds
  where the cost first differs.
  `PORT_ICOUNT_LOG=FILE` writes that log: "read, `__port_icount`,
  `__port_icount_c`" per controller read.  Two `--deterministic` runs
  (with `--cpu-model n64`) of builds that should cost the same give
  the same file.
- `-DPORT_BLKLOG=ON` and `PORT_BLKLOG=FILE:FROM:TO:IDS` log every
  translated block run, and every `ENGINE_BLK`, between controller reads
  FROM and TO.  Two such builds, with and without the replacement, give
  the same file exactly when the native code charges what the original
  does, in its order; the first line that differs is the block to look
  at.  It found what the cost logs only narrowed to a frame in 77E20.
- A replaced function that saves registers and loads them back (77E20's,
  the front end's 1B100) does so with `engine_save(gmask, fmask)` and
  `engine_restore()`, since a translated callee's inputs and leftovers
  stay in the context; `engine_ctx(reg)` reads what a translated callee
  left in a register.
- What the original stores on the N64 stack stays there after it returns,
  and later code may read a slot it never wrote.  The driver's shadow
  (69BB0's `func_802AF340`) passes `func_802582C4` three stack arguments
  it doesn't store.  hd.c runs it right after the chopper, at the same
  depth, so the shadow's pitch is the chopper's model pointer, which
  `func_802ABBEC` saved in its frame.  With the chopper native and no
  frame, `__port_icount_c` drifted from read 5591 of the TAS, and a
  patch of the shadow differed.  A native function whose frame such a
  read reaches keeps it as the original does:
  - `engine_frame(-n)` moves the context's `$sp` as its `addiu`;
  - `engine_frame_sd(off, reg)`, `engine_frame_sdc1` and
    `engine_frame_sw` store at `$sp + off` as its `sd`, `sdc1` and `sw`
    (the context's registers);
  - one the game's C calls starts `ENGINE_C_FRAME` (16) below, where the
    glue would have started its translation.
  Native code calls translated code at the context's `$sp`, as Rare's
  `jal` leaves it.  The check build can't see this: it leaves out the
  dead stack.  The driver, native too, reads those slots with
  `engine_frame_lw(off)`, and writes the return address its `sd $ra`
  saves, each version's own, since two of the shadow's arguments are its
  halves.
  Since `func_802ABBEC` keeps its frame, every native path to it keeps
  the original's frames down to it: the level loader, the vehicles'
  setups, their functions each frame, their put-backs and the matrix
  functions those call, and 62740's carrying with its callbacks
  (`engine_frame_s()` is the usual frame of 0x58 and 0x30).  A saved
  `$ra` is the original's too: a native call into a function that saves
  it sets it first, as the `jal` would, with `ENGINE_RA(next block)`
  (the block's address in the version built, `engine_blocks.h`'s
  `ENGINE_ADDR_`), and the caller loads its own back (`engine_save()`
  with `ENGINE_GPR(31)`).  The TAS needs it: at read 108453 the shadow's
  pitch is the `$ra` that `func_802AB714`'s frame saved, the carrying's,
  and `__port_icount_c` drifted from there without it.  (O2 later
  removed all of these frames: the three stores that reach the shadow
  write its one word directly, "The scaffolding stripped" below.)
- A native called from another native gets its inputs as C arguments,
  and nothing puts them in the context.  When its original keeps or
  saves an input register, it leaves the input there itself at entry
  (`func_8029B02C`, `func_802A8768`): 89250's call of `func_8029B02C`
  left a stale `$t8`, and a translated `func_802A9CAC` later took
  another branch.  So too it saves and loads back with `engine_save()`
  the registers its native callees leave and its original reloads
  (`func_8029BF64`'s `$t8` inside `func_8029B02C`), since no glue
  between two natives does it.  The replay with `PORT_ICOUNT_LOG`
  against a build without the replacement, then `PORT_BLKLOG` around
  the first read that differs, finds such a register.

**What else the native code has to do** (from the loaders, terrain and
textures: 5BF40, 5CB60, 5FD50, 60D50, 60F60, 7F8B0, 8A080):
- `ENGINE_REG(gpr)` reads what the context holds.  Rare's code stores
  registers it never set (5CB60's collision triangles take bytes from
  whatever `$t9`, `$v0`, `$t6` and `$s1` held), and the C sees what the
  translated code would have, wherever it is called from.
- Where callers read many leftovers (5CB60's loader, whose registers the
  vehicles' inits may read), the native code mirrors each write of such a
  register into the context in the original's order (`ENGINE_LEAVE` at
  the point of the write), so the context ends as the original leaves it,
  whichever callee wrote last.
- A native function calling another native one directly gets the callee's
  `REGS()` outputs as C values only; if they are the caller's leftovers too,
  the caller leaves them.
- In LP64, `REGS()` parameters, results and outputs that are addresses are
  `u32`, not pointers.
- Native-endian memory: `tools/recomp/native_sites.txt`'s sites are the
  native code's to handle by hand: big-endian data (`be`: texture streams,
  palettes and texels, `__builtin_bswap16/32` under `PORT_NATIVE_ENDIAN`),
  bytes and halves of wider data at their N64 address (`x3`, `x2`: the
  address `^ 3`, `^ 2`), and the `lwl`/`lwr` words of the level files,
  which the loaders put in host order where they are.
- A function folded into its only callers is listed as `func_X inlined`:
  it has no definition or glue, and its translation is kept only for the
  check build (its callers' translations call it).
- A difference between versions is a `#if` on `VERSION_*` in the native
  code, as in the decompiled C (us.v10's `func_802A2D68` doesn't clear
  `D_803F7812`); `ENGINE_BLK` sizes are each version's.

**The scaffolding stripped** (DISTRIBUTION.md's phase O2, both halves of
`port/engine`, 2026-10-02).  With nothing translated left, the thread's
context and the dead N64 stack are only the native code's own: what
`ENGINE_LEAVE`, `engine_restore()` and the frame stores put there matters
only where some native code reads it back (`engine_ctx`, `ENGINE_REG`,
`engine_frame_lw`).  A write no read takes was removed: a read sees the
last write, and those are never last.  `ENGINE_BLK` stays (the
`--cpu-model n64` timing uses it).
- **Finding the readers.**  Two debug builds of the plain 32-bit port
  answer it, made side by side for the two halves (one since O3: the
  taint build does both, "The engine made readable"):
  - `-DPORT_ENGINE_TAINT=ON` tags each value put in the context or a frame
    slot with the call site that put it there, through the restores and
    frame stores that copy it, and logs every read with the chain of sites
    it saw (`ENGINE_TAINT=DIR`; `ENGINE_LWLOG=FILE` logs the words
    `engine_frame_lw` reads).  `port/tools/taint.py BUILD/blastcorps DIR`
    lists the readers and their chains, `--live` the sites some reader
    depends on.
  - `-DCMAKE_C_FLAGS=-DPORT_PROV` records the same per read as one record:
    the `ENGINE_LEAVE` that wrote the value, the `engine_restore` that put
    it back (only one that changed it), and for a stack word its store and
    the frames around it.  `PROV_OUT=PREFIX` writes `PREFIX.<pid>` at exit
    (not a `PORT_` variable, so `test.py` passes it on), and
    `port/tools/engine_prov.py EXE LOG... [--feeds FILE...]` lists every
    read with its writers, or the writes in some files that a read
    elsewhere takes.  `port_prov_probe(id)` counts whatever a native
    function wants counted.
  Over the quick tier and the TAS, about 300 of the engine's 2,000 sites
  were live.  The quick tier and the TAS, run unchanged, check what went.
- **What became ordinary C.**
  - Half A: 56040's `func_8029C0DC` gives `func_8029BF64`'s eight
    arguments as a struct to its own callers (`c0dc()`), `func_8029AA10`
    takes the vehicle type `func_8029A800` was given from a variable,
    `func_8029C160` no longer makes up a point for a miss its callers
    ignore, the `$a3` 56040's animation passed from part to part and its
    three dead stack-argument registers are gone, and 5BF40's
    `func_802A08E4` is `(dl, end)`.
  - Half B: `func_802A8768` takes `$s0`, `$s4` and `$s7` from `vs` (every
    caller has them from there: `unk96`, `&unk4C`, `unk4`), and
    `func_802A8CCC` returns whether it put the vehicle back (its `$t0`,
    `$t1`), its pass-through inputs gone; the wheels' material in half B's
    setups (`$fp`, which reaches a wheel's byte only when wheel 0 stands
    on a moving object or nothing: 0 in every run) is 0, and
    `func_802A6274`'s position and speed, unused in its mode 1, are 0 in
    half B's calls; `func_802AA890`'s `$s0`-`$s2` matter only for a point
    with no matrices, which no model has (`port_prov_probe` over the quick
    tier and the TAS), so `func_802ABBEC` and half B's `func_802AABE4`
    calls pass 0; the crane finds the hook's point as 56040 does
    (`record_3bd`), the chopper's `func_802A1388` gets 1 for the loader's
    heap top (only tested against 0), and its shadow's height outside the
    level is the model's address `func_802B9B4C` left in `$t3`.
- **The driver's shadow** reads one dead stack word: the third of its
  stack arguments is at `SHADOW_SLOT`, 0xBC below the N64 `$sp` the game's
  C runs the engine at, wherever the last native frame that reached that
  deep left it.  The frame stacks show which: 62740's `func_802ABBEC`
  under the chopper's frames (762 of the TAS's reads: the `$a1` 679E0's
  `func_802ACCCC` left), 62740_carry's `func_802AB714` (38: its saved
  `$ra`) and 69BB0's own `func_802AEC3C` (2).  No frame is left on any of
  those paths: the shadow reads `engine_frame_lw(SHADOW_SLOT)`, its other
  two arguments are the constants its frame stored (`0xFFFFFFFF`,
  `RA_SHADOW`), and the three writers store just that word where their
  frames did: `func_802AEC3C` (`engine_frame_sw(SHADOW_SLOT, ...)`), the
  chopper's `func_802B9B4C` and the carrying's `func_802AB670`, each last.
  That stays while the shadow's tilt is read from there.
- **What is still left, and why**: values that real game state is made
  from, that come from leftovers anywhere in a frame, kept with the leaves
  and the one- or two-register `engine_save()`/`engine_restore()` pairs
  that carry them:
  - `func_8029A800`'s two collision settings from `$t0`/`$t2` (the
    vehicles set them before the call; those that don't get
    `func_802ABBEC`'s);
  - the level loader's collision triangles' bytes 0x4F, 0x50, 0x58 from
    `$t9`, `$v0` and `$s1` (`func_802A41B0`; a hole's in 8A080 too), from
    62740's `func_802A860C`, `func_802A9DC0`, `func_802AA2E4` and
    `func_802A8768`, 77E20's drawing (which still mirrors `$t6`-`$t8`,
    `R()`) and `func_802BD99C`, and even the front end (1B100's `$v0`);
    60F60's `$t6`;
  - the wheels' `self` (`$t8`) and material (`$fp`) where the driver and
    half A's vehicles' setups pass them on, which half B's vehicle frame
    functions keep by putting back `$s4` and `$fp`;
  - the vehicle modules' effects' velocity and spin from `$t6`-`$s4`
    (`func_802A6274(..., engine_ctx(14)...)` in half A's calls), a
    matrixless point's `$s0`-`$s2` in 56040's `func_8029C454`;
  - 77E20 and 89250 read `func_8029C0DC`'s corners from the context
    (`C0DC_LEFT`).
  They are garbage in the original (registers nobody set for that
  purpose), so replacing them with defined values is the gameplay
  digest's call, not the exact TAS's: a few effects' velocities, a byte of
  some triangles, a wheel's material off the level, the driver's `self`.
- The check build (`PORT_ENGINE_CHECK`) still compares as before and
  finds no difference in the quick tier's runs: the leftovers the
  removed sites made were no checked caller's output.

## The engine made readable

DISTRIBUTION.md's phase O3: `port/engine` in ordinary C, by module over
four agents (2026-10-03).  Each agent's part is a subsection below.  What
all of them keep:

- **The CPU model's charges stay.**  `ENGINE_BLK` charges the original's
  blocks for `--cpu-model n64`; the readable code charges, on every path,
  the same blocks as before (in its own order: a C loop's test is
  `for (...; ENGINE_BLK(test), cond; ...)`, its body's block inside), so
  the n64 timing is exactly what it was.  A loop that became one copy
  charges its blocks in bulk (`ENGINE_BLKN(addr, k)`, buildings.h), and
  code several functions share takes its callers' blocks from a table
  (`Blk`, `BLKT`).  The check build (`PORT_ENGINE_CHECK`) compares the
  cost and the state, which stay; its block trace, a diagnostic for a
  difference, now comes in another order.
- **The check.**  Besides the quick tier and the TAS (free timing): the
  quick tier's four scenarios with `PORT_COUNT_PER_OP=2` (the n64 model,
  lag frames and all), whose save, sound, digest and every screenshot have
  to be the build before's, byte for byte: the CPU model's clock turns any
  difference in the charges into a different game.

### Buildings and the world

77E20 (the buildings and their destruction), 89250 (the game's C's
collision tests against them), 8A2E0 (the communication points), and the
vehicle modules that are copies of others' (80280 the J-Bomb and the
level's status, 8AEE0 the hotrod and the Cyclone Suit, 83910 the barges,
853D0 the Ballista, 88160 the A-Team van, 772A0 the train; 7F8B0 and
86ED0 were readable already).

- **Typed records** (buildings.h): the model header's fields the buildings
  read (`M_STRENGTH`, `M_DEBRIS`, `M_SHADOWS`, `M_MAIN`, `M_VALUE`,
  `M_MOVES`, ...), its sections (`MS_*`, each up to the next), a group's
  looks (`GroupDl`: the conditions on other groups' damage that hide it,
  `GroupCond`, then its two lists, `GroupDls`), the animated textures
  (`AnimTex`), what a group rests on (`MS_SUPPORTS`), a building's
  per-group state past objects.h's fields (`B_FALLING`, `B_FALL_T`,
  `B_SPIN_X..Z`), the effect records' fields (`FX_*`; still read through
  `FX_W/H/B`, whose halves and bytes native-endian memory keeps at their
  N64 places), the delayed-damage queue (`DelayedHit`), the smoke, dust and
  falling-shadow slots (`Smoke`, `Dust`, `FallShadow`), and in 77E20 the
  target objects (`TargetObj`), the level's group sets (`GroupSet`) and the
  models a vehicle can't harm (`Immune`).  The display-list commands they
  write are `DL_*`.  Names only where the code shows the meaning (a field
  `unkXX` otherwise).
- **Control flow**: the asm's head-tested loops are C loops, its branch
  ladders `if`s; what three functions did alike is one helper with each
  caller's blocks: the sphere against a piece (`piece_touched`: its plane,
  its triangle, an edge, a corner), the heading to a point
  (`heading_to`), whether the level's goal is open (`goal_open`), an
  object destroyed (`obj_destroyed`), a look shown as its group goes
  (`look_shown`), a limited copy of a look (`copy_cmds_n`); the Ballista's
  wheelie height (four copies) is `wheelie()`.  The barges' three copies
  of the same code (83910) are one setup, one frame and one drawing
  function over the barge's number, each copy's blocks in a table named by
  the first copy's addresses (`BB(802C8C90)`); the nine original
  functions call them.  The J-Bomb's mode machine (80280's
  `func_802C61F0`, a ladder of gotos) is a function per mode
  (`jbomb_mode`: landed, slamming, walking, dropping, flying, and the slam
  and the bounce).
- **Leftovers**: 77E20 and 89250 take `func_8029BF64`'s arguments from
  `piece_flat()` (56040's `collision_flatten()`, with its blocks) instead
  of the context (`C0DC_LEFT` is gone; the leaves in `$v0`-`$t1` went too,
  since the taint build finds no other reader of them).
  80280's and 8AEE0's `func_802A6274` calls (mode 1, which never
  reads them) and 772A0's `func_802AABE4` (a matrixless point no model
  has) pass 0 instead of context registers.  Kept, for 5CB60's collision
  bytes and 69BB0's driver (their readers are the core agent's): 77E20's
  `$v0` (`func_802BD1F8`), `$fp` (`func_802BD99C`), `$t9`
  (`func_802BE944`, `func_802BEADC`), `func_802BD1F8`'s `$t6`-`$t8`
  (three locals now, left at its end) and its `$s4`, `$t8`, `$t9` saves.
- **`func_802BD1F8`** (the buildings' display lists, the biggest single
  game function in the page, O4): it looked up each building's cell in
  the visible cells' list; now a table marked from the list once a frame
  (and cleared after) gives the place, and its blocks are charged by it.
  The display lists are copied by plain loops (a memcpy call for a few
  commands cost more than the copy) with their blocks charged in bulk.
  Natively (the 32-bit build, `PORT_AUTOSTART=3`, rdtsc around the
  function's own work, its callees left out): 21.3-22.6 Mcycles per 1,500
  calls before, 18.4-18.8 after, about 14% less; Simian Acres draws about
  37 buildings, 54 looks and 112 head commands a frame.  With every
  `ENGINE_BLK` taken out (an upper bound, not a build) it would be 11%
  less again: what is left is the work, mostly reading the models.
  Pictures byte for byte (the quick tier's screenshots, both CPU models).

**Checked** (us.v10, 32-bit, each batch): the quick tier as the
references, its four scenarios under `--cpu-model n64` byte for byte the
build before's, and the TAS (free timing: 125,297 reads, 57 platinum, the
reference's save and gameplay digest).  At the end: all eight variants
(`test.py variants --gameplay`, `wasm` included) equal to the references
in every hash, jp's quick tier as its references, and the whole TAS
replayed with the n64 model (`PORT_COUNT_PER_OP=2`, free timing) gives
the same digest (its clock included) and save as main before O3: the
charges are the same on every path the TAS takes.

**The per-frame numbers** (for the 60-tick mode; DISTRIBUTION.md's
"(a) at N=2"): a rate is a frame's, a count is in frames.

| name | value | what |
|---|---|---|
| `FALL_ACCEL` | 30000 | a falling group comes down `FALL_ACCEL * t^2` (16.16, t its frames, `B_FALL_T`) |
| `B_SPIN_X..Z` × `B_FALL_T` | `D_803649E0..E4` | its spin: the rates (12-bit angles a frame) times its frames |
| `DELAY_MAX` | 20 | a queued hit (explosions, `func_802BC888`) waits 1..19 frames |
| `SMOKE_FADE`, `SMOKE_ALPHA`, `SMOKE_WAIT` | 20, 0xFF, 3 | a smoke cloud fades by 20 a frame from 0xFF; rests 3 frames |
| `DUST_ACCEL`, `DUST_SHAKE`, `DUST_WAIT` | 16, 32, 3 | the dust rises faster by 16 a frame, shaken ±32 each frame; rests 3 |
| `SHADOW_ACCEL`, `SHADOW_SIZE_MAX`, `SHADOW_HOLD`, `SHADOW_WAIT` | 15, 0x385, 6, 3 | a falling group's shadow grows faster by 15 a frame to 0x385, lasts 6 more, rests 3 |
| `FX_DEBRIS_DELAY` | 10 | each debris piece but the first starts 10 frames later (`FX_DELAY` counts down a frame) |
| `COMM_SPARKS` | 24 | a communication point's sparks, one a frame (`FX_DELAY` 0..23) |
| `PUSH_FRAMES` | 20 | pushed this many frames in a row brings the group down (`func_802BEFF4`) |
| `SHAKE_HIT_FRAMES`, `SHAKE_DOWN_FRAMES` | 10, 15 | the screen's shake (00000.c: its size falls by a sixth a frame) |
| `AnimTex.lo` (kind 0) | per model | an animated texture's frames per step; kind 1 every `lo` sixteenths of `D_803649D8` |
| the frame counters | — | `D_80358068` (consecutive hits: `func_802C0284`, `func_802BEFF4`), `D_803F77F8`, `D_803F77F4` |
| vehicle modules' (as the vehicles agent's, below) | | `*_TURN_RATE`, `*_BRAKE`, `*_SLOPE_DIV`, `*_STEER_DIV(_AIR)`, `*_WALL_TURN`, `*_HIT_FRAMES`, `*_HIT_MIN_SPEED`, `*_GRAVITY`, `*_BOUNCE_MIN/_DIV` for `TRAIN_`, `VAN_`, `STARSKI_` (the other hotrod), `SUIT_`, `BIKE_`, `BARGE_`, `JBOMB_` |
| `TRAIN_SHUNT_SPEED`, `_SHUNT_FRAMES` | 0x28, 6 | sent back from an end of the line, or stopped if it left the other end fewer frames ago |
| `SUIT_ROLL_SPEED`, `_UNCURL_SPEED`, `_IDLE_CHANCE` | 0x118, 0x3C, 30 | the Cyclone Suit rolling, getting up, an idle animation one frame in 31 |
| `BIKE_LEAN_RATE`, `BIKE_MISSILE_WAIT`, `BIKE_WHEEL_SPIN_DIV` | 0.05, 5, 6 | the Ballista's lean a frame; frames between missiles; its wheels' spin |
| `BIKE_WHEELIE_*` | G 16, V 0x3C, HOLD 0x17, BOOST 0x33, BOUNCE 0x3C | its wheelie: height `v t - 16 t^2`, held up to 0x17 frames |
| `JBOMB_AIR_DRAG`, `_AIR_FRAMES`, `_MODE_GRACE`, `_JET_LIFT`, `_JET_TOP_PUSH`, `_CLIMB_FRAMES` | 8, 9, 3, 0x1E, 0x1E, 0xA | the J-Bomb in the air: speed lost a frame, its timers, the jets' push |
| `JBOMB_SLAM_FRAMES`, `_SLAM_PUSH`, `_PRESS_FRAMES`, `_LAND_FRAMES` | 0x14, -0x4B0, 4, 0x1E | the slam (three presses each within 4 frames), standing still 30 frames lands it |
| `JBOMB_AIR_STEER(_UP, _MAX)`, `_TILT_RATE/_BACK`, `_PITCH_RATE/_BACK`, `_FLAMES_MAX` | 0x14 (+2 to 100), 0.02/0.01, 0.03/0.005, 0x32 | its steering and its jets' tilts and flames, a frame |
| `JBOMB_BOUNCE_SPEED`, `_BOUNCE_LIFT`, `_SLAM_SHAKE(_FRAMES)` | 0x14, 0x14A, 0x320 (0x14) | a bounce in flight, the slam's shake |

(The sparks' `func_802A6274` speeds and the animations' frame counts
(`func_802A039C`) are the effects' and 56040's.)

**The vehicle modules, second pass** (after the vehicles agent's
vehicle.h): 80280's J-Bomb, 8AEE0 (the other hotrod, `STARSKI_`, and the
Cyclone Suit), 853D0 (Ballista), 88160 (A-Team van), 772A0 (train) and
83910 (barges) are written as the vehicles agent wrote theirs: the
`VS_*` accessors, `SET_WHEELS`/`SET_GEARS`, `FRAME_BUF`/`MODEL_AT`, the
frame's tail an `if` between the bounce (a function in each module) and
`turn_along_wall`, `PLAYER_FROM`; the van is 86F60's police car with its
own numbers, the hotrod 72B80's, the suit 6C5E0's Thunderfist with a third
leg (`SUIT_*` states, a switch); the J-Bomb's modes are named (`JB_WALK`,
`JB_FLY`, `JB_FALL`, `JB_DROP`, `JB_SLAM`, `JB_LANDED`) and
`func_802C61F0` is its timers, jets, mode changes and a switch over the
modes; the level's status packs and unpacks its bits with two helpers.
They charge as the vehicles part does, one `ENGINE_COST` a function at
its average, measured the same way (a counting build over the TAS and
the quick tier); the engine's total over the TAS with the model off is
34,832,520,465 instructions against 34,832,528,727 before (8,262 less, two
millionths of a percent), with the same digest and save.  77E20 keeps
its exact blocks (the n64 model's quick-tier hashes are unchanged by its
second pass: the delayed hits, the group sets and the effects' firing as
functions, the loops that went to a trailing block as `continue`s).
Leftovers dropped in this pass, the tests showing nothing reads them:
the frame functions' `$s0`-`$s7` saves (only `$fp`, as the vehicles
agent's), their setups' `$t0`-`$t5` saves, the matrices' `$s2`, the
wheels' material from `$fp` (0), the lean limits' `$s3`, the J-Bomb's
`$t2` (dead since func_8029A800 takes its settings as arguments, core
agent).

What is left (about 1-2 agent-hours): `unkXX` fields named as their
meaning is found; the 77E20 functions that still jump to a shared exit
(the damage rules, the push, the camera's push); the leftovers above
once 5CB60's collision bytes are defined values.

### The parts, collisions and loaders

56040, 5CB60, 5BF40, 60D50, 8A080, 5FD50, 1B100, 69944, 69014;
collision.h, shared.h.  A helper standing for several copies of the
original's code takes the caller's blocks as a table here too
(engine.h's `EngineBlk`, `ENGINE_B()`, `ENGINE_BLK_AT()`); where the
charges were meant to stay the same, `PORT_ICOUNT_LOG` over
`PORT_AUTOSTART=0..3` against the file before gave the same log, poll
for poll.

8A080, 5FD50, 1B100, 69944, 69014; collision.h, shared.h):
- collision.h has the collision records once: `CollisionTri` (the
  0x60-byte triangle and plane: `nx..d`, `nlen`, `nlen2`, `v[3][3]`,
  `heading`, `axis`, `owner`, `id`, `active`, `group`, `side`, `group2`,
  `pushes`, with buildings.h's `Piece` names), `Solid` (a kind's bounding
  sphere, D_803A7300), `KindPart` (its spheres, D_803A6B30), `TriSwitch`
  (D_803BC1D0's switches) and `Wall` (D_803BD310).  56040 adds `Anim` (a
  Part as an animation's state: data, t, tension, loops, limit, running,
  back, mode, key, speed, interp), `Key`, `AnimPart`, `TexAnim`,
  `TexSlot`, `TexPatch`, `MtxCopy`, `ObjSphere`, `ObjTris` and
  `BuildingRule`.  The kinds are vehicle.h's `VEHICLE_*`: the crane's hook
  is its sphere of radius 0x3BD, the train's hits are counted.
- 56040: the matrix builders share one identity/rotation/translation/scale;
  the twelve copies of the piece test are `piece_hit()` with each copy's
  blocks, and `func_8029AB88` is `func_8029B02C`'s walk stopping at a hit.
  `func_8029A800` takes `$t0`/`$t2` as parameters (the hit effect's length
  and the speed it needs: every vehicle that leaves them non-zero also
  enables the effect; the others pass 0), and `func_8029C454`'s
  matrixless point is 0.  `collision_flatten()` and
  `collision_flat_inside()` (shared.h's `FlatTri`) are `func_8029C0DC` and
  `func_8029BF64` for C callers (buildings.h's `piece_flat()` is one).
- Two things the names showed: a texture animation's "second texture" is
  the next key's (each key's kinds follow the last's; the blend's fraction
  goes into the `G_SETPRIMCOLOR`), and `func_8029DD54` walks one record
  past D_803B7FC8's 0x78 onto the pointer after them (the original
  compares with D_803B8568's address, not its value): kept, with a note.
- After `func_8029A800` and `func_8029C454` took values, the taint build
  over the TAS showed thirteen more of 56040's leftovers that no reader
  takes; they are gone.  After the merge with the buildings' and the
  vehicles' halves (their callers take `collision_flatten()`'s struct and
  pass 0 to the effects' and `func_802AABE4`'s dead inputs), nine more
  went: `func_8029C0DC`'s corners are no native code's any more, nor
  `func_8029E5AC`/`func_8029F1BC`/`func_8029E21C`'s effect registers, nor
  `func_8029C454`'s `$s1`/`$s2`.  The rest went with their readers
  ("Garbage made values", below).
- 5CB60 reads the level file and the models by `LevelHeader`, `Model`
  and `VehicleModel` field (`LEVEL_PTR()`, `MODEL_PTR()`, `VMODEL_PTR()`),
  with structs for the file's records and the tables they become
  (level_tables.h's `TargetObj` and `LevelLight`, `MovingGroup`/`MovingTri`,
  `FileVehicle`, `FileCarrier`, `FileBuilding`, `BuildingTri`,
  `ModelEffect`); 8A080's `CollisionObject`, 60D50's `HeightBox`; 5FD50's
  `QuadNode`, `TerrainGroup`, `TerrainSpan`, `TexAnim` and the header's
  display lists by index.  Their copies of the same code are helpers
  (`tri_grid`, `load_gz_model`, `tex_load_now`, `highest_box`, 5FD50's
  `dl_append`/`dl_patch`/`cell_visible`); 5CB60's shared helpers charge one
  copy's block ids (the same sizes), and `func_802A3F80` charges 802A4114
  once more per switch at a level's load.  No goto is left in these.
  Every `ENGINE_REG`/`ENGINE_LEAVE` they had stays: the triangles' bytes
  from `$t9`, `$v0`, `$s1` and the rest are still the gameplay digest's
  call.
- The debug builds are one: `-DPORT_ENGINE_TAINT=ON` also records each
  read's first value and `engine_probe(id)` counts (what `PORT_PROV` and
  `port_prov_probe` did), and `taint.py --feeds FILE...` lists what some
  files' sites give readers elsewhere (`engine_prov.py --feeds`).

**Per-frame constants** (what a different tick rate has to scale; each
runs once a game frame):

| where | name | value | what |
|---|---|---|---|
| 56040 | `ANIM_RATE_DIV` | 300 | an animation's key speed is in 300ths of a key a frame (times `Anim.speed`); the texture animations' too |
| 56040 | (`func_8029A914`) | 1 | the gears (`unkA0`) fall one a frame to 1 when nothing stepped them |
| 56040 | `GEARS_MAX` | 0xC8 | their limit; a hit steps them 1 to 8, else up one (`func_8029B7CC`) |
| 56040 | `BACKUP_SPEED` | 10 | `unk76` once it reaches 0 (after the mode's first frame) |
| 56040 | `TRAIN_HITS_MAX` | 0xC9 | the train's hits, counted once per hit per frame |
| 56040 | `HIT_FX_UNIT` | 7000 | the hit effect's life per `fx_len` (60F60's units) |
| 56040 | `D_803A7429` | once | one hit effect a frame |
| 5FD50 | `VIS_NODES_PER_FRAME` | 1 | the terrain quadtree's nodes the visibility walk tests a frame |
| 5FD50 | `D_803C3248` | the caller's | frames before the walk starts, one down a frame |
| 5FD50 | `TexAnim.period`, `.count` | the level's | frames per terrain texture frame; `count` steps one a frame |
| 5FD50 | `ANIM_LOD_MAX` | 0xFF | the blend is `255 * count / period`: 1/period a frame |

The loaders (5CB60, 5BF40, 8A080, 60D50) and 1B100, 69944, 69014 have
none: they run at a level's load, or are pure functions.

### Garbage made values

The last code that read what other code left in registers or on the dead
stack now takes values, so nothing reads the thread's context or the N64
stack any more: every `ENGINE_LEAVE`, `engine_save()`/`engine_restore()`
pair, `ENGINE_RA` and frame store in `port/engine` went with them (engine.h
keeps the mechanism for the check build, which still runs; its one
difference over `PORT_AUTOSTART=3` is 60F60's `func_802A5FA8`'s cost, its
`ENGINE_COST` average).  The taint build gave each reader's values over the
TAS (`ENGINE_TAINT_VALUES=1` and `taint.py --values`: every value a read
took, with its count), and the values chosen are:

| read | what the original takes | now | why no one can tell |
|---|---|---|---|
| the level's grid and wall triangles' owner, id and pushes (0x4F, 0x50, 0x58), the switch triangles' pushes, the holes' too and their group2 (5CB60's `func_802A41B0` callers, 8A080) | `$t9`, `$v0`, `$s1`, `$t6`: an address's low byte, an earlier triangle's dx | 0 (`LEVEL_TRI_BYTES`) | nothing reads them for those triangles: owner and id only for the objects' (56040), pushes only for the buildings' pieces (func_802BF264), group2 only for those |
| the driver getting out (69BB0's `func_802A9A60` calls): its `self` | `$t8`: the last vehicle collision's type (0xFF, 4, 0xBE...) or a matrix's address | `VEHICLE_DRIVER` | `self` only keeps a vehicle off its own moving triangles; the TAS's replays and digest are the same |
| the same: its wheels' material | `$fp`: 0 in 2,169 of 2,814 reads, else 2 or an odd halfword | 0 | the next frame's ground sets it again; the TAS is the same |
| the vehicles' setups (69BB0, 772A0, 80280, 853D0, 88160, 8AEE0): the material where no ground is | `$fp`: 0, or the level loader's pointer | 0 | only used where no ground is found under a wheel at a level's start |
| 5CB60's models' animated textures (`func_802A2608`, `func_802A24BC`): the palette param | `$fp`: what `func_802A3F80` left, the end of the switch triangles | that value, kept (`tex_param`) | it is the original's palette pointer for the palette types (60F60's 4 and 5): the same colours |
| 5CB60's level's animated textures (`func_802A1C20`) | `$fp`: 0 in 105 of 109 calls | 0 | as above, for the 4 others' textures if any are palette types |
| the driver's shadow's tilt (`func_802582C4`'s sixth argument) | the dead word 0xBC below the C's `$sp`: `&D_803ED3B0` (762 of 802 TAS frames, the chopper's frame), a `$ra` (38, the carrying's), 0 (2) | `SHADOW_TILT`, the first | its low half is the angle: about 83 degrees, as nearly always; 40 frames of the whole TAS draw the shadow at that tilt instead of another |

The TAS with these (us.v10, 32-bit, free timing): all 125,297 reads, 57
platinum, the reference's save and gameplay digest (`7ec6ef73a5265afa`);
the quick tier's every hash as before (so no reference changed).  Also:
buildings.h's `Piece` is now collision.h's `CollisionTri` (77E20's `p`,
`unk20`, `unk24` are `v`, `nlen`, `nlen2`), and level_tables.h has the
objects to destroy (`TargetObj`, D_80306480) and the lights
(`LevelLight`, D_803BDFD8) once for 5CB60, 62740 and 77E20.

### Vehicles

(The vehicles part charges differently from the other parts: one average a function, not the
original's blocks path by path, so with the n64 model a frame of vehicle code costs about what
it did but not exactly, and the `PORT_COUNT_PER_OP=2` byte-for-byte check above doesn't hold
for it; the quick tier and the TAS, with the model off as by default, are exact.)

62740 (the shared physics), 62740_carry, 60F60 (texture decoders and the
effects' sprites), 679E0 (math and matrices) and the vehicle modules
6B4A0 (Sideswipe), 6C5E0 (Thunderfist), 6E200 (Skyfall, Ramdozer), 71140
(Backlash), 72B80 (American Dream, the BCT chopper), 75490 (the missile
carrier, the crane), 86F60 (police car), 8DDB0 (the shuttle) and 69BB0
(the driver): 14,692 lines with 3,692 block charges and 444 `goto`s became
8,615 with one charge a function and 5 `goto`s (early exits).

- **The charges.**  Each function charges once, at its entry,
  `ENGINE_COST(addr, n)` (engine.h): n is what its original took a call on
  average, its blocks' instructions over the TAS and the quick tier
  divided by its calls (us.v10's block counts, a `PORT_BLKLOG` build that
  counted instead of logging); the 18 functions no run reaches charge
  half their blocks.  Over the TAS the total is the original's to 0.13%,
  so `--cpu-model n64` stays right over a frame; a single call is off by
  its path's difference from the average.  Blocks that macros charged
  (the decoders' back references, IDO's division checks, the camera's
  range tests) are folded into their function's average the same way.
- **The record.**  `port/engine/vehicle.h` names VehicleState's fields
  where the code shows what they hold, as accessors over the game
  header's offset names (others' code and the digest use those):
  `VS_HEADING` (unk4C, what it faces), `VS_MOVE_HEADING` (unk4E, the way
  it moves), `VS_TURN_HEADING` (unk74), `VS_SPEED` (unk76), `VS_GEARS`
  (unk78: five rows of lowest speed, highest speed, acceleration a frame),
  `VS_GRIP` (unk50: the wheels' materials' average, how many gear rows it
  may use and how fast it turns), `VS_WHEELS` and `VS_CARRY_WHEELS` (the
  three contact points' offsets), `VS_WHEEL_H` (unk4: three heights a
  wheel, now and the two frames before), `VS_WHEEL_FALL`/`_GROUND`/`_FRAMES`
  (unk28: a wheel in the air's upward speed, the ground it left, its
  frames), `VS_AIRBORNE` (unk96[0..2]), `VS_TURNING`, `VS_IN_SETUP` (unk9A),
  `VS_ON_STATIC` (unk9B), `VS_TOP_MATERIAL` (unk9F), `VS_SPEED_SIGN`
  (unkA5), `VS_SLOPE_RATIO` (unk0), `VS_CARRY_*` (unk6A..72).  The bytes a
  module keeps for itself are named in it (`MAGOO_STATE`, `MAGOO_LEG`,
  `CRANE_GRABBING`, `DRV_WALKING`...).  The controller's bytes are named
  by button (`PAD_L`, `PAD_R`, `PAD_Z` the brake, `PAD_A`, `PAD_B`,
  `PAD_LEFT`/`PAD_RIGHT` the 0x200 and 0x100 buttons, `STICK_X`,
  `STICK_Y` the throttle).
- **What is shared.**  Every vehicle's frame is now the same few lines:
  steering (`func_802A7E70`), the throttle (`func_802A785C`), the move
  heading turned toward the facing (`func_802A7FD8`), the slope
  (`func_802A83B8`, `func_802A843C`), the move (`func_802A860C`), the
  wheels on the ground (`func_802A8768`), the parts, what it hits, and then
  either its bounce off something (a function in each module) or
  `turn_along_wall` (vehicle.h) when the collision narrowed the headings it
  may face (D_803A7425).  The part levers and lifts the d-pad, A, B and Z
  move are vehicle.h's `pad_lever` and `pad_lift`; `PLAYER_FROM` hands the
  frame's result to the camera and the game.
- **Control flow.**  The chopper's flight is a switch on its named states
  (`CHOPPER_HOVER`...), Thunderfist's walk and ball likewise
  (`MAGOO_WALK`...), the carrying's dispatchers find a vehicle's callbacks
  in one table, the ground searches share `tri_under` and their loops, the
  matrix chain (`func_802AA890`) loads each matrix's elements once.
- **Slips kept.**  Where the original does something odd but it reaches
  play, the C does the same and says so: `func_802A9A60` gives every
  wheel its caller's material, not the ground's; the chopper's flight into
  a level works out its heading from (D_80368048, z) after the next stop
  changes; the driver's footsteps test the level's top byte (always 0).

**Leftovers kept.**  Garbage that reaches play stays as it was (the bar is
that a perfect player couldn't tell, and these change which triangles the
driver stands on and its grip, so they would have to be shown not to
matter in every case, not just the TAS's):

- the driver's material where its setup finds no ground (`engine_ctx(30)`,
  the $fp the level loader left: addresses, 0x803B97E0 and the like, so
  material 0xE0...), and getting out its `self` and material
  (`engine_ctx(24)`, `engine_ctx(30)`).  Measured over the TAS
  (`o3_probe`, a throwaway build): `self` is the type of the last vehicle
  stepped (0xFF, 4, 0xBE, 0xA...) in `func_802AEC3C` and pointers or small
  numbers (2, 5) in `func_802AF340`; the material 0 mostly, 2, 0x8E8D...
  A `self` equal to a moving object's id makes the driver ignore that
  object's triangles, and the material becomes its grip.
- the driver's shadow's tilt, the dead stack word at `SHADOW_SLOT`
  (0x803ED3B0's low half 762 times in the TAS, 62740_carry's return
  address 38, 0 twice): purely what the shadow looks like.  Making it 0
  would change screenshots, not play; left for the shadow's owner to
  decide.
- the registers some of these functions still leave for the other half's
  readers (`ENGINE_LEAVE`: 5CB60's and 8A080's collision triangles' bytes,
  56040's `func_8029A800` and `func_8029C454`, 772A0's train), and the
  `engine_save`/`engine_restore` pairs that keep them.

**The per-frame constants** a 60-tick mode would scale (DISTRIBUTION.md,
"a delta-based engine"), by what they are; each module's are at its top,
62740's and vehicle.h's shared ones at theirs.  Positions are `<< 5`,
angles 12-bit, a speed is a distance a frame:

| kind (at N ticks a frame) | constants |
|---|---|
| speeds, distances a frame (/ N) | the gear tables' lowest and highest speeds (`SET_GEARS`), `BOOST_TOP_SPEED` 0x190, `SKYFALL_TOP_SPEED` 0xFA, `CHOPPER_TOP_SPEED` 0xA0, `CHOPPER_SLOW_SPEED` 0x14, `CHOPPER_CLIMB` 0x14, `MAGOO_ROLL_SPEED` 0x1BE, `MAGOO_UNCURL_SPEED` 0x3C, `MAGOO_ROLL_MIN_SPEED` 0x96, `MAGOO_STRIDE_SPEED` 0x78, `*_HIT_MIN_SPEED`, `DRIVER_OUT_STEP` 0x64, `WHEEL_RISE_MAX` 0x240, `*_BOUNCE_MIN` (a landing's speed), `CMO_UNEVEN` 0x2711 |
| accelerations, speed a frame (/ N²) | 62740's `SPEED_TO_LIMIT_STEP` 3, `SPEED_STOP_STEP` 8, `SPEED_OVER_GEARS_STEP` 6, `BUTTON_GEAR_STEP` 4, `ROLL_FRICTION_HEAVY` 3 / `_LIGHT` 1, `SLOPE_PUSH` -4.0 (over `*_SLOPE_DIV`); `*_BRAKE` (police, Sideswipe, Skyfall, Ramdozer 0x14, hotrod 0x10, Thunderfist 0xC, Backlash 8, driver 6); the gear tables' accelerations; `CMO_ACCEL` 8, `BOOST_ACCEL` 0x28, `BOOST_SLOW` 0x19, `CHOPPER_ACCEL` 2, `CHOPPER_SLOW_DOWN` 4; `GRAVITY` (D_803EBBF4: the level's gravity times `*_GRAVITY`, 2.0 or 4.0) |
| turns, angle a frame (/ N) | `*_TURN_RATE` (police 0x2EE0, Sideswipe 0x4650, Backlash 0x2328 / 0xC80, Skyfall 0x3A98 / 0x7D0, Ramdozer 0x2328 / 0x7D0, hotrod 0x1770, Thunderfist 0x59D8, driver 0x2328), the steering rates (`*_STEER_DIV` and `_AIR` from the speed, `*_STEER` fixed), `*_WALL_TURN`, `CHOPPER_HOVER_TURN` 4 |
| frame counts (x N) | `*_HIT_FRAMES` 5, `RAMS_RELOAD_FRAMES` 10, `RAMS_SHAKE_FRAMES` 10, `TURN_SOUND_FRAMES` 7, `RATE_OVERRIDE_FRAMES` 10, `EFFECT_PIECE_FRAMES` 60, `CMO_COUNTDOWN_DELAY` 5 and `_FRAMES` 0x12, `SHUTTLE_EFFECT_FRAMES` 4, `POLICE_SIREN_STEPS` (two frames a light), the sparks every other frame, the idles' chance a frame (`*_IDLE_CHANCE`), `LAND_BOUNCE_DIV` |
| animation a frame (/ N) | `pad_lever`'s steps (5, back 5 or 10), `pad_lift`'s `LIFT_STEP` 0.05 and `LIFT_BACK` 0.03, `BED_STEP` 0.05, the spin divisors (`WHEEL_SPIN_DIV`, `MAGOO_LEG_SPEED_DIV`, `DRIVER_LEG_SPEED_DIV`, `SKYFALL_WHEEL_SPIN_DIV`, `CMO_SHAFT_SPIN`) |
| not time | the scales, the wheels' spans, the distances (`DRIVER_SPOT_STEP`, `DRIVER_BARGE_LIFT`, `CHOPPER_SLOW_DIST`, `CHOPPER_LAND_HEIGHT`, the sound ranges), `CAMERA_MARGIN`, `TURN_SNAP`, `MAGOO_FACING`, `WHEEL_LIFT`, `WHEEL_LAND_SLACK` |

## Other versions

`PORT_VERSION` is `us.v11` (the default), `us.v10`, `jp` (Blastdozer) or
`eu` (the PAL version, "eu" below).
`blastcorps/` and the translator's output hold one version at a time, so
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
- **They are native now**, as Rare's engine is ("Replacing the engine"):
  `port/engine/jp_<object>.c`, under `#ifdef VERSION_JP`, each written from
  jp's asm (the US C of the same function as the guide) and charged by its
  blocks, and Rare's `func_802BA3E8_jp` in 75490.c.  The translation above
  is the check build's only.  What IDO's code needed of the native code:
  - its prototype is the US version's C definition's (the types the US
    port's LP64 build passes), the result where jp's callers use it
    (`func_801F7410` returns the text it made);
  - IDO's float to unsigned conversion is `IDO_CVT_U_S` (engine.h), its
    checked `div` `ENGINE_DIV`, its signed division by 2^n a fixup block
    charged only for a negative dividend;
  - `sprintf` is `engine_sprintf`: the translation's call cost the C
    nothing (`recomp_extern_sprintf`), and the 32-bit build's own
    `n64_sprintf` is counted as the game's C;
  - a host helper returns its results (`engine_ido_cvt_u_s` a u64): a host
    store through a pointer into the N64 side's locals is in the host's
    order, which BEPass reads swapped;
  - `ENGINE_BLK` names a version's own function's blocks by its address and
    suffix (`802BA40C_jp`), and where a jp function is longer than us.v11's
    and its blocks' names reach the next function's, by both
    (`801F7428_801F6F18`; `translate.py` leaves the bare name undefined).
  The register leftovers and the dead stack of the IDO code aren't
  mirrored: its callers are the C, which reads neither, and long jp runs
  (`PORT_AUTOSTART` 0 to 3, 12,000 frames each) give the same
  `PORT_ICOUNT_LOG`, screenshots, saves and sound as the translated build.
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
halves (`asm2x86.py`, `HALF_FILES`).  Two more reads of the IDO code and
of Rare's jp-only code are sites of jp's own (`native_sites.txt` lines
with a third field, `jp`: a version's asm at its own offsets):
`func_8026BCE0` takes the fade's low byte as the byte after the `s16`
`D_8036BB0C` (the name entry's title came out differently), and
`func_802BA3E8_jp` reads (level, value) byte pairs that the asm data
declares as halves (`D_8030606E_jp`; Simian Acres then played
differently).  Found with `build_cmp.py rdram --native` and `itrace`
(skipping `func_802A794C`'s byte copy of words, where the registers hold
bytes in their memory's order).  With them the native-endian builds play
as the big-endian one, the quick tier's hashes but for the carrier on the
globe.  The type inventory and the islands' layouts are still us.v11's;
the runs are what checks jp.

The other variants needed the IDO code's calls out of it looked at (the
translated code's, which the check build still has; the native code does
the same in C):

- **The movable builds**: `func_801F57B0` starts the pak thread with
  `func_801F58E8`'s address, which only the asm takes (a `lui`/`addiu`
  pair), so port-arena didn't see a function used as a value and the
  host's `port_fn` didn't know it.  `gen_glue.py` writes `fn_values.txt`,
  every name the translated code takes the address of, and port-arena
  makes those of them that are functions values (`-port-arena-fn-values`).
- **The LP64 builds**: the IDO code passes its 32-bit display list slot to
  C that takes `Gfx **` (`func_80259BD4`, `func_80259C24`,
  `func_8025E2CC`), and the extern gives the C a native slot (below the
  N64 stack pointer, where the movable build's C can reach it) and copies
  the pointer back; and the pointer arrays it reads by words
  (`D_80208358`/`68`, `D_80208378`, `D_802FF188`, `D_80358050`,
  `D_80365348`) are `PTR32` (the attract mode's Japanese text was missing,
  its scroller divided by zero at frame 7,250).  The native pointers it
  reads or writes whole (`D_802158A0` ...) are right as they are: their
  low half is the N64's word on a little-endian host.
- **Everywhere**: `sprintf` and `bcopy` from the asm went to the host's
  libc, with N64 addresses (the movable build's are in the arena) and
  `sprintf` with four arguments of the five `"%d MINUTE%c %d SECONDS"`
  has; both externs are written by hand now (`gen_glue.py`, `BY_HAND`),
  `sprintf` taking its arguments by the format, the rest off the N64
  stack, and formatting apart as `n64_sprintf` does.

## Source changes for the port

Guarded with `#ifdef TARGET_PC`; the N64 build still matches.

- `hd_code/26570.c`: `func_8026BBD0` is `void`, but its callers use the
  result, which on the N64 is `v0` left over from `func_8026BCE0`; the port
  returns that.  Two string copies whose unsequenced `a[i] = b[i++]` IDO
  evaluates with the old index.
- `hd_code/168B0.c`: the same unsequenced copy.
- `hd_code/2E490.c`: `func_80272C5C` tells the host which textures an
  icon's picture is (`port_icon_texture`, "Model icons").
- `hd_code/37530.c`: the vehicles' tyre marks (`func_8027C4C8`) are
  drawn the port's way.  The marks are a ring of 80 entries (a pair of
  points across each of two tracks); the original packs each run's
  vertices into the frame's buffer, track A's then track B's, so the
  vertex in a given place changes whenever the ring's tail moves on or a
  run grows, and `--interpolate`, which pairs vertex loads by their place
  ("`--interpolate`"), blended marks with other marks, track B with
  track A: over 12,000 retraces of the attract mode (OpenGL), 9,804
  vertices in 215 frames, the marks sliding or stretching for an image.
  Its strips also skip two quads past track A's last (`k += 4`, where
  the gap is one) unless that falls at the end of a load of 16, whose
  next load starts back at track B's first pair: track B's first quad
  came and went as the loads shifted.  The port puts each entry's four
  vertices at its own place (`unk1900[0x70 + 4 * entry]`), loads them
  into slots 0-3 or 4-7 by turns and joins them to the entry before, and
  leaves out the cull box: 4 vertices in one frame are paired with other
  points over the same run (an entry that a strip's break and a commit in
  one frame make the head and then draw, while the full ring hands it
  the tail's place), and every quad is drawn.  The game's loops poll
  (`__port_poll`, "Memory model"), so a different number of iterations
  moves `--deterministic`'s clock: the sound and the screenshots changed
  (the references were updated), the gameplay digests didn't.
- `hd_front_end/196F0.c`: `func_80200714`'s texel reads and writes go
  through `IMG_RD`/`IMG_WR`, byte-swapping only in the native-endian build
  (`TARGET_PC && PORT_NATIVE_ENDIAN`; plain accesses otherwise, so IDO's
  output is unchanged).
- `hd_code/90390.c`: `ll.c`'s `__ll_*` aren't built (clang does 64-bit
  arithmetic itself).
- The SDK's graphics utilities, sines and pak CRC aren't built at all:
  `port/src/gu.c` is the port's own (docs/DISTRIBUTION.md, "Replacing the
  SDK parts"), checked against them by `port/tools/sdk_check/sdk_check.py`
  (sinf and fcos over every float, the rest on random arguments and on
  what a `-DPORT_SDK_TRACE=ON` build logs to `$PORT_SDK_TRACE`).
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
translator's output hold one version at a time, and CMake refuses a mismatch).
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
- **Timing.**  The retraces above are the movie's, its lag frames
  included.  `PORT_REPLAY_TIMING=free` gives each frame the port's own
  instead (two in a level: no lag), which is how the game plays by
  default; the game's mid-frame reads of the counts and the audio answers
  stay the movie's ("Lag frames" has why and what it measured).
  `test.py tas` replays with `free` by default, `--timing movie` the
  movie's way, `both` both.
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
starts (`rdram_N.bin`, in the build's byte order, with `rdram_N.widths` in
a `PORT_ACCESS_PROFILE` build), and `TAS_DUMP=N,...` makes `m64p_tas`
write the same at its Nth read, to compare the two.  Two builds of the
port replay the movie in step with `PORT_COUNT_PER_OP=0` (the reads and
retraces are the log's either way), so `PORT_DUMP`, `PORT_TRACE` and
`PORT_ITRACE` compare them as in "Comparing builds"; those count the
port's controller polls, about a hundred fewer than the log's reads by
the levels (`PORT_PACE` lists the polls with their modes).

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
differs (376 retraces given anyway).  Since the CPU model is off by
default no retrace is given anyway, and with the port's own frame timing
(`PORT_REPLAY_TIMING=free`, no lag frames) the replay is as exact: every
read matched, none skipped, no mode forced, 57 platinum, the same save
and the same gameplay digest ("Lag frames").

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

- Boots, runs every thread, and plays the Rare logo, the title screen and
  the attract mode (the story sequence and the demo levels) for as long as
  it is left running; with Start/A it goes through the save-erase prompt,
  the name entry, the world map and into Simian Acres, which plays (the
  bulldozer, the carrier, the pause menu).
- With the CPU model on (`--cpu-model n64`), pacing matches mupen64plus to
  a few percent in every mode measured ("Timing"); by default the game's
  work takes no time and no frame lags ("Lag frames").  The music and sound
  effects play ("Audio").
- `--interpolate` shows gameplay at 60 frames a second, or at a faster
  display's rate (`--display-hz`), with either renderer ("Frame rate"),
  without changing what the game does.
- Known problems: with `--cpu-model n64`, the CPU's time is a model
  (instruction counts, a scale for the C, fixed costs for libultra), and
  the reference is mupen64plus, not
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
- **Real call states** for the translator's test (the check of the native
  engine).  Recording `ctx` and
  memory at each translated function's entry during play would replace the
  random registers, and reach the 30% of blocks the random states don't.
- **Toward native code**: the 64-bit build is done ("The 64-bit build"),
  native-endian memory ("The native-endian build") and the LP64 build ("The
  LP64 build").  Next are the fixed addresses: the game's C and data should
  work at any base, which arm64 macOS and a WebAssembly build (both wanted
  eventually) need; the movable build ("Movable memory") is that, and the
  WebAssembly build runs on it in node and in a browser ("WebAssembly").  The
  32-bit build still has the out-of-bounds miscompiles the 64-bit one
  avoids; typing those arrays fixes both.
- **Readable C**: done ("Replacing the engine").  Every function of Rare's
  engine, and jp's IDO asm, is hand-written C in `port/engine`, and the
  default build links no translated code.

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
