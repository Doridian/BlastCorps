/*
 * hd_code 41930, jp's func_802860F0, which the C has as jp's GLOBAL_ASM, as
 * native C (engine.h; jp_E7B0.c says more).
 */
#include "engine.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

#ifdef VERSION_JP

typedef struct {
    /* 0x00 */ char *PTR32 unk0;     /* a level's name */
    /* 0x04 */ u16 *PTR32 unk4;     /* jp: the same in its u16 text */
    /* 0x08 */ u8 unk8[0x28];
} UnkStruct_8020D7E4_jp; /* size = 0x30 */

extern UnkStruct_8020D7E4_jp D_8020D7E4[];
extern u8 D_80364B80[][0x100];
extern u8 D_80364B81[][0x100];
extern u8 D_802FDA60[0x10];
extern char D_8036EBA0[0x60];
extern u16 D_80301260[];

/* a level's start: the music and, for the kind of level it is, its
   message (a race's level not yet done: "IN <level>.", in the u16 text
   too at D_8036EBA0 + 0x20) */
void func_802860F0(void) {
    s32 kind, i, j, found;
    u8 open;

    if (D_80364B81[D_80364AE8][0] == 0xD)
        goto out;
    if (D_80364B81[D_80364AE8][0] == 8)
        goto out;
    if (D_80364B81[D_80364AE8][0] == 1)
        goto out;
    D_80364A98 = 0x800000000000;
    func_80255DC8();
    func_80200714(D_802FDA60[D_80364B81[D_80364AE8][0]]);
    kind = D_80364B81[D_80364AE8][0];
    if (kind == 4) {
        open = 0;
        i = 0;
        do {
            j = 0;
            found = 0;
            do {
                if (D_802E8F38[j].level == i) {
                    found = 1;
                    if (!(D_80364B80[D_80364AE8][0] & (1 << j))) {
                        open = 1;
                    }
                }
                j++;
                if (j >= 6)
                    break;
            } while (found == 0);
            i++;
            if (i >= 0x3C)
                break;
        } while (open == 0);
        sprintf(D_8036EBA0, "IN %s.", D_8020D7E4[i].unk0);
        D_8020C070[80].text = D_8036EBA0;
        func_8025B5D4((u16 *)(D_8036EBA0 + 0x20), D_80301260, D_8020D7E4[i].unk4, 0);
        D_8020C070[77].unk10 = (u16 *)(D_8036EBA0 + 0x20);
    } else {
        if (kind == 6) {
            func_801ECC8C();
        }
    }
    func_8026AF6C((D_80364B81[D_80364AE8][0] + 0x16) | 0x8000);
out:
}

#endif
