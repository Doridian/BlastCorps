/*
 * hd_code 8AEE0 (us.v11 0x802CF6A0-0x802D2570): the other hotrod
 * (VEHICLE_STARSKI) and the Cyclone Suit (VEHICLE_MINIMAGOO), as native C
 * (engine.h, vehicle.h).
 *
 * The hotrod's parts are D_803FC200, its state D_803FC500 and its position
 * D_803FC5A8..B0: it is 72B80's American Dream (func_802B7340..8424) with
 * its own numbers, function for function (func_802CF6A0 its setup,
 * func_802CFDE8 its frame).
 *
 * The suit's parts are D_803FC5D0, its state D_803FC8D0 and its position
 * D_803FC978..80: it is Thunderfist's code (6C5E0) with a third leg (part
 * 6, SUIT_LEG 2 at the higher speeds) and its own numbers.  func_802D07E0
 * sets it up, func_802D0F98 runs it each frame.
 *
 * func_802CFC54 and func_802CFCE0, at the end, are the hotrod's two
 * callbacks for 62740's carrying (shared.h).  Three functions nothing
 * calls, in any version, are here too, so that no translation is left:
 * func_802D0E44 (the suit put back on the ground), func_802D24F8 (its state
 * saved) and func_802D2550 (an mtc0 to Compare).
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the hotrod's .bss (asm/data/hd_code/8AEE0.bss.s) */
extern Part D_803FC200[32];
extern VS D_803FC500;
extern s32 D_803FC5A8, D_803FC5AC, D_803FC5B0;  /* x, y, z */
extern u8 *PTR32 D_803FC5B4;                    /* its model file */
extern u8 *PTR32 D_803FC5B8;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803FC5BC;
extern u16 D_803FC5C0;                          /* the heading it turns to against a wall (D_803A7425) */
extern u8 D_803FC5C2;                           /* frames until the next sparks */
extern s8 D_803FC5C3;                           /* turning to it */
extern u8 D_803FC5C4;                           /* frames without the throttle after a hit */
/* the suit's */
extern Part D_803FC5D0[32];
extern VS D_803FC8D0;
extern s32 D_803FC978, D_803FC97C, D_803FC980;  /* x, y, z */
extern u8 *PTR32 D_803FC984;                    /* its model file */
extern u8 *PTR32 D_803FC988;                    /* two 0x1400-byte buffers, one per frame */
extern u8 *PTR32 D_803FC98C;
extern SndState *PTR32 D_803FC990;              /* its sound while it is in */
extern u16 D_803FC994;                          /* the heading it turns to against a wall (D_803A7425) */
extern s16 D_803FC996;                          /* the step sounds' last frame */
extern s8 D_803FC998;                           /* turning to it */
extern u8 D_803FC999;
extern u8 D_803FC99A;                           /* facing the way the wall allows (within SUIT_FACING) */

#define SK D_803FC200
#define SK_X D_803FC5A8
#define SK_Y D_803FC5AC
#define SK_Z D_803FC5B0
#define SK_MODEL D_803FC5B4
#define SK_BUF0 D_803FC5B8
#define SK_BUF1 D_803FC5BC
#define CS D_803FC5D0
#define CS_X D_803FC978
#define CS_Y D_803FC97C
#define CS_Z D_803FC980
#define CS_MODEL D_803FC984
#define CS_BUF0 D_803FC988
#define CS_BUF1 D_803FC98C

/* VehicleState's bytes the suit uses for itself (Thunderfist's MAGOO_*) */
#define SUIT_STATE(vs) ((vs)->unkA1)    /* SUIT_* below */
#define SUIT_LEG(vs) ((vs)->unkA2)      /* the leg in front: 0 part 1, 1 part 5, 2 part 6 */
#define SUIT_WALKING(vs) ((vs)->unkA3)  /* its walk's animation is on (else standing) */
#define SUIT_WALK 0                     /* walking */
#define SUIT_CURL 1                     /* curling into the ball (part 0x1F's animation) */
#define SUIT_ROLL 2                     /* rolling (part 2) */
#define SUIT_CRASH 3                    /* crashed into something rolling (part 0x1F) */
#define SUIT_GET_UP 4                   /* getting up (part 3) */

extern u8 D_80306460[], D_80306470[];           /* their parts' collision (56040's func_8029A800) */
extern u8 D_802C22D0[];                         /* the suit's light (56040's list) */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern u8 D_80370C35;                           /* 45BB0.c: the stick plays the buttons (A and B don't roll) */
extern u8 D_803F7804;                           /* it crashes through what it hits (77E20) */
extern SndState *PTR32 D_803F7844;              /* the rolling sound */
extern Part *PTR32 D_803F77D0;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

