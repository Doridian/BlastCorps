/* hd_code 5CB60: the level loader (func_802A1674) and the model loaders.

   The level file (LevelHeader, game/level.h) is loaded by the C; this
   turns its sections into the run-time tables: the grids, the collision
   triangles (CollisionTri, built from the file's 0x14..0x19-byte ones by
   func_802A41B0), the buildings with their models, the vehicles, the
   animated textures.  Models come from the model table (by number) or,
   for vehicles, from their own ROM files; both are gzip pairs DMA'd to
   0x8021ED00 (init's memory) and inflated onto the heap.

   The original passes nearly everything in registers ($t0 the header,
   $t4 a model, $s2 a vehicle model, $t3 a model number ...).  Some of
   what it stores is whatever a register held (see eng_leak below). */
#include "engine_b.h"
#include "engine_b_types.h"
#include "game/vehicle.h"
#include "game/model.h"

/* ---- reaching into records by offset ------------------------------------ */

#define AT(base, off) ((u8 *)(base) + (off))
#define W(base, off) (*(s32 *)AT(base, off))
#define UW(base, off) (*(u32 *)AT(base, off))
#define H(base, off) (*(s16 *)AT(base, off))
#define UH(base, off) (*(u16 *)AT(base, off))
#define B(base, off) (*(u8 *)AT(base, off))
#define SB(base, off) (*(s8 *)AT(base, off))
/* an offset field: the address it gives from `base` */
#define OFS(base, off) AT(base, UW(base, off))
/* a big-endian s16 or u16 at any address (the files' packed records) */
#define BE16S(p) ((s16)((SB(p, 0) << 8) | B(p, 1)))
#define BE16U(p) ((u16)((B(p, 0) << 8) | B(p, 1)))
#define BE32(p) ((u32)((B(p, 0) << 24) | (B(p, 1) << 16) | (B(p, 2) << 8) | B(p, 3)))

#define INIT_AREA ((u8 *)0x8021ED00)    /* where models are DMA'd before inflating */
#define GZIP_WINDOW ((void *)0x8004B400)

/* What some registers held where the original stores them without
   setting them first.  Set from the caller's context on entry
   (ENG_LEAK_ENTRY) and kept as the original would. */
typedef struct EngLeak {
    u32 v0, s1, t9, fp;
} EngLeak;
static EngLeak eng_leak;
#ifndef ENG_LEAK_ENTRY
#define ENG_LEAK_ENTRY(l) ((l)->v0 = (l)->s1 = (l)->t9 = (l)->fp = 0)
#endif

extern Vehicle *D_803649D0;
extern u8 *D_80358070;
extern u64 D_80364A98;          /* the game mode */
extern u64 D_8030D880, D_8030D888;
extern s32 D_80364AA8;
extern s32 D_802E8BDC;          /* the level */
extern u32 D_802E8BEC;
extern OSMesgQueue D_803150A0;
extern OSIoMesg D_80370C58;
extern u8 D_80370C50;
extern u8 D_00006EC4C0_sym[] __asm__("D_006EC4C0");     /* model_table */
#define ROM_MODEL_TABLE ENG_ADDR(D_00006EC4C0_sym)

void func_80278BF0(void *, void *, void *);
void func_8028FDA0(void *, void *);
void func_8026FBB0(void *, void *);
void func_8028D4C0(void *, void *);
void func_8028C190(void *, void *);
s32 func_80268EE8(s32);
void func_80295AE0(void *, void *);
s32 func_802CE6F8(s32, s32, s32);
u32 eng_tex_get(s32 id, u32 param);
void eng_tex_fix_dl(u32 *dl, u32 *end);
void eng_gzip(u8 **src, u8 **dst, void *window);
void eng_vis_reset(void);
void eng_c049c(void);          /* 77E20's func_802C049C */
void eng_bc840(void);          /* 77E20's func_802BC840 */
void eng_comm_point_load(LevelHeader *h);  /* 8A2E0's func_802CEAA0 */
void eng_anim_slots_reset(void);           /* 60F60's func_802A5F30 */
u32 eng_cop0_status(void);

/* Engine-A's (still translated) functions, with the registers they take.
   (Placeholders until the shared calling mechanism is in.) */
void eng_a_8029DEA0(void);
void eng_a_8029DC80(void);
void eng_a_8029DF78(u32 *dl, u32 *end);
void eng_a_vehicle_init(u32 func, s32 type, s32 x, s32 y, s32 z, s32 heading, u8 *model,
                        LevelHeader *h, u8 *next);
void eng_a_802B9C50(s32 x, s32 y, s32 z, s32 w, u8 n, u8 *model);
void eng_a_802D2570(u8 *model);
void eng_a_802B8480(u8 *model);
void eng_a_802C4BF0(void);

/* ---- small ones ---------------------------------------------------------- */

/* func_802A1320: the COP0 Status register */
u32 func_802A1320(void) {
    ENG_COST(7);
    return eng_cop0_status();
}

/* func_802A133C: move the vehicle of `type` to (x, y, z) (in $v0, $v1,
   $a0; the type in $a1, its state in $gp) */
void eng_vehicle_move(s32 x, s32 y, s32 z, s32 type, u8 *state) {
    Vehicle *v = D_80364460;

    ENG_COST(6);
    for (;;) {
        ENG_COST(3);
        if (v->type == type) {
            break;
        }
        v++;
    }
    ENG_COST(10);
    v->x = x;
    v->y = y;
    v->z = z;
    v->unk70 = B(state, 0x9B);
}

/* func_802A44E4: round up to 8 if not a multiple of 4 (sic) */
static u32 round8(u32 a) {
    ENG_COST(3);
    if (a & 3) {
        ENG_COST(3);
        a = (a & ~7) + 8;
    }
    ENG_COST(2);
    return a;
}

/* func_802A1388: a vehicle from its model file `m` (in $s2): the next
   Vehicle, the model's pointers, its display lists copied to the heap.
   ($v0, $v1, $a0, $a1: unk4, unk8, the type and whether to make its
   heap block even in the attract mode.) */
void eng_vehicle_new(u8 *m, void *unk4, void *unk8, s32 type, s32 always) {
    Vehicle *v = D_803649D0;
    u8 *src, *end, *dst;
    s32 delta;

    ENG_COST(60);
    D_803649D0 = v + 1;
    v->unk70 = 0;
    v->unk4 = unk4;
    v->unk8 = unk8;
    v->type = type;
    v->unk0 = OFS(m, 0x14);
    v->unkC = OFS(m, 0x24);
    v->unk10 = OFS(m, 0x28);
    v->unk14 = OFS(m, 0x2C);
    v->unk18 = OFS(m, 0x30);
    v->unk1C = OFS(m, 0x34);
    v->unk20 = OFS(m, 0x38);
    v->unk24 = OFS(m, 0x3C);
    v->unk28 = OFS(m, 0x40);
    v->unk2C = OFS(m, 0x44);
    v->unk54 = (Gfx *)OFS(m, 0x48);
    src = OFS(m, 0x1C);
    end = OFS(m, 0x20);
    dst = D_80358070;
    delta = dst - (u8 *)v->unkC;
    for (;;) {
        ENG_COST(2);
        if (src == end) {
            break;
        }
        ENG_COST(5);
        UW(dst, 0) = UW(src, 0);
        UW(dst, 4) = UW(src, 4);
        src += 8;
        dst += 8;
    }
    ENG_COST(33);
    D_80358070 = dst;
    v->unk30 = (u8 *)v->unkC + delta;
    v->unk34 = (u8 *)v->unk10 + delta;
    v->unk38 = (u8 *)v->unk14 + delta;
    v->unk3C = (u8 *)v->unk18 + delta;
    v->unk40 = (u8 *)v->unk1C + delta;
    v->unk44 = (u8 *)v->unk20 + delta;
    v->unk48 = (u8 *)v->unk24 + delta;
    v->unk4C = (u8 *)v->unk28 + delta;
    v->unk50 = (u8 *)v->unk2C + delta;
    if (D_80364AA8 == 1) {
        ENG_COST(2);
        if (always == 0) {
            ENG_COST(10);
            return;
        }
    }
    ENG_COST(4 + 37);
    func_80278BF0(v->unkC, v->unk18, &v->unk58);   /* (func_802A1558) */
    ENG_COST(34 + 10);
}

