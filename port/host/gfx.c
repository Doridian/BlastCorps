/*
 * The RSP's graphics tasks: Fast3D (gbi 2.0D) display lists.
 *
 * This file is the front end every renderer shares, and the software
 * renderer.  RSP side: the matrix stack, vertex transform and lighting,
 * fog, clipping, culling, the moveword/movemem state (point ST edits
 * included).  RDP side: TMEM loads (with the odd-row swizzle, as the RDP
 * does it, and RGBA32's split across the two halves), the tiles, texel
 * decoding, level of detail, and the state for the color combiner (both
 * cycles, all inputs but keying), the blender and alpha compare.
 *
 * Each triangle or rectangle then goes to a back end (gfx.h): the OpenGL
 * one (gfx_gl.c) when it is enabled and owns the color image, or the
 * software rasterizer below, which draws into the game's color image in
 * RDRAM as the RDP would, with a float z-buffer tied to the z image and
 * N64 3-point filtering.  The software renderer is the reference and works
 * headless.  What the game sees is the same as on the hardware either way:
 * OS_EVENT_DP only for a list that ends in gDPFullSync.
 *
 * Widescreen (docs/PORT.md, "Widescreen"): both back ends draw the
 * framebuffers gfx_wide_off columns wider on each side; this file decides
 * what covers the sides (full-width scissors and fills, full-width 2D
 * polygons, black beside a picture's edge tiles), and the software
 * renderer then draws those framebuffers on the host (WideFb).
 */
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "host.h"
#include "fiber.h"
#include "gfx.h"
#include "hdtext.h"

/* ---- state ------------------------------------------------------------------- */

typedef GfxTile Tile;
typedef __typeof__(gs.v[0]) Vtx4;

GfxState gs;
uint32_t gfx_state_serial;
uint8_t gfx_tmem[4096];
int gfx_filter;
float gfx_aspect;
int gfx_wide_off;
int gfx_hud_edges = 1;

/* the aspect to render at: gfx_aspect, or the window's, from 4:3 (the
   game's own) up to 32:9 */
float gfx_aspect_of(int w, int h) {
    float a = gfx_aspect == GFX_ASPECT_WINDOW ? (h > 0 ? (float)w / h : 0) : gfx_aspect;
    if (a <= 4.0f / 3 + 1e-3f)
        return 4.0f / 3;
    return a > 32.0f / 9 ? 32.0f / 9 : a;
}

int gfx_wide_off_for(float aspect) {
    int off = (int)lroundf((240 * aspect - 320) / 2);
    return off < 0 ? 0 : off;
}
#ifndef PORT_HAVE_GL
int gfx_gl_enabled;
int gfx_gl_owns_target(void) { return 0; }
void gfx_gl_tri(const GfxVtx *v, int n, const float *flat) { (void)v; (void)n; (void)flat; }
void gfx_gl_fill_rect(int x0, int y0, int x1, int y1) { (void)x0; (void)y0; (void)x1; (void)y1; }
void gfx_gl_tex_rect(int x0, int y0, int x1, int y1, int tile, float s, float t, float dsdx, float dtdy,
                     int flip) {
    (void)x0; (void)y0; (void)x1; (void)y1; (void)tile; (void)s; (void)t; (void)dsdx; (void)dtdy; (void)flip;
}
void gfx_gl_zclear(int x0, int y0, int x1, int y1) { (void)x0; (void)y0; (void)x1; (void)y1; }
void gfx_gl_clear_rect(int x0, int y0, int x1, int y1) { (void)x0; (void)y0; (void)x1; (void)y1; }
void gfx_gl_task_begin(void) {}
void gfx_gl_task_end(void) {}
void gfx_gl_texture_source(uint32_t addr) { (void)addr; }
void gfx_gl_interp(int k) { (void)k; }
unsigned gfx_gl_interp_swap(uint32_t fb) { (void)fb; return 0; }
void gfx_gl_rec_reset(void) {}
int gfx_gl_rec_state(int kind, int tile) { (void)kind; (void)tile; return -1; }
void gfx_gl_rec_force(int id) { (void)id; }
unsigned gfx_gl_gen;
unsigned long long gfx_gl_rec_missed;
int gfx_gl_scale;
int gfx_gl_max_pixels;
unsigned gfx_gl_window_flags(void) { return 0; }
int gfx_gl_init(struct SDL_Window *win) { (void)win; return 0; }
void gfx_gl_present(uint32_t vi_fb, int vi_width, const char *shot, int twin) {
    (void)vi_fb; (void)vi_width; (void)shot; (void)twin;
}
#endif

/* --hud edges (below, "the HUD at the sides"): what the RSP side records for it */
static int hud_task;                    /* this task's HUD goes to the wide frame's sides */
static uint32_t hud_proj;               /* the projection's address (the last G_MTX of one) */
static uint32_t hud_vsrc[16];           /* each vertex's RDRAM address */
static float hud_vx0[16], hud_vx1[16];  /* the screen extent of the load it came in */
static int hud_proj_kind(void);
static void hud_load(int v0, int n);

static float zbuf[640 * 480];
static float *zb = zbuf;        /* the z-buffer drawn with: zbuf, or an in-between pass's */
static int zb_w = 320, zb_off;  /* the z-buffer's row length, and where x 0 is in it */

/* ---- widescreen, software renderer: the framebuffers drawn wide, on the host ---- */

typedef struct {
    uint32_t addr;
    int w;                      /* 320 + 2 * gfx_wide_off */
    uint16_t *px;               /* RGBA5551, host order */
} WideFb;

static WideFb wfb[4];
static int nwfb;
static WideFb *cur_wfb;         /* the color image's, when it is one */
static int wide_sides_drawn;    /* the color image's sides drawn since it was set (a wide fill or 2D) */

/* a 320-wide 16-bit color image: a framebuffer, or the z image */
static int wide_cimg(void) { return gfx_wide_off && gs.cimg_siz == 2 && gs.cimg_w == 320; }

static WideFb *get_wfb(uint32_t addr) {
    for (int i = 0; i < nwfb; i++)
        if (wfb[i].addr == addr)
            return &wfb[i];
    if (nwfb == 4) {                                /* recycle the oldest */
        free(wfb[0].px);
        memmove(wfb, wfb + 1, 3 * sizeof wfb[0]);
        nwfb--;
    }
    WideFb *f = &wfb[nwfb++];
    f->addr = addr;
    f->w = 320 + 2 * gfx_wide_off;
    f->px = calloc((size_t)f->w * 240, 2);
    const uint8_t *src = port_ptr(addr);             /* what was there, in the middle */
    for (int y = 0; y < 240; y++)
        for (int x = 0; x < 320; x++)
            f->px[y * f->w + gfx_wide_off + x] = port_be16(src + 2 * (y * 320 + x));
    return f;
}

/* --interpolate, software renderer: the in-between passes draw each
   framebuffer's twins, on the host (as wide as the WideFb, 320 at 4:3),
   each with a z-buffer of its own; RDRAM stays the first pass's */
typedef struct {
    uint32_t addr;
    uint16_t *px[GFX_TWINS];
    unsigned drawn;             /* the twins drawn since the frame in it was shown */
} SwTwin;
static SwTwin twins[4];
static int ntwins;
static float *twin_z[GFX_TWINS];
static WideFb twin_fb;          /* the one drawn into, as a WideFb */

static int ipass, ik;           /* in an in-between pass (--interpolate, below), into twin ik */

static void free_twins(void) {
    for (int i = 0; i < ntwins; i++)
        for (int k = 0; k < GFX_TWINS; k++)
            free(twins[i].px[k]);
    memset(twins, 0, sizeof twins);
    ntwins = 0;
}

static SwTwin *find_twin(uint32_t addr) {
    for (int i = 0; i < ntwins; i++)
        if (twins[i].addr == addr)
            return &twins[i];
    return NULL;
}

static WideFb *get_twin(uint32_t addr, int k) {
    SwTwin *t = find_twin(addr);
    if (!t) {
        if (ntwins == 4) {                          /* recycle the oldest */
            for (int j = 0; j < GFX_TWINS; j++)
                free(twins[0].px[j]);
            memmove(twins, twins + 1, 3 * sizeof twins[0]);
            ntwins--;
        }
        t = &twins[ntwins++];
        memset(t, 0, sizeof *t);
        t->addr = addr;
    }
    int w = 320 + 2 * gfx_wide_off;
    if (!t->px[k]) {                                /* what the frame has */
        t->px[k] = calloc((size_t)w * 240, 2);
        const WideFb *f = NULL;
        for (int i = 0; i < nwfb; i++)
            if (wfb[i].addr == addr)
                f = &wfb[i];
        if (f)
            memcpy(t->px[k], f->px, (size_t)w * 240 * 2);
        else {
            const uint8_t *src = port_ptr(addr);
            for (int y = 0; y < 240; y++)
                for (int x = 0; x < 320; x++)
                    t->px[k][y * w + gfx_wide_off + x] = port_be16(src + 2 * (y * 320 + x));
        }
    }
    t->drawn |= 1u << k;
    twin_fb.addr = addr;
    twin_fb.w = w;
    twin_fb.px = t->px[k];
    return &twin_fb;
}

const uint16_t *gfx_sw_twin_frame(uint32_t fb, int k, int *w) {
    SwTwin *t = find_twin(fb);
    if (!t || k < 0 || k >= GFX_TWINS || !t->px[k])
        return NULL;
    *w = 320 + 2 * gfx_wide_off;
    return t->px[k];
}

/* after the color or z image changes */
static void sw_target(void) {
    if (ipass && !gfx_gl_enabled) {                 /* a twin, or nothing */
        if (!twin_z[ik])
            twin_z[ik] = calloc(640 * 480, sizeof(float));
        zb = twin_z[ik];
        zb_w = 320 + 2 * gfx_wide_off;
        zb_off = gfx_wide_off;
        cur_wfb = gs.cimg_siz == 2 && gs.cimg_w == 320 && gs.cimg_addr != gs.zimg_addr ? get_twin(gs.cimg_addr, ik)
                                                                                         : NULL;
        return;
    }
    int wide = wide_cimg();
    zb = zbuf;
    zb_w = wide ? 320 + 2 * gfx_wide_off : gs.cimg_w;
    zb_off = wide ? gfx_wide_off : 0;
    cur_wfb = wide && !gfx_gl_enabled && gs.cimg_addr != gs.zimg_addr ? get_wfb(gs.cimg_addr) : NULL;
}

void gfx_set_wide(int off) {
    if (off == gfx_wide_off)
        return;
    gfx_wide_off = off;
    for (int i = 0; i < nwfb; i++)
        free(wfb[i].px);
    nwfb = 0;
    free_twins();
    cur_wfb = NULL;
    sw_target();
}

/* a texture load from a wide framebuffer (none happens): its middle into
   RDRAM first, which the software renderer otherwise leaves alone there,
   as the GPU does */
static void wfb_texture_source(uint32_t addr) {
    for (int i = 0; i < nwfb; i++)
        if (addr >= wfb[i].addr && addr < wfb[i].addr + 320 * 240 * 2) {
            uint8_t *dst = port_ptr(wfb[i].addr);
            for (int y = 0; y < 240; y++)
                for (int x = 0; x < 320; x++)
                    port_wbe16(dst + 2 * (y * 320 + x), wfb[i].px[y * wfb[i].w + gfx_wide_off + x]);
            if (host_verbose)
                host_log("gfx: texture from wide framebuffer %08X: copied back\n", wfb[i].addr);
        }
}

const uint16_t *gfx_sw_wide_frame(uint32_t addr, int *w) {
    for (int i = 0; i < nwfb; i++)
        if (wfb[i].addr == addr) {
            *w = wfb[i].w;
            return wfb[i].px;
        }
    return NULL;
}
int host_gfx_stats[256];
static unsigned long long st_tris, st_raster, st_tested, st_drawn;
static double st_cover;         /* pixels covered, as the geometry says: the RDP's work */
static int charge_tris;         /* the triangles' cover is wanted (the RDP's time counts, or -v) */

/* fminf and fmaxf as musl has them (NaN loses, -0 is below +0), inline:
   emscripten's are calls, a dozen a triangle */
static inline float min_f(float x, float y) {
    if (x != x)
        return y;
    if (y != y)
        return x;
    if (signbit(x) != signbit(y))
        return signbit(x) ? x : y;
    return x < y ? x : y;
}

static inline float max_f(float x, float y) {
    if (x != x)
        return y;
    if (y != y)
        return x;
    if (signbit(x) != signbit(y))
        return signbit(x) ? y : x;
    return x < y ? y : x;
}

/* (the display list's commands that do real work are functions of their
   own, so that run() stays a small loop: one huge function with all of
   them inlined compiled to slow code, in Firefox above all.  Binaryen
   inlines a function called from one place whatever clang was told, so
   in WebAssembly run() calls them through pointers, CALL) */
#define NOINLINE __attribute__((noinline))
#ifdef __EMSCRIPTEN__
#define CALL(f) (*(__typeof__(&f) volatile *)&call_##f)
#define CALL_VIA(f) static __typeof__(&f) call_##f = f;
#else
#define CALL(f) f
#define CALL_VIA(f)
#endif

/* A segmented address is a segment (bits 24..27) and a 24-bit offset, and
   the RSP ignores the top nibble.  But the game's C keeps its locals on the
   fibers' host stacks at 0x90000000 (port.h), whose physical addresses
   (K0_TO_PHYS, osVirtualToPhysical) are 0x10xxxxxx: bit 28 is the only
   thing telling them from RDRAM, so it stays, here and in a segment's base.
   (func_8024B8F4's visibility test loads its box from its stack; without
   the bit its vertices were read from RDRAM at 0x800EFCC0, whatever the
   build had there, and drawn.)  Any other top nibble (a KSEG0 or KSEG1
   address used as a physical one) masks away as before.  The movable
   builds' stacks are in the arena, at physical 0x00C00000 and up, which
   fits in 24 bits. */
static uint32_t seg_to_k0(uint32_t a) {
    uint32_t seg = (a >> 24) & 0x0F;
    return 0x80000000u | ((gs.seg[seg] + (a & 0x00FFFFFFu) + (a & 0xF0000000u)) & 0x1FFFFFFFu);
}

/* PORT_SIMD: four-lane vectors (the compiler's vector extensions: SSE on
   x86, NEON on AArch64, WebAssembly's SIMD128 with -msimd128, scalar code
   anywhere else).  Each lane's arithmetic is the scalar code's, in its
   order, and the build has no FMA contraction (-ffp-contract=off), so the
   results are the same bits.  Not with the access profiler, whose byte
   reads they would skip. */
#if defined(PORT_SIMD) && !defined(PORT_ACCESS_PROFILE)
#define GFX_SIMD 1
typedef float f4 __attribute__((vector_size(16)));
typedef int32_t i4 __attribute__((vector_size(16)));
typedef int16_t i16x8 __attribute__((vector_size(16)));
typedef uint8_t u8x16 __attribute__((vector_size(16)));
typedef int8_t s8x16 __attribute__((vector_size(16)));
static inline f4 ld4(const float *p) {
    f4 v;
    memcpy(&v, p, sizeof v);
    return v;
}
#else
#define GFX_SIMD 0
#endif

