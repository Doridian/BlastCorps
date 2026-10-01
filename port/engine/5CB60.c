/*
 * hd_code 5CB60 (us.v11 0x802A1320-0x802A4510): the level loader's parts,
 * as native C (engine.h).  The loader itself (func_802A1674) and its
 * vehicle and model parts are still translated; these are the parts that
 * turn the level file's sections into the run-time tables.
 *
 * The originals take the level's header in $t0 and leave registers behind
 * that the loader's translated code then reads (ENGINE_LEAVE): what each
 * leaves is said where it does.
 */
#include "engine.h"
#include "game/game.h"
#include "game/level.h"
#include "game/vehicle.h"
#include "game/objects.h"

/* the header's offset fields: what they give */
#define AT(h, off) ((u8 *)(h) + *(u32 *)((u8 *)(h) + (off)))

/* a big-endian u32 at any alignment in the level file: in native-endian
   memory the loaders have put it in host order where it is (docs/PORT.md,
   "Native-endian memory": the lwl/lwr pairs) */
#ifdef PORT_NATIVE_ENDIAN
#define UNALIGNED_W(p) ((u32)((p)[3] << 24 | (p)[2] << 16 | (p)[1] << 8 | (p)[0]))
#else
#define UNALIGNED_W(p) ((u32)((p)[0] << 24 | (p)[1] << 16 | (p)[2] << 8 | (p)[3]))
#endif

/* the high half the original's lui leaves in a register for an address */
#define HI16(p) (((u32)(p) + 0x8000) & 0xFFFF0000)

extern f32 D_803EBBF0;          /* gravity */
extern u8 *PTR32 D_80364458;    /* the level's display data, after its header */
extern u8 *PTR32 D_803EBBEC;
extern u8 D_803EBDB0[];
extern u8 D_803A6B30[100][0x14];
extern u8 D_803A7300[12][0x14];
extern u8 D_803ED3B8[12][4];
extern s32 D_803EBC10[26][4];
extern u8 *PTR32 D_803BE6F8;    /* the vehicles' records */
extern u8 D_803A7426, D_803F7805, D_803F780A, D_803F780B, D_803F780C, D_803F7810;
extern u8 D_803F7804, D_803F7812;
extern s32 D_803A740C, D_803F77F8;
extern u8 D_80364A6E;
extern u8 D_803BDFD8[][0x24];
extern u8 *PTR32 D_803BDFD4;
extern u8 D_80306480[][0x30];
extern u8 *PTR32 D_803F7820, *PTR32 D_803F7824, *PTR32 D_803F7828, *PTR32 D_803F782C;

/* func_802A2D68: the header's grids and constants; the run-time tables
   emptied.  Leaves $f0 the gravity, $t1 the header's unk1C, $t2 the
   bounds' last s16, $t3 -1. */
REGS(t0)
void func_802A2D68(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    s32 i;
    s16 *b;

    ENGINE_BLK(802A2D68);
    D_803EBBF0 = (f32)h->gravity;
    D_80364458 = (u8 *)h + 0xC8;
    D_803BDAF4 = (s16 *)AT(h, 0x24);
    D_803BDAF8 = (s16 *)AT(h, 0x28);
    D_803BE714 = h->unk0[0];
    D_803BE716 = h->unk0[1];
    D_803BE70C = h->unk4[0] << 5;
    D_803BE710 = h->unk4[1] << 5;
    D_803BE720 = h->unk8[0];
    D_803BE722 = h->unk8[1];
    D_803BE718 = h->unkC[0] << 5;
    D_803BE71C = h->unkC[1] << 5;
    D_803BE72C = h->unk10[0];
    D_803BE72E = h->unk10[1];
    D_803BE724 = h->unk14[0] << 5;
    D_803BE728 = h->unk14[1] << 5;
    D_803EBBEC = D_803EBDB0;
    i = 100;
    do {
        ENGINE_BLK(802A2EB4);
        D_803A6B30[100 - i][0x13] = 0xFF;
    } while (--i != 0);
    ENGINE_BLK(802A2EC4);
    i = 12;
    do {
        ENGINE_BLK(802A2ED0);
        D_803A7300[12 - i][0x11] = 0xFF;
    } while (--i != 0);
    ENGINE_BLK(802A2EE0);
    i = 12;
    do {
        u8 *p = D_803ED3B8[12 - i];

        ENGINE_BLK(802A2EEC);
        p[0] = p[1] = p[2] = p[3] = 0xFF;
    } while (--i != 0);
    ENGINE_BLK(802A2F08);
    i = 26;
    do {
        ENGINE_BLK(802A2F14);
        D_803EBC10[26 - i][0] = -1;
    } while (--i != 0);
    ENGINE_BLK(802A2F24);
    D_803BE6F8 = AT(h, 0x50);
    b = (s16 *)AT(h, 0x4C);
    D_803BE730 = b[0];
    D_803BE734 = b[1];
    D_803BE732 = b[2];
    D_803BE736 = b[3];
    D_803A7426 = 0;
    D_803ED400 = 0;
    D_803F7805 = 0;
    D_803F7806 = 0;
    D_803BE738 = 0;
    D_803BE739 = h->unk1C;
    D_803EFECB = 0;
    D_803EF6FF = 0;
    D_803ED40C = 0;
    D_803A740C = 0;
    D_803F77F8 = 0;
    D_803F780A = 0;
    D_803F780B = 0;
    D_803F780C = 0;
    D_803F7810 = 0;
    D_803F7804 = 0;
    D_803F7812 = 0;
    ENGINE_LEAVE_F(0, (f32)h->gravity);
    ENGINE_LEAVE(9, h->unk1C);
    ENGINE_LEAVE(10, (s32)b[3]);
    ENGINE_LEAVE(11, -1);
}

