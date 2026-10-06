/*
 * hd_code 6E200 (us.v11 0x802B29C0-0x802B5900): Skyfall (type 3) and
 * Ramdozer (type 4), as native C (engine.h).
 *
 * Skyfall's parts are D_803EDFE0, its state D_803EE2E0 and its position
 * D_803EE38C..94.  func_802B29C0 sets it up (from the level loader),
 * func_802B327C runs it each frame (from hd.c and at the end of the setup);
 * the others are hd.c's hooks for it.  It is the hotrod's code (72B80) with
 * a boost: L or R held burns D_803EE3B1's fuel for speed, with its sound
 * and a flame; part 3 is lifted by D_803EE3A4 (pad_lift: B or Z, A), part
 * 2 by D_803EE3AF (pad_lever).
 *
 * Ramdozer's parts are D_803EE3C0, its state D_803EE6C0 and its position
 * D_803EE768..70 (see further down).
 *
 * Each one's two callbacks for 62740's carrying (shared.h) follow its
 * frame's functions: func_802B30F4 and func_802B3180, func_802B4818 and
 * func_802B48A4.
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* Skyfall's .bss (asm/data/hd_code/6E200.bss.s) */
extern Part D_803EDFE0[32];
extern VS D_803EE2E0;
extern SndState *PTR32 D_803EE388;              /* its boost's sound, while it plays */
extern s32 D_803EE38C, D_803EE390, D_803EE394;  /* x, y, z */
extern u8 *PTR32 D_803EE398;                    /* its model file */
extern u8 *PTR32 D_803EE39C;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EE3A0;
extern f32 D_803EE3A4;                          /* part 3's lift, 0..1 */
extern u16 D_803EE3A8;                          /* its turn rate (func_802A7FD8) */
extern u16 D_803EE3AA;                          /* the heading it turns to against a wall (D_803A7425) */
extern s16 D_803EE3AC;                          /* the dust's size after a boost */
extern u8 D_803EE3AE;                           /* frames until the next sparks */
extern u8 D_803EE3AF;                           /* part 2's lever, 0..100 */
extern s8 D_803EE3B0;                           /* turning to D_803EE3AA */
extern u8 D_803EE3B2;                           /* frames without the throttle or the boost after a hit */
extern u8 D_803EE3B3;                           /* boosting */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern u8 D_80364A69;
extern u16 D_803F7840;                          /* (Ramdozer's speed at its frame's start; 0 here) */
extern Part *PTR32 D_803F77D0;
extern u8 D_80305D00[];                         /* Skyfall's parts' collision (56040's func_8029A800) */
extern u8 D_802C2314[];                         /* the flame's part record (56040's list) */
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */
extern u8 D_802C37C0[];                         /* the boost's effect record */

static void skyfall_frame(void);
REGS()
void func_802B30B0(void);
REGS(gp)
void func_802B37B0(VS *vs);
REGS(gp)
void func_802B3C68(VS *vs);
REGS(gp)
void func_802B3E40(VS *vs);
REGS(gp -> s3)
s32 func_802B3F78(VS *vs);
REGS(gp)
void func_802B3FF0(VS *vs);

#define T(p) ((s32)(p))

/* ---- Skyfall -------------------------------------------------------------- */

#define SKY D_803EDFE0
#define SKY_X D_803EE38C
#define SKY_Y D_803EE390
#define SKY_Z D_803EE394
#define SKY_MODEL D_803EE398
#define SKY_BUF0 D_803EE39C
#define SKY_BUF1 D_803EE3A0

