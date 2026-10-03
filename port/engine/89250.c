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
    s32 hit = 0, t;
    Building *b;

    ENGINE_BLK(802CDB70);
    D_803F932A = damage;
    for (b = D_803F4030; ENGINE_BLK(802CDBD8), b != D_803F7654; b++) {
        ENGINE_BLK(802CDBE0);
        t = func_8029CFA4(x, y, z, r, b->x, b->y, b->z, B_RADIUS(b));
        ENGINE_BLK(802CDBF4);
        if (t) {
            ENGINE_BLK(802CDBFC);
            if (B_ID(b) == MODEL_GOAL) {
                ENGINE_BLK(802CDC0C);
                t = func_802BD8C8();
                ENGINE_BLK(802CDC14);
                if (t)
                    goto next;
            }
            ENGINE_BLK(802CDC1C);
            hit = func_802CDC7C(b, x, y, z, r, hit);
        }
    next:
        ENGINE_BLK(802CDC24);
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

/* piece p of building b takes `damage` (over the model's strength): its
   group's damage goes up to at most 100, and at 100 the group goes */
REGS(t9, s0, fp)
void func_802CDD74(Building *b, Piece *p, s32 damage) {
    s32 g, d;
    Piece *q;

    ENGINE_BLK(802CDD74);
    D_802E8BE4 = SHAKE_HIT_FRAMES;
    D_802E8BE8 = SHAKE_PLAYER_HIT;
    if (B_ID(b) == MODEL_GOAL)
        goto out;
    ENGINE_BLK(802CDE24);
    g = p->group;
    if (M_STRENGTH(B_MODEL(b)) == 0) {
        ENGINE_BLK(802CDE40);
        engine_break(0x802CDE40, 7);
    }
    ENGINE_BLK(802CDE44);
    d = B_DAMAGE(b)[g - 1] + (s32)engine_divu(damage, M_STRENGTH(B_MODEL(b)));
    if (d >= 100) {
        ENGINE_BLK(802CDE68);
        d = 100;
    }
    ENGINE_BLK(802CDE6C);
    B_DAMAGE(b)[g - 1] = d;
    func_802BF898(g, d, b);
    ENGINE_BLK(802CDE74);
    if (d != 100)
        goto out;
    /* the group destroyed (as 77E20's func_802BEBB0 does it, without the
       shake) */
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
    for (q = b->unk4; ENGINE_BLK(802CDEB0), q != (Piece *)b->unk8; q++) {
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
    s32 n = 0, kind, t;
    Solid *s;

    ENGINE_BLK(802CDF94);
    D_803F932D = 0;
    D_803F932E = 0;
    for (s = D_803A7300; ENGINE_BLK(802CDFFC), s->end != -1; s++) {
        ENGINE_BLK(802CE008);
        kind = s->kind;
        if (kind == 0) {
            /* (kind 0 not while D_803649E8) */
            ENGINE_BLK(802CE014);
            if (D_803649E8 != 0)
                continue;
        }
        ENGINE_BLK(802CE024);
        t = func_8029CFA4(x, y, z, r, s->x, s->y, s->z, s->r);
        ENGINE_BLK(802CE03C);
        if (t) {
            ENGINE_BLK(802CE044);
            n += func_802CE0E4(x, y, z, r, kind);
            ENGINE_BLK(802CE04C);
        }
    }
    ENGINE_BLK(802CE054);
    if (D_803A7410 != 0 || (ENGINE_BLK(802CE068), D_803A7412 != 0xFFF)) {
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
    KindPart *k;
    s32 any = 0, t;

    ENGINE_BLK(802CE0E4);
    /* the kind's first part */
    for (k = D_803A6B30;; k++) {
        ENGINE_BLK(802CE128);
        if (k->end == -1)
            goto out;
        ENGINE_BLK(802CE134);
        if (k->kind == kind)
            break;
    }
    /* and the ones after it of the kind */
    for (; ENGINE_BLK(802CE140), k->end != -1; k++) {
        ENGINE_BLK(802CE14C);
        if (k->kind != kind)
            break;
        ENGINE_BLK(802CE158);
        if (kind == 6) {
            ENGINE_BLK(802CE170);
            if (k->r == 0x3BD)
                continue;
        }
        ENGINE_BLK(802CE17C);
        t = func_8029CFA4(x, y, z, r, k->x, k->y, k->z, k->r);
        ENGINE_BLK(802CE184);
        if (!t)
            continue;
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
        func_802CE204(x, z, k->x, k->z);
        ENGINE_BLK(802CE1C4);
    }
out:
    ENGINE_BLK(802CE1CC);
    return any;
}

/* the heading from (x, z) to (tx, tz) (buildings.h's heading_to), and the
   camera kept within a quarter turn of it */
REGS(v0, a0, a2, t0)
void func_802CE204(s32 x, s32 z, s32 tx, s32 tz) {
    static const HeadingBlks hk = {
        {ENGINE_BLK_802CE284}, {ENGINE_BLK_802CE2B4}, {ENGINE_BLK_802CE2BC}, {ENGINE_BLK_802CE2EC},
        {ENGINE_BLK_802CE304}, {ENGINE_BLK_802CE334}, {ENGINE_BLK_802CE340}, {ENGINE_BLK_802CE370},
        {ENGINE_BLK_802CE278}, {ENGINE_BLK_802CE2F8},
    };
    s32 h = 0;

    ENGINE_BLK(802CE204);
    if (tx - x == 0) {
        ENGINE_BLK(802CE244);
        if (tz - z == 0)
            goto out;
    }
    ENGINE_BLK(802CE24C);
    h = heading_to(x, z, tx, tz, &hk);
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
