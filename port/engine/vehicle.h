/*
 * The vehicles' physics as the engine's C reads it: VehicleState's fields
 * by what they hold, and the per-frame constants of 62740's shared physics
 * (docs/PORT.md, "The engine made readable").
 *
 * VehicleState (blastcorps/include/game/vehicle.h) names its fields by
 * offset, as the game's headers do, and other code (the game's C, the
 * digest, the other half of port/engine) uses those names; the macros here
 * are the same fields, named where what they hold is plain from the code
 * that uses them (62740's shared physics and the vehicle modules).  A
 * field whose use isn't plain keeps its offset name.
 *
 * Everything here steps once a game frame (30 a second on the N64), so
 * every constant marked "a frame" is what a higher tick rate (DISTRIBUTION.md,
 * "a delta-based engine", design (a)) would have to scale.  Angles are
 * 12-bit (0x1000 a turn), positions << 5, speeds s16 in position units
 * (>> 5) a frame... as the original keeps them.
 */
#ifndef ENGINE_VEHICLE_H
#define ENGINE_VEHICLE_H

#include "shared.h"

/* ---- VehicleState's fields ------------------------------------------------ */

/* f32: the speed over the wheels' height difference (func_802A83B8): the
   turn-rate factor func_802A860C scales the step by */
#define VS_SLOPE_RATIO(vs) ((vs)->unk0)
/* s32[9]: the three wheels' heights, three each (now, last frame, the one
   before; func_802A8768 shifts them along) */
#define VS_WHEEL_H(vs) ((vs)->unk4)
/* s32[9]: the wheels' fall: [0..2] its step, [3..5] the ground it left,
   [6..8] the frames it has been falling (func_802A95A4, func_802A9540) */
#define VS_WHEEL_FALL(vs) ((vs)->unk28)
#define VS_WHEEL_GROUND(vs) ((vs)->unk28 + 3)
#define VS_WHEEL_FRAMES(vs) ((vs)->unk28 + 6)
/* u16: the heading it faces (the stick turns it: func_802A7E70) */
#define VS_HEADING(vs) ((vs)->unk4C)
/* u16: the heading it moves along (func_802A860C; func_802A7FD8 turns it
   toward VS_HEADING) */
#define VS_MOVE_HEADING(vs) ((vs)->unk4E)
/* u8: the ground's grip, the average of the three wheels' materials
   (func_802A992C, func_802A9164): how many gear rows it can use
   (func_802A7C28) and how fast it turns (func_802A7FD8) */
#define VS_GRIP(vs) ((vs)->unk50)
/* s16[6]: the three wheels' (x, z) offsets from the vehicle's position,
   turned by its heading (func_802A94A4): where it touches the ground */
#define VS_WHEELS(vs) ((vs)->unk52)
/* s16[6]: the same for what it puts on a moving object (func_802A92C8)
   and is carried by (62740_carry.c) */
#define VS_CARRY_WHEELS(vs) ((vs)->unk5E)
/* s16: where it stands on its carrier (func_802AAD0C): the point, and the
   point ahead of it, in the carrier's own triangles' frame (62740_carry.c) */
#define VS_CARRY_X(vs) ((vs)->unk6A)
#define VS_CARRY_Z(vs) ((vs)->unk6C)
#define VS_CARRY_HEADING(vs) ((vs)->unk6E)
#define VS_CARRY_X2(vs) ((vs)->unk70)
#define VS_CARRY_Z2(vs) ((vs)->unk72)
/* u16: the heading the move heading is turned from (func_802A7FD8's
   *angle) */
#define VS_TURN_HEADING(vs) ((vs)->unk74)
/* s16: the speed, signed (forward > 0) */
#define VS_SPEED(vs) ((vs)->unk76)
/* s16[15]: the gear table, five rows of (lowest speed, highest speed,
   acceleration a frame) (func_802A7C28) */
#define VS_GEARS(vs) ((vs)->unk78)
/* u8[3]: each wheel in the air (1) or on the ground (0); [3]: turning
   (func_802A7FD8's *turning) */
#define VS_AIRBORNE(vs) ((vs)->unk96)
#define VS_TURNING(vs) ((vs)->unk96[3])
/* s8: set while the setup runs the first frame (the frame functions skip
   their effects then) */