void func_802CFC10(void);
REGS(gp)
void func_802D02F8(VS *vs);
REGS(gp)
void func_802D0438(VS *vs);
REGS(gp)
void func_802D05D8(VS *vs);
REGS(gp -> s3)
s32 func_802D0710(VS *vs);
REGS()
void func_802D0784(void);
void func_802D0F54(void);
REGS(gp)
void func_802D1360(VS *vs);
REGS(gp)
void func_802D22F4(VS *vs);
REGS(gp -> s3)
s32 func_802D2444(VS *vs);
void func_802D249C(void);

#define T(p) ((s32)(p))

/* ---- the numbers (a frame, where it's per frame) ------------------------- */

#define STARSKI_SCALE 0x32C8            /* its model's scale */
#define STARSKI_SPAN_ALONG 0x1F4        /* its wheels' spans (func_802A8768) */
#define STARSKI_SPAN_ACROSS 0x15E
#define STARSKI_BRAKE 0x10              /* speed lost braking or against the stick, a frame */
#define STARSKI_TURN_RATE 0x1F40        /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define STARSKI_SLOPE_DIV 500.0f        /* func_802A843C: the slope's push is the height difference over this */
#define STARSKI_STEER_DIV 3.6f          /* the steering rate: the speed over this ... */
#define STARSKI_STEER_DIV_AIR 11.0f     /* ... or this with a wheel in the air */
#define STARSKI_WALL_TURN 0.16f         /* func_802A71DC: turning along a wall, times the speed */
#define STARSKI_HIT_FRAMES 5            /* frames without the throttle after hitting something */
#define STARSKI_HIT_MIN_SPEED 0x32      /* the speed it bounces back with is at least half this */
#define STARSKI_GRAVITY 4.0f            /* times the level's */
#define STARSKI_BOUNCE_MIN 0x3C         /* a landing harder than this bounces ... */
#define STARSKI_BOUNCE_DIV 3            /* ... at the speed over this */

#define SUIT_SCALE 0x2134               /* its model's scale */
#define SUIT_SPAN 0x78                  /* its feet's spans, both ways (func_802A8768) */
#define SUIT_BRAKE 0xC                  /* speed lost braking or against the stick, a frame */
#define SUIT_TURN_RATE 0x59D8           /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define SUIT_SLOPE_DIV 120.0f           /* func_802A843C: the slope's push is the height difference over this */
#define SUIT_STEER 0x6E                 /* the steering rate walking ... */
#define SUIT_STEER_ROLL 5               /* ... and in the ball or crashed (none standing) */
#define SUIT_WALL_TURN 0.25f            /* func_802A71DC: turning along a wall, times the speed */
#define SUIT_FACING 0x190               /* D_803FC99A: this near the way the wall allows */
#define SUIT_GRAVITY 4.0f               /* times the level's */
#define SUIT_BOUNCE_MIN 0x28            /* a landing harder than this bounces ... */
#define SUIT_BOUNCE_DIV 3               /* ... at the speed over this */
#define SUIT_ROLL_MIN_SPEED 0x96        /* it curls up only this fast */
#define SUIT_ROLL_SPEED 0x118           /* its speed rolling */
#define SUIT_UNCURL_SPEED 0x3C          /* its speed (going forward) getting out of the ball */
#define SUIT_STRIDE_5 0x3C              /* this fast, a step is part 5's ... */
#define SUIT_STRIDE_6 0x6E              /* ... this fast, part 6's (else part 1's) */
#define SUIT_LEG_SPEED_DIV 0x18         /* a leg's animation speed: the speed over this */
#define SUIT_IDLE_CHANCE 0x1E           /* standing, an idle 1 frame in 31 */

/* ---- the hotrod ---------------------------------------------------------- */

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802CF6A0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803FC500;
    s32 avg;

    ENGINE_COST(802CF6A0, 219);
    SK_MODEL = model;
    SK_BUF0 = D_80358070;
    SK_BUF1 = D_80358070 + 0x100;
    D_80358070 += 0x200;
    func_802A1388(VEHICLE_STARSKI, 0, SK_BUF0, SK_BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0xAF, 0xFA, -0xAF, 0xFA, 0xAF, -0xFA);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x140, 0x1F4, -0x140, 0x1F4, 0x140, -0x1F4);
    SK_X = x;
    SK_Y = y;
    SK_Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), SK_Y, x, z, VS_WHEEL_H(vs), &SK_Y, (s16 *)&VS_HEADING(vs), VEHICLE_STARSKI, vs, 0,
                  &avg);
    func_8029F85C(SK, SK_MODEL, SK_BUF0, SK_BUF1);
    func_802A039C(0, 100, SK);
    func_802A03D4(0, 0, SK);
    func_802A040C(0, 0, SK);
    func_802A0480(0, 0, SK, 0.0f);
    func_802A0290(0, 1, SK);
    func_8029E558(SK, SK_BUF0, SK_BUF1);
    func_802A0320(0, SK);
    func_802A0290(0, 1, SK);
    func_8029E558(SK, SK_BUF1, SK_BUF0);
    SET_GEARS(vs, -0xB4, 0, 2, 0, 0x118, 4, 0x118, 0x12C, 5, 0x12C, 0x136, 3, 0x136, 0x140, 2);
    D_803FC5C2 = 0;
    D_803FC5C3 = 0;
    D_803FC5C4 = 0;
    func_8029C354(VEHICLE_STARSKI, MODEL_AT(SK_MODEL, 4), MODEL_AT(SK_MODEL, 8), STARSKI_SCALE);
    func_80258230(VEHICLE_STARSKI, 0x3C, 0x19, 0x19);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802CFDE8();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(SK_BUF1, SK_BUF0, MODEL_MTX_OFF(SK_MODEL));
}

