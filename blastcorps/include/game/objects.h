#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include "game/types.h"
#include "game/audio.h"

/*
 * The objects a level file places, as the level file has them (ROM data,
 * read in place from the loaded file: see LevelHeader in level.h) and as
 * the game keeps them while the level runs.  The level-file layouts are the
 * ones tools/assetlib/level.py writes as records (docs/ASSETS.md, docs/
 * blast_corps_levels.txt); the run-time records are the loaders' (each
 * named after the function that fills it).
 *
 * Positions: the level file has s16 world units; the run-time records keep
 * s32 world units << 5, as the player's D_803643E0..E8 do.
 *
 * Pointer fields in the run-time records are plain C pointers: they are set
 * by the game (heap allocations, texture cache entries) and never come from
 * ROM.  The level-file records hold no pointers.
 */

/* ---- ammo boxes: the Ballista's missiles and the Sideswipe's hydraulics */

/* LevelHeader.ammoBoxes, loaded by func_8028C190 (hd_code 479D0.c). */
typedef struct LevelAmmoBox {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 type;          /* 0 Ballista missiles, 1 Sideswipe hydraulics (AmmoBoxInfo index) */
} LevelAmmoBox;
SIZE_CHECK(LevelAmmoBox, 8);

/* Per ammo box type, D_802FDB40[2] (479D0.c's .data). */
typedef struct AmmoBoxInfo {
    /* 0x00 */ s16 unk0;         /* unk0..unkA: the box's corners, added to its position (func_8028C41C) */
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;         /* texture indices (func_802A0CC8) */
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;        /* passed to func_802CE880 */
    /* 0x12 */ s16 unk12;        /* pick-up distance */
    /* 0x14 */ u8 unk14;         /* the vehicle type that collects it (func_8028C874) */
} AmmoBoxInfo;
SIZE_CHECK(AmmoBoxInfo, 0x16);

/* The level's ammo boxes, D_8039AF00[AMMO_BOX_MAX] (479D0.c's .bss);
 * D_8039B068 counts them. */
typedef struct AmmoBox {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
    /* 0x06 */ u8 type;
    /* 0x07 */ u8 collected;     /* set when picked up (func_8028C874) */
    /* 0x08 */ s16 alpha;        /* 0xFF, fades by 20 a frame once collected (the prim colour's alpha) */
    /* 0x0C */ u8 *PTR32 unkC;   /* textures from func_802A0CC8, loaded with OS_K0_TO_PHYSICAL */
    /* 0x10 */ u8 *PTR32 unk10;
    /* 0x14 */ Vtx *PTR32 vtx;   /* 8 vertices, 0x80 bytes from the heap (D_80358070) */
} AmmoBox;
SIZE_CHECK(AmmoBox, 0x18);

#define AMMO_BOX_MAX 15

extern AmmoBox D_8039AF00[AMMO_BOX_MAX];
extern s32 D_8039B068;

/* ---- TNT crates */

/* LevelHeader.tntCrates, loaded by func_8028D4C0 (hd_code 48D00.c). */
typedef struct LevelTntCrate {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ u8 type;           /* TntCrateInfo index (docs: B6) */
    /* 0x7 */ u8 timer;          /* seconds: TntCrate.timer is this * 60 */
    /* 0x8 */ s16 unk8;          /* unk8 and unkA go to func_802CDAE8 when it blows up */
    /* 0xA */ s16 unkA;
} LevelTntCrate;
SIZE_CHECK(LevelTntCrate, 0xC);

/* Per crate type, D_802FDB98[2] (48D00.c's .data). */
typedef struct TntCrateInfo {
    /* 0x00 */ s16 unk0;         /* unk0..unkA: the crate's corners (func_8028DA5C) */
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;         /* unkC..unk12: texture indices (func_802A0CC8) */
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;        /* collision size (func_802CDF94, func_802CE880) */
    /* 0x16 */ s16 unk16;
} TntCrateInfo;
SIZE_CHECK(TntCrateInfo, 0x18);

/* The level's TNT crates, D_8039B070[TNT_CRATE_MAX] (48D00.c's .bss);
 * D_8039B610 counts them. */
