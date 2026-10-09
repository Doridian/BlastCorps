/*
 * --model-icons: the game's pictures of its models drawn as the models.
 *
 * Many of the icons the game shows (the hint panels, the levels' goals,
 * the world map's and the best times' vehicles) are 64x64 pictures Rare
 * rendered from the game's own models: two 64x32 textures of the texture
 * table, loaded by func_80272C5C (hd_code 2E490.c) and drawn by
 * func_80272ED8 as texture rectangles or quads, a drop shadow in one
 * colour first.  The game's C tells the host where it loaded each one
 * (port_icon_texture, a call only: game memory is as it was), and the
 * renderer (gfx.c) asks here about every draw from a texture: a picture
 * listed below isn't drawn, and once its last row is, the model it shows
 * is, in the rectangle the picture covered, from a display list this file
 * makes in the renderer's own memory (gfx.h, GFX_HOST_BASE): the model's
 * vertices, its textures (decoded from the ROM, or a resource pack's
 * edited ones) and its display list, behind a viewport, a projection that fits
 * the model in the rectangle and a clear of the z-buffer there.  The
 * shadow is the model's outline on the ground, in the shadow's colour; a
 * fading picture fades the model (its alpha from the environment colour).
 * The world map's chopper, a picture on the globe, is the chopper's model
 * flying round the selected level (at the end).
 *
 * Only the renderer reads any of it, so runs are the same with it or
 * without; it is off in the runs that are compared (headless,
 * deterministic) unless asked for.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "gfx.h"
#include "micons.h"
#include "pack.h"
#include "romclass.h"

#include "romtab.h"

int micons_on;
unsigned micons_serial;

/* ---- the ROM ------------------------------------------------------------- */

static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }

static int seg_named(const char *name, uint32_t *start, uint32_t *end) {
    for (size_t i = 0; i < sizeof rom_segments / sizeof rom_segments[0]; i++)
        if (!strcmp(rom_segments[i].name, name)) {
            *start = rom_segments[i].start;
            *end = rom_segments[i].end;
            return 1;
        }
    return 0;
}

/* gzip member at ROM [start, end) into a malloc'd buffer: its length, and
   the member's own length in *used */
static uint8_t *inflate_at(const uint8_t *src, size_t n, long *len, size_t *used) {
    size_t cap = 0x40000;
    for (;;) {
        uint8_t *out = malloc(cap);
        long r = host_gunzip(src, n, out, cap, used);
        if (r >= 0 && (size_t)r < cap) {
            *len = r;
            return out;
        }
        free(out);
        if (r < 0 && cap >= 0x400000)
            return NULL;
        cap *= 2;
    }
}

/* ---- the renderer's memory ------------------------------------------------ */

/* models and textures from the bottom, made once; each draw's display
   list in a ring at the top (the in-between passes of --interpolate may
   run a frame's lists again, so a ring's worth of draws stays) */
#define RING_SPAN 0x40000u
#define STATIC_SPAN (GFX_HOST_SPAN - RING_SPAN)
#define SEG_HOST 15                             /* the segment the lists reach it by */
#define HADDR(off) ((uint32_t)SEG_HOST << 24 | (off))
static uint32_t mem_top = 0x10, ring_pos;

static uint32_t mem_alloc(uint32_t n) {
    uint32_t a = (mem_top + 15) & ~15u;
    if (a + n > STATIC_SPAN)
        return 0;
    mem_top = a + n;
    return a;
}

static uint8_t *mem(uint32_t off) { return gfx_host_mem + off; }

/* a display list being written */
typedef struct {
    uint32_t start, at, end;
} Dl;

static void dl_cmd(Dl *d, uint32_t w0, uint32_t w1) {
    if (d->at + 8 > d->end)
        return;
    port_wg32(mem(d->at), w0);
    port_wg32(mem(d->at + 4), w1);
    d->at += 8;
}

/* ---- textures ------------------------------------------------------------- */

#define NTEX 4096
static uint32_t tex_at[NTEX];           /* + 1: where it is in the renderer's memory (0: not made) */

/* texture id in the renderer's memory: its offset there, or 0 */
static uint32_t texture(uint32_t id) {
    if (id >= NTEX)
        return 0;
    if (tex_at[id])
        return tex_at[id] - 1;
    uint32_t ts, te;
    const uint8_t *rom = host_rom();
    if (!seg_named("texture_table", &ts, &te))
        return 0;
    const uint8_t *e = rom + ts + 8 * id;
    uint32_t off = rd32(e), n = rd16(e + 4), type = rd16(e + 6);
    uint8_t *px = NULL;
    size_t len = 0;
    uint32_t plen;
    const uint8_t *edited = host_tex_texels(id, &plen);    /* a resource pack's */
    if (edited) {
        px = malloc(plen);
        memcpy(px, edited, plen);
        len = plen;
    } else if (type == 0) {
        px = malloc(n);
        memcpy(px, rom + ts + off, n);
        len = n;
    } else if (type <= 6) {
        /* types 4 and 5 index a colour table the loader points at; the
           nearest earlier raw entry of its size is the extraction's guess
           (tools/assetlib/textures.py; none of the models below has one) */
        const uint8_t *lut = NULL;
        size_t lut_len = 0;
        if (type == 4 || type == 5) {
            uint32_t want = type == 4 ? 0x80 : 0x100;
            for (int j = (int)id - 1; j >= 0; j--) {
                const uint8_t *f = rom + ts + 8 * j;
                if (rd16(f + 6) == 0 && rd16(f + 4) == want) {
                    lut = rom + ts + rd32(f);
                    lut_len = want;
                    break;
                }
            }
        }
        px = host_blast_decode((int)type, rom + ts + off, n, lut, lut_len, &len);
    }
    if (!px)
        return 0;
    uint32_t at = mem_alloc((uint32_t)len);
    if (at) {
        memcpy(mem(at), px, len);
        tex_at[id] = at + 1;
    }
    free(px);
    return at;
}

/* ---- the models ------------------------------------------------------------ */

enum { V_NORMAL, V_FADE, NVARIANT };

/* the render mode the fading variant draws with: z-buffered, blended by
   the combiner's alpha (G_RM_ZB_XLU_SURF's blender and the z update of an
   opaque surface, so a model hides its own back); in 2-cycle mode the
   first cycle passes the colour through (G_RM_PASS) */
#define RM_FADE 0x00504270u
#define RM_FADE2 0x0C184270u
#define CC_ENV_ALPHA_W0 ((7u << 12) | (7u << 9))
#define CC_ENV_ALPHA_W1 ((7u << 21) | (7u << 18) | (7u << 12) | (5u << 9) | (7u << 3) | 5u)
#define CC_ENV_ALPHA_M0 ((7u << 12) | (7u << 9))
#define CC_ENV_ALPHA_M1 ((7u << 21) | (7u << 18) | (7u << 12) | (7u << 9) | (7u << 3) | 7u)
/* the shadow: the environment colour and alpha */
#define CC_SHADOW_W0 (0xFC000000u | 15u << 20 | 31u << 15 | 7u << 12 | 7u << 9 | 15u << 5 | 31u)
#define CC_SHADOW_W1 (15u << 28 | 15u << 24 | 7u << 21 | 7u << 18 | 5u << 15 | 7u << 12 | 5u << 9 | 5u << 6 | \
                      7u << 3 | 5u)

/* one display-list command of a model into variant v (fade_2cyc: the
   cycle type the commands so far set, which the fading render mode
   follows) */
static int fade_2cyc;

