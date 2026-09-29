# Decompiling notes

What has mattered for matching so far. The loop and tools are in CLAUDE.md.
All of this is IDO 5.3 at `-O1` unless it says otherwise.

## Locals and registers

- At `-O1` every local lives on the stack, so m2c's `spXX` variables are real
  locals. Declare them in order of descending stack offset: the first one
  declared gets the highest address.
- Unused locals still get a stack slot. If the frame is 4 or 8 bytes smaller
  than the original and every instruction is otherwise the same, add an unused
  `s32` where the gap is.
- Some temporaries go in `$s` registers with no stack slot: a value-producing
  `a ? 1 : 0`, `&&`/`||`, a MIN/ABS ternary, a chained assignment
  `a = b = f();`.
- `register` variables get `$s` registers in declaration order, and each still
  gets a stack slot where it's declared.

## Statement shape

- The comma operator changes scheduling. `a = 1, b = 2;` doesn't compile like
  two statements; a comma group of local assignments stores in reverse.
- Where an increment sits matters: `a[i] = b[i++]` vs `a[i++] = b[i]`, and
  `X++; if (X == Y)` vs `if (++X == Y)`.
- `for (;;) { if (x) return ...; p++; }` and `while (!x) p++;` differ. m2c
  writes loops as `goto`; rewrite them as `while`/`for`.
- m2c sometimes drops early returns, so check the function's size.
- An empty `if (x) {}` can force a reload IDO would otherwise skip.
- `if (x) return;` before the body, instead of wrapping the body in an `if`,
  can keep a pointer from being held in a register across the branch.
- For commutative adds and compares, IDO often evaluates the operands in the
  reverse of the order written. Try swapping them.
- Passing `x = y` directly as a call argument, rather than assigning first,
  changes register allocation.
- Things that matched: clamps as `(x + 31 >= 80) ? 79 : x + 31`, and
  `MAX(0.0, x)` as `(0.0 > x) ? 0.0 : x`.
- In a compare-chain `switch`, the source order of the case bodies sets their
  layout.

- A `sqrt.s` inline in one function while its neighbours `jal sqrtf`:
  `#pragma intrinsic(sqrtf)` before that function and `#pragma function(sqrtf)`
  after it.
- IDO drops a loop's first test when the counter starts at a known constant.
  A search loop that guards the first element and then does
  `++i < n && ...` matched as
  `if (a[0] != x) { do {} while (++i < n && a[i] != x); }`.
- GBI macro temporaries take stack slots in the order they appear. Locals
  declared between macros may need a nested `{ }` block to land in the right
  slots. `sp7C = gfx = (Gfx *)D_80358070;` right after bumping the allocator
  let the next macro reuse the register where other spellings reloaded it.
- A `sizeof` in a loop bound makes the compare unsigned (`sltiu`) even with
  an `s32` counter.
- For a multiply by a large constant stride, a pointer to a multi-dimensional
  array (`u16 (*p)[49][0x1A4]`) gives the direct multiply; `p + i * 0x5064`
  multiplies then shifts.

- `X += a; if (X >= b)` and `if ((X += a) >= b)` differ; so do `X--; if (!X)`
  and `if (--X == 0)`. A single-case `switch` can reproduce a reload after a
  branch. `x -= k` written inside a call argument can match where a separate
  statement doesn't.
- IDO only reuses a loaded value if no store (even to a local) comes between.
  Chained assignments store right to left: `a = b = x` stores `b` first; a
  u16 field stored then read back (`sh`, `lhu`) is `a.x = a.y = 20;`.
- Put the operand that must be evaluated first on the left: `sign * f(...)`,
  not `f(...) * sign`; `p->unk14 + (u8 *)p` rather than the other order.
- `D = !p;` gives `sltiu`; `p == NULL` and ternaries don't.
- A ternary assigned to a global becomes a store in each arm; one nested in
  another, or used as a call argument, goes in an `$s` register.
