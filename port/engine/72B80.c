/*
 * hd_code 72B80 (us.v11 0x802B7340-0x802B9C50): the American Dream
 * (VEHICLE_HOTROD) and the BCT chopper (type 0xFE), as native C
 * (engine.h).
 *
 * The hotrod's parts are D_803EEB70, its state D_803EEE70 and its position
 * D_803EEF18..20 (vehicle.h).  func_802B7340 sets it up (from the level
 * loader), func_802B7A88 runs it each frame (from hd.c and at the end of
 * the setup); the others are hd.c's hooks for it.  It is the police car's
 * code (86F60) with its own numbers, and dust and sparks.
 *
 * The chopper's parts are D_803EEF40, its state D_803EF240 and its position
 * D_803EF2EC..F4: func_802B8480 sets it up (from the level loader),
 * func_802B899C flies it each frame (from hd.c), func_802B8794 is its
 * sound and func_802B8AE4 hands its position to the player when it has
 * landed.  func_802B8D04 is its flight, a state in D_803EF32C (CHOPPER_*).
 *
 * func_802B78F4 and func_802B7980, at the end, are the hotrod's two
 * callbacks for 62740's carrying (shared.h).
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the hotrod's .bss (asm/data/hd_code/72B80.bss.s) */
extern Part D_803EEB70[32];
extern VS D_803EEE70;
extern s32 D_803EEF18, D_803EEF1C, D_803EEF20;  /* x, y, z */
extern u8 *PTR32 D_803EEF24;                    /* its model file */
extern u8 *PTR32 D_803EEF28;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803EEF2C;
extern u16 D_803EEF30;                          /* the heading it turns to against a wall (D_803A7425) */
extern u8 D_803EEF32;                           /* frames until the next sparks */
extern s8 D_803EEF33;                           /* turning to it */
extern u8 D_803EEF34;                           /* frames without the throttle after a hit */
/* the chopper's */
extern Part D_803EEF40[32];
extern VS D_803EF240;
extern s16 D_803EF2E6;                          /* the speed its sound was last set for */
extern SndState *PTR32 D_803EF2E8;              /* its rotor's sound, while the player is near */
extern s32 D_803EF2EC, D_803EF2F0, D_803EF2F4;  /* x, y, z */
extern u8 *PTR32 D_803EF2F8;                    /* its model file */
extern u8 *PTR32 D_803EF2FC;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EF300;
extern s32 D_803EF304;                          /* the height it flies at */
extern s32 D_803EF308, D_803EF30C;              /* where it flies to: x, z */
extern s32 D_803EF310, D_803EF314, D_803EF318;  /* where the player gets out */
extern s32 D_803EF31C;                          /* the ground's height under it */
extern s32 D_803EF320;                          /* the distance to go, last frame */
extern s16 D_803EF324;                          /* its turn a frame (4 hovering) */
extern s16 D_803EF326;
extern s16 D_803EF328, D_803EF32A;              /* its speed and heading when it landed */
extern u8 D_803EF32C;                           /* its state (CHOPPER_*) */
extern u8 D_803EF32D;                           /* it has landed */
extern u8 D_803EF32E;                           /* the rotor has stopped */

#define P D_803EEB70
#define Q D_803EEF40
#define HR_X D_803EEF18
#define HR_Y D_803EEF1C
#define HR_Z D_803EEF20
#define HR_MODEL D_803EEF24
#define HR_BUF0 D_803EEF28
#define HR_BUF1 D_803EEF2C
#define CH_X D_803EF2EC
#define CH_Y D_803EF2F0
#define CH_Z D_803EF2F4
#define CH_MODEL D_803EF2F8
#define CH_BUF0 D_803EF2FC
#define CH_BUF1 D_803EF300

extern u8 D_80305D30[];                         /* the hotrod's parts' collision (56040's func_8029A800) */
extern char D_80305D40[];                       /* "moving to zoom2\n" */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern Part *PTR32 D_803F77D0;
extern s32 D_80368030, D_80368044, D_80368048;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

/* 60D50.c's */
REGS(a0)
void func_802A5604(LevelHeader *level);
/* 62740's */
REGS(v0, t0, t1, t2, t8, gp, fp -> t3)
s32 func_802A9B1C(s32 i, s32 x, s32 z, s32 y, s32 self, VS *vs, s32 mat);

