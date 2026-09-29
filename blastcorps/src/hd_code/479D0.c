#include "common.h"
#include "game/audio.h"
#include "game/game.h"

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} UnkStruct_8028C190; /* size = 0x8 */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ u8 unk14;
} UnkStruct_802FDB40; /* size = 0x16 */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ s16 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ Vtx *unk14;
} UnkStruct_8039AF00; /* size = 0x18 */

extern s16 D_803EDC00;
extern s16 D_803F8B72;

void func_80260650(SndBank *, s32, s32 *);
s32 func_8026A6F0(s32, s32, s32, s32, s32, s32);
s32 func_802A0CC8(s32, s32);
void func_802CE880(s32, s32, s32, s32, s32);
void func_802CE90C(s32);
void func_8028C41C(Vtx *, u8, s16, s16, s16);

/* .bss, 0x8039AF00-0x8039B070 (tools/bss_c.py) */
UnkStruct_8039AF00 D_8039AF00[1];
u8 D_8039AF18[0x150];
s32 D_8039B068;

/* .data, 0x802FDB40-0x802FDB70 (tools/data_c.py) */
UnkStruct_802FDB40 D_802FDB40[2] = {
    { -10, 10, 0, 20, -10, 10, 2453, 2453, 640, 100, 10 },
    { -10, 10, 0, 20, -10, 10, 2455, 2455, 640, 100, 1 },
};

void func_8028C190(UnkStruct_8028C190 *arg0, UnkStruct_8028C190 *arg1) {
    D_8039B068 = 0;
    while (arg0 != arg1) {
        D_8039AF00[D_8039B068].unk0 = arg0->unk0;
        D_8039AF00[D_8039B068].unk2 = arg0->unk2;
        D_8039AF00[D_8039B068].unk4 = arg0->unk4;
        D_8039AF00[D_8039B068].unk6 = arg0->unk6;
        D_8039AF00[D_8039B068].unk7 = 0;
        D_8039AF00[D_8039B068].unk8 = 0xFF;
        D_8039AF00[D_8039B068].unkC = func_802A0CC8(D_802FDB40[D_8039AF00[D_8039B068].unk6].unkC, 0);
        D_8039AF00[D_8039B068].unk10 = func_802A0CC8(D_802FDB40[D_8039AF00[D_8039B068].unk6].unkE, 0);
        D_8039AF00[D_8039B068].unk14 = (Vtx *)D_80358070;
        D_80358070 += 0x80;
        func_8028C41C(D_8039AF00[D_8039B068].unk14, D_8039AF00[D_8039B068].unk6, D_8039AF00[D_8039B068].unk0,
                      D_8039AF00[D_8039B068].unk2, D_8039AF00[D_8039B068].unk4);
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
        if (D_8039AF00[i].unk7 == 0) {
            if (D_802FDB40[D_8039AF00[i].unk6].unk14 == arg0) {
                func_802CE90C(i + 0x10000);
                sp30 = func_8026A6F0(D_803643E0 >> 5, D_803643E4 >> 5, D_803643E8 >> 5, D_8039AF00[i].unk0,
                                     D_8039AF00[i].unk2, D_8039AF00[i].unk4);
                if (sp30 <= D_802FDB40[D_8039AF00[i].unk6].unk12) {
                    D_8039AF00[i].unk7 = 1;
                    func_80260650(D_80367738, 0x70, NULL);
                    switch (D_8039AF00[i].unk6) {
                        case 0:
                            D_803F8B72 += 10;
                            break;
                        case 1:
                            D_803EDC00 += 10;
                            break;
                    }
                }
            } else {
                func_802CE880(i + 0x10000, D_8039AF00[i].unk0 << 5, D_8039AF00[i].unk2 << 5,
                              D_8039AF00[i].unk4 << 5, D_802FDB40[D_8039AF00[i].unk6].unk10);
            }
        } else {
            D_8039AF00[i].unk8 -= 20;
            if (D_8039AF00[i].unk8 < 0) {
                D_8039AF00[i].unk8 = 0;
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
        if (D_8039AF00[i].unk7 == 0 || D_8039AF00[i].unk8 != 0) {
            gDPPipeSync(gfx++);
            if (D_8039AF00[i].unk7 == 0 && (spDF == 0 || spDF == 2)) {
                gDPSetRenderMode(gfx++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
                spDF = 1;
            }
            if (D_8039AF00[i].unk7 != 0 && (spDF == 0 || spDF == 1)) {
                gDPSetRenderMode(gfx++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
                spDF = 2;
            }
            gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_8039AF00[i].unk8);
            gSPVertex(gfx++, osVirtualToPhysical(D_8039AF00[i].unk14), 8, 0);
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
