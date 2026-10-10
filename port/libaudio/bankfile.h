/*
 * A bank file (.ctl) as the ROM has it, for alBnkfNew (core.c): its
 * records' layouts and the one reader of their fields.
 *
 * The records are the file's own bank list, the banks, their instruments,
 * the instruments' sounds and the sounds' wave tables.  Their fields are
 * big-endian, and their references are 4-byte offsets from the file's
 * start (a wave table's base: from the sample file's).  The loader leaves
 * the records as the ROM has them in every build (port/host/native.c), so
 * that this reader, which reads them a byte at a time, gives the same on
 * a host of either byte order.  The leaves (ADPCM books, loops, envelopes,
 * key maps) have no offsets: the loader puts their scalars in the order
 * the library reads them in, which is how the parsed objects and the RSP's
 * commands use them, where they are in the file.
 *
 * Every offset is checked against the file before it is followed: a
 * resource pack can replace a bank, and a bad one stops the port with a
 * message instead of reading wild memory.
 */
#ifndef BANKFILE_H
#define BANKFILE_H

/* the file: s16 revision, s16 bankCount, then bankCount offsets of banks */
#define BF_FILE_REVISION 0x0
#define BF_FILE_COUNT 0x2
#define BF_FILE_BANKS 0x4

/* a bank: s16 instCount, u8 flags, u8 pad, s32 sampleRate, the offset of
   its percussion (0: none), then instCount offsets of instruments */
#define BF_BANK_COUNT 0x0
#define BF_BANK_FLAGS 0x2
#define BF_BANK_PAD 0x3
#define BF_BANK_RATE 0x4
#define BF_BANK_PERCUSSION 0x8
#define BF_BANK_INSTS 0xC

/* an instrument: u8 volume, pan, priority, flags, the tremolo's type,
   rate, depth and delay, the vibrato's, s16 bendRange, s16 soundCount,
   then soundCount offsets of sounds */
#define BF_INST_VOLUME 0x0
#define BF_INST_PAN 0x1
#define BF_INST_PRIORITY 0x2
#define BF_INST_FLAGS 0x3
#define BF_INST_TREM 0x4
#define BF_INST_VIB 0x8
#define BF_INST_BEND 0xC
#define BF_INST_COUNT 0xE
#define BF_INST_SOUNDS 0x10

/* a sound: the offsets of its envelope, key map and wave table, then u8
   samplePan, sampleVolume, flags */
#define BF_SOUND_ENVELOPE 0x0
#define BF_SOUND_KEYMAP 0x4
#define BF_SOUND_WAVETABLE 0x8
#define BF_SOUND_PAN 0xC
#define BF_SOUND_VOLUME 0xD
#define BF_SOUND_FLAGS 0xE
#define BF_SOUND_SIZE 0x10

/* a wave table: its base (an offset in the sample file), s32 len, u8 type,
   u8 flags, then the offsets of its loop (0: none) and, for ADPCM, its
   book */
#define BF_WAVE_BASE 0x0
#define BF_WAVE_LEN 0x4
#define BF_WAVE_TYPE 0x8
#define BF_WAVE_FLAGS 0x9
#define BF_WAVE_LOOP 0xC
#define BF_WAVE_BOOK 0x10
#define BF_WAVE_SIZE 0x14

/* the leaves' sizes in the file */
#define BF_ENVELOPE_SIZE 0x10       /* ALEnvelope: three s32, two u8 */
#define BF_KEYMAP_SIZE 0x6          /* ALKeyMap: six u8 */
#define BF_ADPCMLOOP_SIZE 0x2C      /* ALADPCMloop: three u32, sixteen s16 */
#define BF_RAWLOOP_SIZE 0xC         /* ALRawLoop: three u32 */
#define BF_BOOK_HEADER 0x8          /* ALADPCMBook: s32 order, npredictors, */
#define BF_BOOK_ENTRY 0x10          /* then order * npredictors of eight s16 */

typedef struct {
    const u8 *base;
    u32 len;
} BankFileIn;

static u8 bf_u8(const BankFileIn *f, u32 off) {
    return f->base[off];
}

static u16 bf_u16(const BankFileIn *f, u32 off) {
    const u8 *p = f->base + off;
    return (u16)(p[0] << 8 | p[1]);
}

static u32 bf_u32(const BankFileIn *f, u32 off) {
    const u8 *p = f->base + off;
    return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
}

#endif
