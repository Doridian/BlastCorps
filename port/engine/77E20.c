/*
 * hd_code 77E20 (us.v11 0x802BC5E0-0x802C2184): the buildings and their
 * destruction, as native C (engine.h, buildings.h).
 *
 * A building (objects.h) takes damage by groups (a byte each from 0xEC,
 * 0..100, buildings.h): a vehicle running into it (func_802BE77C, which
 * every vehicle module calls with its type in $t8 and its VehicleState in
 * $gp), the game's C's collision tests (89250) and the delayed damage
 * queue (D_803F7690: func_802BC888 adds to it, func_802BCA2C runs it).  A
 * group at 100 is destroyed: its pieces stop colliding, the ones that
 * depend on it fall (func_802BD1F8 draws them falling), debris and smoke
 * are started (the effect records, D_803F3968) and the damage counts
 * towards the level's (D_803649F0).
 *
 * The model file's sections used here, by the offset of their offset:
 * 0x10..0x18 the display list's head, 0x1C four corners, 0x20 the shadow's
 * rectangle, 0x24..0x28 triangles, 0x28..0x2C the animated textures,
 * 0x2C..0x30 the groups' centres (s16 x, y, z), 0x30..0x38 the effect
 * records (two lists), 0x38..0x3C the groups' display lists, 0x3C their
 * fall heights, 0x40..0x44 heights, 0x44..0x48 the groups' dependencies.
 */
#include "buildings.h"
#include "game/game.h"
#include "game/audio.h"
#include "game/level.h"

/* ---- the data -------------------------------------------------------- */

extern u8 D_803F3968[30][0x38];         /* the effect records */
extern u8 D_803F3FF8[0x38];             /* the one being made */
extern u8 D_803F7690[40][8];            /* delayed damage: building, amount, group, frames */
extern u8 *PTR32 D_803F77D4;            /* the kinds hit this frame (D_803F77D8...) */
extern u8 D_803F77D8[];
extern u8 *PTR32 D_803F77E4;            /* and last frame's (D_803F77E8...) */
extern u8 D_803F77E8[];
extern u8 *PTR32 D_803F3960;            /* (building, group) pairs hit this frame */
extern u8 D_803F3910[];
extern u8 *PTR32 D_803F3964;            /* this frame's matrices for falling groups */
extern u8 D_803F24D0[], D_803F2ED0[];   /* (one per frame) */
extern s32 D_803F38D0[16];              /* a matrix being made */
extern Mtx *PTR32 D_803F7658;
extern Mtx *PTR32 D_803F765C;           /* the next free matrix */
extern s32 D_803F7660;
extern s32 D_803F7664, D_803F7668, D_803F766C;
extern s32 D_803F7670, D_803F7674, D_803F7678;
extern s32 D_803F7684, D_803F7688;
extern s32 D_803F77F4, D_803F77F8;
extern s16 D_803F77FC;
extern u16 D_803F77FE;                  /* the damage of the last hit */
extern u8 D_803F7800, D_803F7801, D_803F7802, D_803F7803, D_803F7804, D_803F7805;
extern u8 D_803F7806, D_803F7807, D_803F7808, D_803F7809;
extern s8 D_803F780A;
extern u8 D_803F780B, D_803F780C, D_803F780D, D_803F780E, D_803F780F, D_803F7810, D_803F7812;
extern Part *PTR32 D_803F77D0;
extern u8 D_803F0900[4][0x4B8];         /* smoke clouds */
extern u8 D_803EFED0[0xA30];            /* dust */
extern u8 D_803F1BE0[2][0x478];         /* falling groups' shadows */
extern u8 *PTR32 D_803F7820, *PTR32 D_803F7824;
extern u8 *PTR32 D_803F7828;

extern u8 D_803643D6, D_803643D7, D_803643DB, D_80364AC1;
extern u32 D_803649E8;
extern u64 D_803649D8;                  /* a random state */
extern s32 D_802E8BDC;                  /* the level */
extern s32 D_802E8BE8;
extern s32 D_80358068;
extern u8 D_80306480[];                 /* the objects to destroy: 0x30-byte records */
extern UnkStruct_8039C800 D_8039C800[];
extern u8 D_8039C940;
extern u8 *PTR32 D_803BE708;            /* the level's building groups */
extern u8 D_803BE738;
extern u32 D_8036C790;
extern u16 D_803EF6FC;
extern s32 D_8036C7C8;
extern s16 D_803C30A8[];                /* the visible cells, to -1 */
extern s32 D_803643F8, D_80364400;      /* the camera's position */
extern s32 D_803A740C;
extern u32 D_803059F0[];                /* (level, model, types) */
extern u8 D_8036B971;
extern Building *PTR32 D_8036B974;
extern u8 D_80370C2C;
extern u8 D_8036CB2E, D_8036CB2F;
extern u16 D_8036CB2A, D_8036CB2C;
extern u8 D_80305E50[];
extern u32 D_80305E10[];
extern s16 D_80305E38[];
extern u8 D_8030633C[];
extern u8 D_80306344[], D_80306350[], D_803063D4[], D_803063E0[];
extern u8 *PTR32 D_80306270[];
extern u32 D_802C3FFC[];
extern u8 D_8036DCD4, D_8036DCD7;
extern u64 D_802F46C0[24];
extern u64 D_802F4780[6];
extern u8 D_8036EB92;
extern u32 D_80364A40;
extern u16 D_803649E0, D_803649E2, D_803649E4;
extern u8 D_803BDFD8[];
extern u8 *PTR32 D_803BDFD4;