/* hd.c's: the player gets in */
void func_802CFA0C(void) {
    ENGINE_COST(802CFA0C, 19);
    VS_TURNING(&D_803FC500) = 0;
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xCE);
}

/* hd.c's: whether it can be left: not while a wheel is in the air */
u8 func_802CFA58(void) {
    ENGINE_COST(802CFA58, 23);
    return !ANY_AIRBORNE(&D_803FC500);
}

/* hd.c's: the player gets out */
void func_802CFAB4(void) {
    ENGINE_COST(802CFAB4, 19);
    VS_SPEED(&D_803FC500) = 0;
    func_802A7764((u32 *)SK_BUF0, (u32 *)SK_BUF1, 0x100);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802CFB00(void) {
    VS *vs = &D_803FC500;

    ENGINE_COST(802CFB00, 34);
    func_802A9A60(VS_WHEELS(vs), SK_Y, SK_X, SK_Z, VS_WHEEL_H(vs), &SK_Y, (s16 *)&VS_HEADING(vs), VEHICLE_STARSKI,
                  vs, 0);
    func_802D05D8(vs);
    func_802A133C(SK_X, SK_Y, SK_Z, VEHICLE_STARSKI, vs);
}

/* its light */
void func_802CFC10(void) {
    ENGINE_COST(802CFC10, 17);
    func_802ABD54(VEHICLE_STARSKI, SK_X, SK_Y, SK_Z);
}

/* hit something: back to where it was at the frame's start (and its
   vertices), the throttle off for STARSKI_HIT_FRAMES, the speed turned
   round and halved (at least STARSKI_HIT_MIN_SPEED before) */
static void starski_bounce(VS *vs) {
    s32 v;

    D_803FC5C3 = 0;
    func_802A768C((u8 *)SK, &SK_X, &SK_Y, &SK_Z, (u32 *)OTHER_BUF(SK_BUF0, SK_BUF1),
                  (u32 *)FRAME_BUF(SK_BUF0, SK_BUF1), 0x100, (u8 *)vs);
    D_803FC5C4 = STARSKI_HIT_FRAMES;
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < STARSKI_HIT_MIN_SPEED)
            v = STARSKI_HIT_MIN_SPEED;
    } else if (v > -STARSKI_HIT_MIN_SPEED) {
        v = -STARSKI_HIT_MIN_SPEED;
    }
    VS_SPEED(vs) = -v >> 1;
    func_802D05D8(vs);
}

/* each frame */
void func_802CFDE8(void) {
    VS *vs = &D_803FC500;
    s32 step = 0, x, z, rate_i;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802CFDE8, 213);
    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    func_802CFC10();
    func_802A75DC((u8 *)SK, &SK_X, &SK_Y, &SK_Z, (u8 *)vs);
    func_802C4724(0x76);
    if (VS_IN_SETUP(vs) == 0)
        func_802D02F8(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    func_802D0784();
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802D0710(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803FC5C4 == 0)
        func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), STARSKI_BRAKE, vs, &step);
    else
        D_803FC5C4--;
    func_802A7FD8(STARSKI_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs),
                  &VS_MOVE_HEADING(vs), (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_STARSKI, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), STARSKI_SLOPE_DIV, vs);
    if (D_803FC5C3 != 0)
        func_802A7070((s16 *)&D_803FC5C0, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &SK_X, &SK_Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &SK_X, &SK_Z, &SK_Y, VEHICLE_STARSKI, STARSKI_SPAN_ALONG, STARSKI_SPAN_ACROSS,
                  VS_WHEELS(vs), VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(SK, FRAME_BUF(SK_BUF0, SK_BUF1), OTHER_BUF(SK_BUF0, SK_BUF1));
    func_802D05D8(vs);
    /* what it hits */
    func_8029A800(SK_X, SK_Y, SK_Z, D_80306460, 1, 1, 7, VS_SPEED(vs), 0x96, 0, VEHICLE_STARSKI, vs);
    func_8029C52C(VEHICLE_STARSKI, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = SK;
        func_802BE77C(VEHICLE_STARSKI, vs);
        if (D_803A7424 == 0)
            D_803FC5C3 = 0;
        else
            starski_bounce(vs);
    } else {
        D_803FC5C3 = 1;
        turn_along_wall(vs, &D_803FC5C0, STARSKI_WALL_TURN);
        D_803F77D0 = SK;
        func_802BE77C(VEHICLE_STARSKI, vs);
    }
    PLAYER_FROM(SK_X, SK_Y, SK_Z, vs, VEHICLE_STARSKI);
}

