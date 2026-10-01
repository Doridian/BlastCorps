/* hd_code 5FD50: which terrain cells can be seen, and the display lists
   of what can.

   The visibility test walks a quadtree over the level's grid (the
   LevelHeader's unk0 cells, unk4 cell size), a little each frame: a node
   whose bounding box (the cells' lowest and highest points, from
   LevelHeader.unk48's per-cell s16 pairs) the RSP finds on screen is
   split in four, down to single cells, which are collected in D_803C3178.
   When the stack of nodes runs empty the collected cells become the
   sorted list D_803C30A8 (-1 terminated) that the C and the display list
   builder read, and the walk starts again from the root.

   The RSP task (func_802A4B0C) runs the microcode at D_802E77B0 on the
   eight corners of the box; its first output word is G_ENDDL (0xE8...
   in that microcode's numbering) when nothing was drawn. */
#include "engine_b.h"
#include "engine_b_types.h"
#include "game/sched.h"

typedef struct QuadNode {
    s16 x, z, w, h;     /* in cells */
} QuadNode;

typedef struct TerrainGroup {   /* LevelHeader.unkA0.. ranges, 0x14 bytes */
    /* 0x00 */ u32 dl;          /* offsets from the header: the group's display list, */
    /* 0x04 */ u32 dlNoTex;     /* the same after its texture setup, */
    /* 0x08 */ u32 dlEnd;       /* and its end */
    /* 0x0C */ s32 key;         /* the texture animation (LevelHeader.unk2C's) */
    /* 0x10 */ s32 cell;
} TerrainGroup;

typedef struct TerrainSpan {    /* LevelHeader.unkB0.. ranges: a group over several cells */
    /* 0x00 */ u32 dl, dlNoTex, dlEnd;
    /* 0x0C */ s32 key;
    /* 0x10 */ s32 n;
    /* 0x14 */ s32 cells[1];    /* n of them, ascending */
} TerrainSpan;

typedef struct TexAnim {        /* LevelHeader.animTextures: 8 + 4 n bytes */
    /* 0x00 */ s32 key;
    /* 0x04 */ u8 n;            /* frames */
    /* 0x05 */ u8 cur;
    /* 0x06 */ u8 blend;        /* blend into the next frame */
    /* 0x07 */ u8 lodFrac;      /* how far, 0..255 */
    /* 0x08 */ u16 period;      /* frames per frame */
    /* 0x0A */ u16 count;
    /* 0x0C */ u32 tex1[1];     /* frames 1 .. n - 1 (frame 0 is the display list's own) */
} TexAnim;
#define TEXANIM_TEX(a, i) (*(u32 *)((u8 *)(a) + 8 + (i) * 4))

extern u8 D_803BE740[0x40];     /* the visibility task (a SchedTask: its last 0x20 bytes
                                   overlap the DRAM stack after it) */
extern u64 D_803BE780[0x80];    /* its DRAM stack */
extern u32 D_803BEB80[0x1000];  /* its output */
extern u32 D_803C2B80;          /* its output's size */
extern QuadNode *PTR32 D_803C2B88;      /* the node stack's top */
extern QuadNode D_803C2B90[100];
extern s16 D_803C2EB0[0xFC];    /* the keys drawn so far (func_802A4E4C) */
extern s16 D_803C30A8[100];     /* the visible cells, ascending, -1 at the end */
extern s16 *PTR32 D_803C3170;   /* the end of ... */
extern s16 D_803C3178[100];     /* ... the cells found visible in this walk */
extern u32 D_803C3240;          /* the current animation's texture */
extern u32 D_803C3244;          /* ... the one it blends into */
extern u16 D_803C3248;          /* frames before the next step of the walk */
extern u8 D_803C324A;           /* the animation blends */
extern u8 D_803C324B;           /* ... by this much */

extern u8 D_802E6820[], D_802E68F0[], D_802E77B0[], D_8030EE60[];
extern u64 D_8036AFB0[];
extern OSMesgQueue D_803153D8;

