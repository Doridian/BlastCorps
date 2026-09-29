#include "common.h"
#include "game/audio.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"
#include "game/objects.h"

extern s32 D_802E8BE8;
extern OSMesgQueue D_80370BF8;
extern u8 D_803A7424;
extern u8 D_803F932C;
extern Mtx D_02000000[];
extern u8 D_803F932D;
extern u8 D_803F932E;
extern s32 D_803A73F0;
extern s32 D_803A73F4;
extern s32 D_803A73F8;
extern s16 D_803A7410;
extern s16 D_803A7412;
extern s32 D_803F9320;
extern s32 D_803F9324;

void func_80260650(SndBank *, s32, SndState **);
void func_802608C8(SndState *);
s32 func_8026A6F0(s32, s32, s32, s32, s32, s32);
s32 func_802A0CC8(s32, s32);
void func_802AACD4(u8, s32, s32, s16 *, s16 *);
void func_802AAE1C(u8, s16, s16, s32 *, s32 *);
void func_802CDA10(s32, s32, s32);
s32 func_802CDB70(s16, s16);
void func_802CE4F0(s32, s32, s32);
s32 func_802CE6F8(s32, s32, s32);
void osContGetReadData(OSContPad *);
void func_8028DA5C(Vtx *, u8);
void func_8026AD30(s32);
s32 func_8029B930(void);
s16 func_802A6F6C(void);
void func_802CDAE8(s16, s16);
u8 func_802CDF94(s16);
s16 func_802CE3B8(s16);
void func_802CE5BC(s32, s32, s32, s16, s32, s32);
void func_802CE65C(s32, s32, s16, s16);
void func_802CE880(s32, s32, s32, s32, s32);
void func_802CE90C(s32);
s32 func_802CE958(s32);
f32 sqrtf(f32);
void func_8028DD64();
TntCrate *func_8028DE94(void);
u8 func_8028FCD4(OSMesgQueue *, u8 *);

/* .bss, 0x8039B070-0x8039C550 (tools/bss_c.py) */
TntCrate D_8039B070[TNT_CRATE_MAX];
s32 D_8039B610;
s32 D_8039B614;
s32 D_8039B618;
s32 D_8039B61C;
u8 D_8039B620;
u8 D_8039B621[1];
u8 D_8039B622[2];
u8 D_8039B624[4];
u8 D_8039B628[0xE88];
u8 D_8039C4B0;
s32 D_8039C4B4;
u8 D_8039C4B8[0x40];
u8 D_8039C4F8[0x44];
u8 D_8039C53C[4];
u8 D_8039C540;
u8 D_8039C541;
u8 D_8039C542[2];
u8 D_8039C544[4];
u8 D_8039C548[8];

