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
 */
#include "buildings.h"
#include "level_tables.h"
#include "game/game.h"
#include "game/audio.h"
#include "game/level.h"

/* ---- the data -------------------------------------------------------- */

/* (the level's objects to destroy, TargetObj: level_tables.h) */

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

extern u8 D_803F3FF8[FX_SIZE];          /* the one being made */
extern DelayedHit D_803F7690[NDELAYED];
extern u8 *PTR32 D_803F77D4;            /* the part numbers hit this frame (D_803F77D8...) */
extern u8 D_803F77D8[];
extern u8 *PTR32 D_803F77E4;            /* and last frame's (D_803F77E8...) */
extern u8 D_803F77E8[];
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
/* the chance tables, one variable (asm2c.py's LABEL_TYPES): their walks
   (func_802BF978, func_802BFDAC) run on from one table into the next */
extern u8 D_80306344[0xAC];
#define CHANCES_HIT D_80306344                  /* (D_80306344) */
#define CHANCES_DESTROYED (D_80306344 + 0xC)    /* (D_80306350) */
#define CHANCES_LANDED (D_80306344 + 0x90)      /* (D_803063D4) */
#define DEBRIS_COUNTS (D_80306344 + 0x9C)       /* (D_803063E0) */
extern u8 *PTR32 D_80306270[];
extern u32 D_802C3FFC[];
extern u8 D_8036DCD4, D_8036DCD7;
extern u64 D_802F46C0[24];
extern u64 D_802F4780[6];
extern u8 D_8036EB92;
extern u32 D_80364A40;
extern u16 D_803649E0, D_803649E2, D_803649E4;  /* the spin a falling group starts with */

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

/* ---- the level's end, the targets ------------------------------------ */

/* Whether the level's goal is reached once its goal building is hit: in
   a level that asks nothing else (D_803643DB and D_80364AC1 clear), yes;
   else once every target building is down (D_803F7806), every hole filled
   and every object destroyed (func_802BC5E0's and func_802BDDB4's test) */
static s32 goal_open(void) {
    s32 ok;

    if (D_803643DB == 0) {
        if (D_80364AC1 == 0)
            return 1;
    }
    if (D_803F7806 == 0)
        return 0;
    ok = func_802BC7DC();
    if (!ok)
        return 0;
    ok = func_802BC714();
    return ok;
}

/* Each frame (hd.c): when D_803F7805 is set (the level's goal building
   hit), the level is won if the goal is open: D_803643DA, and the game
   mode moves on. */
void func_802BC5E0(void) {
    if (D_803F7805 != 0) {
        D_803F7805 = 0;
        if (goal_open()) {
            D_803643DA = 1;
            D_802E8BD8 = 1;
            if (D_803643DB == 0) {
                if (D_80364A90 == 0x40) {
                    func_80275390(D_80364A90);
                } else {
                    *((u8 *)&D_803649E8 + NE_X3(0)) = 1;
                    D_80364A98 = 8;
                }
            }
        }
    }
}

/* whether target object o is destroyed: every one of its ids' state
   (func_8029D210) is 0 */
static s32 obj_destroyed(TargetObj *o) {
    s8 *q;
    s32 alive;

    for (q = o->ids; *q != -1; q++) {
        alive = func_8029D210(*q);
        if (alive) {
            return 0;
        }
    }
    return 1;
}

/* whether every object of this level to destroy (D_80306480) is: the ones
   not behind the carrier (z, D_803EF6E4) */
