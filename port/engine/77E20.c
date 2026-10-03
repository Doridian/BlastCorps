/*
 * hd_code 77E20 (us.v11 0x802BC5E0-0x802C2184): the buildings and their
 * destruction, as native C (engine.h, buildings.h).
 *
 * A building (objects.h) takes damage by groups (a byte each from 0xEC,
 * 0..100, buildings.h): a vehicle running into it (func_802BE77C, which
 * every vehicle module calls with its type and its VehicleState), the game's
 * C's collision tests (89250) and the delayed damage queue (D_803F7690:
 * func_802BC888 adds to it, func_802BCA2C runs it).  A group at 100 is
 * destroyed: its pieces stop colliding, the ones that rest on it fall
 * (func_802BF668; func_802BD1F8 draws them falling), debris and smoke are
 * started (the effect records, D_803F3968) and the damage counts towards
 * the level's (D_803649F0).
 *
 * The model file's sections are buildings.h's MS_; the per-frame rates
 * (falling, smoke, dust, shadows, the queue's delay, the push) its named
 * constants.
 *
 * ENGINE_BLK charges the original's blocks for --cpu-model n64: each path
 * charges what the original's did (a loop's test once more than its body;
 * ENGINE_BLKN where a loop became one copy), in an order of their own.
 */
#include "buildings.h"
#include "game/game.h"
#include "game/audio.h"
#include "game/level.h"

/* ---- the data -------------------------------------------------------- */

/* the level's objects to destroy (5CB60.c's D_80306480): 0x30 bytes, to
   a level of -1 */
typedef struct TargetObj {
    /* 0x00 */ s32 x, y, z;
    /* 0x0C */ s8 ids[9];        /* the objects (func_8029D210), to -1 */
    /* 0x15 */ s8 level;
    /* 0x16 */ u8 outline[0x1A]; /* the radar's (D_8036C790) */
} TargetObj;
SIZE_CHECK(TargetObj, 0x30);

/* a set of the level's buildings' groups (D_803BE708: a count, then
   these, 5CB60.c's func_802A1EC8) whose destruction counts as the
   building's (func_802BD064) */
typedef struct GroupSet {
    /* 0x00 */ Building *PTR32 b;
    /* 0x04 */ u32 n;
    /* 0x08 */ u8 groups[16];    /* 0-based */
} GroupSet;
SIZE_CHECK(GroupSet, 0x18);

/* the models a vehicle type can't harm (D_803059F0), to a mask of 0 */
typedef struct Immune {
    u32 level;                  /* 0xFFFF: any */
    u32 model;
    u32 types;                  /* a bit per vehicle type */
} Immune;

extern u8 D_803F3968[NFX][FX_SIZE];     /* the effect records */
extern u8 D_803F3FF8[FX_SIZE];          /* the one being made */
extern DelayedHit D_803F7690[NDELAYED];
extern u8 *PTR32 D_803F77D4;            /* the part numbers hit this frame (D_803F77D8...) */
extern u8 D_803F77D8[];
extern u8 *PTR32 D_803F77E4;            /* and last frame's (D_803F77E8...) */
extern u8 D_803F77E8[];
extern HitPair *PTR32 D_803F3960;       /* (building, group) pairs hit this frame */
extern HitPair D_803F3910[];
extern u8 *PTR32 D_803F3964;            /* this frame's matrices for falling groups */
extern u8 D_803F24D0[], D_803F2ED0[];   /* (one per frame) */
extern s32 D_803F38D0[16];              /* a matrix being made */
extern Mtx *PTR32 D_803F7658;           /* the falling groups' shadows' matrices */
extern Mtx *PTR32 D_803F765C;           /* the next free matrix */
extern s32 D_803F7660;
extern s32 D_803F7664, D_803F7668, D_803F766C;  /* where a moving building is now */
extern s32 D_803F7670, D_803F7674, D_803F7678;
extern s32 D_803F7684, D_803F7688;
extern s32 D_803F77F4, D_803F77F8;      /* the frames of the last push, of the last hit */
extern s16 D_803F77FC;                  /* the push's direction (its sign) */
extern u16 D_803F77FE;                  /* the damage of the last hit */
extern u8 D_803F7800, D_803F7801, D_803F7802, D_803F7803, D_803F7804, D_803F7805;
extern u8 D_803F7806;                   /* every target building down (hd.c: func_802C1AA0) */
extern u8 D_803F7807, D_803F7808, D_803F7809;
extern s8 D_803F780A;
extern u8 D_803F780B, D_803F780C, D_803F780D, D_803F780E, D_803F780F, D_803F7810, D_803F7812;
extern Part *PTR32 D_803F77D0;
extern Smoke D_803F0900[NSMOKE];
extern Dust D_803EFED0[NDUST];
extern FallShadow D_803F1BE0[NSHADOW];
extern u8 *PTR32 D_803F7820, *PTR32 D_803F7824;
extern u8 *PTR32 D_803F7828;

extern u8 D_803643D6, D_803643D7, D_803643DB, D_80364AC1;
extern u32 D_803649E8;
extern u64 D_803649D8;                  /* a random state */
extern s32 D_802E8BDC;                  /* the level */
extern s32 D_80358068;                  /* a frame count */
extern TargetObj D_80306480[];
extern UnkStruct_8039C800 D_8039C800[];
extern u8 D_8039C940;
extern u8 *PTR32 D_803BE708;            /* the level's group sets */
extern u8 D_803BE738;
extern u32 D_8036C790;
extern u16 D_803EF6FC;
extern s32 D_8036C7C8;
extern s16 D_803C30A8[];                /* the visible cells, to -1 */
extern s32 D_803643F8, D_80364400;      /* the camera's position */
extern s32 D_803A740C;
extern Immune D_803059F0[];
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
extern u16 D_803649E0, D_803649E2, D_803649E4;  /* the spin a falling group starts with */
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

/* 56040: the object `id`'s state byte */
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
void func_802BDDB4(Building *b, AnimTex *anim, AnimTex *end);
REGS(a2, s0, t1, t2, t7 -> a2, t7)
u32 *func_802BE228(u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out);
REGS(t0, s0, t1, t2, t7 -> t0, t7)
u32 *func_802BE3C8(u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out);
REGS(v0, a3, s6 -> s5)
u32 func_802BE574(Building *b, s32 g, s32 fall);
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
void func_802C1214(Dust *s, Building *b);
REGS(a0, a1 -> a0, a1)
u32 *func_802C12E0(u32 *g0, u32 *g1, u32 **g1_out);
REGS(t3, t9 -> a2)
s32 func_802C1A28(s32 group, Building *b);

/* the display list commands this module writes */
#define DL_SEGMENT(seg) (0xBC000006 | (seg) * 4 << 8)  /* G_MOVEWORD: segment seg's address */
#define DL_CALL 0x06000000              /* G_DL, pushing */
#define DL_END 0xB8000000               /* G_ENDDL */
#define DL_MTX_PUSH 0x01040040          /* G_MTX: modelview, multiplied, pushed */
#define DL_MTX_POP 0xBD000000           /* G_POPMTX */
#define DL_PIPESYNC 0xE7000000
#define DL_ENVCOLOR 0xFB000000
#define DL_SETTIMG 0xFD                 /* (opcodes) */
#define DL_SETPRIMCOLOR 0xFA

/* ---- the level's end, the targets ------------------------------------ */

/* Whether the level's goal is reached once its goal building is hit: in
   a level that asks nothing else (D_803643DB and D_80364AC1 clear), yes;
   else once every target building is down (D_803F7806), every hole filled
   and every object destroyed.  Two callers, with their blocks: */
typedef struct GoalBlks {
    Blk open, check, holes, holes_b, objs, objs_b;
} GoalBlks;

static s32 goal_open(const GoalBlks *k) {
    s32 ok;

    if (D_803643DB == 0) {
        BLKT(k->open);
        if (D_80364AC1 == 0)
            return 1;
    }
    BLKT(k->check);
    if (D_803F7806 == 0)
        return 0;
    BLKT(k->holes);
    ok = func_802BC7DC();
    BLKT(k->holes_b);
    if (!ok)
        return 0;
    BLKT(k->objs);
    ok = func_802BC714();
    BLKT(k->objs_b);
    return ok;
}

/* Each frame (hd.c): when D_803F7805 is set (the level's goal building
   hit), the level is won if the goal is open: D_803643DA, and the game
   mode moves on. */
void func_802BC5E0(void) {
    static const GoalBlks k = {
        {ENGINE_BLK_802BC638}, {ENGINE_BLK_802BC648}, {ENGINE_BLK_802BC658},
        {ENGINE_BLK_802BC660}, {ENGINE_BLK_802BC668}, {ENGINE_BLK_802BC670},
    };

    ENGINE_BLK(802BC5E0);
    if (D_803F7805 != 0) {
        ENGINE_BLK(802BC620);
        D_803F7805 = 0;
        if (goal_open(&k)) {
            ENGINE_BLK(802BC678);
            D_803643DA = 1;
            D_802E8BD8 = 1;
            if (D_803643DB == 0) {
                ENGINE_BLK(802BC6A0);
                if (D_80364A90 == 0x40) {
                    ENGINE_BLK(802BC6D0);
                    func_80275390(D_80364A90);
                } else {
                    ENGINE_BLK(802BC6B4);
                    *((u8 *)&D_803649E8 + NE_X3(0)) = 1;
                    D_80364A98 = 8;
                }
            }
        }
    }
    ENGINE_BLK(802BC6DC);
}

/* whether target object o is destroyed: every one of its ids' state
   (func_8029D210) is 0; blocks: the test, an id, after its state, the
   next id, and the alive one's */
typedef struct ObjBlks {
    Blk test, id, state, next, alive;
} ObjBlks;

static s32 obj_destroyed(TargetObj *o, const ObjBlks *k) {
    s8 *q;
    s32 alive;

    for (q = o->ids; BLKT(k->test), *q != -1; q++) {
        BLKT(k->id);
        alive = func_8029D210(*q);
        BLKT(k->state);
        if (alive) {
            BLKT(k->alive);
            return 0;
        }
        BLKT(k->next);
    }
    return 1;
}

/* whether every object of this level to destroy (D_80306480) is: the ones
   not behind the carrier (z, D_803EF6E4) */
REGS(-> t4)
s32 func_802BC714(void) {
    static const ObjBlks k = {
        {ENGINE_BLK_802BC77C}, {ENGINE_BLK_802BC78C}, {ENGINE_BLK_802BC794},
        {ENGINE_BLK_802BC79C}, {ENGINE_BLK_802BC7A4},
    };
    TargetObj *o;
    s32 lim = D_803EF6E4;

    ENGINE_BLK(802BC714);
    for (o = D_80306480; ENGINE_BLK(802BC748), o->level != -1; o++) {
        ENGINE_BLK(802BC758);
        if (o->level == D_802E8BDC) {
            ENGINE_BLK(802BC768);
            if (!(o->z < lim)) {
                ENGINE_BLK(802BC778);
                if (!obj_destroyed(o, &k)) {
                    ENGINE_BLK(802BC7B4);
                    return 0;
                }
            }
        }
        ENGINE_BLK(802BC7AC);
    }
    ENGINE_BLK(802BC7B4);
    return 1;
}

/* whether every hole is filled */
REGS(-> t4)
s32 func_802BC7DC(void) {
    UnkStruct_8039C800 *h = D_8039C800;
    s32 n;

    ENGINE_BLK(802BC7DC);
    for (n = D_8039C940; ENGINE_BLK(802BC804), n != 0; n--, h++) {
        ENGINE_BLK(802BC80C);
        if (h->unk26 == 0) {
            ENGINE_BLK(802BC828);
            return 0;
        }
        ENGINE_BLK(802BC818);
    }
    ENGINE_BLK(802BC824);
    ENGINE_BLK(802BC828);
    return 1;
}

/* ---- the delayed damage ---------------------------------------------- */

/* the queue emptied */
REGS()
void func_802BC840(void) {
    s32 i;

    ENGINE_BLK(802BC840);
    for (i = 0; ENGINE_BLK(802BC85C), i < NDELAYED; i++) {
        ENGINE_BLK(802BC864);
        D_803F7690[i].frames = 0;
    }
    ENGINE_BLK(802BC874);
}

/* `amount` of damage to group `group` of building b: added to what is
   queued for it, or queued for 1..DELAY_MAX - 1 frames from now (none if
   the queue is full) */
REGS(a2, t3, t9)
void func_802BC888(s32 amount, s32 group, Building *b) {
    DelayedHit *h;
    s32 i;

    ENGINE_BLK(802BC888);
    for (i = 0, h = D_803F7690; ENGINE_BLK(802BC918), i < NDELAYED; i++, h++) {
        ENGINE_BLK(802BC920);
        if (h->frames == 0)
            continue;
        ENGINE_BLK(802BC930);
        if (h->b != b)
            continue;
        ENGINE_BLK(802BC93C);
        if (h->group != group)
            continue;
        ENGINE_BLK(802BC948);
        h->amount += amount;
        ENGINE_BLK(802BC9A4);
        return;
    }
    ENGINE_BLK(802BC958);
    for (i = 0, h = D_803F7690; ENGINE_BLK(802BC964), i < NDELAYED; i++, h++) {
        ENGINE_BLK(802BC96C);
        if (h->frames == 0) {
            ENGINE_BLK(802BC984);
            h->group = group;
            h->amount = amount;
            h->b = b;
            h->frames = func_8026A8E0(1, DELAY_MAX);
            ENGINE_BLK(802BC9A0);
            break;
        }
        ENGINE_BLK(802BC97C);
    }
    ENGINE_BLK(802BC9A4);
}

/* Each frame (hd.c): the queued damage whose time has come, done as a hit
   (the damage capped at 100; at 100 the group goes) */
