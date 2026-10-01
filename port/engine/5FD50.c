/*
 * hd_code 5FD50 (us.v11 0x802A4510-0x802A5510): which terrain cells can be
 * seen, and the terrain's display lists, as native C (engine.h).
 *
 * The visibility test walks a quadtree over the level's grid (LevelHeader
 * unk0: cells, unk4: their size), a step a frame: a node whose bounding box
 * (the cells' lowest and highest points, LevelHeader.unk48's s16 pairs) the
 * RSP finds on screen is split in four, down to single cells, which are
 * collected in D_803C3178.  When the stack of nodes runs empty, the cells
 * found become the sorted list D_803C30A8 (-1 at the end) that the C and the
 * display lists here read, and the walk starts again from the root.  The
 * test is an RSP task (802A4B0C) running the microcode at D_802E77B0 over
 * the box's eight corners; its first output word is 0xE8000000 when
 * nothing of them was drawn.
 *
 * Each frame the terrain's four display lists are built from the groups
 * (LevelHeader.unkA0..: one cell each) and spans (.unkB0..: several cells)
 * that are visible, with the texture animations (LevelHeader.animTextures)
 * patched into their G_SETTIMG and G_SETPRIMCOLOR words.
 */
#include "engine.h"
#include "game/game.h"
#include "game/level.h"
#include "game/sched.h"

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

typedef struct TerrainSpan {    /* 0x14 + 4 n bytes */
    /* 0x00 */ u32 dl, dlNoTex, dlEnd;
    /* 0x0C */ s32 key;
    /* 0x10 */ s32 n;
    /* 0x14 */ s32 cells[1];    /* n of them, ascending */
} TerrainSpan;

typedef struct TexAnim {        /* 8 + 4 n bytes */
    /* 0x00 */ s32 key;
    /* 0x04 */ u8 n;            /* frames */
    /* 0x05 */ u8 cur;
    /* 0x06 */ u8 blend;        /* blends into the next frame */
    /* 0x07 */ u8 lodFrac;      /* by this much, 0..255 */
    /* 0x08 */ u16 period;      /* game frames per frame */
    /* 0x0A */ u16 count;
    /* 0x0C */ u32 tex1[1];     /* frames 1 .. n - 1 (frame 0 is the list's own) */
} TexAnim;
#define ANIM_TEX(a, i) (*(u32 *)((u8 *)(a) + 8 + (i) * 4))
#define ANIM_NEXT(a) ((TexAnim *)((u8 *)(a) + 8 + (a)->n * 4))
#define SPAN_NEXT(s) ((TerrainSpan *)((u8 *)(s) + 0x14 + (s)->n * 4))
#define AT(h, off) ((u8 *)(h) + *(u32 *)((u8 *)(h) + (off)))

extern u8 D_803BE740[0x40];     /* the visibility task: a SchedTask whose last
                                   0x20 bytes run into its DRAM stack */
extern u64 D_803BE780[0x80];    /* that stack */
extern u32 D_803BEB80[0x1000];  /* the task's output */
extern u32 D_803C2B80;          /* ... its size */
extern QuadNode *PTR32 D_803C2B88;      /* the node stack's top */
extern QuadNode D_803C2B90[100];
extern s16 D_803C2EB0[0xFC];    /* the keys drawn (802A4E4C) */
extern s16 D_803C30A8[100];     /* the visible cells, ascending, -1 at the end */
extern s16 *PTR32 D_803C3170;   /* the end of ... */
extern s16 D_803C3178[100];     /* ... the cells found visible in this walk */
extern u32 D_803C3240;          /* the animation's texture */
extern u32 D_803C3244;          /* ... the one it blends into */
extern u16 D_803C3248;          /* frames before the walk's next step */
extern u8 D_803C324A;           /* the animation blends */
extern u8 D_803C324B;           /* ... by this much */
extern u8 D_802E6820[], D_802E68F0[], D_802E77B0[], D_8030EE60[];
extern u64 D_8036AFB0[];
extern OSMesgQueue D_803153D8;
extern SchedClient D_803156D8;
void func_80285110(u32 msg);

#define OP(w) (((w) & 0xFF000000) >> 24)
#define G_SETTIMG_OP 0xFD
#define G_SETPRIMCOLOR_OP 0xFA
#define G_ENDDL_W0 0xB8000000

/* ---- the quadtree walk --------------------------------------------------- */

