/*
 * The loaders: what the ROM's untyped bytes become in game memory
 * (docs/PORT.md, "Native-endian memory").  src/loads.c (the two
 * decompressors, wrapped at link time) and host_rom_read (PI DMA) call in
 * here with every load.  In the native-endian build (PORT_NATIVE_ENDIAN)
 * each load is converted by what it is, once, where it arrives: its
 * scalars go into host order at their own widths, its bytes (texels,
 * samples, strings, flags) stay as they are.  In either build the
 * access-width profiler (PORT_ACCESS_PROFILE, PORT_ACCESS_ORIGIN) is told
 * where the bytes came from and at what width they were converted, which
 * is how the layouts below were found and are checked.
 *
 * What a load is comes from its gzip member's own name (Rare's headers
 * carry it: "chimp_dl.raw") or, for LZSS and raw DMA, from its ROM
 * address; either way from the ROM's segment table (romtab.h, generated
 * from the link map by tools/gen_romtab.py) and its class.
 *
 * The layouts are the handwritten engine's and libaudio's, as their code
 * reads them; the evidence for each is at its converter.
 */
#include <stdio.h>
#include <string.h>

#include "host.h"

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

#include "romtab.h"

#ifdef PORT_ACCESS_PROFILE
int port_access_asset(const char *name);
void port_access_origin(uint32_t dst, uint32_t len, int asset, uint32_t off);
void port_access_conv(uint32_t dst, uint32_t len, unsigned width);
#endif

#define NSEG (sizeof rom_segments / sizeof rom_segments[0])

static const struct rom_segment *rom_segment(uint32_t rom) {
    size_t lo = 0, hi = NSEG;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (rom_segments[mid].start <= rom)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0 || rom >= rom_segments[lo - 1].end)
        return NULL;
    return &rom_segments[lo - 1];
}

static const struct rom_segment *segment_named(const char *name) {
    for (size_t i = 0; i < NSEG; i++)
        if (!strcmp(rom_segments[i].name, name))
            return &rom_segments[i];
    return NULL;
}

/* ---- the conversion ------------------------------------------------------- */

/* the load being converted: [base, base + len) of game memory */
static uint8_t *base;
static uint32_t base_addr, len;
static const char *load_name;
static int nwarn;

static void warn(const char *fmt, uint32_t a, uint32_t b) {
    if (nwarn++ < 50) {
        char msg[160];
        snprintf(msg, sizeof msg, fmt, a, b);
        host_log("native: %s: %s\n", load_name, msg);
    }
}

/* n bytes at offset off, in units of w bytes (8: two words, the high one
   first, as every 64-bit scalar is kept), into host order */
static void cv(uint32_t off, uint32_t n, unsigned w) {
    if (off > len || n > len - off) {
        warn("conversion past the end (+0x%X, 0x%X bytes)", off, n);
        if (off > len)
            return;
        n = len - off;
    }
#ifdef PORT_NATIVE_ENDIAN
    uint8_t *p = base + off;
    unsigned u = w > 4 ? 4 : w;
    if (u > 1)
        for (uint32_t k = 0; k + u <= n; k += u)
            for (unsigned a = 0, b = u - 1; a < b; a++, b--) {
                uint8_t t = p[k + a];
                p[k + a] = p[k + b];
                p[k + b] = t;
            }
#endif
#ifdef PORT_ACCESS_PROFILE
    port_access_conv(base_addr + off, n, w > 4 ? 4 : w);
#endif
}

/* big-endian reads of the data as it arrived (before cv) */
static uint32_t rd32(uint32_t off) {
    if (off + 4 > len)
        return 0;
    const uint8_t *p = base + off;
    return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}
static uint16_t rd16(uint32_t off) {
    if (off + 2 > len)
        return 0;
    return (uint16_t)(base[off] << 8 | base[off + 1]);
}

/* one record by a field string: h a 16-bit field, w a 32-bit one, b a
   byte, d a 64-bit one; returns its size */
