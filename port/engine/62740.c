/*
 * hd_code 62740 (us.v11 0x802A6F00-0x802AC1A0): the vehicles' shared
 * physics, as native C (engine.h).  Most of it works on a VehicleState
 * (vehicle.h) that every vehicle module passes in $gp: its heading
 * (unk4C, unk4E: 12-bit angles), its speed (unk76), its gear table (unk78:
 * five (s16, s16, s16) rows) and its flags.
 *
 * Still translated: the functions whose register conventions are all
 * pass-through, which go native with their callers (the vehicle modules).
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

    ENGINE_BLK(802A6F00);
    if (D_80367C10 != 0) {
        ENGINE_BLK(802A6F20);
        p = vs->unk78;
        for (n = 5;; n--) {
            ENGINE_BLK(802A6F28);
            if (n == 0)
                break;
            ENGINE_BLK(802A6F30);
            p[0] = p[0] << 1;
            p[1] = p[1] << 1;
            p += 3;
        }
    }
    ENGINE_BLK(802A6F54);
}

/* the angle halfway between the two camera headings (12-bit, wrapping) */
s32 func_802A6F6C(void) {
    s32 a = (u16)D_803A7410, b = (u16)D_803A7412, r;

    ENGINE_BLK(802A6F6C);
    if (b < a) {
        ENGINE_BLK(802A6F98);
        r = ((0xFFF - a + b) >> 1) + a;
        if (r >= 0x1000) {
            ENGINE_BLK(802A6FB8);
            r -= 0xFFF;
        }
        ENGINE_BLK(802A6FC0);
    } else {
        ENGINE_BLK(802A6FC8);
        r = (u32)(a + b) >> 1;
    }
    ENGINE_BLK(802A6FD0);
    return r;
}

/* the speed brought toward a1 by 3 (D_80358064), and its sign in unkA5 */
REGS(a1, gp)
void func_802A6FE4(s32 limit, VS *vs) {
    s32 v;

    ENGINE_BLK(802A6FE4);
    if (D_80358064 != 0) {
        ENGINE_BLK(802A7004);
        v = vs->unk76;
        if (v > 0) {
            ENGINE_BLK(802A7018);
            vs->unkA5 = 1;
        } else {
            ENGINE_BLK(802A7010);
            vs->unkA5 = -1;
        }
        ENGINE_BLK(802A701C);
        if (v >= 0) {
            ENGINE_BLK(802A7028);
            v -= 3;
            ENGINE_LEAVE(1, limit < v);         /* ($at, the compare's) */
            if (!(limit < v)) {
                ENGINE_BLK(802A7038);
                v = limit;
            }
        } else {
            ENGINE_BLK(802A7040);
            v += 3;
            ENGINE_LEAVE(1, v < -limit);
            if (!(v < -limit)) {
                ENGINE_BLK(802A7050);
                v = -limit;
            }
        }
        ENGINE_BLK(802A7054);
        vs->unk76 = v;
    }
    ENGINE_BLK(802A7058);
}

/* unk4E: the heading *t7, turned round when the speed's sign isn't unkA5 */
REGS(t7, gp)
void func_802A7070(s16 *heading, VS *vs) {
    s32 sign, v;

    ENGINE_BLK(802A7070);
    if (vs->unk76 > 0) {
        ENGINE_BLK(802A7094);
        sign = 1;
    } else {
        ENGINE_BLK(802A708C);
        sign = -1;
    }
    ENGINE_BLK(802A7098);
    if (vs->unkA5 == sign) {
        ENGINE_BLK(802A70BC);
        v = *heading;
    } else {
        ENGINE_BLK(802A70A4);
        v = *heading - 0x800;
        if (v < 0) {
            ENGINE_BLK(802A70B4);
            v += 0xFFF;
        }
    }
    ENGINE_BLK(802A70C0);
    vs->unk4E = v;
}

/* when unk4E is more than a quarter turn from unk4C: turned round, the
   speed negated, and the camera's headings turned half round */
REGS(gp)
void func_802A70D8(VS *vs) {
    s32 h = vs->unk4C, lo, hi, a;

    ENGINE_BLK(802A70D8);
    lo = h - 0x400;
    if (lo < 0) {
        ENGINE_BLK(802A70FC);
        lo += 0xFFF;
    }
    ENGINE_BLK(802A7100);
    hi = h + 0x400;
    if (hi >= 0x1000) {
        ENGINE_BLK(802A7110);
        hi -= 0xFFF;
    }
    ENGINE_BLK(802A7114);
    a = vs->unk4E;
    if (hi < lo) {
        ENGINE_BLK(802A7120);
        if (!(a < lo))
            goto done;
        ENGINE_BLK(802A712C);
        if (!(hi < a))
            goto done;
        ENGINE_BLK(802A7134);
    } else {
        ENGINE_BLK(802A713C);
        if (!(a < lo)) {
            ENGINE_BLK(802A7148);
            if (!(hi < a))
                goto done;
        }
    }
    ENGINE_BLK(802A7150);
    a += 0x800;
    if (a >= 0x1000) {
        ENGINE_BLK(802A7160);
        a -= 0xFFF;
    }
    ENGINE_BLK(802A7164);
    vs->unk4E = a;
    vs->unk76 = -vs->unk76;
    a = (u16)D_803A7410 - 0x800;
    if (a < 0) {
        ENGINE_BLK(802A7194);
        a += 0xFFF;
    }
    ENGINE_BLK(802A7198);
    D_803A7410 = a;
    a = (u16)D_803A7412 + 0x800;
    if (0xFFF < a) {
        ENGINE_BLK(802A71B8);
        a -= 0xFFF;
    }
    ENGINE_BLK(802A71BC);
    D_803A7412 = a;
done:
    ENGINE_BLK(802A71C0);
}

