/*
 * hd_front_end 1C40 (player.c: the players' menu and its scroller), jp's
 * func_801E8EB8 and func_801EC770, which the C has as jp's GLOBAL_ASM, as
 * native C (engine.h; jp_E7B0.c says
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
    /* 0x00 */ char *unk0[4];
} UnkStruct_80208358; /* size = 0x10 */

extern UnkStruct_80208358 D_80208358;
extern UnkStruct_80208358 D_80208368;
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
extern u8 D_8021592E;
extern u16 D_80215930[6];

/* the players' menu's scroller for player arg0 (4: none): its name, the
   medals, its money and rank; arg1 or no player, the faster scroll */
void func_801E8EB8(u8 arg0, u8 arg1) {
    PlayerInfo *player = &D_80364AF0[0] + arg0;
    u8 other = 0;
    s32 i;
    u16 *t;

    if (!((u32)D_80364A90 & 0x10E18000)) {
        if (arg0 != D_80364AEA) {
            other = 1;
        }
    }
    D_802154EC = -1;
    i = 1;
    do {
        D_80215508[i] = -1;
        i++;
    } while (i < 5);
    if (arg0 < 4) {
        func_801E93DC(arg0);
    }
    if ((u32)(D_80364A98 >> 32) == 0x10000) {
        if ((u32)D_80364A98 == 0) {
            /* the original reads its stack copies of D_80208368 and,
               above them, D_80208358 (sp24, sp34): arg0 4 (no player)
               gets D_80208358's first, a char string */
            D_802158A0 = (u16 *)(arg0 < 4 ? D_80208368.unk0[arg0] : D_80208358.unk0[arg0 - 4]);
            goto done;
        }
    }
    D_802158A0 = D_802156A0;
    if (arg0 >= 4) {
        t = func_8025B7AC((u8 *)" ");
        func_8025B5D4(D_802158A0, D_8020832C_jp, t, 0);
        goto done;
    }
    if (D_80365060[arg0] != 1) {
        t = func_8025B7AC((u8 *)" ... NEW GAME");
        func_8025B5D4(D_802158A0, D_8020832C_jp, t, 0);
        goto done;
    }
    func_8025B5D4(D_802156A0, D_802082D0_jp, (u16 *)D_80208378[other], 0);
    t = func_8025B7AC((u8 *)player);
    func_8025B918(D_802158A0, t);
    func_8025B918(D_802158A0, D_80208340_jp);
    func_8025B918(D_802158A0, (u16 *)D_802081C0[player->unkC][1]);
    func_8025B918(D_802158A0, D_80208330_jp);
    if (D_80364AF0[arg0].gameState >= 12) {
        i = 4;
    } else {
        i = 3;
    }
    if (i > 0) {
        do {
            s32 len;

            len = func_8025B370(D_802158A0);
            D_80215508[i] = len;
            if (i != 1) {
                func_8025B5D4(D_802158A0, D_802082E4_jp, D_802158A0, D_80215930[i]);
            }
            i--;
        } while (i > 0);
    }
    func_8025B5D4(D_802158A0, D_802082F8_jp, D_802158A0, D_80215930[1]);
    if (D_802E8BF8 == 0) {
        func_8025B5D4(D_802158A0, D_8020830C_jp, D_802158A0, player->unk14);
    }
    i = func_8025B370(D_802158A0);
    D_802154EC = i;
    func_8025B5D4(D_802158A0, D_80208320_jp, D_802158A0, player->unkC);
    if (!((u32)(D_80364A98 >> 32) & 0x02000400)) {
        if (!((u32)(D_80364A90 >> 32) & 0x01000000)) {
            goto done;
        }
    }
    func_8025B918(D_802158A0, D_80208348_jp);
    func_8025B918(D_802158A0, D_803041B8);
done:
    if (D_802158A0 != NULL) {
        i = func_8025B370(D_802158A0);
        D_802154D2 = i;
        D_8021592E = 1;
        D_80215458 = 19;
        if (arg0 == 4) {
            D_802154D4 = 22;
        } else {
            D_802154D4 = 14;
        }
    } else {
        i = func_8025B300(D_802155A0);
        D_802154D2 = i;
        D_8021592E = 0;
        D_80215458 = 12;
        if (arg0 == 4) {
            D_802154D4 = 36;
        } else {
            D_802154D4 = 22;
        }
    }
    if (D_802154D2 >= 0x100) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sslen<TOTAL_SCROLL_LENGTH", "player.c", 400);
    }
    D_802154DC = -1;
    if (arg1 == 0) {
        if (arg0 != 4) {
            D_802154E0 = 8.0f;
            goto out;
        }
    }
    D_802154E0 = 12.0f;
out:
}

extern char D_80215470[];
extern u8 D_80215902[];
extern s16 D_80215910[];
extern u8 D_80215914;

/* the medal screen's cups (two players: the two of them) and, for one, the
   time to beat for the next medal, with its shadow */
Gfx *func_801EC770(Gfx *arg0, union Frame *arg1, s32 *arg2) {
    Gfx *gfx = arg0;
    u8 medal;
    s32 a, b, h;

    gfx = func_80274868(gfx);
    if (D_80364AA8 == 1) {
        gfx = func_801EC49C(gfx, 0x5C, 0x68, 0);
        if (D_80215902[1] != 0) {
            gfx = func_801EC49C(gfx, 0xA8, 0x68, 1);
        }
    } else {
        if (D_80215902[1] == 5) {
            medal = 1;
        } else {
            if (D_80364AF0[D_80364AE8].gameState < 12) {
                a = 3;
            } else {
                a = 4;
            }
            if (a < D_80215902[1] + 1) {
                if (D_80364AF0[D_80364AE8].gameState < 12) {
                    b = 3;
                } else {
                    b = 4;
                }
                medal = b;
            } else {
                medal = D_80215902[1] + 1;
            }
        }
        gfx = func_801EC49C(gfx, 0x82, 0x68, 1);
        h = D_80215910[1] * 3;
        gfx = func_80272ED8(gfx, medal + D_80215914, 0x2E, 0x6C, h / 4, 0, 0.75f);
    }
    gfx = func_80274AA4(gfx);
    if (D_80364AA8 != 1) {
        func_80264A34(D_80215470, D_802E8F94[D_802E8BDC].medalTimes[4 - medal], 0);
        D_80215470[5] = 0;
        func_80259CCC(arg1, D_80215470, NULL, 0, 0, 0x27, 0x7E, 0x10, 0x10, 1, 0, 0, 0, D_80215910[1] / 2);
        func_80259DC8(arg1, D_80215470, NULL, 0, 0, 0x29, 0x7D, 0x10, 0x10, 1, 0xFF, 0xB4, 0, D_80215910[1], 0xFF,
                      0x78, 0, D_80215910[1]);
    }
    *arg2 += gfx - arg0;
    return gfx;
}

#endif