/* Skyfall's numbers (a frame, where it's per frame) */
#define SKYFALL_SCALE 0x2AF8            /* its model's scale */
#define SKYFALL_SPAN_ALONG 0x2D4        /* its wheels' spans (func_802A8768) */
#define SKYFALL_SPAN_ACROSS 0x260
#define SKYFALL_BRAKE 0x14              /* speed lost braking or against the stick, a frame */
#define SKYFALL_TURN_RATE 0x3A98        /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define SKYFALL_TURN_RATE_SLIDE 0x7D0   /* ... with B or Z going forward (A going back): a slide */
#define SKYFALL_SLOPE_DIV 724.0f        /* func_802A843C: the slope's push is the height difference over this */
#define SKYFALL_STEER_DIV 2.5f          /* the steering rate: the speed over this ... */
#define SKYFALL_STEER_DIV_AIR 11.0f     /* ... or this with a wheel in the air */
#define SKYFALL_WALL_TURN 0.1f          /* func_802A71DC: turning along a wall, times the speed */
#define SKYFALL_HIT_FRAMES 5            /* frames without the throttle after hitting something */
#define SKYFALL_HIT_MIN_SPEED 0x50      /* the speed it bounces back with is at least half this */
#define SKYFALL_GRAVITY 4.0f            /* times the level's */
#define SKYFALL_BOUNCE_MIN 0x3C         /* a landing harder than this bounces ... */
#define SKYFALL_BOUNCE_DIV 3            /* ... at the speed over this */
#define SKYFALL_WHEEL_SPIN_DIV 30       /* the wheels' spin: the speed over this */
#define BOOST_FUEL_MAX 100              /* the boost's fuel (D_803EE3B1) ... */
#define BOOST_FUEL_BURN 4               /* ... burnt a frame boosting ... */
#define BOOST_FUEL_REFILL 1             /* ... and back a frame not */
#define BOOST_ACCEL 0x28                /* the speed gained a frame boosting ... */
#define BOOST_TOP_SPEED 0x190           /* ... up to this */
#define BOOST_SLOW 0x19                 /* above SKYFALL_TOP_SPEED without it, lost a frame */
#define SKYFALL_TOP_SPEED 0xFA
#define BOOST_DUST 0x78                 /* the dust's size after a boost, shrinking 8 a frame */

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802B29C0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EE2E0;
    s32 avg;

    ENGINE_COST(802B29C0, 239);
    SKY_MODEL = model;
    SKY_BUF0 = D_80358070;
    SKY_BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(VEHICLE_BUGGY, 0, SKY_BUF0, SKY_BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x130, 0x16A, -0x130, 0x16A, 0x130, -0x16A);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x140, 0x172, -0x140, 0x172, 0x140, -0x172);
    SKY_X = x;
    SKY_Y = y;
    SKY_Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    D_803F7840 = 0;
    D_803EE3AF = 50;
    D_803EE3A4 = 0.5f;
    func_802A992C(VS_WHEELS(vs), SKY_Y, x, z, VS_WHEEL_H(vs), &SKY_Y, (s16 *)&VS_HEADING(vs), VEHICLE_BUGGY, vs, 0,
                  &avg);
    func_8029F85C(SKY, SKY_MODEL, SKY_BUF0, SKY_BUF1);
    func_802A039C(0, 100, SKY);
    func_802A03D4(0, 0, SKY);
    func_802A040C(0, 0, SKY);
    func_802A0480(0, 0, SKY, 0.0f);
    func_802A0290(0, 1, SKY);
    func_8029E558(SKY, SKY_BUF0, SKY_BUF1);
    func_802A0320(0, SKY);
    func_802A0290(0, 1, SKY);
    func_8029E558(SKY, SKY_BUF1, SKY_BUF0);
    SET_GEARS(vs, -0xB4, 5, 2, 0, 0x78, 4, 0x78, 0xA0, 5, 0xA0, 0xDC, 2, 0xDC, 0xFA, 2);
    func_802A6F00(vs);
    D_803EE3B1 = BOOST_FUEL_MAX;
    D_803EE3B2 = 0;
    D_803EE3AE = 0;
    D_803EE3B0 = 0;
    D_803EE388 = NULL;
    func_8029C354(VEHICLE_BUGGY, MODEL_AT(SKY_MODEL, 4), MODEL_AT(SKY_MODEL, 8), SKYFALL_SCALE);
    func_80258230(VEHICLE_BUGGY, 0x50, 0x1F, 0x1F);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    skyfall_frame();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(SKY_BUF1, SKY_BUF0, MODEL_MTX_OFF(SKY_MODEL));
    D_80364A69 = 1;
}

/* hd.c's: the player gets in: the doors shut, the flame out */
void func_802B2D7C(void) {
    Part *p = SKY;

    ENGINE_COST(802B2D7C, 95);
    VS_TURNING(&D_803EE2E0) = 0;
    func_802A039C(1, 0, p);
    func_802A03D4(1, 0, p);
    func_802A040C(1, 0, p);
    func_802A0290(1, -1, p);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 1, p);
    func_802A0290(2, -1, p);
    func_802A039C(3, 0, p);
    func_802A03D4(3, 0, p);
    func_802A040C(3, 1, p);
    func_802A0290(3, -1, p);
    func_802A05D0(D_802C2314, 0);
    func_802A05F8(D_802C2314, 0);
    func_802A0620(D_802C2314, 0);
    func_802A0508(D_802C2314, -1);
    D_8036444C = 0xD48;
    D_80364450 = 0x3E8;
    func_802C4310(0x8C);
}

/* hd.c's: whether it can be left: not while a wheel is in the air */
u8 func_802B2EF8(void) {
    ENGINE_COST(802B2EF8, 20);
    return !ANY_AIRBORNE(&D_803EE2E0);
}

