#include "common.h"
#include "game/sched.h"
#include "game/audio.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "functions.h"

extern s32 D_803649E8;

f32 func_8027DB5C(s32 *arg0, s32 *arg1, s32 arg2);
f32 func_8027DD88(s32 arg0, s32 arg1, s32 *arg2, s32 *arg3);
s32 func_8027E164(s32 arg0, s32 arg1, s32 *arg2, s32 *arg3);
f32 func_8027E228(u8);

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u8 unk12[2];
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ u8 unk24;
    /* 0x25 */ u8 unk25;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ u8 unk30;
    /* 0x31 */ u8 unk31;
    /* 0x32 */ u8 unk32[2];
} UnkStruct_802FC3F0; /* size = 0x34 */

void func_802802D4(Vtx *arg0, s32 arg1, s32 arg2, s32 arg3);
void func_8028072C(Vtx *arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s16 arg6);


typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u8 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ u8 unk16;
    /* 0x17 */ u8 unk17;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ u8 unk20;
    /* 0x21 */ u8 unk21;
} UnkStruct_802FC494; /* size = 0x22 */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
} UnkStruct_8036DCE0; /* size = 0xC */


extern s32 D_803F9320;
extern s32 D_803F9324;

/* 0x50-byte records: a position and the four vertices of a quad placed there. */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[2];
    /* 0x08 */ Vtx unk8[4];
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u8 unk49[7];
} UnkStruct_8036E380; /* size = 0x50 */

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
} UnkStruct_802FC520; /* size = 0x8 */

extern s16 D_80367BD6;
/* Segment 2 base, reached through a relocation, not a constant. */
extern Mtx D_02000000[];
extern Gfx D_802FFF38[];
extern Gfx D_80300A68[];
extern u8 D_803643D6;


void func_8027DA10(s32 arg0, s32 arg1, s32 arg2);

/* .bss, 0x8036DCA0-0x8036E5E0 (tools/bss_c.py) */
Vtx *D_8036DCA0[2];
Vtx *D_8036DCA8[2];
s32 D_8036DCB0;
u8 *D_8036DCB8[3];
Gfx *D_8036DCC8[2];
u8 D_8036DCD0;
s16 D_8036DCD2;
u8 D_8036DCD4;
u8 D_8036DCD5;
u8 D_8036DCD6;
u8 D_8036DCD7;
SndState *D_8036DCD8;
UnkStruct_8036DCE0 D_8036DCE0[12];     /* (one per D_8036DD70 quad; splat made D_8036DCEC, D_8036DCF0 of it) */
Vtx D_8036DD70[2][12][4];
u8 D_8036E370;
s32 D_8036E374;
u8 *D_8036E378;
UnkStruct_8036E380 D_8036E380[4];      /* (splat made D_8036E3D0 of 1-3) */
s32 D_8036E4C0;
u8 *D_8036E4C4;
s16 D_8036E4C8;
u8 D_8036E4CA;
u8 *D_8036E4CC;
s16 D_8036E4D0;
u8 D_8036E4D2;
u8 D_8036E4D3;
u32 D_8036E4D4;
Mtx D_8036E4D8[2][2];
f32 D_8036E5D8[2];

/* .data, 0x802FC3F0-0x802FC5B0 (tools/data_c.py) */
UnkStruct_802FC3F0 D_802FC3F0[3] = {
    {
        54, 20, 20, 0, 1649, 1759, 3779, 5061, 258, 40, 33, { 0 }, 20.0f, 14.0f, 100.0f, 73.0f, 0,
        0, 0, 0xC080000, 0x104A50, 1, 1, { 0 },
    },
    {
        50, 20, 20, 0, -20000, -20000, 20000, 20000, 10, 17, 24, { 0 }, 20.0f, 14.0f, 100.0f,
        73.0f, 0, 0, 0, 0xC080000, 0x112048, 0, 0, { 0 },
    },
    {
        38, 20, 20, 0, -20000, -20000, 20000, 20000, -10, 17, 24, { 0 }, 20.0f, 14.0f, 100.0f,
        73.0f, 0, 0, 0, 0xC080000, 0x112048, 0, 0, { 0 },
    },
};
s16 D_802FC48C[4] = { 1610, 1587, 1588 };
UnkStruct_802FC494 D_802FC494[4] = {
    {
        29, -2000, -2000, 11000, 14000, 80, 140, 12, 150, 200, 1649, 32, 32, 800, 1200, 800, 1200,
        111, 0,
    },
    {
        3, -2000, -2000, 5000, 8000, 80, 140, 10, 50, 100, 1649, 32, 32, 800, 1200, 800, 1200, 79,
        0,
    },
    {
        33, -1000, -1000, 5000, 6000, 130, 160, 4, 20, 30, 1649, 32, 32, 800, 1200, 800, 1200, 63,
        0,
    },
    {
        30, -2000, -2000, 6000, 7500, 130, 160, 8, 20, 30, 1649, 32, 32, 800, 1200, 800, 1200, 79,
        0,
    },
};
s32 D_802FC51C = 0;
UnkStruct_802FC520 D_802FC520[1] = { { 255, 0, 0, 0 } };
Vtx D_802FC528[4] = {
    { { { 12, 30, 18 }, 0, { 992, 992 }, { 255, 255, 255, 255 } } },
    { { { -18, 30, -15 }, 0, { 0, 992 }, { 255, 255, 255, 255 } } },
    { { { -15, 0, -15 }, 0, { 0 }, { 255, 255, 255, 255 } } },
    { { { 15, 0, 15 }, 0, { 992 }, { 255, 255, 255, 255 } } },
};
Vtx D_802FC568[4] = {
    { { { 260, 70, -5 }, 0, { 0 }, { 255, 255, 255, 255 } } },
    { { { 290, 70, -5 }, 0, { 992 }, { 255, 255, 255, 255 } } },
    { { { 290, 40, -5 }, 0, { 992, 992 }, { 255, 255, 255, 255 } } },
    { { { 260, 40, -5 }, 0, { 0, 992 }, { 255, 255, 255, 255 } } },
};

