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

/* a loaded texture: its number and physical address */
typedef struct TexCacheEntry {
    u32 id;
    u32 phys;
} TexCacheEntry;

/* a decode (func_802A57DC): in place at dst */
typedef struct TexDecode {
    /* 0x0 */ u32 dst;
    /* 0x4 */ u32 length;
    /* 0x8 */ u32 type;
    /* 0xC */ u32 param;    /* the palette, for types 4 and 5 */
} TexDecode;

u32 func_802A57DC(TexDecode *req);
extern TexDecode *D_803C4B50;   /* 60F60's queue of decodes: where the next goes */

/* A resource pack's edited textures (port/host/pack.c, docs/PORT.md,
   "Resource packs"): after the game decodes texture `id` to `dst`, the host
   puts the pack's texels over it.  The game's work is the same as with the
   ROM's (the calls cost nothing: the host's); only what is drawn changes.
   A queued decode (func_802A1074) names its texture by its queue slot. */
void host_tex_decoded(u32 id, u32 dst, u32 size);
void host_tex_queued(u32 slot, u32 id);
void host_tex_decoded_slot(u32 slot, u32 dst, u32 size);

/* In native-endian memory (docs/PORT.md, "Native-endian memory") texture
   data stays in the N64's byte order, which is what the RDP reads: the
   packed stream's codes, the palettes and the texels written as halves or
   words are big-endian there (tools/recomp/native_sites.txt's `be`). */
#ifdef PORT_NATIVE_ENDIAN
#define TEX_BE16(x) ((u16)__builtin_bswap16((u16)(x)))
#define TEX_BE32(x) ((u32)__builtin_bswap32((u32)(x)))
#else
#define TEX_BE16(x) ((u16)(x))
#define TEX_BE32(x) ((u32)(x))
#endif

#endif