static uint32_t rec(uint32_t off, const char *f) {
    uint32_t o = off;
    for (; *f; f++) {
        unsigned w = *f == 'h' ? 2 : *f == 'w' ? 4 : *f == 'd' ? 8 : 1;
        if (w > 1)
            cv(o, w, w);
        else
            cv(o, 1, 1);
        o += w;
    }
    return o - off;
}

/* records of one layout over [off, end) */
static void recs(uint32_t off, uint32_t end, const char *f) {
    uint32_t size = 0;
    for (const char *p = f; *p; p++)
        size += *p == 'h' ? 2 : *p == 'w' ? 4 : *p == 'd' ? 8 : 1;
    if (end > len)
        end = len;
    for (; off + size <= end; off += size)
        rec(off, f);
    if (off < end)
        cv(off, end - off, 1);
}

/* Vtx: s16 x, y, z, flag, s, t, then four bytes of colour or normal */
static void vtx(uint32_t off, uint32_t end) {
    recs(off, end, "hhhhhhbbbb");
}

/* an Mtx: sixteen words, each two s16 halves (the integer parts, then the
   fractions), kept as the words the C (guMtxF2L) and the asm build them */
static void mtx(uint32_t off, uint32_t end) {
    if (end > off)
        cv(off, (end - off) & ~3u, 4);
}

/* a display list file: Gfx words only (every _dl member is: each 8 bytes a
   valid F3D command, checked over all of them) */
static void conv_dl(void) {
    cv(0, len & ~3u, 4);
}

/* the sections of a file with a header of offsets: [start, next start) */
static __attribute__((unused)) uint32_t section_end(const uint32_t *offs, int n, uint32_t start, uint32_t file_end) {
    uint32_t e = file_end;
    for (int i = 0; i < n; i++)
        if (offs[i] > start && offs[i] < e)
            e = offs[i];
    return e;
}

/*
 * VehicleModel (game/vehicle.h), a vehicle's or a logo's model file; its
 * display list is a separate member.  19 offsets from the file's start:
 *   0x00  parts: {s16 x, y, z; u16 n; u32 mtx[n]}      func_802ABBEC
 *   0x04  u16, u16; then {s16 x, y, z; u16, u16; u16 n; u32 mtx[n]}
 *                                                     func_8029C354/C454
 *   0x08, 0x0C  {u16 ntris, nmtx; u32 mtx[nmtx]; triangles}  func_802AABE4
 *   0x10  the animations: u16 offset[32], then keyframes  func_8029E5AC
 *   0x14  the vertices (segment 6)                    the RSP
 *   0x18  Mtx[]: copied by words, read as halves      func_802AA890
 *   0x1C..0x48  into the display list, after the file
 */
