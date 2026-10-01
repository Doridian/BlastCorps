/*
 * hd_code 56040 (us.v11 0x8029A800-0x802A06F8): the vehicles' parts, as
 * native C (engine.h).  A vehicle's parts are 32 UnkStruct_803ED460
 * records (vehicle.h); the vehicle modules set their fields through these
 * helpers by index, or by their first word through D_803B35F8's list.
 */
#include "engine.h"
#include "game/vehicle.h"

typedef UnkStruct_803ED460 Part;

extern Part D_803B35F8[];

/* the record whose first word is `key` */
REGS(v0 -> v0)
Part *func_802A06B4(void *key) {
    Part *p = D_803B35F8;

    ENGINE_BLK(802A06B4);
    for (;;) {
        ENGINE_BLK(802A06CC);
        if (p->unk0 == key)
            break;
        ENGINE_BLK(802A06D8);
        p++;
    }
    ENGINE_BLK(802A06E0);
    return p;
}

/* ten halfwords from 20 bytes back; returns the next record */
REGS(t1 -> t1)
s16 *func_8029FF2C(s16 *d) {
    s32 k;

    ENGINE_BLK(8029FF2C);
    for (k = 0; k < 10; k++)
        d[k] = d[k - 10];
    return d + 10;
}

/* ---- the parts, by index ----------------------------------------------- */

REGS(v0, v1, a0)
void func_802A0290(s32 i, s32 v, Part *parts) {
    ENGINE_BLK(802A0290);
    parts[i].unk10 = 1;
    parts[i].unkE = v;
    parts[i].unkC = 0;
}

REGS(v0, v1)
void func_802A02E4(s32 i, Part *parts) {
    ENGINE_BLK(802A02E4);
    parts[i].unk10 = 0;
}

REGS(v0, v1)
void func_802A0320(s32 i, Part *parts) {
    ENGINE_BLK(802A0320);
    parts[i].unk13 = 0;
    parts[i].unk4 = 0.0f;
}

REGS(v0, v1, a0, f0)
void func_802A0360(s32 i, s32 v, Part *parts, f32 f) {
    ENGINE_BLK(802A0360);
    parts[i].unk13 = v;
    parts[i].unk4 = f;
}

REGS(v0, v1, a0)
void func_802A039C(s32 i, s32 v, Part *parts) {
    ENGINE_BLK(802A039C);
    parts[i].unk14 = v;
}

REGS(v0, v1, a0)
void func_802A03D4(s32 i, s32 v, Part *parts) {
    ENGINE_BLK(802A03D4);
    parts[i].unk11 = v;
}

REGS(v0, v1, a0)
void func_802A040C(s32 i, s32 v, Part *parts) {
    ENGINE_BLK(802A040C);
    parts[i].unk12 = v;
}

REGS(v0, v1, a0)
void func_802A0444(s32 i, s32 v, Part *parts) {
    ENGINE_BLK(802A0444);
    parts[i].unkC = 0;
    parts[i].unkE = v;
}

REGS(v0, v1, a0, f0)
void func_802A0480(s32 i, s32 v, Part *parts, f32 f) {
    ENGINE_BLK(802A0480);
    parts[i].unk15 = v;
    parts[i].unk8 = f;
}

/* all of a part's fields, in registers */
REGS(v0, v1 -> v1, a0, a1, a2, a3, t0, t1, f0)
s32 func_802A04BC(s32 i, Part *parts, s32 *f11, s32 *f12, s32 *f14, s32 *fC, s32 *fE, s32 *f13, f32 *f4) {
    Part *p = &parts[i];

    ENGINE_BLK(802A04BC);
    *f11 = p->unk11;
    *f12 = p->unk12;
    *f14 = p->unk14;
    *fC = (u16)p->unkC;
    *fE = (u16)p->unkE;
    *f13 = p->unk13;
    *f4 = p->unk4;
    return p->unk10;
}

/* ---- the parts, by key (D_803B35F8) ------------------------------------- */

REGS(v0, v1)
void func_802A0508(void *key, s32 v) {
    Part *p;

    ENGINE_BLK(802A0508);
    p = func_802A06B4(key);
    ENGINE_BLK(802A051C);
    p->unk10 = 1;
    p->unkE = v;
    p->unkC = 0;
}

REGS(v0)
void func_802A0540(void *key) {
    Part *p;

    ENGINE_BLK(802A0540);
    p = func_802A06B4(key);
    ENGINE_BLK(802A0554);
    p->unk10 = 0;
}

REGS(v0)
void func_802A0570(void *key) {
    Part *p;

    ENGINE_BLK(802A0570);
    p = func_802A06B4(key);
    ENGINE_BLK(802A0584);
    p->unk13 = 0;
    p->unk4 = 0.0f;
}

REGS(v0, v1, f0)
void func_802A05A4(void *key, s32 v, f32 f) {
    Part *p;

    ENGINE_BLK(802A05A4);
    p = func_802A06B4(key);
    ENGINE_BLK(802A05B4);
    p->unk13 = v;
    p->unk4 = f;
}

REGS(v0, v1)
void func_802A05D0(void *key, s32 v) {
    Part *p;

    ENGINE_BLK(802A05D0);
    p = func_802A06B4(key);
    ENGINE_BLK(802A05E0);
    p->unk14 = v;
}

REGS(v0, v1)
void func_802A05F8(void *key, s32 v) {
    Part *p;

    ENGINE_BLK(802A05F8);
    p = func_802A06B4(key);
    ENGINE_BLK(802A0608);
    p->unk11 = v;
}

REGS(v0, v1)
void func_802A0620(void *key, s32 v) {
    Part *p;

    ENGINE_BLK(802A0620);
    p = func_802A06B4(key);
    ENGINE_BLK(802A0630);
    p->unk12 = v;
}

REGS(v0, v1)
void func_802A0648(void *key, s32 v) {
    Part *p;

    ENGINE_BLK(802A0648);
    p = func_802A06B4(key);
    ENGINE_BLK(802A0658);
    p->unkC = 0;
    p->unkE = v;
}

/* (it loads the fields, as func_802A04BC does, but no caller reads them) */
REGS(v0)
void func_802A0674(void *key) {
    ENGINE_BLK(802A0674);
    func_802A06B4(key);
    ENGINE_BLK(802A0684);
}
