/*
 * hd_code 88160 (us.v11 0x802CC920-0x802CDA08): the A-Team van
 * (VEHICLE_ATEAM), as native C (engine.h, vehicle.h).  Its parts are
 * D_803F8F50, its state D_803F9250 and its position D_803F92F8..9300.
 * func_802CC920 sets it up (from the level loader), func_802CD068 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.  It is the police car's code (86F60) with its own numbers,
 * without the siren.
 *
 * func_802CCED4 and func_802CCF60, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the van's .bss (asm/data/hd_code/88160.bss.s) */
extern Part D_803F8F50[32];
extern VS D_803F9250;
extern s32 D_803F92F8, D_803F92FC, D_803F9300;  /* x, y, z */
extern u8 *PTR32 D_803F9304;                    /* its model file */
extern u8 *PTR32 D_803F9308;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803F930C;
extern u16 D_803F9310;                          /* the heading it turns to against a wall (D_803A7425) */
extern u8 D_803F9312;                           /* frames until the next sparks */
extern s8 D_803F9313;                           /* turning to it */
extern u8 D_803F9314;                           /* frames without the throttle after a hit */

#define PARTS D_803F8F50
#define X D_803F92F8
#define Y D_803F92FC
#define Z D_803F9300
#define MODEL D_803F9304
#define BUF0 D_803F9308
#define BUF1 D_803F930C

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306440[];                         /* its parts' collision (56040's func_8029A800) */
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);

void func_802CD068(void);
void func_802CCE90(void);
REGS(gp)
void func_802CD578(VS *vs);
REGS(gp)
void func_802CD660(VS *vs);
REGS(gp)
void func_802CD800(VS *vs);
REGS(gp -> s3)
s32 func_802CD938(VS *vs);
void func_802CD9AC(void);

#define T(p) ((s32)(p))

/* ---- the van's numbers (a frame, where it's per frame) ------------------- */

#define VAN_SCALE 0x55F0                /* its model's scale */
#define VAN_SPAN_ALONG 0x320            /* its wheels' spans (func_802A8768) */
#define VAN_SPAN_ACROSS 0x1F4
#define VAN_BRAKE 0x19                  /* speed lost braking or against the stick, a frame */
#define VAN_TURN_RATE 0x4E20            /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define VAN_SLOPE_DIV 800.0f            /* func_802A843C: the slope's push is the height difference over this */
#define VAN_STEER_DIV 3.6f              /* the steering rate: the speed over this ... */
#define VAN_STEER_DIV_AIR 11.0f         /* ... or this with a wheel in the air */
#define VAN_WALL_TURN 0.16f             /* func_802A71DC: turning along a wall, times the speed */
#define VAN_HIT_FRAMES 5                /* frames without the throttle after hitting something */
#define VAN_HIT_MIN_SPEED 0x32          /* the speed it bounces back with is at least half this */
#define VAN_GRAVITY 4.0f                /* times the level's */
#define VAN_BOUNCE_MIN 0x3C             /* a landing harder than this bounces ... */
#define VAN_BOUNCE_DIV 3                /* ... at the speed over this */

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802CC920(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F9250;
    s32 avg;

    ENGINE_COST(802CC920, 219);
    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x100;
    D_80358070 += 0x200;
    func_802A1388(VEHICLE_ATEAM, 0, BUF0, BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0xFA, 0x190, -0xFA, 0x190, 0xFA, -0x190);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x140, 0x1F4, -0x140, 0x1F4, 0x140, -0x1F4);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_ATEAM, vs, 0, &avg);
    func_8029F85C(PARTS, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, PARTS);
    func_802A03D4(0, 0, PARTS);
    func_802A040C(0, 0, PARTS);
    func_802A0480(0, 0, PARTS, 0.0f);
    func_802A0290(0, 1, PARTS);
    func_8029E558(PARTS, BUF0, BUF1);
    func_802A0320(0, PARTS);
    func_802A0290(0, 1, PARTS);
    func_8029E558(PARTS, BUF1, BUF0);
    SET_GEARS(vs, -0xB4, 0, 8, 0, 0x50, 8, 0x50, 0xA0, 8, 0xA0, 0xFA, 8, 0xFA, 0x14A, 4);
    D_803F9312 = 0;
    D_803F9313 = 0;
    D_803F9314 = 0;
    func_8029C354(VEHICLE_ATEAM, MODEL_AT(model, 4), MODEL_AT(model, 8), VAN_SCALE);
    func_80258230(VEHICLE_ATEAM, 0x3C, 0x19, 0x19);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802CD068();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
}

/* hd.c's: the player gets in */
void func_802CCC8C(void) {
    ENGINE_COST(802CCC8C, 19);
    VS_TURNING(&D_803F9250) = 0;
    D_8036444C = 3000;
    D_80364450 = 1000;
    func_802C4310(0x7B);
}

/* hd.c's: whether it can be left: not while a wheel is in the air */
u8 func_802CCCD8(void) {
    ENGINE_COST(802CCCD8, 23);
    return !ANY_AIRBORNE(&D_803F9250);
}

/* hd.c's: the player gets out */
void func_802CCD34(void) {
    ENGINE_COST(802CCD34, 10);
    VS_SPEED(&D_803F9250) = 0;
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x100);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802CCD80(void) {
    VS *vs = &D_803F9250;

    ENGINE_COST(802CCD80, 34);
    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_ATEAM, vs, 0);
    func_802CD800(vs);
    func_802A133C(X, Y, Z, VEHICLE_ATEAM, vs);
}

