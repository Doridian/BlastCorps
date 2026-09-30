#include "common.h"
#include "game/frontend.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

f32 sqrtf(f32);
s32 func_801FE760(s32);
u8 func_80264BA4(u8);
void func_801FD484(f32 *, f32 *, f32 *, f32 *, f32 *, f32);
void func_802595E0(void *, s32, s32, void *);
void func_801F374C(UnkStruct_8020BD30 *);
void func_801F4878(Gfx *, u8 *);
void func_801F4C3C(UnkStruct_8020BD30 *, f32);
s32 func_801F36B0(s32 *, s32 *);

extern f32 D_8020BDE4;
extern f32 D_8020BDEC;
extern u16 D_8035807C;
extern f32 D_8021A918;
extern f32 D_8021A91C;
extern f32 D_8021A920;

Gfx *func_801F3964(Gfx *, u8 *, UnkStruct_8020BD30 *, f32);
Gfx *func_801F4110(Gfx *, u8 *, UnkStruct_8020BD30 *, f32);
Gfx *func_801FE238(Gfx *, u8 *);
void func_801FCE74(Vtx *, s32, f32, f32, s32, s32, f32, s32);

extern Gfx D_8020BC88[];
extern u8 D_803156F8[];
extern f32 D_8020F080; /* 25000.0f */
extern u16 D_80217288;
extern u16 *D_8021728C;
extern Vtx D_02000000[];

s32 func_8026A828(s32, s32);
void func_801FDCA4(Vtx *, s32, s32);
void func_8027690C(u8 *arg0, f32 x, f32 y, f32 z, s16 *outX, s16 *outY, Mtx *arg6, Mtx *arg7, Mtx *arg8, f32 arg9);

extern u8 *D_80215A7C;
extern u8 *D_80215A80;
extern u8 *D_80215A84;
extern u8 D_0066C900[];
extern u8 D_0068B550[];
extern u16 *D_80215A70[];
extern Vtx D_80215A88[];
extern s32 D_80217290[];
extern s32 D_80217390[];
extern s32 D_80217490[];
extern s32 D_80217590[];
extern f64 D_8020EFC0; /* 250.0 */
extern f64 D_8020EFC8; /* 250.0 */
extern f64 D_8020EFD0; /* 250.0 */
extern f64 D_8020EFD8; /* 250.0 */
extern f64 D_8020EFE0; /* 250.0 */
extern f64 D_8020EFE8; /* 250.0 */
extern f64 D_8020EFF0; /* 250.0 */
extern f64 D_8020EFF8; /* 250.0 */
extern f64 D_8020F000; /* 250.0 */
extern f64 D_8020F008; /* 250.0 */
extern f64 D_8020F010; /* 250.0 */
extern f64 D_8020F018; /* 250.0 */
extern f64 D_8020F020; /* 250.0 */
extern f64 D_8020F028; /* 250.0 */
extern f64 D_8020F030; /* 250.0 */
extern f64 D_8020F038; /* 250.0 */
extern f64 D_8020F040; /* 250.0 */
extern f64 D_8020F048; /* 250.0 */

void func_8028B4C4(u32 arg0, u8 *arg1, u32 *arg2, u8 arg3, u8 arg4, u8 arg5);
void func_801F0570(void);

/* .bss, 0x80217690-0x802182C0 (tools/bss_c.py) */
Vtx D_80217690[7][2][4];
f32 D_80217A10[5][4][4];
s32 D_80217B50;
f32 D_80217B54;
f32 D_80217B58;
f32 D_80217B5C;
f32 D_80217B60;
f32 D_80217B64;
f32 D_80217B68;
s32 D_80217B6C;
Mtx D_80217B70[7][4];
UnkStruct_80218270 D_80218270[1];
u8 D_80218278[0x30];
s16 D_802182A8;
u8 D_802182AA[2];
u8 D_802182AC[4];
u8 D_802182B0[0x10];

/* .data, 0x8020BD30-0x8020BEE0 (tools/data_c.py) */
UnkStruct_8020BD30 D_8020BD30[7] = {
    {
        4476.0f, 27.3f, 27.3f, 614.0f, NULL, NULL, 200, 200, 200, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
    },
    {
        20000.0f, 1.0f, 60193.2f, 17992.0f, NULL, NULL, 40, 40, 180, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
    },
    {
        6790.0f, 1.0f, 686.9f, 9117.0f, &D_8020BD30[1], NULL, 255, 40, 0, 72.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
    },
    {
        12750.0f, 1.0f, 365.25f, 5984.0f, &D_8020BD30[2], D_8020BD30, 0, 200, 100, 144.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    },
    {
        12104.0f, -243.0f, 224.7f, 4328.0f, &D_8020BD30[3], NULL, 255, 120, 0, 216.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    },
    {
        4880.0f, 58.7f, 88.0f, 2320.0f, &D_8020BD30[4], NULL, 255, 120, 120, 288.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    },
    {
        70000.0f, 25.4f, 0.0f, 0.0f, NULL, &D_8020BD30[5], 255, 255, 90, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
    },
};