static void conv_vehicle(void) {
    uint32_t offs[19];
    for (int i = 0; i < 19; i++) {
        offs[i] = rd32(4 * i);
        cv(4 * i, 4, 4);
    }
    /* the sections are in field order: each runs to the next field's */
#define VSEC(f) o = offs[f], e = offs[(f) + 1] < len ? offs[(f) + 1] : len
    uint32_t o, e;
    /* 0x00: parts */
    VSEC(0);
    while (o + 8 <= e) {
        uint32_t n = rd16(o + 6);
        rec(o, "hhhh");
        o += 8;
        if (o + 4 * n > e) {
            warn("parts: record past its section at +0x%X", o, 0);
            break;
        }
        cv(o, 4 * n, 4);
        o += 4 * n;
    }
    /* 0x04 */
    VSEC(1);
    if (o + 4 <= e) {
        rec(o, "hh");
        o += 4;
        while (o + 12 <= e) {
            uint32_t n = rd16(o + 10);
            rec(o, "hhhhhh");
            o += 12;
            if (o + 4 * n > e) {
                warn("0x04: record past its section at +0x%X", o, 0);
                break;
            }
            cv(o, 4 * n, 4);
            o += 4 * n;
        }
    }
    /* 0x08, 0x0C (func_802AABE4, func_8029D24C): {u16 ntris, nmtx;
       u32 mtx[nmtx]; ntris x {s16 x 9; u8, u8}} */
    for (int f = 2; f <= 3; f++) {
        VSEC(f);
        if (o + 4 > e)
            continue;
        uint32_t nt = rd16(o), nm = rd16(o + 2);
        rec(o, "hh");
        o += 4;
        if (o + 4 * nm > e)
            continue;
        cv(o, 4 * nm, 4);
        o += 4 * nm;
        for (uint32_t i = 0; i < nt && o + 20 <= e; i++, o += 20)
            rec(o, "hhhhhhhhhbb");
    }
    /* 0x10: the animations (func_8029E5AC): u16 offset[32] (from here),
       then at each {u8 n; u8 [n]; u8; u8 count; to 4-byte alignment:
       count x {u32, u32 (an Mtx in 0x18), n x s16 [10] (keyframes)}} */
    VSEC(4);
    if (o + 64 <= e) {
        uint32_t s0 = o;
        uint16_t tab[32];
        for (int i = 0; i < 32; i++)
            tab[i] = rd16(s0 + 2 * i);
        cv(s0, 64, 2);
        for (int i = 0; i < 32; i++) {
            int seen = 0;
            for (int j = 0; j < i; j++)
                seen |= tab[j] == tab[i];
            uint32_t a = s0 + tab[i];
            if (seen || a + 2 > e)
                continue;
            uint32_t n = base[a], count = a + n + 1 < len ? base[a + n + 1] : 0;
            uint32_t q = a + n + 2;
            q = ((base_addr + q + 3) & ~3u) - base_addr;       /* aligned in memory */
            for (uint32_t k = 0; k < count && q + 8 + 20 * n <= e; k++) {
                cv(q, 8, 4);
                cv(q + 8, 20 * n, 2);
                q += 8 + 20 * n;
            }
        }
    }
    /* 0x14: vertices */
    VSEC(5);
    vtx(o, e);
    /* 0x18: matrices */
    VSEC(6);
    mtx(o, e);
#undef VSEC
}

/*
 * Model (game/model.h), a model table model.  Its header, then its
 * sections, relocated in place by func_802A26A8 (which shows most of
 * their layouts):
 *   0x50..0x1C  Vtx (x, y, z moved when unkE is 0)
 *   0x1C  4 x {s16 x, y, z}
 *   0x20  s16 x 4 (x, z, x, z)
 *   0x24  {s16 x 9, u8, u8}: triangles
 *   0x28  animated textures {u32 texture; u8 n ...; u32 texture[n - 1]}
 *   0x2C  {s16 x, y, z, u8, u8}
 *   0x30, 0x34  0x38-byte records: s32 x, y, z (<< 16 then), ... s32 at 0x28
 *   0x38  {u32 n; n x {u16, u8, u8}; u32 [4]} (func_802BD1F8)
 *   0x3C  u16[]                                         func_802BD1F8
 *   0x40  s16[] (y values)
 *   0x48  0x19-byte records of unaligned s16 pairs: bytes
 *   0x10, 0x14, 0x18: into the display list
 */
