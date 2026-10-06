/*
 * hd_code 69944 (us.v11 0x802AE104-0x802AE364): cos and sin, of a 12-bit
 * angle in 16.16 fixed point and of a float in radians, as native C
 * (engine.h).  Rare's own: range reduction by subtraction and an even or
 * odd polynomial, with its constants (D_80305C70...) in .rodata.
 */
#include "shared.h"

extern f32 D_80305C70;          /* 2 pi */
extern f32 D_80305C74;          /* pi */
extern f32 D_80305C78;          /* pi / 2 */
extern f32 D_80305C7C, D_80305C84, D_80305C8C, D_80305C94;  /* cos's coefficients */
extern f32 D_80305C80, D_80305C88, D_80305C90;              /* sin's */
extern f32 D_80305C9C;          /* 65536 */
extern f32 D_80305CA0;          /* 2 pi / 4096 */

#define ANGLE_MASK 0xFFF        /* a 12-bit angle: 0x1000 a full turn */

/* cos(x), x in radians */
REGS(f12 -> f0)
f32 func_802AE1BC(f32 x) {
    f32 twopi, pi, x2, x4, x6, x8, r;
    s32 neg = 0;

    twopi = D_80305C70;
    x = __builtin_fabsf(x);
    while (twopi < x) {
        x = x - twopi;
    }
    pi = D_80305C74;
    if (pi < x) {
        x = twopi - x;
    }
    if (D_80305C78 < x) {
        x = pi - x;
        neg = 1;
    }
    x2 = x * x;
    x4 = x2 * x2;
    x6 = x4 * x2;
    x8 = x4 * x4;
    r = 1.0f - x2 * D_80305C7C;
    r = r + x4 * D_80305C84;
    r = r - x6 * D_80305C8C;
    r = r + x8 * D_80305C94;
    if (neg) {
        r = -r;
    }
    return r;
}

/* sin(x), x in radians */
REGS(f12 -> f0)
f32 func_802AE290(f32 x) {
    f32 twopi, pi, x2, x4, x6, r;
    s32 neg = 0;

    twopi = D_80305C70;
    if (x < 0.0f) {
        x = __builtin_fabsf(x);
        neg = 1;
    }
    while (twopi < x) {
        x = x - twopi;
    }
    pi = D_80305C74;
    if (pi < x) {
        x = twopi - x;
        neg ^= 1;
    }
    if (D_80305C78 < x) {
        x = pi - x;
    }
    x2 = x * x;
    x4 = x2 * x2;
    x6 = x4 * x2;
    r = 1.0f - x2 * D_80305C80;
    r = r + x4 * D_80305C88;
    r = r - x6 * D_80305C90;
    r = r * x;
    if (neg) {
        r = -r;
    }
    return r;
}

/* cos of a 12-bit angle, 16.16 */
REGS(v1 -> fp)
s32 func_802AE104(s32 angle) {
    f32 c;

    c = func_802AE1BC((f32)(angle & ANGLE_MASK) * D_80305CA0);
    return engine_cvt_w_s(c * D_80305C9C);
}

/* sin of a 12-bit angle, 16.16 */
REGS(v1 -> fp)
s32 func_802AE160(s32 angle) {
    f32 s;

    s = func_802AE290((f32)(angle & ANGLE_MASK) * D_80305CA0);
    return engine_cvt_w_s(s * D_80305C9C);
}
