#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "game/types.h"

/*
 * Level numbers: the index into D_802E8F94 and the switch in func_8025615C
 * that picks each level's gzip file in ROM.  The names are the levels' own
 * (docs/blast_corps_levels.txt, from the file names and the game).
 * D_802E8BDC is the current one (academy.c's assert calls it `levelno`).
 */
enum LevelId {
    LEVEL_SIMIAN_ACRES,      /*  0 chimp.raw */
    LEVEL_ANGEL_CITY,        /*  1 lagp.raw */
    LEVEL_OUTLAND_FARM,      /*  2 valley.raw */
    LEVEL_BLACKRIDGE_WORKS,  /*  3 fact.raw */
    LEVEL_GLORY_CROSSING,    /*  4 dip.raw */
    LEVEL_SHUTTLE_GULLY,     /*  5 beetle.raw */
    LEVEL_SALVAGE_WHARF,     /*  6 bonus1.raw */
    LEVEL_SKYFALL,           /*  7 bonus2.raw */
    LEVEL_TWILIGHT_FOUNDRY,  /*  8 bonus3.raw */
    LEVEL_CRYSTAL_RIFT,      /*  9 */
    LEVEL_ARGENT_TOWERS,     /* 10 */
    LEVEL_SKERRIES,          /* 11 */
    LEVEL_DIAMOND_SANDS,     /* 12 */
    LEVEL_EBONY_COAST,       /* 13 */
    LEVEL_OYSTER_HARBOR,     /* 14 */
    LEVEL_CARRICK_POINT,     /* 15 */
    LEVEL_HAVOC_DISTRICT,    /* 16 */
    LEVEL_IRONSTONE_MINE,    /* 17 */
    LEVEL_BEETON_TRACKS,     /* 18 */
    LEVEL_J_BOMB,            /* 19 */
    LEVEL_JADE_PLATEAU,      /* 20 */
    LEVEL_MARINE_QUARTER,    /* 21 */
    LEVEL_COOTER_CREEK,      /* 22 */
    LEVEL_GIBBONS_GATE,      /* 23 */
    LEVEL_BABOON_CATACOMB,   /* 24 */
    LEVEL_SLEEK_STREETS,     /* 25 */
    LEVEL_OBSIDIAN_MILE,     /* 26 */
    LEVEL_CORVINE_BLUFF,     /* 27 */
    LEVEL_SIDESWIPE,         /* 28 */
    LEVEL_ECHO_MARCHES,      /* 29 */
    LEVEL_KIPLING_PLANT,     /* 30 */
    LEVEL_FALCHION_FIELD,    /* 31 */
    LEVEL_MORGAN_HALL,       /* 32 */
    LEVEL_TEMPEST_CITY,      /* 33 */
    LEVEL_ORION_PLAZA,       /* 34 */
    LEVEL_GLANDERS_RANCH,    /* 35 */
    LEVEL_DAGGER_PASS,       /* 36 */
    LEVEL_GEODE_SQUARE,      /* 37 */
    LEVEL_SHUTTLE_ISLAND,    /* 38 */
    LEVEL_MICA_PARK,         /* 39 */
    LEVEL_MOON,              /* 40 */
    LEVEL_COBALT_QUARRY,     /* 41 */
    LEVEL_MORAINE_CHASE,     /* 42 */
    LEVEL_MERCURY,           /* 43 */
    LEVEL_VENUS,             /* 44 */
    LEVEL_MARS,              /* 45 */
    LEVEL_NEPTUNE,           /* 46 */
    LEVEL_CMO_INTRO,         /* 47 */
    LEVEL_SILVER_JUNCTION,   /* 48 */
    LEVEL_END_SEQUENCE,      /* 49 */
    LEVEL_SHUTTLE_CLEAR,     /* 50 */
    LEVEL_DARK_HEARTLAND,    /* 51 */
    LEVEL_MAGMA_PEAK,        /* 52 */
    LEVEL_THUNDERFIST,       /* 53 */
    LEVEL_SALINE_WATCH,      /* 54 */
    LEVEL_BACKLASH,          /* 55 */
    LEVEL_BISON_RIDGE,       /* 56 */
    LEVEL_EMBER_HAMLET,      /* 57 */
    LEVEL_CROMLECH_COURT,    /* 58 */
    LEVEL_LIZARD_ISLAND,     /* 59 */
    LEVEL_COUNT
};

