#include "common.h"
#include "game/frontend.h"
#include "game/frame.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

/* Per-frame buffer, double-buffered by D_8035805C. */
void func_801F4E70(s32);
Gfx *func_801F4FBC(FrameBuf *, Gfx *);
void func_80259CCC(FrameBuf *, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);

/* The video refresh rate, and a number of NTSC frames as many of the version's. */
#ifdef VERSION_EU
#define REFRESH_RATE 50
#else
#define REFRESH_RATE 60
#endif
#define FRAMES(n) ((n) * REFRESH_RATE / 60)

/* 1C40.c's texts and their numbers of words, in eu in the language. */
#ifdef VERSION_EU
#define TEXT_802081C0(i) ((u8 *)D_802081C0[i][D_80366F70_eu])
#define WORDS_802082B8(i) D_802082B8[D_80366F70_eu][i]
#else
#define TEXT_802081C0(i) ((u8 *)D_802081C0[i][0])
#define WORDS_802082B8(i) D_802082B8[i]
#endif

/* .bss, 0x80215960-0x802159C0 (tools/bss_c.py) */
s32 D_80215960;
s32 D_80215964;
f32 D_80215968;
f32 D_8021596C;
s32 D_80215970;
s16 D_80215974;
s16 D_80215976;
#ifdef VERSION_EU
char *D_802084B0; /* eu: in the language, set by func_801ED790 */
char *D_802084B4;
#endif
s32 D_80215978;
u8 D_8021597C[3];
u8 D_8021597F[1];
u8 D_80215980[0x18];
u8 D_80215998[0x18];
s16 D_802159B0;

void func_80259DC8(FrameBuf *, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32,
                   s32, s32, s32);
s32 func_8025B300(u8 *);

#ifdef VERSION_EU
extern u8 D_802082B8[][0x1F];
#else
extern u8 D_802082B8[];
#endif
extern char *D_802084B0;
extern char *D_802084B4;
extern u16 *D_802084B8;
extern u16 *D_802084BC;
extern s8 D_802084C0;
extern s32 D_80215960;
extern s32 D_80215964;
extern f32 D_80215968;
extern f32 D_8021596C;
extern s32 D_80215970;
extern s16 D_80215974;
extern s16 D_80215976;
extern s32 D_80215978;
extern u8 D_80215980[];
extern u8 D_80215998[];
extern s16 D_802159B0;
extern s32 D_802FA268;
extern u16 D_8035807C;
#ifdef VERSION_EU
extern u8 D_80366F70_eu; /* the language */
#endif

void func_801ED790(void) {
    func_801F4E70(0);
    D_80215960 = 0;
    D_80215964 = 0;
    D_80215968 = D_8021596C = 0.0f;
    D_80215974 = D_80215976 = 0;
    D_80215978 = D_80364AF0[D_80364AE8].unkC;
#ifdef VERSION_EU
    D_802084B0 = D_80366F70_eu == 0   ? "CONGRATULATIONS"
                 : D_80366F70_eu == 1 ? "GRATULATION ZUR"
                                      : NULL;
    D_802084B4 = D_80366F70_eu == 0   ? "ON YOUR PROMOTION!"
                 : D_80366F70_eu == 1 ? "BEFOERDERUNG!!!"
                                      : NULL;
#endif
}

