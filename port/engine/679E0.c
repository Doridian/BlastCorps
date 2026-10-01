/*
 * hd_code 679E0 (us.v11 0x802AC1A0-0x802ACFD0): Rare's math and matrix
 * routines, and a few helpers that sit with them, as native C (engine.h).
 *
 * Rare's matrices here are 4x4 s32 in 16.16 fixed point, row-major, the
 * translation in the last row (m[12..14]), as the RSP's Mtx holds them
 * before func_802AC8CC splits them into its integer and fraction halves.
 *
 * Not here yet: func_802AC284 and func_802AC2A4, which only the truck's
 * code (71140) calls, with a register convention that is all pass-through
 * (they go with 71140), and func_802ACDB8/func_802ACEB8, which nothing
 * calls.
 */
#include "shared.h"
#include "game/game.h"
#include "game/audio.h"

/* ---- what this calls ------------------------------------------------- */

/* 69944: cos and sin of a 12-bit angle, 16.16 */
REGS(v1 -> fp)
s32 func_802AE104(s32 angle);
REGS(v1 -> fp)
s32 func_802AE160(s32 angle);
/* 62740: the distance between two points */
REGS(t3, t4, t5, t6, t7, s0 -> s1+f0)
s64 func_802ABCDC(s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2);
/* 62740: whether (x, z) is inside a quadrilateral */
REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA460(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3);
/* 60F60: start an effect (a debris or smoke particle) */
REGS(t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, a3 -> t0)
s32 func_802A6274(s32 t0, s32 t1, s32 t2, s32 t3, s32 t4, s32 t5, s32 t6, s32 t7, s32 s0, s32 s1, s32 s2,
                  s32 s3, s32 s4, s32 s5, s32 a3);
/* (a function of the same) */
REGS(a1, t3, t4, t5, t6)
void func_802C18D4(s32 a1, s32 x, s32 y, s32 z, s32 t6);
/* 39050.c's */
extern s16 D_8036E4C8;

extern u8 D_803F4030[];         /* 0xFC-byte records up to D_803F7654 */
extern u8 *PTR32 D_803F7654;
extern s32 D_803EF6DC, D_803EF6E0, D_803EF6E4;
extern s32 D_802E8BDC;
extern u8 D_80364412;
extern u32 D_802C3FFC[];
extern u8 D_802C2A5C[];
extern s32 D_803ED420[16];

void func_802AC6FC(s32 x, s32 y, s32 z, s32 kind, s32 t1);

/* ---- the functions --------------------------------------------------- */

/* Each record at D_803F4030 within `radius` of (D_803EF6DC, ...) whose
   byte 0xEA is clear gets func_802C18D4. */
void func_802AC1A0(s32 radius) {
    u8 *p, *end;
    s32 x, y, z;

    ENGINE_BLK(802AC1A0);
    radius <<= 5;
    p = D_803F4030;
    end = D_803F7654;
    for (;;) {
        ENGINE_BLK(802AC1E4);
        if (p == end)
            break;
        ENGINE_BLK(802AC1EC);
        if (p[0xEA] == 0) {
            ENGINE_BLK(802AC1F8);
            x = *(s32 *)(p + 0x10);
            y = *(s32 *)(p + 0x14);
            z = *(s32 *)(p + 0x18);
            if (func_802ABCDC(x, y, z, D_803EF6DC, D_803EF6E0, D_803EF6E4) < radius) {
                ENGINE_BLK(802AC220);
                ENGINE_BLK(802AC22C);
                func_802C18D4(10, x << 11, y << 11, z << 11, 100000);
            } else {
                ENGINE_BLK(802AC220);
            }
        }
        ENGINE_BLK(802AC248);
        p += 0xFC;
    }
    ENGINE_BLK(802AC250);
}

/* The level that wraps round (D_802E8BDC 0x17): x and z kept inside the
   map; when they wrap, an effect at the new position. */
