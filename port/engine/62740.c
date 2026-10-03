/*
 * hd_code 62740 (us.v11 0x802A6F00-0x802AC1A0): the vehicles' shared
 * physics, as native C (engine.h).  Most of it works on a VehicleState
 * (vehicle.h) that every vehicle module passes in $gp: its heading
 * (unk4C, unk4E: 12-bit angles), its speed (unk76), its gear table (unk78:
 * five (s16, s16, s16) rows) and its flags.
 *
 * A few registers stay in the context (ENGINE_LEAVE) because the other
 * half of port/engine still reads them from there: 5CB60.c's collision
 * triangles, 60F60.c, 69BB0.c's driver and the other vehicles' setups
 * (docs/PORT.md, "Replacing the engine").
 */
#include "shared.h"
#include "game/vehicle.h"
#include "game/game.h"


extern u8 D_80367C10;
extern s16 D_803A7410, D_803A7412;      /* the camera's headings (12-bit) */
extern s32 D_80358064;
extern u8 D_8035805C;
extern s8 D_80370C2D;
extern u8 D_803EB7A0[];                 /* a vehicle's parts, state and position, saved */

/* an unaligned word, as swl/swr and lwl/lwr move it */
typedef struct {
    u32 v;
} __attribute__((packed)) UnalignedWord;

/* gear table rows doubled (unk78[3k], unk78[3k + 1]) when D_80367C10 */
REGS(gp)
void func_802A6F00(VS *vs) {
    s16 *p;
    s32 n;

    ENGINE_COST(802A6F00, 16);
    if (D_80367C10 != 0) {
        p = vs->unk78;
        for (n = 5;; n--) {
            if (n == 0)
                break;
            p[0] = p[0] << 1;
            p[1] = p[1] << 1;
            p += 3;
        }
    }
}

/* the angle halfway between the two camera headings (12-bit, wrapping) */
s32 func_802A6F6C(void) {
    s32 a = (u16)D_803A7410, b = (u16)D_803A7412, r;

    ENGINE_COST(802A6F6C, 21);
    if (b < a) {
        r = ((0xFFF - a + b) >> 1) + a;
        if (r >= 0x1000) {
            r -= 0xFFF;
        }
    } else {
        r = (u32)(a + b) >> 1;
    }
    return r;
}

/* the speed brought toward a1 by 3 (D_80358064), and its sign in unkA5 */
REGS(a1, gp)
void func_802A6FE4(s32 limit, VS *vs) {
    s32 v;

    ENGINE_COST(802A6FE4, 25);
    if (D_80358064 != 0) {
        v = vs->unk76;
        if (v > 0) {
            vs->unkA5 = 1;
        } else {
            vs->unkA5 = -1;
        }
        if (v >= 0) {
            v -= 3;
            if (!(limit < v)) {
                v = limit;
            }
        } else {
            v += 3;
            if (!(v < -limit)) {
                v = -limit;
            }
        }
        vs->unk76 = v;
    }
}

/* unk4E: the heading *t7, turned round when the speed's sign isn't unkA5 */
REGS(t7, gp)
void func_802A7070(s16 *heading, VS *vs) {
    s32 sign, v;

    ENGINE_COST(802A7070, 19);
    if (vs->unk76 > 0) {
        sign = 1;
    } else {
        sign = -1;
    }
    if (vs->unkA5 == sign) {
        v = *heading;
    } else {
        v = *heading - 0x800;
        if (v < 0) {
            v += 0xFFF;
        }
    }
    vs->unk4E = v;
}

/* when unk4E is more than a quarter turn from unk4C: turned round, the
   speed negated, and the camera's headings turned half round */
REGS(gp)
void func_802A70D8(VS *vs) {
    s32 h = vs->unk4C, lo, hi, a;

    ENGINE_COST(802A70D8, 31);
    lo = h - 0x400;
    if (lo < 0) {
        lo += 0xFFF;
    }
    hi = h + 0x400;
    if (hi >= 0x1000) {
        hi -= 0xFFF;
    }
    a = vs->unk4E;
    if (hi < lo) {
        if (!(a < lo))
            goto done;
        if (!(hi < a))
            goto done;
    } else {
        if (!(a < lo)) {
            if (!(hi < a))
                goto done;
        }
    }
    a += 0x800;
    if (a >= 0x1000) {
        a -= 0xFFF;
    }
    vs->unk4E = a;
    vs->unk76 = -vs->unk76;
    a = (u16)D_803A7410 - 0x800;
    if (a < 0) {
        a += 0xFFF;
    }
    D_803A7410 = a;
    a = (u16)D_803A7412 + 0x800;
    if (0xFFF < a) {
        a -= 0xFFF;
    }
    D_803A7412 = a;
done:
    ;
}

/* unk4C turned by `turn` toward unk4E, not past it */
REGS(a1, gp)
void func_802A746C(s32 turn, VS *vs) {
    s32 h = vs->unk4C, target = vs->unk4E, n;

    ENGINE_COST(802A746C, 21);
    if (h == target)
        goto done;
    n = h + turn;
    if (n < 0) {
        n += 0xFFF;
    }
    if (n >= 0x1000) {
        n -= 0xFFF;
    }
    if (turn >= 0) {
        if (h < target) {
            if (target < n)
                goto clamp;
            if (n < h)
                goto clamp;
            goto set;
        }
        if (!(target < h))
            goto set;
        if (!(n < h))
            goto set;
        if (!(target < n))
            goto set;
        goto clamp;
    }
    if (target < h) {
        if (!(target < n))
            goto clamp;
        if (!(n < h))
            goto clamp;
        goto set;
    }
    if (!(h < target))
        goto set;
    if (!(n < target))
        goto set;
    if (!(h < n))
        goto set;
clamp:
    vs->unk4C = target;
    goto done;
set:
    vs->unk4C = n;
done:
    ;
}

/* the state at rest */
REGS(gp)
void func_802A754C(VS *vs) {
    s32 k;

    ENGINE_COST(802A754C, 36);
    vs->unk96[0] = 0;
    vs->unk96[1] = 0;
    vs->unk96[2] = 0;
    vs->unk50 = 1;
    vs->unkA0 = 1;
    for (k = 0; k < 9; k++)
        vs->unk28[k] = 0;
    vs->unk76 = 0;
    vs->unk96[3] = 0;
    vs->unk9B = 0;
    vs->unk9C = 0;
    vs->unk9D = 0;
    vs->unk9E = 0;
    vs->unkA1 = 0;
    vs->unkA2 = 0;
    vs->unkA3 = 0;
    vs->unkA4 = 0;
    vs->unkA5 = 0;
    vs->unk0 = 0.0f;
}

/* the current vehicle's parts (0x300 bytes), state (0xA6) and position
   (three words) to D_803EB7A0 */
REGS(v0, v1, a0, a1, gp)
void func_802A75DC(u8 *parts, s32 *x, s32 *y, s32 *z, u8 *vs) {
    u8 *d = D_803EB7A0;
    s32 n;

    ENGINE_COST(802A75DC, 7504);
    for (n = 0x300;; n--) {
        if (n == 0)
            break;
        *d++ = *parts++;
    }
    for (n = 0xA6;; n--) {
        if (n == 0)
            break;
        *d++ = *vs++;
    }
    ((UnalignedWord *)d)[0].v = *x;
    ((UnalignedWord *)d)[1].v = *y;
    ((UnalignedWord *)d)[2].v = *z;
}

/* ... and back, with `n` bytes from src to dst (in doublewords) */
REGS(v0, v1, a0, a1, a2, a3, t0, gp)
void func_802A768C(u8 *parts, s32 *x, s32 *y, s32 *z, u32 *src, u32 *dst, s32 n, u8 *vs) {
    u8 *s = D_803EB7A0;
    s32 k;

    ENGINE_COST(802A768C, 9529);
    for (k = 0x300;; k--) {
        if (k == 0)
            break;
        *parts++ = *s++;
    }
    for (k = 0xA6;; k--) {
        if (k == 0)
            break;
        *vs++ = *s++;
    }
    for (;;) {
        if (n == 0)
            break;
        dst[0] = src[0];
        dst[1] = src[1];
        src += 2, dst += 2, n -= 8;
    }
    *x = ((UnalignedWord *)s)[0].v;
    *y = ((UnalignedWord *)s)[1].v;
    *z = ((UnalignedWord *)s)[2].v;
}

/* n bytes (in doublewords) from a to b, or b to a when D_8035805C */
REGS(a0, a1, a2)
void func_802A7764(u32 *a, u32 *b, s32 n) {
    u32 *t;

    ENGINE_COST(802A7764, 3293);
    if (D_8035805C != 0) {
        t = a, a = b, b = t;
    }
    for (;;) {
        if (n == 0)
            break;
        b[0] = a[0];
        b[1] = a[1];
        a += 2, b += 2, n -= 8;
    }
}

/* the speed toward 0 by 8, unless D_80370C2D */
REGS(gp)
void func_802A77D0(VS *vs) {
    s32 v;

    ENGINE_COST(802A77D0, 12);
    if (D_80370C2D == 0) {
        v = vs->unk76;
        if (v >= 0) {
            v -= 8;
            if (v < 0) {
                v = 0;
            }
        } else {
            v += 8;
            if (v > 0) {
                v = 0;
            }
        }
        vs->unk76 = v;
    }
}

/* whether x is past the first gear row's (unk78[0]) speed scaled by the
   stick (D_80370C2D / -80) */
REGS(t2, s1 -> t4)
s32 func_802A7A1C(s32 x, s16 *rows) {
    s32 r = 0;

    ENGINE_COST(802A7A1C, 32);
    if (x < rows[0] * D_80370C2D / -0x50) {
        r = 1;
    }
    return r;
}

/* the same against the last row's (unk78[13]), / 80 */
REGS(t2, s1 -> t4)
s32 func_802A7AAC(s32 x, s16 *rows) {
    s32 r = 0;

    ENGINE_COST(802A7AAC, 32);
    if (rows[13] * D_80370C2D / 0x50 < x) {
        r = 1;
    }
    return r;
}

/* the gear (third value) of the row whose range [row[0], row[1]] holds x,
   over the rows the vehicle's gears use (unk50); 0 if none.  The rows
   pointer is left past the last row looked at. */
REGS(t2, s1, gp -> t4, s1)
s32 func_802A7C28(s32 x, s16 *rows, VS *vs, u32 *rows_out) {
    s32 n, r = 0;

    ENGINE_COST(802A7C28, 64);
    if (vs->unk50 == 1) {
        n = 5;
    } else {
        n = 7 - vs->unk50;
    }
    if ((s8)D_803ED40C != 0) {
        n -= 2;
    }
    for (;;) {
        if (n == 0)
            break;
        n--;
        rows += 3;
        if (x < rows[-3])
            continue;
        if (rows[-2] < x)
            continue;
        r = rows[-1];
        break;
    }
    *rows_out = (u32)rows;
    return r;
}

/* whether the speed is within `range` of the current gear's top speed
   (the first row's when reversing) */
REGS(v1, gp -> v0)
s32 func_802A7CB0(s32 range, VS *vs) {
    s32 v = vs->unk76, g;

    ENGINE_COST(802A7CB0, 34);
    if (v >= 0) {
        if (vs->unk50 == 1) {
            g = 4;
        } else {
            g = 6 - vs->unk50;
        }
        v -= *(s16 *)((u8 *)vs->unk78 + (u32)g * 6 + 2);
        if (v < 0) {
            v = -v;
        }
        if (!(v < range))
            goto no;
    } else {
        v -= vs->unk78[0];
        if (v < 0) {
            v = -v;
        }
        if (!(v < range))
            goto no;
    }
    return 1;
no:
    return 0;
}

/* the gear the flags (unk96[0..2]) allow, for the gearbox's mode */
REGS(t7, s0 -> t3)
s32 func_802A7D68(s32 mode, u8 *flags) {
    s32 n, r;

    ENGINE_COST(802A7D68, 23);
    if (mode == 0) {
        if (flags[0] != 0) {
            if (flags[1] == 0) {
                r = 2;
            } else {
                r = 0;
            }
        } else {
            if (flags[1] == 0) {
                r = 4;
            } else {
                r = 2;
            }
        }
        goto done;
    }
    if (mode == 1) {
        if (flags[2] == 0) {
            r = 4;
        } else {
            r = 0;
        }
        goto done;
    }
    if (mode == 2) {
        r = 0;
        if (flags[0] == 0) {
            r += 1;
        }
        if (flags[1] == 0) {
            r += 1;
        }
        if (flags[2] == 0) {
            r += 2;
        }
        goto done;
    }
    if (mode == 4) {
        r = 4;
        goto done;
    }
    for (n = 3;;) {
        if (*flags == 0) {
            r = 4;
            goto done;
        }
        flags++;
        if (--n == 0)
            break;
    }
    r = 0;
done:
    return r;
}

extern s8 D_80370C2C;           /* the stick's x */
extern s8 D_80370C34;
extern u16 D_80370C70;
extern s16 D_80370C72;
extern s8 D_803649EE;

/* steering: the heading *h turned by the stick's x times the turn rate
   (s3; a third less with D_80367C10, fixed with D_80370C75), / 80.
   Returns the turn, and the stick's x and its address (t2, t0) as the
   code leaves them for its callers. */
