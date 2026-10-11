#ifndef GAME_MODEL_H
#define GAME_MODEL_H

#include "game/types.h"

/*
 * The models of the model table (docs/ASSETS.md: model_table_ROM_START,
 * 512 offsets; the gzip pairs la*, ch*, co*, ...): func_802A2A98 (hd_code
 * 5CB60, handwritten) DMAs model i, inflates it and its display list onto
 * the heap (func_802C4108 twice), fixes it up (func_802A44E4) and returns it
 * in $s0.  The buildings load theirs this way (func_802A1D54; Building.model
 * points at it), as do the level's other models.
 *
 * This is the header as the code reads it: ROM data, relocated in place.
 * The AssetOffset fields are offsets from the model's start that the code
 * adds to its address; nothing in it is a stored pointer.  The loader also
 * writes into it (unk4, unkE, and bytes from unk5C in 0x10-byte steps), so
 * each building's copy is its own.  Fields not listed are never read by the
 * code that was analysed (tools/fieldscan.py).
 */
typedef struct Model {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ s8 unk4;              /* set to 2 by func_802A2164 */
    /* 0x05 */ u8 pad5;
    /* 0x06 */ s8 unk6;
    /* 0x07 */ u8 pad7[7];
    /* 0x0E */ u16 unkE;             /* LevelBuilding.behavior (func_802A1D54) */
    /* 0x10 */ AssetOffset unk10;    /* the display list's head, */
    /* 0x14 */ AssetOffset unk14;    /* the end of its textures (func_802A08E4), */
    /* 0x18 */ AssetOffset unk18;    /* and its end (the buildings' MS_DL_END) */
    /* 0x1C */ AssetOffset unk1C;
    /* 0x20 */ AssetOffset unk20;
    /* 0x24 */ AssetOffset unk24;    /* a table of 0x10-byte entries (func_802A26A8) */
    /* 0x28 */ AssetOffset unk28;
    /* 0x2C */ AssetOffset unk2C;
    /* 0x30 */ AssetOffset unk30;
    /* 0x34 */ AssetOffset unk34;
    /* 0x38 */ AssetOffset unk38;
    /* 0x3C */ AssetOffset unk3C;    /* u16 per group (the buildings' MS_FALLS) */
    /* 0x40 */ AssetOffset unk40;
    /* 0x44 */ AssetOffset unk44;
    /* 0x48 */ AssetOffset unk48;    /* bytes, read in pairs (func_802A21AC, func_802A26A8) */
    /* 0x4C */ AssetOffset unk4C;
    /* 0x50 */ s16 unk50;
    /* 0x52 */ s16 unk52;
    /* 0x54 */ s16 unk54;
    /* 0x56 */ u8 pad56[6];
    /* 0x5C */ s8 unk5C;             /* 0xFF, then 0, 0 (func_802A20F4) */
    /* 0x5D */ s8 unk5D;
    /* 0x5E */ s8 unk5E;
    /* 0x5F */ u8 pad5F;
} Model;
SIZE_CHECK(Model, 0x60);

/*
 * Model.unk38's records (the buildings' MS_GROUP_DLS, port/engine/
 * buildings.h): for each look of a group, the conditions on the groups'
 * damage that hide it (the first one is on its own group), then its
 * display lists for the two passes, as offsets from the model's start.  A
 * record is {u32 n; GroupCond cond[n]; GroupDls dls;}, up to Model.unk3C.
 */
typedef struct GroupCond {
    /* 0x0 */ u16 group;         /* 1-based */
    /* 0x2 */ u8 below;          /* hides it while the damage is under `limit`; 0: over it */
    /* 0x3 */ u8 limit;
} GroupCond;
typedef struct GroupDls {
    /* 0x0 */ AssetOffset dl0, dl0_end;
    /* 0x8 */ AssetOffset dl1, dl1_end;
} GroupDls;
#define GDL_N(r) (*(u32 *)(r))
#define GDL_COND(r) ((GroupCond *)((u8 *)(r) + 4))
#define GDL_GROUP(r) (GDL_COND(r)->group)
#define GDL_DLS(r, n) ((GroupDls *)((u8 *)(r) + 4 + (n) * 4))
#define GDL_NEXT(r) ((u8 *)GDL_DLS(r, GDL_N(r)) + sizeof(GroupDls))

#endif