NOINLINE static void mtx_mul(float r[4][4], float a[4][4], float b[4][4]) {
#if GFX_SIMD
    /* row i: b's rows by a[i]'s elements, summed in the scalar order */
    f4 b0 = ld4(b[0]), b1 = ld4(b[1]), b2 = ld4(b[2]), b3 = ld4(b[3]), t[4];
    for (int i = 0; i < 4; i++)
        t[i] = a[i][0] * b0 + a[i][1] * b1 + a[i][2] * b2 + a[i][3] * b3;
    memcpy(r, t, sizeof t);
#else
    float t[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            t[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
    memcpy(r, t, sizeof t);
#endif
}

static void load_mtx(float m[4][4], uint32_t addr) {
    const uint8_t *p = port_ptr(addr);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            int k = i * 4 + j;
            int16_t hi = (int16_t)port_g16_of32(p, k);             /* Mtx: words */
            uint16_t lo = port_g16_of32(p + 32, k);
            m[i][j] = (float)(((int32_t)hi << 16) | lo) / 65536.0f;
        }
}

/* ---- replaying the first pass (--interpolate, OpenGL) ---------------------------
 *
 * An in-between pass draws what the first drew, with only the vertices
 * moved (blended with the previous frame's: below).  So the first pass
 * records, as it runs, what the in-between passes' drawing depends on: its
 * vertex loads as the transform and lighting left them (before blending),
 * the point edits (G_MW_POINTS), the triangles and rectangles, the 2D
 * projection's changes (the HUD's), and, when a display-list command may
 * have changed it, the state the drawing reads (RCtx) and the OpenGL draw
 * state the triangles got (gfx_gl_rec_state).  An in-between pass then
 * replays that record (replay()): each load's vertices into the vertex
 * buffer, blended as the full pass would (iblend, from the same records of
 * the previous frame, in the same order), and each triangle or rectangle
 * through the same functions as the full pass, drawn with the recorded
 * draw state.  No display list, transform, lighting, TMEM load or texture
 * lookup is run again, and the pictures are the full pass's.  The full
 * pass runs instead where the record isn't valid: the software renderer
 * (which draws the twins pixel by pixel, the cost being there), a draw
 * state that may have gone (a texture or target dropped while the first
 * pass ran: gfx_gl_gen), PORT_INTERP_REPLAY=0. */
enum { R_CTX, R_LOAD, R_MODV, R_TRI, R_FILL, R_TEXRECT, R_PROJ, R_TLOAD };
typedef struct {
    uint32_t geom, om_h, om_l, cc0, cc1, fill;
    uint8_t prim[4], env[4];
    float vp_scale[3], vp_trans[3];
    int sc[4];
    int tex_on, tex_tile, masks, cms;   /* (wide_2d reads the texture's tile's wrap) */
    uint32_t timg_addr, cimg_addr, zimg_addr, seg2;
    int cimg_siz, cimg_w;
    int tri_state;                      /* the triangles' draw state (gfx_gl_rec_state), -1: none */
    int snap;                           /* rsnap's, for one made in a replay (-1: none) */
} RCtx;
static int recording;                   /* this first pass is recorded */
static uint32_t *rec;                   /* the events, in order */
static int rec_n, rec_cap;
static RCtx *rctx;
static int rctx_n, rctx_cap;
static uint32_t rec_serial;             /* gfx_state_serial when rctx[rctx_n - 1] was taken */
static Vtx4 *rv;                        /* the loads' vertices */
static int rv_n, rv_cap;
static float (*rmvp)[4][4];             /* the MVP at each load (HUD tasks: hud_text reads it) */
static int rmvp_n, rmvp_cap;
static unsigned rec_gen;
/* what a draw state is made from (gs from geom on), where the first pass
   drew nothing to take the state from: a triangle culled there may not be
   in an in-between pass, which then makes its state as the full pass would
   (from these, and TMEM brought up to date by the loads, R_TLOAD) */
#define SNAP_OFF offsetof(GfxState, geom)
#define SNAP_SZ (sizeof(GfxState) - SNAP_OFF)
static uint8_t *rsnap;
static int rsnap_n, rsnap_cap;
static unsigned long long st_replay[2];  /* in-between passes replayed, run in full */
static int gl_target(void);

#define GROW(p, n, cap, add, first)                                                 \
    do {                                                                            \
        if ((n) + (add) > (cap)) {                                                  \
            (cap) = (cap) ? (cap) * 2 : (first);                                    \
            if ((cap) < (n) + (add))                                                \
                (cap) = (n) + (add);                                                \
            (p) = realloc((p), (size_t)(cap) * sizeof *(p));                        \
        }                                                                           \
    } while (0)

static void rec_reset(void) {
    rec_n = rctx_n = rv_n = rmvp_n = rsnap_n = 0;
    rec_gen = gfx_gl_gen;
    gfx_gl_rec_reset();
}

/* the state an event's drawing reads, when a command may have changed it */
NOINLINE static void rec_ctx(void) {
    rec_serial = gfx_state_serial;
    GROW(rctx, rctx_n, rctx_cap, 1, 256);
    RCtx *c = &rctx[rctx_n++];
    c->geom = gs.geom; c->om_h = gs.om_h; c->om_l = gs.om_l; c->cc0 = gs.cc0; c->cc1 = gs.cc1; c->fill = gs.fill;
    memcpy(c->prim, gs.prim, 4);
    memcpy(c->env, gs.env, 4);
    memcpy(c->vp_scale, gs.vp_scale, sizeof c->vp_scale);
    memcpy(c->vp_trans, gs.vp_trans, sizeof c->vp_trans);
    c->sc[0] = gs.sc_x0; c->sc[1] = gs.sc_y0; c->sc[2] = gs.sc_x1; c->sc[3] = gs.sc_y1;
    c->tex_on = gs.tex_on; c->tex_tile = gs.tex_tile;
    c->masks = gs.tile[gs.tex_tile & 7].masks; c->cms = gs.tile[gs.tex_tile & 7].cms;
    c->timg_addr = gs.timg_addr; c->cimg_addr = gs.cimg_addr; c->zimg_addr = gs.zimg_addr; c->seg2 = gs.seg[2];
    c->cimg_siz = gs.cimg_siz; c->cimg_w = gs.cimg_w;
    c->tri_state = c->snap = -1;
    GROW(rec, rec_n, rec_cap, 2, 65536);
    rec[rec_n++] = R_CTX;
    rec[rec_n++] = (uint32_t)(rctx_n - 1);
}

static int rec_snap(void) {
    if ((rsnap_n + 1) * SNAP_SZ > (size_t)rsnap_cap) {
        rsnap_cap = rsnap_cap ? rsnap_cap * 2 : 64 * (int)SNAP_SZ;
        rsnap = realloc(rsnap, (size_t)rsnap_cap);
    }
    memcpy(rsnap + rsnap_n * SNAP_SZ, (const uint8_t *)&gs + SNAP_OFF, SNAP_SZ);
    return rsnap_n++;
}

static inline uint32_t *rec_raw(int op, int n) {
    GROW(rec, rec_n, rec_cap, n, 65536);
    uint32_t *e = rec + rec_n;
    rec_n += n;
    e[0] = (uint32_t)op;
    return e;
}

static inline uint32_t *rec_ev(int op, int n) {
    if (rec_serial != gfx_state_serial || !rctx_n)
        rec_ctx();
    GROW(rec, rec_n, rec_cap, n, 65536);
    uint32_t *e = rec + rec_n;
    rec_n += n;
    e[0] = (uint32_t)op;
    return e;
}

static void rec_load(uint32_t a, int v0, int m) {
    uint32_t *e = rec_ev(R_LOAD, 5);
    GROW(rv, rv_n, rv_cap, m, 16384);
    memcpy(rv + rv_n, gs.v + v0, (size_t)m * sizeof *rv);
    e[1] = a;
    e[2] = (uint32_t)(v0 | m << 8);
    e[3] = (uint32_t)rv_n;
    e[4] = 0xFFFFFFFFu;
    rv_n += m;
    if (hud_task) {
        GROW(rmvp, rmvp_n, rmvp_cap, 1, 1024);
        memcpy(rmvp[rmvp_n], gs.mvp, sizeof gs.mvp);
        e[4] = (uint32_t)rmvp_n++;
    }
}

static inline void rec_tri(int i0, int i1, int i2, int flag) {
    uint32_t *e = rec_ev(R_TRI, 2);
    e[1] = (uint32_t)((i0 & 0xFF) | (i1 & 0xFF) << 8 | (i2 & 0xFF) << 16 | (uint32_t)(flag & 0xFF) << 24);
}

/* after it: culled, with no state for its kind yet */
static inline void rec_tri_done(void) {
    RCtx *c = &rctx[rctx_n - 1];
    if (c->tri_state < 0 && c->snap < 0 && gl_target())
        c->snap = rec_snap();
}

/* the first pass drew the last triangle (kind GFX_GL_TRI) or rectangle: the
   draw state it got */
static int rec_rect_at = -1;            /* the last rectangle's event */
static void rec_drawn(int kind, int tile) {
    if (kind == GFX_GL_TRI) {
        RCtx *c = &rctx[rctx_n - 1];
        if (c->tri_state < 0)
            c->tri_state = gfx_gl_rec_state(kind, tile);
    } else if (rec_rect_at >= 0) {
        rec[rec_rect_at + 6] = (uint32_t)gfx_gl_rec_state(kind, tile);
        rec_rect_at = -1;
    }
}

static void rec_rect(int op, uint32_t w0, uint32_t w1, uint32_t h2, uint32_t hc, int flip) {
    uint32_t *e = rec_ev(op, 8);
    e[1] = w0; e[2] = w1; e[3] = h2; e[4] = hc; e[5] = (uint32_t)flip;
    e[6] = 0xFFFFFFFFu;
    e[7] = gl_target() ? (uint32_t)rec_snap() : 0xFFFFFFFFu;
    rec_rect_at = (int)(e - rec);
}

/* ---- the RSP ----------------------------------------------------------------- */

NOINLINE static void do_mtx(uint32_t w0, uint32_t w1) {
    int p = (w0 >> 16) & 0xFF;
    float m[4][4];
    load_mtx(m, seg_to_k0(w1));
    if (p & 1) {                        /* projection */
        hud_proj = seg_to_k0(w1);
        if (recording && hud_task)
            rec_ev(R_PROJ, 2)[1] = hud_proj;
        if (p & 2)
            memcpy(gs.proj, m, sizeof m);
        else
            mtx_mul(gs.proj, m, gs.proj);
    } else {
        if ((p & 4) && gs.mv_top < 9) {
            memcpy(gs.mv[gs.mv_top + 1], gs.mv[gs.mv_top], sizeof m);
            gs.mv_top++;
        }
        if (p & 2)
            memcpy(gs.mv[gs.mv_top], m, sizeof m);
        else
            mtx_mul(gs.mv[gs.mv_top], m, gs.mv[gs.mv_top]);
    }
    gs.mvp_dirty = 1;
}

/* ---- in-between frames (--interpolate; docs/PORT.md, "Frame rate") -------------------------
 *
 * Where the game holds each frame for two retraces or more (gameplay),
 * every graphics task runs again for each in-between image, into the
 * framebuffers' twins (the GPU's, or the software renderer's on the host),
 * with each vertex's clip-space position between where the previous frame
 * put the same vertex and where this one does, at t = (k + 1) / (K + 1)
 * for twin k of K.  That is the matrices interpolated (a vertex's clip
 * position is linear in its MVP) for model data that stays put, and it
 * also follows vertices the CPU writes each frame.  The retraces a frame
 * is on screen then show its twins in turn, and the frame itself last.  A
 * vertex is "the same" by the address it was loaded from (inside the
 * double-buffered per-frame buffer, the offset from its start, segment 2)
 * and how many loads from that address came before it in the frame.  The
 * in-between passes read the display list as the first left RDRAM, draw
 * nothing but the twins, and leave the RSP/RDP state as the first pass
 * left it: the game sees nothing.
 *
 * K is chosen as the frame's first task runs, from how long the last
 * frames were held (the one being drawn will be held as long, most
 * likely): D retraces, D * gfx_interp_hz / 60 images.
 */
int gfx_interp;                 /* --interpolate */
int gfx_interp_hz = PORT_RETRACE_HZ;   /* --display-hz */
static int itrack;              /* this task's vertex loads are recorded */
static int iframe_partial;      /* a task of this frame had no in-between pass */
static float interp_t = 0.5f;
static int frame_k = -1, frame_d;   /* the frame being drawn: its twins (-1: not yet chosen), and hold */
static int hold_last[2] = { 2, 2 }; /* the last two frames' holds, in retraces */
static uint32_t shown_fb;           /* the frame the VI shows, */
static int shown_k, shown_d;        /* its twins that are ready (0: none) and the hold they were made for, */
static unsigned long long shown_at, n_presents; /* and the present before it showed */

#define IHASH (1 << 16)
typedef struct {
    uint32_t addr, occ;
    int n;
    float p[16][4];
} ILoad;
typedef struct {
    ILoad *l;
    int n, cap;
    int32_t hash[IHASH];                    /* index + 1 */
    struct { uint32_t addr, cnt; } occ[IHASH];
    uint32_t *used;                         /* the occ slots taken (hash's: one a load, found from l) */
    int nused, capused;
} IFrame;
static IFrame *ifr[2];
static int icur;
static int *tkeys, tk_n, tk_cap, tk_i;      /* this task's loads, in order */
/* vertex loads, loads blended, vertices, vertices blended, vertices of loads
   that moved too far, in-between passes, frames with in-between images, loads
   with no match, frames by their twins (1..GFX_TWINS), frames that were held
   longer than their twins were made for, and shorter */
unsigned long long gfx_st_interp[8 + GFX_TWINS + 2];
unsigned long long gfx_st_shown[3];         /* gfx.h */
unsigned long long gfx_st_images;           /* gfx.h */

static inline uint32_t ihash(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    return (h ^ (h >> 15)) & (IHASH - 1);
}

/* (the slots it took: a frame takes a few thousand of the tables' 64K) */
static void iframe_reset(IFrame *f) {
    for (int i = 0; i < f->n; i++) {
        const ILoad *l = &f->l[i];
        uint32_t h = ihash(l->addr, l->occ);
        while (f->hash[h] != i + 1)
            h = (h + 1) & (IHASH - 1);
        f->hash[h] = 0;
    }
    for (int i = 0; i < f->nused; i++)
        f->occ[f->used[i]].addr = f->occ[f->used[i]].cnt = 0;
    f->n = f->nused = 0;
}

static ILoad *ilookup(IFrame *f, uint32_t addr, uint32_t occ) {
    for (uint32_t h = ihash(addr, occ);; h = (h + 1) & (IHASH - 1)) {
        int32_t k = f->hash[h];
        if (!k)
            return NULL;
        ILoad *l = &f->l[k - 1];
        if (l->addr == addr && l->occ == occ)
            return l;
    }
}

/* the address a vertex load is known by: inside the frame buffer the game
   is building (segment 2), the offset into it, so that the two buffers'
   loads pair up */
static uint32_t ikey(uint32_t w1) {
    uint32_t a = seg_to_k0(w1);
    if (gs.seg[2]) {
        uint32_t fb = seg_to_k0(0x02000000u);
        if (a >= fb && a < fb + 0x21498u)
            return 0x02000000u | (a - fb);
    }
    return a;
}

static void tk_push(int k) {
    if (tk_n == tk_cap) {
        tk_cap = tk_cap ? tk_cap * 2 : 4096;
        tkeys = realloc(tkeys, (size_t)tk_cap * sizeof *tkeys);
    }
    tkeys[tk_n++] = k;
}

/* the first pass: remember where the n points of the thing known as addr
   went (a vertex load's vertices in clip space, a rectangle's corners) */
static void irecord_at(uint32_t addr, const float (*pt)[4], int n) {
    IFrame *f = ifr[icur];
    uint32_t h = ihash(addr, 0xFFFFFFFFu);
    while (f->occ[h].cnt && f->occ[h].addr != addr)
        h = (h + 1) & (IHASH - 1);
    uint32_t occ = f->occ[h].cnt;
    if (f->n >= IHASH / 2) {                /* full: not interpolated */
        tk_push(-1);
        return;
    }
    if (!occ) {
        if (f->nused == f->capused) {
            f->capused = f->capused ? f->capused * 2 : 4096;
            f->used = realloc(f->used, (size_t)f->capused * sizeof *f->used);
        }
        f->used[f->nused++] = h;
    }
    f->occ[h].addr = addr;
    f->occ[h].cnt = occ + 1;
    if (f->n == f->cap) {
        f->cap = f->cap ? f->cap * 2 : 4096;
        f->l = realloc(f->l, (size_t)f->cap * sizeof *f->l);
    }
    ILoad *l = &f->l[f->n];
    l->addr = addr;
    l->occ = occ;
    l->n = n;
    memcpy(l->p, pt, (size_t)n * sizeof pt[0]);
    uint32_t hh = ihash(addr, occ);
    while (f->hash[hh])
        hh = (hh + 1) & (IHASH - 1);
    f->hash[hh] = ++f->n;
    tk_push(f->n - 1);
}

static void irecord(uint32_t w1, int v0, int n) {
    float pt[16][4];
    for (int i = 0; i < n; i++) {
        pt[i][0] = gs.v[v0 + i].x;
        pt[i][1] = gs.v[v0 + i].y;
        pt[i][2] = gs.v[v0 + i].z;
        pt[i][3] = gs.v[v0 + i].w;
    }
    irecord_at(ikey(w1), pt, n);
}

/* an in-between pass: the previous frame's record of the thing the first
   pass recorded next (NULL: none, or none in the previous frame) */
static const ILoad *iprev(int n) {
    if (tk_i >= tk_n)
        return NULL;
    int k = tkeys[tk_i++];
    if (k < 0)
        return NULL;
    const ILoad *c = &ifr[icur]->l[k];
    const ILoad *p = ilookup(ifr[icur ^ 1], c->addr, c->occ);
    return p && p->n >= n ? p : NULL;
}

/* the second pass: the same load, between the previous frame's and this one's */
static void iblend(int v0, int n) {
    if (tk_i >= tk_n)
        return;
    gfx_st_interp[0]++;
    gfx_st_interp[2] += n;
    const ILoad *p = iprev(n);
    if (!p) {
        gfx_st_interp[7]++;
        return;
    }
    /* a load whose vertices moved by more than a quarter of their distance
       from the eye (in clip space, where that is |(x, y, z, w)|) is taken
       for other vertices than the previous frame's (or a camera cut): all
       of it is drawn where this frame has it, so that its triangles hold
       together */
    for (int i = 0; i < n; i++) {
        const Vtx4 *v = &gs.v[v0 + i];
        const float *q = p->p[i];
        float dx = v->x - q[0], dy = v->y - q[1], dz = v->z - q[2], dw = v->w - q[3];
        float nc = v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w;
        float np = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
        if (dx * dx + dy * dy + dz * dz + dw * dw > 0.0625f * (nc > np ? nc : np)) {
            gfx_st_interp[4] += n;
            return;
        }
    }
    gfx_st_interp[1]++;
    for (int i = 0; i < n; i++) {
        Vtx4 *v = &gs.v[v0 + i];
        const float *q = p->p[i];
        v->x = q[0] + (v->x - q[0]) * interp_t;
        v->y = q[1] + (v->y - q[1]) * interp_t;
        v->z = q[2] + (v->z - q[2]) * interp_t;
        v->w = q[3] + (v->w - q[3]) * interp_t;
    }
    gfx_st_interp[3] += n;
}

/* after a vertex load (w1, at a) put m vertices at v0: --interpolate's
   records and blends, the HUD's extents */
static void vtx_tail(uint32_t w1, uint32_t a, int v0, int m) {
    if (itrack) {
        if (ipass)
            iblend(v0, m);
        else
            irecord(w1, v0, m);
    }
    if (hud_task) {
        for (int i = 0; i < m; i++)
            hud_vsrc[v0 + i] = a + 16 * i;
        hud_load(v0, m);
    }
    /* what tri divides by w for, once a vertex rather than once each
       triangle that uses it (the same quotients) */
    for (int i = v0; i < v0 + m; i++) {
        const Vtx4 *v = &gs.v[i];
#if GFX_SIMD
        f4 q = (f4){ v->x, v->y, 1.0f, 1.0f } / (f4){ v->w, v->w, v->w, 1.0f };
        gs.vd[i][0] = q[0];
        gs.vd[i][1] = q[1];
        gs.vd[i][2] = q[2];
#else
        gs.vd[i][0] = v->x / v->w;
        gs.vd[i][1] = v->y / v->w;
        gs.vd[i][2] = 1.0f / v->w;
#endif
    }
}

#if GFX_SIMD
/* do_vtx's vertices with four lanes (PORT_SIMD): a vertex's 16 bytes in one
   load, its position as the matrix's rows summed, the lights' dot products
   four lights at a time, its color as one vector; what do_vtx's scalar
   loop does, to the bit */
NOINLINE static void vtx_simd(const uint8_t *p, int v0, int n, float ldir[8][3], float ladir[2][3]) {
    f4 m0 = ld4(gs.mvp[0]), m1 = ld4(gs.mvp[1]), m2 = ld4(gs.mvp[2]), m3 = ld4(gs.mvp[3]);
    int lit = (gs.geom & 0x20000) != 0, tgen = lit && (gs.geom & 0x40000), fog = (gs.geom & 0x10000) != 0;
    int nl = gs.nlights, ng = 0;
    f4 dx[3], dy[3], dz[3], lc[8];
    if (lit) {
        /* the directions by component, a lane each: the lights, then the
           look-at vectors for G_TEXTURE_GEN */
        float t[3][12];
        memset(t, 0, sizeof t);
        for (int j = 0; j < 3; j++) {
            for (int l = 0; l < nl; l++)
                t[j][l] = ldir[l][j];
            if (tgen) {
                t[j][nl] = ladir[0][j];
                t[j][nl + 1] = ladir[1][j];
            }
        }
        ng = (nl + (tgen ? 2 : 0) + 3) / 4;
        for (int g = 0; g < ng; g++) {
            dx[g] = ld4(&t[0][4 * g]);
            dy[g] = ld4(&t[1][4 * g]);
            dz[g] = ld4(&t[2][4 * g]);
        }
        for (int l = 0; l <= nl; l++)
            lc[l] = (f4){ gs.lcol[l][0], gs.lcol[l][1], gs.lcol[l][2], 0 };
    }
    const f4 k255 = { 255, 255, 255, 255 };
    for (int i = 0; i < n && v0 + i < 16; i++, p += 16) {
        Vtx4 *v = &gs.v[v0 + i];
        u8x16 b;
        memcpy(&b, p, 16);
#ifndef PORT_NATIVE_ENDIAN
        /* the halfwords from big-endian (the bytes, 12-15, as they are) */
        b = __builtin_shufflevector(b, b, 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 12, 13, 14, 15);
#endif
        i16x8 h = (i16x8)b;
        f4 ob = __builtin_convertvector(__builtin_shufflevector(h, h, 0, 1, 2, 4), f4);   /* x, y, z, s */
        f4 pos = __builtin_shufflevector(ob, ob, 0, 0, 0, 0) * m0 + __builtin_shufflevector(ob, ob, 1, 1, 1, 1) * m1 +
                 __builtin_shufflevector(ob, ob, 2, 2, 2, 2) * m2 + m3;
        float s = ob[3], t = (int16_t)h[5];
        f4 col;
        if (lit) {
            s8x16 sb = (s8x16)b;
            f4 nv = __builtin_convertvector(__builtin_shufflevector(sb, sb, 12, 13, 14, 15), f4);
            float len = sqrtf(nv[0] * nv[0] + nv[1] * nv[1] + nv[2] * nv[2]);
            if (len > 0)
                nv = nv / len;
            f4 nx = __builtin_shufflevector(nv, nv, 0, 0, 0, 0), ny = __builtin_shufflevector(nv, nv, 1, 1, 1, 1),
               nz = __builtin_shufflevector(nv, nv, 2, 2, 2, 2);
            float d[12];
            for (int g = 0; g < ng; g++) {
                f4 dg = nx * dx[g] + ny * dy[g] + nz * dz[g];
                memcpy(&d[4 * g], &dg, sizeof dg);
            }
            /* a light facing away adds +0 (the colors are finite and
               never -0), which leaves c as the scalar code's skip does */
            f4 c = lc[nl];
            for (int l = 0; l < nl; l++)
                c = c + (d[l] > 0 ? d[l] : 0.0f) * lc[l];
            i4 over = c > k255;
            col = (f4)(((i4)c & ~over) | ((i4)k255 & over));
            col[3] = p[15];
            if (tgen) {
                s = (d[nl] * 0.5f + 0.5f) * 32 * 32 * 32;
                t = (d[nl + 1] * 0.5f + 0.5f) * 32 * 32 * 32;
            }
        } else {
            col = __builtin_convertvector(__builtin_shufflevector(b, b, 12, 13, 14, 15), f4);
        }
        if (fog && pos[3] > 0) {
            float f = pos[2] / pos[3] * gs.fog_mul + gs.fog_off;
            col[3] = f < 0 ? 0 : f > 255 ? 255 : f;
        }
        /* (stored a float at a time: a 16-byte store of the color made the
           loop three times slower in V8, in the game though not in a
           benchmark of it) */
        v->x = pos[0]; v->y = pos[1]; v->z = pos[2]; v->w = pos[3];
        v->s = s * gs.tex_s / 32.0f;
        v->t = t * gs.tex_t / 32.0f;
        v->r = col[0]; v->g = col[1]; v->b = col[2]; v->a = col[3];
    }
}
#endif

NOINLINE static void do_vtx(uint32_t w0, uint32_t w1) {
    int n = ((w0 >> 20) & 0xF) + 1, v0 = (w0 >> 16) & 0xF;
    const uint8_t *p = port_ptr(seg_to_k0(w1));
    if (gs.mvp_dirty) {
        mtx_mul(gs.mvp, gs.mv[gs.mv_top], gs.proj);
        gs.mvp_dirty = 0;
    }
    float (*mv)[4] = gs.mv[gs.mv_top];
    /* G_LIGHTING: the lights, and for G_TEXTURE_GEN the look-at vectors
       (gSPLookAt: the camera's x and y axes), into model space, as the RSP
       takes them: by the modelview (the game keeps its view in the
       projection), normalized */
    float ldir[8][3], ladir[2][3];
    if (gs.geom & 0x20000) {
        for (int l = 0; l < gs.nlights + 2; l++) {
            const float *src = l < gs.nlights ? gs.ldir[l] : gs.lookat[l - gs.nlights];
            float *d = l < gs.nlights ? ldir[l] : ladir[l - gs.nlights];
            for (int j = 0; j < 3; j++)
                d[j] = mv[j][0] * src[0] + mv[j][1] * src[1] + mv[j][2] * src[2];
            float len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (len > 0)
                for (int j = 0; j < 3; j++)
                    d[j] /= len;
        }
    }
#if GFX_SIMD
    vtx_simd(p, v0, n, ldir, ladir);
#else
    for (int i = 0; i < n && v0 + i < 16; i++, p += 16) {
        Vtx4 *v = &gs.v[v0 + i];
        float x = (int16_t)port_g16(p), y = (int16_t)port_g16(p + 2), z = (int16_t)port_g16(p + 4);
        v->x = x * gs.mvp[0][0] + y * gs.mvp[1][0] + z * gs.mvp[2][0] + gs.mvp[3][0];
        v->y = x * gs.mvp[0][1] + y * gs.mvp[1][1] + z * gs.mvp[2][1] + gs.mvp[3][1];
        v->z = x * gs.mvp[0][2] + y * gs.mvp[1][2] + z * gs.mvp[2][2] + gs.mvp[3][2];
        v->w = x * gs.mvp[0][3] + y * gs.mvp[1][3] + z * gs.mvp[2][3] + gs.mvp[3][3];
        float s = (int16_t)port_g16(p + 8), t = (int16_t)port_g16(p + 10);
        if (gs.geom & 0x20000) {
            float nx = (int8_t)p[12], ny = (int8_t)p[13], nz = (int8_t)p[14];
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 0) { nx /= len; ny /= len; nz /= len; }
            float c[3] = { gs.lcol[gs.nlights][0], gs.lcol[gs.nlights][1], gs.lcol[gs.nlights][2] };
            for (int l = 0; l < gs.nlights; l++) {
                float d = nx * ldir[l][0] + ny * ldir[l][1] + nz * ldir[l][2];
                if (d > 0)
                    for (int j = 0; j < 3; j++)
                        c[j] += d * gs.lcol[l][j];
            }
            v->r = c[0] > 255 ? 255 : c[0];
            v->g = c[1] > 255 ? 255 : c[1];
            v->b = c[2] > 255 ? 255 : c[2];
            if (gs.geom & 0x40000) {    /* G_TEXTURE_GEN: sphere map */
                /* the normal's projections on the look-at vectors, -1..1,
                   to 0..1 of the texture scale in units of 1/64 texel:
                   a 32-texel map takes gSPTexture(0x07C0, 0x07C0), and
                   its texels 0..31 are the whole hemisphere (rather than
                   wrapping it twice, which put the Rare logo's map's dark
                   texels on the back of the logo) */
                float ex = nx * ladir[0][0] + ny * ladir[0][1] + nz * ladir[0][2];
                float ey = nx * ladir[1][0] + ny * ladir[1][1] + nz * ladir[1][2];
                s = (ex * 0.5f + 0.5f) * 32 * 32 * 32;
                t = (ey * 0.5f + 0.5f) * 32 * 32 * 32;
            }
        } else {
            v->r = p[12];
            v->g = p[13];
            v->b = p[14];
        }
        v->a = p[15];
        if ((gs.geom & 0x10000) && v->w > 0) {  /* G_FOG: the fog factor into shade alpha */
            float f = v->z / v->w * gs.fog_mul + gs.fog_off;
            v->a = f < 0 ? 0 : f > 255 ? 255 : f;
        }
        v->s = s * gs.tex_s / 32.0f;
        v->t = t * gs.tex_t / 32.0f;
    }
#endif
    int m = n < 16 - v0 ? n : 16 - v0;
    if (recording)
        rec_load(seg_to_k0(w1), v0, m);
    vtx_tail(w1, seg_to_k0(w1), v0, m);
}

