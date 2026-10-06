# What depends on the N64's layout

Every build of the port keeps each N64-named variable at its N64 address,
and the game's C, the engine's C (`port/engine`) and the host rely on that
in places.  This is the list of those places that the scattered layout
(docs/PORT.md, "The scattered layout", `-DPORT_SCATTER`) found in us.v10:
the input for making the port an ordinary C program, whose variables are
wherever the compiler puts them.  Each one has to read its data through
the variable it is in (a field, an index, a pointer from it), not through
a name or an address that is next to it on the N64.

## How it was made

Two check builds (`-DPORT_SCATTER_CHECK=ON`, 32-bit native-endian
movable), seeds 1 and 2 (`S1`, `S2`), at 4803fbe (the asm data as C,
`port/tools/asm2c.py`: a variable a label); each ran the quick tier's
five base scenarios (`attract`, `auto1`, `auto2`, `auto3`,
`attract.long`: 24,000 frames) and the TAS (`tas`).  Then

```
port/tools/scatter_list.py --static build/sc1-mn32 \
    --run S1:attract build/sc1-mn32 .../attract/log.txt ...
```

The runs column counts the scenarios of each build that reached a row
(`+tas`: the TAS did too).  The check takes every access where the N64's
layout has what it meant and plays on.  Both builds played the whole TAS:
57 platinum, all 125,297 of the log's reads matched, no mode forced, the
save the reference's (S1 in 33.9 minutes, S2 in 19.3); only the gameplay
digest differs from the reference's (S1 `5da1083ec7e48576`, S2
`32f8dae506cfd8e5`, not `7ec6ef73a5265afa`), as a walk that runs past
the padding (`D_803B7FC8`'s, below) reads other variables' bytes.  (The
first list, before the asm data was C, stopped the TAS at the log's read
1,900 and at `port/engine/56040.c:1268` later; neither happens now.)  So
the list covers every level the TAS plays, the attract mode's demo levels
and the menus.

With the check, a name inside the asm data stays in its variable (the
generated data points into the variable, and the engine compares such
pointers: `func_802A06B4`'s keys), so the alias rows are the C's; the
static list below has all 256 names, 241 of them in the asm data, where
a label inside a typed array or a member of a file's struct (asm2c.py's
`FILE_STRUCTS`) is a name inside a variable now.

P9's real array sizes (`D_802158A8`, `D_8036DCE0`, `D_8036E380`,
`D_80215A88`, `D_80218270`, `D_80365588`, `D_8036C7D0`, and
`D_8036BA98` and `D_80367BDC` merged under `TARGET_PC`) took their sites
out of the past-end rows: none of these names is in a row now.  The
engine's `D_803ED390` (a vehicle's two halfwords) is gone from the label
rows too.

Without the check, the scattered builds break where the list says they
would: before the asm data was C, the 64-bit big-endian movable build
with seeds 1 and 2 differed from the quick tier's references in all four
base scenarios from frame 750 on (the gameplay digest, the sound, every
screenshot after 500), and the TAS crashed after 1.1 and 1.8 minutes.
The builds without `PORT_SCATTER` are unchanged.

## The kinds

- **alias**: a name the C declares as its own that is inside another
  variable (a field, the second byte of a word): in the scattered layout
  it has room of its own, and the container doesn't see what is written
  through it.
- **past end**: an access past the end of a C variable, into what follows
  it on the N64: a `T x[1]` or a `T x` that is really an array or a
  struct, a loop past an array's end.
- **label**: the same past an asm data variable (a label), into the next
  label of the same file: a table indexed or walked from its first label
  (the engine's per-wheel words `D_803ED398`..`3A0` and bytes
  `D_803ED3EA`..`EC`, a list's end pointer that is the next label's
  address).
- **before**: an access before a variable's start (a negative index, or
  a walk that started elsewhere); **address**: an N64 place a variable
  left, reached by an address made otherwise than from the variable.
  Neither was seen in these runs.

"N64 layout has" is where the N64 has the byte the access reached (the
name there and the offset); "a walk: then N more" is a site that went on
through N more places (the first is the cause).  A site of `?` has no
line (code the compiler made for several).

## The host

- `port/host/replay.c:492` reads the player's position as
  `D_803643E0 + 4 * i`, three words from one name (`D_803643E4`,
  `D_803643E8` are their own variables): in the scattered layout the
  replay reports the player at a garbage position.
- The host reads `D_803156C0` and `D_803156C4` (`PORT_VAR`), which are
  `D_80315440`'s (the scheduler's) fields at +0x280 and +0x284 and have no
  declaration in the C; the scattered build points the host at them inside
  the scheduler, as the N64 has them.
- Not covered: the host's other reads of game memory by address (the
  renderer's display lists and textures, the audio, the loaders), which
  the check doesn't see; and the front end's variables, which stay where
  they are (`port/src/overlay.c` restores its `.data` and clears its
  `.bss` by address: `-DPORT_SCATTER_FE=ON` moves them, untried).

## Summary

| kind | rows |
| --- | --- |
| alias | 17 |
| past end | 7 |
| label | 37 |

### alias

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `D_802F499A` | `D_802F48D0+0x110` | blastcorps/src/hd_code/26570.c:3047 `func_8026AD30` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_802F8BF4` | `D_802F8BDC+0x884`, `D_802F8BDC+0x84C` | blastcorps/src/hd_code/00000.c:3377, 3399 `func_802507C8` | load 2 | S1 1 +tas, S2 1 +tas |
| `D_80364AF8` | `D_80364AF0+0x8` | blastcorps/src/hd_code/41930.c:94 `func_80286330` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_80364B80` | `D_80364AF0+0x90` | ? `func_802979E0` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_80364B80` | `D_80364AF0+0x90` | blastcorps/src/hd_code/53220.c:490, 498, 558 `func_802979E0` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_80364B80` | `D_80364AF0+0x90` | blastcorps/src/hd_code/53220.c:582 `func_80297EF8` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_80364B81` | `D_80364AF0+0x91` | blastcorps/src/hd_code/41930.c:38, 41, 42, 66 `func_802860F0` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_80364B81` | `D_80364AF0+0x91` | blastcorps/src/hd_code/41930.c:73 `func_802862DC` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_80364B81` | `D_80364AF0+0x91` | blastcorps/src/hd_code/41930.c:80 `func_80286330` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_80364BE0` | `D_80364AF0+0xF0` | blastcorps/src/hd_code/45BB0.c:201 `func_8028A470` | load 4 | S1 6 +tas, S2 6 +tas |
| `D_80370C00` | `D_80370BF8+0x8` | blastcorps/src/hd_front_end/E7B0.c:394 `func_801F5FE4` | load 4 | S1 4 +tas, S2 4 +tas |
| `D_80370C32` | `D_80370C30+0x2` | blastcorps/src/hd_code/17210.c:222 `func_8025BEF8` | store 1 | S1 6 +tas, S2 6 +tas |
| `D_80370C32` | `D_80370C30+0x2` | blastcorps/src/hd_code/51690.c:119 `func_80295EFC` | load 1 | S1 3 +tas, S2 3 +tas |
| `D_80370C33` | `D_80370C30+0x3` | blastcorps/src/hd_code/17210.c:223 `func_8025BEF8` | store 1 | S1 6 +tas, S2 6 +tas |
| `D_80370C33` | `D_80370C30+0x3` | blastcorps/src/hd_code/51690.c:119 `func_80295EFC` | load 1 | S1 3 +tas, S2 3 +tas |
| `D_8039B6B0` | `D_8039B628+0x188` | blastcorps/src/hd_front_end/E7B0.c:530, 532 `func_801F65C4` | load 1, store 1 | S1 1 +tas, S2 1 +tas |
| `D_8039B6B0` | `D_8039B628+0x908` | blastcorps/src/hd_front_end/E7B0.c:666 `func_801F6CA4` | store 1 | S1 1 +tas, S2 1 +tas |

