/*
 * hd_code 6C5E0 (us.v11 0x802B0DA0-0x802B29B0): Thunderfist
 * (VEHICLE_MAGOO), as native C (engine.h).  Its parts are D_803EDC10, its
 * state D_803EDF10 and its position D_803EDFB8..C0 (vehicle.h).
 * func_802B0DA0 sets it up (from the level loader), func_802B152C runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.
 *
 * It runs on two legs (parts 1 and 5, MAGOO_LEG the one in front) and rolls
 * into a ball (MAGOO_STATE, below); part 0x1F is the body's animation,
 * parts 7, 8 and 9 its idles.
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* Thunderfist's .bss (asm/data/hd_code/6C5E0.bss.s) */
extern Part D_803EDC10[32];
extern VS D_803EDF10;
extern s32 D_803EDFB8, D_803EDFBC, D_803EDFC0;  /* x, y, z */
extern u8 *PTR32 D_803EDFC4;                    /* its model file */
extern u8 *PTR32 D_803EDFC8;                    /* two 0x1400-byte buffers, one per frame */
extern u8 *PTR32 D_803EDFCC;
extern SndState *PTR32 D_803EDFD0;              /* its sound while it is in */
extern u16 D_803EDFD4;                          /* the heading it turns to against a wall (D_803A7425) */
extern s16 D_803EDFD6;                          /* the step sounds' last frame */
extern s8 D_803EDFD8;                           /* turning to it */
extern u8 D_803EDFD9;
extern u8 D_803EDFDA;                           /* facing the way the wall allows (within MAGOO_FACING) */

#define P D_803EDC10
#define X D_803EDFB8
#define Y D_803EDFBC
#define Z D_803EDFC0
#define MODEL D_803EDFC4
#define BUF0 D_803EDFC8
#define BUF1 D_803EDFCC

/* VehicleState's bytes Thunderfist uses for itself */
#define MAGOO_STATE(vs) ((vs)->unkA1)   /* MAGOO_* below */
#define MAGOO_LEG(vs) ((vs)->unkA2)     /* the leg in front: 0 part 1, 1 part 5 */
#define MAGOO_WALKING(vs) ((vs)->unkA3) /* its walk's animation is on (else standing) */
#define MAGOO_WALK 0                    /* walking */
#define MAGOO_CURL 1                    /* curling into the ball (part 0x1F's animation) */
#define MAGOO_ROLL 2                    /* rolling (part 2) */
#define MAGOO_CRASH 3                   /* crashed into something rolling (part 0x1F) */
#define MAGOO_GET_UP 4                  /* getting up (part 3) */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern u64 D_803649D8;                          /* this frame's random (osGetTime) */
extern u8 D_80370C35;                           /* 45BB0.c: the stick plays the buttons (A and B don't roll) */
extern u8 D_803F7804;                           /* it crashes through what it hits (77E20) */
extern SndState *PTR32 D_803F7844;              /* the rolling sound */
extern Part *PTR32 D_803F77D0;
extern u8 D_80305CF0[];                         /* its parts' collision (56040's func_8029A800) */
extern u8 D_802C2308[];                         /* its head (56040's list) */
extern u8 D_802C2984[];                         /* the dust's effect record (60F60) */

void func_802B14E8(void);
REGS(gp)
void func_802B18F4(VS *vs);
REGS(gp)
void func_802B2768(VS *vs);
REGS(gp -> s3)
s32 func_802B28B8(VS *vs);
void func_802B2900(void);

#define T(p) ((s32)(p))

/* ---- Thunderfist's numbers (a frame, where it's per frame) ---------------- */

