#include "common.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/level.h"
#include "game/objects.h"
#include "functions.h"

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ u8 unk4;
    /* 0x8 */ s32 unk8;
} UnkStruct_8036C8D0; /* size = 0xC */


/* .bss, 0x8036C8D0-0x8036CB60 (tools/bss_c.py) */
UnkStruct_8036C8D0 D_8036C8D0[50];
u8 D_8036CB28;
u8 D_8036CB29;
s16 D_8036CB2A;
s16 D_8036CB2C;
u8 D_8036CB2E;
u8 D_8036CB2F;
u8 D_8036CB30;
u8 D_8036CB31;
u8 D_8036CB32;
u8 D_8036CB33;
u8 D_8036CB34;
u8 D_8036CB35;
u8 D_8036CB36;
u8 D_8036CB37;
u8 D_8036CB38;
u8 D_8036CB39;
u8 D_8036CB3A;
u8 D_8036CB3B;
u8 D_8036CB3C;
s16 *D_8036CB40;
u8 D_8036CB44;
s32 D_8036CB48[2];
u8 D_8036CB50;
u8 D_8036CB51;

/* .data, 0x802FAD50-0x802FBED0 (tools/data_c.py) */
extern u8 D_802FAD50[0x1000];
#ifdef ASSET_INIT
#include "hd_code/32E00/D_802FAD50.inc"
#else
u8 D_802FAD50[0x1000] = { 0 };
#endif
extern Vtx D_802FBD50[8];
#ifdef ASSET_INIT
#include "hd_code/32E00/D_802FBD50.inc"
#else
Vtx D_802FBD50[8] = { 0 };
#endif
s16 D_802FBDD0[0xe] = { 2031, 2032, 2033, 2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043 };
s16 D_802FBDEC[0x16] = {
    2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 1927, 1928, 1932,
    1933, 1934, 1935, 1936, 1937,
};
s16 D_802FBE18[0x16] = {
    1375, 1376, 1377, 1378, 1379, 1383, 1627, 1628, 1629, 1630, 1631, 1632, 1633, 1634, 1900, 1901,
    1902, 1903, 1904, 1905, 2028,
};
s16 D_802FBE44[0x1e] = {
    2222, 2223, 2224, 2225, 2226, 2227, 2228, 2229, 2230, 2231, 2232, 2233, 2234, 2235, 2236, 2237,
    2238, 2238, 2238, 2238, 2238, 2238, 2238, 2238, 2238, 2238, 2238, 2238, 2238, 2238,
};
s16 D_802FBE80[0x28] = {
    2273, 2274, 2275, 2276, 2277, 2278, 2279, 2280, 2281, 2282, 2283, 2284, 2285, 2286, 2287, 2288,
    2289, 2290, 2291, 2292, 2291, 2290, 2289, 2288, 2287, 2288, 2289, 2290, 2291, 2292, 2292, 2292,
    2292, 2292,
};


s32 func_80277D34(void);
s32 func_80277E08(void);
void func_80277C20(void);
void func_802778FC(void);
void func_80277AE0(void);
void func_80277B84(void);



extern FrameBuf D_02000000;
extern u8 D_803643D6;
extern u8 D_803643DB;
extern UnkStruct_8036C8D0 D_8036C8D0[50];
extern u8 D_8036CB28;
extern u8 D_8036CB29;
extern s16 D_8036CB2A;
extern s16 D_8036CB2C;
extern u8 D_8036CB2E;
extern u8 D_8036CB2F;
extern u8 D_8036CB30;
extern u8 D_8036CB31;
extern u8 D_8036CB32;
extern u8 D_8036CB33;
extern u8 D_8036CB34;
extern u8 D_8036CB35;
extern u8 D_8036CB36;
extern u8 D_8036CB37;
extern u8 D_8036CB38;
extern u8 D_8036CB39;
extern u8 D_8036CB3A;
extern u8 D_8036CB3B;
extern u8 D_8036CB3C;
extern s16 *D_8036CB40;
extern s32 D_8036CB48[2];
extern u8 D_8036CB50;
extern u8 D_8036CB51;