/* gSPModifyVertex (G_MW_POINTS): the RSP's vertex buffer is 40 bytes a
   vertex; the game rewrites texture coordinates this way */
NOINLINE static void modify_vertex(int off, uint32_t val) {
    if (recording) {
        uint32_t *e = rec_ev(R_MODV, 3);
        e[1] = (uint32_t)off;
        e[2] = val;
    }
    Vtx4 *v = &gs.v[(off / 40) & 15];
    switch (off % 40) {
    case 0x10:                                  /* G_MWO_POINT_RGBA */
        v->r = val >> 24; v->g = (val >> 16) & 0xFF; v->b = (val >> 8) & 0xFF; v->a = val & 0xFF;
        break;
    case 0x14:                                  /* G_MWO_POINT_ST, s10.5 texels */
        v->s = (int16_t)(val >> 16) / 32.0f;
        v->t = (int16_t)(val & 0xFFFF) / 32.0f;
        break;
    default:
        break;
    }
}

/* ---- the RDP: textures ---------------------------------------------------------- */

/* TMEM as the RDP lays it out: 8-byte words, and on odd rows the two
   32-bit halves of each word swapped (so that a row pair can be read in
   one cycle).  32-bit texels are split: red/green in the low 2 KB, blue/
   alpha at the same offset in the high 2 KB. */
static inline void tmem_put(uint32_t a, int odd, const uint8_t *px, int siz) {
    a ^= odd ? 4 : 0;
    if (siz == 3) {
        a &= 0x7FF;
        gfx_tmem[a] = px[0];
        gfx_tmem[(a + 1) & 0x7FF] = px[1];
        gfx_tmem[0x800 + a] = px[2];
        gfx_tmem[0x800 + ((a + 1) & 0x7FF)] = px[3];
    } else {
        gfx_tmem[a & 0xFFF] = px[0];
    }
}

static unsigned long long st_loads[4];     /* LoadBlock, LoadTile, LoadTLUT, in-between passes' */
static unsigned long long st_tmem_syncs;

static int bytes_per_texel_x2(int siz) { return siz == 0 ? 1 : siz == 1 ? 2 : siz == 2 ? 4 : 8; }

/* n bytes (not 32-bit texels) into TMEM at a, a run of whole words at once
   where they are whole words: tmem_put's layout, without its cost per byte
   (the loads run for every texture, twice with --interpolate) */
static void tmem_put_bytes(uint32_t a, int odd, const uint8_t *px, uint32_t n) {
    uint32_t i = 0;
    if (!(a & 7) && a < 4096) {
        uint32_t whole = n & ~7u;
        if (whole > 4096 - a)
            whole = 4096 - a;
        uint8_t *d = gfx_tmem + a;
        /* a word at a time, its halves exchanged on odd rows (a rotation
           by 0 or 32); not memcpy for the even ones: the rows are short,
           and in WebAssembly memcpy is a call, and memory.copy a slow one */
        unsigned rot = odd ? 32 : 0;
        for (; i < whole; i += 8) {
            uint64_t v;
            memcpy(&v, px + i, 8);
            v = __builtin_rotateleft64(v, rot);
            memcpy(d + i, &v, 8);
        }
        i = whole;
    }
    for (; i < n; i++)
        tmem_put(a + i, odd, px + i, 0);
}

/* --hd-text: the RDRAM address each TMEM word was loaded from by a
   LoadBlock (0: not one), for the OpenGL renderer to recognize the font's
   glyphs by (hdtext.c) */
uint32_t gfx_tmem_src[512];
static void tmem_src_set(uint32_t dst, uint32_t src, uint32_t bytes) {
    for (uint32_t i = 0; i < bytes; i += 8)
        gfx_tmem_src[((dst + i) >> 3) & 511] = src ? src + i : 0;
}

/* A TMEM load: its state (the tile's size, the generations) changes when
   the command runs, and its bytes are copied by tload_copy: at once, or in
   the OpenGL renderer's in-between passes only when something reads TMEM.
   They are what the first pass loaded at the same point (RDRAM hasn't
   changed since), so the textures the first pass looked up are the same
   at the same gs.tmem_gen, and gfx_gl.c finds them by it without reading
   TMEM (gfx_tmem_sync when it can't). */
typedef struct {
    uint8_t op;                 /* 0xF3 LoadBlock, 0xF4 LoadTile, 0xF0 LoadTLUT */
    uint32_t w0, w1;
    uint32_t timg_addr;
    int timg_siz, timg_w;
    int tmem, line;             /* the tile's, when the load ran */
    uint64_t words[8];          /* the TMEM words it writes (or may: a bit each) */
} TLoad;

static TLoad *tpend;            /* the in-between pass's loads not copied yet, in order */
static int tpend_n, tpend_cap;
static int tmem_lazy;

static void tload_copy(const TLoad *l) {
    uint32_t w0 = l->w0, w1 = l->w1;
    int bpt2 = bytes_per_texel_x2(l->timg_siz);
    if (l->op == 0xF3) {
        int uls = (w0 >> 12) & 0xFFF, ult = w0 & 0xFFF, lrs = (w1 >> 12) & 0xFFF;
        uint32_t dxt = w1 & 0xFFF;
        uint32_t bytes = (uint32_t)(lrs - uls + 1) * bpt2 / 2;
        uint32_t src = l->timg_addr + ((uint32_t)ult * l->timg_w + uls) * bpt2 / 2;
        uint32_t dst = l->tmem * 8;
        const uint8_t *p = port_ptr(src);
        if (bytes > 4096)
            bytes = 4096;
        PORT_ACCESS_BYTES(p, bytes);        /* texels are bytes in either byte order */
        if (hdtext_on)
            tmem_src_set(dst, src, bytes);
        /* the RDP counts rows by adding dxt for every 8-byte word */
        if (l->timg_siz == 3) {
            for (uint32_t i = 0; i < bytes; i += 4)
                tmem_put(dst + i / 2, ((i / 8) * dxt >> 11) & 1, p + i, 3);
            return;
        }
        /* (the row, and so the swap, changes only between words: the words
           of a row at a time) */
        uint32_t words = (bytes + 7) / 8;
        for (uint32_t w = 0; w < words;) {
            uint32_t row = w * dxt >> 11, end = words;
            if (dxt) {
                end = (((row + 1) << 11) + dxt - 1) / dxt;
                if (end > words)
                    end = words;
            }
            uint32_t n = end * 8 < bytes ? (end - w) * 8 : bytes - w * 8;
            tmem_put_bytes(dst + w * 8, row & 1, p + w * 8, n);
            w = end;
        }
    } else if (l->op == 0xF4) {
        int uls = ((w0 >> 12) & 0xFFF) >> 2, ult = (w0 & 0xFFF) >> 2;
        int lrs = ((w1 >> 12) & 0xFFF) >> 2, lrt = (w1 & 0xFFF) >> 2;
        uint32_t rowbytes = (uint32_t)(lrs - uls + 1) * bpt2 / 2;
        if (hdtext_on)
            tmem_src_set(l->tmem * 8, 0, (uint32_t)(lrt - ult + 1) * l->line * 8);
        for (int y = ult; y <= lrt; y++) {
            const uint8_t *p = port_ptr(l->timg_addr + ((uint32_t)y * l->timg_w + uls) * bpt2 / 2);
            int odd = (y - ult) & 1;
            uint32_t dst = l->tmem * 8 + (uint32_t)(y - ult) * l->line * 8;
            PORT_ACCESS_BYTES(p, rowbytes < 4096 ? rowbytes : 4096);
            if (l->timg_siz == 3) {
                for (uint32_t i = 0; i < rowbytes && i / 2 < 2048; i += 4)
                    tmem_put(dst + i / 2, odd, p + i, 3);
            } else {
                tmem_put_bytes(dst, odd, p, rowbytes < 4096 ? rowbytes : 4096);
            }
        }
    } else {
        int uls = ((w0 >> 12) & 0xFFF) >> 2, lrs = ((w1 >> 12) & 0xFFF) >> 2;
        const uint8_t *p = port_ptr(l->timg_addr + (uint32_t)uls * 2);
        uint32_t dst = l->tmem * 8;
        PORT_ACCESS_BYTES(p, 2 * (uint32_t)(lrs - uls + 1));
        if (hdtext_on)
            tmem_src_set(dst, 0, 2 * (uint32_t)(lrs - uls + 1));
        for (int i = 0; i <= lrs - uls && dst + 2 * i + 1 < 4096; i++) {
            gfx_tmem[dst + 2 * i] = p[2 * i];
            gfx_tmem[dst + 2 * i + 1] = p[2 * i + 1];
        }
    }
}

/* words a..a+n-1 of TMEM (mod 512) into a mask of them */
static void words_add(uint64_t *m, uint32_t a, uint32_t n) {
    if (n >= 512) {
        memset(m, 0xFF, 8 * sizeof *m);
        return;
    }
    while (n) {
        a &= 511;
        uint32_t b = a & 63, k = 64 - b < n ? 64 - b : n;
        m[a >> 6] |= (k == 64 ? ~0ull : ((1ull << k) - 1)) << b;
        a += k;
        n -= k;
    }
}

/* the bytes start..start+len-1 of TMEM as the loads so far left them, for
   what reads them: the pending loads that write them, and those that write
   what those do before them (so that what's left to copy never overlaps
   what was copied after it) */
void gfx_tmem_sync_range(uint32_t start, uint32_t len) {
    if (!tpend_n)
        return;
    uint64_t need[8] = { 0 };
    words_add(need, start / 8, (start % 8 + len + 7) / 8);
    int any = 0;
    static uint8_t *take;
    static int take_cap;
    if (take_cap < tpend_n) {
        take_cap = tpend_cap;
        take = realloc(take, (size_t)take_cap);
    }
    for (int i = tpend_n - 1; i >= 0; i--) {
        uint64_t o = 0;
        for (int j = 0; j < 8; j++)
            o |= need[j] & tpend[i].words[j];
        take[i] = o != 0;
        if (o) {
            for (int j = 0; j < 8; j++)
                need[j] |= tpend[i].words[j];
            any = 1;
        }
    }
    if (!any)
        return;
    int n = 0;
    for (int i = 0; i < tpend_n; i++) {
        if (take[i]) {
            tload_copy(&tpend[i]);
            st_tmem_syncs++;
        } else {
            tpend[n++] = tpend[i];
        }
    }
    tpend_n = n;
}