void func_80275390(u64 mode);
s32 func_8026A828(s32 lo, s32 hi);
s32 func_8026A8E0(s32 lo, s32 hi);
u8 func_80286090(s32 level);
s32 func_80288284(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
void func_80264CB4(s16 arg0, s16 arg1, s16 arg2, s16 arg3, u8 arg4, s32 arg5);
void func_8026A454(s16 x, s16 y, s16 z, s16 arg3, s16 arg4, Mtx *arg5);
void func_802933A0(s32 arg0, s32 arg1, s32 arg2, s32 arg3, Mtx *arg4, void *arg5, Gfx *arg6, Gfx *arg7, s32 arg8,
                   s32 arg9, s32 arg10, s32 arg11);
void func_802619D0(u32 arg0);

/* 56040 (still translated): the object `id`'s state byte */
REGS(t4 -> t5)
s32 func_8029D210(s32 id);
/* 60F60 (engine-B's) */
REGS(t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, a3 -> t0)
s32 func_802A6274(s32 t0, s32 t1, s32 t2, s32 t3, s32 t4, s32 t5, s32 t6, s32 t7, s32 s0, s32 s1, s32 s2,
                  s32 s3, s32 s4, s32 s5, s32 a3);
/* 62740 (native) */
REGS(t3, t4, t5, t6, t7, s0 -> s1+f0)
s64 func_802ABCDC(s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2);

/* this module's */
REGS(-> t4)
s32 func_802BC714(void);
REGS(-> t4)
s32 func_802BC7DC(void);
REGS(a2, t3, t9)
void func_802BC888(s32 amount, s32 group, Building *b);
REGS(fp)
void func_802BCCD4(s32 kind);
REGS(v0 -> v1)
s32 func_802BCD80(s32 kind);
REGS(fp -> v1)
s32 func_802BCDE0(s32 kind);
REGS(t0 -> v0)
s32 func_802BD064(Building *next);
REGS(v0, a3)
void func_802BD85C(Building *b, s32 g);
REGS(v0, a2, a3, t0)
void func_802BD99C(Building *b, s32 dx, s32 dy, s32 dz);
REGS(v0, t1, t2)
void func_802BDDB4(Building *b, u8 *anim, u8 *end);
REGS(a2, s0, t1, t2, t7 -> a2, t7)
u32 *func_802BE228(u32 *dl, u32 *end, u8 *anim, u8 *anim_end, u32 *src, u32 **src_out);
REGS(t0, s0, t1, t2, t7 -> t0, t7)
u32 *func_802BE3C8(u32 *dl, u32 *end, u8 *anim, u8 *anim_end, u32 *src, u32 **src_out);
REGS(v0, a3, s6 -> s5)
u32 func_802BE574(Building *b, s32 g2, s32 fall);
REGS(t8, gp)
void func_802BE77C(s32 type, VS *vs);
REGS(t3, t8 -> t9)
s32 func_802BE944(Building *b, s32 type);
REGS()
void func_802BE9F8(void);
REGS(t3, t9)
void func_802BEA30(s32 group, Building *b);
REGS(t3, t9 -> t7)
s32 func_802BEA70(s32 group, Building *b);
REGS(t3, v0, v1, a0, a1, t8)
void func_802BEADC(Building *b, s32 x, s32 y, s32 z, s32 r, s32 type);
REGS(s0, t8, t9)
void func_802BEBB0(Piece *p, s32 type, Building *b);
REGS(t9)
void func_802BEF9C(Building *b);
REGS()
void func_802BEFF4(void);
REGS(v0, t3, t5, t9)
void func_802BF978(u8 *model, s32 group, s32 damage, Building *b);
REGS(a0, a1 -> s4)
s32 func_802BFB50(s32 lo, s32 hi);
REGS(v1, t9)
void func_802BFBF4(u8 *fx, Building *b);
REGS(v0, a3)
void func_802BFD1C(Building *b, s32 g);
REGS(v0, a3)
void func_802BFDAC(Building *b, s32 group);
REGS(gp)
void func_802BFEE4(VS *vs);
REGS(t3, t7, t8, t9, gp, fp -> t7)
s32 func_802BFF6C(s32 group, s32 power, s32 type, Building *b, s32 strength, s32 n);
REGS(t7, t8, gp -> t2, t7)
s32 func_802C0284(s32 power, s32 type, s32 strength, s32 *power_out);
REGS(v0, t9 -> t3)
s32 func_802C038C(s32 group, Building *b);
REGS(v0)
void func_802C08C4(u32 *gfx);
REGS(t4 -> t4)
u64 func_802C0C64(u64 cmd);
REGS(v0)
void func_802C0CBC(u32 *gfx);
REGS(v1, t9)
void func_802C1214(u8 *slot, Building *b);
REGS(a0, a1 -> a0, a1)
u32 *func_802C12E0(u32 *g0, u32 *g1, u32 **g1_out);
REGS(t3, t9 -> a2)
s32 func_802C1A28(s32 group, Building *b);

/* raw fields */
#define U8_(p, off) (*(u8 *)((u8 *)(p) + (off)))
#define S8_(p, off) (*(s8 *)((u8 *)(p) + (off)))
#define U16_(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S16_(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U32_(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S32_(p, off) (*(s32 *)((u8 *)(p) + (off)))
#define U64_(p, off) (*(u64 *)((u8 *)(p) + (off)))

#define K0(p) ((u32)(p) - 0x80000000)
/* %hi() of a symbol, as lui leaves it */
#define HI(sym) (((u32)(sym) + 0x8000) & 0xFFFF0000)

/* a block table, for code two functions share */
typedef struct { u16 id, n; } Blk;
#define BLKT(t, i) ENGINE_BLK_((t)[i].id, (t)[i].n)

/* ---- the level's end, the targets ------------------------------------ */

/* Each frame (hd.c): when D_803F7805 is set (the level's target building
   hit), the level is won if every hole is filled and every object to
   destroy is (or there are none): D_803643DA, and the game mode moves
   on. */
void func_802BC5E0(void) {
    s32 ok;

    ENGINE_BLK(802BC5E0);
    if (D_803F7805 == 0)
        goto out;
    ENGINE_BLK(802BC620);
    D_803F7805 = 0;
    if (D_803643DB != 0)
        goto check;
    ENGINE_BLK(802BC638);
    if (D_80364AC1 == 0)
        goto won;
check:
    ENGINE_BLK(802BC648);
    if (D_803F7806 == 0)
        goto out;
    ENGINE_BLK(802BC658);
    ok = func_802BC7DC();
    ENGINE_BLK(802BC660);
    if (!ok)
        goto out;
    ENGINE_BLK(802BC668);
    ok = func_802BC714();
    ENGINE_BLK(802BC670);
    if (!ok)
        goto out;
won:
    ENGINE_BLK(802BC678);
    D_803643DA = 1;
    D_802E8BD8 = 1;
    if (D_803643DB != 0)
        goto out;
    ENGINE_BLK(802BC6A0);
    if (D_80364A90 == 0x40) {
        ENGINE_BLK(802BC6D0);
        func_80275390(D_80364A90);
    } else {
        ENGINE_BLK(802BC6B4);
        *((u8 *)&D_803649E8 + NE_X3(0)) = 1;
        D_80364A98 = 8;
    }
out:
    ENGINE_BLK(802BC6DC);
}

/* whether every object of this level to destroy (D_80306480) is: the
   ones whose distance limit (+8) isn't under D_803EF6E4, by their ids'
   state (func_8029D210) */
REGS(-> t4)
s32 func_802BC714(void) {
    u8 *p = D_80306480, *q;
    s32 lim = D_803EF6E4, ok = 1;

    ENGINE_BLK(802BC714);
    for (;;) {
        ENGINE_BLK(802BC748);
        if (S8_(p, 0x15) == -1)
            break;
        ENGINE_BLK(802BC758);
        if (S8_(p, 0x15) != D_802E8BDC)
            goto next;
        ENGINE_BLK(802BC768);
        if (S32_(p, 8) < lim)
            goto next;
        ENGINE_BLK(802BC778);
        q = p + 0xC;
        for (;;) {
            ENGINE_BLK(802BC77C);
            if ((s8)*q == -1)
                break;
            ENGINE_BLK(802BC78C);
            if (func_8029D210((s8)*q)) {
                ENGINE_BLK(802BC794);
                ENGINE_BLK(802BC7A4);
                ok = 0;
                goto out;
            }
            ENGINE_BLK(802BC794);
            ENGINE_BLK(802BC79C);
            q++;
        }
    next:
        ENGINE_BLK(802BC7AC);
        p += 0x30;
    }
out:
    ENGINE_BLK(802BC7B4);
    return ok;
}

/* whether every hole is filled */
REGS(-> t4)
s32 func_802BC7DC(void) {
    UnkStruct_8039C800 *h = D_8039C800;
    s32 n = D_8039C940;

    ENGINE_BLK(802BC7DC);
    for (;;) {
        ENGINE_BLK(802BC804);
        if (n == 0) {
            ENGINE_BLK(802BC824);
            ENGINE_BLK(802BC828);
            return 1;
        }
        ENGINE_BLK(802BC80C);
        if (h->unk26 == 0)
            break;
        ENGINE_BLK(802BC818);
        n--;
        h++;
    }
    ENGINE_BLK(802BC828);
    return 0;
}

/* ---- the delayed damage ---------------------------------------------- */

/* the queue emptied */
REGS()
void func_802BC840(void) {
    u8 *p = D_803F7690[0];
    s32 n;

    ENGINE_BLK(802BC840);
    for (n = 40;; p += 8) {
        ENGINE_BLK(802BC85C);
        if (n == 0)
            break;
        ENGINE_BLK(802BC864);
        n--;
        p[7] = 0;
    }
    ENGINE_BLK(802BC874);
}

/* `amount` of damage to group `group` of building b: added to what is
   queued for it, or queued for 1..19 frames from now */
REGS(a2, t3, t9)
void func_802BC888(s32 amount, s32 group, Building *b) {
    u8 *p = D_803F7690[0];
    s32 n;

    ENGINE_BLK(802BC888);
    for (n = 40;;) {
        ENGINE_BLK(802BC918);
        if (n == 0)
            break;
        ENGINE_BLK(802BC920);
        n--;
        if (p[7] == 0) {
            p += 8;
            continue;
        }
        ENGINE_BLK(802BC930);
        if (U32_(p, 0) != (u32)b) {
            p += 8;
            continue;
        }
        ENGINE_BLK(802BC93C);
        if (p[6] != group) {
            p += 8;
            continue;
        }
        ENGINE_BLK(802BC948);
        U16_(p, 4) += amount;
        goto out;
    }
    ENGINE_BLK(802BC958);
    p = D_803F7690[0];
    for (n = 40;;) {
        ENGINE_BLK(802BC964);
        if (n == 0)
            goto out;
        ENGINE_BLK(802BC96C);
        n--;
        if (p[7] == 0)
            break;
        ENGINE_BLK(802BC97C);
        p += 8;
    }
    ENGINE_BLK(802BC984);
    p[6] = group;
    U16_(p, 4) = amount;
    U32_(p, 0) = (u32)b;
    p[7] = func_8026A8E0(1, 0x14);
    ENGINE_BLK(802BC9A0);
out:
    ENGINE_BLK(802BC9A4);
}

/* Each frame (hd.c): the queued damage whose time has come, done as a hit
   (damage capped at 100; at 100 the group goes) */
void func_802BCA2C(void) {
    u8 *p = D_803F7690[0], *dmg;
    Building *b;
    Piece *q, *end;
    s32 n, k, amount, g, d;

    ENGINE_BLK(802BCA2C);
    for (n = 40;; p += 8) {
        ENGINE_BLK(802BCA68);
        if (n == 0)
            break;
        ENGINE_BLK(802BCA70);
        k = p[7];
        n--;
        if (k == 0)
            goto next;
        ENGINE_BLK(802BCA80);
        k--;
        if (k != 0) {
            ENGINE_BLK(802BCB98);
            p[7] = k;
            goto next;
        }
        ENGINE_BLK(802BCA8C);
        amount = U16_(p, 4);
        p[7] = 0;
        b = (Building *)U32_(p, 0);
        g = p[6];
        D_802E8BE4 = 10;
        if (amount < 0x1F5) {
            ENGINE_BLK(802BCAB4);
            D_802E8BE8 = amount;
        } else {
            ENGINE_BLK(802BCABC);
            D_802E8BE8 = 500;
        }
        ENGINE_BLK(802BCAC8);
        dmg = &B_DAMAGE(b)[g - 1];
        d = *dmg + amount;
        if (!(d < 100)) {
            ENGINE_BLK(802BCAE4);
            d = 100;
        }
        ENGINE_BLK(802BCAE8);
        *dmg = d;
        if (d == 100) {
            ENGINE_BLK(802BCAF4);
            func_802C1438(g, b);
            ENGINE_BLK(802BCAFC);
            if (D_803643D6 != 0)
                goto smoke;
            ENGINE_BLK(802BCB0C);
            if (D_803643D7 != 0)
                goto smoke;
            ENGINE_BLK(802BCB1C);
            func_802C09B8(g, b);
        smoke:
            ENGINE_BLK(802BCB24);
            func_802C0E8C(g, b);
            ENGINE_BLK(802BCB2C);
            if (D_80364A90 != 0x200) {
                ENGINE_BLK(802BCB40);
                if (D_80364A90 != 0x400) {
                    ENGINE_BLK(802BCB48);
                    func_802BF384(b);
                }
            }
            ENGINE_BLK(802BCB50);
            q = b->unk4;
            end = b->unk8;
            for (;;) {
                ENGINE_BLK(802BCB58);
                if (q == end)
                    break;
                ENGINE_BLK(802BCB60);
                if (q->group != g) {
                    q++;
                    continue;
                }
                ENGINE_BLK(802BCB6C);
                q->active = 0;
                q++;
            }
        }
        ENGINE_BLK(802BCB78);
        func_802BF668(b);
        ENGINE_BLK(802BCB80);
        func_802BF534(b);
        ENGINE_BLK(802BCB88);
        func_802BF898(g, d, b);
        ENGINE_BLK(802BCB90);
    next:
        ENGINE_BLK(802BCB9C);
    }
    ENGINE_BLK(802BCBA4);
}

/* ---- the kinds of objects hit, this frame and the last --------------- */

/* this frame's list emptied */
REGS()
void func_802BCBD8(void) {
    ENGINE_BLK(802BCBD8);
    D_803F77D4 = D_803F77D8;
}

/* last frame's list emptied */
REGS()
void func_802BCC10(void) {
    ENGINE_BLK(802BCC10);
    D_803F77E4 = D_803F77E8;
}

/* this frame's list becomes last frame's (emptied while D_80358060 is 0) */
REGS()
void func_802BCC48(void) {
    u8 *d = D_803F77E8, *s, *end;

    ENGINE_BLK(802BCC48);
    if (D_80358060 != 0) {
        ENGINE_BLK(802BCC7C);
        s = D_803F77D8;
        end = D_803F77D4;
        for (;;) {
            ENGINE_BLK(802BCC90);
            if (s == end)
                break;
            ENGINE_BLK(802BCC98);
            *d++ = *s++;
        }
    }
    ENGINE_BLK(802BCCAC);
    D_803F77E4 = d;
}

/* kind `kind` added to this frame's list (once) */
REGS(fp)
void func_802BCCD4(s32 kind) {
    u8 *p;

    ENGINE_BLK(802BCCD4);
    if (func_802BCD80(kind) == 0) {
        ENGINE_BLK(802BCCEC);
        ENGINE_BLK(802BCCF4);
        p = D_803F77D4;
        *p = kind;
        D_803F77D4 = p + 1;
    } else {
        ENGINE_BLK(802BCCEC);
    }
    ENGINE_BLK(802BCD0C);
}

/* whether this frame's list holds anything but `kind` (unused) */
REGS(v0 -> v1)
s32 func_802BCD20(s32 kind) {
    u8 *p = D_803F77D8, *end = D_803F77D4;
    s32 r = 0;

    ENGINE_BLK(802BCD20);
    for (;;) {
        ENGINE_BLK(802BCD4C);
        if (p == end)
            break;
        ENGINE_BLK(802BCD54);
        if (kind != *p++) {
            ENGINE_BLK(802BCD64);
            r = 1;
            break;
        }
    }
    ENGINE_BLK(802BCD68);
    return r;
}

/* whether this frame's list holds `kind` */
REGS(v0 -> v1)
s32 func_802BCD80(s32 kind) {
    u8 *p = D_803F77D8, *end = D_803F77D4;
    s32 r = 0;

    ENGINE_BLK(802BCD80);
    for (;;) {
        ENGINE_BLK(802BCDAC);
        if (p == end)
            break;
        ENGINE_BLK(802BCDB4);
        if (kind == *p++) {
            ENGINE_BLK(802BCDC4);
            r = 1;
            break;
        }
    }
    ENGINE_BLK(802BCDC8);
    return r;
}

/* whether last frame's list holds `kind` */
REGS(fp -> v1)
s32 func_802BCDE0(s32 kind) {
    u8 *p = D_803F77E8, *end = D_803F77E4;
    s32 r = 0;

    ENGINE_BLK(802BCDE0);
    for (;;) {
        ENGINE_BLK(802BCE0C);
        if (p == end)
            break;
        ENGINE_BLK(802BCE14);
        if (kind == *p++) {
            ENGINE_BLK(802BCE24);
            r = 1;
            break;
        }
    }
    ENGINE_BLK(802BCE28);
    return r;
}

/* ---- the nearest target ---------------------------------------------- */

/* The nearest target to the player (30C70.c, for the radar): an intact
   building to destroy, an empty hole, or an object to destroy that isn't
   yet; its kind (1, 2, 3; 0 none) in D_803F7808 and its outline in
   D_8036C790. */
s32 func_802BCE40(void) {
    Building *b = D_803F4030, *end = D_803F7654;
    s32 px = D_803643E0, py = D_803643E4, pz = D_803643E8;
    s64 best = 9999999, d;
    u32 found = 0, outline;
    s32 kind = 0, n;
    UnkStruct_8039C800 *h;
    u8 *o, *q;

    ENGINE_BLK(802BCE40);
    for (;;) {
        ENGINE_BLK(802BCEB0);
        if (b == end)
            break;
        ENGINE_BLK(802BCEB8);
        if (b->unkEB == 0) {
            b++;
            continue;
        }
        ENGINE_BLK(802BCEC4);
        if (b->unkEA != 0) {
            b++;
            continue;
        }
        ENGINE_BLK(802BCED0);
        d = func_802ABCDC(b->x, b->y, b->z, px, py, pz);
        ENGINE_BLK(802BCEE0);
        b++;
        if (!(d < best))
            continue;
        ENGINE_BLK(802BCEEC);
        if (func_802BD064(b)) {
            ENGINE_BLK(802BCEF4);
            continue;
        }
        ENGINE_BLK(802BCEF4);
        ENGINE_BLK(802BCEFC);
        best = d;
        found = (u32)(b - 1);
        kind = 1;
    }
    ENGINE_BLK(802BCF0C);
    h = D_8039C800;
    for (n = D_8039C940;; n--, h++) {
        ENGINE_BLK(802BCF1C);
        if (n == 0)
            break;
        ENGINE_BLK(802BCF24);
        if (h->unk26 == 0) {
            ENGINE_BLK(802BCF30);
            d = func_802ABCDC(h->x, h->y, h->z, px, py, pz);
            ENGINE_BLK(802BCF40);
            if (d < best) {
                ENGINE_BLK(802BCF4C);
                kind = 2;
                found = (u32)h;
                best = d;
            }
        }
        ENGINE_BLK(802BCF58);
    }
    ENGINE_BLK(802BCF64);
    o = D_80306480;
    for (;;) {
        ENGINE_BLK(802BCF6C);
        if (S8_(o, 0x15) == -1)
            break;
        ENGINE_BLK(802BCF7C);
        if (S8_(o, 0x15) != D_802E8BDC)
            goto next;
        ENGINE_BLK(802BCF8C);
        d = func_802ABCDC(S32_(o, 0), S32_(o, 4), S32_(o, 8), px, py, pz);
        ENGINE_BLK(802BCF9C);
        if (!(d < best))
            goto next;
        ENGINE_BLK(802BCFA8);
        q = o + 0xC;
        for (;;) {
            ENGINE_BLK(802BCFAC);
            if ((s8)*q == -1)
                break;
            ENGINE_BLK(802BCFBC);
            if (func_8029D210((s8)*q)) {
                ENGINE_BLK(802BCFC4);
                ENGINE_BLK(802BCFD4);
                kind = 3;
                found = (u32)o;
                best = d;
                break;
            }
            ENGINE_BLK(802BCFC4);
            ENGINE_BLK(802BCFCC);
            q++;
        }
    next:
        ENGINE_BLK(802BCFE0);
        o += 0x30;
    }
    ENGINE_BLK(802BCFE8);
    outline = 0;
    if (kind != 0) {
        ENGINE_BLK(802BCFF0);
        if (kind == 1) {
            ENGINE_BLK(802BD014);
            outline = (u32)B_SECTION((Building *)found, 0x1C);
        } else {
            ENGINE_BLK(802BCFFC);
            if (kind == 3) {
                ENGINE_BLK(802BD00C);
                outline = found + 0x16;
            } else {
                ENGINE_BLK(802BD004);
                outline = found + 0xC;
            }
        }
    }
    ENGINE_BLK(802BD020);
    D_8036C790 = outline;
    D_803F7808 = kind;
    return found;
}

/* whether building `next - 1` belongs to a group of the level's
   (D_803BE708) all of whose members are destroyed */
REGS(t0 -> v0)
s32 func_802BD064(Building *next) {
    u8 *b = (u8 *)(next - 1), *p = D_803BE708;
    s32 n, k;

    ENGINE_BLK(802BD064);
    if (p == NULL)
        goto no;
    ENGINE_BLK(802BD090);
    n = U32_(p, 0);
    p += 4;
    for (;;) {
        ENGINE_BLK(802BD098);
        if (n == 0)
            goto no;
        ENGINE_BLK(802BD0A0);
        n--;
        if (U32_(p, 0) == (u32)b)
            break;
        ENGINE_BLK(802BD0B0);
        p += 0x18;
    }
    ENGINE_BLK(802BD0B8);
    k = U32_(p, 4);
    p += 8;
    for (;;) {
        ENGINE_BLK(802BD0C0);
        if (k == 0) {
            ENGINE_BLK(802BD0EC);
            ENGINE_BLK(802BD0F0);
            return 1;
        }
        ENGINE_BLK(802BD0C8);
        k--;
        if (b[0xEC + *p] != 100)
            goto no;
        ENGINE_BLK(802BD0E4);
        p++;
    }
no:
    ENGINE_BLK(802BD0F0);
    return 0;
}

/* the distance from the CMO (D_803EF6DC, D_803EF6E4) to target t in x
   and z, over D_803EF6FC: into D_8036C7C8 (30C70.c) */
void func_802BD10C(s32 t_) {
    u8 *t = (u8 *)t_;
    s32 x, z;
    s64 d;

    ENGINE_BLK(802BD10C);
    if (D_803F7809 == 2) {
        ENGINE_BLK(802BD168);
        x = S32_(t, 0);
        z = S32_(t, 8);
    } else {
        ENGINE_BLK(802BD144);
        if (D_803F7809 == 3) {
            ENGINE_BLK(802BD15C);
            x = S32_(t, 0);
            z = S32_(t, 8);
        } else {
            ENGINE_BLK(802BD150);
            x = S32_(t, 0x10);
            z = S32_(t, 0x18);
        }
    }
    ENGINE_BLK(802BD170);
    d = func_802ABCDC(x, 0, z, D_803EF6DC, 0, D_803EF6E4);
    ENGINE_BLK(802BD194);
    if (D_803EF6FC != 0) {
        ENGINE_BLK(802BD1A8);
        d = (s32)engine_divu((u32)d, D_803EF6FC);
    }
    ENGINE_BLK(802BD1C0);
    D_8036C7C8 = (s32)d;
}

/* ---- drawing them ---------------------------------------------------- */

/* Each frame (hd.c): the buildings in the visible cells drawn into two
   display lists (g0, g1: the frame's two passes), each building's lists
   made at d0 and d1; the falling groups' matrices from D_803F3964
   (D_803F24D0 or D_803F2ED0 by frame), the moving ones' from arg5; the
   smoke (g6) and the dust (g7) first, the shadows of the falling groups
   last. */
void func_802BD1F8(Gfx *g0_, Gfx *g1_, Gfx *d0_, Gfx *d1_, Mtx *arg4, Mtx *arg5, Gfx *g6, Gfx *g7) {
    u32 *g0 = (u32 *)g0_, *g1 = (u32 *)g1_, *d0 = (u32 *)d0_, *d1 = (u32 *)d1_;
    Building *b, *end;
    s16 *cell;
    u8 *model, *anim, *anim_end, *grp, *grp_end;
    u32 *src, *src_end;
    Mtx *m;
    s32 first, n, g2, falling, gi, thr, dmg, t, fall, h;
    u32 mtx = 0;
    /* what the original leaves in $t1-$t8 and $a3 (the game's C goes on to
       translated code that may read them) */
    u32 rv[32], rset = 0;
#define R(r, v) (rv[r] = (u32)(v), rset |= 1u << (r))

    ENGINE_SAVE(G(rS0) | G(rS1) | G(rS2) | G(rS3) | G(rS4) | G(rS5) | G(rS6) | G(rS7) | G(rGP) | G(rFP));
    ENGINE_BLK(802BD1F8);
    D_803F7658 = arg4;
    D_803F765C = arg5;
    func_802C08C4((u32 *)g6);
    ENGINE_BLK(802BD248);
    func_802C0CBC((u32 *)g7);
    ENGINE_BLK(802BD250);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BD268);
        D_803F3964 = D_803F24D0;
    } else {
        ENGINE_BLK(802BD274);
        D_803F3964 = D_803F2ED0;
    }
    ENGINE_BLK(802BD27C);
    end = D_803F7654;
    for (b = D_803F4030;; b++) {
        ENGINE_BLK(802BD29C);
        if (b == end)
            break;
        ENGINE_BLK(802BD2A4);
        cell = D_803C30A8;
        R(rA3, b->unkE8);
        for (;;) {
            ENGINE_BLK(802BD2B0);
            R(rT2, (s32)*cell);
            R(rT1, cell + 1);
            if (*cell++ == -1)
                goto done;
            ENGINE_BLK(802BD2C4);
            if (b->unkE8 == cell[-1])
                break;
        }
        ENGINE_BLK(802BD2CC);
        if (b->unk30 == 0x38) {
            ENGINE_BLK(802BD2DC);
            t = func_802BD8C8();
            R(rT1, t);
            ENGINE_BLK(802BD2E4);
            if (t)
                goto done;
        }
        ENGINE_BLK(802BD2EC);
        model = B_MODEL(b);
        if (U16_(model, 0xE) != 0) {
            /* a moving building: its matrix (4EBE0.c), and its parts
               moved to where it is now */
            ENGINE_BLK(802BD2FC);
            m = D_803F765C;
            D_803F765C = (Mtx *)((u8 *)m + 0x40);
            func_802933A0(b->unk1C, b->unk20, b->unk24, U16_(model, 0xE), m, (u8 *)b + 0x38, (Gfx *)g0,
                          (Gfx *)g1, b->unk34, b->x, b->y, b->z);
            ENGINE_BLK(802BD3A4);
            g0 += 2;
            g1 += 2;
            {
                s32 ox = b->x, oy = b->y, oz = b->z;

                b->x = D_803F7664;
                b->y = D_803F7668;
                b->z = D_803F766C;
                ENGINE_SAVE(G(rV0) | G(rV1) | G(rA0) | G(rA1) | G(rA2) | G(rA3) | G(rT0) | G(rT1) | G(rT2) | G(rT3) |
                            G(rT4) | G(rT5) | G(rT6) | G(rT7) | G(rT8) | G(rT9));
                func_802BD99C(b, D_803F7664 - ox, D_803F7668 - oy, D_803F766C - oz);
                ENGINE_RESTORE();
            }
            ENGINE_BLK(802BD480);
        }
        ENGINE_BLK(802BD4C8);
        model = B_MODEL(b);
        anim = B_SECTION(b, 0x28);
        anim_end = B_SECTION(b, 0x2C);
        grp = B_SECTION(b, 0x38);
        grp_end = B_SECTION(b, 0x3C);
        R(rT3, model);
        R(rT1, anim);
        R(rT2, anim_end);
        R(rT4, grp);
        R(rT5, grp_end);
        func_802BDDB4(b, anim, anim_end);
        ENGINE_BLK(802BD4F0);
        g0[0] = 0xBC002406;             /* segment 9: the model's textures */
        g0[1] = K0(model + 0x50);
        g1[0] = 0xBC002406;
        g1[1] = K0(model + 0x50);
        g0[2] = 0x06000000;             /* its lists, made below */
        g0[3] = K0(d0);
        g1[2] = 0x06000000;
        g1[3] = K0(d1);
        g0 += 4;
        g1 += 4;
        src = (u32 *)B_SECTION(b, 0x10);
        src_end = (u32 *)B_SECTION(b, 0x18);
        R(rT6, 0x80000000);
        R(rA3, src_end);
        R(rT7, src_end);
        for (;;) {
            ENGINE_BLK(802BD568);
            if (src == src_end)
                break;
            ENGINE_BLK(802BD570);
            *(u64 *)d0 = *(u64 *)src;
            *(u64 *)d1 = *(u64 *)src;
            src += 2;
            d0 += 2;
            d1 += 2;
        }
        ENGINE_BLK(802BD58C);
        /* the groups' lists, each under its conditions on other groups'
           damage: the first one drawn ends them, unless one was passed */
        first = 1;
        R(rT6, 1);
        for (;;) {
            ENGINE_BLK(802BD590);
            if (grp == grp_end)
                break;
            ENGINE_BLK(802BD598);
            gi = U16_(grp, 4) - 1;
            n = U32_(grp, 0);
            grp += 4;
            R(rA3, gi);
            R(rT7, n);
            R(rT4, grp);
            for (;;) {
                ENGINE_BLK(802BD5A8);
                if (n == 0)
                    goto draw;
                ENGINE_BLK(802BD5B0);
                dmg = B_DAMAGE(b)[U16_(grp, 0) - 1];
                n--;
                thr = U8_(grp, 3);
                t = U8_(grp, 2);
                grp += 4;
                R(rT7, n);
                R(rT4, grp);
                if (t != 0) {
                    ENGINE_BLK(802BD5D8);
                    if (!(dmg < thr))
                        continue;
                    ENGINE_BLK(802BD5E4);
                } else {
                    ENGINE_BLK(802BD5EC);
                    if (!(thr < dmg))
                        continue;
                }
                ENGINE_BLK(802BD5F8);
                grp += n * 4 + 0x10;
                R(rT7, n * 4);
                R(rT4, grp);
                goto passed;
            }
        draw:
            ENGINE_BLK(802BD608);
            g2 = gi * 2;
            R(rA3, g2);
            falling = U16_(b, 0x48 + g2);
            if (falling != 0) {
                ENGINE_BLK(802BD620);
                t = U16_(b, 0x68 + g2) + 1;
                U16_(b, 0x68 + g2) = t;
                fall = (s32)((u32)t * (u32)t * (u32)-30000);
                h = -(s32)((u32)U16_(B_SECTION(b, 0x3C), g2) << 16);
                R(rT8, U32_(model, 0x3C));
                if (!(h < fall)) {
                    /* it has landed */
                    ENGINE_BLK(802BD678);
                    U16_(b, 0x48 + g2) = 0;
                    R(rA3, gi);
                    func_802BFD1C(b, gi);
                    ENGINE_BLK(802BD684);
                    D_802E8BE4 = 10;
                    D_802E8BE8 = 300;
                    B_DAMAGE(b)[gi] = 100;
                    func_802BD85C(b, gi);
                    ENGINE_BLK(802BD6B8);
                    R(rA3, g2);
                }
                ENGINE_BLK(802BD6BC);
                mtx = func_802BE574(b, g2, fall);
                ENGINE_BLK(802BD6C4);
                d0[0] = 0x01040040;     /* its matrix */
                d0[1] = mtx;
                d0 += 2;
            }
            ENGINE_BLK(802BD6D8);
            src = (u32 *)(model + U32_(grp, 0));
            src_end = (u32 *)(model + U32_(grp, 4));
            if (anim != anim_end) {
                ENGINE_BLK(802BD6EC);
                d0 = func_802BE228(d0, src_end, anim, anim_end, src, &src);
                ENGINE_BLK(802BD6F4);
            } else {
                for (;;) {
                    ENGINE_BLK(802BD6FC);
                    if (src == src_end)
                        break;
                    ENGINE_BLK(802BD704);
                    *(u64 *)d0 = *(u64 *)src;
                    src += 2;
                    d0 += 2;
                }
            }
            R(rT7, src_end);
            ENGINE_BLK(802BD718);
            if (falling != 0) {
                ENGINE_BLK(802BD720);
                d0[0] = 0xBD000000;
                d0[1] = 0;
                d0 += 2;
                d1[0] = 0x01040040;
                d1[1] = mtx;
                d1 += 2;
            }
            ENGINE_BLK(802BD744);
            src = (u32 *)(model + U32_(grp, 8));
            src_end = (u32 *)(model + U32_(grp, 0xC));
            if (anim != anim_end) {
                ENGINE_BLK(802BD758);
                d1 = func_802BE3C8(d1, src_end, anim, anim_end, src, &src);
                ENGINE_BLK(802BD760);
            } else {
                for (;;) {
                    ENGINE_BLK(802BD768);
                    if (src == src_end)
                        break;
                    ENGINE_BLK(802BD770);
                    *(u64 *)d1 = *(u64 *)src;
                    src += 2;
                    d1 += 2;
                }
            }
            R(rT7, src_end);
            ENGINE_BLK(802BD784);
            if (falling != 0) {
                ENGINE_BLK(802BD78C);
                d1[0] = 0xBD000000;
                d1[1] = 0;
                d1 += 2;
            }
            ENGINE_BLK(802BD79C);
            grp += 0x10;
            R(rT4, grp);
            if (first)
                break;
        passed:
            ENGINE_BLK(802BD7A4);
            first = 0;
            R(rT6, 0);
        }
    done:
        ENGINE_BLK(802BD7AC);
        d0[0] = 0xB8000000;
        d0[1] = 0;
        d1[1] = 0;
        d1[0] = 0xB8000000;
        d0 += 2;
        d1 += 2;
        R(rA3, B_MODEL(b));
        R(rT1, U16_(B_MODEL(b), 0xE));
        if (U16_(B_MODEL(b), 0xE) != 0) {
            ENGINE_BLK(802BD7D8);
            g0[0] = 0xBD000000;         /* the moving one's matrix popped */
            g0[1] = 0;
            g1[0] = 0xBD000000;
            g1[1] = 0;
            g0 += 2;
            g1 += 2;
        }
        ENGINE_BLK(802BD7FC);
    }
    ENGINE_BLK(802BD804);
    ENGINE_RESTORE();
    for (t = 1; t < 32; t++)
        if (rset >> t & 1)
            ENGINE_LEAVE(t, rv[t]);
    ENGINE_LEAVE(rA2, (u32)d0);
    ENGINE_LEAVE(rT0, (u32)d1);
    ENGINE_LEAVE(rV0, (u32)end);
    ENGINE_LEAVE(rV1, (u32)end);
    g0 = func_802C12E0(g0, g1, &g1);
    ENGINE_BLK(802BD80C);
    g0[0] = 0xB8000000;
    g0[1] = 0;
    g1[1] = 0;
    g1[0] = 0xB8000000;
    ENGINE_LEAVE(rAT, 0xB8000000);
    ENGINE_LEAVE(rA0, (u32)(g0 + 2));
    ENGINE_LEAVE(rA1, (u32)(g1 + 2));
