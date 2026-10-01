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

extern u8 D_803A6B30[];                 /* 0x14-byte records, to 0xFF in byte 0x13 */
extern u8 *PTR32 D_803BD300;            /* the end of D_803B9890's records */
extern u8 D_803A7440[];                 /* 12 records of 0x1010 bytes */
extern u8 D_8035805C;

/* D_803A6B30's record of kind 6 and id 0x3BD: its point in s0, s1, s2;
   returns whether there is one ($s4) */
REGS( -> s4)
s32 func_8029C6E4(void) {
    u8 *p = D_803A6B30;
    s32 t7, s3 = 0, read3 = 0, r = 0;

    ENGINE_BLK(8029C6E4);
    for (;;) {
        ENGINE_BLK(8029C704);
        t7 = (s8)p[0x13];
        if (t7 == -1)
            break;
        ENGINE_BLK(8029C710);
        t7 = p[0x12];
        if (t7 != 6) {
            p += 0x14;
            continue;
        }
        ENGINE_BLK(8029C71C);
        s3 = *(s32 *)(p + 0xC);
        read3 = 1;
        if (s3 != 0x3BD) {
            p += 0x14;
            continue;
        }
        ENGINE_BLK(8029C728);
        ENGINE_LEAVE(16, *(s32 *)p);
        ENGINE_LEAVE(17, *(s32 *)(p + 4));
        ENGINE_LEAVE(18, *(s32 *)(p + 8));
        r = 1;
        break;
    }
    ENGINE_BLK(8029C738);
    if (read3)
        ENGINE_LEAVE(19, s3);
    ENGINE_LEAVE(11, 0x3BD);
    ENGINE_LEAVE(12, 6);
    ENGINE_LEAVE(13, -1);
    ENGINE_LEAVE(14, (u32)p);
    ENGINE_LEAVE(15, t7);
    return r;
}

/* 0 if a part of kind id has its flag clear, else 1 ($v1) */
REGS(v0 -> v1)
s32 func_8029DC14(s32 id) {
    u8 *p = D_803B9890, *end = D_803BD300;
    s32 r = 1, flag;

    ENGINE_BLK(8029DC14);
    for (;;) {
        ENGINE_BLK(8029DC40);
        if (p == end)
            break;
        ENGINE_BLK(8029DC48);
        if (p[0x4F] != id) {
            p += 0x60;
            continue;
        }
        ENGINE_BLK(8029DC54);
        flag = p[0x51];
        p += 0x60;
        if (flag != 0)
            continue;
        ENGINE_BLK(8029DC64);
        r = 0;
        break;
    }
    ENGINE_BLK(8029DC68);
    return r;
}

s32 func_8029DBF0(s32 id) {
    s32 r;

    ENGINE_BLK(8029DBF0);
    r = func_8029DC14(id);
    ENGINE_BLK(8029DC00);
    return r;
}

/* 0 if one of the n (part, value) byte pairs has the part's byte 0x13
   (parts 0x18 bytes each, at parts) equal to the value, else 1 ($t4) */
REGS(s4, t5, a2 -> t4)
s32 func_8029DB7C(u8 *pairs, s32 n, u8 *parts) {
    s32 r = 1;

    ENGINE_BLK(8029DB7C);
    for (;;) {
        ENGINE_BLK(8029DBA0);
        if (n == 0)
            break;
        ENGINE_BLK(8029DBA8);
        n--;
        if ((s8)parts[pairs[0] * 0x18 + 0x13] == pairs[1]) {
            ENGINE_BLK(8029DBD0);
            r = 0;
            break;
        }
        pairs += 2;
    }
    ENGINE_BLK(8029DBD4);
    ENGINE_LEAVE(13, n);
    return r;
}

/* D_803B7FC8's records for id cleared (the last pulling D_803B8568 back) */
REGS(s2)
void func_8029DD54(s32 id) {
    u8 *p = D_803B7FC8, *end = (u8 *)&D_803B8568;

    ENGINE_BLK(8029DD54);
    for (;;) {
        ENGINE_BLK(8029DD78);
        if (end < p)
            break;
        ENGINE_BLK(8029DD84);
        if (((s32 *)p)[1] != id) {
            p += 0xC;
            continue;
        }
        ENGINE_BLK(8029DD90);
        ((u32 *)p)[0] = 0;
        ((u32 *)p)[1] = 0;
        if (p == end) {
            ENGINE_BLK(8029DD9C);
            D_803B8568 = p - 0xC;
        }
        ENGINE_BLK(8029DDA8);
        p += 0xC;
    }
    ENGINE_BLK(8029DDB0);
    ENGINE_LEAVE(1, 1);
}

/* D_803A7440's used record (byte 6) with this kind (half 4) and id
   (word 0): its flag (byte 7) cleared; returns the physical address of
   its data, 0x10 on ($s1), 0 if none */
REGS(t6, t3 -> s1)
u32 func_8029E4E4(s32 kind, s32 id) {
    u8 *p = D_803A7440;
    s32 n = 12, a2 = 0, read = 0;
    u32 r = 0;

    ENGINE_BLK(8029E4E4);
    for (;;) {
        ENGINE_BLK(8029E4FC);
        if (n == 0)
            break;
        ENGINE_BLK(8029E504);
        a2 = p[6], read = 1;
        n--;
        if (a2 == 0)
            goto next;
        ENGINE_BLK(8029E514);
        a2 = *(u16 *)(p + 4);
        if (a2 != kind)
            goto next;
        ENGINE_BLK(8029E520);
        a2 = *(s32 *)p;
        if (a2 != id)
            goto next;
        ENGINE_BLK(8029E52C);
        p[7] = 0;
        r = (u32)(p + 0x10) - 0x80000000;
        break;
    next:
        ENGINE_BLK(8029E540);
        p += 0x1010;
    }
    ENGINE_BLK(8029E548);
    if (read)
        ENGINE_LEAVE(6, a2);
    return r;
}

/* whether the part's point (0x28) is within r of (x, y, z) ($t7) */
REGS(s0, t3, t4, t5, t6 -> t7)
s32 func_8029BEE4(u8 *part, s32 x, s32 y, s32 z, s32 r) {
    s32 px = *(s32 *)(part + 0x28), py = *(s32 *)(part + 0x2C), pz = *(s32 *)(part + 0x30);
    s32 dx = px - x, dy = py - y, dz = pz - z, in;
    s64 d2, dy2 = (s64)dy * dy, dz2 = (s64)dz * dz, d;

    ENGINE_BLK(8029BEE4);
    d2 = (s64)dx * dx + dy2 + dz2;
    d = engine_cvt_l_s(__builtin_sqrtf((f32)d2));
    if (r < d) {
        ENGINE_BLK(8029BF50);
        in = 0;
    } else {
        ENGINE_BLK(8029BF58);
        in = 1;
    }
    ENGINE_BLK(8029BF5C);
    /* (what it leaves: everything) */
    ENGINE_LEAVE(1, r < d);
    ENGINE_LEAVE(2, px);
    ENGINE_LEAVE(3, py);
    ENGINE_LEAVE(4, pz);
    ENGINE_LEAVE(5, dx);
    ENGINE_LEAVE(6, dy);
    ENGINE_LEAVE(7, dz);
    ENGINE_LEAVE64(8, d);
    ENGINE_LEAVE64(9, dy2);
    ENGINE_LEAVE64(10, dz2);
    ENGINE_LEAVE_FW(0, (u32)d);
    ENGINE_LEAVE_FW(1, (u32)((u64)d >> 32));
    return in;
}

/* a free record of D_803B7FC8 (word 0 clear) for (s2, t5, !D_8035805C),
   D_803B8568 moved up to it */
REGS(s2, t5)
void func_8029DCD4(s32 a, s32 b) {
    u8 *p = D_803B7FC8;
    s32 n = 0x78;

    ENGINE_BLK(8029DCD4);
    for (;;) {
        ENGINE_BLK(8029DCF4);
        if (n == 0)
            goto done;
        ENGINE_BLK(8029DCFC);
        n--;
        if (((u32 *)p)[0] != 0) {
            p += 0xC;
            continue;
        }
        break;
    }
    ENGINE_BLK(8029DD0C);
    ((s32 *)p)[0] = a;
    ((s32 *)p)[1] = b;
    ((s32 *)p)[2] = D_8035805C ^ 1;
    if (D_803B8568 < p) {
        ENGINE_BLK(8029DD38);
        D_803B8568 = p;
    }
    /* ($at: the delay slot's lui) */
    ENGINE_LEAVE(1, ((u32)&D_803B8568 + 0x8000) & 0xFFFF0000);
done:
    ENGINE_BLK(8029DD3C);
}

/* The part's three points (0x28, 12 bytes each) seen along its axis
   (byte 0x4E: 0 drops z, 1 y, else x), as v0-a3; and two of the point
   (x, y, z) in t0, t1 likewise.  All in registers for the callers. */
REGS(s0, v1, a0, a1)
void func_8029C0DC(u8 *part, s32 x, s32 y, s32 z) {
    s32 *w = (s32 *)(part + 0x28);
    s32 axis = (s8)part[0x4E], i, j;

    ENGINE_BLK(8029C0DC);
    ENGINE_LEAVE(1, 1);
    if (axis == 0) {
        ENGINE_BLK(8029C138);
        ENGINE_LEAVE(8, x);
        ENGINE_LEAVE(9, y);
        i = 0, j = 1;
    } else if (axis == 1) {
        ENGINE_BLK(8029C0E8);
        ENGINE_BLK(8029C114);
        ENGINE_LEAVE(8, x);
        ENGINE_LEAVE(9, z);
        i = 0, j = 2;
    } else {
        ENGINE_BLK(8029C0E8);
        ENGINE_BLK(8029C0F0);
        ENGINE_LEAVE(8, y);
        ENGINE_LEAVE(9, z);
        i = 1, j = 2;
    }
    ENGINE_BLK(8029C158);
    ENGINE_LEAVE(2, w[i]);
    ENGINE_LEAVE(3, w[j]);
    ENGINE_LEAVE(4, w[3 + i]);
    ENGINE_LEAVE(5, w[3 + j]);
    ENGINE_LEAVE(6, w[6 + i]);
    ENGINE_LEAVE(7, w[6 + j]);
}

/* D_803B7FC8's records (src, dst, flag): those whose flag is
   D_8035805C copied (func_8029DE50) and cleared; D_803B8568 left at the
   last other one */
void func_8029DDC8(void) {
    u8 *p = D_803B7FC8, *end = D_803B8568, *last = p;
    u32 flag = D_8035805C;

    ENGINE_BLK(8029DDC8);
    for (;;) {
        ENGINE_BLK(8029DDF0);
        if (end < p)
            break;
        ENGINE_BLK(8029DDFC);
        if (((u32 *)p)[0] == 0) {
            p += 0xC;
            continue;
        }
        ENGINE_BLK(8029DE08);
        if (((u32 *)p)[2] != flag) {
            ENGINE_BLK(8029DE14);
            last = p;
            p += 0xC;
            continue;
        }
        ENGINE_BLK(8029DE20);
        func_8029DE50(*(u32 *PTR32 *)(p + 4), *(u32 *PTR32 *)p);
        ENGINE_BLK(8029DE28);
        ((u32 *)p)[0] = 0;
        ((u32 *)p)[1] = 0;
        p += 0xC;
    }
    ENGINE_BLK(8029DE38);
    D_803B8568 = last;
}

/* whether two spheres meet: centres (x1, y1, z1) and (x2, y2, z2), radii
   r1 and r2 ($t2) */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t2)
s32 func_8029CFA4(s32 x1, s32 y1, s32 z1, s32 r1, s32 x2, s32 y2, s32 z2, s32 r2) {
    s32 dx = x2 - x1, dy = y2 - y1, dz = z2 - z1, r = 0;
    f32 d, rr;

    ENGINE_BLK(8029CFA4);
    rr = (f32)(r1 + r2);
    d = __builtin_sqrtf((f32)((s64)dx * dx + (s64)dy * dy + (s64)dz * dz));
    if (d < rr) {
        ENGINE_BLK(8029D020);
        r = 1;
    }
    ENGINE_BLK(8029D024);
    ENGINE_LEAVE_F(0, d);
    ENGINE_LEAVE_F(2, rr);
    return r;
}

/* whether the sphere (x, y, z), r meets the part's (0x10, its radius at
   0xC, all / 4) ($v1) */
REGS(s0, t3, t4, t5, t6 -> v1)
s32 func_8029B514(u8 *part, s32 x, s32 y, s32 z, s32 r) {
    s32 *w = (s32 *)part, hit;

    ENGINE_BLK(8029B514);
    hit = func_8029CFA4(x, y, z, r, w[4] >> 2, w[5] >> 2, w[6] >> 2, w[3] >> 2);
    ENGINE_BLK(8029B57C);
    return hit;
}

extern f32 D_803B3778[16];              /* the spline's basis */

/* with fp 1, D_803B3778 = the cardinal spline basis of tension
   *(f32 *)(t0 + 8) */
REGS(t0, fp)
void func_8029F110(u8 *spline, s32 mode) {
    f32 a = *(f32 *)(spline + 8), na = -a, a2;
    f32 *m = D_803B3778;

    ENGINE_BLK(8029F110);
    if (mode == 1) {
        ENGINE_BLK(8029F148);
        a2 = a * 2.0f;
        m[0] = na;
        m[2] = na;
        m[3] = 0.0f;
        m[6] = 0.0f;
        m[7] = 1.0f;
        m[10] = a;
        m[11] = 0.0f;
        m[12] = a;
        m[1] = a2;
        m[13] = na;
        m[14] = 0.0f;
        m[15] = 0.0f;
        m[9] = 3.0f - a2;
        m[4] = 2.0f - a;
        m[5] = a - 3.0f;
        m[8] = a - 2.0f;
    }
    ENGINE_BLK(8029F1A8);
    ENGINE_LEAVE_F(20, 1.0f);
}