/* unk4C turned by `turn` toward unk4E, not past it */
REGS(a1, gp)
void func_802A746C(s32 turn, VS *vs) {
    s32 h = vs->unk4C, target = vs->unk4E, n, at;

    ENGINE_BLK(802A746C);
    /* (what it leaves in $v0, $a0 and $at, $v1, which the vehicle modules
       read on) */
    ENGINE_LEAVE(2, h);
    ENGINE_LEAVE(4, target);
    if (h == target)
        goto done;
    ENGINE_BLK(802A7484);
    n = h + turn;
    if (n < 0) {
        ENGINE_BLK(802A7490);
        n += 0xFFF;
    }
    ENGINE_BLK(802A7494);
    if (n >= 0x1000) {
        ENGINE_BLK(802A74A0);
        n -= 0xFFF;
    }
    ENGINE_LEAVE(3, n);
    ENGINE_BLK(802A74A4);
    if (turn >= 0) {
        ENGINE_BLK(802A74AC);
        if (h < target) {
            ENGINE_BLK(802A74B4);
            at = n < h;
            if (target < n)
                goto clamp;
            ENGINE_BLK(802A74BC);
            if (n < h)
                goto clamp;
            ENGINE_BLK(802A74C4);
            goto set;
        }
        ENGINE_BLK(802A74CC);
        at = n < h;
        if (!(target < h))
            goto set;
        ENGINE_BLK(802A74D8);
        at = target < n;
        if (!(n < h))
            goto set;
        ENGINE_BLK(802A74E0);
        if (!(target < n))
            goto set;
        ENGINE_BLK(802A74E8);
        goto clamp;
    }
    ENGINE_BLK(802A74F0);
    if (target < h) {
        ENGINE_BLK(802A74FC);
        at = n < h;
        if (!(target < n))
            goto clamp;
        ENGINE_BLK(802A7504);
        if (!(n < h))
            goto clamp;
        ENGINE_BLK(802A750C);
        goto set;
    }
    ENGINE_BLK(802A7514);
    at = n < target;
    if (!(h < target))
        goto set;
    ENGINE_BLK(802A7520);
    at = h < n;
    if (!(n < target))
        goto set;
    ENGINE_BLK(802A7528);
    if (!(h < n))
        goto set;
clamp:
    ENGINE_BLK(802A7530);
    vs->unk4C = target;
    ENGINE_LEAVE(1, at);
    goto done;
set:
    ENGINE_BLK(802A7538);
    vs->unk4C = n;
    ENGINE_LEAVE(1, at);
done:
    ENGINE_BLK(802A753C);
}

/* the state at rest */
REGS(gp)
void func_802A754C(VS *vs) {
    s32 k;

    ENGINE_BLK(802A754C);
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
    ENGINE_LEAVE_F(0, 0.0f);
}

/* the current vehicle's parts (0x300 bytes), state (0xA6) and position
   (three words) to D_803EB7A0 */
REGS(v0, v1, a0, a1, gp)
void func_802A75DC(u8 *parts, s32 *x, s32 *y, s32 *z, u8 *vs) {
    u8 *d = D_803EB7A0;
    s32 n;

    ENGINE_BLK(802A75DC);
    for (n = 0x300;; n--) {
        ENGINE_BLK(802A7604);
        if (n == 0)
            break;
        ENGINE_BLK(802A760C);
        *d++ = *parts++;
    }
    ENGINE_BLK(802A7624);
    for (n = 0xA6;; n--) {
        ENGINE_BLK(802A7628);
        if (n == 0)
            break;
        ENGINE_BLK(802A7630);
        *d++ = *vs++;
    }
    ENGINE_BLK(802A7648);
    ((UnalignedWord *)d)[0].v = *x;
    ((UnalignedWord *)d)[1].v = *y;
    ((UnalignedWord *)d)[2].v = *z;
}

/* ... and back, with `n` bytes from src to dst (in doublewords) */
REGS(v0, v1, a0, a1, a2, a3, t0, gp)
void func_802A768C(u8 *parts, s32 *x, s32 *y, s32 *z, u32 *src, u32 *dst, s32 n, u8 *vs) {
    u8 *s = D_803EB7A0;
    s32 k;

    ENGINE_BLK(802A768C);
    for (k = 0x300;; k--) {
        ENGINE_BLK(802A76B8);
        if (k == 0)
            break;
        ENGINE_BLK(802A76C0);
        *parts++ = *s++;
    }
    ENGINE_BLK(802A76D8);
    for (k = 0xA6;; k--) {
        ENGINE_BLK(802A76DC);
        if (k == 0)
            break;
        ENGINE_BLK(802A76E4);
        *vs++ = *s++;
    }
    for (;;) {
        ENGINE_BLK(802A76FC);
        if (n == 0)
            break;
        ENGINE_BLK(802A7704);
        dst[0] = src[0];
        dst[1] = src[1];
        src += 2, dst += 2, n -= 8;
    }
    ENGINE_BLK(802A771C);
    ENGINE_LEAVE(6, (u32)src);          /* ($a2, $a3 as the copy leaves them) */
    ENGINE_LEAVE(7, (u32)dst);
    *x = ((UnalignedWord *)s)[0].v;
    *y = ((UnalignedWord *)s)[1].v;
    *z = ((UnalignedWord *)s)[2].v;
}

/* n bytes (in doublewords) from a to b, or b to a when D_8035805C */
REGS(a0, a1, a2)
void func_802A7764(u32 *a, u32 *b, s32 n) {
    u32 *t;

    ENGINE_BLK(802A7764);
    if (D_8035805C != 0) {
        ENGINE_BLK(802A7788);
        t = a, a = b, b = t;
    }
    for (;;) {
        ENGINE_BLK(802A7794);
        if (n == 0)
            break;
        ENGINE_BLK(802A779C);
        b[0] = a[0];
        b[1] = a[1];
        a += 2, b += 2, n -= 8;
    }
    ENGINE_BLK(802A77B4);
}

/* the speed toward 0 by 8, unless D_80370C2D */
REGS(gp)
void func_802A77D0(VS *vs) {
    s32 v;

    ENGINE_BLK(802A77D0);
    if (D_80370C2D == 0) {
        ENGINE_BLK(802A77EC);
        v = vs->unk76;
        if (v >= 0) {
            ENGINE_BLK(802A77F8);
            v -= 8;
            if (v < 0) {
                ENGINE_BLK(802A7804);
                v = 0;
            }
        } else {
            ENGINE_BLK(802A780C);
            v += 8;
            if (v > 0) {
                ENGINE_BLK(802A7818);
                v = 0;
            }
        }
        ENGINE_BLK(802A781C);
        vs->unk76 = v;
    }
    ENGINE_BLK(802A7820);
}

/* whether x is past the first gear row's (unk78[0]) speed scaled by the
   stick (D_80370C2D / -80) */
REGS(t2, s1 -> t4)
s32 func_802A7A1C(s32 x, s16 *rows) {
    s32 r = 0;

    ENGINE_BLK(802A7A1C);
    ENGINE_BLK(802A7A64);
    ENGINE_BLK(802A7A7C);
    if (x < rows[0] * D_80370C2D / -0x50) {
        ENGINE_BLK(802A7A90);
        r = 1;
    }
    ENGINE_BLK(802A7A94);
    return r;
}

/* the same against the last row's (unk78[13]), / 80 */
REGS(t2, s1 -> t4)
s32 func_802A7AAC(s32 x, s16 *rows) {
    s32 r = 0;

    ENGINE_BLK(802A7AAC);
    ENGINE_BLK(802A7AF4);
    ENGINE_BLK(802A7B0C);
    if (rows[13] * D_80370C2D / 0x50 < x) {
        ENGINE_BLK(802A7B20);
        r = 1;
    }
    ENGINE_BLK(802A7B24);
    return r;
}

