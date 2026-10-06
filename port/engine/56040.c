/*
 * hd_code 56040 (us.v11 0x8029A800-0x802A06F8): the vehicles' parts and
 * their collisions, as native C (engine.h).
 *
 *   - Animations.  A vehicle's parts are 32 Part records (vehicle.h's
 *     UnkStruct_803ED460), each the state of one animation (Anim below);
 *     the vehicle modules set them through the helpers here, by index or,
 *     for the texture animations in D_803B35F8, by their data's address.
 *     func_8029E558 steps a vehicle's animations each frame and builds its
 *     parts' matrices from key frames or a spline; func_8029E0AC steps the
 *     texture animations and patches the models' display lists.
 *   - Collisions.  Each frame a vehicle module calls func_8029A800 (the
 *     frame's state), func_8029C52C (its spheres against the other
 *     vehicles') and func_8029AA10 (its spheres against the buildings'
 *     pieces and the level's triangles, collision.h).
 *   - The objects' triangles: built from their models (func_8029D24C) and
 *     switched on and off by D_803BC1D0's records (func_8029D040).
 *
 * Angles are 12-bit (0x1000 a turn); positions the game's << 5, and >> 2
 * of that in the collision triangles and spheres.
 */
#include "shared.h"
#include "collision.h"
#include "game/vehicle.h"

/* ---- the records ---------------------------------------------------------- */

/* A Part as the state of an animation (the same 0x18 bytes as vehicle.h's
   UnkStruct_803ED460; func_8029F1BC steps it) */
typedef struct Anim {
    /* 0x00 */ u8 *PTR32 data;      /* AnimData (key frames), or a TexAnim */
    /* 0x04 */ f32 t;               /* the fraction from key `key` to the next */
    /* 0x08 */ f32 tension;         /* the spline's (func_8029F110) */
    /* 0x0C */ s16 loops;           /* the times it went past an end */
    /* 0x0E */ s16 limit;           /* the loops it stops at; -1 never */
    /* 0x10 */ s8 running;
    /* 0x11 */ s8 back;             /* running backwards */
    /* 0x12 */ s8 mode;             /* at an end: ANIM_WRAP or ANIM_BOUNCE */
    /* 0x13 */ s8 key;
    /* 0x14 */ u8 speed;            /* the rate's multiplier */
    /* 0x15 */ u8 interp;           /* ANIM_KEYS or ANIM_SPLINE */
    /* 0x16 */ u8 pad16[2];
} Anim;
SIZE_CHECK(Anim, 0x18);
#define ANIM(p) ((Anim *)(p))

#define ANIM_WRAP 0                     /* Anim.mode: start again */
#define ANIM_BOUNCE 1                   /* ... turn round */
#define ANIM_KEYS 0                     /* Anim.interp: straight between two keys */
#define ANIM_SPLINE 1                   /* ... a cardinal spline through four */

/* An animation's data (Anim.data): n keys, a speed per key boundary (the
   rate between key k and k + 1 goes from speed[k] to speed[k + 1]), the
   parts' count, then from the next word each part's record: its matrix's
   offset and its rest matrix's (from the parts' matrices' base), then n
   keys.  ANIM_PARTS(d) is the first record, ANIM_STRIDE(d) their size. */
#define ANIM_KEYS_N(d) ((d)[0])
#define ANIM_SPEED(d, k) ((d)[(k) + 1])
#define ANIM_NPARTS(d) ((d)[ANIM_KEYS_N(d) + 1])
#define ANIM_STRIDE(d) (ANIM_KEYS_N(d) * (s32)sizeof(Key) + 8)
typedef struct AnimPart {
    /* 0x00 */ s32 mtx;             /* the part's matrix (Mtx, the RSP's form) */
    /* 0x04 */ s32 rest;            /* its rest matrix (16.16, then its Mtx at 0x40) */
    /* 0x08 */ u8 keys[1];          /* Key[n] (not aligned to their size) */
} AnimPart;

/* a key frame: scale (in 256ths), rotation about x, y and z, translation */
typedef struct Key {
    /* 0x00 */ s16 scale[3];
    /* 0x06 */ s16 rot[3];
    /* 0x0C */ s16 pos[3];
    /* 0x12 */ s16 pad;
} Key;
SIZE_CHECK(Key, 0x14);
#define KEY_HALF(k, i) (((s16 *)(k))[i])   /* the key's i-th s16 */

/* the first part record of an animation's data: after its counts,
   aligned to a word */
#define ANIM_MISALIGNED(d) ((u32)((d) + ANIM_KEYS_N(d) + 2) & 3)
static AnimPart *anim_parts(u8 *d) {
    u8 *r = d + ANIM_KEYS_N(d) + 2;

    if ((u32)r & 3)
        r += 4 - ((u32)r & 3);
    return (AnimPart *)r;
}

/* A texture animation's data (an Anim's for D_803B35F8): the vehicle type
   whose model it patches, n keys, the textures a key changes (c), whether
   it blends each into the next key's (by the fraction), n + 1 rates (u16,
   from 4), then each key's c texture kinds. */
typedef struct TexAnim {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 n;
    /* 0x02 */ u8 c;
    /* 0x03 */ u8 second;
    /* 0x04 */ u16 rate[1];
} TexAnim;
#define TEXANIM_KINDS(T, k) ((u16 *)((u8 *)(T) + (T)->n * 2 + 4) + (T)->c * (k))

/* D_803A7440: the textures the texture animations have loaded */
typedef struct TexSlot {
    /* 0x0000 */ TexAnim *PTR32 owner;
    /* 0x0004 */ u16 kind;
    /* 0x0006 */ u8 used;
    /* 0x0007 */ u8 refs;           /* the animations that use it this frame */
    /* 0x0008 */ u8 pad8[8];
    /* 0x0010 */ u8 data[0x1000];   /* (the texture; its physical address is
                                       what goes into the display list) */
} TexSlot;
SIZE_CHECK(TexSlot, 0x1010);
#define TEX_SLOTS 12
extern TexSlot D_803A7440[TEX_SLOTS];

/* D_803B3500..D_803B35F0: where a model's display list loads an animated
   texture (a G_SETTIMG's word), by func_8029DF78 */
typedef struct TexPatch {
    /* 0x00 */ TexAnim *PTR32 anim;
    /* 0x04 */ s32 off;             /* the G_SETTIMG's address word, from the list's start */
    /* 0x08 */ s32 idx;             /* which of the frame's textures */
} TexPatch;
SIZE_CHECK(TexPatch, 0xC);
extern TexPatch D_803B3500[];
extern TexPatch *PTR32 D_803B35F0;      /* one past the last */

/* D_803B7FC8: the parts' matrices to copy from one frame's buffer to the
   other's (func_8029DDC8 does it when the frame comes round) */
typedef struct MtxCopy {
    /* 0x00 */ u32 *PTR32 from;      /* 0: free */
    /* 0x04 */ u32 *PTR32 to;
    /* 0x08 */ u32 frame;           /* the D_8035805C to copy at */
} MtxCopy;
SIZE_CHECK(MtxCopy, 0xC);
#define MTX_COPIES 0x78
extern MtxCopy D_803B7FC8[MTX_COPIES];
extern MtxCopy *PTR32 D_803B8568;       /* the last one in use */

/* D_803059F0: which building kinds each vehicle type collides with, on
   which level (0xFFFF: all), to a zero mask */
typedef struct BuildingRule {
    s32 level;
    s32 kind;                           /* Building.unk30 */
    u32 types;                          /* a bit per vehicle type */
} BuildingRule;
extern BuildingRule D_803059F0[];
#define ALL_LEVELS 0xFFFF
#define KIND_RDU 0x38                   /* (func_802BD8C8 says whether it collides) */

REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t7)
s32 func_8029BF64(s32 x0, s32 z0, s32 x1, s32 z1, s32 x2, s32 z2, s32 px, s32 pz);

/* ---- the parts, by index -------------------------------------------------- */

extern Part D_803B35F8[];

/* the record whose first word is `key` */
REGS(v0 -> v0)
Part *func_802A06B4(void *key) {
    Part *p = D_803B35F8;

    for (;;) {
        if (p->unk0 == key)
            break;
        p++;
    }
    return p;
}

/* ten halfwords from 20 bytes back (a key copied); returns the next */
REGS(t1 -> t1)
s16 *func_8029FF2C(s16 *d) {
    s32 k;

    for (k = 0; k < 10; k++)
        d[k] = d[k - 10];
    return d + 10;
}

/* start part i's animation, with its loops' limit */
REGS(v0, v1, a0)
void func_802A0290(s32 i, s32 v, Part *parts) {
    parts[i].unk10 = 1;
    parts[i].unkE = v;
    parts[i].unkC = 0;
}

REGS(v0, v1)
void func_802A02E4(s32 i, Part *parts) {
    parts[i].unk10 = 0;
}

REGS(v0, v1)
void func_802A0320(s32 i, Part *parts) {
    parts[i].unk13 = 0;
    parts[i].unk4 = 0.0f;
}

REGS(v0, v1, a0, f0)
void func_802A0360(s32 i, s32 v, Part *parts, f32 f) {
    parts[i].unk13 = v;
    parts[i].unk4 = f;
}

REGS(v0, v1, a0)
void func_802A039C(s32 i, s32 v, Part *parts) {
    parts[i].unk14 = v;
}

REGS(v0, v1, a0)
void func_802A03D4(s32 i, s32 v, Part *parts) {
    parts[i].unk11 = v;
}

REGS(v0, v1, a0)
void func_802A040C(s32 i, s32 v, Part *parts) {
    parts[i].unk12 = v;
}

REGS(v0, v1, a0)
void func_802A0444(s32 i, s32 v, Part *parts) {
    parts[i].unkC = 0;
    parts[i].unkE = v;
}

REGS(v0, v1, a0, f0)
void func_802A0480(s32 i, s32 v, Part *parts, f32 f) {
    parts[i].unk15 = v;
    parts[i].unk8 = f;
}

/* all of a part's fields, in registers */
REGS(v0, v1 -> v1, a0, a1, a2, a3, t0, t1, f0)
s32 func_802A04BC(s32 i, Part *parts, s32 *f11, s32 *f12, s32 *f14, s32 *fC, s32 *fE, s32 *f13, f32 *f4) {
    Part *p = &parts[i];

    *f11 = p->unk11;
    *f12 = p->unk12;
    *f14 = p->unk14;
    *fC = (u16)p->unkC;
    *fE = (u16)p->unkE;
    *f13 = p->unk13;
    *f4 = p->unk4;
    return p->unk10;
}

/* ---- the parts, by key (D_803B35F8) --------------------------------------- */

REGS(v0, v1)
void func_802A0508(void *key, s32 v) {
    Part *p;

    p = func_802A06B4(key);
    p->unk10 = 1;
    p->unkE = v;
    p->unkC = 0;
}

REGS(v0)
void func_802A0540(void *key) {
    Part *p;

    p = func_802A06B4(key);
    p->unk10 = 0;
}

REGS(v0)
void func_802A0570(void *key) {
    Part *p;

    p = func_802A06B4(key);
    p->unk13 = 0;
    p->unk4 = 0.0f;
}

REGS(v0, v1, f0)
void func_802A05A4(void *key, s32 v, f32 f) {
    Part *p;

    p = func_802A06B4(key);
    p->unk13 = v;
    p->unk4 = f;
}

REGS(v0, v1)
void func_802A05D0(void *key, s32 v) {
    Part *p;

    p = func_802A06B4(key);
    p->unk14 = v;
}

REGS(v0, v1)
void func_802A05F8(void *key, s32 v) {
    Part *p;

    p = func_802A06B4(key);
    p->unk11 = v;
}

REGS(v0, v1)
void func_802A0620(void *key, s32 v) {
    Part *p;

    p = func_802A06B4(key);
    p->unk12 = v;
}

REGS(v0, v1)
void func_802A0648(void *key, s32 v) {
    Part *p;

    p = func_802A06B4(key);
    p->unkC = 0;
    p->unkE = v;
}

/* (it loads the fields, as func_802A04BC does, but no caller reads them) */
REGS(v0)
void func_802A0674(void *key) {
    func_802A06B4(key);
}

/* ---- the triangles' small helpers ----------------------------------------- */

extern u8 D_803649ED, D_803A742B, D_803A7430;
extern u16 D_803A7410, D_803A7412;      /* the camera's limits (12-bit headings) */

#define TRAIN_HITS_MAX 0xC9             /* D_803A7430 counts the train's hits up to this */

/* the triangle's unk55: whether its owner isn't the train */
REGS(t2, s0)
void func_8029D534(s32 kind, CollisionTri *t) {
    if (kind != VEHICLE_TRAIN) {
        t->unk55 = 1;
    } else {
        t->unk55 = 0;
    }
}

/* the object triangle of this owner and id switched off; returns it */
REGS(t7, t6 -> s0)
CollisionTri *func_8029D1D4(s32 owner, s32 id) {
    CollisionTri *t = D_803B9890;

    for (;; t++) {
        if (t->owner != owner)
            continue;
        if (t->id == id)
            break;
    }
    t->active = 0;
    return t;
}

/* whether the first object triangle with this id is on */
REGS(t4 -> t5)
s32 func_8029D210(s32 id) {
    CollisionTri *t = D_803B9890;

    for (;; t++) {
        if (t->id == id)
            break;
    }
    return t->active;
}

/* D_803649ED = id, unless D_803EFECB and func_802AB41C(id, 7) */
REGS(s1)
void func_8029CF54(s32 id) {
    s32 skip = 0;

    if (D_803EFECB != 0) {
        skip = func_802AB41C(id, 7);
    }
    if (!skip) {
        D_803649ED = id;
    }
}

/* a 4x4 matrix (64 bytes) from src to dst */
REGS(t5, s2)
void func_8029DE50(u32 *dst, u32 *src) {
    s32 i;

    for (i = 8; i != 0; i--) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst += 2, src += 2;
    }
}