void func_802B78B0(void);
REGS(gp)
void func_802B7F98(VS *vs);
REGS(gp)
void func_802B80D8(VS *vs);
REGS(gp)
void func_802B8278(VS *vs);
REGS(gp -> s3)
s32 func_802B83B0(VS *vs);
REGS()
void func_802B8424(void);
static void chopper_frame(void);
REGS(gp)
void func_802B8C18(VS *vs);
REGS()
void func_802B8D04(void);
REGS(-> s1+f0)
s64 func_802B988C(void);
REGS(gp)
void func_802B98E0(VS *vs);
REGS(gp)
void func_802B9B4C(VS *vs);

#define T(p) ((s32)(p))

static s32 f2i(f32 f) {
    union {
        f32 f;
        s32 i;
    } u;

    u.f = f;
    return u.i;
}

/* part i's frame (func_802A04BC's v1), and its f0 */
static s32 part(s32 i, Part *parts, f32 *f0) {
    s32 f11, f12, f14, fC, fE, f13;
    f32 f4;
    s32 r = func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &f13, &f4);

    if (f0)
        *f0 = f4;
    return r;
}

/* ---- the hotrod ---------------------------------------------------------- */

/* the hotrod's numbers (a frame, where it's per frame) */
#define HOTROD_SCALE 0x32C8             /* its model's scale */
#define HOTROD_SPAN_ALONG 0x2BC         /* its wheels' spans (func_802A8768) */
#define HOTROD_SPAN_ACROSS 0x190
#define HOTROD_BRAKE 0x10               /* speed lost braking or against the stick, a frame */
#define HOTROD_TURN_RATE 0x1770         /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define HOTROD_SLOPE_DIV 700.0f         /* func_802A843C: the slope's push is the height difference over this */
#define HOTROD_STEER_DIV 3.6f           /* the steering rate: the speed over this ... */
#define HOTROD_STEER_DIV_AIR 11.0f      /* ... or this with a wheel in the air */
#define HOTROD_WALL_TURN 0.16f          /* func_802A71DC: turning along a wall, times the speed */
#define HOTROD_HIT_FRAMES 5             /* frames without the throttle after hitting something */
#define HOTROD_HIT_MIN_SPEED 0x32       /* the speed it bounces back with is at least half this */
#define HOTROD_GRAVITY 4.0f             /* times the level's */
#define HOTROD_BOUNCE_MIN 0x3C          /* a landing harder than this bounces ... */
#define HOTROD_BOUNCE_DIV 3             /* ... at the speed over this */

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802B7340(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EEE70;
    s32 avg;

    ENGINE_COST(802B7340, 219);
    HR_MODEL = model;
    HR_BUF0 = D_80358070;
    HR_BUF1 = D_80358070 + 0x100;
    D_80358070 += 0x200;
    func_802A1388(VEHICLE_HOTROD, 0, HR_BUF0, HR_BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0xC8, 0x15E, -0xC8, 0x15E, 0xC8, -0x15E);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x140, 0x1F4, -0x140, 0x1F4, 0x140, -0x1F4);
    HR_X = x;
    HR_Y = y;
    HR_Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), HR_Y, x, z, VS_WHEEL_H(vs), &HR_Y, (s16 *)&VS_HEADING(vs), VEHICLE_HOTROD, vs, 0,
                  &avg);
    func_8029F85C(P, HR_MODEL, HR_BUF0, HR_BUF1);
    func_802A039C(0, 100, P);
    func_802A03D4(0, 0, P);
    func_802A040C(0, 0, P);
    func_802A0480(0, 0, P, 0.0f);
    func_802A0290(0, 1, P);
    func_8029E558(P, HR_BUF0, HR_BUF1);
    func_802A0320(0, P);
    func_802A0290(0, 1, P);
    func_8029E558(P, HR_BUF1, HR_BUF0);
    SET_GEARS(vs, -0xB4, 0, 2, 0, 0x50, 4, 0x50, 0xA0, 5, 0xA0, 0xFA, 3, 0xFA, 0x15E, 2);
    D_803EEF32 = 0;
    D_803EEF33 = 0;
    D_803EEF34 = 0;
    func_8029C354(VEHICLE_HOTROD, MODEL_AT(HR_MODEL, 4), MODEL_AT(HR_MODEL, 8), HOTROD_SCALE);
    func_80258230(VEHICLE_HOTROD, 0x3C, 0x19, 0x19);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802B7A88();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(HR_BUF1, HR_BUF0, MODEL_MTX_OFF(HR_MODEL));
}

