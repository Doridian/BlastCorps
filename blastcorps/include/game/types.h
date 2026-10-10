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
 *                (a level file, a model, a sound bank): 4 bytes, relocated
 *                by the game itself after loading (or a KSEG0 address
 *                baked into the asset).  The LP64 port keeps it 4 bytes
 *                (PTR32): the bytes are the ROM's.
 *   RomAddr      a cartridge ROM address, the source of a PI DMA.  In C it
 *                is the address of a linker symbol (D_00xxxxxx, a segment's
 *                _ROM_START); the port reads the ROM file there.
 *   AssetOffset  an offset from the start of the asset (or record) it is
 *                stored in, added to that asset's address at run time.
 *   SegAddr      an RSP segmented address (segment << 24 | offset), only
 *                meaningful to the display-list microcode.
 *
 *   T *PTR32 p   a pointer kept 4 bytes in the LP64 port too
 *                (ultratypes.h), where something other than the C fixes the
 *                layout: ROM data (ROMPTR), libaudio and the sound player
 *                built on it, the display list's and the audio command
 *                list's words, OSDevMgr's two function pointers.  Nothing
 *                else: the engine is C and reads by
 *                field, and the host reads by port/src/layout.c's table.
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

#define ROMPTR(type) type PTR32
typedef u32 RomAddr;
typedef u32 AssetOffset;
typedef u32 SegAddr;

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
