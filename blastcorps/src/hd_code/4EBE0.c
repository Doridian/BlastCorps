#include "common.h"
#include "game/sched.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

/* A spline path: 0x84 bytes. */
typedef struct {
    /* 0x00 */ s16 pts[10][6];
    /* 0x78 */ s32 count;
    /* 0x7C */ f32 tension;
    /* 0x80 */ u8 speed;
    /* 0x81 */ u8 mode;
} UnkStruct_802FE980;

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ s32 unk4;
} UnkStruct_80294B64;

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
} UnkStruct_80293F84;

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
} UnkStruct_802936AC;

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6;
} UnkStruct_802FEDA0; /* size = 0x8 */


void func_802949B0(s32 arg0);
f32 func_80294840(f32 arg0, f32 arg1, f32 arg2, f32 arg3);
extern void func_8026A2E8(f32, f32 *);
extern u8 func_8027EED8(s32, s32, s16 *);
extern void func_802608C8(void *);

extern s16 D_8036E4C8;
extern u8 D_8036E4CA;

extern s32 D_803F7664;
extern s32 D_803F7668;
extern s32 D_803F766C;
void func_80294C50(f32 arg0[4][4], s16 *arg1, s16 *arg2, s16 *arg3, s32 *arg4, s32 arg5);
void func_80294D24(f32 arg0[4][4], s16 *arg1, s16 *arg2, s16 *arg3, s32 *arg4, s32 arg5);

/* .bss, 0x8039CA10-0x8039CA60 (tools/bss_c.py) */
f32 D_8039CA10[4][4];
f32 D_8039CA50;
f32 D_8039CA54;
f32 D_8039CA58;

