/*
 * hd_code 6B4A0 (us.v11 0x802AFC60-0x802B0D9C): the Sideswipe
 * (VEHICLE_SIDESWIPE), as native C (engine.h).  Its parts are D_803ED840,
 * its state D_803EDB40 and its position D_803EDBE8..F0 (vehicle.h).
 * func_802AFC60 sets it up (from the level loader), func_802B03F4 runs it
 * each frame (from hd.c and at the end of the setup); the others are
 * hd.c's hooks for it.  Part 5 is its pair of side rams (the hydraulics
 * D_803EDC00 counts what is left of them), part 3 their extension.
 */
#include "vehicle.h"
#include "game/audio.h"
#include "buildings.h"

/* the Sideswipe's .bss (asm/data/hd_code/6B4A0.bss.s) */
extern Part D_803ED840[32];
extern VS D_803EDB40;
extern s32 D_803EDBE8, D_803EDBEC, D_803EDBF0;  /* x, y, z */
extern u8 *PTR32 D_803EDBF4;                    /* its model file */
extern u8 *PTR32 D_803EDBF8;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EDBFC;
extern u16 D_803EDC02;                          /* the heading it turns to against a wall (D_803A7425) */
extern s8 D_803EDC04;                           /* turning to it */
extern u8 D_803EDC05;                           /* the rams' extension, 0..100 */
extern u8 D_803EDC06;                           /* frames until the rams can fire again */

#define PARTS D_803ED840
#define X D_803EDBE8
#define Y D_803EDBEC
#define Z D_803EDBF0
#define MODEL D_803EDBF4
#define BUF0 D_803EDBF8
#define BUF1 D_803EDBFC

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern u8 D_80364A6D;
extern Part *PTR32 D_803F77D0;
extern u8 D_80305CE0[];                         /* its parts' collision (56040's func_8029A800) */
extern s32 D_802E8BE8;
extern u8 D_802C2390[];                         /* the rams' part record (56040's list) */

void func_802B03B0(void);
REGS(gp)
void func_802B07DC(VS *vs);
void func_802B0AAC(void);
REGS(gp)
void func_802B0B3C(VS *vs);
REGS(gp -> s3)
s32 func_802B0C74(VS *vs);
void func_802B0CE8(void);

/* ---- the Sideswipe's numbers (a frame, where it's per frame) ------------- */

#define SIDESWIPE_SCALE 0x1B58          /* its model's scale */
#define SIDESWIPE_SPAN 0x2D0            /* its wheels' spans, both ways (func_802A8768) */
#define SIDESWIPE_BRAKE 0x14            /* speed lost braking or against the stick, a frame */
#define SIDESWIPE_TURN_RATE 0x4650      /* func_802A7FD8: the move heading's turn, times the grip over the speed */
#define SIDESWIPE_SLOPE_DIV 720.0f      /* func_802A843C: the slope's push is the height difference over this */
#define SIDESWIPE_STEER_DIV 3.2f        /* the steering rate: the speed over this ... */
#define SIDESWIPE_STEER_DIV_AIR 6.0f    /* ... or this with a wheel in the air */
#define SIDESWIPE_WALL_TURN 0.18f       /* func_802A71DC: turning along a wall, times the speed */
#define SIDESWIPE_GRAVITY 4.0f          /* times the level's */
#define SIDESWIPE_BOUNCE_MIN 0x28       /* a landing harder than this bounces ... */
#define SIDESWIPE_BOUNCE_DIV 3          /* ... at the speed over this */
#define RAMS_RELOAD_FRAMES 10           /* frames after firing the rams before they fire again */
#define RAMS_REACH_MID 50               /* the rams' extension at rest (0..100, pad_lever's middle) */
#define RAMS_REACH_STEP 5               /* the extension's change a frame */
#define WHEEL_SPIN_DIV 14               /* the wheels' spin: the speed over this */
#define RAMS_SHAKE_FRAMES 10            /* the screen's shake when the rams hit (D_802E8BE4) */