static void conv_model(void) {
    uint32_t offs[16];
    for (int i = 0; i < 16; i++)
        offs[i] = rd32(0x10 + 4 * i);
    rec(0, "hhbbbbwbbh");                   /* 0x00..0x0F */
    cv(0x10, 0x40, 4);                      /* the offsets */
    uint32_t o, e;
    vtx(0x50, offs[3]);
    o = offs[3]; e = offs[4]; if (e > o) recs(o, e, "hhh");                 /* 0x1C..0x20 */
    o = offs[4]; e = offs[5]; if (e > o) recs(o, e, "h");                   /* 0x20..0x24 */
    o = offs[5]; e = offs[6]; if (e > o) recs(o, e, "hhhhhhhhhbb");         /* 0x24..0x28 */
    /* 0x28..0x2C: animated textures (func_802A2608): {u32 texture; u8 n,
       3 bytes; u32; u16, u16; u32 texture[n - 1]} */
    o = offs[6]; e = offs[7];
    while (o + 0x10 <= e) {
        uint32_t n = base[o + 4];
        rec(o, "wbbbbwhh");
        if (n > 1 && o + 0x10 + 4 * (n - 1) <= e)
            cv(o + 0x10, 4 * (n - 1), 4);
        o += 0x10 + 4 * (n ? n - 1 : 0);
    }
    o = offs[7]; e = offs[8]; if (e > o) recs(o, e, "hhhbb");               /* 0x2C..0x30 */
    /* 0x30..0x38: the effect records func_802BF978 copies into D_803F3968:
       s32 x 11, u16 0x2C and 0x2E (func_802C0574: lhu, sh), then bytes */
    o = offs[8]; e = offs[9]; if (e > o) recs(o, e, "wwwwwwwwwwwhhbbbbbbbb");
    o = offs[9]; e = offs[10]; if (e > o) recs(o, e, "wwwwwwwwwwwhhbbbbbbbb");
    /* 0x38..0x3C: the damage states (func_802BD1F8, func_802C09B8): {u32 n;
       n x {u16 index; u8, u8}; u32 [4] (display list offsets)} */
    o = offs[10]; e = offs[11];
    while (o + 4 <= e) {
        uint32_t n = rd32(o);
        cv(o, 4, 4);
        o += 4;
        for (uint32_t i = 0; i < n && o + 4 <= e; i++, o += 4)
            rec(o, "hbb");
        if (o + 16 > e)
            break;
        cv(o, 16, 4);
        o += 16;
    }
    o = offs[11]; e = offs[12]; if (e > o) recs(o, e, "h");                 /* 0x3C..0x40 */
    o = offs[12]; e = offs[13]; if (e > o) recs(o, e, "h");                 /* 0x40..0x44 */
}

/*
 * The level file (LevelHeader, game/level.h; tools/assetlib/level.py has the
 * section order and the record layouts), loaded by func_8025615C.
 */
/* where the level file's section at header field f ends: at the next
   section's start, in file order (0x20..0x74, then 0xA0..0xC0) */
static uint32_t level_end(const uint32_t *offs, int f) {
    int i = (f - 0x20) / 4, n = i + 1;
    if (n == 22)
        n = 32;                         /* after 0x74: 0xA0 */
    return n <= 40 && offs[n] <= len ? offs[n] : len;
}