Gfx *func_801F3450(Gfx *arg0, u8 *arg1) {
    Gfx *gfx = arg0;
    UnkStruct_8020BD30 *sp48;
    UnkStruct_8020BD30 *sp44;
    u8 sp43;
    s32 sp3C;

    sp48 = D_8020BD30;
    sp44 = &sp48[&sp48[D_80217B6C] - sp48];
    gSPPerspNormalize(gfx++, D_8035807C);
    D_80217B50 = 4;
    guMtxIdentF(D_80217A10[4]);
    func_801F374C(&D_8020BE98);
    if (D_802182A8 != 1) {
        D_80217B54 = sp44->unk2C;
        D_80217B58 = sp44->unk30;
        D_80217B5C = sp44->unk34;
        D_80217B60 = sp44->unk20;
        D_80217B64 = sp44->unk24;
        D_80217B68 = sp44->unk28;
    }
    guLookAt((Mtx *)(arg1 + 0x140), D_80217B54 + 1.0f, D_80217B58, D_80217B5C, D_80217B60, D_80217B64, D_80217B68,
             0.0f, 1.0f, 0.0f);
    func_801F4878((Gfx *)(arg1 + 0xACB0), arg1);
    func_802595E0(D_80218270, 7, sizeof(UnkStruct_80218270), func_801F36B0);
    for (sp3C = 0, sp43 = 0; sp3C < 7; sp3C++) {
        if (sp43 == 0 || D_80217B6C != 3) {
            gSPDisplayList(gfx++, D_80218270[sp3C].unk4);
        }
        if (D_80218270[sp3C].unk0 == 3) {
            sp43 = 1;
        }
    }
    return gfx;
}