REGS(s3, s4 -> s3, t0, t2)
s32 func_802A7E70(s32 rate, u16 *h, u32 *stick_addr, s32 *stick) {
    s32 x, a, t;

    ENGINE_COST(802A7E70, 48);
    if (D_80367C10 != 0) {
        rate -= rate / 3;
    }
    if (D_80370C75 != 0) {
        if (D_80370C34 != 0) {
            rate = 0x64;
        } else {
            rate = 0xC8;
        }
    }
    D_80370C72 = rate;
    x = D_80370C2C;
    t = *h;
    a = x;
    if (x < 0) {
        a = -x;
    }
    rate = a * rate / 0x50;
    if (x != 0) {
        if (x >= 0) {
            t -= rate;
        } else {
            t += rate;
        }
        if (t < 0) {
            t += 0x1000;
        }
        if (t >= 0x1000) {
            t -= 0x1000;
        }
        *h = t;
    }
    if (D_803ED40A != 0) {
        if (D_803649EE == 0) {
            t = D_803ED408;
        }
    }
    *h = t;
    D_80370C70 = t;
    *stick_addr = (u32)&D_80370C2C;
    *stick = x;
    return rate;
}

/* whether (x, z) is inside the triangle (x1, z1), (x2, z2), (x3, z3): on
   the same side of each edge as the point halfway between the first
   corner and the middle of the opposite edge */
REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA460(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    f32 fx, fz, cx, cz, ex, ez, d, c = 2.0f, a, b, b2, dx, dz;
    s32 edge, r = 1, tx, tz;

    ENGINE_COST(802AA460, 118);
    cx = (f32)(x2 + x3) / 2.0f;
    cz = (f32)(z2 + z3) / 2.0f;
    cx = ((f32)x1 + cx) / 2.0f;
    cz = ((f32)z1 + cz) / 2.0f;
    fx = (f32)x;
    fz = (f32)z;
    for (edge = 4;;) {
        if (--edge == 0)
            break;
        if (edge == 3) {
            ex = (f32)x1, ez = (f32)z1;
            tx = z2 - z1, tz = x2 - x1;
        } else {
            if (edge == 2) {
                ex = (f32)x1, ez = (f32)z1;
                tx = z3 - z1, tz = x3 - x1;
            } else {
                ex = (f32)x2, ez = (f32)z2;
                tx = z3 - z2, tz = x3 - x2;
            }
        }
        dx = (f32)tx;
        dz = (f32)tz;
        a = (fx - ex) * dx;
        b = (fz - ez) * dz;
        d = a - b;
        if (d == 0.0f)
            continue;
        a = (cx - ex) * dx;
        b2 = (cz - ez) * dz;
        c = a - b2;
        if (d > 0.0f) {
            if (c > 0.0f)
                continue;
        } else {
            if (c < 0.0f)
                continue;
        }
        r = 0;
        break;
    }
    return r;
}

/* whether (x, z) is inside the triangle's bounding box */
REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA5E0(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    s32 lox = x1, hix = x1, loz = z1, hiz = z1, r;

    ENGINE_COST(802AA5E0, 48);
    if (x2 < lox) {
        lox = x2;
    }
    if (x3 < lox) {
        lox = x3;
    }
    if (hix < x2) {
        hix = x2;
    }
    if (hix < x3) {
        hix = x3;
    }
    if (z2 < loz) {
        loz = z2;
    }
    if (z3 < loz) {
        loz = z3;
    }
    if (hiz < z2) {
        hiz = z2;
    }
    if (hiz < z3) {
        hiz = z3;
    }
    /* ($at as the compares in the delay slots leave it) */
    if (x < lox)
        goto out;
    if (hix < x)
        goto out;
    if (z < loz)
        goto out;
    if (hiz < z)
        goto out;
    r = 1;
    goto done;
out:
    r = 0;
done:
    return r;
}

/* D_803ED390[3] (vehicle.h): the angles func_802AA764 rotates by: x, y, z */
extern s32 D_803EBB58[16];      /* its scratch matrix */

void func_802ACC68(s32 x, s32 y, s32 z, s32 *m);
void func_802ACBDC(s32 angle, s32 *m);
void func_802ACB50(s32 angle, s32 *m);
void func_802ACAC4(s32 angle, s32 *m);
void func_802ACA60(s32 x, s32 y, s32 z, s32 *m);
void func_802ACCCC(s32 *b, s32 *a);
void func_802AC8CC(u32 *m);

/* the Mtx at m: scaled, rotated about x, z and y by D_803ED390's angles,
   and moved to (x, y, z) << 5 */
