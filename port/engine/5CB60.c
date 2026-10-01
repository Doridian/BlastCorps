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
extern u8 D_803F7804;
#ifndef VERSION_US_V10
extern u8 D_803F7812;
#endif
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
#ifndef VERSION_US_V10
    D_803F7812 = 0;         /* (us.v10 doesn't) */
#endif
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
    u32 *out = (u32 *)D_803BDB10;

    ENGINE_BLK(802A4464);
    for (;;) {
        ENGINE_BLK(802A449C);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802A44A4);
        p += 4;
        *out++ = (u32)p;
        p = base + UNALIGNED_W(p - 4);
        n--;
    }
    ENGINE_BLK(802A44C8);
    *out = (u32)(p + 4);
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

/* ---- the collision triangles -------------------------------------------- */

#include "collision.h"

extern u8 D_803BD310[];         /* the walls: 0xFC-byte records */
extern u8 *PTR32 D_803BD308, *PTR32 D_803BD30C;     /* their triangles */
extern u8 D_803BC1D0[];         /* LevelHeader.unk68's records, 0xDC bytes */
extern u8 D_803B9890[];         /* ... their triangles */
extern u8 *PTR32 D_803BD300, *PTR32 D_803BD304;

/* a byte of a wider datum that the original reaches at its N64 address
   (tools/recomp/native_sites.txt's x3): in native-endian memory it is at
   the address ^ 3; and a halfword it keeps big-endian (`be`) */
#ifdef PORT_NATIVE_ENDIAN
#define X3(p) (*(u8 *)((u32)(p) ^ 3))
#define STORE_BE16(p, v) (*(u16 *)(p) = (u16)__builtin_bswap16((u16)(v)))
#else
#define X3(p) (*(u8 *)(p))
#define STORE_BE16(p, v) (*(u16 *)(p) = (u16)(v))
#endif

#define BE16S(p) ((s16)(((s8 *)(p))[0] << 8 | ((u8 *)(p))[1]))

/* func_802A41B0: the triangle at `t` from the file's at `src` (nine
   big-endian s16 corners at any alignment, a u16 at 0x12): its corners
   << 3, the edges' cross product, -(n . v1), |n| and |n|^2, and the
   normal's largest component.  The bytes 0x4F..0x58 come from registers:
   $t9, $v0, $t2 (a halfword), $gp, $t7, $t6 and $s1.  Returns `t` + 1, and
   leaves its temporaries as its callers read them. */
REGS(t4, t5, t2, t7, gp, t9, v0, t6, s1 -> t4)
u32 func_802A41B0(u32 t_, u32 src_, u32 h52, u32 b56, u32 b55, u32 b4f, u32 b50, u32 b57,
                  u32 b58) {
    CollisionTri *t = (CollisionTri *)t_;
    u8 *src = (u8 *)src_;
    s32 x0, y0, z0, x1, y1, z1, x2, y2, z2;
    s32 dy1, dz1, dx1, dy2, dz2, dx2;
    s64 nx, ny, nz, ax, ay, az, nzz1, s5;
    f32 len2, f;

    ENGINE_BLK(802A41B0);
    t->unk59 = 0;
    t->unk55 = b55;
    t->unk56 = b56;
    t->unk52 = h52;
    t->owner = b4f;
    t->unk50 = b50;
    t->unk57 = b57;
    t->unk58 = b58;
    x0 = BE16S(src + 0x0) << 3;
    y0 = BE16S(src + 0x2) << 3;
    z0 = BE16S(src + 0x4) << 3;
    x1 = BE16S(src + 0x6) << 3;
    y1 = BE16S(src + 0x8) << 3;
    z1 = BE16S(src + 0xA) << 3;
    x2 = BE16S(src + 0xC) << 3;
    y2 = BE16S(src + 0xE) << 3;
    z2 = BE16S(src + 0x10) << 3;
    dy1 = y0 - y1;
    dz1 = z0 - z1;
    dy2 = y0 - y2;
    dz2 = z0 - z2;
    dx2 = x0 - x2;
    dx1 = x0 - x1;
    t->v[0][0] = x0; t->v[0][1] = y0; t->v[0][2] = z0;
    t->v[1][0] = x1; t->v[1][1] = y1; t->v[1][2] = z1;
    t->v[2][0] = x2; t->v[2][1] = y2; t->v[2][2] = z2;
    nx = (s64)dy1 * dz2 - (s64)dz1 * dy2;
    t->nx = nx;
    len2 = (f32)nx;
    len2 = len2 * len2;
    ny = (s64)dz1 * dx2 - (s64)dx1 * dz2;
    t->ny = ny;
    f = (f32)ny;
    f = f * f;
    len2 = len2 + f;
    nz = (s64)dx1 * dy2 - (s64)dy1 * dx2;
    t->nz = nz;
    f = (f32)nz;
    f = f * f;
    len2 = len2 + f;
    nzz1 = nz * z1;
    t->d = -(nx * x1 + ny * y1 + nzz1);
    t->nlen2 = len2;
    t->nlen = __builtin_sqrtf(len2);
    ax = nx;
    if (ax < 0) {
        ENGINE_BLK(802A43B8);
        ax = -ax;
    }
    ENGINE_BLK(802A43BC);
    ay = ny;
    if (ay < 0) {
        ENGINE_BLK(802A43C4);
        ay = -ay;
    }
    ENGINE_BLK(802A43C8);
    az = nz;
    if (az < 0) {
        ENGINE_BLK(802A43D0);
        az = -az;
    }
    s5 = ay;
    ENGINE_BLK(802A43D4);
    if (az < ax || (ENGINE_BLK(802A43E0), az < ay)) {
        ENGINE_BLK(802A43F0);
        if (ay < ax || (ENGINE_BLK(802A43FC), ay < az)) {
            ENGINE_BLK(802A4410);
            s5 = 2;
            t->axis = 2;
        } else {
            ENGINE_BLK(802A4404);
            s5 = 1;
            t->axis = 1;
        }
    } else {
        ENGINE_BLK(802A43E8);
        t->axis = 0;
    }
    ENGINE_BLK(802A4418);
    t->unk54 = 0;
    t->unk4C = src[0x12] << 8 | src[0x13];
    ENGINE_LEAVE(1, src[0x12] << 8);
    ENGINE_LEAVE(4, y0);
    ENGINE_LEAVE(6, x1);
    ENGINE_LEAVE(7, y1);
    ENGINE_LEAVE(17, dx2);
    ENGINE_LEAVE(18, dy2);
    ENGINE_LEAVE(19, dz2);
    ENGINE_LEAVE(20, src[0x12] << 8 | src[0x13]);
    ENGINE_LEAVE64(21, s5);
    ENGINE_LEAVE64(22, az);
    ENGINE_LEAVE64(23, t->d);
    ENGINE_LEAVE64(24, nzz1);
    ENGINE_LEAVE_F(0, t->nlen);
    ENGINE_LEAVE_F(2, f);
    return (u32)(t + 1);
}

/* func_802A3D54 / func_802A3DF8: a grid of triangle lists (LevelHeader's
   unk10 cells; each list a big-endian end offset from the section, then
   0x16-byte triangles) into CollisionTris on the heap, a pointer to each
   cell's in `table` and one past them.  The triangles' 0x52 is the cells
   left, 0x57 the low byte of the list's end; 0x59 the file triangle's
   0x15. */