extern f32 D_803B37B8, D_803B37BC;      /* t^2 and t^3 */
extern u8 D_803B7FC0, D_803B7FC1, D_803B7FC2, D_803B7FC3;

/* the spline's step: t^2 and t^3 of t, and the four points' indices
   around i of n, wrapping */
REGS(f30, t2, t6)
void func_8029F060(f32 t, s32 i, s32 n) {
    s32 m = n - 1, v;
    f32 t2 = t * t, t3 = t2 * t;

    ENGINE_BLK(8029F060);
    D_803B37B8 = t2;
    D_803B37BC = t3;
    v = i - 1;
    if (v >= 0) {
        ENGINE_BLK(8029F094);
        D_803B7FC0 = v;
    } else {
        ENGINE_BLK(8029F0A0);
        D_803B7FC0 = v + m;
    }
    ENGINE_BLK(8029F0AC);
    D_803B7FC1 = i;
    v = i + 1;
    if (v < m) {
        ENGINE_BLK(8029F0C4);
        D_803B7FC2 = v;
    } else {
        ENGINE_BLK(8029F0CC);
        D_803B7FC2 = v - m;
    }
    ENGINE_BLK(8029F0D8);
    v = i + 2;
    if (v < m) {
        ENGINE_BLK(8029F0E8);
        D_803B7FC3 = v;
    } else {
        ENGINE_BLK(8029F0F0);
        D_803B7FC3 = v - m;
    }
    ENGINE_BLK(8029F0FC);
    ENGINE_LEAVE(1, ((u32)&D_803B7FC3 + 0x8000) & 0xFFFF0000);
    ENGINE_LEAVE_F(8, t3);
}

extern f32 D_8030D870;                  /* a full turn, in 16ths */

/* the angle from a to b (12-bit) the fraction t of the way, the short way
   round ($t4) */
REGS(a0, a1, f30 -> t4)
s32 func_8029F6B0(s32 a, s32 b, f32 t) {
    f32 turn = D_8030D870, fa, d;
    s32 r;

    ENGINE_BLK(8029F6B0);
    fa = (f32)a / 16.0f;
    d = (f32)b / 16.0f;
    d = d - fa;
    if (d < -2048.0f) {
        ENGINE_BLK(8029F704);
        d = d + turn;
    } else {
        ENGINE_BLK(8029F70C);
        if (!(d <= 2048.0f)) {
            ENGINE_BLK(8029F718);
            d = d - turn;
        }
    }
    ENGINE_BLK(8029F71C);
    d = d * t;
    d = d + fa;
    if (d < 0.0f) {
        ENGINE_BLK(8029F730);
        d = d + turn;
    }
    ENGINE_BLK(8029F734);
    if (!(d <= turn)) {
        ENGINE_BLK(8029F740);
        d = d - turn;
    }
    ENGINE_BLK(8029F744);
    r = engine_cvt_w_s(d * 16.0f);
    ENGINE_LEAVE(1, 0x41800000);
    ENGINE_LEAVE_F(2, fa);
    ENGINE_LEAVE_F(8, 0.0f);
    return r;
}

extern u8 *PTR32 D_803BD304;            /* the end of D_803BC1D0's records */
extern u8 D_803BC1D0[];                 /* 0xDC-byte records: an id at 0xC4, a count at 0xC7 of bytes from 0xC8 */

/* The parts of kind id, and those of kind 0 whose byte 0x50 is listed in
   a D_803BC1D0 record of that id: flagged (0x51) */
REGS(t2)
void func_8029D120(s32 id) {
    u8 *p = D_803B9890, *end = D_803BD300, *q, *qend, *b;
    s32 k, n, c, t4 = 0, t5 = 0, t6 = 0, t7 = 0, set4 = 0, set5 = 0, set6 = 0, set7 = 0;

    ENGINE_BLK(8029D120);
    for (;; p += 0x60) {
        ENGINE_BLK(8029D140);
        if (p == end)
            break;
        ENGINE_BLK(8029D148);
        k = p[0x4F];
        if (k == id)
            goto flag;
        ENGINE_BLK(8029D154);
        if (k != 0)
            goto next;
        ENGINE_BLK(8029D15C);
        t7 = p[0x50], set7 = 1;
        qend = D_803BD304;
        for (q = D_803BC1D0;; q += 0xDC) {
            ENGINE_BLK(8029D174);
            if (q == qend)
                goto next;
            ENGINE_BLK(8029D17C);
            t4 = q[0xC4], set4 = 1;
            if (t4 != id)
                goto nextq;
            ENGINE_BLK(8029D188);
            b = q + 0xC8;
            t4 = (u32)b;
            t5 = n = q[0xC7], set5 = 1;
            for (;;) {
                ENGINE_BLK(8029D190);
                if (n == 0)
                    break;
                ENGINE_BLK(8029D198);
                t6 = c = *b, set6 = 1;
                t5 = --n;
                if (c == t7)
                    goto flag;
                ENGINE_BLK(8029D1A8);
                b++;
                t4 = (u32)b;
            }
        nextq:
            ENGINE_BLK(8029D1B0);
        }
    flag:
        ENGINE_BLK(8029D1B8);
        p[0x51] = 1;
    next:
        ENGINE_BLK(8029D1BC);
    }
    ENGINE_BLK(8029D1C4);
    if (set4)
        ENGINE_LEAVE(12, t4);
    if (set5)
        ENGINE_LEAVE(13, t5);
    if (set6)
        ENGINE_LEAVE(14, t6);
    if (set7)
        ENGINE_LEAVE(15, t7);
    ENGINE_LEAVE(16, (u32)p);
    ENGINE_LEAVE(17, (u32)end);
    ENGINE_LEAVE(18, 1);
}

/* the spline at t: D_803B3778's columns weighted by the four points
   (f0, f2, f4, f6), times t^3, t^2, t and 1, summed ($f8) */
REGS(f0, f2, f4, f6, f30 -> f8)
f32 func_8029E878(f32 p0, f32 p1, f32 p2, f32 p3, f32 t) {
    f32 *m = D_803B3778, s, a, r = 0.0f;
    s32 c;

    ENGINE_BLK(8029E878);
    for (c = 3;; c--, m++) {
        ENGINE_BLK(8029E894);
        s = m[0] * p0;
        a = m[4] * p1;
        s = s + a;
        a = m[8] * p2;
        s = s + a;
        a = m[12] * p3;
        s = s + a;
        if (c == 3) {
            ENGINE_BLK(8029E8E8);
            r = s * D_803B37BC;
        } else {
            ENGINE_BLK(8029E8CC);
            if (c == 2) {
                ENGINE_BLK(8029E8FC);
                s = s * D_803B37B8;
                r = r + s;
            } else {
                ENGINE_BLK(8029E8D8);
                if (c != 1) {
                    ENGINE_BLK(8029E8E0);
                    r = r + s;
                    break;
                }
                ENGINE_BLK(8029E910);
                s = s * t;
                r = r + s;
            }
        }
        ENGINE_BLK(8029E918);
    }
    ENGINE_BLK(8029E924);
    ENGINE_LEAVE(1, 1);
    return r;
}

extern s32 D_803BE724, D_803BE728;
extern s16 D_803BE72C;
extern s16 D_803A7418[2];

/* D_803A7418 = the cell of (x, z): x * 4 / D_803BE724 plus the row
   (z * 4 / D_803BE728) times D_803BE72C; then -1 */
REGS(t3, t5)
void func_8029C284(s32 x, s32 z) {
    s32 cx, cz;

    ENGINE_BLK(8029C284);
    ENGINE_DIV(cx, (s32)((u32)x << 2), D_803BE724, 8029C2AC, 8029C2B0, 8029C2BC, 8029C2C4);
    ENGINE_BLK(8029C2C8);
    ENGINE_DIV(cz, (s32)((u32)z << 2), D_803BE728, 8029C2F0, 8029C2F4, 8029C300, 8029C308);
    ENGINE_BLK(8029C30C);
    D_803A7418[0] = cx + (s32)((u32)cz * (u32)(s32)D_803BE72C);
    D_803A7418[1] = -1;
    ENGINE_LEAVE(15, -1);
}

extern u8 *PTR32 D_803B35F0;
extern u8 D_803B3500[];
extern s32 D_802C23B4[];                /* their ids, to -1 */

/* D_803A7440's records unused; D_803B35F8's set up from D_802C23B4 */
void func_8029DEA0(void) {
    u8 *p = D_803A7440;
    s32 n = 12, *id = D_802C23B4, w;

    ENGINE_BLK(8029DEA0);
    for (;;) {
        ENGINE_BLK(8029DECC);
        if (n == 0)
            break;
        ENGINE_BLK(8029DED4);
        p[6] = 0;
        p += 0x1010;
        n--;
    }
    ENGINE_BLK(8029DEE4);
    D_803B35F0 = D_803B3500;
    for (p = (u8 *)D_803B35F8;; p += 0x18) {
        ENGINE_BLK(8029DF18);
        w = *id;
        *(s32 *)p = w;
        if (w == -1)
            break;
        ENGINE_BLK(8029DF28);
        id++;
        *(f32 *)(p + 4) = 0.0f;
        *(s16 *)(p + 0xC) = 0;
        *(s16 *)(p + 0xE) = 0;
        p[0x10] = 0;
        p[0x11] = 0;
        p[0x12] = 0;
        p[0x13] = 0;
        p[0x14] = 0;
    }
    ENGINE_BLK(8029DF54);
    ENGINE_LEAVE_F(0, 0.0f);
}

extern s32 D_803B3730[16];              /* a part's matrix (16.16) */

/* D_803B3730 = the translation from (x0, y0, z0) the fraction t of the
   way to (x1, y1, z1), unless that is 0 ($a0: 0x10000, or 0) */
REGS(a0, a1, a2, a3, t0, t1, f30 -> a0)
s32 func_8029F3D0(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1, f32 t) {
    s32 res;
    s32 x, y, z, *m = D_803B3730;
    f32 fx;

    ENGINE_BLK(8029F3D0);
    fx = (f32)(x1 - x0) * t;
    x = engine_cvt_w_s(fx);
    ENGINE_LEAVE_FW(2, x);
    y = engine_cvt_w_s((f32)(y1 - y0) * t) + y0;
    z = engine_cvt_w_s((f32)(z1 - z0) * t) + z0;
    x += x0;
    if (x == 0) {
        ENGINE_BLK(8029F434);
        if (y == 0) {
            ENGINE_BLK(8029F43C);
            if (z == 0) {
                ENGINE_BLK(8029F444);
                res = 0;
                goto done;
            }
        }
    }
    ENGINE_BLK(8029F44C);
    m[0] = 0x10000, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = 0x10000, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = 0x10000, m[11] = 0;
    m[12] = (u32)x << 16, m[13] = (u32)y << 16, m[14] = (u32)z << 16, m[15] = 0x10000;
    res = 0x10000;
done:
    ENGINE_BLK(8029F4A4);
    return res;
}

/* D_803B3730 = the rotation about x (F4B8), y (F560) or z (F608) by the
   angle from a to b the fraction t of the way (func_8029F6B0), unless
   that is 0 ($a0: 0x10000, or 0) */
REGS(a0, a1, f30 -> a0)
s32 func_8029F4B8(s32 a, s32 b, f32 t) {
    s32 res;
    s32 r, c, s, *m = D_803B3730;

    ENGINE_BLK(8029F4B8);
    r = func_8029F6B0(a, b, t);
    ENGINE_BLK(8029F4D4);
    if (r == 0) {
        ENGINE_BLK(8029F4DC);
        res = 0;
        goto done;
    }
    ENGINE_BLK(8029F4E4);
    c = func_802AE160((u32)r >> 4);
    ENGINE_BLK(8029F4EC);
    s = func_802AE104((u32)r >> 4);
    ENGINE_BLK(8029F4F4);
    m[0] = 0x10000, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = s, m[6] = c, m[7] = 0;
    m[8] = 0, m[9] = -c, m[10] = s, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    ENGINE_LEAVE(3, (u32)r >> 4);
    res = 0x10000;
    ENGINE_LEAVE(30, s);
done:
    ENGINE_BLK(8029F544);
    return res;
}

REGS(a0, a1, f30 -> a0)
s32 func_8029F560(s32 a, s32 b, f32 t) {
    s32 res;
    s32 r, c, s, *m = D_803B3730;

    ENGINE_BLK(8029F560);
    r = func_8029F6B0(a, b, t);
    ENGINE_BLK(8029F57C);
    if (r == 0) {
        ENGINE_BLK(8029F584);
        res = 0;
        goto done;
    }
    ENGINE_BLK(8029F58C);
    c = func_802AE160((u32)r >> 4);
    ENGINE_BLK(8029F594);
    s = func_802AE104((u32)r >> 4);
    ENGINE_BLK(8029F59C);
    m[0] = s, m[1] = 0, m[2] = -c, m[3] = 0;
    m[4] = 0, m[5] = 0x10000, m[6] = 0, m[7] = 0;
    m[8] = c, m[9] = 0, m[10] = s, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    ENGINE_LEAVE(3, (u32)r >> 4);
    res = 0x10000;
    ENGINE_LEAVE(30, s);
done:
    ENGINE_BLK(8029F5EC);
    return res;
}