/* hd.c's: the player gets in */
void func_802B76AC(void) {
    ENGINE_COST(802B76AC, 19);
    VS_TURNING(&D_803EEE70) = 0;
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xCE);
}

/* hd.c's: whether it can be left: not while a wheel is in the air */
u8 func_802B76F8(void) {
    ENGINE_COST(802B76F8, 23);
    return !ANY_AIRBORNE(&D_803EEE70);
}

/* hd.c's: the player gets out */
void func_802B7754(void) {
    ENGINE_COST(802B7754, 19);
    VS_SPEED(&D_803EEE70) = 0;
    func_802A7764((u32 *)HR_BUF0, (u32 *)HR_BUF1, 0x100);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802B77A0(void) {
    VS *vs = &D_803EEE70;

    ENGINE_COST(802B77A0, 35);
    func_802A9A60(VS_WHEELS(vs), HR_Y, HR_X, HR_Z, VS_WHEEL_H(vs), &HR_Y, (s16 *)&VS_HEADING(vs), VEHICLE_HOTROD, vs,
                  0);
    func_802B8278(vs);
    func_802A133C(HR_X, HR_Y, HR_Z, VEHICLE_HOTROD, vs);
}

/* its light */
void func_802B78B0(void) {
    ENGINE_COST(802B78B0, 17);
    func_802ABD54(VEHICLE_HOTROD, HR_X, HR_Y, HR_Z);
}

/* hit something: back to where it was at the frame's start (and its
   vertices), the throttle off for HOTROD_HIT_FRAMES, the speed turned
   round and halved (at least HOTROD_HIT_MIN_SPEED before) */
static void hotrod_bounce(VS *vs) {
    s32 v;

    D_803EEF33 = 0;
    func_802A768C((u8 *)P, &HR_X, &HR_Y, &HR_Z, (u32 *)OTHER_BUF(HR_BUF0, HR_BUF1), (u32 *)FRAME_BUF(HR_BUF0, HR_BUF1),
                  0x100, (u8 *)vs);
    D_803EEF34 = HOTROD_HIT_FRAMES;
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < HOTROD_HIT_MIN_SPEED)
            v = HOTROD_HIT_MIN_SPEED;
    } else if (v > -HOTROD_HIT_MIN_SPEED) {
        v = -HOTROD_HIT_MIN_SPEED;
    }
    VS_SPEED(vs) = -v >> 1;
    func_802B8278(vs);
}

/* each frame */
void func_802B7A88(void) {
    VS *vs = &D_803EEE70;
    s32 step = 0, x, z, rate_i;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802B7A88, 213);
    func_802B78B0();
    func_802A75DC((u8 *)P, &HR_X, &HR_Y, &HR_Z, (u8 *)vs);
    func_802C4724(0xA4);
    if (VS_IN_SETUP(vs) == 0)
        func_802B7F98(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    func_802B8424();
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802B83B0(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803EEF34 == 0)
        func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), HOTROD_BRAKE, vs, &step);
    else
        D_803EEF34--;
    func_802A7FD8(HOTROD_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_HOTROD, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), HOTROD_SLOPE_DIV, vs);
    if (D_803EEF33 != 0)
        func_802A7070((s16 *)&D_803EEF30, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &HR_X, &HR_Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &HR_X, &HR_Z, &HR_Y, VEHICLE_HOTROD, HOTROD_SPAN_ALONG, HOTROD_SPAN_ACROSS, VS_WHEELS(vs),
                  VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(P, FRAME_BUF(HR_BUF0, HR_BUF1), OTHER_BUF(HR_BUF0, HR_BUF1));
    func_802B8278(vs);
    /* what it hits */
    func_8029A800(D_803EEF18, D_803EEF1C, D_803EEF20, D_80305D30, 1, 1, 7, vs->unk76, 0x96, 0, 8, vs);
    func_8029C52C(VEHICLE_HOTROD, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = P;
        func_802BE77C(VEHICLE_HOTROD, vs);
        if (D_803A7424 == 0)
            D_803EEF33 = 0;
        else
            hotrod_bounce(vs);
    } else {
        D_803EEF33 = 1;
        turn_along_wall(vs, &D_803EEF30, HOTROD_WALL_TURN);
        D_803F77D0 = P;
        func_802BE77C(VEHICLE_HOTROD, vs);
    }
    PLAYER_FROM(HR_X, HR_Y, HR_Z, vs, VEHICLE_HOTROD);
}