REGS(t0)
void func_802A3D54(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    s32 cells = (s16)h->unk10[0] * (s16)h->unk10[1];
    u8 *base = AT(h, 0x6C);
    u8 *p = base, *end = NULL;
    CollisionTri *t = (CollisionTri *)D_80358070;
    u32 *table = (u32 *)D_803BDCA8;

    ENGINE_BLK(802A3D54);
    do {
        ENGINE_BLK(802A3D98);
        *table++ = (u32)t;
        end = base + UNALIGNED_W(p);
        p += 4;
        for (;;) {
            ENGINE_BLK(802A3DB0);
            if (end == p) {
                break;
            }
            ENGINE_BLK(802A3DB8);
            t = (CollisionTri *)func_802A41B0((u32)t, (u32)p, cells, p[0x14], 0, ENGINE_REG(25),
                                              ENGINE_REG(2), (u32)end, ENGINE_REG(17));
            ENGINE_BLK(802A3DC0);
            t[-1].unk59 = p[0x15];
            p += 0x16;
        }
        ENGINE_BLK(802A3DD0);
    } while (--cells != 0);
    ENGINE_BLK(802A3DDC);
    *table = (u32)t;
    D_80358070 = (u8 *)t;
    ENGINE_LEAVE(14, (u32)end);
    ENGINE_LEAVE(28, 0);
}

REGS(t0)
void func_802A3DF8(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    s32 cells = (s16)h->unk10[0] * (s16)h->unk10[1];
    u8 *base = AT(h, 0x70);
    u8 *p = base, *end = NULL;
    CollisionTri *t = (CollisionTri *)D_80358070;
    u32 *table = (u32 *)D_803BDE40;

    ENGINE_BLK(802A3DF8);
    do {
        ENGINE_BLK(802A3E3C);
        *table++ = (u32)t;
        end = base + UNALIGNED_W(p);
        p += 4;
        for (;;) {
            ENGINE_BLK(802A3E54);
            if (end == p) {
                break;
            }
            ENGINE_BLK(802A3E5C);
            t = (CollisionTri *)func_802A41B0((u32)t, (u32)p, cells, p[0x14], 0, ENGINE_REG(25),
                                              ENGINE_REG(2), (u32)end, ENGINE_REG(17));
            ENGINE_BLK(802A3E64);
            t[-1].unk59 = p[0x15];
            p += 0x16;
        }
        ENGINE_BLK(802A3E74);
    } while (--cells != 0);
    ENGINE_BLK(802A3E80);
    *table = (u32)t;
    D_80358070 = (u8 *)t;
    ENGINE_LEAVE(14, (u32)end);
    ENGINE_LEAVE(28, 0);
}

/* func_802A3E9C: the walls (LevelHeader.unk64..unk68) into D_803BD310's
   0xFC-byte records: a count and that many bytes, then m triangles
   (0x14-byte ones) with a pointer to each from 0x08, m at 0xF8 and the
   record's first byte at 0xF9 */
REGS(t0)
void func_802A3E9C(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *p = AT(h, 0x64);
    u8 *end = AT(h, 0x68);
    u8 *w = D_803BD310;
    CollisionTri *t = (CollisionTri *)D_80358070;

    ENGINE_BLK(802A3E9C);
    D_803BD308 = (u8 *)t;
    for (;;) {
        u32 n, k;
        u32 *slot;

        ENGINE_BLK(802A3ED4);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A3EDC);
        X3(w + 0xF9) = p[0];
        n = p[1];
        w[0] = n;
        p += 2;
        for (k = 1;; k++) {
            ENGINE_BLK(802A3EF8);
            if (n == 0) {
                break;
            }
            ENGINE_BLK(802A3F00);
            n--;
            w[k] = *p++;
        }
        ENGINE_BLK(802A3F18);
        n = *p++;
        X3(w + 0xF8) = n;
        slot = (u32 *)(w + 8);
        for (;;) {
            ENGINE_BLK(802A3F28);
            if (n == 0) {
                break;
            }
            ENGINE_BLK(802A3F30);
            n--;
            *slot++ = (u32)t;
            t = (CollisionTri *)func_802A41B0((u32)t, (u32)p, (u32)end, (u32)slot, 1, ENGINE_REG(25),
                                              ENGINE_REG(2), n, ENGINE_REG(17));
            ENGINE_BLK(802A3F40);
            p += 0x14;
        }
        ENGINE_BLK(802A3F48);
        w += 0xFC;
    }
    ENGINE_BLK(802A3F50);
    D_803BDAF0 = w;
    D_803BD30C = (u8 *)t;
    D_80358070 = (u8 *)t;
    ENGINE_LEAVE(28, 1);
}

/* whether a triangle of [D_803B9890, end) has the id `id` (in 0x50):
   5CB60's 802A4168 */
static s32 tri_id_used(u32 id, CollisionTri *end) {
    CollisionTri *t = (CollisionTri *)D_803B9890;
    s32 found = 0;

    ENGINE_BLK(802A4168);
    for (;;) {
        ENGINE_BLK(802A4184);
        if (t == end) {
            break;
        }
        ENGINE_BLK(802A418C);
        if (t->unk50 == id) {
            ENGINE_BLK(802A4198);
            found = 1;
            break;
        }
        t++;
    }
    ENGINE_BLK(802A419C);
    return found;
}

/* func_802A3F80: LevelHeader.unk68..unk6C (the train stops and the like)
   into D_803BC1D0's 0xDC-byte records: three bytes; then n 12-byte
   entries (six big-endian s16, << 5, into 0x18-byte ones) and a u32 after
   them, or (if the second byte isn't 0) n s16; then n triangles of 0x15
   bytes, each id (0x14) once, from D_803B9890 on; then n bytes. */
