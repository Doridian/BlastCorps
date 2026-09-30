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
#include <epoxy/gl.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "gfx.h"

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
    /* --interpolate: the twin the in-between pass draws into (its own depth
       buffer), and whether it holds the in-between frame of what the target
       holds, not yet shown */
    GLuint ifbo, itex;
    int idrawn, iready;
} Target;

static Target targets[8];
static int ntargets;
static GLuint depth_rb, idepth_rb;
static int ipass_gl;            /* drawing the in-between frame */
static Target *cur_target;
static uint32_t cur_cimg = 1;
static Target view;             /* RDRAM shown as it is */

#define TW 320
#define TH 240

static void target_storage(Target *t) {
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

static void twin_storage(Target *t) {
    if (!idepth_rb) {
        glGenRenderbuffers(1, &idepth_rb);
        glBindRenderbuffer(GL_RENDERBUFFER, idepth_rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, tw_px, th_px);
    }
    if (!t->ifbo) {
        glGenFramebuffers(1, &t->ifbo);
        glGenTextures(1, &t->itex);
    }
    glBindTexture(GL_TEXTURE_2D, t->itex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tw_px, th_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, t->ifbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->itex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, idepth_rb);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        host_fatal("gl: framebuffer incomplete");
    t->idrawn = t->iready = 0;
}

/* the framebuffer object a draw into t goes to */
static GLuint target_fbo(Target *t) {
    if (!ipass_gl)
        return t->fbo;
    if (!t->ifbo)
        twin_storage(t);
    return t->ifbo;
}

static Target *find_target(uint32_t addr) {
    for (int i = 0; i < ntargets; i++)
        if (targets[i].addr == addr)
            return &targets[i];
    return NULL;
}

static Target *get_target(uint32_t addr) {
    Target *t = find_target(addr);
    if (t)
        return t;
    if (ntargets == 8) {                            /* recycle the oldest */
        glDeleteFramebuffers(1, &targets[0].fbo);
        glDeleteTextures(1, &targets[0].tex);
        if (targets[0].ifbo) {
            glDeleteFramebuffers(1, &targets[0].ifbo);
            glDeleteTextures(1, &targets[0].itex);
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

/* the internal resolution: 240 * s lines, and aspect (gfx_aspect_of) wide */
static void set_geometry(int s, float aspect) {
    if (s < 1) s = 1;
    if (s > 16) s = 16;
    int off = gfx_wide_off_for(aspect), npx = (TW + 2 * off) * s;
    if (s == scale && npx == tw_px && depth_rb)
        return;
    int old_w = tw_px, old_h = th_px;
    if (!depth_rb)
        glGenRenderbuffers(1, &depth_rb);
    scale = s;
    gfx_set_wide(off);
    tw_px = npx;
    th_px = TH * s;
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, tw_px, th_px);
    if (idepth_rb) {                                /* the twins start over */
        glBindRenderbuffer(GL_RENDERBUFFER, idepth_rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, tw_px, th_px);
        for (int i = 0; i < ntargets; i++)
            if (targets[i].ifbo)
                twin_storage(&targets[i]);
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

static void flush(void);

static void tc_sweep(void) {
    int n = 0;
    flush();                    /* the pending batch may use them */
    for (int i = 0; i < TC_SIZE; i++)
        if (tcache[i].tex) {
            glDeleteTextures(1, &tcache[i].tex);
            n++;
        }
    memset(tcache, 0, sizeof tcache);
    tc_count = 0;
    if (host_verbose)
        host_log("gl: texture cache flushed (%d)\n", n);
}

static uint8_t decode_buf[1024 * 1024 * 4];

/* the GL texture for a tile as TMEM holds it now */
static GLuint tile_texture(int tile) {
    const GfxTile *t = &gs.tile[tile & 7];
    int w, h;
    gfx_tile_dims(t, &w, &h);
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
    if (!k)
        k = 1;
    unsigned i = (unsigned)(k ^ (k >> 32)) & (TC_SIZE - 1);
    while (tcache[i].key && tcache[i].key != k)
        i = (i + 1) & (TC_SIZE - 1);
    if (tcache[i].key == k) {
        tcache[i].used = frame_no;
        st_tex_hits++;
        return tcache[i].tex;
    }
    if (tc_count > TC_SIZE / 2) {
        tc_sweep();
        return tile_texture(tile);
    }
    st_tex_decoded++;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            gfx_fetch_texel(t, x, y, decode_buf + 4 * (y * w + x));
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, decode_buf);
    tcache[i].key = k;
    tcache[i].tex = tex;
    tcache[i].used = frame_no;
    tc_count++;
    return tex;
}

/* ---- shaders ------------------------------------------------------------------- */

enum { K_TRI, K_FILL, K_TEXRECT };

typedef struct {
    uint32_t cc0, cc1, om_l;
    uint8_t cyc, textured, tex1, lod, filt, kind, quant, pad;
} ProgKey;

typedef struct {
    ProgKey key;
    GLuint prog;
    int blend;                  /* 0 none, 1 (ONE, SRC_ALPHA), 2 keep the destination */
    GLint u_vp, u_zbias, u_prim, u_env, u_blend, u_fog, u_primlod, u_seed, u_tA, u_tB, u_tul, u_levels, u_lodscale;
} Prog;

static Prog progs[1024];
static int nprogs;

static const char *vs_src =
    "#version 330 core\n"
    "layout(location = 0) in vec4 a_pos;\n"
    "layout(location = 1) in vec2 a_st;\n"
    "layout(location = 2) in vec4 a_col;\n"
    "uniform vec2 u_vp;\n"
    "uniform float u_zbias;\n"
    "out vec4 v_col;\n"
    "out vec2 v_st;\n"
    "void main() {\n"
    "    float w = a_pos.w;\n"
    "    gl_Position = vec4((a_pos.x / u_vp.x * 2.0 - 1.0) * w, (1.0 - a_pos.y / u_vp.y * 2.0) * w,\n"
    "                       ((a_pos.z - u_zbias) * 2.0 - 1.0) * w, w);\n"
    "    v_col = a_col * (1.0 / 255.0);\n"
    "    v_st = a_st;\n"
    "}\n";

static const char *fs_common =
    "#version 330 core\n"
    "in vec4 v_col;\n"
    "in vec2 v_st;\n"
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
    for (int i = 0; i < nprogs; i++)
        if (!memcmp(&progs[i].key, key, sizeof *key))
            return &progs[i];
    if (nprogs == 1024)
        host_fatal("gl: too many shader programs");
    Prog *p = &progs[nprogs++];
    memset(p, 0, sizeof *p);
    p->key = *key;
    sb_len = 0;
    cat("%s", fs_common);
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
        cat("    t0 = texel(s0, v_st, %d);\n", key->filt);
        if (key->tex1)
            cat("    t1 = texel(s1, v_st, %d);\n", key->filt);
    }
    if (key->cyc == 2) {                            /* copy */
        if ((key->om_l & 3) == 1)
            cat("    if (t0.a * 255.0 < 128.0) discard;\n");
        cat("    o_col = t0;\n}\n");
        goto done;
    }
    {
        uint32_t w0 = gs.cc0, w1 = gs.cc1;
        gs.cc0 = key->cc0;
        gs.cc1 = key->cc1;
        uint8_t idx[2][8];
        gfx_cc_decode(idx);
        gs.cc0 = w0;
        gs.cc1 = w1;
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
        if ((l & 0x1000) && !(l & 0x4000))
            cat("    if (comb.a * 255.0 < 128.0) discard;\n");
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
    glUseProgram(p->prog);
    for (int i = 0; i < 8; i++) {
        char name[16];
        snprintf(name, sizeof name, "u_tex[%d]", i);
        glUniform1i(glGetUniformLocation(p->prog, name), i);
    }
#define U(n) p->n = glGetUniformLocation(p->prog, #n)
    U(u_vp); U(u_zbias); U(u_prim); U(u_env); U(u_blend); U(u_fog); U(u_primlod); U(u_seed);
    U(u_tA); U(u_tB); U(u_tul); U(u_levels); U(u_lodscale);
#undef U
    if (host_verbose > 1)
        host_log("gl: program %d: cc %06X %08X om_l %08X cyc %d\n", nprogs, key->cc0, key->cc1, key->om_l, key->cyc);
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
    float x, y, z, w, s, t, r, g, b, a;
} GLVtx;

static GLVtx *vbuf;
static int vcount, vcap;
static GLuint vao, vbo;
static unsigned long long st_draws, st_verts, st_flushes_state;

static void apply_and_draw(void) {
    const DrawState *d = &ds_batch;
    Prog *p = d->prog;
    glBindFramebuffer(GL_FRAMEBUFFER, target_fbo(d->target));
    glViewport(0, 0, tw_px, th_px);
    glEnable(GL_SCISSOR_TEST);
    glScissor(d->sc[0], (TH - d->sc[3]) * scale, d->sc[2] - d->sc[0], (d->sc[3] - d->sc[1]) * scale);
    glColorMask(1, 1, 1, 0);
    if (d->depth) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc((d->depth & 1) ? GL_LEQUAL : GL_ALWAYS);
        glDepthMask((d->depth & 2) ? GL_TRUE : GL_FALSE);
    } else {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }
    if (p->blend == 1) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
    } else if (p->blend == 2) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_ONE);
    } else {
        glDisable(GL_BLEND);
    }
    glUseProgram(p->prog);
    glUniform2f(p->u_vp, TW + 2 * wide_off, TH);
    glUniform1f(p->u_zbias, d->zbias);
    glUniform4fv(p->u_prim, 1, d->prim);
    glUniform4fv(p->u_env, 1, d->env);
    glUniform4fv(p->u_blend, 1, d->blend);
    glUniform4fv(p->u_fog, 1, d->fog);
    glUniform1f(p->u_primlod, d->primlod);
    glUniform1f(p->u_seed, (float)(frame_no % 997));
    glUniform1f(p->u_lodscale, scale);
    glUniform1i(p->u_levels, d->levels);
    if (d->ntex) {
        glUniform4iv(p->u_tA, d->ntex, &d->tA[0][0]);
        glUniform4iv(p->u_tB, d->ntex, &d->tB[0][0]);
        glUniform2fv(p->u_tul, d->ntex, &d->tul[0][0]);
    }
    for (int i = 0; i < d->ntex; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, d->tex[i]);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vcount * sizeof(GLVtx), vbuf, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, vcount);
    if (ipass_gl)
        d->target->idrawn = 1;
    else
        d->target->dirty = 1;
    st_draws++;
    st_verts += vcount;
}

