#include "common.h"
#include "game/game.h"
#include "game/level.h"
#include "game/frame.h"
#include "game/audio.h"
#include "functions.h"

typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ u8 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ u8 unk28;
    /* 0x29 */ u8 unk29;
} UnkStruct_8039C960; /* size = 0x2C */

typedef struct {
    /* 0x000 */ Gfx unk0[0x50];
    /* 0x280 */ Vtx unk280[0x32];
    /* 0x5A0 */ s16 unk5A0;
    /* 0x5A4 */ s32 unk5A4;
    /* 0x5A8 */ u8 unk5A8;
    /* 0x5A9 */ u8 unk5A9;
    /* 0x5AC */ s32 unk5AC;
    /* 0x5B0 */ u8 unk5B0;
} UnkStruct_802FE3C0; /* size = 0x5B8 */
SIZE_CHECK_C(UnkStruct_802FE3C0, 0x5B8);

void func_80292DDC(s32 arg0);
extern FrameGame D_02000000;

extern s32 D_802E8BE8;
extern s16 D_803A7410;
extern s16 D_803A7412;
extern u8 D_803F932D;

/* .bss, 0x8039C960-0x8039CA10 (tools/bss_c.py) */
UnkStruct_8039C960 D_8039C960[4];

/* .data, 0x802FE3C0-0x802FE980 (tools/data_c.py) */
extern UnkStruct_802FE3C0 D_802FE3C0[1];
#ifdef ASSET_INIT
#include "hd_code/4DA80/D_802FE3C0.inc"
#else
UnkStruct_802FE3C0 D_802FE3C0[1] = {
    {
        { 0 },
        { 0 },
        320, 0x35B60, 19, 1, 0x13880, 20,
    },
};
#endif

void func_80292240(void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        D_8039C960[i].unk28 = 0;
    }
}

s32 func_80292288(s16 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, u8 arg7, s16 arg8) {
    s32 i;
    u8 found;
    f32 fx;
    f32 fy;
    f32 fz;
    f32 dist;

    i = 0;
    found = 0;
    while (!found && i < 4) {
        if (D_8039C960[i].unk28 != 0) {
            i++;
        } else {
            found = 1;
        }
    }
    if (!found) {
        return 0;
    }
    D_8039C960[i].unk28 = 1;
    D_8039C960[i].unk18 = arg7;
    D_8039C960[i].unk1A = arg8;
    D_8039C960[i].x = arg4;
    D_8039C960[i].y = arg5;
    D_8039C960[i].z = arg6;
    D_8039C960[i].unk1C = arg5;
    D_8039C960[i].unk24 = 0;
    D_8039C960[i].unk29 = 0xFF;
    fx = arg4 - arg1, fy = arg5 - arg2, fz = arg6 - arg3;
    guNormalize(&fx, &fy, &fz);
    D_8039C960[i].unk10 = arg0 * fx;
    D_8039C960[i].unk12 = arg0 * fz;
    D_8039C960[i].unk20 = arg0 * fy;
    dist = sqrtf((arg4 - arg1) * (arg4 - arg1) + (arg6 - arg3) * (arg6 - arg3));
    if (dist < 1.0) {
        dist = 1.0f;
    }
    if (arg4 >= arg1 && arg6 >= arg3) {
        D_8039C960[i].unk14 = func_802AD7D4((arg4 - arg1) * 65535.0 / dist) >> 4;
    }
    if (arg4 >= arg1 && arg6 < arg3) {
        D_8039C960[i].unk14 = (func_802AD7D4((arg3 - arg6) * 65535.0 / dist) >> 4) + 0x400;
    }
    if (arg4 < arg1 && arg6 < arg3) {
        D_8039C960[i].unk14 = (func_802AD7D4((arg1 - arg4) * 65535.0 / dist) >> 4) + 0x800;
    }
    if (arg4 < arg1 && arg6 >= arg3) {
        D_8039C960[i].unk14 = (func_802AD7D4((arg6 - arg3) * 65535.0 / dist) >> 4) + 0xC00;
    }
    return 1;
}

