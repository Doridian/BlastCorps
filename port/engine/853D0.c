/*
 * hd_code 853D0 (us.v11 0x802C9B90-0x802CB690): the Ballista
 * (VEHICLE_BIKE), as native C (engine.h, vehicle.h).  Its parts are
 * D_803F87A0, its state D_803F8AA0 and its position D_803F8B48..50.
 * func_802C9B90 sets it up (from the level loader), func_802CA4E0 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.  Parts 4 and 5 are its two missile launchers (it fires them
 * in turn while the missiles D_803F8B72 last), part 3 its lean, part 2 its
 * front wheel in a wheelie (D_803F8B64..70, a parabola), part 1 its wheels.
 *
 * func_802CA34C and func_802CA3D8, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the Ballista's .bss (asm/data/hd_code/853D0.bss.s) */
extern Part D_803F87A0[32];
extern VS D_803F8AA0;
extern s32 D_803F8B48, D_803F8B4C, D_803F8B50;  /* x, y, z */
extern u8 *PTR32 D_803F8B54;                    /* its model file */
extern u8 *PTR32 D_803F8B58;                    /* two 0x700-byte buffers, one per frame */
extern u8 *PTR32 D_803F8B5C;
extern f32 D_803F8B60;                          /* the lean, 0..1 (0.5 upright) */
extern s32 D_803F8B64, D_803F8B68, D_803F8B6C;  /* the wheelie: its speed, its frames, where it started */
extern u16 D_803F8B70;                          /* the wheelie's boosts so far */
extern u16 D_803F8B74;                          /* the heading it turns to against a wall (D_803A7425) */
extern u8 D_803F8B76;                           /* frames until the next sparks */
extern u8 D_803F8B77;
extern s8 D_803F8B78;                           /* turning to it */
extern s8 D_803F8B79;                           /* a sound to play this frame, or -1 */
extern u8 D_803F8B7A;                           /* frames without the throttle after a hit */
extern u8 D_803F8B7B;                           /* bounced (once, until it is clear) */
extern s8 D_803F8B7C;                           /* in a wheelie */
extern u8 D_803F8B7D;                           /* frames until the next missile */
extern u8 D_803F8B7E;                           /* the launcher that fires next */

#define BK D_803F87A0
#define X D_803F8B48
#define Y D_803F8B4C
#define Z D_803F8B50
#define MODEL D_803F8B54
#define BUF0 D_803F8B58
#define BUF1 D_803F8B5C

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern u8 D_80364A6B;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306420[];                         /* its parts' collision (56040's func_8029A800) */
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

void func_802CA308(void);
REGS(gp)
void func_802CAAFC(VS *vs);
REGS(gp)
void func_802CB224(VS *vs);
REGS(gp, s2 -> f2)
f32 func_802CB3C8(VS *vs, s32 up);
REGS(gp)
void func_802CB42C(VS *vs);
REGS(gp -> s3)
s32 func_802CB564(VS *vs);
void func_802CB5D8(void);

#define T(p) ((s32)(p))

/* ---- the Ballista's numbers (a frame, where it's per frame) -------------- */