static void model_cmd(Dl *d, int v, uint32_t w0, uint32_t w1) {
    uint8_t op = w0 >> 24;
    if (op == 0xFD) {                                   /* G_SETTIMG: a texture's number */
        uint32_t t = texture(w1 & 0xFFF);
        w1 = HADDR(t);
    } else if (v != V_NORMAL && op == 0xFC) {           /* G_SETCOMBINE */
        w0 = (w0 & ~CC_ENV_ALPHA_M0) | CC_ENV_ALPHA_W0;
        w1 = (w1 & ~CC_ENV_ALPHA_M1) | CC_ENV_ALPHA_W1;
    } else if (v != V_NORMAL && op == 0xB9 && ((w0 >> 8) & 0xFF) == 3) {   /* the render mode */
        w1 = fade_2cyc ? RM_FADE2 : RM_FADE;
    } else if (v != V_NORMAL && op == 0xBA && ((w0 >> 8) & 0xFF) == 20) {  /* the cycle type */
        fade_2cyc = ((w1 >> 20) & 3) == 1;
        dl_cmd(d, w0, w1);
        w0 = 0xB900031Du;
        w1 = fade_2cyc ? RM_FADE2 : RM_FADE;
    } else if (v != V_NORMAL && op == 0xEF) {           /* G_RDPSETOTHERMODE */
        fade_2cyc = ((w0 >> 20) & 3) == 1;
        w1 = (w1 & 7) | (fade_2cyc ? RM_FADE2 : RM_FADE);
    }
    dl_cmd(d, w0, w1);
}

typedef struct {
    int ok, vehicle;
    uint32_t vtx;                       /* segment 9's base (a building's vertices), or 6's (a vehicle's) */
    uint32_t mtx;                       /* a vehicle's segment 7: its parts' matrices */
    uint32_t body[NVARIANT];            /* the display list, in each variant (a vehicle's opaque pass) */
    uint32_t body2[NVARIANT];           /* a vehicle's translucent pass */
    uint8_t *file;                      /* a vehicle's model file (its parts' animations) */
    uint32_t flen, blk, total;          /* ... its length, its matrix block's, the matrices' size */
    float lo[3], hi[3];                 /* the bounding box */
    float (*pos)[3];                    /* the vertices, for the shadow */
    uint8_t *used;                      /* ... and which of them a triangle drawn uses */
    uint8_t *solid;                     /* ... a vehicle's opaque pass uses (not its rotors' discs) */
    uint32_t npos;
    int slot[16];                       /* (the RSP's vertex buffer, while finding those) */
} Model;

/* n vertices from s (the ROM's bytes) at off, and their box into m */
static void put_vertices(Model *m, uint32_t off, const uint8_t *s, uint32_t n) {
    m->pos = malloc(n * sizeof *m->pos);
    m->used = calloc(n ? n : 1, 1);
    m->npos = m->pos && m->used ? n : 0;
    for (int k = 0; k < 16; k++)
        m->slot[k] = -1;
    for (uint32_t k = 0; k < n; k++, s += 16) {
        uint8_t *d = mem(off + 16 * k);
        for (int j = 0; j < 6; j++)
            port_wg16(d + 2 * j, rd16(s + 2 * j));
        memcpy(d + 12, s + 12, 4);
        for (int j = 0; j < 3; j++)
            if (m->npos)
                m->pos[k][j] = (int16_t)rd16(s + 2 * j);
    }
}

/* a command of the model's as drawn: the vertices its triangles use (the
   vertices at segment seg's base) */
static void mark_used(Model *m, int seg, uint32_t w0, uint32_t w1) {
    uint8_t op = w0 >> 24;
    if (op == 0x04 && (int)((w1 >> 24) & 15) == seg) {
        int n = ((w0 >> 20) & 0xF) + 1, v0 = (w0 >> 16) & 0xF;
        uint32_t first = (w1 & 0xFFFFFF) / 16;
        for (int i = 0; i < n && v0 + i < 16; i++)
            m->slot[v0 + i] = first + i < m->npos ? (int)(first + i) : -1;
    } else if (op == 0xBF) {
        int ix[3] = { (int)((w1 >> 16) & 0xFF) / 10, (int)((w1 >> 8) & 0xFF) / 10, (int)(w1 & 0xFF) / 10 };
        for (int k = 0; k < 3; k++)
            if (ix[k] < 16 && m->slot[ix[k]] >= 0)
                m->used[m->slot[ix[k]]] = 1;
    }
}

/* the box of the vertices drawn (all of them if none is known to be) */
static void model_box(Model *m) {
    int any = 0;
    for (uint32_t k = 0; k < m->npos; k++)
        any |= m->used[k];
    for (int j = 0; j < 3; j++) {
        m->lo[j] = 1e9f;
        m->hi[j] = -1e9f;
    }
    for (uint32_t k = 0; k < m->npos; k++) {
        if (any && !m->used[k])
            continue;
        m->used[k] = 1;
        for (int j = 0; j < 3; j++) {
            if (m->pos[k][j] < m->lo[j]) m->lo[j] = m->pos[k][j];
            if (m->pos[k][j] > m->hi[j]) m->hi[j] = m->pos[k][j];
        }
    }
    if (!m->npos)
        for (int j = 0; j < 3; j++)
            m->lo[j] = m->hi[j] = 0;
}

/* the model table's model i (a building: the vertices through segment 9,
   the display list pieces copied out as the game does, func_802BD1F8):
   its head, then the intact look's pass-0 piece, the head again and the
   pass-1 piece.  But not the shadow on the ground the pass-1 piece starts
   with (fans in the shade's colour, G_CC_SHADE), which the picture's own
   shadow stands for here. */
static void load_building(Model *m, int i) {
    uint32_t ts, te;
    const uint8_t *rom = host_rom();
    if (!seg_named("model_table", &ts, &te))
        return;
    uint32_t a = rd32(rom + ts + 4 * i), b = rd32(rom + ts + 4 * (i + 1));
    if (b <= a || ts + b > host_rom_size())
        return;
    long mlen, dlen;
    size_t used;
    uint8_t *md = inflate_at(rom + ts + a, b - a, &mlen, &used);
    if (!md)
        return;
    uint8_t *dd = used < b - a ? inflate_at(rom + ts + a + used, b - a - used, &dlen, NULL) : NULL;
    if (!dd || mlen < 0x50) {
        free(md);
        free(dd);
        return;
    }
    uint32_t dl0 = rd32(md + 0x10);                     /* the display list's start (the file's end) */
    size_t all = dl0 + (size_t)dlen;
    uint8_t *f = calloc(1, all + 8);
    memcpy(f, md, (size_t)mlen < dl0 ? (size_t)mlen : dl0);
    memcpy(f + dl0, dd, (size_t)dlen);
    free(md);
    free(dd);

    uint32_t head_end = rd32(f + 0x18), vend = rd32(f + 0x1C), looks = rd32(f + 0x38);
    if (vend > all || head_end > all || looks + 4 > all || vend < 0x50) {
        free(f);
        return;
    }
    /* the vertices */
    uint32_t nv = (vend - 0x50) / 16;
    m->vtx = mem_alloc(nv * 16);
    if (!m->vtx) {
        free(f);
        return;
    }
    put_vertices(m, m->vtx, f + 0x50, nv);
    /* the intact look: the first record of the group looks */
    uint32_t n = rd32(f + looks);
    uint32_t r = looks + 4 + 4 * n;
    if (r + 16 > all) {
        free(f);
        return;
    }
    uint32_t piece[4][2] = {
        { dl0, head_end }, { rd32(f + r), rd32(f + r + 4) }, { dl0, head_end }, { rd32(f + r + 8), rd32(f + r + 12) },
    };
    uint32_t cmds = 1;
    for (int p = 0; p < 4; p++) {
        if (piece[p][1] < piece[p][0] || piece[p][1] > all)
            piece[p][1] = piece[p][0];
        cmds += (piece[p][1] - piece[p][0]) / 8;
    }
    for (int v = 0; v < NVARIANT; v++) {
        uint32_t at = mem_alloc(cmds * 8);
        if (!at) {
            free(f);
            return;
        }
        Dl d = { at, at, at + cmds * 8 };
        int shade = 0;
        fade_2cyc = 0;
        for (int p = 0; p < 4; p++)
            for (uint32_t o = piece[p][0]; o + 8 <= piece[p][1]; o += 8) {
                uint32_t w0 = rd32(f + o), w1 = rd32(f + o + 4);
                if (w0 >> 24 == 0xFC)
                    shade = w0 == 0xFCFFFFFFu && w1 == 0xFFFE793Cu;
                if (shade && (w0 >> 24 == 0xBF || w0 >> 24 == 0xB5))
                    continue;
                if (v == 0)
                    mark_used(m, 9, w0, w1);
                model_cmd(&d, v, w0, w1);
            }
        dl_cmd(&d, 0xB8000000u, 0);
        m->body[v] = at;
    }
    free(f);
    model_box(m);
    m->ok = 1;
}