/* func_802A4510 (the level loader's): every cell visible; the walk starts
   at once */
REGS()
void func_802A4510(void) {
    s32 n, i;
    s16 *cand = D_803C3178, *vis = D_803C30A8;

    ENGINE_BLK(802A4510);
    D_803C3248 = 0;
    D_803C2B88 = D_803C2B90;
    n = D_803BE714 * D_803BE716;
    for (i = 0;; i++) {
        ENGINE_BLK(802A4584);
        if (n == i) {
            break;
        }
        ENGINE_BLK(802A458C);
        *cand++ = i;
        *vis++ = i;
    }
    ENGINE_BLK(802A45A4);
    D_803C3170 = cand;
    *vis = -1;
}

/* func_802A45D4 (00000.c's): the same, the walk starting in `wait` frames */
void func_802A45D4(s32 wait) {
    s32 n, i;
    s16 *cand = D_803C3178, *vis = D_803C30A8;

    ENGINE_BLK(802A45D4);
    D_803C3248 = wait;
    D_803C2B88 = D_803C2B90;
    n = D_803BE714 * D_803BE716;
    for (i = 0;; i++) {
        ENGINE_BLK(802A4638);
        if (n == i) {
            break;
        }
        ENGINE_BLK(802A4640);
        *cand++ = i;
        *vis++ = i;
    }
    ENGINE_BLK(802A4658);
    D_803C3170 = cand;
    *vis = -1;
}

/* (802A49A8) the lowest and highest points of the cells under a node */
static void node_bounds(LevelHeader *h, s32 x, s32 z, s32 w, s32 hh, s32 *lo, s32 *hi) {
    s32 width = (s16)h->unk0[0];
    s16 *row = (s16 *)((u8 *)h + h->unk48) + (z * width + x) * 2;
    s32 min = 0x7FFF, max = -0x8000;

    ENGINE_BLK(802A49A8);
    do {
        s16 *p = row;
        s32 cols = w;

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
    } while (--hh != 0);
    ENGINE_BLK(802A4A30);
    *lo = min;
    *hi = max;
}

/* (802A4A50) the box's eight corners */
static void node_box(LevelHeader *h, s32 x, s32 z, s32 w, s32 hh, Vtx *v, s32 lo, s32 hi) {
    s32 sx = (s16)h->unk4[0], sz = (s16)h->unk4[1];
    s16 x0 = x * sx, x1 = (x + w) * sx;
    s16 z0 = z * sz, z1 = (z + hh) * sz;

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
    t->msg = (OSMesg)0x4D3;
    t->framebuffer = NULL;
    t->list.t.data_ptr = (u64 *)dl;
    t->list.t.data_size = size;
    t->msgQ = &D_803153D8;
    t->client = &D_803156D8;
    osWritebackDCache(vtx, 0x80);
    ENGINE_BLK(802A4C44);
    osWritebackDCache(t, 0x40);
    ENGINE_BLK(802A4C50);
    osInvalDCache(D_803BEB80, 0x20);
    ENGINE_BLK(802A4C60);
    osSendMesg(&D_80315440.interruptQ, (OSMesg)t, OS_MESG_BLOCK);
    ENGINE_BLK(802A4C7C);
    func_80285110(0x4D3);
    ENGINE_BLK(802A4C84);
    if (D_803BEB80[0] == 0xE8000000) {
        ENGINE_BLK(802A4CA8);
        ENGINE_BLK(802A4CAC);
        return 0;
    }
    ENGINE_BLK(802A4CA0);
    ENGINE_BLK(802A4CAC);
    return 1;
}

/* (802A484C) test the node; split a visible one in four onto the stack,
   or keep a visible cell */
