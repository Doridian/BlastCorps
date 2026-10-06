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

Four check builds (`-DPORT_SCATTER_CHECK=ON`, 32-bit native-endian
movable), seeds 1 and 2, the asm data placed per label (`L1`, `L2`) and
per file (`F1`, `F2`); each ran the quick tier's five base scenarios
(`attract`, `auto1`, `auto2`, `auto3`, `attract.long`: 24,000 frames) and
the TAS (`tas`).  Then

```
port/tools/scatter_list.py --static build/sc1-mn32 \
    --run L1:attract build/sc1-mn32 .../attract/log.txt ...
```

The runs column counts the scenarios of each build that reached a row
(`+tas`: the TAS did too).  The check takes every access where the N64's
layout has what it meant and plays on, but not perfectly (a walk that
leaves the padding reads another variable), so the TAS parts from the
movie after a while in each: `F2` played 6 levels (6,769 of the log's
125,297 reads) and then hung; `F1` crashed in the renderer after 1.3
minutes (a display list into padding); `L1` stopped at
`port/engine/77E20.c:3473` (a pointer off the arena) after the first
level, `L2` at `77E20.c:3860` after 3.8 minutes.  What the TAS rows show
after that point (most of `L2`'s, from `func_802BFF6C` and
`func_802C1F30`) may be what the walks before them broke.  So the list is
what these runs reached: the first levels of the TAS, the attract mode's
demo levels and the menus, not every level.

Without the check, the scattered builds break where the list says they
would: the 64-bit big-endian movable build with seeds 1 and 2 differs from
the quick tier's references in all four base scenarios from frame 750 on
(the gameplay digest, the sound, every screenshot after 500), and the TAS
crashes after 1.1 and 1.8 minutes (a mode forced at the log's read 28,533
and 39,568 first).  The builds without `PORT_SCATTER` are unchanged.

This was made before the asm data became C (`port/tools/asm2c.py`).  The
`F` builds placed each data file whole, which no longer exists; and
since asm2c.py types the labels, a label inside a typed array or a file
struct is now a name inside a variable (an alias) where it was a label of
its own: 256 names, 241 in the asm data.  The sites are the same; run
the check again to have them as aliases.

## The kinds

- **alias**: a name the C declares as its own that is inside another
  variable (a field, the second byte of a word): in the scattered layout
  it has room of its own, and the container doesn't see what is written
  through it.
- **past end**: an access past the end of a C variable, into what follows
  it on the N64: a `T x[1]` or a `T x` that is really an array or a
  struct, a loop past an array's end.
- **label**: the same past an asm data label, into the next label of the
  same file: a table indexed or walked from its first label (the engine's
  per-wheel bytes `D_803ED3EA`..`EC`, a list's end pointer that is the
  next label's address).  Only per label; per file they are hidden.
- **before**: an access before a variable's start (a negative index, or
  a walk that started elsewhere).
- **address**: an N64 place a variable left, reached by an address made
  otherwise than from the variable.

"N64 layout has" is where the N64 has the byte the access reached (the
name there and the offset); "a walk: then N more" is a site that went on
through N more places (the first is the cause).

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
| alias | 9 |
| past end | 17 |
| label | 51 |
| before | 12 |
| address | 3 |

### alias

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `D_80364B80` | `D_80364AF0+0x90` | blastcorps/src/hd_code/53220.c:493, 503, 563 `func_802979E0` | load 1 | L2 1 +tas, F1 1 +tas, F2 1 +tas |
| `D_80364B80` | `D_80364AF0+0x90` | blastcorps/src/hd_code/53220.c:587 `func_80297EF8` | load 1 | L2 1 +tas, F1 1 +tas, F2 1 +tas |
| `D_80364B81` | `D_80364AF0+0x91` | blastcorps/src/hd_code/41930.c:43 `func_802860F0` | load 1 | L2 1 +tas, F1 1 +tas, F2 1 +tas |
| `D_80364BE0` | `D_80364AF0+0xF0` | blastcorps/src/hd_code/45BB0.c:206 `func_8028A470` | load 4 | L1 6 +tas, L2 6 +tas, F1 6 +tas, F2 6 +tas |
| `D_80370C00` | `D_80370BF8+0x8` | blastcorps/src/hd_front_end/E7B0.c:419 `func_801F5FE4` | load 4 | L1 4 +tas, L2 4 +tas, F1 4 +tas, F2 4 +tas |
| `D_80370C32` | `D_80370C30+0x2` | blastcorps/src/hd_code/17210.c:249 `func_8025BEF8` | store 1 | L1 6 +tas, L2 6 +tas, F1 6 +tas, F2 6 +tas |
| `D_80370C32` | `D_80370C30+0x2` | blastcorps/src/hd_code/51690.c:119 `func_80295EFC` | load 1 | L1 2, L2 3 +tas, F1 3 +tas, F2 3 +tas |
| `D_80370C33` | `D_80370C30+0x3` | blastcorps/src/hd_code/17210.c:250 `func_8025BEF8` | store 1 | L1 6 +tas, L2 6 +tas, F1 6 +tas, F2 6 +tas |
| `D_80370C33` | `D_80370C30+0x3` | blastcorps/src/hd_code/51690.c:119 `func_80295EFC` | load 1 | L1 2, L2 3 +tas, F1 3 +tas, F2 3 +tas |