void func_802775C0(void) {
    D_8036CB34 = 0;
    D_8036CB48[0] = (s32)D_80358070;
    D_80358070 += 0xC80;
    D_8036CB48[1] = (s32)D_80358070;
    D_80358070 += 0xC80;
    D_8036CB28 = 0;
    D_8036CB29 = 0;
}

void func_80277620(s32 arg0) {
    u8 found;
    LevelUnk58 *p;
    s16 z;

    found = FALSE;
    p = D_803BE6FC;
    while (!found && D_8036CB28 != D_8036CB29) {
        if (arg0 - D_8036C8D0[D_8036CB28].unk8 > 1000) {
            if (++D_8036CB28 == 50) {
                D_8036CB28 = 0;
            }
        } else {
            found = TRUE;
        }
    }
    if (D_8036CB2F != 0 && D_8036CB29 + 1 != D_8036CB28 && (D_8036CB29 != 49 || D_8036CB28 != 0)) {
        D_8036C8D0[D_8036CB29].unk0 = func_80277D34();
        D_8036C8D0[D_8036CB29].unk2 = func_80277E08();
        D_8036C8D0[D_8036CB29].unk4 = D_8036CB2E;
        D_8036C8D0[D_8036CB29].unk8 = arg0;
        if (++D_8036CB29 == 50) {
            D_8036CB29 = 0;
        }
    }
    if (D_8036CB2F != 0) {
        func_80277C20();
        func_802778FC();
        func_80277AE0();
        func_80277B84();
    }
    if (arg0 == 50 && D_803643D6 == 0) {
        func_80277EDC(2, 1, 2, func_8026205C(1));
    }
    if (D_803643DB != 0) {
        z = D_803EF6E4 >> 5;
        while (p < D_803BE700) {
            if (p->unk6 == 0 && z > p->unk0) {
                func_80277EDC(p->unk2, p->unk3, p->unk4, p->unk5);
                p->unk6 = 1;
            }
            p++;
        }
    }
    D_8036CB2F = 0;
}

void func_802778FC(void) {
    switch (D_80364456) {
        case 5:
            if (D_8036CB31 >= 16 && D_8036CB30 >= 16) {
                func_80277EDC(3, 1, 1, 0x63);
                D_8036CB28 = 0;
                D_8036CB29 = 0;
                func_8029A7E4("DOING REALLY WELL\n");
            }
            break;
        case 4:
            if (D_8036CB31 >= 41 && D_8036CB30 >= 41) {
                func_80277EDC(4, 1, 1, func_8026205C(4));
                D_8036CB28 = 0;
                D_8036CB29 = 0;
                func_8029A7E4("DOING REALLY WELL\n");
            }
            break;
        case 3:
            if (D_8036CB31 >= 11 && D_8036CB30 >= 11) {
                func_80277EDC(0, 1, 3, 0xB2);
                D_8036CB28 = 0;
                D_8036CB29 = 0;
                func_8029A7E4("DOING REALLY WELL\n");
            }
            break;
        case 9:
            if (D_8036CB31 >= 26 && D_8036CB30 >= 26) {
                func_80277EDC(4, 1, 1, func_8026205C(4));
                D_8036CB28 = 0;
                D_8036CB29 = 0;
                func_8029A7E4("DOING REALLY WELL\n");
            }
            break;
    }
}

void func_80277AE0(void) {
    switch (D_80364456) {
        case 3:
        case 4:
        case 5:
        case 9:
            if (D_8036CB31 >= 6 && D_8036CB30 < 2) {
                func_80277EDC(1, 1, 3, 0xB3);
                D_8036CB28 = 0;
                D_8036CB29 = 0;
                func_8029A7E4("USING WRONG DIGGER\n");
            }
            break;
    }
}

void func_80277B84(void) {
    switch (D_80364456) {
        case 3:
        case 4:
        case 5:
            if (D_8036CB33 >= 31 && D_8036CB30 < 6) {
                func_80277EDC(2, 1, 2, 0x58);
                D_8036CB28 = 0;
                D_8036CB29 = 0;
                func_8029A7E4("USING DIGGER INCORRECTLY\n");
            }
            break;
    }
}