/* all of TMEM */
void gfx_tmem_sync(void) {
    st_tmem_syncs += tpend_n;
    for (int i = 0; i < tpend_n; i++)
        tload_copy(&tpend[i]);
    tpend_n = 0;
}

static void tload_lazy(TLoad l);

static void tload(uint8_t op, uint32_t w0, uint32_t w1) {
    const Tile *t = &gs.tile[(w1 >> 24) & 7];
    TLoad l = { op, w0, w1, gs.timg_addr, gs.timg_siz, gs.timg_w, t->tmem, t->line, { 0 } };
    st_loads[op == 0xF3 ? 0 : op == 0xF4 ? 1 : 2]++;
    st_loads[3] += ipass;
    if (recording) {
        uint32_t *e = rec_raw(R_TLOAD, 9);
        e[1] = op; e[2] = w0; e[3] = w1; e[4] = gs.timg_addr; e[5] = (uint32_t)gs.timg_siz;
        e[6] = (uint32_t)gs.timg_w; e[7] = (uint32_t)t->tmem; e[8] = (uint32_t)t->line;
    }
    if (!tmem_lazy) {
        tload_copy(&l);
        return;
    }
    tload_lazy(l);
}

/* a load into the pending ones (an in-between pass's) */
static void tload_lazy(TLoad l) {
    uint8_t op = l.op;
    uint32_t w0 = l.w0, w1 = l.w1;
    /* what it writes, as tload_copy does (a 32-bit texture's: all of it) */
    int bpt2 = bytes_per_texel_x2(l.timg_siz);
    if (l.timg_siz == 3) {
        words_add(l.words, 0, 512);
    } else if (op == 0xF3) {
        uint32_t bytes = (uint32_t)((((w1 >> 12) & 0xFFF) - ((w0 >> 12) & 0xFFF) + 1) * bpt2 / 2);
        words_add(l.words, l.tmem, ((bytes > 4096 ? 4096 : bytes) + 7) / 8);
    } else if (op == 0xF4) {
        int uls = ((w0 >> 12) & 0xFFF) >> 2, ult = (w0 & 0xFFF) >> 2;
        int lrs = ((w1 >> 12) & 0xFFF) >> 2, lrt = (w1 & 0xFFF) >> 2;
        uint32_t rowbytes = (uint32_t)(lrs - uls + 1) * bpt2 / 2;
        if (rowbytes > 4096)
            rowbytes = 4096;
        uint32_t rows = lrt >= ult ? (uint32_t)(lrt - ult + 1) : 0;
        if (rows)
            words_add(l.words, l.tmem, (rows - 1) * l.line + (rowbytes + 7) / 8 > rows * l.line
                                            ? (rows - 1) * l.line + (rowbytes + 7) / 8 : rows * l.line);
    } else {
        int uls = ((w0 >> 12) & 0xFFF) >> 2, lrs = ((w1 >> 12) & 0xFFF) >> 2;
        words_add(l.words, l.tmem, (2 * (uint32_t)(lrs - uls + 1) + 7) / 8);
    }
    if (tpend_n == tpend_cap) {
        tpend_cap = tpend_cap ? tpend_cap * 2 : 1024;
        tpend = realloc(tpend, (size_t)tpend_cap * sizeof *tpend);
    }
    tpend[tpend_n++] = l;
}

NOINLINE static void load_block(uint32_t w0, uint32_t w1) {
    tload(0xF3, w0, w1);
    gs.tmem_gen++;
}

NOINLINE static void load_tile(uint32_t w0, uint32_t w1) {
    Tile *t = &gs.tile[(w1 >> 24) & 7];
    tload(0xF4, w0, w1);
    int uls = ((w0 >> 12) & 0xFFF) >> 2, ult = (w0 & 0xFFF) >> 2;
    int lrs = ((w1 >> 12) & 0xFFF) >> 2, lrt = (w1 & 0xFFF) >> 2;
    t->uls = uls << 2; t->ult = ult << 2; t->lrs = lrs << 2; t->lrt = lrt << 2;
    gs.tmem_gen++;
    gs.tile_gen++;
}

NOINLINE static void load_tlut(uint32_t w0, uint32_t w1) {
    tload(0xF0, w0, w1);
    gs.tmem_gen++;
}

static inline int wrap(int c, int cm, int mask, int size) {
    if ((cm & 2) || !mask) {            /* clamp */
        if (c < 0) c = 0;
        if (c > size) c = size;
    }
    if (mask) {
        int m = (1 << mask) - 1;
        int mir = (cm & 1) && (c & (1 << mask));
        c &= m;
        if (mir)
            c = m - c;
    }
    return c;
}

/* the texels a tile can address after wrapping: what the GPU decodes */
void gfx_tile_dims(const Tile *t, int *w, int *h) {
    int ss = (t->lrs - t->uls) >> 2, st = (t->lrt - t->ult) >> 2;
    int ws = t->masks ? 1 << t->masks : ss + 1, ht = t->maskt ? 1 << t->maskt : st + 1;
    *w = ws < 1 ? 1 : ws > 1024 ? 1024 : ws;
    *h = ht < 1 ? 1 : ht > 1024 ? 1024 : ht;
}

static void rgba16(uint16_t c, uint8_t *o) {
    o[0] = ((c >> 11) & 31) * 255 / 31;
    o[1] = ((c >> 6) & 31) * 255 / 31;
    o[2] = ((c >> 1) & 31) * 255 / 31;
    o[3] = (c & 1) ? 255 : 0;
}

static void tlut(int idx, uint8_t *o) {
    uint16_t c = port_be16(gfx_tmem + 0x800 + (idx & 0xFF) * 2);
    if ((gs.om_h >> 14 & 3) == 3) {     /* IA16 */
        o[0] = o[1] = o[2] = c >> 8;
        o[3] = c & 0xFF;
    } else {
        rgba16(c, o);
    }
}

static inline int ifloor(float f) {
    int i = (int)f;
    return i - (f < (float)i);
}

/* does the combiner read TEXEL1 (in either cycle)? */
int gfx_uses_tex1(void) {
    uint32_t w0 = gs.cc0, w1 = gs.cc1;
    int sel[] = { (w0 >> 20) & 15, (w1 >> 28) & 15, (w0 >> 15) & 31, (w1 >> 15) & 7,
                  (w0 >> 5) & 15, (w1 >> 24) & 15, w0 & 31, (w1 >> 6) & 7 };
    int asel[] = { (w0 >> 12) & 7, (w1 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 9) & 7,
                   (w1 >> 21) & 7, (w1 >> 3) & 7, (w1 >> 18) & 7, w1 & 7 };
    int two = gfx_cycles() == 2;
    for (int i = 0; i < 8; i++) {
        /* the second cycle of 2-cycle mode reads tile + 1 as TEXEL0 */
        int t = two && i >= 4 ? 1 : 2;
        if (sel[i] == t || asel[i] == t || ((i & 3) == 2 && sel[i] == t + 7))
            return 1;
    }
    return 0;
}

/* one texel of a tile, at wrapped coordinates */
void gfx_fetch_texel(const Tile *t, int s, int tt, uint8_t *o) {
    uint32_t row = t->tmem * 8 + (uint32_t)tt * t->line * 8;
    uint32_t x = (tt & 1) ? 4 : 0;      /* the odd-row swizzle */
    int fmt = t->fmt;
    switch (t->siz) {
    case 0: {                                       /* 4-bit */
        uint8_t b = gfx_tmem[((row + s / 2) ^ x) & 4095];
        int v = (s & 1) ? b & 15 : b >> 4;
        if (fmt == 2) { tlut(v | (t->pal << 4), o); return; }
        if (fmt == 3) {                             /* IA4 */
            o[0] = o[1] = o[2] = (v >> 1) * 255 / 7;
            o[3] = (v & 1) ? 255 : 0;
            return;
        }
        o[0] = o[1] = o[2] = o[3] = v * 17;         /* I4 */
        return;
    }
    case 1: {                                       /* 8-bit */
        uint8_t v = gfx_tmem[((row + s) ^ x) & 4095];
        if (fmt == 2) { tlut(v, o); return; }
        if (fmt == 3) {                             /* IA8 */
            o[0] = o[1] = o[2] = (v >> 4) * 17;
            o[3] = (v & 15) * 17;
            return;
        }
        o[0] = o[1] = o[2] = o[3] = v;
        return;
    }
    case 2: {                                       /* 16-bit */
        uint32_t a = ((row + s * 2) ^ x) & 4094;
        uint16_t c = (uint16_t)(gfx_tmem[a] << 8 | gfx_tmem[a + 1]);
        if (fmt == 3) {                             /* IA16 */
            o[0] = o[1] = o[2] = c >> 8;
            o[3] = c & 0xFF;
            return;
        }
        if (fmt == 2) { tlut(c >> 8, o); return; }
        rgba16(c, o);
        return;
    }
    default: {                                      /* 32-bit, split */
        uint32_t a = ((row + s * 2) ^ x) & 0x7FE;
        o[0] = gfx_tmem[a]; o[1] = gfx_tmem[a + 1];
        o[2] = gfx_tmem[0x800 + a]; o[3] = gfx_tmem[0x801 + a];
        return;
    }
    }
}

int gfx_cycle_type(void) { return (gs.om_h >> 20) & 3; }
int gfx_cycles(void) { return gfx_cycle_type() == 1 ? 2 : 1; }

int gfx_filter_mode(void) {
    if (gfx_cycle_type() >= 2 || !((gs.om_h >> 12) & 3) || gfx_filter == GFX_FILTER_POINT)
        return 0;
    return gfx_filter == GFX_FILTER_BILINEAR ? 2 : 1;
}

int gfx_lod_on(void) { return (gs.om_h >> 16) & 1; }

/* the RDP's LOD: which tiles, and LOD_FRACTION (0..1) */
void gfx_lod_tiles(float lod, int base, int *tile0, int *tile1, float *frac) {
    int max = gs.tex_levels;
    if (lod < 1) {                                  /* magnified */
        *tile0 = base;
        *tile1 = (base + 1) & 7;
        *frac = max == 0 ? 1 : 0;
        if (max == 0)
            *tile1 = base;
        return;
    }
    int l = ilogbf(lod);
    if (l >= max) {                                 /* past the last level */
        *tile0 = *tile1 = (base + max) & 7;
        *frac = 1;
        return;
    }
    *tile0 = (base + l) & 7;
    *tile1 = (base + l + 1) & 7;
    *frac = floorf((lod / (float)(1 << l) - 1) * 256) / 256;
}

/* a filtered sample at s, t (texels) */
static void sample(int tile, float fs, float ft, uint8_t *o, int filt) {
    Tile *t = &gs.tile[tile & 7];
    if (t->shifts) fs = t->shifts < 11 ? fs / (1 << t->shifts) : fs * (1 << (16 - t->shifts));
    if (t->shiftt) ft = t->shiftt < 11 ? ft / (1 << t->shiftt) : ft * (1 << (16 - t->shiftt));
    fs -= t->uls * 0.25f;
    ft -= t->ult * 0.25f;
    int ms = (t->lrs - t->uls) >> 2, mt = (t->lrt - t->ult) >> 2;
    int s0 = ifloor(fs), t0 = ifloor(ft);
    if (!filt) {
        gfx_fetch_texel(t, wrap(s0, t->cms, t->masks, ms), wrap(t0, t->cmt, t->maskt, mt), o);
        return;
    }
    /* the RDP's fractions are 5 bits */
    float fx = floorf((fs - s0) * 32) / 32, fy = floorf((ft - t0) * 32) / 32;
    int sa = wrap(s0, t->cms, t->masks, ms), sb = wrap(s0 + 1, t->cms, t->masks, ms);
    int ta = wrap(t0, t->cmt, t->maskt, mt), tb = wrap(t0 + 1, t->cmt, t->maskt, mt);
    uint8_t c00[4], c10[4], c01[4], c11[4];
    gfx_fetch_texel(t, sa, ta, c00);
    gfx_fetch_texel(t, sb, ta, c10);
    gfx_fetch_texel(t, sa, tb, c01);
    gfx_fetch_texel(t, sb, tb, c11);
    for (int ch = 0; ch < 4; ch++) {
        float v;
        if (filt == 2)
            v = (c00[ch] * (1 - fx) + c10[ch] * fx) * (1 - fy) + (c01[ch] * (1 - fx) + c11[ch] * fx) * fy;
        else if (fx + fy < 1)                       /* N64 3-point */
            v = c00[ch] + fx * (c10[ch] - c00[ch]) + fy * (c01[ch] - c00[ch]);
        else
            v = c11[ch] + (1 - fx) * (c01[ch] - c11[ch]) + (1 - fy) * (c10[ch] - c11[ch]);
        o[ch] = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v + 0.5f);
    }
}

/* ---- the RDP: combiner, blender, pixels ------------------------------------------ */

typedef struct {
    float shade[4];
    uint8_t t0[4], t1[4];
    float lod;
} Inputs;

