#include "common.h"
#include "game/game.h"
#include "game/frame.h"
#include "functions.h"

extern u8 D_006A8DA0[];
extern u8 D_006A9F10[];

Gfx *func_8029700C(Gfx *gfx, s16 arg1, s16 arg2);

/* .bss, 0x8039CA90-0x8039CAB0 (tools/bss_c.py) */
u8 *D_8039CA90;
u8 *D_8039CA94;
u8 *D_8039CA98;
u8 *D_8039CA9C;
s16 D_8039CAA0;
u8 D_8039CAA2;

void func_80295E50(void) {
    s32 sp24;

    D_8039CA90 = D_80358070;
    D_8039CA94 = D_8039CA90 + 0x3200;
    D_8039CA98 = D_8039CA94 + 0xF0;
    D_8039CA9C = D_8039CA98 + 0x180;
    sp24 = ROM(D_006A9F10) - ROM(D_006A8DA0);
    func_8028B4C4(ROM(D_006A8DA0), D_80358070, &sp24, 0xA, 0, 1);
    D_80358070 += sp24;
    D_8039CAA0 = 0;
    D_8039CAA2 = 1;
}

Gfx *func_80295EFC(Frame *arg0, Gfx *arg1, s16 arg2, s16 arg3, u8 arg4) {
    Gfx *gfx = arg1;
    s32 sp118;

    gDPPipeSync(gfx++);
    gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_NOOP2);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, arg4 / 2);
    gfx = func_8029700C(gfx, arg2 - 4, arg3 + 4);
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, arg4);
    gfx = func_8029700C(gfx, arg2, arg3);
    if (D_80370C30.unk0 & 0x30) {
        gDPLoadTextureBlock(gfx++, D_8039CA98, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 6, 0,
                            G_TX_MIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_CLAMP, 5, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        if (D_80370C30.unk0 & 0x10) {
            gSPScisTextureRectangle(gfx++, (arg2 + 48) << 2, (arg3 + 8) << 2, (arg2 + 79) << 2, (arg3 + 13) << 2,
                                    G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
        }
        if (D_80370C30.unk0 & 0x20) {
            gSPScisTextureRectangle(gfx++, arg2 << 2, (arg3 + 8) << 2, (arg2 + 31) << 2, (arg3 + 13) << 2,
                                    G_TX_RENDERTILE, 32 << 5, 0, 1 << 10, 1 << 10);
        }
    }
    if (D_80370C30.unk0 & 0x2000) {
        gDPLoadTextureBlock(gfx++, D_8039CA9C, G_IM_FMT_RGBA, G_IM_SIZ_16b, 24, 24, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPScisTextureRectangle(gfx++, (arg2 + 9) << 2, (arg3 + 51) << 2, (arg2 + 32) << 2, (arg3 + 74) << 2,
                                G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    }
    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPLoadTextureBlock(gfx++, D_8039CA94, G_IM_FMT_IA, G_IM_SIZ_16b, 12, 10, 0,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                        G_TX_NOLOD, G_TX_NOLOD);
    sp118 = 0;
    do {
        u8 sp6F;
        u16 sp6C;
        u16 sp6A;

        sp6F = 1;
        if (D_80370C30.unk0 & (1 << sp118)) {
            gDPPipeSync(gfx++);
            switch (1 << sp118) {
                case 0x8000:
                    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0xFF, arg4);
                    sp6C = 57, sp6A = 30;
                    break;
                case 0x4000:
                    gDPSetPrimColor(gfx++, 0, 0, 0, 0xFF, 0, arg4);
                    sp6C = 52, sp6A = 25;
                    break;
                case 0x1000:
                    gDPSetPrimColor(gfx++, 0, 0, 0xFF, 0, 0, arg4);
                    sp6C = 35, sp6A = 27;
                    break;
                case 0x200:
                    gDPSetPrimColor(gfx++, 0, 0, 0x50, 0x50, 0x50, arg4);
                    sp6C = 4, sp6A = 22;
                    break;
                case 0x100:
                    gDPSetPrimColor(gfx++, 0, 0, 0x50, 0x50, 0x50, arg4);
                    sp6C = 16, sp6A = 22;
                    break;
                default:
                    sp6F = 0;
                    break;
            }
            if (sp6F != 0) {
                gSPScisTextureRectangle(gfx++, (arg2 + sp6C) << 2, (arg3 + sp6A) << 2,
                                        (arg2 + sp6C + 11) << 2, (arg3 + sp6A + 9) << 2,
                                        G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
            }
        }
    } while (++sp118 < 16);
    gDPPipeSync(gfx++);
    gDPSetPrimColor(gfx++, 0, 0, 0xDC, 0xDC, 0xDC, arg4);
    gSPScisTextureRectangle(gfx++, (arg2 + D_80370C30.unk2 / 18 + 35) << 2, (arg3 - D_80370C30.unk3 / 18 + 40) << 2,
                            (arg2 + D_80370C30.unk2 / 18 + 46) << 2, (arg3 - D_80370C30.unk3 / 18 + 49) << 2,
                            G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gDPPipeSync(gfx++);
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    return gfx;
}

Gfx *func_8029700C(Gfx *gfx, s16 arg1, s16 arg2) {
    s32 sp44;
    s32 sp40;

    sp44 = 0;
    do {
        sp40 = 0;
        do {
            gDPLoadTextureTile(gfx++, D_8039CA90, G_IM_FMT_RGBA, G_IM_SIZ_16b, 80, 80, sp44, sp40,
                               ((sp44 + 31 >= 80) ? 79 : sp44 + 31), ((sp40 + 31 >= 80) ? 79 : sp40 + 31), 0, G_TX_NOMIRROR | G_TX_CLAMP,
                               G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPScisTextureRectangle(gfx++, (arg1 + sp44) << 2, (arg2 + sp40) << 2,
                                    (((sp44 + 31 >= 80) ? 79 : sp44 + 31) + arg1) << 2, (((sp40 + 31 >= 80) ? 79 : sp40 + 31) + arg2) << 2,
                                    G_TX_RENDERTILE, sp44 << 5, sp40 << 5, 1 << 10, 1 << 10);
        } while ((sp40 += 31) < 80);
    } while ((sp44 += 31) < 80);
    return gfx;
}