void func_8027D810(s32 arg0) {
    switch (arg0) {
    case 0:
        func_8027DA10(1, 7, 0x320000);
        break;
    case 4:
        func_8027DA10(1, 7, 0xA00000);
        break;
    case 16:
        func_8027DA10(1, 7, 0x3C0000);
        func_8027DA10(2, 7, 0x3C0000);
        break;
    case 20:
        func_8027DA10(1, 7, 0x820000);
        func_8027DA10(2, 7, 0x820000);
        break;
    case 15:
        func_8027DA10(1, 7, 0x410000);
        func_8027DA10(2, 7, 0x410000);
        break;
    }
}

void func_8027D8F4(s32 arg0, s32 arg1, s32 arg2) {
    s32 i;
    s32 j;
    f32 step;
    f32 s;
    f32 angle;

    j = 1;
    step = 6.28318 / (arg1 + 1);
    angle = D_802FC51C / 20.0;
    for (i = D_802FC51C; i < arg1 + D_802FC51C; i++) {
        s = sinf(angle);
        func_802C1F30(arg0, j++, 0, -s * arg2, 0);
        angle += step;
    }
    D_802FC51C++;
}

void func_8027DA10(s32 arg0, s32 arg1, s32 arg2) {
    f32 sp64;
    f32 sp60;
    f32 sp5C;
    s32 i;
    s16 *sp54;
    s32 pad[5];
    s32 sp30[4];
    s32 sp20[4];

    sp54 = func_802C1EE0(arg0);
    for (i = 0; i < 4; i++) {
        sp30[i] = sp54[i * 2] << 5;
        sp20[i] = sp54[i * 2 + 1] << 5;
    }
    sp64 = func_8027DB5C(sp30, sp20, arg2);
    i = 0;
    sp5C = 3.14159 / (arg1 + 1);
    for (; i < arg1; i++) {
        sp60 = sinf((i + 1) * sp5C);
        func_802C1F30(arg0, i + 1, 0, -sp60 * sp64, 0);
    }
}

f32 func_8027DB5C(s32 *arg0, s32 *arg1, s32 arg2) {
    s32 i;
    f32 max;
    f32 sp24;
    f32 sp20;
    f32 sp1C;
    s32 sp18;

    i = 0;
    max = 0.0f;
    while (&D_80364460[i] != D_803649D0) {
        if ((sp18 = D_80364460[i].type) != 0xFE && (sp18 != 0 || D_803649E8 == 0)) {
            if (func_8027E164(D_80364460[i].x, D_80364460[i].z, arg0, arg1) != 0) {
                if (D_80364460[i].unk70 != 0) {
                    sp1C = func_8027DD88(D_80364460[i].x, D_80364460[i].z, arg0, arg1);
                    if (sp1C <= 0.5) {
                        sp1C = sp1C * 2.0;
                    } else {
                        sp1C = (1.0 - sp1C) * 2.0;
                    }
                    sp20 = func_8027E228(D_80364460[i].type);
                    sp24 = sp1C * sp20;
                    if (max < sp24) {
                        max = sp24;
                    }
                }
            }
        }
        i++;
    }
    return arg2 * max;
}

f32 func_8027DD88(s32 arg0, s32 arg1, s32 *arg2, s32 *arg3) {
    f32 a1;
    f32 a2;
    f32 b1;
    f32 b2;
    f32 c1;
    f32 c2;
    f32 r1;
    f32 r2;
    f32 r3;
    f32 r4;
    f32 denom;
    f32 offset;
    f32 num;
    f32 t;
    f32 ret;
    f32 x1;
    f32 y1;
    f32 x2;
    f32 y2;
    f32 x3;
    f32 y3;
    f32 x4;
    f32 y4;

    x1 = arg2[0];
    y1 = arg3[0];
    x2 = arg2[1];
    y2 = arg3[1];
    x3 = (arg2[1] - arg2[2]) + arg0;
    y3 = (arg3[1] - arg3[2]) + arg1;
    x4 = arg0 - (arg2[1] - arg2[2]);
    y4 = arg1 - (arg3[1] - arg3[2]);
    a1 = y2 - y1, b1 = x1 - x2, c1 = x2 * y1 - x1 * y2;
    r3 = a1 * x3 + b1 * y3 + c1;
    r4 = a1 * x4 + b1 * y4 + c1;
    a2 = y4 - y3;
    b2 = x3 - x4;
    c2 = x4 * y3 - x3 * y4;
    r1 = a2 * x1 + b2 * y1 + c2;
    r2 = a2 * x2 + b2 * y2 + c2;
    denom = a1 * b2 - a2 * b1;
    if (denom < 0.0f) {
        offset = -denom / 2.0f;
    } else {
        offset = denom / 2.0f;
    }
    if (((x2 - x1 >= 0.0f) ? x2 - x1 : -(x2 - x1)) > ((y2 - y1 >= 0.0f) ? y2 - y1 : -(y2 - y1))) {
        num = b1 * c2 - b2 * c1;
        t = ((num < 0.0f) ? num - offset : num + offset) / denom;
        ret = (t - x1) / (x2 - x1);
    } else {
        num = a2 * c1 - a1 * c2;
        t = ((num < 0.0f) ? num - offset : num + offset) / denom;
        ret = (t - y1) / (y2 - y1);
    }
    return ret;
}

s32 func_8027E164(s32 arg0, s32 arg1, s32 *arg2, s32 *arg3) {
    if (func_802AC4C4(arg0, arg1, arg2[0], arg3[0], arg2[1], arg3[1], arg2[2], arg3[2]) != 0) {
        return 1;
    }
    if (func_802AC4C4(arg0, arg1, arg2[0], arg3[0], arg2[2], arg3[2], arg2[3], arg3[3]) != 0) {
        return 1;
    }
    return 0;
}


