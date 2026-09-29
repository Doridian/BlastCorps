#include "common.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"

typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x12 */ u8 unk12;
    /* 0x13 */ u8 unk13;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u8 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ u8 unk28;
    /* 0x29 */ u8 unk29;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ s16 unk2C;
    /* 0x2E */ s16 unk2E;
    /* 0x30 */ void *unk30;
    /* 0x34 */ void *unk34;
} UnkStruct_8039C550; /* size = 0x38 */

typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s16 unkC[4][3];
    /* 0x24 */ s16 unk24;
    /* 0x26 */ u8 unk26;
    /* 0x27 */ u8 unk27;
} UnkStruct_8039C800; /* size = 0x28 */

typedef struct {
    /* 0x000 */ Gfx unk0[0x28];
    /* 0x140 */ Vtx unk140[0x14];
    /* 0x280 */ s16 unk280;
    /* 0x282 */ u8 unk282;
    /* 0x283 */ u8 unk283;
    /* 0x284 */ s16 unk284;
    /* 0x286 */ s16 unk286;
    /* 0x288 */ s32 unk288;
    /* 0x28C */ s32 unk28C;
} UnkStruct_802FDC08; /* size = 0x290 */

typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} UnkStruct_8039C718; /* size = 0x1C */

typedef struct {
    /* 0x00 */ s16 unk0[11];
} UnkStruct_8028FDA0_Elem; /* size = 0x16 */

typedef struct {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ UnkStruct_8028FDA0_Elem unkA[1];
} UnkStruct_8028FDA0;

extern FrameGame D_02000000;

void func_80291724(s32 arg0);
extern void *func_80260650(SndBank *, s16, void **);
extern void func_802608C8(void *);
extern s32 func_8026A610(s32, s32, s32, s32);
extern s32 func_8029B930(void);
extern s16 func_802A6F6C(void);
extern void func_802CDB70(s16, s16);
extern u8 func_802CDF94(s16);
extern s16 func_802CE3B8(s16);
extern void func_802CE4F0(s32, s32, s32);
extern void func_802CE5BC(s32, s32, s32, s16, s32, s32);
extern void func_802CE65C(s32, s32, s16, s16);
extern void func_802CE880(s32, s32, s32, s32, s32);
extern void func_802CE90C(s32);
extern s32 func_802CE958(s32);
extern void func_802CEA68(s32, s32);
extern void func_802CE9A4(void);
extern void func_802CE9C8(void *, u8, u8);
extern void *func_802A0CC8(s16, s32);
extern s32 func_8026A6F0(s32, s32, s32, s32, s32, s32);
extern void func_802AACD4(u8, s32, s32, s16 *, s16 *);
extern void func_802AAE1C(u8, s16, s16, s32 *, s32 *);
extern s32 func_802CE6F8(s32, s32, s32);

extern u8 D_803A7424;
extern s16 D_8036443C;
extern s16 D_803A7410;
extern s16 D_803A7412;
extern s32 D_803EF6DC;
extern s32 D_803EF6E0;
extern s32 D_803EF6E4;
extern s32 D_803F9320;
extern s32 D_803F9324;
extern u8 D_803F932C;
extern u8 D_803F932D;
extern u8 D_803F932E;
extern u8 D_803ED40C;
extern s32 D_803FB8B0;

/* .bss, 0x8039C550-0x8039C960 (tools/bss_c.py) */
UnkStruct_8039C550 D_8039C550[1];
u8 D_8039C588[0x188];
s32 D_8039C710;
UnkStruct_8039C718 D_8039C718[1];
u8 D_8039C734[4];
u8 D_8039C738[0xC0];
s32 D_8039C7F8;
UnkStruct_8039C800 D_8039C800[1];
u8 D_8039C828[0x118];
u8 D_8039C940;
s32 D_8039C944;
s32 D_8039C948;
s32 D_8039C94C;
u8 D_8039C950;
s32 D_8039C954;
s32 D_8039C958;
s32 D_8039C95C;

