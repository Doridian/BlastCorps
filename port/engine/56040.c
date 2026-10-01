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
REGS(a0, a1, a2, a3, t0, t1, f30)
void func_8029F3D0(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1, f32 t) {
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
                ENGINE_LEAVE(4, 0);
                goto done;
            }
        }
    }
    ENGINE_BLK(8029F44C);
    m[0] = 0x10000, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = 0x10000, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = 0x10000, m[11] = 0;
    m[12] = (u32)x << 16, m[13] = (u32)y << 16, m[14] = (u32)z << 16, m[15] = 0x10000;
    ENGINE_LEAVE(4, 0x10000);
done:
    ENGINE_BLK(8029F4A4);
}

/* D_803B3730 = the rotation about x (F4B8), y (F560) or z (F608) by the
   angle from a to b the fraction t of the way (func_8029F6B0), unless
   that is 0 ($a0: 0x10000, or 0) */
REGS(a0, a1, f30)
void func_8029F4B8(s32 a, s32 b, f32 t) {
    s32 r, c, s, *m = D_803B3730;

    ENGINE_BLK(8029F4B8);
    r = func_8029F6B0(a, b, t);
    ENGINE_BLK(8029F4D4);
    if (r == 0) {
        ENGINE_BLK(8029F4DC);
        ENGINE_LEAVE(4, 0);
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
    ENGINE_LEAVE(4, 0x10000);
    ENGINE_LEAVE(30, s);
done:
    ENGINE_BLK(8029F544);
}

REGS(a0, a1, f30)
void func_8029F560(s32 a, s32 b, f32 t) {
    s32 r, c, s, *m = D_803B3730;

    ENGINE_BLK(8029F560);
    r = func_8029F6B0(a, b, t);
    ENGINE_BLK(8029F57C);
    if (r == 0) {
        ENGINE_BLK(8029F584);
        ENGINE_LEAVE(4, 0);
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
    ENGINE_LEAVE(4, 0x10000);
    ENGINE_LEAVE(30, s);
done:
    ENGINE_BLK(8029F5EC);
}

REGS(a0, a1, f30)
void func_8029F608(s32 a, s32 b, f32 t) {
    s32 r, c, s, *m = D_803B3730;

    ENGINE_BLK(8029F608);
    r = func_8029F6B0(a, b, t);
    ENGINE_BLK(8029F624);
    if (r == 0) {
        ENGINE_BLK(8029F62C);
        ENGINE_LEAVE(4, 0);
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
    ENGINE_LEAVE(4, 0x10000);
    ENGINE_LEAVE(30, s);
done:
    ENGINE_BLK(8029F694);
}
