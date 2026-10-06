/*
 * Where untyped bytes from the ROM arrive in game memory (docs/PORT.md,
 * "Native-endian memory"): the two decompressors' entry points, wrapped at
 * link time (--wrap), hand what they produced to the host's loaders
 * (host/native.c), which convert it by type in the native-endian build and
 * tag it for the access-width profiler in either.  Raw PI DMAs go there
 * from host_rom_read.
 *
 *   func_8025C230(&src, &dst, heap)   gzip's unzip (hd_code 17A70.c): the
 *                                     models, the levels, the images; the
 *                                     member's own name is in its header
 *   func_8028B4C4(rom, dst, &len, ...) DMA and gzip or Rare's LZSS (method
 *                                     2: the sound banks, static_data, the
 *                                     background images), by ROM address
 *
 * While either runs, its loops' polls take no time without the CPU model
 * (host_loading; docs/PORT.md, "The front end's waits").
 */
#include "common.h"
#include "functions.h"
#include "port.h"

void __real_func_8025C230(u8 *PTR32 *src, u8 *PTR32 *dst, void *heap);
void __real_func_8028B4C4(u32 rom, u8 *dst, u32 *len, u8 bits, u8 bits2, u8 method);

void __wrap_func_8025C230(u8 *PTR32 *src, u8 *PTR32 *dst, void *heap) {
    u8 *s = *src, *d = *dst;

    host_loading(1);
    __real_func_8025C230(src, dst, heap);
    host_loading(0);
    host_loaded_gzip((u32)s, (u32)d, (u32)(*dst - d));
}

void __wrap_func_8028B4C4(u32 rom, u8 *dst, u32 *len, u8 bits, u8 bits2, u8 method) {
    host_loading(1);
    __real_func_8028B4C4(rom, dst, len, bits, bits2, method);
    host_loading(0);
    if (method == 2 && (bits != 0 || bits2 != 0))
        host_loaded_lzss(rom, (u32)dst, *len);
}

/*
 * The save.  The EEPROM file keeps the N64's bytes (host/video.c converts
 * each block as it goes in or out, host_eeprom_order), so a save is the
 * same file in either build; and the checksums the game keeps in it
 * (func_801F75A4: __osContDataCrc of every 32 bytes of the PlayerInfo, and
 * of a player's best times for a pak) are of those bytes.  In native-endian
 * memory the CRC is taken of the same 32 bytes in the N64's order, copied
 * without the game's bcopy: the big-endian build makes no copy, so this
 * one may not cost the game's CPU anything (eu seeds its random numbers
 * from the count soon after checking its save).
 */
extern u8 D_80364AF0[];         /* PlayerInfo[4] (game/player.h) */
extern u8 D_80364EF0[];         /* u16 [4][16]: the best times */
u8 __real___osContDataCrc(u8 *data);

u8 __wrap___osContDataCrc(u8 *data) {
#ifdef PORT_NATIVE_ENDIAN
    u8 be[32];
    u32 a = (u32)data;

    __builtin_memcpy(be, data, 32);
    if (a - (u32)D_80364AF0 < 4 * 0x100)
        host_save_order(be, (a - (u32)D_80364AF0) & 0xFF, 32, 0);
    else if (a - (u32)D_80364EF0 < 4 * 0x20)
        host_save_order(be, 0x100, 32, 0);      /* u16s */
    return __real___osContDataCrc(be);
#else
    return __real___osContDataCrc(data);
#endif
}

/*
 * Textures the C defines as arrays of u16 or Vtx (a decompilation's guess
 * at data it only hands the RDP): the RDP reads them as bytes in the N64's
 * order, and nothing reads them otherwise, so in native-endian memory they
 * are put back into that order once, at boot (src/boot.c).
 */
#ifdef PORT_NATIVE_ENDIAN
extern u16 D_802E9FF0[0x280];   /* 20460.c: RGBA16 20x32 */
extern u16 D_802FCEB0[0x400];   /* 3E4C0.c: RGBA16 32x32 */
extern Vtx D_802F9A00[0x40];    /* 2B3F0.c: RGBA32 16x16 */
extern Vtx D_802F9E00[0x40];
extern u16 D_802FC5B0[0x80];    /* 3E4C0.c: IA16 */
extern u16 D_802FC6B0[0x400];   /* 3E4C0.c: IA16 */
/* hd_front_end 9570.c: RGBA16 32x32 with its mipmaps, the carrier on the
   world map's globe (put back before overlay.c keeps the front end's .data
   for its reloads) */
extern u16 D_802084F0[0x55c];
extern u16 D_80209028[0x55c];
extern u16 D_80209B60[0x55c];
extern u16 D_8020A698[0x55c];
extern u16 D_8020B1D0[0x55c];

/* And the other way round: YoshiIcon.unk6 (game/yoshi.h) is declared u8
   [0x14] but only ever read as u16 frames (func_80272C5C's first argument,
   from 26570.c, 53220.c, 1D990.c and hd_front_end 7800.c): in native
   memory its bytes become the ten u16s they are. */
#include "game/yoshi.h"

/* (no loops here: BEPass's polls would make the boot take longer than the
   big-endian build's, and the two couldn't be compared run for run) */
void port_native_fixups(void) {
    host_layout_to_be_n((u32)D_802F49F4[0].unk6, sizeof D_802F49F4[0], 0x4B, sizeof D_802F49F4[0].unk6, "h");
    host_layout_to_be((u32)D_802E9FF0, sizeof D_802E9FF0, "h", 1);
    host_layout_to_be((u32)D_802FCEB0, sizeof D_802FCEB0, "h", 1);
    host_layout_to_be((u32)D_802F9A00, sizeof D_802F9A00, "hhhhhhbbbb", 1);
    host_layout_to_be((u32)D_802F9E00, sizeof D_802F9E00, "hhhhhhbbbb", 1);
    host_layout_to_be((u32)D_802FC5B0, sizeof D_802FC5B0, "h", 1);
    host_layout_to_be((u32)D_802FC6B0, sizeof D_802FC6B0, "h", 1);
    host_layout_to_be((u32)D_802084F0, sizeof D_802084F0, "h", 1);
    host_layout_to_be((u32)D_80209028, sizeof D_80209028, "h", 1);
    host_layout_to_be((u32)D_80209B60, sizeof D_80209B60, "h", 1);
    host_layout_to_be((u32)D_8020A698, sizeof D_8020A698, "h", 1);
    host_layout_to_be((u32)D_8020B1D0, sizeof D_8020B1D0, "h", 1);
}
#endif