static inline float clamp255(float v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

/* The combiner: (A - B) * C + D per channel, both cycles.  The selectors
   are turned into indices into one table of inputs when the mode changes. */
static uint8_t cc_idx[2][8];
static int cc_cycles;
static uint32_t cc_key0 = 1, cc_key1, cc_keyh;

static int map_a(int s) { return s <= 5 ? s : s == 6 ? CC_ONE : s == 7 ? CC_NOISE : CC_ZERO; }
static int map_b(int s) { return s <= 5 ? s : CC_ZERO; }
static int map_c(int s) {
    static const uint8_t m[] = { CC_COMB, CC_T0, CC_T1, CC_PRIM, CC_SHADE, CC_ENV, CC_ZERO, CC_COMB_A, CC_T0_A,
                                 CC_T1_A, CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD, CC_PRIM_LOD };
    return s < 15 ? m[s] : CC_ZERO;
}
static int map_d(int s) { return s <= 5 ? s : s == 6 ? CC_ONE : CC_ZERO; }
static int map_aa(int s) { return s <= 5 ? s : s == 6 ? CC_ONE : CC_ZERO; }
static int map_ac(int s) { return s == 0 ? CC_LOD : s <= 5 ? s : s == 6 ? CC_PRIM_LOD : CC_ZERO; }

void gfx_cc_decode(uint8_t idx[2][8]) {
    uint32_t w0 = gs.cc0, w1 = gs.cc1;
    int sel[2][8] = {
        { (w0 >> 20) & 15, (w1 >> 28) & 15, (w0 >> 15) & 31, (w1 >> 15) & 7,
          (w0 >> 12) & 7, (w1 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 9) & 7 },
        { (w0 >> 5) & 15, (w1 >> 24) & 15, w0 & 31, (w1 >> 6) & 7,
          (w1 >> 21) & 7, (w1 >> 3) & 7, (w1 >> 18) & 7, w1 & 7 },
    };
    for (int c = 0; c < 2; c++) {
        idx[c][0] = map_a(sel[c][0]);
        idx[c][1] = map_b(sel[c][1]);
        idx[c][2] = map_c(sel[c][2]);
        idx[c][3] = map_d(sel[c][3]);
        idx[c][4] = map_aa(sel[c][4]);
        idx[c][5] = map_aa(sel[c][5]);
        idx[c][6] = map_ac(sel[c][6]);
        idx[c][7] = map_aa(sel[c][7]);
    }
    /* In the second cycle of 2-cycle mode the RDP's texel inputs move up
       one: TEXEL0 is tile + 1's texel, and TEXEL1 is the next pixel's
       tile texel (taken here as this pixel's).  The TVs' video relies on
       it (TEXEL1 * SHADE in the second cycle). */
    if (gfx_cycles() == 2)
        for (int i = 0; i < 8; i++) {
            uint8_t *k = &idx[1][i];
            *k = *k == CC_T0 ? CC_T1 : *k == CC_T1 ? CC_T0 : *k == CC_T0_A ? CC_T1_A : *k == CC_T1_A ? CC_T0_A : *k;
        }
}

static void cc_prepare(void) {
    gfx_cc_decode(cc_idx);
    cc_cycles = gfx_cycles();
    cc_key0 = gs.cc0;
    cc_key1 = gs.cc1;
    cc_keyh = gs.om_h;
}

static inline void splat(float *d, float v) { d[0] = d[1] = d[2] = d[3] = v; }
static uint32_t cc_noise = 0x12345678;         /* the combiner's NOISE */

static void combine(const Inputs *in, float *out) {
    if (gs.cc0 != cc_key0 || gs.cc1 != cc_key1 || gs.om_h != cc_keyh)
        cc_prepare();
    float t[CC_N][4];
    for (int ch = 0; ch < 4; ch++) {
        t[CC_COMB][ch] = 0;
        t[CC_T0][ch] = in->t0[ch];
        t[CC_T1][ch] = in->t1[ch];
        t[CC_PRIM][ch] = gs.prim[ch];
        t[CC_SHADE][ch] = in->shade[ch];
        t[CC_ENV][ch] = gs.env[ch];
    }
    splat(t[CC_ONE], 255);
    splat(t[CC_ZERO], 0);
    splat(t[CC_COMB_A], 0);
    splat(t[CC_T0_A], in->t0[3]);
    splat(t[CC_T1_A], in->t1[3]);
    splat(t[CC_PRIM_A], gs.prim[3]);
    splat(t[CC_SHADE_A], in->shade[3]);
    splat(t[CC_ENV_A], gs.env[3]);
    splat(t[CC_LOD], in->lod);
    splat(t[CC_PRIM_LOD], gs.prim_lod);
    cc_noise ^= cc_noise << 13; cc_noise ^= cc_noise >> 17; cc_noise ^= cc_noise << 5;
    splat(t[CC_NOISE], (float)(cc_noise >> 24));
    for (int c = 0; c < cc_cycles; c++) {
        const uint8_t *k = cc_idx[c];
        float r[4];
        for (int ch = 0; ch < 3; ch++)
            r[ch] = clamp255((t[k[0]][ch] - t[k[1]][ch]) * t[k[2]][ch] * (1.0f / 255.0f) + t[k[3]][ch]);
        r[3] = clamp255((t[k[4]][3] - t[k[5]][3]) * t[k[6]][3] * (1.0f / 255.0f) + t[k[7]][3]);
        memcpy(t[CC_COMB], r, sizeof r);
        splat(t[CC_COMB_A], r[3]);
    }
    memcpy(out, t[CC_COMB], 4 * sizeof(float));
}

static void read_pixel(int x, int y, float *c) {
    uint8_t *p = port_ptr(gs.cimg_addr);
    if (cur_wfb) {
        uint8_t o[4];
        rgba16(cur_wfb->px[y * cur_wfb->w + x + gfx_wide_off], o);
        c[0] = o[0]; c[1] = o[1]; c[2] = o[2]; c[3] = 255;
    } else if (gs.cimg_siz == 1) {
        c[0] = c[1] = c[2] = c[3] = p[y * gs.cimg_w + x];
    } else if (gs.cimg_siz == 3) {
        uint8_t *q = p + 4 * (y * gs.cimg_w + x);
        c[0] = q[0]; c[1] = q[1]; c[2] = q[2]; c[3] = q[3];
    } else {
        uint8_t o[4];
        rgba16(port_be16(p + 2 * (y * gs.cimg_w + x)), o);
        c[0] = o[0]; c[1] = o[1]; c[2] = o[2]; c[3] = 255;
    }
}

static void write_pixel(int x, int y, const float *c) {
    uint8_t *p = port_ptr(gs.cimg_addr);
    if (gs.cimg_siz == 1) {
        p[y * gs.cimg_w + x] = c[0];
    } else if (gs.cimg_siz == 3) {
        uint8_t *q = p + 4 * (y * gs.cimg_w + x);
        q[0] = c[0]; q[1] = c[1]; q[2] = c[2]; q[3] = c[3];
    } else {
        uint16_t v = (uint16_t)(((int)c[0] >> 3) << 11 | ((int)c[1] >> 3) << 6 | ((int)c[2] >> 3) << 1 | 1);
        if (cur_wfb)
            cur_wfb->px[y * cur_wfb->w + x + gfx_wide_off] = v;
        else
            port_wbe16(p + 2 * (y * gs.cimg_w + x), v);
    }
}

/* The blender: returns 0 to drop the pixel.  Alpha compare and coverage
   from alpha first; then the blend equation (P * A + M * B) / (A + B) for
   each cycle.  Without FORCE_BL the last cycle only blends partly covered
   pixels on the hardware (anti-aliasing), which isn't emulated: the pixel
   is the input.  gfx_gl.c compiles the same rules (gfx_cvg_drops). */
static int blend(int x, int y, float *c, const float *shade) {
    uint32_t l = gs.om_l;
    if ((l & 3) == 1 && c[3] < gs.blend[3])         /* alpha compare: threshold */
        return 0;
    if (gfx_cvg_drops(l, c[3]))                     /* coverage from alpha */
        return 0;
    int cyc = gfx_cycles();
    if (!(l & 0x4000))                              /* no FORCE_BL: the last cycle passes */
        cyc--;
    float in[4] = { c[0], c[1], c[2], c[3] };
    for (int k = 0; k < cyc; k++) {
        int sh = k == 0 ? 0 : 2;
        int P = (l >> (30 - sh)) & 3, A = (l >> (26 - sh)) & 3, M = (l >> (22 - sh)) & 3, B = (l >> (18 - sh)) & 3;
        float mem[4] = { 0, 0, 0, 255 }, pc[3], mc[3], a, b;
        if (P == 1 || M == 1 || B == 1)
            read_pixel(x, y, mem);
        float bl[3] = { gs.blend[0], gs.blend[1], gs.blend[2] }, fg[3] = { gs.fog[0], gs.fog[1], gs.fog[2] };
        for (int ch = 0; ch < 3; ch++) {
            pc[ch] = P == 0 ? in[ch] : P == 1 ? mem[ch] : P == 2 ? bl[ch] : fg[ch];
            mc[ch] = M == 0 ? in[ch] : M == 1 ? mem[ch] : M == 2 ? bl[ch] : fg[ch];
        }
        a = A == 0 ? in[3] : A == 1 ? gs.fog[3] : A == 2 ? shade[3] : 0;
        b = B == 0 ? 255 - a : B == 1 ? mem[3] : B == 2 ? 255 : 0;
        float sum = a + b;
        for (int ch = 0; ch < 3; ch++)
            in[ch] = sum > 0 ? (pc[ch] * a + mc[ch] * b) / sum : pc[ch];
    }
    c[0] = in[0]; c[1] = in[1]; c[2] = in[2];
    return 1;
}

static int zbuf_ok(void) { return gs.zimg_addr && gs.cimg_w <= 640; }

/* ---- triangles ---------------------------------------------------------------------- */

typedef struct {
    float x, y, z, iw;          /* screen, z 0..1, 1/w */
    float s, t, r, gg, b, a;    /* divided by w */
} SV;

NOINLINE static void raster(const SV *v0, const SV *v1, const SV *v2, const float *flat) {
    if (tpend_n)
        gfx_tmem_sync();
    float minx = min_f(v0->x, min_f(v1->x, v2->x)), maxx = max_f(v0->x, max_f(v1->x, v2->x));
    float miny = min_f(v0->y, min_f(v1->y, v2->y)), maxy = max_f(v0->y, max_f(v1->y, v2->y));
    int x0 = (int)floorf(minx), x1 = (int)ceilf(maxx), y0 = (int)floorf(miny), y1 = (int)ceilf(maxy);
    int sx0 = gs.sc_x0, sx1 = gs.sc_x1;
    if (cur_wfb)
        gfx_wide_span(gs.sc_x0, gs.sc_x1, &sx0, &sx1);
    if (x0 < sx0) x0 = sx0;
    if (y0 < gs.sc_y0) y0 = gs.sc_y0;
    if (x1 > sx1) x1 = sx1;
    if (y1 > gs.sc_y1) y1 = gs.sc_y1;
    float area = (v1->x - v0->x) * (v2->y - v0->y) - (v1->y - v0->y) * (v2->x - v0->x);
    if (fabsf(area) < 1e-6f)
        return;
    int zcmp = (gs.geom & 1) && (gs.om_l & 0x10) && zbuf_ok();
    int zupd = (gs.geom & 1) && (gs.om_l & 0x20) && zbuf_ok();
    int decal = ((gs.om_l >> 10) & 3) == 3;
    int textured = gs.tex_on;
    int tile = gs.tex_tile;
    int tex1 = textured && gfx_uses_tex1();
    int filt = gfx_filter_mode();
    int lod = textured && gfx_lod_on();
    st_raster++;
    float ia = 1.0f / area;
    /* barycentrics are linear in x and y: step them */
    float d0x = (v1->y - v2->y) * ia, d1x = (v2->y - v0->y) * ia;
    float d0y = (v2->x - v1->x) * ia, d1y = (v0->x - v2->x) * ia;
    for (int y = y0; y < y1; y++) {
        float py = y + 0.5f, px0 = x0 + 0.5f;
        float w0 = ((v1->x - px0) * (v2->y - py) - (v1->y - py) * (v2->x - px0)) * ia;
        float w1 = ((v2->x - px0) * (v0->y - py) - (v2->y - py) * (v0->x - px0)) * ia;
        w0 -= d0x;
        w1 -= d1x;
        for (int x = x0; x < x1; x++) {
            w0 += d0x;
            w1 += d1x;
            float w2 = 1 - w0 - w1;
            st_tested++;
            if (w0 < 0 || w1 < 0 || w2 < 0)
                continue;
            st_drawn++;
            float z = w0 * v0->z + w1 * v1->z + w2 * v2->z;
            int zi = y * zb_w + x + zb_off;
            if (zcmp) {
                if (decal ? z > zb[zi] + 0.0005f : z > zb[zi])
                    continue;
            }
            float iw = w0 * v0->iw + w1 * v1->iw + w2 * v2->iw;
            float wv = iw != 0 ? 1.0f / iw : 0;
            Inputs in;
            in.lod = 255;
            if (flat) {
                memcpy(in.shade, flat, sizeof in.shade);
            } else {
                in.shade[0] = (w0 * v0->r + w1 * v1->r + w2 * v2->r) * wv;
                in.shade[1] = (w0 * v0->gg + w1 * v1->gg + w2 * v2->gg) * wv;
                in.shade[2] = (w0 * v0->b + w1 * v1->b + w2 * v2->b) * wv;
                in.shade[3] = (w0 * v0->a + w1 * v1->a + w2 * v2->a) * wv;
            }
            if (textured) {
                float s = (w0 * v0->s + w1 * v1->s + w2 * v2->s) * wv;
                float t = (w0 * v0->t + w1 * v1->t + w2 * v2->t) * wv;
                int ta = tile, tb = tile + 1;
                if (lod) {
                    /* texels per pixel, from the neighbours to the right and below */
                    float ax = w0 + d0x, bx = w1 + d1x, ay = w0 + d0y, by = w1 + d1y;
                    float iwx = ax * v0->iw + bx * v1->iw + (1 - ax - bx) * v2->iw;
                    float iwy = ay * v0->iw + by * v1->iw + (1 - ay - by) * v2->iw;
                    float sx = (ax * v0->s + bx * v1->s + (1 - ax - bx) * v2->s) / iwx;
                    float tx = (ax * v0->t + bx * v1->t + (1 - ax - bx) * v2->t) / iwx;
                    float sy = (ay * v0->s + by * v1->s + (1 - ay - by) * v2->s) / iwy;
                    float ty = (ay * v0->t + by * v1->t + (1 - ay - by) * v2->t) / iwy;
                    float l = max_f(max_f(fabsf(sx - s), fabsf(tx - t)), max_f(fabsf(sy - s), fabsf(ty - t)));
                    float frac;
                    gfx_lod_tiles(l, tile, &ta, &tb, &frac);
                    in.lod = frac * 255;
                }
                sample(ta, s, t, in.t0, filt);
                if (tex1)
                    sample(tb, s, t, in.t1, filt);
                else
                    memset(in.t1, 0, 4);
            } else {
                memset(in.t0, 0, 4);
                memset(in.t1, 0, 4);
            }
            float c[4];
            combine(&in, c);
            if (!blend(x, y, c, in.shade))
                continue;
            write_pixel(x, y, c);
            if (zupd)
                zb[zi] = z;
        }
    }
}

/* iw: 1 / v->w */
static inline void to_screen_iw(const Vtx4 *v, float iw, GfxVtx *o) {
    o->x = gs.vp_trans[0] + v->x * iw * gs.vp_scale[0];
    o->y = gs.vp_trans[1] - v->y * iw * gs.vp_scale[1];
    o->z = (v->z * iw) * 0.5f + 0.5f;
    o->w = v->w;
    o->s = v->s; o->t = v->t;
    o->r = v->r; o->g = v->g; o->b = v->b; o->a = v->a;
}

static void to_screen(const Vtx4 *v, GfxVtx *o) { to_screen_iw(v, 1.0f / v->w, o); }

static void to_sv(const GfxVtx *v, SV *o) {
    float iw = 1.0f / v->w;
    o->x = v->x; o->y = v->y; o->z = v->z;
    o->iw = iw;
    o->s = v->s * iw; o->t = v->t * iw;
    o->r = v->r * iw; o->gg = v->g * iw; o->b = v->b * iw; o->a = v->a * iw;
}

static void lerp(Vtx4 *o, const Vtx4 *a, const Vtx4 *b, float t) {
    float *po = (float *)o;
    const float *pa = (const float *)a, *pb = (const float *)b;
    for (unsigned i = 0; i < sizeof(Vtx4) / sizeof(float); i++)
        po[i] = pa[i] + (pb[i] - pa[i]) * t;
}

/* clip a polygon against plane dot(p, v) >= 0 */
static int clip_poly(Vtx4 *in, int n, Vtx4 *out, int plane) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        Vtx4 *a = &in[i], *b = &in[i + 1 < n ? i + 1 : 0];
        float da = plane == 0 ? a->w - 1e-3f : a->z + a->w;
        float db = plane == 0 ? b->w - 1e-3f : b->z + b->w;
        if (da >= 0)
            out[m++] = *a;
        if ((da >= 0) != (db >= 0))
            lerp(&out[m++], a, b, da / (da - db));
    }
    return m;
}

static int gl_target(void) { return gfx_gl_enabled && gfx_gl_owns_target(); }

/* what the RDP will cover, for its time: the polygon's area within the
   scissor's bounding box (the same whichever back end draws it) */
static void charge_poly(const GfxVtx *s, int n) {
    float area = 0, x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
    for (int i = 0; i < n; i++) {
        const GfxVtx *a = &s[i], *b = &s[i + 1 < n ? i + 1 : 0];
        area += a->x * b->y - b->x * a->y;
        x0 = min_f(x0, a->x); x1 = max_f(x1, a->x); y0 = min_f(y0, a->y); y1 = max_f(y1, a->y);
    }
    x0 = max_f(x0, gs.sc_x0); y0 = max_f(y0, gs.sc_y0); x1 = min_f(x1, gs.sc_x1); y1 = min_f(y1, gs.sc_y1);
    if (x1 <= x0 || y1 <= y0)
        return;
    st_cover += min_f(fabsf(area) * 0.5f, (x1 - x0) * (y1 - y0));
}

/* widescreen: a 2D polygon (w 1 throughout: an orthographic projection)
   that spans the game's frame, edge to edge, is stretched to the edges of
   the wide one (the sky's gradient behind the levels, full-screen
   overlays); the rest of the 2D stays in the middle (the HUD, and its
   arrows at the edges that point at what is off screen) */
static void wide_2d(GfxVtx *s, int n) {
    float x0 = 1e9f, x1 = -1e9f;
    for (int i = 0; i < n; i++) {
        if (fabsf(s[i].w - 1) >= 1e-4f)
            return;
        x0 = min_f(x0, s[i].x);
        x1 = max_f(x1, s[i].x);
    }
    if (x0 > 1 || x1 < 318)                     /* (within a pixel) */
        return;
    /* the attributes (depth, texture coordinates, shade) continue as the
       polygon has them, a plane over the screen (w is 1: no perspective),
       so that a picture continues past the edges (the sky's panorama)
       rather than being stretched */
    GfxVtx ref[3] = { s[0], s[1], s[2] };
    const float *a = &ref[0].x, *b = &ref[1].x, *c = &ref[2].x;
    float ux = b[0] - a[0], uy = b[1] - a[1], vx = c[0] - a[0], vy = c[1] - a[1];
    float det = ux * vy - uy * vx;
    /* (a texture that doesn't wrap across would show what is beside it in
       TMEM: stretched then) */
    const Tile *tl = &gs.tile[gs.tex_tile];
    int plane = fabsf(det) > 1e-3f && (!gs.tex_on || (tl->masks && !(tl->cms & 2)));
    for (int i = 0; i < n; i++) {
        float nx = s[i].x <= 1 ? -gfx_wide_off : s[i].x >= 318 ? 320 + gfx_wide_off : s[i].x;
        if (nx == s[i].x)
            continue;
        if (plane) {
            /* (nx, y) = a + p * (b - a) + q * (c - a) */
            float dx = nx - a[0], dy = s[i].y - a[1];
            float p = (dx * vy - dy * vx) / det, q = (ux * dy - uy * dx) / det;
            float *o = &s[i].x;
            for (int k = 2; k < 10; k++)
                if (k != 3)                     /* (w stays 1) */
                    o[k] = a[k] + p * (b[k] - a[k]) + q * (c[k] - a[k]);
        }
        s[i].x = nx;
        wide_sides_drawn = 1;
    }
}

/* ---- the HUD at the sides (--hud edges; docs/PORT.md, "Widescreen") -------------------
 *
 * In a wide frame the game's HUD would stay where it puts it, in the 4:3
 * middle, ever further from the edges as the frame widens.  With --hud
 * edges (the default) the elements of the levels' HUD move by off columns
 * towards the side they are on, so that they keep their distance from the
 * picture's edges: the radar and its arrow, the counters, the timer, the
 * money, the TV, the bonus amounts.  The rest stays as the game has it.
 *
 * What is HUD is told from what the game draws, not from where:
 *
 * - The mode (D_80364A90): a level being played, its intro (the traffic
 *   lights) and its end (LEVEL COMPLETE, MISSION COMPLETE) (HUD_MODES).
 *   The pause screen (0x100), the map, the front end, the results and the
 *   stories are never moved.
 * - The projection: the gameplay frame's 2D one (its buffer's mtx[3], at
 *   segment 2 + 0xC0), and its mtx[2] perspective, which only the radar's
 *   arrow uses.  The hint panels and the level's goal ("DESTROY BUILDINGS
 *   IN ...") are drawn by the panel code (26570.c) through mtx[73]: they
 *   stay centred.
 * - The vertices: the frame's own (segment 2: the green markers around
 *   targets, which are placed in the world) stay put; text (drawtext.c's
 *   vertex buffers, D_80365348) goes by its string; models (the radar,
 *   the TV, the icons) by the extent of the vertex load they come in.
 * - Texture rectangles go by the group of touching ones they belong to,
 *   found by a pass over the display list before it runs (the traffic
 *   lights' strip over the top is eight of them, and stays centred).
 *
 * An element then moves by -off if its middle is left of 112, +off right
 * of 208, and in between by the same ramp (a centred one doesn't move, and
 * one that slides across, LEVEL COMPLETE, doesn't jump).  None of it
 * reaches the game: the geometry is moved after the RDP's time was
 * charged from it, and RDRAM is untouched. */
#define HUD_MODES 0x000000100400220Cull     /* the levels' modes with the HUD */
#define HUD_RAMP 48.0f