/* func_802A1C88: the level's display lists: their G_DL addresses made
   physical, and the four the C draws */
REGS(t0)
void func_802A1C88(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u32 base = (u32)AT(h, 0x78) - 0x80000000;
    u32 *p = (u32 *)AT(h, 0x90);
    u32 *end = (u32 *)AT(h, 0x84);

    ENGINE_BLK(802A1C88);
    for (;;) {
        ENGINE_BLK(802A1CC8);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A1CD0);
        if (p[0] >> 24 != 6) {          /* G_DL */
            p += 2;
            continue;
        }
        ENGINE_BLK(802A1CE0);
        p[1] += base;
        p += 2;
    }
    ENGINE_BLK(802A1CF4);
    D_803BE6E0 = (Gfx *)AT(h, 0x90);
    D_803BE6E4 = (Gfx *)AT(h, 0x94);
    D_803BE6E8 = (Gfx *)AT(h, 0x98);
    D_803BE6EC = (Gfx *)AT(h, 0x9C);
}

/* func_802A4464: a pointer to each terrain group (LevelHeader.terrain:
   each a big-endian length, then its data), and one past the last */
REGS(t0)
void func_802A4464(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    s32 n = (s16)h->unk8[0] * (s16)h->unk8[1];
    u8 *base = (u8 *)h + h->terrain;
    u8 *p = base;
    u8 **out = (u8 **)D_803BDB10;

    ENGINE_BLK(802A4464);
    for (;;) {
        ENGINE_BLK(802A449C);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802A44A4);
        p += 4;
        *out++ = p;
        p = base + UNALIGNED_W(p - 4);
        n--;
    }
    ENGINE_BLK(802A44C8);
    *out = p + 4;
}

/* func_802A44E4: a size rounded up to 8, if it isn't a multiple of 4 */
REGS(a1 -> a1)
u32 func_802A44E4(u32 a) {
    ENGINE_BLK(802A44E4);
    if (a & 3) {
        ENGINE_BLK(802A44F0);
        a = (a & ~7) + 8;
    }
    ENGINE_BLK(802A44FC);
    ENGINE_LEAVE(2, a);
    return a;
}

/* func_802A1934: D_80306480's boxes (0x30 bytes each, to the one whose
   0x15 is -1): their corners, from the centre and a size */
REGS()
void func_802A1934(void) {
    u8 *b = D_80306480[0];

    ENGINE_BLK(802A1934);
    for (;;) {
        s32 x, y, z, r;

        ENGINE_BLK(802A195C);
        if ((s8)b[0x15] == -1) {
            break;
        }
        ENGINE_BLK(802A196C);
        x = *(s32 *)(b + 0) >> 5;
        y = *(s32 *)(b + 4) >> 5;
        z = *(s32 *)(b + 8) >> 5;
        r = b[0x14];
        *(s16 *)(b + 0x16) = *(s16 *)(b + 0x22) = x - r;
        *(s16 *)(b + 0x1A) = *(s16 *)(b + 0x20) = z - r;
        *(s16 *)(b + 0x1C) = *(s16 *)(b + 0x28) = x + r;
        *(s16 *)(b + 0x26) = *(s16 *)(b + 0x2C) = z + r;
        *(s16 *)(b + 0x18) = *(s16 *)(b + 0x1E) = *(s16 *)(b + 0x24) = *(s16 *)(b + 0x2A) = y;
        b += 0x30;
    }
    ENGINE_BLK(802A19D0);
}

/* func_802A2C54: LevelHeader.unk60's records into D_803BDFD8 (0x24 bytes
   each).  Leaves $t3 their end, $t4 &D_803BDFD4, $t5 1 and $t6 the last
   byte copied (as they were, where there are none). */
