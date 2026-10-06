/*
 * hd_code 69BB0 (us.v11 0x802AE370-0x802AFC50): the driver on foot (type
 * 0), as native C (engine.h).  The driver's parts are D_803ED460, its
 * state D_803ED760 and its position D_803ED808..10 (vehicle.h).
 *
 * func_802AE370 sets it up (from the level loader), func_802AEEC8 runs it
 * each frame (from hd.c, and at the end of the setup); func_802AE888 gets
 * the driver out of a vehicle, at the first clear spot beside it
 * (func_802AEC3C), and func_802AF340 walks it out from there.
 *
 * Its shadow (func_802AF340) passes func_802582C4 three stack arguments it
 * never stores: two are the halves of the return address its own frame
 * saved (RA_SHADOW, the shadow's heading), the third whatever was below
 * that frame (its tilt): SHADOW_TILT, what the original finds there all
 * but a few frames.  Getting out, its `self` is the driver and its
 * material 0 where the original passed what $t8 and $fp held (docs/PORT.md,
 * "Garbage made values").
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

extern Part D_803ED460[32];
extern VS D_803ED760;
extern s32 D_803ED814;                          /* the coordinate it walks out to */
extern s32 D_803ED81C;                          /* the ground's height under it */
extern s16 D_803ED820;                          /* part 1's frame, last time */
extern u16 D_803ED822;                          /* the heading it turns to against a wall (D_803A7425) */
extern s8 D_803ED824;                           /* turning to D_803ED822 */
extern u8 D_803ED825;                           /* it can get out */
extern u8 D_803F7812;                           /* set on getting out (not by us.v10) */
extern u8 D_803ED827;                           /* getting out (func_802AE888) */
extern u8 D_803ED828;                           /* the side it gets out on: 0 +z, 1 -z, 2 +x, 3 -x */
extern u8 *PTR32 D_803ED82C;                    /* two 0xC80-byte buffers, one per frame */
extern u8 *PTR32 D_803ED830;
extern s32 D_803ED3A8[];

extern u8 D_80305CB0[];                         /* its parts' collision (56040's func_8029A800) */
extern s8 D_80305CB1[];                         /* where to get out of a vehicle: level, vehicle, side, distance, a test */
extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern Part *PTR32 D_803F77D0;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_802582C4(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
s32 func_8026A8E0(s32 lo, s32 hi);
void func_8028F994(s32 x, s32 y, s32 z);
REGS(t3, t6, t7, s0, s1, s2, s3, s4, gp -> t2, t3, s3)
s32 func_802A7834(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, s32 rate, u16 *h, VS *vs,
                  s32 *step_out, s32 *rate_out);

void func_802AEEC8(void);
static void driver_frame(void);
REGS()
void func_802AEE84(void);
REGS(gp)
void func_802AF340(VS *vs);
REGS(gp)
void func_802AF4BC(VS *vs);
REGS(gp)
void func_802AFA64(VS *vs);
REGS(-> s3)
s32 func_802AFB84(void);
REGS()
void func_802AFBA0(void);
REGS(a3 -> a1)
s32 func_802AEB9C(s32 test);
REGS(a2, gp -> a3)
s32 func_802AEC3C(s32 d, VS *vs);

#define T(p) ((s32)(p))
#define DRV D_803ED460
#define MODEL ((u8 *)D_803ED818)
#define X D_803ED808
#define Y D_803ED80C
#define Z D_803ED810
#define BUF0 D_803ED82C
#define BUF1 D_803ED830

/* VehicleState's byte the driver uses for itself */
#define DRV_WALKING(vs) ((vs)->unkA1)  /* its walk's animation is on (else standing) */

/* ---- the driver's numbers (a frame, where it's per frame) ---------------- */

#define DRIVER_SCALE 0x4E20             /* its model's scale */
#define DRIVER_SPAN 0x3C                /* its feet's spans, both ways (func_802A8768) */
#define DRIVER_BRAKE 6                  /* speed lost braking or against the stick, a frame */
#define DRIVER_TURN_RATE 0x2328         /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define DRIVER_SLOPE_DIV 60.0f          /* func_802A843C: the slope's push is the height difference over this */
#define DRIVER_STEER 0x8C               /* the steering rate */
#define DRIVER_WALL_TURN 1.0f           /* func_802A71DC: turning along a wall, times the speed */
#define DRIVER_GRAVITY 4.0f             /* times the level's */
#define DRIVER_BOUNCE_MIN 0x28          /* a landing harder than this bounces ... */
#define DRIVER_BOUNCE_DIV 3             /* ... at the speed over this */
#define DRIVER_OUT_STEP 0x64            /* getting out: the step it walks a frame ... */
#define DRIVER_OUT_SPEED 0x50           /* ... at this speed (for its legs) */
#define DRIVER_SPOT_STEP 0x64           /* the spots beside the vehicle it tries, this far apart */
#define DRIVER_BARGE_LIFT 0x1F4         /* out of a barge it starts this much higher */
#define DRIVER_IDLE_CHANCE 0x14         /* standing, a look round 1 frame in 21 */
#define DRIVER_LEG_SPEED_DIV 11         /* its legs' animation speed: the speed over this */

/* where the shadow's frame saved its return address, in func_802AEEC8:
   each version's own (the shadow's heading) */
#if defined(VERSION_US_V10)
#define RA_SHADOW 0x802AEEBC
#elif defined(VERSION_JP)
#define RA_SHADOW 0x802AF2C0
#elif defined(VERSION_EU)
#define RA_SHADOW 0x802B18C0
#else
#define RA_SHADOW 0x802AEF50
#endif

/* The shadow's tilt (func_802582C4 keeps its low half as an angle): the
   original reads the word 0xBC below the game C's N64 $sp, dead stack that
   72B80's func_802ABBEC call under the chopper leaves &D_803ED3B0 in (762
   of the TAS's 802 reads), the carrying's func_802AB714 its $ra (38), or
   func_802AEC3C its $t6 (2).  The first's low half for all of them: about
   83 degrees, the shadow drawn edge-on as nearly always. */
#define SHADOW_TILT 0x803ED3B0


/* part i's state: func_802A04BC's first result (and its sixth, the
   frame) */
static s32 part_state(s32 i, Part *parts, s32 *f13) {
    s32 f11, f12, f14, fC, fE, t1;
    f32 f4;
    s32 r = func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &t1, &f4);

    if (f13)
        *f13 = t1;
    return r;
}