/* .data, 0x802FDB70-0x802FDBD0 (tools/data_c.py) */
s16 D_802FDB70[0x14] = { 0, 135, 105, 150, 150, 135, 0, 150, 200, 2000, 175, 0, 0, 195, 190, 185, 105 };
TntCrateInfo D_802FDB98[2] = {
    { -15, 15, 0, 30, -15, 15, 1798, 1799, 1798, 1799, 832, 928 },
    { -15, 15, 0, 30, -15, 15, 1798, 1799, 1002, 1003, 832, 928 },
};

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/48D00/func_8028D4C0.s")
#else
void func_8028D4C0(LevelTntCrate *arg0, LevelTntCrate *arg1) {
    D_8039B610 = 0;
    D_8039B614 = 0;
    D_8039B618 = 0;
    D_8039B61C = 0;
    D_8039B620 = 0;
    while (arg0 != arg1) {
        D_8039B070[D_8039B610].x = arg0->x << 5;
        D_8039B070[D_8039B610].y = arg0->y << 5;
        D_8039B070[D_8039B610].z = arg0->z << 5;
        D_8039B070[D_8039B610].y = func_802CE6F8(D_8039B070[D_8039B610].x, D_8039B070[D_8039B610].z,
                                                    D_8039B070[D_8039B610].y);
        D_8039B070[D_8039B610].unk23 = D_803F932C;
        D_8039B070[D_8039B610].type = arg0->type;
        D_8039B070[D_8039B610].timer = arg0->timer * 60;
        D_8039B070[D_8039B610].timerStart = D_8039B070[D_8039B610].timer;
        D_8039B070[D_8039B610].tex[0] = (u8 *)func_802A0CC8(D_802FDB98[D_8039B070[D_8039B610].type].unkC, 0);
        D_8039B070[D_8039B610].tex[1] = (u8 *)func_802A0CC8(D_802FDB98[D_8039B070[D_8039B610].type].unkE, 0);
        D_8039B070[D_8039B610].tex[2] = (u8 *)func_802A0CC8(D_802FDB98[D_8039B070[D_8039B610].type].unk10, 0);
        D_8039B070[D_8039B610].tex[3] = (u8 *)func_802A0CC8(D_802FDB98[D_8039B070[D_8039B610].type].unk12, 0);
        D_8039B070[D_8039B610].unkC = 0;
        D_8039B070[D_8039B610].unk14 = 0;
        D_8039B070[D_8039B610].unk1E = 0;
        D_8039B070[D_8039B610].unk19 = 0;
        D_8039B070[D_8039B610].unk1A = arg0->unk8;
        D_8039B070[D_8039B610].unk1C = arg0->unkA;
        D_8039B070[D_8039B610].unk20 = 0;
        D_8039B070[D_8039B610].unk22 = 0;
        D_8039B070[D_8039B610].unk24 = 0;
        D_8039B070[D_8039B610].unk26 = 0;
        D_8039B070[D_8039B610].unk28 = 0;
        D_8039B070[D_8039B610].unk2A = 0;
        D_8039B070[D_8039B610].active = 1;
        D_8039B070[D_8039B610].vtx = (Vtx *)D_80358070;
        D_80358070 += 0x80;
        func_8028DA5C(D_8039B070[D_8039B610].vtx, D_8039B070[D_8039B610].type);
        D_8039B610++;
        arg0++;
    }
}
#endif


void func_8028DA5C(Vtx *arg0, u8 arg1) {
    arg0[0].v.ob[0] = D_802FDB98[arg1].unk2;
    arg0[0].v.ob[1] = D_802FDB98[arg1].unk4;
    arg0[0].v.ob[2] = D_802FDB98[arg1].unk8;
    arg0[0].v.tc[1] = 0;
    arg0[0].v.tc[0] = 0;
    arg0[1].v.ob[0] = D_802FDB98[arg1].unk2;
    arg0[1].v.ob[1] = D_802FDB98[arg1].unk6;
    arg0[1].v.ob[2] = D_802FDB98[arg1].unk8;
    arg0[1].v.tc[1] = 0x3E0;
    arg0[1].v.tc[0] = 0;
    arg0[2].v.ob[0] = D_802FDB98[arg1].unk2;
    arg0[2].v.ob[1] = D_802FDB98[arg1].unk6;
    arg0[2].v.ob[2] = D_802FDB98[arg1].unkA;
    arg0[2].v.tc[0] = 0x3E0, arg0[2].v.tc[1] = 0x3E0;
    arg0[3].v.ob[0] = D_802FDB98[arg1].unk2;
    arg0[3].v.ob[1] = D_802FDB98[arg1].unk4;
    arg0[3].v.ob[2] = D_802FDB98[arg1].unkA;
    arg0[3].v.tc[0] = 0x3E0;
    arg0[3].v.tc[1] = 0;
    arg0[4].v.ob[0] = D_802FDB98[arg1].unk0;
    arg0[4].v.ob[1] = D_802FDB98[arg1].unk4;
    arg0[4].v.ob[2] = D_802FDB98[arg1].unk8;
    arg0[4].v.tc[0] = 0x3E0;
    arg0[4].v.tc[1] = 0;
    arg0[5].v.ob[0] = D_802FDB98[arg1].unk0;
    arg0[5].v.ob[1] = D_802FDB98[arg1].unk6;
    arg0[5].v.ob[2] = D_802FDB98[arg1].unk8;
    arg0[5].v.tc[0] = 0x3E0, arg0[5].v.tc[1] = 0x3E0;
    arg0[6].v.ob[0] = D_802FDB98[arg1].unk0;
    arg0[6].v.ob[1] = D_802FDB98[arg1].unk6;
    arg0[6].v.ob[2] = D_802FDB98[arg1].unkA;
    arg0[6].v.tc[1] = 0x3E0;
    arg0[6].v.tc[0] = 0;
    arg0[7].v.ob[0] = D_802FDB98[arg1].unk0;
    arg0[7].v.ob[1] = D_802FDB98[arg1].unk4;
    arg0[7].v.ob[2] = D_802FDB98[arg1].unkA;
    arg0[7].v.tc[1] = 0;
    arg0[7].v.tc[0] = 0;
}