/* a vehicle's (or an object's) model file and its display list, the ROM
   segments name and name_dl (VehicleModel, game/vehicle.h): the vertices
   through segment 6, the parts' matrices through segment 7 (all identity:
   the parts at rest, the root too, the view being the modelview), and
   two passes, opaque and translucent (func_801E7598, the vehicle select
   screen, draws them so) */
static void load_vehicle(Model *m, const char *name) {
    char dname[64];
    uint32_t ms, me, ds, de;
    snprintf(dname, sizeof dname, "%s_dl", name);
    const uint8_t *rom = host_rom();
    if (!seg_named(name, &ms, &me) || !seg_named(dname, &ds, &de) || de > host_rom_size())
        return;
    long mlen, dlen;
    uint8_t *md = inflate_at(rom + ms, me - ms, &mlen, NULL);
    uint8_t *dd = md ? inflate_at(rom + ds, de - ds, &dlen, NULL) : NULL;
    if (!dd || mlen < 0x4C) {
        free(md);
        free(dd);
        return;
    }
    uint32_t dl0 = rd32(md + 0x1C), dl1 = rd32(md + 0x20);
    uint32_t vs = rd32(md + 0x14), ve = rd32(md + 0x18), p0 = rd32(md + 0x24), p2 = rd32(md + 0x2C);
    if (dl0 < (uint32_t)mlen || dl1 < dl0 || dl1 - dl0 > (uint32_t)dlen || ve > (uint32_t)mlen || ve < vs ||
        ve + 8 > (uint32_t)mlen || p0 < dl0 || p0 >= dl1 || p2 < dl0 || p2 >= dl1) {
        free(md);
        free(dd);
        return;
    }
    uint32_t total = rd32(md + ve), rest = rd32(md + ve + 4);
    m->vtx = mem_alloc(ve - vs);
    m->mtx = total <= 0x4000 && rest <= total && ve + 8 + rest <= (uint32_t)mlen ? mem_alloc(total) : 0;
    if (!m->vtx || !m->mtx) {
        free(md);
        free(dd);
        return;
    }
    put_vertices(m, m->vtx, md + vs, (ve - vs) / 16);
    /* the rest matrices as they are (no list reads them), then identities */
    for (uint32_t o = 0; o + 4 <= rest; o += 4)
        port_wg32(mem(m->mtx + o), rd32(md + ve + 8 + o));
    static const uint32_t ident[16] = { 0x00010000, 0, 1, 0, 0, 0x00010000, 0, 1 };  /* (an Mtx's words) */
    for (uint32_t o = rest; o + 64 <= total; o += 64)
        for (int k = 0; k < 16; k++)
            port_wg32(mem(m->mtx + o + 4 * k), ident[k]);
    /* the display list, in each variant, whole (its passes end in their
       own G_ENDDL and reach nothing outside it) */
    uint32_t n = (dl1 - dl0) & ~7u;
    for (int v = 0; v < NVARIANT; v++) {
        uint32_t at = mem_alloc(n + 8);
        if (!at) {
            free(md);
            free(dd);
            return;
        }
        Dl d = { at, at, at + n + 8 };
        fade_2cyc = 0;
        for (uint32_t o = 0; o < n; o += 8)
            model_cmd(&d, v, rd32(dd + o), rd32(dd + o + 4));
        dl_cmd(&d, 0xB8000000u, 0);
        m->body[v] = at + (p0 - dl0);
        m->body2[v] = at + (p2 - dl0);
    }
    /* what its two passes draw */
    uint32_t pass[2] = { p0 - dl0, p2 - dl0 };
    for (int q = 0; q < 2; q++) {
        for (uint32_t o = pass[q]; o + 8 <= n && rd32(dd + o) >> 24 != 0xB8; o += 8)
            mark_used(m, 6, rd32(dd + o), rd32(dd + o + 4));
        if (q == 0 && (m->solid = malloc(m->npos ? m->npos : 1)))
            memcpy(m->solid, m->used, m->npos);
    }
    m->file = md;
    m->flen = (uint32_t)mlen;
    m->blk = ve;
    m->total = total;
    free(dd);
    model_box(m);
    m->vehicle = 1;
    m->ok = 1;
}

/* ---- which pictures are of which model ------------------------------------- */

typedef struct {
    uint16_t tex[2];                    /* the picture's textures (any of its rows) */
    int16_t building;                   /* the model table's model, or -1: */
    const char *vehicle;                /* the vehicle's model file */
    int16_t yaw, pitch;                 /* the view, in degrees, as the picture has it */
    Model model;
    int tried;
} Spec;

static Spec specs[] = {
    /* the levels' goals (26570.c's D_802F9934: the building's model number) */
    { { 0x574, 0x573 }, 230, NULL, 35, 40 },           /* crates (mocrat2) */
    { { 0x565, 0x564 }, 189, NULL, 35, 40 },           /* rafts (float1) */
    { { 0x9D6, 0x9D5 }, 104, NULL, 35, 35 },           /* gas plants (mogas1) */
    { { 0x9D8, 0x9D7 }, 101, NULL, 35, 40 },           /* containers (moconw1) */
    { { 0x55E, 0x55D }, 186, NULL, 35, 20 },           /* spheres (ball1) */
    /* the carrier, the vehicles (the world map's and the best times'
       D_8020E350, by vehicle type), the communication point */
    { { 0x910, 0x90F }, -1, "cmo", 35, 25 },
    { { 0x571, 0x570 }, -1, "sideswipe", 35, 25 },
    { { 0x918, 0x917 }, -1, "magoo", 35, 15 },
    { { 0x90C, 0x90B }, -1, "buggy", 35, 25 },
    { { 0x70A, 0x709 }, -1, "bulldozer", 35, 25 },
    { { 0x772, 0x713 }, -1, "truck", 35, 25 },
    { { 0x912, 0x911 }, -1, "hotrod", 35, 25 },
    { { 0x916, 0x915 }, -1, "jetpack", 35, 15 },
    { { 0x90A, 0x909 }, -1, "bike", 35, 25 },
    { { 0x90E, 0x90D }, -1, "police", 35, 25 },
    { { 0x908, 0x907 }, -1, "ateam", 35, 25 },
    { { 0x91A, 0x919 }, -1, "starski", 35, 25 },
    { { 0x712, 0x711 }, -1, "minimagoo", 35, 15 },
    { { 0x2E4, 0x2E3 }, -1, "commpoint", 35, 20 },
};
#define NSPEC (int)(sizeof specs / sizeof specs[0])

static int spec_of(int tex) {
    /* PORT_MODEL_ICONS_AS=N: the hint panels' question mark and the
       goals' turning building are the Nth picture's model (to look at
       them all) */
    static int as = -2;
    if (as == -2) {
        const char *e = getenv("PORT_MODEL_ICONS_AS");
        as = e && *e ? atoi(e) : -1;
        if (as >= NSPEC)
            as = -1;
    }
    if (as >= 0 && (tex == 0xD44 || tex == 0xD43 || (tex >= 0x930 && tex <= 0x934)))
        return as;
    for (int i = 0; i < NSPEC; i++)
        if (specs[i].tex[0] == tex || specs[i].tex[1] == tex)
            return i;
    return -1;
}

static Model *spec_model(int s) {
    Spec *p = &specs[s];
    if (!p->tried) {
        p->tried = 1;
        if (!gfx_host_mem && !(gfx_host_mem = calloc(1, GFX_HOST_SPAN)))
            return NULL;
        if (p->building >= 0)
            load_building(&p->model, p->building);
        else
            load_vehicle(&p->model, p->vehicle);
        if (!p->model.ok)
            host_log("model-icons: no model for picture %03X\n", p->tex[0]);
    }
    return p->model.ok ? &p->model : NULL;
}

