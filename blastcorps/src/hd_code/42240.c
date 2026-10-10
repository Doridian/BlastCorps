#include "common.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/frame.h"
#include "functions.h"

extern s16 D_80367BD6;
/* Segment 2 base, reached through a relocation, not a constant. */
extern Mtx D_02000000[];



/* .bss, 0x8036EC00-0x8036EC30 (tools/bss_c.py) */
u8 *D_8036EC00;
Vtx *D_8036EC04;
Mtx *D_8036EC08;
Mtx *D_8036EC0C;
s16 D_8036EC10;
u8 *D_8036EC14;
Vtx *D_8036EC18;
s16 D_8036EC1C;
u8 *D_8036EC20;
Vtx *D_8036EC24;
s16 D_8036EC28;

void func_80286A00(void) {
    D_80364A68 = 1;
    D_8036EC00 = D_80358070;
    func_802A0CC8(0x760, 0);
    D_8036EC08 = (Mtx *) D_80358070;
    D_80358070 += 0x80;
    D_8036EC0C = (Mtx *) D_80358070;
    D_80358070 += 0x80;
    D_8036EC04 = (Vtx *) D_80358070;
    D_80358070 += 0x80;
    D_8036EC04[0].v.ob[0] = 0x26;
    D_8036EC04[0].v.ob[1] = 0x7B;
    D_8036EC04[0].v.ob[2] = -5;
    D_8036EC04[0].v.tc[0] = 0;
    D_8036EC04[0].v.tc[1] = 0x4E0;
    D_8036EC04[1].v.ob[0] = 0x4D;
    D_8036EC04[1].v.ob[1] = 0x7B;
    D_8036EC04[1].v.ob[2] = -5;
    D_8036EC04[1].v.tc[0] = 0x4E0;
    D_8036EC04[1].v.tc[1] = 0x4E0;
    D_8036EC04[2].v.ob[0] = 0x4D;
    D_8036EC04[2].v.ob[1] = 0xAD;
    D_8036EC04[2].v.ob[2] = -5;
    D_8036EC04[2].v.tc[0] = 0x4E0;
    D_8036EC04[2].v.tc[1] = 0;
    D_8036EC04[3].v.ob[0] = 0x26;
    D_8036EC04[3].v.ob[1] = 0xAD;
    D_8036EC04[3].v.ob[2] = -5;
    D_8036EC04[3].v.tc[0] = 0;
    D_8036EC04[3].v.tc[1] = 0;
    D_8036EC04[4].v.ob[0] = -1;
    D_8036EC04[4].v.ob[1] = 0;
    D_8036EC04[4].v.ob[2] = -5;
    D_8036EC04[5].v.ob[0] = 1;
    D_8036EC04[5].v.ob[1] = 0;
    D_8036EC04[5].v.ob[2] = -5;
    D_8036EC04[6].v.ob[0] = -1;
    D_8036EC04[6].v.ob[1] = -0x11;
    D_8036EC04[6].v.ob[2] = -5;
    D_8036EC04[7].v.ob[0] = 1;
    D_8036EC04[7].v.ob[1] = -0x11;
    D_8036EC04[7].v.ob[2] = -5;
    D_8036EC10 = 0;
}