/* D_803B7FC8's matrix copies cleared, and D_803B8568 at the first */
void func_8029DC80(void) {
    MtxCopy *c = D_803B7FC8;
    s32 n;

    D_803B8568 = c;
    for (n = MTX_COPIES;; n--, c++) {
        if (n == 0)
            break;
        c->from = NULL;
        c->to = NULL;
    }
}

/* a hit on one of the train's triangles: D_803A742B set and D_803A7430
   counted */
REGS(s0)
void func_8029B5B8(CollisionTri *t) {
    if (t->owner == VEHICLE_TRAIN) {
        D_803A742B = 1;
        if (D_803A7430 < TRAIN_HITS_MAX) {
            D_803A7430++;
        }
    }
}

/* the camera's turn from D_803A7410 to D_803A7412 (12-bit), folded */
s32 func_8029B930(void) {
    s32 from = D_803A7410, to = D_803A7412, r;

    if (to < from) {
        r = to + (0xFFF - from);
    } else {
        r = to - from;
        if (r >= 0x801) {
            r -= 0x800;
        }
    }
    return r;
}

extern u8 D_8035805C;                   /* which of the two frames' buffers is drawn */

/* the crane's spheres (KindPart, kind VEHICLE_CRANE) are told apart by
   their radius: its hook's, and one func_8029CD54 leaves out */
#define CRANE_HOOK_R 0x3BD
#define CRANE_R_30E 0x30E

/* the crane's hook (its sphere of radius 0x3BD): whether there is one;
   its centre in *x, *y, *z */
static s32 crane_hook(s32 *x, s32 *y, s32 *z) {
    KindPart *p;

    for (p = D_803A6B30;; p++) {
        if (p->end == -1)
            break;
        if (p->kind != VEHICLE_CRANE)
            continue;
        if (p->r != CRANE_HOOK_R)
            continue;
        *x = p->x, *y = p->y, *z = p->z;
        return 1;
    }
    return 0;
}

/* whether there is one ($s4) */
REGS( -> s4)
s32 func_8029C6E4(void) {
    s32 x, y, z;

    return crane_hook(&x, &y, &z);
}

/* 0 if a triangle of this owner is off, else 1 */
REGS(v0 -> v1)
s32 func_8029DC14(s32 owner) {
    CollisionTri *t;

    for (t = D_803B9890;; t++) {
        if (t == D_803BD300)
            break;
        if (t->owner != owner)
            continue;
        if (t->active == 0) {
            return 0;
        }
    }
    return 1;
}

s32 func_8029DBF0(s32 owner) {
    s32 r;

    r = func_8029DC14(owner);
    return r;
}

/* 0 if one of the n (part, key) byte pairs has the part's key (its Anim's
   at parts, 0x18 each) equal to the given one, else 1 */
REGS(s4, t5, a2 -> t4)
s32 func_8029DB7C(u8 *pairs, s32 n, Anim *parts) {
    for (;; n--, pairs += 2) {
        if (n == 0)
            break;
        if (parts[pairs[0]].key == pairs[1]) {
            return 0;
        }
    }
    return 1;
}

/* the matrix copies to `to` cleared (the last one in use pulling
   D_803B8568 back) */
REGS(s2)
void func_8029DD54(u32 *to) {
    /* (sic: the original's end is D_803B8568's address, the word after the
       records, not its value: the loop goes one record past the last, onto
       the pointer and the word after it) */
    MtxCopy *c, *end = (MtxCopy *)&D_803B8568;

    for (c = D_803B7FC8;; c++) {
        if (end < c)
            break;
        if (c->to != to)
            continue;
        c->from = NULL;
        c->to = NULL;
        if (c == end) {
            D_803B8568 = c - 1;
        }
    }
}

/* ---- the animated textures' slots ----------------------------------------- */

/* the physical address of a slot's texture, as a G_SETTIMG takes it */
#define SLOT_PHYS(s) ((u32)(s)->data - 0x80000000)

/* the slot holding this texture kind for this animation: its refs
   cleared; its texture's physical address, 0 if none */
REGS(t6, t3 -> s1)
u32 func_8029E4E4(s32 kind, TexAnim *owner) {
    TexSlot *s = D_803A7440;
    s32 n;

    for (n = TEX_SLOTS;; n--, s++) {
        if (n == 0)
            break;
        if (s->used != 0) {
            if (s->kind == kind) {
                if (s->owner == owner) {
                    s->refs = 0;
                    return SLOT_PHYS(s);
                }
            }
        }
    }
    return 0;
}

/* A texture (kind, for this animation) into a free slot (the one past them
   when none is free) through func_802A1074; its physical address */
REGS(t3, t6, fp -> s1)
u32 func_8029E47C(TexAnim *owner, s32 kind, s32 fp) {
    TexSlot *s = D_803A7440;
    s32 n;

    for (n = TEX_SLOTS;; n--, s++) {
        if (n == 0) {
            break;
        }
        if (s->used == 0)
            break;
    }
    s->owner = owner;
    s->kind = kind;
    s->used = 1;
    s->refs = 0;
    func_802A1074(kind, (u32)s->data, fp);
    return SLOT_PHYS(s);
}

/* ---- collision tests ------------------------------------------------------ */

/* whether the triangle's first corner is within r of (x, y, z) */
REGS(s0, t3, t4, t5, t6 -> t7)
s32 func_8029BEE4(CollisionTri *t, s32 x, s32 y, s32 z, s32 r) {
    s32 dx = t->v[0][0] - x, dy = t->v[0][1] - y, dz = t->v[0][2] - z;
    s64 dy2 = (s64)dy * dy, dz2 = (s64)dz * dz, d;

    d = engine_cvt_l_s(__builtin_sqrtf((f32)((s64)dx * dx + dy2 + dz2)));
    return r >= d;
}

/* a free matrix copy (from, to, for the other frame), D_803B8568 moved up
   to it; none when all 0x78 are in use */
REGS(s2, t5)
void func_8029DCD4(u32 *from, u32 *to) {
    MtxCopy *c = D_803B7FC8;
    s32 n;

    for (n = MTX_COPIES;; n--, c++) {
        if (n == 0) {
            return;
        }
        if (c->from == NULL)
            break;
    }
    c->from = from;
    c->to = to;
    c->frame = D_8035805C ^ 1;
    if (D_803B8568 < c) {
        D_803B8568 = c;
    }
}

/* func_8029C0DC: the triangle's corners and the point (x, y, z) seen along
   its axis (shared.h's FlatTri, what func_8029BF64 takes) */
void collision_flatten(const CollisionTri *t, s32 x, s32 y, s32 z, FlatTri *f) {
    s32 p[3], i, j;

    if (t->axis == 0) {
        i = 0, j = 1;                   /* (z dropped) */
    } else if (t->axis == 1) {
        i = 0, j = 2;                   /* (y) */
    } else {
        i = 1, j = 2;                   /* (x) */
    }
    p[0] = x, p[1] = y, p[2] = z;
    f->u0 = t->v[0][i], f->v0 = t->v[0][j];
    f->u1 = t->v[1][i], f->v1 = t->v[1][j];
    f->u2 = t->v[2][i], f->v2 = t->v[2][j];
    f->pu = p[i], f->pv = p[j];
}

/* func_8029BF64 on it: whether the point is inside the triangle */
s32 collision_flat_inside(const FlatTri *f) {
    return func_8029BF64(f->u0, f->v0, f->u1, f->v1, f->u2, f->v2, f->pu, f->pv);
}

/* the original's entry, with the results in $v0-$t1 where its callers took
   them; no native code calls it now (77E20's and 89250's use
   collision_flatten()), only the check build's translated callers */
REGS(s0, v1, a0, a1)
void func_8029C0DC(u8 *t, s32 x, s32 y, s32 z) {
    FlatTri f;

    collision_flatten((CollisionTri *)t, x, y, z, &f);
}

/* the matrix copies due this frame (frame D_8035805C) made and freed;
   D_803B8568 left at the last one still waiting */
void func_8029DDC8(void) {
    MtxCopy *c = D_803B7FC8, *end = D_803B8568, *last = c;
    u32 frame = D_8035805C;

    for (;; c++) {
        if (end < c)
            break;
        if (c->from == NULL)
            continue;
        if (c->frame != frame) {
            last = c;
            continue;
        }
        func_8029DE50(c->to, c->from);
        c->from = NULL;
        c->to = NULL;
    }
    D_803B8568 = last;
}

/* whether two spheres meet: centres (x1, y1, z1) and (x2, y2, z2), radii
   r1 and r2 */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t2)
s32 func_8029CFA4(s32 x1, s32 y1, s32 z1, s32 r1, s32 x2, s32 y2, s32 z2, s32 r2) {
    s32 dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
    f32 d, rr;

    rr = (f32)(r1 + r2);
    d = __builtin_sqrtf((f32)((s64)dx * dx + (s64)dy * dy + (s64)dz * dz));
    if (d < rr) {
        return 1;
    }
    return 0;
}

/* whether the sphere (x, y, z), r meets the building's bounding sphere
   (its position and unkC, >> 2) */
REGS(s0, t3, t4, t5, t6 -> v1)
s32 func_8029B514(Building *b, s32 x, s32 y, s32 z, s32 r) {
    s32 hit;

    hit = func_8029CFA4(x, y, z, r, b->x >> 2, b->y >> 2, b->z >> 2, b->unkC >> 2);
    return hit;
}

/* ---- the animations' splines and keys ------------------------------------- */

extern f32 D_803B3778[16];              /* the spline's basis, by columns of 4 */

/* with mode ANIM_SPLINE, D_803B3778 = the cardinal spline basis of the
   animation's tension */
REGS(t0, fp)
void func_8029F110(Anim *a, s32 mode) {
    f32 c = a->tension, nc = -c, c2;
    f32 *m = D_803B3778;

    if (mode == ANIM_SPLINE) {
        c2 = c * 2.0f;
        m[0] = nc, m[4] = 2.0f - c, m[8] = c - 2.0f, m[12] = c;
        m[1] = c2, m[5] = c - 3.0f, m[9] = 3.0f - c2, m[13] = nc;
        m[2] = nc, m[6] = 0.0f, m[10] = c, m[14] = 0.0f;
        m[3] = 0.0f, m[7] = 1.0f, m[11] = 0.0f, m[15] = 0.0f;
    }
}

extern f32 D_803B37B8, D_803B37BC;      /* t^2 and t^3 */
extern u8 D_803B7FC0, D_803B7FC1, D_803B7FC2, D_803B7FC3;  /* the spline's four keys */

/* the spline's step: t^2 and t^3 of t, and the four keys around key i of
   n (the n - 1 keys wrapping) */
REGS(f30, t2, t6)
void func_8029F060(f32 t, s32 i, s32 n) {
    s32 m = n - 1;
    f32 t2 = t * t, t3 = t2 * t;

    D_803B37B8 = t2;
    D_803B37BC = t3;
    if (i - 1 >= 0) {
        D_803B7FC0 = i - 1;
    } else {
        D_803B7FC0 = i - 1 + m;
    }
    D_803B7FC1 = i;
    if (i + 1 < m) {
        D_803B7FC2 = i + 1;
    } else {
        D_803B7FC2 = i + 1 - m;
    }
    if (i + 2 < m) {
        D_803B7FC3 = i + 2;
    } else {
        D_803B7FC3 = i + 2 - m;
    }
}

extern f32 D_8030D870;                  /* a full turn, in the keys' angles / 16 */

/* the angle from a to b (the keys' 16-bit angles) the fraction t of the
   way, the short way round, as a 16-bit angle */
REGS(a0, a1, f30 -> t4)
s32 func_8029F6B0(s32 a, s32 b, f32 t) {
    f32 turn = D_8030D870, fa, d;

    fa = (f32)a / 16.0f;
    d = (f32)b / 16.0f - fa;
    if (d < -2048.0f) {
        d = d + turn;
    } else {
        if (!(d <= 2048.0f)) {
            d = d - turn;
        }
    }
    d = d * t + fa;
    if (d < 0.0f) {
        d = d + turn;
    }
    if (!(d <= turn)) {
        d = d - turn;
    }
    return engine_cvt_w_s(d * 16.0f);
}

/* ---- the objects' triangles switched ----------------------------------------- */

/* whether one of owner's switches lists the level's triangle `id` */
static s32 switch_lists(s32 owner, s32 id) {
    TriSwitch *q, *end = D_803BD304;
    u8 *b;
    s32 n;

    for (q = D_803BC1D0;; q++) {
        if (q == end)
            return 0;
        if (q->owner == owner) {
            for (b = q->level, n = q->nlevel;; b++) {
                if (n == 0)
                    break;
                --n;
                if (*b == id)
                    return 1;
            }
        }
    }
}

/* The owner's triangles switched on, and the level's that one of its
   switches lists */
REGS(t2)
void func_8029D120(s32 owner) {
    CollisionTri *t, *end = D_803BD300;
    s32 on;

    for (t = D_803B9890;; t++) {
        if (t == end)
            break;
        if (t->owner == owner) {
            on = 1;
        } else {
            on = 0;
            if (t->owner == 0) {
                on = switch_lists(owner, t->id);
            }
        }
        if (on) {
            t->active = 1;
        }
    }
}

/* the spline at t: D_803B3778's columns weighted by the four points,
   times t^3, t^2, t and 1, summed */
REGS(f0, f2, f4, f6, f30 -> f8)
f32 func_8029E878(f32 p0, f32 p1, f32 p2, f32 p3, f32 t) {
    f32 *m = D_803B3778, s, r = 0.0f;
    s32 c;

    for (c = 3;; c--, m++) {
        s = m[0] * p0 + m[4] * p1 + m[8] * p2 + m[12] * p3;
        if (c == 3) {
            r = s * D_803B37BC;
        } else {
            if (c == 2) {
                r = r + s * D_803B37B8;
            } else {
                if (c != 1) {
                    r = r + s;
                    break;
                }
                r = r + s * t;
            }
        }
    }
    return r;
}

extern s32 D_803BE724, D_803BE728;      /* the grid's cell size, x and z (<< 5) */
extern s16 D_803BE72C;                  /* its cells in a row */
extern s16 D_803A7418[2];               /* the cells to test, to -1 */

