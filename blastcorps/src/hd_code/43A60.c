#include "common.h"
#include "game/camera.h"
#include "game/game.h"
#include "functions.h"

/* The older gbi.h this game used has no MAX(1, ...) in the 4b load-block DXT. */
#undef TXL2WORDS_4b
#define TXL2WORDS_4b(txls) ((txls) / 16)

typedef struct UnkStruct_8036EC30 {
    /* 0x00 */ s16 *unk0;
    /* 0x04 */ s16 *unk4;
    /* 0x08 */ s16 *unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ u8 unk16;
    /* 0x17 */ u8 unk17;
    /* 0x18 */ s8 unk18;
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 unk1B;
    /* 0x1C */ u8 unk1C;
    /* 0x1D */ u8 unk1D;
    /* 0x1E */ u8 unk1E;
    /* 0x1F */ u8 unk1F;
    /* 0x20 */ u8 unk20;
    /* 0x21 */ u8 unk21;
    /* 0x22 */ u8 unk22;
} UnkStruct_8036EC30;

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ u8 unk16;
    /* 0x17 */ u8 unk17;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
} UnkStruct_8036EC38; /* size = 0x20 */

typedef struct {
    /* 0x0 */ u8 unk0;
#ifdef TARGET_PC
    /* 0x4 */ DlAddr unk4;  /* (func_8028A0A0's: in the port a pointer's value) */
#else
    /* 0x4 */ s32 unk4;
#endif
} UnkStruct_8028A1D0; /* size = 0x8 on the N64 */

extern UnkStruct_8036EC30 *D_802C4A20[]; /* hd_code 800DC's table, to -1 */


void func_80289EF4(Gfx **);
DlAddr func_8028A0A0(s16);
void func_8028A1D0(UnkStruct_8028A1D0 *, s32);

/* .bss, 0x8036EC30-0x80370BC0 (tools/bss_c.py) */
UnkStruct_8036EC30 *D_8036EC30;
UnkStruct_8036EC38 D_8036EC38[50];
Mtx D_8036F278[2][50];
s32 D_80370B78;
s32 D_80370B7C;
s32 D_80370B80;
u8 D_80370B84;
s32 D_80370B88;
u8 D_80370B8C;
u8 D_80370B8D;
u8 *D_80370B90;
s16 D_80370B98[0xc];
s32 D_80370BB0;
s32 D_80370BB4;

/* .data, 0x802FDA80-0x802FDB10 (tools/data_c.py) */
Vtx D_802FDA80[4] = {
    { { { 0 }, 0, { 0 }, { 255, 255, 255, 255 } } },
    { { { 0 }, 0, { 0 }, { 255, 255, 255, 255 } } },
    { { { 0 }, 0, { 0 }, { 255, 255, 255, 255 } } },
    { { { 0 }, 0, { 0 }, { 255, 255, 255, 255 } } },
};
f32 D_802FDAC0[0x14] = {
    2.0f, 0.8f, 1.4f, 0.9f, 0.6f, 1.2f, 0.075f, 0.4f, 0.9f, 1.0f, 0.85f, 1.0f, 1.2f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
};

void func_80288220(void) {
    s32 i;

    for (i = 0; i < 50; i++) {
        D_8036EC38[i].unk0 = 0;
    }
    D_80370B8C = 0;
    D_80370B8D = 0;
    D_80370B90 = (u8 *)D_80358070;
    D_80358070 += 0x1400;
}