/* ---- loading models ------------------------------------------------------ */

/* DMA [rom, rom + size) to INIT_AREA and inflate its two gzip members
   onto the heap; returns the model (the heap's old top). */
static u8 *model_inflate(u32 rom, u32 size, s32 pre, s32 post) {
    u8 *src = INIT_AREA;
    u8 *dst = D_80358070;
    u8 *m;

    osInvalDCache(INIT_AREA, size);
    ENG_COST(11);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, rom, INIT_AREA, size, &D_803150A0);
    ENG_COST(5);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    ENG_COST(7);
    eng_gzip(&src, &dst, GZIP_WINDOW);
    ENG_COST(2);
    eng_gzip(&src, &dst, GZIP_WINDOW);
    ENG_COST(2);
    dst = (u8 *)round8(ENG_ADDR(dst));
    m = D_80358070;
    D_80358070 = dst;
    return m;
}

/* func_802A2A98: model `n` of the model table (the original returns it
   in $s0) */
static u8 *model_load(s32 n) {
    u32 *table = (u32 *)D_803BE6F0;
    u32 start = table[n], end = table[n + 1];
    u8 *m;

    ENG_COST(27);
    m = model_inflate(ROM_MODEL_TABLE + start, end - start, 0, 0);
    ENG_COST(16);
    return m;
}

/* func_802A2BB0: the model table, 0x800 bytes, onto the heap */
static void model_table_load(void) {
    u8 *t = D_80358070;

    ENG_COST(17);
    D_803BE6F0 = (u32 *)t;
    D_80358070 = t + 0x800;
    osInvalDCache(t, 0x800);
    ENG_COST(13);
    osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, ROM_MODEL_TABLE, t, 0x800, &D_803150A0);
    ENG_COST(5);
    osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    ENG_COST(6);
}

/* The vehicles' model files: [start, end) in the ROM by type. */
extern u8 D_0048FE90[], D_004903C0[], D_00490AC0[], D_00491E00[], D_004929D0[], D_00494390[],
    D_00496AD0[], D_00497AF0[], D_004989E0[], D_00499690[], D_0049AD20[], D_0049B630[],
    D_0049BCE0[], D_0049C480[], D_0049E8E0[], D_0049F7A0[], D_0049FF70[], D_004A0720[],
    D_004A1000[], D_004A1690[], D_004A4120[], D_004A5660[];

typedef struct VehicleFile {
    s32 type;
    u8 *start, *end;
} VehicleFile;

/* func_802A396C's table, in the order it tests */
static const struct { s32 type; u8 *start, *end; } vehicle_files[] = {
    { 0x00, D_00491E00, D_004929D0 }, { 0x01, D_004929D0, D_00494390 },
    { 0x02, D_00494390, D_00496AD0 }, { 0x10, D_004A1690, D_004A4120 },
    { 0x03, D_00490AC0, D_00491E00 }, { 0x04, D_00496AD0, D_00497AF0 },
    { 0x05, D_00497AF0, D_004989E0 }, { 0x06, D_0049AD20, D_0049B630 },
    { 0x07, D_0049B630, D_0049BCE0 }, { 0x08, D_0049BCE0, D_0049C480 },
    { 0x09, D_0049C480, D_0049E8E0 }, { 0x0A, D_0049E8E0, D_0049F7A0 },
    { 0x0B, D_0049F7A0, D_0049FF70 }, { 0x11, D_0049F7A0, D_0049FF70 },
    { 0x12, D_0049F7A0, D_0049FF70 }, { 0x0D, D_0049FF70, D_004A0720 },
    { 0x0E, D_004A0720, D_004A1000 }, { 0x0F, D_004A1000, D_004A1690 },
    { 0xFE, D_004989E0, D_00499690 }, { 0xFF, D_00499690, D_0049AD20 },
    { 0xFD, D_004A4120, D_004A5660 }, { 0x96, D_004903C0, D_00490AC0 },
    { 0x98, D_0048FE90, D_004903C0 },
};

/* func_802A32CC's: the carrier's cargo models */
static const struct { s32 type; u8 *start, *end; } cargo_files[] = {
    { 0x03, D_00490AC0, D_00491E00 }, { 0x04, D_00496AD0, D_00497AF0 },
    { 0x05, D_00497AF0, D_004989E0 }, { 0x08, D_0049BCE0, D_0049C480 },
    { 0x09, D_0049C480, D_0049E8E0 }, { 0x0A, D_0049E8E0, D_0049F7A0 },
    { 0x0D, D_0049FF70, D_004A0720 }, { 0x0E, D_004A0720, D_004A1000 },
    { 0x0F, D_004A1000, D_004A1690 },
};

/* func_802A396C: vehicle model `type` (returned in $s2 by the original) */
u8 *eng_vehicle_model_load(s32 type) {
    s32 i, n = sizeof(vehicle_files) / sizeof(vehicle_files[0]);
    u8 *m;

    ENG_COST(14);
    for (i = 0;; i++) {
        if (i == n) {
            ENG_COST(1);
            eng_trap_syscall();
        }
        if (vehicle_files[i].type == type) {
            break;
        }
        ENG_COST(i == 0 ? 3 : 2);
    }
    ENG_COST(6 + 7);
    m = model_inflate(ENG_ADDR(vehicle_files[i].start),
                      vehicle_files[i].end - vehicle_files[i].start, 0, 0);
    ENG_COST(10);
    eng_a_8029DF78((u32 *)OFS(m, 0x1C), (u32 *)OFS(m, 0x20));
    ENG_COST(5);
    eng_tex_fix_dl((u32 *)OFS(m, 0x1C), (u32 *)OFS(m, 0x20));
    ENG_COST(15);
    return m;
}

/* func_802A32CC: cargo model `type` (in $s2) */
static u8 *cargo_model_load(s32 type) {
    s32 i, n = sizeof(cargo_files) / sizeof(cargo_files[0]);
    u8 *m;

    ENG_COST(14);
    for (i = 0;; i++) {
        if (i == n) {
            ENG_COST(1);
            eng_trap_syscall();
        }
        if (cargo_files[i].type == type) {
            break;
        }
        ENG_COST(i == 0 ? 3 : 2);
    }
    ENG_COST(6 + 7);
    m = model_inflate(ENG_ADDR(cargo_files[i].start),
                      cargo_files[i].end - cargo_files[i].start, 0, 0);
    ENG_COST(10);
    eng_tex_fix_dl((u32 *)OFS(m, 0x1C), (u32 *)OFS(m, 0x20));
    ENG_COST(14);
    return m;
}

/* ---- collision triangles ------------------------------------------------- */

/* func_802A41B0: a CollisionTri from a file triangle (9 big-endian s16 at
   any alignment, a u16 at 0x12) */
