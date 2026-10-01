/*
 * The OpenGL 3.3 back end for gfx.c's display-list front end.
 *
 * gfx.c runs the RSP as before (transform, lighting, clipping) and hands
 * over screen-space polygons and rectangles; this file turns the RDP state
 * that goes with them into GPU draws:
 *
 * - Color images.  The game's 320-wide 16-bit framebuffers become render
 *   targets (an RGBA8 texture each, at the internal resolution) sharing one
 *   depth buffer, which is the z image.  A fill rectangle into the z image
 *   clears it.  Everything else the game draws into (the 64-wide 8-bit
 *   images it renders shadows and other textures into) stays with the
 *   software rasterizer, in RDRAM, where the game reads it back as a
 *   texture; the rare texture load from a GPU target reads the target back
 *   first (gfx_gl_texture_source), and PORT_GL_READBACK=1 writes every
 *   target back to RDRAM after each task, for code that reads the
 *   framebuffer with the CPU.
 * - Textures.  A tile is decoded from TMEM (gfx_fetch_texel, the software
 *   renderer's own decoder) into the texels it can address after wrapping,
 *   and cached by a hash of the TMEM bytes it reads and its parameters.
 *   Wrapping, clamping, mirroring, shifts and filtering (N64 3-point,
 *   bilinear or point) are done in the shader with texelFetch, as the RDP
 *   does them, as is the level of detail (tile selection and LOD_FRACTION).
 * - The color combiner (both cycles), alpha compare, coverage-from-alpha
 *   and the blender are compiled into a fragment shader per mode.  The
 *   blender's cycle that reads memory becomes the GL blend
 *   (ONE, SRC_ALPHA) with the source premultiplied, which reproduces
 *   (P * A + M * B) / (A + B) for any P/M/A/B; the other cycles run in the
 *   shader.
 * - Triangles are batched per draw state (program, textures, uniforms,
 *   depth/blend mode, scissor, target).
 *
 * The frame shown is the target the VI points at, scaled to the window;
 * a framebuffer the GPU never drew (or a VI width other than 320) is shown
 * from RDRAM.
 */
#ifdef PORT_HAVE_GL

#include <SDL.h>
#ifdef __EMSCRIPTEN__
/* WebGL 2: OpenGL ES 3.0, GLSL ES 3.00 */
#include <GLES3/gl3.h>
#define GLSL_VERSION "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n"
#define glClearDepth glClearDepthf
#ifndef GL_DEPTH_CLAMP
#define GL_DEPTH_CLAMP 0x864F   /* EXT_depth_clamp, where the browser has it */
#endif
#else
#include <epoxy/gl.h>
#define GLSL_VERSION "#version 330 core\n"
#endif
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "gfx.h"
#include "hdtext.h"

int gfx_gl_enabled;
int gfx_gl_scale;               /* internal resolution: 320x240 times this; 0: from the window */

static SDL_Window *win;
static SDL_GLContext ctx;
static int scale = 1;           /* current */
/* widescreen (gfx.h): the targets are TW + 2 * gfx_wide_off N64 pixels
   across, 240 high, the game's 320 columns in the middle */
#define wide_off gfx_wide_off
static int tw_px = 320, th_px = 240; /* the targets, in pixels */
static int readback_all;
static int quantize;            /* PORT_GL_QUANT=1: 5-bit output, as the RDRAM framebuffer has */
static uint32_t frame_no;

/* ---- targets ------------------------------------------------------------------ */

typedef struct {
    uint32_t addr;
    GLuint fbo, tex;
    int dirty;                  /* drawn since the last read-back */
    /* --interpolate: the twins the in-between passes draw into (each with
       its own depth buffer), and which were drawn since the frame in the
       target was shown */
    GLuint ifbo[GFX_TWINS], itex[GFX_TWINS];
    unsigned idrawn;
} Target;

static Target targets[8];
static int ntargets;
static GLuint depth_rb, idepth_rb[GFX_TWINS];
static int ipass_gl;            /* drawing in-between image ipass_gl - 1 (0: the frame) */
static Target *cur_target;
static uint32_t cur_cimg = 1;
static Target view;             /* RDRAM shown as it is */

#define TW 320
#define TH 240

/* ---- GL state, as last set ------------------------------------------------------ */

/* A browser's GL call costs a trip through JavaScript and WebGL's checks,
   a few hundred of them a frame (a draw per texture, and the game changes
   textures every few triangles): the draws set only what differs from the
   last draw.  Code that sets GL state by other ways (targets, uploads,
   textures, presenting) marks it unknown (GLC_DIRTY). */
static struct {
    int valid;
    GLuint fbo, prog, tex[8];
    int vp_w, vp_h, scissor_on, sc[4], cmask, depth_on, depth_func, depth_mask, blend, active;
} glc;
#define GLC_DIRTY() (glc.valid = 0)

static void glc_check(void);

static void c_fbo(GLuint f) {
    glc_check();
    if (glc.fbo != f)
        glBindFramebuffer(GL_FRAMEBUFFER, glc.fbo = f);
}

static void c_viewport(int w, int h) {
    if (glc.vp_w != w || glc.vp_h != h)
        glViewport(0, 0, glc.vp_w = w, glc.vp_h = h);
}

static void c_scissor(int x, int y, int w, int h) {
    if (glc.scissor_on != 1) {
        glEnable(GL_SCISSOR_TEST);
        glc.scissor_on = 1;
    }
    if (glc.sc[0] != x || glc.sc[1] != y || glc.sc[2] != w || glc.sc[3] != h) {
        glScissor(x, y, w, h);
        glc.sc[0] = x; glc.sc[1] = y; glc.sc[2] = w; glc.sc[3] = h;
    }
}

static void c_cmask(int all) {
    if (glc.cmask != all) {
        glColorMask(1, 1, 1, all);
        glc.cmask = all;
    }
}

static void c_depth_mask(int m) {
    if (glc.depth_mask != m)
        glDepthMask((glc.depth_mask = m) ? GL_TRUE : GL_FALSE);
}

static void c_depth(int on, int func) {
    if (glc.depth_on != on) {
        if (on)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        glc.depth_on = on;
    }
    if (on && glc.depth_func != func)
        glDepthFunc(glc.depth_func = func);
}

static void c_blend(int mode) {
    if (glc.blend == mode)
        return;
    if (mode == 0) {
        glDisable(GL_BLEND);
    } else {
        if (glc.blend <= 0)
            glEnable(GL_BLEND);
        if (mode == 1)
            glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        else
            glBlendFunc(GL_ZERO, GL_ONE);
    }
    glc.blend = mode;
}

static void c_prog(GLuint p) {
    if (glc.prog != p)
        glUseProgram(glc.prog = p);
}

static void c_tex(int unit, GLuint t) {
    if (glc.tex[unit] == t)
        return;
    if (glc.active != unit)
        glActiveTexture(GL_TEXTURE0 + (glc.active = unit));
    glBindTexture(GL_TEXTURE_2D, glc.tex[unit] = t);
}

static void target_storage(Target *t) {
    GLC_DIRTY();
    glBindTexture(GL_TEXTURE_2D, t->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tw_px, th_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->tex, 0);
    if (t != &view)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        host_fatal("gl: framebuffer incomplete");
}

/* an N64 x (the game's 320 columns) to a target's pixel column */
static int px_x(int x) { return (x + wide_off) * scale; }

