/*
 * hd_code 83910 (us.v11 0x802C80D0-0x802C9BB0): the barges (VEHICLE_BARGE,
 * BARGE_2 and BARGE_3: types 0xB, 0x11 and 0x12), as native C (engine.h,
 * vehicle.h).  The original has three copies of the same code over three
 * sets of variables: parts D_803F7C50[3][32], states D_803F8550[3],
 * positions D_803F8748 + 12k, model files D_803F876C + 4k, buffer pairs
 * D_803F8778 + 8k.  Here they are one function each (barge_setup,
 * barge_frame, barge_draw) over the barge's number; the nine original
 * functions charge their own copy's cost and call them.
 *
 * func_802C80D0 sets one up (from the level loader), func_802C8BB8 runs one
 * each frame (from hd.c and at the end of its setup); the others are hd.c's
 * hooks.  A barge has no wheels on the ground: no steering, and a bump
 * turns it back.
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the barges' .bss (asm/data/hd_code/83910.bss.s) */
extern s32 D_803F8748[9];                       /* x, y, z of each */
extern u8 *PTR32 D_803F876C[3];                 /* their model files */
extern u8 *PTR32 D_803F8778[6];                 /* their buffer pairs (0x800 bytes each) */
extern u8 D_803F8790[3];                        /* bumped (turned back once until clear) */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306410[];                         /* their parts' collision (56040's func_8029A800) */

void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_802C8BB8(u8 type);
REGS(gp)
void func_802C95D8(VS *vs);
void func_802C9B30(void);
REGS(gp)
void func_802C9624(VS *vs);
REGS(gp)
void func_802C97C0(VS *vs);
REGS(gp)
void func_802C995C(VS *vs);

/* ---- the barges' numbers (a frame, where it's per frame) ------------------ */

#define BARGE_SCALE 0x88B8              /* their model's scale */
#define BARGE_SPAN 0xA0                 /* their spans, both ways (func_802A8768) */
#define BARGE_BRAKE 6                   /* speed lost braking or against the stick, a frame */
#define BARGE_TURN_RATE 0x2328          /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define BARGE_SLOPE_DIV 160.0f          /* func_802A843C: the slope's push is the height difference over this */
#define BARGE_STEER_DIV 10.6f           /* the steering rate: the speed over this (func_802C9AF8) */
#define BARGE_HIT_MIN_SPEED 0x50        /* a bump's speed at least, either way (then halved, turned round) */
#define BARGE_GRAVITY 2.0f              /* times the level's */
#define BARGE_BOUNCE_MIN 0x3C           /* a landing harder than this bounces ... */
#define BARGE_BOUNCE_DIV 4              /* ... at the speed over this */

static const u8 barge_type[3] = { VEHICLE_BARGE, VEHICLE_BARGE_2, VEHICLE_BARGE_3 };
/* each barge's drawing, its copy's function (each charges its own) */
static void (*const barge_draw_fn[3])(VS *vs) = { func_802C9624, func_802C97C0, func_802C995C };

#define PARTS(n) (D_803F7C50 + 32 * (n))
#define BUF0(n) D_803F8778[2 * (n)]
#define BUF1(n) D_803F8778[2 * (n) + 1]

/* barge n set up: the model file, its position and heading */
static void barge_setup(s32 n, u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F8550[n];
    Part *parts = PARTS(n);
    s32 type = barge_type[n], *pos = &D_803F8748[3 * n], avg;

    D_803F876C[n] = model;
    BUF0(n) = D_80358070;
    BUF1(n) = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(type, 0, BUF0(n), BUF1(n), model);
    D_803F8790[n] = 0;
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x50, 0x50, -0x50, 0x50, 0x50, -0x50);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x5A, 0x5A, -0x5A, 0x5A, 0x5A, -0x5A);
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), pos[1], x, z, VS_WHEEL_H(vs), &pos[1], (s16 *)&VS_HEADING(vs), type, vs, 0, &avg);
    func_8029F85C(parts, D_803F876C[n], BUF0(n), BUF1(n));
    func_802A039C(0, 100, parts);
    func_802A03D4(0, 0, parts);
    func_802A040C(0, 0, parts);
    func_802A0480(0, 0, parts, 0.0f);
    func_802A0290(0, 1, parts);
    func_8029E558(parts, BUF0(n), BUF1(n));
    func_802A0320(0, parts);
    func_802A0290(0, 1, parts);
    func_8029E558(parts, BUF1(n), BUF0(n));
    SET_GEARS(vs, -0xB4, 0, 1, 0, 0x50, 1, 0x50, 0x8C, 1, 0x8C, 0xBE, 1, 0xBE, 0xFA, 1);
    func_8029C354(type, MODEL_AT(model, 4), MODEL_AT(model, 8), BARGE_SCALE);
    func_80258230(type, 0x96, 0x2D, 0x2D);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802C8BB8(type);
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1(n), BUF0(n), MODEL_MTX_OFF(D_803F876C[n]));
}

