#include "common.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "functions.h"

extern u8 D_802FAD50[];

/* 0x1C-byte menu entries, laid out like YoshiEntry in hd_front_end/E7B0.c. */

extern u8 D_0068B550[];
extern u8 D_006A32B0[];

/* .bss, 0x8021AB90-0x8021ABB0 (tools/bss_c.py) */
u16 *D_8021AB90[4];
u8 D_8021ABA0;
u8 D_8021ABA1;
u8 D_8021ABA2;
s32 D_8021ABA4;
s32 D_8021ABA8;

extern char D_803048CC[];
extern char D_803048D8[];
extern char D_803048E4[];
extern char D_803048F0[];
/* .data, 0x8020E3E0-0x8020E440 (tools/data_c.py) */
s16 D_8020E3E0[4] = { 83, 129, 97, 180 };
#ifdef VERSION_EU
extern u8 D_80366F70_eu; /* the language: which of text, text2, text3 */
/* eu: each name once per language (the .rodata only eu has), and NULL for unk10 */
char *D_8020E9F8_eu[4][3] = {
    { "AMBER", "AMBER", "AMBER" },
    { "CLARK", "CLARK", "CLARK" },
    { "SPIKE", "SPIKE", "SPIKE" },
    { "WESLEY", "WESLEY", "WESLEY" },
};
char *D_8020EA28_eu[4] = { NULL, NULL, NULL, NULL };
#else
char D_8020E3E8[4][0x12] = { "AMBER", "CLARK", "SPIKE", "WESLEY" };
char *D_8020E430[4] = { D_803048CC, D_803048D8, D_803048E4, D_803048F0 };
#endif

/*
 * Loads the ROM range D_0068B550..D_006A32B0, splits it into four 160x120
 * RGBA16 images at D_8021AB90 and points D_8020C070[175] at the entries for
 * arg0 in D_8020E3E8/D_8020E430.
 */
void func_80201240(s32 arg0) {
    u32 sp24;
    s32 sp20;

    sp24 = D_006A32B0 - D_0068B550;
    func_8028B4C4(D_0068B550, D_80358070, &sp24, 13, 0, 1);
    for (sp20 = 0; sp20 < 4; sp20++) {
        D_8021AB90[sp20] = (u16 *)(sp20 * 160 * 120 * 2 + D_80358070);
    }
    D_80358070 += sp24;
    D_8021ABA0 = arg0;
    D_8021ABA2 = 0;
    D_8021ABA1 = 0;
#ifdef VERSION_EU
    (&D_8020C070[FE_ENTRY(175)].text)[D_80366F70_eu] = D_8020E9F8_eu[arg0][D_80366F70_eu];
    D_8020C070[FE_ENTRY(175)].unk10 = (u16 *)D_8020EA28_eu[arg0];
#else
    D_8020C070[FE_ENTRY(175)].text = D_8020E3E8[arg0];
    D_8020C070[FE_ENTRY(175)].unk10 = (u16 *)D_8020E430[arg0];
#endif
}

Gfx *func_80201364(s32 arg0, Gfx *arg1) {
    Gfx *gfx = arg1;
    s32 x;
    s32 y;
    s32 xoff;
    s32 yoff;

    gDPPipeSync(gfx++);
    gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPSetCycleType(gfx++, G_CYC_2CYCLE);
    gDPSetCombineLERP(gfx++, NOISE, 0, PRIMITIVE_ALPHA, 0, 0, 0, 0, ENVIRONMENT, TEXEL1, 0, ENV_ALPHA, COMBINED, 0,
                      0, 0, ENVIRONMENT);
    switch (D_8036BB1C) {
        case 1:
        case 4:
            D_8021ABA4 = 0;
            break;
        case 2:
            switch (D_8021ABA4) {
                case 0:
                    if (D_8021ABA1 + 3 >= 0x100) {
                        D_8021ABA1 = 0xFF;
                    } else {
                        D_8021ABA1 += 3;
                    }
                    if (D_8021ABA1 < 0x80) {
                        D_8021ABA2 = D_8021ABA1;
                    } else {
                        D_8021ABA2 = 0xFF - D_8021ABA1;
                    }
                    if (D_8021ABA1 == 0xFF) {
                        D_8021ABA4 = 1;
                        func_80260650(D_80367738, D_8020E3E0[D_8021ABA0], NULL);
                    }
                    break;
                case 1:
                    D_8021ABA2 = 0;
                    D_8021ABA1 = 0xFF;
                    if (func_8026A828(0, 0x28) == 0) {
                        D_8021ABA4 = 2;
                    }
                    break;
                case 2:
                    if (func_8026A828(0, 0x1E) == 0 || D_8021ABA2 == 0xFF) {
                        D_8021ABA4 = 3;
                    }
                    if (D_8021ABA2 + 0x18 >= 0x100) {
                        D_8021ABA2 = 0xFF;
                    } else {
                        D_8021ABA2 += 0x18;
                    }
                    D_8021ABA1 = 0xFF - D_8021ABA2;
                    break;
                case 3:
                    if (func_8026A828(0, 0x32) == 0) {
                        D_8021ABA4 = 2;
                    }
                    if (D_8021ABA2 - 0x20 < 0) {
                        D_8021ABA2 = 0;
                    } else {
                        D_8021ABA2 -= 0x20;
                    }
                    D_8021ABA1 = 0xFF - D_8021ABA2;
                    if (D_8021ABA2 == 0) {
                        D_8021ABA4 = 1;
                    }
                    break;
            }
            if (func_801E96F8() != 0) {
                func_8026AF6C(0x4000);
            }
            break;
        case 8:
            if (D_8021ABA1 - 6 < 0) {
                D_8021ABA1 = 0;
            } else {
                D_8021ABA1 -= 6;
            }
            if (D_8021ABA1 < 0x80) {
                D_8021ABA2 = D_8021ABA1;
            } else {
                D_8021ABA2 = 0xFF - D_8021ABA1;
            }
            break;
    }
    if (D_8021ABA2 >= 0x3D && D_8021ABA8 == 0) {
        func_80260650(D_80367738, 0x69, &D_8021ABA8);
    }
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_8021ABA2);
    gDPSetEnvColor(gfx++, 0, 0, 0, D_8021ABA1);
    gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    xoff = 80;
    for (x = 0; x < 160; x += 32) {
        yoff = 32;
        for (y = 0; y < 120; y += 8) {
            gDPLoadTextureTile(gfx++, D_8021AB90[D_8021ABA0], G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 120, x, y, x + 31,
                               y + 7, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPTextureRectangle(gfx++, (x + xoff) << 2, (y + yoff) << 2, (x + xoff + 32) << 2, (y + yoff + 8) << 2,
                                G_TX_RENDERTILE, x << 5, y << 5, 1 << 10, 1 << 10);
        }
    }
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetPrimColor(gfx++, 0, 0, 200, 200, 200, 255);
    gDPSetCombineLERP(gfx++, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE, 0, TEXEL0, 0, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE, 0,
                      TEXEL0, 0);
    gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_802FAD50), G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR, G_TX_NOMASK, 5, G_TX_NOLOD, G_TX_NOLOD);
    gSPTextureRectangle(gfx++, (xoff - 16) << 2, (yoff - 24) << 2, (xoff + 184) << 2, (yoff + 144) << 2,
                        G_TX_RENDERTILE, 0, 32 << 5, 0xA3, 0xC3);
    gDPPipeSync(gfx++);
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    return gfx;
}
