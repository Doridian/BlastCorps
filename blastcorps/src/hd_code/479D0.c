#include "common.h"
#include "game/vehicle.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/objects.h"
#include "functions.h"


void func_8028C41C(Vtx *, u8, s16, s16, s16);

/* .bss, 0x8039AF00-0x8039B070 (tools/bss_c.py) */
AmmoBox D_8039AF00[AMMO_BOX_MAX];
s32 D_8039B068;

/* .data, 0x802FDB40-0x802FDB70 (tools/data_c.py) */
AmmoBoxInfo D_802FDB40[2] = {
    { -10, 10, 0, 20, -10, 10, 2453, 2453, 640, 100, 10 },
    { -10, 10, 0, 20, -10, 10, 2455, 2455, 640, 100, 1 },
};

void func_8028C190(LevelAmmoBox *arg0, LevelAmmoBox *arg1) {
    D_8039B068 = 0;
    while (arg0 != arg1) {
        D_8039AF00[D_8039B068].x = arg0->x;
        D_8039AF00[D_8039B068].y = arg0->y;
        D_8039AF00[D_8039B068].z = arg0->z;
        D_8039AF00[D_8039B068].type = arg0->type;
        D_8039AF00[D_8039B068].collected = 0;
        D_8039AF00[D_8039B068].alpha = 0xFF;
        D_8039AF00[D_8039B068].unkC = (u8 *)func_802A0CC8(D_802FDB40[D_8039AF00[D_8039B068].type].unkC, 0);
        D_8039AF00[D_8039B068].unk10 = (u8 *)func_802A0CC8(D_802FDB40[D_8039AF00[D_8039B068].type].unkE, 0);
        D_8039AF00[D_8039B068].vtx = (Vtx *)D_80358070;
        D_80358070 += 0x80;
        func_8028C41C(D_8039AF00[D_8039B068].vtx, D_8039AF00[D_8039B068].type, D_8039AF00[D_8039B068].x,
                      D_8039AF00[D_8039B068].y, D_8039AF00[D_8039B068].z);
        D_8039B068++;
        arg0++;
    }
}

void func_8028C41C(Vtx *arg0, u8 arg1, s16 arg2, s16 arg3, s16 arg4) {
    arg0[0].v.ob[0] = D_802FDB40[arg1].unk2 + arg2;
    arg0[0].v.ob[1] = D_802FDB40[arg1].unk4 + arg3;
    arg0[0].v.ob[2] = D_802FDB40[arg1].unk8 + arg4;
    arg0[0].v.tc[1] = 0;
    arg0[0].v.tc[0] = 0;
    arg0[1].v.ob[0] = D_802FDB40[arg1].unk2 + arg2;
    arg0[1].v.ob[1] = D_802FDB40[arg1].unk6 + arg3;
    arg0[1].v.ob[2] = D_802FDB40[arg1].unk8 + arg4;
    arg0[1].v.tc[1] = 0x1E0;
    arg0[1].v.tc[0] = 0;
    arg0[2].v.ob[0] = D_802FDB40[arg1].unk2 + arg2;
    arg0[2].v.ob[1] = D_802FDB40[arg1].unk6 + arg3;
    arg0[2].v.ob[2] = D_802FDB40[arg1].unkA + arg4;
    arg0[2].v.tc[0] = 0x1E0, arg0[2].v.tc[1] = 0x1E0;
    arg0[3].v.ob[0] = D_802FDB40[arg1].unk2 + arg2;
    arg0[3].v.ob[1] = D_802FDB40[arg1].unk4 + arg3;
    arg0[3].v.ob[2] = D_802FDB40[arg1].unkA + arg4;
    arg0[3].v.tc[0] = 0x1E0;
    arg0[3].v.tc[1] = 0;
    arg0[4].v.ob[0] = D_802FDB40[arg1].unk0 + arg2;
    arg0[4].v.ob[1] = D_802FDB40[arg1].unk4 + arg3;
    arg0[4].v.ob[2] = D_802FDB40[arg1].unk8 + arg4;
    arg0[4].v.tc[0] = 0x1E0;
    arg0[4].v.tc[1] = 0;
    arg0[5].v.ob[0] = D_802FDB40[arg1].unk0 + arg2;
    arg0[5].v.ob[1] = D_802FDB40[arg1].unk6 + arg3;
    arg0[5].v.ob[2] = D_802FDB40[arg1].unk8 + arg4;
    arg0[5].v.tc[0] = 0x1E0, arg0[5].v.tc[1] = 0x1E0;
    arg0[6].v.ob[0] = D_802FDB40[arg1].unk0 + arg2;
    arg0[6].v.ob[1] = D_802FDB40[arg1].unk6 + arg3;
    arg0[6].v.ob[2] = D_802FDB40[arg1].unkA + arg4;
    arg0[6].v.tc[1] = 0x1E0;
    arg0[6].v.tc[0] = 0;
    arg0[7].v.ob[0] = D_802FDB40[arg1].unk0 + arg2;
    arg0[7].v.ob[1] = D_802FDB40[arg1].unk4 + arg3;
    arg0[7].v.ob[2] = D_802FDB40[arg1].unkA + arg4;
    arg0[7].v.tc[1] = 0;
    arg0[7].v.tc[0] = 0;
}