REGS(a0, a1, f30 -> a0)
s32 func_8029F608(s32 a, s32 b, f32 t) {
    s32 res;
    s32 r, c, s, *m = D_803B3730;

    ENGINE_BLK(8029F608);
    r = func_8029F6B0(a, b, t);
    ENGINE_BLK(8029F624);
    if (r == 0) {
        ENGINE_BLK(8029F62C);
        res = 0;
        goto done;
    }
    ENGINE_BLK(8029F634);
    c = func_802AE160((u32)r >> 4);
    ENGINE_BLK(8029F63C);
    s = func_802AE104((u32)r >> 4);
    ENGINE_BLK(8029F644);
    m[0] = s, m[1] = c, m[2] = 0, m[3] = 0;
    m[4] = -c, m[5] = s, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = 0x10000, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    ENGINE_LEAVE(3, (u32)r >> 4);
    res = 0x10000;
    ENGINE_LEAVE(30, s);
done:
    ENGINE_BLK(8029F694);
    return res;
}

/* the angle of four keys' halves at off on the spline at t (func_8029E878),
   as a word; the keys' first two as floats in *k0, *k1 */
static s32 spline_angle(u8 *p0, u8 *p1, u8 *p2, u8 *p3, s32 off, f32 t, f32 *k0, f32 *k1) {
    *k0 = (f32) * (s16 *)(p0 + off);
    *k1 = (f32) * (s16 *)(p1 + off);
    return engine_cvt_w_s(func_8029E878(*k0, *k1, (f32) * (s16 *)(p2 + off), (f32) * (s16 *)(p3 + off), t));
}

/* D_803B3730 = the rotation about x (E938), y (EA48) or z (EB58) by the
   keys' angle (halves 6, 8 and 0xA) on the spline at t; $a3 whether it
   isn't 0 (then D_803B3730 is as it was) */
REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029E938(u8 *p0, u8 *p1, u8 *p2, u8 *p3, f32 t) {
    s32 a, c, s, *m = D_803B3730;
    f32 k0, k1;

    ENGINE_BLK(8029E938);
    a = spline_angle(p0, p1, p2, p3, 6, t, &k0, &k1);
    ENGINE_BLK(8029E998);
    if (a == 0) {
        ENGINE_BLK(8029E9AC);
        ENGINE_LEAVE_F(0, k0);
        ENGINE_LEAVE_F(2, k1);
        ENGINE_LEAVE_FW(8, 0);
        ENGINE_BLK(8029EA18);
        return 0;
    }
    ENGINE_BLK(8029E9B4);
    c = func_802AE160((u32)a >> 4);
    ENGINE_BLK(8029E9BC);
    s = func_802AE104((u32)a >> 4);
    ENGINE_BLK(8029E9C4);
    m[0] = 0x10000, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = s, m[6] = c, m[7] = 0;
    m[8] = 0, m[9] = -c, m[10] = s, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    ENGINE_BLK(8029EA18);
    return 1;
}

REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EA48(u8 *p0, u8 *p1, u8 *p2, u8 *p3, f32 t) {
    s32 a, c, s, *m = D_803B3730;
    f32 k0, k1;

    ENGINE_BLK(8029EA48);
    a = spline_angle(p0, p1, p2, p3, 8, t, &k0, &k1);
    ENGINE_BLK(8029EAA8);
    if (a == 0) {
        ENGINE_BLK(8029EABC);
        ENGINE_LEAVE_F(0, k0);
        ENGINE_LEAVE_F(2, k1);
        ENGINE_LEAVE_FW(8, 0);
        ENGINE_BLK(8029EB28);
        return 0;
    }
    ENGINE_BLK(8029EAC4);
    c = func_802AE160((u32)a >> 4);
    ENGINE_BLK(8029EACC);
    s = func_802AE104((u32)a >> 4);
    ENGINE_BLK(8029EAD4);
    m[0] = s, m[1] = 0, m[2] = -c, m[3] = 0;
    m[4] = 0, m[5] = 0x10000, m[6] = 0, m[7] = 0;
    m[8] = c, m[9] = 0, m[10] = s, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    ENGINE_BLK(8029EB28);
    return 1;
}

REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EB58(u8 *p0, u8 *p1, u8 *p2, u8 *p3, f32 t) {
    s32 a, c, s, *m = D_803B3730;
    f32 k0, k1;

    ENGINE_BLK(8029EB58);
    a = spline_angle(p0, p1, p2, p3, 0xA, t, &k0, &k1);
    ENGINE_BLK(8029EBB8);
    if (a == 0) {
        ENGINE_BLK(8029EBCC);
        ENGINE_LEAVE_F(0, k0);
        ENGINE_LEAVE_F(2, k1);
        ENGINE_LEAVE_FW(8, 0);
        ENGINE_BLK(8029EC38);
        return 0;
    }
    ENGINE_BLK(8029EBD4);
    c = func_802AE160((u32)a >> 4);
    ENGINE_BLK(8029EBDC);
    s = func_802AE104((u32)a >> 4);
    ENGINE_BLK(8029EBE4);
    m[0] = s, m[1] = c, m[2] = 0, m[3] = 0;
    m[4] = -c, m[5] = s, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = 0x10000, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    ENGINE_BLK(8029EC38);
    return 1;
}

/* D_803B3730 = the scale (in 256ths) from (x0, y0, z0) the fraction t of
   the way to (x1, y1, z1), unless that is (0x100, 0x100, 0x100) ($a0:
   0x10000, or 0) */
REGS(a0, a1, a2, a3, t0, t1, f30 -> a0)
s32 func_8029F760(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1, f32 t) {
    s32 res;
    s32 x, y, z, *m = D_803B3730;

    ENGINE_BLK(8029F760);
    x = engine_cvt_w_s((f32)(x1 - x0) * t);
    y = engine_cvt_w_s((f32)(y1 - y0) * t);
    ENGINE_LEAVE_FW(0, x);
    ENGINE_LEAVE_FW(2, y);
    z = engine_cvt_w_s((f32)(z1 - z0) * t) + z0;
    x += x0;
    y += y0;
    if (x == 0x100) {
        ENGINE_BLK(8029F7C8);
        if (y == 0x100) {
            ENGINE_BLK(8029F7D4);
            if (z == 0x100) {
                ENGINE_BLK(8029F7DC);
                res = 0;
                goto done;
            }
        }
    }
    ENGINE_BLK(8029F7E4);
    x = (s32)((u32)x << 16) >> 8;
    y = (s32)((u32)y << 16) >> 8;
    z = (s32)((u32)z << 16) >> 8;
    m[0] = x, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = y, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = z, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    res = 0x10000;
done:
    ENGINE_BLK(8029F848);
    ENGINE_LEAVE(7, x);
    ENGINE_LEAVE(8, y);
    ENGINE_LEAVE(9, z);
    return res;
}

extern u8 D_803A7300[];                 /* 0x14-byte records, free with byte 0x11 -1 */

/* An object's list (u16s from p to end) into D_803A7300 (its first, the
   size) and D_803A6B30's free records (then one per entry: half 8, and
   half 6 as the size), the sizes * 32 * scale / 0x10000, all of id */
REGS(t0, t1, t2, t3)
void func_8029C354(s32 id, u16 *p, u16 *end, u32 scale) {
    u8 *r = D_803A7300;
    u32 size;

    ENGINE_BLK(8029C354);
    size = ((u32)p[0] << 5) * scale / 0x10000;
    for (;;) {
        ENGINE_BLK(8029C3A8);
        if ((s8)r[0x11] == -1)
            break;
        r += 0x14;
    }
    ENGINE_BLK(8029C3B4);
    *(u32 *)(r + 0xC) = size;
    r[0x10] = id;
    r[0x11] = 0;
    p += 2;
    for (r = D_803A6B30;;) {
        ENGINE_BLK(8029C3CC);
        if ((s8)r[0x13] == -1)
            break;
        r += 0x14;
    }
    for (;;) {
        ENGINE_BLK(8029C3D8);
        if (p == end)
            break;
        ENGINE_BLK(8029C3E0);
        r[0x12] = id;
        r[0x13] = 0;
        *(u16 *)(r + 0x10) = p[4];
        *(u32 *)(r + 0xC) = ((u32)p[3] << 5) * scale / 0x10000;
        r += 0x14;
        ENGINE_BLK(8029C42C);
        p = (u16 *)((u8 *)p + (u32)p[5] * 4 + 0xC);
    }
    ENGINE_BLK(8029C43C);
}

extern s32 D_803A73FC, D_803A7400, D_803A7404;

/* a doubleword of game memory */
static s64 dword(u8 *p) {
    return (s64)((u64) * (u32 *)p << 32 | *(u32 *)(p + 4));
}

/* Whether (x, y, z) is within lim of the part's plane (a, b, c, d at
   0-0x18, the distance's scale at 0x20); if so, its foot on the plane
   (along the normal, over 0x24) in D_803A73FC-D_803A7404 ($v0) */
REGS(t3, t4, t5, t6, s0 -> v0, v1, a0, a1)
s32 func_8029C160(s32 x, s32 y, s32 z, s32 lim, struct Piece *piece, s32 *px, s32 *py, s32 *pz) {
    u8 *pl = (u8 *)piece;
    s64 a = dword(pl), b = dword(pl + 8), c = dword(pl + 0x10), d = dword(pl + 0x18);
    s64 s1, s2, s3, l2, l4, l6, rx, ry, rz;
    f32 f0, f2, f4, f6;

    ENGINE_BLK(8029C160);
    s1 = (s64)((u64)a * (u64)(s64)x + (u64)b * (u64)(s64)y + (u64)c * (u64)(s64)z);
    s2 = (s64)((u64)s1 + (u64)d);
    f0 = (f32)s2 / *(f32 *)(pl + 0x20);
    s2 = engine_cvt_l_s(f0);
    /* ($f0:$f1, unless it goes on) */
    ENGINE_LEAVE_FW(0, (u32)s2);
    ENGINE_LEAVE_FW(1, (u32)((u64)s2 >> 32));
    if (s2 < 0) {
        ENGINE_BLK(8029C1D4);
        s2 = -s2;
    }
    ENGINE_BLK(8029C1D8);
    ENGINE_LEAVE64(18, s2);
    if (lim < s2) {
        ENGINE_BLK(8029C26C);
        ENGINE_LEAVE(1, 1);
        ENGINE_LEAVE64(3, b);
        ENGINE_LEAVE64(4, c);
        ENGINE_LEAVE64(19, d);
        ENGINE_LEAVE_F(2, *(f32 *)(pl + 0x20));
        ENGINE_BLK(8029C270);
        /* ($v1, $a0 as they were loaded, $a1 as it came) */
        *px = (s32)b;
        *py = (s32)c;
        *pz = engine_ctx(5);
        return 0;
    }
    ENGINE_BLK(8029C1E4);
    s3 = (s64)(0 - (u64)d - (u64)s1);
    f0 = (f32)s3 / *(f32 *)(pl + 0x24);
    f2 = f0 * (f32)a;
    f4 = f0 * (f32)b;
    f6 = f0 * (f32)c;
    l2 = engine_cvt_l_s(f2);
    l4 = engine_cvt_l_s(f4);
    l6 = engine_cvt_l_s(f6);
    rx = (s64)((u64)l2 + (u64)(s64)x);
    ry = (s64)((u64)l4 + (u64)(s64)y);
    rz = (s64)((u64)l6 + (u64)(s64)z);
    D_803A73FC = (s32)rx;
    D_803A7400 = (s32)ry;
    D_803A7404 = (s32)rz;
    /* (what it leaves: the foot as 64 bits, the conversions) */
    ENGINE_LEAVE(1, ((u32)&D_803A7404 + 0x8000) & 0xFFFF0000);
    ENGINE_LEAVE64(3, rx);
    ENGINE_LEAVE64(4, ry);
    ENGINE_LEAVE64(5, rz);
    ENGINE_LEAVE64(19, s3);
    ENGINE_LEAVE_F(0, f0);
    ENGINE_LEAVE_FW(1, (u32)((u64)s3 >> 32));
    ENGINE_LEAVE_FW(2, (u32)l2);
    ENGINE_LEAVE_FW(3, (u32)((u64)l2 >> 32));
    ENGINE_LEAVE_FW(4, (u32)l4);
    ENGINE_LEAVE_FW(5, (u32)((u64)l4 >> 32));
    ENGINE_LEAVE_FW(6, (u32)l6);
    ENGINE_LEAVE_FW(7, (u32)((u64)l6 >> 32));
    ENGINE_BLK(8029C270);
    *px = (s32)rx;
    *py = (s32)ry;
    *pz = (s32)rz;
    return 1;
}

/* Whether the point (x, z) at height y misses all of the n triangles at
   tris (0x18 bytes: three (x, z) words) and the height range after them
   (two halves, the range wrapping when the second is lower) ($t4: 1 if
   it misses, 0 if it is in) */