f32 func_8027E228(u8 arg0) {
    switch (arg0) {
    case 0x0:
        return 0.3f;
    case 0x1:
        return 0.8f;
    case 0x5:
        return 0.8f;
    case 0x4:
        return 0.8f;
    case 0x2:
        return 0.8f;
    case 0x10:
        return 0.4f;
    case 0x3:
        return 0.8f;
    case 0x8:
        return 0.6f;
    case 0xA:
        return 0.6f;
    case 0xD:
        return 0.8f;
    case 0xE:
        return 0.6f;
    case 0xF:
        return 0.6f;
    case 0x9:
        return 0.0f;
    case 0xFF:
        return 0.8f;
    default:
        func_8029A7E4("DIGGER WEIGHT NOT SET\n");
    }
#ifdef TARGET_PC
    return 0.0f;    /* (what was left in $f0) */
#endif
}

void func_8027E344(s32 arg0) {
    s32 dx;
    s32 dz;
    s32 x;
    s32 z;
    s32 i;
    s32 j;
    s32 n;
    s32 t;
    s32 sp24;
    s32 size;
    u8 found;

    n = 0;
    t = 0;
    sp24 = 0;
    found = 0;
    D_8036DCD6 = 0;
    do {
        if (D_802FC3F0[D_8036DCD6].unk0 == arg0) {
            found = 1;
        } else {
            D_8036DCD6++;
        }
    } while (!found && D_8036DCD6 < 3);
    if (!found) {
        D_8036DCD4 = 0;
        return;
    }
    D_8036DCD4 = 1;
    D_8036DCA0[0] = (Vtx *)D_80358070;
    D_80358070 += (D_802FC3F0[D_8036DCD6].unk1 + 1) * (D_802FC3F0[D_8036DCD6].unk2 + 1) * sizeof(Vtx);
    D_8036DCA0[1] = (Vtx *)D_80358070;
    D_8036DCA8[0] = (Vtx *)(D_80358070 += (D_802FC3F0[D_8036DCD6].unk1 + 1) * (D_802FC3F0[D_8036DCD6].unk2 + 1) * sizeof(Vtx));
    D_8036DCA8[1] = (Vtx *)(D_80358070 += 0x12C0);
    D_80358070 += 0x12C0;
    size = D_802FC3F0[D_8036DCD6].unk1 * D_802FC3F0[D_8036DCD6].unk2 * 2 * sizeof(Gfx);
    size += ((D_802FC3F0[D_8036DCD6].unk1 / 8) + 1) * D_802FC3F0[D_8036DCD6].unk2 * sizeof(Gfx);
    size += 0x384 * sizeof(Gfx);
    D_8036DCC8[0] = (Gfx *)D_80358070;
#ifdef TARGET_PC
    D_8036DCC8[1] = port_pool_gfx(D_80358070 + size);       /* (port_game.h) */
    port_pool_gfx(D_80358070 + size);
#else
    D_8036DCC8[1] = (Gfx *)(D_80358070 += size);
    D_80358070 += size;
#endif
    D_8036DCD7 = D_802FC3F0[D_8036DCD6].unk31;
    if (D_8036DCD5 = D_802FC3F0[D_8036DCD6].unk24) {
        D_8036DCB8[0] = D_80358070;
        func_802A0CC8(D_802FC3F0[D_8036DCD6].unk26, 0);
    } else {
        for (i = 0; i < 3; i++) {
            D_8036DCB8[i] = D_80358070;
            func_802A0CC8(D_802FC48C[i], 0);
        }
    }
    D_8036DCD0 = 0;
    D_8036DCD2 = 0;
    dx = (D_802FC3F0[D_8036DCD6].unk8 - D_802FC3F0[D_8036DCD6].unk4) / D_802FC3F0[D_8036DCD6].unk1;
    dz = (D_802FC3F0[D_8036DCD6].unkA - D_802FC3F0[D_8036DCD6].unk6) / D_802FC3F0[D_8036DCD6].unk2;
    x = D_802FC3F0[D_8036DCD6].unk4;
    z = D_802FC3F0[D_8036DCD6].unk6;
    for (i = 0; i <= D_802FC3F0[D_8036DCD6].unk2; i++) {
        sp24 = 0;
        for (j = 0; j <= D_802FC3F0[D_8036DCD6].unk1; j++) {
            D_8036DCA0[0][n].v.ob[0] = x;
            D_8036DCA0[0][n].v.ob[2] = z;
            D_8036DCA0[1][n].v.ob[0] = x;
            D_8036DCA0[1][n].v.ob[2] = z;
            D_8036DCA0[0][n].v.tc[0] = t << 5;
            D_8036DCA0[0][n].v.tc[1] = sp24 << 5;
            D_8036DCA0[1][n].v.tc[0] = t << 5;
            D_8036DCA0[1][n].v.tc[1] = sp24 << 5;
            sp24 ^= 0x1F;
            n++;
            x += dx;
        }
        t ^= 0x1F;
        x = D_802FC3F0[D_8036DCD6].unk4;
        z += dz;
    }
    D_8036DCB0 = 0;
}

