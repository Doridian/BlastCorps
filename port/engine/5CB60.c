/*
 * hd_code 5CB60 (us.v11 0x802A1320-0x802A4510): the level loader's parts,
 * as native C (engine.h): the parts that turn the level file's sections
 * into the run-time tables, and (at the end) the loader itself
 * (func_802A1674) with its vehicle and model parts.
 *
 * The originals take the level's header in $t0.  What they took from
 * registers nobody set for them (the collision triangles' bytes, the
 * textures' palette) are values now: LEVEL_TRI_BYTES, tex_param.
 *
 * The level file is the ROM's big-endian data: what the original reads a
 * byte at a time (records at any alignment) is read a byte at a time here
 * (BE16S, BE16U, UNALIGNED_W), and what it reads as halfwords or words is
 * read at that width (native-endian memory keeps those in host order).
 */
#include "engine.h"
#include "game/game.h"
#include "game/level.h"
#include "game/vehicle.h"
#include "game/objects.h"
#include "game/model.h"
#include "collision.h"
#include "level_tables.h"

/* ---- the level file's and the run-time records -------------------------- */

/* a LevelHeader (or Model, VehicleModel) offset field: what it points at */
#define LEVEL_PTR(h, field) ((u8 *)(h) + (h)->field)
#define MODEL_PTR(m, field) ((u8 *)(m) + ((Model *)(m))->field)
#define VMODEL_PTR(m, field) ((u8 *)(m) + ((VehicleModel *)(m))->field)

/* a big-endian u32 at any alignment in the level file: in native-endian
   memory the loaders have put it in host order where it is (docs/PORT.md,
   "Native-endian memory": the lwl/lwr pairs) */
#ifdef PORT_NATIVE_ENDIAN
#define UNALIGNED_W(p) ((u32)((p)[3] << 24 | (p)[2] << 16 | (p)[1] << 8 | (p)[0]))
#else
#define UNALIGNED_W(p) ((u32)((p)[0] << 24 | (p)[1] << 16 | (p)[2] << 8 | (p)[3]))
#endif

/* a byte of a wider datum that the original reaches at its N64 address
   (tools/recomp/native_sites.txt's x3): in native-endian memory it is at
   the address ^ 3; and a halfword it keeps big-endian (`be`) */
#ifdef PORT_NATIVE_ENDIAN
#define X3(p) (*(u8 *)((u32)(p) ^ 3))
#define STORE_BE16(p, v) (*(u16 *)(p) = (u16)__builtin_bswap16((u16)(v)))
#else
#define X3(p) (*(u8 *)(p))
#define STORE_BE16(p, v) (*(u16 *)(p) = (u16)(v))
#endif

/* a big-endian halfword at any alignment, read a byte at a time */
#define BE16S(p) ((s16)(((s8 *)(p))[0] << 8 | ((u8 *)(p))[1]))
#define BE16U(p) ((u32)(((u8 *)(p))[0] << 8 | ((u8 *)(p))[1]))

#define GBI_DL 0x06             /* F3D's G_DL: a display list call */
#define KSEG0 0x80000000        /* a cached address minus this is physical */
#define LIST_END (-1)           /* the end of the run-time tables' lists */

/* the tables func_802A2D68 empties */
#define N_KIND_PARTS 100        /* D_803A6B30 */
#define N_SOLIDS 12             /* D_803A7300 */
#define N_ED3B8 12              /* D_803ED3B8 */
#define N_EBC10 26              /* D_803EBC10 */

/* (TargetObj, D_80306480, and LevelLight, D_803BDFD8: level_tables.h) */

/* LevelHeader.unk74's moving triangles: a word (the bytes of matrices the
   section's parts need), then groups of triangles; D_803F7828's run-time
   ones (77E20's func_802C2054 moves them each frame) */
typedef struct MovingTriFile {
    /* 0x00 */ s16 v[3][3];         /* the corners, world units */
    /* 0x12 */ u8 unk12;
    /* 0x13 */ u8 pad13[0x31];      /* (77E20 reads the parts' ids and
                                       offsets in here) */
} MovingTriFile;
SIZE_CHECK(MovingTriFile, 0x44);

typedef struct MovingGroup {
    /* 0x00 */ s16 next;            /* the next group's offset in the
                                       section (77E20); -1 on the last */
    /* 0x02 */ u8 pad2[0x26];
    /* 0x28 */ u32 count;
    /* 0x2C */ MovingTriFile tris[];
} MovingGroup;
SIZE_CHECK(MovingGroup, 0x2C);

typedef struct MovingTri {
    /* 0x00 */ s32 v[3][3];         /* << 5 */
    /* 0x24 */ u8 unk24;
    /* 0x25 */ u8 pad25[3];
} MovingTri;
SIZE_CHECK(MovingTri, 0x28);

/* the level's records the loader reads a byte at a time */
typedef struct FileVehicle {    /* LevelHeader.vehicles (LEVEL_VEHICLE_SIZE) */
    /* 0x0 */ u8 type;              /* VehicleType */
    /* 0x1 */ u8 x[2], y[2], z[2];  /* s16, world units */
    /* 0x7 */ u8 heading[2];
} FileVehicle;
SIZE_CHECK(FileVehicle, LEVEL_VEHICLE_SIZE);

typedef struct FileCarrier {    /* LevelHeader.carrier (LEVEL_CARRIER_SIZE) */
    /* 0x0 */ u8 speed;             /* 0: no carrier */
    /* 0x1 */ u8 x[2], z[2];        /* s16, world units */
    /* 0x5 */ u8 heading[2];
    /* 0x7 */ u8 distance[2];       /* to the end of the level */
    /* 0x9 */ u8 unk9;
} FileCarrier;
SIZE_CHECK(FileCarrier, LEVEL_CARRIER_SIZE);

typedef struct FileBuilding {   /* LevelHeader.buildings (LevelBuilding) */
    /* 0x0 */ u8 x[2], y[2], z[2];  /* u16, world units */
    /* 0x6 */ u8 model[2];          /* the model table's index */
    /* 0x8 */ u8 unk8;              /* Building.unkEB; a group member */
    /* 0x9 */ u8 unk9;              /* Model.unk6, summed into D_8036EB93 */
    /* 0xA */ u16 behavior;
    /* 0xC */ u16 speed;            /* Building.unk34 */
} FileBuilding;
SIZE_CHECK(FileBuilding, 0xE);

/* a building model's collision triangle (Model.unk48..unk4C) */
typedef struct BuildingTri {
    /* 0x00 */ u8 v[18];            /* nine big-endian s16: the corners */
    /* 0x12 */ u8 heading[2];
    /* 0x14 */ u8 unk14;            /* CollisionTri.unk56 */
    /* 0x15 */ u8 group;            /* CollisionTri.group */
    /* 0x16 */ u8 pushes;           /* CollisionTri.pushes */
    /* 0x17 */ u8 group2;           /* CollisionTri.group2; not active */
    /* 0x18 */ u8 unk18;            /* CollisionTri.unk55 */
} BuildingTri;
SIZE_CHECK(BuildingTri, 0x19);

/* a building model's effect (Model.unk30..unk34, unk34..unk38) */
typedef struct ModelEffect {
    /* 0x00 */ s32 x, y, z;         /* world units; << 16 once placed */
    /* 0x0C */ u8 padC[0x1C];
    /* 0x28 */ s32 unk28;           /* a height: moves with y */
    /* 0x2C */ u8 pad2C[4];
    /* 0x30 */ u8 unk30[4];         /* a word elsewhere; 0x31 its byte (X3) */
    /* 0x34 */ u8 pad34[4];
} ModelEffect;
SIZE_CHECK(ModelEffect, 0x38);

/* animated textures: a count of frames at 4 and the frames' numbers (the
   loader makes them addresses); a model's (Model.unk28..unk2C) has its
   first frame at 0 and the rest from 0x10, the level's
   (LevelHeader.animTextures) the rest from 0xC */
typedef struct ModelAnimTex {
    /* 0x00 */ u32 frame0;
    /* 0x04 */ u8 nframes;
    /* 0x05 */ u8 pad5[0xB];
    /* 0x10 */ u32 frames[];
} ModelAnimTex;
SIZE_CHECK(ModelAnimTex, 0x10);

typedef struct LevelAnimTex {
    /* 0x00 */ u8 pad0[4];
    /* 0x04 */ u8 nframes;
    /* 0x05 */ u8 pad5[7];
    /* 0x0C */ u32 frames[];
} LevelAnimTex;
SIZE_CHECK(LevelAnimTex, 0xC);

