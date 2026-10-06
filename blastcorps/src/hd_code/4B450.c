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
#include "game/objects.h"

extern s32 D_802E8BE8;
extern OSMesgQueue D_80370BF8;
extern u8 D_803A7424;
extern u8 D_803F932C;
extern Mtx D_02000000[];
extern u8 D_803F932D;
extern u8 D_803F932E;
extern s32 D_803A73F0;
extern s32 D_803A73F4;
extern s32 D_803A73F8;
extern s16 D_803A7410;
extern s16 D_803A7412;
extern s32 D_803F9320;
extern s32 D_803F9324;

void func_80260650(SndBank *, s32, SndState *PTR32 *);
void func_802608C8(SndState *);
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
TntCrate *func_8028DE94(void);
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
#ifdef TARGET_PC
        port_spin_wait();
#endif
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
