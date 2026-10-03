/*
 * hd_code 71140 (us.v11 0x802B5900-0x802B7334): the Backlash truck
 * (VEHICLE_TRUCK), as native C (engine.h), and the two functions of 679E0
 * only it calls (func_802AC284, func_802AC2A4).  Its parts are D_803EE790,
 * its state D_803EEA90 and its position D_803EEB38..40 (vehicle.h).
 * func_802B5900 sets it up (from the level loader), func_802B6294 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.  Part 5 is its tilting bed (PAD_LEFT/PAD_RIGHT,
 * D_803EEB54), part 4 its suspension (D_803EEB50), part 2 the cab's lean,
 * parts D_802C2208/D_802C226C its wheels' turn with the camera.
 *
 * func_802B6100 and func_802B618C, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the truck's .bss (asm/data/hd_code/71140.bss.s) */
extern Part D_803EE790[32];
extern VS D_803EEA90;
extern s32 D_803EEB38, D_803EEB3C, D_803EEB40;  /* x, y, z */
extern u8 *PTR32 D_803EEB44;                    /* its model file */
extern u8 *PTR32 D_803EEB48;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EEB4C;
extern f32 D_803EEB50;                          /* the suspension, 0..1 (0.5 level) */
extern f32 D_803EEB54;                          /* the bed's tilt, 0..1 (0.5 level) */
extern u16 D_803EEB58;                          /* its turn rate (slower with L or R) */
extern u16 D_803EEB5A;                          /* the heading it turns to against a wall (D_803A7425) */
extern s16 D_803EEB5C;                          /* the heading the wheels turn from */
extern u8 D_803EEB5E;                           /* frames until the next sparks */
extern u8 D_803EEB5F;                           /* the cab's lean, 0..100 */
extern s8 D_803EEB60;                           /* turning to it */
extern s8 D_803EEB61;                           /* a sound to play this frame, or -1 */

#define PARTS D_803EE790
#define X D_803EEB38
#define Y D_803EEB3C
#define Z D_803EEB40
#define MODEL D_803EEB44
#define BUF0 D_803EEB48
#define BUF1 D_803EEB4C

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s32 D_80364AA8;
extern f32 D_80364414;                          /* the camera's heading, degrees */
extern Part *PTR32 D_803F77D0;
extern u8 D_80305D20[];                         /* its parts' collision (56040's func_8029A800) */
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */
extern u8 D_802C2208[], D_802C226C[];           /* the front wheels (56040's list) */
extern u8 *PTR32 D_803F3910[];                  /* the objects it is carrying, to D_803F3960 */
extern u8 *PTR32 *PTR32 D_803F3960;
extern u16 D_8036E4C8;
extern u8 D_803BE738;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);
void func_80278EB0(s32 a, f32 b, s32 c);
u8 func_802794F0(void);
void func_802794A4(void);

/* 77E20's: an object moved by (dx, dz) */
REGS(v0, a2, a3, t0)
void func_802BD99C(u8 *obj, s32 dx, s32 a3, s32 dz);

void func_802B6294(void);
void func_802B60BC(void);
REGS(gp)
void func_802B69F8(VS *vs);
REGS(gp)
void func_802B6C28(VS *vs);
REGS(gp)
void func_802B7030(VS *vs);
REGS(gp -> s3)
s32 func_802B7168(VS *vs);
REGS(gp, s2 -> f2)
f32 func_802B71DC(VS *vs, s32 up);
void func_802B7240(void);
REGS(v0, v1, a0)
void func_802AC284(s32 *x, s32 *y, s32 *z);
REGS(v0, v1, a0, a1, t8, gp)
void func_802AC2A4(s32 x, s32 y, s32 z, u8 *a1, s32 type, VS *vs);

#define T(p) ((s32)(p))

/* ---- the truck's numbers (a frame, where it's per frame) ----------------- */