/* RDRAM's RGBA5551 image at addr into t, scaled, in the middle of a wide target */
static uint32_t upload_buf[TW * TH];
static void upload_rdram(Target *t, uint32_t addr, int width) {
    GLC_DIRTY();
    const uint8_t *src = port_ptr(addr);
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++) {
            uint16_t c = port_be16(src + 2 * (y * width + x));
            uint32_t r = (c >> 11) & 31, g = (c >> 6) & 31, b = (c >> 1) & 31;
            upload_buf[(TH - 1 - y) * TW + x] = 0xFF000000u | (r << 3 | r >> 2) | (g << 3 | g >> 2) << 8 |
                                                (b << 3 | b >> 2) << 16;
        }
    static GLuint tmp, tmp_fbo;
    if (!tmp) {
        glGenTextures(1, &tmp);
        glGenFramebuffers(1, &tmp_fbo);
        glBindTexture(GL_TEXTURE_2D, tmp);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TW, TH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindFramebuffer(GL_FRAMEBUFFER, tmp_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmp, 0);
    }
    glBindTexture(GL_TEXTURE_2D, tmp);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TW, TH, GL_RGBA, GL_UNSIGNED_BYTE, upload_buf);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, tmp_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, t->fbo);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(1, 1, 1, 1);
    if (wide_off > 0) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBlitFramebuffer(0, 0, TW, TH, px_x(0), 0, px_x(TW), th_px, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void twin_storage(Target *t, int k) {
    GLC_DIRTY();
    if (!idepth_rb[k]) {
        glGenRenderbuffers(1, &idepth_rb[k]);
        glBindRenderbuffer(GL_RENDERBUFFER, idepth_rb[k]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, tw_px, th_px);
    }
    if (!t->ifbo[k]) {
        glGenFramebuffers(1, &t->ifbo[k]);
        glGenTextures(1, &t->itex[k]);
    }
    glBindTexture(GL_TEXTURE_2D, t->itex[k]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tw_px, th_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, t->ifbo[k]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->itex[k], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, idepth_rb[k]);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        host_fatal("gl: framebuffer incomplete");
    t->idrawn &= ~(1u << k);
}

/* the framebuffer object a draw into t goes to */
static GLuint target_fbo(Target *t) {
    if (!ipass_gl)
        return t->fbo;
    int k = ipass_gl - 1;
    if (!t->ifbo[k])
        twin_storage(t, k);
    return t->ifbo[k];
}

static Target *find_target(uint32_t addr) {
    for (int i = 0; i < ntargets; i++)
        if (targets[i].addr == addr)
            return &targets[i];
    return NULL;
}

static void flush(void);

static Target *get_target(uint32_t addr) {
    Target *t = find_target(addr);
    if (t)
        return t;
    flush();                                        /* (the batches name targets, which move) */
    if (ntargets == 8) {                            /* recycle the oldest */
        GLC_DIRTY();
        glDeleteFramebuffers(1, &targets[0].fbo);
        glDeleteTextures(1, &targets[0].tex);
        for (int k = 0; k < GFX_TWINS; k++)
            if (targets[0].ifbo[k]) {
                glDeleteFramebuffers(1, &targets[0].ifbo[k]);
                glDeleteTextures(1, &targets[0].itex[k]);
            }
        memmove(targets, targets + 1, 7 * sizeof targets[0]);
        ntargets--;
    }
    t = &targets[ntargets++];
    memset(t, 0, sizeof *t);
    t->addr = addr;
    glGenFramebuffers(1, &t->fbo);
    glGenTextures(1, &t->tex);
    target_storage(t);
    upload_rdram(t, addr, TW);                      /* what was there */
    if (host_verbose)
        host_log("gl: target %08X\n", addr);
    return t;
}

int gfx_gl_max_pixels;

/* the scale for a dw x dh drawable at aspect: --scale's, or the lines the
   picture has there (the nearest multiple of 240: the window's height, or
   less in a window narrower than the picture, which is letterboxed),
   lowered until a picture has at most --max-pixels */
static int want_scale(int dw, int dh, float aspect) {
    long cols = TW + 2 * gfx_wide_off_for(aspect);
    int lines = dh;
    if ((long)dw * TH < (long)dh * cols)            /* narrower: as wide as the window */
        lines = (int)((long)dw * TH / cols);
    int s = gfx_gl_scale ? gfx_gl_scale : (lines + TH / 2) / TH;
    while (s > 1 && gfx_gl_max_pixels > 0 && cols * s * TH * s > gfx_gl_max_pixels)
        s--;
    return s < 1 ? 1 : s;
}

/* the internal resolution: 240 * s lines, and aspect (gfx_aspect_of) wide */
static void set_geometry(int s, float aspect) {
    if (s < 1) s = 1;
    if (s > 16) s = 16;
    int off = gfx_wide_off_for(aspect), npx = (TW + 2 * off) * s;
    if (s == scale && npx == tw_px && depth_rb)
        return;
    int old_w = tw_px, old_h = th_px;
    GLC_DIRTY();
    if (!depth_rb)
        glGenRenderbuffers(1, &depth_rb);
    scale = s;
    gfx_set_wide(off);
    tw_px = npx;
    th_px = TH * s;
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, tw_px, th_px);
    for (int k = 0; k < GFX_TWINS; k++)             /* the twins start over */
        if (idepth_rb[k]) {
            glBindRenderbuffer(GL_RENDERBUFFER, idepth_rb[k]);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, tw_px, th_px);
            for (int i = 0; i < ntargets; i++)
                if (targets[i].ifbo[k])
                    twin_storage(&targets[i], k);
        }
    /* keep what the targets hold, rescaled */
    for (int i = -1; i < ntargets; i++) {
        Target *t = i < 0 ? &view : &targets[i];
        if (!t->fbo)
            continue;
        GLuint otex = t->tex, ofbo = t->fbo;
        /* the old target without the (already resized) depth buffer, or
           the blit would be clipped to it */
        glBindFramebuffer(GL_FRAMEBUFFER, ofbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glGenFramebuffers(1, &t->fbo);
        glGenTextures(1, &t->tex);
        target_storage(t);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, ofbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, t->fbo);
        glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(0, 0, old_w, old_h, 0, 0, tw_px, th_px, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glDeleteFramebuffers(1, &ofbo);
        glDeleteTextures(1, &otex);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, targets[0].fbo ? targets[0].fbo : 0);
    glClearDepth(1.0);
    glDepthMask(GL_TRUE);
    if (ntargets) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    host_log("gl: internal resolution %dx%d\n", tw_px, th_px);
}

/* a target's pixels back into RDRAM as RGBA5551 (point-sampled down) */
static uint8_t *rb_buf;
static void read_back(Target *t) {
    int w = tw_px, h = th_px;
    rb_buf = realloc(rb_buf, (size_t)w * h * 4);
    GLC_DIRTY();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rb_buf);
    uint8_t *dst = port_ptr(t->addr);
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++) {
            const uint8_t *p = rb_buf + 4 * ((size_t)(h - 1 - y * scale - scale / 2) * w + px_x(x) + scale / 2);
            port_wbe16(dst + 2 * (y * TW + x), (uint16_t)((p[0] >> 3) << 11 | (p[1] >> 3) << 6 | (p[2] >> 3) << 1 | 1));
        }
    t->dirty = 0;
}

/* ---- textures ------------------------------------------------------------------ */

typedef struct {
    uint64_t key;
    GLuint tex;
    uint32_t used;
    int hd;                     /* --hd-text: a font glyph's, HDTEXT_K times the tile's size (0: the tile's) */
} TexEnt;

#define TC_SIZE 16384
static TexEnt tcache[TC_SIZE];
static int tc_count;
static unsigned long long st_tex_decoded, st_tex_hits;

static uint64_t hash_bytes(uint64_t h, const uint8_t *p, int n) {
    for (int i = 0; i < n; i++) {
        h ^= p[i];
        h *= 0x100000001B3ull;
    }
    return h;
}

static uint64_t hash_words(uint64_t h, const uint8_t *p, int n) {
    int i = 0;
    /* four lanes, so that the multiplies overlap */
    uint64_t l[4] = { h, h ^ 0x243F6A8885A308D3ull, h ^ 0x13198A2E03707344ull, h ^ 0xA4093822299F31D0ull };
    for (; i + 32 <= n; i += 32) {
        uint64_t v[4];
        memcpy(v, p + i, 32);
        for (int j = 0; j < 4; j++) {
            l[j] = (l[j] ^ v[j]) * 0x9E3779B97F4A7C15ull;
            l[j] ^= l[j] >> 29;
        }
    }
    h = l[0] ^ (l[1] * 3) ^ (l[2] * 5) ^ (l[3] * 7);
    for (; i + 8 <= n; i += 8) {
        uint64_t v;
        memcpy(&v, p + i, 8);
        h = (h ^ v) * 0x9E3779B97F4A7C15ull;
        h ^= h >> 29;
    }
    return hash_bytes(h, p + i, n - i);
}

static void tc_sweep(void) {
    int n = 0;
    flush();                    /* the pending batch may use them */
    GLC_DIRTY();
    for (int i = 0; i < TC_SIZE; i++)
        if (tcache[i].tex && !tcache[i].hd) {   /* (a font glyph's is shared: hd_texture) */
            glDeleteTextures(1, &tcache[i].tex);
            n++;
        }
    memset(tcache, 0, sizeof tcache);
    tc_count = 0;
    if (host_verbose)
        host_log("gl: texture cache flushed (%d)\n", n);
}

static uint8_t decode_buf[1024 * 1024 * 4];

/* --hd-text: one GL texture a font glyph, whatever TMEM it came through
   (the game loads the same glyph into many places, and each upload is
   256x256 with its mipmaps, which a software GL takes milliseconds over;
   one channel, a quarter of RGBA's) */