extern SchedClient D_803156D8;
void func_80285110(u32 msg);

#define G_SETTIMG_OP 0xFD
#define G_SETPRIMCOLOR_OP 0xFA

/* ---- the quadtree walk --------------------------------------------------- */

/* func_802A4510 / func_802A45D4: every cell visible, the walk restarting
   in `wait` frames */
static void vis_reset(u16 wait) {
    s32 n = D_803BE714 * D_803BE716;
    s32 i;
    s16 *cand = D_803C3178;
    s16 *vis = D_803C30A8;

    D_803C3248 = wait;
    D_803C2B88 = D_803C2B90;
    for (i = 0;; i++) {
        ENG_COST(2);
        if (n == i) {
            break;
        }
        ENG_COST(6);
        *cand++ = i;
        *vis++ = i;
    }
    D_803C3170 = cand;
    *vis = -1;
}

/* func_802A4510 (the level loader's) */
void eng_vis_reset(void) {
    ENG_COST(29);
    vis_reset(0);
    ENG_COST(12);
}

/* func_802A45D4 (the C's) */
void func_802A45D4(s32 wait) {
    ENG_COST(25);
    vis_reset(wait);
    ENG_COST(9);
}

/* func_802A49A8: the lowest and highest points of the cells under `node` */
static void node_bounds(LevelHeader *h, const QuadNode *node, s32 *lo, s32 *hi) {
    s32 width = h->unk0[0];
    s16 *row = (s16 *)((u8 *)h + h->unk48) + (node->z * width + node->x) * 2;
    s32 min = 0x7FFF, max = -0x8000;
    s32 rows = node->h;

    ENG_COST(16);
    do {
        s16 *p = row;
        s32 cols = node->w;

        ENG_COST(1);
        do {
            ENG_COST(5);
            if (p[0] < min) {
                ENG_COST(1);
                min = p[0];
            }
            ENG_COST(3);
            if (max < p[1]) {
                ENG_COST(1);
                max = p[1];
            }
            ENG_COST(3);
            p += 2;
        } while (--cols != 0);
        ENG_COST(4);
        row += width * 2;
    } while (--rows != 0);
    ENG_COST(8);
    *lo = min;
    *hi = max;
}

/* func_802A4A50: the box's eight corners */
static void node_box(LevelHeader *h, const QuadNode *node, Vtx *v, s32 lo, s32 hi) {
    s16 x0 = node->x * h->unk4[0], x1 = (node->x + node->w) * h->unk4[0];
    s16 z0 = node->z * h->unk4[1], z1 = (node->z + node->h) * h->unk4[1];

    ENG_COST(47);
    v[0].v.ob[0] = x0; v[0].v.ob[1] = lo; v[0].v.ob[2] = z0;
    v[1].v.ob[0] = x1; v[1].v.ob[1] = lo; v[1].v.ob[2] = z0;
    v[2].v.ob[0] = x1; v[2].v.ob[1] = lo; v[2].v.ob[2] = z1;
    v[3].v.ob[0] = x0; v[3].v.ob[1] = lo; v[3].v.ob[2] = z1;
    v[4].v.ob[0] = x0; v[4].v.ob[1] = hi; v[4].v.ob[2] = z0;
    v[5].v.ob[0] = x1; v[5].v.ob[1] = hi; v[5].v.ob[2] = z0;
    v[6].v.ob[0] = x1; v[6].v.ob[1] = hi; v[6].v.ob[2] = z1;
    v[7].v.ob[0] = x0; v[7].v.ob[1] = hi; v[7].v.ob[2] = z1;
}

/* func_802A4B0C: run the RSP's test of the box in `vtx` (drawn by the
   display list `dl`, `size` bytes); whether any of it is on screen.  A
   1 x 1 grid always is. */
