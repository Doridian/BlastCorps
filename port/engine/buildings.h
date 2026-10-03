/*
 * What hd_code 77E20 (the buildings and their destruction), 89250 (the
 * collision tests against them) and 8A2E0 (the communication point) share,
 * as the native code sees it (engine.h).
 *
 * A level's buildings are objects.h's Building records (D_803F4030 up to
 * D_803F7654).  Each has a model file (Building.model) and two heap blocks:
 * unk4..unk8 is a list of 0x60-byte pieces, the planes it collides with,
 * which the destruction switches off group by group.  The model file holds
 * offsets (from its start) to its sections (MS_ below); a section ends where
 * the next one starts.  A building's damage is a byte per group, 0..100,
 * from Building + 0xEC (group g at 0xEC + g - 1); 100 is destroyed.
 *
 * Groups are numbered from 1 where the model's records and the pieces name
 * them, and from 0 where they index the building's arrays (B_DAMAGE and the
 * falling state below); each function says which it takes.
 */
#ifndef ENGINE_BUILDINGS_H
#define ENGINE_BUILDINGS_H

#include "shared.h"
#include "game/objects.h"

/* a piece: the plane n.p + d = 0 (s64, 16.16 by the >> 2 positions), two
   scale factors, three corners (the plane's triangle or box, << 5 >> 2) */
typedef struct Piece {
    /* 0x00 */ s64 nx;
    /* 0x08 */ s64 ny;
    /* 0x10 */ s64 nz;
    /* 0x18 */ s64 d;
    /* 0x20 */ f32 unk20;        /* |n| (func_802BD99C) */
    /* 0x24 */ f32 unk24;        /* |n|^2 */
    /* 0x28 */ s32 p[3][3];
    /* 0x4C */ u16 heading;      /* the side it pushes to (func_802BF264) */
    /* 0x4E */ s8 axis;          /* the axis it is flattest along (func_8029C0DC) */
    /* 0x4F */ u8 pad4F[2];
    /* 0x51 */ u8 active;
    /* 0x52 */ u16 group;        /* the damage group it belongs to (1-based) */
    /* 0x54 */ u8 unk54;
    /* 0x55 */ u8 unk55;
    /* 0x56 */ u8 unk56;
    /* 0x57 */ u8 group2;        /* a group whose destruction brings it back */
    /* 0x58 */ u8 pushes;        /* it turns the camera (func_802BF264) */
    /* 0x59 */ u8 pad59[7];
} Piece;
SIZE_CHECK(Piece, 0x60);

/* ---- the model file ------------------------------------------------------ */

#define B_MODEL(b) ((u8 *)(b)->model)
/* a section of building b's model file: the word at `off` is its offset */
#define B_SECTION(b, off) (B_MODEL(b) + *(s32 *)(B_MODEL(b) + (off)))

/* The model's header as the buildings read it (game/model.h's Model;
   port/host/native.c's conv_model has every record's layout).  The names
   are what the code does with them. */
#define M_NHEIGHTS(m) (*(u16 *)((u8 *)(m) + 0x0))  /* the fall heights (MS_FALLS) func_802C1A28 scans */
#define M_STRENGTH(m) ((m)[4])          /* damage is divided by it; 1 counts for the value only, 0xFF not at all */
#define M_DEBRIS(m) ((m)[5])            /* nonzero: its own debris, thrown at this speed (func_802BF978) */
#define M_UNK6(m) ((m)[6])              /* func_80264CB4's last argument; 80280 sums it */
#define M_SHADOWS(m) ((m)[7])           /* nonzero: falling groups cast shadows, and make no smoke */
#define M_VALUE(m) (*(u32 *)((u8 *)(m) + 0x8))     /* what destroying it adds to the damage (D_803649F0) */
#define M_MAIN(m) ((m)[0xD])            /* the main group: a hit there is the target's, no damage */
#define M_MOVES(m) (*(u16 *)((u8 *)(m) + 0xE))     /* LevelBuilding.behavior: nonzero, a moving building */
#define M_TEXTURES(m) ((u8 *)(m) + 0x50)            /* segment 9 while it is drawn */