/* barge n each frame: its sound, the throttle (no steering), the turn,
   the slope and the move; bumped into something, back to where it was at
   the frame's start and turned round (once until a frame without) */
static void barge_frame(s32 n) {
    VS *vs = &D_803F8550[n];
    Part *parts = PARTS(n);
    s32 type = barge_type[n], *pos = &D_803F8748[3 * n], step = 0, x, z;
    f32 rate;

    func_802A75DC((u8 *)parts, &pos[0], &pos[1], &pos[2], (u8 *)vs);
    func_802C4724(0x71);
    func_802C9B30();
    func_802C4584((u32)iabs(VS_SPEED(vs)) >> 5);
    func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), BARGE_BRAKE, vs, &step);
    func_802A7FD8(BARGE_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 0, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, type, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), BARGE_SLOPE_DIV, vs);
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &pos[0], &pos[2], rate, &z);
    D_803ED40B = 0;
    func_802A8768(x, z, &pos[0], &pos[2], &pos[1], type, BARGE_SPAN, BARGE_SPAN, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(parts, FRAME_BUF(BUF0(n), BUF1(n)), OTHER_BUF(BUF0(n), BUF1(n)));
    barge_draw_fn[n](vs);
    /* what it hits */
    func_8029A800(pos[0], pos[1], pos[2], D_80306410, 0, 0, VS_SPEED(vs), 0, type, vs);
    func_8029C52C(type, vs);
    func_8029AA10();
    D_803F77D0 = parts;
    func_802BE77C(type, vs);
    if (D_803A7424 == 0) {
        D_803F8790[n] = 0;
    } else if (D_803F8790[n] == 0) {
        func_802A768C((u8 *)parts, &pos[0], &pos[1], &pos[2], (u32 *)OTHER_BUF(BUF0(n), BUF1(n)),
                      (u32 *)FRAME_BUF(BUF0(n), BUF1(n)), 0x800, (u8 *)vs);
        func_802C95D8(vs);
        D_803F8790[n] = 1;
        barge_draw_fn[n](vs);
        /* (and not handed to the player and the camera this frame) */
        return;
    }
    PLAYER_FROM(pos[0], pos[1], pos[2], vs, type);
}