extern f32 D_803EBBF0;          /* gravity */
extern u8 *PTR32 D_80364458;    /* the level's display data, after its header */
extern u8 *PTR32 D_803EBBEC;    /* the end of D_803EBDB0's records */
extern u8 D_803EBDB0[];
extern u8 D_803ED3B8[N_ED3B8][4];
extern s32 D_803EBC10[N_EBC10][4];
extern FileVehicle *PTR32 D_803BE6F8;   /* the level's vehicles' records */
extern u8 D_803A7426, D_803F7805, D_803F780A, D_803F780B, D_803F780C, D_803F7810;
extern u8 D_803F7804;
#ifndef VERSION_US_V10
extern u8 D_803F7812;
#endif
extern s32 D_803A740C, D_803F77F8;
extern u8 D_80364A6E;           /* the level's ambient light */
extern u8 *PTR32 D_803F7820, *PTR32 D_803F7824;  /* the matrices, two sets */
extern MovingTri *PTR32 D_803F7828, *PTR32 D_803F782C;

/* func_802A2D68: the header's grids and constants; the run-time tables
   emptied */
REGS(t0)
void func_802A2D68(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    s32 i;
    s16 *b;

    D_803EBBF0 = (f32)h->gravity;
    D_80364458 = (u8 *)h + sizeof(LevelHeader);
    D_803BDAF4 = (s16 *)LEVEL_PTR(h, collisionFixes);
    D_803BDAF8 = (s16 *)LEVEL_PTR(h, commPoint);        /* (the fixes' end) */
    D_803BE714 = h->unk0[0];
    D_803BE716 = h->unk0[1];
    D_803BE70C = h->unk4[0] << 5;
    D_803BE710 = h->unk4[1] << 5;
    D_803BE720 = h->unk8[0];
    D_803BE722 = h->unk8[1];
    D_803BE718 = h->unkC[0] << 5;
    D_803BE71C = h->unkC[1] << 5;
    D_803BE72C = h->unk10[0];
    D_803BE72E = h->unk10[1];
    D_803BE724 = h->unk14[0] << 5;
    D_803BE728 = h->unk14[1] << 5;
    D_803EBBEC = D_803EBDB0;
    for (i = 0; i < N_KIND_PARTS; i++) {
        D_803A6B30[i].end = LIST_END;
    }
    for (i = 0; i < N_SOLIDS; i++) {
        D_803A7300[i].end = LIST_END;
    }
    for (i = 0; i < N_ED3B8; i++) {
        u8 *p = D_803ED3B8[i];

        p[0] = p[1] = p[2] = p[3] = 0xFF;
    }
    for (i = 0; i < N_EBC10; i++) {
        D_803EBC10[i][0] = LIST_END;
    }
    D_803BE6F8 = (FileVehicle *)LEVEL_PTR(h, vehicles);
    b = (s16 *)LEVEL_PTR(h, levelBounds);
    D_803BE730 = b[0];
    D_803BE734 = b[1];
    D_803BE732 = b[2];
    D_803BE736 = b[3];
    D_803A7426 = 0;
    D_803ED400 = 0;
    D_803F7805 = 0;
    D_803F7806 = 0;
    D_803BE738 = 0;
    D_803BE739 = h->unk1C;
    D_803EFECB = 0;
    D_803EF6FF = 0;
    D_803ED40C = 0;
    D_803A740C = 0;
    D_803F77F8 = 0;
    D_803F780A = 0;
    D_803F780B = 0;
    D_803F780C = 0;
    D_803F7810 = 0;
    D_803F7804 = 0;
#ifndef VERSION_US_V10
    D_803F7812 = 0;         /* (us.v10 doesn't) */
#endif
}

/* func_802A1C88: the level's display lists: their G_DL addresses made
   physical, and the four the C draws */
REGS(t0)
void func_802A1C88(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u32 base = (u32)LEVEL_PTR(h, displayLists[0]) - KSEG0;
    u32 *p = (u32 *)LEVEL_PTR(h, displayLists[6]);
    u32 *end = (u32 *)LEVEL_PTR(h, displayLists[3]);

    for (; p != end; p += 2) {
        if (p[0] >> 24 == GBI_DL) {
            p[1] += base;
        }
    }
    D_803BE6E0 = (Gfx *)LEVEL_PTR(h, displayLists[6]);
    D_803BE6E4 = (Gfx *)LEVEL_PTR(h, displayLists[7]);
    D_803BE6E8 = (Gfx *)LEVEL_PTR(h, displayLists[8]);
    D_803BE6EC = (Gfx *)LEVEL_PTR(h, displayLists[9]);
}

/* func_802A4464: a pointer to each terrain group's data (LevelHeader.
   terrain: each a big-endian offset from the section to the next, then
   its data), and one past the last */
REGS(t0)
void func_802A4464(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    s32 n = (s16)h->unk8[0] * (s16)h->unk8[1];
    u8 *base = LEVEL_PTR(h, terrain);
    u8 *p = base;
    u32 *out = (u32 *)D_803BDB10;

    for (; n != 0; n--) {
        *out++ = (u32)(p + 4);
        p = base + UNALIGNED_W(p);
    }
    *out = (u32)(p + 4);
}

/* func_802A44E4: a size rounded up to 8, if it isn't a multiple of 4 */
REGS(a1 -> a1)
u32 func_802A44E4(u32 a) {
    if (a & 3) {
        a = (a & ~7) + 8;
    }
    return a;
}

/* func_802A1934: D_80306480's boxes' corners, from the centre and size */
REGS()
void func_802A1934(void) {
    TargetObj *b;

    for (b = D_80306480; b->level != LIST_END; b++) {
        s32 x, y, z, r;

        x = b->x >> 5;
        y = b->y >> 5;
        z = b->z >> 5;
        r = b->size;
        b->corners[0][0] = b->corners[2][0] = x - r;
        b->corners[0][2] = b->corners[1][2] = z - r;
        b->corners[1][0] = b->corners[3][0] = x + r;
        b->corners[2][2] = b->corners[3][2] = z + r;
        b->corners[0][1] = b->corners[1][1] = b->corners[2][1] = b->corners[3][1] = y;
    }
}

/* func_802A2C54: the level's lights (LevelHeader.unk60: the ambient
   light's byte, then records of four big-endian u16, two bytes, n types
   and two bytes) into D_803BDFD8 */
REGS(t0)
void func_802A2C54(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *p = LEVEL_PTR(h, unk60);
    u8 *end = LEVEL_PTR(h, unk64);
    LevelLight *l;

    D_80364A6E = *p++;
    for (l = D_803BDFD8; p != end; l++) {
        u32 n, k;

        l->x = BE16U(p + 0) << 5;
        l->y = BE16U(p + 2) << 5;
        l->z = BE16U(p + 4) << 5;
        l->radius = BE16U(p + 6) << 5;
        l->full = p[8];
        n = p[9];
        l->ntypes = n;
        p += 10;
        for (k = 0; k < n; k++) {
            l->types[k] = *p++;
        }
        l->building = p[0];
        l->shade = p[1];
        l->on = 1;
        p += 2;
    }
    D_803BDFD4 = l;
}

/* an identity Mtx (s15.16): the integer halves' 1s and 0s, then the
   fractions' 0s, at the widths the original stores them */
static void mtx_identity(u8 *t) {
    *(s16 *)(t + 0x00) = 1;
    *(s16 *)(t + 0x02) = 0;
    *(s32 *)(t + 0x04) = 0;
    *(s16 *)(t + 0x08) = 0;
    *(s16 *)(t + 0x0A) = 1;
    *(s32 *)(t + 0x0C) = 0;
    *(s32 *)(t + 0x10) = 0;
    *(s16 *)(t + 0x14) = 1;
    *(s16 *)(t + 0x16) = 0;
    *(s32 *)(t + 0x18) = 0;
    *(s16 *)(t + 0x1C) = 0;
    *(s16 *)(t + 0x1E) = 1;
    *(s32 *)(t + 0x20) = 0;
    *(s32 *)(t + 0x24) = 0;
    *(s32 *)(t + 0x28) = 0;
    *(s32 *)(t + 0x2C) = 0;
    *(s32 *)(t + 0x30) = 0;
    *(s32 *)(t + 0x34) = 0;
    *(s32 *)(t + 0x38) = 0;
    *(s32 *)(t + 0x3C) = 0;
}

#define MTX_SIZE 0x40

/* func_802A1A9C: LevelHeader.unk74: two sets of identity matrices (as
   many bytes as its first word says, each), and its groups' moving
   triangles after them (D_803F7828..D_803F782C) */