#define MAGOO_SCALE 0x6590              /* its model's scale */
#define MAGOO_SPAN 0x78                 /* its feet's spans, both ways (func_802A8768) */
#define MAGOO_BRAKE 0xC                 /* speed lost braking or against the stick, a frame */
#define MAGOO_TURN_RATE 0x59D8          /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define MAGOO_SLOPE_DIV 120.0f          /* func_802A843C: the slope's push is the height difference over this */
#define MAGOO_STEER 0x78                /* the steering rate walking ... */
#define MAGOO_STEER_ROLL 0x32           /* ... and rolling (none standing) */
#define MAGOO_WALL_TURN 0.25f           /* func_802A71DC: turning along a wall, times the speed */
#define MAGOO_FACING 0x190              /* D_803EDFDA: this near the way the wall allows */
#define MAGOO_GRAVITY 4.0f              /* times the level's */
#define MAGOO_BOUNCE_MIN 0x28           /* a landing harder than this bounces ... */
#define MAGOO_BOUNCE_DIV 3              /* ... at the speed over this */
#define MAGOO_ROLL_MIN_SPEED 0x96       /* it curls up only this fast */
#define MAGOO_ROLL_SPEED 0x1BE          /* its speed rolling */
#define MAGOO_UNCURL_SPEED 0x3C         /* its speed (going forward) getting out of the ball */
#define MAGOO_STRIDE_SPEED 0x78         /* this fast, a step is always the right leg's */
#define MAGOO_LEG_SPEED_DIV 0x18        /* a leg's animation speed: the speed over this */
#define MAGOO_IDLE_CHANCE 0x1E          /* standing, an idle 1 frame in 31 */

/* part i's frame (func_802A04BC's v1), and its a0 (unk11) and t1 (unk13) */
static s32 part(s32 i, s32 *a0, s32 *t1) {
    s32 f12, f14, fC, fE, a0_, t1_;
    f32 f4;
    s32 r = func_802A04BC(i, P, &a0_, &f12, &f14, &fC, &fE, &t1_, &f4);

    if (a0)
        *a0 = a0_;
    if (t1)
        *t1 = t1_;
    return r;
}

/* part 0x1F (the body) on to animation `anim` */
static void body_anim(s32 anim) {
    func_802A039C(0x1F, anim, P);
    func_802A03D4(0x1F, 0, P);
    func_802A040C(0x1F, 0, P);
    func_802A0290(0x1F, 1, P);
}

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802B0DA0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EDF10;
    s32 avg;

    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x1400;
    D_80358070 += 0x2800;
    func_802A1388(VEHICLE_MAGOO, 1, BUF0, BUF1, model);
    func_802A754C(vs);
    /* one point (its feet), and three for what carries it */
    SET_WHEELS(VS_WHEELS(vs), 0, 0, 0, 0, 0, 0);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x50, 0x50, -0x50, 0x50, 0x50, -0x50);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    MAGOO_STATE(vs) = MAGOO_WALK;
    MAGOO_LEG(vs) = 0;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    MAGOO_WALKING(vs) = 1;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_MAGOO, vs, 0, &avg);
    func_8029F85C(P, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, P);
    func_802A03D4(0, 0, P);
    func_802A040C(0, 0, P);
    func_802A0480(0, 0, P, 0.0f);
    func_802A0290(0, 1, P);
    func_8029E558(P, BUF0, BUF1);
    func_802A0320(0, P);
    func_802A0290(0, 1, P);
    func_8029E558(P, BUF1, BUF0);
    SET_GEARS(vs, -0x64, 0, 4, 0, 0xA0, 4, 0, 0xA0, 4, 0, 0xA0, 4, 0, 0xA0, 4);
    D_803EDFD8 = 0;
    D_803EDFDA = 0;
    D_803EDFD9 = 0;
    func_8029C354(VEHICLE_MAGOO, MODEL_AT(model, 4), MODEL_AT(model, 8), MAGOO_SCALE);
    func_80258230(VEHICLE_MAGOO, 0x64, 0x2D, 0x2D);
    func_802A0360(1, 0, P, 0.0f);
    func_802A0290(1, 1, P);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802B152C();
    VS_IN_SETUP(vs) = 0;
    func_802A7764((u32 *)BUF1, (u32 *)BUF0, 0x1400);
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
    D_803F7844 = NULL;
}