/* D_803A7418 = the grid cell of (x, z) (positions >> 2), then -1 */
REGS(t3, t5)
void func_8029C284(s32 x, s32 z) {
    s32 cx, cz;

    ENGINE_DIV(cx, (s32)((u32)x << 2), D_803BE724, 8029C2AC, 8029C2C4);
    ENGINE_DIV(cz, (s32)((u32)z << 2), D_803BE728, 8029C2F0, 8029C308);
    D_803A7418[0] = cx + (s32)((u32)cz * (u32)(s32)D_803BE72C);
    D_803A7418[1] = -1;
}

extern s32 D_802C23B4[];                /* the texture animations (TexAnim *), to -1 */
#define TEXANIM_AT(i) (*(TexAnim *PTR32 *)&D_802C23B4[i])

/* The texture slots freed; D_803B35F8's records set up for the texture
   animations of D_802C23B4 (to a key of -1) */
void func_8029DEA0(void) {
    TexSlot *s = D_803A7440;
    Part *p;
    s32 n, *id = D_802C23B4, w;

    for (n = TEX_SLOTS;; n--, s++) {
        if (n == 0)
            break;
        s->used = 0;
    }
    D_803B35F0 = D_803B3500;
    for (p = D_803B35F8;; p++) {
        w = *id;
        *(s32 *)&p->unk0 = w;
        if (w == -1)
            break;
        id++;
        p->unk4 = 0.0f;
        p->unkC = 0;
        p->unkE = 0;
        p->unk10 = 0;
        p->unk11 = 0;
        p->unk12 = 0;
        p->unk13 = 0;
        p->unk14 = 0;
    }
}

/* ---- a part's matrix from its keys (D_803B3730) ----------------------------- */

extern s32 D_803B3730[16];              /* the matrix (16.16) */
#define FX_ONE 0x10000                  /* 1.0 in 16.16 */

static void mtx_identity(s32 *m) {
    m[0] = FX_ONE, m[1] = 0, m[2] = 0, m[3] = 0;
    m[4] = 0, m[5] = FX_ONE, m[6] = 0, m[7] = 0;
    m[8] = 0, m[9] = 0, m[10] = FX_ONE, m[11] = 0;
    m[12] = 0, m[13] = 0, m[14] = 0, m[15] = FX_ONE;
}

/* D_803B3730 = the rotation by the 16-bit angle a about x (0), y (1) or
   z (2): its sine (func_802AE160) and cosine (func_802AE104) */
static void mtx_rotation(s32 axis, s32 a) {
    s32 *m = D_803B3730, sn, cs;

    sn = func_802AE160((u32)a >> 4);
    cs = func_802AE104((u32)a >> 4);
    mtx_identity(m);
    if (axis == 0)
        m[5] = cs, m[6] = sn, m[9] = -sn, m[10] = cs;
    else if (axis == 1)
        m[0] = cs, m[2] = -sn, m[8] = sn, m[10] = cs;
    else
        m[0] = cs, m[1] = sn, m[4] = -sn, m[5] = cs;
}

/* D_803B3730 = the translation (x, y, z) */
static void mtx_translation(s32 x, s32 y, s32 z) {
    s32 *m = D_803B3730;

    mtx_identity(m);
    m[12] = (u32)x << 16, m[13] = (u32)y << 16, m[14] = (u32)z << 16;
}

/* D_803B3730 = the scale (x, y, z), in 256ths */
static void mtx_scale(s32 x, s32 y, s32 z) {
    s32 *m = D_803B3730;

    mtx_identity(m);
    m[0] = (s32)((u32)x << 16) >> 8;
    m[5] = (s32)((u32)y << 16) >> 8;
    m[10] = (s32)((u32)z << 16) >> 8;
}

#define SCALE_ONE 0x100                 /* a key's scale of 1 */

/* D_803B3730 = the translation from (x0, y0, z0) the fraction t of the way
   to (x1, y1, z1), unless that is 0 (0x10000, or 0) */
REGS(a0, a1, a2, a3, t0, t1, f30 -> a0)
s32 func_8029F3D0(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1, f32 t) {
    s32 x, y, z;

    x = engine_cvt_w_s((f32)(x1 - x0) * t);
    y = engine_cvt_w_s((f32)(y1 - y0) * t) + y0;
    z = engine_cvt_w_s((f32)(z1 - z0) * t) + z0;
    x += x0;
    if (x == 0) {
        if (y == 0) {
            if (z == 0) {
                return 0;
            }
        }
    }
    mtx_translation(x, y, z);
    return FX_ONE;
}

/* D_803B3730 = the rotation about x (F4B8), y (F560) or z (F608) by the
   angle from a to b the fraction t of the way (func_8029F6B0), unless
   that is 0 (0x10000, or 0) */
#define LERP_ROTATION(axis, a, b, t)                                                                          \
    do {                                                                                                     \
        s32 r_;                                                                                              \
        r_ = func_8029F6B0(a, b, t);                                                                         \
        if (r_ == 0) {                                                                                       \
            return 0;                                                                                        \
        }                                                                                                    \
        mtx_rotation(axis, r_);                                                                              \
        return FX_ONE;                                                                                       \
    } while (0)

REGS(a0, a1, f30 -> a0)
s32 func_8029F4B8(s32 a, s32 b, f32 t) {
    LERP_ROTATION(0, a, b, t);
}

REGS(a0, a1, f30 -> a0)
s32 func_8029F560(s32 a, s32 b, f32 t) {
    LERP_ROTATION(1, a, b, t);
}

REGS(a0, a1, f30 -> a0)
s32 func_8029F608(s32 a, s32 b, f32 t) {
    LERP_ROTATION(2, a, b, t);
}

/* the four keys' i-th halves on the spline at t (func_8029E878), as a
   word; the first two keys' as floats in *k0, *k1 */
static s32 spline_half(Key *p0, Key *p1, Key *p2, Key *p3, s32 i, f32 t) {
    return engine_cvt_w_s(func_8029E878((f32)KEY_HALF(p0, i), (f32)KEY_HALF(p1, i), (f32)KEY_HALF(p2, i),
                                        (f32)KEY_HALF(p3, i), t));
}

/* D_803B3730 = the rotation about x (E938), y (EA48) or z (EB58) by the
   keys' angle on the spline at t; whether it isn't 0 (else D_803B3730 is
   as it was) */
#define SPLINE_ROTATION(axis)                                                                                  \
    do {                                                                                                     \
        s32 a_;                                                                                              \
        a_ = spline_half(p0, p1, p2, p3, 3 + (axis), t);                                                     \
        if (a_ == 0) {                                                                                       \
            return 0;                                                                                        \
        }                                                                                                    \
        mtx_rotation(axis, a_);                                                                              \
        return 1;                                                                                            \
    } while (0)

REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029E938(Key *p0, Key *p1, Key *p2, Key *p3, f32 t) {
    SPLINE_ROTATION(0);
}

REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EA48(Key *p0, Key *p1, Key *p2, Key *p3, f32 t) {
    SPLINE_ROTATION(1);
}

REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EB58(Key *p0, Key *p1, Key *p2, Key *p3, f32 t) {
    SPLINE_ROTATION(2);
}

/* D_803B3730 = the scale from (x0, y0, z0) the fraction t of the way to
   (x1, y1, z1), unless that is 1 each way (0x10000, or 0) */
REGS(a0, a1, a2, a3, t0, t1, f30 -> a0)
s32 func_8029F760(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1, f32 t) {
    s32 x, y, z;

    x = engine_cvt_w_s((f32)(x1 - x0) * t);
    y = engine_cvt_w_s((f32)(y1 - y0) * t);
    z = engine_cvt_w_s((f32)(z1 - z0) * t) + z0;
    x += x0;
    y += y0;
    if (x == SCALE_ONE) {
        if (y == SCALE_ONE) {
            if (z == SCALE_ONE) {
                return 0;
            }
        }
    }
    mtx_scale(x, y, z);
    return FX_ONE;
}

/* D_803B3730 = the translation by the keys' position on the spline at t;
   whether it isn't 0 */
REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EC68(Key *p0, Key *p1, Key *p2, Key *p3, f32 t) {
    s32 x, y, z;

    x = spline_half(p0, p1, p2, p3, 6, t);
    y = spline_half(p0, p1, p2, p3, 7, t);
    z = spline_half(p0, p1, p2, p3, 8, t);
    if (x == 0) {
        if (y == 0) {
            if (z == 0) {
                return 0;
            }
        }
    }
    mtx_translation(x, y, z);
    return 1;
}

/* D_803B3730 = the scale by the keys' on the spline at t; whether it
   isn't 1 each way */
REGS(t5, t6, t7, s0, f30 -> a3)
s32 func_8029EDEC(Key *p0, Key *p1, Key *p2, Key *p3, f32 t) {
    s32 x, y, z;

    x = spline_half(p0, p1, p2, p3, 0, t);
    y = spline_half(p0, p1, p2, p3, 1, t);
    z = spline_half(p0, p1, p2, p3, 2, t);
    if (x == SCALE_ONE) {
        if (y == SCALE_ONE) {
            if (z == SCALE_ONE) {
                return 0;
            }
        }
    }
    mtx_scale(x, y, z);
    return 1;
}

/* A part's frame from key k and the next, the fraction t of the way: each
   matrix that isn't the identity into acc (func_802ACCCC), in the order
   scale, z, y, x, translation.  (fp: what it puts back, as the original
   reloads it) */