s32 func_80288284(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    s32 spC;
    s32 sp8;

    if ((D_80370B8C == 0) && (D_80370B8D == 0)) {
        D_80370B78 = arg1;
        D_80370B7C = arg2;
        D_80370B80 = arg3;
        D_80370B88 = arg4;
        D_8036EC30 = D_802C4A20[arg0];
        D_80370B84 = 0;
        D_802FDA80[0].v.ob[0] = -D_8036EC30->unkC;
        D_802FDA80[0].v.ob[1] = D_8036EC30->unkC;
        D_802FDA80[0].v.tc[0] = 0;
        D_802FDA80[0].v.tc[1] = 0;
        D_802FDA80[0].v.cn[0] = D_8036EC30->unk1D;
        D_802FDA80[0].v.cn[1] = D_8036EC30->unk1E;
        D_802FDA80[0].v.cn[2] = D_8036EC30->unk1F;
        D_802FDA80[0].v.cn[3] = D_8036EC30->unk20;
        D_802FDA80[1].v.ob[0] = D_8036EC30->unkC;
        D_802FDA80[1].v.ob[1] = D_8036EC30->unkC;
        D_802FDA80[1].v.tc[0] = 0;
        D_802FDA80[1].v.tc[1] = D_8036EC30->unk17 << 5;
        D_802FDA80[1].v.cn[0] = D_8036EC30->unk1D;
        D_802FDA80[1].v.cn[1] = D_8036EC30->unk1E;
        D_802FDA80[1].v.cn[2] = D_8036EC30->unk1F;
        D_802FDA80[1].v.cn[3] = D_8036EC30->unk20;
        D_802FDA80[2].v.ob[0] = D_8036EC30->unkC;
        D_802FDA80[2].v.ob[1] = -D_8036EC30->unkC;
        D_802FDA80[2].v.tc[0] = D_8036EC30->unk16 << 5;
        D_802FDA80[2].v.tc[1] = D_8036EC30->unk17 << 5;
        D_802FDA80[2].v.cn[0] = D_8036EC30->unk1D;
        D_802FDA80[2].v.cn[1] = D_8036EC30->unk1E;
        D_802FDA80[2].v.cn[2] = D_8036EC30->unk1F;
        D_802FDA80[2].v.cn[3] = D_8036EC30->unk20;
        D_802FDA80[3].v.ob[0] = -D_8036EC30->unkC;
        D_802FDA80[3].v.ob[1] = -D_8036EC30->unkC;
        D_802FDA80[3].v.tc[0] = 0;
        D_802FDA80[3].v.tc[1] = D_8036EC30->unk17 << 5;
        D_802FDA80[3].v.cn[0] = D_8036EC30->unk1D;
        D_802FDA80[3].v.cn[1] = D_8036EC30->unk1E;
        D_802FDA80[3].v.cn[2] = D_8036EC30->unk1F;
        D_802FDA80[3].v.cn[3] = D_8036EC30->unk20;
        switch (D_8036EC30->unk22) {
            case 0:
                sp8 = 4;
                break;
            case 1:
                sp8 = 8;
                break;
            case 2:
                sp8 = 16;
                break;
            case 3:
                sp8 = 32;
                break;
        }
        D_80370BB4 = (D_8036EC30->unk16 * D_8036EC30->unk17 * sp8) / 8;
        D_80370BB0 = 0;
        D_80370B8C = 1;
        return 1;
    }
    return 0;
}

