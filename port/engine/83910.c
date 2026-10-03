/*
 * hd_code 83910 (us.v11 0x802C80D0-0x802C9BB0): the barges (VEHICLE_BARGE,
 * BARGE_2 and BARGE_3: types 0xB, 0x11 and 0x12), as native C (engine.h).
 * Three copies of the same code over three sets of variables: parts
 * D_803F7C50[3][32], states D_803F8550[3], positions D_803F8748 + 12k,
 * model files D_803F876C + 4k, buffer pairs D_803F8778 + 8k (vehicle.h).
 * func_802C80D0 sets one up (from the level loader), func_802C8BB8 runs one
 * each frame (from hd.c and at the end of its setup); the others are hd.c's
 * hooks.  A barge has no wheels on the ground: no gears from the stick, and
 * a bump turns it back.
 *
 * The three copies are one function each here (barge_setup, barge_frame,
 * barge_draw), over the barge's number and its copy's blocks.
 */
#include "buildings.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/audio.h"

/* the barges' numbers (62740's helpers'; a rate is a frame's) */
#define BARGE_BRAKE 6           /* func_802A785C: the speed's fall a frame, braking */
#define BARGE_TURN_RATE 0x2328  /* func_802A7FD8: the heading's turning rate */
#define BARGE_SLOPE_DIV 160.0f  /* func_802A843C: the slope's push divided by */
#define BARGE_BOUNCE_MIN 0x50   /* a bump's speed at least (then halved, turned round) */
#define BARGE_STEER_DIV 10.6f   /* the steering's rate: the speed over this (func_802A7E70) */

/* the barges' .bss (asm/data/hd_code/83910.bss.s) */
extern s32 D_803F8748[9];                       /* x, y, z of each */
extern u8 *PTR32 D_803F876C[3];                 /* their model files */
extern u8 *PTR32 D_803F8778[6];                 /* their buffer pairs (0x800 bytes each) */
extern u8 D_803F8790[3];                        /* bumped: turning back */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306410[];
extern s32 D_803643E4, D_803643E8;

void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_802C8BB8(u8 type);
REGS(gp)
void func_802C95D8(VS *vs);
void func_802C9B30(void);

#define T(p) ((s32)(p))

/* ---- the three barges ---- */

/* Each barge's blocks (the original has three copies of the code, each
   with its own), named by the first copy's (type 0xB) addresses: BB(addr)
   charges barge n's block in that place. */
enum {
    B_802C8150, B_802C81B8, B_802C81CC, B_802C826C, B_802C82A8, B_802C82BC,
    B_802C82D0, B_802C82E4, B_802C82FC, B_802C8310, B_802C8324, B_802C8334,
    B_802C8348, B_802C835C, B_802C83FC, B_802C8414, B_802C8428, B_802C8460,
    B_802C8C90, B_802C8CC8, B_802C8CD0, B_802C8CD8, B_802C8CE4, B_802C8CE8,
    B_802C8CF0, B_802C8D08, B_802C8D24, B_802C8D30, B_802C8D44, B_802C8D5C,
    B_802C8D90, B_802C8DB8, B_802C8DCC, B_802C8DD4, B_802C8DE8, B_802C8DF0,
    B_802C8E34, B_802C8E3C, B_802C8E44, B_802C8E60, B_802C8F10, B_802C8E70,
    B_802C8E80, B_802C8E94, B_802C8EB0, B_802C8EC8, B_802C8EF0, B_802C8EF8,
    B_802C8F08, B_802C8F18, B_802C8F94, B_802C9624, B_802C9654, B_802C9668,
    B_802C9678, B_802C96BC, B_802C96D0, B_802C96E0, B_802C96EC, B_802C9734,
    B_802C9754, B_802C9770, B_802C97B0,
    NBARGE_BLKS
};