/*
 * Per-level constants, D_802E8F94[LEVEL_COUNT] (hd_code 1D990.c's .data).
 * func_80262320 points D_80367C04 at the current level's entry.
 */
typedef struct LevelInfo {
    /* 0x00 */ u8 unk0;           /* 1 for the ordinary levels; 2 gets a ghost buffer (hd.c "Allocating ghost buffer memory"); 0x20, 0x80 and the 0x81 bits are tested */
    /* 0x01 */ u8 gameState;      /* the PlayerInfo.gameState the level belongs to (academy.c) */
    /* 0x02 */ u16 unk2;          /* unk2..unk8: a rectangle, x/z in world units >> 5 */
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 unk6;
    /* 0x08 */ u16 unk8;
    /* 0x0A */ u16 unkA;          /* unkA..unk10: another rectangle (func_8026394C) */
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u8 unk12[4];
    /* 0x16 */ u8 unk16[2];
    /* 0x18 */ u32 goal;          /* "FINISH %d LAPS IN", "CAUSE $%d DAMAGE", "FIND %d RDUS IN" */
    /* 0x1C */ s16 unk1C;         /* << 5 into D_803F7C10 */
    /* 0x1E */ s16 unk1E;         /* << 5 into D_803F7C14 */
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ s16 unk24;         /* compared with the player's y >> 5 */
    /* 0x26 */ s16 unk26;         /* unk26..unk2A: a position (<< 5 into D_803EF2EC..F4) */
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ u32 unk2C;         /* bit flags, 1 << n */
    /* 0x30 */ u16 medalTimes[4]; /* in tenths of a second, fastest first (func_801EF2BC); [3] is the time limit */
    /* 0x38 */ u8 unk38;
    /* 0x39 */ u8 pad39[3];
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ u16 unk40;
    /* 0x42 */ u16 unk42;
} LevelInfo;
SIZE_CHECK(LevelInfo, 0x44);

/* A level's medalTimes[0] in D_802E8F94: jp has its own for most levels. */
#ifdef VERSION_JP
#define MEDAL_TIME0(us, jp) (jp)
#else
#define MEDAL_TIME0(us, jp) (us)
#endif

/* A level's medalTimes: t0..t3, or eu's own (PAL) e0..e3. */
#ifdef VERSION_EU
#define MEDAL_TIMES(t0, t1, t2, t3, e0, e1, e2, e3) { e0, e1, e2, e3 }
#else
#define MEDAL_TIMES(t0, t1, t2, t3, e0, e1, e2, e3) { t0, t1, t2, t3 }
#endif

/* The levels that aren't played: stats.c's assert "!DUMMY_LEVELS(levelno)"
 * tests these three. */
#define DUMMY_LEVELS(l) ((l) == LEVEL_END_SEQUENCE || (l) == LEVEL_CMO_INTRO || (l) == LEVEL_SHUTTLE_ISLAND)

extern LevelInfo D_802E8F94[LEVEL_COUNT];
extern s32 D_802E8BDC; /* the current level: academy.c's `levelno` ("levelno==50") */

/* PlayerInfo.medal[level] is 1..5 once the level is done. */
#define LEVEL_DONE_IN(player, level) \
    (((player).medal[level] > 0 && (player).medal[level] < 6) ? 1 : 0)

/* D_802E8F38[6] (1D990.c's .data): a spot in six of the levels; 52D70.c
 * copies the one for the current level to D_8039CAB0..B4. */
typedef struct UnkStruct_802E8F38 {
    /* 0x0 */ u8 level;
    /* 0x2 */ s16 x;
    /* 0x4 */ s16 y;
    /* 0x6 */ s16 z;
} UnkStruct_802E8F38;
SIZE_CHECK(UnkStruct_802E8F38, 8);

extern UnkStruct_802E8F38 D_802E8F38[6];

/*
 * The front end's per-level table, D_8020D810[LEVEL_COUNT] (hd_front_end
 * 11530.c's .data): the level's name ("SIMIAN ACRES", ...; stats.c prints
 * it), the name in the u16 text encoding, and what the front end's map uses
 * (floats, two lists of other levels).
 */