/* the dust, sparks every other frame while turning (with room for them),
   and the engine's sound */
REGS(gp)
void func_802B7F98(VS *vs) {
    ENGINE_COST(802B7F98, 23);
    func_802B80D8(vs);
    if (D_803EEF32 != 0) {
        D_803EEF32--;
    } else if (VS_TURNING(vs) != 0) {
        D_803EEF32 = 1;
        if (func_802A5ED0() < 0xF) {
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_HOTROD, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_HOTROD, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x1D4C0, 1, VEHICLE_HOTROD, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x1D4C0, 1, VEHICLE_HOTROD, 4, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
        }
    }
    func_802C4584((u32)iabs(VS_SPEED(vs)) >> 5);
}

/* dust behind it while turning on soft ground (grip under 3, not the
   static triangles) with its back wheel down */
REGS(gp)
void func_802B80D8(VS *vs) {
    ENGINE_COST(802B80D8, 69);
    if (VS_TURNING(vs) != 0 && VS_AIRBORNE(vs)[2] != 1 && VS_GRIP(vs) < 3 && VS_ON_STATIC(vs) == 0)
        func_8027BE7C(3, VS_WHEEL_H(vs)[6], 0xFA, -0x190, -0x190, -0x190, HR_X, HR_Z, VS_MOVE_HEADING(vs), 3, 0x32,
                      0x32, 0);
}