REGS(-> t4)
s32 func_802BC714(void) {
    TargetObj *o;
    s32 lim = D_803EF6E4;

    for (o = D_80306480; o->level != -1; o++) {
        if (o->level == D_802E8BDC) {
            if (!(o->z < lim)) {
                if (!obj_destroyed(o)) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/* whether every hole is filled */
REGS(-> t4)
s32 func_802BC7DC(void) {
    UnkStruct_8039C800 *h = D_8039C800;
    s32 n;

    for (n = D_8039C940; n != 0; n--, h++) {
        if (h->unk26 == 0) {
            return 0;
        }
    }
    return 1;
}

/* ---- the delayed damage ---------------------------------------------- */

/* the queue emptied */
REGS()
void func_802BC840(void) {
    s32 i;

    for (i = 0; i < NDELAYED; i++) {
        D_803F7690[i].frames = 0;
    }
}

/* `amount` of damage to group `group` of building b: added to what is
   queued for it, or queued for 1..DELAY_MAX - 1 frames from now (none if
   the queue is full) */
REGS(a2, t3, t9)
void func_802BC888(s32 amount, s32 group, Building *b) {
    DelayedHit *h;
    s32 i;

    for (i = 0, h = D_803F7690; i < NDELAYED; i++, h++) {
        if (h->frames == 0)
            continue;
        if (h->b != b)
            continue;
        if (h->group != group)
            continue;
        h->amount += amount;
        return;
    }
    for (i = 0, h = D_803F7690; i < NDELAYED; i++, h++) {
        if (h->frames == 0) {
            h->group = group;
            h->amount = amount;
            h->b = b;
            h->frames = func_8026A8E0(1, DELAY_MAX);
            break;
        }
    }
}

/* a queued hit whose time has come, done (the damage capped at 100; at
   100 the group goes) */
static void delayed_hit_apply(DelayedHit *h) {
    Building *b;
    Piece *q;
    s32 amount, g, d;

    amount = h->amount;
    h->frames = 0;
    b = h->b;
    g = h->group;
    D_802E8BE4 = SHAKE_HIT_FRAMES;
    if (amount <= SHAKE_HIT_MAX) {
        D_802E8BE8 = amount;
    } else {
        D_802E8BE8 = SHAKE_HIT_MAX;
    }
    d = B_DAMAGE(b)[g - 1] + amount;
    if (d >= 100) {
        d = 100;
    }
    B_DAMAGE(b)[g - 1] = d;
    if (d == 100) {
        /* the group destroyed: its shadow, smoke (not while the level
           ends), dust, the building's first effect, its pieces off */
        func_802C1438(g, b);
        if (D_803643D6 == 0) {
            if (D_803643D7 == 0) {
                func_802C09B8(g, b);
            }
        }
        func_802C0E8C(g, b);
        if (D_80364A90 != 0x200) {
            if (D_80364A90 != 0x400) {
                func_802BF384(b);
            }
        }
        for (q = b->unk4; q != (Piece *)b->unk8; q++) {
            if (q->group == g) {
                q->active = 0;
            }
        }
    }
    func_802BF668(b);
    func_802BF534(b);
    func_802BF898(g, d, b);
}

/* Each frame (hd.c): the queued hits counted down, those whose time has
   come done */
void func_802BCA2C(void) {
    DelayedHit *h;
    s32 i;

    for (i = 0, h = D_803F7690; i < NDELAYED; i++, h++) {
        if (h->frames != 0) {
            if (h->frames != 1) {
                h->frames--;
            } else {
                delayed_hit_apply(h);
            }
        }
    }
}

/* ---- the parts hit, this frame and the last -------------------------- */

/* (The part numbers: a vehicle type's parts (D_803A6B30) counted from 1,
   the one that touched a piece, func_802BEBB0's n.) */

/* this frame's list emptied */
REGS()
void func_802BCBD8(void) {
    D_803F77D4 = D_803F77D8;
}

/* last frame's list emptied */
REGS()
void func_802BCC10(void) {
    D_803F77E4 = D_803F77E8;
}

/* this frame's list becomes last frame's (emptied while D_80358060 is 0) */
REGS()
void func_802BCC48(void) {
    u8 *d = D_803F77E8, *s;

    if (D_80358060 != 0) {
        for (s = D_803F77D8; s != D_803F77D4;) {
            *d++ = *s++;
        }
    }
    D_803F77E4 = d;
}

/* part `kind` added to this frame's list (once) */
REGS(fp)
void func_802BCCD4(s32 kind) {
    s32 found;

    found = func_802BCD80(kind);
    if (!found) {
        *D_803F77D4 = kind;
        D_803F77D4++;
    }
}

/* whether this frame's list holds anything but `kind` (unused) */
REGS(v0 -> v1)
s32 func_802BCD20(s32 kind) {
    u8 *p;
    s32 r = 0;

    for (p = D_803F77D8; p != D_803F77D4;) {
        if (kind != *p++) {
            r = 1;
            break;
        }
    }
    return r;
}

/* whether this frame's list holds `kind` */
REGS(v0 -> v1)
s32 func_802BCD80(s32 kind) {
    u8 *p;
    s32 r = 0;

    for (p = D_803F77D8; p != D_803F77D4;) {
        if (kind == *p++) {
            r = 1;
            break;
        }
    }
    return r;
}

/* whether last frame's list holds `kind` */
REGS(fp -> v1)
s32 func_802BCDE0(s32 kind) {
    u8 *p;
    s32 r = 0;

    for (p = D_803F77E8; p != D_803F77E4;) {
        if (kind == *p++) {
            r = 1;
            break;
        }
    }
    return r;
}

/* ---- the nearest target ---------------------------------------------- */

/* The nearest target to the player (30C70.c, for the radar): an intact
   target building, an empty hole, or an object to destroy that isn't yet;
   its kind (1, 2, 3; 0 none) in D_803F7808 and its outline in
   D_8036C790. */
s32 func_802BCE40(void) {
    Building *b;
    s32 px = D_803643E0, py = D_803643E4, pz = D_803643E8;
    s64 best = 9999999, d;
    u32 found = 0, outline;
    s32 kind = 0, n, down;
    UnkStruct_8039C800 *h;
    TargetObj *o;

    for (b = D_803F4030; b != D_803F7654; b++) {
        if (!B_TARGET(b))
            continue;
        if (B_DESTROYED(b))
            continue;
        d = func_802ABCDC(b->x, b->y, b->z, px, py, pz);
        if (!(d < best))
            continue;
        down = func_802BD064(b + 1);
        if (down)
            continue;
        best = d;
        found = (u32)b;
        kind = 1;
    }
    for (n = D_8039C940, h = D_8039C800; n != 0; n--, h++) {
        if (h->unk26 == 0) {
            d = func_802ABCDC(h->x, h->y, h->z, px, py, pz);
            if (d < best) {
                kind = 2;
                found = (u32)h;
                best = d;
            }
        }
    }
    for (o = D_80306480; o->level != -1; o++) {
        if (o->level == D_802E8BDC) {
            d = func_802ABCDC(o->x, o->y, o->z, px, py, pz);
            if (d < best) {
                if (!obj_destroyed(o)) {
                    kind = 3;
                    found = (u32)o;
                    best = d;
                }
            }
        }
    }
    outline = 0;
    if (kind != 0) {
        if (kind == 1) {
            outline = (u32)B_SECTION((Building *)found, MS_CORNERS);
        } else {
            if (kind == 3) {
                outline = (u32)((TargetObj *)found)->corners;
            } else {
                outline = (u32)((UnkStruct_8039C800 *)found)->corners;
            }
        }
    }
    D_8036C790 = outline;
    D_803F7808 = kind;
    return found;
}

/* building b's group set (D_803BE708), if it has one */
static GroupSet *group_set_of(Building *b) {
    u8 *sets = D_803BE708;
    GroupSet *s;
    s32 n;

    if (sets == NULL)
        return NULL;
    for (n = *(u32 *)sets, s = (GroupSet *)(sets + 4); n != 0; n--, s++) {
        if (s->b == b)
            return s;
    }
    return NULL;
}

/* whether building `next - 1` belongs to a group set of the level's
   (D_803BE708) all of whose groups are destroyed */
REGS(t0 -> v0)
s32 func_802BD064(Building *next) {
    Building *b = next - 1;
    GroupSet *s;
    u8 *g;
    s32 k;

    s = group_set_of(b);
    if (s != NULL) {
        for (k = s->n, g = s->groups; k != 0; k--, g++) {
            if (B_DAMAGE(b)[*g] != 100)
                break;
        }
        if (k == 0) {
            return 1;
        }
    }
    return 0;
}

/* the distance from the carrier (D_803EF6DC, D_803EF6E4) to target t (of
   kind D_803F7809) in x and z, over D_803EF6FC: into D_8036C7C8 (30C70.c) */
void func_802BD10C(s32 t_) {
    s32 x, z;
    s64 d;

    if (D_803F7809 == 2) {
        x = ((UnkStruct_8039C800 *)t_)->x;
        z = ((UnkStruct_8039C800 *)t_)->z;
    } else {
        if (D_803F7809 == 3) {
            x = ((TargetObj *)t_)->x;
            z = ((TargetObj *)t_)->z;
        } else {
            x = ((Building *)t_)->x;
            z = ((Building *)t_)->z;
        }
    }
    d = func_802ABCDC(x, 0, z, D_803EF6DC, 0, D_803EF6E4);
    if (D_803EF6FC != 0) {
        d = (s32)engine_divu((u32)d, D_803EF6FC);
    }
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

/* Display list commands copied from src to src_end at d, in one go */
#define COPY_CMDS(d, src, src_end)                                          \
    do {                                                                    \
        s32 ncmd_ = ((u8 *)(src_end) - (u8 *)(src)) / 8;                    \
        (d) = copy_cmds((d), (src), ncmd_);                                 \
    } while (0)

/* One look of building b's group gi (0-based): a falling group gets its
   matrix (func_802BE574) pushed around its lists and comes down a frame,
   landing at its fall height (its landing effects, the screen shaken, the
   group destroyed and what rests on it let go); then its two lists, with
   the animated textures put in (anim to anim_end, func_802BE228 and
   func_802BE3C8). */
static void draw_look(Building *b, s32 gi, GroupDls *dls, AnimTex *anim, AnimTex *anim_end, u32 **d0p, u32 **d1p,
                      s32 *fell) {
    u8 *model = B_MODEL(b);
    u32 *d0 = *d0p, *d1 = *d1p, *src, *src_end;
    u32 mtx = 0;
    s32 falling, t, fall;

    falling = B_FALLING(b)[gi];
    if (falling) {
        t = B_FALL_T(b)[gi] + 1;
        B_FALL_T(b)[gi] = t;
        fall = (s32)((u32)t * (u32)t * (u32)-FALL_ACCEL);
        *fell = 1;
        if (!(-(s32)((u32)((u16 *)B_SECTION(b, MS_FALLS))[gi] << 16) < fall)) {
            /* it has landed */
            B_FALLING(b)[gi] = 0;
            func_802BFD1C(b, gi);
            D_802E8BE4 = SHAKE_HIT_FRAMES;
            D_802E8BE8 = SHAKE_LANDED;
            B_DAMAGE(b)[gi] = 100;
            func_802BD85C(b, gi);
        }
        mtx = func_802BE574(b, gi, fall);
        d0[0] = DL_MTX_PUSH;
        d0[1] = mtx;
        d0 += 2;
    }
    src = (u32 *)(model + dls->dl0);
    src_end = (u32 *)(model + dls->dl0_end);
    if (anim != anim_end) {
        d0 = func_802BE228(d0, src_end, anim, anim_end, src, &src);
    } else {
        COPY_CMDS(d0, src, src_end);
    }
    if (falling) {
        d0[0] = DL_MTX_POP;
        d0[1] = 0;
        d0 += 2;
        d1[0] = DL_MTX_PUSH;
        d1[1] = mtx;
        d1 += 2;
    }
    src = (u32 *)(model + dls->dl1);
    src_end = (u32 *)(model + dls->dl1_end);
    if (anim != anim_end) {
        d1 = func_802BE3C8(d1, src_end, anim, anim_end, src, &src);
    } else {
        COPY_CMDS(d1, src, src_end);
    }
    if (falling) {
        d1[0] = DL_MTX_POP;
        d1[1] = 0;
        d1 += 2;
    }
    *d0p = d0;
    *d1p = d1;
}

/* Each visible cell's place in D_803C30A8 (the visible cells, to -1) + 1,
   0 for the others: the original searched the list for each building.
   Marked for one func_802BD1F8 and cleared after it (the cells it
   marked). */
static u16 cell_at[256];
static u8 cells_marked[256];
static s32 ncells_marked;

static void visible_cells_mark(void) {
    s32 n;
    s16 c;

    for (n = 0; (c = D_803C30A8[n]) != -1; n++)
        if ((u16)c < 256 && cell_at[c] == 0) {
            cell_at[c] = n + 1;
            cells_marked[ncells_marked++] = c;
        }
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
    s32 first, n, gi, dmg, i, hidden;
    GroupCond *c;
    s32 fell = 0;

    D_803F7658 = arg4;
    D_803F765C = arg5;
    func_802C08C4((u32 *)g6);
    func_802C0CBC((u32 *)g7);
    if (D_8035805C != 0) {
        FALL_MTX = D_803F24D0;
    } else {
        FALL_MTX = D_803F2ED0;
    }
    visible_cells_mark();
    end = D_803F7654;
    for (b = D_803F4030; b != end; b++) {
        i = cell_at[B_CELL(b)] - 1;
        if (i < 0) {
            goto done;
        }
        if (B_ID(b) == MODEL_GOAL) {
            n = func_802BD8C8();
            if (n)
                goto done;
        }
        model = B_MODEL(b);
        if (M_MOVES(model)) {
            /* a moving building: its matrix (4EBE0.c, from its place
               unk1C..24, its behaviour, its state at 0x38 and its speed
               unk34) pushed, and its parts moved to where it is now */
            s32 ox, oy, oz;

            m = D_803F765C;
            D_803F765C = (Mtx *)((u8 *)m + 0x40);
            func_802933A0(b->unk1C, b->unk20, b->unk24, M_MOVES(model), m, (u8 *)b + 0x38, (Gfx *)g0,
                          (Gfx *)g1, b->unk34, b->x, b->y, b->z);
            g0 += 2;
            g1 += 2;
            ox = b->x, oy = b->y, oz = b->z;
            b->x = D_803F7664;
            b->y = D_803F7668;
            b->z = D_803F766C;
            func_802BD99C(b, D_803F7664 - ox, D_803F7668 - oy, D_803F766C - oz);
        }
        model = B_MODEL(b);
        anim = (AnimTex *)B_SECTION(b, MS_ANIMS);
        anim_end = (AnimTex *)B_SECTION(b, MS_CENTRES);
        func_802BDDB4(b, anim, anim_end);
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
        n = ((u8 *)src_end - (u8 *)src) / 8;
        copy_cmds(d0, src, n);
        copy_cmds(d1, src, n);
        d0 += n * 2;
        d1 += n * 2;
        /* the groups' looks */
        first = 1;
        grp_end = B_SECTION(b, MS_FALLS);
        for (r = B_SECTION(b, MS_GROUP_DLS); r != grp_end; r = next) {
            gi = GDL_GROUP(r) - 1;
            n = GDL_N(r);
            next = GDL_NEXT(r);
            hidden = 0;
            for (c = GDL_COND(r); n != 0; c++) {
                dmg = B_DAMAGE(b)[c->group - 1];
                --n;
                if (c->below) {
                    if (dmg < c->limit) {
                        hidden = 1;
                        break;
                    }
                } else {
                    if (c->limit < dmg) {
                        hidden = 1;
                        break;
                    }
                }
            }
            if (hidden) {
            } else {
                draw_look(b, gi, GDL_DLS(r, GDL_N(r)), anim, anim_end, &d0, &d1, &fell);
                if (first)
                    break;
            }
            first = 0;
        }
    done:
        d0[0] = DL_END;
        d0[1] = 0;
        d1[0] = DL_END;
        d1[1] = 0;
        d0 += 2;
        d1 += 2;
        if (M_MOVES(B_MODEL(b))) {
            g0[0] = DL_MTX_POP;         /* the moving one's matrix popped */
            g0[1] = 0;
            g1[0] = DL_MTX_POP;
            g1[1] = 0;
            g0 += 2;
            g1 += 2;
        }
    }
    visible_cells_clear();
    if (fell)
        g0 = func_802C12E0(g0, g1, &g1);
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

    g++;
    for (p = b->unk4; p != (Piece *)b->unk8; p++) {
        if (p->group != g)
            continue;
        if (p->group2 == 0)
            continue;
        p->active = 0;
    }
}

/* in level 0x32, whether func_80286090 says no (the goal there can't be
   hit, or drawn, until then) */
REGS(-> t1)
s32 func_802BD8C8(void) {
    s32 r = 0, ok;

    if (D_802E8BDC == 0x32) {
        ok = func_80286090(D_802E8BDC);
        if (!ok) {
            r = 1;
        }
    }
    return r;
}

/* the axis piece p is flattest along, from its normal's sizes: z if |nz|
   is the largest, then y, else x */
static void piece_axis(Piece *p, s64 ax, s64 ay, s64 az) {
    if (!(az < ax)) {
        if (!(az < ay)) {
            p->axis = 0;
            return;
        }
    }
    if (!(ay < ax)) {
        if (!(ay < az)) {
            p->axis = 1;
            return;
        }
    }
    p->axis = 2;
}

/* Building b moved by (dx, dy, dz) (<< 5): its model's heights, shadow,
   triangles, corners, effect records and group centres, and its pieces,
   whose planes are made again from their corners.  (71140.c moves what
   the chopper carries with it too.) */
REGS(v0, a2, a3, t0)
void func_802BD99C(Building *b, s32 dx, s32 dy, s32 dz) {
    u8 *s, *e;
    s32 sx = dx >> 5, sy = dy >> 5, sz = dz >> 5;
    s32 wx = dx << 11, wy = dy << 11, wz = dz << 11;
    s32 qx = dx >> 2, qy = dy >> 2, qz = dz >> 2;
    s32 n, k;
    Piece *p;

    for (s = B_SECTION(b, MS_HEIGHTS), e = B_SECTION(b, MS_SUPPORTS); s != e; s += 2) {
        *(s16 *)s += sy;
    }
    s = B_SECTION(b, MS_SHADOW);
    ((s16 *)s)[0] += sx;
    ((s16 *)s)[1] += sz;
    ((s16 *)s)[2] += sx;
    ((s16 *)s)[3] += sz;
    for (s = B_SECTION(b, MS_TRIS), e = B_SECTION(b, MS_ANIMS); s != e; s += 0x14) {
        for (k = 0; k < 9; k += 3) {
            ((s16 *)s)[k] += sx;
            ((s16 *)s)[k + 1] += sy;
            ((s16 *)s)[k + 2] += sz;
        }
    }
    s = B_SECTION(b, MS_CORNERS);
    for (n = 4; n != 0; n--, s += 6) {
        ((s16 *)s)[0] += sx;
        ((s16 *)s)[1] += sy;
        ((s16 *)s)[2] += sz;
    }
    for (s = B_SECTION(b, MS_FX_HIT), e = B_SECTION(b, MS_FX_LAND); s != e; s += FX_SIZE) {
        FX_W(s, FX_X) += wx, FX_W(s, FX_Y) += wy, FX_W(s, FX_Z) += wz;
        FX_W(s, FX_UNK28) += wy;
    }
    for (s = B_SECTION(b, MS_FX_LAND), e = B_SECTION(b, MS_GROUP_DLS); s != e; s += FX_SIZE) {
        FX_W(s, FX_X) += wx, FX_W(s, FX_Y) += wy, FX_W(s, FX_Z) += wz;
        FX_W(s, FX_UNK28) += wy;
    }
    for (s = B_SECTION(b, MS_CENTRES), e = B_SECTION(b, MS_FX_HIT); s != e; s += 8) {
        ((s16 *)s)[0] += sx;
        ((s16 *)s)[1] += sy;
        ((s16 *)s)[2] += sz;
    }
    for (p = b->unk4; p != (Piece *)b->unk8; p++) {
        s32 x1, y1, z1, x2, y2, z2, x3, y3, z3;
        s32 ey, ez3, ez2, ey3, ex3, ex2;
        s64 nx, ny, nz, ax, ay, az;
        f32 fx, fy, fz, sq;

        x1 = p->v[0][0] + qx, y1 = p->v[0][1] + qy, z1 = p->v[0][2] + qz;
        x2 = p->v[1][0] + qx, y2 = p->v[1][1] + qy, z2 = p->v[1][2] + qz;
        x3 = p->v[2][0] + qx, y3 = p->v[2][1] + qy, z3 = p->v[2][2] + qz;
        p->v[0][0] = x1, p->v[0][1] = y1, p->v[0][2] = z1;
        p->v[1][0] = x2, p->v[1][1] = y2, p->v[1][2] = z2;
        p->v[2][0] = x3, p->v[2][1] = y3, p->v[2][2] = z3;
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
        p->nlen2 = sq;
        p->nlen = __builtin_sqrtf(sq);
        p->d = -(nx * x2 + ny * y2 + nz * z2);
        ax = nx, ay = ny, az = nz;
        if (ax < 0) {
            ax = -ax;
        }
        if (ay < 0) {
            ay = -ay;
        }
        if (az < 0) {
            az = -az;
        }
        piece_axis(p, ax, ay, az);
    }
}

/* Building b's animated textures stepped (anim to end, AnimTex).  Kind 0
   runs through its frames, a step every `lo` frames, blending from one to
   the next; 1 picks one at random now and then; others show the frame for
   the camera's heading between lo and hi.  The level's goal building only
   while its goal is open. */
REGS(v0, t1, t2)
void func_802BDDB4(Building *b, AnimTex *a, AnimTex *end) {
    s32 lo, hi, n, t, h, span;

    if (B_ID(b) == MODEL_GOAL) {
        if (!goal_open())
            return;
    }
    for (; a != end; a = ANIM_NEXT(a)) {
        if (a->kind == 0) {
            /* the next step every `lo` frames, the blend as far into it */
            t = a->hi + 1;
            n = a->lo;
            if (t == n) {
                lo = a->cur + 1;
                if (lo == a->n) {
                    lo = 0;
                }
                a->cur = lo;
                t = 0;
            }
            a->hi = t;
            a->blend = engine_divu((u32)t * 255, n);
            if (n == 0) {
                engine_break(0x802BE1DC, 7);
            }
        } else if (a->kind == 1) {
            /* a random frame, every `lo` sixteenths of D_803649D8 */
            t = (u32)D_803649D8 >> 4;
            if (a->lo == 0) {
                engine_break(0x802BE140, 7);
            }
            if (engine_remu(t, a->lo) == 0) {
                if (a->n == 0) {
                    engine_break(0x802BE160, 7);
                }
                a->cur = engine_remu((u32)t >> 8, a->n);
            }
        } else {
            /* by the camera's heading from the building: the first frame
               before lo, the last after hi, between them in steps and
               blends (the range may wrap past 0) */
            h = heading_to(b->x, b->z, (u32)D_803643F8 >> 11, (u32)D_80364400 >> 11);
            lo = a->lo;
            hi = a->hi;
            if (!(hi < lo)) {
                if (h < lo)
                    goto first;
                if (hi < h)
                    goto final;
                t = engine_divu((u32)(h - lo) * (a->n - 1) * 255, hi - lo);
                if (hi - lo == 0) {
                    engine_break(0x802BE028, 7);
                }
                a->cur = (u32)t / 255;
                a->blend = (u32)t % 255;
            } else {
                if (h < lo) {
                    if (hi < h)
                        goto first;
                }
                span = 0xFFF - lo;
                if (h < lo) {
                    t = span + h;
                } else {
                    t = h - lo;
                }
                t = engine_divu((u32)t * (a->n - 1) * 255, span + hi);
                if (span + hi == 0) {
                    engine_break(0x802BE0CC, 7);
                }
                a->cur = (u32)t / 255;
                a->blend = (u32)t % 255;
            }
            continue;
        first:
            a->cur = 0;
            continue;
        final:
            a->cur = a->n - 1;
        }
    }
}

/* A building's display list from src to end copied to dl, its animated
   textures' frames put in: a G_SETTIMG of one of them (the records anim
   to anim_end) gets its current frame; one that blends (AnimTex.blends)
   also the next G_SETTIMG the frame after, and the G_SETPRIMCOLOR after
   them the blend between the two.  func_802BE228 and func_802BE3C8 are
   this for the two lists. */
static u32 *copy_dl(u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out) {
    u32 w0, w1, f;
    AnimTex *a;

    while (src != end) {
        w0 = src[0];
        dl[0] = w0;
        w1 = src[1];
        if (w0 >> 24 == DL_SETTIMG) {
            /* a texture: one of the animated ones? */
            for (a = anim; a != anim_end; a = ANIM_NEXT(a)) {
                if (w1 == a->texture)
                    break;
            }
            if (a != anim_end) {
                if (!a->blends) {
                    /* its current frame, if not the first */
                    f = a->cur;
                    if (f != 0) {
                        w1 = ANIM_FRAME(a, f);
                    }
                } else {
                    f = a->cur;
                    if (f != 0) {
                        w1 = ANIM_FRAME(a, f);
                    }
                    dl[1] = w1;
                    src += 2;
                    dl += 2;
                    if (++f == a->n) {
                        f = 0;
                    }
                    /* the next texture: the frame after */
                    for (;;) {
                        w0 = src[0];
                        dl[0] = w0;
                        w1 = src[1];
                        if (w0 >> 24 == DL_SETTIMG)
                            break;
                        dl[1] = w1;
                        src += 2;
                        dl += 2;
                    }
                    if (f != 0) {
                        w1 = ANIM_FRAME(a, f);
                    }
                    dl[1] = w1;
                    src += 2;
                    dl += 2;
                    /* the blend, at the primitive colour */
                    for (;;) {
                        w0 = src[0];
                        if (w0 >> 24 == DL_SETPRIMCOLOR)
                            break;
                        dl[0] = w0;
                        dl[1] = src[1];
                        src += 2;
                        dl += 2;
                    }
                    dl[0] = w0 | a->blend;
                    dl[1] = src[1];
                    src += 2;
                    dl += 2;
                    continue;
                }
            }
        }
        dl[1] = w1;
        src += 2;
        dl += 2;
    }
    *src_out = src;
    return dl;
}

/* the first list (dl in $a2) */
REGS(a2, s0, t1, t2, t7 -> a2, t7)
u32 *func_802BE228(u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out) {
    return copy_dl(dl, end, anim, anim_end, src, src_out);
}

/* the second (dl in $t0) */
REGS(t0, s0, t1, t2, t7 -> t0, t7)
u32 *func_802BE3C8(u32 *dl, u32 *end, AnimTex *anim, AnimTex *anim_end, u32 *src, u32 **src_out) {
    return copy_dl(dl, end, anim, anim_end, src, src_out);
}

/* The matrix of building b's falling group g (0-based): turned about its
   centre by its spin rates times its fall time (12-bit angles), and
   lowered by `fall`; made at D_803F3964 (the next one), returned as its
   physical address. */
REGS(v0, a3, s6 -> s5)
u32 func_802BE574(Building *b, s32 g, s32 fall) {
    s32 *m = (s32 *)FALL_MTX;
    u32 t = B_FALL_T(b)[g];
    s32 ax, ay, az, cx, cy, cz;
    u16 *c;

    FALL_MTX = (u8 *)m + 0x40;
    ax = (s32)(t * (u32)B_SPIN_X(b)[g]);
    if (ax < 0) {
        ax += 0xFFF;
    }
    ay = (s32)(t * B_SPIN_Y(b)[g]);
    if (ay < 0) {
        ay += 0xFFF;
    }
    az = (s32)(t * B_SPIN_Z(b)[g]);
    if (az < 0) {
        az += 0xFFF;
    }
    c = (u16 *)(B_SECTION(b, MS_CENTRES) + g * 8);
    cx = c[0] << 16;
    cy = c[1] << 16;
    cz = c[2] << 16;
    func_802ACA60(-cx, -cy, -cz, m);
    func_802ACBDC(ax, D_803F38D0);
    func_802ACCCC(D_803F38D0, m);
    func_802ACAC4(ay, D_803F38D0);
    func_802ACCCC(D_803F38D0, m);
    func_802ACB50(az, D_803F38D0);
    func_802ACCCC(D_803F38D0, m);
    func_802ACA60(cx, cy, cz, D_803F38D0);
    func_802ACCCC(D_803F38D0, m);
    func_802ACA60(0, fall, 0, D_803F38D0);
    func_802ACCCC(D_803F38D0, m);
    func_802AC8CC((u32 *)m);
    return K0(m);
}

/* ---- a vehicle against the buildings --------------------------------- */

/* whether the camera's limits (D_803A7410..12) are narrowed */
#define CAMERA_NARROWED() (D_803A7410 != 0 || D_803A7412 != 0xFFF)

/* Each frame, from every vehicle module: the vehicle of type `type` (its
   VehicleState vs) against the buildings, by its sphere (its Solid,
   D_803A7300), each piece hit doing damage (func_802BEBB0); what happened
   into vs->unk9C..9E. */
REGS(t8, gp)
void func_802BE77C(s32 type, VS *vs) {
    Solid *s = D_803A7300;
    Building *b;
    s32 hit, t;

    D_803F7802 = 0;
    D_803F7803 = 0;
    D_803F7807 = 0;
    D_803F77F4 = D_803F77F8;
    D_803F780F = 0;
    func_802BFEE4(vs);
    func_802BE9F8();
    while (s->kind != type)
        s++;
    if (s->end != 0) {
        for (b = D_803F4030; b != D_803F7654; b++) {
            hit = func_8029CFA4(s->x, s->y, s->z, s->r, b->x, b->y, b->z, B_RADIUS(b));
            if (hit) {
                if (B_ID(b) == MODEL_GOAL) {
                    t = func_802BD8C8();
                    if (t)
                        continue;
                }
                t = func_802BE944(b, type);
                if (!t) {
                    func_802BEADC(b, s->x, s->y, s->z, s->r, type);
                }
            }
        }
    }
    if (type != 0xFF) {
        func_802BCC48();
    }
    if (CAMERA_NARROWED()) {
        D_803A7425 = 1;
    }
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

    if (model != MODEL_GOAL) {
        for (p = D_803059F0; p->types != 0; p++) {
            if (p->level != 0xFFFF) {
                if (p->level != (u32)D_802E8BDC)
                    continue;
            }
            if (p->model != (u32)model) {
                continue;
            }
            if ((p->types & (1u << (type & 31))) == 0)
                continue;
            r = 1;
            break;
        }
    }
    return r;
}

/* the list of (building, group) pairs hit this frame emptied */
REGS()
void func_802BE9F8(void) {
    HIT_END = HIT_PAIRS;
}

/* (b, group) added to it */
REGS(t3, t9)
void func_802BEA30(s32 group, Building *b) {
    HitPair *p = HIT_END;

    p->b = b;
    p->group = group;
    HIT_END = p + 1;
}

/* whether (b, group) is in it */
REGS(t3, t9 -> t7)
s32 func_802BEA70(s32 group, Building *b) {
    HitPair *p;

    for (p = HIT_PAIRS; p != HIT_END; p++) {
        if (p->b != b)
            continue;
        if (p->group != group)
            continue;
        return 1;
    }
    return 0;
}

/* the pieces of building b the vehicle's sphere hits, each hit with
   func_802BEBB0 */
REGS(t3, v0, v1, a0, a1, t8)
void func_802BEADC(Building *b, s32 x, s32 y, s32 z, s32 r, s32 type) {
    Piece *p;

    x >>= 2;
    y >>= 2;
    z >>= 2;
    r >>= 2;
    for (p = b->unk4; p != (Piece *)b->unk8; p++) {
        if (p->active == 0)
            continue;
        if (piece_touched(p, x, y, z, r)) {
            func_802BEBB0(p, type, b);
        }
    }
}

/* whether the hit marks building b as the one the player last hit
   (D_8036B974; 23C20.c's hint): not an explosion's, not on a building of
   strength 1, a target or the goal, and not the same again */
static s32 hit_marks(Building *b, s32 type) {
    if (type == 0xFF)
        return 0;
    if (M_STRENGTH(B_MODEL(b)) == 1)
        return 0;
    if (B_TARGET(b))
        return 0;
    if (B_ID(b) == MODEL_GOAL)
        return 0;
    return D_8036B974 != b;
}

/* whether a hit by part n of vehicle type `type` does damage: while
   dashing (D_803F7800), from an explosion, after a push long enough
   (func_802BEFF4), or if the part didn't touch last frame */
static s32 hit_damages(s32 type, s32 n) {
    s32 again;

    if (D_803F7800 != 0)
        return 1;
    if (type == 0xFF)
        return 1;
#ifndef VERSION_US_V10
    if (D_803F7812 == 0)
#endif
    {
        func_802BEFF4();
        if (D_803F780C != 0)
            return 1;
    }
    again = func_802BCDE0(n);
    return !again;
}

/* Piece p of building b hit by vehicle type `type`: if a part of the
   type's (D_803A6B30) touches it, the group takes its damage (by the
   part's power and the model's strength, func_802BFF6C), once a frame,
   and what follows: the screen shaken, the camera turned, the goal's
   flag, the group destroyed at 100. */
REGS(s0, t8, t9)
void func_802BEBB0(Piece *p, s32 type, Building *b) {
    KindPart *k;
    s32 n = 0, g, d, dmg, seen;
    s8 *c;
    Piece *q;

    /* the type's parts (numbered n from 1): the first that touches p */
    for (k = D_803A6B30;; k++) {
        if (k->end == -1) {
            return;
        }
        if (k->end == 0)
            continue;
        if (k->kind != type)
            continue;
        n++;
        if (piece_touched(p, k->x >> 2, k->y >> 2, k->z >> 2, k->r >> 2))
            break;
    }
    if (type != 0xFF) {
        D_803F77F8 = D_80358068;
    }
    if (hit_marks(b, type)) {
        D_8036B971 = 1;
        D_8036B974 = b;
    }
    func_802BEF9C(b);
    if (D_803BE738 != 0) {
        return;
    }
    g = p->group;
    func_802BCCD4(n);
    seen = func_802BEA70(g, b);
    if (!seen && hit_damages(type, n)) {
        /* the damage, once a frame for each group */
        func_802BEA30(g, b);
        if (B_ID(b) == MODEL_GOAL) {
            if (type == 0) {
                D_803F7805 = 1;
            }
            d = 0;
        } else {
            d = func_802BFF6C(g, k->power, type, b, M_STRENGTH(B_MODEL(b)), n);
        }
        D_802E8BE4 = SHAKE_HIT_FRAMES;
        if (d <= SHAKE_HIT_MAX) {
            D_802E8BE8 = d;
        } else {
            D_802E8BE8 = SHAKE_HIT_MAX;
        }
        dmg = B_DAMAGE(b)[g - 1] + d;
        if (dmg >= 100) {
            dmg = 100;
        }
        B_DAMAGE(b)[g - 1] = dmg;
        D_803F77FE = d;
        func_802BF898(g, dmg, b);
        if (dmg == 100) {
            /* the group destroyed: its triangles, shadow, smoke, dust,
               the building's first effect; its pieces off, the ones it
               brings back on */
            func_802BF1F0(g, b);
            func_802C1438(g, b);
            func_802C09B8(g, b);
            func_802C0E8C(g, b);
            func_802BF384(b);
            D_802E8BE4 = SHAKE_DOWN_FRAMES;
            D_802E8BE8 = SHAKE_DOWN;
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
        }
    }
    /* the camera turned by the piece (unless the part is listed,
       D_803A7408), or, the group gone, D_803F7802 */
    D_803F7803 = 1;
    for (c = D_803A7408;; c++) {
        if (*c == n)
            goto done;
        if (*c < 0)
            break;
    }
    if (B_DAMAGE(b)[g - 1] != 100) {
        D_803A7424 = 1;
        func_802BF264(p);
    } else {
        D_803F7802 = 1;
    }
done:
    func_802BF668(b);
    func_802BF534(b);
}

/* level 0x2D's buildings 0xE7-0xE9 end it when hit */
REGS(t9)
void func_802BEF9C(Building *b) {
    if (D_802E8BDC == 0x2D) {
        if (!(B_ID(b) < 0xE7)) {
            if (B_ID(b) < 0xEA) {
                D_803BE738 = 1;
            }
        }
    }
}

/* The pushing of the vehicles that can (not types 0-2, 6, 7, 9, 0xB,
   0x10-0x12): pushed in one direction (the sign of D_803F77FC) for
   PUSH_FRAMES frames in a row, D_803F780C is set for a frame, unless
   D_80370C2C was set on every one of them. */
REGS()
void func_802BEFF4(void) {
    static const u8 types[] = { 1, 2, 6, 7, 9, 0xB, 0x10, 0x11, 0x12 };
    s32 type = D_80364456, dir, steer, k, cnt;

    if (type == 0)
        return;
    for (k = 0; k < 9; k++) {
        if (type == types[k])
            return;
    }
    dir = 0;
    if (D_803F77FC < 0) {
        dir = -1;
    } else {
        if (D_803F77FC != 0) {
            dir = 1;
        }
    }
    if (D_803F780F != 0)                /* once a frame */
        return;
    D_803F780F = 1;
    if (D_803F77F4 + 1 != D_80358068)   /* not hit last frame: start again */
        goto reset;
    if (dir != 0) {
        if (D_803F780A == dir)
            goto reset;
    }
    cnt = D_803F780B;
    steer = (u8)D_80370C2C != 0;
    if (cnt == 0) {
        D_803F780E = steer;
        D_803F780D = 0;
    } else {
        if (!steer || steer != D_803F780E) {
            D_803F780D = 1;
        }
    }
    cnt++;
    D_803F780B = cnt;
    D_803F780A = dir;
    D_803F780C = 0;
    if (cnt != PUSH_FRAMES)
        return;
    D_803F780B = 0;
    if (D_803F780D == 0)
        return;
    D_803F780C = 1;
    return;
reset:
    D_803F780B = 0;
    D_803F780A = dir;
    D_803F780C = 0;
}

/* the triangles (MS_TRIS) of group `group` (1-based) marked gone */
REGS(t3, t9)
void func_802BF1F0(s32 group, Building *b) {
    u8 *t, *end = B_SECTION(b, MS_ANIMS);

    for (t = B_SECTION(b, MS_TRIS); t != end; t += 0x14) {
        if (t[0x12] == group) {
            t[0x13] = 1;
        }
    }
}

/* a piece that pushes the camera (Piece.pushes): the camera kept within a
   quarter turn of its side facing the player, unless the piece only works
   from one side (unk55, unk56) */
REGS(s0)
void func_802BF264(Piece *p) {
    s64 d;
    s32 h = p->heading, a, c;

    if (p->pushes == 0)
        return;
    d = (s64)((u64)p->nx * (u64)(s64)(D_803A73F0 >> 2) + (u64)p->ny * (u64)(s64)(D_803A73F4 >> 2) +
              (u64)p->nz * (u64)(s64)(D_803A73F8 >> 2) + (u64)p->d);
    if (p->d > 0) {
        if (d > 0)
            goto behind;
    } else {
        if (d < 0)
            goto behind;
    }
    a = h + 0x400;
    c = h - 0x400;
    if (p->unk55 != 1) {
        if (p->unk56 != 0) {
            return;
        }
    }
    goto push;
behind:
    a = h - 0x400;
    c = h + 0x400;
    if (p->unk55 != 1) {
        if (p->unk56 == 0)
            return;
    }
push:
    func_8029B7CC(a, c);
}

/* ---- destruction ------------------------------------------------------- */

/* the first of building b's groups destroyed: its effect (func_80264CB4) */
REGS(t9)
void func_802BF384(Building *b) {
    s32 n, gone = 0;
    u8 *d = B_DAMAGE(b);

    for (n = B_NGROUPS(b); n != 0; n--) {
        if (*d++ == 100) {
            gone++;
        }
    }
    if (gone == 1) {
        func_80264CB4((u32)b->x >> 5, (u32)b->y >> 5, (u32)b->z >> 5, (u32)B_RADIUS(b) >> 5, B_CELL(b),
                      M_UNK6(B_MODEL(b)));
    }
}

/* Building b destroyed, when every group but the main one is gone or
   falling: counted (D_80364A40, D_803649F0: the model's value) and the
   lights on it (D_803BDFD8, by its index + 1) off. */
REGS(t9)
void func_802BF534(Building *b) {
    s32 main, k, n, idx;
    u8 *d;
    LevelLight *l;
    u16 *f;
    u32 v;

    if (B_DESTROYED(b))
        return;
    main = M_MAIN(B_MODEL(b));
    d = B_DAMAGE(b);
    f = B_FALLING(b);
    for (k = 1, n = B_NGROUPS(b); n != 0; k++, n--, d++, f++) {
        if (main != k) {
            if (*d != 100) {
                if (*f == 0)
                    return;
            }
        }
    }
    B_DESTROYED(b) = 1;
    v = M_VALUE(B_MODEL(b));
    D_80364A40 = v;
    D_803649F0 += v;
    idx = (u32)((u8 *)b - (u8 *)D_803F4030) / sizeof(Building);
    idx++;
    for (l = D_803BDFD8; l != D_803BDFD4; l++) {
        if (l->building == idx) {
            l->on = 0;
        }
    }
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

    do {
        again = 0;
        end = B_SECTION(b, MS_END);
        for (r = B_SECTION(b, MS_SUPPORTS); r != end;) {
            g = r[0] - 1;
            if (B_FALLING(b)[g] != 0 || (B_DAMAGE(b)[g] == 100)) {
                r = SUP_NEXT(r);
                continue;
            }
            sum = 0;
            for (n = r[2], s = r + 3; n != 0; n--, s += 2) {
                q = s[0] - 1;
                if (B_DAMAGE(b)[q] != 100) {
                    if (B_FALLING(b)[q] == 0) {
                        sum += s[1];
                    }
                }
            }
            if (sum < r[1]) {
                again = 1;
                func_802BF1F0(r[0], b);
                g = r[0] - 1;
                B_FALLING(b)[g] = 1;
                B_SPIN_X(b)[g] = D_803649E0;
                B_SPIN_Y(b)[g] = D_803649E2;
                B_SPIN_Z(b)[g] = D_803649E4;
                g++;
                for (p = b->unk4; p != (Piece *)b->unk8; p++) {
                    if (p->group == g) {
                        p->active = 0;
                    }
                    if (p->group2 == g) {
                        if (B_DAMAGE(b)[p->group - 1] != 100) {
                            p->active = 1;
                        }
                    }
                }
            }
            r = s;
        }
    } while (again);
}

/* ---- a hit's effects ------------------------------------------------- */

/* a model's effect record for the group hit (func_802BF898's): one marked
   0xFF fires on every hit of FX_STRONG_HIT or more, another once the
   group's damage reaches its own (and is done) */
static void fx_fire(u8 *fx, s32 damage, Building *b) {
    s32 t;

    t = FX_B(fx, FX_LIMIT);
    if (t == 0xFF) {
        if (D_803F77FE >= FX_STRONG_HIT) {
            func_802C04F0(fx);
        }
        return;
    }
    if (damage >= t) {
        func_802BFBF4(fx, b);
        func_802C04F0(fx);
        FX_B(fx, FX_DONE) = 1;
    }
}

/* The effects of a hit on group `group` (now at `damage`): with a model
   that has debris of its own (M_DEBRIS) func_802BF978's; else its effect
   records (MS_FX_HIT) for the group fire once its damage reaches theirs,
   those marked 0xFF on every strong enough hit (FX_STRONG_HIT). */
REGS(t3, t5, t9)
void func_802BF898(s32 group, s32 damage, Building *b) {
    u8 *model = B_MODEL(b), *fx, *end;

    if (M_DEBRIS(model) != 0) {
        func_802BF978(model, group, damage, b);
        return;
    }
    end = B_SECTION(b, MS_FX_LAND);
    for (fx = B_SECTION(b, MS_FX_HIT); fx != end; fx += FX_SIZE) {
        if (FX_B(fx, FX_DONE) == 0 && (FX_B(fx, FX_GROUP) == group))
            fx_fire(fx, damage, b);
    }
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

    roll = func_802BFB50(0, 100);
    for (t = DEBRIS_COUNTS; t[0] < roll; t += 2) {
    }
    n = t[1];
    for (k = 0; k != n; k++) {
        if (damage >= 100) {
            t = CHANCES_DESTROYED;
        } else {
            t = CHANCES_HIT;
            if (D_803F77FE < FX_STRONG_HIT)
                return;
        }
        roll = func_802BFB50(0, 100);
        for (; !(roll < CHANCE(t)); t += 0xC) {
        }
        fx = D_803F3FF8;
        FX_W(fx, FX_X) = x;
        FX_W(fx, FX_Y) = y;
        FX_W(fx, FX_Z) = z;
        roll = func_802BFB50(CHANCE_LO(t), CHANCE_HI(t));
        sp = (s32)((u32)M_DEBRIS(model) * (u32)roll);
        if (D_803EF6FF != 0) {
            sp <<= 1;
        }
        FX_W(fx, FX_SPEED) = sp;
        FX_W(fx, FX_VEL) = 0;
        FX_W(fx, FX_VEL + 4) = 0;
        FX_W(fx, FX_VEL + 8) = 0;
        FX_W(fx, FX_UNK14) = 0;
        FX_W(fx, FX_UNK14 + 4) = 0;
        FX_W(fx, FX_UNK14 + 8) = 0;
        FX_W(fx, FX_UNK28) = FX_UNK28_INIT;
        if (k != 0) {
            FX_H(fx, FX_DELAY) = FX_DEBRIS_DELAY;
        } else {
            FX_H(fx, FX_DELAY) = 0;
        }
        FX_B(fx, FX_RADIUS) = 0;
        FX_B(fx, FX_AMOUNT) = 0;
        FX_B(fx, FX_DONE) = 0;
        FX_B(fx, FX_UNK35) = 0;
        FX_B(fx, FX_KIND) = CHANCE_KIND(t);
        func_802C04F0(fx);
    }
    func_802BFBF4(fx, b);
}

/* a random number from lo up to hi (23C20.c's) */
REGS(a0, a1 -> s4)
s32 func_802BFB50(s32 lo, s32 hi) {
    s32 r;

    r = func_8026A828(lo, hi);
    return r;
}

/* effect fx's sound, for the kinds D_8030633C lists (not on models of
   strength 1): one of four, at its place */
REGS(v1, t9)
void func_802BFBF4(u8 *fx, Building *b) {
    s8 *k;
    s32 kind;

    if (M_STRENGTH(B_MODEL(b)) == 1)
        return;
    kind = FX_B(fx, FX_KIND);
    for (k = (s8 *)D_8030633C;;) {
        if (*k == -1)
            return;
        if (*k++ == kind)
            break;
    }
    func_80288284(((u32)D_803649D8 >> 8) % 4, (u32)FX_W(fx, FX_X) >> 11, (u32)FX_W(fx, FX_Y) >> 11,
                  (u32)FX_W(fx, FX_Z) >> 11, b->y);
}

/* a group of building b (g, from 0) landed: its effect records for it
   (MS_FX_LAND) fire, or with a model that has debris of its own,
   func_802BFDAC's */
REGS(v0, a3)
void func_802BFD1C(Building *b, s32 g) {
    u8 *fx, *end;

    g++;
    if (M_DEBRIS(B_MODEL(b)) != 0) {
        func_802BFDAC(b, g);
        return;
    }
    end = B_SECTION(b, MS_GROUP_DLS);
    for (fx = B_SECTION(b, MS_FX_LAND); fx != end; fx += FX_SIZE) {
        if (FX_B(fx, FX_GROUP) == g) {
            func_802C04F0(fx);
        }
    }
}

/* a piece of debris where group `group` (1-based) landed, of a kind from
   D_803063D4's chances */
REGS(v0, a3)
void func_802BFDAC(Building *b, s32 group) {
    u8 *model = B_MODEL(b), *t, *fx;
    s16 *c = (s16 *)(B_SECTION(b, MS_CENTRES) + (group - 1) * 8);
    s32 x = c[0], y = b->y, z = c[2], roll;

    roll = func_802BFB50(0, 100);
    for (t = CHANCES_LANDED; !(roll < CHANCE(t)); t += 0xC) {
    }
    fx = D_803F3FF8;
    FX_W(fx, FX_X) = x << 16;
    FX_W(fx, FX_Y) = y << 11;
    FX_W(fx, FX_Z) = z << 16;
    roll = func_802BFB50(CHANCE_LO(t), CHANCE_HI(t));
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
}

/* D_803F7800: whether the vehicle (vs) is dashing or so (D_803F7801 2, or
   one of its three flags at 0x96 is 1; never with D_803F7801 1) */
REGS(gp)
void func_802BFEE4(VS *vs) {
    s32 m = D_803F7801, dash;

    if (m == 1) {
        dash = 0;
    } else {
        dash = m == 2 || (vs->unk96[0] == 1) || (vs->unk96[1] == 1) ||
               (vs->unk96[2] == 1);
    }
    if (dash) {
        D_803F7800 = 1;
    } else {
        D_803F7800 = 0;
    }
}

/* the next of D_80305E50's damage rules after r, when r isn't taken: the
   ones by D_803F7804, the hole and the push are 4 bytes, a vehicle part's
   4 + its byte 3 */
static u8 *rule_skip(u8 *r) {
    s32 t;

    t = r[2];
    if (t == 0xFC || t == 0xFB) {
        return r + 4;
    }
    if (t != 0xFD) {
        if (t != 0xFF) {
            if (t != 0xFE) {
                return r + r[3] + 4;
            }
        }
    }
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

    if (type == 0xFF) {
        if (strength == 1) {
            d = 100;
            goto out;
        }
        d = 0;
        goto out;
    }
    D_8036CB2F = 1;
    D_8036CB2E = type;
    if (M_MAIN(B_MODEL(b)) == group) {
        D_803F7807 = 1;
        D_8036CB2F = 0;
        d = 0;
        goto out;
    }
    t = func_802C0284(d, type, strength, &k);
    d = k;
    if (t != 0)
        goto out;
    if (D_803F7800 != 0) {
        D_8036CB2A = 100;
        D_8036CB2C = 100;
        d = 100;
        goto out;
    }
#define SKIP_RULE                                                           \
    {                                                                       \
        r = rule_skip(r);                                                   \
        continue;                                                           \
    }
    for (r = D_80305E50;;) {
        if (r[0] != type)
            SKIP_RULE
        if (r[1] != n)
            SKIP_RULE
        t = r[2];
        if (t == 0xFC) {
            if (D_803F7804 == 0)
                SKIP_RULE
            d = r[3] * d;
            goto divide;
        }
        if (t == 0xFB) {
            s = func_802C038C(sel, b);
            if (!s)
                SKIP_RULE
            d = r[3] * d;
            goto divide;
        }
        if (t == 0xFF) {
            if (D_803F77FC < 0)
                SKIP_RULE
            break;
        }
        if (t == 0xFE) {
            if (D_803F77FC > 0)
                SKIP_RULE
            break;
        }
        if (t == 0xFD)
            break;
        /* the state of a part of the vehicle's (D_803F77D0) */
        sel = t;
        s = func_802A04BC(t, D_803F77D0, &f11, &f12, &f14, &fC, &fE, &f13, &f4);
        if (s != 0) {
            r += f13;
            s = engine_cvt_w_s(f4 * (f32)(r[5] - r[4]));
            d = (u32)(s + r[4]) * d;
            goto divide;
        }
        r += r[3] + 4;
    }
#undef SKIP_RULE
    /* by the push */
    k = D_803F77FC;
    if (k < 0) {
        k = -k;
    }
    d = ((u32)k * d * r[3]) >> 4;
divide:
    D_8036CB2A = d;
    d = engine_divu(d, strength);
    if (strength == 0) {
        engine_break(0x802C0244, 7);
    }
    D_8036CB2C = d;
out:
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

    if (D_803F780C != 0) {
        D_803F780C = 0;
        *power_out = 100;
        return 1;
    }
    if (D_803A740C + 1 == D_80358068) {
        for (p = D_80305E10; p[1] != 0; p += 2) {
            if ((p[1] & (1u << (type & 31))) == 0)
                continue;
            if (p[0] != (u32)D_802E8BDC)
                continue;
            r = 1;
            power = 100;
            break;
        }
    }
    if (type == 1) {
        if (strength == 1) {
            r = 1;
            power = 100;
        }
    }
    if (type == 0xA) {
        if (strength == 1) {
            r = 1;
            power = 100;
        }
    }
    if (type == 0x10) {
        if (strength == 1) {
            r = 1;
            power = 100;
        }
    }
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

    for (r = B_SECTION(b, MS_SUPPORTS); r != end; r = SUP_NEXT(r)) {
        for (k = r[2], s = r + 3; k != 0; k--, s += 2) {
            if (s[0] == group) {
                if (B_DAMAGE(b)[r[0] - 1] == 100)
                    break;
                return 0;
            }
        }
    }
    if (D_803A73F4 + 0x578 < ((s16 *)B_SECTION(b, MS_HEIGHTS))[group - 1] << 5) {
        return 0;
    }
    return 1;
}

/* ---- the effects ----------------------------------------------------- */

/* all the effects free */
REGS()
void func_802C049C(void) {
    s32 i;

    for (i = 0; i < NFX; i++) {
        FX_B(FX_RECORDS[i], FX_DONE) = 1;
    }
}

/* effect record `src` copied into the first free one (none if none) */
REGS(v1)
void func_802C04F0(u8 *src) {
    s32 i;

    for (i = 0; i < NFX; i++) {
        if (FX_B(FX_RECORDS[i], FX_DONE) != 0) {
            /* (fourteen words: the copy's loop) */
            __builtin_memcpy(FX_RECORDS[i], src, FX_SIZE);
            break;
        }
    }
}

/* Each frame (hd.c): each waiting effect counts down (FX_DELAY) and then
   starts (60F60's func_802A6274: of a kind D_80306270 picks for its type
   at random, the "big" ones with a3 1), with its sound unless the game
   is ending; one with a radius does damage around (func_802C18D4). */
void func_802C0574(void) {
    u8 *fx, *kinds;
    s32 i, t, k, big, started;

    for (i = 0; i < NFX; i++) {
        fx = FX_RECORDS[i];
        if (FX_B(fx, FX_DONE) != 0)
            continue;
        t = FX_H(fx, FX_DELAY);
        if (t != 0) {
            FX_H(fx, FX_DELAY) = t - 1;
            continue;
        }
        kinds = D_80306270[FX_B(fx, FX_KIND)];
        k = kinds[1 + engine_remu((u32)D_803649D8 >> 4, kinds[0])];
        if (kinds[0] == 0) {
            engine_break(0x802C0638, 7);
        }
        big = 1;
        if (FX_B(fx, FX_KIND) != FX_KIND_SPARK) {
            if (D_803F7810 == 0) {
                if (D_8036DCD4 == 0)
                    big = 0;
                else {
                    if (D_8036DCD7 != 1)
                        big = 0;
                }
            }
        }
        started = func_802A6274(D_802C3FFC[k], FX_W(fx, FX_SPEED), 0, FX_W(fx, FX_X), FX_W(fx, FX_Y),
                                FX_W(fx, FX_Z), FX_W(fx, FX_VEL), FX_W(fx, FX_VEL + 4), FX_W(fx, FX_VEL + 8),
                                FX_W(fx, FX_UNK14), FX_W(fx, FX_UNK14 + 4), FX_W(fx, FX_UNK14 + 8),
                                FX_W(fx, FX_UNK28), FX_B(fx, FX_UNK35), big);
        if (started != 0) {
            if (D_80364A98 == 0) {
                if (D_803643D6 == 0) {
                    func_802619D0(FX_B(fx, FX_KIND));
                }
            }
        }
        FX_B(fx, FX_DONE) = 1;
        t = FX_H(fx, FX_RADIUS);
        if (t != 0) {
            func_802C18D4(t, FX_W(fx, FX_X), FX_W(fx, FX_Y), FX_W(fx, FX_Z), FX_B(fx, FX_AMOUNT) << 4);
        }
    }
}

/* ---- smoke, dust and the falling groups' shadows --------------------- */

/* The smoke clouds (D_803F0900): each fading (its alpha down by
   SMOKE_FADE a frame) is drawn at that alpha; a gone one waits SMOKE_WAIT
   frames before it can be used again. */
REGS(v0)
void func_802C08C4(u32 *g) {
    Smoke *s;
    s32 i, a;

    for (i = 0, s = D_803F0900; i < NSMOKE; i++, s++) {
        a = s->alpha;
        if (a == 0) {
            if (s->wait != 0) {
                s->wait--;
            }
            continue;
        }
        a -= SMOKE_FADE;
        if (a <= 0) {
            s->alpha = 0;
            s->wait = SMOKE_WAIT;
            continue;
        }
        s->alpha = a;
        g[0] = DL_PIPESYNC;
        g[1] = 0;
        g[2] = DL_ENVCOLOR;             /* the environment colour: alpha */
        g[3] = a;
        g[4] = DL_CALL;                 /* the cloud's list */
        g[5] = K0(s->dl);
        g += 6;
    }
    g[0] = DL_END;
    g[1] = 0;
}

/* whether the look at r (a GroupDl of group `group`'s, 1-based) is shown
   the moment the group goes: no condition hides it, the group itself
   counted at GROUP_DYING_DAMAGE */
static s32 look_shown(u8 *r, s32 group, Building *b) {
    GroupCond *c;
    s32 n, dmg;

    for (n = GDL_N(r), c = GDL_COND(r); n != 0; n--, c++) {
        if (c->group == group) {
            dmg = GROUP_DYING_DAMAGE;
        } else {
            dmg = B_DAMAGE(b)[c->group - 1];
        }
        if (c->below) {
            if (dmg < c->limit) {
                return 0;
            }
        } else {
            if (c->limit < dmg)
                return 0;
        }
    }
    return 1;
}

/* A display list's commands from src to src_end copied to *d (through
   func_802C0C64 when `subst`) while *k, counting down, lasts: 0 if it ran
   out. */
static s32 copy_cmds_n(u32 **d, u32 *src, u32 *src_end, s32 *k, s32 subst) {
    for (; src != src_end; src += 2) {
        if (--*k == 0)
            return 0;
        if (subst) {
            *(u64 *)*d = func_802C0C64(*(u64 *)src);
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
    u8 *model = B_MODEL(b), *r, *end;
    Smoke *s;
    GroupDls *dls;
    u32 *d;
    s32 i, k;

    if (M_SHADOWS(model) != 0)
        return;
    for (i = NSMOKE, s = D_803F0900;; i--, s++) {
        if (i == 0)
            return;
        if (s->alpha == 0) {
            if (s->wait == 0)
                break;
        }
    }
    d = s->dl;
    d[0] = DL_SEGMENT(9);
    d[1] = K0(M_TEXTURES(model));
    d += 2;
    k = SMOKE_DL_MAX;
    end = B_SECTION(b, MS_FALLS);
    for (r = B_SECTION(b, MS_GROUP_DLS); r != end; r = GDL_NEXT(r)) {
        if (GDL_GROUP(r) != group || !look_shown(r, group, b)) {
            continue;
        }
        dls = GDL_DLS(r, GDL_N(r));
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl0), (u32 *)(model + dls->dl0_end), &k, 1))
            return;
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl1), (u32 *)(model + dls->dl1_end), &k, 1))
            return;
    }
    s->alpha = SMOKE_ALPHA;
    d[0] = DL_END;
    d[1] = 0;
}

/* a display list command as a smoke cloud has it: D_802F46C0's pairs
   (twelve) replace some */
REGS(t4 -> t4)
u64 func_802C0C64(u64 cmd) {
    u64 *p;
    s32 n;

    for (n = 12, p = D_802F46C0; n != 0; n--, p += 2) {
        if (p[0] == cmd) {
            cmd = p[1];
            break;
        }
    }
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

    D_803F7810 = 0;
    for (i = 0, s = D_803EFED0; i < NDUST; i++, s++) {
        if (s->active == 0) {
            if (s->wait != 0) {
                s->wait--;
            }
            continue;
        }
        h = s->speed + DUST_ACCEL;
        y = s->y + h;
        s->speed = h;
        if (y >= s->top) {
            s->active = 0;
            s->wait = DUST_WAIT;
            continue;
        }
        s->y = y;
        D_803F7810 = 1;
        x = func_802BFB50(-DUST_SHAKE, DUST_SHAKE);
        x <<= 12;
        z = func_802BFB50(-DUST_SHAKE, DUST_SHAKE);
        y = -(y << 11);
        z <<= 12;
        if (D_8035805C != 0) {
            m = s->mtx[1];
        } else {
            m = s->mtx[0];
        }
        func_802ACA60(x, y, z, m);
        func_802AC8CC((u32 *)m);
        g[0] = DL_PIPESYNC;
        g[1] = 0;
        g += 2;
        if (D_8035805C != 0) {
            m = s->mtx[1];
        } else {
            m = s->mtx[0];
        }
        g[0] = DL_SEGMENT(12);          /* the matrix */
        g[1] = K0(m);
        g[2] = DL_CALL;                 /* the cloud */
        g[3] = K0(s->dl);
        g += 4;
    }
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
    u8 *model = B_MODEL(b), *r, *end;
    s16 *id;
    Dust *s;
    GroupDls *dls;
    u32 *d;
    u64 *c;
    s32 i, k;

    if ((D_80364A90 & 0x440) != 0)
        return;
    for (id = D_80305E38; B_ID(b) != *id; id++) {
        if (*id < 0)
            return;
    }
    for (i = NDUST, s = D_803EFED0;; i--, s++) {
        if (i == 0)
            return;
        if (s->active == 0) {
            if (s->wait == 0)
                break;
        }
    }
    func_802C1214(s, b);
    d = s->dl;
    d[0] = DL_SEGMENT(9);               /* the model's textures */
    d[1] = K0(M_TEXTURES(model));
    d[2] = DL_SEGMENT(11);              /* the corners */
    d[3] = K0(s->corners);
    d += 4;
    for (i = 6, c = D_802F4780; i != 0; i--) {
        *(u64 *)d = *c++;
        d += 2;
    }
    d[0] = DL_MTX_PUSH;                 /* segment 12's matrix */
    d[1] = 0x0C000000;
    d += 2;
    k = DUST_DL_MAX;
    end = B_SECTION(b, MS_FALLS);
    for (r = B_SECTION(b, MS_GROUP_DLS); r != end; r = GDL_NEXT(r)) {
        if (GDL_GROUP(r) != group || !look_shown(r, group, b)) {
            continue;
        }
        dls = GDL_DLS(r, GDL_N(r));
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl0), (u32 *)(model + dls->dl0_end), &k, 0))
            return;
        if (!copy_cmds_n(&d, (u32 *)(model + dls->dl1), (u32 *)(model + dls->dl1_end), &k, 0))
            return;
    }
    s->y = 0;
    s->top = (((s16 *)B_SECTION(b, MS_HEIGHTS))[group - 1] << 5) - b->y;
    s->speed = 0;
    s->active = 1;
    d[0] = DL_MTX_POP;
    d[1] = 0;
    d[2] = DL_END;
    d[3] = 0;
}

