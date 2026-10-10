#include "common.h"
#include "functions.h"

/* The game's gbi.h predates the MAX(1, ...) guard in TXL2WORDS_4b. */
#undef TXL2WORDS_4b
#define TXL2WORDS_4b(txls) ((txls) / 16)

Gfx *func_80257540(Gfx *arg0) {
    gDPPipeSync(arg0++);
    gDPSetCycleType(arg0++, G_CYC_1CYCLE);
    gSPClearGeometryMode(arg0++, 0xFFFFFFFF);
    gSPSetGeometryMode(arg0++, G_SHADE | G_SHADING_SMOOTH | G_LOD);
    gSPTexture(arg0++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    return arg0;
}

#define FMT ((u8)(arg3 >> 8))

/* Draws a textured quad; arg3 holds the texture format (high byte) and
 * texel size (low byte). */
Gfx *func_802575F4(Gfx *arg0, void *arg1, void *arg2, s16 arg3, s32 arg4, s32 arg5, s32 arg6) {
    s32 spBC;

    gDPPipeSync(arg0++);
    if (arg6 != 0) {
        gSPSetGeometryMode(arg0++, G_ZBUFFER);
    } else {
        gSPClearGeometryMode(arg0++, G_ZBUFFER);
    }
    spBC = K0_TO_PHYS(arg2);
    gSPVertex(arg0++, K0_TO_PHYS(arg1), 4, 0);
    switch ((u8)arg3) {
        case G_IM_SIZ_4b:
            gDPLoadTextureBlock_4b(arg0++, spBC, FMT, arg4, arg5, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                   G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            break;
        case G_IM_SIZ_8b:
            gDPLoadTextureBlock(arg0++, spBC, FMT, G_IM_SIZ_8b, arg4, arg5, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            break;
        case G_IM_SIZ_16b:
            gDPLoadTextureBlock(arg0++, spBC, FMT, G_IM_SIZ_16b, arg4, arg5, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            break;
        case G_IM_SIZ_32b:
            gDPLoadTextureBlock(arg0++, spBC, FMT, G_IM_SIZ_32b, arg4, arg5, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            break;
    }
    switch (FMT) {
        case G_IM_FMT_RGBA:
            if (arg6 != 0) {
                gDPSetRenderMode(arg0++, G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2);
            } else {
                gDPSetRenderMode(arg0++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
            }
            gDPSetCombineMode(arg0++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
            break;
        case G_IM_FMT_IA:
            if (arg6 != 0) {
                gDPSetRenderMode(arg0++, G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2);
            } else {
                gDPSetRenderMode(arg0++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
            }
            gDPSetCombineMode(arg0++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
            break;
        case G_IM_FMT_I:
            if (arg6 != 0) {
                gDPSetRenderMode(arg0++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
            } else {
                gDPSetRenderMode(arg0++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
            }
            gDPSetCombineMode(arg0++, G_CC_MODULATEI, G_CC_MODULATEI);
            break;
    }
    gSP1Triangle(arg0++, 0, 1, 3, 0);
    gSP1Triangle(arg0++, 0, 2, 3, 0);
    return arg0;
}
