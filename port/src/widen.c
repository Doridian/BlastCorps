/*
 * The ROM's display lists in the port's Gfx (docs/PORT.md, "Graphics").
 * Where a command is wider than the N64's two 32-bit words (the LP64
 * builds': words as wide as a pointer), a display list still arrives from
 * the ROM in the N64's form, and is widened where it lands: in place in the pool,
 * spread backwards from its last command over twice its room, its words
 * kept (texture numbers, segment addresses, offsets).  The decompressor
 * then reports the wide length (src/loads.c), so the callers' heap bump
 * (*dst, func_8028B4C4's length) takes it.
 *
 * Every offset into a widened list is rescaled once, here, right after it:
 * the ones in the header member that was loaded just before it (a model
 * file's, a level's: native.c has already put them in host order), field
 * by field and in the records that carry them, and the level lists' own
 * G_DL words.  An offset o from the file's start, the list member starting
 * at B, becomes B + (o - B) / 8 * sizeof(Gfx); one below B is into the
 * header itself and stays.  That holds for an end as for a start, and for
 * an offset past the list (two logos' VehicleModel.unk20).  The readers
 * (5CB60.c's LEVEL_PTR, MODEL_PTR and VMODEL_PTR, 5FD50.c's terrain,
 * buildings.h's B_SECTION and GroupDls, 8A2E0.c, the front end's models)
 * stay as they are.
 *
 *   record (ROM class)          offsets into its display list
 *   VehicleModel (ROM_VEHICLE)  unk1C, unk20, unk24[0..8], unk48
 *   Model (ROM_MODEL)           unk10, unk14, unk18; GroupDls in [unk38, unk3C)
 *   LevelHeader (ROM_LEVEL)     displayLists[0..9]; TerrainGroup in
 *                               [unkA0[0], unkA0[4]), TerrainSpan in
 *                               [unkA0[4], unkA0[8]) (dl, dlNoTex, dlEnd);
 *                               the list's G_DL words (from displayLists[0])
 *   static_data (ROM_STATIC)    a Vp, then its lists (D_01000010, D_01000038:
 *                               port/tools/gen_syms.py places the second)
 *
 * The header's other offsets must be into the header (a warning if not:
 * a list nobody rescaled).  A list widened in the pool's free part counts
 * as a carve-out of the pool's (src/pool.c).  With the N64's 8-byte
 * commands (the ILP32 builds) all of this is the identity.  In every build
 * the lists' segmented addresses get the port's tag here (dl_tag, gbi.h's
 * G_SEGTAG).
 */
#include "common.h"
#include "port.h"
#include "romclass.h"
#include "game/level.h"
#include "game/model.h"
#include "game/vehicle.h"

/* a command in the ROM, and in this build */
#define N64_GFX_SIZE 8u
#define GFX_SIZE ((u32)sizeof(Gfx))
#define WIDE (GFX_SIZE != N64_GFX_SIZE)

/* the header member loaded last, which a display list right after it
   belongs to */
static u8 *hdr, *hdr_end;
static int hdr_class = ROM_BYTES;

/* a narrow offset o from a file whose display list member starts at b */
static AssetOffset rescale(AssetOffset o, u32 b) {
    return o < b ? o : b + (o - b) / N64_GFX_SIZE * GFX_SIZE;
}
#define RESCALE(f) ((f) = rescale((f), b))

static void check(const char *what, AssetOffset o, u32 b) {
    if (o > b) {
        host_log("widen: %s 0x%X is past the header (0x%X): not rescaled\n", what, (unsigned)o, (unsigned)b);
    }
}

/* n bytes of N64 commands at p as Gfx; returns their length now */
static u32 dl_widen(u8 *p, u32 n) {
    u32 i, cmds = n / N64_GFX_SIZE;

    if (n % N64_GFX_SIZE != 0) {
        host_log("widen: a display list of 0x%X bytes, not whole commands\n", (unsigned)n);
    }
    if ((uintptr_t)p % sizeof(uintptr_t) != 0) {
        host_log("widen: a display list at %p, not aligned for its words\n", (void *)p);
    }
    /* from the last: command i's wide place is at or after its narrow one,
       and only overlaps those after it, already moved */
    for (i = cmds; i-- > 0;) {
        u32 w0 = ((u32 *)p)[2 * i];
        u32 w1 = ((u32 *)p)[2 * i + 1];
        Gfx *g = (Gfx *)p + i;

        g->words.w0 = w0;
        g->words.w1 = w1;
    }
    return cmds * GFX_SIZE;
}

/* Its segmented addresses tagged (gbi.h's G_SEGTAG): the words of
   G_MTX, G_MOVEMEM, G_VTX and G_DL to segments 1..15.  A level's G_DL
   words are offsets from its first list, segment 0 (func_802A1C88 makes
   them addresses). */
static void dl_tag(Gfx *p, Gfx *end) {
    for (; p < end; p++) {
        u32 op = (u32)p->words.w0 >> 24, w1 = (u32)p->words.w1;

        if ((op == G_MTX || op == G_MOVEMEM || op == G_VTX || op == G_DL) && (w1 >> 24 & 15) != 0) {
            p->words.w1 = w1 | G_SEGTAG;
        }
    }
}