void func_8028DD64(arg0)
    u8 arg0;
{
    TntCrate *sp1C;

    func_802CDA10(D_8039B070[arg0].x, D_8039B070[arg0].y, D_8039B070[arg0].z);
    D_8039B070[arg0].unk19 = 5;
    D_8039B070[arg0].active = 0;
    D_802E8BE4 = 10;
    D_802E8BE8 = 400;
    if (D_8039B070[arg0].unk40 != 0) {
        func_802608C8(D_8039B070[arg0].unk40);
        sp1C = func_8028DE94();
        if (sp1C != NULL) {
            func_80260650(D_80367738, 0x73, &sp1C->unk40);
        }
    }
    if (D_8039B070[arg0].unk44 != 0) {
        func_802608C8(D_8039B070[arg0].unk44);
    }
    func_80260650(D_80367738, 0x10, NULL);
}

TntCrate *func_8028DE94(void) {
    s32 i;

    for (i = 0; i < D_8039B610; i++) {
        if (D_8039B070[i].active != 0 && D_8039B070[i].unk14 != 0) {
            return &D_8039B070[i];
        }
    }
    return NULL;
}

void func_8028DF14(arg0)
    u8 arg0;
{
    s32 i;
    u8 sp4B;
    s32 sp44;
    s32 sp40;
    s32 sp3C;
    s16 sp3A;
    s16 sp38;
    u8 sp37;
    f32 sp30;
    s16 sp2E;
    u8 sp2D;

    for (i = 0; i < D_8039B610; i++) {
        if (D_8039B070[i].unk19 != 0) {
            D_8039B070[i].unk19--;
            if (D_8039B070[i].unk19 == 0) {
                D_803A73F0 = D_8039B070[i].x;
                D_803A73F4 = D_8039B070[i].y;
                D_803A73F8 = D_8039B070[i].z;
                if (D_8039B070[i].unk1A != 0) {
                    func_802CDAE8(D_8039B070[i].unk1C, D_8039B070[i].unk1A);
                }
            }
        }
        if (D_8039B070[i].active != 0) {
            if (D_8039B070[i].unk14 != 0 && D_8039B070[i].timer != 0) {
                D_8039B070[i].timer--;
                if (D_8039B070[i].timer <= 0) {
                    func_8028DD64(i);
                }
            }
            if (D_8039B070[i].unk14 != 0 && D_8039B070[i].timerStart != 0) {
                sp30 = (f32)D_8039B070[i].timer / (f32)D_8039B070[i].timerStart;
                sp30 = 1.0 - sp30;
                sp30 = sp30 * 50.0;
                if (D_8039B070[i].unk22 != 0) {
                    D_8039B070[i].unk20 -= (s16)sp30;
                    if (D_8039B070[i].unk20 < 0) {
                        D_8039B070[i].unk20 = -D_8039B070[i].unk20;
                        D_8039B070[i].unk22 = 0;
                    }
                } else {
                    D_8039B070[i].unk20 += (s16)sp30;
                    if (D_8039B070[i].unk20 >= 0x100) {
                        D_8039B070[i].unk20 = 0x1FE - D_8039B070[i].unk20;
                        D_8039B070[i].unk22 = 1;
                    }
                }
            }
            if (arg0 == 0) {
                D_8039B070[i].unk1E = 0;
            }
            if (D_8039B070[i].unk1E != 0 && func_802CE958(i + 0x100) == 0) {
                func_802CE65C(D_8039B070[i].x, D_8039B070[i].z, D_8039B070[i].unk1E, D_8039B070[i].unkC);
                D_8039B070[i].x = D_803F9320;
                D_8039B070[i].z = D_803F9324;
                D_8039B070[i].y = func_802CE6F8(D_8039B070[i].x, D_8039B070[i].z, D_8039B070[i].y);
                D_8039B070[i].unk23 = D_803F932C;
            }
            func_802CE4F0(D_8039B070[i].x, D_8039B070[i].y, D_8039B070[i].z);
            sp4B = func_802CDF94(D_802FDB98[D_8039B070[i].type].unk14);
            if (sp4B != 0) {
                if (D_803F932D != 0) {
                    D_803643D9 = 1;
                    func_8028DD64(i);
                }
                if (D_803F932E != 0) {
                    D_803ED40C = 1;
                }
                if (D_8039B070[i].unk14 == 0 && arg0 != 0) {
                    if (func_8028DE94() == NULL) {
                        func_80260650(D_80367738, 0x73, &D_8039B070[i].unk40);
                    }
                    D_8039B070[i].unk14 = D_80358060;
                }
                if (D_8039B620 == arg0) {
                    sp44 = D_803643E0 - D_8039B614, sp40 = D_803643E4 - D_8039B618;
                    sp3C = D_803643E8 - D_8039B61C;
                    D_8039B070[i].unk1E = sqrtf(sp44 * sp44 + sp40 * sp40 + sp3C * sp3C) + 8.0f;
                    if (D_8039B070[i].unk1E > D_802FDB70[arg0] && D_802E8BDC != 0x22) {
                        D_8039B070[i].unk1E = D_802FDB70[arg0];
                    }
                } else {
                    D_8039B070[i].unk1E = 0;
                }
            }
            if (D_8039B070[i].unk1E != 0) {
                sp37 = 0;
                sp3A = D_803A7410, sp38 = D_803A7412;
                func_802CE5BC(D_8039B070[i].x, D_8039B070[i].y, D_8039B070[i].z,
                              D_802FDB98[D_8039B070[i].type].unk14, 0xC9, 0);
                if (sp3A != D_803A7410 || sp38 != D_803A7412) {
                    sp37 = 1;
                }
                if (D_8039B070[i].unk1A != 0) {
                    sp2E = 0;
                } else {
                    sp2E = D_8039B070[i].unk1C;
                }
                if (func_802CDB70(D_802FDB98[D_8039B070[i].type].unk16, sp2E)) {
                    func_8028DD64(i);
                }
            } else {
                sp37 = 0;
            }
            sp2D = 0;
            if (D_803A7410 != 0 || D_803A7412 != 0xFFF) {
                if (func_8029B930() < 100) {
                    sp2D = 1;
                } else if (sp37 != 0) {
                    D_8039B070[i].unkC = func_802CE3B8(D_8039B070[i].unkC);
                } else {
                    D_8039B070[i].unkC = func_802A6F6C();
                }
            }
            if ((sp2D != 0 || arg0 == 0) && D_8039B070[i].active != 0) {
                func_802CE880(i + 0x100, D_8039B070[i].x, D_8039B070[i].y, D_8039B070[i].z,
                              D_802FDB98[D_8039B070[i].type].unk14);
                D_8039B070[i].unk1E = 0;
            } else {
                func_802CE90C(i + 0x100);
            }
            if (D_8039B070[i].unk1E > 0) {
                D_8039B070[i].unk1E -= (D_802E8BDC != 0x2B) ? 8 : 4;
            } else {
                D_8039B070[i].unk1E = 0;
            }
            if (D_8039B070[i].unk1E > 0 && D_8039B070[i].unk44 == 0 && D_8039B070[i].active != 0) {
                func_80260650(D_80367738, 7, &D_8039B070[i].unk44);
                if (D_80364456 == 4) {
                    func_8026AD30(0x54);
                }
            }
            if (D_8039B070[i].unk44 != 0 && D_8039B070[i].unk1E == 0) {
                func_802608C8(D_8039B070[i].unk44);
            }
        }
    }
    D_8039B614 = D_803643E0;
    D_8039B618 = D_803643E4;
    D_8039B61C = D_803643E8;
    D_8039B620 = arg0;
}

