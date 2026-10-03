/*
 * hd_code 75490 (us.v11 0x802B9C50-0x802BBA60): the missile carrier (type
 * 0xFF) and the crane (type 6), as native C (engine.h).
 *
 * The carrier's parts are D_803EF330, its state D_803EF630 and its position
 * D_803EF6DC..E4.  func_802B9C50 sets it up (from the level loader),
 * func_802BA354 runs it each frame (from hd.c and at the end of the setup):
 * it rolls along z at up to D_803EF6FC (the level's records in D_80305D62
 * raise it as it goes), and the level is won when it is D_803EF6F0 from
 * where it started (D_803643DA), lost when it hits something or a wheel is
 * off the ground (D_803643D9).  func_802BA148 is its engine's sound.
 *
 * The crane's parts are D_803EF720, its state D_803EFA20 and its position
 * D_803EFAC8..D0: func_802BAD80 sets it up, func_802BB274 runs it, and
 * func_802BB054, func_802BB170 and func_802BB1A0 are hd.c's hooks for
 * getting in, whether it can be left, and getting out.
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* the carrier's .bss (asm/data/hd_code/75490.bss.s) */
extern Part D_803EF330[32];
extern VS D_803EF630;
extern s16 D_803EF6D6;                          /* the speed its sound was last set for */
extern SndState *PTR32 D_803EF6D8;              /* its engine's sound, while the player is near */
extern s32 D_803EF6DC, D_803EF6E0, D_803EF6E4;  /* x, y, z */
extern s32 D_803EF6E8, D_803EF6EC;              /* how far it goes before D_803EF710, D_803EF711 */
extern s32 D_803EF6F0;                          /* how far it goes to win the level */
extern s32 D_803EF6F4, D_803EF6F8;              /* where it started: x, z */
extern s16 D_803EF6FC;                          /* its top speed */
extern u8 D_803EF6FE;                           /* frames before the countdown's effect */
extern u8 D_803EF6FF;                           /* the effect's frames are over */
extern u8 D_803EF700;                           /* the effect's frames */
extern u8 D_803EF701;                           /* the effect has started */
extern u8 *PTR32 D_803EF704;                    /* two 0xC00-byte buffers, one per frame */
extern u8 *PTR32 D_803EF708;
extern u8 *PTR32 D_803EF70C;                    /* its model file */
extern u8 D_803EF710, D_803EF711;
/* the crane's */
extern Part D_803EF720[32];
extern VS D_803EFA20;
extern s32 D_803EFAC8, D_803EFACC, D_803EFAD0;  /* x, y, z */
extern u8 *PTR32 D_803EFAD4;                    /* its model file */
extern SndState *PTR32 D_803EFAD8;              /* its three sounds */
extern SndState *PTR32 D_803EFADC;
extern SndState *PTR32 D_803EFAE0;
extern u8 *PTR32 D_803EFAE4;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EFAE8;

extern u8 D_80305D60[];                         /* the carrier's (no) bounce records */
extern u8 D_80305D62[];                         /* {s16 z >> 5, u8 level, u8 top speed}, to z < 0 */
extern s16 D_80305D74[];                        /* {level, distance >> 5 to D_803EF710, to D_803EF711} */
extern u8 D_80305DF0[];                         /* the crane's */
extern u8 D_802C236C[];                         /* its lights (56040's list) */
extern u8 D_802C3B44[];                         /* the countdown's effect record (60F60) */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7426;
extern u8 D_80370C15, D_80370C16, D_80370C1A, D_80370C1B, D_80370C1C, D_80370C1D;
extern u8 D_803643D6, D_803643D8;
extern Part *PTR32 D_803F77D0;
extern s32 D_803643E4, D_803643E8;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80260AB8(SndState *state, s16 type, s32 param);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_80278318(void);