static void vehicle_rescale(VehicleModel *m, u32 b) {
    s32 i;

    for (i = 0; i < 5; i++) {
        check("VehicleModel.unk0[]", m->unk0[i], b);
    }
    check("VehicleModel.unk14", m->unk14, b);
    check("VehicleModel.unk18", m->unk18, b);
    RESCALE(m->unk1C);
    RESCALE(m->unk20);
    for (i = 0; i < 9; i++) {
        RESCALE(m->unk24[i]);
    }
    RESCALE(m->unk48);
}

static void model_rescale(Model *m, u32 b) {
    u8 *base = (u8 *)m, *r, *end;
    AssetOffset *f;

    RESCALE(m->unk10);
    RESCALE(m->unk14);
    RESCALE(m->unk18);
    for (f = &m->unk1C; f <= &m->unk4C; f++) {
        check("a Model offset", *f, b);
    }
    /* the groups' looks: {u32 n; GroupCond[n]; GroupDls} */
    r = base + m->unk38;
    end = base + m->unk3C;
    while (r < end) {
        GroupDls *d = GDL_DLS(r, GDL_N(r));

        if ((u8 *)(d + 1) > end) {
            host_log("widen: a Model's GroupDls past its section\n");
            break;
        }
        RESCALE(d->dl0);
        RESCALE(d->dl0_end);
        RESCALE(d->dl1);
        RESCALE(d->dl1_end);
        r = GDL_NEXT(r);
    }
}

static void level_rescale(LevelHeader *h, u32 b, Gfx *list, Gfx *list_end) {
    u8 *base = (u8 *)h;
    AssetOffset *f, dl0 = h->displayLists[0];
    TerrainGroup *g, *gEnd;
    TerrainSpan *s, *sEnd;
    s32 i;
    Gfx *p;

    /* the G_DL words: offsets from displayLists[0] (func_802A1C88 makes
       them addresses) */
    for (p = list; p < list_end; p++) {
        if ((u32)p->words.w0 >> 24 == G_DL) {
            p->words.w1 = rescale(dl0 + (u32)p->words.w1, b) - rescale(dl0, b);
        }
    }
    for (i = 0; i < 10; i++) {
        RESCALE(h->displayLists[i]);
    }
    for (f = &h->ammoBoxes; f <= &h->unk74; f++) {
        check("a LevelHeader offset", *f, b);
    }
    for (i = 0; i < 9; i++) {
        check("LevelHeader.unkA0[]", h->unkA0[i], b);
    }
    g = (TerrainGroup *)(base + h->unkA0[0]);
    gEnd = (TerrainGroup *)(base + h->unkA0[4]);
    for (; g < gEnd; g++) {
        RESCALE(g->dl);
        RESCALE(g->dlNoTex);
        RESCALE(g->dlEnd);
    }
    s = (TerrainSpan *)(base + h->unkA0[4]);
    sEnd = (TerrainSpan *)(base + h->unkA0[8]);
    for (; s < sEnd; s = SPAN_NEXT(s)) {
        RESCALE(s->dl);
        RESCALE(s->dlNoTex);
        RESCALE(s->dlEnd);
    }
}

/* [start, end) arrived from the ROM, a `klass` (romclass.h); returns its
   end in this build's form */
u8 *port_rom_loaded(int klass, u8 *start, u8 *end) {
    u32 b, w;

    switch (klass) {
    case ROM_VEHICLE:
    case ROM_MODEL:
    case ROM_LEVEL:
        hdr = start;
        hdr_end = end;
        hdr_class = klass;
        return end;
    case ROM_DL:
        if (WIDE) {
            w = dl_widen(start, (u32)(end - start));
            port_pool_widened(start, (u32)(end - start), w);
            end = start + w;
        }
        dl_tag((Gfx *)start, (Gfx *)end);
        if (!WIDE) {
            return end;
        }
        if (hdr_class == ROM_BYTES || hdr_end != start) {
            host_log("widen: a display list at %p without its header\n", (void *)start);
            return end;
        }
        b = (u32)(start - hdr);
        if (hdr_class == ROM_VEHICLE) {
            vehicle_rescale((VehicleModel *)hdr, b);
        } else if (hdr_class == ROM_MODEL) {
            model_rescale((Model *)hdr, b);
        } else {
            level_rescale((LevelHeader *)hdr, b, (Gfx *)start, (Gfx *)end);
        }
        hdr_class = ROM_BYTES;
        return end;
    case ROM_STATIC:            /* a Vp, then the lists */
        if (end - start <= (s32)sizeof(Vp)) {
            return end;
        }
        if (WIDE) {
            w = dl_widen(start + sizeof(Vp), (u32)(end - start - sizeof(Vp)));
            port_pool_widened(start, (u32)(end - start - sizeof(Vp)), w);
            end = start + sizeof(Vp) + w;
        }
        dl_tag((Gfx *)(start + sizeof(Vp)), (Gfx *)end);
        return end;
    default:
        hdr_class = ROM_BYTES;
        return end;
    }
}
