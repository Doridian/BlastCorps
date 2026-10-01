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
#include "engine.h"
#include "game/vehicle.h"

typedef VehicleState VS;

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
    s32 h = vs->unk4C, target = vs->unk4E, n;

    ENGINE_BLK(802A746C);
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
    ENGINE_BLK(802A74A4);
    if (turn >= 0) {
        ENGINE_BLK(802A74AC);
        if (h < target) {
            ENGINE_BLK(802A74B4);
            if (target < n)
                goto clamp;
            ENGINE_BLK(802A74BC);
            if (n < h)
                goto clamp;
            ENGINE_BLK(802A74C4);
            goto set;
        }
        ENGINE_BLK(802A74CC);
        if (!(target < h))
            goto set;
        ENGINE_BLK(802A74D8);
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
        if (!(target < n))
            goto clamp;
        ENGINE_BLK(802A7504);
        if (!(n < h))
            goto clamp;
        ENGINE_BLK(802A750C);
        goto set;
    }
    ENGINE_BLK(802A7514);
    if (!(h < target))
        goto set;
    ENGINE_BLK(802A7520);
    if (!(n < target))
        goto set;
    ENGINE_BLK(802A7528);
    if (!(h < n))
        goto set;
clamp:
    ENGINE_BLK(802A7530);
    vs->unk4C = target;
    goto done;
set:
    ENGINE_BLK(802A7538);
    vs->unk4C = n;
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
