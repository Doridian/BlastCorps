/*
 * hd_code 5BF40 (us.v11 0x802A0700-0x802A1320): the texture loader, as
 * native C (engine.h).
 *
 * The ROM's texture table (docs/ASSETS.md, "Texture table") is DMA'd onto
 * the heap once (D_803B8D44).  A texture is DMA'd from its entry's offset
 * to the next entry's and decoded in place by its type (60F60's
 * func_802A57DC), straight away or (func_802A1074) when its DMA is in.
 * D_803B8570 lists what is loaded, by number and physical address, up to
 * D_803B8D40; display lists name textures by number in their G_SETTIMG
 * words, and func_802A08E4 puts the addresses in.
 */
#include "engine.h"
#include "texture.h"
#include "game/game.h"

extern TexCacheEntry D_803B8570[250];
extern TexCacheEntry *PTR32 D_803B8D40;     /* the list's end */
extern TextureEntry *PTR32 D_803B8D44;      /* the table, on the heap */
extern OSIoMesg D_803B8D48[144];            /* one per DMA in flight */
extern u8 D_803B9888;                       /* the table is in */
extern TexDecode D_803C4B58;
extern OSMesgQueue D_80315180;
extern OSIoMesg D_80370C58;
extern s32 D_80358080;                      /* DMAs in flight */
extern s32 D_80358084;                      /* raw ones among them */
extern u8 D_00004CE0[];                     /* the texture table's ROM address */

#define TABLE_ROM ((u32)D_00004CE0)
#define PHYS(p) ((u32)(p) - 0x80000000)

REGS(s1, s2, s3, fp)
void func_802A5764(u32 dst, u32 length, u32 type, u32 param);

/* func_802A0700 (the C's and the level loader's): bring the table in,
   once */
REGS()
void func_802A0700(void) {
    ENGINE_BLK(802A0700);
    if (D_803B9888 == 0) {
        u8 *table = D_80358070;

        ENGINE_BLK(802A0790);
        D_803B8D44 = (TextureEntry *)table;
        D_80358070 = table + 0x8000;
        osInvalDCache(table, 0x8000);
        ENGINE_BLK(802A07C4);
        osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM, table, 0x8000, &D_80315180);
        ENGINE_BLK(802A07F8);
        osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
        ENGINE_BLK(802A080C);
        D_803B8D40 = D_803B8570;
        D_803B9888 = 1;
    }
    ENGINE_BLK(802A082C);
}

/* func_802A08E4: every G_SETTIMG in [dl, end) names a texture by number:
   load the ones not loaded yet and put their addresses in.  The original
   leaves $s2..$s6 as it used them, which its callers may see. */
REGS(s0, s1, s4 -> s2, s3, s4, s5, s6)
u32 func_802A08E4(u32 dl_, u32 end_, u32 s4, u32 *s3_out, u32 *s4_out, u32 *s5_out,
                  u32 *s6_out) {
    u32 *dl = (u32 *)dl_, *end = (u32 *)end_;
    TexCacheEntry *top = D_803B8D40;
    TextureEntry *table = D_803B8D44;
    TexCacheEntry *c = (TexCacheEntry *)s4;

    ENGINE_BLK(802A08E4);
    for (;;) {
        u32 id;

        ENGINE_BLK(802A095C);
        if (dl == end) {
            break;
        }
        ENGINE_BLK(802A0964);
        dl += 2;
        if ((dl[-2] & 0xFF000000) >> 24 != 0xFD) {     /* G_SETTIMG */
            continue;
        }
        ENGINE_BLK(802A0980);
        id = dl[-1];
        for (c = D_803B8570;; c++) {
            ENGINE_BLK(802A098C);
            if (c == top) {
                /* not loaded: load it at the heap's top */
                TextureEntry *e;
                u8 *heap;
                u32 phys;

                ENGINE_BLK(802A09A8);
                c->id = id;
                osInvalDCache(D_80358070, 0x1000);
                ENGINE_BLK(802A09D0);
                e = &table[id];
                heap = D_80358070;
                D_803C4B58.length = e->length;
                D_803C4B58.type = e->type;
                D_803C4B58.dst = (u32)heap;
                phys = PHYS(heap);
                dl[-1] = phys;
                c->phys = phys;
                top++;
                osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM + e->offset, heap,
                             e[1].offset - e->offset, &D_80315180);
                ENGINE_BLK(802A0A60);
                osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
                ENGINE_BLK(802A0A74);
                {
                    u32 size = func_802A57DC(&D_803C4B58);

                    ENGINE_BLK(802A0A80);
                    D_80358070 += size;
                }
                break;
            }
            ENGINE_BLK(802A0994);
            if (c->id == id) {
                break;
            }
            ENGINE_BLK(802A09A0);
        }
        ENGINE_BLK(802A0A94);
        dl[-1] = c->phys;
    }
    ENGINE_BLK(802A0AA0);
    D_803B8D40 = top;
    *s3_out = (u32)top;
    *s4_out = (u32)c;
    *s5_out = (u32)table;
    *s6_out = 0xFF000000;
    return (u32)&D_803B8D40;
}

/* func_802A08B4 (DE70.c's) */
void func_802A08B4(u32 *dl, u32 *end) {
    u32 s3, s4, s5, s6;

    ENGINE_BLK(802A08B4);
    func_802A08E4((u32)dl, (u32)end, 0, &s3, &s4, &s5, &s6);
    ENGINE_BLK(802A08D0);
}

/* the cache's entry `id`, from the table on the heap */
#define ENTRY(id) (&D_803B8D44[id])

