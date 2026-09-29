/*
 * The object after stats.c (7800.c): jp pads 7800's .text to 16 before
 * func_801EF380, and the double at 0x8020EFB0 after 7800's .rodata (and its
 * padding) is this object's, as are the .bss variables only it uses.
 */
#include "common.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

/* Per-frame buffer, double-buffered by D_8035805C. */
void func_801E8DCC(u8);
u8 func_801EEDB4(u8, u8, u8);
u8 func_801EF2BC(u16, u8, u8);
Gfx *func_801F4FBC(FrameBuf *, Gfx *);
void func_80260650(SndBank *, s32, s32);
void func_80264A34(char *, u16, s32);
u8 func_80272C5C(u16 *, u16 *, u8, u8, u8, f32);
void func_80284E54(Gfx *, s32, s32, s32, s32, s32);
u32 func_802852EC(void);
s32 func_80286038(u16);
void func_8028A3E4(void);
void func_8028A470(void);
void func_80295A20(s32);
void func_8029A7E4(char *, ...);

extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern char *D_802084D0[];
extern u16 *D_802084E0[];
extern Mtx D_802182D0[];
extern s32 D_802FA268;
extern FrameBuf D_803156F8[];
extern void *D_80358050[];
extern void *D_80358058;
extern u32 D_80358060;
extern void *D_8035806C;
extern s32 D_80358078;
extern u16 D_8035807C;
extern u8 D_803643D4;
extern u8 D_803643D5;
extern s32 D_803649F0;
extern char D_8036B980[];
extern char D_8036B9A8[];

/* .bss, 0x802159D0-0x802159F0 (tools/bss_c.py) */
u16 D_802159D0;
u8 *D_802159D4;
u8 *D_802159D8;
u16 D_802159DC;
f32 D_802159E0;
f32 D_802159E4;
u8 D_802159E8[8];

