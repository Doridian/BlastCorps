/*
 * hd_front_end 6790, jp's func_801ED800, which the C has as jp's
 * GLOBAL_ASM, as native C charged by the original's blocks (engine.h;
 * jp_E7B0.c says more).  The US versions split the new rank's name into two
 * lines of char text; jp shows its u16 text (D_802081C0[rank][1]) as one.
 */
#include "engine.h"
#include "game/frontend.h"
#include "game/frame.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

#ifdef VERSION_JP

extern s32 D_80215960;
extern s32 D_80215964;
extern f32 D_80215968;
extern f32 D_8021596C;
extern s32 D_80215970;
extern s16 D_80215974;
extern s16 D_80215976;
extern s32 D_80215978;
extern s16 D_802159B0;
extern char *D_802084B0;
extern char *D_802084B4;
extern u16 *D_802084B8;
extern u16 *D_802084BC;
extern s8 D_802084C0;
extern s32 D_802FA268;
extern u16 D_8035807C;

Gfx *func_801F4FBC(FrameBuf *, Gfx *);
void func_80259CCC(FrameBuf *, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
void func_80259DC8(FrameBuf *, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32,
                   s32, s32, s32);

/* the promotion screen: the badge growing in and turning, and the
   congratulations and the new rank fading in and out */
Gfx *func_801ED800(Gfx *arg0, FrameBuf *arg1, u8 arg2, s32 *arg3) {
    Gfx *gfx = arg0;
    s32 v;
    s16 h;
    s8 dir;
    u16 *rank;

    ENGINE_BLK(801ED800);
    if (D_80370C28 & 0x8000) {
        ENGINE_BLK(801ED834);
        if (!(D_80370C2A & 0x8000)) {
            ENGINE_BLK(801ED848);
            if (D_802FA268 != 0) {
                ENGINE_BLK(801ED858);
                D_80215960 = 2;
                D_80215964 = 3;
            }
        }
    }
    ENGINE_BLK(801ED870);
    switch (D_80215960) {
        case 0:
            ENGINE_BLK(801ED8A0);
            v = sins(D_80358060 * 0x4000 * 60 / 60 / 90);
            ENGINE_BLK(801ED8E8);
            D_8021596C = (f64)v * 2.85 / 32767.0;
            if (D_8021596C >= 2.84) {
                ENGINE_BLK(801ED930);
                D_8021596C = 2.84f;
                D_80215960 = 1;
                D_80215970 = 0x91;
            }
            break;
        case 1:
            ENGINE_BLK(801ED880);
            ENGINE_BLK(801ED958);
            if (D_80215970-- == 0) {
                ENGINE_BLK(801ED974);
                D_80215960 = 2;
            }
            break;
        case 2:
            ENGINE_BLK(801ED880);
            ENGINE_BLK(801ED888);
            ENGINE_BLK(801ED984);
            D_8021596C *= 0.9;
            break;
        case 3:
            ENGINE_BLK(801ED880);
            ENGINE_BLK(801ED888);
            ENGINE_BLK(801ED890);
            break;
        default:
            ENGINE_BLK(801ED880);
            ENGINE_BLK(801ED888);
            ENGINE_BLK(801ED890);
            ENGINE_BLK(801ED898);
            break;
    }
    ENGINE_BLK(801ED9A8);
    switch (D_80215964) {
        case 0:
            ENGINE_BLK(801ED9D8);
            if (D_80358060 == 50) {
                ENGINE_BLK(801ED9EC);
                D_80215964 = 1;
                D_80215974 = 0;
                D_80215976 = 0;
            }
            break;
        case 1:
            ENGINE_BLK(801ED9B8);
            ENGINE_BLK(801EDA0C);
            if (D_80215974 + 16 >= 0x100) {
                ENGINE_BLK(801EDA24);
                D_80215974 = 0xFF;
            } else {
                ENGINE_BLK(801EDA34);
                D_80215974 += 16;
            }
            ENGINE_BLK(801EDA48);
            if (D_80358060 == 200) {
                ENGINE_BLK(801EDA5C);
                D_80215964 = 2;
            }
            break;
        case 2:
            ENGINE_BLK(801ED9B8);
            ENGINE_BLK(801ED9C0);
            ENGINE_BLK(801EDA6C);
            if (D_80215974 - 32 < 0) {
                ENGINE_BLK(801EDA80);
                D_80215974 = 0;
            } else {
                ENGINE_BLK(801EDA88);
                D_80215974 -= 32;
            }
            ENGINE_BLK(801EDA9C);
            if (D_80215976 + 16 >= 0x100) {
                ENGINE_BLK(801EDAB4);
                D_80215976 = 0xFF;
            } else {
                ENGINE_BLK(801EDAC4);
                D_80215976 += 16;
            }
            ENGINE_BLK(801EDAD8);
            if (D_80358060 == 0x122) {
                ENGINE_BLK(801EDAEC);
                D_80215964 = 3;
            }
            break;
        case 3:
            ENGINE_BLK(801ED9B8);
            ENGINE_BLK(801ED9C0);
            ENGINE_BLK(801ED9C8);
            ENGINE_BLK(801EDAFC);
            if (D_80215976 - 32 < 0) {
                ENGINE_BLK(801EDB10);
                D_80215976 = 0;
            } else {
                ENGINE_BLK(801EDB18);
                D_80215976 -= 32;
            }
            ENGINE_BLK(801EDB2C);
            if (D_80215976 == 0) {
                ENGINE_BLK(801EDB3C);
                D_80364A98 = 0x08000000;
            }
            break;
        default:
            ENGINE_BLK(801ED9B8);
            ENGINE_BLK(801ED9C0);
            ENGINE_BLK(801ED9C8);
            ENGINE_BLK(801ED9D0);
            break;
    }
    ENGINE_BLK(801EDB54);
    if (D_80215974 <= 0) {
        ENGINE_BLK(801EDB64);
        if (D_80215976 <= 0)
            goto turn;
    }
    ENGINE_BLK(801EDB74);
    if (D_80215974 < 0)
        ENGINE_BLK(801EDBF0);
    ENGINE_BLK(801EDBF8);
    func_80259CCC(arg1, D_802084B0, D_802084B8, 0, 0x9C, 0, 0x18, 0x1A, 0x1A, 1, 0, 0, 0, D_80215974 / 2);
    ENGINE_BLK(801EDC00);
    if (D_80215974 < 0)
        ENGINE_BLK(801EDC60);
    ENGINE_BLK(801EDC68);
    func_80259CCC(arg1, D_802084B4, D_802084BC, 0, 0x9D, 0, 0xCB, 0x16, 0x16, 1, 0, 0, 0, D_80215974 / 2);
    ENGINE_BLK(801EDC70);
    rank = (u16 *)D_802081C0[D_80215978][1];
    if (D_80215976 < 0)
        ENGINE_BLK(801EDCDC);
    ENGINE_BLK(801EDCE4);
    func_80259CCC(arg1, NULL, rank, 0, 0x9D, 0, 0x67, 0x20, 0x20, 1, 0, 0, 0, D_80215976 / 2);
    ENGINE_BLK(801EDCEC);
    dir = D_802084C0;
    h = D_802159B0 + dir * 15;
    D_802159B0 = h;
    if (h >= 0x100) {
        ENGINE_BLK(801EDD24);
        D_802159B0 = h - 30;
        D_802084C0 = -dir;
    }
    ENGINE_BLK(801EDD44);
    if (D_802159B0 < 0) {
        ENGINE_BLK(801EDD54);
        D_802159B0 += 30;
        D_802084C0 = -D_802084C0;
    }
    ENGINE_BLK(801EDD74);
    func_80259DC8(arg1, D_802084B0, D_802084B8, 0, 0xA0, 0, 0x14, 0x1A, 0x1A, 1, 0xFF, 0xFF - D_802159B0, 0,
                  D_80215974, 0xFF, D_802159B0, 0, D_80215974);
    ENGINE_BLK(801EDDFC);
    func_80259DC8(arg1, D_802084B4, D_802084BC, 0, 0xA0, 0, 0xC8, 0x16, 0x16, 1, 0xFF, 0xFF - D_802159B0, 0,
                  D_80215974, 0xFF, D_802159B0, 0, D_80215974);
    ENGINE_BLK(801EDE84);
    rank = (u16 *)D_802081C0[D_80215978][1];
    func_80259DC8(arg1, NULL, rank, 0, 0xA0, 0, 0x64, 0x20, 0x20, 1, 0xFF, 0xFF - D_802159B0, 0, D_80215976, 0xFF,
                  D_802159B0, 0, D_80215976);
turn:
    ENGINE_BLK(801EDF18);
    D_80215968 += 12.0 - D_8021596C * 2.0f;
    if (D_80215968 > 360.0) {
        ENGINE_BLK(801EDF74);
        D_80215968 -= 360.0;
    }
    ENGINE_BLK(801EDF88);
    if (D_80358060 < 2) {
        ENGINE_BLK(801EDF9C);
        guPerspective(&arg1->mtx[73], &D_8035807C, 45.0f, 4.0f / 3.0f, 40.0f, 4000.0f, 1.0f);
        ENGINE_BLK(801EDFDC);
        guLookAtReflect(&arg1->mtx[5], &arg1->lookAt, 5.0f, 7.0f, 400.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        ENGINE_BLK(801EE034);
        guMtxIdent(&arg1->mtx[74]);
        ENGINE_BLK(801EE040);
        guMtxIdent(&arg1->mtx[75]);
    }
    ENGINE_BLK(801EE04C);
    guScale(&arg1->mtx[76], D_8021596C / 8.0f, D_8021596C / 8.0f, D_8021596C / 8.0f);
    ENGINE_BLK(801EE07C);
    guRotate(&D_802182D0[arg2], D_80215968, 1.0f, 1.0f, 1.0f);
    ENGINE_BLK(801EE0B0);
    if (D_8021596C > 0.2) {
        ENGINE_BLK(801EE0D4);
        gfx = func_801F4FBC(arg1, gfx);
        ENGINE_BLK(801EE0E0);
    }
    ENGINE_BLK(801EE0E4);
    *arg3 += gfx - arg0;
    return gfx;
}

#endif
