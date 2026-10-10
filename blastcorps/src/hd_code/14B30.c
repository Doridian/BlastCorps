#include "common.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

/* 12-byte sort records: a u16 key, a quad index and a texture address. */
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} UnkStruct_80365340; /* size = 0xC */


void func_802597D8(u8 *arg0, u8 *arg1, s32 arg2);
s32 func_80259814(u16 *arg0, u16 *arg1);
void func_8025946C(Gfx **arg0, s32 arg1);
void func_80259824(Gfx **arg0, s32 arg1);

/* .bss, 0x80365340-0x80365360 (tools/bss_c.py) */
UnkStruct_80365340 *D_80365340;
Vtx *D_80365348[2];
s32 D_80365350;

/* .data, 0x802E8C70-0x802E8C80 (tools/data_c.py) */
s32 D_802E8C70 = 0;
s32 D_802E8C74 = 0;
s32 D_802E8C78 = 0;

void func_802592F0(void) {
    s32 sp1C;

    if ((D_80364A98 & 0xC9FD8FE7DBFF8080) || (D_80364A98 == 0x100000000000) || (D_80364A98 == 2) ||
        (D_802E8BDC == 0x28) || (D_802E8BDC == 0x32)) {
        D_80365350 = 0x200;
    } else if (D_80364A98 == 0x40) {
        D_80365350 = 0x9C;
    } else {
        D_80365350 = 0xAC;
    }
    for (sp1C = 0; sp1C < 2; sp1C++) {
        D_80365348[sp1C] = (Vtx *)D_80358070;
        D_80358070 += D_80365350 * 0x10 * 4;
    }
    D_80365340 = (UnkStruct_80365340 *)D_80358070;
    D_80358070 += D_80365350 * 0xC;
    func_8025B070();
}

void func_80259450(void) {
    D_802E8C70 = 0;
    D_802E8C74 = 0;
    D_802E8C78 = 0;
}

void func_8025946C(Gfx **arg0, s32 arg1) {
    Gfx *gfx = *arg0;

    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    *arg0 = gfx;
}

/* Shell sort. */
void func_802595E0(u8 *arg0, s32 arg1, s32 arg2, s32 (*arg3)(void *, void *)) {
    s32 sp124;
    s32 sp120;
    s32 sp11C;
    u8 *sp118;
    u8 sp18[0x100];

    sp118 = arg0;
    for (sp11C = 1; sp11C <= arg1 / 9; sp11C = sp11C * 3 + 1) {
    }
    for (; sp11C > 0; sp11C /= 3) {
        for (sp124 = sp11C; sp124 < arg1; sp124++) {
            func_802597D8(sp18, sp118 + arg2 * sp124, arg2);
            sp120 = sp124;
            while ((sp120 >= sp11C) && (arg3(sp118 + (sp120 - sp11C) * arg2, sp18) > 0)) {
                func_802597D8(sp118 + arg2 * sp120, sp118 + (sp120 - sp11C) * arg2, arg2);
                sp120 -= sp11C;
            }
            func_802597D8(sp118 + arg2 * sp120, sp18, arg2);
        }
    }
}

void func_802597D8(u8 *arg0, u8 *arg1, s32 arg2) {
    s32 sp4;

    for (sp4 = 0; sp4 < arg2; sp4++) {
        arg0[sp4] = arg1[sp4];
    }
}

s32 func_80259814(u16 *arg0, u16 *arg1) {
    return *arg0 - *arg1;
}