static const Blk barge_blks[3][NBARGE_BLKS] = {
    {
        {ENGINE_BLK_802C8150}, {ENGINE_BLK_802C81B8}, {ENGINE_BLK_802C81CC}, {ENGINE_BLK_802C826C},
        {ENGINE_BLK_802C82A8}, {ENGINE_BLK_802C82BC}, {ENGINE_BLK_802C82D0}, {ENGINE_BLK_802C82E4},
        {ENGINE_BLK_802C82FC}, {ENGINE_BLK_802C8310}, {ENGINE_BLK_802C8324}, {ENGINE_BLK_802C8334},
        {ENGINE_BLK_802C8348}, {ENGINE_BLK_802C835C}, {ENGINE_BLK_802C83FC}, {ENGINE_BLK_802C8414},
        {ENGINE_BLK_802C8428}, {ENGINE_BLK_802C8460}, {ENGINE_BLK_802C8C90}, {ENGINE_BLK_802C8CC8},
        {ENGINE_BLK_802C8CD0}, {ENGINE_BLK_802C8CD8}, {ENGINE_BLK_802C8CE4}, {ENGINE_BLK_802C8CE8},
        {ENGINE_BLK_802C8CF0}, {ENGINE_BLK_802C8D08}, {ENGINE_BLK_802C8D24}, {ENGINE_BLK_802C8D30},
        {ENGINE_BLK_802C8D44}, {ENGINE_BLK_802C8D5C}, {ENGINE_BLK_802C8D90}, {ENGINE_BLK_802C8DB8},
        {ENGINE_BLK_802C8DCC}, {ENGINE_BLK_802C8DD4}, {ENGINE_BLK_802C8DE8}, {ENGINE_BLK_802C8DF0},
        {ENGINE_BLK_802C8E34}, {ENGINE_BLK_802C8E3C}, {ENGINE_BLK_802C8E44}, {ENGINE_BLK_802C8E60},
        {ENGINE_BLK_802C8F10}, {ENGINE_BLK_802C8E70}, {ENGINE_BLK_802C8E80}, {ENGINE_BLK_802C8E94},
        {ENGINE_BLK_802C8EB0}, {ENGINE_BLK_802C8EC8}, {ENGINE_BLK_802C8EF0}, {ENGINE_BLK_802C8EF8},
        {ENGINE_BLK_802C8F08}, {ENGINE_BLK_802C8F18}, {ENGINE_BLK_802C8F94}, {ENGINE_BLK_802C9624},
        {ENGINE_BLK_802C9654}, {ENGINE_BLK_802C9668}, {ENGINE_BLK_802C9678}, {ENGINE_BLK_802C96BC},
        {ENGINE_BLK_802C96D0}, {ENGINE_BLK_802C96E0}, {ENGINE_BLK_802C96EC}, {ENGINE_BLK_802C9734},
        {ENGINE_BLK_802C9754}, {ENGINE_BLK_802C9770}, {ENGINE_BLK_802C97B0},
    },
    {
        {ENGINE_BLK_802C8470}, {ENGINE_BLK_802C84D8}, {ENGINE_BLK_802C84EC}, {ENGINE_BLK_802C858C},
        {ENGINE_BLK_802C85C8}, {ENGINE_BLK_802C85DC}, {ENGINE_BLK_802C85F0}, {ENGINE_BLK_802C8604},
        {ENGINE_BLK_802C861C}, {ENGINE_BLK_802C8630}, {ENGINE_BLK_802C8644}, {ENGINE_BLK_802C8654},
        {ENGINE_BLK_802C8668}, {ENGINE_BLK_802C867C}, {ENGINE_BLK_802C871C}, {ENGINE_BLK_802C8734},
        {ENGINE_BLK_802C8748}, {ENGINE_BLK_802C8780}, {ENGINE_BLK_802C8FA8}, {ENGINE_BLK_802C8FE0},
        {ENGINE_BLK_802C8FE8}, {ENGINE_BLK_802C8FF0}, {ENGINE_BLK_802C8FFC}, {ENGINE_BLK_802C9000},
        {ENGINE_BLK_802C9008}, {ENGINE_BLK_802C9020}, {ENGINE_BLK_802C903C}, {ENGINE_BLK_802C9048},
        {ENGINE_BLK_802C905C}, {ENGINE_BLK_802C9074}, {ENGINE_BLK_802C90A8}, {ENGINE_BLK_802C90D0},
        {ENGINE_BLK_802C90E4}, {ENGINE_BLK_802C90EC}, {ENGINE_BLK_802C9100}, {ENGINE_BLK_802C9108},
        {ENGINE_BLK_802C914C}, {ENGINE_BLK_802C9154}, {ENGINE_BLK_802C915C}, {ENGINE_BLK_802C9178},
        {ENGINE_BLK_802C9228}, {ENGINE_BLK_802C9188}, {ENGINE_BLK_802C9198}, {ENGINE_BLK_802C91AC},
        {ENGINE_BLK_802C91C8}, {ENGINE_BLK_802C91E0}, {ENGINE_BLK_802C9208}, {ENGINE_BLK_802C9210},
        {ENGINE_BLK_802C9220}, {ENGINE_BLK_802C9230}, {ENGINE_BLK_802C92AC}, {ENGINE_BLK_802C97C0},
        {ENGINE_BLK_802C97F0}, {ENGINE_BLK_802C9804}, {ENGINE_BLK_802C9814}, {ENGINE_BLK_802C9858},
        {ENGINE_BLK_802C986C}, {ENGINE_BLK_802C987C}, {ENGINE_BLK_802C9888}, {ENGINE_BLK_802C98D0},
        {ENGINE_BLK_802C98F0}, {ENGINE_BLK_802C990C}, {ENGINE_BLK_802C994C},
    },
    {
        {ENGINE_BLK_802C8790}, {ENGINE_BLK_802C87F8}, {ENGINE_BLK_802C880C}, {ENGINE_BLK_802C88AC},
        {ENGINE_BLK_802C88E8}, {ENGINE_BLK_802C88FC}, {ENGINE_BLK_802C8910}, {ENGINE_BLK_802C8924},
        {ENGINE_BLK_802C893C}, {ENGINE_BLK_802C8950}, {ENGINE_BLK_802C8964}, {ENGINE_BLK_802C8974},
        {ENGINE_BLK_802C8988}, {ENGINE_BLK_802C899C}, {ENGINE_BLK_802C8A3C}, {ENGINE_BLK_802C8A54},
        {ENGINE_BLK_802C8A68}, {ENGINE_BLK_802C8AA0}, {ENGINE_BLK_802C92C0}, {ENGINE_BLK_802C92F8},
        {ENGINE_BLK_802C9300}, {ENGINE_BLK_802C9308}, {ENGINE_BLK_802C9314}, {ENGINE_BLK_802C9318},
        {ENGINE_BLK_802C9320}, {ENGINE_BLK_802C9338}, {ENGINE_BLK_802C9354}, {ENGINE_BLK_802C9360},
        {ENGINE_BLK_802C9374}, {ENGINE_BLK_802C938C}, {ENGINE_BLK_802C93C0}, {ENGINE_BLK_802C93E8},
        {ENGINE_BLK_802C93FC}, {ENGINE_BLK_802C9404}, {ENGINE_BLK_802C9418}, {ENGINE_BLK_802C9420},
        {ENGINE_BLK_802C9464}, {ENGINE_BLK_802C946C}, {ENGINE_BLK_802C9474}, {ENGINE_BLK_802C9490},
        {ENGINE_BLK_802C9540}, {ENGINE_BLK_802C94A0}, {ENGINE_BLK_802C94B0}, {ENGINE_BLK_802C94C4},
        {ENGINE_BLK_802C94E0}, {ENGINE_BLK_802C94F8}, {ENGINE_BLK_802C9520}, {ENGINE_BLK_802C9528},
        {ENGINE_BLK_802C9538}, {ENGINE_BLK_802C9548}, {ENGINE_BLK_802C95C4}, {ENGINE_BLK_802C995C},
        {ENGINE_BLK_802C998C}, {ENGINE_BLK_802C99A0}, {ENGINE_BLK_802C99B0}, {ENGINE_BLK_802C99F4},
        {ENGINE_BLK_802C9A08}, {ENGINE_BLK_802C9A18}, {ENGINE_BLK_802C9A24}, {ENGINE_BLK_802C9A6C},
        {ENGINE_BLK_802C9A8C}, {ENGINE_BLK_802C9AA8}, {ENGINE_BLK_802C9AE8},
    },
};