/* hd.c's: whether it can be left: on the ground and walking */
u8 func_802B1150(void) {
    VS *vs = &D_803EDF10;

    return !ANY_AIRBORNE(vs) && MAGOO_STATE(vs) == MAGOO_WALK;
}

/* hd.c's: the player gets out */
void func_802B11B8(void) {
    VS_SPEED(&D_803EDF10) = 0;
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x1400);
    func_802A02E4(0x1F, P);
    func_802C444C();
    func_802608C8(D_803EDFD0);
}

/* hd.c's: the player gets in: its sound, its head and its arms */
void func_802B1228(void) {
    VS_TURNING(&D_803EDF10) = 0;
    D_8036444C = 0x1068;
    D_80364450 = 0xBB8;
    func_80260650(D_80367738, 0x50, &D_803EDFD0);
    func_802A05D0(D_802C2308, 0x19);
    func_802A05F8(D_802C2308, 0);
    func_802A0620(D_802C2308, 0);
    func_802A039C(7, 2, P);
    func_802A040C(7, 1, P);
    func_802A0480(7, 1, P, 0.5f);
    func_802A039C(8, 2, P);
    func_802A040C(8, 1, P);
    func_802A0480(8, 1, P, 0.5f);
    func_802A039C(9, 4, P);
    func_802A040C(9, 1, P);
    func_802A0480(9, 1, P, 0.5f);
    func_802A0480(1, 1, P, 0.5f);
    func_802A0480(5, 1, P, 0.5f);
    func_802A0480(3, 0, P, 0.0f);
}

/* hd.c's: put back on the ground where it is */
void func_802B13D8(void) {
    VS *vs = &D_803EDF10;

    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_MAGOO, vs, 0);
    func_802B2768(vs);
    func_802A133C(X, Y, Z, VEHICLE_MAGOO, vs);
}

/* its light */
void func_802B14E8(void) {
    func_802ABD54(VEHICLE_MAGOO, X, Y, Z);
}