/* hd.c's: the player gets out */
void func_802B2F54(void) {
    ENGINE_COST(802B2F54, 19);
    VS_SPEED(&D_803EE2E0) = 0;
    func_802A7764((u32 *)SKY_BUF0, (u32 *)SKY_BUF1, 0x800);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802B2FA0(void) {
    VS *vs = &D_803EE2E0;

    ENGINE_COST(802B2FA0, 35);
    func_802A9A60(VS_WHEELS(vs), SKY_Y, SKY_X, SKY_Z, VS_WHEEL_H(vs), &SKY_Y, (s16 *)&VS_HEADING(vs), VEHICLE_BUGGY,
                  vs, 0);
    func_802B3E40(vs);
    func_802A133C(SKY_X, SKY_Y, SKY_Z, VEHICLE_BUGGY, vs);
}

/* its light */
REGS()
void func_802B30B0(void) {
    ENGINE_COST(802B30B0, 17);
    func_802ABD54(VEHICLE_BUGGY, SKY_X, SKY_Y, SKY_Z);
}

/* hit something: back to where it was at the frame's start (and its
   vertices), the throttle off a while, the speed turned round and halved
   (at least `min` before) */
static void bounce_back(VS *vs, Part *parts, s32 *x, s32 *y, s32 *z, u8 *buf0, u8 *buf1, u8 *off_frames, s32 min) {
    s32 v;

    func_802A768C((u8 *)parts, x, y, z, (u32 *)OTHER_BUF(buf0, buf1), (u32 *)FRAME_BUF(buf0, buf1), 0x800, (u8 *)vs);
    *off_frames = SKYFALL_HIT_FRAMES;
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < min)
            v = min;
    } else if (v > -min) {
        v = -min;
    }
    VS_SPEED(vs) = -v >> 1;
}

/* each frame */
static void skyfall_frame(void) {
    VS *vs = &D_803EE2E0;
    s32 step = 0, x, z, rate_i;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802B327C, 222);
    func_802B30B0();
    func_802A75DC((u8 *)SKY, &SKY_X, &SKY_Y, &SKY_Z, (u8 *)vs);
    if (VS_IN_SETUP(vs) == 0)
        func_802B37B0(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    func_802B3FF0(vs);
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802B3F78(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803EE3B2 == 0)
        func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), SKYFALL_BRAKE, vs, &step);
    else
        D_803EE3B2--;
    func_802A7FD8(D_803EE3A8, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_BUGGY, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), SKYFALL_SLOPE_DIV, vs);
    if (D_803EE3B0 != 0)
        func_802A7070((s16 *)&D_803EE3AA, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &SKY_X, &SKY_Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &SKY_X, &SKY_Z, &SKY_Y, VEHICLE_BUGGY, SKYFALL_SPAN_ALONG, SKYFALL_SPAN_ACROSS, VS_WHEELS(vs),
                  VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(SKY, FRAME_BUF(SKY_BUF0, SKY_BUF1), OTHER_BUF(SKY_BUF0, SKY_BUF1));
    func_802B3E40(vs);
    /* what it hits */
    func_8029A800(D_803EE38C, D_803EE390, D_803EE394, D_80305D00, 1, 1, 6, vs->unk76, 0x64, 0, 3, vs);
    func_8029C52C(VEHICLE_BUGGY, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = SKY;
        func_802BE77C(VEHICLE_BUGGY, vs);
        D_803EE3B0 = 0;
        if (D_803A7424 != 0) {
            bounce_back(vs, SKY, &SKY_X, &SKY_Y, &SKY_Z, SKY_BUF0, SKY_BUF1, &D_803EE3B2, SKYFALL_HIT_MIN_SPEED);
            func_802B3E40(vs);
        }
    } else {
        D_803EE3B0 = 1;
        turn_along_wall(vs, &D_803EE3AA, SKYFALL_WALL_TURN);
        D_803F77D0 = SKY;
        func_802BE77C(VEHICLE_BUGGY, vs);
    }
    PLAYER_FROM(SKY_X, SKY_Y, SKY_Z, vs, VEHICLE_BUGGY);
}

/* hd.c's: Skyfall each frame */
void func_802B327C(void) {
    skyfall_frame();
}

/* The parts a frame: sparks every other frame while turning; part 3's lift
   (pad_lift); part 2's lever (pad_lever); the wheels' spin with the speed;
   the engine's sound; and the boost: L or R held, with fuel and not just
   hit, burns BOOST_FUEL_BURN for BOOST_ACCEL of speed, with its sound,
   flame and effect; otherwise the fuel comes back and the speed above
   SKYFALL_TOP_SPEED goes. */
