/*
 * hd_front_end 1B100 (us.v11 0x80202100-0x8020272C): the front end's
 * vehicle models, through hd_code's model loader and part helpers (5CB60,
 * 56040), as native C (engine.h).  The front end's C calls them: to load a
 * vehicle's model with its two part buffers and display lists
 * (func_80202100), set its parts up (func_80202270, func_802022EC,
 * func_80202380), turn them (func_802025D0) and step them a frame
 * (func_802021FC).
 */
#include "shared.h"
#include "game/game.h"
#include "game/model.h"
#include "game/vehicle.h"



/* each of a front end vehicle's two part buffers */
#define PART_BUF_SIZE 0x1770

/* func_802025D0's angles: 12-bit, a full turn 0x1000 */
#define ANGLE_HALF_TURN 0x800
#define TRUCK_PART_STEP 0x55    /* the truck's parts: a step per 0x55 */
#define DOZER_ANGLE_LO 0x355    /* the bulldozer's turning part: from here, */
#define DOZER_ANGLE_HI 0x8AB    /* to here (then 0 again), */
#define DOZER_PART_STEP 0x4C    /* a step per 0x4C */
#define DOZER_FIXED_PART 0x32   /* its two others' setting */

/* 5CB60: model n loaded; a vehicle's record from its model */
REGS(t3 -> s2)
u8 *func_802A396C(u32 type);
REGS(a0, a1, v0, v1, s2)
void func_802A1388(s32 type, s32 a1, u8 *buf1, u8 *buf2, u8 *model);
/* 56040 */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2);
REGS(t0, v0, v1)
void func_8029E558(Part *parts, u8 *buf, u8 *other);

/* the moving parts' keys: the truck's two, the bulldozer's three */
extern u8 D_802C2208[], D_802C226C[], D_802C2190[], D_802C21A4[], D_802C21B8[];

/* Vehicle model `type` loaded: the model into *rec (the front end's
   pointer to it, whatever its type there), two PART_BUF_SIZE part buffers
   from the heap into bufs[0..1], and the model's two display list sections
   (Model.unk1C, unk2C) into dls[0] and dls[2] with their places in the
   second buffer (dls[1], dls[3]); then its Vehicle record (5CB60). */
void func_80202100(s32 type, void *rec, u8 **bufs, Gfx **dls) {
    Model *model;
    u8 *heap;
    s32 o1, o2;

    model = (Model *)func_802A396C(type);
    *(Model **)rec = model;
    heap = D_80358070;
    bufs[0] = heap;
    bufs[1] = heap + PART_BUF_SIZE;
    heap += PART_BUF_SIZE * 2;
    D_80358070 = heap;
    o1 = (s32)model->unk1C;
    o2 = (s32)model->unk2C;
    dls[0] = (Gfx *)((u8 *)model + o1);
    dls[1] = (Gfx *)heap;
    dls[2] = (Gfx *)((u8 *)model + o2);
    dls[3] = (Gfx *)(heap + o2 - o1);
    func_802A1388(type, 0, bufs[0], bufs[1], (u8 *)model);
}

/* the parts stepped a frame (from the buffer `buf`, `other` the next) */
void func_802021FC(Part *parts, u8 *buf, u8 *other) {
    func_8029E558(parts, buf, other);
}

/* the parts set up from the model in the two buffers */
void func_80202270(void *model, u8 **bufs, Part *parts) {
    func_8029F85C(parts, model, bufs[0], bufs[1]);
}

/* part i's settings */
void func_802022EC(Part *parts, s32 i, s32 a, s32 b, f32 f, s32 c, s32 d) {
    func_802A039C(i, c, parts);
    func_802A03D4(i, d, parts);
    func_802A040C(i, a, parts);
    func_802A0480(i, b, parts, f);
    func_802A0290(i, -1, parts);
}

/* one part's four settings (func_802A05D0... by its key) */
static void part_reset(u8 *key) {
    func_802A05D0(key, 0);
    func_802A05F8(key, 0);
    func_802A0620(key, 0);
    func_802A0508(key, -1);
}

/* the front end's vehicle `type`'s moving parts at rest (the truck's two,
   the bulldozer's three) */
void func_80202380(s32 type) {
    if (type == VEHICLE_TRUCK) {
        part_reset(D_802C2208);
        part_reset(D_802C226C);
    } else if (type == VEHICLE_BULLDOZER) {
        part_reset(D_802C2190);
        part_reset(D_802C21A4);
        part_reset(D_802C21B8);
    }
}

/* the front end's vehicle `type`'s moving parts turned for its angle a:
   the truck's two by a / TRUCK_PART_STEP (each half turn the same); the
   bulldozer's one by (a - DOZER_ANGLE_LO) / DOZER_PART_STEP between
   DOZER_ANGLE_LO and DOZER_ANGLE_HI (0 elsewhere), its two others fixed */
void func_802025D0(u8 type, u32 a) {
    u32 s;

    if (type == VEHICLE_TRUCK) {
        s = a;
        if ((s32)s >= ANGLE_HALF_TURN) {
            s -= ANGLE_HALF_TURN;
        }
        s /= TRUCK_PART_STEP;
        func_802A05A4(D_802C2208, s, 0.0f);
        func_802A05A4(D_802C226C, s, 0.0f);
    } else {
        if (type == VEHICLE_BULLDOZER) {
            s = a;
            if ((s32)s < DOZER_ANGLE_LO) {
                s = 0;
            } else {
                if ((s32)s >= DOZER_ANGLE_HI) {
                    s = 0;
                } else {
                    s -= DOZER_ANGLE_LO;
                }
            }
            s /= DOZER_PART_STEP;
            func_802A05A4(D_802C21B8, s, 0.0f);
            func_802A05D0(D_802C2190, DOZER_FIXED_PART);
            func_802A05D0(D_802C21A4, DOZER_FIXED_PART);
        }
    }
}