/* the dust, sparks every other frame while turning (with room for them),
   and the engine's sound */
REGS(gp)
void func_802D02F8(VS *vs) {
    ENGINE_COST(802D02F8, 24);
    func_802D0438(vs);
    if (D_803FC5C2 != 0) {
        D_803FC5C2--;
    } else if (VS_TURNING(vs) != 0) {
        D_803FC5C2 = 1;
        if (func_802A5ED0() < 0xF) {
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_STARSKI, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_STARSKI, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x1D4C0, 1, VEHICLE_STARSKI, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x1D4C0, 1, VEHICLE_STARSKI, 4, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
        }
    }
    func_802C4584((u32)iabs(VS_SPEED(vs)) >> 5);
}

/* dust behind it while turning on soft ground (grip under 3, not the
   static triangles) with its back wheel down */
REGS(gp)
void func_802D0438(VS *vs) {
    ENGINE_COST(802D0438, 69);
    if (VS_TURNING(vs) != 0 && VS_AIRBORNE(vs)[2] != 1 && VS_GRIP(vs) < 3 && VS_ON_STATIC(vs) == 0)
        func_8027BE7C(3, VS_WHEEL_H(vs)[6], 0xFA, -0x190, -0x190, -0x190, SK_X, SK_Z, VS_MOVE_HEADING(vs), 3, 0x32,
                      0x32, 0);
}

/* the hotrod's matrix, its vertices and its collision */
REGS(gp)
void func_802D05D8(VS *vs) {
    u8 *model = SK_MODEL, *buf = FRAME_BUF(SK_BUF0, SK_BUF1);

    ENGINE_COST(802D05D8, 70);
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(SK_X, SK_Y, SK_Z, STARSKI_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(SK_X, SK_Y, SK_Z, VEHICLE_STARSKI, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_STARSKI, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over STARSKI_STEER_DIV, or over
   STARSKI_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802D0710(VS *vs) {
    ENGINE_COST(802D0710, 24);
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? STARSKI_STEER_DIV_AIR : STARSKI_STEER_DIV));
}

/* the physics' settings for the hotrod: gravity, and how its wheels land */
REGS()
void func_802D0784(void) {
    ENGINE_COST(802D0784, 23);
    D_803EBBF4 = D_803EBBF0 * STARSKI_GRAVITY;
    D_803ED3F6 = STARSKI_BOUNCE_MIN;
    D_803ED3F7 = STARSKI_BOUNCE_DIV;
}

/* ---- the suit ----------------------------------------------------------- */

/* part i's frame (func_802A04BC's v1), and its a0 (unk11) and t1 (unk13) */
static s32 part(s32 i, s32 *a0, s32 *t1) {
    s32 f12, f14, fC, fE, a0_, t1_;
    f32 f4;
    s32 r = func_802A04BC(i, CS, &a0_, &f12, &f14, &fC, &fE, &t1_, &f4);

    if (a0)
        *a0 = a0_;
    if (t1)
        *t1 = t1_;
    return r;
}

/* part 0x1F (the body) on to animation `anim` */
static void body_anim(s32 anim) {
    func_802A039C(0x1F, anim, CS);
    func_802A03D4(0x1F, 0, CS);
    func_802A040C(0x1F, 0, CS);
    func_802A0290(0x1F, 1, CS);
}

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802D07E0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803FC8D0;
    s32 avg;

    ENGINE_COST(802D07E0, 236);
    CS_MODEL = model;
    CS_BUF0 = D_80358070;
    CS_BUF1 = D_80358070 + 0x1400;
    D_80358070 += 0x2800;
    func_802A1388(VEHICLE_MINIMAGOO, 1, CS_BUF0, CS_BUF1, model);
    func_802A754C(vs);
    /* one point (its feet), and three for what carries it */
    SET_WHEELS(VS_WHEELS(vs), 0, 0, 0, 0, 0, 0);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x50, 0x50, -0x50, 0x50, 0x50, -0x50);
    CS_X = x;
    CS_Y = y;
    CS_Z = z;
    VS_HEADING(vs) = heading;
    SUIT_STATE(vs) = SUIT_WALK;
    SUIT_LEG(vs) = 0;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    SUIT_WALKING(vs) = 1;
    func_802A992C(VS_WHEELS(vs), CS_Y, x, z, VS_WHEEL_H(vs), &CS_Y, (s16 *)&VS_HEADING(vs), VEHICLE_MINIMAGOO, vs, 0,
                  &avg);
    func_8029F85C(CS, CS_MODEL, CS_BUF0, CS_BUF1);
    func_802A039C(0, 100, CS);
    func_802A03D4(0, 0, CS);
    func_802A040C(0, 0, CS);
    func_802A0480(0, 0, CS, 0.0f);
    func_802A0290(0, 1, CS);
    func_8029E558(CS, CS_BUF0, CS_BUF1);
    func_802A0320(0, CS);
    func_802A0290(0, 1, CS);
    func_8029E558(CS, CS_BUF1, CS_BUF0);
    SET_GEARS(vs, -0x64, 0, 4, 0, 0xA0, 4, 0, 0xA0, 4, 0, 0xA0, 4, 0, 0xA0, 4);
    D_803FC998 = 0;
    D_803FC99A = 0;
    D_803FC999 = 0;
    func_8029C354(VEHICLE_MINIMAGOO, MODEL_AT(model, 4), MODEL_AT(model, 8), SUIT_SCALE);
    func_80258230(VEHICLE_MINIMAGOO, 0x64, 0x2D, 0x2D);
    func_802A0360(1, 0, CS, 0.0f);
    func_802A0290(1, 1, CS);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802D0F98();
    VS_IN_SETUP(vs) = 0;
    func_802A7764((u32 *)CS_BUF1, (u32 *)CS_BUF0, 0x1400);
    func_802AA838(CS_BUF1, CS_BUF0, MODEL_MTX_OFF(CS_MODEL));
    D_803F7844 = NULL;
}

