#include "common.h"
#include "game/sched.h"
#include "game/frame.h"
#include "game/yoshi.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x4 */ f32 unk4;
} UnkStruct_80208060; /* size = 0x8 */

typedef struct {
    /* 0x0 */ u8 unk0[2];
} UnkStruct_802081A8; /* size = 0x2 */

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
} UnkStruct_80210E90_18;

typedef struct {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} UnkStruct_80210E90;

/* Per-frame buffer, double-buffered by D_8035805C. */
void func_801E74E8(u8);

/* .bss, 0x80210E90-0x80215440 (tools/bss_c.py) */
UnkStruct_80210E90 *PTR32 D_80210E90[0x14];
u8 *PTR32 D_80210EE0[0x13][2];
Gfx *PTR32 D_80210F78[0x13][4];
Mtx D_802110A8[0x13];
Mtx D_80211568[0x13];
Mtx D_80211A28;
s16 D_80211A68;
s16 D_80211A6A;
f32 D_80211A70[0x14];
u8 D_80211AC0[0x13][0x300];
u8 D_802153C0[0x14];
f32 D_802153D4;
f32 D_802153D8;
f32 D_802153DC;
f32 D_802153E0;
u16 D_802153E4;
u16 D_802153E6;
#ifndef VERSION_US_V10
s32 D_802153E8;
#endif
s32 D_802153EC;
u32 D_802153F0[0x14];

extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern char *D_80208040;
extern u16 *D_80208044;
extern u8 D_8020804C[];
extern UnkStruct_80208060 D_80208060[];
extern u8 D_802080F8[];
extern u8 D_8020810C[];
extern f32 D_80208120[];
extern s16 D_8020816C[];
extern u8 D_80208194[];
extern UnkStruct_802081A8 D_802081A8;
extern s32 D_802081AC;
extern s16 D_802081B0;
extern s8 D_802081B4;
extern UnkStruct_80210E90 *PTR32 D_80210E90[];
extern u8 *PTR32 D_80210EE0[][2];
extern Gfx *PTR32 D_80210F78[][4];
extern Mtx D_802110A8[];
extern Mtx D_80211568[];
extern Mtx D_80211A28;
extern s16 D_80211A68;
extern s16 D_80211A6A;
extern f32 D_80211A70[];
extern u8 D_80211AC0[][0x300];
extern u8 D_802153C0[];
extern f32 D_802153D4;
extern f32 D_802153D8;
extern f32 D_802153DC;
extern f32 D_802153E0;
extern u16 D_802153E4;
extern u16 D_802153E6;
extern s32 D_802153E8;
extern s32 D_802153EC;
extern u32 D_802153F0[];
extern f32 D_802FDAC0[];
extern OSMesgQueue D_80315180;
extern FrameBuf D_803156F8[];
extern s32 D_80358078;
extern u16 D_8035807C;
extern s32 D_80358080;
extern s32 D_80358084;
extern u64 D_80364A88;
extern s8 D_80370C2C;
extern s8 D_80370C2E;

#ifdef VERSION_EU
extern u8 D_80366F70_eu; /* the language: 0 English, 1 German, 2 French */