REGS(s4, t5, t0, t1, a1 -> t4)
s32 func_8029DA90(s32 *tris, s32 n, s32 x, s32 z, s32 y) {
    s32 *t = tris, lo, hi, r = 1, read = 0, found = 0;

    ENGINE_BLK(8029DA90);
    for (;;) {
        ENGINE_BLK(8029DABC);
        if (n == 0)
            break;
        ENGINE_BLK(8029DAC4);
        n--;
        read = 1;
        t += 6;
        if (!func_802AA5E0(x, z, t[-6], t[-5], t[-4], t[-3], t[-2], t[-1])) {
            ENGINE_BLK(8029DAE8);
            continue;
        }
        ENGINE_BLK(8029DAE8);
        ENGINE_BLK(8029DAF0);
        if (!func_802AA460(x, z, t[-6], t[-5], t[-4], t[-3], t[-2], t[-1])) {
            ENGINE_BLK(8029DAF8);
            continue;
        }
        ENGINE_BLK(8029DAF8);
        ENGINE_BLK(8029DB00);
        n = (u32)n * 0x18;
        /* (the halves of a word func_802A3F80 stores: through the word,
           which is how they are in native-endian memory) */
        lo = *(u32 *)((u8 *)t + n) >> 16;
        hi = *(u32 *)((u8 *)t + n) & 0xFFFF;
        if (hi < lo) {
            ENGINE_BLK(8029DB24);
            if (!(y < lo))
                goto in;
            ENGINE_BLK(8029DB2C);
            if (!(hi < y))
                goto in;
            ENGINE_BLK(8029DB34);
            goto hit;
        }
        ENGINE_BLK(8029DB3C);
        if (y < lo)
            goto hit;
        ENGINE_BLK(8029DB48);
        if (hi < y)
            goto hit;
    in:
        ENGINE_BLK(8029DB50);
        r = 0;
    hit:
        found = 1;
        break;
    }
    ENGINE_BLK(8029DB54);
    /* (what it leaves: the last triangle's words, or the range) */
    if (found) {
        ENGINE_LEAVE(1, hi < y);
        ENGINE_LEAVE(17, lo);
        ENGINE_LEAVE(19, hi);
    } else if (read) {
        ENGINE_LEAVE(17, t[-6]);
        ENGINE_LEAVE(19, t[-5]);
    }
    if (read) {
        ENGINE_LEAVE(22, t[-3]);
        ENGINE_LEAVE(23, t[-2]);
        ENGINE_LEAVE(25, t[-1]);
    }
    ENGINE_LEAVE(13, n);
    return r;
}

extern u8 D_803A742D, D_803A742E;
extern s32 D_80358064;

/* The gears (unkA0): D_803A742D (to 0xC8) with D_803A742E set, else one
   down to 1; unk76 10 when it is 0 and D_80358064; and with unk76
   negative, the camera's headings turned half way round */
REGS(gp)
void func_8029A914(VS *vs) {
    s32 g, h;

    ENGINE_BLK(8029A914);
    if (D_803A742E != 0) {
        ENGINE_BLK(8029A93C);
        g = D_803A742D;
        if (g >= 0xC9) {
            ENGINE_BLK(8029A950);
            g = 0xC8;
        }
        ENGINE_BLK(8029A954);
        vs->unkA0 = g;
    } else {
        ENGINE_BLK(8029A95C);
        g = vs->unkA0;
        if (g != 1) {
            ENGINE_BLK(8029A96C);
            vs->unkA0 = g - 1;
        }
    }
    ENGINE_BLK(8029A974);
    if (vs->unk76 == 0) {
        ENGINE_BLK(8029A980);
        if (D_80358064 != 0) {
            ENGINE_BLK(8029A990);
            vs->unk76 = 10;
        }
    }
    ENGINE_BLK(8029A998);
    if (vs->unk76 < 0) {
        ENGINE_BLK(8029A9A4);
        h = (u16)D_803A7410 - 0x800;
        if (h < 0) {
            ENGINE_BLK(8029A9C4);
            h += 0xFFF;
        }
        ENGINE_BLK(8029A9C8);
        D_803A7410 = h;
        h = (u16)D_803A7412 + 0x800;
        if (0xFFF < h) {
            ENGINE_BLK(8029A9E8);
            h -= 0xFFF;
        }
        ENGINE_BLK(8029A9EC);
        D_803A7412 = h;
    }
    ENGINE_BLK(8029A9F0);
}

/* For each texture record of D_802C23B4 for this id (byte 0; its first
   texture number, a half at 4 + byte 1 * 2), the display list's
   G_SETTIMG commands (0xFD) from dl to end that load it: (record, the
   command's offset + 4, 0) appended at D_803B35F0 */
REGS(s2, s0, s1)
void func_8029DF78(s32 id, u32 *dl, u32 *end) {
    s32 *ids = D_802C23B4, k;
    u8 *rec, *out = D_803B35F0;
    u32 *p, w, tex;

    ENGINE_BLK(8029DF78);
    for (;;) {
        ENGINE_BLK(8029DFC8);
        if (*ids == -1)
            break;
        ENGINE_BLK(8029DFD8);
        rec = *(u8 *PTR32 *)ids;
        ids++;
        if (rec[0] != id)
            continue;
        ENGINE_BLK(8029DFE8);
        k = -1;
        if (rec[2] == 0)
            continue;
        ENGINE_BLK(8029E008);
        k++;
        tex = *(u16 *)(rec + rec[1] * 2 + 4);
        for (p = dl;;) {
            ENGINE_BLK(8029E020);
            if (p == end)
                break;
            ENGINE_BLK(8029E028);
            w = p[0];
            p += 2;
            if ((w & 0xFF000000) >> 24 != 0xFD)
                continue;
            ENGINE_BLK(8029E044);
            if (p[-1] != tex)
                continue;
            ENGINE_BLK(8029E050);
            *(u8 *PTR32 *)out = rec;
            ((u32 *)out)[1] = (u8 *)p - (u8 *)dl - 4;
            ((u32 *)out)[2] = k;
            out += 0xC;
        }
    }
    ENGINE_BLK(8029E06C);
    D_803B35F0 = out;
}

/* D_803B3730 = the translation by the keys' point (halves 0xC, 0xE,
   0x10) on the spline at t; $a3 whether it isn't 0 (then D_803B3730 is
   as it was) */
REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EC68(u8 *p0, u8 *p1, u8 *p2, u8 *p3, f32 t) {
    s32 x, y, z, *m = D_803B3730;
    f32 k0, k1;

    ENGINE_BLK(8029EC68);
    x = spline_angle(p0, p1, p2, p3, 0xC, t, &k0, &k1);
    ENGINE_BLK(8029ECC4);
    y = spline_angle(p0, p1, p2, p3, 0xE, t, &k0, &k1);
    ENGINE_BLK(8029ED00);
    z = spline_angle(p0, p1, p2, p3, 0x10, t, &k0, &k1);
    ENGINE_BLK(8029ED3C);
    ENGINE_LEAVE_F(0, k0);
    ENGINE_LEAVE_F(2, k1);
    ENGINE_LEAVE_FW(8, z);
    if (x == 0) {
        ENGINE_BLK(8029ED4C);
        if (y == 0) {
            ENGINE_BLK(8029ED54);
            if (z == 0) {
                ENGINE_BLK(8029ED5C);
                ENGINE_BLK(8029EDC0);
                return 0;
            }
        }
    }
    ENGINE_BLK(8029ED64);
    m[0] = 0x10000, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = 0x10000, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = 0x10000, m[11] = 0;
    m[12] = (u32)x << 16, m[13] = (u32)y << 16, m[14] = (u32)z << 16, m[15] = 0x10000;
    ENGINE_BLK(8029EDC0);
    return 1;
}

/* D_803B3730 = the scale (in 256ths) by the keys' halves 0, 2 and 4 on
   the spline at t; $a3 whether it isn't 0x100 each way */
REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EDEC(u8 *p0, u8 *p1, u8 *p2, u8 *p3, f32 t) {
    s32 x, y, z, *m = D_803B3730;
    f32 k0, k1;

    ENGINE_BLK(8029EDEC);
    x = spline_angle(p0, p1, p2, p3, 0, t, &k0, &k1);
    ENGINE_BLK(8029EE48);
    y = spline_angle(p0, p1, p2, p3, 2, t, &k0, &k1);
    ENGINE_BLK(8029EE84);
    z = spline_angle(p0, p1, p2, p3, 4, t, &k0, &k1);
    ENGINE_BLK(8029EEC0);
    ENGINE_LEAVE_F(0, k0);
    ENGINE_LEAVE_F(2, k1);
    ENGINE_LEAVE_FW(8, z);
    if (x == 0x100) {
        ENGINE_BLK(8029EED4);
        if (y == 0x100) {
            ENGINE_BLK(8029EEDC);
            if (z == 0x100) {
                ENGINE_BLK(8029EEE4);
                ENGINE_BLK(8029EF54);
                return 0;
            }
        }
    }
    ENGINE_BLK(8029EEEC);
    m[0] = (s32)((u32)x << 16) >> 8, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = (s32)((u32)y << 16) >> 8, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = (s32)((u32)z << 16) >> 8, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0x10000;
    ENGINE_BLK(8029EF54);
    return 1;
}

/* A part's frame from its two key frames (k: scale, rotation x, y, z,
   translation as s16s, the second key 0x14 on), the fraction t of the way:
   each matrix that isn't the identity into acc (func_802ACCCC) in the
   order scale, z, y, x, translation */
/* (fp: what it puts back, as the original reloads it; its rotations
   leave theirs) */
REGS(s3, s2, f30, fp)
void func_8029EF80(s16 *k, s32 *acc, f32 t, s32 fp) {
    s32 r;

    ENGINE_BLK(8029EF80);
    r = func_8029F760(k[0], k[1], k[2], k[10], k[11], k[12], t);
    ENGINE_BLK(8029EFA8);
    if (r != 0) {
        ENGINE_BLK(8029EFB0);
        func_802ACCCC(D_803B3730, acc);
    }
    ENGINE_BLK(8029EFBC);
    r = func_8029F608(k[5], k[15], t);
    ENGINE_BLK(8029EFC8);
    if (r != 0) {
        ENGINE_BLK(8029EFD0);
        func_802ACCCC(D_803B3730, acc);
    }
    ENGINE_BLK(8029EFDC);
    r = func_8029F560(k[4], k[14], t);
    ENGINE_BLK(8029EFE8);
    if (r != 0) {
        ENGINE_BLK(8029EFF0);
        func_802ACCCC(D_803B3730, acc);
    }
    ENGINE_BLK(8029EFFC);
    r = func_8029F4B8(k[3], k[13], t);
    ENGINE_BLK(8029F008);
    if (r != 0) {
        ENGINE_BLK(8029F010);
        func_802ACCCC(D_803B3730, acc);
    }
    ENGINE_BLK(8029F01C);
    r = func_8029F3D0(k[6], k[7], k[8], k[16], k[17], k[18], t);
    ENGINE_BLK(8029F038);
    if (r != 0) {
        ENGINE_BLK(8029F040);
        func_802ACCCC(D_803B3730, acc);
        ENGINE_LEAVE(4, (u32)D_803B3730);
    } else {
        /* (what the last call was given) */
        ENGINE_LEAVE(4, 0);
        ENGINE_LEAVE(5, k[7]);
        ENGINE_LEAVE(6, k[8]);
    }
    ENGINE_BLK(8029F04C);
    ENGINE_LEAVE(30, fp);
}

/* the same along a spline: the four keys (D_803B7FC0-D_803B7FC3 of the
   0x14-byte records 8 on from s1) at t, in the order scale, z, y, x,
   translation */
/* (a3 likewise, which func_802ACCCC leaves) */
REGS(s1, s2, f30, a3)
void func_8029E730(u8 *keys, s32 *acc, f32 t, s32 a3) {
    u8 *k = keys + 8, *p0 = k + D_803B7FC0 * 0x14, *p1 = k + D_803B7FC1 * 0x14, *p2 = k + D_803B7FC2 * 0x14,
       *p3 = k + D_803B7FC3 * 0x14;

    ENGINE_BLK(8029E730);
    if (func_8029EDEC(p0, p1, p2, p3, t)) {
        ENGINE_BLK(8029E7C4);
        ENGINE_BLK(8029E7CC);
        func_802ACCCC(D_803B3730, acc);
    } else {
        ENGINE_BLK(8029E7C4);
    }
    ENGINE_BLK(8029E7D8);
    if (func_8029EB58(p0, p1, p2, p3, t)) {
        ENGINE_BLK(8029E7E0);
        ENGINE_BLK(8029E7E8);
        func_802ACCCC(D_803B3730, acc);
    } else {
        ENGINE_BLK(8029E7E0);
    }
    ENGINE_BLK(8029E7F4);
    if (func_8029EA48(p0, p1, p2, p3, t)) {
        ENGINE_BLK(8029E7FC);
        ENGINE_BLK(8029E804);
        func_802ACCCC(D_803B3730, acc);
    } else {
        ENGINE_BLK(8029E7FC);
    }
    ENGINE_BLK(8029E810);
    if (func_8029E938(p0, p1, p2, p3, t)) {
        ENGINE_BLK(8029E818);
        ENGINE_BLK(8029E820);
        func_802ACCCC(D_803B3730, acc);
    } else {
        ENGINE_BLK(8029E818);
    }
    ENGINE_BLK(8029E82C);
    if (func_8029EC68(p0, p1, p2, p3, t)) {
        ENGINE_BLK(8029E834);
        ENGINE_BLK(8029E83C);
        func_802ACCCC(D_803B3730, acc);
    } else {
        ENGINE_BLK(8029E834);
    }
    ENGINE_BLK(8029E848);
    ENGINE_LEAVE(7, a3);
}

/* An animation's step (its state at a: 4 the fraction, 0xC the loops so
   far, 0xE their limit (-1 none), 0x10 running, 0x11 backwards, 0x12 the
   mode (0 wraps, 1 stops at the end), 0x13 the key, 0x14 the speed):
   the fraction t on by rate * speed, the key k on by its whole part, over
   n keys.  At an end it wraps (mode 0) or stops there (1, and it turns
   round), and when the loops reach their limit it stops (0x10 clear). */