REGS(t0)
void func_802A3F80(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *p = AT(h, 0x68);
    u8 *end = AT(h, 0x6C);
    u8 *r = D_803BC1D0;
    CollisionTri *tris = (CollisionTri *)D_803B9890;   /* $fp */
    u32 last = ENGINE_REG(15);                         /* $t7 */
    u32 s0 = ENGINE_REG(16), s1 = ENGINE_REG(17);
    u32 v0 = ENGINE_REG(2), t9 = ENGINE_REG(25);

    ENGINE_BLK(802A3F80);
    for (;;) {
        u32 n, kind;
        u8 *q = r;
        u8 *ids;
        CollisionTri *t;

        ENGINE_BLK(802A3FB0);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A3FB8);
        r[0xC4] = p[0];
        kind = p[1];
        r[0xC5] = kind;
        n = p[2];
        r[0xC6] = n;
        p += 3;
        if (kind == 0) {
            s32 c[6];

            do {
                ENGINE_BLK(802A3FDC);
                c[0] = BE16S(p + 0) << 5;
                c[1] = BE16S(p + 2) << 5;
                c[2] = BE16S(p + 4) << 5;
                c[3] = BE16S(p + 6) << 5;
                c[4] = BE16S(p + 8) << 5;
                c[5] = BE16S(p + 10) << 5;
                *(s32 *)(q + 0x00) = c[0];
                *(s32 *)(q + 0x04) = c[1];
                *(s32 *)(q + 0x08) = c[2];
                *(s32 *)(q + 0x0C) = c[3];
                *(s32 *)(q + 0x10) = c[4];
                *(s32 *)(q + 0x14) = c[5];
                p += 0xC;
                q += 0x18;
            } while (--n != 0);
            s1 = c[2];
            ENGINE_LEAVE(18, c[3]);
            ENGINE_LEAVE(19, c[4]);
            ENGINE_LEAVE(20, c[5]);
            ENGINE_BLK(802A407C);
            last = UNALIGNED_W(p);
            *(u32 *)q = last;
            p += 4;
        } else {
            do {
                ENGINE_BLK(802A4090);
                last = (u32)(s32)BE16S(p);
                STORE_BE16(q, last);
                p += 2;
                q += 2;
            } while (--n != 0);
        }
        ENGINE_BLK(802A40B4);
        n = *p++;
        t = tris;
        t9 = 0;
        ids = r + 0xC8;
        r[0xC7] = n;
        for (;;) {
            u32 id;

            ENGINE_BLK(802A40CC);
            if (n == 0) {
                break;
            }
            ENGINE_BLK(802A40D4);
            v0 = id = p[0x14];
            *ids++ = id;
            if (!tri_id_used(id, tris)) {
                ENGINE_BLK(802A40E4);
                ENGINE_BLK(802A40EC);
                t = (CollisionTri *)func_802A41B0((u32)t, (u32)p, (u32)end, last, 1, 0, id, n, s1);
                s1 = ENGINE_REG(17);
            } else {
                ENGINE_BLK(802A40E4);
            }
            ENGINE_BLK(802A40F4);
            p += 0x15;
            n--;
        }
        ENGINE_BLK(802A4100);
        tris = t;
        n = *p++;
        ids = r + 0xD1;
        r[0xD0] = n;
        ENGINE_BLK(802A4114);
        if (n != 0) {
            for (;;) {
                ENGINE_BLK(802A411C);
                *ids++ = *p++;
                if (--n == 0) {
                    break;
                }
                ENGINE_BLK(802A4114);
            }
        }
        s0 = (u32)ids;
        ENGINE_BLK(802A4134);
        r += 0xDC;
    }
    ENGINE_BLK(802A413C);
    D_803BD304 = r;
    D_803BD300 = (u8 *)tris;
    ENGINE_LEAVE(2, v0);
    ENGINE_LEAVE(15, last);
    ENGINE_LEAVE(16, s0);
    ENGINE_LEAVE(17, s1);
    ENGINE_LEAVE(25, t9);
    ENGINE_LEAVE(28, 1);
    ENGINE_LEAVE(30, (u32)tris);
}

/* ---- the buildings' parts and the textures ---------------------------- */

#include "game/model.h"

REGS(t6, fp -> s0)
u32 func_802A0CFC(u32 id, u32 param);
REGS(s0, s1, s4 -> s2, s3, s4, s5, s6)
u32 func_802A08E4(u32 dl, u32 end, u32 s4, u32 *s3_out, u32 *s4_out, u32 *s5_out, u32 *s6_out);
s32 func_802CE6F8(s32 x, s32 z, s32 y);

extern u8 *PTR32 D_803BE704, *PTR32 D_803BE708;    /* this level's building groups */
extern u8 D_802D30D0[], D_802D3194[], D_802D32A0[], D_802D331C[], D_802D33C8[], D_802D3444[],
    D_802D3538[], D_802D3614[], D_802D36C0[], D_802D3784[], D_802D3890[], D_802D393C[],
    D_802D3A00[], D_802D3A4C[], D_802D3BA0[], D_802D3BD4[], D_802D3CE0[], D_802D3DD4[],
    D_802D3EB0[], D_802D3F74[];
extern u8 D_8030631F[];
extern u32 D_80365330;

/* func_802A1EC8: this level's table of building groups (D_803BE708, its
   cursor D_803BE704 four bytes on), or none.  Leaves $at the high half of
   the last of the two it stored. */
#define LEVEL_TABLE(b_found, table)                                         \
    do {                                                                    \
        ENGINE_BLK(b_found);                                                \
        t = (table);                                                        \
        goto found;                                                         \
    } while (0)
#define LEVEL_TEST(b_test, level)                                           \
    ENGINE_BLK(b_test);                                                     \
    if (D_802E8BDC == (level))

REGS()
void func_802A1EC8(void) {
    u8 *t;

    ENGINE_BLK(802A1EC8);
    if (D_802E8BDC == 0) LEVEL_TABLE(802A1EE4, D_802D30D0);
    LEVEL_TEST(802A1EF0, 0x01) LEVEL_TABLE(802A1EFC, D_802D3194);
    LEVEL_TEST(802A1F08, 0x02) LEVEL_TABLE(802A1F14, D_802D32A0);
    LEVEL_TEST(802A1F20, 0x03) LEVEL_TABLE(802A1F2C, D_802D331C);
    LEVEL_TEST(802A1F38, 0x04) LEVEL_TABLE(802A1F44, D_802D33C8);
    LEVEL_TEST(802A1F50, 0x05) LEVEL_TABLE(802A1F5C, D_802D3444);
    LEVEL_TEST(802A1F68, 0x09) LEVEL_TABLE(802A1F74, D_802D3538);
    LEVEL_TEST(802A1F80, 0x0A) LEVEL_TABLE(802A1F8C, D_802D3614);
    LEVEL_TEST(802A1F98, 0x0C) LEVEL_TABLE(802A1FA4, D_802D36C0);
    LEVEL_TEST(802A1FB0, 0x0D) LEVEL_TABLE(802A1FBC, D_802D3784);
    LEVEL_TEST(802A1FC8, 0x0E) LEVEL_TABLE(802A1FD4, D_802D3890);
    LEVEL_TEST(802A1FE0, 0x0F) LEVEL_TABLE(802A1FEC, D_802D393C);
    LEVEL_TEST(802A1FF8, 0x10) LEVEL_TABLE(802A2004, D_802D3A00);
    LEVEL_TEST(802A2010, 0x11) LEVEL_TABLE(802A201C, D_802D3A4C);
    LEVEL_TEST(802A2028, 0x12) LEVEL_TABLE(802A2034, D_802D3BA0);
    LEVEL_TEST(802A2040, 0x1A) LEVEL_TABLE(802A204C, D_802D3BD4);
    LEVEL_TEST(802A2058, 0x1D) LEVEL_TABLE(802A2064, D_802D3CE0);
    LEVEL_TEST(802A2070, 0x21) LEVEL_TABLE(802A207C, D_802D3DD4);
    LEVEL_TEST(802A2088, 0x39) LEVEL_TABLE(802A2094, D_802D3EB0);
    LEVEL_TEST(802A20A0, 0x3A) LEVEL_TABLE(802A20AC, D_802D3F74);
    ENGINE_BLK(802A20B8);
    D_803BE704 = NULL;
    D_803BE708 = NULL;
    ENGINE_BLK(802A20E0);
    ENGINE_LEAVE(1, HI16(&D_803BE708));
    return;
found:
    ENGINE_BLK(802A20CC);
    D_803BE708 = t;
    D_803BE704 = t + 4;
    ENGINE_BLK(802A20E0);
    ENGINE_LEAVE(1, HI16(&D_803BE704));
}

/* func_802A20F4: in game mode 0x800, a model's vertex colours (0xC..0xE of
   the 0x10-byte vertices from 0x50) set to 0xFF, 0, 0, if `flag` */