typedef struct UnkStruct_8020D810 {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ char *name;
#ifdef VERSION_EU
    /* 0x08 */ char *name2;          /* eu: the German name */
    /* 0x0C */ char *name3;          /* eu: NULL in the table */
#endif
    /* 0x08 */ u16 *unk8;            /* eu: NULL in the table */
    /* 0x0C */ u8 unkC[4];
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ s8 unk18[4];       /* level numbers, -1 terminated */
    /* 0x1C */ s8 unk1C[8];       /* level numbers, -1 terminated */
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
} UnkStruct_8020D810;
/* A level's name in the language shown (eu has one per language). */
#ifdef VERSION_EU
extern u8 D_80366F70_eu;
#define LEVEL_NAME(e) (&(e).name)[D_80366F70_eu]
#else
#define LEVEL_NAME(e) (e).name
#endif
#ifdef VERSION_EU
SIZE_CHECK_C(UnkStruct_8020D810, 0x38); /* eu: the offsets from unk8 on are 8 more */
#else
SIZE_CHECK_C(UnkStruct_8020D810, 0x30);
#endif

extern UnkStruct_8020D810 D_8020D810[LEVEL_COUNT];

/*
 * The header of a level file (the gzip members func_8025615C picks; loaded
 * at D_80358074 by the level setup).  0xC8 bytes, followed by the display
 * data that segment 8 points at.  From docs/blast_corps_levels.txt; only
 * the handwritten engine reads it (func_802A1674 and what it calls).
 *
 * Every offset is from the start of the file, so a native port that keeps
 * the file as loaded can use them as they are, after byteswapping.
 */
typedef struct LevelHeader {
    /* 0x00 */ u16 unk0[2];
    /* 0x04 */ u16 unk4[2];          /* << 5 */
    /* 0x08 */ u16 unk8[2];          /* multiplied: the number of terrain groups */
    /* 0x0C */ u16 unkC[2];          /* << 5 */
    /* 0x10 */ u16 unk10[2];
    /* 0x14 */ u16 unk14[2];         /* << 5 */
    /* 0x18 */ s32 gravity;          /* -4, -2 on the planets */
    /* 0x1C */ u32 unk1C;            /* 0 in dip, level9, level18, level20, level34, level43 */
    /* 0x20 */ AssetOffset ammoBoxes;      /* s16 x, y, z, type (0 missiles, 1 hydraulics) */
    /* 0x24 */ AssetOffset collisionFixes; /* 0x14-byte triangles */
    /* 0x28 */ AssetOffset commPoint;
    /* 0x2C */ AssetOffset animTextures;
    /* 0x30 */ AssetOffset terrain;
    /* 0x34 */ AssetOffset rdus;           /* s16 x, y, z */
    /* 0x38 */ AssetOffset tntCrates;
    /* 0x3C */ AssetOffset blocks;         /* square blocks and holes */
    /* 0x40 */ AssetOffset bounds40;       /* 10-byte boxes, func_802A5510 */
    /* 0x44 */ AssetOffset bounds44;
    /* 0x48 */ AssetOffset unk48;
    /* 0x4C */ AssetOffset levelBounds;
    /* 0x50 */ AssetOffset vehicles;       /* 9-byte records: type, s16 x, y, z, heading */
    /* 0x54 */ AssetOffset carrier;        /* the missile carrier's path */
    /* 0x58 */ AssetOffset unk58;
    /* 0x5C */ AssetOffset buildings;      /* 14-byte records */
    /* 0x60 */ AssetOffset unk60;
    /* 0x64 */ AssetOffset unk64;
    /* 0x68 */ AssetOffset trainStops;
    /* 0x6C */ AssetOffset collisionXZ;
    /* 0x70 */ AssetOffset playerCollisionXZ;
    /* 0x74 */ AssetOffset unk74;
    /* 0x78 */ AssetOffset displayLists[10]; /* into the display data after the header */
    /* 0xA0 */ AssetOffset unkA0[9];
    /* 0xC4 */ u32 unkC4;             /* not an offset: 0 in chimp (tools/assetlib/level.py keeps it as a value) */
} LevelHeader;
SIZE_CHECK(LevelHeader, 0xC8);