/* part i's state: func_802A04BC's first result */
static s32 part_state(s32 i, Part *parts) {
    s32 f11, f12, f14, fC, fE, f13;
    f32 f4;

    return func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &f13, &f4);
}

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802AFC60(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EDB40;
    s32 avg;

    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(VEHICLE_SIDESWIPE, 0, BUF0, BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x168, 0x168, -0x168, 0x168, 0x168, -0x168);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x17C, 0x17C, -0x17C, 0x17C, 0x17C, -0x17C);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    D_803EDC05 = RAMS_REACH_MID;
    D_803EDC00 = 0;
    D_803EDC06 = 0;
    D_803EDC04 = 0;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_SIDESWIPE, vs, 0, &avg);
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
    SET_GEARS(vs, -0xB4, 0, 5, 0, 0x3C, 5, 0x3C, 0x64, 4, 0x64, 0xB4, 3, 0xB4, 0xDC, 2);
    func_8029C354(VEHICLE_SIDESWIPE, MODEL_AT(model, 4), MODEL_AT(model, 8), SIDESWIPE_SCALE);
    func_80258230(VEHICLE_SIDESWIPE, 0x96, 0x3C, 0x3C);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802B03F4();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
    D_80364A6D = 1;
}

/* hd.c's (and 17210.c's): the player gets in: the rams back in */
void func_802AFFD4(void) {
    Part *p = PARTS;

    VS_TURNING(&D_803EDB40) = 0;
    func_802A039C(1, 0, p);
    func_802A03D4(1, 0, p);
    func_802A040C(1, 0, p);
    func_802A0290(1, -1, p);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 0, p);
    func_802A0290(2, -1, p);
    func_802A039C(5, 7, p);
    func_802A03D4(5, 0, p);
    func_802A040C(5, 1, p);
    func_802A039C(3, 0, p);
    func_802A03D4(3, 0, p);
    func_802A040C(3, 1, p);
    func_802A0290(3, -1, p);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 1, p);
    func_802A0290(2, -1, p);
    func_802A05D0(D_802C2390, 100);
    func_802A05F8(D_802C2390, 0);
    func_802A0620(D_802C2390, 1);
    func_802A0508(D_802C2390, -1);
    D_8036444C = 0xD48;
    D_80364450 = 0x12C;
    func_802C4310(0x36);
}

/* hd.c's: whether it can be left: not with a wheel in the air; with the
   rams out (part 5's state 1) the original answers 5, its $v0, which hd.c
   takes as yes all the same */
u8 func_802B01DC(void) {
    if (ANY_AIRBORNE(&D_803EDB40))
        return 0;
    return part_state(5, PARTS) != 1 ? 1 : 5;
}

/* hd.c's: the player gets out */
void func_802B0254(void) {
    VS_SPEED(&D_803EDB40) = 0;
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x800);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802B02A0(void) {
    VS *vs = &D_803EDB40;

    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_SIDESWIPE, vs, 0);
    func_802B0B3C(vs);
    func_802A133C(X, Y, Z, VEHICLE_SIDESWIPE, vs);
}

/* its light */
void func_802B03B0(void) {
    func_802ABD54(VEHICLE_SIDESWIPE, X, Y, Z);
}

/* each frame */
void func_802B03F4(void) {
    VS *vs = &D_803EDB40;
    s32 step = 0, x, z, rate_i, turn;
    f32 rate;

    func_802B03B0();
    if (VS_IN_SETUP(vs) == 0)
        func_802B07DC(vs);
    func_802B0CE8();
    /* steering, the throttle, the turn and the slope */
    rate_i = func_802B0C74(vs);
    func_802A7834(step, &VS_SPEED(vs), 3, VS_AIRBORNE(vs), VS_GEARS(vs), SIDESWIPE_BRAKE, rate_i, &VS_HEADING(vs),
                  vs, &step, &turn);
    func_802A7FD8(SIDESWIPE_TURN_RATE, &VS_SPEED(vs), (u16 *)&VS_TURN_HEADING(vs), &VS_HEADING(vs),
                  &VS_MOVE_HEADING(vs), (s8 *)&VS_TURNING(vs), 1, vs);
    rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    func_802A843C(&VS_SPEED(vs), 1, VEHICLE_SIDESWIPE, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), SIDESWIPE_SLOPE_DIV,
                  vs);
    if (D_803EDC04 != 0)
        func_802A7070((s16 *)&D_803EDC02, vs);
    /* the move, on the ground */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_SIDESWIPE, SIDESWIPE_SPAN, SIDESWIPE_SPAN, VS_WHEELS(vs),
                  VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(PARTS, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802B0B3C(vs);
    /* what it hits */
    func_8029A800(D_803EDBE8, D_803EDBEC, D_803EDBF0, D_80305CE0, 0, 1, 7, vs->unk76, 0x64, 0, 1, vs);
    func_8029C52C(VEHICLE_SIDESWIPE, vs);
    func_8029AA10();
    D_803F77D0 = PARTS;
    func_802BE77C(VEHICLE_SIDESWIPE, vs);
    func_802B0AAC();
    if (D_803A7425 == 0) {
        D_803EDC04 = 0;
    } else {
        D_803EDC04 = 1;
        turn_along_wall(vs, &D_803EDC02, SIDESWIPE_WALL_TURN);
    }
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_SIDESWIPE);
}

