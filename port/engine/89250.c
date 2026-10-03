/*
 * hd_code 89250 (us.v11 0x802CDA10-0x802CE83C): the collision tests the
 * game's C makes against the buildings and the level's other objects, as
 * native C (engine.h, buildings.h).  The player's position is
 * D_803A73F0..F8 (<< 5); D_803A7410 and D_803A7412 are the camera's limits
 * (12-bit headings), which a hit narrows.
 */
#include "buildings.h"
#include "game/game.h"

extern u8 D_80306450[];
extern u16 D_803F932A;                          /* the damage a hit does */
extern u8 D_803F932C;                           /* the object under the last ground test */
extern u8 D_803F932D, D_803F932E;               /* a hit on a crate, on the player's own kind */
extern s32 D_803F9320, D_803F9324;              /* a pushed object's position */
extern s16 D_803F9328;                          /* and its speed */
extern u8 D_802E8BE4;
extern s32 D_802E8BE8;
extern u8 D_802C382C[];
extern u8 D_80364456;                           /* the player's vehicle type */
extern u32 D_803649E8;


/* 60F60: start an effect */
REGS(t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, a3 -> t0)
s32 func_802A6274(s32 t0, s32 t1, s32 t2, s32 t3, s32 t4, s32 t5, s32 t6, s32 t7, s32 s0, s32 s1, s32 s2,
                  s32 s3, s32 s4, s32 s5, s32 a3);
/* 62740 (native) */
REGS(t0, t1, t2, fp -> t3, fp)
s32 func_802A9DC0(s32 x, s32 z, s32 y, s32 mat, s32 *mat_out);
REGS(t0, t1, t2, t8 -> t3, t6)
s32 func_802A9F24(s32 x, s32 z, s32 y, s32 self, s32 *id_out);
REGS(t0, t1, t2, t3, fp -> a1, t3, fp)
s32 func_802AA094(s32 x, s32 z, s32 y, s32 h, s32 mat, s32 *h_out, s32 *mat_out);

REGS(t3, v0, v1, a0, a1, fp -> fp)
s32 func_802CDC7C(Building *b, s32 x, s32 y, s32 z, s32 r, s32 hit);
REGS(t9, s0, fp)
void func_802CDD74(Building *b, Piece *p, s32 damage);
REGS(v0, v1, a0, a1, t4 -> s4)
s32 func_802CE0E4(s32 x, s32 y, s32 z, s32 r, s32 kind);
REGS(v0, a0, a2, t0)
void func_802CE204(s32 x, s32 z, s32 tx, s32 tz);