static s32 box_visible(Gfx *dl, Vtx *vtx, s32 size) {
    SchedTask *t = (SchedTask *)D_803BE740;

    ENG_COST(15);
    if (D_803BE714 == 1) {
        ENG_COST(5);
        if (D_803BE716 == 1) {
            ENG_COST(2 + 12);
            return 1;
        }
    }
    ENG_COST(58);
    t->list.t.type = M_GFXTASK;
    t->list.t.flags = 0;
    t->list.t.ucode_boot = (u64 *)D_802E6820;
    t->list.t.ucode_boot_size = D_802E68F0 - D_802E6820;
    t->list.t.ucode = (u64 *)D_802E77B0;
    t->list.t.ucode_size = 0x1000;
    t->list.t.ucode_data = (u64 *)D_8030EE60;
    t->list.t.ucode_data_size = 0x800;
    t->list.t.dram_stack = D_803BE780;
    t->list.t.dram_stack_size = 0x400;
    t->list.t.output_buff = (u64 *)D_803BEB80;
    t->list.t.output_buff_size = (u64 *)&D_803C2B80;
    t->list.t.yield_data_ptr = D_8036AFB0;
    t->list.t.yield_data_size = 0x900;
    t->flags = 1;
    t->msgQ = &D_803153D8;
    t->msg = (OSMesg)0x4D3;
    t->framebuffer = NULL;
    t->list.t.data_ptr = (u64 *)dl;
    t->list.t.data_size = size;
    t->client = &D_803156D8;
    osWritebackDCache(vtx, 0x80);
    ENG_COST(3);
    osWritebackDCache(t, 0x40);
    ENG_COST(4);
    osInvalDCache(D_803BEB80, 0x20);
    ENG_COST(7);
    osSendMesg(&D_80315440.interruptQ, (OSMesg)t, OS_MESG_BLOCK);
    ENG_COST(2);
    func_80285110(0x4D3);
    ENG_COST(7);
    if (D_803BEB80[0] == 0xE8000000) {
        ENG_COST(1 + 12);
        return 0;
    }
    ENG_COST(2 + 12);
    return 1;
}

/* func_802A484C: test `node`; split a visible one in four onto the
   stack, or keep a visible cell. */
static void node_visit(LevelHeader *h, const QuadNode *node, Gfx *dl, Vtx *vtx, s32 size,
                       QuadNode **top, s16 **cells) {
    s32 lo, hi;
    s32 x = node->x, z = node->z, w = node->w, hh = node->h;
    s32 w0, h0;
    QuadNode *t = *top;

    ENG_COST(11);
    node_bounds(h, node, &lo, &hi);
    ENG_COST(2);
    node_box(h, node, vtx, lo, hi);
    ENG_COST(2);
    if (box_visible(dl, vtx, size)) {
        ENG_COST(2 + 3);
        if (w == 1) {
            ENG_COST(2);
            if (hh == 1) {
                ENG_COST(6);
                *(*cells)++ = z * h->unk0[0] + x;
                goto done;
            }
        }
        ENG_COST(7);
        w0 = (u32)w >> 1;
        h0 = (u32)hh >> 1;
        if (w0 != 0) {
            ENG_COST(2);
            if (h0 != 0) {
                ENG_COST(5);
                t->x = x; t->z = z; t->w = w0; t->h = h0;
                t++;
            }
        }
        ENG_COST(4);
        if (w - w0 != 0) {
            ENG_COST(2);
            if (h0 != 0) {
                ENG_COST(5);
                t->x = x + w0; t->z = z; t->w = w - w0; t->h = h0;
                t++;
            }
        }
        ENG_COST(5);
        if (w0 != 0) {
            ENG_COST(2);
            if (hh - h0 != 0) {
                ENG_COST(5);
                t->x = x; t->z = z + h0; t->w = w0; t->h = hh - h0;
                t++;
            }
        }
        ENG_COST(4);
        if (w - w0 != 0) {
            ENG_COST(2);
            if (hh - h0 != 0) {
                ENG_COST(5);
                t->x = x + w0; t->z = z + h0; t->w = w - w0; t->h = hh - h0;
                t++;
            }
        }
    } else {
        ENG_COST(2);
    }
done:
    ENG_COST(11);
    *top = t;
}