static void node_visit(LevelHeader *h, QuadNode node, Gfx *dl, Vtx *vtx, s32 size, QuadNode **top,
                       s16 **cells) {
    s32 x = node.x, z = node.z, w = node.w, hh = node.h;
    s32 lo, hi, w0, h0;
    QuadNode *t = *top;

    ENGINE_BLK(802A484C);
    node_bounds(h, x, z, w, hh, &lo, &hi);
    ENGINE_BLK(802A4878);
    node_box(h, x, z, w, hh, vtx, lo, hi);
    ENGINE_BLK(802A4880);
    if (box_visible(dl, vtx, size)) {
        ENGINE_BLK(802A4888);
        ENGINE_BLK(802A4890);
        if (w == 1) {
            ENGINE_BLK(802A489C);
            if (hh == 1) {
                ENGINE_BLK(802A48A4);
                *(*cells)++ = z * (s16)h->unk0[0] + x;
                goto done;
            }
        }
        ENGINE_BLK(802A48BC);
        w0 = (u32)w >> 1;
        h0 = (u32)hh >> 1;
        if (w0 != 0) {
            ENGINE_BLK(802A48D8);
            if (h0 != 0) {
                ENGINE_BLK(802A48E0);
                t->x = x; t->z = z; t->w = w0; t->h = h0;
                t++;
            }
        }
        ENGINE_BLK(802A48F4);
        if (w - w0 != 0) {
            ENGINE_BLK(802A4904);
            if (h0 != 0) {
                ENGINE_BLK(802A490C);
                t->x = x + w0; t->z = z; t->w = w - w0; t->h = h0;
                t++;
            }
        }
        ENGINE_BLK(802A4920);
        if (w0 != 0) {
            ENGINE_BLK(802A4934);
            if (hh - h0 != 0) {
                ENGINE_BLK(802A493C);
                t->x = x; t->z = z + h0; t->w = w0; t->h = hh - h0;
                t++;
            }
        }
        ENGINE_BLK(802A4950);
        if (w - w0 != 0) {
            ENGINE_BLK(802A4960);
            if (hh - h0 != 0) {
                ENGINE_BLK(802A4968);
                t->x = x + w0; t->z = z + h0; t->w = w - w0; t->h = hh - h0;
                t++;
            }
        }
    } else {
        ENGINE_BLK(802A4888);
    }
done:
    ENGINE_BLK(802A497C);
    *top = t;
}

/* (802A470C) one step of the walk */
static void vis_step(LevelHeader *h, Gfx *dl, Vtx *vtx, s32 size) {
    QuadNode *top = D_803C2B88;
    QuadNode node;
    s16 *cells;

    ENGINE_BLK(802A470C);
    if (top == D_803C2B90) {
        /* the walk is done: its cells become the visible list, sorted */
        s16 *c = D_803C3178;
        s16 *end = D_803C3170;
        s16 *sorted = D_803C30A8;   /* (its end) */

        ENGINE_BLK(802A4730);
        top->x = 0;
        top->z = 0;
        top->w = h->unk0[0];
        top->h = h->unk0[1];
        top++;
        for (;;) {
            s16 cell, *p;

            ENGINE_BLK(802A4768);
            if (end == c) {
                break;
            }
            ENGINE_BLK(802A4770);
            cell = *c;
            for (p = D_803C30A8;; p++) {
                ENGINE_BLK(802A477C);
                if (p == sorted) {
                    ENGINE_BLK(802A47C0);
                    *sorted = cell;
                    break;
                }
                ENGINE_BLK(802A4784);
                if (cell < *p) {
                    /* in here, the rest moved up */
                    s16 moved = *p;

                    ENGINE_BLK(802A479C);
                    *p = cell;
                    do {
                        s16 next = p[1];

                        ENGINE_BLK(802A47A0);
                        p[1] = moved;
                        p++;
                        moved = next;
                    } while (p != sorted);
                    ENGINE_BLK(802A47B4);
                    break;
                }
                ENGINE_BLK(802A4794);
            }
            c++;
            sorted++;
        }
        ENGINE_BLK(802A47D0);
        *sorted = -1;
        D_803C3170 = D_803C3178;
    }
    ENGINE_BLK(802A47EC);
    top--;
    node = *top;
    cells = D_803C3170;
    node_visit(h, node, dl, vtx, size, &top, &cells);
    ENGINE_BLK(802A4824);
    D_803C3170 = cells;
    D_803C2B88 = top;
}

/* func_802A467C (00000.c's, every frame): a step of the walk, unless it is
   waiting */
void func_802A467C(LevelHeader *h, Gfx *dl, Vtx *vtx, s32 size) {
    ENGINE_BLK(802A467C);
    if (D_803C3248 == 0) {
        ENGINE_BLK(802A46C0);
        vis_step(h, dl, vtx, size);
        ENGINE_BLK(802A46C8);
    } else {
        ENGINE_BLK(802A46D0);
        D_803C3248--;
    }
    ENGINE_BLK(802A46D8);
}

/* ---- the terrain's display lists ----------------------------------------- */