extern u8 D_0048F5A0[];
extern u8 D_0048F970[];
extern u8 D_0048F970_2[]; /* same address as D_0048F970: see undefined_syms */
extern u8 D_0048FA70[];
void func_801F4E70(s32);
void func_8028B4C4(u8 *romStart, u8 *dst, u32 *size, u8, u8, u8);

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/8380/func_801EF380.s")
#else
void func_801EF380(s32 arg0) {
    u32 sp24;
    u32 sp20;

    sp24 = D_0048F970 - D_0048F5A0;
    sp20 = D_0048FA70 - D_0048F970_2;
    func_801F4E70(arg0);
    if (arg0 == 2) {
        D_802159D0 = 90;
    } else {
        D_802159D0 = 0;
    }
    func_8028B4C4(D_0048F5A0, D_80358070, &sp24, 12, 0, 1);
    D_802159D4 = D_80358070;
    D_80358070 += sp24;
    func_8028B4C4(D_0048F970, D_80358070, &sp20, 12, 0, 1);
    D_802159D8 = D_80358070;
    D_80358070 += sp20;
    D_802159DC = arg0;
    D_802159E0 = 0.0f;
    D_802159E4 = 3.0f;
}
#endif

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/8380/func_801F4214_eu.s")
#else
void func_801EF4AC(void) {
    FrameBuf *sp12C;
    Gfx *gfx;
    s32 sp124;
    s32 sp120;
    s32 sp11C;
    s16 sp11A;

    sp12C = &D_803156F8[D_8035805C ^ 1];
    gfx = sp12C->dl;
    func_8028A470();
    func_80284E54(D_803156F8[D_8035805C].dl, D_80358078, 1, 1, 0x4D2, 0);
    D_8035805C ^= 1;
    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(sp12C));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000038);
    gSPDisplayList(gfx++, D_01000010);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gDPSetDepthImage(gfx++, D_80358058);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358058);
    gDPSetFillColor(gfx++, 0xFFFCFFFC);
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gDPPipeSync(gfx++);
    if (D_802159DC == 1) {
        if (D_80358060 * 185 / 20 > 185) {
            sp11A = 185;
        } else {
            sp11A = D_80358060 * 185 / 20;
        }
    } else {
        sp11A = 0;
    }
    gDPSetFillColor(gfx++, (GPACK_RGBA5551(sp11A, sp11A, sp11A, 1) << 16) | GPACK_RGBA5551(sp11A, sp11A, sp11A, 1));
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    func_8028A3E4();
    if (D_80358060 == 250) {
        if (D_80364A90 == 0x10) {
            D_80364A98 = 0x20;
        } else {
            D_80364A98 = 0x0400000000000000;
            if (D_802FA268 != 0) {
                func_80260650(D_80367738, 0x68, 0);
            }
        }
    }
    if (D_80358060 < 2) {
        guPerspective(&sp12C->mtx[73], &D_8035807C, 45.0f, 4.0f / 3.0f, 40.0f, 8000.0f, 0.25f);
        if (D_802159DC == 1) {
            guTranslate(&sp12C->mtx[74], 0.0f, -130.0f, 0.0f);
            guRotate(&sp12C->mtx[75], 35.0f, 0.1f, 0.0f, 0.0f);
        } else {
            guTranslate(&sp12C->mtx[74], 0.0f, 0.0f, 0.0f);
            guRotate(&sp12C->mtx[75], -10.0f, 0.1f, 0.0f, 0.0f);
        }
    }
    if (D_80358060 >= 20) {
        if (D_80358060 == 20 && D_802159DC == 1) {
            func_80260650(D_80367738, 0xBA, 0);
        } else if (D_80358060 == 20 && D_802159DC == 2) {
            func_80260650(D_80367738, 0xBD, 0);
        }
        if (D_80358060 < 80) {
            D_802159E0 = (80 - D_80358060) * 7600.0 / 60.0 + 400.0;
        }
        if (D_80358060 == 75 && D_802159DC == 2) {
            func_80260650(D_80367738, 0xB8, 0);
        } else if (D_80358060 == 75 && D_802159DC == 1) {
            func_80260650(D_80367738, 0xBB, 0);
        }
        guLookAtReflect(&sp12C->mtx[5], &sp12C->lookAt, 1.0f, 0.0f, D_802159E0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        D_802159D0 += D_802159E4;
        guRotate(&D_802182D0[D_8035805C], D_802159D0 % 360, 0.0f, 1.0f, 0.0f);
        guScale(&sp12C->mtx[76], 1.5f, 1.5f, 1.5f);
        gDPSetRenderMode(gfx++, G_RM_AA_ZB_OPA_INTER, G_RM_NOOP2);
        gfx = func_801F4FBC(sp12C, gfx);
    }
    if (D_80358060 > 80 && D_802159DC == 1) {
        gDPPipeSync(gfx++);
        gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetTexturePersp(gfx++, G_TP_NONE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, 0x28, 0x00, 0xFF, (D_80358060 * 6 - 480 < 255) ? D_80358060 * 6 - 480 : 255);
        sp120 = 26, sp11C = 42;
        for (sp124 = 0; sp124 < 256; sp124 += 32) {
            gDPLoadTextureTile(gfx++, D_802159D4, G_IM_FMT_IA, G_IM_SIZ_8b, 256, 0, sp124, 0, sp124 + 31, 31, 0,
                               G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPTextureRectangle(gfx++, (sp124 + sp120) << 2, sp11C << 2, (sp124 + sp120 + 32) << 2, (sp11C + 32) << 2,
                                G_TX_RENDERTILE, sp124 << 5, 0, 1 << 10, 1 << 10);
        }
        gDPPipeSync(gfx++);
        gDPSetPrimColor(gfx++, 0, 0, 0xFF, 0x00, 0x28, (D_80358060 * 4 - 320 < 255) ? D_80358060 * 4 - 320 : 255);
        gDPLoadTextureBlock(gfx++, D_802159D8, G_IM_FMT_IA, G_IM_SIZ_8b, 40, 24, 0, G_TX_CLAMP, G_TX_CLAMP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 256 << 2, 20 << 2, 296 << 2, 44 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
        gDPSetTexturePersp(gfx++, G_TP_PERSP);
    }
    if (D_80358060 >= 221) {
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, (D_80358060 - 220) * 255 / 30);
        gDPFillRectangle(gfx++, 0, 0, 319, 239);
    }
    gDPFullSync(gfx++);
    gSPEndDisplayList(gfx++);
    D_80358078 = gfx - sp12C->dl;
}
#endif