#define BB(addr) BLKT(k[B_##addr])

static const u8 barge_type[3] = { 0xB, 0x11, 0x12 };

static void barge_draw(s32 n, const Blk *k, s32 type, VS *vs);

/* barge n set up: the model file, its position and heading */
static void barge_setup(s32 n, u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    const Blk *k = barge_blks[n];
    s32 type = barge_type[n], *pos = &D_803F8748[3 * n];
    s32 avg;
    VS *vs = &D_803F8550[n];
    Part *parts = D_803F7C50 + 32 * n;
    u8 *buf;
    s16 *r;

    BB(802C8150);
    D_803F876C[n] = model;
    buf = D_80358070;
    D_803F8778[2 * n] = buf;
    D_803F8778[2 * n + 1] = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(type, 0, D_803F8778[2 * n], D_803F8778[2 * n + 1], model);
    BB(802C81B8);
    D_803F8790[n] = 0;
    func_802A754C(vs);
    BB(802C81CC);
    vs->unk52[0] = 0x50;
    vs->unk52[1] = 0x50;
    vs->unk52[2] = -0x50;
    vs->unk52[3] = 0x50;
    vs->unk52[4] = 0x50;
    vs->unk52[5] = -0x50;
    vs->unk5E[0] = 0x5A;
    vs->unk5E[1] = 0x5A;
    vs->unk5E[2] = -0x5A;
    vs->unk5E[3] = 0x5A;
    vs->unk5E[4] = 0x5A;
    vs->unk5E[5] = -0x5A;
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, pos[1], x, z, vs->unk4, &pos[1], (s16 *)&vs->unk4C, type, vs, 0, &avg);
    BB(802C826C);
    func_8029F85C(parts, D_803F876C[n], D_803F8778[2 * n], D_803F8778[2 * n + 1]);
    BB(802C82A8);
    func_802A039C(0, 100, parts);
    BB(802C82BC);
    func_802A03D4(0, 0, parts);
    BB(802C82D0);
    func_802A040C(0, 0, parts);
    BB(802C82E4);
    func_802A0480(0, 0, parts, 0.0f);
    BB(802C82FC);
    func_802A0290(0, 1, parts);
    BB(802C8310);
    func_8029E558(parts, D_803F8778[2 * n], D_803F8778[2 * n + 1]);
    BB(802C8324);
    func_802A0320(0, parts);
    BB(802C8334);
    func_802A0290(0, 1, parts);
    BB(802C8348);
    func_8029E558(parts, D_803F8778[2 * n + 1], D_803F8778[2 * n]);
    BB(802C835C);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 1;
    r[3] = 0, r[4] = 0x50, r[5] = 1;
    r[6] = 0x50, r[7] = 0x8C, r[8] = 1;
    r[9] = 0x8C, r[10] = 0xBE, r[11] = 1;
    r[12] = 0xBE, r[13] = 0xFA, r[14] = 1;
    func_8029C354(type, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x88B8);
    BB(802C83FC);
    func_80258230(type, 0x96, 0x2D, 0x2D);
    BB(802C8414);
    vs->unk9A = 1;
    func_802C8BB8(type);
    BB(802C8428);
    vs->unk9A = 0;
    model = D_803F876C[n];
    func_802AA838(D_803F8778[2 * n + 1], D_803F8778[2 * n], *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    BB(802C8460);
}