#define BIKE_SCALE 0x3E80               /* its model's scale */
#define BIKE_SPAN_ALONG 0x280           /* its wheels' spans (func_802A8768) */
#define BIKE_SPAN_ACROSS 0x208
#define BIKE_BRAKE 0x10                 /* speed lost braking or against the stick, a frame */
#define BIKE_TURN_RATE 0x3E80           /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define BIKE_SLOPE_DIV 640.0f           /* func_802A843C: the slope's push is the height difference over this */
#define BIKE_STEER_DIV 2.2f             /* the steering rate: the speed over this ... */
#define BIKE_STEER_DIV_AIR 6.0f         /* ... or this with a wheel in the air */
#define BIKE_WALL_TURN 0.36f            /* func_802A71DC: turning along a wall, times the speed */
#define BIKE_HIT_FRAMES 5               /* frames without the throttle after hitting something */
#define BIKE_HIT_FWD_MIN 0x41           /* the speed it bounces back with: going forward, 0x41..0x82 */
#define BIKE_HIT_FWD_MAX 0x82
#define BIKE_HIT_BACK_MIN 0x25          /* going back, 0x25..0x4B */
#define BIKE_HIT_BACK_MAX 0x4B
#define BIKE_WALL_SOUND 0xC             /* against a wall, each frame */
#define BIKE_GRAVITY 4.0f               /* times the level's */
#define BIKE_BOUNCE_MIN 0x3C            /* a landing harder than this bounces ... */
#define BIKE_BOUNCE_DIV 2               /* ... at the speed over this */
#define BIKE_WHEEL_SPIN_DIV 6           /* the wheels' animation speed: the speed over this */
#define BIKE_LEAN_RATE 0.05f            /* the lean (part 3, D_803F8B60) a frame, to func_802CB3C8's limits */
#define BIKE_UPRIGHT 0.5f               /* and back to this */
#define BIKE_LEAN_SPEED 280.0f          /* the lean's limit: half the speed over this either way */
#define BIKE_MISSILE_WAIT 5             /* frames between missiles (D_803F8B7D) */
/* the wheelie: the front wheel's height t frames up is v t - BIKE_WHEELIE_G t^2
   (D_803F8B64 v, D_803F8B68 t, from D_803F8B6C), v BIKE_WHEELIE_V to start
   with; while the stick stays up (BIKE_WHEELIE_STICK), for BIKE_WHEELIE_HOLD
   frames at most, it starts again from where it is BIKE_WHEELIE_BOOST
   faster; landing faster than BIKE_WHEELIE_BOUNCE it bounces at half */
#define BIKE_WHEELIE_G 16.0f
#define BIKE_WHEELIE_V 0x3C
#define BIKE_WHEELIE_STICK 0x3C
#define BIKE_WHEELIE_HOLD 0x17
#define BIKE_WHEELIE_BOOST 0x33
#define BIKE_WHEELIE_BOUNCE 0x3C
#define BIKE_WHEELIE_TOP 3400.0f        /* the height part 2's frame is all the way up at */

/* the wheelie's height t frames up at speed v */
static s32 wheelie(s32 v, s32 t) {
    return (s32)((u32)v * (u32)t) + engine_cvt_w_s(-BIKE_WHEELIE_G * (f32)(s32)((u32)t * (u32)t));
}

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802C9B90(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F8AA0;
    s32 avg;

    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x700;
    D_80358070 += 0xE00;
    func_802A1388(VEHICLE_BIKE, 0, BUF0, BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x104, 0x140, -0x104, 0x140, 0x104, -0x140);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x12C, 0x154, -0x12C, 0x154, 0x12C, -0x154);
    X = x;
    Y = y;
    Z = z;
    D_803F8B7C = 0;
    D_803F8B7E = 0;
    D_803F8B60 = BIKE_UPRIGHT;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_BIKE, vs, 0, &avg);
    func_8029F85C(BK, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, BK);
    func_802A03D4(0, 0, BK);
    func_802A040C(0, 0, BK);
    func_802A0480(0, 0, BK, 0.0f);
    func_802A0290(0, 1, BK);
    func_8029E558(BK, BUF0, BUF1);
    func_802A0320(0, BK);
    func_802A0290(0, 1, BK);
    func_8029E558(BK, BUF1, BUF0);
    SET_GEARS(vs, -0x96, 0, 2, 0, 0xB4, 8, 0xB4, 0xC8, 4, 0xC8, 0xFA, 2, 0xFA, 0x104, 2);
    D_803F8B78 = 0;
    D_803F8B7B = 0;
    D_803F8B7A = 0;
    D_803F8B76 = 0;
    D_803F8B77 = 0x32;
    D_803F8B7D = 0;
    D_803F8B72 = 0;
    func_8029C354(VEHICLE_BIKE, MODEL_AT(model, 4), MODEL_AT(model, 8), BIKE_SCALE);
    func_80258230(VEHICLE_BIKE, 0x64, 0x2D, 0x2D);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802CA4E0();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
    D_80364A6B = 1;
}

/* a part's animation set: speed, direction, its third setting, and its
   play (func_802A0290's second argument) */
static void part_anim(s32 i, s32 speed, s32 dir, s32 c, s32 play) {
    func_802A039C(i, speed, BK);
    func_802A03D4(i, dir, BK);
    func_802A040C(i, c, BK);
    func_802A0290(i, play, BK);
}