void func_8028E9E4(Gfx **arg0, Mtx *arg1) {
    Gfx *gfx;
    s32 i;
    f32 sp160[4][4];
    f32 sp120[4][4];

    gfx = *arg0;
    if (D_8039B610 > 0) {
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_2CYCLE);
        gDPSetRenderMode(gfx++, G_RM_PASS, G_RM_AA_ZB_OPA_SURF2);
        gDPSetCombineLERP(gfx++, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, 0, 0, 0,
                          COMBINED, 0, 0, 0, SHADE);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLOD(gfx++, G_TL_TILE);
    }
    for (i = 0; i < D_8039B610; i++) {
        if (D_8039B070[i].active != 0) {
            guRotateF(sp160, (f32)D_8039B070[i].unk2A / 4095.0 * 360.0, 0.0f, 1.0f, 0.0f);
            guTranslateF(sp120, D_8039B070[i].x / 32.0f, D_8039B070[i].y / 32.0f, D_8039B070[i].z / 32.0f);
            guMtxCatF(sp160, sp120, sp160);
            guMtxF2L(sp160, (Mtx *)((u8 *)arg1 + i * sizeof(Mtx) + 0x600));
            gSPMatrix(gfx++, &D_02000000[i + 24], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            gSPVertex(gfx++, osVirtualToPhysical(D_8039B070[i].vtx), 8, 0);
            gDPPipeSync(gfx++);
            gDPSetTextureImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_8039B070[i].tex[0]));
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
            gDPLoadSync(gfx++);
            gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 1023, 0x100);
            gDPSetTextureImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_8039B070[i].tex[1]));
            gDPTileSync(gfx++);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0x100, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
            gDPLoadSync(gfx++);
            gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 1023, 0x100);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, 0, 5, 0, 0, 5, 0);
            gDPSetTileSize(gfx++, G_TX_RENDERTILE, 2, 2, 0x7E, 0x7E);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0x100, 1, 0, 0, 5, 0, 0, 5, 0);
            gDPSetTileSize(gfx++, 1, 2, 2, 0x7E, 0x7E);
            gDPSetPrimColor(gfx++, 0, D_8039B070[i].unk20, 0, 0, 0, 0);
            gSP1Triangle(gfx++, 6, 3, 2, 0);
            gSP1Triangle(gfx++, 6, 7, 3, 0);
            gSP1Triangle(gfx++, 4, 1, 0, 0);
            gSP1Triangle(gfx++, 1, 4, 5, 0);
            gSPModifyVertex(gfx++, 0, G_MWO_POINT_ST, 0x03E00000);
            gSPModifyVertex(gfx++, 1, G_MWO_POINT_ST, 0x03E003E0);
            gSPModifyVertex(gfx++, 2, G_MWO_POINT_ST, 0x000003E0);
            gSPModifyVertex(gfx++, 3, G_MWO_POINT_ST, 0x00000000);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
            gSPModifyVertex(gfx++, 7, G_MWO_POINT_ST, 0x03E00000);
            gSPModifyVertex(gfx++, 6, G_MWO_POINT_ST, 0x03E003E0);
            gSPModifyVertex(gfx++, 5, G_MWO_POINT_ST, 0x000003E0);
            gSPModifyVertex(gfx++, 4, G_MWO_POINT_ST, 0x00000000);
            gSP1Triangle(gfx++, 7, 5, 4, 0);
            gSP1Triangle(gfx++, 7, 6, 5, 0);
            gDPSetTextureImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_8039B070[i].tex[2]));
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
            gDPLoadSync(gfx++);
            gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 1023, 0x100);
            gDPSetTextureImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_8039B070[i].tex[3]));
            gDPTileSync(gfx++);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0x100, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
            gDPLoadSync(gfx++);
            gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 1023, 0x100);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, 0, 5, 0, 0, 5, 0);
            gDPSetTileSize(gfx++, G_TX_RENDERTILE, 2, 2, 0x7E, 0x7E);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0x100, 1, 0, 0, 5, 0, 0, 5, 0);
            gDPSetTileSize(gfx++, 1, 2, 2, 0x7E, 0x7E);
            gSPModifyVertex(gfx++, 2, G_MWO_POINT_ST, 0x00000000);
            gSPModifyVertex(gfx++, 1, G_MWO_POINT_ST, 0x03E00000);
            gSPModifyVertex(gfx++, 5, G_MWO_POINT_ST, 0x03E003E0);
            gSPModifyVertex(gfx++, 6, G_MWO_POINT_ST, 0x000003E0);
            gSP1Triangle(gfx++, 5, 2, 1, 0);
            gSP1Triangle(gfx++, 2, 5, 6, 0);
            gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
        }
    }
    gDPSetTextureLOD(gfx++, G_TL_LOD);
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}