#undef R
}

/* the pieces of group g + 1 that depend on another (group2) switched off */
REGS(v0, a3)
void func_802BD85C(Building *b, s32 g) {
    Piece *p = b->unk4, *end = b->unk8;

    ENGINE_BLK(802BD85C);
    g++;
    for (;;) {
        ENGINE_BLK(802BD880);
        if (p == end)
            break;
        ENGINE_BLK(802BD888);
        if (p->group != g) {
            p++;
            continue;
        }
        ENGINE_BLK(802BD894);
        if (p->group2 == 0) {
            p++;
            continue;
        }
        ENGINE_BLK(802BD8A0);
        p->active = 0;
        p++;
    }
    ENGINE_BLK(802BD8AC);
}

/* in level 0x32, whether func_80286090 says no (the target there can't be
   hit until then) */
REGS(-> t1)
s32 func_802BD8C8(void) {
    s32 r = 0;

    ENGINE_BLK(802BD8C8);
    ENGINE_LEAVE(1, 0x32);              /* ($at) */
    if (D_802E8BDC == 0x32) {
        ENGINE_BLK(802BD930);
        if (func_80286090(D_802E8BDC) != 0) {
            ENGINE_BLK(802BD938);
        } else {
            ENGINE_BLK(802BD938);
            ENGINE_BLK(802BD940);
            r = 1;
        }
    }
    ENGINE_BLK(802BD944);
    return r;
}

/* Building b moved by (dx, dy, dz) (<< 5): its model's heights, shadow,
   triangles, corners, effect records and group centres, and its pieces,
   whose planes are made again from their corners.  (The truck's code,
   679E0's func_802AC2A4, still translated, reads what it leaves in the
   registers.) */
REGS(v0, a2, a3, t0)
void func_802BD99C(Building *b, s32 dx, s32 dy, s32 dz) {
    u8 *s, *e;
    s32 sx = dx >> 5, sy = dy >> 5, sz = dz >> 5;
    s32 wx = dx << 11, wy = dy << 11, wz = dz << 11;
    s32 qx = dx >> 2, qy = dy >> 2, qz = dz >> 2;
    s32 n, last = 0;
    Piece *p, *pe;
    /* the last piece's registers, for the leaves */
    s32 x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0, x3 = 0, y3 = 0, z3 = 0;
    s32 ey = 0, ez3 = 0, ez2 = 0, ey3 = 0, ex3 = 0, ex2 = 0;
    s64 nx = 0, ny = 0, nz = 0, dd = 0, t8 = 0, s7;
    f32 f0 = 0.0f, f2 = 0.0f;
    u32 at = 0;
    s32 any = 0;

    ENGINE_BLK(802BD99C);
    s = B_SECTION(b, 0x40);
    e = B_SECTION(b, 0x44);
    for (;;) {
        ENGINE_BLK(802BD9D0);
        if (s == e)
            break;
        ENGINE_BLK(802BD9D8);
        last = S16_(s, 0) + sy;
        S16_(s, 0) = last;
        s += 2;
    }
    ENGINE_BLK(802BD9EC);
    s = B_SECTION(b, 0x20);
    S16_(s, 0) += sx;
    S16_(s, 2) += sz;
    S16_(s, 4) += sx;
    last = S16_(s, 6) + sz;
    S16_(s, 6) = last;
    s = B_SECTION(b, 0x24);
    e = B_SECTION(b, 0x28);
    for (;;) {
        ENGINE_BLK(802BDA34);
        if (s == e)
            break;
        ENGINE_BLK(802BDA3C);
        S16_(s, 0) += sx, S16_(s, 2) += sy, S16_(s, 4) += sz;
        S16_(s, 6) += sx, S16_(s, 8) += sy, S16_(s, 0xA) += sz;
        S16_(s, 0xC) += sx, S16_(s, 0xE) += sy;
        last = S16_(s, 0x10) + sz;
        S16_(s, 0x10) = last;
        s += 0x14;
    }
    ENGINE_BLK(802BDAB0);
    s = B_SECTION(b, 0x1C);
    for (n = 4;; n--) {
        ENGINE_BLK(802BDABC);
        if (n == 0)
            break;
        ENGINE_BLK(802BDAC4);
        S16_(s, 0) += sx, S16_(s, 2) += sy;
        last = S16_(s, 4) + sz;
        S16_(s, 4) = last;
        s += 6;
    }
    ENGINE_BLK(802BDAF4);
    s = B_SECTION(b, 0x30);
    e = B_SECTION(b, 0x34);
    for (;;) {
        ENGINE_BLK(802BDB04);
        if (s == e)
            break;
        ENGINE_BLK(802BDB0C);
        FX_W(s, 0) += wx, FX_W(s, 4) += wy, FX_W(s, 8) += wz;
        last = FX_W(s, 0x28) + wy;
        FX_W(s, 0x28) = last;
        s += 0x38;
    }
    ENGINE_BLK(802BDB44);
    s = B_SECTION(b, 0x34);
    e = B_SECTION(b, 0x38);
    for (;;) {
        ENGINE_BLK(802BDB54);
        if (s == e)
            break;
        ENGINE_BLK(802BDB5C);
        FX_W(s, 0) += wx, FX_W(s, 4) += wy, FX_W(s, 8) += wz;
        last = FX_W(s, 0x28) + wy;
        FX_W(s, 0x28) = last;
        s += 0x38;
    }
    ENGINE_BLK(802BDB94);
    s = B_SECTION(b, 0x2C);
    e = B_SECTION(b, 0x30);
    for (;;) {
        ENGINE_BLK(802BDBA4);
        if (s == e)
            break;
        ENGINE_BLK(802BDBAC);
        S16_(s, 0) += sx, S16_(s, 2) += sy;
        last = S16_(s, 4) + sz;
        S16_(s, 4) = last;
        s += 8;
    }
    ENGINE_BLK(802BDBD8);
    p = b->unk4;
    pe = b->unk8;
    for (;;) {
        s64 ax, ay, az;

        ENGINE_BLK(802BDBEC);
        if (p == pe)
            break;
        ENGINE_BLK(802BDBF4);
        any = 1;
        x1 = p->p[0][0] + qx, y1 = p->p[0][1] + qy, z1 = p->p[0][2] + qz;
        x2 = p->p[1][0] + qx, y2 = p->p[1][1] + qy, z2 = p->p[1][2] + qz;
        x3 = p->p[2][0] + qx, y3 = p->p[2][1] + qy, z3 = p->p[2][2] + qz;
        ey = y1 - y2;
        ez3 = z1 - z3;
        ez2 = z1 - z2;
        ey3 = y1 - y3;
        ex3 = x1 - x3;
        ex2 = x1 - x2;
        p->p[0][0] = x1, p->p[0][1] = y1, p->p[0][2] = z1;
        p->p[1][0] = x2, p->p[1][1] = y2, p->p[1][2] = z2;
        p->p[2][0] = x3, p->p[2][1] = y3, p->p[2][2] = z3;
        nx = (s64)ey * ez3 - (s64)ez2 * ey3;
        f0 = (f32)nx;
        p->nx = nx;
        f0 = f0 * f0;
        ny = (s64)ez2 * ex3 - (s64)ex2 * ez3;
        f2 = (f32)ny;
        p->ny = ny;
        f2 = f2 * f2;
        f0 = f0 + f2;
        nz = (s64)ex2 * ey3 - (s64)ey * ex3;
        f2 = (f32)nz;
        p->nz = nz;
        f2 = f2 * f2;
        f0 = f0 + f2;
        s7 = (s64)((u64)nx * (u64)(s64)x2);
        p->unk24 = f0;
        f0 = __builtin_sqrtf(f0);
        t8 = (s64)((u64)ny * (u64)(s64)y2);
        s7 = (s64)((u64)s7 + (u64)t8);
        t8 = (s64)((u64)nz * (u64)(s64)z2);
        s7 = (s64)((u64)s7 + (u64)t8);
        dd = (s64)(0 - (u64)s7);
        p->d = dd;
        p->unk20 = f0;
        ax = nx, ay = ny, az = nz;
        if (ax < 0) {
            ENGINE_BLK(802BDD3C);
            ax = -ax;
        }
        ENGINE_BLK(802BDD40);
        if (ay < 0) {
            ENGINE_BLK(802BDD48);
            ay = -ay;
        }
        ENGINE_BLK(802BDD4C);
        if (az < 0) {
            ENGINE_BLK(802BDD54);
            az = -az;
        }
        /* the axis it is flattest along: z if |nz| is the largest, then
           y, else x */
        ENGINE_BLK(802BDD58);
        nx = ax, ny = ay, nz = az;
        at = az < ay;
        if (!(az < ax)) {
            ENGINE_BLK(802BDD64);
            if (!(az < ay)) {
                ENGINE_BLK(802BDD6C);
                p->axis = 0;
                goto next;
            }
        }
        ENGINE_BLK(802BDD74);
        at = ay < az;
        if (!(ay < ax)) {
            ENGINE_BLK(802BDD80);
            if (!(ay < az)) {
                ENGINE_BLK(802BDD88);
                p->axis = 1;
                ny = 1;
                goto next;
            }
        }
        ENGINE_BLK(802BDD94);
        p->axis = 2;
        ny = 2;
    next:
        ENGINE_BLK(802BDD9C);
        p++;
    }
    ENGINE_BLK(802BDDA4);
    /* what it leaves in the registers */
    ENGINE_LEAVE(2, (u32)pe);           /* $v0, $t4: the pieces' end */
    ENGINE_LEAVE(12, (u32)pe);
    ENGINE_LEAVE(13, qx);               /* $t5, $t9, $fp */
    ENGINE_LEAVE(25, qy);
    ENGINE_LEAVE(30, qz);
    if (!any) {
        ENGINE_LEAVE(3, (u32)B_SECTION(b, 0x30));   /* $v1, $s0: the last list's end */
        ENGINE_LEAVE(16, (u32)B_SECTION(b, 0x30));
        ENGINE_LEAVE(4, last);          /* $a0: the last value it stored */
        ENGINE_LEAVE(9, sx);            /* $t1-$t3, $t6, $t7 */
        ENGINE_LEAVE(10, sy);
        ENGINE_LEAVE(11, sz);
        ENGINE_LEAVE(14, wy);
        ENGINE_LEAVE(15, wz);
    } else {
        ENGINE_LEAVE(1, at);
        ENGINE_LEAVE(3, x1);            /* the last piece's corners */
        ENGINE_LEAVE(4, y1);
        ENGINE_LEAVE(5, z1);
        ENGINE_LEAVE(6, x2);
        ENGINE_LEAVE(7, y2);
        ENGINE_LEAVE(8, z2);
        ENGINE_LEAVE(9, x3);
        ENGINE_LEAVE(10, y3);
        ENGINE_LEAVE(11, z3);
        ENGINE_LEAVE(14, ex2);          /* and their differences */
        ENGINE_LEAVE(15, ey);
        ENGINE_LEAVE(16, ez2);
        ENGINE_LEAVE(17, ex3);
        ENGINE_LEAVE(18, ey3);
        ENGINE_LEAVE(19, ez3);
        ENGINE_LEAVE64(20, nx);         /* |n|, $s5 the axis when it isn't z */
        ENGINE_LEAVE64(21, ny);
        ENGINE_LEAVE64(22, nz);
        ENGINE_LEAVE64(23, dd);
        ENGINE_LEAVE64(24, t8);
        ENGINE_LEAVE_F(0, f0);
        ENGINE_LEAVE_F(2, f2);
    }
}

/* the heading from building b to the camera (16.16 asin by quadrant, a
   12-bit angle) */
static s32 heading_to_camera(Building *b, s32 *h_) {
    s32 cx = (u32)D_803643F8 >> 11, cz = (u32)D_80364400 >> 11;
    s32 bx = b->x, bz = b->z, v, h;
    f32 d, q;

    d = (f32)(cx - bx) * (f32)(cx - bx);
    q = (f32)(cz - bz) * (f32)(cz - bz);
    d = __builtin_sqrtf(d + q);
    if (!(cx < bx)) {
        ENGINE_BLK(802BDEBC);
        if (!(cz < bz)) {
            ENGINE_BLK(802BDEC8);
            q = (f32)(cx - bx) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802BDEF8);
            h = (u32)h >> 4;
        } else {
            ENGINE_BLK(802BDF00);
            q = (f32)(bz - cz) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802BDF30);
            h = ((u32)h >> 4) + 0x400;
        }
    } else {
        ENGINE_BLK(802BDF3C);
        if (cz < bz) {
            ENGINE_BLK(802BDF48);
            q = (f32)(bx - cx) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802BDF78);
            h = ((u32)h >> 4) + 0x800;
        } else {
            ENGINE_BLK(802BDF84);
            q = (f32)(cz - bz) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802BDFB4);
            h = ((u32)h >> 4) + 0xC00;
        }
    }
    *h_ = h;
    return h;
}

/* Building b's animated textures stepped (the model's records, anim up to
   end: an id, a count of frames at 4, the current one at 5, a kind at 6,
   a word at 8, two halves at 0xC and 0xE, then the frames' addresses).
   Kind 0 runs through its frames, 1 picks one now and then, others show
   the frame for the camera's heading between the two halves.  The level's
   target building only while every target is down. */
REGS(v0, t1, t2)
void func_802BDDB4(Building *b, u8 *a, u8 *end) {
    s32 kind, lo, hi, n, t, h;

    ENGINE_BLK(802BDDB4);
    if (b->unk30 == 0x38) {
        ENGINE_BLK(802BDDF4);
        if (D_803643DB == 0) {
            ENGINE_BLK(802BDE04);
            if (D_80364AC1 == 0)
                goto loop;
        }
        ENGINE_BLK(802BDE14);
        if (D_803F7806 == 0)
            goto out;
        ENGINE_BLK(802BDE24);
        t = func_802BC7DC();
        ENGINE_BLK(802BDE2C);
        if (!t)
            goto out;
        ENGINE_BLK(802BDE34);
        t = func_802BC714();
        ENGINE_BLK(802BDE3C);
        if (!t)
            goto out;
    }
loop:
    for (;;) {
        ENGINE_BLK(802BDE44);
        if (a == end)
            break;
        ENGINE_BLK(802BDE4C);
        kind = U8_(a, 6);
        if (kind == 0) {
            ENGINE_BLK(802BE178);
            t = U16_(a, 0xE) + 1;
            n = U16_(a, 0xC);
            if (t == n) {
                ENGINE_BLK(802BE18C);
                lo = U8_(a, 5) + 1;
                if (lo == U8_(a, 4)) {
                    ENGINE_BLK(802BE1A0);
                    lo = 0;
                }
                ENGINE_BLK(802BE1A4);
                U8_(a, 5) = lo;
                t = 0;
            }
            ENGINE_BLK(802BE1AC);
            U16_(a, 0xE) = t;
            S32_(a, 8) = engine_divu((u32)t * 255, n);
            if (n == 0)
                ENGINE_BLK(802BE1DC);
        } else if (kind == 1) {
            ENGINE_BLK(802BDE58);
            ENGINE_BLK(802BE114);
            t = (u32)D_803649D8 >> 4;
            n = U16_(a, 0xC);
            if (n == 0)
                ENGINE_BLK(802BE140);
            ENGINE_BLK(802BE144);
            if (engine_remu(t, n) == 0) {
                ENGINE_BLK(802BE14C);
                if (U8_(a, 4) == 0)
                    ENGINE_BLK(802BE160);
                ENGINE_BLK(802BE164);
                U8_(a, 5) = engine_remu((u32)t >> 8, U8_(a, 4));
            }
        } else {
            ENGINE_BLK(802BDE58);
            ENGINE_BLK(802BDE60);
            heading_to_camera(b, &h);
            ENGINE_BLK(802BDFBC);
            lo = U16_(a, 0xC);
            hi = U16_(a, 0xE);
            if (!(hi < lo)) {
                ENGINE_BLK(802BDFD0);
                if (h < lo)
                    goto first;
                ENGINE_BLK(802BDFD8);
                if (hi < h)
                    goto final;
                ENGINE_BLK(802BDFE0);
                t = engine_divu((u32)(h - lo) * (U8_(a, 4) - 1) * 255, hi - lo);
                if (hi - lo == 0)
                    ENGINE_BLK(802BE028);
                ENGINE_BLK(802BE02C);
                U8_(a, 5) = (u32)t / 255;
                ENGINE_BLK(802BE048);
                S32_(a, 8) = (u32)t % 255;
            } else {
                ENGINE_BLK(802BE058);
                if (h < lo) {
                    ENGINE_BLK(802BE064);
                    if (hi < h)
                        goto first;
                }
                /* the range wraps */
                ENGINE_BLK(802BE06C);
                n = 0xFFF - lo;
                if (h < lo) {
                    ENGINE_BLK(802BE080);
                    t = n + h;
                } else {
                    ENGINE_BLK(802BE088);
                    t = h - lo;
                }
                ENGINE_BLK(802BE08C);
                t = engine_divu((u32)t * (U8_(a, 4) - 1) * 255, n + hi);
                if (n + hi == 0)
                    ENGINE_BLK(802BE0CC);
                ENGINE_BLK(802BE0D0);
                U8_(a, 5) = (u32)t / 255;
                ENGINE_BLK(802BE0F0);
                S32_(a, 8) = (u32)t % 255;
            }
            goto step;
        first:
            ENGINE_BLK(802BE0FC);
            U8_(a, 5) = 0;
            goto step;
        final:
            ENGINE_BLK(802BE104);
            U8_(a, 5) = U8_(a, 4) - 1;
        }
    step:
        ENGINE_BLK(802BE1E0);
        a += U8_(a, 4) * 4 + 0xC;
    }
out:
    ENGINE_BLK(802BE1F4);
}

/* A building's display list from src to end copied to dl, its animated
   textures' frames put in: a G_SETTIMG of one of them (the records anim
   to anim_end) gets its current frame, the next G_SETTIMG the frame after
   (while it has them), and the G_SETPRIMCOLOR after them the record's
   word at 8 (the blend between the two).  func_802BE228 and func_802BE3C8
   are this for the two lists, with their own blocks. */
typedef struct {
    Blk entry, loop, cmd, scan0, scan, cmp, skip, found, anim, cur, set, wrap, cmd2, copy2, sel, sel2,
        set2, cmd3, copy3, prim, still, still2, put, out;
} CopyBlks;