/* (802A5020) every texture animation a frame on */
static void tex_anims_tick(LevelHeader *h) {
    TexAnim *a = (TexAnim *)((u8 *)h + h->animTextures);
    TexAnim *end = (TexAnim *)((u8 *)h + h->terrain);

    ENGINE_BLK(802A5020);
    for (;;) {
        u32 count;

        ENGINE_BLK(802A5038);
        if (a == end) {
            break;
        }
        ENGINE_BLK(802A5040);
        count = a->count + 1;
        if (a->period == count) {
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
            a->lodFrac = (0xFF * count) / a->period;
        }
        ENGINE_BLK(802A50B4);
        a->count = count;
        a = ANIM_NEXT(a);
    }
    ENGINE_BLK(802A50CC);
}

/* (802A50DC) animation `key`'s textures for the patches below; whether
   there is anything to patch */
static s32 tex_anim_find(LevelHeader *h, s32 key) {
    TexAnim *a = (TexAnim *)((u8 *)h + h->animTextures);
    TexAnim *end = (TexAnim *)((u8 *)h + h->terrain);
    u32 cur;

    ENGINE_BLK(802A50DC);
    for (;;) {
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
        a = ANIM_NEXT(a);
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
        ENGINE_BLK(802A5178);
        if (cur == 0) {
            ENGINE_BLK(802A51DC);
            ENGINE_BLK(802A51E0);
            return 0;
        }
        ENGINE_BLK(802A5180);
        ENGINE_BLK(802A51E0);
        return 1;
    }
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
    ENGINE_BLK(802A51E0);
    return 1;
}

/* (802A51FC) copy the display list [h + from, h + to) to *out, with the
   animation's patches if `patch`: the first G_SETTIMG gets the frame's
   texture, and when it blends the next G_SETTIMG the next frame's and the
   G_SETPRIMCOLOR after it the fraction */
static void dl_copy(LevelHeader *h, u32 from, u32 to, u32 **out, s32 patch) {
    u32 *src = (u32 *)((u8 *)h + from);
    u32 *end = (u32 *)((u8 *)h + to);
    u32 *start = *out, *dst = *out, *p, w;

    ENGINE_BLK(802A51FC);
    for (;;) {
        ENGINE_BLK(802A5218);
        if (src == end) {
            break;
        }
        ENGINE_BLK(802A5220);
        dst[0] = src[0];
        dst[1] = src[1];
        src += 2;
        dst += 2;
    }
    *out = dst;
    ENGINE_BLK(802A5234);
    if (!patch) {
        ENGINE_BLK(802A5320);
        return;
    }
    ENGINE_BLK(802A523C);
    p = start;
    do {
        ENGINE_BLK(802A5258);
        w = p[0];
        p += 2;
    } while (OP(w) != G_SETTIMG_OP);
    ENGINE_BLK(802A5274);
    if (D_803C3240 != 0) {
        ENGINE_BLK(802A527C);
        p[-1] = D_803C3240;
    }
    ENGINE_BLK(802A5280);
    if (D_803C324A == 0) {
        ENGINE_BLK(802A5320);
        return;
    }
    ENGINE_BLK(802A529C);
    do {
        ENGINE_BLK(802A52A0);
        w = p[0];
        p += 2;
    } while (OP(w) != G_SETTIMG_OP);
    ENGINE_BLK(802A52BC);
    if (D_803C3244 != 0) {
        ENGINE_BLK(802A52D0);
        p[-1] = D_803C3244;
    }
    ENGINE_BLK(802A52D4);
    do {
        ENGINE_BLK(802A52E4);
        w = p[0];
        p += 2;
    } while (OP(w) != G_SETPRIMCOLOR_OP);
    ENGINE_BLK(802A5300);
    p[-2] = w | D_803C324B;
    ENGINE_BLK(802A5320);
}