REGS(t0)
void func_802A1A9C(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *src = LEVEL_PTR(h, unk74);
    u32 size = *(u32 *)src;
    u8 *t = D_80358070;
    u8 *end;
    MovingTri *d;

    D_803F7820 = t;
    D_803F7824 = t + size;
    end = t + size * 2;
    for (; t != end; t += MTX_SIZE) {
        mtx_identity(t);
    }
    d = (MovingTri *)t;
    D_803F7828 = d;
    if (size != 0) {
        MovingGroup *g;
        s32 next;

        g = (MovingGroup *)(src + 4);
        do {
            MovingTriFile *e = g->tris;
            u32 n;

            next = g->next;
            for (n = g->count; n != 0; n--, e++, d++) {
                d->v[0][0] = e->v[0][0] << 5;
                d->v[0][1] = e->v[0][1] << 5;
                d->v[0][2] = e->v[0][2] << 5;
                d->v[1][0] = e->v[1][0] << 5;
                d->v[1][1] = e->v[1][1] << 5;
                d->v[1][2] = e->v[1][2] << 5;
                d->v[2][0] = e->v[2][0] << 5;
                d->v[2][1] = e->v[2][1] << 5;
                d->v[2][2] = e->v[2][2] << 5;
                d->unk24 = e->unk12;
            }
            g = (MovingGroup *)e;
        } while (next != -1);
    }
    D_803F782C = d;
    D_80358070 = (u8 *)d;
}

/* ---- the collision triangles -------------------------------------------- */

/* LEVEL_TRI_BYTES: the level's triangles' owner (0x4F), id (0x50) and
   pushes (0x58), and the switches' pushes, are 0.  The original stores
   whatever $t9, $v0 and $s1 held (an address's low byte, an earlier
   triangle's dx), and nothing reads them back for these triangles: owner
   and id are only read for the objects' (56040), pushes for the
   buildings' pieces (77E20's func_802BF264). */

/* func_802A41B0: the triangle at `t` from the file's at `src` (nine
   big-endian s16 corners at any alignment, a u16 at 0x12): its corners
   << 3, the edges' cross product, -(n . v1), |n| and |n|^2, and the
   normal's largest component.  The bytes 0x4F..0x58 are the caller's
   (the original's $t9, $v0, $t2 (a halfword), $gp, $t7, $t6 and $s1).
   Returns `t` + 1. */
REGS(t4, t5, t2, t7, gp, t9, v0, t6, s1 -> t4)
u32 func_802A41B0(u32 t_, u32 src_, u32 h52, u32 b56, u32 b55, u32 b4f, u32 b50, u32 b57,
                  u32 b58) {
    CollisionTri *t = (CollisionTri *)t_;
    u8 *src = (u8 *)src_;
    s32 x0, y0, z0, x1, y1, z1, x2, y2, z2;
    s32 dy1, dz1, dx1, dy2, dz2, dx2;
    s64 nx, ny, nz, ax, ay, az, nzz1;
    f32 len2, f;

    t->unk59 = 0;
    t->unk55 = b55;
    t->unk56 = b56;
    t->group = h52;
    t->owner = b4f;
    t->id = b50;
    t->group2 = b57;
    t->pushes = b58;
    x0 = BE16S(src + 0x0) << 3;
    y0 = BE16S(src + 0x2) << 3;
    z0 = BE16S(src + 0x4) << 3;
    x1 = BE16S(src + 0x6) << 3;
    y1 = BE16S(src + 0x8) << 3;
    z1 = BE16S(src + 0xA) << 3;
    x2 = BE16S(src + 0xC) << 3;
    y2 = BE16S(src + 0xE) << 3;
    z2 = BE16S(src + 0x10) << 3;
    dy1 = y0 - y1;
    dz1 = z0 - z1;
    dy2 = y0 - y2;
    dz2 = z0 - z2;
    dx2 = x0 - x2;
    dx1 = x0 - x1;
    t->v[0][0] = x0; t->v[0][1] = y0; t->v[0][2] = z0;
    t->v[1][0] = x1; t->v[1][1] = y1; t->v[1][2] = z1;
    t->v[2][0] = x2; t->v[2][1] = y2; t->v[2][2] = z2;
    /* (the float sum in the original's order: nx², + ny², + nz²) */
    nx = (s64)dy1 * dz2 - (s64)dz1 * dy2;
    t->nx = nx;
    len2 = (f32)nx;
    len2 = len2 * len2;
    ny = (s64)dz1 * dx2 - (s64)dx1 * dz2;
    t->ny = ny;
    f = (f32)ny;
    f = f * f;
    len2 = len2 + f;
    nz = (s64)dx1 * dy2 - (s64)dy1 * dx2;
    t->nz = nz;
    f = (f32)nz;
    f = f * f;
    len2 = len2 + f;
    nzz1 = nz * z1;
    t->d = -(nx * x1 + ny * y1 + nzz1);
    t->nlen2 = len2;
    t->nlen = __builtin_sqrtf(len2);
    ax = nx;
    if (ax < 0) {
        ax = -ax;
    }
    ay = ny;
    if (ay < 0) {
        ay = -ay;
    }
    az = nz;
    if (az < 0) {
        az = -az;
    }
    if (az < ax || (az < ay)) {
        if (ay < ax || (ay < az)) {
            t->axis = 2;
        } else {
            t->axis = 1;
        }
    } else {
        t->axis = 0;
    }
    t->side = 0;
    t->heading = src[0x12] << 8 | src[0x13];
    return (u32)(t + 1);
}

/* func_802A3D54 / func_802A3DF8: a grid of triangle lists (LevelHeader's
   unk10 cells from `base`; each list a big-endian end offset from the
   section, then LevelCollisionTris) into CollisionTris on the heap, a
   pointer to each cell's in `table` and one past them.  The triangles'
   0x52 is the cells left, 0x57 the low byte of the list's end; 0x59 the
   file triangle's 0x15; 0x4F, 0x50 and 0x58 are 0 (LEVEL_TRI_BYTES).  (802A3D54's code: 802A3DF8 is the
   same.) */
static void tri_grid(LevelHeader *h, u8 *base, u32 *table) {
    s32 cells = (s16)h->unk10[0] * (s16)h->unk10[1];
    u8 *p = base, *end;
    CollisionTri *t = (CollisionTri *)D_80358070;

    do {
        *table++ = (u32)t;
        end = base + UNALIGNED_W(p);
        for (p += 4; p != end; p += sizeof(LevelCollisionTri)) {
            LevelCollisionTri *f = (LevelCollisionTri *)p;

            t = (CollisionTri *)func_802A41B0((u32)t, (u32)p, cells, f->unk14, 0, 0, 0, (u32)end, 0);
            t[-1].unk59 = f->unk15;
        }
    } while (--cells != 0);
    *table = (u32)t;
    D_80358070 = (u8 *)t;
}

REGS(t0)
void func_802A3D54(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;

    tri_grid(h, LEVEL_PTR(h, collisionXZ), (u32 *)D_803BDCA8);
}

REGS(t0)
void func_802A3DF8(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;

    tri_grid(h, LEVEL_PTR(h, playerCollisionXZ), (u32 *)D_803BDE40);
}

#define WALL_TRI_SIZE 0x14      /* the walls' file triangles: no 0x14, 0x15 */
#define STOP_TRI_SIZE 0x15      /* the train stops': an id at 0x14 */

/* func_802A3E9C: the walls (LevelHeader.unk64..trainStops) into
   D_803BD310 (collision.h's Wall): the kinds' count and the kinds, then
   the triangles' count and the triangles, a pointer to each in `tris`;
   and the record's first byte (WALL_SIDED) */
REGS(t0)
void func_802A3E9C(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *p = LEVEL_PTR(h, unk64);
    u8 *end = LEVEL_PTR(h, trainStops);
    Wall *w;
    CollisionTri *t = (CollisionTri *)D_80358070;

    D_803BD308 = t;
    for (w = D_803BD310; p != end; w++) {
        u32 n, k;

        X3((u8 *)&w->info + 1) = p[0];      /* WALL_SIDED */
        n = p[1];
        w->nkinds = n;
        p += 2;
        for (k = 0; k < n; k++) {
            w->kinds[k] = *p++;
        }
        n = *p++;
        X3((u8 *)&w->info) = n;             /* WALL_COUNT */
        for (k = 0; k < n; k++) {
            /* (the TAS has walls of more than 0x3C triangles: tris[0x3C]
               is info, which the store overwrites, as the original does;
               -fsanitize=array-bounds reports it) */
            w->tris[k] = t;
            /* 0x52 the section's end, 0x56 the next slot's address, 0x57
               the triangles left (what the original has in those
               registers); 0x4F, 0x50, 0x58 0 (LEVEL_TRI_BYTES) */
            t = (CollisionTri *)func_802A41B0((u32)t, (u32)p, (u32)end, (u32)&w->tris[k + 1], 1, 0, 0,
                                              n - 1 - k, 0);
            p += WALL_TRI_SIZE;
        }
    }
    D_803BDAF0 = w;
    D_803BD30C = t;
    D_80358070 = (u8 *)t;
}

/* whether a triangle of [D_803B9890, end) has the id `id`: 5CB60's
   802A4168 */
static s32 tri_id_used(u32 id, CollisionTri *end) {
    CollisionTri *t;

    for (t = D_803B9890; t != end; t++) {
        if (t->id == id) {
            return 1;
        }
    }
    return 0;
}

