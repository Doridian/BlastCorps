#ifndef GAME_VEHICLE_H
#define GAME_VEHICLE_H

#include "game/types.h"

/*
 * Vehicle (and other level object) types: the first byte of a level file's
 * vehicle record, and the switch in the object loader (func_802A396C, docs/
 * blast_corps_vehicles.txt).  The names are the model files' (the docs list
 * each type's ROM range), with the game's name where it differs.
 * D_80364456 is the one the player is in.
 */
enum VehicleType {
    VEHICLE_DRIVER = 0x00,      /* on foot */
    VEHICLE_SIDESWIPE = 0x01,
    VEHICLE_MAGOO = 0x02,       /* Thunderfist */
    VEHICLE_BUGGY = 0x03,       /* Skyfall */
    VEHICLE_BULLDOZER = 0x04,   /* Ramdozer */
    VEHICLE_TRUCK = 0x05,       /* Backlash */
    VEHICLE_CRANE = 0x06,
    VEHICLE_TRAIN = 0x07,
    VEHICLE_HOTROD = 0x08,      /* American Dream */
    VEHICLE_JETPACK = 0x09,     /* J-Bomb */
    VEHICLE_BIKE = 0x0A,        /* Ballista */
    VEHICLE_BARGE = 0x0B,       /* also 0x11 and 0x12 */
    VEHICLE_POLICE = 0x0D,
    VEHICLE_ATEAM = 0x0E,       /* A-Team Van */
    VEHICLE_STARSKI = 0x0F,     /* the other hotrod */
    VEHICLE_MINIMAGOO = 0x10,   /* Cyclone Suit */
    VEHICLE_BARGE_2 = 0x11,
    VEHICLE_BARGE_3 = 0x12,
    VEHICLE_COMMPOINT = 0x96,   /* the communication point satellite */
    VEHICLE_SCIENTIST = 0x98,
    VEHICLE_SHUTTLE = 0xFD,
    VEHICLE_CHOPPER = 0xFE,     /* the BCT chopper */
    VEHICLE_CMO = 0xFF          /* the missile carrier */
};

/*
 * The level's vehicles: D_80364460[12] (hd.c's .bss), D_803649D0 one past
 * the last one in use.  func_802A1388 (hd_code 5CB60, handwritten) fills
 * one from the vehicle's model file once func_802A396C has loaded it: every
 * pointer here is into that file or its display list file, both on the
 * heap.  (The RDRAM snapshots agree: 0x00-0x54 always hold RAM addresses.)
 */
typedef struct Vehicle {
    /* 0x00 */ void *unk0;        /* model + VehicleModel.unk14 */
    /* 0x04 */ void *unk4;        /* the model file (func_802A1388's $v0) */
    /* 0x08 */ void *unk8;        /* the display list file ($v1) */
    /* 0x0C */ void *unkC;        /* unkC..unk2C: model + VehicleModel.unk24..unk44 */
    /* 0x10 */ void *unk10;
    /* 0x14 */ void *unk14;
    /* 0x18 */ void *unk18;
    /* 0x1C */ void *unk1C;
    /* 0x20 */ void *unk20;
    /* 0x24 */ void *unk24;
    /* 0x28 */ void *unk28;
    /* 0x2C */ void *unk2C;
    /* 0x30 */ void *unk30;       /* unk30..unk50: from unkC..unk2C (func_802A1388) */
    /* 0x34 */ void *unk34;
    /* 0x38 */ void *unk38;       /* a display list */
    /* 0x3C */ void *unk3C;
    /* 0x40 */ void *unk40;
    /* 0x44 */ void *unk44;
    /* 0x48 */ void *unk48;
    /* 0x4C */ void *unk4C;
    /* 0x50 */ void *unk50;
    /* 0x54 */ Gfx *unk54;        /* model + VehicleModel.unk48 */
    /* 0x58 */ void *unk58;       /* a heap block (hd.c's func_80278BF0) */
    /* 0x5C */ s32 type;          /* VehicleType; the one equal to D_80364456 is the player's */
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s32 x;             /* world position << 5, as D_803643E0..E8 */
    /* 0x68 */ s32 y;
    /* 0x6C */ s32 z;
    /* 0x70 */ s32 unk70;
} Vehicle;
SIZE_CHECK(Vehicle, 0x74);