/* The current level's file, at the bottom of the level's heap allocations. */
extern LevelHeader *D_80358074;

/*
 * What the level loader (func_802A1674 and its helpers, hd_code 5CB60,
 * handwritten) keeps about the loaded level: separate variables in 5CB60's
 * .bss, each addressed by its own symbol.  Pointers into the level file
 * are the file's address plus a LevelHeader offset.  The level is split
 * into grids of cells in x and z (three: the objects', the terrain's and
 * the collision's), each given by a cell size (<< 5) and a count.
 */
extern void *D_803BDAF0;        /* D_803BD310: 0xFC-byte records (the walls, LevelHeader.unk64) */
extern s16 *D_803BDAF4;         /* LevelHeader.collisionFixes: triangles of 9 s16 and a pad */
extern s16 *D_803BDAF8;         /* ... and its end */
extern void *D_803BDAFC;        /* a model func_802A32CC loaded for the missile carrier (func_802A3198) */
extern void *D_803BDB00;        /* a heap block: segment 7 while 50670.c draws */
extern void *D_803BDB04;        /* that model + its unk14: segment 6 */
extern void *D_803BDB08;        /* that model + its unk24: a display list */
extern u8 *D_803BDB10[102];     /* LevelHeader.terrain: each group's first triangle (func_802A4464) */
/* LevelHeader.collisionXZ and playerCollisionXZ: each cell's first CollisionTri,
   the next cell's its end (func_802A3D54, func_802A3DF8).  D_803BDE40 is one
   short of the cells and their end: its last is the lights' end D_803BDFD4,
   which they set after it (the port's has the room where pointers aren't the
   N64's 4 bytes: port/tools/asm2c.py). */
extern struct CollisionTri *D_803BDCA8[102];
extern struct CollisionTri *D_803BDE40[];
extern Gfx *D_803BE6E0;         /* level display lists (LevelHeader.displayLists) hd.c draws */
extern Gfx *D_803BE6E4;
extern Gfx *D_803BE6E8;
extern Gfx *D_803BE6EC;
extern u32 *D_803BE6F0;         /* the model table (DMA'd, func_802A2BB0) */
extern s32 D_803BE6F4;          /* func_802A1674's second argument */
extern struct LevelUnk58 *D_803BE6FC;  /* LevelHeader.unk58 (game/objects.h) ... */
extern struct LevelUnk58 *D_803BE700;  /* ... to LevelHeader.buildings */
extern void *D_803BE6F8;        /* LevelHeader.vehicles: the vehicles' records, read as their start points */
extern u8 *D_803BE704;          /* the level's building groups (0x18-byte records): the next one ... */
extern u8 *D_803BE708;          /* ... and the first */
extern s32 D_803BE70C;          /* the object grid (the RDUs' Rdu.cell): cell width in x, << 5 */
extern s32 D_803BE710;          /* ... in z */
extern u16 D_803BE714;          /* ... cells in x (LevelHeader.unk0[0]) */
extern u16 D_803BE716;          /* ... in z */
extern u32 D_803BE718;          /* the terrain grid: cell width in x, << 5 (LevelHeader.unkC) */
extern u32 D_803BE71C;
extern u16 D_803BE720;          /* ... cells (LevelHeader.unk8) */
extern u16 D_803BE722;
extern s32 D_803BE724;          /* the collision grid: cell width << 5 (LevelHeader.unk14) */
extern s32 D_803BE728;
extern s16 D_803BE72C;          /* ... cells (LevelHeader.unk10) */
extern s16 D_803BE72E;
extern s16 D_803BE730;          /* LevelBounds.x1 (func_802A2D68) */
extern s16 D_803BE732;          /* LevelBounds.x2 */
extern s16 D_803BE734;          /* LevelBounds.z1 */
extern s16 D_803BE736;          /* LevelBounds.z2 */
extern u8 D_803BE738;
extern u8 D_803BE739;           /* LevelHeader.unk1C */

/*
 * Other pointers into the level that the game's C and the handwritten code
 * (port/engine) share, each declared once here.
 */
