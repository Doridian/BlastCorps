/* hd_code 5BF40: the texture loader.

   The ROM's texture table (docs/ASSETS.md, "Texture table") is DMA'd once
   into the heap (D_803B8D44).  A texture is DMA'd from the entry's offset
   to the next entry's into the heap and decoded in place by its type
   (60F60's func_802A57DC).  D_803B8570 caches what is loaded: the id and
   the physical address it was loaded at, up to D_803B8D40.  Display
   lists name textures by id in their G_SETTIMG words; func_802A08E4
   rewrites those to the loaded addresses. */
#include "engine_b.h"
#include "engine_b_types.h"

/* ---- the entries ------------------------------------------------------- */

/* func_802A0700: bring the texture table in, once. */
void func_802A0700(void) {
    ENG_COST(36);
    if (D_803B9888 == 0) {
        u8 *table = D_80358070;

        ENG_COST(13);
        D_803B8D44 = (TextureEntry *)table;
        D_80358070 = table + 0x8000;
        osInvalDCache(table, 0x8000);
        ENG_COST(13);
        osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, (u32)ROM_TEXTURE_TABLE,
                     table, 0x8000, &D_80315180);
        ENG_COST(5);
        osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
        ENG_COST(8);
        D_803B8D40 = D_803B8570;
        D_803B9888 = 1;
    }
    ENG_COST(34);
}

/* func_802A08E4: every G_SETTIMG in [dl, end) names a texture by id;
   load the ones not loaded yet and put their addresses in.  (The
   original takes dl, end in $s0, $s1 and leaves $s0 = end.) */
void eng_tex_fix_dl(u32 *dl, u32 *end) {
    TexCacheEntry *top = D_803B8D40;

    ENG_COST(30);
    for (;;) {
        TexCacheEntry *c;
        u32 id;

        ENG_COST(2);
        if (dl == end) {
            break;
        }
        ENG_COST(7);
        dl += 2;
        if ((dl[-2] & 0xFF000000) >> 24 != 0xFD) {   /* G_SETTIMG */
            continue;
        }
        ENG_COST(3);
        id = dl[-1];
        for (c = D_803B8570;; c++) {
            ENG_COST(2);
            if (c == top) {
                /* not loaded: load it at the heap's top */
                TextureEntry *e;
                u8 *heap;
                u32 phys;

                ENG_COST(10);
                c->id = id;
                osInvalDCache(D_80358070, 0x1000);
                ENG_COST(36);
                e = &D_803B8D44[id];
                heap = D_80358070;
                D_803C4B58.length = e->length;
                D_803C4B58.type = e->type;
                D_803C4B58.dst = ENG_ADDR(heap);
                phys = ENG_PHYS(heap);
                dl[-1] = phys;
                c->phys = phys;
                top++;
                osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ,
                             (u32)ROM_TEXTURE_TABLE + e->offset, heap,
                             e[1].offset - e->offset, &D_80315180);
                ENG_COST(5);
                osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
                ENG_COST(3);
                {
                    u32 size = func_802A57DC(&D_803C4B58);

                    ENG_COST(5);
                    D_80358070 += size;
                }
                break;
            }
            ENG_COST(3);
            if (c->id == id) {
                break;
            }
            ENG_COST(2);
        }
        ENG_COST(3);
        dl[-1] = c->phys;
    }
    ENG_COST(24);
    D_803B8D40 = top;
}

/* func_802A08B4: the C's entry to it. */
void func_802A08B4(u32 *dl, u32 *end) {
    ENG_COST(7);
    eng_tex_fix_dl(dl, end);
    ENG_COST(5);
}

/* func_802A0B34: load texture `id` at the heap's top with `param`,
   always (no cache). */
static void tex_load_uncached(s32 id, u32 param) {
    TextureEntry *e;
    u8 *heap;

    ENG_COST(30);
    osInvalDCache(D_80358070, 0x1000);
    ENG_COST(37);
    e = &D_803B8D44[id];
    heap = D_80358070;
    D_803C4B58.length = e->length;
    D_803C4B58.type = e->type;
    D_803C4B58.param = param;
    D_803C4B58.dst = ENG_ADDR(heap);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ,
                 (u32)ROM_TEXTURE_TABLE + e->offset, heap,
                 e[1].offset - e->offset, &D_80315180);
    ENG_COST(5);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    ENG_COST(3);
    {
        u32 size = func_802A57DC(&D_803C4B58);

        ENG_COST(26);
        D_80358070 += size;
    }
}

/* func_802A0B00 (2D810.c, 2E490.c, 39050.c).  The original returns its
   caller's $s0, which no caller reads. */
