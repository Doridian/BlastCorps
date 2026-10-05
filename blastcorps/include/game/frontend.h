#ifndef GAME_FRONTEND_H
#define GAME_FRONTEND_H

#include "game/types.h"

/*
 * Structures that more than one hd_front_end file uses.  hd_front_end
 * 9570.c and C450.c were one object before the split at 0xC450.
 */

/* The front end's gDPSetPrimColor with u8 fields ORs the colour word in a
 * different order (DECOMPILING.md, "Display lists"). */
#define gDPSetPrimColorB(pkt, m, l, r, g, b, a)                                                         \
    {                                                                                                  \
        Gfx *_g = (Gfx *)(pkt);                                                                        \
                                                                                                       \
        _g->words.w0 = (_SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8));         \
        _g->words.w1 = (_SHIFTL(b, 8, 8) | (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8)) | _SHIFTL(a, 0, 8)); \
    }

/* A node of the front end's object tree (D_8020BD30[7], C450.c's .data;
 * D_8020BE98 is its [6], reached as its own symbol). */
typedef struct UnkStruct_8020BD30 {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ struct UnkStruct_8020BD30 *PTR32 unk10;
    /* 0x14 */ struct UnkStruct_8020BD30 *PTR32 unk14;
    /* 0x18 */ u8 unk18;
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 unk1A;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
} UnkStruct_8020BD30;
SIZE_CHECK(UnkStruct_8020BD30, 0x3C);

/* C450.c's .bss; sorted with func_802595E0 by func_801F36B0. */
typedef struct UnkStruct_80218270 {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ Gfx *PTR32 unk4;
} UnkStruct_80218270;
SIZE_CHECK(UnkStruct_80218270, 8);

extern UnkStruct_8020BD30 D_8020BD30[7];
extern UnkStruct_8020BD30 D_8020BE98; /* D_8020BD30[6] */
extern UnkStruct_80218270 D_80218270[];

extern Mtx D_802182D0[2];
/* 1C40.c's .data: two texts per entry (the first is what 6790.c prints, byte by byte). */
#ifdef VERSION_EU
extern char *D_802081C0[0x1F][4];  /* eu: English, German, French (NULL) and no u16 text */
#else
extern char *D_802081C0[0x1F][2];
#endif

#endif