REGS(gp)
void func_802B37B0(VS *vs) {
    Part *p = SKY;
    s32 s;

    ENGINE_COST(802B37B0, 151);
    if (D_803EE3AE != 0) {
        D_803EE3AE--;
    } else if (VS_TURNING(vs) != 0) {
        D_803EE3AE = 1;
        if (func_802A5ED0() < 0xF) {
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_BUGGY, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
            func_802A6274(T(D_802C2954), 0x29810, 1, VEHICLE_BUGGY, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
        }
    }
    D_803EE3A4 = pad_lift(vs, D_803EE3A4, 10);
    func_802A0360(3, 0, p, D_803EE3A4);
    D_803EE3AF = pad_lever(D_803EE3AF, 5, 10);
    func_802A0360(2, 0, p, (f32)D_803EE3AF / 100.0f);
    /* the wheels' spin, backwards in reverse */
    s = VS_SPEED(vs);
    func_802A03D4(1, s < 0 ? 1 : 0, p);
    s = (u32)iabs(s) / SKYFALL_WHEEL_SPIN_DIV;
    func_802A039C(1, s, p);
    func_802C4584(s);
    /* the boost */
    if (D_803EE3B2 != 0) {
        D_803EE3B2--;
    } else if ((PAD_L != 0 || PAD_R != 0) && (s8)D_803EE3B1 >= BOOST_FUEL_BURN) {
        D_803EE3B1 = (s8)D_803EE3B1 - BOOST_FUEL_BURN;
        if (D_803EE3B3 == 0) {
            D_803EE3B3 = 1;
            func_80260650(D_80367738, 0x8D, &D_803EE388);
        }
        func_802A05A4(D_802C2314, 1, 0.0f);
        func_802A6274(T(D_802C37C0), 0x186A0, 1, VEHICLE_BUGGY, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0);
        s = VS_SPEED(vs);
        VS_SPEED(vs) = s < BOOST_TOP_SPEED ? s + BOOST_ACCEL : BOOST_TOP_SPEED;
        func_802B3C68(vs);
        return;
    }
    if (D_803EE388 != NULL) {
        func_802608C8(D_803EE388);
        D_803EE388 = NULL;
    }
    D_803EE3AC = BOOST_DUST;
    D_803EE3B3 = 0;
    func_802A05A4(D_802C2314, 0, 0.0f);
    s = VS_SPEED(vs);
    if (s > SKYFALL_TOP_SPEED) {
        s -= BOOST_SLOW;
        if (s < SKYFALL_TOP_SPEED)
            s = SKYFALL_TOP_SPEED;
        VS_SPEED(vs) = s;
    }
    s = (s8)D_803EE3B1 + BOOST_FUEL_REFILL;
    if (s > BOOST_FUEL_MAX)
        s = BOOST_FUEL_MAX;
    D_803EE3B1 = s;
    func_802B3C68(vs);
}

/* dust behind it on soft ground (grip under 3, not the static triangles)
   with its back wheel down: while turning, or while boosting (shrinking
   after D_803EE3AC's) */
REGS(gp)
void func_802B3C68(VS *vs) {
    s32 size;

    ENGINE_COST(802B3C68, 80);
    if (VS_TURNING(vs) != 0) {
        size = 0x32;
    } else {
        if (D_803EE3B3 == 0)
            return;
        size = D_803EE3AC;
        if (size < 8)
            size = 8;
        D_803EE3AC = size -= 8;
    }
    if (VS_AIRBORNE(vs)[2] != 1 && VS_GRIP(vs) < 3 && VS_ON_STATIC(vs) == 0)
        func_8027BE7C(2, VS_WHEEL_H(vs)[6], 0x190, -0x12C, -0x190, -0x12C, SKY_X, SKY_Z, VS_MOVE_HEADING(vs), 4, size,
                      size, 0);
}

/* Skyfall's matrix, its vertices and its collision */
REGS(gp)
void func_802B3E40(VS *vs) {
    u8 *model = SKY_MODEL, *buf = FRAME_BUF(SKY_BUF0, SKY_BUF1);

    ENGINE_COST(802B3E40, 70);
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(SKY_X, SKY_Y, SKY_Z, SKYFALL_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(SKY_X, SKY_Y, SKY_Z, VEHICLE_BUGGY, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_BUGGY, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over SKYFALL_STEER_DIV, or over
   SKYFALL_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802B3F78(VS *vs) {
    ENGINE_COST(802B3F78, 25);
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? SKYFALL_STEER_DIV_AIR : SKYFALL_STEER_DIV));
}

/* the physics' settings for Skyfall: gravity, how its wheels land, and
   its turn rate (a slide with B or Z going forward, A going back) */
REGS(gp)
void func_802B3FF0(VS *vs) {
    ENGINE_COST(802B3FF0, 36);
    D_803EBBF4 = D_803EBBF0 * SKYFALL_GRAVITY;
    D_803ED3F6 = SKYFALL_BOUNCE_MIN;
    D_803ED3F7 = SKYFALL_BOUNCE_DIV;
    if (VS_SPEED(vs) > 0)
        D_803EE3A8 = PAD_B_OR_Z != 0 ? SKYFALL_TURN_RATE_SLIDE : SKYFALL_TURN_RATE;
    else
        D_803EE3A8 = PAD_A != 0 ? SKYFALL_TURN_RATE_SLIDE : SKYFALL_TURN_RATE;
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B30F4(s32 carrier) {
    CARRY_KEEP(802B30F4, 802B3130, 802B3150, 802B315C, &D_803EE2E0, D_803EE38C, D_803EE394);
}

REGS(a3)
void func_802B3180(s32 carrier) {
    CARRY_MOVE(802B3180, 802B31BC, 802B31D0, 802B31D8, 802B322C, 802B3234, 802B3260, &D_803EE2E0, &D_803EE38C, &D_803EE390, &D_803EE394, 3, 0x2D4, 0x260,
               (void)0, func_802B3FF0(&D_803EE2E0), func_802B3E40(&D_803EE2E0));
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B40A8(u8 *dst) {
    ENGINE_COST(802B40A8, 7);
    func_802AC7DC(dst, (u8 *)&D_803EE2E0, (u32 *)&D_803EE38C);
}

/* and back */
void func_802B40D4(u8 *src) {
    ENGINE_COST(802B40D4, 11);
    func_802AC85C(src, (u8 *)&D_803EE2E0, (u32 *)&D_803EE38C);
}

/* ---- Ramdozer ------------------------------------------------------------- */

/* Ramdozer's .bss */
extern Part D_803EE3C0[32];
extern VS D_803EE6C0;
extern s32 D_803EE768, D_803EE76C, D_803EE770;  /* x, y, z */
extern u8 *PTR32 D_803EE774;                    /* its model file */
extern u8 *PTR32 D_803EE778;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EE77C;
extern f32 D_803EE780;                          /* part 2's (the blade's) lift, 0..1 */
extern u16 D_803EE784;                          /* its turn rate (func_802A7FD8) */
extern u16 D_803EE786;                          /* the heading it turns to against a wall (D_803A7425) */
extern u8 D_803EE788;                           /* frames until the next sparks */
extern s8 D_803EE789;                           /* turning to D_803EE786 */
extern u8 D_803EE78A;                           /* frames without the throttle after a hit */

extern u8 D_80305D10[];                         /* Ramdozer's parts' collision (56040's func_8029A800) */
extern u8 D_802C2190[], D_802C21A4[];           /* its tracks' part records (56040's list) */
extern u8 D_802C21B8[];                         /* part 3's */

static void ramdozer_frame(void);
REGS()
void func_802B47D4(void);
REGS(gp)
void func_802B4EF8(VS *vs);
REGS(gp)
void func_802B54EC(VS *vs);
REGS(gp)
void func_802B568C(VS *vs);
REGS(gp -> s3)
s32 func_802B57C4(VS *vs);
REGS(gp)
void func_802B5814(VS *vs);

#define RAM D_803EE3C0
#define RAM_X D_803EE768
#define RAM_Y D_803EE76C
#define RAM_Z D_803EE770
#define RAM_MODEL D_803EE774
#define RAM_BUF0 D_803EE778
#define RAM_BUF1 D_803EE77C

/* Ramdozer's numbers (a frame, where it's per frame) */
#define RAMDOZER_SCALE 0x2710           /* its model's scale */
#define RAMDOZER_SPAN_ALONG 0x258       /* its wheels' spans (func_802A8768) */
#define RAMDOZER_SPAN_ACROSS 0x190
#define RAMDOZER_BRAKE 0x14             /* speed lost braking or against the stick, a frame */
#define RAMDOZER_TURN_RATE 0x2328       /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define RAMDOZER_TURN_RATE_SLOW 0x7D0   /* ... with B or Z */
#define RAMDOZER_SLOPE_DIV 600.0f       /* func_802A843C: the slope's push is the height difference over this */
#define RAMDOZER_STEER 0x4B             /* the steering rate (not by the speed: it turns on the spot) ... */
#define RAMDOZER_STEER_AIR 0x16         /* ... with a wheel in the air */
#define RAMDOZER_WALL_TURN 0.16f        /* func_802A71DC: turning along a wall, times the speed */
#define RAMDOZER_HIT_FRAMES 5           /* frames without the throttle after hitting something */
#define RAMDOZER_GRAVITY 2.0f           /* times the level's */
#define RAMDOZER_BOUNCE_MIN 0x6E        /* a landing harder than this bounces ... */
#define RAMDOZER_BOUNCE_DIV 4           /* ... at the speed over this */

/* part i's state: func_802A04BC's first result */
static s32 part_state(s32 i, Part *parts) {
    s32 f11, f12, f14, fC, fE, f13;
    f32 f4;

    return func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &f13, &f4);
}

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802B4100(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EE6C0;
    s32 avg;

    ENGINE_COST(802B4100, 227);
    RAM_MODEL = model;
    RAM_BUF0 = D_80358070;
    RAM_BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(VEHICLE_BULLDOZER, 0, RAM_BUF0, RAM_BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0xC8, 0x12C, -0xC8, 0x12C, 0xC8, -0x12C);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x12C, 0x1C2, -0x12C, 0x1C2, 0x12C, -0x1C2);
    RAM_X = x;
    RAM_Y = y;
    RAM_Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), RAM_Y, x, z, VS_WHEEL_H(vs), &RAM_Y, (s16 *)&VS_HEADING(vs), VEHICLE_BULLDOZER, vs,
                  0, &avg);
    func_8029F85C(RAM, RAM_MODEL, RAM_BUF0, RAM_BUF1);
    func_802A039C(0, 100, RAM);
    func_802A03D4(0, 0, RAM);
    func_802A040C(0, 0, RAM);
    func_802A0480(0, 0, RAM, 0.0f);
    func_802A0290(0, 1, RAM);
    func_8029E558(RAM, RAM_BUF0, RAM_BUF1);
    func_802A0320(0, RAM);
    func_802A0290(0, 1, RAM);
    func_8029E558(RAM, RAM_BUF1, RAM_BUF0);
    SET_GEARS(vs, -0xB4, 0, 6, 0, 0x50, 6, 0x50, 0x8C, 4, 0x8C, 0xBE, 2, 0xBE, 0xFA, 2);
    func_802A6F00(vs);
    D_803EE789 = 0;
    D_803EE78A = 0;
    D_803EE788 = 0;
    D_803F7840 = 0;
    D_803EE780 = 0.5f;
    func_8029C354(VEHICLE_BULLDOZER, MODEL_AT(RAM_MODEL, 4), MODEL_AT(RAM_MODEL, 8), RAMDOZER_SCALE);
    func_80258230(VEHICLE_BULLDOZER, 0x96, 0x32, 0x32);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    ramdozer_frame();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(RAM_BUF1, RAM_BUF0, MODEL_MTX_OFF(RAM_MODEL));
}