void func_8028C874(u8 arg0) {
    s32 i;
    s32 sp30;

    for (i = 0; i < D_8039B068; i++) {
        if (D_8039AF00[i].collected == 0) {
            if (D_802FDB40[D_8039AF00[i].type].unk14 == arg0) {
                func_802CE90C(i + 0x10000);
                sp30 = func_8026A6F0(D_803643E0 >> 5, D_803643E4 >> 5, D_803643E8 >> 5, D_8039AF00[i].x,
                                     D_8039AF00[i].y, D_8039AF00[i].z);
                if (sp30 <= D_802FDB40[D_8039AF00[i].type].unk12) {
                    D_8039AF00[i].collected = 1;
                    func_80260650(D_80367738, 0x70, NULL);
                    switch (D_8039AF00[i].type) {
                        case 0:
                            D_803F8B72 += 10;
                            break;
                        case 1:
                            D_803EDC00 += 10;
                            break;
                    }
                }
            } else {
                func_802CE880(i + 0x10000, D_8039AF00[i].x << 5, D_8039AF00[i].y << 5,
                              D_8039AF00[i].z << 5, D_802FDB40[D_8039AF00[i].type].unk10);
            }
        } else {
            D_8039AF00[i].alpha -= 20;
            if (D_8039AF00[i].alpha < 0) {
                D_8039AF00[i].alpha = 0;
            }
        }
    }
}

void func_8028CB30(Gfx **arg0, s32 arg1) {
    Gfx *gfx;
    s32 i;
    u8 spDF;

    gfx = *arg0;
    spDF = 0;
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, 0, 0, 0, PRIMITIVE, 0, 0, 0, TEXEL0, 0, 0, 0, PRIMITIVE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    for (i = 0; i < D_8039B068; i++) {
        if (D_8039AF00[i].collected == 0 || D_8039AF00[i].alpha != 0) {
            gDPPipeSync(gfx++);
            if (D_8039AF00[i].collected == 0 && (spDF == 0 || spDF == 2)) {
                gDPSetRenderMode(gfx++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
                spDF = 1;
            }
            if (D_8039AF00[i].collected != 0 && (spDF == 0 || spDF == 1)) {
                gDPSetRenderMode(gfx++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
                spDF = 2;
            }
            gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_8039AF00[i].alpha);
            gSPVertex(gfx++, osVirtualToPhysical(D_8039AF00[i].vtx), 8, 0);
            gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8039AF00[i].unkC), G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, 0,
                                G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSP1Triangle(gfx++, 6, 3, 2, 0);
            gSP1Triangle(gfx++, 6, 7, 3, 0);
            gSP1Triangle(gfx++, 4, 1, 0, 0);
            gSP1Triangle(gfx++, 1, 4, 5, 0);
            gSPModifyVertex(gfx++, 0, G_MWO_POINT_ST, 0x01E00000);
            gSPModifyVertex(gfx++, 1, G_MWO_POINT_ST, 0x01E001E0);
            gSPModifyVertex(gfx++, 2, G_MWO_POINT_ST, 0x000001E0);
            gSPModifyVertex(gfx++, 3, G_MWO_POINT_ST, 0x00000000);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
            gSPModifyVertex(gfx++, 7, G_MWO_POINT_ST, 0x01E00000);
            gSPModifyVertex(gfx++, 6, G_MWO_POINT_ST, 0x01E001E0);
            gSPModifyVertex(gfx++, 5, G_MWO_POINT_ST, 0x000001E0);
            gSPModifyVertex(gfx++, 4, G_MWO_POINT_ST, 0x00000000);
            gSP1Triangle(gfx++, 7, 5, 4, 0);
            gSP1Triangle(gfx++, 7, 6, 5, 0);
            gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8039AF00[i].unk10), G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, 0,
                                G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPModifyVertex(gfx++, 2, G_MWO_POINT_ST, 0x00000000);
            gSPModifyVertex(gfx++, 1, G_MWO_POINT_ST, 0x01E00000);
            gSPModifyVertex(gfx++, 5, G_MWO_POINT_ST, 0x01E001E0);
            gSPModifyVertex(gfx++, 6, G_MWO_POINT_ST, 0x000001E0);
            gSP1Triangle(gfx++, 5, 2, 1, 0);
            gSP1Triangle(gfx++, 2, 5, 6, 0);
        }
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}