/* (802A5334) the spans of [s, end) not drawn yet whose cells are visible */
static void spans_rest(LevelHeader *h, TerrainSpan *s, TerrainSpan *end, s16 *drawn, u32 **out) {
    s32 last = -2;

    ENGINE_BLK(802A5334);
    for (;;) {
        s16 *v, *k;
        u32 *patchAt;
        u32 from;
        s32 patch;

        ENGINE_BLK(802A5340);
        if (s == end) {
            break;
        }
        ENGINE_BLK(802A5348);
        /* any of its cells visible? */
        for (v = D_803C30A8;;) {
            s32 cell, n;
            s32 *c;

            ENGINE_BLK(802A5358);
            cell = *v;
            if (cell == -1) {
                goto next;
            }
            ENGINE_BLK(802A5368);
            v++;
            c = s->cells;
            n = s->n;
            for (;;) {
                ENGINE_BLK(802A5374);
                if (n == 0) {
                    break;
                }
                ENGINE_BLK(802A537C);
                if (*c == cell) {
                    goto visible;
                }
                ENGINE_BLK(802A5388);
                if (cell < *c) {
                    break;
                }
                ENGINE_BLK(802A5390);
                c++;
                n--;
            }
        }
    visible:
        /* not one of the keys drawn? */
        ENGINE_BLK(802A539C);
        for (k = D_803C2EB0;;) {
            ENGINE_BLK(802A53A8);
            if (k == drawn) {
                break;
            }
            ENGINE_BLK(802A53B0);
            if (s->key < *k) {
                break;
            }
            ENGINE_BLK(802A53C0);
            if (*k++ == s->key) {
                goto next;
            }
            ENGINE_BLK(802A53C8);
        }
        ENGINE_BLK(802A53D0);
        if (s->key == last) {
            ENGINE_BLK(802A53D8);
            from = s->dlNoTex;
            patchAt = NULL;
        } else {
            ENGINE_BLK(802A53E8);
            from = s->dl;
            patchAt = *out;
        }
        ENGINE_BLK(802A53F4);
        last = s->key;
        patch = tex_anim_find(h, last);
        ENGINE_BLK(802A5400);
        {
            u32 *src = (u32 *)((u8 *)h + from);
            u32 *srcEnd = (u32 *)((u8 *)h + s->dlEnd);
            u32 *dst = *out;

            for (;;) {
                ENGINE_BLK(802A5408);
                if (src == srcEnd) {
                    break;
                }
                ENGINE_BLK(802A5410);
                dst[0] = src[0];
                dst[1] = src[1];
                src += 2;
                dst += 2;
            }
            *out = dst;
        }
        ENGINE_BLK(802A5424);
        if (!patch) {
            goto next;
        }
        ENGINE_BLK(802A542C);
        if (patchAt == NULL) {
            goto next;
        }
        {
            u32 *p = patchAt;
            u32 w;

            ENGINE_BLK(802A5434);
            do {
                ENGINE_BLK(802A5444);
                w = p[0];
                p += 2;
            } while (OP(w) != G_SETTIMG_OP);
            ENGINE_BLK(802A5460);
            if (D_803C3240 != 0) {
                ENGINE_BLK(802A5468);
                p[-1] = D_803C3240;
            }
            ENGINE_BLK(802A546C);
            if (D_803C324A == 0) {
                goto next;
            }
            do {
                ENGINE_BLK(802A5480);
                w = p[0];
                p += 2;
            } while (OP(w) != G_SETTIMG_OP);
            ENGINE_BLK(802A549C);
            if (D_803C3244 != 0) {
                ENGINE_BLK(802A54B0);
                p[-1] = D_803C3244;
            }
            do {
                ENGINE_BLK(802A54B4);
                w = p[0];
                p += 2;
            } while (OP(w) != G_SETPRIMCOLOR_OP);
            ENGINE_BLK(802A54D0);
            p[-2] = w | D_803C324B;
        }
    next:
        ENGINE_BLK(802A54E4);
        s = SPAN_NEXT(s);
    }
    ENGINE_BLK(802A54F4);
}

/* (802A4E4C) one display list: the header's prefix (unk88..unk8C), the
   visible groups of [g, gEnd) and the spans of [spans, sEnd) with the same
   key, then the other visible spans */