/* func_802A3F80: LevelHeader.trainStops..collisionXZ into D_803BC1D0's
   records (collision.h's TriSwitch): the owner, the kind and n; then n 12-byte areas (six big-endian
   s16, << 5) and a u32 after them, or (kind not 0) n s16; then n
   triangles (each id once, from D_803B9890 on); then n part bytes.  The
   triangles' 0x58 is the last area's third word, then what each leaves. */
/* The palette param the models' animated textures load with
   (func_802A2608, func_802A24BC): the original passes whatever $fp held,
   which is what func_802A3F80 left there, the end of the level's switch
   triangles (an address in D_803B9890).  Only the palette types (60F60's
   4 and 5) use it, as the original's colours: kept, as a value. */
static u32 tex_param;

REGS(t0)
void func_802A3F80(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *p = LEVEL_PTR(h, trainStops);
    u8 *end = LEVEL_PTR(h, collisionXZ);
    TriSwitch *r;
    CollisionTri *tris = D_803B9890;   /* $fp */
    u32 last = 0;

    for (r = D_803BC1D0; p != end; r++) {
        u32 n, kind;
        u8 *ids;
        CollisionTri *t;

        r->owner = p[0];
        kind = p[1];
        r->kind = kind;
        n = p[2];
        r->n = n;
        p += 3;
        if (kind == 0) {
            s32 *q = r->area;
            s32 c[6];

            do {
                c[0] = BE16S(p + 0) << 5;
                c[1] = BE16S(p + 2) << 5;
                c[2] = BE16S(p + 4) << 5;
                c[3] = BE16S(p + 6) << 5;
                c[4] = BE16S(p + 8) << 5;
                c[5] = BE16S(p + 10) << 5;
                q[0] = c[0];
                q[1] = c[1];
                q[2] = c[2];
                q[3] = c[3];
                q[4] = c[4];
                q[5] = c[5];
                p += 0xC;
                q += 6;
            } while (--n != 0);
            last = UNALIGNED_W(p);
            *q = last;
            p += 4;
        } else {
            u16 *q = (u16 *)r->area;   /* n halfwords, kept big-endian */

            do {
                last = (u32)(s32)BE16S(p);
                STORE_BE16(q, last);
                p += 2;
                q++;
            } while (--n != 0);
        }
        n = *p++;
        t = tris;
        ids = r->level;
        r->nlevel = n;
        for (; n != 0; n--) {
            u32 id;

            id = p[0x14];
            *ids++ = id;
            if (!tri_id_used(id, tris)) {
                t = (CollisionTri *)func_802A41B0((u32)t, (u32)p, (u32)end, last, 1, 0, id, n, 0);
            }
            p += STOP_TRI_SIZE;
        }
        tris = t;
        n = *p++;
        ids = r->own;
        r->nown = n;
        /* (the original tests n once less: its loop tests at the end) */
        for (; n != 0; n--) {
            *ids++ = *p++;
        }
    }
    D_803BD304 = r;
    D_803BD300 = tris;
    tex_param = (u32)tris;
}

/* ---- the buildings' parts and the textures ---------------------------- */

REGS(t6, fp -> s0)
u32 func_802A0CFC(u32 id, u32 param);
REGS(s0, s1)
void func_802A08E4(u32 dl, u32 end);
s32 func_802CE6F8(s32 x, s32 z, s32 y);

extern u8 *PTR32 D_803BE704, *PTR32 D_803BE708;    /* this level's building groups */
extern u8 D_802D30D0[], D_802D3194[], D_802D32A0[], D_802D331C[], D_802D33C8[], D_802D3444[],
    D_802D3538[], D_802D3614[], D_802D36C0[], D_802D3784[], D_802D3890[], D_802D393C[],
    D_802D3A00[], D_802D3A4C[], D_802D3BA0[], D_802D3BD4[], D_802D3CE0[], D_802D3DD4[],
    D_802D3EB0[], D_802D3F74[];
extern u8 D_8030631F[];
extern u32 D_80365330;

#define GROUP_SIZE 0x18         /* a building group's record (D_803BE708) */

/* func_802A1EC8: this level's table of building groups (D_803BE708, its
   cursor D_803BE704 four bytes on), or none */
REGS()
void func_802A1EC8(void) {
    s32 level = D_802E8BDC;
    u8 *t;

#define GROUPS(table) (t = (table))
    if (level == LEVEL_SIMIAN_ACRES) GROUPS(D_802D30D0);
    else if (level == LEVEL_ANGEL_CITY) GROUPS(D_802D3194);
    else if (level == LEVEL_OUTLAND_FARM) GROUPS(D_802D32A0);
    else if (level == LEVEL_BLACKRIDGE_WORKS) GROUPS(D_802D331C);
    else if (level == LEVEL_GLORY_CROSSING) GROUPS(D_802D33C8);
    else if (level == LEVEL_SHUTTLE_GULLY) GROUPS(D_802D3444);
    else if (level == LEVEL_CRYSTAL_RIFT) GROUPS(D_802D3538);
    else if (level == LEVEL_ARGENT_TOWERS) GROUPS(D_802D3614);
    else if (level == LEVEL_DIAMOND_SANDS) GROUPS(D_802D36C0);
    else if (level == LEVEL_EBONY_COAST) GROUPS(D_802D3784);
    else if (level == LEVEL_OYSTER_HARBOR) GROUPS(D_802D3890);
    else if (level == LEVEL_CARRICK_POINT) GROUPS(D_802D393C);
    else if (level == LEVEL_HAVOC_DISTRICT) GROUPS(D_802D3A00);
    else if (level == LEVEL_IRONSTONE_MINE) GROUPS(D_802D3A4C);
    else if (level == LEVEL_BEETON_TRACKS) GROUPS(D_802D3BA0);
    else if (level == LEVEL_OBSIDIAN_MILE) GROUPS(D_802D3BD4);
    else if (level == LEVEL_ECHO_MARCHES) GROUPS(D_802D3CE0);
    else if (level == LEVEL_TEMPEST_CITY) GROUPS(D_802D3DD4);
    else if (level == LEVEL_EMBER_HAMLET) GROUPS(D_802D3EB0);
    else if (level == LEVEL_CROMLECH_COURT) GROUPS(D_802D3F74);
    else {
        D_803BE704 = NULL;
        D_803BE708 = NULL;
        return;
    }
#undef GROUPS
    D_803BE708 = t;
    D_803BE704 = t + 4;
}

#define MODE_RED_VERTICES 0x800 /* the game mode (D_80364A98) in which
                                   func_802A20F4 colours models red */

/* func_802A20F4: in game mode 0x800, a model's vertex colours (from 0x50
   to Model.unk1C) set to red, if `flag` */
REGS(t4, s0)
void func_802A20F4(u32 m_, u32 flag) {
    u8 *m = (u8 *)m_;

    if (flag != 0 && (D_80364A98 == MODE_RED_VERTICES)) {
        Vtx *v = (Vtx *)(m + 0x50), *end = (Vtx *)MODEL_PTR(m, unk1C);

        for (; v != end; v++) {
            v->v.cn[0] = 0xFF;
            v->v.cn[1] = 0;
            v->v.cn[2] = 0;
        }
    }
}

/* func_802A2164: in level 0x31, model 0x24's unk4 is 2 */
REGS(t3, s0)
void func_802A2164(u32 n, u32 m_) {
    Model *m = (Model *)m_;

    if (D_802E8BDC == LEVEL_END_SEQUENCE && (n == 0x24)) {
        m->unk4 = 2;
    }
}

/* func_802A23E0: a building's model's effects (Model.unk30..unk38): byte
   0x31 through D_8030631F */
REGS(v1)
void func_802A23E0(u32 b_) {
    u8 *m = (u8 *)((Building *)b_)->model;
    ModelEffect *e = (ModelEffect *)MODEL_PTR(m, unk30);
    ModelEffect *end = (ModelEffect *)MODEL_PTR(m, unk38);

    for (; e != end; e++) {
        X3(&e->unk30[1]) = D_8030631F[X3(&e->unk30[1])];
    }
}

/* func_802A2458: a building's centre in x and z (unk28, unk2C), from its
   model's box (Model.unk1C: four corners) */
REGS(v1, t4)
void func_802A2458(u32 b_, u32 m_) {
    Building *b = (Building *)b_;
    s16 (*box)[3] = (s16 (*)[3])MODEL_PTR(m_, unk1C);

    b->unk28 = ((box[0][0] + box[1][0]) >> 1) << 5;
    b->unk2C = ((box[0][2] + box[2][2]) >> 1) << 5;
}

/* func_802A2608: a model's animated textures loaded (Model.unk28..unk2C),
   each number replaced by its physical address */
REGS(t4)
void func_802A2608(u32 m_) {
    u8 *m = (u8 *)m_;
    u8 *a = MODEL_PTR(m, unk28), *end = MODEL_PTR(m, unk2C);
    u32 fp = tex_param;

    while (a != end) {
        ModelAnimTex *at = (ModelAnimTex *)a;
        s32 n, k;

        at->frame0 = func_802A0CFC(at->frame0, fp);
        n = at->nframes - 1;
        for (k = 0; k < n; k++) {
            at->frames[k] = func_802A0CFC(at->frames[k], fp);
        }
        a = (u8 *)&at->frames[k];
    }
}