/* the dust's four corners: the model's shadow rectangle (MS_SHADOW)
   DUST_MARGIN bigger, at the building's height */
REGS(v1, t9)
void func_802C1214(Dust *s, Building *b) {
    s16 *r = (s16 *)B_SECTION(b, MS_SHADOW);
    s32 y = b->y >> 5, x0 = r[0] - DUST_MARGIN, z0 = r[1] - DUST_MARGIN, x1 = r[2] + DUST_MARGIN,
        z1 = r[3] + DUST_MARGIN;

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

    for (i = 0, s = D_803F1BE0; i < NSHADOW; i++, s++, m = (Mtx *)((u8 *)m + 0x40)) {
        if (s->active == 0) {
            if (s->wait != 0) {
                s->wait--;
            }
            continue;
        }
        gr = s->growth;
        sz = s->size + gr;
        gr += SHADOW_ACCEL;
        if (sz < SHADOW_SIZE_MAX) {
            s->growth = gr;
            s->size = sz;
        } else {
            sz2 = s->held + 1;
            s->held = sz2;
            if (sz2 >= SHADOW_HOLD) {
                s->active = 0;
                s->wait = SHADOW_WAIT;
                continue;
            }
        }
        func_8026A454(s->x, s->y, s->z, sz, s->heading, m);
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
    }
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
    u8 *model = B_MODEL(b), *r, *end;
    FallShadow *s;
    GroupDls *dls;
    s16 *c;
    u32 *d0, *d1, seg = 0x02000000;
    s32 i, k0, k1, t, h;

    if (M_SHADOWS(model) == 0)
        return;
    t = func_802C1A28(group, b);
    if (t != 0)
        return;
    for (i = NSHADOW, s = D_803F1BE0;; i--, s++, seg += 0x40) {
        if (i == 0)
            return;
        if (s->active == 0) {
            if (s->wait == 0)
                break;
        }
    }
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
    for (r = B_SECTION(b, MS_GROUP_DLS); r != end; r = GDL_NEXT(r)) {
        if (GDL_GROUP(r) != group || !look_shown(r, group, b)) {
            continue;
        }
        dls = GDL_DLS(r, GDL_N(r));
        if (!copy_cmds_n(&d0, (u32 *)(model + dls->dl0), (u32 *)(model + dls->dl0_end), &k0, 0))
            return;
        if (!copy_cmds_n(&d1, (u32 *)(model + dls->dl1), (u32 *)(model + dls->dl1_end), &k1, 0))
            return;
    }
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
    h = heading_to(c[0] << 5, c[2] << 5, D_803A73F0, D_803A73F8);
    h -= 0x400;
    if (h < 0) {
        h += 0xFFF;
    }
    s->heading = h;
}