/* .data */
u16 *D_80208044 = NULL;
#else
extern u16 D_80304954[];
/* .data, 0x80208040-0x802081C0 (tools/data_c.py) */
char *D_80208040 = "SELECT VEHICLE!";
u16 *D_80208044 = D_80304954;
#endif
s32 D_80208048 = -0x10000;
u8 D_8020804C[0x14] = { 0, 1, 0, 0, 1, 0, 1 };
UnkStruct_80208060 D_80208060[0x13] = {
    { 0, 0.0f },
    { 0, 0.0f },
    { 1, 0.5f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 1, 0.5f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 0, 0.0f },
    { 1, 0.5f },
};
u8 D_802080F8[0x14] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 6, 1, 1, 0, 1, 1, 1, 1, 1, 1 };
#ifdef VERSION_EU
u8 D_8020810C[0x14] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 14, 4, 4, 4, 4, 4, 4, 4, 4 };
#else
u8 D_8020810C[0x14] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 12, 3, 3, 3, 3, 3, 3, 3, 3 };
#endif
f32 D_80208120[0x13] = {
    220.0f, 400.0f, 800.0f, 320.0f, 400.0f, 460.0f, 400.0f, 400.0f, 320.0f, 550.0f, 320.0f, 320.0f,
    500.0f, 320.0f, 320.0f, 320.0f, 320.0f, 320.0f, 320.0f,
};
s16 D_8020816C[0x14] = {
    -75, -75, -250, -50, -80, -93, -75, -75, -53, -180, -50, -75, -75, -50, -65, -50, -65, -75,
    -75,
};
u8 D_80208194[0x14] = { 0, 54, 80, 140, 11, 5, 0, 32, 206, 2, 148, 114, 0, 206, 123, 206, 80, 114, 114 };
UnkStruct_802081A8 D_802081A8 = { { 29, 208 } };
s32 D_802081AC = 1;
s16 D_802081B0 = 0;
s8 D_802081B4 = 1;

s32 func_801E7000(void) {
    s32 spA4;
    s32 spA0;
    s32 sp9C;
    LevelInfo *sp98;
    u16 sp96;
    s32 sp90;
    s32 sp8C;
    s32 sp40[0x13];

#ifdef VERSION_US_V10
    sp9C = 0;
    sp98 = &D_802E8F94[D_802E8BDC];
    D_80211A68 = 0;
    sp96 = 0xE73C;
    D_80211A6A = 0;
#else
    D_80211A68 = 0;
    sp9C = 0;
    sp98 = &D_802E8F94[D_802E8BDC];
    D_80211A6A = 0;
    sp96 = 0xE73C;
    D_802153E8 = D_803643D4;
#endif
    func_8029DEA0();
    for (spA4 = 0; spA4 < 0x13; spA4++) {
        sp40[spA4] = -1;
        if ((D_80364AF0[D_80364AEA].unk10 & (1 << spA4) & sp98->unk2C) &&
            (D_80364AE8 == D_80364AEA || D_80364EF0[D_80364AEA][D_802E8C44[spA4]] != 0)) {
            func_80202100(spA4, &D_80210E90[D_80211A6A], D_80210EE0[D_80211A6A], D_80210F78[D_80211A6A]);
            D_80211A70[spA4] = sp9C;
            sp9C += 380.0;
            guTranslate(&D_802110A8[D_80211A6A], D_80211A70[spA4], D_8020816C[spA4], 0.0f);
            guScale(&D_80211568[D_80211A6A], D_802FDAC0[spA4], D_802FDAC0[spA4], D_802FDAC0[spA4]);
            func_80202270(D_80210E90[D_80211A6A], D_80210EE0[D_80211A6A], D_80211AC0[D_80211A6A]);
            func_802022EC(D_80211AC0[D_80211A6A], D_802080F8[spA4], D_8020804C[spA4], D_80208060[spA4].unk0,
                          D_80208060[spA4].unk4, D_8020810C[spA4], 0);
            func_80202380(spA4);
            D_802153C0[D_80211A6A] = spA4;
            sp40[spA4] = D_80211A6A;
            D_80211A6A++;
        }
    }
    guRotate(&D_80211A28, 20.0f, 1.0f, 0.0f, 0.0f);
    sp90 = (D_80211A6A - 1) / 2;
    sp8C = -1;
    switch (D_80364A90) {
        case 0x80:
            sp8C = sp40[D_80364AF0[D_80364AE8].unk92[D_802E8BDC]];
            break;
        case 4:
        case 0x100:
        case 0x08000000:
            sp8C = sp40[D_803643D4];
            break;
        case 0x4000:
            break;
        default:
#ifdef VERSION_US_V10
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "digger_loop.c", 0x87);
#else
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "digger_loop.c", LINE_EU(0x8B, 0x8A));
#endif
            break;
    }
    func_8029A7E4("default %d auto %d\n", sp90, sp8C);
    func_801E74E8((sp8C == -1) ? sp90 : sp8C);
    D_802153D8 = D_802153D4;
    D_802153E0 = D_802153DC * 4.0;
    D_802153E4 = D_802153E6 = 0;
    return D_80211A6A > 1;
}