/* the parts: the wheels' turn from the camera, the rams (L or R fires
   them while the hydraulics last), their extension with the d-pad, the
   wheels' spin with the speed, and the engine's sound */
REGS(gp)
void func_802B07DC(VS *vs) {
    Part *p = PARTS;
    u32 a;
    s32 s;

    /* the wheels' turn: the camera's heading from behind, in thirds */
    a = (u16)D_80364452 + ANGLE_HALF;
    if ((s32)a >= ANGLE_TURN)
        a -= ANGLE_WRAP;
    func_802A0360(2, a / 0x555 < 2 ? a / 0x555 : 2, p, (f32)(s32)(a % 0x555) / 1365.0f);
    /* the rams */
    if (D_803EDC06 != 0) {
        D_803EDC06--;
    } else if (part_state(5, p) != 1 && (PAD_L != 0 || PAD_R != 0) && D_803EDC00 != 0) {
        D_803EDC00--;
        func_802A0290(5, 2, p);
        D_803EDC06 = RAMS_RELOAD_FRAMES;
        func_80260650(D_80367738, 0x37, NULL);
    }
    D_803EDC05 = pad_lever(D_803EDC05, RAMS_REACH_STEP, RAMS_REACH_STEP);
    func_802A0360(3, 0, p, (f32)D_803EDC05 / 100.0f);
    /* the wheels' spin, backwards in reverse */
    s = VS_SPEED(vs);
    func_802A03D4(1, s < 0 ? 1 : 0, p);
    s = (u32)iabs(s) / WHEEL_SPIN_DIV;
    func_802A039C(1, s, p);
    func_802C4584(s);
}

/* the rams out against something breakable (func_802BCD80's kinds 4 and
   5): they stop, and the screen shakes */
void func_802B0AAC(void) {
    if (part_state(5, PARTS) != 0 && (func_802BCD80(4) != 0 || func_802BCD80(5) != 0)) {
        func_802A03D4(5, 1, PARTS);
        func_802A0290(5, 1, PARTS);
        D_802E8BE4 = RAMS_SHAKE_FRAMES;
        D_802E8BE8 = 600;
    }
}

/* the Sideswipe's matrix, its vertices and its collision */
REGS(gp)
void func_802B0B3C(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);

    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, SIDESWIPE_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(X, Y, Z, VEHICLE_SIDESWIPE, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_SIDESWIPE, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: the speed over SIDESWIPE_STEER_DIV, or over
   SIDESWIPE_STEER_DIV_AIR with a wheel in the air */
REGS(gp -> s3)
s32 func_802B0C74(VS *vs) {
    return engine_cvt_w_s((f32)VS_SPEED(vs) / (ANY_AIRBORNE(vs) ? SIDESWIPE_STEER_DIV_AIR : SIDESWIPE_STEER_DIV));
}

/* the physics' settings for the Sideswipe: gravity, and how its wheels
   land */
void func_802B0CE8(void) {
    D_803EBBF4 = D_803EBBF0 * SIDESWIPE_GRAVITY;
    D_803ED3F6 = SIDESWIPE_BOUNCE_MIN;
    D_803ED3F7 = SIDESWIPE_BOUNCE_DIV;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B0D44(u8 *dst) {
    vehicle_save(dst, (u8 *)&D_803EDB40, (u32 *)&D_803EDBE8, (u32 *)&D_803EDBEC, (u32 *)&D_803EDBF0);
}

/* and back */
void func_802B0D70(u8 *src) {
    vehicle_restore(src, (u8 *)&D_803EDB40, (u32 *)&D_803EDBE8, (u32 *)&D_803EDBEC, (u32 *)&D_803EDBF0);
}
