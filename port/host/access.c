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
#include <signal.h>
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

/* writer kinds (low nibble: the width; 0 for DMA).  In the native-endian
   build also: what a loader converted (host/native.c, at its width: 1 for
   what it left as bytes) and the image's own initialized data, typed by
   asm2x86.py (port_widths) and BEPass (port_cwidths). */
enum { K_C = 0x10, K_ASM = 0x20, K_DMA = 0x40, K_HOST = 0x80, K_CONV = 0xC0, K_IMG = 0xA0 };

static uint8_t *wkind;
static uint8_t *wpos;            /* the byte's place in the unit it was written in */
static uint32_t *wsite;         /* C: function hash; DMA: ROM offset */
static int on = -1;

/* Where ROM-derived bytes came from, for PORT_ACCESS_ORIGIN=FILE: an asset
   (registered by name: a gzip member, an LZSS segment, a raw ROM range) and
   the offset in it, as asset << 20 | offset; and the width the native
   loader converted the byte at (0: not converted, left as bytes).  Every
   read at a width > 1 of such a byte is counted by (asset, offset, width,
   converted width): the observed width map of the ROM's data, which the
   native-endian loaders' types are checked against (docs/PORT.md). */
static uint32_t *origin;
static uint8_t *cwidth;
#define MAX_ASSETS 4096
static char asset_name[MAX_ASSETS][40];
static int n_assets = 1;
typedef struct {
    uint32_t org, site;
    uint8_t w, cw, kind, straddle;
    uint32_t count;
} ORec;
static ORec *otab;
#define OTAB_CAP (1u << 22)
static uint32_t otab_n;
static void odump(void);

typedef struct {
    uint32_t rsite, wsite;
    uint16_t rk, wk;
    uint32_t count, lo, hi;
} Rec;
static Rec *tab;
static uint32_t tab_n, tab_cap = 1u << 16;

static void dump(void);
static void odump(void);
static void seed(void);
int port_access_asset(const char *name);
void port_access_origin(uint32_t dst, uint32_t len, int asset, uint32_t off);

/* a crash writes the tables too, then goes on to main.c's handler */
static struct sigaction prev_segv;
static void on_segv(int sig, siginfo_t *si, void *uc) {
    dump();
    if (getenv("PORT_ACCESS_ORIGIN"))
        odump();
    sigaction(SIGSEGV, &prev_segv, NULL);
    if (prev_segv.sa_flags & SA_SIGINFO)
        prev_segv.sa_sigaction(sig, si, uc);
    else
        raise(sig);
}