void func_80292830(void) {
    s32 i;
    s16 dy;
    s16 saveA;
    s16 saveB;
    s32 maxX;
    s32 maxZ;

    maxX = D_803BE714 * D_803BE70C;
    maxZ = D_803BE716 * D_803BE710;
    for (i = 0; i < 4; i++) {
        if (D_8039C960[i].unk28 != 0) {
            D_8039C960[i].x += D_8039C960[i].unk10;
            D_8039C960[i].z += D_8039C960[i].unk12;
            D_8039C960[i].y = D_8039C960[i].unk1C + D_8039C960[i].unk20 * D_8039C960[i].unk24 +
                              -12.0 * D_8039C960[i].unk24 * D_8039C960[i].unk24;
            dy = D_8039C960[i].unkC - D_8039C960[i].y;
            if (dy >= 0x400) {
                dy = 0x3FF;
            }
            if (dy < -0x3FF) {
                dy = -0x3FF;
            }
            D_8039C960[i].unk16 = dy;
            if (D_8039C960[i].x >= maxX || D_8039C960[i].z >= maxZ || D_8039C960[i].x < 0 || D_8039C960[i].z < 0 ||
                D_8039C960[i].y < 0) {
                func_80292DDC(i);
            } else {
                func_802CE4F0(D_8039C960[i].x, D_8039C960[i].y, D_8039C960[i].z);
                func_802CDF94(D_802FE3C0[D_8039C960[i].unk18].unk5A0);
                if (D_803F932D != 0) {
                    D_803643D9 = 1;
                }
                saveA = D_803A7410, saveB = D_803A7412;
                func_802CE5BC(D_8039C960[i].x, D_8039C960[i].y, D_8039C960[i].z, D_802FE3C0[D_8039C960[i].unk18].unk5A0,
                              0xCA, 0);
                func_802CDB70(D_802FE3C0[D_8039C960[i].unk18].unk5A0, D_8039C960[i].unk1A);
                if (saveA != D_803A7410 || saveB != D_803A7412) {
                    func_80292DDC(i);
                }
                if (D_8039C960[i].y < func_802CE6F8(D_8039C960[i].x, D_8039C960[i].z, D_8039C960[i].y)) {
                    func_80292DDC(i);
                }
                if (D_802FE3C0[D_8039C960[i].unk18].unk5A9 != 0) {
                    func_802AC61C(D_8039C960[i].x, D_8039C960[i].y, D_8039C960[i].z,
                                  D_802FE3C0[D_8039C960[i].unk18].unk5B0, D_802FE3C0[D_8039C960[i].unk18].unk5AC);
                }
                D_8039C960[i].unkC = D_8039C960[i].y;
                D_8039C960[i].unk29 -= 0x19;
                if (D_8039C960[i].unk29 < 100) {
                    D_8039C960[i].unk29 = 0xFF;
                }
                D_8039C960[i].unk24++;
            }
        }
    }
}

void func_80292DDC(s32 arg0) {
    D_8039C960[arg0].unk28 = 0;
    func_802AC61C(D_8039C960[arg0].x, D_8039C960[arg0].y, D_8039C960[arg0].z, D_802FE3C0[D_8039C960[arg0].unk18].unk5A8,
                  D_802FE3C0[D_8039C960[arg0].unk18].unk5A4);
    D_802E8BE4 = 10;
    D_802E8BE8 = 400;
    func_80260650(D_80367738, 0x10, NULL);
}

void func_80292EB8(Gfx **arg0, Frame *arg1) {
    Gfx *gfx;
    s32 i;
    f32 mf[4][4];
    f32 tmp[4][4];

    gfx = *arg0;
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
    gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    for (i = 0; i < 4; i++) {
        if (D_8039C960[i].unk28 != 0 && D_8039C960[i].unk24 >= 2) {
            guScaleF(mf, 0.5f, 0.5f, 0.35f);
            guRotateF(tmp, D_8039C960[i].unk16 / 4095.0 * 360.0, 1.0f, 0.0f, 0.0f);
            guMtxCatF(mf, tmp, mf);
            guRotateF(tmp, D_8039C960[i].unk14 / 4095.0 * 360.0, 0.0f, 1.0f, 0.0f);
            guMtxCatF(mf, tmp, mf);
            guTranslateF(tmp, D_8039C960[i].x / 32.0f, D_8039C960[i].y / 32.0f, D_8039C960[i].z / 32.0f);
            guMtxCatF(mf, tmp, mf);
            guMtxF2L(mf, &arg1->game.unk2C0[0x29 + i]);
            gSPMatrix(gfx++, &D_02000000.unk2C0[0x29 + i], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            gDPPipeSync(gfx++);
            gDPSetPrimColor(gfx++, 0, 0, D_8039C960[i].unk29, 0, 0, 255);
            gSPDisplayList(gfx++, osVirtualToPhysical(&D_802FE3C0[D_8039C960[i].unk18]));
            gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
        }
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}
