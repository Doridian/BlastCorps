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


/* a block table, for code repeated with its own blocks */
typedef struct { u16 id, n; } Blk;
#define B(addr) {ENGINE_BLK_##addr}
#define BLK(t, i) ENGINE_BLK_((t)[i].id, (t)[i].n)

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
u8 *func_802A396C(s32 n);
REGS(a0, a1, v0, v1, s2)
void func_802A1388(s32 type, s32 a1, u8 *buf1, u8 *buf2, u8 *model);
/* 56040 */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2);
REGS(t0, v0, v1)
void func_8029E558(Part *parts, u8 *buf, u8 *other);

/* the moving parts' keys: the truck's two, the bulldozer's three */
extern u8 D_802C2208[], D_802C226C[], D_802C2190[], D_802C21A4[], D_802C21B8[];

/* Vehicle model `type` loaded: the model into *rec, two PART_BUF_SIZE
   part buffers from the heap into bufs[0..1], and the model's two display
   list sections (Model.unk1C, unk2C) into dls[0] and dls[2] with their
   places in the second buffer (dls[1], dls[3]); then its Vehicle record
   (5CB60). */
void func_80202100(s32 type, u32 *rec, u32 *bufs, u32 *dls) {
    Model *model;
    u8 *heap;
    s32 o1, o2;

    ENGINE_BLK(80202100);
    model = (Model *)func_802A396C(type);
    ENGINE_BLK(80202158);
    rec[0] = (u32)model;
    heap = D_80358070;
    bufs[0] = (u32)heap;
    bufs[1] = (u32)(heap + PART_BUF_SIZE);
    heap += PART_BUF_SIZE * 2;
    D_80358070 = heap;
    o1 = (s32)model->unk1C;
    o2 = (s32)model->unk2C;
    dls[0] = (u32)((u8 *)model + o1);
    dls[1] = (u32)heap;
    dls[2] = (u32)((u8 *)model + o2);
    dls[3] = (u32)(heap + o2 - o1);
    func_802A1388(type, 0, (u8 *)bufs[0], (u8 *)bufs[1], (u8 *)rec[0]);
    ENGINE_BLK(802021C8);
}

/* the parts stepped a frame (from the buffer `buf`, `other` the next) */
void func_802021FC(Part *parts, u8 *buf, u8 *other) {
    ENGINE_BLK(802021FC);
    func_8029E558(parts, buf, other);
    ENGINE_BLK(8020223C);
}

/* the parts set up from the model in the two buffers */
void func_80202270(u8 *model, u32 *bufs, Part *parts) {
    ENGINE_BLK(80202270);
    func_8029F85C(parts, model, (u8 *)bufs[0], (u8 *)bufs[1]);
    ENGINE_BLK(802022B8);
}

/* part i's settings */
void func_802022EC(Part *parts, s32 i, s32 a, s32 b, f32 f, s32 c, s32 d) {
    ENGINE_BLK(802022EC);
    func_802A039C(i, c, parts);
    ENGINE_BLK(80202328);
    func_802A03D4(i, d, parts);
    ENGINE_BLK(80202330);
    func_802A040C(i, a, parts);
    ENGINE_BLK(80202338);
    func_802A0480(i, b, parts, f);
    ENGINE_BLK(80202344);
    func_802A0290(i, -1, parts);
    ENGINE_BLK(8020234C);
}

/* one part's four settings (func_802A05D0... by its key), with the blocks
   of its copy `k` */
static void part_reset(u8 *key, s32 k) {
    static const Blk blks[][4] = {
        { B(802023CC), B(802023DC), B(802023EC), B(802023FC) },
        { B(8020240C), B(8020241C), B(8020242C), B(8020243C) },
        { B(80202454), B(80202464), B(80202474), B(80202484) },
        { B(80202494), B(802024A4), B(802024B4), B(802024C4) },
        { B(802024D4), B(802024E4), B(802024F4), B(80202504) },
    };

    BLK(blks[k], 0);
    func_802A05D0(key, 0);
    BLK(blks[k], 1);
    func_802A05F8(key, 0);
    BLK(blks[k], 2);
    func_802A0620(key, 0);
    BLK(blks[k], 3);
    func_802A0508(key, -1);
}

/* the front end's vehicle `type`'s moving parts at rest (the truck's two,
   the bulldozer's three) */
void func_80202380(s32 type) {
    u8 *last = NULL;

    ENGINE_BLK(80202380);
    if (type == VEHICLE_TRUCK) {
        part_reset(D_802C2208, 0);
        part_reset(D_802C226C, 1);
        last = D_802C226C;
        ENGINE_BLK(8020244C);
    } else {
        ENGINE_BLK(802023B8);
        if (type == VEHICLE_BULLDOZER) {
            part_reset(D_802C2190, 2);
            part_reset(D_802C21A4, 3);
            part_reset(D_802C21B8, 4);
            last = D_802C21B8;
            ENGINE_BLK(80202514);
        } else {
            ENGINE_BLK(802023C4);
        }
    }
    ENGINE_BLK(8020259C);
    if (last != NULL) {
    }
}

/* the front end's vehicle `type`'s moving parts turned for its angle a:
   the truck's two by a / TRUCK_PART_STEP (each half turn the same); the
   bulldozer's one by (a - DOZER_ANGLE_LO) / DOZER_PART_STEP between
   DOZER_ANGLE_LO and DOZER_ANGLE_HI (0 elsewhere), its two others fixed */
void func_802025D0(u8 type, u32 a) {
    u32 s;

    ENGINE_BLK(802025D0);
    if (type == VEHICLE_TRUCK) {
        ENGINE_BLK(8020261C);
        s = a;
        if ((s32)s >= ANGLE_HALF_TURN) {
            ENGINE_BLK(8020262C);
            s -= ANGLE_HALF_TURN;
        }
        ENGINE_BLK(80202630);
        s /= TRUCK_PART_STEP;
        ENGINE_BLK(80202654);
        func_802A05A4(D_802C2208, s, 0.0f);
        ENGINE_BLK(80202664);
        func_802A05A4(D_802C226C, s, 0.0f);
        ENGINE_BLK(80202678);
    } else {
        ENGINE_BLK(80202608);
        if (type == VEHICLE_BULLDOZER) {
            ENGINE_BLK(80202680);
            s = a;
            if ((s32)s < DOZER_ANGLE_LO) {
                s = 0;
                ENGINE_BLK(802026A0);
            } else {
                ENGINE_BLK(80202690);
                if ((s32)s >= DOZER_ANGLE_HI) {
                    s = 0;
                    ENGINE_BLK(802026A0);
                } else {
                    ENGINE_BLK(80202698);
                    s -= DOZER_ANGLE_LO;
                }
            }
            ENGINE_BLK(802026A4);
            s /= DOZER_PART_STEP;
            ENGINE_BLK(802026C4);
            func_802A05A4(D_802C21B8, s, 0.0f);
            ENGINE_BLK(802026D4);
            func_802A05D0(D_802C2190, DOZER_FIXED_PART);
            ENGINE_BLK(802026E8);
            func_802A05D0(D_802C21A4, DOZER_FIXED_PART);
        } else {
            ENGINE_BLK(80202614);
        }
    }
    ENGINE_BLK(802026F8);
}
