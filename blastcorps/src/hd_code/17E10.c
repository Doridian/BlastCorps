#include "common.h"
#include "game/sched.h"
#include "game/game.h"
#include "game/audio.h"
#include "game/level.h"
#include "game/player.h"

void func_8024FC2C(Gfx **gfxp, s32 arg1);
Gfx *func_8025D2B4(Gfx *gfx, s32 arg1, s32 *arg2);
void func_8025E1E0(Gfx **gfxp);
SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802609D0(void);
void func_802609F0(void);
void func_80260A10(void);
void func_80260DFC(void);
void func_80261570(f32);
s32 func_8026B10C(void);
void func_80275390(u64);
void func_80260E2C(void);
void func_80260EE0(s32);
s32 func_8026205C(s32);
void func_8026AF6C(s32);
void func_80275270(u64, f32);
s32 func_802753C0(void);
void func_80277EDC(s32, s32, s32, s32);
void func_80278318(void);
void func_802C1DD0(s32);
s32 func_802D4E10(ALCSPlayer *);
void func_8028B4C4(u32 romAddr, void *dest, s32 *size, s32 arg3, s32 arg4, s32 arg5);

/* Segment 2 base and three ROM addresses, reached through relocations. */
extern Mtx D_02000000[];
extern u8 D_00487050[];
extern u8 D_00489E70[];
extern u8 D_0048F5A0[];
extern u8 D_802E8BF0;
extern u8 D_803643D6;
extern u8 D_803643D7;
extern u8 D_803643D8;
extern s16 D_8036BB18;
extern s16 D_8036BB1A;
extern s16 D_8036BB1C;
extern Vtx D_802FA8B0[][4];
extern s16 D_8039CAA0;


/* .bss, 0x80366A00-0x80366BD0 (tools/bss_c.py) */
s16 D_80366A00;
s16 D_80366A02;
s16 D_80366A04;
u8 D_80366A06[2];
u8 D_80366A08[8];
s8 D_80366A10;
s8 D_80366A11;
u16 D_80366A12;
s16 D_80366A14;
s16 D_80366A16;
u8 D_80366A18;
u8 D_80366A19[1];
u8 D_80366A1A[2];
u8 D_80366A1C[4];
u8 D_80366A20[0x180];
Vtx *D_80366BA0;
u8 *D_80366BA4;
s32 D_80366BA8;
u32 D_80366BB0[2];
u32 D_80366BB8;
u32 D_80366BBC;
u8 D_80366BC0;
u16 D_80366BC2;
u8 D_80366BC4;
u8 D_80366BC5;

/* .data, 0x802E8CD0-0x802E8CE0 (tools/data_c.py) */
#ifdef VERSION_EU
u32 D_802E8CD0[4] = { 25, 67 }; /* frames: eu's are 50 a second */
#else
u32 D_802E8CD0[4] = { 30, 80 };
#endif

void func_8025C5D0(void) {
    switch (D_802E8BEC) {
        case 0:
            if (D_80364A90 == 2) {
                if (D_80358060 == 0x96) {
                    func_8026AF6C(0x803E);
                }
                if (D_80358060 == 0x190) {
                    func_8026AF6C(0x8025);
                }
                if (D_80358060 == 0x2BC) {
                    func_8026AF6C(0x8026);
                }
            } else {
                if (D_80358060 == 0x64) {
                    func_8026AF6C(0x8027);
                }
                if (D_80358060 == 0x12C) {
                    func_8026AF6C(0x8028);
                }
                if (D_80358060 == 0x1F4) {
                    func_8026AF6C(0x8029);
                }
                if (D_80358060 == 0x2BC) {
                    func_8026AF6C(0x802A);
                }
            }
            break;
        case 1:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x802B);
            }
            if (D_80358060 == 0x1D6) {
                func_8026AF6C(0x802C);
            }
            break;
        case 2:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x802D);
            }
            break;
        case 3:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x802E);
            }
            break;
        case 4:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x802F);
            }
            break;
        case 5:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x8030);
            }
            break;
        case 6:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x8031);
            }
            if (D_80358060 == 0x1D6) {
                func_8026AF6C(0x8032);
            }
            break;
        case 7:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x8033);
            }
            break;
        case 8:
            if (D_80358060 == 0xB4) {
                func_8026AF6C(0x8034);
            }
            break;
    }
    if (D_80358060 == 0x82) {
        D_80366A18 = 1;
    }
}

