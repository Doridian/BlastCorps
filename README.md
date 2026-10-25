# Blast Corps

- This repo contains a matching decompilation of Blast Corps `(Japan)`, `(USA)`, `(USA) (Rev 1)` and `(Europe) (En,De)`, and a working PC port built from it that plays all four.
- The port runs on:
  - **Linux** (x86-64 and arm64; also builds for i386, PowerPC, MIPS and ARM)
  - **Windows** (x86-64)
  - **macOS** (Apple silicon; builds, not yet tested on a Mac)
  - **the browser** (WebAssembly, at https://doridian.github.io/BlastCorps/)
- It plays the whole game, including a full replay of a TAS (below), and adds widescreen, high frame rates, high-resolution text, model icons, a free camera, and mouse and touch controls.  More enhancements are still being added.
- Naming and documentation of the source code and data structures are ongoing.

[![Blast Corps port: TAS run](https://img.youtube.com/vi/wQkqY01qK9I/hqdefault.jpg)](https://www.youtube.com/watch?v=wQkqY01qK9I)

The port playing through the TAS (tool-assisted speedrun) of WarHippy's
"all platinum medals" run, TASVideos submission
[#8170](https://tasvideos.org/8170S) (1:16:02, from power-on).  The movie is
WarHippy's; the port's replay of it is what [docs/PORT.md](docs/PORT.md)
checks against ("The TAS").

# Documentation

The WebAssembly build of the port runs in the browser at
https://doridian.github.io/BlastCorps/ (bring your own ROM; see
[Build it](#build-it) below for building it yourself).

| Page | What it covers |
| --- | --- |
| [docs/PORT.md](docs/PORT.md) | The PC port: building and running it, the platform layer, graphics, audio, timing, the TAS, testing |
| [docs/ASSETS.md](docs/ASSETS.md) | The ROM's assets, their formats, and how they are extracted and rebuilt |
| [docs/DECOMPILING.md](docs/DECOMPILING.md) | Notes on matching the decompiled code: IDO quirks, the permuter, versions |
| [docs/ROADMAP.md](docs/ROADMAP.md) | The plan from the decompilation to the port |
| [docs/DISTRIBUTION.md](docs/DISTRIBUTION.md) | What a built port contains, by origin, and what publishing it would take |
| [docs/FONTS.md](docs/FONTS.md) | How the game draws its text, its typefaces, and the port's high-resolution text |
| [docs/LAYOUT.md](docs/LAYOUT.md) | What depends on the N64's memory layout |
| [docs/TYPES.md](docs/TYPES.md) | The game's structures and the memory inventory |

It uses the following ROMs:

| no-intro                       | Location             | sha1                                       |
| ---                            | ---                  | ---                                        |
| `Blast Corps (USA)`            | `baserom.us.v10.z64` | `185a6ef7ba1adb243278062c81a7d4e119bda58c` |
| `Blast Corps (USA) (Rev 1)`    | `baserom.us.v11.z64` | `483f7161aea39de8b45c9fbc70a2c3883c4dea8c` |
| `Blastdozer (Japan)`           | `baserom.jp.z64`     | `b147fdbeb661c89107c440b00dc4810508f58636` |
| `Blast Corps (Europe) (En,De)` | `baserom.eu.z64`     | `460212600f8b9f0da95219c4c7330f2e626d9a7e` |

This repo does not include all assets necessary for compiling the ROMs.
A prior copy of the game is required to extract the assets.

# Clone the repo

Clone recursivley to initialize the splat submodule.

```
git clone https://github.com/Doridian/BlastCorps.git --recursive
```

If you cloned it without `--recursive`, you can initialize the submodule later.

```
git submodule init
git submodule update
```

# The PC port

The port runs the decompiled C natively, and Rare's handwritten engine code
as C rewritten from it (`port/engine`); it needs your own copy of the ROM
(`us.v11` by default; `us.v10`, `jp`, Blastdozer, and `eu`, the PAL version, too).  It is built from the decompilation, so
the steps are: set up the tools, build the decompilation once, then the port.
[docs/PORT.md](docs/PORT.md) has the details.  Linux on x86-64 is what it's
built and tested on (clang or gcc, also for i386 and under the sanitizers);
it also builds and passes its tests as WebAssembly (under node, and as a
page), and cross-built with zig under qemu for PowerPC, MIPS, ARM and
AArch64 Linux (PORT.md "Other hosts", "Big-endian and 32-bit hosts");
Windows on x86-64 builds with MinGW-w64's clang and passes the quick tier
under wine ([Windows](#windows) below); macOS on Apple silicon is prepared
but untested ([macOS](#macos) below).

Prebuilt us.v11 ports are built by `.github/workflows/build.yml` on every
push, as the run's artifacts, and attached to the GitHub release of every
`v*` tag, each one file: an AppImage for Linux (x86-64, arm64), an `.exe`
for Windows (x86-64), an app in a `.dmg` for macOS (arm64), and a zip of
the browser page.  They are built from the snapshot and hold no ROM: started
without one, they ask for yours with the system's file dialog the first
time and remember it (docs/PORT.md, "Packages").

## Requirements

* Everything the decompilation needs (see [Build](#build) below): the
  `mips-linux-gnu-` binutils, Python 3 and the venv.
* A C compiler: clang or gcc (the port is C alone: no LLVM headers, no
  plugins).
* CMake and Ninja, SDL2, and libepoxy for the OpenGL renderer (without it only
  the software renderer is built).

## Build it

The quick way, from the repo's root, with your ROM wherever it is (`.z64`,
`.v64` or `.n64`; us.v11, us.v10, jp or eu):

```
port/build.py path/to/rom.z64                  # the port for this machine: build/port-<version>
port/build.py --wasm path/to/rom.z64           # the browser page: build/web-<version>
port/build.py --wasm --serve path/to/rom.z64   # ... and serve it on http://localhost:8000
```

With no ROM argument, `port/build.py` builds the us.v11 port (`--wasm`: the
browser page) from the repository alone (`port/snapshot/us.v11`, docs/PORT.md
"The snapshot"), with no decompilation and so no `mips-linux-gnu-` binutils:
no ROM is needed to build it, only to play it (the page asks for it).

It identifies the ROM by its sha1. It sets up the submodules and the venv
if they aren't there yet, builds the decompilation for that version, then
the port. That's about a minute on a fast machine.
Run it again after a `git pull` and it redoes only what changed.
`--wasm` needs emsdk: `--emsdk DIR`, `$EMSDK`, emcmake on the `PATH`, or
`--install-emsdk` to fetch it into `.emsdk/`; `--node` builds the headless
WebAssembly build; `port/build.py --help` has the rest.  Building another version
replaces the decompilation's output, but not the port builds already made.

The steps it runs, by hand: put `baserom.us.v11.z64` in the repo's root,
then, from the root:

```
python3 -m venv .env                       # once (see "Set up Python for splat")
. .env/bin/activate
pip install -r tools/splat/requirements.txt

make VERSION=us.v11 extract
make VERSION=us.v11
make VERSION=us.v11 decompress
make VERSION=us.v11 -C blastcorps extract
make -j VERSION=us.v11 -C blastcorps       # the decompilation, sha1-checked
make VERSION=us.v11 -C blastcorps compress
make VERSION=us.v11                        # the ROM's layout the port reads

cmake -S port -B build/port -G Ninja -DCMAKE_C_COMPILER=clang
cmake --build build/port
```

That is the whole port: one build, ordinary C in the host's byte order and
pointer size (gcc works as well as clang; `-DPORT_SANITIZE=address,undefined`
instruments it; `-DCMAKE_C_FLAGS=-m32` makes an i386 one, which needs the
multilib libc, SDL2 and libepoxy).  It plays the same game, exactly, on
every host and compiler it builds with (docs/PORT.md, "The build").

For WebAssembly (a page that plays in a browser, or headless under node),
install [emsdk](https://github.com/emscripten-core/emsdk) (`./emsdk install
latest && ./emsdk activate latest`), and after the steps above but the
last two, in its environment (`source emsdk_env.sh`; emcc compiles the
whole port):

```
emcmake cmake -S port -B build/wasm-web -G Ninja -DPORT_WASM_TARGET=web
cmake --build build/wasm-web
cd build/wasm-web && python3 -m http.server 8000   # then open http://localhost:8000/blastcorps.html
```

The page asks for your ROM once and keeps it, and the save, in the
browser's IndexedDB; `?args=--aspect%204:3` and the like pass options.
The picture fills the page and takes its shape as it is resized (and the
screen's, with the Full screen button), from 4:3 up to 32:9, with the
HUD at its sides; a page narrower than 4:3 (a phone held upright) gets
the 4:3 picture with bars above and below.
It keeps to the display's frames, draws at most about 1.3 million pixels
(`?args=--scale%203` picks for itself), and when the GPU can't keep up
it draws smaller, when the CPU can't it drops `--interpolate`'s
in-between pictures (`?env=PORT_ADAPT=0` turns both off), and when a
frame's work runs over it shows each picture a frame later rather than
late (`?env=PORT_QUEUE=0` turns that off, `2` keeps it on);
`?env=PORT_PERF=600` logs where the time goes to the browser's console
(docs/PORT.md, "Performance").
Without `-DPORT_WASM_TARGET=web` it builds `blastcorps.js` for node:
`node build/wasm-node/blastcorps.js --headless --frames 600 --screenshot shot
baserom.us.v11.z64` (docs/PORT.md, "WebAssembly").

For `us.v10`, `jp` or `eu`, put `baserom.<version>.z64` in the root and run
the same with `VERSION=<version>` (after
`make clean` and `make -C blastcorps clean`), and configure a separate build
directory with `-DPORT_VERSION=<version>`.  jp still has some of its
compiled code as asm where it differs from the US versions; the port has
that as C too, like the handwritten engine.  eu is PAL: it runs at 50
retraces a second, as on a PAL console (docs/PORT.md, "Other versions").

## Run it

```
build/port-us.v11/blastcorps baserom.us.v11.z64      # port/build.py's; build/port/ by hand
```

The window's title shows the frame rate once a second: the pictures shown
and, with `--interpolate`, how many of them were the game's own frames, e.g.
"60 fps (game 30)".  In the browser, the page's title shows the same.

The ROM argument may be left out: it defaults to `baserom.us.v11.z64` (the
other versions' builds' to `baserom.<version>.z64`) in the
current directory.  The port reads everything else from the ROM, so the
binary can be run from anywhere.  It must be the exact ROM the port was
built for (its sha1, above, is checked at startup).  The executable still
carries the game's initialized data (the ROM's, about 260 K), so don't
publish a binary (docs/PORT.md, "The build"; docs/DISTRIBUTION.md).

The save (the 4 Kbit EEPROM) goes to `blastcorps.eep` in the current
directory, or wherever `--save PATH` says.

The keys follow what's on the screen.  Driving:

| Key                  | in a level                                              |
| ---                  | ---                                                     |
| W, S (or ↑, ↓)       | accelerate, brake and reverse (on foot: walk)           |
| A, D (or ←, →)       | steer                                                   |
| Space                | brake; held when stopped, get out                       |
| Shift                | the vehicle's special (R: rams, boost, missiles, roll, jets, siren) |
| Q, E                 | turn the camera                                         |
| R, F, mouse wheel    | zoom the camera in and out                              |
| Escape, Enter        | pause (Start)                                           |

W and S are the A and B buttons, which is how the game drives in its
default control mode.  Where it steers by the stick's direction instead
(on foot, the Thunderfist, the Cyclone Suit, the J-Bomb, and any vehicle
switched to "speed on 3D stick" in the pause menu), WASD is the stick.

In the menus, on the world map and in the pause menu, the arrows or WASD
move, Enter or Space selects (A), and Escape or Backspace goes back (B).
The mouse works there too.  Pointing at a menu entry highlights it, a
click takes it, and the right button goes back.  On the world map, a
click on a level picks it and a click on the picked one goes in.
Dragging turns the globe, and it keeps the view until you pick a level
or use the keys.  A finger on a touchscreen does the same, in the
browser too.

The N64's own layout still works everywhere: X, C and Z are A, B and Z,
and I, J, K, L the C buttons.  An SDL game controller works too.

On the name entry the keyboard types the name: a letter (or 1-4, `/`,
`.`, the wheel's other characters) turns the wheel to it and takes it, as
fast as you type.  Backspace deletes the last one, Escape is B (back
out), and Enter confirms.  With the mouse, a click on a letter of the
wheel types it and a click on the name in the middle confirms.  The
letter keys don't act as buttons there; the arrows and a controller still
turn the wheel as on the N64.

An SDL game controller works too.  With a window, the port's improvements
are on by default: in-between frames at the display's rate, the text from
a font and the icons' models (each has a `--no-` option to turn it off);
headless runs and the compared ones (`--deterministic`, `--replay`) have
them only when asked for.  Some options (`--help` lists them all):

| option                       | what                                                      |
| ---                          | ---                                                       |
| `--renderer gl` / `sw`       | OpenGL (the default with a window) or the software renderer |
| `--scale N`                  | OpenGL: render at 320x240 times N (default: the window's size; `--max-pixels N` caps that) |
| `--filter n64` / `bilinear` / `point` | texture filtering (default: the N64's 3-point filter) |
| `--interpolate` / `--no-interpolate` | (default with a window) gameplay at 60 frames a second (either renderer), with in-between images drawn between the game's frames: 3D, texture and fill rectangles (docs/PORT.md, "Frame rate") |
| `--display-hz N` / `auto`    | with `--interpolate`: make in-between images for an N Hz display (e.g. 120, 144; `auto`, the default with a window: the display's) and show them between retraces (not with `--deterministic`) |
| `--aspect window` / `W:H`, `--widescreen` | the picture's shape: the window's, followed as it is resized (the default with a window), or a fixed W:H (`4:3`, the N64's, is the default headless and with `--deterministic` or `--replay`; `--widescreen` is `16:9`).  Wider than 4:3 (up to 32:9) shows more of the 3D world; narrower gets bars above and below |
| `--hud edges` / `centre`     | wider than 4:3: the levels' HUD (radar, money, counters, timer, TV) at the picture's sides (the default), or where the game puts it, in the 4:3 middle; menus, panels and full-screen pictures stay centred either way |
| `--hd-text [FONT]` / `--no-hd-text` | (default with a window) OpenGL: the game's text drawn from a font at the internal resolution (built in: Stardos Stencil, SIL OFL; docs/FONTS.md) |
| `--free-camera` / `--no-free-camera` | (default with a window) in a level, dragging with the left mouse button and a controller's right stick turn the camera freely and tilt it up and down, where C-left/right turn it 45 degrees at a time (the stick's click is C-down there); `PORT_CAMERA_SENS` is the mouse's degrees a pixel (0.2) (docs/PORT.md, "Keyboard, mouse and touch") |
| `--model-icons` / `--no-model-icons` | (default with a window) the icons that are pictures of the game's models (the carrier, the goals, the vehicles) drawn as the models, and the world map's chopper as the one that flies in at a level's start, circling the selected level (docs/PORT.md, "Model icons") |
| `--load-waits n64`           | the N64's waits for its hardware back: the controllers' half second at power-on, the EEPROM's write cycles, the pak thread's retrace a command (default `off`; docs/PORT.md, "The front end's waits") |
| `--no-audio`, `--wav PATH`   | no sound, or everything the game plays to a file          |
| `--headless`, `--frames N`, `--screenshot PREFIX` | run without a window (with the software renderer unless `--renderer gl`), for N frames, saving the last frame as `PREFIXnnnnn.bmp` |

## Resource packs

The ROM's assets can be turned into editable files (textures as PNGs, levels
as YAML, the sound banks and sequences) and played from instead of the ROM:

```
port/make_pack.py baserom.us.v11.z64                  # blastcorps-us.v11-pack.zip
build/port-us.v11/blastcorps blastcorps-us.v11-pack.zip   # or --pack FILE, or the unzipped folder
```

A pack made from an unmodified ROM plays exactly as the ROM does.  Change a
texture's PNG (the same size, or 2, 3, 4... times it), a level's YAML or a
sound, zip it again (or run the folder), and the game shows it.  The page
takes a pack in its file picker like a ROM.  Everything in a pack comes from
your ROM: don't share it.  `--no-code` leaves out the game's code modules
too.  docs/ASSETS.md ("The pack") has the format and what can be edited how,
docs/PORT.md ("Resource packs") how the port reads it.

## Test it

```
ctest --test-dir build/port -L quick -V       # a minute: deterministic runs against committed hashes (us.v10)
ctest --test-dir build/port -L tas -V         # the TAS replay (us.v10 builds, 10-20 minutes)
ctest --test-dir build/port -L tas-prefix -V  # its first 3000 frames (what an emulated build runs)
ctest --test-dir build/port -L budget -V      # the memory the build takes
port/tools/test.py kinds                      # build clang, gcc, ASan+UBSan and i386 into build/test-*/ and check them all
port/tools/test.py kinds --only wasm --emsdk DIR   # the WebAssembly build under node: the same hashes
```

These are `port/tools/test.py`'s tiers; docs/PORT.md, "Testing the port",
says what each compares.

## macOS

**Untested**: the build is prepared for macOS on Apple silicon and checked
from Linux as far as that goes (docs/PORT.md, "macOS": every object
compiles for arm64-apple-macos and links, leaving only libSystem, SDL2 and
libepoxy names undefined), but no one has run it on a Mac yet.  Reports
welcome.

The simplest is the us.v11 port from the snapshot, which needs no
decompilation (and is what CI builds there):

```
brew install sdl2 libepoxy pkgconf cmake ninja python
port/build.py --release
build/port-us.v11-release/blastcorps baserom.us.v11.z64
```

Other versions take the same build with the system's clang:

```
brew install sdl2 libepoxy pkgconf cmake ninja python
port/tools/macos_build.sh build/port-macos -DPORT_VERSION=us.v11
build/port-macos/blastcorps baserom.us.v11.z64
```

`macos_build.sh` runs CMake with `cc` as the compiler (Homebrew's LLVM
works as well); the same by hand is

```
cmake -S port -B build/port-macos -G Ninja -DCMAKE_C_COMPILER=cc
cmake --build build/port-macos
```

The port is built from the decompilation's stage 2, which needs the
`mips-linux-gnu-` binutils and the IDO recompilation this repo has for
Linux.  The simplest is to run the steps up to `make VERSION=us.v11` on
Linux (or in a Linux container) and copy `blastcorps/build`,
`blastcorps/asm`, `blastcorps/assets`, `blastcorps/.version` and
`build/blastcorps.<version>.map` over; the port's own build reads the N64
ELFs with `llvm-nm` when there is no MIPS `nm`.

## Windows

Windows on x86-64 builds with MinGW-w64's clang, natively in MSYS2's CLANG64
environment (what CI does) or cross-compiled from Linux with
[llvm-mingw](https://github.com/mstorsjo/llvm-mingw).  The game's threads
are Windows fibers there (`port/host/fiber_win32.c`); the rest is the same
build.  In MSYS2's CLANG64 shell:

```
pacman -S mingw-w64-clang-x86_64-{clang,llvm-tools,cmake,ninja,pkgconf,python,SDL2,libepoxy}
python3 port/build.py --release
build/port-us.v11-release/blastcorps.exe baserom.us.v11.z64
```

(SDL2 and libepoxy are linked in, `PORT_STATIC`, the default on Windows:
the `.exe` is the one file).  From Linux, with
llvm-mingw and the MinGW builds of SDL2 and libepoxy found by
`PKG_CONFIG_LIBDIR`, it is the CMake build with a toolchain file naming
`x86_64-w64-mingw32-clang` and `CMAKE_SYSTEM_NAME Windows`;
`-DCMAKE_CROSSCOMPILING_EMULATOR=wine` runs `test.py quick` on it (the
software renderer's scenarios: SDL's offscreen driver has no OpenGL on
Windows).

# Build

## Requirements

* A MIPS cross-binutils, prefixed `mips-linux-gnu-` (`as`, `ld`, `objcopy`).
  Debian/Ubuntu ship it as `binutils-mips-linux-gnu`; elsewhere build binutils
  with `--target=mips-linux-gnu`.  Override the prefix with e.g.
  `make CROSS=mips64-elf-` if yours is named differently.
* Python 3 with splat's dependencies (see below), and `gzip`.

## Set up Python for splat

```
python3 -m venv .env
. .env/bin/activate
pip install -r tools/splat/requirements.txt
```

Keep the virtualenv active for every `make` below; the Makefiles invoke plain
`python3`.  Alternatively pass it explicitly: `make PYTHON=.env/bin/python ...`.

## Versions

`VERSION` selects the revision to work on and defaults to `us.v11`:

| `VERSION` | ROM                            |
| ---       | ---                            |
| `us.v10`  | `Blast Corps (USA)`            |
| `us.v11`  | `Blast Corps (USA) (Rev 1)`    |
| `jp`      | `Blastdozer (Japan)`           |
| `eu`      | `Blast Corps (Europe) (En,De)` |

`asm/` and `assets/` hold one version at a time, so run `make clean` (and
`make -C blastcorps clean`) before switching `VERSION`.  The build refuses to
run on a tree left over from a different version rather than producing a
mismatched ROM.

This is a two-stage build.  Stage 1 splits the ROM and links it back together;
stage 2 additionally disassembles and reassembles the three code modules.

## Stage 1

**Extract init, hd_code and hd_front_end code from ROM**
```
make VERSION=us.v11 extract
```
**Build ROM**
```
make VERSION=us.v11
```

The build ends with a `sha1sum --check` against `blastcorps.<VERSION>.sha1`.

## Stage 2 (Optional)

**Decompress hd_code and hd_front_end .text and .data sections**
```
make VERSION=us.v11 decompress
```
**Extract `init`, `hd_code` and `hd_front_end`:**
```
make VERSION=us.v11 -C blastcorps extract
```
**Compile ASM/C** (each module is checked against its own sha1)
```
make VERSION=us.v11 -C blastcorps
```
**Compress compiled code and replace**
```
make VERSION=us.v11 -C blastcorps compress
```
**(re)Build ROM**
```
make VERSION=us.v11
```

Going all the way round -- split, disassemble, reassemble, re-deflate, relink --
reproduces every one of the four ROMs byte for byte.

## Assets

`make extract` also turns the ROM's assets into editable files under
`assets/` (`tools/assets.py`, see [docs/ASSETS.md](docs/ASSETS.md)):

| directory          | what                                                         |
| ---                | ---                                                          |
| `assets/textures/` | the 4096-entry texture table: a PNG per texture, `textures.yaml` |
| `assets/levels/`   | the 60 level files as YAML (header, objects, collision)      |
| `assets/audio/`    | the music and sound effect banks (`.ctl`/`.tbl`), the 66 sequences (`.seq`) |
| `assets/images/`   | the three 320x240 background images (PNG)                    |
| `assets/gzip/`     | every other gzip member (models, display lists, ...), inflated |

It checks that they rebuild to the ROM's bytes, and `make` builds the ROM
from them: compressing them again and regenerating the texture and model
tables.  After an edit that changes an asset's size, everything after it
moves; rebuild with `SHIFT=1` (both stages, see CLAUDE.md) so the code
follows.

## Regenerating the splat configs

The top-level split and the stage-2 module splits are both generated rather than
hand-maintained, because the gzip members are self-delimiting and hand-written
offsets have been wrong before:

```
python3 tools/gen_build_yaml.py baserom.jp.z64 jp > blastcorps.jp.yaml
tools/regen_code_yaml.sh
```

`regen_code_yaml.sh` runs `gen_code_yaml.py` for hd_code and hd_front_end of
us.v11, and ports those configs (and init's) to the other versions through
`tools/vermap.py`; it needs all their decompressed module binaries in
`blastcorps/`.
It records where the data islands inside `.text` sit in each version.  Pass
`--c` and set `SYMS=1` to get the configs as committed (see below).

## Functions and symbols

Every function is its own file.  Stage 2 extraction writes
`blastcorps/asm/nonmatchings/<module>/<file>/<function>.s`, and
`blastcorps/src/<module>/<file>.c` pulls each one in with
`#pragma GLOBAL_ASM`; decompiling a function means replacing its pragma with C.
The same C files build all four versions (named after us.v11's objects), with
`#if VERSION_*` where a version differs.
Splat only creates those `.c` files when they are missing and never rewrites
them, so if the function boundaries change, delete the untouched stubs and
extract again.  The only code not in this form is the handwritten math code
between hd_code's data islands, which starts and ends at odd offsets and stays
plain `asm`.

Only recognised functions and variables have names.  Everything else keeps an
address-based default: `func_802447C0` for functions, `D_80307B50` for data.

The recognised names are libultra's.  `tools/libmatch.py` compiles nothing
itself; it compares the game against libultra objects built from
[decompals/ultralib](https://github.com/decompals/ultralib) with the same
IDO 5.3.  Relocated fields are masked, so a function matches only when every
other bit is identical.  The relocations of each match then name what it calls
and the globals it uses.  Blast Corps links an older libultra than ultralib
reproduces, so a few functions only match by similarity.  Each symbol is tagged
with how it was found:

| tag         | meaning                                                            |
| ---         | ---                                                                |
| `lib:exact` | the body matches a reference object (masked relocations aside)      |
| `lib:ref`   | a matched function calls or references it by this name             |
| `lib:fuzzy` | at least 85% similar to one reference and clearly closer than any other |
| `from:<m>`  | defined in module `<m>`; named here because this module calls it     |
| `gzip`      | Rare's copy of gzip 1.2.4's decompressor, named in `symbols_known.txt` |
| `called`    | unnamed, but called; listed so splat starts a function there        |

Each module is linked separately and carries its own copy of the libultra
functions it uses, so the same name can appear in several modules at different
addresses.  Functions whose bodies are identical under two names
(`alCSPPlay`/`alSeqpPlay`, `guFrustum`/`guPosition`, ...) stay unnamed.

To regenerate the symbol files:

```
tools/build_ultralib.sh /tmp/ultralib        # prints the object directories
python3 tools/gen_symbols.py us.v11 <those directories>
```

Names added by hand go above the marker line in a `symbol_addrs` file and
survive regeneration.  Code that is identified by hand and exists in more than
one place goes in `blastcorps/symbols_known.txt` instead, at its address in
us.v11's init: `gen_symbols.py` names every copy of it, in every module and
version, along with the globals each copy uses.  That is how Rare's gzip
decompressor (`inflate`, `huft_build`, `get_method`, ... and globals such as
`inptr`, `outcnt`, `cplens`) is named in both init and hd_code; the file lists
the evidence for each name.

## C tools

C tools from queueRAM's `blast_corps_docs`, `sm64tools` and other places.
They can be found in the `tools/src` subdirectory of this repo.  The build
doesn't use them (`tools/assets.py` and `tools/assetlib/` do the same in
Python, and more); they are kept for reference.

### Build the tools

```
meson build-tools
ninja -C build-tools
```

### Run the tools

```
./build-tools/tools/src/unblast        # decompress a single Blast block
./build-tools/tools/src/unblast_rom    # decompress every Blast block in a ROM
./build-tools/tools/src/gen_level_table
./build-tools/tools/src/gen_splat_yaml
```

# Credits

* retroplastic's [blastcorps](https://github.com/retroplastic/blastcorps)

  The repository this one started as a fork of: the splat-based two-stage
  build, the ROM and module configs, and the `gzip`/`rzip`/`blast` splat
  extensions.

* retroplastic's [splat](https://github.com/retroplastic/splat) fork

  The splat used as the `tools/splat` submodule (0.7.10 plus the `gzip` and
  `shebang` branches' changes).

* mkst's [blastcorps](https://github.com/mkst/blastcorps)

  Initial set up of splat and Makefile build this repo is based on.

* queueRAM's [blast_corps_docs](https://github.com/queueRAM/blast_corps_docs)

  The original repository this is based upon. The content can be found in `docs` and `tools`.

* queueRAM's [BlastCorpsEditor](https://github.com/queueRAM/BlastCorpsEditor)

  A C# level editor for blast corps.

* queueRAM's [sm64tools](https://github.com/queueRAM/sm64tools)

  A N64 rom manipulation tool silimar to splat, written in C.

* mkst's [gzip](https://github.com/mkst/gzip) branch

  Backport of the pre-1.5 bug behaviour of gzip to support the rare gzip format.

* ethteck's [splat](https://github.com/ethteck/splat)

  A binary splitting tool, used as subrepository in this project.

* [n64decomp](https://github.com/n64decomp)

  A collection of N64 decompilation projects.

* queueRAM's [Texture](https://github.com/queueRAM/Texture64)

  Can be used to view raw textures extracted from gzip. Works with mono.


# License

The project's own code is licensed under the GNU Affero General Public
License, version 3 or (at your option) any later version: see `LICENSE`.

That covers what this project wrote. It doesn't cover:
- the game, its ROM, or anything extracted or translated from it;
- the Nintendo SDK parts in the tree, which are decompiled or kept as they
  are (`blastcorps/include/2.0I`, `blastcorps/src/libultra`, and
  `port/tools/sdk_check/orig_rotate.c`, which only that test tool builds);
- the submodules and third-party libraries, which carry their own licenses;
- the bundled font `port/fonts/StardosStencil-Bold.ttf` (SIL Open Font
  License 1.1, `port/fonts/OFL.txt`) and `port/third_party/stb/stb_truetype.h`
  (public domain or MIT, in the file).

docs/DISTRIBUTION.md has the details.  The third-party notices for a built
port (SDL2, libepoxy, musl, Emscripten, stb, gzip, the font) are in
[NOTICES.md](NOTICES.md).
