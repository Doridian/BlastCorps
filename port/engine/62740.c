/*
 * hd_code 62740 (us.v11 0x802A6F00-0x802AC1A0): the vehicles' shared
 * physics, as native C (engine.h).  Most of it works on a VehicleState
 * (vehicle.h) that every vehicle module passes in $gp: its headings, its
 * speed, its gear table and its wheels.  vehicle.h names the fields and
 * the per-frame constants (docs/PORT.md, "The engine made readable").
 */
#include "vehicle.h"
#include "game/game.h"
#include "level_tables.h"
#include "game/level.h"
#include "game/objects.h"

extern u8 D_80367C10;                   /* the level is one of 1D990.c's five (D_802E8F30): slower steering, gears doubled */
extern s16 D_803A7410, D_803A7412;      /* the camera's headings (12-bit) */
extern s32 D_80358064;                  /* the game's frame in this mode */
extern u8 D_8035805C;                   /* which of the two display buffers is this frame's */
extern u8 D_803EB7A0[];                 /* a vehicle's parts, state and position, saved */

/* an unaligned word, as swl/swr and lwl/lwr move it */
typedef struct {
    u32 v;
} __attribute__((packed)) UnalignedWord;

/* ---- per-frame constants of the shared physics (vehicle.h has the rest) --- */

#define SPEED_TO_LIMIT_STEP 3   /* func_802A6FE4: the speed toward its limit, a frame */
#define SPEED_STOP_STEP 8       /* func_802A77D0: toward 0 with the stick let go, a frame */
#define SPEED_OVER_GEARS_STEP 6 /* func_802A785C, func_802A7B3C: lost a frame above the gear table */
#define BUTTON_GEAR_STEP 4      /* func_802A7B3C: a gear row's acceleration times this, a frame */
#define SLOPE_PUSH (-4.0f)      /* func_802A843C: the wheels' height difference over the grip divisor, times this, a frame */
#define ROLL_FRICTION_HEAVY 3   /* func_802A843C: lost a frame on the ground (modes 0, 2, 9, 0x10) */
#define ROLL_FRICTION_LIGHT 1   /* ... in the others */
#define TURN_SNAP 0x156         /* func_802A7FD8: a move heading this near the target is there */
#define TURN_SOUND_FRAMES 7     /* func_802A7FD8: the turning sound every this many frames */
#define WHEEL_LIFT 0x78         /* func_802A9B1C: a wheel's point is looked for this far above it */
#define WHEEL_RISE_MAX 0x240    /* func_802A9514: the most a wheel rises a frame leaving the ground */
#define WHEEL_LAND_SLACK 0x1E   /* func_802A93B0: a wheel within this of the ground lands */
#define CAMERA_MARGIN 0x78      /* func_802A71DC: the camera's range narrowed by this at each end */

/* the gear table's speeds doubled on D_80367C10's levels */
REGS(gp)
void func_802A6F00(VS *vs) {
    s16 *row = VS_GEARS(vs);
    s32 n;

    if (D_80367C10 != 0) {
        for (n = 0; n < GEAR_ROWS; n++, row += 3) {
            row[GEAR_LO] <<= 1;
            row[GEAR_HI] <<= 1;
        }
    }
}

/* the angle halfway between the two camera headings (12-bit, wrapping) */
s32 func_802A6F6C(void) {
    s32 a = (u16)D_803A7410, b = (u16)D_803A7412, r;

    if (b < a) {
        r = ((ANGLE_WRAP - a + b) >> 1) + a;
        if (r >= ANGLE_TURN)
            r -= ANGLE_WRAP;
    } else {
        r = (u32)(a + b) >> 1;
    }
    return r;
}

/* once the mode has run a frame: the speed brought toward `limit` (or
   -limit) by SPEED_TO_LIMIT_STEP, not past it; its sign kept */
REGS(a1, gp)
void func_802A6FE4(s32 limit, VS *vs) {
    s32 v;

    if (D_80358064 == 0)
        return;
    v = VS_SPEED(vs);
    VS_SPEED_SIGN(vs) = v > 0 ? 1 : -1;
    if (v >= 0) {
        v -= SPEED_TO_LIMIT_STEP;
        if (v <= limit)
            v = limit;
    } else {
        v += SPEED_TO_LIMIT_STEP;
        if (v >= -limit)
            v = -limit;
    }
    VS_SPEED(vs) = v;
}

/* the move heading: *heading, turned round when the speed's sign isn't the
   one func_802A6FE4 kept */
REGS(t7, gp)
void func_802A7070(s16 *heading, VS *vs) {
    s32 sign = VS_SPEED(vs) > 0 ? 1 : -1, v = *heading;

    if (VS_SPEED_SIGN(vs) != sign) {
        v -= ANGLE_HALF;
        if (v < 0)
            v += ANGLE_WRAP;
    }
    VS_MOVE_HEADING(vs) = v;
}

/* when the move heading is more than a quarter turn from the heading: it
   is turned round, the speed negated, and the camera's headings turned
   half round */
REGS(gp)
void func_802A70D8(VS *vs) {
    s32 h = VS_HEADING(vs), lo, hi, a = VS_MOVE_HEADING(vs);

    lo = wrap_fff(h - ANGLE_QUARTER);
    hi = wrap_fff(h + ANGLE_QUARTER);
    if (hi < lo) {
        if (a >= lo || a <= hi)
            return;
    } else if (a >= lo && a <= hi) {
        return;
    }
    VS_MOVE_HEADING(vs) = wrap_fff(a + ANGLE_HALF);
    VS_SPEED(vs) = -VS_SPEED(vs);
    D_803A7410 = wrap_fff((u16)D_803A7410 - ANGLE_HALF);
    D_803A7412 = wrap_fff((u16)D_803A7412 + ANGLE_HALF);
}

/* the heading turned by `turn` toward the move heading, not past it */
REGS(a1, gp)
void func_802A746C(s32 turn, VS *vs) {
    s32 h = VS_HEADING(vs), target = VS_MOVE_HEADING(vs), n, past;

    if (h == target)
        return;
    n = wrap_fff(h + turn);
    if (turn >= 0) {
        if (h < target)
            past = target < n || n < h;
        else
            past = target < h && n < h && target < n;
    } else {
        if (target < h)
            past = !(target < n) || !(n < h);
        else
            past = h < target && n < target && h < n;
    }
    VS_HEADING(vs) = past ? target : n;
}

/* the state at rest */
REGS(gp)
void func_802A754C(VS *vs) {
    s32 k;

    VS_AIRBORNE(vs)[0] = 0;
    VS_AIRBORNE(vs)[1] = 0;
    VS_AIRBORNE(vs)[2] = 0;
    VS_GRIP(vs) = 1;
    vs->unkA0 = 1;
    for (k = 0; k < 9; k++)
        VS_WHEEL_FALL(vs)[k] = 0;
    VS_SPEED(vs) = 0;
    VS_TURNING(vs) = 0;
    VS_ON_STATIC(vs) = 0;
    vs->unk9C = 0;
    vs->unk9D = 0;
    vs->unk9E = 0;
    vs->unkA1 = 0;
    vs->unkA2 = 0;
    vs->unkA3 = 0;
    vs->unkA4 = 0;
    VS_SPEED_SIGN(vs) = 0;
    VS_SLOPE_RATIO(vs) = 0.0f;
}

/* the current vehicle's parts (0x300 bytes), state (0xA6) and position
   (three words) to D_803EB7A0 */
REGS(v0, v1, a0, a1, gp)
void func_802A75DC(u8 *parts, s32 *x, s32 *y, s32 *z, u8 *vs) {
    u8 *d = D_803EB7A0;
    s32 n;

    for (n = 0; n < 0x300; n++)
        *d++ = *parts++;
    for (n = 0; n < 0xA6; n++)
        *d++ = *vs++;
    ((UnalignedWord *)d)[0].v = *x;
    ((UnalignedWord *)d)[1].v = *y;
    ((UnalignedWord *)d)[2].v = *z;
}

/* ... and back, with `n` bytes from src to dst (in doublewords) */
REGS(v0, v1, a0, a1, a2, a3, t0, gp)
void func_802A768C(u8 *parts, s32 *x, s32 *y, s32 *z, u32 *src, u32 *dst, s32 n, u8 *vs) {
    u8 *s = D_803EB7A0;
    s32 k;

    for (k = 0; k < 0x300; k++)
        *parts++ = *s++;
    for (k = 0; k < 0xA6; k++)
        *vs++ = *s++;
    for (; n != 0; n -= 8, src += 2, dst += 2) {
        dst[0] = src[0];
        dst[1] = src[1];
    }
    *x = ((UnalignedWord *)s)[0].v;
    *y = ((UnalignedWord *)s)[1].v;
    *z = ((UnalignedWord *)s)[2].v;
}

/* n bytes (in doublewords) from a to b, or b to a on the other buffer */
REGS(a0, a1, a2)
void func_802A7764(u32 *a, u32 *b, s32 n) {
    u32 *t;

    if (D_8035805C != 0)
        t = a, a = b, b = t;
    for (; n != 0; n -= 8, a += 2, b += 2) {
        b[0] = a[0];
        b[1] = a[1];
    }
}

/* the stick let go: the speed toward 0 by SPEED_STOP_STEP */
REGS(gp)
void func_802A77D0(VS *vs) {
    s32 v;

    if (STICK_Y != 0)
        return;
    v = VS_SPEED(vs);
    if (v >= 0) {
        v -= SPEED_STOP_STEP;
        if (v < 0)
            v = 0;
    } else {
        v += SPEED_STOP_STEP;
        if (v > 0)
            v = 0;
    }
    VS_SPEED(vs) = v;
}

/* whether x is past the first gear row's lowest speed scaled by the
   stick pulled back (STICK_Y / -STICK_RANGE) */
REGS(t2, s1 -> t4)
s32 func_802A7A1C(s32 x, s16 *rows) {
    return x < rows[0] * STICK_Y / -STICK_RANGE;
}

/* the same against the last row's highest, pushed forward */
REGS(t2, s1 -> t4)
s32 func_802A7AAC(s32 x, s16 *rows) {
    return rows[3 * (GEAR_ROWS - 1) + GEAR_HI] * STICK_Y / STICK_RANGE < x;
}

/* The acceleration of the gear row whose speeds hold x, over the rows
   the ground's grip allows (all five on grip 1, then 7 - grip; two fewer
   with D_803ED40C); 0 if none.  The rows pointer is left past the last
   row looked at. */
REGS(t2, s1, gp -> t4, s1)
s32 func_802A7C28(s32 x, s16 *rows, VS *vs, u32 *rows_out) {
    s32 n, r = 0;

    n = VS_GRIP(vs) == 1 ? GEAR_ROWS : 7 - VS_GRIP(vs);
    if ((s8)D_803ED40C != 0)
        n -= 2;
    for (; n != 0; n--) {
        rows += 3;
        if (x >= rows[GEAR_LO - 3] && x <= rows[GEAR_HI - 3]) {
            r = rows[GEAR_ACCEL - 3];
            break;
        }
    }
    *rows_out = (u32)rows;
    return r;
}

/* whether the speed is within `range` of the top speed of the last gear
   row the grip allows (the first row's lowest when reversing) */