/* barge n's matrix, its vertices, its collision and its shadow */
static void barge_draw(s32 n, VS *vs) {
    s32 type = barge_type[n], *pos = &D_803F8748[3 * n];
    u8 *model = D_803F876C[n], *buf = FRAME_BUF(BUF0(n), BUF1(n));

    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(pos[0], pos[1], pos[2], BARGE_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(pos[0], pos[1], pos[2], type, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(type, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
    func_802AABE4(type, (u16 *)MODEL_AT(model, 8), buf, 0, 0);
    func_8029D040(pos[0], pos[2], type, MODEL_AT(model, 0xC), VS_HEADING(vs), PARTS(n), buf);
}

/* set up: from the level loader, with the model file, the position and
   the heading (barges 0, 1, 2) */
REGS(s2, t7, s3, s0, s1)
void func_802C8150(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    ENGINE_COST(802C8150, 200);
    barge_setup(0, model, x, y, z, heading);
}

REGS(s2, t7, s3, s0, s1)
void func_802C8470(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    ENGINE_COST(802C8470, 200);
    barge_setup(1, model, x, y, z, heading);
}

REGS(s2, t7, s3, s0, s1)
void func_802C8790(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    ENGINE_COST(802C8790, 200);
    barge_setup(2, model, x, y, z, heading);
}

/* each frame (each copy runs its own barge, whatever `type` says) */
REGS(a0)
void func_802C8C90(u8 type) {
    ENGINE_COST(802C8C90, 152);
    barge_frame(0);
}

REGS(a0)
void func_802C8FA8(u8 type) {
    ENGINE_COST(802C8FA8, 152);
    barge_frame(1);
}

REGS(a0)
void func_802C92C0(u8 type) {
    ENGINE_COST(802C92C0, 152);
    barge_frame(2);
}

REGS(gp)
void func_802C9624(VS *vs) {
    ENGINE_COST(802C9624, 95);
    barge_draw(0, vs);
}

REGS(gp)
void func_802C97C0(VS *vs) {
    ENGINE_COST(802C97C0, 95);
    barge_draw(1, vs);
}

REGS(gp)
void func_802C995C(VS *vs) {
    ENGINE_COST(802C995C, 95);
    barge_draw(2, vs);
}

/* ---- shared ---- */

/* the barge of this type's number (another type: barge 2's, as the
   original's dispatchers have it) */
static s32 barge_of(s32 type) {
    return type == VEHICLE_BARGE ? 0 : type == VEHICLE_BARGE_2 ? 1 : 2;
}

/* set up a barge: from the level loader, with its type */
REGS(t3, s2, t7, s3, s0, s1)
void func_802C80D0(s32 type, u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    static void (*const setup[3])(u8 *, s32, s32, s32, s32) = { func_802C8150, func_802C8470, func_802C8790 };

    ENGINE_COST(802C80D0, 24);
    setup[barge_of(type)](model, x, y, z, heading);
}

/* hd.c's: the player gets in */
void func_802C8AB0(void) {
    ENGINE_COST(802C8AB0, 16);
    D_8036444C = 3000;
    D_80364450 = 0;
    func_802C4310(0x72);
}

/* hd.c's: whether it can be left (always) */
u8 func_802C8AF0(void) {
    ENGINE_COST(802C8AF0, 7);
    return 1;
}

/* hd.c's: the player gets out of the barge of this type */
void func_802C8B0C(u8 type) {
    s32 n = type == VEHICLE_BARGE_2 ? 1 : type == VEHICLE_BARGE_3 ? 2 : 0;    /* (another type: barge 0's) */

    ENGINE_COST(802C8B0C, 23);
    func_802A7764((u32 *)BUF0(n), (u32 *)BUF1(n), 0x800);
    func_802C444C();
}

/* each frame: the barge of this type */
void func_802C8BB8(u8 type) {
    static void (*const frame[3])(u8) = { func_802C8C90, func_802C8FA8, func_802C92C0 };

    ENGINE_COST(802C8BB8, 46);
    frame[barge_of(type)](type);
}

/* bumped: the speed at least BARGE_HIT_MIN_SPEED either way, then turned
   round and halved */
REGS(gp)
void func_802C95D8(VS *vs) {
    s32 v = VS_SPEED(vs);

    ENGINE_COST(802C95D8, 15);
    if (v < 0) {
        if (v > -BARGE_HIT_MIN_SPEED)
            v = -BARGE_HIT_MIN_SPEED;
    } else if (v < BARGE_HIT_MIN_SPEED) {
        v = BARGE_HIT_MIN_SPEED;
    }
    VS_SPEED(vs) = -v >> 1;
}

/* the steering rate: the speed over BARGE_STEER_DIV (nothing steers a
   barge, though) */
REGS(gp -> s3)
s32 func_802C9AF8(VS *vs) {
    ENGINE_COST(802C9AF8, 7);
    return engine_cvt_w_s((f32)VS_SPEED(vs) / BARGE_STEER_DIV);
}

/* the physics' settings for a barge: gravity, and how it lands */
void func_802C9B30(void) {
    ENGINE_COST(802C9B30, 23);
    D_803EBBF4 = D_803EBBF0 * BARGE_GRAVITY;
    D_803ED3F6 = BARGE_BOUNCE_MIN;
    D_803ED3F7 = BARGE_BOUNCE_DIV;
}
