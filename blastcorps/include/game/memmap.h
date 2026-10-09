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
extern u8 D_80055400[];     /* the ghost's buffers, in the pool */
extern u8 D_80065400[];
extern u8 D_801E7000[];     /* the front end's area, the pool's tail */
extern u8 D_8021ED00[];     /* init's area: depth buffer, compressed staging */
#define MEM_POOL D_8004B400
#define MEM_GHOST0 D_80055400
#define MEM_GHOST1 D_80065400
#define MEM_FRONT_END D_801E7000
#define MEM_INIT_AREA D_8021ED00
#else
#define MEM_POOL 0x8004B400
#define MEM_GHOST0 0x80055400
#define MEM_GHOST1 0x80065400
#define MEM_FRONT_END 0x801E7000
#define MEM_INIT_AREA 0x8021ED00
#endif

#endif