REGS(v1, gp -> v0)
s32 func_802A7CB0(s32 range, VS *vs) {
    s32 v = VS_SPEED(vs), g;

    if (v >= 0) {
        g = VS_GRIP(vs) == 1 ? GEAR_ROWS - 1 : 6 - VS_GRIP(vs);
        v -= VS_GEARS(vs)[3 * g + GEAR_HI];
    } else {
        v -= VS_GEARS(vs)[GEAR_LO];
    }
    return iabs(v) < range;
}

/* the gear rows' acceleration factor (0..4) for which wheels are on the
   ground (flags: VS_AIRBORNE), by the vehicle's drive: mode 0 wheels 0 and
   1 driven, 1 wheel 2, 2 all three (wheel 2 counting double), 4 always
   full; the others (3) full with any wheel down */
REGS(t7, s0 -> t3)
s32 func_802A7D68(s32 mode, u8 *flags) {
    s32 r;

    switch (mode) {
    case 0:
        r = (flags[0] == 0 ? 2 : 0) + (flags[1] == 0 ? 2 : 0);
        break;
    case 1:
        r = flags[2] == 0 ? 4 : 0;
        break;
    case 2:
        r = (flags[0] == 0) + (flags[1] == 0) + (flags[2] == 0 ? 2 : 0);
        break;
    case 4:
        r = 4;
        break;
    default:
        r = flags[0] == 0 || flags[1] == 0 || flags[2] == 0 ? 4 : 0;
        break;
    }
    return r;
}

extern s8 D_80370C34;                   /* 45BB0.c: the J-Bomb's own steering */
extern u16 D_80370C70;
extern s16 D_80370C72;
extern s8 D_803649EE;

/* Steering: the heading *h turned by the stick's x times the turn rate
   (`rate`; a third less on D_80367C10's levels, 100 or 200 with
   D_80370C75), over STICK_RANGE; or D_803ED408 with D_803ED40A.  Returns
   the turn, and the stick's x and its address as the original leaves them
   for its callers. */
REGS(s3, s4 -> s3, t0, t2)
s32 func_802A7E70(s32 rate, u16 *h, u32 *stick_addr, s32 *stick) {
    s32 x = STICK_X, t = *h;

    if (D_80367C10 != 0)
        rate -= rate / 3;
    if (D_80370C75 != 0)
        rate = D_80370C34 != 0 ? 0x64 : 0xC8;
    D_80370C72 = rate;
    rate = iabs(x) * rate / STICK_RANGE;
    if (x != 0) {
        t = x >= 0 ? t - rate : t + rate;
        if (t < 0)
            t += ANGLE_TURN;
        if (t >= ANGLE_TURN)
            t -= ANGLE_TURN;
    }
    if (D_803ED40A != 0 && D_803649EE == 0)
        t = D_803ED408;
    *h = t;
    D_80370C70 = t;
    *stick_addr = (u32)&STICK_X;
    *stick = x;
    return rate;
}

/* ---- triangles under a point ---------------------------------------------- */

/* whether (x, z) is inside the triangle (x1, z1), (x2, z2), (x3, z3): on
   the same side of each edge as the point halfway between the first
   corner and the middle of the opposite edge (a point on an edge's line
   counts as inside for that edge) */
REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA460(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    f32 fx = (f32)x, fz = (f32)z, cx, cz, ex, ez, d, c, dx, dz;
    s32 edge;

    cx = (f32)(x2 + x3) / 2.0f;
    cz = (f32)(z2 + z3) / 2.0f;
    cx = ((f32)x1 + cx) / 2.0f;
    cz = ((f32)z1 + cz) / 2.0f;
    for (edge = 0; edge < 3; edge++) {
        switch (edge) {
        case 0: /* first to second */
            ex = (f32)x1, ez = (f32)z1;
            dx = (f32)(z2 - z1), dz = (f32)(x2 - x1);
            break;
        case 1: /* first to third */
            ex = (f32)x1, ez = (f32)z1;
            dx = (f32)(z3 - z1), dz = (f32)(x3 - x1);
            break;
        default: /* second to third */
            ex = (f32)x2, ez = (f32)z2;
            dx = (f32)(z3 - z2), dz = (f32)(x3 - x2);
            break;
        }
        d = (fx - ex) * dx - (fz - ez) * dz;
        if (d == 0.0f)
            continue;
        c = (cx - ex) * dx - (cz - ez) * dz;
        if (d > 0.0f ? !(c > 0.0f) : !(c < 0.0f))
            return 0;
    }
    return 1;
}

/* whether (x, z) is inside the triangle's bounding box */
static inline s32 in_box(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    s32 lox = x1, hix = x1, loz = z1, hiz = z1;

    if (x2 < lox)
        lox = x2;
    if (x3 < lox)
        lox = x3;
    if (hix < x2)
        hix = x2;
    if (hix < x3)
        hix = x3;
    if (z2 < loz)
        loz = z2;
    if (z3 < loz)
        loz = z3;
    if (hiz < z2)
        hiz = z2;
    if (hiz < z3)
        hiz = z3;
    return x >= lox && x <= hix && z >= loz && z <= hiz;
}

REGS(t0, t1, s1, s3, s4, s6, s7, t9 -> v0)
s32 func_802AA5E0(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    return in_box(x, z, x1, z1, x2, z2, x3, z3);
}

/* both tests on a triangle's (x, z) corners: under the point */
static s32 tri_under(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3) {
    return func_802AA5E0(x, z, x1, z1, x2, z2, x3, z3) && func_802AA460(x, z, x1, z1, x2, z2, x3, z3);
}

/* ---- matrices -------------------------------------------------------------- */

/* D_803ED390[3] (vehicle.h): the angles func_802AA764 rotates by: x, y, z */
extern s32 D_803EBB58[16];      /* its scratch matrix */

void func_802ACC68(s32 x, s32 y, s32 z, s32 *m);
void func_802ACBDC(s32 angle, s32 *m);
void func_802ACB50(s32 angle, s32 *m);
void func_802ACAC4(s32 angle, s32 *m);
void func_802ACA60(s32 x, s32 y, s32 z, s32 *m);
void func_802ACCCC(s32 *b, s32 *a);
void func_802AC8CC(u32 *m);

/* the Mtx at m: scaled, rotated about x, z and y by D_803ED390's angles,
   and moved to (x, y, z) << 5 */