static struct {
    const uint8_t *img;         /* hdtext_image()'s, which it keeps until the glyph changes */
    unsigned serial;
    GLuint tex;
} hd_shared[64];

static GLuint hd_texture(int glyph, const uint8_t *img) {
    unsigned serial = hdtext_serial(glyph);
    int s = glyph & 63;
    if (hd_shared[s].tex && hd_shared[s].img == img && hd_shared[s].serial == serial)
        return hd_shared[s].tex;
    enum { N = 32 * HDTEXT_K };
    if (!hd_shared[s].tex)
        glGenTextures(1, &hd_shared[s].tex);
    glBindTexture(GL_TEXTURE_2D, hd_shared[s].tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, N, N, 0, GL_RED, GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);
    hd_shared[s].img = img;
    hd_shared[s].serial = serial;
    return hd_shared[s].tex;
}

/* --hd-text: the font glyph a tile holds (hdtext.c), or -1: a 32x32 I4
   tile whose 512 bytes one LoadBlock brought from one of font.c's slots */
static int tile_glyph(const GfxTile *t, int w, int h) {
    if (!hdtext_on || t->fmt != 4 || t->siz != 0 || w != 32 || h != 32 || t->line != 2)
        return -1;
    int at = t->tmem & 511;
    uint32_t src = gfx_tmem_src[at];
    if (!src || at + 63 > 511 || gfx_tmem_src[at + 63] != src + 504)
        return -1;
    return hdtext_glyph_at(src);
}

/* the GL texture for a tile as TMEM holds it now; *hd: HDTEXT_K for a
   font glyph drawn from the font (--hd-text), else 0 */
static GLuint tile_texture(int tile, int *hd) {
    const GfxTile *t = &gs.tile[tile & 7];
    int w, h;
    gfx_tile_dims(t, &w, &h);
    int glyph = tile_glyph(t, w, h);
    /* what the tile reads: its rows of TMEM (and the palette) */
    uint64_t k = 0xCBF29CE484222325ull;
    int params[8] = { t->fmt, t->siz, t->line, t->tmem, t->pal, w, h, t->fmt == 2 ? (int)(gs.om_h >> 14 & 3) : 0 };
    k = hash_words(k, (const uint8_t *)params, sizeof params);
    int bpp2 = t->siz == 0 ? 1 : t->siz == 1 ? 2 : t->siz == 2 ? 4 : 4;
    int rowbytes = (w * bpp2 + 1) / 2;
    int len = (h - 1) * t->line * 8 + rowbytes + 8;
    if (len > 4096 || len < 0)
        len = 4096;
    int start = t->tmem * 8;
    if (t->siz == 3) {
        start &= 0x7FF;
        if (len > 2048) len = 2048;
        uint8_t tmp[4096];
        for (int i = 0; i < len; i++) {
            tmp[i] = gfx_tmem[(start + i) & 0x7FF];
            tmp[len + i] = gfx_tmem[0x800 + ((start + i) & 0x7FF)];
        }
        k = hash_words(k, tmp, 2 * len);
    } else if (start + len <= 4096) {
        k = hash_words(k, gfx_tmem + start, len);
    } else {
        uint8_t tmp[4096];
        for (int i = 0; i < len; i++)
            tmp[i] = gfx_tmem[(start + i) & 4095];
        k = hash_words(k, tmp, len);
    }
    if (t->fmt == 2)
        k = hash_words(k, gfx_tmem + 0x800, 512);
    if (glyph >= 0)             /* (not the tile's own texture) */
        k = (k ^ (uint64_t)glyph << 40 ^ 0x48442D74657874ull) * 0x9E3779B97F4A7C15ull;
    if (!k)
        k = 1;
    unsigned i = (unsigned)(k ^ (k >> 32)) & (TC_SIZE - 1);
    while (tcache[i].key && tcache[i].key != k)
        i = (i + 1) & (TC_SIZE - 1);
    if (tcache[i].key == k) {
        tcache[i].used = frame_no;
        st_tex_hits++;
        *hd = tcache[i].hd;
        return tcache[i].tex;
    }
    if (tc_count > TC_SIZE / 2) {
        tc_sweep();
        return tile_texture(tile, hd);
    }
    st_tex_decoded++;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            gfx_fetch_texel(t, x, y, decode_buf + 4 * (y * w + x));
    /* --hd-text: the glyph from the font instead, as the I4 texels would
       be (the intensity in all four channels), with mipmaps for when it
       is drawn small */
    const uint8_t *img = glyph >= 0 ? hdtext_image(glyph, decode_buf) : NULL;
    GLuint tex;
    GLC_DIRTY();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (img) {
        tex = hd_texture(glyph, img);
    } else {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, decode_buf);
    }
    tcache[i].key = k;
    tcache[i].tex = tex;
    tcache[i].used = frame_no;
    tcache[i].hd = *hd = img ? HDTEXT_K : 0;
    tc_count++;
    return tex;
}

/* ---- shaders ------------------------------------------------------------------- */

enum { K_TRI, K_FILL, K_TEXRECT };

typedef struct {
    uint32_t cc0, cc1, om_l;
    uint8_t cyc, textured, tex1, lod, filt, kind, quant, hd;      /* hd: --hd-text's font glyphs */
} ProgKey;

typedef struct {
    ProgKey key;
    GLuint prog;
    int blend;                  /* 0 none, 1 (ONE, SRC_ALPHA), 2 keep the destination */
    GLint u_vp, u_zbias, u_prim, u_env, u_blend, u_fog, u_primlod, u_seed, u_tA, u_tB, u_tul, u_levels, u_lodscale,
        u_hd;
    /* what its uniforms were last set to (uv: they were) */
    int uv;
    struct {
        float vp[2], zbias, prim[4], env[4], blend[4], fog[4], primlod, seed, lodscale;
        int levels, ntex, tA[8][4], tB[8][4];
        float tul[8][2];
        float hd[8];
    } u;
} Prog;

static Prog progs[1024];
static int nprogs;
static int16_t prog_hash[2048];         /* index + 1, open addressing by the key's hash */

static unsigned prog_slot(const ProgKey *key) {
    uint32_t h = key->cc0 * 0x9E3779B1u ^ key->cc1 * 0x85EBCA77u ^ key->om_l * 0xC2B2AE3Du;
    h ^= (uint32_t)(key->cyc | key->textured << 4 | key->tex1 << 8 | key->lod << 12 | key->filt << 16 |
                    key->kind << 20 | key->quant << 24 | (uint32_t)key->hd << 28) * 0x27D4EB2Fu;
    return (h ^ h >> 15) & 2047;
}

/* the programs the game uses, compiled ahead (gfx_gl_present) */
static const ProgKey prog_table[] = {
#include "gfx_gl_progs.h"
};
static unsigned prog_ahead;

static const char *vs_src =
    GLSL_VERSION
    "layout(location = 0) in vec4 a_pos;\n"
    "layout(location = 1) in vec2 a_st;\n"
    "layout(location = 2) in vec4 a_col;\n"
    "layout(location = 3) in vec4 a_box;\n"
    "uniform vec2 u_vp;\n"
    "uniform float u_zbias;\n"
    "out vec4 v_col;\n"
    "out vec2 v_st;\n"
    "flat out vec4 v_box;\n"
    "void main() {\n"
    "    float w = a_pos.w;\n"
    "    gl_Position = vec4((a_pos.x / u_vp.x * 2.0 - 1.0) * w, (1.0 - a_pos.y / u_vp.y * 2.0) * w,\n"
    "                       ((a_pos.z - u_zbias) * 2.0 - 1.0) * w, w);\n"
    "    v_col = a_col * (1.0 / 255.0);\n"
    "    v_st = a_st;\n"
    "    v_box = a_box;\n"
    "}\n";