static void flush(void) {
    if (vcount && batch_valid)
        apply_and_draw();
    vcount = 0;
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
        key.textured = kind == K_TEXRECT || (kind == K_TRI && gs.tex_on);
        key.tex1 = key.textured && key.cyc < 2 && gfx_uses_tex1();
        key.lod = key.textured && key.cyc < 2 && gfx_lod_on();
        key.filt = key.textured ? gfx_filter_mode() : 0;
    }
    if (key.cyc == 3 && kind != K_FILL)
        key.cyc = 0;
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
    if (key.textured) {
        int base = kind == K_TEXRECT ? tile : gs.tex_tile;
        int n = key.lod ? gs.tex_levels + 2 : key.tex1 ? 2 : 1;
        if (n > 8) n = 8;
        d.ntex = n;
        for (int i = 0; i < n; i++) {
            int ti = (base + i) & 7;
            const GfxTile *t = &gs.tile[ti];
            d.tex[i] = tile_texture(ti);
            d.tA[i][0] = t->shifts; d.tA[i][1] = t->shiftt; d.tA[i][2] = t->masks; d.tA[i][3] = t->maskt;
            d.tB[i][0] = t->cms; d.tB[i][1] = t->cmt;
            d.tB[i][2] = (t->lrs - t->uls) >> 2; d.tB[i][3] = (t->lrt - t->ult) >> 2;
            d.tul[i][0] = t->uls * 0.25f; d.tul[i][1] = t->ult * 0.25f;
        }
    }
    ds_cur = d;
    return 1;
}

