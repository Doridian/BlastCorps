/*
 * The engine's shared functions as the native code calls them: REGS() and
 * the prototype of every native function the vehicle modules and the
 * others call (engine.h).  A declaration here and its definition must
 * agree; one for a still-translated function belongs here too, so that two
 * modules don't give it different REGS() (gen_glue.py refuses that).
 * Generated at first from the definitions; kept by hand.
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

/* 56040, still translated */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2);
REGS(t0, v0, v1)
void func_8029E558(Part *parts, u8 *buf, u8 *other);

/* 5CB60 (engine-B's), still translated: the Vehicle record for a type,
   from its model file (s2) and buffers */
REGS(a0, a1, v0, v1, s2)
void func_802A1388(s32 type, s32 a1, u8 *buf1, u8 *buf2, u8 *model);

/* 60F60 (engine-B's), still translated: start an effect (a debris or smoke
   particle) */
REGS(t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, a3 -> t0)
s32 func_802A6274(s32 t0, s32 t1, s32 t2, s32 t3, s32 t4, s32 t5, s32 t6, s32 t7, s32 s0, s32 s1, s32 s2,
                  s32 s3, s32 s4, s32 s5, s32 a3);

/* 56040, still translated: the parts' frame */
REGS(v0, v1, a0, a1, a2, a3, t1, t3, t8, gp)
void func_8029A800(s32 x, s32 y, s32 z, u8 *a1, s32 a2, s32 a3, s32 speed, s32 t3, s32 type, VS *vs);
REGS(t8, gp)
void func_8029C52C(s32 type, VS *vs);
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

/* 5CB60 (engine-B's), still translated */
REGS(v0, v1, a0, a1, gp)
void func_802A133C(s32 x, s32 y, s32 z, s32 type, VS *vs);

/* 62740, still translated */
REGS(t0, t1, t2, s4)
void func_802ABBEC(s32 id, u8 *verts, u8 *end, u8 *buf);
REGS(t3, t4, s4)
void func_802AABE4(s32 id, u8 *t4, u8 *buf);
REGS(a0, a1, f0 -> a0, a1)
s32 func_802A71DC(s32 h, s32 h2, f32 rate, s32 *a1);
REGS(t1, t5, t6, s4, s5, s6, s7, gp)
void func_802A7FD8(s16 *t1, s32 t5, s16 *speed, u16 *s4, u16 *s5, u8 *s6, s32 s7, VS *vs);
REGS(t6, t7, s0, s7, t8, f2, gp)
void func_802A843C(s16 *speed, s32 type, u8 *flags, s32 *pos, s32 t8, f32 f2, VS *vs);
REGS(t0, t1, t7, s1, s2, t8, t9, fp, v1, a1, a2, a3, t3, gp)
void func_802A8768(s32 x, s32 z, s32 *px, s32 *pz, s32 *py, s32 type, s32 t9, s32 fp, s16 *v1, s32 *a1, s32 *a2,
                   s32 *a3, s16 *t3, VS *vs);

/* 60F60 (engine-B's), still translated */
REGS(-> t0)
s32 func_802A5ED0(void);

/* 86ED0: getting in (the vehicle modules call it) */
REGS(gp)
void func_802CB690(VS *vs);

/* 77E20 (engine-B's), still translated */
REGS(t8, gp)
void func_802BE77C(s32 type, VS *vs);
REGS(v0 -> v1)
s32 func_802BCD80(s32 id);
REGS()
void func_802BCC10(void);

/* 7F8B0 (engine-B's), still translated: the vehicles' engine sounds */
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

/* 69944 */
REGS(f12 -> f0)
f32 func_802AE1BC(f32 x);
REGS(f12 -> f0)
f32 func_802AE290(f32 x);
REGS(v1 -> fp)
s32 func_802AE104(s32 angle);
REGS(v1 -> fp)
s32 func_802AE160(s32 angle);

#endif
