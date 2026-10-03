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

extern u8 D_803ED3B8[];

REGS(s2)
void func_802AB50C(s32 carrier);
REGS(a3)
void func_802AB714(s32 carrier);

/* hd.c's: the vehicles on `carrier` keep where they stand on it */
void func_802AB478(u8 carrier) {
    ENGINE_COST(802AB478, 37);
    func_802AB50C(carrier);
}

REGS(s2)
void func_802AB50C(s32 carrier) {
    u8 *t0 = D_803ED3B8;
    s32 t1;

    ENGINE_COST(802AB50C, 55);
    for (;; t0 += 4) {
        if (*(s32 *)t0 == -1)
            break;
        if (t0[1] != carrier)
            continue;
        t1 = t0[0];
        if (t1 == 0)
            goto next;
        if (t1 == 3) {
            func_802B30F4(t0[1]);
            goto up;
        }
        if (t1 == 5) {
            func_802B6100(t0[1]);
            goto up;
        }
        if (t1 == 4) {
            func_802B4818(t0[1]);
            goto up;
        }
        if (t1 == 8) {
            func_802B78F4(t0[1]);
            goto up;
        }
        if (t1 == 0xD) {
            func_802CBD5C(t0[1]);
            goto up;
        }
        if (t1 == 0xE) {
            func_802CCED4(t0[1]);
            goto up;
        }
        if (t1 == 0xF) {
            func_802CFC54(t0[1]);
            goto up;
        }
        if (t1 == 9) {
            func_802C59B4(t0[1]);
            goto up;
        }
        if (t1 == 0xA) {
            func_802CA34C(t0[1]);
            goto up;
        }
        goto next;
    up:
        /* and what stands on it */
        func_802AB50C(t1);
    next:
        ;
    }
}

/* hd.c's: the vehicles on `carrier`, which moved, put back where they
   stood on it */
void func_802AB670(u8 carrier) {
    ENGINE_COST(802AB670, 41);
    func_802AB714(carrier);
    /* the shadow's word: the $ra the original's func_802AB714 saved at the
       bottom of its frame (0x28 under this one's 0x88, under the 16 the
       glue would have left the C), its return here */
    engine_save(ENGINE_GPR(31), 0);
    ENGINE_RA(802AB6C4);
    engine_frame(-(ENGINE_C_FRAME + ENGINE_FRAME_S + 0x28));
    engine_frame_sd(0, 31);
    engine_frame(ENGINE_C_FRAME + ENGINE_FRAME_S + 0x28);
    engine_restore();
}

REGS(a3)
void func_802AB714(s32 carrier) {
    u8 *t0 = D_803ED3B8;
    s32 t1;

    ENGINE_COST(802AB714, 49);
    for (;; t0 += 4) {
        if (*(s32 *)t0 == -1)
            break;
        t1 = t0[1];
        if (t1 != carrier)
            continue;
        if (t1 == 0)
            continue;
        t1 = t0[0];
        if (t1 == 0)
            goto next;
        if (t1 == 3) {
            func_802B3180(carrier);
            goto up;
        }
        if (t1 == 5) {
            func_802B618C(carrier);
            goto up;
        }
        if (t1 == 4) {
            func_802B48A4(carrier);
            goto up;
        }
        if (t1 == 8) {
            func_802B7980(carrier);
            goto up;
        }
        if (t1 == 0xD) {
            func_802CBDE8(carrier);
            goto up;
        }
        if (t1 == 0xE) {
            func_802CCF60(carrier);
            goto up;
        }
        if (t1 == 0xF) {
            func_802CFCE0(carrier);
            goto up;
        }
        if (t1 == 9) {
            func_802C5A14(carrier);
            goto up;
        }
        if (t1 == 0xA) {
            func_802CA3D8(carrier);
            goto up;
        }
        goto next;
    up:
        /* and what stands on it */
        func_802AB714(t1);
    next:
        ;
    }
}