REGS(v0, v1, a0)
void func_802AC3B8(s32 *x, s32 *y, s32 *z) {
    s32 t;

    ENGINE_BLK(802AC3B8);
    if (D_802E8BDC == 0x17) {
        ENGINE_BLK(802AC3D8);
        t = *x;
        if (t >= 0x15181) {
            ENGINE_BLK(802AC3F0);
            *x = t = 0x2BC0;
            D_80364412 = 1;
        }
        ENGINE_BLK(802AC404);
        if (t < 0x2581) {
            ENGINE_BLK(802AC410);
            *x = 0x14B40;
            D_80364412 = 1;
        }
        ENGINE_BLK(802AC428);
        t = *z;
        if (t >= 0x13881) {
            ENGINE_BLK(802AC440);
            *z = t = 0x44C0;
            D_80364412 = 1;
        }
        ENGINE_BLK(802AC454);
        if (t < 0x3E81) {
            ENGINE_BLK(802AC460);
            *z = 0x13240;
            D_80364412 = 1;
        }
        ENGINE_BLK(802AC478);
        if (D_80364412 != 0) {
            ENGINE_BLK(802AC488);
            func_802AC6FC(*x, *y, *z, 0x13, 1000000);
            ENGINE_BLK(802AC4AC);
        }
    }
    ENGINE_BLK(802AC4B0);
}

/* whether (x, z) is inside the quadrilateral of the other four points */
s32 func_802AC4C4(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    s32 r;

    ENGINE_BLK(802AC4C4);
    r = func_802AA460(x, z, x1, z1, x2, z2, x3, z3);
    ENGINE_BLK(802AC514);
    return r;
}