static int enabled(void) {
    if (on < 0) {
        on = getenv("PORT_ACCESS") != NULL;
        if (on) {
            wkind = calloc(SH_SIZE, 1);
            wpos = calloc(SH_SIZE, 1);
            wsite = calloc(SH_SIZE, 4);
            tab = calloc(tab_cap, sizeof *tab);
            atexit(dump);
            seed();
            struct sigaction sa;
            memset(&sa, 0, sizeof sa);
            sa.sa_sigaction = on_segv;
            sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
            sigaction(SIGSEGV, &sa, &prev_segv);
            if (getenv("PORT_ACCESS_ORIGIN")) {
                origin = calloc(SH_SIZE, 4);
                cwidth = calloc(SH_SIZE, 1);
                otab = calloc(OTAB_CAP, sizeof *otab);
                atexit(odump);
                /* PORT_ACCESS_REGIONS=NAME@ADDR+LEN,...: parts of the image
                   (its data islands, say) to record the reads of as well */
                const char *r = getenv("PORT_ACCESS_REGIONS");
                while (r && *r) {
                    char name[40];
                    unsigned a, n;
                    int used = 0;
                    if (sscanf(r, "%39[^@]@%x+%x%n", name, &a, &n, &used) != 3 || !used)
                        break;
                    port_access_origin(a, n, port_access_asset(name), 0);
                    r += used;
                    if (*r == ',')
                        r++;
                }
            }
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
        if ((k & 0xF) != w || (k & 0xF0) == K_DMA || wsite[off + b] != wsite[off])
            return (int)b;
    }
    return -1;
}

static void orecord(uint32_t a, unsigned w, uint32_t site, unsigned kind) {
    uint32_t off = sh(a), o = origin[off];
    if (w < 2)
        return;
    if (!o && kind == K_HOST)
        return;             /* (the renderer's reads of CPU data: not needed) */
    unsigned st = 0;
    for (unsigned b = 1; b < w; b++)
        if (origin[off + b] != o + b)
            st = 1;
    unsigned cw = cwidth[off];
    uint32_t h = (o * 2654435761u) ^ (w << 3) ^ (cw << 7) ^ (kind << 11) ^ (site * 40503u);
    for (uint32_t i = h & (OTAB_CAP - 1);; i = (i + 1) & (OTAB_CAP - 1)) {
        ORec *r = &otab[i];
        if (r->count == 0) {
            if (otab_n * 2 > OTAB_CAP)
                return;
            *r = (ORec){o, site, (uint8_t)w, (uint8_t)cw, (uint8_t)kind, (uint8_t)st, 1};
            otab_n++;
            return;
        }
        if (r->org == o && r->w == w && r->cw == cw && r->kind == kind && r->site == site) {
            r->count++;
            r->straddle |= st;
            return;
        }
    }
}

int port_access_asset(const char *name) {
    if (!enabled() || !origin)
        return 0;
    for (int i = 1; i < n_assets; i++)
        if (!strcmp(asset_name[i], name))
            return i;
    if (n_assets == MAX_ASSETS)
        return 0;
    snprintf(asset_name[n_assets], sizeof asset_name[0], "%s", name);
    return n_assets++;
}

void port_access_origin(uint32_t dst, uint32_t len, int asset, uint32_t off) {
    uint32_t d = dst;
    if (d - 0xA0000000u < 0x20000000u)
        d -= 0x20000000u;
    if (!enabled() || !origin || !asset || !in_rdram(d, len))
        return;
    for (uint32_t i = 0; i < len; i++) {
        origin[sh(d) + i] = (uint32_t)asset << 20 | ((off + i) & 0xFFFFF);
        cwidth[sh(d) + i] = 0;
    }
}

void port_access_conv(uint32_t dst, uint32_t len, unsigned width) {
    uint32_t d = dst;
    if (d - 0xA0000000u < 0x20000000u)
        d -= 0x20000000u;
    if (!enabled() || !in_rdram(d, len))
        return;
    if (origin)
        memset(cwidth + sh(d), width, len);
#ifdef PORT_NATIVE_ENDIAN
    /* the loader's conversion is this data's width now */
    for (uint32_t k = 0; k < len; k++) {
        uint32_t u = sh(d) + k - k % width;         /* one site per unit */
        wkind[sh(d) + k] = K_CONV | width;
        wpos[sh(d) + k] = (uint8_t)(k % width);
        wsite[sh(d) + k] = origin && origin[u] ? origin[u] : d + k - k % width;
    }
#endif
}

static void omove(uint32_t d, uint32_t s, uint32_t n) {
    if (origin) {
        memmove(origin + d, origin + s, 4 * n);
        memmove(cwidth + d, cwidth + s, n);
    }
}

static void oclear(uint32_t d, uint32_t n) {
    if (origin) {
        memset(origin + d, 0, 4 * n);
        memset(cwidth + d, 0, n);
    }
}

/* the image's data, by the widths its generators knew */
extern uint32_t __start_port_widths[] __attribute__((weak));
extern uint32_t __stop_port_widths[] __attribute__((weak));
struct cwidth { uint8_t *p; uint32_t count, size; };
extern struct cwidth __start_port_cwidths[] __attribute__((weak));
extern struct cwidth __stop_port_cwidths[] __attribute__((weak));

static void seed_run(uint32_t a, uint32_t n, unsigned w) {
    if (!in_rdram(a, n) || !w)
        return;
    for (uint32_t k = 0; k < n; k++) {
        wkind[sh(a) + k] = K_IMG | (w > 4 ? 4 : w);
        wpos[sh(a) + k] = (uint8_t)(k % (w > 4 ? 4 : w));
        wsite[sh(a) + k] = a + k - k % (w > 4 ? 4 : w);   /* one per unit */
    }
}

static void seed(void) {
    for (uint32_t *p = __start_port_widths; p && p + 1 < __stop_port_widths; p += 2)
        seed_run(p[0], p[1] >> 4, p[1] & 0xF);
    for (struct cwidth *c = __start_port_cwidths; c && c < __stop_port_cwidths; c++)
        seed_run((uint32_t)(uintptr_t)c->p, c->count * c->size, c->size);
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

/* the same in either byte order: all its bytes equal (0, -1 ...) */
static int symmetric(uint64_t raw, unsigned w) {
    for (unsigned b = 1; b < w; b++)
        if (((raw >> 8 * b) & 0xFF) != (raw & 0xFF))
            return 0;
    return 1;
}

static void read_at(uint32_t a, unsigned w, uint32_t site, unsigned kind, uint64_t raw) {
#ifdef PORT_NATIVE_ENDIAN
    if (w == 8) {       /* two words, the high one first, in every piece of code */
        read_at(a, 4, site, kind, raw >> 32);
        read_at(a + 4, 4, site, kind, (uint32_t)raw);
        return;
    }
#endif
    if (origin)
        orecord(a, w, site, kind);
    int b = mismatch(a, w);
    if (b < 0 || symmetric(raw, w))
        b = 0xFF;       /* no mismatch, but it may be a copy (alCopy, byte loops) */
#ifdef PORT_NATIVE_ENDIAN
    /* the handwritten code's byte of a halfword or word: in host order it
       is the byte at the mirrored place (unless the two are equal), so
       wrong unless copied.  (The C's byte reads of wider data are its byte
       copies, often into locals the profiler doesn't see: left out.) */
    if (w == 1 && kind == K_ASM) {
        unsigned k = wkind[sh(a)], kw = k & 0xF;
        if ((kw == 2 || kw == 4) && (k & 0xF0) != K_DMA &&
            *(volatile uint8_t *)(uintptr_t)(a ^ (kw - 1)) != (uint8_t)raw)
            b = 0;
    }
    /* what a loader converted or the image's data, read by the C at
       another width: wrong whatever becomes of the value (the C copies with
       memcpy; the asm's word loops are copies and are left to the check
       below) */
    if (b != 0xFF && kind == K_C && ((wkind[sh(a) + b] & 0xE0) == K_CONV || (wkind[sh(a) + b] & 0xE0) == K_IMG)) {
        uint32_t off = sh(a) + b;
        record(site, kind | w, wsite[off], wkind[off], a + b);
        b = 0xFF;
    }
#endif
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
#ifdef PORT_NATIVE_ENDIAN
    if (w == 8) {
        write_at(a, 4, site, kind, raw >> 32);
        write_at(a + 4, 4, site, kind, (uint32_t)raw);
        return;
    }
#endif
    uint32_t off = sh(a);
    int q = kind == K_ASM;
    for (int i = (int)pend_n[q] - 1; i >= 0; i--) {
        Pend *p = &pend[q][i];
        if (p->w == w && p->raw == raw) {
            uint32_t src = sh(p->addr);
            memmove(wkind + off, wkind + src, w);
            memmove(wpos + off, wpos + src, w);
            memmove(wsite + off, wsite + src, 4 * w);
            omove(off, src, w);
            memmove(p, p + 1, (pend_n[q] - 1 - i) * sizeof(Pend));
            pend_n[q]--;
            return;
        }
    }
    /* zeros read the same in either byte order */
    memset(wkind + off, raw ? kind | w : 0, w);
    for (unsigned b = 0; b < w; b++)
        wpos[off + b] = (uint8_t)b;
    for (unsigned b = 0; b < w; b++)
        wsite[off + b] = site;
    oclear(off, w);
}

static uint32_t addr_of(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a - 0xA0000000u < 0x20000000u)
        a -= 0x20000000u;
    return (uint32_t)a;
}

/* the game's C (BEPass) */
void __port_access(void *p, uint32_t sz, uint32_t site, uint32_t lo, uint32_t hi) {
    uint32_t a = addr_of(p), w = sz & 0xFF;
    uint64_t raw = (uint64_t)hi << 32 | lo;
    if (!enabled() || !in_rdram(a, w))
        return;
    if (sz & 0x200) {               /* swapped by the source: bytes in the N64's order */
        if (sz & 0x100)
            for (uint32_t b = 0; b < w; b++)
                write_at(a + b, 1, site, K_C, (raw >> 8 * b) & 0xFF);
        return;
    }
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
        memmove(wpos + sh(d), wpos + sh(s), n);
        memmove(wsite + sh(d), wsite + sh(s), 4 * n);
        omove(sh(d), sh(s), n);
    } else if (in_rdram(d, n)) {
        memset(wkind + sh(d), 0, n);
        memset(wpos + sh(d), 0, n);
        oclear(sh(d), n);
    }
}

