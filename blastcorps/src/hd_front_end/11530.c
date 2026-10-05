#include "common.h"
#include "game/sched.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

#define SQ(x) ((x) * (x))

void func_801ECB18(void);
Gfx *func_801F1568(void);
Gfx *func_801F2000(void);
Gfx *func_801F2428(void);
Gfx *func_801F2E20(void);
Gfx *func_801F3450(Gfx *, u8 *);
void func_801F885C(s32);
Gfx *func_801F9258(Gfx *, u8 *, s32 *);
Gfx *func_801F9820(Gfx *, u8 *, s32 *);
Gfx *func_801F9B84(Gfx *, u8 *, s32 *);
Gfx *func_801FA180(Gfx *, u8 *, f32, s8 *);
Gfx *func_801FA74C(); /* K&R: callers pass the u8 arguments unconverted */
Gfx *func_801FC5B8(u8 *, Gfx *, u8, u8);
void func_801FCF38(Vtx *, f32, f32, f32, u8, u8, f32, u8);
void func_801FD484(f32 *, f32 *, f32 *, f32 *, f32 *, f32);
f32 func_801FD6B8(f32, f32, f32);
void func_801FD748(void);
void func_801FDCA4(Vtx *, s32, s32);
void func_801FDE50(void);
void func_801FDE98(void);
Gfx *func_801FE5D0(Gfx *, u8 *);
u8 func_801FE760(u8);
Gfx *func_8024C404(Gfx *, u8 *, s32 *);
f32 func_802574F0(f32);
f32 func_80257514(f32);
void func_80259450(void);
void func_80259C24(Gfx **, u8 *);
void func_80259DC8(u8 *, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
s32 func_8025B300(char *);
s32 func_8025B498(s16, u16, char *, u16 *);
void func_80260650(SndBank *, s32, s32 *);
void func_802608C8(s32);
void func_80260A10(void);
void func_80261FB0(u8);
u8 func_80264BA4(u8);
s32 func_8026A828(s32, s32);
u8 func_80272C5C(u16 *, s32, s32, s32, s32, f32);
Gfx *func_80272ED8(Gfx *, u8, s16, s16, u8, u8, f32);
Gfx *func_80274868(Gfx *);
Gfx *func_80274AA4(Gfx *);
Gfx *func_80274BF0(u8 *, Gfx *);
void func_80275390(u64);
s32 func_802753C0(void);
void func_80284E54(Gfx *, s32, s32, s32, s32, s32);
void func_8028A3E4(void);
void func_8028A470(void);
f32 func_8028BBF4(s16, s16, s16, s16);
u64 func_80299FE8(s32);
void func_8029A7E4(char *, ...);
void func_802A5720(void);
void func_802A57AC(void);
s32 func_802AD7D4(s32);
void guMtxXFMF(float mf[4][4], float x, float y, float z, float *ox, float *oy, float *oz);
void guRotateF(float mf[4][4], float a, float x, float y, float z);
u32 osVirtualToPhysical(void *);
f32 sqrtf(f32);

extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern u16 D_8020E350[];
extern u16 D_8020E39C[];
extern Lights1 D_8020E3A8[];
extern f32 D_8020E3D8;
extern u8 D_802159F0[];
extern u16 *D_80215A70[];
extern s32 D_80217B6C;
extern Mtx D_80217B70[][4];

/* .bss, 0x8021A840-0x8021AB70 (tools/bss_c.py) */
Vtx D_8021A840[2][4];
s32 D_8021A8C0;
f32 D_8021A8C4;
f32 D_8021A8C8;
f32 D_8021A8CC;
f32 D_8021A8D0;
f32 D_8021A8D4;
f32 D_8021A8D8;
f32 D_8021A8DC;
f32 D_8021A8E0;
f32 D_8021A8E4;
f32 D_8021A8E8;
f32 D_8021A8EC;
u8 D_8021A8F0;
Gfx *D_8021A8F4;
Gfx *D_8021A8F8;
Gfx *D_8021A8FC;
Gfx *D_8021A900;
u8 D_8021A904;
u8 D_8021A905;
u8 D_8021A906;
s8 D_8021A907;
u8 D_8021A908;
u8 D_8021A909;
f32 D_8021A90C;
f32 D_8021A910;
f32 D_8021A914;
f32 D_8021A918;
f32 D_8021A91C;
f32 D_8021A920;
u16 D_8021A924;
u16 D_8021A926;
u8 *D_8021A928[2];
u8 D_8021A930;
f32 D_8021A934;
f32 D_8021A938;
u64 D_8021A940[0x3C];
u8 D_8021AB20;
u8 D_8021AB21;
s32 D_8021AB24;
f32 D_8021AB28;
s16 D_8021AB2C;
u8 D_8021AB2E;
PlayerInfo *D_8021AB30;
LevelInfo *D_8021AB34;
s32 D_8021AB38;
u8 D_8021AB3C[4];
f32 D_8021AB40;
f32 D_8021AB44;
f32 D_8021AB48;
f32 D_8021AB4C;
f32 D_8021AB50;
f32 D_8021AB54;
s32 D_8021AB58;
s32 D_8021AB5C;
f32 D_8021AB60;
f32 D_8021AB64;
Gfx *D_8021AB68;
Gfx *D_8021AB6C;

extern f32 D_802E8C84[];
extern s32 D_802FA264;
extern OSMesgQueue D_80315180;
extern FrameGame D_803156F8[];
extern s32 D_80358078;
extern u16 D_8035807C;
extern s32 D_80358080;
extern s32 D_80358084;
extern s16 D_8036BB1C;
extern s8 D_80370C2C;
extern s8 D_80370C2D;

/* .bss, defined here so the u64 halves share a lui as in the original. */

void func_801F8530(s32 arg0) {
    u8 *sp3C;
    u32 sp38;
    UnkStruct_8020D810 *sp34;
    f32 sp30;
    f32 sp2C;

    D_8021AB30 = &D_80364AF0[D_80364AE8];
    D_8021AB34 = &D_802E8F94[arg0];
    D_8021A91C = func_801FD6B8(90.0f, D_8020D810[arg0].unk14, 180.0f);
    D_8021A920 = func_801FD6B8(90.0f, D_8020D810[arg0].unk10, 180.0f);
    D_8021A8F4 = func_801F1568();
    D_8021A8FC = func_801F2428();
    D_8021A900 = func_801F2000();
    for (sp38 = 0; sp38 < 2; sp38++) {
        D_8021A928[sp38] = D_80358070;
        D_80358070 += 0x17000;
    }
    D_8021A8F8 = func_801F2E20();
    for (sp38 = 0; sp38 < 2; sp38++) {
        sp3C = (u8 *)&D_803156F8[sp38];
        guPerspective((Mtx *)(sp3C + 0x80), &D_8035807C, 45.0f, 4.0f / 3.0f, 100.0f, 2e+04f, 1.0f);
        guTranslate((Mtx *)(sp3C + 0x1280), 0.0f, -50.0f, 0.0f);
    }
    D_80217B6C = 6;
    func_801F885C(arg0);
    D_8021A904 = arg0;
    D_8021A908 = arg0;
    D_8021A926 = 1;
    D_8021A930 = 0;
    D_8021A907 = -1;
    D_8021AB21 = 0;
    D_8021A906 = arg0;
    D_8021AB28 = 0.0f;
    D_8021AB38 = 0;
    D_8021AB2C = 0;
    D_8021AB40 = 0.915f;
    D_8021AB44 = 0.009f;
    D_8021AB48 = 2.61e+04f;
    func_801FDE50();
    D_8021AB20 = func_80272C5C(D_8020E39C, 0, 4, 1, 1, 1.0f);
    for (sp38 = 0; sp38 < 0x3C; sp38++) {
        sp34 = &D_8020D810[sp38];
        sp30 = sp34->unk10;
        sp2C = sp34->unk14;
        func_801FD484(&sp30, &sp2C, &sp34->unk24, &sp34->unk28, &sp34->unk2C, 250.0f);
    }
    D_8021AB2E = 0;
}

void func_801F885C(s32 arg0) {
    u8 sp1F;

    sp1F = func_80264BA4(arg0);
    D_8021A905 = arg0;
    if (sp1F != D_80217B6C) {
        D_8021A924 = 0;
        D_8021A918 = 6.25e+03f;
        if (D_80217B6C != 6) {
            if (D_80217B6C == 3) {
                func_80261FB0(0x13);
            } else if (sp1F == 3) {
                func_80261FB0(0xC);
            }
        }
    }
    if (D_80217B6C != 6) {
        if ((sp1F == 3) && (D_80217B6C == 3)) {
            func_80260650(D_80367738, 0x1D, 0);
        } else {
            func_80260650(D_80367738, 0x3F, 0);
        }
    }
    D_80217B6C = sp1F;
    D_8021AB34 = &D_802E8F94[D_8021A905];
}

extern u16 D_80303700[];
extern u16 D_8030370C[];
extern u16 D_80303720[];
extern u16 D_80303734[];
extern u16 D_80303748[];
extern u16 D_8030375C[];
extern u16 D_8030376C[];
extern u16 D_8030377C[];
extern u16 D_80303794[];
extern u16 D_803037A8[];
extern u16 D_803037B8[];
extern u16 D_803037CC[];
extern u16 D_803037D8[];
extern u16 D_803037EC[];
extern u16 D_803037FC[];
extern u16 D_8030380C[];
extern u16 D_8030381C[];
extern u16 D_8030382C[];
extern u16 D_80303844[];
extern u16 D_80303854[];
extern u16 D_80303864[];
extern u16 D_80303874[];
extern u16 D_80303884[];
extern u16 D_80303890[];
extern u16 D_803038A0[];
extern u16 D_803038B0[];
extern u16 D_803038C0[];
extern u16 D_803038D0[];
extern u16 D_803038E0[];
extern u16 D_803038F8[];
extern u16 D_80303904[];
extern u16 D_80303918[];
extern u16 D_80303930[];
extern u16 D_8030393C[];
extern u16 D_80303950[];
extern u16 D_80303960[];
extern u16 D_80303974[];
extern u16 D_80303980[];
extern u16 D_80303990[];
extern u16 D_803039A8[];
extern u16 D_803039B4[];
extern u16 D_803039BC[];
extern u16 D_803039CC[];
extern u16 D_803039E0[];
extern u16 D_803039F0[];
extern u16 D_803039FC[];
extern u16 D_80303A04[];
extern u16 D_80303A14[];
extern u16 D_80303A24[];
extern u16 D_80303A34[];
extern u16 D_80303A48[];
extern u16 D_80303A58[];
extern u16 D_80303A68[];
extern u16 D_80303A74[];
extern u16 D_80303A8C[];
extern u16 D_80303A98[];
extern u16 D_80303AB0[];
extern u16 D_80303AC0[];
extern u16 D_80303AD4[];
extern u16 D_80303AE8[];
/* .data, 0x8020D810-0x8020E3E0 (tools/data_c.py) */
#ifdef VERSION_EU
UnkStruct_8020D810 D_8020D810[0x3c] = {
    {
        { 100, 1 }, "SIMIAN ACRES", "EDUKATIVA", NULL, NULL, { 0 }, 0.0f, 0.0f,
        { -1 }, { 28, 19, 3, 55, 50, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "ANGEL CITY", "METROPOLIKO", NULL, NULL, { 0 }, -20.0f, -120.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "OUTLAND FARM", "ALTERNATIVA", NULL, NULL, { 0 }, 20.0f, 60.0f,
        { 37, 27, -1 }, { 5, 13, 9, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BLACKRIDGE WORKS", "GROBSTADT", NULL, NULL, { 0 }, -20.0f, -20.0f,
        { 20, 6, -1 }, { 29, 53, 15, 10, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "GLORY CROSSING", "KATENASCHLUCHT", NULL, NULL, { 0 }, 70.0f, 180.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SHUTTLE GULLY", "RAMMSTERDAMM", NULL, NULL, { 0 }, 50.0f, 20.0f,
        { 41, -1 }, { 4, 33, 2, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SALVAGE WHARF", "HOFFNUNGSKAI", NULL, NULL, { 0 }, -30.0f, -10.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SKYFALL", "TURBOBUGGY", NULL, NULL, { 0 }, 40.0f, 20.0f,
        { -1 }, { 5, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "TWILIGHT FOUNDRY", "GROSSGIESSEREI", NULL, NULL, { 0 }, 10.0f, 30.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "CRYSTAL RIFT", "KRISTALLRISS", NULL, NULL, { 0 }, 20.0f, 120.0f,
        { 51, 31, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "ARGENT TOWERS", "SILBERSTADT", NULL, NULL, { 0 }, -20.0f, 20.0f,
        { 21, 34, -1 }, { 13, 17, 16, 3, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SKERRIES", "IMMOTURBO", NULL, NULL, { 0 }, 70.0f, 0.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "DIAMOND SANDS", "DIAMANTSAND", NULL, NULL, { 0 }, -70.0f, 110.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "EBONY COAST", "ELFENBEINKUESTE", NULL, NULL, { 0 }, -20.0f, 60.0f,
        { 32, -1 }, { 26, 17, 2, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "OYSTER HARBOUR", "AUSTERNHAFEN", NULL, NULL, { 0 }, -70.0f, -110.0f,
        { 23, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "CARRICK POINT", "MULTIPLIA", NULL, NULL, { 0 }, 20.0f, -20.0f,
        { 25, 30, -1 }, { 33, 18, 16, 3, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "HAVOC DISTRICT", "CHAOSDISTRIKT", NULL, NULL, { 0 }, 20.0f, 20.0f,
        { 22, 8, -1 }, { 7, 2, 15, 10, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "IRONSTONE MINE", "EISENMINE", NULL, NULL, { 0 }, -50.0f, 20.0f,
        { -1 }, { 10, 12, 13, 58, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BEETON TRACKS", "INTERCITY", NULL, NULL, { 0 }, 20.0f, -60.0f,
        { 48, -1 }, { 29, 15, 33, 57, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 0 }, "J-BOMB", "J-BOMB", NULL, NULL, { 0 }, 10.0f, 10.0f,
        { -1 }, { 16, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "JADE PLATEAU", "JADEPLATEAU", NULL, NULL, { 0 }, -10.0f, -30.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MARINE QUARTER", "AQUALONIA", NULL, NULL, { 0 }, -30.0f, 10.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "COOTER CREEK", "BASALKA", NULL, NULL, { 0 }, 30.0f, 10.0f,
        { 11, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "GIBBON'S GATE", "PACTRUCK", NULL, NULL, { 0 }, -50.0f, 180.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BABOON CATACOMB", "KATAKOMBO", NULL, NULL, { 0 }, 30.0f, -150.0f,
        { -1 }, { 37, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SLEEK STREETS", "SPIEGELSTRASSEN", NULL, NULL, { 0 }, 10.0f, -30.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "OBSIDIAN MILE", "OBSIDIANMEILE", NULL, NULL, { 0 }, -20.0f, 120.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "CORVINE BLUFF", "BLIFFBLUFF", NULL, NULL, { 0 }, 10.0f, 50.0f,
        { 56, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 40, 1 }, "SIDESWIPE", "XR7-PROTOCRASH", NULL, NULL, { 0 }, 10.0f, -10.0f,
        { -1 }, { 15, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "ECHO MARCHES", "ECHOLARIA", NULL, NULL, { 0 }, -20.0f, -60.0f,
        { 42, 39, -1 }, { 1, 58, 18, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "KIPLING PLANT", "TEMPOFABRIK", NULL, NULL, { 0 }, 30.0f, -10.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "FALCHION FIELD", "FATALDEMOLAGE", NULL, NULL, { 0 }, 45.0f, 100.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MORGAN HALL", "KISTENKNALL", NULL, NULL, { 0 }, -10.0f, 50.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "TEMPEST CITY", "TEMPCITY", NULL, NULL, { 0 }, 50.0f, -20.0f,
        { 35, -1 }, { 18, 5, 4, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "ORION PLAZA", "BILLIARDBOOM", NULL, NULL, { 0 }, -10.0f, 30.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "GLANDER'S RANCH", "GLANDERRANCH", NULL, NULL, { 0 }, 50.0f, -70.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "DAGGER PASS", "STAMPFSPEKTAKEL", NULL, NULL, { 0 }, -20.0f, 180.0f,
        { 52, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "GEODE SQUARE", "KUGELTRUBEL", NULL, NULL, { 0 }, 50.0f, 180.0f,
        { 59, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SHUTTLE ISLAND", "SHUTTLE-INSEL", NULL, NULL, { 0 }, 0.0f, -140.0f,
        { -1 }, { 40, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MICA PARK", "MICAPARK", NULL, NULL, { 0 }, -40.0f, -80.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MOON", "MOND", NULL, NULL, { 0 }, 10.0f, -140.0f,
        { -1 }, { 43, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "COBALT QUARRY", "COBALTRENNEN", NULL, NULL, { 0 }, 50.0f, 70.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MORAINE CHASE", "DUENENSPRINT", NULL, NULL, { 0 }, -10.0f, -80.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MERCURY", "MERKUR", NULL, NULL, { 0 }, 20.0f, -140.0f,
        { -1 }, { 44, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "VENUS", "VENUS", NULL, NULL, { 0 }, 30.0f, -140.0f,
        { -1 }, { 45, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MARS", "MARS", NULL, NULL, { 0 }, 40.0f, -140.0f,
        { -1 }, { 46, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "NEPTUNE", "NEPTUN", NULL, NULL, { 0 }, 50.0f, -140.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 0 }, "CMO INTRO", "CMO INTRO", NULL, NULL, { 0 }, 70.0f, 0.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SILVER JUNCTION", "KISTENSPRINT", NULL, NULL, { 0 }, 10.0f, -80.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "END SEQUENCE", "ENDSEQUENZ", NULL, NULL, { 0 }, 0.0f, -10.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SHUTTLE CLEAR", "SHUTTLEFREI", NULL, NULL, { 0 }, 0.0f, -15.0f,
        { -1 }, { 38, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "DARK HEARTLAND", "DUNKELHERZ", NULL, NULL, { 0 }, 0.0f, 90.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MAGMA PEAK", "MAGMAPOOL", NULL, NULL, { 0 }, 0.0f, -150.0f,
        { 24, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 10 }, "THUNDERFIST", "DONNERFAUST", NULL, NULL, { 0 }, -40.0f, -20.0f,
        { -1 }, { 58, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SALINE WATCH", "SALINENFEUER", NULL, NULL, { 0 }, 0.0f, 150.0f,
        { 36, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BACKLASH", "MONSTERTRUCK", NULL, NULL, { 0 }, -10.0f, 10.0f,
        { -1 }, { 10, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BISON RIDGE", "BISONFURCHE", NULL, NULL, { 0 }, 0.0f, 40.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "EMBER HAMLET", "EMBERHAM", NULL, NULL, { 0 }, 20.0f, -120.0f,
        { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "CROMLECH COURT", "SALTONIA", NULL, NULL, { 0 }, -50.0f, -20.0f,
        { -1 }, { 14, 17, 29, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "LIZARD ISLAND", "ECHSENINSEL", NULL, NULL, { 0 }, 30.0f, 150.0f,
        { 54, -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
};
#else
UnkStruct_8020D810 D_8020D810[0x3c] = {
    {
        { 100, 1 }, "SIMIAN ACRES", D_80303700, { 0 }, 0.0f, 0.0f, { -1 },
        { 28, 19, 3, 55, 50, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "ANGEL CITY", D_8030370C, { 0 }, -20.0f, -120.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "OUTLAND FARM", D_80303720, { 0 }, 20.0f, 60.0f, { 37, 27, -1 },
        { 5, 13, 9, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BLACKRIDGE WORKS", D_80303734, { 0 }, -20.0f, -20.0f, { 20, 6, -1 },
        { 29, 53, 15, 10, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "GLORY CROSSING", D_80303748, { 0 }, 70.0f, 180.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "SHUTTLE GULLY", D_8030375C, { 0 }, 50.0f, 20.0f, { 41, -1 }, { 4, 33, 2, -1 },
        0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SALVAGE WHARF", D_8030376C, { 0 }, -30.0f, -10.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "SKYFALL", D_8030377C, { 0 }, 40.0f, 20.0f, { -1 }, { 5, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "TWILIGHT FOUNDRY", D_80303794, { 0 }, 10.0f, 30.0f, { -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "CRYSTAL RIFT", D_803037A8, { 0 }, 20.0f, 120.0f, { 51, 31, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "ARGENT TOWERS", D_803037B8, { 0 }, -20.0f, 20.0f, { 21, 34, -1 },
        { 13, 17, 16, 3, -1 }, 0.0f, 0.0f, 0.0f,
    },
    { { 100, 1 }, "SKERRIES", D_803037CC, { 0 }, 70.0f, 0.0f, { -1 }, { -1 }, 0.0f, 0.0f, 0.0f },
    {
        { 100, 1 }, "DIAMOND SANDS", D_803037D8, { 0 }, -70.0f, 110.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "EBONY COAST", D_803037EC, { 0 }, -20.0f, 60.0f, { 32, -1 }, { 26, 17, 2, -1 },
        0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "OYSTER HARBOR", D_803037FC, { 0 }, -70.0f, -110.0f, { 23, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "CARRICK POINT", D_8030380C, { 0 }, 20.0f, -20.0f, { 25, 30, -1 },
        { 33, 18, 16, 3, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "HAVOC DISTRICT", D_8030381C, { 0 }, 20.0f, 20.0f, { 22, 8, -1 },
        { 7, 2, 15, 10, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "IRONSTONE MINE", D_8030382C, { 0 }, -50.0f, 20.0f, { -1 },
        { 10, 12, 13, 58, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BEETON TRACKS", D_80303844, { 0 }, 20.0f, -60.0f, { 48, -1 },
        { 29, 15, 33, 57, -1 }, 0.0f, 0.0f, 0.0f,
    },
    { { 0 }, "J-BOMB", D_80303854, { 0 }, 10.0f, 10.0f, { -1 }, { 16, -1 }, 0.0f, 0.0f, 0.0f },
    {
        { 100, 1 }, "JADE PLATEAU", D_80303864, { 0 }, -10.0f, -30.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "MARINE QUARTER", D_80303874, { 0 }, -30.0f, 10.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "COOTER CREEK", D_80303884, { 0 }, 30.0f, 10.0f, { 11, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "GIBBON'S GATE", D_80303890, { 0 }, -50.0f, 180.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "BABOON CATACOMB", D_803038A0, { 0 }, 30.0f, -150.0f, { -1 }, { 37, -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SLEEK STREETS", D_803038B0, { 0 }, 10.0f, -30.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "OBSIDIAN MILE", D_803038C0, { 0 }, -20.0f, 120.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "CORVINE BLUFF", D_803038D0, { 0 }, 10.0f, 50.0f, { 56, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 40, 1 }, "SIDESWIPE", D_803038E0, { 0 }, 10.0f, -10.0f, { -1 }, { 15, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "ECHO MARCHES", D_803038F8, { 0 }, -20.0f, -60.0f, { 42, 39, -1 },
        { 1, 58, 18, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "KIPLING PLANT", D_80303904, { 0 }, 30.0f, -10.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "FALCHION FIELD", D_80303918, { 0 }, 45.0f, 100.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "MORGAN HALL", D_80303930, { 0 }, -10.0f, 50.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "TEMPEST CITY", D_8030393C, { 0 }, 50.0f, -20.0f, { 35, -1 }, { 18, 5, 4, -1 },
        0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "ORION PLAZA", D_80303950, { 0 }, -10.0f, 30.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "GLANDER'S RANCH", D_80303960, { 0 }, 50.0f, -70.0f, { -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "DAGGER PASS", D_80303974, { 0 }, -20.0f, 180.0f, { 52, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "GEODE SQUARE", D_80303980, { 0 }, 50.0f, 180.0f, { 59, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "SHUTTLE ISLAND", D_80303990, { 0 }, 0.0f, -140.0f, { -1 }, { 40, -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "MICA PARK", D_803039A8, { 0 }, -40.0f, -80.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "MOON", D_803039B4, { 0 }, 10.0f, -140.0f, { -1 }, { 43, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "COBALT QUARRY", D_803039BC, { 0 }, 50.0f, 70.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "MORAINE CHASE", D_803039CC, { 0 }, -10.0f, -80.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "MERCURY", D_803039E0, { 0 }, 20.0f, -140.0f, { -1 }, { 44, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "VENUS", D_803039F0, { 0 }, 30.0f, -140.0f, { -1 }, { 45, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "MARS", D_803039FC, { 0 }, 40.0f, -140.0f, { -1 }, { 46, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "NEPTUNE", D_80303A04, { 0 }, 50.0f, -140.0f, { -1 }, { -1 }, 0.0f, 0.0f, 0.0f,
    },
    { { 0 }, "CMO INTRO", D_80303A14, { 0 }, 70.0f, 0.0f, { -1 }, { -1 }, 0.0f, 0.0f, 0.0f },
    {
        { 100, 1 }, "SILVER JUNCTION", D_80303A24, { 0 }, 10.0f, -80.0f, { -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "END SEQUENCE", D_80303A34, { 0 }, 0.0f, -10.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "SHUTTLE CLEAR", D_80303A48, { 0 }, 0.0f, -15.0f, { -1 }, { 38, -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "DARK HEARTLAND", D_80303A58, { 0 }, 0.0f, 90.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "MAGMA PEAK", D_80303A68, { 0 }, 0.0f, -150.0f, { 24, -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 10 }, "THUNDERFIST", D_80303A74, { 0 }, -40.0f, -20.0f, { -1 }, { 58, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "SALINE WATCH", D_80303A8C, { 0 }, 0.0f, 150.0f, { 36, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
    {
        { 100, 1 }, "BACKLASH", D_80303A98, { 0 }, -10.0f, 10.0f, { -1 }, { 10, -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "BISON RIDGE", D_80303AB0, { 0 }, 0.0f, 40.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "EMBER HAMLET", D_80303AC0, { 0 }, 20.0f, -120.0f, { -1 }, { -1 }, 0.0f, 0.0f,
        0.0f,
    },
    {
        { 100, 1 }, "CROMLECH COURT", D_80303AD4, { 0 }, -50.0f, -20.0f, { -1 },
        { 14, 17, 29, -1 }, 0.0f, 0.0f, 0.0f,
    },
    {
        { 100, 1 }, "LIZARD ISLAND", D_80303AE8, { 0 }, 30.0f, 150.0f, { 54, -1 }, { -1 }, 0.0f,
        0.0f, 0.0f,
    },
};
#endif
u16 D_8020E350[0x26] = {
    0, 0, 0x571, 0x570, 0x918, 0x917, 0x90C, 0x90B, 0x70A, 0x709, 0x772, 0x713, 0, 0, 0, 0, 0x912,
    0x911, 0x916, 0x915, 0x90A, 0x909, 0, 0, 0, 0, 0x90E, 0x90D, 0x908, 0x907, 0x91A, 0x919, 0x712,
    0x711,
};
u16 D_8020E39C[6] = { 0xA98, 0xA99, 0xA9A, 0xA9B };
Lights1 D_8020E3A8[2] = {
    {
        { { { 16, 16, 16 }, 0, { 16, 16, 16 }, 0 } },
        { { { { 255, 255, 255 }, 0, { 255, 255, 255 }, 0, { 0, 0, 1 }, 0 } } },
    },
    {
        { { { 16, 16, 16 }, 0, { 16, 16, 16 }, 0 } },
        { { { { 255, 255, 255 }, 0, { 255, 255, 255 }, 0, { 0, 0, 1 }, 0 } } },
    },
};
f32 D_8020E3D8 = 0.0f;

void func_801F8980(void) {
    s16 sp46;
    s16 sp44;
    s32 sp40;
    u32 sp3C;
    u8 *sp38;

    D_80358080 = 0;
    D_80358084 = 0;
    func_802A5720();
    func_8028A3E4();
    if (D_80358060 != 0) {
        func_80284E54(D_803156F8[D_8035805C].unk48B0, D_80358078, 2, 0, 1234, 0);
        func_80284E54(D_8021AB68, D_8021AB58, 0, 0, 1234, 0);
        func_80284E54(D_8021AB6C, D_8021AB5C, 1, 1, 1234, 0);
    } else {
        func_80284E54(D_803156F8[D_8035805C].unk48B0, D_80358078, 1, 1, 1234, 0);
    }
    D_8035805C ^= 1;
    sp38 = (u8 *)&D_803156F8[D_8035805C];
    func_8028A470();
    if ((D_8021AB2E != 0) ||
        ((func_802753C0() == 0) && (D_8021A924 == 1) && (D_80370C28 & 0x9000) && !(D_80370C2A & 0x9000))) {
        if (D_8021A926 == 0) {
            func_80260A10();
            func_80260650(D_80367738, 0x1E, 0);
            D_8021A924 = 2;
            func_8029A7E4("selected level %d\n", D_8021A905);
            D_802E8BDC = D_8021A905;
            func_801ECB18();
            D_8021AB2E = 0;
        } else {
            D_8021AB2E = 1;
        }
    }
    if ((D_80370C28 & 0x4000) && !(D_80370C2A & 0x4000) && (func_802753C0() == 0) && (D_8021A924 == 1)) {
        D_80364A87 = 0;
        func_80260650(D_80367738, 0xDE, 0);
        func_80275390(0x4000000000000000);
        D_802E8BDC = D_8021A905;
    }
    sp3C = (D_80370C2C * D_80370C2C) + (D_80370C2D * D_80370C2D);
    if (sp3C < 1500) {
        D_8021A930 = 1;
    }
    if (sp3C != 0) {
        D_8020E3D8 = func_8028BBF4(0, 0, D_80370C2C, -D_80370C2D);
    }
    if ((sp3C > 1500) && (D_8021A930 != 0) && (D_8021A907 >= 0) && (D_8021AB2C == 0xFF) && (D_8021A924 == 1) &&
        (D_8036BB1C == 1)) {
        D_8021A930 = 0;
        D_8021A904 = D_8021A905;
        func_801F885C(D_8021A907);
        D_8021A926 = 1;
        D_8021AB28 = 1.0f;
        if (D_8021AB38 != 0) {
            func_802608C8(D_8021AB38);
        }
    }
    switch (D_8021A924) {
        case 0:
            if (D_8021AB21 + 10 >= 0x100) {
                D_8021AB21 = 0xFF;
            } else {
                D_8021AB21 += 10;
            }
#ifdef VERSION_EU
            D_8021A918 -= 240.0f;
#else
            D_8021A918 -= 200.0f;
#endif
            if (D_8021A918 <= 925.0) {
                D_8021A924 = 1;
                D_8021A918 = 925.0f;
                D_8021AB24 = D_803156C4;
            }
            break;
        case 2:
#ifdef VERSION_EU
            D_8021A918 -= 60.0f;
#else
            D_8021A918 -= 50.0f;
#endif
            if (D_8021AB21 - 0x19 < 0) {
                D_8021AB21 = 0;
            } else {
                D_8021AB21 -= 0x19;
            }
            if (D_8021AB2C - 0x40 < 0) {
                D_8021AB2C = 0;
            } else {
                D_8021AB2C -= 0x40;
            }
            if (D_8021A918 <= 300.0) {
                D_80364A98 = func_80299FE8(D_802E8BDC);
            }
            break;
        default:
            D_8021AB21 = 0xFF;
            if (D_8021A926 == 0) {
                if (D_8021AB2C + 0x20 >= 0x100) {
                    D_8021AB2C = 0xFF;
                } else {
                    D_8021AB2C += 0x20;
                }
            }
            break;
    }
    if (D_80217B6C != 3) {
        D_8021AB21 = 0;
    }
    sp46 = D_8021A90C;
    sp44 = D_8021A910;
    func_801FD484(&D_8021A920, &D_8021A91C, &D_8021A90C, &D_8021A910, &D_8021A914, 925.0f);
    D_8021A934 = D_8021A90C - sp46;
    D_8021A938 = D_8021A910 - sp44;
    D_8021AB60 = D_8020D810[D_8021A905].unk14;
    D_8021AB64 = D_8020D810[D_8021A905].unk10;
#ifdef TARGET_PC
    /* the port's mouse or touch has turned the globe: the camera goes to
       where it was turned to instead of the selected level */
    if (port_globe_view(0)) {
        D_8021AB60 = port_globe_view(1) / 1000.0f;
        D_8021AB64 = port_globe_view(2) / 1000.0f;
        D_8021A934 = func_801FD6B8(D_8021AB60, D_8021A91C, 180.0f) * port_globe_view(3) / 1e+04f;
        D_8021A938 = func_801FD6B8(D_8021AB64, D_8021A920, 180.0f) * port_globe_view(3) / 1e+04f;
    } else {
#endif
    D_8021A934 = func_801FD6B8(D_8021AB60, D_8021A91C, 180.0f) * ((sp3C >> 2 < 500) ? 500 : sp3C >> 2) / 1e+04f;
    D_8021A938 = func_801FD6B8(D_8021AB64, D_8021A920, 180.0f) * ((sp3C >> 2 < 500) ? 500 : sp3C >> 2) / 1e+04f;
#ifdef TARGET_PC
    }
#endif
    D_8021A91C -= D_8021A934;
    D_8021A920 -= D_8021A938;
    D_8021AB68 = func_801F9258((Gfx *)(sp38 + 0x48B0), sp38, &D_80358078);
    D_8021AB6C = func_801F9820(D_8021AB68, sp38, &D_8021AB58);
    func_801F9B84(D_8021AB6C, sp38, &D_8021AB5C);
    func_801FD748();
    for (sp40 = 0; sp40 < D_80358080; sp40++) {
        osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    }
    for (sp40 = 0; sp40 < D_80358080 - D_80358084; sp40++) {
        func_802A57AC();
    }
}

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/11530/func_801F9258.s")
#else
Gfx *func_801F9258(Gfx *arg0, u8 *arg1, s32 *arg2) {
    Gfx *gfx = arg0;
    s32 spC0;
    s32 spBC;
    s32 spB8;
    s32 spB4;
    s32 spA0[5];
    s32 sp9C;

    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(arg1));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000038);
    gSPDisplayList(gfx++, D_01000010);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gDPSetFillColor(gfx++, 0x10001);
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    gDPPipeSync(gfx++);
    gDPPipelineMode(gfx++, G_PM_1PRIMITIVE);
    gDPSetColorDither(gfx++, G_CD_NOISE);
    gfx = func_801FE5D0(gfx, arg1);
    gfx = func_801F3450(gfx, arg1);
    func_80259450();
    {
        u32 sp68;
        s32 sp64;
        LevelInfo *sp60;
        s32 sp5C;
        s32 sp58;

        sp9C = 0x18;
        sp68 = D_8021AB2C / 9;
        func_80259DC8(arg1, LEVEL_NAME(D_8020D810[D_8021A908]), D_8020D810[D_8021A908].unk8, 0, 0xA0, 0,
                      (0x1C - sp68) / 2 + 0x12, sp9C, sp68, 1, 0xFF, 0xFF, 0xFF, D_8021AB2C, 0, 0, 0xFF,
                      D_8021AB2C);
        gfx = func_8024C404(gfx, arg1, &sp64);
        func_80259C24(&gfx, arg1);
        sp60 = &D_802E8F94[D_8021A908];
        gfx = func_80274868(gfx);
        spB4 = 0xDA;
        for (spBC = 0, spB8 = 0; spBC < 0x13 && spB4 > 0x28; spBC++) {
            if ((sp60->unk2C & (1 << spBC)) && ((sp60->unk0 == 1) || (D_8021AB30->unk10 & (1 << spBC))) &&
                (D_8020E350[spBC * 2] != 0)) {
                if (spB8 != 0) {
                    gfx = func_80272ED8(gfx, D_8021A8F0 + spBC, 0x16 - (0xFF - D_8021AB2C) / 6, spB4, D_8021AB2C, 0,
                                        0.8125f);
                } else {
                    gfx = func_80272ED8(gfx, D_8021A8F0 + spBC, (0xFF - D_8021AB2C) / 6 + 0xF6, spB4 -= 0x2C,
                                        D_8021AB2C, 0, 0.8125f);
                }
                spB8 ^= 1;
            }
        }
        sp5C = func_8025B498(0xA0, sp9C, LEVEL_NAME(D_8020D810[D_8021A908]), D_8020D810[D_8021A908].unk8);
        sp58 = (s32)(sp9C * D_802E8C84[0]) * func_8025B300(LEVEL_NAME(D_8020D810[D_8021A908]));
        gfx = func_80274AA4(gfx);
        gSPEndDisplayList(gfx++);
    }
    *arg2 = gfx - arg0;
    return gfx;
}
#endif

Gfx *func_801F9820(Gfx *arg0, u8 *arg1, s32 *arg2) {
    Gfx *gfx = arg0;

    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(arg1));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000010);
    gSPDisplayList(gfx++, D_01000038);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gDPSetDepthImage(gfx++, D_80358058);
    gDPPipeSync(gfx++);
    gDPSetRenderMode(gfx++, G_RM_AA_XLU_LINE, G_RM_AA_XLU_LINE2);
    gSPPerspNormalize(gfx++, D_8035807C);
    gSPMatrix(gfx++, arg1 + 0x80, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg1 + 0x140, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_80217B70[3][D_8035805C], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_80217B70[3][D_8035805C + 2], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg1 + 0x1280, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gfx = func_801FA180(gfx, arg1, D_8020E3D8, &D_8021A907);
    gSPEndDisplayList(gfx++);
    *arg2 = gfx - arg0;
    return gfx;
}

Gfx *func_801F9B84(Gfx *arg0, u8 *arg1, s32 *arg2) {
    Gfx *gfx = arg0;

    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(arg1));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000010);
    gSPDisplayList(gfx++, D_01000038);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gDPSetDepthImage(gfx++, D_80358058);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gSPTexture(gfx++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_8021AB21);
    gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
    gSPPerspNormalize(gfx++, D_8035807C);
    gSPMatrix(gfx++, arg1 + 0x80, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg1 + 0x140, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_80217B70[3][D_8035805C], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_80217B70[3][D_8035805C + 2], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg1 + 0x1280, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gfx = func_801FC5B8(arg1, gfx, D_8021A904, D_8021A905);
    func_801FDE98();
    gfx = func_80274BF0(arg1 + D_8035805C * 0x21498, gfx);
    gDPFullSync(gfx++);
    gSPEndDisplayList(gfx++);
    if (D_802FA264 != 0) {
        if (D_80370C28 & 0x800) {
            D_8021AB40 += 0.01;
        }
        if (D_80370C28 & 0x400) {
            D_8021AB40 -= 0.01;
        }
        if (D_80370C28 & 0xC00) {
            func_8029A7E4("angd %f\n", D_8021AB40);
        }
        if (D_80370C28 & 0x100) {
            D_8021AB44 += 0.001;
        }
        if (D_80370C28 & 0x200) {
            D_8021AB44 -= 0.001;
        }
        if (D_80370C28 & 0x300) {
            func_8029A7E4("dmm %f\n", D_8021AB44);
        }
        if (D_80370C28 & 0x10) {
            D_8021AB48 += 100.0f;
        }
        if (D_80370C28 & 0x20) {
            D_8021AB48 -= 100.0f;
        }
        if (D_80370C28 & 0x30) {
            func_8029A7E4("mmm %f\n", D_8021AB48);
        }
    }
    *arg2 = gfx - arg0;
    return gfx;
}

Gfx *func_801FA180(Gfx *arg0, u8 *arg1, f32 arg2, s8 *arg3) {
    UnkStruct_8020D810 *sp74;
    Gfx *gfx = arg0;
    f32 sp6C;
    f32 sp68;
    s32 sp64;
    s32 sp60;
    s8 sp5F;
    s8 sp5E;

    sp68 = 180.0f;
    D_8021A909 = 0;
    gDPPipeSync(gfx++);
    for (sp64 = 0; sp64 < 0x3C; sp64++) {
        D_8021A940[sp64] = 0;
    }
    for (sp64 = 0; sp64 < 0x3C; sp64++) {
        sp74 = &D_8020D810[sp64];
        if ((((D_80364AF0[D_80364AE8].medal[sp64] > 0) && (D_80364AF0[D_80364AE8].medal[sp64] < 6)) ? 1 : 0) != 0) {
            for (sp60 = 0; sp60 < 8 && sp74->unk1C[sp60] != -1; sp60++) {
                gfx = func_801FA74C(arg1, gfx, sp64, sp74->unk1C[sp60], &sp5F, &sp6C, 0, 0, 0, 0, 0, 0, 0);
            }
        }
    }
    for (sp64 = 0; sp64 < 0x3C; sp64++) {
        D_8021A940[sp64] = 0;
    }
    for (sp64 = 0; sp64 < 0x3C; sp64++) {
        sp74 = &D_8020D810[sp64];
        if ((((D_80364AF0[D_80364AE8].medal[sp64] > 0) && (D_80364AF0[D_80364AE8].medal[sp64] < 6)) ? 1 : 0) != 0) {
            for (sp60 = 0; sp60 < 4 && sp74->unk18[sp60] != -1; sp60++) {
                if (D_80364AF0[D_80364AE8].unk54[sp64] & (1 << sp60)) {
                    gfx = func_801FA74C(arg1, gfx, sp64, sp74->unk18[sp60], &sp5F, &sp6C, 1, 0, 0xFF, 0, 0xFF, 0xFF,
                                        0);
                    if (sp5F != -1) {
                        f32 sp54;

                        sp54 = func_801FD6B8(arg2, sp6C, 180.0f);
                        if (sp54 > 0.0f) {
                        } else {
                            sp54 = -sp54;
                        }
                        if (sp54 < sp68) {
                            sp68 = sp54;
                            sp5E = sp5F;
                        }
                    }
                }
            }
        }
    }
    for (sp64 = 0; sp64 < 0x3C; sp64++) {
        sp74 = &D_8020D810[sp64];
        if ((((D_80364AF0[D_80364AE8].medal[sp64] > 0) && (D_80364AF0[D_80364AE8].medal[sp64] < 6)) ? 1 : 0) != 0) {
            for (sp60 = 0; sp60 < 8 && sp74->unk1C[sp60] != -1; sp60++) {
                gfx = func_801FA74C(arg1, gfx, sp64, sp74->unk1C[sp60], &sp5F, &sp6C, 1, 0xFF, 0, 0, 0xFF, 0x80,
                                    0x80);
                if (sp5F != -1) {
                    f32 sp50;

                    sp50 = func_801FD6B8(arg2, sp6C, 180.0f);
                    if (sp50 > 0.0f) {
                    } else {
                        sp50 = -sp50;
                    }
                    if (sp50 < sp68) {
                        sp68 = sp50;
                        sp5E = sp5F;
                    }
                }
            }
        }
    }
    osWritebackDCache(D_8021A928[D_8035805C], 0x17000);
    if (sp68 < 45.0f) {
        *arg3 = sp5E;
    } else {
        *arg3 = -1;
    }
    return gfx;
}

#define ABS(x) (((x) > 0) ? (x) : -(x))

void func_8027690C(u8 *arg0, f32 x, f32 y, f32 z, s16 *outX, s16 *outY, Mtx *arg6, Mtx *arg7, Mtx *arg8, f32 arg9);

Gfx *func_801FA74C(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)
    u8 *arg0;
    Gfx *arg1;
    u8 arg2;
    u8 arg3;
    s8 *arg4;
    f32 *arg5;
    u8 arg6;
    u8 arg7;
    u8 arg8;
    u8 arg9;
    u8 arg10;
    u8 arg11;
    u8 arg12;
{
    Vtx *sp12C;
    Vtx *sp128;
    Gfx *gfx;
    s32 sp120;
    s32 sp11C;
    s32 sp118;
    u8 sp117;
    u8 sp116;
    f32 sp110;
    UnkStruct_8020D810 *sp10C;
    UnkStruct_8020D810 *sp108;
    f32 sp104;
    f32 sp100;
    f32 spFC;
    f32 spF8;
    s16 spF6;
    s16 spF4;
    s16 spF2;
    s16 spF0;
    f32 spEC;
    f32 spE8;
    f32 spE4;
    f32 spE0;
    f32 spDC;
    f32 spD8;
    f32 spD4;
    f32 spD0;
    f32 spCC;
    f32 spC8;
    f32 spC4;
    f32 spC0;
    f32 spBC;
    u8 spBB;
    u8 spBA;
    u32 spB4;
    u8 spB3;

    gfx = arg1;
    sp117 = 1;
    spBA = 0;
    spC0 = 1.0f;
    if (arg3 == D_8021A905 || D_80364AF0[D_80364AE8].medal[arg2] == 0) {
        spB3 = arg3;
        arg3 = arg2;
        arg2 = spB3;
    }
    if (D_8021A940[arg2] & ((u64)1 << arg3)) {
        sp116 = 1;
    } else {
        sp116 = 0;
    }
    D_8021A940[arg2] |= (u64)1 << arg3;
    D_8021A940[arg3] |= (u64)1 << arg2;
    *arg4 = -1;
    if (sp116 != 0 || func_801FE760(arg3) != 0) {
        return arg1;
    }
    if (func_80264BA4(arg2) == 3 || func_80264BA4(arg3) == 3) {
        spBA = 1;
    }
    if (arg2 == D_8021A905) {
        *arg4 = arg3;
    }
    if (func_80264BA4(arg2) == 3 && func_80264BA4(arg3) == 3) {
        if (D_80364A87 != 0 &&
            (D_80364AF0[D_80364AE8].medal[arg3] == 0 ||
             ((D_80364A87 & 1) && arg2 == D_8021A905 &&
              !(((D_80364AF0[D_80364AE8].medal[arg3] > 0) && (D_80364AF0[D_80364AE8].medal[arg3] < 6)) ? 1 : 0)))) {
            if (D_8021A924 != 0) {
#ifdef VERSION_EU
                sp110 = MIN(1.0, MAX(D_8021AB28, (f32)(D_803156C4 - D_8021AB24) * 60.0 / 50.0 / 90.0));
#else
                sp110 = MIN(1.0, MAX(D_8021AB28, (f32)(D_803156C4 - D_8021AB24) * 60.0 / 60.0 / 90.0));
#endif
                if (sp110 == 1.0 && D_8021AB38 != 0) {
                    func_802608C8(D_8021AB38);
                } else if (D_8021AB38 == 0 && sp110 != 1.0 && D_80217B6C == 3) {
                    func_80260650(D_80367738, 0x7C, &D_8021AB38);
                }
            } else {
                sp110 = 0.0f;
            }
        } else {
            sp110 = 1.0f;
        }
    } else {
        sp110 = 1.0f;
    }
    sp10C = &D_8020D810[arg2];
    sp108 = &D_8020D810[arg3];
    spEC = sp10C->unk24;
    spE4 = sp10C->unk28;
    spDC = sp10C->unk2C;
    spE8 = sp108->unk24;
    spE0 = sp108->unk28;
    spD8 = sp108->unk2C;
    spF8 = sqrtf(SQ(spE8 - spEC) + SQ(spE0 - spE4) + SQ(spD8 - spDC));
    sp118 = MAX(3.0, MIN(spF8 / 32.0, 15.0));
    sp11C = sp118 + 1;
    sp12C = sp128 = (Vtx *)D_8021A928[D_8035805C] + D_8021A909 * 16;
    spCC = (spEC * spE8 + spE4 * spE0 + spDC * spD8) / 250.0 / 250.0;
    spC8 = 90.0 - (((spCC >= 0.0f) ? 1 : -1) * func_802AD7D4(((spCC > 0.0f) ? spCC : -spCC) * 65535.0)) / 16.0 /
                      11.377777;
    if (spC8 >= 180.0) {
        spC8 -= 180.0;
    }
    if (spC8 < -180.0) {
        spC8 += 180.0;
    }
    spC8 *= 0.017453292519943295;
    spBB = 0;
    for (sp120 = 0; sp120 < sp11C; sp120++, sp12C++) {
        spC4 = MIN(sp110, 1.0 / (sp11C - 1) * (f32)sp120);
        spD4 = func_802574F0((1.0 - spC4) * spC8) / func_802574F0(spC8);
        spD0 = func_802574F0(spC4 * spC8) / func_802574F0(spC8);
        if (arg6 != 0) {
            spC0 = func_802574F0(spC4 * 3.141592653) * sp118 / 64.0 + 1.0;
        }
        sp104 = (spD4 * spEC + spD0 * spE8) * spC0;
        sp100 = (spD4 * spE4 + spD0 * spE0) * spC0;
        spFC = (spD4 * spDC + spD0 * spD8) * spC0;
        if (*arg4 != -1 && sp120 < 2) {
            switch (D_80217B6C) {
                case 3:
                    if (sp120 != 0) {
                        func_8027690C(arg0, sp104, sp100, spFC, &spF6, &spF4, &D_80217B70[3][D_8035805C],
                                      &D_80217B70[3][D_8035805C] + 2, (Mtx *)(arg0 + 0x1280), 1.0f);
                        *arg5 = func_8028BBF4(spF2, spF0, spF6, spF4);
                    } else {
                        func_8027690C(arg0, sp104, sp100, spFC, &spF2, &spF0, &D_80217B70[3][D_8035805C],
                                      &D_80217B70[3][D_8035805C] + 2, (Mtx *)(arg0 + 0x1280), 1.0f);
                    }
                    break;
                default:
                    if (sp120 != 0) {
                        *arg5 = (arg2 < arg3) ? 0 : 180;
                    }
                    break;
            }
        }
        sp12C->v.ob[0] = sp104;
        sp12C->v.ob[1] = sp100;
        sp12C->v.ob[2] = spFC;
        spBC = MAX(0.0, (sp104 * D_8021A90C + sp100 * D_8021A910 + spFC * D_8021A914) / spC0 / 250000.0);
        if (arg6 != 0) {
            spB4 = 1.0 /
                   MAX(0.001, ((spBC - D_8021AB40) > 0.0f) ? (spBC - D_8021AB40) : -(spBC - D_8021AB40)) *
                   D_8021AB44 * D_8021AB44 * D_8021AB48;
        } else {
            spB4 = 0;
        }
        sp12C->v.cn[0] = MIN(255.0, ((0.5 - ABS(spC4 - 0.5)) * (f32)arg10 + ABS(spC4 - 0.5) * (f32)arg7) * 2.0 + spB4);
        sp12C->v.cn[1] =
            MIN(255.0, ((0.5 - ABS(spC4 - 0.5)) * (f32)arg11 + ABS(spC4 - 0.5) * (f32)arg8) * 2.0 * spBC + spB4);
        sp12C->v.cn[2] =
            MIN(255.0, ((0.5 - ABS(spC4 - 0.5)) * (f32)arg12 + ABS(spC4 - 0.5) * (f32)arg9) * 2.0 * (1.0 - spBC) + spB4);
        sp12C->v.cn[3] = D_8021AB21 * spBC / (2 - arg6);
        if (sp120 != 0 && sp116 == 0 && spBA != 0 && spBC > 0.0f) {
            if (spBB == 0) {
                gSPVertex(gfx++, sp128, sp11C, 0);
                spBB = 1;
            }
            gSPLineW3D(gfx++, sp120 - 1, sp120, 2.0 - MIN(2.0, (D_8021A918 / 1000.0f) / (1.0 + spBC / 2.0f)), 0);
        }
    }
    D_8021A909++;
    return gfx;
}

Gfx *func_801FC5B8(u8 *arg0, Gfx *arg1, u8 arg2, u8 arg3) {
    f32 spAC;
    f32 spA8;
    Gfx *gfx;
    Vtx *spA0;
    UnkStruct_8020D810 *sp9C;
    UnkStruct_8020D810 *sp98;

    gfx = arg1;
    spA0 = D_8021A840[D_8035805C];
    switch (D_8021A926) {
        case 0:
            break;
        case 1:
            sp9C = &D_8020D810[arg2];
            sp98 = &D_8020D810[arg3];
            D_8021A8C4 = sp9C->unk24;
            D_8021A8CC = sp9C->unk28;
            D_8021A8D4 = sp9C->unk2C;
            D_8021A8C8 = sp98->unk24;
            D_8021A8D0 = sp98->unk28;
            D_8021A8D8 = sp98->unk2C;
            D_8021A8DC = sqrtf(SQ(D_8021A8C8 - D_8021A8C4) + SQ(D_8021A8D0 - D_8021A8CC) + SQ(D_8021A8D8 - D_8021A8D4));
            D_8021A8C0 = (((D_8021A8DC / 32.0 < 15.0) ? D_8021A8DC / 32.0 : 15.0) < 3.0)
                             ? 3.0
                             : ((D_8021A8DC / 32.0 < 15.0) ? D_8021A8DC / 32.0 : 15.0);
            if (arg2 != arg3) {
                D_8021A926 = 2;
            } else {
                D_8021A926 = 0;
            }
            D_8021A8E8 = 0.0f;
            D_8021A8E0 = ((D_8021A8C4 * D_8021A8C8) + (D_8021A8CC * D_8021A8D0) + (D_8021A8D4 * D_8021A8D8)) /
                         250.0 / 250.0;
            D_8021A8E4 = 90.0 - (((D_8021A8E0 >= 0.0f) ? 1 : -1) *
                                       func_802AD7D4(((D_8021A8E0 > 0.0f) ? D_8021A8E0 : -D_8021A8E0) * 65535.0)) /
                                          16.0 / 11.377777;
            if (D_8021A8E4 >= 180.0) {
                D_8021A8E4 -= 180.0;
            }
            if (D_8021A8E4 < -180.0) {
                D_8021A8E4 += 180.0;
            }
            D_8021A8E4 *= 0.017453292519943295;
            break;
        case 2:
            D_8021A8E8 += 0.4 / D_8021A8C0;
            if (D_8021A8E8 > 1.0) {
                D_8021A926 = 0;
                D_8021A8E8 = 1.0f;
            }
            D_8021AB2C = (((0.5 - D_8021A8E8) > 0.0) ? (0.5 - D_8021A8E8) : -(0.5 - D_8021A8E8)) * 510.0;
            if (D_8021A8E8 >= 0.5) {
                D_8021A908 = D_8021A905;
            }
            break;
    }
    spAC = func_802574F0((1.0 - D_8021A8E8) * D_8021A8E4) / func_802574F0(D_8021A8E4);
    spA8 = func_802574F0(D_8021A8E8 * D_8021A8E4) / func_802574F0(D_8021A8E4);
    D_8021A8EC = func_802574F0(D_8021A8E8 * 3.141592653) * D_8021A8C0 / 64.0 + 1.0;
    D_8021AB4C = ((spAC * D_8021A8C4) + (spA8 * D_8021A8C8)) * D_8021A8EC;
    D_8021AB50 = ((spAC * D_8021A8CC) + (spA8 * D_8021A8D0)) * D_8021A8EC;
    D_8021AB54 = ((spAC * D_8021A8D4) + (spA8 * D_8021A8D8)) * D_8021A8EC;
    func_801FCF38(spA0, D_8021AB4C, D_8021AB50, D_8021AB54, 0x20, 0x20, 1.75f, 1);
    osWritebackDCache(spA0, 0x40);
    gDPLoadTextureBlock(gfx++, D_80215A70[(D_803156C4 / 3) % 3], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSPVertex(gfx++, spA0, 4, 0);
    gSP1Triangle(gfx++, 0, 1, 2, 0);
    gSP1Triangle(gfx++, 2, 3, 0, 0);
    return gfx;
}

void func_801FCE74(Vtx *arg0, u8 arg1, f32 arg2, f32 arg3, u8 arg4, u8 arg5, f32 arg6, u8 arg7) {
    UnkStruct_8020D810 *sp3C;
    f32 sp38;
    f32 sp34;
    f32 sp30;
    f32 sp2C;
    f32 sp28;

    sp3C = &D_8020D810[arg1];
    sp38 = sp3C->unk10 + arg2;
    sp34 = sp3C->unk14 + arg3;
    func_801FD484(&sp38, &sp34, &sp30, &sp2C, &sp28, 248.75f);
    func_801FCF38(arg0, sp30, sp2C, sp28, arg4, arg5, arg6, arg7);
}

void func_801FCF38(Vtx *arg0, f32 arg1, f32 arg2, f32 arg3, u8 arg4, u8 arg5, f32 arg6, u8 arg7) {
    f32 spA0[4][4];
    f32 sp9C;
    f32 sp98;
    f32 sp94;
    f32 sp90;
    f32 sp8C;
    f32 sp88;
    f32 sp84;
    f32 sp80;
    f32 sp7C;
    f32 sp78;
    f32 sp74;
    f32 sp70;
    f32 sp6C;
    f32 sp68;
    f32 sp64;
    f32 sp60;
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    f32 sp50;

    sp9C = -arg3;
    sp98 = 0.0f;
    sp94 = arg1;
    sp90 = arg1 * arg2;
    sp8C = -((arg1 * arg1) + (arg3 * arg3));
    sp88 = arg2 * arg3;
    sp84 = sqrtf((sp9C * sp9C) + (sp98 * sp98) + (sp94 * sp94)) / 50.0 * 2.0 / arg6;
    if (sp84 < 0.1) {
        sp84 = 0.1f;
    }
    sp9C /= sp84;
    sp98 /= sp84;
    sp94 /= sp84;
    sp80 = sqrtf((sp90 * sp90) + (sp8C * sp8C) + (sp88 * sp88)) / 50.0 * 2.0 / arg6;
    if (sp80 < 0.1) {
        sp80 = 0.1f;
    }
    sp90 /= sp80;
    sp8C /= sp80;
    sp88 /= sp80;
    guRotateF(spA0, 45.0f, arg1, arg2, arg3);
    sp7C = arg1 + sp9C;
    sp6C = arg2 + sp98;
    sp5C = arg3 + sp94;
    sp78 = arg1 + sp90;
    sp68 = arg2 + sp8C;
    sp58 = arg3 + sp88;
    sp74 = arg1 - sp9C;
    sp64 = arg2 - sp98;
    sp54 = arg3 - sp94;
    sp70 = arg1 - sp90;
    sp60 = arg2 - sp8C;
    sp50 = arg3 - sp88;
    guMtxXFMF(spA0, sp7C, sp6C, sp5C, &sp7C, &sp6C, &sp5C);
    guMtxXFMF(spA0, sp78, sp68, sp58, &sp78, &sp68, &sp58);
    guMtxXFMF(spA0, sp74, sp64, sp54, &sp74, &sp64, &sp54);
    guMtxXFMF(spA0, sp70, sp60, sp50, &sp70, &sp60, &sp50);
    arg0[0].v.tc[0] = 0;
    arg0[0].v.tc[1] = (arg7 ? 0 : arg5 - 1) << 6;
    arg0[1].v.tc[0] = (arg4 - 1) << 6;
    arg0[1].v.tc[1] = (arg7 ? 0 : arg5 - 1) << 6;
    arg0[2].v.tc[0] = (arg4 - 1) << 6;
    arg0[2].v.tc[1] = (arg7 ? arg5 - 1 : 0) << 6;
    arg0[3].v.tc[0] = 0;
    arg0[3].v.tc[1] = (arg7 ? arg5 - 1 : 0) << 6;
    arg0[0].v.ob[0] = sp7C;
    arg0[0].v.ob[1] = sp6C;
    arg0[0].v.ob[2] = sp5C;
    arg0[1].v.ob[0] = sp78;
    arg0[1].v.ob[1] = sp68;
    arg0[1].v.ob[2] = sp58;
    arg0[2].v.ob[0] = sp74;
    arg0[2].v.ob[1] = sp64;
    arg0[2].v.ob[2] = sp54;
    arg0[3].v.ob[0] = sp70;
    arg0[3].v.ob[1] = sp60;
    arg0[3].v.ob[2] = sp50;
}

void func_801FD484(f32 *arg0, f32 *arg1, f32 *arg2, f32 *arg3, f32 *arg4, f32 arg5) {
    if ((*arg0 >= 90.0) || (*arg0 < -90.0)) {
        if (*arg0 >= 90.0) {
            *arg0 = 180.0 - *arg0;
        } else {
            *arg0 = -180.0 - *arg0;
        }
        *arg1 = *arg1 + 180.0;
    }
    if (*arg1 >= 180.0) {
        *arg1 = *arg1 - 360.0;
    } else if (*arg1 < -180.0) {
        *arg1 = *arg1 + 360.0;
    }
    *arg2 = func_80257514(-*arg1 * 0.017453292519943295) * func_80257514(*arg0 * 0.017453292519943295) * arg5;
    *arg3 = func_802574F0(*arg0 * 0.017453292519943295) * arg5;
    *arg4 = func_802574F0(-*arg1 * 0.017453292519943295) * func_80257514(*arg0 * 0.017453292519943295) * arg5;
}

f32 func_801FD6B8(f32 arg0, f32 arg1, f32 arg2) {
    f32 sp4;

    sp4 = arg1 - arg0;
    if ((sp4 >= -arg2) && (sp4 < arg2)) {
        return sp4;
    }
    if (sp4 >= arg2) {
        return (-2.0f * arg2) + sp4;
    }
    return (2.0f * arg2) + sp4;
}

void func_801FD748(void) {
    s32 sp74;
    s32 sp70;
    s32 sp6C;
    s32 sp68;
    s32 sp64;
    s32 sp60;
    s32 sp5C;
    s32 sp58;
    s32 sp54;
    s32 sp50;
    s32 sp4C;
    s32 sp48;
    s32 sp44;
    s32 pad[2];
    Vtx *sp38;
    Vtx *sp34;

    sp74 = 0;
    sp38 = D_803156F8[D_8035805C].unk15C0;
    sp34 = D_803156F8[D_8035805C ^ 1].unk15C0;
    do {
        sp48 = 0x40000000, sp4C = 0x40000000;
        sp70 = 0;
        sp50 = 0;
        sp54 = 0;
        do {
            sp6C = sp74 * 4 + sp70;
            sp68 = sp38[sp6C].v.ob[0] = sp34[sp6C].v.ob[0] - ((D_80217B6C == 3) ? D_8021A934 * 40.0f : 0.0f);
            sp64 = sp38[sp6C].v.ob[1] = sp34[sp6C].v.ob[1] + ((D_80217B6C == 3) ? D_8021A938 * 40.0f : 0.0f);
            sp60 = sp38[sp6C].v.ob[2] = sp34[sp6C].v.ob[2] - D_802159F0[sp74] * D_8021A918 / 925.0;
            if (((sp68 > 0) ? sp68 : -sp68) > 4000) {
                sp54 = sp68;
            }
            if (((sp64 > 0) ? sp64 : -sp64) > 4000) {
                sp50 = sp64;
            }
            if (((sp68 > 0) ? sp68 : -sp68) < sp4C) {
                sp4C = (sp68 > 0) ? sp68 : -sp68;
            }
            if (((sp64 > 0) ? sp64 : -sp64) < sp48) {
                sp48 = (sp64 > 0) ? sp64 : -sp64;
            }
        } while (++sp70 < 4);
        sp44 = 0;
        if ((sp54 != 0) || (sp50 != 0)) {
            sp70 = 0;
            do {
                sp6C = sp74 * 4 + sp70;
                sp5C = sp38[sp6C].v.ob[0];
                sp58 = sp38[sp6C].v.ob[1];
                if (sp54 != 0) {
                    sp68 = sp38[sp6C].v.ob[0] = func_801FD6B8(0.0f, sp38[sp6C].v.ob[0], 3890.0f);
                }
                if (sp50 != 0) {
                    sp64 = sp38[sp6C].v.ob[1] = func_801FD6B8(0.0f, sp38[sp6C].v.ob[1], 3890.0f);
                }
                if ((sp54 != 0) && ((sp5C * sp68) > 0)) {
                    sp44 = 1;
                }
                if ((sp50 != 0) && ((sp58 * sp64) > 0)) {
                    sp44 = 1;
                }
            } while (++sp70 < 4);
        }
        if ((sp44 != 0) || (sp60 < 0) || ((sp4C < 231250.0 / D_8021A918) && (sp48 < 231250.0 / D_8021A918))) {
            func_801FDCA4(sp38, sp74, 25000);
        }
    } while (++sp74 < 0x80);
}

void func_801FDCA4(Vtx *arg0, s32 arg1, s32 arg2) {
    s32 sp24;
    s32 sp20;
    s32 sp1C;

    sp24 = arg1 * 4;
#ifdef VERSION_EU
    D_802159F0[arg1] = func_8026A828(0x60, 0xF0);
#else
    D_802159F0[arg1] = func_8026A828(0x50, 0xC8);
#endif
    sp20 = func_8026A828(-0xFA0, 0xFA0);
    sp1C = func_8026A828(-0xFA0, 0xFA0);
    arg0[sp24 + 0].v.ob[0] = sp20 - 0x37;
    arg0[sp24 + 0].v.ob[1] = sp1C - 0x37;
    arg0[sp24 + 0].v.ob[2] = arg2;
    arg0[sp24 + 1].v.ob[0] = sp20 - 0x37;
    arg0[sp24 + 1].v.ob[1] = sp1C + 0x36;
    arg0[sp24 + 1].v.ob[2] = arg2;
    arg0[sp24 + 2].v.ob[0] = sp20 + 0x36;
    arg0[sp24 + 2].v.ob[1] = sp1C + 0x36;
    arg0[sp24 + 2].v.ob[2] = arg2;
    arg0[sp24 + 3].v.ob[0] = sp20 + 0x36;
    arg0[sp24 + 3].v.ob[1] = sp1C - 0x37;
    arg0[sp24 + 3].v.ob[2] = arg2;
}

void func_801FDE50(void) {
    D_8021A8F0 = func_80272C5C(D_8020E350, 0, 0x13, 2, 1, 1.0f);
}

void func_801FDE98(void) {
    f32 sp24;
    f32 sp20;
    f32 sp1C;
    f32 sp18;

    sp24 = sqrtf((D_8021AB4C * D_8021AB4C) + (D_8021AB50 * D_8021AB50) + (D_8021AB54 * D_8021AB54));
    if (sp24 < 1.0) {
        sp24 = 1.0f;
    }
    sp20 = (D_8021AB4C / sp24) * 120.0f;
    sp1C = (D_8021AB50 / sp24) * 120.0f;
    sp18 = (D_8021AB54 / sp24) * 120.0f;
    D_8020E3A8[D_8035805C].l[0].l.dir[0] = sp20;
    D_8020E3A8[D_8035805C].l[0].l.dir[1] = sp1C;
    D_8020E3A8[D_8035805C].l[0].l.dir[2] = sp18;
    osWritebackDCache(&D_8020E3A8[D_8035805C], 0x30);
}

void func_801FE018(u8 arg0) {
    s32 sp34;
    s32 sp30;
    UnkStruct_8020D810 *sp2C;
    LevelInfo *sp28;

    D_8021AB30 = &D_80364AF0[D_80364AE8];
    for (sp34 = 0; sp34 < 0x3C; sp34++) {
        if ((((D_80364AF0[D_80364AE8].medal[sp34] > 0) && (D_80364AF0[D_80364AE8].medal[sp34] < 6)) ? 1 : 0) != 0) {
            sp2C = &D_8020D810[sp34];
            sp28 = &D_802E8F94[sp34];
            for (sp30 = 0; sp30 < 8 && sp2C->unk1C[sp30] != -1; sp30++) {
                if ((D_8021AB30->medal[sp2C->unk1C[sp30]] == 0) && (func_801FE760(sp2C->unk1C[sp30]) == 0)) {
                    D_8021AB30->medal[sp2C->unk1C[sp30]] = arg0;
                }
            }
            for (sp30 = 0; sp30 < 4 && sp2C->unk18[sp30] != -1; sp30++) {
                if (D_8021AB30->unk54[sp34] & (1 << sp30)) {
                    if (D_8021AB30->medal[sp2C->unk18[sp30]] == 0) {
                        D_8021AB30->medal[sp2C->unk18[sp30]] = arg0;
                    }
                }
            }
        }
    }
}

Gfx *func_801FE238(Gfx *arg0, u8 *arg1) {
    Gfx *gfx = arg0;

    gDPSetTextureLOD(gfx++, G_TL_LOD);
    gDPSetCycleType(gfx++, G_CYC_2CYCLE);
    gDPSetRenderMode(gfx++, G_RM_PASS, G_RM_AA_OPA_TERR2);
    if (D_80217B6C == 3) {
        gSPSetLights1(gfx++, D_8020E3A8[D_8035805C]);
        gSPSetGeometryMode(gfx++, G_LIGHTING | G_CULL_BACK | G_SHADE | G_SHADING_SMOOTH);
        gDPSetCombineLERP(gfx++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL0,
                          0, SHADE, 0, 0, 0, 0, SHADE);
    } else {
        gSPSetGeometryMode(gfx++, G_LIGHTING | G_CULL_BACK | G_SHADE | G_SHADING_SMOOTH);
        gDPSetCombineLERP(gfx++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, 0, 0, 0,
                          TEXEL0, 0, 0, 0, SHADE);
    }
    gSPDisplayList(gfx++, D_8021A8F4);
    gDPPipeSync(gfx++);
    gSPClearGeometryMode(gfx++, G_LIGHTING);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, D_8021AB21);
    gDPSetEnvColor(gfx++, 0, 0, 0, D_8021AB21 * 2 / 3);
    gSPDisplayList(gfx++, D_8021A8FC);
    gSPDisplayList(gfx++, D_8021A900);
    gSPEndDisplayList(gfx++);
    return gfx;
}

Gfx *func_801FE5D0(Gfx *arg0, u8 *arg1) {
    Gfx *gfx = arg0;

    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gDPSetRenderMode(gfx++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
    gDPSetCombineMode(gfx++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gSPDisplayList(gfx++, D_8021A8F8);
    gDPPipeSync(gfx++);
    return gfx;
}

u8 func_801FE760(u8 arg0) {
    u8 sp7;

    sp7 = 0;
    if ((D_802E8F94[arg0].gameState > D_80364AF0[D_80364AE8].gameState) &&
        ((D_802E8F94[arg0].unk0 & 0x81) || ((arg0 >= 0x2B) && (arg0 < 0x2F)))) {
        sp7 = 1;
    }
    if (arg0 == 0xA) {
        if ((((D_80364AF0[D_80364AE8].medal[0x37] > 0) && (D_80364AF0[D_80364AE8].medal[0x37] < 6)) ? 1 : 0) == 0) {
            sp7 = 1;
        }
    }
    if (arg0 == 0xF) {
        if ((((D_80364AF0[D_80364AE8].medal[0x1C] > 0) && (D_80364AF0[D_80364AE8].medal[0x1C] < 6)) ? 1 : 0) == 0) {
            sp7 = 1;
        }
    }
    if (arg0 == 0x3A) {
        if ((((D_80364AF0[D_80364AE8].medal[0x35] > 0) && (D_80364AF0[D_80364AE8].medal[0x35] < 6)) ? 1 : 0) == 0) {
            sp7 = 1;
        }
    }
    if (arg0 == 5) {
        if ((((D_80364AF0[D_80364AE8].medal[7] > 0) && (D_80364AF0[D_80364AE8].medal[7] < 6)) ? 1 : 0) == 0) {
            sp7 = 1;
        }
    }
    if (arg0 == 0x10) {
        if ((((D_80364AF0[D_80364AE8].medal[0x13] > 0) && (D_80364AF0[D_80364AE8].medal[0x13] < 6)) ? 1 : 0) == 0) {
            sp7 = 1;
        }
    }
    if (arg0 == 0) {
        sp7 = 0;
    }
    return sp7;
}