Gfx *func_8025C878(Gfx *arg0, s32 arg1, u8 arg2, s32 *arg3) {
    u8 *sp6C;
    u32 sp68;
    Gfx *gfx;
    s32 i;

    sp68 = D_803156C4;
    gfx = arg0;
    if (D_802E8BF0 != 0 && (D_80364A90 & 2)) {
        if (arg2 != 0) {
            sp6C = D_80366BA4;
        } else {
            sp6C = D_80366BA4 + 0x3C0;
        }
        D_80366BA8 = 0;
        if (D_80358060 == 0) {
            D_80366A10 = 0;
            D_80366A11 = 0;
        }
        if (D_80358060 > D_80366A04) {
            if (sp68 < D_80366BBC + 0x78) {
                gDPPipeSync(gfx++);
                gSPMatrix(gfx++, &D_02000000[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
                gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
                gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
                gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
                gSPVertex(gfx++, (u32)D_802FA8B0[arg2] + 0x80000000, 4, 0);
                gSP1Triangle(gfx++, 0, 1, 2, 0);
                gSP1Triangle(gfx++, 0, 2, 3, 0);
                for (i = 0; i < 4; i++) {
                    D_802FA8B0[arg2][i].v.cn[0] = 0;
                    D_802FA8B0[arg2][i].v.cn[1] = 0;
                    D_802FA8B0[arg2][i].v.cn[2] = 0;
                    D_802FA8B0[arg2][i].v.cn[3] = (sp68 - D_80366BBC) * 2.125;
                }
                func_8025E1E0(&gfx);
            } else {
                gDPPipeSync(gfx++);
                gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
                gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
                gDPSetCycleType(gfx++, G_CYC_FILL);
                gDPSetFillColor(gfx++, 0x00010001);
                gDPFillRectangle(gfx++, 0, 0, 319, 239);
                gDPPipeSync(gfx++);
                gDPSetCycleType(gfx++, G_CYC_1CYCLE);
                func_8025E1E0(&gfx);
            }
        } else {
            if (D_80366A04 == D_80358060) {
                D_80366BBC = sp68;
            }
            func_8025E1E0(&gfx);
        }
    } else {
        gfx = func_8025D2B4(gfx, arg1, arg3);
    }
    *arg3 += gfx - arg0;
    return gfx;
}

void func_8025CE74(void) {
    D_80366BA0 = (Vtx *)D_80358070;
    D_80358070 += 4 * sizeof(Vtx);
    D_80366BA0[0].v.ob[0] = 0;
    D_80366BA0[0].v.ob[1] = 0;
    D_80366BA0[0].v.ob[2] = 0;
    D_80366BA0[0].v.flag = 0;
    D_80366BA0[0].v.tc[0] = 0;
    D_80366BA0[0].v.tc[1] = 0x9E0;
    D_80366BA0[0].v.cn[0] = 0;
    D_80366BA0[0].v.cn[1] = 0;
    D_80366BA0[0].v.cn[2] = 0;
    D_80366BA0[0].v.cn[3] = 0;
    D_80366BA0[1].v.ob[0] = 0x14;
    D_80366BA0[1].v.ob[1] = 0;
    D_80366BA0[1].v.ob[2] = 0;
    D_80366BA0[1].v.flag = 0;
    D_80366BA0[1].v.tc[0] = 0x280;
    D_80366BA0[1].v.tc[1] = 0x9E0;
    D_80366BA0[1].v.cn[0] = 0;
    D_80366BA0[1].v.cn[1] = 0;
    D_80366BA0[1].v.cn[2] = 0;
    D_80366BA0[1].v.cn[3] = 0;
    D_80366BA0[2].v.ob[0] = 0x14;
    D_80366BA0[2].v.ob[1] = 0x4F;
    D_80366BA0[2].v.ob[2] = 0;
    D_80366BA0[2].v.flag = 0;
    D_80366BA0[2].v.tc[0] = 0x280;
    D_80366BA0[2].v.tc[1] = 0;
    D_80366BA0[2].v.cn[0] = 0;
    D_80366BA0[2].v.cn[1] = 0;
    D_80366BA0[2].v.cn[2] = 0;
    D_80366BA0[2].v.cn[3] = 0;
    D_80366BA0[3].v.ob[0] = 0;
    D_80366BA0[3].v.ob[1] = 0x4F;
    D_80366BA0[3].v.ob[2] = 0;
    D_80366BA0[3].v.flag = 0;
    D_80366BA0[3].v.tc[0] = 0;
    D_80366BA0[3].v.tc[1] = 0;
    D_80366BA0[3].v.cn[0] = 0;
    D_80366BA0[3].v.cn[1] = 0;
    D_80366BA0[3].v.cn[2] = 0;
    D_80366BA0[3].v.cn[3] = 0;
    D_80366BA4 = D_80358070;
    D_80358070 += 0x780;
}

void func_8025D0B0(u8 arg0) {
    u32 romAddr;
    s32 size;

    switch (arg0) {
        case 1:
            romAddr = (u32)D_00489E70;
            size = D_0048F5A0 - D_00489E70;
            break;
        case 0:
            romAddr = (u32)D_00487050;
            size = D_00489E70 - D_00487050;
            break;
    }
    func_8028B4C4(romAddr, D_80358070, &size, 0xC, 0, 1);
    D_80366BB0[arg0] = K0_TO_PHYS(D_80358070);
    D_80358070 += size;
}

void func_8025D184(void) {
    if (D_80364A98 & 2) {
        func_8025D0B0(1);
        func_8025D0B0(0);
        D_80366A16 = 0xFF;
        D_80366A12 = 0;
        D_80366A00 = 0xA5;
        D_80366A02 = 0xD;
    } else if (D_80364A98 & 0x40000) {
        D_80366A16 = 0;
        D_80366A12 = 4;
        D_80366A02 = 0x2A;
    } else if (D_80364A98 & 0x10000) {
        D_80366A16 = 0;
        D_80366A12 = 3;
    } else {
        func_8025D0B0(1);
        func_8025D0B0(0);
        D_80366A12 = 3;
        D_80366A14 = 0;
        D_80366A00 = 0x10;
    }
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/17E10/func_8025D2B4.s")
#else
Gfx *func_8025D2B4(Gfx *arg0, s32 arg1, s32 *arg2) {
    Gfx *gfx;
    s32 x;
    s32 y;
    s32 xoff;
    s16 yoff;

    gfx = arg0;
    if (D_80364A90 & 0x08040E2110418002) {
        D_80366A14 += 10;
        if (D_80366A14 >= 0x100) {
            D_80366A14 = 0xFF;
        }
    } else if (D_80364A90 & 0x0188004203160000) {
        D_80366A14 -= 10;
        if (D_80366A14 <= 0) {
            D_80366A14 = 0;
        }
    }
    switch (D_80366A12) {
        case 0:
            if (D_80358060 == 60) {
                D_80366A12 = 1;
            }
            break;
        case 1:
            D_80366A16 -= 8;
            if (D_80366A16 < 0) {
                D_80366A16 = 0;
                D_80366A12 = 2;
            }
            break;
        case 2:
            D_80366A00 -= 10;
            if (D_80366A00 <= 0x10) {
                D_80366A00 = 0x10;
                D_80366A12 = 3;
            }
            break;
        case 4:
            if (D_8036BB1C == 2) {
                if (D_80366A16 + 4 > 0x80) {
                    D_80366A16 = 0x80;
                } else {
                    D_80366A16 += 4;
                }
            } else if (D_80364A90 == 0x200000) {
                if (D_80366A16 - 6 < 0) {
                    D_80366A16 = 0;
                } else {
                    D_80366A16 -= 6;
                }
            }
            break;
    }
    gDPPipeSync(gfx++);
    gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
    if (D_80364A90 == 2 && D_8036BB1C != 1) {
        if ((yoff = D_80366A00 - (0xFF - D_8039CAA0) / 4) + 63 == D_80366A00) {
            yoff = -100;
        }
    } else {
        yoff = D_80366A00;
    }
    switch (D_80366A12) {
        case 0:
        case 1:
        case 4:
            gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_80366A16);
            xoff = 82;
            for (x = 0; x < 150; x += 31) {
                for (y = 0; y < 150; y += 31) {
                    gDPLoadTextureTile(gfx++, D_80366BB0[1], G_IM_FMT_RGBA, G_IM_SIZ_16b, 156, 0, x, y, x + 31,
                                       y + 31, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                                       G_TX_NOLOD);
                    gSPTextureRectangle(gfx++, (x + xoff) << 2, (y + D_80366A02) << 2, (x + xoff + 31) << 2,
                                        (y + D_80366A02 + 31) << 2, G_TX_RENDERTILE, x << 5, y << 5, 1 << 10,
                                        1 << 10);
                }
            }
            if (D_80366A12 == 4) {
                break;
            }
            /* fallthrough */
        default:
            gDPPipeSync(gfx++);
            gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0,
                              PRIMITIVE, 0);
            gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_80366A14 / 2);
            xoff = 20;
            for (x = 0; x < 272; x += 31) {
                gDPLoadTextureTile(gfx++, D_80366BB0[0], G_IM_FMT_RGBA, G_IM_SIZ_16b, 280, 0, x, 0, x + 31, 63, 0,
                                   G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                gDPPipeSync(gfx++);
                gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
                gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0,
                                  PRIMITIVE, 0);
                gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_80366A14 / 2);
                gSPScisTextureRectangle(gfx++, (x + xoff) << 2, (yoff + 6) << 2, (x + xoff + 31) << 2,
                                        (yoff + 69) << 2, G_TX_RENDERTILE, x << 5, 0, 1 << 10, 1 << 10);
                gDPPipeSync(gfx++);
                if (D_80366A14 == 0xFF && (D_80364A90 & 0xC9FD0FE79BFF80B0)) {
                    gDPSetRenderMode(gfx++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
                }
                gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0,
                                  PRIMITIVE, 0);
                gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_80366A14);
                gSPScisTextureRectangle(gfx++, (x + xoff) << 2, yoff << 2, (x + xoff + 31) << 2, (yoff + 63) << 2,
                                        G_TX_RENDERTILE, x << 5, 0, 1 << 10, 1 << 10);
            }
            break;
    }
    gDPPipeSync(gfx++);
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    *arg2 += gfx - arg0;
    return gfx;
}
#endif

