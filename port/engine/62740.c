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
            if (!(limit < v)) {
                ENGINE_BLK(802A7038);
                v = limit;
            }
        } else {
            ENGINE_BLK(802A7040);
            v += 3;
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
    if (x < lox)
        goto out;
    ENGINE_BLK(802AA690);
    if (hix < x)
        goto out;
    ENGINE_BLK(802AA698);
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
    s32 r, n, amb;
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
        ENGINE_BLK(802ABD88);
        r = ((s32 *)p)[3];
        if ((s64)r < d)
            goto next;
        ENGINE_BLK(802ABD98);
        if (p[0x12] == 0)
            goto next;
        ENGINE_BLK(802ABDA4);
        n = p[0x13];
        t = p + 0x15;
        for (;;) {
            ENGINE_BLK(802ABDAC);
            if (n == 0)
                goto next;
            ENGINE_BLK(802ABDB4);
            if (type == *t)
                break;
            ENGINE_BLK(802ABDC0);
            t++, n--;
        }
        ENGINE_BLK(802ABDCC);
        n = p[0x14];
        amb = D_80364A6E[0];
        if (n == 0) {
            ENGINE_BLK(802ABDD8);
            if (p[0x10] == 1) {
                ENGINE_BLK(802ABE34);
                r = 0xFF;
            } else {
                ENGINE_BLK(802ABDE8);
                ENGINE_BLK(802ABE28);
                r = 0xFF - amb - (u32)((0xFF - amb) * (u32)d) / (u32)r + amb;
            }
        } else {
            ENGINE_BLK(802ABE3C);
            if (p[0x10] == 1) {
                ENGINE_BLK(802ABE90);
                r = n;
            } else {
                ENGINE_BLK(802ABE4C);
                ENGINE_BLK(802ABE88);
                r = n + (u32)((amb - n) * (u32)d) / (u32)r;
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
}
