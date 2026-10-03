/*
 * The collision data the engine shares (hd_code 5CB60 builds it, 56040,
 * 89250, 8A080 and 77E20 read it):
 *
 *   - CollisionTri: a 0x60-byte triangle with its plane.  The level's
 *     (func_802A3D54/func_802A3DF8: the grid cells, D_803BDCA8 and
 *     D_803BDE40; func_802A3E9C: the walls, D_803BD308..D_803BD30C), the
 *     objects' (D_803B9890..D_803BD300, from their models: 56040's
 *     func_8029D24C, and the level's own: func_802A3F80), and the
 *     buildings' pieces (77E20's Piece, buildings.h: the same record).
 *   - Solid: a kind's bounding sphere (D_803A7300, 12 of them).
 *   - KindPart: the spheres that make up a kind (D_803A6B30, 100).
 *
 * Positions are the game's (<< 5) >> 2 unless said otherwise.
 */
#ifndef ENGINE_COLLISION_H
#define ENGINE_COLLISION_H

#include "engine.h"
#include "game/objects.h"

typedef struct CollisionTri {
    /* 0x00 */ s64 nx, ny, nz;      /* the edges' cross product */
    /* 0x18 */ s64 d;               /* -(n . v[0]): n.p + d = 0 on the plane */
    /* 0x20 */ f32 nlen;            /* |n| */
    /* 0x24 */ f32 nlen2;           /* |n|^2 */
    /* 0x28 */ s32 v[3][3];         /* the corners */
    /* 0x4C */ u16 heading;         /* 12-bit: the way its face points in the
                                       ground plane (func_8029D56C; the level's
                                       from the file) */
    /* 0x4E */ u8 axis;             /* the normal's largest component: 0 z,
                                       1 y, 2 x (the one func_8029C0DC drops) */
    /* 0x4F */ u8 owner;            /* the kind (vehicle type) it belongs to,
                                       0 for the level's own */
    /* 0x50 */ u8 id;               /* its number within its owner (1 on),
                                       which D_803BC1D0's lists name */
    /* 0x51 */ u8 active;           /* tested at all */
    /* 0x52 */ u16 group;           /* a building's damage group (1 on) */
    /* 0x54 */ u8 side;             /* a wall's: the side of its plane the
                                       vehicle was on (1, 2; 0 not yet) */
    /* 0x55 */ u8 unk55;            /* 1: pushes the camera both ways */
    /* 0x56 */ u8 unk56;            /* faces away (func_8029D56C turned its
                                       heading round) */
    /* 0x57 */ u8 group2;           /* a group whose destruction brings it back */
    /* 0x58 */ u8 pushes;           /* it turns the camera (func_802BF264) */
    /* 0x59 */ u8 unk59;            /* a sound for D_803BE738 on a hit */
    /* 0x5A */ u8 pad5A[6];
} CollisionTri;
SIZE_CHECK(CollisionTri, 0x60);

#ifndef ENGINE_BUILDINGS_H
/* (buildings.h has these two as well, until the buildings' code takes
   them from here) */

/* a kind's bounding sphere: 0x14-byte records, to an `end` of -1 */
typedef struct Solid {
    /* 0x00 */ s32 x, y, z, r;
    /* 0x10 */ u8 kind;
    /* 0x11 */ s8 end;              /* -1 after the last; 1 live (placed this
                                       frame), 0 not */
    /* 0x12 */ u8 pad12[2];
} Solid;
SIZE_CHECK(Solid, 0x14);

/* the spheres a kind is made of: 0x14-byte records, to an `end` of -1 */
typedef struct KindPart {
    /* 0x00 */ s32 x, y, z, r;
    /* 0x10 */ u16 power;           /* the damage it does (func_802BEBB0) */
    /* 0x12 */ u8 kind;
    /* 0x13 */ s8 end;              /* -1 after the last; 1 placed, 0 not */
} KindPart;
SIZE_CHECK(KindPart, 0x14);
#endif

extern Solid D_803A7300[];
extern KindPart D_803A6B30[];

/* The level's and the objects' triangles */
extern CollisionTri D_803B9890[];               /* the objects', to D_803BD300 */
extern CollisionTri *PTR32 D_803BD300;
extern CollisionTri *PTR32 D_803BD308, *PTR32 D_803BD30C;  /* the walls' */

/* func_802A41B0 (5CB60): the triangle at $t4 from the file's at $t5;
   0x52 from $t2, 0x56 from $t7, 0x55 from $gp, 0x4F, 0x50, 0x57 and 0x58
   from $t9, $v0, $t6 and $s1.  Returns $t4 advanced past it. */
REGS(t4, t5, t2, t7, gp, t9, v0, t6, s1 -> t4)
u32 func_802A41B0(u32 t, u32 src, u32 h52, u32 b56, u32 b55, u32 b4f, u32 b50, u32 b57,
                  u32 b58);

#endif
