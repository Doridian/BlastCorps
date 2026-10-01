/*
 * --hd-text: the game's text drawn from a font at high resolution (docs/
 * FONTS.md).  Render-side only: the game, its memory and its timing are
 * the same with it on.
 *
 * The game draws all its text with drawtext.c (hd_code 14B30), one
 * textured quad a character.  Each glyph is a 32x32 I4 texture that
 * font.c (hd_code 168B0, func_8025B0B8) decodes from the ROM's texture
 * table into one of 0x50 slots of 512 bytes at D_8039CAF0, recording
 * which texture is in which slot in D_803653B0.  The OpenGL renderer asks
 * hdtext_glyph_at() whether a texture it loads comes from a slot; if so,
 * hdtext_image() makes a 32K x 32K version of it from a TrueType font
 * (built in: Stardos Stencil Bold, SIL OFL 1.1, port/fonts/), fitted to
 * the box the game's glyph covers and as heavy as it is, so that the
 * text keeps the game's layout, colours and shadows (those come from the
 * vertices and the combiner).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "port.h"
#include "host.h"
#include "hdtext.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include "../third_party/stb/stb_truetype.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

int hdtext_on;
float hdtext_weight = 1.0f;
const char *hdtext_font_path;           /* NULL: the built-in font */

/* the built-in font (CMake embeds port/fonts/StardosStencil-Bold.ttf) */
extern const unsigned char hdtext_builtin_ttf[];
extern const unsigned int hdtext_builtin_ttf_len;

/* font.c's glyph cache: 0x50 slots of a 32x32 I4 texture, and the texture
   table index in each (0: free) */
extern char D_8039CAF0[], D_803653B0[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_803653B0 PORT_VAR(D_803653B0)
#endif
#define SLOTS 0x50
#define SLOT_BYTES 0x200

/* The stencil font: texture table entries 0xF4C-0xF7E, in drawtext.c's
   order.  0 marks the small "DEL" of the name entry (14), drawn as its
   three letters, 1 a solid box (49, the name entry's cursor). */
#define STENCIL_FIRST 0xF4C
static const char stencil_chars[51] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 0, 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '&', '\'', ')', ':', ',', '$', '!', '.',
    '-', '(', '%', '?', 1, '/',
};

static stbtt_fontinfo font;
static unsigned char *font_data;
static int font_ok = -1;                /* -1: not loaded yet */

typedef struct {
    uint64_t src;                       /* the hash of the 32x32 it was fitted to (0: none yet) */
    uint8_t *img;                       /* HDTEXT_K * 32 squared, coverage; NULL: the original */
} HdGlyph;
static HdGlyph cache[64];

static int load_font(void) {
    if (font_ok >= 0)
        return font_ok;
    font_ok = 0;
    const unsigned char *data = hdtext_builtin_ttf;
    if (hdtext_font_path) {
        FILE *f = fopen(hdtext_font_path, "rb");
        long n = -1;
        if (f && !fseek(f, 0, SEEK_END) && (n = ftell(f)) > 0 && !fseek(f, 0, SEEK_SET)) {
            font_data = malloc((size_t)n);
            if (!font_data || fread(font_data, 1, (size_t)n, f) != (size_t)n) {
                free(font_data);
                font_data = NULL;
            }
        }
        if (f)
            fclose(f);
        if (!font_data) {
            host_log("hd-text: can't read %s; using the built-in font\n", hdtext_font_path);
        } else {
            data = font_data;
        }
    }
    int off = stbtt_GetFontOffsetForIndex(data, 0);
    if (off < 0 || !stbtt_InitFont(&font, data, off)) {
        host_log("hd-text: not a font I can read; text stays as it is\n");
        return 0;
    }
    font_ok = 1;
    return 1;
}

int hdtext_glyph_at(uint32_t addr) {
    uint32_t base = PORT_ADDR(D_8039CAF0);
    uint32_t off = addr - base;
    if (off >= SLOTS * SLOT_BYTES || off % SLOT_BYTES)
        return -1;
    int tex = port_g16(D_803653B0 + 2 * (off / SLOT_BYTES));
    return tex >= STENCIL_FIRST && tex < STENCIL_FIRST + (int)sizeof stencil_chars ? tex : -1;
}

/* the box the ink a[] covers in texel units (x right, y down the
   texture's rows), to a fraction of a texel from its edge texels'
   coverage; its ink area and its peak */
static int ink_box(const float *a32, float box[4], float *area, float *peak) {
    float colmax[32] = { 0 }, rowmax[32] = { 0 };
    float sum = 0;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            float a = a32[y * 32 + x];
            sum += a;
            if (a > colmax[x]) colmax[x] = a;
            if (a > rowmax[y]) rowmax[y] = a;
        }
    int x0 = 0, x1 = 31, y0 = 0, y1 = 31;
    while (x0 < 32 && colmax[x0] == 0) x0++;
    if (x0 == 32)
        return 0;
    while (colmax[x1] == 0) x1--;
    while (rowmax[y0] == 0) y0++;
    while (rowmax[y1] == 0) y1--;
    box[0] = x0 + 1 - colmax[x0];
    box[1] = y0 + 1 - rowmax[y0];
    box[2] = x1 + colmax[x1];
    box[3] = y1 + rowmax[y1];
    *area = sum;
    *peak = 0;
    for (int x = 0; x < 32; x++)
        if (colmax[x] > *peak) *peak = colmax[x];
    return 1;
}