s32 func_801F36B0(s32 *arg0, s32 *arg1) {
    UnkStruct_8020BD30 *spC;
    UnkStruct_8020BD30 *sp8;
    UnkStruct_8020BD30 *sp4;

    spC = D_8020BD30;
    sp8 = &spC[&spC[*arg0] - spC];
    sp4 = &spC[&spC[*arg1] - spC];
    return sp4->unk38 - sp8->unk38;
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/C450/func_801F374C.s")
#else
void func_801F374C(UnkStruct_8020BD30 *arg0) {
    f32 sp74;
    f32 sp70;
    f32 sp6C;
    f32 sp68;
    f32 sp64;
    Mtx *sp60;
    f32 sp20[4][4];

    while (arg0 != NULL) {
        sp74 = arg0->unk0 / D_8020BDE4;
        sp60 = D_80217B70[arg0 - D_8020BD30];
        sp64 = 0.0f;
        if (arg0->unk8 != 0.0f) {
            arg0->unk1C += D_8020BDEC * 360.0 / arg0->unk8 / 1800.0 * 60.0 / 60.0;
        }
        func_801FD484(&sp64, &arg0->unk1C, &sp70, &sp6C, &sp68, arg0->unkC);
        guTranslateF(sp20, -sp70, -sp6C, -sp68);
        guScale(&sp60[D_8035805C + 2], sp74, sp74, sp74);
        guMtxCatF(D_80217A10[D_80217B50], sp20, D_80217A10[D_80217B50 - 1]);
        D_80217B50--;
        guMtxF2L(D_80217A10[D_80217B50], &sp60[D_8035805C]);
        osWritebackDCache(sp60, 4 * sizeof(Mtx));
        func_801F4C3C(arg0, sp74);
        func_801F374C(arg0->unk14);
        D_80217B50++;
        arg0 = arg0->unk10;
    }
}
#endif

Gfx *func_801F3964(Gfx *arg0, u8 *arg1, UnkStruct_8020BD30 *arg2, f32 arg3) {
    Gfx *gfx = arg0;
    f32 spA0;
    s16 sp9E;
    s16 sp9C;
    u8 *sp98;
    Vtx *sp94;
    s32 sp90;

    spA0 = arg2->unk0 * 64.0 / D_8020BDE4;
    sp94 = D_80217690[arg2 - D_8020BD30][D_8035805C];
    gDPPipeSync(gfx++);
    gDPSetPrimColorB(gfx++, 0xFF, 0xFF, arg2->unk18, arg2->unk19, arg2->unk1A, 0xFF);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0x2000, 0x2000, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineLERP(gfx++, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0);
    switch (arg2 - D_8020BD30) {
        case 1:
        case 4:
            sp98 = D_80215A80;
            break;
        default:
            sp98 = D_80215A7C;
            break;
    }
    if (arg2->unk38 > 1.0) {
        spA0 = spA0 * 10000.0 / arg2->unk38;
    }
    sp90 = spA0;
    func_8027690C(arg1, 0.0f, 0.0f, 0.0f, &sp9E, &sp9C, &D_80217B70[arg2 - D_8020BD30][D_8035805C],
                  &D_80217B70[arg2 - D_8020BD30][D_8035805C] + 2, (Mtx *)(arg1 + 0x1280), 4.0f);
    if (((sp9E > 0) ? sp9E : -sp9E) + spA0 / 2.0 < 4096.0 && ((sp9C > 0) ? sp9C : -sp9C) + spA0 / 2.0 < 4096.0) {
        gDPLoadTextureBlock(gfx++, sp98, G_IM_FMT_IA, G_IM_SIZ_8b, 64, 64, 0, G_TX_CLAMP, G_TX_MIRROR, G_TX_NOMASK,
                            6, G_TX_NOLOD, G_TX_NOLOD);
        sp94[0].v.ob[0] = sp9E - sp90 / 2;
        sp94[0].v.ob[1] = sp9C - sp90 / 2;
        sp94[0].v.ob[2] = -10;
        sp94[0].v.tc[0] = 0;
        sp94[0].v.tc[1] = 0;
        sp94[1].v.ob[0] = sp9E - sp90 / 2;
        sp94[1].v.ob[1] = sp9C + sp90 / 2;
        sp94[1].v.ob[2] = -10;
        sp94[1].v.tc[0] = 0;
        sp94[1].v.tc[1] = 0x3F00;
        sp94[2].v.ob[0] = sp9E + sp90 / 2;
        sp94[2].v.ob[1] = sp9C + sp90 / 2;
        sp94[2].v.ob[2] = -10;
        sp94[2].v.tc[0] = 0x3F00;
        sp94[2].v.tc[1] = 0x3F00;
        sp94[3].v.ob[0] = sp9E + sp90 / 2;
        sp94[3].v.ob[1] = sp9C - sp90 / 2;
        sp94[3].v.ob[2] = -10;
        sp94[3].v.tc[0] = 0x3F00;
        sp94[3].v.tc[1] = 0;
        gSPVertex(gfx++, sp94, 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 3, 2, 0);
        gDPPipeSync(gfx++);
    }
    gDPPipeSync(gfx++);
    osWritebackDCache(sp94, 4 * sizeof(Vtx));
    return gfx;
}

Gfx *func_801F4110(Gfx *arg0, u8 *arg1, UnkStruct_8020BD30 *arg2, f32 arg3) {
    Gfx *gfx = arg0;
    f32 spA0;
    s16 sp9E;
    s16 sp9C;
    Vtx *sp98;
    s32 sp94;

    spA0 = arg2->unk0 * 64.0 / D_8020BDE4;
    sp98 = D_80217690[arg2 - D_8020BD30][D_8035805C];
    gDPPipeSync(gfx++);
    gDPSetPrimColorB(gfx++, 0xFF, 0xFF, arg2->unk18, arg2->unk19, arg2->unk1A, 0xFF);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0x1000, 0x1000, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, PRIMITIVE, 0, TEXEL0, 0, 0, 0, 0, TEXEL0, PRIMITIVE, 0, TEXEL0, 0);
    if (arg2->unk38 > 1.0) {
        spA0 = spA0 * 10000.0 / arg2->unk38;
    }
    sp94 = spA0;
    func_8027690C(arg1, 0.0f, 0.0f, 0.0f, &sp9E, &sp9C, &D_80217B70[arg2 - D_8020BD30][D_8035805C],
                  &D_80217B70[arg2 - D_8020BD30][D_8035805C] + 2, (Mtx *)(arg1 + 0x1280), 4.0f);
    if (((sp9E > 0) ? sp9E : -sp9E) + spA0 / 2.0 < 4096.0 && ((sp9C > 0) ? sp9C : -sp9C) + spA0 / 2.0 < 4096.0) {
        gDPLoadTextureBlock(gfx++, D_80215A84, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_CLAMP, G_TX_CLAMP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        sp98[0].v.ob[0] = sp9E - sp94 / 2;
        sp98[0].v.ob[1] = sp9C - sp94 / 2;
        sp98[0].v.ob[2] = -10;
        sp98[0].v.tc[0] = 0;
        sp98[0].v.tc[1] = 0;
        sp98[1].v.ob[0] = sp9E - sp94 / 2;
        sp98[1].v.ob[1] = sp9C + sp94 / 2;
        sp98[1].v.ob[2] = -10;
        sp98[1].v.tc[0] = 0;
        sp98[1].v.tc[1] = 0x3E00;
        sp98[2].v.ob[0] = sp9E + sp94 / 2;
        sp98[2].v.ob[1] = sp9C + sp94 / 2;
        sp98[2].v.ob[2] = -10;
        sp98[2].v.tc[0] = 0x3E00;
        sp98[2].v.tc[1] = 0x3E00;
        sp98[3].v.ob[0] = sp9E + sp94 / 2;
        sp98[3].v.ob[1] = sp9C - sp94 / 2;
        sp98[3].v.ob[2] = -10;
        sp98[3].v.tc[0] = 0x3E00;
        sp98[3].v.tc[1] = 0;
        gSPVertex(gfx++, sp98, 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 3, 2, 0);
        gDPPipeSync(gfx++);
    }
    gDPPipeSync(gfx++);
    osWritebackDCache(sp98, 4 * sizeof(Vtx));
    return gfx;
}

void func_801F4878(Gfx *arg0, u8 *arg1) {
    Gfx *gfx = arg0;
    s32 sp68;
    UnkStruct_8020BD30 *sp64;
    UnkStruct_8020BD30 *sp60;
    Mtx *sp5C;
    f32 sp58;
    f32 sp54;
    f32 sp50;
    f32 sp4C;

    for (sp68 = 0; sp68 < 7; sp68++) {
        sp64 = D_8020BD30;
        sp60 = &sp64[&sp64[sp68] - sp64];
        sp5C = D_80217B70[sp68];
        sp58 = sp60->unk0 / D_8020BDE4;
        sp54 = sp60->unk20 - D_80217B54;
        sp50 = sp60->unk24 - D_80217B58;
        sp4C = sp60->unk28 - D_80217B5C;
        sp60->unk38 = sqrtf(sp54 * sp54 + sp50 * sp50 + sp4C * sp4C);
        D_80218270[sp60 - D_8020BD30].unk0 = sp60 - D_8020BD30;
        D_80218270[sp60 - D_8020BD30].unk4 = gfx;
        switch (sp68) {
            case 3:
                gSPMatrix(gfx++, arg1 + 0x80, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPMatrix(gfx++, arg1 + 0x140, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
                gSPMatrix(gfx++, &sp5C[D_8035805C], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPMatrix(gfx++, &sp5C[D_8035805C + 2], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
                gSPMatrix(gfx++, arg1 + 0x1280, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
                gfx = func_801FE238(gfx, arg1);
                break;
            case 6:
                gSPMatrix(gfx++, arg1 + 0x100, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPMatrix(gfx++, arg1 + 0x1C0, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                gfx = func_801F4110(gfx, arg1, sp60, sp58);
                break;
            default:
                gSPMatrix(gfx++, arg1 + 0x100, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPMatrix(gfx++, arg1 + 0x1C0, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                gfx = func_801F3964(gfx, arg1, sp60, sp58);
                break;
        }
        gSPEndDisplayList(gfx++);
    }
}

void func_801F4C3C(UnkStruct_8020BD30 *arg0, f32 arg1) {
    f32 sp2C;
    f32 sp28;
    f32 sp24;

    sp28 = 0.0f;
    sp24 = 0.0f;
    if (arg0 - D_8020BD30 != 6) {
        sp2C = D_80217A10[D_80217B50][3][3];
        arg0->unk20 = D_80217A10[D_80217B50][3][0] / sp2C;
        arg0->unk24 = D_80217A10[D_80217B50][3][1] / sp2C;
        arg0->unk28 = D_80217A10[D_80217B50][3][2] / sp2C;
    }
    if (arg0 - D_8020BD30 == 3) {
        if (arg0) {
        }
        func_801FD484(&D_8021A920, &D_8021A91C, &arg0->unk2C, &arg0->unk30, &arg0->unk34, arg1 * D_8021A918);
        arg0->unk2C += arg0->unk20;
        arg0->unk30 += arg0->unk24;
        arg0->unk34 += arg0->unk28;
        return;
    }
    func_801FD484(&sp28, &sp24, &arg0->unk2C, &arg0->unk30, &arg0->unk34, arg1 * D_8021A918);
    arg0->unk2C += arg0->unk20;
    arg0->unk30 += arg0->unk24;
    arg0->unk34 += arg0->unk28;
}

void func_801F4E1C(s32 *arg0, s32 *arg1) {
    s32 *spC = arg0;
    s32 *sp8 = arg1;
    u32 sp4;

    for (sp4 = 0; sp4 < 16; sp4++) {
        sp8[sp4] = spC[sp4];
    }
}
