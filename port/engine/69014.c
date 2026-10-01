/*
 * hd_code 69014 (us.v11 0x802AD7D4-0x802AD880): a table lookup with
 * linear interpolation, as native C (engine.h).  The table, 1025 u16
 * samples in 64ths, sits in hd_code's .text right after it (690C0); the
 * game's C uses it for angles from a ratio (an arctangent).
 */
#include "shared.h"

extern u16 D_802AD880[];
extern u16 D_802AE084[];        /* (the table's last samples, read flat) */

REGS(v1 -> fp)
s32 func_802AD7FC(u32 x) {
    u32 i = x >> 6;
    s32 a, b;

    ENGINE_BLK(802AD7FC);
    if ((s32)i >= 0x3FF) {
        ENGINE_BLK(802AD81C);
        a = D_802AE084[x & 0x3F];
    } else {
        ENGINE_BLK(802AD838);
        a = D_802AD880[i];
        b = D_802AD880[i + 1];
        a = a + (s32)((u32)((b - a) * (s32)(x & 0x3F)) >> 6);
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
