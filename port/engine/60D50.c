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
    /* 0x0 */ s16 x1, z1, x2, z2;
    /* 0x8 */ s16 height;
} HeightBox;
SIZE_CHECK(HeightBox, 10);

#define HEIGHT_NONE 3000        /* a box's height that means no ground */

extern u8 D_80364411;           /* the player is over no ground (HEIGHT_NONE) */
extern s16 D_8036444C;
extern s16 D_8036444E;          /* the box's height << 5, plus D_80364450 */
extern u16 D_80364450;
extern u8 D_803BE73A;           /* the vehicle the player starts in */
extern s32 D_803EF304;
extern s16 D_80305B90[][3];     /* by D_803BE73A */

/* the highest of the boxes [b, end) over (x, z) (world units), or -1.
   (Both callers charge func_802A5510's blocks: func_802A5604's are the
   same sizes.) */
static s32 highest_box(HeightBox *b, HeightBox *end, s32 x, s32 z) {
    s32 best = -1;

    for (; ENGINE_BLK(802A554C), b != end; b++) {
        ENGINE_BLK(802A5554);
        if (b->x1 <= x && (ENGINE_BLK(802A5564), b->z1 <= z) &&
            (ENGINE_BLK(802A5574), b->x2 >= x) && (ENGINE_BLK(802A5584), b->z2 >= z)) {
            ENGINE_BLK(802A5594);
            if (b->height >= best) {
                ENGINE_BLK(802A55A4);
                best = b->height;
            }
        }
        ENGINE_BLK(802A55A8);
    }
    return best;
}

/* func_802A5510 (00000.c's): the highest box over the player */
void func_802A5510(LevelHeader *h) {
    HeightBox *b = (HeightBox *)((u8 *)h + h->bounds40);
    HeightBox *end = (HeightBox *)((u8 *)h + h->bounds44);
    s32 x = (u32)D_803643E0 >> 5, z = (u32)D_803643E8 >> 5;
    s32 best;

    ENGINE_BLK(802A5510);
    best = highest_box(b, end, x, z);
    ENGINE_BLK(802A55B0);
    if (best == HEIGHT_NONE) {
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
   bounds44's boxes, into D_803EF304 */
void func_802A5604(LevelHeader *h) {
    HeightBox *b = (HeightBox *)((u8 *)h + h->bounds44);
    HeightBox *end = (HeightBox *)((u8 *)h + h->unk48);
    s32 x = (u32)(&D_803EF2EC)[0] >> 5, z = (u32)(&D_803EF2EC)[2] >> 5;

    ENGINE_BLK(802A5604);
    D_803EF304 = highest_box(b, end, x, z) << 5;
    ENGINE_BLK(802A56A4);
}

/* func_802A56C4 (00000.c's): this level's three values; returns the third */
s32 func_802A56C4(void) {
    s16 *v = D_80305B90[D_803BE73A];

    ENGINE_BLK(802A56C4);
    D_8036444C = v[0];
    D_80364450 = v[1];
    return v[2];
}
