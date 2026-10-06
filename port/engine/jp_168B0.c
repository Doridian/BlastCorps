/*
 * hd_code 168B0 (font.c: the text's glyphs), jp's func_8025B498, which the
 * C has as jp's GLOBAL_ASM, as native C (engine.h; jp_E7B0.c says more).
 */
#include "engine.h"

#ifdef VERSION_JP

extern f32 D_802E8C84[2];       /* a glyph's advance: char text's, u16 text's */

/* where a line of text starts for it to be centred on x at scale: the u16
   text (jp's) where there is some, else the char text */
s32 func_8025B498(s16 x, u16 scale, char *s, u16 *t) {
    s32 wide = 0, len;
    s16 r;
    f32 w;

    if (t != NULL) {
        if (*t != 0xFFF) {
            wide = 1;
        }
    }
    if (wide) {
        len = func_8025B370(t);
        w = D_802E8C84[1] * (f32)(len - 1) + 1.0f;
        /* (8025B528 adds 2^32 to a negative scale: a u16 never is) */
    } else {
        len = func_8025B300((u8 *)s);
        w = D_802E8C84[0] * (f32)(len - 1) + 1.0f;
    }
    r = engine_trunc_w_d((f64)x - (f64)(w * (f32)scale) / 2.0);
    return r;
}

#endif