/* hd.c's: the player gets in: its parts back as they start */
void func_802B448C(void) {
    Part *p = RAM;

    ENGINE_COST(802B448C, 92);
    VS_TURNING(&D_803EE6C0) = 0;
    func_802A03D4(1, 0, p);
    func_802A05D0(D_802C2190, 0);
    func_802A05F8(D_802C2190, 0);
    func_802A0620(D_802C2190, 0);
    func_802A0508(D_802C2190, -1);
    func_802A05D0(D_802C21A4, 0);
    func_802A05F8(D_802C21A4, 0);
    func_802A0620(D_802C21A4, 0);
    func_802A0508(D_802C21A4, -1);
    func_802A05D0(D_802C21B8, 0);
    func_802A05F8(D_802C21B8, 0);
    func_802A0620(D_802C21B8, 0);
    func_802A0508(D_802C21B8, -1);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 1, p);
    func_802A0290(2, -1, p);
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xB);
}

/* hd.c's: whether it can be left: not while a wheel is in the air */
u8 func_802B45FC(void) {
    ENGINE_COST(802B45FC, 23);
    return !ANY_AIRBORNE(&D_803EE6C0);
}

/* hd.c's: the player gets out: the tracks stopped */
void func_802B4658(void) {
    ENGINE_COST(802B4658, 27);
    VS_SPEED(&D_803EE6C0) = 0;
    func_802A7764((u32 *)RAM_BUF0, (u32 *)RAM_BUF1, 0x800);
    func_802C444C();
    func_802A05D0(D_802C2190, 0);
    func_802A05D0(D_802C21A4, 0);
}