/* the sections, by the offset of their offset in the header */
enum {
    MS_DL = 0x10,               /* the display list's head (to MS_DL_END), both passes */
    MS_DL_END = 0x18,
    MS_CORNERS = 0x1C,          /* four corners, s16 x, y, z: the radar's outline */
    MS_SHADOW = 0x20,           /* the shadow's rectangle: s16 x0, z0, x1, z1 */
    MS_TRIS = 0x24,             /* 0x14-byte triangles: s16 x, y, z x 3, u8 group, u8 gone */
    MS_ANIMS = 0x28,            /* the animated textures (AnimTex) */
    MS_CENTRES = 0x2C,          /* each group's centre: s16 x, y, z, 2 bytes */
    MS_FX_HIT = 0x30,           /* effect records fired by hits (FX_ below) */
    MS_FX_LAND = 0x34,          /* and by a falling group landing */
    MS_GROUP_DLS = 0x38,        /* the groups' display lists (GroupDl) */
    MS_FALLS = 0x3C,            /* each group's fall height, u16 */
    MS_HEIGHTS = 0x40,          /* each group's height, s16 */
    MS_SUPPORTS = 0x44,         /* what each group rests on (func_802BF668) */
    MS_END = 0x48
};

/* MS_GROUP_DLS: for each group's look, the conditions on the groups'
   damage that hide it (the first one is on its own group), then its lists
   for the two passes, as offsets in the model */
typedef struct GroupCond {
    /* 0x0 */ u16 group;         /* 1-based */
    /* 0x2 */ u8 below;          /* hides it while the damage is under `limit`; 0: over it */
    /* 0x3 */ u8 limit;
} GroupCond;
typedef struct GroupDls {
    /* 0x0 */ u32 dl0, dl0_end;
    /* 0x8 */ u32 dl1, dl1_end;
} GroupDls;
/* a GroupDl is {u32 n; GroupCond cond[n]; GroupDls dls;} */
#define GDL_N(r) (*(u32 *)(r))
#define GDL_COND(r) ((GroupCond *)((u8 *)(r) + 4))
#define GDL_GROUP(r) (GDL_COND(r)->group)
#define GDL_DLS(r, n) ((GroupDls *)((u8 *)(r) + 4 + (n) * 4))
#define GDL_NEXT(r) ((u8 *)GDL_DLS(r, GDL_N(r)) + sizeof(GroupDls))
/* whether condition c hides a look at damage `dmg` */
#define GCOND_HIDES(c, dmg) ((c)->below ? (dmg) < (c)->limit : (c)->limit < (dmg))
/* the damage a hidden-look test takes for the group being destroyed itself
   (func_802C09B8, func_802C0E8C, func_802C1438: the look it had just
   before) */
#define GROUP_DYING_DAMAGE 90

/* MS_ANIMS: an animated texture */
typedef struct AnimTex {
    /* 0x00 */ u32 texture;      /* the G_SETTIMG address the display list has */
    /* 0x04 */ u8 n;             /* its frames */
    /* 0x05 */ u8 cur;           /* the one shown */
    /* 0x06 */ u8 kind;          /* 0 runs through them, 1 at random, else by the camera's heading */
    /* 0x07 */ u8 blends;        /* 0: one texture; else the next one and a blend (copy_dl) */
    /* 0x08 */ u32 blend;        /* 0..254 between the two (primitive colour) */
    /* 0x0C */ u16 lo;           /* kind 0: frames a step, 1: how often; else the headings' range */
    /* 0x0E */ u16 hi;           /* kind 0: the frames into this step */
    /* 0x10 */ u32 frames[1];    /* frame f >= 1's texture at frames[f - 1] */
} AnimTex;
#define ANIM_NEXT(a) ((AnimTex *)((u8 *)(a) + (a)->n * 4 + 0xC))
/* frame f's texture (f >= 1) */
#define ANIM_FRAME(a, f) (*(u32 *)((u8 *)(a) + (f) * 4 + 0xC))

/* MS_SUPPORTS: {u8 group (1-based), strength, n; n x {u8 group, weight}} */
#define SUP_NEXT(r) ((r) + (r)[2] * 2 + 3)

/* ---- a building's state, past objects.h's fields -------------------------- */

#define B_DAMAGE(b) ((u8 *)(b) + 0xEC)
/* per group (0-based): falling (nonzero), the frames it has fallen, and its
   spin rates (12-bit angles a frame: x signed, y and z read unsigned) */