static const char *fs_common =
    GLSL_VERSION
    "in vec4 v_col;\n"
    "in vec2 v_st;\n"
    "flat in vec4 v_box;        /* where s, t may go (gfx_gl_tex_rect) */\n"
    "out vec4 o_col;\n"
    "uniform sampler2D u_tex[8];\n"
    "uniform ivec4 u_tA[8];     /* shift s, shift t, mask s, mask t */\n"
    "uniform ivec4 u_tB[8];     /* cm s, cm t, clamp s, clamp t */\n"
    "uniform vec2 u_tul[8];\n"
    "uniform vec4 u_prim, u_env, u_blend, u_fog;\n"
    "uniform float u_primlod, u_lodscale, u_seed;\n"
    "uniform int u_levels;\n"
    "int wrapc(int c, int cm, int mask, int size) {\n"
    "    if ((cm & 2) != 0 || mask == 0) { if (c < 0) c = 0; if (c > size) c = size; }\n"
    "    if (mask != 0) {\n"
    "        int m = (1 << mask) - 1;\n"
    "        bool mir = (cm & 1) != 0 && (c & (1 << mask)) != 0;\n"
    "        c &= m;\n"
    "        if (mir) c = m - c;\n"
    "    }\n"
    "    return c;\n"
    "}\n"
    "vec4 fetch(int i, ivec2 c) {\n"
    "    switch (i) {\n"
    "    case 0: return texelFetch(u_tex[0], clamp(c, ivec2(0), textureSize(u_tex[0], 0) - 1), 0);\n"
    "    case 1: return texelFetch(u_tex[1], clamp(c, ivec2(0), textureSize(u_tex[1], 0) - 1), 0);\n"
    "    case 2: return texelFetch(u_tex[2], clamp(c, ivec2(0), textureSize(u_tex[2], 0) - 1), 0);\n"
    "    case 3: return texelFetch(u_tex[3], clamp(c, ivec2(0), textureSize(u_tex[3], 0) - 1), 0);\n"
    "    case 4: return texelFetch(u_tex[4], clamp(c, ivec2(0), textureSize(u_tex[4], 0) - 1), 0);\n"
    "    case 5: return texelFetch(u_tex[5], clamp(c, ivec2(0), textureSize(u_tex[5], 0) - 1), 0);\n"
    "    case 6: return texelFetch(u_tex[6], clamp(c, ivec2(0), textureSize(u_tex[6], 0) - 1), 0);\n"
    "    default: return texelFetch(u_tex[7], clamp(c, ivec2(0), textureSize(u_tex[7], 0) - 1), 0);\n"
    "    }\n"
    "}\n"
    "ivec2 wrap2(int i, ivec2 c) {\n"
    "    return ivec2(wrapc(c.x, u_tB[i].x, u_tA[i].z, u_tB[i].z), wrapc(c.y, u_tB[i].y, u_tA[i].w, u_tB[i].w));\n"
    "}\n"
    "float shiftc(float f, int sh) {\n"
    "    if (sh == 0) return f;\n"
    "    return sh < 11 ? f / float(1 << sh) : f * float(1 << (16 - sh));\n"
    "}\n"
    "vec4 texel(int i, vec2 st, int filt) {\n"
    "    vec2 f = vec2(shiftc(st.x, u_tA[i].x), shiftc(st.y, u_tA[i].y)) - u_tul[i];\n"
    "    vec2 fl = floor(f);\n"
    "    ivec2 c = ivec2(fl);\n"
    "    if (filt == 0) return fetch(i, wrap2(i, c));\n"
    "    vec2 fr = floor((f - fl) * 32.0) / 32.0;\n"
    "    ivec2 a = wrap2(i, c), b = wrap2(i, c + ivec2(1));\n"
    "    vec4 c00 = fetch(i, a), c10 = fetch(i, ivec2(b.x, a.y));\n"
    "    vec4 c01 = fetch(i, ivec2(a.x, b.y)), c11 = fetch(i, b);\n"
    "    if (filt == 2) return mix(mix(c00, c10, fr.x), mix(c01, c11, fr.x), fr.y);\n"
    "    if (fr.x + fr.y < 1.0) return c00 + fr.x * (c10 - c00) + fr.y * (c01 - c00);\n"
    "    return c11 + (1.0 - fr.x) * (c01 - c11) + (1.0 - fr.y) * (c10 - c11);\n"
    "}\n";

/* --hd-text: a unit whose u_hd is set holds a font glyph HDTEXT_K times
   the tile's size (its coverage in red only, which is the I4 texel's
   intensity in all four channels), sampled with GL's filtering and
   mipmaps; the texel
   centres line up with the tile's (the RDP's texel c is at c, not c + 0.5) */
static const char *fs_hd =
    "uniform float u_hd[8];\n"
    "vec4 hdfetch(int i, vec2 px) {\n"
    "    switch (i) {\n"
    "    case 0: return texture(u_tex[0], px / vec2(textureSize(u_tex[0], 0))).rrrr;\n"
    "    case 1: return texture(u_tex[1], px / vec2(textureSize(u_tex[1], 0))).rrrr;\n"
    "    case 2: return texture(u_tex[2], px / vec2(textureSize(u_tex[2], 0))).rrrr;\n"
    "    case 3: return texture(u_tex[3], px / vec2(textureSize(u_tex[3], 0))).rrrr;\n"
    "    case 4: return texture(u_tex[4], px / vec2(textureSize(u_tex[4], 0))).rrrr;\n"
    "    case 5: return texture(u_tex[5], px / vec2(textureSize(u_tex[5], 0))).rrrr;\n"
    "    case 6: return texture(u_tex[6], px / vec2(textureSize(u_tex[6], 0))).rrrr;\n"
    "    default: return texture(u_tex[7], px / vec2(textureSize(u_tex[7], 0))).rrrr;\n"
    "    }\n"
    "}\n"
    "vec4 texel_hd(int i, vec2 st, int filt) {\n"
    "    if (u_hd[i] <= 0.0) return texel(i, st, filt);\n"
    "    vec2 f = vec2(shiftc(st.x, u_tA[i].x), shiftc(st.y, u_tA[i].y)) - u_tul[i];\n"
    "    return hdfetch(i, (f + 0.5) * u_hd[i]);\n"
    "}\n";

static const char *cc_rgb(int k) {
    static const char *m[] = { "comb.rgb", "t0.rgb", "t1.rgb", "u_prim.rgb", "shade.rgb", "u_env.rgb",
                               "vec3(1.0)", "vec3(0.0)", "vec3(comb.a)", "vec3(t0.a)", "vec3(t1.a)",
                               "vec3(u_prim.a)", "vec3(shade.a)", "vec3(u_env.a)", "vec3(lodf)",
                               "vec3(u_primlod)", "vec3(noise)" };
    return m[k];
}

static const char *cc_a(int k) {
    static const char *m[] = { "comb.a", "t0.a", "t1.a", "u_prim.a", "shade.a", "u_env.a", "1.0", "0.0",
                               "comb.a", "t0.a", "t1.a", "u_prim.a", "shade.a", "u_env.a", "lodf",
                               "u_primlod", "noise" };
    return m[k];
}

static char *sb;
static size_t sb_len, sb_cap;
static void cat(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void cat(const char *fmt, ...) {
    va_list ap;
    for (;;) {
        va_start(ap, fmt);
        int n = vsnprintf(sb + sb_len, sb_cap - sb_len, fmt, ap);
        va_end(ap);
        if (n >= 0 && sb_len + (size_t)n < sb_cap) {
            sb_len += n;
            return;
        }
        sb_cap = sb_cap ? sb_cap * 2 : 16384;
        sb = realloc(sb, sb_cap);
    }
}

static GLuint compile(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        host_log("%s\n", src);
        host_fatal("gl: shader: %s", log);
    }
    return s;
}

static const char *bl_color(int sel, const char *in) {
    return sel == 0 ? in : sel == 2 ? "u_blend.rgb" : sel == 3 ? "u_fog.rgb" : "vec3(0.0)";
}

static const char *bl_alpha(int sel) {
    return sel == 0 ? "comb.a" : sel == 1 ? "u_fog.a" : sel == 2 ? "shade.a" : "0.0";
}

