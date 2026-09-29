/*
 * The object after 48D00.c: eu pads its .text to 16 before func_8028FC10.
 * Its .data is the two bytes after 48D00's tables (and their padding).
 */
#include "common.h"
#include "game/audio.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ u8 unk6;
    /* 0x7 */ u8 unk7;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
} UnkStruct_8028D4C0; /* size = 0xC */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
} UnkStruct_802FDB98; /* size = 0x18 */

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u8 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ u8 unk18;
    /* 0x19 */ u8 unk19;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u8 unk22;
    /* 0x23 */ u8 unk23;
    /* 0x24 */ u8 unk24;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ Vtx *unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
} UnkStruct_8039B070; /* size = 0x48 */

extern s8 D_802E8BE4;
extern s32 D_802E8BE8;
extern OSMesgQueue D_80370BF8;
extern u8 D_803A7424;
extern u8 D_803F932C;
extern Mtx D_02000000[];
extern u8 D_803F932D;
extern u8 D_803F932E;
extern s32 D_80358060;
extern s8 D_803643D9;
extern s32 D_803A73F0;
extern s32 D_803A73F4;
extern s32 D_803A73F8;
extern s16 D_803A7410;
extern s16 D_803A7412;
extern s8 D_803ED40C;
extern s32 D_803F9320;
extern s32 D_803F9324;

void func_80260650(SndBank *, s32, s32 *);
void func_802608C8(s32);
s32 func_8026A6F0(s32, s32, s32, s32, s32, s32);
s32 func_802A0CC8(s32, s32);
void func_802AACD4(u8, s32, s32, s16 *, s16 *);
void func_802AAE1C(u8, s16, s16, s32 *, s32 *);
void func_802CDA10(s32, s32, s32);
s32 func_802CDB70(s16, s16);
void func_802CE4F0(s32, s32, s32);
s32 func_802CE6F8(s32, s32, s32);
void osContGetReadData(OSContPad *);
void func_8028DA5C(Vtx *, u8);
void func_8026AD30(s32);
s32 func_8029B930(void);
s16 func_802A6F6C(void);
void func_802CDAE8(s16, s16);
u8 func_802CDF94(s16);
s16 func_802CE3B8(s16);
void func_802CE5BC(s32, s32, s32, s16, s32, s32);
void func_802CE65C(s32, s32, s16, s16);
void func_802CE880(s32, s32, s32, s32, s32);
void func_802CE90C(s32);
s32 func_802CE958(s32);
f32 sqrtf(f32);
void func_8028DD64();
UnkStruct_8039B070 *func_8028DE94(void);
u8 func_8028FCD4(OSMesgQueue *, u8 *);


/* .data, 0x802FDBD0-0x802FDBE0 (tools/data_c.py) */
u8 D_802FDBD0 = 0;
u8 D_802FDBD4 = 0;

void func_8028FC10(void) {
    u8 sp3F;
    u8 sp3E;
    OSContPad sp24[4];

    sp3E = 0;
    osContStartReadData(&D_80370BF8);
    osRecvMesg(&D_80370BF8, NULL, OS_MESG_BLOCK);
    osContGetReadData(sp24);
    if (sp24[0].button & 0x1000) {
        sp3E = 1;
    }
    if (func_8028FCD4(&D_80370BF8, &sp3F) == 0 && (sp3F & 1)) {
        D_802FDBD0 = sp3E;
    }
    D_802FDBD4 = sp3E != 0 && D_802FDBD0 == 0;
}

u8 func_8028FCD4(OSMesgQueue *arg0, u8 *arg1) {
    OSContStatus sp20[4];
    s32 i;

    *arg1 = 0;
    osContStartQuery(arg0);
    while (arg0->validCount == 0) {
    }
    osRecvMesg(arg0, NULL, OS_MESG_NOBLOCK);
    osContGetQuery(sp20);
    for (i = 0; i < 4; i++) {
        if ((sp20[i].status & CONT_CARD_ON) && sp20[i].errno == 0) {
            *arg1 |= 1 << i;
        }
    }
    return sp20[0].errno;
}
