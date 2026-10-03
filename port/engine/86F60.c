/*
 * hd_code 86F60 (us.v11 0x802CB720-0x802CC920): the police car
 * (VEHICLE_POLICE), as native C (engine.h).  Its parts are D_803F8B80, its
 * state D_803F8E80 and its position D_803F8F28..30 (vehicle.h).
 * func_802CB720 sets it up (from the level loader), func_802CBEF0 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.  It is the A-Team van's code (88160) with its own numbers,
 * and the siren's lights.
 *
 * func_802CBD5C and func_802CBDE8, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the police car's .bss (asm/data/hd_code/86F60.bss.s) */
extern Part D_803F8B80[32];
extern VS D_803F8E80;
extern s32 D_803F8F28, D_803F8F2C, D_803F8F30;  /* x, y, z */
extern u8 *PTR32 D_803F8F34;                    /* its model file */
extern u8 *PTR32 D_803F8F38;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803F8F3C;
extern u16 D_803F8F40;                          /* the heading it turns to against a wall (D_803A7425) */
extern u8 D_803F8F42;                           /* frames until the next sparks */
extern s8 D_803F8F43;                           /* turning to it */
extern u8 D_803F8F44;                           /* frames without the throttle after a hit */
extern u8 D_803F8F45;                           /* the siren's lights: a step, 2..13 */

#define PARTS D_803F8B80
#define X D_803F8F28
#define Y D_803F8F2C
#define Z D_803F8F30
#define MODEL D_803F8F34
#define BUF0 D_803F8F38
#define BUF1 D_803F8F3C

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306430[];                         /* its parts' collision (56040's func_8029A800) */
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */
extern u8 D_802C2324[], D_802C2348[];           /* the siren's two lights (56040's list) */

void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);

void func_802CBEF0(void);
void func_802CBD18(void);
REGS(gp)
void func_802CC400(VS *vs);
REGS(gp)
void func_802CC56C(VS *vs);
REGS(gp)
void func_802CC70C(VS *vs);
REGS(gp -> s3)
s32 func_802CC844(VS *vs);
void func_802CC8B8(void);

#define T(p) ((s32)(p))

/* ---- the police car's numbers (a frame, where it's per frame) ------------ */

#define POLICE_SCALE 0x4268             /* its model's scale */
#define POLICE_SPAN_ALONG 0x2BC         /* its wheels' spans (func_802A8768) */
#define POLICE_SPAN_ACROSS 0x190
#define POLICE_BRAKE 0x14               /* speed lost braking or against the stick, a frame */
#define POLICE_TURN_RATE 0x2EE0         /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define POLICE_SLOPE_DIV 700.0f         /* func_802A843C: the slope's push is the height difference over this */
#define POLICE_STEER_DIV 3.6f           /* the steering rate: the speed over this ... */
#define POLICE_STEER_DIV_AIR 11.0f      /* ... or this with a wheel in the air */
#define POLICE_WALL_TURN 0.16f          /* func_802A71DC: turning along a wall, times the speed */
#define POLICE_HIT_FRAMES 5             /* frames without the throttle after hitting something */
#define POLICE_HIT_MIN_SPEED 0x32       /* the speed it bounces back with is at least half this */
#define POLICE_GRAVITY 4.0f             /* times the level's */
#define POLICE_BOUNCE_MIN 0x96          /* a landing harder than this bounces ... */
#define POLICE_BOUNCE_DIV 6             /* ... at the speed over this */
#define POLICE_SIREN_STEPS 0xE          /* the siren's lights' cycle: steps 2..13, two frames a light */

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802CB720(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F8E80;
    s32 avg;

    ENGINE_COST(802CB720, 221);
    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x100;
    D_80358070 += 0x200;
    func_802A1388(VEHICLE_POLICE, 0, BUF0, BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0xC8, 0x15E, -0xC8, 0x15E, 0xC8, -0x15E);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x140, 0x1F4, -0x140, 0x1F4, 0x140, -0x1F4);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_POLICE, vs, 0, &avg);
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
    SET_GEARS(vs, -0xB4, 0, 2, 0, 0x50, 1, 0x50, 0xA0, 2, 0xA0, 0xB4, 3, 0xB4, 0x154, 2);
    D_803F8F42 = 0;
    D_803F8F43 = 0;
    D_803F8F44 = 0;
    D_803F8F45 = 0;
    func_8029C354(VEHICLE_POLICE, MODEL_AT(model, 4), MODEL_AT(model, 8), POLICE_SCALE);
    func_80258230(VEHICLE_POLICE, 0x3C, 0x19, 0x19);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802CBEF0();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
}