REGS(t4, s0)
void func_802A20F4(u32 m_, u32 flag) {
    u8 *m = (u8 *)m_;

    ENGINE_BLK(802A20F4);
    if (flag != 0) {
        ENGINE_BLK(802A210C);
        if (D_80364A98 == 0x800) {
            u8 *v = m + 0x50, *end = AT(m, 0x1C);

            ENGINE_BLK(802A2120);
            for (;;) {
                ENGINE_BLK(802A2130);
                if (v == end) {
                    break;
                }
                ENGINE_BLK(802A2138);
                v[0xC] = 0xFF;
                v[0xD] = 0;
                v[0xE] = 0;
                v += 0x10;
            }
        }
    }
    ENGINE_BLK(802A214C);
}

/* func_802A2164: in level 0x31, model 0x24's unk4 is 2 */
REGS(t3, s0)
void func_802A2164(u32 n, u32 m_) {
    u8 *m = (u8 *)m_;

    ENGINE_BLK(802A2164);
    if (D_802E8BDC == 0x31) {
        ENGINE_BLK(802A2184);
        if (n == 0x24) {
            ENGINE_BLK(802A2190);
            m[4] = 2;
        }
    }
    ENGINE_BLK(802A2198);
}

/* func_802A23E0: a building's model's 0x38-byte effect records
   (Model.unk30..unk38): byte 0x31 through D_8030631F */
REGS(v1)
void func_802A23E0(u32 b_) {
    u8 *m = (u8 *)((Building *)b_)->model;
    u8 *p = AT(m, 0x30), *end = AT(m, 0x38);

    ENGINE_BLK(802A23E0);
    for (;;) {
        ENGINE_BLK(802A2418);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A2420);
        X3(p + 0x31) = D_8030631F[X3(p + 0x31)];
        p += 0x38;
    }
    ENGINE_BLK(802A2438);
}

/* func_802A2458: a building's centre in x and z, from its model's box */
REGS(v1, t4)
void func_802A2458(u32 b_, u32 m_) {
    u8 *b = (u8 *)b_;
    s16 *box = (s16 *)AT(m_, 0x1C);

    ENGINE_BLK(802A2458);
    *(s32 *)(b + 0x28) = ((box[0] + box[3]) >> 1) << 5;
    *(s32 *)(b + 0x2C) = ((box[2] + box[8]) >> 1) << 5;
}

/* func_802A2608: a model's animated textures loaded (Model.unk28..unk2C:
   a texture, a count n at 4, and n - 1 more from 0x10), each number
   replaced by its physical address.  Leaves $t1 and $t2 the records' end
   and $s1 0x80000000 where there are any. */
REGS(t4)
void func_802A2608(u32 m_) {
    u8 *m = (u8 *)m_;
    u8 *a = AT(m, 0x28), *end = AT(m, 0x2C);
    u32 fp = ENGINE_REG(30);

    ENGINE_BLK(802A2608);
    for (;;) {
        u32 *tex;
        s32 n;

        ENGINE_BLK(802A2638);
        if (a == end) {
            break;
        }
        ENGINE_BLK(802A2640);
        *(u32 *)a = func_802A0CFC(*(u32 *)a, fp);
        ENGINE_BLK(802A2648);
        n = a[4] - 1;
        tex = (u32 *)(a + 0x10);
        ENGINE_LEAVE(17, 0x80000000);
        for (;;) {
            ENGINE_BLK(802A265C);
            if (n == 0) {
                break;
            }
            ENGINE_BLK(802A2664);
            n--;
            *tex = func_802A0CFC(*tex, fp);
            ENGINE_BLK(802A2670);
            tex++;
        }
        ENGINE_BLK(802A267C);
        a = (u8 *)tex;
    }
    ENGINE_BLK(802A2684);
    ENGINE_LEAVE(9, (u32)end);
    ENGINE_LEAVE(10, (u32)end);
}

/* func_802A24BC: the three buildings 0xBA..0xBC get texture 0xF81
   (D_80365330) and a collision object (89250's func_802CE6F8) */
REGS(v1)
void func_802A24BC(u32 b_) {
    Building *b = (Building *)b_;

    ENGINE_BLK(802A24BC);
    if (b->unk30 != 0xBA) {
        ENGINE_BLK(802A254C);
        if (b->unk30 != 0xBB) {
            ENGINE_BLK(802A2554);
            if (b->unk30 != 0xBC) {
                ENGINE_BLK(802A2584);
                return;
            }
        }
    }
    ENGINE_BLK(802A255C);
    D_80365330 = func_802A0CFC(0xF81, ENGINE_REG(30));
    ENGINE_BLK(802A2564);
    b->unk44 = func_802CE6F8(b->x, b->z, b->y);
    ENGINE_BLK(802A2580);
    ENGINE_BLK(802A2584);
}

/* func_802A26A8: a model moved by (dx, dy, dz) */
REGS(t4, t5, t6, t7)
void func_802A26A8(u32 m_, s32 dx, s32 dy, s32 dz) {
    u8 *m = (u8 *)m_;
    u8 *p, *end;
    s32 i, k;

    ENGINE_BLK(802A26A8);
    for (p = AT(m, 0x40), end = AT(m, 0x44);; p += 2) {
        ENGINE_BLK(802A26CC);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A26D4);
        *(s16 *)p += dy;
    }
    ENGINE_BLK(802A26E8);
    p = AT(m, 0x20);
    *(s16 *)(p + 0) += dx;
    *(s16 *)(p + 2) += dz;
    *(s16 *)(p + 4) += dx;
    *(s16 *)(p + 6) += dz;
    for (p = AT(m, 0x24), end = AT(m, 0x28);; p += 0x14) {
        ENGINE_BLK(802A2730);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A2738);
        for (i = 0; i < 18; i += 6) {
            *(s16 *)(p + i + 0) += dx;
            *(s16 *)(p + i + 2) += dy;
            *(s16 *)(p + i + 4) += dz;
        }
    }
    ENGINE_BLK(802A27AC);
    for (p = AT(m, 0x1C), i = 4;; p += 6) {
        ENGINE_BLK(802A27B8);
        if (i == 0) {
            break;
        }
        ENGINE_BLK(802A27C0);
        i--;
        *(s16 *)(p + 0) += dx;
        *(s16 *)(p + 2) += dy;
        *(s16 *)(p + 4) += dz;
    }
    ENGINE_BLK(802A27F0);
    if (*(u16 *)(m + 0xE) == 0) {
        ENGINE_BLK(802A27FC);
        for (p = m + 0x50, end = AT(m, 0x1C);; p += 0x10) {
            ENGINE_BLK(802A2808);
            if (p == end) {
                break;
            }
            ENGINE_BLK(802A2810);
            *(s16 *)(p + 0) += dx;
            *(s16 *)(p + 2) += dy;
            *(s16 *)(p + 4) += dz;
        }
    }
    ENGINE_BLK(802A283C);
    for (p = AT(m, 0x48), end = AT(m, 0x4C);; p += 0x19) {
        ENGINE_BLK(802A284C);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A2854);
        for (i = 0; i < 18; i += 6) {
            s32 d[3];

            d[0] = dx; d[1] = dy; d[2] = dz;
            for (k = 0; k < 3; k++) {
                u32 v = (u32)(BE16S(p + i + 2 * k) + d[k]);

                p[i + 2 * k] = v >> 8;
                p[i + 2 * k + 1] = v;
            }
        }
    }
    ENGINE_BLK(802A297C);
    for (p = AT(m, 0x30), end = AT(m, 0x34);; p += 0x38) {
        ENGINE_BLK(802A298C);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A2994);
        *(s32 *)(p + 0x0) = (*(s32 *)(p + 0x0) + dx) << 16;
        *(s32 *)(p + 0x4) = (*(s32 *)(p + 0x4) + dy) << 16;
        *(s32 *)(p + 0x8) = (*(s32 *)(p + 0x8) + dz) << 16;
        *(s32 *)(p + 0x28) = (*(s32 *)(p + 0x28) + dy) << 16;
    }
    ENGINE_BLK(802A29DC);
    for (p = AT(m, 0x34), end = AT(m, 0x38);; p += 0x38) {
        ENGINE_BLK(802A29EC);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A29F4);
        *(s32 *)(p + 0x0) = (*(s32 *)(p + 0x0) + dx) << 16;
        *(s32 *)(p + 0x4) = (*(s32 *)(p + 0x4) + dy) << 16;
        *(s32 *)(p + 0x8) = (*(s32 *)(p + 0x8) + dz) << 16;
        *(s32 *)(p + 0x28) = (*(s32 *)(p + 0x28) + dy) << 16;
    }
    ENGINE_BLK(802A2A3C);
    for (p = AT(m, 0x2C), end = AT(m, 0x30);; p += 8) {
        ENGINE_BLK(802A2A4C);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A2A54);
        *(s16 *)(p + 0) += dx;
        *(s16 *)(p + 2) += dy;
        *(s16 *)(p + 4) += dz;
    }
    ENGINE_BLK(802A2A80);
}