/* 60F60.c's */
REGS()
void func_802A5E60(void);
REGS()
void func_802C049C(void);
REGS()
void func_8029C5EC(void);
REGS(-> s4)
s32 func_8029C6E4(void);
REGS(t0, t1, t3, s4, s7, t8, gp, fp)
void func_802A92C8(s32 x, s32 z, s16 *pts, s16 *a, s32 *heights, s32 self, VS *vs, s32 mat);
REGS(t0, t1, t2 -> a1)
s32 func_802AC0BC(s32 x, s32 z, s32 y);

void func_802BA148(void);
void func_802BA354(void);
void func_802BB054(void);
u8 func_802BB170(void);
void func_802BB1A0(void);
void func_802BB274(void);
void func_802BA074(void);
void func_802BA104(void);
void func_802BA5A4(void);
void func_802BA638(void);
REGS(gp)
void func_802BA6AC(VS *vs);
void func_802BA91C(void);
REGS(gp, s7)
void func_802BA9A0(VS *vs, s32 *s7);
REGS(gp)
void func_802BABEC(VS *vs);
void func_802BAD24(void);
void func_802BB230(void);
REGS(gp)
void func_802BB4C0(VS *vs);
REGS(-> v0)
s32 func_802BB868(void);
REGS(gp)
void func_802BB8B8(VS *vs);

#define T(p) ((s32)(p))
#define P D_803EF330
#define Q D_803EF720

static s32 f2i(f32 f) {
    union {
        f32 f;
        s32 i;
    } u;

    u.f = f;
    return u.i;
}

/* how far the carrier is from where it started (x and z) */
static s64 dist(void) {
    return func_802ABCDC(D_803EF6DC, 0, D_803EF6E4, D_803EF6F4, 0, D_803EF6F8);
}

/* part i's frame (func_802A04BC's v1), and its t1 (unk13) and f0 */
static s32 part(s32 i, Part *parts, s32 *t1, f32 *f0) {
    s32 f11, f12, f14, fC, fE, t1_;
    f32 f4;
    s32 r = func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &t1_, &f4);

    if (t1)
        *t1 = t1_;
    if (f0)
        *f0 = f4;
    return r;
}

/* jp's carrier takes its top speed from D_8030606E_jp's (level, speed)
   pairs where its level has one (func_802BA3E8_jp), and so its blocks from
   there on are 4 bytes on */
#ifdef VERSION_JP
extern u16 D_8030606E_jp[];     /* (level, speed) byte pairs, -1 after the last */
/* (the asm data declares them as halves: in native-endian memory a byte is
   at its address ^ 1) */
#ifdef PORT_NATIVE_ENDIAN
#define NE_X1(off) ((off) ^ 1)
#else
#define NE_X1(off) (off)
#endif

/* the level's top speed for the carrier, where D_8030606E_jp has one, else
   speed (it keeps $v0, $v1 and $a0) */
REGS(s1 -> s1)
s32 func_802BA3E8_jp(s32 speed) {
    u8 *pairs = (u8 *)D_8030606E_jp;
    s32 i = 0;
    s8 level;

    ENGINE_BLK(802BA3E8_jp);
    for (;;) {
        ENGINE_BLK(802BA40C_jp);
        level = (s8)pairs[NE_X1(i)];
        if (level < 0)
            break;
        ENGINE_BLK(802BA418_jp);
        i += 2;
        if (D_802E8BDC == level) {
            ENGINE_BLK(802BA420_jp);
            speed = pairs[NE_X1(i - 1)];
            break;
        }
    }
    ENGINE_BLK(802BA424_jp);
    return speed;
}
#define B9C50_BLK(us, jp) ENGINE_BLK(jp)
#else
#define B9C50_BLK(us, jp) ENGINE_BLK(us)
#endif

/* the carrier's set up: from the level loader, with the model file in $s2,
   x and z in $t4 and $t5, the heading in $t6, the distance to win in $t7
   and the top speed in $s1 */