REGS(t0)
void func_802A2C54(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *p = AT(h, 0x60);
    u8 *end = AT(h, 0x64);
    u8 *r = D_803BDFD8[0];
    u32 t5 = ENGINE_REG(13), t6 = ENGINE_REG(14);

    ENGINE_BLK(802A2C54);
    D_80364A6E = *p++;
    for (;;) {
        u32 n;
        u8 *q;

        ENGINE_BLK(802A2C88);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A2C90);
        *(s32 *)(r + 0x0) = (p[0] << 8 | p[1]) << 5;
        *(s32 *)(r + 0x4) = (p[2] << 8 | p[3]) << 5;
        *(s32 *)(r + 0x8) = (p[4] << 8 | p[5]) << 5;
        *(s32 *)(r + 0xC) = (p[6] << 8 | p[7]) << 5;
        r[0x10] = p[8];
        n = p[9];
        r[0x13] = n;
        p += 10;
        q = r + 0x15;
        for (;;) {
            ENGINE_BLK(802A2D08);
            if (n == 0) {
                break;
            }
            ENGINE_BLK(802A2D10);
            t6 = *p++;
            *q++ = t6;
            n--;
        }
        ENGINE_BLK(802A2D28);
        r[0x11] = p[0];
        r[0x14] = p[1];
        t5 = 1;
        r[0x12] = 1;
        p += 2;
        r += 0x24;
    }
    ENGINE_BLK(802A2D4C);
    D_803BDFD4 = r;
    ENGINE_LEAVE(11, (u32)r);
    ENGINE_LEAVE(12, (u32)&D_803BDFD4);
    ENGINE_LEAVE(13, t5);
    ENGINE_LEAVE(14, t6);
}

/* func_802A1A9C: LevelHeader.unk74: its identity matrices (as many bytes
   as its first word says, twice) and its groups of 0x44-byte records, cut
   down to 0x28 bytes, after them.  Leaves $t5 &D_803F782C, $t6, $t7, $s1,
   $s2 and $s3 as its walk left them. */
REGS(t0)
void func_802A1A9C(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *src = AT(h, 0x74);
    u32 size = *(u32 *)src;
    u8 *t = D_80358070;
    u8 *end;
    u32 t6, t7 = 1, s1 = ENGINE_REG(17), s2 = ENGINE_REG(18), s3 = ENGINE_REG(19);

    ENGINE_BLK(802A1A9C);
    D_803F7820 = t;
    D_803F7824 = t + size;
    end = t + size * 2;
    t6 = (u32)end;
    for (;;) {
        ENGINE_BLK(802A1AE0);
        if (t == end) {
            break;
        }
        ENGINE_BLK(802A1AE8);
        *(s16 *)(t + 0x00) = 1;
        *(s16 *)(t + 0x02) = 0;
        *(s32 *)(t + 0x04) = 0;
        *(s16 *)(t + 0x08) = 0;
        *(s16 *)(t + 0x0A) = 1;
        *(s32 *)(t + 0x0C) = 0;
        *(s32 *)(t + 0x10) = 0;
        *(s16 *)(t + 0x14) = 1;
        *(s16 *)(t + 0x16) = 0;
        *(s32 *)(t + 0x18) = 0;
        *(s16 *)(t + 0x1C) = 0;
        *(s16 *)(t + 0x1E) = 1;
        *(s32 *)(t + 0x20) = 0;
        *(s32 *)(t + 0x24) = 0;
        *(s32 *)(t + 0x28) = 0;
        *(s32 *)(t + 0x2C) = 0;
        *(s32 *)(t + 0x30) = 0;
        *(s32 *)(t + 0x34) = 0;
        *(s32 *)(t + 0x38) = 0;
        *(s32 *)(t + 0x3C) = 0;
        t += 0x40;
    }
    ENGINE_BLK(802A1B40);
    D_803F7828 = t;
    if (size != 0) {
        u8 *g;
        s32 more;

        ENGINE_BLK(802A1B50);
        t6 = (u32)(src + 4);
        g = src + 4;
        do {
            u8 *e;

            ENGINE_BLK(802A1B58);
            t7 = (u32)g;
            more = *(s16 *)g;
            s1 = *(u32 *)(g + 0x28);
            e = g + 0x2C;
            for (;;) {
                ENGINE_BLK(802A1B68);
                if (s1 == 0) {
                    break;
                }
                ENGINE_BLK(802A1B70);
                s1--;
                *(s32 *)(t + 0x00) = *(s16 *)(e + 0x0) << 5;
                *(s32 *)(t + 0x04) = *(s16 *)(e + 0x2) << 5;
                *(s32 *)(t + 0x08) = *(s16 *)(e + 0x4) << 5;
                *(s32 *)(t + 0x0C) = *(s16 *)(e + 0x6) << 5;
                *(s32 *)(t + 0x10) = *(s16 *)(e + 0x8) << 5;
                *(s32 *)(t + 0x14) = *(s16 *)(e + 0xA) << 5;
                *(s32 *)(t + 0x18) = *(s16 *)(e + 0xC) << 5;
                *(s32 *)(t + 0x1C) = *(s16 *)(e + 0xE) << 5;
                *(s32 *)(t + 0x20) = *(s16 *)(e + 0x10) << 5;
                s3 = e[0x12];
                t[0x24] = s3;
                t += 0x28;
                e += 0x44;
            }
            g = e;
            s2 = (u32)e;
            ENGINE_BLK(802A1BF4);
        } while (more != -1);
    }
    ENGINE_BLK(802A1C00);
    D_803F782C = t;
    D_80358070 = t;
    ENGINE_LEAVE(13, (u32)&D_803F782C);
    ENGINE_LEAVE(14, t6);
    ENGINE_LEAVE(15, t7);
    ENGINE_LEAVE(17, s1);
    ENGINE_LEAVE(18, s2);
    ENGINE_LEAVE(19, s3);
}