/* hd.c's: whether it can be left: on the ground and walking */
u8 func_802D0B90(void) {
    VS *vs = &D_803FC8D0;

    ENGINE_COST(802D0B90, 26);
    return !ANY_AIRBORNE(vs) && SUIT_STATE(vs) == SUIT_WALK;
}

/* hd.c's: the player gets out */
void func_802D0BF8(void) {
    ENGINE_COST(802D0BF8, 14);
    VS_SPEED(&D_803FC8D0) = 0;
    func_802A7764((u32 *)CS_BUF0, (u32 *)CS_BUF1, 0x1400);
    func_802A02E4(0x1F, CS);
    func_802C444C();
    func_802608C8(D_803FC990);
}

/* hd.c's (and 17210.c's): the player gets in: its sound, its light and its
   arms */
void func_802D0C68(void) {
    ENGINE_COST(802D0C68, 119);
    VS_TURNING(&D_803FC8D0) = 0;
    D_8036444C = 0x7D0;
    D_80364450 = -0x3E8;
    func_80260650(D_80367738, 0x50, &D_803FC990);
    func_802A05D0(D_802C22D0, 0x64);
    func_802A05F8(D_802C22D0, 0);
    func_802A0620(D_802C22D0, 0);
    func_802A0508(D_802C22D0, -1);
    func_802A039C(7, 2, CS);
    func_802A040C(7, 1, CS);
    func_802A0480(7, 1, CS, 0.5f);
    func_802A039C(8, 2, CS);
    func_802A040C(8, 1, CS);
    func_802A0480(8, 1, CS, 0.5f);
    func_802A039C(9, 4, CS);
    func_802A040C(9, 1, CS);
    func_802A0480(9, 1, CS, 0.5f);
    func_802A0480(1, 1, CS, 0.5f);
    func_802A0480(5, 1, CS, 0.5f);
    func_802A0480(6, 1, CS, 0.5f);
    func_802A0480(3, 0, CS, 0.0f);
}

/* put back on the ground where it is (Thunderfist's func_802B13D8; nothing
   calls it) */
void func_802D0E44(void) {
    VS *vs = &D_803FC8D0;

    ENGINE_COST(802D0E44, 34);
    func_802A9A60(VS_WHEELS(vs), CS_Y, CS_X, CS_Z, VS_WHEEL_H(vs), &CS_Y, (s16 *)&VS_HEADING(vs), VEHICLE_MINIMAGOO,
                  vs, 0);
    func_802D22F4(vs);
    func_802A133C(CS_X, CS_Y, CS_Z, VEHICLE_MINIMAGOO, vs);
}

/* its light */
void func_802D0F54(void) {
    ENGINE_COST(802D0F54, 17);
    func_802ABD54(VEHICLE_MINIMAGOO, CS_X, CS_Y, CS_Z);
}