void eng_collision_tri_init(CollisionTri *t, const u8 *src, u8 b4f, u8 b50, u16 h52,
                            u8 b55, u8 b56, u8 b57, u8 b58) {
    s32 x0, y0, z0, x1, y1, z1, x2, y2, z2;
    s32 e1x, e1y, e1z, e2x, e2y, e2z;
    s64 nx, ny, nz, ax, ay, az;
    f32 len2;

    ENG_COST(130);
    t->unk59 = 0;
    t->unk55 = b55;
    t->unk56 = b56;
    t->unk52 = h52;
    t->unk4F = b4f;
    t->unk50 = b50;
    t->unk57 = b57;
    t->unk58 = b58;
    x0 = BE16S(src + 0x0) << 3;
    y0 = BE16S(src + 0x2) << 3;
    z0 = BE16S(src + 0x4) << 3;
    x1 = BE16S(src + 0x6) << 3;
    y1 = BE16S(src + 0x8) << 3;
    z1 = BE16S(src + 0xA) << 3;
    x2 = BE16S(src + 0xC) << 3;
    y2 = BE16S(src + 0xE) << 3;
    z2 = BE16S(src + 0x10) << 3;
    e1y = y0 - y1;      /* t7 */
    e1z = z0 - z1;      /* s0 */
    e2y = y0 - y2;      /* s2 */
    e2z = z0 - z2;      /* s3 */
    e1x = x0 - x1;      /* s1 */
    e2x = x0 - x2;      /* t6 */
    t->v[0][0] = x0; t->v[0][1] = y0; t->v[0][2] = z0;
    t->v[1][0] = x1; t->v[1][1] = y1; t->v[1][2] = z1;
    t->v[2][0] = x2; t->v[2][1] = y2; t->v[2][2] = z2;
    nx = (s64)e1y * e2z - (s64)e1z * e2y;
    t->nx = nx;
    len2 = (f32)nx;
    len2 = len2 * len2;
    ny = (s64)e1z * e2x - (s64)e2x * 0;     /* placeholder replaced below */
    ny = (s64)e1z * e1x * 0 + ((s64)e1z * e2x - (s64)e2z * e1x) * 0;
    /* s5 = s0*s1 - t6*s3 : e1z*e1x - e2x*e2z */
    ny = (s64)e1z * e1x - (s64)e2x * e2z;
    t->ny = ny;
    {
        f32 f = (f32)ny;

        len2 = len2 + f * f;
    }
    /* s6 = t6*s2 - t7*s1 : e2x*e2y - e1y*e1x */
    nz = (s64)e2x * e2y - (s64)e1y * e1x;
    t->nz = nz;
    {
        f32 f = (f32)nz;

        len2 = len2 + f * f;
    }
    t->nlen2 = len2;
    /* s7 = -(s4*a2 + s5*a3 + s6*t0): a2, a3, t0 are x1, y1, z1 */
    t->d = -(nx * x1 + ny * y1 + nz * z1);
    t->nlen = __builtin_sqrtf(len2);
    ax = nx < 0 ? -nx : nx;
    if (nx < 0) {
        ENG_COST(1);
    }
    ENG_COST(2);
    ay = ny < 0 ? -ny : ny;
    if (ny < 0) {
        ENG_COST(1);
    }
    ENG_COST(2);
    az = nz < 0 ? -nz : nz;
    if (nz < 0) {
        ENG_COST(1);
    }
    ENG_COST(3);
    if (az < ax || (ENG_COST(2), az < ay)) {
        ENG_COST(3);
        if (ay < ax || (ENG_COST(2), ay < az)) {
            ENG_COST(2);
            t->axis = 2;
        } else {
            ENG_COST(3);
            t->axis = 1;
        }
    } else {
        ENG_COST(2);
        t->axis = 0;
    }
    ENG_COST(19);
    t->unk54 = 0;
    t->unk4C = BE16U(src + 0x12);
}

/* func_802A3D54 / func_802A3DF8: a grid of triangle lists (LevelHeader's
   unk10 cells; each list a big-endian length, then 0x16-byte triangles)
   into CollisionTris on the heap, with a pointer per cell in `table` */
static void collision_grid(LevelHeader *h, u32 off, CollisionTri **table) {
    s32 cells = H(h, 0x10) * H(h, 0x12);
    u8 *base = OFS(h, off);
    u8 *p = base;
    CollisionTri *t = (CollisionTri *)D_80358070;

    ENG_COST(17);
    do {
        u8 *end;

        ENG_COST(6);
        *table++ = t;
        end = base + BE32(p);
        p += 4;
        for (;;) {
            ENG_COST(2);
            if (end == p) {
                break;
            }
            ENG_COST(2);
            eng_collision_tri_init(t, p, eng_leak.t9, eng_leak.v0, cells, 0, B(p, 0x14),
                                   (u8)ENG_ADDR(end), eng_leak.s1);
            ENG_COST(4);
            t->unk59 = B(p, 0x15);
            t++;
            p += 0x16;
        }
        ENG_COST(3);
    } while (--cells != 0);
    ENG_COST(7);
    *table = t;
    D_80358070 = (u8 *)t;
}

/* func_802A3E9C: the walls (LevelHeader.unk64..unk68) into D_803BD310's
   0xFC-byte records, their triangles on the heap */
static void walls_load(LevelHeader *h) {
    u8 *p = OFS(h, 0x64);
    u8 *end = OFS(h, 0x68);
    u8 *w = (u8 *)D_803BD310;
    CollisionTri *t = (CollisionTri *)D_80358070;

    ENG_COST(14);
    D_803BD308 = t;
    for (;;) {
        u32 n, k;
        u32 *slot;

        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(7);
        B(w, 0xF9) = p[0];
        n = p[1];
        B(w, 0) = n;
        p += 2;
        for (k = 1;; k++) {
            ENG_COST(2);
            if (n == 0) {
                break;
            }
            ENG_COST(6);
            B(w, k) = *p++;
            n--;
        }
        ENG_COST(4);
        n = *p++;
        B(w, 0xF8) = n;
        slot = (u32 *)AT(w, 8);
        for (;;) {
            ENG_COST(2);
            if (n == 0) {
                break;
            }
            ENG_COST(4);
            n--;
            *slot++ = ENG_ADDR(t);
            eng_collision_tri_init(t, p, eng_leak.t9, eng_leak.v0, (u16)ENG_ADDR(end), 1,
                                   (u8)ENG_ADDR(slot), n, eng_leak.s1);
            t++;
            ENG_COST(2);
            p += 0x14;
        }
        ENG_COST(2);
        w += 0xFC;
    }
    ENG_COST(12);
    D_803BDAF0 = w;
    D_803BD30C = t;
    D_80358070 = (u8 *)t;
}

/* func_802A4168: whether a triangle of [D_803B9890, end) has id `id` */
static s32 tri_id_used(u32 id, CollisionTri *end) {
    CollisionTri *t = (CollisionTri *)D_803B9890;

    ENG_COST(7);
    for (;;) {
        ENG_COST(2);
        if (t == end) {
            ENG_COST(5);
            return 0;
        }
        ENG_COST(3);
        if (t->unk50 == id) {
            ENG_COST(1 + 5);
            return 1;
        }
        t++;
    }
}

/* func_802A3F80: LevelHeader.unk68..unk6C into D_803BC1D0's 0xDC-byte
   records, their triangles from D_803B9890 (one per id) */