static u32 *copy_dl(const CopyBlks *k, u32 *dl, u32 *end, u8 *anim, u8 *anim_end, u32 *src, u32 **src_out) {
    u32 w0, w1, f;
    u8 *a;

    BLKT(&k->entry, 0);
    for (;;) {
        BLKT(&k->loop, 0);
        if (src == end)
            break;
        BLKT(&k->cmd, 0);
        w0 = src[0];
        dl[0] = w0;
        w1 = src[1];
        if ((w0 & 0xFF000000) >> 24 != 0xFD)
            goto put;
        /* a texture: one of the animated ones? */
        BLKT(&k->scan0, 0);
        for (a = anim;;) {
            BLKT(&k->scan, 0);
            if (a == anim_end)
                goto put;
            BLKT(&k->cmp, 0);
            if (w1 == U32_(a, 0))
                break;
            BLKT(&k->skip, 0);
            a += U8_(a, 4) * 4 + 0xC;
        }
        BLKT(&k->found, 0);
        if (U8_(a, 7) == 0) {
            /* a still one: its current frame, if not the first */
            BLKT(&k->still, 0);
            f = U8_(a, 5);
            if (f != 0) {
                BLKT(&k->still2, 0);
                a += f * 4;
                w1 = U32_(a, 0xC);
            }
            goto put;
        }
        BLKT(&k->anim, 0);
        f = U8_(a, 5);
        if (f != 0) {
            BLKT(&k->cur, 0);
            w1 = U32_(a + f * 4, 0xC);
        }
        BLKT(&k->set, 0);
        dl[1] = w1;
        f++;
        src += 2;
        dl += 2;
        if (f == U8_(a, 4)) {
            BLKT(&k->wrap, 0);
            f = 0;
        }
        /* the next texture: the frame after */
        for (;;) {
            BLKT(&k->cmd2, 0);
            w0 = src[0];
            dl[0] = w0;
            w1 = src[1];
            if ((w0 & 0xFF000000) >> 24 == 0xFD)
                break;
            BLKT(&k->copy2, 0);
            dl[1] = w1;
            src += 2;
            dl += 2;
        }
        BLKT(&k->sel, 0);
        if (f != 0) {
            BLKT(&k->sel2, 0);
            w1 = U32_(a + f * 4, 0xC);
        }
        BLKT(&k->set2, 0);
        dl[1] = w1;
        src += 2;
        dl += 2;
        /* the blend, at the primitive colour */
        for (;;) {
            BLKT(&k->cmd3, 0);
            w0 = src[0];
            if ((w0 & 0xFF000000) >> 24 == 0xFA)
                break;
            BLKT(&k->copy3, 0);
            dl[0] = w0;
            dl[1] = src[1];
            src += 2;
            dl += 2;
        }
        BLKT(&k->prim, 0);
        dl[0] = w0 | U32_(a, 8);
        dl[1] = src[1];
        src += 2;
        dl += 2;
        continue;
    put:
        BLKT(&k->put, 0);
        dl[1] = w1;
        src += 2;
        dl += 2;
    }
    BLKT(&k->out, 0);
    *src_out = src;
    return dl;
}

static const CopyBlks blks_802BE228 = {
    {ENGINE_BLK_802BE228}, {ENGINE_BLK_802BE24C}, {ENGINE_BLK_802BE254}, {ENGINE_BLK_802BE270},
    {ENGINE_BLK_802BE278}, {ENGINE_BLK_802BE280}, {ENGINE_BLK_802BE28C}, {ENGINE_BLK_802BE2A0},
    {ENGINE_BLK_802BE2AC}, {ENGINE_BLK_802BE2B8}, {ENGINE_BLK_802BE2C4}, {ENGINE_BLK_802BE2DC},
    {ENGINE_BLK_802BE2E0}, {ENGINE_BLK_802BE2FC}, {ENGINE_BLK_802BE30C}, {ENGINE_BLK_802BE314},
    {ENGINE_BLK_802BE320}, {ENGINE_BLK_802BE32C}, {ENGINE_BLK_802BE344}, {ENGINE_BLK_802BE35C},
    {ENGINE_BLK_802BE37C}, {ENGINE_BLK_802BE388}, {ENGINE_BLK_802BE394}, {ENGINE_BLK_802BE3A4},
};

static const CopyBlks blks_802BE3C8 = {
    {ENGINE_BLK_802BE3C8}, {ENGINE_BLK_802BE3F0}, {ENGINE_BLK_802BE3F8}, {ENGINE_BLK_802BE414},
    {ENGINE_BLK_802BE41C}, {ENGINE_BLK_802BE424}, {ENGINE_BLK_802BE430}, {ENGINE_BLK_802BE444},
    {ENGINE_BLK_802BE450}, {ENGINE_BLK_802BE460}, {ENGINE_BLK_802BE46C}, {ENGINE_BLK_802BE484},
    {ENGINE_BLK_802BE488}, {ENGINE_BLK_802BE4A4}, {ENGINE_BLK_802BE4B4}, {ENGINE_BLK_802BE4BC},
    {ENGINE_BLK_802BE4C8}, {ENGINE_BLK_802BE4D4}, {ENGINE_BLK_802BE4EC}, {ENGINE_BLK_802BE504},
    {ENGINE_BLK_802BE524}, {ENGINE_BLK_802BE530}, {ENGINE_BLK_802BE53C}, {ENGINE_BLK_802BE54C},
};

/* the first list (dl in $a2) */
REGS(a2, s0, t1, t2, t7 -> a2, t7)
u32 *func_802BE228(u32 *dl, u32 *end, u8 *anim, u8 *anim_end, u32 *src, u32 **src_out) {
    return copy_dl(&blks_802BE228, dl, end, anim, anim_end, src, src_out);
}

/* the second (dl in $t0) */
REGS(t0, s0, t1, t2, t7 -> t0, t7)
u32 *func_802BE3C8(u32 *dl, u32 *end, u8 *anim, u8 *anim_end, u32 *src, u32 **src_out) {
    return copy_dl(&blks_802BE3C8, dl, end, anim, anim_end, src, src_out);
}

/* The matrix of building b's falling group (g2 = its index * 2): turned
   about its centre by its rates times its fall time (the three halves at
   0x88, 0xA8, 0xC8 + g2 times 0x68 + g2, 12-bit) and lowered by `fall`;
   made at D_803F3964 (the next one), returned as its physical address. */
REGS(v0, a3, s6 -> s5)
u32 func_802BE574(Building *b, s32 g2, s32 fall) {
    s32 *m = (s32 *)D_803F3964;
    u32 t = U16_(b, 0x68 + g2);
    s32 ax, ay, az, cx, cy, cz;
    u8 *c;

    ENGINE_BLK(802BE574);
    D_803F3964 = (u8 *)m + 0x40;
    ax = (s32)(t * (u32)S16_(b, 0x88 + g2));
    if (ax < 0) {
        ENGINE_BLK(802BE5F8);
        ax += 0xFFF;
    }
    ENGINE_BLK(802BE600);
    ay = (s32)(t * U16_(b, 0xA8 + g2));
    if (ay < 0) {
        ENGINE_BLK(802BE618);
        ay += 0xFFF;
    }
    ENGINE_BLK(802BE620);
    az = (s32)(t * U16_(b, 0xC8 + g2));
    if (az < 0) {
        ENGINE_BLK(802BE638);
        az += 0xFFF;
    }
    ENGINE_BLK(802BE640);
    c = B_SECTION(b, 0x2C) + g2 * 4;
    cx = U16_(c, 0) << 16;
    cy = U16_(c, 2) << 16;
    cz = U16_(c, 4) << 16;
    func_802ACA60(-cx, -cy, -cz, m);
    ENGINE_BLK(802BE680);
    func_802ACBDC(ax, D_803F38D0);
    ENGINE_BLK(802BE690);
    func_802ACCCC(D_803F38D0, m);
    ENGINE_BLK(802BE69C);
    func_802ACAC4(ay, D_803F38D0);
    ENGINE_BLK(802BE6AC);
    func_802ACCCC(D_803F38D0, m);
    ENGINE_BLK(802BE6B8);
    func_802ACB50(az, D_803F38D0);
    ENGINE_BLK(802BE6C8);
    func_802ACCCC(D_803F38D0, m);
    ENGINE_BLK(802BE6D4);
    func_802ACA60(cx, cy, cz, D_803F38D0);
    ENGINE_BLK(802BE6EC);
    func_802ACCCC(D_803F38D0, m);
    ENGINE_BLK(802BE6F8);
    func_802ACA60(0, fall, 0, D_803F38D0);
    ENGINE_BLK(802BE710);
    func_802ACCCC(D_803F38D0, m);
    ENGINE_BLK(802BE71C);
    func_802AC8CC((u32 *)m);
    ENGINE_BLK(802BE724);
    return K0(m);
}

/* ---- a vehicle against the buildings --------------------------------- */

/* Each frame, from every vehicle module: the vehicle of type `type` (its
   VehicleState in $gp) against the buildings, by its sphere (its Solid,
   D_803A7300), each piece hit doing damage (func_802BEBB0); what happened
   into vs->unk9C..9E.  (The vehicles' code is still translated in places
   and reads the registers it leaves.) */
REGS(t8, gp)
void func_802BE77C(s32 type, VS *vs) {
    Solid *s = D_803A7300;
    Building *b, *end;
    s32 x, y, z, r;

    ENGINE_SAVE(G(rA2) | G(rA3) | G(rT0) | G(rT1) | G(rT2) | G(rT3) | G(rT4) | G(rT5) | G(rT7) | G(rS0) | G(rS2) |
                G(rT8));
    ENGINE_BLK(802BE77C);
    D_803F7802 = 0;
    D_803F7803 = 0;
    D_803F7807 = 0;
    D_803F77F4 = D_803F77F8;
    D_803F780F = 0;
    func_802BFEE4(vs);
    ENGINE_BLK(802BE7E8);
    func_802BE9F8();
    ENGINE_BLK(802BE7F0);
    for (;;) {
        ENGINE_BLK(802BE7F8);
        if (s->kind == type)
            break;
        s++;
    }
    ENGINE_BLK(802BE804);
    if (s->end != 0) {
        ENGINE_BLK(802BE810);
        x = s->x, y = s->y, z = s->z, r = s->r;
        ENGINE_LEAVE(3, y);             /* ($v1, $a0, $a1: the sphere, left) */
        ENGINE_LEAVE(4, z);
        ENGINE_LEAVE(5, r);
        end = D_803F7654;
        for (b = D_803F4030;; b++) {
            ENGINE_BLK(802BE834);
            if (b == end)
                break;
            ENGINE_BLK(802BE83C);
            if (func_8029CFA4(x, y, z, r, b->x, b->y, b->z, b->unkC)) {
                ENGINE_BLK(802BE850);
                ENGINE_BLK(802BE858);
                if (b->unk30 == 0x38) {
                    ENGINE_BLK(802BE868);
                    if (func_802BD8C8()) {
                        ENGINE_BLK(802BE870);
                        goto next;
                    }
                    ENGINE_BLK(802BE870);
                }
                ENGINE_BLK(802BE878);
                if (func_802BE944(b, type)) {
                    ENGINE_BLK(802BE880);
                    goto next;
                }
                ENGINE_BLK(802BE880);
                ENGINE_BLK(802BE888);
                func_802BEADC(b, x, y, z, r, type);
            } else {
                ENGINE_BLK(802BE850);
            }
        next:
            ENGINE_BLK(802BE890);
        }
    }
    ENGINE_BLK(802BE898);
    if (type != 0xFF) {
        ENGINE_BLK(802BE8A4);
        func_802BCC48();
    }
    ENGINE_BLK(802BE8AC);
    ENGINE_RESTORE();
    if (D_803A7410 != 0) {
        ENGINE_LEAVE(2, D_803A7410);    /* ($v0, $at as it leaves them) */
        goto turned;
    }
    ENGINE_BLK(802BE8C0);
    ENGINE_LEAVE(2, D_803A7412);
    ENGINE_LEAVE(1, 0xFFF);
    if (D_803A7412 != 0xFFF) {
    turned:
        ENGINE_BLK(802BE8D8);
        D_803A7425 = 1;
        ENGINE_LEAVE(1, HI(&D_803A7425));
    }
    ENGINE_BLK(802BE8E4);
    vs->unk9C = (s8)D_803F7802;
    vs->unk9D = (s8)D_803F7803;
    vs->unk9E = (s8)D_803F7807;
}

/* whether vehicle type `type` can't harm building b: its model is listed
   for this level (or any, 0xFFFF) with the type's bit (D_803059F0: level,
   model, types) */
REGS(t3, t8 -> t9)
s32 func_802BE944(Building *b, s32 type) {
    u32 *p = D_803059F0;
    s32 model = b->unk30, r = 0;

    ENGINE_BLK(802BE944);
    ENGINE_LEAVE(1, 0x38);
    if (model == 0x38)
        goto out;
    for (;;) {
        ENGINE_BLK(802BE980);
        p += 3;
        if (p[-1] == 0)
            break;
        ENGINE_BLK(802BE990);
        ENGINE_LEAVE(1, 0xFFFF);
        if (p[-3] != 0xFFFF) {
            ENGINE_BLK(802BE9A0);
            if (p[-3] != (u32)D_802E8BDC)
                continue;
        }
        ENGINE_BLK(802BE9A8);
        if (p[-2] != (u32)model) {
            ENGINE_BLK(802BE9B4);
            continue;
        }
        ENGINE_BLK(802BE9BC);
        if ((p[-1] & (1u << (type & 31))) == 0)
            continue;
        ENGINE_BLK(802BE9D4);
        r = 1;
        break;
    }
out:
    ENGINE_BLK(802BE9D8);
    ENGINE_LEAVE(25, r);
    return r;
}

/* the list of (building, group) pairs hit this frame emptied */
REGS()
void func_802BE9F8(void) {
    ENGINE_BLK(802BE9F8);
    D_803F3960 = D_803F3910;
}

/* (b, group) added to it */
REGS(t3, t9)
void func_802BEA30(s32 group, Building *b) {
    u8 *p = D_803F3960;

    ENGINE_BLK(802BEA30);
    U32_(p, 0) = (u32)b;
    U32_(p, 4) = group;
    D_803F3960 = p + 8;
}

/* whether (b, group) is in it */
REGS(t3, t9 -> t7)
s32 func_802BEA70(s32 group, Building *b) {
    u8 *p = D_803F3910, *end = D_803F3960;

    ENGINE_BLK(802BEA70);
    for (;;) {
        ENGINE_BLK(802BEA9C);
        if (p == end)
            break;
        ENGINE_BLK(802BEAA4);
        p += 8;
        if (U32_(p, -8) != (u32)b)
            continue;
        ENGINE_BLK(802BEAB4);
        if (U32_(p, -4) != (u32)group)
            continue;
        ENGINE_BLK(802BEAC0);
        ENGINE_BLK(802BEAC4);
        return 1;
    }
    ENGINE_BLK(802BEAC4);
    return 0;
}

/* the pieces of building b the vehicle's sphere hits, each hit with
   func_802BEBB0 */
REGS(t3, v0, v1, a0, a1, t8)
void func_802BEADC(Building *b, s32 x, s32 y, s32 z, s32 r, s32 type) {
    Piece *p, *end;
    s32 px, py, pz, v1, a0, a1, a2, a3, t0, t1, in;

    ENGINE_SAVE(G(rV0) | G(rV1) | G(rA0) | G(rA1) | G(rA2) | G(rT3) | G(rT4));
    ENGINE_BLK(802BEADC);
    x >>= 2;
    y >>= 2;
    z >>= 2;
    r >>= 2;
    p = b->unk4;
    end = b->unk8;
    for (;;) {
        ENGINE_BLK(802BEB1C);
        if (p == end)
            break;
        ENGINE_BLK(802BEB24);
        if (p->active == 0) {
            p++;
            continue;
        }
        ENGINE_BLK(802BEB30);
        in = func_8029C160(x, y, z, r, p, &px, &py, &pz);
        ENGINE_BLK(802BEB38);
        if (in) {
            ENGINE_BLK(802BEB40);
            in = func_8029C0DC(px, py, pz, p, &v1, &a0, &a1, &a2, &a3, &t0, &t1);
            ENGINE_BLK(802BEB48);
            in = func_8029BF64(in, v1, a0, a1, a2, a3, t0, t1);
            ENGINE_BLK(802BEB50);
            if (!in) {
                ENGINE_BLK(802BEB58);
                in = func_8029BD0C(x, y, z, r, p);
                ENGINE_BLK(802BEB60);
                if (!in) {
                    ENGINE_BLK(802BEB68);
                    in = func_8029BEE4(x, y, z, r, p);
                    ENGINE_BLK(802BEB70);
                }
            }
            if (in) {
                ENGINE_BLK(802BEB78);
                func_802BEBB0(p, type, b);
            }
        }
        ENGINE_BLK(802BEB80);
        p++;
    }
    ENGINE_BLK(802BEB88);
    ENGINE_RESTORE();
    ENGINE_LEAVE(13, z);                /* ($t5, $t6, $s0, $s1, $t9: it leaves them) */
    ENGINE_LEAVE(14, r);
    ENGINE_LEAVE(16, (u32)p);
    ENGINE_LEAVE(17, (u32)end);
    ENGINE_LEAVE(25, (u32)b);
}

/* us.v10's func_802BEBB0 lacks the check of D_803F7812 (802BED4C), and
   its blocks after it are named 0x10 lower */
#ifdef VERSION_US_V10
#define BLK_V10(v11, v10) ENGINE_BLK(v10)
#else
#define BLK_V10(v11, v10) ENGINE_BLK(v11)
#endif

/* Piece p of building b hit by vehicle type `type`: if a part of the
   type's (D_803A6B30) touches it, the group takes its damage (by the
   type's power and the model's strength, func_802BFF6C), once a frame,
   and what follows: the camera turned, the target's flags, the group
   destroyed at 100. */
