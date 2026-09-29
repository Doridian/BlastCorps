#include "common.h"
#include "game/game.h"
#include "game/sched.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

typedef struct {
    /* 0x0 */ u16 unk0[2];
} UnkStruct_802FA8A0;

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ u8 pad2[2];
    /* 0x4 */ u8 *unk4;
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
} UnkStruct_802FA280; /* size = 0xC */


void func_802A0B00(u16, s32);

/* .bss, 0x8036BFC0-0x8036BFE0 (tools/bss_c.py) */
f32 D_8036BFC0;
u8 D_8036BFC4;
u8 D_8036BFC5;
f32 D_8036BFC8;
f32 D_8036BFCC;
f32 D_8036BFD0;

/* .data, 0x802FA280-0x802FA8B0 (tools/data_c.py) */
UnkStruct_802FA280 D_802FA280[0x3c][2] = {
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 0, { 0 }, NULL, 2, 0, 0, 2 }, { 0, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 0, { 0 }, NULL, 2, 0, 0, 2 }, { 0, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 0, { 0 }, NULL, 2, 0, 0, 2 }, { 0, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 0, { 0 }, NULL, 2, 0, 0, 2 }, { 0, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 0, { 0 }, NULL, 2, 0, 0, 2 }, { 0, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 0, { 0 }, NULL, 2, 0, 0, 2 }, { 0, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 0, { 0 }, NULL, 2, 0, 0, 2 }, { 0, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1317, { 0 }, NULL, 2, 0, 0, 2 }, { 1648, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
    { { 1335, { 0 }, NULL, 2, 0, 0, 2 }, { 1649, { 0 }, NULL, 2, 0, 0, 0 } },
};
extern Vtx D_802FA820[2][4];
#ifdef ASSET_INIT
#include "hd_code/2D810/D_802FA820.inc"
#else
Vtx D_802FA820[2][4] = { 0 };
#endif
UnkStruct_802FA8A0 D_802FA8A0 = { { 0, 3 } };

Gfx *func_80271FD0(Gfx *arg0, s32 arg1, u16 arg2, s16 arg3, s16 arg4, s32 *arg5) {
    Gfx *gfx;
    UnkStruct_802FA8A0 sp78;
    s32 i;
    UnkStruct_802FA280 *sp70;
    f32 sp6C;
    f32 sp68;
    f32 sp64;
    f32 sp60;
    f32 sp5C;

    gfx = arg0;
    sp78 = D_802FA8A0;
    if (D_8036BFC4 == 0) {
        *arg5 = 0;
        return gfx;
    }
    sp6C = 120.0 - (f32) arg4 * 0.22 + D_8036BFC0;
    D_8036BFCC = (0.0 > sp6C) ? 0.0 : sp6C;
    D_8036BFD0 = (0.0 > -sp6C) ? 0.0 : -sp6C;
    *arg5 = D_8036BFCC;
    if (*arg5 <= 0) {
        return gfx;
    }
    D_8036BFC8 = (arg3 - 2048.0) * 0.5;
    D_802FA820[D_8036BFC5][2].v.ob[1] = D_8036BFCC * 4.0 - 1.0;
    D_802FA820[D_8036BFC5][3].v.ob[1] = D_8036BFCC * 4.0 - 1.0;
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, TEXEL1, TEXEL0, TEXEL1_ALPHA, TEXEL0, TEXEL1, TEXEL0, TEXEL0, TEXEL0, 0, 0, 0,
                      COMBINED, 0, 0, 0, COMBINED);
    gDPSetCycleType(gfx++, G_CYC_2CYCLE);
    gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    for (i = 0; i < 2; i++) {
        sp70 = &D_802FA280[arg2][i];
        gDPSetTextureImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, sp70->unk4);
        gDPTileSync(gfx++);
        gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, i << 8, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
        gDPLoadSync(gfx++);
        gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 0x3FF, 0x100);
    }
    gSPMatrix(gfx++, OS_PHYSICAL_TO_K0(arg1 + 0x100), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, OS_PHYSICAL_TO_K0(arg1 + 0x1C0), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gDPTileSync(gfx++);
    gDPSetTextureLOD(gfx++, G_TL_TILE);
    for (i = 0; i < 2; i++) {
        sp70 = &D_802FA280[arg2][i];
        gDPSetTile(gfx++, sp78.unk0[i], G_IM_SIZ_16b, 8, i << 8, i, 0, sp70->unkB & 3, 5, i, sp70->unkA & 3, 5, i);
        gDPSetTileSize(gfx++, i, 0, 0, 0x7C, 0x7C);
    }
    sp68 = (D_8036BFC8 + 16.0f) / (1 << sp70->unk8);
    sp60 = 319.0 / (f32) (1 << sp70->unk8);
    D_802FA820[D_8036BFC5][0].v.tc[0] = sp68 * 32.0;
    D_802FA820[D_8036BFC5][1].v.tc[0] = (sp68 + sp60) * 32.0;
    D_802FA820[D_8036BFC5][2].v.tc[0] = sp68 * 32.0;
    D_802FA820[D_8036BFC5][3].v.tc[0] = (sp68 + sp60) * 32.0;
    sp64 = (D_8036BFCC - D_8036BFC0 - D_8036BFD0) / (1 << sp70->unk9);
    sp5C = (D_8036BFCC - 1.0) / (f32) (1 << sp70->unk9);
    D_802FA820[D_8036BFC5][0].v.tc[1] = sp64 * 32.0;
    D_802FA820[D_8036BFC5][1].v.tc[1] = sp64 * 32.0;
    D_802FA820[D_8036BFC5][2].v.tc[1] = (sp64 - sp5C) * 32.0;
    D_802FA820[D_8036BFC5][3].v.tc[1] = (sp64 - sp5C) * 32.0;
    gSPVertex(gfx++, D_802FA820[D_8036BFC5], 4, 0);
    gDPPipeSync(gfx++);
    gSP1Triangle(gfx++, 0, 1, 2, 0);
    gSP1Triangle(gfx++, 1, 2, 3, 0);
    D_8036BFC5 ^= 1;
    return gfx;
}

void func_802729F0(u16 arg0, u16 arg1) {
    s32 i;
    f32 sp28;
    u16 sp26;

    sp28 = D_803BE720 * D_803BE718;
    sp28 = (D_803BE722 * D_803BE71C + sp28) / 32.0f;
    D_8036BFC4 = 1;
    switch (arg0) {
        case 0:
        case 1:
        case 2:
        case 0x40:
        case 0x800:
        case 0x1000:
            D_8036BFC0 = 1200000.0 / sp28;
            break;
        case 4:
        case 0x100:
        case 0x2000:
            D_8036BFC0 = 600000.0 / sp28;
            break;
        default:
            D_8036BFC0 = 0.0f;
            break;
    }
    if (arg1 == 0x34 || arg1 == 0x3B || arg1 == 0x26 || arg1 == 0x11) {
        D_8036BFC0 += 32.0f;
    }
    for (i = 0; i < 2; i++) {
        D_802FA280[arg1][i].unk4 = D_80358070;
        sp26 = D_802FA280[arg1][i].unk0;
        if (sp26 != 0) {
            func_802A0B00(sp26, 0);
        } else {
            D_8036BFC4 = 0;
        }
    }
}

void func_80272C40(s32 arg0) {
}