REGS(s2, t4, t5, t6, t7, s1)
void func_802B9C50(u8 *model, s32 x, s32 z, s32 heading, s32 dist, s32 speed) {
    VS *vs = &D_803EF630;
    u8 *buf;
    s32 avg;

    ENGINE_COST(802B9C50, 265);
    D_803EF70C = model;
    buf = D_80358070;
    D_803EF704 = buf;
    D_803EF708 = buf + 0xC00;
    D_80358070 = buf + 0x1800;
    func_802A1388(0xFF, 0, D_803EF704, D_803EF708, model);
    D_803EF6DC = x;
    D_803EF6F4 = x;
    D_803EF6E4 = z;
    D_803EF6F8 = z;
#ifdef VERSION_JP
    speed = func_802BA3E8_jp(speed);
#endif
    D_803EF6FC = speed;
    D_803EF6D6 = 0;
    D_803EF6F0 = dist;
    func_802A754C(vs);
    vs->unk4E = heading;
    vs->unk4C = heading;
    D_803EF6FE = 5;
    D_803EF700 = 0;
    D_803EF701 = 0;
    func_802BA074();
    vs->unk52[0] = 0x190;
    vs->unk52[1] = 0x4B0;
    vs->unk52[2] = -0x190;
    vs->unk52[3] = 0x4B0;
    vs->unk52[4] = 0x190;
    vs->unk52[5] = -0x4B0;
    vs->unk5E[0] = 0x1A4;
    vs->unk5E[1] = 0x4D8;
    vs->unk5E[2] = -0x1A4;
    vs->unk5E[3] = 0x4D8;
    vs->unk5E[4] = 0x1A4;
    vs->unk5E[5] = -0x4D8;
    func_802A992C(vs->unk52, 0x7FFF, D_803EF6DC, D_803EF6E4, vs->unk4, &D_803EF6E0, (s16 *)&vs->unk4C, 0xFF, vs, 0,
                  &avg);
    func_8029F85C(P, D_803EF70C, D_803EF704, D_803EF708);
    func_802A039C(0, 100, P);
    func_802A03D4(0, 0, P);
    func_802A040C(0, 0, P);
    func_802A0480(0, 0, P, 0.0f);
    func_802A0290(0, 1, P);
    func_8029E558(P, D_803EF704, D_803EF708);
    func_802A0320(0, P);
    func_802A0290(0, 1, P);
    func_8029E558(P, D_803EF708, D_803EF704);
    model = D_803EF70C;
    func_8029C354(0xFF, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x59D8);
    func_80258230(0xFF, 0x96, 0x3C, 0x3C);
    func_802BABEC(vs);
    vs->unk9A = 1;
    func_802BA354();
    vs->unk9A = 0;
    model = D_803EF70C;
    func_802AA838(D_803EF708, D_803EF704, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    func_802A039C(1, 0, P);
    func_802A03D4(1, 0, P);
    func_802A040C(1, 0, P);
    func_802A0290(1, -1, P);
    func_802A039C(2, 0, P);
    func_802A03D4(2, 0, P);
    func_802A040C(2, 1, P);
    func_802A0290(2, -1, P);
    func_802A05D0(D_802C236C, 100);
    func_802A05F8(D_802C236C, 0);
    func_802A0620(D_802C236C, 1);
    func_802A0508(D_802C236C, -1);
}

/* how far it goes before D_803EF710 and D_803EF711: the level's record in
   D_80305D74, or 0 */
void func_802BA074(void) {
    s16 *p;
    s32 l, v;

    ENGINE_COST(802BA074, 83);
    D_803EF6E8 = 0;
    D_803EF710 = 0;
    D_803EF6EC = 0;
    D_803EF711 = 0;
    l = D_802E8BDC;
    p = D_80305D74;
    for (;;) {
        v = p[0];
        if (v < 0)
            goto done;
        p += 3;
        if (l == v)
            break;
    }
    D_803EF6E8 = (u16)p[-2] << 5;
    D_803EF6EC = (u16)p[-1] << 5;
done:
    ;
}

/* its light */
void func_802BA104(void) {
    ENGINE_COST(802BA104, 17);
    func_802ABD54(0xFF, D_803EF6DC, D_803EF6E0, D_803EF6E4);
}

/* its engine's sound: on within 0x3E80 of the player, louder nearer, panned
   by x */
void func_802BA148(void) {
    s32 d, dx, v;

    ENGINE_COST(802BA148, 101);
    dx = D_803643E0 - D_803EF6DC;
    d = (s32)func_802ABCDC(D_803643E0, D_803643E4, D_803643E8, D_803EF6DC, D_803EF6E0, D_803EF6E4);
    if (!(d < 0x3E81)) {
        if (D_803EF6D8 == NULL)
            goto done;
        func_802608C8(D_803EF6D8);
        D_803EF6D8 = NULL;
        goto done;
    }
    if (D_803EF6D8 == NULL) {
        func_80260650(D_80367738, 0x75, &D_803EF6D8);
    }
    d -= 0xFA0;
    if (d < 0) {
        d = 0;
    }
    func_80260AB8(D_803EF6D8, 8, 0x7FFF - (d << 1));
    v = 0x40 + (dx >> 5);
    if (v < 0) {
        v = 0;
    } else {
        if (!(v < 0x80)) {
            v = 0x7F;
        }
    }
    func_80260AB8(D_803EF6D8, 4, v);
done:
    ;
}

/* each frame */
void func_802BA354(void) {
    VS *vs = &D_803EF630;
    s32 t3 = 0, x, z, s;
    f32 rate;

    ENGINE_COST(802BA354, 135);
    /* (its $s4 and $fp as it found them: 5CB60.c and the other vehicles read them from the context) */
    engine_save(ENGINE_GPR(20) | ENGINE_GPR(30), 0);
    func_802BA104();
    func_802BA5A4();
    if (vs->unk9A == 0) {
        func_802BA6AC(vs);
        if ((s8)D_803643D6 != 0)
            goto placed;
        if ((s8)D_802E8BD0 != 0)
            goto placed;
    }
    func_802BA638();
    /* speed up by 8 a frame to the top speed */
    s = vs->unk76;
    if (s < D_803EF6FC) {
        s += 8;
        if (D_803EF6FC < s) {
            s = D_803EF6FC;
        }
    }
    vs->unk76 = s;
    func_802BAD24();
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 0, 0xFF, (s8 *)vs->unk96, vs->unk4, 2400.0f, vs);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EF6DC, &D_803EF6E4, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EF6DC, &D_803EF6E4, &D_803EF6E0, 0xFF, 0x960, 0x320, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
placed:
    if (D_8035805C != 0) {
        func_8029E558(P, D_803EF704, D_803EF708);
    } else {
        func_8029E558(P, D_803EF708, D_803EF704);
    }
    func_802BABEC(vs);
    if (vs->unk9A == 0) {
        func_802BA9A0(vs, vs->unk4);
    }
    func_802BA91C();
    func_802A133C(D_803EF6DC, D_803EF6E0, D_803EF6E4, 0xFF, vs);
    engine_restore();
}

