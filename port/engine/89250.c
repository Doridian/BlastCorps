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
s32 func_802A6274(u8 *anim, s32 t1, s32 t2, s32 t3, s32 t4, s32 t5, s32 t6, s32 t7, s32 s0, s32 s1, s32 s2,
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
    func_802A6274(D_802C382C, 0x591C8, 0, x << 11, y << 11, z << 11, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/* func_802C18D4 at the player's position */
void func_802CDAE8(s16 a, s16 b) {
    func_802C18D4(b, D_803A73F0 << 11, D_803A73F4 << 11, D_803A73F8 << 11, a);
}

/* the camera's limits off, if D_803A742F says */
#define LIMITS_OFF()                                                        \
    do {                                                                    \
        if (D_803A742F != 0) {                                              \
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

    D_803F932A = damage;
    for (b = D_803F4030; b != D_803F7654; b++) {
        t = func_8029CFA4(x, y, z, r, b->x, b->y, b->z, B_RADIUS(b));
        if (t) {
            if (B_ID(b) == MODEL_GOAL) {
                t = func_802BD8C8();
                if (t)
                    continue;
            }
            hit = func_802CDC7C(b, x, y, z, r, hit);
        }
    }
    LIMITS_OFF();
    return hit;
}

/* the pieces of building b the sphere hits: each turns the camera
   (func_802BF264), and the first does the damage (D_803F932A) unless
   `hit` was already set; returns whether any was hit (or `hit`) */
REGS(t3, v0, v1, a0, a1, fp -> fp)
s32 func_802CDC7C(Building *b, s32 x, s32 y, s32 z, s32 r, s32 hit) {
    Piece *p;

    x >>= 2;
    y >>= 2;
    z >>= 2;
    r >>= 2;
    for (p = b->unk4; p != (Piece *)b->unk8; p++) {
        if (p->active == 0)
            continue;
        if (piece_touched(p, x, y, z, r)) {
            if (hit == 0) {
                hit = D_803F932A;
                if (hit != 0) {
                    func_802CDD74(b, p, hit);
                }
            }
            func_802BF264(p);
            hit = 1;
        }
    }
    return hit;
}

/* piece p of building b takes `damage` (over the model's strength): its
   group's damage goes up to at most 100, and at 100 the group goes */
REGS(t9, s0, fp)
void func_802CDD74(Building *b, Piece *p, s32 damage) {
    s32 g, d;
    Piece *q;

    D_802E8BE4 = SHAKE_HIT_FRAMES;
    D_802E8BE8 = SHAKE_PLAYER_HIT;
    if (B_ID(b) == MODEL_GOAL)
        return;
    g = p->group;
    if (M_STRENGTH(B_MODEL(b)) == 0) {
        engine_break(N64_PC(0x802CDE40), 7);
    }
    d = B_DAMAGE(b)[g - 1] + (s32)engine_divu(damage, M_STRENGTH(B_MODEL(b)));
    if (d >= 100) {
        d = 100;
    }
    B_DAMAGE(b)[g - 1] = d;
    func_802BF898(g, d, b);
    if (d != 100)
        return;
    /* the group destroyed (as 77E20's func_802BEBB0 does it, without the
       shake) */
    func_802BF1F0(g, b);
    func_802C1438(g, b);
    func_802C09B8(g, b);
    func_802C0E8C(g, b);
    func_802BF384(b);
    for (q = b->unk4; q != (Piece *)b->unk8; q++) {
        if (q->group == g) {
            q->active = 0;
        }
        if (q->group2 == g) {
            if (B_DAMAGE(b)[q->group - 1] != 100) {
                q->active = 1;
            }
        }
    }
    func_802BF668(b);
    func_802BF534(b);
}

/* The level's solid objects the player's sphere (radius r) touches, and
   their kinds' parts (func_802CE0E4): returns how many kinds were hit. */
u8 func_802CDF94(s16 r) {
    s32 x = D_803A73F0, y = D_803A73F4, z = D_803A73F8;
    s32 n = 0, kind, t;
    Solid *s;

    D_803F932D = 0;
    D_803F932E = 0;
    for (s = D_803A7300; s->end != -1; s++) {
        kind = s->kind;
        if (kind == 0) {
            /* (kind 0 not while D_803649E8) */
            if (D_803649E8 != 0)
                continue;
        }
        t = func_8029CFA4(x, y, z, r, s->x, s->y, s->z, s->r);
        if (t) {
            n += func_802CE0E4(x, y, z, r, kind);
        }
    }
    if (D_803A7410 != 0 || (D_803A7412 != 0xFFF)) {
        D_803A7425 = 1;
        D_803A7424 = 1;
    }
    if (D_803A742F != 0) {
        D_803A7410 = 0;
        D_803A7412 = 0;
    }
    return n;
}

/* the parts of kind `kind` the sphere touches: each narrows the camera
   (func_802CE204); returns whether any did */
REGS(v0, v1, a0, a1, t4 -> s4)
s32 func_802CE0E4(s32 x, s32 y, s32 z, s32 r, s32 kind) {
    KindPart *k;
    s32 any = 0, t;

    /* the kind's first part */
    for (k = D_803A6B30;; k++) {
        if (k->end == -1)
            goto out;
        if (k->kind == kind)
            break;
    }
    /* and the ones after it of the kind */
    for (; k->end != -1; k++) {
        if (k->kind != kind)
            break;
        if (kind == 6) {
            if (k->r == 0x3BD)
                continue;
        }
        t = func_8029CFA4(x, y, z, r, k->x, k->y, k->z, k->r);
        if (!t)
            continue;
        if (D_80364456 == kind) {
            D_803F932E = 1;
        }
        any = 1;
        if (kind == 0xFF) {
            D_803F932D = 1;
        }
        func_802CE204(x, z, k->x, k->z);
    }
out:
    return any;
}

/* the heading from (x, z) to (tx, tz) (buildings.h's heading_to), and the
   camera kept within a quarter turn of it */
REGS(v0, a0, a2, t0)
void func_802CE204(s32 x, s32 z, s32 tx, s32 tz) {
    s32 h = 0;

    if (tx - x == 0) {
        if (tz - z == 0)
            goto out;
    }
    h = heading_to(x, z, tx, tz);
out:
    func_8029B7CC(h + 0x400, h - 0x400);
}

/* a pushed object's heading h kept off the camera's limits: moved 0x78
   past the nearer one, or to their middle */
s16 func_802CE3B8(s16 h) {
    s32 lo = D_803A7410, hi = D_803A7412, d1, d2, a;

    d1 = lo - h;
    if (d1 < 0) {
        d1 = -d1;
    }
    if (!(d1 < 0x801)) {
        d1 = 0xFFF - d1;
    }
    d2 = hi - h;
    if (d2 < 0) {
        d2 = -d2;
    }
    if (!(d2 < 0x801)) {
        d2 = 0xFFF - d2;
    }
    if (d2 < d1) {
        a = hi - 0x78;
        if (a < 0) {
            a += 0xFFF;
        }
    } else {
        a = lo + 0x78;
        if (!(a < 0x1000)) {
            a -= 0xFFF;
        }
    }
    if (hi < lo) {
        if (!(a < lo))
            goto keep;
        if (!(hi < a))
            goto keep;
    } else {
        if (!(a < lo)) {
            if (!(hi < a)) {
                goto keep;
            }
        }
    }
    a = func_802A6F6C();
keep:
    return a;
}

/* the player's position for the collision tests, and the camera's limits
   open */
void func_802CE4F0(s32 x, s32 y, s32 z) {
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
}

/* an object's collision against the buildings (func_8029B02C) */
void func_802CE5BC(s32 x, s32 y, s32 z, s16 r, s32 t8, s32 flag) {
    D_803A742A = flag;
    func_8029B02C(y, z, r, x >> 2, y >> 2, z >> 2, r >> 2, t8, 0);
    LIMITS_OFF();
}

/* a pushed object at (x, z) moved by its speed in direction h */
void func_802CE65C(s32 x, s32 z, s16 speed, s16 h) {
    s32 nz;

    D_803F9320 = x;
    D_803F9324 = z;
    D_803F9328 = speed;
    x = func_802A860C(h, &D_803F9328, &D_803F9320, &D_803F9324, 0.0f, &nz);
    D_803F9320 = x;
    D_803F9324 = nz;
}

/* The ground under an object at (x, z), from height y: the nearest of the
   static triangles, the moving objects' and the level grid's (as
   func_802A9B1C for a wheel); the object under it in D_803F932C; 0 or
   more. */
s32 func_802CE6F8(s32 x, s32 z, s32 y) {
    s32 s0, s1, t3, t4, t5, t6, t7, s2, fp, a1;

    y += 0x78;
    s0 = func_802A9DC0(x, z, y, 0, &s1);
    t4 = func_802A9F24(x, z, y, 0, &t6);
    a1 = func_802AA094(x, z, y, t4, s1, &t3, &fp);
    if (a1 == 0) {
        if (s0 == 99999999) {
            if (t4 == 99999999) {
                t3 = y - 0x78;
                goto none;
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
            if (!(s2 < t5))
                goto none;
        }
    }
    if (t7 < s2) {
        t3 = t4;
        goto store;
    }
    t3 = s0;
none:
    t6 = 0;
store:
    D_803F932C = t6;
    if (t3 < 0) {
        t3 = 0;
    }
    (void)fp;
    return t3;
}
