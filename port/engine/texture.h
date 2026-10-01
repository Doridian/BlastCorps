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