/* the hotrod's matrix, its vertices and its collision */
REGS(gp)
void func_802B8278(VS *vs) {
    u8 *model = HR_MODEL, *buf = FRAME_BUF(HR_BUF0, HR_BUF1);

    ENGINE_COST(802B8278, 70);
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(HR_X, HR_Y, HR_Z, HOTROD_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(HR_X, HR_Y, HR_Z, VEHICLE_HOTROD, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_HOTROD, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over HOTROD_STEER_DIV, or over
   HOTROD_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802B83B0(VS *vs) {
    ENGINE_COST(802B83B0, 24);
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? HOTROD_STEER_DIV_AIR : HOTROD_STEER_DIV));
}

/* the physics' settings for the hotrod: gravity, and how its wheels land */
REGS()
void func_802B8424(void) {
    ENGINE_COST(802B8424, 23);
    D_803EBBF4 = D_803EBBF0 * HOTROD_GRAVITY;
    D_803ED3F6 = HOTROD_BOUNCE_MIN;
    D_803ED3F7 = HOTROD_BOUNCE_DIV;
}

/* ---- the chopper --------------------------------------------------------- */

/* its flight's states (D_803EF32C) */
#define CHOPPER_HOVER 0                 /* turning round where it is */
#define CHOPPER_FLY 1                   /* to D_803EF308/30C, speeding up, turning to it */
#define CHOPPER_SLOW 2                  /* slowing down near it */
#define CHOPPER_COME_IN 3               /* straight to it, slowly */
#define CHOPPER_LAND 4                  /* down to the ground (D_80368030) + CHOPPER_LAND_HEIGHT */
#define CHOPPER_DOWN 5                  /* landed, the rotor stopping; then up again */
#define CHOPPER_START 6                 /* the rotor starting, then hovering */

/* its numbers (a frame, where it's per frame) */
#define CHOPPER_SCALE 0x5208            /* its model's scale */
#define CHOPPER_CLIMB 0x14              /* its height's change a frame */
#define CHOPPER_HOVER_TURN 4            /* its turn hovering, a frame */
#define CHOPPER_ACCEL 2                 /* flying: its speed up a frame ... */
#define CHOPPER_TOP_SPEED 0xA0          /* ... to this */
#define CHOPPER_SLOW_DOWN 4             /* slowing down: its speed down a frame ... */
#define CHOPPER_SLOW_SPEED 0x14         /* ... to this */
#define CHOPPER_SLOW_DIST 0x7D0         /* it slows down within this */
#define CHOPPER_TURN_SHIFT 6            /* flying: its turn at most the heading's difference >> this */
#define CHOPPER_LAND_HEIGHT 0xFA0       /* it lands this high above the ground */
#define CHOPPER_SOUND_RANGE 0x3E80      /* its rotor heard within this of the player */

/* set up: from the level loader (func_802A3134), with the model file; its
   position is the level's (D_803EF2EC..F4, 1D990.c) */
REGS(s2)
void func_802B8480(u8 *model) {
    VS *vs = &D_803EF240;

    ENGINE_COST(802B8480, 197);
    CH_MODEL = model;
    CH_BUF0 = D_80358070;
    CH_BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    /* (the original's $a1: what func_802A396C left, the heap's top, never 0) */
    func_802A1388(VEHICLE_CHOPPER, 1, CH_BUF0, CH_BUF1, model);
    VS_HEADING(vs) = 0;
    VS_MOVE_HEADING(vs) = 0;
    D_803EF32A = 0;
    VS_SPEED(vs) = CHOPPER_SLOW_SPEED;
    D_803EF2E6 = 0;
    D_803EF328 = 0;
    func_8029F85C(Q, CH_MODEL, CH_BUF0, CH_BUF1);
    func_802A039C(0, 100, Q);
    func_802A03D4(0, 0, Q);
    func_802A040C(0, 0, Q);
    func_802A0480(0, 0, Q, 0.0f);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, CH_BUF0, CH_BUF1);
    func_802A0320(0, Q);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, CH_BUF1, CH_BUF0);
    func_802A039C(1, 9, Q);
    func_802A03D4(1, 0, Q);
    func_802A040C(1, 0, Q);
    func_802A0290(1, -1, Q);
    func_802A0290(2, -1, Q);
    func_802A0290(3, -1, Q);
    func_802A039C(4, 1, Q);
    func_802A03D4(4, 1, Q);
    func_802A040C(4, 1, Q);
    func_802A0290(4, 1, Q);
    D_803EF32C = CHOPPER_HOVER;
    D_803EF324 = 0;
    D_803EF32D = 0;
    D_803EF31C = 0;
    func_80258230(VEHICLE_CHOPPER, 0x78, 0x2D, 0x2D);
    chopper_frame();
    func_802AA838(CH_BUF1, CH_BUF0, MODEL_MTX_OFF(CH_MODEL));
}

/* its rotor's sound: on within CHOPPER_SOUND_RANGE of the player, louder
   nearer, panned by x */
void func_802B8794(void) {
    s32 d, dx = D_803643E0 - CH_X, pan;

    ENGINE_COST(802B8794, 94);
    d = (s32)func_802ABCDC(D_803643E0, D_803643E4, D_803643E8, CH_X, CH_Y, CH_Z);
    if (d > CHOPPER_SOUND_RANGE) {
        if (D_803EF2E8 != NULL) {
            func_802608C8(D_803EF2E8);
            D_803EF2E8 = NULL;
        }
        return;
    }
    if (D_803EF2E8 == NULL)
        func_80260650(D_80367738, 0x13, &D_803EF2E8);
    d -= 0x1F4;
    if (d < 0)
        d = 0;
    func_80260AB8(D_803EF2E8, 8, 0x7FFF - d);
    pan = 0x40 + (dx >> 5);
    if (pan < 0)
        pan = 0;
    else if (pan > 0x7F)
        pan = 0x7F;
    func_80260AB8(D_803EF2E8, 4, pan);
}

/* each frame: its flight, its rotors, its matrix, where the player would
   get out, and its shadow */