static float sdf_at(const uint8_t *sdf, int w, int h, float x, float y) {
    x -= 0.5f; y -= 0.5f;
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    float v[4];
    for (int k = 0; k < 4; k++) {
        int sx = ix + (k & 1), sy = iy + (k >> 1);
        v[k] = sx < 0 || sy < 0 || sx >= w || sy >= h ? 0 : sdf[sy * w + sx];
    }
    return (v[0] * (1 - fx) + v[1] * fx) * (1 - fy) + (v[2] * (1 - fx) + v[3] * fx) * fy;
}

#define SDF_EM 160.0f           /* the font's em in the distance field, pixels */
#define SDF_PAD 24
#define SDF_SCALE 6.0f          /* distance-field units per pixel */

typedef struct {
    const uint8_t *sdf;
    int w, h;
    float gx0, gy0, gx1, gy1;   /* the glyph's ink in the field's pixels */
    float box[4];               /* where it goes, in hd pixels (rows as the texture's) */
    float aa;                   /* field pixels per hd pixel */
} Fit;

/* coverage at hd pixel (px, py), the outline moved out by e field pixels */
static float cover(const Fit *f, int px, int py, float e) {
    float u = (px + 0.5f - f->box[0]) / (f->box[2] - f->box[0]);
    float v = (py + 0.5f - f->box[1]) / (f->box[3] - f->box[1]);
    if (u < -0.2f || u > 1.2f || v < -0.2f || v > 1.2f)
        return 0;
    /* (the texture's rows run up the glyph: the box's first row is its
       bottom) */
    float x = f->gx0 + u * (f->gx1 - f->gx0), y = f->gy1 - v * (f->gy1 - f->gy0);
    float d = (sdf_at(f->sdf, f->w, f->h, x, y) - 128.0f) / SDF_SCALE + e;
    float c = d / f->aa + 0.5f;
    return c < 0 ? 0 : c > 1 ? 1 : c;
}

static float area_at(const Fit *f, float e, int step) {
    float s = 0;
    int n = HDTEXT_K * 32;
    for (int y = 0; y < n; y += step)
        for (int x = 0; x < n; x += step)
            s += cover(f, x, y, e);
    return s * step * step / (float)(HDTEXT_K * HDTEXT_K);
}

static double secs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* the font's distance field for a character, made once */
typedef struct {
    uint8_t *sdf;
    int w, h, done;
    float gx0, gy0, gx1, gy1;
} Field;
static Field fields[sizeof stencil_chars];

static const Field *field(int i);

/* the ink's connected pieces (8-neighbours), up to max: each one's ink in
   out[k] (zero elsewhere); how many there are */
static int pieces(const float *a32, float (*out)[1024], int max) {
    int lab[1024] = { 0 }, n = 0, stack[1024];
    for (int s0 = 0; s0 < 1024; s0++) {
        if (a32[s0] == 0 || lab[s0])
            continue;
        if (++n > max)
            return n;
        memset(out[n - 1], 0, sizeof out[0]);
        int sp = 0;
        stack[sp++] = s0;
        lab[s0] = n;
        while (sp) {
            int p = stack[--sp];
            out[n - 1][p] = a32[p];
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int x = p % 32 + dx, y = p / 32 + dy;
                    if (x < 0 || y < 0 || x > 31 || y > 31 || a32[y * 32 + x] == 0 || lab[y * 32 + x])
                        continue;
                    lab[y * 32 + x] = n;
                    stack[sp++] = y * 32 + x;
                }
        }
    }
    return n;
}

/* font character i (stencil_chars' index) into img over box, as heavy as
   area; returns the outline's offset */
static float draw(uint8_t *img, int i, const float box[4], float area, float peak) {
    int n = HDTEXT_K * 32;
    const Field *fd = field(i);
    if (!fd)
        return 0;
    Fit f = { fd->sdf, fd->w, fd->h, fd->gx0, fd->gy0, fd->gx1, fd->gy1,
              { box[0] * HDTEXT_K, box[1] * HDTEXT_K, box[2] * HDTEXT_K, box[3] * HDTEXT_K }, 1 };
    float ax = (f.gx1 - f.gx0) / (f.box[2] - f.box[0]), ay = (f.gy1 - f.gy0) / (f.box[3] - f.box[1]);
    f.aa = ax > ay ? ax : ay;
    /* as heavy as the game's: the outline's offset whose ink area is the
       glyph's (a bisection; the area grows with e) */
    float lo = -SDF_PAD * 0.5f, hi = SDF_PAD * 0.9f, e = 0;
    float want = area / peak * hdtext_weight;
    for (int it = 0; it < 12; it++) {
        e = (lo + hi) / 2;
        if (area_at(&f, e, 4) < want)
            lo = e;
        else
            hi = e;
    }
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            uint8_t v = (uint8_t)(cover(&f, x, y, e) * peak * 255 + 0.5f);
            if (v > img[y * n + x])
                img[y * n + x] = v;
        }
    return e;
}