/* hd.c's: put back on the ground where it is */
void func_802B46C4(void) {
    VS *vs = &D_803EE6C0;

    ENGINE_COST(802B46C4, 35);
    func_802A9A60(VS_WHEELS(vs), RAM_Y, RAM_X, RAM_Z, VS_WHEEL_H(vs), &RAM_Y, (s16 *)&VS_HEADING(vs),
                  VEHICLE_BULLDOZER, vs, 0);
    func_802B568C(vs);
    func_802A133C(RAM_X, RAM_Y, RAM_Z, VEHICLE_BULLDOZER, vs);
}

/* its light */
REGS()
void func_802B47D4(void) {
    ENGINE_COST(802B47D4, 17);
    func_802ABD54(VEHICLE_BULLDOZER, RAM_X, RAM_Y, RAM_Z);
}

/* pushed back by something: back to where it was at the frame's start
   (and its vertices), the throttle off for RAMDOZER_HIT_FRAMES, the speed
   turned round (0x3E..0x7D forward, 0x2D..0x5A back, before) */
static void ramdozer_bounce(VS *vs) {
    s32 v;

    D_803EE789 = 0;
    func_802A768C((u8 *)RAM, &RAM_X, &RAM_Y, &RAM_Z, (u32 *)OTHER_BUF(RAM_BUF0, RAM_BUF1),
                  (u32 *)FRAME_BUF(RAM_BUF0, RAM_BUF1), 0x800, (u8 *)vs);
    D_803EE78A = RAMDOZER_HIT_FRAMES;
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < 0x3E)
            v = 0x3E;
        else if (v > 0x7D)
            v = 0x7D;
    } else if (v > -0x2D) {
        v = -0x2D;
    } else if (v < -0x5A) {
        v = -0x5A;
    }
    VS_SPEED(vs) = -v;
    func_802B568C(vs);
}