#ifndef VERSION_EU
extern u16 D_80303B78[];
extern u16 D_80303B88[];
#endif
/* .data, 0x802084B0-0x802084D0 (tools/data_c.py) */
#ifdef VERSION_EU
u16 *D_802084B8 = NULL; /* eu has no u16 text */
u16 *D_802084BC = NULL;
#else
char *D_802084B0 = "CONGRATULATIONS";
char *D_802084B4 = "ON YOUR PROMOTION!";
u16 *D_802084B8 = D_80303B78;
u16 *D_802084BC = D_80303B88;
#endif
s8 D_802084C0 = 1;

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/6790/func_801ED800.s")
#else
Gfx *func_801ED800(Gfx *arg0, FrameBuf *arg1, u8 arg2, s32 *arg3) {
    Gfx *sp74;
    s32 sp70;
    s32 sp6C;
    PlayerInfo *sp68;
    s32 sp64;

    sp74 = arg0;
    if ((D_80370C28 & 0x8000) && !(D_80370C2A & 0x8000) && D_802FA268 != 0) {
        D_80215960 = 2;
        D_80215964 = 3;
    }
    switch (D_80215960) {
        case 0:
            D_8021596C = sins(D_80358060 * 0x4000 * 60 / REFRESH_RATE / 90) * 2.85 / 32767.0;
            if (D_8021596C >= 2.84) {
                D_8021596C = 2.84f;
                D_80215960 = 1;
                D_80215970 = 145;
            }
            break;
        case 1:
            if (D_80215970-- == 0) {
                D_80215960 = 2;
            }
            break;
        case 2:
            D_8021596C *= 0.9;
            break;
        case 3:
            break;
    }
    switch (D_80215964) {
        case 0:
            if (D_80358060 == FRAMES(50)) {
                D_80215964 = 1;
                D_80215974 = 0;
                D_80215976 = 0;
            }
            break;
        case 1:
            if (D_80215974 + 16 >= 0x100) {
                D_80215974 = 0xFF;
            } else {
                D_80215974 += 16;
            }
            if (D_80358060 == FRAMES(200)) {
                D_80215964 = 2;
            }
            break;
        case 2:
            if (D_80215974 - 32 < 0) {
                D_80215974 = 0;
            } else {
                D_80215974 -= 32;
            }
            if (D_80215976 + 16 >= 0x100) {
                D_80215976 = 0xFF;
            } else {
                D_80215976 += 16;
            }
            if (D_80358060 == FRAMES(290)) {
                D_80215964 = 3;
            }
            break;
        case 3:
            if (D_80215976 - 32 < 0) {
                D_80215976 = 0;
            } else {
                D_80215976 -= 32;
            }
            if (D_80215976 == 0) {
                D_80364A98 = 0x08000000;
            }
            break;
    }
    if (D_80215974 > 0 || D_80215976 > 0) {
        sp68 = &D_80364AF0[D_80364AE8];
        func_80259CCC(arg1, D_802084B0, D_802084B8, 0, 0x9C, 0, 0x18, 0x1A, 0x1A, 1, 0, 0, 0, D_80215974 / 2);
        func_80259CCC(arg1, D_802084B4, D_802084BC, 0, 0x9D, 0, 0xCB, 0x16, 0x16, 1, 0, 0, 0, D_80215974 / 2);
        for (sp70 = 0, sp6C = 0; sp6C < WORDS_802082B8(D_80215978); sp70++) {
            if ((D_80215980[sp70] = TEXT_802081C0(D_80215978)[sp70]) == ' ') {
                sp6C++;
            }
        }
        D_80215980[sp70 - 1] = 0;
        for (sp6C = sp70; TEXT_802081C0(D_80215978)[sp70] != 0; sp70++) {
            D_80215998[sp70 - sp6C] = TEXT_802081C0(D_80215978)[sp70];
        }
        D_80215998[sp70 - sp6C] = 0;
        if (func_8025B300(D_80215980) >= 14 || func_8025B300(D_80215998) >= 14) {
            sp64 = 29;
        } else {
            sp64 = 33;
        }
        func_80259CCC(arg1, (char *)D_80215980, NULL, 0, 0x9D, 0, 0x58, sp64, sp64, 1, 0, 0, 0, D_80215976 / 2);
        func_80259CCC(arg1, (char *)D_80215998, NULL, 0, 0x9D, 0, 0x76, sp64, sp64, 1, 0, 0, 0, D_80215976 / 2);
        D_802159B0 += D_802084C0 * 15;
        if (D_802159B0 >= 0x100) {
            D_802159B0 -= 30;
            D_802084C0 = -D_802084C0;
        }
        if (D_802159B0 < 0) {
            D_802159B0 += 30;
            D_802084C0 = -D_802084C0;
        }
        func_80259DC8(arg1, D_802084B0, D_802084B8, 0, 0xA0, 0, 0x14, 0x1A, 0x1A, 1, 0xFF, 0xFF - D_802159B0, 0,
                      D_80215974, 0xFF, D_802159B0, 0, D_80215974);
        func_80259DC8(arg1, D_802084B4, D_802084BC, 0, 0xA0, 0, 0xC8, 0x16, 0x16, 1, 0xFF, 0xFF - D_802159B0, 0,
                      D_80215974, 0xFF, D_802159B0, 0, D_80215974);
        func_80259DC8(arg1, (char *)D_80215980, NULL, 0, 0xA0, 0, 0x55, sp64, sp64, 1, 0xFF, 0xFF - D_802159B0, 0,
                      D_80215976, 0xFF, D_802159B0, 0, D_80215976);
        func_80259DC8(arg1, (char *)D_80215998, NULL, 0, 0xA0, 0, 0x73, sp64, sp64, 1, 0xFF, 0xFF - D_802159B0, 0,
                      D_80215976, 0xFF, D_802159B0, 0, D_80215976);
    }
    D_80215968 += 12.0 * 60 / REFRESH_RATE - D_8021596C * 2.0f;
    if (D_80215968 > 360.0) {
        D_80215968 -= 360.0;
    }
    if (D_80358060 < 2) {
        guPerspective(&arg1->mtx[73], &D_8035807C, 45.0f, 4.0f / 3.0f, 40.0f, 4000.0f, 1.0f);
        guLookAtReflect(&arg1->mtx[5], &arg1->lookAt, 5.0f, 7.0f, 400.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        guMtxIdent(&arg1->mtx[74]);
        guMtxIdent(&arg1->mtx[75]);
    }
    guScale(&arg1->mtx[76], D_8021596C / 8.0f, D_8021596C / 8.0f, D_8021596C / 8.0f);
    guRotate(&D_802182D0[arg2], D_80215968, 1.0f, 1.0f, 1.0f);
    if (D_8021596C > 0.2) {
        sp74 = func_801F4FBC(arg1, sp74);
    }
    *arg3 += sp74 - arg0;
    return sp74;
}
#endif