void func_802A0B00(u16 id, s32 param) {
    ENG_COST(7);
    tex_load_uncached(id, param);
    ENG_COST(6);
}

/* func_802A0CFC: texture `id`, from the cache or loaded at the heap's top
   with `param`; returns its physical address. */
u32 eng_tex_get(s32 id, u32 param) {
    TexCacheEntry *top = D_803B8D40;
    TexCacheEntry *c;
    u32 phys;

    ENG_COST(25);
    for (c = D_803B8570;; c++) {
        ENG_COST(2);
        if (c == top) {
            break;
        }
        ENG_COST(3);
        if (c->id == (u32)id) {
            ENG_COST(1);
            phys = c->phys;
            ENG_COST(21);
            return phys;
        }
        ENG_COST(2);
    }
    ENG_COST(13);
    top->id = id;
    D_803B8D40 = top + 1;
    osInvalDCache(D_80358070, 0x1000);
    ENG_COST(40);
    {
        TextureEntry *e = &D_803B8D44[id];
        u8 *heap = D_80358070;

        D_803C4B58.length = e->length;
        D_803C4B58.type = e->type;
        D_803C4B58.param = param;
        D_803C4B58.dst = ENG_ADDR(heap);
        phys = ENG_PHYS(heap);
        top->phys = phys;
        osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ,
                     (u32)ROM_TEXTURE_TABLE + e->offset, heap,
                     e[1].offset - e->offset, &D_80315180);
    }
    ENG_COST(5);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    ENG_COST(3);
    {
        u32 size = func_802A57DC(&D_803C4B58);

        ENG_COST(6);
        D_80358070 += size;
    }
    ENG_COST(21);
    return phys;
}

/* func_802A0CC8 (the C's): the same. */
u32 func_802A0CC8(s32 id, s32 param) {
    u32 phys;

    ENG_COST(7);
    phys = eng_tex_get(id, param);
    ENG_COST(6);
    return phys;
}

/* func_802A0F0C: DMA texture `id`'s raw bytes to `dst` and wait (no
   decoding). */
static void tex_load_raw(s32 id, void *dst) {
    TextureEntry *e;

    ENG_COST(27);
    osInvalDCache(dst, 0x100);
    ENG_COST(21);
    e = &D_803B8D44[id];
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ,
                 (u32)ROM_TEXTURE_TABLE + e->offset, dst,
                 e[1].offset - e->offset, &D_80315180);
    ENG_COST(5);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    ENG_COST(24);
}

/* func_802A0EE0 (2E490.c) */
void func_802A0EE0(u16 id, s32 dst) {
    ENG_COST(6);
    tex_load_raw(id, (void *)dst);
    ENG_COST(5);
}

/* func_802A1074: start texture `id`'s DMA to `dst` without waiting, and
   queue its decoding (60F60's func_802A5764) with `param`.  D_80358080
   counts the DMAs in flight; each has its own OSIoMesg in D_803B8D48. */
void eng_tex_load_async(s32 id, void *dst, u32 param) {
    TextureEntry *e;
    u32 n;

    ENG_COST(27);
    osInvalDCache(dst, 0x1000);
    ENG_COST(30);
    n = D_80358080;
    D_80358080 = n + 1;
    e = &D_803B8D44[id];
    osPiStartDma(&D_803B8D48[n], OS_MESG_PRI_NORMAL, OS_READ,
                 (u32)ROM_TEXTURE_TABLE + e->offset, dst,
                 e[1].offset - e->offset, &D_80315180);
    ENG_COST(3);
    eng_tex_queue_decode((u32)dst, e->length, e->type, param);
    ENG_COST(24);
}

/* func_802A1040 (168B0.c, 32E00.c, 43A60.c) */
void func_802A1040(u16 id, u8 *dst, s32 param) {
    ENG_COST(8);
    eng_tex_load_async(id, dst, param);
    ENG_COST(5);
}

/* func_802A11C4: start texture `id`'s raw DMA to `dst` without waiting;
   D_80358084 counts these, D_80358080 all DMAs in flight. */
void eng_tex_load_raw_async(s32 id, void *dst) {
    TextureEntry *e;
    u32 n;

    ENG_COST(27);
    osInvalDCache(dst, 0x100);
    ENG_COST(35);
    D_80358084++;
    n = D_80358080;
    D_80358080 = n + 1;
    e = &D_803B8D44[id];
    osPiStartDma(&D_803B8D48[n], OS_MESG_PRI_NORMAL, OS_READ,
                 (u32)ROM_TEXTURE_TABLE + e->offset, dst,
                 e[1].offset - e->offset, &D_80315180);
    ENG_COST(24);
}