extern char D_80364A90[], D_80365348[], D_80365350[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_80364A90 PORT_VAR(D_80364A90)
#define D_80365348 PORT_VAR(D_80365348)
#define D_80365350 PORT_VAR(D_80365350)
#endif

enum { HUD_NO, HUD_FLAT, HUD_PERSP };
static uint32_t hud_txt[2], hud_txt_n;          /* drawtext.c's vertex buffers, characters each */
static uint32_t hud_stamp;                      /* per task, for the memo below */
static struct { uint32_t stamp; float x0, x1; } hud_str[2][0x200];
#define HUD_RECTS 256
static struct { float x0, x1, y0, y1, gx0, gx1; int up; } hud_rect[HUD_RECTS];
static int hud_nrects, hud_rect_i;

static uint32_t hud_base(void) { return 0x80000000u | gs.seg[2]; }

static int hud_proj_kind(void) {
    uint32_t o = hud_proj - hud_base();
    return o == 0xC0 ? HUD_FLAT : o == 0x80 ? HUD_PERSP : HUD_NO;
}

/* how far an element from x0 to x1 moves */
static float hud_dx(float x0, float x1) {
    float c = (x0 + x1) * 0.5f, off = (float)gfx_wide_off;
    if (c <= 160 - HUD_RAMP)
        return -off;
    if (c >= 160 + HUD_RAMP)
        return off;
    return off * (c - 160) / HUD_RAMP;
}

static float hud_screen_x(float x, float y, float z) {
    float cx = x * gs.mvp[0][0] + y * gs.mvp[1][0] + z * gs.mvp[2][0] + gs.mvp[3][0];
    float cw = x * gs.mvp[0][3] + y * gs.mvp[1][3] + z * gs.mvp[2][3] + gs.mvp[3][3];
    return gs.vp_trans[0] + (cw > 1e-6f ? cx / cw : cx) * gs.vp_scale[0];
}

/* a vertex load under a HUD projection: its screen extent, for each vertex */
static void hud_load(int v0, int n) {
    if (hud_proj_kind() == HUD_NO || n <= 0)
        return;
    float x0 = 1e9f, x1 = -1e9f;
    for (int i = 0; i < n; i++) {
        const Vtx4 *v = &gs.v[v0 + i];
        if (v->w <= 1e-6f)
            continue;
        float x = gs.vp_trans[0] + v->x / v->w * gs.vp_scale[0];
        x0 = x < x0 ? x : x0;
        x1 = x > x1 ? x : x1;
    }
    for (int i = 0; i < n; i++) {
        hud_vx0[v0 + i] = x0;
        hud_vx1[v0 + i] = x1;
    }
}

/* drawtext.c's character k of buffer base: its quad's corner, width and height */
static void hud_char(uint32_t base, int k, int *x, int *y, int *w, int *h) {
    const uint8_t *p = port_ptr(base + 64 * (uint32_t)k);
    *x = (int16_t)port_g16(p);
    *y = (int16_t)port_g16(p + 2);
    *w = (int16_t)port_g16(p + 16) - *x;
    *h = (int16_t)port_g16(p + 32 + 2) - *y;
}

/* j and k are neighbours in one string: on one line, the same size, an
   advance or two apart (a space draws nothing; a right-aligned string is
   laid out backwards) */
static int hud_same_string(uint32_t base, int j, int k) {
    int xj, yj, wj, hj, xk, yk, wk, hk;
    hud_char(base, j, &xj, &yj, &wj, &hj);
    hud_char(base, k, &xk, &yk, &wk, &hk);
    int d = xj > xk ? xj - xk : xk - xj;
    return yj == yk && hj == hk && wj == wk && d > 0 && d <= 2 * (wj < 0 ? -wj : wj);
}

/* the screen extent of the string the character at address a is in; 0 if
   a isn't drawtext.c's */
static int hud_text(uint32_t a, float *x0, float *x1) {
    int b = a - hud_txt[0] < hud_txt_n * 64 ? 0 : a - hud_txt[1] < hud_txt_n * 64 ? 1 : -1;
    if (b < 0)
        return 0;
    int k = (int)((a - hud_txt[b]) / 64);
    if (k >= 0x200)
        return 0;
    if (hud_str[b][k].stamp != hud_stamp) {
        uint32_t base = hud_txt[b];
        int lo = k, hi = k;
        while (lo > 0 && hud_same_string(base, lo - 1, lo))
            lo--;
        while (hi + 1 < (int)hud_txt_n && hi + 1 < 0x200 && hud_same_string(base, hi, hi + 1))
            hi++;
        float m0 = 1e9f, m1 = -1e9f;
        int x, y = 0, w, h;
        for (int i = lo; i <= hi; i++) {
            hud_char(base, i, &x, &y, &w, &h);
            m0 = x < m0 ? x : m0;
            m0 = x + w < m0 ? x + w : m0;
            m1 = x > m1 ? x : m1;
            m1 = x + w > m1 ? x + w : m1;
        }
        float s0 = hud_screen_x(m0, (float)y, -10), s1 = hud_screen_x(m1, (float)y, -10);
        for (int i = lo; i <= hi; i++) {
            hud_str[b][i].stamp = hud_stamp;
            hud_str[b][i].x0 = s0 < s1 ? s0 : s1;
            hud_str[b][i].x1 = s0 < s1 ? s1 : s0;
        }
    }
    *x0 = hud_str[b][k].x0;
    *x1 = hud_str[b][k].x1;
    return 1;
}

/* a triangle of vertices i0, i1, i2: how far it moves */
static float hud_tri_dx(int i0, int i1, int i2) {
    if (hud_proj_kind() == HUD_NO)
        return 0;
    uint32_t a = hud_vsrc[i0 & 15];
    if (a - hud_base() < 0x21498)           /* the frame's own: placed in the world */
        return 0;
    float x0, x1;
    if (!hud_text(a, &x0, &x1)) {
        int v[3] = { i0 & 15, i1 & 15, i2 & 15 };
        x0 = 1e9f;
        x1 = -1e9f;
        for (int i = 0; i < 3; i++) {
            x0 = hud_vx0[v[i]] < x0 ? hud_vx0[v[i]] : x0;
            x1 = hud_vx1[v[i]] > x1 ? hud_vx1[v[i]] : x1;
        }
        if (x1 < x0)
            return 0;
    }
    return hud_dx(x0, x1);
}

static int hud_find(int i) {
    while (hud_rect[i].up != i)
        i = hud_rect[i].up = hud_rect[hud_rect[i].up].up;
    return i;
}

/* the pass before the display list runs: its texture rectangles under the
   2D projection, in order, as groups of touching ones (the segments and
   the projection followed as run() follows them) */
static void hud_scan(uint32_t dl, int depth, uint32_t *proj) {
    for (int n = 0; n < 1000000; n++, dl += 8) {
        const uint8_t *p = port_ptr(dl);
        uint32_t w0 = port_g32(p), w1 = port_g32(p + 4);
        switch (w0 >> 24) {
        case 0x01:                                              /* G_MTX */
            if ((w0 >> 16) & 1)
                *proj = seg_to_k0(w1);
            break;
        case 0x06:                                              /* G_DL */
            if (depth < 16) {
                hud_scan(seg_to_k0(w1), depth + 1, proj);
                if ((w0 >> 16) & 1)
                    return;
            }
            break;
        case 0xB8:                                              /* G_ENDDL */
            return;
        case 0xBC:                                              /* G_MOVEWORD: a segment */
            if ((w0 & 0xFF) == 0x06)
                gs.seg[(((w0 >> 8) & 0xFFFF) / 4) & 15] = w1 & 0x1FFFFFFFu;
            break;
        case 0xE4: case 0xE5:                                   /* texture rectangle */
            if (*proj - hud_base() == 0xC0 && hud_nrects < HUD_RECTS) {
                int k = hud_nrects++;
                hud_rect[k].x0 = ((w1 >> 12) & 0xFFF) / 4.0f;
                hud_rect[k].y0 = (w1 & 0xFFF) / 4.0f;
                hud_rect[k].x1 = ((w0 >> 12) & 0xFFF) / 4.0f + 1;
                hud_rect[k].y1 = (w0 & 0xFFF) / 4.0f + 1;
                hud_rect[k].up = k;
            }
            dl += 16;
            break;
        default:
            break;
        }
    }
}

static void hud_begin(uint32_t dl, int rdp) {
    uint64_t mode = (uint64_t)port_g32(D_80364A90) << 32 | port_g32(D_80364A90 + 4);
    hud_task = rdp && gfx_hud_edges && gfx_wide_off > 0 && (mode & HUD_MODES) && !(mode & (mode - 1));
    if (!hud_task)
        return;
    hud_stamp++;
    hud_txt[0] = port_g32(D_80365348);
    hud_txt[1] = port_g32(D_80365348 + 4);
    hud_txt_n = port_g32(D_80365350);
    if (hud_txt_n > 0x200)
        hud_txt_n = 0x200;
    hud_proj = 0;
    hud_nrects = 0;
    uint32_t seg[16], proj = 0;
    memcpy(seg, gs.seg, sizeof seg);
    hud_scan(dl, 0, &proj);
    memcpy(gs.seg, seg, sizeof seg);
    /* touching (to within a pixel) is one group */
    for (int i = 0; i < hud_nrects; i++)
        for (int j = 0; j < i; j++)
            if (hud_rect[i].x0 <= hud_rect[j].x1 + 1 && hud_rect[j].x0 <= hud_rect[i].x1 + 1 &&
                hud_rect[i].y0 <= hud_rect[j].y1 + 1 && hud_rect[j].y0 <= hud_rect[i].y1 + 1)
                hud_rect[hud_find(i)].up = hud_find(j);
    float gx0[HUD_RECTS], gx1[HUD_RECTS];
    for (int i = 0; i < hud_nrects; i++) {
        gx0[i] = 1e9f;
        gx1[i] = -1e9f;
    }
    for (int i = 0; i < hud_nrects; i++) {
        int r = hud_find(i);
        gx0[r] = hud_rect[i].x0 < gx0[r] ? hud_rect[i].x0 : gx0[r];
        gx1[r] = hud_rect[i].x1 > gx1[r] ? hud_rect[i].x1 : gx1[r];
    }
    for (int i = 0; i < hud_nrects; i++) {      /* each rectangle: its group's extent */
        int r = hud_find(i);
        hud_rect[i].gx0 = gx0[r];
        hud_rect[i].gx1 = gx1[r];
    }
}

/* a texture rectangle (in run()'s order): how far it moves */
static float hud_rect_dx(void) {
    if (hud_proj - hud_base() != 0xC0)
        return 0;
    int k = hud_rect_i++;
    return k < hud_nrects ? hud_dx(hud_rect[k].gx0, hud_rect[k].gx1) : 0;
}

/* A task whose microcode writes the RDP's commands to memory instead of
   handing them to the RDP (ultra.c, host_gfx_task's rdp): nothing is drawn.
   Nor are the commands written: the RSP the TAS was made with (mupen64plus's
   rsp-hle, which hands every graphics task to the graphics plugin) doesn't
   write them either, and the game's timing depends on what it finds there
   (docs/PORT.md, "Graphics"). */
static int rsp_only;

__attribute__((always_inline)) static inline void tri_draw(int i0, int i1, int i2, int flag);

static void line_draw(int i0, int i1, int wd, int flag);

/* (flag 0x80: a line, G_LINE3D's, i2 its width) */
NOINLINE static void tri(int i0, int i1, int i2, int flag) {
    if (recording)
        rec_tri(i0, i1, i2, flag);
    if (flag & 0x80)
        line_draw(i0, i1, i2, flag & 0x7F);
    else
        tri_draw(i0, i1, i2, flag);
    if (recording)
        rec_tri_done();
}

__attribute__((always_inline)) static inline void tri_draw(int i0, int i1, int i2, int flag) {
    st_tris++;
    Vtx4 *a = &gs.v[i0 & 15], *b = &gs.v[i1 & 15], *c = &gs.v[i2 & 15];
    float flat[4];
    const float *fl = NULL;
    if (!(gs.geom & 0x200) || !(gs.geom & 0x4)) {
        const Vtx4 *f = flag == 1 ? b : flag == 2 ? c : a;
        flat[0] = f->r; flat[1] = f->g; flat[2] = f->b; flat[3] = f->a;
        if (!(gs.geom & 0x4)) {
            flat[0] = flat[1] = flat[2] = flat[3] = 0;
            flat[3] = 255;
        }
        fl = flat;
    }
    /* culling, in normalized device coordinates (a triangle with a vertex
       behind the eye is culled below, once clipped) */
    int cull_late = 0;
    if (!(a->w > 0 && b->w > 0 && c->w > 0))
        cull_late = (gs.geom & 0x3000) != 0;
    else if (gs.geom & 0x3000) {
        const float *da = gs.vd[i0 & 15], *db = gs.vd[i1 & 15], *dc = gs.vd[i2 & 15];
        float ax = da[0], ay = da[1], bx = db[0], by = db[1];
        float cx = dc[0], cy = dc[1];
        float cross = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
        if ((gs.geom & 0x2000) && cross < 0)
            return;
        if ((gs.geom & 0x1000) && cross > 0)
            return;
    }
    if (rsp_only)
        return;
    GfxVtx s[9];
    int n;
    if (a->w - 1e-3f >= 0 && b->w - 1e-3f >= 0 && c->w - 1e-3f >= 0 &&
        a->z + a->w >= 0 && b->z + b->w >= 0 && c->z + c->w >= 0) {
        /* inside both planes: what clip_poly would give back */
        n = 3;
        to_screen_iw(a, gs.vd[i0 & 15][2], &s[0]);
        to_screen_iw(b, gs.vd[i1 & 15][2], &s[1]);
        to_screen_iw(c, gs.vd[i2 & 15][2], &s[2]);
    } else {
        Vtx4 p0[3] = { *a, *b, *c }, p1[9], p2[9];
        n = clip_poly(p0, 3, p1, 0);
        n = clip_poly(p1, n, p2, 1);
        if (n < 3)
            return;
        /* the clipped polygon is all in front of the eye, where its
           winding on the screen is the triangle's facing, as the RSP
           culls what its clipper hands on: without it, the back faces of
           walls and girders that reach behind the camera were drawn as
           sheets over the screen, with a straight edge */
        if (cull_late) {
            float area = 0;
            for (int i = 0; i < n; i++) {
                const Vtx4 *p = &p2[i], *q = &p2[i + 1 < n ? i + 1 : 0];
                area += p->x / p->w * (q->y / q->w) - q->x / q->w * (p->y / p->w);
            }
            if ((gs.geom & 0x2000) && area < 0)
                return;
            if ((gs.geom & 0x1000) && area > 0)
                return;
        }
        for (int i = 0; i < n; i++)
            to_screen(&p2[i], &s[i]);
    }
    /* (an in-between pass's isn't charged; nor anything when the RDP's
       time is taken as nothing, PORT_RDP_SCALE's default, but for -v) */
    if (!ipass && charge_tris)
        charge_poly(s, n);
    int gl = gl_target();
    if (hud_task && (gl || cur_wfb)) {
        float dx = hud_tri_dx(i0, i1, i2);
        for (int i = 0; dx != 0 && i < n; i++)
            s[i].x += dx;
    }
    if (gfx_wide_off && (gl || cur_wfb))
        wide_2d(s, n);
    if (gl) {
        gfx_gl_tri(s, n, fl);
        if (recording)
            rec_drawn(GFX_GL_TRI, 0);
        return;
    }
    if (ipass && !cur_wfb)              /* RDRAM is the first pass's; a twin only */
        return;
    SV sv[9];
    for (int i = 0; i < n; i++)
        to_sv(&s[i], &sv[i]);
    for (int i = 1; i + 1 < n; i++)
        raster(&sv[0], &sv[i], &sv[i + 1], fl);
}

/* G_LINE3D (the front end's line microcode: the routes on the world map),
   as the RDP gets it: a quad 1.5 + wd / 2 pixels wide along the projected
   segment, clipped first, shaded and z-tested as a triangle would be */
static void line_draw(int i0, int i1, int wd, int flag) {
    st_tris++;
    if (rsp_only)
        return;
    Vtx4 p[2] = { gs.v[i0 & 15], gs.v[i1 & 15] };
    float flat[4];
    const float *fl = NULL;
    if (!(gs.geom & 0x200) || !(gs.geom & 0x4)) {
        const Vtx4 *f = flag == 1 ? &p[1] : &p[0];
        flat[0] = f->r; flat[1] = f->g; flat[2] = f->b; flat[3] = f->a;
        if (!(gs.geom & 0x4)) {
            flat[0] = flat[1] = flat[2] = 0;
            flat[3] = 255;
        }
        fl = flat;
    }
    for (int plane = 0; plane < 2; plane++) {
        float d0 = plane == 0 ? p[0].w - 1e-3f : p[0].z + p[0].w;
        float d1 = plane == 0 ? p[1].w - 1e-3f : p[1].z + p[1].w;
        if (d0 < 0 && d1 < 0)
            return;
        if (d0 < 0)
            lerp(&p[0], &p[0], &p[1], d0 / (d0 - d1));
        else if (d1 < 0)
            lerp(&p[1], &p[1], &p[0], d1 / (d1 - d0));
    }
    GfxVtx e[2], s[4];
    to_screen(&p[0], &e[0]);
    to_screen(&p[1], &e[1]);
    float dx = e[1].x - e[0].x, dy = e[1].y - e[0].y, len = sqrtf(dx * dx + dy * dy);
    float hw = (1.5f + wd * 0.5f) * 0.5f;
    float nx, ny;
    if (len < 1e-4f) {
        nx = hw;
        ny = 0;
    } else {
        nx = -dy / len * hw;
        ny = dx / len * hw;
    }
    s[0] = e[0]; s[0].x += nx; s[0].y += ny;
    s[1] = e[1]; s[1].x += nx; s[1].y += ny;
    s[2] = e[1]; s[2].x -= nx; s[2].y -= ny;
    s[3] = e[0]; s[3].x -= nx; s[3].y -= ny;
    if (!ipass && charge_tris)
        charge_poly(s, 4);
    int gl = gl_target();
    if (gfx_wide_off && (gl || cur_wfb))
        wide_2d(s, 4);
    if (gl) {
        gfx_gl_tri(s, 4, fl);
        if (recording)
            rec_drawn(GFX_GL_TRI, 0);
        return;
    }
    if (ipass && !cur_wfb)
        return;
    SV sv[4];
    for (int i = 0; i < 4; i++)
        to_sv(&s[i], &sv[i]);
    raster(&sv[0], &sv[1], &sv[2], fl);
    raster(&sv[0], &sv[2], &sv[3], fl);
}

/* ---- rectangles ------------------------------------------------------------------------ */

/* --interpolate: a rectangle (a texture or fill rectangle: text, the HUD,
   the menus) is known by what it draws (the key: its texture and texture
   coordinates, or its colors, and its size) and how many before it in the
   frame were the same, as the vertex loads are by their addresses.  The
   in-between passes move its corners (ulx, uly, lrx, lry) between the
   previous frame's and this one's, unless one moved by more than 64
   pixels (another piece, or a cut). */
#define IRECT_KEY(tag, h) ((uint32_t)(tag) << 28 | ((h) & 0x0FFFFFFFu))
static unsigned long long gfx_st_rect[3];             /* rectangles, moved, moved too far */

static uint32_t rect_hash(const uint32_t *w, int n) {
    uint32_t h = 0x811C9DC5u;
    for (int i = 0; i < n; i++)
        h = (h ^ w[i]) * 0x01000193u, h ^= h >> 13;
    return h;
}

static void irect(uint32_t key, float *r) {
    if (!ipass) {
        const float pt[1][4] = { { r[0], r[1], r[2], r[3] } };
        irecord_at(key, pt, 1);
        return;
    }
    const ILoad *p = iprev(1);
    gfx_st_rect[0]++;
    if (!p)
        return;
    const float *q = p->p[0];
    int moved = 0;
    for (int i = 0; i < 4; i++) {
        if (fabsf(r[i] - q[i]) > 64) {
            gfx_st_rect[2]++;
            return;
        }
        moved |= r[i] != q[i];
    }
    gfx_st_rect[1] += moved;
    for (int i = 0; i < 4; i++)
        r[i] = q[i] + (r[i] - q[i]) * interp_t;
}

/* a fill rectangle drawn in software, into the color image in RDRAM (or
   the software renderer's wide frame) */
NOINLINE static void fill_sw(int ulx, int uly, int lrx, int lry, int cyc) {
    uint8_t *p = port_ptr(gs.cimg_addr);
    if (cyc == 3 && gs.cimg_siz == 2 && !cur_wfb && lrx > ulx) {
        /* 16-bit: the fill's two pixels, over and over; a row of them
           made once, as the memory holds them, and copied */
        static uint8_t row[2 * 4096 + 4];
        int n = lrx - ulx + (ulx & 1);
        if (n > 4096)
            n = 4096;
        for (int i = 0; i < n; i += 2) {
            port_wbe16(row + 2 * i, gs.fill >> 16);
            port_wbe16(row + 2 * i + 2, gs.fill & 0xFFFF);
        }
        for (int y = uly; y < lry; y++)
            memcpy(p + 2 * (y * gs.cimg_w + ulx), row + 2 * (ulx & 1), 2 * (size_t)(lrx - ulx));
        return;
    }
    for (int y = uly; y < lry; y++)
        for (int x = ulx; x < lrx; x++) {
            if (cyc == 3) {
                if (gs.cimg_siz == 1)
                    p[y * gs.cimg_w + x] = gs.fill >> (24 - 8 * (x & 3));
                else if (gs.cimg_siz == 3)
                    port_wbe32(p + 4 * (y * gs.cimg_w + x), gs.fill);
                else if (cur_wfb)
                    cur_wfb->px[y * cur_wfb->w + x + gfx_wide_off] = (x & 1) ? gs.fill & 0xFFFF : gs.fill >> 16;
                else
                    port_wbe16(p + 2 * (y * gs.cimg_w + x), (x & 1) ? gs.fill & 0xFFFF : gs.fill >> 16);
            } else {
                Inputs in;
                memset(&in, 0, sizeof in);
                in.lod = 255;
                float c[4];
                combine(&in, c);
                if (blend(x, y, c, in.shade))
                    write_pixel(x, y, c);
            }
        }
}

NOINLINE static void fill_rect(uint32_t w0, uint32_t w1) {
    if (recording)
        rec_rect(R_FILL, w0, w1, 0, 0, 0);
    int lrx = ((w0 >> 12) & 0xFFF) >> 2, lry = (w0 & 0xFFF) >> 2;
    int ulx = ((w1 >> 12) & 0xFFF) >> 2, uly = (w1 & 0xFFF) >> 2;
    int cyc = gfx_cycle_type();
    if (cyc == 3 || cyc == 2) { lrx++; lry++; }
    if (itrack && gs.cimg_addr != gs.zimg_addr) {
        uint32_t k[] = { (uint32_t)cyc, gs.fill, port_be32(gs.prim), port_be32(gs.env), gs.cc0, gs.cc1, gs.om_l };
        float r[4] = { (float)ulx, (float)uly, (float)lrx, (float)lry };
        irect(IRECT_KEY(3, rect_hash(k, 7)), r);
        ulx = (int)lroundf(r[0]); uly = (int)lroundf(r[1]); lrx = (int)lroundf(r[2]); lry = (int)lroundf(r[3]);
    }
    if (ulx < gs.sc_x0) ulx = gs.sc_x0;
    if (uly < gs.sc_y0) uly = gs.sc_y0;
    if (lrx > gs.sc_x1) lrx = gs.sc_x1;
    if (lry > gs.sc_y1) lry = gs.sc_y1;
    if (lrx <= ulx || lry <= uly)
        return;
    st_cover += (double)(lrx - ulx) * (lry - uly) * (cyc == 3 ? 0.25 : 1);
    int wx0 = ulx, wx1 = lrx;                       /* widescreen: a full-width fill covers the wide frame */
    if (wide_cimg())
        gfx_wide_span(ulx, lrx, &wx0, &wx1);
    if (wx0 < ulx && gs.cimg_addr != gs.zimg_addr)
        wide_sides_drawn = 1;
    if (ipass && gfx_gl_enabled) {      /* the twins' only; RDRAM is the first pass's */
        if (gs.cimg_addr == gs.zimg_addr && zbuf_ok())
            gfx_gl_zclear(wx0, uly, wx1, lry);
        else if (gl_target())
            gfx_gl_fill_rect(wx0, uly, wx1, lry);
        return;
    }
    if (ipass && gs.cimg_addr == gs.zimg_addr && zbuf_ok()) {  /* the software renderer's twin's z */
        for (int y = uly; y < lry; y++)
            for (int x = wx0; x < wx1; x++)
                zb[y * zb_w + x + zb_off] = 1.0f;
        return;
    }
    if (ipass && !cur_wfb)
        return;
    if (ipass) {
        /* a twin: drawn below */
    } else if (gs.cimg_addr == gs.zimg_addr && zbuf_ok()) {  /* clearing the z-buffer */
        for (int y = uly; y < lry; y++)
            for (int x = wx0; x < wx1; x++)
                zb[y * zb_w + x + zb_off] = 1.0f;
        if (gfx_gl_enabled)
            gfx_gl_zclear(wx0, uly, wx1, lry);
    } else if (gl_target()) {
        gfx_gl_fill_rect(wx0, uly, wx1, lry);
        if (recording)
            rec_drawn(GFX_GL_FILL, 0);
        return;
    }
    if (cur_wfb) {
        ulx = wx0;
        lrx = wx1;
    }
    fill_sw(ulx, uly, lrx, lry, cyc);
}

/* black into the wide frame's side columns x0..x1 (x0 < 0 or x1 > 320) */
static void wide_band_clear(int x0, int y0, int x1, int y1, int gl) {
    if (gl) {
        gfx_gl_clear_rect(x0, y0, x1, y1);
        return;
    }
    for (int y = y0; y < y1; y++)
        memset(cur_wfb->px + y * cur_wfb->w + x0 + gfx_wide_off, 0, (size_t)(x1 - x0) * 2);
}

NOINLINE static void tex_rect(uint32_t w0, uint32_t w1, uint32_t h2, uint32_t hc, int flip) {
    if (recording)
        rec_rect(R_TEXRECT, w0, w1, h2, hc, flip);
    float lrx = ((w0 >> 12) & 0xFFF) / 4.0f, lry = (w0 & 0xFFF) / 4.0f;
    float ulx = ((w1 >> 12) & 0xFFF) / 4.0f, uly = (w1 & 0xFFF) / 4.0f;
    int tile = (w1 >> 24) & 7;
    float s0 = (int16_t)(h2 >> 16) / 32.0f, t0 = (int16_t)(h2 & 0xFFFF) / 32.0f;
    float dsdx = (int16_t)(hc >> 16) / 1024.0f, dtdy = (int16_t)(hc & 0xFFFF) / 1024.0f;
    float hdx = hud_task ? hud_rect_dx() : 0;  /* (every rectangle counts, as hud_scan's) */
    int cyc = gfx_cycle_type();
    if (cyc == 2) {                     /* copy mode: 4 texels per step, inclusive */
        dsdx /= 4;
        lrx += 1;
        lry += 1;
    }
    if (itrack) {
        const Tile *tl = &gs.tile[tile];
        uint32_t k[] = { gs.timg_addr, (uint32_t)(tl->tmem | tl->fmt << 9 | tl->siz << 12 | tl->pal << 14 | flip << 18),
                         h2, hc, ((w0 >> 12) & 0xFFF) - ((w1 >> 12) & 0xFFF), (w0 & 0xFFF) - (w1 & 0xFFF) };
        float r[4] = { ulx, uly, lrx, lry };
        irect(IRECT_KEY(1, rect_hash(k, 6)), r);
        ulx = r[0]; uly = r[1]; lrx = r[2]; lry = r[3];
    }
    int x0 = (int)ulx, y0 = (int)uly, x1 = (int)lrx, y1 = (int)lry;
    if (x0 < gs.sc_x0) x0 = gs.sc_x0;
    if (y0 < gs.sc_y0) y0 = gs.sc_y0;
    if (x1 > gs.sc_x1) x1 = gs.sc_x1;
    if (y1 > gs.sc_y1) y1 = gs.sc_y1;
    if (x1 <= x0 || y1 <= y0)
        return;
    st_cover += (double)(x1 - x0) * (y1 - y0) * (cyc == 2 ? 0.25 : 1);
    int gl = gl_target();
    if (hdx != 0 && (gl || cur_wfb)) {
        /* the HUD at the sides: moved, and clipped by the wide frame's
           scissor (a full-width one covers it) */
        int sx0, sx1;
        gfx_wide_span(gs.sc_x0, gs.sc_x1, &sx0, &sx1);
        ulx += hdx;
        lrx += hdx;
        x0 = (int)floorf(ulx);
        x1 = (int)floorf(lrx);
        y0 = (int)uly;
        y1 = (int)lry;
        if (x0 < sx0) x0 = sx0;
        if (y0 < gs.sc_y0) y0 = gs.sc_y0;
        if (x1 > sx1) x1 = sx1;
        if (y1 > gs.sc_y1) y1 = gs.sc_y1;
        if (x1 <= x0 || y1 <= y0)
            return;
    } else if (gfx_wide_off && !wide_sides_drawn && (gl || cur_wfb)) {
        /* widescreen: a rectangle at an edge of the game's frame (the tiles
           of a full-screen picture) blacks out the side beyond it, so a
           2D screen is pillarboxed rather than framed by stale pixels; not
           once the frame has drawn its sides itself (the world map's
           background fill), where it would be a black box around the
           vehicles sliding in and out at the edges */
        if (x0 <= 0)
            wide_band_clear(-gfx_wide_off, y0, 0, y1, gl);
        if (x1 >= 320)
            wide_band_clear(320, y0, 320 + gfx_wide_off, y1, gl);
    }
    if (gl) {
        float sa = s0 + ((flip ? y0 - uly : x0 - ulx)) * dsdx;
        float ta = t0 + ((flip ? x0 - ulx : y0 - uly)) * dtdy;
        gfx_gl_tex_rect(x0, y0, x1, y1, tile, sa, ta, dsdx, dtdy, flip);
        if (recording)
            rec_drawn(GFX_GL_TEXRECT, tile);
        return;
    }
    if (ipass && !cur_wfb)
        return;
    if (tpend_n)
        gfx_tmem_sync();
    int filt = gfx_filter_mode();
    int ta = tile, tb = tile + 1;
    float lodfrac = 255;
    if (gfx_lod_on() && cyc < 2) {
        float frac;
        gfx_lod_tiles(max_f(fabsf(dsdx), fabsf(dtdy)), tile, &ta, &tb, &frac);
        lodfrac = frac * 255;
    }
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            float fx = x - ulx, fy = y - uly;
            float s = s0 + (flip ? fy : fx) * dsdx, t = t0 + (flip ? fx : fy) * dtdy;
            Inputs in;
            memset(&in, 0, sizeof in);
            in.lod = lodfrac;
            sample(ta, s, t, in.t0, filt);
            if (cyc < 2)
                sample(tb, s, t, in.t1, filt);
            float c[4];
            if (cyc == 2) {
                c[0] = in.t0[0]; c[1] = in.t0[1]; c[2] = in.t0[2]; c[3] = in.t0[3];
                if ((gs.om_l & 3) == 1 && c[3] < 128)
                    continue;
            } else {
                combine(&in, c);
                if (!blend(x, y, c, in.shade))
                    continue;
            }
            write_pixel(x, y, c);
        }
}

