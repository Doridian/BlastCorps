#include "common.h"
#include "game/objects.h"
#include "game/sched.h"
#include "game/vehicle.h"
#include "game/audio.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
} UnkStruct_802E8F68; /* size = 0xA */

extern s32 D_80358064;
extern s32 D_80364A58;
extern u8 D_8036DCD4;
extern u8 D_8036EB92;

u8 func_8027EED8(s32, s32, s16 *);
s32 func_8026394C();
void func_80264A34(char *, u16, s32);
void func_8026AF6C(s32);
u16 func_8028604C(u32);
void func_8029A7E4(char *, ...);
void alCSPSetTempo(ALCSPlayer *, s32);
u8 func_802C1B1C(void);
void func_802C1DD0(s32);

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} UnkStruct_802E8F74; /* size = 0x8 */

/* .bss, 0x80367B50-0x80367D60 (tools/bss_c.py) */
s32 D_80367B50;
u8 D_80367B54;
u8 D_80367B56[2];
u16 D_80367B58[4];
char D_80367B60[0x50];
#ifdef VERSION_JP
u8 D_80367C70_jp[0x28];
#endif
char D_80367BB0[0xc];
s32 D_80367BBC;
s32 D_80367BC0;
u32 D_80367BC4;
u16 D_80367BC8;
YoshiIcon *D_80367BCC;
YoshiIcon *D_80367BD0;
u8 D_80367BD4;
u8 D_80367BD5;
s16 D_80367BD6;
s16 D_80367BD8;
u8 *D_80367BDC[1];
u8 *D_80367BE0[5];
u16 D_80367BF4;
u16 D_80367BF6;
u8 D_80367BF8;
u8 D_80367BF9;
u8 D_80367BFA;
u8 D_80367BFB;
u16 D_80367BFC;
u8 D_80367BFE;
s8 D_80367BFF;
u8 D_80367C00;
u8 D_80367C01;
LevelInfo *D_80367C04;
char *D_80367C08;
u16 *D_80367C0C;
u8 D_80367C10;
char D_80367C18[0x28];
char D_80367C40[0x28];
u16 D_80367C68[0x28];
u16 D_80367CB8[0x28];
u16 D_80367D08;
char D_80367D10[0x18];
char D_80367D28[0x28];
s16 D_80367D50;
u8 D_80367D52;
u8 D_80367D53;