typedef struct TntCrate {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s16 unkC;         /* heading while pushed (func_802CE3B8, func_802A6F6C) */
    /* 0x0E */ u8 type;
    /* 0x10 */ s16 timer;        /* frames left once lit */
    /* 0x12 */ s16 timerStart;
    /* 0x14 */ s32 unk14;        /* nonzero once lit: the frame count D_80358060 then */
    /* 0x18 */ u8 active;        /* cleared when it blows up (func_8028DD64) */
    /* 0x19 */ u8 unk19;         /* frames until the blast (5) */
    /* 0x1A */ s16 unk1A;        /* LevelTntCrate.unk8 */
    /* 0x1C */ s16 unk1C;        /* LevelTntCrate.unkA */
    /* 0x1E */ s16 unk1E;        /* push distance */
    /* 0x20 */ s16 unk20;        /* once lit: counts 0..0xFF and back, faster as the timer runs out */
    /* 0x22 */ u8 unk22;
    /* 0x23 */ u8 unk23;         /* D_803F932C after a move */
    /* 0x24 */ u8 unk24;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ u8 *PTR32 tex[4]; /* textures from func_802A0CC8 (TntCrateInfo.unkC..unk12) */
    /* 0x3C */ Vtx *PTR32 vtx;   /* 8 vertices, 0x80 bytes from the heap */
    /* 0x40 */ SndState *PTR32 unk40;  /* sound 0x73, started when it is lit (func_80260650's handle) */
    /* 0x44 */ SndState *PTR32 unk44;  /* sound 7, while it is pushed */
} TntCrate;
SIZE_CHECK(TntCrate, 0x48);

#define TNT_CRATE_MAX 20

extern TntCrate D_8039B070[TNT_CRATE_MAX];
extern s32 D_8039B610;

/* ---- square blocks and the holes they are pushed into */

/*
 * LevelHeader.blocks, loaded by func_8028FDA0 (hd_code 4B5E0.c): a u16
 * count and that many LevelBlocks, then a u16 count of holes, each a
 * LevelHole followed by its triangles.
 */
typedef struct LevelBlock {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 type;          /* BlockInfo index */
} LevelBlock;
SIZE_CHECK(LevelBlock, 8);

typedef struct LevelHoleTri {
    /* 0x00 */ s16 v[3][3];      /* a triangle, s16 x, y, z each */
    /* 0x12 */ u8 unk12[4];
} LevelHoleTri;
SIZE_CHECK(LevelHoleTri, 0x16);

typedef struct LevelHole {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
    /* 0x06 */ u8 type;          /* the block type that fits (docs: T1) */
    /* 0x07 */ u8 numTris;       /* triangles after the record (docs: T2 = 8 is a hole) */
    /* 0x08 */ s16 unk8;         /* nonzero: the hole also gets an UnkStruct_8039C800 */
    /* 0x0A */ LevelHoleTri tris[1];
} LevelHole;

/* Per block type, D_802FDC08[3] (4B5E0.c's .data): its display list and
 * vertices, then its parameters. */
typedef struct BlockInfo {
    /* 0x000 */ Gfx dl[0x28];
    /* 0x140 */ Vtx vtx[0x14];
    /* 0x280 */ s16 unk280;      /* texture index (func_802A0CC8) */
    /* 0x282 */ u8 unk282;
    /* 0x283 */ u8 unk283;
    /* 0x284 */ s16 unk284;      /* collision size */
    /* 0x286 */ s16 unk286;      /* how close to a hole it drops in */
    /* 0x288 */ s32 unk288;      /* the hole's depth */
    /* 0x28C */ s32 unk28C;
} BlockInfo;
SIZE_CHECK(BlockInfo, 0x290);