REGS(s4, s5, s6, s7, t8 -> s2)
s32 *func_802AA764(s32 x, s32 y, s32 z, s32 scale, s32 *m) {
    func_802ACC68(scale, scale, scale, m);
    func_802ACBDC((u16)D_803ED390[0], D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802ACB50((u16)D_803ED390[2], D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802ACAC4((u16)D_803ED390[1], D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802ACA60(x << 11, y << 11, z << 11, D_803EBB58);
    func_802ACCCC(D_803EBB58, m);
    func_802AC8CC((u32 *)m);
    return m;
}

/* the same, with the angles as arguments */
void func_802AA6D0(s32 x, s32 y, s32 z, s32 ax, s32 ay, s32 az, s32 scale, s32 *m) {
    D_803ED390[1] = ay;
    D_803ED390[0] = ax;
    D_803ED390[2] = az;
    func_802AA764(x, y, z, scale, m);
}

/* 64 bytes from a + off to b + off (a matrix into the other buffer) */
REGS(t0, t1, t2)
void func_802AA838(u8 *a, u8 *b, s32 off) {
    u32 *s = (u32 *)(a + off), *d = (u32 *)(b + off);
    s32 n;

    for (n = 0; n < 16; n++)
        d[n] = s[n];
}

/* where the line through two points meets another (the four forms of the
   same solve, for an edge parallel to an axis): f10 and f20 */
REGS(v1, a0, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB1B0(s32 v1, s32 a0, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 b = (f32)(t0 - s7) / (f32)v1;

    *f20 = b;
    return ((f32)a1 * b + (f32)t9 - (f32)t1) / (f32)a0;
}

REGS(v0, a0, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB234(s32 v0, s32 a0, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 a = (f32)(s7 - t0) / (f32)v0;

    *f20 = ((f32)a0 * a + (f32)t1 - (f32)t9) / (f32)a1;
    return a;
}

REGS(v0, v1, a1, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB2B8(s32 v0, s32 v1, s32 a1, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 b = (f32)(t1 - t9) / (f32)a1;

    *f20 = b;
    return ((f32)v1 * b + (f32)s7 - (f32)t0) / (f32)v0;
}

REGS(v0, v1, a0, t0, t1, s7, t9 -> f10, f20)
f32 func_802AB33C(s32 v0, s32 v1, s32 a0, s32 t0, s32 t1, s32 s7, s32 t9, f32 *f20) {
    f32 a = (f32)(t9 - t1) / (f32)a0;

    *f20 = ((f32)v0 * a + (f32)t0 - (f32)s7) / (f32)v1;
    return a;
}

/* ---- D_803ED3B8: the vehicles' records of what their wheels stand on ------ */

/* (type, three bytes: each wheel's object, func_802A992C) in words, to a
   word of -1 */
extern u8 D_803ED3B8[];
#define RECORDS_END(p) (*(s32 *)(p) == -1)

/* whether the first record of this type has wheel 0 on something */
s32 func_802AB3C0(s32 type) {
    u8 *p;

    for (p = D_803ED3B8; !RECORDS_END(p); p += 4)
        if (p[0] == type)
            return p[1] != 0;
    return 0;
}

/* whether there is a record of type a with wheel 0 on b */
REGS(t4, t8 -> a2)
s32 func_802AB41C(s32 a, s32 b) {
    u8 *p;

    for (p = D_803ED3B8; !RECORDS_END(p); p += 4)
        if (p[1] == b && p[0] == a)
            return 1;
    return 0;
}

REGS(v1 -> fp)
s32 func_802AD7FC(u32 x);

/* the angle at (x2, z2) of the corner... (an arctangent of the ratio of
   the two distances), in 1/8ths of func_802AD7FC's */
REGS(t3, t4, t5, t6, t7, s0 -> s6)
s32 func_802ABB1C(s32 x, s32 z, s32 dx, s32 dz, s32 x2, s32 z2) {
    s32 a = x - x2, b = z - z2, r;
    f32 d1, d2;

    d1 = __builtin_sqrtf((f32)((s64)a * a + (s64)b * b));
    a = x2 + dx - x;
    b = z2 + dz - z;
    d2 = __builtin_sqrtf((f32)((s64)a * a + (s64)b * b));
    d2 = d2 / 2.0f;
    d2 = d2 / d1;
    r = engine_cvt_w_s(d2 * 65536.0f);
    if (r >= 0x10000)
        r = 0xFFFF;
    return (u32)func_802AD7FC(r) >> 3;
}

/* D_803EBC10: 16-byte records of the vehicles' points (func_802ABBEC):
   (x, y, z), the vehicle's id in byte 12 */
extern u8 D_803EBC10[];

/* the n-th (from 1) point of this id */
REGS(v0, v1 -> a0)
void *func_802ABC88(s32 id, s32 n) {
    u8 *p = D_803EBC10;

    while (p[0xC] != id)
        p += 0x10;
    return p + (u32)(n - 1) * 0x10;
}

/* the distance between two points, rounded (64 bits) */
REGS(t3, t4, t5, t6, t7, s0 -> s1+f0)
s64 func_802ABCDC(s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2) {
    s32 dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;

    return engine_cvt_l_d(__builtin_sqrt((f64)((s64)dx * dx + (s64)dy * dy + (s64)dz * dz)));
}

/* the level's lights: level_tables.h's LevelLight */
extern u8 D_80364A6E;                 /* the level's ambient light */

/* does this light, d away, light this type? */
static s32 light_reaches(LevelLight *p, s64 d, s32 type) {
    s32 n;

    if ((s64)p->radius < d || p->on == 0)
        return 0;
    for (n = 0; n < p->ntypes; n++)
        if (p->types[n] == type)
            return 1;
    return 0;
}

/* The light on the vehicle of this type at (x, y, z): the first light in
   range that lists the type, faded with the distance toward the ambient
   (or full), or the level's ambient; into its Vehicle's unk60. */
REGS(a3, t3, t4, t5)
void func_802ABD54(s32 type, s32 x, s32 y, s32 z) {
    LevelLight *p;
    s64 d = 0;
    s32 r = D_80364A6E, amb, level, radius;
    Vehicle *v;

    for (p = D_803BDFD8; p != D_803BDFD4; p++) {
        d = func_802ABCDC(x, y, z, p->x, p->y, p->z);
        if (!light_reaches(p, d, type))
            continue;
        radius = p->radius;
        level = p->shade;
        amb = D_80364A6E;
        if (level == 0)
            r = p->full == 1 ? 0xFF : 0xFF - amb - (s32)((u32)((0xFF - amb) * (u32)d) / (u32)radius) + amb;
        else
            r = p->full == 1 ? level : level + (s32)((u32)((amb - level) * (u32)d) / (u32)radius);
        break;
    }
    for (v = D_80364460; v->type != type; v++)
        ;
    v->unk60 = r;
}

/* ---- the turn rate's override on a level's start -------------------------- */

extern s32 D_803A740C;                  /* the frame the level (re)started */
extern s32 D_802E8BDC;                  /* the level */
extern s32 D_80358068;                  /* the frame count */
extern s32 D_80305C58[];                /* (level, rate, frames) triples, to a rate of 0 */
#define RATE_OVERRIDE_FRAMES 10

/* the turn rate v, or for the first RATE_OVERRIDE_FRAMES after the level
   (re)started the level's own from D_80305C58 while its frames last */
REGS(t5 -> t5)
s32 func_802A8314(s32 v) {
    s32 base = D_803A740C, now = D_80358068, *p;

    if (base + RATE_OVERRIDE_FRAMES < now)
        return v;
    for (p = D_80305C58; p[1] != 0; p += 3)
        if (p[0] == D_802E8BDC && p[2] + base >= now)
            return p[1];
    return v;
}

/* ---- moving ------------------------------------------------------------------ */

/* The slope: how much the front wheel's height changed (pos: VS_WHEEL_H)
   for the speed *speed, kept in *ratio; the old ratio when a wheel is in
   the air or the speed is 0.  *t3_out: the speed (t3 if it didn't look). */
REGS(t3, t6, s0, s7, t8 -> f12, t3)
f32 func_802A83B8(s32 t3, s16 *speed, u8 *flags, s32 *pos, f32 *ratio, s32 *t3_out) {
    if (flags[1] != 1 && flags[2] != 1 && flags[0] != 1) {
        t3 = *speed;
        if (t3 != 0)
            *ratio = (f32)(pos[0] - pos[1]) / (f32)t3;
    }
    *t3_out = t3;
    return *ratio;
}

/* of the front wheel's and the second's height over the third's
   (VS_WHEEL_H), the one nearer 0 */
REGS(s7 -> t1)
s32 func_802A8590(s32 *s7) {
    s32 a = s7[0] - s7[6], b = s7[3] - s7[6];

    return iabs(b) < iabs(a) ? b : a;
}

/* (*x, *z) moved by the speed in the direction `angle`: slower up a slope
   (rate: func_802A83B8's ratio; by up to half below 1, divided by twice
   it from 1).  Returns the new x, and z in *z_out. */
REGS(t4, t6, t7, s1, f12 -> t0, t1)
s32 func_802A860C(s32 angle, s16 *speed, s32 *x, s32 *z, f32 rate, s32 *z_out) {
    s32 v = *speed, rem, s, c, dx, dz, rx, rz, px = *x, pz = *z;
    f32 a = __builtin_fabsf(rate);

    if (v != 0) {
        if (!(a >= 1.0f))
            v = engine_cvt_w_s((1.0f - a / 2.0f) * (f32)v);
        else
            v = engine_cvt_w_s((f32)v / (a * 2.0f));
    }
    /* the sine and cosine of the angle within its quarter, then the quarter */
    rem = angle % ANGLE_QUARTER;
    s = func_802AE160(rem);
    dx = (s32)(v * s) >> 16;
    c = func_802AE104(rem);
    dz = (s32)(v * c) >> 16;
    if (angle < ANGLE_QUARTER)
        rx = px + dx, rz = pz + dz;
    else if (angle < ANGLE_HALF)
        rx = px + dz, rz = pz - dx;
    else if (angle < ANGLE_HALF + ANGLE_QUARTER)
        rx = px - dx, rz = pz - dz;
    else
        rx = px - dz, rz = pz + dx;
    *z_out = rz;
    return rx;
}

/* ---- the speed: throttle, brake and gears ----------------------------------- */

extern s16 D_803ED400;                  /* the speed after the last throttle step */

/* the speed toward 0 by `brake`, not past it, signed by `sign` */
static s32 brake_to_zero(s32 v, s32 sign, s32 brake) {
    if (sign > 0) {
        v -= brake;
        if (v <= 0)
            v = 0;
    } else {
        v += brake;
        if (v >= 0)
            v = 0;
    }
    return v;
}

/* The speed (*speed, VS_SPEED) a frame: braking (Z) toward 0 by `brake`
   while it was moving; otherwise, with D_803ED40C (not on level 0x22 or
   in a barge), held to half the top speeds; then the stick's y:
   pushed against the motion it brakes by `brake`, with it it accelerates
   by the gear row's acceleration times the drive's factor (func_802A7D68
   of `mode`, given back in *step_out), or loses SPEED_OVER_GEARS_STEP
   above the rows, up to the top speed scaled by the stick.  D_803ED400
   gets the result.  Returns what the original leaves in $t2 (772A0.c's
   train reads it): the stick's y when it is 0, else the speed it worked
   on, or D_803ED400 when braking. */
REGS(t3, t6, t7, s0, s1, s2, gp -> t2, t3)
s32 func_802A785C(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, VS *vs, s32 *step_out) {
    s32 t2, g;
    u32 rows2;

    if (PAD_Z != 0) {
        t2 = D_803ED400;
        if (t2 != 0)
            *speed = brake_to_zero(*speed, *speed > 0 ? 1 : -1, brake);
        goto done;
    }
    if ((s8)D_803ED40C != 0 && D_802E8BDC != 0x22 && D_80364456 != VEHICLE_BARGE &&
        D_80364456 != VEHICLE_BARGE_2 && D_80364456 != VEHICLE_BARGE_3) {
        if (*speed >= 0) {
            g = rows[3 * (GEAR_ROWS - 1) + GEAR_HI] >> 1;
            if (g < *speed)
                *speed = g;
        } else {
            g = rows[GEAR_LO] >> 1;
            if (*speed < g)
                *speed = g;
        }
    }
    step = func_802A7D68(mode, flags);
    t2 = STICK_Y;
    if (t2 == 0)
        goto done;
    t2 = *speed;
    if (STICK_Y > 0) {
        if (t2 < 0) {
            t2 += brake;
            *speed = t2;
        } else if (!func_802A7AAC(t2, rows)) {
            g = func_802A7C28(t2, rows, vs, &rows2);
            t2 = g != 0 ? t2 + g * step : t2 - SPEED_OVER_GEARS_STEP;
            *speed = t2;
        }
    } else {
        if (t2 > 0) {
            t2 -= brake;
            *speed = t2;
        } else if (!func_802A7A1C(t2, rows)) {
            t2 -= func_802A7C28(t2, rows, vs, &rows2) * step;
            *speed = t2;
        }
    }
done:
    D_803ED400 = *speed;
    *step_out = step;
    return t2;
}

REGS(s3, s4 -> s3, t0, t2)
s32 func_802A7E70(s32 rate, u16 *h, u32 *stick_addr, s32 *stick);

/* the steering, then the speed: a turn and the gears in one */
REGS(t3, t6, t7, s0, s1, s2, s3, s4, gp -> t2, t3, s3)
s32 func_802A7834(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, s32 rate, u16 *h, VS *vs,
                  s32 *step_out, s32 *turn_out) {
    u32 sa;
    s32 st;

    *turn_out = func_802A7E70(rate, h, &sa, &st);
    return func_802A785C(step, speed, mode, flags, rows, brake, vs, step_out);
}

/* The speed for a vehicle driven by the triggers (L or R accelerates):
   braking (Z) toward 0 by `brake` as D_803ED400 was signed; going
   backwards it gains `brake`; otherwise the gear row's acceleration times
   BUTTON_GEAR_STEP, or SPEED_OVER_GEARS_STEP lost above the rows. */
REGS(t6, s1, s2, gp)
void func_802A7B3C(s16 *speed, s16 *rows, s32 brake, VS *vs) {
    s32 v, g;
    u32 rows2;

    if (PAD_Z != 0) {
        if (D_803ED400 != 0)
            *speed = brake_to_zero(*speed, D_803ED400, brake);
    } else if (PAD_L != 0 || PAD_R != 0) {
        v = *speed;
        if (v < 0) {
            *speed = v + brake;
        } else {
            g = func_802A7C28(v, rows, vs, &rows2);
            *speed = g != 0 ? v + g * BUTTON_GEAR_STEP : v - SPEED_OVER_GEARS_STEP;
        }
    }
    D_803ED400 = *speed;
}

/* ---- the wheels' bookkeeping ------------------------------------------------- */

/* per wheel, wheel i's at [i] (each one variable: asm2c.py's LABEL_TYPES) */
extern u8 D_803ED3EE[4];                /* in the air (1) this frame */
extern u8 D_803ED3F2[3];                /* the material under it */
extern u8 D_803ED3EA[4];                /* the moving object it stands on (0: none) */
extern s32 D_803ED398[4];               /* its height */
extern s32 D_803ED3A8[4];               /* the ground's height under it */
extern u8 D_803BE738;                   /* the level is lost (1D990.c) */
extern u8 D_803ED410;                   /* the vehicle is one point (all its wheel offsets 0) */
#define WHEEL_AIRBORNE(i) D_803ED3EE[i]
#define WHEEL_MATERIAL(i) D_803ED3F2[i]
#define WHEEL_OBJECT(i) D_803ED3EA[i]
#define WHEEL_HEIGHT(i) D_803ED398[i]

/* the vehicle's gravity a frame (the level's, LevelHeader.gravity, times
   the vehicle's factor: its camera function sets it), and how hard a
   wheel lands before it bounces (func_802A9710) */
extern f32 D_803EBBF0, D_803EBBF4;
#define LEVEL_GRAVITY D_803EBBF0
#define GRAVITY D_803EBBF4
extern u8 D_803ED3F6, D_803ED3F7;
#define LAND_BOUNCE_MIN D_803ED3F6      /* a landing harder than this bounces */
#define LAND_BOUNCE_DIV D_803ED3F7      /* ... at the speed over this */

/* materials (a wheel's triangle's byte): from 100 up special, counting as
   2 toward the grip */
#define MATERIAL_SPECIAL 100
#define MATERIAL_DEADLY 0x64            /* loses the level (with the mode run a frame) */
#define MATERIAL_DEADLY_ALWAYS 0x66     /* loses it any time */
#define MATERIAL_SOFT 0x67              /* on level 12 lands softer (LAND_BOUNCE_DIV 8) */

/* gravity tripled with D_803ED3F5 */
void func_802A8FB4(void) {
    if (D_803ED3F5 != 0)
        GRAVITY = GRAVITY * 3.0f;
}

/* landings softer on MATERIAL_SOFT on level 12 */
REGS(gp)
void func_802A8FF4(VS *vs) {
    if (VS_TOP_MATERIAL(vs) == MATERIAL_SOFT && D_802E8BDC == 0xC)
        LAND_BOUNCE_DIV = 8;
}

/* a one-point vehicle: wheel 0's everything copied to the other two */
REGS(gp)
void func_802A9038(VS *vs) {
    s32 k;

    WHEEL_AIRBORNE(1) = WHEEL_AIRBORNE(2) = WHEEL_AIRBORNE(0);
    WHEEL_HEIGHT(1) = WHEEL_HEIGHT(2) = WHEEL_HEIGHT(0);
    D_803ED3A8[1] = D_803ED3A8[2] = D_803ED3A8[0];
    for (k = 0; k < 9; k += 3)
        VS_WHEEL_FALL(vs)[k + 1] = VS_WHEEL_FALL(vs)[k + 2] = VS_WHEEL_FALL(vs)[k];
    WHEEL_MATERIAL(1) = WHEEL_MATERIAL(2) = WHEEL_MATERIAL(0);
    WHEEL_OBJECT(1) = WHEEL_OBJECT(2) = WHEEL_OBJECT(0);
}

/* D_803ED410: whether all three of a set of wheel offsets are 0 */
REGS(v1)
void func_802A90E4(u16 *g) {
    D_803ED410 = g[0] == 0 && g[1] == 0 && g[2] == 0 && g[3] == 0 && g[4] == 0 && g[5] == 0;
}

static s32 grip_part(s32 m) {
    return m < MATERIAL_SPECIAL ? m : 2;
}

/* The grip: the average of the wheels' materials (special ones as 2); the
   highest material in VS_TOP_MATERIAL and D_803ED40D; and the level lost
   (D_803BE738) on MATERIAL_DEADLY_ALWAYS, or on MATERIAL_DEADLY once the
   mode has run a frame (not for the J-Bomb or the barges), unless wheel
   0 is in the air. */
REGS(s0, t8, gp)
void func_802A9164(u8 *flags, s32 type, VS *vs) {
    s32 m = 0;

    VS_GRIP(vs) = (u32)(grip_part(WHEEL_MATERIAL(0)) + grip_part(WHEEL_MATERIAL(1)) + grip_part(WHEEL_MATERIAL(2))) / 3;
    if (m < WHEEL_MATERIAL(0))
        m = WHEEL_MATERIAL(0);
    if (m < WHEEL_MATERIAL(1))
        m = WHEEL_MATERIAL(1);
    if (m < WHEEL_MATERIAL(2))
        m = WHEEL_MATERIAL(2);
    VS_TOP_MATERIAL(vs) = m;
    D_803ED40D = m;
    if (D_803ED40D == MATERIAL_DEADLY_ALWAYS && flags[0] != 1)
        D_803BE738 = 1;
    if (type == VEHICLE_JETPACK || type == VEHICLE_BARGE || type == VEHICLE_BARGE_2 || type == VEHICLE_BARGE_3)
        return;
    if (D_80358064 != 0 && D_803ED40D == MATERIAL_DEADLY && flags[0] != 1)
        D_803BE738 = 1;
}

REGS(a0, a2, a3 -> a1, a3, t1)
s64 func_802ACE38(s64 x, s64 z, s32 angle, s32 *xr, s32 *zr);

/* wheel i's offset (pts: VS_WHEELS) turned by the heading *a */
REGS(v0, v1, s4 -> t5, t6)
s32 func_802A94A4(s32 i, s16 *pts, s16 *a, s32 *z_out) {
    s32 xr, zr;

    func_802ACE38(pts[2 * i], pts[2 * i + 1], *a, &xr, &zr);
    *z_out = zr;
    return xr;
}

/* a wheel's upward speed, at most WHEEL_RISE_MAX */
REGS(s3 -> s3)
s32 func_802A9514(s32 v) {
    return v > WHEEL_RISE_MAX ? WHEEL_RISE_MAX : v;
}

/* wheel i leaves the ground at height g going up by v a frame (at most
   WHEEL_RISE_MAX): its fall starts (h: VS_WHEEL_FALL, state its frames,
   ground where it left) */
REGS(v0, a1, a2, a3, t2, s3)
void func_802A9540(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 v) {
    h[i] = func_802A9514(v);
    state[i] = 2;
    ground[i] = g;
    WHEEL_AIRBORNE(i) = 1;
}

/* ---- the ground under a point: triangles, nearest in height ------------- */

#define NO_GROUND 99999999              /* a height search that found nothing */

extern f64 D_80305C50;

/* the height at (x, z) of the plane through three points (x1, y1, z1)...,
   in the world's units; -9999999 for a vertical one */
REGS(t0, t1, s1, s2, s3, s4, s5, s6, s7, t8, t9 -> v0)
s32 func_802AA2E4(s32 x, s32 z, s32 x1, s32 y1, s32 z1, s32 x2, s32 y2, s32 z2, s32 x3, s32 y3, s32 z3) {
    s32 a = y1 - y2, b = z1 - z3, c = z1 - z2, d = y1 - y3, e = x1 - x3, g = x1 - x2;
    s64 nx, ny, nz, w, t;
    f64 f;

    /* the plane's normal and its distance (64-bit, wrapping as the
       original's dmult/dsub) */
    nx = (s64)((u64)((s64)a * b) - (u64)((s64)c * d));
    ny = (s64)((u64)((s64)c * e) - (u64)((s64)g * b));
    nz = (s64)((u64)((s64)g * d) - (u64)((s64)a * e));
    w = -(s64)((u64)nx * (u64)(s64)x2 + (u64)ny * (u64)(s64)y2 + (u64)nz * (u64)(s64)z2);
    t = (s64)((u64)nx * (u64)(s64)x + (u64)ny * (u64)(s64)-1000 + (u64)nz * (u64)(s64)z + (u64)w);
    if (ny == 0)
        return -9999999;
    f = (f64)t / (f64)(s64)((u64)ny * 2000);
    f = D_80305C50 * __builtin_fabs(f);
    return (s32)engine_cvt_l_d(f) - 1000;
}

extern u8 D_803EBDB0[];                         /* the moving objects' triangles, 0x38 bytes each ... */
extern u8 *D_803EBC00, *D_803EBC04;               /* the cell looked in last */
extern u8 *D_803EBC08;                           /* the triangle found there */
extern u8 D_803EBBD8[];                         /* a triangle's room, for a swap */

/* a moving object's triangle: the world's (x, y, z) words, its own s16
   points from 0x24, its object's id at 0x36 */
#define MOVING_TRI_SIZE 0x38
#define MOVING_TRI_ID(p) (*(u16 *)((u8 *)(p) + 0x36))
#define MOVING_TRI_OWN(p) ((s16 *)((u8 *)(p) + 0x24))
/* a level triangle (the grid's, the objects', the water's): nine s16 >> 5,
   its material at 0x12 and whether it is solid at 0x13 */
#define LEVEL_TRI_SIZE 0x14

static u32 udist(s32 y, s32 h) {
    s32 a = y - h;

    return (u32)(a < 0 ? -a : a);
}

/* Of the static triangles (D_803F7828) under (x, z), the height nearest
   y (NO_GROUND if none) and its material (*mat_out, mat if none). */
REGS(t0, t1, t2, fp -> t3, fp)
s32 func_802A9DC0(s32 x, s32 z, s32 y, s32 mat, s32 *mat_out) {
    u8 *p;
    s32 best = NO_GROUND, h, *w;
    u32 dist = NO_GROUND, a;

    for (p = D_803F7828; p != D_803F782C; p += 0x28) {
        w = (s32 *)p;
        if (!tri_under(x, z, w[0], w[2], w[3], w[5], w[6], w[8]))
            continue;
        h = func_802AA2E4(x, z, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8]);
        a = udist(y, h);
        if (dist < a)
            continue;
        best = h;
        dist = a;
        mat = p[0x24];
    }
    *mat_out = mat;
    return best;
}

/* the same over the moving objects' triangles, but not object `self`'s:
   the height, and the object's id (0 if none) */
REGS(t0, t1, t2, t8 -> t3, t6)
s32 func_802A9F24(s32 x, s32 z, s32 y, s32 self, s32 *id_out) {
    u8 *p;
    s32 best = NO_GROUND, h, id = 0, k, *w;
    u32 dist = NO_GROUND, a;

    for (p = D_803EBDB0; p != D_803EBBEC; p += MOVING_TRI_SIZE) {
        k = MOVING_TRI_ID(p);
        if (k == self)
            continue;
        w = (s32 *)p;
        if (!tri_under(x, z, w[0], w[2], w[3], w[5], w[6], w[8]))
            continue;
        h = func_802AA2E4(x, z, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8]);
        a = udist(y, h);
        if (dist < a)
            continue;
        best = h;
        dist = a;
        id = k;
    }
    *id_out = id;
    return best;
}

/* The same over the level's triangles in the grid cell of (x, z), which it
   remembers (D_803EBC00..08); it stops at the first good one that is solid.
   Returns whether it found one; the height (*h_out, h if none) and its
   material (*mat_out, mat if none). */
REGS(t0, t1, t2, t3, fp -> a1, t3, fp)
s32 func_802AA094(s32 x, s32 z, s32 y, s32 h, s32 mat, s32 *h_out, s32 *mat_out) {
    s32 found = 0, hh, x1, z1, x2, z2, x3, z3;
    u32 dist = NO_GROUND, a;
    u8 **cell;
    u8 *p, *end;
    s16 *w;

    cell = &D_803BDB10[D_803BE720 * (z / (s32)D_803BE71C) + x / (s32)D_803BE718];   /* (the grid, game/level.h) */
    p = cell[0];
    end = cell[1] - 4;
    D_803EBC00 = p;
    D_803EBC04 = end;
    for (; p != end; p += LEVEL_TRI_SIZE) {
        w = (s16 *)p;
        x1 = w[0] << 5, z1 = w[2] << 5, x2 = w[3] << 5, z2 = w[5] << 5, x3 = w[6] << 5, z3 = w[8] << 5;
        if (!tri_under(x, z, x1, z1, x2, z2, x3, z3))
            continue;
        hh = func_802AA2E4(x, z, x1, w[1] << 5, z1, x2, w[4] << 5, z2, x3, w[7] << 5, z3);
        a = udist(y, hh);
        if (dist < a)
            continue;
        D_803EBC08 = p;
        found = 1;
        h = hh;
        dist = a;
        mat = p[0x12];
        if (p[0x13] != 0)
            break;
    }
    *h_out = h;
    *mat_out = mat;
    return found;
}

/* the triangle found in the cell (D_803EBC08) moved to the cell's first
   places (index i, or i + 3 for the missile carrier), swapping with the
   one there, so that it's found first next time (cells of 7 or more) */
REGS(v0, t8)
void func_802A9CAC(s32 i, s32 who) {
    u8 *q = D_803EBC00 + (u32)((who == VEHICLE_CMO ? 3 : 0) + i) * LEVEL_TRI_SIZE;
    u32 *found = (u32 *)D_803EBC08, *there = (u32 *)q, *tmp = (u32 *)D_803EBBD8;
    s32 n;

    if (D_803EBC04 - D_803EBC00 < 7 * LEVEL_TRI_SIZE || D_803EBC08 == q)
        return;
    for (n = 0; n < LEVEL_TRI_SIZE / 4; n++)
        tmp[n] = found[n];
    for (n = 0; n < LEVEL_TRI_SIZE / 4; n++)
        found[n] = there[n];
    for (n = 0; n < LEVEL_TRI_SIZE / 4; n++)
        there[n] = tmp[n];
}

/* Wheel i's ground at (x, z) for a wheel at height y: of the static
   triangles, the moving objects' (not `self`'s) and the level grid's, the
   height nearest y + WHEEL_LIFT (the grid's when it is as near as either,
   moving it to the cell's front); with none, y.  Its material (`mat`
   when it isn't the static's or the grid's; 1 on a moving object or
   nothing) and object in WHEEL_MATERIAL(i) and WHEEL_OBJECT(i), the
   height (0 or more) in D_803ED3A8[i]; VS_ON_STATIC set when the static
   triangle won.  Returns the height. */
REGS(v0, t0, t1, t2, t8, gp, fp -> t3)
s32 func_802A9B1C(s32 i, s32 x, s32 z, s32 y, s32 self, VS *vs, s32 mat) {
    s32 st, st_mat, mov, obj, h, m, found, d_mov, d_st;

    y += WHEEL_LIFT;
    st = func_802A9DC0(x, z, y, mat, &st_mat);
    mov = func_802A9F24(x, z, y, self, &obj);
    found = func_802AA094(x, z, y, mov, st_mat, &h, &m);
    d_mov = iabs(mov - y);
    d_st = iabs(st - y);
    if (!found && st == NO_GROUND && mov == NO_GROUND) {
        h = y - WHEEL_LIFT;
        m = 1;
        obj = 0;
    } else if (found && d_mov >= iabs(h - y) && d_st >= iabs(h - y)) {
        func_802A9CAC(i, self);
        obj = 0;
    } else if (d_mov < d_st) {
        h = mov;
        m = 1;
    } else {
        VS_ON_STATIC(vs) = 1;
        h = st;
        m = st_mat;
        obj = 0;
    }
    WHEEL_OBJECT(i) = obj;
    WHEEL_MATERIAL(i) = m;
    if (h < 0)
        h = 0;
    D_803ED3A8[i] = h;
    return h;
}

/* ---- the three wheels on the ground -------------------------------------- */

/* the vehicle's record in D_803ED3B8 (`self` its type): its own, or the
   first free one (all 0xFF), which it takes */
static u8 *wheel_record(s32 self) {
    u8 *p;

    for (p = D_803ED3B8; p[0] != self; p += 4) {
        if (p[0] == 0xFF && RECORDS_END(p)) {
            p[0] = self;
            break;
        }
    }
    return p;
}

/* the wheels' objects into the vehicle's record */
static void wheel_record_set(s32 self) {
    u8 *p = wheel_record(self);

    p[1] = WHEEL_OBJECT(0);
    p[2] = WHEEL_OBJECT(1);
    p[3] = WHEEL_OBJECT(2);
}

/* The three wheels (pts: VS_WHEELS, turned by *a, from (x, z)) put on the
   ground, for a setup: a moving object's triangles first (not the
   vehicle's own), else the level grid's (the material `mat` when there is
   none).  Each height three times at out[3 * i] (VS_WHEEL_H), the
   average of the back two in *avg and *avg_out; the grip from the
   materials (plain average); the vehicle's record gets the wheels'
   objects.  Returns out. */
REGS(v1, t2, t7, s0, s1, s2, s4, t8, gp, fp -> s3, s5)
s32 *func_802A992C(s16 *pts, s32 y, s32 x, s32 z, s32 *out, s32 *avg, s16 *a, s32 self, VS *vs, s32 mat,
                   s32 *avg_out) {
    s32 i, dx, dz, h, id, back;

    for (i = 0; i < 3; i++) {
        dx = func_802A94A4(i, pts, a, &dz);
        h = func_802A9F24(x + dx, z + dz, y, self, &id);
        if (id == 0)
            func_802AA094(x + dx, z + dz, y, h, mat, &h, &mat);
        WHEEL_OBJECT(i) = id;
        out[3 * i] = out[3 * i + 1] = out[3 * i + 2] = h;
        WHEEL_MATERIAL(i) = mat;
    }
    back = (u32)(out[3] + out[6]) >> 1;
    *avg = back;
    VS_GRIP(vs) = (u32)(WHEEL_MATERIAL(0) + WHEEL_MATERIAL(1) + WHEEL_MATERIAL(2)) / 3;
    wheel_record_set(self);
    *avg_out = back;
    return out;
}

/* The same through func_802A9B1C (the nearest of all three grounds), for
   putting a vehicle down where it is; every wheel's material the caller's
   `mat`, the vehicle level (D_803ED390's x and z angles 0).  Returns the
   out table past its end. */
REGS(v1, t2, t7, s0, s1, s2, s4, t8, gp, fp -> s1)
s32 *func_802A9A60(s16 *pts, s32 y, s32 x, s32 z, s32 *out, s32 *avg, s16 *a, s32 self, VS *vs, s32 mat) {
    s32 i, dx, dz, h;

    for (i = 0; i < 3; i++) {
        dx = func_802A94A4(i, pts, a, &dz);
        VS_ON_STATIC(vs) = 0;
        h = func_802A9B1C(i, x + dx, z + dz, y, self, vs, mat);
        out[3 * i] = out[3 * i + 1] = out[3 * i + 2] = h;
        /* (the original's $fp: what the caller had, not the ground's) */
        WHEEL_MATERIAL(i) = mat;
    }
    *avg = (u32)(out[3] + out[6]) >> 1;
    VS_GRIP(vs) = (u32)(WHEEL_MATERIAL(0) + WHEEL_MATERIAL(1) + WHEEL_MATERIAL(2)) / 3;
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    return out + 9;
}

/* the three wheels (pts: VS_CARRY_WHEELS, at *a's angle around (x, z)) on
   the ground through func_802A9B1C, each at its own height
   (heights[3 * i]); then the vehicle's record */
REGS(t0, t1, t3, s4, s7, t8, gp, fp)
void func_802A92C8(s32 x, s32 z, s16 *pts, s16 *a, s32 *heights, s32 self, VS *vs, s32 mat) {
    s32 i, dx, dz;

    for (i = 0; i < 3; i++) {
        dx = func_802A94A4(i, pts, a, &dz);
        func_802A9B1C(i, x + dx, z + dz, heights[3 * i], self, vs, mat);
    }
    wheel_record_set(self);
}

/* Wheel i on the ground this frame: where it would be (its height,
   heights[3 * i], plus its rise since the last frame, at most
   WHEEL_RISE_MAX, plus gravity) against the ground under it.  Above it by
   WHEEL_LAND_SLACK or more, it leaves the ground (func_802A9540) and is
   there; otherwise it stays on the ground (WHEEL_AIRBORNE(i) 0) at its
   height.  WHEEL_HEIGHT(i) gets the result; returns the rise. */
REGS(v0, v1, a1, a2, a3, t0, t1, s4, s7, t8, gp, fp -> s3)
s32 func_802A93B0(s32 i, s16 *pts, s32 *h, s32 *state, s32 *ground, s32 x, s32 z, s16 *a, s32 *heights,
                  s32 self, VS *vs, s32 mat) {
    s32 dx, dz, now = heights[3 * i], rise, next, g;

    dx = func_802A94A4(i, pts, a, &dz);
    rise = func_802A9514(now - heights[3 * i + 1]);
    next = rise + engine_cvt_w_s(GRAVITY) + now;
    g = func_802A9B1C(i, x + dx, z + dz, now, self, vs, mat);
    if (next - WHEEL_LAND_SLACK >= g) {
        func_802A9540(i, h, state, ground, now, rise);
        g = next;
    } else {
        WHEEL_AIRBORNE(i) = 0;
    }
    WHEEL_HEIGHT(i) = g;
    return rise;
}

/* ---- the ground of the level's objects and of the water ---------------- */

extern s32 D_803EBBF8, D_803EBBFC;

/* Of an object model's solid triangles (byte 0x13 clear; the model's list
   at its offsets 0x24 to 0x28) under (x, z), the nearest at or below y: its
   height in D_803EBBF8.  Returns whether one was found (`found` if not). */
REGS(a1, t0, t1, t2, s0 -> a1)
s32 func_802ABFC8(s32 found, s32 x, s32 z, s32 y, u8 *model) {
    u8 *p = model + *(s32 *)(model + 0x24), *end = model + *(s32 *)(model + 0x28);
    u32 dist = NO_GROUND;
    s32 h, d, x1, z1, x2, z2, x3, z3;
    s16 *w;

    for (; p != end; p += LEVEL_TRI_SIZE) {
        if (p[0x13] != 0)
            continue;
        w = (s16 *)p;
        x1 = w[0] << 5, z1 = w[2] << 5, x2 = w[3] << 5, z2 = w[5] << 5, x3 = w[6] << 5, z3 = w[8] << 5;
        if (!tri_under(x, z, x1, z1, x2, z2, x3, z3))
            continue;
        h = func_802AA2E4(x, z, x1, w[1] << 5, z1, x2, w[4] << 5, z2, x3, w[7] << 5, z3);
        d = y - h;
        if (d < 0 || dist < (u32)d)
            continue;
        found = 1;
        D_803EBBF8 = h;
        dist = d;
    }
    return found;
}


/* whether (x, z) is in one of the level's objects (their model's
   rectangle, as two triangles) with ground under it at or below y
   (func_802ABFC8) */
s32 func_802ABEDC(s32 x, s32 y, s32 z) {
    u8 *p, *model;
    s16 *r;
    s32 found = 0, x1, z1, x2, z2;

    for (p = (u8 *)D_803F4030; p != (u8 *)D_803F7654; p += 0xFC) {   /* the level's objects, 0xFC bytes each */
        model = *(u8 *PTR32 *)p;
        r = (s16 *)(model + *(s32 *)(model + 0x20));
        x1 = r[0] << 5, z1 = r[1] << 5, x2 = r[2] << 5, z2 = r[3] << 5;
        if (func_802AA5E0(x, z, x1, z1, x2, z2, x2, z1) || func_802AA5E0(x, z, x1, z1, x2, z2, x1, z2))
            found = func_802ABFC8(found, x, z, y, model);
    }
    return found;
}

/* the same as func_802ABFC8 over the water's triangles (D_803BDAF4 to
   D_803BDAF8), at any height: the nearest's in D_803EBBFC */
REGS(t0, t1, t2 -> a1)
s32 func_802AC0BC(s32 x, s32 z, s32 y) {
    u8 *p;
    u32 dist = NO_GROUND, d;
    s32 h, found = 0, x1, z1, x2, z2, x3, z3;
    s16 *w;

    for (p = (u8 *)D_803BDAF4; p != (u8 *)D_803BDAF8; p += LEVEL_TRI_SIZE) {
        w = (s16 *)p;
        x1 = w[0] << 5, z1 = w[2] << 5, x2 = w[3] << 5, z2 = w[5] << 5, x3 = w[6] << 5, z3 = w[8] << 5;
        if (!tri_under(x, z, x1, z1, x2, z2, x3, z3))
            continue;
        h = func_802AA2E4(x, z, x1, w[1] << 5, z1, x2, w[4] << 5, z2, x3, w[7] << 5, z3);
        d = udist(y, h);
        if (dist < d)
            continue;
        found = 1;
        D_803EBBFC = h;
        dist = d;
    }
    return found;
}

/* ---- a point's place in another triangle (the moving objects' two
   shapes: their world triangle and their own) ------------------------------ */

/* The point (x, z) of triangle A ((x1, z1), (x2, z2), (x3, z3)) at the
   same place in triangle B ((u1, v1), ...): by the corners exactly, else
   by solving for its two coordinates in A: t, the fraction of the way
   from the first corner to the opposite edge, and e, along that edge from
   the third corner.  Returns u, and v in *v_out. */
REGS(t0, t1, s1, s3, s4, s6, s7, t9, t3, t4, t5, t6, t7, s0 -> t3, t4)
s32 func_802AAF64(s32 x, s32 z, s32 x1, s32 z1, s32 x2, s32 z2, s32 x3, s32 z3, s32 u1, s32 v1, s32 u2,
                  s32 v2, s32 u3, s32 v3, s32 *v_out) {
    s32 dx, ex, dz, ez;
    f32 t, e, den, a, b, fdx;

    if (x == x1 && z == z1) {
        *v_out = v1;
        return u1;
    }
    if (x == x2 && z == z2) {
        *v_out = v2;
        return u2;
    }
    if (x == x3 && z == z3) {
        *v_out = v3;
        return u3;
    }
    dx = x1 - x;
    ex = x2 - x3;
    dz = z1 - z;
    ez = z2 - z3;
    /* an edge along an axis: the simpler solves */
    if (dx == 0) {
        t = func_802AB1B0(ex, dz, ez, x, z, x3, z3, &e);
    } else if (ex == 0) {
        t = func_802AB234(dx, dz, ez, x, z, x3, z3, &e);
    } else if (dz == 0) {
        t = func_802AB2B8(dx, ex, ez, x, z, x3, z3, &e);
    } else if (ez == 0) {
        t = func_802AB33C(dx, ex, dz, x, z, x3, z3, &e);
    } else {
        /* (the original's order of operations, for the same rounding) */
        fdx = (f32)dx;
        den = (f32)((s64)ez * dx);
        a = 1.0f - (f32)((s64)ex * dz) / den;
        t = (f32)((s64)ex * z) / den;
        t = t - (f32)((s64)ex * z3) / den;
        t = t + (f32)x3 / fdx;
        t = t - (f32)x / fdx;
        t = t / a;
        e = fdx * t + (f32)x;
        e = (e - (f32)x3) / (f32)ex;
    }
    a = 1.0f - t;
    b = (f32)(u2 - u3) * e + (f32)u3;
    u1 = engine_cvt_w_s((b - (f32)u1 * t) / a);
    b = (f32)(v2 - v3) * e + (f32)v3;
    v1 = engine_cvt_w_s((b - (f32)v1 * t) / a);
    *v_out = v1;
    return u1;
}

/* the first triangle of object id's (from D_803EBDB0, which has one) that
   has (x, z) under it: by its world corners, or its own */
static u8 *moving_tri_under(s32 id, s32 x, s32 z, s32 own) {
    u8 *p;
    s32 *w;
    s16 *h;

    for (p = D_803EBDB0;; p += MOVING_TRI_SIZE) {
        if (MOVING_TRI_ID(p) != id)
            continue;
        if (own) {
            h = MOVING_TRI_OWN(p);
            if (tri_under(x, z, h[0], h[2], h[3], h[5], h[6], h[8]))
                return p;
        } else {
            w = (s32 *)p;
            if (tri_under(x, z, w[0], w[2], w[3], w[5], w[6], w[8]))
                return p;
        }
    }
}

/* where (x, z), on object id's world triangles, is on its own ones */
REGS(a3, t0, t1 -> t3, t4)
s32 func_802AAD0C(s32 id, s32 x, s32 z, s32 *v_out) {
    u8 *p;
    s32 *w;
    s16 *h;

    p = moving_tri_under(id, x, z, 0);
    w = (s32 *)p;
    h = MOVING_TRI_OWN(p);
    return func_802AAF64(x, z, w[0], w[2], w[3], w[5], w[6], w[8], h[0], h[2], h[3], h[5], h[6], h[8], v_out);
}

/* the reverse: from its own triangles to the world's */
REGS(a3, t0, t1 -> t3, t4)
s32 func_802AAE54(s32 id, s32 x, s32 z, s32 *v_out) {
    u8 *p;
    s32 *w;
    s16 *h;

    p = moving_tri_under(id, x, z, 1);
    w = (s32 *)p;
    h = MOVING_TRI_OWN(p);
    return func_802AAF64(x, z, h[0], h[2], h[3], h[5], h[6], h[8], w[0], w[2], w[3], w[5], w[6], w[8], v_out);
}

void func_802AACD4(s32 id, s32 x, s32 z, s16 *u, s16 *v) {
    s32 vv;

    *u = func_802AAD0C(id, x, z, &vv);
    *v = vv;
}

void func_802AAE1C(s32 id, s32 x, s32 z, s32 *u, s32 *v) {
    s32 vv;

    *u = func_802AAE54(id, x, z, &vv);
    *v = vv;
}

/* ---- D_803ED3B8's tree: what stands on what ------------------------------- */

/* Whether something stands on vehicle `id`: a record with id under some
   of its wheels (not all three), or under all three of a vehicle that
   something stands on (recursively).  Returns 1 or 0. */
REGS(s2 -> t0)
s32 func_802AB8D8(s32 id) {
    u8 *p;

    for (p = D_803ED3B8; !RECORDS_END(p); p += 4) {
        if (p[1] == id) {
            if (p[2] != id || p[3] != id)
                return 1;
            if (p[0] != 0 && func_802AB8D8(p[0]) == 1)
                return 1;
        } else if (p[2] == id || p[3] == id) {
            return 1;
        }
    }
    return 0;
}

s32 func_802AB878(s32 id) {
    return func_802AB8D8(id);
}

/* ---- a point through a chain of matrices -------------------------------- */

extern u8 D_803EBB98[];                 /* the product's scratch matrix */

/* An Mtx's halves by their words (element 2n is word n's high half; in
   native-endian memory a halfword's address is another, docs/PORT.md) */
static u32 mtx_half(u8 *m, s32 off) {
    u32 w = *(u32 *)(m + (off & ~3));

    return off & 2 ? w & 0xFFFF : w >> 16;
}

static void mtx_set_half(u8 *m, s32 off, u32 v) {
    u32 *w = (u32 *)(m + (off & ~3));

    *w = off & 2 ? (*w & 0xFFFF0000) | (v & 0xFFFF) : (*w & 0xFFFF) | (v << 16);
}

/* a 4x4 16.16 matrix's element at this offset (the integer halves first,
   the fractions 0x20 on) */
static s32 mtx_el(u8 *m, s32 off) {
    return (s32)((mtx_half(m, off) << 16) | mtx_half(m, off + 0x20));
}

/* all 16 elements, row by row */
static void mtx_load(u8 *m, s32 *e) {
    s32 k;

    for (k = 0; k < 16; k++)
        e[k] = mtx_el(m, k * 2);
}

/* (x, y, z) through the n matrices at base + offs[i], multiplied in
   order (D_803EBB58, with D_803EBB98 for each product): the result >> 11,
   y and z in *y_out and *z_out.  The x row's translation is only its
   integer part.  With no matrices it is s0, s1 and s2 >> 11 (whatever the
   caller passes: no model has such a point).  The sums before the shift
   in *s1_out and *s2_out. */
REGS(v0, v1, a0, a1, a2, s4, s0, s1, s2 -> v0, v1, a0, s1, s2)
s32 func_802AA890(s32 x, s32 y, s32 z, s32 n, s32 *offs, u8 *base, s32 s0, s32 s1, s32 s2, s32 *y_out, s32 *z_out,
                  s32 *s1_out, s32 *s2_out) {
    u8 *cur = (u8 *)D_803EBB58;
    u32 *src;
    s32 i, j, k, a[16], b[16];
    u64 sum;

    if (n != 0) {
        src = (u32 *)(base + *offs++);
        for (k = 0; k < 16; k++)
            ((u32 *)cur)[k] = src[k];
        for (n--; n != 0; n--) {
            mtx_load(cur, a);
            mtx_load(base + *offs++, b);
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    sum = 0;
                    for (k = 0; k < 4; k++)
                        sum += (u64)((s64)a[i * 4 + k] * b[k * 4 + j]);
                    sum >>= 16;
                    mtx_set_half(D_803EBB98, 0x20 + (i * 4 + j) * 2, (u32)sum);
                    mtx_set_half(D_803EBB98, (i * 4 + j) * 2, (u32)(sum >> 16));
                }
            }
            for (k = 0; k < 16; k++)
                ((u32 *)cur)[k] = ((u32 *)D_803EBB98)[k];
        }
        mtx_load(cur, a);
        s0 = (u32)a[0] * x + (u32)a[4] * y + (u32)a[8] * z + (mtx_half(cur, 0x18) << 16);
        s1 = (u32)a[1] * x + (u32)a[5] * y + (u32)a[9] * z + (u32)a[13];
        s2 = (u32)a[2] * x + (u32)a[6] * y + (u32)a[10] * z + (u32)a[14];
    }
    *y_out = s1 >> 11;
    *z_out = s2 >> 11;
    *s1_out = s1;
    *s2_out = s2;
    return s0 >> 11;
}

/* An object's triangles (data: their count, the matrices' count n and
   offsets, then 0x14-byte triangles of three s16 points) into the moving
   objects' triangles of this id (new ones past D_803EBBEC's end): each
   point as it is (its own) and through the matrices at base (the world's).
   (s1 and s2: func_802AA890's for a point with no matrices, passed on.) */
REGS(t3, t4, s4, s1, s2)
void func_802AABE4(s32 id, u16 *data, u8 *base, s32 s1, s32 s2) {
    s32 n = data[1], count, x, y = 0, z = 0, k;
    s32 *offs = (s32 *)(data + 2);
    s16 *t = (s16 *)((u8 *)data + 4 + n * 4);
    u8 *end = D_803EBBEC, *r = D_803EBDB0;
    s16 *h;
    s32 *w;

    for (count = data[0]; count != 0; count--, t += 10, r += MOVING_TRI_SIZE) {
        /* this id's next triangle, or a new one at the end */
        for (; r != end; r += MOVING_TRI_SIZE)
            if (MOVING_TRI_ID(r) == id)
                break;
        if (r == end)
            end += MOVING_TRI_SIZE;
        MOVING_TRI_ID(r) = id;
        h = MOVING_TRI_OWN(r);
        w = (s32 *)r;
        for (k = 0; k < 9; k += 3) {
            h[k] = t[k], h[k + 1] = t[k + 1], h[k + 2] = t[k + 2];
            x = func_802AA890(t[k], t[k + 1], t[k + 2], n, offs, base, (u32)r, s1, s2, &y, &z, &s1, &s2);
            w[k] = x, w[k + 1] = y, w[k + 2] = z;
        }
    }
    D_803EBBEC = end;
}

/* ---- the speed on a slope ------------------------------------------------ */

/* The speed *speed: held to 1.5 times the vehicle's top speeds when
   `limit`; then, with all three wheels on the ground, pushed down the
   slope (func_802A8590's height difference over `div`, times SLOPE_PUSH)
   and brought toward 0 by ROLL_FRICTION_HEAVY (the driver, Thunderfist,
   the J-Bomb and the Cyclone Suit: `type`) or ROLL_FRICTION_LIGHT.  Not at
   all braking (Z) while stopped.  Returns the speed, or 1 with a wheel in
   the air (0 braking while stopped). */
REGS(t6, t8, t7, s0, s7, f2, gp -> t1)
s32 func_802A843C(s16 *speed, s32 limit, s32 type, s8 *wheels, s32 *h, f32 div, VS *vs) {
    s32 v, lim, friction;

    if (PAD_Z != 0 && *speed == 0)
        return 0;
    if (limit != 0) {
        if (*speed >= 0) {
            lim = VS_GEARS(vs)[3 * (GEAR_ROWS - 1) + GEAR_HI];
            lim += lim >> 1;
            if (lim < *speed)
                *speed = lim;
        } else {
            lim = VS_GEARS(vs)[GEAR_LO];
            lim += lim >> 1;
            if (*speed < lim)
                *speed = lim;
        }
    }
    if (wheels[0] == 1 || wheels[1] == 1 || wheels[2] == 1)
        return 1;
    v = *speed + engine_cvt_w_s((f32)func_802A8590(h) / div * SLOPE_PUSH);
    friction = type == VEHICLE_DRIVER || type == VEHICLE_MAGOO || type == VEHICLE_MINIMAGOO ||
                       type == VEHICLE_JETPACK
                   ? ROLL_FRICTION_HEAVY
                   : ROLL_FRICTION_LIGHT;
    if (v < 0) {
        v += friction;
        if (v > 0)
            v = 0;
    } else if (v > 0) {
        v -= friction;
        if (v < 0)
            v = 0;
    }
    *speed = v;
    return v;
}

/* ---- the vehicle's pitch and roll from its wheels' heights --------------- */

extern s16 D_803ED402, D_803ED404;     /* the wheels' spans: along, across */

/* the angle of a height difference d over a span (func_802ACF64 of
   d / span in 16.16), / 16, negative when d is (or with `neg` when it
   isn't) */
static s32 slope_angle(s32 d, s32 span, s32 neg) {
    s32 q, a;

    if (span == 0)
        engine_break(N64_PC(0x802A8B74), 7);
    if (d >= 0) {
        q = (s32)((u32)d << 16) / span;
        a = (u32)func_802ACF64(q) >> 4;
        return neg ? -a : a;
    }
    d = -d;
    q = (s32)((u32)d << 16) / span;
    a = (u32)func_802ACF64(q) >> 4;
    return neg ? a : -a;
}

/* The angles of the wheels' grounds (D_803ED3A8: wheel 0's, then the two
   others), each difference over its span: across (the second's, negative
   when it is higher) in *across_out, and along (the third's, negative
   when it is lower). */
REGS(-> a1, a3)
s32 func_802A8B10(s32 *across_out) {
    s32 h0 = D_803ED3A8[0];

    *across_out = slope_angle(D_803ED3A8[1] - h0, D_803ED404, 1);
    return slope_angle(D_803ED3A8[2] - h0, D_803ED402, 0);
}

/* ---- the angle to turn a vehicle on a moving object to ------------------- */

extern s16 D_803ED3E8;                  /* an angle func_802A94A4 turns by */

/* wheel 0's offset (pts) at heading h, through D_803ED3E8 */
static s32 wheel0_at(s16 *pts, s32 h, s32 *dz) {
    D_803ED3E8 = h;
    return func_802A94A4(0, pts, &D_803ED3E8, dz);
}

/* Of the heading *angle turned either way by the corner angle between the
   two points (x, z) and (x2, z2) in object id's own frame (func_802AAE54
   and func_802ABB1C, in quarter turns), the one that puts wheel 0 (pts)
   nearer the second point. */
REGS(a3, t0, t1, s1, s2, v1, a2 -> t0)
s32 func_802AB9A4(s32 id, s32 x, s32 z, s32 x2, s32 z2, s16 *pts, u16 *angle) {
    s32 ox, oz, px, pz, dx, dz, a = *angle, turn, plus, minus, d_plus, d_minus;

    ox = func_802AAE54(id, x, z, &oz);
    px = func_802AAE54(id, x2, z2, &pz);
    dx = func_802A94A4(0, pts, (s16 *)angle, &dz);
    turn = func_802ABB1C(px, pz, dx, dz, ox, oz);
    if (turn > ANGLE_QUARTER) {
        dx = wheel0_at(pts, wrap_fff(a + ANGLE_QUARTER), &dz);
        turn = func_802ABB1C(px, pz, dx, dz, ox, oz) + ANGLE_QUARTER;
        if (turn > ANGLE_HALF) {
            dx = wheel0_at(pts, wrap_fff(a + ANGLE_HALF), &dz);
            turn = func_802ABB1C(px, pz, dx, dz, ox, oz) + ANGLE_HALF;
        }
    }
    plus = a + turn;
    if (plus >= ANGLE_TURN)
        plus -= ANGLE_WRAP;
    dx = wheel0_at(pts, plus, &dz);
    d_plus = iabs(dx + ox - px) + iabs(dz + oz - pz);
    minus = a - turn;
    if (minus < 0)
        minus += ANGLE_WRAP;
    dx = wheel0_at(pts, minus, &dz);
    d_minus = iabs(dx + ox - px) + iabs(dz + oz - pz);
    return d_plus < d_minus ? plus : minus;
}

/* ---- a wheel landing ----------------------------------------------------- */

#include "game/audio.h"

extern u8 D_803ED40B;                   /* the vehicle's wheels make a sound landing */

/* how far a wheel has fallen after `t` frames from rising at `rise`:
   rise * t + gravity * t * t */
static s32 fall_drop(s32 rise, s32 t) {
    return (s32)((u32)rise * (u32)t + engine_cvt_w_s(GRAVITY * (f32)(s32)((u32)t * (u32)t)));
}

/* Wheel i landing on the ground g: `y` its drop this frame less the one
   before, its speed down.  Harder than LAND_BOUNCE_MIN it bounces off
   again (func_802A9540) at that over LAND_BOUNCE_DIV (with sound 12 when
   D_803ED40B); otherwise it stays down (WHEEL_AIRBORNE(i) 0).  Returns the
   speed (bounced). */
REGS(v0, a1, a2, a3, t3, s6 -> s6)
s32 func_802A9710(s32 i, s32 *h, s32 *state, s32 *ground, s32 g, s32 y) {
    y = iabs(y - fall_drop(h[i], state[i] - 2));
    if (LAND_BOUNCE_MIN < y) {
        if (D_803ED40B != 0)
            func_80260650(D_80367738, 0xC, NULL);
        if (LAND_BOUNCE_DIV == 0)
            engine_break(N64_PC(0x802A98F0), 7);
        y = (u32)y / LAND_BOUNCE_DIV;
        func_802A9540(i, h, state, ground, g, y);
    } else {
        WHEEL_AIRBORNE(i) = 0;
    }
    return y;
}

/* ---- one wheel's step in the air --------------------------------------------- */

extern u8 D_803F7C49;                   /* the J-Bomb flies: it passes ground far above */
#define JETPACK_PASS_ABOVE 0x6A5

/* Wheel i one frame on in the air (its frames count up): where it would
   be (the ground it left plus its fall so far) against the ground under it
   (func_802A9B1C).  Above that it stays in the air (WHEEL_HEIGHT(i), at
   least 0); otherwise it lands (func_802A9710).  The flying J-Bomb also
   stays in the air under ground JETPACK_PASS_ABOVE or more above.
   Returns the drop, or func_802A9710's speed. */
REGS(v0, a1, a2, a3, t0, t1, s7, v1, s4, t8, gp, fp -> s6)
s32 func_802A95A4(s32 i, s32 *h, s32 *state, s32 *ground, s32 x, s32 z, s32 *heights, s16 *pts, s16 *a,
                  s32 self, VS *vs, s32 mat) {
    s32 t = state[i], drop, y, dx, dz, g;

    state[i] = t + 1;
    drop = fall_drop(h[i], t);
    y = ground[i] + drop;
    dx = func_802A94A4(i, pts, a, &dz);
    g = func_802A9B1C(i, x + dx, z + dz, heights[3 * i], self, vs, mat);
    if (g >= y && !(self == VEHICLE_JETPACK && D_803F7C49 != 0 && (s32)((u32)g - y) >= JETPACK_PASS_ABOVE)) {
        WHEEL_HEIGHT(i) = g;
        return func_802A9710(i, h, state, ground, g, drop);
    }
    WHEEL_HEIGHT(i) = y < 0 ? 0 : y;
    WHEEL_AIRBORNE(i) = 1;
    return drop;
}

/* ---- turning toward an angle --------------------------------------------- */

extern u8 D_803ED3F8;                   /* frames since the turning sound */
extern f32 D_803ED3FC, D_8030D890;      /* its pitch, and the speed's part in it */

/* a 12-bit angle a step toward a target, the short way round, not past
   it */
static s32 turn_toward(s32 a, s32 target, s32 step) {
    if (iabs(a - target) > ANGLE_HALF) {
        /* the short way is through 0 */
        if (target < a) {
            a += step;
            if (a < ANGLE_TURN)
                return a;
            a -= ANGLE_TURN;
            return target < a ? target : a;
        }
        a -= step;
        if (a >= 0)
            return a;
        a += ANGLE_TURN;
        return a < target ? target : a;
    }
    if (a < target) {
        a += step;
        return a >= ANGLE_TURN || target < a ? target : a;
    }
    a -= step;
    return a < 0 || a < target ? target : a;
}

/* The move heading a frame: *angle (VS_TURN_HEADING) a step toward
   *target (VS_HEADING): the turn rate (func_802A8314's), times the grip
   over the speed *speed when it isn't 0.  Unless *turning (VS_TURNING),
   the target itself when within TURN_SNAP of it; otherwise *turning is
   set, and cleared once it gets there.  With `sound`, the turning sound
   (10) every TURN_SOUND_FRAMES frames at a pitch from the speed.  *out
   (VS_MOVE_HEADING) gets the angle. */
REGS(t5, t6, t1, s4, s5, s6, s7, gp)
void func_802A7FD8(s32 rate, s16 *speed, u16 *angle, u16 *target, u16 *out, s8 *turning, s32 sound, VS *vs) {
    s32 v = *speed, a, t = *target, step, d, n;

    step = func_802A8314(rate);
    if (v != 0)
        step = (s32)((u32)step * VS_GRIP(vs)) / v;
    a = turn_toward(*angle, t, iabs(step));
    *angle = a;
    if (*turning == 0) {
        d = iabs(a - t);
        if (d > ANGLE_HALF)
            d = ANGLE_TURN - d;
        if (d < TURN_SNAP) {
            *out = t;
            return;
        }
        *turning = 1;
    }
    if (sound != 0) {
        n = D_803ED3F8 + 1;
        if (n >= TURN_SOUND_FRAMES) {
            n = 0;
            D_803ED3FC = 0.5f + (f32)iabs(v) * D_8030D890;
            func_80260AB8(func_80260650(D_80367738, 0xA, NULL), 0x10, *(s32 *)&D_803ED3FC);
        }
        D_803ED3F8 = n;
    }
    *out = a;
    if (a == t)
        *turning = 0;
}

/* whether h lies inside the camera's headings' range lo..hi (12-bit,
   wrapping when hi < lo) narrowed by CAMERA_MARGIN at each end; a
   narrowed range that's empty holds nothing */
static s32 in_camera_range(s32 h, s32 lo, s32 hi) {
    s32 a = lo + CAMERA_MARGIN, b = hi - CAMERA_MARGIN;

    if (hi < lo) {
        if (a >= ANGLE_TURN)
            a -= ANGLE_TURN;
        if (b < 0)
            b += ANGLE_TURN;
        if (!(b < a))
            return 0;
        return h >= a || h <= b;
    }
    return a < b && h >= a && h <= b;
}

/* The camera's turn: toward h2 when that is inside the headings' range
   D_803A7410..D_803A7412 (less CAMERA_MARGIN at each end), with no turn;
   else toward h if that is, or the nearer end of the range (the middle
   when that end is outside it), at the speed times `rate`, signed the
   short way to h2.  Returns the heading, the turn in *rate_out. */
REGS(a0, a1, f0, gp -> a0, a1)
s32 func_802A71DC(s32 h, s32 h2, f32 rate, VS *vs, s32 *rate_out) {
    s32 lo = (u16)D_803A7410, hi = (u16)D_803A7412, dl, dh, s, neg;

    if (in_camera_range(h2, lo, hi)) {
        *rate_out = 0;
        return h2;
    }
    if (!in_camera_range(h, lo, hi)) {
        /* to the nearer end of the range */
        dl = iabs(lo - h);
        if (dl > ANGLE_HALF)
            dl = ANGLE_WRAP - dl;
        dh = iabs(hi - h);
        if (dh > ANGLE_HALF)
            dh = ANGLE_WRAP - dh;
        if (dh < dl) {
            h = hi - CAMERA_MARGIN;
            if (h < 0)
                h += ANGLE_WRAP;
        } else {
            h = lo + CAMERA_MARGIN;
            if (h >= ANGLE_TURN)
                h -= ANGLE_WRAP;
        }
        /* and when that end is outside the range itself, the middle */
        if (hi < lo ? h < lo && h > hi : h < lo || h > hi)
            h = func_802A6F6C();
    }
    s = engine_cvt_w_s(rate * (f32)iabs(VS_SPEED(vs)));
    if (h < h2)
        neg = iabs(h - h2) <= ANGLE_HALF;
    else
        neg = h - h2 > ANGLE_HALF;
    *rate_out = neg ? -s : s;
    return h;
}

/* ---- out of the level's bounds ------------------------------------------- */

extern s16 D_803BE730, D_803BE732, D_803BE734, D_803BE736; /* the level's bounds (game/level.h) */
/* D_803BE6F8: the start points, 9-byte records, a key, then (x, y, z) as s16 pairs of bytes */
extern u8 D_80364412;

/* a start point's coordinate: a big-endian s16 at p, << 5 */
#define START_COORD(p) ((s32)(((u32)((s8)(p)[0] << 8) | (p)[1]) << 5))

/* With (x, z) outside the level's bounds (D_803BE730-D_803BE736, in 32s;
   and in us.v11 and jp, the train short of a line on levels 0, 0x12 and
   0xD): the vehicle back at its start point (D_803BE6F8's record of key
   1, or of its type when D_80364AA8 is 1 or 0x80) in *xo, *yo, *zo, its
   state at rest (func_802A754C, its wheels' heights the point's), the
   matrices (func_802AC6FC) and a sound (func_80277EDC).  Returns whether
   it put it back. */
REGS(t0, t1, t7, s2, s1, t8, gp -> v0)
s32 func_802A8CCC(s32 x, s32 z, s32 *xo, s32 *yo, s32 *zo, s32 type, VS *vs) {
    s32 cx = x >> 5, cz = z >> 5, key, rx, ry, rz, i, out;
    u8 *r;

    out = cx < D_803BE730 || D_803BE732 < cx || cz < D_803BE734 || D_803BE736 < cz;
#ifndef VERSION_US_V10
    if (type == VEHICLE_TRAIN &&
        ((D_802E8BDC == 0 && cx < 0x3E8) || (D_802E8BDC == 0x12 && cz < 0x384) || (D_802E8BDC == 0xD && cz < 0x44C)))
        out = 1;
#endif
    if (!out)
        return 0;
    key = D_80364AA8 == 1 || D_80364AA8 == 0x80 ? type : 1;
    for (r = D_803BE6F8; r[0] != key; r += 9)
        ;
    D_80364412 = 1;
    *xo = rx = START_COORD(r + 1);
    *yo = ry = START_COORD(r + 3);
    *zo = rz = START_COORD(r + 5);
    func_802A754C(vs);
    for (i = 0; i < 9; i++)
        VS_WHEEL_H(vs)[i] = ry;
    func_802AC6FC(rx, ry, rz, 0x13, 1000000);
    func_80277EDC(1, 1, 4, 0x6C);
    return 1;
}

/* The points of a part list (p to end: (x, y, z) s16s, the matrices'
   count and offsets) through their matrices at base (func_802AA890), into
   D_803EBC10's records of this id from the first of it (or its end) on.
   (A point with no matrices would be whatever the original's s0, s1 and
   s2 held: no model has one.) */
static void abbec(s32 id, u8 *p, u8 *end, u8 *base) {
    u8 *r;
    s16 *h;
    s32 x, y, z, n, s1, s2;

    for (r = D_803EBC10; !RECORDS_END(r) && r[0xC] != id; r += 0x10)
        ;
    for (; p != end; p += n * 4 + 8, r += 0x10) {
        h = (s16 *)p;
        n = (u16)h[3];
        x = func_802AA890(h[0], h[1], h[2], n, (s32 *)(p + 8), base, 0, 0, 0, &y, &z, &s1, &s2);
        ((s32 *)r)[0] = x;
        ((s32 *)r)[1] = y;
        ((s32 *)r)[2] = z;
        r[0xC] = id;
    }
}

/* ---- a wheeled vehicle one step on the ground ---------------------------- */

extern s16 D_803ED406;                  /* the heading, for the shadow */
extern u8 D_803ED40E, D_803ED40F;

/* The wheeled vehicle (type, at (x, z), vs) one frame on: back in the
   level's bounds (func_802A8CCC); its spans (along, across) and heading
   kept; then each of its three wheels on the ground (func_802A93B0) or in
   the air (func_802A95A4) as VS_AIRBORNE says (a one-point vehicle: wheel
   0, copied to the others).  The wheels' heights go into VS_WHEEL_H and
   their airborne flags into VS_AIRBORNE; *px and *pz are (x, z), *py the
   average of the back two wheels; D_803ED390's pitch and roll from the
   height differences over the spans; the shadow (func_802582C4, tilted by
   the grounds' angles); the grip (func_802A9164); and its record of what
   it stands on (func_802A92C8).  `mat`... the caller's: func_802A9B1C's
   material where none is found (the original's $fp, the across span).
   The arguments v1 (VS_WHEELS), a1..a3 (VS_WHEEL_FALL's three parts) and
   t3 (VS_CARRY_WHEELS) are vs's in every caller. */
REGS(t0, t1, t7, s1, s2, t8, t9, fp, v1, a1, a2, a3, t3, gp)
void func_802A8768(s32 x, s32 z, s32 *px, s32 *pz, s32 *py, s32 type, s32 t9, s32 fp, s16 *v1, s32 *a1, s32 *a2,
                   s32 *a3, s16 *t3, VS *vs) {
    u8 *air = VS_AIRBORNE(vs);
    s16 *heading = (s16 *)&VS_HEADING(vs);
    s32 *hist = VS_WHEEL_H(vs);
    s32 along = t9, across = fp, i, w, hw[3], roll, shadow_along, shadow_across;

    if (func_802A8CCC(x, z, px, py, pz, type, vs)) {
        x = *px;
        z = *pz;
    }
    D_803ED402 = along;
    D_803ED404 = across;
    D_803ED406 = *heading;
    D_803ED40E = air[0];
    VS_ON_STATIC(vs) = 0;
    func_802A90E4((u16 *)v1);
    func_802A8FF4(vs);
    func_802A8FB4();
    for (i = 0; i < 3; i++) {
        if (D_803ED410 != 0 && i != 0) {
            func_802A9038(vs);
            break;
        }
        if ((s8)air[i] == 1)
            func_802A95A4(i, a1, a2, a3, x, z, hist, v1, heading, type, vs, across);
        else if ((s8)air[i] == 0)
            func_802A93B0(i, v1, a1, a2, a3, x, z, heading, hist, type, vs, across);
        else
            engine_syscall(N64_PC(0x802A87FC));
    }
    for (w = 0; w < 3; w++) {
        air[w] = WHEEL_AIRBORNE(w);
        hw[w] = WHEEL_HEIGHT(w);
        hist[3 * w + 2] = hist[3 * w + 1];
        hist[3 * w + 1] = hist[3 * w];
        hist[3 * w] = hw[w];
    }
    *px = x;
    *pz = z;
    *py = ((u32)hw[1] + (u32)hw[2]) >> 1;
    /* the pitch from the second wheel over the across span, the roll from
       the third over the along span */
    D_803ED390[2] = slope_angle(hw[1] - hw[0], across, 1);
    D_803ED390[0] = roll = slope_angle(hw[2] - hw[0], along, 0);
    /* the shadow, on the ground's slope */
    shadow_along = func_802A8B10(&shadow_across);
    func_802582C4(type, *px, ((u32)D_803ED3A8[1] + (u32)D_803ED3A8[2]) >> 1, *pz, ((u32)hw[1] + (u32)hw[2]) >> 1,
                  shadow_along, shadow_across, D_803ED406);
    func_802A9164(air, type, vs);
    if (D_803ED3F5 == 0 && type != VEHICLE_CMO && D_803ED40F != 0)
        func_802A92C8(*px, *pz, VS_CARRY_WHEELS(vs), (s16 *)&VS_HEADING(vs), hist, type, vs, roll);
}

/* func_802ABBEC as the vehicle modules declare it (shared.h) */
REGS(t0, t1, t2, s4)
void func_802ABBEC(s32 id, u8 *verts, u8 *end, u8 *buf) {
    abbec(id, verts, end, buf);
}
