/*
 * Resource packs (docs/PORT.md, "Resource packs"; the format in
 * docs/ASSETS.md, "The pack"): the port takes a zip of the ROM's assets as
 * editable files (port/make_pack.py makes one from a ROM) and builds the ROM
 * image from it at startup, as tools/assets.py build does, so the rest of
 * the port reads the image as it reads a ROM.  A pack from an unmodified ROM
 * gives back the ROM byte for byte: the gzip members through gzip 1.2.4's
 * own deflate (third_party/gzip-1.2.4), the LZSS through Nelson's encoder,
 * the textures from the ROM's streams.
 *
 * Every segment goes where the port's link expects it (romtab.h, from the
 * link map), so an edited one has to fit where the original was: a gzip
 * member that grows is deflated at -9 before giving up.  An edited texture
 * doesn't need to: the ROM's stream stays in the image, the game decodes it
 * as it always did, and host_tex_decoded() puts the PNG's texels over what
 * it decoded, so the game takes the same time and only the picture changes.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "pack.h"
#include "romclass.h"

#include "romtab.h"

#include "../third_party/gzip-1.2.4/gzip124.h"
#include "../third_party/stb/stb_image.h"

#ifndef PORT_VERSION_NAME
#define PORT_VERSION_NAME "us.v11"
#endif

/* ---- errors and buffers ---------------------------------------------------- */

static const char *pack_path;

static void __attribute__((noreturn, format(printf, 1, 2))) die(const char *fmt, ...) {
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    host_fatal("pack %s: %s", pack_path, msg);
}

typedef struct {
    uint8_t *p;
    size_t n, cap;
} buf;

static void bput(buf *b, const void *p, size_t n) {
    if (b->n + n > b->cap) {
        b->cap = (b->n + n) * 2 + 256;
        b->p = realloc(b->p, b->cap);
    }
    if (n)
        memcpy(b->p + b->n, p, n);
    b->n += n;
}

static void bbyte(buf *b, unsigned v) {
    uint8_t c = (uint8_t)v;
    bput(b, &c, 1);
}

static void bzero_(buf *b, size_t n) {
    while (n--)
        bbyte(b, 0);
}

