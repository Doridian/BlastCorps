#ifndef GAME_TYPES_H
#define GAME_TYPES_H

/*
 * Conventions for the shared game headers (include/game/).
 *
 * Every struct here has the layout the N64 code uses; its size is checked
 * with SIZE_CHECK, which costs nothing (a typedef) and fails the build, on
 * IDO and on the port's host compiler alike, if a field moves.
 *
 * Fields that hold something other than a host-usable value are marked, so
 * the port can find exactly the fields it has to widen, rebase or byteswap
 * (ROADMAP, Phase 3).  The markers expand to the type the N64 code uses, so
 * they never change what IDO emits:
 *
 *   ROMPTR(T)    a pointer stored in data that comes from ROM at run time
 *                (a level file, a model): 4 bytes, relocated by the game
 *                itself after loading (or a KSEG0 address baked into the
 *                asset).  The LP64 port keeps it 4 bytes (PTR32).
 *   RomAddr      a cartridge ROM address, the source of a PI DMA.  In C it
 *                is the address of a linker symbol (D_00xxxxxx, a segment's
 *                _ROM_START); the port reads the ROM file there.
 *   AssetOffset  an offset from the start of the asset (or record) it is
 *                stored in, added to that asset's address at run time.
 *   SegAddr      an RSP segmented address (segment << 24 | offset), only
 *                meaningful to the display-list microcode.
 *
 *   T *PTR32 p   a pointer the handwritten code, the asm data files or the
 *                host also read at the N64's offsets, or in a struct whose
 *                instances other files reach through labels inside them:
 *                4 bytes in the LP64 port too (ultratypes.h).
 *
 * Data that is only ever produced by the running game and only the C reads
 * (the .data/.bss of the modules) uses plain pointers: the host compiler
 * lays those out itself, and their SIZE_CHECK_C holds on the N64 only
 * (docs/PORT.md, "The LP64 build").
 *
 * Names: fields keep m2c's unkXX names unless the evidence for a name is
 * given next to it (a string, an assert, a known source, the notes in
 * docs/).  A struct named here is one the code treats as one object; one
 * still called UnkStruct_<address> is shared and typed but not understood.
 */

#include <ultra64.h>

#define ROMPTR(type) type PTR32
typedef u32 RomAddr;
typedef u32 AssetOffset;
typedef u32 SegAddr;

/* A compile-time check that a struct has its N64 size. */
#define SIZE_CHECK(type, size) typedef char type##_size_check[(sizeof(type) == (size)) ? 1 : -1]
/* The same for a struct only the C uses, with its own pointers: the LP64
   port lays it out as the host does (docs/PORT.md, "The LP64 build"). */
#ifdef PORT_LP64
#define SIZE_CHECK_C(type, size) typedef char type##_size_check
#else
#define SIZE_CHECK_C(type, size) SIZE_CHECK(type, size)
#endif

#endif