/* each frame */
static void ramdozer_frame(void) {
    VS *vs = &D_803EE6C0;
    s32 step = 0, x, z, rate_i;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802B49AC, 220);
    func_802B47D4();
    func_802A75DC((u8 *)RAM, &RAM_X, &RAM_Y, &RAM_Z, (u8 *)vs);
    func_802C4724(0x8F);
    if (VS_IN_SETUP(vs) == 0)
        func_802B4EF8(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    D_803F7840 = VS_SPEED(vs);
    func_802B5814(vs);
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802B57C4(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803EE78A == 0)
        func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), RAMDOZER_BRAKE, vs, &step);
    else
        D_803EE78A--;
    func_802A7FD8(D_803EE784, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_BULLDOZER, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), RAMDOZER_SLOPE_DIV, vs);
    if (D_803EE789 != 0)
        func_802A7070((s16 *)&D_803EE786, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &RAM_X, &RAM_Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &RAM_X, &RAM_Z, &RAM_Y, VEHICLE_BULLDOZER, RAMDOZER_SPAN_ALONG, RAMDOZER_SPAN_ACROSS,
                  VS_WHEELS(vs), VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(RAM, FRAME_BUF(RAM_BUF0, RAM_BUF1), OTHER_BUF(RAM_BUF0, RAM_BUF1));
    func_802B568C(vs);
    /* what it hits */
    func_8029A800(D_803EE768, D_803EE76C, D_803EE770, D_80305D10, 0, 1, 8, vs->unk76, 0x78, 0, 4, vs);
    func_8029C52C(VEHICLE_BULLDOZER, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = RAM;
        func_802BE77C(VEHICLE_BULLDOZER, vs);
        if (D_803A7424 == 0)
            D_803EE789 = 0;
        else
            ramdozer_bounce(vs);
    } else {
        D_803EE789 = 1;
        turn_along_wall(vs, &D_803EE786, RAMDOZER_WALL_TURN);
        func_802BCC10();
        D_803F77D0 = RAM;
        func_802BE77C(VEHICLE_BULLDOZER, vs);
    }
    PLAYER_FROM(RAM_X, RAM_Y, RAM_Z, vs, VEHICLE_BULLDOZER);
}

/* hd.c's: the Ramdozer each frame */
void func_802B49AC(void) {
    ramdozer_frame();
}

/* The parts a frame: the dust; a spark every other frame while turning;
   a sound and sparks starting off; the blade's lift (pad_lift); the
   tracks (with the speed, or turning on the spot with the stick); part 3
   from the heading; and part 1 (with a sound) once while it turns along a
   wall. */
