/*
 * hd_code 77E20 (us.v11 0x802BC5E0-0x802C20A0): the buildings and their
 * destruction, as native C (engine.h, buildings.h).
 *
 * The debris: 30 records of 0x38 bytes at D_803F3968, free while byte
 * 0x34 is set; D_803F3FF8 is the one being made.
 */
#include "buildings.h"
#include "game/game.h"

extern u8 D_803F3968[30][0x38];         /* the debris */

/* all the debris free */
REGS()
void func_802C049C(void) {
    s32 n;
    u8 *d = D_803F3968[0];

    ENGINE_BLK(802C049C);
    for (n = 30;; n--) {
        ENGINE_BLK(802C04C0);
        if (n == 0)
            break;
        ENGINE_BLK(802C04C8);
        d[0x34] = 1;
        d += 0x38;
    }
    ENGINE_BLK(802C04D8);
}

/* debris record `src` copied into the first free one (none if none) */
REGS(v1)
void func_802C04F0(u8 *src) {
    s32 n;
    u8 *d = D_803F3968[0];

    ENGINE_BLK(802C04F0);
    for (n = 30;; n--) {
        ENGINE_BLK(802C0514);
        if (n == 0)
            goto out;
        ENGINE_BLK(802C051C);
        if (d[0x34] != 0)
            break;
        ENGINE_BLK(802C0528);
        d += 0x38;
    }
    ENGINE_BLK(802C0534);
    for (n = 0x38;; n -= 4) {
        ENGINE_BLK(802C0538);
        if (n == 0)
            break;
        ENGINE_BLK(802C0540);
        *(u32 *)d = *(u32 *)src;
        src += 4;
        d += 4;
    }
out:
    ENGINE_BLK(802C0558);
}