/* func_802A1C20: the level's animated textures' frames loaded
   (LevelHeader.animTextures: 8 + 4 n bytes each, frames 1.. from 0xC).
   Leaves $t6 the last number, $s0 the last address and $s1 0x80000000. */
REGS(t0)
void func_802A1C20(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *a = AT(h, 0x2C), *end = AT(h, 0x30);
    u32 fp = ENGINE_REG(30);

    ENGINE_BLK(802A1C20);
    for (;;) {
        u32 *tex;
        s32 n;

        ENGINE_BLK(802A1C38);
        if (a == end) {
            break;
        }
        ENGINE_BLK(802A1C40);
        tex = (u32 *)(a + 0xC);
        n = a[4] - 1;
        ENGINE_LEAVE(17, 0x80000000);
        for (;;) {
            u32 phys;

            ENGINE_BLK(802A1C50);
            if (n == 0) {
                break;
            }
            ENGINE_BLK(802A1C58);
            n--;
            ENGINE_LEAVE(14, *tex);
            phys = func_802A0CFC(*tex, fp);
            ENGINE_LEAVE(16, phys);
            ENGINE_BLK(802A1C64);
            *tex++ = phys;
        }
        ENGINE_BLK(802A1C70);
        a = (u8 *)tex;
    }
    ENGINE_BLK(802A1C78);
}

/* func_802A3008: the level's display lists' textures (802A08E4 over
   [unk78, unk84)), leaving its registers */
REGS(t0)
void func_802A3008(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u32 s2, s3, s4, s5, s6;

    ENGINE_BLK(802A3008);
    s2 = func_802A08E4((u32)AT(h, 0x78), (u32)AT(h, 0x84), ENGINE_REG(20), &s3, &s4, &s5, &s6);
    ENGINE_BLK(802A3028);
    ENGINE_LEAVE(17, (u32)AT(h, 0x84));
    ENGINE_LEAVE(18, s2);
    ENGINE_LEAVE(19, s3);
    ENGINE_LEAVE(20, s4);
    ENGINE_LEAVE(21, s5);
    ENGINE_LEAVE(22, s6);
}

/* ---- the buildings ------------------------------------------------------ */

/* The originals keep their values in registers that the loader's
   translated code may read after them; these mirror each write of one
   into the context, in the original's order, so that it ends as the
   original leaves it. */
#define R_AT 1
#define R_V0 2
#define R_V1 3
#define R_A0 4
#define R_A1 5
#define R_A2 6
#define R_A3 7
#define R_T1 9
#define R_T2 10
#define R_T3 11
#define R_T4 12
#define R_T5 13
#define R_T6 14
#define R_T7 15
#define R_S0 16
#define R_S1 17
#define R_S7 23
#define R_T9 25
#define R_GP 28

u32 func_802C4108(u32 src, u32 dst, u32 arg2, u32 *dst_end);
extern u32 *PTR32 D_803BE6F0;   /* the model table, on the heap */
extern OSMesgQueue D_803150A0;
extern OSIoMesg D_80370C58;
extern u8 D_006EC4C0[];         /* the model table's ROM address */
extern u8 D_8036EB93;
extern u8 D_803EFED0[], D_803F0900[], D_803F1BE0[];
extern s16 D_803F767C, D_803F767E, D_803F7680;

#define INIT_AREA 0x8021ED00    /* init's memory: where models are DMA'd before inflating */
#define GZIP_WINDOW 0x8004B400

/* func_802A2BB0: the model table, 0x800 bytes, onto the heap */
REGS()
void func_802A2BB0(void) {
    u8 *t = D_80358070;

    ENGINE_BLK(802A2BB0);
    D_803BE6F0 = (u32 *)t;
    D_80358070 = t + 0x800;
    ENGINE_LEAVE(R_T1, (u32)(t + 0x800));
    ENGINE_LEAVE(R_T2, 0x800);
    ENGINE_LEAVE(R_S0, (u32)t);
    osInvalDCache(t, 0x800);
    ENGINE_BLK(802A2BF4);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, (u32)D_006EC4C0, t, 0x800, &D_803150A0);
    ENGINE_BLK(802A2C28);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(802A2C3C);
    ENGINE_LEAVE(R_A0, (u32)&D_803150A0);
    ENGINE_LEAVE(R_A1, 0);
    ENGINE_LEAVE(R_A2, 1);
    ENGINE_LEAVE(R_A3, (u32)D_006EC4C0);
}

/* func_802A2A98: model `n` of the model table: DMA'd to init's memory,
   its two gzip members inflated onto the heap, the heap's top rounded up.
   Returns it (the original's $s0). */
REGS(t3 -> s0)
u32 func_802A2A98(u32 n) {
    u32 *table = D_803BE6F0;
    u32 start = table[n], size = table[n + 1] - start;
    u32 src, dst, top;
    u8 *m;

    ENGINE_BLK(802A2A98);
    ENGINE_LEAVE(R_S7, INIT_AREA);
    osInvalDCache((void *)INIT_AREA, size);
    ENGINE_BLK(802A2B04);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, (u32)D_006EC4C0 + start,
                 (void *)INIT_AREA, size, &D_803150A0);
    ENGINE_BLK(802A2B30);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(802A2B44);
    src = func_802C4108(INIT_AREA, (u32)D_80358070, GZIP_WINDOW, &dst);
    ENGINE_BLK(802A2B60);
    src = func_802C4108(src, dst, GZIP_WINDOW, &dst);
    ENGINE_BLK(802A2B68);
    ENGINE_LEAVE(R_A0, src);
    ENGINE_LEAVE(R_A2, GZIP_WINDOW);
    ENGINE_LEAVE(R_A3, 0xA);
    top = func_802A44E4(dst);
    ENGINE_LEAVE(R_A1, top);
    ENGINE_BLK(802A2B70);
    m = D_80358070;
    D_80358070 = (u8 *)top;
    ENGINE_LEAVE(R_S0, (u32)m);
    return (u32)m;
}

/* func_802A21AC: building `n` with model `m` at (x, y, z), its flag and
   unk34: the next Building, its collision triangles (the model's 0x19-byte
   ones) on the heap */
