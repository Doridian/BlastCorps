/*
 * hd_front_end 1C40 (player.c: the players' menu and its scroller), jp's
 * func_801E8EB8 and func_801EC770, which the C has as jp's GLOBAL_ASM, as
 * native C charged by the original's blocks (engine.h; jp_E7B0.c says
 * more).
 *
 * jp builds the scroller's line in its u16 text (into D_802156A0, which
 * D_802158A0 then points at) with func_8025B5D4/func_8025B918 where the US
 * versions sprintf into D_802155A0.
 */
#include "engine.h"
#include "game/frontend.h"
#include "game/game.h"
#include "game/player.h"
#include "game/level.h"

#ifdef VERSION_JP

typedef struct {
    /* 0x00 */ char *PTR32 unk0[4];
} UnkStruct_80208358; /* size = 0x10 */

extern UnkStruct_80208358 D_80208358;
extern UnkStruct_80208358 D_80208368;
extern char *PTR32 D_80208378[];
extern u16 D_802082D0_jp[];
extern u16 D_802082E4_jp[];
extern u16 D_802082F8_jp[];
extern u16 D_8020830C_jp[];
extern u16 D_80208320_jp[];
extern u16 D_8020832C_jp[];
extern u16 D_80208330_jp[];
extern u16 D_80208340_jp[];
extern u16 D_80208348_jp[];
extern u16 D_802156A0[];
extern u16 D_803041B8[];
extern u8 D_80365060[];
extern s32 D_80215458;
extern s16 D_802154D2;
extern s16 D_802154D4;
extern s32 D_802154DC;
extern f32 D_802154E0;
extern s32 D_802154EC;
extern s32 D_80215508[];
extern u8 D_802155A0[];
extern u16 *D_802158A0;
extern u8 D_8021592E;
extern u16 D_80215930[6];

/* the players' menu's scroller for player arg0 (4: none): its name, the
   medals, its money and rank; arg1 or no player, the faster scroll */