static void conv_level(void) {
    rec(0, "hhhhhhhhhhhhww");           /* 0x00..0x1F */
    uint32_t offs[42];
    for (int i = 0; i < 42; i++) {
        offs[i] = rd32(0x20 + 4 * i);
        cv(0x20 + 4 * i, 4, 4);
    }
#define LSEC(f) o = offs[((f) - 0x20) / 4], e = level_end(offs, (f))
    uint32_t o, e;
    /* the display data: Vtx, segment 8, up to the first section */
    vtx(0xC8, offs[0]);
    /* fixed records.  Fields the handwritten code reads as big-endian byte
       pairs (lb/lbu, where a record may be unaligned) stay bytes: the
       vehicles (9 bytes), the carrier (10), the buildings' first eight
       (func_802A1D54), the collision triangles (func_802A41B0) */
    static const struct { int field; const char *fmt; } secs[] = {
        {0x20, "hhhh"},                 /* ammo boxes (479D0.c) */
        {0x24, "hhhhhhhhhh"},           /* collision fixes: 9 s16 and a pad (62740) */
        {0x28, "hhhh"},                 /* the comm point (8A2E0) */
        {0x34, "hhh"},                  /* RDUs (2B3F0.c) */
        {0x38, "hhhbbhh"},              /* TNT crates (48D00.c) */
        {0x40, "hhhhh"},                /* bounds (60D50) */
        {0x44, "hhhhh"},
        {0x48, "hh"},                   /* (5FD50) */
        {0x4C, "hhhh"},                 /* the level's bounds (5CB60) */
        {0x58, "hbbbbbb"},              /* LevelUnk58 (32E00.c: s16, then bytes) */
        {0x5C, "bbbbbbbbbbhh"},         /* buildings: x, y, z, type as byte pairs */
    };
    for (size_t k = 0; k < sizeof secs / sizeof secs[0]; k++) {
        LSEC(secs[k].field);
        recs(o, e, secs[k].fmt);
    }
    /* animated textures (5FD50 func_802A5020, func_802A50DC): {u32 id; u8 n,
       frame, flag, alpha; u16 period, timer; u32 texture[n - 1]} */
    LSEC(0x2C);
    while (o + 12 <= e) {
        uint32_t n = base[o + 4];
        rec(o, "wbbbbhh");
        if (n && o + 8 + 4 * n <= e)
            cv(o + 12, 4 * (n - 1), 4);
        o += 8 + 4 * (n ? n : 1);
    }
    /* groups of triangles: u32 end (from the section's start), then records */
    static const struct { int field; const char *fmt; } groups[] = {
        {0x30, "hhhhhhhhhbb"},          /* terrain: read with lh (62740) */
        {0x6C, ""},                     /* collision (x, z): bytes (func_802A3D54) */
        {0x70, ""},                     /* the player's (func_802A3DF8) */
    };
    for (size_t k = 0; k < sizeof groups / sizeof groups[0]; k++) {
        uint32_t s0;
        LSEC(groups[k].field);
        s0 = o;
        while (o + 4 <= e) {
            uint32_t g = s0 + rd32(o);
            cv(o, 4, 4);                /* lwl/lwr: it may be unaligned */
            if (g < o + 4 || g > e) {
                warn("a group ends outside its section at +0x%X", o, 0);
                break;
            }
            if (*groups[k].fmt)
                recs(o + 4, g, groups[k].fmt);
            o = g;
        }
    }
    /* blocks and holes (4B5E0.c func_8028FDA0): u16 n, n x {s16 x, y, z,
       type}, u16 m, m x {s16 x, y, z; u8 type, ntris; s16; ntris x 22 bytes
       of triangle (bytes: func_802A41B0)} */
    LSEC(0x3C);
    if (o + 2 <= e) {
        uint32_t n = rd16(o);
        cv(o, 2, 2);
        o += 2;
        for (uint32_t i = 0; i < n && o + 8 <= e; i++, o += 8)
            rec(o, "hhhh");
        if (o + 2 <= e) {
            uint32_t m = rd16(o);
            cv(o, 2, 2);
            o += 2;
            for (uint32_t i = 0; i < m && o + 10 <= e; i++) {
                uint32_t nt = base[o + 7];
                rec(o, "hhhbbh");
                o += 10 + 22 * nt;
            }
        }
    }
    /* the train stops (func_802A3F80): {u8, u8 k, u8 n; k == 0: n x 12
       bytes, then a u32 (lwl/lwr); else n x 2 bytes; u8 m; m x 21 bytes;
       u8 j; j bytes}, all read as bytes but the u32 */
    LSEC(0x68);
    while (o + 3 <= e) {
        uint32_t k = base[o + 1], n = base[o + 2];
        o += 3;
        if (k == 0) {
            o += 12 * n;
            if (o + 4 <= e)
                cv(o, 4, 4);
            o += 4;
        } else {
            o += 2 * n;
        }
        if (o >= e)
            break;
        o += 1 + 21 * base[o];
        if (o >= e)
            break;
        o += 1 + base[o];
    }
    /* 0x74 (func_802A1A9C): u32 size; if size, groups until one with id -1:
       {s16 [20] (id first); u32 n; n x {s16 [9]; u8, u8; u32 [12]}} */
    LSEC(0x74);
    if (o + 4 <= e) {
        uint32_t size = rd32(o);
        cv(o, 4, 4);
        o += 4;
        if (size)
            for (;;) {
                if (o + 0x2C > e)
                    break;
                int16_t id = (int16_t)rd16(o);
                uint32_t n = rd32(o + 0x28);
                cv(o, 0x28, 2);
                cv(o + 0x28, 4, 4);
                o += 0x2C;
                for (uint32_t i = 0; i < n && o + 0x44 <= e; i++, o += 0x44)
                    rec(o, "hhhhhhhhhbbwwwwwwwwwwww");
                if (id == -1)
                    break;
            }
    }
    /* 0xA0..0xC0 (5FD50 func_802A4E4C): records of words */
    for (int f = 0xA0; f <= 0xC0; f += 4) {
        LSEC(f);
        if (e > o)
            cv(o, (e - o) & ~3u, 4);
    }
    /* the rest (0x50 vehicles, 0x54 the carrier, 0x60, 0x64) are bytes */
#undef LSEC
}

