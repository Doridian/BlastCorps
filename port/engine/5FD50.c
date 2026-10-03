/*
 * hd_code 5FD50 (us.v11 0x802A4510-0x802A5510): which terrain cells can be
 * seen, and the terrain's display lists, as native C (engine.h).
 *
 * The visibility test walks a quadtree over the level's object grid
 * (LevelHeader unk0: cells, unk4: their size), one node a game frame
 * (VIS_NODES_PER_FRAME): a node whose bounding box (the cells' lowest and
 * highest points, LevelHeader.unk48's s16 pairs) the RSP finds on screen is
 * split in four, down to single cells, which are collected in D_803C3178.
 * When the stack of nodes runs empty, the cells found become the sorted
 * list D_803C30A8 (-1 at the end) that the C and the display lists here
 * read, and the walk starts again from the root.  The test is an RSP task
 * (box_visible) running the microcode at D_802E77B0 over the box's eight
 * corners; its first output word stays VIS_NOTHING_DRAWN when nothing of
 * them was drawn.  (Neither the port nor HLE writes that buffer, so every
 * node is in view: docs/PORT.md.)
 *
 * Each frame the terrain's four display lists are built from the groups
 * (LevelHeader.unkA0[0..4]: one cell each) and spans (unkA0[4..8]: several
 * cells) that are visible, with the texture animations
 * (LevelHeader.animTextures) patched into their G_SETTIMG and
 * G_SETPRIMCOLOR words.
 *
 * Code the original repeats with its own blocks is one helper here, given
 * the caller's blocks as a table (Blk), so `--cpu-model n64` charges each
 * path what it did.
 */
#include "engine.h"
#include "game/game.h"
#include "game/level.h"
#include "game/sched.h"

/* the walk's per-frame rate: one quadtree node is tested each game frame
   (func_802A467C runs once a frame) */
#define VIS_NODES_PER_FRAME 1

typedef struct QuadNode {
    s16 x, z, w, h;     /* in cells */
} QuadNode;

typedef struct TerrainGroup {   /* 0x14 bytes */
    /* 0x00 */ u32 dl;          /* offsets from the header: its display list, */
    /* 0x04 */ u32 dlNoTex;     /* the same after its texture setup, */
    /* 0x08 */ u32 dlEnd;       /* and its end */
    /* 0x0C */ s32 key;         /* its texture animation */
    /* 0x10 */ s32 cell;
} TerrainGroup;
SIZE_CHECK(TerrainGroup, 0x14);

typedef struct TerrainSpan {    /* 0x14 + 4 n bytes */
    /* 0x00 */ u32 dl, dlNoTex, dlEnd;  /* as TerrainGroup's */
    /* 0x0C */ s32 key;
    /* 0x10 */ s32 n;
    /* 0x14 */ s32 cells[1];    /* n of them, ascending */
} TerrainSpan;

typedef struct TexAnim {        /* 8 + 4 n bytes */
    /* 0x00 */ s32 key;
    /* 0x04 */ u8 n;            /* frames */
    /* 0x05 */ u8 cur;
    /* 0x06 */ u8 blend;        /* blends into the next frame */
    /* 0x07 */ u8 lodFrac;      /* by this much, 0..ANIM_LOD_MAX */
    /* 0x08 */ u16 period;      /* game frames per animation frame */
    /* 0x0A */ u16 count;       /* game frames into the current one */
    /* 0x0C */ u32 tex1[1];     /* frames 1 .. n - 1 (frame 0 is the list's own) */
} TexAnim;
#define ANIM_LOD_MAX 0xFF
/* frame i's texture (i >= 1: tex1[i - 1]) */
#define ANIM_TEX(a, i) (*(u32 *)((u8 *)(a) + 8 + (i) * 4))
#define ANIM_NEXT(a) ((TexAnim *)((u8 *)(a) + 8 + (a)->n * 4))
#define SPAN_NEXT(s) ((TerrainSpan *)((u8 *)(s) + 0x14 + (s)->n * 4))
/* the level file at header offset `off` */
#define AT(h, off) ((u8 *)(h) + (off))

/* LevelHeader.displayLists[] and unkA0[] as this file reads them */
#define DL_OTHERS       1       /* .. 2: the level's other display lists */
#define DL_PREFIX       4       /* .. 5: what starts each terrain list */
#define TERRAIN_GROUPS  0       /* unkA0[0..4]: four lists' groups */
#define TERRAIN_SPANS   4       /* unkA0[4..8]: and spans */