/* func_802A24BC: the three buildings 0xBA..0xBC get texture 0xF81
   (D_80365330) and a collision object (89250's func_802CE6F8) */
REGS(v1)
void func_802A24BC(u32 b_) {
    Building *b = (Building *)b_;

    if (b->unk30 != 0xBA && (b->unk30 != 0xBB) &&
        (b->unk30 != 0xBC)) {
        return;
    }
    D_80365330 = func_802A0CFC(0xF81, tex_param);
    b->unk44 = func_802CE6F8(b->x, b->z, b->y);
}

/* a big-endian s16 at any alignment plus d, stored back a byte at a time */
static void be16_add(u8 *p, s32 d) {
    u32 v = (u32)(BE16S(p) + d);

    p[0] = v >> 8;
    p[1] = v;
}

/* func_802A26A8: a model moved by (dx, dy, dz), world units: its heights
   (unk40..unk44), its rectangle (unk20), the 0x14-byte triangles
   (unk24..unk28), its box (unk1C), its vertices if it doesn't move
   (unkE, the behaviour, 0), its collision triangles (unk48..unk4C), its
   effects (unk30..unk38, made 16.16) and the points of unk2C..unk30 */
REGS(t4, t5, t6, t7)
void func_802A26A8(u32 m_, s32 dx, s32 dy, s32 dz) {
    u8 *m = (u8 *)m_;
    s32 i;

    {
        s16 *p = (s16 *)MODEL_PTR(m, unk40), *end = (s16 *)MODEL_PTR(m, unk44);

        for (; p != end; p++) {
            *p += dy;
        }
    }
    {
        s16 *r = (s16 *)MODEL_PTR(m, unk20);

        r[0] += dx;
        r[1] += dz;
        r[2] += dx;
        r[3] += dz;
    }
    {
        u8 *p = MODEL_PTR(m, unk24), *end = MODEL_PTR(m, unk28);

        for (; p != end; p += 0x14) {
            s16 *v = (s16 *)p;

            for (i = 0; i < 9; i += 3) {
                v[i + 0] += dx;
                v[i + 1] += dy;
                v[i + 2] += dz;
            }
        }
    }
    {
        s16 (*c)[3] = (s16 (*)[3])MODEL_PTR(m, unk1C);

        for (i = 0; i < 4; i++) {
            c[i][0] += dx;
            c[i][1] += dy;
            c[i][2] += dz;
        }
    }
    if (((Model *)m)->unkE == 0) {
        Vtx *v = (Vtx *)(m + 0x50), *end = (Vtx *)MODEL_PTR(m, unk1C);

        for (; v != end; v++) {
            v->v.ob[0] += dx;
            v->v.ob[1] += dy;
            v->v.ob[2] += dz;
        }
    }
    {
        BuildingTri *t = (BuildingTri *)MODEL_PTR(m, unk48), *end = (BuildingTri *)MODEL_PTR(m, unk4C);

        for (; t != end; t++) {
            for (i = 0; i < 18; i += 6) {
                be16_add(&t->v[i + 0], dx);
                be16_add(&t->v[i + 2], dy);
                be16_add(&t->v[i + 4], dz);
            }
        }
    }
    {
        ModelEffect *e = (ModelEffect *)MODEL_PTR(m, unk30), *end = (ModelEffect *)MODEL_PTR(m, unk34);

        for (; e != end; e++) {
            e->x = (e->x + dx) << 16;
            e->y = (e->y + dy) << 16;
            e->z = (e->z + dz) << 16;
            e->unk28 = (e->unk28 + dy) << 16;
        }
    }
    {
        ModelEffect *e = (ModelEffect *)MODEL_PTR(m, unk34), *end = (ModelEffect *)MODEL_PTR(m, unk38);

        for (; e != end; e++) {
            e->x = (e->x + dx) << 16;
            e->y = (e->y + dy) << 16;
            e->z = (e->z + dz) << 16;
            e->unk28 = (e->unk28 + dy) << 16;
        }
    }
    {
        s16 *p = (s16 *)MODEL_PTR(m, unk2C), *end = (s16 *)MODEL_PTR(m, unk30);

        for (; p != end; p += 4) {
            p[0] += dx;
            p[1] += dy;
            p[2] += dz;
        }
    }
}

/* func_802A1C20: the level's animated textures' frames loaded
   (LevelHeader.animTextures).  Leaves $s0 the last address and $s1
   0x80000000. */
REGS(t0)
void func_802A1C20(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *a = LEVEL_PTR(h, animTextures), *end = LEVEL_PTR(h, terrain);
    /* (the palette param: 0, the original's $fp in all but 4 of the TAS's
       105 calls; the others an earlier function's working) */
    u32 fp = 0;

    while (a != end) {
        LevelAnimTex *at = (LevelAnimTex *)a;
        s32 n, k;

        n = at->nframes - 1;
        for (k = 0; k < n; k++) {
            u32 phys;

            phys = func_802A0CFC(at->frames[k], fp);
            at->frames[k] = phys;
        }
        a = (u8 *)&at->frames[k];
    }
}

/* func_802A3008: the level's display lists' textures (802A08E4 over
   [displayLists[0], displayLists[3])); $s1 the end, the first collision
   triangles' byte 0x58 */
REGS(t0)
void func_802A3008(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;

    func_802A08E4((u32)LEVEL_PTR(h, displayLists[0]), (u32)LEVEL_PTR(h, displayLists[3]));
}

/* ---- the buildings ------------------------------------------------------ */

u32 func_802C4108(u32 src, u32 dst, u32 arg2, u32 *dst_end);
extern u32 *PTR32 D_803BE6F0;   /* the model table, on the heap */
extern OSMesgQueue D_803150A0;
extern OSIoMesg D_80370C58;
extern u8 D_006EC4C0[];         /* the model table's ROM address */
extern u8 D_8036EB93;
extern u8 D_803EFED0[], D_803F0900[], D_803F1BE0[];
extern s16 D_803F767C, D_803F767E, D_803F7680;

#define INIT_AREA 0x8021ED00    /* init's memory: where models are DMA'd before inflating */
#define GZIP_WINDOW 0x8004B400
#define MODEL_TABLE_SIZE 0x800

/* a model file of `size` bytes at ROM `rom`: DMA'd to init's memory, its
   two gzip members inflated onto the heap; returns the heap's new top
   (rounded up).  (802A2A98's code: 802A396C and 802A32CC are the
   same.) */
static u32 load_gz_model(u32 rom, u32 size) {
    u32 src, dst;

    osInvalDCache((void *)INIT_AREA, size);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, rom, (void *)INIT_AREA, size, &D_803150A0);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    src = func_802C4108(INIT_AREA, (u32)D_80358070, GZIP_WINDOW, &dst);
    src = func_802C4108(src, dst, GZIP_WINDOW, &dst);
    return func_802A44E4(dst);
}

/* func_802A2BB0: the model table onto the heap */
REGS()
void func_802A2BB0(void) {
    u8 *t = D_80358070;

    D_803BE6F0 = (u32 *)t;
    D_80358070 = t + MODEL_TABLE_SIZE;
    osInvalDCache(t, MODEL_TABLE_SIZE);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, (u32)D_006EC4C0, t, MODEL_TABLE_SIZE,
                 &D_803150A0);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
}

/* func_802A2A98: model `n` of the model table, loaded onto the heap.
   Returns it (the original's $s0). */
REGS(t3 -> s0)
u32 func_802A2A98(u32 n) {
    u32 *table = D_803BE6F0;
    u32 start = table[n], size = table[n + 1] - start;
    u32 top;
    u8 *m;

    top = load_gz_model((u32)D_006EC4C0 + start, size);
    m = D_80358070;
    D_80358070 = (u8 *)top;
    return (u32)m;
}


/* func_802A21AC: building `n` with model `m` at (x, y, z), its flag and
   unk34: the next Building, its collision triangles (the model's
   BuildingTris) on the heap */