static void be16(uint8_t *p, unsigned v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t rbe16(const uint8_t *p) { return (uint32_t)p[0] << 8 | p[1]; }

/* ---- the pack's files ------------------------------------------------------ */

static pack_files *files;

static uint8_t *must_read(const char *name, size_t *len) {
    uint8_t *b = pack_read(files, name, len);
    if (!b)
        die("%s is missing (or the zip is damaged there)", name);
    return b;
}

static ynode *read_yaml(const char *name) {
    size_t len;
    char err[256];
    uint8_t *t = must_read(name, &len);
    ynode *y = yaml_parse((const char *)t, len, err, sizeof err);
    free(t);
    if (!y)
        die("%s: %s", name, err);
    return y;
}

static char *join(const char *dir, const char *name) {
    size_t a = strlen(dir), b = strlen(name);
    char *s = malloc(a + b + 2);
    memcpy(s, dir, a);
    if (a && dir[a - 1] != '/')
        s[a++] = '/';
    memcpy(s + a, name, b + 1);
    return s;
}

static const char *str_of(const ynode *n, const char *what) {
    if (!n || n->type != Y_SCALAR)
        die("%s: expected a string", what);
    return n->str;
}

static long long int_of(const ynode *n, const char *what) {
    long long v;
    if (!yint(n, &v))
        die("%s: expected a number (line %d)", what, n ? n->line + 1 : 0);
    return v;
}

static uint8_t *hex_of(const char *s, size_t *len, const char *what) {
    size_t n = 0, cap = strlen(s) / 2 + 1;
    uint8_t *b = malloc(cap);
    int half = -1;
    for (; *s; s++) {
        int v;
        if (*s == ' ' || *s == '\n' || *s == '\t' || *s == '\r')
            continue;
        if (*s >= '0' && *s <= '9')
            v = *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            v = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            v = *s - 'A' + 10;
        else
            die("%s: '%c' in hex", what, *s);
        if (half < 0) {
            half = v;
        } else {
            b[n++] = (uint8_t)(half << 4 | v);
            half = -1;
        }
    }
    if (half >= 0)
        die("%s: an odd number of hex digits", what);
    *len = n;
    return b;
}

/* ---- the link's segments (romtab.h) ------------------------------------------ */

#define NSEG ((int)(sizeof rom_segments / sizeof rom_segments[0]))

/* a layout name as a symbol: what isn't an identifier character becomes '_',
   and a leading digit gets one before it (rom_syms.py, gen_romtab.py) */
static const struct rom_segment *seg_named(const char *name) {
    char sym[128];
    size_t k = 0;
    if (name[0] >= '0' && name[0] <= '9')
        sym[k++] = '_';
    for (const char *c = name; *c && k + 1 < sizeof sym; c++)
        sym[k++] = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') ? *c : '_';
    sym[k] = 0;
    for (int i = 0; i < NSEG; i++)
        if (strcmp(rom_segments[i].name, sym) == 0)
            return &rom_segments[i];
    return NULL;
}

/* ---- PNGs as N64 texels (tools/assetlib/texel.py) ---------------------------- */

enum { F_RGBA16, F_RGBA32, F_IA16, F_IA8 };
static const int fmt_bpp[] = {2, 4, 2, 1};

/* An image's texels in `fmt`: exactly what texel.read_png makes of an 8-bit
   PNG.  One k times as wide and high (k = 2, 3, ...) is a higher-resolution
   replacement: averaged down by k x k boxes for the game, and kept for the
   renderer (*hires; docs/PORT.md, "Resource packs"). */
static uint8_t *png_texels(const char *name, int fmt, int want_w, int want_h, uint8_t **hires, int *hires_k) {
    size_t len;
    uint8_t *f = must_read(name, &len);
    int w, h, comp;
    uint8_t *px = stbi_load_from_memory(f, (int)len, &w, &h, &comp, 4);
    free(f);
    if (!px)
        die("%s: not a PNG this port reads", name);
    int k = 1;
    if (w != want_w || h != want_h) {
        if (want_w > 0 && want_h > 0 && w % want_w == 0 && h % want_h == 0 && w / want_w == h / want_h &&
            w / want_w >= 2) {
            k = w / want_w;
        } else {
            stbi_image_free(px);
            die("%s: %dx%d, textures.yaml has %dx%d (or a multiple, for a higher resolution)", name, w, h,
                want_w, want_h);
        }
    }
    uint8_t *rgba = px;
    if (k > 1) {
        rgba = malloc((size_t)want_w * want_h * 4);
        for (int y = 0; y < want_h; y++)
            for (int x = 0; x < want_w; x++)
                for (int c = 0; c < 4; c++) {
                    unsigned sum = 0;
                    for (int dy = 0; dy < k; dy++)
                        for (int dx = 0; dx < k; dx++)
                            sum += px[(((size_t)(y * k + dy) * w) + (size_t)(x * k + dx)) * 4 + c];
                    rgba[((size_t)y * want_w + x) * 4 + c] = (uint8_t)((sum + (unsigned)(k * k) / 2) / (unsigned)(k * k));
                }
        if (hires) {
            *hires = px;
            *hires_k = k;
            px = NULL;
        }
    } else if (hires) {
        *hires = NULL;
        *hires_k = 1;
    }
    size_t n = (size_t)want_w * want_h;
    uint8_t *out = malloc(n * fmt_bpp[fmt] + 1);
    for (size_t i = 0; i < n; i++) {
        const uint8_t *s = rgba + i * 4;
        unsigned r = s[0], g = s[1], b = s[2], a = s[3];
        unsigned grey = (r == g && g == b) ? r : (r + g + b) / 3;
        switch (fmt) {
        case F_RGBA16:
            be16(out + i * 2, (r >> 3) << 11 | (g >> 3) << 6 | (b >> 3) << 1 | (a >= 128));
            break;
        case F_RGBA32:
            memcpy(out + i * 4, s, 4);
            break;
        case F_IA16:
            out[i * 2] = (uint8_t)grey;
            out[i * 2 + 1] = (uint8_t)a;
            break;
        case F_IA8:
            out[i] = (uint8_t)((grey >> 4) << 4 | (a >> 4));
            break;
        }
    }
    if (rgba != px)
        free(rgba);
    stbi_image_free(px);
    return out;
}

/* ---- Rare's texture compression (tools/assetlib/blast.py) --------------------- */

static const int blast_elem[] = {0, 2, 4, 2, 4, 4, 2};
static const int blast_fmt[] = {0, F_RGBA16, F_RGBA32, F_IA16, F_RGBA16, F_RGBA32, F_IA8};

static void blast_literal(int t, unsigned code, const uint8_t *lut, size_t lut_len, buf *out) {
    uint8_t b[4];
    switch (t) {
    case 1:
        be16(b, ((code & 0xFFC0) << 1) | (code & 0x3F));
        bput(out, b, 2);
        break;
    case 2:
        be32(b, ((code & 0x7800) << 17) | ((code & 0x780) << 13) | ((code & 0x78) << 9) | ((code & 0x7) << 5));
        bput(out, b, 4);
        break;
    case 3:
        b[0] = (uint8_t)((code >> 8) << 1);
        b[1] = (uint8_t)((code & 0xFF) << 1);
        bput(out, b, 2);
        break;
    case 4: {
        unsigned hi = code >> 8, ia = hi & 0xFE, ib = code & 0xFE;
        unsigned ea = ia + 1 < lut_len ? rbe16(lut + ia) : 0, eb = ib + 1 < lut_len ? rbe16(lut + ib) : 0;
        be16(b, ((ea << 1) | (hi & 1)) & 0xFFFF);
        be16(b + 2, ((eb << 1) | (code & 1)) & 0xFFFF);
        bput(out, b, 4);
        break;
    }
    case 5: {
        unsigned i = (code >> 4) << 1, e = i + 1 < lut_len ? rbe16(lut + i) : 0;
        be32(b, ((e & 0x7C00) << 17) | ((e & 0x3E0) << 14) | ((e & 0x1F) << 11) | ((code & 0xF) << 4));
        bput(out, b, 4);
        break;
    }
    case 6: {
        unsigned hi = code >> 8, lo = code & 0xFF;
        b[0] = (uint8_t)(((hi & 0x38) << 2) | ((hi & 7) << 1));
        b[1] = (uint8_t)(((lo & 0x38) << 2) | ((lo & 7) << 1));
        bput(out, b, 2);
        break;
    }
    }
}

static int blast_decode(int t, const uint8_t *s, size_t n, const uint8_t *lut, size_t lut_len, buf *out) {
    int es = blast_elem[t];
    for (size_t k = 0; k + 1 < n; k += 2) {
        unsigned code = rbe16(s + k);
        if (code & 0x8000) {
            size_t len = (size_t)(code & 0x1F) * es;
            size_t off = es == 2 ? (code & 0x7FFF) >> 5 : (code & 0x7FE0) >> 4;
            if (off > out->n)
                return 0;
            size_t from = out->n - off;
            for (size_t i = 0; i < len; i++)
                bbyte(out, out->p[from + i]);
        } else {
            blast_literal(t, code, lut, lut_len, out);
        }
    }
    return 1;
}

/* the literal code of each unit of raw (quantizing what the format can't
   hold; types 4 and 5 by the nearest colour in the LUT) */
static unsigned *blast_literals(int t, const uint8_t *raw, size_t n, const uint8_t *lut, size_t lut_len,
                                size_t *count) {
    int es = blast_elem[t];
    size_t m = n / es;
    unsigned *codes = malloc(sizeof *codes * (m + 1));
    int nl = (int)(lut_len / 2);
    for (size_t i = 0; i < m; i++) {
        const uint8_t *u = raw + i * es;
        switch (t) {
        case 1: {
            unsigned v = rbe16(u);
            codes[i] = ((v >> 1) & 0x7FC0) | (v & 0x3F);
            break;
        }
        case 2:
            codes[i] = (unsigned)(u[0] >> 4) << 11 | (unsigned)(u[1] >> 4) << 7 | (unsigned)(u[2] >> 4) << 3 | u[3] >> 5;
            break;
        case 3:
            codes[i] = (unsigned)(u[0] >> 1) << 8 | (u[1] >> 1);
            break;
        case 6: {
            unsigned a = (((u[0] >> 5) & 7) << 3) | ((u[0] >> 1) & 7), b = (((u[1] >> 5) & 7) << 3) | ((u[1] >> 1) & 7);
            codes[i] = a << 8 | b;
            break;
        }
        case 4:
        case 5: {
            int lim = t == 4 ? (nl < 128 ? nl : 128) : (nl < 0x800 ? nl : 0x800);
            unsigned idx[2];
            int parts = t == 4 ? 2 : 1;
            for (int q = 0; q < parts; q++) {
                int r, g, b;
                if (t == 4) {
                    unsigned v = rbe16(u + q * 2);
                    r = (v >> 11) & 31, g = (v >> 6) & 31, b = (v >> 1) & 31;
                } else {
                    r = u[0] >> 3, g = u[1] >> 3, b = u[2] >> 3;
                }
                int best = 0, bd = 1 << 30;
                for (int e = 0; e < lim; e++) {
                    unsigned c = rbe16(lut + e * 2);
                    int dr = (int)((c >> 10) & 31) - r, dg = (int)((c >> 5) & 31) - g, db = (int)(c & 31) - b;
                    int d = dr * dr + dg * dg + db * db;
                    if (d < bd) {
                        bd = d;
                        best = e;
                        if (!d)
                            break;
                    }
                }
                idx[q] = (unsigned)best;
            }
            if (t == 4) {
                unsigned a = rbe16(u), b = rbe16(u + 2);
                codes[i] = idx[0] << 9 | (a & 1) << 8 | idx[1] << 1 | (b & 1);
            } else {
                codes[i] = idx[0] << 4 | (u[3] >> 4);
            }
            break;
        }
        }
    }
    *count = m;
    return codes;
}

/* a stream that decodes to raw as the format quantizes it: greedy, the
   farthest match first (blast.encode; not Rare's encoder, which can't be
   reproduced, docs/ASSETS.md) */
static void blast_encode(int t, const uint8_t *raw, size_t n, const uint8_t *lut, size_t lut_len, buf *out) {
    size_t m;
    unsigned *codes = blast_literals(t, raw, n, lut, lut_len, &m);
    int es = blast_elem[t];
    buf units = {0};
    for (size_t i = 0; i < m; i++)
        blast_literal(t, codes[i], lut, lut_len, &units);
    size_t r = 0;
    while (r < m) {
        size_t best = 0, bd = 0;
        for (size_t d = r < 511 ? r : 511; d > 0; d--) {
            size_t j = 0;
            while (j < 31 && r + j < m && memcmp(units.p + (r - d + j) * es, units.p + (r + j) * es, (size_t)es) == 0)
                j++;
            if (j > best) {
                best = j;
                bd = d;
                if (j == 31)
                    break;
            }
        }
        uint8_t b[2];
        if (best >= 2) {
            unsigned off = (unsigned)(bd * es);
            be16(b, 0x8000 | (es == 2 ? off << 5 : off << 4) | (unsigned)best);
            r += best;
        } else {
            be16(b, codes[r]);
            r++;
        }
        bput(out, b, 2);
    }
    free(units.p);
    free(codes);
}

/* ---- texture overrides ------------------------------------------------------ */

#define NTEX 4096
static struct {
    uint8_t *texels;        /* what the game's decode is replaced with */
    uint32_t len;
    uint8_t *hires;         /* RGBA8, hires_k times the size (for later) */
    int hires_k;
} tex_override[NTEX];
static int tex_overrides, tex_warned;

void host_tex_decoded(uint32_t id, uint32_t dst, uint32_t size) {
    if (id >= NTEX || !tex_override[id].texels)
        return;
    if (size != tex_override[id].len) {
        if (!tex_warned++)
            host_log("pack: texture %03X decoded to %u bytes, the pack's has %u: not replaced\n", id, size,
                     tex_override[id].len);
        return;
    }
    memcpy(port_ptr(dst), tex_override[id].texels, size);
}

/* the decode queue's textures, by slot (60F60's D_803C4250: 144) */
static uint16_t tex_slot_id[144];

void host_tex_queued(uint32_t slot, uint32_t id) {
    if (slot < 144)
        tex_slot_id[slot] = (uint16_t)(id + 1);
}

void host_tex_decoded_slot(uint32_t slot, uint32_t dst, uint32_t size) {
    if (slot < 144 && tex_slot_id[slot])
        host_tex_decoded(tex_slot_id[slot] - 1u, dst, size);
}

/* ---- textures (tools/assetlib/textures.py) ------------------------------------ */

#define TABLE_SIZE 0x8000

static void build_textures(uint32_t blob_start, uint32_t table_start, buf *table, buf *blob, int *changed) {
    ynode *items = read_yaml("textures/textures.yaml");
    if (items->type != Y_SEQ || items->n != NTEX)
        die("textures/textures.yaml: %d entries, want %d", items->type == Y_SEQ ? items->n : 0, NTEX);
    static uint8_t *built[NTEX];
    static size_t built_len[NTEX];
    uint32_t pos = blob_start;
    int prev = -1;
    for (int n = 0; n < NTEX; n++) {
        ynode *it = items->items[n];
        char what[64];
        snprintf(what, sizeof what, "textures.yaml entry %03X", n);
        if (int_of(ymap(it, "i"), what) != n)
            die("%s is numbered otherwise", what);
        uint32_t a = (pos + 7) & ~7u;
        uint8_t ent[8];
        ynode *slot = ymap(it, "slot");
        if (slot) {
            if (slot->type != Y_SEQ || slot->n != 2)
                die("%s: slot: [length, type]", what);
            be32(ent, a - table_start);
            be16(ent + 4, (unsigned)int_of(slot->items[0], what));
            be16(ent + 6, (unsigned)int_of(slot->items[1], what));
            bput(table, ent, 8);
            continue;
        }
        int t = (int)int_of(ymap(it, "type"), what);
        if (t < 0 || t > 6)
            die("%s: type %d", what, t);
        char name[16], fn[64];
        uint8_t *data;
        size_t dlen;
        snprintf(name, sizeof name, "%03X", n);
        if (t == 0) {
            snprintf(fn, sizeof fn, "textures/%s.bin", name);
            data = must_read(fn, &dlen);
        } else {
            const uint8_t *lut = NULL;
            size_t lut_len = 0;
            ynode *l = ymap(it, "lut");
            if (l) {
                long long j = int_of(l, what);
                if (j < 0 || j >= n || !built[j])
                    die("%s: lut 0x%llX isn't an earlier texture", what, j);
                lut = built[j];
                lut_len = built_len[j];
            } else if (t == 4 || t == 5) {
                die("%s: type %d needs a lut", what, t);
            }
            int fmt = blast_fmt[t];
            buf raw = {0};
            ynode *plan = ymap(it, "png");
            if (!plan || plan->type != Y_SEQ)
                die("%s: png: [[w, h], ...]", what);
            uint8_t *hires = NULL;
            int hires_k = 1;
            for (int k = 0; k < plan->n; k++) {
                ynode *wh = plan->items[k];
                if (wh->type != Y_SEQ || wh->n != 2)
                    die("%s: png: [[w, h], ...]", what);
                if (plan->n == 1)
                    snprintf(fn, sizeof fn, "textures/%s.png", name);
                else
                    snprintf(fn, sizeof fn, "textures/%s.%d.png", name, k);
                uint8_t *hr = NULL;
                int hk = 1;
                int w = (int)int_of(wh->items[0], what), h = (int)int_of(wh->items[1], what);
                uint8_t *px = png_texels(fn, fmt, w, h, &hr, &hk);
                bput(&raw, px, (size_t)w * h * fmt_bpp[fmt]);
                free(px);
                if (k == 0) {
                    hires = hr;
                    hires_k = hk;
                } else {
                    stbi_image_free(hr);
                }
            }
            snprintf(fn, sizeof fn, "textures/%s.rest.bin", name);
            if (pack_has(files, fn)) {
                size_t rl;
                uint8_t *rest = must_read(fn, &rl);
                bput(&raw, rest, rl);
                free(rest);
            }
            snprintf(fn, sizeof fn, "textures/%s.blast", name);
            data = pack_has(files, fn) ? must_read(fn, &dlen) : NULL;
            buf dec = {0};
            int ok = data && blast_decode(t, data, dlen, lut, lut_len, &dec);
            if (ok && dec.n == raw.n && memcmp(dec.p, raw.p, raw.n) == 0) {
                /* the ROM's stream, as it is */
                free(raw.p);
                stbi_image_free(hires);
            } else if (ok && dec.n == raw.n) {
                /* edited: the ROM's stream, and the texels over what it decodes to */
                tex_override[n].texels = raw.p;
                tex_override[n].len = (uint32_t)raw.n;
                tex_override[n].hires = hires;
                tex_override[n].hires_k = hires_k;
                tex_overrides++;
            } else {
                /* no stream (or another size): compressed here */
                buf enc = {0};
                free(data);
                blast_encode(t, raw.p, raw.n, lut, lut_len, &enc);
                data = enc.p;
                dlen = enc.n;
                free(raw.p);
                stbi_image_free(hires);
                (*changed)++;
            }
            free(dec.p);
        }
        built[n] = data;
        built_len[n] = dlen;
        uint32_t gap = a - pos;
        if (gap) {
            size_t pl = 0;
            uint8_t *pad = NULL;
            ynode *pn = prev >= 0 ? ymap(items->items[prev], "pad") : NULL;
            if (pn)
                pad = hex_of(str_of(pn, what), &pl, what);
            if (pad && pl == gap)
                bput(blob, pad, gap);
            else
                bzero_(blob, gap);
            free(pad);
        }
        be32(ent, a - table_start);
        be16(ent + 4, (unsigned)dlen);
        be16(ent + 6, (unsigned)t);
        if (dlen > 0xFFFF)
            die("%s: %zu bytes, more than a texture can have", what, dlen);
        bput(table, ent, 8);
        bput(blob, data, dlen);
        pos = a + (uint32_t)dlen;
        prev = n;
    }
    for (int n = 0; n < NTEX; n++) {
        free(built[n]);
        built[n] = NULL;
    }
    yaml_free(items);
}

/* ---- Rare's LZSS: Mark Nelson's, BREAK_EVEN 2 (tools/assetlib/lzss.py) -------- */

typedef struct {
    int W, M, LA;
    uint8_t *window;
    int *parent, *small, *large;
    buf out;
    unsigned mask, rack;
} lzss;

static void lz_put(lzss *z, unsigned v, int k) {
    for (unsigned b = 1u << (k - 1); b; b >>= 1) {
        if (v & b)
            z->rack |= z->mask;
        z->mask >>= 1;
        if (!z->mask) {
            bbyte(&z->out, z->rack);
            z->rack = 0;
            z->mask = 0x80;
        }
    }
}

static void lz_contract(lzss *z, int old, int new_) {
    z->parent[new_] = z->parent[old];
    int p = z->parent[old];
    if (z->large[p] == old)
        z->large[p] = new_;
    else
        z->small[p] = new_;
    z->parent[old] = 0;
}

static void lz_replace(lzss *z, int old, int new_) {
    int p = z->parent[old];
    if (z->small[p] == old)
        z->small[p] = new_;
    else
        z->large[p] = new_;
    z->parent[new_] = z->parent[old];
    z->small[new_] = z->small[old];
    z->large[new_] = z->large[old];
    z->parent[z->small[new_]] = new_;
    z->parent[z->large[new_]] = new_;
    z->parent[old] = 0;
}

static void lz_delete(lzss *z, int p) {
    if (z->parent[p] == 0)
        return;
    if (z->large[p] == 0) {
        lz_contract(z, p, z->small[p]);
    } else if (z->small[p] == 0) {
        lz_contract(z, p, z->large[p]);
    } else {
        int r = z->small[p];
        while (z->large[r] != 0)
            r = z->large[r];
        lz_delete(z, r);
        lz_replace(z, p, r);
    }
}

static int lz_add(lzss *z, int new_, int *mp) {
    int t = z->large[z->W], ml = 0;
    *mp = 0;
    for (;;) {
        int i = 0, delta = 0;
        while (i < z->LA) {
            delta = (int)z->window[(new_ + i) & z->M] - (int)z->window[(t + i) & z->M];
            if (delta)
                break;
            i++;
        }
        if (i >= ml) {
            ml = i;
            *mp = t;
            if (ml >= z->LA) {
                lz_replace(z, t, new_);
                return ml;
            }
        }
        int *child = delta >= 0 ? z->large : z->small;
        if (child[t] == 0) {
            child[t] = new_;
            z->parent[new_] = t;
            z->large[new_] = z->small[new_] = 0;
            return ml;
        }
        t = child[t];
    }
}

static void lzss_encode(const uint8_t *data, size_t len, int bits, buf *out) {
    const int break_even = 2;
    lzss z = {0};
    z.W = 1 << bits;
    z.M = z.W - 1;
    z.LA = (1 << (16 - bits)) + break_even;
    z.window = calloc((size_t)z.W, 1);
    z.parent = calloc((size_t)z.W + 1, sizeof(int));
    z.small = calloc((size_t)z.W + 1, sizeof(int));
    z.large = calloc((size_t)z.W + 1, sizeof(int));
    z.mask = 0x80;
    int cur = 1;
    size_t n = len < (size_t)z.LA ? len : (size_t)z.LA;
    memcpy(z.window + cur, data, n);
    size_t pos = n;
    int look_ahead = (int)n;
    z.large[z.W] = cur;
    z.parent[cur] = z.W;
    int ml = 0, mp = 0;
    while (look_ahead > 0) {
        int count;
        if (ml > look_ahead)
            ml = look_ahead;
        if (ml <= break_even) {
            count = 1;
            lz_put(&z, 1, 1);
            lz_put(&z, z.window[cur], 8);
        } else {
            lz_put(&z, 0, 1);
            lz_put(&z, (unsigned)mp, bits);
            lz_put(&z, (unsigned)(ml - (break_even + 1)), 16 - bits);
            count = ml;
        }
        for (int k = 0; k < count; k++) {
            lz_delete(&z, (cur + z.LA) & z.M);
            if (pos >= len)
                look_ahead--;
            else
                z.window[(cur + z.LA) & z.M] = data[pos++];
            cur = (cur + 1) & z.M;
            if (look_ahead) {
                if (cur == 0)
                    ml = 0;
                else
                    ml = lz_add(&z, cur, &mp);
            }
        }
    }
    lz_put(&z, 0, 1);
    lz_put(&z, 0, bits);
    if (z.mask != 0x80)
        bbyte(&z.out, z.rack);
    if (z.out.n & 1)
        bbyte(&z.out, 0);
    *out = z.out;
    free(z.window);
    free(z.parent);
    free(z.small);
    free(z.large);
}

/* ---- gzip members (tools/assetlib/gz.py) ------------------------------------- */

/* Rare's header (FNAME, the name and mtime, OS 3) and gzip 1.2.4's deflate at
   `level` */
static void gzip_member(const uint8_t *data, size_t len, const char *gzname, uint32_t mtime, int level, buf *out) {
    uint8_t h[10] = {0x1F, 0x8B, 8, 8, 0, 0, 0, 0, 0, 3};
    h[4] = (uint8_t)mtime;
    h[5] = (uint8_t)(mtime >> 8);
    h[6] = (uint8_t)(mtime >> 16);
    h[7] = (uint8_t)(mtime >> 24);
    bput(out, h, 10);
    bput(out, gzname, strlen(gzname) + 1);
    size_t n;
    uint8_t *body = gzip124_compress(data, len, level, &n);
    if (!body)
        die("%s: deflate failed", gzname);
    bput(out, body, n);
    free(body);
}

/* ---- levels (tools/assetlib/level.py) ----------------------------------------- */

static const struct {
    const char *name;
    int hoff;
    const char *rec;        /* a record's struct format, or NULL */
    const char *group;      /* groups of these, or NULL */
} level_sections[] = {
    {"ammoBoxes", 0x20, "hhhh", NULL},
    {"collisionFixes", 0x24, "hhhhhhhhhh", NULL},
    {"commPoint", 0x28, "hhhh", NULL},
    {"animTextures", 0x2C, NULL, NULL},
    {"terrain", 0x30, NULL, "hhhhhhhhhBB"},
    {"rdus", 0x34, "hhh", NULL},
    {"tntCrates", 0x38, "hhhBBHH", NULL},
    {"blocks", 0x3C, NULL, NULL},
    {"bounds40", 0x40, "HHHHH", NULL},
    {"bounds44", 0x44, "HHHHH", NULL},
    {"unk48", 0x48, "I", NULL},
    {"levelBounds", 0x4C, "HHHH", NULL},
    {"vehicles", 0x50, "Bhhhh", NULL},
    {"carrier", 0x54, "BhhhhB", NULL},
    {"unk58", 0x58, "hhhh", NULL},
    {"buildings", 0x5C, "hhhHBBHH", NULL},
    {"unk60", 0x60, NULL, NULL},
    {"unk64", 0x64, NULL, NULL},
    {"trainStops", 0x68, NULL, NULL},
    {"collisionXZ", 0x6C, NULL, "hhhhhhhhhHBB"},
    {"playerCollisionXZ", 0x70, NULL, "hhhhhhhhhHBB"},
    {"unk74", 0x74, NULL, NULL},
    {"unkA0_0", 0xA0, NULL, NULL},
    {"unkA0_1", 0xA4, NULL, NULL},
    {"unkA0_2", 0xA8, NULL, NULL},
    {"unkA0_3", 0xAC, NULL, NULL},
    {"unkA0_4", 0xB0, NULL, NULL},
    {"unkA0_5", 0xB4, NULL, NULL},
    {"unkA0_6", 0xB8, NULL, NULL},
    {"unkA0_7", 0xBC, NULL, NULL},
    {"unkA0_8", 0xC0, NULL, NULL},
};
#define NSECTIONS ((int)(sizeof level_sections / sizeof level_sections[0]))

static const struct {
    const char *name, *fmt;
    int off;
} level_header[] = {
    {"unk0", "HH", 0x0},    {"unk4", "HH", 0x4},    {"unk8", "HH", 0x8},
    {"unkC", "HH", 0xC},    {"unk10", "HH", 0x10},  {"unk14", "HH", 0x14},
    {"gravity", "i", 0x18}, {"unk1C", "I", 0x1C},   {"unkC4", "I", 0xC4},
};

static int fmt_size(const char *f) {
    int n = 0;
    for (; *f; f++)
        n += *f == 'b' || *f == 'B' ? 1 : *f == 'h' || *f == 'H' ? 2 : 4;
    return n;
}

/* struct.pack(">" + fmt, *values), with its range checks */
static void pack_fields(const char *fmt, const ynode *vals, uint8_t *out, const char *what) {
    int nf = (int)strlen(fmt);
    if (vals->type != Y_SEQ || vals->n != nf)
        die("%s (line %d): %d numbers, want %d", what, vals->line + 1, vals->type == Y_SEQ ? vals->n : 1, nf);
    for (int k = 0; k < nf; k++) {
        long long v = int_of(vals->items[k], what), lo, hi;
        int w;
        switch (fmt[k]) {
        case 'b': lo = -128, hi = 127, w = 1; break;
        case 'B': lo = 0, hi = 255, w = 1; break;
        case 'h': lo = -32768, hi = 32767, w = 2; break;
        case 'H': lo = 0, hi = 65535, w = 2; break;
        case 'i': lo = -2147483648LL, hi = 2147483647LL, w = 4; break;
        default: lo = 0, hi = 4294967295LL, w = 4; break;
        }
        if (v < lo || v > hi)
            die("%s (line %d): %lld is out of range (%lld to %lld)", what, vals->items[k]->line + 1, v, lo, hi);
        for (int b = w - 1; b >= 0; b--)
            *out++ = (uint8_t)((unsigned long long)v >> (8 * b));
    }
}

static void build_level(const char *path, buf *out) {
    ynode *y = read_yaml(path);
    const char *slash = strrchr(path, '/');
    char dir[256];
    snprintf(dir, sizeof dir, "%.*s", slash ? (int)(slash - path) : 0, path);
    char *disp_name = join(dir, str_of(ymap(y, "display"), path));
    size_t disp_len;
    uint8_t *disp = must_read(disp_name, &disp_len);
    free(disp_name);
    ynode *secs = ymap(y, "sections"), *head = ymap(y, "header");
    if (!secs || secs->type != Y_MAP || !head || head->type != Y_MAP)
        die("%s: needs header: and sections:", path);
    buf body[NSECTIONS];
    memset(body, 0, sizeof body);
    for (int s = 0; s < NSECTIONS; s++) {
        char what[160];
        snprintf(what, sizeof what, "%s: %s", path, level_sections[s].name);
        ynode *sec = ymap(secs, level_sections[s].name), *v;
        if (!sec)
            die("%s is missing", what);
        if ((v = ymap(sec, "hex"))) {
            size_t n;
            uint8_t *b = ynull(v) && !v->quoted ? calloc(1, 1) : hex_of(str_of(v, what), &n, what);
            if (ynull(v) && !v->quoted)
                n = 0;
            bput(&body[s], b, n);
            free(b);
        } else if ((v = ymap(sec, "records"))) {
            const char *fmt = level_sections[s].rec;
            if (!fmt)
                die("%s: records aren't known here (hex)", what);
            if (v->type != Y_SEQ && !ynull(v))
                die("%s: records: a list", what);
            int sz = fmt_size(fmt);
            for (int r = 0; v->type == Y_SEQ && r < v->n; r++) {
                uint8_t rec[64];
                pack_fields(fmt, v->items[r], rec, what);
                bput(&body[s], rec, (size_t)sz);
            }
            ynode *tail = ymap(sec, "tail");
            if (tail && !ynull(tail)) {
                size_t n;
                uint8_t *b = hex_of(str_of(tail, what), &n, what);
                bput(&body[s], b, n);
                free(b);
            }
        } else if ((v = ymap(sec, "groups"))) {
            const char *fmt = level_sections[s].group;
            if (!fmt)
                die("%s: groups aren't known here (hex)", what);
            if (v->type != Y_SEQ && !ynull(v))
                die("%s: groups: a list", what);
            int sz = fmt_size(fmt);
            for (int g = 0; v->type == Y_SEQ && g < v->n; g++) {
                ynode *grp = v->items[g];
                int nr = grp->type == Y_SEQ ? grp->n : 0;
                if (grp->type != Y_SEQ && !ynull(grp))
                    die("%s (line %d): a group is a list of triangles", what, grp->line + 1);
                uint8_t e[4];
                be32(e, (uint32_t)(body[s].n + 4 + (size_t)sz * nr));
                bput(&body[s], e, 4);
                for (int r = 0; r < nr; r++) {
                    uint8_t rec[64];
                    pack_fields(fmt, grp->items[r], rec, what);
                    bput(&body[s], rec, (size_t)sz);
                }
            }
        } else {
            die("%s: needs hex:, records: or groups:", what);
        }
    }
    uint8_t h[0xC8] = {0};
    for (size_t k = 0; k < sizeof level_header / sizeof level_header[0]; k++) {
        char what[160];
        snprintf(what, sizeof what, "%s: header %s", path, level_header[k].name);
        ynode *v = ymap(head, level_header[k].name);
        if (!v)
            die("%s is missing", what);
        if (v->type == Y_SCALAR) {
            /* a single value */
            ynode one = {.type = Y_SEQ, .n = 1, .items = &v, .line = v->line};
            pack_fields(level_header[k].fmt, &one, h + level_header[k].off, what);
        } else {
            pack_fields(level_header[k].fmt, v, h + level_header[k].off, what);
        }
    }
    uint32_t pos = 0xC8 + (uint32_t)disp_len;
    for (int s = 0; s < NSECTIONS; s++) {
        be32(h + level_sections[s].hoff, pos);
        pos += (uint32_t)body[s].n;
    }
    ynode *dls = ymap(head, "displayLists");
    if (!dls || dls->type != Y_SEQ || dls->n != 10)
        die("%s: header displayLists: 10 numbers", path);
    for (int k = 0; k < 10; k++)
        be32(h + 0x78 + 4 * k, pos + (uint32_t)int_of(dls->items[k], path));
    bput(out, h, sizeof h);
    bput(out, disp, disp_len);
    for (int s = 0; s < NSECTIONS; s++) {
        bput(out, body[s].p, body[s].n);
        free(body[s].p);
    }
    free(disp);
    yaml_free(y);
}

/* ---- the sequence bank (tools/assetlib/audio.py) ------------------------------- */

static void build_sequences(const char *dir, buf *out) {
    char *yn = join(dir, "sequences.yaml");
    ynode *items = read_yaml(yn);
    if (items->type != Y_SEQ)
        die("%s: a list", yn);
    int n = items->n;
    buf head = {0}, body = {0};
    uint8_t h[8];
    be16(h, 0x5331);
    be16(h + 2, (unsigned)n);
    bput(&head, h, 4);
    uint32_t pos = 4 + 8 * (uint32_t)n;
    for (int k = 0; k < n; k++) {
        char *fn = join(dir, str_of(ymap(items->items[k], "file"), yn));
        size_t len;
        uint8_t *s = must_read(fn, &len);
        free(fn);
        be32(h, pos + (uint32_t)body.n);
        be32(h + 4, (uint32_t)len);
        bput(&head, h, 8);
        bput(&body, s, len);
        free(s);
        if (k + 1 < n) {
            size_t gap = (size_t)((4 - (pos + body.n) % 4) % 4), pl = 0;
            ynode *pn = ymap(items->items[k], "pad");
            uint8_t *pad = pn && !ynull(pn) ? hex_of(str_of(pn, yn), &pl, yn) : NULL;
            if (pad && pl == gap)
                bput(&body, pad, gap);
            else
                bzero_(&body, gap);
            free(pad);
        }
    }
    bput(out, head.p, head.n);
    bput(out, body.p, body.n);
    free(head.p);
    free(body.p);
    free(yn);
    yaml_free(items);
}

/* ---- the code modules' data (pack.yaml's modules: and data:) ------------------- */

#define RDRAM 0x400000u
static uint8_t *data_src;           /* the modules' data at their physical addresses */
static struct {
    char name[16];
    uint32_t base, size, text;
} modules[4];
static int nmodules;

const uint8_t *pack_data_source(void) { return data_src; }

static void build_data_source(ynode *pk) {
    ynode *mods = ymap(pk, "modules"), *data = ymap(pk, "data");
    if (!mods && !data)
        return;
    if (!mods || mods->type != Y_SEQ || !data || data->type != Y_SEQ || mods->n > 4)
        die("pack.yaml: modules: and data: are lists");
    for (int k = 0; k < mods->n; k++) {
        ynode *m = mods->items[k];
        snprintf(modules[k].name, sizeof modules[k].name, "%s", str_of(ymap(m, "name"), "modules: name"));
        modules[k].base = (uint32_t)int_of(ymap(m, "base"), "modules: base");
        modules[k].size = (uint32_t)int_of(ymap(m, "size"), "modules: size");
        modules[k].text = (uint32_t)int_of(ymap(m, "text"), "modules: text");
        if (modules[k].base + modules[k].size > RDRAM || modules[k].text > modules[k].size)
            die("pack.yaml: module %s doesn't fit", modules[k].name);
    }
    nmodules = mods->n;
    data_src = calloc(RDRAM, 1);
    for (int k = 0; k < data->n; k++) {
        ynode *d = data->items[k];
        const char *mod = str_of(ymap(d, "module"), "data: module");
        int m = 0;
        while (m < nmodules && strcmp(modules[m].name, mod))
            m++;
        if (m == nmodules)
            die("pack.yaml: data: no module %s", mod);
        uint32_t off = (uint32_t)int_of(ymap(d, "offset"), "data: offset");
        size_t len;
        const char *fn = str_of(ymap(d, "file"), "data: file");
        uint8_t *b = must_read(fn, &len);
        if (off + len > modules[m].size)
            die("%s: 0x%zX bytes at 0x%X, past %s's 0x%X", fn, len, off, mod, modules[m].size);
        memcpy(data_src + modules[m].base + off, b, len);
        free(b);
    }
}

/* Without the code, the front end's slot still has to hold what the game
   inflates there (port/src/overlay.c does, for the time it takes): its
   .text as zeros and its .data, each a gzip member, one after the other.
   Nothing reads what they inflate to (the port restores the front end's
   data itself), so only the time differs: less than the ROM's code takes. */
static void stand_in_front_end(uint8_t *rom) {
    const struct rom_segment *t = seg_named("hd_front_end_text." PORT_VERSION_NAME),
                             *d = seg_named("hd_front_end_data." PORT_VERSION_NAME);
    int m = 0;
    while (m < nmodules && strcmp(modules[m].name, "hd_front_end"))
        m++;
    if (!t || !d || m == nmodules)
        die("no hd_front_end to stand in for (pack.yaml's modules:)");
    uint8_t *zeros = calloc(modules[m].text, 1);
    buf out = {0};
    gzip_member(zeros, modules[m].text, "hd_front_end_text.raw", 0, 6, &out);
    gzip_member(data_src + modules[m].base + modules[m].text, modules[m].size - modules[m].text,
                "hd_front_end_data.raw", 0, 6, &out);
    free(zeros);
    if (out.n > d->end - t->start)
        die("the front end's stand-in doesn't fit");
    memcpy(rom + t->start, out.p, out.n);
    free(out.p);
}

/* ---- the ROM ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    const struct rom_segment *seg;
    uint32_t start, end;    /* where its data went */
} placed;

