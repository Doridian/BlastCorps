/*
 * The engine's shared functions as the native code calls them: REGS() and
 * the prototype of every native function the vehicle modules and the
 * others call (engine.h).  A declaration here and its definition must
 * agree (gen_glue.py, for the check build, refuses two REGS() for one
 * function).  Generated at first from the definitions; kept by hand, by
 * the object that defines them.
 */
#ifndef ENGINE_SHARED_H
#define ENGINE_SHARED_H

#include "engine.h"
#include "game/vehicle.h"

typedef UnkStruct_803ED460 Part;
typedef VehicleState VS;

/* 56040 */
REGS(v0 -> v0)
Part *func_802A06B4(void *key);
REGS(t1 -> t1)
s16 *func_8029FF2C(s16 *d);
REGS(v0, v1, a0)
void func_802A0290(s32 i, s32 v, Part *parts);
REGS(v0, v1)
void func_802A02E4(s32 i, Part *parts);
REGS(v0, v1)
void func_802A0320(s32 i, Part *parts);
REGS(v0, v1, a0, f0)
void func_802A0360(s32 i, s32 v, Part *parts, f32 f);
REGS(v0, v1, a0)
void func_802A039C(s32 i, s32 v, Part *parts);
REGS(v0, v1, a0)
void func_802A03D4(s32 i, s32 v, Part *parts);
REGS(v0, v1, a0)
void func_802A040C(s32 i, s32 v, Part *parts);
REGS(v0, v1, a0)
void func_802A0444(s32 i, s32 v, Part *parts);
REGS(v0, v1, a0, f0)
void func_802A0480(s32 i, s32 v, Part *parts, f32 f);
REGS(v0, v1 -> v1, a0, a1, a2, a3, t0, t1, f0)
s32 func_802A04BC(s32 i, Part *parts, s32 *f11, s32 *f12, s32 *f14, s32 *fC, s32 *fE, s32 *f13, f32 *f4);
REGS(v0, v1)
void func_802A0508(void *key, s32 v);
REGS(v0)
void func_802A0540(void *key);
REGS(v0)
void func_802A0570(void *key);
REGS(v0, v1, f0)
void func_802A05A4(void *key, s32 v, f32 f);
REGS(v0, v1)
void func_802A05D0(void *key, s32 v);
REGS(v0, v1)
void func_802A05F8(void *key, s32 v);
REGS(v0, v1)
void func_802A0620(void *key, s32 v);
REGS(v0, v1)
void func_802A0648(void *key, s32 v);
REGS(v0)
void func_802A0674(void *key);

/* 56040: an object's parts from its model, and their animations each frame */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2);
REGS(t0, v0, v1)
void func_8029E558(Part *parts, u8 *buf, u8 *other);

/* 5CB60: the Vehicle record for a type,
   from its model file (s2) and buffers */
REGS(a0, a1, v0, v1, s2)
void func_802A1388(s32 type, s32 a1, u8 *buf1, u8 *buf2, u8 *model);

/* 56040: a vehicle's collisions each frame.  func_8029A800 sets up the
   frame's state: its point, the kinds whose hit turns the camera (`kinds`,
   to a negative byte), whether a hit makes the effect (hit_fx) and how
   long it is (fx_len * 7000), its speed and the speed a hit needs for the
   effect (fx_speed) */
REGS(v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t8, gp)
void func_8029A800(s32 x, s32 y, s32 z, u8 *kinds, s32 a2, s32 hit_fx, s32 fx_len, s32 speed, s32 fx_speed,
                   s32 t3, s32 type, VS *vs);
REGS(t8, gp)
void func_8029C52C(s32 type, VS *vs);

/* func_8029C0DC's work as C (collision.h's CollisionTri; buildings.h's
   Piece is the same record): a triangle's corners and a point on its
   plane, seen along the triangle's axis, in 2D; and func_8029BF64 on
   them, whether the point is inside.  77E20's and 89250's
       func_8029C0DC((u8 *)p, px, py, pz); ...; in = func_8029BF64(C0DC_LEFT);
   is
       FlatTri f; collision_flatten((void *)p, px, py, pz, &f); ...;
       in = collision_flat_inside(&f); */
