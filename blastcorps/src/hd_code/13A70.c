#include "common.h"
#include "game/frame.h"
#include "game/level.h"
#include "game/player.h"

extern s32 D_803EBBF8;

void func_80284E54(Gfx *, s32, s32, s32, s32, s32);

u8 func_8027EED8(s32, s32, s16 *);
u8 func_802ABEDC(s32, s32, s32);

/* .bss, 0x803650B0-0x80365340 (tools/bss_c.py) */
Gfx D_803650B0[0x28];
Mtx D_803651F0;
Mtx D_80365230;
Mtx D_80365270;
Mtx D_803652B0;
Mtx D_803652F0;
u8 *D_80365330;

/* .data, 0x802E8C60-0x802E8C70 (tools/data_c.py) */
Vp D_802E8C60 = { { { 128, 128, 511 }, { 128, 128, 511 } } };

void func_80258230(u8 arg0, s32 arg1, s16 arg2, s16 arg3) {
    D_803643CC->unk1022 = arg0;
    D_803643CC->unk1023 = 0;
    D_803643CC->unk1000 = arg1;
    D_803643CC->unk1018 = arg2;
    D_803643CC->unk101A = arg3;
    D_803643CC->unk101C = 0;
    D_803643CC->unk101E = 0;
    D_803643CC->unk1020 = 0;
    D_803643CC++;
}

void func_802582C4(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7) {
    u8 sp37;
    UnkStruct_803643C8 *sp30;
    u8 sp2F;
    u8 sp2E;
    u8 sp2D;
    s16 sp2A;
    s32 sp24;
    s32 sp20;
    s32 sp1C;

    sp37 = 0;
    sp2F = 0;
    sp2E = 0;
    if (arg0 == 9) {
        sp2F = func_802ABEDC(arg1, arg4, arg3);
        if (sp2F != 0) {
            sp2E = 1;
        }
        sp2D = func_8027EED8(arg1 >> 5, arg3 >> 5, &sp2A);
        if (sp2D != 0) {
            sp24 = arg4 - (sp2A << 5);
            sp1C = arg4 - arg2;
            if (sp2F != 0) {
                sp20 = arg4 - D_803EBBF8;
            } else {
                sp20 = 9999999;
            }
            if ((sp24 > 0) && (sp24 < sp20) && (sp24 < sp1C)) {
                D_803EBBF8 = sp2A << 5;
                sp2E = 2;
            }
        }
    }
    sp30 = D_803643C8;
    while (sp37 == 0) {
        if (sp30->unk1022 == arg0) {
            sp30->unk1004 = arg1;
            if (sp2E != 0) {
                sp30->unk1008 = D_803EBBF8;
            } else {
                sp30->unk1008 = arg2;
            }
            sp30->unk100C = arg3;
            sp30->unk1014 = arg2;
            sp30->unk101C = arg5;
            sp30->unk101E = arg7;
            sp30->unk1020 = arg6;
            sp30->unk1010 = arg4;
            sp30->unk1023 = sp2E;
            sp37 = 1;
        } else {
            sp30++;
        }
    }
}

s32 func_802584BC(u8 arg0) {
    UnkStruct_803643C8 *sp4 = D_803643C8;

    for (;;) {
        if (sp4->unk1022 == arg0) {
            return sp4->unk1008;
        }
        sp4++;
    }
}

s32 func_80258500(u8 arg0) {
    UnkStruct_803643C8 *sp4 = D_803643C8;

    for (;;) {
        if (sp4->unk1022 == arg0) {
            return sp4->unk1014;
        }
        sp4++;
    }
}

/*
 * Renders dl into a 64x64 8-bit image, seen from 45 degrees at distance
 * dist, looking at (x, y, z) / 32 in world units.
 */
