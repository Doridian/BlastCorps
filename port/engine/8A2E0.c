/*
 * hd_code 8A2E0 (us.v11 0x802CEAA0-0x802CF698): the communication points
 * (the satellite dishes some levels have, LevelHeader.commPoint), as native
 * C (engine.h, buildings.h).  func_802CEAA0 sets them up from the level
 * file, func_802CEEFC draws them, func_802CF1A4 lights one the player
 * comes near; the other two are hd_code 409D0.c's hooks.
 *
 * They share one model (model 0x96, D_803FBBD8) and one set of parts, the
 * dish's (D_803FBBE0: a turning one, D_803FBEE0: a still one), each with
 * two 0x300-byte buffers, one per frame (D_803FC1E0...EC).
 */
#include "buildings.h"
#include "game/game.h"
#include "game/audio.h"

/* a communication point (0x14 bytes, D_803FBBB0[D_803FC1F0]) */
typedef struct CommPoint {
    /* 0x00 */ s32 x, y, z;      /* << 5 */
    /* 0x0C */ s32 r;
    /* 0x10 */ u16 lit;          /* the player has been near (the dish turns) */
    /* 0x12 */ u16 spin;         /* LevelCommPoint.spin */
} CommPoint;
SIZE_CHECK(CommPoint, 0x14);

extern CommPoint D_803FBBB0[];
extern u8 D_803FC1F0;                           /* how many */
extern u8 *D_803FBBD8;                          /* their model */
extern Part D_803FBBE0[32], D_803FBEE0[32];     /* the dish's parts, turning and still */
extern u8 *D_803FC1E0, *D_803FC1E4;             /* the turning parts' two buffers */
extern u8 *D_803FC1E8, *D_803FC1EC;             /* the still ones' */
extern s16 D_803FBAB0[];                        /* each one's box, 8 Vtx (0x80 bytes) */
extern s32 D_803643E0, D_803643E4, D_803643E8;  /* the player's position */
extern u8 D_803F3FF8[];                         /* a debris record being made (77E20) */

/* 5CB60's: model n, loaded */
REGS(t3 -> s2)
u32 func_802A396C(u32 type);
/* 60F60's */
REGS()
void func_802A5E60(void);
/* 56040's */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2);
REGS(t0, v0, v1)
void func_8029E558(Part *parts, u8 *buf, u8 *other);

REGS(t4, t5, t6)
void func_802CEE14(s32 x, s32 y, s32 z);
REGS(t1)
void func_802CF3E0(CommPoint *c);

#define COMM_RADIUS 0x280          /* CommPoint.r */
#define COMM_NEAR 0x4C              /* the player this near lights one (whole units) */
#define COMM_SPARKS 24              /* func_802CF3E0's, one a frame */
#define COMM_SPARK_SPEED 50000
#define COMM_SPARK_FASTER 0x36B0    /* each one's speed over the last's */
#define COMM_SPARK_RISE 0xA0000     /* and its height (16.16: 10 units) */

/* one set of the dish's parts, set up in the two buffers b1, b2 */
static void dish_parts(Part *parts, u8 *b1, u8 *b2, s32 still) {
    func_8029F85C(parts, D_803FBBD8, b1, b2);
    func_802A039C(0, 100, parts);
    func_802A03D4(0, 0, parts);
    func_802A040C(0, 0, parts);
    func_802A0480(0, 0, parts, 0.0f);
    func_802A0290(0, 1, parts);
    func_8029E558(parts, b1, b2);
    func_802A0320(0, parts);
    func_802A0290(0, 1, parts);
    func_8029E558(parts, b2, b1);
}

/* the level's communication points (LevelCommPoint records, from the level
   header in $t0: +0x28 to +0x2C), their model and the dish's parts */