REGS(gp)
void func_802B4EF8(VS *vs) {
    Part *p = RAM;
    s32 s;

    ENGINE_COST(802B4EF8, 109);
    func_802B54EC(vs);
    if (D_803EE788 != 0) {
        D_803EE788--;
    } else if (VS_TURNING(vs) != 0) {
        D_803EE788 = 1;
        if (func_802A5ED0() < 0xE)
            func_802A6274(T(D_802C2954), 0x30D40, 1, VEHICLE_BULLDOZER, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    }
    /* starting off (stopped at the last frame's start, moving now) */
    if (D_803F7840 == 0 && VS_SPEED(vs) != 0) {
        func_80260650(D_80367738, 0xA, NULL);
        func_802A6274(T(D_802C2954), 0x1D4C0, 1, VEHICLE_BULLDOZER, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    }
    D_803EE780 = pad_lift(vs, D_803EE780, 0x1E);
    func_802A0360(2, 0, p, D_803EE780);
    /* the tracks */
    s = VS_SPEED(vs);
    if (s != 0) {
        func_802A05F8(D_802C2190, s < 0 ? 1 : 0);
        func_802A05F8(D_802C21A4, s < 0 ? 1 : 0);
        s = (u32)iabs(s) >> 1;
        func_802A05D0(D_802C2190, s);
        func_802A05D0(D_802C21A4, s);
        func_802C4584((u32)s >> 2);
    } else {
        /* standing: turning on the spot with the stick, the tracks opposite */
        func_802A05F8(D_802C2190, STICK_X >= 0 ? 0 : 1);
        func_802A05F8(D_802C21A4, STICK_X >= 0 ? 1 : 0);
        func_802A05D0(D_802C2190, iabs(STICK_X));
        func_802A05D0(D_802C21A4, iabs(STICK_X));
    }
    /* part 3 from the heading (0x355..0x8AA) */
    s = (u16)VS_HEADING(vs);
    s = s >= 0x355 && s < 0x8AB ? s - 0x355 : 0;
    func_802A05A4(D_802C21B8, (u32)s / 0x4C, 0.0f);
    if (D_803EE789 != 0 && part_state(1, p) != 1) {
        func_802A039C(1, 9, p);
        func_802A040C(1, 1, p);
        func_802A0290(1, 2, p);
        func_80260650(D_80367738, 1, NULL);
    }
}

/* dust behind it while turning on soft ground (grip under 3, not the
   static triangles) with its back wheel down */
REGS(gp)
void func_802B54EC(VS *vs) {
    ENGINE_COST(802B54EC, 70);
    if (VS_TURNING(vs) != 0 && VS_AIRBORNE(vs)[2] != 1 && VS_GRIP(vs) < 3 && VS_ON_STATIC(vs) == 0)
        func_8027BE7C(3, VS_WHEEL_H(vs)[6], 0x190, -0x12C, -0x190, -0x12C, RAM_X, RAM_Z, VS_MOVE_HEADING(vs), 5, 0x32,
                      0x32, 0);
}

/* Ramdozer's matrix, its vertices and its collision */
REGS(gp)
void func_802B568C(VS *vs) {
    u8 *model = RAM_MODEL, *buf = FRAME_BUF(RAM_BUF0, RAM_BUF1);

    ENGINE_COST(802B568C, 70);
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(RAM_X, RAM_Y, RAM_Z, RAMDOZER_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(RAM_X, RAM_Y, RAM_Z, VEHICLE_BULLDOZER, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_BULLDOZER, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: RAMDOZER_STEER, or RAMDOZER_STEER_AIR with a wheel
   in the air */
REGS(gp -> s3)
s32 func_802B57C4(VS *vs) {
    ENGINE_COST(802B57C4, 19);
    return ANY_AIRBORNE(vs) ? RAMDOZER_STEER_AIR : RAMDOZER_STEER;
}

/* the physics' settings for Ramdozer: gravity, how its wheels land, and
   its turn rate (slower with B or Z) */
REGS(gp)
void func_802B5814(VS *vs) {
    ENGINE_COST(802B5814, 33);
    D_803EBBF4 = D_803EBBF0 * RAMDOZER_GRAVITY;
    D_803ED3F6 = RAMDOZER_BOUNCE_MIN;
    D_803ED3F7 = RAMDOZER_BOUNCE_DIV;
    D_803EE784 = PAD_B_OR_Z != 0 ? RAMDOZER_TURN_RATE_SLOW : RAMDOZER_TURN_RATE;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B589C(u8 *dst) {
    ENGINE_COST(802B589C, 7);
    func_802AC7DC(dst, (u8 *)&D_803EE6C0, (u32 *)&D_803EE768);
}

/* and back */
void func_802B58C8(u8 *src) {
    ENGINE_COST(802B58C8, 11);
    func_802AC85C(src, (u8 *)&D_803EE6C0, (u32 *)&D_803EE768);
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B4818(s32 carrier) {
    CARRY_KEEP(802B4818, 802B4854, 802B4874, 802B4880, &D_803EE6C0, D_803EE768, D_803EE770);
}

REGS(a3)
void func_802B48A4(s32 carrier) {
    CARRY_MOVE(802B48A4, 802B48E0, 802B48F4, 802B48FC, 802B495C, 802B4964, 802B4990, &D_803EE6C0, &D_803EE768, &D_803EE76C, &D_803EE770, 4, 0x258, 0x190,
               D_803ED40B = 1, func_802B5814(&D_803EE6C0), func_802B568C(&D_803EE6C0));
}