REGS(t0, t2, t6, f6, f30 -> t2, f30)
s32 func_8029F1BC(u8 *a, s32 k, s32 n, f32 rate, f32 t, f32 *t_out) {
    s32 speed = a[0x14], back = (s8)a[0x11], mode = a[0x12], w, c, lim, at = 1, s2set = 0;
    f32 fspeed = (f32)speed, f, f2w;
    u32 f2;

    ENGINE_BLK(8029F1BC);
    rate = rate * fspeed;
    if (back != 1) {
        ENGINE_BLK(8029F1E8);
        t = t + rate;
        w = engine_trunc_w_s(t);
        f2 = w;
        t = t - (f32)w;
        k += w;
        if (k < n - 1)
            goto done;
        ENGINE_BLK(8029F20C);
        if (n - 1 == 0) {
            ENGINE_BLK(8029F21C);
            engine_break(0x8029F21C, 7);
        }
        ENGINE_BLK(8029F220);
        c = *(s16 *)(a + 0xC) + (s32)((u32)k / (u32)(n - 1));
        lim = *(s16 *)(a + 0xE);
        *(s16 *)(a + 0xC) = c;
        if (!(c < lim)) {
            ENGINE_BLK(8029F244);
            if (lim != -1) {
                ENGINE_BLK(8029F24C);
                t = 1.0f;
                k = n - 2;
                if (mode == 1) {
                    ENGINE_BLK(8029F25C);
                    back = 1;
                }
                ENGINE_BLK(8029F260);
                s2set = 1;
                a[0x10] = 0;
                goto done;
            }
        }
        ENGINE_BLK(8029F26C);
        if (mode != 0) {
            ENGINE_BLK(8029F274);
            if (mode != 1) {
                ENGINE_BLK(8029F27C);
                engine_syscall(0x8029F27C);
            }
            ENGINE_BLK(8029F2A4);
            t = 1.0f;
            at = 0x3F800000;            /* (its lui) */
            back = 1;
            k = n - 2;
            goto done;
        }
        ENGINE_BLK(8029F280);
        if (n - 1 == 0) {
            ENGINE_BLK(8029F298);
            engine_break(0x8029F298, 7);
        }
        k = (u32)k % (u32)(n - 1);
        ENGINE_BLK(8029F29C);
        goto done;
    }
    ENGINE_BLK(8029F2B8);
    t = t - rate;
    f2 = 0;                             /* (0.0f) */
    if (t <= 0.0f) {
        ENGINE_BLK(8029F2D0);
        f = (f32)engine_trunc_w_s(t);
        f = f - 1.0f;
        t = t - f;
        w = engine_cvt_w_s(f);
        f2 = w;
        k += w;
        at = 0x3F800000;
    }
    ENGINE_BLK(8029F2F8);
    if (k >= 0)
        goto done;
    ENGINE_BLK(8029F300);
    at = 1;
    c = *(s16 *)(a + 0xC) + back;
    lim = *(s16 *)(a + 0xE);
    if (n == 0) {
        ENGINE_BLK(8029F328);
        engine_break(0x8029F328, 7);
    }
    ENGINE_BLK(8029F32C);
    *(s16 *)(a + 0xC) = c;
    if (!(c < lim)) {
        ENGINE_BLK(8029F338);
        if (lim != -1) {
            ENGINE_BLK(8029F344);
            t = 0.0f;
            k = 0;
            if (mode == 1) {
                ENGINE_BLK(8029F354);
                back = 0;
            }
            ENGINE_BLK(8029F358);
            s2set = 1;
            a[0x10] = 0;
            goto done;
        }
    }
    ENGINE_BLK(8029F364);
    if (mode != 0) {
        ENGINE_BLK(8029F36C);
        if (mode != 1) {
            ENGINE_BLK(8029F374);
            engine_syscall(0x8029F374);
        }
        ENGINE_BLK(8029F3A8);
        t = 0.0f;
        back = 0;
        k = 0;
        goto done;
    }
    ENGINE_BLK(8029F378);
    if (n - 1 == 0) {
        ENGINE_BLK(8029F394);
        engine_break(0x8029F394, 7);
    }
    k = (u32)(-k) % (u32)(n - 1);
    ENGINE_BLK(8029F398);
    if (k != 0) {
        ENGINE_BLK(8029F3A0);
        k = (n - 1) - k;
    }
done:
    ENGINE_BLK(8029F3B4);
    a[0x11] = back;
    a[0x13] = k;
    *(f32 *)(a + 4) = t;
    /* (what it leaves) */
    ENGINE_LEAVE(1, at);
    ENGINE_LEAVE(10, k);
    ENGINE_LEAVE(12, speed);
    ENGINE_LEAVE(13, mode);
    ENGINE_LEAVE(15, back);
    if (s2set)
        ENGINE_LEAVE(18, 0);
    ENGINE_LEAVE_FW(2, f2);
    ENGINE_LEAVE_F(8, fspeed);
    ENGINE_LEAVE_F(30, t);
    *t_out = t;
    return k;
}

extern s32 D_803B3770;                  /* the parts' records' base */

/* An object's animation, one frame on: its state a (func_8029F1BC; the
   rate from its data's per-key speeds, / 300), then each part's matrix
   (base + the part's offset) from its rest matrix with the key frames'
   (mode 0, func_8029EF80) or the spline's (mode 1, func_8029E730) on top,
   to the RSP's form, and recorded (func_8029DCD4) or, when the animation
   has stopped, dropped (func_8029DD54).  Leaves its registers in the
   original's order, around its callees' (ENGINE_LEAVE). */
REGS(t0, v0, a3)
void func_8029E5AC(u8 *a, u8 *base, s32 a3) {
    u8 *d = *(u8 *PTR32 *)a, *rec, *dst, *src;
    s32 k = (s8)a[0x13], n, mode, running, parts, step, rem, i;
    u32 *s, *w;
    f32 t = *(f32 *)(a + 4), rate;

    ENGINE_BLK(8029E5AC);
    n = d[0];
    rate = (f32)(d[k + 2] - d[k + 1]) * t + (f32)d[k + 1];
    rate = rate / 300.0f;
    ENGINE_LEAVE(1, 0x43960000);
    ENGINE_LEAVE(14, n);
    k = func_8029F1BC(a, k, n, rate, t, &t);
    ENGINE_BLK(8029E604);
    mode = a[0x15];
    running = a[0x10];
    ENGINE_LEAVE(30, mode);
    ENGINE_LEAVE(12, running);
    if (mode != 0) {
        ENGINE_BLK(8029E614);
        func_8029F110(a, mode);
        ENGINE_BLK(8029E61C);
        func_8029F060(t, k, n);
    }
    ENGINE_BLK(8029E624);
    step = 0x14 * n + 8;
    rec = d + n + 2;
    parts = d[n + 1];
    rem = (u32)rec & 3;
    ENGINE_LEAVE(11, (u32)(d + n));
    ENGINE_LEAVE(21, 0x14);
    ENGINE_LEAVE(22, step);
    ENGINE_LEAVE(18, rem);
    if (rem != 0) {
        ENGINE_BLK(8029E64C);
        ENGINE_LEAVE(19, 4);
        ENGINE_LEAVE(18, 4 - rem);
        rec += 4 - rem;
    }
    for (;;) {
        ENGINE_BLK(8029E658);
        if (parts == 0)
            break;
        ENGINE_BLK(8029E660);
        dst = base + ((s32 *)rec)[0];
        src = base + ((s32 *)rec)[1];
        ENGINE_LEAVE(13, D_803B3770 + ((s32 *)rec)[0]);
        ENGINE_LEAVE(19, (u32)(rec + 8 + k * 0x14));
        ENGINE_LEAVE(20, k * 0x14);
        s = (u32 *)src;
        w = (u32 *)dst;
        for (i = 8; i != 0; i--) {
            ENGINE_BLK(8029E694);
            w[0] = s[0];
            w[1] = s[1];
            w += 2, s += 2;
        }
        ENGINE_BLK(8029E6AC);
        ENGINE_LEAVE(4, 0);
        ENGINE_LEAVE64(5, (u64)s[-2] << 32 | s[-1]);
        ENGINE_LEAVE(18, (u32)dst);
        ENGINE_LEAVE(23, (u32)(src + 0x40));
        if (mode == 0) {
            ENGINE_BLK(8029E6C4);
            func_8029EF80((s16 *)(rec + 8 + k * 0x14), (s32 *)dst, t, mode);
            ENGINE_BLK(8029E6CC);
        } else {
            ENGINE_BLK(8029E6B4);
            ENGINE_LEAVE(1, 1);
            if (mode != 1) {
                ENGINE_BLK(8029E6C0);
                engine_syscall(0x8029E6C0);
            }
            ENGINE_BLK(8029E6D4);
            func_8029E730(rec, (s32 *)dst, t, a3);
        }
        ENGINE_BLK(8029E6DC);
        ENGINE_LEAVE(4, (u32)(src + 0x40));
        func_802ACCCC((s32 *)(src + 0x40), (s32 *)dst);
        a3 = 0;                         /* (as func_802ACCCC leaves it) */
        ENGINE_BLK(8029E6E4);
        func_802AC8CC((u32 *)dst);
        ENGINE_BLK(8029E6EC);
        ENGINE_LEAVE(1, 1);
        if (running != 1) {
            ENGINE_BLK(8029E6F8);
            func_8029DCD4((u32)dst, D_803B3770 + ((s32 *)rec)[0]);
            ENGINE_BLK(8029E700);
        } else {
            ENGINE_BLK(8029E708);
            func_8029DD54((u32)dst);
        }
        ENGINE_BLK(8029E710);
        rec += step;
        parts--;
    }
    ENGINE_BLK(8029E71C);
    ENGINE_LEAVE(16, 0);
    ENGINE_LEAVE(17, (u32)rec);
}

REGS(v0, v1, a0, a1, a2, s4, s0, s1, s2 -> v0, v1, a0, s1, s2)
s32 func_802AA890(s32 x, s32 y, s32 z, s32 n, s32 *offs, u8 *base, s32 s0, s32 s1, s32 s2, s32 *y_out, s32 *z_out,
                  s32 *s1_out, s32 *s2_out);

/* An object's points (from p to end: its first (x, y, z), then records of
   three s16s, the matrices' count and offsets) into D_803A7300's record
   of id (the first, as it is) and D_803A6B30's from the first of id on
   (each through its matrices at base, func_802AA890) */
REGS(t0, t1, t2, v0, v1, a0, s4, s0, s1, s2)
void func_8029C454(s32 id, u8 *p, u8 *end, s32 x, s32 y, s32 z, u8 *base, s32 s0, s32 s1, s32 s2) {
    u8 *r = D_803A7300;
    s16 *h;
    s32 ry, rz, called = 0;

    ENGINE_BLK(8029C454);
    for (;;) {
        ENGINE_BLK(8029C480);
        if (r[0x10] == id)
            break;
        r += 0x14;
    }
    ENGINE_BLK(8029C48C);
    p += 4;
    ((s32 *)r)[0] = x;
    ((s32 *)r)[1] = y;
    ((s32 *)r)[2] = z;
    r[0x11] = 1;
    if (p == end)
        goto done;
    ENGINE_BLK(8029C4A8);
    for (r = D_803A6B30;;) {
        ENGINE_BLK(8029C4B0);
        if (r[0x12] == id)
            break;
        r += 0x14;
    }
    for (;;) {
        ENGINE_BLK(8029C4BC);
        if (p == end)
            break;
        ENGINE_BLK(8029C4C4);
        h = (s16 *)p;
        x = func_802AA890(h[0], h[1], h[2], (u16)h[5], (s32 *)(p + 0xC), base, s0, s1, s2, &ry, &rz, &s1, &s2);
        called = 1;
        ENGINE_BLK(8029C4DC);
        ((s32 *)r)[0] = x;
        ((s32 *)r)[1] = ry;
        ((s32 *)r)[2] = rz;
        r[0x13] = 1;
        p += (u16)h[5] * 4 + 0xC;
        r += 0x14;
    }
done:
    ENGINE_BLK(8029C504);
    if (called) {
        ENGINE_LEAVE(17, s1);
        ENGINE_LEAVE(18, s2);
    }
    ENGINE_LEAVE(9, (u32)p);
}

/* Whether (px, pz) is inside the triangle (x0, z0), (x1, z1), (x2, z2):
   on the same side of each edge as a point inside (between the third
   corner and the first edge's middle), an edge it lies on not counting
   ($t7) */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t7)
s32 func_8029BF64(s32 x0, s32 z0, s32 x1, s32 z1, s32 x2, s32 z2, s32 px, s32 pz) {
    f32 fpx = (f32)px, fpz = (f32)pz, cx, cz, f2 = 0.0f, f4, f14, f22 = 2.0f, f24 = 0.0f, f26 = 0.0f, f28;
    s32 e = 4, in = 1, at = 3, ex, ez, f24set = 0;

    ENGINE_BLK(8029BF64);
    cx = (f32)(x0 + x1) / 2.0f;
    cz = (f32)(z0 + z1) / 2.0f;
    cx = ((f32)x2 + cx) / 2.0f;
    cz = ((f32)z2 + cz) / 2.0f;
    for (;;) {
        ENGINE_BLK(8029BFE4);
        at = 3;
        if (--e == 0)
            break;
        ENGINE_BLK(8029BFF0);
        at = 2;                         /* (the delay slot's, either way) */
        if (e == 3) {
            ENGINE_BLK(8029C038);
            f2 = (f32)x0, f4 = (f32)z0;
            ez = z1 - z0, ex = x1 - x0;
        } else {
            if (e == 2) {
                ENGINE_BLK(8029BFF8);
                ENGINE_BLK(8029C01C);
                f2 = (f32)x0, f4 = (f32)z0;
                ez = z2 - z0, ex = x2 - x0;
            } else {
                ENGINE_BLK(8029BFF8);
                ENGINE_BLK(8029C000);
                f2 = (f32)x1, f4 = (f32)z1;
                ez = z2 - z1, ex = x2 - x1;
            }
        }
        ENGINE_BLK(8029C050);
        f26 = (f32)ez;
        f28 = (f32)ex;
        f14 = (fpx - f2) * f26;
        f14 = f14 - (fpz - f4) * f28;
        if (f14 == 0.0f)
            continue;
        ENGINE_BLK(8029C080);
        f22 = (cx - f2) * f26;
        f24 = (cz - f4) * f28;
        f24set = 1;
        f22 = f22 - f24;
        if (!(f14 > 0.0f)) {
            ENGINE_BLK(8029C09C);
            if (f22 < 0.0f)
                continue;
            ENGINE_BLK(8029C0A8);
        } else {
            ENGINE_BLK(8029C0B0);
            if (f22 > 0.0f)
                continue;
        }
        ENGINE_BLK(8029C0BC);
        in = 0;
        break;
    }
    ENGINE_BLK(8029C0C0);
    ENGINE_LEAVE(1, at);
    ENGINE_LEAVE(10, e);
    ENGINE_LEAVE_F(0, 0.0f);
    ENGINE_LEAVE_F(2, f2);
    ENGINE_LEAVE_F(20, cz);
    ENGINE_LEAVE_F(22, f22);
    if (f24set)
        ENGINE_LEAVE_F(24, f24);
    ENGINE_LEAVE_F(26, f26);
    return in;
}

