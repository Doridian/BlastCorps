/*
 * hd_code 80280 (us.v11 0x802C4A40-0x802C80D0): the J-Bomb (type 9), as
 * native C (engine.h).  Its parts are D_803F7850, its state D_803F7B50 and
 * its position D_803F7BF8..C00 (vehicle.h).
 *
 * Native so far: its two callbacks for 62740's carrying (shared.h); the
 * rest is still translated.
 */
#include "shared.h"

extern VS D_803F7B50;
extern s32 D_803F7BF8, D_803F7BFC, D_803F7C00;  /* x, y, z */
extern u8 D_803ED40B;

/* translated */
REGS(gp)
void func_802C7ECC(VS *vs);
REGS(gp)
void func_802C7F28(VS *vs);
REGS(gp)
void func_802C7CB0(VS *vs);

#define T(p) ((s32)(p))

/* 62740's carrying (shared.h): where it stands on its carrier (its point
   only; the original saves and loads back $t0, $t1, $t3 and $t4) */
REGS(a3)
void func_802C59B4(s32 carrier) {
    VS *vs = &D_803F7B50;
    s32 t3, t4;

    ENGINE_BLK(802C59B4);
    engine_save(ENGINE_GPR(8) | ENGINE_GPR(9) | ENGINE_GPR(11) | ENGINE_GPR(12), 0);
    ENGINE_LEAVE(28, T(vs));
    t3 = func_802AAD0C(carrier, D_803F7BF8, D_803F7C00, &t4);
    ENGINE_BLK(802C59F0);
    vs->unk6A = t3;
    vs->unk6C = t4;
    engine_restore();
}

/* and back there after the carrier moved, its heading kept (the original
   saves and loads back $a3, $t0, $t1 and $t2) */
REGS(a3)
void func_802C5A14(s32 carrier) {
    VS *vs = &D_803F7B50;
    s32 t3, t4;

    ENGINE_BLK(802C5A14);
    engine_save(ENGINE_GPR(7) | ENGINE_GPR(8) | ENGINE_GPR(9) | ENGINE_GPR(10), 0);
    ENGINE_LEAVE(28, T(vs));
    t3 = func_802AAE54(carrier, vs->unk6A, vs->unk6C, &t4);
    ENGINE_LEAVE(8, vs->unk6A);
    ENGINE_LEAVE(9, vs->unk6C);
    ENGINE_LEAVE(11, t3);
    ENGINE_LEAVE(12, t4);
    ENGINE_BLK(802C5A40);
    func_802C7ECC(vs);
    ENGINE_BLK(802C5A48);
    func_802C7F28(vs);
    ENGINE_BLK(802C5A50);
    D_803ED40B = 0;
    ENGINE_LEAVE(12, t4);
    ENGINE_LEAVE(14, T(&vs->unk76));
    ENGINE_LEAVE(16, T(vs->unk96));
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(t3, t4, &D_803F7BF8, &D_803F7C00, &D_803F7BFC, 9, 0x78, 0x78, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802C5AAC);
    func_802C7CB0(vs);
    ENGINE_BLK(802C5AB4);
    func_802A133C(D_803F7BF8, D_803F7BFC, D_803F7C00, 9, vs);
    ENGINE_BLK(802C5AE0);
    engine_restore();
}
