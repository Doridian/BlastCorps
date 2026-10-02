/* What each of the ROM's segments holds, as the loaders convert it
   (native.c) and the resource packs rebuild it (pack.c); romtab.h,
   generated from the link map by port/tools/gen_romtab.py, lists the
   segments with theirs. */
#ifndef PORT_ROMCLASS_H
#define PORT_ROMCLASS_H

enum {
    ROM_BYTES,              /* texels, samples, code, images: as they are */
    ROM_DL,                 /* a display list file: Gfx */
    ROM_VEHICLE,            /* a vehicle's (or a logo's) model file: VehicleModel */
    ROM_MODEL,              /* a model table model: Model */
    ROM_LEVEL,              /* a level file: LevelHeader */
    ROM_TEXTURE_TABLE,      /* 4096 x {u32, u16, u16} */
    ROM_MODEL_TABLE,        /* 512 x u32 */
    ROM_BANK,               /* libaudio ALBankFile (LZSS) */
    ROM_SEQUENCES,          /* libaudio ALSeqFile, then the sequences */
    ROM_STATIC,             /* static_data: a Vp, then a display list (LZSS) */
    ROM_ATTRACT,            /* the attract mode's recordings */
};

#endif