static Prog *get_prog(const ProgKey *key) {
    unsigned h = prog_slot(key);
    for (; prog_hash[h]; h = (h + 1) & 2047)
        if (!memcmp(&progs[prog_hash[h] - 1].key, key, sizeof *key))
            return &progs[prog_hash[h] - 1];
    if (nprogs == 1024)
        host_fatal("gl: too many shader programs");
    Prog *p = &progs[nprogs++];
    prog_hash[h] = (int16_t)nprogs;
    host_perf_push(PERF_SHADER);
    memset(p, 0, sizeof *p);
    p->key = *key;
    sb_len = 0;
    cat("%s", fs_common);
    if (key->hd)
        cat("%s", fs_hd);
    cat("void main() {\n");
    cat("    vec4 shade = v_col, t0 = vec4(0.0), t1 = vec4(0.0), comb = vec4(0.0);\n");
    cat("    float lodf = 1.0;\n");
    cat("    float noise = fract(sin(dot(floor(gl_FragCoord.xy) + u_seed, vec2(12.9898, 78.233))) * 43758.5453);\n");
    cat("    int s0 = 0, s1 = 1;\n");
    if (key->lod) {
        cat("    vec2 dx = dFdx(v_st), dy = dFdy(v_st);\n");
        cat("    float lod = max(max(abs(dx.x), abs(dx.y)), max(abs(dy.x), abs(dy.y))) * u_lodscale;\n");
        cat("    if (lod < 1.0) { s1 = u_levels == 0 ? 0 : 1; lodf = u_levels == 0 ? 1.0 : 0.0; }\n");
        cat("    else { int l = int(floor(log2(lod)));\n");
        cat("        if (l >= u_levels) { s0 = s1 = u_levels; lodf = 1.0; }\n");
        cat("        else { s0 = l; s1 = l + 1; lodf = floor((lod / exp2(float(l)) - 1.0) * 256.0) / 256.0; } }\n");
    }
    if (key->kind == K_FILL) {
        cat("    o_col = vec4(v_col.rgb, 1.0);\n}\n");
        goto done;
    }
    if (key->textured) {
        const char *fn = key->hd ? "texel_hd" : "texel";
        cat("    vec2 st = clamp(v_st, v_box.xy, v_box.zw);\n");
        cat("    t0 = %s(s0, st, %d);\n", fn, key->filt);
        if (key->tex1)
            cat("    t1 = %s(s1, st, %d);\n", fn, key->filt);
    }
    if (key->cyc == 2) {                            /* copy */
        if ((key->om_l & 3) == 1)
            cat("    if (t0.a * 255.0 < 128.0) discard;\n");
        cat("    o_col = t0;\n}\n");
        goto done;
    }
    {
        /* (the decoding reads the cycle type too: the key's, not the RDP's now,
           when a program is made ahead) */
        uint32_t w0 = gs.cc0, w1 = gs.cc1, oh = gs.om_h;
        gs.cc0 = key->cc0;
        gs.cc1 = key->cc1;
        gs.om_h = (gs.om_h & ~(3u << 20)) | (uint32_t)key->cyc << 20;
        uint8_t idx[2][8];
        gfx_cc_decode(idx);
        gs.cc0 = w0;
        gs.cc1 = w1;
        gs.om_h = oh;
        int ncyc = key->cyc == 1 ? 2 : 1;
        for (int c = 0; c < ncyc; c++) {
            const uint8_t *k = idx[c];
            cat("    comb = vec4(clamp((%s - %s) * %s + %s, 0.0, 1.0),\n", cc_rgb(k[0]), cc_rgb(k[1]), cc_rgb(k[2]),
                cc_rgb(k[3]));
            cat("                clamp((%s - %s) * %s + %s, 0.0, 1.0));\n", cc_a(k[4]), cc_a(k[5]), cc_a(k[6]),
                cc_a(k[7]));
        }
        uint32_t l = key->om_l;
        if ((l & 3) == 1)
            cat("    if (comb.a * 255.0 < u_blend.a * 255.0) discard;\n");
        if (gfx_cvg_alpha_min(l))                   /* coverage from alpha */
            cat("    if (comb.a * 255.0 < %d.0) discard;\n", gfx_cvg_alpha_min(l));
        int bcyc = ncyc - ((l & 0x4000) ? 0 : 1);
        cat("    vec3 inp = comb.rgb;\n");
        cat("    o_col = vec4(inp, 1.0);\n");
        for (int k = 0; k < bcyc; k++) {
            int sh = k == 0 ? 0 : 2;
            int P = (l >> (30 - sh)) & 3, A = (l >> (26 - sh)) & 3, M = (l >> (22 - sh)) & 3, B = (l >> (18 - sh)) & 3;
            cat("    { float a = %s;\n", bl_alpha(A));
            /* the destination's alpha is 1 (RGBA5551 with coverage full) */
            cat("      float b = %s;\n", B == 0 ? "1.0 - a" : B == 3 ? "0.0" : "1.0");
            cat("      float sum = a + b;\n");
            if (P == 1 || M == 1) {
                if (k != bcyc - 1)
                    host_log("gl: blender cycle %d reads memory before the last (om_l %08X)\n", k, l);
                if (P == 1 && M == 1) {
                    p->blend = 2;
                } else if (M == 1) {
                    p->blend = 1;
                    cat("      o_col = sum > 0.0 ? vec4(%s * (a / sum), b / sum) : vec4(%s, 0.0);\n",
                        bl_color(P, "inp"), bl_color(P, "inp"));
                } else {
                    p->blend = 1;
                    cat("      o_col = sum > 0.0 ? vec4(%s * (b / sum), a / sum) : vec4(0.0, 0.0, 0.0, 1.0);\n",
                        bl_color(M, "inp"));
                }
                cat("    }\n");
                break;
            }
            cat("      inp = sum > 0.0 ? (%s * a + %s * b) / sum : %s;\n", bl_color(P, "inp"), bl_color(M, "inp"),
                bl_color(P, "inp"));
            cat("      o_col = vec4(inp, 1.0); }\n");
        }
        if (key->quant && !p->blend)
            cat("    { vec3 q = floor(o_col.rgb * 255.0 / 8.0); o_col.rgb = (q * 8.0 + floor(q / 4.0)) / 255.0; }\n");
        cat("}\n");
    }
done:;
    GLuint vs = compile(GL_VERTEX_SHADER, vs_src), fs = compile(GL_FRAGMENT_SHADER, sb);
    p->prog = glCreateProgram();
    glAttachShader(p->prog, vs);
    glAttachShader(p->prog, fs);
    glLinkProgram(p->prog);
    GLint ok;
    glGetProgramiv(p->prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p->prog, sizeof log, NULL, log);
        host_fatal("gl: link: %s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLC_DIRTY();
    glUseProgram(p->prog);
    for (int i = 0; i < 8; i++) {
        char name[16];
        snprintf(name, sizeof name, "u_tex[%d]", i);
        glUniform1i(glGetUniformLocation(p->prog, name), i);
    }
#define U(n) p->n = glGetUniformLocation(p->prog, #n)
    U(u_vp); U(u_zbias); U(u_prim); U(u_env); U(u_blend); U(u_fog); U(u_primlod); U(u_seed);
    U(u_tA); U(u_tB); U(u_tul); U(u_levels); U(u_lodscale); U(u_hd);
#undef U
    if (host_verbose > 1)
        host_log("gl: program %d: cc %06X %08X om_l %08X cyc %d\n", nprogs, key->cc0, key->cc1, key->om_l, key->cyc);
    /* PORT_GL_PROGS=FILE: each new program's key, for gfx_gl_progs.h */
    static FILE *keys = (FILE *)1;
    if (keys == (FILE *)1) {
        const char *e = getenv("PORT_GL_PROGS");
        keys = e ? fopen(e, "a") : NULL;
    }
    if (keys) {
        fprintf(keys, "0x%06X, 0x%08X, 0x%08X, %d, %d, %d, %d, %d, %d\n", key->cc0, key->cc1, key->om_l, key->cyc,
                key->textured, key->tex1, key->lod, key->filt, key->kind);
        fflush(keys);
    }
    host_perf_pop();
    return p;
}

/* ---- draw state and batches ------------------------------------------------------ */

typedef struct {
    Prog *prog;
    Target *target;
    int depth;                  /* bit 0 test, bit 1 write */
    float zbias;
    int sc[4];
    GLuint tex[8];
    int ntex;
    float prim[4], env[4], blend[4], fog[4], primlod;
    int tA[8][4], tB[8][4];
    float tul[8][2];
    int levels;
    float hd[8];                /* --hd-text: HDTEXT_K for a font glyph's unit (tile_texture) */
} DrawState;

/* what a draw state is built from: when this doesn't change, neither does it */
typedef struct {
    int kind, tile;
    uint32_t om_h, om_l, cc0, cc1, geom, fill;
    uint8_t prim[4], env[4], blend[4], fog[4];
    uint8_t prim_lod;
    int tex_on, tex_tile, tex_levels;
    int sc[4];
    uint32_t tmem_gen, tile_gen, cimg, zimg;
} RawState;

static RawState raw_last;
static int raw_valid;
static DrawState ds_cur, ds_batch;
static int batch_valid;

typedef struct {
    float x, y, z, w, s, t, r, g, b, a;     /* GfxVtx's */
    float box[4];                           /* s, t kept within: low s, t, high s, t */
} GLVtx;

static const float no_box[4] = { -1e9f, -1e9f, 1e9f, 1e9f };

static GLVtx *vbuf;
static int vcount, vcap;
static GLuint vao, vbo;
static unsigned long long st_draws, st_verts, st_flushes_state;

/* The batches wait, their vertices one after another in vbuf, until
   something needs them drawn (flush: the task's end, a clear, a read-back,
   a texture or target about to go): then the vertices go to the GPU in one
   upload, and each batch is a draw of its range. */
typedef struct {
    DrawState d;
    int first, count;
} Batch;
static Batch *batches;
static int nbatches, batches_cap, batch_first;

static void glc_check(void) {
    if (glc.valid)
        return;
    memset(&glc, 0xFF, sizeof glc);                 /* all unknown: set at the next use */
    glc.valid = 1;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
}

#define U_SET(field, n, call)                                               \
    if (!p->uv || memcmp(p->u.field, d->field, (n) * sizeof p->u.field[0])) { \
        memcpy(p->u.field, d->field, (n) * sizeof p->u.field[0]);           \
        call;                                                               \
    }
#define U_SET1(field, val, call)                                            \
    if (!p->uv || p->u.field != (val)) {                                    \
        p->u.field = (val);                                                 \
        call;                                                               \
    }

static void apply_and_draw(const DrawState *d, int first, int count) {
    Prog *p = d->prog;
    GLuint fbo = target_fbo(d->target);            /* (first: a twin may be made) */
    c_fbo(fbo);
    c_viewport(tw_px, th_px);
    c_scissor(d->sc[0], (TH - d->sc[3]) * scale, d->sc[2] - d->sc[0], (d->sc[3] - d->sc[1]) * scale);
    c_cmask(0);
    c_depth(d->depth != 0, (d->depth & 1) ? GL_LEQUAL : GL_ALWAYS);
    c_depth_mask(d->depth ? (d->depth & 2) != 0 : 0);
    c_blend(p->blend);
    c_prog(p->prog);
    float vp[2] = { TW + 2 * wide_off, TH };
    if (!p->uv || p->u.vp[0] != vp[0] || p->u.vp[1] != vp[1]) {
        p->u.vp[0] = vp[0];
        p->u.vp[1] = vp[1];
        glUniform2fv(p->u_vp, 1, vp);
    }
    U_SET1(zbias, d->zbias, glUniform1f(p->u_zbias, d->zbias))
    U_SET(prim, 4, glUniform4fv(p->u_prim, 1, d->prim))
    U_SET(env, 4, glUniform4fv(p->u_env, 1, d->env))
    U_SET(blend, 4, glUniform4fv(p->u_blend, 1, d->blend))
    U_SET(fog, 4, glUniform4fv(p->u_fog, 1, d->fog))
    U_SET1(primlod, d->primlod, glUniform1f(p->u_primlod, d->primlod))
    float seed = (float)(frame_no % 997);
    U_SET1(seed, seed, glUniform1f(p->u_seed, seed))
    U_SET1(lodscale, (float)scale, glUniform1f(p->u_lodscale, (float)scale))
    U_SET1(levels, d->levels, glUniform1i(p->u_levels, d->levels))
    if (d->ntex) {
        /* (only the tiles it has: the shader reads no others) */
        if (!p->uv || p->u.ntex != d->ntex || memcmp(p->u.tA, d->tA, d->ntex * sizeof d->tA[0])) {
            memcpy(p->u.tA, d->tA, d->ntex * sizeof d->tA[0]);
            glUniform4iv(p->u_tA, d->ntex, &d->tA[0][0]);
        }
        if (!p->uv || p->u.ntex != d->ntex || memcmp(p->u.tB, d->tB, d->ntex * sizeof d->tB[0])) {
            memcpy(p->u.tB, d->tB, d->ntex * sizeof d->tB[0]);
            glUniform4iv(p->u_tB, d->ntex, &d->tB[0][0]);
        }
        if (!p->uv || p->u.ntex != d->ntex || memcmp(p->u.tul, d->tul, d->ntex * sizeof d->tul[0])) {
            memcpy(p->u.tul, d->tul, d->ntex * sizeof d->tul[0]);
            glUniform2fv(p->u_tul, d->ntex, &d->tul[0][0]);
        }
        if (p->key.hd && (!p->uv || p->u.ntex != d->ntex || memcmp(p->u.hd, d->hd, d->ntex * sizeof d->hd[0]))) {
            memcpy(p->u.hd, d->hd, d->ntex * sizeof d->hd[0]);
            glUniform1fv(p->u_hd, d->ntex, d->hd);
        }
        p->u.ntex = d->ntex;
    }
    p->uv = 1;
    if (ipass_gl)                       /* (again: making the twin, target_fbo above, starts it over) */
        d->target->idrawn |= 1u << (ipass_gl - 1);
    for (int i = 0; i < d->ntex; i++)
        c_tex(i, d->tex[i]);
    glDrawArrays(GL_TRIANGLES, first, count);
    st_draws++;
    st_verts += count;
}

/* the batch being built is complete */
static void batch_close(void) {
    if (!batch_valid || vcount == batch_first)
        return;
    if (nbatches == batches_cap) {
        batches_cap = batches_cap ? batches_cap * 2 : 1024;
        batches = realloc(batches, (size_t)batches_cap * sizeof *batches);
    }
    Batch *b = &batches[nbatches++];
    b->d = ds_batch;
    b->first = batch_first;
    b->count = vcount - batch_first;
    batch_first = vcount;
    if (ipass_gl)                       /* (drawn as far as anyone asking is concerned) */
        ds_batch.target->idrawn |= 1u << (ipass_gl - 1);
    else
        ds_batch.target->dirty = 1;
}

static void flush(void) {
    batch_close();
    if (nbatches) {
        host_perf_push(PERF_GL);
        glc_check();
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vcount * sizeof(GLVtx), vbuf, GL_STREAM_DRAW);
        for (int i = 0; i < nbatches; i++)
            apply_and_draw(&batches[i].d, batches[i].first, batches[i].count);
        host_perf_pop();
    }
    nbatches = 0;
    vcount = batch_first = 0;
}

