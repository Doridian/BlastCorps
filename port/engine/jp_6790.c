/*
 * hd_front_end 6790, jp's func_801ED800, which the C has as jp's
 * GLOBAL_ASM, as native C (engine.h; jp_E7B0.c says more).  The US
 * versions split the new rank's name into two lines of char text; jp shows
 * its u16 text (D_802081C0[rank][1]) as one.
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

    if (D_80370C28 & 0x8000) {
        if (!(D_80370C2A & 0x8000)) {
            if (D_802FA268 != 0) {
                D_80215960 = 2;
                D_80215964 = 3;
            }
        }
    }
    switch (D_80215960) {
        case 0:
            v = sins(D_80358060 * 0x4000 * 60 / 60 / 90);
            D_8021596C = (f64)v * 2.85 / 32767.0;
            if (D_8021596C >= 2.84) {
                D_8021596C = 2.84f;
                D_80215960 = 1;
                D_80215970 = 0x91;
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
        default:
            break;
    }
    switch (D_80215964) {
        case 0:
            if (D_80358060 == 50) {
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
            if (D_80358060 == 200) {
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
            if (D_80358060 == 0x122) {
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
        default:
            break;
    }
    if (D_80215974 <= 0) {
        if (D_80215976 <= 0)
            goto turn;
    }
    func_80259CCC(arg1, D_802084B0, D_802084B8, 0, 0x9C, 0, 0x18, 0x1A, 0x1A, 1, 0, 0, 0, D_80215974 / 2);
    func_80259CCC(arg1, D_802084B4, D_802084BC, 0, 0x9D, 0, 0xCB, 0x16, 0x16, 1, 0, 0, 0, D_80215974 / 2);
    rank = (u16 *)D_802081C0[D_80215978][1];
    func_80259CCC(arg1, NULL, rank, 0, 0x9D, 0, 0x67, 0x20, 0x20, 1, 0, 0, 0, D_80215976 / 2);
    dir = D_802084C0;
    h = D_802159B0 + dir * 15;
    D_802159B0 = h;
    if (h >= 0x100) {
        D_802159B0 = h - 30;
        D_802084C0 = -dir;
    }
    if (D_802159B0 < 0) {
        D_802159B0 += 30;
        D_802084C0 = -D_802084C0;
    }
    func_80259DC8(arg1, D_802084B0, D_802084B8, 0, 0xA0, 0, 0x14, 0x1A, 0x1A, 1, 0xFF, 0xFF - D_802159B0, 0,
                  D_80215974, 0xFF, D_802159B0, 0, D_80215974);
    func_80259DC8(arg1, D_802084B4, D_802084BC, 0, 0xA0, 0, 0xC8, 0x16, 0x16, 1, 0xFF, 0xFF - D_802159B0, 0,
                  D_80215974, 0xFF, D_802159B0, 0, D_80215974);
    rank = (u16 *)D_802081C0[D_80215978][1];
    func_80259DC8(arg1, NULL, rank, 0, 0xA0, 0, 0x64, 0x20, 0x20, 1, 0xFF, 0xFF - D_802159B0, 0, D_80215976, 0xFF,
                  D_802159B0, 0, D_80215976);
turn:
    D_80215968 += 12.0 - D_8021596C * 2.0f;
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
        gfx = func_801F4FBC(arg1, gfx);
    }
    *arg3 += gfx - arg0;
    return gfx;
}

#endif
