#ifndef GAME_CAMERA_H
#define GAME_CAMERA_H

#include "game/types.h"

/*
 * The gameplay camera: separate variables in hd.c's (hd_code 00000.c's)
 * .bss.  Nothing reaches them through a base pointer (tools/fieldscan.py
 * finds no view of any of them) and IDO addresses each by its own symbol,
 * so they were separate globals in the original too; this is the list of
 * them, with what hd.c's view setup (func_8024C414's camera code, and the
 * guLookAt/guLookAtReflect/guPerspective calls) does with each.
 *
 * Units: the level data's world units << 5 for positions the game logic
 * uses (D_803643E0..E8, the player), << 16 for the camera's own s32s (hd.c
 * shifts << 5 positions left by 11 into them and divides by 65536 for
 * guLookAt), f32 world units for the look-at point.  Angles are 0..0xFFF
 * for a full turn.
 */

/* The eye's target position (<< 16): hd.c sets it behind the vehicle (or
 * from the chopper's D_803FCD48..50, or an RDU's position) each frame. */
extern s32 D_803643EC;
extern s32 D_803643F0;
extern s32 D_803643F4;

/* The eye, in world units << 16 (hd.c divides by 65536 for guLookAt; >> 11
 * gives the << 5 units the level data uses).  It follows D_803643EC..F4. */
extern s32 D_803643F8;
extern s32 D_803643FC;
extern s32 D_80364400;

/* A position << 16 set from D_802E8F74 (1D990.c) when a level starts. */
extern s32 D_80364404;
extern s32 D_80364408;
extern s32 D_8036440C;

/* guPerspective's fovy (45.0 after a level's setup, func_80262150). */
extern f32 D_80364438;

extern s16 D_8036443C;          /* compared with 0 and 40 (46F60.c, 4B5E0.c, 20460.c) */
extern s16 D_8036443E;          /* the heading the camera turns to, 0..0xFFF (30C70.c turns the map by it) */
extern s16 D_80364440;
extern f32 D_80364444;          /* the eye's distance, eased towards its target each frame */
extern f32 D_80364448;          /* the eye's heading, eased towards D_8036443E, 0..4095 */

/* The view's yaw from the eye to the look-at point, 0..0xFFF for a full
 * turn (hd.c works it out per quadrant, 3E4C0.c converts it to degrees),
 * and its pitch; both go to func_80271FD0. */
extern s16 D_80364452;
extern s16 D_80364454;

/* The look-at point's target, then the look-at point itself (f32 world
 * units: guLookAt's at-x/y/z), which eases towards it; D_80365090..98 is
 * the difference, damped by 0.95 a frame. */
extern f32 D_80365078;
extern f32 D_8036507C;
extern f32 D_80365080;
extern f32 D_80365084;
extern f32 D_80365088;
extern f32 D_8036508C;
extern f32 D_80365090;
extern f32 D_80365094;
extern f32 D_80365098;

extern s16 D_8036509C;          /* the chopper camera's angle, 0..0x3FFF (from D_803FCD68 * 16) */
extern s16 D_8036509E;          /* which camera: the VehicleType followed, or 0xFD..0xFF */
extern s16 D_803650A0;

#endif