/* the first n bytes of a layout */
static void rec_n(uint32_t off, const char *f, uint32_t n) {
    char part[512];
    uint32_t size = 0, k = 0;
    for (; f[k] && k + 1 < sizeof part; k++) {
        uint32_t w = f[k] == 'h' ? 2 : f[k] == 'w' ? 4 : f[k] == 'd' ? 8 : 1;
        if (size + w > n)
            break;
        part[k] = f[k];
        size += w;
    }
    part[k] = 0;
    rec(off, part);
}

/*
 * The attract mode's recordings (17210.c func_8025B9D0), one after
 * another: {u16, u16, 4 bytes, u16, u8, u8 vehicle type; 0x400 x 5 bytes of
 * input; s16 n; n bytes}, each 2-aligned.  The n bytes are the vehicle's
 * state at the start (func_802AC85C copies them in): the first 0xA6 bytes
 * of a VehicleState, byte by byte, then x, y, z (s32, lwl/lwr).
 */
static void conv_attract(void) {
    uint32_t o = 0;
    while (o + 0x140E <= len) {
        uint32_t n = rd16(o + 0x140C);
        rec(o, "hhbbbbhbb");
        cv(o + 0x140C, 2, 2);
        uint32_t t = o + 0x140E;
        if (n >= 0xA6 + 12 && t + n <= len) {
            rec_n(t, layout_VehicleState, 0xA6);
            rec(t + 0xA6, "www");
        }
        o = (t + n + 1) & ~1u;
    }
}

/* ---- libaudio's banks and sequences -------------------------------------- */

/* the objects of a bank file already converted (they are shared) */
#define MAX_DONE 4096
static uint32_t done[MAX_DONE];
static int ndone;

static int once(uint32_t off) {
    for (int i = 0; i < ndone; i++)
        if (done[i] == off)
            return 0;
    if (ndone < MAX_DONE)
        done[ndone++] = off;
    return off != 0 && off < len;
}

/* ALWaveTable {u8 *base; s32 len; u8 type, flags; {loop, book} | {loop}} */
static void conv_wavetable(uint32_t w) {
    if (!once(w))
        return;
    uint32_t loop = rd32(w + 12), book = rd32(w + 16);
    uint8_t type = w + 8 < len ? base[w + 8] : 0;
    rec(w, "wwbbhw");          /* the pad after flags: the union's alignment */
    if (type == 0) {           /* AL_ADPCM_WAVE */
        cv(w + 16, 4, 4);
        if (loop && once(loop))
            rec(loop, "wwwhhhhhhhhhhhhhhhh");   /* ALADPCMloop */
        if (book && once(book)) {
            uint32_t order = rd32(book), npred = rd32(book + 4);
            rec(book, "ww");
            if (order * npred * 8 * 2 <= len - book - 8)
                cv(book + 8, order * npred * 8 * 2, 2);
            else
                warn("an ADPCM book past the end at +0x%X", book, 0);
        }
    } else if (loop && once(loop)) {
        rec(loop, "www");                       /* ALRawLoop */
    }
}

/* ALSound {env, keymap, wavetable; u8 pan, volume, flags} */
static void conv_sound(uint32_t s) {
    if (!once(s))
        return;
    uint32_t env = rd32(s), km = rd32(s + 4), wt = rd32(s + 8);
    rec(s, "wwwbbbb");
    if (once(env))
        rec(env, "wwwbbbb");                    /* ALEnvelope (+ padding) */
    if (once(km))
        rec(km, "bbbbbb");
    conv_wavetable(wt);
}