/* the gear (third value) of the row whose range [row[0], row[1]] holds x,
   over the rows the vehicle's gears use (unk50); 0 if none.  The rows
   pointer is left past the last row looked at. */
REGS(t2, s1, gp -> t4, s1)
s32 func_802A7C28(s32 x, s16 *rows, VS *vs, u32 *rows_out) {
    s32 n, r = 0;

    ENGINE_BLK(802A7C28);
    if (vs->unk50 == 1) {
        ENGINE_BLK(802A7C4C);
        n = 5;
    } else {
        ENGINE_BLK(802A7C40);
        n = 7 - vs->unk50;
    }
    ENGINE_BLK(802A7C50);
    if ((s8)D_803ED40C != 0) {
        ENGINE_BLK(802A7C64);
        n -= 2;
    }
    for (;;) {
        ENGINE_BLK(802A7C68);
        if (n == 0)
            break;
        ENGINE_BLK(802A7C70);
        n--;
        rows += 3;
        if (x < rows[-3])
            continue;
        ENGINE_BLK(802A7C88);
        if (rows[-2] < x)
            continue;
        ENGINE_BLK(802A7C98);
        r = rows[-1];
        break;
    }
    ENGINE_BLK(802A7C9C);
    *rows_out = (u32)rows;
    return r;
}

/* whether the speed is within `range` of the current gear's top speed
   (the first row's when reversing) */
REGS(v1, gp -> v0)
s32 func_802A7CB0(s32 range, VS *vs) {
    s32 v = vs->unk76, g;

    ENGINE_BLK(802A7CB0);
    if (v >= 0) {
        ENGINE_BLK(802A7CD0);
        if (vs->unk50 == 1) {
            ENGINE_BLK(802A7CEC);
            g = 4;
        } else {
            ENGINE_BLK(802A7CE0);
            g = 6 - vs->unk50;
        }
        ENGINE_BLK(802A7CF0);
        v -= *(s16 *)((u8 *)vs->unk78 + (u32)g * 6 + 2);
        if (v < 0) {
            ENGINE_BLK(802A7D14);
            v = -v;
        }
        ENGINE_BLK(802A7D18);
        if (!(v < range))
            goto no;
        ENGINE_BLK(802A7D24);
    } else {
        ENGINE_BLK(802A7D2C);
        v -= vs->unk78[0];
        if (v < 0) {
            ENGINE_BLK(802A7D3C);
            v = -v;
        }
        ENGINE_BLK(802A7D40);
        if (!(v < range))
            goto no;
    }
    ENGINE_BLK(802A7D4C);
    ENGINE_BLK(802A7D50);
    return 1;
no:
    ENGINE_BLK(802A7D50);
    return 0;
}

/* the gear the flags (unk96[0..2]) allow, for the gearbox's mode */
REGS(t7, s0 -> t3)
s32 func_802A7D68(s32 mode, u8 *flags) {
    s32 n, r;

    ENGINE_BLK(802A7D68);
    if (mode == 0) {
        ENGINE_BLK(802A7DCC);
        if (flags[0] != 0) {
            ENGINE_BLK(802A7DDC);
            if (flags[1] == 0) {
                ENGINE_BLK(802A7DEC);
                r = 2;
            } else {
                ENGINE_BLK(802A7DE4);
                r = 0;
            }
        } else {
            ENGINE_BLK(802A7DF4);
            if (flags[1] == 0) {
                ENGINE_BLK(802A7E04);
                r = 4;
            } else {
                ENGINE_BLK(802A7DFC);
                r = 2;
            }
        }
        goto done;
    }
    ENGINE_BLK(802A7D7C);
    if (mode == 1) {
        ENGINE_BLK(802A7E0C);
        if (flags[2] == 0) {
            ENGINE_BLK(802A7E20);
            r = 4;
        } else {
            ENGINE_BLK(802A7E18);
            r = 0;
        }
        goto done;
    }
    ENGINE_BLK(802A7D88);
    if (mode == 2) {
        ENGINE_BLK(802A7E28);
        r = 0;
        if (flags[0] == 0) {
            ENGINE_BLK(802A7E38);
            r += 1;
        }
        ENGINE_BLK(802A7E3C);
        if (flags[1] == 0) {
            ENGINE_BLK(802A7E48);
            r += 1;
        }
        ENGINE_BLK(802A7E4C);
        if (flags[2] == 0) {
            ENGINE_BLK(802A7E58);
            r += 2;
        }
        goto done;
    }
    ENGINE_BLK(802A7D90);
    if (mode == 4) {
        ENGINE_BLK(802A7DC4);
        r = 4;
        goto done;
    }
    ENGINE_BLK(802A7D98);
    for (n = 3;;) {
        ENGINE_BLK(802A7D9C);
        if (*flags == 0) {
            ENGINE_BLK(802A7DBC);
            r = 4;
            goto done;
        }
        ENGINE_BLK(802A7DA8);
        flags++;
        if (--n == 0)
            break;
    }
    ENGINE_BLK(802A7DB4);
    r = 0;
done:
    ENGINE_BLK(802A7E5C);
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

    ENGINE_BLK(802A7E70);
    if (D_80367C10 != 0) {
        ENGINE_BLK(802A7E88);
        ENGINE_BLK(802A7EA0);
        ENGINE_BLK(802A7EB8);
        rate -= rate / 3;
    }
    ENGINE_BLK(802A7EC4);
    if (D_80370C75 != 0) {
        ENGINE_BLK(802A7ED4);
        if (D_80370C34 != 0) {
            ENGINE_BLK(802A7EEC);
            rate = 0x64;
        } else {
            ENGINE_BLK(802A7EE4);
            rate = 0xC8;
        }
    }
    ENGINE_BLK(802A7EF0);
    D_80370C72 = rate;
    x = D_80370C2C;
    t = *h;
    a = x;
    if (x < 0) {
        ENGINE_BLK(802A7F10);
        a = -x;
    }
    ENGINE_BLK(802A7F14);
    ENGINE_BLK(802A7F38);
    ENGINE_BLK(802A7F50);
    rate = a * rate / 0x50;
    if (x != 0) {
        ENGINE_BLK(802A7F60);
        if (x >= 0) {
            ENGINE_BLK(802A7F68);
            t -= rate;
        } else {
            ENGINE_BLK(802A7F70);
            t += rate;
        }
        ENGINE_BLK(802A7F74);
        if (t < 0) {
            ENGINE_BLK(802A7F7C);
            t += 0x1000;
        }
        ENGINE_BLK(802A7F80);
        if (t >= 0x1000) {
            ENGINE_BLK(802A7F8C);
            t -= 0x1000;
        }
        ENGINE_BLK(802A7F90);
        *h = t;
    }
    ENGINE_BLK(802A7F94);
    if (D_803ED40A != 0) {
        ENGINE_BLK(802A7FA4);
        if (D_803649EE == 0) {
            ENGINE_BLK(802A7FB4);
            t = D_803ED408;
        }
    }
    ENGINE_BLK(802A7FBC);
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
    s32 edge, r = 1, tx, tz, at, set = 0;

    ENGINE_BLK(802AA460);
    cx = (f32)(x2 + x3) / 2.0f;
    cz = (f32)(z2 + z3) / 2.0f;
    cx = ((f32)x1 + cx) / 2.0f;
    cz = ((f32)z1 + cz) / 2.0f;
    fx = (f32)x;
    fz = (f32)z;
    for (edge = 4;;) {
        ENGINE_BLK(802AA4E4);
        at = 3;
        if (--edge == 0)
            break;
        ENGINE_BLK(802AA4F0);
        at = 2;
        if (edge == 3) {
            ENGINE_BLK(802AA538);
            ex = (f32)x1, ez = (f32)z1;
            tx = z2 - z1, tz = x2 - x1;
        } else {
            ENGINE_BLK(802AA4F8);
            if (edge == 2) {
                ENGINE_BLK(802AA51C);
                ex = (f32)x1, ez = (f32)z1;
                tx = z3 - z1, tz = x3 - x1;
            } else {
                ENGINE_BLK(802AA500);
                ex = (f32)x2, ez = (f32)z2;
                tx = z3 - z2, tz = x3 - x2;
            }
        }
        ENGINE_BLK(802AA550);
        dx = (f32)tx;
        dz = (f32)tz;
        a = (fx - ex) * dx;
        b = (fz - ez) * dz;
        d = a - b;
        if (d == 0.0f)
            continue;
        ENGINE_BLK(802AA580);
        a = (cx - ex) * dx;
        b2 = (cz - ez) * dz;
        c = a - b2;
        set = 1;
        if (d > 0.0f) {
            ENGINE_BLK(802AA5B0);
            if (c > 0.0f)
                continue;
        } else {
            ENGINE_BLK(802AA59C);
            if (c < 0.0f)
                continue;
            ENGINE_BLK(802AA5A8);
        }
        ENGINE_BLK(802AA5BC);
        r = 0;
        break;
    }
    ENGINE_BLK(802AA5C0);
    /* (the floats it leaves, which the vehicle modules' code reads on) */
    ENGINE_LEAVE(1, at);
    ENGINE_LEAVE_F(0, 0.0f);
    ENGINE_LEAVE_F(10, fx);
    ENGINE_LEAVE_F(12, fz);
    ENGINE_LEAVE_F(18, cx);
    ENGINE_LEAVE_F(20, cz);
    ENGINE_LEAVE_F(22, c);
    {
        ENGINE_LEAVE_F(2, ex);
        ENGINE_LEAVE_F(4, ez);
        ENGINE_LEAVE_F(26, dx);
        ENGINE_LEAVE_F(28, dz);
        ENGINE_LEAVE_F(14, d);
        ENGINE_LEAVE_F(16, b);
    }
    if (set)
        ENGINE_LEAVE_F(24, b2);
    return r;
}