REGS(t4, t3, t5, t6, t7, s0, t9)
void func_802A21AC(u32 m_, u32 n, u32 x, u32 y, u32 z, u32 flag, u32 unk34) {
    u8 *m = (u8 *)m_;
    Building *b;
    u32 k, ngroups, cell, a0;
    u16 *f48, *f68;
    BuildingTri *src, *end;
    CollisionTri *t;

    func_802A2608((u32)m);
    if (n == 0x38) {                    /* model 0x38: its place kept */
        D_803F767C = x;
        D_803F767E = y;
        D_803F7680 = z;
    }
    b = D_803F7654;
    D_803F7654 = b + 1;
    func_802A2458((u32)b, (u32)m);
    b->unk34 = unk34;
    b->unk38 = 0;
    b->unk3C = 0;
    b->unk40 = 0;
    b->unk30 = n;
    b->unkEB = flag;
    b->unkEA = 0;
    b->model = (struct Model *)m;
    b->unkC = ((Model *)m)->unk2 << 5;
    b->x = b->unk1C = x << 5;
    b->y = b->unk20 = y << 5;
    b->z = b->unk24 = z << 5;
    ngroups = ((Model *)m)->unk0;
    b->unkE9 = ngroups;
    /* per group: a byte from 0xEC, halfwords from 0x48 and 0x68 */
    for (k = 0; k < ngroups; k++) {
        (&b->unkEC)[k] = 0;
    }
    f48 = &b->unk48;
    for (k = 0; k < ngroups; k++) {
        f48[k] = 0;
    }
    f68 = (u16 *)&b->unk68;
    for (k = 0; k < ngroups; k++) {
        f68[k] = 0;
    }
    if (D_803BE704 != NULL && (flag != 0)) {
        *(u32 *)D_803BE704 = (u32)b;
        D_803BE704 += GROUP_SIZE;
    }
    /* the object grid's cell it is in */
    a0 = (u32)D_803BE70C >> 5;
    cell = z / ((u32)D_803BE710 >> 5) * D_803BE714 + x / a0;
    b->unkE8 = cell;
    func_802A24BC((u32)b);
    func_802A23E0((u32)b);
    src = (BuildingTri *)MODEL_PTR(m, unk48);
    end = (BuildingTri *)MODEL_PTR(m, unk4C);
    t = (CollisionTri *)D_80358070;
    b->unk4 = t;
    for (; src != end; src++) {
        if (src->group2 != 0) {
            t->active = 0;
        } else {
            t->active = 1;
        }
        /* (0x4F unk34, 0x50 the low byte of &D_803F7654: the original's
           $t9 and $v0) */
        t = (CollisionTri *)func_802A41B0((u32)t, (u32)src, src->group, src->unk14, src->unk18, unk34,
                                          (u32)&D_803F7654, src->group2, src->pushes);
    }
    b->unk8 = t;
    D_80358070 = (u8 *)t;
}

/* the vehicle modules' blocks that func_802A1D54 resets, by their size */
#define EFED0_SIZE 0xA30
#define F0900_SIZE 0x4B8
#define F1BE0_SIZE 0x478

/* func_802A1D54: the buildings (LevelHeader.buildings), after the
   destruction tables are reset and the model table is in */
REGS(t0)
void func_802A1D54(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    FileBuilding *p, *end;
    u8 *q;
    s32 i;

    D_8036EB93 = 0;
    func_802A1EC8();
    for (i = 0, q = D_803EFED0; i < 1; i++, q += EFED0_SIZE) {
        q[0xA2A] = 0;
        q[0xA2B] = 0;
    }
    for (i = 0, q = D_803F0900; i < 4; i++, q += F0900_SIZE) {
        *(s16 *)(q + 0x4B0) = 0;
        q[0x4B2] = 0;
    }
    for (i = 0, q = D_803F1BE0; i < 2; i++, q += F1BE0_SIZE) {
        q[0x470] = 0;
        q[0x472] = 0;
    }
    D_803F7654 = D_803F4030;
    func_802A2BB0();
    p = (FileBuilding *)LEVEL_PTR(h, buildings);
    end = (FileBuilding *)LEVEL_PTR(h, unk60);
    for (; p != end; p++) {
        u32 n, x, y, z;
        u8 *m;

        n = BE16U(p->model);
        m = (u8 *)func_802A2A98(n);
        func_802A2164(n, (u32)m);
        func_802A08E4((u32)MODEL_PTR(m, unk10), (u32)MODEL_PTR(m, unk14));
        ((Model *)m)->unkE = p->behavior;
        x = BE16U(p->x);
        y = BE16U(p->y);
        z = BE16U(p->z);
        func_802A26A8((u32)m, x, y, z);
        func_802A20F4((u32)m, p->unk8);
        ((Model *)m)->unk6 = p->unk9;
        D_8036EB93 += p->unk9;
        func_802A21AC((u32)m, n, x, y, z, p->unk8, p->speed);
    }
}

/* ---- the vehicles' records --------------------------------------------- */

#include "shared.h"

void func_80278BF0(void *, void *, void *);

#define ATTRACT_DEMO 1          /* D_80364AA8 in the attract modes */
#define ATTRACT_DEMO2 0x80

/* func_802A1388 (the vehicle modules'): the next Vehicle, from its model
   file `model`: the model's pointers, and its display lists copied to the
   heap with pointers into the copy; then its heap block (hd.c's
   func_80278BF0, through 802A1558), unless in the attract mode without
   `a1` */
REGS(a0, a1, v0, v1, s2)
void func_802A1388(s32 type, s32 a1, u8 *buf1, u8 *buf2, u8 *model) {
    Vehicle *v = D_803649D0;
    VehicleModel *vm = (VehicleModel *)model;
    u32 *src, *end, *dst;
    s32 delta;

    D_803649D0 = v + 1;
    v->unk70 = 0;
    v->unk4 = buf1;
    v->unk8 = buf2;
    v->type = type;
    v->unk0 = model + vm->unk14;
    v->unkC = model + vm->unk24[0];
    v->unk10 = model + vm->unk24[1];
    v->unk14 = model + vm->unk24[2];
    v->unk18 = model + vm->unk24[3];
    v->unk1C = model + vm->unk24[4];
    v->unk20 = model + vm->unk24[5];
    v->unk24 = model + vm->unk24[6];
    v->unk28 = model + vm->unk24[7];
    v->unk2C = model + vm->unk24[8];
    v->unk54 = (Gfx *)(model + vm->unk48);
    src = (u32 *)(model + vm->unk1C);
    end = (u32 *)(model + vm->unk20);
    dst = (u32 *)D_80358070;
    delta = (u8 *)dst - (u8 *)v->unkC;
    for (; src != end; src += 2, dst += 2) {
        dst[0] = src[0];
        dst[1] = src[1];
    }
    D_80358070 = (u8 *)dst;
    v->unk30 = (u8 *)v->unkC + delta;
    v->unk34 = (u8 *)v->unk10 + delta;
    v->unk38 = (u8 *)v->unk14 + delta;
    v->unk3C = (u8 *)v->unk18 + delta;
    v->unk40 = (u8 *)v->unk1C + delta;
    v->unk44 = (u8 *)v->unk20 + delta;
    v->unk48 = (u8 *)v->unk24 + delta;
    v->unk4C = (u8 *)v->unk28 + delta;
    v->unk50 = (u8 *)v->unk2C + delta;
    if (D_80364AA8 == ATTRACT_DEMO && (a1 == 0)) {
        return;
    }
    func_80278BF0(v->unkC, v->unk18, &v->unk58);
}

/* func_802A133C (the vehicle modules'): the Vehicle of `type` moves to
   (x, y, z), with the state's byte 0x9B */
REGS(v0, v1, a0, a1, gp)
void func_802A133C(s32 x, s32 y, s32 z, s32 type, VS *vs) {
    Vehicle *v;

    for (v = D_80364460; v->type != type; v++) {
    }
    v->x = x;
    v->y = y;
    v->z = z;
    v->unk70 = vs->unk9B;
}

/* ---- the vehicles' and the cargo's model files -------------------------- */

extern u8 D_0048FE90[], D_004903C0[], D_00490AC0[], D_00491E00[], D_004929D0[], D_00494390[],
    D_00496AD0[], D_00497AF0[], D_004989E0[], D_00499690[], D_0049AD20[], D_0049B630[],
    D_0049BCE0[], D_0049C480[], D_0049E8E0[], D_0049F7A0[], D_0049FF70[], D_004A0720[],
    D_004A1000[], D_004A1690[], D_004A4120[], D_004A5660[];
REGS(s0, s1, s2)
void func_8029DF78(u32 dl, u32 end, u32 type);

/* the model file's ROM range */
#define ROM_RANGE(s, e) (start = (u32)(s), end = (u32)(e))

/* func_802A396C: vehicle model `type` (or the carrier's, the chopper's,
   the shuttle's, the comm point's or the scientist's): returns it ($s2),
   its display list's textures put in (8029DF78 and 802A08E4) */
