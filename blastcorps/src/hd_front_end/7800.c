#include "common.h"
#include "game/frontend.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

/* Per-frame buffer, double-buffered by D_8035805C. */
void func_801E8DCC(u8);
u8 func_801EEDB4(u8, u8, u8);
u8 func_801EF2BC(u16, u8, u8);
Gfx *func_801F4FBC(FrameBuf *, Gfx *);
void func_80260650(SndBank *, s32, s32);
void func_80264A34(char *, u16, s32);
u8 func_80272C5C(u16 *, u16 *, u8, u8, u8, f32);
void func_80284E54(Gfx *, s32, s32, s32, s32, s32);
u32 func_802852EC(void);
s32 func_80286038(u16);
void func_8028A3E4(void);
void func_8028A470(void);
void func_80295A20(s32);
void func_8029A7E4(char *, ...);

extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern char *D_802084D0[];
extern u16 *D_802084E0[];
extern s32 D_802FA268;
extern FrameBuf D_803156F8[];
extern s32 D_80358078;
extern u16 D_8035807C;
extern char D_8036B980[];
extern char D_8036B9A8[];

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/7800/func_801EE800.s")
#else
u8 func_801EE800(u8 *arg0, u8 arg1, u8 arg2) {
    PlayerInfo *sp3C;
    LevelInfo *sp38;
    u32 sp34;
    u8 sp33;
    s32 sp2C;
    s32 sp28;

    sp3C = &D_80364AF0[D_80364AE8];
    sp38 = &D_802E8F94[D_802E8BDC];
    func_8029A7E4("new ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA70.ip, D_8036EA70.tc, D_8036EA70.bd, D_8036EA70.cr,
                  D_8036EA70.rt, D_8036EA70.coin, D_8036EA70.bdn);
    func_8029A7E4("old ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA60.ip, D_8036EA60.tc, D_8036EA60.bd, D_8036EA60.cr,
                  D_8036EA60.rt, D_8036EA60.coin, D_8036EA60.bdn);
    func_8029A7E4("res ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA80.ip, D_8036EA80.tc, D_8036EA80.bd, D_8036EA80.cr,
                  D_8036EA80.rt, D_8036EA80.coin, D_8036EA80.bdn);
    func_8029A7E4("rs2 ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA90.ip, D_8036EA90.tc, D_8036EA90.bd, D_8036EA90.cr,
                  D_8036EA90.rt, D_8036EA90.coin, D_8036EA90.bdn);
    func_8029A7E4("units %d\n", sp3C->units);
    if (D_802E8F94[D_802E8BDC].unk0 == 1) {
        sp34 = func_802852EC();
        if (arg2) {
            if (arg1) {
                if (sp34 >= 100) {
                    D_8036EA70.coin = 3;
                } else if (sp34 >= 90) {
                    D_8036EA70.coin = 2;
                } else if (sp34 >= 70) {
                    D_8036EA70.coin = 1;
                } else {
                    D_8036EA70.coin = 5;
                }
                if (D_803643D5 != 0) {
                    func_8029A7E4("Units up 3\n");
                    sp3C->units += 3;
                }
                D_8036EA70.bdn = 1;
            } else {
                D_8036EA70.coin = 0;
            }
        }
        sp33 = D_8036EA70.coin;
    } else {
        sp33 = func_801EEDB4(D_802E8BDC, arg1, arg2);
    }
    sprintf(D_8036B980, "%s", D_8020D810[D_802E8BDC].name);
    *arg0 = 0;
    if (arg1 && arg2) {
        if (DUMMY_LEVELS(D_802E8BDC)) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!DUMMY_LEVELS(levelno)", "stats.c", 0x5E);
        }
        if (D_802E8F94[D_802E8BDC].unk0 == 1) {
            sp3C->unk14 = D_803649F0;
        }
        if (sp3C->units < 360) {
            func_8029A7E4("UNITS UP %d\n", D_8036EA70.coin % 5 - D_8036EA60.coin % 5);
            sp3C->units += D_8036EA70.coin % 5 - D_8036EA60.coin % 5;
        }
        if (sp3C->units == 354) {
            sp3C->units += 6;
        }
        if (sp3C->units / 12 > sp3C->unkC) {
            *arg0 = 1;
            sp3C->unkC++;
        }
        if (!(D_802E8F94[D_802E8BDC].unk0 & 0x81)) {
            sp3C->unk92[D_802E8BDC] = D_8036EA70.bdn;
        }
        D_80364EF0[D_80364AE8][D_802E8C44[D_8036EA70.bdn]] = D_8036EA70.tc;
        if (D_803643D5 != 0 && D_802E8F94[D_802E8BDC].unk0 == 1) {
            D_80364EF0[D_80364AE8][D_802E8C44[0]] = D_8036EA70.tc;
        }
        sp3C->medal[D_802E8BDC] = sp33;
        func_801E8DCC(D_80364AE8);
    }
    return sp33;
}
#endif