/* whether (x, z) is inside the triangle's bounding box */
REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA5E0(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    s32 lox = x1, hix = x1, loz = z1, hiz = z1, r;

    ENGINE_BLK(802AA5E0);
    if (x2 < lox) {
        ENGINE_BLK(802AA610);
        lox = x2;
    }
    ENGINE_BLK(802AA614);
    if (x3 < lox) {
        ENGINE_BLK(802AA620);
        lox = x3;
    }
    ENGINE_BLK(802AA624);
    if (hix < x2) {
        ENGINE_BLK(802AA630);
        hix = x2;
    }
    ENGINE_BLK(802AA634);
    if (hix < x3) {
        ENGINE_BLK(802AA640);
        hix = x3;
    }
    ENGINE_BLK(802AA644);
    if (z2 < loz) {
        ENGINE_BLK(802AA650);
        loz = z2;
    }
    ENGINE_BLK(802AA654);
    if (z3 < loz) {
        ENGINE_BLK(802AA660);
        loz = z3;
    }
    ENGINE_BLK(802AA664);
    if (hiz < z2) {
        ENGINE_BLK(802AA670);
        hiz = z2;
    }
    ENGINE_BLK(802AA674);
    if (hiz < z3) {
        ENGINE_BLK(802AA680);
        hiz = z3;
    }
    ENGINE_BLK(802AA684);
    /* ($at as the compares in the delay slots leave it) */
    ENGINE_LEAVE(1, hix < x);
    if (x < lox)
        goto out;
    ENGINE_BLK(802AA690);
    ENGINE_LEAVE(1, z < loz);
    if (hix < x)
        goto out;
    ENGINE_BLK(802AA698);
    ENGINE_LEAVE(1, hiz < z);
    if (z < loz)
        goto out;
    ENGINE_BLK(802AA6A0);
    if (hiz < z)
        goto out;
    ENGINE_BLK(802AA6A8);
    r = 1;
    goto done;
out:
    ENGINE_BLK(802AA6B0);
    r = 0;
done:
    ENGINE_BLK(802AA6B4);
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
    ENGINE_BLK(802AA764);
    func_802ACC68(scale, scale, scale, m);
    ENGINE_BLK(802AA780);
    func_802ACBDC((u16)D_803ED390[0], D_803EBB58);
    ENGINE_BLK(802AA798);
    func_802ACCCC(D_803EBB58, m);
    ENGINE_BLK(802AA7A8);
    func_802ACB50((u16)D_803ED390[2], D_803EBB58);
    ENGINE_BLK(802AA7C0);
    func_802ACCCC(D_803EBB58, m);
    ENGINE_BLK(802AA7D0);
    func_802ACAC4((u16)D_803ED390[1], D_803EBB58);
    ENGINE_BLK(802AA7E8);
    func_802ACCCC(D_803EBB58, m);
    ENGINE_BLK(802AA7F8);
    func_802ACA60(x << 11, y << 11, z << 11, D_803EBB58);
    ENGINE_BLK(802AA810);
    func_802ACCCC(D_803EBB58, m);
    ENGINE_BLK(802AA820);
    func_802AC8CC((u32 *)m);
    ENGINE_BLK(802AA828);
    return m;
}

/* the same, with the angles as arguments */
void func_802AA6D0(s32 x, s32 y, s32 z, s32 ax, s32 ay, s32 az, s32 scale, s32 *m) {
    ENGINE_BLK(802AA6D0);
    D_803ED390[1] = ay;
    D_803ED390[0] = ax;
    D_803ED390[2] = az;
    func_802AA764(x, y, z, scale, m);
    ENGINE_BLK(802AA730);
}