/* An object's 32 parts from its model file (their data at the offsets of
   the u16 list at model + its word 0x10), then its matrix block (model +
   its word 0x18: a size, a length, the words) into both buffers, the rest
   of the size as identity matrices */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2) {
    u8 *p = (u8 *)parts, *base = model + *(s32 *)(model + 0x10), *blk, *end;
    u16 *off = (u16 *)base;
    s32 n, size;
    u32 w = 0, *s;

    ENGINE_BLK(8029F85C);
    for (n = 0x20; n != 0;) {
        ENGINE_BLK(8029F888);
        n--;
        *(f32 *)(p + 4) = 0.0f;
        *(u8 *PTR32 *)p = base + *off;
        *(s16 *)(p + 0xC) = 0;
        *(s16 *)(p + 0xE) = 0;
        p[0x10] = 0;
        p[0x11] = 0;
        p[0x12] = 0;
        p[0x13] = 0;
        p[0x14] = 0;
        p[0x15] = 0;
        p += 0x18;
        off++;
    }
    ENGINE_BLK(8029F8C8);
    blk = model + *(s32 *)(model + 0x18);
    size = *(s32 *)blk;
    end = blk + 8 + *(s32 *)(blk + 4);
    for (s = (u32 *)(blk + 8);;) {
        ENGINE_BLK(8029F8E4);
        if ((u8 *)s == end)
            break;
        ENGINE_BLK(8029F8EC);
        w = *s++;
        *(u32 *)buf1 = w;
        *(u32 *)buf2 = w;
        buf1 += 4, buf2 += 4;
        size -= 4;
    }
    for (;;) {
        ENGINE_BLK(8029F90C);
        if (size == 0)
            break;
        ENGINE_BLK(8029F914);
        /* (the identity Mtx; by words, since its halves are a word's in
           native-endian memory, tools/recomp/native_sites.txt) */
        for (n = 0; n < 2; n++) {
            u8 *m = n == 0 ? buf1 : buf2;

            *(u32 *)(m + 0x00) = 0x00010000;
            *(u32 *)(m + 0x04) = 0;
            *(u32 *)(m + 0x08) = 0x00000001;
            *(u32 *)(m + 0x0C) = 0;
            *(u32 *)(m + 0x10) = 0;
            *(u32 *)(m + 0x14) = 0x00010000;
            *(u32 *)(m + 0x18) = 0;
            *(u32 *)(m + 0x1C) = 0x00000001;
            *(u32 *)(m + 0x20) = 0;
            *(u32 *)(m + 0x24) = 0;
            *(u32 *)(m + 0x28) = 0;
            *(u32 *)(m + 0x2C) = 0;
            *(u32 *)(m + 0x30) = 0;
            *(u32 *)(m + 0x34) = 0;
            *(u32 *)(m + 0x38) = 0;
            *(u32 *)(m + 0x3C) = 0;
        }
        buf1 += 0x40, buf2 += 0x40;
        size -= 0x40;
    }
    ENGINE_BLK(8029F9C4);
    ENGINE_LEAVE(8, (u32)p);
    ENGINE_LEAVE(10, (u32)base);
    ENGINE_LEAVE(11, (u32)end);
    ENGINE_LEAVE(12, (u32)end);
    ENGINE_LEAVE(13, 0);
    ENGINE_LEAVE(14, w);
    ENGINE_LEAVE(15, 0);
    ENGINE_LEAVE(16, 1);
    ENGINE_LEAVE(17, 0);
    ENGINE_LEAVE_F(0, 0.0f);
}

/* The plane through the part's three points (0x28 on, each / 8): its
   normal (a, b, c) as the cross product of two edges, and d (64 bits) */
REGS(s0 -> a1, a2, a3, t0)
s64 func_8029D90C(u8 *part, s64 *b_out, s64 *c_out, s64 *d_out) {
    s32 *w = (s32 *)(part + 0x28);
    s32 x1 = w[0] >> 3, y1 = w[1] >> 3, z1 = w[2] >> 3, x2 = w[3] >> 3, y2 = w[4] >> 3, z2 = w[5] >> 3;
    s32 x3 = w[6] >> 3, y3 = w[7] >> 3, z3 = w[8] >> 3;
    s64 dy2 = y1 - y2, dz3 = z1 - z3, dz2 = z1 - z2, dy3 = y1 - y3, dx3 = x1 - x3, dx2 = x1 - x2;
    s64 a, b, c;

    ENGINE_BLK(8029D90C);
    a = (s64)((u64)dy2 * (u64)dz3 - (u64)dz2 * (u64)dy3);
    b = (s64)((u64)dz2 * (u64)dx3 - (u64)dx2 * (u64)dz3);
    c = (s64)((u64)dx2 * (u64)dy3 - (u64)dy2 * (u64)dx3);
    *d_out = (s64)(0 - ((u64)a * (u64)(s64)x2 + (u64)b * (u64)(s64)y2 + (u64)c * (u64)(s64)z2));
    *b_out = b;
    *c_out = c;
    return a;
}

/* Whether one of the part's triangle's edges (0x28 on: the corners 0-1,
   0-2, 1-2) passes within r of (x, y, z): the edge's line meets the
   sphere at a t in [0, 1] ($t7) */
REGS(t3, t4, t5, t6, s0 -> t7)
s32 func_8029BD0C(s32 x, s32 y, s32 z, s32 r, struct Piece *piece) {
    s32 *w = (s32 *)((u8 *)piece + 0x28), *a, *b;
    s32 e = 4, hit = 0, at = 3, ex, ey, ez, px, py, pz, fsat = 0;
    u64 dd = 0, dot2 = 0, cc = 0, disc = 0, q = 0;
    f32 f0 = 0.0f, f2 = 0.0f, f4, f6;

    ENGINE_BLK(8029BD0C);
    for (;;) {
        ENGINE_BLK(8029BD14);
        at = 3;
        if (--e == 0)
            break;
        ENGINE_BLK(8029BD20);
        at = 2;
        if (e == 3) {
            ENGINE_BLK(8029BD68);
            a = w, b = w + 3;
        } else {
            ENGINE_BLK(8029BD28);
            if (e == 2) {
                ENGINE_BLK(8029BD4C);
                a = w, b = w + 6;
            } else {
                ENGINE_BLK(8029BD30);
                a = w + 3, b = w + 6;
            }
        }
        ENGINE_BLK(8029BD80);
        px = a[0] - x, ex = b[0] - a[0];
        py = a[1] - y, ey = b[1] - a[1];
        pz = a[2] - z, ez = b[2] - a[2];
        dd = (u64)(s64)ex * (u64)(s64)ex + (u64)(s64)ey * (u64)(s64)ey + (u64)(s64)ez * (u64)(s64)ez;
        dot2 = ((u64)(s64)ex * (u64)(s64)px + (u64)(s64)ey * (u64)(s64)py + (u64)(s64)ez * (u64)(s64)pz) << 1;
        cc = (u64)(s64)px * (u64)(s64)px + (u64)(s64)py * (u64)(s64)py + (u64)(s64)pz * (u64)(s64)pz -
             (u64)(s64)r * (u64)(s64)r;
        q = (dd * cc) << 2;
        disc = dot2 * dot2 - q;
        ENGINE_LEAVE(2, ex);
        ENGINE_LEAVE(3, ey);
        ENGINE_LEAVE(4, ez);
        ENGINE_LEAVE(5, px);
        ENGINE_LEAVE(6, py);
        ENGINE_LEAVE(7, pz);
        if ((s64)disc < 0)
            continue;
        ENGINE_BLK(8029BE60);
        f2 = __builtin_sqrtf((f32)(s64)disc);
        dot2 = 0 - dot2;
        f4 = (f32)(s64)dot2;
        dd <<= 1;
        f0 = (f32)(s64)dd;
        at = 0x3F800000;
        fsat = 1;
        f6 = (f4 + f2) / f0;
        if (!(f6 < 0.0f)) {
            ENGINE_BLK(8029BEA4);
            if (!(f6 > 1.0f)) {
                ENGINE_BLK(8029BEB0);
                hit = 1;
                break;
            }
        }
        ENGINE_BLK(8029BEB8);
        f6 = (f4 - f2) / f0;
        if (f6 < 0.0f)
            continue;
        ENGINE_BLK(8029BECC);
        if (f6 > 1.0f)
            continue;
        ENGINE_BLK(8029BED8);
        hit = 1;
        break;
    }
    ENGINE_BLK(8029BEDC);
    /* (what it leaves: the last edge's working) */
    ENGINE_LEAVE(1, at);
    ENGINE_LEAVE64(8, dd);
    ENGINE_LEAVE64(9, dot2);
    ENGINE_LEAVE64(10, cc);
    ENGINE_LEAVE(18, e);
    ENGINE_LEAVE64(19, disc);
    ENGINE_LEAVE64(20, q);
    if (fsat) {
        ENGINE_LEAVE_F(0, f0);
        ENGINE_LEAVE_F(2, f2);
        ENGINE_LEAVE_F(8, 0.0f);
    }
    return hit;
}

extern u8 D_803A742C, D_803A742F;

/* the high half of an address's lui */
#define HI(sym) (((u32) & (sym) + 0x8000) & 0xFFFF0000)

/* The camera's two headings (D_803A7410, D_803A7412) widened to take in
   a and b (12-bit, wrapped): set to them when unset (0 and 0xFFF).  When
   that widens the turn between them (func_8029B930) with D_80358064 set,
   D_803A742F is set, and with D_803A742C (and D_803A742E clear) the
   count D_803A742D steps on (1 to 8, else up one) and D_803A742E is set. */
REGS(a0, a1)
void func_8029B7CC(s32 a, s32 b) {
    s32 before, h0, h1, d, at;

    ENGINE_BLK(8029B7CC);
    if (a < 0) {
        ENGINE_BLK(8029B7DC);
        a += 0xFFF;
    }
    ENGINE_BLK(8029B7E0);
    if (a >= 0x1000) {
        ENGINE_BLK(8029B7EC);
        a -= 0xFFF;
    }
    ENGINE_BLK(8029B7F0);
    if (b < 0) {
        ENGINE_BLK(8029B7F8);
        b += 0xFFF;
    }
    ENGINE_BLK(8029B7FC);
    if (b >= 0x1000) {
        ENGINE_BLK(8029B808);
        b -= 0xFFF;
    }
    ENGINE_BLK(8029B80C);
    before = func_8029B930();
    ENGINE_BLK(8029B814);
    h0 = (u16)D_803A7410;
    h1 = (u16)D_803A7412;
    if (h0 == 0) {
        ENGINE_BLK(8029B834);
        if (h1 == 0xFFF) {
            ENGINE_BLK(8029B840);
            D_803A7410 = a;
            D_803A7412 = b;
            ENGINE_LEAVE(1, HI(D_803A7412));
            ENGINE_LEAVE(5, b);
            goto done;
        }
    }
    ENGINE_BLK(8029B850);
    h1 = (u32)h1 << 20;
    b = (u32)b << 20;
    h0 = (u32)h0 << 20;
    a = (u32)a << 20;
    d = (s32)((u32)b - h1);
    if (d <= 0) {
        ENGINE_BLK(8029B868);
        h1 = b;
    }
    ENGINE_BLK(8029B86C);
    d = (s32)((u32)a - h0);
    if (d > 0) {
        ENGINE_BLK(8029B878);
        h0 = a;
    }
    ENGINE_BLK(8029B87C);
    D_803A7410 = (u32)h0 >> 20;
    D_803A7412 = (u32)h1 >> 20;
    ENGINE_LEAVE(5, b);
    ENGINE_LEAVE(7, d);
    {
        s32 after = func_8029B930();

        ENGINE_BLK(8029B8A0);
        at = before < after;
    }
    if (!at)
        goto leave;
    ENGINE_BLK(8029B8AC);
    if (D_80358064 == 0)
        goto leave;
    ENGINE_BLK(8029B8BC);
    D_803A742F = 1;
    at = HI(D_803A742F);
    if (D_803A742C == 0)
        goto leave;
    ENGINE_BLK(8029B8D8);
    if (D_803A742E != 0)
        goto leave;
    ENGINE_BLK(8029B8E8);
    if (D_803A742D == 1) {
        ENGINE_BLK(8029B8FC);
        d = 8;
    } else {
        ENGINE_BLK(8029B904);
        d = D_803A742D + 1;
    }
    ENGINE_BLK(8029B908);
    D_803A742D = d;
    D_803A742E = 1;
    at = HI(D_803A742E);
leave:
    ENGINE_LEAVE(1, at);
done:
    ENGINE_BLK(8029B91C);
}

REGS(v1 -> fp)
s32 func_802AD7FC(u32 x);

/* a double's or a 64-bit integer's two FPR words */
#define LEAVE_FPAIR(fpr, v)                                                 \
    do {                                                                    \
        u64 b_ = (v);                                                       \
        ENGINE_LEAVE_FW((fpr), (u32)b_);                                    \
        ENGINE_LEAVE_FW((fpr) + 1, (u32)(b_ >> 32));                        \
    } while (0)

