#include "common.h"
#include "game/sched.h"
#include "game/camera.h"
#include "functions.h"


extern Mtx D_02000000[];
extern u16 D_8035807C;
extern s16 D_80367BD6;
extern s32 D_803F7660;
extern s32 D_803F7670;
extern s32 D_803F7678;

extern u8 D_803643DB;
extern f32 D_80364414;
extern s16 D_803F767C;
extern s16 D_803F7680;

f32 func_80284ADC(s16, s16, s16, s16);

/* .bss, 0x8036E5E0-0x8036E660 (tools/bss_c.py) */
Mtx D_8036E5E0[2];

/* .data, 0x802FC5B0-0x802FDA60 (tools/data_c.py) */
extern u16 D_802FC5B0[0x80];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FC5B0.inc"
#else
u16 D_802FC5B0[0x80] = { 0 };
#endif
extern u16 D_802FC6B0[0x400];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FC6B0.inc"
#else
u16 D_802FC6B0[0x400] = { 0 };
#endif
extern u16 D_802FCEB0[0x400];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FCEB0.inc"
#else
u16 D_802FCEB0[0x400] = { 0 };
#endif
extern u8 D_802FD6B0[0x100];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FD6B0.inc"
#else
u8 D_802FD6B0[0x100] = { 0 };
#endif
extern Vtx D_802FD7B0[4];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FD7B0.inc"
#else
Vtx D_802FD7B0[4] = { 0 };
#endif
extern Vtx D_802FD7F0[4];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FD7F0.inc"
#else
Vtx D_802FD7F0[4] = { 0 };
#endif
extern Vtx D_802FD830[2][12];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FD830.inc"
#else
Vtx D_802FD830[2][12] = { 0 };
#endif
f32 D_802FD9B0 = 0.0f;
extern Vtx D_802FD9B8[0xa];
#ifdef ASSET_INIT
#include "hd_code/3E4C0/D_802FD9B8.inc"
#else
Vtx D_802FD9B8[0xa] = { 0 };
#endif