/* .data, 0x802FDBE0-0x802FE3C0 (tools/data_c.py) */
s16 D_802FDBE0[0x14] = { 0, 135, 105, 150, 150, 135, 0, 150, 200, 2000, 175, 0, 0, 195, 190, 185, 105 };
extern UnkStruct_802FDC08 D_802FDC08[3];
#ifdef ASSET_INIT
#include "hd_code/4B5E0/D_802FDC08.inc"
#else
UnkStruct_802FDC08 D_802FDC08[3] = {
    {
        { 0 },
        { 0 },
        1164, 64, 32, 1600, 30, 960, 0,
    },
    {
        { 0 },
        { 0 },
        1164, 64, 32, 1600, 30, 960, 0,
    },
    {
        { 0 },
        { 0 },
        1164, 64, 32, 1280, 40, 960, 0,
    },
};
#endif

void func_8028FDA0(s16 *arg0, s16 *arg1) {
    s32 i;

    D_8039C710 = 0;
    D_8039C7F8 = 0;
    D_8039C940 = 0;
    D_8039C944 = 0;
    D_8039C948 = 0;
    D_8039C94C = 0;
    D_8039C954 = 0;
    D_8039C958 = 0;
    D_8039C95C = 0;
    D_8039C950 = 0;
    D_803ED40C = 0;
    func_802CE9A4();
    if (arg0 != arg1) {
        D_8039C710 = *arg0++;
        for (i = 0; i < D_8039C710; i++) {
            D_8039C550[i].x = arg0[0] << 5;
            D_8039C550[i].y = arg0[1] << 5;
            D_8039C550[i].z = arg0[2] << 5;
            D_8039C550[i].unk10 = arg0[3];
            arg0 += 4;
            D_8039C550[i].y = func_802CE6F8(D_8039C550[i].x, D_8039C550[i].z, D_8039C550[i].y);
            D_8039C550[i].unkE = 0;
            D_8039C550[i].unkC = 0;
            D_8039C550[i].unk11 = 0;
            D_8039C550[i].unk12 = 0;
            D_8039C550[i].unk20 = 0;
            D_8039C550[i].unk14 = 0;
            D_8039C550[i].unk18 = 0;
            D_8039C550[i].unk29 = 0;
            D_8039C550[i].unk30 = func_802A0CC8(D_802FDC08[D_8039C550[i].unk10].unk280, 0);
        }
        D_8039C7F8 = *arg0++;
        for (i = 0; i < D_8039C7F8; i++) {
            D_8039C718[i].x = ((UnkStruct_8028FDA0 *) arg0)->x << 5;
            D_8039C718[i].y = ((UnkStruct_8028FDA0 *) arg0)->y << 5;
            D_8039C718[i].z = ((UnkStruct_8028FDA0 *) arg0)->z << 5;
            D_8039C718[i].unk10 = ((UnkStruct_8028FDA0 *) arg0)->unk6;
            D_8039C718[i].unkC = D_8039C718[i].y - D_802FDC08[D_8039C718[i].unk10].unk288;
            D_8039C718[i].unk11 = 0;
            D_8039C718[i].unk14 = D_803FB8B0;
            func_802CE9C8(((UnkStruct_8028FDA0 *) arg0)->unkA, ((UnkStruct_8028FDA0 *) arg0)->unk7,
                          D_8039C718[i].unk10);
            D_8039C718[i].unk18 = D_803FB8B0;
            if (((UnkStruct_8028FDA0 *) arg0)->unk8 != 0) {
                D_8039C800[D_8039C940].x = D_8039C718[i].x;
                D_8039C800[D_8039C940].y = D_8039C718[i].y;
                D_8039C800[D_8039C940].z = D_8039C718[i].z;
                D_8039C800[D_8039C940].unkC[0][1] = D_8039C718[i].y >> 5;
                D_8039C800[D_8039C940].unkC[1][1] = D_8039C718[i].y >> 5;
                D_8039C800[D_8039C940].unkC[2][1] = D_8039C718[i].y >> 5;
                D_8039C800[D_8039C940].unkC[3][1] = D_8039C718[i].y >> 5;
                D_8039C800[D_8039C940].unkC[0][0] = (D_8039C718[i].x >> 5) - 40;
                D_8039C800[D_8039C940].unkC[0][2] = (D_8039C718[i].z >> 5) - 40;
                D_8039C800[D_8039C940].unkC[1][0] = (D_8039C718[i].x >> 5) + 40;
                D_8039C800[D_8039C940].unkC[1][2] = (D_8039C718[i].z >> 5) - 40;
                D_8039C800[D_8039C940].unkC[2][0] = (D_8039C718[i].x >> 5) - 40;
                D_8039C800[D_8039C940].unkC[2][2] = (D_8039C718[i].z >> 5) + 40;
                D_8039C800[D_8039C940].unkC[3][0] = (D_8039C718[i].x >> 5) + 40;
                D_8039C800[D_8039C940].unkC[3][2] = (D_8039C718[i].z >> 5) + 40;
                D_8039C800[D_8039C940].unk26 = 0;
                D_8039C800[D_8039C940].unk27 = i;
                D_8039C940++;
            }
            arg0 = (s16 *) (((UnkStruct_8028FDA0 *) arg0)->unk7 * 0x16 + (u8 *) arg0 + 0xA);
        }
    }
}