void func_8027E9B8(u8 arg0) {
    s32 i;
    s32 j;
    s32 dx;
    s32 dz;
    register f32 s;

    if (D_8036DCD4 != 0 && D_802E8BD0 == 0) {
        D_8036DCB0++;
        D_8036DCD2 += 0xF;
        if (D_8036DCD2 >= 0xFF) {
            D_8036DCD2 = 0;
            D_8036DCD0++;
            if (D_8036DCD0 >= 3) {
                D_8036DCD0 = 0;
            }
        }
    }
    if (D_8036DCD4 != 0) {
        dx = (D_802FC3F0[D_8036DCD6].unk8 - D_802FC3F0[D_8036DCD6].unk4) / D_802FC3F0[D_8036DCD6].unk1;
        for (i = 0; i <= D_802FC3F0[D_8036DCD6].unk1; i++) {
            for (j = 0; j <= D_802FC3F0[D_8036DCD6].unk2; j++) {
                s = sinf((f32)((i + 1) * dx) / D_802FC3F0[D_8036DCD6].unk1C +
                         (f32)D_8036DCB0 / D_802FC3F0[D_8036DCD6].unk14);
                D_8036DCA0[arg0][i + j * (D_802FC3F0[D_8036DCD6].unk2 + 1)].v.ob[1] =
                    D_802FC3F0[D_8036DCD6].unkE * s + D_802FC3F0[D_8036DCD6].unkC;
            }
        }
        dz = (D_802FC3F0[D_8036DCD6].unkA - D_802FC3F0[D_8036DCD6].unk6) / D_802FC3F0[D_8036DCD6].unk2;
        for (i = 0; i <= D_802FC3F0[D_8036DCD6].unk2; i++) {
            for (j = 0; j <= D_802FC3F0[D_8036DCD6].unk1; j++) {
                s = sinf((f32)((i + 1) * dz) / D_802FC3F0[D_8036DCD6].unk20 +
                         (f32)D_8036DCB0 / D_802FC3F0[D_8036DCD6].unk18);
                D_8036DCA0[arg0][(D_802FC3F0[D_8036DCD6].unk2 + 1) * i + j].v.ob[1] =
                    D_802FC3F0[D_8036DCD6].unk10 * s +
                    D_8036DCA0[arg0][(D_802FC3F0[D_8036DCD6].unk2 + 1) * i + j].v.ob[1];
            }
        }
    }
}

u8 func_8027EED8(s16 arg0, s16 arg1, s16 *arg2) {
    register f32 s;
    f32 step;
    f32 t;

    if (D_8036DCD4 == 0) {
        return 0;
    }
    if (arg0 < D_802FC3F0[D_8036DCD6].unk4 || arg0 > D_802FC3F0[D_8036DCD6].unk8 ||
        arg1 < D_802FC3F0[D_8036DCD6].unk6 || arg1 > D_802FC3F0[D_8036DCD6].unkA) {
        *arg2 = D_802FC3F0[D_8036DCD6].unkC;
        return 0;
    }
    step = (f32)(D_802FC3F0[D_8036DCD6].unk8 - D_802FC3F0[D_8036DCD6].unk4) / D_802FC3F0[D_8036DCD6].unk1;
    t = (arg0 - D_802FC3F0[D_8036DCD6].unk4) / step;
    s = sinf((t + 1.0f) * step / D_802FC3F0[D_8036DCD6].unk1C + D_8036DCB0 / D_802FC3F0[D_8036DCD6].unk14);
    *arg2 = D_802FC3F0[D_8036DCD6].unkE * s + D_802FC3F0[D_8036DCD6].unkC;
    step = (f32)(D_802FC3F0[D_8036DCD6].unkA - D_802FC3F0[D_8036DCD6].unk6) / D_802FC3F0[D_8036DCD6].unk2;
    t = (arg1 - D_802FC3F0[D_8036DCD6].unk6) / step;
    s = sinf((t + 1.0f) * step / D_802FC3F0[D_8036DCD6].unk20 + D_8036DCB0 / D_802FC3F0[D_8036DCD6].unk18);
    *arg2 += D_802FC3F0[D_8036DCD6].unk10 * s;
    return 1;
}