REGS(s0, t8, t9)
void func_802BEBB0(Piece *p, s32 type, Building *b) {
    KindPart *k = D_803A6B30;
    s32 n = 0, g, in, d, dmg, px, py, pz, v1, a0, a1, a2, a3, t0, t1;
    s8 *c;
    u8 *pd;
    Piece *q, *end;

    ENGINE_SAVE(G(rT3) | G(rT4) | G(rT5) | G(rT6) | G(rT7) | G(rS0) | G(rS1) | G(rS2) | G(rS3) | G(rS4) | G(rGP) |
                G(rFP));
    ENGINE_BLK(802BEBB0);
    for (;;) {
        ENGINE_BLK(802BEBF4);
        if (k->end == -1) {
            BLK_V10(802BEF5C, 802BEF4C);
            goto out;
        }
        ENGINE_BLK(802BEC04);
        if (k->end == 0) {
            k++;
            continue;
        }
        ENGINE_BLK(802BEC0C);
        if (k->kind != type) {
            k++;
            continue;
        }
        ENGINE_BLK(802BEC18);
        n++;
        k++;
        in = func_8029C160(k[-1].x >> 2, k[-1].y >> 2, k[-1].z >> 2, k[-1].r >> 2, p, &px, &py, &pz);
        ENGINE_BLK(802BEC44);
        if (!in)
            continue;
        ENGINE_BLK(802BEC4C);
        in = func_8029C0DC(px, py, pz, p, &v1, &a0, &a1, &a2, &a3, &t0, &t1);
        ENGINE_BLK(802BEC54);
        in = func_8029BF64(in, v1, a0, a1, a2, a3, t0, t1);
        ENGINE_BLK(802BEC5C);
        if (in)
            break;
        ENGINE_BLK(802BEC64);
        in = func_8029BD0C(k[-1].x >> 2, k[-1].y >> 2, k[-1].z >> 2, k[-1].r >> 2, p);
        ENGINE_BLK(802BEC6C);
        if (in)
            break;
        ENGINE_BLK(802BEC74);
        in = func_8029BEE4(k[-1].x >> 2, k[-1].y >> 2, k[-1].z >> 2, k[-1].r >> 2, p);
        ENGINE_BLK(802BEC7C);
        if (in)
            break;
    }
    ENGINE_BLK(802BEC84);
    if (type != 0xFF) {
        ENGINE_BLK(802BEC90);
        D_803F77F8 = D_80358068;
    }
    ENGINE_BLK(802BECA0);
    if (type == 0xFF)
        goto hit;
    ENGINE_BLK(802BECAC);
    if (B_MODEL(b)[4] == 1)
        goto hit;
    ENGINE_BLK(802BECC0);
    if (b->unkEB != 0)
        goto hit;
    ENGINE_BLK(802BECCC);
    if (b->unk30 == 0x38)
        goto hit;
    ENGINE_BLK(802BECDC);
    if (D_8036B974 == b)
        goto hit;
    ENGINE_BLK(802BECEC);
    D_8036B971 = 1;
    D_8036B974 = b;
hit:
    ENGINE_BLK(802BED00);
    func_802BEF9C(b);
    ENGINE_BLK(802BED08);
    if (D_803BE738 != 0)
        goto out;
    ENGINE_BLK(802BED18);
    k--;
    g = p->group;
    func_802BCCD4(n);
    ENGINE_BLK(802BED24);
    if (func_802BEA70(g, b)) {
        ENGINE_BLK(802BED2C);
        goto turn;
    }
    ENGINE_BLK(802BED2C);
    ENGINE_BLK(802BED34);
    if (D_803F7800 != 0)
        goto add;
    ENGINE_BLK(802BED44);
    if (type == 0xFF)
        goto add;
#ifndef VERSION_US_V10
    ENGINE_BLK(802BED4C);
    if (D_803F7812 != 0)
        goto seen;
#endif
    BLK_V10(802BED5C, 802BED4C);
    func_802BEFF4();
    BLK_V10(802BED64, 802BED54);
    if (D_803F780C != 0)
        goto add;
#ifndef VERSION_US_V10
seen:
#endif
    BLK_V10(802BED74, 802BED64);
    if (func_802BCDE0(n)) {
        BLK_V10(802BED7C, 802BED6C);
        goto turn;
    }
    BLK_V10(802BED7C, 802BED6C);
add:
    BLK_V10(802BED84, 802BED74);
    func_802BEA30(g, b);
    BLK_V10(802BED8C, 802BED7C);
    if (b->unk30 == 0x38) {
        BLK_V10(802BED9C, 802BED8C);
        if (type == 0) {
            BLK_V10(802BEDA4, 802BED94);
            D_803F7805 = 1;
        }
        BLK_V10(802BEDB0, 802BEDA0);
        d = 0;
    } else {
        BLK_V10(802BEDB8, 802BEDA8);
        d = func_802BFF6C(g, k->power, type, b, B_MODEL(b)[4], n);
    }
    BLK_V10(802BEDC8, 802BEDB8);
    D_802E8BE4 = 10;
    if (d < 0x1F5) {
        BLK_V10(802BEDE8, 802BEDD8);
        D_802E8BE8 = d;
    } else {
        BLK_V10(802BEDF0, 802BEDE0);
        D_802E8BE8 = 500;
    }
    BLK_V10(802BEDF8, 802BEDE8);
    pd = &B_DAMAGE(b)[g - 1];
    dmg = *pd + d;
    if (!(dmg < 100)) {
        BLK_V10(802BEE18, 802BEE08);
        dmg = 100;
    }
    BLK_V10(802BEE1C, 802BEE0C);
    *pd = dmg;
    D_803F77FE = d;
    func_802BF898(g, dmg, b);
    BLK_V10(802BEE2C, 802BEE1C);
    if (dmg != 100)
        goto turn;
    BLK_V10(802BEE38, 802BEE28);
    func_802BF1F0(g, b);
    BLK_V10(802BEE40, 802BEE30);
    func_802C1438(g, b);
    BLK_V10(802BEE48, 802BEE38);
    func_802C09B8(g, b);
    BLK_V10(802BEE50, 802BEE40);
    func_802C0E8C(g, b);
    BLK_V10(802BEE58, 802BEE48);
    func_802BF384(b);
    BLK_V10(802BEE60, 802BEE50);
    D_802E8BE4 = 15;
    D_802E8BE8 = 400;
    q = b->unk4;
    end = b->unk8;
    for (;;) {
        BLK_V10(802BEE88, 802BEE78);
        if (q == end)
            break;
        BLK_V10(802BEE90, 802BEE80);
        if (q->group == g) {
            BLK_V10(802BEE9C, 802BEE8C);
            q->active = 0;
        }
        BLK_V10(802BEEA0, 802BEE90);
        if (q->group2 == g) {
            BLK_V10(802BEEAC, 802BEE9C);
            if (B_DAMAGE(b)[q->group - 1] != 100) {
                BLK_V10(802BEEC8, 802BEEB8);
                q->active = 1;
            }
        }
        BLK_V10(802BEED0, 802BEEC0);
        q++;
    }
turn:
    /* the camera turned by the piece, for the kinds listed (D_803A7408) */
    BLK_V10(802BEED8, 802BEEC8);
    D_803F7803 = 1;
    c = D_803A7408;
    for (;;) {
        BLK_V10(802BEEF0, 802BEEE0);
        if (*c == n)
            goto done;
        BLK_V10(802BEEFC, 802BEEEC);
        if (*c++ < 0)
            break;
    }
    BLK_V10(802BEF04, 802BEEF4);
    if (B_DAMAGE(b)[g - 1] != 100) {
        BLK_V10(802BEF20, 802BEF10);
        D_803A7424 = 1;
        func_802BF264(p);
        BLK_V10(802BEF30, 802BEF20);
    } else {
        BLK_V10(802BEF38, 802BEF28);
        D_803F7802 = 1;
    }
done:
    BLK_V10(802BEF44, 802BEF34);
    func_802BF668(b);
    BLK_V10(802BEF4C, 802BEF3C);
    func_802BF534(b);
    BLK_V10(802BEF54, 802BEF44);
out:
    BLK_V10(802BEF60, 802BEF50);
    ENGINE_RESTORE();
}

/* level 0x2D's buildings 0xE7-0xE9 end it when hit */
REGS(t9)
void func_802BEF9C(Building *b) {
    ENGINE_BLK(802BEF9C);
    ENGINE_LEAVE(1, 0x2D);
    if (D_802E8BDC == 0x2D) {
        ENGINE_BLK(802BEFBC);
        ENGINE_LEAVE(1, b->unk30 < 0xE7);
        if (!(b->unk30 < 0xE7)) {
            ENGINE_LEAVE(1, b->unk30 < 0xEA);
            ENGINE_BLK(802BEFCC);
            if (b->unk30 < 0xEA) {
                ENGINE_BLK(802BEFD4);
                D_803BE738 = 1;
                ENGINE_LEAVE(1, HI(&D_803BE738));
            }
        }
    }
    ENGINE_BLK(802BEFE0);
}

/* The pushing of the vehicles that can (not types 0-2, 6, 7, 9, 0xB,
   0x10-0x12): pushed for 20 frames in a row in one direction (the sign of
   D_803F77FC), steering the same way all along (D_80370C2C), D_803F780C
   is set for a frame. */
REGS()
void func_802BEFF4(void) {
    static const u8 types[] = { 1, 2, 6, 7, 9, 0xB, 0x10, 0x11, 0x12 };
    s32 type = D_80364456, dir, steer, k, cnt;

    ENGINE_BLK(802BEFF4);
    if (type == 0)
        goto out;
    for (k = 0; k < 9; k++) {
        switch (k) {
        case 0: ENGINE_BLK(802BF018); break;
        case 1: ENGINE_BLK(802BF024); break;
        case 2: ENGINE_BLK(802BF02C); break;
        case 3: ENGINE_BLK(802BF034); break;
        case 4: ENGINE_BLK(802BF03C); break;
        case 5: ENGINE_BLK(802BF044); break;
        case 6: ENGINE_BLK(802BF04C); break;
        case 7: ENGINE_BLK(802BF054); break;
        case 8: ENGINE_BLK(802BF05C); break;
        }
        if (type == types[k])
            goto out;
    }
    ENGINE_BLK(802BF064);
    dir = 0;
    if (D_803F77FC < 0) {
        ENGINE_BLK(802BF088);
        dir = -1;
    } else {
        ENGINE_BLK(802BF078);
        if (D_803F77FC != 0) {
            ENGINE_BLK(802BF080);
            dir = 1;
        }
    }
    ENGINE_BLK(802BF08C);
    if (D_803F780F != 0)
        goto out;
    ENGINE_BLK(802BF09C);
    D_803F780F = 1;
    if (D_803F77F4 + 1 != D_80358068)
        goto reset;
    ENGINE_BLK(802BF0C4);
    if (dir != 0) {
        ENGINE_BLK(802BF0CC);
        if (D_803F780A == dir)
            goto reset;
    }
    ENGINE_BLK(802BF0DC);
    cnt = D_803F780B;
    if (cnt == 0) {
        ENGINE_BLK(802BF0EC);
        steer = 0;
        if ((u8)D_80370C2C != 0) {
            ENGINE_BLK(802BF100);
            ENGINE_BLK(802BF108);
            steer = 1;
        } else {
            ENGINE_BLK(802BF100);
        }
        ENGINE_BLK(802BF114);
        D_803F780E = steer;
        D_803F780D = 0;
    } else {
        ENGINE_BLK(802BF128);
        steer = 0;
        if ((u8)D_80370C2C == 0) {
            ENGINE_BLK(802BF13C);
            goto changed;
        }
        ENGINE_BLK(802BF13C);
        ENGINE_BLK(802BF144);
        steer = 1;
        ENGINE_BLK(802BF150);
        if (steer != D_803F780E) {
        changed:
            ENGINE_BLK(802BF160);
            D_803F780D = 1;
        }
    }
    ENGINE_BLK(802BF16C);
    cnt++;
    D_803F780B = cnt;
    D_803F780A = dir;
    D_803F780C = 0;
    if (cnt != 0x14)
        goto out;
    ENGINE_BLK(802BF194);
    D_803F780B = 0;
    if (D_803F780D == 0)
        goto out;
    ENGINE_BLK(802BF1AC);
    D_803F780C = 1;
    goto out;
reset:
    ENGINE_BLK(802BF1BC);
    D_803F780B = 0;
    D_803F780A = dir;
    D_803F780C = 0;
out:
    ENGINE_BLK(802BF1D4);
}

/* the triangles of group `group` (the model's 0x24 section) marked gone */
REGS(t3, t9)
void func_802BF1F0(s32 group, Building *b) {
    u8 *t = B_SECTION(b, 0x24), *end = B_SECTION(b, 0x28);

    ENGINE_BLK(802BF1F0);
    for (;;) {
        ENGINE_BLK(802BF224);
        if (t == end)
            break;
        ENGINE_BLK(802BF22C);
        if (U8_(t, 0x12) != group) {
            t += 0x14;
            continue;
        }
        ENGINE_BLK(802BF238);
        U8_(t, 0x13) = 1;
        t += 0x14;
    }
    ENGINE_BLK(802BF244);
}

/* a piece that pushes the camera (0x58): the camera kept within a
   quarter turn of its side facing the player (0x4C), unless the piece
   only works one way (0x55, 0x56) */
REGS(s0)
void func_802BF264(Piece *p) {
    s64 d;
    s32 h = p->heading, a, c;

    ENGINE_SAVE(G(rV0) | G(rV1) | G(rA0) | G(rA1) | G(rA2) | G(rA3) | G(rT0));
    ENGINE_BLK(802BF264);
    if (p->pushes == 0)
        goto out;
    ENGINE_BLK(802BF294);
    ENGINE_LEAVE(1, 1);
    d = (s64)((u64)p->nx * (u64)(s64)(D_803A73F0 >> 2) + (u64)p->ny * (u64)(s64)(D_803A73F4 >> 2) +
              (u64)p->nz * (u64)(s64)(D_803A73F8 >> 2) + (u64)p->d);
    if (p->d > 0) {
        ENGINE_BLK(802BF310);
        if (d > 0)
            goto behind;
    } else {
        ENGINE_BLK(802BF300);
        if (d < 0)
            goto behind;
        ENGINE_BLK(802BF308);
    }
    ENGINE_BLK(802BF318);
    a = h + 0x400;
    c = h - 0x400;
    if (p->unk55 == 1)
        goto push;
    ENGINE_BLK(802BF324);
    if (p->unk56 == 0)
        goto push;
    ENGINE_BLK(802BF330);
    goto out;
behind:
    ENGINE_BLK(802BF338);
    a = h - 0x400;
    c = h + 0x400;
    if (p->unk55 == 1)
        goto push;
    ENGINE_BLK(802BF348);
    if (p->unk56 == 0)
        goto out;
push:
    ENGINE_BLK(802BF354);
    func_8029B7CC(a, c);
out:
    ENGINE_BLK(802BF35C);
    ENGINE_RESTORE();
}

/* the first destruction of one of building b's groups: its effect
   (func_80264CB4) */
REGS(t9)
void func_802BF384(Building *b) {
    s32 n = b->unkE9, gone = 0;
    u8 *d = B_DAMAGE(b);

    ENGINE_BLK(802BF384);
    for (;;) {
        ENGINE_BLK(802BF3A8);
        if (n == 0)
            break;
        ENGINE_BLK(802BF3B0);
        n--;
        if (*d++ != 100)
            continue;
        ENGINE_BLK(802BF3C4);
        gone++;
    }
    ENGINE_BLK(802BF3CC);
    if (gone == 1) {
        ENGINE_BLK(802BF3D8);
        func_80264CB4((u32)b->x >> 5, (u32)b->y >> 5, (u32)b->z >> 5, (u32)b->unkC >> 5, b->unkE8, B_MODEL(b)[6]);
        ENGINE_BLK(802BF494);
    }
    ENGINE_BLK(802BF518);
}

/* Building b destroyed, when every group but the main one (model 0xD) is
   gone or falling: counted (D_80364A40, D_803649F0: the model's value)
   and the lights on it (D_803BDFD8, by its index + 1) off. */
REGS(t9)
void func_802BF534(Building *b) {
    s32 main, k = 0, n, idx;
    u8 *d, *e;
    u16 *f;
    u32 v;

    ENGINE_BLK(802BF534);
    if (b->unkEA != 0)
        goto out;
    ENGINE_BLK(802BF560);
    main = B_MODEL(b)[0xD];
    n = b->unkE9;
    d = B_DAMAGE(b);
    f = &U16_(b, 0x48);
    for (;;) {
        ENGINE_BLK(802BF578);
        if (n == 0)
            break;
        ENGINE_BLK(802BF580);
        k++;
        n--;
        if (main != k) {
            ENGINE_BLK(802BF58C);
            if (*d != 100) {
                ENGINE_BLK(802BF59C);
                if (*f == 0)
                    goto out;
            }
        }
        ENGINE_BLK(802BF5A8);
        d++;
        f++;
    }
    ENGINE_BLK(802BF5B4);
    b->unkEA = 1;
    v = U32_(B_MODEL(b), 8);
    D_80364A40 = v;
    D_803649F0 += v;
    idx = (u32)((u8 *)b - (u8 *)D_803F4030) / 0xFC;
    ENGINE_BLK(802BF618);
    idx++;
    d = D_803BDFD8;
    e = D_803BDFD4;
    for (;;) {
        ENGINE_BLK(802BF624);
        if (d == e)
            break;
        ENGINE_BLK(802BF62C);
        if (U8_(d, 0x11) != idx) {
            d += 0x24;
            continue;
        }
        ENGINE_BLK(802BF638);
        U8_(d, 0x12) = 0;
        d += 0x24;
    }
out:
    ENGINE_BLK(802BF644);
}

/* The groups that rest on others (the model's 0x44 section: a group, a
   strength, and (group, weight) pairs): one whose remaining support falls
   under its strength starts falling (0x48 + 2g; its rates from
   D_803649E0..E4), its triangles go (func_802BF1F0) and its pieces with
   it; again until none does. */
REGS(t9)
void func_802BF668(Building *b) {
    u8 *r, *end, *s;
    s32 again, g, n, sum, q;
    Piece *p, *pe;

    ENGINE_BLK(802BF668);
    do {
        ENGINE_BLK(802BF6A4);
        again = 0;
        r = B_SECTION(b, 0x44);
        end = B_SECTION(b, 0x48);
        for (;;) {
            ENGINE_BLK(802BF6C4);
            if (r == end)
                break;
            ENGINE_BLK(802BF6CC);
            g = r[0] - 1;
            if (U16_(b, 0x48 + g * 2) == 0) {
                ENGINE_BLK(802BF6EC);
                if (B_DAMAGE(b)[g] != 100)
                    goto weigh;
            }
            ENGINE_BLK(802BF704);
            r += r[2] * 2 + 3;
            continue;
        weigh:
            ENGINE_BLK(802BF718);
            n = r[2];
            s = r + 3;
            sum = 0;
            for (;;) {
                ENGINE_BLK(802BF724);
                if (n == 0)
                    break;
                ENGINE_BLK(802BF72C);
                q = s[0];
                n--;
                if (B_DAMAGE(b)[q - 1] != 100) {
                    ENGINE_BLK(802BF748);
                    if (U16_(b, 0x48 + (q - 1) * 2) == 0) {
                        ENGINE_BLK(802BF764);
                        sum += s[1];
                    }
                }
                ENGINE_BLK(802BF76C);
                s += 2;
            }
            ENGINE_BLK(802BF774);
            if (sum < r[1]) {
                ENGINE_BLK(802BF784);
                again = 1;
                func_802BF1F0(r[0], b);
                ENGINE_BLK(802BF790);
                g = r[0] - 1;
                U16_(b, 0x48 + g * 2) = 1;
                U16_(b, 0x88 + g * 2) = D_803649E0;
                U16_(b, 0xA8 + g * 2) = D_803649E2;
                U16_(b, 0xC8 + g * 2) = D_803649E4;
                g++;
                p = b->unk4;
                pe = b->unk8;
                for (;;) {
                    ENGINE_BLK(802BF7F8);
                    if (p == pe)
                        break;
                    ENGINE_BLK(802BF800);
                    if (p->group == g) {
                        ENGINE_BLK(802BF80C);
                        p->active = 0;
                    }
                    ENGINE_BLK(802BF810);
                    if (p->group2 == g) {
                        ENGINE_BLK(802BF81C);
                        if (B_DAMAGE(b)[p->group - 1] != 100) {
                            ENGINE_BLK(802BF838);
                            p->active = 1;
                        }
                    }
                    ENGINE_BLK(802BF840);
                    p++;
                }
            }
            ENGINE_BLK(802BF848);
            r = s;
        }
        ENGINE_BLK(802BF850);
    } while (again);
    ENGINE_BLK(802BF858);
}

/* The effects of a hit on group `group` (now at `damage`): with a model
   that has debris of its own (byte 5) func_802BF978's; else its effect
   records (sections 0x30..0x34) for the group fire once, those marked 0xFF
   only for a strong enough hit (D_803F77FE) and always. */
REGS(t3, t5, t9)
void func_802BF898(s32 group, s32 damage, Building *b) {
    u8 *model = B_MODEL(b), *fx, *end;
    s32 t;

    ENGINE_BLK(802BF898);
    if (model[5] != 0) {
        ENGINE_BLK(802BF954);
        func_802BF978(model, group, damage, b);
        goto out;
    }
    ENGINE_BLK(802BF8C0);
    fx = B_SECTION(b, 0x30);
    end = B_SECTION(b, 0x34);
    for (;;) {
        ENGINE_BLK(802BF8D0);
        if (fx == end)
            break;
        ENGINE_BLK(802BF8D8);
        if (FX_B(fx, 0x34) != 0)
            goto next;
        ENGINE_BLK(802BF8E4);
        if (FX_B(fx, 0x32) != group)
            goto next;
        ENGINE_BLK(802BF8F0);
        t = FX_B(fx, 0x33);
        if (t == 0xFF) {
            ENGINE_BLK(802BF900);
            if (D_803F77FE < 0x14)
                goto next;
            ENGINE_BLK(802BF914);
            ENGINE_BLK(802BF944);
            func_802C04F0(fx);
            goto next;
        }
        ENGINE_BLK(802BF91C);
        if (damage < t)
            goto next;
        ENGINE_BLK(802BF928);
        func_802BFBF4(fx, b);
        ENGINE_BLK(802BF930);
        func_802C04F0(fx);
        ENGINE_BLK(802BF938);
        FX_B(fx, 0x34) = 1;
    next:
        ENGINE_BLK(802BF94C);
        fx += 0x38;
    }
out:
    ENGINE_BLK(802BF95C);
}

/* Debris thrown up from group `group`'s centre: how many by a roll of
   D_803063E0's (chance, count) pairs; each of a kind from the chance
   table for the damage (D_80306344, or D_80306350 once destroyed; a hit
   under 20 makes none), its speed by the model's byte 5 (doubled with
   D_803EF6FF); and their sound. */