void func_80258544(void *image, s32 x, s32 y, s32 z, f32 dist, Gfx *dl, void *seg6, void *seg7) {
    Gfx *gfx;
    u16 perspNorm;
    s32 unused;

    gfx = D_803650B0;
    gSPViewport(gfx++, K0_TO_PHYS(&D_802E8C60));
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 6, K0_TO_PHYS(seg6));
    gSPSegment(gfx++, 7, K0_TO_PHYS(seg7));
    gDPPipeSync(gfx++);
    gDPSetScissor(gfx++, G_SC_NON_INTERLACE, 0, 0, 63, 63);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, 255);
    gDPSetColorDither(gfx++, G_CD_DISABLE);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gDPSetColorImage(gfx++, G_IM_FMT_CI, G_IM_SIZ_8b, 64, K0_TO_PHYS(image));
    gDPSetFillColor(gfx++, 0x00010001);
    gDPFillRectangle(gfx++, 0, 0, 63, 63);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_PASS, GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    guPerspective(&D_803651F0, &perspNorm, 45.0f, 1.0f, 1.0f, 1000.0f, 1.0f);
    gSPMatrix(gfx++, K0_TO_PHYS(&D_803651F0), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(gfx++, perspNorm);
    guTranslate(&D_803652F0, 0.0f, 0.0f, -dist);
    gSPMatrix(gfx++, K0_TO_PHYS(&D_803652F0), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    guRotate(&D_80365270, -90.0f, 1.0f, 0.0f, 0.0f);
    gSPMatrix(gfx++, K0_TO_PHYS(&D_80365270), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    guRotate(&D_803652B0, 180.0f, 0.0f, 1.0f, 0.0f);
    gSPMatrix(gfx++, K0_TO_PHYS(&D_803652B0), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    guTranslate(&D_80365230, -(x / 32.0f), -(y / 32.0f), -(z / 32.0f));
    gSPMatrix(gfx++, K0_TO_PHYS(&D_80365230), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gSPDisplayList(gfx++, K0_TO_PHYS(dl));
    gSPEndDisplayList(gfx++);
    osWritebackDCache(&D_803651F0, 5 * sizeof(Mtx));
    func_80284E54(D_803650B0, gfx - D_803650B0, 1, 0, 0x61F, 0);
}

/* 0xFC-byte records, D_803F4030 up to D_803F7654. */
typedef struct {
    /* 0x00 */ u8 pad0[0x10];
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ u8 pad1C[0x14];
    /* 0x30 */ s32 unk30;
    /* 0x34 */ u8 pad34[0x10];
    /* 0x44 */ s32 unk44;
    /* 0x48 */ u8 pad48[0xA2];
    /* 0xEA */ u8 unkEA;
    /* 0xEB */ u8 padEB[0x11];
} UnkStruct_803F4030; /* size = 0xFC */

/* The per-frame buffer (UnkStruct_02000000 in 00000.c); only the part used here. */
extern UnkStruct_803F4030 D_803F4030[];
extern UnkStruct_803F4030 *D_803F7654;
extern FrameGame D_02000000;
extern u8 *D_80365330;

void func_80258B78(Gfx **arg0, FrameGame *arg1) {
    Gfx *gfx;
    s32 sp70;
    s32 sp6C;
    s32 sp68;
    s16 sp66;
    s16 sp64;
    s16 sp62;
    s16 sp60;

    gfx = *arg0;
    sp70 = 0;
    sp6C = 0;
    sp60 = 0;
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, 200, 200, 200, 200);
    while (&D_803F4030[sp70] != D_803F7654) {
        if (D_803F4030[sp70].unkEA == 0) {
            switch (D_803F4030[sp70].unk30) {
                case 0xBA:
                case 0xBB:
                case 0xBC:
            sp68 = 60 - (D_803F4030[sp70].unk14 - D_803F4030[sp70].unk44) / 800;
            if (sp68 < 0) {
                sp68 = 0;
            }
            sp66 = D_803F4030[sp70].unk10 >> 5;
            sp64 = D_803F4030[sp70].unk44 >> 5;
            sp62 = D_803F4030[sp70].unk18 >> 5;
            arg1->unk1900[0x200 + sp6C].v.ob[0] = sp66 - sp68;
            arg1->unk1900[0x200 + sp6C].v.ob[1] = sp64;
            arg1->unk1900[0x200 + sp6C].v.ob[2] = sp62 - sp68;
            arg1->unk1900[0x200 + sp6C].v.tc[0] = 0;
            arg1->unk1900[0x200 + sp6C].v.tc[1] = 0;
            sp6C++;
            arg1->unk1900[0x200 + sp6C].v.ob[0] = sp66 + sp68;
            arg1->unk1900[0x200 + sp6C].v.ob[1] = sp64;
            arg1->unk1900[0x200 + sp6C].v.ob[2] = sp62 - sp68;
            arg1->unk1900[0x200 + sp6C].v.tc[0] = 0x7E0;
            arg1->unk1900[0x200 + sp6C].v.tc[1] = 0;
            sp6C++;
            arg1->unk1900[0x200 + sp6C].v.ob[0] = sp66 + sp68;
            arg1->unk1900[0x200 + sp6C].v.ob[1] = sp64;
            arg1->unk1900[0x200 + sp6C].v.ob[2] = sp62 + sp68;
            arg1->unk1900[0x200 + sp6C].v.tc[0] = 0x7E0;
            arg1->unk1900[0x200 + sp6C].v.tc[1] = 0x7E0;
            sp6C++;
            arg1->unk1900[0x200 + sp6C].v.ob[0] = sp66 - sp68;
            arg1->unk1900[0x200 + sp6C].v.ob[1] = sp64;
            arg1->unk1900[0x200 + sp6C].v.ob[2] = sp62 + sp68;
            arg1->unk1900[0x200 + sp6C].v.tc[0] = 0;
            arg1->unk1900[0x200 + sp6C].v.tc[1] = 0x7E0;
            sp6C++;
            if (sp60 == 0) {
                gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_80365330), G_IM_FMT_IA, G_IM_SIZ_8b, 64, 64, 0,
                                    G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK,
                                    G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                sp60 = 1;
            }
            gDPPipeSync(gfx++);
            if (D_802E8BDC == 16 && sp66 == 0x19B7 && sp62 == 0xF99) {
                gSPSetGeometryMode(gfx++, G_ZBUFFER);
                gDPSetRenderMode(gfx++, G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2);
            } else {
                gSPClearGeometryMode(gfx++, G_ZBUFFER);
                gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
            }
            gSPVertex(gfx++, &D_02000000.unk1900[0x200 + sp6C - 4], 4, 0);
            gSP1Triangle(gfx++, 0, 1, 3, 0);
            gSP1Triangle(gfx++, 1, 2, 3, 0);
            break;
            }
        }
        sp70++;
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}