void func_8027F1F8(Gfx **arg0, u8 arg1, u8 arg2) {
    Gfx *gfx = *arg0;
    s32 sp130;
    s32 sp12C;
    s32 sp128;
    s32 sp124;
    s32 sp120;
    s32 sp11C;
    s32 sp118;
    u8 done;
    s32 sp110;
    s32 sp10C;
    u8 next;
    s16 minX;
    s16 maxX;
    s16 minY;
    s16 maxY;
    s16 minZ;
    s16 maxZ;
    Gfx *dl;
    Vtx *box;
    s32 count;

    done = 0;
    dl = D_8036DCC8[arg1];
    box = D_8036DCA8[arg1];
    if (D_8036DCD4 == 0 || arg2 != D_8036DCD7) {
        return;
    }
    {
        gDPPipeSync(gfx++);
        if (D_8036DCD5 != 0) {
            gDPSetCycleType(gfx++, G_CYC_1CYCLE);
            gDPSetRenderMode(gfx++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
            gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
            gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LIGHTING | G_TEXTURE_GEN);
            gSPTexture(gfx++, 0x07C0, 0x07C0, 0, G_TX_RENDERTILE, G_ON);
            gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, 0, 0, 0, PRIMITIVE, 0, 0, 0, TEXEL0, 0, 0, 0, PRIMITIVE);
            gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, 0xAA);
            gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8036DCB8[0]), G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                                G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        } else {
            gDPSetCycleType(gfx++, G_CYC_2CYCLE);
            gDPSetRenderMode(gfx++, D_802FC3F0[D_8036DCD6].unk28, D_802FC3F0[D_8036DCD6].unk2C);
            gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
            if (D_802FC3F0[D_8036DCD6].unk30 != 0) {
                gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK);
            } else {
                gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK);
            }
            gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
            gDPSetCombineLERP(gfx++, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, 0, 0, 0, 0, 0, 0, 0, COMBINED, 0, 0,
                              0, PRIMITIVE);
            gDPSetTextureLOD(gfx++, G_TL_TILE);
            if (D_8036DCD0 == 2) {
                next = 0;
            } else {
                next = D_8036DCD0 + 1;
            }
            gDPSetTextureImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_8036DCB8[D_8036DCD0]));
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
            gDPLoadSync(gfx++);
            gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES));
            gDPSetTextureImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, OS_K0_TO_PHYSICAL(D_8036DCB8[next]));
            gDPTileSync(gfx++);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0x100, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
            gDPLoadSync(gfx++);
            gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES));
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, G_TX_CLAMP, 5, G_TX_NOLOD,
                       G_TX_CLAMP, 5, G_TX_NOLOD);
            gDPSetTileSize(gfx++, G_TX_RENDERTILE, 2, 2, 0x7E, 0x7E);
            gDPSetTile(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0x100, G_TX_RENDERTILE + 1, 0, G_TX_CLAMP, 5,
                       G_TX_NOLOD, G_TX_CLAMP, 5, G_TX_NOLOD);
            gDPSetTileSize(gfx++, G_TX_RENDERTILE + 1, 2, 2, 0x7E, 0x7E);
            gDPSetPrimColor(gfx++, 0, D_8036DCD2, 0, 0, 0, 0xAA);
        }
        sp124 = (D_802FC3F0[D_8036DCD6].unk1 + 1) * (D_802FC3F0[D_8036DCD6].unk2 + 1), sp130 = 0;
        sp12C = D_802FC3F0[D_8036DCD6].unk1 + 1;
        sp11C = 0;
        count = 0;
        while (!done) {
            if (D_802FC3F0[D_8036DCD6].unk1 - sp11C + 1 >= 9) {
                sp120 = 8;
            } else {
                sp120 = D_802FC3F0[D_8036DCD6].unk1 - sp11C + 1;
            }
            if (count == 0) {
                gSPDisplayList(gfx++, osVirtualToPhysical(dl));
                gSPVertex(dl++, osVirtualToPhysical(box), 8, 0);
                gSPCullDisplayList(dl++, 0, 7);
                minZ = minY = minX = 0x7FFF;
                maxZ = maxY = maxX = -0x8000;
            }
            gSPVertex(dl++, osVirtualToPhysical(&D_8036DCA0[arg1][sp130]), sp120, 0);
            gSPVertex(dl++, osVirtualToPhysical(&D_8036DCA0[arg1][sp12C]), sp120, 8);
            for (sp110 = 0; sp110 < sp120; sp110++) {
                if (D_8036DCA0[arg1][sp130 + sp110].v.ob[0] < minX) {
                    minX = D_8036DCA0[arg1][sp130 + sp110].v.ob[0];
                }
                if (D_8036DCA0[arg1][sp130 + sp110].v.ob[1] < minY) {
                    minY = D_8036DCA0[arg1][sp130 + sp110].v.ob[1];
                }
                if (D_8036DCA0[arg1][sp130 + sp110].v.ob[2] < minZ) {
                    minZ = D_8036DCA0[arg1][sp130 + sp110].v.ob[2];
                }
                if (D_8036DCA0[arg1][sp130 + sp110].v.ob[0] > maxX) {
                    maxX = D_8036DCA0[arg1][sp130 + sp110].v.ob[0];
                }
                if (D_8036DCA0[arg1][sp130 + sp110].v.ob[1] > maxY) {
                    maxY = D_8036DCA0[arg1][sp130 + sp110].v.ob[1];
                }
                if (D_8036DCA0[arg1][sp130 + sp110].v.ob[2] > maxZ) {
                    maxZ = D_8036DCA0[arg1][sp130 + sp110].v.ob[2];
                }
            }
            for (sp110 = 0; sp110 < sp120; sp110++) {
                if (D_8036DCA0[arg1][sp12C + sp110].v.ob[0] < minX) {
                    minX = D_8036DCA0[arg1][sp12C + sp110].v.ob[0];
                }
                if (D_8036DCA0[arg1][sp12C + sp110].v.ob[1] < minY) {
                    minY = D_8036DCA0[arg1][sp12C + sp110].v.ob[1];
                }
                if (D_8036DCA0[arg1][sp12C + sp110].v.ob[2] < minZ) {
                    minZ = D_8036DCA0[arg1][sp12C + sp110].v.ob[2];
                }
                if (D_8036DCA0[arg1][sp12C + sp110].v.ob[0] > maxX) {
                    maxX = D_8036DCA0[arg1][sp12C + sp110].v.ob[0];
                }
                if (D_8036DCA0[arg1][sp12C + sp110].v.ob[1] > maxY) {
                    maxY = D_8036DCA0[arg1][sp12C + sp110].v.ob[1];
                }
                if (D_8036DCA0[arg1][sp12C + sp110].v.ob[2] > maxZ) {
                    maxZ = D_8036DCA0[arg1][sp12C + sp110].v.ob[2];
                }
            }
            sp118 = 0;
            for (sp110 = 0; sp110 < sp120 - 1; sp110++) {
                gSP1Triangle(dl++, sp118 + 1, sp118, sp118 + 8, 0);
                if (D_8036DCD5 != 0) {
                    func_802802D4(D_8036DCA0[arg1], sp130 + sp118, sp130 + sp118 + 1, sp12C + sp118);
                }
                gSP1Triangle(dl++, sp118 + 8, sp118 + 9, sp118 + 1, 0);
                if (D_8036DCD5 != 0) {
                    func_802802D4(D_8036DCA0[arg1], sp12C + sp118, sp12C + sp118 + 1, sp130 + sp118 + 1);
                }
                sp118++;
            }
            sp10C = sp120 - 1;
            sp11C += sp10C;
            sp130 += sp10C, sp12C += sp10C;
            if (D_802FC3F0[D_8036DCD6].unk1 == sp11C) {
                sp11C = 0;
                sp130++, sp12C++;
            }
            count += sp120 * 2;
            if (count >= 32) {
                gSPEndDisplayList(dl++);
                func_8028072C(box, minX, minY, minZ, maxX, maxY, maxZ);
                count = 0;
                box += 8;
            }
            if (sp12C == sp124) {
                done = 1;
            }
        }
        if (count != 0) {
            gSPEndDisplayList(dl++);
            func_8028072C(box, minX, minY, minZ, maxX, maxY, maxZ);
            box += 8;
        }
        gDPPipeSync(gfx++);
        gDPSetTextureLOD(gfx++, G_TL_LOD);
        gDPPipeSync(gfx++);
        *arg0 = gfx;
    }
}