void func_80286C60(Gfx **arg0, Frame *arg1, u8 arg2, u8 arg3) {
    Gfx *gfx;

    gfx = *arg0;
    if (arg3 == 3 || D_8036EC10 != 0) {
        if (arg3 == 3 && D_8036EC10 < 0xFF) {
            D_8036EC10 += 10;
            if (D_8036EC10 >= 0x100) {
                D_8036EC10 = 0xFF;
            }
        }
        if (arg3 != 3 && D_8036EC10 != 0) {
            D_8036EC10 -= 10;
            if (D_8036EC10 < 0) {
                D_8036EC10 = 0;
            }
        }
        gSPMatrix(gfx++, &D_02000000[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, (D_8036EC10 < D_80367BD6) ? D_8036EC10 : D_80367BD6);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8036EC00), G_IM_FMT_RGBA, G_IM_SIZ_16b, 40, 40, 0,
                            G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPVertex(gfx++, OS_K0_TO_PHYSICAL(D_8036EC04), 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
        gDPPipeSync(gfx++);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetRenderMode(gfx++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        guTranslate(&D_8036EC0C[arg2], 56.0f, 151.0f, 0.0f);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&D_8036EC0C[arg2]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        guRotate(&D_8036EC08[arg2], (D_803EE3B1 / 100.0f) * 180.0 + 270.0, 0.0f, 0.0f, 1.0f);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&D_8036EC08[arg2]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        gSPVertex(gfx++, OS_K0_TO_PHYSICAL(&D_8036EC04[4]), 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 1, 2, 3, 0);
        gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
        gDPPipeSync(gfx++);
        *arg0 = gfx;
    }
}

void func_802873AC(void) {
    D_80364A6A = 1;
    D_8036EC14 = D_80358070;
    func_802A0CC8(0x996, 0);
    D_8036EC18 = (Vtx *) D_80358070;
    D_80358070 += 0x40;
    D_8036EC18[0].v.ob[0] = 0x22;
    D_8036EC18[0].v.ob[1] = 0x87;
    D_8036EC18[0].v.ob[2] = -5;
    D_8036EC18[0].v.tc[0] = 0;
    D_8036EC18[0].v.tc[1] = 0x3E0;
    D_8036EC18[1].v.ob[0] = 0x40;
    D_8036EC18[1].v.ob[1] = 0x87;
    D_8036EC18[1].v.ob[2] = -5;
    D_8036EC18[1].v.tc[0] = 0x3E0;
    D_8036EC18[1].v.tc[1] = 0x3E0;
    D_8036EC18[2].v.ob[0] = 0x40;
    D_8036EC18[2].v.ob[1] = 0xA5;
    D_8036EC18[2].v.ob[2] = -5;
    D_8036EC18[2].v.tc[0] = 0x3E0;
    D_8036EC18[2].v.tc[1] = 0;
    D_8036EC18[3].v.ob[0] = 0x22;
    D_8036EC18[3].v.ob[1] = 0xA5;
    D_8036EC18[3].v.ob[2] = -5;
    D_8036EC18[3].v.tc[0] = 0;
    D_8036EC18[3].v.tc[1] = 0;
    D_8036EC1C = 0;
}

void func_80287530(Gfx **arg0, Frame *arg1, s32 arg2, u8 arg3) {
    Gfx *gfx;
    s32 spB8[5];

    gfx = *arg0;
    if (arg3 == 10 || D_8036EC1C != 0) {
        if (arg3 == 10 && D_8036EC1C < 0xFF) {
            D_8036EC1C += 10;
            if (D_8036EC1C >= 0x100) {
                D_8036EC1C = 0xFF;
            }
        }
        if (arg3 != 10 && D_8036EC1C != 0) {
            D_8036EC1C -= 10;
            if (D_8036EC1C < 0) {
                D_8036EC1C = 0;
            }
        }
        gSPMatrix(gfx++, &D_02000000[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, (D_8036EC1C < D_80367BD6) ? D_8036EC1C : D_80367BD6);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8036EC14), G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                            G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPVertex(gfx++, OS_K0_TO_PHYSICAL(D_8036EC18), 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
        gDPPipeSync(gfx++);
        func_8026A378(D_803F8B72, spB8);
        func_80259DC8(arg1, spB8, 0, 1, 0, 0x40, 0x91, 15, 15, 1, 0xFF, 0xFF, 0, (D_8036EC1C < D_80367BD6) ? D_8036EC1C : D_80367BD6,
                      0xFF, 0, 0, (D_8036EC1C < D_80367BD6) ? D_8036EC1C : D_80367BD6);
        gDPPipeSync(gfx++);
        *arg0 = gfx;
    }
}

void func_80287AE4(void) {
    D_80364A6C = 1;
    D_8036EC20 = D_80358070;
    func_802A0CC8(0x998, 0);
    D_8036EC24 = (Vtx *) D_80358070;
    D_80358070 += 0x40;
    D_8036EC24[0].v.ob[0] = 0x22;
    D_8036EC24[0].v.ob[1] = 0x91;
    D_8036EC24[0].v.ob[2] = -5;
    D_8036EC24[0].v.tc[0] = 0;
    D_8036EC24[0].v.tc[1] = 0x3E0;
    D_8036EC24[1].v.ob[0] = 0x40;
    D_8036EC24[1].v.ob[1] = 0x91;
    D_8036EC24[1].v.ob[2] = -5;
    D_8036EC24[1].v.tc[0] = 0x3E0;
    D_8036EC24[1].v.tc[1] = 0x3E0;
    D_8036EC24[2].v.ob[0] = 0x40;
    D_8036EC24[2].v.ob[1] = 0xAF;
    D_8036EC24[2].v.ob[2] = -5;
    D_8036EC24[2].v.tc[0] = 0x3E0;
    D_8036EC24[2].v.tc[1] = 0;
    D_8036EC24[3].v.ob[0] = 0x22;
    D_8036EC24[3].v.ob[1] = 0xAF;
    D_8036EC24[3].v.ob[2] = -5;
    D_8036EC24[3].v.tc[0] = 0;
    D_8036EC24[3].v.tc[1] = 0;
    D_8036EC28 = 0;
}

void func_80287C68(Gfx **arg0, Frame *arg1, s32 arg2, u8 arg3) {
    Gfx *gfx;
    s32 spB8[5];

    gfx = *arg0;
    if (arg3 == 1 || D_8036EC28 != 0) {
        if (arg3 == 1 && D_8036EC28 < 0xFF) {
            D_8036EC28 += 10;
            if (D_8036EC28 >= 0x100) {
                D_8036EC28 = 0xFF;
            }
        }
        if (arg3 != 1 && D_8036EC28 != 0) {
            D_8036EC28 -= 10;
            if (D_8036EC28 < 0) {
                D_8036EC28 = 0;
            }
        }
        gSPMatrix(gfx++, &D_02000000[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, (D_8036EC28 < D_80367BD6) ? D_8036EC28 : D_80367BD6);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8036EC20), G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                            G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPVertex(gfx++, OS_K0_TO_PHYSICAL(D_8036EC24), 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
        gDPPipeSync(gfx++);
        func_8026A378(D_803EDC00, spB8);
        func_80259DC8(arg1, spB8, 0, 1, 0, 0x44, 0x97, 15, 15, 1, 0xFF, 0xFF, 0, (D_8036EC28 < D_80367BD6) ? D_8036EC28 : D_80367BD6,
                      0xFF, 0, 0, (D_8036EC28 < D_80367BD6) ? D_8036EC28 : D_80367BD6);
        gDPPipeSync(gfx++);
        *arg0 = gfx;
    }
}