/* func_802A470C: one step of the walk */
static void vis_step(LevelHeader *h, Gfx *dl, Vtx *vtx, s32 size) {
    QuadNode *top = D_803C2B88;
    QuadNode node;
    s16 *cells;

    ENG_COST(9);
    if (top == D_803C2B90) {
        /* the walk is done: its cells become the visible list, sorted */
        s16 *c = D_803C3178;
        s16 *end = D_803C3170;
        s16 *sorted = D_803C30A8;   /* its end */

        ENG_COST(14);
        top->x = 0;
        top->z = 0;
        top->w = h->unk0[0];
        top->h = h->unk0[1];
        top++;
        for (;;) {
            s16 cell, *p;

            ENG_COST(2);
            if (end == c) {
                break;
            }
            ENG_COST(3);
            cell = *c;
            for (p = D_803C30A8;; p++) {
                ENG_COST(2);
                if (p == sorted) {
                    ENG_COST(4);
                    *sorted = cell;
                    break;
                }
                ENG_COST(4);
                if (cell < *p) {
                    /* insert it here, moving the rest up */
                    s16 moved = *p;

                    ENG_COST(1);
                    *p = cell;
                    do {
                        s16 next = p[1];

                        ENG_COST(5);
                        p[1] = moved;
                        p++;
                        moved = next;
                    } while (p != sorted);
                    ENG_COST(3);
                    break;
                }
                ENG_COST(2);
            }
            c++;
            sorted++;
        }
        ENG_COST(7);
        *sorted = -1;
        D_803C3170 = D_803C3178;
    }
    ENG_COST(14);
    top--;
    node = *top;
    cells = D_803C3170;
    node_visit(h, &node, dl, vtx, size, &top, &cells);
    ENG_COST(10);
    D_803C3170 = cells;
    D_803C2B88 = top;
}

/* func_802A467C (the C's, every frame): a step of the walk, unless it is
   waiting */
void func_802A467C(LevelHeader *h, Gfx *dl, Vtx *vtx, s32 size) {
    ENG_COST(17);
    if (D_803C3248 == 0) {
        ENG_COST(2);
        vis_step(h, dl, vtx, size);
        ENG_COST(2);
    } else {
        ENG_COST(2);
        D_803C3248--;
    }
    ENG_COST(13);
}

/* ---- the terrain's display lists ---------------------------------------- */

/* func_802A5020: advance every texture animation by a frame */
static void tex_anims_tick(LevelHeader *h) {
    TexAnim *a = (TexAnim *)((u8 *)h + h->animTextures);
    TexAnim *end = (TexAnim *)((u8 *)h + h->terrain);

    ENG_COST(6);
    for (;;) {
        u32 count;

        ENG_COST(2);
        if (a == end) {
            break;
        }
        ENG_COST(5);
        count = a->count + 1;
        if (a->period == count) {
            u32 next;

            ENG_COST(5);
            next = a->cur + 1;
            if (a->n == next) {
                ENG_COST(1);
                next = 0;
            }
            ENG_COST(2);
            a->cur = next;
            count = 0;
        }
        ENG_COST(3);
        if (a->blend != 0) {
            ENG_COST(12);
            a->lodFrac = (0xFF * count) / a->period;
        }
        ENG_COST(6);
        a->count = count;
        a = (TexAnim *)((u8 *)a + 8 + a->n * 4);
    }
    ENG_COST(4);
}

/* func_802A50DC: look up animation `key` for the patches below; whether
   there is anything to patch */
