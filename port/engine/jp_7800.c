/*
 * hd_front_end 7800 (stats.c: a level's results), jp's func_801EE800,
 * which the C has as jp's GLOBAL_ASM, as native C charged by the original's
 * blocks (engine.h; jp_E7B0.c says more).
 *
 * jp's also puts the level's u16 name into D_802F4880 (func_8025B5D4)
 * after the char one into D_8036B980.
 */
#include "engine.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

#ifdef VERSION_JP

extern char D_8036B980[];
extern u16 D_802F4880[];
extern u16 D_802E8CA0[];

void func_801E8DCC(u8);
u8 func_801EEDB4(u8, u8, u8);
u32 func_802852EC(void);
u16 *func_8025B5D4(u16 *, u16 *, u16 *, s32);
void func_8029A7E4(char *, ...);

/* a level's results into the player's record: the medal (returned), the
   units it brings, the time; *arg0 set when the rank goes up */
u8 func_801EE800(u8 *arg0, u8 arg1, u8 arg2) {
    PlayerInfo *player = &D_80364AF0[D_80364AE8];
    u32 percent;
    u8 medal;

    ENGINE_BLK(801EE800);
    func_8029A7E4("new ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA70.ip, D_8036EA70.tc,
                  D_8036EA70.bd, D_8036EA70.cr, D_8036EA70.rt, D_8036EA70.coin, D_8036EA70.bdn);
    ENGINE_BLK(801EE8A8);
    func_8029A7E4("old ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA60.ip, D_8036EA60.tc,
                  D_8036EA60.bd, D_8036EA60.cr, D_8036EA60.rt, D_8036EA60.coin, D_8036EA60.bdn);
    ENGINE_BLK(801EE8FC);
    func_8029A7E4("res ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA80.ip, D_8036EA80.tc,
                  D_8036EA80.bd, D_8036EA80.cr, D_8036EA80.rt, D_8036EA80.coin, D_8036EA80.bdn);
    ENGINE_BLK(801EE950);
    func_8029A7E4("rs2 ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA90.ip, D_8036EA90.tc,
                  D_8036EA90.bd, D_8036EA90.cr, D_8036EA90.rt, D_8036EA90.coin, D_8036EA90.bdn);
    ENGINE_BLK(801EE9A4);
    func_8029A7E4("units %d\n", player->units);
    ENGINE_BLK(801EE9B8);
    if (D_802E8F94[D_802E8BDC].unk0 == 1) {
        ENGINE_BLK(801EE9E4);
        percent = func_802852EC();
        ENGINE_BLK(801EE9EC);
        if (arg2 != 0) {
            ENGINE_BLK(801EE9FC);
            if (arg1 != 0) {
                ENGINE_BLK(801EEA08);
                if (percent >= 100) {
                    ENGINE_BLK(801EEA18);
                    D_8036EA70.coin = 3;
                } else {
                    ENGINE_BLK(801EEA28);
                    if (percent >= 90) {
                        ENGINE_BLK(801EEA38);
                        D_8036EA70.coin = 2;
                    } else {
                        ENGINE_BLK(801EEA48);
                        if (percent >= 70) {
                            ENGINE_BLK(801EEA58);
                            D_8036EA70.coin = 1;
                        } else {
                            ENGINE_BLK(801EEA68);
                            D_8036EA70.coin = 5;
                        }
                    }
                }
                ENGINE_BLK(801EEA74);
                if (D_803643D5 != 0) {
                    ENGINE_BLK(801EEA84);
                    func_8029A7E4("Units up 3\n");
                    ENGINE_BLK(801EEA90);
                    player->units += 3;
                }
                ENGINE_BLK(801EEAA0);
                D_8036EA70.bdn = 1;
            } else {
                ENGINE_BLK(801EEAB0);
                D_8036EA70.coin = 0;
            }
        }
        ENGINE_BLK(801EEAB8);
        medal = D_8036EA70.coin;
    } else {
        ENGINE_BLK(801EEAC8);
        medal = func_801EEDB4(D_802E8BDC, arg1, arg2);
        ENGINE_BLK(801EEADC);
    }
    ENGINE_BLK(801EEAE0);
    engine_sprintf(D_8036B980, "%s", D_8020D810[D_802E8BDC].name);
    ENGINE_BLK(801EEB14);
    func_8025B5D4(D_802F4880, D_802E8CA0, D_8020D810[D_802E8BDC].unk8, 0);
    ENGINE_BLK(801EEB4C);
    *arg0 = 0;
    if (arg1 == 0)
        goto out;
    ENGINE_BLK(801EEB60);
    if (arg2 == 0)
        goto out;
    ENGINE_BLK(801EEB6C);
    if (D_802E8BDC == 0x31)
        goto dummy;
    ENGINE_BLK(801EEB80);
    if (D_802E8BDC == 0x2F)
        goto dummy;
    ENGINE_BLK(801EEB88);
    if (D_802E8BDC == 0x26) {
    dummy:
        ENGINE_BLK(801EEB90);
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!DUMMY_LEVELS(levelno)", "stats.c", 0x5E);
    }
    ENGINE_BLK(801EEBB0);
    if (D_802E8F94[D_802E8BDC].unk0 == 1) {
        ENGINE_BLK(801EEBDC);
        player->unk14 = D_803649F0;
    }
    ENGINE_BLK(801EEBEC);
    if (player->units < 360) {
        ENGINE_BLK(801EEC00);
        func_8029A7E4("UNITS UP %d\n", D_8036EA70.coin % 5 - D_8036EA60.coin % 5);
        ENGINE_BLK(801EEC38);
        player->units += D_8036EA70.coin % 5 - D_8036EA60.coin % 5;
    }
    ENGINE_BLK(801EEC70);
    if (player->units == 354) {
        ENGINE_BLK(801EEC84);
        player->units += 6;
    }
    ENGINE_BLK(801EEC8C);
    if (player->units / 12 > player->unkC) {
        ENGINE_BLK(801EECB0);
        *arg0 = 1;
        player->unkC++;
    }
    ENGINE_BLK(801EECCC);
    if (!(D_802E8F94[D_802E8BDC].unk0 & 0x81)) {
        ENGINE_BLK(801EECF8);
        player->unk92[D_802E8BDC] = D_8036EA70.bdn;
    }
    ENGINE_BLK(801EED0C);
    D_80364EF0[D_80364AE8][D_802E8C44[D_8036EA70.bdn]] = D_8036EA70.tc;
    if (D_803643D5 != 0) {
        ENGINE_BLK(801EED54);
        if (D_802E8F94[D_802E8BDC].unk0 == 1) {
            ENGINE_BLK(801EED80);
            D_80364EF0[D_80364AE8][D_802E8C44[0]] = D_8036EA70.tc;
        }
    }
    ENGINE_BLK(801EEDB4);
    player->medal[D_802E8BDC] = medal;
    func_801E8DCC(D_80364AE8);
out:
    ENGINE_BLK(801EEDD8);
    return medal;
}

#endif
