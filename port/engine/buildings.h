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

/* Native-endian memory (docs/PORT.md): where the original reaches a field
   at another width than its type's (tools/recomp/native_sites.txt), a byte
   of a word (x3) or a half of one (x2) is at the address ^ 3 or ^ 2, and a
   table kept big-endian is swapped.  The identity in big-endian memory. */
#ifdef PORT_NATIVE_ENDIAN
#define NE_X3(off) ((off) ^ 3)
#define NE_X2(off) ((off) ^ 2)
#define NE_BE16(v) ((u16)__builtin_bswap16((u16)(v)))
#define NE_BE32(v) ((u32)__builtin_bswap32((u32)(v)))
#else
#define NE_X3(off) (off)
#define NE_X2(off) (off)
#define NE_BE16(v) ((u16)(v))
#define NE_BE32(v) ((u32)(v))
#endif

/* The effect records (debris, sparks): 0x38 bytes, a model's (sections
   0x30 and 0x34), D_803F3968[30] and D_803F3FF8 (the one being made).
   Fourteen words, which func_802C04F0 copies as such, and whose halves
   (0x2C, 0x2E) and bytes (0x2E's high one, 0x30-0x37) are read and
   written as the N64 has them. */
#define FX_W(d, off) (*(s32 *)((u8 *)(d) + (off)))
#define FX_H(d, off) (*(u16 *)((u8 *)(d) + NE_X2(off)))
#define FX_B(d, off) (*(u8 *)((u8 *)(d) + NE_X3(off)))

/* The collision tests' state (89250): the player's position, the
   camera's limits (12-bit headings) and what narrows them. */
extern s32 D_803A73F0, D_803A73F4, D_803A73F8;
extern u16 D_803A7410, D_803A7412;
extern u8 D_803A742F;                           /* limits off */
extern u8 D_803A7424, D_803A7425, D_803A7427, D_803A742A;
extern s8 *PTR32 D_803A7408;                    /* the kinds that turn the camera, to -1 */

/* the level's other solid objects: 0x14-byte records, to an `end` of -1 */
typedef struct Solid {
    /* 0x00 */ s32 x, y, z, r;
    /* 0x10 */ u8 kind;
    /* 0x11 */ s8 end;           /* -1 after the last; 0: not solid */
    /* 0x12 */ u8 pad12[2];
} Solid;
SIZE_CHECK(Solid, 0x14);
extern Solid D_803A7300[];

/* the kinds' parts: 0x14-byte records, to an `end` of -1 */
typedef struct KindPart {
    /* 0x00 */ s32 x, y, z, r;
    /* 0x10 */ u16 power;        /* the damage it does (func_802BEBB0) */
    /* 0x12 */ u8 kind;
    /* 0x13 */ s8 end;
} KindPart;
SIZE_CHECK(KindPart, 0x14);
extern KindPart D_803A6B30[];

/* the registers by number, for engine_save()'s masks (engine.h) */
enum {
    rAT = 1, rV0, rV1, rA0, rA1, rA2, rA3, rT0, rT1, rT2, rT3, rT4, rT5, rT6, rT7,
    rS0, rS1, rS2, rS3, rS4, rS5, rS6, rS7, rT8, rT9, rK0, rK1, rGP, rSP, rFP, rRA
};
#define G(r) ENGINE_GPR(r)
/* what the original saves and loads back, undone in the thread's context
   as it undoes it (the registers its translated callees, and their REGS(),
   leave there) */
#define ENGINE_SAVE(gmask) engine_save((gmask), 0)
#define ENGINE_RESTORE() engine_restore()

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
/* that point and piece p's corners, flattened along its axis (56040.c,
   native): the corners in 2D left in $v0-$a3, the point in $t0, $t1 */
REGS(s0, v1, a0, a1)
void func_8029C0DC(u8 *part, s32 x, s32 y, s32 z);
/* whether the 2D point (x, z) is inside the triangle */
REGS(v0, v1, a0, a1, a2, a3, t0, t1 -> t7)
s32 func_8029BF64(s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3, s32 x, s32 z);
/* whether the sphere crosses one of piece p's edges */
REGS(t3, t4, t5, t6, s0 -> t7)
s32 func_8029BD0C(s32 x, s32 y, s32 z, s32 r, Piece *p);
/* whether the sphere holds piece p's first corner (56040.c, native) */
REGS(s0, t3, t4, t5, t6 -> t7)
s32 func_8029BEE4(u8 *part, s32 x, s32 y, s32 z, s32 r);

/* what func_8029C0DC leaves, for func_8029BF64 */
#define C0DC_LEFT engine_ctx(2), engine_ctx(3), engine_ctx(4), engine_ctx(5), engine_ctx(6), engine_ctx(7), \
                  engine_ctx(8), engine_ctx(9)
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
REGS(t3, t9)
void func_802C0E8C(s32 group, Building *b);
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
REGS()
void func_802C049C(void);
REGS(v1)
void func_802C04F0(u8 *fx);

#endif