/* ALInstrument {12 u8; s16 bendRange, soundCount; ALSound *[]} */
static void conv_inst(uint32_t in) {
    if (!once(in))
        return;
    uint32_t n = rd16(in + 14);
    if (16 + 4 * n > len - in) {
        warn("an instrument past the end at +0x%X", in, 0);
        return;
    }
    rec(in, "bbbbbbbbbbbbhh");
    for (uint32_t i = 0; i < n; i++) {
        uint32_t s = rd32(in + 16 + 4 * i);
        cv(in + 16 + 4 * i, 4, 4);
        conv_sound(s);
    }
}

/* ALBankFile {s16 revision, bankCount; ALBank *[]}; ALBank {s16 instCount;
   u8 flags, pad; s32 sampleRate; ALInstrument *percussion, *[]} (bnkf.c) */
static void conv_bank(void) {
    ndone = 0;
    /* func_80261588 first inflates each bank to 0x8004B400 to learn its
       size, with the LZSS window there too: that copy is scrambled */
    if (rd16(0) != 0x4231)          /* AL_BANK_VERSION */
        return;
    uint32_t nb = rd16(2);
    rec(0, "hh");
    for (uint32_t b = 0; b < nb && 4 + 4 * b + 4 <= len; b++) {
        uint32_t bk = rd32(4 + 4 * b);
        cv(4 + 4 * b, 4, 4);
        if (!once(bk))
            continue;
        uint32_t ni = rd16(bk), perc = rd32(bk + 8);
        rec(bk, "hbbww");
        conv_inst(perc);
        for (uint32_t i = 0; i < ni; i++) {
            uint32_t in = rd32(bk + 12 + 4 * i);
            cv(bk + 12 + 4 * i, 4, 4);
            conv_inst(in);
        }
    }
}

/* ---- entry points ---------------------------------------------------------- */

static void begin(const char *name, uint32_t dst, uint32_t n, uint32_t off) {
    base = port_ptr(dst);
    base_addr = dst;
    len = n;
    load_name = name;
    (void)off;
#ifdef PORT_ACCESS_PROFILE
    port_access_origin(dst, n, port_access_asset(name), off);
#endif
}

/* a gzip member's own name (Rare's headers have FNAME: "chimp_dl.raw"),
   without the extension */
static void gzip_name(const uint8_t *s, char *out, size_t n) {
    out[0] = 0;
    if (s[0] != 0x1F)
        s++;
    if (s[0] != 0x1F || s[1] != 0x8B || !(s[3] & 8))
        return;
    const uint8_t *p = s + 10;
    if (s[3] & 4)
        p += 2 + (p[0] | p[1] << 8);
    size_t k = 0;
    while (*p && *p != '.' && k + 1 < n)
        out[k++] = (char)*p++;
    out[k] = 0;
}

void host_loaded_gzip(uint32_t src, uint32_t dst, uint32_t n) {
    char name[40];
    gzip_name(port_ptr(src), name, sizeof name);
    if (!name[0])
        snprintf(name, sizeof name, "gzip@%08X", src);
    const struct rom_segment *s = segment_named(name);
    int k = s ? s->klass : ROM_BYTES;
    if (strstr(name, "_dl"))
        k = ROM_DL;
    if (host_verbose > 1)
        host_log("load: gzip %s (class %d) -> %08X (%X)\n", name, k, dst, n);
    begin(name, dst, n, 0);
    switch (k) {
    case ROM_DL: conv_dl(); break;
    case ROM_VEHICLE: conv_vehicle(); break;
    case ROM_MODEL: conv_model(); break;
    case ROM_LEVEL: conv_level(); break;
    case ROM_ATTRACT: conv_attract(); break;
    default: break;
    }
}

void host_loaded_lzss(uint32_t rom, uint32_t dst, uint32_t n) {
    const struct rom_segment *s = rom_segment(rom & 0x0FFFFFFFu);
    char name[48];
    snprintf(name, sizeof name, "lzss:%s", s ? s->name : "?");
    if (host_verbose > 1)
        host_log("load: %s -> %08X (%X)\n", name, dst, n);
    begin(name, dst, n, 0);
    switch (s ? s->klass : ROM_BYTES) {
    case ROM_BANK: conv_bank(); break;
    case ROM_STATIC:            /* Vp (s16 x 8), then Gfx */
        cv(0, 16, 2);
        if (n > 16)
            cv(16, (n - 16) & ~3u, 4);
        break;
    default: break;
    }
}