/* each frame */
void func_802D0F98(void) {
    VS *vs = &D_803FC8D0;
    s32 step = 0, x, z, rate_i, turn, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802D0F98, 188);
    /* (its $s4 and $fp as it found them: 5CB60.c and the other vehicles read them from the context) */
    func_802D0F54();
    if (VS_IN_SETUP(vs) == 0)
        func_802D1360(vs);
    func_802D249C();
    /* steering, the throttle (the stick let go: stopping, walking), the
       turn and the slope */
    rate_i = func_802D2444(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), SUIT_BRAKE, vs, &step);
    if (SUIT_STATE(vs) == SUIT_WALK)
        func_802A77D0(vs);
    func_802A7FD8(SUIT_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 0, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 0, VEHICLE_MINIMAGOO, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), SUIT_SLOPE_DIV, vs);
    if (D_803FC998 != 0)
        func_802A7070((s16 *)&D_803FC994, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &CS_X, &CS_Z, rate, &z);
    D_803ED40B = 0;
    func_802A8768(x, z, &CS_X, &CS_Z, &CS_Y, VEHICLE_MINIMAGOO, SUIT_SPAN, SUIT_SPAN, VS_WHEELS(vs),
                  VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(CS, FRAME_BUF(CS_BUF0, CS_BUF1), OTHER_BUF(CS_BUF0, CS_BUF1));
    func_802D22F4(vs);
    /* what it hits */
    func_8029A800(CS_X, CS_Y, CS_Z, D_80306470, 0, 0, 0, VS_SPEED(vs), 0, 0, VEHICLE_MINIMAGOO, vs);
    func_8029C52C(VEHICLE_MINIMAGOO, vs);
    func_8029AA10();
    D_803F77D0 = CS;
    func_802BE77C(VEHICLE_MINIMAGOO, vs);
    if (D_803A7425 == 0) {
        D_803FC998 = 0;
        D_803FC99A = 0;
    } else {
        /* against a wall: whether it faces the way the wall allows, then
           turned along it (turn_along_wall's steps, the check between) */
        func_8029A914(vs);
        D_803FC998 = 1;
        h = (u16)VS_MOVE_HEADING(vs) - ANGLE_HALF;
        if (h < 0)
            h += ANGLE_WRAP;
        h = iabs(h - func_802A6F6C());
        if (h > ANGLE_HALF)
            h = ANGLE_WRAP - h;
        D_803FC99A = h < SUIT_FACING;
        func_802A70D8(vs);
        D_803FC994 = func_802A71DC(VS_MOVE_HEADING(vs), VS_HEADING(vs), SUIT_WALL_TURN, vs, &turn);
        VS_MOVE_HEADING(vs) = D_803FC994;
        VS_TURN_HEADING(vs) = D_803FC994;
        func_802A746C(turn, vs);
        func_802A6FE4(0, vs);
    }
    PLAYER_FROM(CS_X, CS_Y, CS_Z, vs, VEHICLE_MINIMAGOO);
}

/* the first of the n parts that is animating handed over to idle `to`
   (stopped and `to`'s frame set from it, in the original's order of the
   two), or, none animating, the first started at frame 0 and handed over */
static void hand_over(const s32 *parts, s32 n, s32 to, s32 stop_first) {
    s32 k;

    for (k = 0; k < n; k++) {
        if (part(parts[k], NULL, NULL) != 0) {
            if (stop_first) {
                func_802A02E4(parts[k], CS);
                func_8029F9D4(parts[k], to, CS);
            } else {
                func_8029F9D4(parts[k], to, CS);
                func_802A02E4(parts[k], CS);
            }
            return;
        }
    }
    func_802A0360(parts[0], 0, CS, 0.0f);
    func_8029F9D4(parts[0], to, CS);
}

/* standing: once, the walk's animation (the body's, then the legs')
   handed over to idle 7; then now and then (1 in SUIT_IDLE_CHANCE + 1) a
   random idle (7, 8 or 9) when none plays */
static void stand(VS *vs) {
    static const s32 legs[3] = { 1, 5, 6 };
    s32 r;

    if (SUIT_WALKING(vs) != 0) {
        func_802A0360(7, 0, CS, 0.0f);
        if (part(0x1F, NULL, NULL) != 0) {
            func_802A02E4(0x1F, CS);
            func_8029F9D4(0x1F, 7, CS);
        } else {
            hand_over(legs, 3, 7, 1);
        }
        body_anim(0x1E);
    }
    SUIT_WALKING(vs) = 0;
    if (part(0x1F, NULL, NULL) == 1 || part(7, NULL, NULL) == 1 || part(8, NULL, NULL) == 1 ||
        part(9, NULL, NULL) == 1)
        return;
    if (func_8026A8E0(0, SUIT_IDLE_CHANCE) != 0)
        return;
    r = func_8026A8E0(0, 2);
    r = r == 0 ? 7 : r == 1 ? 8 : 9;
    func_802A0360(r, 0, CS, 0.0f);
    func_802A0290(r, 1, CS);
}

/* Walking: once, the idles handed over to the walk; then, the body's
   animation done, curling into the ball (L or R, or A or B unless
   D_80370C35, at SUIT_ROLL_MIN_SPEED or more), or the step of the leg in
   front at its speed; its step done, the next one: part 6's at
   SUIT_STRIDE_6 or more, part 5's at SUIT_STRIDE_5 or more, else part
   1's. */