extern u8 *D_80364458;          /* 00000.c's: the level's display data, after its header (segment 8) */
extern u8 *D_80365330;          /* 13A70.c's: a texture of the level (func_802A0CFC 0xF81) */
extern Gfx *D_8039CABC;         /* 52D70.c's: the level's extra model's display list ... */
extern void *D_8039CAC0;        /* ... and its vertices (segment 6) */
extern u8 *D_803EBBEC;          /* 62740's: the end of D_803EBDB0's records */
extern u8 *D_803F7820;          /* 77E20's: the moving sections' matrices (segment 10), two sets */
extern u8 *D_803F7824;
extern void *D_803F7828;        /* ... their triangles (0x28 bytes each) ... */
extern void *D_803F782C;        /* ... to here */
extern struct Building *D_8036B974;  /* 23C20.c's: the building the hint is about */

/* 30C70.c's: the radar's outline of the target (its corners, x y z) */
typedef struct UnkStruct_8036C7A0 {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
} UnkStruct_8036C7A0; /* size = 0x6 */
extern UnkStruct_8036C7A0 *D_8036C790;  /* the target's, from the handwritten code (func_802BCE40) */
extern UnkStruct_8036C7A0 *D_8036C794;

/*
 * Objects the level file places.  The records are read straight out of the
 * loaded file (at the LevelHeader offsets), so they are big-endian ROM data.
 */

/* LevelHeader.rdus: one RDU (docs/blast_corps_levels.txt, offset 0x34). */
typedef struct LevelRdu {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
} LevelRdu;
SIZE_CHECK(LevelRdu, 6);

/*
 * The level's RDUs at run time: func_8026FBB0 copies the LevelRdu records
 * into D_8036BED8[D_8036EB90], allocated from the heap.  D_8036EA7C
 * (LevelStats.rt) counts the collected ones (LevelInfo.goal for "FIND %d
 * RDUS IN").
 */
typedef struct Rdu {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
    /* 0x06 */ u8 collected;
    /* 0x07 */ u8 cell;           /* the level grid cell it is in (LevelHeader.unk0/unk4 sizes) */
    /* 0x08 */ Vtx vtx[2][4];     /* its quad, double-buffered by D_8036BEE0 */
} Rdu;
SIZE_CHECK(Rdu, 0x88);

/*
 * A level's results, four copies in hd_code 409D0.c (stats_perm.c): the
 * current attempt (D_8036EA70), the previous one (D_8036EA60) and two saved
 * copies (D_8036EA80, D_8036EA90), moved about whole with func_80285A78.
 * The field names are stats.c's, from its debug print ("new ip=%8d : tc=%5d
 * : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d"); other files still name
 * the fields by their own symbols (D_8036EA7C is D_8036EA70.rt).
 */
typedef struct LevelStats {
    /* 0x0 */ u32 ip;             /* the damage, shown as "$%d" */
    /* 0x4 */ u32 tc;             /* the time, saved as the best time */
    /* 0x8 */ u8 bd;              /* out of D_8036EB92, shown as a percentage */
    /* 0x9 */ u8 cr;              /* out of D_8036EB93 */
    /* 0xA */ u8 coin;            /* the medal (PlayerInfo.medal) */
    /* 0xB */ u8 bdn;             /* PlayerInfo.unk92[level]: which time slot */
    /* 0xC */ u16 rt;             /* RDUs collected, out of D_8036EB90 */
    /* 0xE */ u8 padE[2];
} LevelStats;
SIZE_CHECK(LevelStats, 0x10);

extern LevelStats D_8036EA60;
extern LevelStats D_8036EA70;
extern LevelStats D_8036EA80;
extern LevelStats D_8036EA90;

extern Rdu *D_8036BED8;
extern u16 D_8036BBB0[0x192];   /* 2B3F0.c's: the RDUs in the order collected (indices into D_8036BED8) */
extern u16 D_8036EB90;
extern u16 D_8036EA7C;

/* 409D0.c's (stats_perm.c's) and 1D990.c's flags. */
extern u8 D_8036EB98;
extern u8 D_8036EB99;
extern s8 D_80367BFF;
extern u8 D_80367C00;

#endif
