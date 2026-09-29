#include "common.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

/*
 * This file's .bss: func_80274BF0 and func_80275270 store D_8036C778 (a u64)
 * through one shared lui, which IDO only does for a symbol defined in the
 * same file.
 */

/* .bss, 0x8036C770-0x8036C790 (tools/bss_c.py) */
u16 D_8036C770;
f32 D_8036C774;
u64 D_8036C778;
u32 D_8036C780;
u8 D_8036C784;

/* .data, 0x802FA8B0-0x802FA940 (tools/data_c.py) */
extern Vtx D_802FA8B0[2][4];
#ifdef ASSET_INIT
#include "hd_code/30430/D_802FA8B0.inc"
#else
Vtx D_802FA8B0[2][4] = { 0 };
#endif
f32 D_802FA930 = 8.0f;


void func_80275270(u64 arg0, f32 arg2);

extern s32 D_80358060;
extern u32 D_803156C4;
u16 func_8026B10C(void);
void func_8026AF6C(u16);

Gfx *func_80274BF0(u8 *arg0, Gfx *arg1) {
    Gfx *gfx;

    gfx = arg1;
    if (D_80358060 == 0) {
        if ((D_80364A90 & 0x4055800100040000) || ((D_80364A90 & 0x1801) && D_802E8BDC == 0x32)) {
            D_8036C784 = 0xFF;
            if (D_80364A90 & 0x0051800100040000) {
                D_8036C770 = func_8026B10C();
                func_8026AF6C(0);
            }
            D_8036C780 = D_803156C4;
        } else {
            D_8036C784 = 0;
        }
    }
    if (D_8036C778 != 0) {
        D_8036C784 = (255.0f < (D_803156C4 - D_8036C780) * D_8036C774) ? 255.0f : (D_803156C4 - D_8036C780) * D_8036C774;
        if (D_8036C784 == 0xFF) {
            D_80364A98 = D_8036C778;
            D_8036C778 = 0;
        }
    } else if (D_8036C784 != 0) {
        D_8036C784 = (0.0f > 255.0f - (D_803156C4 - D_8036C780) * D_802FA930) ? 0.0f : 255.0f - (D_803156C4 - D_8036C780) * D_802FA930;
        if (D_8036C784 == 0) {
            func_8026AF6C(D_8036C770);
            D_8036C770 = 0;
        }
    }
    if (D_8036C784 != 0) {
        gDPPipeSync(gfx++);
        gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_8036C784);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPFillRectangle(gfx++, 0, 0, 319, 239);
        osWritebackDCache(D_802FA8B0[D_8035805C], sizeof(D_802FA8B0[0]));
    }
    return gfx;
}

void func_8029A7E4(char *, ...);
void func_80261570(f32);

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/30430/func_80275270.s")
#else
void func_80275270(u64 arg0, f32 arg2) {
    if (D_8036C778 != 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!postFadeLoop_done", "fade.c", 100);
    }
    if (D_8036C778 == 0) {
        D_8036C778 = arg0;
        D_8036C774 = 4.25 / arg2;
        D_8036C780 = D_803156C4;
        if (!(arg0 & 0x40000000080004C2) && !(D_80364A90 & 0x4000000000040000)) {
            func_80261570(0.0f);
        }
    }
}
#endif

void func_80275390(u64 arg0) {
    func_80275270(arg0, 0.25f);
}

s32 func_802753C0(void) {
    return (D_8036C778 != 0) ? 1 : 0;
}

s32 func_802753F8(void) {
    return (D_8036C770 != 0) ? 1 : 0;
}