void func_80282C80(Gfx **arg0, Mtx *arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7) {
    Gfx *gfx;
    f32 dist;
    f32 angle;
    u8 r;
    u8 g;
    s16 v;
    f32 mf[4][4];
    f32 mf2[4][4];
    s32 px;
    s32 pz;
    u8 near;

    gfx = *arg0;
    arg2 >>= 5;
    arg3 >>= 5;
    arg4 >>= 5;
    arg5 >>= 5;
    arg6 >>= 5;
    arg7 >>= 5;
    px = D_803F7670 >> 5;
    pz = D_803F7678 >> 5;
    dist = sqrtf((px - arg2) * (px - arg2) + (pz - arg4) * (pz - arg4));
    if (dist < 1.0) {
        dist = 1.0f;
    }
    if (px >= arg2 && pz >= arg4) {
        angle = func_802AD7D4((px - arg2) / dist * 65536.0) >> 4;
    }
    if (px >= arg2 && pz < arg4) {
        angle = (func_802AD7D4((arg4 - pz) / dist * 65536.0) >> 4) + 0x400;
    }
    if (px < arg2 && pz < arg4) {
        angle = (func_802AD7D4((arg2 - px) / dist * 65536.0) >> 4) + 0x800;
    }
    if (px < arg2 && pz >= arg4) {
        angle = (func_802AD7D4((pz - arg4) / dist * 65536.0) >> 4) + 0xC00;
    }
    angle = angle * (360.0 / 4095.0);
    angle = 360.0 - angle - 45.0;
    angle += D_80364452 * 360.0 / 4095.0 - 135.0;
    dist = sqrtf((arg5 - px) * (arg5 - px) + (arg7 - pz) * (arg7 - pz));
    if (dist > 1500.0f) {
        g = 0xFF;
        r = 0;
    } else if (dist < 500.0f) {
        r = 0xFF;
        g = 0;
    } else {
        v = (dist - 500.0f) / 1000.0f * 511.0f;
        if (v < 0x100) {
            g = v, r = 0xFF;
        } else {
            g = 0xFF, r = 0x1FE - v;
        }
    }
    if (dist < 250.0f) {
        near = 1;
    } else {
        near = 0;
    }
    if ((D_803156C4 % 30 >= 16 || near == 0) && D_803F7660 != 9999999) {
        guRotateF(mf, 20.0f, 1.0f, 0.0f, 0.0f);
        guRotateF(mf2, -angle, 0.0f, 0.0f, 1.0f);
        guMtxCatF(mf, mf2, mf);
        guTranslateF(mf2, -150.0f, -230.0f, -800.0f);
        guMtxCatF(mf, mf2, mf);
        guMtxF2L(mf, &arg1[86]);
        gSPMatrix(gfx++, &D_02000000[2], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPPerspNormalize(gfx++, D_8035807C);
        gSPMatrix(gfx++, &D_02000000[86], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH | G_CULL_FRONT | G_LIGHTING | G_TEXTURE_GEN);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        if (D_80367BD6 == 0xFF) {
            gDPSetRenderMode(gfx++, G_RM_RA_OPA_SURF, G_RM_RA_OPA_SURF2);
        } else {
            gDPSetRenderMode(gfx++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
        }
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, r, g, 0, D_80367BD6);
        gSPTexture(gfx++, 0x07C0, 0x07C0, 0, G_TX_RENDERTILE, G_ON);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_802FCEB0), G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                            G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPVertex(gfx++, osVirtualToPhysical(D_802FD9B8), 10, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
        gSP1Triangle(gfx++, 3, 1, 0, 0);
        gSP1Triangle(gfx++, 1, 4, 2, 0);
        gSP1Triangle(gfx++, 4, 5, 2, 0);
        gSP1Triangle(gfx++, 5, 3, 2, 0);
        gSP1Triangle(gfx++, 6, 7, 8, 0);
        gSP1Triangle(gfx++, 9, 6, 8, 0);
        gSP1Triangle(gfx++, 7, 9, 8, 0);
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}

void func_8028376C(Gfx **arg0, Mtx *arg1, u8 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    Gfx *gfx = *arg0;
    s32 dist;
    u8 shift;
    s32 scale;
    s16 x;
    s16 y;
    s16 px;
    s16 py;
    Mtx sp1C8;
    f32 sp188[4][4];
    f32 sp148[4][4];
    f32 ox;
    f32 oy;
    f32 oz;
    f32 angle;

    dist = func_8026A610(arg3, arg4, arg5, arg6);
    if (dist < 0x55F0) {
        shift = 10;
        scale = 1200;
    }
    if (dist >= 0x55F0 && dist < 0xABE0) {
        shift = 0;
        scale = 2400;
    }
    if (dist >= 0xABE0) {
        shift = 15;
        scale = 4800;
    }
    if (D_803643DB == 0) {
        shift = 10;
        scale = 1200;
    }
    guRotateF(sp188, 360.0 - (D_80364414 - 180.0), 0.0f, 1.0f, 0.0f);
    guMtxXFMF(sp188, (arg3 - arg5) / scale, 0.0f, (arg4 - arg6) / scale, &ox, &oy, &oz);
    x = 58.0f + ox;
    y = 195.0f + oz;
    D_802FD830[arg2][0].v.ob[0] = x - 2;
    D_802FD830[arg2][0].v.ob[1] = y - 2;
    D_802FD830[arg2][1].v.ob[0] = x - 2;
    D_802FD830[arg2][1].v.ob[1] = y + 1;
    D_802FD830[arg2][2].v.ob[0] = x + 1;
    D_802FD830[arg2][2].v.ob[1] = y + 1;
    D_802FD830[arg2][3].v.ob[0] = x + 1;
    D_802FD830[arg2][3].v.ob[1] = y - 2;
    func_802C1B9C();
    guMtxXFMF(sp188, (arg3 - D_803F7670) / scale, 0.0f, (arg4 - D_803F7678) / scale, &ox, &oy, &oz);
    px = 58.0f + ox;
    py = 195.0f + oz;
    D_802FD830[arg2][4].v.ob[0] = px - 2;
    D_802FD830[arg2][4].v.ob[1] = py - 2;
    D_802FD830[arg2][5].v.ob[0] = px - 2;
    D_802FD830[arg2][5].v.ob[1] = py + 1;
    D_802FD830[arg2][6].v.ob[0] = px + 1;
    D_802FD830[arg2][6].v.ob[1] = py + 1;
    D_802FD830[arg2][7].v.ob[0] = px + 1;
    D_802FD830[arg2][7].v.ob[1] = py - 2;
    gSPMatrix(gfx++, &D_02000000[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gDPPipeSync(gfx++);
    gDPSetTextureLOD(gfx++, G_TL_TILE);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureImage(gfx++, G_IM_FMT_IA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_802FC5B0));
    gDPSetTile(gfx++, G_IM_FMT_IA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPLoadSync(gfx++);
    gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 0x7F, 0x400);
    gDPSetTextureImage(gfx++, G_IM_FMT_IA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_802FC6B0));
    gDPTileSync(gfx++);
    gDPSetTile(gfx++, G_IM_FMT_IA, G_IM_SIZ_16b, 0, 0x100, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPLoadSync(gfx++);
    gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 0x3FF, 0x200);
    gDPSetTile(gfx++, G_IM_FMT_IA, G_IM_SIZ_8b, 2, 0, G_TX_RENDERTILE, 0, G_TX_MIRROR, 4, G_TX_NOLOD, G_TX_MIRROR, 4,
               G_TX_NOLOD);
    gDPSetTileSize(gfx++, G_TX_RENDERTILE, 2, 2, 0x3E, 0x3E);
    gDPSetTile(gfx++, G_IM_FMT_IA, G_IM_SIZ_4b, 4, 0x100, G_TX_RENDERTILE + 1, 0, G_TX_MIRROR, 6, shift, G_TX_MIRROR,
               6, shift);
    gDPSetTileSize(gfx++, G_TX_RENDERTILE + 1, 2, 2, 0xFC, 0xFC);
    gDPSetCycleType(gfx++, G_CYC_2CYCLE);
    gDPSetRenderMode(gfx++, G_RM_PASS, G_RM_CLD_SURF2);
    gDPSetCombineLERP(gfx++, PRIMITIVE, SHADE, TEXEL1_ALPHA, SHADE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED, 0, 0,
                      0, COMBINED);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, (D_80367BD6 < 0x3F) ? D_80367BD6 : 0x3F);
    gSPVertex(gfx++, osVirtualToPhysical(D_802FD7B0), 4, 0);
    gSP1Triangle(gfx++, 0, 1, 2, 0);
    gSP1Triangle(gfx++, 0, 2, 3, 0);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetCombineLERP(gfx++, 0, 0, 0, SHADE, 0, 0, 0, PRIMITIVE, 0, 0, 0, SHADE, 0, 0, 0, PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_80367BD6);
    if (D_803643DB != 0) {
        gDPSetRenderMode(gfx++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
        if ((D_803156C4 % 40 > 20 || shift != 10) && x >= 0x22 && x < 0x53 && y >= 0xAB && y < 0xDC) {
            gSPVertex(gfx++, osVirtualToPhysical(D_802FD830[arg2]), 4, 0);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
        }
        if (px >= 0x22 && px < 0x53 && py >= 0xB0 && py < 0xD7 && D_803F7660 != 9999999) {
            gSPVertex(gfx++, osVirtualToPhysical(&D_802FD830[arg2][4]), 4, 0);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
        }
    }
    angle = func_80284ADC(arg3 >> 5, arg4 >> 5, D_803F767C, D_803F7680);
    guRotateF(sp188, -((360.0 - (D_80364414 - 180.0)) + (angle + 180.0)), 0.0f, 0.0f, 1.0f);
    guTranslateF(sp148, 58.0f, 195.0f, 0.0f);
    guMtxCatF(sp188, sp148, sp188);
    guMtxF2L(sp188, &D_8036E5E0[arg2]);
    gDPPipeSync(gfx++);
    if (D_80367BD6 == 0xFF) {
        gDPSetRenderMode(gfx++, G_RM_AA_OPA_SURF, G_RM_AA_OPA_SURF2);
    } else {
        gDPSetRenderMode(gfx++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
    }
    gSPMatrix(gfx++, osVirtualToPhysical(&D_8036E5E0[arg2]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPVertex(gfx++, osVirtualToPhysical(&D_802FD830[arg2][8]), 4, 0);
    gSP1Triangle(gfx++, 0, 1, 2, 0);
    gSP1Triangle(gfx++, 0, 2, 3, 0);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    if (D_803643DB != 0) {
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
        gDPSetCombineLERP(gfx++, 0, 0, 0, SHADE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, SHADE, TEXEL0, 0, PRIMITIVE, 0);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, (D_80367BD6 >= 0x80) ? 0x7F : D_80367BD6);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_802FD6B0), G_IM_FMT_IA, G_IM_SIZ_8b, 8, 32, 0, G_TX_CLAMP,
                            G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        guRotate(&arg1[23], D_802FD9B0, 0.0f, 0.0f, 1.0f);
        guTranslate(&sp1C8, 58.0f, 195.0f, 0.0f);
        guMtxCatL(&arg1[23], &sp1C8, &arg1[23]);
        D_802FD9B0 += 4.0;
        gSPMatrix(gfx++, &D_02000000[23], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPVertex(gfx++, osVirtualToPhysical(D_802FD7F0), 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 1, 2, 3, 0);
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}

f32 func_80284ADC(s16 arg0, s16 arg1, s16 arg2, s16 arg3) {
    f32 dist;

    dist = sqrtf((arg2 - arg0) * (arg2 - arg0) + (arg3 - arg1) * (arg3 - arg1));
    if (dist < 1.0) {
        return 0.0f;
    }
    if (arg2 >= arg0 && arg3 >= arg1) {
        return func_802AD7D4((arg2 - arg0) * 65535.9 / dist) / 65536.0 * 360.0;
    }
    if (arg2 >= arg0 && arg3 < arg1) {
        return (func_802AD7D4((arg1 - arg3) * 65535.9 / dist) + 0x4000) / 65536.0 * 360.0;
    }
    if (arg2 < arg0 && arg3 < arg1) {
        return (func_802AD7D4((arg0 - arg2) * 65535.9 / dist) + 0x8000) / 65536.0 * 360.0;
    }
    if (arg2 < arg0 && arg3 >= arg1) {
        return (func_802AD7D4((arg3 - arg1) * 65535.9 / dist) + 0xC000) / 65536.0 * 360.0;
    }
}