- `&a[3][i] + 2` and `&a[3][i + 2]` compile differently.
- `if (x > 0.0f) {} else { x = -x; }` is the abs pattern that reloads `x`.
- A constant index into an extern array (`D_X[7].f`) is `lui/addiu` plus an
  offset. If the asm instead loads `%lo(sym)` at a nonzero offset directly,
  those are separate scalar globals.
- `lw $at`/`sw` copies of an initialized local array or struct are a struct
  assignment from an extern: `sp34 = D_80208358;`.

## Types

- An argument that's `lw`'d from its stack slot and truncated at use is `s32`,
  not `s16`. `andi a0,a0,0xff` at entry means a `u8` parameter.
- A function that reads its own arguments back from their stack slots as
  bytes, or whose callers pass narrow values unconverted, is probably a K&R
  (non-prototype) definition. Declare it `s32 func();`.
- m2c shows a 64-bit global as two 32-bit ones (`D_X` and `D_X+4`). If the
  code uses `ld`/`sd` or 64-bit compares, it's one `s64`/`u64`. A
  `(x & 0) || ...` shape in m2c is a u64 mask test.
- `addiu a0,zero,0` for a high word, where plain 0 would be `or a0,zero,zero`,
  is the tell for a u64 argument.
- A `switch` on a u64 compiles to compare trees, not a jump table.
- Storing an `s8`-converted value into a u8 (`v.cn[i] = (s8)f`) produces the
  `sll`/`sra` pair.
- Converting a `u8` to float gives the unsigned-conversion fixup sequence; it
  doesn't mean the variable is `u32`. A float passed in an integer register is
  built with `lui`/`ori`, so it needs no `.rodata`.
- libultra was built with unsigned `char`; this build passes `-signed`. Read
  through `(u8 *)` where the asm uses `lbu`.

## Data

hd_code's and hd_front_end's `.data`/`.rodata` are split per object. A C file whose subsegment is `.rodata` in the yaml owns its `.rodata`:

- Write strings, float/double literals and `switch`es in C. The GLOBAL_ASM
  functions still in the file get their share put into their `.s` as
  `.rdata`/`.late_rodata` by `tools/split.py`, so only functions that are C
  must produce theirs. A constant used twice by one extern was probably two
  literals; IDO doesn't merge them.
- To switch a file: `tools/inline_rodata.py hd_code <file>` (turns
  `extern char/f32/f64` uses into literals), change its `rodata` line to
  `.rodata`, remove `blastcorps/asm` and re-extract.
- A file still on asm `rodata` must keep using externs: no string literals,
  no float or double constants that IDO puts in `.rodata` (floats with the low
  16 bits zero are fine: `lui`), no jump-table `switch`.
- A string or constant nobody references can be a folded-away assert:
  IDO still emits the literals of `if (sizeof(x) > 512) { printf(...); }`
  (hd_front_end/E7B0.c, "sizeof(playerInfo)<=512").
- Strings that asm `.data` points to (menu tables, local initialized
  aggregates) must keep their names: define them as
  `const char D_X[] = "...";` at the point in the file where they fall
  (const globals are early `.rodata`, in source order with the strings).
  Unreferenced zero bytes among the early `.rodata` can be emulated the same
  way, but first suspect an object boundary: the 12 bytes 7800.c had that
  way were the padding before 8380.c's, which jp showed. A zero slot in
  the late part is usually the padding before another object's `.rodata`
  (hd_front_end/9570.c's at `0x8020F088` was: C450.c starts there).
- `.data`: every C file owns its own; see below. A local
  initialized aggregate (`char *sp24[] = {...}`) is `.data` too, with its
  strings at that point in `.rodata`.
- A global defined in the same file is addressed differently from an extern
  one: neighbouring globals, or a u64's two halves, share one `lui`. Every C
  file already defines its own `.bss` (the `/* .bss, ... (tools/bss_c.py) */`
  block), so such a variable is usually there to use.

`.data` (us.v11): `tools/data_c.py <module> <object>` writes a C file's
definitions from its asm block (while the block is still `data` and
extracted; `--ref` takes a saved `.data.s` to redo one). What mattered:

- IDO lays `.data` out in definition order, not first use as with `.bss`,
  and aligns every variable to at least 4 (8 for doubles, 64-bit integers
  and the GBI unions `Gfx`, `Vtx`, `Mtx`), then pads the section to 16. So a
  label that isn't 4-aligned is a field of something, and consecutive `u8`
  globals are really one array or struct. Static locals come out where
  their function is.
- The definitions go after the `.bss` block: a `.data` initializer that
  names a `.bss` variable counts as a use and would reorder `.bss` if it came
  first. Code above the definitions keeps its `extern`s; defining a variable
  in the same file changed no code here.
- Use the types the code declares (from any C file of any module; the tool
  borrows the declaration and copies the typedef). Nonzero bytes in a
  struct's padding mean a missing field (23C20's `unk9`). A typed variable
  also shows where a block really starts: hd_front_end's 7800/9570 boundary
  was inside a texture a 9570 display list loads whole.