void func_8025E1E0(Gfx **gfxp) {
    Gfx *gfx;

    gfx = *gfxp;
    gSPMatrix(gfx++, &D_02000000[2], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_02000000[5], G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    func_8024FC2C(&gfx, 0);
    func_8024FC2C(&gfx, 1);
    func_8024FC2C(&gfx, 2);
    *gfxp = gfx;
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/17E10/func_8025E2CC.s")
#else
void func_8025E2CC(Gfx **gfxp, s32 arg1, s32 arg2) {
    Gfx *gfx;

    gfx = *gfxp;
    if (D_803643D7 != 0 && func_802753C0() == 0) {
        if (D_80366BC0 == 0) {
            func_802C1DD0(D_802E8F94[D_802E8BDC].unk0 == 0x20 || D_802E8F94[D_802E8BDC].unk0 == 0x80);
            D_80366BB8 = 0;
            if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) {
                func_802609F0();
                func_80260A10();
                D_80366BC2 = 5;
                D_80366BC4 = 0;
                if (D_80364AA8 != 1) {
                    func_80260E2C();
                    D_80366BC4 = 1;
                }
            } else {
                switch (D_802E8BDC) {
                    case 40:
                        func_80260A10();
                        func_80260DFC();
                        D_80366BC2 = 0x1E;
                        D_80366BC4 = 0;
                        break;
                    case 50:
                        func_80260A10();
                        func_80260EE0(0x25);
                        D_80366BC2 = 0x23;
                        D_80366BC4 = 0;
                        break;
                    default:
                        func_80260A10();
                        D_80366BC2 = 5;
                        func_80260E2C();
                        D_80366BC4 = 1;
                        break;
                }
            }
            func_8026AF6C(D_80366BC2 | 0x8000 | 0x2000);
            D_8036BB1A = -1;
            if (D_80366BC2 == 5) {
                func_80260650(D_80367738, func_8026205C(2), NULL);
            }
        }
        D_80366BC0 = D_803643D7;
        if (D_80366BB8 == 0) {
            if (D_8036BB1C == 8 && D_8036BB18 == D_80366BC2) {
                D_80366BB8 = D_803156C4;
                func_80278318();
                func_80277EDC(2, 1, 2, func_8026205C(3));
            }
        } else if (func_802D4E10(D_80367734) == 0 ||
                   (D_80366BC4 != 0 && D_803156C4 - D_80367740 > 480) ||
                   (D_80366BC4 == 0 && D_803156C4 - D_80366BB8 > D_802E8CD0[(D_80364AA8 & 0x81) ? 1 : 0])) {
            func_80275270(0x08000000, 0.75f);
            D_80366BB8 = 0;
            D_80366BC0 = 0;
        }
    }
    *gfxp = gfx;
}
#endif

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/17E10/func_8025E67C.s")
#else
void func_8025E67C(Gfx **gfxp, s32 arg1, u8 arg2) {
    Gfx *gfx;
    u32 now;
    u32 i;
    u32 j;
    s32 unused;
    u32 alpha;

    gfx = *gfxp;
    now = D_803156C4;
    if (D_803643D6 != 0) {
        if (D_803643D8 == 0) {
            func_802609D0();
            func_802C1DD0(D_802E8F94[D_802E8BDC].unk0 == 0x20 || D_802E8F94[D_802E8BDC].unk0 == 0x80);
            switch (D_802E8BDC) {
                case 49:
                    func_80260650(D_80367738, 0x31, NULL);
                    func_80261570(0.0f);
                    break;
                case 50:
                    D_8036BB1A = -1;
                    func_8026AF6C(0xA00E);
                    func_80261570(0.0f);
                    break;
                default:
                    func_80260650(D_80367738, 0x31, NULL);
                    D_802E8BD8 = 1;
                    if (D_8036BB18 != -1 || func_8026B10C() != 0) {
                        func_8026AF6C(0x4000);
                    }
                    D_8036BB1A = -1;
                    func_80261570(0.0f);
                    break;
            }
            D_80366BB8 = now;
            D_80366BC5 = 0;
        }
        if ((i = now - D_80366BB8) >= 180) {
            switch (D_802E8BDC) {
                case 49:
                    if (D_80366BC5 == 0) {
                        func_80260650(D_80367738, 0x32, NULL);
                        D_80366BC5 = 1;
                    }
                    break;
                case 50:
                    if (D_8036BB1C == 1 && func_802753C0() == 0) {
                        if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) {
                            func_80275390(0x08000000);
                        } else {
                            func_80275390(0x40);
                        }
                    }
                    break;
                default:
                    if (D_80366BC5 == 0) {
                        func_80260650(D_80367738, 0x32, NULL);
                        D_80366BC5 = 1;
                    }
                    gSPMatrix(gfx++, &D_02000000[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
                    gSPMatrix(gfx++, &D_02000000[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                    gDPPipeSync(gfx++);
                    gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
                    gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
                    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
                    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
                    gSPVertex(gfx++, (u32)D_802FA8B0[arg2] + 0x80000000, 4, 0);
                    gSP1Triangle(gfx++, 0, 1, 2, 0);
                    gSP1Triangle(gfx++, 0, 2, 3, 0);
                    gDPPipeSync(gfx++);
                    if (func_802753C0() == 0) {
                        if (now - D_80366BB8 - 180 < 90) {
                            alpha = (now - D_80366BB8 - 180) * 2.8333333333333335;
                            for (i = 0; i < 4; i++) {
                                for (j = 0; j < 4; j++) {
                                    D_802FA8B0[arg2][i].v.cn[j] = alpha;
                                }
                            }
                        } else if (now - D_80366BB8 - 270 >= 46) {
                            if (D_80364A90 == 0x100000000000) {
                                D_80364A98 = 0x200000000000;
                            } else if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) {
                                func_80275390(0x08000000);
                            } else {
                                func_80275390(0x40);
                            }
                        } else {
                            for (i = 0; i < 4; i++) {
                                for (j = 0; j < 4; j++) {
                                    D_802FA8B0[arg2][i].v.cn[j] = 0xFF;
                                }
                            }
                        }
                    }
                    break;
            }
        }
    }
    *gfxp = gfx;
}
#endif
