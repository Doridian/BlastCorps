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

#define N_LOADED 250            /* D_803B8570's entries */
#define N_DMAS 144              /* DMAs (and decodes) in flight at most */

extern TexCacheEntry D_803B8570[N_LOADED];
extern TexCacheEntry *D_803B8D40;           /* the list's end */
extern TextureEntry *D_803B8D44;            /* the table, on the heap */
extern OSIoMesg D_803B8D48[N_DMAS];         /* one per DMA in flight */
extern u8 D_803B9888;                       /* the table is in */
extern TexDecode D_803C4B58;                /* the decode of a texture loaded now */
extern TexDecode D_803C4250[N_DMAS];        /* the decode queue (60F60) */
extern OSMesgQueue D_80315180;
extern OSIoMesg D_80370C58;
extern s32 D_80358080;                      /* DMAs in flight */
extern s32 D_80358084;                      /* raw ones among them */
extern u8 D_00004CE0[];                     /* the texture table's ROM address */

#define TABLE_ROM ((u32)D_00004CE0)
#define TABLE_SIZE 0x8000                   /* the table's bytes */
#define PHYS(p) K0_TO_PHYS((u32)(uintptr_t)(p))
#define G_SETTIMG_OP 0xFD                   /* F3D's G_SETTIMG */
#define INVAL_DECODED 0x1000    /* the bytes of cache invalidated for a
                                   texture to decode */
#define INVAL_RAW 0x100         /* ... and for a raw one */

/* the cache's entry `id`, from the table on the heap */
#define ENTRY(id) (&D_803B8D44[id])

REGS(s1, s2, s3, fp)
void func_802A5764(u8 *dst, u32 length, u32 type, u8 *param);

/* start the DMA of texture entry `e` to `dst`, with the message block `mb` */
static void tex_dma(OSIoMesg *mb, TextureEntry *e, void *dst) {
    osPiStartDma(mb, OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM + e->offset, dst,
                 e[1].offset - e->offset, &D_80315180);
}

/* texture `id` (entry `e`) DMA'd to `dst` and decoded there by
   D_803C4B58, which the caller has set up: returns its decoded size.
   (802A08E4's code: 802A0B34 and 802A0CFC are the same.) */
static u32 tex_load_now(u32 id, TextureEntry *e, u8 *dst) {
    u32 size;

    tex_dma(&D_80370C58, e, dst);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    size = func_802A57DC(&D_803C4B58);
    host_tex_decoded(id, (u32)(uintptr_t)D_803C4B58.dst, size);
    return size;
}

/* func_802A0700 (the C's and the level loader's): bring the table in,
   once */
REGS()
void func_802A0700(void) {
    if (D_803B9888 == 0) {
        u8 *table = D_80358070;

        D_803B8D44 = (TextureEntry *)table;
        D_80358070 = table + TABLE_SIZE;
        osInvalDCache(table, TABLE_SIZE);
        osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, TABLE_ROM, table, TABLE_SIZE,
                     &D_80315180);
        osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
        D_803B8D40 = D_803B8570;
        D_803B9888 = 1;
    }
}

/* func_802A08E4: every G_SETTIMG in [dl, end) names a texture by number:
   load the ones not loaded yet and put their addresses in.  (A texture
   loaded here keeps the last decode's param.) */
static void tex_fix_dl(u32 *dl, u32 *end) {
    TexCacheEntry *top = D_803B8D40;
    TextureEntry *table = D_803B8D44;
    TexCacheEntry *c;

    for (; dl != end; dl += 2) {
        u32 id;

        if ((dl[0] & 0xFF000000) >> 24 != G_SETTIMG_OP) {
            continue;
        }
        id = dl[1];
        for (c = D_803B8570; c != top; c++) {
            if (c->id == id) {
                break;
            }
        }
        if (c == top) {
            /* not loaded: load it at the heap's top */
            TextureEntry *e;
            u8 *heap;
            u32 phys, size;

            c->id = id;
            osInvalDCache(D_80358070, INVAL_DECODED);
            e = &table[id];
            heap = D_80358070;
            D_803C4B58.length = e->length;
            D_803C4B58.type = e->type;
            D_803C4B58.dst = heap;
            phys = PHYS(heap);
            dl[1] = phys;
            c->phys = phys;
            top++;
            size = tex_load_now(id, e, heap);
            D_80358070 += size;
        }
        dl[1] = c->phys;
    }
    D_803B8D40 = top;
}

