/*
 * hd_code 772A0 (us.v11 0x802BBA60-0x802BC5D4): the train (VEHICLE_TRAIN),
 * as native C (engine.h, vehicle.h).  Its parts are D_803EFAF0, its state
 * D_803EFDF0 and its position D_803EFE98..A0.  func_802BBA60 sets it up
 * (from the level loader), func_802BBEB8 runs it each frame (from hd.c and
 * at the end of the setup); the others are hd.c's hooks for it.
 *
 * The train runs on rails: with D_803EFEC8 set, its x follows its z along
 * the line from (D_803EFEB0, D_803EFEB4) to (D_803EFEB8, D_803EFEBC).  At
 * the line's ends (its parts 2 and 3 hitting something) it is sent back.
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

/* the train's .bss (asm/data/hd_code/772A0.bss.s) */
extern Part D_803EFAF0[32];
extern VS D_803EFDF0;
extern s32 D_803EFE98, D_803EFE9C, D_803EFEA0;  /* x, y, z */
extern u8 *PTR32 D_803EFEA4;                    /* its model file */
extern u8 *PTR32 D_803EFEA8;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EFEAC;
extern s32 D_803EFEC0, D_803EFEC4;              /* frames since it was last at each end */
extern u8 D_803EFEC9;                           /* the wheels' sparks: frames to wait */
extern u8 D_803EFECA;

#define TR D_803EFAF0
#define X D_803EFE98
#define Y D_803EFE9C
#define Z D_803EFEA0
#define MODEL D_803EFEA4
#define BUF0 D_803EFEA8
#define BUF1 D_803EFEAC

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern Part *PTR32 D_803F77D0;
extern u8 D_80305E00[];                         /* its parts' collision (56040's func_8029A800) */
extern u8 D_802C2984[];                         /* the sparks' effect record (60F60) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);

void func_802BBEB8(void);
void func_802BBE74(void);
void func_802BC2C8(void);
REGS(gp)
void func_802BC3D0(VS *vs);
void func_802BC578(void);

#define T(p) ((s32)(p))

/* ---- the train's numbers (a frame, where it's per frame) ----------------- */

#define TRAIN_SCALE 15000               /* its model's scale */
#define TRAIN_SPAN 0xA0                 /* its wheels' spans, both ways (func_802A8768) */
#define TRAIN_BRAKE 6                   /* speed lost braking or against the stick, a frame */
#define TRAIN_TURN_RATE 0x2328          /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define TRAIN_SLOPE_DIV 160.0f          /* func_802A843C: the slope's push is the height difference over this */
#define TRAIN_SHUNT_SPEED 0x28          /* sent back from an end of the line at this */
#define TRAIN_SHUNT_FRAMES 6            /* ... or stopped, if it left the other end fewer frames ago */
#define TRAIN_GRAVITY 2.0f              /* times the level's */
#define TRAIN_BOUNCE_MIN 0x3C           /* a landing harder than this bounces ... */
#define TRAIN_BOUNCE_DIV 4              /* ... at the speed over this */

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802BBA60(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EFDF0;
    s32 avg;

    ENGINE_COST(802BBA60, 218);
    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(VEHICLE_TRAIN, 0, BUF0, BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x50, 0x50, -0x50, 0x50, 0x50, -0x50);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x5A, 0x5A, -0x5A, 0x5A, 0x5A, -0x5A);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_TRAIN, vs, 0, &avg);
    func_8029F85C(TR, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, TR);
    func_802A03D4(0, 0, TR);
    func_802A040C(0, 0, TR);
    func_802A0480(0, 0, TR, 0.0f);
    func_802A0290(0, 1, TR);
    func_8029E558(TR, BUF0, BUF1);
    func_802A0320(0, TR);
    func_802A0290(0, 1, TR);
    func_8029E558(TR, BUF1, BUF0);
    SET_GEARS(vs, -0xB4, 0, 1, 0, 0x50, 1, 0x50, 0x8C, 1, 0x8C, 0xBE, 1, 0xBE, 0xFA, 1);
    D_803EFEC9 = 0;
    D_803EFECA = 0;
    D_803EFEC0 = 999999;
    D_803EFEC4 = 999999;
    func_8029C354(VEHICLE_TRAIN, MODEL_AT(model, 4), MODEL_AT(model, 8), TRAIN_SCALE);
    func_80258230(VEHICLE_TRAIN, 0x96, 0x2D, 0x2D);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802BBEB8();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
}

/* hd.c's: the player gets in */
void func_802BBDC8(void) {
    ENGINE_COST(802BBDC8, 18);
    D_8036444C = 3000;
    D_80364450 = 0;
    func_802C4310(0x20);
}

/* hd.c's: whether it can be left (always) */
u8 func_802BBE10(void) {
    ENGINE_COST(802BBE10, 7);
    return 1;
}