uint8_t *pack_build_rom(const char *path, uint32_t *size_out, int *edited) {
    char err[512];
    pack_path = path;
    files = pack_open(path, err, sizeof err);
    if (!files)
        host_fatal("%s", err);

    ynode *pk = read_yaml("pack.yaml");
    long long format = int_of(ymap(pk, "format"), "pack.yaml format");
    if (format != 1)
        die("pack.yaml: format %lld; this port reads format 1", format);
    const char *version = str_of(ymap(pk, "version"), "pack.yaml version");
    if (strcmp(version, PORT_VERSION_NAME) != 0)
        die("made from %s's ROM; this port is built for " PORT_VERSION_NAME, version);

    uint32_t size = rom_segments[NSEG - 1].end;
    uint8_t *rom = calloc(size, 1);
    placed *done = calloc((size_t)NSEG, sizeof *done);
    int ndone = 0, changed = 0;

    /* the pieces that aren't assets: as they are (pack.yaml's rom:) */
    ynode *codev = ymap(pk, "code");
    int have_code = !codev || (codev->type == Y_SCALAR && strcmp(codev->str, "no") != 0 &&
                               strcmp(codev->str, "false") != 0);
    build_data_source(pk);
    ynode *pieces = ymap(pk, "rom");
    if (!pieces || pieces->type != Y_SEQ)
        die("pack.yaml: rom: a list of {name, file}");
    for (int k = 0; k < pieces->n; k++) {
        const char *name = str_of(ymap(pieces->items[k], "name"), "pack.yaml rom: name");
        const struct rom_segment *s = seg_named(name);
        if (!s)
            die("pack.yaml: this port's link has no segment %s", name);
        ynode *fill = ymap(pieces->items[k], "fill");
        if (fill) {
            memset(rom + s->start, (int)int_of(fill, "pack.yaml fill"), s->end - s->start);
            continue;
        }
        if (!have_code)
            die("pack.yaml says code: no, but rom: lists %s", name);
        size_t len;
        uint8_t *d = must_read(str_of(ymap(pieces->items[k], "file"), "pack.yaml rom: file"), &len);
        if (len > s->end - s->start)
            die("%s: 0x%zX bytes, the link has 0x%X for it", name, len, s->end - s->start);
        memcpy(rom + s->start, d, len);
        free(d);
    }

    /* the assets (layout.yaml, tools/assets.py's build_all) */
    ynode *lay = read_yaml("layout.yaml");
    ynode *segs = ymap(lay, "segments");
    if (!segs || segs->type != Y_SEQ)
        die("layout.yaml: segments: a list");
    const char *lv = str_of(ymap(lay, "version"), "layout.yaml version");
    if (strcmp(lv, PORT_VERSION_NAME) != 0)
        die("layout.yaml is %s's", lv);
    /* Where each goes: a segment the code names (a level, a vehicle's model,
       an image, the tables...) where the port's link has it; the display
       lists after their files and the model table's models after each other,
       as the ROM has them, wherever that comes to (the level's DMA takes its
       display list from right after it; the model table is made from where
       the models went).  So a level and its display list share their room,
       and the models share theirs. */
    const struct rom_segment *ttable = NULL, *mtable = NULL;
    ynode *mtable_ent = NULL;
    buf tex_table = {0};
    int after_mtable = 0;
    uint32_t pos = 0;
    for (int k = 0; k < segs->n; k++) {
        ynode *ent = segs->items[k];
        const char *name = str_of(ymap(ent, "name"), "layout.yaml name");
        const char *kind = str_of(ymap(ent, "kind"), name);
        const struct rom_segment *s = seg_named(name);
        if (!s)
            die("layout.yaml: this port's link has no segment %s", name);
        size_t nl = strlen(name);
        int floating = (nl > 3 && !strcmp(name + nl - 3, "_dl")) || (after_mtable && !strcmp(kind, "gzip"));
        ynode *al = ymap(ent, "align");
        uint32_t align = al ? (uint32_t)int_of(al, name) : 1, start;
        if (!floating) {
            start = s->start;
            if (k && pos > start)
                die("%s (and what comes with it) ends at 0x%X, past %s at 0x%X: an edited asset has to fit "
                    "in the room the original had (docs/ASSETS.md, \"The pack\")",
                    done[ndone - 1].name, pos, name, start);
        } else {
            start = (pos + align - 1) & ~(align - 1);
        }
        if (k) {    /* the ROM's bytes between the last one and this, if the gap is still theirs */
            ynode *pn = ymap(segs->items[k - 1], "pad");
            uint32_t gap = start - pos;
            if (pn && gap) {
                size_t pl;
                uint8_t *pad = hex_of(str_of(pn, name), &pl, name);
                if (pl == gap)
                    memcpy(rom + pos, pad, pl);
                free(pad);
            }
        }
        buf d = {0};
        ynode *fn = ymap(ent, "file");
        if (!strcmp(kind, "copy") && !have_code && !pack_has(files, str_of(fn, name))) {
            /* init, without the code: nothing reads its slot (its data is in data/) */
            d.n = s->end - s->start;
            d.p = calloc(d.n, 1);
        } else if (!strcmp(kind, "copy") || !strcmp(kind, "raw")) {
            size_t len;
            d.p = must_read(str_of(fn, name), &len);
            d.n = d.cap = len;
        } else if (!strcmp(kind, "texture_table")) {
            ttable = s;
            bzero_(&d, TABLE_SIZE);
        } else if (!strcmp(kind, "model_table")) {
            mtable = s;
            mtable_ent = ent;
            after_mtable = 1;
            bzero_(&d, 0x800);
        } else if (!strcmp(kind, "textures")) {
            if (!ttable)
                die("layout.yaml: textures before texture_table");
            build_textures(start, ttable->start, &tex_table, &d, &changed);
        } else if (!strcmp(kind, "lzss") || !strcmp(kind, "lzss_image")) {
            int bits = (int)int_of(ymap(ent, "bits"), name);
            size_t len;
            uint8_t *raw;
            if (!strcmp(kind, "lzss")) {
                raw = must_read(str_of(fn, name), &len);
            } else {
                int w = (int)int_of(ymap(ent, "width"), name), h = (int)int_of(ymap(ent, "height"), name);
                raw = png_texels(str_of(fn, name), F_RGBA16, w, h, NULL, NULL);
                len = (size_t)w * h * 2;
            }
            lzss_encode(raw, len, bits, &d);
            free(raw);
        } else if (!strcmp(kind, "sequences")) {
            build_sequences(str_of(ymap(ent, "dir"), name), &d);
        } else if (!strcmp(kind, "gzip") || !strcmp(kind, "level")) {
            buf raw = {0};
            if (!strcmp(kind, "level")) {
                build_level(str_of(fn, name), &raw);
            } else {
                size_t len;
                raw.p = must_read(str_of(fn, name), &len);
                raw.n = len;
            }
            const char *gzname = str_of(ymap(ent, "gzname"), name);
            uint32_t mtime = (uint32_t)int_of(ymap(ent, "mtime"), name);
            gzip_member(raw.p, raw.n, gzname, mtime, 6, &d);
            if (d.n > s->end - s->start) {
                /* edited and grown: gzip -9 makes more room */
                free(d.p);
                memset(&d, 0, sizeof d);
                gzip_member(raw.p, raw.n, gzname, mtime, 9, &d);
                changed++;
            }
            free(raw.p);
        } else {
            die("layout.yaml: %s has an unknown kind %s", name, kind);
        }
        if (start + d.n > size)
            die("%s doesn't fit in the ROM", name);
        memcpy(rom + start, d.p, d.n);
        done[ndone].name = name;
        done[ndone].seg = s;
        done[ndone].start = start;
        done[ndone].end = start + (uint32_t)d.n;
        ndone++;
        pos = start + (uint32_t)d.n;
        free(d.p);
    }
    if (ndone) {    /* the last one: up to what follows the assets (the code modules) */
        const struct rom_segment *s = done[ndone - 1].seg;
        if (pos > s->end)
            die("%s ends at 0x%X, past the code modules at 0x%X", done[ndone - 1].name, pos, s->end);
        ynode *pn = ymap(segs->items[segs->n - 1], "pad");
        if (pn && s->end > pos) {
            size_t pl;
            uint8_t *pad = hex_of(str_of(pn, "pad"), &pl, "pad");
            if (pl == s->end - pos)
                memcpy(rom + pos, pad, pl);
            free(pad);
        }
    }
    if (ttable) {
        if (tex_table.n != TABLE_SIZE)
            die("the texture table came out 0x%zX bytes", tex_table.n);
        memcpy(rom + ttable->start, tex_table.p, TABLE_SIZE);
    }
    free(tex_table.p);
    if (mtable) {
        ynode *ents = ymap(mtable_ent, "entries");
        const char *end_of = str_of(ymap(mtable_ent, "end_of"), "model_table end_of");
        uint32_t end = 0;
        for (int k = 0; k < ndone; k++)
            if (!strcmp(done[k].name, end_of))
                end = done[k].end;
        if (!end || !ents || ents->type != Y_SEQ || ents->n != 512)
            die("layout.yaml: model_table needs end_of and 512 entries");
        for (int k = 0; k < 512; k++) {
            uint32_t at = end;
            if (!ynull(ents->items[k])) {
                const char *m = str_of(ents->items[k], "model_table");
                int j = 0;
                while (j < ndone && strcmp(done[j].name, m))
                    j++;
                if (j == ndone)
                    die("model_table: no segment %s", m);
                at = done[j].start;
            }
            be32(rom + mtable->start + 4 * k, at - mtable->start);
        }
    }
    if (!have_code)
        stand_in_front_end(rom);
    yaml_free(lay);
    yaml_free(pk);
    free(done);
    pack_close(files);
    files = NULL;

    char sha1[41];
    host_sha1_hex(rom, size, sha1);
    *edited = strcmp(sha1, PORT_ROM_SHA1) != 0 || tex_overrides > 0;
    if (*edited)
        host_log("pack %s: %s%d textures replaced%s\n", path, have_code ? "edited: " : "without the code: ",
                 tex_overrides, strcmp(sha1, PORT_ROM_SHA1) ? ", the ROM image differs from the ROM" : "");
    (void)changed;
    /* PORT_PACK_DUMP=FILE: the image, for a look (port/make_pack.py --check) */
    const char *dump = getenv("PORT_PACK_DUMP");
    if (dump) {
        FILE *f = fopen(dump, "wb");
        if (!f || fwrite(rom, 1, size, f) != size)
            host_fatal("PORT_PACK_DUMP: can't write %s", dump);
        fclose(f);
    }
    *size_out = size;
    return rom;
}

int pack_is_pack(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    uint8_t m[4] = {0};
    size_t n = fread(m, 1, 4, f);
    fclose(f);
    if (n == 4 && m[0] == 'P' && m[1] == 'K' && (m[2] == 3 || m[2] == 5))
        return 1;
    char *y = join(path, "pack.yaml");
    f = fopen(y, "rb");
    free(y);
    if (f)
        fclose(f);
    return f != NULL;
}