### past end

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `D_802EB3F0` +0x770, +0x738 | `D_802EB8F0+0x270`, `D_802EB8F0+0x238` | blastcorps/src/hd_code/00000.c:3706, 3728 `func_802507C8` | load 2 | F2 1 +tas |
| `D_802F47B0` +0xC0, +0xC2, +0xC3, +0xC4, +0xC6, +0xC7 | `D_802F4870`, `D_802F4870+0x2`, `D_802F4870+0x3`, `D_802F4870+0x4`, ... (6) | blastcorps/src/hd_code/26570.c:3914, 3915, 3916, 3919, 3920, 3921 `func_8026BCE0` | load 1 | L2 1 +tas, F2 1 +tas |
| `D_802F499A` +0x53 | `D_802F48D0+0x11D (past the alias)` | blastcorps/src/hd_code/26570.c:3051 `func_8026AD30` | load 1 | L1 4 +tas, L2 4 +tas, F2 4 +tas |
| `D_802FC260` +0x2C, +0xC0 (a walk: then 57 more places) | `D_802FC280+0xC`, `D_802FC320` | blastcorps/src/hd_code/37530.c:160, 161 `func_8027BCF0` | load 2 | L1 2 +tas, L2 2 +tas, F1 2 +tas, F2 2 +tas |
| `D_80364BE0` +0xFC | `D_80364AF0+0x1EC (past the alias)` | blastcorps/src/hd_code/45BB0.c:206 `func_8028A470` | load 4 | L1 1, F1 1, F2 1 |
| `D_80365588` +0x5, +0x6, +0x8, +0x9, +0x7 (a walk: then 5 more places) | `D_8036558D`, `D_8036558E`, `D_80365590`, `D_80365590+0x1`, ... (5) | blastcorps/src/hd_code/17210.c:130, 131, 132, 133, 134 `func_8025BBE8` | store 1 | L1 3 +tas, L2 3 +tas, F1 3 +tas, F2 3 +tas |
| `D_80367BDC` +0xC | `D_80367BE0+0x8` | blastcorps/src/hd_code/1D990.c:1126 `func_80264264` | load 4 | L2 1 +tas, F1 1 +tas, F2 1 +tas |
| `D_8036B9A8` +0x20, +0x40, +0x60, +0x80 | `D_8036B9C8`, `D_8036B9E8`, `D_8036BA08`, `D_8036BA28` | blastcorps/src/hd_code/409D0.c:94, 95, 96, 98 `func_802852EC` | to n64_sprintf 1 | L2 1 +tas, F1 1 +tas, F2 1 +tas |
| `D_8036B9A8` +0x80 | `D_8036BA28` | blastcorps/src/hd_front_end/7800.c:183 `func_801EEDB4` | to n64_sprintf 1 | L2 1 +tas, F1 1 +tas, F2 1 +tas |
| `D_8036BA98` +0x4A | `D_8036BAA8+0x3A` | blastcorps/src/hd_code/26570.c:3276 `func_8026BA7C` | store 1 | L1 4 +tas, L2 4 +tas, F1 4 +tas |
| `D_8036BA98` +0x4A, +0xC, +0xE | `D_8036BAA8+0x3A`, `D_8036BAA2+0x2`, `D_8036BAA6` | blastcorps/src/hd_code/26570.c:0, 3821, 3823 `func_8026BCE0` | load 1, store 1 | L1 4 +tas, L2 4 +tas, F1 4 +tas, F2 1 +tas |
| `D_8036BAA2` +0x57 | `D_8036BAE8+0x11` | blastcorps/src/hd_code/26570.c:3053, 3057 `func_8026AD30` | load 1, store 1 | L1 4 +tas, L2 4 +tas, F1 4 +tas, F2 4 +tas |
| `D_8036BEF0` +0xC0 | `D_8036BF98+0x18` | blastcorps/src/hd_front_end/E7B0.c:692 `func_801F6CA4` | store 1 | F1 1 +tas |
| `D_8036C7D0` +0x40, +0x42, +0x44, +0x48, +0x4A, +0x50, ... (20) | `D_8036C810`, `D_8036C810+0x2`, `D_8036C810+0x4`, `D_8036C810+0x8`, ... (20) | blastcorps/src/hd_code/30C70.c:545, 546, 547, 548, 549, 550, 551, 552, ... (20) `func_80276E50` | store 2 | L1 1, L2 1, F1 1, F2 1 |
| `D_8036DCE0` +0x12, +0x14, +0xE, +0x10, +0xC, +0x16 | `D_8036DCF0+0x2`, `D_8036DCF0+0x4`, `D_8036DCEC+0x2`, `D_8036DCF0`, ... (6) | blastcorps/src/hd_code/39050.c:836, 837, 838, 839, 840, 841 `func_802807D8` | store 2 | L1 1, L2 2 +tas, F1 1, F2 1 |
| `D_8036DCE0` +0xE, +0x10, +0x12, +0x14, +0x6E | `D_8036DCEC+0x2`, `D_8036DCF0`, `D_8036DCF0+0x2`, `D_8036DCF0+0x4`, ... (5) | blastcorps/src/hd_code/39050.c:0, 887, 890, 891, 908, 909 `func_80280F34` | load 2, store 2 | L2 1 +tas |
| `D_8039B6B0` +0xFC (a walk: then 4 more places) | `D_8039B628+0x184 (past the alias)` | blastcorps/src/hd_front_end/E7B0.c:556 `func_801F65C4` | store 1 | L2 1 +tas, F1 1 +tas, F2 1 +tas |