/* an in-between pass from the first pass's record (above, "replaying the
   first pass"), from the state the first started from */
static void replay_snap(int i) {
    memcpy((uint8_t *)&gs + SNAP_OFF, rsnap + (size_t)i * SNAP_SZ, SNAP_SZ);
    gfx_state_serial++;                 /* (a state made again, from gs) */
}

static void replay(void) {
    const uint32_t *e = rec, *end = rec + rec_n;
    int tri_state = -1;
    wide_sides_drawn = 0;
    while (e < end) {
        switch (e[0]) {
        case R_CTX: {
            const RCtx *c = &rctx[e[1]];
            gs.geom = c->geom; gs.om_h = c->om_h; gs.om_l = c->om_l; gs.cc0 = c->cc0; gs.cc1 = c->cc1;
            gs.fill = c->fill;
            memcpy(gs.prim, c->prim, 4);
            memcpy(gs.env, c->env, 4);
            memcpy(gs.vp_scale, c->vp_scale, sizeof gs.vp_scale);
            memcpy(gs.vp_trans, c->vp_trans, sizeof gs.vp_trans);
            gs.sc_x0 = c->sc[0]; gs.sc_y0 = c->sc[1]; gs.sc_x1 = c->sc[2]; gs.sc_y1 = c->sc[3];
            gs.tex_on = c->tex_on; gs.tex_tile = c->tex_tile;
            gs.tile[c->tex_tile & 7].masks = c->masks; gs.tile[c->tex_tile & 7].cms = c->cms;
            gs.timg_addr = c->timg_addr; gs.cimg_addr = c->cimg_addr; gs.zimg_addr = c->zimg_addr;
            gs.seg[2] = c->seg2;
            gs.cimg_siz = c->cimg_siz; gs.cimg_w = c->cimg_w;
            tri_state = c->tri_state;
            if (tri_state < 0 && c->snap >= 0)
                replay_snap(c->snap);
            else if (tri_state < 0)
                tri_state = -2;         /* (nothing the GPU draws: none needed) */
            gfx_gl_rec_force(tri_state);
            e += 2;
            break;
        }
        case R_LOAD: {
            int v0 = e[2] & 0xFF, m = (e[2] >> 8) & 0xFF;
            memcpy(gs.v + v0, rv + e[3], (size_t)m * sizeof *rv);
            if (e[4] != 0xFFFFFFFFu)
                memcpy(gs.mvp, rmvp[e[4]], sizeof gs.mvp);
            vtx_tail(0, e[1], v0, m);
            e += 5;
            break;
        }
        case R_MODV:
            modify_vertex((int)e[1], e[2]);
            e += 3;
            break;
        case R_TRI:
            tri(e[1] & 0xFF, (e[1] >> 8) & 0xFF, (e[1] >> 16) & 0xFF, (int)(e[1] >> 24));
            e += 2;
            break;
        case R_FILL:
        case R_TEXRECT:
            if (e[6] != 0xFFFFFFFFu) {
                gfx_gl_rec_force((int)e[6]);
            } else if (e[7] != 0xFFFFFFFFu) {
                replay_snap((int)e[7]);
                gfx_gl_rec_force(-1);
            } else {
                gfx_gl_rec_force(-2);
            }
            if (e[0] == R_FILL)
                fill_rect(e[1], e[2]);
            else
                tex_rect(e[1], e[2], e[3], e[4], (int)e[5]);
            gfx_gl_rec_force(tri_state);
            e += 8;
            break;
        case R_TLOAD: {
            TLoad l = { (uint8_t)e[1], e[2], e[3], e[4], (int)e[5], (int)e[6], (int)e[7], (int)e[8], { 0 } };
            tload_lazy(l);
            e += 9;
            break;
        }
        case R_PROJ:
            hud_proj = e[1];
            e += 2;
            break;
        default:
            e = end;
            break;
        }
    }
    gfx_gl_rec_force(-1);
}

/* ---- the display list ------------------------------------------------------------------- */

CALL_VIA(do_mtx) CALL_VIA(do_vtx) CALL_VIA(modify_vertex) CALL_VIA(load_tlut) CALL_VIA(load_block)
CALL_VIA(load_tile) CALL_VIA(fill_rect) CALL_VIA(tex_rect) CALL_VIA(tri)