REGS(s4, s5, s6, s7, t8 -> s2)
s32 *func_802AA764(s32 x, s32 y, s32 z, s32 scale, s32 *m) {
    ENGINE_COST(802AA764, 53);
    func_802ACC68(scale, scale, scale, m);
    func_802ACBDC((u16)D_803ED390[0], D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802ACB50((u16)D_803ED390[2], D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802ACAC4((u16)D_803ED390[1], D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802ACA60(x << 11, y << 11, z << 11, D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802AC8CC((u32 *)m);
    return m;
}

/* the same, with the angles as arguments */
void func_802AA6D0(s32 x, s32 y, s32 z, s32 ax, s32 ay, s32 az, s32 scale, s32 *m) {
    ENGINE_COST(802AA6D0, 37);
    D_803ED390[1] = ay;
    D_803ED390[0] = ax;
    D_803ED390[2] = az;
    func_802AA764(x, y, z, scale, m);
}

/* 64 bytes from a + off to b + off */
REGS(t0, t1, t2)
void func_802AA838(u8 *a, u8 *b, s32 off) {
    u32 *s = (u32 *)(a + off), *d = (u32 *)(b + off);
    s32 n;

    ENGINE_COST(802AA838, 64);
    for (n = 8; n != 0; n--) {
        d[0] = s[0];
        d[1] = s[1];
        s += 2, d += 2;
    }
}

/* where the line through two points meets another (the four forms of the
   same solve): f10 and f20 */
REGS(v1, a0, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB1B0(s32 v1, s32 a0, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 a, b;

    ENGINE_COST(802AB1B0, 33);
    b = (f32)(t0 - s7) / (f32)v1;
    a = (f32)a1 * b;
    a = a + (f32)t9;
    a = a - (f32)t1;
    *f20 = b;
    return a / (f32)a0;
}

REGS(v0, a0, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB234(s32 v0, s32 a0, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 a, b;

    ENGINE_COST(802AB234, 33);
    a = (f32)(s7 - t0) / (f32)v0;
    b = (f32)a0 * a;
    b = b + (f32)t1;
    b = b - (f32)t9;
    *f20 = b / (f32)a1;
    return a;
}

REGS(v0, v1, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB2B8(s32 v0, s32 v1, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 a, b;

    ENGINE_COST(802AB2B8, 33);
    b = (f32)(t1 - t9) / (f32)a1;
    a = (f32)v1 * b;
    a = a + (f32)s7;
    a = a - (f32)t0;
    *f20 = b;
    return a / (f32)v0;
}

REGS(v0, v1, a0, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB33C(s32 v0, s32 v1, s32 a0, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 a, b;

    ENGINE_COST(802AB33C, 33);
    a = (f32)(t9 - t1) / (f32)a0;
    b = (f32)v0 * a;
    b = b + (f32)t0;
    b = b - (f32)s7;
    *f20 = b / (f32)v1;
    return a;
}

/* D_803ED3B8: (type, flag) byte pairs in words, to a word of -1 */
extern u8 D_803ED3B8[];

/* the flag of the first record of this type */
s32 func_802AB3C0(s32 type) {
    u8 *p = D_803ED3B8;
    s32 r = 0;

    ENGINE_COST(802AB3C0, 43);
    for (;;) {
        if (*(s32 *)p == -1)
            break;
        if (p[0] != type) {
            p += 4;
            continue;
        }
        if (p[1] != 0) {
            r = 1;
        }
        break;
    }
    return r;
}

/* whether there is a record (a, b) */
REGS(t4, t8 -> a2)
s32 func_802AB41C(s32 a, s32 b) {
    u8 *p = D_803ED3B8;
    s32 r = 0;

    ENGINE_COST(802AB41C, 39);
    for (;;) {
        if (*(s32 *)p == -1)
            break;
        if (p[1] != b) {
            p += 4;
            continue;
        }
        if (p[0] != a) {
            p += 4;
            continue;
        }
        r = 1;
        break;
    }
    return r;
}

REGS(v1 -> fp)
s32 func_802AD7FC(u32 x);

/* the angle at (x2, z2) of the corner... (an arctangent of the ratio of
   the two distances), in 1/8ths of func_802AD7FC's */
REGS(t3, t4, t5, t6, t7, s0 -> s6)
s32 func_802ABB1C(s32 x, s32 z, s32 dx, s32 dz, s32 x2, s32 z2) {
    s32 a, b, ex, ez, r;
    f32 d1, d2;

    ENGINE_COST(802ABB1C, 51);
    a = x - x2;
    b = z - z2;
    ex = x2 + dx;
    ez = z2 + dz;
    d1 = __builtin_sqrtf((f32)((s64)a * a + (s64)b * b));
    a = ex - x;
    b = ez - z;
    d2 = __builtin_sqrtf((f32)((s64)a * a + (s64)b * b));
    d2 = d2 / 2.0f;
    d2 = d2 / d1;
    d2 = d2 * 65536.0f;
    r = engine_cvt_w_s(d2);
    if (r >= 0x10000) {
        r = 0xFFFF;
    }
    r = func_802AD7FC(r);
    return (u32)r >> 3;
}

/* D_803EBC10: 16-byte records, the id in byte 12 */
extern u8 D_803EBC10[];

/* the n-th record from the first with this id */
REGS(v0, v1 -> a0)
void *func_802ABC88(s32 id, s32 n) {
    u8 *p = D_803EBC10;

    ENGINE_COST(802ABC88, 48);
    for (;;) {
        if (p[0xC] == id)
            break;
        p += 0x10;
    }
    if (--n != 0) {
        p += (u32)n * 0x10;
    } else {
    }
    return p;
}

/* the distance between two points, rounded (64 bits) */
REGS(t3, t4, t5, t6, t7, s0 -> s1+f0)
s64 func_802ABCDC(s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2) {
    s32 dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
    s64 r;

    ENGINE_COST(802ABCDC, 30);
    r = engine_cvt_l_d(__builtin_sqrt((f64)((s64)dx * dx + (s64)dy * dy + (s64)dz * dz)));
    return r;
}

/* D_803BDFD8: the level's lights, 0x24-byte records up to D_803BDFD4 */
extern u8 D_803BDFD8[];
extern u8 *PTR32 D_803BDFD4;
extern u8 D_80364A6E[];

/* the light on the vehicle of this type at (x, y, z): the first light in
   range that lists the type, faded with the distance, or the level's
   ambient D_80364A6E; into its Vehicle's unk60 */
REGS(a3, t3, t4, t5)
void func_802ABD54(s32 type, s32 x, s32 y, s32 z) {
    u8 *p = D_803BDFD8, *end = D_803BDFD4, *t;
    s64 d;
    s32 r, n, amb, q;
    Vehicle *v;

    ENGINE_COST(802ABD54, 53);
    for (;; p += 0x24) {
        if (p == end) {
            r = D_80364A6E[0];
            break;
        }
        d = func_802ABCDC(x, y, z, ((s32 *)p)[0], ((s32 *)p)[1], ((s32 *)p)[2]);
        r = ((s32 *)p)[3];
        if ((s64)r < d)
            goto next;
        if (p[0x12] == 0)
            goto next;
        n = p[0x13];
        t = p + 0x15;
        for (;;) {
            if (n == 0)
                goto next;
            if (type == *t)
                break;
            t++, n--;
        }
        n = p[0x14];
        amb = D_80364A6E[0];
        if (n == 0) {
            if (p[0x10] == 1) {
                r = 0xFF;
            } else {
                q = (u32)((0xFF - amb) * (u32)d) / (u32)r;
                r = 0xFF - amb - q + amb;
            }
        } else {
            if (p[0x10] == 1) {
                r = n;
            } else {
                q = (u32)((amb - n) * (u32)d) / (u32)r;
                r = n + q;
            }
        }
        break;
    next:
        ;
    }
    for (v = D_80364460;; v++) {
        if (v->type == type)
            break;
    }
    v->unk60 = r;
}

extern s32 D_803A740C;          /* a frame count */
extern s32 D_802E8BDC;          /* the level */
extern s32 D_80358068;
extern s32 D_80305C58[];        /* (level, value, frames) triples, to a value of 0 */

/* the level's value for this many frames past D_803A740C (unchanged until
   ten frames have gone) */
REGS(t5 -> t5)
s32 func_802A8314(s32 v) {
    s32 base = D_803A740C, now = D_80358068, *p;

    ENGINE_COST(802A8314, 25);
    if (base + 10 < now)
        goto done;
    p = D_80305C58;
    for (;;) {
        if (p[1] == 0)
            break;
        if (p[0] != D_802E8BDC) {
            p += 3;
            continue;
        }
        p += 3;
        if (p[-1] + base < now)
            continue;
        v = p[-2];
        break;
    }
done:
    return v;
}

/* the speed (*speed) over the distance from s7[1] to s7[0], stored at
   *ratio; the old ratio when a flag of three is set or the speed is 0 */
REGS(t3, t6, s0, s7, t8 -> f12, t3)
f32 func_802A83B8(s32 t3, s16 *speed, u8 *flags, s32 *pos, f32 *ratio, s32 *t3_out) {
    f32 r;

    ENGINE_COST(802A83B8, 30);
    /* ($t1: the flag it looked at last, or s7[1]) */
    if (flags[1] == 1)
        goto old;
    if (flags[2] == 1)
        goto old;
    if (flags[0] == 1)
        goto old;
    t3 = *speed;
    if (t3 == 0)
        goto old;
    r = (f32)(pos[0] - pos[1]) / (f32)t3;
    *ratio = r;
    goto done;
old:
    r = *ratio;
done:
    *t3_out = t3;
    return r;
}

/* of s7[0] - s7[6] and s7[3] - s7[6], the one nearer 0 */
REGS(s7 -> t1)
s32 func_802A8590(s32 *s7) {
    s32 a, b, aa, bb, r;

    ENGINE_COST(802A8590, 27);
    a = s7[0] - s7[6];
    aa = a;
    if (a < 0) {
        aa = -a;
    }
    b = s7[3] - s7[6];
    bb = b;
    if (b < 0) {
        bb = -b;
    }
    if (bb < aa) {
        r = b;
    } else {
        r = a;
    }
    return r;
}

/* (x, z) moved by the speed (scaled by the turn rate f12) in the direction
   `angle`: *x + ..., *z + ... */
REGS(t4, t6, t7, s1, f12 -> t0, t1)
s32 func_802A860C(s32 angle, s16 *speed, s32 *x, s32 *z, f32 rate, s32 *z_out) {
    s32 v = *speed, rem, s, c, dx, dz, at, rx, rz, px, pz;
    f32 a = __builtin_fabsf(rate), q;

    ENGINE_COST(802A860C, 63);
    if (v != 0) {
        if (!(a >= 1.0f)) {
            q = a / 2.0f;
            q = 1.0f - q;
            q = q * (f32)v;
            v = engine_cvt_w_s(q);
        } else {
            q = a * 2.0f;
            q = (f32)v / q;
            v = engine_cvt_w_s(q);
        }
    }
    rem = angle % 0x400;
    s = func_802AE160(rem);
    dx = (s32)(v * s) >> 16;
    c = func_802AE104(rem);
    dz = (s32)(v * c) >> 16;
    px = *x;
    pz = *z;
    at = angle < 0x400;
    if (at) {
        rx = px + dx, rz = pz + dz;
    } else {
        at = angle < 0x800;
        if (at) {
            rx = px + dz, rz = pz - dx;
        } else {
            at = angle < 0xC00;
            if (at) {
                rx = px - dx, rz = pz - dz;
            } else {
                rx = px - dz, rz = pz + dx;
            }
        }
    }
    /* ($v0 and $fp: 5CB60.c's collision triangles and 69BB0.c's driver read
       them from the context) */
    ENGINE_LEAVE(2, 0x400);
    ENGINE_LEAVE(30, c);
    *z_out = rz;
    return rx;
}

extern u8 D_80370C22, D_80370C1A, D_80370C1B;
extern s16 D_803ED400;

REGS(t2, s1, gp -> t4, s1)
s32 func_802A7C28(s32 x, s16 *rows, VS *vs, u32 *rows_out);
REGS(t2, s1 -> t4)
s32 func_802A7A1C(s32 x, s16 *rows);
REGS(t2, s1 -> t4)
s32 func_802A7AAC(s32 x, s16 *rows);
REGS(t7, s0 -> t3)
s32 func_802A7D68(s32 mode, u8 *flags);

/* The speed (*speed, the state's unk76): with D_80370C22 set, toward 0 by
   `brake` while D_803ED400 holds; otherwise capped at half the first or
   last gear's, then accelerated through the gears by the stick
   (D_80370C2D).  D_803ED400 gets the result.  Returns $t2 as it leaves it,
   and $t3: the gear rows' step (func_802A7D68's, for the mode). */
REGS(t3, t6, t7, s0, s1, s2, gp -> t2, t3)
s32 func_802A785C(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, VS *vs, s32 *step_out) {
    s32 t2, t4, g;
    u32 rows2;

    ENGINE_COST(802A785C, 32);
    if (D_80370C22 != 0) {
        t2 = D_803ED400;
        t4 = *speed;
        if (t2 == 0)
            goto done;
        if (t4 > 0) {
            t4 -= brake;
            if (!(t4 > 0)) {
                t4 = 0;
            }
        } else {
            t4 += brake;
            if (!(t4 < 0)) {
                t4 = 0;
            }
        }
        *speed = t4;
        goto done;
    }
    if ((s8)D_803ED40C == 0)
        goto gears;
    if (D_802E8BDC == 0x22)
        goto gears;
    if (D_80364456 == 0xB)
        goto gears;
    if (D_80364456 == 0x11)
        goto gears;
    if (D_80364456 == 0x12)
        goto gears;
    t4 = *speed;
    if (t4 >= 0) {
        g = rows[13] >> 1;
        if (!(g < t4))
            goto gears;
        *speed = g;
    } else {
        g = rows[0] >> 1;
        if (!(t4 < g))
            goto gears;
        *speed = g;
    }
gears:
    step = func_802A7D68(mode, flags);
    t2 = D_80370C2D;
    if (t2 == 0)
        goto done;
    if (t2 > 0) {
        t2 = *speed;
        if (t2 < 0) {
            t2 += brake;
            *speed = t2;
            goto done;
        }
        t4 = func_802A7AAC(t2, rows);
        if (t4 != 0)
            goto done;
        t4 = func_802A7C28(t2, rows, vs, &rows2);
        if (t4 != 0) {
            t2 += t4 * step;
        } else {
            t2 -= 6;
        }
        *speed = t2;
        goto done;
    }
    t2 = *speed;
    if (t2 > 0) {
        t2 -= brake;
        *speed = t2;
        goto done;
    }
    t4 = func_802A7A1C(t2, rows);
    if (t4 != 0)
        goto done;
    t4 = func_802A7C28(t2, rows, vs, &rows2);
    t2 -= t4 * step;
    *speed = t2;
done:
    D_803ED400 = *speed;
    *step_out = step;
    return t2;
}

REGS(s3, s4 -> s3, t0, t2)
s32 func_802A7E70(s32 rate, u16 *h, u32 *stick_addr, s32 *stick);

/* the steering, then the speed: a turn and the gears in one */
REGS(t3, t6, t7, s0, s1, s2, s3, s4, gp -> t2, t3, s3)
s32 func_802A7834(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, s32 rate, u16 *h, VS *vs,
                  s32 *step_out, s32 *turn_out) {
    u32 sa;
    s32 st, r;

    ENGINE_COST(802A7834, 10);
    *turn_out = func_802A7E70(rate, h, &sa, &st);
    r = func_802A785C(step, speed, mode, flags, rows, brake, vs, step_out);
    return r;
}

/* the speed for a vehicle without gears: toward 0 by `brake` with
   D_80370C22 and D_803ED400 set, otherwise accelerated through the gear
   rows on D_80370C1A/D_80370C1B (the buttons) */
REGS(t6, s1, s2, gp)
void func_802A7B3C(s16 *speed, s16 *rows, s32 brake, VS *vs) {
    s32 t2, t4;
    u32 rows2;

    ENGINE_COST(802A7B3C, 30);
    if (D_80370C22 != 0) {
        t2 = D_803ED400;
        t4 = *speed;
        if (t2 == 0)
            goto done;
        if (t2 > 0) {
            t4 -= brake;
            if (!(t4 > 0)) {
                t4 = 0;
            }
        } else {
            t4 += brake;
            if (!(t4 < 0)) {
                t4 = 0;
            }
        }
        *speed = t4;
        goto done;
    }
    if (D_80370C1A == 0) {
        if (D_80370C1B == 0)
            goto done;
    }
    t2 = *speed;
    if (t2 < 0) {
        *speed = t2 + brake;
        goto done;
    }
    t4 = func_802A7C28(t2, rows, vs, &rows2);
    if (t4 != 0) {
        *speed = t2 + t4 * 4;
    } else {
        *speed = t2 - 6;
    }
done:
    D_803ED400 = *speed;
}

extern f32 D_803EBBF4;
extern u8 D_803BE738;
extern u8 D_803ED3F7, D_803ED3EE, D_803ED3EF, D_803ED3F2, D_803ED3F3, D_803ED3F4;
extern u8 D_803ED3EA, D_803ED3EB, D_803ED3EC, D_803ED410;
extern s32 D_803ED398, D_803ED39C, D_803ED3A0, D_803ED3AC, D_803ED3B0;
extern s32 D_803ED3A8[];

/* D_803EBBF4 tripled when D_803ED3F5 */
void func_802A8FB4(void) {
    ENGINE_COST(802A8FB4, 10);
    if (D_803ED3F5 != 0) {
        D_803EBBF4 = D_803EBBF4 * 3.0f;
    }
}

/* D_803ED3F7 = 8 when the state's unk9F is 0x67 on level 12 */
REGS(gp)
void func_802A8FF4(VS *vs) {
    ENGINE_COST(802A8FF4, 9);
    if (vs->unk9F == 0x67) {
        if (D_802E8BDC == 0xC) {
            D_803ED3F7 = 8;
        }
    }
}

/* the state's previous values set from its current ones */
REGS(gp)
void func_802A9038(VS *vs) {
    u8 b;
    s32 w;

    ENGINE_COST(802A9038, 43);
    b = D_803ED3EE;
    D_803ED3EF = b;
    *(u8 *)((u32)&D_803ED3EF + 1) = b;   /* (the byte after it, which has no name) */
    w = D_803ED398;
    D_803ED39C = w;
    D_803ED3A0 = w;
    w = D_803ED3A8[0];
    D_803ED3AC = w;
    D_803ED3B0 = w;
    w = vs->unk28[3];
    vs->unk28[4] = w;
    vs->unk28[5] = w;
    w = vs->unk28[0];
    vs->unk28[1] = w;
    vs->unk28[2] = w;
    w = vs->unk28[6];
    vs->unk28[7] = w;
    vs->unk28[8] = w;
    b = D_803ED3F2;
    D_803ED3F3 = b;
    D_803ED3F4 = b;
    b = D_803ED3EA;
    D_803ED3EB = b;
    D_803ED3EC = b;
}

/* D_803ED410: whether all six of a group are 0 */
REGS(v1)
void func_802A90E4(u16 *g) {
    ENGINE_COST(802A90E4, 15);
    if (g[0] != 0)
        goto no;
    if (g[1] != 0)
        goto no;
    if (g[2] != 0)
        goto no;
    if (g[3] != 0)
        goto no;
    if (g[4] != 0)
        goto no;
    if (g[5] != 0)
        goto no;
    D_803ED410 = 1;
    goto done;
no:
    D_803ED410 = 0;
done:
    ;
}

/* the gears (unk50): the average of the three gearboxes' (D_803ED3F2[3],
   100 and up counting as 2); unk9F and D_803ED40D: the highest; and
   D_803BE738 set on 0x66, or 0x64 with D_80358064, unless the first flag */
REGS(s0, t8, gp)
void func_802A9164(u8 *flags, s32 type, VS *vs) {

    s32 a, b, m, t2;

    ENGINE_COST(802A9164, 68);
    a = D_803ED3F2;
    if (!(a < 100)) {
        a = 2;
    }
    b = D_803ED3F3;
    if (!(b < 100)) {
        b = 2;
    }
    a += b;
    b = D_803ED3F4;
    if (!(b < 100)) {
        b = 2;
    }
    a += b;
    vs->unk50 = (u32)a / 3;
    m = 0;
    if (m < D_803ED3F2) {
        m = D_803ED3F2;
    }
    if (m < D_803ED3F3) {
        m = D_803ED3F3;
    }
    if (m < D_803ED3F4) {
        m = D_803ED3F4;
    }
    vs->unk9F = m;
    D_803ED40D = m;
    t2 = D_803ED40D;
    if (D_803ED40D == 0x66) {
        t2 = flags[0];
        if (flags[0] != 1) {
            D_803BE738 = t2 = 1;
        }
    }
    if (type == 9)
        goto done;
    if (type == 0xB)
        goto done;
    if (type == 0x11)
        goto done;
    if (type == 0x12)
        goto done;
    t2 = D_80358064;
    if (D_80358064 == 0)
        goto done;
    t2 = D_803ED40D;
    if (D_803ED40D != 0x64)
        goto done;
    t2 = flags[0];
    if (flags[0] == 1)
        goto done;
    D_803BE738 = t2 = 1;
done:
    ;
}

REGS(a0, a2, a3 -> a1, a3, t1)
s64 func_802ACE38(s64 x, s64 z, s32 angle, s32 *xr, s32 *zr);

/* point `i` of a table of (x, z) s16 pairs rotated by the angle *a */
REGS(v0, v1, s4 -> t5, t6)
s32 func_802A94A4(s32 i, s16 *pts, s16 *a, s32 *z_out) {
    s32 xr, zr;

    ENGINE_COST(802A94A4, 28);
    func_802ACE38(pts[2 * i], pts[2 * i + 1], *a, &xr, &zr);
    *z_out = zr;
    return xr;
}

/* clamped to 0x240 when positive */
REGS(s3 -> s3)
s32 func_802A9514(s32 v) {
    ENGINE_COST(802A9514, 10);
    if (v >= 0) {
        if (!(v < 0x241)) {
            v = 0x240;
        }
    }
    return v;
}

/* wheel i on the ground: its height (clamped), 2 and t2 in the three
   arrays, and its flag in D_803ED3EE[] */
REGS(v0, a1, a2, a3, t2, s3)
void func_802A9540(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 v) {
    ENGINE_COST(802A9540, 25);
    v = func_802A9514(v);
    h[i] = v;
    state[i] = 2;
    ground[i] = g;
    *(u8 *)((u32)&D_803ED3EE + i) = 1;
}

/* ---- the ground under a point: triangles, nearest in height ------------- */

extern f64 D_80305C50;

static u64 double_bits(f64 v) {
    union { f64 d; u64 u; } l;

    l.d = v;
    return l.u;
}

/* the height at (x, z) of the plane through three points (x1, y1, z1)...,
   in the world's units; -9999999 for a vertical one */
REGS(t0, t1, s1, s2, s3, s4, s5, s6, s7, t8, t9 -> v0)
s32 func_802AA2E4(s32 x, s32 z, s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2, s32 x3, s32 y3, s32 z3) {
    s32 a = y1 - y2, b = z1 - z3, c = z1 - z2, d = y1 - y3, e = x1 - x3, g = x1 - x2;
    s64 nx, ny, nz, w, t, l;
    f64 f0, f2, f4;
    s32 r = -9999999;

    ENGINE_COST(802AA2E4, 95);
    nx = (s64)((u64)((s64)a * b) - (u64)((s64)c * d));
    ny = (s64)((u64)((s64)c * e) - (u64)((s64)g * b));
    nz = (s64)((u64)((s64)g * d) - (u64)((s64)a * e));
    w = -(s64)((u64)nx * (u64)(s64)x2 + (u64)ny * (u64)(s64)y2 + (u64)nz * (u64)(s64)z2);
    t = (s64)((u64)nx * (u64)(s64)x + (u64)ny * (u64)(s64)-1000 + (u64)nz * (u64)(s64)z + (u64)w);
    f0 = (f64)t;
    /* ($t6, which 8A080.c's func_802CE9C8 passes on from the context) */
    ENGINE_LEAVE(14, d);
    if (ny != 0) {
        f2 = (f64)(s64)((u64)ny * 2000);
        f0 = f0 / f2;
        f2 = D_80305C50;
        f0 = __builtin_fabs(f0);
        f4 = f2 * f0;
        l = engine_cvt_l_d(f4);
        r = (s32)l - 1000;
    }
    return r;
}

extern u8 *PTR32 D_803F7828, *PTR32 D_803F782C;   /* the static triangles, 0x28 bytes each */
extern u8 D_803EBDB0[];                         /* the moving ones, 0x38 bytes each ... */
extern u8 *PTR32 D_803EBBEC;                     /* ... to here */
extern s32 D_803BE718, D_803BE71C;              /* the triangle grid's cell sizes */
extern u16 D_803BE720;                          /* its width in cells */
extern u32 D_803BDB10[];                        /* each cell's first triangle (0x14 bytes each) */
extern u8 *PTR32 D_803EBC00, *PTR32 D_803EBC04;   /* the cell looked in last */
extern u8 *PTR32 D_803EBC08;                     /* the triangle found there */
extern u8 D_803EBBD8[];

/* Of the static triangles (D_803F7828) under (x, z), the height nearest
   y (99999999 if none) and its material (*mat, unchanged if none). */
REGS(t0, t1, t2, fp -> t3, fp)
s32 func_802A9DC0(s32 x, s32 z, s32 y, s32 mat, s32 *mat_out) {
    u8 *p = D_803F7828, *end = D_803F782C;
    s32 best = 99999999, h;
    u32 dist = 99999999, a;
    s32 *w;

    ENGINE_COST(802A9DC0, 284);
    for (;;) {
        if (p == end)
            break;
        w = (s32 *)p;
        p += 0x28;
        if (!func_802AA5E0(x, z, w[0], w[2], w[3], w[5], w[6], w[8])) {
            continue;
        }
        if (!func_802AA460(x, z, w[0], w[2], w[3], w[5], w[6], w[8])) {
            continue;
        }
        h = func_802AA2E4(x, z, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8]);
        a = y - h;
        if ((s32)a < 0) {
            a = -(s32)a;
        }
        if (dist < a)
            continue;
        best = h;
        dist = a;
        mat = p[-4];
    }
    ENGINE_LEAVE(14, 0);                /* ($t6, which 60F60.c reads from the context) */
    *mat_out = mat;
    return best;
}

/* the same over the moving objects' triangles (D_803EBDB0), but those of
   object `self`: the height, and the object's id (0 if none) */
REGS(t0, t1, t2, t8 -> t3, t6)
s32 func_802A9F24(s32 x, s32 z, s32 y, s32 self, s32 *id_out) {
    u8 *p = D_803EBDB0, *end = D_803EBBEC;
    s32 best = 99999999, h, id = 0, k;
    u32 dist = 99999999, a;
    s32 *w;

    ENGINE_COST(802A9F24, 170);
    for (;;) {
        if (p == end)
            break;
        k = *(u16 *)(p + 0x36);
        if (k == self) {
            p += 0x38;
            continue;
        }
        w = (s32 *)p;
        p += 0x38;
        if (!func_802AA5E0(x, z, w[0], w[2], w[3], w[5], w[6], w[8])) {
            continue;
        }
        if (!func_802AA460(x, z, w[0], w[2], w[3], w[5], w[6], w[8])) {
            continue;
        }
        h = func_802AA2E4(x, z, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8]);
        a = y - h;
        if ((s32)a < 0) {
            a = -(s32)a;
        }
        if (dist < a)
            continue;
        best = h;
        dist = a;
        id = k;
    }
    *id_out = id;
    return best;
}

/* the same over the level's triangles in the grid cell of (x, z), which it
   remembers (D_803EBC00..08); stops at the first good one that is solid
   (byte 0x13).  Returns whether it found one; the height (*h, unchanged if
   none) and its material (*mat). */
REGS(t0, t1, t2, t3, fp -> a1, t3, fp)
s32 func_802AA094(s32 x, s32 z, s32 y, s32 h, s32 mat, s32 *h_out, s32 *mat_out) {
    s32 cx, cz, found = 0, hh;
    u32 dist = 99999999, a;
    u8 *p, *end;
    s16 *w;
    u32 *cell;

    ENGINE_COST(802AA094, 5132);
    cx = x / D_803BE718;
    cz = z / D_803BE71C;
    cell = &D_803BDB10[D_803BE720 * cz + cx];
    p = (u8 *)cell[0];
    end = (u8 *)cell[1] - 4;
    D_803EBC00 = p;
    D_803EBC04 = end;
    for (;;) {
        if (p == end)
            break;
        w = (s16 *)p;
        p += 0x14;
        if (!func_802AA5E0(x, z, w[0] << 5, w[2] << 5, w[3] << 5, w[5] << 5, w[6] << 5, w[8] << 5)) {
            continue;
        }
        if (!func_802AA460(x, z, w[0] << 5, w[2] << 5, w[3] << 5, w[5] << 5, w[6] << 5, w[8] << 5)) {
            continue;
        }
        hh = func_802AA2E4(x, z, w[0] << 5, w[1] << 5, w[2] << 5, w[3] << 5, w[4] << 5, w[5] << 5,
                           w[6] << 5, w[7] << 5, w[8] << 5);
        a = y - hh;
        if ((s32)a < 0) {
            a = -(s32)a;
        }
        if (dist < a)
            continue;
        D_803EBC08 = p - 0x14;
        found = 1;
        h = hh;
        dist = a;
        mat = p[-2];
        if (p[-1] != 0)
            break;
    }
    *h_out = h;
    *mat_out = mat;
    return found;
}

/* the triangle found in the cell (D_803EBC08) moved to the cell's first
   places (index i, or i + 3 for the player's vehicle), swapping with the
   one there, so that it's found first next time */
REGS(v0, t8)
void func_802A9CAC(s32 i, s32 who) {
    u8 *first = D_803EBC00, *q;
    u32 *s, *d;
    s32 n, k;

    ENGINE_COST(802A9CAC, 127);
    if (D_803EBC04 - first < 0x8C)
        goto done;
    k = 0;
    if (who == 0xFF) {
        k = 3;
    }
    q = first + (u32)(k + i) * 0x14;
    if (D_803EBC08 == q)
        goto done;
    s = (u32 *)D_803EBC08, d = (u32 *)D_803EBBD8;
    for (n = 0x14;; n -= 4) {
        if (n == 0)
            break;
        *d++ = *s++;
    }
    s = (u32 *)q, d = (u32 *)D_803EBC08;
    for (n = 0x14;; n -= 4) {
        if (n == 0)
            break;
        *d++ = *s++;
    }
    s = (u32 *)D_803EBBD8, d = (u32 *)q;
    for (n = 0x14;; n -= 4) {
        if (n == 0)
            break;
        *d++ = *s++;
    }
done:
    ;
}

/* Wheel i's ground height at (x, y, z), y raised by 0x78: the nearest of
   the static triangles, the moving objects' (not the vehicle's own, self)
   and the level grid's; with none, y.  Its material and object in
   D_803ED3F2[i] and D_803ED3EA[i], the height (0 or more) in
   D_803ED3A8[i]; unk9B set when the static one won. */
REGS(v0, t0, t1, t2, t8, gp, fp -> t3)
s32 func_802A9B1C(s32 i, s32 x, s32 z, s32 y, s32 self, VS *vs, s32 mat) {
    s32 s0, s1, t3, t4, t5, t6, t7, s2, fp, a1;

    ENGINE_COST(802A9B1C, 67);
    y += 0x78;
    s0 = func_802A9DC0(x, z, y, mat, &s1);
    t4 = func_802A9F24(x, z, y, self, &t6);
    a1 = func_802AA094(x, z, y, t4, s1, &t3, &fp);
    if (a1 == 0) {
        if (s0 == 99999999) {
            if (t4 == 99999999) {
                t3 = y - 0x78;
                fp = 1;
                t6 = 0;
                goto store;
            }
        }
        t7 = t4 - y;
        s2 = s0 - y;
        if (t7 < 0) {
            t7 = -t7;
        }
        if (s2 < 0) {
            s2 = -s2;
        }
    } else {
        t7 = t4 - y;
        t5 = t3 - y;
        if (t7 < 0) {
            t7 = -t7;
        }
        if (t5 < 0) {
            t5 = -t5;
        }
        s2 = s0 - y;
        if (s2 < 0) {
            s2 = -s2;
        }
        if (!(t7 < t5)) {
            if (!(s2 < t5)) {
                func_802A9CAC(i, self);
                t6 = 0;
                goto store;
            }
        }
    }
    if (t7 < s2) {
        t3 = t4;
        fp = 1;
    } else {
        vs->unk9B = 1;
        t3 = s0;
        fp = s1;
        t6 = 0;
    }
store:
    *(u8 *)((u32)&D_803ED3EA + i) = t6;
    *(u8 *)((u32)&D_803ED3F2 + i) = fp;
    if (t3 < 0) {
        t3 = 0;
    }
    D_803ED3A8[i] = t3;
    return t3;
}

/* ---- the three wheels on the ground -------------------------------------- */

#define WHEEL_BYTE(base, i) (*(u8 *)((u32)&(base) + (i)))

REGS(v0, v1, s4 -> t5, t6)
s32 func_802A94A4(s32 i, s16 *pts, s16 *a, s32 *z_out);
REGS(v0, t0, t1, t2, t8, gp, fp -> t3)
s32 func_802A9B1C(s32 i, s32 x, s32 z, s32 y, s32 self, VS *vs, s32 mat);
REGS(v0, a1, a2, a3, t2, s3)
void func_802A9540(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 v);
REGS(s3 -> s3)
s32 func_802A9514(s32 v);

/* The three wheels (pts, rotated by *a, from (x, z)) on the ground: the
   moving objects' triangles first (not the vehicle's own), then the
   level's.  Each height three times at out[3 * i], the average of the
   last two in *avg; the gears (unk50) from the materials; the vehicle's
   record in D_803ED3B8 gets the wheels' objects.  Returns the out table
   ($s3) and the average ($s5). */
REGS(v1, t2, t7, s0, s1, s2, s4, t8, gp, fp -> s3, s5)
s32 *func_802A992C(s16 *pts, s32 y, s32 x, s32 z, s32 *out, s32 *avg, s16 *a, s32 self, VS *vs, s32 mat,
                   s32 *avg_out) {
    s32 i, dx, dz, h, id, s5, s6;
    s32 *o = out;
    u8 *p;

    ENGINE_COST(802A992C, 134);
    for (i = 0; i != 3;) {
        dx = func_802A94A4(i, pts, a, &dz);
        h = func_802A9F24(x + dx, z + dz, y, self, &id);
        if (id == 0) {
            func_802AA094(x + dx, z + dz, y, h, mat, &h, &mat);
            id = 0;
        }
        WHEEL_BYTE(D_803ED3EA, i) = id;
        o[0] = h, o[1] = h, o[2] = h;
        WHEEL_BYTE(D_803ED3F2, i) = mat;
        i++;
        o += 3;
    }
    s5 = out[3];
    s6 = out[6];
    s5 = (u32)(s5 + s6) >> 1;
    *avg = s5;
    vs->unk50 = (u32)(D_803ED3F2 + D_803ED3F3 + D_803ED3F4) / 3;
    for (p = D_803ED3B8;;) {
        if (p[0] == self)
            break;
        if (p[0] != 0xFF) {
            p += 4;
            continue;
        }
        if (*(s32 *)p != -1) {
            p += 4;
            continue;
        }
        p[0] = self;
        break;
    }
    p[1] = D_803ED3EA;
    p[2] = D_803ED3EB;
    p[3] = D_803ED3EC;
    *avg_out = s5;
    return out;
}

/* the same, each wheel through func_802A9B1C; returns the out table past
   its end ($s1) */
REGS(v1, t2, t7, s0, s1, s2, s4, t8, gp, fp -> s1)
s32 *func_802A9A60(s16 *pts, s32 y, s32 x, s32 z, s32 *out, s32 *avg, s16 *a, s32 self, VS *vs, s32 mat) {
    s32 i, dx, dz, h, s5, s6;
    s32 *o = out;

    ENGINE_COST(802A9A60, 80);
    for (i = 0; i != 3;) {
        dx = func_802A94A4(i, pts, a, &dz);
        vs->unk9B = 0;
        h = func_802A9B1C(i, x + dx, z + dz, y, self, vs, mat);
        o[0] = h, o[1] = h, o[2] = h;
        WHEEL_BYTE(D_803ED3F2, i) = mat;
        i++;
        o += 3;
    }
    s5 = out[3];
    s6 = out[6];
    s5 = (u32)(s5 + s6) >> 1;
    *avg = s5;
    vs->unk50 = (u32)(D_803ED3F2 + D_803ED3F3 + D_803ED3F4) / 3;
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    return o;
}

/* the three wheels (pts at *a's angle around (x, z)) on the ground through
   func_802A9B1C, each at its own height (heights[3 * i]); then the
   vehicle's record in D_803ED3B8 */
REGS(t0, t1, t3, s4, s7, t8, gp, fp)
void func_802A92C8(s32 x, s32 z, s16 *pts, s16 *a, s32 *heights, s32 self, VS *vs, s32 mat) {
    s32 i, dx, dz;
    u8 *p;

    ENGINE_COST(802A92C8, 91);
    for (i = 0;;) {
        dx = func_802A94A4(i, pts, a, &dz);
        func_802A9B1C(i, x + dx, z + dz, heights[3 * i], self, vs, mat);
        if (++i == 3)
            break;
    }
    for (p = D_803ED3B8;;) {
        if (p[0] == self)
            break;
        if (p[0] != 0xFF) {
            p += 4;
            continue;
        }
        if (*(s32 *)p != -1) {
            p += 4;
            continue;
        }
        p[0] = self;
        break;
    }
    p[1] = D_803ED3EA;
    p[2] = D_803ED3EB;
    p[3] = D_803ED3EC;
}

/* One wheel (i) on the ground: its height above its last
   (heights[3 * i] - heights[3 * i + 1], clamped) plus D_803EBBF4 gives
   where it would be; when that is at or below the ground, it lands there
   (func_802A9540), otherwise it is in the air (D_803ED3EE[i] = 0).  The
   result in D_803ED398[i]; returns the clamped height ($s3). */
REGS(v0, v1, a1, a2, a3, t0, t1, s4, s7, t8, gp, fp -> s3)
s32 func_802A93B0(s32 i, s16 *pts, s32 *h, s32 *state, s32 *ground, s32 x, s32 z, s16 *a, s32 *heights,
                  s32 self, VS *vs, s32 mat) {
    s32 dx, dz, t2, s1, s3, s4, t3;

    ENGINE_COST(802A93B0, 57);
    dx = func_802A94A4(i, pts, a, &dz);
    t2 = heights[3 * i];
    s1 = heights[3 * i + 1];
    s3 = func_802A9514(t2 - s1);
    s4 = engine_cvt_w_s(D_803EBBF4);
    s4 = s3 + s4 + t2;
    t3 = func_802A9B1C(i, x + dx, z + dz, t2, self, vs, mat);
    if (!(s4 - 0x1E < t3)) {
        func_802A9540(i, h, state, ground, t2, s3);
        t3 = s4;
    } else {
        WHEEL_BYTE(D_803ED3EE, i) = 0;
    }
    *(s32 *)((u32)&D_803ED398 + 4 * i) = t3;
    return s3;
}

/* ---- the ground of the level's objects and of the water ---------------- */

extern s32 D_803EBBF8, D_803EBBFC;
extern u8 *PTR32 D_803BDAF4, *PTR32 D_803BDAF8;

/* Of an object model's solid triangles (byte 0x13 clear; the model's list
   at its offsets 0x24 to 0x28) under (x, z), the nearest at or below y: its
   height in D_803EBBF8.  Returns whether one was found (`found` if not). */
REGS(a1, t0, t1, t2, s0 -> a1)
s32 func_802ABFC8(s32 found, s32 x, s32 z, s32 y, u8 *model) {
    u8 *p = model + *(s32 *)(model + 0x24), *end = model + *(s32 *)(model + 0x28);
    u32 dist = 99999999;
    s32 h, d;
    s16 *w;

    ENGINE_COST(802ABFC8, 825);
    for (;; p += 0x14) {
        if (p == end)
            break;
        if (p[0x13] != 0)
            goto next;
        w = (s16 *)p;
        if (!func_802AA5E0(x, z, w[0] << 5, w[2] << 5, w[3] << 5, w[5] << 5, w[6] << 5, w[8] << 5)) {
            goto next;
        }
        if (!func_802AA460(x, z, w[0] << 5, w[2] << 5, w[3] << 5, w[5] << 5, w[6] << 5, w[8] << 5)) {
            goto next;
        }
        h = func_802AA2E4(x, z, w[0] << 5, w[1] << 5, w[2] << 5, w[3] << 5, w[4] << 5, w[5] << 5, w[6] << 5,
                          w[7] << 5, w[8] << 5);
        d = y - h;
        if (d < 0)
            goto next;
        if (dist < (u32)d)
            goto next;
        found = 1;
        D_803EBBF8 = h;
        dist = d;
    next:
        ;
    }
    return found;
}

extern u8 D_803F4030[];
extern u8 *PTR32 D_803F7654;

/* whether (x, z) is in one of the level's objects (D_803F4030's records:
   their model's rectangle, as two triangles) with ground under it at or
   below y (func_802ABFC8) */
s32 func_802ABEDC(s32 x, s32 y, s32 z) {
    u8 *p = D_803F4030, *end = D_803F7654, *model;
    s16 *r;
    s32 found = 0, x1, z1, x2, z2;

    ENGINE_COST(802ABEDC, 576);
    for (;; p += 0xFC) {
        if (p == end)
            break;
        model = *(u8 *PTR32 *)p;
        r = (s16 *)(model + *(s32 *)(model + 0x20));
        z1 = r[1] << 5;
        x2 = r[2] << 5;
        x1 = r[0] << 5;
        z2 = r[3] << 5;
        if (func_802AA5E0(x, z, x1, z1, x2, z2, x2, z1)) {
        } else {
            if (!func_802AA5E0(x, z, x1, z1, x2, z2, x1, z2)) {
                goto next;
            }
        }
        found = func_802ABFC8(found, x, z, y, model);
    next:
        ;
    }
    return found;
}

/* the same as func_802ABFC8 over the water's triangles (D_803BDAF4 to
   D_803BDAF8), at any height: the nearest's in D_803EBBFC */
REGS(t0, t1, t2 -> a1)
s32 func_802AC0BC(s32 x, s32 z, s32 y) {
    u8 *p = D_803BDAF4, *end = D_803BDAF8;
    u32 dist = 99999999, d;
    s32 h, found = 0;
    s16 *w = NULL;

    ENGINE_COST(802AC0BC, 264);
    for (;; p += 0x14) {
        if (p == end)
            break;
        w = (s16 *)p;
        if (!func_802AA5E0(x, z, w[0] << 5, w[2] << 5, w[3] << 5, w[5] << 5, w[6] << 5, w[8] << 5)) {
            goto next;
        }
        if (!func_802AA460(x, z, w[0] << 5, w[2] << 5, w[3] << 5, w[5] << 5, w[6] << 5, w[8] << 5)) {
            goto next;
        }
        h = func_802AA2E4(x, z, w[0] << 5, w[1] << 5, w[2] << 5, w[3] << 5, w[4] << 5, w[5] << 5, w[6] << 5,
                          w[7] << 5, w[8] << 5);
        d = y - h;
        if ((s32)d < 0) {
            d = -(s32)d;
        }
        /* (the distance stays in $a0, and the delay slot's lui in $at) */
        if (dist < d)
            goto next;
        found = 1;
        D_803EBBFC = h;
        dist = d;
    next:
        ;
    }
    /* (it saves nothing: the vehicle modules read on what the last
       triangle's loads and the loop left) */
    if (w != NULL) {
    }
    return found;
}

/* ---- a point's place in another triangle (the moving objects' two
   shapes: their world triangle and their own) ------------------------------ */

/* The point (x, z) of triangle A ((x1, z1), (x2, z2), (x3, z3)) at the
   same place in triangle B ((u1, v1), ...): by the corners exactly, else
   by solving for its two coordinates along A's edges (f10, f20).
   Returns u ($t3), and v ($t4). */
REGS(t0, t1, s1, s3, s4, s6, s7, t9, t3, t4, t5, t6, t7, s0 -> t3, t4)
s32 func_802AAF64(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3, s32 u1, s32 v1, s32 u2,
                  s32 v2, s32 u3, s32 v3, s32 *v_out) {
    s32 dx, ex, dz, ez;
    f32 f0, f2, f4, f6, f8, f10, f12, f14, f16, f18, f20;

    ENGINE_COST(802AAF64, 69);
    if (x == x1) {
        if (z == z1)
            goto done;
    }
    if (x == x2) {
        if (z == z2) {
            u1 = u2, v1 = v2;
            goto done;
        }
    }
    if (x == x3) {
        if (z == z3) {
            u1 = u3, v1 = v3;
            goto done;
        }
    }
    dx = x1 - x;
    ex = x2 - x3;
    dz = z1 - z;
    ez = z2 - z3;
    if (dx == 0) {
        f10 = func_802AB1B0(ex, dz, ez, x, z, x3, z3, &f20);
        goto map;
    }
    if (ex == 0) {
        f10 = func_802AB234(dx, dz, ez, x, z, x3, z3, &f20);
        goto map;
    }
    if (dz == 0) {
        f10 = func_802AB2B8(dx, ex, ez, x, z, x3, z3, &f20);
        goto map;
    }
    if (ez == 0) {
        f10 = func_802AB33C(dx, ex, dz, x, z, x3, z3, &f20);
        goto map;
    }
    f14 = (f32)dx;
    f6 = 1.0f;
    f18 = (f32)x;
    f0 = (f32)((s64)ex * dz);
    f2 = (f32)((s64)ez * dx);
    f4 = f0 / f2;
    f8 = (f32)((s64)ex * z);
    f4 = f6 - f4;
    f12 = (f32)((s64)ex * z3);
    f10 = f8 / f2;
    f12 = f12 / f2;
    f10 = f10 - f12;
    f12 = (f32)x3;
    f12 = f12 / f14;
    f10 = f10 + f12;
    f12 = (f32)x;
    f12 = f12 / f14;
    f10 = f10 - f12;
    f10 = f10 / f4;
    f16 = f14 * f10;
    f20 = f16 + f18;
    f18 = (f32)x3;
    f20 = f20 - f18;
    f18 = (f32)ex;
    f20 = f20 / f18;
map:
    f16 = (f32)dx * f10;
    f0 = (f32)(u2 - u3);
    f2 = (f32)u3;
    f8 = (f32)u1;
    f14 = (f32)v1;
    f4 = (f32)v3;
    f6 = 1.0f;
    f12 = f6 - f10;
    f0 = f0 * f20;
    f0 = f0 + f2;
    f2 = (f32)(v2 - v3);
    f2 = f2 * f20;
    f8 = f8 * f10;
    f14 = f14 * f10;
    f2 = f2 + f4;
    f8 = f0 - f8;
    f14 = f2 - f14;
    f8 = f8 / f12;
    f14 = f14 / f12;
    u1 = engine_cvt_w_s(f8);
    v1 = engine_cvt_w_s(f14);
    /* what the callers read on: the point on the first triangle (f22,
       f24), and the last temporaries */
done:
    *v_out = v1;
    return u1;
}

/* where (x, z), on object id's world triangle (D_803EBDB0's words), is
   on its own one (the halves at 0x24) */
REGS(a3, t0, t1 -> t3, t4)
s32 func_802AAD0C(s32 id, s32 x, s32 z, s32 *v_out) {
    u8 *p = D_803EBDB0 - 0x38;
    s32 *w;
    s16 *h;

    ENGINE_COST(802AAD0C, 171);
    for (;;) {
        p += 0x38;
        if (*(u16 *)(p + 0x36) != id)
            continue;
        w = (s32 *)p;
        if (!func_802AA5E0(x, z, w[0], w[2], w[3], w[5], w[6], w[8])) {
            continue;
        }
        if (!func_802AA460(x, z, w[0], w[2], w[3], w[5], w[6], w[8])) {
            continue;
        }
        break;
    }
    h = (s16 *)(p + 0x24);
    w = (s32 *)p;
    {
        s32 r = func_802AAF64(x, z, w[0], w[2], w[3], w[5], w[6], w[8], h[0], h[2], h[3], h[5], h[6], h[8], v_out);

        return r;
    }
}

/* the reverse: from its own triangle to the world's */
REGS(a3, t0, t1 -> t3, t4)
s32 func_802AAE54(s32 id, s32 x, s32 z, s32 *v_out) {
    u8 *p = D_803EBDB0 - 0x38;
    s32 *w;
    s16 *h;

    ENGINE_COST(802AAE54, 180);
    for (;;) {
        p += 0x38;
        if (*(u16 *)(p + 0x36) != id)
            continue;
        h = (s16 *)(p + 0x24);
        if (!func_802AA5E0(x, z, h[0], h[2], h[3], h[5], h[6], h[8])) {
            continue;
        }
        if (!func_802AA460(x, z, h[0], h[2], h[3], h[5], h[6], h[8])) {
            continue;
        }
        break;
    }
    w = (s32 *)p;
    {
        s32 r = func_802AAF64(x, z, h[0], h[2], h[3], h[5], h[6], h[8], w[0], w[2], w[3], w[5], w[6], w[8], v_out);

        return r;
    }
}

void func_802AACD4(s32 id, s32 x, s32 z, s16 *u, s16 *v) {
    s32 vv;

    ENGINE_COST(802AACD4, 14);
    *u = func_802AAD0C(id, x, z, &vv);
    *v = vv;
}

void func_802AAE1C(s32 id, s32 x, s32 z, s32 *u, s32 *v) {
    s32 vv;

    ENGINE_COST(802AAE1C, 14);
    *u = func_802AAE54(id, x, z, &vv);
    *v = vv;
}

/* ---- D_803ED3B8's tree: whether a record's parts reach another --------- */

/* Whether any record (type, three part bytes) has id among its parts but
   not as all three, or, where all three are id, whether its type's
   records do (recursively).  Returns 1 or 0 ($t0). */
REGS(s2 -> t0)
s32 func_802AB8D8(s32 id) {
    u8 *p = D_803ED3B8;
    s32 r;

    ENGINE_COST(802AB8D8, 100);
    for (;; p += 4) {
        if (*(s32 *)p == -1) {
            r = 0;
            break;
        }
        if (p[1] == id) {
            if (p[2] != id)
                goto yes;
            if (p[3] != id)
                goto yes;
            if (p[0] != 0) {
                r = func_802AB8D8(p[0]);
                if (r == 1)
                    break;
            }
        } else {
            if (p[2] == id)
                goto yes;
            if (p[3] == id)
                goto yes;
        }
        continue;
    yes:
        r = 1;
        break;
    }
    return r;
}

s32 func_802AB878(s32 id) {
    s32 r;

    ENGINE_COST(802AB878, 24);
    r = func_802AB8D8(id);
    return r;
}

/* ---- a point through a chain of matrices -------------------------------- */

extern u8 D_803EBB98[];                 /* the product's scratch matrix */

/* An Mtx's halves by their words (element 2n is word n's high half; in
   native-endian memory a halfword's address is another, docs/PORT.md) */
static u32 mtx_half(u8 *m, s32 off) {
    u32 w = *(u32 *)(m + (off & ~3));

    return off & 2 ? w & 0xFFFF : w >> 16;
}

static void mtx_set_half(u8 *m, s32 off, u32 v) {
    u32 *w = (u32 *)(m + (off & ~3));

    *w = off & 2 ? (*w & 0xFFFF0000) | (v & 0xFFFF) : (*w & 0xFFFF) | (v << 16);
}

/* a 4x4 16.16 matrix's element at this offset (the integer halves first,
   the fractions 0x20 on) */
static s32 mtx_el(u8 *m, s32 off) {
    return (s32)((mtx_half(m, off) << 16) | mtx_half(m, off + 0x20));
}

/* (x, y, z) through the n matrices at base + offs[i], multiplied in
   order (D_803EBB58, with D_803EBB98 for each product): the result >> 11
   in v0, v1 and a0.  The x row's translation is only its integer part.
   With no matrices it is s0, s1 and s2 >> 11 (whatever the caller has
   there).  The sums stay in s1 and s2. */
REGS(v0, v1, a0, a1, a2, s4, s0, s1, s2 -> v0, v1, a0, s1, s2)
s32 func_802AA890(s32 x, s32 y, s32 z, s32 n, s32 *offs, u8 *base, s32 s0, s32 s1, s32 s2, s32 *y_out, s32 *z_out,
                  s32 *s1_out, s32 *s2_out) {
    u8 *cur = (u8 *)D_803EBB58, *m;
    u32 *s, *d;
    s32 i, j, k;
    u64 sum;

    ENGINE_COST(802AA890, 1449);
    if (n != 0) {
        s = (u32 *)(base + offs[0]);
        d = (u32 *)cur;
        for (k = 16; k != 0; k--) {
            *d++ = *s++;
        }
        m = (u8 *)s;
        offs++, n--;
        for (;;) {
            if (n == 0)
                break;
            m = base + *offs;
            for (i = 0; i != 4; i++) {
                for (j = 0; j != 4; j++) {
                    sum = 0;
                    for (k = 0; k < 4; k++)
                        sum += (u64)((s64)mtx_el(cur, i * 8 + k * 2) * mtx_el(m, k * 8 + j * 2));
                    sum >>= 16;
                    mtx_set_half(D_803EBB98, 0x20 + (i * 4 + j) * 2, (u32)sum);
                    mtx_set_half(D_803EBB98, (i * 4 + j) * 2, (u32)(sum >> 16));
                }
            }
            s = (u32 *)D_803EBB98;
            d = (u32 *)cur;
            for (k = 16; k != 0; k--) {
                *d++ = *s++;
            }
            offs++, n--;
        }
        s0 = (u32)mtx_el(cur, 0x00) * x + (u32)mtx_el(cur, 0x08) * y + (u32)mtx_el(cur, 0x10) * z +
             (mtx_half(cur, 0x18) << 16);
        s1 = (u32)mtx_el(cur, 0x02) * x + (u32)mtx_el(cur, 0x0A) * y + (u32)mtx_el(cur, 0x12) * z +
             (u32)mtx_el(cur, 0x1A);
        s2 = (u32)mtx_el(cur, 0x04) * x + (u32)mtx_el(cur, 0x0C) * y + (u32)mtx_el(cur, 0x14) * z +
             (u32)mtx_el(cur, 0x1C);
    }
    *y_out = s1 >> 11;
    *z_out = s2 >> 11;
    *s1_out = s1;
    *s2_out = s2;
    return s0 >> 11;
}

/* An object's triangles (data: their count, the matrices' count n and
   offsets, then 0x14-byte triangles of three s16 points) into
   D_803EBDB0's 0x38-byte records of this id (new ones past D_803EBBEC's
   end): each point as it is (the halves at 0x24) and through the
   matrices at base (the words). */
REGS(t3, t4, s4, s1, s2)
void func_802AABE4(s32 id, u16 *data, u8 *base, s32 s1, s32 s2) {
    s32 n = data[1], count = data[0], x, y = 0, z = 0;
    s32 *offs = (s32 *)(data + 2);
    s16 *t = (s16 *)((u8 *)data + 4 + n * 4);
    u8 *end = D_803EBBEC, *r = D_803EBDB0;
    s16 *h;
    s32 *w;

    ENGINE_COST(802AABE4, 312);
    for (;;) {
        if (count == 0)
            break;
        count--;
        for (;;) {
            if (r == end) {
                end += 0x38;
                break;
            }
            if (*(u16 *)(r + 0x36) == id)
                break;
            r += 0x38;
        }
        *(u16 *)(r + 0x36) = id;
        h = (s16 *)(r + 0x24);
        w = (s32 *)r;
        h[0] = t[0], h[1] = t[1], h[2] = t[2];
        x = func_802AA890(t[0], t[1], t[2], n, offs, base, (u32)r, s1, s2, &y, &z, &s1, &s2);
        w[0] = x, w[1] = y, w[2] = z;
        h[3] = t[3], h[4] = t[4], h[5] = t[5];
        x = func_802AA890(t[3], t[4], t[5], n, offs, base, (u32)r, s1, s2, &y, &z, &s1, &s2);
        w[3] = x, w[4] = y, w[5] = z;
        h[6] = t[6], h[7] = t[7], h[8] = t[8];
        x = func_802AA890(t[6], t[7], t[8], n, offs, base, (u32)r, s1, s2, &y, &z, &s1, &s2);
        w[6] = x, w[7] = y, w[8] = z;
        t += 10;
        r += 0x38;
    }
    D_803EBBEC = end;
}

/* ---- the speed on a slope ------------------------------------------------ */

/* The speed *speed: held to 1.5 times the vehicle's top speeds (unk78[13]
   forward, unk78[0] in reverse) when `limit`, then, with all three wheels
   on the ground (none of wheels[] 1), pushed by the slope (func_802A8590
   of the heights h, over f2, times -4) and brought toward 0 by 3 (in
   modes 0, 2, 9 and 0x10) or 1.  Not at all with D_80370C22 set and the
   vehicle stopped.  Returns the speed, or 1 with a wheel off the ground
   ($t1). */
REGS(t6, t8, t7, s0, s7, f2, gp -> t1)
s32 func_802A843C(s16 *speed, s32 limit, s32 mode, s8 *wheels, s32 *h, f32 div, VS *vs) {
    s32 t1, t2, lim;
    f32 f;

    ENGINE_COST(802A843C, 58);
    if (D_80370C22 != 0) {
        t1 = *speed;
        if (t1 == 0)
            goto done;
    }
    if (limit != 0) {
        t1 = *speed;
        if (t1 >= 0) {
            lim = vs->unk78[13];
            lim += lim >> 1;
            if (lim < t1) {
                *speed = lim;
            }
        } else {
            lim = vs->unk78[0];
            lim += lim >> 1;
            if (t1 < lim) {
                *speed = lim;
            }
        }
    }
    t1 = 1;
    if (wheels[0] == 1)
        goto done;
    if (wheels[1] == 1)
        goto done;
    if (wheels[2] == 1)
        goto done;
    t1 = func_802A8590(h);
    f = (f32)t1 / div;
    f = f * -4.0f;
    t2 = engine_cvt_w_s(f);
    t1 = *speed + t2;
    if (mode == 0)
        goto three;
    if (mode == 2)
        goto three;
    if (mode == 0x10)
        goto three;
    if (mode == 9)
        goto three;
    t2 = 1;
    goto have;
three:
    t2 = 3;
have:
    if (t1 != 0) {
        if (t1 <= 0) {
            t1 += t2;
            if (t1 > 0)
                goto zero;
        } else {
            t1 -= t2;
            if (t1 < 0)
                goto zero;
        }
        goto store;
    zero:
        t1 = 0;
    }
store:
    *speed = t1;
done:
    return t1;
}

/* ---- the vehicle's pitch and roll from its wheels' heights --------------- */

extern s16 D_803ED402, D_803ED404;     /* the wheels' spans: along, across */

/* The angles of the wheels' heights (D_803ED3A8: the front wheel's, then
   the two others), each difference over its span through func_802ACF64,
   / 16: across ($a3, negative when the second is higher) and along
   ($a1, negative when the third is lower). */
REGS(-> a1, a3)
s32 func_802A8B10(s32 *across_out) {
    s32 h0 = D_803ED3A8[0], h1 = D_803ED3A8[1], h2 = D_803ED3A8[2];
    s32 along = D_803ED402, span = D_803ED404, d, q, a, t2, t3;

    ENGINE_COST(802A8B10, 58);
    d = h1 - h0;
    if (d >= 0) {
        ENGINE_DIV(q, (s32)((u32)d << 16), span, 802A8B74, 802A8B78, 802A8B84, 802A8B8C);
        a = func_802ACF64(q);
        t2 = -(s32)((u32)a >> 4);
    } else {
        d = -d;
        ENGINE_DIV(q, (s32)((u32)d << 16), span, 802A8BC8, 802A8BCC, 802A8BD8, 802A8BE0);
        a = func_802ACF64(q);
        t2 = (u32)a >> 4;
    }
    d = h2 - h0;
    if (d >= 0) {
        ENGINE_DIV(q, (s32)((u32)d << 16), along, 802A8C14, 802A8C18, 802A8C24, 802A8C2C);
        a = func_802ACF64(q);
        t3 = (u32)a >> 4;
    } else {
        d = -d;
        ENGINE_DIV(q, (s32)((u32)d << 16), along, 802A8C64, 802A8C68, 802A8C74, 802A8C7C);
        a = func_802ACF64(q);
        t3 = -(s32)((u32)a >> 4);
    }
    *across_out = t2;
    return t3;
}

/* ---- the angle to turn a vehicle on a moving object to ------------------- */

extern s16 D_803ED3E8;                  /* an angle func_802A94A4 turns by */

/* Of the angle *angle turned either way by the corner angle between the
   two points (x, z) and (x2, z2) as object id's own (func_802AAE54 and
   func_802ABB1C, in steps of 0x400 up to 0xC00), the one whose first
   wheel (func_802A94A4 of pts) comes nearer the second point ($t0). */
REGS(a3, t0, t1, s1, s2, v1, a2 -> t0)
s32 func_802AB9A4(s32 id, s32 x, s32 z, s32 x2, s32 z2, s16 *pts, u16 *angle) {
    s32 ox, oz, px, pz, dx, dz, s5 = 0, a, t0, t1, s1, s2, s3, s4, s6;

    ENGINE_COST(802AB9A4, 59);
    ox = func_802AAE54(id, x, z, &oz);
    px = func_802AAE54(id, x2, z2, &pz);
    dx = func_802A94A4(0, pts, (s16 *)angle, &dz);
    s6 = func_802ABB1C(px, pz, dx, dz, ox, oz);
    if (s6 >= 0x401) {
        s5 = *angle + 0x400;
        if (s5 >= 0x1000) {
            s5 -= 0xFFF;
        }
        D_803ED3E8 = s5;
        dx = func_802A94A4(0, pts, &D_803ED3E8, &dz);
        s6 = func_802ABB1C(px, pz, dx, dz, ox, oz);
        s6 += 0x400;
        if (s6 >= 0x801) {
            s5 = *angle + 0x800;
            if (s5 >= 0x1000) {
                s5 -= 0xFFF;
            }
            D_803ED3E8 = s5;
            dx = func_802A94A4(0, pts, &D_803ED3E8, &dz);
            s6 = func_802ABB1C(px, pz, dx, dz, ox, oz);
            s6 += 0x800;
        }
    }
    a = *angle;
    t0 = a + s6;
    if (t0 >= 0x1000) {
        t0 -= 0xFFF;
    }
    D_803ED3E8 = t0;
    dx = func_802A94A4(0, pts, &D_803ED3E8, &dz);
    t1 = a - s6;
    s1 = dx + ox;
    s2 = dz + oz;
    if (t1 < 0) {
        t1 += 0xFFF;
    }
    D_803ED3E8 = t1;
    dx = func_802A94A4(0, pts, &D_803ED3E8, &dz);
    s3 = dx + ox;
    s4 = dz + oz;
    s1 -= px;
    s2 -= pz;
    s3 -= px;
    s4 -= pz;
    if (s1 < 0) {
        s1 = -s1;
    }
    if (s2 < 0) {
        s2 = -s2;
    }
    s1 += s2;
    if (s3 < 0) {
        s3 = -s3;
    }
    if (s4 < 0) {
        s4 = -s4;
    }
    s3 += s4;
    if (!(s1 < s3)) {
        t0 = t1;
    }
    return t0;
}

/* ---- a wheel landing hard ------------------------------------------------ */

#include "game/audio.h"

extern u8 D_803ED3F6, D_803ED40B;
SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);

/* Wheel i's fall: y less the drop its state predicts (h[i] times
   state[i] - 2, plus D_803EBBF4 times its square), as a distance.  Past
   D_803ED3F6 the wheel lands (func_802A9540 at the ground g, the distance
   over D_803ED3F7), with a sound (12) when D_803ED40B; otherwise it's in
   the air (D_803ED3EE[i] = 0).  Returns the distance ($s6). */
REGS(v0, a1, a2, a3, t3, s6 -> s6)
s32 func_802A9710(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 y) {
    s32 t2 = state[i] - 2, t5;
    u32 t4 = (u32)h[i] * (u32)t2;
    f32 f;

    ENGINE_COST(802A9710, 84);
    f = D_803EBBF4 * (f32)(s32)((u32)t2 * (u32)t2);
    t5 = engine_cvt_w_s(f);
    y -= (s32)(t4 + t5);
    if (y < 0) {
        y = -y;
    }
    if (D_803ED3F6 < y) {
        if (D_803ED40B != 0) {
            func_80260650(D_80367738, 0xC, NULL);
        }
        if (D_803ED3F7 == 0) {
            engine_break(0x802A98F0, 7);
        }
        y = (u32)y / D_803ED3F7;
        func_802A9540(i, h, state, ground, g, y);
    } else {
        WHEEL_BYTE(D_803ED3EE, i) = 0;
    }
    return y;
}

/* ---- one wheel's step ---------------------------------------------------- */

extern u8 D_803F7C49;

REGS(v0, a1, a2, a3, t3, s6 -> s6)
s32 func_802A9710(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 y);

/* Wheel i one step on (its state counts up): where it would be (ground[i]
   plus the drop its state gives), against the ground under it
   (func_802A9B1C).  Above the ground it falls on (D_803ED398[i], flag
   D_803ED3EE[i]); otherwise it lands on it (func_802A9710).  Vehicle 9
   with D_803F7C49 set also falls when the ground is 0x6A5 or more above.
   Returns the drop, or func_802A9710's distance ($s6). */
REGS(v0, a1, a2, a3, t0, t1, s7, v1, s4, t8, gp, fp -> s6)
s32 func_802A95A4(s32 i, s32 *h, s32 *state, s32 *ground, s32 x, s32 z, s32 *heights, s16 *pts, s16 *a,
                  s32 self, VS *vs, s32 mat) {
    s32 t2, t5, s1, s6, dx, dz, g;
    u32 t4;
    s32 *p;
    f32 f;

    ENGINE_COST(802A95A4, 81);
    t2 = state[i];
    t4 = (u32)h[i] * (u32)t2;
    state[i] = t2 + 1;
    s1 = ground[i];
    f = D_803EBBF4 * (f32)(s32)((u32)t2 * (u32)t2);
    t5 = engine_cvt_w_s(f);
    s6 = (s32)(t4 + t5);
    s1 += s6;
    dx = func_802A94A4(i, pts, a, &dz);
    g = func_802A9B1C(i, x + dx, z + dz, heights[3 * i], self, vs, mat);
    p = (s32 *)((u32)&D_803ED398 + 4 * i);
    if (self == 9) {
        if (D_803F7C49 != 0) {
            if ((s32)((u32)g - s1) >= 0x6A5)
                goto fall;
        }
    }
    if (!(g < s1)) {
        *p = g;
        s6 = func_802A9710(i, h, state, ground, g, s6);
        goto done;
    }
fall:
    if (s1 < 0) {
        s1 = 0;
    }
    *p = s1;
    WHEEL_BYTE(D_803ED3EE, i) = 1;
done:
    return s6;
}

/* ---- turning toward an angle --------------------------------------------- */

extern u8 D_803ED3F8;                   /* the turning sound's count */
extern f32 D_803ED3FC, D_8030D890;      /* its pitch, and the speed's part in it */
void func_80260AB8(SndState *state, s16 type, s32 param);

/* The angle *angle (0 to 0xFFF) a step toward *target: rate
   (func_802A8314), times the gears (unk50) over the speed *speed when
   it isn't 0, the short way round, not past it.  Unless *turning, it is
   done when within 0x156 (*out = the target).  With `sound`, every
   seventh call plays the turn (10) at a pitch from the speed.  *out gets
   the angle, and *turning clears when it is there.  Leaves the angle in
   $t2 and the target in $t3. */
REGS(t5, t6, t1, s4, s5, s6, s7, gp)
void func_802A7FD8(s32 rate, s16 *speed, u16 *angle, u16 *target, u16 *out, s8 *turning, s32 sound, VS *vs) {
    s32 v, t2, t3, t4, t5, n;
    u32 p;
    f32 f;

    ENGINE_COST(802A7FD8, 62);
    t5 = func_802A8314(rate);
    v = *speed;
    t2 = *angle;
    t3 = *target;
    if (v != 0) {
        t5 = (s32)((u32)t5 * vs->unk50);
        ENGINE_DIV(t5, t5, v, 802A801C, 802A8020, 802A802C, 802A8034);
    }
    t4 = t2 - t3;
    if (t5 < 0) {
        t5 = -t5;
    }
    if (t4 < 0) {
        t4 = -t4;
    }
    if (t4 >= 0x801) {
        /* the other way round, through 0 */
        if (t3 < t2) {
            t2 += t5;
            if (t2 < 0x1000)
                goto store;
            t2 -= 0x1000;
            if (t3 < t2)
                goto there;
            goto store;
        }
        t2 -= t5;
        if (t2 >= 0)
            goto store;
        t2 += 0x1000;
        if (t2 < t3)
            goto there;
        goto store;
    }
    if (t2 < t3) {
        t2 += t5;
        if (t2 >= 0x1000)
            goto there;
        if (t3 < t2)
            goto there;
        goto store;
    }
    t2 -= t5;
    if (t2 < 0)
        goto there;
    if (t2 < t3)
        goto there;
    goto store;
there:
    t2 = t3;
store:
    *angle = t2;
    if (*turning == 0) {
        t4 = t2 - t3;
        if (t4 < 0) {
            t4 = -t4;
        }
        if (t4 >= 0x801) {
            t4 = 0x1000 - t4;
        }
        if (t4 < 0x156) {
            *out = t3;
            goto done;
        }
        *turning = 1;
    }
    if (sound != 0) {
        n = D_803ED3F8 + 1;
        if (n >= 7) {
            D_803ED3F8 = 0;
            n = 0;
            p = v;
            if (v < 0) {
                p = -v;
            }
            f = (f32)(s32)p * D_8030D890;
            f = 0.5f + f;
            D_803ED3FC = f;
            {
                SndState *st = func_80260650(D_80367738, 0xA, NULL);

                func_80260AB8(st, 0x10, *(s32 *)&D_803ED3FC);
            }
        }
        D_803ED3F8 = n;
    }
    *out = t2;
    if (t2 == t3) {
        *turning = 0;
    }
done:
    ;
}

/* whether h lies inside the camera's range from lo to hi (12-bit, the
   range wrapping when hi < lo) narrowed by 0x78 at each end (the
   original's two copies of the test, which differ in where they're
   charged; a narrowed range that's empty holds nothing) */
#define IN_RANGE(h, lo, hi, B_WRAP, B_WRAP_LO, B_WRAP_HI0, B_WRAP_HI, B_CMP, B_LO, B_HI, B_OUT, B_NOWRAP,      \
                 B_NW_LO, B_NW_HI, IN, OUT)                                                                       \
    do {                                                                                                         \
        s32 t1_, t2_;                                                                                            \
        if ((hi) < (lo)) {                                                                                       \
            ENGINE_BLK(B_WRAP);                                                                                  \
            t1_ = (lo) + 0x78;                                                                                   \
            t2_ = (hi) - 0x78;                                                                                   \
            if (t1_ >= 0x1000) {                                                                                 \
                ENGINE_BLK(B_WRAP_LO);                                                                           \
                t1_ -= 0x1000;                                                                                   \
            }                                                                                                    \
            ENGINE_BLK(B_WRAP_HI0);                                                                              \
            if (t2_ < 0) {                                                                                       \
                ENGINE_BLK(B_WRAP_HI);                                                                           \
                t2_ += 0x1000;                                                                                   \
            }                                                                                                    \
            ENGINE_BLK(B_CMP);                                                                                   \
            if (!(t2_ < t1_))                                                                                    \
                goto OUT;                                                                                        \
            ENGINE_BLK(B_LO);                                                                                    \
            if (!((h) < t1_))                                                                                    \
                goto IN;                                                                                         \
            ENGINE_BLK(B_HI);                                                                                    \
            if (!(t2_ < (h)))                                                                                    \
                goto IN;                                                                                         \
            ENGINE_BLK(B_OUT);                                                                                   \
            goto OUT;                                                                                            \
        }                                                                                                        \
        ENGINE_BLK(B_NOWRAP);                                                                                    \
        t1_ = (lo) + 0x78;                                                                                       \
        t2_ = (hi) - 0x78;                                                                                       \
        if (!(t1_ < t2_))                                                                                        \
            goto OUT;                                                                                            \
        ENGINE_BLK(B_NW_LO);                                                                                     \
        if ((h) < t1_)                                                                                           \
            goto OUT;                                                                                            \
        ENGINE_BLK(B_NW_HI);                                                                                     \
        if (t2_ < (h))                                                                                           \
            goto OUT;                                                                                            \
        goto IN;                                                                                                 \
    } while (0)

/* The camera's turn toward h2 (or toward h, kept inside the headings'
   range D_803A7410-D_803A7412 less 0x78 at each end, or the nearer end,
   or the middle when the range is too narrow): the heading ($a0) and the
   rate (the speed unk76 times rate, signed toward it, $a1) */
REGS(a0, a1, f0, gp -> a0, a1)
s32 func_802A71DC(s32 h, s32 h2, f32 rate, VS *vs, s32 *rate_out) {
    s32 lo = (u16)D_803A7410, hi = (u16)D_803A7412, dl, dh, s, d, at;
    f32 f;

    ENGINE_COST(802A71DC, 56);
    IN_RANGE(h2, lo, hi, 802A7218, 802A7228, 802A722C, 802A7234, 802A7238, 802A7244, 802A724C, 802A7254, 802A725C,
             802A7270, 802A7278, h2_in, h2_out);
h2_in:
    *rate_out = 0;
    return h2;
h2_out:
    IN_RANGE(h, lo, hi, 802A7298, 802A72A8, 802A72AC, 802A72B4, 802A72B8, 802A72C4, 802A72CC, 802A72D4, 802A72DC,
             802A72F0, 802A72F8, h_in, h_out);
h_out:
    /* to the nearer end of the range */
    dl = lo - h;
    if (dl < 0) {
        dl = -dl;
    }
    if (dl >= 0x801) {
        dl = 0xFFF - dl;
    }
    dh = hi - h;
    if (dh < 0) {
        dh = -dh;
    }
    if (dh >= 0x801) {
        dh = 0xFFF - dh;
    }
    if (dh < dl) {
        h = hi - 0x78;
        if (h < 0) {
            h += 0xFFF;
        }
    } else {
        h = lo + 0x78;
        if (h >= 0x1000) {
            h -= 0xFFF;
        }
    }
    /* and when that end is outside the range itself, the middle */
    if (hi < lo) {
        if (!(h < lo))
            goto h_in;
        if (!(hi < h))
            goto h_in;
        goto mid;
    }
    if (h < lo)
        goto mid;
    if (hi < h)
        goto mid;
    goto h_in;
mid:
    h = func_802A6F6C();
h_in:
    s = vs->unk76;
    at = h < h2;
    if (s < 0) {
        s = -s;
    }
    f = (f32)s;
    rate = rate * f;
    s = engine_cvt_w_s(rate);
    if (at) {
        d = h - h2;
        if (d < 0) {
            d = -d;
        }
        if (d < 0x801)
            goto neg;
        goto pos;
    }
    d = h - h2;
    if (d < 0x801)
        goto pos;
neg:
    *rate_out = -s;
    goto done;
pos:
    *rate_out = s;
done:
    return h;
}

/* ---- out of the level's bounds ------------------------------------------- */

extern s16 D_803BE730, D_803BE732, D_803BE734, D_803BE736; /* the level's bounds (game/level.h) */
extern s32 D_802E8BDC;                  /* the level */
extern u8 *PTR32 D_803BE6F8;            /* the start points: 9-byte records, a key, then (x, y, z) as s16 pairs of bytes */
extern u8 D_80364412;
void func_80277EDC(s32 a, s32 b, s32 c, s32 d);

/* us.v10 has no first check, so its blocks are 0x64 earlier from there on */
#ifdef VERSION_US_V10
#define A8CCC_BLK(v11, v10) ENGINE_BLK(v10)
#else
#define A8CCC_BLK(v11, v10) ENGINE_BLK(v11)
#endif

/* With (x, z) outside the level's bounds (D_803BE730-D_803BE736, in 32s;
   and in us.v11 and jp, vehicle 7 short of a line on levels 0, 0x12 and
   0xD): the vehicle back at its start point (D_803BE6F8's record of key
   type, or 1, or the vehicle when D_80364AA8 is 1 or 0x80) in *xo, *yo,
   *zo, its state at rest (func_802A754C, the nine words from 4 the
   height), the matrices (func_802AC6FC) and a sound (func_80277EDC).
   Returns whether it put it back (the original leaves its x and z in $t0
   and $t1 then, which its caller goes on with). */
REGS(t0, t1, t7, s2, s1, t8, gp -> v0)
s32 func_802A8CCC(s32 x, s32 z, s32 *xo, s32 *yo, s32 *zo, s32 type, VS *vs) {
    s32 cx = x >> 5, cz = z >> 5, key, rx, ry, rz, i;
    u8 *r;

    ENGINE_COST(802A8CCC, 36);
#ifndef VERSION_US_V10
    if (type == 7) {
        if (D_802E8BDC == 0) {
            if (cx < 0x3E8)
                goto respawn;
        } else {
            if (D_802E8BDC == 0x12) {
                if (cz < 0x384)
                    goto respawn;
            } else {
                if (D_802E8BDC == 0xD) {
                    if (cz < 0x44C)
                        goto respawn;
                }
            }
        }
    }
#endif
    if (cx < D_803BE730)
        goto respawn;
    if (D_803BE732 < cx)
        goto respawn;
    if (cz < D_803BE734)
        goto respawn;
    if (!(D_803BE736 < cz))
        goto done;
respawn:
    key = D_80364AA8;
    if (key != 1) {
        if (key != 0x80) {
            key = 1;
            goto have;
        }
    }
    key = type;
have:
    for (r = D_803BE6F8;; r += 9) {
        if (r[0] == key)
            break;
    }
    D_80364412 = 1;
    rx = (s32)(((u32)((s8)r[1] << 8) | r[2]) << 5);
    ry = (s32)(((u32)((s8)r[3] << 8) | r[4]) << 5);
    rz = (s32)(((u32)((s8)r[5] << 8) | r[6]) << 5);
    *xo = rx;
    *yo = ry;
    *zo = rz;
    func_802A754C(vs);
    for (i = 1; i <= 9; i++)
        ((s32 *)vs)[i] = ry;
    func_802AC6FC(rx, ry, rz, 0x13, 1000000);
    func_80277EDC(1, 1, 4, 0x6C);
    return 1;
done:
    return 0;
}

/* The points of a part list (p to end: (x, y, z) s16s, the matrices'
   count and offsets) through their matrices at base (func_802AA890), into
   D_803EBC10's records of this id from the first of it (or its end) on.
   (A point with no matrices would be whatever the original's s0, s1 and
   s2 held: no model has one.) */
static void abbec(s32 id, u8 *p, u8 *end, u8 *base) {
    u8 *r = D_803EBC10;
    s16 *h;
    s32 x, y, z, n, s1, s2;

    ENGINE_COST(802ABBEC, 115);
    for (;;) {
        if (*(s32 *)r == -1)
            break;
        if (r[0xC] == id)
            break;
        r += 0x10;
    }
    for (;;) {
        if (p == end)
            break;
        h = (s16 *)p;
        n = (u16)h[3];
        x = func_802AA890(h[0], h[1], h[2], n, (s32 *)(p + 8), base, 0, 0, 0, &y, &z, &s1, &s2);
        ((s32 *)r)[0] = x;
        ((s32 *)r)[1] = y;
        ((s32 *)r)[2] = z;
        r[0xC] = id;
        p += n * 4 + 8;
        r += 0x10;
    }
}

/* ---- a wheeled vehicle one step on the ground ---------------------------- */

extern s16 D_803ED406;                  /* *$s4 (the heading) */
extern u8 D_803ED40E, D_803ED40F;
void func_802582C4(u8 type, s32 x, s32 y, s32 z, s32 h, s32 along, s32 across, s32 heading);

/* The wheeled vehicle (type, at (x, z), vs) one step on: back in the
   level's bounds (func_802A8CCC), the spans (t9 along, fp across), its
   heading (vs->unk4C, s4) and first flag (vs->unk96, s0) kept; then each
   of its three wheels on the ground (func_802A93B0) or stepping
   (func_802A95A4) as its flag in s0 says (from the second on only when
   D_803ED410 is clear, else func_802A9038).  The wheels' heights
   (D_803ED398) go into vs->unk4's three triples (s7) and the flags back
   into s0 (the original takes s0, s4 and s7 in those registers: every
   caller has them from vs); *px and *pz are (x, z), *py the
   average of the two rear wheels; D_803ED390's angles from the height
   differences over the spans (func_802ACF64); the sound (func_802582C4
   with the angles of func_802A8B10); the gears (func_802A9164); and the
   wheels on a moving object (func_802A92C8). */
REGS(t0, t1, t7, s1, s2, t8, t9, fp, v1, a1, a2, a3, t3, gp)
void func_802A8768(s32 x, s32 z, s32 *px, s32 *pz, s32 *py, s32 type, s32 t9, s32 fp, s16 *v1, s32 *a1, s32 *a2,
                   s32 *a3, s16 *t3, VS *vs) {
    u8 *s0 = (u8 *)vs->unk96;
    s16 *s4 = (s16 *)&vs->unk4C;
    s32 *s7 = vs->unk4;
    s32 i, t5, h, h0, h1, h2, s5, q, r, along, across, t2;

    ENGINE_COST(802A8768, 181);
    /* ($t7, $t8 and $t9: 5CB60.c's collision triangles and 69BB0.c's driver
       read them from the context) */
    ENGINE_LEAVE(15, (u32)px);
    ENGINE_LEAVE(24, type);
    ENGINE_LEAVE(25, t9);
    if (func_802A8CCC(x, z, px, py, pz, type, vs)) {
        x = *px;
        z = *pz;
    }
    D_803ED402 = t9;
    D_803ED404 = fp;
    D_803ED406 = *s4;
    D_803ED40E = *s0;
    vs->unk9B = 0;
    func_802A90E4((u16 *)v1);
    func_802A8FF4(vs);
    func_802A8FB4();
    for (i = 0;;) {
        if (D_803ED410 != 0) {
            if (i != 0) {
                func_802A9038(vs);
                break;
            }
        }
        t5 = (s8)s0[i];
        if (t5 == 1) {
            r = func_802A95A4(i, a1, a2, a3, x, z, s7, v1, s4, type, vs, fp);
            if (++i == 3)
                break;
            continue;
        }
        if (t5 != 0) {
            engine_syscall(0x802A87FC);
        }
        r = func_802A93B0(i, v1, a1, a2, a3, x, z, s4, s7, type, vs, fp);
        if (++i == 3) {
            break;
        }
    }
    s0[0] = (&D_803ED3EE)[0];
    s0[1] = (&D_803ED3EE)[1];
    s0[2] = (&D_803ED3EE)[2];
    h0 = (&D_803ED398)[0], h1 = (&D_803ED398)[1], h2 = (&D_803ED398)[2];
    s7[2] = s7[1];
    s7[1] = s7[0];
    s7[0] = h0;
    s7[5] = s7[4];
    s7[4] = s7[3];
    s7[3] = h1;
    t2 = s7[6];
    s7[8] = s7[7];
    s7[7] = t2;
    s7[6] = h2;
    *px = x;
    *pz = z;
    *py = ((u32)h1 + (u32)h2) >> 1;
    /* the pitch (D_803ED390[2]) from the second wheel's height over fp */
    s5 = h1 - h0;
    if (s5 >= 0) {
        ENGINE_DIV(q, (s32)((u32)s5 << 16), fp, 802A88D4, 802A88D8, 802A88E4, 802A88EC);
        r = func_802ACF64(q);
        r = -(s32)((u32)r >> 4);
    } else {
        s5 = -s5;
        ENGINE_DIV(q, (s32)((u32)s5 << 16), fp, 802A8934, 802A8938, 802A8944, 802A894C);
        r = func_802ACF64(q);
        r = (u32)r >> 4;
    }
    D_803ED390[2] = r;
    /* the roll (D_803ED390[0]) from the third's over t9 */
    s5 = h2 - h0;
    if (s5 >= 0) {
        ENGINE_DIV(q, (s32)((u32)s5 << 16), t9, 802A898C, 802A8990, 802A899C, 802A89A4);
        r = func_802ACF64(q);
        r = (u32)r >> 4;
    } else {
        s5 = -s5;
        ENGINE_DIV(q, (s32)((u32)s5 << 16), t9, 802A89E8, 802A89EC, 802A89F8, 802A8A00);
        r = func_802ACF64(q);
        r = -(s32)((u32)r >> 4);
    }
    D_803ED390[0] = r;
    fp = r;
    ENGINE_LEAVE(30, fp);               /* ($fp: 5CB60.c and the other vehicles read it from the context) */
    /* the sound */
    h = ((u32)D_803ED3A8[1] + (u32)D_803ED3A8[2]) >> 1;
    h1 = ((u32)h1 + (u32)h2) >> 1;
    along = func_802A8B10(&across);
    func_802582C4(type, *px, h, *pz, h1, along, across, D_803ED406);
    func_802A9164(s0, type, vs);
    if (D_803ED3F5 != 0)
        goto out;
    if (type == 0xFF)
        goto out;
    if (D_803ED40F == 0)
        goto out;
    func_802A92C8(*px, *pz, vs->unk5E, (s16 *)&vs->unk4C, vs->unk4, type, vs, fp);
out:
    ;
}

/* func_802ABBEC as the vehicle modules declare it (shared.h).  Its $t0
   and $t2 stay in the context: 56040.c's func_8029A800 takes those of a
   vehicle that doesn't set them itself from there. */
REGS(t0, t1, t2, s4)
void func_802ABBEC(s32 id, u8 *verts, u8 *end, u8 *buf) {
    ENGINE_LEAVE(8, id);
    ENGINE_LEAVE(10, (u32)end);
    abbec(id, verts, end, buf);
}