/* barge n each frame */
static void barge_frame(s32 n) {
    const Blk *k = barge_blks[n];
    s32 type = barge_type[n], *pos = &D_803F8748[3 * n];
    VS *vs = &D_803F8550[n];
    Part *parts = D_803F7C50 + 32 * n;
    s32 t3 = 0, x, z, s;
    f32 rate;
    u8 *a2, *a3;

    BB(802C8C90);
    func_802A75DC((u8 *)parts, &pos[0], &pos[1], &pos[2], (u8 *)vs);
    BB(802C8CC8);
    func_802C4724(0x71);
    BB(802C8CD0);
    func_802C9B30();
    BB(802C8CD8);
    s = vs->unk76;
    if (s < 0) {
        BB(802C8CE4);
        s = -s;
    }
    BB(802C8CE8);
    func_802C4584((u32)s >> 5);
    BB(802C8CF0);
    func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, BARGE_BRAKE, vs, &t3);
    BB(802C8D08);
    func_802A7FD8(BARGE_TURN_RATE, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 0, vs);
    BB(802C8D24);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    BB(802C8D30);
    func_802A843C(&vs->unk76, 1, type, (s8 *)vs->unk96, vs->unk4, BARGE_SLOPE_DIV, vs);
    BB(802C8D44);
    x = func_802A860C(vs->unk4E, &vs->unk76, &pos[0], &pos[2], rate, &z);
    BB(802C8D5C);
    D_803ED40B = 0;
    func_802A8768(x, z, &pos[0], &pos[2], &pos[1], type, 0xA0, 0xA0, vs->unk52, vs->unk28, vs->unk28 + 6, vs->unk28 + 3,
                  vs->unk5E, vs);
    BB(802C8D90);
    if (D_8035805C != 0) {
        BB(802C8DB8);
        func_8029E558(parts, D_803F8778[2 * n], D_803F8778[2 * n + 1]);
        BB(802C8DCC);
    } else {
        BB(802C8DD4);
        func_8029E558(parts, D_803F8778[2 * n + 1], D_803F8778[2 * n]);
    }
    BB(802C8DE8);
    barge_draw(n, k, type, vs);
    BB(802C8DF0);
    func_8029A800(pos[0], pos[1], pos[2], D_80306410, 0, 0, 0, vs->unk76, 0, 0, type, vs);
    BB(802C8E34);
    func_8029C52C(type, vs);
    BB(802C8E3C);
    func_8029AA10();
    BB(802C8E44);
    D_803F77D0 = parts;
    func_802BE77C(type, vs);
    BB(802C8E60);
    if (D_803A7424 == 0) {
        BB(802C8F10);
        D_803F8790[n] = 0;
        goto done;
    }
    BB(802C8E70);
    if (D_803F8790[n] != 0)
        goto done;
    /* bumped: back to where it was, turned round */
    BB(802C8E80);
    if (D_8035805C != 0) {
        BB(802C8E94);
        a2 = D_803F8778[2 * n + 1];
        a3 = D_803F8778[2 * n];
    } else {
        BB(802C8EB0);
        a2 = D_803F8778[2 * n];
        a3 = D_803F8778[2 * n + 1];
    }
    BB(802C8EC8);
    func_802A768C((u8 *)parts, &pos[0], &pos[1], &pos[2], (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    BB(802C8EF0);
    func_802C95D8(vs);
    BB(802C8EF8);
    D_803F8790[n] = 1;
    barge_draw(n, k, type, vs);
    BB(802C8F08);
    goto out;
done:
    BB(802C8F18);
    D_803643E0 = pos[0];
    D_803643E4 = pos[1];
    D_803643E8 = pos[2];
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, type, vs);
out:
    BB(802C8F94);
}