extern u16 D_8030480C[];
/* .data, 0x802E8F30-0x802E9FB0 (tools/data_c.py) */
u8 D_802E8F30[8] = { 40, 43, 44, 45, 46 };
UnkStruct_802E8F38 D_802E8F38[6] = {
    { 10, 3972, 44, 2478 },
    { 17, 4063, 100, 1402 },
    { 33, 1085, 163, 2423 },
    { 14, 2974, 183, 5642 },
    { 13, 2540, 160, 5973 },
    { 4, 3213, 415, 4464 },
};
UnkStruct_802E8F68 D_802E8F68[1] = { { 13, 6355, 1359, 4533, 8141 } };
UnkStruct_802E8F74 D_802E8F74[4] = {
    { 39, 2061, 180, 4118 },
    { 24, 1687, 200, 639 },
    { 51, 1793, 245, 2261 },
    { 41, 2250, 200, 1700 },
};
LevelInfo D_802E8F94[LEVEL_COUNT] = {
    {
        1, 1, 0, 0x190, 0x3E8, 0x7D0, 0x15F, 0x5DB, 0x1E9, 0x60E, { 2, 3, 1 }, { 0 }, 6, 140, 500,
        { 0 }, -32768, 1380, 218, 5018, 0x190, { MEDAL_TIME0(140, 150), 250, 0x12C, 0x190 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 937, 406, 2812, 32,
        { MEDAL_TIME0(0x3B6, 0x3E8), 0x6A4, 0x708, 0x76C }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 3111, 700, 7335, 32,
        { MEDAL_TIME0(0x258, 0x2EE), 0x41A, 0x44C, 0x47E }, 1, { 0 }, 625, 0x15E, 200,
    },
    {
        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0, 93, 1, 119 }, -32768, 1462, 612,
        4128, 0x10010, { MEDAL_TIME0(230, 250), 0x190, 0x1F4, 0x258 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 320, 800, { 0 }, -32768, 1956, 600, 3416,
        32, { MEDAL_TIME0(0x190, 0x1C2), 0x2BC, 0x320, 0x384 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 2289, 480, 5076, 8,
        { MEDAL_TIME0(0x1B8, 0x1EA), 0x352, 0x384, 0x44C }, 0, { 0 }, 0, 0, 0,
    },
    {
        8, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 0xF4240, 0, 0, { 0, 60, 0, 200 }, -32768, 1000,
        500, 1000, 0x430, { MEDAL_TIME0(0x172, 0x190), 0x1F4, 0x258, 0x5DC }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 1000, 500, 1000, 8,
        { 40, 100, 200, 0x258 }, 0, { 0 }, 0, 0, 0,
    },
    {
        16, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 85, 0, 0, { 0 }, -32768, 1000, 500, 1000,
        0xE538, { MEDAL_TIME0(0x122, 0x136), 0x15E, 0x190, 0x320 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 3591, 600, 5575,
        0x4098, { MEDAL_TIME0(0x23A, 0x258), 0x4E2, 0x578, 0x5DC }, 1, { 0 }, 425, 0x15E, 200,
    },
    {
        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 600, { 0 }, -32768, 1337, 506, 4312,
        0x26A0, { MEDAL_TIME0(0x118, 0x136), 0x1F4, 0x258, 0x320 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0x32C8, 0x2968, 0x1A6C, 0x10EE, 0x1AE4, 0x1102, { 1, 3, 2 }, { 0 }, 4, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0xE538, { MEDAL_TIME0(0x3B6, 0x3C0), 0x4B0, 0x578, 0x708 }, 1, { 0 }, 275,
        0x15E, 100,
    },
    {
        1, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 2, 48, 2, 248 }, -32768, 3350, 1000,
        8490, 32, { MEDAL_TIME0(0x60E, 0x672), 0x898, 0x8FC, 0x960 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 1000, { 0 }, -32768, 3000, 1500, 11650,
        0x290, { 0x2D0, 0x41A, 0x47E, 0x4B0 }, 1, { 0 }, 625, 0x15E, 200,
    },
    {
        1, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 5000, 450, 6500, 0x450,
        { MEDAL_TIME0(0x6D6, 0x726), 0xE10, 0x1068, 0x12C0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 2280, 500, 6547, 0x406,
        { MEDAL_TIME0(0x168, 0x190), 0x2BC, 0x320, 0x44C }, 1, { 0 }, 625, 0x15E, 200,
    },
    {
        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 180, 800, { 0 }, -32768, 6311, 800, 7559,
        0x8608, { 0x118, 0x2BC, 0x384, 0x44C }, 1, { 0 }, 625, 0x15E, 200,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 1800, 800, 5800, 214,
        { 0x21C, 0x384, 0x4B0, 0x640 }, 1, { 0 }, 250, 0x15E, 200,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 751, 250, 4500, 0x20D0,
        { MEDAL_TIME0(0x2EE, 0x302), 0x3B6, 0x4B0, 0x640 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 7, 180, 800, { 0 }, -32768, 1000, 500, 1000,
        0x200, { MEDAL_TIME0(0x12C, 0x15E), 0x320, 0x4B0, 0xBB8 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0x1F40, 0x1F40, 0x10D2, 0xE10, 0x1136, 0xE24, { 1, 3, 2 }, { 0 }, 4, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0xE538, { MEDAL_TIME0(0x2DA, 0x2EE), 0x44C, 0x514, 0x834 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0xFA0, 0xFA0, 0x5C8, 0x640, 0x654, 0x65E, { 0, 2, 3, 1 }, { 0 }, 4, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0xE538, { 0x168, 0x190, 0x1F4, 0x258 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0xFA0, 0xFA0, 0x659, 0x6E4, 0x6D1, 0x70C, { 0, 1, 3, 2 }, { 0 }, 4, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0xE538, { MEDAL_TIME0(250, 260), 0x15E, 0x1F4, 0x2D0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        64, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 0x190, 0, 0, { 0 }, -32768, 1000, 500, 1000,
        32, { MEDAL_TIME0(0x2EE, 0x2F8), 0x4B0, 0x6A4, 0x708 }, 0, { 0 }, 0, 0, 0,
    },
    {
        64, 4, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 224, 0, 0, { 0 }, -32768, 1000, 500, 1000, 32,
        { MEDAL_TIME0(0x1C2, 0x1D6), 0x2BC, 0x44C, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0xFA0, 0xFA0, 0x555, 0x715, 0x5F4, 0x728, { 0, 2, 3, 1 }, { 0 }, 4, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0xE538, { MEDAL_TIME0(0x190, 0x19A), 0x1F4, 0x258, 0x2EE }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 0, { 0 }, -32768, 937, 406, 2812,
        0x10000, { 0x226, 0x41A, 0x47E, 0x578 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0x1770, 0x1770, 0xC1C, 0xB2C, 0xFDC, 0xB40, { 1, 3, 2 }, { 0 }, 4, 0, 0, { 0 },
        -32768, 1000, 500, 1000, 0xE138, { 0x1CC, 0x226, 0x2BC, 0x3E8 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 1, 0, 0, 0x1F40, 0x2710, 0x1194, 0x15CF, 0x1590, 0x15E3, { 0, 1, 2, 3 }, { 0 }, 6, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 2, { MEDAL_TIME0(120, 140), 0x190, 0x1F4, 0x258 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 820, { 0, 200, 2, 108 }, -32768, 5571,
        400, 9539, 0x8380, { MEDAL_TIME0(0x1C2, 0x1E0), 0x6D6, 0x708, 0x7D0 }, 1, { 0 }, 625, 0x15E, 200,
    },
    {
        32, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 6, 0, 320, { 0 }, -32768, 1000, 500, 1000,
        0x400, { MEDAL_TIME0(120, 140), 200, 0x12C, 0x258 }, 0, { 0 }, 0, 0, 0,
    },
    {
        32, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 6, 0, 320, { 0 }, -32768, 1000, 500, 1000,
        0x200, { MEDAL_TIME0(210, 220), 0x12C, 0x190, 0x2BC }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 17, 0, 320, { 0 }, -32768, 1000, 500, 1000, 32,
        { 0x15E, 0x2BC, 0x352, 0x44C }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 320, { 0 }, -32768, 1900, 350, 3640,
        0x400, { MEDAL_TIME0(0x10E, 0x12C), 0x208, 0x226, 0x258 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 6, 0, 320, { 0 }, -32768, 1000, 500, 1000, 16,
        { MEDAL_TIME0(0x12C, 0x15E), 0x258, 0x384, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0x3EE4, 0x1EDC, 0x1B58, 0xCE4, 0x1B6C, 0xDFC, { 0, 2, 3, 1 }, { 0 }, 4, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0xE138, { MEDAL_TIME0(0x3A2, 0x3AC), 0x41A, 0x514, 0x834 }, 0, { 0 }, 0, 0, 0,
    },
    {
        32, 4, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 8, 0, 1200, { 0 }, -32768, 1000, 500, 1000,
        0x200, { MEDAL_TIME0(0x1F4, 0x226), 0x4B0, 0x708, 0x960 }, 0, { 0 }, 0, 0, 0,
    },
    {
        32, 4, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 10, 550, 1500, { 0 }, -32768, 1000, 500, 1000,
        0x200, { MEDAL_TIME0(200, 230), 0x190, 0x1F4, 0x384 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 7, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 6, 0, 500, { 0 }, -32768, 1000, 500, 1000, 0,
        { 210, 0x190, 0x1F4, 0x258 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 17, 0, 320, { 0 }, -32768, 1000, 500, 1000,
        0x400, { MEDAL_TIME0(0x12C, 0x154), 0x1F4, 0x320, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 7, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 30, 40, 350, { 0, 50, 1, 94 }, -32768, 1000,
        500, 1000, 32, { MEDAL_TIME0(0x578, 0x5DC), 0x960, 0xC80, 0xE10 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0xFA0, 0xFA0, 0x690, 0x769, 0x708, 0x79B, { 0, 2, 3, 1 }, { 0 }, 4, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0xE538, { 0x168, 0x190, 0x1F4, 0x320 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0xFA0, 0xFA0, 0x519, 0x71B, 0x5E1, 0x743, { 0, 2, 3, 1 }, { 0 }, 4, 0, 320,
        { 0 }, -32768, 1000, 500, 1000, 0xE138, { 0x262, 0x2BC, 0x5DC, 0x640 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 9, 0, 0, 0x2710, 0x2710, 0xE45, 0x15EA, 0x1165, 0x163A, { 2, 3, 1 }, { 0 }, 4, 0, 320,
        { 0 }, -32768, 1000, 500, 1000, 56, { 0x2BC, 0x320, 0x5DC, 0x640 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 9, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 28, 0, 320, { 0 }, -32768, 1000, 500, 1000, 32,
        { MEDAL_TIME0(0x546, 0x578), 0x7D0, 0x960, 0xAF0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        32, 9, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 9, 0, 320, { 0 }, -32768, 1000, 500, 1000, 32,
        { 0x44C, 0xBB8, 0xE10, 0x12C0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 9, 0, 0, 0x2710, 0x1F40, 0x143C, 0x80C, 0x148C, 0xA8C, { 1, 3, 2 }, { 0 }, 4, 0, 320,
        { 0 }, -32768, 1000, 500, 1000, 32, { 0x370, 0x3E8, 0x44C, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 14, 0, 0, 0x2710, 0x1F40, 0x143C, 0x80C, 0x148C, 0xA8C, { 1, 3, 2 }, { 0 }, 4, 0, 320,
        { 0 }, -32768, 1000, 500, 1000, 0x6138, { 210, 0x384, 0x41A, 0x960 }, 0, { 0 }, 0, 0, 0,
    },
    {
        32, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 25, 0, 320, { 0 }, -32768, 1000, 500, 1000, 4,
        { MEDAL_TIME0(0x12C, 0x140), 0x258, 0x3E8, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 14, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 320, { 0 }, -32768, 1000, 500, 1000,
        0x6138, { 210, 0x2EE, 0x3E8, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 6, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 31, 0, 320, { 0 }, -32768, 1000, 500, 1000, 32,
        { MEDAL_TIME0(0x21C, 0x230), 0x2BC, 0x320, 0x384 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 2, 0, 0, 0x1770, 0x1770, 0xC1C, 0xB2C, 0xFDC, 0xB40, { 1, 3, 2 }, { 0 }, 11, 0, 0,
        { 0 }, -32768, 1000, 500, 1000, 0x400, { MEDAL_TIME0(0x1B8, 0x1D6), 0x384, 0x4B0, 0x5DC }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 4, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 14, 0, 3000, { 0 }, -32768, 1000, 500, 1000,
        0x200, { MEDAL_TIME0(0x258, 0x2BC), 0x4B0, 0x708, 0xBB8 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 11, 0, 320, { 0, 100, 1, 94 }, -32768, 1000,
        500, 1000, 4, { 150, 0x12C, 0x4B0, 0x708 }, 1, { 0 }, 625, 0x15E, 200,
    },
    {
        32, 4, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 15, 300, 620, { 0 }, 260, 1000, 500, 1000,
        0x200, { MEDAL_TIME0(0x1C2, 0x1F4), 0x3E8, 0x708, 0xE10 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 3, 0, 320, { 0 }, -32768, 1000, 500, 1000, 32,
        { MEDAL_TIME0(110, 130), 220, 0x15E, 0xE10 }, 0, { 0 }, 0, 0, 0,
    },
    {
        2, 2, 0, 0, 0x1518, 0x1518, 0xD67, 0xA7C, 0xFC9, 0xAB8, { 1, 3, 2 }, { 0 }, 4, 0, 320,
        { 0 }, -32768, 1000, 500, 1000, 0xE138, { 0x29E, 0x320, 0x384, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 3, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 320, { 0 }, -32768, 1571, 218, 4865,
        0x10400, { 0x190, 0x384, 0x41A, 0x4B0 }, 0, { 0 }, 0, 0, 0,
    },
    {
        1, 2, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 4, 0, 320, { 0 }, -32768, 4500, 700, 7360, 4,
        { MEDAL_TIME0(220, 240), 0x168, 0x1C2, 0x1F4 }, 0, { 0 }, 0, 0, 0,
    },
    {
        4, 4, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, { 0 }, 15, 0, 2000, { 0 }, -32768, 1000, 500, 1000,
        0x200, { MEDAL_TIME0(0x384, 0x3B6), 0x5DC, 0x708, 0x960 }, 0, { 0 }, 0, 0, 0,
    },
};
u8 D_802E9F84[0xc] = { 0 };
char D_802E9F90[0xc] = "BUILDINGS";
u16 *D_802E9F9C = D_8030480C;
u8 D_802E9FA0[0x10] = { 32, 83 };

void func_80262150(u8 arg0) {
    s32 i;

    D_80364438 = 45.0f;
    i = 0;
    D_80367C10 = 0;
    do {
        if (D_802E8F30[i] == arg0) {
            D_80364438 = 80.0f;
            D_80367C10 = 1;
        } else {
            i++;
        }
    } while (i < 5 && D_80367C10 == 0);
}

void func_802621DC(u8 arg0) {
    LevelInfo *p;

    p = &D_802E8F94[arg0];
    D_803EF2EC = p->unk26 << 5;
    D_803EF2F0 = p->unk28 << 5;
    D_803EF2F4 = p->unk2A << 5;
}

void func_80262238(u8 arg0) {
    s32 i;
    u8 found;

    i = 0;
    found = 0;
    D_803EFEC8 = 0;
    do {
        if (D_802E8F68[i].unk0 == arg0) {
            found = 1;
        } else {
            i++;
        }
    } while (found == 0 && i <= 0);
    if (found != 0) {
        D_803EFEC8 = 1;
        D_803EFEB0 = D_802E8F68[i].unk2 << 5;
        D_803EFEB4 = D_802E8F68[i].unk4 << 5;
        D_803EFEB8 = D_802E8F68[i].unk6 << 5;
        D_803EFEBC = D_802E8F68[i].unk8 << 5;
    }
}

extern u8 D_006A32B0[];
extern u8 D_006A8DA0[];
extern u8 D_80364424;
extern u16 D_8036442C;
extern s32 D_80364430;
extern char D_80367BB0[];
extern s32 D_80367BC0;
extern u32 D_80367BC4;
extern u16 D_80367BC8;
extern YoshiIcon *D_80367BCC;
extern YoshiIcon *D_80367BD0;
extern u8 D_80367BD4;
extern s16 D_80367BD8;
extern u8 *D_80367BE0[];
extern u8 D_80367C01;
extern char *D_80367C08;
extern u16 *D_80367C0C;

u8 func_8026FA38(char **, u16 **);
u8 func_80272C5C(u8 *, s32, s32, s32, s32, f32);
void func_8028B4C4(u8 *, u8 *, s32 *, s32, s32, s32);

void func_80262320(u8 arg0) {
    s32 i;
    u8 sp33;
    s8 sp32;
    u8 found;
    s32 size;

    i = 0;
    sp33 = 0;
    sp32 = 0;
    found = 0;
    do {
        if (D_802E8F74[i].unk0 == arg0) {
            found = 1;
        } else {
            i++;
        }
    } while (found == 0 && i < 4);
    if (found != 0) {
        D_80364410 = 1;
        D_80364404 = D_802E8F74[i].unk2 << 16;
        D_80364408 = D_802E8F74[i].unk4 << 16;
        D_8036440C = D_802E8F74[i].unk6 << 16;
    } else {
        D_80364410 = 0;
    }
    D_80367C04 = &D_802E8F94[arg0];
    D_803F7C10 = D_802E8F94[arg0].unk1C << 5;
    D_803F7C14 = D_802E8F94[arg0].unk1E << 5;
    D_80364424 = D_802E8F94[arg0].unk38;
    D_80364428 = D_802E8F94[arg0].unk3C << 5;
    D_8036442C = D_802E8F94[arg0].unk40;
    D_80364430 = D_802E8F94[arg0].unk42 << 5;
    D_80367BC0 = D_803156C4;
    D_80367BC4 = -1;
    D_80367BD8 = 0;
    switch ((u32)D_80364AA8) {
        case 0x4:
        case 0x80:
            sp33 = 6;
            break;
        case 0x20:
            sp33 = func_8026FA38(&D_80367C08, &D_80367C0C);
            D_80367BD8 = 2;
            break;
        case 0x10:
        case 0x40:
            sp33 = 9;
            D_80367BD8 = -2;
            break;
        case 0x1:
        case 0x2:
        case 0x8:
            break;
    }
    if (sp33 == 6) {
        D_80367BD8 = 6;
        D_80367C08 = D_802E9F90;
        sp32 = 0;
        D_80367C0C = D_802E9F9C;
    }
    if (D_80364AA8 != 1) {
        size = D_006A8DA0 - D_006A32B0;
        if (D_80364AA8 != 0x80 && D_80364A98 == 0x2000) {
            func_8028B4C4(D_006A32B0, D_80358070, &size, 0xA, 0, 1);
            for (i = 0; i < 5; i++) {
                D_80367BE0[i] = D_80358070 + (i << 15);
            }
            D_80358070 += size;
        }
        if (D_80364A98 == 0x40) {
            D_80367BC8 = 0;
        } else {
            D_80367BC8 = 1;
        }
    } else {
        D_80367BC8 = 0;
    }
    D_80367C01 = 0;
    D_80367C00 = 0;
    if (D_80364A98 == 0x40) {
        D_80367BFF = 0;
    } else {
        D_80367BFF = 0;
        D_80367BFE = 0;
    }
    D_80367BCC = &D_802F49F4[sp33];
    if (D_802E8F94[arg0].unk0 == 0x20) {
        D_80367BD0 = 0;
        D_80367BD4 = func_80272C5C(D_80367BCC->unk6, 0, D_80367BCC->unk4, D_80367BCC->unk2C, D_80367BCC->unk2D,
                                   D_80367BCC->unk28 * 0.5);
    } else {
        D_80367BD0 = 0;
        if (sp33 != 0) {
            D_80367BD4 = func_80272C5C(D_80367BCC->unk6, 0, D_80367BCC->unk4, D_80367BCC->unk2C, D_80367BCC->unk2D,
                                       1.0f);
        } else {
            D_80367BCC = NULL;
        }
    }
    if (D_80364A98 & 0x440) {
        func_80264A34(D_80367BB0, D_80367C04->medalTimes[3] - D_80367BF6, 0);
    } else {
        D_80367B54 = 0;
    }
}

extern u8 D_803156F4;
extern char D_80367C18[];
extern char D_80367C40[];
extern u16 D_80367C68[];
extern u16 D_80367CB8[];

void func_80260650(SndBank *, s32, s32);
s32 func_8026205C(s32);

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/1D990/func_80262840.s")
#else
void func_80262840(void) {
    u32 sp34;
    u16 sp32;
    u16 sp30;

    sp34 = (u32)(D_803156C4 - D_80367BC0) / 60;
    if (sp34 == D_80367BC4) {
    } else {
        switch (sp34) {
            case 0:
                D_80367C68[0] = 0xFFF;
                D_80367CB8[0] = 0xFFF;
                switch ((u32)D_80364AA8) {
                    case 0x2:
                        sprintf(D_80367C18, "FINISH %d LAPS IN", D_80367C04->goal);
                        break;
                    case 0x4:
                    case 0x20:
                        if (D_802E8BDC == 0x34) {
                            sprintf(D_80367C18, "DESTROY TARGETS IN");
                        } else {
                            sprintf(D_80367C18, "DESTROY %s IN", D_80367C08);
                        }
                        break;
                    case 0x80:
                        if (D_802E8BDC == 0x32) {
                            sprintf(D_80367C18, "CLEAR SHUTTLE PATH");
                        } else {
                            sprintf(D_80367C18, "CLEAR CARRIER PATH");
                        }
                        break;
                    case 0x8:
                        sprintf(D_80367C18, "CAUSE $%d DAMAGE", D_80367C04->goal);
                        break;
                    case 0x10:
                    case 0x40:
                        sprintf(D_80367C18, "FIND %d RDUS IN", D_80367C04->goal);
                        break;
                }
                sp32 = D_80367C04->medalTimes[3] / 600;
                sp30 = (D_80367C04->medalTimes[3] / 10) % 60;
                sprintf(D_80367C40, "%d MINUTE%c %d SECONDS", sp32, D_802E9FA0[sp32 != 1], sp30);
                D_802F5804[YOSHI_ENTRY(36)].text = D_80367C18;
                D_802F5804[YOSHI_ENTRY(37)].text = D_80367C40;
                D_802F5804[YOSHI_ENTRY(36)].unk10 = D_80367C68;
                D_802F5804[YOSHI_ENTRY(37)].unk10 = D_80367CB8;
                func_8026AF6C(0x8009);
                func_80260650(D_80367738, func_8026205C(0), 0);
                break;
            case 1:
                func_80260650(D_80367738, 0x96, 0);
                break;
            case 2:
                func_80260650(D_80367738, 0x96, 0);
                break;
            case 3:
                func_80260650(D_80367738, 0x96, 0);
                break;
            case 4:
                func_80260650(D_80367738, 0x99, 0);
                break;
        }
    }
    D_80367BC4 = sp34;
    if (sp34 == 4 && D_8035805C == D_803156F4) {
        D_80364A98 = 4;
    }
}
#endif

extern u8 D_803643D6;
extern u8 D_803643D7;
extern char D_80367BB0[];
extern u16 D_80367BF4;
extern u16 D_80367D08;
extern s16 D_8036BB1A;

void func_802609F0(void);
void func_80260A10(void);
void func_80262FD0(void);
void func_8026303C(void);
void func_80263140(void);
void func_80263358(void);
void func_802633E0(void);
void func_80275270(u64, f32);
s32 func_802753C0(void);

void func_80262BF4(void) {
    if (D_803643D7 == 0 && D_803643D6 == 0) {
        D_80367BF6 = D_80367C04->medalTimes[3] - ((D_80367C04->medalTimes[3] < func_8028604C(D_803156C0 - D_80364A58))
                                              ? D_80367C04->medalTimes[3]
                                              : func_8028604C(D_803156C0 - D_80364A58));
    }
    func_80264A34(D_80367BB0, D_80367BF6, 1);
    switch (D_80364A90) {
        case 0x2000:
            func_80262840();
            break;
        case 0x4000000:
            if (D_803643D6 != 0 && func_802753C0() == 0 && D_8036BB1C == 1) {
                if ((D_80364AF0[D_80364AE8].medal[D_802E8BDC] > 0 && D_80364AF0[D_80364AE8].medal[D_802E8BDC] < 6)
                        ? 1
                        : 0) {
                    func_80275270(0x8000000, 0.25f);
                } else {
                    func_80275270(0x40, 0.25f);
                }
            }
            break;
        default:
            if (D_80367BF6 == 0) {
                D_803643D9 = 1;
            } else {
                D_80367BF4 = D_80367BF6 / 10;
                if (D_80367BF4 < 10 && D_80367BF4 != D_80367D08) {
                    func_80260650(D_80367738, 0xA3 - D_80367BF4, 0);
                }
                D_80367D08 = D_80367BF4;
            }
            if (D_803BE738 != 0 && D_80364AA8 == 0x40) {
                func_80260650(D_80367738, 0x3C, 0);
            }
            if (D_803BE738 != 0) {
                D_803643D9 = 1;
            }
            if (D_803643D6 != 0) {
                func_80260A10();
                func_802609F0();
                func_8026AF6C(0xA00E);
                D_8036BB1A = -1;
                D_80364A98 = 0x4000000;
            } else if (D_803643D7 != 0) {
                D_80364A98 = 0x4000000;
            } else {
                switch ((u32)D_80364AA8) {
                    case 0x2:
                        func_802633E0();
                        break;
                    case 0x4:
                        func_8026303C();
                        break;
                    case 0x20:
                    case 0x80:
                        func_80263140();
                        break;
                    case 0x8:
                        func_80263358();
                        break;
                    case 0x10:
                    case 0x40:
                        func_80262FD0();
                        break;
                }
            }
            break;
    }
}

void func_80262FD0(void) {
    if (D_8036EA70.rt >= D_80367C04->goal) {
        D_803643DA = 1;
    }
    sprintf(D_80367B60, "%d/%d", D_8036EA70.rt, D_80367C04->goal);
}

void func_8026303C(void) {
    s16 sp1E;

    func_802C1DD0(0);
    if (D_8036EA70.bd >= D_80367C04->goal) {
        D_803643DA = 1;
    }
    if (D_8036DCD4 != 0) {
        func_8027EED8(D_803643E0 >> 5, D_803643E8 >> 5, &sp1E);
        if (sp1E > (D_803643E4 >> 5)) {
            D_803643D9 = 1;
        }
    } else if (D_80367C04->unk24 > (D_803643E4 >> 5)) {
        D_803643D9 = 1;
    }
    sprintf(D_80367B60, "%d/%d", D_8036EA70.bd, D_8036EB92);
}

void func_80263140(void) {
    s16 sp2E;

    func_802C1DD0(1);
    switch (D_80364AA8) {
        case 0x20:
            if (D_8036EA70.bd >= D_8036EB92) {
                D_803643DA = 1;
            }
            break;
        case 0x80:
            if (D_803F7806 != 0 || (D_802E8BDC == 0x32 && D_8036EA70.bd >= D_8036EB92)) {
                D_803643DA = 1;
            }
            break;
    }
    if (D_8036DCD4 != 0) {
        func_8027EED8(D_803643E0 >> 5, D_803643E8 >> 5, &sp2E);
        if (sp2E > (D_803643E4 >> 5)) {
            D_803643D9 = 1;
        }
    } else if (D_80367C04->unk24 > (D_803643E4 >> 5)) {
        D_803643D9 = 1;
    }
    switch (D_80364AA8) {
        case 0x20:
            sprintf(D_80367B60, "%d/%d", D_8036EA70.bd, D_8036EB92);
            break;
        case 0x80:
            if (D_802E8BDC == 0x32) {
                sprintf(D_80367B60, "%d/%d", D_8036EA70.bd, D_8036EB92);
            } else {
                sprintf(D_80367B60, "%d/%d", func_802C1B1C(), D_8036EB92);
            }
            break;
    }
}

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/1D990/func_80263358.s")
#else
void func_80263358(void) {
    s32 sp1C;

    func_802C1DD0(0);
    if (D_8036EA70.ip >= D_80367C04->goal) {
        D_803643DA = 1;
    }
    sp1C = D_80367C04->goal - D_8036EA70.ip;
    if (sp1C < 0) {
        sp1C = 0;
    }
    sprintf(D_80367B60, "$%d LEFT", sp1C);
}
#endif

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/1D990/func_802633E0.s")
#else
void func_802633E0(void) {
    s32 i;
    u8 sp33;

    if (D_80367B54 == 0) {
        if (func_8026394C(D_803643E0 >> 5, D_803643E8 >> 5, D_80367C04->unkA, D_80367C04->unkC, D_80367C04->unkE,
                          D_80367C04->unk10) != 0) {
            D_80367B54 = 1;
            D_80367BF8 = 0;
            D_80367BF9 = D_80367C04->unk12[0];
            D_80367BBC = D_803156C0;
            D_80367BFB = 1;
            D_80367BFC = 0xFFFF;
        }
    } else {
        D_80367BFA = ((D_803643E8 >> 5) - D_80367C04->unk4) / ((D_80367C04->unk8 - D_80367C04->unk4) >> 1);
        D_80367BFA *= 2;
        D_80367BFA += ((D_803643E0 >> 5) - D_80367C04->unk2) / ((D_80367C04->unk6 - D_80367C04->unk2) >> 1);
        D_80367B58[D_80367B54 - 1] = func_8028604C(D_803156C0 - D_80364A58);
        for (i = D_80367B54 - 2; i >= 0; i--) {
            D_80367B58[D_80367B54 - 1] -= D_80367B58[i];
        }
        if (D_80370C28 & 0x2000) {
            func_8029A7E4("box number=%d\n", D_80367BFA);
        }
        if (D_80367BF8 == 4) {
            if (func_8026394C(D_803643E0 >> 5, D_803643E8 >> 5, D_80367C04->unkA, D_80367C04->unkC, D_80367C04->unkE,
                              D_80367C04->unk10) != 0) {
                if (D_80367B58[D_80367B54 - 1] < D_80367BFC) {
                    func_8029A7E4("new best lap %d %d\n", D_80367BFC, D_80367B58[D_80367B54 - 1]);
                    D_80367BFB = D_80367B54;
                    D_80367BFC = D_80367B58[D_80367B54 - 1];
                }
                if (D_80367C04->goal - 1 < D_80367B54) {
                    D_803643DA = 1;
                } else {
                    sp33 = D_80367C04->goal - D_80367B54;
                    if (D_802E8BD0 == 0) {
                        func_8026AF6C(0x8008);
                    }
                    if (sp33 == 1) {
                        sprintf(D_80367D10, "1 LAP LEFT!");
                    } else {
                        sprintf(D_80367D10, "%d LAPS LEFT!", sp33);
                    }
                    D_802F5804[YOSHI_ENTRY(35)].text = D_80367D10;
                    D_802F5804[YOSHI_ENTRY(35)].unk10 = (u16 *)D_80367D28;
                    alCSPSetTempo(D_80367734, alCSPGetTempo(D_80367734) * 0.95);
                }
                D_80367B54++;
                D_80367BF8 = 0;
            }
        } else if (D_80367C04->unk12[D_80367BF8] == D_80367BF9 &&
                   D_80367C04->unk12[(D_80367BF8 + 1) % 4] == D_80367BFA) {
            func_8029A7E4("box cross: %d to %d\n", D_80367BF9, D_80367BFA);
            D_80367BF8++;
        }
        D_80367BF9 = D_80367BFA;
    }
    for (i = 0; i < D_80367B54 && i < D_80367C04->goal; i++) {
        func_80264A34(D_80367B60 + i * 0x14, D_80367B58[i], D_80367B54 == i + 1);
    }
}
#endif

s32 func_8026394C(x, y, x0, y0, x1, y1)
    s16 x;
    s16 y;
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
{
    if (x >= x0 && y >= y0 && x < x1 && y < y1) {
        return 1;
    }
    return 0;
}

extern u8 D_80367BD5;
extern s16 D_80367BD6;
extern u8 D_80367BFB;

void func_80259CCC(void *, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
void func_8025E2CC(Gfx **, void *, s32);
Gfx *func_80264264(void *, Gfx *);
Gfx *func_80272ED8(Gfx *, u8, s16, s16, u8, u8, f32);
Gfx *func_80274868(Gfx *);
Gfx *func_80274AA4(Gfx *);

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/1D990/func_802639B4.s")
#else
Gfx *func_802639B4(Gfx *arg0, void *arg1, s32 arg2) {
    Gfx *gfx;
    s32 unused;
    u8 sp5F;
    u8 sp5E;
    u8 sp5D;
    s16 alpha;
    s32 unused2;
    s32 i;

    gfx = arg0;
    if ((D_80364A90 & 0x04000200) && D_8036BB18 != 0x1E) {
        alpha = 0xFF;
    } else {
        alpha = D_80367BD6;
    }
    switch ((u32)D_80364AA8) {
        case 0x2:
            for (i = 0; i < D_80367B54 - ((D_80364A90 & 0x440) ? 1 : 0) && i < D_80367C04->goal; i++) {
                if (i + 1 == D_80367BFB && i + 1 != D_80367B54) {
                    func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xFF,
                                  0, 0, D_80367BD6);
                } else if (i + 1 == D_80367B54) {
                    func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xFF,
                                  0xFF, 0xFF, D_80367BD6);
                } else {
                    func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xA0,
                                  0xA0, 0xA0, D_80367BD6);
                }
            }
            break;
        case 0x4:
        case 0x20:
        case 0x80:
            func_80259CCC(arg1, D_80367B60, NULL, 1, 0, 0x38, 0x14, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
            break;
        case 0x8:
            func_80259CCC(arg1, D_80367B60, NULL, 1, 0, 0x1C, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
            break;
        case 0x10:
        case 0x40:
            func_80259CCC(arg1, D_80367B60, NULL, 1, 0, 0x38, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
            break;
    }
    sp5E = 0xFF;
    sp5F = 0;
    if (D_80364A90 & 0x04000104) {
        sp5D = D_80367BF4 < 10;
        if (!sp5D) {
            sp5F = 0xFF;
        }
    } else {
        sp5D = 0;
        sp5F = 0xFF;
        sp5E = 0;
    }
    if (D_80367BF4 == 0) {
        sp5D = 0;
    }
    if (sp5D == 0 || (u32)D_803156C4 % 20 < 16) {
        if (D_80364AA8 == 2) {
            func_80259CCC(arg1, D_80367BB0, NULL, 1, 0, 0x18, i * 0x12 + 0x14, 0x10, 0x10, 1, sp5E, sp5F, 0,
                          D_80367BD6);
        } else {
            func_80259CCC(arg1, D_80367BB0, NULL, 1, 0, 0x1C, D_80367BD8 + 0x2A, 0x10, 0x10, 1, sp5E, sp5F, 0, alpha);
        }
    }
    if (D_803643D7 != 0 && D_80364A90 == 0x04000000) {
        func_8025E2CC(&gfx, arg1, D_8035805C);
    }
    if (D_80367BC8 != 0) {
        gfx = func_80264264(arg1, gfx);
    }
    gfx = func_80274868(gfx);
    if (D_80367BCC != NULL) {
        gfx = func_80272ED8(gfx,
                            D_80367BCC->unk1B[((u32)D_803156C4 / D_80367BCC->unk26) % D_80367BCC->unk1A] + D_80367BD4 - 1,
                            0x18, D_80367BD8 + 0xC, alpha, 1, 1.0f);
    }
    if (D_80367BD0 != NULL) {
        gfx = func_80272ED8(gfx,
                            D_80367BD0->unk1B[((u32)D_803156C4 / D_80367BD0->unk26) % D_80367BD0->unk1A] + D_80367BD5 - 1,
                            0x18, D_80367BD8 + 0xC, alpha, 1, 1.0f);
    }
    gfx = func_80274AA4(gfx);
    arg2 += (gfx - arg0) * 4;
    return gfx;
}
#endif

void func_8026420C(void) {
    if (D_80367BFE != 0 && D_80358064 == D_80367B50) {
        func_8029A7E4("Replay turbo ....\n");
        D_80367BFF = 1;
    }
}

extern u8 D_80367BFE;
extern u8 *D_80367BDC[];
extern s16 D_80367D50;
extern u8 D_80367D52;
extern u8 D_80367D53;
extern u8 D_80370C1C;

Gfx *func_80264264(void *arg0, Gfx *arg1) {
    Gfx *gfx;
    s32 i;
    s32 x;

    gfx = arg1;
    switch (D_80367BC8) {
        case 1:
            D_80367D52 = 3;
            D_80367BC8 = 2;
            break;
        case 2:
            D_80367D50 = (u32)((D_803156C4 - D_80367BC0) * 60) / 60 - 64;
            D_80367D52 = 3 - ((D_80367D50 * 2 / 10 < 0) ? 0 : D_80367D50 * 2 / 10);
            if (D_80367D50 >= 10) {
                D_80367D52 = 1;
                D_80367D50 = 10;
                D_80367BC8 = 3;
            }
            break;
        case 3:
            if (D_802E8BD0 == 0) {
                D_80367BC8 = 4;
            }
            if (D_80370C1C != 0) {
                D_80367C01 = 1;
            }
            break;
        case 4:
            D_80367D53 = D_80367D52;
            if ((u32)(D_803156C0 - D_80364A58) >= 7) {
                D_80367D52 = 5;
            } else {
                D_80367D52 = 4;
            }
            if ((u32)(D_803156C0 - D_80364A58) >= 21) {
                D_80367BC8 = 5;
                D_80367BFF = D_80367BFE;
                D_80367B50 = D_80358064;
            }
            if (D_80367D53 == 1 && D_80370C1C == 0) {
                D_80367BFE = !D_80367C01;
            }
            if (D_80367D53 == 4 && D_80367D52 == 5 && D_80370C1C == 0) {
                D_80367BFE = 0;
            }
            break;
        case 5:
            D_80367D50 = 10 - (u32)((D_803156C0 - D_80364A58) * 2 * 60 - 2400) / 60;
            if (D_80367D50 < -63) {
                D_80367BC8 = 0, func_8029A7E4("turbo %d\n", D_80367BFE);
            }
            break;
    }
    if (D_80364AA8 != 0x80) {
        gDPPipeSync(gfx++);
        gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetTexturePersp(gfx++, G_TP_NONE);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
        gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, 0xFF);
        x = 32;
        for (i = 0; i < 0x100; i += 0x20) {
            gDPLoadTextureTile(gfx++, D_80367BDC[D_80367D52], G_IM_FMT_RGBA, G_IM_SIZ_16b, 256, 64, i, 0, i + 31, 63, 0,
                               G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                               G_TX_NOLOD, G_TX_NOLOD);
            gSPScisTextureRectangle(gfx++, (i + x) << 2, 0, (i + x + 32) << 2, (D_80367D50 + 63) << 2,
                                    G_TX_RENDERTILE, i << 5, -(D_80367D50 << 5), 1 << 10, 1 << 10);
        }
        gDPPipeSync(gfx++);
        gDPSetTexturePersp(gfx++, G_TP_PERSP);
    }
    return gfx;
}

void func_80264A34(char *buf, u16 t, s32 arg2) {
    buf[0] = t / 6000 + '0';
    t %= 6000;
    buf[1] = t / 600 + '0';
    buf[2] = ':';
    t %= 600;
    buf[3] = t / 100 + '0';
    t %= 100;
    buf[4] = t / 10 + '0';
    t %= 10;
    buf[5] = '.';
    buf[6] = t + '0';
    buf[7] = 0;
}

void func_80264AEC(void) {
    s32 i;
    u16 sum;

    sum = 0;
    if (D_80364AA8 == 2 && D_80367B54 >= 2) {
        for (i = 0; i < D_80367B54 - 1; i++) {
            sum += D_80367B58[i];
        }
        D_80367BF6 = D_80367C04->medalTimes[3] - sum;
    }
    if (D_80367BF6 >= 60000) {
        D_80367BF6 = 0;
    }
}

u8 func_80264BA4(u8 arg0) {
    u8 ret;

    switch (arg0) {
        case 40:
            ret = 0;
            break;
        case 43:
            ret = 5;
            break;
        case 44:
            ret = 4;
            break;
        case 45:
            ret = 2;
            break;
        case 46:
            ret = 1;
            break;
        default:
            ret = 3;
            break;
    }
    return ret;
}