static s32 tex_anim_find(LevelHeader *h, s32 key) {
    TexAnim *a = (TexAnim *)((u8 *)h + h->animTextures);
    TexAnim *end = (TexAnim *)((u8 *)h + h->terrain);
    u32 cur;

    ENG_COST(10);
    for (;;) {
        ENG_COST(2);
        if (a == end) {
            ENG_COST(1 + 7);
            return 0;
        }
        ENG_COST(3);
        if (a->key == key) {
            break;
        }
        ENG_COST(5);
        a = (TexAnim *)((u8 *)a + 8 + a->n * 4);
    }
    ENG_COST(3);
    cur = a->cur;
    if (cur != 0) {
        ENG_COST(8);
        D_803C3240 = TEXANIM_TEX(a, cur);
    } else {
        ENG_COST(3);
        D_803C3240 = 0;
    }
    ENG_COST(5);
    D_803C324A = a->blend;
    if (a->blend == 0) {
        ENG_COST(2);
        if (cur == 0) {
            ENG_COST(1 + 7);
            return 0;
        }
        ENG_COST(2 + 7);
        return 1;
    }
    ENG_COST(4);
    cur++;
    if (a->n == cur) {
        ENG_COST(4);
        D_803C3244 = 0;
    } else {
        ENG_COST(7);
        D_803C3244 = TEXANIM_TEX(a, cur);
    }
    ENG_COST(6 + 7);
    D_803C324B = a->lodFrac;
    return 1;
}

/* func_802A51FC: copy the display list [h + from, h + to) to *out, with
   the animation's patches when `patch` */
static void dl_copy(LevelHeader *h, u32 from, u32 to, u32 **out, s32 patch) {
    u32 *src = (u32 *)((u8 *)h + from);
    u32 *end = (u32 *)((u8 *)h + to);
    u32 *start = *out, *dst = *out, *p;
    u32 w;

    ENG_COST(7);
    for (;;) {
        ENG_COST(2);
        if (src == end) {
            break;
        }
        ENG_COST(5);
        dst[0] = src[0];
        dst[1] = src[1];
        src += 2;
        dst += 2;
    }
    *out = dst;
    ENG_COST(2);
    if (!patch) {
        ENG_COST(5);
        return;
    }
    ENG_COST(7);
    p = start;
    do {
        ENG_COST(7);
        w = p[0];
        p += 2;
    } while ((w & 0xFF000000) >> 24 != G_SETTIMG_OP);
    ENG_COST(2);
    if (D_803C3240 != 0) {
        ENG_COST(1);
        p[-1] = D_803C3240;
    }
    ENG_COST(7);
    if (D_803C324A == 0) {
        ENG_COST(5);
        return;
    }
    ENG_COST(1);
    do {
        ENG_COST(7);
        w = p[0];
        p += 2;
    } while ((w & 0xFF000000) >> 24 != G_SETTIMG_OP);
    ENG_COST(5);
    if (D_803C3244 != 0) {
        ENG_COST(1);
        p[-1] = D_803C3244;
    }
    ENG_COST(4);
    do {
        ENG_COST(7);
        w = p[0];
        p += 2;
    } while ((w & 0xFF000000) >> 24 != G_SETPRIMCOLOR_OP);
    ENG_COST(8 + 5);
    p[-2] = w | D_803C324B;
}

/* whether `cell` is in the visible list */
static s32 cell_visible(s32 cell) {
    s16 *p = D_803C30A8;

    for (;;) {
        s32 v;

        ENG_COST(5);
        v = *p++;
        if (v == -1) {
            return 0;
        }
        ENG_COST(2);
        if (v >= cell) {
            ENG_COST(2);
            return v == cell;
        }
    }
}

/* whether any of a span's cells is visible */
static s32 span_visible(TerrainSpan *s) {
    s16 *p = D_803C30A8;

    for (;;) {
        s32 v;
        s32 *c;
        s32 n;

        ENG_COST(4);
        v = *p;
        if (v == -1) {
            return 0;
        }
        ENG_COST(3);
        p++;
        c = s->cells;
        n = s->n;
        for (;;) {
            ENG_COST(2);
            if (n == 0) {
                break;
            }
            ENG_COST(3);
            if (*c == v) {
                return 1;
            }
            ENG_COST(2);
            if (v < *c) {
                break;
            }
            ENG_COST(3);
            c++;
            n--;
        }
    }
}

#define SPAN_NEXT(s) ((TerrainSpan *)((u8 *)(s) + 0x14 + (s)->n * 4))