/* hd.c's: the player gets in: the wheels', wheelie's, lean's and
   launchers' parts reset */
void func_802C9F54(void) {
    VS_TURNING(&D_803F8AA0) = 0;
    D_8036444C = 0xD48;
    D_80364450 = 1000;
    func_802C4310(0x94);
    part_anim(1, 0, 0, 0, -1);
    part_anim(2, 0, 0, 1, -1);
    part_anim(3, 0, 0, 1, -1);
    func_802A039C(4, 8, BK);
    func_802A03D4(4, 0, BK);
    func_802A040C(4, 0, BK);
    func_802A0480(4, 1, BK, 1.0f);
    func_802A039C(5, 8, BK);
    func_802A03D4(5, 0, BK);
    func_802A040C(5, 0, BK);
    func_802A0480(5, 1, BK, 1.0f);
}

/* hd.c's: whether it can be left: not with a wheel in the air or in a
   wheelie */
u8 func_802CA140(void) {
    return !ANY_AIRBORNE(&D_803F8AA0) && D_803F8B7C == 0;
}

/* hd.c's: the player gets out */
void func_802CA1AC(void) {
    VS_SPEED(&D_803F8AA0) = 0;
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x700);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802CA1F8(void) {
    VS *vs = &D_803F8AA0;

    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_BIKE, vs, 0);
    func_802CB42C(vs);
    func_802A133C(X, Y, Z, VEHICLE_BIKE, vs);
}

/* its light */
void func_802CA308(void) {
    func_802ABD54(VEHICLE_BIKE, X, Y, Z);
}

/* hit something: once (until a frame without), the throttle off for
   BIKE_HIT_FRAMES and the speed turned round, within BIKE_HIT_FWD_* going
   forward, BIKE_HIT_BACK_* going back; it stays where it is */
static void bike_bounce(VS *vs) {
    s32 v;

    D_803F8B78 = 0;
    if (D_803F8B7B != 0 || D_803F8B7A != 0)
        return;
    D_803F8B7A = BIKE_HIT_FRAMES;
    v = VS_SPEED(vs);
    if (v >= 0) {
        if (v < BIKE_HIT_FWD_MIN)
            v = BIKE_HIT_FWD_MIN;
        else if (v > BIKE_HIT_FWD_MAX)
            v = BIKE_HIT_FWD_MAX;
    } else {
        if (v > -BIKE_HIT_BACK_MIN)
            v = -BIKE_HIT_BACK_MIN;
        else if (v < -BIKE_HIT_BACK_MAX)
            v = -BIKE_HIT_BACK_MAX;
    }
    VS_SPEED(vs) = -v;
    D_803F8B7B = 1;
}

/* each frame */
void func_802CA4E0(void) {
    VS *vs = &D_803F8AA0;
    s32 step = 0, x, z, rate_i;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    func_802CA308();
    func_802A75DC((u8 *)BK, &X, &Y, &Z, (u8 *)vs);
    if (VS_IN_SETUP(vs) == 0)
        func_802CAAFC(vs);
    if (D_80367BFF != 0)
        func_802CB690(vs);
    func_802CB5D8();
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802CB564(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803F8B7A == 0)
        func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), BIKE_BRAKE, vs, &step);
    else
        D_803F8B7A--;
    func_802A7FD8(BIKE_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_BIKE, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), BIKE_SLOPE_DIV, vs);
    if (D_803F8B78 != 0)
        func_802A7070((s16 *)&D_803F8B74, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_BIKE, BIKE_SPAN_ALONG, BIKE_SPAN_ACROSS, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(BK, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802CB42C(vs);
    /* what it hits */
    func_8029A800(X, Y, Z, D_80306420, 1, 1, 8, VS_SPEED(vs), 0x64, 0, VEHICLE_BIKE, vs);
    func_8029C52C(VEHICLE_BIKE, vs);
    func_8029AA10();
    D_803F8B79 = BIKE_WALL_SOUND;
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = BK;
        func_802BE77C(VEHICLE_BIKE, vs);
        D_803F8B79 = -1;
        if (D_803A7424 == 0) {
            D_803F8B78 = 0;
            D_803F8B7B = 0;
        } else {
            bike_bounce(vs);
        }
    } else {
        D_803F8B78 = 1;
        turn_along_wall(vs, &D_803F8B74, BIKE_WALL_TURN);
        D_803F77D0 = BK;
        func_802BE77C(VEHICLE_BIKE, vs);
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_BIKE);
    if (D_803F8B79 >= 0)
        func_80260650(D_80367738, D_803F8B79, NULL);
}