REGS(t4, t3, t5, t6, t7, s0, t9)
void func_802A21AC(u32 m_, u32 n, u32 x, u32 y, u32 z, u32 flag, u32 unk34) {
    u8 *m = (u8 *)m_;
    Building *b;
    u32 i, cell, a0;
    u8 *src, *end;
    CollisionTri *t;

    ENGINE_BLK(802A21AC);
    func_802A2608((u32)m);
    ENGINE_BLK(802A21C4);
    ENGINE_LEAVE(R_AT, HI16(&D_803F767C));
    if (n == 0x38) {
        ENGINE_BLK(802A21D0);
        D_803F767C = x;
        D_803F767E = y;
        D_803F7680 = z;
        ENGINE_LEAVE(R_AT, HI16(&D_803F7680));
    }
    ENGINE_BLK(802A21E4);
    b = D_803F7654;
    D_803F7654 = b + 1;
    ENGINE_LEAVE(R_V0, (u32)&D_803F7654);
    ENGINE_LEAVE(R_V1, (u32)b);
    func_802A2458((u32)b, (u32)m);
    ENGINE_BLK(802A21FC);
    b->unk34 = unk34;
    b->unk38 = 0;
    b->unk3C = 0;
    b->unk40 = 0;
    b->unk30 = n;
    b->unkEB = flag;
    b->unkEA = 0;
    b->model = (struct Model *)m;
    b->unkC = ((u16 *)m)[1] << 5;
    b->x = b->unk1C = x << 5;
    b->y = b->unk20 = y << 5;
    b->z = b->unk24 = z << 5;
    b->unkE9 = ((u16 *)m)[0];
    for (i = ((u16 *)m)[0];; i--) {
        ENGINE_BLK(802A2258);
        if (i == 0) {
            break;
        }
        ENGINE_BLK(802A2260);
        (&b->unkEC)[((u16 *)m)[0] - i] = 0;
    }
    ENGINE_BLK(802A2270);
    for (i = ((u16 *)m)[0];; i--) {
        ENGINE_BLK(802A2278);
        if (i == 0) {
            break;
        }
        ENGINE_BLK(802A2280);
        ((u16 *)((u8 *)b + 0x48))[((u16 *)m)[0] - i] = 0;
    }
    ENGINE_BLK(802A2290);
    for (i = ((u16 *)m)[0];; i--) {
        ENGINE_BLK(802A2298);
        if (i == 0) {
            break;
        }
        ENGINE_BLK(802A22A0);
        ((u16 *)((u8 *)b + 0x68))[((u16 *)m)[0] - i] = 0;
    }
    ENGINE_BLK(802A22B0);
    if (D_803BE704 != NULL) {
        ENGINE_BLK(802A22C0);
        if (flag != 0) {
            ENGINE_BLK(802A22C8);
            *(u32 *)D_803BE704 = (u32)b;
            D_803BE704 += 0x18;
            ENGINE_LEAVE(R_AT, HI16(&D_803BE704));
        }
    }
    ENGINE_BLK(802A22D8);
    ENGINE_BLK(802A22F8);
    a0 = (u32)D_803BE70C >> 5;
    cell = z / ((u32)D_803BE710 >> 5) * D_803BE714 + x / a0;
    ENGINE_BLK(802A234C);
    b->unkE8 = cell;
    ENGINE_LEAVE(R_A0, a0);
    ENGINE_LEAVE(R_T5, x / a0);
    ENGINE_LEAVE(R_T7, cell);
    func_802A24BC((u32)b);
    ENGINE_BLK(802A2358);
    func_802A23E0((u32)b);
    ENGINE_BLK(802A2360);
    src = AT(m, 0x48);
    end = AT(m, 0x4C);
    t = (CollisionTri *)D_80358070;
    b->unk4 = t;
    ENGINE_LEAVE(R_S0, 1);
    for (;;) {
        ENGINE_BLK(802A2384);
        if (src == end) {
            break;
        }
        ENGINE_BLK(802A238C);
        ENGINE_LEAVE(R_T6, src[0x17]);
        if (src[0x17] != 0) {
            ENGINE_BLK(802A2398);
            t->unk51 = 0;
        } else {
            ENGINE_BLK(802A23A0);
            t->unk51 = 1;
        }
        ENGINE_BLK(802A23A4);
        ENGINE_LEAVE(R_GP, src[0x18]);
        ENGINE_LEAVE(R_T7, src[0x14]);
        t = (CollisionTri *)func_802A41B0((u32)t, (u32)src, src[0x15], src[0x14], src[0x18], unk34,
                                          (u32)&D_803F7654, src[0x17], src[0x16]);
        ENGINE_BLK(802A23B8);
        src += 0x19;
    }
    ENGINE_BLK(802A23C0);
    b->unk8 = t;
    D_80358070 = (u8 *)t;
    ENGINE_LEAVE(R_T4, (u32)t);
    ENGINE_LEAVE(R_T5, (u32)end);
}

/* func_802A1D54: the buildings (LevelHeader.buildings, 14-byte records:
   x, y, z as big-endian u16, the model number, behaviour, two flags,
   unk34), after the destruction tables are reset and the model table is
   in */
REGS(t0)
void func_802A1D54(u32 h_) {
    LevelHeader *h = (LevelHeader *)h_;
    u8 *p, *end, *q;
    s32 i;

    ENGINE_BLK(802A1D54);
    D_8036EB93 = 0;
    ENGINE_LEAVE(R_AT, HI16(&D_8036EB93));
    func_802A1EC8();
    ENGINE_BLK(802A1D68);
    for (i = 1, q = D_803EFED0;; q += 0xA30) {
        ENGINE_BLK(802A1D74);
        if (i == 0) {
            break;
        }
        ENGINE_BLK(802A1D7C);
        i--;
        q[0xA2A] = 0;
        q[0xA2B] = 0;
    }
    ENGINE_BLK(802A1D90);
    for (i = 4, q = D_803F0900;; q += 0x4B8) {
        ENGINE_BLK(802A1D9C);
        if (i == 0) {
            break;
        }
        ENGINE_BLK(802A1DA4);
        i--;
        *(s16 *)(q + 0x4B0) = 0;
        q[0x4B2] = 0;
    }
    ENGINE_BLK(802A1DB8);
    for (i = 2, q = D_803F1BE0;; q += 0x478) {
        ENGINE_BLK(802A1DC4);
        if (i == 0) {
            break;
        }
        ENGINE_BLK(802A1DCC);
        i--;
        q[0x470] = 0;
        q[0x472] = 0;
    }
    ENGINE_BLK(802A1DE0);
    D_803F7654 = D_803F4030;
    func_802A2BB0();
    ENGINE_BLK(802A1DF8);
    p = AT(h, 0x5C);
    end = AT(h, 0x60);
    ENGINE_LEAVE(R_T2, (u32)end);
    for (;;) {
        u32 n, x, y, z, s2, s3, s4, s5, s6;
        u8 *m;

        ENGINE_LEAVE(R_T1, (u32)p);
        ENGINE_BLK(802A1E08);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A1E10);
        n = p[6] << 8 | p[7];
        ENGINE_LEAVE(R_AT, p[6] << 8);
        ENGINE_LEAVE(R_T3, n);
        m = (u8 *)func_802A2A98(n);
        ENGINE_BLK(802A1E24);
        func_802A2164(n, (u32)m);
        ENGINE_BLK(802A1E2C);
        ENGINE_LEAVE(R_T4, (u32)m);
        ENGINE_LEAVE(R_S1, (u32)AT(m, 0x14));
        s2 = func_802A08E4((u32)AT(m, 0x10), (u32)AT(m, 0x14), ENGINE_REG(20), &s3, &s4, &s5, &s6);
        ENGINE_LEAVE(18, s2);
        ENGINE_LEAVE(19, s3);
        ENGINE_LEAVE(20, s4);
        ENGINE_LEAVE(21, s5);
        ENGINE_LEAVE(22, s6);
        ENGINE_BLK(802A1E44);
        *(u16 *)(m + 0xE) = *(u16 *)(p + 0xA);
        x = p[0] << 8 | p[1];
        y = p[2] << 8 | p[3];
        z = p[4] << 8 | p[5];
        ENGINE_LEAVE(R_AT, p[4] << 8);
        ENGINE_LEAVE(R_T5, x);
        ENGINE_LEAVE(R_T6, y);
        ENGINE_LEAVE(R_T7, z);
        func_802A26A8((u32)m, x, y, z);
        ENGINE_BLK(802A1E80);
        func_802A20F4((u32)m, p[8]);
        ENGINE_BLK(802A1E88);
        m[6] = p[9];
        D_8036EB93 += p[9];
        ENGINE_LEAVE(R_S1, D_8036EB93);
        ENGINE_LEAVE(R_AT, HI16(&D_8036EB93));
        ENGINE_LEAVE(R_S0, p[8]);
        ENGINE_LEAVE(R_T9, *(u16 *)(p + 0xC));
        func_802A21AC((u32)m, n, x, y, z, p[8], *(u16 *)(p + 0xC));
        ENGINE_BLK(802A1EB0);
        p += 0xE;
    }
    ENGINE_BLK(802A1EB8);
}