extern Vehicle D_80364460[12];
extern Vehicle *D_803649D0;
extern u8 D_80364456; /* the player's VehicleType */

/*
 * A vehicle's model file (the gzip member named after the vehicle, docs/
 * ASSETS.md), as loaded on the heap: a table of offsets from the file's
 * start (AssetOffset), which the handwritten code adds to the file's address
 * (func_802A1388 for the Vehicle record).  The vehicle modules keep a
 * pointer to their file (D_803ED818 and the like, below).  Every word up to
 * 0x48 is used as an offset somewhere.
 */
typedef struct VehicleModel {
    /* 0x00 */ AssetOffset unk0[5];
    /* 0x14 */ AssetOffset unk14;
    /* 0x18 */ AssetOffset unk18;      /* its pointee's +4 is read by every module */
    /* 0x1C */ AssetOffset unk1C;
    /* 0x20 */ AssetOffset unk20;
    /* 0x24 */ AssetOffset unk24[9];
    /* 0x48 */ AssetOffset unk48;
} VehicleModel;
SIZE_CHECK(VehicleModel, 0x4C);

/*
 * The vehicles' simulation.  Each kind of vehicle is one handwritten object
 * in hd_code (the modules below: the level loader func_802A350C and hd.c's
 * switches on D_80364456 call into them by type), and each keeps its state
 * in its own .bss: a block per vehicle of
 *
 *   0x000  UnkStruct_803ED460 parts[32]   (hd_code 56040)
 *   0x300  VehicleState state             (hd_code 62740, through $gp)
 *   0x3A8  the vehicle's own variables: x, y, z (s32, << 5) first, then
 *          pointers to its model file and the rest (0x3AC in three)
 *
 * The parts and the state are structures (the code reaches them through a
 * base register); what follows is separate variables, each addressed by its
 * own symbol, so it is declared as such.  func_802A75DC copies the current
 * vehicle's parts, state and position to D_803EB7A0 and func_802A768C back.
 *
 *   module  bss block    type(s)                 state        x, y, z
 *   69BB0   0x803ED460   DRIVER                  D_803ED760   D_803ED808
 *   6B4A0   0x803ED840   SIDESWIPE               D_803EDB40   D_803EDBE8
 *   6C5E0   0x803EDC10   MAGOO                   D_803EDF10   D_803EDFB8
 *   6E200   0x803EDFE0   BUGGY                   D_803EE2E0   D_803EE38C
 *           0x803EE3C0   BULLDOZER               D_803EE6C0   D_803EE768
 *   71140   0x803EE790   TRUCK                   D_803EEA90   D_803EEB38
 *   72B80   0x803EEB70   HOTROD                  D_803EEE70   D_803EEF18
 *           0x803EEF40   (func_802B8480, from func_802A3134)  D_803EF240  D_803EF2EC
 *   75490   0x803EF330   CMO (func_802B9C50)     D_803EF630   D_803EF6DC
 *           0x803EF720   CRANE                   D_803EFA20   D_803EFACC
 *   772A0   0x803EFAF0   TRAIN                   D_803EFDF0   D_803EFE98
 *   80280   0x803F7850   JETPACK                 D_803F7B50   D_803F7BF8
 *   83910   0x803F7C50   BARGE (three: parts[3][32], state[3], positions[3])
 *                                                D_803F8550   D_803F8748
 *   853D0   0x803F87A0   BIKE                    D_803F8AA0   D_803F8B48
 *   86F60   0x803F8B80   POLICE                  D_803F8E80   D_803F8F28
 *   88160   0x803F8F50   ATEAM                   D_803F9250   D_803F92F8
 *   8AEE0   0x803FC200   STARSKI                 D_803FC500   D_803FC5A8
 *           0x803FC5D0   MINIMAGOO               D_803FC8D0   D_803FC978
 *   8DDB0   0x803FC9A0   CHOPPER (func_802D2570, from func_802A30DC)
 *                                                D_803FCCA0   D_803FCD48
 *
 * The blocks are the handwritten objects' .bss (asm/bss), so the C only
 * declares them.
 */

/*
 * 32 per vehicle block (and a few more users: hd_front_end's models at
 * D_80211AC0 and D_80218430, hd_code's D_803B35F8), handled by hd_code
 * 56040.  unk0 points at a small record of bytes and halfwords.  Its role
 * isn't known.
 */