/* func_802A5334: the spans not drawn yet whose cells are visible */
static void spans_rest(LevelHeader *h, TerrainSpan *s, TerrainSpan *end, s16 *drawn,
                       u32 **out) {
    s32 last = -2;

    ENG_COST(3);
    for (;;) {
        s16 *k;
        u32 *patchAt;
        u32 from;
        s32 patch;

        ENG_COST(2);
        if (s == end) {
            break;
        }
        ENG_COST(4);
        if (!span_visible(s)) {
            goto next;
        }
        ENG_COST(3);
        for (k = D_803C2EB0;; ) {
            ENG_COST(2);
            if (k == drawn) {
                break;
            }
            ENG_COST(4);
            if (s->key < *k) {
                break;
            }
            ENG_COST(2);
            if (*k++ == s->key) {
                goto next;
            }
            ENG_COST(2);
        }
        ENG_COST(2);
        if (s->key == last) {
            ENG_COST(4);
            from = s->dlNoTex;
            patchAt = NULL;
        } else {
            ENG_COST(3);
            from = s->dl;
            patchAt = *out;
        }
        ENG_COST(3);
        last = s->key;
        patch = tex_anim_find(h, last);
        ENG_COST(2);
        {
            u32 *src = (u32 *)((u8 *)h + from);
            u32 *srcEnd = (u32 *)((u8 *)h + s->dlEnd);
            u32 *dst = *out;

            for (;;) {
                ENG_COST(2);
                if (src == srcEnd) {
                    break;
                }
                ENG_COST(5);
                dst[0] = src[0];
                dst[1] = src[1];
                src += 2;
                dst += 2;
            }
            *out = dst;
        }
        ENG_COST(2);
        if (!patch) {
            goto next;
        }
        ENG_COST(2);
        if (patchAt == NULL) {
            goto next;
        }
        {
            u32 *p = patchAt;
            u32 w;

            ENG_COST(4);
            do {
                ENG_COST(7);
                w = p[0];
                p += 2;
            } while ((w & 0xFF000000) >> 24 != G_SETTIMG_OP);
            ENG_COST(2);
            if (D_803C3240 != 0) {
                ENG_COST(1);
                p[-1] = D_803C3240;
            }
            ENG_COST(5);
            if (D_803C324A == 0) {
                goto next;
            }
            do {
                ENG_COST(7);
                w = p[0];
                p += 2;
            } while ((w & 0xFF000000) >> 24 != G_SETTIMG_OP);
            ENG_COST(5);
            if (D_803C3244 != 0) {
                ENG_COST(1);
                p[-1] = D_803C3244;
            }
            do {
                ENG_COST(7);
                w = p[0];
                p += 2;
            } while ((w & 0xFF000000) >> 24 != G_SETPRIMCOLOR_OP);
            ENG_COST(5);
            p[-2] = w | D_803C324B;
        }
    next:
        ENG_COST(4);
        s = SPAN_NEXT(s);
    }
    ENG_COST(4);
}

/* func_802A4E4C: one display list: the header's prefix (unk88..unk8C),
   the visible groups of [g, gEnd) and the spans of [s, sEnd) with the
   same key, then the rest of the visible spans; returns its end. */