/* the sides it gets out on (D_803ED828): 0 +z, 1 -z, 2 +x, 3 -x */
static void side_step(s32 side, s32 d, s32 *x, s32 *z) {
    switch (side) {
    case 0: *z += d; break;
    case 1: *z -= d; break;
    case 2: *x += d; break;
    default: *x -= d; break;
    }
}

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802AE370(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803ED760;
    s32 avg, k;

    D_803ED818 = (VehicleModel *)model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0xC80;
    D_80358070 += 0x1900;
    func_802A1388(VEHICLE_DRIVER, 0, BUF0, BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x1E, 0x1E, -0x1E, 0x1E, 0x1E, -0x1E);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x23, 0x23, -0x23, 0x23, 0x23, -0x23);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    D_803ED825 = 1;
    D_803ED826 = 0;
    D_803ED827 = 0;
    D_803ED824 = 0;
    DRV_WALKING(vs) = 0;
    /* (its material where no ground is found: 0, where the original has
       the $fp the level loader left) */
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_DRIVER, vs, 0,
                  &avg);
    func_8029F85C(DRV, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, DRV);
    func_802A03D4(0, 0, DRV);
    func_802A040C(0, 0, DRV);
    func_802A0480(0, 0, DRV, 0.0f);
    func_802A0290(0, 1, DRV);
    func_8029E558(DRV, BUF0, BUF1);
    func_802A0320(0, DRV);
    func_802A0290(0, 1, DRV);
    func_8029E558(DRV, BUF1, BUF0);
    SET_GEARS(vs, -0x28, 0, 2, 0, 0x28, 2, 0x28, 0x3C, 2, 0x3C, 0x50, 2, 0x50, 0x64, 2);
    func_8029C354(VEHICLE_DRIVER, MODEL_AT(MODEL, 4), MODEL_AT(MODEL, 8), DRIVER_SCALE);
    func_80258230(VEHICLE_DRIVER, 0x28, 0xF, 0xF);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    driver_frame();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
    /* parts 1 (the legs) to 4 at rest */
    func_802A039C(1, 0, DRV);
    func_802A03D4(1, 0, DRV);
    func_802A040C(1, 0, DRV);
    func_802A0480(1, 1, DRV, 0.5f);
    for (k = 2; k <= 4; k++) {
        func_802A039C(k, 2, DRV);
        func_802A03D4(k, 0, DRV);
        func_802A040C(k, 1, DRV);
        func_802A0480(k, 1, DRV, 0.5f);
    }
    D_8036444C = 0xD48;
    D_80364450 = 0;
}

