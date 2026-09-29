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
extern uint8_t gfx_tmem[4096];

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

/* the OpenGL back end (gfx_gl.c); gfx_gl_enabled is 0 without it */
extern int gfx_gl_enabled;
int gfx_gl_owns_target(void);                   /* is the color image the GPU's? */
void gfx_gl_tri(const GfxVtx *v, int n, const float *flat);  /* a convex polygon */
void gfx_gl_fill_rect(int x0, int y0, int x1, int y1);
void gfx_gl_tex_rect(int x0, int y0, int x1, int y1, int tile, float s_at0, float t_at0,
                     float dsdx, float dtdy, int flip);
void gfx_gl_zclear(int x0, int y0, int x1, int y1);
void gfx_gl_task_begin(void);
void gfx_gl_task_end(void);
void gfx_gl_texture_source(uint32_t addr);      /* SETTIMG: read back a GPU target? */

#endif