extern u8 D_803BE740[0x40];     /* the visibility task: a SchedTask whose last
                                   0x20 bytes run into its DRAM stack */
extern u64 D_803BE780[0x80];    /* that stack */
extern u32 D_803BEB80[0x1000];  /* the task's output */
extern u32 D_803C2B80;          /* ... its size */
extern QuadNode *PTR32 D_803C2B88;      /* the node stack's top */
extern QuadNode D_803C2B90[100];        /* the node stack */
extern s16 D_803C2EB0[0xFC];    /* the keys drawn (terrain_dl) */
extern s16 D_803C30A8[100];     /* the visible cells, ascending, -1 at the end */
extern s16 *PTR32 D_803C3170;   /* the end of ... */
extern s16 D_803C3178[100];     /* ... the cells found visible in this walk */
extern u32 D_803C3240;          /* the animation's texture (0: the list's own) */
extern u32 D_803C3244;          /* ... the one it blends into */
extern u16 D_803C3248;          /* game frames before the walk's next step */
extern u8 D_803C324A;           /* the animation blends */
extern u8 D_803C324B;           /* ... by this much */
extern u8 D_802E6820[], D_802E68F0[], D_802E77B0[], D_8030EE60[];
extern u64 D_8036AFB0[];
extern OSMesgQueue D_803153D8;
extern SchedClient D_803156D8;
void func_80285110(u32 msg);

#define VIS_TASK_MSG 0x4D3              /* the visibility task's done message */
#define VIS_NOTHING_DRAWN 0xE8000000    /* its output's first word when the box
                                           wasn't drawn (a G_RDPTILESYNC) */
#define VIS_OUT_CHECKED 0x20            /* the output bytes read back */

#define OP(w) (((w) & 0xFF000000) >> 24)
#define G_DL_W0 _SHIFTL(G_DL, 24, 8)
#define G_ENDDL_W0 _SHIFTL(G_ENDDL, 24, 8)
_Static_assert(G_DL_W0 == 0x06000000u && G_ENDDL_W0 == 0xB8000000u, "F3D's G_DL, G_ENDDL");
_Static_assert(G_SETTIMG == 0xFD && G_SETPRIMCOLOR == 0xFA, "F3D's G_SETTIMG, G_SETPRIMCOLOR");

/* a block table, for code repeated with its own blocks; an entry with no
   instructions is a block that copy doesn't have */
typedef struct { u16 id, n; } Blk;
#define B(addr) {ENGINE_BLK_##addr}
#define B_NONE {0, 0}
#define BLK(t, i)                                                           \
    do {                                                                    \
        if ((t)[i].n != 0) {                                                \
            ENGINE_BLK_((t)[i].id, (t)[i].n);                               \
        }                                                                   \
    } while (0)

/* ---- the quadtree walk --------------------------------------------------- */

/* every cell visible and the node stack empty, so the walk starts from the
   root at its next step */
static void vis_all(const Blk *b) {
    s32 n = D_803BE714 * D_803BE716;
    s32 i;

    D_803C2B88 = D_803C2B90;
    for (i = 0;; i++) {
        BLK(b, 0);
        if (i == n) {
            break;
        }
        BLK(b, 1);
        D_803C3178[i] = i;
        D_803C30A8[i] = i;
    }
    D_803C3170 = &D_803C3178[n];
    D_803C30A8[n] = -1;
}

/* func_802A4510 (the level loader's): every cell visible; the walk starts
   at once */
REGS()
void func_802A4510(void) {
    static const Blk b[] = { B(802A4584), B(802A458C) };

    ENGINE_BLK(802A4510);
    D_803C3248 = 0;
    vis_all(b);
    ENGINE_BLK(802A45A4);
}

/* func_802A45D4 (00000.c's): the same, the walk starting in `wait` game
   frames */
void func_802A45D4(s32 wait) {
    static const Blk b[] = { B(802A4638), B(802A4640) };

    ENGINE_BLK(802A45D4);
    D_803C3248 = wait;
    vis_all(b);
    ENGINE_BLK(802A4658);
}