static void run(uint32_t dl, int depth) {
    for (int n = 0; n < 1000000; n++, dl += 8) {
        const uint8_t *p = port_ptr(dl);
        uint32_t w0 = port_g32(p), w1 = port_g32(p + 4);
        uint8_t op = w0 >> 24;
        if (!ipass)
            host_gfx_stats[op]++;
        /* (what a draw's state can't depend on leaves it as it was) */
        if (!(op == 0x04 || op == 0xBF || op == 0x01 || op == 0xBD || op == 0x06 || op == 0xB8 ||
              (op >= 0xE6 && op <= 0xE8) || (op >= 0xB2 && op <= 0xB4) || op == 0xBE || op == 0xC0))
            gfx_state_serial++;
        if (rsp_only && op >= 0xE4) {                           /* the RDP's: written, not run */
            if (op == 0xE4 || op == 0xE5)
                dl += 16;                                       /* and its two halves */
            continue;
        }
        switch (op) {
        case 0x01: CALL(do_mtx)(w0, w1); break;                       /* G_MTX */
        case 0x03: {                                            /* G_MOVEMEM */
            int idx = (w0 >> 16) & 0xFF;
            const uint8_t *m = port_ptr(seg_to_k0(w1));
            if (idx == 0x80) {                                  /* viewport */
                for (int i = 0; i < 3; i++) {
                    gs.vp_scale[i] = (int16_t)port_g16(m + 2 * i) / 4.0f;
                    gs.vp_trans[i] = (int16_t)port_g16(m + 8 + 2 * i) / 4.0f;
                }
            } else if (idx == 0x82 || idx == 0x84) {            /* look-at y, x */
                for (int j = 0; j < 3; j++)
                    gs.lookat[idx == 0x84 ? 0 : 1][j] = (int8_t)m[8 + j] / 127.0f;
            } else if (idx >= 0x86 && idx <= 0x94) {            /* light */
                int l = (idx - 0x86) / 2;
                memcpy(gs.lcol[l], m, 3);
                for (int j = 0; j < 3; j++)
                    gs.ldir[l][j] = (int8_t)m[8 + j] / 127.0f;
            }
            break;
        }
        case 0x04: CALL(do_vtx)(w0, w1); break;                       /* G_VTX */
        case 0x06:                                              /* G_DL */
            if (depth < 16) {
                if ((w0 >> 16) & 1) {
                    run(seg_to_k0(w1), depth + 1);
                    return;
                }
                run(seg_to_k0(w1), depth + 1);
            }
            break;
        case 0xB2: case 0xB3: case 0xB4: case 0xC0: break;      /* halves, nop */
        case 0xB6: gs.geom &= ~w1; break;                       /* G_CLEARGEOMETRYMODE */
        case 0xB7: gs.geom |= w1; break;                        /* G_SETGEOMETRYMODE */
        case 0xB8: return;                                      /* G_ENDDL */
        case 0xB9: case 0xBA: {                                 /* G_SETOTHERMODE_L/H */
            int sft = (w0 >> 8) & 0xFF, len = w0 & 0xFF;
            uint32_t mask = (len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1)) << sft;
            uint32_t *om = op == 0xB9 ? &gs.om_l : &gs.om_h;
            *om = (*om & ~mask) | (w1 & mask);
            break;
        }
        case 0xBB:                                              /* G_TEXTURE */
            gs.tex_s = (w1 >> 16) / 65536.0f;
            gs.tex_t = (w1 & 0xFFFF) / 65536.0f;
            gs.tex_levels = (w0 >> 11) & 7;
            gs.tex_tile = (w0 >> 8) & 7;
            gs.tex_on = w0 & 0xFF;
            break;
        case 0xBC: {                                            /* G_MOVEWORD */
            int idx = w0 & 0xFF, off = (w0 >> 8) & 0xFFFF;
            if (idx == 0x06)
                gs.seg[(off / 4) & 15] = w1 & 0x1FFFFFFFu;  /* see seg_to_k0 */
            else if (idx == 0x02)
                gs.nlights = (int)((w1 - 0x80000000u) / 32) - 1;
            else if (idx == 0x08) {                             /* G_MW_FOG */
                gs.fog_mul = (int16_t)(w1 >> 16);
                gs.fog_off = (int16_t)(w1 & 0xFFFF);
            } else if (idx == 0x0C)                             /* G_MW_POINTS */
                CALL(modify_vertex)(off, w1);
            if (gs.nlights < 0) gs.nlights = 0;
            if (gs.nlights > 7) gs.nlights = 7;
            break;
        }
        case 0xBD:                                              /* G_POPMTX */
            if (gs.mv_top > 0)
                gs.mv_top--;
            gs.mvp_dirty = 1;
            break;
        case 0xBE: break;                                       /* G_CULLDL */
        case 0xB5:                                              /* G_LINE3D */
            CALL(tri)(((w1 >> 16) & 0xFF) / 10, ((w1 >> 8) & 0xFF) / 10, w1 & 0xFF, 0x80 | (w1 >> 24 & 0x7F));
            break;
        case 0xBF:                                              /* G_TRI1 */
            CALL(tri)(((w1 >> 16) & 0xFF) / 10, ((w1 >> 8) & 0xFF) / 10, (w1 & 0xFF) / 10, w1 >> 24);
            break;
        case 0xE4: case 0xE5: {                                 /* texture rectangle */
            const uint8_t *q = port_ptr(dl + 8);
            uint32_t h2 = port_g32(q + 4), hc = port_g32(q + 12);
            CALL(tex_rect)(w0, w1, h2, hc, op == 0xE5);
            dl += 16;
            break;
        }
        case 0xE6: case 0xE7: case 0xE8: break;                 /* syncs */
        case 0xE9: gs.sync = 1; break;                          /* G_RDPFULLSYNC */
        case 0xED:                                              /* G_SETSCISSOR */
            gs.sc_x0 = ((w0 >> 12) & 0xFFF) >> 2; gs.sc_y0 = (w0 & 0xFFF) >> 2;
            gs.sc_x1 = ((w1 >> 12) & 0xFFF) >> 2; gs.sc_y1 = (w1 & 0xFFF) >> 2;
            break;
        case 0xEF: gs.om_h = w0 & 0xFFFFFF; gs.om_l = w1; break; /* G_RDPSETOTHERMODE */
        case 0xF0: CALL(load_tlut)(w0, w1); break;
        case 0xF2: {                                            /* G_SETTILESIZE */
            Tile *t = &gs.tile[(w1 >> 24) & 7];
            t->uls = (w0 >> 12) & 0xFFF; t->ult = w0 & 0xFFF;
            t->lrs = (w1 >> 12) & 0xFFF; t->lrt = w1 & 0xFFF;
            gs.tile_gen++;
            break;
        }
        case 0xF3: CALL(load_block)(w0, w1); break;
        case 0xF4: CALL(load_tile)(w0, w1); break;
        case 0xF5: {                                            /* G_SETTILE */
            Tile *t = &gs.tile[(w1 >> 24) & 7];
            t->fmt = (w0 >> 21) & 7; t->siz = (w0 >> 19) & 3;
            t->line = (w0 >> 9) & 0x1FF; t->tmem = w0 & 0x1FF;
            t->pal = (w1 >> 20) & 15;
            t->cmt = (w1 >> 18) & 3; t->maskt = (w1 >> 14) & 15; t->shiftt = (w1 >> 10) & 15;
            t->cms = (w1 >> 8) & 3; t->masks = (w1 >> 4) & 15; t->shifts = w1 & 15;
            gs.tile_gen++;
            break;
        }
        case 0xF6: CALL(fill_rect)(w0, w1); break;
        case 0xF7: gs.fill = w1; break;
        case 0xF8: port_wbe32(gs.fog, w1); break;
        case 0xF9: port_wbe32(gs.blend, w1); break;
        case 0xFA: port_wbe32(gs.prim, w1); gs.prim_lod = w0 & 0xFF; break;
        case 0xFB: port_wbe32(gs.env, w1); break;
        case 0xFC: gs.cc0 = w0 & 0xFFFFFF; gs.cc1 = w1; break;
        case 0xFD:                                              /* G_SETTIMG */
            gs.timg_fmt = (w0 >> 21) & 7; gs.timg_siz = (w0 >> 19) & 3;
            gs.timg_w = (w0 & 0xFFF) + 1;
            gs.timg_addr = seg_to_k0(w1);
            if (ipass)                          /* (the first pass's did) */
                break;
            if (gfx_gl_enabled)
                gfx_gl_texture_source(gs.timg_addr);
            else if (nwfb)
                wfb_texture_source(gs.timg_addr);
            break;
        case 0xFE: gs.zimg_addr = seg_to_k0(w1); sw_target(); break;
        case 0xFF:                                              /* G_SETCIMG */
            gs.cimg_siz = (w0 >> 19) & 3;
            gs.cimg_w = (w0 & 0xFFF) + 1;
            gs.cimg_addr = seg_to_k0(w1);
            wide_sides_drawn = 0;
            sw_target();
            if (getenv("PORT_GFXLOG"))
                host_log("cimg %08X w %d siz %d (w1 %08X)\n", gs.cimg_addr, gs.cimg_w, gs.cimg_siz, w1);
            break;
        default:
            break;
        }
    }
    host_log("gfx: display list at %08X doesn't end\n", dl);
}

static int gfx_tasks;
static double gfx_host_ms;      /* host time spent running display lists */

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

/* --interpolate: the twins for the frame whose first task this is, from
   the holds of the last two frames (the shorter: a frame held longer than
   its twins were made for shows the frame for the rest, which looks better
   than one held shorter, which never gets to it) */
static int twins_used;          /* the last frame's choice */

static int twins_max(void) {
    static int max = -1;
    if (max < 0) {
        const char *e = getenv("PORT_INTERP_MAX");
        /* the software renderer draws a frame again for each */
        max = e ? atoi(e) : gfx_gl_enabled ? GFX_TWINS : 3;
        max = max < 1 ? 1 : max > GFX_TWINS ? GFX_TWINS : max;
    }
    return max;
}

static void choose_twins(void) {
    int d = hold_last[0] < hold_last[1] ? hold_last[0] : hold_last[1];
    d = d < 2 ? 2 : d > 4 ? 4 : d;
    int hz = gfx_interp_hz > PORT_RETRACE_HZ ? gfx_interp_hz : PORT_RETRACE_HZ;
    int k = (int)lround((double)d * hz / PORT_RETRACE_HZ) - 1;
    frame_d = d;
    frame_k = k < 1 ? 1 : k > twins_max() ? twins_max() : k;
    if (frame_k > host_interp_limit)        /* the host can't keep up (main.c) */
        frame_k = host_interp_limit;
    twins_used = frame_k;
}

int gfx_interp_twins_used(void) { return twins_used; }

static int gfx_task(uint32_t dl, uint32_t size, uint32_t ucode, int rdp) {
    (void)ucode;
    gfx_tasks++;
    rsp_only = !rdp;
    charge_tris = host_rdp_scaled() || host_verbose;
    memset(gs.seg, 0, sizeof gs.seg);
    gs.sync = 0;
    gs.mv_top = 0;
    gs.mvp_dirty = 1;
    if (!gs.sc_x1) {
        gs.sc_x1 = 320;
        gs.sc_y1 = 240;
    }
    double t0 = now_ms();
    sw_target();
    /* --interpolate: record the vertex loads always (the next frame may want
       them), run the in-between passes where the game holds frames */
    int between = 0;
    static GfxState s0;
    static uint8_t tmem0[4096];
    uint32_t noise0 = cc_noise;
    itrack = gfx_interp && rdp;     /* (an RSP-only task draws nothing to interpolate) */
    if (itrack) {
        if (!ifr[0]) {
            ifr[0] = calloc(1, sizeof *ifr[0]);
            ifr[1] = calloc(1, sizeof *ifr[1]);
        }
        between = host_frame_held() && host_interp_limit > 0;
        if (!between)
            iframe_partial = 1;
        else if (frame_k < 0)
            choose_twins();
        tk_n = 0;
        if (between) {
            s0 = gs;
            memcpy(tmem0, gfx_tmem, sizeof tmem0);
            static int replay_on = -1;
            if (replay_on < 0) {
                const char *e = getenv("PORT_INTERP_REPLAY");
                replay_on = !e || atoi(e) != 0;
            }
            if (gfx_gl_enabled && replay_on) {
                recording = 1;
                rec_reset();
            }
        }
    }
    host_perf_push(PERF_GFX);
    if (gfx_gl_enabled)
        gfx_gl_task_begin();
    double cover = st_cover;
    hud_begin(dl, rdp);
    hud_rect_i = 0;
    run(dl, 0);
    int replayable = recording && rec_n && rec_gen == gfx_gl_gen;
    recording = 0;
    if (gfx_gl_enabled)
        gfx_gl_task_end();
    host_perf_pop();
    double covered = st_cover - cover;
    if (between) {
        static GfxState s1;
        static uint8_t tmem1[4096];
        unsigned long long st[4] = { st_tris, st_raster, st_tested, st_drawn };
        uint32_t noise1 = cc_noise;
        s1 = gs;
        memcpy(tmem1, gfx_tmem, sizeof tmem1);
        host_perf_push(PERF_GFX2);
        tmem_lazy = gfx_gl_enabled;
        for (int k = 0; k < frame_k; k++) {
            gs = s0;
            memcpy(gfx_tmem, tmem0, sizeof tmem0);
            tpend_n = 0;
            cc_noise = noise0;
            ipass = 1;
            ik = k;
            interp_t = (float)(k + 1) / (frame_k + 1);
            tk_i = 0;
            hud_rect_i = 0;
            sw_target();
            if (gfx_gl_enabled) {
                gfx_gl_interp(k);
                gfx_gl_task_begin();
            }
            if (replayable) {
                replay();
                st_replay[0]++;
            } else {
                run(dl, 0);
                st_replay[1]++;
            }
            if (gfx_gl_enabled) {
                gfx_gl_task_end();
                gfx_gl_interp(-1);
            }
            gfx_st_interp[5]++;
        }
        host_perf_pop();
        tmem_lazy = 0;
        tpend_n = 0;
        ipass = 0;
        gs = s1;
        memcpy(gfx_tmem, tmem1, sizeof tmem1);
        cc_noise = noise1;
        sw_target();
        st_cover = cover + covered;
        st_tris = st[0]; st_raster = st[1]; st_tested = st[2]; st_drawn = st[3];
    }
    itrack = 0;
    hud_task = 0;
    gfx_host_ms += now_ms() - t0;
    /* --deterministic: the RDP's time, roughly (a pixel per 2 cycles), from
       the geometry, so that both renderers see the same timeline */
    host_charge(500000 + (uint64_t)(covered * 30));
    if (host_verbose && (gfx_tasks < 5 || gfx_tasks % 300 == 0))
        host_log("gfx task %d: dl %08X size %X%s\n", gfx_tasks, dl, size, gs.sync ? " (full sync)" : "");
    return gs.sync;
}

/* on the loop's OS thread, which has the GL context (fiber.h) */
struct gfx_call { uint32_t dl, size, ucode; int rdp, sync; };
static void gfx_task_call(void *p) {
    struct gfx_call *c = p;
    c->sync = gfx_task(c->dl, c->size, c->ucode, c->rdp);
}

int host_gfx_task(uint32_t dl, uint32_t size, uint32_t ucode, int rdp) {
    struct gfx_call c = { dl, size, ucode, rdp, 0 };
    fiber_call_on_loop(gfx_task_call, &c);
    return c.sync;
}

/* The VI shows fb from this retrace on, so the frame drawn into it is
   complete.  (Hooked here rather than in osViSwapBuffer: a call there would
   be counted as the game's CPU time.)  Its in-between images are shown
   first if every task of it had its passes. */
void host_gfx_frame_shown(uint32_t fb) {
    if (!gfx_interp || !ifr[0])
        return;
    unsigned drawn;
    if (gfx_gl_enabled) {
        drawn = gfx_gl_interp_swap(fb);
    } else {
        SwTwin *t = find_twin(fb);
        drawn = t ? t->drawn : 0;
        if (t)
            t->drawn = 0;
    }
    /* the frame on screen until now was held this long */
    int held = (int)(n_presents - shown_at);
    if (shown_k > 0) {
        if (held > shown_d)
            gfx_st_interp[8 + GFX_TWINS]++;
        else if (held < shown_d)
            gfx_st_interp[9 + GFX_TWINS]++;
    }
    if (shown_fb) {
        hold_last[1] = hold_last[0];
        hold_last[0] = held;
    }
    unsigned all = frame_k > 0 ? (1u << frame_k) - 1 : 0;
    int ready = !iframe_partial && frame_k > 0 && (drawn & all) == all;
    shown_fb = fb;
    shown_at = n_presents;
    shown_k = ready ? frame_k : 0;
    shown_d = frame_d;
    if (ready) {
        gfx_st_interp[6]++;
        gfx_st_interp[7 + frame_k]++;
    }
    iframe_partial = 0;
    frame_k = -1;
    icur ^= 1;
    iframe_reset(ifr[icur]);
}

/* the image for a present `phase` retraces after the frame in fb first
   showed: twin j of K (at t = (j + 1) / (K + 1)) while j < K, spread over
   the retraces the twins were made for */
static int pick_image(uint32_t fb, double phase) {
    if (!gfx_interp || fb != shown_fb || shown_k <= 0 || phase < 0)
        return -1;
    int j = (int)floor(phase * (shown_k + 1) / shown_d + 1e-6);
    return j < shown_k ? j : -1;
}

int gfx_interp_image(uint32_t fb) {
    static uint32_t last_fb;
    n_presents++;
    int k = pick_image(fb, (double)(n_presents - shown_at - 1));
    gfx_st_shown[0]++;
    gfx_st_shown[1] += fb != last_fb;
    gfx_st_shown[2] += k >= 0;
    last_fb = fb;
    return k;
}

static unsigned long long st_between[2];        /* presents between retraces, in-between images among them */

int gfx_interp_image_at(uint32_t fb, double phase) {
    int k = pick_image(fb, (double)(n_presents - shown_at - 1) + phase);
    st_between[0]++;
    st_between[1] += k >= 0;
    return k;
}

void host_gfx_interp_report(void) {
    if (!gfx_interp)
        return;
    unsigned long long *s = gfx_st_interp;
    host_log("interpolate: %llu in-between passes for %llu frames; of %llu vertex loads, %llu blended with the "
             "previous frame's, %llu without a match there, %llu vertices (of %llu) in loads that moved too far\n",
             s[5], s[6], s[0], s[1], s[7], s[4], s[2]);
    host_log("interpolate: %llu in-between passes replayed from the first pass's record, %llu run in full%s\n",
             st_replay[0], st_replay[1], gfx_gl_rec_missed ? " (and drawn without a state, a bug)" : "");
    host_log("interpolate: %llu rectangles in in-between passes, %llu moved, %llu moved too far\n",
             gfx_st_rect[0], gfx_st_rect[1], gfx_st_rect[2]);
    host_log("interpolate: %llu retraces presented: %llu showed a new frame of the game's, %llu an "
             "in-between one\n", gfx_st_shown[0], gfx_st_shown[1], gfx_st_shown[2]);
    if (st_between[0])
        host_log("interpolate: %llu presents between retraces (--display-hz %d), %llu of them in-between images\n",
                 st_between[0], gfx_interp_hz, st_between[1]);
    host_log("interpolate: frames by their in-between images:");
    for (int k = 1; k <= GFX_TWINS; k++)
        if (s[7 + k])
            host_log(" %d: %llu", k, s[7 + k]);
    host_log("; held longer than they were made for %llu, shorter %llu\n", s[8 + GFX_TWINS], s[9 + GFX_TWINS]);
}

void host_gfx_dump_stats(void) {
    host_log("gfx opcode counts:");
    for (int i = 0; i < 256; i++)
        if (host_gfx_stats[i])
            host_log(" %02X:%d", i, host_gfx_stats[i]);
    host_log("\ntris %llu rastered %llu pixels tested %llu inside %llu covered %.0f over %d tasks\n",
             st_tris, st_raster, st_tested, st_drawn, st_cover, gfx_tasks);
    host_log("gfx: TMEM loads: %llu blocks, %llu tiles, %llu palettes (%llu of them in in-between passes; copied "
             "%llu of those for a read)\n", st_loads[0], st_loads[1], st_loads[2], st_loads[3], st_tmem_syncs);
    host_log("gfx: %s renderer, %.3f ms of host time per task\n", gfx_gl_enabled ? "OpenGL" : "software",
             gfx_tasks ? gfx_host_ms / gfx_tasks : 0.0);
}