/* hd.c's: part 0x1F (the run) stopped */
void func_802AE860(void) {
    func_802A02E4(0x1F, DRV);
}

/* hd.c's: the player gets out of the vehicle at distance `dist` (times the
   table's): the spots beside it on the table's side (D_80305CB1: level,
   vehicle, side, distance, a test), every DRIVER_SPOT_STEP and then `dist`
   itself, must all be clear (func_802AEC3C); then it faces that side and
   walks out from where the vehicle is.  Whether it could. */
u8 func_802AE888(s32 dist) {
    VS *vs = &D_803ED760;
    s8 *t;
    s32 side = 0, mul = 1, d, r = 0, h;

#ifndef VERSION_US_V10
    D_803F7812 = 1;
#endif
    if (D_803ED825 == 0)
        goto done;
    for (t = D_80305CB1; t[0] != -1; t += 5) {
        if (t[0] == D_802E8BDC && (u8)t[1] == D_80364456 && (t[4] == 0 || func_802AEB9C((u8)t[4]) != 0)) {
            side = (u8)t[2];
            mul = (u8)t[3];
            break;
        }
    }
    dist = (u32)dist * (u32)mul;
    D_803ED828 = side;
    D_803ED827 = 1;
    D_803ED81C = D_803643E4;
    for (d = 0; d <= dist; d += DRIVER_SPOT_STEP)
        if (func_802AEC3C(d, vs) == 0)
            goto done;
    if (func_802AEC3C(dist, vs) == 0)
        goto done;
    VS_AIRBORNE(vs)[0] = 0, VS_AIRBORNE(vs)[1] = 0, VS_AIRBORNE(vs)[2] = 0;
    VS_TURNING(vs) = 0;
    VS_SPEED(vs) = 0;
    side = D_803ED828;
    h = side == 0 ? 0 : side == 1 ? ANGLE_HALF : side == 2 ? ANGLE_QUARTER : ANGLE_HALF + ANGLE_QUARTER;
    VS_MOVE_HEADING(vs) = h;
    VS_HEADING(vs) = h;
    VS_TURN_HEADING(vs) = h;
    /* where it walks to (along the side's axis), from where the vehicle is
       (a barge's deck higher) */
    D_803ED814 = side == 0 || side == 1 ? Z : X;
    X = D_803643E0;
    h = D_803643E4;
    if (D_80364456 == VEHICLE_BARGE || D_80364456 == VEHICLE_BARGE_2 || D_80364456 == VEHICLE_BARGE_3)
        h += DRIVER_BARGE_LIFT;
    Y = h;
    Z = D_803643E8;
    D_803ED826 = 1;
    func_802A039C(1, 0, DRV);
    func_802A03D4(1, 0, DRV);
    func_802A040C(1, 0, DRV);
    func_802A0290(1, -1, DRV);
    D_8036444C = 0xD48;
    D_80364450 = 0;
    r = 1;
done:
    D_803ED827 = 0;
    return r;
}

/* the table's test for a spot (D_80305CB1's fifth byte): 1, the player's z
   (>> 5) in 0xCCD..0xDB4; 2, its x in 0x834..0x960 and z in 0x4B0..0x5DC */
REGS(a3 -> a1)
s32 func_802AEB9C(s32 test) {
    s32 x = D_803643E0 >> 5, z = D_803643E8 >> 5;

    if (test == 1)
        return z >= 0xCCD && z <= 0xDB4;
    if (test == 2)
        return x >= 0x834 && x <= 0x960 && z >= 0x4B0 && z <= 0x5DC;
    return 0;
}

/* the spot `d` from the vehicle on the side D_803ED828 says: the driver
   put there on the ground, and whether nothing is in the way (the
   original saves and loads back $v0, $v1, $a0, $a2, $t0..$t9 and $gp) */
REGS(a2, gp -> a3)
s32 func_802AEC3C(s32 d, VS *vs) {
    s32 r;

    X = D_803643E0;
    Z = D_803643E8;
    side_step(D_803ED828, d, &X, &Z);
    /* (its `self` the driver and its material 0: the original's are
       whatever $t8 and $fp held) */
    func_802A9A60(VS_WHEELS(vs), D_803ED81C, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_DRIVER, vs,
                  0);
    func_802AFA64(vs);
    D_803ED81C = (u32)(VS_WHEEL_H(vs)[3] + VS_WHEEL_H(vs)[6]) >> 1;
    func_8029A800(D_803ED808, D_803ED80C, D_803ED810, D_80305CB0, 0, 0, 0, 0, 0, 0, 0, vs);
    func_8029AA10();
    D_803F77D0 = DRV;
    func_802BE77C(VEHICLE_DRIVER, vs);
    if (D_80364456 != VEHICLE_BARGE)
        func_8028F994(X, Y, Z);
    r = D_803A7424 == 0;
    return r;
}