/* whether it is past D_803EF6E8 and D_803EF6EC from where it started */
void func_802BA5A4(void) {
    s64 d;

    ENGINE_COST(802BA5A4, 33);
    d = dist();
    if (!(d < D_803EF6E8)) {
        D_803EF710 = 1;
    }
    if (!(d < D_803EF6EC)) {
        D_803EF711 = 1;
    }
}

/* its top speed: the last of the level's records in D_80305D62 that it is
   past, if any */
void func_802BA638(void) {
    u8 *p = D_80305D62;
    s32 z = D_803EF6E4 >> 5, l = D_802E8BDC, v = 0, rz;

    ENGINE_COST(802BA638, 43);
    for (;;) {
        rz = *(s16 *)p;
        if (rz < 0)
            break;
        if (p[2] != l) {
            p += 4;
            continue;
        }
        if (z < rz) {
            p += 4;
            continue;
        }
        v = p[3];
        p += 4;
    }
    if (v != 0) {
        D_803EF6FC = v;
    }
}

/* its sound's pitch by its speed, its wheels' turn (part 2, by the camera's
   heading) and its drive shaft's (part 1, by its speed) */
REGS(gp)
void func_802BA6AC(VS *vs) {
    s32 s, a;
    u32 h, q;
    f32 f;

    ENGINE_COST(802BA6AC, 122);
    s = vs->unk76;
    a = D_803EF6D6;
    D_803EF6D6 = s;
    if (a == s)
        goto turn;
    if (D_803EF6D8 == NULL)
        goto turn;
    func_80260AB8(D_803EF6D8, 0x10, f2i(1.5f + (f32)s * -0.004f));
turn:
    h = (u16)D_80364452 + 0x800;
    if (!((s32)h < 0x1000)) {
        h -= 0xFFF;
    }
    q = h / 0x555;
    f = (f32)(s32)(h % 0x555) / 1365.0f;
    if (q == 0) {
        func_802A0360(2, 0, P, f);
    } else {
        if (q == 1) {
            func_802A0360(2, 1, P, f);
        } else {
            func_802A0360(2, 2, P, f);
        }
    }
    if (D_802E8BD0 != 0) {
        s = 0;
    } else {
        s = vs->unk76;
    }
    func_802A039C(1, engine_cvt_w_s((f32)s * 0.6f), P);
}

