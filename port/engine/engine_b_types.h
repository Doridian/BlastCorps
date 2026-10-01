/* The engine's memory that engine-B's modules share: types and the
   variables (the handwritten objects' .bss, and the C's that they use). */
#ifndef ENGINE_B_TYPES_H
#define ENGINE_B_TYPES_H

#include "common.h"
#include "game/game.h"

/* an N64 address as a word, and the physical address of a KSEG0 one */
#define ENG_ADDR(p) ((u32)(__UINTPTR_TYPE__)(p))
#define ENG_PHYS(p) (ENG_ADDR(p) - 0x80000000)

/* ROM segments (the linker's symbols) */
extern u8 D_00004CE0[];             /* texture_table */
#define ROM_TEXTURE_TABLE ENG_ADDR(D_00004CE0)

/* ---- textures (5BF40, 60F60) ------------------------------------------- */

/* the ROM's texture table (docs/ASSETS.md) */
typedef struct TextureEntry {
    /* 0x0 */ u32 offset;   /* from the table's start; the next entry's ends it */
    /* 0x4 */ u16 length;
    /* 0x6 */ u16 type;
} TextureEntry;

/* what is loaded: the id and its physical address */
typedef struct TexCacheEntry {
    u32 id;
    u32 phys;
} TexCacheEntry;

/* a decoding request (func_802A57DC) */
typedef struct TexDecode {
    /* 0x0 */ u32 dst;
    /* 0x4 */ u32 length;
    /* 0x8 */ u32 type;
    /* 0xC */ u32 param;
} TexDecode;

extern TexCacheEntry D_803B8570[250];
extern TexCacheEntry *PTR32 D_803B8D40;     /* the cache's end */
extern TextureEntry *PTR32 D_803B8D44;      /* the table, in the heap */
extern OSIoMesg D_803B8D48[144];            /* one per DMA in flight */
extern u8 D_803B9888;                       /* the table is in */
extern TexDecode D_803C4B58;
extern TexDecode *PTR32 D_803C4B50;         /* the decode queue's end */

extern OSMesgQueue D_80315180;
extern OSIoMesg D_80370C58;
extern s32 D_80358080;                      /* DMAs in flight */
extern s32 D_80358084;                      /* raw ones among them */

u32 func_802A57DC(TexDecode *req);
void eng_tex_queue_decode(u32 dst, u32 length, u32 type, u32 param);

/* ---- collision (5CB60's func_802A41B0, 8A080, 89250) -------------------- */

#include "game/objects.h"
#include "game/level.h"

/* a run-time collision triangle, from a LevelCollisionTri */
typedef struct CollisionTri {
    /* 0x00 */ s64 nx, ny, nz;      /* the edges' cross product */
    /* 0x18 */ s64 d;               /* -(n . v0) */
    /* 0x20 */ f32 nlen;            /* |n| */
    /* 0x24 */ f32 nlen2;           /* |n|^2 */
    /* 0x28 */ s32 v[3][3];         /* the vertices, << 3 */
    /* 0x4C */ u16 unk4C;           /* LevelCollisionTri.unk12 */
    /* 0x4E */ u8 axis;             /* the normal's largest component: 0 x, 1 y, 2 z */
    /* 0x4F */ u8 unk4F;
    /* 0x50 */ u8 unk50;
    /* 0x51 */ u8 unk51;
    /* 0x52 */ u16 unk52;
    /* 0x54 */ u8 unk54;
    /* 0x55 */ u8 unk55;
    /* 0x56 */ u8 unk56;
    /* 0x57 */ u8 unk57;
    /* 0x58 */ u8 unk58;
    /* 0x59 */ u8 unk59;
    /* 0x5A */ u8 pad5A[6];
} CollisionTri;
SIZE_CHECK(CollisionTri, 0x60);

/* func_802A41B0: the original takes t4 = tri, t5 = src and the bytes in
   $t9, $v0, $t2, $gp, $t7, $t6, $s1 */
void eng_collision_tri_init(CollisionTri *t, const u8 *src, u8 b4f, u8 b50, u16 h52,
                            u8 b55, u8 b56, u8 b57, u8 b58);

#endif
