/* hd_code 60D50: the height boxes of the level (LevelHeader.bounds40,
   bounds44: 10-byte records of x1, z1, x2, z2, height) and the per-level
   heights. */
#include "engine_b.h"
#include "engine_b_types.h"
#include "game/vehicle.h"

typedef struct HeightBox {
    s16 x1, z1, x2, z2;
    s16 height;
} HeightBox;

extern u8 D_80364411;           /* the player is over no box (3000) */
extern s16 D_8036444C;
extern s16 D_8036444E;          /* the box's height << 5, plus D_80364450 */
extern u16 D_80364450;
extern u8 D_803BE73A;           /* an index into D_80305B90 */
extern s32 D_803EF304;
extern s16 D_80305B90[][3];

/* the highest box over (x, z) of [box, end), or -1 */
static s32 height_at(const HeightBox *box, const HeightBox *end, s32 x, s32 z) {
    s32 best = -1;

    for (;;) {
        ENG_COST(2);
        if (box == end) {
            break;
        }
        ENG_COST(4);
        if (box->x1 <= x) {
            ENG_COST(4);
            if (box->z1 <= z) {
                ENG_COST(4);
                if (box->x2 >= x) {
                    ENG_COST(4);
                    if (box->z2 >= z) {
                        ENG_COST(4);
                        if (box->height >= best) {
                            ENG_COST(1);
                            best = box->height;
                        }
                    }
                }
            }
        }
        ENG_COST(2);
        box = (const HeightBox *)((const u8 *)box + 10);
    }
    return best;
}

/* func_802A5510: the player's box (hd.c) */
void func_802A5510(LevelHeader *h) {
    s32 best;

    ENG_COST(15);
    best = height_at((const HeightBox *)((u8 *)h + h->bounds40),
                     (const HeightBox *)((u8 *)h + h->bounds44),
                     (u32)D_803643E0 >> 5, (u32)D_803643E8 >> 5);
    ENG_COST(3);
    if (best == 3000) {
        ENG_COST(4);
        D_80364411 = 1;
    } else {
        ENG_COST(10);
        D_8036444E = (best << 5) + D_80364450;
        D_80364411 = 0;
    }
    ENG_COST(4);
}

/* func_802A5604: the same for the position at D_803EF2EC (72B80's
   hotrod), from bounds44's boxes, into D_803EF304 */
void func_802A5604(LevelHeader *h) {
    s32 best;

    ENG_COST(15);
    best = height_at((const HeightBox *)((u8 *)h + h->bounds44),
                     (const HeightBox *)((u8 *)h + h->unk48),
                     (u32)D_803EF2EC >> 5, (u32)(&D_803EF2EC)[2] >> 5);
    ENG_COST(8);
    D_803EF304 = best << 5;
}

/* func_802A56C4: this level's three values; returns the third */
s32 func_802A56C4(void) {
    s16 *v = D_80305B90[D_803BE73A];

    ENG_COST(20);
    D_8036444C = v[0];
    D_80364450 = v[1];
    return v[2];
}
