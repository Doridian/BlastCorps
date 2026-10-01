/*
 * hd_code 60D50 (us.v11 0x802A5510-0x802A5720): the level's height boxes
 * (LevelHeader.bounds40, bounds44: 10-byte records of x1, z1, x2, z2 and a
 * height, in world units) and the per-level heights, as native C
 * (engine.h).
 */
#include "engine.h"
#include "game/level.h"
#include "game/game.h"
#include "game/vehicle.h"

typedef struct HeightBox {
    s16 x1, z1, x2, z2;
    s16 height;
} HeightBox;

extern u8 D_80364411;           /* the player is over no box (height 3000) */
extern s16 D_8036444C;
extern s16 D_8036444E;          /* the box's height << 5, plus D_80364450 */
extern u16 D_80364450;
extern u8 D_803BE73A;           /* the vehicle the player starts in */
extern s32 D_803EF304;
extern s16 D_80305B90[][3];     /* by D_803BE73A */

#define BOX_NEXT(b) ((HeightBox *)((u8 *)(b) + 10))

/* func_802A5510 (00000.c's): the highest box over the player */
void func_802A5510(LevelHeader *h) {
    HeightBox *b = (HeightBox *)((u8 *)h + h->bounds40);
    HeightBox *end = (HeightBox *)((u8 *)h + h->bounds44);
    s32 x = (u32)D_803643E0 >> 5, z = (u32)D_803643E8 >> 5;
    s32 best = -1;

    ENGINE_BLK(802A5510);
    for (;;) {
        ENGINE_BLK(802A554C);
        if (b == end) {
            break;
        }
        ENGINE_BLK(802A5554);
        if (b->x1 <= x) {
            ENGINE_BLK(802A5564);
            if (b->z1 <= z) {
                ENGINE_BLK(802A5574);
                if (b->x2 >= x) {
                    ENGINE_BLK(802A5584);
                    if (b->z2 >= z) {
                        ENGINE_BLK(802A5594);
                        if (b->height >= best) {
                            ENGINE_BLK(802A55A4);
                            best = b->height;
                        }
                    }
                }
            }
        }
        ENGINE_BLK(802A55A8);
        b = BOX_NEXT(b);
    }
    ENGINE_BLK(802A55B0);
    if (best == 3000) {
        ENGINE_BLK(802A55BC);
        D_80364411 = 1;
    } else {
        ENGINE_BLK(802A55CC);
        D_8036444E = (best << 5) + D_80364450;
        D_80364411 = 0;
    }
    ENGINE_BLK(802A55F4);
}

/* func_802A5604 (72B80's): the same for the position at D_803EF2EC, from
   bounds44's boxes, into D_803EF304.  Its caller reads what it left in $v0
   (the last height it looked at), $t4 (the last coordinate), $t5 and
   $t6. */
void func_802A5604(LevelHeader *h) {
    HeightBox *b = (HeightBox *)((u8 *)h + h->bounds44);
    HeightBox *end = (HeightBox *)((u8 *)h + h->unk48);
    s32 x = (u32)(&D_803EF2EC)[0] >> 5, z = (u32)(&D_803EF2EC)[2] >> 5;
    s32 best = -1;
    u32 t4 = (u32)&(&D_803EF2EC)[2];

    ENGINE_BLK(802A5604);
    for (;;) {
        ENGINE_BLK(802A5640);
        if (b == end) {
            break;
        }
        ENGINE_BLK(802A5648);
        t4 = b->x1;
        if (b->x1 <= x) {
            ENGINE_BLK(802A5658);
            t4 = b->z1;
            if (b->z1 <= z) {
                ENGINE_BLK(802A5668);
                t4 = b->x2;
                if (b->x2 >= x) {
                    ENGINE_BLK(802A5678);
                    t4 = b->z2;
                    if (b->z2 >= z) {
                        ENGINE_BLK(802A5688);
                        ENGINE_LEAVE(2, b->height);
                        if (b->height >= best) {
                            ENGINE_BLK(802A5698);
                            best = b->height;
                        }
                    }
                }
            }
        }
        ENGINE_BLK(802A569C);
        b = BOX_NEXT(b);
    }
    ENGINE_BLK(802A56A4);
    D_803EF304 = best << 5;
    ENGINE_LEAVE(12, t4);
    ENGINE_LEAVE(13, (u32)end);
    ENGINE_LEAVE(14, best << 5);
}

/* func_802A56C4 (00000.c's): this level's three values; returns the third */
s32 func_802A56C4(void) {
    s16 *v = D_80305B90[D_803BE73A];

    ENGINE_BLK(802A56C4);
    D_8036444C = v[0];
    D_80364450 = v[1];
    return v[2];
}