#define VS_IN_SETUP(vs) ((vs)->unk9A)
/* u8: a wheel stands on the level's static triangles (func_802A9B1C) */
#define VS_ON_STATIC(vs) ((vs)->unk9B)
/* u8: the highest of the wheels' materials (func_802A9164) */
#define VS_TOP_MATERIAL(vs) ((vs)->unk9F)
/* s8: the speed's sign when func_802A6FE4 last ran (+1, -1) */
#define VS_SPEED_SIGN(vs) ((vs)->unkA5)
/* s16: the speed the engine's sound was last set for (the chopper's and
   the carrier's only) */
#define VS_SOUND_SPEED(vs) ((vs)->unkA6)

/* the gear table's rows (VS_GEARS) */
#define GEAR_LO 0
#define GEAR_HI 1
#define GEAR_ACCEL 2
#define GEAR_ROWS 5
/* a row as (lowest, highest, acceleration a frame) */
#define SET_GEAR(rows, k, lo, hi, accel) \
    ((rows)[3 * (k)] = (lo), (rows)[3 * (k) + 1] = (hi), (rows)[3 * (k) + 2] = (accel))

/* ---- angles --------------------------------------------------------------- */

#define ANGLE_TURN 0x1000       /* a whole turn */
#define ANGLE_HALF 0x800
#define ANGLE_QUARTER 0x400
/* Rare's wrap of a heading back into 0..0xFFF after a step: by 0xFFF, not
   0x1000, in most places (so a heading drifts a unit a wrap); kept */
#define ANGLE_WRAP 0xFFF

/* a 12-bit angle back into 0..0xFFF after a step, Rare's way (by 0xFFF) */
static inline s32 wrap_fff(s32 a) {
    if (a < 0)
        a += ANGLE_WRAP;
    else if (a >= ANGLE_TURN)
        a -= ANGLE_WRAP;
    return a;
}

static inline s32 iabs(s32 v) {
    return v < 0 ? -v : v;
}

/* ---- the controller, as 45BB0.c's func_8028AE88... polls it each frame ---- */

/* one byte a button (1 held), and the stick (45BB0.c) */
extern u8 D_80370C15, D_80370C16, D_80370C17, D_80370C18, D_80370C19, D_80370C1A, D_80370C1B, D_80370C1C,
    D_80370C1D, D_80370C1E, D_80370C1F, D_80370C20, D_80370C21, D_80370C22, D_80370C23;
extern s8 D_80370C2C, D_80370C2D;
#define PAD_LEFT D_80370C15     /* button 0x200 (the d-pad's left, as 45BB0.c maps it) */
#define PAD_RIGHT D_80370C16    /* button 0x100 (its right) */
#define PAD_L D_80370C1A        /* the L trigger (0x20) */
#define PAD_R D_80370C1B        /* the R trigger (0x10) */
#define PAD_A D_80370C1C        /* A (0x8000) */
#define PAD_B D_80370C1D        /* B (0x4000) */
#define PAD_Z D_80370C22        /* Z (0x2000): the brake */
#define PAD_B_OR_Z D_80370C23
#define STICK_X D_80370C2C      /* the stick, -80..80 or so: x steers */
#define STICK_Y D_80370C2D      /* y is the throttle */
#define STICK_RANGE 0x50        /* what a full push is taken as (func_802A7E70, func_802A7A1C) */

/* ---- what every vehicle module does ------------------------------------- */

#include "game/game.h"
#include "game/camera.h"

extern u8 D_8035805C;                   /* which of the two frame buffers is this frame's (game/frame.h) */
extern s16 D_8036444C, D_80364450;      /* the camera's distance and height for the vehicle the player is in */

/* this frame's of a vehicle's two buffers (its vertices and matrices, one
   per frame buffer), and the other */
#define FRAME_BUF(a, b) (D_8035805C != 0 ? (a) : (b))
#define OTHER_BUF(a, b) (D_8035805C != 0 ? (b) : (a))

/* a model file's part at the offset its table holds at `off`
   (VehicleModel: 0 the collision points, 4 and 8 the parts' lists, 0xC
   the crane's, 0x18 the matrices' table, 0x20 the bounding rectangle,
   0x24..0x28 the triangles) */
#define MODEL_AT(model, off) ((u8 *)(model) + *(s32 *)((u8 *)(model) + (off)))
/* the offset of the vehicle's matrix in its buffers */
#define MODEL_MTX_OFF(model) (*(s32 *)(MODEL_AT(model, 0x18) + 4))

/* the gear table, five rows at once (lowest, highest, acceleration a
   frame) */