#define B_FALLING(b) ((u16 *)((u8 *)(b) + 0x48))
#define B_FALL_T(b) ((u16 *)((u8 *)(b) + 0x68))
#define B_SPIN_X(b) ((s16 *)((u8 *)(b) + 0x88))
#define B_SPIN_Y(b) ((u16 *)((u8 *)(b) + 0xA8))
#define B_SPIN_Z(b) ((u16 *)((u8 *)(b) + 0xC8))
#define B_RADIUS(b) ((b)->unkC)         /* its sphere, << 5 (func_8029CFA4) */
#define B_ID(b) ((b)->unk30)            /* the model number */
#define B_CELL(b) ((b)->unkE8)          /* the grid cell it is drawn in */
#define B_NGROUPS(b) ((b)->unkE9)
#define B_DESTROYED(b) ((b)->unkEA)
#define B_TARGET(b) ((b)->unkEB)        /* one of the level's targets */
/* the level's goal building: drawn and hit only on terms (func_802BD8C8) */
#define MODEL_GOAL 0x38

/* Native-endian memory (docs/PORT.md): where the original reaches a field
   at another width than its type's (tools/recomp/native_sites.txt), a byte
   of a word (x3) or a half of one (x2) is at the address ^ 3 or ^ 2, and a
   table kept big-endian is swapped.  The identity in big-endian memory. */
#ifdef PORT_NATIVE_ENDIAN
#define NE_X3(off) ((off) ^ 3)
#define NE_X2(off) ((off) ^ 2)
#define NE_BE16(v) ((u16)__builtin_bswap16((u16)(v)))
#define NE_BE32(v) ((u32)__builtin_bswap32((u32)(v)))
#else
#define NE_X3(off) (off)
#define NE_X2(off) (off)
#define NE_BE16(v) ((u16)(v))
#define NE_BE32(v) ((u32)(v))
#endif

/* The effect records (debris, sparks): 0x38 bytes, a model's (sections
   0x30 and 0x34), D_803F3968[30] and D_803F3FF8 (the one being made).
   Fourteen words, which func_802C04F0 copies as such, and whose halves
   (0x2C, 0x2E) and bytes (0x2E's high one, 0x30-0x37) are read and
   written as the N64 has them. */
#define FX_W(d, off) (*(s32 *)((u8 *)(d) + (off)))
#define FX_H(d, off) (*(u16 *)((u8 *)(d) + NE_X2(off)))
#define FX_B(d, off) (*(u8 *)((u8 *)(d) + NE_X3(off)))
enum {
    FX_X = 0x00, FX_Y = 0x04, FX_Z = 0x08,     /* W: where (16.16) */
    FX_SPEED = 0x0C,            /* W: func_802A6274's second argument */
    FX_VEL = 0x10,              /* W x 3: its velocity (func_802A6274's t6, t7, s0) */
    FX_UNK14 = 0x1C,            /* W x 3: (s1-s3) */
    FX_UNK28 = 0x28,            /* W: (s4) moved with the building's y; -1000.0 made */
    FX_DELAY = 0x2C,            /* H: frames before it starts */
    FX_RADIUS = 0x2E,           /* H: the damage's radius (func_802C18D4), 0 none */
    FX_AMOUNT = 0x30,           /* B: the damage, / 16 */
    FX_KIND = 0x31,             /* B: what it is (D_80306270's sprites, its sound) */
    FX_GROUP = 0x32,            /* B: the group (1-based) that fires it */
    FX_LIMIT = 0x33,            /* B: the group's damage it fires at; 0xFF on any strong hit */
    FX_DONE = 0x34,             /* B: fired (a model's record), free (D_803F3968's) */
    FX_UNK35 = 0x35,            /* B: func_802A6274's s5 */
    FX_SIZE = 0x38
};
#define NFX 30                  /* D_803F3968's records */
#define FX_KIND_SPARK 0x15      /* a communication point's sparks: always started big */
/* the hit a record marked 0xFF needs (D_803F77FE), and a debris roll */
#define FX_STRONG_HIT 20
/* the frames each debris piece but the first waits (func_802BF978) */
#define FX_DEBRIS_DELAY 10
/* where a debris record starts its FX_UNK28: -1000.0 */
#define FX_UNK28_INIT 0xFC180000

/* ---- the other per-frame things (77E20's .bss) ------------------------------ */