static void objects68_load(LevelHeader *h) {
    u8 *p = OFS(h, 0x68);
    u8 *end = OFS(h, 0x6C);
    u8 *r = (u8 *)D_803BC1D0;
    CollisionTri *tris = (CollisionTri *)D_803B9890;   /* $fp */
    u32 last = eng_leak.s1;     /* $t7: the last value read */

    ENG_COST(12);
    for (;;) {
        u32 n, kind;
        u8 *q = r;
        u8 *ids;
        CollisionTri *t;

        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(9);
        B(r, 0xC4) = p[0];
        kind = p[1];
        B(r, 0xC5) = kind;
        n = p[2];
        B(r, 0xC6) = n;
        p += 3;
        if (kind == 0) {
            do {
                ENG_COST(40);
                W(q, 0x00) = BE16S(p + 0) << 5;
                W(q, 0x04) = BE16S(p + 2) << 5;
                W(q, 0x08) = BE16S(p + 4) << 5;
                W(q, 0x0C) = BE16S(p + 6) << 5;
                W(q, 0x10) = BE16S(p + 8) << 5;
                W(q, 0x14) = BE16S(p + 10) << 5;
                eng_leak.s1 = W(q, 0x08);
                p += 0xC;
                q += 0x18;
            } while (--n != 0);
            ENG_COST(5);
            last = BE32(p);
            UW(q, 0) = last;
            p += 4;
        } else {
            do {
                ENG_COST(9);
                last = (u32)(s32)BE16S(p);
                UH(q, 0) = last;
                p += 2;
                q += 2;
            } while (--n != 0);
        }
        ENG_COST(6);
        n = *p++;
        t = tris;
        ids = AT(r, 0xC8);
        B(r, 0xC7) = n;
        for (;;) {
            u32 id;

            ENG_COST(2);
            if (n == 0) {
                break;
            }
            ENG_COST(4);
            id = p[0x14];
            *ids++ = id;
            if (!tri_id_used(id, tris)) {
                ENG_COST(2 + 2);
                eng_collision_tri_init(t, p, 0, id, (u16)ENG_ADDR(end), 1, last, n,
                                       eng_leak.s1);
                t++;
            } else {
                ENG_COST(2);
            }
            ENG_COST(3);
            p += 0x15;
            n--;
        }
        ENG_COST(5);
        tris = t;
        n = *p++;
        ids = AT(r, 0xD1);
        B(r, 0xD0) = n;
        ENG_COST(2);
        if (n != 0) {
            do {
                ENG_COST(6);
                *ids++ = *p++;
            } while (--n != 0);
        }
        ENG_COST(2);
        r += 0xDC;
    }
    ENG_COST(11);
    D_803BD304 = r;
    D_803BD300 = tris;
}

/* func_802A4464: a pointer to each terrain group (after its length) */
static void terrain_groups(LevelHeader *h) {
    s32 n = H(h, 8) * H(h, 0xA);
    u8 *base = OFS(h, 0x30);
    u8 *p = base;
    u8 **out = (u8 **)D_803BDB10;

    ENG_COST(14);
    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            break;
        }
        ENG_COST(9);
        p += 4;
        *out++ = p;
        p = base + BE32(p - 4);
        n--;
    }
    ENG_COST(7);
    *out = p + 4;
}

/* ---- the level's parts --------------------------------------------------- */

/* func_802A2D68: the header's grids and constants; the run-time tables
   emptied */
static void level_globals(LevelHeader *h) {
    s32 i;
    u8 *p;

    ENG_COST(83);
    D_803EBBF0 = (f32)h->gravity;
    D_80364458 = AT(h, 0xC8);
    D_803BDAF4 = (s16 *)OFS(h, 0x24);
    D_803BDAF8 = (s16 *)OFS(h, 0x28);
    D_803BE714 = UH(h, 0x0);
    D_803BE716 = UH(h, 0x2);
    D_803BE70C = UH(h, 0x4) << 5;
    D_803BE710 = UH(h, 0x6) << 5;
    D_803BE720 = UH(h, 0x8);
    D_803BE722 = UH(h, 0xA);
    D_803BE718 = UH(h, 0xC) << 5;
    D_803BE71C = UH(h, 0xE) << 5;
    D_803BE72C = UH(h, 0x10);
    D_803BE72E = UH(h, 0x12);
    D_803BE724 = UH(h, 0x14) << 5;
    D_803BE728 = UH(h, 0x16) << 5;
    D_803EBBEC = D_803EBDB0;
    for (i = 0, p = D_803A6B30; i < 100; i++, p += 0x14) {
        ENG_COST(4);
        B(p, 0x13) = 0xFF;
    }
    ENG_COST(3);
    for (i = 0, p = D_803A7300; i < 12; i++, p += 0x14) {
        ENG_COST(4);
        B(p, 0x11) = 0xFF;
    }
    ENG_COST(3);
    for (i = 0, p = D_803ED3B8; i < 12; i++, p += 4) {
        ENG_COST(7);
        p[0] = p[1] = p[2] = p[3] = 0xFF;
    }
    ENG_COST(3);
    for (i = 0, p = D_803EBC10; i < 26; i++, p += 0x10) {
        ENG_COST(4);
        W(p, 0) = -1;
    }
    ENG_COST(57);
    D_803BE6F8 = OFS(h, 0x50);
    p = OFS(h, 0x4C);
    D_803BE730 = H(p, 0);
    D_803BE734 = H(p, 2);
    D_803BE732 = H(p, 4);
    D_803BE736 = H(p, 6);
    D_803A7426 = 0;
    D_803ED400 = 0;
    D_803F7805 = 0;
    D_803F7806 = 0;
    D_803BE738 = 0;
    D_803BE739 = h->unk1C;
    D_803EFECB = 0;
    D_803EF6FF = 0;
    D_803ED40C = 0;
    D_803A740C = 0;
    D_803F77F8 = 0;
    D_803F780A = 0;
    D_803F780B = 0;
    D_803F780C = 0;
    D_803F7810 = 0;
    D_803F7804 = 0;
    D_803F7812 = 0;
}

/* func_802A1C20: the animated textures' frames loaded (each id replaced
   by its physical address) */
static void tex_anims_load(LevelHeader *h) {
    u8 *a = OFS(h, 0x2C);
    u8 *end = OFS(h, 0x30);

    ENG_COST(6);
    for (;;) {
        u32 *tex;
        s32 n;

        ENG_COST(2);
        if (a == end) {
            break;
        }
        ENG_COST(4);
        tex = (u32 *)AT(a, 0xC);
        n = B(a, 4) - 1;
        eng_leak.s1 = 0x80000000;
        for (;;) {
            ENG_COST(2);
            if (n == 0) {
                break;
            }
            ENG_COST(3);
            n--;
            *tex = eng_tex_get(*tex, eng_leak.fp);
            ENG_COST(3);
            tex++;
        }
        ENG_COST(2);
        a = (u8 *)tex;
    }
    ENG_COST(4);
}

/* func_802A1C88: the level's display lists: G_DL addresses made physical */
static void level_dls(LevelHeader *h) {
    u32 base = ENG_ADDR(OFS(h, 0x78)) - 0x80000000;
    u32 *p = (u32 *)OFS(h, 0x90);
    u32 *end = (u32 *)OFS(h, 0x84);

    ENG_COST(16);
    for (;;) {
        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(4);
        if (p[0] >> 24 != 6) {
            p += 2;
            continue;
        }
        ENG_COST(5);
        p[1] += base;
        p += 2;
    }
    ENG_COST(24);
    D_803BE6E0 = (Gfx *)OFS(h, 0x90);
    D_803BE6E4 = (Gfx *)OFS(h, 0x94);
    D_803BE6E8 = (Gfx *)OFS(h, 0x98);
    D_803BE6EC = (Gfx *)OFS(h, 0x9C);
}

