/*
 * hd_front_end, the overlay.
 *
 * On the N64, func_8028B3E0 (hd_code/46C20.c) inflates the front end's
 * .text and .data from the ROM to 0x801E7000 and clears the rest of its
 * area, up to 0x8021ED00 (so its .bss); gameplay reuses that memory for
 * the level.  In the port the front end's code is linked in, and its
 * variables are ordinary globals, listed at build time (port_fe_tables:
 * tools/gen_fevars.py), so "loading" it means putting each back as it was
 * at startup, from a copy made then, which is what the inflate leaves
 * behind.  The ROM's copy can't be used: its .data holds N64 pointers to
 * N64 code.
 *
 * It still does the N64's load first (the DMA and the inflate, into the
 * same memory), for the time they take: a second or so of PI and CPU
 * time that the pacing (docs/PORT.md, "Timing") would otherwise leave out
 * wherever the game returns to the menus.  What the inflate leaves in the
 * .text area is the MIPS code, which nothing in the port reads.
 *
 * The front end's area (the level pool's tail: port/include/
 * port_regions.h) gets the .data the inflate would leave there, and its
 * .bss cleared.
 *
 * This is func_8028B3E0 for every caller; its defining file has the
 * original as __real_func_8028B3E0 (CMake).
 */
#include "common.h"
#include "functions.h"
#include "port.h"
#include "game/memmap.h"

/* where the version's .hd_front_end_data and .hd_front_end_bss are in the
   front end's area (CMake reads the sections from the N64 link) */
#define FE_DATA (PORT_FE_DATA - PORT_FE_TEXT)
#define FE_BSS (PORT_FE_BSS - PORT_FE_TEXT)
#define FE_SIZE PORT_REGION_SIZE_D_801E7000

extern u8 D_80370C50;           /* "front end loaded" */
extern u32 *D_802FDB30, *D_802FDB34;    /* the compressed front end's ROM range */
extern void *memmove(void *, const void *, unsigned int);
extern void *memset(void *, int, unsigned int);

static u8 fe_data[FE_BSS - FE_DATA];

/* tools/gen_fevars.py's (port_game.h's PortFeVar): a table a source of the
   front end's, each a row a variable, then a row of zeros */
extern const PortFeVar *const port_fe_tables[];

void port_overlay_init(void) {
    const PortFeVar *const *t;
    const PortFeVar *v;

    for (t = port_fe_tables; *t; t++)
        for (v = *t; v->size; v++) {
            u32 at = v->n64 - PORT_FE_DATA;

            if (v->snap)
                memmove(v->snap, v->at, v->size);
            if (v->n64 && at < sizeof fe_data)
                memmove(fe_data + at, v->at, MIN(MIN(v->n64room, v->size), sizeof fe_data - at));
        }
}

static void restore_vars(void) {
    const PortFeVar *const *t;
    const PortFeVar *v;

    for (t = port_fe_tables; *t; t++)
        for (v = *t; v->size; v++)
            if (v->snap)
                memmove(v->at, v->snap, v->size);
}
#define __wrap_func_8028B3E0 func_8028B3E0


void __wrap_func_8028B3E0(void) {
    u32 len = *D_802FDB34 - *D_802FDB30;

    osViBlack(1);
    if (D_80370C50 == 0) {
        func_8028B4C4(*D_802FDB30, MEM_FRONT_END, &len, 13, 10, 1);
        memmove(MEM_FRONT_END + FE_DATA, fe_data, sizeof fe_data);
        memset(MEM_FRONT_END + FE_BSS, 0, FE_SIZE - FE_BSS);
        restore_vars();
        D_80370C50 = 1;
        func_801F57B0();
        if (host_verbose)
            host_log("front end loaded\n");
    }
}