/* the level is won when it is D_803EF6F0 from where it started */
void func_802BA91C(void) {
    s64 d;

    ENGINE_COST(802BA91C, 27);
    d = dist();
    if (!(d < D_803EF6F0)) {
        D_803643DA = 1;
        D_802E8BD8 = 1;
    }
}

static s32 iabs(s32 v) { return v < 0 ? -v : v; }

/* its collisions (lost if it hits something, or its wheels' heights differ
   by 0x2710 or more), its smoke, and the countdown's effect */
REGS(gp, s7)
void func_802BA9A0(VS *vs, s32 *s7) {
    s32 d, v;

    ENGINE_COST(802BA9A0, 75);
    if (D_803643D6 != 0)
        goto smoke;
    /* ($t0: func_8029A800 (56040.c) takes it from the context, and $t2,
       which this leaves as func_802ABBEC left it) */
    ENGINE_LEAVE(8, T(&D_803643D6));
    func_8029A800(D_803EF6DC, D_803EF6E0, D_803EF6E4, D_80305D60, 0, 0, vs->unk76, 0, 0xFF, vs);
    func_8029C52C(0xFF, vs);
    if (D_803A7426 != 0)
        goto lost;
    v = D_803EF710;
    D_803A7424 = 0;
    if (v != 0) {
        func_8029AA10();
    }
    if (D_803EF711 != 0) {
        D_803F77D0 = P;
        func_802BE77C(0xFF, vs);
    }
    if (D_803A7424 != 0)
        goto lost;
    d = s7[1] - s7[0];
    if (d < 0) {
        d = -d;
    }
    if (!(d < 0x2711))
        goto lost;
    d = s7[4] - s7[3];
    if (d < 0) {
        d = -d;
    }
    if (!(d < 0x2711))
        goto lost;
    d = s7[7] - s7[6];
    if (d < 0) {
        d = -d;
    }
    if (d < 0x2711)
        goto done;
lost:
    D_803643D9 = 1;
    D_802E8BD8 = 1;
smoke:
    if (D_803643D8 == 0) {
        func_80278318();
        func_802A02E4(1, P);
        func_802A5E60();
        func_802C049C();
    }
    if (D_803EF6FE != 0) {
        D_803EF6FE--;
        goto done;
    }
    if (D_803EF6FF != 0)
        goto done;
    if (D_803EF701 == 0) {
        func_802A6274(T(D_802C3B44), 0x7A120, 1, 0xFF, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
        D_803EF701 = 1;
        goto done;
    }
    v = D_803EF700 + 1;
    D_803EF700 = v;
    if (v == 0x12) {
        D_803EF6FF = 1;
    }
done:
    ;
}

/* its matrix, its vertices and its collision */
REGS(gp)
void func_802BABEC(VS *vs) {
    u8 *model = D_803EF70C, *buf;
    s32 *m, off;

    ENGINE_COST(802BABEC, 70);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EF704 + off);
    } else {
        m = (s32 *)(D_803EF708 + off);
    }
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EF6DC, D_803EF6E0, D_803EF6E4, 0x59D8, m);
    if (D_8035805C != 0) {
        buf = D_803EF704;
    } else {
        buf = D_803EF708;
    }
    model = D_803EF70C;
    func_8029C454(D_803EF6DC, D_803EF6E0, D_803EF6E4, 0xFF, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(0xFF, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* the camera's distance and speed for the carrier */
void func_802BAD24(void) {
    ENGINE_COST(802BAD24, 23);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 0xA;
}

/* the crane's set up: from the level loader, with the model file in $s2,
   the position in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802BAD80(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EFA20;
    u8 *buf;
    s32 avg;

    ENGINE_COST(802BAD80, 181);
    D_803EFAD4 = model;
    buf = D_80358070;
    D_803EFAE4 = buf;
    D_803EFAE8 = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(6, 0, D_803EFAE4, D_803EFAE8, model);
    func_802A754C(vs);
    vs->unk52[0] = 0x50;
    vs->unk52[1] = 0x50;
    vs->unk52[2] = -0x50;
    vs->unk52[3] = 0x50;
    vs->unk52[4] = 0x50;
    vs->unk52[5] = -0x50;
    vs->unk5E[0] = 0x5A;
    vs->unk5E[1] = 0x5A;
    vs->unk5E[2] = -0x5A;
    vs->unk5E[3] = 0x5A;
    vs->unk5E[4] = 0x5A;
    vs->unk5E[5] = -0x5A;
    D_803EFAC8 = x;
    D_803EFACC = y;
    D_803EFAD0 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803EFACC, x, z, vs->unk4, &D_803EFACC, (s16 *)&vs->unk4C, 6, vs, 0, &avg);
    func_8029F85C(Q, D_803EFAD4, D_803EFAE4, D_803EFAE8);
    func_802A039C(0, 100, Q);
    func_802A03D4(0, 0, Q);
    func_802A040C(0, 0, Q);
    func_802A0480(0, 0, Q, 0.0f);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, D_803EFAE4, D_803EFAE8);
    func_802A0320(0, Q);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, D_803EFAE8, D_803EFAE4);
    vs->unkA1 = 0;
    vs->unkA2 = 0;
    model = D_803EFAD4;
    func_8029C354(6, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x1B58);
    func_80258230(6, 0x96, 0x2D, 0x2D);
    vs->unk9A = 1;
    func_802BB274();
    vs->unk9A = 0;
    model = D_803EFAD4;
    func_802AA838(D_803EFAE8, D_803EFAE4, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
}

/* hd.c's: getting in: the camera's, and the crane's arm parts' start */
void func_802BB054(void) {
    ENGINE_COST(802BB054, 71);
    D_8036444C = 0x1770;
    D_80364450 = 0x2328;
    func_802A039C(2, 1, Q);
    func_802A03D4(2, 0, Q);
    func_802A040C(2, 1, Q);
    func_802A039C(4, 0, Q);
    func_802A03D4(4, 0, Q);
    func_802A040C(4, 0, Q);
    func_802A0290(4, -1, Q);
    func_802A039C(5, 0, Q);
    func_802A03D4(5, 0, Q);
    func_802A040C(5, 1, Q);
    func_802A0290(5, -1, Q);
}

/* hd.c's: whether the crane can be left: not while its arm is moving */
u8 func_802BB170(void) {
    u8 r = 1;

    ENGINE_COST(802BB170, 11);
    if (D_803EFA20.unkA1 != 0) {
        r = 0;
    }
    return r;
}

/* hd.c's: getting out: this frame's buffer to the other one, and its sounds
   off */
void func_802BB1A0(void) {
    ENGINE_COST(802BB1A0, 32);
    func_802A7764((u32 *)D_803EFAE4, (u32 *)D_803EFAE8, 0x800);
    if (D_803EFADC != NULL) {
        func_802608C8(D_803EFADC);
    }
    if (D_803EFAD8 != NULL) {
        func_802608C8(D_803EFAD8);
    }
    if (D_803EFAE0 != NULL) {
        func_802608C8(D_803EFAE0);
    }
}

/* the crane's light */
void func_802BB230(void) {
    ENGINE_COST(802BB230, 17);
    func_802ABD54(6, D_803EFAC8, D_803EFACC, D_803EFAD0);
}

/* the crane, each frame */
void func_802BB274(void) {
    VS *vs = &D_803EFA20;

    ENGINE_COST(802BB274, 139);
    func_802BB230();
    if (vs->unk9A == 0) {
        func_802BB4C0(vs);
    }
    func_802A92C8(D_803EFAC8, D_803EFAD0, vs->unk5E, (s16 *)&vs->unk4C, vs->unk4, 6, vs, 0);
    if (D_8035805C != 0) {
        func_8029E558(Q, D_803EFAE4, D_803EFAE8);
    } else {
        func_8029E558(Q, D_803EFAE8, D_803EFAE4);
    }
    func_802BB8B8(vs);
    vs->unkA2 = 0;
    func_8029A800(D_803EFAC8, D_803EFACC, D_803EFAD0, D_80305DF0, 0, 0, 0, 0, 6, vs);
    func_8029C52C(6, vs);
    func_8029C5EC();
    D_803F77D0 = Q;
    func_802BE77C(6, vs);
    func_8029AA10();
    if (D_803A7424 != 0) {
        vs->unkA2 = 1;
    }
    D_803643E0 = D_803EFAC8;
    D_803643E4 = D_803EFACC;
    D_803643E8 = D_803EFAD0;
    D_8036443C = 0;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
}

/* the crane's controls: its arm (part 5, the C buttons up and down), its
   turn (part 4, left and right) and its grab (part 2, B or A), with their
   sounds */
REGS(gp)
void func_802BB4C0(VS *vs) {
    s32 r, t1;
    f32 f0;

    ENGINE_COST(802BB4C0, 60);
    if (vs->unkA2 != 0) {
        /* it hit something: all stop, and let go */
        func_802A039C(5, 0, Q);
        func_802A039C(4, 0, Q);
        func_802A03D4(2, 0, Q);
        func_802A0290(2, 1, Q);
        vs->unkA1 = 1;
        goto grab;
    }
    if (D_80370C1C != 0) {
        part(5, Q, NULL, &f0);
        if (f0 <= 0.9f) {
            func_802A03D4(5, 0, Q);
            func_802A039C(5, 1, Q);
            goto arm_on;
        }
    }
    if (D_80370C1D != 0) {
        part(5, Q, NULL, &f0);
        if (!(f0 < 0.1f)) {
            func_802A03D4(5, 1, Q);
            func_802A039C(5, 1, Q);
            goto arm_on;
        }
    }
    func_802A039C(5, 0, Q);
    if (D_803EFAD8 != NULL) {
        func_802608C8(D_803EFAD8);
    }
    goto turn;
arm_on:
    if (D_803EFAD8 == NULL) {
        func_80260650(D_80367738, 0x6F, &D_803EFAD8);
    }
turn:
    if (D_80370C15 != 0) {
        func_802A03D4(4, 0, Q);
        func_802A039C(4, 1, Q);
        goto turn_on;
    }
    if (D_80370C16 != 0) {
        func_802A03D4(4, 1, Q);
        func_802A039C(4, 1, Q);
        goto turn_on;
    }
    func_802A039C(4, 0, Q);
    if (D_803EFADC != NULL) {
        func_802608C8(D_803EFADC);
    }
    goto grab_key;
turn_on:
    if (D_803EFADC == NULL) {
        func_80260650(D_80367738, 0x6E, &D_803EFADC);
    }
grab_key:
    r = func_802BB868();
    if (r != 0) {
        /* something to pick up under it */
        part(2, Q, &t1, &f0);
        if (t1 == 1) {
            if (engine_cvt_w_s(f0 * 100.0f) == 100)
                goto grab;
        }
        func_802A03D4(2, 0, Q);
        func_802A0290(2, 1, Q);
        vs->unkA1 = 1;
        goto grab;
    }
    if (vs->unkA1 != 0)
        goto grab;
    if (D_80370C1A == 0) {
        if (D_80370C1B == 0)
            goto grab;
    }
    if (D_803EFAE0 == NULL) {
        func_80260650(D_80367738, 0x6D, &D_803EFAE0);
    }
    func_802A0290(2, 1, Q);
    vs->unkA1 = 1;
    goto done;
grab:
    /* the grab still moving, or its sound off */
    r = part(2, Q, NULL, NULL);
    if (r == 1)
        goto done;
    vs->unkA1 = 0;
    if (D_803EFAE0 != NULL) {
        func_802608C8(D_803EFAE0);
    }
done:
    ;
}

extern u8 D_803A6B30[];                 /* 0x14-byte records, to 0xFF in byte 0x13 */

/* D_803A6B30's record func_8029C6E4 finds (kind 6, id 0x3BD): the hook's
   position (the original has it in $s0..$s2 from there) */
static s32 *record_3bd(void) {
    u8 *p;

    for (p = D_803A6B30; (s8)p[0x13] != -1; p += 0x14)
        if (p[0x12] == 6 && *(s32 *)(p + 0xC) == 0x3BD)
            return (s32 *)p;
    return NULL;
}

/* on level 0x11, whether there is an object under the crane's hook to pick
   up (func_8029C6E4 finds the hook's position) */
REGS(-> v0)
s32 func_802BB868(void) {
    s32 r = 0, *p;

    ENGINE_COST(802BB868, 20);
    if (D_802E8BDC != 0x11)
        goto done;
    r = func_8029C6E4();
    if (r == 0)
        goto done;
    p = record_3bd();
    r = func_802AC0BC(p[0], p[2], p[1]);
done:
    return r;
}

/* the crane's matrix, its vertices, its arm's and its collision */
REGS(gp)
void func_802BB8B8(VS *vs) {
    u8 *model = D_803EFAD4, *buf;
    s32 *m, off;

    ENGINE_COST(802BB8B8, 95);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EFAE4 + off);
    } else {
        m = (s32 *)(D_803EFAE8 + off);
    }
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EFAC8, D_803EFACC, D_803EFAD0, 0x1B58, m);
    if (D_8035805C != 0) {
        buf = D_803EFAE4;
    } else {
        buf = D_803EFAE8;
    }
    model = D_803EFAD4;
    func_8029C454(D_803EFAC8, D_803EFACC, D_803EFAD0, 6, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(6, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    func_802AABE4(6, (u16 *)(model + *(s32 *)(model + 8)), buf, 0, 0);
    func_8029D040(D_803EFAC8, D_803EFAD0, 6, model + *(s32 *)(model + 0xC), vs->unk4C, Q, buf);
}