/* 64 bytes from a + off to b + off */
REGS(t0, t1, t2)
void func_802AA838(u8 *a, u8 *b, s32 off) {
    u32 *s = (u32 *)(a + off), *d = (u32 *)(b + off);
    s32 n;

    ENGINE_BLK(802AA838);
    for (n = 8; n != 0; n--) {
        ENGINE_BLK(802AA85C);
        d[0] = s[0];
        d[1] = s[1];
        s += 2, d += 2;
    }
    ENGINE_BLK(802AA874);
}

/* where the line through two points meets another (the four forms of the
   same solve): f10 and f20 */
REGS(v1, a0, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB1B0(s32 v1, s32 a0, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 a, b;

    ENGINE_BLK(802AB1B0);
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

    ENGINE_BLK(802AB234);
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

    ENGINE_BLK(802AB2B8);
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

    ENGINE_BLK(802AB33C);
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

    ENGINE_BLK(802AB3C0);
    for (;;) {
        ENGINE_BLK(802AB3DC);
        if (*(s32 *)p == -1)
            break;
        ENGINE_BLK(802AB3EC);
        if (p[0] != type) {
            p += 4;
            continue;
        }
        ENGINE_BLK(802AB3F8);
        if (p[1] != 0) {
            ENGINE_BLK(802AB404);
            r = 1;
        }
        break;
    }
    ENGINE_BLK(802AB408);
    ENGINE_LEAVE(1, -1);
    return r;
}

/* whether there is a record (a, b) */
REGS(t4, t8 -> a2)
s32 func_802AB41C(s32 a, s32 b) {
    u8 *p = D_803ED3B8;
    s32 r = 0;

    ENGINE_BLK(802AB41C);
    for (;;) {
        ENGINE_BLK(802AB438);
        if (*(s32 *)p == -1)
            break;
        ENGINE_BLK(802AB448);
        if (p[1] != b) {
            p += 4;
            continue;
        }
        ENGINE_BLK(802AB454);
        if (p[0] != a) {
            p += 4;
            continue;
        }
        ENGINE_BLK(802AB460);
        r = 1;
        break;
    }
    ENGINE_BLK(802AB464);
    ENGINE_LEAVE(1, -1);
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

    ENGINE_BLK(802ABB1C);
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
        ENGINE_BLK(802ABBCC);
        r = 0xFFFF;
    }
    ENGINE_BLK(802ABBD0);
    r = func_802AD7FC(r);
    /* (what it leaves: the second point, the squares, the arctangent) */
    ENGINE_LEAVE(17, ex);
    ENGINE_LEAVE(18, ez);
    ENGINE_LEAVE64(19, (s64)a * a + (s64)b * b);
    ENGINE_LEAVE64(20, (s64)b * b);
    ENGINE_LEAVE(30, r);
    ENGINE_BLK(802ABBD8);
    return (u32)r >> 3;
}

/* D_803EBC10: 16-byte records, the id in byte 12 */
extern u8 D_803EBC10[];

/* the n-th record from the first with this id */
REGS(v0, v1 -> a0)
void *func_802ABC88(s32 id, s32 n) {
    u8 *p = D_803EBC10;

    ENGINE_BLK(802ABC88);
    for (;;) {
        ENGINE_BLK(802ABC9C);
        if (p[0xC] == id)
            break;
        p += 0x10;
    }
    ENGINE_BLK(802ABCA8);
    if (--n != 0) {
        ENGINE_BLK(802ABCB4);
        p += (u32)n * 0x10;
        ENGINE_LEAVE(3, (u32)n * 0x10);      /* ($v1, as it leaves it) */
    } else {
        ENGINE_LEAVE(3, 0);
    }
    ENGINE_BLK(802ABCC8);
    return p;
}

/* the distance between two points, rounded (64 bits) */
REGS(t3, t4, t5, t6, t7, s0 -> s1+f0)
s64 func_802ABCDC(s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2) {
    s32 dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
    s64 r;

    ENGINE_BLK(802ABCDC);
    r = engine_cvt_l_d(__builtin_sqrt((f64)((s64)dx * dx + (s64)dy * dy + (s64)dz * dz)));
    ENGINE_LEAVE_FW(0, (u32)r);         /* (cvt.l.d left it in $f0/$f1 too) */
    ENGINE_LEAVE_FW(1, (u32)(r >> 32));
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
    s32 r, n, amb, at = 0, q;
    s32 seen = 0, s3set = 0, s4set = 0, s5set = 0, atset = 0;
    s32 s3 = 0, s4 = 0, s5 = 0;
    Vehicle *v;

    ENGINE_BLK(802ABD54);
    for (;; p += 0x24) {
        ENGINE_BLK(802ABD70);
        if (p == end) {
            ENGINE_BLK(802ABEA0);
            r = D_80364A6E[0];
            break;
        }
        ENGINE_BLK(802ABD78);
        d = func_802ABCDC(x, y, z, ((s32 *)p)[0], ((s32 *)p)[1], ((s32 *)p)[2]);
        seen = 1;
        ENGINE_LEAVE(17, (s32)d);           /* ($s1: the distance) */
        ENGINE_LEAVE(14, ((s32 *)p)[0]);    /* ($t6, $t7, $s0: the light's position) */
        ENGINE_LEAVE(15, ((s32 *)p)[1]);
        ENGINE_LEAVE(16, ((s32 *)p)[2]);
        ENGINE_BLK(802ABD88);
        r = ((s32 *)p)[3];
        ENGINE_LEAVE(18, r);                /* ($s2: its range) */
        at = (s64)r < d, atset = 1;
        if (at)
            goto next;
        ENGINE_BLK(802ABD98);
        s3 = p[0x12], s3set = 1;
        if (s3 == 0)
            goto next;
        ENGINE_BLK(802ABDA4);
        n = p[0x13];
        t = p + 0x15;
        s3 = n, s4 = (u32)t, s4set = 1;
        for (;;) {
            ENGINE_BLK(802ABDAC);
            if (n == 0)
                goto next;
            ENGINE_BLK(802ABDB4);
            s5 = *t, s5set = 1;
            if (type == *t)
                break;
            ENGINE_BLK(802ABDC0);
            t++, n--;
            s3 = n, s4 = (u32)t;
        }
        ENGINE_BLK(802ABDCC);
        n = p[0x14];
        s3 = n;
        amb = D_80364A6E[0];
        if (n == 0) {
            ENGINE_BLK(802ABDD8);
            at = 1;
            s3 = p[0x10];
            if (p[0x10] == 1) {
                ENGINE_BLK(802ABE34);
                r = 0xFF;
            } else {
                ENGINE_BLK(802ABDE8);
                ENGINE_BLK(802ABE28);
                s5 = amb;
                q = (u32)((0xFF - amb) * (u32)d) / (u32)r;
                ENGINE_LEAVE(17, q);
                ENGINE_LEAVE(18, r);
                r = 0xFF - amb - q + amb;
            }
        } else {
            ENGINE_BLK(802ABE3C);
            at = 1;
            s4 = p[0x10];
            if (p[0x10] == 1) {
                ENGINE_BLK(802ABE90);
                r = n;
            } else {
                ENGINE_BLK(802ABE4C);
                ENGINE_BLK(802ABE88);
                s5 = amb - n;
                q = (u32)((amb - n) * (u32)d) / (u32)r;
                ENGINE_LEAVE(17, q);
                ENGINE_LEAVE(18, r);
                r = n + q;
            }
        }
        break;
    next:
        ENGINE_BLK(802ABE98);
    }
    ENGINE_BLK(802ABEAC);
    for (v = D_80364460;; v++) {
        ENGINE_BLK(802ABEB4);
        if (v->type == type)
            break;
    }
    ENGINE_BLK(802ABEC4);
    v->unk60 = r;
    /* (what it leaves for the vehicle modules, which read on) */
    ENGINE_LEAVE(2, (u32)p);
    ENGINE_LEAVE(3, (u32)end);
    ENGINE_LEAVE(8, (u32)v);
    ENGINE_LEAVE(9, type);
    ENGINE_LEAVE(22, r);
    if (atset)
        ENGINE_LEAVE(1, at);
    if (s3set)
        ENGINE_LEAVE(19, s3);
    if (s4set)
        ENGINE_LEAVE(20, s4);
    if (s5set)
        ENGINE_LEAVE(21, s5);
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

    ENGINE_BLK(802A8314);
    if (base + 10 < now)
        goto done;
    ENGINE_BLK(802A8350);
    p = D_80305C58;
    for (;;) {
        ENGINE_BLK(802A8360);
        if (p[1] == 0)
            break;
        ENGINE_BLK(802A836C);
        if (p[0] != D_802E8BDC) {
            p += 3;
            continue;
        }
        ENGINE_BLK(802A8378);
        p += 3;
        if (p[-1] + base < now)
            continue;
        ENGINE_BLK(802A8390);
        v = p[-2];
        break;
    }
done:
    ENGINE_BLK(802A8394);
    return v;
}