/* ---- the pictures the game has loaded ------------------------------------- */

#define NPIC 128
typedef struct {
    uint32_t addr;
    int16_t spec;
    uint8_t row, rows;
    uint32_t group;                     /* the rows of one picture (one frame of an icon) */
    uint32_t len, sum;                  /* its texels then, to tell it is still there */
} Pic;
static Pic pics[NPIC];
static int npic;
static uint32_t ngroup;

static uint32_t texel_sum(uint32_t addr, uint32_t len) {
    const uint8_t *p = port_ptr(addr);
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < len; i += 4)
        h = (h ^ (uint32_t)(p[i] | p[i + 1] << 8 | p[i + 2] << 16 | (uint32_t)p[i + 3] << 24)) * 16777619u;
    return h;
}

void port_icon_texture(unsigned int addr, int tex, int row, int rows, int flags) {
    if (!micons_on)
        return;
    if (tex < 0) {
        npic = 0;
        micons_serial++;
        return;
    }
    int s = spec_of(tex);
    if (row == 0)
        ngroup++;
    for (int i = 0; i < npic; i++)               /* the address's last picture */
        if (pics[i].addr == addr) {
            pics[i] = pics[--npic];
            micons_serial++;
            break;
        }
    if (s < 0 || npic == NPIC)
        return;
    Pic *p = &pics[npic++];
    p->addr = addr;
    p->spec = (int16_t)s;
    p->row = (uint8_t)row;
    p->rows = (uint8_t)rows;
    p->group = ngroup;
    p->len = (uint32_t)rows * 32 * 32 * ((flags & 2) ? 4 : 2);
    p->sum = texel_sum(addr, p->len);
    micons_serial++;
}

int micons_lookup(uint32_t addr) {
    if (!micons_on)
        return -1;
    for (int i = 0; i < npic; i++)
        if (pics[i].addr == addr) {
            if (texel_sum(addr, pics[i].len) != pics[i].sum) {    /* something else is there now */
                pics[i] = pics[--npic];
                micons_serial++;
                return -1;
            }
            return spec_model(pics[i].spec) ? i : -1;
        }
    return -1;
}

/* ---- drawing ---------------------------------------------------------------- */

/* m (row vectors, as the RSP's) into an Mtx at off */
static void put_mtx(uint32_t off, float m[4][4]) {
    uint8_t *p = mem(off);
    for (int k = 0; k < 16; k += 2) {
        int32_t a = (int32_t)lroundf(m[k / 4][k % 4] * 65536.0f), b = (int32_t)lroundf(m[k / 4][k % 4 + 1] * 65536.0f);
        port_wg32(p + 2 * k, (uint32_t)(a & 0xFFFF0000) | ((uint32_t)b >> 16));
        port_wg32(p + 32 + 2 * k, (uint32_t)a << 16 | ((uint32_t)b & 0xFFFF));
    }
}

static uint32_t ring_alloc(uint32_t n) {
    if (ring_pos + n > RING_SPAN)
        ring_pos = 0;
    uint32_t a = STATIC_SPAN + ring_pos;
    ring_pos += (n + 15) & ~15u;
    return a;
}

/* the shadow: the model's outline on the ground, as a light from above
   and to the right of the view throws it (the convex hull of its vertices
   projected so, at most 15 of them), one flat polygon, so that it is as
   dark everywhere as the picture's shadow was */
#define SHADOW_MAX 15
static int shadow_hull(const Model *m, const uint8_t *use, const float rot[3][3], float dx, float dz,
                       int16_t out[SHADOW_MAX][3]) {
    if (m->npos < 3)
        return 0;
    float y0 = m->lo[1];
    uint32_t n = m->npos;
    float (*g)[4] = malloc(n * sizeof *g);          /* ground x, z; on screen u, v */
    int *h = malloc(2 * (n + 1) * sizeof *h);
    if (!g || !h) {
        free(g);
        free(h);
        return 0;
    }
    for (uint32_t i = 0; i < n; i++) {
        float t = m->pos[i][1] - y0;
        g[i][0] = m->pos[i][0] + t * dx;
        g[i][1] = m->pos[i][2] + t * dz;
        g[i][2] = g[i][0] * rot[0][0] + y0 * rot[1][0] + g[i][1] * rot[2][0];
        g[i][3] = g[i][0] * rot[0][1] + y0 * rot[1][1] + g[i][1] * rot[2][1];
    }
    /* Andrew's monotone chain, on an index list sorted by u, then v */
    int *ix = malloc(n * sizeof *ix);
    if (!ix) {
        free(g);
        free(h);
        return 0;
    }
    uint32_t nu = 0;
    for (uint32_t i = 0; i < n; i++)
        if (use[i])
            ix[nu++] = (int)i;
    n = nu;
    if (n < 3) {
        free(g);
        free(h);
        free(ix);
        return 0;
    }
    for (uint32_t i = 1; i < n; i++) {              /* (insertion sort: a few hundred) */
        int x = ix[i];
        int j = (int)i - 1;
        while (j >= 0 && (g[ix[j]][2] > g[x][2] || (g[ix[j]][2] == g[x][2] && g[ix[j]][3] > g[x][3]))) {
            ix[j + 1] = ix[j];
            j--;
        }
        ix[j + 1] = x;
    }
#define CROSS(o, a, b) ((g[a][2] - g[o][2]) * (g[b][3] - g[o][3]) - (g[a][3] - g[o][3]) * (g[b][2] - g[o][2]))
    int k = 0;
    for (uint32_t i = 0; i < n; i++) {
        while (k >= 2 && CROSS(h[k - 2], h[k - 1], ix[i]) <= 0)
            k--;
        h[k++] = ix[i];
    }
    for (int i = (int)n - 2, lower = k + 1; i >= 0; i--) {
        while (k >= lower && CROSS(h[k - 2], h[k - 1], ix[i]) <= 0)
            k--;
        h[k++] = ix[i];
    }
#undef CROSS
    k--;                                            /* (the first again) */
    int cnt = k < SHADOW_MAX ? k : SHADOW_MAX;
    for (int i = 0; i < cnt; i++) {
        int j = h[(int)((long)i * k / cnt)];
        out[i][0] = (int16_t)lroundf(g[j][0]);
        out[i][1] = (int16_t)lroundf(y0);
        out[i][2] = (int16_t)lroundf(g[j][1]);
    }
    free(g);
    free(h);
    free(ix);
    return cnt >= 3 ? cnt : 0;
}

/* the display list drawing spec s's model over the rectangle r, with the
   picture's alpha prim[3], and its shadow in colour shadow (or none) */