static void walk(VS *vs) {
    static const s32 idles[3] = { 7, 8, 9 };
    static const s32 leg_part[3] = { 1, 5, 6 };
    s32 a0, leg, s;

    if (SUIT_WALKING(vs) != 1) {
        func_802A0360(1, 0, CS, 0.0f);
        hand_over(idles, 3, 1, 0);
        body_anim(0x28);
    }
    SUIT_WALKING(vs) = 1;
    if (part(0x1F, NULL, NULL) == 1)
        return;
    D_803F7804 = 0;
    if (((D_80370C35 == 0 && (PAD_A != 0 || PAD_B != 0)) || PAD_L != 0 || PAD_R != 0) &&
        VS_SPEED(vs) >= SUIT_ROLL_MIN_SPEED) {
        /* curling up */
        SUIT_STATE(vs) = SUIT_CURL;
        func_802A0360(2, 0, CS, 0.0f);
        if (SUIT_LEG(vs) > 2)
            engine_trap(0x802D1984);
        leg = leg_part[SUIT_LEG(vs)];
        func_8029F9D4(leg, 2, CS);
        func_802A02E4(leg, CS);
        body_anim(0x50);
        func_80278EB0(6, 0.1f, 100);
        return;
    }
    if (SUIT_LEG(vs) > 2)
        engine_trap(0x802D1A94);
    for (leg = leg_part[SUIT_LEG(vs)];;) {
        if (part(leg, &a0, NULL) != 0) {
            s = VS_SPEED(vs);
            func_802A03D4(leg, s < 0 ? 1 : 0, CS);
            func_802A039C(leg, (u32)iabs(s) / SUIT_LEG_SPEED_DIV, CS);
            return;
        }
        s = iabs(VS_SPEED(vs));
        SUIT_LEG(vs) = s < SUIT_STRIDE_5 ? 0 : s < SUIT_STRIDE_6 ? 1 : 2;
        leg = leg_part[SUIT_LEG(vs)];
        func_802A0290(leg, a0 != 0 ? 2 : 1, CS);
        func_802A0360(leg, 0, CS, 0.0f);
    }
}

/* getting out of the ball (part `ball` stopped, or the body's from the
   curl): forward at SUIT_UNCURL_SPEED, the body's animation 0x32, walking
   again */
static void uncurl(VS *vs, s32 ball) {
    if (ball >= 0)
        func_802C444C();
    if (VS_SPEED(vs) >= 0)
        VS_SPEED(vs) = SUIT_UNCURL_SPEED;
    if (ball < 0)
        func_802C444C();
    func_802A0360(1, 0, CS, 0.0f);
    if (ball >= 0) {
        func_802A02E4(ball, CS);
        func_8029F9D4(ball, 1, CS);
    } else {
        func_8029F9D4(0x1F, 1, CS);
    }
    body_anim(0x32);
    func_802794A4();
    SUIT_STATE(vs) = SUIT_WALK;
}

/* crashed rolling into something (VS unk9C): the speed halved, the
   crash's animation, through what it hits (D_803F7804) */
static void crash(VS *vs, s32 ball) {
    VS_SPEED(vs) = VS_SPEED(vs) >> 1;
    if (ball < 0)
        func_802794A4();
    SUIT_STATE(vs) = SUIT_CRASH;
    func_802A0360(3, 0, CS, 0.0f);
    if (ball < 0) {
        func_8029F9D4(0x1F, 3, CS);
    } else {
        func_8029F9D4(ball, 3, CS);
        func_802A02E4(ball, CS);
    }
    body_anim(0x21);
    D_803F7804 = 1;
}

/* The parts and the states a frame.  Walking: the step sounds (the legs'
   frames 2 and 6), then standing or walking.  Curling and rolling: at
   SUIT_ROLL_SPEED, crashing into what it hits (unk9C), uncurling on unk9D
   or facing along a wall; curling done, rolling.  Crashed: the sound;
   its animation done, getting up; that done, walking. */