/* .data, 0x802FE980-0x802FF0D0 (tools/data_c.py) */
UnkStruct_802FE980 D_802FE980[8] = {
    {
        {
            { -260, 0, 340 },
            { -400, 0, -400 },
            { 300, 0, -460 },
            { 600, 0, -40 },
            { 280, 0, 440 },
            { -260, 0, 340 },
        },
        6, 0.5f, 20, 1,
    },
    {
        { { 0 }, { -400, 0, -700 }, { 300, 0, -760 }, { 0 }, { -400, 0, 700 }, { 300, 0, 700 } },
        7, 0.5f, 20, 1,
    },
    { { { 0 }, { 0, 0, -100 }, { 0 }, { 0, 0, 600 } }, 5, 0.5f, 50, 0 },
    { { { 0 }, { 100 }, { 0 }, { -600 } }, 5, 0.5f, 50, 0 },
    {
        { { 0, -7, 0, 0, 0, 3 }, { 0 }, { 0, -7, 0, 0, 0, -3 }, { 0 }, { 0, -7, 0, 0, 0, 3 } }, 5,
        0.5f, 30, 0,
    },
    { { { 0, -7, 0, 3 }, { 0 }, { 0, -7, 0, -3 }, { 0 }, { 0, -7, 0, 3 } }, 5, 0.5f, 30, 0 },
    { { { 0 }, { 0, 0, 0, 0, 90 }, { 0, 0, 0, 0, 180 }, { 0, 0, 0, 0, 270 } }, 5, 0.5f, 30, 0 },
    {
        {
            { 0 },
            { 300 },
            { 300, 0, 0, 0, 180 },
            { 0, 0, 0, 0, 180 },
            { -300, 0, 0, 0, 180 },
            { -300 },
        },
        5, 0.5f, 50, 0,
    },
};
UnkStruct_802FEDA0 D_802FEDA0[0x66] = {
    { 23, 1250, 750, 13 },
    { 23, 950, 750, 5 },
    { 23, 1750, 750, 13 },
    { 23, 2050, 750, 9 },
    { 23, 550, 950, 5 },
    { 23, 850, 950, 13 },
    { 23, 950, 950, 11 },
    { 23, 1250, 950, 7 },
    { 23, 1500, 950, 13 },
    { 23, 1750, 950, 11 },
    { 23, 2050, 950, 7 },
    { 23, 2150, 950, 13 },
    { 23, 2450, 950, 9 },
    { 23, 850, 1050, 7 },
    { 23, 950, 1050, 14 },
    { 23, 1250, 1050, 11 },
    { 23, 1750, 1050, 7 },
    { 23, 2050, 1050, 14 },
    { 23, 2150, 1050, 11 },
    { 23, 550, 1150, 6 },
    { 23, 850, 1150, 11 },
    { 23, 2150, 1150, 7 },
    { 23, 2450, 1150, 10 },
    { 23, 850, 1250, 7 },
    { 23, 1050, 1250, 13 },
    { 23, 1250, 1250, 15 },
    { 23, 1500, 1250, 14 },
    { 23, 1750, 1250, 15 },
    { 23, 1950, 1250, 13 },
    { 23, 2150, 1250, 11 },
    { 23, 550, 1350, 5 },
    { 23, 850, 1350, 11 },
    { 23, 2150, 1350, 7 },
    { 23, 2450, 1350, 9 },
    { 23, 850, 1500, 7 },
    { 23, 1050, 1500, 11 },
    { 23, 1950, 1500, 7 },
    { 23, 2150, 1500, 11 },
    { 23, 550, 1650, 6 },
    { 23, 850, 1650, 11 },
    { 23, 2150, 1650, 7 },
    { 23, 2450, 1650, 10 },
    { 23, 850, 1750, 7 },
    { 23, 1050, 1750, 14 },
    { 23, 1250, 1750, 15 },
    { 23, 1500, 1750, 13 },
    { 23, 1750, 1750, 15 },
    { 23, 1950, 1750, 14 },
    { 23, 2150, 1750, 11 },
    { 23, 550, 1850, 5 },
    { 23, 850, 1850, 11 },
    { 23, 2150, 1850, 7 },
    { 23, 2450, 1850, 9 },
    { 23, 850, 1950, 7 },
    { 23, 950, 1950, 13 },
    { 23, 1250, 1950, 11 },
    { 23, 1750, 1950, 7 },
    { 23, 2050, 1950, 13 },
    { 23, 2150, 1950, 11 },
    { 23, 550, 2050, 6 },
    { 23, 850, 2050, 14 },
    { 23, 950, 2050, 11 },
    { 23, 1250, 2050, 7 },
    { 23, 1500, 2050, 14 },
    { 23, 1750, 2050, 11 },
    { 23, 2050, 2050, 7 },
    { 23, 2150, 2050, 14 },
    { 23, 2450, 2050, 10 },
    { 23, 950, 2250, 6 },
    { 23, 1250, 2250, 14 },
    { 23, 1750, 2250, 14 },
    { 23, 2050, 2250, 10 },
    { 24, 1050, 150, 5 },
    { 24, 1700, 150, 13 },
    { 24, 2350, 150, 9 },
    { 24, 950, 350, 5 },
    { 24, 1050, 350, 14 },
    { 24, 1250, 350, 13 },
    { 24, 1450, 350, 13 },
    { 24, 1700, 350, 14 },
    { 24, 1950, 350, 13 },
    { 24, 2150, 350, 13 },
    { 24, 2350, 350, 14 },
    { 24, 2450, 350, 9 },
    { 24, 950, 650, 7 },
    { 24, 1250, 650, 11 },
    { 24, 1450, 600, 7 },
    { 24, 1950, 600, 11 },
    { 24, 2150, 650, 7 },
    { 24, 2450, 650, 11 },
    { 24, 950, 950, 6 },
    { 24, 1050, 950, 13 },
    { 24, 1250, 950, 14 },
    { 24, 1450, 950, 14 },
    { 24, 1700, 950, 13 },
    { 24, 1950, 950, 14 },
    { 24, 2150, 950, 14 },
    { 24, 2350, 950, 13 },
    { 24, 2450, 950, 10 },
    { 24, 1050, 1150, 6 },
    { 24, 1700, 1150, 14 },
    { 24, 2350, 1150, 10 },
};

void func_80293F84(f32 arg0[4][4], s16 arg1, s16 arg2, s16 *arg3, s16 *arg4, s16 *arg5, UnkStruct_80293F84 *arg6,
                   s32 arg7);