static void chopper_frame(void) {
    VS *vs = &D_803EF240;       /* (the $gp func_802B8D04 sets) */
    s32 *p;

    ENGINE_COST(802B899C, 76);
    func_802B8D04();
    func_802B98E0(vs);
    func_8029E558(Q, FRAME_BUF(CH_BUF0, CH_BUF1), OTHER_BUF(CH_BUF0, CH_BUF1));
    func_802B9B4C(vs);
    p = (s32 *)func_802ABC88(VEHICLE_CHOPPER, 1);
    D_803EF310 = p[0];
    D_803EF314 = p[1];
    D_803EF318 = p[2];
    func_802B8C18(vs);
}

/* hd.c's: the chopper each frame */
void func_802B899C(void) {
    chopper_frame();
}

/* hd.c's, when it has landed: the player's start where it is; in the
   chopper's mode (0x1000) the level's end once it is down or the player is
   above the ground, else that mode once it is landing */
void func_802B8AE4(void) {
    VS *vs = &D_803EF240;
    u8 s = D_803EF32C;

    ENGINE_COST(802B8AE4, 58);
    D_803EF32A = VS_MOVE_HEADING(vs);
    D_803EF328 = VS_SPEED(vs);
    D_803ED808 = D_803EF310;
    D_803ED80C = D_803EF314;
    D_803ED810 = D_803EF318;
    D_80368030 = D_803EF31C;
    if (D_80364A90 != 0x1000) {
        if (s == CHOPPER_LAND)
            D_80364A98 = 0x1000;
    } else if (s == CHOPPER_DOWN || D_803EF31C >= D_803EF314) {
        func_80275390(0x2000);
    }
}

/* the ground's height under it (inside the level), and its shadow there */
REGS(gp)
void func_802B8C18(VS *vs) {
    /* (the original's $t3, the shadow's height outside the level: the
       model's address func_802B9B4C leaves there) */
    s32 x = CH_X, z = CH_Z, h = T(CH_MODEL);

    ENGINE_COST(802B8C18, 59);
    if (x > 0 && z > 0 && x < D_803BE732 << 5 && z < D_803BE736 << 5)
        h = D_803EF31C = func_802A9B1C(0, x, z, D_803EF31C, VEHICLE_CHOPPER, vs, 0);
    func_802582C4(VEHICLE_CHOPPER, CH_X, h, CH_Z, CH_Y, 0, 0, (s16)VS_HEADING(vs));
    D_803EF326 = VS_HEADING(vs);
}

/* one axis's step toward the target at the speed, in proportion to the
   distance left (dist) */
static s32 axis_step(s32 cur, s32 target, s32 dist, s32 speed) {
    s32 d = target - cur, step;
    u32 u = ((u32)iabs(d) << 10) / (u32)dist;

    step = (u32)(u * (u32)speed) >> 10;
    return cur + (d < 0 ? -step : step);
}

/* the heading by the turn, and on at the speed */
static void chopper_turn(VS *vs) {
    s32 h = wrap_fff((u16)VS_MOVE_HEADING(vs) + D_803EF324), z;

    VS_MOVE_HEADING(vs) = h;
    VS_HEADING(vs) = h;
    CH_X = func_802A860C(h, &VS_SPEED(vs), &CH_X, &CH_Z, 0.0f, &z);
    CH_Z = z;
}

/* flying: toward D_803EF308/30C, speeding up and turning toward it, until
   within CHOPPER_SLOW_DIST.  On its flight into a level (mode 0x800), near
   enough (a distance by the level), the next stop is the level's
   (D_80368044/48) and the intro ends. */