/* (802A49A8) the lowest and highest points of the cells under a node */
static void node_bounds(LevelHeader *h, const QuadNode *nd, s32 *lo, s32 *hi) {
    s32 width = (s16)h->unk0[0];
    s16 *row = (s16 *)AT(h, h->unk48) + (nd->z * width + nd->x) * 2;
    s32 min = 0x7FFF, max = -0x8000;
    s32 rows = nd->h;

    ENGINE_BLK(802A49A8);
    do {
        s16 *p = row;
        s32 cols = nd->w;

        ENGINE_BLK(802A49E8);
        do {
            ENGINE_BLK(802A49EC);
            if (p[0] < min) {
                ENGINE_BLK(802A4A00);
                min = p[0];
            }
            ENGINE_BLK(802A4A04);
            if (max < p[1]) {
                ENGINE_BLK(802A4A10);
                max = p[1];
            }
            ENGINE_BLK(802A4A14);
            p += 2;
        } while (--cols != 0);
        ENGINE_BLK(802A4A20);
        row += width * 2;
    } while (--rows != 0);
    ENGINE_BLK(802A4A30);
    *lo = min;
    *hi = max;
}

/* (802A4A50) the box's eight corners: the bottom four, then the top four */
static void node_box(LevelHeader *h, const QuadNode *nd, Vtx *v, s32 lo, s32 hi) {
    s32 sx = (s16)h->unk4[0], sz = (s16)h->unk4[1];
    s16 x0 = nd->x * sx, x1 = (nd->x + nd->w) * sx;
    s16 z0 = nd->z * sz, z1 = (nd->z + nd->h) * sz;

    ENGINE_BLK(802A4A50);
    v[0].v.ob[0] = x0; v[0].v.ob[1] = lo; v[0].v.ob[2] = z0;
    v[1].v.ob[0] = x1; v[1].v.ob[1] = lo; v[1].v.ob[2] = z0;
    v[2].v.ob[0] = x1; v[2].v.ob[1] = lo; v[2].v.ob[2] = z1;
    v[3].v.ob[0] = x0; v[3].v.ob[1] = lo; v[3].v.ob[2] = z1;
    v[4].v.ob[0] = x0; v[4].v.ob[1] = hi; v[4].v.ob[2] = z0;
    v[5].v.ob[0] = x1; v[5].v.ob[1] = hi; v[5].v.ob[2] = z0;
    v[6].v.ob[0] = x1; v[6].v.ob[1] = hi; v[6].v.ob[2] = z1;
    v[7].v.ob[0] = x0; v[7].v.ob[1] = hi; v[7].v.ob[2] = z1;
}

/* (802A4B0C) the RSP's test of the box in `vtx` (drawn by `dl`, `size`
   bytes): whether any of it is on screen.  A 1 x 1 grid always is. */
static s32 box_visible(Gfx *dl, Vtx *vtx, s32 size) {
    SchedTask *t = (SchedTask *)D_803BE740;
    s32 drawn;

    ENGINE_BLK(802A4B0C);
    if ((s16)D_803BE714 == 1) {
        ENGINE_BLK(802A4B48);
        if ((s16)D_803BE716 == 1) {
            ENGINE_BLK(802A4CA0);
            ENGINE_BLK(802A4CAC);
            return 1;
        }
    }
    ENGINE_BLK(802A4B5C);
    t->list.t.ucode_boot_size = D_802E68F0 - D_802E6820;
    t->list.t.type = M_GFXTASK;
    t->list.t.flags = 0;
    t->list.t.ucode_boot = (u64 *)D_802E6820;
    t->list.t.ucode = (u64 *)D_802E77B0;
    t->list.t.ucode_size = 0x1000;   /* SP_UCODE_SIZE */
    t->list.t.ucode_data = (u64 *)D_8030EE60;
    t->list.t.ucode_data_size = 0x800;   /* SP_UCODE_DATA_SIZE */
    t->list.t.dram_stack = D_803BE780;
    t->list.t.dram_stack_size = sizeof(D_803BE780);
    t->list.t.output_buff = (u64 *)D_803BEB80;
    t->list.t.output_buff_size = (u64 *)&D_803C2B80;
    t->list.t.yield_data_ptr = D_8036AFB0;
    t->list.t.yield_data_size = OS_YIELD_DATA_SIZE;
    t->flags = 1;
    t->msg = (OSMesg)VIS_TASK_MSG;
    t->framebuffer = NULL;
    t->list.t.data_ptr = (u64 *)dl;
    t->list.t.data_size = size;
    t->msgQ = &D_803153D8;
    t->client = &D_803156D8;
    osWritebackDCache(vtx, 8 * sizeof(Vtx));
    ENGINE_BLK(802A4C44);
    osWritebackDCache(t, sizeof(D_803BE740));
    ENGINE_BLK(802A4C50);
    osInvalDCache(D_803BEB80, VIS_OUT_CHECKED);
    ENGINE_BLK(802A4C60);
    osSendMesg(&D_80315440.interruptQ, (OSMesg)t, OS_MESG_BLOCK);
    ENGINE_BLK(802A4C7C);
    func_80285110(VIS_TASK_MSG);        /* waits for it */
    ENGINE_BLK(802A4C84);
    drawn = D_803BEB80[0] != VIS_NOTHING_DRAWN;
    if (drawn) {
        ENGINE_BLK(802A4CA0);
    } else {
        ENGINE_BLK(802A4CA8);
    }
    ENGINE_BLK(802A4CAC);
    return drawn;
}