/* barge n's matrix, its vertices, its collision and its shadow */
static void barge_draw(s32 n, const Blk *k, s32 type, VS *vs) {
    s32 *pos = &D_803F8748[3 * n];
    u8 *model = D_803F876C[n], *buf;
    s32 *m, off;

    BB(802C9624);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        BB(802C9654);
        m = (s32 *)(D_803F8778[2 * n] + off);
    } else {
        BB(802C9668);
        m = (s32 *)(D_803F8778[2 * n + 1] + off);
    }
    BB(802C9678);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(pos[0], pos[1], pos[2], 0x88B8, m);
    BB(802C96BC);
    if (D_8035805C != 0) {
        BB(802C96D0);
        buf = D_803F8778[2 * n];
    } else {
        BB(802C96E0);
        buf = D_803F8778[2 * n + 1];
    }
    BB(802C96EC);
    model = D_803F876C[n];
    func_8029C454(pos[0], pos[1], pos[2], type, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), buf);
    BB(802C9734);
    func_802ABBEC(type, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    BB(802C9754);
    func_802AABE4(type, (u16 *)(model + *(s32 *)(model + 8)), buf, 0, 0);
    BB(802C9770);
    func_8029D040(pos[0], pos[2], type, model + *(s32 *)(model + 0xC), vs->unk4C, D_803F7C50 + 32 * n, buf);
    BB(802C97B0);
}

/* set up: the model file in $s2, the position in $t7, $s3, $s0, the
   heading in $s1 (barges 0, 1, 2) */
REGS(s2, t7, s3, s0, s1)
void func_802C8150(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    barge_setup(0, model, x, y, z, heading);
}

REGS(s2, t7, s3, s0, s1)
void func_802C8470(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    barge_setup(1, model, x, y, z, heading);
}

REGS(s2, t7, s3, s0, s1)
void func_802C8790(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    barge_setup(2, model, x, y, z, heading);
}

/* each frame (each copy runs its own barge, whatever `type` says) */
REGS(a0)
void func_802C8C90(u8 type) {
    barge_frame(0);
}

REGS(a0)
void func_802C8FA8(u8 type) {
    barge_frame(1);
}

REGS(a0)
void func_802C92C0(u8 type) {
    barge_frame(2);
}

/* the matrix and the rest */
REGS(gp)
void func_802C9624(VS *vs) {
    barge_draw(0, barge_blks[0], 0xB, vs);
}

