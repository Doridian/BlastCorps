/*
 * The object after yoshi.c (26570.c): eu pads yoshi.c's .text to 16 before
 * func_8026FBB0.  Its .data starts at the vertices after yoshi.c's tables,
 * its .rodata after yoshi.c's constants, and its .bss after yoshi.c's
 * variables (the 0x60 bytes nothing refers to taken to be this object's).
 */
#include "common.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/sched.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

/* Element type of the arrays D_8036BB10 points at. */
typedef struct {
    /* 0x00 */ u8 pad0[2];
    /* 0x02 */ u16 unk2;
} UnkStruct_8026F644;

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ char unk1[0xF];
    /* 0x10 */ u16 *unk10;
} UnkStruct_802F9934; /* size = 0x14 */

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x02 */ s16 unk2[16];
} UnkStruct_802F48D0; /* size = 0x22 */

extern f32 D_80364414;
extern u16 D_802E8C8C[];
extern u16 D_802E8C90[];
extern u16 D_802E8C94[];
extern u16 D_802E8C98[];
extern u16 D_802E8C9C[];
extern u32 D_803156C4;
extern u16 D_803C30A8[];
extern u32 D_80364AA8;
extern s32 D_803F7684;
extern s32 D_803BE70C;
extern s32 D_803BE710;
extern s16 D_803BE714;
extern u8 D_802F499A[];

void func_8026AF6C(u16 arg0);
void func_8029A7E4(char *, ...);
s32 func_80270A54(Rdu *arg0);
char *func_8025B558(u16 *);
void func_8026BA7C(YoshiWindow *arg0);
s32 func_8026F92C(u64);
u8 func_8026FA38(char **, u16 **);
void func_8026FB50(YoshiWindow *);
u16 func_8026F8A8(u16, u16, u16, u16);
void func_8026A5CC(u64 *dst, u64 *src, s32 size);
s32 func_8026A6F0(s32, s32, s32, s32, s32, s32);
void func_802AC544(s32, s32, s32);
void func_80260650(SndBank *, u16, s32);
s8 func_80272C5C(u8 *, s32, s32, s32, s32, f32);

/*
 * YoshiWindow and dialogue text, reached only through the pointer tables in this
 * file's .data (still asm), so they are defined here to keep their place at
 * the start of the .rodata.
 */

/* .bss, 0x8036BB50-0x8036BEF0 (tools/bss_c.py) */
u8 D_8036BB50[0x60];
u16 D_8036BBB0[1];
u8 D_8036BBB2[2];
u8 D_8036BBB4[4];
u8 D_8036BBB8[0x31C];
s32 D_8036BED4;
Rdu *D_8036BED8;
f32 D_8036BEDC;
u8 D_8036BEE0;

/* .data, 0x802F99C0-0x802FA270 (tools/data_c.py) */
extern Vtx D_802F99C0[4];
#ifdef ASSET_INIT
#include "hd_code/2B3F0/D_802F99C0.inc"
#else
Vtx D_802F99C0[4] = { 0 };
#endif
extern Vtx D_802F9A00[0x40];
#ifdef ASSET_INIT
#include "hd_code/2B3F0/D_802F9A00.inc"
#else
Vtx D_802F9A00[0x40] = { 0 };
#endif
extern Vtx D_802F9E00[0x40];
#ifdef ASSET_INIT
#include "hd_code/2B3F0/D_802F9E00.inc"
#else
Vtx D_802F9E00[0x40] = { 0 };
#endif
s32 D_802FA200[0x14] = { 60, 100, 200, 50, 120, 120, 0, 0, 50, 200, 120, 0, 0, 50, 50, 50, 100 };
s32 D_802FA250 = 0;
s32 D_802FA254 = 0;
s32 D_802FA258 = 0;
s32 D_802FA25C = 0;
s32 D_802FA260 = 0;
s32 D_802FA264 = 0;
s32 D_802FA268 = 0;
s32 D_802FA26C = 0;

void func_8026FBB0(LevelRdu *arg0, LevelRdu *arg1) {
    s32 pad;

    D_8036EB90 = 0;
    D_8036EA70.rt = 0;
    if (D_80364A90 != 0x40) {
        D_8036BED4 = D_8036BBB0[0] = 0;
    }
    D_8036BED8 = (Rdu *) D_80358070;
    D_8036BEE0 = 0;
    D_8036BEDC = 999999.0f;
    while (arg0 != arg1) {
        D_8036BED8[D_8036EB90].x = arg0->x;
        D_8036BED8[D_8036EB90].y = arg0->y;
        D_8036BED8[D_8036EB90].z = arg0->z;
        D_8036BED8[D_8036EB90].collected = 0;
        D_8036BED8[D_8036EB90].cell = (arg0->z / (D_803BE710 >> 5)) * D_803BE714 + arg0->x / (D_803BE70C >> 5);
        func_8026A5CC((u64 *) D_8036BED8[D_8036EB90].vtx[0], (u64 *) D_802F99C0, 0x40);
        func_8026A5CC((u64 *) D_8036BED8[D_8036EB90].vtx[1], (u64 *) D_802F99C0, 0x40);
        D_8036EB90++;
        arg0++;
    }
    D_80358070 += D_8036EB90 * sizeof(Rdu);
}

u8 func_8026FE6C(s32 arg0) {
    return D_8036BED8[arg0].collected;
}