/* the queued damage (D_803F7690[40]) */
typedef struct DelayedHit {
    /* 0x0 */ Building *PTR32 b;
    /* 0x4 */ u16 amount;
    /* 0x6 */ u8 group;          /* 1-based */
    /* 0x7 */ u8 frames;         /* until it is done; 0 free */
} DelayedHit;
SIZE_CHECK(DelayedHit, 8);
#define NDELAYED 40
/* how long a queued hit waits: func_8026A8E0(1, DELAY_MAX) frames */
#define DELAY_MAX 20

/* this frame's (building, group) pairs hit (D_803F3910 to D_803F3960) */
typedef struct HitPair {
    Building *PTR32 b;
    s32 group;                  /* 1-based */
} HitPair;

/* a smoke cloud (D_803F0900[4]): a group's look copied, fading */
typedef struct Smoke {
    /* 0x000 */ u32 dl[0x12C];
    /* 0x4B0 */ s16 alpha;       /* 0: not drawn */
    /* 0x4B2 */ u8 wait;         /* frames before it can be used again */
    /* 0x4B3 */ u8 pad[5];
} Smoke;
SIZE_CHECK(Smoke, 0x4B8);
#define NSMOKE 4
#define SMOKE_DL_MAX 0x94       /* the commands it copies, at most (- 1) */
#define SMOKE_ALPHA 0xFF        /* where it starts */
#define SMOKE_FADE 20           /* its alpha's fall a frame */
#define SMOKE_WAIT 3            /* the frames it rests when gone */

/* the dust (D_803EFED0, one): a group's look, rising from the ground */
typedef struct Dust {
    /* 0x000 */ u32 dl[0x258];
    /* 0x960 */ s32 mtx[2][16];  /* by frame (D_8035805C) */
    /* 0x9E0 */ s16 corners[4][8];       /* Vtx */
    /* 0xA20 */ s32 top;         /* how far it rises (<< 5) */
    /* 0xA24 */ s32 y;           /* how far it has */
    /* 0xA28 */ u16 speed;
    /* 0xA2A */ u8 active;
    /* 0xA2B */ u8 wait;
    /* 0xA2C */ u8 padA2C[4];
} Dust;
SIZE_CHECK(Dust, 0xA30);
#define NDUST 1
#define DUST_DL_MAX 0x121       /* the commands it copies, at most (- 1) */
#define DUST_ACCEL 16           /* its speed's rise a frame */
#define DUST_SHAKE 32           /* it is shaken by up to this either way, each frame */
#define DUST_WAIT 3
#define DUST_MARGIN 100         /* its corners: the shadow's rectangle this much bigger */

/* a falling group's shadow (D_803F1BE0[2]) */
typedef struct FallShadow {
    /* 0x000 */ u32 dl0[0x8C];   /* the two passes' lists */
    /* 0x230 */ u32 dl1[0x8C];
    /* 0x460 */ u32 textures;    /* its model's (segment 9) */
    /* 0x464 */ s16 x, y, z;
    /* 0x46A */ u16 heading;
    /* 0x46C */ u16 size;
    /* 0x46E */ u16 growth;      /* the size's rise this frame */
    /* 0x470 */ u8 active;
    /* 0x471 */ u8 held;         /* the frames at its full size */
    /* 0x472 */ u8 wait;
    /* 0x473 */ u8 pad473[5];
} FallShadow;
SIZE_CHECK(FallShadow, 0x478);
#define NSHADOW 2
#define SHADOW_DL_MAX 0x43      /* the commands each list copies, at most (- 1) */
#define SHADOW_ACCEL 15         /* the growth's rise a frame */
#define SHADOW_SIZE_MAX 0x385   /* it stops growing at */
#define SHADOW_HOLD 6           /* and then lasts these frames */
#define SHADOW_WAIT 3

/* A falling group comes down FALL_ACCEL * t^2 (16.16, t its frames) and
   lands at its fall height (MS_FALLS, << 16) */
#define FALL_ACCEL 30000
/* the pushing that brings a building down (func_802BEFF4): this many
   frames in a row, steering one way */
#define PUSH_FRAMES 20

/* the screen's shake (hd_code 00000.c: D_802E8BE4 frames, by D_802E8BE8
   either way, the size falling by a sixth each frame) */
extern u8 D_802E8BE4;
extern s32 D_802E8BE8;
#define SHAKE_HIT_FRAMES 10     /* a hit: as big as its damage, up to SHAKE_HIT_MAX */
#define SHAKE_HIT_MAX 500
#define SHAKE_DOWN_FRAMES 15    /* a group destroyed by a vehicle */
#define SHAKE_DOWN 400
#define SHAKE_LANDED 300        /* a falling group landing (SHAKE_HIT_FRAMES) */
#define SHAKE_PLAYER_HIT 200    /* the game's C's hits (89250) */