static uint32_t draw_list(int s, const float r[4], const uint8_t prim[4], const uint8_t *shadow, uint32_t cimg,
                          int cimg_siz, int cimg_w, uint32_t zimg, const int sc[4]) {
    Model *m = spec_model(s);
    if (!m)
        return 0;
    Spec *sp = &specs[s];
    uint32_t base = ring_alloc(0x400);
    uint32_t vp = base, proj = base + 0x10, mv = base + 0x50, sv = base + 0x90;
    Dl d = { base + 0x190, base + 0x190, base + 0x400 };

    /* the view: turned about y by yaw, tilted toward the viewer by pitch,
       the box's middle at the origin, fitted to the rectangle */
    float cy = cosf(sp->yaw * (float)M_PI / 180), sy = sinf(sp->yaw * (float)M_PI / 180);
    float cp = cosf(sp->pitch * (float)M_PI / 180), spi = sinf(sp->pitch * (float)M_PI / 180);
    float rot[3][3] = { { cy, -sy * spi, sy * cp }, { 0, cp, spi }, { -sy, -cy * spi, cy * cp } };
    float mid[3], lo[3] = { 1e9f, 1e9f, 1e9f }, hi[3] = { -1e9f, -1e9f, -1e9f };
    for (int k = 0; k < 3; k++)
        mid[k] = (m->lo[k] + m->hi[k]) / 2;
    for (int c = 0; c < 8; c++) {
        float p[3] = { ((c & 1) ? m->hi[0] : m->lo[0]) - mid[0], ((c & 2) ? m->hi[1] : m->lo[1]) - mid[1],
                       ((c & 4) ? m->hi[2] : m->lo[2]) - mid[2] };
        for (int j = 0; j < 3; j++) {
            float q = p[0] * rot[0][j] + p[1] * rot[1][j] + p[2] * rot[2][j];
            if (q < lo[j]) lo[j] = q;
            if (q > hi[j]) hi[j] = q;
        }
    }
    float w = r[2] - r[0], h = r[3] - r[1];
    float ext = fmaxf((hi[0] - lo[0]) / w, (hi[1] - lo[1]) / h);
    if (ext <= 0)
        return 0;
    float k = 0.92f / ext;                      /* pixels a model unit */
    float sx = k * w / (w > 0 ? w : 1), sz = 1.0f / fmaxf(fmaxf(-lo[2], hi[2]), 1);
    float tx = -(lo[0] + hi[0]) / 2, ty = -(lo[1] + hi[1]) / 2;
    /* model -> (pixels from the rectangle's middle, x right, y up; z -1..1
       nearest first), by the modelview; the projection takes pixels to the
       viewport's -1..1 */
    float mvm[4][4] = { { 0 } };
    for (int i = 0; i < 3; i++) {
        mvm[i][0] = rot[i][0] * sx;
        mvm[i][1] = rot[i][1] * sx;
        mvm[i][2] = -rot[i][2] * sz;
    }
    float t0[3] = { -mid[0], -mid[1], -mid[2] };
    for (int j = 0; j < 3; j++)
        mvm[3][j] = t0[0] * mvm[0][j] + t0[1] * mvm[1][j] + t0[2] * mvm[2][j];
    mvm[3][0] += tx * sx;
    mvm[3][1] += ty * sx;
    mvm[3][3] = 1;
    float pm[4][4] = { { 2 / w, 0, 0, 0 }, { 0, 2 / h, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    put_mtx(proj, pm);
    put_mtx(mv, mvm);
    uint8_t *v = mem(vp);
    port_wg16(v + 0, (uint16_t)(int16_t)lroundf(w * 2));
    port_wg16(v + 2, (uint16_t)(int16_t)lroundf(h * 2));
    port_wg16(v + 4, 0x1FF);
    port_wg16(v + 6, 0);
    port_wg16(v + 8, (uint16_t)(int16_t)lroundf((r[0] + r[2]) * 2));
    port_wg16(v + 10, (uint16_t)(int16_t)lroundf((r[1] + r[3]) * 2));
    port_wg16(v + 12, 0x1FF);
    port_wg16(v + 14, 0);

    uint32_t host_phys = GFX_HOST_BASE & 0x1FFFFFFFu;
    dl_cmd(&d, 0xE7000000u, 0);
    dl_cmd(&d, 0xBC000006u, 0);                                         /* segment 0: physical */
    dl_cmd(&d, 0xBC000006u | (SEG_HOST * 4) << 8, host_phys);
    if (m->vehicle) {
        dl_cmd(&d, 0xBC000006u | (6 * 4) << 8, host_phys + m->vtx);
        dl_cmd(&d, 0xBC000006u | (7 * 4) << 8, host_phys + m->mtx);
    } else {
        dl_cmd(&d, 0xBC000006u | (9 * 4) << 8, host_phys + m->vtx);
    }
    /* the z-buffer under the rectangle, cleared (as the game clears it) */
    int zx0 = (int)floorf(r[0]), zy0 = (int)floorf(r[1]), zx1 = (int)ceilf(r[2]), zy1 = (int)ceilf(r[3]);
    if (zx0 < sc[0]) zx0 = sc[0];
    if (zy0 < sc[1]) zy0 = sc[1];
    if (zx1 > sc[2]) zx1 = sc[2];
    if (zy1 > sc[3]) zy1 = sc[3];
    int zbuf = zimg && cimg_w == 320 && zx1 > zx0 && zy1 > zy0;
    if (zbuf) {
        dl_cmd(&d, 0xFF000000u | 2u << 19 | (uint32_t)(cimg_w - 1), zimg & 0x00FFFFFFu);
        dl_cmd(&d, 0xBA001402u, 3u << 20);                              /* fill */
        dl_cmd(&d, 0xF7000000u, 0xFFFCFFFCu);
        dl_cmd(&d, 0xF6000000u | (uint32_t)(zx1 - 1) << 14 | (uint32_t)(zy1 - 1) << 2,
               (uint32_t)zx0 << 14 | (uint32_t)zy0 << 2);
        dl_cmd(&d, 0xE7000000u, 0);
        dl_cmd(&d, 0xFF000000u | (uint32_t)cimg_siz << 19 | (uint32_t)(cimg_w - 1), cimg & 0x00FFFFFFu);
    }
    dl_cmd(&d, 0xB6000000u, 0xFFFFFFFFu);
    dl_cmd(&d, 0xB7000000u, (zbuf ? 0x1u : 0) | 0x4u | 0x200u | 0x2000u);   /* z, shade, smooth, cull back */
    dl_cmd(&d, 0xBA001402u, 0);                                         /* 1-cycle */
    dl_cmd(&d, 0xBA001301u, 1u << 19);                                  /* perspective texture */
    dl_cmd(&d, 0xBA001001u, 0);                                         /* no LOD */
    dl_cmd(&d, 0xBA001102u, 0);                                         /* no detail */
    dl_cmd(&d, 0xBA000E02u, 0);                                         /* no TLUT */
    dl_cmd(&d, 0xBA000C02u, 2u << 12);                                  /* bilinear */
    dl_cmd(&d, 0xBA000903u, 6u << 9);                                   /* filtered, not converted */
    dl_cmd(&d, 0xB9000002u, 0);                                         /* no alpha compare */
    dl_cmd(&d, 0xB9000201u, 0);                                         /* z from the pixel */
    dl_cmd(&d, 0x03800010u, HADDR(vp));                                 /* the viewport */
    dl_cmd(&d, 0x01030040u, HADDR(proj));                               /* projection: load */
    dl_cmd(&d, 0x01020040u, HADDR(mv));                                 /* modelview: load */
    int16_t hull[SHADOW_MAX][3];
    int nh = 0;
    if (shadow) {
        float right[3] = { rot[0][0], 0, rot[2][0] }, toward[3] = { rot[0][2], 0, rot[2][2] };
        float rl = sqrtf(right[0] * right[0] + right[2] * right[2]);
        float tl = sqrtf(toward[0] * toward[0] + toward[2] * toward[2]);
        if (rl > 0 && tl > 0)
            nh = shadow_hull(m, m->used, rot, -0.35f * right[0] / rl + 0.15f * toward[0] / tl,
                             -0.35f * right[2] / rl + 0.15f * toward[2] / tl, hull);
    }
    if (nh) {
        /* blended over what is there, untextured, in the shadow's colour,
           without the z-buffer (the model then covers it) */
        for (int i = 0; i < nh; i++) {
            uint8_t *q = mem(sv + 16 * i);
            for (int j = 0; j < 3; j++)
                port_wg16(q + 2 * j, (uint16_t)hull[i][j]);
            for (int j = 6; j < 12; j += 2)
                port_wg16(q + j, 0);
            q[12] = q[13] = q[14] = 0;
            q[15] = 0xFF;
        }
        dl_cmd(&d, 0xB7000000u, 0x4u | 0x200u);
        dl_cmd(&d, 0xB6000000u, 0x1u | 0x3000u);
        dl_cmd(&d, 0xBB000000u, 0);                                     /* no texture */
        dl_cmd(&d, 0xB900031Du, 0x00504A40u);                           /* G_RM_XLU_SURF */
        dl_cmd(&d, CC_SHADOW_W0, CC_SHADOW_W1);
        dl_cmd(&d, 0xFB000000u, (uint32_t)shadow[0] << 24 | (uint32_t)shadow[1] << 16 | (uint32_t)shadow[2] << 8 |
                                    shadow[3]);
        dl_cmd(&d, 0x04000000u | (uint32_t)(nh - 1) << 20 | (uint32_t)(nh * 16), HADDR(sv));
        for (int i = 1; i + 1 < nh; i++)
            dl_cmd(&d, 0xBF000000u, (uint32_t)(i * 10) << 8 | (uint32_t)((i + 1) * 10));
        dl_cmd(&d, 0xE7000000u, 0);
        dl_cmd(&d, 0xB7000000u, (zbuf ? 0x1u : 0) | 0x2000u);
    }
    /* the environment colour: white with the picture's alpha (a vehicle's
       textures are scaled by its alpha) */
    uint32_t a = prim[3];
    dl_cmd(&d, 0xFB000000u, 0xFFFFFF00u | a);
    if (a != 0xFF)
        dl_cmd(&d, 0xB900031Du, RM_FADE);
    int var = a != 0xFF ? V_FADE : V_NORMAL;
    dl_cmd(&d, 0x06000000u, HADDR(m->body[var]));
    if (m->vehicle) {                                                   /* (its parts popped back to here) */
        dl_cmd(&d, 0x01020040u, HADDR(mv));
        dl_cmd(&d, 0x06000000u, HADDR(m->body2[var]));
    }
    dl_cmd(&d, 0xE7000000u, 0);
    dl_cmd(&d, 0xB8000000u, 0);
    return GFX_HOST_BASE + d.start;
}

/* the rows of the picture being drawn, gathered: the shadow's and the
   picture's.  The shadow (drawn first) is the picture's silhouette moved
   down and left, which a model's outline doesn't make a shadow of: it is
   drawn with the model, from below it, in its colour. */
static struct {
    int active;
    uint32_t group;
    float r[4];
    int done;                           /* the shadow: all its rows are in */
    uint8_t col[4];
} acc[2];

uint32_t micons_part(int h, float x0, float y0, float x1, float y1, int shadow, const uint8_t prim[4],
                     uint32_t cimg, int cimg_siz, int cimg_w, uint32_t zimg, const int scissor[4]) {
    if (h < 0 || h >= npic)
        return 0;
    const Pic *p = &pics[h];
    shadow = !!shadow;
    if (p->row == 0 || !acc[shadow].active || acc[shadow].group != p->group) {
        acc[shadow].active = 1;
        acc[shadow].group = p->group;
        acc[shadow].r[0] = x0; acc[shadow].r[1] = y0; acc[shadow].r[2] = x1; acc[shadow].r[3] = y1;
    } else {
        float *r = acc[shadow].r;
        if (x0 < r[0]) r[0] = x0;
        if (y0 < r[1]) r[1] = y0;
        if (x1 > r[2]) r[2] = x1;
        if (y1 > r[3]) r[3] = y1;
    }
    if (p->row + 1 < p->rows)
        return 0;
    acc[shadow].active = 0;
    if (shadow) {
        acc[1].done = 1;
        memcpy(acc[1].col, prim, 4);
        return 0;
    }
    const uint8_t *sh = acc[1].done && acc[1].group == p->group ? acc[1].col : NULL;
    acc[1].done = 0;
    return draw_list(p->spec, acc[0].r, prim, sh, cimg, cimg_siz, cimg_w, zimg, scissor);
}

/* ---- the world map's chopper ------------------------------------------------- */

/* The world map shows where the player is with a picture of the chopper
   (9570.c's D_80215A70: three frames of its rotors) on a quad lying on
   the globe, at the selected level or flying to the next (11530.c,
   func_801FC5B8).  Here it is the BCT chopper's model (the one that flies
   in at a level's start), circling the quad's middle: the circle closes
   while the quad moves and opens again where it stops.  It heads the way
   it goes, banks into the turn and leans forward with its speed (its
   parts' animations 2 and 3, as 72B80.c's func_802B98E0 sets them), and
   its rotors turn (animation 1).  All of it from the quad the renderer is
   given and the scheduler's retrace count, once a frame. */
#define GLOBE_SIZE 70.0f                /* its length, in the globe's units (radius 250) */
#define GLOBE_LIFT 14.0f                /* its middle above where the picture lay */
#define GLOBE_ORBIT 38.0f               /* the circle's radius */
#define GLOBE_LAP 330.0f                /* retraces a lap */
#define GLOBE_SETTLE 24.0f              /* retraces the circle takes to open or close (1/e) */
#define GLOBE_STILL 0.15f               /* the quad's speed (units a retrace) it is still below */
#define GLOBE_TURN 6.0f                 /* retraces the heading takes to follow (1/e) */
#define GLOBE_ROTOR 0.09f               /* the rotors' keys a retrace (three a turn) */
#define GLOBE_BANK 0.30f                /* its bank, at most (of the animation's half, 59 degrees) */
#define GLOBE_LEAN 0.32f                /* its lean forward, at most (of the half, 22 degrees) */
#define GLOBE_RADIUS 250.0f             /* the globe's (its shadow a little above) */
#define GLOBE_SHADOW 0x60               /* its shadow's alpha */

#include "sched_vars.h"

static Spec chopper = { { 0, 0 }, -1, "chopper", 0, 0, { 0 }, 0 };

static struct {
    int live;
    uint32_t clock;
    float anchor[3], pos[3], head[3];
    float radius, angle, rotor, bank, lean;
    uint32_t dl;
} globe;

static float dot3(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static void cross3(const float a[3], const float b[3], float o[3]) {
    float r[3] = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
    memcpy(o, r, sizeof r);
}
static float norm3(float v[3]) {
    float l = sqrtf(dot3(v, v));
    if (l > 0)
        for (int j = 0; j < 3; j++)
            v[j] /= l;
    return l;
}
/* v without its part along unit u */
static void flatten3(float v[3], const float u[3]) {
    float d = dot3(v, u);
    for (int j = 0; j < 3; j++)
        v[j] -= d * u[j];
}

/* the matrices animation a of model m puts its parts at, at key position
   u (key floor(u), the fraction on to the next; the keys loop, the last
   being the first again), into the parts' matrix buffer at buf: as
   56040.c's func_8029E5AC makes them from straight keys, the rest matrix
   times the key's scale, rotations about z, y and x (the short way round)
   and translation, times the second rest matrix */
static void pose(const Model *m, uint32_t buf, int a, float u) {
    const uint8_t *f = m->file;
    uint32_t len = m->flen, rest = m->blk + 8;
    if (!f || len < 0x14)
        return;
    uint32_t base = rd32(f + 0x10);
    if (base + 64 > len)
        return;
    uint32_t d = base + rd16(f + base + 2 * a);
    if (d + 2 > len)
        return;
    int n = f[d];
    if (n < 1 || d + n + 2 > len)
        return;
    int np = f[d + n + 1];
    uint32_t r = (d + n + 2 + 3) & ~3u;
    int k = 0;
    float t = 0;
    if (n > 1) {
        float w = fmodf(u, (float)(n - 1));
        if (w < 0)
            w += (float)(n - 1);
        k = (int)w;
        if (k > n - 2)
            k = n - 2;
        t = w - (float)k;
    }
    for (int p = 0; p < np; p++, r += 8 + 20 * (uint32_t)n) {
        if (r + 8 + 20 * (uint32_t)n > len)
            return;
        uint32_t mt = rd32(f + r), rs = rd32(f + r + 4);
        if (mt + 64 > m->total || rest + rs + 128 > len)
            continue;
        const uint8_t *k0 = f + r + 8 + 20 * k, *k1 = k + 1 < n ? k0 + 20 : k0;
        float h[10];
        for (int i = 0; i < 10; i++) {
            float x0 = (int16_t)rd16(k0 + 2 * i), x1 = (int16_t)rd16(k1 + 2 * i);
            if (i >= 3 && i < 6) {                  /* an angle, the short way */
                float dd = x1 - x0;
                if (dd < -32768) dd += 65536;
                if (dd > 32768) dd -= 65536;
                h[i] = x0 + dd * t;
            } else {
                h[i] = x0 + (x1 - x0) * t;
            }
        }
        float acc[4][4], op[4][4], tmp[4][4];
        for (int i = 0; i < 16; i++)
            acc[i / 4][i % 4] = (int32_t)rd32(f + rest + rs + 4 * i) / 65536.0f;
        /* acc * op, for each of the key's transforms that isn't nothing */
#define MUL_OP()                                                                                             \
    do {                                                                                                     \
        for (int i_ = 0; i_ < 4; i_++)                                                                       \
            for (int j_ = 0; j_ < 4; j_++)                                                                   \
                tmp[i_][j_] = acc[i_][0] * op[0][j_] + acc[i_][1] * op[1][j_] + acc[i_][2] * op[2][j_] +     \
                              acc[i_][3] * op[3][j_];                                                        \
        memcpy(acc, tmp, sizeof acc);                                                                        \
    } while (0)
#define OP_IDENTITY() memset(op, 0, sizeof op), op[0][0] = op[1][1] = op[2][2] = op[3][3] = 1
        if (h[0] != 256 || h[1] != 256 || h[2] != 256) {
            OP_IDENTITY();
            op[0][0] = h[0] / 256, op[1][1] = h[1] / 256, op[2][2] = h[2] / 256;
            MUL_OP();
        }
        for (int ax = 2; ax >= 0; ax--) {
            if (h[3 + ax] == 0)
                continue;
            float th = h[3 + ax] * (float)M_PI / 32768, c = cosf(th), s = sinf(th);
            OP_IDENTITY();
            if (ax == 0)
                op[1][1] = c, op[1][2] = s, op[2][1] = -s, op[2][2] = c;
            else if (ax == 1)
                op[0][0] = c, op[0][2] = -s, op[2][0] = s, op[2][2] = c;
            else
                op[0][0] = c, op[0][1] = s, op[1][0] = -s, op[1][1] = c;
            MUL_OP();
        }
        if (h[6] != 0 || h[7] != 0 || h[8] != 0) {
            OP_IDENTITY();
            op[3][0] = h[6], op[3][1] = h[7], op[3][2] = h[8];
            MUL_OP();
        }
        for (int i = 0; i < 16; i++)
            op[i / 4][i % 4] = (int32_t)rd32(f + rest + rs + 64 + 4 * i) / 65536.0f;
        MUL_OP();
#undef MUL_OP
#undef OP_IDENTITY
        put_mtx(buf + mt, acc);
    }
}

/* the frame's step (dt retraces on): where the circle's middle is now c */
static void globe_step(const float c[3], float dt) {
    float up[3] = { c[0], c[1], c[2] };
    float rad = norm3(up);
    if (!globe.live) {
        memset(&globe, 0, sizeof globe);
        globe.live = 1;
        globe.radius = GLOBE_ORBIT;
        memcpy(globe.anchor, c, sizeof globe.anchor);
        dt = 0;
    }
    float mv[3] = { c[0] - globe.anchor[0], c[1] - globe.anchor[1], c[2] - globe.anchor[2] };
    float speed = dt > 0 ? sqrtf(dot3(mv, mv)) / dt : 0;
    float settle = 1 - expf(-dt / GLOBE_SETTLE);
    globe.radius += ((speed < GLOBE_STILL ? GLOBE_ORBIT : 0) - globe.radius) * settle;
    globe.angle = fmodf(globe.angle + 2 * (float)M_PI * dt / GLOBE_LAP, 2 * (float)M_PI);
    globe.rotor = fmodf(globe.rotor + GLOBE_ROTOR * dt, 3);
    /* the circle, about the middle, on the globe */
    float pole[3] = { 0, 1, 0 }, e1[3], e2[3];
    cross3(pole, up, e1);
    if (norm3(e1) < 1e-3f) {
        float x[3] = { 1, 0, 0 };
        cross3(x, up, e1);
        norm3(e1);
    }
    cross3(up, e1, e2);
    float q[3], ca = cosf(globe.angle) * globe.radius, sa = sinf(globe.angle) * globe.radius;
    for (int j = 0; j < 3; j++)
        q[j] = c[j] + ca * e1[j] + sa * e2[j];
    float qu[3] = { q[0], q[1], q[2] };
    norm3(qu);
    for (int j = 0; j < 3; j++)
        q[j] = qu[j] * (rad + GLOBE_LIFT);
    /* the heading: the way it went, along the globe there */
    float v[3] = { q[0] - globe.pos[0], q[1] - globe.pos[1], q[2] - globe.pos[2] };
    flatten3(v, qu);
    float vs = dt > 0 ? sqrtf(dot3(v, v)) / dt : 0;
    float h[3] = { globe.head[0], globe.head[1], globe.head[2] };
    flatten3(h, qu);
    if (norm3(h) < 1e-3f) {                         /* (the first frame) */
        memcpy(h, e2, sizeof h);
        if (vs > 1e-4f)
            memcpy(h, v, sizeof h), norm3(h);
    }
    float turn = 0;
    if (vs > 1e-3f) {
        norm3(v);
        float follow = 1 - expf(-dt / GLOBE_TURN), nh[3], x[3];
        for (int j = 0; j < 3; j++)
            nh[j] = h[j] + (v[j] - h[j]) * follow;
        if (norm3(nh) > 1e-4f) {
            cross3(h, nh, x);
            turn = asinf(fmaxf(-1, fminf(1, dot3(x, qu)))) / dt;   /* radians a retrace, to the left + */
            memcpy(h, nh, sizeof h);
        }
    }
    float ease = 1 - expf(-dt / 8);
    float bank = fmaxf(-1, fminf(1, turn * 60));
    globe.bank += (bank - globe.bank) * ease;
    globe.lean += (fminf(1, vs / 1.2f) - globe.lean) * ease;
    memcpy(globe.head, h, sizeof h);
    memcpy(globe.pos, q, sizeof q);
    memcpy(globe.anchor, c, sizeof globe.anchor);
}

uint32_t micons_globe(const float c[3], const float mv[4][4], int first, uint8_t alpha, uint32_t cimg,
                      int cimg_siz, int cimg_w, uint32_t zimg, const int sc[4]) {
    if (!first)
        return globe.dl;
    globe.dl = 0;
    if (!chopper.tried) {
        chopper.tried = 1;
        if (!gfx_host_mem && !(gfx_host_mem = calloc(1, GFX_HOST_SPAN)))
            return 0;
        load_vehicle(&chopper.model, chopper.vehicle);
        if (!chopper.model.ok)
            host_log("model-icons: no model for the world map's chopper\n");
    }
    Model *m = &chopper.model;
    if (!m->ok || dot3(c, c) < 1)
        return 0;
    uint32_t now = port_var32(SCHED_FRAMECOUNT);
    float dt = (float)(uint32_t)(now - globe.clock);
    if (globe.live && dt > 120)                     /* (back on the map after a while) */
        globe.live = 0;
    globe_step(c, dt);
    globe.clock = now;

    /* the model's axes: x to its left (up x ahead), y up from the globe,
       z ahead */
    float up[3] = { globe.pos[0], globe.pos[1], globe.pos[2] }, right[3];
    float height = norm3(up) - GLOBE_RADIUS;
    cross3(up, globe.head, right);
    float s = GLOBE_SIZE / fmaxf(m->hi[2] - m->lo[2], 1);
    float mid[3] = { (m->lo[0] + m->hi[0]) / 2, (m->lo[1] + m->hi[1]) / 2, (m->lo[2] + m->hi[2]) / 2 };
    float place[4][4], full[4][4];
    for (int j = 0; j < 3; j++) {
        place[0][j] = right[j] * s;
        place[1][j] = up[j] * s;
        place[2][j] = globe.head[j] * s;
        place[3][j] = globe.pos[j] - s * (mid[0] * right[j] + mid[1] * up[j] + mid[2] * globe.head[j]);
    }
    place[0][3] = place[1][3] = place[2][3] = 0;
    place[3][3] = 1;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            full[i][j] = place[i][0] * mv[0][j] + place[i][1] * mv[1][j] + place[i][2] * mv[2][j] +
                         place[i][3] * mv[3][j];

    /* its shadow: its body's outline from above (once), flat on the globe
       below it, thrown down and to the right of the view as the icons'
       are (the view's axes: mv's columns), the further the higher it is */
    static int16_t hull[SHADOW_MAX][3];
    static int nhull = -1;
    if (nhull < 0) {
        static const float top[3][3] = { { 1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 } };   /* (x, z on the "screen") */
        nhull = shadow_hull(m, m->solid ? m->solid : m->used, top, 0, 0, hull);
    }
    float sfull[4][4];
    if (nhull) {
        float vr[3] = { mv[0][0], mv[1][0], mv[2][0] }, vu[3] = { mv[0][1], mv[1][1], mv[2][1] }, at[3];
        norm3(vr);
        norm3(vu);
        for (int j = 0; j < 3; j++)
            at[j] = globe.pos[j] + height * (0.5f * vr[j] - 0.7f * vu[j]);
        norm3(at);
        float flat[4][4];
        for (int j = 0; j < 3; j++) {
            flat[0][j] = right[j] * s;
            flat[1][j] = 0;
            flat[2][j] = globe.head[j] * s;
            flat[3][j] = at[j] * (GLOBE_RADIUS + 0.5f) - s * (mid[0] * right[j] + mid[2] * globe.head[j]);
        }
        flat[0][3] = flat[1][3] = flat[2][3] = 0;
        flat[3][3] = 1;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                sfull[i][j] = flat[i][0] * mv[0][j] + flat[i][1] * mv[1][j] + flat[i][2] * mv[2][j] +
                              flat[i][3] * mv[3][j];
    }

    uint32_t total = (m->total + 15) & ~15u;
    uint32_t base = ring_alloc(0x80 + 16 * SHADOW_MAX + total + 0x300);
    uint32_t mvo = base, smv = base + 0x40, sv = base + 0x80, mtx = sv + 16 * SHADOW_MAX;
    Dl d = { mtx + total, mtx + total, mtx + total + 0x300 };
    put_mtx(mvo, full);
    memcpy(mem(mtx), mem(m->mtx), m->total);
    pose(m, mtx, 0, 0);                             /* at rest */
    pose(m, mtx, 4, 0);                             /* its legs up */
    pose(m, mtx, 2, 0.5f - 0.5f * GLOBE_LEAN * globe.lean);
    pose(m, mtx, 3, 0.5f - 0.5f * GLOBE_BANK * globe.bank);
    pose(m, mtx, 1, globe.rotor);

    uint32_t host_phys = GFX_HOST_BASE & 0x1FFFFFFFu;
    dl_cmd(&d, 0xE7000000u, 0);
    dl_cmd(&d, 0xBC000006u, 0);                                         /* segment 0: physical */
    dl_cmd(&d, 0xBC000006u | (SEG_HOST * 4) << 8, host_phys);
    dl_cmd(&d, 0xBC000006u | (6 * 4) << 8, host_phys + m->vtx);
    dl_cmd(&d, 0xBC000006u | (7 * 4) << 8, host_phys + mtx);
    /* the z-buffer, cleared (the world map draws nothing else with it) */
    int zbuf = zimg && cimg_w == 320 && sc[2] > sc[0] && sc[3] > sc[1];
    if (zbuf) {
        dl_cmd(&d, 0xFF000000u | 2u << 19 | (uint32_t)(cimg_w - 1), zimg & 0x00FFFFFFu);
        dl_cmd(&d, 0xBA001402u, 3u << 20);                              /* fill */
        dl_cmd(&d, 0xF7000000u, 0xFFFCFFFCu);
        dl_cmd(&d, 0xF6000000u | (uint32_t)(sc[2] - 1) << 14 | (uint32_t)(sc[3] - 1) << 2,
               (uint32_t)sc[0] << 14 | (uint32_t)sc[1] << 2);
        dl_cmd(&d, 0xE7000000u, 0);
        dl_cmd(&d, 0xFF000000u | (uint32_t)cimg_siz << 19 | (uint32_t)(cimg_w - 1), cimg & 0x00FFFFFFu);
    }
    dl_cmd(&d, 0xB6000000u, 0xFFFFFFFFu);
    dl_cmd(&d, 0xB7000000u, (zbuf ? 0x1u : 0) | 0x4u | 0x200u | 0x2000u);   /* z, shade, smooth, cull back */
    dl_cmd(&d, 0xBA001402u, 0);                                         /* 1-cycle */
    dl_cmd(&d, 0xBA001301u, 1u << 19);                                  /* perspective texture */
    dl_cmd(&d, 0xBA001001u, 0);                                         /* no LOD */
    dl_cmd(&d, 0xBA001102u, 0);                                         /* no detail */
    dl_cmd(&d, 0xBA000E02u, 0);                                         /* no TLUT */
    dl_cmd(&d, 0xBA000C02u, 2u << 12);                                  /* bilinear */
    dl_cmd(&d, 0xBA000903u, 6u << 9);                                   /* filtered, not converted */
    dl_cmd(&d, 0xB9000002u, 0);                                         /* no alpha compare */
    dl_cmd(&d, 0xB9000201u, 0);                                         /* z from the pixel */
    if (nhull) {
        /* (as draw_list's: blended over the globe, untextured, no z) */
        put_mtx(smv, sfull);
        for (int i = 0; i < nhull; i++) {
            uint8_t *q = mem(sv + 16 * i);
            for (int j = 0; j < 3; j++)
                port_wg16(q + 2 * j, (uint16_t)hull[i][j]);
            for (int j = 6; j < 12; j += 2)
                port_wg16(q + j, 0);
            q[12] = q[13] = q[14] = 0;
            q[15] = 0xFF;
        }
        dl_cmd(&d, 0xB6000000u, 0xFFFFFFFFu);
        dl_cmd(&d, 0xB7000000u, 0x4u | 0x200u);
        dl_cmd(&d, 0xBB000000u, 0);                                     /* no texture */
        dl_cmd(&d, 0xB900031Du, 0x00504A40u);                           /* G_RM_XLU_SURF */
        dl_cmd(&d, CC_SHADOW_W0, CC_SHADOW_W1);
        dl_cmd(&d, 0xFB000000u, (uint32_t)(GLOBE_SHADOW * alpha / 255));
        dl_cmd(&d, 0x01020040u, HADDR(smv));
        dl_cmd(&d, 0x04000000u | (uint32_t)(nhull - 1) << 20 | (uint32_t)(nhull * 16), HADDR(sv));
        for (int i = 1; i + 1 < nhull; i++)
            dl_cmd(&d, 0xBF000000u, (uint32_t)(i * 10) << 8 | (uint32_t)((i + 1) * 10));
        dl_cmd(&d, 0xE7000000u, 0);
        dl_cmd(&d, 0xB6000000u, 0xFFFFFFFFu);
        dl_cmd(&d, 0xB7000000u, (zbuf ? 0x1u : 0) | 0x4u | 0x200u | 0x2000u);
    }
    dl_cmd(&d, 0x01020040u, HADDR(mvo));                                /* modelview: load */
    dl_cmd(&d, 0xFB000000u, 0xFFFFFF00u | alpha);
    if (alpha != 0xFF)
        dl_cmd(&d, 0xB900031Du, RM_FADE);
    int var = alpha != 0xFF ? V_FADE : V_NORMAL;
    dl_cmd(&d, 0x06000000u, HADDR(m->body[var]));
    dl_cmd(&d, 0x01020040u, HADDR(mvo));
    dl_cmd(&d, 0x06000000u, HADDR(m->body2[var]));
    dl_cmd(&d, 0xE7000000u, 0);
    dl_cmd(&d, 0xB8000000u, 0);
    globe.dl = GFX_HOST_BASE + d.start;
    return globe.dl;
}