void func_802906C0(u8 arg0) {
    s32 i;
    s32 j;
    u8 res;
    s32 dx;
    s32 dy;
    s32 dz;
    s16 saveA;
    s16 saveB;
    u8 changed;
    s32 dist;
    s32 step;
    s32 sp4C;
    s32 sp48;
    s32 sp44;
    u8 sp43;
    u8 found;
    s32 tx;
    s32 tz;
    s32 ty;
    u8 flag;

    for (i = 0; i < D_8039C710; i++) {
        if (D_8039C550[i].unk11 == 0 && D_8039C550[i].unk12 == 0) {
            if (arg0 == 0) {
                D_8039C550[i].unkC = 0;
            }
            if (D_8039C550[i].unkC != 0 && func_802CE958(i) == 0) {
                func_802CE65C(D_8039C550[i].x, D_8039C550[i].z, D_8039C550[i].unkC, D_8039C550[i].unkE);
                D_8039C550[i].x = D_803F9320;
                D_8039C550[i].z = D_803F9324;
                D_8039C550[i].y = func_802CE6F8(D_8039C550[i].x, D_8039C550[i].z, D_8039C550[i].y);
                D_8039C550[i].unk28 = D_803F932C;
                found = 0;
                j = 0;
                while (j < D_8039C7F8 && found == 0) {
                    if (D_8039C550[i].unk10 == D_8039C718[j].unk10 && D_8039C550[i].unk28 == 0 &&
                        D_8039C718[j].unk11 == 0) {
                        dx = (D_8039C550[i].x - D_8039C718[j].x) >> 5;
                        dy = (D_8039C550[i].y - D_8039C718[j].y) >> 5;
                        dz = (D_8039C550[i].z - D_8039C718[j].z) >> 5;
                        dist = sqrtf(dx * dx + dy * dy + dz * dz);
                        if (dist <= D_802FDC08[D_8039C550[i].unk10].unk286) {
                            D_8039C550[i].unk24 =
                                func_8026A610(D_8039C550[i].x, D_8039C550[i].z, D_8039C718[j].x, D_8039C718[j].z);
                            D_8039C550[i].unk1C = D_8039C550[i].y;
                            D_8039C550[i].unk12 = 1;
                            D_8039C550[i].unk13 = j;
                            D_8039C718[j].unk11 = 1;
                            found = 1;
                        }
                    }
                    j++;
                }
            }
            func_802CE4F0(D_8039C550[i].x, D_8039C550[i].y, D_8039C550[i].z);
            res = func_802CDF94(D_802FDC08[D_8039C550[i].unk10].unk284);
            if (res != 0) {
                if (D_803F932D == 0 && D_803F932E != 0) {
                    D_803ED40C = 1;
                }
                flag = D_8036443C != 0 || arg0 == 9;
                if ((D_8039C950 == arg0 && flag != 0) || D_803F932D != 0) {
                    if (D_803F932D == 0) {
                        dx = D_803643E0 - D_8039C944, dy = D_803643E4 - D_8039C948, dz = D_803643E8 - D_8039C94C;
                    } else {
                        dx = D_803EF6DC - D_8039C954, dy = D_803EF6E0 - D_8039C958, dz = D_803EF6E4 - D_8039C95C;
                    }
                    D_8039C550[i].unkC = sqrtf(dx * dx + dy * dy + dz * dz) + 10.0f;
                    if (D_8039C550[i].unkC > D_802FDBE0[arg0]) {
                        D_8039C550[i].unkC = D_802FDBE0[arg0];
                    }
                } else {
                    D_8039C550[i].unkC = 0;
                }
            }
            if (D_8039C550[i].unkC != 0) {
                changed = 0;
                saveA = D_803A7410, saveB = D_803A7412;
                func_802CE5BC(D_8039C550[i].x, D_8039C550[i].y, D_8039C550[i].z,
                              D_802FDC08[D_8039C550[i].unk10].unk284, 200, D_8039C550[i].unk10);
                func_802CDB70(D_802FDC08[D_8039C550[i].unk10].unk284, 0);
                if (saveA != D_803A7410 || saveB != D_803A7412) {
                    changed = 1;
                }
            } else {
                changed = 0;
            }
            sp43 = 0;
            if (D_803A7410 != 0 || D_803A7412 != 0xFFF) {
                if (func_8029B930() < 300) {
                    sp43 = 1;
                } else if (changed != 0) {
                    D_8039C550[i].unkE = func_802CE3B8(D_8039C550[i].unkE);
                } else {
                    D_8039C550[i].unkE = func_802A6F6C();
                }
            }
            if ((sp43 != 0 || arg0 == 0) && D_8039C550[i].unk12 == 0) {
                func_802CE880(i, D_8039C550[i].x, D_8039C550[i].y, D_8039C550[i].z,
                              D_802FDC08[D_8039C550[i].unk10].unk284);
                D_8039C550[i].unkC = 0;
            } else {
                func_802CE90C(i);
            }
            if (D_8039C550[i].unkC > 0) {
                D_8039C550[i].unkC -= 8;
            } else {
                D_8039C550[i].unkC = 0;
            }
        }
        if (D_8039C550[i].unk12 != 0) {
            tx = D_8039C718[D_8039C550[i].unk13].x;
            tz = D_8039C718[D_8039C550[i].unk13].z;
            if (D_8039C550[i].x != tx || D_8039C550[i].z != tz) {
                dist = func_8026A610(D_8039C550[i].x, D_8039C550[i].z, tx, tz);
                if (dist != 0) {
                    dx = tx - D_8039C550[i].x;
                    if (dx > 0) {
                        step = dx;
                    } else {
                        step = -dx;
                    }
                    step <<= 10;
                    step /= dist;
                    step *= D_8039C550[i].unkC;
                    step >>= 10;
                    if (dx < 0) {
                        step = -step;
                    }
                    D_8039C550[i].x += step;
                    dz = tz - D_8039C550[i].z;
                    if (dz > 0) {
                        step = dz;
                    } else {
                        step = -dz;
                    }
                    step <<= 10;
                    step /= dist;
                    step *= D_8039C550[i].unkC;
                    step >>= 10;
                    if (dz < 0) {
                        step = -step;
                    }
                    D_8039C550[i].z += step;
                    dist = func_8026A610(D_8039C550[i].x, D_8039C550[i].z, tx, tz);
                    if (dist >= D_8039C550[i].unk24) {
                        D_8039C550[i].x = tx;
                        D_8039C550[i].z = tz;
                    } else {
                        D_8039C550[i].unk24 = dist;
                    }
                }
            } else {
                if (D_8039C550[i].y == D_8039C550[i].unk1C) {
                    func_80260650(D_80367738, 100, NULL);
                }
                ty = D_8039C718[D_8039C550[i].unk13].unkC;
                sp4C = D_8039C550[i].unk14 * D_8039C550[i].unk18 + (D_8039C550[i].unk18 * -4) * D_8039C550[i].unk18;
                D_8039C550[i].y = D_8039C550[i].unk1C + sp4C;
                if (D_8039C550[i].y < ty) {
                    D_8039C550[i].y = ty;
                    sp48 = D_8039C550[i].unk14 * (D_8039C550[i].unk18 - 1) +
                           ((D_8039C550[i].unk18 - 1) * -4) * (D_8039C550[i].unk18 - 1);
                    sp44 = sp4C - sp48;
                    if (sp44 < 0) {
                        sp44 = -sp44;
                    }
                    if (sp44 < 20) {
                        D_8039C550[i].unk12 = 0;
                        D_8039C550[i].unk11 = 1;
                        func_802CEA68(D_8039C718[D_8039C550[i].unk13].unk14, D_8039C718[D_8039C550[i].unk13].unk18);
                        func_80291724(D_8039C550[i].unk13);
                    } else {
                        D_8039C550[i].unk14 = sp44 >> 1;
                        D_8039C550[i].unk18 = 0;
                        D_8039C550[i].unk1C = ty;
                        func_80260650(D_80367738, 12, NULL);
                    }
                } else {
                    D_8039C550[i].unk18++;
                }
            }
        }
        if (D_8039C550[i].unkC != 0 && D_8039C550[i].unk12 == 0 && D_8039C550[i].unk11 == 0 &&
            D_8039C550[i].unk34 == NULL) {
            func_80260650(D_80367738, 7, &D_8039C550[i].unk34);
        }
        if (D_8039C550[i].unk34 != NULL &&
            (D_8039C550[i].unkC == 0 || D_8039C550[i].unk12 != 0 || D_8039C550[i].unk11 != 0)) {
            func_802608C8(D_8039C550[i].unk34);
        }
    }
    D_8039C944 = D_803643E0;
    D_8039C948 = D_803643E4;
    D_8039C94C = D_803643E8;
    D_8039C950 = arg0;
    D_8039C954 = D_803EF6DC;
    D_8039C958 = D_803EF6E0;
    D_8039C95C = D_803EF6E4;
}