/* The missiles (L or R, while there are some, every BIKE_MISSILE_WAIT + 1
   frames): from the two launchers in turn (parts 4 and 5, each between
   two of the model's points), as fast as the bike and more. */
static void bike_missile(void) {
    s32 *rec, sx, sy, sz, ex, ey, ez;

    if (D_803F8B7D != 0) {
        D_803F8B7D--;
        return;
    }
    if (PAD_L == 0 && PAD_R == 0)
        return;
    if (D_803F8B72 == 0)
        return;
    D_803F8B72--;
    func_80260650(D_80367738, 3, NULL);
    D_803F8B7E ^= 1;
    if (D_803F8B7E != 0) {
        func_802A0360(4, 0, BK, 0.0f);
        func_802A0290(4, 1, BK);
        rec = func_802ABC88(VEHICLE_BIKE, 3);
        sx = rec[0], sy = rec[1], sz = rec[2];
        rec = func_802ABC88(VEHICLE_BIKE, 4);
    } else {
        func_802A0360(5, 0, BK, 0.0f);
        func_802A0290(5, 1, BK);
        rec = func_802ABC88(VEHICLE_BIKE, 1);
        sx = rec[0], sy = rec[1], sz = rec[2];
        rec = func_802ABC88(VEHICLE_BIKE, 2);
    }
    ex = rec[0], ey = rec[1], ez = rec[2];
    func_80292288((u32)iabs(VS_SPEED(&D_803F8AA0)) * 0x140 / 0x104 + 0x280, ex, ey, ez, sx, sy, sz, 0, 0x1A4);
    D_803F8B7D = BIKE_MISSILE_WAIT;
}

/* The wheelie (the stick hard forward, not near the top speed): started at
   BIKE_WHEELIE_V, boosted while held; the front wheel's height (part 2)
   over the parabola, a hard landing bouncing it up at half, down when it
   lands otherwise. */
static void bike_wheelie(VS *vs) {
    s32 h, last, now;
    f32 f;

    if (func_802A7CB0(0xA, vs) == 0 && STICK_Y >= BIKE_WHEELIE_STICK) {
        if (D_803F8B7C == 0) {
            D_803F8B7C = 1;
            D_803F8B6C = 0;
            D_803F8B68 = 1;
            D_803F8B64 = BIKE_WHEELIE_V;
            D_803F8B70 = 0;
        } else if (D_803F8B70 < BIKE_WHEELIE_HOLD) {
            /* a higher one while the stick is held: the parabola started
               again from its height with the speed it has */
            D_803F8B70++;
            now = wheelie(D_803F8B64, D_803F8B68);
            D_803F8B6C = D_803F8B6C + now;
            last = wheelie(D_803F8B64, D_803F8B68 - 1);
            D_803F8B64 = now - last + BIKE_WHEELIE_BOOST;
            D_803F8B68 = 1;
        }
    }
    if (D_803F8B7C == 0) {
        func_802A0360(2, 0, BK, 0.0f);
        return;
    }
    now = wheelie(D_803F8B64, D_803F8B68);
    h = D_803F8B6C + now;
    if (h <= 0) {
        /* landed: a bounce, unless it was a small one or a wheel is off */
        last = iabs(wheelie(D_803F8B64, D_803F8B68 - 1) - now);
        if (last <= BIKE_WHEELIE_BOUNCE || ANY_AIRBORNE(vs)) {
            D_803F8B7C = 0;
            func_802A0360(2, 0, BK, 0.0f);
            return;
        }
        D_803F8B6C = 0;
        D_803F8B64 = (u32)last / 2;
        D_803F8B68 = 1;
        h = 0;
    }
    f = (f32)h / BIKE_WHEELIE_TOP;
    if (!(f <= 1.0f))
        f = 1.0f;
    func_802A0360(2, 0, BK, f);
    D_803F8B68++;
}