typedef struct FlatTri {
    s32 u0, v0, u1, v1, u2, v2, pu, pv;
} FlatTri;
struct CollisionTri;
void collision_flatten(const struct CollisionTri *t, s32 x, s32 y, s32 z, FlatTri *f);
s32 collision_flat_inside(const FlatTri *f);
REGS()
void func_8029AA10(void);
REGS(t0, t1, t2, t3)
void func_8029C354(s32 type, u8 *a, u8 *b, s32 t3);
REGS(v0, v1, a0, t0, t1, t2, s4)
void func_8029C454(s32 x, s32 y, s32 z, s32 type, u8 *a, u8 *b, u8 *buf);
REGS(t0, t1, t2, t6, a1, a2, s4)
void func_8029D040(s32 x, s32 z, s32 type, u8 *t6, s32 heading, Part *parts, u8 *buf);
REGS(v0 -> v1)
s32 func_8029DC14(s32 type);
REGS(gp)
void func_8029A914(VS *vs);
REGS(v0, v1, a0)
void func_8029F9D4(s32 i, s32 v, Part *parts);

/* 5CB60 */
REGS(v0, v1, a0, a1, gp)
void func_802A133C(s32 x, s32 y, s32 z, s32 type, VS *vs);

/* 62740 */
REGS(t0, t1, t2, s4)
void func_802ABBEC(s32 id, u8 *verts, u8 *end, u8 *buf);
REGS(t3, t4, s4, s1, s2)
void func_802AABE4(s32 id, u16 *data, u8 *base, s32 s1, s32 s2);
REGS(a0, a1, f0, gp -> a0, a1)
s32 func_802A71DC(s32 h, s32 h2, f32 rate, VS *vs, s32 *a1);
REGS(t5, t6, t1, s4, s5, s6, s7, gp)
void func_802A7FD8(s32 rate, s16 *speed, u16 *angle, u16 *target, u16 *out, s8 *turning, s32 sound, VS *vs);
REGS(t6, t8, t7, s0, s7, f2, gp -> t1)
s32 func_802A843C(s16 *speed, s32 limit, s32 mode, s8 *wheels, s32 *h, f32 div, VS *vs);
REGS(t0, t1, t7, s1, s2, t8, t9, fp, v1, a1, a2, a3, t3, gp)
void func_802A8768(s32 x, s32 z, s32 *px, s32 *pz, s32 *py, s32 type, s32 t9, s32 fp, s16 *v1, s32 *a1, s32 *a2,
                   s32 *a3, s16 *t3, VS *vs);

/* 60F60 */
REGS(-> t0)
s32 func_802A5ED0(void);

/* 86ED0: getting in (the vehicle modules call it) */
REGS(gp)
void func_802CB690(VS *vs);

/* 77E20 */
REGS(t8, gp)
void func_802BE77C(s32 type, VS *vs);
REGS(v0 -> v1)
s32 func_802BCD80(s32 id);

/* 7F8B0: the vehicles' engine sounds */
REGS(a1)
void func_802C4310(s32 a1);
REGS()
void func_802C444C(void);
REGS(s5)
void func_802C4584(s32 speed);
REGS(a1)
void func_802C4724(s32 a1);

/* 62740 */
REGS(v1, t2, t7, s0, s1, s2, s4, t8, gp, fp -> s3, s5)
s32 *func_802A992C(s16 *pts, s32 y, s32 x, s32 z, s32 *out, s32 *avg, s16 *a, s32 self, VS *vs, s32 mat,
                   s32 *avg_out);