REGS(v0, t3, t5, t9)
void func_802BF978(u8 *model, s32 group, s32 damage, Building *b) {
    u8 *c = B_SECTION(b, 0x2C) + (group - 1) * 8, *t, *fx = (u8 *)(u32)model[5];
    s32 x = S16_(c, 0) << 16, y = S16_(c, 2) << 16, z = S16_(c, 4) << 16;
    s32 roll, n, k, sp;

    ENGINE_BLK(802BF978);
    roll = func_802BFB50(0, 100);
    ENGINE_BLK(802BF9EC);
    t = D_803063E0;
    for (;;) {
        ENGINE_BLK(802BF9F4);
        if (!(t[0] < roll))
            break;
        ENGINE_BLK(802BFA04);
        t += 2;
    }
    ENGINE_BLK(802BFA0C);
    n = t[1];
    for (k = 0;; k++) {
        ENGINE_BLK(802BFA14);
        if (k == n)
            break;
        ENGINE_BLK(802BFA1C);
        if (!(damage < 100)) {
            ENGINE_BLK(802BFA24);
            t = D_80306350;
        } else {
            ENGINE_BLK(802BFA30);
            t = D_80306344;
            if (D_803F77FE < 0x14)
                goto out;
        }
        ENGINE_BLK(802BFA4C);
        roll = func_802BFB50(0, 100);
        for (;;) {
            ENGINE_BLK(802BFA58);
            if (roll < NE_BE16(U16_(t, 0)))
                break;
            ENGINE_BLK(802BFA68);
            t += 0xC;
        }
        ENGINE_BLK(802BFA70);
        fx = D_803F3FF8;
        FX_W(fx, 0) = x;
        FX_W(fx, 4) = y;
        FX_W(fx, 8) = z;
        roll = func_802BFB50(NE_BE32(U32_(t, 4)), NE_BE32(U32_(t, 8)));
        ENGINE_BLK(802BFA90);
        sp = (s32)((u32)model[5] * (u32)roll);
        if (D_803EF6FF != 0) {
            ENGINE_BLK(802BFAAC);
            sp <<= 1;
        }
        ENGINE_BLK(802BFAB0);
        FX_W(fx, 0xC) = sp;
        FX_W(fx, 0x10) = 0;
        FX_W(fx, 0x14) = 0;
        FX_W(fx, 0x18) = 0;
        FX_W(fx, 0x1C) = 0;
        FX_W(fx, 0x20) = 0;
        FX_W(fx, 0x24) = 0;
        FX_W(fx, 0x28) = 0xFC180000;
        if (k != 0) {
            ENGINE_BLK(802BFAD8);
            FX_H(fx, 0x2C) = 10;
        } else {
            ENGINE_BLK(802BFAE4);
            FX_H(fx, 0x2C) = 0;
        }
        ENGINE_BLK(802BFAE8);
        FX_B(fx, 0x2E) = 0;
        FX_B(fx, 0x30) = 0;
        FX_B(fx, 0x34) = 0;
        FX_B(fx, 0x35) = 0;
        FX_B(fx, 0x31) = NE_BE16(U16_(t, 2));
        func_802C04F0(fx);
        ENGINE_BLK(802BFB04);
    }
    ENGINE_BLK(802BFB0C);
    func_802BFBF4(fx, b);
out:
    ENGINE_BLK(802BFB14);
}

/* a random number from lo up to hi (23C20.c's) */
REGS(a0, a1 -> s4)
s32 func_802BFB50(s32 lo, s32 hi) {
    s32 r;

    ENGINE_BLK(802BFB50);
    r = func_8026A828(lo, hi);
    ENGINE_BLK(802BFBA0);
    return r;
}

/* effect fx's sound, for the kinds D_8030633C lists (not on models of
   kind 1): one of four, at its place */
REGS(v1, t9)
void func_802BFBF4(u8 *fx, Building *b) {
    u8 *k;
    s32 kind;

    ENGINE_BLK(802BFBF4);
    if (B_MODEL(b)[4] == 1)
        goto out;
    ENGINE_BLK(802BFC54);
    k = D_8030633C;
    kind = FX_B(fx, 0x31);
    for (;;) {
        ENGINE_BLK(802BFC64);
        if ((s8)*k == -1)
            goto out;
        ENGINE_BLK(802BFC70);
        if ((s8)*k++ == kind)
            break;
    }
    ENGINE_BLK(802BFC78);
    ENGINE_BLK(802BFC9C);
    func_80288284(((u32)D_803649D8 >> 8) % 4, (u32)FX_W(fx, 0) >> 11, (u32)FX_W(fx, 4) >> 11,
                  (u32)FX_W(fx, 8) >> 11, b->y);
    ENGINE_BLK(802BFCC8);
out:
    ENGINE_BLK(802BFCCC);
}

/* a group of building b (g, from 0) landed: its effect records for it
   (section 0x34) fire, or with a model that has debris of its own,
   func_802BFDAC's */
REGS(v0, a3)
void func_802BFD1C(Building *b, s32 g) {
    u8 *fx, *end;

    ENGINE_BLK(802BFD1C);
    g++;
    if (B_MODEL(b)[5] != 0) {
        ENGINE_BLK(802BFD84);
        func_802BFDAC(b, g);
        goto out;
    }
    ENGINE_BLK(802BFD4C);
    fx = B_SECTION(b, 0x34);
    end = B_SECTION(b, 0x38);
    for (;;) {
        ENGINE_BLK(802BFD60);
        if (fx == end)
            break;
        ENGINE_BLK(802BFD68);
        if (FX_B(fx, 0x32) == g) {
            ENGINE_BLK(802BFD74);
            func_802C04F0(fx);
        }
        ENGINE_BLK(802BFD7C);
        fx += 0x38;
    }
out:
    ENGINE_BLK(802BFD8C);
}

/* a piece of debris where group `group` landed, of a kind from
   D_803063D4's chances */
REGS(v0, a3)
void func_802BFDAC(Building *b, s32 group) {
    u8 *model = B_MODEL(b), *c = B_SECTION(b, 0x2C) + (group - 1) * 8, *t, *fx;
    s32 x = S16_(c, 0), y = b->y, z = S16_(c, 4), roll;

    ENGINE_BLK(802BFDAC);
    roll = func_802BFB50(0, 100);
    ENGINE_BLK(802BFE10);
    t = D_803063D4;
    for (;;) {
        ENGINE_BLK(802BFE18);
        if (roll < NE_BE16(U16_(t, 0)))
            break;
        ENGINE_BLK(802BFE28);
        t += 0xC;
    }
    ENGINE_BLK(802BFE30);
    fx = D_803F3FF8;
    FX_W(fx, 0) = x << 16;
    FX_W(fx, 4) = y << 11;
    FX_W(fx, 8) = z << 16;
    roll = func_802BFB50(NE_BE32(U32_(t, 4)), NE_BE32(U32_(t, 8)));
    ENGINE_BLK(802BFE5C);
    FX_W(fx, 0x10) = 0;
    FX_W(fx, 0x14) = 0;
    FX_W(fx, 0x18) = 0;
    FX_W(fx, 0x1C) = 0;
    FX_W(fx, 0x20) = 0;
    FX_W(fx, 0x24) = 0;
    FX_H(fx, 0x2C) = 0;
    FX_B(fx, 0x2E) = 0;
    FX_B(fx, 0x30) = 0;
    FX_W(fx, 0xC) = (s32)((u32)model[5] * (u32)roll);
    FX_W(fx, 0x28) = 0xFC180000;
    FX_B(fx, 0x34) = 0;
    FX_B(fx, 0x35) = 0;
    FX_B(fx, 0x31) = NE_BE16(U16_(t, 2));
    func_802C04F0(fx);
    ENGINE_BLK(802BFEAC);
}

/* D_803F7800: whether the vehicle (vs) is dashing or so (D_803F7801, or
   one of its three flags at 0x96 is 1) */
REGS(gp)
void func_802BFEE4(VS *vs) {
    s32 m = D_803F7801;

    ENGINE_BLK(802BFEE4);
    if (m == 1)
        goto no;
    ENGINE_BLK(802BFF0C);
    if (m == 2)
        goto yes;
    ENGINE_BLK(802BFF18);
    if (vs->unk96[0] == 1)
        goto yes;
    ENGINE_BLK(802BFF28);
    if (vs->unk96[1] == 1)
        goto yes;
    ENGINE_BLK(802BFF38);
    if (vs->unk96[2] == 1)
        goto yes;
no:
    ENGINE_BLK(802BFF48);
    D_803F7800 = 0;
    goto out;
yes:
    ENGINE_BLK(802BFF50);
    D_803F7800 = 1;
out:
    ENGINE_BLK(802BFF58);
}

/* The damage vehicle type `type` (power `power`, its n-th part) does to
   group `group` of building b (strength `strength`): none to the main
   group (which counts as the target hit, D_803F7807); 100 when the
   target's conditions say (func_802C0284) or while dashing; else by
   D_80305E50's rules for the type and part: a factor, the push
   (D_803F77FC), the hole state (func_802C038C) or a vehicle part's state
   (func_802A04BC); over the strength.  The type 0xFF (an explosion) 100
   at strength 1, else none.  Into D_8036CB2A (before) and D_8036CB2C. */
REGS(t3, t7, t8, t9, gp, fp -> t7)
s32 func_802BFF6C(s32 group, s32 power, s32 type, Building *b, s32 strength, s32 n) {
    u8 *r;
    s32 sel = group, k, t, s, f11, f12, f14, fC, fE, f13;
    u32 d = power;
    f32 f4;

    ENGINE_BLK(802BFF6C);
    if (type == 0xFF) {
        ENGINE_BLK(802BFFC4);
        if (strength == 1) {
            ENGINE_BLK(802BFFD8);
            d = 100;
            goto out;
        }
        ENGINE_BLK(802BFFD0);
        d = 0;
        goto out;
    }
    ENGINE_BLK(802BFFAC);
    D_8036CB2F = 1;
    D_8036CB2E = type;
    ENGINE_BLK(802BFFE0);
    if (B_MODEL(b)[0xD] == group) {
        ENGINE_BLK(802BFFF0);
        D_803F7807 = 1;
        D_8036CB2F = 0;
        d = 0;
        goto out;
    }
    ENGINE_BLK(802C000C);
    t = func_802C0284(d, type, strength, &k);
    d = k;
    ENGINE_BLK(802C0014);
    if (t != 0)
        goto out;
    ENGINE_BLK(802C001C);
    if (D_803F7800 != 0) {
        ENGINE_BLK(802C0030);
        D_8036CB2A = 100;
        D_8036CB2C = 100;
        d = 100;
        goto out;
    }
    ENGINE_BLK(802C0048);
    r = D_80305E50;
    for (;;) {
        ENGINE_BLK(802C005C);
        if (r[0] != type)
            goto skip;
        ENGINE_BLK(802C0068);
        if (r[1] != n)
            goto skip;
        ENGINE_BLK(802C0074);
        t = r[2];
        if (t == 0xFC) {
            ENGINE_BLK(802C0084);
            if (D_803F7804 == 0)
                goto skip;
            ENGINE_BLK(802C0094);
            ENGINE_BLK(802C01C0);
            d = r[3] * d;
            goto divide;
        }
        ENGINE_BLK(802C009C);
        if (t == 0xFB) {
            ENGINE_BLK(802C00A8);
            if (!func_802C038C(sel, b)) {
                ENGINE_BLK(802C00B0);
                goto skip;
            }
            ENGINE_BLK(802C00B0);
            ENGINE_BLK(802C00B8);
            ENGINE_BLK(802C01D4);
            d = r[3] * d;
            goto divide;
        }
        ENGINE_BLK(802C00C0);
        if (t == 0xFF) {
            ENGINE_BLK(802C00CC);
            if (D_803F77FC < 0)
                goto skip;
            ENGINE_BLK(802C00E0);
            goto push;
        }
        ENGINE_BLK(802C00E8);
        if (t == 0xFE) {
            ENGINE_BLK(802C00F4);
            if (D_803F77FC > 0)
                goto skip;
            ENGINE_BLK(802C0108);
            goto push;
        }
        ENGINE_BLK(802C0110);
        if (t == 0xFD)
            goto push;
        /* the state of a part of the vehicle's (D_803F77D0) */
        ENGINE_BLK(802C011C);
        sel = t;
        s = func_802A04BC(t, D_803F77D0, &f11, &f12, &f14, &fC, &fE, &f13, &f4);
        ENGINE_BLK(802C0128);
        if (s != 0) {
            f32 f2;

            ENGINE_BLK(802C0130);
            r += f13;
            f2 = (f32)(r[5] - r[4]);
            s = engine_cvt_w_s(f4 * f2);
            ENGINE_LEAVE_FW(0, s);      /* ($f0, $f2) */
            ENGINE_LEAVE_F(2, f2);
            d = (u32)(s + r[4]) * d;
            goto divide;
        }
        ENGINE_LEAVE_F(0, f4);          /* (func_802A04BC's) */
        goto next;
    skip:
        ENGINE_BLK(802C0170);
        t = r[2];
        if (t == 0xFC || t == 0xFB) {
            if (t == 0xFB)
                ENGINE_BLK(802C0180);
            ENGINE_BLK(802C01B8);
            r += 4;
            continue;
        }
        ENGINE_BLK(802C0180);
        ENGINE_BLK(802C0188);
        if (t == 0xFD)
            goto four;
        ENGINE_BLK(802C0190);
        if (t == 0xFF)
            goto four;
        ENGINE_BLK(802C0198);
        if (t == 0xFE)
            goto four;
    next:
        ENGINE_BLK(802C01A0);
        r += r[3] + 4;
        continue;
    four:
        ENGINE_BLK(802C01B0);
        r += 4;
    }
push:
    ENGINE_BLK(802C01E8);
    k = D_803F77FC;
    if (k < 0) {
        ENGINE_BLK(802C01FC);
        k = -k;
    }
    ENGINE_BLK(802C0200);
    d = ((u32)k * d * r[3]) >> 4;
divide:
    ENGINE_BLK(802C0224);
    D_8036CB2A = d;
    d = engine_divu(d, strength);
    if (strength == 0)
        ENGINE_BLK(802C0244);
    ENGINE_BLK(802C0248);
    D_8036CB2C = d;
out:
    ENGINE_BLK(802C024C);
    return d;
}

/* whether the target condition D_803F780C (the push) or this level's
   rules (D_80305E10: level, types) or the types 1, 10 and 16 at strength
   1 make the hit 100 (*power_out 100), else *power_out stays `power` */
REGS(t7, t8, gp -> t2, t7)
s32 func_802C0284(s32 power, s32 type, s32 strength, s32 *power_out) {
    u32 *p;
    s32 r = 0;

    ENGINE_BLK(802C0284);
    if (D_803F780C != 0) {
        ENGINE_BLK(802C02A8);
        D_803F780C = 0;
        ENGINE_BLK(802C0374);
        *power_out = 100;
        return 1;
    }
    ENGINE_BLK(802C02BC);
    if (D_803A740C + 1 == D_80358068) {
        ENGINE_BLK(802C02D8);
        p = D_80305E10;
        for (;;) {
            ENGINE_BLK(802C02E0);
            if (p[1] == 0)
                break;
            ENGINE_BLK(802C02EC);
            if ((p[1] & (1u << (type & 31))) == 0) {
                p += 2;
                continue;
            }
            ENGINE_BLK(802C0300);
            p += 2;
            if (p[-2] != (u32)D_802E8BDC)
                continue;
            ENGINE_BLK(802C0318);
            r = 1;
            power = 100;
            break;
        }
    }
    ENGINE_BLK(802C0320);
    if (type == 1) {
        ENGINE_BLK(802C032C);
        if (strength == 1) {
            ENGINE_BLK(802C0334);
            r = 1;
            power = 100;
        }
    }
    ENGINE_BLK(802C033C);
    if (type == 0xA) {
        ENGINE_BLK(802C0348);
        if (strength == 1) {
            ENGINE_BLK(802C0350);
            r = 1;
            power = 100;
        }
    }
    ENGINE_BLK(802C0358);
    if (type == 0x10) {
        ENGINE_BLK(802C0364);
        if (strength == 1) {
            ENGINE_BLK(802C036C);
            r = 1;
            power = 100;
        }
    }
    ENGINE_BLK(802C0374);
    *power_out = power;
    return r;
}

/* whether group `group` of building b can be hit from below: every group
   it rests on (the 0x44 section) that lists it is destroyed, and its
   height (section 0x40) is under the player's + 0x578 */
REGS(v0, t9 -> t3)
s32 func_802C038C(s32 group, Building *b) {
    u8 *r = B_SECTION(b, 0x44), *end = B_SECTION(b, 0x48), *s;
    s32 n, k;

    ENGINE_BLK(802C038C);
    for (;;) {
        ENGINE_BLK(802C03D4);
        if (r == end)
            break;
        ENGINE_BLK(802C03DC);
        n = r[2];
        s = r + 3;
        k = n;
        for (;;) {
            ENGINE_BLK(802C03E8);
            if (k == 0)
                break;
            ENGINE_BLK(802C03F0);
            k--;
            if (s[0] == group) {
                ENGINE_BLK(802C0408);
                if (B_DAMAGE(b)[r[0] - 1] == 100)
                    break;
                ENGINE_BLK(802C0420);
                ENGINE_BLK(802C046C);
                return 0;
            }
            ENGINE_BLK(802C0400);
            s += 2;
        }
        ENGINE_BLK(802C0428);
        r += n * 2 + 3;
    }
    ENGINE_BLK(802C0438);
    if (D_803A73F4 + 0x578 < S16_(B_SECTION(b, 0x40), group * 2 - 2) << 5) {
        ENGINE_BLK(802C0468);
        ENGINE_BLK(802C046C);
        return 0;
    }
    ENGINE_BLK(802C046C);
    return 1;
}

/* ---- the effects ----------------------------------------------------- */

/* all the effects free */
REGS()
void func_802C049C(void) {
    s32 n;
    u8 *d = D_803F3968[0];

    ENGINE_BLK(802C049C);
    for (n = 30;; n--) {
        ENGINE_BLK(802C04C0);
        if (n == 0)
            break;
        ENGINE_BLK(802C04C8);
        FX_B(d, 0x34) = 1;
        d += 0x38;
    }
    ENGINE_BLK(802C04D8);
}

/* effect record `src` copied into the first free one (none if none) */
REGS(v1)
void func_802C04F0(u8 *src) {
    s32 n;
    u8 *d = D_803F3968[0];

    ENGINE_BLK(802C04F0);
    for (n = 30;; n--) {
        ENGINE_BLK(802C0514);
        if (n == 0)
            goto out;
        ENGINE_BLK(802C051C);
        if (FX_B(d, 0x34) != 0)
            break;
        ENGINE_BLK(802C0528);
        d += 0x38;
    }
    ENGINE_BLK(802C0534);
    for (n = 0x38;; n -= 4) {
        ENGINE_BLK(802C0538);
        if (n == 0)
            break;
        ENGINE_BLK(802C0540);
        *(u32 *)d = *(u32 *)src;
        src += 4;
        d += 4;
    }
out:
    ENGINE_BLK(802C0558);
}

/* Each frame (hd.c): each waiting effect counts down (0x2C) and then
   starts (60F60's func_802A6274: of a kind D_80306270 picks for its type
   at random, the "big" ones with a3 1), with its sound unless the game
   is ending; one with a radius (0x2E) does that much damage around
   (func_802C18D4). */
