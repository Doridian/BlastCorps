#include "common.h"
#include "game/sched.h"
#include "game/level.h"
#include "game/vehicle.h"
#include "game/frame.h"
#include "game/game.h"

/* A recorded replay frame: 0x14 bytes. */
typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
} UnkStruct_8039CA68;

typedef struct {
    /* 0x00 */ u32 from;
    /* 0x04 */ u32 to;
} UnkStruct_802FF11C;

typedef struct {
    /* 0x00 */ s32 x[4];
    /* 0x10 */ s32 z[4];
    /* 0x20 */ s8 list[8];
    /* 0x28 */ u8 unk28;
} UnkStruct_802FF150;

extern s32 func_802AC4C4(s32, s32, s32, s32, s32, s32, s32, s32);
extern FrameGame D_02000000;

void func_80295394(s32 *arg0, s32 *arg1, s32 *arg2, s16 *arg3, s16 *arg4, s16 *arg5);
extern void func_802AA6D0(s32, s32, s32, s16, s32, s32, s32, Mtx *);
s16 func_80295924(s16 arg0, s16 arg1, f32 arg2);
extern void func_8029A7E4(char *, ...);
extern s32 func_80286038(s32);

extern u16 D_803C30A8[];

/* .bss, 0x8039CA60-0x8039CA90 (tools/bss_c.py) */
u8 D_8039CA60;
u8 D_8039CA61;
u8 D_8039CA62;
UnkStruct_8039CA68 *D_8039CA68[2];
s32 D_8039CA70[2];
s32 D_8039CA78;
u8 D_8039CA7C;
u8 D_8039CA7D;
u8 D_8039CA7E;
s32 D_8039CA80;
s32 D_8039CA84;
u32 D_8039CA88;
u8 D_8039CA8C;

/* .data, 0x802FF0D0-0x802FF180 (tools/data_c.py) */
s32 D_802FF0D0[0x13] = {
    20000, 7000, 26000, 11000, 10000, 17000, 7000, 15000, 13000, 17000, 16000, 35000, 0, 17000,
    22000, 13000, 8500, 35000, 35000,
};
UnkStruct_802FF11C D_802FF11C[6] = {
    { 0x552078, 0x5049D8 },
    { 0x5049D8, 0x5049D8 },
    { 0x553078, 0x5049D8 },
    { 0xC192078, 0xC1849D8 },
    { 0xC1849D8, 0xC1849D8 },
    { 0xC193078, 0xC1849D8 },
};
u8 D_802FF14C[4] = { 0 };
UnkStruct_802FF150 D_802FF150[1] = {
    {
        { 0x28A00, 0x28A00, 0x32C80, 0x32C80 }, { 0x12340, 0x1B580, 0x12340, 0x1B580 },
        { 1, 2, 4, 5, 8, -1 }, 9,
    },
};

void func_80294E30(void) {
    D_8039CA68[0] = (UnkStruct_8039CA68 *) 0x80055400;
    D_8039CA68[1] = (UnkStruct_8039CA68 *) 0x80065400;
    D_8039CA88 = func_80286038(0xFFFF) - 1;
    D_8039CA7D = 0;
}

void func_80294E88(void) {
    D_8039CA70[1] = 0;
    D_8039CA80 = -1;
    D_8039CA62 = 1;
    D_8039CA8C = 0;
}

void func_80294EB8(void) {
    if (D_8039CA7D != 0) {
        D_8039CA61 = 1;
        D_8039CA84 = -1;
        D_8039CA78 = 0;
        D_8039CA7E = D_8039CA7C;
    }
}

void func_80294F00(void) {
    if (D_80364A90 & 0x104) {
        if (D_8039CA80 == -1) {
            D_8039CA80 = D_803156C0;
        }
        if ((u32) D_8039CA70[1] < 0xCCC) {
            D_8039CA68[1][D_8039CA70[1]].x = D_803643E0;
            D_8039CA68[1][D_8039CA70[1]].y = D_803643E4;
            D_8039CA68[1][D_8039CA70[1]].z = D_803643E8;
            D_8039CA68[1][D_8039CA70[1]].unkC = D_803ED390[0];
            D_8039CA68[1][D_8039CA70[1]].unkE = D_803ED390[1];
            D_8039CA68[1][D_8039CA70[1]].unk10 = D_803ED390[2];
            D_8039CA68[1][D_8039CA70[1]].unk12 = D_803156C0 - D_8039CA80;
            D_8039CA70[1]++;
        } else {
            D_8039CA8C = 1;
            func_8029A7E4("OVERRUN GHOST DIGGER ARRAY\n");
        }
    }
}