/* An explosion of radius r (<< 5) at (x, y, z) (16.16): each building it
   reaches but the level's goal takes amount (over its strength) in every
   intact group, after a while (func_802BC888). */
REGS(a1, t3, t4, t5, t6)
void func_802C18D4(s32 r, s32 x, s32 y, s32 z, s32 amount) {
    Building *b;
    s32 cx = (u32)x >> 11, cy = (u32)y >> 11, cz = (u32)z >> 11, n, g, a, hit;
    u8 *d;

    r <<= 5;
    for (b = D_803F4030; b != D_803F7654; b++) {
        hit = func_8029CFA4(cx, cy, cz, r, b->x, b->y, b->z, B_RADIUS(b));
        if (!hit)
            continue;
        if (B_ID(b) == MODEL_GOAL)
            continue;
        for (g = 1, n = B_NGROUPS(b), d = B_DAMAGE(b); n != 0; g++, n--, d++) {
            if (*d != 100) {
                if (M_STRENGTH(B_MODEL(b)) == 0) {
                    engine_break(0x802C19B4, 7);
                }
                a = engine_divu(amount, M_STRENGTH(B_MODEL(b)));
                func_802BC888(a, g, b);
            }
        }
    }
}

/* whether a group of building b's falls lower than group `group` (the
   fall heights, MS_FALLS, M_NHEIGHTS of them): the count left when one is
   found, else 0 */
