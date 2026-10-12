/*
 * The game's fixed buffers: the regions of port_regions.h as ordinary
 * buffers, wherever the linker puts them.
 *
 * On the N64 each is at an address of its own; here each is a variable of
 * its own, sized as the table says, and the names inside one
 * (the ghost's buffers, the front end's area and the effects' heap's
 * limits, in the pool) are offsets into it (game/memmap.h).  The boot
 * code's words are libultra's variables (src/ultra.c), and init's
 * hand-over words two of their own.  Static arrays, not malloc: their sizes
 * are known at compile time, so a small target sees the whole footprint in
 * its .bss.
 */
#include "common.h"
#include "port.h"
#include "game/memmap.h"

/* the two 320x240 RGBA16 framebuffers */
u16 D_80000400[2][320 * 240] __attribute__((aligned(64)));

/* the level pool, with the ghost's buffers, the front end's area and the
   effects' heap's limits inside it */
u8 D_8004B400[PORT_REGION_SIZE_D_8004B400] __attribute__((aligned(64)));

/* segment 1's static data, after hd_code's .bss on the N64 up to init's
   hand-over words (CMake reads where it starts from the version's link) */
u8 D_803FF600[0x803FFFF8u - PORT_STATIC_START] __attribute__((aligned(16)));

/* the front end's compressed ROM range, init's hand-over to hd_code
   (src/boot.c, hd_code 46C20.c) */
u32 D_803FFFF8, D_803FFFFC;

/* what the host dumps of the game's memory (PORT_DUMP, host/dump.c): these, in this order */
const PortRegion port_regions[] = {
    { "D_80000400", D_80000400, sizeof D_80000400 },
    { "D_8004B400", D_8004B400, sizeof D_8004B400 },
    { "D_803FF600", D_803FF600, sizeof D_803FF600 },
    { "D_803FFFF8", &D_803FFFF8, sizeof D_803FFFF8 },
    { "D_803FFFFC", &D_803FFFFC, sizeof D_803FFFFC },
};
const unsigned port_nregions = sizeof port_regions / sizeof port_regions[0];
