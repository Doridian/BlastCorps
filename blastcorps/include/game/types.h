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
 *   RomAddr      a cartridge ROM address, the source of a PI DMA, written
 *                ROM(D_00xxxxxx) (or a segment's _ROM_START).  For IDO that is
 *                the address of the linker symbol, as the original had it;
 *                in the port it is the number (port/tools/gen_syms.py's
 *                rom_addrs.h), and the port reads the ROM file there.
 *   AssetOffset  an offset from the start of the asset (or record) it is
 *                stored in, added to that asset's address at run time.
 *   SegAddr      an RSP segmented address (segment << 24 | offset), only
 *                meaningful to the display-list microcode.
 *   DlAddr       an address the game keeps as an integer to put in a display
 *                list (K0_TO_PHYS's, osVirtualToPhysical's result): on the
 *                N64 the physical address, a u32; in the port a pointer's
 *                value, as wide as one (PR/R4300.h).
 *
 *   T *PTR32 p   a pointer kept 4 bytes in the LP64 port too
 *                (ultratypes.h), where something other than the C would
 *                fix the layout.  Nothing has one any more: the engine is
 *                C and reads by field, the host reads by port/src/
 *                layout.c's table, libaudio's bank files are parsed into
 *                objects of the port's own (no ROM data is read as
 *                pointers), and the display lists' and the audio command
 *                lists' words are as wide as a pointer (gbi.h, abi.h).
 *
 * Everything the running game makes and the C reads (the modules' .data
 * and .bss, the asm data, the OS's and the scheduler's structures) has
 * plain pointers: the host compiler lays it out, and a struct's
 * SIZE_CHECK_C holds where pointers are 4 bytes (docs/PORT.md, "The LP64
 * build").  A variable's definition and every declaration of it agree.
 *
 * Names: fields keep m2c's unkXX names unless the evidence for a name is
 * given next to it (a string, an assert, a known source, the notes in
 * docs/).  A struct named here is one the code treats as one object; one
 * still called UnkStruct_<address> is shared and typed but not understood.
 */

#include <ultra64.h>

typedef u32 RomAddr;
#ifdef TARGET_PC
#include "rom_addrs.h"
#define ROM(sym) ((RomAddr)ROM_##sym)
#else
#define ROM(sym) ((RomAddr)(sym))
#endif
typedef u32 AssetOffset;
/* the address `off` bytes into a loaded asset at `base` (its header's
   fields are offsets from its start); for IDO the original's sum */
#ifdef TARGET_PC
#define ASSET_PTR(base, off) ((void *)((u8 *)(base) + (off)))
#else
#define ASSET_PTR(base, off) ((void *)((off) + (u32)(base)))
#endif
typedef u32 SegAddr;
#ifdef TARGET_PC
typedef __UINTPTR_TYPE__ DlAddr;
#else
typedef u32 DlAddr;
#endif

/* osRecvMesg into an integer variable.  OSMesg is a pointer, so in the port
   (where it is as wide as the host's) the message goes through one and is then
   narrowed; for IDO it is the original's cast. */
#ifdef TARGET_PC
#define osRecvMesgInt(mq, var, flag) ({ OSMesg msg_ = 0; s32 r_ = osRecvMesg(mq, &msg_, flag); \
    if (r_ == 0) (var) = (__typeof__(var))(__UINTPTR_TYPE__)msg_; r_; })
#else
#define osRecvMesgInt(mq, var, flag) osRecvMesg(mq, (OSMesg *)&(var), flag)
#endif

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