/* The level's blocks, D_8039C550[BLOCK_MAX]; D_8039C710 counts them. */
typedef struct Block {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s16 unkC;         /* push distance */
    /* 0x0E */ s16 unkE;         /* heading while pushed */
    /* 0x10 */ u8 type;
    /* 0x11 */ u8 unk11;         /* 1 once it is in its hole */
    /* 0x12 */ u8 unk12;         /* 1 while it drops into a hole */
    /* 0x13 */ u8 unk13;         /* that hole's index */
    /* 0x14 */ s32 unk14;        /* the drop: y = unk1C + unk14 * t - 4t^2, t = unk18 */
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u8 unk20;
    /* 0x24 */ s32 unk24;        /* distance to the hole */
    /* 0x28 */ u8 unk28;
    /* 0x29 */ u8 unk29;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ s16 unk2C;
    /* 0x2E */ s16 unk2E;
    /* 0x30 */ void *PTR32 unk30;      /* texture (func_802A0CC8) */
    /* 0x34 */ SndState *PTR32 unk34;  /* sound 7, while it is pushed */
} Block;
SIZE_CHECK(Block, 0x38);

/* The level's holes, D_8039C718[HOLE_MAX]; D_8039C7F8 counts them. */
typedef struct Hole {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 unkC;         /* y less the block type's depth */
    /* 0x10 */ u8 type;
    /* 0x11 */ u8 filled;
    /* 0x14 */ s32 unk14;        /* D_803FB8B0 before and after its triangles are added (func_802CE9C8) */
    /* 0x18 */ s32 unk18;
} Hole;
/* 8A080's: the end of the holes' collision triangles (the next free one) */
extern struct CollisionTri *D_803FB8B0;
SIZE_CHECK(Hole, 0x1C);

/* The holes with LevelHole.unk8 set, D_8039C800[HOLE_MAX]; D_8039C940
 * counts them. */
typedef struct UnkStruct_8039C800 {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s16 corners[4][3]; /* the hole's position >> 5, +-40 in x and z */
    /* 0x24 */ s16 unk24;
    /* 0x26 */ u8 unk26;         /* 1 once its block is in (func_80291724) */
    /* 0x27 */ u8 hole;          /* the Hole's index */
} UnkStruct_8039C800;
SIZE_CHECK(UnkStruct_8039C800, 0x28);

#define BLOCK_MAX 8
#define HOLE_MAX 8

/* ---- the level file's other records (read by the handwritten loaders) */

/*
 * LevelHeader.vehicles: 9-byte records, read a byte at a time
 * (func_802A350C), so they have no alignment; this is their layout, not a
 * struct the code declares.  heading: 0 is +z (north), 0x400 +x (west).
 */
#define LEVEL_VEHICLE_SIZE 9
/* u8 type (VehicleType); s16 x, y, z; u16 heading */

/*
 * LevelHeader.carrier: one 10-byte record, read a byte at a time
 * (func_802A303C): u8 speed; s16 x, z; u16 heading; u16 distance (to the
 * end of the level); u8 unk9.
 */
#define LEVEL_CARRIER_SIZE 10

/* LevelHeader.buildings, read by func_802A1D54 (14 bytes). */
typedef struct LevelBuilding {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ u16 type;          /* the model (model_table index) */
    /* 0x8 */ u8 counts;         /* 1: counts towards the destroyed buildings (D_8036EB93) */
    /* 0x9 */ u8 unk9;
    /* 0xA */ u16 behavior;      /* 1 vertical, 2 circle, 3 horizontal, 4 follow the player, 5 rotate 90 */
    /* 0xC */ u16 speed;
} LevelBuilding;
SIZE_CHECK(LevelBuilding, 0xE);

/*
 * The level's buildings (and the other destructible objects the level file
 * lists as buildings): D_803F4030[] (hd_code 77E20's .bss), up to
 * D_803F7654.  func_802A1D54 fills one per LevelBuilding: it loads the
 * record's model (func_802A2A98) and func_802A21AC stores it here along
 * with two heap blocks.  hd_code 77E20 runs them; 13A70.c draws their
 * shadows.
 */