- A string that only a `.data` table points to was a literal in the
  initializer: IDO emits it into `.rodata` where the table is defined. With
  `--literals` the tool writes them that way and puts the definitions where
  the strings were (7800's table sits after its first function). Where the
  `.rodata` order doesn't allow it for every table, `--literals-in <var>`
  limits it (hd_front_end/1C40); the rest stay `const char` arrays, cast to
  the field's type. Making a field `char *` for this is only safe where the
  code never reads bytes through it (`lbu` becomes `lb`); cast at that use
  instead (26570's `func_8026F004`).
- Display lists in `.data` are gs* macros. Some hold the physical address
  of vertices in the same block (4B5E0, 4DA80): `STATIC_K0_TO_PHYS(x)`
  (`common.h`) is `(u32)(x) - K0BASE`, which the linker can relocate, so
  those move in a shifted build too. Vertices stored inside a struct typed
  as a `Gfx` array get their own `Vtx` field.
- An end pointer to the array right before a table (37530's
  `D_802FC360[10].unk4`) is `&D_802FC320[4]`.
- A name that only C uses and no object defines any more (a field of
  another file's variable, hd_code's text tables that hd_front_end points
  to) needs a line in `undefined_syms`.

`.bss` is per object as well; every C file defines its block's
variables, the handwritten objects get a `.bss.s`. What mattered:

- IDO puts uninitialized globals in `.bss`, not COMMON, in the order it
  first sees them, defined or used (an `extern` declaration doesn't count).
  Definitions after the code that uses them come out in the order of first
  use, so the block goes before the first function (or the first use in
  `.data` initializers, or the `#include "src/..."` of a libultra stub).
- Each variable is aligned by its size: 8 from 8 bytes up, else 4, 2, 1,
  whatever its type. A `.bss` section is padded to 16. So an unreferenced
  gap that IDO's alignment wouldn't leave needs a `u8 D_<addr>[n]` (the
  tool writes those), and an array of unknown size can come out smaller than
  its gap, with padding after it.
- `tools/bss_c.py <module> <object>` writes the definitions: declared types
  where the file (or a header) declares the name, `u8` arrays for the rest.
  A name inside another variable (a field another object uses as a symbol)
  isn't defined; `link_syms.py` makes it relative. To give a file a new
  variable, re-run it after `rm -rf asm` and an extract (the block's
  reference is `asm/bss/<module>/<object>.bss.s`), or edit the block by
  hand and run `bss_c.py --check`.
- Changing a declared type changes the layout: keep the size (or add the
  padding) so that `--check` and the sha1 stay happy.

Addresses reached through a `lui`/`addiu` pair that splat didn't match up
are symbolized by `tools/postsplit.py` when it can follow the pair; C that
names one splat never saw gets a line in
`undefined_syms.<module>.<VERSION>.txt` (including segment and ROM addresses
like `0x02000000` or `0x00487050`). Inside a module such a name is made
relative to the symbol below it at link time. Don't use constant-address
casts or struct-offset tricks for them: a cast doesn't move in a shifted
build.

## Display lists

- The game uses SGI's Fast3D GBI (2.0D-era; `include/2.0I/PR/gbi.h`, without
  `F3DEX_GBI`). Write display lists with the GBI macros. Each `Gfx *_g`
  temporary m2c shows is one macro call on `gfx++`.
- Where no named mode fits, use `gDPSetCombineLERP` for combiners and write
  raw tile/load sequences as `gDPSetTextureImage`/`gDPSetTile`/
  `gDPLoadBlock`/`gDPTileSync`.
- `x & 0x1FFFFFFF` is `K0_TO_PHYS(x)`; `x + 0x80000000` is
  `OS_K0_TO_PHYSICAL(x)`.
- Where the older gbi.h builds different words:
  - `gSPPerspNormalize` is its own command, `0xB4`. `common.h` fixes this.
  - Texture rectangles send s/t with `G_RDPHALF_2` and dsdx/dtdy with
    `G_RDPHALF_CONT`, and `gSPScisTextureRectangle` clamps without 2.0I's
    `s16` casts and sign tests. `common.h` redefines both.
- hd_front_end's `gDPSetPrimColor` with u8 struct fields ORs the colour
  word in a different order (`b | (r | g) | a`); `hd_front_end/9570.c` has a
  local variant.
- Segment 2 (`D_02000000`) holds a buffer with `Mtx[8]` at 0 and vertices from
  `0x1E00`; see `hd_code/30C70.c`.

## Handwritten code

Code that saves registers with `sd`/`ld`, uses `add`/`addi`, passes
arguments in `$v0`/`$v1`/`$t*`/`$s0` or has no normal prologue isn't IDO
output. `tools/gen_code_yaml.py` makes such code an `asm` subsegment, so it
never reaches a stub:

- any code subsegment that saves `$ra` with `sd` and never with `sw` (Rare's
  engine block, `0x56040`–`0x8F860` in us.v11 hd_code);
- a run of such sd-`$ra` functions inside otherwise-IDO code, split out at
  its first function and at the next IDO one (both must be 16-aligned; a
  leaf function at either end of the run stays with the C). hd_front_end's
  `0x1B100`–`0x1B730` in us.v11 is one;
- libultra's handwritten functions, by name (`HANDWRITTEN_LIBULTRA` in the
  generator), split out of whatever C they sat next to.

If you find more, extend the generator rather than hand-editing a config.

## More that mattered in the data-owning pass

- A string after a late constant in one `.rodata` block, or zero padding to
  16 after a jump table, marks an object boundary splat missed.
  `gen_code_yaml.py` finds these itself (`missed_boundaries()`), and takes
  `OBJECT_STARTS` (first functions by name) and `--split` for ones that
  don't show. `--join` keeps a found boundary unsplit (for a C file that
  was decompiled before it was found).