void func_802802D4(Vtx *arg0, s32 arg1, s32 arg2, s32 arg3) {
    f32 a[3];
    f32 b[3];
    f32 pad[3];
    f32 scale;
    f32 nx;
    f32 ny;
    f32 nz;
    f32 len;

    a[0] = arg0[arg3].v.ob[0] - arg0[arg1].v.ob[0];
    a[1] = arg0[arg3].v.ob[1] - arg0[arg1].v.ob[1];
    a[2] = arg0[arg3].v.ob[2] - arg0[arg1].v.ob[2];
    b[0] = arg0[arg2].v.ob[0] - arg0[arg1].v.ob[0];
    b[1] = arg0[arg2].v.ob[1] - arg0[arg1].v.ob[1];
    b[2] = arg0[arg2].v.ob[2] - arg0[arg1].v.ob[2];
    nx = a[1] * b[2] - a[2] * b[1];
    ny = a[2] * b[0] - a[0] * b[2];
    nz = a[0] * b[1] - a[1] * b[0];
    len = sqrtf(nx * nx + ny * ny + nz * nz);
    if (len < 1.0) {
        len = 1.0f;
    }
    scale = 120.0 / len;
    nx *= scale;
    ny *= scale;
    nz *= scale;
    arg0[arg1].v.cn[0] = (s8)nx;
    arg0[arg1].v.cn[1] = (s8)ny;
    arg0[arg1].v.cn[2] = (s8)nz;
    arg0[arg1].v.cn[3] = 0;
    arg0[arg2].v.cn[0] = (s8)nx;
    arg0[arg2].v.cn[1] = (s8)ny;
    arg0[arg2].v.cn[2] = (s8)nz;
    arg0[arg2].v.cn[3] = 0;
    arg0[arg3].v.cn[0] = (s8)nx;
    arg0[arg3].v.cn[1] = (s8)ny;
    arg0[arg3].v.cn[2] = (s8)nz;
    arg0[arg3].v.cn[3] = 0;
}

void func_8028072C(Vtx *arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s16 arg6) {
    arg0[0].v.ob[0] = arg1;
    arg0[0].v.ob[1] = arg2;
    arg0[0].v.ob[2] = arg3;
    arg0[1].v.ob[0] = arg1;
    arg0[1].v.ob[1] = arg5;
    arg0[1].v.ob[2] = arg3;
    arg0[2].v.ob[0] = arg4;
    arg0[2].v.ob[1] = arg2;
    arg0[2].v.ob[2] = arg3;
    arg0[3].v.ob[0] = arg4;
    arg0[3].v.ob[1] = arg5;
    arg0[3].v.ob[2] = arg3;
    arg0[4].v.ob[0] = arg1;
    arg0[4].v.ob[1] = arg2;
    arg0[4].v.ob[2] = arg6;
    arg0[5].v.ob[0] = arg1;
    arg0[5].v.ob[1] = arg5;
    arg0[5].v.ob[2] = arg6;
    arg0[6].v.ob[0] = arg4;
    arg0[6].v.ob[1] = arg2;
    arg0[6].v.ob[2] = arg6;
    arg0[7].v.ob[0] = arg4;
    arg0[7].v.ob[1] = arg5;
    arg0[7].v.ob[2] = arg6;
}

void func_802807D8(u8 arg0) {
    s32 i;
    s32 j;
    s16 x;
    s16 y;
    s16 z;
    s16 dx;
    s16 dz;
    s32 sp20;
    u8 found;

    D_8036E374 = 0;
    sp20 = 0;
    found = 0;
    D_8036E370 = 0;
    do {
        if (D_802FC494[D_8036E370].unk0 == arg0) {
            found = 1;
        } else {
            D_8036E370++;
        }
    } while (!found && D_8036E370 < 4);
    if (found) {
        D_8036E374 = D_802FC494[D_8036E370].unkE;
        D_8036E378 = D_80358070;
        func_802A0CC8(D_802FC494[D_8036E370].unk14, 0);
        for (i = 0; i < D_8036E374; i++) {
            x = func_8026A828(D_802FC494[D_8036E370].unk2, D_802FC494[D_8036E370].unk6);
            y = func_8026A828(D_802FC494[D_8036E370].unkA, D_802FC494[D_8036E370].unkC);
            z = func_8026A828(D_802FC494[D_8036E370].unk4, D_802FC494[D_8036E370].unk8);
            dx = func_8026A828(D_802FC494[D_8036E370].unk18, D_802FC494[D_8036E370].unk1A);
            dz = func_8026A828(D_802FC494[D_8036E370].unk1C, D_802FC494[D_8036E370].unk1E);
            dx >>= 1, dz >>= 1;
            D_8036DCE0[i].unk6 = dx;
            D_8036DCE0[i].unk8 = dz;
            D_8036DCE0[i].unk2 = x;
            D_8036DCE0[i].unk4 = z;
            D_8036DCE0[i].unk0 = func_8026A828(D_802FC494[D_8036E370].unk10, D_802FC494[D_8036E370].unk12);
            D_8036DCE0[i].unkA = func_8026A828(0, 0xFFF);
            for (j = 0; j < 2; j++) {
                D_8036DD70[j][i][0].v.ob[0] = x - dx;
                D_8036DD70[j][i][0].v.ob[1] = y;
                D_8036DD70[j][i][0].v.ob[2] = z - dz;
                D_8036DD70[j][i][0].v.tc[0] = 0;
                D_8036DD70[j][i][0].v.tc[1] = 0;
                D_8036DD70[j][i][1].v.ob[0] = x + dx;
                D_8036DD70[j][i][1].v.ob[1] = y;
                D_8036DD70[j][i][1].v.ob[2] = z - dz;
                D_8036DD70[j][i][1].v.tc[0] = D_802FC494[D_8036E370].unk16 << 5;
                D_8036DD70[j][i][1].v.tc[1] = 0;
                D_8036DD70[j][i][2].v.ob[0] = x + dx;
                D_8036DD70[j][i][2].v.ob[1] = y;
                D_8036DD70[j][i][2].v.ob[2] = z + dz;
                D_8036DD70[j][i][2].v.tc[0] = D_802FC494[D_8036E370].unk16 << 5;
                D_8036DD70[j][i][2].v.tc[1] = D_802FC494[D_8036E370].unk17 << 5;
                D_8036DD70[j][i][3].v.ob[0] = x - dx;
                D_8036DD70[j][i][3].v.ob[1] = y;
                D_8036DD70[j][i][3].v.ob[2] = z + dz;
                D_8036DD70[j][i][3].v.tc[0] = 0;
                D_8036DD70[j][i][3].v.tc[1] = D_802FC494[D_8036E370].unk17 << 5;
            }
        }
    }
}