/* (802A484C) test the node; split a visible one in four onto the stack,
   or keep a visible cell */
static void node_visit(LevelHeader *h, const QuadNode *nd, Gfx *dl, Vtx *vtx, s32 size) {
    /* each quarter's blocks: its width tested, its height, pushed */
    static const Blk quarter[4][3] = {
        { B(802A48BC), B(802A48D8), B(802A48E0) },
        { B(802A48F4), B(802A4904), B(802A490C) },
        { B(802A4920), B(802A4934), B(802A493C) },
        { B(802A4950), B(802A4960), B(802A4968) },
    };
    s32 x = nd->x, z = nd->z, w = nd->w, hh = nd->h;
    s32 lo, hi, single, w0, h0, q;

    ENGINE_BLK(802A484C);
    node_bounds(h, nd, &lo, &hi);
    ENGINE_BLK(802A4878);
    node_box(h, nd, vtx, lo, hi);
    ENGINE_BLK(802A4880);
    if (!box_visible(dl, vtx, size)) {
        ENGINE_BLK(802A4888);
        ENGINE_BLK(802A497C);
        return;
    }
    ENGINE_BLK(802A4888);
    ENGINE_BLK(802A4890);
    single = 0;
    if (w == 1) {
        ENGINE_BLK(802A489C);
        single = hh == 1;
    }
    if (single) {
        ENGINE_BLK(802A48A4);
        *D_803C3170++ = z * (s16)h->unk0[0] + x;
        ENGINE_BLK(802A497C);
        return;
    }
    /* the quarters: left/right halves w0 and w - w0, top/bottom h0 and
       hh - h0, the empty ones (of a 1-wide or 1-high node) left out */
    w0 = (u32)w >> 1;
    h0 = (u32)hh >> 1;
    for (q = 0; q < 4; q++) {
        s32 right = q & 1, bottom = q >> 1;
        s32 qw = right ? w - w0 : w0;
        s32 qh = bottom ? hh - h0 : h0;

        BLK(quarter[q], 0);
        if (qw != 0) {
            BLK(quarter[q], 1);
            if (qh != 0) {
                QuadNode *t = D_803C2B88++;

                BLK(quarter[q], 2);
                t->x = right ? x + w0 : x;
                t->z = bottom ? z + h0 : z;
                t->w = qw;
                t->h = qh;
            }
        }
    }
    ENGINE_BLK(802A497C);
}

/* insert `cell` into the ascending list [list, end), which has room after
   end */
static void sorted_insert(s16 *list, s16 *end, s16 cell) {
    s16 *p;

    for (p = list;; p++) {
        ENGINE_BLK(802A477C);
        if (p == end) {
            ENGINE_BLK(802A47C0);
            *end = cell;
            return;
        }
        ENGINE_BLK(802A4784);
        if (cell < *p) {
            break;
        }
        ENGINE_BLK(802A4794);
    }
    /* in at p, the rest moved up */
    ENGINE_BLK(802A479C);
    {
        s16 moved = *p;

        *p = cell;
        do {
            s16 next = p[1];

            ENGINE_BLK(802A47A0);
            p[1] = moved;
            p++;
            moved = next;
        } while (p != end);
    }
    ENGINE_BLK(802A47B4);
}