/* func_802A1A9C: LevelHeader.unk74's matrices (identity) and records */
static void level_mtx(LevelHeader *h) {
    u8 *src = OFS(h, 0x74);
    u32 size = UW(src, 0);
    u8 *t = D_80358070;
    u8 *end;

    ENG_COST(17);
    D_803F7820 = t;
    D_803F7824 = t + size;
    end = t + size * 2;
    for (;;) {
        ENG_COST(2);
        if (t == end) {
            break;
        }
        ENG_COST(22);
        H(t, 0x00) = 1; H(t, 0x02) = 0; W(t, 0x04) = 0;
        H(t, 0x08) = 0; H(t, 0x0A) = 1; W(t, 0x0C) = 0;
        W(t, 0x10) = 0; H(t, 0x14) = 1; H(t, 0x16) = 0;
        W(t, 0x18) = 0; H(t, 0x1C) = 0; H(t, 0x1E) = 1;
        W(t, 0x20) = 0; W(t, 0x24) = 0; W(t, 0x28) = 0; W(t, 0x2C) = 0;
        W(t, 0x30) = 0; W(t, 0x34) = 0; W(t, 0x38) = 0; W(t, 0x3C) = 0;
        t += 0x40;
    }
    ENG_COST(4);
    D_803F7828 = t;
    if (size != 0) {
        u8 *g = src + 4;
        s32 more;

        ENG_COST(2);
        do {
            u8 *q = g;
            s32 n;

            ENG_COST(4);
            more = H(q, 0);
            n = W(q, 0x28);
            g = q + 0x2C;
            for (;;) {
                ENG_COST(2);
                if (n == 0) {
                    break;
                }
                ENG_COST(33);
                n--;
                W(t, 0x00) = H(g, 0x0) << 5;
                W(t, 0x04) = H(g, 0x2) << 5;
                W(t, 0x08) = H(g, 0x4) << 5;
                W(t, 0x0C) = H(g, 0x6) << 5;
                W(t, 0x10) = H(g, 0x8) << 5;
                W(t, 0x14) = H(g, 0xA) << 5;
                W(t, 0x18) = H(g, 0xC) << 5;
                W(t, 0x1C) = H(g, 0xE) << 5;
                W(t, 0x20) = H(g, 0x10) << 5;
                B(t, 0x24) = B(g, 0x12);
                t += 0x28;
                g += 0x44;
            }
            ENG_COST(3);
        } while (more != -1);
        eng_leak.s1 = 0;
    }
    ENG_COST(8);
    D_803F782C = t;
    D_80358070 = t;
}

/* func_802A1934: D_80306480's boxes: corners from centre and size */
static void boxes_init(void) {
    u8 *b = (u8 *)D_80306480;

    ENG_COST(10);
    for (;;) {
        s32 x, y, z, r;

        ENG_COST(4);
        if (SB(b, 0x15) == -1) {
            break;
        }
        ENG_COST(25);
        x = W(b, 0) >> 5;
        y = W(b, 4) >> 5;
        z = W(b, 8) >> 5;
        r = B(b, 0x14);
        H(b, 0x16) = H(b, 0x22) = x - r;
        H(b, 0x1A) = H(b, 0x20) = z - r;
        H(b, 0x1C) = H(b, 0x28) = x + r;
        H(b, 0x26) = H(b, 0x2C) = z + r;
        H(b, 0x18) = H(b, 0x1E) = H(b, 0x24) = H(b, 0x2A) = y;
        b += 0x30;
    }
    ENG_COST(9);
}

/* func_802A19F4: the level's extra model (D_8039CAB7), moved to
   D_8039CAB0..B4 */
static void extra_model_load(void) {
    ENG_COST(6);
    if (D_8039CAB7 != 0) {
        u8 *m;
        u8 *v, *end;

        ENG_COST(2);
        m = eng_vehicle_model_load(0x98);
        ENG_COST(12);
        v = OFS(m, 0x14);
        D_8039CAC0 = v;
        end = OFS(m, 0x18);
        for (;;) {
            ENG_COST(2);
            if (v == end) {
                break;
            }
            ENG_COST(11);
            H(v, 0) += D_8039CAB0;
            H(v, 2) += D_8039CAB2;
            H(v, 4) += D_8039CAB4;
            v += 0x10;
        }
        ENG_COST(4);
        D_8039CABC = OFS(m, 0x24);
    }
    ENG_COST(5);
}

/* func_802A2C54: LevelHeader.unk60's records into D_803BDFD8 (0x24 each) */
static void level_unk60(LevelHeader *h) {
    u8 *p = OFS(h, 0x60);
    u8 *end = OFS(h, 0x64);
    u8 *r = (u8 *)D_803BDFD8;

    ENG_COST(13);
    D_80364A6E = *p++;
    for (;;) {
        u32 n;
        u8 *q;

        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(30);
        W(r, 0x0) = BE16U(p + 0) << 5;
        W(r, 0x4) = BE16U(p + 2) << 5;
        W(r, 0x8) = BE16U(p + 4) << 5;
        W(r, 0xC) = BE16U(p + 6) << 5;
        B(r, 0x10) = p[8];
        n = p[9];
        B(r, 0x13) = n;
        p += 10;
        q = AT(r, 0x15);
        for (;;) {
            ENG_COST(2);
            if (n == 0) {
                break;
            }
            ENG_COST(6);
            *q++ = *p++;
            n--;
        }
        ENG_COST(9);
        B(r, 0x11) = p[0];
        B(r, 0x14) = p[1];
        B(r, 0x12) = 1;
        p += 2;
        r += 0x24;
    }
    ENG_COST(7);
    D_803BDFD4 = r;
}

/* ---- buildings ------------------------------------------------------------ */

/* func_802A2608: a model's animated textures loaded */
static void model_tex_anims_load(u8 *m) {
    u8 *a = OFS(m, 0x28);
    u8 *end = OFS(m, 0x2C);

    ENG_COST(12);
    for (;;) {
        u32 *tex;
        s32 n;

        ENG_COST(2);
        if (a == end) {
            break;
        }
        ENG_COST(2);
        UW(a, 0) = eng_tex_get(UW(a, 0), eng_leak.fp);
        ENG_COST(5);
        n = B(a, 4) - 1;
        tex = (u32 *)AT(a, 0x10);
        eng_leak.s1 = 0x80000000;
        for (;;) {
            ENG_COST(2);
            if (n == 0) {
                break;
            }
            ENG_COST(3);
            n--;
            *tex = eng_tex_get(*tex, eng_leak.fp);
            ENG_COST(3);
            tex++;
        }
        ENG_COST(2);
        a = (u8 *)tex;
    }
    ENG_COST(9);
}

/* func_802A26A8: move a model by (dx, dy, dz) */
static void model_move(u8 *m, s32 dx, s32 dy, s32 dz) {
    u8 *p, *end;
    s32 i;

    ENG_COST(9);
    for (p = OFS(m, 0x40), end = OFS(m, 0x44);; p += 2) {
        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(5);
        H(p, 0) += dy;
    }
    ENG_COST(18);
    p = OFS(m, 0x20);
    H(p, 0) += dx;
    H(p, 2) += dz;
    H(p, 4) += dx;
    H(p, 6) += dz;
    for (p = OFS(m, 0x24), end = OFS(m, 0x28);; p += 0x14) {
        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(29);
        for (i = 0; i < 18; i += 6) {
            H(p, i + 0) += dx;
            H(p, i + 2) += dy;
            H(p, i + 4) += dz;
        }
    }
    ENG_COST(3);
    for (p = OFS(m, 0x1C), i = 4;; p += 6) {
        ENG_COST(2);
        if (i == 0) {
            break;
        }
        ENG_COST(12);
        i--;
        H(p, 0) += dx;
        H(p, 2) += dy;
        H(p, 4) += dz;
    }
    ENG_COST(3);
    if (UH(m, 0xE) == 0) {
        ENG_COST(3);
        for (p = AT(m, 0x50), end = OFS(m, 0x1C);; p += 0x10) {
            ENG_COST(2);
            if (p == end) {
                break;
            }
            ENG_COST(11);
            H(p, 0) += dx;
            H(p, 2) += dy;
            H(p, 4) += dz;
        }
    }
    ENG_COST(4);
    for (p = OFS(m, 0x48), end = OFS(m, 0x4C);; p += 0x19) {
        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(74);
        for (i = 0; i < 18; i += 6) {
            u32 v;

            v = (u32)(BE16S(p + i + 0) + dx);
            p[i + 0] = v >> 8; p[i + 1] = v;
            v = (u32)(BE16S(p + i + 2) + dy);
            p[i + 2] = v >> 8; p[i + 3] = v;
            v = (u32)(BE16S(p + i + 4) + dz);
            p[i + 4] = v >> 8; p[i + 5] = v;
        }
    }
    ENG_COST(4);
    for (i = 0x30; i <= 0x34; i += 4) {
        for (p = OFS(m, i), end = OFS(m, i + 4);; p += 0x38) {
            ENG_COST(2);
            if (p == end) {
                break;
            }
            ENG_COST(18);
            W(p, 0x0) = (W(p, 0x0) + dx) << 16;
            W(p, 0x4) = (W(p, 0x4) + dy) << 16;
            W(p, 0x8) = (W(p, 0x8) + dz) << 16;
            W(p, 0x28) = (W(p, 0x28) + dy) << 16;
        }
        ENG_COST(4);
    }
    for (p = OFS(m, 0x2C), end = OFS(m, 0x30);; p += 8) {
        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(11);
        H(p, 0) += dx;
        H(p, 2) += dy;
        H(p, 4) += dz;
    }
    ENG_COST(6);
}