REGS(gp)
void func_802D1360(VS *vs) {
    s32 t1, last;

    ENGINE_COST(802D1360, 50);
    switch (SUIT_STATE(vs)) {
    case SUIT_WALK:
        if (part(1, NULL, &t1) == 1 || part(5, NULL, &t1) == 1 || part(6, NULL, &t1) == 1) {
            last = D_803FC996;
            D_803FC996 = t1;
            if (t1 != last) {
                if (t1 == 6)
                    func_80260650(D_80367738, 0x4E, NULL);
                else if (t1 == 2)
                    func_80260650(D_80367738, 0x4F, NULL);
            }
        }
        if (VS_SPEED(vs) == 0)
            stand(vs);
        else
            walk(vs);
        break;
    case SUIT_CURL:
        if (D_803F7844 == NULL)
            func_80260650(D_80367738, 0x51, &D_803F7844);
        if (vs->unk9C != 0) {
            crash(vs, -1);
        } else if (vs->unk9D != 0 || D_803FC99A != 0) {
            uncurl(vs, -1);
        } else {
            VS_SPEED(vs) = SUIT_ROLL_SPEED;
            if (part(0x1F, NULL, NULL) != 1) {
                func_802A039C(2, 0xA, CS);
                func_802A03D4(2, 0, CS);
                func_802A040C(2, 0, CS);
                func_802A0290(2, 1, CS);
                SUIT_STATE(vs) = SUIT_ROLL;
            }
        }
        break;
    case SUIT_ROLL:
        if (vs->unk9C != 0) {
            crash(vs, 2);
        } else if (vs->unk9D != 0 || D_803FC99A != 0) {
            uncurl(vs, 2);
        } else {
            VS_SPEED(vs) = SUIT_ROLL_SPEED;
            if (part(2, NULL, NULL) != 1)
                uncurl(vs, 2);
        }
        break;
    case SUIT_CRASH:
        if (D_803F7844 != NULL) {
            func_802C444C();
            func_80260650(D_80367738, 0x4B, NULL);
        }
        func_802BCC10();
        if (part(0x1F, NULL, NULL) != 1) {
            D_803F7804 = 0;
            func_802794A4();
            func_802A039C(3, 5, CS);
            func_802A03D4(3, 0, CS);
            func_802A040C(3, 0, CS);
            func_802A0290(3, 1, CS);
            SUIT_STATE(vs) = SUIT_GET_UP;
        }
        break;
    case SUIT_GET_UP:
        func_802BCC10();
        if (part(3, NULL, NULL) != 1) {
            D_803F7804 = 1;
            func_802A0360(1, 0, CS, 0.0f);
            func_802A0360(5, 0, CS, 0.0f);
            func_802A0360(6, 0, CS, 0.0f);
            SUIT_STATE(vs) = SUIT_WALK;
        }
        break;
    default:
        engine_trap(0x802D1398);
        break;
    }
}

/* the suit's matrix (turned a quarter), its vertices and its collision */
REGS(gp)
void func_802D22F4(VS *vs) {
    u8 *model = CS_MODEL, *buf = FRAME_BUF(CS_BUF0, CS_BUF1);
    s32 h = (u16)VS_HEADING(vs) + ANGLE_QUARTER;

    ENGINE_COST(802D22F4, 75);
    if (h >= ANGLE_TURN)
        h -= ANGLE_WRAP;
    D_803ED390[1] = h;
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    func_802AA764(CS_X, CS_Y, CS_Z, SUIT_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(CS_X, CS_Y, CS_Z, VEHICLE_MINIMAGOO, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_MINIMAGOO, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: none standing, SUIT_STEER_ROLL out of the walk, else
   SUIT_STEER */
REGS(gp -> s3)
s32 func_802D2444(VS *vs) {
    ENGINE_COST(802D2444, 18);
    if (VS_SPEED(vs) == 0)
        return 0;
    return SUIT_STATE(vs) >= SUIT_CURL && SUIT_STATE(vs) <= SUIT_GET_UP ? SUIT_STEER_ROLL : SUIT_STEER;
}

/* the physics' settings for the suit: gravity, and how it lands */
void func_802D249C(void) {
    ENGINE_COST(802D249C, 23);
    D_803EBBF4 = D_803EBBF0 * SUIT_GRAVITY;
    D_803ED3F6 = SUIT_BOUNCE_MIN;
    D_803ED3F7 = SUIT_BOUNCE_DIV;
}

/* its state and position saved to dst (0xB2 bytes; Thunderfist's
   func_802B295C, but nothing calls it) */
void func_802D24F8(u8 *dst) {
    ENGINE_COST(802D24F8, 6);
    func_802AC7DC(dst, (u8 *)&D_803FC8D0, (u32 *)&D_803FC978);
}

/* and back */
void func_802D2524(u8 *src) {
    ENGINE_COST(802D2524, 6);
    func_802AC85C(src, (u8 *)&D_803FC8D0, (u32 *)&D_803FC978);
}

/* an mtc0 of its argument to COP0's Compare, which the port has no timer
   interrupt for (the translation's recomp_mtc0 does nothing either); nothing
   calls it */
void func_802D2550(u32 compare) {
    (void)compare;
    ENGINE_COST(802D2550, 4);
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802CFC54(s32 carrier) {
    CARRY_KEEP(802CFC54, 802CFC90, 802CFCB0, 802CFCBC, &D_803FC500, D_803FC5A8, D_803FC5B0);
}

REGS(a3)
void func_802CFCE0(s32 carrier) {
    CARRY_MOVE(802CFCE0, 802CFD1C, 802CFD30, 802CFD38, 802CFD98, 802CFDA0, 802CFDCC, &D_803FC500, &D_803FC5A8,
               &D_803FC5AC, &D_803FC5B0, 0xF, 0x1F4, 0x15E, D_803ED40B = 1, func_802D0784(),
               func_802D05D8(&D_803FC500));
}