REGS(t3, t9 -> a2)
s32 func_802C1A28(s32 group, Building *b) {
    s16 *f = (s16 *)B_SECTION(b, MS_FALLS);
    s32 n = M_NHEIGHTS(B_MODEL(b)), h = f[group - 1];

    for (; n != 0; f++, n--) {
        if (*f < h)
            break;
    }
    return n;
}

/* ---- the level's targets, for the game's C --------------------------- */

/* whether every target building is down by its group set
   (func_802BD064) */
s32 func_802C1AA0(void) {
    Building *b;
    s32 down;

    for (b = D_803F4030; b != D_803F7654; b++) {
        if (!B_TARGET(b))
            continue;
        down = func_802BD064(b + 1);
        if (!down) {
            return 0;
        }
    }
    return 1;
}

/* how many target buildings are down */
u8 func_802C1B1C(void) {
    Building *b;
    s32 n = 0, down;

    for (b = D_803F4030; b != D_803F7654; b++) {
        if (!B_TARGET(b))
            continue;
        down = func_802BD064(b + 1);
        if (down) {
            n++;
        }
    }
    return n;
}

/* The nearest target to the carrier (D_803EF6DC...), as func_802BCE40
   finds the player's: its position in D_803F7670..78, its distance in
   D_803F7660 (and returned).  A target building or object behind it (in
   z) only within its radius, or not at all. */