void func_801E74E8(u8 arg0) {
    D_80211A68 = arg0;
    D_803643D4 = D_802153C0[arg0];
    func_80260A10();
    func_80260650(D_80367738, D_80208194[D_802153C0[arg0]], 0);
    D_802153D4 = D_80211A70[D_802153C0[arg0]];
    D_802153DC = D_80208120[D_802153C0[arg0]];
}

void func_801E7598(void) {
    UnkStruct_802081A8 sp14C;
    s32 sp148;
    s32 sp144;
    FrameBuf *sp140;
    Gfx *gfx;

    sp144 = 0;
    sp14C = D_802081A8;
    sp140 = &D_803156F8[D_8035805C ^ 1];
    gfx = sp140->dl;
    func_8028A3E4();
    D_80358080 = 0;
    D_80358084 = 0;
    func_802A5720();
    func_8029E0AC();
    func_8028A470();
    if (D_80358060 >= 11 && func_802753C0() == 0) {
        if ((D_80370C28 & 0x9000) && !(D_80370C2A & 0x9000)) {
            func_80275390(0x2000);
            func_80260650(D_80367738, 0x1E, 0);
        } else if ((D_80370C28 & 0x4000) && !(D_80370C2A & 0x4000)) {
            func_80260650(D_80367738, 0xDE, 0);
#ifndef VERSION_US_V10
            D_803643D4 = D_802153E8;
#endif
            switch (D_80364A88) {
                case 0x80:
                case 0x08000000:
                    D_80364A98 = D_80364A88;
                    break;
                default:
                    D_80364A98 = 0x4000;
                    break;
            }
        }
    }
    if (D_80370C2C < -10 && func_802753C0() == 0) {
        if (D_80370C2E < -10) {
            if (D_80211A68 > 0) {
                D_802153E4 -= D_80370C2C;
            }
        } else {
            D_802153E4 = 750;
        }
    }
    if ((D_80370C28 & 0x200) && !(D_80370C2A & 0x200) && func_802753C0() == 0) {
        D_802153E4 = 750;
    }
    if (D_80370C2C > 10 && func_802753C0() == 0) {
        if (D_80370C2E > 10) {
            if (D_80211A68 < D_80211A6A - 1) {
                D_802153E6 += D_80370C2C;
            }
        } else {
            D_802153E6 = 750;
        }
    }
    if ((D_80370C28 & 0x100) && !(D_80370C2A & 0x100) && func_802753C0() == 0) {
        D_802153E6 = 750;
    }
    if (D_8035805C == 0 && D_802153E4 >= 750) {
        func_80260650(D_80367738, sp14C.unk0[D_80211A68 == 0], 0);
        if (D_80211A68 != 0) {
            func_801E74E8(D_80211A68 - 1);
        }
        D_802153E4 -= 750;
    }
    if (D_8035805C == 0 && D_802153E6 >= 750) {
        func_80260650(D_80367738, sp14C.unk0[D_80211A68 + 1 == D_80211A6A], 0);
        if (D_80211A68 + 1 != D_80211A6A) {
            func_801E74E8(D_80211A68 + 1);
        }
        D_802153E6 -= 750;
    }
    func_80284E54(D_803156F8[D_8035805C].dl, D_80358078, 1, 1, 0x4D2, 0);
    D_8035805C ^= 1;
    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(sp140));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000038);
    gSPDisplayList(gfx++, D_01000010);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gDPSetDepthImage(gfx++, D_80358058);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358058);
    gDPSetFillColor(gfx++, 0xFFFCFFFC);
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    func_80259450();
    gfx = func_80200BE0(gfx, sp140, &D_80358078);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
