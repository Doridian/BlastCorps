/*
 * The display-list front end (gfx.c) and its two back ends: the software
 * rasterizer (gfx.c, draws into RDRAM) and the OpenGL one (gfx_gl.c).
 *
 * gfx.c runs the RSP (matrices, vertices, lighting, clipping, culling) and
 * keeps the RDP's state (tiles, TMEM, combiner, other modes).  A triangle
 * or rectangle then goes to whichever back end owns the current color
 * image: the GPU draws the 320-wide 16-bit framebuffers, everything else
 * (the game's small 8-bit render-to-texture images) is drawn in software
 * into RDRAM, where the game reads it back as a texture.
 */
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

typedef struct {
    int fmt, siz, line, tmem, pal;
    int cms, cmt, masks, maskt, shifts, shiftt;
    int uls, ult, lrs, lrt;     /* 10.2 */
} GfxTile;

typedef struct {
    uint32_t seg[16];
    float mv[10][4][4];
    int mv_top;
    float proj[4][4];
    float mvp[4][4];
    int mvp_dirty;
    struct { float x, y, z, w, s, t, r, g, b, a; } v[16];   /* clip space, texels, 0..255 */
    float vd[16][3];            /* each vertex's x / w, y / w, 1 / w (gfx.c, vtx_tail) */
    uint32_t geom;
    uint32_t om_h, om_l;
    uint32_t cc0, cc1;
    uint32_t fill;
    uint8_t fog[4], blend[4], prim[4], env[4];
    uint8_t prim_lod;
    float tex_s, tex_t;
    int tex_tile, tex_on, tex_levels;
    GfxTile tile[8];
    uint32_t timg_addr;
    int timg_fmt, timg_siz, timg_w;
    uint32_t cimg_addr;
    int cimg_siz, cimg_w;
    uint32_t zimg_addr;
    int sc_x0, sc_y0, sc_x1, sc_y1;
    float vp_scale[3], vp_trans[3];
    int nlights;
    uint8_t lcol[8][3];
    float ldir[8][3];
    float lookat[2][3];
    int16_t fog_mul, fog_off;
    int sync;
    uint32_t tmem_gen;          /* bumped by every TMEM load */
    uint32_t tile_gen;          /* bumped by every tile change */
} GfxState;

extern GfxState gs;
/* bumped by every display-list command that may change what a draw's state
   is made from (all but vertices, triangles, matrices, calls and syncs) */
extern uint32_t gfx_state_serial;
extern uint8_t gfx_tmem[4096];
/* --hd-text: each TMEM word's RDRAM source, by the LoadBlock that wrote it (0: none) */
extern uint32_t gfx_tmem_src[512];
/* TMEM brought up to date: the OpenGL renderer's in-between passes copy
   the loads only when something reads it (gfx.c, tload) */
void gfx_tmem_sync(void);
void gfx_tmem_sync_range(uint32_t start, uint32_t len);   /* bytes start.. (mod 4096) */

/* a vertex after clipping: screen position (pixels, z 0..1), clip w, and
   the attributes (texels, 0..255), not divided by w */
typedef struct {
    float x, y, z, w;
    float s, t, r, g, b, a;
} GfxVtx;

/* the combiner's inputs, as gfx_cc_decode() indexes them */
enum { CC_COMB, CC_T0, CC_T1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO, CC_COMB_A, CC_T0_A,
       CC_T1_A, CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD, CC_PRIM_LOD, CC_NOISE, CC_N };

/* texture filtering: what the game asks for (N64 3-point when it sets
   G_TF_BILERP), forced point sampling, or 4-tap bilinear where the game
   filters */
enum { GFX_FILTER_N64, GFX_FILTER_POINT, GFX_FILTER_BILINEAR };
extern int gfx_filter;

/* widescreen: gfx_aspect (host.h) is the display's aspect, width / height.
   The game still draws its 320x240 frame; the renderer's frame is
   gfx_wide_off columns wider on each side, with the game's in the middle,
   and shows the 3D world there too (docs/PORT.md, "Widescreen").  Only
   the 320-wide 16-bit color images (the framebuffers) and the z-buffer
   are widened. */
extern int gfx_wide_off;                        /* 0: 4:3 */
float gfx_aspect_of(int w, int h);              /* the aspect to render at, for a w x h window */
int gfx_wide_off_for(float aspect);             /* the columns added on each side */
void gfx_set_wide(int off);                     /* the presenter's: from the next draw on */
/* x0..x1 of a scissor or a fill in the game's columns: one that covers all
   320 of them covers the wide frame.  The game's 1-cycle fills stop at
   319, one short. */
static inline void gfx_wide_span(int x0, int x1, int *a, int *b) {
    int full = gfx_wide_off > 0 && x0 <= 0 && x1 >= 319;
    *a = full ? -gfx_wide_off : x0;
    *b = full ? 320 + gfx_wide_off : x1;
}
/* the software renderer's wide framebuffer at addr (RGBA5551, host order,
   *w pixels across, 240 lines), or NULL: what video.c shows */
const uint16_t *gfx_sw_wide_frame(uint32_t addr, int *w);

