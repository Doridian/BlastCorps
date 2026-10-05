#ifndef COMMON_H
#define COMMON_H

#include <ultra64.h>

/*
 * The game was built against an older gbi.h than the 2.0I one here.  Where
 * the two build different display-list words, the older form is redefined.
 */

/* Fast3D 2.0D takes the perspective normalization as a command of its own
 * (0xB4, G_RDPHALF_1 in 2.0I); 2.0I moves it with G_MW_PERSPNORM. */
#undef gSPPerspNormalize
#define gSPPerspNormalize(pkt, s) gImmp1(pkt, G_RDPHALF_1, (s))

/* Texture rectangles: the older gbi.h sends s/t with G_RDPHALF_2 and
 * dsdx/dtdy with G_RDPHALF_CONT, and the scissoring version clamps with
 * plain MAX/MIN, without 2.0I's s16 casts and sign tests. */
#undef gSPTextureRectangle
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)    \
{                                                                           \
    Gfx *_g = (Gfx *)(pkt);                                                 \
                                                                            \
    _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) |       \
                    _SHIFTL(yh, 0, 12));                                    \
    _g->words.w1 = (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) |            \
                    _SHIFTL(yl, 0, 12));                                    \
    gImmp1(pkt, G_RDPHALF_2, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16)));     \
    gImmp1(pkt, G_RDPHALF_CONT, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16))); \
}
#undef gSPScisTextureRectangle
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
{                                                                           \
    Gfx *_g = (Gfx *)(pkt);                                                 \
                                                                            \
    _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(MAX(xh, 0), 12, 12) | \
                    _SHIFTL(MAX(yh, 0), 0, 12));                            \
    _g->words.w1 = (_SHIFTL(tile, 24, 3) | _SHIFTL(MAX(xl, 0), 12, 12) |    \
                    _SHIFTL(MAX(yl, 0), 0, 12));                            \
    gImmp1(pkt, G_RDPHALF_2,                                                \
           (_SHIFTL((s) - MIN(((xl) * (dsdx)) >> 7, 0), 16, 16) |           \
            _SHIFTL((t) - MIN(((yl) * (dtdy)) >> 7, 0), 0, 16)));           \
    gImmp1(pkt, G_RDPHALF_CONT, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16))); \
}

/* K0_TO_PHYS for a static initializer (a display list in .data): the linker
 * can add a constant to an address but not mask it, and every address such a
 * list points at is KSEG0. */
#if defined(TARGET_PC) && defined(PORT_LP64)
#define STATIC_K0_TO_PHYS(x) ((u32)(void *PTR32)(x) - K0BASE)    /* see gbi.h's _GBI_W */
#else
#define STATIC_K0_TO_PHYS(x) ((u32)(x) - K0BASE)
#endif

/*
 * The versions: the Makefile defines one of VERSION_US_V10, VERSION_US_V11,
 * VERSION_JP and VERSION_EU (the PC port builds us.v11).  Code that differs
 * is #if'd on them where it is.
 *
 * LINE_EU(us, eu): an assert's line number, which eu's source has elsewhere.
 * FRAMES_PER_SECOND: the game's frames, 50 a second in eu (PAL).
 */
#if !defined(VERSION_US_V10) && !defined(VERSION_JP) && !defined(VERSION_EU) && !defined(VERSION_US_V11)
#define VERSION_US_V11
#endif
#ifdef VERSION_EU
#define LINE_EU(us, eu) (eu)
#define FRAMES_PER_SECOND 50
#else
#define LINE_EU(us, eu) (us)
#define FRAMES_PER_SECOND 60
#endif

#endif