/* (802A470C) one step of the walk */
static void vis_step(LevelHeader *h, Gfx *dl, Vtx *vtx, s32 size) {
    ENGINE_BLK(802A470C);
    if (D_803C2B88 == D_803C2B90) {
        /* the walk is done: its cells become the visible list, sorted, and
           the root goes on the stack */
        s16 *c;
        s32 n = 0;

        ENGINE_BLK(802A4730);
        D_803C2B90[0].x = 0;
        D_803C2B90[0].z = 0;
        D_803C2B90[0].w = h->unk0[0];
        D_803C2B90[0].h = h->unk0[1];
        D_803C2B88 = &D_803C2B90[1];
        for (c = D_803C3178;; c++, n++) {
            ENGINE_BLK(802A4768);
            if (c == D_803C3170) {
                break;
            }
            ENGINE_BLK(802A4770);
            sorted_insert(D_803C30A8, &D_803C30A8[n], *c);
        }
        ENGINE_BLK(802A47D0);
        D_803C30A8[n] = -1;
        D_803C3170 = D_803C3178;
    }
    ENGINE_BLK(802A47EC);
    {
        /* (copied: node_visit pushes over it) */
        QuadNode nd = *--D_803C2B88;

        node_visit(h, &nd, dl, vtx, size);
    }
    ENGINE_BLK(802A4824);
}

/* func_802A467C (00000.c's, every frame): a step of the walk, unless it is
   waiting */
void func_802A467C(LevelHeader *h, Gfx *dl, Vtx *vtx, s32 size) {
    s32 i;

    ENGINE_BLK(802A467C);
    if (D_803C3248 == 0) {
        for (i = 0; i < VIS_NODES_PER_FRAME; i++) {
            ENGINE_BLK(802A46C0);
            vis_step(h, dl, vtx, size);
            ENGINE_BLK(802A46C8);
        }
    } else {
        ENGINE_BLK(802A46D0);
        D_803C3248--;
    }
    ENGINE_BLK(802A46D8);
}

/* ---- the terrain's display lists ----------------------------------------- */

/* (802A5020) every texture animation a game frame on */
static void tex_anims_tick(LevelHeader *h) {
    TexAnim *a = (TexAnim *)AT(h, h->animTextures);
    TexAnim *end = (TexAnim *)AT(h, h->terrain);

    ENGINE_BLK(802A5020);
    for (;; a = ANIM_NEXT(a)) {
        u32 count;

        ENGINE_BLK(802A5038);
        if (a == end) {
            break;
        }
        ENGINE_BLK(802A5040);
        count = a->count + 1;
        if (a->period == count) {
            /* the next frame, after the last the first */
            u32 next;

            ENGINE_BLK(802A5054);
            next = a->cur + 1;
            if (a->n == next) {
                ENGINE_BLK(802A5068);
                next = 0;
            }
            ENGINE_BLK(802A506C);
            a->cur = next;
            count = 0;
        }
        ENGINE_BLK(802A5074);
        if (a->blend != 0) {
            ENGINE_BLK(802A5080);
            a->lodFrac = (ANIM_LOD_MAX * count) / a->period;
        }
        ENGINE_BLK(802A50B4);
        a->count = count;
    }
    ENGINE_BLK(802A50CC);
}

/* (802A50DC) animation `key`'s textures for the patches below; whether
   there is anything to patch */
static s32 tex_anim_find(LevelHeader *h, s32 key) {
    TexAnim *a = (TexAnim *)AT(h, h->animTextures);
    TexAnim *end = (TexAnim *)AT(h, h->terrain);
    u32 cur;
    s32 patch;

    ENGINE_BLK(802A50DC);
    for (;; a = ANIM_NEXT(a)) {
        ENGINE_BLK(802A5104);
        if (a == end) {
            ENGINE_BLK(802A51DC);
            ENGINE_BLK(802A51E0);
            return 0;
        }
        ENGINE_BLK(802A510C);
        if (a->key == key) {
            break;
        }
        ENGINE_BLK(802A5118);
    }
    ENGINE_BLK(802A512C);
    cur = a->cur;
    if (cur != 0) {
        ENGINE_BLK(802A5138);
        D_803C3240 = ANIM_TEX(a, cur);
    } else {
        ENGINE_BLK(802A5158);
        D_803C3240 = 0;
    }
    ENGINE_BLK(802A5164);
    D_803C324A = a->blend;
    if (a->blend == 0) {
        /* a frame other than the list's own */
        ENGINE_BLK(802A5178);
        patch = cur != 0;
        if (patch) {
            ENGINE_BLK(802A5180);
        } else {
            ENGINE_BLK(802A51DC);
        }
    } else {
        ENGINE_BLK(802A5188);
        cur++;
        if (a->n == cur) {
            ENGINE_BLK(802A5198);
            D_803C3244 = 0;
        } else {
            ENGINE_BLK(802A51A8);
            D_803C3244 = ANIM_TEX(a, cur);
        }
        ENGINE_BLK(802A51C4);
        D_803C324B = a->lodFrac;
        patch = 1;
    }
    ENGINE_BLK(802A51E0);
    return patch;
}