#define TRUCK_SCALE 0x4268              /* its model's scale */
#define TRUCK_SPAN 0x2D0                /* its wheels' spans, both ways (func_802A8768) */
#define TRUCK_BRAKE 8                   /* speed lost braking or against the stick, a frame */
#define TRUCK_TURN_RATE 0x2328          /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define TRUCK_TURN_RATE_SLOW 0xC80      /* ... with L or R held */
#define TRUCK_SLOPE_DIV 720.0f          /* func_802A843C: the slope's push is the height difference over this */
#define TRUCK_STEER_DIV 2.6f            /* the steering rate: the speed over this ... */
#define TRUCK_STEER_DIV_AIR 6.0f        /* ... or this with a wheel in the air */
#define TRUCK_WALL_TURN 0.18f           /* func_802A71DC: turning along a wall, times the speed */
#define TRUCK_HIT_MIN_SPEED 0x50        /* the speed it bounces back with is at least half this */
#define TRUCK_GRAVITY 2.0f              /* times the level's */
#define TRUCK_BOUNCE_MIN 0x3C           /* a landing harder than this bounces ... */
#define TRUCK_BOUNCE_DIV 2              /* ... at the speed over this */
#define BED_STEP 0.05f                  /* the bed's tilt a frame (0..1; the suspension: pad_lift) */
#define CAB_LEAN_STEP 5                 /* the cab's lean a frame (0..100) ... */
#define CAB_LEAN_BACK 10                /* ... and back toward the middle */
#define WHEEL_SPIN_DIV 14               /* the wheels' spin: the speed over this */

/* the camera's heading turned into a 12-bit angle, added to h */
static s32 camera_turn(s32 h) {
    return h + engine_cvt_w_s(D_80364414 * 4096.0f / 360.0f);
}

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802B5900(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EEA90;
    Part *p = PARTS;
    s32 avg, h;

    ENGINE_COST(802B5900, 245);
    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(VEHICLE_TRUCK, 1, BUF0, BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x168, 0x168, -0x168, 0x168, 0x168, -0x168);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x17C, 0x17C, -0x17C, 0x17C, 0x17C, -0x17C);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_TRUCK, vs, 0, &avg);
    func_8029F85C(p, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, p);
    func_802A03D4(0, 0, p);
    func_802A040C(0, 0, p);
    func_802A0480(0, 0, p, 0.0f);
    func_802A0290(0, 1, p);
    func_8029E558(p, BUF0, BUF1);
    func_802A0320(0, p);
    func_802A0290(0, 1, p);
    func_8029E558(p, BUF1, BUF0);
    SET_GEARS(vs, -0x78, 0, 2, 0, 0x50, 6, 0x50, 0x64, 4, 0x64, 0x8C, 2, 0x8C, 0xDC, 2);
    func_802A6F00(vs);
    D_803EEB60 = 0;
    D_803EEB5E = 0;
    D_803EEB5F = 50;
    D_803EEB50 = 0.5f;
    D_803EEB54 = 0.5f;
    func_8029C354(VEHICLE_TRUCK, MODEL_AT(model, 4), MODEL_AT(model, 8), TRUCK_SCALE);
    func_80258230(VEHICLE_TRUCK, 0x64, 0x2D, 0x2D);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802B6294();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
    h = camera_turn((u16)VS_HEADING(vs));
    if (h >= ANGLE_TURN)
        h -= ANGLE_WRAP;
    D_803EEB5C = h;
}

/* hd.c's: the player gets in: the bed and wheels reset */
void func_802B5CD8(void) {
    Part *p = PARTS;

    ENGINE_COST(802B5CD8, 139);
    VS_TURNING(&D_803EEA90) = 0;
    func_802A039C(1, 0, p);
    func_802A03D4(1, 0, p);
    func_802A040C(1, 0, p);
    func_802A0290(1, -1, p);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 1, p);
    func_802A0360(2, 0, p, 0.5f);
    func_802A0290(2, -1, p);
    func_802A039C(4, 0, p);
    func_802A03D4(4, 0, p);
    func_802A040C(4, 1, p);
    func_802A0290(4, -1, p);
    func_802A039C(5, 0, p);
    func_802A03D4(5, 0, p);
    func_802A040C(5, 1, p);
    func_802A0290(5, -1, p);
    func_802A05D0(D_802C2208, 0);
    func_802A05F8(D_802C2208, 0);
    func_802A0620(D_802C2208, 0);
    func_802A0508(D_802C2208, -1);
    func_802A05D0(D_802C226C, 0);
    func_802A05F8(D_802C226C, 0);
    func_802A0620(D_802C226C, 0);
    func_802A0508(D_802C226C, -1);
    D_8036444C = 0xD48;
    D_80364450 = 0x12C;
    func_802C4310(5);
}