REGS(gp)
void func_802C97C0(VS *vs) {
    barge_draw(1, barge_blks[1], 0x11, vs);
}

REGS(gp)
void func_802C995C(VS *vs) {
    barge_draw(2, barge_blks[2], 0x12, vs);
}

/* ---- shared ---- */

/* set up a barge: from the level loader, with its type in $t3 */
REGS(t3, s2, t7, s3, s0, s1)
void func_802C80D0(s32 type, u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    ENGINE_BLK(802C80D0);
    if (type == 0xB) {
        ENGINE_BLK(802C80F8);
        func_802C8150(model, x, y, z, heading);
        ENGINE_BLK(802C8100);
    } else {
        ENGINE_BLK(802C8108);
        if (type == 0x11) {
            ENGINE_BLK(802C8114);
            func_802C8470(model, x, y, z, heading);
            ENGINE_BLK(802C811C);
        } else {
            ENGINE_BLK(802C8124);
            func_802C8790(model, x, y, z, heading);
        }
    }
    ENGINE_BLK(802C812C);
}

/* hd.c's: the player gets in */
void func_802C8AB0(void) {
    ENGINE_BLK(802C8AB0);
    D_8036444C = 3000;
    D_80364450 = 0;
    func_802C4310(0x72);
    ENGINE_BLK(802C8AE0);
}

/* hd.c's: whether it can be left (always) */
u8 func_802C8AF0(void) {
    ENGINE_BLK(802C8AF0);
    return 1;
}

/* hd.c's: the player gets out of the barge of this type */
void func_802C8B0C(u8 type) {
    ENGINE_BLK(802C8B0C);
    if (type == 0x11) {
        ENGINE_BLK(802C8B54);
        func_802A7764((u32 *)D_803F8778[2], (u32 *)D_803F8778[3], 0x800);
        ENGINE_BLK(802C8B74);
    } else {
        ENGINE_BLK(802C8B20);
        if (type == 0x12) {
            ENGINE_BLK(802C8B7C);
            func_802A7764((u32 *)D_803F8778[4], (u32 *)D_803F8778[5], 0x800);
        } else {
            ENGINE_BLK(802C8B2C);
            func_802A7764((u32 *)D_803F8778[0], (u32 *)D_803F8778[1], 0x800);
            ENGINE_BLK(802C8B4C);
        }
    }
    ENGINE_BLK(802C8B9C);
    func_802C444C();
    ENGINE_BLK(802C8BA4);
}

/* each frame: the barge of this type */
void func_802C8BB8(u8 type) {
    ENGINE_BLK(802C8BB8);
    if (type == 0xB) {
        ENGINE_BLK(802C8C0C);
        func_802C8C90(type);
        ENGINE_BLK(802C8C14);
    } else {
        ENGINE_BLK(802C8C1C);
        if (type == 0x11) {
            ENGINE_BLK(802C8C28);
            func_802C8FA8(type);
            ENGINE_BLK(802C8C30);
        } else {
            ENGINE_BLK(802C8C38);
            func_802C92C0(type);
        }
    }
    ENGINE_BLK(802C8C40);
}

/* bumped: the speed at least BARGE_BOUNCE_MIN either way, then turned
   round and halved */
REGS(gp)
void func_802C95D8(VS *vs) {
    s32 v = vs->unk76;

    ENGINE_BLK(802C95D8);
    if (v < 0) {
        ENGINE_BLK(802C95FC);
        if (v > -BARGE_BOUNCE_MIN) {
            ENGINE_BLK(802C9608);
            v = -BARGE_BOUNCE_MIN;
        }
    } else {
        ENGINE_BLK(802C95EC);
        if (v < BARGE_BOUNCE_MIN) {
            ENGINE_BLK(802C95F4);
            v = BARGE_BOUNCE_MIN;
        }
    }
    ENGINE_BLK(802C960C);
    vs->unk76 = -v >> 1;
}

/* the turn rate: the speed / BARGE_STEER_DIV */
REGS(gp -> s3)
s32 func_802C9AF8(VS *vs) {
    ENGINE_BLK(802C9AF8);
    return engine_cvt_w_s((f32)vs->unk76 / BARGE_STEER_DIV);
}

/* the camera's distance and speed for a barge */
void func_802C9B30(void) {
    ENGINE_BLK(802C9B30);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 4;
}
