# Blast Corps

- This repo contains a decompilation, and a PC port in progress, of Blast Corps `(Japan)`, `(USA)`, `(USA) (Rev 1)` and `(Europe) (En,De)`.
- Naming and documentation of the source code and data structures are in progress.

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
git clone https://github.com/Doridian/blastcorps.git --recursive
```

If you cloned it without `--recursive`, you can initialize the submodule later.

```
git submodule init
git submodule update
```

# The PC port

The port runs the decompiled C natively and Rare's handwritten engine code
through a mechanical translation to C; it needs your own copy of the ROM
(`us.v11` by default; `us.v10` and `jp`, Blastdozer, too).  It is built from the decompilation, so
the steps are: set up the tools, build the decompilation once, then the port.
[docs/PORT.md](docs/PORT.md) has the details.  Linux on x86-64 is what it's
built and tested on; the 64-bit build also cross-compiles for AArch64 Linux
(`port/tools/cross-aarch64.cmake`, PORT.md "Other hosts"), and macOS on
Apple silicon is prepared but untested ([macOS](#macos) below).

## Requirements

* Everything the decompilation needs (see [Build](#build) below): the
  `mips-linux-gnu-` binutils, Python 3 and the venv.
* clang and LLVM with its development headers (`llvm-config`, `opt`): the
  port's build runs an LLVM pass plugin over the game's C.
* CMake and Ninja, SDL2, and libepoxy for the OpenGL renderer (without it only
  the software renderer is built).

The default build is a 32-bit program and needs the 32-bit (multilib) libc,
SDL2 and libepoxy; the 64-bit build below needs only the ordinary 64-bit ones,
and is the one to use if you don't have multilib installed.

## Build it

The quick way, from the repo's root, with your ROM wherever it is (`.z64`,
`.v64` or `.n64`; us.v11, us.v10 or jp):

```
port/build.py path/to/rom.z64                  # the port for this machine: build/port-<version>
port/build.py --wasm path/to/rom.z64           # the browser page: build/web-<version>
port/build.py --wasm --serve path/to/rom.z64   # ... and serve it on http://localhost:8000
```

It identifies the ROM by its sha1. It sets up the submodules and the venv
if they aren't there yet, builds the decompilation and the translated engine
for that version, then the port. That's about a minute on a fast machine.
Run it again after a `git pull` and it redoes only what changed.
`--wasm` needs emsdk: `--emsdk DIR`, `$EMSDK`, emcmake on the `PATH`, or
`--install-emsdk` to fetch it into `.emsdk/`.  `--variant 32|lp64|movable|native`
builds the other native variants, and `--node` the headless WebAssembly
build; `port/build.py --help` has the rest.  Building another version
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
make -C tools/recomp                       # translate the handwritten code

cmake -S port -B build/port64 -G Ninja -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_ASM_COMPILER=clang -DPORT_64BIT=ON
cmake --build build/port64
```

Leave out `-DPORT_64BIT=ON` for the 32-bit build.  Other variants, all
playing the same game: `-DPORT_NATIVE_ENDIAN=ON` (game memory in the host's
byte order) and `-DPORT_LP64=ON` (the game's C as an ordinary 64-bit
program); see docs/PORT.md.