void func_802C0574(void) {
    u8 *fx = D_803F3968[0], *kinds;
    s32 n, t, k, big, started;

    ENGINE_BLK(802C0574);
    for (n = 30;; n--) {
        ENGINE_BLK(802C05A8);
        if (n == 0)
            break;
        ENGINE_BLK(802C05B0);
        if (FX_B(fx, 0x34) != 0)
            goto next;
        ENGINE_BLK(802C05C0);
        t = FX_H(fx, 0x2C);
        if (t != 0) {
            ENGINE_BLK(802C05CC);
            FX_H(fx, 0x2C) = t - 1;
            goto next;
        }
        ENGINE_BLK(802C05D8);
        kinds = (u8 *)(u32)U32_(D_80306270, FX_B(fx, 0x31) * 4);
        k = kinds[1 + engine_remu((u32)D_803649D8 >> 4, kinds[0])];
        if (kinds[0] == 0)
            ENGINE_BLK(802C0638);
        ENGINE_BLK(802C063C);
        big = 1;
        if (FX_B(fx, 0x31) != 0x15) {
            ENGINE_BLK(802C0674);
            if (D_803F7810 == 0) {
                ENGINE_BLK(802C0684);
                if (D_8036DCD4 == 0)
                    big = 0;
                else {
                    ENGINE_BLK(802C0694);
                    if (D_8036DCD7 != 1)
                        big = 0;
                }
            }
        }
        if (big) {
            ENGINE_BLK(802C06A8);
        } else {
            ENGINE_BLK(802C06F0);
        }
        started = func_802A6274(D_802C3FFC[k], FX_W(fx, 0xC), 0, FX_W(fx, 0), FX_W(fx, 4), FX_W(fx, 8),
                                FX_W(fx, 0x10), FX_W(fx, 0x14), FX_W(fx, 0x18), FX_W(fx, 0x1C), FX_W(fx, 0x20),
                                FX_W(fx, 0x24), FX_W(fx, 0x28), FX_B(fx, 0x35), big);
        if (big)
            ENGINE_BLK(802C06E8);
        ENGINE_BLK(802C0730);
        if (started != 0) {
            ENGINE_BLK(802C0738);
            if (D_80364A98 == 0) {
                ENGINE_BLK(802C0748);
                if (D_803643D6 == 0) {
                    ENGINE_BLK(802C0758);
                    func_802619D0(FX_B(fx, 0x31));
                    ENGINE_BLK(802C07E4);
                }
            }
        }
        ENGINE_BLK(802C0868);
        FX_B(fx, 0x34) = 1;
        t = FX_H(fx, 0x2E);
        if (t != 0) {
            ENGINE_BLK(802C0884);
            func_802C18D4(t, FX_W(fx, 0), FX_W(fx, 4), FX_W(fx, 8), FX_B(fx, 0x30) << 4);
        }
    next:
        ENGINE_BLK(802C0890);
        fx += 0x38;
    }
    ENGINE_BLK(802C0898);
}

/* The smoke clouds (D_803F0900[4]): each fading (0x4B0 down by 20) is
   drawn at that alpha; a gone one counts down 0x4B2 before it can be
   used again. */
REGS(v0)
void func_802C08C4(u32 *g) {
    u8 *s = D_803F0900[0];
    s32 n, a;

    ENGINE_BLK(802C08C4);
    for (n = 4;; s += 0x4B8) {
        ENGINE_BLK(802C08EC);
        if (n == 0)
            break;
        ENGINE_BLK(802C08F4);
        a = S16_(s, 0x4B0);
        n--;
        if (a == 0) {
            ENGINE_BLK(802C0904);
            a = U8_(s, 0x4B2);
            if (a != 0) {
                ENGINE_BLK(802C0910);
                U8_(s, 0x4B2) = a - 1;
            }
            ENGINE_BLK(802C0918);
            continue;
        }
        ENGINE_BLK(802C0920);
        a -= 0x14;
        if (!(a > 0)) {
            ENGINE_BLK(802C092C);
            S16_(s, 0x4B0) = 0;
            U8_(s, 0x4B2) = 3;
            continue;
        }
        ENGINE_BLK(802C0940);
        S16_(s, 0x4B0) = a;
        g[0] = 0xE7000000;              /* pipe sync */
        g[1] = 0;
        g[2] = 0xFB000000;              /* the environment colour: alpha */
        g[3] = a;
        g[4] = 0x06000000;              /* the cloud's list */
        g[5] = K0(s);
        g += 6;
    }
    ENGINE_BLK(802C0988);
    g[0] = 0xB8000000;
    g[1] = 0;
}

/* A smoke cloud of group `group` of building b (a model with no smoke of
   its own, byte 7), in the first free one: the group's lists copied
   (at most 0x94 commands; their textures through D_802F46C0), as
   func_802BD1F8 picks them. */
REGS(t3, t9)
void func_802C09B8(s32 group, Building *b) {
    u8 *model = B_MODEL(b), *s, *r, *end;
    u32 *d, *src, *src_end;
    s32 n, k, g, t, thr, dmg;

    ENGINE_BLK(802C09B8);
    if (model[7] != 0)
        goto out;
    ENGINE_BLK(802C0A48);
    s = D_803F0900[0];
    for (n = 4;;) {
        ENGINE_BLK(802C0A54);
        if (n == 0)
            goto out;
        ENGINE_BLK(802C0A5C);
        if (U16_(s, 0x4B0) == 0) {
            ENGINE_BLK(802C0A68);
            if (U8_(s, 0x4B2) == 0)
                break;
        }
        ENGINE_BLK(802C0A74);
        n--;
        s += 0x4B8;
    }
    ENGINE_BLK(802C0A80);
    d = (u32 *)s;
    d[0] = 0xBC002406;
    d[1] = K0(model + 0x50);
    d += 2;
    k = 0x94;
    r = B_SECTION(b, 0x38);
    end = B_SECTION(b, 0x3C);
    for (;;) {
        ENGINE_BLK(802C0AC0);
        if (r == end)
            break;
        ENGINE_BLK(802C0AC8);
        g = U16_(r, 4);
        n = U32_(r, 0);
        r += 4;
        if (g != group)
            goto skip;
        for (;;) {
            ENGINE_BLK(802C0ADC);
            if (n == 0)
                goto take;
            ENGINE_BLK(802C0AE4);
            g = U16_(r, 0);
            n--;
            t = U8_(r, 2);
            thr = U8_(r, 3);
            r += 4;
            if (g == group) {
                ENGINE_BLK(802C0AFC);
                dmg = 0x5A;
            } else {
                ENGINE_BLK(802C0B04);
                dmg = B_DAMAGE(b)[g - 1];
            }
            ENGINE_BLK(802C0B14);
            if (t != 0) {
                ENGINE_BLK(802C0B1C);
                if (!(dmg < thr))
                    continue;
                ENGINE_BLK(802C0B24);
            } else {
                ENGINE_BLK(802C0B2C);
                if (!(thr < dmg))
                    continue;
            }
            break;
        }
    skip:
        ENGINE_BLK(802C0B38);
        r += n * 4 + 0x10;
        continue;
    take:
        ENGINE_BLK(802C0B48);
        src = (u32 *)(model + U32_(r, 0));
        src_end = (u32 *)(model + U32_(r, 4));
        for (;;) {
            ENGINE_BLK(802C0B58);
            if (src == src_end)
                break;
            ENGINE_BLK(802C0B60);
            if (--k == 0)
                goto out;
            ENGINE_BLK(802C0B6C);
            *(u64 *)d = func_802C0C64(*(u64 *)src);
            ENGINE_BLK(802C0B74);
            src += 2;
            d += 2;
        }
        ENGINE_BLK(802C0B84);
        src = (u32 *)(model + U32_(r, 8));
        src_end = (u32 *)(model + U32_(r, 0xC));
        for (;;) {
            ENGINE_BLK(802C0B94);
            if (src == src_end)
                break;
            ENGINE_BLK(802C0B9C);
            if (--k == 0)
                goto out;
            ENGINE_BLK(802C0BA8);
            *(u64 *)d = func_802C0C64(*(u64 *)src);
            ENGINE_BLK(802C0BB0);
            src += 2;
            d += 2;
        }
        ENGINE_BLK(802C0BC0);
        r += 0x10;
    }
    ENGINE_BLK(802C0BC8);
    U16_(s, 0x4B0) = 0xFF;
    d[0] = 0xB8000000;
    d[1] = 0;
out:
    ENGINE_BLK(802C0BE0);
}

/* a display list command as a smoke cloud has it: D_802F46C0's pairs
   (twelve) replace some */
REGS(t4 -> t4)
u64 func_802C0C64(u64 cmd) {
    u64 *p = D_802F46C0;
    s32 n;

    ENGINE_BLK(802C0C64);
    for (n = 12;;) {
        ENGINE_BLK(802C0C84);
        if (n == 0)
            break;
        ENGINE_BLK(802C0C8C);
        n--;
        p += 2;
        if (p[-2] != cmd)
            continue;
        ENGINE_BLK(802C0CA0);
        cmd = p[-1];
        break;
    }
    ENGINE_BLK(802C0CA4);
    return cmd;
}

/* The dust (D_803EFED0, one cloud): rising by 16 more each frame while
   under its height (0xA20), shaken about (two rolls), drawn with this
   frame's matrix (0x960 or 0x9A0); then counted down (0xA2B) before it
   can be used again.  D_803F7810 while it rises. */
REGS(v0)
void func_802C0CBC(u32 *g) {
    u8 *s = D_803EFED0;
    s32 n, h, y, x, z;
    s32 *m;

    ENGINE_BLK(802C0CBC);
    D_803F7810 = 0;
    for (n = 1;; s += 0xA30) {
        ENGINE_BLK(802C0D08);
        if (n == 0)
            break;
        ENGINE_BLK(802C0D10);
        n--;
        if (U8_(s, 0xA2A) == 0) {
            ENGINE_BLK(802C0D20);
            if (U8_(s, 0xA2B) != 0) {
                ENGINE_BLK(802C0D2C);
                U8_(s, 0xA2B) -= 1;
            }
            ENGINE_BLK(802C0D34);
            continue;
        }
        ENGINE_BLK(802C0D3C);
        h = U16_(s, 0xA28) + 0x10;
        y = S32_(s, 0xA24) + h;
        U16_(s, 0xA28) = h;
        if (!(y < S32_(s, 0xA20))) {
            ENGINE_BLK(802C0D5C);
            U8_(s, 0xA2A) = 0;
            U8_(s, 0xA2B) = 3;
            continue;
        }
        ENGINE_BLK(802C0D70);
        S32_(s, 0xA24) = y;
        D_803F7810 = 1;
        x = func_802BFB50(-0x20, 0x20);
        ENGINE_BLK(802C0D8C);
        x <<= 12;
        z = func_802BFB50(-0x20, 0x20);
        ENGINE_BLK(802C0D9C);
        y = -(y << 11);
        z <<= 12;
        if (D_8035805C != 0) {
            ENGINE_BLK(802C0DB4);
            m = (s32 *)(s + 0x9A0);
        } else {
            ENGINE_BLK(802C0DBC);
            m = (s32 *)(s + 0x960);
        }
        ENGINE_BLK(802C0DC0);
        func_802ACA60(x, y, z, m);
        ENGINE_BLK(802C0DC8);
        func_802AC8CC((u32 *)m);
        ENGINE_BLK(802C0DD0);
        g[0] = 0xE7000000;
        g[1] = 0;
        g += 2;
        if (D_8035805C != 0) {
            ENGINE_BLK(802C0DF0);
            m = (s32 *)(s + 0x9A0);
        } else {
            ENGINE_BLK(802C0DF8);
            m = (s32 *)(s + 0x960);
        }
        ENGINE_BLK(802C0DFC);
        g[0] = 0xBC003006;              /* segment 12: the matrix */
        g[1] = K0(m);
        g[2] = 0x06000000;              /* the cloud */
        g[3] = K0(s);
        g += 4;
    }
    ENGINE_BLK(802C0E40);
    g[0] = 0xB8000000;
    g[1] = 0;
}

/* The dust of group `group` of building b (a model D_80305E38 lists; not
   in some game modes), if the cloud is free: its corners (func_802C1214),
   D_802F4780's six commands, the group's lists (at most 0x121 commands),
   from the group's height (section 0x40) to the building's. */
REGS(t3, t9)
void func_802C0E8C(s32 group, Building *b) {
    u8 *model = B_MODEL(b), *s, *r, *end;
    s16 *id;
    u32 *d, *src, *src_end;
    u64 *c;
    s32 n, k, g, t, thr, dmg;

    ENGINE_BLK(802C0E8C);
    if ((D_80364A90 & 0x440) != 0)
        goto out;
    ENGINE_BLK(802C0F1C);
    id = D_80305E38;
    for (;;) {
        ENGINE_BLK(802C0F28);
        if (b->unk30 == *id)
            break;
        ENGINE_BLK(802C0F34);
        if (*id < 0)
            goto out;
        ENGINE_BLK(802C0F3C);
        id++;
    }
    ENGINE_BLK(802C0F44);
    s = D_803EFED0;
    for (n = 1;;) {
        ENGINE_BLK(802C0F50);
        if (n == 0)
            goto out;
        ENGINE_BLK(802C0F58);
        if (U8_(s, 0xA2A) == 0) {
            ENGINE_BLK(802C0F64);
            if (U8_(s, 0xA2B) == 0)
                break;
        }
        ENGINE_BLK(802C0F70);
        n--;
        s += 0xA30;
    }
    ENGINE_BLK(802C0F7C);
    func_802C1214(s, b);
    ENGINE_BLK(802C0F84);
    d = (u32 *)s;
    d[0] = 0xBC002406;                  /* segment 9: the model's textures */
    d[1] = K0(model + 0x50);
    d[2] = 0xBC002C06;                  /* segment 11: the corners */
    d[3] = K0(s + 0x9E0);
    d += 4;
    c = D_802F4780;
    for (n = 6;; n--) {
        ENGINE_BLK(802C0FE0);
        if (n == 0)
            break;
        ENGINE_BLK(802C0FE8);
        *(u64 *)d = *c++;
        d += 2;
    }
    ENGINE_BLK(802C1000);
    d[0] = 0x01040040;                  /* segment 12's matrix */
    d[1] = 0x0C000000;
    d += 2;
    k = 0x121;
    r = B_SECTION(b, 0x38);
    end = B_SECTION(b, 0x3C);
    for (;;) {
        ENGINE_BLK(802C1030);
        if (r == end)
            break;
        ENGINE_BLK(802C1038);
        g = U16_(r, 4);
        n = U32_(r, 0);
        r += 4;
        if (g != group)
            goto skip;
        for (;;) {
            ENGINE_BLK(802C104C);
            if (n == 0)
                goto take;
            ENGINE_BLK(802C1054);
            g = U16_(r, 0);
            n--;
            t = U8_(r, 2);
            thr = U8_(r, 3);
            r += 4;
            if (g == group) {
                ENGINE_BLK(802C106C);
                dmg = 0x5A;
            } else {
                ENGINE_BLK(802C1074);
                dmg = B_DAMAGE(b)[g - 1];
            }
            ENGINE_BLK(802C1084);
            if (t != 0) {
                ENGINE_BLK(802C108C);
                if (!(dmg < thr))
                    continue;
                ENGINE_BLK(802C1094);
            } else {
                ENGINE_BLK(802C109C);
                if (!(thr < dmg))
                    continue;
            }
            break;
        }
    skip:
        ENGINE_BLK(802C10A8);
        r += n * 4 + 0x10;
        continue;
    take:
        ENGINE_BLK(802C10B8);
        src = (u32 *)(model + U32_(r, 0));
        src_end = (u32 *)(model + U32_(r, 4));
        for (;;) {
            ENGINE_BLK(802C10C8);
            if (src == src_end)
                break;
            ENGINE_BLK(802C10D0);
            if (--k == 0)
                goto out;
            ENGINE_BLK(802C10DC);
            *(u64 *)d = *(u64 *)src;
            src += 2;
            d += 2;
        }
        ENGINE_BLK(802C10F0);
        src = (u32 *)(model + U32_(r, 8));
        src_end = (u32 *)(model + U32_(r, 0xC));
        for (;;) {
            ENGINE_BLK(802C1100);
            if (src == src_end)
                break;
            ENGINE_BLK(802C1108);
            if (--k == 0)
                goto out;
            ENGINE_BLK(802C1114);
            *(u64 *)d = *(u64 *)src;
            src += 2;
            d += 2;
        }
        ENGINE_BLK(802C1128);
        r += 0x10;
    }
    ENGINE_BLK(802C1130);
    S32_(s, 0xA24) = 0;
    S32_(s, 0xA20) = (S16_(B_SECTION(b, 0x40), (group - 1) * 2) << 5) - b->y;
    U16_(s, 0xA28) = 0;
    U8_(s, 0xA2A) = 1;
    d[0] = 0xBD000000;
    d[1] = 0;
    d[2] = 0xB8000000;
    d[3] = 0;
out:
    ENGINE_BLK(802C1190);
}

/* the dust's four corners (at slot + 0x9E0): the model's shadow
   rectangle (section 0x20) 100 bigger, at the building's height */
REGS(v1, t9)
void func_802C1214(u8 *s, Building *b) {
    u8 *r = B_SECTION(b, 0x20);
    s16 *v = (s16 *)(s + 0x9E0);
    s32 y = b->y >> 5, x0 = S16_(r, 0) - 100, z0 = S16_(r, 2) - 100, x1 = S16_(r, 4) + 100,
        z1 = S16_(r, 6) + 100;

    ENGINE_BLK(802C1214);
    v[0x00] = x0, v[0x01] = y, v[0x02] = z0;
    v[0x08] = x1, v[0x09] = y, v[0x0A] = z0;
    v[0x10] = x0, v[0x11] = y, v[0x12] = z1;
    v[0x18] = x1, v[0x19] = y, v[0x1A] = z1;
}

/* The falling groups' shadows (D_803F1BE0[2]): each growing (0x46C by
   0x46E, which grows by 15, to 0x385; then 6 more frames) drawn with its
   matrix (func_8026A454, from D_803F7658 on), into both lists; then
   counted down (0x472) before it can be used again.  Returns g0 (and g1). */
REGS(a0, a1 -> a0, a1)
u32 *func_802C12E0(u32 *g0, u32 *g1, u32 **g1_out) {
    u8 *s = D_803F1BE0[0];
    Mtx *m = D_803F7658;
    s32 n, a, sz, gr;

    ENGINE_BLK(802C12E0);
    for (n = 2;; s += 0x478, m = (Mtx *)((u8 *)m + 0x40)) {
        ENGINE_BLK(802C1304);
        if (n == 0)
            break;
        ENGINE_BLK(802C130C);
        n--;
        if (U8_(s, 0x470) == 0) {
            ENGINE_BLK(802C131C);
            a = U8_(s, 0x472);
            if (a == 0)
                goto next;
            ENGINE_BLK(802C1328);
            U8_(s, 0x472) = a - 1;
            goto next;
        }
        ENGINE_BLK(802C1334);
        gr = U16_(s, 0x46E);
        sz = U16_(s, 0x46C) + gr;
        gr += 0xF;
        ENGINE_LEAVE(rA3, sz);
        if (sz < 0x385) {
            ENGINE_BLK(802C134C);
            U16_(s, 0x46E) = gr;
            U16_(s, 0x46C) = sz;
        } else {
            ENGINE_BLK(802C1358);
            a = U8_(s, 0x471) + 1;
            U8_(s, 0x471) = a;
            if (!(a < 6)) {
                ENGINE_BLK(802C140C);
                U8_(s, 0x470) = 0;
                U8_(s, 0x472) = 3;
                continue;
            }
        }
        ENGINE_BLK(802C136C);
        ENGINE_LEAVE(rA2, S16_(s, 0x468));
        ENGINE_LEAVE(rT1, 0x80000000);
        func_8026A454(S16_(s, 0x464), S16_(s, 0x466), S16_(s, 0x468), sz, U16_(s, 0x46A), m);
        ENGINE_BLK(802C138C);
        g0[0] = 0xBC002406;             /* segment 9: the model's textures */
        g0[1] = U32_(s, 0x460) - 0x80000000;
        g1[0] = 0xBC002406;
        g1[1] = U32_(s, 0x460) - 0x80000000;
        g0[2] = 0x06000000;
        g0[3] = K0(s);
        g1[2] = 0x06000000;
        g1[3] = K0(s + 0x230);
        g0 += 4;
        g1 += 4;
    next:
        ENGINE_BLK(802C1400);
    }
    ENGINE_BLK(802C1424);
    *g1_out = g1;
    return g0;
}

/* A shadow for group `group` of building b as it starts to fall (a model
   with shadows, byte 7; unless a lower group would hide it,
   func_802C1A28): the group's two lists copied (0x43 commands each at
   most) into the first free slot with their matrix (segment 2, one per
   slot), at the group's centre, facing the player. */