/* the effect of a crate blowing up at (x, y, z) */
void func_802CDA10(s32 x, s32 y, s32 z) {
    ENGINE_BLK(802CDA10);
    func_802A6274((s32)D_802C382C, 0x591C8, 0, x << 11, y << 11, z << 11, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ENGINE_BLK(802CDAB8);
}

/* func_802C18D4 at the player's position */
void func_802CDAE8(s16 a, s16 b) {
    ENGINE_BLK(802CDAE8);
    func_802C18D4(b, D_803A73F0 << 11, D_803A73F4 << 11, D_803A73F8 << 11, a);
    ENGINE_BLK(802CDB40);
}

/* the camera's limits off, if D_803A742F says (block id/size `blk`) */
#define LIMITS_OFF(blk)                                                     \
    do {                                                                    \
        if (D_803A742F != 0) {                                              \
            ENGINE_BLK(blk);                                                \
            D_803A7410 = 0;                                                 \
            D_803A7412 = 0;                                                 \
        }                                                                   \
    } while (0)

/* The buildings the player's sphere (radius r) touches, each piece of them
   it hits doing `damage` (to the first); returns whether any. */
s32 func_802CDB70(s16 r, s16 damage) {
    s32 x = D_803A73F0, y = D_803A73F4, z = D_803A73F8;
    s32 hit = 0;
    Building *b, *end;

    ENGINE_BLK(802CDB70);
    D_803F932A = damage;
    b = D_803F4030;
    end = D_803F7654;
    for (;;) {
        ENGINE_BLK(802CDBD8);
        if (b == end)
            break;
        ENGINE_BLK(802CDBE0);
        if (func_8029CFA4(x, y, z, r, b->x, b->y, b->z, b->unkC)) {
            ENGINE_BLK(802CDBF4);
            ENGINE_BLK(802CDBFC);
            if (b->unk30 == 0x38) {
                ENGINE_BLK(802CDC0C);
                if (func_802BD8C8()) {
                    ENGINE_BLK(802CDC14);
                    goto next;
                }
                ENGINE_BLK(802CDC14);
            }
            ENGINE_BLK(802CDC1C);
            hit = func_802CDC7C(b, x, y, z, r, hit);
        } else {
            ENGINE_BLK(802CDBF4);
        }
    next:
        ENGINE_BLK(802CDC24);
        b++;
    }
    ENGINE_BLK(802CDC2C);
    LIMITS_OFF(802CDC40);
    ENGINE_BLK(802CDC4C);
    return hit;
}

/* the pieces of building b the sphere hits: each turns the camera
   (func_802BF264), and the first does the damage (D_803F932A) unless
   `hit` was already set; returns whether any was hit (or `hit`) */
REGS(t3, v0, v1, a0, a1, fp -> fp)
s32 func_802CDC7C(Building *b, s32 x, s32 y, s32 z, s32 r, s32 hit) {
    static const PieceTestBlks k = {
        {ENGINE_BLK_802CDCD8}, {ENGINE_BLK_802CDCE0}, {ENGINE_BLK_802CDCE8}, {ENGINE_BLK_802CDCF0},
        {ENGINE_BLK_802CDCF8}, {ENGINE_BLK_802CDD00}, {ENGINE_BLK_802CDD08}, {ENGINE_BLK_802CDD10},
    };
    Piece *p;

    ENGINE_BLK(802CDC7C);
    x >>= 2;
    y >>= 2;
    z >>= 2;
    r >>= 2;
    for (p = b->unk4; ENGINE_BLK(802CDCBC), p != (Piece *)b->unk8; p++) {
        ENGINE_BLK(802CDCC4);
        if (p->active == 0)
            continue;
        ENGINE_BLK(802CDCD0);
        if (piece_touched(p, x, y, z, r, &k)) {
            ENGINE_BLK(802CDD18);
            if (hit == 0) {
                ENGINE_BLK(802CDD20);
                hit = D_803F932A;
                if (hit != 0) {
                    ENGINE_BLK(802CDD30);
                    func_802CDD74(b, p, hit);
                }
            }
            ENGINE_BLK(802CDD38);
            func_802BF264(p);
            ENGINE_BLK(802CDD40);
            hit = 1;
        }
        ENGINE_BLK(802CDD44);
    }
    ENGINE_BLK(802CDD4C);
    return hit;
}

/* piece p of building b takes `damage` (by the model's strength, byte 4):
   its group's damage goes up to at most 100, and at 100 the group goes */
REGS(t9, s0, fp)
void func_802CDD74(Building *b, Piece *p, s32 damage) {
    s32 g, d;
    u8 *dmg;
    Piece *q, *end;

    ENGINE_BLK(802CDD74);
    D_802E8BE4 = 10;
    D_802E8BE8 = 200;
    if (b->unk30 == 0x38)
        goto out;
    ENGINE_BLK(802CDE24);
    g = p->group;
    if (B_MODEL(b)[4] == 0) {
        { ENGINE_BLK(802CDE40); engine_break(0x802CDE40, 7); }
    }
    ENGINE_BLK(802CDE44);
    dmg = &B_DAMAGE(b)[g - 1];
    d = *dmg + (s32)engine_divu(damage, B_MODEL(b)[4]);
    if (!(d < 100)) {
        ENGINE_BLK(802CDE68);
        d = 100;
    }
    ENGINE_BLK(802CDE6C);
    *dmg = d;
    func_802BF898(g, d, b);
    ENGINE_BLK(802CDE74);
    if (d != 100)
        goto out;
    ENGINE_BLK(802CDE80);
    func_802BF1F0(g, b);
    ENGINE_BLK(802CDE88);
    func_802C1438(g, b);
    ENGINE_BLK(802CDE90);
    func_802C09B8(g, b);
    ENGINE_BLK(802CDE98);
    func_802C0E8C(g, b);
    ENGINE_BLK(802CDEA0);
    func_802BF384(b);
    ENGINE_BLK(802CDEA8);
    q = b->unk4;
    end = b->unk8;
    for (;;) {
        ENGINE_BLK(802CDEB0);
        if (q == end)
            break;
        ENGINE_BLK(802CDEB8);
        if (q->group == g) {
            ENGINE_BLK(802CDEC4);
            q->active = 0;
        }
        ENGINE_BLK(802CDEC8);
        if (q->group2 == g) {
            ENGINE_BLK(802CDED4);
            if (B_DAMAGE(b)[q->group - 1] != 100) {
                ENGINE_BLK(802CDEF0);
                q->active = 1;
            }
        }
        ENGINE_BLK(802CDEF8);
        q++;
    }
    ENGINE_BLK(802CDF00);
    func_802BF668(b);
    ENGINE_BLK(802CDF08);
    func_802BF534(b);
out:
    ENGINE_BLK(802CDF10);
}

/* The level's solid objects the player's sphere (radius r) touches, and
   their kinds' parts (func_802CE0E4): returns how many kinds were hit. */
u8 func_802CDF94(s16 r) {
    s32 x = D_803A73F0, y = D_803A73F4, z = D_803A73F8;
    s32 n = 0, kind;
    Solid *s;

    ENGINE_BLK(802CDF94);
    D_803F932D = 0;
    D_803F932E = 0;
    s = D_803A7300;
    for (;;) {
        ENGINE_BLK(802CDFFC);
        if (s->end == -1)
            break;
        ENGINE_BLK(802CE008);
        kind = s->kind;
        if (kind == 0) {
            ENGINE_BLK(802CE014);
            if (D_803649E8 != 0) {
                s++;
                continue;
            }
        }
        ENGINE_BLK(802CE024);
        s++;
        if (func_8029CFA4(x, y, z, r, s[-1].x, s[-1].y, s[-1].z, s[-1].r)) {
            ENGINE_BLK(802CE03C);
            ENGINE_BLK(802CE044);
            n += func_802CE0E4(x, y, z, r, kind);
            ENGINE_BLK(802CE04C);
        } else {
            ENGINE_BLK(802CE03C);
        }
    }
    ENGINE_BLK(802CE054);
    if (D_803A7410 != 0) {
        goto narrowed;
    }
    ENGINE_BLK(802CE068);
    if (D_803A7412 != 0xFFF) {
    narrowed:
        ENGINE_BLK(802CE080);
        D_803A7425 = 1;
        D_803A7424 = 1;
    }
    ENGINE_BLK(802CE094);
    if (D_803A742F != 0) {
        ENGINE_BLK(802CE0A8);
        D_803A7410 = 0;
        D_803A7412 = 0;
    }
    ENGINE_BLK(802CE0B4);
    return n;
}

/* the parts of kind `kind` the sphere touches: each narrows the camera
   (func_802CE204); returns whether any did */
REGS(v0, v1, a0, a1, t4 -> s4)
s32 func_802CE0E4(s32 x, s32 y, s32 z, s32 r, s32 kind) {
    KindPart *k = D_803A6B30;
    s32 any = 0;

    ENGINE_BLK(802CE0E4);
    /* the kind's first part */
    for (;;) {
        ENGINE_BLK(802CE128);
        if (k->end == -1)
            goto out;
        ENGINE_BLK(802CE134);
        if (k->kind == kind)
            break;
        k++;
    }
    for (;;) {
        ENGINE_BLK(802CE140);
        if (k->end == -1)
            break;
        ENGINE_BLK(802CE14C);
        if (k->kind != kind)
            break;
        ENGINE_BLK(802CE158);
        k++;
        if (kind == 6) {
            ENGINE_BLK(802CE170);
            if (k[-1].r == 0x3BD)
                continue;
        }
        ENGINE_BLK(802CE17C);
        if (!func_8029CFA4(x, y, z, r, k[-1].x, k[-1].y, k[-1].z, k[-1].r)) {
            ENGINE_BLK(802CE184);
            continue;
        }
        ENGINE_BLK(802CE184);
        ENGINE_BLK(802CE18C);
        if (D_80364456 == kind) {
            ENGINE_BLK(802CE19C);
            D_803F932E = 1;
        }
        ENGINE_BLK(802CE1A8);
        any = 1;
        if (kind == 0xFF) {
            ENGINE_BLK(802CE1B4);
            D_803F932D = 1;
        }
        ENGINE_BLK(802CE1BC);
        func_802CE204(x, z, k[-1].x, k[-1].z);
        ENGINE_BLK(802CE1C4);
    }
out:
    ENGINE_BLK(802CE1CC);
    return any;
}

/* the heading from (x, z) to (tx, tz), 16.16 asin quadrant by quadrant,
   and the camera kept within a quarter turn of it */
REGS(v0, a0, a2, t0)
void func_802CE204(s32 x, s32 z, s32 tx, s32 tz) {
    s32 dx = tx - x, dz = tz - z, h = 0, v;
    f32 d, q;

    ENGINE_BLK(802CE204);
    if (dx == 0) {
        ENGINE_BLK(802CE244);
        if (dz == 0)
            goto out;
    }
    ENGINE_BLK(802CE24C);
    d = (f32)dx * (f32)dx;
    q = (f32)dz * (f32)dz;
    d = __builtin_sqrtf(d + q);
    if (!(tx < x)) {
        ENGINE_BLK(802CE278);
        if (!(tz < z)) {
            ENGINE_BLK(802CE284);
            q = (f32)(tx - x) / d;
            v = engine_cvt_w_s(65536.0f * q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802CE2B4);
            h = (u32)h >> 4;
        } else {
            ENGINE_BLK(802CE2BC);
            q = (f32)(z - tz) / d;
            v = engine_cvt_w_s(65536.0f * q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802CE2EC);
            h = ((u32)h >> 4) + 0x400;
        }
    } else {
        ENGINE_BLK(802CE2F8);
        if (tz < z) {
            ENGINE_BLK(802CE304);
            q = (f32)(x - tx) / d;
            v = engine_cvt_w_s(65536.0f * q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802CE334);
            h = ((u32)h >> 4) + 0x800;
        } else {
            ENGINE_BLK(802CE340);
            q = (f32)(tz - z) / d;
            v = engine_cvt_w_s(65536.0f * q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802CE370);
            h = ((u32)h >> 4) + 0xC00;
        }
    }
out:
    ENGINE_BLK(802CE378);
    func_8029B7CC(h + 0x400, h - 0x400);
    ENGINE_BLK(802CE384);
}

/* a pushed object's heading h kept off the camera's limits: moved 0x78
   past the nearer one, or to their middle */
s16 func_802CE3B8(s16 h) {
    s32 lo = D_803A7410, hi = D_803A7412, d1, d2, a;

    ENGINE_BLK(802CE3B8);
    d1 = lo - h;
    if (d1 < 0) {
        ENGINE_BLK(802CE400);
        d1 = -d1;
    }
    ENGINE_BLK(802CE404);
    if (!(d1 < 0x801)) {
        ENGINE_BLK(802CE410);
        d1 = 0xFFF - d1;
    }
    ENGINE_BLK(802CE418);
    d2 = hi - h;
    if (d2 < 0) {
        ENGINE_BLK(802CE424);
        d2 = -d2;
    }
    ENGINE_BLK(802CE428);
    if (!(d2 < 0x801)) {
        ENGINE_BLK(802CE434);
        d2 = 0xFFF - d2;
    }
    ENGINE_BLK(802CE43C);
    if (d2 < d1) {
        ENGINE_BLK(802CE448);
        a = hi - 0x78;
        if (a < 0) {
            ENGINE_BLK(802CE454);
            a += 0xFFF;
        }
    } else {
        ENGINE_BLK(802CE45C);
        a = lo + 0x78;
        if (!(a < 0x1000)) {
            ENGINE_BLK(802CE46C);
            a -= 0xFFF;
        }
    }
    ENGINE_BLK(802CE470);
    if (hi < lo) {
        ENGINE_BLK(802CE47C);
        if (!(a < lo))
            goto keep;
        ENGINE_BLK(802CE484);
        if (!(hi < a))
            goto keep;
        ENGINE_BLK(802CE48C);
    } else {
        ENGINE_BLK(802CE494);
        if (!(a < lo)) {
            ENGINE_BLK(802CE4A0);
            if (!(hi < a)) {
                ENGINE_BLK(802CE4A8);
                goto keep;
            }
        }
    }
    ENGINE_BLK(802CE4B0);
    a = func_802A6F6C();
    ENGINE_BLK(802CE4B8);
keep:
    ENGINE_BLK(802CE4BC);
    return a;
}

/* the player's position for the collision tests, and the camera's limits
   open */
void func_802CE4F0(s32 x, s32 y, s32 z) {
    ENGINE_BLK(802CE4F0);
    D_803A7410 = 0;
    D_803A7412 = 0xFFF;
    D_803A73F0 = x;
    D_803A73F4 = y;
    D_803A73F8 = z;
    D_803A742F = 0;
    D_803A7427 = 0;
    D_803A7408 = (s8 *)D_80306450;
    D_803A7424 = 0;
    D_803A7425 = 0;
    func_802BCBD8();
    ENGINE_BLK(802CE58C);
}

/* an object's collision against the buildings (func_8029B02C) */
void func_802CE5BC(s32 x, s32 y, s32 z, s16 r, s32 t8, s32 flag) {
    ENGINE_BLK(802CE5BC);
    D_803A742A = flag;
    func_8029B02C(y, z, r, x >> 2, y >> 2, z >> 2, r >> 2, t8, 0);
    ENGINE_BLK(802CE610);
    LIMITS_OFF(802CE620);
    ENGINE_BLK(802CE62C);
}

/* a pushed object at (x, z) moved by its speed in direction h */
void func_802CE65C(s32 x, s32 z, s16 speed, s16 h) {
    s32 nz;

    ENGINE_BLK(802CE65C);
    D_803F9320 = x;
    D_803F9324 = z;
    D_803F9328 = speed;
    x = func_802A860C(h, &D_803F9328, &D_803F9320, &D_803F9324, 0.0f, &nz);
    ENGINE_BLK(802CE6B8);
    D_803F9320 = x;
    D_803F9324 = nz;
}

/* The ground under an object at (x, z), from height y: the nearest of the
   static triangles, the moving objects' and the level grid's (as
   func_802A9B1C for a wheel); the object under it in D_803F932C; 0 or
   more. */
s32 func_802CE6F8(s32 x, s32 z, s32 y) {
    s32 s0, s1, t3, t4, t5, t6, t7, s2, fp, a1;

    ENGINE_BLK(802CE6F8);
    y += 0x78;
    s0 = func_802A9DC0(x, z, y, 0, &s1);
    ENGINE_BLK(802CE738);
    t4 = func_802A9F24(x, z, y, 0, &t6);
    ENGINE_BLK(802CE744);
    a1 = func_802AA094(x, z, y, t4, s1, &t3, &fp);
    ENGINE_BLK(802CE74C);
    if (a1 == 0) {
        ENGINE_BLK(802CE754);
        if (s0 == 99999999) {
            ENGINE_BLK(802CE760);
            if (t4 == 99999999) {
                ENGINE_BLK(802CE76C);
                t3 = y - 0x78;
                goto none;
            }
        }
        ENGINE_BLK(802CE774);
        t7 = t4 - y;
        s2 = s0 - y;
        if (t7 < 0) {
            ENGINE_BLK(802CE784);
            t7 = -t7;
        }
        ENGINE_BLK(802CE788);
        if (s2 < 0) {
            ENGINE_BLK(802CE790);
            s2 = -s2;
        }
        ENGINE_BLK(802CE794);
    } else {
        ENGINE_BLK(802CE79C);
        t7 = t4 - y;
        t5 = t3 - y;
        if (t7 < 0) {
            ENGINE_BLK(802CE7AC);
            t7 = -t7;
        }
        ENGINE_BLK(802CE7B0);
        if (t5 < 0) {
            ENGINE_BLK(802CE7B8);
            t5 = -t5;
        }
        ENGINE_BLK(802CE7BC);
        s2 = s0 - y;
        if (s2 < 0) {
            ENGINE_BLK(802CE7C8);
            s2 = -s2;
        }
        ENGINE_BLK(802CE7CC);
        if (!(t7 < t5)) {
            ENGINE_BLK(802CE7D8);
            if (!(s2 < t5))
                goto none;
        }
    }
    ENGINE_BLK(802CE7E0);
    if (t7 < s2) {
        ENGINE_BLK(802CE7E8);
        t3 = t4;
        goto store;
    }
    ENGINE_BLK(802CE7F0);
    t3 = s0;
none:
    ENGINE_BLK(802CE7F4);
    t6 = 0;
store:
    ENGINE_BLK(802CE7F8);
    D_803F932C = t6;
    if (t3 < 0) {
        ENGINE_BLK(802CE808);
        t3 = 0;
    }
    ENGINE_BLK(802CE80C);
    (void)fp;
    return t3;
}