void func_80277C20(void) {
    u8 i;

    i = D_8036CB28;
    D_8036CB30 = 0;
    D_8036CB31 = 0;
    D_8036CB32 = 0;
    D_8036CB33 = 0;
    while (i != D_8036CB29) {
        if (D_8036C8D0[i].unk2 == 0) {
            D_8036CB30++;
        } else {
            D_8036CB32++;
        }
        if (D_8036C8D0[i].unk0 == 0) {
            D_8036CB31++;
        } else {
            D_8036CB33++;
        }
        i++;
        if (i == 50) {
            i = 0;
        }
    }
}

s32 func_80277D34(void) {
    switch (D_8036CB2E) {
        case 5:
            if (D_8036CB2A > 400) {
                return 0;
            }
            return 1;
        case 4:
            if (D_8036CB2A > 400) {
                return 0;
            }
            return 1;
        case 9:
            if (D_8036CB2A > 99) {
                return 0;
            }
            return 1;
        case 3:
            if (D_8036CB2A > 99) {
                return 0;
            }
            return 1;
    }
    return 1;
}

s32 func_80277E08(void) {
    switch (D_8036CB2E) {
        case 5:
            if (D_8036CB2C > 80) {
                return 0;
            }
            return 1;
        case 4:
            if (D_8036CB2C > 80) {
                return 0;
            }
            return 1;
        case 9:
            if (D_8036CB2C > 99) {
                return 0;
            }
            return 1;
        case 3:
            if (D_8036CB2C > 99) {
                return 0;
            }
            return 1;
    }
    return 1;
}

extern u8 D_8036CB44;

void func_80277EDC(u8 arg0, u8 arg1, s32 arg2, u8 arg3) {
    u8 sp27;

    if ((D_80364A90 & 0x200000000400220C) && D_8036CB34 == 0) {
        if (arg3 != 0) {
            func_80260650(D_80367738, arg3, 0);
        }
        D_8036CB34 = 1;
        D_8036CB35 = arg2;
        D_8036CB36 = 0;
        D_8036CB37 = 0;
        D_8036CB39 = 0;
        D_8036CB3A = 0;
        D_8036CB38 = arg1;
        D_8036CB44 = arg0;
        D_8036CB50 = 0;
        switch (arg0) {
            case 0:
                D_8036CB3C = 13;
                D_8036CB40 = D_802FBDD0;
                sp27 = 0;
                D_8036CB3B = 1;
                break;
            case 1:
                sp27 = 1;
                D_8036CB3C = 21;
                D_8036CB40 = D_802FBDEC;
                D_8036CB3B = 1;
                break;
            case 2:
                D_8036CB3C = 21;
                D_8036CB40 = D_802FBE18;
                sp27 = 0;
                D_8036CB3B = 2;
                break;
            case 3:
                sp27 = 1;
                D_8036CB3C = 30;
                D_8036CB40 = D_802FBE44;
                D_8036CB3B = 1;
                break;
            case 4:
                D_8036CB3C = 34;
                D_8036CB40 = D_802FBE80;
                sp27 = 0;
                D_8036CB3B = 1;
                break;
        }
        if (D_80364AA8 != 1) {
            sp27 = 1;
        }
        switch (sp27) {
            case 0:
                D_802FBD50[0].v.ob[0] = 32, D_802FBD50[0].v.ob[1] = 68;
                D_802FBD50[1].v.ob[0] = 78, D_802FBD50[1].v.ob[1] = 68;
                D_802FBD50[2].v.ob[0] = 78, D_802FBD50[2].v.ob[1] = 23;
                D_802FBD50[3].v.ob[0] = 32, D_802FBD50[3].v.ob[1] = 23;
                D_802FBD50[4].v.ob[0] = 26, D_802FBD50[4].v.ob[1] = 73;
                D_802FBD50[5].v.ob[0] = 84, D_802FBD50[5].v.ob[1] = 73;
                D_802FBD50[6].v.ob[0] = 84, D_802FBD50[6].v.ob[1] = 13;
                D_802FBD50[7].v.ob[0] = 26, D_802FBD50[7].v.ob[1] = 13;
                break;
            case 1:
                D_802FBD50[0].v.ob[0] = 238, D_802FBD50[0].v.ob[1] = 222;
                D_802FBD50[1].v.ob[0] = 284, D_802FBD50[1].v.ob[1] = 222;
                D_802FBD50[2].v.ob[0] = 284, D_802FBD50[2].v.ob[1] = 177;
                D_802FBD50[3].v.ob[0] = 238, D_802FBD50[3].v.ob[1] = 177;
                D_802FBD50[4].v.ob[0] = 232, D_802FBD50[4].v.ob[1] = 227;
                D_802FBD50[5].v.ob[0] = 290, D_802FBD50[5].v.ob[1] = 227;
                D_802FBD50[6].v.ob[0] = 290, D_802FBD50[6].v.ob[1] = 167;
                D_802FBD50[7].v.ob[0] = 232, D_802FBD50[7].v.ob[1] = 167;
                break;
        }
    }
}