typedef struct Building {
    /* 0x00 */ struct Model *PTR32 model;  /* game/model.h */
    /* 0x04 */ void *PTR32 unk4; /* its pieces (CollisionTri) on the heap, to unk8 (func_802A21AC) */
    /* 0x08 */ void *PTR32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 x;            /* world position << 5 (13A70.c draws the shadow at x, unk44, z) */
    /* 0x14 */ s32 y;
    /* 0x18 */ s32 z;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;        /* the model number (13A70.c treats 0xBA..0xBC apart) */
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;        /* unk38..unk44: a moving building's state (4EBE0.c) */
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;        /* the ground under it, << 5; 77E20 also reads its low half */
    /* per damage group (0-based): falling (nonzero), the frames it has
       fallen, and its spin rates (12-bit angles a frame: x signed, y and z
       read unsigned) */
    /* 0x48 */ u16 falling[16];
    /* 0x68 */ u16 fall_t[16];
    /* 0x88 */ s16 spin_x[16];
    /* 0xA8 */ u16 spin_y[16];
    /* 0xC8 */ u16 spin_z[16];
    /* 0xE8 */ u8 unkE8;         /* the grid cell it is drawn in */
    /* 0xE9 */ u8 unkE9;         /* its damage groups */
    /* 0xEA */ u8 unkEA;         /* destroyed; 13A70.c only draws a shadow while it is 0 */
    /* 0xEB */ u8 unkEB;         /* one of the level's targets */
    /* 0xEC */ u8 damage[16];    /* per group (0-based), 0..100 */
} Building;
SIZE_CHECK(Building, 0xFC);

extern Building D_803F4030[];
extern Building *D_803F7654;       /* one past the last */

/* the effects' sprites by kind (hd_code 7D9D0's table; the handwritten code's func_802A6274) */
extern u8 *D_802C3FFC[];

/* LevelHeader.unk58 (in two levels): what hd_code 32E00.c walks from
 * D_803BE6FC to D_803BE700.  (tools/assetlib/level.py writes it as four
 * s16; the code reads a halfword and five bytes.) */
typedef struct LevelUnk58 {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u8 unk6;
    /* 0x7 */ u8 pad7;
} LevelUnk58;
SIZE_CHECK(LevelUnk58, 8);

/* LevelHeader.commPoint (func_802CEAA0). */
typedef struct LevelCommPoint {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 spin;          /* 1: the dish turns from the start */
} LevelCommPoint;
SIZE_CHECK(LevelCommPoint, 8);

/* LevelHeader.bounds40 and bounds44: boxes in x/z with a height
 * (func_802A5510 takes the highest one the player is in). */
typedef struct LevelBox {
    /* 0x0 */ u16 x1;
    /* 0x2 */ u16 z1;
    /* 0x4 */ u16 x2;
    /* 0x6 */ u16 z2;
    /* 0x8 */ u16 y;
} LevelBox;
SIZE_CHECK(LevelBox, 0xA);

/* LevelHeader.levelBounds: func_802A2D68 copies it to D_803BE730..36. */
typedef struct LevelBounds {
    /* 0x0 */ u16 x1;
    /* 0x2 */ u16 z1;
    /* 0x4 */ u16 x2;
    /* 0x6 */ u16 z2;
} LevelBounds;
SIZE_CHECK(LevelBounds, 8);

/*
 * LevelHeader.terrain, collisionXZ and playerCollisionXZ: groups of
 * triangles, each group a u32 end offset (from the section's start) and
 * then its triangles.  func_802A4464 keeps a pointer to each terrain group
 * in D_803BDB10 (LevelHeader.unk8[0] * unk8[1] of them); func_802A3D54 and
 * func_802A3DF8 walk the collision groups (LevelHeader.unk10[0] * unk10[1]).
 */
typedef struct LevelTerrainTri {
    /* 0x00 */ s16 v[3][3];
    /* 0x12 */ u8 type;          /* 1 dirt, 2 road, 3 grass, 5 pond, 0x67 gravel (docs) */
    /* 0x13 */ u8 flags;
} LevelTerrainTri;
SIZE_CHECK(LevelTerrainTri, 0x14);

typedef struct LevelCollisionTri {
    /* 0x00 */ s16 v[3][3];
    /* 0x12 */ u16 unk12;        /* stored to +0x4C of the run-time triangle (func_802A41B0) */
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15;
} LevelCollisionTri;
SIZE_CHECK(LevelCollisionTri, 0x16);

/* hd_code 77E20's (the buildings module's): func_802C1AA0's result (hd.c). */
extern u8 D_803F7806;

#endif