static const Field *field(int i) {
    Field *fd = &fields[i];
    if (fd->done)
        return fd->sdf ? fd : NULL;
    fd->done = 1;
    int g = stbtt_FindGlyphIndex(&font, (unsigned char)stencil_chars[i]);
    int ix0, iy0, ix1, iy1;
    if (!g || !stbtt_GetGlyphBox(&font, g, &ix0, &iy0, &ix1, &iy1))
        return NULL;
    float scale = stbtt_ScaleForMappingEmToPixels(&font, SDF_EM);
    int xo, yo;
    fd->sdf = stbtt_GetGlyphSDF(&font, scale, g, SDF_PAD, 128, SDF_SCALE, &fd->w, &fd->h, &xo, &yo);
    fd->gx0 = ix0 * scale - xo;
    fd->gy0 = -iy1 * scale - yo;
    fd->gx1 = ix1 * scale - xo;
    fd->gy1 = -iy0 * scale - yo;
    return fd->sdf ? fd : NULL;
}

static int char_index(int ch) {
    for (unsigned i = 0; i < sizeof stencil_chars; i++)
        if (stencil_chars[i] == ch)
            return (int)i;
    return -1;
}

static uint8_t *make(int i, const uint8_t *rgba) {
    double t0 = secs();
    int ch = stencil_chars[i];
    float a32[1024], box[4], area, peak;
    for (int k = 0; k < 1024; k++)
        a32[k] = rgba[4 * k + 3] / 255.0f;
    if (!ink_box(a32, box, &area, &peak))
        return NULL;
    /* (the ink at the game's intensity: its I4 glyphs have 3 bits, 14/15
       at most, which the text's colours are made with) */
    int n = HDTEXT_K * 32;
    uint8_t *img = calloc((size_t)n * n, 1);
    float e = 0;
    if (ch == 1) {                      /* the cursor: a box */
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++) {
                float cx = fminf(x + 1.0f, box[2] * HDTEXT_K) - fmaxf((float)x, box[0] * HDTEXT_K);
                float cy = fminf(y + 1.0f, box[3] * HDTEXT_K) - fmaxf((float)y, box[1] * HDTEXT_K);
                img[y * n + x] = cx > 0 && cy > 0 ? (uint8_t)(fminf(cx, 1) * fminf(cy, 1) * peak * 255 + 0.5f) : 0;
            }
        return img;
    }
    if (ch == 0) {                      /* "DEL": its three letters, each in its own box */
        static float part[4][1024];
        float pb[3][4], pa[3], pp;
        int order[3] = { 0, 1, 2 };
        if (pieces(a32, part, 3) != 3) {
            free(img);
            return NULL;
        }
        for (int k = 0; k < 3; k++)
            ink_box(part[k], pb[k], &pa[k], &pp);
        for (int k = 0; k < 2; k++)     /* (left to right) */
            for (int j = 0; j < 2 - k; j++)
                if (pb[order[j]][0] > pb[order[j + 1]][0]) {
                    int t = order[j]; order[j] = order[j + 1]; order[j + 1] = t;
                }
        for (int k = 0; k < 3; k++)
            draw(img, char_index("DEL"[k]), pb[order[k]], pa[order[k]], peak);
    } else {
        e = draw(img, i, box, area, peak);
    }
    if (host_verbose)
        host_log("hd-text: glyph '%s': box %.2f,%.2f-%.2f,%.2f, outline %+.2f px, %.1f ms\n",
                 ch ? (char[]){ (char)ch, 0 } : "DEL", box[0], box[1], box[2], box[3], e, (secs() - t0) * 1e3);
    return img;
}

void hdtext_init(void) {
    if (!hdtext_on || !load_font())
        return;
    /* (the distance fields ahead, so that a screen of new text doesn't
       wait for them) */
    double t0 = secs();
    for (unsigned i = 0; i < sizeof stencil_chars; i++)
        if (stencil_chars[i] > 1)
            field((int)i);
    if (host_verbose)
        host_log("hd-text: %s, distance fields in %.0f ms\n", hdtext_font_path ? hdtext_font_path : "built-in font",
                 (secs() - t0) * 1e3);
}

const uint8_t *hdtext_image(int tex, const uint8_t *rgba) {
    if (!load_font())
        return NULL;
    int i = tex - STENCIL_FIRST;
    if (i < 0 || i >= (int)sizeof stencil_chars)
        return NULL;
    uint64_t h = 0xCBF29CE484222325ull;
    for (int k = 0; k < 32 * 32 * 4; k += 4)
        h = (h ^ rgba[k + 3]) * 0x100000001B3ull;
    h |= 1;
    HdGlyph *c = &cache[i];
    if (c->src != h) {
        /* (another texture in the same entry would be another game's
           glyph: fitted again) */
        free(c->img);
        c->img = make(i, rgba);
        c->src = h;
    }
    return c->img;
}
