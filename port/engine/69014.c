/*
 * hd_code 69014 (us.v11 0x802AD7D4-0x802AD880): a table lookup with
 * linear interpolation, as native C (engine.h).  The table, 1025 u16
 * samples in 64ths, sits in hd_code's .text right after it (690C0); the
 * game's C uses it for angles from a ratio (an arctangent).
 */
#include "shared.h"

extern u16 D_802AD880[];        /* the table: 1025 samples */
extern u16 D_802AE084[];        /* (the table's last samples, read flat) */

#define STEP_BITS 6             /* x's bits between two samples */
#define STEP_MASK ((1 << STEP_BITS) - 1)
#define LAST_STEP 0x3FF         /* the last interval: from there on flat */

/* the table at x (its samples 1 << STEP_BITS apart), interpolated */
REGS(v1 -> fp)
s32 func_802AD7FC(u32 x) {
    u32 i = x >> STEP_BITS;
    u32 frac = x & STEP_MASK;
    s32 a, b;

    ENGINE_BLK(802AD7FC);
    if ((s32)i >= LAST_STEP) {
        ENGINE_BLK(802AD81C);
        a = D_802AE084[frac];
    } else {
        ENGINE_BLK(802AD838);
        a = D_802AD880[i];
        b = D_802AD880[i + 1];
        /* (an unsigned shift of the signed product, as the original) */
        a = a + (s32)((u32)((b - a) * (s32)frac) >> STEP_BITS);
    }
    ENGINE_BLK(802AD868);
    return a;
}

s32 func_802AD7D4(s32 x) {
    s32 r;

    ENGINE_BLK(802AD7D4);
    r = func_802AD7FC(x);
    ENGINE_BLK(802AD7E8);
    return r;
}