/* ---- the collision tests' state -------------------------------------------- */

/* The collision tests' state (89250): the player's position, the
   camera's limits (12-bit headings) and what narrows them. */
extern s32 D_803A73F0, D_803A73F4, D_803A73F8;
extern u16 D_803A7410, D_803A7412;
extern u8 D_803A742F;                           /* limits off */
extern u8 D_803A7424, D_803A7425, D_803A7427, D_803A742A;
extern s8 *PTR32 D_803A7408;                    /* the kinds that turn the camera, to -1 */

/* the level's other solid objects: 0x14-byte records, to an `end` of -1 */
typedef struct Solid {
    /* 0x00 */ s32 x, y, z, r;
    /* 0x10 */ u8 kind;
    /* 0x11 */ s8 end;           /* -1 after the last; 0: not solid */
    /* 0x12 */ u8 pad12[2];
} Solid;
SIZE_CHECK(Solid, 0x14);
extern Solid D_803A7300[];

/* the kinds' parts: 0x14-byte records, to an `end` of -1 */
typedef struct KindPart {
    /* 0x00 */ s32 x, y, z, r;
    /* 0x10 */ u16 power;        /* the damage it does (func_802BEBB0) */
    /* 0x12 */ u8 kind;
    /* 0x13 */ s8 end;
} KindPart;
SIZE_CHECK(KindPart, 0x14);
extern KindPart D_803A6B30[];

/* the registers by number, for engine_save()'s masks (engine.h) */
enum {
    rAT = 1, rV0, rV1, rA0, rA1, rA2, rA3, rT0, rT1, rT2, rT3, rT4, rT5, rT6, rT7,
    rS0, rS1, rS2, rS3, rS4, rS5, rS6, rS7, rT8, rT9, rK0, rK1, rGP, rSP, rFP, rRA
};
#define G(r) ENGINE_GPR(r)
/* what the original saves and loads back, undone in the thread's context
   as it undoes it (the registers its translated callees, and their REGS(),
   leave there) */
#define ENGINE_SAVE(gmask) engine_save((gmask), 0)
#define ENGINE_RESTORE() engine_restore()

/* divu, as the VR4300 (and the translation) gives it for 0 */
static inline u32 engine_divu(u32 n, u32 d) { return d != 0 ? n / d : 0xFFFFFFFFu; }
static inline u32 engine_remu(u32 n, u32 d) { return d != 0 ? n % d : n; }

#define K0(p) ((u32)(p) - 0x80000000)

/* the display list commands 77E20 and 8A2E0 write */
#define DL_SEGMENT(seg) (0xBC000006 | (seg) * 4 << 8)  /* G_MOVEWORD: segment seg's address */
#define DL_CALL 0x06000000              /* G_DL, pushing */
#define DL_END 0xB8000000               /* G_ENDDL */
#define DL_MTX_PUSH 0x01040040          /* G_MTX: modelview, multiplied, pushed */
#define DL_MTX_POP 0xBD000000           /* G_POPMTX */
#define DL_PIPESYNC 0xE7000000
#define DL_ENVCOLOR 0xFB000000
#define DL_SETTIMG 0xFD                 /* (opcodes) */
#define DL_SETPRIMCOLOR 0xFA

/* ---- the CPU model's charges (engine.h's ENGINE_BLK) ----------------------- */

/* ENGINE_BLKN(addr, k): block addr's charge k times at once (a loop that
   ran k times, done with memcpy).  The check and block-log builds want
   every one. */
#if defined(PORT_ENGINE_CHECK) || defined(PORT_BLKLOG)
#define ENGINE_BLKN(addr, k)                                                \
    do {                                                                    \
        u32 blkn_ = (k);                                                    \
        while (blkn_--)                                                     \
            ENGINE_BLK(addr);                                               \
    } while (0)
#define ENGINE_BLKN_T(b, k)                                                 \
    do {                                                                    \
        u32 blkn_ = (k);                                                    \
        while (blkn_--)                                                     \
            ENGINE_BLK_((b).id, (b).n);                                     \
    } while (0)