- One C stub has one optimisation level, and IDO -O3 inlines across the
  whole stub, so a stub holding several original objects needs a `.text`
  split per object. A C file also packs its data with no 16-byte gap
  between objects.
- Strings reached only from asm `.data`: define them as
  `const char D_X[] = "...";` at their place in the source (at the top if
  they come first). Const arrays and literals go in `.rodata` in source
  order, each 4-aligned. Unreferenced zero bytes can be emulated the same
  way. Asserts IDO folds away still emit their strings.
- IDO CSEs identical expressions containing a literal, so a constant used
  three times can still be one literal per use.
- Two different globals sharing one `lui` need to be fields of one struct
  defined in the file; separate definitions aren't enough.
- `X = f(); g(Y);` on one line lets the store sink into the jal's delay
  slot; try that first when only a store's position differs.
  `X = 0, f(Y);` puts the store in the call's delay slot.
- Operand order sets FP register allocation and the order of constants in
  `.late_rodata`.
- `D &= !f();` gives `sltiu`. `X = f() + X;` differs from `X += f();`. An
  `||` chain on a value in `$s0` is a `switch` with fall-through.
- Temp registers are assigned in source order even when scheduling moves
  the instruction. `volatile` can reproduce a reload IDO would fold; so can
  an empty `if (x == y) {} else`. `register f32` explains a value kept in
  `$f20` plus a spare stack slot.