/* sx and sz below are never read.  Rare indexes D_802FC494 by the quad
   there (up to 12), not by D_8036E370, which reads past its four entries;
   the port leaves those dead reads out. */
#ifdef TARGET_PC
#define FC494_SPAN(i, hi, lo) 0
#else
#define FC494_SPAN(i, hi, lo) (D_802FC494[i].hi - D_802FC494[i].lo)
#endif

void func_80280F34(Gfx **arg0, u8 arg1) {
    Gfx *gfx = *arg0;
    s32 i;
    s16 sx;
    s16 sz;

    if (D_8036E374 != 0) {
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, D_802FC494[D_8036E370].unk20);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8036E378), G_IM_FMT_IA, G_IM_SIZ_16b,
                            D_802FC494[D_8036E370].unk16, D_802FC494[D_8036E370].unk17, 0, G_TX_CLAMP, G_TX_CLAMP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        for (i = 0; i < D_8036E374; i++) {
            func_802CE65C(D_8036DCE0[i].unk2 << 5, D_8036DCE0[i].unk4 << 5, D_8036DCE0[i].unk0, D_8036DCE0[i].unkA);
            sx = (D_803F9320 >> 5) - D_8036DCE0[i].unk2;
            sz = (D_803F9324 >> 5) - D_8036DCE0[i].unk4;
            D_8036DCE0[i].unk2 = D_803F9320 >> 5;
            D_8036DCE0[i].unk4 = D_803F9324 >> 5;
            if (D_8036DCE0[i].unk2 > D_802FC494[D_8036E370].unk6) {
                D_8036DCE0[i].unk2 = D_802FC494[D_8036E370].unk2;
                sx -= FC494_SPAN(i, unk6, unk2);
            }
            if (D_8036DCE0[i].unk2 < D_802FC494[D_8036E370].unk2) {
                D_8036DCE0[i].unk2 = D_802FC494[D_8036E370].unk6;
                sx += FC494_SPAN(i, unk6, unk2);
            }
            if (D_8036DCE0[i].unk4 > D_802FC494[D_8036E370].unk8) {
                D_8036DCE0[i].unk4 = D_802FC494[D_8036E370].unk4;
                sz -= FC494_SPAN(i, unk8, unk4);
            }
            if (D_8036DCE0[i].unk4 < D_802FC494[D_8036E370].unk4) {
                D_8036DCE0[i].unk4 = D_802FC494[D_8036E370].unk8;
                sz += FC494_SPAN(i, unk8, unk4);
            }
            D_8036DD70[arg1][i][0].v.ob[0] = D_8036DCE0[i].unk2 - D_8036DCE0[i].unk6;
            D_8036DD70[arg1][i][0].v.ob[2] = D_8036DCE0[i].unk4 - D_8036DCE0[i].unk8;
            D_8036DD70[arg1][i][1].v.ob[0] = D_8036DCE0[i].unk2 + D_8036DCE0[i].unk6;
            D_8036DD70[arg1][i][1].v.ob[2] = D_8036DCE0[i].unk4 - D_8036DCE0[i].unk8;
            D_8036DD70[arg1][i][2].v.ob[0] = D_8036DCE0[i].unk2 + D_8036DCE0[i].unk6;
            D_8036DD70[arg1][i][2].v.ob[2] = D_8036DCE0[i].unk4 + D_8036DCE0[i].unk8;
            D_8036DD70[arg1][i][3].v.ob[0] = D_8036DCE0[i].unk2 - D_8036DCE0[i].unk6;
            D_8036DD70[arg1][i][3].v.ob[2] = D_8036DCE0[i].unk4 + D_8036DCE0[i].unk8;
            gSPVertex(gfx++, OS_K0_TO_PHYSICAL(D_8036DD70[arg1][i]), 4, 0);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
        }
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}

void func_80281A70(s32 arg0) {
    s32 i;
    s32 j;

    D_8036E4C0 = 0;
    for (i = 0; i < 1; i++) {
        if (D_802FC520[i].unk0 == arg0) {
            D_8036E380[D_8036E4C0].unk0 = D_802FC520[i].unk2;
            D_8036E380[D_8036E4C0].unk2 = D_802FC520[i].unk4;
            D_8036E380[D_8036E4C0].unk4 = D_802FC520[i].unk6;
            func_8026A5CC((u64 *)D_8036E380[D_8036E4C0].unk8, (u64 *)D_802FC528, sizeof(D_8036E380[0].unk8));
            for (j = 0; j < 4; j++) {
                D_8036E380[D_8036E4C0].unk8[j].v.ob[0] += D_8036E380[D_8036E4C0].unk0;
                D_8036E380[D_8036E4C0].unk8[j].v.ob[1] += D_8036E380[D_8036E4C0].unk2;
                D_8036E380[D_8036E4C0].unk8[j].v.ob[2] += D_8036E380[D_8036E4C0].unk4;
            }
            D_8036E380[D_8036E4C0].unk48 = 1;
            D_8036E4C0++;
        }
    }
    if (D_8036E4C0 != 0) {
        D_8036E4C4 = func_802A0CC8(0x546, 0);
    }
    D_8036E4C8 = 0;
    D_8036E4CA = 0;
}