/* the display list [h + from, h + to) appended to out; the new end */
static u32 *dl_append(LevelHeader *h, u32 from, u32 to, u32 *out, const Blk *b) {
    u32 *src = (u32 *)AT(h, from);
    u32 *end = (u32 *)AT(h, to);

    for (;;) {
        BLK(b, 0);
        if (src == end) {
            break;
        }
        BLK(b, 1);
        out[0] = src[0];
        out[1] = src[1];
        src += 2;
        out += 2;
    }
    return out;
}

/* the first command at or after p with opcode `op`; the one after it */
static u32 *dl_find(u32 *p, u32 op, const Blk *b, s32 i) {
    u32 w;

    do {
        BLK(b, i);
        w = p[0];
        p += 2;
    } while (OP(w) != op);
    return p;
}

/* the animation's patches (tex_anim_find's) to a group's display list at
   p: the first G_SETTIMG gets the frame's texture, and when it blends the
   next G_SETTIMG the next frame's and the G_SETPRIMCOLOR after that the
   fraction (its lodfrac byte) */
static void dl_patch(u32 *p, const Blk *b) {
    p = dl_find(p, G_SETTIMG, b, 0);
    BLK(b, 1);
    if (D_803C3240 != 0) {
        BLK(b, 2);
        p[-1] = D_803C3240;
    }
    BLK(b, 3);
    if (D_803C324A == 0) {
        return;
    }
    BLK(b, 4);
    p = dl_find(p, G_SETTIMG, b, 5);
    BLK(b, 6);
    if (D_803C3244 != 0) {
        BLK(b, 7);
        p[-1] = D_803C3244;
    }
    BLK(b, 8);
    p = dl_find(p, G_SETPRIMCOLOR, b, 9);
    BLK(b, 10);
    p[-2] |= D_803C324B;
}

/* (802A51FC) copy the display list [h + from, h + to) to *out, with the
   animation's patches if `patch` */
static u32 *dl_copy(LevelHeader *h, u32 from, u32 to, u32 *out, s32 patch) {
    static const Blk copy[] = { B(802A5218), B(802A5220) };
    static const Blk patches[] = {
        B(802A5258), B(802A5274), B(802A527C), B(802A5280), B(802A529C), B(802A52A0),
        B(802A52BC), B(802A52D0), B(802A52D4), B(802A52E4), B(802A5300),
    };
    u32 *start = out;

    ENGINE_BLK(802A51FC);
    out = dl_append(h, from, to, out, copy);
    ENGINE_BLK(802A5234);
    if (patch) {
        ENGINE_BLK(802A523C);
        dl_patch(start, patches);
    }
    ENGINE_BLK(802A5320);
    return out;
}

/* whether `cell` is in the visible list */
static s32 cell_visible(s32 cell, const Blk *b) {
    const s16 *p = D_803C30A8;
    s32 v;

    for (;;) {
        BLK(b, 0);
        v = *p++;
        if (v == -1) {
            return 0;
        }
        BLK(b, 1);
        if (v >= cell) {
            break;
        }
    }
    BLK(b, 2);
    return v == cell;
}