/* its light */
REGS()
void func_802AEE84(void) {
    func_802ABD54(VEHICLE_DRIVER, X, Y, Z);
}

/* hd.c's: each frame */
void func_802AEEC8(void) {
    driver_frame();
}

/* the game's mode, or the one it is going to */
static u64 game_mode(void) {
    return D_80364A98 != 0 ? D_80364A98 : D_80364A90;
}

/* each frame: its walk (not in the level's intro, its flight in or the
   chopper's: the modes 1, 0x800 and 0x1000), or getting out
   (func_802AF340); then, unless getting out or in those modes, what it
   hits */
static void driver_frame(void) {
    VS *vs = &D_803ED760;
    s32 step = 0, x, z, rate_i;
    f32 rate;
    u64 mode;

    func_802AEE84();
    if (VS_IN_SETUP(vs) == 0)
        func_802AF4BC(vs);
    if (VS_IN_SETUP(vs) == 0 && D_803ED826 != 0) {
        func_802AF340(vs);
    } else {
        func_802AFBA0();
        rate_i = func_802AFB84();
        mode = game_mode();
        if (mode != 0x800 && mode != 1 && mode != 0x1000) {
            /* steering, the throttle (the stick let go: stopping, walking),
               the turn and the slope */
            func_802A7834(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), DRIVER_BRAKE, rate_i,
                          &VS_HEADING(vs), vs, &step, &rate_i);
            if (DRV_WALKING(vs) == 1)
                func_802A77D0(vs);
            func_802A7FD8(DRIVER_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs),
                          &VS_MOVE_HEADING(vs), (s8 *)&VS_TURNING(vs), 0, vs);
            rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
            func_802A843C(&VS_SPEED(vs), 1, VEHICLE_DRIVER, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), DRIVER_SLOPE_DIV,
                          vs);
            if (D_803ED824 != 0)
                func_802A7070((s16 *)&D_803ED822, vs);
            /* the move, on the ground */
            x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
            D_803ED40B = 0;
            func_802A8768(x, z, &X, &Z, &Y, VEHICLE_DRIVER, DRIVER_SPAN, DRIVER_SPAN, VS_WHEELS(vs),
                          VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
        }
    }
    func_8029E558(DRV, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802AFA64(vs);
    if (D_803ED826 == 0) {
        if (game_mode() & 0x1801) {
            D_803ED824 = 0;
        } else {
            /* what it hits */
            func_8029A800(D_803ED808, D_803ED80C, D_803ED810, D_80305CB0, 0, 0, 0, vs->unk76, 0, 0, 0, vs);
            func_8029C52C(VEHICLE_DRIVER, vs);
            func_8029AA10();
            D_803F77D0 = DRV;
            func_802BE77C(VEHICLE_DRIVER, vs);
            if (D_803A7425 == 0) {
                D_803ED824 = 0;
            } else {
                D_803ED824 = 1;
                turn_along_wall(vs, &D_803ED822, DRIVER_WALL_TURN);
            }
        }
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_DRIVER);
}

/* Getting out: a step of DRIVER_OUT_STEP toward the side, on the ground,
   until it is past D_803ED814 (then walking, stopped); and its shadow. */
REGS(gp)
void func_802AF340(VS *vs) {
    s32 v, past;

    VS_SPEED(vs) = DRIVER_OUT_SPEED;
    side_step(D_803ED828, DRIVER_OUT_STEP, &X, &Z);
    /* (as func_802AEC3C's) */
    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_DRIVER, vs, 0);
    v = D_803ED814;
    switch (D_803ED828) {
    case 0: past = !(Z < v); break;
    case 1: past = !(v < Z); break;
    case 2: past = !(X < v); break;
    default: past = !(v < X); break;
    }
    if (past) {
        D_803ED826 = 0;
        VS_SPEED(vs) = 0;
    }
    /* the shadow: its last three stack arguments the original never
       stores: the tilt (SHADOW_TILT), and the halves of the return address
       its frame saved (the heading) */
    func_802582C4(VEHICLE_DRIVER, X, (u32)(D_803ED3A8[1] + D_803ED3A8[2]) >> 1, Z, Y, SHADOW_TILT, 0xFFFFFFFF,
                  RA_SHADOW);
}