typedef struct UnkStruct_803ED460 {
    /* 0x00 */ void *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s8 unk10;
    /* 0x11 */ s8 unk11;
    /* 0x12 */ s8 unk12;
    /* 0x13 */ s8 unk13;
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ u8 pad16[2];
} UnkStruct_803ED460;
SIZE_CHECK(UnkStruct_803ED460, 0x18);

/*
 * A vehicle's physical state: hd_code 62740's functions take it in $gp, and
 * every vehicle module passes its own (the table above).  0xA6 bytes of it
 * are copied (func_802A75DC).  Nothing here is a pointer.  The widths are
 * those of the loads and stores.
 */
typedef struct VehicleState {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ s32 unk4[9];       /* func_802A83B8 and its neighbours take &unk4 in $s7 and use all nine */
    /* 0x28 */ s32 unk28[9];      /* nine more, used alike (by absolute address only) */
    /* 0x4C */ u16 unk4C;
    /* 0x4E */ u16 unk4E;
    /* 0x50 */ u8 unk50;
    /* 0x51 */ u8 pad51;
    /* 0x52 */ s16 unk52[6];      /* two groups of six (func_802A90E4 takes either in $v1) */
    /* 0x5E */ s16 unk5E[6];
    /* 0x6A */ s16 unk6A;
    /* 0x6C */ s16 unk6C;
    /* 0x6E */ s16 unk6E;
    /* 0x70 */ s16 unk70;
    /* 0x72 */ s16 unk72;
    /* 0x74 */ s16 unk74;
    /* 0x76 */ s16 unk76;         /* passed by address as a group of its own (func_802A785C's $t6) */
    /* 0x78 */ s16 unk78[15];     /* func_802A785C and its callees take &unk78 in $s1 */
    /* 0x96 */ u8 unk96[4];       /* func_802A7D68 takes &unk96 in $s0 */
    /* 0x9A */ s8 unk9A;
    /* 0x9B */ u8 unk9B;
    /* 0x9C */ u8 unk9C;
    /* 0x9D */ u8 unk9D;
    /* 0x9E */ u8 unk9E;
    /* 0x9F */ u8 unk9F;
    /* 0xA0 */ u8 unkA0;
    /* 0xA1 */ u8 unkA1;
    /* 0xA2 */ u8 unkA2;
    /* 0xA3 */ u8 unkA3;
    /* 0xA4 */ s8 unkA4;
    /* 0xA5 */ s8 unkA5;
    /* 0xA6 */ s16 unkA6;         /* 72B80 and 75490 only */
} VehicleState;
SIZE_CHECK(VehicleState, 0xA8);

/* The vehicles' parts (the table above; the barges' are [3][32]). */
extern UnkStruct_803ED460 D_803ED460[32]; /* DRIVER */
extern UnkStruct_803ED460 D_803ED840[32]; /* SIDESWIPE */
extern UnkStruct_803ED460 D_803EDC10[32]; /* MAGOO */
extern UnkStruct_803ED460 D_803EDFE0[32]; /* BUGGY */
extern UnkStruct_803ED460 D_803EE3C0[32]; /* BULLDOZER */
extern UnkStruct_803ED460 D_803EE790[32]; /* TRUCK */
extern UnkStruct_803ED460 D_803EEB70[32]; /* HOTROD */
extern UnkStruct_803ED460 D_803EEF40[32]; /* 72B80's second */
extern UnkStruct_803ED460 D_803EF330[32]; /* CMO */
extern UnkStruct_803ED460 D_803EF720[32]; /* CRANE */
extern UnkStruct_803ED460 D_803EFAF0[32]; /* TRAIN */
extern UnkStruct_803ED460 D_803F7850[32]; /* JETPACK */
extern UnkStruct_803ED460 D_803F7C50[3 * 32]; /* BARGE */
extern UnkStruct_803ED460 D_803F87A0[32]; /* BIKE */
extern UnkStruct_803ED460 D_803F8B80[32]; /* POLICE */
extern UnkStruct_803ED460 D_803F8F50[32]; /* ATEAM */
extern UnkStruct_803ED460 D_803FC200[32]; /* STARSKI */
extern UnkStruct_803ED460 D_803FC5D0[32]; /* MINIMAGOO */
extern UnkStruct_803ED460 D_803FC9A0[32]; /* CHOPPER */

