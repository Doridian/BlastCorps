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
    ENGINE_BLK(802AB478);
    func_802AB50C(carrier);
    ENGINE_BLK(802AB4C4);
}

REGS(s2)
void func_802AB50C(s32 carrier) {
    u8 *t0 = D_803ED3B8;
    s32 t1;

    ENGINE_BLK(802AB50C);
    for (;; t0 += 4) {
        ENGINE_BLK(802AB530);
        if (*(s32 *)t0 == -1)
            break;
        ENGINE_BLK(802AB540);
        if (t0[1] != carrier)
            continue;
        ENGINE_BLK(802AB54C);
        t1 = t0[0];
        if (t1 == 0)
            goto next;
        ENGINE_BLK(802AB558);
        if (t1 == 3) {
            ENGINE_BLK(802AB5B8);
            func_802B30F4(t0[1]);
            ENGINE_BLK(802AB5C0);
            goto up;
        }
        ENGINE_BLK(802AB560);
        if (t1 == 5) {
            ENGINE_BLK(802AB5C8);
            func_802B6100(t0[1]);
            ENGINE_BLK(802AB5D0);
            goto up;
        }
        ENGINE_BLK(802AB568);
        if (t1 == 4) {
            ENGINE_BLK(802AB5D8);
            func_802B4818(t0[1]);
            ENGINE_BLK(802AB5E0);
            goto up;
        }
        ENGINE_BLK(802AB570);
        if (t1 == 8) {
            ENGINE_BLK(802AB5E8);
            func_802B78F4(t0[1]);
            ENGINE_BLK(802AB5F0);
            goto up;
        }
        ENGINE_BLK(802AB578);
        if (t1 == 0xD) {
            ENGINE_BLK(802AB5F8);
            func_802CBD5C(t0[1]);
            ENGINE_BLK(802AB600);
            goto up;
        }
        ENGINE_BLK(802AB580);
        if (t1 == 0xE) {
            ENGINE_BLK(802AB608);
            func_802CCED4(t0[1]);
            ENGINE_BLK(802AB610);
            goto up;
        }
        ENGINE_BLK(802AB588);
        if (t1 == 0xF) {
            ENGINE_BLK(802AB618);
            func_802CFC54(t0[1]);
            ENGINE_BLK(802AB620);
            goto up;
        }
        ENGINE_BLK(802AB590);
        if (t1 == 9) {
            ENGINE_BLK(802AB628);
            func_802C59B4(t0[1]);
            ENGINE_BLK(802AB630);
            goto up;
        }
        ENGINE_BLK(802AB598);
        if (t1 == 0xA) {
            ENGINE_BLK(802AB5A8);
            func_802CA34C(t0[1]);
            ENGINE_BLK(802AB5B0);
            goto up;
        }
        ENGINE_BLK(802AB5A0);
        goto next;
    up:
        /* and what stands on it */
        ENGINE_BLK(802AB638);
        func_802AB50C(t1);
        ENGINE_BLK(802AB644);
    next:
        ENGINE_BLK(802AB648);
    }
    ENGINE_BLK(802AB650);
}

/* hd.c's: the vehicles on `carrier`, which moved, put back where they
   stood on it */
void func_802AB670(u8 carrier) {
    ENGINE_BLK(802AB670);
    func_802AB714(carrier);
    ENGINE_BLK(802AB6C4);
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

    ENGINE_BLK(802AB714);
    for (;; t0 += 4) {
        ENGINE_BLK(802AB734);
        if (*(s32 *)t0 == -1)
            break;
        ENGINE_BLK(802AB744);
        t1 = t0[1];
        if (t1 != carrier)
            continue;
        ENGINE_BLK(802AB750);
        if (t1 == 0)
            continue;
        ENGINE_BLK(802AB758);
        t1 = t0[0];
        if (t1 == 0)
            goto next;
        ENGINE_BLK(802AB764);
        if (t1 == 3) {
            ENGINE_BLK(802AB7C4);
            func_802B3180(carrier);
            ENGINE_BLK(802AB7CC);
            goto up;
        }
        ENGINE_BLK(802AB76C);
        if (t1 == 5) {
            ENGINE_BLK(802AB7D4);
            func_802B618C(carrier);
            ENGINE_BLK(802AB7DC);
            goto up;
        }
        ENGINE_BLK(802AB774);
        if (t1 == 4) {
            ENGINE_BLK(802AB7E4);
            func_802B48A4(carrier);
            ENGINE_BLK(802AB7EC);
            goto up;
        }
        ENGINE_BLK(802AB77C);
        if (t1 == 8) {
            ENGINE_BLK(802AB7F4);
            func_802B7980(carrier);
            ENGINE_BLK(802AB7FC);
            goto up;
        }
        ENGINE_BLK(802AB784);
        if (t1 == 0xD) {
            ENGINE_BLK(802AB804);
            func_802CBDE8(carrier);
            ENGINE_BLK(802AB80C);
            goto up;
        }
        ENGINE_BLK(802AB78C);
        if (t1 == 0xE) {
            ENGINE_BLK(802AB814);
            func_802CCF60(carrier);
            ENGINE_BLK(802AB81C);
            goto up;
        }
        ENGINE_BLK(802AB794);
        if (t1 == 0xF) {
            ENGINE_BLK(802AB824);
            func_802CFCE0(carrier);
            ENGINE_BLK(802AB82C);
            goto up;
        }
        ENGINE_BLK(802AB79C);
        if (t1 == 9) {
            ENGINE_BLK(802AB834);
            func_802C5A14(carrier);
            ENGINE_BLK(802AB83C);
            goto up;
        }
        ENGINE_BLK(802AB7A4);
        if (t1 == 0xA) {
            ENGINE_BLK(802AB7B4);
            func_802CA3D8(carrier);
            ENGINE_BLK(802AB7BC);
            goto up;
        }
        ENGINE_BLK(802AB7AC);
        goto next;
    up:
        /* and what stands on it */
        ENGINE_BLK(802AB844);
        func_802AB714(t1);
        ENGINE_BLK(802AB850);
    next:
        ENGINE_BLK(802AB854);
    }
    ENGINE_BLK(802AB85C);
}
