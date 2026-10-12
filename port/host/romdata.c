/*
 * The ROM as the port's data source (docs/DISTRIBUTION.md): the ROM's
 * sha1, its gzip members inflated, and the ROM's assets in the game's data
 * (images, vertices, the lists that draw them), which the executable
 * doesn't carry, filled from the ROM's code modules at startup
 * (port_rom_fill; port/tools/rom_assets.txt).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "pack.h"

/* ---- sha1 (FIPS 180-4) ------------------------------------------------------ */

static uint32_t rol(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static void sha1_block(uint32_t h[5], const uint8_t *p) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 80; i++)
        w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999u;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        uint32_t t = rol(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rol(b, 30);
        b = a;
        a = t;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

/* the sha1 of n bytes as 40 lowercase hex digits */
void host_sha1_hex(const uint8_t *p, size_t n, char out[41]) {
    uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
    size_t k = 0;
    for (; k + 64 <= n; k += 64)
        sha1_block(h, p + k);
    uint8_t last[128] = {0};
    size_t r = n - k;
    memcpy(last, p + k, r);
    last[r] = 0x80;
    size_t len = r + 1 + 8 <= 64 ? 64 : 128;
    uint64_t bits = (uint64_t)n * 8;
    for (int i = 0; i < 8; i++)
        last[len - 1 - i] = (uint8_t)(bits >> (8 * i));
    sha1_block(h, last);
    if (len == 128)
        sha1_block(h, last + 64);
    for (int i = 0; i < 5; i++)
        snprintf(out + 8 * i, 9, "%08x", h[i]);
}

/* ---- inflate (RFC 1951), for the ROM's gzip members (RFC 1952) --------------- */

struct inf {
    const uint8_t *in;
    size_t n, pos;
    uint32_t bitbuf;
    int bitcnt;
    uint8_t *out;
    size_t outn, outpos;
};

/* a canonical Huffman code: how many codes of each length, and the
   symbols in code order */
struct huff {
    uint16_t count[16];
    uint16_t sym[320];
};

static int bits(struct inf *s, int need, uint32_t *v) {
    uint32_t b = s->bitbuf;
    while (s->bitcnt < need) {
        if (s->pos >= s->n)
            return -1;
        b |= (uint32_t)s->in[s->pos++] << s->bitcnt;
        s->bitcnt += 8;
    }
    *v = b & ((1u << need) - 1);
    s->bitbuf = b >> need;
    s->bitcnt -= need;
    return 0;
}

static int build(struct huff *h, const uint8_t *len, int n) {
    uint16_t offs[16];
    memset(h->count, 0, sizeof h->count);
    for (int i = 0; i < n; i++)
        h->count[len[i]]++;
    h->count[0] = 0;
    offs[1] = 0;
    for (int l = 1; l < 15; l++)
        offs[l + 1] = offs[l] + h->count[l];
    for (int i = 0; i < n; i++)
        if (len[i])
            h->sym[offs[len[i]]++] = (uint16_t)i;
    return 0;
}

static int decode(struct inf *s, const struct huff *h) {
    int code = 0, first = 0, index = 0;
    for (int l = 1; l < 16; l++) {
        uint32_t b;
        if (bits(s, 1, &b))
            return -1;
        code |= (int)b;
        int count = h->count[l];
        if (code - count < first)
            return h->sym[index + (code - first)];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

/* the length and distance codes' base values and extra bits (RFC 1951,
   3.2.5), worked out rather than tabulated */
static uint16_t len_base[29], dist_base[30];
static uint8_t len_extra[29], dist_extra[30];

static void tables(void) {
    if (len_base[0])
        return;
    uint32_t b = 3;
    for (int i = 0; i < 28; i++) {
        len_extra[i] = (uint8_t)(i < 8 ? 0 : i / 4 - 1);
        len_base[i] = (uint16_t)b;
        b += 1u << len_extra[i];
    }
    len_base[28] = 258;
    len_extra[28] = 0;
    b = 1;
    for (int i = 0; i < 30; i++) {
        dist_extra[i] = (uint8_t)(i < 4 ? 0 : i / 2 - 1);
        dist_base[i] = (uint16_t)b;
        b += 1u << dist_extra[i];
    }
}

static int codes(struct inf *s, const struct huff *lit, const struct huff *dist) {
    for (;;) {
        int sym = decode(s, lit);
        if (sym < 0)
            return -1;
        if (sym < 256) {
            if (s->outpos >= s->outn)
                return -1;
            s->out[s->outpos++] = (uint8_t)sym;
        } else if (sym == 256) {
            return 0;
        } else {
            sym -= 257;
            if (sym >= 29)
                return -1;
            uint32_t e;
            if (bits(s, len_extra[sym], &e))
                return -1;
            size_t len = len_base[sym] + e;
            int ds = decode(s, dist);
            if (ds < 0 || ds >= 30 || bits(s, dist_extra[ds], &e))
                return -1;
            size_t d = dist_base[ds] + e;
            if (d > s->outpos || s->outpos + len > s->outn)
                return -1;
            for (size_t k = 0; k < len; k++, s->outpos++)
                s->out[s->outpos] = s->out[s->outpos - d];
        }
    }
}

static int fixed(struct inf *s) {
    static struct huff lit, dist;
    static int made;
    if (!made) {
        uint8_t l[288];
        int i = 0;
        for (; i < 144; i++)
            l[i] = 8;
        for (; i < 256; i++)
            l[i] = 9;
        for (; i < 280; i++)
            l[i] = 7;
        for (; i < 288; i++)
            l[i] = 8;
        build(&lit, l, 288);
        for (i = 0; i < 30; i++)
            l[i] = 5;
        build(&dist, l, 30);
        made = 1;
    }
    return codes(s, &lit, &dist);
}

static int dynamic(struct inf *s) {
    uint32_t hlit, hdist, hclen, v;
    uint8_t l[320];
    struct huff lc, lit, dist;
    if (bits(s, 5, &hlit) || bits(s, 5, &hdist) || bits(s, 4, &hclen))
        return -1;
    hlit += 257;
    hdist += 1;
    hclen += 4;
    if (hlit > 286 || hdist > 30)
        return -1;
    memset(l, 0, 19);
    for (uint32_t i = 0; i < hclen; i++) {
        if (bits(s, 3, &v))
            return -1;
        /* the code lengths' order: 16, 17, 18, 0, 8, 7, 9, 6, ..., 1, 15 */
        int o = i < 3 ? 16 + (int)i : i == 3 ? 0 : (i & 1) ? 8 - (int)(i - 3) / 2 : 8 + (int)(i - 4) / 2;
        l[o] = (uint8_t)v;
    }
    build(&lc, l, 19);
    uint32_t n = 0;
    while (n < hlit + hdist) {
        int sym = decode(s, &lc);
        if (sym < 0)
            return -1;
        if (sym < 16) {
            l[n++] = (uint8_t)sym;
            continue;
        }
        uint8_t rep = 0;
        uint32_t times;
        if (sym == 16) {
            if (n == 0 || bits(s, 2, &v))
                return -1;
            rep = l[n - 1];
            times = 3 + v;
        } else if (sym == 17) {
            if (bits(s, 3, &v))
                return -1;
            times = 3 + v;
        } else {
            if (bits(s, 7, &v))
                return -1;
            times = 11 + v;
        }
        if (n + times > hlit + hdist)
            return -1;
        while (times--)
            l[n++] = rep;
    }
    build(&lit, l, (int)hlit);
    build(&dist, l + hlit, (int)hdist);
    return codes(s, &lit, &dist);
}

static size_t gunzip_used;      /* the last member's length, in the input */

/* a gzip member's contents into out (at most outn bytes); their length, or -1 */
static long gunzip(const uint8_t *in, size_t n, uint8_t *out, size_t outn) {
    if (n < 18 || in[0] != 0x1F || in[1] != 0x8B || in[2] != 8)
        return -1;
    uint8_t flg = in[3];
    size_t p = 10;
    if (flg & 4) {
        if (p + 2 > n)
            return -1;
        p += 2 + (in[p] | in[p + 1] << 8);
    }
    for (int f = 8; f <= 16; f <<= 1)       /* FNAME, FCOMMENT: zero-terminated */
        if (flg & f) {
            while (p < n && in[p])
                p++;
            p++;
        }
    if (flg & 2)
        p += 2;
    if (p >= n)
        return -1;
    struct inf s = {in + p, n - p, 0, 0, 0, out, outn, 0};
    tables();
    uint32_t last, type;
    do {
        if (bits(&s, 1, &last) || bits(&s, 2, &type))
            return -1;
        if (type == 0) {
            s.bitbuf = 0;
            s.bitcnt = 0;
            if (s.pos + 4 > s.n)
                return -1;
            size_t len = s.in[s.pos] | s.in[s.pos + 1] << 8;
            s.pos += 4;
            if (s.pos + len > s.n || s.outpos + len > s.outn)
                return -1;
            memcpy(s.out + s.outpos, s.in + s.pos, len);
            s.pos += len;
            s.outpos += len;
        } else if (type == 1) {
            if (fixed(&s))
                return -1;
        } else if (type == 2) {
            if (dynamic(&s))
                return -1;
        } else {
            return -1;
        }
    } while (!last);
    gunzip_used = p + s.pos - (size_t)(s.bitcnt / 8) + 8;     /* (and the CRC and length) */
    return (long)s.outpos;
}

/* (micons.c too) */
long host_gunzip(const uint8_t *in, size_t n, uint8_t *out, size_t outn, size_t *used) {
    long r = gunzip(in, n, out, outn);
    if (used)
        *used = gunzip_used;
    return r;
}

/* ---- the ROM's assets in the game's data (port.h's port_fill) ------------------ */

#define RDRAM 0x400000u
#define HD_CODE 0x2447C0u           /* the modules' load addresses, physical */
#define HD_FRONT_END 0x1E7000u

/* a code module's .text and .data inflated from the ROM where the game loads
   them, one after the other */
static void module(uint8_t *src, uint32_t base, const char *name) {
    const uint8_t *rom = host_rom();
    uint32_t rom_size = host_rom_size();
    char t_name[48], d_name[48];
    uint32_t ts, te, ds, de;
    snprintf(t_name, sizeof t_name, "%s_text." PORT_VERSION_NAME, name);
    snprintf(d_name, sizeof d_name, "%s_data." PORT_VERSION_NAME, name);
    if (!pack_rom_segment(t_name, &ts, &te) || !pack_rom_segment(d_name, &ds, &de))
        host_fatal("the link has no %s", name);
    if (te > rom_size || de > rom_size)
        host_fatal("the ROM is too short for %s", name);
    long t = gunzip(rom + ts, te - ts, src + base, RDRAM - base);
    long d = t < 0 ? -1 : gunzip(rom + ds, de - ds, src + base + t, RDRAM - base - t);
    if (d < 0)
        host_fatal("can't inflate %s from the ROM", name);
}

/* The ROM's assets the executable has as zeros (gen/rom_fill.c's table,
   port/tools/gen_fill.py), from the modules' data as the game loads them:
   inflated from the ROM, or a resource pack's (pack.c: a pack without the
   code has them).  Before anything reads them: src/boot.c, first. */
void port_rom_fill(void) {
    double t0 = host_perf_now();
    const uint8_t *src = pack_data_source();
    uint8_t *own = NULL;
    if (!src) {
        own = calloc(RDRAM, 1);
        if (!own)
            host_fatal("out of memory for the ROM's modules");
        module(own, HD_CODE, "hd_code");
        module(own, HD_FRONT_END, "hd_front_end");
        src = own;
    }
    uint32_t bytes = 0;
    for (unsigned k = 0; k < port_fill_n; k++) {
        const PortFill *e = &port_fill[k];
        const uint8_t *s = src + e->n64;
        if (e->n64 >= RDRAM || e->size > RDRAM - e->n64)
            host_fatal("port_fill[%u]: outside the modules", k);
        bytes += e->size;
        switch (e->kind) {
        case PORT_FILL_BYTES:
            /* (texels, as the RDP reads them, whatever the C declares) */
            memcpy(e->dst, s, e->size);
            break;
        case PORT_FILL_VTX:
            /* gbi.h's Vtx: s16 ob[3], u16 flag, s16 tc[2] in host order, u8 cn[4] */
            for (uint32_t v = 0; v < e->size; v += 16) {
                uint8_t *d = (uint8_t *)e->dst + v;
                for (int j = 0; j < 12; j += 2) {
                    uint16_t h = (uint16_t)(s[v + j] << 8 | s[v + j + 1]);
                    memcpy(d + j, &h, 2);
                }
                memcpy(d + 12, s + v + 12, 4);
            }
            break;
        case PORT_FILL_GFX: {
            /* commands, each word as wide as a pointer; a segmented address
               (G_MTX, G_MOVEMEM, G_VTX, G_DL to segments 1..15) tagged as
               gbi.h's G_SEGTAG; the words that are addresses below */
            uintptr_t *w = e->dst;
            for (uint32_t c = 0; c < e->size / 8; c++) {
                uint32_t w0 = port_rd32(s + 8 * c), w1 = port_rd32(s + 8 * c + 4);
                uint32_t op = w0 >> 24;
                if ((op == 0x01 || op == 0x03 || op == 0x04 || op == 0x06) && (w1 >> 24) & 15)
                    w1 |= 1;
                w[2 * c] = w0;
                w[2 * c + 1] = w1;
            }
            break;
        }
        default:
            host_fatal("port_fill[%u]: kind %u", k, e->kind);
        }
    }
    for (unsigned k = 0; k < port_fill_relocs_n; k++) {
        const PortFillReloc *r = &port_fill_relocs[k];
        ((uintptr_t *)port_fill[r->entry].dst)[2 * r->cmd + 1] = (uintptr_t)r->to;
    }
    if (host_verbose)
        host_log("the ROM's assets: %u in %u bytes, %s, %.1f ms\n", port_fill_n, bytes,
                 own ? "from the ROM" : "from the pack", host_perf_now() - t0);
    free(own);
}