REGS(v1, t2, t7, s0, s1, s2, s4, t8, gp, fp -> s1)
s32 *func_802A9A60(s16 *pts, s32 y, s32 x, s32 z, s32 *out, s32 *avg, s16 *a, s32 self, VS *vs, s32 mat);
REGS(gp)
void func_802A6F00(VS *vs);
s32 func_802A6F6C(void);
REGS(a1, gp)
void func_802A6FE4(s32 limit, VS *vs);
REGS(t7, gp)
void func_802A7070(s16 *heading, VS *vs);
REGS(gp)
void func_802A70D8(VS *vs);
REGS(a1, gp)
void func_802A746C(s32 turn, VS *vs);
REGS(gp)
void func_802A754C(VS *vs);
REGS(v0, v1, a0, a1, gp)
void func_802A75DC(u8 *parts, s32 *x, s32 *y, s32 *z, u8 *vs);
REGS(v0, v1, a0, a1, a2, a3, t0, gp)
void func_802A768C(u8 *parts, s32 *x, s32 *y, s32 *z, u32 *src, u32 *dst, s32 n, u8 *vs);
REGS(a0, a1, a2)
void func_802A7764(u32 *a, u32 *b, s32 n);
REGS(gp)
void func_802A77D0(VS *vs);
REGS(t2, s1 -> t4)
s32 func_802A7A1C(s32 x, s16 *rows);
REGS(t2, s1 -> t4)
s32 func_802A7AAC(s32 x, s16 *rows);
REGS(t2, s1, gp -> t4, s1)
s32 func_802A7C28(s32 x, s16 *rows, VS *vs, u32 *rows_out);
REGS(v1, gp -> v0)
s32 func_802A7CB0(s32 range, VS *vs);
REGS(t7, s0 -> t3)
s32 func_802A7D68(s32 mode, u8 *flags);
REGS(s3, s4 -> s3, t0, t2)
s32 func_802A7E70(s32 rate, u16 *h, u32 *stick_addr, s32 *stick);
REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA460(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3);
REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA5E0(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3);
REGS(s4, s5, s6, s7, t8 -> s2)
s32 *func_802AA764(s32 x, s32 y, s32 z, s32 scale, s32 *m);
void func_802AA6D0(s32 x, s32 y, s32 z, s32 ax, s32 ay, s32 az, s32 scale, s32 *m);
REGS(t0, t1, t2)
void func_802AA838(u8 *a, u8 *b, s32 off);
REGS(v1, a0, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB1B0(s32 v1, s32 a0, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20);
REGS(v0, a0, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB234(s32 v0, s32 a0, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20);
REGS(v0, v1, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB2B8(s32 v0, s32 v1, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20);
REGS(v0, v1, a0, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB33C(s32 v0, s32 v1, s32 a0, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20);
s32 func_802AB3C0(s32 type);
REGS(t4, t8 -> a2)
s32 func_802AB41C(s32 a, s32 b);
REGS(t3, t4, t5, t6, t7, s0 -> s6)
s32 func_802ABB1C(s32 x, s32 z, s32 dx, s32 dz, s32 x2, s32 z2);
REGS(v0, v1 -> a0)
void *func_802ABC88(s32 id, s32 n);
REGS(t3, t4, t5, t6, t7, s0 -> s1+f0)
s64 func_802ABCDC(s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2);
REGS(a3, t3, t4, t5)
void func_802ABD54(s32 type, s32 x, s32 y, s32 z);
REGS(t5 -> t5)
s32 func_802A8314(s32 v);
REGS(t3, t6, s0, s7, t8 -> f12, t3)
f32 func_802A83B8(s32 t3, s16 *speed, u8 *flags, s32 *pos, f32 *ratio, s32 *t3_out);
REGS(s7 -> t1)
s32 func_802A8590(s32 *s7);
REGS(t4, t6, t7, s1, f12 -> t0, t1)
s32 func_802A860C(s32 angle, s16 *speed, s32 *x, s32 *z, f32 rate, s32 *z_out);
REGS(t3, t6, t7, s0, s1, s2, gp -> t2, t3)
s32 func_802A785C(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, VS *vs, s32 *step_out);
REGS(t3, t6, t7, s0, s1, s2, s3, s4, gp -> t2, t3, s3)
s32 func_802A7834(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, s32 rate, u16 *h, VS *vs,
                  s32 *step_out, s32 *turn_out);
REGS(t6, s1, s2, gp)
void func_802A7B3C(s16 *speed, s16 *rows, s32 brake, VS *vs);
void func_802A8FB4(void);
REGS(gp)
void func_802A8FF4(VS *vs);
REGS(gp)
void func_802A9038(VS *vs);
REGS(v1)
void func_802A90E4(u16 *g);
REGS(s0, t8, gp)
void func_802A9164(u8 *flags, s32 type, VS *vs);
REGS(v0, v1, s4 -> t5, t6)
s32 func_802A94A4(s32 i, s16 *pts, s16 *a, s32 *z_out);
REGS(s3 -> s3)
s32 func_802A9514(s32 v);
REGS(v0, a1, a2, a3, t2, s3)
void func_802A9540(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 v);

/* 679E0 */
void func_802AC1A0(s32 radius);
REGS(v0, v1, a0)
void func_802AC3B8(s32 *x, s32 *y, s32 *z);
s32 func_802AC4C4(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3);
void func_802AC544(s32 x, s32 y, s32 z);
void func_802AC61C(s32 x, s32 y, s32 z, s32 kind, s32 t1);
void func_802AC6FC(s32 x, s32 y, s32 z, s32 kind, s32 t1);
REGS(a0, a1, a2)
void func_802AC7DC(u8 *dst, u8 *src, u32 *words);
REGS(a0, a1, a2)
void func_802AC85C(u8 *src, u8 *dst, u32 *words);
REGS(s2)
void func_802AC8CC(u32 *m);
REGS(t0, t1, t2, t3)
void func_802ACA60(s32 x, s32 y, s32 z, s32 *m);
REGS(t0, t3)
void func_802ACAC4(s32 angle, s32 *m);
REGS(t0, t3)
void func_802ACB50(s32 angle, s32 *m);
REGS(t0, t3)
void func_802ACBDC(s32 angle, s32 *m);
REGS(t0, t1, t2, t3)
void func_802ACC68(s32 x, s32 y, s32 z, s32 *m);
REGS(a0, s2)
void func_802ACCCC(s32 *b, s32 *a);
REGS(a0, a2, a3 -> a1, a3, t1)
s64 func_802ACE38(s64 x, s64 z, s32 angle, s32 *xr, s32 *zr);
REGS(v1 -> fp)
s32 func_802ACF64(u32 x);
s32 func_802ACF3C(s32 x);

/* 69014 */
REGS(v1 -> fp)
s32 func_802AD7FC(u32 x);
s32 func_802AD7D4(s32 x);

/* 60F60 and 5BF40: start an effect (a debris or smoke
   particle; it leaves at, a3, t2-t6 and s1 itself), and load a texture */
REGS(t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, a3 -> t0)
s32 func_802A6274(s32 t0, s32 t1, s32 t2, s32 t3, s32 t4, s32 t5, s32 t6, s32 t7, s32 s0, s32 s1, s32 s2,
                  s32 s3, s32 s4, s32 s5, s32 a3);
REGS(t6, s1, fp)
void func_802A1074(u32 id, u32 dst, u32 param);

/* 77E20 and 89250: a building kind into this frame's list,
   two list resets, and the angle test (it leaves f0 and f2) */
REGS(fp)
void func_802BCCD4(s32 kind);
REGS()
void func_802BCBD8(void);
REGS()
void func_802BCC10(void);
REGS(v0, a0, a2, t0)
void func_802CE204(s32 x, s32 z, s32 tx, s32 tz);

/* 69944 */
REGS(f12 -> f0)
f32 func_802AE1BC(f32 x);
REGS(f12 -> f0)
f32 func_802AE290(f32 x);
REGS(v1 -> fp)
s32 func_802AE104(s32 angle);
REGS(v1 -> fp)
s32 func_802AE160(s32 angle);

/* 62740's carrying (62740_carry.c): func_802AB478 and func_802AB670 walk
   D_803ED3B8's links (a carried vehicle's type, its carrier's) and call
   the carried vehicle's two callbacks with the carrier's id in $a3.  The
   first keeps where the vehicle stands in the carrier's frame (its point
   and the point ahead of it, func_802AAD0C, into unk6A..72); the second,
   after the carrier moved, puts it back there (func_802AB9A4,
   func_802AAE54) and steps it on the ground (func_802A8768). */
REGS(a3, t0, t1 -> t3, t4)
s32 func_802AAD0C(s32 id, s32 x, s32 z, s32 *v_out);
REGS(a3, t0, t1 -> t3, t4)
s32 func_802AAE54(s32 id, s32 x, s32 z, s32 *v_out);
REGS(a3, t0, t1, s1, s2, v1, a2 -> t0)
s32 func_802AB9A4(s32 id, s32 x, s32 z, s32 x2, s32 z2, s16 *pts, u16 *angle);
REGS(a3)
void func_802CA34C(s32 carrier);
REGS(a3)
void func_802CA3D8(s32 carrier);
REGS(a3)
void func_802B30F4(s32 carrier);
REGS(a3)
void func_802B3180(s32 carrier);
REGS(a3)
void func_802B4818(s32 carrier);
REGS(a3)
void func_802B48A4(s32 carrier);
REGS(a3)
void func_802B6100(s32 carrier);
REGS(a3)
void func_802B618C(s32 carrier);
REGS(a3)
void func_802B78F4(s32 carrier);
REGS(a3)
void func_802B7980(s32 carrier);
REGS(a3)
void func_802CBD5C(s32 carrier);
REGS(a3)
void func_802CBDE8(s32 carrier);
REGS(a3)
void func_802CCED4(s32 carrier);
REGS(a3)
void func_802CCF60(s32 carrier);
REGS(a3)
void func_802CFC54(s32 carrier);
REGS(a3)
void func_802CFCE0(s32 carrier);
REGS(a3)
void func_802C59B4(s32 carrier);
REGS(a3)
void func_802C5A14(s32 carrier);

/* the first callback of a wheeled vehicle at (x, z) (the original saves
   $t0, $t1, $t3 and $t4 and loads them back) */
#define CARRY_KEEP(b0, b1, b2, b3, vs, x, z)                                 \
    do {                                                                     \
        s32 t3_, t4_, t5_, t6_;                                              \
        ENGINE_BLK(b0);                                                      \
        t3_ = func_802AAD0C(carrier, (x), (z), &t4_);                        \
        ENGINE_BLK(b1);                                                      \
        (vs)->unk6A = t3_;                                                   \
        (vs)->unk6C = t4_;                                                   \
        (vs)->unk6E = (vs)->unk4C;                                           \
        t5_ = func_802A94A4(0, (vs)->unk52, (s16 *)&(vs)->unk4C, &t6_);      \
        ENGINE_BLK(b2);                                                      \
        t3_ = func_802AAD0C(carrier, (x) + t5_, (z) + t6_, &t4_);            \
        ENGINE_BLK(b3);                                                      \
        (vs)->unk70 = t3_;                                                   \
        (vs)->unk72 = t4_;                                                   \
    } while (0)

/* the second, for a wheeled vehicle at (*px, *py, *pz) of the given type
   and spans, its camera and matrix functions called as `camera` and
   `matrix`, `flag` before func_802A8768 (the original saves $a3, $t0,
   $t1 and $t2 in its frame of 0x28 and loads them back) */
#define CARRY_MOVE(b0, b1, b2, b3, b4, b5, b6, vs, px, py, pz, type, t9, fp, flag, camera, matrix) \
    do {                                                                     \
        s32 t0_, t3_, t4_;                                                   \
        ENGINE_BLK(b0);                                                      \
        t0_ = func_802AB9A4(carrier, (vs)->unk6A, (vs)->unk6C, (vs)->unk70, (vs)->unk72, (vs)->unk52, \
                            (u16 *)&(vs)->unk6E);                            \
        ENGINE_BLK(b1);                                                      \
        (vs)->unk4E = t0_;                                                   \
        (vs)->unk4C = t0_;                                                   \
        t3_ = func_802AAE54(carrier, (vs)->unk6A, (vs)->unk6C, &t4_);        \
        ENGINE_BLK(b2);                                                      \
        camera;                                                              \
        ENGINE_BLK(b3);                                                      \
        flag;                                                                \
        func_802A8768(t3_, t4_, (px), (pz), (py), (type), (t9), (fp), (vs)->unk52, (vs)->unk28, (vs)->unk28 + 6, \
                      (vs)->unk28 + 3, (vs)->unk5E, (vs));                   \
        ENGINE_BLK(b4);                                                      \
        matrix;                                                              \
        ENGINE_BLK(b5);                                                      \
        func_802A133C(*(px), *(py), *(pz), (type), (vs));                    \
        ENGINE_BLK(b6);                                                      \
    } while (0)

#endif