/* whether any of span s's cells is in the visible list */
static s32 span_visible(const TerrainSpan *s, const Blk *b) {
    const s16 *v;

    for (v = D_803C30A8;; v++) {
        s32 cell = *v, n;
        const s32 *c;

        BLK(b, 0);
        if (cell == -1) {
            return 0;
        }
        BLK(b, 1);
        for (c = s->cells, n = s->n;; c++, n--) {
            BLK(b, 2);
            if (n == 0) {
                break;
            }
            BLK(b, 3);
            if (*c == cell) {
                return 1;
            }
            BLK(b, 4);
            if (cell < *c) {
                break;
            }
            BLK(b, 5);
        }
    }
}

/* whether `key` is one of the keys drawn [D_803C2EB0, drawn) (ascending) */
static s32 key_drawn(s32 key, const s16 *drawn) {
    const s16 *k;

    for (k = D_803C2EB0;;) {
        ENGINE_BLK(802A53A8);
        if (k == drawn) {
            return 0;
        }
        ENGINE_BLK(802A53B0);
        if (key < *k) {
            return 0;
        }
        ENGINE_BLK(802A53C0);
        if (*k++ == key) {
            return 1;
        }
        ENGINE_BLK(802A53C8);
    }
}

/* (802A5334) the spans of [s, end) not drawn yet whose cells are visible;
   a run of spans with one key gets the texture setup (and the animation's
   patches) once */
static u32 *spans_rest(LevelHeader *h, TerrainSpan *s, TerrainSpan *end, s16 *drawn, u32 *out) {
    static const Blk visible[] = {
        B(802A5358), B(802A5368), B(802A5374), B(802A537C), B(802A5388), B(802A5390),
    };
    static const Blk copy[] = { B(802A5408), B(802A5410) };
    static const Blk patches[] = {
        B(802A5444), B(802A5460), B(802A5468), B(802A546C), B_NONE, B(802A5480),
        B(802A549C), B(802A54B0), B_NONE, B(802A54B4), B(802A54D0),
    };
    s32 last = -2;      /* no key */

    ENGINE_BLK(802A5334);
    for (;; s = SPAN_NEXT(s)) {
        u32 *start = out;
        s32 same, patch;

        ENGINE_BLK(802A5340);
        if (s == end) {
            break;
        }
        ENGINE_BLK(802A5348);
        if (!span_visible(s, visible)) {
            ENGINE_BLK(802A54E4);
            continue;
        }
        ENGINE_BLK(802A539C);
        if (key_drawn(s->key, drawn)) {
            ENGINE_BLK(802A54E4);
            continue;
        }
        ENGINE_BLK(802A53D0);
        same = s->key == last;
        if (same) {
            ENGINE_BLK(802A53D8);
        } else {
            ENGINE_BLK(802A53E8);
        }
        ENGINE_BLK(802A53F4);
        last = s->key;
        patch = tex_anim_find(h, last);
        ENGINE_BLK(802A5400);
        out = dl_append(h, same ? s->dlNoTex : s->dl, s->dlEnd, out, copy);
        ENGINE_BLK(802A5424);
        if (patch) {
            ENGINE_BLK(802A542C);
            if (!same) {
                ENGINE_BLK(802A5434);
                dl_patch(start, patches);
            }
        }
        ENGINE_BLK(802A54E4);
    }
    ENGINE_BLK(802A54F4);
    return out;
}

/* (802A4E4C) one display list: the header's prefix, then for each visible
   group of [g, gEnd) its list and those of the following visible groups
   and the visible spans of [spans, sEnd) with the same key (the texture
   setup once), then the other visible spans */