/* func_802A0B34: load texture `id` at the heap's top with `param`, always */
REGS(t6, fp)
void func_802A0B34(u32 id, u32 param) {
    TextureEntry *e;
    u8 *heap;

    ENGINE_BLK(802A0B34);
    osInvalDCache(D_80358070, 0x1000);
    ENGINE_BLK(802A0BAC);
    e = ENTRY(id);
    heap = D_80358070;
    D_803C4B58.length = e->length;
    D_803C4B58.type = e->type;
    D_803C4B58.param = param;
    D_803C4B58.dst = (u32)heap;
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM + e->offset, heap,
                 e[1].offset - e->offset, &D_80315180);
    ENGINE_BLK(802A0C40);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(802A0C54);
    {
        u32 size = func_802A57DC(&D_803C4B58);

        ENGINE_BLK(802A0C60);
        D_80358070 += size;
    }
}

/* func_802A0B00 (2D810.c, 2E490.c, 39050.c).  (The original returns its
   caller's $s0, which none of them reads.) */
void func_802A0B00(u16 id, s32 param) {
    ENGINE_BLK(802A0B00);
    func_802A0B34(id, param);
    ENGINE_BLK(802A0B1C);
}

/* func_802A0CFC: texture `id`'s physical address, loaded at the heap's top
   with `param` if it isn't yet */
REGS(t6, fp -> s0)
u32 func_802A0CFC(u32 id, u32 param) {
    TexCacheEntry *top = D_803B8D40;
    TexCacheEntry *c;
    u32 phys;

    ENGINE_BLK(802A0CFC);
    for (c = D_803B8570;; c++) {
        ENGINE_BLK(802A0D60);
        if (c == top) {
            break;
        }
        ENGINE_BLK(802A0D68);
        if (c->id == id) {
            ENGINE_BLK(802A0E88);
            phys = c->phys;
            ENGINE_BLK(802A0E8C);
            return phys;
        }
        ENGINE_BLK(802A0D74);
    }
    ENGINE_BLK(802A0D7C);
    top->id = id;
    D_803B8D40 = top + 1;
    osInvalDCache(D_80358070, 0x1000);
    ENGINE_BLK(802A0DB0);
    {
        TextureEntry *e = ENTRY(id);
        u8 *heap = D_80358070;

        D_803C4B58.length = e->length;
        D_803C4B58.type = e->type;
        D_803C4B58.param = param;
        D_803C4B58.dst = (u32)heap;
        phys = PHYS(heap);
        top->phys = phys;
        osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM + e->offset, heap,
                     e[1].offset - e->offset, &D_80315180);
    }
    ENGINE_BLK(802A0E50);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(802A0E64);
    {
        u32 size = func_802A57DC(&D_803C4B58);

        ENGINE_BLK(802A0E70);
        D_80358070 += size;
    }
    ENGINE_BLK(802A0E8C);
    return phys;
}

/* func_802A0CC8 (the C's): the same */
u32 func_802A0CC8(s32 id, s32 param) {
    u32 phys;

    ENGINE_BLK(802A0CC8);
    phys = func_802A0CFC(id, param);
    ENGINE_BLK(802A0CE4);
    return phys;
}

/* func_802A0F0C: texture `id`'s raw bytes DMA'd to `dst` (not decoded) */
REGS(t6, s1)
void func_802A0F0C(u32 id, u32 dst) {
    TextureEntry *e;

    ENGINE_BLK(802A0F0C);
    osInvalDCache((void *)dst, 0x100);
    ENGINE_BLK(802A0F78);
    e = ENTRY(id);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM + e->offset, (void *)dst,
                 e[1].offset - e->offset, &D_80315180);
    ENGINE_BLK(802A0FCC);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(802A0FE0);
}

/* func_802A0EE0 (2E490.c's) */
void func_802A0EE0(u16 id, s32 dst) {
    ENGINE_BLK(802A0EE0);
    func_802A0F0C(id, dst);
    ENGINE_BLK(802A0EF8);
}

/* func_802A1074: start texture `id`'s DMA to `dst` without waiting and
   queue its decode with `param`; each DMA in flight has its own OSIoMesg */
REGS(t6, s1, fp)
void func_802A1074(u32 id, u32 dst, u32 param) {
    TextureEntry *e;
    s32 n;

    ENGINE_BLK(802A1074);
    osInvalDCache((void *)dst, 0x1000);
    ENGINE_BLK(802A10E0);
    n = D_80358080;
    D_80358080 = n + 1;
    e = ENTRY(id);
    osPiStartDma(&D_803B8D48[n], OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM + e->offset, (void *)dst,
                 e[1].offset - e->offset, &D_80315180);
    ENGINE_BLK(802A1158);
    func_802A5764(dst, e->length, e->type, param);
    ENGINE_BLK(802A1164);
}

/* func_802A1040 (168B0.c, 32E00.c, 43A60.c) */
void func_802A1040(u16 id, u8 *dst, s32 param) {
    ENGINE_BLK(802A1040);
    func_802A1074(id, (u32)dst, param);
    ENGINE_BLK(802A1060);
}

/* func_802A11C4: start texture `id`'s raw DMA to `dst` without waiting */
REGS(t6, s1)
void func_802A11C4(u32 id, u32 dst) {
    TextureEntry *e;
    s32 n;

    ENGINE_BLK(802A11C4);
    osInvalDCache((void *)dst, 0x100);
    ENGINE_BLK(802A1230);
    D_80358084++;
    n = D_80358080;
    D_80358080 = n + 1;
    e = ENTRY(id);
    osPiStartDma(&D_803B8D48[n], OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM + e->offset, (void *)dst,
                 e[1].offset - e->offset, &D_80315180);
    ENGINE_BLK(802A12BC);
}