s32 func_802C1B9C(void) {
    Building *b;
    s32 cx = D_803EF6DC, cy = D_803EF6E0, cz = D_803EF6E4, n, down;
    s64 best = 9999999, d;
    UnkStruct_8039C800 *h;
    TargetObj *o;

    for (b = D_803F4030; b != D_803F7654; b++) {
        if (!B_TARGET(b))
            continue;
        if (B_DESTROYED(b))
            continue;
        d = func_802ABCDC(b->x, b->y, b->z, cx, cy, cz);
        if (!(d < best))
            continue;
        down = func_802BD064(b + 1);
        if (down)
            continue;
        if (b->z < cz) {
            if (B_RADIUS(b) < d)
                continue;
        }
        D_803F7670 = b->unk28;
        best = d;
        D_803F7674 = b->y;
        D_803F7678 = b->unk2C;
    }
    for (n = D_8039C940, h = D_8039C800; n != 0; n--, h++) {
        if (h->unk26 == 0) {
            d = func_802ABCDC(h->x, h->y, h->z, cx, cy, cz);
            if (d < best) {
                D_803F7670 = h->x;
                D_803F7678 = h->z;
                best = d;
            }
        }
    }
    for (o = D_80306480; o->level != -1; o++) {
        if (o->level != D_802E8BDC)
            continue;
        d = func_802ABCDC(o->x, o->y, o->z, cx, cy, cz);
        if (!(d < best))
            continue;
        if (o->z < cz)
            continue;
        if (!obj_destroyed(o)) {
            D_803F7670 = o->x;
            D_803F7678 = o->z;
            best = d;
        }
    }
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

    for (b = D_803F4030; b != D_803F7654; b++) {
        model = B_MODEL(b);
        if (targets != 0) {
            if (!B_TARGET(b))
                continue;
            D_803F7684 = B_ID(b);
        }
        kind = M_STRENGTH(model);
        if (kind == 0xFF)
            continue;
        if (kind != 1) {
            all++;
        }
        if ((B_DESTROYED(b) | D_803F7688) == 0)
            continue;
        if (kind != 1) {
            down++;
        }
        value += M_VALUE(model);
    }
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

    for (p = base + 4, n--; n != 0; n--) {
        p = base + *(u16 *)p;
    }
    return (s16 *)(p + 2);
}