REGS(s0, s1)
void func_802A08E4(u32 *dl, u32 *end) {
    tex_fix_dl(dl, end);
}

/* func_802A08B4 (DE70.c's) */
void func_802A08B4(u32 *dl, u32 *end) {
    tex_fix_dl(dl, end);
}

/* func_802A0B34: load texture `id` at the heap's top with `param`, always */
REGS(t6, fp)
void func_802A0B34(u32 id, u8 *param) {
    TextureEntry *e;
    u8 *heap;
    u32 size;

    osInvalDCache(D_80358070, INVAL_DECODED);
    e = ENTRY(id);
    heap = D_80358070;
    D_803C4B58.length = e->length;
    D_803C4B58.type = e->type;
    D_803C4B58.param = param;
    D_803C4B58.dst = heap;
    size = tex_load_now(id, e, heap);
    D_80358070 += size;
}

/* func_802A0B00 (2D810.c, 2E490.c, 39050.c).  (The original returns its
   caller's $s0, which none of them reads.) */
void func_802A0B00(u16 id, u8 *param) {
    func_802A0B34(id, param);
}

/* func_802A0CFC: texture `id`'s physical address, loaded at the heap's top
   with `param` if it isn't yet */
REGS(t6, fp -> s0)
u32 func_802A0CFC(u32 id, u8 *param) {
    TexCacheEntry *top = D_803B8D40;
    TexCacheEntry *c;
    TextureEntry *e;
    u8 *heap;
    u32 phys, size;

    for (c = D_803B8570; c != top; c++) {
        if (c->id == id) {
            return c->phys;
        }
    }
    top->id = id;
    D_803B8D40 = top + 1;
    osInvalDCache(D_80358070, INVAL_DECODED);
    e = ENTRY(id);
    heap = D_80358070;
    D_803C4B58.length = e->length;
    D_803C4B58.type = e->type;
    D_803C4B58.param = param;
    D_803C4B58.dst = heap;
    phys = PHYS(heap);
    top->phys = phys;
    size = tex_load_now(id, e, heap);
    D_80358070 += size;
    return phys;
}

/* func_802A0CC8 (the C's): the same */
u8 *func_802A0CC8(s32 id, u8 *param) {
    u32 phys;

    phys = func_802A0CFC(id, param);
    return (u8 *)(uintptr_t)phys;       /* (a physical address: P8) */
}

/* func_802A0F0C: texture `id`'s raw bytes DMA'd to `dst` (not decoded) */
REGS(t6, s1)
void func_802A0F0C(u32 id, u8 *dst) {
    osInvalDCache(dst, INVAL_RAW);
    tex_dma(&D_80370C58, ENTRY(id), dst);
    osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
}

/* func_802A0EE0 (2E490.c's) */
void func_802A0EE0(u16 id, u8 *dst) {
    func_802A0F0C(id, dst);
}

/* func_802A1074: start texture `id`'s DMA to `dst` without waiting and
   queue its decode with `param`; each DMA in flight has its own OSIoMesg */
REGS(t6, s1, fp)
void func_802A1074(u32 id, u8 *dst, u8 *param) {
    TextureEntry *e;

    osInvalDCache(dst, INVAL_DECODED);
    e = ENTRY(id);
    tex_dma(&D_803B8D48[D_80358080++], e, dst);
    host_tex_queued(D_803C4B50 - D_803C4250, id);
    func_802A5764(dst, e->length, e->type, param);
}

/* func_802A1040 (168B0.c, 32E00.c, 43A60.c) */
void func_802A1040(u16 id, u8 *dst, u8 *param) {
    func_802A1074(id, dst, param);
}

/* func_802A11C4: start texture `id`'s raw DMA to `dst` without waiting */
REGS(t6, s1)
void func_802A11C4(u32 id, u8 *dst) {
    osInvalDCache(dst, INVAL_RAW);
    D_80358084++;
    tex_dma(&D_803B8D48[D_80358080++], ENTRY(id), dst);
}