/* hd.c's: the player gets out */
void func_802BBE2C(void) {
    ENGINE_COST(802BBE2C, 18);
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x800);
    func_802C444C();
}

/* its light */
void func_802BBE74(void) {
    ENGINE_COST(802BBE74, 17);
    func_802ABD54(VEHICLE_TRAIN, X, Y, Z);
}

/* at an end of the line (D_803A7425: its front, part 2, or its back, part
   3, hit something this frame): sent back the other way, or stopped if it
   was at the other end only TRAIN_SHUNT_FRAMES ago, or both ends hit */
static void train_shunt(VS *vs) {
    s32 front = func_802BCD80(2), back = func_802BCD80(3);

    if (back != 0) {
        if (front != 0) {
            VS_SPEED(vs) = 0;
            return;
        }
        D_803EFEC0 = 0;
        VS_SPEED(vs) = D_803EFEC4 < TRAIN_SHUNT_FRAMES ? 0 : -TRAIN_SHUNT_SPEED;
    } else if (front != 0) {
        D_803EFEC4 = 0;
        VS_SPEED(vs) = D_803EFEC0 < TRAIN_SHUNT_FRAMES ? 0 : TRAIN_SHUNT_SPEED;
    }
}

/* each frame */
void func_802BBEB8(void) {
    VS *vs = &D_803EFDF0;
    s32 step = X, x, z, x0, z0;
    f32 rate, f;

    ENGINE_COST(802BBEB8, 187);
    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802BBE74();
    if (VS_IN_SETUP(vs) == 0)
        func_802BC2C8();
    func_802BC578();
    /* the throttle (no steering), the turn and the slope */
    func_802A785C(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), TRAIN_BRAKE, vs, &step);
    func_802A7FD8(TRAIN_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs), &VS_MOVE_HEADING(vs),
                  (s8 *)&VS_TURNING(vs), 0, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_TRAIN, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), TRAIN_SLOPE_DIV, vs);
    /* the move, on the rails: x from z */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    if (D_803EFEC8 != 0) {
        z0 = D_803EFEB4;
        f = (f32)(Z - z0) / (f32)(D_803EFEBC - z0);
        x0 = D_803EFEB0;
        x = x0 + engine_cvt_w_s((f32)(D_803EFEB8 - x0) * f);
        X = x;
    }
    D_803ED40B = 0;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_TRAIN, TRAIN_SPAN, TRAIN_SPAN, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(TR, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802BC3D0(vs);
    D_803EFEC0++;
    D_803EFEC4++;
    /* what it hits */
    if (VS_IN_SETUP(vs) == 0) {
        func_8029A800(X, Y, Z, D_80305E00, 0, 0, 0, VS_SPEED(vs), 0, 0, VEHICLE_TRAIN, vs);
        func_8029C52C(VEHICLE_TRAIN, vs);
        func_8029AA10();
        D_803F77D0 = TR;
        func_802BE77C(VEHICLE_TRAIN, vs);
        if (D_803A7425 != 0)
            train_shunt(vs);
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_TRAIN);
    engine_restore();
}

/* the wheels' sparks every other frame while it accelerates (A going
   forward, B or Z going back), with their sound; the engine's sound */
void func_802BC2C8(void) {
    VS *vs = &D_803EFDF0;

    ENGINE_COST(802BC2C8, 29);
    if (D_803EFEC9 != 0) {
        D_803EFEC9--;
    } else if (VS_SPEED(vs) > 0 ? PAD_B_OR_Z != 0 : PAD_A != 0) {
        func_802A6274(T(D_802C2984), 0x9C40, 1, VEHICLE_TRAIN, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        func_802A6274(T(D_802C2984), 0x9C40, 1, VEHICLE_TRAIN, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        func_80260650(D_80367738, 0x29, NULL);
        D_803EFEC9 = 1;
    }
    func_802C4584(iabs(VS_SPEED(vs) >> 4));
    func_802C4724(0x21);
}

/* the train's matrix, its vertices, its collision and its shadow */
REGS(gp)
void func_802BC3D0(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);

    ENGINE_COST(802BC3D0, 98);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, TRAIN_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(X, Y, Z, VEHICLE_TRAIN, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_TRAIN, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
    func_802AABE4(VEHICLE_TRAIN, (u16 *)MODEL_AT(model, 8), buf, 0, 0);
    func_8029D040(X, Z, VEHICLE_TRAIN, MODEL_AT(model, 0xC), VS_HEADING(vs), TR, buf);
    D_803EFECB = func_8029DC14(VEHICLE_TRAIN);
}

/* the physics' settings for the train: gravity, and how its wheels land */
void func_802BC578(void) {
    ENGINE_COST(802BC578, 23);
    D_803EBBF4 = D_803EBBF0 * TRAIN_GRAVITY;
    D_803ED3F6 = TRAIN_BOUNCE_MIN;
    D_803ED3F7 = TRAIN_BOUNCE_DIV;
}