### label

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `D_80301E7C` +0xB4 | `D_80301F28+0x8` | blastcorps/src/hd_code/00000.c:3728 `func_802507C8` | load 2 | L1 1 +tas |
| `D_80305E50` +0x4E2, +0x4E4 (a walk: then 506 more places) | `D_8030631F+0x13`, `D_8030631F+0x15` | port/engine/77E20.c:2699, 2700 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_803062F1` +0x18F | `hd_code_C1CC0_data__s` | blastcorps/src/hd_front_end/E7B0.c:692 `func_801F6CA4` | store 1 | L2 1 +tas |
| `D_803063E0` +0x10, +0x1D | `hd_code_80280_data__s`, `hd_code_80280_data__s+0xD` | port/engine/77E20.c:2414, 2417 `func_802BF978` | load 1 | L2 1 +tas |
| `D_80306E70` +0x9B7 | `hd_code_C3030_data__s+0x37` | port/engine/77E20.c:2759 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_803A6B30` +0x7F7 | `D_803A7300+0x27` | port/engine/77E20.c:2759 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_803B7FC8` +0x5A4 (a walk: then 1260 more places) | `D_803B8568+0x4` | port/engine/56040.c:1661 `func_8029E5AC` | load 4 | L2 6 +tas |
| `D_803B7FC8` +0x81B | `D_803B8570+0x273` | port/engine/77E20.c:2759 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_803BE740` +0x54, +0x58 | `D_803BE780+0x14`, `D_803BE780+0x18` | blastcorps/src/hd_code/2C560.c:0, 385 `__scTaskComplete` | load 4 | L1 6 +tas, L2 6 +tas |
| `D_803BE740` +0x48 | `D_803BE780+0x8` | port/engine/5FD50.c:424 `func_802A467C` | store 4 | L1 6 +tas, L2 6 +tas |
| `D_803BE740` +0x40, +0x44 | `D_803BE780`, `D_803BE780+0x4` | port/src/ultra.c:0, 622 `osSpTaskStartGo` | load 4 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x4 | `D_803ED394` | port/engine/62740.c:2140 `func_802A8768` | store 2 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x4 | `D_803ED394` | port/engine/62740.c:1279 `func_802A9A60` | store 2 | L2 1 +tas |
| `D_803ED390` +0x4, +0x2 | `D_803ED394`, `D_803ED392` | port/engine/62740.c:467, 469 `func_802AA764` | load 2 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x4 | `D_803ED394` | port/engine/69BB0.c:380 `driver_frame` | store 2 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x4 | `D_803ED394` | port/engine/69BB0.c:309 `func_802AEC3C` | store 2 | L2 1 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/6B4A0.c:228 `func_802B03F4` | store 2 | L1 1, L2 2 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/6C5E0.c:267 `func_802B152C` | store 2 | L1 1, L2 1 |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/6E200.c:698, 710 `ramdozer_frame` | store 2 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/6E200.c:279 `skyfall_frame` | store 2 | L1 1, L2 1 |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/71140.c:303, 316 `func_802B6294` | store 2 | L1 2, L2 3 +tas |
| `D_803ED390` +0x4 | `D_803ED394` | port/engine/72B80.c:503 `chopper_frame` | store 2 | L1 4 +tas, L2 4 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/72B80.c:312 `func_802B7A88` | store 2 | L1 4 +tas, L2 4 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/75490.c:250 `func_802B9C50` | store 2 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/75490.c:349 `func_802BA354` | store 2 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x4, +0x2 | `D_803ED394`, `D_803ED392` | port/engine/772A0.c:233, 234 `func_802BC3D0` | store 2 | L1 6 +tas, L2 6 +tas |
| `D_803ED390` +0x4, +0x2 | `D_803ED394`, `D_803ED392` | port/engine/80280.c:1258, 1259 `func_802C7CB0` | store 2 | L1 2, L2 3 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/853D0.c:298 `func_802CA4E0` | store 2 | L1 2, L2 3 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/86F60.c:236, 248 `func_802CBEF0` | store 2 | L1 2, L2 3 +tas |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/88160.c:223 `func_802CD068` | store 2 | L1 1, L2 1 |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/8AEE0.c:306 `func_802CFDE8` | store 2 | L1 1, L2 1 |
| `D_803ED390` +0x2 | `D_803ED392` | port/engine/8AEE0.c:561 `func_802D0F98` | store 2 | L2 1 +tas |
| `D_803ED398` +0x4 | `D_803ED39C` | port/engine/62740.c:2130 `func_802A8768` | load 4 | L1 6 +tas, L2 6 +tas |
| `D_803ED398` +0x4 | `D_803ED39C` | port/engine/62740.c:1320 `func_802A93B0` | store 4 | L1 6 +tas, L2 6 +tas |
| `D_803ED398` +0x8, +0x4 | `D_803ED3A0`, `D_803ED39C` | port/engine/62740.c:1862, 1865 `func_802A95A4` | store 4 | L1 3 +tas, L2 3 +tas |
| `D_803ED3A8` +0x4 | `D_803ED3AC` | port/engine/62740.c:2144 `func_802A8768` | load 4 | L1 6 +tas, L2 6 +tas |
| `D_803ED3A8` +0x4, +0x8 | `D_803ED3AC`, `D_803ED3B0` | port/engine/62740.c:1756, 1757 `func_802A8B10` | load 4 | L1 6 +tas, L2 6 +tas |
| `D_803ED3A8` +0x4 | `D_803ED3AC` | port/engine/62740.c:1200 `func_802A9B1C` | store 4 | L1 6 +tas, L2 6 +tas |
| `D_803ED3A8` +0x4 | `D_803ED3AC` | port/engine/69BB0.c:427 `func_802AF340` | load 4 | L2 1 +tas |
| `D_803ED3EA` +0x1 | `D_803ED3EB` | port/engine/62740.c:1247 `func_802A992C` | store 1 | L1 6 +tas, L2 6 +tas |
| `D_803ED3EA` +0x1 | `D_803ED3EB` | port/engine/62740.c:1196 `func_802A9B1C` | store 1 | L1 6 +tas, L2 6 +tas |
| `D_803ED3EA` +0xD, +0xC | `D_803ED3F7`, `D_803ED3F6` | port/engine/77E20.c:2754, 2755 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_803ED3EE` +0x1, +0x2 | `D_803ED3EF`, `D_803ED3EF+0x1` | port/engine/62740.c:2118, 2129 `func_802A8768` | load 1, store 1 | L1 6 +tas, L2 6 +tas |
| `D_803ED3EE` +0x1 | `D_803ED3EF` | port/engine/62740.c:0 `func_802A93B0` | store 1 | L1 6 +tas, L2 6 +tas |
| `D_803ED3EE` +0x2, +0x1 | `D_803ED3EF+0x1`, `D_803ED3EF` | port/engine/62740.c:1866 `func_802A95A4` | store 1 | L1 3 +tas, L2 3 +tas |
| `D_803ED3EE` +0x2, +0x1 | `D_803ED3EF+0x1`, `D_803ED3EF` | port/engine/62740.c:0 `func_802A9710` | store 1 | L1 3 +tas, L2 3 +tas |
| `D_803ED3F2` +0x1 | `D_803ED3F3` | port/engine/62740.c:1249 `func_802A992C` | store 1 | L1 6 +tas, L2 6 +tas |
| `D_803ED3F2` +0x1 | `D_803ED3F3` | port/engine/62740.c:1274 `func_802A9A60` | store 1 | L2 1 +tas |
| `D_803ED3F2` +0x1 | `D_803ED3F3` | port/engine/62740.c:1197 `func_802A9B1C` | store 1 | L1 6 +tas, L2 6 +tas |
| `D_803EE768` +0x4, +0x8 (a walk: then 5 more places) | `D_803EE76C`, `D_803EE770` | port/engine/679E0.c:155, 156 `func_802AC85C` | store 4 | L1 6 +tas, L2 6 +tas |
| `D_803EF2EC` +0x8 | `D_803EF2F4` | port/engine/60D50.c:75 `func_802A5604` | load 4 | L1 4 +tas, L2 4 +tas |

### before

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `D_802FAD50` -0x2E | `D_802FA940+0x3E2` | blastcorps/src/hd_code/26570.c:3276 `func_8026BA7C` | store 1 | F2 4 +tas |
| `D_802FAD50` -0x2E | `D_802FA940+0x3E2` | blastcorps/src/hd_code/26570.c:3823 `func_8026BCE0` | load 1 | F2 4 +tas |
| `D_802FDB10` -0x40 | `D_802FDAC0+0x10` | blastcorps/src/hd_code/45BB0.c:206 `func_8028A470` | load 4 | L2 1 |
| `D_80301B30` -0xB0 | `?` | blastcorps/src/hd_code/45BB0.c:206 `func_8028A470` | load 4 | L2 1 |
| `D_8030222C` -0xFB | `D_80302124+0xD` | port/engine/77E20.c:2770 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_80305E00` -0x11D | `hd_code_6B4A0_data__s+0x3` | port/engine/77E20.c:2770 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_8030630B` -0xEC | `D_80305E50+0x3CF` | port/engine/77E20.c:2770 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_803077F0` -0x309 | `hd_code_C26B0_data__s+0x677` | port/engine/77E20.c:2759 `func_802BFF6C` | load 1 | L2 1 +tas |
| `D_80364A72` -0x28, -0x22, -0x20, -0x1E, -0x1A, -0x18 | `D_80364A4A`, `D_80364A50`, `D_80364A50+0x2`, `D_80364A54`, ... (6) | blastcorps/src/hd_code/30C70.c:559, 560, 561, 562, 563, 564 `func_80276E50` | store 2 | F1 1 |
| `D_8036BB30` -0x8E, -0x7E, -0xA8, -0x98, -0x88, -0x78, ... (10) | `D_8036BAA2`, `D_8036BAA8+0xA`, `D_8036BA48+0x40`, `D_8036BA98`, ... (10) | blastcorps/src/hd_code/2D810.c:137, 138, 167, 168, 169, 170, 173, 174, ... (10) `func_80271FD0` | store 2 | F1 1 +tas |
| `D_80370C72` -0x4F | `D_80370C23` | blastcorps/src/hd_code/26570.c:3051 `func_8026AD30` | load 1 | F1 4 +tas |
| `D_803EEB38` -0x67, -0x66, -0x65 (a walk: then 134 more places) | `D_803EEA90+0x41`, `D_803EEA90+0x42`, `D_803EEA90+0x43` | port/engine/77E20.c:0, 2702, 2703 `func_802BFF6C` | load 1 | L2 1 +tas |

### address

