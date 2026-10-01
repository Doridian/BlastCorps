/*
 * hd_code 69944 (us.v11 0x802AE104-0x802AE364): cos and sin, of a 12-bit
 * angle in 16.16 fixed point and of a float in radians, as native C
 * (engine.h).  Rare's own: range reduction by subtraction and an even or
 * odd polynomial, with its constants (D_80305C70...) in .rodata.
 */
#include "engine.h"

extern f32 D_80305C70;          /* 2 pi */
extern f32 D_80305C74;          /* pi */
extern f32 D_80305C78;          /* pi / 2 */
extern f32 D_80305C7C, D_80305C84, D_80305C8C, D_80305C94;  /* cos's coefficients */
extern f32 D_80305C80, D_80305C88, D_80305C90;              /* sin's */
extern f32 D_80305C9C;          /* 65536 */
extern f32 D_80305CA0;          /* 2 pi / 4096 */

/* cos(x), x in radians */
REGS(f12 -> f0)
f32 func_802AE1BC(f32 x) {
    f32 twopi, pi, x2, x4, x6, x8, r;
    s32 neg = 0;

    ENGINE_BLK(802AE1BC);
    twopi = D_80305C70;
    x = __builtin_fabsf(x);
    if (twopi < x) {
        do {
            ENGINE_BLK(802AE1D8);
            x = x - twopi;
        } while (twopi < x);
    }
    ENGINE_BLK(802AE1E8);
    pi = D_80305C74;
    if (pi < x) {
        ENGINE_BLK(802AE1FC);
        x = twopi - x;
    }
    ENGINE_BLK(802AE200);
    if (D_80305C78 < x) {
        ENGINE_BLK(802AE218);
        x = pi - x;
        neg = 1;
    }
    ENGINE_BLK(802AE220);
    x2 = x * x;
    x4 = x2 * x2;
    x6 = x4 * x2;
    x8 = x4 * x4;
    r = 1.0f - x2 * D_80305C7C;
    r = r + x4 * D_80305C84;
    r = r - x6 * D_80305C8C;
    r = r + x8 * D_80305C94;
    if (neg) {
        ENGINE_BLK(802AE284);
        r = -r;
    }
    ENGINE_BLK(802AE288);
    return r;
}

/* sin(x), x in radians */
REGS(f12 -> f0)
f32 func_802AE290(f32 x) {
    f32 twopi, pi, x2, x4, x6, r;
    s32 neg = 0;

    ENGINE_BLK(802AE290);
    twopi = D_80305C70;
    if (x < 0.0f) {
        ENGINE_BLK(802AE2B0);
        x = __builtin_fabsf(x);
        neg = 1;
    }
    ENGINE_BLK(802AE2B8);
    if (twopi < x) {
        do {
            ENGINE_BLK(802AE2C4);
            x = x - twopi;
        } while (twopi < x);
    }
    ENGINE_BLK(802AE2D4);
    pi = D_80305C74;
    if (pi < x) {
        ENGINE_BLK(802AE2E8);
        x = twopi - x;
        neg ^= 1;
    }
    ENGINE_BLK(802AE2F0);
    if (D_80305C78 < x) {
        ENGINE_BLK(802AE304);
        x = pi - x;
    }
    ENGINE_BLK(802AE308);
    x2 = x * x;
    x4 = x2 * x2;
    x6 = x4 * x2;
    r = 1.0f - x2 * D_80305C80;
    r = r + x4 * D_80305C88;
    r = r - x6 * D_80305C90;
    r = r * x;
    if (neg) {
        ENGINE_BLK(802AE35C);
        r = -r;
    }
    ENGINE_BLK(802AE360);
    return r;
}

/* cos of a 12-bit angle, 16.16 */
REGS(v1 -> fp)
s32 func_802AE104(s32 angle) {
    f32 c;

    ENGINE_BLK(802AE104);
    c = func_802AE1BC((f32)(angle & 0xFFF) * D_80305CA0);
    ENGINE_BLK(802AE134);
    return engine_cvt_w_s(c * D_80305C9C);
}

/* sin of a 12-bit angle, 16.16 */
REGS(v1 -> fp)
s32 func_802AE160(s32 angle) {
    f32 s;

    ENGINE_BLK(802AE160);
    s = func_802AE290((f32)(angle & 0xFFF) * D_80305CA0);
    ENGINE_BLK(802AE190);
    return engine_cvt_w_s(s * D_80305C9C);
}
