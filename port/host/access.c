/*
 * The access-width profiler (PORT_ACCESS_PROFILE builds; docs/PORT.md,
 * "Native-endian memory").
 *
 * Big-endian memory lets any byte be read at any width and mean what it
 * meant on the N64.  Native-endian memory doesn't: a datum has to be read
 * at the width it was written (or converted, when it came from the ROM).
 * This finds where that doesn't hold.  For every byte of RDRAM it keeps
 * who wrote it last and at what width (the game's C, the translated asm, a
 * PI DMA from the ROM, a copy passes that on); every read at another width
 * is counted, by reader, writer and widths.  At exit PORT_ACCESS=FILE gets
 * the table (port/tools/access_report.py names the sites).
 *
 * The C reports through BEPass's hooks (BEPASS_ACCESS=1), the translated
 * code through recomp.h's RECOMP_ACCESS, DMA through host_rom_read, the
 * renderer's and the audio HLE's reads through port_be16/port_be32.
 */
#ifdef PORT_ACCESS_PROFILE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"

#define RD_BASE 0x80000000u
#define RD_SIZE 0x00400000u
/* the fibers' stacks too: the C's locals carry data along (a sort through
   a temporary) */
#define ST_SIZE (PORT_STACK_SIZE * PORT_MAX_THREADS)
#define SH_SIZE (RD_SIZE + ST_SIZE)

/* writer kinds (low nibble: the width; 0 for DMA) */
enum { K_C = 0x10, K_ASM = 0x20, K_DMA = 0x40, K_HOST = 0x80 };

static uint8_t *wkind;
static uint32_t *wsite;         /* C: function hash; DMA: ROM offset */
static int on = -1;

typedef struct {
    uint32_t rsite, wsite;
    uint16_t rk, wk;
    uint32_t count, lo, hi;
} Rec;
static Rec *tab;
static uint32_t tab_n, tab_cap = 1u << 16;

static void dump(void);

static int enabled(void) {
    if (on < 0) {
        on = getenv("PORT_ACCESS") != NULL;
        if (on) {
            wkind = calloc(SH_SIZE, 1);
            wsite = calloc(SH_SIZE, 4);
            tab = calloc(tab_cap, sizeof *tab);
            atexit(dump);
        }
    }
    return on;
}

static void record(uint32_t rsite, unsigned rk, uint32_t ws, unsigned wk, uint32_t off) {
    if (wk & K_DMA)
        ws &= ~0xFFFu;                  /* 4 KB of ROM per row */
    uint32_t h = (rsite * 2654435761u) ^ (ws * 40503u) ^ (rk << 8) ^ wk;
    for (uint32_t i = h & (tab_cap - 1);; i = (i + 1) & (tab_cap - 1)) {
        Rec *r = &tab[i];
        if (r->count == 0) {
            if (tab_n * 2 > tab_cap)
                return;                 /* full: keep what we have */
            *r = (Rec){rsite, ws, (uint16_t)rk, (uint16_t)wk, 1, off, off};
            tab_n++;
            return;
        }
        if (r->rsite == rsite && r->wsite == ws && r->rk == rk && r->wk == wk) {
            r->count++;
            if (off < r->lo) r->lo = off;
            if (off > r->hi) r->hi = off;
            return;
        }
    }
}

static int in_rdram(uint32_t a, uint32_t n) {
    return (a - RD_BASE < RD_SIZE && a - RD_BASE + n <= RD_SIZE) ||
           (a - PORT_STACK_BASE < ST_SIZE && a - PORT_STACK_BASE + n <= ST_SIZE);
}

/* the shadow's index of an address in_rdram() accepts */
static uint32_t sh(uint32_t a) {
    return a - RD_BASE < RD_SIZE ? a - RD_BASE : RD_SIZE + (a - PORT_STACK_BASE);
}

/* The first byte of [a, a + w) whose writer doesn't match a read of width
   w, or -1.  Bytes are bytes: a byte read never mismatches. */
static int mismatch(uint32_t a, unsigned w) {
    uint32_t off = sh(a);
    if (w == 1)
        return -1;
    for (unsigned b = 0; b < w; b++) {
        unsigned k = wkind[off + b];
        if (!k)
            continue;                   /* the image's own data, cleared, or never written */
        if ((k & 0xF) != w || (k & K_DMA) || wsite[off + b] != wsite[off])
            return (int)b;
    }
    return -1;
}

/* A mismatched read is held back for a while: if the same bytes are then
   stored somewhere at the same width, it was a copy (memcpy loops, ld/sd
   pairs, a field copied into another struct), which native-endian memory
   gets right; the copy then carries the source's writers along.  Reads
   that aren't copied go into the table. */
#define PEND 16
typedef struct {
    uint32_t addr, site, wsite;
    uint64_t raw;
    uint8_t w, kind, bad, wkind;
} Pend;
static Pend pend[2][PEND];
static unsigned pend_n[2];

static void flush_one(Pend *p) {
    if (p->bad != 0xFF)
        record(p->site, p->kind | p->w, p->wsite, p->wkind, p->addr + p->bad);
}