REGS(t3 -> s2)
u32 func_802A396C(u32 type) {
    u32 start, end, top;
    u8 *m;

    if (type == VEHICLE_DRIVER) ROM_RANGE(D_00491E00, D_004929D0);
    else if (type == VEHICLE_SIDESWIPE) ROM_RANGE(D_004929D0, D_00494390);
    else if (type == VEHICLE_MAGOO) ROM_RANGE(D_00494390, D_00496AD0);
    else if (type == VEHICLE_MINIMAGOO) ROM_RANGE(D_004A1690, D_004A4120);
    else if (type == VEHICLE_BUGGY) ROM_RANGE(D_00490AC0, D_00491E00);
    else if (type == VEHICLE_BULLDOZER) ROM_RANGE(D_00496AD0, D_00497AF0);
    else if (type == VEHICLE_TRUCK) ROM_RANGE(D_00497AF0, D_004989E0);
    else if (type == VEHICLE_CRANE) ROM_RANGE(D_0049AD20, D_0049B630);
    else if (type == VEHICLE_TRAIN) ROM_RANGE(D_0049B630, D_0049BCE0);
    else if (type == VEHICLE_HOTROD) ROM_RANGE(D_0049BCE0, D_0049C480);
    else if (type == VEHICLE_JETPACK) ROM_RANGE(D_0049C480, D_0049E8E0);
    else if (type == VEHICLE_BIKE) ROM_RANGE(D_0049E8E0, D_0049F7A0);
    else if (type == VEHICLE_BARGE) ROM_RANGE(D_0049F7A0, D_0049FF70);
    else if (type == VEHICLE_BARGE_2) ROM_RANGE(D_0049F7A0, D_0049FF70);
    else if (type == VEHICLE_BARGE_3) ROM_RANGE(D_0049F7A0, D_0049FF70);
    else if (type == VEHICLE_POLICE) ROM_RANGE(D_0049FF70, D_004A0720);
    else if (type == VEHICLE_ATEAM) ROM_RANGE(D_004A0720, D_004A1000);
    else if (type == VEHICLE_STARSKI) ROM_RANGE(D_004A1000, D_004A1690);
    else if (type == VEHICLE_CHOPPER) ROM_RANGE(D_004989E0, D_00499690);
    else if (type == VEHICLE_CMO) ROM_RANGE(D_00499690, D_0049AD20);
    else if (type == VEHICLE_SHUTTLE) ROM_RANGE(D_004A4120, D_004A5660);
    else if (type == VEHICLE_COMMPOINT) ROM_RANGE(D_004903C0, D_00490AC0);
    else if (type == VEHICLE_SCIENTIST) ROM_RANGE(D_0048FE90, D_004903C0);
    else {
        *(volatile u32 *)0 = 0;     /* (the original's syscall: an unknown type) */
    }
    top = load_gz_model(start, end - start);
    m = D_80358070;
    D_80358070 = (u8 *)top;
    func_8029DF78((u32)VMODEL_PTR(m, unk1C), (u32)VMODEL_PTR(m, unk20), type);
    func_802A08E4((u32)VMODEL_PTR(m, unk1C), (u32)VMODEL_PTR(m, unk20));
    return (u32)m;
}

/* func_802A32CC: the carrier's cargo model `type` (the vehicles 3, 4, 5, 8,
   9, 10, 13, 14, 15): returns it ($s2), its display list's textures put in */
REGS(t3 -> s2)
u32 func_802A32CC(u32 type) {
    u32 start, end, top;
    u8 *m;

    if (type == VEHICLE_BUGGY) ROM_RANGE(D_00490AC0, D_00491E00);
    else if (type == VEHICLE_BULLDOZER) ROM_RANGE(D_00496AD0, D_00497AF0);
    else if (type == VEHICLE_TRUCK) ROM_RANGE(D_00497AF0, D_004989E0);
    else if (type == VEHICLE_HOTROD) ROM_RANGE(D_0049BCE0, D_0049C480);
    else if (type == VEHICLE_JETPACK) ROM_RANGE(D_0049C480, D_0049E8E0);
    else if (type == VEHICLE_BIKE) ROM_RANGE(D_0049E8E0, D_0049F7A0);
    else if (type == VEHICLE_POLICE) ROM_RANGE(D_0049FF70, D_004A0720);
    else if (type == VEHICLE_ATEAM) ROM_RANGE(D_004A0720, D_004A1000);
    else if (type == VEHICLE_STARSKI) ROM_RANGE(D_004A1000, D_004A1690);
    else {
        *(volatile u32 *)0 = 0;     /* (the original's syscall) */
    }
    top = load_gz_model(start, end - start);
    m = D_80358070;
    D_80358070 = (u8 *)top;
    func_802A08E4((u32)VMODEL_PTR(m, unk1C), (u32)VMODEL_PTR(m, unk20));
    return (u32)m;
}

#undef ROM_RANGE

/* func_802A3824: the vehicle the player starts in (D_803BE73A): the one
   standing where the driver's record (the first of type 0) is, or the
   chosen one (D_803643D4) outside the attract modes */
extern u8 D_803643D4;
extern u8 D_803BE73A;

REGS(t0)
void func_802A3824(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    FileVehicle *r = (FileVehicle *)LEVEL_PTR(h, vehicles), *end;
    s32 x, y, z;

    while (r->type != VEHICLE_DRIVER) {
        r++;
    }
    x = BE16S(r->x);
    y = BE16S(r->y);
    z = BE16S(r->z);
    r = (FileVehicle *)LEVEL_PTR(h, vehicles);
    end = (FileVehicle *)LEVEL_PTR(h, carrier);
    for (; r != end; r++) {
        u32 type;

        type = r->type;
        if (type != VEHICLE_DRIVER && (BE16S(r->x) == x) &&
            (BE16S(r->y) == y) && (BE16S(r->z) == z)) {
            if (D_80364AA8 != ATTRACT_DEMO && (D_80364AA8 != ATTRACT_DEMO2)) {
                type = D_803643D4;
            }
            D_803BE73A = type;
        }
    }
}

/* func_802A1320 (the scheduler's): the COP0 Status register */
u32 func_802A1320(void) {
    return engine_mfc0(12);
}

/* ---- the loader and its vehicle parts ------------------------------------ */

/* the vehicles' setups (the vehicle modules), with the model in $s2, the
   position in $t7, $s3, $s0 and the heading in $s1 */
#define SETUP(f) REGS(s2, t7, s3, s0, s1) void f(u8 *model, s32 x, s32 y, s32 z, s32 heading)
SETUP(func_802AE370);
SETUP(func_802AFC60);
SETUP(func_802B0DA0);
SETUP(func_802B29C0);
SETUP(func_802B4100);
SETUP(func_802B5900);
SETUP(func_802BAD80);
SETUP(func_802BBA60);
SETUP(func_802B7340);
SETUP(func_802C5120);
SETUP(func_802C9B90);
SETUP(func_802CB720);
SETUP(func_802CC920);
SETUP(func_802CF6A0);
SETUP(func_802D07E0);
REGS(t3, s2, t7, s3, s0, s1)
void func_802C80D0(s32 type, u8 *model, s32 x, s32 y, s32 z, s32 heading);
REGS(s2, t4, t5, t6, t7, s1)
void func_802B9C50(u8 *model, s32 x, s32 z, s32 heading, s32 dist, s32 speed);
REGS(s2)
void func_802B8480(u8 *model);
REGS(s2)
void func_802D2570(u8 *model);
REGS(t0)
void func_802CEAA0(u8 *level);
REGS()
void func_802A0700(void);
void func_8029DEA0(void);
void func_8029DC80(void);
REGS()
void func_802A4510(void);
REGS()
void func_802A5F30(void);
REGS()
void func_802BC840(void);
REGS()
void func_802C049C(void);
void func_802C4BF0(u8 *buf);
void func_8028FDA0(s16 *a, s16 *b);
void func_8026FBB0(void *a, void *b);
void func_8028D4C0(void *a, void *b);
void func_8028C190(void *a, void *b);
s32 func_80268EE8(s32 level);
void func_80295AE0(Gfx *gfx, Gfx *end);
void func_802A303C(u32 h_);
void func_802A30DC(void);
void func_802A3134(u32 h_);
void func_802A3198(void);
void func_802A19F4(void);
void func_802A350C(u32 h_);

extern u8 D_8039CA61, D_8039CA7E, D_8039CAB7;  /* the level's extras: a carrier model's, its number; another */
extern s16 D_8039CAB0, D_8039CAB2, D_8039CAB4;  /* ... the other's offset */
extern u8 *PTR32 D_8039CAC0, *PTR32 D_8039CABC;
extern u8 D_8036698C;
extern u8 D_803ED40F;
extern u8 D_80364AC1, D_803643DB, D_803643DC;


/* hd.c's: the level at h loaded into the run-time tables, its vehicles set
   up, and (status non-zero, not the attract modes) its status from the
   Controller Pak's (func_802C4BF0) */
