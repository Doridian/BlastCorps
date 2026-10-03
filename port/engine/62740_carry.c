/*
 * hd_code 62740's carrying (us.v11 0x802AB478-0x802AB868), as native C
 * (engine.h): a vehicle on top of another, which D_803ED3B8's links record
 * (a word each, -1 at the end: the carried vehicle's type in its first
 * byte, its carrier's in the second).  func_802AB478 (from hd.c, with the
 * vehicle that is about to move) has every vehicle on it keep where it
 * stands; func_802AB670 (from hd.c, with the vehicle that moved) puts them
 * back there.  Both go up the links recursively, to what stands on those.
 * The callbacks are the vehicle modules' (shared.h, CARRY_KEEP and
 * CARRY_MOVE).  The rest of 62740 is engine-A's 62740.c.
 *
 * The original's func_802AB714 leaves the driver's shadow its tilt in
 * its frame (69BB0.c's func_802AF340 reads it, when hd.c runs it next):
 * func_802AB670 puts that word where the shadow looks for it.
 */
#include "shared.h"
#include "game/vehicle.h"

extern u8 D_803ED3B8[];

REGS(s2)
void func_802AB50C(s32 carrier);
REGS(a3)
void func_802AB714(s32 carrier);

/* the carried vehicle's two callbacks, by its type (the vehicles that can
   stand on another: Skyfall, Ramdozer, Backlash, the American Dream, the
   police car, the A-Team van, Starski's hotrod, the J-Bomb, the Ballista) */
typedef void CarryFn(s32 carrier);

static void carry_fns(s32 type, CarryFn **keep, CarryFn **move) {
    switch (type) {
    case VEHICLE_BUGGY: *keep = func_802B30F4, *move = func_802B3180; break;
    case VEHICLE_TRUCK: *keep = func_802B6100, *move = func_802B618C; break;
    case VEHICLE_BULLDOZER: *keep = func_802B4818, *move = func_802B48A4; break;
    case VEHICLE_HOTROD: *keep = func_802B78F4, *move = func_802B7980; break;
    case VEHICLE_POLICE: *keep = func_802CBD5C, *move = func_802CBDE8; break;
    case VEHICLE_ATEAM: *keep = func_802CCED4, *move = func_802CCF60; break;
    case VEHICLE_STARSKI: *keep = func_802CFC54, *move = func_802CFCE0; break;
    case VEHICLE_JETPACK: *keep = func_802C59B4, *move = func_802C5A14; break;
    case VEHICLE_BIKE: *keep = func_802CA34C, *move = func_802CA3D8; break;
    default: *keep = *move = NULL; break;
    }
}

/* hd.c's: the vehicles on `carrier` keep where they stand on it */
void func_802AB478(u8 carrier) {
    ENGINE_COST(802AB478, 37);
    func_802AB50C(carrier);
}

/* each link (carried, carrier) with this carrier: the carried vehicle's
   first callback, then the same for what stands on it */
REGS(s2)
void func_802AB50C(s32 carrier) {
    u8 *p;
    s32 type;
    CarryFn *keep, *move;

    ENGINE_COST(802AB50C, 55);
    for (p = D_803ED3B8; *(s32 *)p != -1; p += 4) {
        if (p[1] != carrier)
            continue;
        type = p[0];
        carry_fns(type, &keep, &move);
        if (keep != NULL) {
            keep(p[1]);
            func_802AB50C(type);
        }
    }
}

/* hd.c's: the vehicles on `carrier`, which moved, put back where they
   stood on it */
void func_802AB670(u8 carrier) {
    ENGINE_COST(802AB670, 41);
    func_802AB714(carrier);
}

/* each link with this carrier (not 0): the carried vehicle's second
   callback, then the same for what stands on it */
REGS(a3)
void func_802AB714(s32 carrier) {
    u8 *p;
    s32 type;
    CarryFn *keep, *move;

    ENGINE_COST(802AB714, 49);
    for (p = D_803ED3B8; *(s32 *)p != -1; p += 4) {
        if (p[1] != carrier || carrier == 0)
            continue;
        type = p[0];
        carry_fns(type, &keep, &move);
        if (move != NULL) {
            move(carrier);
            func_802AB714(type);
        }
    }
}