void func_802886A0(void) {
    s32 sp9C;
    s32 sp98;
    u8 sp97;
    u8 sp96;
    s16 sp94;
    f32 sp54[4][4];
    f32 sp50;
    f32 sp4C;
    f32 sp48;
    f32 sp44;
    f32 sp40;
    f32 sp3C;
    s32 sp38;
    s16 sp36;

    sp9C = 0;
    sp98 = 0;
    sp97 = 0;
    if (D_80370B8C == 0) {
        if (D_80370B8D != 0) {
            D_80370B8D--;
        }
    } else {
        D_80370B8C = 0;
        if (D_80370B84 < D_8036EC30->unk1A) {
            while (!sp97 && sp9C < D_8036EC30->unk19) {
                sp96 = 0;
                while (!sp96 && sp98 < 50) {
                    if (D_8036EC38[sp98].unk0 == 0) {
                        sp96 = 1;
                    } else {
                        sp98++;
                    }
                }
                if (sp96) {
                    D_8036EC38[sp98].unk0 = 1;
                    D_8036EC38[sp98].unk4 = D_80370B78;
                    D_8036EC38[sp98].unk8 = D_80370B7C;
                    D_8036EC38[sp98].unkC = D_80370B80;
                    sp38 = func_8026A828(0, D_8036EC30->unk1B);
                    guRotateF(sp54, sp38, 0.0f, 0.0f, 1.0f);
                    guMtxXFMF(sp54, 0.0f,
                              (f32)(func_8026A828(-D_8036EC30->unk12, D_8036EC30->unk12) + D_8036EC30->unk10) / 32.0f,
                              0.0f, &sp50, &sp4C, &sp48);
                    sp38 = func_8026A828(0, 359);
                    guRotateF(sp54, sp38, 0.0f, 1.0f, 0.0f);
                    guMtxXFMF(sp54, sp50, sp4C, sp48, &sp44, &sp40, &sp3C);
                    D_8036EC38[sp98].unk10 = sp44 * 32.0f;
                    D_8036EC38[sp98].unk12 = sp40 * 32.0f;
                    D_8036EC38[sp98].unk14 = sp3C * 32.0f;
                    D_8036EC38[sp98].unk16 = 0;
                    D_8036EC38[sp98].unk18 = 0;
                    D_8036EC38[sp98].unk1A = 0;
                    D_8036EC38[sp98].unk1C = 0;
                    D_8036EC38[sp98].unk17 =
                        func_8026A828(-D_8036EC30->unk15, D_8036EC30->unk15) + D_8036EC30->unk14;
                    sp98++;
                } else {
                    sp97 = 1;
                }
                sp9C++;
            }
            D_80370B84++;
        }
        for (sp9C = 0; sp9C < 50; sp9C++) {
            if (D_8036EC38[sp9C].unk0 != 0) {
                D_80370B8C = 1;
                switch (D_8036EC38[sp9C].unk16) {
                    case 0:
                        sp36 = *D_8036EC30->unk0;
                        if (D_8036EC38[sp9C].unk18 == sp36) {
                            D_8036EC38[sp9C].unk16 = 1;
                            D_8036EC38[sp9C].unk18 = 0;
                        }
                        break;
                    case 1:
                        sp36 = *D_8036EC30->unk4;
                        if (D_8036EC38[sp9C].unk18 == sp36) {
                            D_8036EC38[sp9C].unk18 = 0;
                        }
                        break;
                    case 3:
                        sp36 = *D_8036EC30->unk8;
                        if (D_8036EC38[sp9C].unk18 == sp36) {
                            D_8036EC38[sp9C].unk0 = 0;
                        }
                        break;
                }
                if (D_8036EC38[sp9C].unk1C == D_8036EC38[sp9C].unk17) {
                    D_8036EC38[sp9C].unk16 = 3;
                    D_8036EC38[sp9C].unk18 = 0;
                }
            }
        }
        for (sp9C = 0; sp9C < 50; sp9C++) {
            if (D_8036EC38[sp9C].unk0 != 0) {
                D_8036EC38[sp9C].unk4 += D_8036EC38[sp9C].unk10;
                D_8036EC38[sp9C].unkC += D_8036EC38[sp9C].unk14;
                sp94 = D_8036EC38[sp9C].unk12 + D_8036EC38[sp9C].unk1A * D_8036EC30->unk18;
                D_8036EC38[sp9C].unk8 += sp94;
                if (D_8036EC38[sp9C].unk8 < D_80370B88) {
                    D_8036EC38[sp9C].unk8 = D_80370B88;
                    D_8036EC38[sp9C].unk12 = -((sp94 * 16) / D_8036EC30->unk1C);
                    D_8036EC38[sp9C].unk1A = 0;
                }
            }
            D_8036EC38[sp9C].unk1A++;
            D_8036EC38[sp9C].unk18++;
            D_8036EC38[sp9C].unk1C++;
        }
        if (D_80370B8C == 0) {
            D_80370B8D = 3;
        }
    }
}