/* hd.c's: whether it can be left: not while a wheel is in the air */
u8 func_802B5F04(void) {
    ENGINE_COST(802B5F04, 23);
    return !ANY_AIRBORNE(&D_803EEA90);
}

/* hd.c's: the player gets out */
void func_802B5F60(void) {
    ENGINE_COST(802B5F60, 19);
    VS_SPEED(&D_803EEA90) = 0;
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x800);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802B5FAC(void) {
    VS *vs = &D_803EEA90;

    ENGINE_COST(802B5FAC, 35);
    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_TRUCK, vs, 0);
    func_802B7030(vs);
    func_802A133C(X, Y, Z, VEHICLE_TRUCK, vs);
}

/* its light */
void func_802B60BC(void) {
    ENGINE_COST(802B60BC, 17);
    func_802ABD54(VEHICLE_TRUCK, X, Y, Z);
}

/* hit something: back to where it was at the frame's start (and its
   vertices), the speed turned round and halved (at least
   TRUCK_HIT_MIN_SPEED before) */
static void truck_bounce(VS *vs) {
    s32 v;

    D_803EEB60 = 0;
    func_802A768C((u8 *)PARTS, &X, &Y, &Z, (u32 *)OTHER_BUF(BUF0, BUF1), (u32 *)FRAME_BUF(BUF0, BUF1), 0x800,
                  (u8 *)vs);
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < TRUCK_HIT_MIN_SPEED)
            v = TRUCK_HIT_MIN_SPEED;
    } else if (v > -TRUCK_HIT_MIN_SPEED) {
        v = -TRUCK_HIT_MIN_SPEED;
    }
    VS_SPEED(vs) = -v >> 1;
    func_802B7030(vs);
}