void func_802A1674(LevelHeader *hp, s32 status) {
    u32 h = (u32)hp;
    u64 mode;

    D_803BE6F4 = status;
    func_802A2D68(h);
    func_802A0700();
    func_802A3008(h);
    func_802A1C20(h);
    func_802A1C88(h);
    func_8029DEA0();
    func_802A3D54(h);
    func_802A3DF8(h);
    func_802A3E9C(h);
    func_802A3F80(h);
    func_802A4464(h);
    func_802A1A9C(h);
    func_802A1934();
    func_8029DC80();
    func_802A4510();
    func_802A2C54(h);
    func_802A5F30();
    func_802BC840();
    func_802A1D54(h);
    func_802C049C();
    func_8028FDA0((s16 *)LEVEL_PTR(hp, blocks), (s16 *)LEVEL_PTR(hp, bounds40));
    if (D_80364A98 != 0x80) {
        func_802A350C(h);
    }
    D_80364AC1 = 0;
    D_803643DB = 0;
    D_803643DC = 0;
    /* the carrier, the shuttle and the chopper */
    mode = D_80364A98;
    if (mode == 2) {
        if (D_802E8BEC == 0) {
            func_802A303C(h);
        }
    } else {
        if (mode != 0x80 && (D_803BE6F4 == 0)) {
            func_802A303C(h);
            func_802A30DC();
        }
        func_802A3134(h);
    }
    func_802A3198();
    func_802CEAA0((u8 *)hp);
    func_802A19F4();
    D_803BE6FC = (struct LevelUnk58 *)LEVEL_PTR(hp, unk58);
    D_803BE700 = (struct LevelUnk58 *)LEVEL_PTR(hp, buildings);
    func_8026FBB0(LEVEL_PTR(hp, rdus), LEVEL_PTR(hp, tntCrates));
    func_8028D4C0(LEVEL_PTR(hp, tntCrates), LEVEL_PTR(hp, blocks));
    func_8028C190(LEVEL_PTR(hp, ammoBoxes), LEVEL_PTR(hp, collisionFixes));
    D_80370C50 = 0;
    mode = D_80364A98;
    if (mode != 2 && (mode != 0x100000000000ULL) &&
        (D_803BE6F4 != 0)) {
        func_802C4BF0((u8 *)(__UINTPTR_TYPE__)(u32)D_803BE6F4);
    }
}

/* the extra model the level has (D_8039CAB7): the scientist, its vertices
   moved by D_8039CAB0..B4; its parts into D_8039CAC0 and D_8039CABC */
REGS()
void func_802A19F4(void) {
    if (D_8039CAB7 != 0) {
        u8 *m;
        Vtx *v, *end;

        m = (u8 *)(__UINTPTR_TYPE__)func_802A396C(VEHICLE_SCIENTIST);
        v = (Vtx *)VMODEL_PTR(m, unk14);
        D_8039CAC0 = (u8 *)v;
        end = (Vtx *)VMODEL_PTR(m, unk18);
        for (; v != end; v++) {
            v->v.ob[0] += D_8039CAB0;
            v->v.ob[1] += D_8039CAB2;
            v->v.ob[2] += D_8039CAB4;
        }
        D_8039CABC = VMODEL_PTR(m, unk24[0]);
    }
}

/* the missile carrier (LevelHeader.carrier) */
REGS(t0)
void func_802A303C(u32 h_) {
    FileCarrier *r = (FileCarrier *)LEVEL_PTR((LevelHeader *)h_, carrier);
    s32 speed = r->speed;

    if (speed != 0) {
        u32 m;

        m = func_802A396C(VEHICLE_CMO);
        func_802B9C50((u8 *)(__UINTPTR_TYPE__)m, BE16S(r->x) << 5, BE16S(r->z) << 5, BE16S(r->heading),
                      BE16S(r->distance) << 5, speed);
        D_803643DB = 1;
    }
}

/* the shuttle (8DDB0), on the levels func_80268EE8 says */
REGS()
void func_802A30DC(void) {
    s32 has;

    has = func_80268EE8(D_802E8BDC);
    if (has != 0) {
        u32 m;

        D_80364AC1 = 1;
        m = func_802A396C(VEHICLE_SHUTTLE);
        func_802D2570((u8 *)(__UINTPTR_TYPE__)m);
    }
}

/* the BCT chopper, where there is a carrier and outside the attract
   modes */
REGS(t0)
void func_802A3134(u32 h_) {
    FileCarrier *r = (FileCarrier *)LEVEL_PTR((LevelHeader *)h_, carrier);

    if (r->speed != 0 && (D_80364AA8 != ATTRACT_DEMO2)) {
        u32 m;

        m = func_802A396C(VEHICLE_CHOPPER);
        func_802B8480((u8 *)(__UINTPTR_TYPE__)m);
        D_803643DC = 1;
    }
}

/* the missile carrier's cargo model (D_8039CA61, model D_8039CA7E): its
   segment 7 block (VehicleModel.unk18: its size, then that many bytes of
   words, then identity matrices to the size), and its display list
   (func_80295AE0 over [unk24[0], unk24[1])) */
REGS()
void func_802A3198(void) {
    if (D_8039CA61 != 0) {
        u8 *m, *heap;
        s32 n, *src, *w, *wend;

        m = (u8 *)(__UINTPTR_TYPE__)func_802A32CC(D_8039CA7E);
        D_803BDAFC = m;
        D_803BDB04 = VMODEL_PTR(m, unk14);
        D_803BDB08 = VMODEL_PTR(m, unk24[0]);
        heap = D_80358070;
        D_803BDB00 = heap;
        src = (s32 *)VMODEL_PTR(m, unk18);
        n = src[0];
        wend = (s32 *)((u8 *)(src + 2) + src[1]);
        for (w = src + 2; w != wend; w++) {
            *(s32 *)heap = *w;
            heap += 4;
            n -= 4;
        }
        for (; n != 0; n -= MTX_SIZE) {
            mtx_identity(heap);
            heap += MTX_SIZE;
        }
        D_80358070 = heap;
        func_80295AE0((Gfx *)VMODEL_PTR(m, unk24[0]), (Gfx *)VMODEL_PTR(m, unk24[1]));
    }
}

/* the level's vehicles (LevelHeader.vehicles): each one's model loaded and
   its setup run; the Sideswipe's record is the vehicle the player chose
   outside the attract modes */
REGS(t0)
void func_802A350C(u32 h_) {
    FileVehicle *r, *end;
    u8 *m;
    u32 type;
    s32 x, y, z, heading;

    func_802A3824(h_);
    D_803ED3F5 = 0;
    D_803ED40F = 0;
    r = (FileVehicle *)LEVEL_PTR((LevelHeader *)h_, vehicles);
    end = (FileVehicle *)LEVEL_PTR((LevelHeader *)h_, carrier);
    for (; r != end; r++) {
        type = r->type;
        if (type == VEHICLE_SIDESWIPE && (D_80364AA8 != ATTRACT_DEMO) &&
            (D_80364AA8 != ATTRACT_DEMO2)) {
            type = D_803643D4;
        }
        /* the crane, the train and the barges */
        if (type == VEHICLE_CRANE || (type == VEHICLE_TRAIN) ||
            (type == VEHICLE_BARGE) ||
            (type == VEHICLE_BARGE_2) ||
            (type == VEHICLE_BARGE_3)) {
            D_803ED40F = 1;
        }
        x = BE16S(r->x) << 5;
        y = BE16S(r->y) << 5;
        z = BE16S(r->z) << 5;
        heading = BE16S(r->heading);
        m = (u8 *)(__UINTPTR_TYPE__)func_802A396C(type);

        /* the type's setup */
#define RUN_SETUP(f) f(m, x, y, z, heading)
        if (type == VEHICLE_DRIVER) RUN_SETUP(func_802AE370);
        else if (type == VEHICLE_SIDESWIPE) RUN_SETUP(func_802AFC60);
        else if (type == VEHICLE_MAGOO) RUN_SETUP(func_802B0DA0);
        else if (type == VEHICLE_BUGGY) RUN_SETUP(func_802B29C0);
        else if (type == VEHICLE_BULLDOZER) RUN_SETUP(func_802B4100);
        else if (type == VEHICLE_TRUCK) RUN_SETUP(func_802B5900);
        else if (type == VEHICLE_CRANE) RUN_SETUP(func_802BAD80);
        else if (type == VEHICLE_TRAIN) RUN_SETUP(func_802BBA60);
        else if (type == VEHICLE_HOTROD) RUN_SETUP(func_802B7340);
        else if (type == VEHICLE_JETPACK) RUN_SETUP(func_802C5120);
        else if (type == VEHICLE_BIKE) RUN_SETUP(func_802C9B90);
        else if ((type == VEHICLE_BARGE) ||
                 (type == VEHICLE_BARGE_2) ||
                 (type == VEHICLE_BARGE_3)) {
            func_802C80D0(type, m, x, y, z, heading);
        }
        else if (type == VEHICLE_POLICE) RUN_SETUP(func_802CB720);
        else if (type == VEHICLE_ATEAM) RUN_SETUP(func_802CC920);
        else if (type == VEHICLE_STARSKI) RUN_SETUP(func_802CF6A0);
        else if (type == VEHICLE_MINIMAGOO) RUN_SETUP(func_802D07E0);
        else {
            engine_trap(0x802A380C);
        }
#undef RUN_SETUP
    }
}