/* func_802A20F4: in game mode 0x800, a model's vertex colours (0xC..0xE)
   set to 0xFF, 0, 0 for `flag` */
static void model_tint(u8 *m, u32 flag) {
    ENG_COST(6);
    if (flag != 0) {
        ENG_COST(5);
        if (D_80364A98 == 0x800) {
            u8 *v = AT(m, 0x50), *end = OFS(m, 0x1C);

            ENG_COST(4);
            for (;;) {
                ENG_COST(2);
                if (v == end) {
                    break;
                }
                ENG_COST(5);
                B(v, 0xC) = 0xFF;
                B(v, 0xD) = 0;
                B(v, 0xE) = 0;
                v += 0x10;
            }
        }
    }
    ENG_COST(6);
}

/* func_802A2164: in level 0x31, model 0x24 gets unk4 = 2 */
static void model_fix(u8 *m, s32 n) {
    ENG_COST(8);
    if (D_802E8BDC == 0x31) {
        ENG_COST(3);
        if (n == 0x24) {
            ENG_COST(2);
            B(m, 4) = 2;
        }
    }
    ENG_COST(5);
}

/* func_802A1EC8: this level's table of building groups (D_803BE708, its
   cursor D_803BE704) */
extern u8 D_802D30D0[], D_802D3194[], D_802D32A0[], D_802D331C[], D_802D33C8[], D_802D3444[],
    D_802D3538[], D_802D3614[], D_802D36C0[], D_802D3784[], D_802D3890[], D_802D393C[],
    D_802D3A00[], D_802D3A4C[], D_802D3BA0[], D_802D3BD4[], D_802D3CE0[], D_802D3DD4[],
    D_802D3EB0[], D_802D3F74[];
static const struct { s32 level; u8 *table; } level_tables[] = {
    { 0x00, D_802D30D0 }, { 0x01, D_802D3194 }, { 0x02, D_802D32A0 }, { 0x03, D_802D331C },
    { 0x04, D_802D33C8 }, { 0x05, D_802D3444 }, { 0x09, D_802D3538 }, { 0x0A, D_802D3614 },
    { 0x0C, D_802D36C0 }, { 0x0D, D_802D3784 }, { 0x0E, D_802D3890 }, { 0x0F, D_802D393C },
    { 0x10, D_802D3A00 }, { 0x11, D_802D3A4C }, { 0x12, D_802D3BA0 }, { 0x1A, D_802D3BD4 },
    { 0x1D, D_802D3CE0 }, { 0x21, D_802D3DD4 }, { 0x39, D_802D3EB0 }, { 0x3A, D_802D3F74 },
};

static void level_table_select(void) {
    s32 i, n = sizeof(level_tables) / sizeof(level_tables[0]);

    ENG_COST(7);
    for (i = 0; i < n; i++) {
        if (i != 0) {
            ENG_COST(3);
        }
        if (level_tables[i].level == D_802E8BDC) {
            ENG_COST(3 + 5);
            D_803BE708 = level_tables[i].table;
            D_803BE704 = level_tables[i].table + 4;
            ENG_COST(5);
            return;
        }
    }
    ENG_COST(5 + 5);
    D_803BE704 = NULL;
    D_803BE708 = NULL;
}

/* func_802A23E0: a building's model's 0x38-byte records: byte 0x31
   through D_8030631F */
static void building_remap(Building *b) {
    u8 *m = (u8 *)b->model;
    u8 *p = OFS(m, 0x30), *end = OFS(m, 0x38);

    ENG_COST(14);
    for (;;) {
        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(6);
        B(p, 0x31) = D_8030631F[B(p, 0x31)];
        p += 0x38;
    }
    ENG_COST(8);
}

/* func_802A24BC: the three comm-point-like buildings (0xBA..0xBC) get
   texture 0xF81 and a collision object */
static void building_special(Building *b) {
    ENG_COST(36);
    if (b->unk30 == 0xBA || (ENG_COST(2), b->unk30 == 0xBB) ||
        (ENG_COST(2), b->unk30 == 0xBC)) {
        ENG_COST(2);
        D_80365330 = eng_tex_get(0xF81, eng_leak.fp);
        ENG_COST(7);
        b->unk44 = func_802CE6F8(b->x, b->z, b->y);
        ENG_COST(1);
    }
    ENG_COST(33);
}

/* func_802A21AC: building `n` (model `m`) at (x, y, z) */
static void building_new(u8 *m, s32 n, s32 x, s32 y, s32 z, u8 flag, u32 unk34) {
    Building *b;
    s32 i, cell;
    u8 *src, *end;
    CollisionTri *t;

    ENG_COST(6);
    model_tex_anims_load(m);
    ENG_COST(3);
    if (n == 0x38) {
        ENG_COST(5);
        D_803F767C = x;
        D_803F767E = y;
        D_803F7680 = z;
    }
    ENG_COST(6);
    b = D_803F7654;
    D_803F7654 = b + 1;
    {
        /* func_802A2458: its centre */
        u8 *bb = OFS(m, 0x1C);

        ENG_COST(25);
        b->unk28 = ((H(bb, 0) + H(bb, 6)) >> 1) << 5;
        b->unk2C = ((H(bb, 4) + H(bb, 0x10)) >> 1) << 5;
    }
    ENG_COST(23);
    b->unk34 = unk34;
    b->unk38 = 0;
    b->unk3C = 0;
    b->unk40 = 0;
    b->unk30 = n;
    b->unkEB = flag;
    b->unkEA = 0;
    b->model = (struct Model *)m;
    b->unkC = UH(m, 2) << 5;
    b->x = b->unk1C = x << 5;
    b->y = b->unk20 = y << 5;
    b->z = b->unk24 = z << 5;
    b->unkE9 = UH(m, 0);
    for (i = 0;; i++) {
        ENG_COST(2);
        if (i == UH(m, 0)) {
            break;
        }
        ENG_COST(4);
        B(b, 0xEC + i) = 0;
    }
    ENG_COST(2);
    for (i = 0;; i++) {
        ENG_COST(2);
        if (i == UH(m, 0)) {
            break;
        }
        ENG_COST(4);
        H(b, 0x48 + i * 2) = 0;
    }
    ENG_COST(2);
    for (i = 0;; i++) {
        ENG_COST(2);
        if (i == UH(m, 0)) {
            break;
        }
        ENG_COST(4);
        H(b, 0x68 + i * 2) = 0;
    }
    ENG_COST(4);
    if (D_803BE704 != NULL) {
        ENG_COST(2);
        if (flag != 0) {
            ENG_COST(4);
            UW(D_803BE704, 0) = ENG_ADDR(b);
            D_803BE704 += 0x18;
        }
    }
    ENG_COST(7 + 20);
    cell = (u32)z / ((u32)D_803BE710 >> 5) * D_803BE714 + (u32)x / ((u32)D_803BE70C >> 5);
    b->unkE8 = cell;
    ENG_COST(3);
    building_special(b);
    ENG_COST(2);
    building_remap(b);
    ENG_COST(9);
    src = OFS(m, 0x48);
    end = OFS(m, 0x4C);
    t = (CollisionTri *)D_80358070;
    b->unk4 = t;
    for (;;) {
        ENG_COST(2);
        if (src == end) {
            break;
        }
        ENG_COST(3);
        if (src[0x17] != 0) {
            ENG_COST(2);
            t->unk51 = 0;
        } else {
            ENG_COST(1);
            t->unk51 = 1;
        }
        ENG_COST(5);
        /* $v0 still holds &D_803F7654 here */
        eng_collision_tri_init(t, src, unk34, (u8)ENG_ADDR(&D_803F7654), src[0x15], src[0x18],
                               src[0x14], src[0x17], src[0x16]);
        t++;
        ENG_COST(2);
        src += 0x19;
    }
    ENG_COST(8);
    b->unk8 = t;
    D_80358070 = (u8 *)t;
    eng_leak.s1 = src[-0x19 + 0x16];    /* (the last triangle's, if any) */
}