/* the speed (*speed) over the distance from s7[1] to s7[0], stored at
   *ratio; the old ratio when a flag of three is set or the speed is 0 */
REGS(t3, t6, s0, s7, t8 -> f12, t3)
f32 func_802A83B8(s32 t3, s16 *speed, u8 *flags, s32 *pos, f32 *ratio, s32 *t3_out) {
    f32 r;

    ENGINE_BLK(802A83B8);
    /* ($t1: the flag it looked at last, or s7[1]) */
    ENGINE_LEAVE(9, flags[1]);
    if (flags[1] == 1)
        goto old;
    ENGINE_BLK(802A83D4);
    ENGINE_LEAVE(9, flags[2]);
    if (flags[2] == 1)
        goto old;
    ENGINE_BLK(802A83E4);
    ENGINE_LEAVE(9, flags[0]);
    if (flags[0] == 1)
        goto old;
    ENGINE_BLK(802A83F4);
    ENGINE_LEAVE(9, pos[1]);
    t3 = *speed;
    if (t3 == 0)
        goto old;
    ENGINE_BLK(802A8408);
    r = (f32)(pos[0] - pos[1]) / (f32)t3;
    *ratio = r;
    goto done;
old:
    ENGINE_BLK(802A8424);
    r = *ratio;
done:
    ENGINE_BLK(802A8428);
    *t3_out = t3;
    return r;
}

/* of s7[0] - s7[6] and s7[3] - s7[6], the one nearer 0 */
REGS(s7 -> t1)
s32 func_802A8590(s32 *s7) {
    s32 a, b, aa, bb, r;

    ENGINE_BLK(802A8590);
    a = s7[0] - s7[6];
    aa = a;
    if (a < 0) {
        ENGINE_BLK(802A85C4);
        aa = -a;
    }
    ENGINE_BLK(802A85C8);
    b = s7[3] - s7[6];
    bb = b;
    if (b < 0) {
        ENGINE_BLK(802A85D4);
        bb = -b;
    }
    ENGINE_BLK(802A85D8);
    if (bb < aa) {
        ENGINE_BLK(802A85E4);
        r = b;
    } else {
        ENGINE_BLK(802A85EC);
        r = a;
    }
    ENGINE_BLK(802A85F0);
    ENGINE_LEAVE(1, bb < aa);
    ENGINE_LEAVE(10, s7[6]);            /* ($t2, as it leaves it) */
    return r;
}

/* (x, z) moved by the speed (scaled by the turn rate f12) in the direction
   `angle`: *x + ..., *z + ... */
