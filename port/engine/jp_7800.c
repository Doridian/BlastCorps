/*
 * hd_front_end 7800 (stats.c: a level's results), jp's func_801EE800,
 * which the C has as jp's GLOBAL_ASM, as native C (engine.h; jp_E7B0.c says more).
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

/* a level's results into the player's record: the medal (returned), the
   units it brings, the time; *arg0 set when the rank goes up */
u8 func_801EE800(u8 *arg0, u8 arg1, u8 arg2) {
    PlayerInfo *player = &D_80364AF0[D_80364AE8];
    u32 percent;
    u8 medal;

    func_8029A7E4("new ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA70.ip, D_8036EA70.tc,
                  D_8036EA70.bd, D_8036EA70.cr, D_8036EA70.rt, D_8036EA70.coin, D_8036EA70.bdn);
    func_8029A7E4("old ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA60.ip, D_8036EA60.tc,
                  D_8036EA60.bd, D_8036EA60.cr, D_8036EA60.rt, D_8036EA60.coin, D_8036EA60.bdn);
    func_8029A7E4("res ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA80.ip, D_8036EA80.tc,
                  D_8036EA80.bd, D_8036EA80.cr, D_8036EA80.rt, D_8036EA80.coin, D_8036EA80.bdn);
    func_8029A7E4("rs2 ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n", D_8036EA90.ip, D_8036EA90.tc,
                  D_8036EA90.bd, D_8036EA90.cr, D_8036EA90.rt, D_8036EA90.coin, D_8036EA90.bdn);
    func_8029A7E4("units %d\n", player->units);
    if (D_802E8F94[D_802E8BDC].unk0 == 1) {
        percent = func_802852EC();
        if (arg2 != 0) {
            if (arg1 != 0) {
                if (percent >= 100) {
                    D_8036EA70.coin = 3;
                } else {
                    if (percent >= 90) {
                        D_8036EA70.coin = 2;
                    } else {
                        if (percent >= 70) {
                            D_8036EA70.coin = 1;
                        } else {
                            D_8036EA70.coin = 5;
                        }
                    }
                }
                if (D_803643D5 != 0) {
                    func_8029A7E4("Units up 3\n");
                    player->units += 3;
                }
                D_8036EA70.bdn = 1;
            } else {
                D_8036EA70.coin = 0;
            }
        }
        medal = D_8036EA70.coin;
    } else {
        medal = func_801EEDB4(D_802E8BDC, arg1, arg2);
    }
    sprintf(D_8036B980, "%s", D_8020D810[D_802E8BDC].name);
    func_8025B5D4(D_802F4880, D_802E8CA0, D_8020D810[D_802E8BDC].unk8, 0);
    *arg0 = 0;
    if (arg1 == 0)
        goto out;
    if (arg2 == 0)
        goto out;
    if (D_802E8BDC == 0x31)
        goto dummy;
    if (D_802E8BDC == 0x2F)
        goto dummy;
    if (D_802E8BDC == 0x26) {
    dummy:
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!DUMMY_LEVELS(levelno)", "stats.c", 0x5E);
    }
    if (D_802E8F94[D_802E8BDC].unk0 == 1) {
        player->unk14 = D_803649F0;
    }
    if (player->units < 360) {
        func_8029A7E4("UNITS UP %d\n", D_8036EA70.coin % 5 - D_8036EA60.coin % 5);
        player->units += D_8036EA70.coin % 5 - D_8036EA60.coin % 5;
    }
    if (player->units == 354) {
        player->units += 6;
    }
    if (player->units / 12 > player->unkC) {
        *arg0 = 1;
        player->unkC++;
    }
    if (!(D_802E8F94[D_802E8BDC].unk0 & 0x81)) {
        player->unk92[D_802E8BDC] = D_8036EA70.bdn;
    }
    D_80364EF0[D_80364AE8][D_802E8C44[D_8036EA70.bdn]] = D_8036EA70.tc;
    if (D_803643D5 != 0) {
        if (D_802E8F94[D_802E8BDC].unk0 == 1) {
            D_80364EF0[D_80364AE8][D_802E8C44[0]] = D_8036EA70.tc;
        }
    }
    player->medal[D_802E8BDC] = medal;
    func_801E8DCC(D_80364AE8);
out:
    return medal;
}

#endif