/* its light */
void func_802CCE90(void) {
    ENGINE_COST(802CCE90, 17);
    func_802ABD54(VEHICLE_ATEAM, X, Y, Z);
}

/* hit something: back to where it was at the frame's start (and its
   vertices), the throttle off for VAN_HIT_FRAMES, the speed turned round
   and halved (at least VAN_HIT_MIN_SPEED before) */
static void van_bounce(VS *vs) {
    s32 v;

    D_803F9313 = 0;
    func_802A768C((u8 *)PARTS, &X, &Y, &Z, (u32 *)OTHER_BUF(BUF0, BUF1), (u32 *)FRAME_BUF(BUF0, BUF1), 0x100,
                  (u8 *)vs);
    D_803F9314 = VAN_HIT_FRAMES;
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < VAN_HIT_MIN_SPEED)
            v = VAN_HIT_MIN_SPEED;
    } else if (v > -VAN_HIT_MIN_SPEED) {
        v = -VAN_HIT_MIN_SPEED;
    }
    VS_SPEED(vs) = -v >> 1;
    func_802CD800(vs);
}

/* each frame */
void func_802CD068(void) {
    VS *vs = &D_803F9250;
    s32 step = 0, x, z, rate_i;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802CD068, 214);
    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    func_802CCE90();
    func_802A75DC((u8 *)PARTS, &X, &Y, &Z, (u8 *)vs);
    func_802C4724(0x54);
    if (VS_IN_SETUP(vs) == 0)
        func_802CD578(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    func_802CD9AC();
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802CD938(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803F9314 == 0)
        func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), VAN_BRAKE, vs, &step);
    else
        D_803F9314--;
    func_802A7FD8(VAN_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_ATEAM, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), VAN_SLOPE_DIV, vs);
    if (D_803F9313 != 0)
        func_802A7070((s16 *)&D_803F9310, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_ATEAM, VAN_SPAN_ALONG, VAN_SPAN_ACROSS, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(PARTS, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802CD800(vs);
    /* what it hits */
    func_8029A800(X, Y, Z, D_80306440, 1, 1, 7, VS_SPEED(vs), 0x96, 0, VEHICLE_ATEAM, vs);
    func_8029C52C(VEHICLE_ATEAM, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = PARTS;
        func_802BE77C(VEHICLE_ATEAM, vs);
        if (D_803A7424 == 0)
            D_803F9313 = 0;
        else
            van_bounce(vs);
    } else {
        D_803F9313 = 1;
        turn_along_wall(vs, &D_803F9310, VAN_WALL_TURN);
        D_803F77D0 = PARTS;
        func_802BE77C(VEHICLE_ATEAM, vs);
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_ATEAM);
}

/* the dust, the wheels' sparks every other frame while it turns (with
   room for them), and the engine's sound */
REGS(gp)
void func_802CD578(VS *vs) {
    ENGINE_COST(802CD578, 24);
    func_802CD660(vs);
    if (D_803F9312 != 0) {
        D_803F9312--;
    } else if (VS_TURNING(vs) != 0) {
        D_803F9312 = 1;
        if (func_802A5ED0() < 0xF) {
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_ATEAM, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_ATEAM, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
        }
    }
    func_802C4584((u32)iabs(VS_SPEED(vs)) >> 5);
}

/* dust behind it while turning on soft ground (grip under 3, not the
   static triangles) with its back wheel down */
REGS(gp)
void func_802CD660(VS *vs) {
    ENGINE_COST(802CD660, 71);
    if (VS_TURNING(vs) != 0 && VS_AIRBORNE(vs)[2] != 1 && VS_GRIP(vs) < 3 && VS_ON_STATIC(vs) == 0)
        func_8027BE7C(3, VS_WHEEL_H(vs)[6], 0xFA, -0x190, -0x190, -0x190, X, Z, VS_MOVE_HEADING(vs), 3, 0x32, 0x32,
                      0);
}

/* the van's matrix, its vertices and its collision */
REGS(gp)
void func_802CD800(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);

    ENGINE_COST(802CD800, 70);
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, VAN_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(X, Y, Z, VEHICLE_ATEAM, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_ATEAM, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over VAN_STEER_DIV, or over
   VAN_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802CD938(VS *vs) {
    ENGINE_COST(802CD938, 25);
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? VAN_STEER_DIV_AIR : VAN_STEER_DIV));
}

/* the physics' settings for the van: gravity, and how its wheels land */
void func_802CD9AC(void) {
    ENGINE_COST(802CD9AC, 23);
    D_803EBBF4 = D_803EBBF0 * VAN_GRAVITY;
    D_803ED3F6 = VAN_BOUNCE_MIN;
    D_803ED3F7 = VAN_BOUNCE_DIV;
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802CCED4(s32 carrier) {
    CARRY_KEEP(802CCED4, 802CCF10, 802CCF30, 802CCF3C, &D_803F9250, D_803F92F8, D_803F9300);
}

REGS(a3)
void func_802CCF60(s32 carrier) {
    CARRY_MOVE(802CCF60, 802CCF9C, 802CCFB0, 802CCFB8, 802CD018, 802CD020, 802CD04C, &D_803F9250, &D_803F92F8,
               &D_803F92FC, &D_803F9300, 0xE, 0x320, 0x1F4, D_803ED40B = 1, func_802CD9AC(),
               func_802CD800(&D_803F9250));
}