/* func_802A1D54: the buildings (LevelHeader.buildings, 14-byte records:
   x, y, z as big-endian u16, the model number, behaviour, two flags,
   unk34) */
static void buildings_load(LevelHeader *h) {
    u8 *p, *end;

    ENG_COST(5);
    D_8036EB93 = 0;
    level_table_select();
    {
        u8 *q;
        s32 i;

        ENG_COST(3);
        for (i = 1, q = D_803EFED0;; q += 0xA30) {
            ENG_COST(2);
            if (i == 0) {
                break;
            }
            ENG_COST(5);
            i--;
            B(q, 0xA2A) = 0;
            B(q, 0xA2B) = 0;
        }
        ENG_COST(3);
        for (i = 4, q = D_803F0900;; q += 0x4B8) {
            ENG_COST(2);
            if (i == 0) {
                break;
            }
            ENG_COST(5);
            i--;
            H(q, 0x4B0) = 0;
            B(q, 0x4B2) = 0;
        }
        ENG_COST(3);
        for (i = 2, q = D_803F1BE0;; q += 0x478) {
            ENG_COST(2);
            if (i == 0) {
                break;
            }
            ENG_COST(5);
            i--;
            B(q, 0x470) = 0;
            B(q, 0x472) = 0;
        }
    }
    ENG_COST(6);
    D_803F7654 = D_803F4030;
    model_table_load();
    ENG_COST(4);
    p = OFS(h, 0x5C);
    end = OFS(h, 0x60);
    for (;;) {
        s32 n;
        u8 *m;

        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(5);
        n = BE16U(p + 6);
        m = model_load(n);
        ENG_COST(2);
        model_fix(m, n);
        ENG_COST(6);
        eng_tex_fix_dl((u32 *)OFS(m, 0x10), (u32 *)OFS(m, 0x14));
        ENG_COST(15);
        UH(m, 0xE) = BE16U(p + 0xA);
        model_move(m, BE16U(p + 0), BE16U(p + 2), BE16U(p + 4));
        ENG_COST(2);
        model_tint(m, p[8]);
        ENG_COST(10);
        B(m, 6) = p[9];
        D_8036EB93 += p[9];
        eng_leak.t9 = BE16U(p + 0xC);
        building_new(m, n, BE16U(p + 0), BE16U(p + 2), BE16U(p + 4), p[8], BE16U(p + 0xC));
        ENG_COST(2);
        p += 0xE;
    }
    ENG_COST(4);
}

/* ---- vehicles ------------------------------------------------------------- */

/* func_802A3824: the vehicle the player starts in: the one at the
   driver's (type 0's) position (in the attract mode and the like, the
   chosen one, D_803643D4) */
static void player_vehicle_find(LevelHeader *h) {
    u8 *p = OFS(h, 0x50);
    u8 *end = OFS(h, 0x54);
    s32 x, y, z;

    ENG_COST(14);
    do {
        ENG_COST(4);
        p += 9;
    } while (p[-9] != 0);
    ENG_COST(17);
    x = BE16S(p - 8);
    y = BE16S(p - 6);
    z = BE16S(p - 4);
    for (p = OFS(h, 0x50);; p += 9) {
        u32 type;

        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(3);
        type = p[0];
        if (type == 0) {
            goto next;
        }
        ENG_COST(6);
        if (BE16S(p + 1) != x) {
            goto next;
        }
        ENG_COST(6);
        if (BE16S(p + 3) != y) {
            goto next;
        }
        ENG_COST(6);
        if (BE16S(p + 5) != z) {
            goto next;
        }
        ENG_COST(5);
        if (D_80364AA8 != 1) {
            ENG_COST(2);
            if (D_80364AA8 != 0x80) {
                ENG_COST(2);
                type = D_803643D4;
            }
        }
        ENG_COST(2);
        D_803BE73A = type;
    next:
        ENG_COST(2);
    }
    ENG_COST(11);
}

/* the vehicle modules' init functions by type (func_802A350C) */
static const u32 vehicle_inits[] = {
    0x802AE370, 0x802AFC60, 0x802B0DA0, 0x802B29C0, 0x802B4100, 0x802B5900, 0x802BAD80,
    0x802BBA60, 0x802B7340, 0x802C5120, 0x802C9B90,
};

/* func_802A350C: the level's vehicles (LevelHeader.vehicles, 9-byte
   records: type, x, y, z, heading as big-endian s16) */
static void vehicles_load(LevelHeader *h) {
    u8 *p, *end;

    ENG_COST(4);
    player_vehicle_find(h);
    ENG_COST(9);
    D_803ED3F5 = 0;
    D_803ED40F = 0;
    p = OFS(h, 0x50);
    end = OFS(h, 0x54);
    for (;;) {
        s32 type, x, y, z, heading;
        u8 *m;
        u32 init;

        ENG_COST(2);
        if (p == end) {
            break;
        }
        ENG_COST(6);
        type = p[0];
        if (D_80364A98 == 2) {
            ENG_COST(3);
            if (D_80364A98 == D_8030D888) {
                ENG_COST(4);
                if (D_8036698C != type) {
                    ENG_COST(2);
                    p += 9;
                    continue;
                }
            }
        }
        ENG_COST(3);
        if (type == 1) {
            ENG_COST(5);
            if (D_80364AA8 != 1) {
                ENG_COST(2);
                if (D_80364AA8 != 0x80) {
                    ENG_COST(2);
                    type = D_803643D4;
                }
            }
        }
        ENG_COST(3);
        if (type == 6 || (ENG_COST(2), type == 7) || (ENG_COST(2), type == 0xB) ||
            (ENG_COST(2), type == 0x11) || (ENG_COST(2), type == 0x12)) {
            ENG_COST(3);
            D_803ED40F = 1;
        }
        ENG_COST(21);
        x = BE16S(p + 1) << 5;
        y = BE16S(p + 3) << 5;
        z = BE16S(p + 5) << 5;
        heading = BE16S(p + 7);
        p += 9;
        m = eng_vehicle_model_load(type);
        ENG_COST(2);
        if (type <= 10) {
            ENG_COST(type * 3);
            ENG_COST(type == 0 ? 0 : 0);
            init = vehicle_inits[type];
        } else if (type == 0xB || type == 0x11 || type == 0x12) {
            ENG_COST(10 * 3 + 3 + (type == 0xB ? 0 : type == 0x11 ? 2 : 4));
            init = 0x802C80D0;
        } else {
            static const struct { s32 type; u32 init; } rest[] = {
                { 0xD, 0x802CB720 }, { 0xE, 0x802CC920 }, { 0xF, 0x802CF6A0 }, { 0x10, 0x802D07E0 },
            };
            s32 i;

            ENG_COST(10 * 3 + 3 + 2 + 2);
            for (i = 0;; i++) {
                ENG_COST(3);
                if (i == 4) {
                    ENG_COST(1 - 3);
                    eng_trap_syscall();
                }
                if (rest[i].type == type) {
                    break;
                }
            }
            init = rest[i].init;
        }
        ENG_COST(2);
        eng_a_vehicle_init(init, type, x, y, z, heading, m, h, p);
        ENG_COST(2);
    }
    ENG_COST(5);
}