void func_80288DF0(Gfx **arg0, u8 arg1) {
    s32 sp2FC;
    s32 sp2F8;
    Gfx *gfx;
    f32 sp2B4[4][4];
    f32 sp274[4][4];
    s32 sp270;
    s32 sp26C;
    s32 sp268;
    s32 sp264;
    s32 sp260;
    s32 sp25C;
    UnkStruct_8028A1D0 spCC[50];
    s32 spC8;
#ifdef TARGET_PC
    DlAddr spC4;
#else
    s32 spC4;
#endif

    gfx = *arg0;
    spC8 = 0;
    spC4 = -1;
    if (D_80370B8C != 0) {
        func_80289EF4(&gfx);
        for (sp2FC = 0; sp2FC < 50; sp2FC++) {
            if (D_8036EC38[sp2FC].unk0 != 0) {
                switch (D_8036EC38[sp2FC].unk16) {
                    case 0:
                        sp25C = D_8036EC30->unk0[D_8036EC38[sp2FC].unk18];
                        break;
                    case 1:
                        sp25C = D_8036EC30->unk4[D_8036EC38[sp2FC].unk18];
                        break;
                    case 3:
                        sp25C = D_8036EC30->unk8[D_8036EC38[sp2FC].unk18];
                        break;
                }
                spCC[spC8].unk0 = sp2FC;
                spCC[spC8].unk4 = func_8028A0A0(sp25C);
                spC8++;
            }
        }
        func_8028A1D0(spCC, spC8);
        for (sp2F8 = 0; sp2F8 < spC8; sp2F8++) {
            sp2FC = spCC[sp2F8].unk0;
            if (spCC[sp2F8].unk4 != spC4) {
                spC4 = spCC[sp2F8].unk4;
                gDPPipeSync(gfx++);
                switch (D_8036EC30->unk22) {
                    case 0:
                        gDPLoadTextureBlock_4b(gfx++, OS_K0_TO_PHYSICAL(spC4), D_8036EC30->unk21, D_8036EC30->unk16,
                                               D_8036EC30->unk17, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK,
                                               G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                        break;
                    case 1:
                        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(spC4), D_8036EC30->unk21, G_IM_SIZ_8b,
                                            D_8036EC30->unk16, D_8036EC30->unk17, 0, G_TX_CLAMP, G_TX_CLAMP,
                                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                        break;
                    case 2:
                        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(spC4), D_8036EC30->unk21, G_IM_SIZ_16b,
                                            D_8036EC30->unk16, D_8036EC30->unk17, 0, G_TX_CLAMP, G_TX_CLAMP,
                                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                        break;
                    case 3:
                        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(spC4), D_8036EC30->unk21, G_IM_SIZ_32b,
                                            D_8036EC30->unk16, D_8036EC30->unk17, 0, G_TX_CLAMP, G_TX_CLAMP,
                                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                        break;
                }
            }
            sp270 = func_8026A6F0(D_803643F8 >> 11, D_803643FC >> 11, D_80364400 >> 11, D_8036EC38[sp2FC].unk4,
                                  D_8036EC38[sp2FC].unk8, D_8036EC38[sp2FC].unkC);
            if (sp270 == 0) {
                sp270 = 1;
            }
            sp26C = (D_803643FC >> 11) - D_8036EC38[sp2FC].unk8;
            sp26C = sp26C << 16;
            sp264 = sp26C / sp270;
            if (sp264 < 0) {
                sp260 = -sp264;
            } else {
                sp260 = sp264;
            }
            sp268 = func_802AD7D4(sp260);
            if (sp264 > 0) {
                sp268 = -sp268;
            }
            guRotateF(sp2B4, (f32)sp268 / 65536.0 * 360.0, 1.0f, 0.0f, 0.0f);
            guRotateF(sp274, (f32)D_80364452 / 4095.0 * 360.0, 0.0f, 1.0f, 0.0f);
            guMtxCatF(sp2B4, sp274, sp2B4);
            guTranslateF(sp274, D_8036EC38[sp2FC].unk4 / 32.0f, D_8036EC38[sp2FC].unk8 / 32.0f,
                         D_8036EC38[sp2FC].unkC / 32.0f);
            guMtxCatF(sp2B4, sp274, sp2B4);
            guMtxF2L(sp2B4, &D_8036F278[arg1][sp2FC]);
            gSPMatrix(gfx++, osVirtualToPhysical(&D_8036F278[arg1][sp2FC]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            gSPVertex(gfx++, osVirtualToPhysical(D_802FDA80), 4, 0);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
            gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
        }
        *arg0 = gfx;
    }
}

void func_80289EF4(Gfx **arg0) {
    Gfx *gfx = *arg0;

    gDPPipeSync(gfx++);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetRenderMode(gfx++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
    switch (D_8036EC30->unk21) {
        case 0:
            gDPSetCombineMode(gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
            break;
        case 3:
            gDPSetCombineMode(gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
            break;
        case 4:
            gDPSetCombineMode(gfx++, G_CC_MODULATEI, G_CC_MODULATEI);
            break;
    }
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    *arg0 = gfx;
}

DlAddr func_8028A0A0(s16 arg0) {
    u8 found;
    s32 i;
    u8 *sp1C;

    found = 0;
    i = 0;
    while (!found && i < D_80370BB0) {
        if (D_80370B98[i] == arg0) {
            found = 1;
        } else {
            i++;
        }
    }
    if (found) {
        return osVirtualToPhysical(D_80370B90 + i * D_80370BB4);
    }
    sp1C = D_80370B90 + D_80370BB0 * D_80370BB4;
    D_80370B98[D_80370BB0] = arg0;
    D_80370BB0++;
    func_802A1040(arg0, sp1C, 0);
    return osVirtualToPhysical(sp1C);
}

void func_8028A1D0(UnkStruct_8028A1D0 *arg0, s32 arg1) {
    s32 i;
    s32 j;
    s32 h;
    UnkStruct_8028A1D0 tmp;

    for (h = 1; h <= arg1 / 9; h = h * 3 + 1) {
    }
    for (; h > 0; h /= 3) {
        for (i = h; i < arg1; i++) {
            tmp.unk0 = arg0[i].unk0;
            tmp.unk4 = arg0[i].unk4;
            j = i;
            while (j >= h && arg0[j - h].unk4 > tmp.unk4) {
                arg0[j].unk0 = arg0[j - h].unk0;
                arg0[j].unk4 = arg0[j - h].unk4;
                j -= h;
            }
            arg0[j].unk0 = tmp.unk0;
            arg0[j].unk4 = tmp.unk4;
        }
    }
}