REGS(t3, t9)
void func_802C1438(s32 group, Building *b) {
    u8 *model = B_MODEL(b), *s, *r, *end, *c;
    u32 *d0, *d1, *src, *src_end, seg = 0x02000000;
    s32 n, k0, k1, g, t, thr, dmg, cx, cz, px, pz, v, h;
    f32 d, q;

    ENGINE_BLK(802C1438);
    if (model[7] == 0)
        goto out;
    ENGINE_BLK(802C14C8);
    t = func_802C1A28(group, b);
    ENGINE_BLK(802C14D0);
    if (t != 0)
        goto out;
    ENGINE_BLK(802C14D8);
    s = D_803F1BE0[0];
    for (n = 2;;) {
        ENGINE_BLK(802C14E8);
        if (n == 0)
            goto out;
        ENGINE_BLK(802C14F0);
        if (U8_(s, 0x470) == 0) {
            ENGINE_BLK(802C14FC);
            if (U8_(s, 0x472) == 0)
                break;
        }
        ENGINE_BLK(802C1508);
        n--;
        seg += 0x40;
        s += 0x478;
    }
    ENGINE_BLK(802C1518);
    U32_(s, 0x460) = (u32)(model + 0x50);
    d0 = (u32 *)s;
    d1 = (u32 *)(s + 0x230);
    d0[0] = 0x01040040;                 /* its matrix */
    d0[1] = seg;
    d1[1] = seg;
    d1[0] = 0x01040040;
    d0 += 2;
    d1 += 2;
    k0 = 0x43;
    k1 = 0x43;
    r = B_SECTION(b, 0x38);
    end = B_SECTION(b, 0x3C);
    for (;;) {
        ENGINE_BLK(802C1560);
        if (r == end)
            break;
        ENGINE_BLK(802C1568);
        g = U16_(r, 4);
        n = U32_(r, 0);
        r += 4;
        if (g != group)
            goto skip;
        for (;;) {
            ENGINE_BLK(802C157C);
            if (n == 0)
                goto take;
            ENGINE_BLK(802C1584);
            g = U16_(r, 0);
            n--;
            t = U8_(r, 2);
            thr = U8_(r, 3);
            r += 4;
            if (g == group) {
                ENGINE_BLK(802C159C);
                dmg = 0x5A;
            } else {
                ENGINE_BLK(802C15A4);
                dmg = B_DAMAGE(b)[g - 1];
            }
            ENGINE_BLK(802C15B4);
            if (t != 0) {
                ENGINE_BLK(802C15BC);
                if (!(dmg < thr))
                    continue;
                ENGINE_BLK(802C15C4);
            } else {
                ENGINE_BLK(802C15CC);
                if (!(thr < dmg))
                    continue;
            }
            break;
        }
    skip:
        ENGINE_BLK(802C15D8);
        r += n * 4 + 0x10;
        continue;
    take:
        ENGINE_BLK(802C15E8);
        src = (u32 *)(model + U32_(r, 0));
        src_end = (u32 *)(model + U32_(r, 4));
        for (;;) {
            ENGINE_BLK(802C15F8);
            if (src == src_end)
                break;
            ENGINE_BLK(802C1600);
            if (--k0 == 0)
                goto out;
            ENGINE_BLK(802C160C);
            *(u64 *)d0 = *(u64 *)src;
            src += 2;
            d0 += 2;
        }
        ENGINE_BLK(802C1620);
        src = (u32 *)(model + U32_(r, 8));
        src_end = (u32 *)(model + U32_(r, 0xC));
        for (;;) {
            ENGINE_BLK(802C1630);
            if (src == src_end)
                break;
            ENGINE_BLK(802C1638);
            if (--k1 == 0)
                goto out;
            ENGINE_BLK(802C1644);
            *(u64 *)d1 = *(u64 *)src;
            src += 2;
            d1 += 2;
        }
        ENGINE_BLK(802C1658);
        r += 0x10;
    }
    ENGINE_BLK(802C1660);
    d0[0] = 0xBD000000;
    d0[1] = 0;
    d0[2] = 0xB8000000;
    d0[3] = 0;
    d1[0] = 0xBD000000;
    d1[1] = 0;
    d1[2] = 0xB8000000;
    d1[3] = 0;
    U8_(s, 0x470) = 1;
    U16_(s, 0x46C) = 0;
    U16_(s, 0x46E) = 1;
    U8_(s, 0x471) = 0;
    S16_(s, 0x466) = b->y >> 5;
    c = B_SECTION(b, 0x2C) + (group - 1) * 8;
    cx = S16_(c, 0);
    S16_(s, 0x464) = cx;
    cz = S16_(c, 4);
    S16_(s, 0x468) = cz;
    px = D_803A73F0;
    pz = D_803A73F8;
    cx <<= 5;
    cz <<= 5;
    d = (f32)(px - cx) * (f32)(px - cx);
    q = (f32)(pz - cz) * (f32)(pz - cz);
    d = __builtin_sqrtf(d + q);
    if (!(px < cx)) {
        ENGINE_BLK(802C173C);
        if (!(pz < cz)) {
            ENGINE_BLK(802C1748);
            q = (f32)(px - cx) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802C1778);
            h = (u32)h >> 4;
        } else {
            ENGINE_BLK(802C1780);
            q = (f32)(cz - pz) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802C17B0);
            h = ((u32)h >> 4) + 0x400;
        }
    } else {
        ENGINE_BLK(802C17BC);
        if (pz < cz) {
            ENGINE_BLK(802C17C8);
            q = (f32)(cx - px) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802C17F8);
            h = ((u32)h >> 4) + 0x800;
        } else {
            ENGINE_BLK(802C1804);
            q = (f32)(pz - cz) / d;
            v = engine_cvt_w_s(65536.0f * q);
            ENGINE_LEAVE_FW(0, v);
            ENGINE_LEAVE_F(2, q);
            h = func_802AD7FC(v);
            ENGINE_BLK(802C1834);
            h = ((u32)h >> 4) + 0xC00;
        }
    }
    ENGINE_BLK(802C183C);
    h -= 0x400;
    if (h < 0) {
        ENGINE_BLK(802C1848);
        h += 0xFFF;
    }
    ENGINE_BLK(802C184C);
    U16_(s, 0x46A) = h;
out:
    ENGINE_BLK(802C1850);
}

/* An explosion of radius r (<< 5) at (x, y, z) (16.16): each building it
   reaches but the level's target takes amount (over its strength) in
   every intact group, after a while (func_802BC888). */
REGS(a1, t3, t4, t5, t6)
void func_802C18D4(s32 r, s32 x, s32 y, s32 z, s32 amount) {
    Building *b = D_803F4030, *end = D_803F7654;
    s32 cx = (u32)x >> 11, cy = (u32)y >> 11, cz = (u32)z >> 11, n, g, a;
    u8 *d;

    ENGINE_SAVE(G(rV0) | G(rV1) | G(rA0) | G(rA1) | G(rA2) | G(rA3) | G(rT0) | G(rT1) | G(rT2) | G(rT3) | G(rT4) |
                G(rT5) | G(rT6) | G(rT7) | G(rS0) | G(rT9));
    ENGINE_BLK(802C18D4);
    r <<= 5;
    for (;; b++) {
        ENGINE_BLK(802C1940);
        if (b == end)
            break;
        ENGINE_BLK(802C1948);
        if (!func_8029CFA4(cx, cy, cz, r, b->x, b->y, b->z, b->unkC)) {
            ENGINE_BLK(802C195C);
            goto next;
        }
        ENGINE_BLK(802C195C);
        ENGINE_BLK(802C1964);
        if (b->unk30 == 0x38)
            goto next;
        ENGINE_BLK(802C1974);
        n = b->unkE9;
        d = B_DAMAGE(b);
        for (g = 1;; g++, d++) {
            ENGINE_BLK(802C1980);
            if (n == 0)
                break;
            ENGINE_BLK(802C1988);
            n--;
            if (*d == 100) {
                ENGINE_BLK(802C19C8);
                continue;
            }
            ENGINE_BLK(802C199C);
            if (B_MODEL(b)[4] == 0)
                ENGINE_BLK(802C19B4);
            ENGINE_BLK(802C19B8);
            a = engine_divu(amount, B_MODEL(b)[4]);
            func_802BC888(a, g, b);
            ENGINE_BLK(802C19C8);
        }
    next:
        ENGINE_BLK(802C19D4);
    }
    ENGINE_BLK(802C19DC);
    ENGINE_RESTORE();
}

/* whether a group of building b's falls lower than group `group` (the
   fall heights, section 0x3C, as many as the model's first half): the
   count left when one is found, else 0 */
REGS(t3, t9 -> a2)
s32 func_802C1A28(s32 group, Building *b) {
    u8 *model = B_MODEL(b);
    s16 *f = (s16 *)B_SECTION(b, 0x3C);
    s32 n = U16_(model, 0), h = f[group - 1];

    ENGINE_BLK(802C1A28);
    for (;;) {
        ENGINE_BLK(802C1A60);
        if (n == 0)
            break;
        ENGINE_BLK(802C1A68);
        if (*f < h)
            break;
        ENGINE_BLK(802C1A78);
        f++;
        n--;
    }
    ENGINE_BLK(802C1A84);
    return n;
}

/* ---- the level's targets, for the game's C --------------------------- */

/* whether every target building (0xEB) is down by its group
   (func_802BD064) */
s32 func_802C1AA0(void) {
    Building *b = D_803F4030, *end = D_803F7654;

    ENGINE_BLK(802C1AA0);
    for (;;) {
        ENGINE_BLK(802C1AD0);
        if (b == end)
            break;
        ENGINE_BLK(802C1AD8);
        b++;
        if (b[-1].unkEB == 0)
            continue;
        ENGINE_BLK(802C1AE8);
        if (func_802BD064(b)) {
            ENGINE_BLK(802C1AF0);
            continue;
        }
        ENGINE_BLK(802C1AF0);
        ENGINE_BLK(802C1AF8);
        ENGINE_BLK(802C1AFC);
        return 0;
    }
    ENGINE_BLK(802C1AFC);
    return 1;
}

/* how many target buildings are down */
u8 func_802C1B1C(void) {
    Building *b = D_803F4030, *end = D_803F7654;
    s32 n = 0;

    ENGINE_BLK(802C1B1C);
    for (;;) {
        ENGINE_BLK(802C1B4C);
        if (b == end)
            break;
        ENGINE_BLK(802C1B54);
        b++;
        if (b[-1].unkEB == 0)
            continue;
        ENGINE_BLK(802C1B64);
        if (!func_802BD064(b)) {
            ENGINE_BLK(802C1B6C);
            continue;
        }
        ENGINE_BLK(802C1B6C);
        ENGINE_BLK(802C1B74);
        n++;
    }
    ENGINE_BLK(802C1B7C);
    return n;
}

/* The nearest target to the CMO (D_803EF6DC...), as func_802BCE40 finds
   the player's: its position in D_803F7670..78, its distance in
   D_803F7660 (and returned).  A target building ahead of it (in z) only
   within its radius. */
s32 func_802C1B9C(void) {
    Building *b = D_803F4030, *end = D_803F7654;
    s32 cx = D_803EF6DC, cy = D_803EF6E0, cz = D_803EF6E4, n;
    s64 best = 9999999, d;
    UnkStruct_8039C800 *h;
    u8 *o, *q;

    ENGINE_BLK(802C1B9C);
    for (;;) {
        ENGINE_BLK(802C1C04);
        if (b == end)
            break;
        ENGINE_BLK(802C1C0C);
        if (b->unkEB == 0) {
            b++;
            continue;
        }
        ENGINE_BLK(802C1C18);
        if (b->unkEA != 0) {
            b++;
            continue;
        }
        ENGINE_BLK(802C1C24);
        d = func_802ABCDC(b->x, b->y, b->z, cx, cy, cz);
        ENGINE_BLK(802C1C34);
        b++;
        if (!(d < best))
            continue;
        ENGINE_BLK(802C1C40);
        if (func_802BD064(b)) {
            ENGINE_BLK(802C1C48);
            continue;
        }
        ENGINE_BLK(802C1C48);
        ENGINE_BLK(802C1C50);
        if (b[-1].z < cz) {
            ENGINE_BLK(802C1C5C);
            if (b[-1].unkC < d)
                continue;
        }
        ENGINE_BLK(802C1C6C);
        D_803F7670 = b[-1].unk28;
        best = d;
        D_803F7674 = b[-1].y;
        D_803F7678 = b[-1].unk2C;
    }
    ENGINE_BLK(802C1C98);
    h = D_8039C800;
    for (n = D_8039C940;; n--, h++) {
        ENGINE_BLK(802C1CA8);
        if (n == 0)
            break;
        ENGINE_BLK(802C1CB0);
        if (h->unk26 == 0) {
            ENGINE_BLK(802C1CBC);
            d = func_802ABCDC(h->x, h->y, h->z, cx, cy, cz);
            ENGINE_BLK(802C1CCC);
            if (d < best) {
                ENGINE_BLK(802C1CD8);
                D_803F7670 = h->x;
                D_803F7678 = h->z;
                best = d;
            }
        }
        ENGINE_BLK(802C1CEC);
    }
    ENGINE_BLK(802C1CF8);
    o = D_80306480;
    for (;;) {
        ENGINE_BLK(802C1D00);
        if (S8_(o, 0x15) == -1)
            break;
        ENGINE_BLK(802C1D10);
        if (S8_(o, 0x15) != D_802E8BDC)
            goto next;
        ENGINE_BLK(802C1D20);
        d = func_802ABCDC(S32_(o, 0), S32_(o, 4), S32_(o, 8), cx, cy, cz);
        ENGINE_BLK(802C1D30);
        if (!(d < best))
            goto next;
        ENGINE_BLK(802C1D3C);
        if (S32_(o, 8) < cz)
            goto next;
        ENGINE_BLK(802C1D44);
        q = o + 0xC;
        for (;;) {
            ENGINE_BLK(802C1D48);
            if ((s8)*q == -1)
                break;
            ENGINE_BLK(802C1D58);
            if (func_8029D210((s8)*q)) {
                ENGINE_BLK(802C1D60);
                ENGINE_BLK(802C1D70);
                D_803F7670 = S32_(o, 0);
                D_803F7678 = S32_(o, 8);
                best = d;
                break;
            }
            ENGINE_BLK(802C1D60);
            ENGINE_BLK(802C1D68);
            q++;
        }
    next:
        ENGINE_BLK(802C1D90);
        o += 0x30;
    }
    ENGINE_BLK(802C1D98);
    D_803F7660 = (s32)best;
    return (s32)best;
}

/* The level's buildings counted (1D990.c and others): the ones of models
   not of kind 0xFF towards D_8036EB92, the destroyed ones (0xEA, or all
   with D_803F7688) towards D_8036EA70.bd and their value towards its ip;
   kind 1 counts for the value only.  With `targets`, only the target
   buildings (0xEB), the last one's model in D_803F7684. */
void func_802C1DD0(s32 targets) {
    Building *b = D_803F4030, *end = D_803F7654;
    s32 all = 0, down = 0, kind;
    u32 value = 0;
    u8 *model;

    ENGINE_BLK(802C1DD0);
    for (;; b++) {
        ENGINE_BLK(802C1E18);
        if (b == end)
            break;
        ENGINE_BLK(802C1E20);
        model = B_MODEL(b);
        if (targets != 0) {
            ENGINE_BLK(802C1E28);
            if (b->unkEB == 0)
                goto next;
            ENGINE_BLK(802C1E34);
            D_803F7684 = b->unk30;
        }
        ENGINE_BLK(802C1E40);
        kind = model[4];
        if (kind == 0xFF)
            goto next;
        ENGINE_BLK(802C1E50);
        if (kind != 1) {
            ENGINE_BLK(802C1E58);
            all++;
        }
        ENGINE_BLK(802C1E5C);
        if ((b->unkEA | D_803F7688) == 0)
            continue;
        ENGINE_BLK(802C1E74);
        if (kind != 1) {
            ENGINE_BLK(802C1E80);
            down++;
        }
        ENGINE_BLK(802C1E84);
        value += U32_(model, 8);
    next:
        ENGINE_BLK(802C1E8C);
    }
    ENGINE_BLK(802C1E94);
    D_8036EB92 = all;
    D_8036EA70.bd = down;
    D_8036EA70.ip = value;
    D_803F7688 = 0;
}

/* the level file's 0x74 section's record n (from 1): its data */
s16 *func_802C1EE0(s32 n) {
    u8 *l = (u8 *)D_80358074, *base, *p;

    ENGINE_BLK(802C1EE0);
    base = l + U32_(l, 0x74);
    p = base + 4;
    for (n--;; n--) {
        ENGINE_BLK(802C1F04);
        if (n == 0)
            break;
        ENGINE_BLK(802C1F0C);
        p = base + U16_(p, 0);
    }
    ENGINE_BLK(802C1F1C);
    return (s16 *)(p + 2);
}

/* record n's part `part` (from 1) placed at (x, y, z) (16.16, 39050.c):
   its matrix (this frame's, D_803F7820/24 plus its offset), and its
   points with that id moved there */
void func_802C1F30(s32 n, s32 part, s32 x, s32 y, s32 z) {
    u8 *l = (u8 *)D_80358074, *base, *p, *t, *m;
    s32 id, k;

    ENGINE_BLK(802C1F30);
    base = l + U32_(l, 0x74);
    p = base + 4;
    for (n--;; n--) {
        ENGINE_BLK(802C1F58);
        if (n == 0)
            break;
        ENGINE_BLK(802C1F60);
        p = U16_(p, 0) + base;
    }
    ENGINE_BLK(802C1F70);
    id = U16_(p, 0x14 + (part - 1) * 2);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C1F98);
        m = D_803F7824;
    } else {
        ENGINE_BLK(802C1FA8);
        m = D_803F7820;
    }
    ENGINE_BLK(802C1FB4);
    func_802ACA60(x, y, z, (s32 *)(m + id));
    ENGINE_BLK(802C1FC8);
    func_802AC8CC((u32 *)(m + id));
    ENGINE_BLK(802C1FD0);
    x >>= 11;
    y >>= 11;
    z >>= 11;
    k = S32_(p, 0x28);
    t = p + 0x2C;
    for (;; t += 0x44) {
        ENGINE_BLK(802C1FE4);
        if (k == 0)
            break;
        ENGINE_BLK(802C1FEC);
        k--;
        if (S32_(t, 0x14) == id) {
            ENGINE_BLK(802C1FFC);
            S32_(t, 0x18) = x, S32_(t, 0x1C) = y, S32_(t, 0x20) = z;
        }
        ENGINE_BLK(802C2008);
        if (S32_(t, 0x24) == id) {
            ENGINE_BLK(802C2014);
            S32_(t, 0x28) = x, S32_(t, 0x2C) = y, S32_(t, 0x30) = z;
        }
        ENGINE_BLK(802C2020);
        if (S32_(t, 0x34) == id) {
            ENGINE_BLK(802C202C);
            S32_(t, 0x38) = x, S32_(t, 0x3C) = y, S32_(t, 0x40) = z;
        }
        ENGINE_BLK(802C2038);
    }
    ENGINE_BLK(802C2040);
}

/* each frame (hd.c): the 0x74 section's moving triangles made (into
   D_803F7828's 0x28-byte records) from their points' offsets and their
   parts' places */
void func_802C2054(void) {
    u8 *l = (u8 *)D_80358074, *base, *r, *t;
    u32 *d = (u32 *)D_803F7828;
    s32 k, end;

    ENGINE_BLK(802C2054);
    base = l + U32_(l, 0x74);
    if (U32_(base, 0) == 0)
        goto out;
    ENGINE_BLK(802C2088);
    t = base + 4;
    do {
        ENGINE_BLK(802C208C);
        r = t;
        end = S16_(r, 0);
        k = S32_(r, 0x28);
        t = r + 0x2C;
        for (;;) {
            ENGINE_BLK(802C209C);
            if (k == 0)
                break;
            ENGINE_BLK(802C20A4);
            k--;
            d[0] = (U16_(t, 0) << 5) + S32_(t, 0x18);
            d[1] = (U16_(t, 2) << 5) + S32_(t, 0x1C);
            d[2] = (U16_(t, 4) << 5) + S32_(t, 0x20);
            d[3] = (U16_(t, 6) << 5) + S32_(t, 0x28);
            d[4] = (U16_(t, 8) << 5) + S32_(t, 0x2C);
            d[5] = (U16_(t, 0xA) << 5) + S32_(t, 0x30);
            d[6] = (U16_(t, 0xC) << 5) + S32_(t, 0x38);
            d[7] = (U16_(t, 0xE) << 5) + S32_(t, 0x3C);
            d[8] = (U16_(t, 0x10) << 5) + S32_(t, 0x40);
            d += 10;
            t += 0x44;
        }
        ENGINE_BLK(802C2168);
    } while (end != -1);
out:
    ENGINE_BLK(802C2174);
}