#define SET_GEARS(vs, l0, h0, a0, l1, h1, a1, l2, h2, a2, l3, h3, a3, l4, h4, a4)                              \
    do {                                                                                                       \
        s16 *g_ = VS_GEARS(vs);                                                                                \
        g_[0] = (l0), g_[1] = (h0), g_[2] = (a0), g_[3] = (l1), g_[4] = (h1), g_[5] = (a1), g_[6] = (l2),      \
        g_[7] = (h2), g_[8] = (a2), g_[9] = (l3), g_[10] = (h3), g_[11] = (a3), g_[12] = (l4), g_[13] = (h4), \
        g_[14] = (a4);                                                                                         \
    } while (0)

/* the three wheels' (x, z) offsets, and the three it puts on what carries
   it */
#define SET_WHEELS(pts, x0, z0, x1, z1, x2, z2)                                                                \
    ((pts)[0] = (x0), (pts)[1] = (z0), (pts)[2] = (x1), (pts)[3] = (z1), (pts)[4] = (x2), (pts)[5] = (z2))

/* a wheel in the air */
#define ANY_AIRBORNE(vs) (VS_AIRBORNE(vs)[0] == 1 || VS_AIRBORNE(vs)[1] == 1 || VS_AIRBORNE(vs)[2] == 1)

/* the player's vehicle's position, speed and headings for the camera and
   the rest of the game (game.h, camera.h), at the end of its frame; then
   its Vehicle record's position (func_802A133C) */
#define PLAYER_FROM(x, y, z, vs, type)                                                                         \
    do {                                                                                                       \
        D_803643E0 = (x);                                                                                      \
        D_803643E4 = (y);                                                                                      \
        D_803643E8 = (z);                                                                                      \
        D_8036443C = VS_SPEED(vs);                                                                             \
        D_8036443E = VS_MOVE_HEADING(vs);                                                                      \
        D_80364440 = VS_HEADING(vs);                                                                           \
        func_802A133C(D_803643E0, D_803643E4, D_803643E8, (type), (vs));                                       \
    } while (0)

/* against a wall (the collision narrowed the headings it may face,
   D_803A7425): the vehicle's move heading turned along it at `rate` times
   the speed (func_802A71DC), into *target too, its heading turned toward
   that, and the speed brought toward 0 (func_802A6FE4) */
static inline void turn_along_wall(VS *vs, u16 *target, f32 rate) {
    s32 turn;

    func_8029A914(vs);
    func_802A70D8(vs);
    *target = func_802A71DC(VS_MOVE_HEADING(vs), VS_HEADING(vs), rate, vs, &turn);
    VS_MOVE_HEADING(vs) = *target;
    VS_TURN_HEADING(vs) = *target;
    func_802A746C(turn, vs);
    func_802A6FE4(0, vs);
}

/* v a step toward `to`, not past it */
static inline s32 step_toward(s32 v, s32 to, s32 step) {
    if (v < to) {
        v += step;
        if (v > to)
            v = to;
    } else {
        v -= step;
        if (v < to)
            v = to;
    }
    return v;
}

static inline f32 step_toward_f(f32 v, f32 to, f32 step) {
    if (v < to) {
        v = v + step;
        if (!(v <= to))
            v = to;
    } else {
        v = v - step;
        if (v < to)
            v = to;
    }
    return v;
}

/* a part's lever (0..100) a frame: PAD_LEFT lowers it by `step`, PAD_RIGHT
   raises it, else it goes back to the middle (50) by `back` */
static inline s32 pad_lever(s32 v, s32 step, s32 back) {
    if (PAD_LEFT != 0) {
        v -= step;
        if (v < 0)
            v = 0;
    } else if (PAD_RIGHT != 0) {
        v += step;
        if (v > 100)
            v = 100;
    } else {
        v = step_toward(v, 50, back);
    }
    return v;
}

#define LIFT_STEP 0.05f                 /* pad_lift: a frame */
#define LIFT_BACK 0.03f

/* a part's lift (0..1) a frame: B or Z lowers it by LIFT_STEP, A raises
   it, unless the speed is within `range` of the top (func_802A7CB0); else
   it goes back to the middle (0.5) by LIFT_BACK */
static inline f32 pad_lift(VS *vs, f32 f, s32 range) {
    s32 near_top = func_802A7CB0(range, vs);

    if (!near_top && PAD_B_OR_Z != 0) {
        f = f - LIFT_STEP;
        if (f < 0.0f)
            f = 0.0f;
    } else if (!near_top && PAD_A != 0) {
        f = f + LIFT_STEP;
        if (!(f <= 1.0f))
            f = 1.0f;
    } else {
        f = step_toward_f(f, 0.5f, LIFT_BACK);
    }
    return f;
}

#endif