/* ---- the vehicles' records --------------------------------------------- */

#include "shared.h"

void func_80278BF0(void *, void *, void *);

/* func_802A1388 (the vehicle modules'): the next Vehicle, from its model
   file `model`: the model's pointers, and its display lists copied to the
   heap with pointers into the copy; then its heap block (hd.c's
   func_80278BF0, through 802A1558), unless in the attract mode without
   `a1` */
REGS(a0, a1, v0, v1, s2)
void func_802A1388(s32 type, s32 a1, u8 *buf1, u8 *buf2, u8 *model) {
    Vehicle *v = D_803649D0;
    u8 *src, *end, *dst;
    s32 delta;

    ENGINE_BLK(802A1388);
    D_803649D0 = v + 1;
    v->unk70 = 0;
    v->unk4 = buf1;
    v->unk8 = buf2;
    v->type = type;
    v->unk0 = AT(model, 0x14);
    v->unkC = AT(model, 0x24);
    v->unk10 = AT(model, 0x28);
    v->unk14 = AT(model, 0x2C);
    v->unk18 = AT(model, 0x30);
    v->unk1C = AT(model, 0x34);
    v->unk20 = AT(model, 0x38);
    v->unk24 = AT(model, 0x3C);
    v->unk28 = AT(model, 0x40);
    v->unk2C = AT(model, 0x44);
    v->unk54 = (Gfx *)AT(model, 0x48);
    src = AT(model, 0x1C);
    end = AT(model, 0x20);
    dst = D_80358070;
    delta = dst - (u8 *)v->unkC;
    for (;;) {
        ENGINE_BLK(802A1478);
        if (src == end) {
            break;
        }
        ENGINE_BLK(802A1480);
        ((u32 *)dst)[0] = ((u32 *)src)[0];
        ((u32 *)dst)[1] = ((u32 *)src)[1];
        src += 8;
        dst += 8;
    }
    ENGINE_BLK(802A1494);
    D_80358070 = dst;
    v->unk30 = (u8 *)v->unkC + delta;
    v->unk34 = (u8 *)v->unk10 + delta;
    v->unk38 = (u8 *)v->unk14 + delta;
    v->unk3C = (u8 *)v->unk18 + delta;
    v->unk40 = (u8 *)v->unk1C + delta;
    v->unk44 = (u8 *)v->unk20 + delta;
    v->unk48 = (u8 *)v->unk24 + delta;
    v->unk4C = (u8 *)v->unk28 + delta;
    v->unk50 = (u8 *)v->unk2C + delta;
    ENGINE_LEAVE(R_AT, 1);
    if (D_80364AA8 == 1) {
        ENGINE_BLK(802A1518);
        if (a1 == 0) {
            ENGINE_BLK(802A1530);
            return;
        }
    }
    ENGINE_BLK(802A1520);
    ENGINE_BLK(802A1558);
    func_80278BF0(v->unkC, v->unk18, &v->unk58);
    ENGINE_BLK(802A15EC);
    ENGINE_BLK(802A1530);
}

/* func_802A133C (the vehicle modules'): the Vehicle of `type` moves to
   (x, y, z), with the state's byte 0x9B */
REGS(v0, v1, a0, a1, gp)
void func_802A133C(s32 x, s32 y, s32 z, s32 type, VS *vs) {
    Vehicle *v = D_80364460;

    ENGINE_BLK(802A133C);
    for (;;) {
        ENGINE_BLK(802A1354);
        if (v->type == type) {
            break;
        }
        v++;
    }
    ENGINE_BLK(802A1360);
    v->x = x;
    v->y = y;
    v->z = z;
    v->unk70 = ((u8 *)vs)[0x9B];
}

/* ---- the vehicles' and the cargo's model files -------------------------- */

extern u8 D_0048FE90[], D_004903C0[], D_00490AC0[], D_00491E00[], D_004929D0[], D_00494390[],
    D_00496AD0[], D_00497AF0[], D_004989E0[], D_00499690[], D_0049AD20[], D_0049B630[],
    D_0049BCE0[], D_0049C480[], D_0049E8E0[], D_0049F7A0[], D_0049FF70[], D_004A0720[],
    D_004A1000[], D_004A1690[], D_004A4120[], D_004A5660[];
REGS(s0, s1, s2)
void func_8029DF78(u32 dl, u32 end, u32 type);

#define R_S2 18
#define R_S4 20
#define R_S5 21
#define R_S6 22

#define MODEL_TEST(b_test, ty, b_go, s, e)                                  \
    ENGINE_BLK(b_test);                                                     \
    if (type == (ty)) {                                                     \
        ENGINE_BLK(b_go);                                                   \
        start = (u32)(s);                                                   \
        end = (u32)(e);                                                     \
        goto found;                                                         \
    }

/* func_802A396C: vehicle model `type` (or the carrier's 0xFF, the J-Bomb's
   0xFE, the chopper's 0xFD, the extra models 0x96 and 0x98): returns it
   ($s2), its display list's textures put in (8029DF78 and 802A08E4) */