#else
#define ENGINE_BLKN_(id, n, k) (__port_icount += (u32)(n) * (u32)(k))
#define ENGINE_BLKN_X(...) ENGINE_BLKN_(__VA_ARGS__)
#define ENGINE_BLKN(addr, k) ENGINE_BLKN_X(ENGINE_BLK_##addr, (k))
#define ENGINE_BLKN_T(b, k) (__port_icount += (u32)(b).n * (u32)(k))
#endif

/* a block named in a table, for code several functions share with blocks
   of their own: {ENGINE_BLK_802BE228} */
typedef struct { u16 id, n; } Blk;
#define BLKT(b) ENGINE_BLK_((b).id, (b).n)

/* ---- 56040's (engine-A's): the collision tests ---- */

/* whether the spheres (x, y, z, r) and (bx, by, bz, br) overlap */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t2)
s32 func_8029CFA4(s32 x, s32 y, s32 z, s32 r, s32 bx, s32 by, s32 bz, s32 br);
/* whether the sphere (x, y, z, r) crosses piece p's plane; where it does,
   the point on the plane (*px, *py, *pz) */
REGS(t3, t4, t5, t6, s0 -> v0, v1, a0, a1)
s32 func_8029C160(s32 x, s32 y, s32 z, s32 r, Piece *p, s32 *px, s32 *py, s32 *pz);
/* that point and piece p's corners, flattened along its axis (56040.c,
   native): the corners in 2D left in $v0-$a3, the point in $t0, $t1 */
REGS(s0, v1, a0, a1)
void func_8029C0DC(u8 *part, s32 x, s32 y, s32 z);
/* whether the 2D point (x, z) is inside the triangle */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t7)
s32 func_8029BF64(s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3, s32 x, s32 z);
/* whether the sphere crosses one of piece p's edges */
REGS(t3, t4, t5, t6, s0 -> t7)
s32 func_8029BD0C(s32 x, s32 y, s32 z, s32 r, Piece *p);
/* whether the sphere holds piece p's first corner (56040.c, native) */
REGS(s0, t3, t4, t5, t6 -> t7)
s32 func_8029BEE4(u8 *part, s32 x, s32 y, s32 z, s32 r);

/* Piece p's corners and the point (x, y, z) seen along its axis (0 drops
   z, 1 y, else x): func_8029BF64's arguments.  This is 56040.c's c0dc()
   (static there), func_8029C0DC's body, with its blocks; like
   func_8029C0DC it also leaves them in $v0-$t1, where the original's
   callers took them from and later code might find them. */
typedef struct PieceFlat {
    s32 x1, z1, x2, z2, x3, z3, x, z;
} PieceFlat;

static inline void piece_flat(Piece *p, s32 x, s32 y, s32 z, PieceFlat *f) {
    s32 *w = &p->p[0][0], q[3];
    s32 i, j;

    ENGINE_BLK(8029C0DC);
    if (p->axis == 0) {
        ENGINE_BLK(8029C138);
        i = 0, j = 1;
    } else if (p->axis == 1) {
        ENGINE_BLK(8029C0E8);
        ENGINE_BLK(8029C114);
        i = 0, j = 2;
    } else {
        ENGINE_BLK(8029C0E8);
        ENGINE_BLK(8029C0F0);
        i = 1, j = 2;
    }
    ENGINE_BLK(8029C158);
    q[0] = x, q[1] = y, q[2] = z;
    f->x1 = w[i], f->z1 = w[j];
    f->x2 = w[3 + i], f->z2 = w[3 + j];
    f->x3 = w[6 + i], f->z3 = w[6 + j];
    f->x = q[i], f->z = q[j];
    ENGINE_LEAVE(rV0, f->x1), ENGINE_LEAVE(rV1, f->z1);
    ENGINE_LEAVE(rA0, f->x2), ENGINE_LEAVE(rA1, f->z2);
    ENGINE_LEAVE(rA2, f->x3), ENGINE_LEAVE(rA3, f->z3);
    ENGINE_LEAVE(rT0, f->x), ENGINE_LEAVE(rT1, f->z);
}

/* The sphere (x, y, z, r) (>> 2) against piece p: across its plane
   (func_8029C160), and then inside its triangle there, across one of its
   edges or holding its first corner.  Three callers (func_802BEADC,
   func_802BEBB0, func_802CDC7C) do it with blocks of their own, in this
   order: after the plane's test, its point flattened, after that, after
   the triangle's test, before and after the edges', and the corner's. */
typedef struct PieceTestBlks {
    Blk plane, cross, flat, tri, edge0, edge, corner0, corner;
} PieceTestBlks;

