/*
 * hd_code 679E0 (us.v11 0x802AC1A0-0x802ACFD0): Rare's math and matrix
 * routines, as native C (engine.h).
 */
#include "engine.h"

/* a u16 table in hd_code's .text, after func_802ACF64: 1025 samples of a
   curve, read with linear interpolation */
extern u16 D_802ACFD0[];

/* the table at x / 256, interpolated between the samples by the low byte */
REGS(v1 -> fp)
s32 func_802ACF64(u32 x) {
    u32 i;
    s32 a, b;

    ENGINE_BLK(802ACF64);
    i = x >> 8;
    if ((s32)i >= 0x3FF) {
        ENGINE_BLK(802ACF84);
        i = 0x3FF;
    }
    ENGINE_BLK(802ACF88);
    a = D_802ACFD0[i];
    b = D_802ACFD0[i + 1];
    return a + (s32)((u32)((b - a) * (s32)(x & 0xFF)) >> 8);
}

s32 func_802ACF3C(s32 x) {
    s32 r;

    ENGINE_BLK(802ACF3C);
    r = func_802ACF64(x);
    ENGINE_BLK(802ACF50);
    return r;
}