void func_801E8EB8(u8 arg0, u8 arg1) {
    PlayerInfo *player = &D_80364AF0[0] + arg0;
    u8 other = 0;
    s32 i;
    u16 *t;

    ENGINE_BLK(801E8EB8);
    ENGINE_BLK(801E8F64);
    if (!((u32)D_80364A90 & 0x10E18000)) {
        ENGINE_BLK(801E8F6C);
        if (arg0 != D_80364AEA) {
            ENGINE_BLK(801E8F80);
            other = 1;
        }
    }
    ENGINE_BLK(801E8F88);
    D_802154EC = -1;
    i = 1;
    do {
        ENGINE_BLK(801E8F9C);
        D_80215508[i] = -1;
        i++;
    } while (i < 5);
    ENGINE_BLK(801E8FC8);
    if (arg0 < 4) {
        ENGINE_BLK(801E8FD8);
        func_801E93DC(arg0);
    }
    ENGINE_BLK(801E8FE0);
    if ((u32)(D_80364A98 >> 32) == 0x10000) {
        ENGINE_BLK(801E8FF8);
        if ((u32)D_80364A98 == 0) {
            ENGINE_BLK(801E9000);
            /* the original reads its stack copies of D_80208368 and,
               above them, D_80208358 (sp24, sp34): arg0 4 (no player)
               gets D_80208358's first, a char string */
            D_802158A0 = (u16 *)(arg0 < 4 ? D_80208368.unk0[arg0] : D_80208358.unk0[arg0 - 4]);
            goto done;
        }
    }
    ENGINE_BLK(801E901C);
    D_802158A0 = D_802156A0;
    if (arg0 >= 4) {
        ENGINE_BLK(801E92DC);
        t = func_8025B7AC((u8 *)" ");
        ENGINE_BLK(801E92E8);
        func_8025B5D4(D_802158A0, D_8020832C_jp, t, 0);
        goto done;
    }
    ENGINE_BLK(801E903C);
    if (D_80365060[arg0] != 1) {
        ENGINE_BLK(801E92A8);
        t = func_8025B7AC((u8 *)" ... NEW GAME");
        ENGINE_BLK(801E92B4);
        func_8025B5D4(D_802158A0, D_8020832C_jp, t, 0);
        ENGINE_BLK(801E92D4);
        goto done;
    }
    ENGINE_BLK(801E9054);
    func_8025B5D4(D_802156A0, D_802082D0_jp, (u16 *)D_80208378[other], 0);
    ENGINE_BLK(801E907C);
    t = func_8025B7AC((u8 *)player);
    ENGINE_BLK(801E9084);
    func_8025B918(D_802158A0, t);
    ENGINE_BLK(801E9098);
    func_8025B918(D_802158A0, D_80208340_jp);
    ENGINE_BLK(801E90AC);
    func_8025B918(D_802158A0, (u16 *)D_802081C0[player->unkC][1]);
    ENGINE_BLK(801E90D0);
    func_8025B918(D_802158A0, D_80208330_jp);
    ENGINE_BLK(801E90E4);
    if (D_80364AF0[arg0].gameState >= 12) {
        ENGINE_BLK(801E9104);
        i = 4;
    } else {
        ENGINE_BLK(801E9110);
        i = 3;
    }
    ENGINE_BLK(801E9118);
    if (i > 0) {
        do {
            s32 len;

            ENGINE_BLK(801E9124);
            len = func_8025B370(D_802158A0);
            ENGINE_BLK(801E9130);
            D_80215508[i] = len;
            if (i != 1) {
                ENGINE_BLK(801E9154);
                func_8025B5D4(D_802158A0, D_802082E4_jp, D_802158A0, D_80215930[i]);
            }
            ENGINE_BLK(801E9180);
            i--;
        } while (i > 0);
    }
    ENGINE_BLK(801E9190);
    func_8025B5D4(D_802158A0, D_802082F8_jp, D_802158A0, D_80215930[1]);
    ENGINE_BLK(801E91B8);
    if (D_802E8BF8 == 0) {
        ENGINE_BLK(801E91C8);
        func_8025B5D4(D_802158A0, D_8020830C_jp, D_802158A0, player->unk14);
    }
    ENGINE_BLK(801E91EC);
    i = func_8025B370(D_802158A0);
    ENGINE_BLK(801E91F8);
    D_802154EC = i;
    func_8025B5D4(D_802158A0, D_80208320_jp, D_802158A0, player->unkC);
    ENGINE_BLK(801E9224);
    if (!((u32)(D_80364A98 >> 32) & 0x02000400)) {
        ENGINE_BLK(801E9248);
        ENGINE_BLK(801E9250);
        if (!((u32)(D_80364A90 >> 32) & 0x01000000)) {
            ENGINE_BLK(801E9270);
            goto done;
        }
    }
    ENGINE_BLK(801E9278);
    func_8025B918(D_802158A0, D_80208348_jp);
    ENGINE_BLK(801E928C);
    func_8025B918(D_802158A0, D_803041B8);
    ENGINE_BLK(801E92A0);
done:
    ENGINE_BLK(801E9308);
    if (D_802158A0 != NULL) {
        ENGINE_BLK(801E9318);
        i = func_8025B370(D_802158A0);
        ENGINE_BLK(801E9320);
        D_802154D2 = i;
        D_8021592E = 1;
        D_80215458 = 19;
        if (arg0 == 4) {
            ENGINE_BLK(801E9350);
            D_802154D4 = 22;
        } else {
            ENGINE_BLK(801E9360);
            D_802154D4 = 14;
        }
    } else {
        ENGINE_BLK(801E9370);
        i = func_8025B300(D_802155A0);
        ENGINE_BLK(801E937C);
        D_802154D2 = i;
        D_8021592E = 0;
        D_80215458 = 12;
        if (arg0 == 4) {
            ENGINE_BLK(801E93A8);
            D_802154D4 = 36;
        } else {
            ENGINE_BLK(801E93B8);
            D_802154D4 = 22;
        }
    }
    ENGINE_BLK(801E93C4);
    if (D_802154D2 >= 0x100) {
        ENGINE_BLK(801E93D8);
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sslen<TOTAL_SCROLL_LENGTH", "player.c", 400);
    }
    ENGINE_BLK(801E93F8);
    D_802154DC = -1;
    if (arg1 == 0) {
        ENGINE_BLK(801E940C);
        if (arg0 != 4) {
            ENGINE_BLK(801E942C);
            D_802154E0 = 8.0f;
            goto out;
        }
    }
    ENGINE_BLK(801E941C);
    D_802154E0 = 12.0f;
out:
    ENGINE_BLK(801E943C);
}