static void terrain_dl(LevelHeader *h, u32 *out, TerrainGroup *g, TerrainGroup *gEnd,
                       TerrainSpan *spans, TerrainSpan *sEnd) {
    u32 *src = (u32 *)AT(h, 0x88);
    u32 *srcEnd = (u32 *)AT(h, 0x8C);
    s16 *drawn = D_803C2EB0;

    ENGINE_BLK(802A4E4C);
    for (;;) {
        ENGINE_BLK(802A4E70);
        if (src == srcEnd) {
            break;
        }
        ENGINE_BLK(802A4E78);
        out[0] = src[0];
        out[1] = src[1];
        src += 2;
        out += 2;
    }
    ENGINE_BLK(802A4E8C);
    for (;;) {
        s32 key, v;
        s16 *p;
        TerrainSpan *s;

        ENGINE_BLK(802A4E94);
        if (g == gEnd) {
            break;
        }
        ENGINE_BLK(802A4E9C);
        for (p = D_803C30A8;;) {
            ENGINE_BLK(802A4EA8);
            v = *p++;
            if (v == -1) {
                goto skip;
            }
            ENGINE_BLK(802A4EBC);
            if (v >= g->cell) {
                break;
            }
        }
        ENGINE_BLK(802A4EC4);
        if (v != g->cell) {
            goto skip;
        }
        ENGINE_BLK(802A4ECC);
        key = g->key;
        *drawn++ = key;
        {
            s32 patch = tex_anim_find(h, key);

            ENGINE_BLK(802A4EDC);
            dl_copy(h, g->dl, g->dlEnd, &out, patch);
        }
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
            for (p = D_803C30A8;;) {
                ENGINE_BLK(802A4F0C);
                v = *p++;
                if (v == -1) {
                    goto same_next;
                }
                ENGINE_BLK(802A4F20);
                if (v >= g->cell) {
                    break;
                }
            }
            ENGINE_BLK(802A4F2C);
            if (v != g->cell) {
                goto same_next;
            }
            ENGINE_BLK(802A4F34);
            dl_copy(h, g->dlNoTex, g->dlEnd, &out, 0);
            ENGINE_BLK(802A4F44);
        same_next:;
        }
        /* the spans with that key */
        ENGINE_BLK(802A4F4C);
        for (s = spans;; s = SPAN_NEXT(s)) {
            s16 *vp;

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
                for (vp = D_803C30A8;;) {
                    s32 cell, n;
                    s32 *c;

                    ENGINE_BLK(802A4F80);
                    cell = *vp;
                    if (cell == -1) {
                        break;
                    }
                    ENGINE_BLK(802A4F90);
                    vp++;
                    c = s->cells;
                    n = s->n;
                    for (;;) {
                        ENGINE_BLK(802A4F9C);
                        if (n == 0) {
                            break;
                        }
                        ENGINE_BLK(802A4FA4);
                        if (*c == cell) {
                            ENGINE_BLK(802A4FC4);
                            dl_copy(h, s->dlNoTex, s->dlEnd, &out, 0);
                            goto span_next;
                        }
                        ENGINE_BLK(802A4FB0);
                        if (cell < *c) {
                            break;
                        }
                        ENGINE_BLK(802A4FB8);
                        c++;
                        n--;
                    }
                }
            }
        span_next:
            ENGINE_BLK(802A4FD4);
        }
        continue;
    skip:
        ENGINE_BLK(802A4FE8);
        g++;
    }
    ENGINE_BLK(802A4FF0);
    spans_rest(h, spans, sEnd, drawn, &out);
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
        out[0] = 0x06000000;
        out[1] = (u32)p - 0x80000000;
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
    LevelHeader *h = D_80358074;

    ENGINE_BLK(802A4CDC);
    terrain_dl(h, (u32 *)dl0, (TerrainGroup *)AT(h, 0xA0), (TerrainGroup *)AT(h, 0xA4),
               (TerrainSpan *)AT(h, 0xB0), (TerrainSpan *)AT(h, 0xB4));
    ENGINE_BLK(802A4D34);
    terrain_dl(h, (u32 *)dl1, (TerrainGroup *)AT(h, 0xA4), (TerrainGroup *)AT(h, 0xA8),
               (TerrainSpan *)AT(h, 0xB4), (TerrainSpan *)AT(h, 0xB8));
    ENGINE_BLK(802A4D5C);
    terrain_dl(h, (u32 *)dl2, (TerrainGroup *)AT(h, 0xA8), (TerrainGroup *)AT(h, 0xAC),
               (TerrainSpan *)AT(h, 0xB8), (TerrainSpan *)AT(h, 0xBC));
    ENGINE_BLK(802A4D84);
    terrain_dl(h, (u32 *)dl3, (TerrainGroup *)AT(h, 0xAC), (TerrainGroup *)AT(h, 0xB0),
               (TerrainSpan *)AT(h, 0xBC), (TerrainSpan *)AT(h, 0xC0));
    ENGINE_BLK(802A4DAC);
    dl_calls((u32 *)calls, (u32 *)AT(h, 0x7C), (u32 *)AT(h, 0x80));
    ENGINE_BLK(802A4DC4);
    tex_anims_tick(h);
    ENGINE_BLK(802A4DCC);
}