static void chopper_fly(VS *vs) {
    s32 x = CH_X, z = CH_Z, tx = D_803EF308, tz = D_803EF30C, near, d1, d2, dir, diff, max, xr, zr;
    s64 dist = func_802B988C();

    if (D_80364A90 == 0x800) {
        switch (D_802E8BDC) {
        case 0x1A: case 4: near = 0xBB80; break;
        case 0x1D: case 0x3A: case 0xD: near = 0x11170; break;
        default: near = 0x7530; break;
        }
        if (dist <= near) {
            D_80364A98 = 1;
            func_8029A7E4(D_80305D40);
            D_803EF308 = D_80368044;
            /* (the original goes on with this in x's register: the heading
               below is worked out from (D_80368048, z)) */
            x = D_803EF30C = D_80368048;
            func_8026AF6C(0x4000);
        }
    }
    if (dist < CHOPPER_SLOW_DIST) {
        D_803EF32C = CHOPPER_SLOW;
        chopper_turn(vs);
        return;
    }
    VS_SPEED(vs) = VS_SPEED(vs) + CHOPPER_ACCEL > CHOPPER_TOP_SPEED ? CHOPPER_TOP_SPEED : VS_SPEED(vs) + CHOPPER_ACCEL;
    /* the heading to it: func_802ABB1C's angle at the target (by the
       quadrant), or its reverse, whichever puts a point at that distance
       nearer the target */
    dir = func_802ABB1C(tx, tz, 0, (s32)dist, x, z);
    if (dir > ANGLE_QUARTER) {
        dir = func_802ABB1C(tx, tz, (s32)dist, 0, x, z) + ANGLE_QUARTER;
        if (dir > ANGLE_HALF)
            dir = func_802ABB1C(tx, tz, 0, -(s32)dist, x, z) + ANGLE_HALF;
    }
    func_802ACE38(0, dist, dir, &xr, &zr);
    d1 = iabs(xr + x - tx) + iabs(zr + z - tz);
    func_802ACE38(0, dist, ANGLE_WRAP - dir, &xr, &zr);
    d2 = iabs(xr + x - tx) + iabs(zr + z - tz);
    if (d1 >= d2)
        dir = ANGLE_WRAP - dir;
    /* turn toward it: at most the difference >> CHOPPER_TURN_SHIFT, a
       unit a frame more */
    diff = (u16)VS_MOVE_HEADING(vs) - dir;
    if (diff >= 0)
        max = diff >= ANGLE_HALF ? ANGLE_WRAP - diff : diff;
    else
        max = diff < -(ANGLE_HALF - 1) ? diff + ANGLE_WRAP : -diff;
    max = (u32)max >> CHOPPER_TURN_SHIFT;
    if (diff > 0 ? diff >= ANGLE_HALF : diff >= -ANGLE_HALF) {
        D_803EF324 = D_803EF324 + 1 > max ? max : D_803EF324 + 1;
    } else {
        D_803EF324 = D_803EF324 - 1 < -max ? -max : D_803EF324 - 1;
    }
    chopper_turn(vs);
}

/* coming in: its turn back to 0, and straight to the target at the speed
   until it is there or the distance stops shrinking; then landing */
static void chopper_come_in(VS *vs) {
    s64 dist;

    D_803EF324 = step_toward(D_803EF324, 0, 1);
    dist = func_802B988C();
    if (dist != 0) {
        CH_X = axis_step(CH_X, D_803EF308, (s32)dist, VS_SPEED(vs));
        CH_Z = axis_step(CH_Z, D_803EF30C, (s32)dist, VS_SPEED(vs));
        dist = func_802B988C();
        if (dist < D_803EF320) {
            D_803EF320 = (s32)dist;
            return;
        }
    }
    CH_X = D_803EF308;
    CH_Z = D_803EF30C;
    D_803EF32C = CHOPPER_LAND;
    func_802A0290(4, 1, Q);
}

/* its flight a frame (see the top); then, but landing or down, the level's
   ground (func_802A5604) and its height toward its flying height */