#ifdef VERSION_EU
    func_80259CCC(sp140,
                  D_80366F70_eu == 0   ? "SELECT VEHICLE!"
                  : D_80366F70_eu == 1 ? "WAEHLE FAHRZEUG!"
                                          : NULL,
                  D_80208044, 0, 0xA0, 0x50, 0x18, 0x15, 0x15, 1, 0, 0, 0, 0xA0);
    if (D_803156C0 % 20 * 50 / 60 < 13) {
#else
    func_80259CCC(sp140, D_80208040, D_80208044, 0, 0xA0, 0x50, 0x18, 0x15, 0x15, 1, 0, 0, 0, 0xA0);
    if (D_803156C0 % 20 * 60 / 60 < 16) {
#endif
        D_802081B0 += D_802081B4 * 30;
        if (D_802081B0 >= 0x100) {
            D_802081B0 -= 60;
            D_802081B4 = -D_802081B4;
        }
        if (D_802081B0 < 0) {
            D_802081B0 += 60;
            D_802081B4 = -D_802081B4;
        }
        func_80259DC8(sp140,
#ifdef VERSION_EU
                      D_80366F70_eu == 0   ? "SELECT VEHICLE!"
                      : D_80366F70_eu == 1 ? "WAEHLE FAHRZEUG!"
                                              : NULL,
#else
                      D_80208040,
#endif
                      D_80208044, 0, 0xA0, 0x54, 0x15, 0x15, 0x15, 1, 0xFF, 0xFF - D_802081B0, 0,
                      0xFF, 0xFF, D_802081B0, 0, 0xFF);
    }
    func_80259C24(&gfx, sp140);
    if (D_80358060 < 2) {
        guPerspective(&sp140->mtx[73], &D_8035807C, 45.0f, 4.0f / 3.0f, 40.0f, 4000.0f, 1.0f);
    }
#ifdef VERSION_EU
    D_802153D8 += (D_802153D4 - D_802153D8) * 0.1 * 60.0 / 50.0;
    D_802153E0 += (D_802153DC - D_802153E0) * 0.1 * 60.0 / 50.0;
#else
    D_802153D8 += (D_802153D4 - D_802153D8) * 0.1 * 60.0 / 60.0;
    D_802153E0 += (D_802153DC - D_802153E0) * 0.1 * 60.0 / 60.0;
#endif
    guLookAtReflect(&sp140->mtx[5], &sp140->lookAt, D_802153D8, 1.0f, D_802153E0, D_802153D8, 0.0f, 0.0f, 0.0f,
                    1.0f, 0.0f);
    gSPPerspNormalize(gfx++, D_8035807C);
    gSPLookAt(gfx++, &sp140->lookAt);
    gSPMatrix(gfx++, &sp140->mtx[73], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &sp140->mtx[5], G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gDPSetEnvColor(gfx++, 0, 0, 0, 0xFF);
    for (sp148 = 0; sp148 < D_80211A6A; sp148++) {
        u8 spEF;
        u8 *spE8;
        s32 spE4;
        u8 *spE0;

        spEF = D_8035805C & (sp148 == D_80211A68);
        if (((D_802153D8 - D_80211A70[D_802153C0[sp148]] > 0.0f) ? D_802153D8 - D_80211A70[D_802153C0[sp148]]
                                                                  : -(D_802153D8 - D_80211A70[D_802153C0[sp148]])) <
            570.0) {
            spE8 = D_80210E90[sp148]->unk14 + (u8 *)D_80210E90[sp148];
            if (D_80211A68 == sp148) {
                spE4 = ((UnkStruct_80210E90_18 *)(D_80210E90[sp148]->unk18 + (u8 *)D_80210E90[sp148]))->unk4;
                spE0 = D_80210EE0[sp148][spEF] + spE4;
                func_802021FC(D_80211AC0[sp148], D_80210EE0[sp148][D_8035805C], D_80210EE0[sp148][D_8035805C ^ 1]);
                guRotate((Mtx *)spE0, (D_802153F0[sp148] += 4) % 360, 0.0f, 1.0f, 0.0f);
                osWritebackDCache(spE0, 0x40);
                func_802025D0(D_802153C0[sp148], (f32)((D_802153F0[sp148] + 180) % 360) * 11.37778);
            }
            gSPSegment(gfx++, 6, spE8);
            gSPSegment(gfx++, 7, D_80210EE0[sp148][spEF]);
            gSPMatrix(gfx++, &D_80211A28, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            gSPMatrix(gfx++, &D_802110A8[sp148], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
            gSPMatrix(gfx++, &D_80211568[sp148], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
            gSPDisplayList(gfx++, D_80210F78[sp148][D_8035805C]);
            gSPMatrix(gfx++, &D_80211A28, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            gSPMatrix(gfx++, &D_802110A8[sp148], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
            gSPMatrix(gfx++, &D_80211568[sp148], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
            gSPDisplayList(gfx++, D_80210F78[sp148][D_8035805C + 2]);
        }
    }
    D_802153EC += D_802081AC;
    if (D_802081AC < 0) {
        D_802153EC += D_802081AC * 2;
    }
    if (D_802153EC < 0 || D_802153EC >= 16) {
        D_802153EC -= D_802081AC * 2;
        D_802081AC = -D_802081AC;
    }
    if (D_80211A68 + 1 != D_80211A6A) {
        ColorPair *spB4;

        spB4 = &D_802F47B0[19];
        sp144 = func_80276130(sp140, 2, sp144, D_802153EC + 264, 32, D_802153EC / 4 + 12, 16, D_802F47B0[19].r0,
                              D_802F47B0[19].g0, D_802F47B0[19].b0, D_802F47B0[19].a0, D_802F47B0[19].r1,
                              D_802F47B0[19].g1, D_802F47B0[19].b1, D_802F47B0[19].a1, D_802F47B0[19].r0,
                              D_802F47B0[19].g0, D_802F47B0[19].b0, D_802F47B0[19].a0, D_802F47B0[19].r1,
                              D_802F47B0[19].g1, D_802F47B0[19].b1, D_802F47B0[19].a1);
        sp144 = func_80276080(sp140, 2, sp144, D_802153EC + 268, 35, D_802153EC / 4 + 12, 16, 0, 0, 0, 0xA0);
        gfx = func_80275DA4(gfx, 0);
        gSPVertex(gfx++, sp140->vtx, 8, 0);
        gSP1Triangle(gfx++, 4, 5, 6, 0);
        gSP1Triangle(gfx++, 4, 6, 7, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
    }
    if (D_80211A68 > 0) {
        ColorPair *sp9C;

        sp9C = &D_802F47B0[19];
        sp144 = func_80276130(sp140, 3, sp144, 52 - D_802153EC, 32, D_802153EC / 4 + 12, 16, D_802F47B0[19].r0,
                              D_802F47B0[19].g0, D_802F47B0[19].b0, D_802F47B0[19].a0, D_802F47B0[19].r1,
                              D_802F47B0[19].g1, D_802F47B0[19].b1, D_802F47B0[19].a1, D_802F47B0[19].r0,
                              D_802F47B0[19].g0, D_802F47B0[19].b0, D_802F47B0[19].a0, D_802F47B0[19].r1,
                              D_802F47B0[19].g1, D_802F47B0[19].b1, D_802F47B0[19].a1);
        sp144 = func_80276080(sp140, 3, sp144, 48 - D_802153EC, 35, D_802153EC / 4 + 12, 16, 0, 0, 0, 0xA0);
        gfx = func_80275DA4(gfx, 0);
        gSPVertex(gfx++, &sp140->vtx[sp144 - 8], 8, 0);
        gSP1Triangle(gfx++, 4, 5, 6, 0);
        gSP1Triangle(gfx++, 4, 6, 7, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
    }
    gfx = func_8026BBD0(gfx, &D_803156F8[D_8035805C], &D_80358078);
    gfx = func_80274BF0(sp140, gfx);
    gDPFullSync(gfx++);
    gSPEndDisplayList(gfx++);
    D_80358078 = gfx - sp140->dl;
    for (sp148 = 0; sp148 < D_80358080; sp148++) {
        osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    }
    for (sp148 = 0; sp148 < D_80358080 - D_80358084; sp148++) {
        func_802A57AC();
    }
}