static u64 f64_bits(f64 d) {
    union {
        f64 d;
        u64 u;
    } u;

    u.d = d;
    return u.u;
}

/* The part's heading in the ground plane (0x4C, 12-bit) from its
   triangle's normal (corners / 8 at 0x28): the arctangent
   (func_802AD7FC) of the normal's x over its length across, by quadrant;
   turned half round (0x56 set) when the corner moved 100 along the
   normal lies on the other side of the plane (func_8029D90C) than the
   origin does */
REGS(s0)
void func_8029D56C(u8 *part) {
    u32 *w = (u32 *)(part + 0x28);
    s32 x1 = w[0] >> 3, y1 = w[1] >> 3, z1 = w[2] >> 3, x2 = w[3] >> 3, y2 = w[4] >> 3, z2 = w[5] >> 3;
    s32 x3 = w[6] >> 3, y3 = w[7] >> 3, z3 = w[8] >> 3, h, px, py, pz;
    s64 ey3 = y3 - y1, ez2 = z2 - z1, ez3 = z3 - z1, ey2 = y2 - y1, ex2 = x2 - x1, ex3 = x3 - x1;
    s64 nx, ny, nz, ax, az, num, len, q, dn, nb, nc, nd, side;
    f64 f0, f2, f4, f6;
    s64 l2, l4, l6;

    ENGINE_BLK(8029D56C);
    nx = (s64)((u64)ey3 * (u64)ez2 - (u64)ez3 * (u64)ey2);
    ny = (s64)((u64)ez3 * (u64)ex2 - (u64)ex3 * (u64)ez2);
    nz = (s64)((u64)ex3 * (u64)ey2 - (u64)ey3 * (u64)ex2);
    ax = nx;
    if (nx < 0) {
        ENGINE_BLK(8029D6B0);
        ax = -nx;
    }
    ENGINE_BLK(8029D6B4);
    az = nz;
    if (nz < 0) {
        ENGINE_BLK(8029D6BC);
        az = -nz;
    }
    ENGINE_BLK(8029D6C0);
    num = (s64)((u64)ax << 16);
    len = engine_cvt_l_d(__builtin_sqrt((f64)(s64)((u64)az * (u64)az + (u64)ax * (u64)ax)));
    if (len == 0) {
        ENGINE_BLK(8029D70C);
        engine_break(0x8029D70C, 7);
    }
    ENGINE_BLK(8029D710);
    if (len == -1) {
        ENGINE_BLK(8029D71C);
        if ((u64)num == (u64)1 << 63) {
            ENGINE_BLK(8029D728);
            engine_break(0x8029D728, 6);
        }
    }
    ENGINE_BLK(8029D72C);
    q = num / len;
    h = func_802AD7FC((u32)q);
    ENGINE_BLK(8029D738);
    h = (u32)h >> 4;
    if (nx < 0) {
        ENGINE_BLK(8029D740);
        if (nz < 0) {
            ENGINE_BLK(8029D754);
            h += 0x800;
        } else {
            ENGINE_BLK(8029D748);
            h = 0xFFF - h;
        }
    } else {
        ENGINE_BLK(8029D75C);
        if (nz < 0) {
            ENGINE_BLK(8029D764);
            h = 0x800 - h;
        }
    }
    ENGINE_BLK(8029D76C);
    f0 = __builtin_sqrt((f64)(s64)((u64)nx * (u64)nx + (u64)ny * (u64)ny + (u64)nz * (u64)nz));
    f2 = (f64)nx / f0;
    f4 = (f64)ny / f0;
    f2 = f2 * 100.0;
    l2 = engine_cvt_l_d(f2);
    px = x1 + (s32)l2;
    f6 = (f64)nz / f0;
    f4 = f4 * 100.0;
    l4 = engine_cvt_l_d(f4);
    py = y1 + (s32)l4;
    f6 = f6 * 100.0;
    l6 = engine_cvt_l_d(f6);
    pz = z1 + (s32)l6;
    dn = func_8029D90C(part, &nb, &nc, &nd);
    ENGINE_BLK(8029D81C);
    part[0x56] = 0;
    side = (s64)((u64)(s64)px * (u64)dn + (u64)(s64)py * (u64)nb + (u64)(s64)pz * (u64)nc + (u64)nd);
    if (side > 0) {
        ENGINE_BLK(8029D868);
        if (nd > 0)
            goto done;
    } else {
        ENGINE_BLK(8029D858);
        if (nd < 0)
            goto done;
        ENGINE_BLK(8029D860);
    }
    ENGINE_BLK(8029D870);
    h -= 0x800;
    part[0x56] = 1;
    if (h < 0) {
        ENGINE_BLK(8029D880);
        h += 0xFFF;
    }
done:
    ENGINE_BLK(8029D884);
    *(s16 *)(part + 0x4C) = h;
    /* (what it leaves: the FPU's last values) */
    LEAVE_FPAIR(0, f64_bits(f0));
    LEAVE_FPAIR(2, (u64)l2);
    LEAVE_FPAIR(4, (u64)l4);
    LEAVE_FPAIR(6, (u64)l6);
    LEAVE_FPAIR(8, f64_bits(100.0));
}

/* a doubleword into game memory, the high word first */
static void put_dword(u8 *p, s64 v) {
    ((u32 *)p)[0] = (u32)((u64)v >> 32);
    ((u32 *)p)[1] = (u32)v;
}

/* The parts of kind id from a model's triangles (data: their count, the
   matrices' count and offsets, then 0x14-byte triangles of three s16
   points): each part (D_803B9890's of (id, its number), new ones past
   D_803BD300's end) gets the triangle's corners through the matrices at
   base (func_802AA890, / 4), its plane (the normal at 0, 8, 0x10, d at
   0x18, 64 bits; |n|^2 and |n| at 0x24 and 0x20), the axis the normal is
   most along (0x4E: 0 z, 1 y, 2 x), its heading (func_8029D56C) and
   func_8029D534 */
REGS(t6, t2, s4)
void func_8029D24C(u16 *data, s32 id, u8 *base) {
    s32 count = data[0], idx = 1, n = data[1], x, y, z, *pw;
    s32 *offs = (s32 *)(data + 2);
    s16 *t;
    u8 *p, *end;
    s32 s1, s2, y1, z1, x1, y2, x2, z2, x3, y3, z3;
    s32 dy2, dz3, dz2, dy3, dx3, dx2;
    s64 nx, ny, nz, d, ax, ay, az;
    f32 f0, f2;

    ENGINE_BLK(8029D24C);
    for (;;) {
        ENGINE_BLK(8029D278);
        if (count == 0)
            break;
        ENGINE_BLK(8029D280);
        p = D_803B9890;
        end = D_803BD300;
        for (;;) {
            ENGINE_BLK(8029D294);
            if (p == end) {
                end += 0x60;
                break;
            }
            ENGINE_BLK(8029D29C);
            if (p[0x4F] != id) {
                p += 0x60;
                continue;
            }
            ENGINE_BLK(8029D2A8);
            if (p[0x50] == idx)
                break;
            p += 0x60;
        }
        ENGINE_BLK(8029D2B4);
        D_803BD300 = end;
        p[0x4F] = id;
        p[0x50] = idx;
        t = (s16 *)((u8 *)data + 4 + n * 4 + (idx - 1) * 0x14);
        pw = (s32 *)(p + 0x28);
        s1 = (u32)end;
        s2 = (u32)&D_803BD300;
        x = func_802AA890(t[0], t[1], t[2], n, offs, base, (u32)p, s1, s2, &y, &z, &s1, &s2);
        ENGINE_BLK(8029D2F8);
        pw[0] = x >> 2, pw[1] = y >> 2, pw[2] = z >> 2;
        x = func_802AA890(t[3], t[4], t[5], n, offs, base, (u32)p, s1, s2, &y, &z, &s1, &s2);
        ENGINE_BLK(8029D320);
        pw[3] = x >> 2, pw[4] = y >> 2, pw[5] = z >> 2;
        x = func_802AA890(t[6], t[7], t[8], n, offs, base, (u32)p, s1, s2, &y, &z, &s1, &s2);
        ENGINE_BLK(8029D348);
        x3 = pw[6] = x >> 2, y3 = pw[7] = y >> 2, z3 = pw[8] = z >> 2;
        x1 = pw[0], y1 = pw[1], z1 = pw[2], x2 = pw[3], y2 = pw[4], z2 = pw[5];
        dz3 = z1 - z3, dy2 = y1 - y2, dy3 = y1 - y3, dz2 = z1 - z2, dx3 = x1 - x3, dx2 = x1 - x2;
        nx = (s64)((u64)(s64)dy2 * (u64)(s64)dz3 - (u64)(s64)dz2 * (u64)(s64)dy3);
        put_dword(p, nx);
        f0 = (f32)nx;
        f0 = f0 * f0;
        ny = (s64)((u64)(s64)dz2 * (u64)(s64)dx3 - (u64)(s64)dx2 * (u64)(s64)dz3);
        put_dword(p + 8, ny);
        f2 = (f32)ny;
        f2 = f2 * f2;
        f0 = f0 + f2;
        nz = (s64)((u64)(s64)dx2 * (u64)(s64)dy3 - (u64)(s64)dy2 * (u64)(s64)dx3);
        put_dword(p + 0x10, nz);
        f2 = (f32)nz;
        f2 = f2 * f2;
        f0 = f0 + f2;
        *(f32 *)(p + 0x24) = f0;
        f0 = __builtin_sqrtf(f0);
        d = (s64)(0 - ((u64)nx * (u64)(s64)x2 + (u64)ny * (u64)(s64)y2 + (u64)nz * (u64)(s64)z2));
        put_dword(p + 0x18, d);
        *(f32 *)(p + 0x20) = f0;
        ax = nx;
        if (nx < 0) {
            ENGINE_BLK(8029D47C);
            ax = -nx;
        }
        ENGINE_BLK(8029D480);
        ay = ny;
        if (ny < 0) {
            ENGINE_BLK(8029D488);
            ay = -ny;
        }
        ENGINE_BLK(8029D48C);
        az = nz;
        if (nz < 0) {
            ENGINE_BLK(8029D494);
            az = -nz;
        }
        ENGINE_BLK(8029D498);
        /* (what it leaves for this triangle, before its two calls) */
        ENGINE_LEAVE(7, z1);
        ENGINE_LEAVE(15, x3);
        ENGINE_LEAVE(19, dx2);
        ENGINE_LEAVE(22, dx3);
        ENGINE_LEAVE(23, dy3);
        ENGINE_LEAVE(24, dz3);
        ENGINE_LEAVE64(25, ax);
        ENGINE_LEAVE64(30, d);
        ENGINE_LEAVE_F(0, f0);
        ENGINE_LEAVE_F(2, f2);
        if (!(az < ax)) {
            ENGINE_BLK(8029D4A4);
            if (!(az < ay)) {
                ENGINE_BLK(8029D4AC);
                p[0x4E] = 0;
                ENGINE_LEAVE(1, 0);
                goto axis;
            }
        }
        ENGINE_BLK(8029D4B4);
        if (!(ay < ax)) {
            ENGINE_BLK(8029D4C0);
            if (!(ay < az)) {
                ENGINE_BLK(8029D4C8);
                p[0x4E] = 1;
                ENGINE_LEAVE(1, 0);
                goto axis;
            }
        }
        ENGINE_BLK(8029D4D4);
        p[0x4E] = 2;
        ENGINE_LEAVE(1, ay < az);
    axis:
        ENGINE_BLK(8029D4DC);
        func_8029D56C(p);
        ENGINE_BLK(8029D4E4);
        func_8029D534(id, p);
        ENGINE_BLK(8029D4EC);
        idx++;
        count--;
    }
    ENGINE_BLK(8029D50C);
    ENGINE_LEAVE(12, idx);
    ENGINE_LEAVE(13, 0);
}

/* An object's parts set up (func_8029D24C, flagged by func_8029D120),
   then each train stop of this id (D_803BC1D0's records) that the point
   (x, z) at y doesn't miss (by its triangles, func_8029DA90, or its part
   pairs, func_8029DB7C): the parts it lists (kind 0, then this id's)
   unflagged (func_8029D1D4) */
REGS(t6, t2, s4, t0, t1, a1, a2)
void func_8029D040(u16 *data, s32 id, u8 *base, s32 x, s32 z, s32 y, u8 *parts) {
    u8 *q, *end, *b, *part;
    s32 kind, n, miss, t7;

    ENGINE_BLK(8029D040);
    func_8029D24C(data, id, base);
    ENGINE_BLK(8029D050);
    func_8029D120(id);
    ENGINE_BLK(8029D058);
    end = D_803BD304;
    ENGINE_LEAVE(21, (u32)end);
    for (q = D_803BC1D0;;) {
        ENGINE_BLK(8029D06C);
        if (q == end)
            break;
        ENGINE_BLK(8029D074);
        ENGINE_LEAVE(12, q[0xC4]);
        if (q[0xC4] != id) {
            q += 0xDC;
            continue;
        }
        ENGINE_BLK(8029D080);
        kind = q[0xC5];
        n = q[0xC6];
        ENGINE_LEAVE(20, (u32)q);
        if (kind == 0) {
            ENGINE_BLK(8029D090);
            miss = func_8029DA90((s32 *)q, n, x, z, y);
            ENGINE_BLK(8029D098);
        } else {
            ENGINE_BLK(8029D0A0);
            miss = func_8029DB7C(q, n, parts);
        }
        ENGINE_LEAVE(12, miss);
        ENGINE_BLK(8029D0A8);
        if (miss != 0) {
            q += 0xDC;
            continue;
        }
        ENGINE_BLK(8029D0B0);
        b = q + 0xC8;
        n = q[0xC7];
        t7 = 0;
        for (;;) {
            ENGINE_BLK(8029D0BC);
            if (n == 0)
                break;
            ENGINE_BLK(8029D0C4);
            n--;
            ENGINE_LEAVE(14, *b);
            part = func_8029D1D4(t7, *b);
            ENGINE_LEAVE(16, (u32)part);
            b++;
            ENGINE_BLK(8029D0D4);
        }
        ENGINE_BLK(8029D0DC);
        b = q + 0xD1;
        n = q[0xD0];
        t7 = q[0xC4];
        for (;;) {
            ENGINE_BLK(8029D0E8);
            if (n == 0)
                break;
            ENGINE_BLK(8029D0F0);
            n--;
            ENGINE_LEAVE(14, *b);
            part = func_8029D1D4(t7, *b);
            ENGINE_LEAVE(16, (u32)part);
            b++;
            ENGINE_BLK(8029D100);
        }
        ENGINE_LEAVE(12, (u32)b);
        ENGINE_LEAVE(13, 0);
        ENGINE_LEAVE(15, t7);
        ENGINE_BLK(8029D108);
        q += 0xDC;
    }
    ENGINE_BLK(8029D110);
    ENGINE_LEAVE(20, (u32)q);
}