void func_80291724(s32 arg0) {
    u8 found;
    s32 i;

    found = 0;
    i = 0;
    while (!found && i < D_8039C940) {
        if (D_8039C800[i].unk27 == arg0) {
            D_8039C800[i].unk26 = 1;
            found = 1;
        } else {
            i++;
        }
    }
}

void func_802917B0(Gfx **arg0, FrameGame *arg1) {
    Gfx *gfx;
    s32 i;

    gfx = *arg0;
    if (D_8039C710 > 0) {
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
        gDPSetCombineMode(gfx++, G_CC_DECALRGBA, G_CC_DECALRGBA);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    }
    for (i = 0; i < D_8039C710; i++) {
        guTranslate(&arg1->unk2C0[0x21 + i], D_8039C550[i].x / 32.0f, D_8039C550[i].y / 32.0f, D_8039C550[i].z / 32.0f);
        gSPMatrix(gfx++, &D_02000000.unk2C0[0x21 + i], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_8039C550[i].unk30), G_IM_FMT_RGBA, G_IM_SIZ_16b,
                            D_802FDC08[D_8039C550[i].unk10].unk282, D_802FDC08[D_8039C550[i].unk10].unk283, 0,
                            G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPDisplayList(gfx++, osVirtualToPhysical(&D_802FDC08[D_8039C550[i].unk10]));
        gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}

void func_80291ED8(u8 arg0) {
    s32 i;

    for (i = 0; i < D_8039C710; i++) {
        if (D_8039C550[i].unk28 == arg0) {
            D_8039C550[i].unkC = 0;
            D_8039C550[i].unk29 = 1;
            func_802AACD4(arg0, D_8039C550[i].x, D_8039C550[i].z, &D_8039C550[i].unk2A, &D_8039C550[i].unk2C);
        }
    }
}

void func_80291FAC(u8 arg0) {
    s32 i;

    for (i = 0; i < D_8039C710; i++) {
        if (D_8039C550[i].unk29 != 0) {
            func_802AAE1C(arg0, D_8039C550[i].unk2A, D_8039C550[i].unk2C, &D_8039C550[i].x, &D_8039C550[i].z);
            D_8039C550[i].y = func_802CE6F8(D_8039C550[i].x, D_8039C550[i].z, D_8039C550[i].y);
        }
    }
}

void func_80292084(void) {
    s32 i;

    for (i = 0; i < D_8039C710; i++) {
        D_8039C550[i].unk29 = 0;
    }
}

void func_802920DC(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 i;
    s32 dist;

    arg0 >>= 5, arg1 >>= 5, arg2 >>= 5, arg3 >>= 5;
    for (i = 0; i < D_8039C710; i++) {
        if (D_8039C550[i].unk11 == 0 && D_8039C550[i].unk29 == 0) {
            dist = func_8026A6F0(arg0, arg1, arg2, D_8039C550[i].x >> 5, D_8039C550[i].y >> 5, D_8039C550[i].z >> 5);
            if (dist <= (D_802FDC08[D_8039C550[i].unk10].unk284 >> 5) + arg3) {
                D_803A7424 = 1;
            }
        }
    }
}