extern char D_80215470[];
extern u8 D_80215902[];
extern s16 D_80215910[];
extern u8 D_80215914;
extern u8 D_80364B81[][0x100];

/* the medal screen's cups (two players: the two of them) and, for one, the
   time to beat for the next medal, with its shadow */
Gfx *func_801EC770(Gfx *arg0, s32 arg1, s32 *arg2) {
    Gfx *gfx = arg0;
    u8 medal;
    s32 a, b, h;

    ENGINE_BLK(801EC770);
    gfx = func_80274868(gfx);
    ENGINE_BLK(801EC79C);
    if (D_80364AA8 == 1) {
        ENGINE_BLK(801EC7B4);
        gfx = func_801EC49C(gfx, 0x5C, 0x68, 0);
        ENGINE_BLK(801EC7C8);
        if (D_80215902[1] != 0) {
            ENGINE_BLK(801EC7E0);
            gfx = func_801EC49C(gfx, 0xA8, 0x68, 1);
            ENGINE_BLK(801EC7F4);
        }
    } else {
        ENGINE_BLK(801EC7FC);
        if (D_80215902[1] == 5) {
            ENGINE_BLK(801EC814);
            medal = 1;
        } else {
            ENGINE_BLK(801EC820);
            if (D_80364B81[D_80364AE8][0] < 12) {
                ENGINE_BLK(801EC84C);
                a = 3;
            } else {
                ENGINE_BLK(801EC844);
                a = 4;
            }
            ENGINE_BLK(801EC850);
            if (a < D_80215902[1] + 1) {
                ENGINE_BLK(801EC86C);
                if (D_80364B81[D_80364AE8][0] < 12) {
                    ENGINE_BLK(801EC898);
                    b = 3;
                } else {
                    ENGINE_BLK(801EC890);
                    b = 4;
                }
                ENGINE_BLK(801EC89C);
                medal = b;
            } else {
                ENGINE_BLK(801EC8A4);
                medal = D_80215902[1] + 1;
            }
        }
        ENGINE_BLK(801EC8B8);
        gfx = func_801EC49C(gfx, 0x82, 0x68, 1);
        ENGINE_BLK(801EC8CC);
        h = D_80215910[1] * 3;
        if (h < 0)
            ENGINE_BLK(801EC90C);
        ENGINE_BLK(801EC914);
        gfx = func_80272ED8(gfx, medal + D_80215914, 0x2E, 0x6C, h / 4, 0, 0.75f);
        ENGINE_BLK(801EC928);
    }
    ENGINE_BLK(801EC92C);
    gfx = func_80274AA4(gfx);
    ENGINE_BLK(801EC934);
    if (D_80364AA8 != 1) {
        ENGINE_BLK(801EC94C);
        func_80264A34(D_80215470, D_802E8F94[D_802E8BDC].medalTimes[4 - medal], 0);
        ENGINE_BLK(801EC98C);
        D_80215470[5] = 0;
        if (D_80215910[1] < 0)
            ENGINE_BLK(801EC9F8);
        ENGINE_BLK(801ECA00);
        func_80259CCC(arg1, D_80215470, NULL, 0, 0, 0x27, 0x7E, 0x10, 0x10, 1, 0, 0, 0, D_80215910[1] / 2);
        ENGINE_BLK(801ECA08);
        func_80259DC8(arg1, D_80215470, NULL, 0, 0, 0x29, 0x7D, 0x10, 0x10, 1, 0xFF, 0xB4, 0, D_80215910[1], 0xFF,
                      0x78, 0, D_80215910[1]);
    }
    ENGINE_BLK(801ECA88);
    *arg2 += gfx - arg0;
    return gfx;
}

#endif
