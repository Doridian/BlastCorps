# Gameplay types and the memory inventory

What is known about the layout of the game's memory: the structures the C,
the handwritten asm and the ROM's data share, which of their fields hold
addresses, and how wide every field is.  The 64-bit port needs exactly
this: which words to widen, rebase or byteswap (docs/PORT.md, "Memory
model", "Toward native code").  Addresses are us.v11's.

There are two outputs:

- **The headers** (`blastcorps/include/game/`): the structures written out
  as C, with `SIZE_CHECK`, the evidence for each name, and the address
  markers of `types.h` (`ROMPTR`, `AssetOffset`, `RomAddr`, `SegAddr`).
  The C uses them; they are the readable side.
- **The inventory** (`blastcorps/include/game/inventory.json`, written by
  `tools/inventory.py`): the machine-readable side, for all memory the C and
  the translated asm share and for the data that comes from ROM.  It is
  generated from the headers (through clang) and from a data-flow analysis
  of all the code (`tools/fieldscan.py`), checked against RDRAM snapshots
  of real play.

## Regenerating

```
make VERSION=us.v11 -C blastcorps            # the analysis reads the linked ELFs and asm/
tools/inventory.py                           # rewrites inventory.json
tools/inventory.py --check                   # fails if inventory.json is out of date
tools/inventory.py --dumps rdram_*.bin       # also refresh inventory_dumps.json
tools/fieldscan.py                           # the working files, in blastcorps/build/fieldscan
```

The RDRAM snapshots are the port's `PORT_DUMP` files (`PORT_AUTOSTART=2
PORT_DUMP=1200,2500,4000,6000,7900 build/port/blastcorps ... --headless
--deterministic --frames 16000`: title, name entry, Simian Acres).  What
they showed is kept in `inventory_dumps.json`, so later runs don't need
them.  Both tools fix the hash seed; the output is the same every run.

## How the analysis works (tools/fieldscan.py)

Every function of init, hd_code and hd_front_end (IDO C and Rare's asm
alike, 1666 of them; the data islands and microcode left out) gets a
forward data-flow pass over its registers and stack frame.  A register is
unknown or `a*X + k (+ a multiple of s)`, X being where the value came from:
a word loaded from a location, an entry register, a callee's result, the
entry `$sp`.  Loop joins turn the difference of two constants into the
stride `s`, so an index scaled by 0x74 or a pointer stepping 0x18 through an
array comes out as base + stride.  Where two paths meet with different
origins, the value keeps all of them.  Jump tables are followed; calls into
the C return only what the C definition returns (`void` returns nothing).
Of the code's 71,828 loads and stores, 71,559 are reached and all but 1,329
resolve to a location.

A location is an absolute address (a global, maybe with a stride) or an
offset into what some other location, argument or return value points to.
Those are "slots", and every store, call and return of a value from one
slot to another is a flow.  A slot holds an address when its value is
dereferenced, when an address constant (a `%hi/%lo` relocation into RAM) is
stored in it, when an address flows into it, or when it flows into a
dereferenced slot; the varargs printfs and message queues are left out of
the flows.  ROM and segment constants (relocations against `_ROM_START`
symbols and segment addresses) give `romoff` and `segptr`, `& 0x1FFFFFFF` of
an address gives `phys`, and a word that is added to the address of the
record it sits in is an `offset` (all of `LevelHeader`'s section offsets
come out this way, and no other field of it does).

Arguments are then resolved to what the callers pass, so the handwritten
engine's `$gp`, `$t0` or `$s1` arguments become the globals or heap records
they point at.  Structures fall out of that: the addresses that flow into
one argument are instances of one type (`types.txt`), which is how the
vehicle state below was found (62740's functions take it in `$gp`, from 20
places).

Checked against five RDRAM snapshots of real play: of the words the
analysis calls pointers in .data/.bss, 325 fields held only addresses or 0
and 141 held other values at some point; the latter are nearly all display
list words (a `Gfx`'s second word is an address for some commands only)
and loops that step 4 bytes through a structure array.  235 words held
addresses in every snapshot but the analysis doesn't reach them as such;
they are in the inventory as `ptr?` where the code touches them at all
(42) and are mostly byte copies (`func_802A75DC` copies a vehicle's
state with `lbu`/`sb`), libultra's `OSTask`s and `OSMesg` arrays.

## inventory.json

One JSON object:

- **`types`**: `{name: {size, align, fields: [{off, name, type, count}],
  header, union?, note?}}` for every structure in `include/game/` and the
  libultra structures they contain.  `type` is a scalar (`s8 u8 s16 u16 s32
  u32 f32 s64 u64 f64`), another type's name, `bytes` (texels, text,
  padding: endian-free), `gfx` (a display-list word pair), or an address
  kind: `ptr` (an N64 address, with `to`; `rom: true` for a pointer the
  loader relocates in ROM data), `romoff` (a cartridge address), `segptr`
  (a segmented address), `offset` (from `base`).  Layouts come from clang
  (`-m32 -malign-double`; the SIZE_CHECKs hold on IDO and clang alike).
- **`symbols`**: `{name: {type, count, declared, storage, note}}` for the
  variables whose declaration isn't their type: the handwritten objects'
  (`storage`: their `asm/data/*.s`, which has only `.space`/`.word`), typed
  by the headers; the C placeholders `tools/bss_c.py` wrote for memory no C
  names (`placeholder: true`: `u8` blobs and `T x[1]` arrays); and the fixed
  memory outside the modules (framebuffers, the heap from `0x8004B400`, the
  ghost buffers at `0x80055400`/`0x80065400`, init's memory reused as the
  depth buffer, the static data at `0x803FF600`).
- **`loads`**: every call that brings bytes in from the ROM: `{site, via,
  func, rom, dst, type, count, reloc}`, found at the calls to
  `osPiStartDma`, `func_8028B4C4` (DMA, then gzip or Rare's LZSS), the gzip
  inflate `func_802C4108` and the LZSS `func_802C41C0` (the ROM argument as
  its segment, the destination as a symbol or slot), plus the loaders that
  go through the handwritten code (the level file, vehicle models, model
  table models, textures).  `reloc` says what the game turns into pointers.
- **`punning`**: `{where, as, sites}`: a field accessed at two widths at the
  same offset, or as float and integer, outside copy loops; plus the known
  cases: the `u64` game mode words (`ld`/`sd` in the C, words in the asm,
  high word first), `D_8021A830` (4 bytes defined, 8 read), the Controller
  Pak block `D_8039C4B8`, the per-frame buffer's two layouts, `Mtx`.
- **`packed`**: `{site, op, where, fields}`: the handwritten code's word
  and doubleword accesses that carry narrower fields, found two ways: where
  other code reaches the same bytes as narrower fields (vertices' s16s,
  records' bytes), and the `Mtx` builders (words stored with `0x10000`, 1.0,
  on the diagonal: `func_8029EC68`, `func_802ACA60` .. `func_802ACC68`,
  `func_802C0E8C`, ...).
- **`asm_uses`**: `{type: {function: [offsets]}}`: the handwritten
  functions that touch each header type, and where.
- **`pointers`**: `{where, kind, evidence, field?}` for every field or
  variable holding an address: `ptr`, `ptr (initialized data)` (a
  relocation in `.data`: exact), `offset`, `romoff`, `segptr`, `phys`,
  `ptr?` (only the snapshots say so).  `where` is a global
  (`D_80364460[0x74]+0x54`: element size in brackets) or what a slot points
  to (`*(D_80358074+0x0)+0x20`); `field` gives the header type and offset
  where one covers it.  `evidence` lists `deref`, `ram` (an address stored
  or flowing in), `offset`, `reloc`, and `dump:ptr`/`dump:int`.

`blastcorps/build/fieldscan/fields.tsv` has the same per-field view with
the access widths and signedness of every field of every region, pointers
or not.

## The structures

Confidence: **high** where the layout is what the code does with every
field (C and asm agree, sizes are exact); **medium** where the size and the
pointer fields are sure but fields are only partly understood; **low**
where only part of the structure is seen.

| structure | header | where | size | confidence | evidence |
| --- | --- | --- | --- | --- | --- |
| `Vehicle` | vehicle.h | `D_80364460[12]` | 0x74 | high | `func_802A1388` fills it: 22 pointers into the model file (all RAM addresses in every snapshot), type, position |
| `VehicleModel` | vehicle.h | the vehicle's model file (ROM) | 0x4C | medium | every word is added to the file's address somewhere (offsets); what they point at isn't typed |
| `VehicleState` | vehicle.h | one per vehicle, in each vehicle module's .bss (19 instances) | 0xA8 | medium | hd_code 62740 takes it in `$gp` from every module; widths from 108 views; 0xA6 bytes copied by `func_802A75DC`; no pointers; names unknown |
| `UnkStruct_803ED460` | vehicle.h | 32 per vehicle block (and front-end models) | 0x18 | medium | 56040's records; `unk0` a pointer, two f32, halfwords, bytes |
| vehicle modules | vehicle.h | the module table | | high (mapping) | `func_802A350C`'s and hd.c's switches on the type; each module's own variables are separate symbols (declared individually) |
| `Model` | model.h | model_table models (ROM, relocated in place) | 0x60 | medium | `func_802A2A98` loads it; offset fields as added by the code; written into by the building loader |
| `Building` | objects.h | `D_803F4030[]` to `D_803F7654` | 0xFC | medium | `func_802A1D54`/`func_802A21AC` fill it; `model` points at a `Model`; position; 77E20 runs it, 13A70.c draws shadows |
| `AmmoBox`, `AmmoBoxInfo`, `LevelAmmoBox` | objects.h | `D_8039AF00[15]`, `D_802FDB40[2]` | 0x18, 0x16, 8 | high | 479D0.c |
| `TntCrate`, `TntCrateInfo`, `LevelTntCrate` | objects.h | `D_8039B070[20]`, `D_802FDB98[2]` | 0x48, 0x18, 0xC | high | 48D00.c |
| `Block`, `Hole`, `UnkStruct_8039C800`, `BlockInfo`, `LevelBlock`, `LevelHole` | objects.h | `D_8039C550[8]`, `D_8039C718[8]`, `D_8039C800[8]`, `D_802FDC08[3]` | | high | 4B5E0.c |
| level-file records (`LevelBuilding`, `LevelCommPoint`, `LevelBox`, `LevelBounds`, `LevelTerrainTri`, `LevelCollisionTri`, `LevelUnk58`) | objects.h | ROM, in the level file | | high (layout) | tools/assetlib/level.py's layouts; `LevelUnk58` is how 32E00.c reads it (level.py writes four s16) |
| `LevelHeader` | level.h | `*D_80358074` | 0xC8 | high | 39 of its 41 offset fields come out of the analysis as offsets (0x80 and 0x8C are each read once, by 5FD50, in a way the analysis does not follow), and no other field does |
| level run-time variables | level.h | 5CB60's .bss | | medium | the grids (cell sizes and counts from the header), the collision fix / terrain / collision group tables, the carrier's model and segments, the level bounds |
| camera | camera.h | hd.c's .bss | | medium | separate variables (no base-pointer access anywhere); roles from hd.c's view setup |
| `Rdu`, `LevelRdu`, `LevelStats` | level.h | as before | | high | unchanged; `D_8036BBB0[0x192]` (the collection order) added |

Fixed along the way: yoshi.c's `D_8036BB48` is a `u16[0x34]` text buffer
(func_8026F004 copies whole window texts into it), so 2B3F0's `.bss`
starts at `0x8036BBB0`, not `0x8036BB50` (the 0x60 bytes between were the
rest of the buffer; `tools/regen_code_yaml.sh`, all four yamls); 2B3F0's
`D_8036BBB0` is a `u16[0x192]`.  Both were `[1]` arrays written past their
end, which clang (the 32-bit port) miscompiled.

## The C

- The level objects' files use the shared types instead of their own copies
  (479D0.c, 48D00.c, 4B450.c, 4B5E0.c, 13A70.c, 32E00.c) and their `.bss`
  are arrays of them where they were a `T x[1]` and a filler.
- The engine's variables the C reads (vehicle positions and counters, the
  level loader's tables, the camera) are declared once, in vehicle.h,
  level.h and camera.h.
- Globals the files declared with different types are declared once, in
  the owner's header (game.h, level.h, audio.h, sched.h, player.h,
  frontend.h, objects.h); where a file needs the other signedness it casts
  (only `(u32)D_80364AA8` in yoshi.c and `(s16)D_803BE714` in 2B3F0.c were
  needed), and where the defining file's own type was the odd one out (a
  `u8[1]`, an `s8` only ever stored to) the definition changed.  What is
  left declared two ways is deliberate: `D_02000000`/`D_803156F8` (one
  buffer, two layouts), `D_8039C4B8` (a pak block with a u64 marker),
  `D_80367750`/`D_8036AFB0` (buffers 405F0.c fills as u64), `D_8021A830`,
  `D_80301080`, `D_8036C794` (two views of one record, not yet reconciled).
- Three struct types yoshi.c and 2B3F0.c both defined are in yoshi.h.

All four versions still match; `bss_c.py --check` and `data_c.py --check`
give what they gave before.

## For the port

- Everything in `pointers` is 4 bytes on the N64.  The initialized-data
  pointers (`ptr (initialized data)`) are pointer initializers in the
  port's C (the asm data's too, port/tools/asm2c.py); the rest are written
  at run time.
- ROM data with offsets: `LevelHeader` (the offsets are added where used;
  the loader stores the sums in the level variables of level.h),
  `VehicleModel` and `Model` (the same; `Model` is also written into), the
  sound banks and sequences (libaudio relocates them in place).
- Fixed addresses the C and asm use: the framebuffers at `0x80000400`, the
  heap from `0x8004B400`, the ghost buffers at `0x80055400` and
  `0x80065400` (50670.c's `D_8039CA68`), init's memory as the depth buffer,
  the static data at `0x803FF600`.
- Width mismatches: see `punning` and `packed`.  The ones that matter most:
  the `u64` game mode words, `Mtx` words written by the handwritten
  builders, vertices written as words.

## Still unknown

- Field names: nearly every field is still `unkXX`; the headers only name
  what the code shows.
- The vehicle modules' own variables past the state (each module's tail:
  ammunition, the model pointers at +0x3B4..0x3BC, timers) are declared only
  where the C uses them; their full layouts are in `fields.tsv`.
- hd_code 77E20 (buildings) beyond the `Building` record, 5FD50/60D50/60F60
  (terrain, collision and the texture cache: `D_803C4B70`, `D_803C3250`,
  `D_803C4250`), 679E0 (debris or particles: the 0x40-byte matrices at
  `D_803B3730`, `D_803C4F30`, ...), 8A2E0 (the communication point), 89250
  (the collision test results `D_803F9320..2E`).
- `D_803EB7A0` (the copy of the current vehicle) and other byte-copied
  memory: the analysis sees bytes there, the snapshots see pointers.
- The model formats beyond their headers, and what `func_802A44E4` fixes
  up in a loaded model.
- The front end's structures (hd_front_end): the inventory covers their
  pointers, no new headers were written for them.