/* hd.c's: the player gets in: the siren's lights off */
void func_802CBA94(void) {
    VS *vs = &D_803F8E80;

    ENGINE_COST(802CBA94, 51);
    VS_TURNING(vs) = 0;
    D_8036444C = 3000;
    D_80364450 = 1000;
    func_802A05D0(D_802C2324, 0);
    func_802A05F8(D_802C2324, 0);
    func_802A0620(D_802C2324, 0);
    func_802A0508(D_802C2324, -1);
    func_802A05D0(D_802C2348, 0);
    func_802A05F8(D_802C2348, 0);
    func_802A0620(D_802C2348, 0);
    func_802A0508(D_802C2348, -1);
    func_802C4310(0xCE);
}

/* hd.c's: whether it can be left: not while a wheel is in the air */
u8 func_802CBB60(void) {
    ENGINE_COST(802CBB60, 23);
    return !ANY_AIRBORNE(&D_803F8E80);
}

/* hd.c's: the player gets out */
void func_802CBBBC(void) {
    ENGINE_COST(802CBBBC, 19);
    VS_SPEED(&D_803F8E80) = 0;
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x100);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802CBC08(void) {
    VS *vs = &D_803F8E80;

    ENGINE_COST(802CBC08, 35);
    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_POLICE, vs, 0);
    func_802CC70C(vs);
    func_802A133C(X, Y, Z, VEHICLE_POLICE, vs);
}

/* its light */
void func_802CBD18(void) {
    ENGINE_COST(802CBD18, 17);
    func_802ABD54(VEHICLE_POLICE, X, Y, Z);
}

/* hit something: back to where it was at the frame's start (and its
   vertices), the throttle off for POLICE_HIT_FRAMES, the speed turned round
   and halved (at least POLICE_HIT_MIN_SPEED before) */
static void police_bounce(VS *vs) {
    s32 v;

    D_803F8F43 = 0;
    func_802A768C((u8 *)PARTS, &X, &Y, &Z, (u32 *)OTHER_BUF(BUF0, BUF1), (u32 *)FRAME_BUF(BUF0, BUF1), 0x100,
                  (u8 *)vs);
    D_803F8F44 = POLICE_HIT_FRAMES;
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < POLICE_HIT_MIN_SPEED)
            v = POLICE_HIT_MIN_SPEED;
    } else if (v > -POLICE_HIT_MIN_SPEED) {
        v = -POLICE_HIT_MIN_SPEED;
    }
    VS_SPEED(vs) = -v >> 1;
    func_802CC70C(vs);
}

