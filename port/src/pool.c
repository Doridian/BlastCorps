/*
 * The level pool's display lists (docs/PORT.md, "Graphics").
 *
 * The pool (the region D_8004B400, bump pointer D_80358070) holds the
 * level's display lists among everything else: the eight per-frame lists
 * (00000.c), the vehicles' copies (34430.c's func_80278BF0, 5CB60.c's
 * func_802A1388), the water's (39050.c), the front end's built in place
 * (9570.c) and the ROM's, widened where they land (src/widen.c).  Where the
 * port's Gfx is wider than the N64's 8 bytes (the LP64 builds' 16), they
 * take more of it, and every one of them is carved out here, so that the
 * N64's own position, what the top would be with 8-byte commands, is kept
 * beside the real one: port_pool_n64.  The effects' heap, which takes what
 * is left below a limit in 0x1010-byte slots (port/engine/60F60.c,
 * func_802A5FA8), counts its slots from that, so a level has the N64's
 * number of them in every build, and the pool's end is the N64's plus the
 * most the display lists can take beyond it (port_regions.h,
 * PORT_POOL_GROWTH), checked here.
 */
#include "common.h"
#include "port.h"
#include "game/game.h"
#include "game/level.h"
#include "game/memmap.h"

#define N64_GFX_SIZE 8u

_Static_assert(sizeof(Gfx) == PORT_GFX_SIZE, "port_regions.h's PORT_GFX_SIZE is gbi.h's Gfx");

/* what the display lists below the top take beyond the N64's 8 bytes a
   command, since the pool was emptied */
static u32 excess;

/* init's area (port_regions.h): where the N64 has it, the pool may grow */
u8 port_init_area[PORT_INIT_AREA_SIZE] __attribute__((aligned(64)));

/* the pool's end (its region's) */
#define POOL_END (MEM_POOL + PORT_REGION_SIZE_D_8004B400)
/* (an address of the N64 side's, for the log) */
#define N64(p) ((unsigned)(uintptr_t)(p))

/* the front end's variables, in the pool's tail while it runs (its .data
   and .bss where the N64 has them: src/overlay.c) */
extern u8 D_80370C50;           /* "front end loaded" */
#define FE_VARS (MEM_FRONT_END + (PORT_FE_DATA - PORT_FE_TEXT))

static void check(const char *what) {
    if (D_80358070 > POOL_END) {
        host_fatal("pool: %s: the top is 0x%X bytes past the pool's end (its N64 place 0x%X, its display lists "
                   "0x%X bytes more than the N64's: port_regions.h's PORT_POOL_GFX_N64 is too small)\n",
                   what, (unsigned)(D_80358070 - POOL_END), N64(port_pool_n64()), (unsigned)excess);
    }
}

/* the level the pool is for (-1: none yet), and whether it has had its
   effects' heap (a level does; the front end doesn't) */
static s32 level = -1, slots;

void port_pool_reset(void) {
    if (level >= 0) {
        /* (its top only grows: where it is now is the level's high-water) */
        host_pool_report(slots ? "end" : "end (front end)", level, D_80358070, port_pool_n64(), POOL_END);
        /* the front end ran with its variables in the pool's tail: the
           pool kept below them, as the N64's (where the N64's own top
           is past them too, that is the game's) */
        if (!slots && D_80370C50 && D_80358070 > FE_VARS && (u8 *)port_pool_n64() <= FE_VARS) {
            host_fatal("pool: the front end's top was 0x%X bytes into its variables (its N64 place 0x%X, its "
                       "display lists 0x%X bytes more than the N64's)\n",
                       (unsigned)(D_80358070 - FE_VARS), N64(port_pool_n64()), (unsigned)excess);
        }
    }
    level = D_802E8BDC;
    slots = 0;
    excess = 0;
}

void *port_pool_gfx(void *end) {
    u32 n = (u32)((u8 *)end - D_80358070) / sizeof(Gfx);

    excess += n * (sizeof(Gfx) - N64_GFX_SIZE);
    D_80358070 = end;
    check("display lists");
    return end;
}

void port_pool_widened(void *at, u32 narrow, u32 wide) {
    /* (a list loaded into the pool's free part, which its caller then
       takes: anywhere else it is no carve-out) */
    if ((u8 *)at >= D_80358070 && (u8 *)at < POOL_END) {
        excess += wide - narrow;
    }
}

void *port_pool_n64(void) {
    return D_80358070 - excess;
}

void *port_pool_slots(void *end) {
    D_80358070 = end;
    check("the effects' heap");
    /* (the heap runs to the N64's limits, inside the front end's area: the
       level loader has let the front end go by then, 5CB60.c) */
    if (D_80370C50)
        host_fatal("pool: the effects' heap while the front end is loaded\n");
    slots = 1;
    host_pool_report((D_80364AA8 & 0x20) ? "effects (the lower limit)" : "effects", level, D_80358070, port_pool_n64(),
                     POOL_END);
    return end;
}