void func_8026FE8C(s32 arg0) {
    D_8036BED8[arg0].collected = 1;
    D_8036EA70.rt++;
}

s32 func_8026AD30(s32);

void func_8026FEC4(void) {
    s32 i;
    s32 pad;
    s32 sp2C;
    u8 sp2B;
    u8 sp2A;

    sp2A = 0;
    sp2B = (D_803643E8 / D_803BE710) * D_803BE714 + D_803643E0 / D_803BE70C;
    for (i = 0; i < D_8036EB90; i++) {
        if (D_8036BED8[i].cell == sp2B && D_8036BED8[i].collected == 0) {
            sp2C = func_8026A6F0(D_803643E0 >> 5, D_803643E4 >> 5, D_803643E8 >> 5, D_8036BED8[i].x,
                                 D_8036BED8[i].y, D_8036BED8[i].z);
            if (sp2C < D_802FA200[D_80364456]) {
                if (++D_8036EA70.rt >= 4 && D_802E8BD0 == 0) {
                    func_8026AD30(0x46);
                }
                if (D_80364A90 != 0x40) {
                    D_8036BBB0[D_8036BED4] = D_8036BBB0[D_8036BED4 + 1] = i;
                    D_8036BED4++;
                }
                func_802AC544(D_8036BED8[i].x, D_8036BED8[i].y + 5, D_8036BED8[i].z);
                D_8036BED8[i].collected = 1;
                if (sp2A == 0) {
                    sp2A = 1;
                    if (D_80364AA8 == 0x40) {
                        func_80260650(D_80367738, 0x3B, 0);
                    } else {
                        func_80260650(D_80367738, 0x27, 0);
                    }
                }
            }
        }
    }
}

void func_802701A8(Gfx **arg0, s32 arg1) {
    Gfx *gfx;
    s32 i;
    s32 j;
    f32 mf[4][4];
    f32 x[4];
    f32 y[4];
    f32 z[4];

    gfx = *arg0;
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    if (D_80364414 != D_8036BEDC) {
        D_8036BEE0 ^= 1;
        guRotateF(mf, D_80364414 - 135.0, 0.0f, 1.0f, 0.0f);
        for (i = 0; i < 4; i++) {
            guMtxXFMF(mf, D_802F99C0[i].v.ob[0], D_802F99C0[i].v.ob[1], D_802F99C0[i].v.ob[2], &x[i], &y[i], &z[i]);
        }
        for (i = 0; i < D_8036EB90; i++) {
            for (j = 0; j < 4; j++) {
                D_8036BED8[i].vtx[D_8036BEE0][j].v.ob[0] = (s16) x[j] + D_8036BED8[i].x;
                D_8036BED8[i].vtx[D_8036BEE0][j].v.ob[1] = (s16) y[j] + D_8036BED8[i].y;
                D_8036BED8[i].vtx[D_8036BEE0][j].v.ob[2] = (s16) z[j] + D_8036BED8[i].z;
            }
        }
    }
    gDPLoadTextureBlock(gfx++, osVirtualToPhysical(D_802F9A00), G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 0, G_TX_CLAMP,
                        G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    for (i = 0; i < D_8036EB90; i++) {
        if (D_8036BED8[i].collected == 0 && func_80270A54(&D_8036BED8[i])) {
            gSPVertex(gfx++, D_8036BED8[i].vtx[D_8036BEE0], 4, 0);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
        }
    }
    gDPPipeSync(gfx++);
    gDPLoadTextureBlock(gfx++, osVirtualToPhysical(D_802F9E00), G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 0, G_TX_CLAMP,
                        G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    for (i = 0; i < D_8036EB90; i++) {
        if (D_8036BED8[i].collected != 0 && func_80270A54(&D_8036BED8[i])) {
            gSPVertex(gfx++, D_8036BED8[i].vtx[D_8036BEE0], 4, 0);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
        }
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
    D_8036BEDC = D_80364414;
}

s32 func_80270A54(Rdu *arg0) {
    s32 i;
    u8 spB;

    i = 0;
    spB = arg0->cell;
    while (D_803C30A8[i] != 0xFFFF) {
        if (D_803C30A8[i++] == spB) {
            return 1;
        }
    }
    return 0;
}

void func_80270AE0(u8 *arg0) {
    s32 argc;
    u8 *argv[33];
    u8 **av;
    s32 pad;
    u8 *p;

    argc = 1;
    av = argv;
    if (arg0 != NULL && *arg0 != 0) {
        p = arg0;
        while (*p != 0) {
            while (*p != 0 && *p == ' ') {
                *p = 0;
                p++;
            }
            if (*p != 0) {
                argv[argc] = p;
                argc++;
            }
            while (*p != 0 && *p != ' ') {
                p++;
            }
        }
        while (argc >= 2 && av[1][0] == '-') {
            switch (av[1][1]) {
                case 'd':
                    D_802FA254 = 1;
                    break;
                case 'v':
                    D_802FA250 = 1;
                    break;
                case 's':
                    D_802FA258 = 1;
                    break;
                case 'j':
                    D_802FA25C = 1;
                    break;
                case 'm':
                    D_802FA260 = 1;
                    break;
                case 'l':
                    D_802FA264 = 1;
                    break;
                case 'c':
                    D_802FA268 = 1;
                    break;
                case 'C':
                    D_802FA26C = 1;
                    D_802FA268 = 1;
                    break;
            }
            argc--;
            av++;
        }
    }
}