/* the wheels: sparks every other frame while turning (with room for
   them), their spin and the engine's sound; the dust; the lean (the 0x200
   and 0x100 buttons); the missiles; the wheelie */
REGS(gp)
void func_802CAAFC(VS *vs) {
    f32 f, g;

    if (D_803F8B76 != 0) {
        D_803F8B76--;
    } else if (VS_TURNING(vs) != 0) {
        D_803F8B76 = 1;
        if (func_802A5ED0() < 4)
            func_802A6274(T(D_802C2954), 0x30D40, 1, VEHICLE_BIKE, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0);
    }
    func_802A03D4(1, VS_SPEED(vs) < 0 ? 1 : 0, BK);
    func_802A039C(1, (u32)iabs(VS_SPEED(vs)) / BIKE_WHEEL_SPIN_DIV, BK);
    func_802C4584((u32)iabs(VS_SPEED(vs)) >> 3);
    func_802CB224(vs);
    /* the lean */
    f = D_803F8B60;
    if (PAD_LEFT != 0) {
        f = f - BIKE_LEAN_RATE;
        g = func_802CB3C8(vs, 0);
        if (f < g)
            f = g;
    } else if (PAD_RIGHT != 0) {
        f = f + BIKE_LEAN_RATE;
        g = func_802CB3C8(vs, 1);
        if (!(f <= g))
            f = g;
    } else {
        f = step_toward_f(f, BIKE_UPRIGHT, BIKE_LEAN_RATE);
    }
    D_803F8B60 = f;
    func_802A0360(3, 0, BK, f);
    bike_missile();
    bike_wheelie(vs);
}

/* dust behind it while turning on soft ground (grip under 3, not the
   static triangles) with its back wheel down */
REGS(gp)
void func_802CB224(VS *vs) {
    if (VS_TURNING(vs) != 0 && VS_AIRBORNE(vs)[2] != 1 && VS_GRIP(vs) < 3 && VS_ON_STATIC(vs) == 0)
        func_8027BE7C(3, VS_WHEEL_H(vs)[6], 0, -0x320, 0, -0x320, X, Z, VS_MOVE_HEADING(vs), 5, 0x28, 0, 1);
}

/* the lean's limit at this speed: upright less (or, `up`, more) half the
   speed over BIKE_LEAN_SPEED */
REGS(gp, s2 -> f2)
f32 func_802CB3C8(VS *vs, s32 up) {
    f32 f = (f32)iabs(VS_SPEED(vs)) / BIKE_LEAN_SPEED * 0.5f;

    return up != 0 ? f + BIKE_UPRIGHT : BIKE_UPRIGHT - f;
}

/* the Ballista's matrix, its vertices and its collision */
REGS(gp)
void func_802CB42C(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);

    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, BIKE_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(X, Y, Z, VEHICLE_BIKE, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_BIKE, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over BIKE_STEER_DIV, or over
   BIKE_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802CB564(VS *vs) {
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? BIKE_STEER_DIV_AIR : BIKE_STEER_DIV));
}

/* the physics' settings for the Ballista: gravity, and how its wheels
   land */
void func_802CB5D8(void) {
    D_803EBBF4 = D_803EBBF0 * BIKE_GRAVITY;
    D_803ED3F6 = BIKE_BOUNCE_MIN;
    D_803ED3F7 = BIKE_BOUNCE_DIV;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802CB634(u8 *dst) {
    vehicle_save(dst, (u8 *)&D_803F8AA0, (u32 *)&D_803F8B48, (u32 *)&D_803F8B4C, (u32 *)&D_803F8B50);
}

/* and back */
void func_802CB660(u8 *src) {
    vehicle_restore(src, (u8 *)&D_803F8AA0, (u32 *)&D_803F8B48, (u32 *)&D_803F8B4C, (u32 *)&D_803F8B50);
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802CA34C(s32 carrier) {
    CARRY_KEEP(&D_803F8AA0, D_803F8B48, D_803F8B50);
}

REGS(a3)
void func_802CA3D8(s32 carrier) {
    CARRY_MOVE(&D_803F8AA0, &D_803F8B48, &D_803F8B4C, &D_803F8B50, 0xA, 0x280, 0x208, D_803ED40B = 1, func_802CB5D8(), func_802CB42C(&D_803F8AA0));
}