REGS(t0)
void func_802CEAA0(u8 *level) {
    LevelCommPoint *p, *end;
    CommPoint *c;
    u8 *heap;

    D_803FC1F0 = 0;
    p = (LevelCommPoint *)(level + *(s32 *)(level + 0x28));
    end = (LevelCommPoint *)(level + *(s32 *)(level + 0x2C));
    if (p != end) {
        D_803FBBD8 = (u8 *)(__UINTPTR_TYPE__)func_802A396C(0x96);
        for (c = D_803FBBB0; p != end; c++, p++) {
            c->x = p->x << 5;
            c->y = p->y << 5;
            c->z = p->z << 5;
            func_802CEE14(p->x, p->y, p->z);
            c->r = COMM_RADIUS;
            c->lit = 0;
            c->spin = p->spin;
            D_803FC1F0++;
        }
    }
    if (D_803FC1F0 == 0) {
        return;
    }
    heap = D_80358070;
    D_803FC1E0 = heap;
    D_803FC1E4 = heap + 0x300;
    D_803FC1E8 = heap + 0x600;
    D_803FC1EC = heap + 0x900;
    D_80358070 = heap + 0xC00;
    dish_parts(D_803FBBE0, D_803FC1E0, D_803FC1E4, 0);
    func_802A039C(1, 2, D_803FBBE0);
    func_802A03D4(1, 0, D_803FBBE0);
    func_802A040C(1, 0, D_803FBBE0);
    func_802A0290(1, -1, D_803FBBE0);
    func_802A039C(2, 2, D_803FBBE0);
    func_802A03D4(2, 0, D_803FBBE0);
    func_802A040C(2, 0, D_803FBBE0);
    func_802A0290(2, -1, D_803FBBE0);
    dish_parts(D_803FBEE0, D_803FC1E8, D_803FC1EC, 1);
}

/* communication point D_803FC1F0's box: eight corners, +-10 in x, 0..20
   in y, -10..20 in z (whole units) */
REGS(t4, t5, t6)
void func_802CEE14(s32 x, s32 y, s32 z) {
    s16 *v = &D_803FBAB0[D_803FC1F0 * 0x40];
    s32 x0 = x - 10, x1 = x + 10, y1 = y + 20, z0 = z - 10, z1 = z + 20;

    v[0x00] = x0, v[0x01] = y, v[0x02] = z0;
    v[0x08] = x0, v[0x09] = y1, v[0x0A] = z0;
    v[0x10] = x1, v[0x11] = y, v[0x12] = z0;
    v[0x18] = x1, v[0x19] = y1, v[0x1A] = z0;
    v[0x20] = x0, v[0x21] = y, v[0x22] = z1;
    v[0x28] = x0, v[0x29] = y1, v[0x2A] = z1;
    v[0x30] = x1, v[0x31] = y, v[0x32] = z1;
    v[0x38] = x1, v[0x39] = y1, v[0x3A] = z1;
}

/* Draw them: the model's segment, then for each its parts' buffer (this
   frame's, `frame`), and a display list made at `dl` (its box, culled,
   and the model at its position, the matrix at `mtx`); last the turning
   parts stepped.  Returns the end of gfx. */
Gfx *func_802CEEFC(Gfx *gfx_, u8 frame, void *dl_, void *mtx_) {
    u32 *gfx = (u32 *)gfx_, *dl = dl_;
    s32 *mtx = mtx_;
    CommPoint *c;
    s32 n, i = 0;
    u8 *model, *buf;

    if (D_803FC1F0 == 0) {
        return (Gfx *)gfx;
    }
    model = D_803FBBD8;
    gfx[0] = DL_SEGMENT(6);             /* the model's section 0x14 */
    gfx[1] = K0(model + *(s32 *)(model + 0x14));
    gfx += 2;
    for (n = D_803FC1F0, c = D_803FBBB0; n != 0; n--, c++, i++) {
        if (c->lit != 0) {
            if (frame != 0) {
                buf = D_803FC1E4;
            } else {
                buf = D_803FC1E0;
            }
        } else {
            if (frame != 0) {
                buf = D_803FC1EC;
            } else {
                buf = D_803FC1E8;
            }
        }
        gfx[0] = DL_SEGMENT(7);         /* the parts */
        gfx[1] = K0(buf);
        gfx[2] = DL_CALL;               /* the display list below */
        gfx[3] = K0(dl);
        dl[0] = 0x04700080;             /* its box: 8 vertices */
        dl[1] = (u32)&D_803FBAB0[i * 0x40];
        dl[2] = 0xBE000000;             /* culled by them */
        dl[3] = 0x140;
        gfx += 4;
        dl += 4;
        func_802ACA60(c->x << 11, c->y << 11, c->z << 11, mtx);
        func_802AC8CC((u32 *)mtx);
        model = D_803FBBD8;
        dl[0] = DL_MTX_PUSH;            /* the matrix */
        dl[1] = K0(mtx);
        dl[2] = DL_CALL;                /* the model's two lists */
        dl[3] = K0(model + *(s32 *)(model + 0x24));
        dl[4] = DL_CALL;
        dl[5] = K0(model + *(s32 *)(model + 0x2C));
        dl[6] = DL_MTX_POP;
        dl[7] = 0;
        dl[8] = DL_END;
        dl[9] = 0;
        dl += 10;
        mtx += 0x10;
    }
    if (frame != 0) {
        func_8029E558(D_803FBBE0, D_803FC1E4, D_803FC1E0);
    } else {
        func_8029E558(D_803FBBE0, D_803FC1E0, D_803FC1E4);
    }
    return (Gfx *)gfx;
}