void func_802BCA2C(void) {
    DelayedHit *h;
    Building *b;
    Piece *q;
    s32 i, amount, g, d;

    ENGINE_BLK(802BCA2C);
    for (i = 0, h = D_803F7690; ENGINE_BLK(802BCA68), i < NDELAYED; i++, h++) {
        ENGINE_BLK(802BCA70);
        if (h->frames == 0)
            goto next;
        ENGINE_BLK(802BCA80);
        if (h->frames != 1) {
            ENGINE_BLK(802BCB98);
            h->frames--;
            goto next;
        }
        ENGINE_BLK(802BCA8C);
        amount = h->amount;
        h->frames = 0;
        b = h->b;
        g = h->group;
        D_802E8BE4 = SHAKE_HIT_FRAMES;
        if (amount <= SHAKE_HIT_MAX) {
            ENGINE_BLK(802BCAB4);
            D_802E8BE8 = amount;
        } else {
            ENGINE_BLK(802BCABC);
            D_802E8BE8 = SHAKE_HIT_MAX;
        }
        ENGINE_BLK(802BCAC8);
        d = B_DAMAGE(b)[g - 1] + amount;
        if (d >= 100) {
            ENGINE_BLK(802BCAE4);
            d = 100;
        }
        ENGINE_BLK(802BCAE8);
        B_DAMAGE(b)[g - 1] = d;
        if (d == 100) {
            /* the group destroyed: its shadow, smoke (not while the level
               ends), dust, the building's first effect, its pieces off */
            ENGINE_BLK(802BCAF4);
            func_802C1438(g, b);
            ENGINE_BLK(802BCAFC);
            if (D_803643D6 == 0) {
                ENGINE_BLK(802BCB0C);
                if (D_803643D7 == 0) {
                    ENGINE_BLK(802BCB1C);
                    func_802C09B8(g, b);
                }
            }
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
            for (q = b->unk4; ENGINE_BLK(802BCB58), q != (Piece *)b->unk8; q++) {
                ENGINE_BLK(802BCB60);
                if (q->group == g) {
                    ENGINE_BLK(802BCB6C);
                    q->active = 0;
                }
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

/* ---- the parts hit, this frame and the last -------------------------- */

/* (The part numbers: a vehicle type's parts (D_803A6B30) counted from 1,
   the one that touched a piece, func_802BEBB0's n.) */

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
    u8 *d = D_803F77E8, *s;

    ENGINE_BLK(802BCC48);
    if (D_80358060 != 0) {
        ENGINE_BLK(802BCC7C);
        for (s = D_803F77D8; ENGINE_BLK(802BCC90), s != D_803F77D4;) {
            ENGINE_BLK(802BCC98);
            *d++ = *s++;
        }
    }
    ENGINE_BLK(802BCCAC);
    D_803F77E4 = d;
}

/* part `kind` added to this frame's list (once) */
REGS(fp)
void func_802BCCD4(s32 kind) {
    s32 found;

    ENGINE_BLK(802BCCD4);
    found = func_802BCD80(kind);
    ENGINE_BLK(802BCCEC);
    if (!found) {
        ENGINE_BLK(802BCCF4);
        *D_803F77D4 = kind;
        D_803F77D4++;
    }
    ENGINE_BLK(802BCD0C);
}

/* whether this frame's list holds anything but `kind` (unused) */
REGS(v0 -> v1)
s32 func_802BCD20(s32 kind) {
    u8 *p;
    s32 r = 0;

    ENGINE_BLK(802BCD20);
    for (p = D_803F77D8; ENGINE_BLK(802BCD4C), p != D_803F77D4;) {
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
    u8 *p;
    s32 r = 0;

    ENGINE_BLK(802BCD80);
    for (p = D_803F77D8; ENGINE_BLK(802BCDAC), p != D_803F77D4;) {
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
    u8 *p;
    s32 r = 0;

    ENGINE_BLK(802BCDE0);
    for (p = D_803F77E8; ENGINE_BLK(802BCE0C), p != D_803F77E4;) {
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
   target building, an empty hole, or an object to destroy that isn't yet;
   its kind (1, 2, 3; 0 none) in D_803F7808 and its outline in
   D_8036C790. */
s32 func_802BCE40(void) {
    static const ObjBlks k = {
        {ENGINE_BLK_802BCFAC}, {ENGINE_BLK_802BCFBC}, {ENGINE_BLK_802BCFC4},
        {ENGINE_BLK_802BCFCC}, {ENGINE_BLK_802BCFD4},
    };
    Building *b;
    s32 px = D_803643E0, py = D_803643E4, pz = D_803643E8;
    s64 best = 9999999, d;
    u32 found = 0, outline;
    s32 kind = 0, n, down;
    UnkStruct_8039C800 *h;
    TargetObj *o;

    ENGINE_BLK(802BCE40);
    for (b = D_803F4030; ENGINE_BLK(802BCEB0), b != D_803F7654; b++) {
        ENGINE_BLK(802BCEB8);
        if (!B_TARGET(b))
            continue;
        ENGINE_BLK(802BCEC4);
        if (B_DESTROYED(b))
            continue;
        ENGINE_BLK(802BCED0);
        d = func_802ABCDC(b->x, b->y, b->z, px, py, pz);
        ENGINE_BLK(802BCEE0);
        if (!(d < best))
            continue;
        ENGINE_BLK(802BCEEC);
        down = func_802BD064(b + 1);
        ENGINE_BLK(802BCEF4);
        if (down)
            continue;
        ENGINE_BLK(802BCEFC);
        best = d;
        found = (u32)b;
        kind = 1;
    }
    ENGINE_BLK(802BCF0C);
    for (n = D_8039C940, h = D_8039C800; ENGINE_BLK(802BCF1C), n != 0; n--, h++) {
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
    for (o = D_80306480; ENGINE_BLK(802BCF6C), o->level != -1; o++) {
        ENGINE_BLK(802BCF7C);
        if (o->level == D_802E8BDC) {
            ENGINE_BLK(802BCF8C);
            d = func_802ABCDC(o->x, o->y, o->z, px, py, pz);
            ENGINE_BLK(802BCF9C);
            if (d < best) {
                ENGINE_BLK(802BCFA8);
                if (!obj_destroyed(o, &k)) {
                    kind = 3;
                    found = (u32)o;
                    best = d;
                }
            }
        }
        ENGINE_BLK(802BCFE0);
    }
    ENGINE_BLK(802BCFE8);
    outline = 0;
    if (kind != 0) {
        ENGINE_BLK(802BCFF0);
        if (kind == 1) {
            ENGINE_BLK(802BD014);
            outline = (u32)B_SECTION((Building *)found, MS_CORNERS);
        } else {
            ENGINE_BLK(802BCFFC);
            if (kind == 3) {
                ENGINE_BLK(802BD00C);
                outline = (u32)((TargetObj *)found)->outline;
            } else {
                ENGINE_BLK(802BD004);
                outline = (u32)((UnkStruct_8039C800 *)found)->corners;
            }
        }
    }
    ENGINE_BLK(802BD020);
    D_8036C790 = outline;
    D_803F7808 = kind;
    return found;
}

/* whether building `next - 1` belongs to a group set of the level's
   (D_803BE708) all of whose groups are destroyed */
REGS(t0 -> v0)
s32 func_802BD064(Building *next) {
    Building *b = next - 1;
    u8 *sets = D_803BE708;
    GroupSet *s;
    u8 *g;
    s32 n, k;

    ENGINE_BLK(802BD064);
    if (sets == NULL)
        goto no;
    ENGINE_BLK(802BD090);
    n = *(u32 *)sets;
    for (s = (GroupSet *)(sets + 4);; s++) {
        ENGINE_BLK(802BD098);
        if (n == 0)
            goto no;
        ENGINE_BLK(802BD0A0);
        n--;
        if (s->b == b)
            break;
        ENGINE_BLK(802BD0B0);
    }
    ENGINE_BLK(802BD0B8);
    for (k = s->n, g = s->groups; ENGINE_BLK(802BD0C0), k != 0; k--, g++) {
        ENGINE_BLK(802BD0C8);
        if (B_DAMAGE(b)[*g] != 100)
            goto no;
        ENGINE_BLK(802BD0E4);
    }
    ENGINE_BLK(802BD0EC);
    ENGINE_BLK(802BD0F0);
    return 1;
no:
    ENGINE_BLK(802BD0F0);
    return 0;
}

/* the distance from the carrier (D_803EF6DC, D_803EF6E4) to target t (of
   kind D_803F7809) in x and z, over D_803EF6FC: into D_8036C7C8 (30C70.c) */
void func_802BD10C(s32 t_) {
    s32 x, z;
    s64 d;

    ENGINE_BLK(802BD10C);
    if (D_803F7809 == 2) {
        ENGINE_BLK(802BD168);
        x = ((UnkStruct_8039C800 *)t_)->x;
        z = ((UnkStruct_8039C800 *)t_)->z;
    } else {
        ENGINE_BLK(802BD144);
        if (D_803F7809 == 3) {
            ENGINE_BLK(802BD15C);
            x = ((TargetObj *)t_)->x;
            z = ((TargetObj *)t_)->z;
        } else {
            ENGINE_BLK(802BD150);
            x = ((Building *)t_)->x;
            z = ((Building *)t_)->z;
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

/* n display list commands copied from src to d (a loop the compiler keeps
   inline: these are a few commands each, too few for a call to memcpy) */
static inline u32 *copy_cmds(u32 *d, const u32 *src, s32 n) {
    s32 i;

    for (i = 0; i < n * 2; i++)
        d[i] = src[i];
    return d + n * 2;
}

/* Display list commands copied from src to src_end at d, in one go; the
   original's loop charged its test block once more than its copy block. */
#define COPY_CMDS(d, src, src_end, B_TEST, B_COPY)                          \
    do {                                                                    \
        s32 ncmd_ = ((u8 *)(src_end) - (u8 *)(src)) / 8;                    \
        ENGINE_BLKN(B_TEST, ncmd_ + 1);                                     \
        ENGINE_BLKN(B_COPY, ncmd_);                                         \
        (d) = copy_cmds((d), (src), ncmd_);                                 \
    } while (0)

/* One look of building b's group gi (0-based): a falling group gets its
   matrix (func_802BE574) pushed around its lists and comes down a frame,
   landing at its fall height (its landing effects, the screen shaken, the
   group destroyed and what rests on it let go); then its two lists, with
   the animated textures put in (anim to anim_end, func_802BE228 and
   func_802BE3C8).  *t7, *t8: what the original leaves in $t7 and $t8. */
static void draw_look(Building *b, s32 gi, GroupDls *dls, AnimTex *anim, AnimTex *anim_end, u32 **d0p, u32 **d1p,
                      u32 *t7, u32 *t8, s32 *fell) {
    u8 *model = B_MODEL(b);
    u32 *d0 = *d0p, *d1 = *d1p, *src, *src_end;
    u32 mtx = 0;
    s32 falling, t, fall;

    ENGINE_BLK(802BD608);
    falling = B_FALLING(b)[gi];
    if (falling) {
        ENGINE_BLK(802BD620);
        t = B_FALL_T(b)[gi] + 1;
        B_FALL_T(b)[gi] = t;
        fall = (s32)((u32)t * (u32)t * (u32)-FALL_ACCEL);
        *t8 = *(u32 *)(model + MS_FALLS);
        *fell = 1;
        if (!(-(s32)((u32)((u16 *)B_SECTION(b, MS_FALLS))[gi] << 16) < fall)) {
            /* it has landed */
            ENGINE_BLK(802BD678);
            B_FALLING(b)[gi] = 0;
            func_802BFD1C(b, gi);
            ENGINE_BLK(802BD684);
            D_802E8BE4 = SHAKE_HIT_FRAMES;
            D_802E8BE8 = SHAKE_LANDED;
            B_DAMAGE(b)[gi] = 100;
            func_802BD85C(b, gi);
            ENGINE_BLK(802BD6B8);
        }
        ENGINE_BLK(802BD6BC);
        mtx = func_802BE574(b, gi, fall);
        ENGINE_BLK(802BD6C4);
        d0[0] = DL_MTX_PUSH;
        d0[1] = mtx;
        d0 += 2;
    }
    ENGINE_BLK(802BD6D8);
    src = (u32 *)(model + dls->dl0);
    src_end = (u32 *)(model + dls->dl0_end);
    if (anim != anim_end) {
        ENGINE_BLK(802BD6EC);
        d0 = func_802BE228(d0, src_end, anim, anim_end, src, &src);
        ENGINE_BLK(802BD6F4);
    } else {
        COPY_CMDS(d0, src, src_end, 802BD6FC, 802BD704);
    }
    *t7 = (u32)src_end;
    ENGINE_BLK(802BD718);
    if (falling) {
        ENGINE_BLK(802BD720);
        d0[0] = DL_MTX_POP;
        d0[1] = 0;
        d0 += 2;
        d1[0] = DL_MTX_PUSH;
        d1[1] = mtx;
        d1 += 2;
    }
    ENGINE_BLK(802BD744);
    src = (u32 *)(model + dls->dl1);
    src_end = (u32 *)(model + dls->dl1_end);
    if (anim != anim_end) {
        ENGINE_BLK(802BD758);
        d1 = func_802BE3C8(d1, src_end, anim, anim_end, src, &src);
        ENGINE_BLK(802BD760);
    } else {
        COPY_CMDS(d1, src, src_end, 802BD768, 802BD770);
    }
    *t7 = (u32)src_end;
    ENGINE_BLK(802BD784);
    if (falling) {
        ENGINE_BLK(802BD78C);
        d1[0] = DL_MTX_POP;
        d1[1] = 0;
        d1 += 2;
    }
    ENGINE_BLK(802BD79C);
    *d0p = d0;
    *d1p = d1;
}

/* Each visible cell's place in D_803C30A8 (the visible cells, to -1) + 1,
   0 for the others: the original searched the list for each building,
   its blocks charged by the place.  Marked for one func_802BD1F8 and
   cleared after it (the cells it marked). */
static u16 cell_at[256];
static u8 cells_marked[256];
static s32 ncells_marked;

static s32 visible_cells_mark(void) {
    s32 n;
    s16 c;

    for (n = 0; (c = D_803C30A8[n]) != -1; n++)
        if ((u16)c < 256 && cell_at[c] == 0) {
            cell_at[c] = n + 1;
            cells_marked[ncells_marked++] = c;
        }
    return n;
}

static void visible_cells_clear(void) {
    while (ncells_marked != 0)
        cell_at[cells_marked[--ncells_marked]] = 0;
}

/* Each frame (hd.c): the buildings in the visible cells drawn into two
   display lists (g0, g1: the frame's two passes), each building's lists
   made at d0 and d1; the falling groups' matrices from D_803F3964
   (D_803F24D0 or D_803F2ED0 by frame), the moving ones' from arg5; the
   smoke (g6) and the dust (g7) first, the shadows of the falling groups
   last.

   A building's lists: its model's head, then its groups' looks
   (MS_GROUP_DLS) each unless a condition hides it; if the first is shown,
   only it (the building whole).  Every building, drawn or not, ends both
   its lists (and, moving, pops a matrix from g0 and g1) as the original
   does. */
void func_802BD1F8(Gfx *g0_, Gfx *g1_, Gfx *d0_, Gfx *d1_, Mtx *arg4, Mtx *arg5, Gfx *g6, Gfx *g7) {
    u32 *g0 = (u32 *)g0_, *g1 = (u32 *)g1_, *d0 = (u32 *)d0_, *d1 = (u32 *)d1_;
    Building *b, *end;
    u8 *model, *r, *next, *grp_end;
    AnimTex *anim, *anim_end;
    u32 *src, *src_end;
    Mtx *m;
    s32 first, n, gi, dmg, i, nvis, hidden;
    GroupCond *c;
    /* what the original leaves in $t6-$t8, which 5CB60.c's collision
       triangles, 60F60.c and 69BB0.c's driver read from the context */
    u32 t6 = 0, t7 = 0, t8 = 0;
    s32 drawn = 0, fell = 0;

    ENGINE_BLK(802BD1F8);
    /* (its $s4 as it found it: 5CB60.c reads it from the context) */
    ENGINE_SAVE(G(rS4));
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
    nvis = visible_cells_mark();
    end = D_803F7654;
    for (b = D_803F4030; ENGINE_BLK(802BD29C), b != end; b++) {
        ENGINE_BLK(802BD2A4);
        i = cell_at[B_CELL(b)] - 1;
        if (i < 0) {
            ENGINE_BLKN(802BD2B0, nvis + 1);
            ENGINE_BLKN(802BD2C4, nvis);
            goto done;
        }
        ENGINE_BLKN(802BD2B0, i + 1);
        ENGINE_BLKN(802BD2C4, i + 1);
        ENGINE_BLK(802BD2CC);
        if (B_ID(b) == MODEL_GOAL) {
            ENGINE_BLK(802BD2DC);
            n = func_802BD8C8();
            ENGINE_BLK(802BD2E4);
            if (n)
                goto done;
        }
        ENGINE_BLK(802BD2EC);
        model = B_MODEL(b);
        if (M_MOVES(model)) {
            /* a moving building: its matrix (4EBE0.c, from its place
               unk1C..24, its behaviour, its state at 0x38 and its speed
               unk34) pushed, and its parts moved to where it is now */
            s32 ox, oy, oz;

            ENGINE_BLK(802BD2FC);
            m = D_803F765C;
            D_803F765C = (Mtx *)((u8 *)m + 0x40);
            func_802933A0(b->unk1C, b->unk20, b->unk24, M_MOVES(model), m, (u8 *)b + 0x38, (Gfx *)g0,
                          (Gfx *)g1, b->unk34, b->x, b->y, b->z);
            ENGINE_BLK(802BD3A4);
            g0 += 2;
            g1 += 2;
            ox = b->x, oy = b->y, oz = b->z;
            b->x = D_803F7664;
            b->y = D_803F7668;
            b->z = D_803F766C;
            /* (its $t8 and $t9 as they were: 5CB60.c and 69BB0.c read them) */
            ENGINE_SAVE(G(rT8) | G(rT9));
            func_802BD99C(b, D_803F7664 - ox, D_803F7668 - oy, D_803F766C - oz);
            ENGINE_RESTORE();
            ENGINE_BLK(802BD480);
        }
        ENGINE_BLK(802BD4C8);
        model = B_MODEL(b);
        anim = (AnimTex *)B_SECTION(b, MS_ANIMS);
        anim_end = (AnimTex *)B_SECTION(b, MS_CENTRES);
        func_802BDDB4(b, anim, anim_end);
        ENGINE_BLK(802BD4F0);
        g0[0] = DL_SEGMENT(9);          /* the model's textures */
        g0[1] = K0(M_TEXTURES(model));
        g1[0] = DL_SEGMENT(9);
        g1[1] = K0(M_TEXTURES(model));
        g0[2] = DL_CALL;                /* its lists, made below */
        g0[3] = K0(d0);
        g1[2] = DL_CALL;
        g1[3] = K0(d1);
        g0 += 4;
        g1 += 4;
        /* the head, in both */
        src = (u32 *)B_SECTION(b, MS_DL);
        src_end = (u32 *)B_SECTION(b, MS_DL_END);
        drawn = 1;
        t7 = (u32)src_end;
        n = ((u8 *)src_end - (u8 *)src) / 8;
        ENGINE_BLKN(802BD568, n + 1);
        ENGINE_BLKN(802BD570, n);
        copy_cmds(d0, src, n);
        copy_cmds(d1, src, n);
        d0 += n * 2;
        d1 += n * 2;
        ENGINE_BLK(802BD58C);
        /* the groups' looks */
        first = 1;
        grp_end = B_SECTION(b, MS_FALLS);
        for (r = B_SECTION(b, MS_GROUP_DLS); ENGINE_BLK(802BD590), r != grp_end; r = next) {
            ENGINE_BLK(802BD598);
            gi = GDL_GROUP(r) - 1;
            n = GDL_N(r);
            next = GDL_NEXT(r);
            t7 = n;
            hidden = 0;
            for (c = GDL_COND(r); ENGINE_BLK(802BD5A8), n != 0; c++) {
                ENGINE_BLK(802BD5B0);
                dmg = B_DAMAGE(b)[c->group - 1];
                t7 = --n;
                if (c->below) {
                    ENGINE_BLK(802BD5D8);
                    if (dmg < c->limit) {
                        ENGINE_BLK(802BD5E4);
                        hidden = 1;
                        break;
                    }
                } else {
                    ENGINE_BLK(802BD5EC);
                    if (c->limit < dmg) {
                        hidden = 1;
                        break;
                    }
                }
            }
            if (hidden) {
                ENGINE_BLK(802BD5F8);
                t7 = n * 4;
            } else {
                draw_look(b, gi, GDL_DLS(r, GDL_N(r)), anim, anim_end, &d0, &d1, &t7, &t8, &fell);
                if (first)
                    break;
            }
            ENGINE_BLK(802BD7A4);
            first = 0;
        }
        t6 = first;
    done:
        ENGINE_BLK(802BD7AC);
        d0[0] = DL_END;
        d0[1] = 0;
        d1[0] = DL_END;
        d1[1] = 0;
        d0 += 2;
        d1 += 2;
        if (M_MOVES(B_MODEL(b))) {
            ENGINE_BLK(802BD7D8);
            g0[0] = DL_MTX_POP;         /* the moving one's matrix popped */
            g0[1] = 0;
            g1[0] = DL_MTX_POP;
            g1[1] = 0;
            g0 += 2;
            g1 += 2;
        }
        ENGINE_BLK(802BD7FC);
    }
    ENGINE_BLK(802BD804);
    visible_cells_clear();
    ENGINE_RESTORE();
    if (drawn) {
        ENGINE_LEAVE(rT6, t6);
        ENGINE_LEAVE(rT7, t7);
    }
    if (fell)
        ENGINE_LEAVE(rT8, t8);
    ENGINE_LEAVE(rV0, (u32)end);        /* ($v0, which 5CB60.c reads) */
    g0 = func_802C12E0(g0, g1, &g1);
    ENGINE_BLK(802BD80C);
    g0[0] = DL_END;
    g0[1] = 0;
    g1[0] = DL_END;
    g1[1] = 0;
}

/* the pieces of group g (0-based) that depend on another (group2)
   switched off */
REGS(v0, a3)
void func_802BD85C(Building *b, s32 g) {
    Piece *p;

    ENGINE_BLK(802BD85C);
    g++;
    for (p = b->unk4; ENGINE_BLK(802BD880), p != (Piece *)b->unk8; p++) {
        ENGINE_BLK(802BD888);
        if (p->group != g)
            continue;
        ENGINE_BLK(802BD894);
        if (p->group2 == 0)
            continue;
        ENGINE_BLK(802BD8A0);
        p->active = 0;
    }
    ENGINE_BLK(802BD8AC);
}

/* in level 0x32, whether func_80286090 says no (the goal there can't be
   hit, or drawn, until then) */
REGS(-> t1)
s32 func_802BD8C8(void) {
    s32 r = 0, ok;

    ENGINE_BLK(802BD8C8);
    if (D_802E8BDC == 0x32) {
        ENGINE_BLK(802BD930);
        ok = func_80286090(D_802E8BDC);
        ENGINE_BLK(802BD938);
        if (!ok) {
            ENGINE_BLK(802BD940);
            r = 1;
        }
    }
    ENGINE_BLK(802BD944);
    return r;
}

/* the axis piece p is flattest along, from its normal's sizes: z if |nz|
   is the largest, then y, else x */
static void piece_axis(Piece *p, s64 ax, s64 ay, s64 az) {
    ENGINE_BLK(802BDD58);
    if (!(az < ax)) {
        ENGINE_BLK(802BDD64);
        if (!(az < ay)) {
            ENGINE_BLK(802BDD6C);
            p->axis = 0;
            return;
        }
    }
    ENGINE_BLK(802BDD74);
    if (!(ay < ax)) {
        ENGINE_BLK(802BDD80);
        if (!(ay < az)) {
            ENGINE_BLK(802BDD88);
            p->axis = 1;
            return;
        }
    }
    ENGINE_BLK(802BDD94);
    p->axis = 2;
}

/* Building b moved by (dx, dy, dz) (<< 5): its model's heights, shadow,
   triangles, corners, effect records and group centres, and its pieces,
   whose planes are made again from their corners.  It leaves its $fp
   (qz) in the context, which 5CB60.c reads.  (71140.c moves what the
   chopper carries with it too.) */
REGS(v0, a2, a3, t0)
void func_802BD99C(Building *b, s32 dx, s32 dy, s32 dz) {
    u8 *s, *e;
    s32 sx = dx >> 5, sy = dy >> 5, sz = dz >> 5;
    s32 wx = dx << 11, wy = dy << 11, wz = dz << 11;
    s32 qx = dx >> 2, qy = dy >> 2, qz = dz >> 2;
    s32 n, k;
    Piece *p;

    ENGINE_BLK(802BD99C);
    for (s = B_SECTION(b, MS_HEIGHTS), e = B_SECTION(b, MS_SUPPORTS); ENGINE_BLK(802BD9D0), s != e; s += 2) {
        ENGINE_BLK(802BD9D8);
        *(s16 *)s += sy;
    }
    ENGINE_BLK(802BD9EC);
    s = B_SECTION(b, MS_SHADOW);
    ((s16 *)s)[0] += sx;
    ((s16 *)s)[1] += sz;
    ((s16 *)s)[2] += sx;
    ((s16 *)s)[3] += sz;
    for (s = B_SECTION(b, MS_TRIS), e = B_SECTION(b, MS_ANIMS); ENGINE_BLK(802BDA34), s != e; s += 0x14) {
        ENGINE_BLK(802BDA3C);
        for (k = 0; k < 9; k += 3) {
            ((s16 *)s)[k] += sx;
            ((s16 *)s)[k + 1] += sy;
            ((s16 *)s)[k + 2] += sz;
        }
    }
    ENGINE_BLK(802BDAB0);
    s = B_SECTION(b, MS_CORNERS);
    for (n = 4; ENGINE_BLK(802BDABC), n != 0; n--, s += 6) {
        ENGINE_BLK(802BDAC4);
        ((s16 *)s)[0] += sx;
        ((s16 *)s)[1] += sy;
        ((s16 *)s)[2] += sz;
    }
    ENGINE_BLK(802BDAF4);
    for (s = B_SECTION(b, MS_FX_HIT), e = B_SECTION(b, MS_FX_LAND); ENGINE_BLK(802BDB04), s != e; s += FX_SIZE) {
        ENGINE_BLK(802BDB0C);
        FX_W(s, FX_X) += wx, FX_W(s, FX_Y) += wy, FX_W(s, FX_Z) += wz;
        FX_W(s, FX_UNK28) += wy;
    }
    ENGINE_BLK(802BDB44);
    for (s = B_SECTION(b, MS_FX_LAND), e = B_SECTION(b, MS_GROUP_DLS); ENGINE_BLK(802BDB54), s != e; s += FX_SIZE) {
        ENGINE_BLK(802BDB5C);
        FX_W(s, FX_X) += wx, FX_W(s, FX_Y) += wy, FX_W(s, FX_Z) += wz;
        FX_W(s, FX_UNK28) += wy;
    }
    ENGINE_BLK(802BDB94);
    for (s = B_SECTION(b, MS_CENTRES), e = B_SECTION(b, MS_FX_HIT); ENGINE_BLK(802BDBA4), s != e; s += 8) {
        ENGINE_BLK(802BDBAC);
        ((s16 *)s)[0] += sx;
        ((s16 *)s)[1] += sy;
        ((s16 *)s)[2] += sz;
    }
    ENGINE_BLK(802BDBD8);
    for (p = b->unk4; ENGINE_BLK(802BDBEC), p != (Piece *)b->unk8; p++) {
        s32 x1, y1, z1, x2, y2, z2, x3, y3, z3;
        s32 ey, ez3, ez2, ey3, ex3, ex2;
        s64 nx, ny, nz, ax, ay, az;
        f32 fx, fy, fz, sq;

        ENGINE_BLK(802BDBF4);
        x1 = p->p[0][0] + qx, y1 = p->p[0][1] + qy, z1 = p->p[0][2] + qz;
        x2 = p->p[1][0] + qx, y2 = p->p[1][1] + qy, z2 = p->p[1][2] + qz;
        x3 = p->p[2][0] + qx, y3 = p->p[2][1] + qy, z3 = p->p[2][2] + qz;
        p->p[0][0] = x1, p->p[0][1] = y1, p->p[0][2] = z1;
        p->p[1][0] = x2, p->p[1][1] = y2, p->p[1][2] = z2;
        p->p[2][0] = x3, p->p[2][1] = y3, p->p[2][2] = z3;
        /* the normal: (p1 - p2) x (p1 - p3) */
        ey = y1 - y2, ez2 = z1 - z2, ex2 = x1 - x2;
        ey3 = y1 - y3, ez3 = z1 - z3, ex3 = x1 - x3;
        nx = (s64)ey * ez3 - (s64)ez2 * ey3;
        ny = (s64)ez2 * ex3 - (s64)ex2 * ez3;
        nz = (s64)ex2 * ey3 - (s64)ey * ex3;
        p->nx = nx;
        p->ny = ny;
        p->nz = nz;
        fx = (f32)nx;
        fy = (f32)ny;
        fz = (f32)nz;
        sq = fx * fx + fy * fy + fz * fz;
        p->unk24 = sq;
        p->unk20 = __builtin_sqrtf(sq);
        p->d = -(nx * x2 + ny * y2 + nz * z2);
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
        piece_axis(p, ax, ay, az);
        ENGINE_BLK(802BDD9C);
    }
    ENGINE_BLK(802BDDA4);
    ENGINE_LEAVE(rFP, qz);              /* ($fp, which 5CB60.c reads) */
}

/* Building b's animated textures stepped (anim to end, AnimTex).  Kind 0
   runs through its frames, a step every `lo` frames, blending from one to
   the next; 1 picks one at random now and then; others show the frame for
   the camera's heading between lo and hi.  The level's goal building only
   while its goal is open. */
REGS(v0, t1, t2)
void func_802BDDB4(Building *b, AnimTex *a, AnimTex *end) {
    static const GoalBlks gk = {
        {ENGINE_BLK_802BDE04}, {ENGINE_BLK_802BDE14}, {ENGINE_BLK_802BDE24},
        {ENGINE_BLK_802BDE2C}, {ENGINE_BLK_802BDE34}, {ENGINE_BLK_802BDE3C},
    };
    static const HeadingBlks hk = {
        {ENGINE_BLK_802BDEC8}, {ENGINE_BLK_802BDEF8}, {ENGINE_BLK_802BDF00}, {ENGINE_BLK_802BDF30},
        {ENGINE_BLK_802BDF48}, {ENGINE_BLK_802BDF78}, {ENGINE_BLK_802BDF84}, {ENGINE_BLK_802BDFB4},
        {ENGINE_BLK_802BDEBC}, {ENGINE_BLK_802BDF3C},
    };
    s32 lo, hi, n, t, h, span;

    ENGINE_BLK(802BDDB4);
    if (B_ID(b) == MODEL_GOAL) {
        ENGINE_BLK(802BDDF4);
        if (!goal_open(&gk))
            goto out;
    }
    for (; ENGINE_BLK(802BDE44), a != end; a = ANIM_NEXT(a)) {
        ENGINE_BLK(802BDE4C);
        if (a->kind == 0) {
            /* the next step every `lo` frames, the blend as far into it */
            ENGINE_BLK(802BE178);
            t = a->hi + 1;
            n = a->lo;
            if (t == n) {
                ENGINE_BLK(802BE18C);
                lo = a->cur + 1;
                if (lo == a->n) {
                    ENGINE_BLK(802BE1A0);
                    lo = 0;
                }
                ENGINE_BLK(802BE1A4);
                a->cur = lo;
                t = 0;
            }
            ENGINE_BLK(802BE1AC);
            a->hi = t;
            a->blend = engine_divu((u32)t * 255, n);
            if (n == 0) {
                ENGINE_BLK(802BE1DC);
                engine_break(0x802BE1DC, 7);
            }
        } else if (a->kind == 1) {
            /* a random frame, every `lo` sixteenths of D_803649D8 */
            ENGINE_BLK(802BDE58);
            ENGINE_BLK(802BE114);
            t = (u32)D_803649D8 >> 4;
            if (a->lo == 0) {
                ENGINE_BLK(802BE140);
                engine_break(0x802BE140, 7);
            }
            ENGINE_BLK(802BE144);
            if (engine_remu(t, a->lo) == 0) {
                ENGINE_BLK(802BE14C);
                if (a->n == 0) {
                    ENGINE_BLK(802BE160);
                    engine_break(0x802BE160, 7);
                }
                ENGINE_BLK(802BE164);
                a->cur = engine_remu((u32)t >> 8, a->n);
            }
        } else {
            /* by the camera's heading from the building: the first frame
               before lo, the last after hi, between them in steps and
               blends (the range may wrap past 0) */
            ENGINE_BLK(802BDE58);
            ENGINE_BLK(802BDE60);
            h = heading_to(b->x, b->z, (u32)D_803643F8 >> 11, (u32)D_80364400 >> 11, &hk);
            ENGINE_BLK(802BDFBC);
            lo = a->lo;
            hi = a->hi;
            if (!(hi < lo)) {
                ENGINE_BLK(802BDFD0);
                if (h < lo)
                    goto first;
                ENGINE_BLK(802BDFD8);
                if (hi < h)
                    goto final;
                ENGINE_BLK(802BDFE0);
                t = engine_divu((u32)(h - lo) * (a->n - 1) * 255, hi - lo);
                if (hi - lo == 0) {
                    ENGINE_BLK(802BE028);
                    engine_break(0x802BE028, 7);
                }
                ENGINE_BLK(802BE02C);
                a->cur = (u32)t / 255;
                ENGINE_BLK(802BE048);
                a->blend = (u32)t % 255;
            } else {
                ENGINE_BLK(802BE058);
                if (h < lo) {
                    ENGINE_BLK(802BE064);
                    if (hi < h)
                        goto first;
                }
                ENGINE_BLK(802BE06C);
                span = 0xFFF - lo;
                if (h < lo) {
                    ENGINE_BLK(802BE080);
                    t = span + h;
                } else {
                    ENGINE_BLK(802BE088);
                    t = h - lo;
                }
                ENGINE_BLK(802BE08C);
                t = engine_divu((u32)t * (a->n - 1) * 255, span + hi);
                if (span + hi == 0) {
                    ENGINE_BLK(802BE0CC);
                    engine_break(0x802BE0CC, 7);
                }
                ENGINE_BLK(802BE0D0);
                a->cur = (u32)t / 255;
                ENGINE_BLK(802BE0F0);
                a->blend = (u32)t % 255;
            }
            goto step;
        first:
            ENGINE_BLK(802BE0FC);
            a->cur = 0;
            goto step;
        final:
            ENGINE_BLK(802BE104);
            a->cur = a->n - 1;
        }
    step:
        ENGINE_BLK(802BE1E0);
    }
out:
    ENGINE_BLK(802BE1F4);
}

/* A building's display list from src to end copied to dl, its animated
   textures' frames put in: a G_SETTIMG of one of them (the records anim
   to anim_end) gets its current frame; one that blends (AnimTex.blends)
   also the next G_SETTIMG the frame after, and the G_SETPRIMCOLOR after
   them the blend between the two.  func_802BE228 and func_802BE3C8 are
   this for the two lists, with their own blocks. */
typedef struct {
    Blk entry, loop, cmd, scan0, scan, cmp, skip, found, anim, cur, set, wrap, cmd2, copy2, sel, sel2,
        set2, cmd3, copy3, prim, still, still2, put, out;
} CopyBlks;

static u32 *copy_dl(const CopyBlks *k, u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out) {
    u32 w0, w1, f;
    AnimTex *a;

    BLKT(k->entry);
    while (BLKT(k->loop), src != end) {
        BLKT(k->cmd);
        w0 = src[0];
        dl[0] = w0;
        w1 = src[1];
        if (w0 >> 24 == DL_SETTIMG) {
            /* a texture: one of the animated ones? */
            BLKT(k->scan0);
            for (a = anim; BLKT(k->scan), a != anim_end; a = ANIM_NEXT(a)) {
                BLKT(k->cmp);
                if (w1 == a->texture)
                    break;
                BLKT(k->skip);
            }
            if (a != anim_end) {
                BLKT(k->found);
                if (!a->blends) {
                    /* its current frame, if not the first */
                    BLKT(k->still);
                    f = a->cur;
                    if (f != 0) {
                        BLKT(k->still2);
                        w1 = ANIM_FRAME(a, f);
                    }
                } else {
                    BLKT(k->anim);
                    f = a->cur;
                    if (f != 0) {
                        BLKT(k->cur);
                        w1 = ANIM_FRAME(a, f);
                    }
                    BLKT(k->set);
                    dl[1] = w1;
                    src += 2;
                    dl += 2;
                    if (++f == a->n) {
                        BLKT(k->wrap);
                        f = 0;
                    }
                    /* the next texture: the frame after */
                    for (;;) {
                        BLKT(k->cmd2);
                        w0 = src[0];
                        dl[0] = w0;
                        w1 = src[1];
                        if (w0 >> 24 == DL_SETTIMG)
                            break;
                        BLKT(k->copy2);
                        dl[1] = w1;
                        src += 2;
                        dl += 2;
                    }
                    BLKT(k->sel);
                    if (f != 0) {
                        BLKT(k->sel2);
                        w1 = ANIM_FRAME(a, f);
                    }
                    BLKT(k->set2);
                    dl[1] = w1;
                    src += 2;
                    dl += 2;
                    /* the blend, at the primitive colour */
                    for (;;) {
                        BLKT(k->cmd3);
                        w0 = src[0];
                        if (w0 >> 24 == DL_SETPRIMCOLOR)
                            break;
                        BLKT(k->copy3);
                        dl[0] = w0;
                        dl[1] = src[1];
                        src += 2;
                        dl += 2;
                    }
                    BLKT(k->prim);
                    dl[0] = w0 | a->blend;
                    dl[1] = src[1];
                    src += 2;
                    dl += 2;
                    continue;
                }
            }
        }
        BLKT(k->put);
        dl[1] = w1;
        src += 2;
        dl += 2;
    }
    BLKT(k->out);
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
u32 *func_802BE228(u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out) {
    return copy_dl(&blks_802BE228, dl, end, anim, anim_end, src, src_out);
}

/* the second (dl in $t0) */
REGS(t0, s0, t1, t2, t7 -> t0, t7)
u32 *func_802BE3C8(u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out) {
    return copy_dl(&blks_802BE3C8, dl, end, anim, anim_end, src, src_out);
}

/* The matrix of building b's falling group g (0-based): turned about its
   centre by its spin rates times its fall time (12-bit angles), and
   lowered by `fall`; made at D_803F3964 (the next one), returned as its
   physical address. */
REGS(v0, a3, s6 -> s5)
u32 func_802BE574(Building *b, s32 g, s32 fall) {
    s32 *m = (s32 *)D_803F3964;
    u32 t = B_FALL_T(b)[g];
    s32 ax, ay, az, cx, cy, cz;
    u16 *c;

    ENGINE_BLK(802BE574);
    D_803F3964 = (u8 *)m + 0x40;
    ax = (s32)(t * (u32)B_SPIN_X(b)[g]);
    if (ax < 0) {
        ENGINE_BLK(802BE5F8);
        ax += 0xFFF;
    }
    ENGINE_BLK(802BE600);
    ay = (s32)(t * B_SPIN_Y(b)[g]);
    if (ay < 0) {
        ENGINE_BLK(802BE618);
        ay += 0xFFF;
    }
    ENGINE_BLK(802BE620);
    az = (s32)(t * B_SPIN_Z(b)[g]);
    if (az < 0) {
        ENGINE_BLK(802BE638);
        az += 0xFFF;
    }
    ENGINE_BLK(802BE640);
    c = (u16 *)(B_SECTION(b, MS_CENTRES) + g * 8);
    cx = c[0] << 16;
    cy = c[1] << 16;
    cz = c[2] << 16;
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

/* whether the camera's limits (D_803A7410..12) are narrowed; blocks: the
   second test's */
#define CAMERA_NARROWED(B2) (D_803A7410 != 0 || (ENGINE_BLK(B2), D_803A7412 != 0xFFF))

/* Each frame, from every vehicle module: the vehicle of type `type` (its
   VehicleState vs) against the buildings, by its sphere (its Solid,
   D_803A7300), each piece hit doing damage (func_802BEBB0); what happened
   into vs->unk9C..9E. */
REGS(t8, gp)
void func_802BE77C(s32 type, VS *vs) {
    Solid *s = D_803A7300;
    Building *b;
    s32 hit, t;

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
    while (ENGINE_BLK(802BE7F8), s->kind != type)
        s++;
    ENGINE_BLK(802BE804);
    if (s->end != 0) {
        ENGINE_BLK(802BE810);
        for (b = D_803F4030; ENGINE_BLK(802BE834), b != D_803F7654; b++) {
            ENGINE_BLK(802BE83C);
            hit = func_8029CFA4(s->x, s->y, s->z, s->r, b->x, b->y, b->z, B_RADIUS(b));
            ENGINE_BLK(802BE850);
            if (hit) {
                ENGINE_BLK(802BE858);
                if (B_ID(b) == MODEL_GOAL) {
                    ENGINE_BLK(802BE868);
                    t = func_802BD8C8();
                    ENGINE_BLK(802BE870);
                    if (t)
                        goto next;
                }
                ENGINE_BLK(802BE878);
                t = func_802BE944(b, type);
                ENGINE_BLK(802BE880);
                if (!t) {
                    ENGINE_BLK(802BE888);
                    func_802BEADC(b, s->x, s->y, s->z, s->r, type);
                }
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
    if (CAMERA_NARROWED(802BE8C0)) {
        ENGINE_BLK(802BE8D8);
        D_803A7425 = 1;
    }
    ENGINE_BLK(802BE8E4);
    vs->unk9C = (s8)D_803F7802;
    vs->unk9D = (s8)D_803F7803;
    vs->unk9E = (s8)D_803F7807;
}

/* whether vehicle type `type` can't harm building b: its model is listed
   for this level (or any) with the type's bit (D_803059F0) */
REGS(t3, t8 -> t9)
s32 func_802BE944(Building *b, s32 type) {
    Immune *p;
    s32 model = B_ID(b), r = 0;

    ENGINE_BLK(802BE944);
    if (model != MODEL_GOAL) {
        for (p = D_803059F0;; p++) {
            ENGINE_BLK(802BE980);
            if (p->types == 0)
                break;
            ENGINE_BLK(802BE990);
            if (p->level != 0xFFFF) {
                ENGINE_BLK(802BE9A0);
                if (p->level != (u32)D_802E8BDC)
                    continue;
            }
            ENGINE_BLK(802BE9A8);
            if (p->model != (u32)model) {
                ENGINE_BLK(802BE9B4);
                continue;
            }
            ENGINE_BLK(802BE9BC);
            if ((p->types & (1u << (type & 31))) == 0)
                continue;
            ENGINE_BLK(802BE9D4);
            r = 1;
            break;
        }
    }
    ENGINE_BLK(802BE9D8);
    ENGINE_LEAVE(rT9, r);               /* ($t9, which 5CB60.c reads) */
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
    HitPair *p = D_803F3960;

    ENGINE_BLK(802BEA30);
    p->b = b;
    p->group = group;
    D_803F3960 = p + 1;
}

/* whether (b, group) is in it */
REGS(t3, t9 -> t7)
s32 func_802BEA70(s32 group, Building *b) {
    HitPair *p;

    ENGINE_BLK(802BEA70);
    for (p = D_803F3910; ENGINE_BLK(802BEA9C), p != D_803F3960; p++) {
        ENGINE_BLK(802BEAA4);
        if (p->b != b)
            continue;
        ENGINE_BLK(802BEAB4);
        if (p->group != group)
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
    static const PieceTestBlks k = {
        {ENGINE_BLK_802BEB38}, {ENGINE_BLK_802BEB40}, {ENGINE_BLK_802BEB48}, {ENGINE_BLK_802BEB50},
        {ENGINE_BLK_802BEB58}, {ENGINE_BLK_802BEB60}, {ENGINE_BLK_802BEB68}, {ENGINE_BLK_802BEB70},
    };
    Piece *p;

    ENGINE_BLK(802BEADC);
    x >>= 2;
    y >>= 2;
    z >>= 2;
    r >>= 2;
    for (p = b->unk4; ENGINE_BLK(802BEB1C), p != (Piece *)b->unk8; p++) {
        ENGINE_BLK(802BEB24);
        if (p->active == 0)
            continue;
        ENGINE_BLK(802BEB30);
        if (piece_touched(p, x, y, z, r, &k)) {
            ENGINE_BLK(802BEB78);
            func_802BEBB0(p, type, b);
        }
        ENGINE_BLK(802BEB80);
    }
    ENGINE_BLK(802BEB88);
    ENGINE_LEAVE(rT9, (u32)b);          /* ($t9, which 5CB60.c reads) */
}

/* us.v10's func_802BEBB0 lacks the check of D_803F7812 (802BED4C), and
   its blocks after it are named 0x10 lower */
#ifdef VERSION_US_V10
#define BLK_V10(v11, v10) ENGINE_BLK(v10)
#else
#define BLK_V10(v11, v10) ENGINE_BLK(v11)
#endif

/* whether the hit marks building b as the one the player last hit
   (D_8036B974; 23C20.c's hint): not an explosion's, not on a building of
   strength 1, a target or the goal, and not the same again */
static s32 hit_marks(Building *b, s32 type) {
    if (type == 0xFF)
        return 0;
    ENGINE_BLK(802BECAC);
    if (M_STRENGTH(B_MODEL(b)) == 1)
        return 0;
    ENGINE_BLK(802BECC0);
    if (B_TARGET(b))
        return 0;
    ENGINE_BLK(802BECCC);
    if (B_ID(b) == MODEL_GOAL)
        return 0;
    ENGINE_BLK(802BECDC);
    return D_8036B974 != b;
}

/* whether a hit by part n of vehicle type `type` does damage: while
   dashing (D_803F7800), from an explosion, after a push long enough
   (func_802BEFF4), or if the part didn't touch last frame */
static s32 hit_damages(s32 type, s32 n) {
    s32 again;

    ENGINE_BLK(802BED34);
    if (D_803F7800 != 0)
        return 1;
    ENGINE_BLK(802BED44);
    if (type == 0xFF)
        return 1;
#ifndef VERSION_US_V10
    ENGINE_BLK(802BED4C);
    if (D_803F7812 == 0)
#endif
    {
        BLK_V10(802BED5C, 802BED4C);
        func_802BEFF4();
        BLK_V10(802BED64, 802BED54);
        if (D_803F780C != 0)
            return 1;
    }
    BLK_V10(802BED74, 802BED64);
    again = func_802BCDE0(n);
    BLK_V10(802BED7C, 802BED6C);
    return !again;
}

/* Piece p of building b hit by vehicle type `type`: if a part of the
   type's (D_803A6B30) touches it, the group takes its damage (by the
   part's power and the model's strength, func_802BFF6C), once a frame,
   and what follows: the screen shaken, the camera turned, the goal's
   flag, the group destroyed at 100. */
REGS(s0, t8, t9)
void func_802BEBB0(Piece *p, s32 type, Building *b) {
    static const PieceTestBlks pk = {
        {ENGINE_BLK_802BEC44}, {ENGINE_BLK_802BEC4C}, {ENGINE_BLK_802BEC54}, {ENGINE_BLK_802BEC5C},
        {ENGINE_BLK_802BEC64}, {ENGINE_BLK_802BEC6C}, {ENGINE_BLK_802BEC74}, {ENGINE_BLK_802BEC7C},
    };
    KindPart *k;
    s32 n = 0, g, d, dmg, seen;
    s8 *c;
    Piece *q;

    ENGINE_BLK(802BEBB0);
    /* the type's parts (numbered n from 1): the first that touches p */
    for (k = D_803A6B30;; k++) {
        ENGINE_BLK(802BEBF4);
        if (k->end == -1) {
            BLK_V10(802BEF5C, 802BEF4C);
            BLK_V10(802BEF60, 802BEF50);
            return;
        }
        ENGINE_BLK(802BEC04);
        if (k->end == 0)
            continue;
        ENGINE_BLK(802BEC0C);
        if (k->kind != type)
            continue;
        ENGINE_BLK(802BEC18);
        n++;
        if (piece_touched(p, k->x >> 2, k->y >> 2, k->z >> 2, k->r >> 2, &pk))
            break;
    }
    ENGINE_BLK(802BEC84);
    if (type != 0xFF) {
        ENGINE_BLK(802BEC90);
        D_803F77F8 = D_80358068;
    }
    ENGINE_BLK(802BECA0);
    if (hit_marks(b, type)) {
        ENGINE_BLK(802BECEC);
        D_8036B971 = 1;
        D_8036B974 = b;
    }
    ENGINE_BLK(802BED00);
    func_802BEF9C(b);
    ENGINE_BLK(802BED08);
    if (D_803BE738 != 0) {
        BLK_V10(802BEF60, 802BEF50);
        return;
    }
    ENGINE_BLK(802BED18);
    g = p->group;
    func_802BCCD4(n);
    ENGINE_BLK(802BED24);
    seen = func_802BEA70(g, b);
    ENGINE_BLK(802BED2C);
    if (!seen && hit_damages(type, n)) {
        /* the damage, once a frame for each group */
        BLK_V10(802BED84, 802BED74);
        func_802BEA30(g, b);
        BLK_V10(802BED8C, 802BED7C);
        if (B_ID(b) == MODEL_GOAL) {
            BLK_V10(802BED9C, 802BED8C);
            if (type == 0) {
                BLK_V10(802BEDA4, 802BED94);
                D_803F7805 = 1;
            }
            BLK_V10(802BEDB0, 802BEDA0);
            d = 0;
        } else {
            BLK_V10(802BEDB8, 802BEDA8);
            d = func_802BFF6C(g, k->power, type, b, M_STRENGTH(B_MODEL(b)), n);
        }
        BLK_V10(802BEDC8, 802BEDB8);
        D_802E8BE4 = SHAKE_HIT_FRAMES;
        if (d <= SHAKE_HIT_MAX) {
            BLK_V10(802BEDE8, 802BEDD8);
            D_802E8BE8 = d;
        } else {
            BLK_V10(802BEDF0, 802BEDE0);
            D_802E8BE8 = SHAKE_HIT_MAX;
        }
        BLK_V10(802BEDF8, 802BEDE8);
        dmg = B_DAMAGE(b)[g - 1] + d;
        if (dmg >= 100) {
            BLK_V10(802BEE18, 802BEE08);
            dmg = 100;
        }
        BLK_V10(802BEE1C, 802BEE0C);
        B_DAMAGE(b)[g - 1] = dmg;
        D_803F77FE = d;
        func_802BF898(g, dmg, b);
        BLK_V10(802BEE2C, 802BEE1C);
        if (dmg == 100) {
            /* the group destroyed: its triangles, shadow, smoke, dust,
               the building's first effect; its pieces off, the ones it
               brings back on */
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
            D_802E8BE4 = SHAKE_DOWN_FRAMES;
            D_802E8BE8 = SHAKE_DOWN;
            for (q = b->unk4; BLK_V10(802BEE88, 802BEE78), q != (Piece *)b->unk8; q++) {
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
            }
        }
    }
    /* the camera turned by the piece (unless the part is listed,
       D_803A7408), or, the group gone, D_803F7802 */
    BLK_V10(802BEED8, 802BEEC8);
    D_803F7803 = 1;
    for (c = D_803A7408;; c++) {
        BLK_V10(802BEEF0, 802BEEE0);
        if (*c == n)
            goto done;
        BLK_V10(802BEEFC, 802BEEEC);
        if (*c < 0)
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
    BLK_V10(802BEF60, 802BEF50);
}

/* level 0x2D's buildings 0xE7-0xE9 end it when hit */
REGS(t9)
void func_802BEF9C(Building *b) {
    ENGINE_BLK(802BEF9C);
    if (D_802E8BDC == 0x2D) {
        ENGINE_BLK(802BEFBC);
        if (!(B_ID(b) < 0xE7)) {
            ENGINE_BLK(802BEFCC);
            if (B_ID(b) < 0xEA) {
                ENGINE_BLK(802BEFD4);
                D_803BE738 = 1;
            }
        }
    }
    ENGINE_BLK(802BEFE0);
}

/* The pushing of the vehicles that can (not types 0-2, 6, 7, 9, 0xB,
   0x10-0x12): pushed in one direction (the sign of D_803F77FC) for
   PUSH_FRAMES frames in a row, D_803F780C is set for a frame, unless
   D_80370C2C was set on every one of them. */
REGS()
void func_802BEFF4(void) {
    static const u8 types[] = { 1, 2, 6, 7, 9, 0xB, 0x10, 0x11, 0x12 };
    static const Blk tblk[] = {
        {ENGINE_BLK_802BF018}, {ENGINE_BLK_802BF024}, {ENGINE_BLK_802BF02C}, {ENGINE_BLK_802BF034},
        {ENGINE_BLK_802BF03C}, {ENGINE_BLK_802BF044}, {ENGINE_BLK_802BF04C}, {ENGINE_BLK_802BF054},
        {ENGINE_BLK_802BF05C},
    };
    s32 type = D_80364456, dir, steer, k, cnt;

    ENGINE_BLK(802BEFF4);
    if (type == 0)
        goto out;
    for (k = 0; k < 9; k++) {
        BLKT(tblk[k]);
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
    if (D_803F780F != 0)                /* once a frame */
        goto out;
    ENGINE_BLK(802BF09C);
    D_803F780F = 1;
    if (D_803F77F4 + 1 != D_80358068)   /* not hit last frame: start again */
        goto reset;
    ENGINE_BLK(802BF0C4);
    if (dir != 0) {
        ENGINE_BLK(802BF0CC);
        if (D_803F780A == dir)
            goto reset;
    }
    ENGINE_BLK(802BF0DC);
    cnt = D_803F780B;
    steer = (u8)D_80370C2C != 0;
    if (cnt == 0) {
        ENGINE_BLK(802BF0EC);
        ENGINE_BLK(802BF100);
        if (steer)
            ENGINE_BLK(802BF108);
        ENGINE_BLK(802BF114);
        D_803F780E = steer;
        D_803F780D = 0;
    } else {
        ENGINE_BLK(802BF128);
        ENGINE_BLK(802BF13C);
        if (steer) {
            ENGINE_BLK(802BF144);
            ENGINE_BLK(802BF150);
        }
        if (!steer || steer != D_803F780E) {
            ENGINE_BLK(802BF160);
            D_803F780D = 1;
        }
    }
    ENGINE_BLK(802BF16C);
    cnt++;
    D_803F780B = cnt;
    D_803F780A = dir;
    D_803F780C = 0;
    if (cnt != PUSH_FRAMES)
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

/* the triangles (MS_TRIS) of group `group` (1-based) marked gone */
REGS(t3, t9)
void func_802BF1F0(s32 group, Building *b) {
    u8 *t, *end = B_SECTION(b, MS_ANIMS);

    ENGINE_BLK(802BF1F0);
    for (t = B_SECTION(b, MS_TRIS); ENGINE_BLK(802BF224), t != end; t += 0x14) {
        ENGINE_BLK(802BF22C);
        if (t[0x12] == group) {
            ENGINE_BLK(802BF238);
            t[0x13] = 1;
        }
    }
    ENGINE_BLK(802BF244);
}

/* a piece that pushes the camera (Piece.pushes): the camera kept within a
   quarter turn of its side facing the player, unless the piece only works
   from one side (unk55, unk56) */
REGS(s0)
void func_802BF264(Piece *p) {
    s64 d;
    s32 h = p->heading, a, c;

    ENGINE_BLK(802BF264);
    if (p->pushes == 0)
        goto out;
    ENGINE_BLK(802BF294);
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
    if (p->unk55 != 1) {
        ENGINE_BLK(802BF324);
        if (p->unk56 != 0) {
            ENGINE_BLK(802BF330);
            goto out;
        }
    }
    goto push;
behind:
    ENGINE_BLK(802BF338);
    a = h - 0x400;
    c = h + 0x400;
    if (p->unk55 != 1) {
        ENGINE_BLK(802BF348);
        if (p->unk56 == 0)
            goto out;
    }
push:
    ENGINE_BLK(802BF354);
    func_8029B7CC(a, c);
out:
    ENGINE_BLK(802BF35C);
}

/* ---- destruction ------------------------------------------------------- */

/* the first of building b's groups destroyed: its effect (func_80264CB4) */
REGS(t9)
void func_802BF384(Building *b) {
    s32 n, gone = 0;
    u8 *d = B_DAMAGE(b);

    ENGINE_BLK(802BF384);
    for (n = B_NGROUPS(b); ENGINE_BLK(802BF3A8), n != 0; n--) {
        ENGINE_BLK(802BF3B0);
        if (*d++ == 100) {
            ENGINE_BLK(802BF3C4);
            gone++;
        }
    }
    ENGINE_BLK(802BF3CC);
    if (gone == 1) {
        ENGINE_BLK(802BF3D8);
        func_80264CB4((u32)b->x >> 5, (u32)b->y >> 5, (u32)b->z >> 5, (u32)B_RADIUS(b) >> 5, B_CELL(b),
                      M_UNK6(B_MODEL(b)));
        ENGINE_BLK(802BF494);
    }
    ENGINE_BLK(802BF518);
}

/* Building b destroyed, when every group but the main one is gone or
   falling: counted (D_80364A40, D_803649F0: the model's value) and the
   lights on it (D_803BDFD8, by its index + 1) off. */
REGS(t9)
void func_802BF534(Building *b) {
    s32 main, k, n, idx;
    u8 *d, *l;
    u16 *f;
    u32 v;

    ENGINE_BLK(802BF534);
    if (B_DESTROYED(b))
        goto out;
    ENGINE_BLK(802BF560);
    main = M_MAIN(B_MODEL(b));
    d = B_DAMAGE(b);
    f = B_FALLING(b);
    for (k = 1, n = B_NGROUPS(b); ENGINE_BLK(802BF578), n != 0; k++, n--, d++, f++) {
        ENGINE_BLK(802BF580);
        if (main != k) {
            ENGINE_BLK(802BF58C);
            if (*d != 100) {
                ENGINE_BLK(802BF59C);
                if (*f == 0)
                    goto out;
            }
        }
        ENGINE_BLK(802BF5A8);
    }
    ENGINE_BLK(802BF5B4);
    B_DESTROYED(b) = 1;
    v = M_VALUE(B_MODEL(b));
    D_80364A40 = v;
    D_803649F0 += v;
    idx = (u32)((u8 *)b - (u8 *)D_803F4030) / sizeof(Building);
    ENGINE_BLK(802BF618);
    idx++;
    for (l = D_803BDFD8; ENGINE_BLK(802BF624), l != D_803BDFD4; l += 0x24) {
        ENGINE_BLK(802BF62C);
        if (l[0x11] == idx) {
            ENGINE_BLK(802BF638);
            l[0x12] = 0;
        }
    }
out:
    ENGINE_BLK(802BF644);
}

/* The groups that rest on others (MS_SUPPORTS): one, still standing,
   whose remaining support (the weights of the groups under it that are
   neither destroyed nor falling) is under its strength starts falling
   (B_FALLING; its spin from D_803649E0..E4), its triangles go
   (func_802BF1F0) and its pieces with it, the ones it brings back on;
   again until none does. */
REGS(t9)
void func_802BF668(Building *b) {
    u8 *r, *end, *s;
    s32 again, g, n, sum, q;
    Piece *p;

    ENGINE_BLK(802BF668);
    do {
        ENGINE_BLK(802BF6A4);
        again = 0;
        end = B_SECTION(b, MS_END);
        for (r = B_SECTION(b, MS_SUPPORTS); ENGINE_BLK(802BF6C4), r != end;) {
            ENGINE_BLK(802BF6CC);
            g = r[0] - 1;
            if (B_FALLING(b)[g] != 0 || (ENGINE_BLK(802BF6EC), B_DAMAGE(b)[g] == 100)) {
                ENGINE_BLK(802BF704);
                r = SUP_NEXT(r);
                continue;
            }
            ENGINE_BLK(802BF718);
            sum = 0;
            for (n = r[2], s = r + 3; ENGINE_BLK(802BF724), n != 0; n--, s += 2) {
                ENGINE_BLK(802BF72C);
                q = s[0] - 1;
                if (B_DAMAGE(b)[q] != 100) {
                    ENGINE_BLK(802BF748);
                    if (B_FALLING(b)[q] == 0) {
                        ENGINE_BLK(802BF764);
                        sum += s[1];
                    }
                }
                ENGINE_BLK(802BF76C);
            }
            ENGINE_BLK(802BF774);
            if (sum < r[1]) {
                ENGINE_BLK(802BF784);
                again = 1;
                func_802BF1F0(r[0], b);
                ENGINE_BLK(802BF790);
                g = r[0] - 1;
                B_FALLING(b)[g] = 1;
                B_SPIN_X(b)[g] = D_803649E0;
                B_SPIN_Y(b)[g] = D_803649E2;
                B_SPIN_Z(b)[g] = D_803649E4;
                g++;
                for (p = b->unk4; ENGINE_BLK(802BF7F8), p != (Piece *)b->unk8; p++) {
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
                }
            }
            ENGINE_BLK(802BF848);
            r = s;
        }
        ENGINE_BLK(802BF850);
    } while (again);
    ENGINE_BLK(802BF858);
}

/* ---- a hit's effects ------------------------------------------------- */

/* The effects of a hit on group `group` (now at `damage`): with a model
   that has debris of its own (M_DEBRIS) func_802BF978's; else its effect
   records (MS_FX_HIT) for the group fire once its damage reaches theirs,
   those marked 0xFF on every strong enough hit (FX_STRONG_HIT). */
REGS(t3, t5, t9)
void func_802BF898(s32 group, s32 damage, Building *b) {
    u8 *model = B_MODEL(b), *fx, *end;
    s32 t;

    ENGINE_BLK(802BF898);
    if (M_DEBRIS(model) != 0) {
        ENGINE_BLK(802BF954);
        func_802BF978(model, group, damage, b);
        goto out;
    }
    ENGINE_BLK(802BF8C0);
    end = B_SECTION(b, MS_FX_LAND);
    for (fx = B_SECTION(b, MS_FX_HIT); ENGINE_BLK(802BF8D0), fx != end; fx += FX_SIZE) {
        ENGINE_BLK(802BF8D8);
        if (FX_B(fx, FX_DONE) != 0)
            goto next;
        ENGINE_BLK(802BF8E4);
        if (FX_B(fx, FX_GROUP) != group)
            goto next;
        ENGINE_BLK(802BF8F0);
        t = FX_B(fx, FX_LIMIT);
        if (t == 0xFF) {
            ENGINE_BLK(802BF900);
            if (D_803F77FE >= FX_STRONG_HIT) {
                ENGINE_BLK(802BF914);
                ENGINE_BLK(802BF944);
                func_802C04F0(fx);
            }
            goto next;
        }
        ENGINE_BLK(802BF91C);
        if (damage >= t) {
            ENGINE_BLK(802BF928);
            func_802BFBF4(fx, b);
            ENGINE_BLK(802BF930);
            func_802C04F0(fx);
            ENGINE_BLK(802BF938);
            FX_B(fx, FX_DONE) = 1;
        }
    next:
        ENGINE_BLK(802BF94C);
    }
out:
    ENGINE_BLK(802BF95C);
}

/* the chance tables' records (D_80306344, D_80306350, D_803063D4): kept
   big-endian, 0xC bytes, to a chance over 100 */
#define CHANCE(t) NE_BE16(*(u16 *)(t))
#define CHANCE_KIND(t) NE_BE16(*(u16 *)((t) + 2))
#define CHANCE_LO(t) NE_BE32(*(u32 *)((t) + 4))
#define CHANCE_HI(t) NE_BE32(*(u32 *)((t) + 8))

/* Debris thrown up from group `group`'s centre: how many by a roll of
   D_803063E0's (chance, count) pairs; each of a kind from the chance
   table for the damage (D_80306344, or D_80306350 once destroyed; a hit
   under FX_STRONG_HIT makes none), its speed by the model's M_DEBRIS
   (doubled with D_803EF6FF), all but the first FX_DEBRIS_DELAY frames
   later; and the last one's sound. */
REGS(v0, t3, t5, t9)
void func_802BF978(u8 *model, s32 group, s32 damage, Building *b) {
    s16 *c = (s16 *)(B_SECTION(b, MS_CENTRES) + (group - 1) * 8);
    /* (with no debris, the sound's record is whatever $s7 held: the
       model's byte 5, as an address) */
    u8 *t, *fx = (u8 *)(u32)M_DEBRIS(model);
    s32 x = c[0] << 16, y = c[1] << 16, z = c[2] << 16;
    s32 roll, n, k, sp;

    ENGINE_BLK(802BF978);
    roll = func_802BFB50(0, 100);
    ENGINE_BLK(802BF9EC);
    for (t = D_803063E0; ENGINE_BLK(802BF9F4), t[0] < roll; t += 2)
        ENGINE_BLK(802BFA04);
    ENGINE_BLK(802BFA0C);
    n = t[1];
    for (k = 0; ENGINE_BLK(802BFA14), k != n; k++) {
        ENGINE_BLK(802BFA1C);
        if (damage >= 100) {
            ENGINE_BLK(802BFA24);
            t = D_80306350;
        } else {
            ENGINE_BLK(802BFA30);
            t = D_80306344;
            if (D_803F77FE < FX_STRONG_HIT)
                goto out;
        }
        ENGINE_BLK(802BFA4C);
        roll = func_802BFB50(0, 100);
        for (; ENGINE_BLK(802BFA58), !(roll < CHANCE(t)); t += 0xC)
            ENGINE_BLK(802BFA68);
        ENGINE_BLK(802BFA70);
        fx = D_803F3FF8;
        FX_W(fx, FX_X) = x;
        FX_W(fx, FX_Y) = y;
        FX_W(fx, FX_Z) = z;
        roll = func_802BFB50(CHANCE_LO(t), CHANCE_HI(t));
        ENGINE_BLK(802BFA90);
        sp = (s32)((u32)M_DEBRIS(model) * (u32)roll);
        if (D_803EF6FF != 0) {
            ENGINE_BLK(802BFAAC);
            sp <<= 1;
        }
        ENGINE_BLK(802BFAB0);
        FX_W(fx, FX_SPEED) = sp;
        FX_W(fx, FX_VEL) = 0;
        FX_W(fx, FX_VEL + 4) = 0;
        FX_W(fx, FX_VEL + 8) = 0;
        FX_W(fx, FX_UNK14) = 0;
        FX_W(fx, FX_UNK14 + 4) = 0;
        FX_W(fx, FX_UNK14 + 8) = 0;
        FX_W(fx, FX_UNK28) = FX_UNK28_INIT;
        if (k != 0) {
            ENGINE_BLK(802BFAD8);
            FX_H(fx, FX_DELAY) = FX_DEBRIS_DELAY;
        } else {
            ENGINE_BLK(802BFAE4);
            FX_H(fx, FX_DELAY) = 0;
        }
        ENGINE_BLK(802BFAE8);
        FX_B(fx, FX_RADIUS) = 0;
        FX_B(fx, FX_AMOUNT) = 0;
        FX_B(fx, FX_DONE) = 0;
        FX_B(fx, FX_UNK35) = 0;
        FX_B(fx, FX_KIND) = CHANCE_KIND(t);
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
   strength 1): one of four, at its place */
REGS(v1, t9)
void func_802BFBF4(u8 *fx, Building *b) {
    s8 *k;
    s32 kind;

    ENGINE_BLK(802BFBF4);
    if (M_STRENGTH(B_MODEL(b)) == 1)
        goto out;
    ENGINE_BLK(802BFC54);
    kind = FX_B(fx, FX_KIND);
    for (k = (s8 *)D_8030633C;;) {
        ENGINE_BLK(802BFC64);
        if (*k == -1)
            goto out;
        ENGINE_BLK(802BFC70);
        if (*k++ == kind)
            break;
    }
    ENGINE_BLK(802BFC78);
    ENGINE_BLK(802BFC9C);
    func_80288284(((u32)D_803649D8 >> 8) % 4, (u32)FX_W(fx, FX_X) >> 11, (u32)FX_W(fx, FX_Y) >> 11,
                  (u32)FX_W(fx, FX_Z) >> 11, b->y);
    ENGINE_BLK(802BFCC8);
out:
    ENGINE_BLK(802BFCCC);
}

/* a group of building b (g, from 0) landed: its effect records for it
   (MS_FX_LAND) fire, or with a model that has debris of its own,
   func_802BFDAC's */
REGS(v0, a3)
void func_802BFD1C(Building *b, s32 g) {
    u8 *fx, *end;

    ENGINE_BLK(802BFD1C);
    g++;
    if (M_DEBRIS(B_MODEL(b)) != 0) {
        ENGINE_BLK(802BFD84);
        func_802BFDAC(b, g);
        goto out;
    }
    ENGINE_BLK(802BFD4C);
    end = B_SECTION(b, MS_GROUP_DLS);
    for (fx = B_SECTION(b, MS_FX_LAND); ENGINE_BLK(802BFD60), fx != end; fx += FX_SIZE) {
        ENGINE_BLK(802BFD68);
        if (FX_B(fx, FX_GROUP) == g) {
            ENGINE_BLK(802BFD74);
            func_802C04F0(fx);
        }
        ENGINE_BLK(802BFD7C);
    }
out:
    ENGINE_BLK(802BFD8C);
}

/* a piece of debris where group `group` (1-based) landed, of a kind from
   D_803063D4's chances */
REGS(v0, a3)
void func_802BFDAC(Building *b, s32 group) {
    u8 *model = B_MODEL(b), *t, *fx;
    s16 *c = (s16 *)(B_SECTION(b, MS_CENTRES) + (group - 1) * 8);
    s32 x = c[0], y = b->y, z = c[2], roll;

    ENGINE_BLK(802BFDAC);
    roll = func_802BFB50(0, 100);
    ENGINE_BLK(802BFE10);
    for (t = D_803063D4; ENGINE_BLK(802BFE18), !(roll < CHANCE(t)); t += 0xC)
        ENGINE_BLK(802BFE28);
    ENGINE_BLK(802BFE30);
    fx = D_803F3FF8;
    FX_W(fx, FX_X) = x << 16;
    FX_W(fx, FX_Y) = y << 11;
    FX_W(fx, FX_Z) = z << 16;
    roll = func_802BFB50(CHANCE_LO(t), CHANCE_HI(t));
    ENGINE_BLK(802BFE5C);
    FX_W(fx, FX_VEL) = 0;
    FX_W(fx, FX_VEL + 4) = 0;
    FX_W(fx, FX_VEL + 8) = 0;
    FX_W(fx, FX_UNK14) = 0;
    FX_W(fx, FX_UNK14 + 4) = 0;
    FX_W(fx, FX_UNK14 + 8) = 0;
    FX_H(fx, FX_DELAY) = 0;
    FX_B(fx, FX_RADIUS) = 0;
    FX_B(fx, FX_AMOUNT) = 0;
    FX_W(fx, FX_SPEED) = (s32)((u32)M_DEBRIS(model) * (u32)roll);
    FX_W(fx, FX_UNK28) = FX_UNK28_INIT;
    FX_B(fx, FX_DONE) = 0;
    FX_B(fx, FX_UNK35) = 0;
    FX_B(fx, FX_KIND) = CHANCE_KIND(t);
    func_802C04F0(fx);
    ENGINE_BLK(802BFEAC);
}

/* D_803F7800: whether the vehicle (vs) is dashing or so (D_803F7801 2, or
   one of its three flags at 0x96 is 1; never with D_803F7801 1) */
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
    ENGINE_BLK(802BFF58);
    return;
yes:
    ENGINE_BLK(802BFF50);
    D_803F7800 = 1;
    ENGINE_BLK(802BFF58);
}

/* the next of D_80305E50's damage rules after r, when r isn't taken: the
   ones by D_803F7804, the hole and the push are 4 bytes, a vehicle part's
   4 + its byte 3 */
static u8 *rule_skip(u8 *r) {
    s32 t;

    ENGINE_BLK(802C0170);
    t = r[2];
    if (t == 0xFC || t == 0xFB) {
        if (t == 0xFB)
            ENGINE_BLK(802C0180);
        ENGINE_BLK(802C01B8);
        return r + 4;
    }
    ENGINE_BLK(802C0180);
    ENGINE_BLK(802C0188);
    if (t != 0xFD) {
        ENGINE_BLK(802C0190);
        if (t != 0xFF) {
            ENGINE_BLK(802C0198);
            if (t != 0xFE) {
                ENGINE_BLK(802C01A0);
                return r + r[3] + 4;
            }
        }
    }
    ENGINE_BLK(802C01B0);
    return r + 4;
}

/* The damage vehicle type `type` (power `power`, its n-th part) does to
   group `group` of building b (strength `strength`): none to the main
   group (which counts as the target hit, D_803F7807); 100 when the
   target's conditions say (func_802C0284) or while dashing; else by the
   first of D_80305E50's rules for the type and part that applies
   ({type, part, rule, factor, ...}): times the factor while D_803F7804
   (0xFC) or the hole's state allows (0xFB, func_802C038C), by the push
   (0xFF forward, 0xFE back, 0xFD either: |D_803F77FC| * factor / 16), or
   by a vehicle part's state (func_802A04BC) between two factors; over
   the strength.  The type 0xFF (an explosion) does 100 at strength 1,
   else none.  Into D_8036CB2A (before the strength) and D_8036CB2C. */
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
    if (M_MAIN(B_MODEL(b)) == group) {
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
#define SKIP_RULE                                                           \
    {                                                                       \
        r = rule_skip(r);                                                   \
        continue;                                                           \
    }
    for (r = D_80305E50;;) {
        ENGINE_BLK(802C005C);
        if (r[0] != type)
            SKIP_RULE
        ENGINE_BLK(802C0068);
        if (r[1] != n)
            SKIP_RULE
        ENGINE_BLK(802C0074);
        t = r[2];
        if (t == 0xFC) {
            ENGINE_BLK(802C0084);
            if (D_803F7804 == 0)
                SKIP_RULE
            ENGINE_BLK(802C0094);
            ENGINE_BLK(802C01C0);
            d = r[3] * d;
            goto divide;
        }
        ENGINE_BLK(802C009C);
        if (t == 0xFB) {
            ENGINE_BLK(802C00A8);
            s = func_802C038C(sel, b);
            ENGINE_BLK(802C00B0);
            if (!s)
                SKIP_RULE
            ENGINE_BLK(802C00B8);
            ENGINE_BLK(802C01D4);
            d = r[3] * d;
            goto divide;
        }
        ENGINE_BLK(802C00C0);
        if (t == 0xFF) {
            ENGINE_BLK(802C00CC);
            if (D_803F77FC < 0)
                SKIP_RULE
            ENGINE_BLK(802C00E0);
            break;
        }
        ENGINE_BLK(802C00E8);
        if (t == 0xFE) {
            ENGINE_BLK(802C00F4);
            if (D_803F77FC > 0)
                SKIP_RULE
            ENGINE_BLK(802C0108);
            break;
        }
        ENGINE_BLK(802C0110);
        if (t == 0xFD)
            break;
        /* the state of a part of the vehicle's (D_803F77D0) */
        ENGINE_BLK(802C011C);
        sel = t;
        s = func_802A04BC(t, D_803F77D0, &f11, &f12, &f14, &fC, &fE, &f13, &f4);
        ENGINE_BLK(802C0128);
        if (s != 0) {
            ENGINE_BLK(802C0130);
            r += f13;
            s = engine_cvt_w_s(f4 * (f32)(r[5] - r[4]));
            d = (u32)(s + r[4]) * d;
            goto divide;
        }
        ENGINE_BLK(802C01A0);
        r += r[3] + 4;
    }
#undef SKIP_RULE
    /* by the push */
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
    if (strength == 0) {
        ENGINE_BLK(802C0244);
        engine_break(0x802C0244, 7);
    }
    ENGINE_BLK(802C0248);
    D_8036CB2C = d;
out:
    ENGINE_BLK(802C024C);
    return d;
}

/* whether the push (D_803F780C), this level's rules (D_80305E10: level,
   types; on consecutive hits, D_803A740C) or the types 1, 10 and 16 at
   strength 1 make the hit 100 (*power_out 100); else *power_out stays
   `power` */
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
        for (p = D_80305E10; ENGINE_BLK(802C02E0), p[1] != 0; p += 2) {
            ENGINE_BLK(802C02EC);
            if ((p[1] & (1u << (type & 31))) == 0)
                continue;
            ENGINE_BLK(802C0300);
            if (p[0] != (u32)D_802E8BDC)
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

/* whether group `group` (1-based) of building b can be hit from below:
   every group it rests on (MS_SUPPORTS) that lists it is destroyed, and
   its height (MS_HEIGHTS) is under the player's + 0x578 */
REGS(v0, t9 -> t3)
s32 func_802C038C(s32 group, Building *b) {
    u8 *r, *end = B_SECTION(b, MS_END), *s;
    s32 k;

    ENGINE_BLK(802C038C);
    for (r = B_SECTION(b, MS_SUPPORTS); ENGINE_BLK(802C03D4), r != end; r = SUP_NEXT(r)) {
        ENGINE_BLK(802C03DC);
        for (k = r[2], s = r + 3; ENGINE_BLK(802C03E8), k != 0; k--, s += 2) {
            ENGINE_BLK(802C03F0);
            if (s[0] == group) {
                ENGINE_BLK(802C0408);
                if (B_DAMAGE(b)[r[0] - 1] == 100)
                    break;
                ENGINE_BLK(802C0420);
                ENGINE_BLK(802C046C);
                return 0;
            }
            ENGINE_BLK(802C0400);
        }
        ENGINE_BLK(802C0428);
    }
    ENGINE_BLK(802C0438);
    if (D_803A73F4 + 0x578 < ((s16 *)B_SECTION(b, MS_HEIGHTS))[group - 1] << 5) {
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
    s32 i;

    ENGINE_BLK(802C049C);
    for (i = 0; ENGINE_BLK(802C04C0), i < NFX; i++) {
        ENGINE_BLK(802C04C8);
        FX_B(D_803F3968[i], FX_DONE) = 1;
    }
    ENGINE_BLK(802C04D8);
}

/* effect record `src` copied into the first free one (none if none) */
REGS(v1)
void func_802C04F0(u8 *src) {
    s32 i;

    ENGINE_BLK(802C04F0);
    for (i = 0; ENGINE_BLK(802C0514), i < NFX; i++) {
        ENGINE_BLK(802C051C);
        if (FX_B(D_803F3968[i], FX_DONE) != 0) {
            /* (fourteen words: the copy's loop) */
            ENGINE_BLK(802C0534);
            ENGINE_BLKN(802C0538, FX_SIZE / 4 + 1);
            ENGINE_BLKN(802C0540, FX_SIZE / 4);
            __builtin_memcpy(D_803F3968[i], src, FX_SIZE);
            break;
        }
        ENGINE_BLK(802C0528);
    }
    ENGINE_BLK(802C0558);
}

/* Each frame (hd.c): each waiting effect counts down (FX_DELAY) and then
   starts (60F60's func_802A6274: of a kind D_80306270 picks for its type
   at random, the "big" ones with a3 1), with its sound unless the game
   is ending; one with a radius does damage around (func_802C18D4). */
void func_802C0574(void) {
    u8 *fx, *kinds;
    s32 i, t, k, big, started;

    ENGINE_BLK(802C0574);
    for (i = 0; ENGINE_BLK(802C05A8), i < NFX; i++) {
        fx = D_803F3968[i];
        ENGINE_BLK(802C05B0);
        if (FX_B(fx, FX_DONE) != 0)
            goto next;
        ENGINE_BLK(802C05C0);
        t = FX_H(fx, FX_DELAY);
        if (t != 0) {
            ENGINE_BLK(802C05CC);
            FX_H(fx, FX_DELAY) = t - 1;
            goto next;
        }
        ENGINE_BLK(802C05D8);
        kinds = D_80306270[FX_B(fx, FX_KIND)];
        k = kinds[1 + engine_remu((u32)D_803649D8 >> 4, kinds[0])];
        if (kinds[0] == 0) {
            ENGINE_BLK(802C0638);
            engine_break(0x802C0638, 7);
        }
        ENGINE_BLK(802C063C);
        big = 1;
        if (FX_B(fx, FX_KIND) != 0x15) {
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
        if (big)
            ENGINE_BLK(802C06A8);
        else
            ENGINE_BLK(802C06F0);
        started = func_802A6274(D_802C3FFC[k], FX_W(fx, FX_SPEED), 0, FX_W(fx, FX_X), FX_W(fx, FX_Y),
                                FX_W(fx, FX_Z), FX_W(fx, FX_VEL), FX_W(fx, FX_VEL + 4), FX_W(fx, FX_VEL + 8),
                                FX_W(fx, FX_UNK14), FX_W(fx, FX_UNK14 + 4), FX_W(fx, FX_UNK14 + 8),
                                FX_W(fx, FX_UNK28), FX_B(fx, FX_UNK35), big);
        if (big)
            ENGINE_BLK(802C06E8);
        ENGINE_BLK(802C0730);
        if (started != 0) {
            ENGINE_BLK(802C0738);
            if (D_80364A98 == 0) {
                ENGINE_BLK(802C0748);
                if (D_803643D6 == 0) {
                    ENGINE_BLK(802C0758);
                    func_802619D0(FX_B(fx, FX_KIND));
                    ENGINE_BLK(802C07E4);
                }
            }
        }
        ENGINE_BLK(802C0868);
        FX_B(fx, FX_DONE) = 1;
        t = FX_H(fx, FX_RADIUS);
        if (t != 0) {
            ENGINE_BLK(802C0884);
            func_802C18D4(t, FX_W(fx, FX_X), FX_W(fx, FX_Y), FX_W(fx, FX_Z), FX_B(fx, FX_AMOUNT) << 4);
        }
    next:
        ENGINE_BLK(802C0890);
    }
    ENGINE_BLK(802C0898);
}

/* ---- smoke, dust and the falling groups' shadows --------------------- */

/* The smoke clouds (D_803F0900): each fading (its alpha down by
   SMOKE_FADE a frame) is drawn at that alpha; a gone one waits SMOKE_WAIT
   frames before it can be used again. */
REGS(v0)
void func_802C08C4(u32 *g) {
    Smoke *s;
    s32 i, a;

    ENGINE_BLK(802C08C4);
    for (i = 0, s = D_803F0900; ENGINE_BLK(802C08EC), i < NSMOKE; i++, s++) {
        ENGINE_BLK(802C08F4);
        a = s->alpha;
        if (a == 0) {
            ENGINE_BLK(802C0904);
            if (s->wait != 0) {
                ENGINE_BLK(802C0910);
                s->wait--;
            }
            ENGINE_BLK(802C0918);
            continue;
        }
        ENGINE_BLK(802C0920);
        a -= SMOKE_FADE;
        if (a <= 0) {
            ENGINE_BLK(802C092C);
            s->alpha = 0;
            s->wait = SMOKE_WAIT;
            continue;
        }
        ENGINE_BLK(802C0940);
        s->alpha = a;
        g[0] = DL_PIPESYNC;
        g[1] = 0;
        g[2] = DL_ENVCOLOR;             /* the environment colour: alpha */
        g[3] = a;
        g[4] = DL_CALL;                 /* the cloud's list */
        g[5] = K0(s->dl);
        g += 6;
    }
    ENGINE_BLK(802C0988);
    g[0] = DL_END;
    g[1] = 0;
}

/* whether the look at r (a GroupDl of group `group`'s, 1-based) is shown
   the moment the group goes: no condition hides it, the group itself
   counted at GROUP_DYING_DAMAGE; blocks: the test, a condition, the group
   itself, another, after, below, below and hidden, over */
typedef struct LookBlks {
    Blk test, cond, self, other, after, below, below_hit, over;
} LookBlks;

static s32 look_shown(u8 *r, s32 group, Building *b, const LookBlks *k) {
    GroupCond *c;
    s32 n, dmg;

    for (n = GDL_N(r), c = GDL_COND(r); BLKT(k->test), n != 0; n--, c++) {
        BLKT(k->cond);
        if (c->group == group) {
            BLKT(k->self);
            dmg = GROUP_DYING_DAMAGE;
        } else {
            BLKT(k->other);
            dmg = B_DAMAGE(b)[c->group - 1];
        }
        BLKT(k->after);
        if (c->below) {
            BLKT(k->below);
            if (dmg < c->limit) {
                BLKT(k->below_hit);
                return 0;
            }
        } else {
            BLKT(k->over);
            if (c->limit < dmg)
                return 0;
        }
    }
    return 1;
}

/* A display list's commands from src to src_end copied to *d (through
   func_802C0C64 when `subst`) while *k, counting down, lasts: 0 if it ran
   out.  Blocks: the test, the count's, the copy, after func_802C0C64. */
typedef struct CopyNBlks {
    Blk test, count, copy, after;
} CopyNBlks;

static s32 copy_cmds_n(u32 **d, u32 *src, u32 *src_end, s32 *k, s32 subst, const CopyNBlks *bk) {
    for (; BLKT(bk->test), src != src_end; src += 2) {
        BLKT(bk->count);
        if (--*k == 0)
            return 0;
        BLKT(bk->copy);
        if (subst) {
            *(u64 *)*d = func_802C0C64(*(u64 *)src);
            BLKT(bk->after);
        } else
            *(u64 *)*d = *(u64 *)src;
        *d += 2;
    }
    return 1;
}

/* A smoke cloud of group `group` of building b (a model without falling
   shadows), in the first free one: the group's looks shown as it goes
   copied (at most SMOKE_DL_MAX - 1 commands, their textures through
   D_802F46C0), to fade from SMOKE_ALPHA. */
REGS(t3, t9)
void func_802C09B8(s32 group, Building *b) {
    static const LookBlks lk = {
        {ENGINE_BLK_802C0ADC}, {ENGINE_BLK_802C0AE4}, {ENGINE_BLK_802C0AFC}, {ENGINE_BLK_802C0B04},
        {ENGINE_BLK_802C0B14}, {ENGINE_BLK_802C0B1C}, {ENGINE_BLK_802C0B24}, {ENGINE_BLK_802C0B2C},
    };
    static const CopyNBlks c0 = {
        {ENGINE_BLK_802C0B58}, {ENGINE_BLK_802C0B60}, {ENGINE_BLK_802C0B6C}, {ENGINE_BLK_802C0B74},
    };
    static const CopyNBlks c1 = {
        {ENGINE_BLK_802C0B94}, {ENGINE_BLK_802C0B9C}, {ENGINE_BLK_802C0BA8}, {ENGINE_BLK_802C0BB0},
    };
    u8 *model = B_MODEL(b), *r, *end;
    Smoke *s;
    GroupDls *dls;
    u32 *d;
    s32 i, k;

    ENGINE_BLK(802C09B8);
    if (M_SHADOWS(model) != 0)
        goto out;
    ENGINE_BLK(802C0A48);
    for (i = NSMOKE, s = D_803F0900;; i--, s++) {
        ENGINE_BLK(802C0A54);
        if (i == 0)
            goto out;
        ENGINE_BLK(802C0A5C);
        if (s->alpha == 0) {
            ENGINE_BLK(802C0A68);
            if (s->wait == 0)
                break;
        }
        ENGINE_BLK(802C0A74);
    }
    ENGINE_BLK(802C0A80);
    d = s->dl;
    d[0] = DL_SEGMENT(9);
    d[1] = K0(M_TEXTURES(model));
    d += 2;
    k = SMOKE_DL_MAX;
    end = B_SECTION(b, MS_FALLS);
    for (r = B_SECTION(b, MS_GROUP_DLS); ENGINE_BLK(802C0AC0), r != end; r = GDL_NEXT(r)) {
        ENGINE_BLK(802C0AC8);
        if (GDL_GROUP(r) != group || !look_shown(r, group, b, &lk)) {
            ENGINE_BLK(802C0B38);
            continue;
        }
        ENGINE_BLK(802C0B48);
        dls = GDL_DLS(r, GDL_N(r));
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl0), (u32 *)(model + dls->dl0_end), &k, 1, &c0))
            goto out;
        ENGINE_BLK(802C0B84);
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl1), (u32 *)(model + dls->dl1_end), &k, 1, &c1))
            goto out;
        ENGINE_BLK(802C0BC0);
    }
    ENGINE_BLK(802C0BC8);
    s->alpha = SMOKE_ALPHA;
    d[0] = DL_END;
    d[1] = 0;
out:
    ENGINE_BLK(802C0BE0);
}

/* a display list command as a smoke cloud has it: D_802F46C0's pairs
   (twelve) replace some */
REGS(t4 -> t4)
u64 func_802C0C64(u64 cmd) {
    u64 *p;
    s32 n;

    ENGINE_BLK(802C0C64);
    for (n = 12, p = D_802F46C0; ENGINE_BLK(802C0C84), n != 0; n--, p += 2) {
        ENGINE_BLK(802C0C8C);
        if (p[0] == cmd) {
            ENGINE_BLK(802C0CA0);
            cmd = p[1];
            break;
        }
    }
    ENGINE_BLK(802C0CA4);
    return cmd;
}

/* The dust (D_803EFED0): rising faster each frame (by DUST_ACCEL) until
   it is up (Dust.top), shaken about (two rolls, DUST_SHAKE either way),
   drawn with this frame's matrix; then it waits DUST_WAIT frames before
   it can be used again.  D_803F7810 while it rises. */
REGS(v0)
void func_802C0CBC(u32 *g) {
    Dust *s;
    s32 i, h, y, x, z;
    s32 *m;

    ENGINE_BLK(802C0CBC);
    D_803F7810 = 0;
    for (i = 0, s = D_803EFED0; ENGINE_BLK(802C0D08), i < NDUST; i++, s++) {
        ENGINE_BLK(802C0D10);
        if (s->active == 0) {
            ENGINE_BLK(802C0D20);
            if (s->wait != 0) {
                ENGINE_BLK(802C0D2C);
                s->wait--;
            }
            ENGINE_BLK(802C0D34);
            continue;
        }
        ENGINE_BLK(802C0D3C);
        h = s->speed + DUST_ACCEL;
        y = s->y + h;
        s->speed = h;
        if (y >= s->top) {
            ENGINE_BLK(802C0D5C);
            s->active = 0;
            s->wait = DUST_WAIT;
            continue;
        }
        ENGINE_BLK(802C0D70);
        s->y = y;
        D_803F7810 = 1;
        x = func_802BFB50(-DUST_SHAKE, DUST_SHAKE);
        ENGINE_BLK(802C0D8C);
        x <<= 12;
        z = func_802BFB50(-DUST_SHAKE, DUST_SHAKE);
        ENGINE_BLK(802C0D9C);
        y = -(y << 11);
        z <<= 12;
        if (D_8035805C != 0) {
            ENGINE_BLK(802C0DB4);
            m = s->mtx[1];
        } else {
            ENGINE_BLK(802C0DBC);
            m = s->mtx[0];
        }
        ENGINE_BLK(802C0DC0);
        func_802ACA60(x, y, z, m);
        ENGINE_BLK(802C0DC8);
        func_802AC8CC((u32 *)m);
        ENGINE_BLK(802C0DD0);
        g[0] = DL_PIPESYNC;
        g[1] = 0;
        g += 2;
        if (D_8035805C != 0) {
            ENGINE_BLK(802C0DF0);
            m = s->mtx[1];
        } else {
            ENGINE_BLK(802C0DF8);
            m = s->mtx[0];
        }
        ENGINE_BLK(802C0DFC);
        g[0] = DL_SEGMENT(12);          /* the matrix */
        g[1] = K0(m);
        g[2] = DL_CALL;                 /* the cloud */
        g[3] = K0(s->dl);
        g += 4;
    }
    ENGINE_BLK(802C0E40);
    g[0] = DL_END;
    g[1] = 0;
}

/* The dust of group `group` of building b (a model D_80305E38 lists; not
   in some game modes), if the cloud is free: its corners (func_802C1214),
   D_802F4780's six commands, the group's looks shown as it goes (at most
   DUST_DL_MAX - 1 commands), to rise from the group's height (MS_HEIGHTS)
   to the building's. */
REGS(t3, t9)
void func_802C0E8C(s32 group, Building *b) {
    static const LookBlks lk = {
        {ENGINE_BLK_802C104C}, {ENGINE_BLK_802C1054}, {ENGINE_BLK_802C106C}, {ENGINE_BLK_802C1074},
        {ENGINE_BLK_802C1084}, {ENGINE_BLK_802C108C}, {ENGINE_BLK_802C1094}, {ENGINE_BLK_802C109C},
    };
    static const CopyNBlks c0 = {
        {ENGINE_BLK_802C10C8}, {ENGINE_BLK_802C10D0}, {ENGINE_BLK_802C10DC}, {0, 0},
    };
    static const CopyNBlks c1 = {
        {ENGINE_BLK_802C1100}, {ENGINE_BLK_802C1108}, {ENGINE_BLK_802C1114}, {0, 0},
    };
    u8 *model = B_MODEL(b), *r, *end;
    s16 *id;
    Dust *s;
    GroupDls *dls;
    u32 *d;
    u64 *c;
    s32 i, k;

    ENGINE_BLK(802C0E8C);
    if ((D_80364A90 & 0x440) != 0)
        goto out;
    ENGINE_BLK(802C0F1C);
    for (id = D_80305E38;; id++) {
        ENGINE_BLK(802C0F28);
        if (B_ID(b) == *id)
            break;
        ENGINE_BLK(802C0F34);
        if (*id < 0)
            goto out;
        ENGINE_BLK(802C0F3C);
    }
    ENGINE_BLK(802C0F44);
    for (i = NDUST, s = D_803EFED0;; i--, s++) {
        ENGINE_BLK(802C0F50);
        if (i == 0)
            goto out;
        ENGINE_BLK(802C0F58);
        if (s->active == 0) {
            ENGINE_BLK(802C0F64);
            if (s->wait == 0)
                break;
        }
        ENGINE_BLK(802C0F70);
    }
    ENGINE_BLK(802C0F7C);
    func_802C1214(s, b);
    ENGINE_BLK(802C0F84);
    d = s->dl;
    d[0] = DL_SEGMENT(9);               /* the model's textures */
    d[1] = K0(M_TEXTURES(model));
    d[2] = DL_SEGMENT(11);              /* the corners */
    d[3] = K0(s->corners);
    d += 4;
    for (i = 6, c = D_802F4780; ENGINE_BLK(802C0FE0), i != 0; i--) {
        ENGINE_BLK(802C0FE8);
        *(u64 *)d = *c++;
        d += 2;
    }
    ENGINE_BLK(802C1000);
    d[0] = DL_MTX_PUSH;                 /* segment 12's matrix */
    d[1] = 0x0C000000;
    d += 2;
    k = DUST_DL_MAX;
    end = B_SECTION(b, MS_FALLS);
    for (r = B_SECTION(b, MS_GROUP_DLS); ENGINE_BLK(802C1030), r != end; r = GDL_NEXT(r)) {
        ENGINE_BLK(802C1038);
        if (GDL_GROUP(r) != group || !look_shown(r, group, b, &lk)) {
            ENGINE_BLK(802C10A8);
            continue;
        }
        ENGINE_BLK(802C10B8);
        dls = GDL_DLS(r, GDL_N(r));
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl0), (u32 *)(model + dls->dl0_end), &k, 0, &c0))
            goto out;
        ENGINE_BLK(802C10F0);
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl1), (u32 *)(model + dls->dl1_end), &k, 0, &c1))
            goto out;
        ENGINE_BLK(802C1128);
    }
    ENGINE_BLK(802C1130);
    s->y = 0;
    s->top = (((s16 *)B_SECTION(b, MS_HEIGHTS))[group - 1] << 5) - b->y;
    s->speed = 0;
    s->active = 1;
    d[0] = DL_MTX_POP;
    d[1] = 0;
    d[2] = DL_END;
    d[3] = 0;
out:
    ENGINE_BLK(802C1190);
}

/* the dust's four corners: the model's shadow rectangle (MS_SHADOW)
   DUST_MARGIN bigger, at the building's height */
REGS(v1, t9)
void func_802C1214(Dust *s, Building *b) {
    s16 *r = (s16 *)B_SECTION(b, MS_SHADOW);
    s32 y = b->y >> 5, x0 = r[0] - DUST_MARGIN, z0 = r[1] - DUST_MARGIN, x1 = r[2] + DUST_MARGIN,
        z1 = r[3] + DUST_MARGIN;

    ENGINE_BLK(802C1214);
    s->corners[0][0] = x0, s->corners[0][1] = y, s->corners[0][2] = z0;
    s->corners[1][0] = x1, s->corners[1][1] = y, s->corners[1][2] = z0;
    s->corners[2][0] = x0, s->corners[2][1] = y, s->corners[2][2] = z1;
    s->corners[3][0] = x1, s->corners[3][1] = y, s->corners[3][2] = z1;
}

/* The falling groups' shadows (D_803F1BE0): each growing (its growth up
   by SHADOW_ACCEL a frame, to SHADOW_SIZE_MAX; then SHADOW_HOLD frames
   more) drawn with its matrix (func_8026A454, from D_803F7658 on), into
   both lists; then it waits SHADOW_WAIT frames before it can be used
   again.  Returns g0 (and g1). */
REGS(a0, a1 -> a0, a1)
u32 *func_802C12E0(u32 *g0, u32 *g1, u32 **g1_out) {
    FallShadow *s;
    Mtx *m = D_803F7658;
    s32 i, sz, sz2, gr;

    ENGINE_BLK(802C12E0);
    for (i = 0, s = D_803F1BE0; ENGINE_BLK(802C1304), i < NSHADOW; i++, s++, m = (Mtx *)((u8 *)m + 0x40)) {
        ENGINE_BLK(802C130C);
        if (s->active == 0) {
            ENGINE_BLK(802C131C);
            if (s->wait != 0) {
                ENGINE_BLK(802C1328);
                s->wait--;
            }
            ENGINE_BLK(802C1400);
            continue;
        }
        ENGINE_BLK(802C1334);
        gr = s->growth;
        sz = s->size + gr;
        gr += SHADOW_ACCEL;
        if (sz < SHADOW_SIZE_MAX) {
            ENGINE_BLK(802C134C);
            s->growth = gr;
            s->size = sz;
        } else {
            ENGINE_BLK(802C1358);
            sz2 = s->held + 1;
            s->held = sz2;
            if (sz2 >= SHADOW_HOLD) {
                ENGINE_BLK(802C140C);
                s->active = 0;
                s->wait = SHADOW_WAIT;
                continue;
            }
        }
        ENGINE_BLK(802C136C);
        func_8026A454(s->x, s->y, s->z, sz, s->heading, m);
        ENGINE_BLK(802C138C);
        g0[0] = DL_SEGMENT(9);          /* the model's textures */
        g0[1] = s->textures - 0x80000000;
        g1[0] = DL_SEGMENT(9);
        g1[1] = s->textures - 0x80000000;
        g0[2] = DL_CALL;
        g0[3] = K0(s->dl0);
        g1[2] = DL_CALL;
        g1[3] = K0(s->dl1);
        g0 += 4;
        g1 += 4;
        ENGINE_BLK(802C1400);
    }
    ENGINE_BLK(802C1424);
    *g1_out = g1;
    return g0;
}

/* A shadow for group `group` of building b as it starts to fall (a model
   with falling shadows; unless a lower group would hide it,
   func_802C1A28): the group's looks shown as it goes copied
   (SHADOW_DL_MAX - 1 commands a list at most) into the first free slot
   with their matrix (segment 2, one per slot), at the group's centre,
   facing the player. */
REGS(t3, t9)
void func_802C1438(s32 group, Building *b) {
    static const LookBlks lk = {
        {ENGINE_BLK_802C157C}, {ENGINE_BLK_802C1584}, {ENGINE_BLK_802C159C}, {ENGINE_BLK_802C15A4},
        {ENGINE_BLK_802C15B4}, {ENGINE_BLK_802C15BC}, {ENGINE_BLK_802C15C4}, {ENGINE_BLK_802C15CC},
    };
    static const CopyNBlks c0 = {
        {ENGINE_BLK_802C15F8}, {ENGINE_BLK_802C1600}, {ENGINE_BLK_802C160C}, {0, 0},
    };
    static const CopyNBlks c1 = {
        {ENGINE_BLK_802C1630}, {ENGINE_BLK_802C1638}, {ENGINE_BLK_802C1644}, {0, 0},
    };
    static const HeadingBlks hk = {
        {ENGINE_BLK_802C1748}, {ENGINE_BLK_802C1778}, {ENGINE_BLK_802C1780}, {ENGINE_BLK_802C17B0},
        {ENGINE_BLK_802C17C8}, {ENGINE_BLK_802C17F8}, {ENGINE_BLK_802C1804}, {ENGINE_BLK_802C1834},
        {ENGINE_BLK_802C173C}, {ENGINE_BLK_802C17BC},
    };
    u8 *model = B_MODEL(b), *r, *end;
    FallShadow *s;
    GroupDls *dls;
    s16 *c;
    u32 *d0, *d1, seg = 0x02000000;
    s32 i, k0, k1, t, h;

    ENGINE_BLK(802C1438);
    if (M_SHADOWS(model) == 0)
        goto out;
    ENGINE_BLK(802C14C8);
    t = func_802C1A28(group, b);
    ENGINE_BLK(802C14D0);
    if (t != 0)
        goto out;
    ENGINE_BLK(802C14D8);
    for (i = NSHADOW, s = D_803F1BE0;; i--, s++, seg += 0x40) {
        ENGINE_BLK(802C14E8);
        if (i == 0)
            goto out;
        ENGINE_BLK(802C14F0);
        if (s->active == 0) {
            ENGINE_BLK(802C14FC);
            if (s->wait == 0)
                break;
        }
        ENGINE_BLK(802C1508);
    }
    ENGINE_BLK(802C1518);
    s->textures = (u32)M_TEXTURES(model);
    d0 = s->dl0;
    d1 = s->dl1;
    d0[0] = DL_MTX_PUSH;                /* its matrix */
    d0[1] = seg;
    d1[0] = DL_MTX_PUSH;
    d1[1] = seg;
    d0 += 2;
    d1 += 2;
    k0 = SHADOW_DL_MAX;
    k1 = SHADOW_DL_MAX;
    end = B_SECTION(b, MS_FALLS);
    for (r = B_SECTION(b, MS_GROUP_DLS); ENGINE_BLK(802C1560), r != end; r = GDL_NEXT(r)) {
        ENGINE_BLK(802C1568);
        if (GDL_GROUP(r) != group || !look_shown(r, group, b, &lk)) {
            ENGINE_BLK(802C15D8);
            continue;
        }
        ENGINE_BLK(802C15E8);
        dls = GDL_DLS(r, GDL_N(r));
        if (!copy_cmds_n(&d0, (u32 *)(model + dls->dl0), (u32 *)(model + dls->dl0_end), &k0, 0, &c0))
            goto out;
        ENGINE_BLK(802C1620);
        if (!copy_cmds_n(&d1, (u32 *)(model + dls->dl1), (u32 *)(model + dls->dl1_end), &k1, 0, &c1))
            goto out;
        ENGINE_BLK(802C1658);
    }
    ENGINE_BLK(802C1660);
    d0[0] = DL_MTX_POP;
    d0[1] = 0;
    d0[2] = DL_END;
    d0[3] = 0;
    d1[0] = DL_MTX_POP;
    d1[1] = 0;
    d1[2] = DL_END;
    d1[3] = 0;
    s->active = 1;
    s->size = 0;
    s->growth = 1;
    s->held = 0;
    s->y = b->y >> 5;
    c = (s16 *)(B_SECTION(b, MS_CENTRES) + (group - 1) * 8);
    s->x = c[0];
    s->z = c[2];
    h = heading_to(c[0] << 5, c[2] << 5, D_803A73F0, D_803A73F8, &hk);
    ENGINE_BLK(802C183C);
    h -= 0x400;
    if (h < 0) {
        ENGINE_BLK(802C1848);
        h += 0xFFF;
    }
    ENGINE_BLK(802C184C);
    s->heading = h;
out:
    ENGINE_BLK(802C1850);
}

/* An explosion of radius r (<< 5) at (x, y, z) (16.16): each building it
   reaches but the level's goal takes amount (over its strength) in every
   intact group, after a while (func_802BC888). */
REGS(a1, t3, t4, t5, t6)
void func_802C18D4(s32 r, s32 x, s32 y, s32 z, s32 amount) {
    Building *b;
    s32 cx = (u32)x >> 11, cy = (u32)y >> 11, cz = (u32)z >> 11, n, g, a, hit;
    u8 *d;

    ENGINE_BLK(802C18D4);
    r <<= 5;
    for (b = D_803F4030; ENGINE_BLK(802C1940), b != D_803F7654; b++) {
        ENGINE_BLK(802C1948);
        hit = func_8029CFA4(cx, cy, cz, r, b->x, b->y, b->z, B_RADIUS(b));
        ENGINE_BLK(802C195C);
        if (!hit)
            goto next;
        ENGINE_BLK(802C1964);
        if (B_ID(b) == MODEL_GOAL)
            goto next;
        ENGINE_BLK(802C1974);
        for (g = 1, n = B_NGROUPS(b), d = B_DAMAGE(b); ENGINE_BLK(802C1980), n != 0; g++, n--, d++) {
            ENGINE_BLK(802C1988);
            if (*d != 100) {
                ENGINE_BLK(802C199C);
                if (M_STRENGTH(B_MODEL(b)) == 0) {
                    ENGINE_BLK(802C19B4);
                    engine_break(0x802C19B4, 7);
                }
                ENGINE_BLK(802C19B8);
                a = engine_divu(amount, M_STRENGTH(B_MODEL(b)));
                func_802BC888(a, g, b);
            }
            ENGINE_BLK(802C19C8);
        }
    next:
        ENGINE_BLK(802C19D4);
    }
    ENGINE_BLK(802C19DC);
}

/* whether a group of building b's falls lower than group `group` (the
   fall heights, MS_FALLS, M_NHEIGHTS of them): the count left when one is
   found, else 0 */
REGS(t3, t9 -> a2)
s32 func_802C1A28(s32 group, Building *b) {
    s16 *f = (s16 *)B_SECTION(b, MS_FALLS);
    s32 n = M_NHEIGHTS(B_MODEL(b)), h = f[group - 1];

    ENGINE_BLK(802C1A28);
    for (; ENGINE_BLK(802C1A60), n != 0; f++, n--) {
        ENGINE_BLK(802C1A68);
        if (*f < h)
            break;
        ENGINE_BLK(802C1A78);
    }
    ENGINE_BLK(802C1A84);
    return n;
}

/* ---- the level's targets, for the game's C --------------------------- */

/* whether every target building is down by its group set
   (func_802BD064) */
s32 func_802C1AA0(void) {
    Building *b;
    s32 down;

    ENGINE_BLK(802C1AA0);
    for (b = D_803F4030; ENGINE_BLK(802C1AD0), b != D_803F7654; b++) {
        ENGINE_BLK(802C1AD8);
        if (!B_TARGET(b))
            continue;
        ENGINE_BLK(802C1AE8);
        down = func_802BD064(b + 1);
        ENGINE_BLK(802C1AF0);
        if (!down) {
            ENGINE_BLK(802C1AF8);
            ENGINE_BLK(802C1AFC);
            return 0;
        }
    }
    ENGINE_BLK(802C1AFC);
    return 1;
}

/* how many target buildings are down */
u8 func_802C1B1C(void) {
    Building *b;
    s32 n = 0, down;

    ENGINE_BLK(802C1B1C);
    for (b = D_803F4030; ENGINE_BLK(802C1B4C), b != D_803F7654; b++) {
        ENGINE_BLK(802C1B54);
        if (!B_TARGET(b))
            continue;
        ENGINE_BLK(802C1B64);
        down = func_802BD064(b + 1);
        ENGINE_BLK(802C1B6C);
        if (down) {
            ENGINE_BLK(802C1B74);
            n++;
        }
    }
    ENGINE_BLK(802C1B7C);
    return n;
}

/* The nearest target to the carrier (D_803EF6DC...), as func_802BCE40
   finds the player's: its position in D_803F7670..78, its distance in
   D_803F7660 (and returned).  A target building or object behind it (in
   z) only within its radius, or not at all. */
s32 func_802C1B9C(void) {
    static const ObjBlks k = {
        {ENGINE_BLK_802C1D48}, {ENGINE_BLK_802C1D58}, {ENGINE_BLK_802C1D60},
        {ENGINE_BLK_802C1D68}, {ENGINE_BLK_802C1D70},
    };
    Building *b;
    s32 cx = D_803EF6DC, cy = D_803EF6E0, cz = D_803EF6E4, n, down;
    s64 best = 9999999, d;
    UnkStruct_8039C800 *h;
    TargetObj *o;

    ENGINE_BLK(802C1B9C);
    for (b = D_803F4030; ENGINE_BLK(802C1C04), b != D_803F7654; b++) {
        ENGINE_BLK(802C1C0C);
        if (!B_TARGET(b))
            continue;
        ENGINE_BLK(802C1C18);
        if (B_DESTROYED(b))
            continue;
        ENGINE_BLK(802C1C24);
        d = func_802ABCDC(b->x, b->y, b->z, cx, cy, cz);
        ENGINE_BLK(802C1C34);
        if (!(d < best))
            continue;
        ENGINE_BLK(802C1C40);
        down = func_802BD064(b + 1);
        ENGINE_BLK(802C1C48);
        if (down)
            continue;
        ENGINE_BLK(802C1C50);
        if (b->z < cz) {
            ENGINE_BLK(802C1C5C);
            if (B_RADIUS(b) < d)
                continue;
        }
        ENGINE_BLK(802C1C6C);
        D_803F7670 = b->unk28;
        best = d;
        D_803F7674 = b->y;
        D_803F7678 = b->unk2C;
    }
    ENGINE_BLK(802C1C98);
    for (n = D_8039C940, h = D_8039C800; ENGINE_BLK(802C1CA8), n != 0; n--, h++) {
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
    for (o = D_80306480; ENGINE_BLK(802C1D00), o->level != -1; o++) {
        ENGINE_BLK(802C1D10);
        if (o->level != D_802E8BDC)
            goto next;
        ENGINE_BLK(802C1D20);
        d = func_802ABCDC(o->x, o->y, o->z, cx, cy, cz);
        ENGINE_BLK(802C1D30);
        if (!(d < best))
            goto next;
        ENGINE_BLK(802C1D3C);
        if (o->z < cz)
            goto next;
        ENGINE_BLK(802C1D44);
        if (!obj_destroyed(o, &k)) {
            D_803F7670 = o->x;
            D_803F7678 = o->z;
            best = d;
        }
    next:
        ENGINE_BLK(802C1D90);
    }
    ENGINE_BLK(802C1D98);
    D_803F7660 = (s32)best;
    return (s32)best;
}

/* The level's buildings counted (1D990.c and others): the ones of models
   not of strength 0xFF towards D_8036EB92, the destroyed ones (or all
   with D_803F7688) towards D_8036EA70.bd and their value towards its ip;
   strength 1 counts for the value only.  With `targets`, only the target
   buildings, the last one's model in D_803F7684. */
void func_802C1DD0(s32 targets) {
    Building *b;
    s32 all = 0, down = 0, kind;
    u32 value = 0;
    u8 *model;

    ENGINE_BLK(802C1DD0);
    for (b = D_803F4030; ENGINE_BLK(802C1E18), b != D_803F7654; b++) {
        ENGINE_BLK(802C1E20);
        model = B_MODEL(b);
        if (targets != 0) {
            ENGINE_BLK(802C1E28);
            if (!B_TARGET(b))
                goto next;
            ENGINE_BLK(802C1E34);
            D_803F7684 = B_ID(b);
        }
        ENGINE_BLK(802C1E40);
        kind = M_STRENGTH(model);
        if (kind == 0xFF)
            goto next;
        ENGINE_BLK(802C1E50);
        if (kind != 1) {
            ENGINE_BLK(802C1E58);
            all++;
        }
        ENGINE_BLK(802C1E5C);
        if ((B_DESTROYED(b) | D_803F7688) == 0)
            continue;
        ENGINE_BLK(802C1E74);
        if (kind != 1) {
            ENGINE_BLK(802C1E80);
            down++;
        }
        ENGINE_BLK(802C1E84);
        value += M_VALUE(model);
    next:
        ENGINE_BLK(802C1E8C);
    }
    ENGINE_BLK(802C1E94);
    D_8036EB92 = all;
    D_8036EA70.bd = down;
    D_8036EA70.ip = value;
    D_803F7688 = 0;
}

/* ---- the level file's moving triangles (its 0x74 section) ------------ */

/* The section: a count, then records, each from the section's start: a
   u16 offset of the next (-1 after the last), a part's matrix offsets
   (u16 at 0x14 + 2 * (part - 1)), a count of triangles at 0x28 and
   0x44-byte triangles from 0x2C: three points (u16 x, y, z), and for each
   corner a part's id and its place (s32 at 0x14..0x20, 0x24..0x30,
   0x34..0x40). */
#define L74(l) ((u8 *)(l) + *(u32 *)((u8 *)(l) + 0x74))

/* the section's record n (from 1): its data */
s16 *func_802C1EE0(s32 n) {
    u8 *base = L74(D_80358074), *p;

    ENGINE_BLK(802C1EE0);
    for (p = base + 4, n--; ENGINE_BLK(802C1F04), n != 0; n--) {
        ENGINE_BLK(802C1F0C);
        p = base + *(u16 *)p;
    }
    ENGINE_BLK(802C1F1C);
    return (s16 *)(p + 2);
}

/* record n's part `part` (from 1) placed at (x, y, z) (16.16, 39050.c):
   its matrix (this frame's, D_803F7820/24 plus its offset), and its
   corners with that id moved there */
void func_802C1F30(s32 n, s32 part, s32 x, s32 y, s32 z) {
    u8 *base = L74(D_80358074), *p, *t, *m;
    s32 id, k;

    ENGINE_BLK(802C1F30);
    for (p = base + 4, n--; ENGINE_BLK(802C1F58), n != 0; n--) {
        ENGINE_BLK(802C1F60);
        p = *(u16 *)p + base;
    }
    ENGINE_BLK(802C1F70);
    id = ((u16 *)(p + 0x14))[part - 1];
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
    for (k = *(s32 *)(p + 0x28), t = p + 0x2C; ENGINE_BLK(802C1FE4), k != 0; k--, t += 0x44) {
        s32 *w = (s32 *)t;

        ENGINE_BLK(802C1FEC);
        if (w[5] == id) {
            ENGINE_BLK(802C1FFC);
            w[6] = x, w[7] = y, w[8] = z;
        }
        ENGINE_BLK(802C2008);
        if (w[9] == id) {
            ENGINE_BLK(802C2014);
            w[10] = x, w[11] = y, w[12] = z;
        }
        ENGINE_BLK(802C2020);
        if (w[13] == id) {
            ENGINE_BLK(802C202C);
            w[14] = x, w[15] = y, w[16] = z;
        }
        ENGINE_BLK(802C2038);
    }
    ENGINE_BLK(802C2040);
}

/* each frame (hd.c): the section's triangles made (into D_803F7828's
   0x28-byte records) from their points and their parts' places */
void func_802C2054(void) {
    u8 *base = L74(D_80358074), *r, *t;
    u32 *d = (u32 *)D_803F7828;
    s32 k, last;

    ENGINE_BLK(802C2054);
    if (*(u32 *)base == 0)
        goto out;
    ENGINE_BLK(802C2088);
    t = base + 4;
    do {
        ENGINE_BLK(802C208C);
        r = t;
        last = *(s16 *)r;
        for (k = *(s32 *)(r + 0x28), t = r + 0x2C; ENGINE_BLK(802C209C), k != 0; k--, t += 0x44, d += 10) {
            u16 *pt = (u16 *)t;
            s32 *w = (s32 *)t;

            ENGINE_BLK(802C20A4);
            d[0] = (pt[0] << 5) + w[6];
            d[1] = (pt[1] << 5) + w[7];
            d[2] = (pt[2] << 5) + w[8];
            d[3] = (pt[3] << 5) + w[10];
            d[4] = (pt[4] << 5) + w[11];
            d[5] = (pt[5] << 5) + w[12];
            d[6] = (pt[6] << 5) + w[14];
            d[7] = (pt[7] << 5) + w[15];
            d[8] = (pt[8] << 5) + w[16];
        }
        ENGINE_BLK(802C2168);
    } while (last != -1);
out:
    ENGINE_BLK(802C2174);
}