void host_loaded_dma(uint32_t dst, uint32_t rom, uint32_t n) {
    const struct rom_segment *s = rom_segment(rom);
    char name[48];
    snprintf(name, sizeof name, "rom:%s", s ? s->name : "?");
    uint32_t off = s ? rom - s->start : rom;
    begin(name, dst, n, off);
    switch (s ? s->klass : ROM_BYTES) {
    case ROM_TEXTURE_TABLE:     /* {u32 offset, u16 length, u16 type} */
        if (off % 8 == 0)
            recs(0, n, "whh");
        else
            warn("the texture table from +0x%X", off, 0);
        break;
    case ROM_MODEL_TABLE:
        cv(0, n & ~3u, 4);
        break;
    case ROM_SEQUENCES:
        if (off == 0) {         /* ALSeqFile {s16 revision, seqCount; {u8 *offset; s32 len}[]} */
            rec(0, "hh");
            if (n > 4)
                cv(4, (n - 4) & ~3u, 4);
        } else {                /* a sequence: ALCMidiHdr {s32 trackOffset[16]; s32 division} */
            cv(0, n < 0x44 ? n & ~3u : 0x44, 4);
        }
        break;
    default:
        break;
    }
}

/* n bytes of game memory between host order and the N64's, by a layout
   that repeats (src/loads.c: the C's texels that its declarations type as
   u16 or Vtx, to_bytes; and bytes it reads as u16s) */
void host_layout_to_be(uint32_t addr, uint32_t n, const char *layout, int to_bytes) {
    begin("(the C's data)", addr, n, 0);
    uint32_t size = 0;
    for (const char *p = layout; *p; p++)
        size += *p == 'h' ? 2 : *p == 'w' ? 4 : *p == 'd' ? 8 : 1;
    for (uint32_t o = 0; o + size <= n; o += size)
        rec(o, layout);
#ifdef PORT_ACCESS_PROFILE
    if (to_bytes)
        port_access_conv(addr, n, 1);   /* bytes now */
#endif
}

/* the same for count records stride bytes apart (to host order: tagged at
   their widths) */
void host_layout_to_be_n(uint32_t addr, uint32_t stride, uint32_t count, uint32_t n, const char *layout) {
    for (uint32_t i = 0; i < count; i++)
        host_layout_to_be(addr + i * stride, n, layout, 0);
}

/* ---- the save ---------------------------------------------------------------- */

/*
 * EepromSave (game/player.h): the PlayerInfo (u16 units at 0x0A, u32 at
 * 0x10, 0x14 and 0xF0, bytes else), the best times (u16), eight bytes, the
 * semaphore (u64).  n bytes at offset off of it, in place, between host
 * order and the N64's (either way: it is its own inverse).  `off` below
 * 0x100 is inside a PlayerInfo, from 0x100 u16s.  The identity in the
 * big-endian build.
 */
void host_save_order(uint8_t *p, uint32_t off, uint32_t n, int unused) {
    (void)unused;
#ifdef PORT_NATIVE_ENDIAN
    static const struct { uint32_t off, w; } pi[] = {{0x0A, 2}, {0x10, 4}, {0x14, 4}, {0xF0, 4}};
    for (uint32_t k = 0; k < n; ) {
        uint32_t o = off + k, w = 1;
        if (o < 0x100) {
            for (size_t i = 0; i < sizeof pi / sizeof pi[0]; i++)
                if (o == pi[i].off)
                    w = pi[i].w;
        } else if (o < 0x1F0) {
            w = 2;
        } else if (o >= 0x1F8) {
            w = 4;
        }
        if (k + w > n)
            break;
        for (uint32_t a = 0, b = w - 1; a < b; a++, b--) {
            uint8_t t = p[k + a];
            p[k + a] = p[k + b];
            p[k + b] = t;
        }
        k += w;
    }
#else
    (void)p; (void)off; (void)n;
#endif
}