void __port_access_set(void *dst, uint32_t n, uint32_t site) {
    uint32_t d = addr_of(dst);
    (void)site;
    if (enabled() && n && in_rdram(d, n)) {
        memset(wkind + sh(d), 0, n);    /* cleared: any width reads 0 */
        memset(wpos + sh(d), 0, n);
        oclear(sh(d), n);
    }
}

/* the translated code (recomp.h, RECOMP_ACCESS): the site is the
   instruction's N64 address */
static uint32_t asm_pc;
void recomp_access_pc(uint32_t pc) { asm_pc = pc; }

/* the next access is at a listed site of native-endian memory
   (tools/recomp/native_sites.txt): a word that is two halves, a big-endian
   datum, a part of a wider field */
enum { NE_H2 = 1, NE_BE, NE_X1, NE_X2, NE_X3, NE_UNALIGNED };
static int site_kind;
void recomp_access_site(int kind) { site_kind = kind; }

void recomp_access(uint32_t a, uint32_t sz, uint64_t raw) {
    uint32_t w = sz & 0xFF;
    a = addr_of((void *)(uintptr_t)a);
    int kind = site_kind;
    site_kind = 0;
    if (!enabled() || !in_rdram(a, w))
        return;
    if (kind == NE_H2) {            /* two 16-bit fields */
        for (int h = 0; h < 2; h++)
            if (sz & 0x100)
                write_at(a + 2 * h, 2, asm_pc, K_ASM, (raw >> 16 * h) & 0xFFFF);
            else
                read_at(a + 2 * h, 2, asm_pc, K_ASM, (raw >> 16 * h) & 0xFFFF);
        return;
    }
    if (kind == NE_BE) {            /* bytes in the N64's order */
        if (sz & 0x100)
            for (uint32_t b = 0; b < w; b++)
                write_at(a + b, 1, asm_pc, K_ASM, (raw >> 8 * b) & 0xFF);
        return;
    }
    if (kind == NE_X1 || kind == NE_X2 || kind == NE_X3) {
        /* a part of a wider field (a halfword for x1, a word for x2 and
           x3): a store makes the field the unit written */
        if (sz & 0x100) {
            uint32_t u = kind == NE_X1 ? 2 : 4, f = a & ~(u - 1), off = sh(f);
            memset(wkind + off, K_ASM | u, u);
            for (uint32_t b = 0; b < u; b++) {
                wpos[off + b] = (uint8_t)b;
                wsite[off + b] = asm_pc;
            }
            oclear(off, u);
        }
        return;
    }
    if (kind)                       /* lwl/lwr/swl/swr: as it is */
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
    memset(wpos + sh(d), 0, len);
    for (uint32_t i = 0; i < len; i++)
        wsite[sh(d) + i] = rom + i;
}