static void begin(int kind, int tile) {
    int changed = build_state(kind, tile);
    if (!batch_valid || (changed && memcmp(&ds_cur, &ds_batch, sizeof ds_cur))) {
        flush();
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
            memcpy(v, p[j], sizeof *v);
            v->x += wide_off;
            if (flat) {
                v->r = flat[0]; v->g = flat[1]; v->b = flat[2]; v->a = flat[3];
            }
        }
    }
}

static void quad(float x0, float y0, float x1, float y1, float s00, float t00, float s10, float t10,
                 float s01, float t01, float s11, float t11, const float *col) {
    GLVtx *v = push(6);
    x0 += wide_off;
    x1 += wide_off;
    GLVtx c[4] = {
        { x0, y0, 0, 1, s00, t00, col[0], col[1], col[2], col[3] },
        { x1, y0, 0, 1, s10, t10, col[0], col[1], col[2], col[3] },
        { x0, y1, 0, 1, s01, t01, col[0], col[1], col[2], col[3] },
        { x1, y1, 0, 1, s11, t11, col[0], col[1], col[2], col[3] },
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
    quad(x0, y0, x1, y1, 0, 0, 0, 0, 0, 0, 0, 0, col);
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
    quad(x0, y0, x1, y1, s[0], t[0], s[1], t[1], s[2], t[2], s[3], t[3], c);
}

void gfx_gl_zclear(int x0, int y0, int x1, int y1) {
    flush();
    Target *t = cur_target ? cur_target : ntargets ? &targets[0] : NULL;
    if (!t)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, target_fbo(t));
    glEnable(GL_SCISSOR_TEST);
    glScissor(px_x(x0), (TH - y1) * scale, (x1 - x0) * scale, (y1 - y0) * scale);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void gfx_gl_clear_rect(int x0, int y0, int x1, int y1) {
    flush();
    if (!cur_target)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, target_fbo(cur_target));
    glEnable(GL_SCISSOR_TEST);
    glScissor(px_x(x0), (TH - y1) * scale, (x1 - x0) * scale, (y1 - y0) * scale);
    glColorMask(1, 1, 1, 1);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
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

void gfx_gl_interp(int on) {
    flush();
    batch_valid = 0;
    raw_valid = 0;
    ipass_gl = on;
}

void gfx_gl_interp_swap(uint32_t fb, int ready) {
    Target *t = find_target(fb);
    if (!t)
        return;
    t->iready = ready && t->idrawn;
    t->idrawn = 0;
}

/* ---- window, presentation --------------------------------------------------------- */

unsigned gfx_gl_window_flags(void) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    /* macOS has a core profile only forward-compatible (4.1, for 3.3) */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    return SDL_WINDOW_OPENGL;
}