static void terrain_dl(LevelHeader *h, u32 *out, TerrainGroup *g, TerrainGroup *gEnd,
                       TerrainSpan *spans, TerrainSpan *sEnd) {
    static const Blk prefix[] = { B(802A4E70), B(802A4E78) };
    static const Blk first[] = { B(802A4EA8), B(802A4EBC), B(802A4EC4) };
    static const Blk more[] = { B(802A4F0C), B(802A4F20), B(802A4F2C) };
    static const Blk visible[] = {
        B(802A4F80), B(802A4F90), B(802A4F9C), B(802A4FA4), B(802A4FB0), B(802A4FB8),
    };
    s16 *drawn = D_803C2EB0;

    ENGINE_BLK(802A4E4C);
    out = dl_append(h, h->displayLists[DL_PREFIX], h->displayLists[DL_PREFIX + 1], out, prefix);
    ENGINE_BLK(802A4E8C);
    for (;;) {
        TerrainSpan *s;
        s32 key, patch;

        ENGINE_BLK(802A4E94);
        if (g == gEnd) {
            break;
        }
        ENGINE_BLK(802A4E9C);
        if (!cell_visible(g->cell, first)) {
            ENGINE_BLK(802A4FE8);
            g++;
            continue;
        }
        ENGINE_BLK(802A4ECC);
        key = g->key;
        *drawn++ = key;
        patch = tex_anim_find(h, key);
        ENGINE_BLK(802A4EDC);
        out = dl_copy(h, g->dl, g->dlEnd, out, patch);
        /* the following groups with the same key, without the texture
           setup */
        for (;;) {
            ENGINE_BLK(802A4EE8);
            g++;
            if (g == gEnd) {
                break;
            }
            ENGINE_BLK(802A4EF4);
            if (g->key != key) {
                break;
            }
            ENGINE_BLK(802A4F00);
            if (cell_visible(g->cell, more)) {
                ENGINE_BLK(802A4F34);
                out = dl_copy(h, g->dlNoTex, g->dlEnd, out, 0);
                ENGINE_BLK(802A4F44);
            }
        }
        /* the spans with that key (ascending by key) */
        ENGINE_BLK(802A4F4C);
        for (s = spans;; s = SPAN_NEXT(s)) {
            ENGINE_BLK(802A4F50);
            if (s == sEnd) {
                break;
            }
            ENGINE_BLK(802A4F58);
            if (key < s->key) {
                break;
            }
            ENGINE_BLK(802A4F68);
            if (key == s->key) {
                ENGINE_BLK(802A4F70);
                if (span_visible(s, visible)) {
                    ENGINE_BLK(802A4FC4);
                    out = dl_copy(h, s->dlNoTex, s->dlEnd, out, 0);
                }
            }
            ENGINE_BLK(802A4FD4);
        }
    }
    ENGINE_BLK(802A4FF0);
    out = spans_rest(h, spans, sEnd, drawn, out);
    ENGINE_BLK(802A4FF8);
    out[0] = G_ENDDL_W0;
    out[1] = 0;
}

/* (802A4DE8) a G_DL to each display list of [p, end) (each running to its
   G_ENDDL), then G_ENDDL */
static void dl_calls(u32 *out, u32 *p, u32 *end) {
    ENGINE_BLK(802A4DE8);
    for (;;) {
        ENGINE_BLK(802A4DF8);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A4E00);
        out[0] = G_DL_W0;
        out[1] = (u32)p - 0x80000000;   /* KSEG0 to physical */
        out += 2;
        do {
            ENGINE_BLK(802A4E14);
            p += 2;
        } while (p[-2] != G_ENDDL_W0);
        ENGINE_BLK(802A4E24);
    }
    ENGINE_BLK(802A4E2C);
    out[0] = G_ENDDL_W0;
    out[1] = 0;
}

/* func_802A4CDC (00000.c's, every frame): the terrain's four display lists
   and the list of the level's other display lists, then the texture
   animations' step */
void func_802A4CDC(Gfx *dl0, Gfx *dl1, Gfx *dl2, Gfx *dl3, Gfx *calls) {
    static const Blk after[4] = { B(802A4D34), B(802A4D5C), B(802A4D84), B(802A4DAC) };
    LevelHeader *h = D_80358074;
    Gfx *dls[4];
    s32 i;

    dls[0] = dl0;
    dls[1] = dl1;
    dls[2] = dl2;
    dls[3] = dl3;
    ENGINE_BLK(802A4CDC);
    for (i = 0; i < 4; i++) {
        terrain_dl(h, (u32 *)dls[i],
                   (TerrainGroup *)AT(h, h->unkA0[TERRAIN_GROUPS + i]),
                   (TerrainGroup *)AT(h, h->unkA0[TERRAIN_GROUPS + i + 1]),
                   (TerrainSpan *)AT(h, h->unkA0[TERRAIN_SPANS + i]),
                   (TerrainSpan *)AT(h, h->unkA0[TERRAIN_SPANS + i + 1]));
        BLK(after, i);
    }
    dl_calls((u32 *)calls, (u32 *)AT(h, h->displayLists[DL_OTHERS]),
             (u32 *)AT(h, h->displayLists[DL_OTHERS + 1]));
    ENGINE_BLK(802A4DC4);
    tex_anims_tick(h);
    ENGINE_BLK(802A4DCC);
}