void func_8028FAC0(s32 x, s32 y, s32 z, s32 id);
void func_802920DC(s32 x, s32 y, s32 z, s32 id);

/* D_803A6B30's record func_8029C6E4 finds (kind 6, id 0x3BD), looked up
   again for its point (no cost: the original keeps it in registers) */
static s32 *record_3bd(void) {
    u8 *p;

    for (p = D_803A6B30; (s8)p[0x13] != -1; p += 0x14)
        if (p[0x12] == 6 && *(s32 *)(p + 0xC) == 0x3BD)
            return (s32 *)p;
    return NULL;
}

/* With D_803A6B30's record 0x3BD (func_8029C6E4), its point to
   func_8028FAC0 and func_802920DC */
void func_8029C5EC(void) {
    s32 found, *r;

    ENGINE_BLK(8029C5EC);
    found = func_8029C6E4();
    ENGINE_BLK(8029C650);
    if (found != 0) {
        r = record_3bd();
        ENGINE_BLK(8029C658);
        func_8028FAC0(r[0], r[1], r[2], 0x3BD);
        ENGINE_BLK(8029C66C);
        func_802920DC(r[0], r[1], r[2], 0x3BD);
    }
    ENGINE_BLK(8029C680);
    ENGINE_LEAVE(20, found);
}

/* The animation a's nine values (the halves of its four current keys,
   D_803B7FC0-D_803B7FC3 of the 0x14-byte records 8 on from keys) on the
   spline at its fraction, into out; the basis set up for its mode
   (func_8029F110, func_8029F060).  $t1 is out's end + 2. */
REGS(t4, t5, t1)
void func_8029FFA0(u8 *a, u8 *keys, s16 *out) {
    u8 *d = *(u8 *PTR32 *)a, *k = keys + 8;
    s16 *p0, *p1, *p2, *p3;
    s32 i, v = 0;
    f32 t = *(f32 *)(a + 4), k0 = 0.0f, k1 = 0.0f;

    ENGINE_BLK(8029FFA0);
    func_8029F110(a, a[0x15]);
    ENGINE_BLK(8029FFFC);
    func_8029F060(t, a[0x13], d[0]);
    ENGINE_BLK(802A0004);
    p0 = (s16 *)(k + D_803B7FC0 * 0x14);
    p1 = (s16 *)(k + D_803B7FC1 * 0x14);
    p2 = (s16 *)(k + D_803B7FC2 * 0x14);
    p3 = (s16 *)(k + D_803B7FC3 * 0x14);
    for (i = 9;;) {
        ENGINE_BLK(802A0070);
        if (i == 0)
            break;
        ENGINE_BLK(802A0078);
        k0 = (f32)*p0;
        k1 = (f32)*p1;
        v = engine_cvt_w_s(func_8029E878(k0, k1, (f32)*p2, (f32)*p3, t));
        ENGINE_BLK(802A00AC);
        p0++, p1++, p2++, p3++;
        *out++ = v;
        i--;
    }
    ENGINE_BLK(802A00D4);
    ENGINE_LEAVE(9, (u32)(out + 1));
    ENGINE_LEAVE_F(0, k0);
    ENGINE_LEAVE_F(2, k1);
    ENGINE_LEAVE_FW(8, v);
    ENGINE_LEAVE_F(30, t);
}

/* The key k (nine s16s: scale or position, three angles, position) the
   fraction t of the way to the next (0x14 on), into out: the six
   positions straight, the angles the short way round (func_8029F6B0) */
REGS(t5, t1, f30)
void func_802A0118(s16 *k, s16 *out, f32 t) {
    s32 w0, w;

    ENGINE_BLK(802A0118);
    w0 = engine_cvt_w_s((f32)(k[10] - k[0]) * t);
    out[0] = k[0] + w0;
    out[1] = k[1] + engine_cvt_w_s((f32)(k[11] - k[1]) * t);
    out[2] = k[2] + engine_cvt_w_s((f32)(k[12] - k[2]) * t);
    ENGINE_LEAVE_FW(0, w0);
    out[3] = func_8029F6B0(k[3], k[13], t);
    ENGINE_BLK(802A01C4);
    out[4] = func_8029F6B0(k[4], k[14], t);
    ENGINE_BLK(802A01D4);
    out[5] = func_8029F6B0(k[5], k[15], t);
    ENGINE_BLK(802A01E4);
    w = engine_cvt_w_s((f32)(k[16] - k[6]) * t);
    out[6] = k[6] + w;
    out[7] = k[7] + engine_cvt_w_s((f32)(k[17] - k[7]) * t);
    out[8] = k[8] + engine_cvt_w_s((f32)(k[18] - k[8]) * t);
    ENGINE_LEAVE_FW(2, w);
    ENGINE_LEAVE(9, (u32)(out + 10));
}

extern u8 D_803A7428, D_80370C3C;
extern u8 D_802C2984[];                 /* the effect's description */

/* An effect (func_802A6274: D_802C2984, D_803A7428 * 7000 long) at the
   point D_803A73FC-D_803A7404, and D_80370C3C set */
void func_8029B994(void) {
    ENGINE_BLK(8029B994);
    func_802A6274((s32)(u32)D_802C2984, D_803A7428 * 0x1B58, 0, (u32)D_803A73FC << 13, (u32)D_803A7400 << 13,
                  (u32)D_803A7404 << 13, 0, 0, 0, 0, 0, 0, 0, 1, 0);
    ENGINE_BLK(8029BA98);
    D_80370C3C = 1;
}

extern s32 D_803A73F0, D_803A73F4, D_803A73F8, D_803A7408, D_80358060;
extern u8 D_803F7811, D_803F7801, D_803A7427, D_803A7429, D_803A7424, D_803A7425;
extern s16 D_803A7422, D_803F77FC;

/* A vehicle's collision state for the frame: the camera's headings
   unset, its point and settings from the caller's registers, the gears
   into D_803A742D; the building lists reset (func_802BCC10 when
   D_80358060 is clear or with a2 for a vehicle other than 0xFF, then
   func_802BCBD8).  Returns D_80358060 ($v0). */
REGS(v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t8, gp -> v0)
s32 func_8029A800(s32 x, s32 y, s32 z, s32 a1, s32 a2, s32 a3, s32 t0, s32 t1, s32 t2, s32 t3, s32 type, VS *vs) {
    s32 v;

    ENGINE_BLK(8029A800);
    D_803A7410 = 0;
    D_803A7412 = 0xFFF;
    D_803A73F0 = x;
    D_803A73F4 = y;
    D_803A73F8 = z;
    D_803F7811 = a2;
    D_803A742C = 0;
    D_803A742E = 0;
    D_803A742D = vs->unkA0;
    D_803A742F = 0;
    D_803F7801 = t3;
    D_803A7427 = a3;
    D_803A7428 = t0;
    D_803A7429 = 0;
    D_803A7422 = t2;
    D_803F77FC = t1;
    D_803A7408 = a1;
    D_803A7424 = 0;
    D_803A7425 = 0;
    v = D_80358060;
    if (v == 0) {
        ENGINE_BLK(8029A8D8);
        func_802BCC10();
    }
    ENGINE_BLK(8029A8E0);
    if (a2 != 0) {
        ENGINE_BLK(8029A8E8);
        if (type != 0xFF) {
            ENGINE_BLK(8029A8F0);
            func_802BCC10();
        }
    }
    ENGINE_BLK(8029A8F8);
    func_802BCBD8();
    ENGINE_BLK(8029A900);
    return v;
}

extern u8 D_803B67C0[], D_803B37C0[];   /* a blend's animation, built and its copy */

/* the frame of the animation in part p's state (spline: func_8029FFA0;
   key frames: func_802A0118) for its record rec, into out */
static u8 *anim_frame(u8 *p, u8 *rec, u8 *out, s32 *t5) {
    if (p[0x15] == 0) {
        u8 *k = rec + (s8)p[0x13] * 0x14 + 8;

        *t5 = (u32)k;
        ENGINE_LEAVE_F(30, *(f32 *)(p + 4));
        func_802A0118((s16 *)k, (s16 *)out, *(f32 *)(p + 4));
    } else {
        *t5 = (u32)rec;
        func_8029FFA0(p, rec, (s16 *)out);
    }
    return out + 0x14;
}

/* A blend of the animations of parts[a] and parts[b] (0x18 each): a
   two-key animation (speeds 1) whose parts are those both have, each with
   its frame in a and in b, built at D_803B67C0 and kept at D_803B37C0 for
   parts[31] (from its start) */
REGS(v0, v1, a0)
void func_8029F9D4(s32 a, s32 b, u8 *parts) {
    u8 *pa = parts + a * 0x18, *pb = parts + b * 0x18, *da = *(u8 *PTR32 *)pa, *db = *(u8 *PTR32 *)pb;
    u8 *ra, *rb, *rb0, *out = D_803B67C0 + 4, *p31;
    s32 na = da[da[0] + 1], nb = db[db[0] + 1], sa = da[0] * 0x14 + 8, sb = db[0] * 0x14 + 8, n = 0, j;
    s32 t5;
    u32 *s, *d;

    ENGINE_BLK(8029F9D4);
    D_803B67C0[0] = 2;
    D_803B67C0[1] = 1;
    D_803B67C0[2] = 1;
    ra = da + da[0] + 2;
    if ((u32)ra & 3) {
        ENGINE_BLK(8029FA94);
        ra += 4 - ((u32)ra & 3);
    }
    ENGINE_BLK(8029FAA0);
    rb0 = db + db[0] + 2;
    t5 = (u32)(db + db[0]);
    if ((u32)rb0 & 3) {
        ENGINE_BLK(8029FAC8);
        rb0 += 4 - ((u32)rb0 & 3);
    }
    for (;;) {
        ENGINE_BLK(8029FAD4);
        if (na == 0)
            break;
        ENGINE_BLK(8029FADC);
        rb = rb0;
        for (j = nb;; j--, rb += sb) {
            ENGINE_BLK(8029FAE8);
            if (j == 0)
                goto next;
            ENGINE_BLK(8029FAF0);
            t5 = *(s32 *)rb;
            if (*(u32 *)ra == *(u32 *)rb)
                break;
            ENGINE_BLK(8029FAFC);
        }
        ENGINE_BLK(8029FB08);
        ((u32 *)out)[0] = *(u32 *)ra;
        ((u32 *)out)[1] = t5 = ((s32 *)ra)[1];
        out += 8;
        n++;
        ENGINE_LEAVE(1, 1);
        if (pa[0x15] != 0) {
            ENGINE_BLK(8029FB28);
            if (pa[0x15] != 1) {
                ENGINE_BLK(8029FB30);
                engine_syscall(0x8029FB30);
            }
            ENGINE_BLK(8029FB34);
            out = anim_frame(pa, ra, out, &t5);
            ENGINE_BLK(8029FB40);
        } else {
            ENGINE_BLK(8029FB48);
            out = anim_frame(pa, ra, out, &t5);
        }
        ENGINE_BLK(8029FB68);
        ENGINE_LEAVE(1, 1);
        if (pb[0x15] != 0) {
            ENGINE_BLK(8029FB74);
            if (pb[0x15] != 1) {
                ENGINE_BLK(8029FB7C);
                engine_syscall(0x8029FB7C);
            }
            ENGINE_BLK(8029FB80);
            out = anim_frame(pb, rb, out, &t5);
            ENGINE_BLK(8029FB8C);
        } else {
            ENGINE_BLK(8029FB94);
            out = anim_frame(pb, rb, out, &t5);
        }
    next:
        ENGINE_BLK(8029FBB4);
        ra += sa;
        na--;
    }
    ENGINE_BLK(8029FBC0);
    D_803B67C0[3] = n;
    p31 = parts + 0x2E8;
    p31[0x10] = 0;
    p31[0x11] = 0;
    p31[0x12] = 0;
    p31[0x15] = 0;
    p31[0x13] = 0;
    *(u8 *PTR32 *)p31 = D_803B37C0;
    *(f32 *)(p31 + 4) = 0.0f;
    s = (u32 *)D_803B67C0;
    d = (u32 *)D_803B37C0;
    for (j = 0x1800;;) {
        ENGINE_BLK(8029FC04);
        if (j == 0)
            break;
        ENGINE_BLK(8029FC0C);
        d[0] = s[0];
        d[1] = s[1];
        d += 2, s += 2;
        j -= 8;
    }
    ENGINE_BLK(8029FC24);
    ENGINE_LEAVE(13, t5);
    ENGINE_LEAVE_F(0, 0.0f);
}