void func_80259824(Gfx **arg0, s32 arg1) {
    s32 sp54;
    Gfx *gfx;
    s32 sp4C;

    sp54 = 0;
    gfx = *arg0;
    func_802595E0((u8 *)&D_80365340[D_802E8C70], D_802E8C74 - D_802E8C70, sizeof(UnkStruct_80365340),
                  (s32(*)(void *, void *))func_80259814);
    for (sp4C = D_802E8C70; sp4C < D_802E8C74; sp4C++) {
        if (D_80365340[sp4C].unk8 != sp54) {
            sp54 = D_80365340[sp4C].unk8;
            if (sp4C == D_802E8C70) {
                gDPLoadTextureBlock_4b(gfx++, OS_PHYSICAL_TO_K0(sp54), G_IM_FMT_I, 32, 32, 0,
                                       G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK,
                                       G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            } else {
                gDPSetTextureImage(gfx++, G_IM_FMT_I, G_IM_SIZ_16b, 1, OS_PHYSICAL_TO_K0(sp54));
                gDPLoadSync(gfx++);
                gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, 255, 1024);
            }
        }
        gSPVertex(gfx++, &D_80365348[D_8035805C][D_80365340[sp4C].unk4 * 4], 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
    }
    osWritebackDCache(&D_80365348[D_8035805C][D_802E8C70 * 4], (D_802E8C74 - D_802E8C70) << 6);
    D_802E8C70 = D_802E8C74;
    *arg0 = gfx;
}

void func_80259BD4(Gfx **arg0, s32 arg1) {
    Gfx *sp1C;

    sp1C = *arg0;
    func_8025946C(&sp1C, arg1);
    func_80259824(&sp1C, arg1);
    *arg0 = sp1C;
}

void func_80259C24(Gfx **arg0, s32 arg1) {
    Gfx *gfx = *arg0;

    gSPMatrix(gfx++, arg1 + 0xC0, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg1 + 0x1C0, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    func_8025946C(&gfx, arg1);
    func_80259824(&gfx, arg1);
    *arg0 = gfx;
}

void func_80259EC4(s32, u8 *, u16 *, u8, s32, f32, s32, f32, s32, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8,
                   u8, u8, u8, u8);

void func_80259CCC(s32 arg0, u8 *arg1, u16 *arg2, u8 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12, u8 arg13) {
    func_80259EC4(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg10, arg11,
                  arg12, arg13, arg10, arg11, arg12, arg13, arg10, arg11, arg12, arg13);
}

void func_80259DC8(s32 arg0, u8 *arg1, u16 *arg2, u8 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12, u8 arg13, u8 arg14, u8 arg15, u8 arg16, u8 arg17) {
    func_80259EC4(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg10, arg11,
                  arg12, arg13, arg14, arg15, arg16, arg17, arg14, arg15, arg16, arg17);
}

extern f32 D_802E8C84[];
extern u16 D_802E8C8C[];
extern u16 D_802E8C90[];
extern u16 D_802E8C94[];


#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/14B30/func_80259EC4.s")
#else
void func_80259EC4(s32 arg0, u8 *arg1, u16 *arg2, u8 arg3, s32 arg4, f32 arg5, s32 arg6, f32 arg7, s32 arg8,
                   u8 arg9, u8 arg10, u8 arg11, u8 arg12, u8 arg13, u8 arg14, u8 arg15, u8 arg16, u8 arg17,
                   u8 arg18, u8 arg19, u8 arg20, u8 arg21, u8 arg22, u8 arg23, u8 arg24, u8 arg25) {
    u16 sp3E;
    u8 *sp38;
    u16 *sp34;
    u8 sp33;
    f32 sp2C;
    u8 sp2B;
    u8 sp2A;
    s32 sp24;

    sp33 = 0;
    sp2B = 0;
    sp2A = 0;
    if (arg1 == NULL || *arg1 == 0) {
        return;
    }
    sp38 = arg1;
    sp34 = arg2;
    if (arg9 == 0) {
        if (sp2B != 0) {
            while (*sp34 != 0xFFE) {
                sp34++;
            }
            sp34--;
        } else {
            while (*sp38 != 0) {
                sp38++;
            }
            sp38--;
        }
    }
    if (arg4 != 0) {
        arg5 = func_8025B498(arg4, arg7, arg1, arg2);
    }
    while (sp33 == 0) {
        switch (sp2A) {
            case 0:
                sp2C = 0.21f;
                switch (*sp38) {
                    case '0':
                        sp3E = 0;
                        sp2C = 0.25f;
                        break;
                    case '1':
                        sp3E = 1;
                        sp2C = 0.25f;
                        break;
                    case '2':
                        sp3E = 2;
                        sp2C = 0.25f;
                        break;
                    case '3':
                        sp3E = 3;
                        sp2C = 0.25f;
                        break;
                    case '4':
                        sp3E = 4;
                        sp2C = 0.25f;
                        break;
                    case '5':
                        sp3E = 5;
                        sp2C = 0.25f;
                        break;
                    case '6':
                        sp3E = 6;
                        sp2C = 0.25f;
                        break;
                    case '7':
                        sp3E = 7;
                        sp2C = 0.25f;
                        break;
                    case '8':
                        sp3E = 8;
                        sp2C = 0.25f;
                        break;
                    case '9':
                        sp3E = 9;
                        sp2C = 0.25f;
                        break;
                    case 'A':
                        sp3E = 10;
                        break;
                    case 'B':
                        sp3E = 11;
                        break;
                    case 'C':
                        sp3E = 12;
                        break;
                    case 'D':
                        sp3E = 13;
                        break;
                    case 0x7F:
                        sp3E = 14;
                        sp2C = 0.05f;
                        break;
                    case 'E':
                        sp3E = 15;
                        break;
                    case 'F':
                        sp3E = 16;
                        break;
                    case 'G':
                        sp3E = 17;
                        break;
                    case 'H':
                        sp3E = 18;
                        break;
                    case 'I':
                        sp3E = 19;
                        sp2C = 0.27f;
                        break;
                    case 'J':
                        sp3E = 20;
                        break;
                    case 'K':
                        sp3E = 21;
                        break;
                    case 'L':
                        sp3E = 22;
                        break;
                    case 'M':
                        sp3E = 23;
                        sp2C = 0.05f;
                        break;
                    case 'N':
                        sp3E = 24;
                        break;
                    case 'O':
                        sp3E = 25;
                        break;
                    case 'P':
                        sp3E = 26;
                        break;
                    case 'Q':
                        sp3E = 27;
                        break;
                    case 'R':
                        sp3E = 28;
                        break;
                    case 'S':
                        sp3E = 29;
                        break;
                    case 'T':
                        sp3E = 30;
                        break;
                    case 'U':
                        sp3E = 31;
                        break;
                    case 'V':
                        sp3E = 32;
                        break;
                    case 'W':
                        sp3E = 33;
                        sp2C = 0.05f;
                        break;
                    case 'X':
                        sp3E = 34;
                        break;
                    case 'Y':
                        sp3E = 35;
                        break;
                    case 'Z':
                        sp3E = 36;
                        break;
                    case '\'':
                        sp3E = 38;
                        sp2C = 0.25f;
                        break;
                    case ')':
                        sp3E = 39;
                        sp2C = 0.25f;
                        break;
                    case ':':
                        sp3E = 40;
                        sp2C = 0.36f;
                        break;
                    case ',':
                        sp3E = 41;
                        sp2C = 0.25f;
                        break;
                    case '$':
                        sp3E = 42;
                        break;
                    case '!':
                        sp3E = 43;
                        sp2C = 0.25f;
                        break;
                    case '.':
                        sp3E = 44;
                        sp2C = 0.3f;
                        break;
                    case '-':
                        sp3E = 45;
                        sp2C = 0.25f;
                        break;
                    case '(':
                        sp3E = 46;
                        sp2C = 0.25f;
                        break;
                    case '%':
                        sp3E = 47;
                        break;
                    case '?':
                        sp3E = 48;
                        break;
                    case '#':
                        sp3E = 49;
                        break;
                    case '/':
                        sp3E = 50;
                        break;
                    case ' ':
                    case '&':
                        sp3E = 0;
                        sp2C = 0.3f;
                        break;
                    case 'a':
                    case 'b':
                    case 'd':
                    case 'e':
                    case 'k':
                    case 'm':
                        sp3E = *sp38 - 0x22C;
                        break;
                    default:
                        sp3E = 0;
                        sp2C = 0.0f;
                        break;
                }
                sp3E += 0xF4C;
                break;
            case 1:
                sp2C = 1.0f;
                if (*sp34 < 0x200) {
                    sp3E = *sp34 + 0xD4C;
                } else if (*sp34 == 0x1001) {
                    sp3E = 0xF7D;
                } else {
                    sp3E = 0xD4C;
                }
                break;
        }
        if (arg3 == 1) {
            if (arg9 != 0) {
                arg5 -= sp2C * arg7;
            } else {
                arg5 += sp2C * arg7;
            }
        }
        if (sp2B != 0) {
            sp24 = *sp34;
        } else {
            sp24 = *sp38;
        }
        if (D_802E8C94[sp2A] != sp24 && D_802E8C90[sp2A] != sp24 && D_802E8C8C[sp2A] != sp24 &&
            (arg13 != 0 || arg17 != 0 || arg21 != 0 || arg25 != 0)) {
            D_80365348[D_8035805C][D_802E8C78].v.ob[0] = arg5;
            D_80365348[D_8035805C][D_802E8C78].v.ob[1] = arg6;
            D_80365348[D_8035805C][D_802E8C78].v.ob[2] = -10;
            D_80365348[D_8035805C][D_802E8C78].v.flag = 0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[0] = 0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[1] = 0x3E0;
            D_80365348[D_8035805C][D_802E8C78].v.cn[0] = arg10;
            D_80365348[D_8035805C][D_802E8C78].v.cn[1] = arg11;
            D_80365348[D_8035805C][D_802E8C78].v.cn[2] = arg12;
            D_80365348[D_8035805C][D_802E8C78].v.cn[3] = arg13;
            D_802E8C78++;
            D_80365348[D_8035805C][D_802E8C78].v.ob[0] = arg5 + arg7;
            D_80365348[D_8035805C][D_802E8C78].v.ob[1] = arg6;
            D_80365348[D_8035805C][D_802E8C78].v.ob[2] = -10;
            D_80365348[D_8035805C][D_802E8C78].v.flag = 0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[0] = 0x3E0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[1] = 0x3E0;
            D_80365348[D_8035805C][D_802E8C78].v.cn[0] = arg14;
            D_80365348[D_8035805C][D_802E8C78].v.cn[1] = arg15;
            D_80365348[D_8035805C][D_802E8C78].v.cn[2] = arg16;
            D_80365348[D_8035805C][D_802E8C78].v.cn[3] = arg17;
            D_802E8C78++;
            D_80365348[D_8035805C][D_802E8C78].v.ob[0] = arg5 + arg7;
            D_80365348[D_8035805C][D_802E8C78].v.ob[1] = arg6 + arg8;
            D_80365348[D_8035805C][D_802E8C78].v.ob[2] = -10;
            D_80365348[D_8035805C][D_802E8C78].v.flag = 0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[0] = 0x3E0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[1] = 0;
            D_80365348[D_8035805C][D_802E8C78].v.cn[0] = arg18;
            D_80365348[D_8035805C][D_802E8C78].v.cn[1] = arg19;
            D_80365348[D_8035805C][D_802E8C78].v.cn[2] = arg20;
            D_80365348[D_8035805C][D_802E8C78].v.cn[3] = arg21;
            D_802E8C78++;
            D_80365348[D_8035805C][D_802E8C78].v.ob[0] = arg5;
            D_80365348[D_8035805C][D_802E8C78].v.ob[1] = arg6 + arg8;
            D_80365348[D_8035805C][D_802E8C78].v.ob[2] = -10;
            D_80365348[D_8035805C][D_802E8C78].v.flag = 0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[0] = 0;
            D_80365348[D_8035805C][D_802E8C78].v.tc[1] = 0;
            D_80365348[D_8035805C][D_802E8C78].v.cn[0] = arg22;
            D_80365348[D_8035805C][D_802E8C78].v.cn[1] = arg23;
            D_80365348[D_8035805C][D_802E8C78].v.cn[2] = arg24;
            D_80365348[D_8035805C][D_802E8C78].v.cn[3] = arg25;
            D_802E8C78++;
            D_80365340[D_802E8C74].unk0 = sp24;
            if (arg10 != 0 || arg12 != 0 || arg11 != 0) {
                D_80365340[D_802E8C74].unk0 += 0x8000;
            }
            D_80365340[D_802E8C74].unk4 = D_802E8C74;
            D_80365340[D_802E8C74].unk8 = (s32)func_8025B0B8(sp3E);
            if (!(++D_802E8C74 < D_80365350)) {
                func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n", "index<maxCharacters", "drawtext.c",
                              435);
            }
            if (D_802E8C74 >= D_80365350) {
                func_8029A7E4("%d %d\n", D_802E8C74, D_80365350);
            }
        }
        if (arg3 == 1) {
            if (arg9 != 0) {
                arg5 += arg7 - sp2C * arg7;
            } else {
                arg5 -= arg7 - sp2C * arg7;
            }
        } else if (D_802E8C8C[sp2A] != sp24) {
            if (arg9 != 0) {
                arg5 += arg7 * D_802E8C84[sp2A];
            }
            if (arg9 == 0) {
                arg5 -= arg7 * D_802E8C84[sp2A];
            }
        }
        if (arg9 != 0) {
            if (sp2B != 0) {
                if (*++sp34 == 0xFFE) {
                    sp33 = 1;
                }
            } else {
                if (*++sp38 == 0) {
                    sp33 = 1;
                }
            }
        } else if (sp2B != 0) {
            if (sp34 == arg2) {
                sp33 = 1;
            }
            sp34--;
        } else {
            if (sp38 == arg1) {
                sp33 = 1;
            }
            sp38--;
        }
    }
}
#endif
