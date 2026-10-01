/*
 * The run-time collision triangles (hd_code 5CB60's func_802A41B0 makes
 * them from the level's and the models' file triangles; 56040, 89250,
 * 8A080 and 77E20 read them).
 */
#ifndef ENGINE_COLLISION_H
#define ENGINE_COLLISION_H

#include "engine.h"
#include "game/objects.h"

typedef struct CollisionTri {
    /* 0x00 */ s64 nx, ny, nz;      /* the edges' cross product */
    /* 0x18 */ s64 d;               /* -(n . v1) */
    /* 0x20 */ f32 nlen;            /* |n| */
    /* 0x24 */ f32 nlen2;           /* |n|^2 */
    /* 0x28 */ s32 v[3][3];         /* the corners, file units << 3 */
    /* 0x4C */ u16 unk4C;           /* the file triangle's unk12 */
    /* 0x4E */ u8 axis;             /* the normal's largest component: 0 x, 1 y, 2 z */
    /* 0x4F */ u8 owner;            /* 0, or what the collision code skips it for */
    /* 0x50 */ u8 unk50;
    /* 0x51 */ u8 unk51;            /* in use */
    /* 0x52 */ u16 unk52;
    /* 0x54 */ u8 unk54;
    /* 0x55 */ u8 unk55;
    /* 0x56 */ u8 unk56;
    /* 0x57 */ u8 unk57;
    /* 0x58 */ u8 unk58;
    /* 0x59 */ u8 unk59;
    /* 0x5A */ u8 pad5A[6];
} CollisionTri;
SIZE_CHECK(CollisionTri, 0x60);

/* func_802A41B0 (5CB60): the triangle at $t4 from the file's at $t5;
   0x52 from $t2, 0x56 from $t7, 0x55 from $gp (and 0x4F, 0x50, 0x57, 0x58
   from $t9, $v0, $t6, $s1).  Returns $t4 advanced past it; leaves $s1 one
   of its differences (it is the next one's 0x58 where callers don't set
   it). */
REGS(t4, t5, t2, t7, gp -> t4)
u32 func_802A41B0(u32 t, u32 src, u32 h52, u32 b56, u32 b55);

#endif
