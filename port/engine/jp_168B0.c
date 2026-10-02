/*
 * hd_code 168B0 (font.c: the text's glyphs), jp's func_8025B498, which the
 * C has as jp's GLOBAL_ASM, as native C charged by the original's blocks
 * (engine.h; jp_E7B0.c says more).
 */
#include "engine.h"

#ifdef VERSION_JP

extern f32 D_802E8C84[2];       /* a glyph's advance: char text's, u16 text's */

s32 func_8025B300(u8 *);
s32 func_8025B370(u16 *);

/* where a line of text starts for it to be centred on x at scale: the u16
   text (jp's) where there is some, else the char text */
s32 func_8025B498(s16 x, u16 scale, char *s, u16 *t) {
    s32 wide = 0, len;
    s16 r;
    f32 w;

    ENGINE_BLK(8025B498);
    if (t != NULL) {
        ENGINE_BLK(8025B4C0);
        if (*t != 0xFFF) {
            ENGINE_BLK(8025B4D0);
            wide = 1;
        }
    }
    ENGINE_BLK(8025B4D8);
    if (wide) {
        ENGINE_BLK(8025B4E4);
        len = func_8025B370(t);
        ENGINE_BLK(8025B4EC);
        w = D_802E8C84[1] * (f32)(len - 1) + 1.0f;
        /* (8025B528 adds 2^32 to a negative scale: a u16 never is) */
        ENGINE_BLK(8025B538);
    } else {
        ENGINE_BLK(8025B570);
        len = func_8025B300((u8 *)s);
        ENGINE_BLK(8025B578);
        w = D_802E8C84[0] * (f32)(len - 1) + 1.0f;
        ENGINE_BLK(8025B5C4);
    }
    r = engine_trunc_w_d((f64)x - (f64)(w * (f32)scale) / 2.0);
    ENGINE_BLK(8025B5FC);
    return r;
}

#endif