/* The walk a frame.  Standing: once, the run's animation handed over to a
   look round (part 3), then now and then (1 in DRIVER_IDLE_CHANCE + 1) a
   random one of parts 2, 3 and 4 when none plays.  Moving: once, the look
   round handed over to the run (part 0x1F, then the legs); the legs (part
   1) with the speed, backwards in reverse, and a footstep's sound on their
   frames 2 and 6. */
REGS(gp)
void func_802AF4BC(VS *vs) {
    Part *p = DRV;
    s32 s = VS_SPEED(vs), v, f13, last, k;

    if (s == 0) {
        if (DRV_WALKING(vs) != 0) {
            func_802A02E4(1, p);
            func_802A0360(3, 0, p, 0.0f);
            func_8029F9D4(1, 3, p);
            func_802A039C(0x1F, 0x46, p);
            func_802A03D4(0x1F, 0, p);
            func_802A040C(0x1F, 0, p);
            func_802A0290(0x1F, 1, p);
        }
        DRV_WALKING(vs) = 0;
        if (part_state(0x1F, p, NULL) == 1 || part_state(2, p, NULL) == 1 || part_state(3, p, NULL) == 1 ||
            part_state(4, p, NULL) == 1)
            return;
        if (func_8026A8E0(0, DRIVER_IDLE_CHANCE) != 0)
            return;
        v = func_8026A8E0(0, 2);
        v = v == 0 ? 3 : v == 1 ? 4 : 2;
        func_802A0360(v, 0, p, 0.0f);
        func_802A0290(v, 1, p);
        return;
    }
    if (DRV_WALKING(vs) != 1) {
        /* starting off: the look round still playing handed over */
        func_802A0360(1, 2, p, 0.0f);
        for (k = 2; k <= 4; k++) {
            if (part_state(k, p, NULL) != 0) {
                func_8029F9D4(k, 1, p);
                func_802A02E4(k, p);
                break;
            }
        }
        if (k > 4) {
            func_802A0360(3, 0, p, 0.0f);
            func_8029F9D4(3, 1, p);
        }
        func_802A039C(0x1F, 0x28, p);
        func_802A03D4(0x1F, 0, p);
        func_802A040C(0x1F, 0, p);
        func_802A0290(0x1F, 1, p);
    }
    if (part_state(0x1F, p, NULL) != 1) {
        s = VS_SPEED(vs);
        func_802A03D4(1, s < 0 ? 1 : 0, p);
        /* the footsteps (the original saves and loads back every register;
           the level's byte it tests is an s32's top one, always 0, so its
           two levels without them never match) */
        v = (u32)D_802E8BDC >> 24;
        if (v != 0x31 && v != 0x26) {
            part_state(1, p, &f13);
            last = D_803ED820;
            D_803ED820 = f13;
            if (f13 != last && (f13 == 2 || f13 == 6))
                func_80260650(D_80367738, f13 == 2 ? 0x14 : 0x15, NULL);
        }
        func_802A039C(1, (u32)iabs(s) / DRIVER_LEG_SPEED_DIV, p);
        func_802A0290(1, -1, p);
    }
    DRV_WALKING(vs) = 1;
}

/* the driver's matrix and its vertices */
REGS(gp)
void func_802AFA64(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);
    s32 *m = (s32 *)(buf + MODEL_MTX_OFF(model));

    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, DRIVER_SCALE, m);
    func_8029C454(X, Y, Z, VEHICLE_DRIVER, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
}

/* the steering rate */
REGS(-> s3)
s32 func_802AFB84(void) {
    return DRIVER_STEER;
}

/* the physics' settings for the driver: gravity, and how it lands */
REGS()
void func_802AFBA0(void) {
    D_803EBBF4 = D_803EBBF0 * DRIVER_GRAVITY;
    D_803ED3F6 = DRIVER_BOUNCE_MIN;
    D_803ED3F7 = DRIVER_BOUNCE_DIV;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802AFBFC(u8 *dst) {
    func_802AC7DC(dst, (u8 *)&D_803ED760, (u32 *)&D_803ED808);
}

/* and back */
void func_802AFC28(u8 *src) {
    func_802AC85C(src, (u8 *)&D_803ED760, (u32 *)&D_803ED808);
}