void func_8028F6B4(u8 arg0) {
    s32 i;

    for (i = 0; i < D_8039B610; i++) {
        if (D_8039B070[i].active != 0 && D_8039B070[i].unk23 == arg0) {
            D_8039B070[i].unk1E = 0;
            D_8039B070[i].unk24 = 1;
            func_802AACD4(arg0, D_8039B070[i].x, D_8039B070[i].z, &D_8039B070[i].unk26,
                          &D_8039B070[i].unk28);
        }
    }
}

void func_8028F794(u8 arg0) {
    s32 i;
    s16 sp22;

    for (i = 0; i < D_8039B610; i++) {
        if (D_8039B070[i].active != 0 && D_8039B070[i].unk24 != 0) {
            func_802AAE1C(arg0, D_8039B070[i].unk26, D_8039B070[i].unk28, &D_8039B070[i].x,
                          &D_8039B070[i].z);
            D_8039B070[i].y = func_802CE6F8(D_8039B070[i].x, D_8039B070[i].z, D_8039B070[i].y);
            if (D_8039B070[i].unk1A != 0) {
                sp22 = 0;
            } else {
                sp22 = D_8039B070[i].unk1C;
            }
            func_802CE4F0(D_8039B070[i].x, D_8039B070[i].y, D_8039B070[i].z);
            if (func_802CDB70(D_802FDB98[D_8039B070[i].type].unk14, sp22)) {
                func_8028DD64(i);
            }
        }
    }
}