void func_80281CE4(void) {
    s32 i;
    s32 dist;

    D_8036E4CA = 0;
    if (D_8036E4C8 != 0) {
        D_8036E4C8--;
    }
    for (i = 0; i < D_8036E4C0; i++) {
        if (D_8036E380[i].unk48 != 0) {
            dist = func_8026A6F0(D_803643E0 >> 5, D_803643E4 >> 5, D_803643E8 >> 5, D_8036E380[i].unk0,
                                 D_8036E380[i].unk2, D_8036E380[i].unk4);
            if (dist < 0x28) {
                D_8036E4C8 = 0x190;
                D_8036E4CA = 1;
                D_8036E380[i].unk48 = 0;
                func_80260650(D_80367738, 0xB0, NULL);
                if (D_8036DCD8 == 0) {
                    func_80260650(D_80367738, 0xCF, &D_8036DCD8);
                }
            }
        }
    }
}

void func_80281E44(Gfx **arg0) {
    Gfx *gfx = *arg0;
    s32 i;

    if (D_8036E4C0 != 0) {
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPLoadTextureBlock(gfx++, D_8036E4C4, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_CLAMP, G_TX_CLAMP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        for (i = 0; i < D_8036E4C0; i++) {
            if (D_8036E380[i].unk48 != 0) {
                gSPVertex(gfx++, D_8036E380[i].unk8, 4, 0);
                gSP1Triangle(gfx++, 0, 1, 2, 0);
                gSP1Triangle(gfx++, 0, 2, 3, 0);
            }
        }
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}

void func_802821D0(void) {
    D_8036E4CC = D_80358070;
    func_802A0B00(0xA98, 0);
    D_80358070 += 0x800;
    D_8036E4D0 = 0;
    D_8036E4D2 = 0;
}

void func_80282224(Gfx **arg0, u8 arg1) {
    Gfx *gfx = *arg0;
    u8 sp7B;
    u8 sp7A;

    sp7B = arg1 == 7 || arg1 == 11 || arg1 == 17 || arg1 == 18;
    if (D_8036E4D2 != 0 && !sp7B) {
        D_8036E4D0 -= 10;
        if (D_8036E4D0 < 0) {
            D_8036E4D0 = 0;
        }
    } else {
        if (sp7B) {
            sp7A = !func_8029DBF0(arg1);
        }
        if (sp7B && sp7A) {
            D_8036E4D0 = 0xFF;
            D_8036E4D2 = 1;
        } else {
            D_8036E4D0 = 0;
        }
    }
    if (D_8036E4D0 == 0) {
        D_8036E4D2 = 0;
    }
    if (D_8036E4D2 != 0) {
        gSPMatrix(gfx++, &D_02000000[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, (D_8036E4D0 < D_80367BD6) ? D_8036E4D0 : D_80367BD6);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8036E4CC), G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                            G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPVertex(gfx++, OS_K0_TO_PHYSICAL(D_802FC568), 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}

void func_80282728(void) {
    D_8036E4D3 = 0;
    D_8036E4D4 = 0;
}

void func_8028273C(Gfx **arg0, u8 arg1) {
    Gfx *gfx = *arg0;
    f32 sp9C[4][4];
    f32 sp5C[4][4];
    s32 i;

    if (D_803643D6 != 0) {
        if (D_803EF6FF != 0 && D_8036E4D3 == 0) {
            D_8036E4D3 = 1;
            D_8036E5D8[0] = 0.001f;
            D_8036E4D4 = D_803156C4;
        }
        gDPPipeSync(gfx++);
        gDPSetColorDither(gfx++, G_CD_DISABLE);
        if (D_8036E4D3 != 0) {
            gDPPipeSync(gfx++);
            gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            gDPSetPrimColor(gfx++, 0, 0, 0x37, 0, 0, 0x9B);
            gDPFillRectangle(gfx++, 0, 0, 319, 239);
        }
        for (i = 0; i < D_8036E4D3; i++) {
            guTranslateF(sp9C, D_803EF6DC / 32.0f, D_803EF6E0 / 32.0f, D_803EF6E4 / 32.0f);
            guScaleF(sp5C, D_8036E5D8[i], D_8036E5D8[i], D_8036E5D8[i]);
            D_8036E5D8[i] += 0.06;
            guMtxCatF(sp5C, sp9C, sp9C);
            guMtxF2L(sp9C, &D_8036E4D8[arg1][i]);
            gDPPipeSync(gfx++);
            gDPSetPrimColor(gfx++, 0, 0, 0xFF, 0, 0, 0x64);
            gSPMatrix(gfx++, osVirtualToPhysical(&D_8036E4D8[arg1][i]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            gSPSegment(gfx++, 6, osVirtualToPhysical(D_802FFF38));
            gSPDisplayList(gfx++, osVirtualToPhysical(D_80300A68));
            gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        }
        if (D_8036E4D4 + 10 < D_803156C4 && D_8036E4D3 != 0 && D_8036E4D3 < 2) {
            D_8036E4D4 = D_803156C4;
            D_8036E5D8[D_8036E4D3] = 0.001f;
            D_8036E4D3++;
        }
        gDPPipeSync(gfx++);
        gDPSetColorDither(gfx++, G_CD_MAGICSQ);
        gDPPipeSync(gfx++);
        if (D_8036E4D3 != 0) {
            func_802AC1A0(D_8036E5D8[0] * 283.0f);
        }
    }
    *arg0 = gfx;
}