/* the host's reads of game memory (the RSP/RDP, the AI): never copies */
void port_access_host(const void *p, unsigned w) {
    uint32_t a = addr_of(p);
    int b;
    if (enabled() && origin && in_rdram(a, w))
        orecord(a, w, 0, K_HOST);
    if (enabled() && in_rdram(a, w) && (b = mismatch(a, w)) >= 0) {
        uint32_t off = sh(a) + b;
        record(0, K_HOST | w, wsite[off], wkind[off], a + b);
    }
}

static void odump(void) {
    FILE *f = fopen(getenv("PORT_ACCESS_ORIGIN"), "w");
    if (!f)
        return;
    static const char *kn[16] = {"?", "c", "asm", "?", "dma", "?", "?", "?", "host", "?", "img", "?", "conv"};
    fprintf(f, "asset\toffset\twidth\tconv\treader\tsite\tcount\tstraddle\n");
    for (uint32_t i = 0; i < OTAB_CAP; i++) {
        ORec *r = &otab[i];
        if (r->count)
            fprintf(f, "%s\t%X\t%u\t%u\t%s\t%08X\t%u\t%u\n", r->org ? asset_name[r->org >> 20] : "-", r->org & 0xFFFFF, r->w,
                    r->cw, kn[(r->kind >> 4) & 0xF], r->site, r->count, r->straddle);
    }
    fclose(f);
}