void func_80278318(void) {
    D_8036CB34 = 0;
}

void func_80278324(Gfx **arg0, s32 arg1, u8 arg2) {
    Gfx *gfx;

    gfx = *arg0;
    if (D_8036CB34 != 0) {
        func_802A1040(D_8036CB40[D_8036CB39], D_8036CB48[arg2], 0);
        switch (D_8036CB38) {
            case 1:
                if (D_8036CB37 == 0) {
                    D_8036CB3A++;
                    if (D_8036CB3A == D_8036CB3B) {
                        D_8036CB3A = 0;
                        D_8036CB39++;
                        if (D_8036CB39 + 1 == D_8036CB3C) {
                            D_8036CB37 = 1;
                            D_8036CB36++;
                        }
                    }
                } else {
                    D_8036CB3A++;
                    if (D_8036CB3A == D_8036CB3B) {
                        D_8036CB3A = 0;
                        D_8036CB39--;
                        if (D_8036CB39 == 0) {
                            D_8036CB37 = 0;
                            D_8036CB36++;
                        }
                    }
                }
                break;
            case 0:
                D_8036CB3A++;
                if (D_8036CB3A == D_8036CB3B) {
                    D_8036CB3A = 0;
                    D_8036CB39++;
                    if (D_8036CB39 == D_8036CB3C) {
                        D_8036CB39 = 0;
                        D_8036CB36++;
                    }
                }
                break;
        }
        if (D_8036CB36 == D_8036CB35) {
            D_8036CB34 = 0;
        }
        if (D_8036CB36 == 0 && D_8036CB39 < 5) {
            if (D_8036CB39 == 0) {
                func_80260650(D_80367738, 0x69, 0);
            }
            D_8036CB51 = 80;
        } else if (D_8036CB50 != 0) {
            D_8036CB51 -= 15;
            D_8036CB50--;
        } else if (func_8026A828(0, 20) == 0) {
            D_8036CB50 = 5;
            D_8036CB51 = 80;
            func_80260650(D_80367738, 0x69, 0);
        } else {
            D_8036CB51 = 0;
        }
        gSPMatrix(gfx++, &D_02000000.mtx[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(gfx++, &D_02000000.mtx[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_2CYCLE);
        gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetCombineLERP(gfx++, NOISE, 0, PRIMITIVE_ALPHA, 0, 0, 0, 0, SHADE, TEXEL1, 0, SHADE, COMBINED, 0, 0, 0,
                          SHADE);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_8036CB51);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8036CB48[arg2]), G_IM_FMT_RGBA, G_IM_SIZ_16b, 40, 40, 0,
                            G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
        gSPVertex(gfx++, OS_K0_TO_PHYSICAL(D_802FBD50), 8, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_802FAD50), G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                            G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
        gSP1Triangle(gfx++, 4, 5, 6, 0);
        gSP1Triangle(gfx++, 4, 6, 7, 0);
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}