/* each frame */
void func_802B152C(void) {
    VS *vs = &D_803EDF10;
    s32 step = 0, x, z, rate_i, turn, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    func_802B14E8();
    if (VS_IN_SETUP(vs) == 0)
        func_802B18F4(vs);
    func_802B2900();
    /* steering, the throttle (the stick let go: stopping, walking), the
       turn and the slope */
    rate_i = func_802B28B8(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), MAGOO_BRAKE, vs, &step);
    if (MAGOO_STATE(vs) == MAGOO_WALK)
        func_802A77D0(vs);
    func_802A7FD8(MAGOO_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 0, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 0, VEHICLE_MAGOO, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), MAGOO_SLOPE_DIV, vs);
    if (D_803EDFD8 != 0)
        func_802A7070((s16 *)&D_803EDFD4, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    D_803ED40B = 0;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_MAGOO, MAGOO_SPAN, MAGOO_SPAN, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(P, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802B2768(vs);
    /* what it hits */
    func_8029A800(D_803EDFB8, D_803EDFBC, D_803EDFC0, D_80305CF0, 0, 0, 0, vs->unk76, 0, 0, 2, vs);
    func_8029C52C(VEHICLE_MAGOO, vs);
    func_8029AA10();
    D_803F77D0 = P;
    func_802BE77C(VEHICLE_MAGOO, vs);
    if (D_803A7425 == 0) {
        D_803EDFD8 = 0;
        D_803EDFDA = 0;
    } else {
        /* against a wall: whether it faces the way the wall allows, then
           turned along it (turn_along_wall's steps, the check between) */
        func_8029A914(vs);
        D_803EDFD8 = 1;
        h = (u16)VS_MOVE_HEADING(vs) - ANGLE_HALF;
        if (h < 0)
            h += ANGLE_WRAP;
        h = iabs(h - func_802A6F6C());
        if (h > ANGLE_HALF)
            h = ANGLE_WRAP - h;
        D_803EDFDA = h < MAGOO_FACING;
        func_802A70D8(vs);
        D_803EDFD4 = func_802A71DC(VS_MOVE_HEADING(vs), VS_HEADING(vs), MAGOO_WALL_TURN, vs, &turn);
        VS_MOVE_HEADING(vs) = D_803EDFD4;
        VS_TURN_HEADING(vs) = D_803EDFD4;
        func_802A746C(turn, vs);
        func_802A6FE4(0, vs);
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_MAGOO);
}

/* the first of the n parts that is animating handed over to idle `to`
   (stopped and `to`'s frame set from it, in the original's order of the
   two), or, none animating, the first started at frame 0 and handed over */
static void hand_over(const s32 *parts, s32 n, s32 to, s32 stop_first) {
    s32 k;

    for (k = 0; k < n; k++) {
        if (part(parts[k], NULL, NULL) != 0) {
            if (stop_first) {
                func_802A02E4(parts[k], P);
                func_8029F9D4(parts[k], to, P);
            } else {
                func_8029F9D4(parts[k], to, P);
                func_802A02E4(parts[k], P);
            }
            return;
        }
    }
    func_802A0360(parts[0], 0, P, 0.0f);
    func_8029F9D4(parts[0], to, P);
}

/* standing: once, the walk's animation (the body's, then the legs')
   handed over to idle 7; then now and then (1 in MAGOO_IDLE_CHANCE + 1) a
   random idle (7, 8 or 9) when none plays */
static void stand(VS *vs) {
    s32 r;

    if (MAGOO_WALKING(vs) != 0) {
        func_802A0360(7, 0, P, 0.0f);
        if (part(0x1F, NULL, NULL) != 0) {
            func_802A02E4(0x1F, P);
            func_8029F9D4(0x1F, 7, P);
        } else {
            s32 legs[2] = { 1, 5 };

            hand_over(legs, 2, 7, 1);
        }
        body_anim(0x1E);
    }
    MAGOO_WALKING(vs) = 0;
    if (part(0x1F, NULL, NULL) == 1 || part(7, NULL, NULL) == 1 || part(8, NULL, NULL) == 1 ||
        part(9, NULL, NULL) == 1)
        return;
    if (func_8026A8E0(0, MAGOO_IDLE_CHANCE) != 0)
        return;
    r = func_8026A8E0(0, 2);
    r = r == 0 ? 7 : r == 1 ? 8 : 9;
    func_802A0360(r, 0, P, 0.0f);
    func_802A0290(r, 1, P);
}

/* a leg's animation's direction and speed by the speed */
static void leg_speed(s32 leg) {
    s32 s = VS_SPEED(&D_803EDF10);

    func_802A03D4(leg, s < 0 ? 1 : 0, P);
    func_802A039C(leg, (u32)iabs(s) / MAGOO_LEG_SPEED_DIV, P);
}

/* Walking: once, the idles handed over to the walk; then, the body's
   animation done, curling into the ball (L or R, or A or B unless
   D_80370C35, at MAGOO_ROLL_MIN_SPEED or more), or the step of the leg in
   front (dust now and then); its step done, the next one: the right leg's
   (part 5) at MAGOO_STRIDE_SPEED or more, else the left's. */
static void walk(VS *vs) {
    s32 a0, leg, idles[3] = { 7, 8, 9 };

    if (MAGOO_WALKING(vs) != 1) {
        func_802A0360(1, 0, P, 0.0f);
        hand_over(idles, 3, 1, 0);
        body_anim(0x28);
    }
    MAGOO_WALKING(vs) = 1;
    if (part(0x1F, NULL, NULL) == 1)
        return;
    D_803F7804 = 0;
    if (((D_80370C35 == 0 && (PAD_A != 0 || PAD_B != 0)) || PAD_L != 0 || PAD_R != 0) &&
        VS_SPEED(vs) >= MAGOO_ROLL_MIN_SPEED) {
        /* curling up */
        MAGOO_STATE(vs) = MAGOO_CURL;
        func_802A0360(2, 0, P, 0.0f);
        if (MAGOO_LEG(vs) == 0) {
            func_8029F9D4(1, 2, P);
            func_802A02E4(1, P);
        } else {
            if (MAGOO_LEG(vs) != 1)
                engine_trap(0x802B1EB0);
            func_8029F9D4(5, 2, P);
            func_802A02E4(5, P);
        }
        body_anim(0x23);
        func_80278EB0(6, 0.1f, 100);
        return;
    }
    if (((u32)D_803649D8 >> 8 & 0x2F) == 0)
        func_802A6274(T(D_802C2984), 0xEA60, 1, VEHICLE_MAGOO, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    if (MAGOO_LEG(vs) != 0 && MAGOO_LEG(vs) != 1)
        engine_trap(0x802B1FCC);
    for (leg = MAGOO_LEG(vs) == 0 ? 1 : 5;; ) {
        if (part(leg, &a0, NULL) != 0) {
            leg_speed(leg);
            return;
        }
        leg = iabs(VS_SPEED(vs)) >= MAGOO_STRIDE_SPEED ? 5 : 1;
        func_802A0290(leg, a0 != 0 ? 2 : 1, P);
        func_802A0360(leg, 0, P, 0.0f);
        MAGOO_LEG(vs) = leg == 5;
    }
}

/* getting out of the ball: forward at MAGOO_UNCURL_SPEED, the body's
   animation 0x32, walking again (`part` the ball's part, stopped, or -1) */
static void uncurl(VS *vs, s32 ball, s32 sound_first) {
    if (sound_first)
        func_802C444C();
    if (VS_SPEED(vs) >= 0)
        VS_SPEED(vs) = MAGOO_UNCURL_SPEED;
    if (!sound_first)
        func_802C444C();
    func_802A0360(1, 0, P, 0.0f);
    if (ball >= 0) {
        func_802A02E4(ball, P);
        func_8029F9D4(ball, 1, P);
    } else {
        func_8029F9D4(0x1F, 1, P);
    }
    body_anim(0x32);
    func_802794A4();
    MAGOO_STATE(vs) = MAGOO_WALK;
}

/* crashed rolling into something (VS unk9C): the speed halved, the
   crash's animation, through what it hits (D_803F7804) */
static void crash(VS *vs, s32 ball) {
    VS_SPEED(vs) = VS_SPEED(vs) >> 1;
    if (ball < 0)
        func_802794A4();
    MAGOO_STATE(vs) = MAGOO_CRASH;
    func_802A0360(3, 3, P, 0.0f);
    if (ball < 0) {
        func_8029F9D4(0x1F, 3, P);
    } else {
        func_8029F9D4(ball, 3, P);
        func_802A02E4(ball, P);
    }
    body_anim(0x21);
    D_803F7804 = 1;
}

/* The parts and the states a frame.  Walking: the step sounds (the legs'
   frames 2 and 6), then standing or walking.  Curling and rolling: at
   MAGOO_ROLL_SPEED, crashing into what it hits (unk9C), uncurling on
   unk9D or facing along a wall; curling done, rolling.  Crashed: the
   sound and dust; its animation done, getting up; that done, walking. */
REGS(gp)
void func_802B18F4(VS *vs) {
    s32 t1, last;

    switch (MAGOO_STATE(vs)) {
    case MAGOO_WALK:
        if (part(1, NULL, &t1) == 1 || part(5, NULL, &t1) == 1) {
            last = D_803EDFD6;
            D_803EDFD6 = t1;
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
    case MAGOO_CURL:
        if (D_803F7844 == NULL)
            func_80260650(D_80367738, 0x51, &D_803F7844);
        if (vs->unk9C != 0) {
            crash(vs, -1);
        } else if (vs->unk9D != 0 || D_803EDFDA != 0) {
            uncurl(vs, -1, 0);
        } else {
            VS_SPEED(vs) = MAGOO_ROLL_SPEED;
            if (part(0x1F, NULL, NULL) != 1) {
                func_802A039C(2, 0xA, P);
                func_802A03D4(2, 0, P);
                func_802A040C(2, 0, P);
                func_802A0290(2, 1, P);
                MAGOO_STATE(vs) = MAGOO_ROLL;
            }
        }
        break;
    case MAGOO_ROLL:
        if (vs->unk9C != 0) {
            crash(vs, 2);
        } else if (vs->unk9D != 0 || D_803EDFDA != 0) {
            uncurl(vs, 2, 1);
        } else {
            VS_SPEED(vs) = MAGOO_ROLL_SPEED;
            if (part(2, NULL, NULL) != 1)
                uncurl(vs, 2, 1);
        }
        break;
    case MAGOO_CRASH:
        if (D_803F7844 != NULL) {
            func_802C444C();
            func_80260650(D_80367738, 0x4B, NULL);
        }
        func_802A6274(T(D_802C2984), 0x222E0, 1, VEHICLE_MAGOO, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
        func_802BCC10();
        if (part(0x1F, NULL, NULL) != 1) {
            D_803F7804 = 0;
            func_802794A4();
            func_802A039C(3, 8, P);
            func_802A03D4(3, 0, P);
            func_802A040C(3, 0, P);
            func_802A0290(3, 1, P);
            MAGOO_STATE(vs) = MAGOO_GET_UP;
        }
        break;
    case MAGOO_GET_UP:
        func_802A6274(T(D_802C2984), 0x222E0, 1, VEHICLE_MAGOO, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
        func_802BCC10();
        if (part(3, NULL, NULL) != 1) {
            D_803F7804 = 1;
            func_802A0360(1, 0, P, 0.0f);
            func_802A0360(5, 0, P, 0.0f);
            MAGOO_STATE(vs) = MAGOO_WALK;
        }
        break;
    default:
        engine_trap(0x802B192C);
        break;
    }
}

/* Thunderfist's matrix (turned a quarter), its vertices and its collision */
REGS(gp)
void func_802B2768(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);
    s32 h = (u16)VS_HEADING(vs) + ANGLE_QUARTER;

    if (h >= ANGLE_TURN)
        h -= ANGLE_WRAP;
    D_803ED390[1] = h;
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    func_802AA764(X, Y, Z, MAGOO_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(X, Y, Z, VEHICLE_MAGOO, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_MAGOO, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: none standing, MAGOO_STEER_ROLL in the ball, else
   MAGOO_STEER */
REGS(gp -> s3)
s32 func_802B28B8(VS *vs) {
    if (VS_SPEED(vs) == 0)
        return 0;
    return MAGOO_STATE(vs) == MAGOO_ROLL || MAGOO_STATE(vs) == MAGOO_CURL ? MAGOO_STEER_ROLL : MAGOO_STEER;
}

/* the physics' settings for Thunderfist: gravity, and how it lands */
void func_802B2900(void) {
    D_803EBBF4 = D_803EBBF0 * MAGOO_GRAVITY;
    D_803ED3F6 = MAGOO_BOUNCE_MIN;
    D_803ED3F7 = MAGOO_BOUNCE_DIV;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B295C(u8 *dst) {
    func_802AC7DC(dst, (u8 *)&D_803EDF10, (u32 *)&D_803EDFB8);
}

/* and back */
void func_802B2988(u8 *src) {
    func_802AC85C(src, (u8 *)&D_803EDF10, (u32 *)&D_803EDFB8);
}