void func_802936AC(f32 arg0[4][4], s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 *arg5, s16 *arg6, s16 *arg7,
                   UnkStruct_802936AC *arg8, s32 arg9);
void func_80294B64(f32 arg0[4][4], s32 arg1, s16 *arg2, s16 *arg3, s16 *arg4, UnkStruct_80294B64 *arg5, s32 arg6);

void func_802933A0(s32 arg0, s32 arg1, s32 arg2, s32 arg3, Mtx *arg4, void *arg5, Gfx *arg6, Gfx *arg7, s32 arg8,
                   s32 arg9, s32 arg10, s32 arg11) {
    s16 spBE;
    s16 spBC;
    s16 spBA;
    f32 sp78[4][4];
    f32 sp38[4][4];

    spBE = 0;
    spBC = 0;
    spBA = 0;
    switch (arg3) {
        case 1:
            func_80294C50(sp78, &spBE, &spBC, &spBA, arg5, arg8);
            break;
        case 2:
            func_80294D24(sp78, &spBE, &spBC, &spBA, arg5, arg8);
            break;
        case 3:
            func_80293F84(sp78, arg0 >> 5, arg2 >> 5, &spBE, &spBC, &spBA, arg5, arg8);
            break;
        case 4:
            func_802936AC(sp78, arg0 >> 5, arg2 >> 5, arg9 >> 5, arg11 >> 5, &spBE, &spBC, &spBA, arg5, arg8);
            break;
        case 5:
            func_80294B64(sp78, 90, &spBE, &spBC, &spBA, arg5, arg8);
            break;
        default:
            guTranslateF(sp78, 0.0f, 0.0f, 0.0f);
            break;
    }
    if (arg3 == 3 && D_802FE980[arg8].mode == 1) {
        arg1 = 0;
    }
    guTranslateF(sp38, arg0 / 32.0f, arg1 / 32.0f, arg2 / 32.0f);
    guMtxCatF(sp78, sp38, sp78);
    guMtxF2L(sp78, arg4);
    D_803F7664 = (spBE << 5) + arg0;
    D_803F7668 = (spBC << 5) + arg1;
    D_803F766C = (spBA << 5) + arg2;
    gSPMatrix(arg6++, osVirtualToPhysical(arg4), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPMatrix(arg7++, osVirtualToPhysical(arg4), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
}

void func_802936AC(f32 arg0[4][4], s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 *arg5, s16 *arg6, s16 *arg7,
                   UnkStruct_802936AC *arg8, s32 arg9) {
    f32 mf[4][4];
    s32 j;
    u8 found;
    s16 angle;
    s16 px;
    s16 pz;
    s16 dx;
    s16 dz;
    s16 adx;
    s16 adz;
    u8 mask;
    u8 dir;
    s32 opp;
    s16 cur;
    s16 target;
    s16 diff;
    s32 dist;

    j = 0;
    found = 0;
    if (D_8036E4C8 == 0 && D_8036DCD8 != NULL) {
        func_802608C8(D_8036DCD8);
    }
    if (D_802E8BD0 == 0) {
        if (D_8036E4CA != 0) {
            arg8->unk8 = -1;
            switch (arg8->unk0) {
                case 1:
                    arg8->unk0 = 2;
                    break;
                case 2:
                    arg8->unk0 = 1;
                    break;
                case 4:
                    arg8->unk0 = 8;
                    break;
                case 8:
                    arg8->unk0 = 4;
                    break;
            }
        }
        while (j < 0x66 && found == 0) {
            if (D_802FEDA0[j].unk0 == D_802E8BDC && arg3 >= D_802FEDA0[j].unk2 - 5 &&
                arg4 >= D_802FEDA0[j].unk4 - 5 && arg3 < D_802FEDA0[j].unk2 + 5 && arg4 < D_802FEDA0[j].unk4 + 5 &&
                arg8->unk8 != j) {
                found = 1;
            } else {
                j++;
            }
        }
        if (found != 0) {
            arg8->unk8 = j;
            px = D_803643E0 >> 5, pz = D_803643E8 >> 5;
            if (D_8036E4C8 == 0) {
                dx = px - arg3, dz = pz - arg4;
            } else {
                dx = arg3 - px;
                dz = arg4 - pz;
            }
            if (dx < 0) {
                adx = -dx;
            } else {
                adx = dx;
            }
            if (dz < 0) {
                adz = -dz;
            } else {
                adz = dz;
            }
            switch (arg8->unk0) {
                case 1:
                    opp = 2;
                    break;
                case 2:
                    opp = 1;
                    break;
                case 4:
                    opp = 8;
                    break;
                case 8:
                    opp = 4;
                    break;
                default:
                    opp = 0;
                    break;
            }
            if (D_802FEDA0[j].unk6 & opp) {
                mask = D_802FEDA0[j].unk6 & (opp ^ 0xFF);
            } else {
                mask = D_802FEDA0[j].unk6;
            }
            if (dz > 0 && adz >= adx) {
                dir = 1;
            }
            if (dz <= 0 && adz >= adx) {
                dir = 2;
            }
            if (dx > 0 && adx >= adz) {
                dir = 4;
            }
            if (dx <= 0 && adx >= adz) {
                dir = 8;
            }
            if (mask & dir) {
                arg8->unk0 = dir;
            } else {
                if (dir == 1) {
                    if (mask & 4) {
                        arg8->unk0 = 4;
                    } else if (mask & 8) {
                        arg8->unk0 = 8;
                    } else {
                        arg8->unk0 = 2;
                    }
                }
                if (dir == 2) {
                    if (mask & 8) {
                        arg8->unk0 = 8;
                    } else if (mask & 4) {
                        arg8->unk0 = 4;
                    } else {
                        arg8->unk0 = 1;
                    }
                }
                if (dir == 4) {
                    if (mask & 1) {
                        arg8->unk0 = 1;
                    } else if (mask & 2) {
                        arg8->unk0 = 2;
                    } else {
                        arg8->unk0 = 8;
                    }
                }
                if (dir == 8) {
                    if (mask & 2) {
                        arg8->unk0 = 2;
                    } else if (mask & 1) {
                        arg8->unk0 = 1;
                    } else {
                        arg8->unk0 = 4;
                    }
                }
            }
        }
        if (D_8036E4C8 == 0) {
            dist = arg9;
        } else {
            dist = (arg9 * 3) / 2;
        }
        switch (arg8->unk0) {
            case 1:
                *arg5 = arg3 - arg1;
                *arg7 = arg4 - arg2 + dist;
                angle = 270;
                break;
            case 2:
                *arg5 = arg3 - arg1;
                *arg7 = arg4 - arg2 - dist;
                angle = 90;
                break;
            case 4:
                *arg5 = arg3 - arg1 + dist;
                *arg7 = arg4 - arg2;
                angle = 0;
                break;
            case 8:
                *arg5 = arg3 - arg1 - dist;
                *arg7 = arg4 - arg2;
                angle = 180;
                break;
        }
        *arg6 = 0;
        cur = (arg8->unk4 << 16) / 360;
        target = (angle << 16) / 360;
        diff = target - cur;
        if (((diff > 0) ? diff : -diff) < 2000) {
            arg8->unk4 = angle;
        } else {
            if (diff > 0) {
                arg8->unk4 += 10;
            } else {
                arg8->unk4 -= 10;
            }
            if (arg8->unk4 >= 360) {
                arg8->unk4 -= 360;
            }
            if (arg8->unk4 < 0) {
                arg8->unk4 += 360;
            }
        }
    } else {
        *arg5 = arg3 - arg1;
        *arg6 = 0;
        *arg7 = arg4 - arg2;
    }
    guRotateF(arg0, arg8->unk4, 0.0f, 1.0f, 0.0f);
    if (D_8036E4C8 == 0 || (D_803156C4 & 0xF) >= 7) {
        guTranslateF(mf, *arg5, *arg6, *arg7);
    } else {
        guTranslateF(mf, 0.0f, 20000.0f, 0.0f);
    }
    guMtxCatF(arg0, mf, arg0);
}

void func_80293F84(f32 arg0[4][4], s16 arg1, s16 arg2, s16 *arg3, s16 *arg4, s16 *arg5, UnkStruct_80293F84 *arg6,
                   s32 arg7) {
    s32 cur;
    s32 n;
    f32 rx;
    f32 ry;
    f32 rz;
    f32 mf[4][4];
    u8 idx[4];
    f32 p0;
    f32 p1;
    f32 p2;
    f32 p3;

    func_802949B0(arg7);
    if (D_802E8BD0 == 0) {
        arg6->unk4 += D_802FE980[arg7].speed;
        if (arg6->unk4 >= 1000) {
            arg6->unk4 = 0;
            arg6->unk0++;
            if (arg6->unk0 >= D_802FE980[arg7].count - 1) {
                arg6->unk0 = 0;
            }
        }
    }
    cur = arg6->unk0;
    n = D_802FE980[arg7].count - 1;
    if (cur <= 0) {
        idx[0] = n + cur - 1;
    } else {
        idx[0] = cur - 1;
    }
    idx[1] = cur;
    if (cur + 1 >= n) {
        idx[2] = cur - n + 1;
    } else {
        idx[2] = cur + 1;
    }
    if (cur + 2 >= n) {
        idx[3] = cur - n + 2;
    } else {
        idx[3] = cur + 2;
    }
    D_8039CA50 = (f32) arg6->unk4 / 1000.0;
    D_8039CA54 = D_8039CA50 * D_8039CA50;
    D_8039CA58 = D_8039CA54 * D_8039CA50;
    p0 = D_802FE980[arg7].pts[idx[0]][3];
    p1 = D_802FE980[arg7].pts[idx[1]][3];
    p2 = D_802FE980[arg7].pts[idx[2]][3];
    p3 = D_802FE980[arg7].pts[idx[3]][3];
    func_8026A2E8(p0, &p1);
    func_8026A2E8(p1, &p2);
    func_8026A2E8(p2, &p3);
    rx = func_80294840(p0, p1, p2, p3);
    p0 = D_802FE980[arg7].pts[idx[0]][4];
    p1 = D_802FE980[arg7].pts[idx[1]][4];
    p2 = D_802FE980[arg7].pts[idx[2]][4];
    p3 = D_802FE980[arg7].pts[idx[3]][4];
    func_8026A2E8(p0, &p1);
    func_8026A2E8(p1, &p2);
    func_8026A2E8(p2, &p3);
    ry = func_80294840(p0, p1, p2, p3);
    p0 = D_802FE980[arg7].pts[idx[0]][5];
    p1 = D_802FE980[arg7].pts[idx[1]][5];
    p2 = D_802FE980[arg7].pts[idx[2]][5];
    p3 = D_802FE980[arg7].pts[idx[3]][5];
    func_8026A2E8(p0, &p1);
    func_8026A2E8(p1, &p2);
    func_8026A2E8(p2, &p3);
    rz = func_80294840(p0, p1, p2, p3);
    *arg3 = func_80294840(D_802FE980[arg7].pts[idx[0]][0], D_802FE980[arg7].pts[idx[1]][0],
                        D_802FE980[arg7].pts[idx[2]][0], D_802FE980[arg7].pts[idx[3]][0]);
    *arg5 = func_80294840(D_802FE980[arg7].pts[idx[0]][2], D_802FE980[arg7].pts[idx[1]][2],
                        D_802FE980[arg7].pts[idx[2]][2], D_802FE980[arg7].pts[idx[3]][2]);
    switch (D_802FE980[arg7].mode) {
        case 0:
            *arg4 = func_80294840(D_802FE980[arg7].pts[idx[0]][1], D_802FE980[arg7].pts[idx[1]][1],
                                  D_802FE980[arg7].pts[idx[2]][1], D_802FE980[arg7].pts[idx[3]][1]);
            break;
        case 1:
            func_8027EED8(*arg3 + arg1, *arg5 + arg2, arg4);
            break;
    }
    guRotateF(mf, rx, 1.0f, 0.0f, 0.0f);
    guRotateF(arg0, ry, 0.0f, 1.0f, 0.0f);
    guMtxCatF(mf, arg0, mf);
    guRotateF(arg0, rz, 0.0f, 0.0f, 1.0f);
    guMtxCatF(mf, arg0, mf);
    guTranslateF(arg0, *arg3, *arg4, *arg5);
    guMtxCatF(mf, arg0, arg0);
}

f32 func_80294840(f32 arg0, f32 arg1, f32 arg2, f32 arg3) {
    s32 a;
    s32 b;
    s32 c;
    s32 d;

    a = D_8039CA10[0][0] * arg0 + D_8039CA10[1][0] * arg1 + D_8039CA10[2][0] * arg2 + D_8039CA10[3][0] * arg3;
    b = D_8039CA10[0][1] * arg0 + D_8039CA10[1][1] * arg1 + D_8039CA10[2][1] * arg2 + D_8039CA10[3][1] * arg3;
    c = D_8039CA10[0][2] * arg0 + D_8039CA10[1][2] * arg1 + D_8039CA10[2][2] * arg2 + D_8039CA10[3][2] * arg3;
    d = D_8039CA10[0][3] * arg0 + D_8039CA10[1][3] * arg1 + D_8039CA10[2][3] * arg2 + D_8039CA10[3][3] * arg3;
    return a * D_8039CA58 + D_8039CA54 * b + D_8039CA50 * c + d;
}

void func_802949B0(s32 arg0) {
    f32 t;

    t = D_802FE980[arg0].tension;
    D_8039CA10[0][0] = -t;
    D_8039CA10[0][1] = t * 2.0;
    D_8039CA10[0][2] = -t;
    D_8039CA10[0][3] = 0.0f;
    D_8039CA10[1][0] = 2.0 - t;
    D_8039CA10[1][1] = t - 3.0;
    D_8039CA10[1][2] = 0.0f;
    D_8039CA10[1][3] = 1.0f;
    D_8039CA10[2][0] = t - 2.0;
    D_8039CA10[2][1] = 3.0 - t * 2.0;
    D_8039CA10[2][2] = t;
    D_8039CA10[2][3] = 0.0f;
    D_8039CA10[3][0] = t;
    D_8039CA10[3][1] = -t;
    D_8039CA10[3][2] = 0.0f;
    D_8039CA10[3][3] = 0.0f;
}

void func_80294B64(f32 arg0[4][4], s32 arg1, s16 *arg2, s16 *arg3, s16 *arg4, UnkStruct_80294B64 *arg5, s32 arg6) {
    if (arg5->unk0 == 0) {
        arg5->unk0 = arg6 + D_803156C4;
    }
    if (D_803156C4 > arg5->unk0 && D_802E8BD0 == 0) {
        arg5->unk4++;
        if (arg5->unk4 > arg1) {
            arg5->unk4 = arg1;
        }
    }
    guRotateF(arg0, -arg5->unk4, 0.0f, 1.0f, 0.0f);
    *arg2 = 0;
    *arg3 = 0;
    *arg4 = 0;
}

void func_80294C50(f32 arg0[4][4], s16 *arg1, s16 *arg2, s16 *arg3, s32 *arg4, s32 arg5) {
    f32 s;

    if (D_802E8BD0 == 0) {
        *arg4 += 1;
    }
    s = sinf(*arg4 / 5.0f);
    *arg1 = 0;
    *arg3 = 0;
    *arg2 = arg5 * s;
    guTranslateF(arg0, 0.0f, *arg2, 0.0f);
}

void func_80294D24(f32 arg0[4][4], s16 *arg1, s16 *arg2, s16 *arg3, s32 *arg4, s32 arg5) {
    f32 mf[4][4];
    f32 x;
    f32 y;
    f32 z;

    if (D_802E8BD0 == 0) {
        *arg4 += arg5;
    }
    guRotateF(mf, *arg4, 0.2f, 0.7f, 0.1f);
    guMtxXFMF(mf, 90.0f, 0.0f, 0.0f, &x, &y, &z);
    *arg1 = x;
    *arg2 = y;
    *arg3 = z;
    guTranslateF(arg0, x, y, z);
}