/* each frame */
void func_802B6294(void) {
    VS *vs = &D_803EEA90;
    s32 step = 0, x, z, rate_i, turn;
    f32 rate;

    ENGINE_COST(802B6294, 259);
    func_802B60BC();
    func_802A75DC((u8 *)PARTS, &X, &Y, &Z, (u8 *)vs);
    if (VS_IN_SETUP(vs) == 0)
        func_802B6C28(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    func_802B7240();
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802B7168(vs);
    func_802A7834(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), TRUCK_BRAKE, rate_i, &VS_HEADING(vs), vs,
                  &step, &turn);
    func_802A7FD8(D_803EEB58, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_TRUCK, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), TRUCK_SLOPE_DIV, vs);
    if (D_803EEB60 != 0)
        func_802A7070((s16 *)&D_803EEB5A, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_TRUCK, TRUCK_SPAN, TRUCK_SPAN, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(PARTS, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    /* the engine's rumble while turning */
    if (VS_TURNING(vs) != 0) {
        if (func_802794F0() == 0)
            func_80278EB0(6, 0.25f, 100);
    } else {
        func_802794A4();
    }
    func_802B69F8(vs);
    /* a spark every other frame while turning, with room for it */
    if (D_803EEB5E != 0) {
        D_803EEB5E--;
    } else if (VS_TURNING(vs) != 0) {
        D_803EEB5E = 1;
        if (func_802A5ED0() < 4)
            func_802A6274(T(D_802C2954), 0x30D40, 1, VEHICLE_TRUCK, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    }
    func_802AC284(&X, &Y, &Z);
    func_802B7030(vs);
    /* what it hits (not once the level is over, D_80364AA8 0x40) */
    func_8029A800(D_803EEB38, D_803EEB3C, D_803EEB40, D_80305D20, 1, 1, 8, vs->unk76, 0x64, 0, 5, vs);
    func_8029C52C(VEHICLE_TRUCK, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803EEB61 = -1;
        if (D_80364AA8 != 0x40) {
            D_803F77D0 = PARTS;
            func_802BE77C(VEHICLE_TRUCK, vs);
        }
        if (D_803A7424 != 0)
            truck_bounce(vs);
        else
            D_803EEB60 = 0;
    } else {
        D_803EEB61 = 0xC;
        D_803EEB60 = 1;
        turn_along_wall(vs, &D_803EEB5A, TRUCK_WALL_TURN);
        if (D_80364AA8 != 0x40) {
            D_803F77D0 = PARTS;
            func_802BE77C(VEHICLE_TRUCK, vs);
        }
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_TRUCK);
    D_803F77D0 = PARTS;
    func_802AC2A4(X, Y, Z, D_80305D20, VEHICLE_TRUCK, vs);
    if (D_803EEB61 >= 0)
        func_80260650(D_80367738, D_803EEB61, NULL);
}

/* dust behind it while turning on soft ground (grip under 3, not the
   static triangles) with its back wheel down, thrown to the side the bed
   tilts to */
REGS(gp)
void func_802B69F8(VS *vs) {
    f32 f;
    s32 t, left, right;

    ENGINE_COST(802B69F8, 78);
    if (VS_TURNING(vs) == 0 || VS_AIRBORNE(vs)[2] == 1 || VS_GRIP(vs) >= 3 || VS_ON_STATIC(vs) != 0)
        return;
    f = D_803EEB54;
    if (f <= 0.5f) {
        t = engine_cvt_w_s((0.5f - f) * 2.0f * 20.0f);
        left = 0x28 - t;
        right = t + 0x28;
    } else {
        t = engine_cvt_w_s((f - 0.5f) * 2.0f * 20.0f);
        left = t + 0x28;
        right = 0x28 - t;
    }
    func_8027BE7C(3, VS_WHEEL_H(vs)[6], 0x190, -0x190, -0x190, -0x190, X, Z, VS_MOVE_HEADING(vs), 5, left, right, 0);
}

/* The parts a frame: the front wheels turn with the camera; the bed tilts
   with PAD_LEFT/PAD_RIGHT (as far as the speed allows, func_802B71DC) or
   back to level; the suspension goes down with B or Z and up with A
   unless near the top speed, or back to level; the wheels spin with the
   speed; the cab leans with PAD_LEFT/PAD_RIGHT; and the engine's sound. */
REGS(gp)
void func_802B6C28(VS *vs) {
    Part *p = PARTS;
    s32 a, s, t;
    f32 f, g;

    ENGINE_COST(802B6C28, 168);
    /* the wheels: the camera's heading from where they started, in 0x55 steps */
    a = camera_turn((u16)VS_HEADING(vs));
    if (a >= ANGLE_TURN)
        a -= ANGLE_WRAP;
    a -= D_803EEB5C;
    if (a < 0)
        a += ANGLE_WRAP;
    if (a >= ANGLE_HALF)
        a -= ANGLE_HALF;
    a = (u32)a / 0x55;
    func_802A05A4(D_802C2208, a, 0.0f);
    func_802A05A4(D_802C226C, a, 0.0f);
    /* the bed */
    f = D_803EEB54;
    if (PAD_LEFT != 0) {
        f = f - BED_STEP;
        g = func_802B71DC(vs, 0);
        if (f < g)
            f = g;
    } else if (PAD_RIGHT != 0) {
        f = f + BED_STEP;
        g = func_802B71DC(vs, 1);
        if (!(f <= g))
            f = g;
    } else {
        f = step_toward_f(f, 0.5f, BED_STEP);
    }
    D_803EEB54 = f;
    func_802A0360(5, 0, p, f);
    /* the suspension */
    f = pad_lift(vs, D_803EEB50, 0xA);
    D_803EEB50 = f;
    func_802A0360(4, 0, p, f);
    /* the wheels' spin, backwards in reverse */
    s = VS_SPEED(vs);
    func_802A03D4(1, s < 0 ? 1 : 0, p);
    s = (u32)iabs(s) / WHEEL_SPIN_DIV;
    func_802A039C(1, s, p);
    func_802C4584(s);
    /* the cab */
    t = pad_lever(D_803EEB5F, CAB_LEAN_STEP, CAB_LEAN_BACK);
    D_803EEB5F = t;
    func_802A0360(2, 0, p, (f32)t / 100.0f);
}

/* the truck's matrix, its vertices and its collision */
REGS(gp)
void func_802B7030(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);

    ENGINE_COST(802B7030, 70);
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, TRUCK_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(X, Y, Z, VEHICLE_TRUCK, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_TRUCK, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over TRUCK_STEER_DIV, or over
   TRUCK_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802B7168(VS *vs) {
    ENGINE_COST(802B7168, 25);
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? TRUCK_STEER_DIV_AIR : TRUCK_STEER_DIV));
}

/* the bed's tilt limit at this speed: 0.5 -/+ |speed| / 240 / 2 */
REGS(gp, s2 -> f2)
f32 func_802B71DC(VS *vs, s32 up) {
    f32 f;

    ENGINE_COST(802B71DC, 23);
    f = (f32)iabs(VS_SPEED(vs)) / 240.0f * 0.5f;
    return up != 0 ? f + 0.5f : 0.5f - f;
}

/* the physics' settings for the truck: gravity, how its wheels land, and
   its turn rate (slower with L or R) */
void func_802B7240(void) {
    ENGINE_COST(802B7240, 38);
    D_803EBBF4 = D_803EBBF0 * TRUCK_GRAVITY;
    D_803ED3F6 = TRUCK_BOUNCE_MIN;
    D_803ED3F7 = TRUCK_BOUNCE_DIV;
    D_803EEB58 = PAD_L != 0 || PAD_R != 0 ? TRUCK_TURN_RATE_SLOW : TRUCK_TURN_RATE;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B72DC(u8 *dst) {
    ENGINE_COST(802B72DC, 7);
    func_802AC7DC(dst, (u8 *)&D_803EEA90, (u32 *)&D_803EEB38);
}

/* and back */
void func_802B7308(u8 *src) {
    ENGINE_COST(802B7308, 11);
    func_802AC85C(src, (u8 *)&D_803EEA90, (u32 *)&D_803EEB38);
}

/* ---- 679E0's, which only the truck calls ---- */

/* the position (*x, *y, *z) kept in the level's bounds (func_802AC3B8) */
REGS(v0, v1, a0)
void func_802AC284(s32 *x, s32 *y, s32 *z) {
    ENGINE_COST(802AC284, 8);
    func_802AC3B8(x, y, z);
}

/* the level over (D_80364AA8 0x40): the parts' last frame and the
   buildings', and whatever it carries (D_803F3910..D_803F3960) thrown far
   off, with a sound; with D_8036E4C8 clear, the level lost instead */
REGS(v0, v1, a0, a1, t8, gp)
void func_802AC2A4(s32 x, s32 y, s32 z, u8 *a1, s32 type, VS *vs) {
    u8 *PTR32 *q;
    u8 *o;
    s32 dx, dz;

    ENGINE_COST(802AC2A4, 12);
    if (D_80364AA8 != 0x40)
        return;
    func_8029A800(x, y, z, a1, 1, 0, 0, 0, 0, 0, type, vs);
    func_802BE77C(type, vs);
    if (D_803F3910 == D_803F3960)
        return;
    if (D_8036E4C8 == 0) {
        D_803BE738 = 1;
        return;
    }
    func_80260650(D_80367738, 0x3D, NULL);
    for (q = D_803F3910; q != D_803F3960; q += 2) {
        o = q[0];
        *(s32 *)(o + 0x38) = 1;
        *(s32 *)(o + 0x40) = -1;
        dx = 0xBB80 - *(s32 *)(o + 0x10);
        *(s32 *)(o + 0x10) = 0xBB80;
        dz = 0xBB80 - *(s32 *)(o + 0x18);
        *(s32 *)(o + 0x18) = 0xBB80;
        func_802BD99C(o, dx, 0, dz);
    }
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B6100(s32 carrier) {
    CARRY_KEEP(802B6100, 802B613C, 802B615C, 802B6168, &D_803EEA90, D_803EEB38, D_803EEB40);
}

REGS(a3)
void func_802B618C(s32 carrier) {
    CARRY_MOVE(802B618C, 802B61C8, 802B61DC, 802B61E4, 802B6244, 802B624C, 802B6278, &D_803EEA90, &D_803EEB38, &D_803EEB3C, &D_803EEB40, 5, 0x2D0, 0x2D0,
               D_803ED40B = 1, func_802B7240(), func_802B7030(&D_803EEA90));
}
