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

/* ---- the controller, as 45BB0.c's func_8028AE88... polls it each frame ---- */

/* one byte a button (1 held), and the stick (45BB0.c) */
extern u8 D_80370C15, D_80370C16, D_80370C17, D_80370C18, D_80370C19, D_80370C1A, D_80370C1B, D_80370C1C,
    D_80370C1D, D_80370C1E, D_80370C1F, D_80370C20, D_80370C21, D_80370C22, D_80370C23;
extern s8 D_80370C2C, D_80370C2D;
#define PAD_LEFT D_80370C15     /* the d-pad's left (0x200) */
#define PAD_RIGHT D_80370C16    /* its right (0x100) */
#define PAD_L D_80370C1A        /* the L trigger (0x20) */
#define PAD_R D_80370C1B        /* the R trigger (0x10) */
#define PAD_A D_80370C1C        /* A (0x8000) */
#define PAD_B D_80370C1D        /* B (0x4000) */
#define PAD_Z D_80370C22        /* Z (0x2000): the brake */
#define PAD_B_OR_Z D_80370C23
#define STICK_X D_80370C2C      /* the stick, -80..80 or so: x steers */
#define STICK_Y D_80370C2D      /* y is the throttle */
#define STICK_RANGE 0x50        /* what a full push is taken as (func_802A7E70, func_802A7A1C) */

#endif