/* returns 0 when nothing changed since the last call */
static int build_state(int kind, int tile) {
    RawState r;
    memset(&r, 0, sizeof r);
    r.kind = kind; r.tile = tile;
    r.om_h = gs.om_h; r.om_l = gs.om_l; r.cc0 = gs.cc0; r.cc1 = gs.cc1; r.geom = gs.geom; r.fill = gs.fill;
    memcpy(r.prim, gs.prim, 4); memcpy(r.env, gs.env, 4); memcpy(r.blend, gs.blend, 4); memcpy(r.fog, gs.fog, 4);
    r.prim_lod = gs.prim_lod;
    r.tex_on = gs.tex_on; r.tex_tile = gs.tex_tile; r.tex_levels = gs.tex_levels;
    r.sc[0] = gs.sc_x0; r.sc[1] = gs.sc_y0; r.sc[2] = gs.sc_x1; r.sc[3] = gs.sc_y1;
    r.tmem_gen = gs.tmem_gen; r.tile_gen = gs.tile_gen; r.cimg = gs.cimg_addr; r.zimg = gs.zimg_addr;
    if (raw_valid && !memcmp(&r, &raw_last, sizeof r))
        return 0;
    raw_last = r;
    raw_valid = 1;

    DrawState d;
    memset(&d, 0, sizeof d);
    ProgKey key;
    memset(&key, 0, sizeof key);
    key.kind = kind;
    key.cyc = gfx_cycle_type();
    key.quant = quantize;
    if (kind == K_FILL && key.cyc != 3)
        key.kind = K_TRI;                           /* a 1/2-cycle fill rect runs the combiner */
    if (key.kind != K_FILL) {
        key.cc0 = gs.cc0;
        key.cc1 = gs.cc1;
        key.om_l = gs.om_l & 0xFFFF5003u;           /* blender, FORCE_BL, CVG_X_ALPHA, alpha compare */
        if ((gs.om_l & 0x5000) == 0x5000)
            key.om_l |= gs.om_l & 0x8;              /* and AA_EN, where gfx_cvg_alpha_min reads it */
        key.textured = kind == K_TEXRECT || (kind == K_TRI && gs.tex_on);
        key.tex1 = key.textured && key.cyc < 2 && gfx_uses_tex1();
        key.lod = key.textured && key.cyc < 2 && gfx_lod_on();
        key.filt = key.textured ? gfx_filter_mode() : 0;
    }
    if (key.cyc == 3 && kind != K_FILL)
        key.cyc = 0;
    /* (the textures first: a font glyph's, --hd-text, needs its program) */
    if (key.textured) {
        int base = kind == K_TEXRECT ? tile : gs.tex_tile;
        int n = key.lod ? gs.tex_levels + 2 : key.tex1 ? 2 : 1;
        if (n > 8) n = 8;
        d.ntex = n;
        for (int i = 0; i < n; i++) {
            int ti = (base + i) & 7, hd;
            const GfxTile *t = &gs.tile[ti];
            d.tex[i] = tile_texture(ti, &hd);
            d.hd[i] = (float)hd;
            key.hd |= hd != 0;
            d.tA[i][0] = t->shifts; d.tA[i][1] = t->shiftt; d.tA[i][2] = t->masks; d.tA[i][3] = t->maskt;
            d.tB[i][0] = t->cms; d.tB[i][1] = t->cmt;
            d.tB[i][2] = (t->lrs - t->uls) >> 2; d.tB[i][3] = (t->lrt - t->ult) >> 2;
            d.tul[i][0] = t->uls * 0.25f; d.tul[i][1] = t->ult * 0.25f;
        }
    }
    d.prog = get_prog(&key);
    d.target = cur_target;
    if (kind == K_TRI) {
        int zok = gs.zimg_addr != 0;
        int zcmp = (gs.geom & 1) && (gs.om_l & 0x10) && zok;
        int zupd = (gs.geom & 1) && (gs.om_l & 0x20) && zok;
        d.depth = zcmp | zupd << 1;
        d.zbias = ((gs.om_l >> 10) & 3) == 3 ? 0.0005f : 0;
    }
    /* the scissor in target pixels across (a full-width one covers the
       wide target), lines down */
    int sx0, sx1;
    gfx_wide_span(gs.sc_x0, gs.sc_x1, &sx0, &sx1);
    d.sc[0] = px_x(sx0); d.sc[1] = gs.sc_y0; d.sc[2] = px_x(sx1); d.sc[3] = gs.sc_y1;
    for (int i = 0; i < 4; i++) {
        d.prim[i] = gs.prim[i] / 255.0f;
        d.env[i] = gs.env[i] / 255.0f;
        d.blend[i] = gs.blend[i] / 255.0f;
        d.fog[i] = gs.fog[i] / 255.0f;
    }
    d.primlod = gs.prim_lod / 255.0f;
    d.levels = gs.tex_levels;
    ds_cur = d;
    return 1;
}