static inline s32 piece_touched(Piece *p, s32 x, s32 y, s32 z, s32 r, const PieceTestBlks *k) {
    PieceFlat f;
    s32 px, py, pz, in;

    in = func_8029C160(x, y, z, r, p, &px, &py, &pz);
    BLKT(k->plane);
    if (!in)
        return 0;
    BLKT(k->cross);
    piece_flat(p, px, py, pz, &f);
    BLKT(k->flat);
    in = func_8029BF64(f.x1, f.z1, f.x2, f.z2, f.x3, f.z3, f.x, f.z);
    BLKT(k->tri);
    if (in)
        return 1;
    BLKT(k->edge0);
    in = func_8029BD0C(x, y, z, r, p);
    BLKT(k->edge);
    if (in)
        return 1;
    BLKT(k->corner0);
    in = func_8029BEE4((u8 *)p, x, y, z, r);
    BLKT(k->corner);
    return in;
}

/* the camera's heading kept between two angles */
REGS(a0, a1)
void func_8029B7CC(s32 a, s32 b);
/* the vehicle collision against the buildings (89250's func_802CE5BC) */
REGS(a1, a2, a3, t3, t4, t5, t6, t8, fp)
void func_8029B02C(s32 a1, s32 a2, s32 a3, s32 x, s32 y, s32 z, s32 r, s32 t8, s32 fp);

/* The heading from (x, z) to (tx, tz), a 12-bit angle (0 along +x, a
   quarter towards -z): the arcsine (func_802AD7FC) of the 16.16 sine,
   quadrant by quadrant, with each quadrant's blocks.  Three functions
   (func_802BDDB4, func_802C1438, func_802CE204) do it alike. */
typedef struct HeadingBlks {
    Blk q0, q0b, q1, q1b, q2, q2b, q3, q3b, right, left;
} HeadingBlks;

static inline s32 heading_to(s32 x, s32 z, s32 tx, s32 tz, const HeadingBlks *k) {
    f32 d, q;
    s32 h;

    d = (f32)(tx - x) * (f32)(tx - x);
    q = (f32)(tz - z) * (f32)(tz - z);
    d = __builtin_sqrtf(d + q);
    if (!(tx < x)) {
        BLKT(k->right);
        if (!(tz < z)) {
            BLKT(k->q0);
            h = func_802AD7FC(engine_cvt_w_s(65536.0f * ((f32)(tx - x) / d)));
            BLKT(k->q0b);
            return (u32)h >> 4;
        }
        BLKT(k->q1);
        h = func_802AD7FC(engine_cvt_w_s(65536.0f * ((f32)(z - tz) / d)));
        BLKT(k->q1b);
        return ((u32)h >> 4) + 0x400;
    }
    BLKT(k->left);
    if (tz < z) {
        BLKT(k->q2);
        h = func_802AD7FC(engine_cvt_w_s(65536.0f * ((f32)(x - tx) / d)));
        BLKT(k->q2b);
        return ((u32)h >> 4) + 0x800;
    }
    BLKT(k->q3);
    h = func_802AD7FC(engine_cvt_w_s(65536.0f * ((f32)(tz - z) / d)));
    BLKT(k->q3b);
    return ((u32)h >> 4) + 0xC00;
}

/* ---- 77E20 (this module's), as 89250 and 8A2E0 call it ---- */
REGS(-> t1)
s32 func_802BD8C8(void);
REGS(s0)
void func_802BF264(Piece *p);
REGS(t3, t5, t9)
void func_802BF898(s32 group, s32 damage, Building *b);
REGS(t3, t9)
void func_802BF1F0(s32 group, Building *b);
REGS(t3, t9)
void func_802C1438(s32 group, Building *b);
REGS(t3, t9)
void func_802C09B8(s32 group, Building *b);
REGS(t3, t9)
void func_802C0E8C(s32 group, Building *b);
REGS(t9)
void func_802BF384(Building *b);
REGS(t9)
void func_802BF668(Building *b);
REGS(t9)
void func_802BF534(Building *b);
REGS()
void func_802BCBD8(void);
REGS(a1, t3, t4, t5, t6)
void func_802C18D4(s32 a1, s32 x, s32 y, s32 z, s32 t6);
REGS()
void func_802C049C(void);
REGS(v1)
void func_802C04F0(u8 *fx);

#endif