/* ---- the carrier and the other one-offs ------------------------------- */

/* func_802A303C: the carrier, from LevelHeader.carrier */
static void carrier_load(LevelHeader *h) {
    u8 *p = OFS(h, 0x54);
    u8 n = p[0];

    ENG_COST(8);
    if (n != 0) {
        u8 *m;

        ENG_COST(21);
        m = eng_vehicle_model_load(0xFF);
        ENG_COST(2);
        eng_a_802B9C50(BE16S(p + 1) << 5, BE16S(p + 3) << 5, BE16S(p + 5),
                       BE16S(p + 7) << 5, n, m);
        ENG_COST(4);
        D_803643DB = 1;
    }
    ENG_COST(5);
}

/* func_802A30DC: the chopper (8DDB0), where the level has one */
static void chopper_load(void) {
    ENG_COST(7);
    if (func_80268EE8(D_802E8BDC) != 0) {
        u8 *m;

        ENG_COST(2 + 5);
        D_80364AC1 = 1;
        m = eng_vehicle_model_load(0xFD);
        ENG_COST(2);
        eng_a_802D2570(m);
    } else {
        ENG_COST(2);
    }
    ENG_COST(6);
}

/* func_802A3134: the J-Bomb's ... (72B80's func_802B8480) */
static void hotrod_load(LevelHeader *h) {
    ENG_COST(8);
    if (B(OFS(h, 0x54), 0) != 0) {
        ENG_COST(5);
        if (D_80364AA8 != 0x80) {
            u8 *m;

            ENG_COST(2);
            m = eng_vehicle_model_load(0xFE);
            ENG_COST(2);
            eng_a_802B8480(m);
            ENG_COST(3);
            D_803643DC = 1;
        }
    }
    ENG_COST(5);
}

/* func_802A3198: the carrier's cargo model (D_8039CA7E): display lists,
   identity matrices after them, and its own setup (func_80295AE0) */
static void cargo_load(void) {
    ENG_COST(6);
    if (D_8039CA61 != 0) {
        u8 *m, *dst, *src, *end;
        s32 left;

        ENG_COST(3);
        m = cargo_model_load(D_8039CA7E);
        ENG_COST(21);
        D_803BDAFC = m;
        D_803BDB04 = OFS(m, 0x14);
        D_803BDB08 = OFS(m, 0x24);
        dst = D_80358070;
        D_803BDB00 = dst;
        src = OFS(m, 0x18);
        left = W(src, 0);
        end = src + 8 + W(src, 4);
        src += 8;
        for (;;) {
            ENG_COST(2);
            if (src == end) {
                break;
            }
            ENG_COST(6);
            UW(dst, 0) = UW(src, 0);
            src += 4;
            dst += 4;
            left -= 4;
        }
        for (;;) {
            ENG_COST(2);
            if (left == 0) {
                break;
            }
            ENG_COST(23);
            H(dst, 0x00) = 1; H(dst, 0x02) = 0; W(dst, 0x04) = 0;
            H(dst, 0x08) = 0; H(dst, 0x0A) = 1; W(dst, 0x0C) = 0;
            W(dst, 0x10) = 0; H(dst, 0x14) = 1; H(dst, 0x16) = 0;
            W(dst, 0x18) = 0; H(dst, 0x1C) = 0; H(dst, 0x1E) = 1;
            W(dst, 0x20) = 0; W(dst, 0x24) = 0; W(dst, 0x28) = 0; W(dst, 0x2C) = 0;
            W(dst, 0x30) = 0; W(dst, 0x34) = 0; W(dst, 0x38) = 0; W(dst, 0x3C) = 0;
            dst += 0x40;
            left -= 0x40;
        }
        ENG_COST(8);
        D_80358070 = dst;
        func_80295AE0(OFS(m, 0x24), OFS(m, 0x28));
        ENG_COST(1);
    }
    ENG_COST(5);
}

/* ---- the loader ----------------------------------------------------------- */

/* func_802A1674 (00000.c's): load the level `h` */
void func_802A1674(LevelHeader *h, s32 arg1) {
    ENG_LEAK_ENTRY(&eng_leak);
    ENG_COST(23);
    D_803BE6F4 = arg1;
    level_globals(h);
    ENG_COST(2);
    func_802A0700();
    ENG_COST(2);
    {
        /* func_802A3008: the level's display lists' textures */
        ENG_COST(8);
        eng_tex_fix_dl((u32 *)OFS(h, 0x78), (u32 *)OFS(h, 0x84));
        eng_leak.s1 = ENG_ADDR(OFS(h, 0x84));
        ENG_COST(5);
    }
    ENG_COST(2);
    tex_anims_load(h);
    ENG_COST(2);
    level_dls(h);
    ENG_COST(2);
    eng_a_8029DEA0();
    ENG_COST(2);
    collision_grid(h, 0x6C, (CollisionTri **)D_803BDCA8);
    ENG_COST(2);
    collision_grid(h, 0x70, (CollisionTri **)D_803BDE40);
    ENG_COST(2);
    walls_load(h);
    ENG_COST(2);
    objects68_load(h);
    ENG_COST(2);
    terrain_groups(h);
    ENG_COST(2);
    level_mtx(h);
    ENG_COST(2);
    boxes_init();
    ENG_COST(2);
    eng_a_8029DC80();
    ENG_COST(2);
    eng_vis_reset();
    ENG_COST(2);
    level_unk60(h);
    ENG_COST(2);
    eng_anim_slots_reset();
    ENG_COST(2);
    eng_bc840();
    ENG_COST(2);
    buildings_load(h);
    ENG_COST(2);
    eng_c049c();
    ENG_COST(7);
    func_8028FDA0(OFS(h, 0x3C), OFS(h, 0x40));
    ENG_COST(6);
    if (D_80364A98 != 0x80) {
        ENG_COST(2);
        vehicles_load(h);
    }
    ENG_COST(11);
    D_80364AC1 = 0;
    D_803643DB = 0;
    D_803643DC = 0;
    if (D_80364A98 == 2) {
        ENG_COST(4);
        if (D_802E8BEC == 0) {
            ENG_COST(2);
            carrier_load(h);
            ENG_COST(2);
        }
    } else {
        ENG_COST(3);
        if (D_80364A98 != 0x80) {
            ENG_COST(4);
            if (D_803BE6F4 == 0) {
                ENG_COST(2);
                carrier_load(h);
                ENG_COST(2);
                chopper_load();
            }
        }
        ENG_COST(2);
        hotrod_load(h);
    }
    ENG_COST(2);
    cargo_load();
    ENG_COST(2);
    eng_comm_point_load(h);
    ENG_COST(2);
    extra_model_load();
    ENG_COST(15);
    D_803BE6FC = (struct LevelUnk58 *)OFS(h, 0x58);
    D_803BE700 = (struct LevelUnk58 *)OFS(h, 0x5C);
    func_8026FBB0(OFS(h, 0x34), OFS(h, 0x38));
    ENG_COST(5);
    func_8028D4C0(OFS(h, 0x38), OFS(h, 0x3C));
    ENG_COST(5);
    func_8028C190(OFS(h, 0x20), OFS(h, 0x24));
    ENG_COST(7);
    D_80370C50 = 0;
    if (D_80364A98 != 2) {
        ENG_COST(4);
        if (D_80364A98 != D_8030D880) {
            ENG_COST(4);
            if (D_803BE6F4 != 0) {
                ENG_COST(2);
                eng_a_802C4BF0();
            }
        }
    }
    ENG_COST(20);
}