### past end

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `D_802F47B0` +0xC0, +0xC2, +0xC3, +0xC4, +0xC6, +0xC7 (a walk: then 4 more places) | `D_802F4870`, `D_802F4870+0x2`, `D_802F4870+0x3`, `D_802F4870+0x4`, ... (6) | blastcorps/src/hd_code/26570.c:3887, 3888, 3889, 3892, 3893, 3894 `func_8026BCE0` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_802F499A` +0xF | `D_802F48D0+0xD9 (past the alias)` | blastcorps/src/hd_code/26570.c:3047 `func_8026AD30` | load 1 | S1 4 +tas, S2 4 +tas |
| `D_802F5804` +0x33D8 | `D_802F8BDC` | blastcorps/src/hd_code/26570.c:3474 `func_8026BCE0` | load 2 | S1 1 +tas, S2 1 +tas |
| `D_802FC260` +0x2C (a walk: then 156 more places) | `D_802FC280+0xC` | blastcorps/src/hd_code/37530.c:194 `func_8027BE7C` | load 2 | S1 2 +tas, S2 2 +tas |
| `D_80364AF0` +0x491 | `D_80364F70+0x11` | ? `func_801E9718` | load 1 | S1 1 +tas, S2 1 +tas |
| `D_8036B9A8` +0x20, +0x40, +0x60, +0x80 | `D_8036B9C8`, `D_8036B9E8`, `D_8036BA08`, `D_8036BA28` | blastcorps/src/hd_code/409D0.c:78, 79, 80, 82 `func_802852EC` | to n64_sprintf 1 | S1 1 +tas, S2 1 +tas |
| `D_8036B9A8` +0x80 | `D_8036BA28` | blastcorps/src/hd_front_end/7800.c:170 `func_801EEDB4` | to n64_sprintf 1 | S1 1 +tas, S2 1 +tas |

### label

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `D_803B7FC8` +0x5A4 (a walk: then 811 more places) | `D_803B8568+0x4` | port/engine/56040.c:1301 `func_8029E5AC` | load 4 | S1 6 +tas |
| `D_803BE740` +0x54, +0x58 | `D_803BE780+0x14`, `D_803BE780+0x18` | blastcorps/src/hd_code/2C560.c:0, 384 `__scTaskComplete` | load 4 | S1 6 +tas, S2 6 +tas |
| `D_803BE740` +0x48 | `D_803BE780+0x8` | port/engine/5FD50.c:336 `func_802A467C` | store 4 | S1 6 +tas, S2 6 +tas |
| `D_803BE740` +0x40, +0x44 | `D_803BE780`, `D_803BE780+0x4` | port/src/ultra.c:0, 616 `osSpTaskStartGo` | load 4 | S1 6 +tas, S2 6 +tas |
| `D_803ED398` +0x4 | `D_803ED39C` | port/engine/62740.c:2046 `func_802A8768` | load 4 | S1 6 +tas, S2 6 +tas |
| `D_803ED398` +0x4 | `D_803ED39C` | port/engine/62740.c:1263 `func_802A93B0` | store 4 | S1 6 +tas, S2 6 +tas |
| `D_803ED398` +0x8, +0x4 | `D_803ED3A0`, `D_803ED39C` | port/engine/62740.c:1787, 1790 `func_802A95A4` | store 4 | S1 3 +tas, S2 3 +tas |
| `D_803ED3A8` +0x4 | `D_803ED3AC` | port/engine/62740.c:2060 `func_802A8768` | load 4 | S1 6 +tas, S2 6 +tas |
| `D_803ED3A8` +0x4, +0x8 | `D_803ED3AC`, `D_803ED3B0` | port/engine/62740.c:1685, 1686 `func_802A8B10` | load 4 | S1 6 +tas, S2 6 +tas |
| `D_803ED3A8` +0x4 | `D_803ED3AC` | port/engine/62740.c:1147 `func_802A9B1C` | store 4 | S1 6 +tas, S2 6 +tas |
| `D_803ED3A8` +0x4 | `D_803ED3AC` | port/engine/69BB0.c:413 `func_802AF340` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803ED3EA` +0x1 | `D_803ED3EB` | port/engine/62740.c:1193 `func_802A992C` | store 1 | S1 6 +tas, S2 6 +tas |
| `D_803ED3EA` +0x1 | `D_803ED3EB` | port/engine/62740.c:1143 `func_802A9B1C` | store 1 | S1 6 +tas, S2 6 +tas |
| `D_803ED3EE` +0x1, +0x2 | `D_803ED3EF`, `D_803ED3EF+0x1` | port/engine/62740.c:2034, 2045 `func_802A8768` | load 1, store 1 | S1 6 +tas, S2 6 +tas |
| `D_803ED3EE` +0x1 | `D_803ED3EF` | port/engine/62740.c:0 `func_802A93B0` | store 1 | S1 6 +tas, S2 6 +tas |
| `D_803ED3EE` +0x2, +0x1 | `D_803ED3EF+0x1`, `D_803ED3EF` | port/engine/62740.c:0 `func_802A95A4` | store 1 | S1 3 +tas, S2 3 +tas |
| `D_803ED3F2` +0x1 | `D_803ED3F3` | port/engine/62740.c:1195 `func_802A992C` | store 1 | S1 6 +tas, S2 6 +tas |
| `D_803ED3F2` +0x1 | `D_803ED3F3` | port/engine/62740.c:1219 `func_802A9A60` | store 1 | S1 1 +tas, S2 1 +tas |
| `D_803ED3F2` +0x1 | `D_803ED3F3` | port/engine/62740.c:1144 `func_802A9B1C` | store 1 | S1 6 +tas, S2 6 +tas |
| `D_803EE768` +0x4, +0x8 (a walk: then 6 more places) | `D_803EE76C`, `D_803EE770` | port/engine/679E0.c:147, 148 `func_802AC85C` | store 4 | S1 6 +tas, S2 6 +tas |
| `D_803EF2EC` +0x8 | `D_803EF2F4` | port/engine/60D50.c:65 `func_802A5604` | load 4 | S1 4 +tas, S2 4 +tas |
| `D_803F8748` +0x4, +0x8, +0xC | `D_803F874C`, `D_803F8750`, `D_803F8754` | port/engine/62740.c:183, 184, 185 `func_802A75DC` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8748` +0x18, +0x1C, +0x20 | `D_803F8760`, `D_803F8764`, `D_803F8768` | port/engine/62740.c:202, 203, 204 `func_802A768C` | store 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8748` +0x8 | `D_803F8750` | port/engine/62740.c:676 `func_802A860C` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8748` +0x8, +0x4, +0xC | `D_803F8750`, `D_803F874C`, `D_803F8754` | port/engine/62740.c:2051, 2052, 2053, 2060, 2064 `func_802A8768` | load 4, store 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8748` +0x4 | `D_803F874C` | port/engine/62740.c:1198 `func_802A992C` | store 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8748` +0x4, +0x8 | `D_803F874C`, `D_803F8750` | port/engine/83910.c:158, 159, 162 `barge_draw` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8748` +0x4 | `D_803F874C` | port/engine/83910.c:131, 147 `barge_frame` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8748` +0x4, +0x8, +0xC | `D_803F874C`, `D_803F8750`, `D_803F8754` | port/engine/83910.c:79, 80, 81 `barge_setup` | store 4 | S1 1 +tas, S2 1 +tas |
| `D_803F876C` +0x4 | `D_803F8770` | port/engine/83910.c:153 `barge_draw` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F876C` +0x4 | `D_803F8770` | port/engine/83910.c:70, 86, 103 `barge_setup` | load 4, store 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8778` +0x4 | `D_803F877C` | port/engine/83910.c:153 `barge_draw` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8778` +0x4, +0x14, +0x10 | `D_803F877C`, `D_803F878C`, `D_803F8788` | port/engine/83910.c:128, 139, 140 `barge_frame` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8778` +0x4, +0x8 | `D_803F877C`, `D_803F8780` | port/engine/83910.c:71, 72, 86, 92, 95, 103 `barge_setup` | load 4, store 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8778` +0x4 | `D_803F877C` | port/engine/83910.c:245 `func_802C8B0C` | load 4 | S1 1 +tas, S2 1 +tas |
| `D_803F8790` +0x1, +0x2 | `D_803F8791`, `D_803F8792` | port/engine/83910.c:137, 138, 142 `barge_frame` | load 1, store 1 | S1 1 +tas, S2 1 +tas |
| `D_803F8790` +0x1 | `D_803F8791` | port/engine/83910.c:75 `barge_setup` | store 1 | S1 1 +tas, S2 1 +tas |

