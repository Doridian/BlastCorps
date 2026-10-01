/*
 * What hd_code 77E20 (the buildings and their destruction), 89250 (the
 * collision tests against them) and 8A2E0 (the communication point) share,
 * as the native code sees it (engine.h).
 *
 * A level's buildings are objects.h's Building records (D_803F4030 up to
 * D_803F7654).  Each has a model file (Building.model) and two heap blocks:
 * unk4..unk8 is a list of 0x60-byte pieces, the planes it collides with,
 * which the destruction switches off group by group.  The model file holds
 * offsets (from its start) to its sections; the ones read here are below.
 * A building's damage is a byte per group, 0..100, from Building + 0xEC
 * (group g at 0xEC + g - 1); 100 is destroyed.
 */
#ifndef ENGINE_BUILDINGS_H
#define ENGINE_BUILDINGS_H

#include "shared.h"
#include "game/objects.h"

/* a piece: the plane n.p + d = 0 (s64, 16.16 by the >> 2 positions), two
   scale factors, three corners (the plane's triangle or box, << 5 >> 2) */
typedef struct Piece {
    /* 0x00 */ s64 nx;
    /* 0x08 */ s64 ny;
    /* 0x10 */ s64 nz;
    /* 0x18 */ s64 d;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ s32 p[3][3];
    /* 0x4C */ u16 heading;      /* the side it pushes to (func_802BF264) */
    /* 0x4E */ s8 axis;          /* the axis it is flattest along (func_8029C0DC) */
    /* 0x4F */ u8 pad4F[2];
    /* 0x51 */ u8 active;
    /* 0x52 */ u16 group;        /* the damage group it belongs to (1-based) */
    /* 0x54 */ u8 unk54;
    /* 0x55 */ u8 unk55;
    /* 0x56 */ u8 unk56;
    /* 0x57 */ u8 group2;        /* a group whose destruction brings it back */
    /* 0x58 */ u8 pushes;        /* it turns the camera (func_802BF264) */
    /* 0x59 */ u8 pad59[7];
} Piece;
SIZE_CHECK(Piece, 0x60);

#define B_MODEL(b) ((u8 *)(b)->model)
/* a section of building b's model file: the word at `off` is its offset */
#define B_SECTION(b, off) (B_MODEL(b) + *(s32 *)(B_MODEL(b) + (off)))
#define B_DAMAGE(b) ((u8 *)(b) + 0xEC)

/* divu, as the VR4300 (and the translation) gives it for 0 */
static inline u32 engine_divu(u32 n, u32 d) { return d != 0 ? n / d : 0xFFFFFFFFu; }
static inline u32 engine_remu(u32 n, u32 d) { return d != 0 ? n % d : n; }

/* ---- 56040's (engine-A's), still translated: the collision tests ---- */

/* whether the spheres (x, y, z, r) and (bx, by, bz, br) overlap */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t2)
s32 func_8029CFA4(s32 x, s32 y, s32 z, s32 r, s32 bx, s32 by, s32 bz, s32 br);
/* whether the sphere (x, y, z, r) crosses piece p's plane; where it does,
   the point on the plane (*px, *py, *pz) */
REGS(t3, t4, t5, t6, s0 -> v0, v1, a0, a1)
s32 func_8029C160(s32 x, s32 y, s32 z, s32 r, Piece *p, s32 *px, s32 *py, s32 *pz);
/* that point and piece p's corners, flattened along its axis: the corners
   in 2D (*v0, *v1 ... *a3) and the point (*t0, *t1) */
REGS(v1, a0, a1, s0 -> v0, v1, a0, a1, a2, a3, t0, t1)
s32 func_8029C0DC(s32 px, s32 py, s32 pz, Piece *p, s32 *v1, s32 *a0, s32 *a1, s32 *a2, s32 *a3, s32 *t0,
                  s32 *t1);
/* whether the 2D point (x, z) is inside the triangle */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t7)
s32 func_8029BF64(s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3, s32 x, s32 z);
/* whether the sphere crosses one of piece p's edges */
REGS(t3, t4, t5, t6, s0 -> t7)
s32 func_8029BD0C(s32 x, s32 y, s32 z, s32 r, Piece *p);
/* whether the sphere holds piece p's first corner */
REGS(t3, t4, t5, t6, s0 -> t7)
s32 func_8029BEE4(s32 x, s32 y, s32 z, s32 r, Piece *p);
/* the camera's heading kept between two angles */
REGS(a0, a1)
void func_8029B7CC(s32 a, s32 b);
/* the vehicle collision against the buildings (89250's func_802CE5BC) */
REGS(a1, a2, a3, t3, t4, t5, t6, t8, fp)
void func_8029B02C(s32 a1, s32 a2, s32 a3, s32 x, s32 y, s32 z, s32 r, s32 t8, s32 fp);

/* ---- 77E20 (this module's), as 89250 and 8A2E0 call it ---- */
REGS(-> t1)
s32 func_802BD8C8(void);
REGS(s0)
void func_802BF264(Piece *p);
REGS(t3, t5, t9)
void func_802BF898(s32 group, s32 damage, Building *b);
REGS(t3, t9)
void func_802BF1F0(s32 group, Building *b);
REGS(t3, t9)
void func_802C1438(s32 group, Building *b);
REGS(t3, t9)
void func_802C09B8(s32 group, Building *b);
REGS(t9)
void func_802C0E8C(Building *b);
REGS(t9)
void func_802BF384(Building *b);
REGS(t9)
void func_802BF668(Building *b);
REGS(t9)
void func_802BF534(Building *b);
REGS()
void func_802BCBD8(void);
REGS(a1, t3, t4, t5, t6)
void func_802C18D4(s32 a1, s32 x, s32 y, s32 z, s32 t6);

#endif