For WebAssembly (a page that plays in a browser, or headless under node),
install [emsdk](https://github.com/emscripten-core/emsdk) (`./emsdk install
latest && ./emsdk activate latest`), and after the steps above but the
last two, in its environment (`source emsdk_env.sh`; the system's clang
and LLVM still compile the game's C, since they load the pass plugin):

```
emcmake cmake -S port -B build/wasm-web -G Ninja -DPORT_WASM_TARGET=web
cmake --build build/wasm-web
cd build/wasm-web && python3 -m http.server 8000   # then open http://localhost:8000/blastcorps.html
```

The page asks for your ROM once and keeps it, and the save, in the
browser's IndexedDB; `?args=--widescreen` and the like pass options.
It keeps to the display's frames, draws at most about 1.3 million pixels
(`?args=--scale%203` picks for itself), and when the GPU can't keep up
it draws smaller, when the CPU can't it drops `--interpolate`'s
in-between pictures (`?env=PORT_ADAPT=0` turns both off);
`?env=PORT_PERF=600` logs where the time goes to the browser's console
(docs/PORT.md, "Performance").
Without `-DPORT_WASM_TARGET=web` it builds `blastcorps.js` for node:
`node build/wasm/blastcorps.js --headless --frames 600 --screenshot shot
baserom.us.v11.z64` (docs/PORT.md, "WebAssembly").

For `us.v10` or `jp`, put `baserom.us.v10.z64` or `baserom.jp.z64` in the
root and run the same with `VERSION=us.v10` or `VERSION=jp`, `make -C
tools/recomp` included (after `make clean` and `make -C blastcorps clean`),
and configure a separate build directory with `-DPORT_VERSION=us.v10` or
`-DPORT_VERSION=jp`.  jp still has some of its compiled code as asm where it
differs from the US versions; the port translates that the same way as the
handwritten code (docs/PORT.md, "Other versions").

## Run it

```
build/port64/blastcorps baserom.us.v11.z64
```

The ROM argument may be left out: it defaults to `baserom.us.v11.z64` (the
us.v10 and jp builds' to `baserom.us.v10.z64` and `baserom.jp.z64`) in the
current directory.  The port reads everything else from the ROM, so the
binary can be run from anywhere.

The save (the 4 Kbit EEPROM) goes to `blastcorps.eep` in the current
directory, or wherever `--save PATH` says.

| Key                 | N64            |
| ---                 | ---            |
| arrows or WASD      | stick          |
| X, C                | A, B           |
| Z                   | Z              |
| Enter               | Start          |
| Q, E                | L, R           |
| I, J, K, L          | C buttons      |
| T, F, G, H          | D-pad          |

An SDL game controller works too.  Some options (`--help` lists them all):

| option                       | what                                                      |
| ---                          | ---                                                       |
| `--renderer gl` / `sw`       | OpenGL (the default with a window) or the software renderer |
| `--scale N`                  | OpenGL: render at 320x240 times N (default: the window's size) |
| `--filter n64` / `bilinear` / `point` | texture filtering (default: the N64's 3-point filter) |
| `--interpolate`              | gameplay at 60 frames a second (either renderer), with in-between images drawn between the game's frames: 3D, texture and fill rectangles (docs/PORT.md, "Frame rate") |
| `--display-hz N` / `auto`    | with `--interpolate`: make in-between images for an N Hz display (e.g. 120, 144; `auto`: the display's) and show them between retraces (not with `--deterministic`) |
| `--widescreen`, `--aspect W:H` / `window` | show the 3D world 16:9, W:H (up to 32:9) or as wide as the window (default: the N64's 4:3); the 2D stays 4:3 in the middle |
| `--no-audio`, `--wav PATH`   | no sound, or everything the game plays to a file          |
| `--headless`, `--frames N`, `--screenshot PREFIX` | run without a window (with the software renderer unless `--renderer gl`), for N frames, saving the last frame as `PREFIXnnnnn.bmp` |

## Test it

```
ctest --test-dir build/port64 -L quick -V     # a minute: deterministic runs against committed hashes (us.v10)
ctest --test-dir build/port64 -L tas -V       # the TAS replay (us.v10 builds, 10-20 minutes)
ctest --test-dir build/port64 -L recomp -V    # the translated engine's differential test
port/tools/test.py variants                   # build the standard variants into build/test-*/ and check them all
port/tools/test.py variants --emsdk DIR      # ... and the WebAssembly build under node, which must equal mn32's
```

These are `port/tools/test.py`'s tiers; docs/PORT.md, "Testing the port",
says what each compares.

## macOS

**Untested**: the build is prepared for macOS on Apple silicon and checked
from Linux as far as that goes (docs/PORT.md, "macOS"), but no one has run
it on a Mac yet.  Reports welcome.

macOS takes the movable LP64 build (`-DPORT_LP64=ON -DPORT_MOVABLE=ON`),
built with Homebrew's LLVM: Apple's clang can't load the port's LLVM plugin.

```
brew install llvm sdl2 libepoxy pkgconf cmake ninja python
port/tools/macos_build.sh build/port-macos -DPORT_VERSION=us.v11
build/port-macos/blastcorps baserom.us.v11.z64
```

`macos_build.sh` runs CMake with `$(brew --prefix llvm)/bin/clang` as the
compiler; the same by hand is

```
LLVM=$(brew --prefix llvm)
cmake -S port -B build/port-macos -G Ninja -DCMAKE_C_COMPILER=$LLVM/bin/clang \
      -DCMAKE_CXX_COMPILER=$LLVM/bin/clang++ -DCMAKE_ASM_COMPILER=$LLVM/bin/clang \
      -DPORT_LP64=ON -DPORT_MOVABLE=ON
cmake --build build/port-macos
```

The port is built from the decompilation's stage 2 and the translated
engine, and those need the `mips-linux-gnu-` binutils and the IDO
recompilation this repo has for Linux.  The simplest is to run the steps up
to `make -C tools/recomp` on Linux (or in a Linux container) and copy
`blastcorps/build`, `blastcorps/asm`, `blastcorps/assets`,
`blastcorps/.version` and `build/blastcorps.<version>.map` over; the port's
own build reads the N64 ELFs with `llvm-nm` when there is no MIPS `nm`.

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
  are (`blastcorps/include/2.0I`, `blastcorps/src/libultra`,
  `port/src/gu_extra.c`);
- the submodules and third-party libraries, which carry their own licenses.

docs/DISTRIBUTION.md has the details.