void func_8028F93C(void) {
    s32 i;

    for (i = 0; i < D_8039B610; i++) {
        D_8039B070[i].unk24 = 0;
    }
}

void func_8028F994(s32 arg0, s32 arg1, s32 arg2) {
    s32 i;
    s32 sp20;

    arg0 >>= 5, arg1 >>= 5, arg2 >>= 5;
    for (i = 0; i < D_8039B610; i++) {
        if (D_8039B070[i].active != 0) {
            sp20 = func_8026A6F0(arg0, arg1, arg2, D_8039B070[i].x >> 5, D_8039B070[i].y >> 5,
                                 D_8039B070[i].z >> 5);
            if (sp20 <= (D_802FDB98[D_8039B070[i].type].unk14 >> 5)) {
                D_803A7424 = 1;
            }
        }
    }
}

void func_8028FAC0(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 i;
    s32 sp20;

    arg0 >>= 5, arg1 >>= 5, arg2 >>= 5, arg3 >>= 5;
    for (i = 0; i < D_8039B610; i++) {
        if (D_8039B070[i].active != 0 && D_8039B070[i].unk24 == 0) {
            sp20 = func_8026A6F0(arg0, arg1, arg2, D_8039B070[i].x >> 5, D_8039B070[i].y >> 5,
                                 D_8039B070[i].z >> 5);
            if (sp20 <= (D_802FDB98[D_8039B070[i].type].unk14 >> 5) + arg3) {
                D_803A7424 = 1;
            }
        }
    }
}
