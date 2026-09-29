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
 * The link wraps func_8028B3E0 (--wrap) with this.
 */
#include "common.h"
#include "port.h"

#define FE_START 0x801E7000     /* the overlay area */
#define FE_DATA 0x80208040      /* .hd_front_end_data */
#define FE_BSS 0x80210E90       /* .hd_front_end_bss */
#define FE_END 0x8021ED00       /* init's .text: the end of the area */

extern u8 D_80370C50;           /* "front end loaded" */
extern u32 *D_802FDB30, *D_802FDB34;    /* the compressed front end's ROM range */
void func_8028B4C4(u32 rom, u8 *dst, u32 *len, u8 arg3, u8 arg4, u8 arg5);
void func_801F57B0(void);
void func_8029A7E4(char *, ...);
extern void *memmove(void *, const void *, unsigned int);
extern void *memset(void *, int, unsigned int);

static u8 fe_data[FE_BSS - FE_DATA];

void port_overlay_init(void) {
    memmove(fe_data, (void *)FE_DATA, sizeof fe_data);
}

void __wrap_func_8028B3E0(void) {
    u32 len = *D_802FDB34 - *D_802FDB30;

    osViBlack(1);
    if (D_80370C50 == 0) {
        func_8028B4C4(*D_802FDB30, (u8 *)FE_START, &len, 13, 10, 1);
        host_cpu_charge((FE_END - FE_START - len) / 4);     /* its bzero */
        memmove((void *)FE_DATA, fe_data, sizeof fe_data);
        memset((void *)FE_BSS, 0, FE_END - FE_BSS);
#ifdef PORT_ACCESS_PROFILE
        __port_access_set((void *)FE_DATA, FE_END - FE_DATA, 0);    /* the image's data again */
#endif
        D_80370C50 = 1;
        func_801F57B0();
        if (host_verbose)
            host_log("front end loaded\n");
    }
}