/* The vehicles' states (the table above). */
extern VehicleState D_803ED760; /* DRIVER */
extern VehicleState D_803EDB40; /* SIDESWIPE */
extern VehicleState D_803EDF10; /* MAGOO */
extern VehicleState D_803EE2E0; /* BUGGY */
extern VehicleState D_803EE6C0; /* BULLDOZER */
extern VehicleState D_803EEA90; /* TRUCK */
extern VehicleState D_803EEE70; /* HOTROD */
extern VehicleState D_803EF240; /* 72B80's second */
extern VehicleState D_803EF630; /* CMO */
extern VehicleState D_803EFA20; /* CRANE */
extern VehicleState D_803EFDF0; /* TRAIN */
extern VehicleState D_803F7B50; /* JETPACK */
extern VehicleState D_803F8550[3]; /* BARGE */
extern VehicleState D_803F8AA0; /* BIKE */
extern VehicleState D_803F8E80; /* POLICE */
extern VehicleState D_803F9250; /* ATEAM */
extern VehicleState D_803FC500; /* STARSKI */
extern VehicleState D_803FC8D0; /* MINIMAGOO */
extern VehicleState D_803FCCA0; /* CHOPPER */

/* hd_code 62740's own variables that the C reads. */
extern s16 D_803ED390[3];
extern s8 D_803ED3F5;
extern s16 D_803ED400;
extern s16 D_803ED408;        /* 0x7FF - x / 16 (hd_code 46F60.c) */
extern u8 D_803ED40A;
extern u8 D_803ED40C;         /* set by 48D00.c and 4B5E0.c when a collision test sets D_803F932E */
extern u8 D_803ED40D;

/* The driver (69BB0). */
extern s32 D_803ED808;        /* x, y, z: the player's start (hd.c copies them to D_803643E0..E8) */
extern s32 D_803ED80C;
extern s32 D_803ED810;
extern VehicleModel *D_803ED818; /* its model file */
extern u8 D_803ED826;

/* The Sideswipe (6B4A0). */
extern s16 D_803EDC00;        /* hydraulics: an ammo box of type 1 adds 10 (func_8028C874) */

/* The Skyfall (6E200's first). */
extern u8 D_803EE3B1;

/* 72B80's second vehicle. */
extern s32 D_803EF2EC;        /* x, y, z: set from LevelInfo.unk26..2A (1D990.c) */
extern s32 D_803EF2F0;
extern s32 D_803EF2F4;
extern s32 D_803EF308;
extern s32 D_803EF30C;
extern s32 D_803EF310;
extern s32 D_803EF314;
extern s32 D_803EF318;
extern s16 D_803EF326;        /* a heading, 0..0xFFF (hd.c turns it into degrees for guRotate) */
extern s16 D_803EF328;
extern s16 D_803EF32A;
extern u8 D_803EF32C;
extern u8 D_803EF32D;
extern u8 D_803EF32E;

/* The missile carrier (75490's first). */
extern s32 D_803EF6DC;        /* x, y, z: blocks it pushes move with it (4B5E0.c) */
extern s32 D_803EF6E0;
extern s32 D_803EF6E4;
extern s32 D_803EF6F0;
extern s32 D_803EF6F8;
extern u8 D_803EF6FF;

/* The train (772A0). */
extern s32 D_803EFEB0;
extern s32 D_803EFEB4;
extern s32 D_803EFEB8;
extern s32 D_803EFEBC;
extern s8 D_803EFEC8;
extern u8 D_803EFECB;

/* The J-Bomb (80280). */
extern s32 D_803F7C10;        /* set from LevelInfo.unk1C << 5 (1D990.c) */
extern s32 D_803F7C14;        /* from LevelInfo.unk1E << 5 */
extern s16 D_803F7C34;
extern u8 D_803F7C3F;

/* The Ballista (853D0). */
extern s16 D_803F8B72;        /* missiles: an ammo box of type 0 adds 10 (func_8028C874) */

/* The BCT chopper (8DDB0). */
extern s32 D_803FCD48;        /* x, y, z */
extern s32 D_803FCD4C;
extern s32 D_803FCD50;
extern s32 D_803FCD60;
extern s16 D_803FCD68;
extern s16 D_803FCD6A;
extern s16 D_803FCD6C;
extern s16 D_803FCD6E;
extern u8 D_803FCD70;
extern u8 D_803FCD75;

#endif