static void read_at(uint32_t a, unsigned w, uint32_t site, unsigned kind, uint64_t raw) {
    int b = mismatch(a, w);
    if (b < 0)
        b = 0xFF;       /* no mismatch, but it may be a copy (alCopy, byte loops) */
    int q = kind == K_ASM;
    if (pend_n[q] == PEND) {
        flush_one(&pend[q][0]);
        memmove(&pend[q][0], &pend[q][1], (PEND - 1) * sizeof(Pend));
        pend_n[q]--;
    }
    uint32_t off = sh(a) + (b == 0xFF ? 0 : b);
    pend[q][pend_n[q]++] = (Pend){a, site, wsite[off], raw, (uint8_t)w, (uint8_t)kind, (uint8_t)b, wkind[off]};
}

static void write_at(uint32_t a, unsigned w, uint32_t site, unsigned kind, uint64_t raw) {
    uint32_t off = sh(a);
    int q = kind == K_ASM;
    for (int i = (int)pend_n[q] - 1; i >= 0; i--) {
        Pend *p = &pend[q][i];
        if (p->w == w && p->raw == raw) {
            uint32_t src = sh(p->addr);
            memmove(wkind + off, wkind + src, w);
            memmove(wsite + off, wsite + src, 4 * w);
            memmove(p, p + 1, (pend_n[q] - 1 - i) * sizeof(Pend));
            pend_n[q]--;
            return;
        }
    }
    /* zeros read the same in either byte order */
    memset(wkind + off, raw ? kind | w : 0, w);
    for (unsigned b = 0; b < w; b++)
        wsite[off + b] = site;
}

static uint32_t addr_of(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a - 0xA0000000u < 0x20000000u)
        a -= 0x20000000u;
    return (uint32_t)a;
}

/* the game's C (BEPass) */
void __port_access(void *p, uint32_t sz, uint32_t site, uint64_t raw) {
    uint32_t a = addr_of(p), w = sz & 0xFF;
    if (!enabled() || !in_rdram(a, w))
        return;
    if (sz & 0x100)
        write_at(a, w, site, K_C, raw);
    else
        read_at(a, w, site, K_C, raw);
}

void __port_access_copy(void *dst, const void *src, uint32_t n, uint32_t site) {
    uint32_t d = addr_of(dst), s = addr_of(src);
    (void)site;
    if (!enabled() || !n)
        return;
    if (in_rdram(d, n) && in_rdram(s, n)) {
        memmove(wkind + sh(d), wkind + sh(s), n);
        memmove(wsite + sh(d), wsite + sh(s), 4 * n);
    } else if (in_rdram(d, n)) {
        memset(wkind + sh(d), 0, n);
    }
}

void __port_access_set(void *dst, uint32_t n, uint32_t site) {
    uint32_t d = addr_of(dst);
    (void)site;
    if (enabled() && n && in_rdram(d, n))
        memset(wkind + sh(d), 0, n);    /* cleared: any width reads 0 */
}

/* the translated code (recomp.h, RECOMP_ACCESS): the site is the
   instruction's N64 address */
static uint32_t asm_pc;
void recomp_access_pc(uint32_t pc) { asm_pc = pc; }

void recomp_access(uint32_t a, uint32_t sz, uint64_t raw) {
    uint32_t w = sz & 0xFF;
    a = addr_of((void *)(uintptr_t)a);
    if (!enabled() || !in_rdram(a, w))
        return;
    if (sz & 0x100)
        write_at(a, w, asm_pc, K_ASM, raw);
    else
        read_at(a, w, asm_pc, K_ASM, raw);
}

/* PI DMA */
void port_access_dma(uint32_t dst, uint32_t rom, uint32_t len) {
    uint32_t d = addr_of((void *)(uintptr_t)dst);
    if (!enabled() || !in_rdram(d, len))
        return;
    memset(wkind + sh(d), K_DMA, len);
    for (uint32_t i = 0; i < len; i++)
        wsite[sh(d) + i] = rom + i;
}

/* the host's reads of game memory (the RSP/RDP, the AI): never copies */
void port_access_host(const void *p, unsigned w) {
    uint32_t a = addr_of(p);
    int b;
    if (enabled() && in_rdram(a, w) && (b = mismatch(a, w)) >= 0) {
        uint32_t off = sh(a) + b;
        record(0, K_HOST | w, wsite[off], wkind[off], a + b);
    }
}

static void dump(void) {
    for (int q = 0; q < 2; q++)
        for (unsigned i = 0; i < pend_n[q]; i++)
            flush_one(&pend[q][i]);
    FILE *f = fopen(getenv("PORT_ACCESS"), "w");
    if (!f)
        return;
    fprintf(f, "reader\trsite\trwidth\twriter\twsite\twwidth\tcount\tlo\thi\n");
    static const char *kn[] = {"?", "c", "asm", "?", "dma", "?", "?", "?", "host"};
    for (uint32_t i = 0; i < tab_cap; i++) {
        Rec *r = &tab[i];
        if (!r->count)
            continue;
        fprintf(f, "%s\t%08X\t%u\t%s\t%08X\t%u\t%u\t%08X\t%08X\n", kn[(r->rk >> 4) & 0xF], r->rsite, r->rk & 0xF,
                kn[(r->wk >> 4) & 0xF], r->wsite, r->wk & 0xF, r->count, r->lo, r->hi);
    }
    fclose(f);
}
#endif