static void begin(int kind, int tile) {
    int changed = build_state(kind, tile);
    if (!batch_valid || (changed && memcmp(&ds_cur, &ds_batch, sizeof ds_cur))) {
        batch_close();
        ds_batch = ds_cur;
        batch_valid = 1;
        st_flushes_state++;
    }
}

static GLVtx *push(int n) {
    if (vcount + n > vcap) {
        vcap = vcap ? vcap * 2 : 65536;
        vbuf = realloc(vbuf, (size_t)vcap * sizeof(GLVtx));
    }
    GLVtx *v = vbuf + vcount;
    vcount += n;
    return v;
}

/* ---- the back-end interface --------------------------------------------------------- */

int gfx_gl_owns_target(void) {
    if (gs.cimg_addr != cur_cimg) {
        cur_cimg = gs.cimg_addr;
        cur_target = gs.cimg_siz == 2 && gs.cimg_w == TW && gs.cimg_addr != gs.zimg_addr ? get_target(gs.cimg_addr)
                                                                                          : NULL;
        raw_valid = 0;
    }
    return cur_target != NULL;
}

void gfx_gl_tri(const GfxVtx *s, int n, const float *flat) {
    begin(K_TRI, 0);
    GLVtx *v = push(3 * (n - 2));
    for (int i = 1; i + 1 < n; i++) {
        const GfxVtx *p[3] = { &s[0], &s[i], &s[i + 1] };
        for (int j = 0; j < 3; j++, v++) {
            memcpy(v, p[j], sizeof *p[j]);
            memcpy(v->box, no_box, sizeof v->box);
            v->x += wide_off;
            if (flat) {
                v->r = flat[0]; v->g = flat[1]; v->b = flat[2]; v->a = flat[3];
            }
        }
    }
}

static void quad(float x0, float y0, float x1, float y1, float s00, float t00, float s10, float t10,
                 float s01, float t01, float s11, float t11, const float *col, const float *box) {
    GLVtx *v = push(6);
    x0 += wide_off;
    x1 += wide_off;
    GLVtx c[4] = {
        { x0, y0, 0, 1, s00, t00, col[0], col[1], col[2], col[3], { box[0], box[1], box[2], box[3] } },
        { x1, y0, 0, 1, s10, t10, col[0], col[1], col[2], col[3], { box[0], box[1], box[2], box[3] } },
        { x0, y1, 0, 1, s01, t01, col[0], col[1], col[2], col[3], { box[0], box[1], box[2], box[3] } },
        { x1, y1, 0, 1, s11, t11, col[0], col[1], col[2], col[3], { box[0], box[1], box[2], box[3] } },
    };
    v[0] = c[0]; v[1] = c[1]; v[2] = c[2];
    v[3] = c[1]; v[4] = c[3]; v[5] = c[2];
}

void gfx_gl_fill_rect(int x0, int y0, int x1, int y1) {
    begin(K_FILL, 0);
    float col[4] = { 0, 0, 0, 0 };
    if (gfx_cycle_type() == 3) {
        uint16_t c = gs.fill >> 16;
        col[0] = ((c >> 11) & 31) * 255 / 31;
        col[1] = ((c >> 6) & 31) * 255 / 31;
        col[2] = ((c >> 1) & 31) * 255 / 31;
        col[3] = 255;
    }
    quad(x0, y0, x1, y1, 0, 0, 0, 0, 0, 0, 0, 0, col, no_box);
}

void gfx_gl_tex_rect(int x0, int y0, int x1, int y1, int tile, float sa, float ta, float dsdx, float dtdy,
                     int flip) {
    begin(K_TEXRECT, tile);
    /* the software renderer samples pixel x at s(x); the GPU samples at the
       pixel's center, so s(X) = s(X - 0.5), plus a hair so that exact
       texel boundaries land the same way */
    const float e = 1.0f / 512;
    float c[4] = { 0, 0, 0, 0 };
    float s[4], t[4];
    for (int i = 0; i < 4; i++) {
        float dx = (i & 1 ? x1 - x0 : 0) - 0.5f, dy = (i & 2 ? y1 - y0 : 0) - 0.5f;
        s[i] = sa + (flip ? dy : dx) * dsdx + e;
        t[i] = ta + (flip ? dx : dy) * dtdy + e;
    }
    /* Above 1x the pixels at a rectangle's edges sample between its first
       or last texel and the one beyond, which the RDP never reads there: a
       picture drawn as rectangles (the results screen's medal, two 64x32
       halves whose tile wraps at 64 rows) shows a seam where the last row
       of a half is filtered with what lies past it.  So s and t stay within
       what the 1x pixels sample. */
    float sn = (float)((flip ? y1 - y0 : x1 - x0) - 1), tn = (float)((flip ? x1 - x0 : y1 - y0) - 1);
    float s_end = sa + sn * dsdx, t_end = ta + tn * dtdy;
    float box[4] = { fminf(sa, s_end) + e, fminf(ta, t_end) + e, fmaxf(sa, s_end) + e, fmaxf(ta, t_end) + e };
    quad(x0, y0, x1, y1, s[0], t[0], s[1], t[1], s[2], t[2], s[3], t[3], c, box);
}