REGS(t3 -> s2)
u32 func_802A396C(u32 type) {
    u32 start, end, size, src, dst, top, s2, s3, s4, s5, s6;
    u8 *m;

    ENGINE_BLK(802A396C);
    if (type == 0) {
        ENGINE_BLK(802A3A5C);
        start = (u32)D_00491E00;
        end = (u32)D_004929D0;
        goto found;
    }
    MODEL_TEST(802A39A4, 0x01, 802A3A74, D_004929D0, D_00494390)
    MODEL_TEST(802A39B0, 0x02, 802A3A8C, D_00494390, D_00496AD0)
    MODEL_TEST(802A39B8, 0x10, 802A3AA4, D_004A1690, D_004A4120)
    MODEL_TEST(802A39C0, 0x03, 802A3ABC, D_00490AC0, D_00491E00)
    MODEL_TEST(802A39C8, 0x04, 802A3AD4, D_00496AD0, D_00497AF0)
    MODEL_TEST(802A39D0, 0x05, 802A3AEC, D_00497AF0, D_004989E0)
    MODEL_TEST(802A39D8, 0x06, 802A3B04, D_0049AD20, D_0049B630)
    MODEL_TEST(802A39E0, 0x07, 802A3B1C, D_0049B630, D_0049BCE0)
    MODEL_TEST(802A39E8, 0x08, 802A3B34, D_0049BCE0, D_0049C480)
    MODEL_TEST(802A39F0, 0x09, 802A3B4C, D_0049C480, D_0049E8E0)
    MODEL_TEST(802A39F8, 0x0A, 802A3B64, D_0049E8E0, D_0049F7A0)
    MODEL_TEST(802A3A00, 0x0B, 802A3B7C, D_0049F7A0, D_0049FF70)
    MODEL_TEST(802A3A08, 0x11, 802A3B7C, D_0049F7A0, D_0049FF70)
    MODEL_TEST(802A3A10, 0x12, 802A3B7C, D_0049F7A0, D_0049FF70)
    MODEL_TEST(802A3A18, 0x0D, 802A3B94, D_0049FF70, D_004A0720)
    MODEL_TEST(802A3A20, 0x0E, 802A3BAC, D_004A0720, D_004A1000)
    MODEL_TEST(802A3A28, 0x0F, 802A3BC4, D_004A1000, D_004A1690)
    MODEL_TEST(802A3A30, 0xFE, 802A3BDC, D_004989E0, D_00499690)
    MODEL_TEST(802A3A38, 0xFF, 802A3BF4, D_00499690, D_0049AD20)
    MODEL_TEST(802A3A40, 0xFD, 802A3C0C, D_004A4120, D_004A5660)
    MODEL_TEST(802A3A48, 0x96, 802A3C24, D_004903C0, D_00490AC0)
    MODEL_TEST(802A3A50, 0x98, 802A3C3C, D_0048FE90, D_004903C0)
    ENGINE_BLK(802A3A58);
    *(volatile u32 *)0 = 0;     /* (the original's syscall: an unknown type) */
found:
    size = end - start;
    ENGINE_LEAVE(R_S4, start);
    ENGINE_LEAVE(R_S5, size);
    ENGINE_LEAVE(R_S7, INIT_AREA);
    ENGINE_LEAVE(R_S2, type);
    ENGINE_BLK(802A3C54);
    osInvalDCache((void *)INIT_AREA, size);
    ENGINE_BLK(802A3C70);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, start, (void *)INIT_AREA, size,
                 &D_803150A0);
    ENGINE_BLK(802A3C9C);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(802A3CB0);
    src = func_802C4108(INIT_AREA, (u32)D_80358070, GZIP_WINDOW, &dst);
    ENGINE_BLK(802A3CCC);
    src = func_802C4108(src, dst, GZIP_WINDOW, &dst);
    ENGINE_BLK(802A3CD4);
    ENGINE_LEAVE(R_A0, src);
    ENGINE_LEAVE(R_A2, GZIP_WINDOW);
    ENGINE_LEAVE(R_A3, 0xA);
    top = func_802A44E4(dst);
    ENGINE_LEAVE(R_A1, top);
    ENGINE_BLK(802A3CDC);
    m = D_80358070;
    D_80358070 = (u8 *)top;
    ENGINE_LEAVE(R_S7, (u32)m);
    func_8029DF78((u32)AT(m, 0x1C), (u32)AT(m, 0x20), type);
    ENGINE_BLK(802A3D04);
    s2 = func_802A08E4((u32)AT(m, 0x1C), (u32)AT(m, 0x20), ENGINE_REG(R_S4), &s3, &s4, &s5, &s6);
    ENGINE_LEAVE(R_S4, s4);
    ENGINE_LEAVE(R_S5, s5);
    ENGINE_LEAVE(R_S6, s6);
    ENGINE_BLK(802A3D18);
    ENGINE_LEAVE(R_S2, (u32)m);
    return (u32)m;
}

/* func_802A32CC: the carrier's cargo model `type` (the vehicles 3, 4, 5, 8,
   9, 10, 13, 14, 15): returns it ($s2), its display list's textures put in */
REGS(t3 -> s2)
u32 func_802A32CC(u32 type) {
    u32 start, end, size, src, dst, top, s2, s3, s4, s5, s6;
    u8 *m;

    ENGINE_BLK(802A32CC);
    if (type == 3) {
        ENGINE_BLK(802A334C);
        start = (u32)D_00490AC0;
        end = (u32)D_00491E00;
        goto found;
    }
    MODEL_TEST(802A3304, 0x04, 802A3364, D_00496AD0, D_00497AF0)
    MODEL_TEST(802A3310, 0x05, 802A337C, D_00497AF0, D_004989E0)
    MODEL_TEST(802A3318, 0x08, 802A3394, D_0049BCE0, D_0049C480)
    MODEL_TEST(802A3320, 0x09, 802A33AC, D_0049C480, D_0049E8E0)
    MODEL_TEST(802A3328, 0x0A, 802A33C4, D_0049E8E0, D_0049F7A0)
    MODEL_TEST(802A3330, 0x0D, 802A33DC, D_0049FF70, D_004A0720)
    MODEL_TEST(802A3338, 0x0E, 802A33F4, D_004A0720, D_004A1000)
    MODEL_TEST(802A3340, 0x0F, 802A340C, D_004A1000, D_004A1690)
    ENGINE_BLK(802A3348);
    *(volatile u32 *)0 = 0;     /* (the original's syscall) */
found:
    size = end - start;
    ENGINE_LEAVE(R_S4, start);
    ENGINE_LEAVE(R_S5, size);
    ENGINE_LEAVE(R_S7, INIT_AREA);
    ENGINE_LEAVE(R_S2, type);
    ENGINE_BLK(802A3424);
    osInvalDCache((void *)INIT_AREA, size);
    ENGINE_BLK(802A3440);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, start, (void *)INIT_AREA, size,
                 &D_803150A0);
    ENGINE_BLK(802A346C);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(802A3480);
    src = func_802C4108(INIT_AREA, (u32)D_80358070, GZIP_WINDOW, &dst);
    ENGINE_BLK(802A349C);
    src = func_802C4108(src, dst, GZIP_WINDOW, &dst);
    ENGINE_BLK(802A34A4);
    ENGINE_LEAVE(R_A0, src);
    ENGINE_LEAVE(R_A2, GZIP_WINDOW);
    ENGINE_LEAVE(R_A3, 0xA);
    top = func_802A44E4(dst);
    ENGINE_LEAVE(R_A1, top);
    ENGINE_BLK(802A34AC);
    m = D_80358070;
    D_80358070 = (u8 *)top;
    ENGINE_LEAVE(R_S7, (u32)m);
    s2 = func_802A08E4((u32)AT(m, 0x1C), (u32)AT(m, 0x20), ENGINE_REG(R_S4), &s3, &s4, &s5, &s6);
    ENGINE_LEAVE(19, s3);
    ENGINE_LEAVE(R_S4, s4);
    ENGINE_LEAVE(R_S5, s5);
    ENGINE_LEAVE(R_S6, s6);
    ENGINE_BLK(802A34D4);
    ENGINE_LEAVE(R_S2, (u32)m);
    return (u32)m;
}