| symbol | N64 layout has | site | access | runs |
| --- | --- | --- | --- | --- |
| `hd_code_68810_bin` (a walk: then 3361 more places) | `hd_code_68810_bin+0xC`, `hd_code_68810_bin+0x1C`, `hd_code_68810_bin+0x2C` | port/engine/77E20.c:3850, 3855, 3860 `func_802C1F30` | load 4 | L2 1 +tas |
| `hd_code_690C0_bin` (a walk: then 215 more places) | `hd_code_690C0_bin+0x0` | port/engine/77E20.c:3862 `func_802C1F30` | store 4 | L2 1 +tas |
| `hd_code_8E910_bin` (a walk: then 203 more places) | `hd_code_8E910_bin+0x64`, `hd_code_8E910_bin+0xDC` | port/engine/77E20.c:3852, 3857 `func_802C1F30` | store 4 | L2 1 +tas |

### Names inside other variables (static)

277 names the N64 side declares that are inside another variable (port-arena's report), whether a run reached them or not; `seen` marks those a run did.

| name | inside | C or asm | users | seen |
| --- | --- | --- | --- | --- |
| `D_802F499A` | `D_802F48D0+0xCA` | c | func_8026AD30 |  |
| `D_802F8BF4` | `D_802F8BDC+0x18` | c | func_802507C8 (blastcorps/src/hd_code/00000.c:3706, 3717, 3728) |  |
| `D_80301048` | `D_80300A68+0x5E0` | asm | initializer of D_8020C070 |  |
| `D_80301060` | `D_80300A68+0x5F8` | asm | initializer of D_8020C070 |  |
| `D_80301070` | `D_80300A68+0x608` | asm | initializer of D_8020C0C1 |  |
| `D_80301080` | `D_80300A68+0x618` | asm | func_801F6F18 (blastcorps/src/hd_front_end/E7B0.c:799) |  |
| `D_803010D0` | `D_803010C0+0x10` | asm | initializer of D_8020C488 |  |
| `D_803010E0` | `D_803010C0+0x20` | asm | initializer of D_8020C488 |  |
| `D_803010F8` | `D_803010C0+0x38` | asm | initializer of D_8020C488 |  |
| `D_80301108` | `D_803010C0+0x48` | asm | initializer of D_8020C488 |  |
| `D_8030111C` | `D_803010C0+0x5C` | asm | initializer of D_8020C488 |  |
| `D_80301138` | `D_803010C0+0x78` | asm | initializer of D_8020C488 |  |
| `D_80301148` | `D_803010C0+0x88` | asm | initializer of D_8020C488 |  |
| `D_8030115C` | `D_803010C0+0x9C` | asm | initializer of D_8020C488 |  |
| `D_80301174` | `D_803010C0+0xB4` | asm | initializer of D_8020C488 |  |
| `D_8030118C` | `D_803010C0+0xCC` | asm | initializer of D_8020C488 |  |
| `D_803011A8` | `D_803010C0+0xE8` | asm | initializer of D_8020C488 |  |
| `D_803011B8` | `D_803010C0+0xF8` | asm | initializer of D_8020C488 |  |
| `D_803011BC` | `D_803010C0+0xFC` | asm | initializer of D_8020C488 |  |
| `D_803011D8` | `D_803010C0+0x118` | asm | initializer of D_8020C488 |  |
| `D_803011F4` | `D_803010C0+0x134` | asm | initializer of D_8020C488 |  |
| `D_80301204` | `D_803010C0+0x144` | asm | initializer of D_8020C488 |  |
| `D_80301208` | `D_803010C0+0x148` | asm | initializer of D_8020C488 |  |
| `D_80301228` | `D_803010C0+0x168` | asm | initializer of D_8020C488 |  |
| `D_80301244` | `D_803010C0+0x184` | asm | initializer of D_8020C488 |  |
| `D_80301258` | `D_803010C0+0x198` | asm | initializer of D_8020C488 |  |
| `D_8030125C` | `D_803010C0+0x19C` | asm | initializer of D_8020C488 |  |
| `D_80301260` | `D_803010C0+0x1A0` | asm | initializer of D_8020C488 |  |
| `D_80301270` | `D_803010C0+0x1B0` | asm | initializer of D_8020C488 |  |
| `D_80301284` | `D_803010C0+0x1C4` | asm | initializer of D_8020C488 |  |
| `D_80301288` | `D_803010C0+0x1C8` | asm | initializer of D_8020C488 |  |
| `D_8030128C` | `D_803010C0+0x1CC` | asm | initializer of D_8020C488 |  |
| `D_80301290` | `D_803010C0+0x1D0` | asm | initializer of D_8020C488 |  |
| `D_803012A8` | `D_803010C0+0x1E8` | asm | initializer of D_8020C488 |  |
| `D_803012BC` | `D_803010C0+0x1FC` | asm | initializer of D_8020C488 |  |
| `D_803012C0` | `D_803010C0+0x200` | asm | initializer of D_8020C488 |  |
| `D_803012C4` | `D_803010C0+0x204` | asm | initializer of D_8020C488 |  |
| `D_803012DC` | `D_803010C0+0x21C` | asm | initializer of D_8020C488 |  |
| `D_803012F8` | `D_803010C0+0x238` | asm | initializer of D_8020C488 |  |
| `D_80301310` | `D_803010C0+0x250` | asm | initializer of D_8020C488 |  |
| `D_80301324` | `D_803010C0+0x264` | asm | initializer of D_8020C488 |  |
| `D_80301338` | `D_803010C0+0x278` | asm | initializer of D_8020C488 |  |
| `D_80301354` | `D_803010C0+0x294` | asm | initializer of D_8020C488 |  |
| `D_80301368` | `D_803010C0+0x2A8` | asm | initializer of D_8020C488 |  |
| `D_80301380` | `D_803010C0+0x2C0` | asm | initializer of D_8020C488 |  |
| `D_80301384` | `D_803010C0+0x2C4` | asm | initializer of D_8020C488 |  |
| `D_80301388` | `D_803010C0+0x2C8` | asm | initializer of D_8020C488 |  |
| `D_8030138C` | `D_803010C0+0x2CC` | asm | initializer of D_8020C488 |  |
| `D_80301390` | `D_803010C0+0x2D0` | asm | initializer of D_8020C488 |  |
| `D_80301394` | `D_803010C0+0x2D4` | asm | initializer of D_8020C488 |  |
| `D_803013AC` | `D_803010C0+0x2EC` | asm | initializer of D_8020C488 |  |
| `D_803013C4` | `D_803010C0+0x304` | asm | initializer of D_8020C488 |  |
| `D_803013E0` | `D_803010C0+0x320` | asm | initializer of D_8020C488 |  |
| `D_803013FC` | `D_803010C0+0x33C` | asm | initializer of D_8020C488 |  |
| `D_80301400` | `D_803010C0+0x340` | asm | initializer of D_8020C488 |  |
| `D_80301404` | `D_803010C0+0x344` | asm | initializer of D_8020C488 |  |
| `D_80301408` | `D_803010C0+0x348` | asm | initializer of D_8020C488 |  |
| `D_8030140C` | `D_803010C0+0x34C` | asm | initializer of D_8020C488 |  |
| `D_8030142C` | `D_803010C0+0x36C` | asm | initializer of D_8020C488 |  |
| `D_8030144C` | `D_803010C0+0x38C` | asm | initializer of D_8020C488 |  |
| `D_80301468` | `D_803010C0+0x3A8` | asm | initializer of D_8020C488 |  |
| `D_8030147C` | `D_803010C0+0x3BC` | asm | initializer of D_8020C488 |  |
| `D_80301480` | `D_803010C0+0x3C0` | asm | initializer of D_8020C488 |  |
| `D_80301484` | `D_803010C0+0x3C4` | asm | initializer of D_8020C488 |  |
| `D_80301794` | `D_80301790+0x4` | asm | initializer of D_8020C488 |  |
| `D_803017A4` | `D_80301790+0x14` | asm | initializer of D_8020C488 |  |
| `D_803017C0` | `D_80301790+0x30` | asm | initializer of D_8020C488 |  |
| `D_803017C4` | `D_80301790+0x34` | asm | initializer of D_8020C488 |  |
| `D_803017E0` | `D_80301790+0x50` | asm | initializer of D_8020C488 |  |
| `D_803017F0` | `D_80301790+0x60` | asm | initializer of D_8020C488 |  |
| `D_80301808` | `D_80301790+0x78` | asm | initializer of D_8020C488 |  |
| `D_80301820` | `D_80301790+0x90` | asm | initializer of D_8020C488 |  |
| `D_80301824` | `D_80301790+0x94` | asm | initializer of D_8020C488 |  |
| `D_80301828` | `D_80301790+0x98` | asm | initializer of D_8020C488 |  |
| `D_8030182C` | `D_80301790+0x9C` | asm | initializer of D_8020C488 |  |
| `D_80301830` | `D_80301790+0xA0` | asm | initializer of D_8020C488 |  |
| `D_8030183C` | `D_80301790+0xAC` | asm | initializer of D_8020C488 |  |
| `D_80301854` | `D_80301790+0xC4` | asm | initializer of D_8020C488 |  |
| `D_8030186C` | `D_80301790+0xDC` | asm | initializer of D_8020C488 |  |
| `D_80301870` | `D_80301790+0xE0` | asm | initializer of D_8020C488 |  |
| `D_80301874` | `D_80301790+0xE4` | asm | initializer of D_8020C488 |  |
| `D_80301878` | `D_80301790+0xE8` | asm | initializer of D_8020C488 |  |
| `D_8030187C` | `D_80301790+0xEC` | asm | initializer of D_8020C488 |  |
| `D_80301898` | `D_80301790+0x108` | asm | initializer of D_8020C488 |  |
| `D_803018B4` | `D_80301790+0x124` | asm | initializer of D_8020C488 |  |
| `D_803018C0` | `D_80301790+0x130` | asm | initializer of D_8020C488 |  |
| `D_803018D8` | `D_80301790+0x148` | asm | initializer of D_8020C488 |  |
| `D_803018EC` | `D_80301790+0x15C` | asm | initializer of D_8020C488 |  |
| `D_803018F0` | `D_80301790+0x160` | asm | initializer of D_8020C488 |  |
| `D_80301900` | `D_80301790+0x170` | asm | initializer of D_8020C488 |  |
| `D_8030191C` | `D_80301790+0x18C` | asm | initializer of D_8020C488 |  |
| `D_80301934` | `D_80301790+0x1A4` | asm | initializer of D_8020C488 |  |
| `D_80301948` | `D_80301790+0x1B8` | asm | initializer of D_8020C488 |  |
| `D_8030194C` | `D_80301790+0x1BC` | asm | initializer of D_8020C488 |  |
| `D_80301950` | `D_80301790+0x1C0` | asm | initializer of D_8020C488 |  |
| `D_80301954` | `D_80301790+0x1C4` | asm | initializer of D_8020C488 |  |
| `D_80301958` | `D_80301790+0x1C8` | asm | initializer of D_8020C488 |  |
| `D_8030196C` | `D_80301790+0x1DC` | asm | initializer of D_8020C488 |  |
| `D_80301984` | `D_80301790+0x1F4` | asm | initializer of D_8020C488 |  |
| `D_80301998` | `D_80301790+0x208` | asm | initializer of D_8020C488 |  |
| `D_803019B0` | `D_80301790+0x220` | asm | initializer of D_8020C488 |  |
| `D_803019C4` | `D_80301790+0x234` | asm | initializer of D_8020C488 |  |
| `D_803019C8` | `D_80301790+0x238` | asm | initializer of D_8020C488 |  |
| `D_803019CC` | `D_80301790+0x23C` | asm | initializer of D_8020C488 |  |
| `D_803019D0` | `D_80301790+0x240` | asm | initializer of D_8020C488 |  |
| `D_803019D4` | `D_80301790+0x244` | asm | initializer of D_8020C488 |  |
| `D_803019D8` | `D_80301790+0x248` | asm | initializer of D_8020C488 |  |
| `D_803019F0` | `D_80301790+0x260` | asm | initializer of D_8020C488 |  |
| `D_80301A10` | `D_80301790+0x280` | asm | initializer of D_8020C488 |  |
| `D_80301A28` | `D_80301790+0x298` | asm | initializer of D_8020C488 |  |
| `D_80301A3C` | `D_80301790+0x2AC` | asm | initializer of D_8020C488 |  |
| `D_80301A50` | `D_80301790+0x2C0` | asm | initializer of D_8020C488 |  |
| `D_80301A6C` | `D_80301790+0x2DC` | asm | initializer of D_8020C488 |  |
| `D_80301A70` | `D_80301790+0x2E0` | asm | initializer of D_8020C488 |  |
| `D_80301A74` | `D_80301790+0x2E4` | asm | initializer of D_8020C488 |  |
| `D_80301A78` | `D_80301790+0x2E8` | asm | initializer of D_8020C488 |  |
| `D_80301A7C` | `D_80301790+0x2EC` | asm | initializer of D_8020C488 |  |
| `D_80301A80` | `D_80301790+0x2F0` | asm | initializer of D_8020C488 |  |
| `D_80301AA0` | `D_80301790+0x310` | asm | initializer of D_8020C488 |  |
| `D_80301AB8` | `D_80301790+0x328` | asm | initializer of D_8020C488 |  |
| `D_80301ABC` | `D_80301790+0x32C` | asm | initializer of D_8020C488 |  |
| `D_80301AC0` | `D_80301790+0x330` | asm | initializer of D_8020C488 |  |
| `D_80301AC4` | `D_80301790+0x334` | asm | initializer of D_8020C488 |  |
| `D_80301AC8` | `D_80301790+0x338` | asm | initializer of D_8020C488 |  |
| `D_80301ACC` | `D_80301790+0x33C` | asm | initializer of D_8020C488 |  |
| `D_80301AD0` | `D_80301790+0x340` | asm | initializer of D_8020C488 |  |
| `D_80301AD4` | `D_80301790+0x344` | asm | initializer of D_8020C488 |  |
| `D_80301AE4` | `D_80301790+0x354` | asm | initializer of D_8020C488 |  |
| `D_80301AFC` | `D_80301790+0x36C` | asm | initializer of D_8020C488 |  |
| `D_80301B14` | `D_80301790+0x384` | asm | initializer of D_8020C488 |  |
| `D_80301B2C` | `D_80301790+0x39C` | asm | initializer of D_8020C488 |  |
| `D_80301B30` | `D_80301790+0x3A0` | asm | initializer of D_8020C488 |  |
| `D_80301B48` | `D_80301790+0x3B8` | asm | initializer of D_8020C488 |  |
| `D_80301B5C` | `D_80301790+0x3CC` | asm | initializer of D_8020D530 |  |
| `D_80301B78` | `D_80301790+0x3E8` | asm | initializer of D_8020D530 |  |
| `D_803035D0` | `D_803035C8+0x8` | asm | initializer of D_8020D530 |  |
| `D_803035E4` | `D_803035C8+0x18` | asm | initializer of D_8020D530 |  |
| `D_803035F8` | `D_803035C8+0x2C` | asm | initializer of D_8020D530 |  |
| `D_8030360C` | `D_803035C8+0x40` | asm | initializer of D_8020D530 |  |
| `D_80303624` | `D_803035C8+0x58` | asm | initializer of D_8020D530 |  |
| `D_80303638` | `D_803035C8+0x6C` | asm | initializer of D_8020D530 |  |
| `D_80303644` | `D_803035C8+0x78` | asm | initializer of D_8020D530 |  |
| `D_8030365C` | `D_803035C8+0x90` | asm | initializer of D_8020D530 |  |
| `D_80303674` | `D_803035C8+0xA8` | asm | initializer of D_8020D530 |  |
| `D_80303684` | `D_803035C8+0xB8` | asm | initializer of D_8020D530 |  |
| `D_80303698` | `D_803035C8+0xCC` | asm | initializer of D_8020D530 |  |
| `D_803036B0` | `D_803035C8+0xE4` | asm | initializer of D_8020D530 |  |
| `D_803036BC` | `D_803035C8+0xF0` | asm | initializer of D_8020D530 |  |
| `D_803036D4` | `D_803035C8+0x108` | asm | initializer of D_8020D530 |  |
| `D_803036E8` | `D_803035C8+0x11C` | asm | initializer of D_8020C0C1 |  |
| `D_803036F8` | `D_803035C8+0x12C` | asm | initializer of D_8020C0C1 |  |
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
| `D_80304730` | `D_803041B4+0x57C` | asm | func_801EA6E8 (blastcorps/src/hd_front_end/1C40.c:928) |  |
| `D_803048CC` | `D_8030480C+0xC0` | asm | initializer of D_8020E430 |  |
| `D_803048D8` | `D_8030480C+0xCC` | asm | initializer of D_8020E430 |  |
| `D_803048E4` | `D_8030480C+0xD8` | asm | initializer of D_8020E430 |  |
| `D_803048F0` | `D_8030480C+0xE4` | asm | initializer of D_8020E430 |  |
| `D_80304904` | `D_8030480C+0xF8` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:3233) |  |
| `D_80304910` | `D_8030480C+0x104` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:3235) |  |
| `D_8030491C` | `D_8030480C+0x110` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:3226) |  |
| `D_80304938` | `D_8030480C+0x12C` | asm | func_8024C414 (blastcorps/src/hd_code/00000.c:3229) |  |
| `D_80304954` | `D_8030480C+0x148` | asm | initializer of D_80208044 |  |
| `D_80364AF8` | `D_80364AF0+0x8` | c | func_80286330 (blastcorps/src/hd_code/41930.c:102) |  |
| `D_80364B80` | `D_80364AF0+0x90` | c | func_802860F0 (blastcorps/src/hd_code/41930.c:54); func_802979E0 (blastcorps/src/hd_code/53220.c:493, 495, 503, 563); func_80297EF8 (blastcorps/src/hd_code/53220.c:587); func_80297F74 (blastcorps/src/hd_code/53220.c:595) | seen |
| `D_80364B81` | `D_80364AF0+0x91` | c | func_802860F0 (blastcorps/src/hd_code/41930.c:43, 46, 47, 71); func_802862DC (blastcorps/src/hd_code/41930.c:78); func_80286330 (blastcorps/src/hd_code/41930.c:88) | seen |
| `D_80364BE0` | `D_80364AF0+0xF0` | c | func_8028A470 (blastcorps/src/hd_code/45BB0.c:206); func_8028B240 (blastcorps/src/hd_code/45BB0.c:515) | seen |
| `D_8036BF90` | `D_8036BF78+0x18` | c | __scMain (blastcorps/src/hd_code/2C560.c:172) |  |
| `D_8036BF94` | `D_8036BF78+0x1C` | c | __scMain (blastcorps/src/hd_code/2C560.c:172) |  |
| `D_80370C00` | `D_80370BF8+0x8` | c | func_801F5FE4 (blastcorps/src/hd_front_end/E7B0.c:419) | seen |
| `D_80370C32` | `D_80370C30+0x2` | c | func_8025BEF8 (blastcorps/src/hd_code/17210.c:249); func_80295EFC (blastcorps/src/hd_code/51690.c:119) | seen |
| `D_80370C33` | `D_80370C30+0x3` | c | func_8025BEF8 (blastcorps/src/hd_code/17210.c:250); func_80295EFC (blastcorps/src/hd_code/51690.c:119) | seen |
| `D_8039B630` | `D_8039B628+0x8` | c | func_801F58E8 (blastcorps/src/hd_front_end/E7B0.c:226, 235, 262, 275, 281, 284, 321, 347); func_801F60C8 (blastcorps/src/hd_front_end/E7B0.c:449); func_801F6160 (blastcorps/src/hd_front_end/E7B0.c:461); func_801F61C8 (blastcorps/src/hd_front_end/E7B0.c:465); func_801F6210 (blastcorps/src/hd_front_end/E7B0.c:471); func_801F6264 (blastcorps/src/hd_front_end/E7B0.c:517, 523, 532); func_801F65C4 (blastcorps/src/hd_front_end/E7B0.c:562, 564); func_801F6AF4 (blastcorps/src/hd_front_end/E7B0.c:621); func_801F6BD0 (blastcorps/src/hd_front_end/E7B0.c:664); func_801F6CA4 (blastcorps/src/hd_front_end/E7B0.c:698, 701); func_801F6ED4 (blastcorps/src/hd_front_end/E7B0.c:711) |  |
| `D_8039B698` | `D_8039B628+0x70` | c | func_801F58E8; func_801F6160 (blastcorps/src/hd_front_end/E7B0.c:461); func_801F6264; func_801F6264 (blastcorps/src/hd_front_end/E7B0.c:532); func_801F65C4 (blastcorps/src/hd_front_end/E7B0.c:562); func_801F6AF4 (blastcorps/src/hd_front_end/E7B0.c:621); func_801F6BD0 (blastcorps/src/hd_front_end/E7B0.c:664); func_801F6CA4 (blastcorps/src/hd_front_end/E7B0.c:698) |  |
| `D_8039B6B0` | `D_8039B628+0x88` | c | func_801F6264 (blastcorps/src/hd_front_end/E7B0.c:0); func_801F65C4 (blastcorps/src/hd_front_end/E7B0.c:556, 558); func_801F6CA4 (blastcorps/src/hd_front_end/E7B0.c:692, 694) |  |
| `D_8039C538` | `D_8039C4F8+0x40` | c | func_801EA278; func_801EA278 (blastcorps/src/hd_front_end/1C40.c:836); func_801F58E8 (blastcorps/src/hd_front_end/E7B0.c:226, 341); func_801F60C8 (blastcorps/src/hd_front_end/E7B0.c:442) |  |