/* the host's reads of what is bytes in either byte order (texels, the
   framebuffers): whatever wrote it had to write bytes */
void port_access_bytes(const void *p, uint32_t n) {
    uint32_t a = addr_of(p);
    if (!enabled() || !in_rdram(a, n))
        return;
    for (uint32_t b = 0; b < n; b++) {
        unsigned k = wkind[sh(a) + b];
        if (k && (k & 0xF) > 1 && (k & 0xF0) != K_DMA) {
            record(0, K_HOST | 1, wsite[sh(a) + b], k, a + b);
            return;
        }
    }
}

/* rdram_N.widths next to a PORT_DUMP's rdram_N.bin: per byte of RDRAM the
   width it was last written or converted at (the low nibble, 0 unknown),
   0x10 where a unit starts (build_cmp.py rdram --native --widths) */
void port_access_dump_widths(unsigned n) {
    if (!enabled())
        return;
    char path[64];
    snprintf(path, sizeof path, "rdram_%u.widths", n);
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    static uint8_t out[RD_SIZE];
    for (uint32_t i = 0; i < RD_SIZE; i++) {
        unsigned k = wkind[i], w = k & 0xF;
        if ((k & 0xF0) == K_DMA)
            w = 1;                      /* untouched DMA'd bytes */
        /* bit 4: a unit starts here; bits 5..7: bytes into the unit (a
           byte whose unit was partly overwritten keeps its place, so a cut
           unit shows) */
        unsigned pos = w > 1 ? wpos[i] : 0;
        out[i] = (uint8_t)(w | (pos == 0 ? 0x10 : 0) | (pos & 7) << 5);
    }
    fwrite(out, 1, RD_SIZE, f);
    fclose(f);
}

/* PORT_ACCESS_WHO=ADDR,...: at exit, who wrote each of those bytes last
   (to stderr: kind, width, site) */
static void who(void) {
    const char *w = getenv("PORT_ACCESS_WHO");
    static const char *kn[16] = {"?", "c", "asm", "?", "dma", "?", "?", "?", "host", "?", "img", "?", "conv"};
    while (w && *w) {
        unsigned a;
        int used = 0;
        if (sscanf(w, "%x%n", &a, &used) != 1 || !used)
            break;
        for (unsigned b = 0; b < 4 && in_rdram(a + b, 1); b++) {
            unsigned k = wkind[sh(a + b)];
            fprintf(stderr, "who %08X: %s w%u site %08X\n", a + b, kn[(k >> 4) & 0xF], k & 0xF, wsite[sh(a + b)]);
        }
        w += used;
        if (*w == ',')
            w++;
    }
}

static void dump(void) {
    who();
    for (int q = 0; q < 2; q++)
        for (unsigned i = 0; i < pend_n[q]; i++)
            flush_one(&pend[q][i]);
    FILE *f = fopen(getenv("PORT_ACCESS"), "w");
    if (!f)
        return;
    fprintf(f, "reader\trsite\trwidth\twriter\twsite\twwidth\tcount\tlo\thi\n");
    for (int i = 1; origin && i < n_assets; i++)     /* a conv writer's site: asset << 20 | offset */
        fprintf(f, "#asset\t%d\t%s\n", i, asset_name[i]);
    static const char *kn[16] = {"?", "c", "asm", "?", "dma", "?", "?", "?", "host", "?", "img", "?", "conv"};
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
