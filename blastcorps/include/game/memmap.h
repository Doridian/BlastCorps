#ifndef GAME_MEMMAP_H
#define GAME_MEMMAP_H

/*
 * The game's fixed buffers, the memory it uses by address (docs/PORT.md,
 * "Memory model").  MEM_* is the address as IDO sees it, a number; in the
 * port it is the region's name (port/include/port_regions.h), which the
 * port's link puts at that address, or wherever the layout moved it.
 * Sites keep their own casts.
 */
#ifdef TARGET_PC
#include "port_regions.h"
extern u8 D_8004B400[];     /* the level pool */
/* init's area, depth buffer and compressed staging: the port's own buffer
   (port_regions.h, PORT_INIT_AREA_SIZE), not at 0x8021ED00, where the
   pool may grow */
extern u8 port_init_area[];
#define MEM_POOL D_8004B400
/* the ghost's buffers, the front end's area (the pool's tail) and the
   effects' heap's limits (port/engine/60F60.c): places in the pool
   (D_80055400, D_80065400, D_801E7000, D_8020ED00, D_8021DD00) */
#define MEM_GHOST0 (D_8004B400 + PORT_REGION_OFF_D_80055400)
#define MEM_GHOST1 (D_8004B400 + PORT_REGION_OFF_D_80065400)
#define MEM_FRONT_END (D_8004B400 + PORT_REGION_OFF_D_801E7000)
#define MEM_LIMIT_LOW (D_8004B400 + PORT_REGION_OFF_D_8020ED00)
#define MEM_LIMIT (D_8004B400 + PORT_REGION_OFF_D_8021DD00)
#define MEM_INIT_AREA port_init_area
#else
#define MEM_POOL 0x8004B400
#define MEM_GHOST0 0x80055400
#define MEM_GHOST1 0x80065400
#define MEM_FRONT_END 0x801E7000
#define MEM_INIT_AREA 0x8021ED00
#endif

#endif
