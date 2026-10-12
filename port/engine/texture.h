/*
 * The texture loader's (hd_code 5BF40) and decoder's (60F60) shared types.
 */
#ifndef ENGINE_TEXTURE_H
#define ENGINE_TEXTURE_H

#include "engine.h"

/* the ROM's texture table (docs/ASSETS.md, "Texture table") */
typedef struct TextureEntry {
    /* 0x0 */ u32 offset;   /* from the table's start; the next entry's ends it */
    /* 0x4 */ u16 length;   /* what the decoder gets (the packed stream's bytes) */
    /* 0x6 */ u16 type;     /* the decoder: 0 raw ... 6 */
} TextureEntry;

/* a loaded texture: its number and its address as a G_SETTIMG has it
   (DL_ADDR) */
typedef struct TexCacheEntry {
    u32 id;
    uintptr_t phys;
} TexCacheEntry;

/* a decode (func_802A57DC): in place at dst */
typedef struct TexDecode {
    /* 0x0 */ u8 *dst;
    /* 0x4 */ u32 length;
    /* 0x8 */ u32 type;
    /* 0xC */ u8 *param;    /* the palette's address, for types 4 and 5 */
} TexDecode;

u32 func_802A57DC(TexDecode *req);
uintptr_t tex_phys(u32 id);     /* a loaded texture's address, as a G_SETTIMG has it (5BF40) */
/* 5BF40's: texture `id` loaded if it isn't, its address as tex_phys's */
REGS(t6, fp -> s0)
uintptr_t func_802A0CFC(u32 id, u8 *param);
/* and the textures [dl, end) name by number loaded, their addresses put in */
REGS(s0, s1)
void func_802A08E4(Gfx *dl, Gfx *end);
extern TexDecode *D_803C4B50;   /* 60F60's queue of decodes: where the next goes */

/* A resource pack's edited textures (port/host/pack.c, docs/PORT.md,
   "Resource packs"): after the game decodes texture `id` to `dst`, the host
   puts the pack's texels over it.  The game's work is the same as with the
   ROM's (the calls cost nothing: the host's); only what is drawn changes.
   A queued decode (func_802A1074) names its texture by its queue slot. */
void host_tex_decoded(u32 id, void *dst, u32 size);
void host_tex_queued(u32 slot, u32 id);
void host_tex_decoded_slot(u32 slot, void *dst, u32 size);

/* Texture data stays in the N64's byte order, which is what the RDP reads
   (docs/PORT.md, "Byte order and the data's types"): the packed stream's codes, the
   palettes and the texels written as halves or words are big-endian
   (tools/recomp/native_sites.txt's `be`), read and written a byte at a
   time (engine.h's BE16U, be_put16, be_put32), whatever the host's order. */

#endif