static u32 *terrain_dl(LevelHeader *h, u32 *out, TerrainGroup *g, TerrainGroup *gEnd,
                       TerrainSpan *spans, TerrainSpan *sEnd) {
    u32 *src = (u32 *)((u8 *)h + *(u32 *)((u8 *)h + 0x88));
    u32 *srcEnd = (u32 *)((u8 *)h + *(u32 *)((u8 *)h + 0x8C));
    s16 *drawn = D_803C2EB0;

    ENG_COST(9);
    for (;;) {
        ENG_COST(2);
        if (src == srcEnd) {
            break;
        }
        ENG_COST(5);
        out[0] = src[0];
        out[1] = src[1];
        src += 2;
        out += 2;
    }
    ENG_COST(2);
    for (;;) {
        s32 key;
        TerrainSpan *s;

        ENG_COST(2);
        if (g == gEnd) {
            break;
        }
        ENG_COST(3);
        if (!cell_visible(g->cell)) {
            ENG_COST(2);
            g++;
            continue;
        }
        ENG_COST(4);
        key = g->key;
        *drawn++ = key;
        {
            s32 patch = tex_anim_find(h, key);

            ENG_COST(3);
            dl_copy(h, g->dl, g->dlEnd, &out, patch);
        }
        /* the following groups with the same key, without the texture
           setup */
        for (;;) {
            ENG_COST(3);
            g++;
            if (g == gEnd) {
                break;
            }
            ENG_COST(3);
            if (g->key != key) {
                break;
            }
            ENG_COST(3);
            {
                s16 *p = D_803C30A8;
                s32 v;

                for (;;) {
                    ENG_COST(5);
                    v = *p++;
                    if (v == -1) {
                        goto skip;
                    }
                    ENG_COST(3);
                    if (v >= g->cell) {
                        break;
                    }
                }
                ENG_COST(2);
                if (v != g->cell) {
                    goto skip;
                }
            }
            ENG_COST(4);
            dl_copy(h, g->dlNoTex, g->dlEnd, &out, 0);
            ENG_COST(2);
        skip:;
        }
        /* the spans with that key */
        ENG_COST(1);
        for (s = spans;; s = SPAN_NEXT(s)) {
            ENG_COST(2);
            if (s == sEnd) {
                break;
            }
            ENG_COST(4);
            if (key < s->key) {
                break;
            }
            ENG_COST(2);
            if (key == s->key) {
                ENG_COST(4);
                if (span_visible(s)) {
                    ENG_COST(4);
                    dl_copy(h, s->dlNoTex, s->dlEnd, &out, 0);
                }
            }
            ENG_COST(5);
        }
    }
    ENG_COST(2);
    spans_rest(h, spans, sEnd, drawn, &out);
    ENG_COST(10);
    out[0] = 0xB8000000;        /* G_ENDDL */
    out[1] = 0;
    return out + 2;
}

/* func_802A4DE8: a G_DL to each display list of [p, end) (each running to
   its G_ENDDL), then G_ENDDL */
static u32 *dl_calls(u32 *out, u32 *p, u32 *end) {
    ENG_COST(4);
    for (;;) {
        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(5);
        out[0] = 0x06000000;
        out[1] = ENG_PHYS(p);
        out += 2;
        do {
            ENG_COST(4);
            p += 2;
        } while (p[-2] != 0xB8000000);
        ENG_COST(2);
    }
    ENG_COST(8);
    out[0] = 0xB8000000;
    out[1] = 0;
    return out + 2;
}

/* func_802A4CDC (the C's, every frame): the terrain's four display lists
   and the list of the level's other display lists, then the texture
   animations' step */
void func_802A4CDC(Gfx *dl0, Gfx *dl1, Gfx *dl2, Gfx *dl3, Gfx *calls) {
    LevelHeader *h = D_80358074;
    u32 *H = (u32 *)h;

#define AT(off) ((void *)((u8 *)h + H[(off) / 4]))
    ENG_COST(22);
    terrain_dl(h, (u32 *)dl0, AT(0xA0), AT(0xA4), AT(0xB0), AT(0xB4));
    ENG_COST(10);
    terrain_dl(h, (u32 *)dl1, AT(0xA4), AT(0xA8), AT(0xB4), AT(0xB8));
    ENG_COST(10);
    terrain_dl(h, (u32 *)dl2, AT(0xA8), AT(0xAC), AT(0xB8), AT(0xBC));
    ENG_COST(10);
    terrain_dl(h, (u32 *)dl3, AT(0xAC), AT(0xB0), AT(0xBC), AT(0xC0));
    ENG_COST(6);
    dl_calls((u32 *)calls, AT(0x7C), AT(0x80));
    ENG_COST(2);
    tex_anims_tick(h);
    ENG_COST(7);
#undef AT
}