/* shared helpers (gfx.c) */
void gfx_cc_decode(uint8_t idx[2][8]);          /* [cycle][a b c d  Aa Ab Ac Ad] */
int gfx_cycles(void);                           /* 1 or 2 (fill and copy: 1) */
int gfx_cycle_type(void);                       /* G_CYC_*: 0 1CYC, 1 2CYC, 2 COPY, 3 FILL */
int gfx_uses_tex1(void);
int gfx_filter_mode(void);                      /* 0 point, 1 N64 3-point, 2 bilinear */
void gfx_tile_dims(const GfxTile *t, int *w, int *h);
void gfx_fetch_texel(const GfxTile *t, int s, int tt, uint8_t *o);  /* s, t already wrapped */
/* LOD: the tiles and the fraction for a level of detail (texels per pixel) */
void gfx_lod_tiles(float lod, int base, int *tile0, int *tile1, float *frac);
int gfx_lod_on(void);
/* Coverage from alpha (othermode L's CVG_X_ALPHA, 0x1000): the RDP
   multiplies the pixel's coverage by the combined alpha, and with AA_EN
   (0x8) a pixel left with no coverage isn't written, whatever the blender
   would do.  A covered pixel's coverage is 8 eighths, alpha / 256 of it is
   kept to the eighth, so alpha below 32 drops it: the game's 2D sprites
   once faded in (G_RM_OPA_SURF with FORCE_BL, AA_EN, CVG_X_ALPHA and
   ALPHA_CVG_SEL, 0x0F0A7008), whose transparent texels would otherwise
   be drawn black.  Without FORCE_BL the anti-aliased blend that softens
   such an edge isn't emulated, and the edge is put at alpha 128.  The
   alpha (0..255) below which a pixel is dropped, 0 for none. */
static inline int gfx_cvg_alpha_min(uint32_t om_l) {
    if (!(om_l & 0x1000))
        return 0;
    if (!(om_l & 0x4000))
        return 128;
    return (om_l & 0x8) ? 32 : 0;
}
static inline int gfx_cvg_drops(uint32_t om_l, float alpha) { return alpha < gfx_cvg_alpha_min(om_l); }

/* the OpenGL back end (gfx_gl.c); gfx_gl_enabled is 0 without it */
extern int gfx_gl_enabled;
int gfx_gl_owns_target(void);                   /* is the color image the GPU's? */
void gfx_gl_tri(const GfxVtx *v, int n, const float *flat);  /* a convex polygon */
void gfx_gl_fill_rect(int x0, int y0, int x1, int y1);
void gfx_gl_tex_rect(int x0, int y0, int x1, int y1, int tile, float s_at0, float t_at0,
                     float dsdx, float dtdy, int flip);
void gfx_gl_zclear(int x0, int y0, int x1, int y1);
void gfx_gl_clear_rect(int x0, int y0, int x1, int y1);   /* black, whatever the draw state */
void gfx_gl_task_begin(void);
void gfx_gl_task_end(void);
void gfx_gl_texture_source(uint32_t addr);      /* SETTIMG: read back a GPU target? */
/* --interpolate (gfx.c; docs/PORT.md, "Frame rate"): up to GFX_TWINS
   in-between images a frame, each drawn into a twin of the framebuffer.
   gfx_gl_interp(k) draws into twin k from now on (-1: the targets);
   gfx_gl_interp_swap(fb) returns the twins of fb drawn since the last call
   (a mask) and starts over. */
#define GFX_TWINS 7
void gfx_gl_interp(int k);
unsigned gfx_gl_interp_swap(uint32_t fb);
/* the replays of the first pass (gfx.c): gfx_gl_rec_state(kind, tile) is
   the number of the draw state a draw of that kind would get now (kept
   until gfx_gl_rec_reset); gfx_gl_rec_force(n) draws with state n from
   now on, whatever gs says (-1: as gs says); gfx_gl_gen changes when a
   kept state may no longer be what a draw would get (a texture or target
   gone, the resolution changed) */
enum { GFX_GL_TRI, GFX_GL_FILL, GFX_GL_TEXRECT };
void gfx_gl_rec_reset(void);
int gfx_gl_rec_state(int kind, int tile);
void gfx_gl_rec_force(int id);     /* (-2: none recorded, nothing should be drawn) */
extern unsigned gfx_gl_gen;
extern unsigned long long gfx_gl_rec_missed;
/* which image a present at this retrace shows of fb: twin k (>= 0) or the
   frame itself (-1).  gfx_interp_image counts the retraces (once per VI
   present); gfx_interp_image_at is a present between retraces, `phase`
   retraces after the last one (host clock, --display-hz). */
int gfx_interp_image(uint32_t fb);
int gfx_interp_image_at(uint32_t fb, double phase);
/* the software renderer's twin k of fb (as gfx_sw_wide_frame), or NULL */
const uint16_t *gfx_sw_twin_frame(uint32_t fb, int k, int *w);
extern unsigned long long gfx_st_shown[3];     /* presents, new frames among them, in-between ones */
extern unsigned long long gfx_st_images;        /* retraces that showed another picture than the last */

#endif