extern u16 D_80303B3C[];
extern u16 D_80303B48[];
extern u16 D_80303B58[];
extern u16 D_80303B68[];
/* .data, 0x802084D0-0x802084F0 (tools/data_c.py) */
char *D_802084D0[4] = { "YOUR NEW BEST!", "BEST TO DATE", "YOUR BEST STAYS", "GUEST BEST IS" };
u16 *D_802084E0[4] = { D_80303B3C, D_80303B48, D_80303B58, D_80303B68 };

u8 func_801EEDB4(u8 arg0, u8 arg1, u8 arg2) {
    s32 sp6C;
    s32 sp68;
    s32 sp64;
    PlayerInfo *sp60;
    LevelInfo *sp5C;
    YoshiIcon *sp58;
    YoshiEntry *sp54;
    char sp34[0x20];
    u16 sp32;

    sp60 = &D_80364AF0[D_80364AE8];
    sp5C = &D_802E8F94[arg0];
    if (D_80364A98 == 0x08000000 && arg1 && sp5C->unk0 == 2) {
        func_80295A20(func_80286038(D_8036EA70.tc));
    }
    if (arg2 && arg1) {
        if (D_802E8F94[arg0].unk0 == 0x80) {
            D_8036EA70.bdn = 0;
        } else if (D_8036EA70.tc <= D_8036EA60.tc) {
            D_8036EA70.bdn = D_803643D4;
        } else if (D_8036EA70.tc != 0xFFFF) {
            if ((sp32 = D_80364EF0[D_80364AE8][D_802E8C44[D_803643D4]]) == 0 || D_8036EA70.tc < sp32) {
                D_80364EF0[D_80364AE8][D_802E8C44[D_803643D4]] = D_8036EA70.tc;
            }
        }
    }
    if (D_8036EA70.tc <= D_8036EA60.tc && arg1) {
        D_8036EA70.coin = func_801EF2BC(D_8036EA70.tc, arg0, D_80364AF0[D_80364AE8].gameState);
    } else {
        D_8036EA70.tc = D_8036EA60.tc;
    }
    if (D_8036EA70.tc < D_8036EA60.tc && D_803643D5 == 0) {
        sp6C = 0x484;
        if (arg2) {
            sp6C = 0x584;
        }
    } else {
        sp6C = 0x480;
    }
    func_80264A34(sp34, D_8036EA70.tc, 0);
    sprintf(D_8036B9A8 + 0x80, "****%s*", sp34);
    if (arg1) {
        sp54 = &D_8020C070[FE_ENTRY(25)];
        D_8020C070[FE_ENTRY(25)].flags = sp6C;
        func_8029A7E4("getting icon %d\n", D_8036EA70.bdn);
        sp54->unk14 = D_8036EA70.bdn + 0x22;
        sp58 = &D_802F49F4[sp54->unk14];
        sp54->unk1A = func_80272C5C((u16 *)sp58->unk6, NULL, sp58->unk4, sp58->unk2C, sp58->unk2D | 4, 1.0f);
        if (D_80364AE8 != D_80364AEA) {
            sp64 = 3;
        } else if (D_80364A98 == 0x80 || D_803643D5 != 0) {
            sp64 = 1;
        } else if (D_8036EA70.tc < D_8036EA60.tc) {
            sp64 = 0;
        } else {
            sp64 = 2;
        }
        D_8020C070[FE_ENTRY(24)].text = D_802084D0[sp64];
        D_8020C070[FE_ENTRY(24)].unk10 = D_802084E0[sp64];
    }
    return D_8036EA70.coin;
}

s8 func_801EF1E0(void) {
    UnkStruct_8020D810 *spC;
    s32 sp8;
    s32 sp4;

    spC = &D_8020D810[D_802E8BDC];
    if (spC->unk18[0] == -1) {
        return -1;
    }
    for (sp8 = 0, sp4 = 0; spC->unk18[sp8] != -1 && sp8 < 2; sp8++) {
        if (D_80364AF0[D_80364AE8].unk54[D_802E8BDC] & (1 << sp8)) {
            sp4++;
        }
    }
    return sp4;
}

u8 func_801EF2BC(u16 arg0, u8 arg1, u8 arg2) {
    u8 sp7;
    LevelInfo *sp0;

    sp0 = &D_802E8F94[arg1];
    if (sp0->medalTimes[0] >= arg0 && arg2 >= 12) {
        sp7 = 4;
    } else if (sp0->medalTimes[1] >= arg0) {
        sp7 = 3;
    } else if (sp0->medalTimes[2] >= arg0) {
        sp7 = 2;
    } else if (sp0->medalTimes[3] >= arg0) {
        sp7 = 1;
    } else {
        sp7 = 5;
    }
    return sp7;
}