/* the player near (under 0x4C) an unlit one: it lights (the dish turns) */
void func_802CF1A4(void) {
    s32 px = D_803643E0 >> 5, py = D_803643E4 >> 5, pz = D_803643E8 >> 5;
    s32 n, dx, dy, dz, d;
    CommPoint *c;

    for (n = D_803FC1F0, c = D_803FBBB0; n != 0; c++, n--) {
        if (c->lit != 0)
            continue;
        dx = (c->x >> 5) - px;
        dy = (c->y >> 5) - py;
        dz = (c->z >> 5) - pz;
        d = (s32)((u32)dx * (u32)dx + (u32)dy * (u32)dy + (u32)dz * (u32)dz);
        d = engine_cvt_w_s(__builtin_sqrtf((f32)d));
        if (d >= COMM_NEAR)
            continue;
        c->lit = 1;
        func_802A0360(2, 0, D_803FBBE0, 0.0f);
        func_802A0360(1, 0, D_803FBBE0, 0.0f);
        func_80285B68(n);
        func_80285CA0();
        func_802CF3E0(c);
    }
}

/* communication point c lit: COMM_SPARKS sparks rising from it, one a
   frame (FX_DELAY), each COMM_SPARK_STEP higher and faster, and a sound */
REGS(t1)
void func_802CF3E0(CommPoint *c) {
    s32 x, y, z, k, speed;
    u8 *d = D_803F3FF8;

    func_802A5E60();
    func_802C049C();
    x = c->x << 11;
    y = c->y << 11;
    z = c->z << 11;
    speed = COMM_SPARK_SPEED;
    for (k = 0; k < COMM_SPARKS; k++) {
        FX_W(d, FX_X) = x;
        FX_W(d, FX_Y) = y;
        FX_W(d, FX_Z) = z;
        FX_W(d, FX_UNK28) = FX_UNK28_INIT;
        FX_W(d, FX_SPEED) = speed;
        FX_H(d, FX_DELAY) = k;
        y += COMM_SPARK_RISE;
        speed += COMM_SPARK_FASTER;
        FX_W(d, FX_VEL) = 0;
        FX_W(d, FX_VEL + 4) = 0;
        FX_W(d, FX_VEL + 8) = 0;
        FX_W(d, FX_UNK14) = 0;
        FX_W(d, FX_UNK14 + 4) = 0;
        FX_W(d, FX_UNK14 + 8) = 0;
        FX_B(d, FX_RADIUS) = 0;
        FX_B(d, FX_AMOUNT) = 0;
        FX_B(d, FX_KIND) = FX_KIND_SPARK;
        FX_B(d, FX_DONE) = 0;
        FX_B(d, FX_UNK35) = 0;
        func_802C04F0(d);
    }
    func_80260650(D_80367738, 0x5E, NULL);
}

/* 409D0.c's: each lit one's hook (func_80285AB0) */
void func_802CF5B0(void) {
    s32 n;
    CommPoint *c;

    for (n = D_803FC1F0, c = D_803FBBB0; n != 0; c++, n--) {
        if (c->lit != 0) {
            func_80285AB0(n);
        }
    }
}

/* 00000.c's: each one lit or not as func_80285B10 says */
void func_802CF628(void) {
    s32 n;
    CommPoint *c;

    for (n = D_803FC1F0, c = D_803FBBB0; n != 0; c++, n--) {
        c->lit = func_80285B10(n);
    }
}
