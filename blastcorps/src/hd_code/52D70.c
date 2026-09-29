#include "common.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

extern s32 D_802FA268;

u8 func_8029766C(u8 arg0, u8 *arg1);
s32 func_8026A6F0(s32, s32, s32, s32, s32, s32);

/* .bss, 0x8039CAB0-0x8039CAD0 (tools/bss_c.py) */
s16 D_8039CAB0;
s16 D_8039CAB2;
s16 D_8039CAB4;
u8 D_8039CAB6;
u8 D_8039CAB7;
u8 D_8039CAB8;
Gfx *D_8039CABC;
void *D_8039CAC0;
Mtx *D_8039CAC4;
u8 D_8039CAC8;

void func_80297530(u8 arg0) {
    u8 sp1F;
    u8 sp1E;

    sp1F = 0;
    if (D_80364A98 == 0x2000) {
        sp1F = func_8029766C(arg0, &sp1E);
    } else {
        sp1F = 0;
    }
    D_8039CAC8 = D_80364AF0[D_80364AE8].unk90;
    if (sp1F != 0) {
        D_8039CAB7 = 1;
        D_8039CAB0 = D_802E8F38[sp1E].x;
        D_8039CAB2 = D_802E8F38[sp1E].y;
        D_8039CAB4 = D_802E8F38[sp1E].z;
        D_8039CAB6 = sp1E;
        D_8039CAB8 = 0;
        D_8039CAC4 = (Mtx *)D_80358070;
        D_80358070 += 0x80;
        guTranslate(D_8039CAC4, 0.0f, 0.0f, 0.0f);
        guTranslate(D_8039CAC4 + 1, 0.0f, 0.0f, 0.0f);
    } else {
        D_8039CAB7 = 0;
    }
}

u8 func_8029766C(u8 arg0, u8 *arg1) {
    u8 sp7;
    s32 sp0;

    sp7 = 0;
    sp0 = 0;
    do {
        if (D_802E8F38[sp0].level == arg0) {
            sp7 = 1;
        } else {
            sp0++;
        }
    } while (sp7 == 0 && sp0 < 6);
    if (arg1 != NULL) {
        *arg1 = sp0;
    }
    return sp7;
}

void func_802976E8(Gfx **arg0) {
    Gfx *gfx = *arg0;

    if (D_8039CAB7 != 0) {
        gSPSegment(gfx++, 6, osVirtualToPhysical(D_8039CAC0));
        gSPSegment(gfx++, 7, osVirtualToPhysical(D_8039CAC4));
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPDisplayList(gfx++, osVirtualToPhysical(D_8039CABC));
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}

void func_80297804(s32 arg0, s32 arg1, s32 arg2) {
    if (D_8039CAB7 != 0 && D_80364AF0[D_80364AE8].gameState < 5) {
        if (D_8039CAB8 != 0) {
            if (func_8026A6F0(arg0 >> 5, arg1 >> 5, arg2 >> 5, D_8039CAB0, D_8039CAB2, D_8039CAB4) >= 0x8D) {
                D_8039CAB8 = 0;
            }
        } else if (func_8026A6F0(arg0 >> 5, arg1 >> 5, arg2 >> 5, D_8039CAB0, D_8039CAB2, D_8039CAB4) < 0x50) {
            D_8039CAB8 = 1;
            D_8039CAC8 |= 1 << D_8039CAB6;
            D_80364A98 = 0x1000000000;
        }
    }
}

void func_80297960(void) {
    if (D_802FA268 != 0 && (D_80370C28 & 0x2000) && D_8039CAB7 != 0) {
        D_8039CAC8 |= 1 << D_8039CAB6;
    }
    D_80364AF0[D_80364AE8].unk90 = D_8039CAC8;
}