### Names that go with what they are inside

93 names inside a bin (no labels to split it by), another name of an asm object's start, or (PORT_SCATTER_ASM=file) inside a data file: they move with it, so the scattered layout doesn't test them.

| name | inside | users |
| --- | --- | --- |
| `D_802ACFD0` | `hd_code_68810_bin+0x0` | func_802ACF3C (port/engine/679E0.c:290); func_802ACF64 (port/engine/679E0.c:283) |
| `D_802AD880` | `hd_code_690C0_bin+0x0` | func_802AD7D4 (port/engine/69014.c:42); func_802AD7FC (port/engine/69014.c:29) |
| `D_802AE084` | `hd_code_690C0_bin+0x804` | func_802AD7D4 (port/engine/69014.c:42); func_802AD7FC (port/engine/69014.c:26) |
| `D_802C2190` | `hd_code_7D9D0_bin+0x0` | func_80202380 (port/engine/1B100.c:139); func_802025D0 (port/engine/1B100.c:198); func_802B448C (port/engine/6E200.c:581, 582, 583, 584); func_802B4658 (port/engine/6E200.c:614); func_802B4EF8 (port/engine/6E200.c:755, 758, 763, 765); initializer of hd_code_7D9D0_bin |
| `D_802C21A4` | `hd_code_7D9D0_bin+0x14` | func_80202380 (port/engine/1B100.c:140); func_802025D0 (port/engine/1B100.c:200); func_802B448C (port/engine/6E200.c:585, 586, 587, 588); func_802B4658 (port/engine/6E200.c:615); func_802B4EF8 (port/engine/6E200.c:756, 759, 764, 766); initializer of hd_code_7D9D0_bin |
| `D_802C21B8` | `hd_code_7D9D0_bin+0x28` | func_80202380 (port/engine/1B100.c:141); func_802025D0 (port/engine/1B100.c:196); func_802B448C (port/engine/6E200.c:589, 590, 591, 592); func_802B4EF8 (port/engine/6E200.c:771); initializer of hd_code_7D9D0_bin |
| `D_802C2208` | `hd_code_7D9D0_bin+0x78` | func_80202380 (port/engine/1B100.c:132); func_802025D0 (port/engine/1B100.c:171); func_802B5CD8 (port/engine/71140.c:193, 194, 195, 196); func_802B6C28 (port/engine/71140.c:381); initializer of hd_code_7D9D0_bin |
| `D_802C226C` | `hd_code_7D9D0_bin+0xDC` | func_80202380 (port/engine/1B100.c:133); func_802025D0 (port/engine/1B100.c:173); func_802B5CD8 (port/engine/71140.c:197, 198, 199, 200); func_802B6C28 (port/engine/71140.c:382); initializer of hd_code_7D9D0_bin |
| `D_802C22D0` | `hd_code_7D9D0_bin+0x140` | func_802D0C68 (port/engine/8AEE0.c:491, 492, 493, 494); initializer of hd_code_7D9D0_bin |
| `D_802C2308` | `hd_code_7D9D0_bin+0x178` | func_802B1228 (port/engine/6C5E0.c:202, 203, 204); initializer of hd_code_7D9D0_bin |
| `D_802C2314` | `hd_code_7D9D0_bin+0x184` | func_802B2D7C (port/engine/6E200.c:185, 186, 187, 188); func_802B37B0 (port/engine/6E200.c:347, 360); initializer of hd_code_7D9D0_bin |
| `D_802C2324` | `hd_code_7D9D0_bin+0x194` | func_802CBA94 (port/engine/86F60.c:138, 139, 140, 141); func_802CC400 (port/engine/86F60.c:274); initializer of hd_code_7D9D0_bin |
| `D_802C2348` | `hd_code_7D9D0_bin+0x1B8` | func_802CBA94 (port/engine/86F60.c:142, 143, 144, 145); func_802CC400 (port/engine/86F60.c:275); initializer of hd_code_7D9D0_bin |
| `D_802C236C` | `hd_code_7D9D0_bin+0x1DC` | func_802B9C50 (port/engine/75490.c:264, 265, 266, 267); initializer of hd_code_7D9D0_bin |
| `D_802C2390` | `hd_code_7D9D0_bin+0x200` | func_802AFFD4 (port/engine/6B4A0.c:157, 158, 159, 160); initializer of hd_code_7D9D0_bin |
| `D_802C23B4` | `hd_code_7D9D0_bin+0x224` | func_8029DEA0; func_8029DEA0 (port/engine/56040.c:1071); func_8029DF78; func_8029DF78 (port/engine/56040.c:1904) |
| `D_802C23F0` | `hd_code_7D9D0_bin+0x260` | initializer of hd_code_7D9D0_bin |
| `D_802C24E8` | `hd_code_7D9D0_bin+0x358` | initializer of hd_code_7D9D0_bin |
| `D_802C26D8` | `hd_code_7D9D0_bin+0x548` | initializer of hd_code_7D9D0_bin |
| `D_802C2744` | `hd_code_7D9D0_bin+0x5B4` | initializer of hd_code_7D9D0_bin |
| `D_802C28E4` | `hd_code_7D9D0_bin+0x754` | func_802C61F0 (port/engine/80280.c:904); func_802D2C20 (port/engine/8DDB0.c:205); initializer of hd_code_7D9D0_bin |
| `D_802C2954` | `hd_code_7D9D0_bin+0x7C4` | func_802B37B0 (port/engine/6E200.c:324, 325); func_802B4EF8 (port/engine/6E200.c:743, 748); func_802B6294 (port/engine/71140.c:300); func_802B7F98 (port/engine/72B80.c:345, 346, 347, 348); func_802CAAFC (port/engine/853D0.c:426); func_802CC400 (port/engine/86F60.c:283, 284); func_802CD578 (port/engine/88160.c:256, 257); func_802D02F8 (port/engine/8AEE0.c:339, 340, 341, 342); initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C2984` | `hd_code_7D9D0_bin+0x7F4` | func_8029B614 (port/engine/56040.c:2984); func_8029B994 (port/engine/56040.c:2652); func_802B18F4 (port/engine/6C5E0.c:476, 512, 525); func_802BC2C8 (port/engine/772A0.c:217, 218); initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C29D0` | `hd_code_7D9D0_bin+0x840` | initializer of hd_code_7D9D0_bin |
| `D_802C2A5C` | `hd_code_7D9D0_bin+0x8CC` | func_802AC544 (port/engine/679E0.c:113); initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C2A80` | `hd_code_7D9D0_bin+0x8F0` | initializer of hd_code_7D9D0_bin |
| `D_802C2C70` | `hd_code_7D9D0_bin+0xAE0` | initializer of hd_code_7D9D0_bin |
| `D_802C2E60` | `hd_code_7D9D0_bin+0xCD0` | initializer of hd_code_7D9D0_bin |
| `D_802C3050` | `hd_code_7D9D0_bin+0xEC0` | initializer of hd_code_7D9D0_bin |
| `D_802C3240` | `hd_code_7D9D0_bin+0x10B0` | initializer of hd_code_7D9D0_bin |
| `D_802C3430` | `hd_code_7D9D0_bin+0x12A0` | initializer of hd_code_7D9D0_bin |
| `D_802C3620` | `hd_code_7D9D0_bin+0x1490` | initializer of hd_code_7D9D0_bin |
| `D_802C37C0` | `hd_code_7D9D0_bin+0x1630` | func_802B37B0 (port/engine/6E200.c:348); initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C37E4` | `hd_code_7D9D0_bin+0x1654` | initializer of hd_code_7D9D0_bin |
| `D_802C3804` | `hd_code_7D9D0_bin+0x1674` | func_802C7544 (port/engine/80280.c:1117, 1118); func_802D2C20 (port/engine/8DDB0.c:213, 214, 215); initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C382C` | `hd_code_7D9D0_bin+0x169C` | func_802CDA10 (port/engine/89250.c:46); initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C3848` | `hd_code_7D9D0_bin+0x16B8` | initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C386C` | `hd_code_7D9D0_bin+0x16DC` | initializer of D_80305C10; initializer of hd_code_7D9D0_bin |
| `D_802C3888` | `hd_code_7D9D0_bin+0x16F8` | initializer of hd_code_7D9D0_bin |
| `D_802C399C` | `hd_code_7D9D0_bin+0x180C` | initializer of hd_code_7D9D0_bin |
| `D_802C3B44` | `hd_code_7D9D0_bin+0x19B4` | func_802BA9A0 (port/engine/75490.c:453); initializer of hd_code_7D9D0_bin |
| `D_802C3C14` | `hd_code_7D9D0_bin+0x1A84` | initializer of hd_code_7D9D0_bin |
| `D_802C3DD4` | `hd_code_7D9D0_bin+0x1C44` | initializer of hd_code_7D9D0_bin |
| `D_802C3EB4` | `hd_code_7D9D0_bin+0x1D24` | initializer of hd_code_7D9D0_bin |
| `D_802C3FFC` | `hd_code_7D9D0_bin+0x1E6C` | func_802AC3B8 (port/engine/679E0.c:99); func_802AC61C (port/engine/679E0.c:119); func_802AC6FC (port/engine/679E0.c:125); func_802C0574 (port/engine/77E20.c:2969) |
| `D_802C48A0` | `hd_code_800DC_bin+0x4` | initializer of hd_code_800DC_bin |
| `D_802C48C4` | `hd_code_800DC_bin+0x28` | initializer of hd_code_800DC_bin |
| `D_802C48D0` | `hd_code_800DC_bin+0x34` | initializer of hd_code_800DC_bin |
| `D_802C48D8` | `hd_code_800DC_bin+0x3C` | initializer of hd_code_800DC_bin |
| `D_802C4900` | `hd_code_800DC_bin+0x64` | initializer of hd_code_800DC_bin |
| `D_802C4924` | `hd_code_800DC_bin+0x88` | initializer of hd_code_800DC_bin |
| `D_802C4934` | `hd_code_800DC_bin+0x98` | initializer of hd_code_800DC_bin |
| `D_802C4938` | `hd_code_800DC_bin+0x9C` | initializer of hd_code_800DC_bin |
| `D_802C4948` | `hd_code_800DC_bin+0xAC` | initializer of hd_code_800DC_bin |
| `D_802C496C` | `hd_code_800DC_bin+0xD0` | initializer of hd_code_800DC_bin |
| `D_802C497C` | `hd_code_800DC_bin+0xE0` | initializer of hd_code_800DC_bin |
| `D_802C4980` | `hd_code_800DC_bin+0xE4` | initializer of hd_code_800DC_bin |
| `D_802C4990` | `hd_code_800DC_bin+0xF4` | initializer of hd_code_800DC_bin |
| `D_802C49B4` | `hd_code_800DC_bin+0x118` | initializer of hd_code_800DC_bin |
| `D_802C49C4` | `hd_code_800DC_bin+0x128` | initializer of hd_code_800DC_bin |
| `D_802C49C8` | `hd_code_800DC_bin+0x12C` | initializer of hd_code_800DC_bin |
| `D_802C49D8` | `hd_code_800DC_bin+0x13C` | initializer of hd_code_800DC_bin |
| `D_802C49FC` | `hd_code_800DC_bin+0x160` | initializer of hd_code_800DC_bin |
| `D_802C4A08` | `hd_code_800DC_bin+0x16C` | initializer of hd_code_800DC_bin |
| `D_802C4A0C` | `hd_code_800DC_bin+0x170` | initializer of hd_code_800DC_bin |
| `D_802C4A20` | `hd_code_800DC_bin+0x184` | func_80288284 (blastcorps/src/hd_code/43A60.c:113) |
| `D_802D30D0` | `hd_code_8E910_bin+0x0` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3194` | `hd_code_8E910_bin+0xC4` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D32A0` | `hd_code_8E910_bin+0x1D0` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D331C` | `hd_code_8E910_bin+0x24C` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D33C8` | `hd_code_8E910_bin+0x2F8` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3444` | `hd_code_8E910_bin+0x374` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3538` | `hd_code_8E910_bin+0x468` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3614` | `hd_code_8E910_bin+0x544` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D36C0` | `hd_code_8E910_bin+0x5F0` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3784` | `hd_code_8E910_bin+0x6B4` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3890` | `hd_code_8E910_bin+0x7C0` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D393C` | `hd_code_8E910_bin+0x86C` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3A00` | `hd_code_8E910_bin+0x930` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3A4C` | `hd_code_8E910_bin+0x97C` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3BA0` | `hd_code_8E910_bin+0xAD0` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3BD4` | `hd_code_8E910_bin+0xB04` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3CE0` | `hd_code_8E910_bin+0xC10` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3DD4` | `hd_code_8E910_bin+0xD04` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3EB0` | `hd_code_8E910_bin+0xDE0` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802D3F74` | `hd_code_8E910_bin+0xEA4` | func_802A1EC8 (port/engine/5CB60.c:0) |
| `D_802E53F0` | `hd_code_A0C30_bin+0x0` | func_80284DB0 (blastcorps/src/hd_code/405F0.c:32, 34, 36) |
| `D_802E6820` | `hd_code_A0C30_bin+0x1430` | func_80267CDC (blastcorps/src/hd_code/22EE0.c:228, 229); func_80284E54 (blastcorps/src/hd_code/405F0.c:53, 54); func_802A467C (port/engine/5FD50.c:424) |
| `D_802E68F0` | `hd_code_A0C30_bin+0x1500` | func_80267CDC (blastcorps/src/hd_code/22EE0.c:229, 231); func_80284E54 (blastcorps/src/hd_code/405F0.c:54); func_802A467C (port/engine/5FD50.c:424) |
| `D_802E77B0` | `hd_code_A0C30_bin+0x23C0` | func_802A467C (port/engine/5FD50.c:424); osDpSetStatus (port/src/ultra.c:604); osSpTaskStartGo (port/src/ultra.c:626) |
| `D_8030E390` | `hd_code_C9BD0_bin+0x0` | func_80284DB0 (blastcorps/src/hd_code/405F0.c:33, 35, 37) |
| `D_8030EB90` | `hd_code_CA3D0_bin+0x0` | func_80267CDC (blastcorps/src/hd_code/22EE0.c:232) |
| `D_8030EE60` | `hd_code_CA6A0_bin+0x0` | func_802A467C (port/engine/5FD50.c:424) |

### Names the host reads inside other variables

The host names game variables by their N64 addresses (`PORT_VAR`, `n64_syms.h`); these are inside another variable.

| name | inside |
| --- | --- |
| `D_803156C0` | `D_80315440+0x280` |
| `D_803156C4` | `D_80315440+0x284` |