REGS(t4, t6, t7, s1, f12 -> t0, t1)
s32 func_802A860C(s32 angle, s16 *speed, s32 *x, s32 *z, f32 rate, s32 *z_out) {
    s32 v = *speed, rem, s, c, dx, dz, at, rx, rz, px, pz;
    f32 a = __builtin_fabsf(rate), q;

    ENGINE_BLK(802A860C);
    if (v != 0) {
        if (!(a >= 1.0f)) {
            ENGINE_BLK(802A863C);
            ENGINE_BLK(802A8648);
            q = a / 2.0f;
            q = 1.0f - q;
            q = q * (f32)v;
            v = engine_cvt_w_s(q);
        } else {
            ENGINE_BLK(802A863C);
            ENGINE_BLK(802A8670);
            q = a * 2.0f;
            q = (f32)v / q;
            v = engine_cvt_w_s(q);
        }
    }
    ENGINE_BLK(802A8690);
    ENGINE_BLK(802A86A8);
    ENGINE_BLK(802A86C0);
    rem = angle % 0x400;
    s = func_802AE160(rem);
    ENGINE_BLK(802A86D4);
    dx = (s32)(v * s) >> 16;
    c = func_802AE104(rem);
    ENGINE_BLK(802A86EC);
    dz = (s32)(v * c) >> 16;
    px = *x;
    pz = *z;
    at = angle < 0x400;
    if (at) {
        ENGINE_BLK(802A870C);
        rx = px + dx, rz = pz + dz;
    } else {
        ENGINE_BLK(802A8718);
        at = angle < 0x800;
        if (at) {
            ENGINE_BLK(802A8724);
            rx = px + dz, rz = pz - dx;
        } else {
            ENGINE_BLK(802A8730);
            at = angle < 0xC00;
            if (at) {
                ENGINE_BLK(802A873C);
                rx = px - dx, rz = pz - dz;
            } else {
                ENGINE_BLK(802A8748);
                rx = px - dz, rz = pz + dx;
            }
        }
    }
    ENGINE_BLK(802A8750);
    /* (what it leaves for the vehicle modules, which read on) */
    ENGINE_LEAVE(1, at);
    ENGINE_LEAVE(2, 0x400);
    ENGINE_LEAVE(3, rem);
    ENGINE_LEAVE(13, rem);
    ENGINE_LEAVE(19, px);
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

    ENGINE_BLK(802A785C);
    if (D_80370C22 != 0) {
        ENGINE_BLK(802A7874);
        t2 = D_803ED400;
        t4 = *speed;
        if (t2 == 0)
            goto done;
        ENGINE_BLK(802A788C);
        if (t4 > 0) {
            ENGINE_BLK(802A78A8);
            t4 -= brake;
            if (!(t4 > 0)) {
                ENGINE_BLK(802A78B4);
                t4 = 0;
            }
        } else {
            ENGINE_BLK(802A7894);
            t4 += brake;
            if (!(t4 < 0)) {
                ENGINE_BLK(802A78A0);
                t4 = 0;
            }
        }
        ENGINE_BLK(802A78B8);
        *speed = t4;
        goto done;
    }
    ENGINE_BLK(802A78C0);
    if ((s8)D_803ED40C == 0)
        goto gears;
    ENGINE_BLK(802A78D0);
    if (D_802E8BDC == 0x22)
        goto gears;
    ENGINE_BLK(802A78E4);
    if (D_80364456 == 0xB)
        goto gears;
    ENGINE_BLK(802A78F8);
    if (D_80364456 == 0x11)
        goto gears;
    ENGINE_BLK(802A7900);
    if (D_80364456 == 0x12)
        goto gears;
    ENGINE_BLK(802A7908);
    t4 = *speed;
    if (t4 >= 0) {
        ENGINE_BLK(802A7914);
        g = rows[13] >> 1;
        if (!(g < t4))
            goto gears;
        ENGINE_BLK(802A7928);
        *speed = g;
    } else {
        ENGINE_BLK(802A7930);
        g = rows[0] >> 1;
        if (!(t4 < g))
            goto gears;
        ENGINE_BLK(802A7944);
        *speed = g;
    }
gears:
    ENGINE_BLK(802A7948);
    step = func_802A7D68(mode, flags);
    ENGINE_BLK(802A7950);
    t2 = D_80370C2D;
    if (t2 == 0)
        goto done;
    ENGINE_BLK(802A7964);
    if (t2 > 0) {
        ENGINE_BLK(802A79B0);
        t2 = *speed;
        if (t2 < 0) {
            ENGINE_BLK(802A79FC);
            t2 += brake;
            *speed = t2;
            goto done;
        }
        ENGINE_BLK(802A79BC);
        t4 = func_802A7AAC(t2, rows);
        ENGINE_BLK(802A79C4);
        if (t4 != 0)
            goto done;
        ENGINE_BLK(802A79CC);
        t4 = func_802A7C28(t2, rows, vs, &rows2);
        ENGINE_BLK(802A79D4);
        if (t4 != 0) {
            ENGINE_BLK(802A79DC);
            t2 += t4 * step;
        } else {
            ENGINE_BLK(802A79F0);
            t2 -= 6;
        }
        *speed = t2;
        goto done;
    }
    ENGINE_BLK(802A796C);
    t2 = *speed;
    if (t2 > 0) {
        ENGINE_BLK(802A79A4);
        t2 -= brake;
        *speed = t2;
        goto done;
    }
    ENGINE_BLK(802A7978);
    t4 = func_802A7A1C(t2, rows);
    ENGINE_BLK(802A7980);
    if (t4 != 0)
        goto done;
    ENGINE_BLK(802A7988);
    t4 = func_802A7C28(t2, rows, vs, &rows2);
    ENGINE_BLK(802A7990);
    t2 -= t4 * step;
    *speed = t2;
done:
    ENGINE_BLK(802A7A04);
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

    ENGINE_BLK(802A7834);
    *turn_out = func_802A7E70(rate, h, &sa, &st);
    ENGINE_BLK(802A7844);
    r = func_802A785C(step, speed, mode, flags, rows, brake, vs, step_out);
    ENGINE_BLK(802A784C);
    return r;
}

/* the speed for a vehicle without gears: toward 0 by `brake` with
   D_80370C22 and D_803ED400 set, otherwise accelerated through the gear
   rows on D_80370C1A/D_80370C1B (the buttons) */
REGS(t6, s1, s2, gp)
void func_802A7B3C(s16 *speed, s16 *rows, s32 brake, VS *vs) {
    s32 t2, t4;
    u32 rows2;

    ENGINE_BLK(802A7B3C);
    if (D_80370C22 != 0) {
        ENGINE_BLK(802A7B54);
        t2 = D_803ED400;
        t4 = *speed;
        if (t2 == 0)
            goto done;
        ENGINE_BLK(802A7B6C);
        if (t2 > 0) {
            ENGINE_BLK(802A7B88);
            t4 -= brake;
            if (!(t4 > 0)) {
                ENGINE_BLK(802A7B94);
                t4 = 0;
            }
        } else {
            ENGINE_BLK(802A7B74);
            t4 += brake;
            if (!(t4 < 0)) {
                ENGINE_BLK(802A7B80);
                t4 = 0;
            }
        }
        ENGINE_BLK(802A7B98);
        *speed = t4;
        goto done;
    }
    ENGINE_BLK(802A7BA0);
    if (D_80370C1A == 0) {
        ENGINE_BLK(802A7BB4);
        if (D_80370C1B == 0)
            goto done;
    }
    ENGINE_BLK(802A7BC8);
    t2 = *speed;
    if (t2 < 0) {
        ENGINE_BLK(802A7C08);
        *speed = t2 + brake;
        goto done;
    }
    ENGINE_BLK(802A7BD4);
    t4 = func_802A7C28(t2, rows, vs, &rows2);
    ENGINE_BLK(802A7BDC);
    if (t4 != 0) {
        ENGINE_BLK(802A7BE4);
        *speed = t2 + t4 * 4;
    } else {
        ENGINE_BLK(802A7BFC);
        *speed = t2 - 6;
    }
done:
    ENGINE_BLK(802A7C10);
    D_803ED400 = *speed;
}

extern f32 D_803EBBF4;
extern u8 D_803BE738;
extern u8 D_803ED3F7, D_803ED3EE, D_803ED3EF, D_803ED3F2, D_803ED3F3, D_803ED3F4;
extern u8 D_803ED3EA, D_803ED3EB, D_803ED3EC, D_803ED410;
extern s32 D_803ED398, D_803ED39C, D_803ED3A0, D_803ED3A8, D_803ED3AC, D_803ED3B0;