void gfx_gl_zclear(int x0, int y0, int x1, int y1) {
    flush();
    Target *t = cur_target ? cur_target : ntargets ? &targets[0] : NULL;
    if (!t)
        return;
    GLuint fbo = target_fbo(t);
    c_fbo(fbo);
    c_scissor(px_x(x0), (TH - y1) * scale, (x1 - x0) * scale, (y1 - y0) * scale);
    c_depth_mask(1);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void gfx_gl_clear_rect(int x0, int y0, int x1, int y1) {
    flush();
    if (!cur_target)
        return;
    GLuint fbo = target_fbo(cur_target);
    c_fbo(fbo);
    c_scissor(px_x(x0), (TH - y1) * scale, (x1 - x0) * scale, (y1 - y0) * scale);
    c_cmask(1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!ipass_gl)
        cur_target->dirty = 1;
}

void gfx_gl_texture_source(uint32_t addr) {
    for (int i = 0; i < ntargets; i++) {
        Target *t = &targets[i];
        if (addr >= t->addr && addr < t->addr + TW * TH * 2 && t->dirty) {
            flush();
            read_back(t);
            if (host_verbose)
                host_log("gl: texture from target %08X: read back\n", t->addr);
        }
    }
}

void gfx_gl_task_begin(void) {
    raw_valid = 0;
    batch_valid = 0;
    cur_cimg = 1;
}

void gfx_gl_task_end(void) {
    flush();
    batch_valid = 0;
    if (readback_all && !ipass_gl)
        for (int i = 0; i < ntargets; i++)
            if (targets[i].dirty)
                read_back(&targets[i]);
}

void gfx_gl_interp(int k) {
    flush();
    batch_valid = 0;
    raw_valid = 0;
    ipass_gl = k + 1;
}

unsigned gfx_gl_interp_swap(uint32_t fb) {
    Target *t = find_target(fb);
    if (!t)
        return 0;
    unsigned d = t->idrawn;
    t->idrawn = 0;
    return d;
}

/* ---- window, presentation --------------------------------------------------------- */

unsigned gfx_gl_window_flags(void) {
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    /* macOS has a core profile only forward-compatible (4.1, for 3.3) */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
#ifdef __EMSCRIPTEN__
    /* the canvas in the display's pixels (the page sizes it by CSS), so
       that the scale follows those */
    return SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    return SDL_WINDOW_OPENGL;
#endif
}

int gfx_gl_init(SDL_Window *w) {
    win = w;
    ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        host_log("gl: no OpenGL 3.3 context (%s); using the software renderer\n", SDL_GetError());
        return 0;
    }
    SDL_GL_MakeCurrent(win, ctx);
#ifndef __EMSCRIPTEN__      /* (there the page shows what was drawn when the loop yields) */
    SDL_GL_SetSwapInterval(0);      /* the host loop paces the frames */
#endif
    host_log("gl: %s, %s\n", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
    const char *e = getenv("PORT_GL_READBACK");
    readback_all = e && *e && *e != '0';
    e = getenv("PORT_GL_QUANT");
    quantize = e && *e && *e != '0';
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLVtx), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVtx), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GLVtx), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GLVtx), (void *)(10 * sizeof(float)));
    /* depth clamped rather than clipped at the far plane (in WebGL only
       with EXT_depth_clamp, which Firefox hasn't); PORT_GL_DEPTH_CLAMP=0
       leaves it off, to see what a browser without it draws */
    e = getenv("PORT_GL_DEPTH_CLAMP");
    if (!e || *e != '0')
        glEnable(GL_DEPTH_CLAMP);
    if (!glIsEnabled(GL_DEPTH_CLAMP))
        host_log("gl: no depth clamp: clipped at the far plane\n");
    while (glGetError() != GL_NO_ERROR)     /* (the enum, where it is unknown) */
        ;
    glClearColor(0, 0, 0, 1);       /* (every clear's) */
    glClearDepth(1.0);
    GLC_DIRTY();
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    set_geometry(want_scale(dw, dh, gfx_aspect_of(dw, dh)), gfx_aspect_of(dw, dh));
    glGenFramebuffers(1, &view.fbo);
    glGenTextures(1, &view.tex);
    target_storage(&view);
    gfx_gl_enabled = 1;
    return 1;
}

static void save_bmp(const char *path, const uint8_t *rgba, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    uint32_t rowbytes = (uint32_t)w * 3, pad4 = (4 - rowbytes % 4) % 4, size = (rowbytes + pad4) * (uint32_t)h;
    uint8_t hdr[54] = { 'B', 'M' };
    uint32_t v[] = { 54 + size, 0, 54, 40, (uint32_t)w, (uint32_t)h, 1 | (24 << 16), 0, size, 2835, 2835, 0, 0 };
    memcpy(hdr + 2, v, sizeof v);
    fwrite(hdr, 1, 54, f);
    for (int y = 0; y < h; y++) {                   /* GL rows are bottom-up, as BMP's */
        for (int x = 0; x < w; x++) {
            const uint8_t *p = rgba + 4 * ((size_t)y * w + x);
            uint8_t bgr[3] = { p[2], p[1], p[0] };
            fwrite(bgr, 1, 3, f);
        }
        fwrite("\0\0\0", 1, pad4, f);
    }
    fclose(f);
}

/* PORT_ADAPT=1 (the page's default): the internal resolution follows the
   GPU.  A fence after each picture tells whether the GPU has finished the
   picture before last by the time the next is presented; when it hasn't in
   a quarter of the retraces of a two-second window, the scale goes down by
   one (as far as 1), and after a minute of none it may go up again, as far
   as the window's or --scale's. */
static int adapt = -1, adapt_cap = 16, adapt_n, adapt_late, adapt_quiet;
static double adapt_down_at = -1e9;
static GLsync fences[2];
static unsigned fence_hist;                 /* the last 32 presents: 1 where the GPU was behind */

/* the GPU has been behind lately (main.c: then a late retrace is the GPU's
   to fix, with the resolution, not the in-between pictures') */
int gfx_gl_gpu_behind(void) { return adapt > 0 && __builtin_popcount(fence_hist) >= 4; }

static int adapt_scale(int want) {
    if (adapt < 0) {
        const char *e = getenv("PORT_ADAPT");
#ifdef __EMSCRIPTEN__
        adapt = !e || (*e && *e != '0');
#else
        adapt = e && *e && *e != '0';
#endif
    }
    if (!adapt)
        return want;
    if (adapt_cap > want || adapt_down_at < 0)     /* (until the GPU first fell behind: the window's) */
        adapt_cap = want;
    if (fences[0]) {                        /* the picture before last */
        GLenum r = glClientWaitSync(fences[0], 0, 0);
        adapt_late += r == GL_TIMEOUT_EXPIRED;
        fence_hist = fence_hist << 1 | (r == GL_TIMEOUT_EXPIRED);
        glDeleteSync(fences[0]);
    }
    fences[0] = fences[1];
    fences[1] = NULL;
    if (++adapt_n == 120) {
        double now = host_perf_now();
        if (adapt_late * 4 > adapt_n && adapt_cap > 1) {
            adapt_cap--;
            adapt_down_at = now;
            host_log("gl: the GPU is behind (%d of %d retraces): scale %d\n", adapt_late, adapt_n, adapt_cap);
        } else if (adapt_late == 0 && now - adapt_down_at > 60000 && ++adapt_quiet >= 30 && adapt_cap < want) {
            adapt_cap++;
            adapt_quiet = 0;
            host_log("gl: the GPU keeps up: scale %d\n", adapt_cap);
        }
        if (adapt_late)
            adapt_quiet = 0;
        adapt_n = adapt_late = 0;
    }
    return adapt_cap < want ? adapt_cap : want;
}

void gfx_gl_present(uint32_t vi_fb, int vi_width, const char *shot, int twin) {
    frame_no++;
    GLC_DIRTY();
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    set_geometry(adapt_scale(want_scale(dw, dh, gfx_aspect_of(dw, dh))), gfx_aspect_of(dw, dh));
    /* the game's shader programs, a few a retrace (at most 4 ms of them,
       at least one) until all are there: compiling one when it is first
       drawn with stalls that frame, by tens of milliseconds in a browser
       (with the default --filter: the table's filter is its) */
    if (prog_ahead < sizeof prog_table / sizeof prog_table[0] && gfx_filter == 0) {
        double t0 = host_perf_now();
        do {
            ProgKey k = prog_table[prog_ahead++];
            k.quant = quantize;
            get_prog(&k);
        } while (prog_ahead < sizeof prog_table / sizeof prog_table[0] && host_perf_now() - t0 < 4.0);
    }
    Target *t = vi_fb && vi_width == TW ? find_target(vi_fb) : NULL;
    if (vi_fb && !t) {
        upload_rdram(&view, vi_fb, vi_width > 0 && vi_width <= 640 ? vi_width : TW);
        t = &view;
    }
    /* --interpolate: one of the frame's in-between images (gfx.c) */
    GLuint src = t ? t->fbo : 0;
    if (t && t != &view && twin >= 0 && twin < GFX_TWINS && t->ifbo[twin])
        src = t->ifbo[twin];
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(1, 1, 1, 1);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (t) {
        /* the target's aspect (4:3, or wider), as large as fits */
        int w = dw, h = (int)((int64_t)dw * th_px / tw_px);
        if (h > dh) { h = dh; w = (int)((int64_t)dh * tw_px / th_px); }
        int x = (dw - w) / 2, y = (dh - h) / 2;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
        glBlitFramebuffer(0, 0, tw_px, th_px, x, y, x + w, y + h, GL_COLOR_BUFFER_BIT,
                          w == tw_px && h == th_px ? GL_NEAREST : GL_LINEAR);
    }
    if (shot) {
        int w = tw_px, h = th_px;
        uint8_t *buf = calloc((size_t)w * h, 4);
        if (t) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        }
        save_bmp(shot, buf, w, h);
        free(buf);
    }
    SDL_GL_SwapWindow(win);
    if (adapt > 0)
        fences[1] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    if (host_verbose && frame_no % 300 == 0)
        host_log("gl: %llu draws, %llu vertices, %llu textures decoded (%llu hits), %d programs\n", st_draws,
                 st_verts, st_tex_decoded, st_tex_hits, nprogs);
}

#endif