int gfx_gl_init(SDL_Window *w) {
    win = w;
    ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        host_log("gl: no OpenGL 3.3 context (%s); using the software renderer\n", SDL_GetError());
        return 0;
    }
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(0);      /* the host loop paces the frames */
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
    glEnable(GL_DEPTH_CLAMP);
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    set_geometry(gfx_gl_scale ? gfx_gl_scale : (dh + TH / 2) / TH, gfx_aspect_of(dw, dh));
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

void gfx_gl_present(uint32_t vi_fb, int vi_width, const char *shot) {
    frame_no++;
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    set_geometry(gfx_gl_scale ? gfx_gl_scale : (dh + TH / 2) / TH, gfx_aspect_of(dw, dh));
    Target *t = vi_fb && vi_width == TW ? find_target(vi_fb) : NULL;
    if (vi_fb && !t) {
        upload_rdram(&view, vi_fb, vi_width > 0 && vi_width <= 640 ? vi_width : TW);
        t = &view;
    }
    /* --interpolate: a frame's first retrace shows its in-between frame */
    GLuint src = t ? t->fbo : 0;
    static uint32_t last_fb;
    int fresh = vi_fb != last_fb;
    last_fb = vi_fb;
    gfx_st_shown[0]++;
    gfx_st_shown[1] += fresh;
    if (t && t != &view && t->iready) {
        t->iready = 0;
        if (fresh && t->ifbo) {
            src = t->ifbo;
            gfx_st_shown[2]++;
        }
    }
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
    if (host_verbose && frame_no % 300 == 0)
        host_log("gl: %llu draws, %llu vertices, %llu textures decoded (%llu hits), %d programs\n", st_draws,
                 st_verts, st_tex_decoded, st_tex_hits, nprogs);
}

#endif