/* record n's part `part` (from 1) placed at (x, y, z) (16.16, 39050.c):
   its matrix (this frame's, D_803F7820/24 plus its offset), and its
   corners with that id moved there */
void func_802C1F30(s32 n, s32 part, s32 x, s32 y, s32 z) {
    u8 *base = L74(D_80358074), *p, *t, *m;
    s32 id, k;

    for (p = base + 4, n--; n != 0; n--) {
        p = *(u16 *)p + base;
    }
    id = ((u16 *)(p + 0x14))[part - 1];
    if (D_8035805C != 0) {
        m = D_803F7824;
    } else {
        m = D_803F7820;
    }
    func_802ACA60(x, y, z, (s32 *)(m + id));
    func_802AC8CC((u32 *)(m + id));
    x >>= 11;
    y >>= 11;
    z >>= 11;
    for (k = *(s32 *)(p + 0x28), t = p + 0x2C; k != 0; k--, t += 0x44) {
        s32 *w = (s32 *)t;

        if (w[5] == id) {
            w[6] = x, w[7] = y, w[8] = z;
        }
        if (w[9] == id) {
            w[10] = x, w[11] = y, w[12] = z;
        }
        if (w[13] == id) {
            w[14] = x, w[15] = y, w[16] = z;
        }
    }
}

/* each frame (hd.c): the section's triangles made (into D_803F7828's
   0x28-byte records) from their points and their parts' places */
void func_802C2054(void) {
    u8 *base = L74(D_80358074), *r, *t;
    u32 *d = (u32 *)D_803F7828;
    s32 k, last;

    if (*(u32 *)base == 0)
        return;
    t = base + 4;
    do {
        r = t;
        last = *(s16 *)r;
        for (k = *(s32 *)(r + 0x28), t = r + 0x2C; k != 0; k--, t += 0x44, d += 10) {
            u16 *pt = (u16 *)t;
            s32 *w = (s32 *)t;

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
    } while (last != -1);
}