/* each frame */
void func_802CBEF0(void) {
    VS *vs = &D_803F8E80;
    s32 step = 0, x, z, rate_i;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802CBEF0, 214);
    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802CBD18();
    func_802A75DC((u8 *)PARTS, &X, &Y, &Z, (u8 *)vs);
    func_802C4724(VEHICLE_POLICE);
    if (VS_IN_SETUP(vs) == 0)
        func_802CC400(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    func_802CC8B8();
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802CC844(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803F8F44 == 0)
        func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), POLICE_BRAKE, vs, &step);
    else
        D_803F8F44--;
    func_802A7FD8(POLICE_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs),
                  &VS_MOVE_HEADING(vs), (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_POLICE, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), POLICE_SLOPE_DIV, vs);
    if (D_803F8F43 != 0)
        func_802A7070((s16 *)&D_803F8F40, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_POLICE, POLICE_SPAN_ALONG, POLICE_SPAN_ACROSS, VS_WHEELS(vs),
                  VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(PARTS, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802CC70C(vs);
    /* what it hits */
    ENGINE_LEAVE(8, 7);         /* $t0 and $t2: func_8029A800 (56040.c) takes them from the context */
    ENGINE_LEAVE(10, 0x96);
    func_8029A800(D_803F8F28, D_803F8F2C, D_803F8F30, D_80306430, 1, 1, vs->unk76, 0, 0xD, vs);
    func_8029C52C(VEHICLE_POLICE, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = PARTS;
        func_802BE77C(VEHICLE_POLICE, vs);
        if (D_803A7424 == 0)
            D_803F8F43 = 0;
        else
            police_bounce(vs);
    } else {
        D_803F8F43 = 1;
        turn_along_wall(vs, &D_803F8F40, POLICE_WALL_TURN);
        D_803F77D0 = PARTS;
        func_802BE77C(VEHICLE_POLICE, vs);
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_POLICE);
    engine_restore();
}

/* the siren's lights (flashing while L or R is held), the dust, the
   wheels' sparks while it turns, and the engine's sound */
REGS(gp)
void func_802CC400(VS *vs) {
    s32 l;

    ENGINE_COST(802CC400, 48);
    if (PAD_L == 0 && PAD_R == 0) {
        l = D_803F8F45 = 0;
    } else {
        l = D_803F8F45 + 1;
        if (l == POLICE_SIREN_STEPS)
            l = 2;
        D_803F8F45 = l;
        l = (u32)l >> 1;
    }
    func_802A05A4(D_802C2324, l, 0.0f);
    func_802A05A4(D_802C2348, l, 0.0f);
    func_802CC56C(vs);
    /* sparks every other frame while turning, with room for them */
    if (D_803F8F42 != 0) {
        D_803F8F42--;
    } else if (VS_TURNING(vs) != 0) {
        D_803F8F42 = 1;
        if (func_802A5ED0() < 0xF) {
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_POLICE, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_POLICE, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
        }
    }
    func_802C4584((u32)iabs(VS_SPEED(vs)) >> 5);
}

/* dust behind it while turning on soft ground (grip under 3, not the
   static triangles) with its back wheel down */
REGS(gp)
void func_802CC56C(VS *vs) {
    ENGINE_COST(802CC56C, 70);
    if (VS_TURNING(vs) != 0 && VS_AIRBORNE(vs)[2] != 1 && VS_GRIP(vs) < 3 && VS_ON_STATIC(vs) == 0)
        func_8027BE7C(3, VS_WHEEL_H(vs)[6], 0xFA, -0x190, -0x190, -0x190, X, Z, VS_MOVE_HEADING(vs), 3, 0x32, 0x32,
                      0);
}

/* the car's matrix, its vertices and its collision */
REGS(gp)
void func_802CC70C(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);

    ENGINE_COST(802CC70C, 70);
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, POLICE_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(X, Y, Z, VEHICLE_POLICE, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_POLICE, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over POLICE_STEER_DIV, or over
   POLICE_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802CC844(VS *vs) {
    ENGINE_COST(802CC844, 25);
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? POLICE_STEER_DIV_AIR : POLICE_STEER_DIV));
}

/* the physics' settings for the police car: gravity, and how its wheels
   land */
void func_802CC8B8(void) {
    ENGINE_COST(802CC8B8, 23);
    D_803EBBF4 = D_803EBBF0 * POLICE_GRAVITY;
    D_803ED3F6 = POLICE_BOUNCE_MIN;
    D_803ED3F7 = POLICE_BOUNCE_DIV;
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802CBD5C(s32 carrier) {
    CARRY_KEEP(802CBD5C, 802CBD98, 802CBDB8, 802CBDC4, &D_803F8E80, D_803F8F28, D_803F8F30);
}

REGS(a3)
void func_802CBDE8(s32 carrier) {
    CARRY_MOVE(802CBDE8, 802CBE24, 802CBE38, 802CBE40, 802CBEA0, 802CBEA8, 802CBED4, &D_803F8E80, &D_803F8F28, &D_803F8F2C, &D_803F8F30, 0xD, 0x2BC, 0x190,
               D_803ED40B = 1, func_802CC8B8(), func_802CC70C(&D_803F8E80));
}
