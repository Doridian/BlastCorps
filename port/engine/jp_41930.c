/*
 * hd_code 41930, jp's func_802860F0, which the C has as jp's GLOBAL_ASM, as
 * native C charged by the original's blocks (engine.h; jp_E7B0.c says more).
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

    ENGINE_BLK(802860F0);
    if (D_80364B81[D_80364AE8][0] == 0xD)
        goto out;
    ENGINE_BLK(8028611C);
    if (D_80364B81[D_80364AE8][0] == 8)
        goto out;
    ENGINE_BLK(80286128);
    if (D_80364B81[D_80364AE8][0] == 1)
        goto out;
    ENGINE_BLK(80286130);
    D_80364A98 = 0x800000000000;
    func_80255DC8();
    ENGINE_BLK(8028614C);
    func_80200714(D_802FDA60[D_80364B81[D_80364AE8][0]]);
    ENGINE_BLK(80286174);
    kind = D_80364B81[D_80364AE8][0];
    if (kind == 4) {
        ENGINE_BLK(802861A8);
        open = 0;
        i = 0;
        do {
            ENGINE_BLK(802861B0);
            j = 0;
            found = 0;
            do {
                ENGINE_BLK(802861B8);
                if (D_802E8F38[j].level == i) {
                    ENGINE_BLK(802861D8);
                    found = 1;
                    if (!(D_80364B80[D_80364AE8][0] & (1 << j))) {
                        ENGINE_BLK(80286208);
                        open = 1;
                    }
                }
                ENGINE_BLK(80286210);
                j++;
                if (j >= 6)
                    break;
                ENGINE_BLK(80286224);
            } while (found == 0);
            ENGINE_BLK(80286230);
            i++;
            if (i >= 0x3C)
                break;
            ENGINE_BLK(80286244);
        } while (open == 0);
        ENGINE_BLK(80286250);
        engine_sprintf(D_8036EBA0, "IN %s.", D_8020D7E4[i].unk0);
        ENGINE_BLK(80286280);
        D_8020C070[80].text = D_8036EBA0;
        func_8025B5D4((u16 *)(D_8036EBA0 + 0x20), D_80301260, D_8020D7E4[i].unk4, 0);
        ENGINE_BLK(802862C8);
        D_8020C070[77].unk10 = (u16 *)(D_8036EBA0 + 0x20);
    } else {
        ENGINE_BLK(80286198);
        if (kind == 6) {
            ENGINE_BLK(802862E0);
            func_801ECC8C();
        } else {
            ENGINE_BLK(802861A0);
        }
    }
    ENGINE_BLK(802862E8);
    func_8026AF6C((D_80364B81[D_80364AE8][0] + 0x16) | 0x8000);
out:
    ENGINE_BLK(80286310);
}

#endif
