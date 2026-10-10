#include "common.h"
#include "game/game.h"
#include "game/frame.h"
#include "functions.h"

extern u8 D_0048FA70[];
extern u8 D_0048FE90[];

/* .bss, 0x803A6B10-0x803A6B20 (tools/bss_c.py) */
u8 *D_803A6B10;
s16 D_803A6B14;

void func_8029A130(void) {
    s32 sp24;

    sp24 = ROM(D_0048FE90) - ROM(D_0048FA70);
    func_8028B4C4(ROM(D_0048FA70), D_80358070, &sp24, 0xC, 0, 1);
    D_803A6B10 = D_80358070;
    D_80358070 += sp24;
    D_803A6B14 = 0;
}

Gfx *func_8029A1A8(Frame *arg0, Gfx *arg1) {
    Gfx *gfx = arg1;

    gDPPipeSync(gfx++);
    gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    if (D_803A6B14 + 0x40 >= 0x100) {
        D_803A6B14 = 0xFF;
    } else {
        D_803A6B14 += 0x40;
    }
    gDPSetPrimColor(gfx++, 0, 0, 0xFF, 0xFF, 0xFF, D_803A6B14);
    gDPLoadTextureBlock(gfx++, D_803A6B10, G_IM_FMT_IA, G_IM_SIZ_8b, 256, 16, 0,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                        G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPTextureRectangle(gfx++, 32 << 2, 208 << 2, 288 << 2, 224 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gDPPipeSync(gfx++);
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    return gfx;
}