### Names inside other variables (static)

256 names the N64 side declares that are inside another variable (port-arena's report), whether a run reached them or not; `seen` marks those a run did.

| name | inside | C or asm | users | seen |
| --- | --- | --- | --- | --- |
| `D_802F499A` | `D_802F48D0+0xCA` | c | func_8026AD30 | seen |
| `D_802F8BF4` | `D_802F8BDC+0x18` | c | func_802507C8 (blastcorps/src/hd_code/00000.c:3377, 3388, 3399) | seen |
| `D_80301080` | `D_80300A68+0x618` | asm | func_801F6F18 (blastcorps/src/hd_front_end/E7B0.c:773) |  |
| `D_80303700` | `D_803035C8+0x134` | asm | initializer of D_8020D810 |  |
| `D_8030370C` | `D_803035C8+0x140` | asm | initializer of D_8020D810 |  |
| `D_80303720` | `D_803035C8+0x154` | asm | initializer of D_8020D810 |  |
| `D_80303734` | `D_803035C8+0x168` | asm | initializer of D_8020D810 |  |
| `D_80303748` | `D_803035C8+0x17C` | asm | initializer of D_8020D810 |  |
| `D_8030375C` | `D_803035C8+0x190` | asm | initializer of D_8020D810 |  |
| `D_8030376C` | `D_803035C8+0x1A0` | asm | initializer of D_8020D810 |  |
| `D_8030377C` | `D_803035C8+0x1B0` | asm | initializer of D_8020D810 |  |
| `D_80303794` | `D_803035C8+0x1C8` | asm | initializer of D_8020D810 |  |
| `D_803037A8` | `D_803035C8+0x1DC` | asm | initializer of D_8020D810 |  |
| `D_803037B8` | `D_803035C8+0x1EC` | asm | initializer of D_8020D810 |  |
| `D_803037CC` | `D_803035C8+0x200` | asm | initializer of D_8020D810 |  |
| `D_803037D8` | `D_803035C8+0x20C` | asm | initializer of D_8020D810 |  |
| `D_803037EC` | `D_803035C8+0x220` | asm | initializer of D_8020D810 |  |
| `D_803037FC` | `D_803035C8+0x230` | asm | initializer of D_8020D810 |  |
| `D_8030380C` | `D_803035C8+0x240` | asm | initializer of D_8020D810 |  |
| `D_8030381C` | `D_803035C8+0x250` | asm | initializer of D_8020D810 |  |
| `D_8030382C` | `D_803035C8+0x260` | asm | initializer of D_8020D810 |  |
| `D_80303844` | `D_803035C8+0x278` | asm | initializer of D_8020D810 |  |
| `D_80303854` | `D_803035C8+0x288` | asm | initializer of D_8020D810 |  |
| `D_80303864` | `D_803035C8+0x298` | asm | initializer of D_8020D810 |  |
| `D_80303874` | `D_803035C8+0x2A8` | asm | initializer of D_8020D810 |  |
| `D_80303884` | `D_803035C8+0x2B8` | asm | initializer of D_8020D810 |  |
| `D_80303890` | `D_803035C8+0x2C4` | asm | initializer of D_8020D810 |  |
| `D_803038A0` | `D_803035C8+0x2D4` | asm | initializer of D_8020D810 |  |
| `D_803038B0` | `D_803035C8+0x2E4` | asm | initializer of D_8020D810 |  |
| `D_803038C0` | `D_803035C8+0x2F4` | asm | initializer of D_8020D810 |  |
| `D_803038D0` | `D_803035C8+0x304` | asm | initializer of D_8020D810 |  |
| `D_803038E0` | `D_803035C8+0x314` | asm | initializer of D_8020D810 |  |
| `D_803038F8` | `D_803035C8+0x32C` | asm | initializer of D_8020D810 |  |
| `D_80303904` | `D_803035C8+0x338` | asm | initializer of D_8020D810 |  |
| `D_80303918` | `D_803035C8+0x34C` | asm | initializer of D_8020D810 |  |
| `D_80303930` | `D_803035C8+0x364` | asm | initializer of D_8020D810 |  |
| `D_8030393C` | `D_803035C8+0x370` | asm | initializer of D_8020D810 |  |
| `D_80303950` | `D_803035C8+0x384` | asm | initializer of D_8020D810 |  |
| `D_80303960` | `D_803035C8+0x394` | asm | initializer of D_8020D810 |  |
| `D_80303974` | `D_803035C8+0x3A8` | asm | initializer of D_8020D810 |  |
| `D_80303980` | `D_803035C8+0x3B4` | asm | initializer of D_8020D810 |  |
| `D_80303990` | `D_803035C8+0x3C4` | asm | initializer of D_8020D810 |  |
| `D_803039A8` | `D_803035C8+0x3DC` | asm | initializer of D_8020D810 |  |
| `D_803039B4` | `D_803035C8+0x3E8` | asm | initializer of D_8020D810 |  |
| `D_803039BC` | `D_803035C8+0x3F0` | asm | initializer of D_8020D810 |  |
| `D_803039CC` | `D_803035C8+0x400` | asm | initializer of D_8020D810 |  |
| `D_803039E0` | `D_803035C8+0x414` | asm | initializer of D_8020D810 |  |
| `D_803039F0` | `D_803035C8+0x424` | asm | initializer of D_8020D810 |  |
| `D_803039FC` | `D_803035C8+0x430` | asm | initializer of D_8020D810 |  |
| `D_80303A04` | `D_803035C8+0x438` | asm | initializer of D_8020D810 |  |
| `D_80303A14` | `D_803035C8+0x448` | asm | initializer of D_8020D810 |  |
| `D_80303A24` | `D_803035C8+0x458` | asm | initializer of D_8020D810 |  |
| `D_80303A34` | `D_803035C8+0x468` | asm | initializer of D_8020D810 |  |
| `D_80303A48` | `D_803035C8+0x47C` | asm | initializer of D_8020D810 |  |
| `D_80303A58` | `D_803035C8+0x48C` | asm | initializer of D_8020D810 |  |
| `D_80303A68` | `D_803035C8+0x49C` | asm | initializer of D_8020D810 |  |
| `D_80303A74` | `D_803035C8+0x4A8` | asm | initializer of D_8020D810 |  |
| `D_80303A8C` | `D_803035C8+0x4C0` | asm | initializer of D_8020D810 |  |
| `D_80303A98` | `D_803035C8+0x4CC` | asm | initializer of D_8020D810 |  |
| `D_80303AB0` | `D_803035C8+0x4E4` | asm | initializer of D_8020D810 |  |
| `D_80303AC0` | `D_803035C8+0x4F4` | asm | initializer of D_8020D810 |  |
| `D_80303AD4` | `D_803035C8+0x508` | asm | initializer of D_8020D810 |  |
| `D_80303AE8` | `D_803035C8+0x51C` | asm | initializer of D_8020D810 |  |
| `D_80303AF4` | `D_803035C8+0x528` | asm | func_80269258 (blastcorps/src/hd_code/23C20.c:620) |  |
| `D_80303B3C` | `D_80303B24+0x18` | asm | initializer of D_802084E0 |  |
| `D_80303B48` | `D_80303B24+0x24` | asm | initializer of D_802084E0 |  |
| `D_80303B58` | `D_80303B24+0x34` | asm | initializer of D_802084E0 |  |
| `D_80303B68` | `D_80303B24+0x44` | asm | initializer of D_802084E0 |  |
| `D_80303B78` | `D_80303B24+0x54` | asm | initializer of D_802084B8 |  |
| `D_80303B88` | `D_80303B24+0x64` | asm | initializer of D_802084BC |  |
| `D_803041DC` | `D_803041B4+0x28` | asm | initializer of D_802081C0 |  |
| `D_803041EC` | `D_803041B4+0x38` | asm | initializer of D_802081C0 |  |
| `D_803041FC` | `D_803041B4+0x48` | asm | initializer of D_802081C0 |  |
| `D_8030420C` | `D_803041B4+0x58` | asm | initializer of D_802081C0 |  |
| `D_8030421C` | `D_803041B4+0x68` | asm | initializer of D_802081C0 |  |
| `D_8030422C` | `D_803041B4+0x78` | asm | initializer of D_802081C0 |  |
| `D_8030423C` | `D_803041B4+0x88` | asm | initializer of D_802081C0 |  |
| `D_8030424C` | `D_803041B4+0x98` | asm | initializer of D_802081C0 |  |
| `D_8030425C` | `D_803041B4+0xA8` | asm | initializer of D_802081C0 |  |
| `D_8030426C` | `D_803041B4+0xB8` | asm | initializer of D_802081C0 |  |
| `D_8030427C` | `D_803041B4+0xC8` | asm | initializer of D_802081C0 |  |
| `D_80304288` | `D_803041B4+0xD4` | asm | initializer of D_802081C0 |  |
| `D_8030429C` | `D_803041B4+0xE8` | asm | initializer of D_802081C0 |  |
| `D_803042AC` | `D_803041B4+0xF8` | asm | initializer of D_802081C0 |  |
| `D_803042B8` | `D_803041B4+0x104` | asm | initializer of D_802081C0 |  |
| `D_803042CC` | `D_803041B4+0x118` | asm | initializer of D_802081C0 |  |
| `D_803042E0` | `D_803041B4+0x12C` | asm | initializer of D_802081C0 |  |
| `D_803042F0` | `D_803041B4+0x13C` | asm | initializer of D_802081C0 |  |
| `D_80304304` | `D_803041B4+0x150` | asm | initializer of D_802081C0 |  |
| `D_8030430C` | `D_803041B4+0x158` | asm | initializer of D_802081C0 |  |
| `D_80304318` | `D_803041B4+0x164` | asm | initializer of D_802081C0 |  |
| `D_80304328` | `D_803041B4+0x174` | asm | initializer of D_802081C0 |  |
| `D_80304334` | `D_803041B4+0x180` | asm | initializer of D_802081C0 |  |
| `D_80304344` | `D_803041B4+0x190` | asm | initializer of D_802081C0 |  |
| `D_80304358` | `D_803041B4+0x1A4` | asm | initializer of D_802081C0 |  |
| `D_80304364` | `D_803041B4+0x1B0` | asm | initializer of D_802081C0 |  |
| `D_80304370` | `D_803041B4+0x1BC` | asm | initializer of D_802081C0 |  |
| `D_8030437C` | `D_803041B4+0x1C8` | asm | initializer of D_802081C0 |  |
| `D_80304388` | `D_803041B4+0x1D4` | asm | initializer of D_802081C0 |  |
| `D_80304394` | `D_803041B4+0x1E0` | asm | initializer of D_802081C0 |  |
| `D_8030439C` | `D_803041B4+0x1E8` | asm | initializer of D_802081C0 |  |
| `D_803043B8` | `D_803041B4+0x204` | asm | initializer of D_80208368 |  |
| `D_80304474` | `D_803041B4+0x2C0` | asm | initializer of D_80208368 |  |
| `D_80304544` | `D_803041B4+0x390` | asm | initializer of D_80208368 |  |
| `D_80304614` | `D_803041B4+0x460` | asm | initializer of D_80208368 |  |
| `D_803046F8` | `D_803041B4+0x544` | asm | func_801EA4B8 |  |
| `D_80304710` | `D_803041B4+0x55C` | asm | func_801EA4B8 |  |
| `D_80304730` | `D_803041B4+0x57C` | asm | func_801EA6E8 (blastcorps/src/hd_front_end/1C40.c:898) |  |
| `D_803048CC` | `D_8030480C+0xC0` | asm | initializer of D_8020E430 |  |
| `D_803048D8` | `D_8030480C+0xCC` | asm | initializer of D_8020E430 |  |
| `D_803048E4` | `D_8030480C+0xD8` | asm | initializer of D_8020E430 |  |
| `D_803048F0` | `D_8030480C+0xE4` | asm | initializer of D_8020E430 |  |
| `D_80304904` | `D_8030480C+0xF8` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:2916) |  |
| `D_80304910` | `D_8030480C+0x104` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:2918) |  |
| `D_8030491C` | `D_8030480C+0x110` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:2909) |  |
| `D_80304938` | `D_8030480C+0x12C` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:2912) |  |
| `D_80304954` | `D_8030480C+0x148` | asm | initializer of D_80208044 |  |
| `D_80305D62` | `D_80305D60+0x2` | asm | func_802BA354; func_802BA354 (port/engine/75490.c:319); func_802BA638; func_802BA638 (port/engine/75490.c:355) |  |
| `D_80305D74` | `D_80305D60+0x14` | asm | func_802B9C50; func_802B9C50 (port/engine/75490.c:219); func_802BA074; func_802BA074 (port/engine/75490.c:265) |  |
| `D_80305DF0` | `D_80305D60+0x90` | asm | func_802BB274 (port/engine/75490.c:558) |  |
| `D_80305E38` | `D_80305E10+0x28` | asm | func_802C0E8C; func_802C0E8C (port/engine/77E20.c:2428) |  |
| `D_80305E50` | `D_80305E10+0x40` | asm | func_802BFF6C |  |
| `D_80306270` | `D_80305E10+0x460` | asm | func_802C0574 (port/engine/77E20.c:2181) |  |
| `D_8030631F` | `D_80305E10+0x50F` | asm | func_802A21AC (port/engine/5CB60.c:1114); func_802A23E0 (port/engine/5CB60.c:806) |  |
| `D_8030633C` | `D_80305E10+0x52C` | asm | func_802BF898 (port/engine/77E20.c:1765); func_802BF978 (port/engine/77E20.c:1835); func_802BFBF4 (port/engine/77E20.c:0) |  |
| `D_80306344` | `D_80305E10+0x534` | asm | func_802BF978 (port/engine/77E20.c:0) |  |
| `D_80306350` | `D_80305E10+0x540` | asm | func_802BF978 (port/engine/77E20.c:0) |  |
| `D_803063D4` | `D_80305E10+0x5C4` | asm | func_802BFDAC (port/engine/77E20.c:1896) |  |
| `D_803063E0` | `D_80305E10+0x5D0` | asm | func_802BF978 (port/engine/77E20.c:1792) |  |
| `D_80364AF8` | `D_80364AF0+0x8` | c | func_80286330 (blastcorps/src/hd_code/41930.c:94) | seen |
| `D_80364B80` | `D_80364AF0+0x90` | c | func_802860F0; func_802979E0; func_802979E0 (blastcorps/src/hd_code/53220.c:490, 498, 558); func_80297EF8 (blastcorps/src/hd_code/53220.c:582); func_80297F74 | seen |
| `D_80364B81` | `D_80364AF0+0x91` | c | func_802860F0 (blastcorps/src/hd_code/41930.c:38, 41, 42, 66); func_802862DC (blastcorps/src/hd_code/41930.c:73); func_80286330 (blastcorps/src/hd_code/41930.c:80) | seen |
| `D_80364BE0` | `D_80364AF0+0xF0` | c | func_8028A470 (blastcorps/src/hd_code/45BB0.c:201); func_8028B240 (blastcorps/src/hd_code/45BB0.c:510) | seen |
| `D_8036BF90` | `D_8036BF78+0x18` | c | __scMain (blastcorps/src/hd_code/2C560.c:171) |  |
| `D_8036BF94` | `D_8036BF78+0x1C` | c | __scMain (blastcorps/src/hd_code/2C560.c:171) |  |
| `D_80370C00` | `D_80370BF8+0x8` | c | func_801F5FE4 (blastcorps/src/hd_front_end/E7B0.c:394) | seen |
| `D_80370C32` | `D_80370C30+0x2` | c | func_8025BEF8 (blastcorps/src/hd_code/17210.c:222); func_80295EFC (blastcorps/src/hd_code/51690.c:119) | seen |
| `D_80370C33` | `D_80370C30+0x3` | c | func_8025BEF8 (blastcorps/src/hd_code/17210.c:223); func_80295EFC (blastcorps/src/hd_code/51690.c:119) | seen |
| `D_8039B630` | `D_8039B628+0x8` | c | func_801F58E8 (blastcorps/src/hd_front_end/E7B0.c:201, 204, 207, 210, 237, 247, 250, 256, 259, 296, 322); func_801F60C8 (blastcorps/src/hd_front_end/E7B0.c:424); func_801F6160 (blastcorps/src/hd_front_end/E7B0.c:435); func_801F61C8 (blastcorps/src/hd_front_end/E7B0.c:439); func_801F6210 (blastcorps/src/hd_front_end/E7B0.c:445); func_801F6264 (blastcorps/src/hd_front_end/E7B0.c:491, 497, 506); func_801F65C4 (blastcorps/src/hd_front_end/E7B0.c:536, 538); func_801F6AF4 (blastcorps/src/hd_front_end/E7B0.c:595); func_801F6BD0 (blastcorps/src/hd_front_end/E7B0.c:638); func_801F6CA4 (blastcorps/src/hd_front_end/E7B0.c:672, 675); func_801F6ED4 (blastcorps/src/hd_front_end/E7B0.c:685) |  |
| `D_8039B698` | `D_8039B628+0x70` | c | func_801F58E8; func_801F6160 (blastcorps/src/hd_front_end/E7B0.c:435); func_801F6264; func_801F6264 (blastcorps/src/hd_front_end/E7B0.c:506); func_801F65C4 (blastcorps/src/hd_front_end/E7B0.c:536); func_801F6AF4 (blastcorps/src/hd_front_end/E7B0.c:595); func_801F6BD0 (blastcorps/src/hd_front_end/E7B0.c:638); func_801F6CA4 (blastcorps/src/hd_front_end/E7B0.c:672) |  |
| `D_8039B6B0` | `D_8039B628+0x88` | c | func_801F6264 (blastcorps/src/hd_front_end/E7B0.c:0); func_801F65C4 (blastcorps/src/hd_front_end/E7B0.c:530, 532); func_801F6CA4 (blastcorps/src/hd_front_end/E7B0.c:666, 668) | seen |
| `D_8039C538` | `D_8039C4F8+0x40` | c | func_801EA278; func_801EA278 (blastcorps/src/hd_front_end/1C40.c:806); func_801F58E8 (blastcorps/src/hd_front_end/E7B0.c:201, 316); func_801F60C8 (blastcorps/src/hd_front_end/E7B0.c:417) |  |
| `D_803BDFD4` | `D_803BDE40+0x194` | asm | func_802A1674 (port/engine/5CB60.c:1460); func_802A2C54 (port/engine/5CB60.c:345); func_802ABD54 (port/engine/62740.c:606); func_802BCA2C; func_802BEBB0; func_802BF534 |  |
| `D_803EF2E6` | `D_803EF240+0xA6` | asm | func_802B8480 (port/engine/72B80.c:407); func_802B98E0 (port/engine/72B80.c:711, 714) |  |
| `D_803EF6D6` | `D_803EF630+0xA6` | asm | func_802B9C50 (port/engine/75490.c:211); func_802BA6AC (port/engine/75490.c:366, 369) |  |
| `D_803F0900` | `D_803EFED0+0xA30` | asm | func_802A1D54 (port/engine/5CB60.c:1155, 1156); func_802C08C4 (port/engine/77E20.c:2227, 2229, 2230, 2236, 2237, 2240, 2246); func_802C09B8; func_802C09B8 (port/engine/77E20.c:2311) |  |
| `D_803F1BE0` | `D_803EFED0+0x1D10` | asm | func_802A1D54 (port/engine/5CB60.c:1159, 1160); func_802C12E0 (port/engine/77E20.c:2502, 2503, 2504, 2508, 2509, 2512, 2513, 2515, 2516, 2518, 2519, 2523, 2525, 2527, 2529, 2531); func_802C1438; func_802C1438 (port/engine/77E20.c:2562) |  |
| `D_803F24D0` | `D_803EFED0+0x2600` | asm | func_802BD1F8 |  |
| `D_803F2ED0` | `D_803EFED0+0x3000` | asm | func_802BD1F8 |  |
| `D_803F38D0` | `D_803EFED0+0x3A00` | asm | func_802BE574 (port/engine/77E20.c:1213, 1214, 1215, 1216, 1217, 1218, 1219, 1220, 1221, 1222) |  |
| `D_803F3910` | `D_803EFED0+0x3A40` | asm | func_802AC2A4; func_802AC2A4 (port/engine/71140.c:465, 472); func_802BE77C (port/engine/77E20.c:1248); func_802BE9F8 (port/engine/77E20.c:1306); func_802BEA70; func_802BEA70 (port/engine/77E20.c:1324); func_802BEBB0; func_802BEBB0 (port/engine/77E20.c:1428) |  |
| `D_803F3960` | `D_803EFED0+0x3A90` | asm | func_802AC2A4 (port/engine/71140.c:465, 472); func_802BE77C (port/engine/77E20.c:1248); func_802BE9F8 (port/engine/77E20.c:1306); func_802BEA30 (port/engine/77E20.c:1312, 1316); func_802BEA70; func_802BEBB0; func_802BEBB0 (port/engine/77E20.c:1431) |  |
| `D_803F3964` | `D_803EFED0+0x3A94` | asm | func_802BD1F8 (port/engine/77E20.c:0); func_802BE574 (port/engine/77E20.c:1190, 1195) |  |
| `D_803F3968` | `D_803EFED0+0x3A98` | asm | func_802BD1F8 (port/engine/77E20.c:833); func_802BF898 (port/engine/77E20.c:1765); func_802BF978 (port/engine/77E20.c:1833); func_802BFD1C (port/engine/77E20.c:1882); func_802BFDAC (port/engine/77E20.c:1917); func_802C049C (port/engine/77E20.c:2146); func_802C04F0 (port/engine/77E20.c:2156); func_802C0574 (port/engine/77E20.c:2173) |  |
| `D_803F3FF8` | `D_803EFED0+0x4128` | asm | func_802BF978 (port/engine/77E20.c:0, 1807, 1808, 1809, 1815, 1816, 1817, 1818, 1819, 1820, 1821, 1822, 1828, 1829, 1830, 1831, 1832, 1833); func_802BFDAC (port/engine/77E20.c:1899, 1900, 1901, 1903, 1904, 1905, 1906, 1907, 1908, 1909, 1910, 1911, 1912, 1913, 1914, 1915, 1916, 1917); func_802CF3E0 (port/engine/8A2E0.c:240, 241, 242, 243, 244, 245, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259) |  |
| `D_803F4030` | `D_803EFED0+0x4160` | asm | func_80258B78 (blastcorps/src/hd_code/13A70.c:182, 183); func_8029AB88; func_8029AB88 (port/engine/56040.c:3098); func_8029B02C; func_8029B02C (port/engine/56040.c:2910); func_802A1D54 (port/engine/5CB60.c:1162); func_802ABEDC; func_802ABEDC (port/engine/62740.c:1311); func_802AC1A0; func_802AC1A0 (port/engine/679E0.c:59); func_802BCA2C (port/engine/77E20.c:392); func_802BCE40; func_802BCE40 (port/engine/77E20.c:500); func_802BD1F8; func_802BD1F8 (port/engine/77E20.c:757); func_802BE77C; func_802BEBB0 (port/engine/77E20.c:1493); func_802BF534 (port/engine/77E20.c:1664); func_802C18D4; func_802C18D4 (port/engine/77E20.c:2623); func_802C1AA0; func_802C1AA0 (port/engine/77E20.c:2664); func_802C1B1C; func_802C1B1C (port/engine/77E20.c:2680); func_802C1B9C; func_802C1B9C (port/engine/77E20.c:2702); func_802C1DD0; func_802C1DD0 (port/engine/77E20.c:2761); func_802C4A40; func_802C4A40 (port/engine/80280.c:76); func_802C4BF0; func_802C4BF0 (port/engine/80280.c:97); func_802C4E58; func_802C4E58 (port/engine/80280.c:146); func_802CDB70; func_802CDB70 (port/engine/89250.c:70) |  |
| `D_803F7654` | `D_803EFED0+0x7784` | asm | func_80258B78 (blastcorps/src/hd_code/13A70.c:182); func_8029AB88 (port/engine/56040.c:3098); func_8029B02C (port/engine/56040.c:2908); func_802A1D54 (port/engine/5CB60.c:1162); func_802A21AC (port/engine/5CB60.c:1076, 1077, 1127); func_802ABEDC (port/engine/62740.c:1311); func_802AC1A0 (port/engine/679E0.c:59); func_802BCE40 (port/engine/77E20.c:500); func_802BD1F8 (port/engine/77E20.c:756); func_802BE77C; func_802BE77C (port/engine/77E20.c:1252); func_802C18D4 (port/engine/77E20.c:2623); func_802C1AA0; func_802C1B1C; func_802C1B9C (port/engine/77E20.c:2702); func_802C1DD0; func_802C4A40 (port/engine/80280.c:76); func_802C4BF0 (port/engine/80280.c:97); func_802C4E58; func_802C4E58 (port/engine/80280.c:160); func_802CDB70 (port/engine/89250.c:70) |  |
| `D_803F7658` | `D_803EFED0+0x7788` | asm | func_802BD1F8 (port/engine/77E20.c:746); func_802C12E0 (port/engine/77E20.c:2498) |  |
| `D_803F765C` | `D_803EFED0+0x778C` | asm | func_802BD1F8 (port/engine/77E20.c:747, 774, 775) |  |
| `D_803F7660` | `D_803EFED0+0x7790` | asm | func_80282C80; func_8028376C; func_802C1B9C (port/engine/77E20.c:2746) |  |
| `D_803F7664` | `D_803EFED0+0x7794` | asm | func_802933A0 (blastcorps/src/hd_code/4EBE0.c:245); func_802BD1F8 (port/engine/77E20.c:781) |  |
| `D_803F7668` | `D_803EFED0+0x7798` | asm | func_802933A0 (blastcorps/src/hd_code/4EBE0.c:246); func_802BD1F8 (port/engine/77E20.c:782) |  |
| `D_803F766C` | `D_803EFED0+0x779C` | asm | func_802933A0 (blastcorps/src/hd_code/4EBE0.c:247); func_802BD1F8 (port/engine/77E20.c:783) |  |
| `D_803F7670` | `D_803EFED0+0x77A0` | asm | func_802507C8 (blastcorps/src/hd_code/00000.c:3892, 3990); func_80282C80 (blastcorps/src/hd_code/3E4C0.c:250); func_8028376C (blastcorps/src/hd_code/3E4C0.c:377); func_802C1B9C (port/engine/77E20.c:2717, 2726, 2741) |  |
| `D_803F7674` | `D_803EFED0+0x77A4` | asm | func_802507C8 (blastcorps/src/hd_code/00000.c:3893, 3991); func_802C1B9C (port/engine/77E20.c:2719) |  |
| `D_803F7678` | `D_803EFED0+0x77A8` | asm | func_802507C8 (blastcorps/src/hd_code/00000.c:3894, 3992); func_80282C80 (blastcorps/src/hd_code/3E4C0.c:251); func_8028376C (blastcorps/src/hd_code/3E4C0.c:377); func_802C1B9C (port/engine/77E20.c:2720, 2727, 2742) |  |
| `D_803F767C` | `D_803EFED0+0x77AC` | asm | func_802507C8 (blastcorps/src/hd_code/00000.c:3887, 3986); func_8028376C (blastcorps/src/hd_code/3E4C0.c:436); func_802A21AC (port/engine/5CB60.c:1072) |  |
| `D_803F767E` | `D_803EFED0+0x77AE` | asm | func_802507C8 (blastcorps/src/hd_code/00000.c:3888, 3987); func_802A21AC (port/engine/5CB60.c:1073) |  |
| `D_803F7680` | `D_803EFED0+0x77B0` | asm | func_802507C8 (blastcorps/src/hd_code/00000.c:3889, 3988); func_8028376C (blastcorps/src/hd_code/3E4C0.c:436); func_802A21AC (port/engine/5CB60.c:1074) |  |
| `D_803F7684` | `D_803EFED0+0x77B4` | asm | func_8026B8F8; func_8026B8F8 (blastcorps/src/hd_code/26570.c:3244); func_8026FA38; func_8026FA38 (blastcorps/src/hd_code/26570.c:4114); func_802C1DD0 (port/engine/77E20.c:2766) |  |
| `D_803F7688` | `D_803EFED0+0x77B8` | asm | func_802475D8 (blastcorps/src/hd_code/00000.c:1276); func_802C1DD0; func_802C1DD0 (port/engine/77E20.c:2784) |  |
| `D_803F7690` | `D_803EFED0+0x77C0` | asm | func_802BC840 (port/engine/77E20.c:300); func_802BC888; func_802BCA2C; func_802C18D4 |  |
| `D_803F77D0` | `D_803EFED0+0x7900` | asm | barge_frame (port/engine/83910.c:134); driver_frame (port/engine/69BB0.c:376); func_802AEC3C (port/engine/69BB0.c:302); func_802B03F4 (port/engine/6B4A0.c:222); func_802B152C (port/engine/6C5E0.c:257); func_802B6294 (port/engine/71140.c:296, 308, 313); func_802B7A88 (port/engine/72B80.c:298, 307); func_802BA9A0 (port/engine/75490.c:406); func_802BB274 (port/engine/75490.c:561); func_802BBEB8 (port/engine/772A0.c:190); func_802BFF6C (port/engine/77E20.c:2041); func_802CA4E0 (port/engine/853D0.c:292, 304); func_802CBEF0 (port/engine/86F60.c:231, 240); func_802CD068 (port/engine/88160.c:218, 227); func_802CFDE8 (port/engine/8AEE0.c:295, 304); func_802D0F98 (port/engine/8AEE0.c:536); jbomb_frame (port/engine/80280.c:611); ramdozer_frame (port/engine/6E200.c:675, 685); skyfall_frame (port/engine/6E200.c:272, 282) |  |
| `D_803F77D4` | `D_803EFED0+0x7904` | asm | func_802BCBD8 (port/engine/77E20.c:406); func_802BCC48; func_802BCCD4; func_802BCCD4 (port/engine/77E20.c:436); func_802BCD20; func_802BCD80; func_802BE77C; func_802BEBB0; func_802BEBB0 (port/engine/77E20.c:1427) |  |
| `D_803F77D8` | `D_803EFED0+0x7908` | asm | func_802BCBD8 (port/engine/77E20.c:406); func_802BCC48; func_802BCC48 (port/engine/77E20.c:421); func_802BCCD4; func_802BCCD4 (port/engine/77E20.c:433); func_802BCD20; func_802BCD20 (port/engine/77E20.c:446); func_802BCD80; func_802BCD80 (port/engine/77E20.c:461); func_802BE77C; func_802BE77C (port/engine/77E20.c:1268); func_802BEBB0; func_802BEBB0 (port/engine/77E20.c:1427) |  |
| `D_803F77E4` | `D_803EFED0+0x7914` | asm | func_802BCC10 (port/engine/77E20.c:412); func_802BCC48 (port/engine/77E20.c:425); func_802BCDE0; func_802BE77C (port/engine/77E20.c:1268); func_802BEBB0 |  |
| `D_803F77E8` | `D_803EFED0+0x7918` | asm | func_802BCC10 (port/engine/77E20.c:412); func_802BCC48; func_802BCC48 (port/engine/77E20.c:418); func_802BCDE0; func_802BCDE0 (port/engine/77E20.c:476); func_802BE77C; func_802BE77C (port/engine/77E20.c:1268); func_802BEBB0; func_802BEBB0 (port/engine/77E20.c:1429) |  |
| `D_803F77F4` | `D_803EFED0+0x7924` | asm | func_802BE77C (port/engine/77E20.c:1245); func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4 (port/engine/77E20.c:1534) |  |
| `D_803F77F8` | `D_803EFED0+0x7928` | asm | func_802A2D68 (port/engine/5CB60.c:237); func_802BE77C (port/engine/77E20.c:1245); func_802BEBB0 (port/engine/77E20.c:1416) |  |
| `D_803F77FC` | `D_803EFED0+0x792C` | asm | func_8029A800 (port/engine/56040.c:3165); func_8029B614 (port/engine/56040.c:2300); func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4 (port/engine/77E20.c:1524); func_802BFF6C |  |
| `D_803F77FE` | `D_803EFED0+0x792E` | asm | func_802BEBB0 (port/engine/77E20.c:1451); func_802BF898 (port/engine/77E20.c:1765); func_802BF978 (port/engine/77E20.c:1800) |  |
| `D_803F7800` | `D_803EFED0+0x7930` | asm | func_802BE77C (port/engine/77E20.c:1247); func_802BEBB0 (port/engine/77E20.c:1429); func_802BFEE4 (port/engine/77E20.c:0); func_802BFF6C (port/engine/77E20.c:1997) |  |
| `D_803F7801` | `D_803EFED0+0x7931` | asm | func_8029A800 (port/engine/56040.c:3165); func_802BE77C (port/engine/77E20.c:1247); func_802BFEE4 (port/engine/77E20.c:1924) |  |
| `D_803F7802` | `D_803EFED0+0x7932` | asm | func_802BE77C (port/engine/77E20.c:1242, 1273); func_802BEBB0 (port/engine/77E20.c:1489) |  |
| `D_803F7803` | `D_803EFED0+0x7933` | asm | func_802BE77C (port/engine/77E20.c:1243, 1274); func_802BEBB0 (port/engine/77E20.c:1478) |  |
| `D_803F7804` | `D_803EFED0+0x7934` | asm | func_802A2D68 (port/engine/5CB60.c:242); func_802B18F4 (port/engine/6C5E0.c:458, 464, 480, 497, 510); func_802BFF6C; func_802D1360 (port/engine/8AEE0.c:729, 735, 751, 767, 779) |  |
| `D_803F7805` | `D_803EFED0+0x7935` | asm | func_802A2D68 (port/engine/5CB60.c:229); func_802BC5E0 (port/engine/77E20.c:227, 228); func_802BEBB0 (port/engine/77E20.c:1434) |  |
| `D_803F7806` | `D_803EFED0+0x7936` | asm | func_802475D8 (blastcorps/src/hd_code/00000.c:1585); func_80263140 (blastcorps/src/hd_code/1D990.c:805); func_802A2D68 (port/engine/5CB60.c:230); func_802BC5E0 (port/engine/77E20.c:229); func_802BDDB4 (port/engine/77E20.c:1012) |  |
| `D_803F7807` | `D_803EFED0+0x7937` | asm | func_802BE77C (port/engine/77E20.c:1244, 1275); func_802BFF6C (port/engine/77E20.c:1988) |  |
| `D_803F7808` | `D_803EFED0+0x7938` | asm | func_80275478; func_802BCE40 (port/engine/77E20.c:550) |  |
| `D_803F7809` | `D_803EFED0+0x7939` | asm | func_80275478 (blastcorps/src/hd_code/30C70.c:183); func_802BD10C (port/engine/77E20.c:597) |  |
| `D_803F780A` | `D_803EFED0+0x793A` | asm | func_802A2D68 (port/engine/5CB60.c:238); func_802BEBB0; func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4; func_802BEFF4 (port/engine/77E20.c:0, 1552) |  |
| `D_803F780B` | `D_803EFED0+0x793B` | asm | func_802A2D68 (port/engine/5CB60.c:239); func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4 (port/engine/77E20.c:0, 1540, 1551, 1556) |  |
| `D_803F780C` | `D_803EFED0+0x793C` | asm | func_802A2D68 (port/engine/5CB60.c:240); func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4 (port/engine/77E20.c:0, 1553); func_802BFF6C (port/engine/77E20.c:1993); func_802C0284 (port/engine/77E20.c:2077, 2078) |  |
| `D_803F780D` | `D_803EFED0+0x793D` | asm | func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4 (port/engine/77E20.c:1544, 1547, 1557) |  |
| `D_803F780E` | `D_803EFED0+0x793E` | asm | func_802BEBB0; func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4; func_802BEFF4 (port/engine/77E20.c:1543) |  |
| `D_803F780F` | `D_803EFED0+0x793F` | asm | func_802BE77C (port/engine/77E20.c:1246); func_802BEBB0 (port/engine/77E20.c:1429); func_802BEFF4 (port/engine/77E20.c:1531, 1533) |  |
| `D_803F7810` | `D_803EFED0+0x7940` | asm | func_802A2D68 (port/engine/5CB60.c:241); func_802C0574; func_802C0CBC (port/engine/77E20.c:2363, 2380) |  |
| `D_803F7811` | `D_803EFED0+0x7941` | asm | func_8029A800 (port/engine/56040.c:3165) |  |
| `D_803F7820` | `D_803EFED0+0x7950` | asm | func_8024C414; func_802A1A9C (port/engine/5CB60.c:387); func_802C1F30 (port/engine/77E20.c:0) |  |
| `D_803F7824` | `D_803EFED0+0x7954` | asm | func_8024C414; func_802A1A9C (port/engine/5CB60.c:388); func_802C1F30 (port/engine/77E20.c:0) |  |
| `D_803F7828` | `D_803EFED0+0x7958` | asm | func_802A1A9C (port/engine/5CB60.c:394); func_802A9DC0 (port/engine/62740.c:1009); func_802C2054 (port/engine/77E20.c:2847) |  |
| `D_803F782C` | `D_803EFED0+0x795C` | asm | func_802A1A9C (port/engine/5CB60.c:420); func_802A9DC0 (port/engine/62740.c:1009) |  |
| `D_802ACFD0` | `hd_code_68810_bin+0x0` | asm | func_802ACF3C (port/engine/679E0.c:272); func_802ACF64 (port/engine/679E0.c:266) |  |
| `D_802AD880` | `hd_code_690C0_bin+0x0` | asm | func_802AD7D4 (port/engine/69014.c:37); func_802AD7FC (port/engine/69014.c:26) |  |
| `D_802AE084` | `hd_code_690C0_bin+0x804` | asm | func_802AD7D4 (port/engine/69014.c:37); func_802AD7FC (port/engine/69014.c:24) |  |
| `D_802C2190` | `hd_code_7D9D0_bin+0x0` | asm | func_80202380 (port/engine/1B100.c:102); func_802025D0 (port/engine/1B100.c:137); func_802B448C (port/engine/6E200.c:556, 557, 558, 559); func_802B4658 (port/engine/6E200.c:587); func_802B4EF8 (port/engine/6E200.c:724, 727, 732, 734) |  |
| `D_802C21A4` | `hd_code_7D9D0_bin+0x14` | asm | func_80202380; func_802025D0 (port/engine/1B100.c:138); func_802B448C (port/engine/6E200.c:560, 561, 562, 563); func_802B4658 (port/engine/6E200.c:588); func_802B4EF8 (port/engine/6E200.c:725, 728, 733, 735) |  |
| `D_802C21B8` | `hd_code_7D9D0_bin+0x28` | asm | func_80202380; func_802025D0 (port/engine/1B100.c:136); func_802B448C (port/engine/6E200.c:564, 565, 566, 567); func_802B4EF8 (port/engine/6E200.c:740) |  |
| `D_802C2208` | `hd_code_7D9D0_bin+0x78` | asm | func_80202380; func_802025D0 (port/engine/1B100.c:121); func_802B5CD8 (port/engine/71140.c:182, 183, 184, 185); func_802B6C28 (port/engine/71140.c:363) |  |
| `D_802C226C` | `hd_code_7D9D0_bin+0xDC` | asm | func_80202380; func_802025D0 (port/engine/1B100.c:122); func_802B5CD8 (port/engine/71140.c:186, 187, 188, 189); func_802B6C28 (port/engine/71140.c:364) |  |
| `D_802C22D0` | `hd_code_7D9D0_bin+0x140` | asm | func_802D0C68 (port/engine/8AEE0.c:464, 465, 466, 467) |  |
| `D_802C2308` | `hd_code_7D9D0_bin+0x178` | asm | func_802B1228 (port/engine/6C5E0.c:190, 191, 192) |  |
| `D_802C2314` | `hd_code_7D9D0_bin+0x184` | asm | func_802B2D7C (port/engine/6E200.c:176, 177, 178, 179); func_802B37B0 (port/engine/6E200.c:332, 345) |  |
| `D_802C2324` | `hd_code_7D9D0_bin+0x194` | asm | func_802CBA94 (port/engine/86F60.c:131, 132, 133, 134); func_802CC400 (port/engine/86F60.c:261) |  |
| `D_802C2348` | `hd_code_7D9D0_bin+0x1B8` | asm | func_802CBA94 (port/engine/86F60.c:135, 136, 137, 138); func_802CC400 (port/engine/86F60.c:262) |  |
| `D_802C236C` | `hd_code_7D9D0_bin+0x1DC` | asm | func_802B9C50 (port/engine/75490.c:250, 251, 252, 253) |  |
| `D_802C2390` | `hd_code_7D9D0_bin+0x200` | asm | func_802AFFD4 (port/engine/6B4A0.c:151, 152, 153, 154) |  |
| `D_802C23B4` | `hd_code_7D9D0_bin+0x224` | asm | func_8029DEA0; func_8029DEA0 (port/engine/56040.c:872); func_8029DF78; func_8029DF78 (port/engine/56040.c:1493) |  |
| `D_802C28E4` | `hd_code_7D9D0_bin+0x754` | asm | func_802C61F0 (port/engine/80280.c:880); func_802D2C20 (port/engine/8DDB0.c:193) |  |
| `D_802C2954` | `hd_code_7D9D0_bin+0x7C4` | asm | func_802B37B0 (port/engine/6E200.c:309, 310); func_802B4EF8 (port/engine/6E200.c:712, 717); func_802B6294 (port/engine/71140.c:284); func_802B7F98 (port/engine/72B80.c:323, 324, 325, 326); func_802CAAFC (port/engine/853D0.c:411); func_802CC400 (port/engine/86F60.c:270, 271); func_802CD578 (port/engine/88160.c:243, 244); func_802D02F8 (port/engine/8AEE0.c:320, 321, 322, 323) |  |
| `D_802C2984` | `hd_code_7D9D0_bin+0x7F4` | asm | func_8029B614 (port/engine/56040.c:2311); func_8029B994 (port/engine/56040.c:2065); func_802B18F4 (port/engine/6C5E0.c:458, 494, 507); func_802BC2C8 (port/engine/772A0.c:206, 207) |  |
| `D_802C2A5C` | `hd_code_7D9D0_bin+0x8CC` | asm | func_802AC544 (port/engine/679E0.c:109) |  |
| `D_802C37C0` | `hd_code_7D9D0_bin+0x1630` | asm | func_802B37B0 (port/engine/6E200.c:333) |  |
| `D_802C3804` | `hd_code_7D9D0_bin+0x1674` | asm | func_802C7544 (port/engine/80280.c:1085, 1086); func_802D2C20 (port/engine/8DDB0.c:201, 202, 203) |  |
| `D_802C382C` | `hd_code_7D9D0_bin+0x169C` | asm | func_802CDA10 (port/engine/89250.c:45) |  |
| `D_802C3B44` | `hd_code_7D9D0_bin+0x19B4` | asm | func_802BA9A0 (port/engine/75490.c:430) |  |
| `D_802C3FFC` | `hd_code_7D9D0_bin+0x1E6C` | asm | func_802AC3B8 (port/engine/679E0.c:97); func_802AC61C (port/engine/679E0.c:114); func_802AC6FC (port/engine/679E0.c:119); func_802C0574 (port/engine/77E20.c:2197) |  |
| `D_802C4A20` | `hd_code_800DC_bin+0x184` | asm | func_80288284 (blastcorps/src/hd_code/43A60.c:110) |  |
| `D_802D30D0` | `hd_code_8E910_bin+0x0` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3194` | `hd_code_8E910_bin+0xC4` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D32A0` | `hd_code_8E910_bin+0x1D0` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D331C` | `hd_code_8E910_bin+0x24C` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D33C8` | `hd_code_8E910_bin+0x2F8` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3444` | `hd_code_8E910_bin+0x374` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3538` | `hd_code_8E910_bin+0x468` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3614` | `hd_code_8E910_bin+0x544` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D36C0` | `hd_code_8E910_bin+0x5F0` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3784` | `hd_code_8E910_bin+0x6B4` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3890` | `hd_code_8E910_bin+0x7C0` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D393C` | `hd_code_8E910_bin+0x86C` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3A00` | `hd_code_8E910_bin+0x930` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3A4C` | `hd_code_8E910_bin+0x97C` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3BA0` | `hd_code_8E910_bin+0xAD0` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3BD4` | `hd_code_8E910_bin+0xB04` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3CE0` | `hd_code_8E910_bin+0xC10` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3DD4` | `hd_code_8E910_bin+0xD04` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3EB0` | `hd_code_8E910_bin+0xDE0` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802D3F74` | `hd_code_8E910_bin+0xEA4` | asm | func_802A1EC8 (port/engine/5CB60.c:0) |  |
| `D_802E53F0` | `hd_code_A0C30_bin+0x0` | asm | func_80284DB0 (blastcorps/src/hd_code/405F0.c:31, 33, 35) |  |
| `D_802E6820` | `hd_code_A0C30_bin+0x1430` | asm | func_80267CDC (blastcorps/src/hd_code/22EE0.c:223, 224); func_80284E54 (blastcorps/src/hd_code/405F0.c:52, 53); func_802A467C (port/engine/5FD50.c:336) |  |
| `D_802E68F0` | `hd_code_A0C30_bin+0x1500` | asm | func_80267CDC (blastcorps/src/hd_code/22EE0.c:224, 226); func_80284E54 (blastcorps/src/hd_code/405F0.c:53); func_802A467C (port/engine/5FD50.c:336) |  |
| `D_802E77B0` | `hd_code_A0C30_bin+0x23C0` | asm | func_802A467C (port/engine/5FD50.c:336); osDpSetStatus (port/src/ultra.c:599); osSpTaskStartGo (port/src/ultra.c:620) |  |
| `D_8030E390` | `hd_code_C9BD0_bin+0x0` | asm | func_80284DB0 (blastcorps/src/hd_code/405F0.c:32, 34, 36) |  |
| `D_8030EB90` | `hd_code_CA3D0_bin+0x0` | asm | func_80267CDC (blastcorps/src/hd_code/22EE0.c:227) |  |
| `D_8030EE60` | `hd_code_CA6A0_bin+0x0` | asm | func_802A467C (port/engine/5FD50.c:336) |  |

### Names the host reads inside other variables

The host names game variables by their N64 addresses (`PORT_VAR`, `n64_syms.h`); these are inside another variable.

| name | inside |
| --- | --- |
| `D_803156C0` | `D_80315440+0x280` |
| `D_803156C4` | `D_80315440+0x284` |
| `D_803F4030` | `D_803EFED0+0x4160` |