/* D_803EBBF4 tripled when D_803ED3F5 */
void func_802A8FB4(void) {
    ENGINE_BLK(802A8FB4);
    if (D_803ED3F5 != 0) {
        ENGINE_BLK(802A8FCC);
        D_803EBBF4 = D_803EBBF4 * 3.0f;
    }
    ENGINE_BLK(802A8FE4);
}

/* D_803ED3F7 = 8 when the state's unk9F is 0x67 on level 12 */
REGS(gp)
void func_802A8FF4(VS *vs) {
    ENGINE_BLK(802A8FF4);
    if (vs->unk9F == 0x67) {
        ENGINE_BLK(802A9008);
        if (D_802E8BDC == 0xC) {
            ENGINE_BLK(802A901C);
            D_803ED3F7 = 8;
        }
    }
    ENGINE_BLK(802A9028);
}

/* the state's previous values set from its current ones */
REGS(gp)
void func_802A9038(VS *vs) {
    u8 b;
    s32 w;

    ENGINE_BLK(802A9038);
    b = D_803ED3EE;
    D_803ED3EF = b;
    *(u8 *)((u32)&D_803ED3EF + 1) = b;   /* (the byte after it, which has no name) */
    w = D_803ED398;
    D_803ED39C = w;
    D_803ED3A0 = w;
    w = D_803ED3A8;
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
    ENGINE_BLK(802A90E4);
    if (g[0] != 0)
        goto no;
    ENGINE_BLK(802A90FC);
    if (g[1] != 0)
        goto no;
    ENGINE_BLK(802A9108);
    if (g[2] != 0)
        goto no;
    ENGINE_BLK(802A9114);
    if (g[3] != 0)
        goto no;
    ENGINE_BLK(802A9120);
    if (g[4] != 0)
        goto no;
    ENGINE_BLK(802A912C);
    if (g[5] != 0)
        goto no;
    ENGINE_BLK(802A9138);
    D_803ED410 = 1;
    goto done;
no:
    ENGINE_BLK(802A9148);
    D_803ED410 = 0;
done:
    ENGINE_BLK(802A9150);
}

/* the gears (unk50): the average of the three gearboxes' (D_803ED3F2[3],
   100 and up counting as 2); unk9F and D_803ED40D: the highest; and
   D_803BE738 set on 0x66, or 0x64 with D_80358064, unless the first flag */
REGS(s0, t8, gp)
void func_802A9164(u8 *flags, s32 type, VS *vs) {

    s32 a, b, m, t2;

    ENGINE_BLK(802A9164);
    a = D_803ED3F2;
    if (!(a < 100)) {
        ENGINE_BLK(802A9184);
        a = 2;
    }
    ENGINE_BLK(802A9188);
    b = D_803ED3F3;
    if (!(b < 100)) {
        ENGINE_BLK(802A9198);
        b = 2;
    }
    ENGINE_BLK(802A919C);
    a += b;
    b = D_803ED3F4;
    if (!(b < 100)) {
        ENGINE_BLK(802A91B0);
        b = 2;
    }
    ENGINE_BLK(802A91B4);
    a += b;
    vs->unk50 = (u32)a / 3;
    ENGINE_BLK(802A91DC);
    m = 0;
    if (m < D_803ED3F2) {
        ENGINE_BLK(802A91EC);
        m = D_803ED3F2;
    }
    ENGINE_BLK(802A91F0);
    if (m < D_803ED3F3) {
        ENGINE_BLK(802A9200);
        m = D_803ED3F3;
    }
    ENGINE_BLK(802A9204);
    if (m < D_803ED3F4) {
        ENGINE_BLK(802A9214);
        m = D_803ED3F4;
    }
    ENGINE_BLK(802A9218);
    vs->unk9F = m;
    D_803ED40D = m;
    t2 = D_803ED40D;
    if (D_803ED40D == 0x66) {
        ENGINE_BLK(802A9238);
        t2 = flags[0];
        if (flags[0] != 1) {
            ENGINE_BLK(802A9248);
            D_803BE738 = t2 = 1;
        }
    }
    ENGINE_BLK(802A9254);
    if (type == 9)
        goto done;
    ENGINE_BLK(802A9260);
    if (type == 0xB)
        goto done;
    ENGINE_BLK(802A9268);
    if (type == 0x11)
        goto done;
    ENGINE_BLK(802A9270);
    if (type == 0x12)
        goto done;
    ENGINE_BLK(802A9278);
    t2 = D_80358064;
    if (D_80358064 == 0)
        goto done;
    ENGINE_BLK(802A9288);
    t2 = D_803ED40D;
    if (D_803ED40D != 0x64)
        goto done;
    ENGINE_BLK(802A929C);
    t2 = flags[0];
    if (flags[0] == 1)
        goto done;
    ENGINE_BLK(802A92AC);
    D_803BE738 = t2 = 1;
done:
    ENGINE_BLK(802A92B8);
    ENGINE_LEAVE(10, t2);               /* ($t2, as it leaves it) */
}

REGS(a0, a2, a3 -> a1, a3, t1)
s64 func_802ACE38(s64 x, s64 z, s32 angle, s32 *xr, s32 *zr);

/* point `i` of a table of (x, z) s16 pairs rotated by the angle *a */
REGS(v0, v1, s4 -> t5, t6)
s32 func_802A94A4(s32 i, s16 *pts, s16 *a, s32 *z_out) {
    s32 xr, zr;

    ENGINE_BLK(802A94A4);
    func_802ACE38(pts[2 * i], pts[2 * i + 1], *a, &xr, &zr);
    ENGINE_BLK(802A94E4);
    *z_out = zr;
    return xr;
}

/* clamped to 0x240 when positive */
REGS(s3 -> s3)
s32 func_802A9514(s32 v) {
    ENGINE_BLK(802A9514);
    if (v >= 0) {
        ENGINE_BLK(802A9520);
        if (!(v < 0x241)) {
            ENGINE_BLK(802A952C);
            v = 0x240;
        }
    }
    ENGINE_BLK(802A9530);
    return v;
}

/* wheel i on the ground: its height (clamped), 2 and t2 in the three
   arrays, and its flag in D_803ED3EE[] */
REGS(v0, a1, a2, a3, t2, s3)
void func_802A9540(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 v) {
    ENGINE_BLK(802A9540);
    v = func_802A9514(v);
    ENGINE_BLK(802A9558);
    h[i] = v;
    state[i] = 2;
    ground[i] = g;
    *(u8 *)((u32)&D_803ED3EE + i) = 1;
    /* (what it leaves: $s3, $s0, $t3, $t7) */
    ENGINE_LEAVE(19, v);
    ENGINE_LEAVE(16, 1);
    ENGINE_LEAVE(11, i << 2);
    ENGINE_LEAVE(15, (u32)&D_803ED3EE + i);
}