REGS(s3, s2, f30, fp)
void func_8029EF80(Key *k, s32 *acc, f32 t, s32 fp) {
    Key *n = k + 1;

    if (func_8029F760(k->scale[0], k->scale[1], k->scale[2], n->scale[0], n->scale[1], n->scale[2], t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029F608(k->rot[2], n->rot[2], t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029F560(k->rot[1], n->rot[1], t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029F4B8(k->rot[0], n->rot[0], t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029F3D0(k->pos[0], k->pos[1], k->pos[2], n->pos[0], n->pos[1], n->pos[2], t)) {
        func_802ACCCC(D_803B3730, acc);
    }
}

/* the spline's four keys of a part's record */
#define SPLINE_KEYS(rec, p0, p1, p2, p3)                                                                       \
    do {                                                                                                     \
        Key *k_ = (Key *)(rec)->keys;                                                                        \
        p0 = k_ + D_803B7FC0, p1 = k_ + D_803B7FC1, p2 = k_ + D_803B7FC2, p3 = k_ + D_803B7FC3;               \
    } while (0)

/* the same along the spline: the four keys at t, in the order scale, z, y,
   x, translation */
REGS(s1, s2, f30)
void func_8029E730(AnimPart *rec, s32 *acc, f32 t) {
    Key *p0, *p1, *p2, *p3;

    SPLINE_KEYS(rec, p0, p1, p2, p3);
    if (func_8029EDEC(p0, p1, p2, p3, t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029EB58(p0, p1, p2, p3, t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029EA48(p0, p1, p2, p3, t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029E938(p0, p1, p2, p3, t)) {
        func_802ACCCC(D_803B3730, acc);
    }
    if (func_8029EC68(p0, p1, p2, p3, t)) {
        func_802ACCCC(D_803B3730, acc);
    }
}

/* ---- an animation's step --------------------------------------------------- */

/* a key's speed: in 300ths of a key a frame (times the Anim's speed) */
#define ANIM_RATE_DIV 300.0f

/* the rate at fraction t between keys k and k + 1 of these speeds */
#define ANIM_RATE(lo, hi, t) (((f32)((hi) - (lo)) * (t) + (f32)(lo)) / ANIM_RATE_DIV)

/* An animation's step: the fraction t on by rate * speed (back, when it
   runs backwards), the key k on by its whole part, over n keys.  At an end
   it wraps (ANIM_WRAP) or turns round there (ANIM_BOUNCE), counting a
   loop; when the loops reach their limit it stops at that end (running
   cleared).  Returns the key, the fraction in *t_out. */
REGS(t0, t2, t6, f6, f30 -> t2, f30)
s32 func_8029F1BC(Anim *a, s32 k, s32 n, f32 rate, f32 t, f32 *t_out) {
    s32 speed = a->speed, back = a->back, mode = (u8)a->mode, w, c, lim, stop;
    f32 f;

    rate = rate * (f32)speed;
    if (back != 1) {
        t = t + rate;
        w = engine_trunc_w_s(t);
        t = t - (f32)w;
        k += w;
        if (k >= n - 1) {
            /* past the last key */
            if (n - 1 == 0) {
                engine_break(0x8029F21C, 7);
            }
            c = a->loops + (s32)((u32)k / (u32)(n - 1));
            lim = a->limit;
            a->loops = c;
            stop = 0;
            if (c >= lim) {
                stop = lim != -1;
            }
            if (stop) {
                t = 1.0f;
                k = n - 2;
                if (mode == ANIM_BOUNCE) {
                    back = 1;
                }
                a->running = 0;
            } else {
                if (mode != ANIM_WRAP) {
                    if (mode != ANIM_BOUNCE) {
                        engine_syscall(0x8029F27C);
                    }
                    t = 1.0f;
                    back = 1;
                    k = n - 2;
                } else {
                    if (n - 1 == 0) {
                        engine_break(0x8029F298, 7);
                    }
                    k = (u32)k % (u32)(n - 1);
                }
            }
        }
    } else {
        t = t - rate;
        if (t <= 0.0f) {
            f = (f32)engine_trunc_w_s(t) - 1.0f;
            t = t - f;
            k += engine_cvt_w_s(f);
        }
        if (k < 0) {
            /* before the first key */
            c = a->loops + back;
            lim = a->limit;
            if (n == 0) {
                engine_break(0x8029F328, 7);
            }
            a->loops = c;
            stop = 0;
            if (c >= lim) {
                stop = lim != -1;
            }
            if (stop) {
                t = 0.0f;
                k = 0;
                if (mode == ANIM_BOUNCE) {
                    back = 0;
                }
                a->running = 0;
            } else {
                if (mode != ANIM_WRAP) {
                    if (mode != ANIM_BOUNCE) {
                        engine_syscall(0x8029F374);
                    }
                    t = 0.0f;
                    back = 0;
                    k = 0;
                } else {
                    if (n - 1 == 0) {
                        engine_break(0x8029F394, 7);
                    }
                    k = (u32)(-k) % (u32)(n - 1);
                    if (k != 0) {
                        k = (n - 1) - k;
                    }
                }
            }
        }
    }
    a->back = back;
    a->key = k;
    a->t = t;
    *t_out = t;
    return k;
}

extern u8 *PTR32 D_803B3770;            /* the other frame's matrices' base */

/* An object's animation, one frame on: its state (func_8029F1BC; the rate
   from its data's speeds), then each part's matrix from its rest matrix
   with the key frames' (func_8029EF80) or the spline's (func_8029E730) on
   top, to the RSP's form, and its copy into the other frame's buffer
   queued (func_8029DCD4) or, when the animation has stopped, dropped
   (func_8029DD54).  base is the parts' matrices'. */
REGS(t0, v0)
void func_8029E5AC(Anim *a, u8 *base) {
    u8 *d = a->data;
    AnimPart *rec;
    s32 k = a->key, n, interp, running, parts, step, i;
    u32 *s, *w, *dst;
    f32 t = a->t;

    n = ANIM_KEYS_N(d);
    k = func_8029F1BC(a, k, n, ANIM_RATE(ANIM_SPEED(d, k), ANIM_SPEED(d, k + 1), t), t, &t);
    interp = a->interp;
    running = (u8)a->running;
    if (interp != ANIM_KEYS) {
        func_8029F110(a, interp);
        func_8029F060(t, k, n);
    }
    step = ANIM_STRIDE(d);
    parts = ANIM_NPARTS(d);
    for (rec = anim_parts(d);; parts--, rec = (AnimPart *)((u8 *)rec + step)) {
        if (parts == 0)
            break;
        dst = (u32 *)(base + rec->mtx);
        s = (u32 *)(base + rec->rest);
        for (w = dst, i = 8; i != 0; i--) {
            w[0] = s[0];
            w[1] = s[1];
            w += 2, s += 2;
        }
        if (interp == ANIM_KEYS) {
            func_8029EF80((Key *)rec->keys + k, (s32 *)dst, t, interp);
        } else {
            if (interp != ANIM_SPLINE) {
                engine_syscall(0x8029E6C0);
            }
            func_8029E730(rec, (s32 *)dst, t);
        }
        func_802ACCCC((s32 *)(base + rec->rest + 0x40), (s32 *)dst);
        func_802AC8CC(dst);
        if (running != 1) {
            func_8029DCD4(dst, (u32 *)(D_803B3770 + rec->mtx));
        } else {
            func_8029DD54(dst);
        }
    }
}

/* ---- an object's spheres ---------------------------------------------------- */

/* An object's spheres in its model (from the word after its bounding
   sphere's radius): a point, its radius and power, the matrices it goes
   through (their count and offsets) */
typedef struct ObjSphere {
    /* 0x00 */ s16 x, y, z;
    /* 0x06 */ u16 r;
    /* 0x08 */ u16 power;
    /* 0x0A */ u16 nmtx;
    /* 0x0C */ s32 mtx[1];
} ObjSphere;
#define OBJSPHERE_NEXT(s) ((ObjSphere *)((u8 *)(s) + (u32)(s)->nmtx * 4 + 0xC))
#define OBJSPHERE_FIRST(p) ((ObjSphere *)((u8 *)(p) + 4))

/* a size from the model's (<< 5, times scale / 0x10000) */
#define SCALED(v, scale) (((u32)(v) << 5) * (scale) / 0x10000)

/* An object's spheres (from p to end) into D_803A7300's first free record
   (its bounding sphere: the first u16 the radius) and D_803A6B30's (one
   per sphere: its radius and power), all of this kind and not placed */
REGS(t0, t1, t2, t3)
void func_8029C354(s32 kind, u8 *p, u8 *end, s32 scale_) {
    u32 scale = scale_, size;
    ObjSphere *s;
    Solid *b;
    KindPart *r;

    size = SCALED(*(u16 *)p, scale);
    for (b = D_803A7300;; b++) {
        if (b->end == -1)
            break;
    }
    b->r = size;
    b->kind = kind;
    b->end = 0;
    for (r = D_803A6B30;; r++) {
        if (r->end == -1)
            break;
    }
    for (s = OBJSPHERE_FIRST(p);; s = OBJSPHERE_NEXT(s)) {
        if ((u8 *)s == end)
            break;
        r->kind = kind;
        r->end = 0;
        r->power = s->power;
        r->r = SCALED(s->r, scale);
        r++;
    }
}

extern s32 D_803A73FC, D_803A7400, D_803A7404;  /* the last plane hit's point */

/* Whether (x, y, z) is within lim of the triangle's plane; if so, its foot
   on the plane (along the normal) in D_803A73FC-D_803A7404 and *px-*pz */
REGS(t3, t4, t5, t6, s0 -> v0, v1, a0, a1)
s32 func_8029C160(s32 x, s32 y, s32 z, s32 lim, CollisionTri *t, s32 *px, s32 *py, s32 *pz) {
    s64 a = t->nx, b = t->ny, c = t->nz, d = t->d;
    s64 dot, dist, s3, rx, ry, rz;
    f32 f;

    dot = (s64)((u64)a * (u64)(s64)x + (u64)b * (u64)(s64)y + (u64)c * (u64)(s64)z);
    dist = engine_cvt_l_s((f32)(s64)((u64)dot + (u64)d) / t->nlen);
    if (dist < 0) {
        dist = -dist;
    }
    if (lim < dist) {
        *px = *py = *pz = 0;            /* (none: its callers don't look) */
        return 0;
    }
    s3 = (s64)(0 - (u64)d - (u64)dot);
    f = (f32)s3 / t->nlen2;
    rx = (s64)((u64)engine_cvt_l_s(f * (f32)a) + (u64)(s64)x);
    ry = (s64)((u64)engine_cvt_l_s(f * (f32)b) + (u64)(s64)y);
    rz = (s64)((u64)engine_cvt_l_s(f * (f32)c) + (u64)(s64)z);
    D_803A73FC = *px = (s32)rx;
    D_803A7400 = *py = (s32)ry;
    D_803A7404 = *pz = (s32)rz;
    return 1;
}

/* Whether the point (x, z) at height y misses a switch's area: its n
   triangles and the height range after them (two halves, wrapping when
   the second is lower).  0 when it is inside a triangle and the range. */
REGS(s4, t5, t0, t1, a1 -> t4)
s32 func_8029DA90(s32 *tris, s32 n, s32 x, s32 z, s32 y) {
    s32 *t = tris, lo, hi, in;
    u32 range;

    for (;;) {
        if (n == 0) {
            return 1;
        }
        n--;
        t += 6;
        in = func_802AA5E0(x, z, t[-6], t[-5], t[-4], t[-3], t[-2], t[-1]);
        if (!in)
            continue;
        in = func_802AA460(x, z, t[-6], t[-5], t[-4], t[-3], t[-2], t[-1]);
        if (in)
            break;
    }
    /* inside this triangle: the range after the last (the halves of a word
       func_802A3F80 stores, read through the word, which is how they are in
       native-endian memory) */
    range = *(u32 *)(t + n * 6);
    lo = range >> 16;
    hi = range & 0xFFFF;
    if (hi < lo) {
        in = 1;
        if (y < lo) {
            if (hi < y) {
                in = 0;
            }
        }
    } else {
        in = 0;
        if (y >= lo) {
            in = y <= hi;
        }
    }
    return !in;
}

extern u8 D_803A742D, D_803A742E;       /* the gears' count, and whether it was stepped */
extern s32 D_80358064;                  /* the game's frame in this mode */

#define GEARS_MAX 0xC8
#define BACKUP_SPEED 10                 /* unk76 when it was 0 (after the first frame) */
#define HALF_TURN 0x800
#define TURN 0x1000

/* The gears (unkA0): D_803A742D (to 0xC8) when it was stepped this frame,
   else one down to 1; unk76 10 when it is 0 (but on the mode's first
   frame); and with unk76 negative, the camera's limits turned half round */
REGS(gp)
void func_8029A914(VS *vs) {
    s32 g, h;

    if (D_803A742E != 0) {
        g = D_803A742D;
        if (g > GEARS_MAX) {
            g = GEARS_MAX;
        }
        vs->unkA0 = g;
    } else {
        g = vs->unkA0;
        if (g != 1) {
            vs->unkA0 = g - 1;
        }
    }
    if (vs->unk76 == 0) {
        if (D_80358064 != 0) {
            vs->unk76 = BACKUP_SPEED;
        }
    }
    if (vs->unk76 < 0) {
        h = D_803A7410 - HALF_TURN;
        if (h < 0) {
            h += TURN - 1;
        }
        D_803A7410 = h;
        h = D_803A7412 + HALF_TURN;
        if (h > TURN - 1) {
            h -= TURN - 1;
        }
        D_803A7412 = h;
    }
}

#define G_SETTIMG_OP 0xFD
#define G_SETPRIMCOLOR_OP 0xFA
#define GFX_OP(w) (((w) & 0xFF000000) >> 24)

/* For each texture animation of this vehicle type, the G_SETTIMG commands
   of its display list (dl to end) that load its first texture: (the
   animation, the command's address word, texture 0) appended at
   D_803B35F0 */
REGS(s0, s1, s2)
void func_8029DF78(u32 dl_, u32 end_, u32 type) {
    u32 *dl = (u32 *)(__UINTPTR_TYPE__)dl_, *end = (u32 *)(__UINTPTR_TYPE__)end_;
    s32 *ids = D_802C23B4;
    TexAnim *T;
    TexPatch *out = D_803B35F0;
    u32 *p, w, tex;

    for (;;) {
        if (*ids == -1)
            break;
        T = *(TexAnim *PTR32 *)ids;
        ids++;
        if (T->type != type)
            continue;
        if (T->c == 0)
            continue;
        tex = TEXANIM_KINDS(T, 0)[0];
        for (p = dl;;) {
            if (p == end)
                break;
            w = p[0];
            p += 2;
            if (GFX_OP(w) != G_SETTIMG_OP)
                continue;
            if (p[-1] != tex)
                continue;
            out->anim = T;
            out->off = (u8 *)p - (u8 *)dl - 4;
            out->idx = 0;
            out++;
        }
    }
    D_803B35F0 = out;
}

/* Whether (px, pz) is inside the triangle (x0, z0), (x1, z1), (x2, z2): on
   the same side of each edge as a point inside it (between the third
   corner and the first edge's middle), an edge it lies on not counting */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t7)
s32 func_8029BF64(s32 x0, s32 z0, s32 x1, s32 z1, s32 x2, s32 z2, s32 px, s32 pz) {
    f32 fpx = (f32)px, fpz = (f32)pz, cx, cz, ax, az, ex, ez, side, inside;
    s32 e;

    cx = (f32)(x0 + x1) / 2.0f;
    cz = (f32)(z0 + z1) / 2.0f;
    cx = ((f32)x2 + cx) / 2.0f;
    cz = ((f32)z2 + cz) / 2.0f;
    /* the edges 0-1, 0-2 and 1-2 */
    for (e = 3; e != 0; e--) {
        if (e == 3) {
            ax = (f32)x0, az = (f32)z0;
            ez = (f32)(z1 - z0), ex = (f32)(x1 - x0);
        } else if (e == 2) {
            ax = (f32)x0, az = (f32)z0;
            ez = (f32)(z2 - z0), ex = (f32)(x2 - x0);
        } else {
            ax = (f32)x1, az = (f32)z1;
            ez = (f32)(z2 - z1), ex = (f32)(x2 - x1);
        }
        side = (fpx - ax) * ez - (fpz - az) * ex;
        if (side == 0.0f)
            continue;
        inside = (cx - ax) * ez - (cz - az) * ex;
        if (side > 0.0f) {
            if (inside > 0.0f)
                continue;
        } else {
            if (inside < 0.0f)
                continue;
        }
        /* on the other side of this edge */
        return 0;
    }
    return 1;
}

/* ---- an object's parts and matrices from its model ------------------------- */

/* An object's 32 parts from its model file (their animations' data at the
   offsets of the u16 list at model + its word 0x10), then its matrix block
   (model + its word 0x18: a size, a length, the words) into both frames'
   buffers, the rest of the size as identity matrices */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2) {
    Anim *a = ANIM(parts);
    u8 *base = model + *(s32 *)(model + 0x10), *blk, *end;
    u16 *off = (u16 *)base;
    s32 n, size;
    u32 w, *s;

    for (n = 0x20; n != 0; n--, a++, off++) {
        a->t = 0.0f;
        a->data = base + *off;
        a->loops = 0;
        a->limit = 0;
        a->running = 0;
        a->back = 0;
        a->mode = 0;
        a->key = 0;
        a->speed = 0;
        a->interp = 0;
    }
    blk = model + *(s32 *)(model + 0x18);
    size = *(s32 *)blk;
    end = blk + 8 + *(s32 *)(blk + 4);
    for (s = (u32 *)(blk + 8);; size -= 4) {
        if ((u8 *)s == end)
            break;
        w = *s++;
        *(u32 *)buf1 = w;
        *(u32 *)buf2 = w;
        buf1 += 4, buf2 += 4;
    }
    for (;; size -= 0x40) {
        if (size == 0)
            break;
        /* (the identity Mtx; by words, since its halves are a word's in
           native-endian memory, tools/recomp/native_sites.txt) */
        for (n = 0; n < 2; n++) {
            u32 *m = (u32 *)(n == 0 ? buf1 : buf2);

            m[0] = 0x00010000, m[1] = 0, m[2] = 0x00000001, m[3] = 0;
            m[4] = 0, m[5] = 0x00010000, m[6] = 0, m[7] = 0x00000001;
            m[8] = 0, m[9] = 0, m[10] = 0, m[11] = 0;
            m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 0;
        }
        buf1 += 0x40, buf2 += 0x40;
    }
}

/* the corner's position (the triangle's are the model's << 3, from there
   >> 3 again for the plane) */
#define CORNER(t, i, j) ((t)->v[i][j] >> 3)

/* The plane through the triangle's corners: its normal (a, b, c) as the
   cross product of two edges, and d */
REGS(s0 -> a1, a2, a3, t0)
s64 func_8029D90C(CollisionTri *t, s64 *b_out, s64 *c_out, s64 *d_out) {
    s32 x1 = CORNER(t, 0, 0), y1 = CORNER(t, 0, 1), z1 = CORNER(t, 0, 2);
    s32 x2 = CORNER(t, 1, 0), y2 = CORNER(t, 1, 1), z2 = CORNER(t, 1, 2);
    s32 x3 = CORNER(t, 2, 0), y3 = CORNER(t, 2, 1), z3 = CORNER(t, 2, 2);
    s64 dy2 = y1 - y2, dz3 = z1 - z3, dz2 = z1 - z2, dy3 = y1 - y3, dx3 = x1 - x3, dx2 = x1 - x2;
    s64 a, b, c;

    a = (s64)((u64)dy2 * (u64)dz3 - (u64)dz2 * (u64)dy3);
    b = (s64)((u64)dz2 * (u64)dx3 - (u64)dx2 * (u64)dz3);
    c = (s64)((u64)dx2 * (u64)dy3 - (u64)dy2 * (u64)dx3);
    *d_out = (s64)(0 - ((u64)a * (u64)(s64)x2 + (u64)b * (u64)(s64)y2 + (u64)c * (u64)(s64)z2));
    *b_out = b;
    *c_out = c;
    return a;
}

/* Whether one of the triangle's edges (the corners 0-1, 0-2, 1-2) passes
   within r of (x, y, z): the edge's line meets the sphere at a t in
   [0, 1] (either root of |a + t e - p|^2 = r^2) */
REGS(t3, t4, t5, t6, s0 -> t7)
s32 func_8029BD0C(s32 x, s32 y, s32 z, s32 r, CollisionTri *tri) {
    s32 *a, *b, e, ex, ey, ez, px, py, pz;
    u64 ee, pe2, cc = 0, disc = 0, q = 0;
    f32 root, num, den, t;

    for (e = 3;; e--) {
        if (e == 0)
            break;
        if (e == 3) {
            a = tri->v[0], b = tri->v[1];
        } else if (e == 2) {
            a = tri->v[0], b = tri->v[2];
        } else {
            a = tri->v[1], b = tri->v[2];
        }
        px = a[0] - x, ex = b[0] - a[0];
        py = a[1] - y, ey = b[1] - a[1];
        pz = a[2] - z, ez = b[2] - a[2];
        ee = (u64)(s64)ex * (u64)(s64)ex + (u64)(s64)ey * (u64)(s64)ey + (u64)(s64)ez * (u64)(s64)ez;
        pe2 = ((u64)(s64)ex * (u64)(s64)px + (u64)(s64)ey * (u64)(s64)py + (u64)(s64)ez * (u64)(s64)pz) << 1;
        cc = (u64)(s64)px * (u64)(s64)px + (u64)(s64)py * (u64)(s64)py + (u64)(s64)pz * (u64)(s64)pz -
             (u64)(s64)r * (u64)(s64)r;
        q = (ee * cc) << 2;
        disc = pe2 * pe2 - q;
        if ((s64)disc < 0)
            continue;
        root = __builtin_sqrtf((f32)(s64)disc);
        num = (f32)(s64)(0 - pe2);
        den = (f32)(s64)(ee << 1);
        t = (num + root) / den;
        if (!(t < 0.0f)) {
            if (!(t > 1.0f)) {
                break;
            }
        }
        t = (num - root) / den;
        if (t < 0.0f)
            continue;
        if (t > 1.0f)
            continue;
        break;
    }
    return e != 0;
}

extern u8 D_803A742C, D_803A742F;

/* The camera's limits (D_803A7410, D_803A7412) widened to take in a and b
   (12-bit, wrapped): set to them when unset (0 and 0xFFF).  When that
   widens the turn between them (func_8029B930) after the mode's first
   frame, D_803A742F is set, and with D_803A742C (and D_803A742E clear) the
   gears D_803A742D step on (1 to 8, else up one) and D_803A742E is set. */
REGS(a0, a1)
void func_8029B7CC(s32 a, s32 b) {
    s32 before, h0, h1;

    if (a < 0) {
        a += TURN - 1;
    }
    if (a >= TURN) {
        a -= TURN - 1;
    }
    if (b < 0) {
        b += TURN - 1;
    }
    if (b >= TURN) {
        b -= TURN - 1;
    }
    before = func_8029B930();
    h0 = D_803A7410;
    h1 = D_803A7412;
    if (h0 == 0) {
        if (h1 == TURN - 1) {
            D_803A7410 = a;
            D_803A7412 = b;
            return;
        }
    }
    /* the wider of each, compared as 12-bit angles (in the top bits) */
    h1 = (u32)h1 << 20;
    b = (u32)b << 20;
    h0 = (u32)h0 << 20;
    a = (u32)a << 20;
    if ((s32)((u32)b - h1) <= 0) {
        h1 = b;
    }
    if ((s32)((u32)a - h0) > 0) {
        h0 = a;
    }
    D_803A7410 = (u32)h0 >> 20;
    D_803A7412 = (u32)h1 >> 20;
    if (before < func_8029B930()) {
        if (D_80358064 != 0) {
            D_803A742F = 1;
            if (D_803A742C != 0) {
                if (D_803A742E == 0) {
                    if (D_803A742D == 1) {
                        D_803A742D = 8;
                    } else {
                        D_803A742D = D_803A742D + 1;
                    }
                    D_803A742E = 1;
                }
            }
        }
    }
}

/* The triangle's heading in the ground plane (12-bit) from its normal: the
   arctangent (func_802AD7FC) of the normal's x over its length across, by
   quadrant; turned half round (unk56 set) when the corner moved 100 along
   the normal lies on the other side of the plane (func_8029D90C) than the
   origin does */
REGS(s0)
void func_8029D56C(CollisionTri *t) {
    s32 x1 = CORNER(t, 0, 0), y1 = CORNER(t, 0, 1), z1 = CORNER(t, 0, 2);
    s32 x2 = CORNER(t, 1, 0), y2 = CORNER(t, 1, 1), z2 = CORNER(t, 1, 2);
    s32 x3 = CORNER(t, 2, 0), y3 = CORNER(t, 2, 1), z3 = CORNER(t, 2, 2), h, px, py, pz;
    s64 ey3 = y3 - y1, ez2 = z2 - z1, ez3 = z3 - z1, ey2 = y2 - y1, ex2 = x2 - x1, ex3 = x3 - x1;
    s64 nx, ny, nz, ax, az, num, len, dn, nb, nc, nd, side;
    f64 nlen;

    nx = (s64)((u64)ey3 * (u64)ez2 - (u64)ez3 * (u64)ey2);
    ny = (s64)((u64)ez3 * (u64)ex2 - (u64)ex3 * (u64)ez2);
    nz = (s64)((u64)ex3 * (u64)ey2 - (u64)ey3 * (u64)ex2);
    ax = nx;
    if (nx < 0) {
        ax = -nx;
    }
    az = nz;
    if (nz < 0) {
        az = -nz;
    }
    num = (s64)((u64)ax << 16);
    len = engine_cvt_l_d(__builtin_sqrt((f64)(s64)((u64)az * (u64)az + (u64)ax * (u64)ax)));
    if (len == 0) {
        engine_break(0x8029D70C, 7);
    }
    if (len == -1) {
        if ((u64)num == (u64)1 << 63) {
            engine_break(0x8029D728, 6);
        }
    }
    h = func_802AD7FC((u32)(num / len));
    h = (u32)h >> 4;
    if (nx < 0) {
        if (nz < 0) {
            h += HALF_TURN;
        } else {
            h = TURN - 1 - h;
        }
    } else {
        if (nz < 0) {
            h = HALF_TURN - h;
        }
    }
    nlen = __builtin_sqrt((f64)(s64)((u64)nx * (u64)nx + (u64)ny * (u64)ny + (u64)nz * (u64)nz));
    px = x1 + (s32)engine_cvt_l_d((f64)nx / nlen * 100.0);
    py = y1 + (s32)engine_cvt_l_d((f64)ny / nlen * 100.0);
    pz = z1 + (s32)engine_cvt_l_d((f64)nz / nlen * 100.0);
    dn = func_8029D90C(t, &nb, &nc, &nd);
    t->unk56 = 0;
    side = (s64)((u64)(s64)px * (u64)dn + (u64)(s64)py * (u64)nb + (u64)(s64)pz * (u64)nc + (u64)nd);
    if (side > 0) {
        if (nd > 0)
            goto done;
    } else {
        if (nd < 0)
            goto done;
    }
    /* the point and the origin on the same side: it faces away */
    h -= HALF_TURN;
    t->unk56 = 1;
    if (h < 0) {
        h += TURN - 1;
    }
done:
    t->heading = h;
}

/* ---- an object's spheres and triangles placed --------------------------------- */

REGS(v0, v1, a0, a1, a2, s4, s0, s1, s2 -> v0, v1, a0, s1, s2)
s32 func_802AA890(s32 x, s32 y, s32 z, s32 n, s32 *offs, u8 *base, s32 s0, s32 s1, s32 s2, s32 *y_out, s32 *z_out,
                  s32 *s1_out, s32 *s2_out);

/* An object's spheres (from p to end) placed: its bounding sphere (its
   D_803A7300 record) at (x, y, z), and each of its D_803A6B30's at its
   point through its matrices at base (func_802AA890; s0-s2 its point
   when it has none) */
static void c454(s32 kind, u8 *p, u8 *end, s32 x, s32 y, s32 z, u8 *base, s32 s0, s32 s1, s32 s2) {
    Solid *b;
    KindPart *r;
    ObjSphere *s;
    s32 ry, rz;

    for (b = D_803A7300;; b++) {
        if (b->kind == kind)
            break;
    }
    s = OBJSPHERE_FIRST(p);
    b->x = x;
    b->y = y;
    b->z = z;
    b->end = 1;
    if ((u8 *)s != end) {
        for (r = D_803A6B30;; r++) {
            if (r->kind == kind)
                break;
        }
        for (;; s = OBJSPHERE_NEXT(s), r++) {
            if ((u8 *)s == end)
                break;
            r->x = func_802AA890(s->x, s->y, s->z, s->nmtx, s->mtx, base, s0, s1, s2, &ry, &rz, &s1, &s2);
            r->y = ry;
            r->z = rz;
            r->end = 1;
        }
    }
}

/* an object model's triangles (func_8029D24C's data): their count, the
   matrices' count and offsets, then the triangles, three s16 corners */
typedef struct ObjTris {
    /* 0x00 */ u16 count;
    /* 0x02 */ u16 nmtx;
    /* 0x04 */ s32 mtx[1];
} ObjTris;
#define OBJTRI(o, i) ((s16 *)((u8 *)(o) + 4 + (o)->nmtx * 4 + (i) * 0x14))

/* a 64-bit plane component into game memory, the high word first */
static void put_dword(s64 *p, s64 v) {
    ((u32 *)p)[0] = (u32)((u64)v >> 32);
    ((u32 *)p)[1] = (u32)v;
}

/* the triangle's corner i through the matrices at base (func_802AA890,
   / 4) */
#define PLACE_CORNER(i)                                                                                        \
    do {                                                                                                     \
        s32 x_, y_, z_;                                                                                      \
        x_ = func_802AA890(c[3 * (i)], c[3 * (i) + 1], c[3 * (i) + 2], data->nmtx, data->mtx, base, (u32)t, s1, \
                           s2, &y_, &z_, &s1, &s2);                                                          \
        t->v[i][0] = x_ >> 2, t->v[i][1] = y_ >> 2, t->v[i][2] = z_ >> 2;                                   \
    } while (0)

/* The triangles of owner id from its model (the next ids from 1; new
   ones past D_803BD300): each placed through the matrices at base, its
   plane (the normal, d, |n|^2 and |n|), the axis the normal is most
   along, its heading (func_8029D56C) and unk55 (func_8029D534) */
REGS(t6, t2, s4)
void func_8029D24C(ObjTris *data, s32 owner, u8 *base) {
    s32 count = data->count, id = 1, x1, y1, z1, x2, y2, z2, x3, y3, z3, s1, s2;
    s32 dy2, dz3, dz2, dy3, dx3, dx2;
    s16 *c;
    CollisionTri *t, *end;
    s64 nx, ny, nz, d, ax, ay, az;
    f32 len2;

    for (;; id++, count--) {
        if (count == 0)
            break;
        /* its triangle of this id, or a new one */
        end = D_803BD300;
        for (t = D_803B9890;; t++) {
            if (t == end) {
                end++;
                break;
            }
            if (t->owner != owner)
                continue;
            if (t->id == id)
                break;
        }
        D_803BD300 = end;
        t->owner = owner;
        t->id = id;
        c = OBJTRI(data, id - 1);
        /* (what $s1 and $s2 hold: func_802AA890's point when there are no
           matrices) */
        s1 = (u32)end;
        s2 = (u32)&D_803BD300;
        PLACE_CORNER(0);
        PLACE_CORNER(1);
        PLACE_CORNER(2);
        x1 = t->v[0][0], y1 = t->v[0][1], z1 = t->v[0][2];
        x2 = t->v[1][0], y2 = t->v[1][1], z2 = t->v[1][2];
        x3 = t->v[2][0], y3 = t->v[2][1], z3 = t->v[2][2];
        dz3 = z1 - z3, dy2 = y1 - y2, dy3 = y1 - y3, dz2 = z1 - z2, dx3 = x1 - x3, dx2 = x1 - x2;
        nx = (s64)((u64)(s64)dy2 * (u64)(s64)dz3 - (u64)(s64)dz2 * (u64)(s64)dy3);
        ny = (s64)((u64)(s64)dz2 * (u64)(s64)dx3 - (u64)(s64)dx2 * (u64)(s64)dz3);
        nz = (s64)((u64)(s64)dx2 * (u64)(s64)dy3 - (u64)(s64)dy2 * (u64)(s64)dx3);
        d = (s64)(0 - ((u64)nx * (u64)(s64)x2 + (u64)ny * (u64)(s64)y2 + (u64)nz * (u64)(s64)z2));
        put_dword(&t->nx, nx);
        put_dword(&t->ny, ny);
        put_dword(&t->nz, nz);
        put_dword(&t->d, d);
        len2 = (f32)nx * (f32)nx + (f32)ny * (f32)ny + (f32)nz * (f32)nz;
        t->nlen2 = len2;
        t->nlen = __builtin_sqrtf(len2);
        ax = nx;
        if (nx < 0) {
            ax = -nx;
        }
        ay = ny;
        if (ny < 0) {
            ay = -ny;
        }
        az = nz;
        if (nz < 0) {
            az = -nz;
        }
        if (az >= ax && (az >= ay)) {
            t->axis = 0;
        } else {
            if (ay >= ax && (ay >= az)) {
                t->axis = 1;
            } else {
                t->axis = 2;
            }
        }
        func_8029D56C(t);
        func_8029D534(owner, t);
    }
}

/* The owner's triangles placed (func_8029D24C) and switched on
   (func_8029D120); then each of its switches whose area the point (x, z)
   at height y is in (func_8029DA90), or whose (part, key) pairs its parts
   match (func_8029DB7C), switches its triangles off (func_8029D1D4) */
static void d040(ObjTris *data, s32 owner, u8 *base, s32 x, s32 z, s32 y, Anim *parts) {
    TriSwitch *q, *end;
    CollisionTri *t;
    u8 *b;
    s32 n, miss;

    func_8029D24C(data, owner, base);
    func_8029D120(owner);
    end = D_803BD304;
    for (q = D_803BC1D0;; q++) {
        if (q == end)
            break;
        if (q->owner != owner)
            continue;
        if (q->kind == 0) {
            miss = func_8029DA90(q->area, q->n, x, z, y);
        } else {
            miss = func_8029DB7C((u8 *)q->area, q->n, parts);
        }
        if (miss != 0)
            continue;
        for (b = q->level, n = q->nlevel;; b++) {
            if (n == 0)
                break;
            n--;
            func_8029D1D4(0, *b);
        }
        for (b = q->own, n = q->nown;; b++) {
            if (n == 0)
                break;
            n--;
            t = func_8029D1D4(q->owner, *b);
        }
    }
}

void func_8028FAC0(s32 x, s32 y, s32 z, s32 id);
void func_802920DC(s32 x, s32 y, s32 z, s32 id);

/* With the crane's hook (func_8029C6E4), its point to func_8028FAC0 and
   func_802920DC */
void func_8029C5EC(void) {
    s32 x, y, z;

    if (crane_hook(&x, &y, &z)) {
        func_8028FAC0(x, y, z, CRANE_HOOK_R);
        func_802920DC(x, y, z, CRANE_HOOK_R);
    }
}

/* ---- the animations' blends ------------------------------------------------- */

/* The animation's frame at its fraction on the spline (its four keys,
   D_803B7FC0-D_803B7FC3, of the part's record rec): the nine halves into
   out, the basis set up for its mode (func_8029F110, func_8029F060) */
REGS(t4, t5, t1)
void func_8029FFA0(Anim *a, AnimPart *rec, s16 *out) {
    Key *p0, *p1, *p2, *p3;
    s32 i;
    f32 t = a->t;

    func_8029F110(a, a->interp);
    func_8029F060(t, (u8)a->key, ANIM_KEYS_N(a->data));
    SPLINE_KEYS(rec, p0, p1, p2, p3);
    for (i = 0;; i++) {
        if (i == 9)
            break;
        *out++ = spline_half(p0, p1, p2, p3, i, t);
    }
}

/* The key k the fraction t of the way to the next, into out: the
   positions and scales straight, the angles the short way round
   (func_8029F6B0) */
REGS(t5, t1, f30)
void func_802A0118(Key *k, s16 *out, f32 t) {
    Key *n = k + 1;
    s32 i;

    for (i = 0; i < 3; i++)
        out[i] = k->scale[i] + engine_cvt_w_s((f32)(n->scale[i] - k->scale[i]) * t);
    out[3] = func_8029F6B0(k->rot[0], n->rot[0], t);
    out[4] = func_8029F6B0(k->rot[1], n->rot[1], t);
    out[5] = func_8029F6B0(k->rot[2], n->rot[2], t);
    for (i = 0; i < 3; i++)
        out[6 + i] = k->pos[i] + engine_cvt_w_s((f32)(n->pos[i] - k->pos[i]) * t);
}

extern u8 D_803A7428, D_80370C3C;
extern u8 D_802C2984[];                 /* the hit effect's description */

#define HIT_FX_UNIT 7000                /* the effect's length, per func_8029A800's fx_len */

/* The hit effect (func_802A6274) at the point D_803A73FC-D_803A7404, and
   D_80370C3C set */
void func_8029B994(void) {
    func_802A6274((s32)(u32)D_802C2984, D_803A7428 * HIT_FX_UNIT, 0, (u32)D_803A73FC << 13, (u32)D_803A7400 << 13,
                  (u32)D_803A7404 << 13, 0, 0, 0, 0, 0, 0, 0, 1, 0);
    D_80370C3C = 1;
}

extern s32 D_803A73F0, D_803A73F4, D_803A73F8;  /* the vehicle's point */
extern s32 D_80358060;                  /* a frame count; 0 before the first */
extern s8 *PTR32 D_803A7408;            /* the kinds whose hit turns the camera, to a negative byte */
extern u8 D_803F7811, D_803F7801, D_803A7427, D_803A7429, D_803A7424, D_803A7425;
extern s16 D_803A7422, D_803F77FC;

/* A vehicle's collision state for the frame (func_8029A800): the camera's
   limits unset, its point and settings, the gears into D_803A742D; the
   building lists reset (func_802BCC10 before the first frame, or with a2
   for a vehicle other than the carrier, then func_802BCBD8). */
static void a800(s32 x, s32 y, s32 z, u8 *kinds, s32 a2, s32 hit_fx, s32 fx_len, s32 speed, s32 fx_speed, s32 t3,
                 s32 type, VS *vs) {
    s32 first;

    D_803A7410 = 0;
    D_803A7412 = TURN - 1;
    D_803A73F0 = x;
    D_803A73F4 = y;
    D_803A73F8 = z;
    D_803F7811 = a2;
    D_803A742C = 0;
    D_803A742E = 0;
    D_803A742D = vs->unkA0;
    D_803A742F = 0;
    D_803F7801 = t3;
    D_803A7427 = hit_fx;
    D_803A7428 = fx_len;
    D_803A7429 = 0;
    D_803A7422 = fx_speed;
    D_803F77FC = speed;
    D_803A7408 = (s8 *)kinds;
    D_803A7424 = 0;
    D_803A7425 = 0;
    first = D_80358060 == 0;
    if (first) {
        func_802BCC10();
    }
    if (a2 != 0) {
        if (type != VEHICLE_CMO) {
            func_802BCC10();
        }
    }
    func_802BCBD8();
}

extern u8 D_803B67C0[], D_803B37C0[], D_803B4FC0[];  /* a blend's data, built and its copies */
#define BLEND_SIZE 0x1800

/* the frame of an animation (spline: func_8029FFA0; keys: func_802A0118)
   for its part's record rec, into out; returns out past it */
static u8 *anim_frame(Anim *a, AnimPart *rec, u8 *out) {
    if (a->interp == ANIM_KEYS)
        func_802A0118((Key *)rec->keys + a->key, (s16 *)out, a->t);
    else
        func_8029FFA0(a, rec, (s16 *)out);
    return out + sizeof(Key);
}

/* anim_frame() with the mode checked: one that is neither keys nor a
   spline stops the game, as the original's syscall at `bad` does */
#define ANIM_FRAME(a, rec, out, bad)                                                                           \
    do {                                                                                                     \
        if ((a)->interp != ANIM_KEYS && (a)->interp != ANIM_SPLINE)                                          \
            engine_syscall(0x##bad);                                                                         \
        out = anim_frame(a, rec, out);                                                                       \
    } while (0)

/* a blend's header: two keys, at speeds 1 */
static void blend_start(void) {
    D_803B67C0[0] = 2;
    D_803B67C0[1] = 1;
    D_803B67C0[2] = 1;
}

/* the blend in D_803B67C0 (n parts) copied to `to`, and part p set to run
   it from its start (not running: the caller starts it) */
#define BLEND_FINISH(p, n, to)                                                                                 \
    do {                                                                                                     \
        u32 *s_ = (u32 *)D_803B67C0, *d_ = (u32 *)(to);                                                      \
        s32 j_;                                                                                              \
        D_803B67C0[3] = (n);                                                                                 \
        (p)->running = 0;                                                                                    \
        (p)->back = 0;                                                                                       \
        (p)->mode = 0;                                                                                       \
        (p)->interp = 0;                                                                                     \
        (p)->key = 0;                                                                                        \
        (p)->data = (to);                                                                                    \
        (p)->t = 0.0f;                                                                                       \
        for (j_ = BLEND_SIZE;; j_ -= 8) {                                                                    \
            if (j_ == 0)                                                                                     \
                break;                                                                                       \
            d_[0] = s_[0];                                                                                   \
            d_[1] = s_[1];                                                                                   \
            d_ += 2, s_ += 2;                                                                                \
        }                                                                                                    \
    } while (0)

/* A blend from parts[a]'s animation to parts[b]'s: a two-key animation
   whose parts are those both have, each from its frame in a to its frame
   in b, built at D_803B67C0 and kept at D_803B37C0 for parts[31] */
REGS(v0, v1, a0)
void func_8029F9D4(s32 a, s32 b, Part *parts_) {
    Anim *parts = ANIM(parts_), *pa = &parts[a], *pb = &parts[b];
    u8 *da = pa->data, *db = pb->data, *out = D_803B67C0 + 4;
    AnimPart *ra, *rb, *rb0;
    s32 na = ANIM_NPARTS(da), nb = ANIM_NPARTS(db), sa = ANIM_STRIDE(da), sb = ANIM_STRIDE(db), n = 0, j;

    blend_start();
    ra = anim_parts(da);
    rb0 = anim_parts(db);
    for (;; na--, ra = (AnimPart *)((u8 *)ra + sa)) {
        if (na == 0)
            break;
        /* the same part in b's */
        for (rb = rb0, j = nb;; j--, rb = (AnimPart *)((u8 *)rb + sb)) {
            if (j == 0)
                break;
            if (ra->mtx == rb->mtx)
                break;
        }
        if (j != 0) {
            ((s32 *)out)[0] = ra->mtx;
            ((s32 *)out)[1] = ra->rest;
            out += 8;
            n++;
            ANIM_FRAME(pa, ra, out, 8029FB30);
            ANIM_FRAME(pb, rb, out, 8029FB7C);
        }
    }
    BLEND_FINISH(&parts[31], n, D_803B37C0);
}

/* the parts of one animation, each its current frame held (both keys the
   same, func_8029FF2C), appended at out */
static u8 *held_frames(Anim *p, u8 *out, s32 *n) {
    u8 *d = p->data;
    AnimPart *r;
    s32 cnt = ANIM_NPARTS(d), stride = ANIM_STRIDE(d);

    for (r = anim_parts(d);; cnt--, r = (AnimPart *)((u8 *)r + stride)) {
        if (cnt == 0)
            break;
        ((s32 *)out)[0] = r->mtx;
        ((s32 *)out)[1] = r->rest;
        out += 8;
        (*n)++;
        ANIM_FRAME(p, r, out, 8029FD74);
        out = (u8 *)func_8029FF2C((s16 *)out);
    }
    return out;
}

/* parts[a]'s animation then parts[b]'s, each frame held as it is now: a
   two-key animation built at D_803B67C0 and kept at D_803B4FC0 for
   parts[30] */
REGS(v0, v1, a0)
void func_8029FC74(s32 a, s32 b, Part *parts_) {
    Anim *parts = ANIM(parts_);
    u8 *out = D_803B67C0 + 4;
    s32 n = 0;

    blend_start();
    out = held_frames(&parts[a], out, &n);
    out = held_frames(&parts[b], out, &n);
    BLEND_FINISH(&parts[30], n, D_803B4FC0);
}

/* ---- a vehicle's hits ----------------------------------------------------------- */

extern u8 D_803BE738;                   /* a hit's sound */
extern u8 D_803ED825;
extern s32 D_802E8BDC;                  /* the level */

#define QUARTER_TURN 0x400

/* the side of the triangle's plane the vehicle's point (D_803A73F0..F8,
   >> 2) is on, as the plane's d: whether it is the origin's other side */
static s32 vehicle_in_front(const CollisionTri *t) {
    s64 side = (s64)((u64)t->nx * (u64)(s64)(D_803A73F0 >> 2) + (u64)t->ny * (u64)(s64)(D_803A73F4 >> 2) +
                     (u64)t->nz * (u64)(s64)(D_803A73F8 >> 2) + (u64)t->d);

    return t->d > 0 ? side > 0 : side < 0;
}

/* A vehicle hitting triangle t of this kind: the kind into the frame's
   list (func_802BCCD4); unless the frame's kinds (D_803A7408) list it,
   D_803A7424 set and the camera turned to take in the triangle's heading
   +-a quarter turn, the way round given by the side the vehicle is on,
   when the triangle pushes both ways (unk55) or faces it (unk56) (its
   sound unk59 into D_803BE738).  Then, once a frame (D_803A7429), when the
   vehicle's speed reaches the frame's fx_speed with hit_fx, the hit
   effect (func_8029B994). */
REGS(s0, fp)
void func_8029B614(CollisionTri *t, s32 kind) {
    s8 *list;
    s32 h, lo, hi, v, w, front, turn, listed = 0;

    func_802BCCD4(kind);
    for (list = D_803A7408;; list++) {
        v = *list;
        if (v == kind) {
            listed = 1;
            break;
        }
        if (v < 0)
            break;
    }
    if (!listed) {
        D_803A7424 = 1;
        h = t->heading;
        front = vehicle_in_front(t);
        if (front) {
            lo = h - QUARTER_TURN, hi = h + QUARTER_TURN;
        } else {
            lo = h + QUARTER_TURN, hi = h - QUARTER_TURN;
        }
        turn = t->unk55 == 1;
        if (!turn) {
            /* one-way: only from the side it faces */
            turn = front == (t->unk56 != 0);
        }
        if (turn) {
            func_8029B7CC(lo, hi);
            if (t->unk59 != 0) {
                D_803BE738 = t->unk59;
            }
        }
    }
    /* the hit effect */
    if (D_803A7429 == 0) {
        v = D_803F77FC;
        w = D_803A7422;
        if (v < 0) {
            v = -v;
        }
        if (w < 0) {
            w = -w;
        }
        if (v >= w) {
            if (D_803A7427 != 0) {
                D_803A7429 = D_803A7427;
                func_8029B994();
            }
        }
    }
}

#define WALL_ANY_KIND_LEVEL 9           /* the level whose walls have sides for every kind */
#define VEHICLE_TYPES 0x13              /* the vehicles' types (the others' walls have no sides) */

/* Wall triangle t hit by vehicle kind: through its wall (D_803BD310's
   that lists it), a hit (func_8029B614, 1) when the wall stops that kind;
   otherwise, for a wall with sides (vehicles' kinds, or any on level 9),
   the side of its plane the vehicle is on (1 in front, 2 behind) kept in
   t->side, and when it changes, D_803ED825 toggled and every triangle's
   side of the wall swapped (0) */
REGS(s0, t8, fp -> a1)
s32 func_8029BB28(CollisionTri *t, s32 kind, s32 fp) {
    Wall *w = D_803BD310;
    CollisionTri *PTR32 *pc;
    u8 *k;
    u32 n;
    s32 s, cur, front;

    for (;; w++) {
        for (n = WALL_COUNT(w), pc = w->tris;; pc++) {
            if (n == 0)
                break;
            n--;
            if (*pc == t)
                goto found;
        }
    }
found:
    for (n = w->nkinds, k = w->kinds;; k++) {
        if (n == 0)
            break;
        n--;
        if (*k == kind) {
            func_8029B614(t, fp);
            return 1;
        }
    }
    if (D_802E8BDC != WALL_ANY_KIND_LEVEL) {
        if (kind >= VEHICLE_TYPES) {
            return 0;
        }
    }
    if (WALL_SIDED(w) != 0) {
        front = vehicle_in_front(t);
        if (front) {
            s = 1;
        } else {
            s = 2;
        }
        cur = t->side;
        if (cur == 0) {
            t->side = s;
        } else {
            if (cur != s) {
                /* through the wall */
                D_803ED825 ^= 1;
                for (n = WALL_COUNT(w), pc = w->tris;;) {
                    CollisionTri *q;

                    if (n == 0)
                        break;
                    n--;
                    q = *pc++;
                    if (q->side == 0)
                        continue;
                    if (q->side != 1) {
                        q->side = 1;
                    } else {
                        q->side = 2;
                    }
                }
            }
        }
    }
    return 0;
}

/* ---- the vehicles' spheres against each other's ------------------------------ */

/* the communication points' spheres (8A2E0's CommPoint: x, y, z, r first) */
extern s32 D_803FBBB0[][5];
extern u8 D_803FC1F0;                   /* how many */
/* the objects' spheres (8A080's CollisionObject: a key, -1 free, then x, y,
   z, r) */
extern s32 D_803FB8B8[][5];
#define N_OBJ_SPHERES 25
#define KEY_FREE (-1)

/* The sphere (x, y, z, r) against kind's spheres (D_803A6B30's run of
   them): each one it meets through func_802CE204 and into the frame's
   list by its number in the run (func_802BCCD4, 1 on) */
REGS(t8, a2, a3, t0, t1)
void func_8029C914(s32 kind, s32 x, s32 y, s32 z, s32 r) {
    KindPart *p;
    s32 n = 0;

    for (p = D_803A6B30;; p++) {
        if (p->end == -1) {
            return;
        }
        if (p->kind == kind)
            break;
    }
    for (;; p++) {
        if (p->end == -1)
            break;
        if (p->kind != kind)
            break;
        n++;
        if (func_8029CFA4(p->x, p->y, p->z, p->r, x, y, z, r)) {
            func_802CE204(p->x, p->z, x, z);
            func_802BCCD4(n);
        }
    }
}

/* Kind's bounding sphere, when placed, against the communication points'
   spheres: for each it meets, kind's spheres against that one
   (func_8029C914) */
REGS(t8)
void func_8029C748(s32 kind) {
    Solid *s;
    s32 *w, n;

    for (s = D_803A7300;; s++) {
        if (s->kind == kind)
            break;
    }
    if (s->end == 1) {
        for (n = D_803FC1F0, w = D_803FBBB0[0];; w += 5) {
            if (n == 0)
                break;
            n--;
            if (func_8029CFA4(s->x, s->y, s->z, s->r, w[0], w[1], w[2], w[3])) {
                func_8029C914(kind, w[0], w[1], w[2], w[3]);
            }
        }
    }
}

/* the same against the objects' 25 spheres (the free ones skipped) */
REGS(t8)
void func_8029C828(s32 kind) {
    Solid *s;
    s32 *w, n;

    for (s = D_803A7300;; s++) {
        if (s->kind == kind)
            break;
    }
    if (s->end == 1) {
        for (n = N_OBJ_SPHERES, w = D_803FB8B8[0];; w += 5) {
            if (n == 0)
                break;
            n--;
            if (w[0] == KEY_FREE)
                continue;
            if (func_8029CFA4(s->x, s->y, s->z, s->r, w[1], w[2], w[3], w[4])) {
                func_8029C914(kind, w[1], w[2], w[3], w[4]);
            }
        }
    }
}

/* a radius over the vehicle's gears (unkA0, when it isn't 1), unless the
   other is the carrier: the two copies, which share their end */
REGS(t4, t1, gp -> t1)
s32 func_8029CB04(s32 kind, u32 v, VS *vs) {
    s32 d;

    if (kind == VEHICLE_CMO) {
        return v;
    }
    d = vs->unkA0;
    if (d != 1) {
        if (d == 0) {
            engine_break(0x8029CB3C, 7);
        }
        v /= (u32)d;
    }
    return v;
}

REGS(s1, t1, gp -> t1)
s32 func_8029CF04(s32 kind, u32 v, VS *vs) {
    s32 d;

    if (kind != VEHICLE_CMO) {
        d = vs->unkA0;
        if (d != 1) {
            if (d == 0) {
                engine_break(0x8029CF3C, 7);
            }
            v /= (u32)d;
        }
    }
    return v;
}

extern u8 D_803A7426;

/* the 12-bit angle of a direction in its quadrant: func_802AD7FC of n over
   the distance, scaled to 16 bits, / 16 */
static s32 quad_angle(s32 n, f32 dist) {
    f32 q = (f32)n / dist;

    return (u32)func_802AD7FC(engine_cvt_w_s(65536.0f * q)) >> 4;
}

/* Vehicle type's sphere number n (at (x, z)) hitting the other kind's
   sphere at (ox, oz): the number into the frame's list (func_802BCCD4);
   D_803A7426 set when the carrier and a vehicle other than the driver
   meet; the camera turned to take in the direction from the vehicle to
   the sphere +-a quarter turn (func_8029B7CC), and D_803A742C set.
   (The original takes the other's kind from the byte before the end of
   its record, through the record after it.) */
REGS(fp, t8, s0, v0, a0, a2, t0)
void func_8029CB54(s32 n, s32 type, KindPart *other, s32 x, s32 z, s32 ox, s32 oz) {
    s32 dx, dz, h = 0, set;
    f32 d;

    func_802BCCD4(n);
    if (type == VEHICLE_CMO) {
        set = other->kind != VEHICLE_DRIVER;
    } else {
        set = 0;
        if (type != VEHICLE_DRIVER) {
            set = other->kind == VEHICLE_CMO;
        }
    }
    if (set) {
        D_803A7426 = 1;
    }
    dx = ox - x;
    dz = oz - z;
    if (dx != 0 || dz != 0) {
        d = __builtin_sqrtf((f32)dx * (f32)dx + (f32)dz * (f32)dz);
        if (!(ox < x)) {
            if (!(oz < z)) {
                h = quad_angle(ox - x, d);
            } else {
                h = quad_angle(z - oz, d);
                h += QUARTER_TURN;
            }
        } else {
            if (oz < z) {
                h = quad_angle(x - ox, d);
                h += 2 * QUARTER_TURN;
            } else {
                h = quad_angle(oz - z, d);
                h += 3 * QUARTER_TURN;
            }
        }
    }
    func_8029B7CC(h + QUARTER_TURN, h - QUARTER_TURN);
    D_803A742C = 1;
}

/* Kind a's spheres (D_803A6B30's run) against kind b's: each pair that
   meets (b's radius over the gears, func_8029CF04; func_8029CFA4), but
   not the crane's sphere 0x30E nor its hook, counts the train's hits
   (D_803A7430) and, unless func_802AB41C(b, a) says otherwise, is a hit
   (func_8029CB54, func_8029CF54) */
REGS(t8, t4, gp)
void func_8029CD54(s32 a, s32 b, VS *vs) {
    KindPart *ra, *rb0, *rb, *pa, *pb;
    s32 n = 0, r, hit;

    for (ra = D_803A6B30;; ra++) {
        if (ra->end == -1)
            return;
        if (ra->kind == a)
            break;
    }
    for (rb0 = D_803A6B30;; rb0++) {
        if (rb0->end == -1)
            return;
        if (rb0->kind == b)
            break;
    }
    for (;;) {
        if (ra->end == -1)
            break;
        if (ra->kind != a)
            break;
        n++;
        pa = ra++;
        for (rb = rb0;;) {
            if (rb->end == -1)
                break;
            if (rb->kind != b)
                break;
            pb = rb++;
            r = func_8029CF04(b, pb->r, vs);
            if (a == VEHICLE_CRANE) {
                if (pa->r == CRANE_R_30E)
                    continue;
            }
            if (b == VEHICLE_CRANE) {
                if (r == CRANE_HOOK_R)
                    continue;
            }
            hit = func_8029CFA4(pa->x, pa->y, pa->z, pa->r, pb->x, pb->y, pb->z, r);
            if (hit == 0)
                continue;
            if (b == VEHICLE_TRAIN) {
                if (D_803A7430 < TRAIN_HITS_MAX) {
                    ++D_803A7430;
                }
            }
            if (a != VEHICLE_DRIVER) {
                hit = func_802AB41C(b, a) == 0;
                if (!hit)
                    continue;
            }
            func_8029CB54(n, a, pb, pa->x, pa->z, pb->x, pb->z);
            func_8029CF54(b);
        }
    }
}

extern s32 D_803649E8;

/* Kind's bounding sphere, when placed, against every other kind's (their
   radius over the gears, func_8029CB04): for the crane not where
   func_802AB41C says, and not the driver with D_803649E8, the two kinds'
   spheres against each other (func_8029CD54) where they meet */
REGS(t8, gp)
void func_8029C9D4(s32 kind, VS *vs) {
    Solid *s, *q;
    s32 k, rr, skip;

    for (s = D_803A7300;; s++) {
        if (s->kind == kind)
            break;
    }
    if (s->end != 1)
        return;
    for (q = D_803A7300;; q++) {
        if (q->end == -1)
            break;
        k = q->kind;
        if (k == kind)
            continue;
        if (kind == VEHICLE_CRANE) {
            skip = func_802AB41C(k, kind) != 0;
            if (skip)
                continue;
        }
        if (D_803649E8 != 0) {
            if (k == VEHICLE_DRIVER)
                continue;
        }
        rr = func_8029CB04(k, q->r, vs);
        if (func_8029CFA4(s->x, s->y, s->z, s->r, q->x, q->y, q->z, rr)) {
            func_8029CD54(kind, k, vs);
        }
    }
}

/* A vehicle kind's spheres against the other vehicles', the objects' and
   the communication points' (func_8029C9D4, func_8029C828, func_8029C748);
   D_803A7424 and D_803A7425 set when the camera's limits were moved */
REGS(t8, gp)
void func_8029C52C(s32 kind, VS *vs) {
    func_8029C9D4(kind, vs);
    func_8029C828(kind);
    func_8029C748(kind);
    if (D_803A7410 == 0) {
        if (D_803A7412 == TURN - 1) {
            return;
        }
    }
    D_803A7425 = 1;
    D_803A7424 = 1;
}

/* ---- the animations each frame ------------------------------------------------ */

#define N_PARTS 32

/* An object's 32 parts' animations one frame on (func_8029E5AC for each
   running one), with base the parts' matrices and D_803B3770 the other
   frame's. */
REGS(t0, v0, v1)
void func_8029E558(Part *parts, u8 *base, u8 *other) {
    Anim *a = ANIM(parts);
    s32 n;

    D_803B3770 = other;
    for (n = N_PARTS; n != 0; n--, a++) {
        if (a->running != 0) {
            func_8029E5AC(a, base);
        }
    }
}

extern Vehicle D_80364460[];
#define TEX_REFS_STALE 2                /* a slot counted this often wasn't refreshed */

/* A texture animation one frame on: the slots it uses counted, its state
   stepped (func_8029F1BC), each of the key's textures found (func_8029E4E4)
   or loaded (func_8029E47C) and put into its vehicle's display list where
   D_803B3500 says (and, blending, the next key's, with the fraction as
   G_SETPRIMCOLOR's LOD), and the slots no key uses any more freed */
REGS(t0, fp)
void func_8029E21C(Anim *a, s32 fp) {
    TexAnim *T = (TexAnim *)a->data;
    TexSlot *s;
    TexPatch *e, *end;
    Vehicle *m;
    u8 *dl;
    u16 *h1, *h2;
    u32 tex1, tex2 = 0, *w, cmd;
    s32 k, n, c, second, idx;
    f32 t = a->t;

    for (s = D_803A7440, n = TEX_SLOTS;; n--, s++) {
        if (n == 0)
            break;
        if (s->owner == T) {
            if (s->used != 0) {
                s->refs++;
            }
        }
    }
    k = a->key;
    k = func_8029F1BC(a, k, T->n, ANIM_RATE(T->rate[k], T->rate[k + 1], t), t, &t);
    c = T->c;
    second = T->second;
    h1 = TEXANIM_KINDS(T, k);
    h2 = h1 + c;
    for (idx = 0;; idx++) {
        if (c == 0)
            break;
        c--;
        tex1 = func_8029E4E4(*h1, T);
        h1++;
        if (tex1 == 0) {
            tex1 = func_8029E47C(T, h1[-1], fp);
        }
        if (second != 0) {
            tex2 = func_8029E4E4(*h2, T);
            h2++;
            if (tex2 == 0) {
                tex2 = func_8029E47C(T, h2[-1], fp);
            }
        }
        for (m = D_80364460;; m++) {
            if (m->type == T->type)
                break;
        }
        if (D_8035805C != 0) {
            dl = m->unkC;
        } else {
            dl = m->unk30;
        }
        end = D_803B35F0;
        for (e = D_803B3500;; e++) {
            if (e == end)
                break;
            if (e->anim == T) {
                if (e->idx == idx) {
                    *(u32 *)(dl + e->off) = tex1;
                    if (second != 0) {
                        /* the next record: the second texture's G_SETTIMG,
                           then the G_SETPRIMCOLOR after it gets the blend */
                        e++;
                        w = (u32 *)(dl + e->off);
                        *w++ = tex2;
                        for (;;) {
                            cmd = *w;
                            w += 2;
                            if (GFX_OP(cmd) == G_SETPRIMCOLOR_OP)
                                break;
                        }
                        w[-2] = engine_cvt_w_s(t * 255.0f) | ((u32)G_SETPRIMCOLOR_OP << 24);
                    }
                }
            }
        }
    }
    for (s = D_803A7440, n = TEX_SLOTS;; n--, s++) {
        if (n == 0)
            break;
        if (s->used != 0) {
            if (s->refs >= TEX_REFS_STALE) {
                if (s->owner == T) {
                    s->used = 0;
                }
            }
        }
    }
}

/* The texture animations (D_803B35F8's running records) one frame on */
REGS(fp)
void func_8029E0AC(s32 fp) {
    Part *p;

    for (p = D_803B35F8;; p++) {
        if (*(s32 *)&p->unk0 == -1)
            break;
        if (p->unk10 != 0) {
            func_8029E21C(ANIM(p), fp);
        }
    }
}

/* ---- a vehicle's spheres against the buildings and the level ------------------- */

REGS(-> t1)
s32 func_802BD8C8(void);
extern BuildingRule D_803059F0[];
extern CollisionTri D_803F9330[];       /* the holes' triangles (8A080) */
extern CollisionTri *PTR32 D_803FB8B0;  /* one past the last */
extern u32 D_803BDE40[], D_803BDCA8[];  /* the grid's cells: their triangles from one word to the next */
extern u8 D_803A742A;                   /* the hole group a vehicle of type 0xC8 stands on */
#define CELL_TRIS(tab, i) (*(CollisionTri *PTR32 *)&(tab)[i])
#define VEHICLE_ON_HOLE 0xC8            /* (the type that skips the hole group it stands on) */
#define NO_GRID_TYPE VEHICLE_JETPACK    /* the J-Bomb skips the first grid */

/* The sphere (x, y, z, r) against one triangle: across its plane
   (func_8029C160), and then inside it there (collision_flatten,
   func_8029BF64), across one of its edges (func_8029BD0C) or holding its
   first corner (func_8029BEE4) */
static s32 piece_hit(CollisionTri *t, s32 x, s32 y, s32 z, s32 r) {
    s32 px, py, pz, hit;
    FlatTri f;

    hit = func_8029C160(x, y, z, r, t, &px, &py, &pz);
    if (!hit)
        return 0;
    collision_flatten(t, px, py, pz, &f);
    hit = collision_flat_inside(&f);
    if (hit)
        return 1;
    hit = func_8029BD0C(x, y, z, r, t);
    if (hit)
        return 1;
    hit = func_8029BEE4(t, x, y, z, r);
    return hit;
}

/* whether the rule applies to this vehicle type on this level (the tests
   func_8029B02C and func_8029AB88 each have) */
static s32 rule_applies(const BuildingRule *e, s32 type) {
    if ((e->types & (1u << (type & 31))) == 0)
        return 0;
    if (e->level != ALL_LEVELS) {
        if (e->level != D_802E8BDC)
            return 0;
    }
    if (e->kind == KIND_RDU) {
        s32 v;

        v = func_802BD8C8();
        if (v != 0) {
            return 0;
        }
    }
    return 1;
}

/* The vehicle (type, the sphere (x, y, z, r), its number n) against the
   buildings: the pieces of the level's buildings of the kinds its rules
   give, whose bounding sphere it meets (func_8029B514); the holes' (not
   the one a vehicle 0xC8 stands on); the walls' (func_8029BB28 on a
   hit); the objects' not of its own type (func_8029B5B8 after a hit); and
   the triangles in its grid cell (func_8029C284; the first grid not for
   the J-Bomb).  A hit is func_8029B614.  (a1-a3 are nothing to it: what
   its callers had in those registers.) */
REGS(a1, a2, a3, t3, t4, t5, t6, t8, fp)
void func_8029B02C(s32 a1, s32 a2, s32 a3, s32 x, s32 y, s32 z, s32 r, s32 type, s32 n) {
    BuildingRule *e;
    Building *o, *oend;
    CollisionTri *t, *tend;
    s16 *cell;

    /* (the inputs the vehicle modules' readers find in their registers
       afterwards: $t8 and $fp) */
    /* the buildings */
    for (e = D_803059F0;; e++) {
        if (e->types == 0)
            break;
        if (!rule_applies(e, type))
            continue;
        oend = D_803F7654;
        for (o = D_803F4030;; o++) {
            if (o == oend)
                break;
            if (o->unk30 != e->kind)
                continue;
            if (!func_8029B514(o, x, y, z, r)) {
                continue;
            }
            tend = o->unk8;
            for (t = o->unk4;; t++) {
                if (t == tend)
                    break;
                if (t->active == 0)
                    continue;
                if (piece_hit(t, x, y, z, r)) {
                    func_8029B614(t, n);
                }
            }
        }
    }
    /* the holes */
    tend = D_803FB8B0;
    for (t = D_803F9330;; t++) {
        if (t == tend)
            break;
        if (t->active == 0)
            continue;
        if (type == VEHICLE_ON_HOLE) {
            if (t->group == D_803A742A)
                continue;
        }
        if (piece_hit(t, x, y, z, r)) {
            func_8029B614(t, n);
        }
    }
    /* the walls */
    tend = D_803BD30C;
    for (t = D_803BD308;; t++) {
        if (t == tend)
            break;
        if (piece_hit(t, x, y, z, r)) {
            func_8029BB28(t, type, n);
        }
    }
    /* the objects */
    tend = D_803BD300;
    for (t = D_803B9890;; t++) {
        if (t == tend)
            break;
        if (t->active == 0)
            continue;
        if (t->owner != 0) {
            if (t->owner == type)
                continue;
        }
        if (piece_hit(t, x, y, z, r)) {
            func_8029B614(t, n);
            func_8029B5B8(t);
        }
    }
    /* the grid cells */
    func_8029C284(x, z);
    if (type != NO_GRID_TYPE) {
        for (cell = D_803A7418;; cell++) {
            if (*cell == -1)
                break;
            tend = CELL_TRIS(D_803BDE40, *cell + 1);
            for (t = CELL_TRIS(D_803BDE40, *cell);; t++) {
                if (t == tend)
                    break;
                if (piece_hit(t, x, y, z, r)) {
                    func_8029B614(t, n);
                }
            }
        }
    }
    for (cell = D_803A7418;; cell++) {
        if (*cell == -1)
            break;
        tend = CELL_TRIS(D_803BDCA8, *cell + 1);
        for (t = CELL_TRIS(D_803BDCA8, *cell);; t++) {
            if (t == tend)
                break;
            if (piece_hit(t, x, y, z, r)) {
                func_8029B614(t, n);
            }
        }
    }
}

/* Whether the sphere (x, y, z, r) meets any triangle func_8029B02C would
   test, for this vehicle type: the same walk, stopping at a hit */
static s32 ab88(s32 x, s32 y, s32 z, s32 r, s32 type) {
    BuildingRule *e;
    Building *o, *oend;
    CollisionTri *t, *tend;
    s16 *cell;

    for (e = D_803059F0;; e++) {
        if (e->types == 0)
            break;
        if (!rule_applies(e, type))
            continue;
        oend = D_803F7654;
        for (o = D_803F4030;; o++) {
            if (o == oend)
                break;
            if (o->unk30 != e->kind)
                continue;
            if (!func_8029B514(o, x, y, z, r)) {
                continue;
            }
            tend = o->unk8;
            for (t = o->unk4;; t++) {
                if (t == tend)
                    break;
                if (t->active == 0)
                    continue;
                if (piece_hit(t, x, y, z, r)) {
                    return 1;
                }
            }
        }
    }
    tend = D_803FB8B0;
    for (t = D_803F9330;; t++) {
        if (t == tend)
            break;
        if (t->active == 0)
            continue;
        if (type == VEHICLE_ON_HOLE) {
            if (t->group == D_803A742A)
                continue;
        }
        if (piece_hit(t, x, y, z, r))
            return 1;
    }
    tend = D_803BD30C;
    for (t = D_803BD308;; t++) {
        if (t == tend)
            break;
        if (piece_hit(t, x, y, z, r))
            return 1;
    }
    tend = D_803BD300;
    for (t = D_803B9890;; t++) {
        if (t == tend)
            break;
        if (t->active == 0)
            continue;
        if (t->owner != 0) {
            if (t->owner == type)
                continue;
        }
        if (piece_hit(t, x, y, z, r))
            return 1;
    }
    func_8029C284(x, z);
    if (type != NO_GRID_TYPE) {
        for (cell = D_803A7418;; cell++) {
            if (*cell == -1)
                break;
            tend = CELL_TRIS(D_803BDE40, *cell + 1);
            for (t = CELL_TRIS(D_803BDE40, *cell);; t++) {
                if (t == tend)
                    break;
                if (piece_hit(t, x, y, z, r))
                    return 1;
            }
        }
    }
    for (cell = D_803A7418;; cell++) {
        if (*cell == -1)
            break;
        tend = CELL_TRIS(D_803BDCA8, *cell + 1);
        for (t = CELL_TRIS(D_803BDCA8, *cell);; t++) {
            if (t == tend)
                break;
            if (piece_hit(t, x, y, z, r))
                return 1;
        }
    }
    return 0;
}

REGS(t3, t4, t5, t6, t8 -> a1)
s32 func_8029AB88(s32 x, s32 y, s32 z, s32 r, s32 type) {
    s32 hit;

    hit = ab88(x, y, z, r, type);
    return hit;
}

extern s32 D_803A740C, D_80358068;

/* The vehicle of this type against the buildings: its bounding sphere
   first (func_8029AB88, when placed), and on a hit each of its spheres
   (func_8029B02C, numbered from 1).  D_803A742B is cleared, and
   D_803A740C set to the frame when the camera's limits were moved. */
static void aa10(s32 type) {
    Solid *s;
    KindPart *p;
    s32 n = 0;

    D_803A742B = 0;
    for (s = D_803A7300;; s++) {
        if (s->kind == type)
            break;
    }
    if (s->end != 0) {
        if (func_8029AB88(s->x >> 2, s->y >> 2, s->z >> 2, s->r >> 2, type)) {
            for (p = D_803A6B30;; p++) {
                if (p->end == -1)
                    break;
                if (p->end == 0)
                    continue;
                if (p->kind != type)
                    continue;
                n++;
                func_8029B02C(0, 0, 0, p->x >> 2, p->y >> 2, p->z >> 2, p->r >> 2, type, n);
            }
        }
    }
    if (D_803A7410 == 0) {
        if (D_803A7412 == TURN - 1) {
            return;
        }
    }
    D_803A7425 = 1;
    D_803A740C = D_80358068;
}

/* ---- the entry points as the vehicle modules declare them (shared.h) ----- */

/* (They leave some of their inputs, as the original has them, for the
   readers after them.)  func_8029C454's point when it has no matrices
   (func_802AA890's $s0-$s2: whatever the caller had there) is 0: no
   model has such a point. */
REGS(v0, v1, a0, t0, t1, t2, s4)
void func_8029C454(s32 x, s32 y, s32 z, s32 type, u8 *a, u8 *b, u8 *buf) {
    c454(type, a, b, x, y, z, buf, 0, 0, 0);
}

REGS(t0, t1, t2, t6, a1, a2, s4)
void func_8029D040(s32 x, s32 z, s32 type, u8 *t6, s32 heading, Part *parts, u8 *buf) {
    d040((ObjTris *)t6, type, buf, x, z, heading, ANIM(parts));
}

/* the vehicle type func_8029A800 was last given: func_8029AA10's, which
   the original has in $t8 */
static s32 a800_type;

REGS(v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t8, gp)
void func_8029A800(s32 x, s32 y, s32 z, u8 *kinds, s32 a2, s32 hit_fx, s32 fx_len, s32 speed, s32 fx_speed,
                   s32 t3, s32 type, VS *vs) {
    a800_type = type;
    a800(x, y, z, kinds, a2, hit_fx, fx_len, speed, fx_speed, t3, type, vs);
}

REGS()
void func_8029AA10(void) {
    aa10(a800_type);
}
