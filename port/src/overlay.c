/*
 * hd_front_end, the overlay.
 *
 * On the N64, func_8028B3E0 (hd_code/46C20.c) inflates the front end's
 * .text and .data from the ROM to 0x801E7000 and clears the rest of its
 * area, up to 0x8021ED00 (so its .bss); gameplay reuses that memory for
 * the level.  In the port the front end's code is linked in, and its data
 * sits at its N64 addresses (tools/gen_ld.py), so "loading" it means
 * putting its .data back as it was at startup and clearing its .bss, which
 * is what the inflate leaves behind.  The ROM's copy can't be used: its
 * .data holds N64 pointers to N64 code.
 *
 * It still does the N64's load first (the DMA and the inflate, into the
 * same memory), for the time they take: a second or so of PI and CPU
 * time that the pacing (docs/PORT.md, "Timing") would otherwise leave out
 * wherever the game returns to the menus.  What the inflate leaves in the
 * .text area is the MIPS code, which nothing in the port reads.
 *
 * With PORT_SCATTER_FE the front end's variables aren't in its area: the
 * scattered layout put each somewhere else, and its area moved with the
 * level pool (port/include/port_regions.h).  port-arena then lists them
 * (__port_fe_vars), and each is put back as it was at startup, from a
 * copy made then; the area itself gets the .data the inflate would leave
 * there, and its .bss cleared, as in the other builds.
 *
 * The link wraps func_8028B3E0 (--wrap) with this.
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

#ifdef PORT_SCATTER_FE
/* port-arena's (bepass/Arena.cpp, feTableMake): a row a variable, by N64
   address, then a row of zeros */
extern const struct {
    u8 *at;         /* where it is */
    u32 n64;        /* its N64 address */
    u32 size, n64size;
    u32 snap;       /* its copy, in __port_fe_snap */
} __port_fe_vars[];
extern u8 __port_fe_snap[];
#endif

void port_overlay_init(void) {
#ifdef PORT_SCATTER_FE
    int i;

    for (i = 0; __port_fe_vars[i].size; i++)
        memmove(__port_fe_snap + __port_fe_vars[i].snap, __port_fe_vars[i].at, __port_fe_vars[i].size);
    for (i = 0; __port_fe_vars[i].size; i++) {
        u32 at = __port_fe_vars[i].n64 - PORT_FE_DATA;

        if (at < sizeof fe_data)
            memmove(fe_data + at, __port_fe_vars[i].at,
                    MIN(__port_fe_vars[i].n64size, sizeof fe_data - at));
    }
#else
    memmove(fe_data, D_801E7000 + FE_DATA, sizeof fe_data);
#endif
}

void __wrap_func_8028B3E0(void) {
    u32 len = *D_802FDB34 - *D_802FDB30;

    osViBlack(1);
    if (D_80370C50 == 0) {
        func_8028B4C4(*D_802FDB30, D_801E7000, &len, 13, 10, 1);
        memmove(D_801E7000 + FE_DATA, fe_data, sizeof fe_data);
        memset(D_801E7000 + FE_BSS, 0, FE_SIZE - FE_BSS);
#ifdef PORT_SCATTER_FE
        {
            int i;

            for (i = 0; __port_fe_vars[i].size; i++)
                memmove(__port_fe_vars[i].at, __port_fe_snap + __port_fe_vars[i].snap, __port_fe_vars[i].size);
        }
#endif
#ifdef PORT_ACCESS_PROFILE
        __port_access_set(D_801E7000 + FE_DATA, FE_SIZE - FE_DATA, 0);    /* the image's data again */
#endif
        D_80370C50 = 1;
        func_801F57B0();
        if (host_verbose)
            host_log("front end loaded\n");
    }
}
