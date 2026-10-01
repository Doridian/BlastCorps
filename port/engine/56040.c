/*
 * hd_code 56040 (us.v11 0x8029A800-0x802A06F8): the vehicles' parts, as
 * native C (engine.h).  A vehicle's parts are 32 UnkStruct_803ED460
 * records (vehicle.h); the vehicle modules set their fields through these
 * helpers by index, or by their first word through D_803B35F8's list.
 */
#include "shared.h"
#include "game/vehicle.h"


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
    ENGINE_LEAVE_F(0, 0.0f);
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
    ENGINE_LEAVE_F(0, 0.0f);
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

/* ---- the parts' small helpers ------------------------------------------- */

extern u8 D_803B9890[];                 /* 0x60-byte records: ids at 0x4F, 0x50, a flag at 0x51 */
extern u8 D_803649ED, D_803A742B, D_803A7430;
extern s16 D_803A7410, D_803A7412;     /* the camera's headings (12-bit) */
extern u8 D_803B7FC8[];                 /* 0x78 records of 0xC bytes */
extern u8 *PTR32 D_803B8568;

/* part[0x55] = whether kind isn't 7 */
REGS(t2, s0)
void func_8029D534(s32 kind, u8 *part) {
    ENGINE_BLK(8029D534);
    if (kind != 7) {
        ENGINE_BLK(8029D548);
        part[0x55] = 1;
    } else {
        ENGINE_BLK(8029D550);
        part[0x55] = 0;
    }
    ENGINE_BLK(8029D554);
    ENGINE_LEAVE(1, 7);
}

/* D_803B9890's record with ids (a, b): its flag cleared; returns it ($s0) */
REGS(t7, t6 -> s0)
u8 *func_8029D1D4(s32 a, s32 b) {
    u8 *p = D_803B9890;

    ENGINE_BLK(8029D1D4);
    for (;;) {
        ENGINE_BLK(8029D1E4);
        if (p[0x4F] != a) {
            p += 0x60;
            continue;
        }
        ENGINE_BLK(8029D1F0);
        if (p[0x50] == b)
            break;
        p += 0x60;
    }
    ENGINE_BLK(8029D1FC);
    p[0x51] = 0;
    ENGINE_LEAVE(17, b);
    return p;
}

/* the flag of D_803B9890's first record with second id b ($t5) */
REGS(t4 -> t5)
s32 func_8029D210(s32 b) {
    u8 *p = D_803B9890;

    ENGINE_BLK(8029D210);
    for (;;) {
        ENGINE_BLK(8029D228);
        if (p[0x50] == b)
            break;
        p += 0x60;
    }
    ENGINE_BLK(8029D234);
    return p[0x51];
}



/* D_803649ED = id, unless D_803EFECB and func_802AB41C(id, 7) */
REGS(s1)
void func_8029CF54(s32 id) {
    ENGINE_BLK(8029CF54);
    if (D_803EFECB != 0) {
        s32 r;

        ENGINE_BLK(8029CF70);
        r = func_802AB41C(id, 7);
        ENGINE_BLK(8029CF7C);
        if (r != 0)
            goto done;
    }
    ENGINE_BLK(8029CF84);
    D_803649ED = id;
done:
    ENGINE_BLK(8029CF90);
}

/* 64 bytes from src to dst */
REGS(t5, s2)
void func_8029DE50(u32 *dst, u32 *src) {
    s32 i;

    ENGINE_BLK(8029DE50);
    for (i = 8; i != 0; i--) {
        ENGINE_BLK(8029DE6C);
        dst[0] = src[0];
        dst[1] = src[1];
        dst += 2, src += 2;
    }
    ENGINE_BLK(8029DE84);
}

/* D_803B7FC8's records cleared, and D_803B8568 at the first */
void func_8029DC80(void) {
    u8 *p = D_803B7FC8;
    s32 n = 0x78;

    ENGINE_BLK(8029DC80);
    D_803B8568 = p;
    for (;;) {
        ENGINE_BLK(8029DCA4);
        if (n == 0)
            break;
        ENGINE_BLK(8029DCAC);
        n--;
        ((u32 *)p)[0] = 0;
        ((u32 *)p)[1] = 0;
        p += 0xC;
    }
    ENGINE_BLK(8029DCC0);
}

/* a part of kind 7: D_803A742B set and D_803A7430 counted up to 0xC9 */
REGS(s0)
void func_8029B5B8(u8 *part) {
    ENGINE_BLK(8029B5B8);
    if (part[0x4F] == 7) {
        ENGINE_BLK(8029B5D4);
        D_803A742B = 1;
        if (D_803A7430 < 0xC9) {
            ENGINE_BLK(8029B5F4);
            D_803A7430++;
        }
    }
    ENGINE_BLK(8029B600);
}

/* the camera's turn from D_803A7410 to D_803A7412 (12-bit), folded */
s32 func_8029B930(void) {
    s32 from = (u16)D_803A7410, to = (u16)D_803A7412, r;

    ENGINE_BLK(8029B930);
    if (to < from) {
        ENGINE_BLK(8029B95C);
        r = to + (0xFFF - from);
    } else {
        ENGINE_BLK(8029B96C);
        r = to - from;
        if (r >= 0x801) {
            ENGINE_BLK(8029B97C);
            r -= 0x800;
        }
    }
    ENGINE_BLK(8029B980);
    return r;
}
