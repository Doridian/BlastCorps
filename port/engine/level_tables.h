/*
 * Run-time tables the level loader (hd_code 5CB60) fills from the level
 * file and other modules read: the objects to destroy (77E20) and the
 * lights (62740, 77E20).
 */
#ifndef ENGINE_LEVEL_TABLES_H
#define ENGINE_LEVEL_TABLES_H

#include "engine.h"

/* D_80306480: the level's objects to destroy, 0x30-byte records to one
   whose level is -1 (77E20's func_802BC714): a position, the ids
   (func_8029D210's) of the triangles that make each up, the level it is
   in, and its box's corners, which the radar draws (D_8036C790) */
typedef struct TargetObj {
    /* 0x00 */ s32 x, y, z;         /* << 5 */
    /* 0x0C */ s8 ids[8];           /* -1 after the last */
    /* 0x14 */ u8 size;             /* half the box's width, world units */
    /* 0x15 */ s8 level;            /* LevelId; -1 after the last record */
    /* 0x16 */ s16 corners[4][3];   /* (func_802A1934): (-,-), (+,-), (-,+), (+,+) in x, z */
    /* 0x2E */ u8 pad2E[2];
} TargetObj;
SIZE_CHECK(TargetObj, 0x30);
extern TargetObj D_80306480[];

/* D_803BDFD8..D_803BDFD4: the level's lights (5CB60's func_802A2C54 from
   LevelHeader.unk60; 62740's func_802ABD54 lights the vehicles) */
typedef struct LevelLight {
    /* 0x00 */ s32 x, y, z;         /* << 5 */
    /* 0x0C */ s32 radius;          /* << 5: its reach */
    /* 0x10 */ u8 full;             /* 1: full brightness in its reach, else fading */
    /* 0x11 */ u8 building;         /* the Building it is on, + 1 (77E20's
                                       func_802BF534 turns it off with it) */
    /* 0x12 */ u8 on;
    /* 0x13 */ u8 ntypes;           /* the VehicleTypes it lights */
    /* 0x14 */ u8 shade;            /* its own light level (0: white) */
    /* 0x15 */ u8 types[15];
} LevelLight;
SIZE_CHECK(LevelLight, 0x24);
extern LevelLight D_803BDFD8[];
extern LevelLight *D_803BDFD4;          /* one past the last */

/* D_803BE708: the level's group sets (5CB60's func_802A1EC8 picks one of
   the tables D_802D30D0...; hd_code's .text island 8E910), a u32 count,
   then these: the groups of a building whose destruction counts as the
   building's (77E20's func_802BD064).  The ROM has 0 in `building`; the
   level loader (func_802A21AC) gives the sets to the buildings flagged
   for one, in order, at each load of the level.  (The original keeps the
   building's address there.) */
typedef struct GroupSet {
    /* 0x00 */ u32 building;        /* GROUP_SET_KEY: the Building, + 1 (0: none) */
    /* 0x04 */ u32 n;
    /* 0x08 */ u8 groups[16];       /* 0-based */
} GroupSet;
SIZE_CHECK(GroupSet, 0x18);
/* Building b's key in a group set: its index in D_803F4030, + 1 */
#define GROUP_SET_KEY(b) ((u32)((b) - D_803F4030) + 1)

#endif