- Empty cases still count toward IDO's jump-table decision: add
  `case 0: break;` to make a table start at 0. A u64 `switch` can make a
  jump table for dense small low-word cases.
- IDO forwards a just-stored global into a later load at -O1, so an index
  stored right before use shows as an unfolded address computation.
- libaudio here is old: its asserts are compiled out except in env.c, where
  they call `func_8029A7E4("\n--- ASSERTION FAULT - %s - %s, line %d\n\n",
  #EX, "env.c", __LINE__)`. At -O3 a static gets a custom calling
  convention, an inlined-everywhere static still leaves an empty stub, and
  an unused static is dropped (so reverb's `L_INC` is non-static). 2.0D's
  gu.h FTOFRAC8 is double where 2.0I's is float.

## From the last stragglers

- A dead `while (0) { stmt; }` right after a switch reproduces a `b` to the
  next instruction at the end of the last case (`if (0)`, an empty
  `while (0) {}` and case padding don't).
- `switch ((u32)x)` on an `s8` local gives `lbu` with no mask; `(u8)x` gives
  `lb` + `andi`.
- A single-case `switch` in place of an `if` stops IDO forwarding values
  just computed into a later call, so the call reloads its arguments.
- `if (x % 16)` keeps the signed-remainder sequence; `x % 16 != 0` becomes
  `& 15`.
- Register allocation depends on later code, so fix the diffs lowest in a
  function first.
- Two ROM-range sizes that share an address but are built separately come
  from two linker symbols at that address (`D_0048F970`, `D_0048F970_2`).
- Zero padding after an object that no C produces (0x40 bytes after
  osCreateMesgQueue) is a `bin` subsegment, passed to the generator with
  `--bin`.

## The permuter

For a draft that is only register allocation or scheduling away,
[decomp-permuter](https://github.com/simonlindholm/decomp-permuter)
(`tools/decomp-permuter`, a submodule) tries random rewrites and keeps the
ones that score better. It needs `toml` and `Levenshtein` in the venv
(`.env/bin/python -m pip install toml Levenshtein`).

```
tools/permute.sh <module> <function> [draft.c]
.env/bin/python tools/decomp-permuter/permuter.py -j24 --stop-on-zero permuter/nonmatchings/<function>
```

`permute.sh` builds `permuter/nonmatchings/<function>/` (ignored by git):

- `base.c`: the function's C file run through the preprocessor, with the
  function's `GLOBAL_ASM` replaced by `draft.c` (the definition plus any
  declarations or macros it needs; m2c's output when there's no draft).
  The function has to be `GLOBAL_ASM` still, for its `.s`. The other
  `GLOBAL_ASM` lines are dropped, and the permuter keeps only what the
  function uses.
- `target.o`: the function's `.s` assembled alone.
- `compile.sh`: the command make runs for that file (`make -n PERMUTER=1`
  selects the plain recipe), so per-file `-O2`/`-O3`/`-mips3` come along.

The permuter writes each improvement to `output-<score>-<n>/`; `diff` its
`source.c` against `base.c` and carry the change back into the real file by
hand. The score is only a guide: the build's sha1 and `tools/fdiff.py`
decide. Things to know:

- A draft's `extern` must agree with the file's own definitions (the `.bss`
  block defines most globals now). An address cast (`*(s32 *)D_X` on a
  `u8 D_X[4]`) is addressed differently from a real `s32 D_X`, so change
  the block's type instead (same size; `bss_c.py --check` confirms it).
- The target's relocations name the symbols splat made, so a field
  reached as `D_80367D73` in the `.s` is `D_80367D60[i].unk13` in C. That
  shows up as a constant difference in the score; rewrite the `.s`'s
  `%lo(D_80367D73)` as `%lo(D_80367D60 + 0x13)` (and the `%hi`) and
  reassemble with `mips-linux-gnu-as -EB -march=vr4300 -mabi=32 target.s
  -o target.o` to get rid of it. Constants in `.rodata` don't count.
- pycparser reads gSPVertex's `sizeof(Vtx)*(n)` as a cast. `permute.sh`
  rewrites it as `(n * sizeof(Vtx))`, which compiles the same.
- asm-processor has no `-O3`, so an `-O3` file can't mix C and
  `GLOBAL_ASM`: all of its functions become C at once or none do.

To score a hand variant without waiting for the permuter, compile it with
the directory's `compile.sh` and compare `objdump -d` of `target.o` and the
result. Scripting a sweep over small rewrites (every order of a few
statements, both operand orders of each `+`) often gets there sooner than
the permuter on a big function.

What worked on the last three:

- func_80265E48 (20460.c): the permuter found that the register numbers
  after the search loop follow the order of the two assignments in a
  min/max update (`farthest = i; farthestDist = dist;`, not the other way
  round). Flipping the other update by hand, and a comma group
  (`nearest = -1, nearestDist = 99999999, farthest = -1;`) for the order of
  the stores at the top, finished it.
- func_801FA74C (11530.c): about 1500 lines of FP register differences
  came from one expression shape. IDO evaluates value-producing ternaries
  (the `ABS`es in `$f20`-`$f30`) in source order and the arithmetic around
  them from the right, so `ABS(t) * a + (0.5 - ABS(t)) * b` and
  `(0.5 - ABS(t)) * b + ABS(t) * a` swap which saved register holds which
  `ABS`, and that renumbers every temporary in the rest of the function.
  FP register allocation depends on code much further down: cutting
  the function short changed the register of its first instruction. The
  permuter's partial wins (and an `if (x) {}` it added) weren't needed once
  the three colour lines were right.
- guRotateF (90C50.c) doesn't match yet. With 2.0I's rotate.c body and
  `dtor` assigned each call (it's `.bss` here), everything is the same
  except that the `lui` for the store of `dtor` in guNormalize's delay
  slot comes three instructions late, at both -O2 and -O3. The file is
  libultra gu, so it's `-O3` (guRotate matches only there), and an `-O3`
  file can't keep a `GLOBAL_ASM`, so guRotate waits for it.


## Versions

The same C builds us.v10, us.v11, jp and eu (CLAUDE.md "Versions" has the
scheme and the tools). What mattered getting them to match:

- Build the version, then `tools/vdiff.py <v>`. A function that differs
  there by a constant or two is usually one idea: a text or table index
  (`YOSHI_ENTRY`, `FE_ENTRY` in `yoshi.h`: jp's entry tables lack entries,
  eu's have more), an assert's `__LINE__` (`LINE_EU`, or a file's own macro
  as 10850.c's `BESTTIMES_LINE`), a PAL timing (eu counts 50 frames a
  second: 17E10's `{25, 67}` for `{30, 80}`), a size (`AUDIO_HEAP_SIZE`,
  `NUM_DMA_MESSAGES` in `audio.h`, `PAK_GAME_CODE` in `player.h`). Where
  several functions share one, a macro in the header reads better than
  `#if`s at every use.
- Field offsets that differ everywhere mean a struct is laid out
  differently: eu's `YoshiEntry` and `UnkStruct_8020D810` carry two more
  text pointers (German and a third language). An `#ifdef` in the header
  fixes every function that only differs by it at once.
- us.v10 is us.v11 before a few fixes: us.v11's hd_front_end 00000.c keeps
  `D_803643D4` in `D_802153E8` when its menu opens and puts it back when
  it's left with 0x4000 (B), and 10850.c counts its player list itself
  (`LIST_COUNT`); statement order around such a change moved too
  (`tools/permute.sh` works for any version with `VERSION=`).
- A function the version has differently and nothing simple explains is
  its `GLOBAL_ASM` (`tools/version_asm.py`); its `.rodata` goes into its .s
  as usual. Its local initialized aggregates are `.data`, which
  asm-processor doesn't move: write the function in C for that version, or
  define the aggregate (45BB0's `func_8028B240` is C in jp for that).
- Data and `.bss` differences: `tools/bss_c.py`/`data_c.py <module>
  <object> --version <v>` write the version's definitions (with the
  version's names), to put next to us.v11's under `#ifdef`. Only lines
  that differ need it (1D990's jp `.bss` has one more array); where a
  version orders a whole block differently (eu's hd.c `.bss`), a second
  block is clearer. Check each variable the map names: the code that
  matches is the evidence (vdiff reports where it disagrees with the map,
  and the `--pair` that fixes it).
- A version can show object boundaries us.v11 hides: jp and eu pad `.text`
  to 16 before `func_801EF380`, `func_8026FBB0` and `func_8028FC10` (us.v11
  needs no padding there), so those start objects (8380.c, 2B3F0.c,
  4B450.c); their data blocks were at the ends of the previous object's.
  Likewise us.v10 links 9570 after 17990, which showed that the 0x80 bytes
  before 9570's `.bss` are 9570's, not 7800's.

## Shared types (include/game/)

The game's structures have one definition each, in `blastcorps/include/game/`
(one header per subsystem); a C file includes the headers it needs after
`common.h` and keeps only the types nothing else uses.

| header       | what's in it |
| ---          | --- |
| `types.h`    | the conventions below: `ROMPTR()`, `RomAddr`, `AssetOffset`, `SegAddr`, `SIZE_CHECK` |
| `game.h`     | the shared game state: game mode words, the heap pointer, the frame index, the player's position, the controller buttons, `frontEndPresent` |
| `player.h`   | `PlayerInfo` (pfsHandler.c's `playerInfo`, `players[playerNumber]`), the medal values, the best times, `saveIt`, the EEPROM and Controller Pak layouts (`EepromSave`, `PAK_*`) |
| `level.h`    | the level numbers (`LEVEL_*`, `DUMMY_LEVELS`), `LevelInfo` (the per-level table), `LevelHeader` (the level file, ROM data), the level loader's run-time variables (grids, group tables, bounds), `LevelRdu`/`Rdu`, `LevelStats`, the front end's per-level table |
| `vehicle.h`  | `VEHICLE_*` (the object loader's types), `Vehicle` (the level's vehicles), `VehicleModel` (a vehicle's model file), `VehicleState` and the vehicle modules (which handwritten object runs which vehicle, and their variables the C uses) |
| `objects.h`  | the level objects: ammo boxes, TNT crates, blocks and holes, buildings, and the level file's records for them |
| `model.h`    | `Model` (the model table's models, ROM data) |
| `yoshi.h`    | the window system (yoshi.c): `YoshiWindow`, `YoshiEntry`, `YoshiIcon`, `ColorPair`, `yoshiState`/`currentYoshiWindow` |
| `sched.h`    | the scheduler: `Sched`, `SchedTask`, `SchedClient`, its functions and messages |
| `audio.h`    | the audio manager (`AMAudioMgr`, `AudioInfo`, `AMDMABuffer`, ...), the old-libaudio `SynConfig`/`SndBank`, the sound player |
| `frame.h`    | the per-frame buffer's two layouts (`FrameBuf` for the front end, `FrameGame` for gameplay) |
| `camera.h`   | the camera's variables (separate globals in hd.c) |
| `frontend.h` | hd_front_end-only structures and its `gDPSetPrimColorB` |

Conventions:

- Every struct gets `SIZE_CHECK(Type, size)`: a typedef that fails to compile,
  on IDO and on the port's clang alike, if a field moves.
- A field keeps its `unkXX` name unless there's evidence for a name, given in
  a comment next to it (a string, an assert, a known source, docs/). A struct
  whose meaning isn't known stays `UnkStruct_<address>` even when it's shared.
  The `D_`/`func_` symbols keep their address names; the header says what one
  is and why (`D_80364AE8` is academy.c's `playerNumber`). Renaming them is a
  separate step (symbols_known.txt, gen_symbols.py), and the port refers to a
  few of them by name.
- Fields in data that comes from ROM at run time are marked: `ROMPTR(T *)`
  for a pointer the loader relocates, `AssetOffset` for an offset from the
  start of the asset, `RomAddr` for a cartridge address. They expand to the
  N64 type, so they change no code; they are what a native port widens,
  rebases or byteswaps on load. `.data`/`.bss` stay plain C.
- Where two files read one field with different signedness, the header has
  the type most uses need, and the other uses get a cast: IDO folds
  `(s16)p->u16field` into `lh` and `(s8)p->u8field` into `lb`. A `char *`
  field read as bytes in one place is cast there: `((u8 *)p->text)[i]`.
- Views of one array at a constant offset (`D_802F8BDC[0].unk210` in a file
  whose struct was 0x268 bytes) become an index and a field
  (`D_802F8BDC[18].unk18`): the address is the same constant.
- Folding a constant into a variable index compiled the same everywhere it
  was tried: `D_02000000.unkD00[i]` and `.unk2C0[0x29 + i]`,
  `unk88[i]` and `.unk54[0x34 + i]`.
- A file's `.bss` variables can become one struct when they were laid out
  back to back (the scheduler at `D_80315440`, the four `LevelStats` at
  `D_8036EA60`), since the addresses don't change. Other files keep using
  the inner names (`D_803156C4` is `D_80315440.frameCount`): the names are in
  `module_syms_auto`, which `link_syms.py` makes relative to the struct;
  a name only C uses needs a line in `undefined_syms`. `bss_c.py --check`
  accepts a label inside a variable.
- Changing a type that a `.data` initializer uses means regenerating it:
  `tools/data_c.py <module> <object> --ref <saved .data.s>`. The reference
  is the block's asm from an extract with the subsegment switched back to
  `data` (save `asm/data/` from that, restore the yaml, re-extract). Remove
  the old definitions after it writes the new ones.

Things the headers don't cover yet: the per-frame buffer is declared per
file (the front end as `FrameBuf[]`, gameplay as `FrameGame[]`), since it is
one buffer with two layouts; some files still see segment 2 as a plain
`Mtx[]`; 55 file-local struct types are used by one file only.

A global is declared once, in its owner's header; a file that needs the
other signedness casts (`(s16)D_803BE714`), which IDO folds into the load.
Most of the casts that unifying 50 of them called for turned out not to
matter (drop each, rebuild, keep it only if the sha1 fails): two were
left.  Two things didn't work: `(u8)x` of an `s8` global passed to a `u8`
parameter loads it with `lb`, and `*(u8 *)&x` in hd.c moved hd.c's
`.bss`; changing the defining file's type (a variable it only stores to)
did.

docs/TYPES.md has the structures of the gameplay state, and
`include/game/inventory.json` (tools/inventory.py) the machine-readable
inventory of every field that holds an address and every field's width.

These are only guesses at meaning until they're in `symbols_known.txt`:

- `func_8029A7E4` and `func_8029A7D0` are the debug printf, compiled out:
  `void (char *, ...)`.
- `D_8036C778` is a u64 flag word.
  `func_80275270` takes `(u64, f32)`; `func_80275390` and `func_8026F92C`
  take `(u64)`.
- `func_8028B4C4(u8 *romStart, u8 *dst, s32 *size, s32, s32, s32)` loads from
  ROM; callers pass `end - start` of ROM ranges.
- The C files' original names, from their asserts and prints: hd_code 00000
  hd.c (with master_switch.c included), 14B30 drawtext.c, 168B0 font.c, 17210
  recording.c, 22EE0 audio.c, 26570 yoshi.c, 2C560 sched.c, 30430 fade.c,
  34430 mb.c, 409D0 stats_perm.c, 41930 academy.c, 45BB0 controller.c, 50670
  ghostdigger.c; hd_front_end 00000 digger_loop.c, 10850 bestTimes.c, 17990
  back_loop.c, 1C40 player.c, 7800 stats.c, E7B0 pfsHandler.c.