REGS()
void func_802B8D04(void) {
    VS *vs = &D_803EF240;
    f32 f0;
    s32 t;

    ENGINE_COST(802B8D04, 85);
    switch (D_803EF32C) {
    case CHOPPER_START:
        part(4, Q, &f0);
        if (!(f0 == 0.0f)) {
            func_802A03D4(4, 1, Q);
            func_802A0290(4, 1, Q);
        }
        D_803EF32C = CHOPPER_HOVER;
        /* fall through */
    case CHOPPER_HOVER:
        D_803EF324 = step_toward(D_803EF324, CHOPPER_HOVER_TURN, 1);
        chopper_turn(vs);
        break;
    case CHOPPER_FLY:
        chopper_fly(vs);
        break;
    case CHOPPER_SLOW:
        if (VS_SPEED(vs) > CHOPPER_SLOW_SPEED) {
            VS_SPEED(vs) -= CHOPPER_SLOW_DOWN;
            chopper_turn(vs);
            break;
        }
        VS_SPEED(vs) = CHOPPER_SLOW_SPEED;
        D_803EF32C = CHOPPER_COME_IN;
        D_803EF320 = (s32)func_802B988C();
        /* fall through */
    case CHOPPER_COME_IN:
        chopper_come_in(vs);
        break;
    case CHOPPER_LAND:
        t = D_80368030 + CHOPPER_LAND_HEIGHT;
        if (t != CH_Y) {
            CH_Y = step_toward(CH_Y, t, CHOPPER_CLIMB);
        } else if (part(4, Q, NULL) != 1) {
            /* down: the landing's sound by the player's distance */
            D_803EF32D = 1;
            if (D_80364A90 != 0x1000) {
                t = 0x88B8 - (func_8026A610(D_803643E0, D_803643E0, CH_X, CH_Z) << 1);
                if (t > 0xFA0) {
                    if (t > 0x7FFF)
                        t = 0x7FFF;
                    func_80260AB8(func_80260650(D_80367738, 0x28, NULL), 8, t);
                }
            }
            D_803EF32C = CHOPPER_DOWN;
            func_802A0290(4, 1, Q);
        }
        break;
    case CHOPPER_DOWN:
        if (part(4, Q, NULL) != 1)
            D_803EF32E = 1;
        if (D_803EF304 != CH_Y) {
            CH_Y = step_toward(CH_Y, D_803EF304, CHOPPER_CLIMB);
        } else if (D_803EF32E != 0) {
            D_803EF32C = CHOPPER_HOVER;
            D_803EF32E = 0;
        }
        break;
    default:
        engine_trap(0x802B8D58);
        break;
    }
    if (D_803EF32C == CHOPPER_LAND || D_803EF32C == CHOPPER_DOWN)
        return;
    func_802A5604(D_80358074);
    if (D_803EF304 != CH_Y)
        CH_Y = step_toward(CH_Y, D_803EF304, CHOPPER_CLIMB);
}

/* how far it is from D_803EF308/30C (x and z) */
REGS(-> s1+f0)
s64 func_802B988C(void) {
    ENGINE_COST(802B988C, 21);
    return func_802ABCDC(CH_X, 0, CH_Z, D_803EF308, 0, D_803EF30C);
}

/* its sound's pitch by its speed, its tail's tilt (part 2) by its speed and
   its body's lean (part 3) by its turn */
REGS(gp)
void func_802B98E0(VS *vs) {
    s32 t = CHOPPER_TOP_SPEED - VS_SPEED(vs), last = D_803EF2E6;
    f32 tilt = (f32)t / (f32)0x140;

    ENGINE_COST(802B98E0, 118);
    D_803EF2E6 = t;
    if (last != t && D_803EF2E8 != NULL)
        func_80260AB8(D_803EF2E8, 0x10, f2i(0.8f + (f32)t * -0.004f));
    func_802A0360(2, 0, Q, tilt);
    t = D_803EF324;
    if (t == 1 || t == -1)
        t = 0;
    if (t >= 0)
        func_802A0360(3, 0, Q, (f32)(0x20 - t) / (f32)0x40);
    else
        func_802A0360(3, 0, Q, (f32)-t / (f32)0x40 + 0.5f);
}

/* the chopper's matrix and its collision */
REGS(gp)
void func_802B9B4C(VS *vs) {
    u8 *model = CH_MODEL, *buf = FRAME_BUF(CH_BUF0, CH_BUF1);
    s32 *m = (s32 *)(buf + MODEL_MTX_OFF(model));

    ENGINE_COST(802B9B4C, 55);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(CH_X, CH_Y, CH_Z, CHOPPER_SCALE, m);
    func_802ABBEC(VEHICLE_CHOPPER, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B78F4(s32 carrier) {
    CARRY_KEEP(802B78F4, 802B7930, 802B7950, 802B795C, &D_803EEE70, D_803EEF18, D_803EEF20);
}

REGS(a3)
void func_802B7980(s32 carrier) {
    CARRY_MOVE(802B7980, 802B79BC, 802B79D0, 802B79D8, 802B7A38, 802B7A40, 802B7A6C, &D_803EEE70, &D_803EEF18, &D_803EEF1C, &D_803EEF20, 8, 0x2BC, 0x190,
               D_803ED40B = 1, func_802B8424(), func_802B8278(&D_803EEE70));
}