void func_80295120(Gfx **arg0, FrameGame *arg1) {
    Gfx *gfx;
    s32 x;
    s32 y;
    s32 z;
    s16 rx;
    s16 ry;
    s16 rz;

    gfx = *arg0;
    if (D_8039CA61 != 0) {
        func_80295394(&x, &y, &z, &rx, &ry, &rz);
        func_802AA6D0(x, y, z, rx, ry, rz, D_802FF0D0[D_8039CA7C], &arg1->unk11C0[12]);
        gSPSegment(gfx++, 6, osVirtualToPhysical(D_803BDB04));
        gSPSegment(gfx++, 7, osVirtualToPhysical(D_803BDB00));
        gSPMatrix(gfx++, &D_02000000.unk11C0[12], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
        gDPSetEnvColor(gfx++, 0, 0, 0, 255);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, 100);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPDisplayList(gfx++, osVirtualToPhysical(D_803BDB08));
        gSPMatrix(gfx++, &D_02000000.unk0[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}

void func_80295394(s32 *arg0, s32 *arg1, s32 *arg2, s16 *arg3, s16 *arg4, s16 *arg5) {
    u8 found;
    s32 t;
    s32 j;
    s32 span;
    s32 elapsed;
    f32 frac;

    t = D_803156C0 - D_8039CA84;
    if (D_80364A90 == 0x2000) {
        *arg0 = D_8039CA68[0][0].x;
        *arg1 = D_8039CA68[0][0].y;
        *arg2 = D_8039CA68[0][0].z;
        *arg3 = D_8039CA68[0][0].unkC;
        *arg4 = D_8039CA68[0][0].unkE;
        *arg5 = D_8039CA68[0][0].unk10;
        D_8039CA84 = D_803156C0;
        return;
    }
    found = 0;
    while (D_8039CA78 < D_8039CA70[0] - 2 && found == 0) {
        if (t >= D_8039CA68[0][D_8039CA78].unk12 && t < D_8039CA68[0][D_8039CA78 + 1].unk12) {
            found = 1;
        } else {
            D_8039CA78++;
        }
    }
    found = 0;
    j = D_8039CA78 + 1;
    while (j < D_8039CA70[0] - 2 && found == 0) {
        if (t <= D_8039CA68[0][j].unk12) {
            found = 1;
        } else {
            j++;
        }
    }
    span = D_8039CA68[0][j].unk12 - D_8039CA68[0][D_8039CA78].unk12;
    if (j >= D_8039CA70[0] - 2) {
        elapsed = D_8039CA68[0][j].unk12 - D_8039CA68[0][D_8039CA78].unk12;
    } else {
        elapsed = t - D_8039CA68[0][D_8039CA78].unk12;
    }
    frac = (f32) elapsed / (f32) span;
    *arg0 = (D_8039CA68[0][j].x - D_8039CA68[0][D_8039CA78].x) * frac + D_8039CA68[0][D_8039CA78].x;
    *arg1 = (D_8039CA68[0][j].y - D_8039CA68[0][D_8039CA78].y) * frac + D_8039CA68[0][D_8039CA78].y;
    *arg2 = (D_8039CA68[0][j].z - D_8039CA68[0][D_8039CA78].z) * frac + D_8039CA68[0][D_8039CA78].z;
    *arg3 = func_80295924(D_8039CA68[0][D_8039CA78].unkC, D_8039CA68[0][j].unkC, frac);
    *arg4 = func_80295924(D_8039CA68[0][D_8039CA78].unkE, D_8039CA68[0][j].unkE, frac);
    *arg5 = func_80295924(D_8039CA68[0][D_8039CA78].unk10, D_8039CA68[0][j].unk10, frac);
}

s16 func_80295924(s16 arg0, s16 arg1, f32 arg2) {
    s16 d;

    d = arg1 - arg0;
    if (d >= -0x800) {
        if (d > 0x800) {
            d -= 0xFFF;
        }
    } else {
        d += 0xFFF;
    }
    arg2 *= d;
    arg2 += arg0;
    if (arg2 < 0.0) {
        arg2 += 4095.0;
    }
    if (arg2 > 4095.0) {
        arg2 -= 4095.0;
    }
    return arg2;
}

void func_80295A20(u32 arg0) {
    u8 *dst;
    u8 *src;
    s32 i;

    if (arg0 <= D_8039CA88) {
        if (D_8039CA8C == 0) {
            D_8039CA7D = 1;
        } else {
            D_8039CA7D = 0;
        }
        dst = (u8 *) D_8039CA68[0];
        src = (u8 *) D_8039CA68[1];
        for (i = 0; i < 0x10000; i++) {
            dst[i] = src[i];
        }
        D_8039CA7C = D_803643D4;
        D_8039CA70[0] = D_8039CA70[1];
        D_8039CA88 = arg0;
    }
}

void func_80295AE0(Gfx *gfx, Gfx *end) {
    s8 cmd;
    s32 sft;
    u8 found;
    s32 i;
    u32 w1;

    while (gfx != end) {
        switch (cmd = gfx->words.w0 >> 24) {
            case (s8) G_SETCOMBINE:
                gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
                break;
            case (s8) G_SETOTHERMODE_L:
                sft = (gfx->words.w0 >> 8) & 0xFFFF;
                if (sft == G_MDSFT_RENDERMODE) {
                    w1 = gfx->words.w1;
                    i = 0;
                    found = 0;
                    do {
                        if (D_802FF11C[i].from == w1) {
                            found = 1;
                        } else {
                            i++;
                        }
                    } while (!found && i < 6);
                    if (!found) {
                        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "found", "ghostdigger.c", 0x152);
                    }
                    gfx->words.w1 = D_802FF11C[i].to;
                }
                gfx++;
                break;
            default:
                gfx++;
                break;
        }
    }
}

void func_80295C70(u8 arg0, s32 arg1, s32 arg2) {
    s32 i;
    s32 j;

    for (i = 0; i < 1; i++) {
        if (D_802FF150[i].unk28 == arg0 &&
            (func_802AC4C4(arg1, arg2, D_802FF150[i].x[0], D_802FF150[i].z[0], D_802FF150[i].x[1],
                           D_802FF150[i].z[1], D_802FF150[i].x[2], D_802FF150[i].z[2]) != 0 ||
             func_802AC4C4(arg1, arg2, D_802FF150[i].x[0], D_802FF150[i].z[0], D_802FF150[i].x[2],
                           D_802FF150[i].z[2], D_802FF150[i].x[3], D_802FF150[i].z[3]) != 0)) {
            j = 0;
            while (D_802FF150[i].list[j] != -1) {
                D_803C30A8[j] = D_802FF150[i].list[j];
                j++;
            }
            D_803C30A8[j] = 0xFFFF;
        }
    }
}