/* effects at a point (whole units): one of D_802C2A5C's */
void func_802AC544(s32 x, s32 y, s32 z) {
    ENGINE_BLK(802AC544);
    func_802A6274((s32)D_802C2A5C, 0x28488, 0, x << 16, y << 16, z << 16, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ENGINE_BLK(802AC5EC);
}

/* effects at a position (<< 5), of kind `kind` (D_802C3FFC) */
void func_802AC61C(s32 x, s32 y, s32 z, s32 kind, s32 t1) {
    ENGINE_BLK(802AC61C);
    func_802A6274(D_802C3FFC[kind], t1, 0, x << 11, y << 11, z << 11, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ENGINE_BLK(802AC6CC);
}

/* the same with a3 = 1 */
void func_802AC6FC(s32 x, s32 y, s32 z, s32 kind, s32 t1) {
    ENGINE_BLK(802AC6FC);
    func_802A6274(D_802C3FFC[kind], t1, 0, x << 11, y << 11, z << 11, 0, 0, 0, 0, 0, 0, 0, 0, 1);
    ENGINE_BLK(802AC7AC);
}

/* an unaligned word, as swl/swr and lwl/lwr move it */
typedef struct {
    u32 v;
} __attribute__((packed)) UnalignedWord;

/* 0xA6 bytes from src, then three words, to dst (0xB2 bytes) */
REGS(a0, a1, a2)
void func_802AC7DC(u8 *dst, u8 *src, u32 *words) {
    s32 n;

    ENGINE_BLK(802AC7DC);
    for (n = 0xA6;; n--) {
        ENGINE_BLK(802AC7F8);
        if (n == 0)
            break;
        ENGINE_BLK(802AC800);
        *dst++ = *src++;
    }
    ENGINE_BLK(802AC818);
    ((UnalignedWord *)dst)[0].v = words[0];
    ((UnalignedWord *)dst)[1].v = words[1];
    ((UnalignedWord *)dst)[2].v = words[2];
}

/* the reverse: 0xA6 bytes from src to dst, then three words to `words` */
REGS(a0, a1, a2)
void func_802AC85C(u8 *src, u8 *dst, u32 *words) {
    s32 n;

    ENGINE_BLK(802AC85C);
    for (n = 0xA6;; n--) {
        ENGINE_BLK(802AC870);
        if (n == 0)
            break;
        ENGINE_BLK(802AC878);
        *dst++ = *src++;
    }
    ENGINE_BLK(802AC890);
    words[0] = ((UnalignedWord *)src)[0].v;
    words[1] = ((UnalignedWord *)src)[1].v;
    words[2] = ((UnalignedWord *)src)[2].v;
}

/* A 16.16 matrix in place to the RSP's Mtx: the integer halves of the 16
   words, then their fractions.  (By words: a halfword's address is
   another in native-endian memory.) */
REGS(s2)
void func_802AC8CC(u32 *m) {
    u32 w[16];
    s32 k;

    ENGINE_BLK(802AC8CC);
    for (k = 0; k < 16; k++)
        w[k] = m[k];
    for (k = 0; k < 8; k++) {
        m[k] = (w[2 * k] & 0xFFFF0000) | (w[2 * k + 1] >> 16);
        m[8 + k] = (w[2 * k] << 16) | (w[2 * k + 1] & 0xFFFF);
    }
}

/* a translation */
REGS(t0, t1, t2, t3)
void func_802ACA60(s32 x, s32 y, s32 z, s32 *m) {
    ENGINE_BLK(802ACA60);
    m[0] = 0x10000, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = 0x10000, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = 0x10000, m[11] = 0;
    m[12] = x, m[13] = y, m[14] = z, m[15] = 0x10000;
}

/* a rotation about y */
REGS(t0, t3)
void func_802ACAC4(s32 angle, s32 *m) {
    s32 s, c;

    ENGINE_BLK(802ACAC4);
    s = func_802AE160(angle);
    ENGINE_BLK(802ACAE4);
    c = func_802AE104(angle);
    ENGINE_BLK(802ACAEC);
    m[0] = c, m[1] = 0, m[2] = -s, m[3] = 0;
    m[4] = 0, m[5] = 0x10000, m[6] = 0, m[7] = 0;
    m[8] = s, m[9] = 0, m[10] = c, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
}

/* a rotation about z */
REGS(t0, t3)
void func_802ACB50(s32 angle, s32 *m) {
    s32 s, c;

    ENGINE_BLK(802ACB50);
    s = func_802AE160(angle);
    ENGINE_BLK(802ACB70);
    c = func_802AE104(angle);
    ENGINE_BLK(802ACB78);
    m[0] = c, m[1] = s, m[2] = 0, m[3] = 0;
    m[4] = -s, m[5] = c, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = 0x10000, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
}

/* a rotation about x */
REGS(t0, t3)
void func_802ACBDC(s32 angle, s32 *m) {
    s32 s, c;

    ENGINE_BLK(802ACBDC);
    s = func_802AE160(angle);
    ENGINE_BLK(802ACBFC);
    c = func_802AE104(angle);
    ENGINE_BLK(802ACC04);
    m[0] = 0x10000, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = c, m[6] = s, m[7] = 0;
    m[8] = 0, m[9] = -s, m[10] = c, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
}

/* a scale */
REGS(t0, t1, t2, t3)
void func_802ACC68(s32 x, s32 y, s32 z, s32 *m) {
    ENGINE_BLK(802ACC68);
    m[0] = x, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = y, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = z, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
}

/* a = a * b, through D_803ED420 */
REGS(a0, s2)
void func_802ACCCC(s32 *b, s32 *a) {
    s32 i, j;
    s32 *d = D_803ED420;

    ENGINE_BLK(802ACCCC);
    for (i = 0; i < 4; i++) {
        ENGINE_BLK(802ACCEC);
        for (j = 0; j < 4; j++) {
            ENGINE_BLK(802ACCF8);
            *d++ = (s32)(((s64)a[4 * i] * b[j] + (s64)a[4 * i + 1] * b[4 + j] + (s64)a[4 * i + 2] * b[8 + j] +
                          (s64)a[4 * i + 3] * b[12 + j]) >> 16);
        }
        ENGINE_BLK(802ACD6C);
    }
    ENGINE_BLK(802ACD7C);
    for (i = 0; i < 16; i++) {
        ENGINE_BLK(802ACD88);
        a[i] = D_803ED420[i];
    }
    ENGINE_BLK(802ACDA0);
    /* (the loops' registers, as it leaves them for the translated code) */
    ENGINE_LEAVE(1, 4);
    ENGINE_LEAVE(4, (u32)b);
    ENGINE_LEAVE(5, (u32)(D_803ED420 + 16));
    ENGINE_LEAVE(6, 4);
    ENGINE_LEAVE(7, 0);
    ENGINE_LEAVE(8, D_803ED420[15]);
    ENGINE_LEAVE(9, (u32)(D_803ED420 + 16));
    ENGINE_LEAVE(11, b[15]);
    ENGINE_LEAVE(18, (u32)a);
}

/* (x, z) rotated by the angle about y, 16.16: x' (a3) and z' (t1), and
   x * sin (a1, as the code leaves it: 64 bits) */
REGS(a0, a2, a3 -> a1, a3, t1)
s64 func_802ACE38(s64 x, s64 z, s32 angle, s32 *xr, s32 *zr) {
    s64 c, s;

    ENGINE_BLK(802ACE38);
    c = func_802AE104(angle);
    ENGINE_BLK(802ACE50);
    s = func_802AE160(angle);
    ENGINE_BLK(802ACE5C);
    *xr = (s32)((x * c) >> 16) + (s32)((z * s) >> 16);
    *zr = (s32)((z * c) >> 16) - (s32)((x * s) >> 16);
    return (x * s) >> 16;
}

/* the table at x / 256, interpolated between its samples by the low byte */
extern u16 D_802ACFD0[];

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
