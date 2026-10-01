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

/* translated */
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

/* func_802ABD54 (a light), with its arguments left in $a3, $t3..$t5 as
   the original's are */
static void light(s32 type, s32 x, s32 y, s32 z) {
    ENGINE_LEAVE(7, type);
    ENGINE_LEAVE(11, x);
    ENGINE_LEAVE(12, y);
    ENGINE_LEAVE(13, z);
    func_802ABD54(type, x, y, z);
}

/* how far the carrier is from where it started (x and z), with the
   arguments and the result left in $t3..$t7, $s0 and $s1 */
static s64 dist(void) {
    s64 d;

    ENGINE_LEAVE(11, D_803EF6DC);
    ENGINE_LEAVE(12, 0);
    ENGINE_LEAVE(13, D_803EF6E4);
    ENGINE_LEAVE(14, D_803EF6F4);
    ENGINE_LEAVE(15, 0);
    ENGINE_LEAVE(16, D_803EF6F8);
    d = func_802ABCDC(D_803EF6DC, 0, D_803EF6E4, D_803EF6F4, 0, D_803EF6F8);
    ENGINE_LEAVE64(17, d);
    return d;
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

/* the carrier's set up: from the level loader, with the model file in $s2,
   x and z in $t4 and $t5, the heading in $t6, the distance to win in $t7
   and the top speed in $s1 */
REGS(s2, t4, t5, t6, t7, s1)
void func_802B9C50(u8 *model, s32 x, s32 z, s32 heading, s32 dist, s32 speed) {
    VS *vs = &D_803EF630;
    u8 *buf;
    s32 *s3, avg;

    ENGINE_BLK(802B9C50);
    engine_save(ENGINE_T0_T5, 0);
    D_803EF70C = model;
    buf = D_80358070;
    D_803EF704 = buf;
    D_803EF708 = buf + 0xC00;
    D_80358070 = buf + 0x1800;
    func_802A1388(0xFF, 0, D_803EF704, D_803EF708, model);
    ENGINE_BLK(802B9CD0);
    D_803EF6DC = x;
    D_803EF6F4 = x;
    D_803EF6E4 = z;
    D_803EF6F8 = z;
    D_803EF6FC = speed;
    D_803EF6D6 = 0;
    D_803EF6F0 = dist;
    func_802A754C(vs);
    ENGINE_BLK(802B9D2C);
    vs->unk4E = heading;
    vs->unk4C = heading;
    D_803EF6FE = 5;
    D_803EF700 = 0;
    D_803EF701 = 0;
    func_802BA074();
    ENGINE_BLK(802B9D54);
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
    s3 = func_802A992C(vs->unk52, 0x7FFF, D_803EF6DC, D_803EF6E4, vs->unk4, &D_803EF6E0, (s16 *)&vs->unk4C, 0xFF, vs,
                       engine_ctx(30), &avg);
    /* (the registers it leaves its caller, but for what func_802BABEC
       sets again) */
    ENGINE_LEAVE(16, D_803EF6E4);
    ENGINE_LEAVE(17, T(vs->unk4));
    ENGINE_LEAVE(18, T(&D_803EF6E0));
    ENGINE_LEAVE(19, T(s3));
    ENGINE_LEAVE(21, avg);
    ENGINE_BLK(802B9DEC);
    func_8029F85C(P, D_803EF70C, D_803EF704, D_803EF708);
    ENGINE_BLK(802B9E28);
    func_802A039C(0, 100, P);
    ENGINE_BLK(802B9E3C);
    func_802A03D4(0, 0, P);
    ENGINE_BLK(802B9E50);
    func_802A040C(0, 0, P);
    ENGINE_BLK(802B9E64);
    func_802A0480(0, 0, P, 0.0f);
    ENGINE_BLK(802B9E7C);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802B9E90);
    func_8029E558(P, D_803EF704, D_803EF708);
    ENGINE_BLK(802B9EA4);
    func_802A0320(0, P);
    ENGINE_BLK(802B9EB4);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802B9EC8);
    func_8029E558(P, D_803EF708, D_803EF704);
    ENGINE_BLK(802B9EDC);
    model = D_803EF70C;
    func_8029C354(0xFF, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x59D8);
    ENGINE_BLK(802B9F04);
    func_80258230(0xFF, 0x96, 0x3C, 0x3C);
    ENGINE_BLK(802B9F1C);
    func_802BABEC(vs);
    ENGINE_BLK(802B9F24);
    vs->unk9A = 1;
    func_802BA354();
    ENGINE_BLK(802B9F38);
    vs->unk9A = 0;
    model = D_803EF70C;
    func_802AA838(D_803EF708, D_803EF704, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802B9F70);
    func_802A039C(1, 0, P);
    ENGINE_BLK(802B9F84);
    func_802A03D4(1, 0, P);
    ENGINE_BLK(802B9F98);
    func_802A040C(1, 0, P);
    ENGINE_BLK(802B9FAC);
    func_802A0290(1, -1, P);
    ENGINE_BLK(802B9FC0);
    func_802A039C(2, 0, P);
    ENGINE_BLK(802B9FD4);
    func_802A03D4(2, 0, P);
    ENGINE_BLK(802B9FE8);
    func_802A040C(2, 1, P);
    ENGINE_BLK(802B9FFC);
    func_802A0290(2, -1, P);
    ENGINE_BLK(802BA010);
    func_802A05D0(D_802C236C, 100);
    ENGINE_BLK(802BA020);
    func_802A05F8(D_802C236C, 0);
    ENGINE_BLK(802BA030);
    func_802A0620(D_802C236C, 1);
    ENGINE_BLK(802BA040);
    func_802A0508(D_802C236C, -1);
    ENGINE_BLK(802BA050);
    engine_restore();
    ENGINE_LEAVE(28, T(vs));
}

/* how far it goes before D_803EF710 and D_803EF711: the level's record in
   D_80305D74, or 0 */
void func_802BA074(void) {
    s16 *p;
    s32 l, v;

    ENGINE_BLK(802BA074);
    D_803EF6E8 = 0;
    D_803EF710 = 0;
    D_803EF6EC = 0;
    D_803EF711 = 0;
    l = D_802E8BDC;
    p = D_80305D74;
    for (;;) {
        ENGINE_BLK(802BA0B8);
        v = p[0];
        if (v < 0)
            goto done;
        ENGINE_BLK(802BA0C4);
        p += 3;
        if (l == v)
            break;
    }
    ENGINE_BLK(802BA0CC);
    D_803EF6E8 = (u16)p[-2] << 5;
    D_803EF6EC = (u16)p[-1] << 5;
done:
    ENGINE_BLK(802BA0EC);
}

/* its light */
void func_802BA104(void) {
    ENGINE_BLK(802BA104);
    light(0xFF, D_803EF6DC, D_803EF6E0, D_803EF6E4);
    ENGINE_BLK(802BA138);
}

/* its engine's sound: on within 0x3E80 of the player, louder nearer, panned
   by x */
void func_802BA148(void) {
    s32 d, dx, v;

    ENGINE_BLK(802BA148);
    engine_save(0x5FFFFFFE, 0);
    dx = D_803643E0 - D_803EF6DC;
    d = (s32)func_802ABCDC(D_803643E0, D_803643E4, D_803643E8, D_803EF6DC, D_803EF6E0, D_803EF6E4);
    ENGINE_BLK(802BA21C);
    if (!(d < 0x3E81)) {
        ENGINE_BLK(802BA22C);
        if (D_803EF6D8 == NULL)
            goto done;
        ENGINE_BLK(802BA234);
        func_802608C8(D_803EF6D8);
        ENGINE_BLK(802BA23C);
        D_803EF6D8 = NULL;
        goto done;
    }
    ENGINE_BLK(802BA248);
    if (D_803EF6D8 == NULL) {
        ENGINE_BLK(802BA250);
        func_80260650(D_80367738, 0x75, &D_803EF6D8);
    }
    ENGINE_BLK(802BA268);
    d -= 0xFA0;
    if (d < 0) {
        ENGINE_BLK(802BA274);
        d = 0;
    }
    ENGINE_BLK(802BA278);
    func_80260AB8(D_803EF6D8, 8, 0x7FFF - (d << 1));
    ENGINE_BLK(802BA294);
    v = 0x40 + (dx >> 5);
    if (v < 0) {
        ENGINE_BLK(802BA2B8);
        v = 0;
    } else {
        ENGINE_BLK(802BA2A8);
        if (!(v < 0x80)) {
            ENGINE_BLK(802BA2B0);
            v = 0x7F;
        }
    }
    ENGINE_BLK(802BA2BC);
    func_80260AB8(D_803EF6D8, 4, v);
done:
    ENGINE_BLK(802BA2CC);
    engine_restore();
}

/* each frame */
void func_802BA354(void) {
    VS *vs = &D_803EF630;
    s32 t3 = 0, x, z, s;
    f32 rate;

    ENGINE_BLK(802BA354);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802BA104();
    ENGINE_BLK(802BA3A4);
    func_802BA5A4();
    ENGINE_BLK(802BA3AC);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802BA3C0);
        func_802BA6AC(vs);
        ENGINE_BLK(802BA3C8);
        if ((s8)D_803643D6 != 0)
            goto placed;
        ENGINE_BLK(802BA3D8);
        if ((s8)D_802E8BD0 != 0)
            goto placed;
    }
    ENGINE_BLK(802BA3E8);
    func_802BA638();
    /* speed up by 8 a frame to the top speed */
    ENGINE_BLK(802BA3F0);
    s = vs->unk76;
    if (s < D_803EF6FC) {
        ENGINE_BLK(802BA410);
        s += 8;
        if (D_803EF6FC < s) {
            ENGINE_BLK(802BA420);
            s = D_803EF6FC;
        }
    }
    ENGINE_BLK(802BA424);
    vs->unk76 = s;
    func_802BAD24();
    ENGINE_BLK(802BA42C);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802BA440);
    func_802A843C(&vs->unk76, 0, 0xFF, (s8 *)vs->unk96, vs->unk4, 2400.0f, vs);
    ENGINE_BLK(802BA454);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EF6DC, &D_803EF6E4, rate, &z);
    ENGINE_BLK(802BA46C);
    D_803ED40B = 1;
    /* ($s4 and $s7, which func_802A8768 reads too) */
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803EF6DC, &D_803EF6E4, &D_803EF6E0, 0xFF, 0x960, 0x320, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
placed:
    ENGINE_BLK(802BA4A8);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BA4D0);
        func_8029E558(P, D_803EF704, D_803EF708);
        ENGINE_BLK(802BA4E4);
    } else {
        ENGINE_BLK(802BA4EC);
        func_8029E558(P, D_803EF708, D_803EF704);
    }
    ENGINE_BLK(802BA500);
    func_802BABEC(vs);
    ENGINE_BLK(802BA508);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802BA518);
        func_802BA9A0(vs, vs->unk4);
    }
    ENGINE_BLK(802BA520);
    func_802BA91C();
    ENGINE_BLK(802BA528);
    func_802A133C(D_803EF6DC, D_803EF6E0, D_803EF6E4, 0xFF, vs);
    ENGINE_BLK(802BA554);
    engine_restore();
}

/* whether it is past D_803EF6E8 and D_803EF6EC from where it started */
void func_802BA5A4(void) {
    s64 d;

    ENGINE_BLK(802BA5A4);
    d = dist();
    ENGINE_BLK(802BA5E8);
    if (!(d < D_803EF6E8)) {
        ENGINE_BLK(802BA5FC);
        D_803EF710 = 1;
    }
    ENGINE_BLK(802BA608);
    if (!(d < D_803EF6EC)) {
        ENGINE_BLK(802BA61C);
        D_803EF711 = 1;
    }
    ENGINE_BLK(802BA628);
}

/* its top speed: the last of the level's records in D_80305D62 that it is
   past, if any */
void func_802BA638(void) {
    u8 *p = D_80305D62;
    s32 z = D_803EF6E4 >> 5, l = D_802E8BDC, v = 0, rz;

    ENGINE_BLK(802BA638);
    for (;;) {
        ENGINE_BLK(802BA660);
        rz = *(s16 *)p;
        if (rz < 0)
            break;
        ENGINE_BLK(802BA66C);
        if (p[2] != l) {
            p += 4;
            continue;
        }
        ENGINE_BLK(802BA678);
        if (z < rz) {
            p += 4;
            continue;
        }
        ENGINE_BLK(802BA684);
        v = p[3];
        p += 4;
    }
    ENGINE_BLK(802BA690);
    if (v != 0) {
        ENGINE_BLK(802BA698);
        D_803EF6FC = v;
    }
    ENGINE_BLK(802BA69C);
}

/* its sound's pitch by its speed, its wheels' turn (part 2, by the camera's
   heading) and its drive shaft's (part 1, by its speed) */
REGS(gp)
void func_802BA6AC(VS *vs) {
    s32 s, a;
    u32 h, q;
    f32 f;

    ENGINE_BLK(802BA6AC);
    engine_save(0x5FFFFFFE, 0);
    s = vs->unk76;
    a = D_803EF6D6;
    D_803EF6D6 = s;
    if (a == s)
        goto turn;
    ENGINE_BLK(802BA74C);
    if (D_803EF6D8 == NULL)
        goto turn;
    ENGINE_BLK(802BA75C);
    func_80260AB8(D_803EF6D8, 0x10, f2i(1.5f + (f32)s * -0.004f));
turn:
    ENGINE_BLK(802BA78C);
    h = (u16)D_80364452 + 0x800;
    if (!((s32)h < 0x1000)) {
        ENGINE_BLK(802BA820);
        h -= 0xFFF;
    }
    ENGINE_BLK(802BA824);
    ENGINE_BLK(802BA838);
    q = h / 0x555;
    f = (f32)(s32)(h % 0x555) / 1365.0f;
    if (q == 0) {
        ENGINE_BLK(802BA888);
        func_802A0360(2, 0, P, f);
        ENGINE_BLK(802BA8A0);
    } else {
        ENGINE_BLK(802BA85C);
        if (q == 1) {
            ENGINE_BLK(802BA8A8);
            func_802A0360(2, 1, P, f);
        } else {
            ENGINE_BLK(802BA868);
            func_802A0360(2, 2, P, f);
            ENGINE_BLK(802BA880);
        }
    }
    ENGINE_BLK(802BA8C0);
    if (D_802E8BD0 != 0) {
        ENGINE_BLK(802BA8D0);
        s = 0;
    } else {
        ENGINE_BLK(802BA8D8);
        s = vs->unk76;
    }
    ENGINE_BLK(802BA8DC);
    func_802A039C(1, engine_cvt_w_s((f32)s * 0.6f), P);
    ENGINE_BLK(802BA90C);
    engine_restore();
}

/* the level is won when it is D_803EF6F0 from where it started */
void func_802BA91C(void) {
    s64 d;

    ENGINE_BLK(802BA91C);
    d = dist();
    ENGINE_BLK(802BA960);
    if (!(d < D_803EF6F0)) {
        ENGINE_BLK(802BA978);
        D_803643DA = 1;
        D_802E8BD8 = 1;
    }
    ENGINE_BLK(802BA990);
}

static s32 iabs(s32 v) { return v < 0 ? -v : v; }

/* its collisions (lost if it hits something, or its wheels' heights differ
   by 0x2710 or more), its smoke, and the countdown's effect */
REGS(gp, s7)
void func_802BA9A0(VS *vs, s32 *s7) {
    s32 d, v;

    ENGINE_BLK(802BA9A0);
    if (D_803643D6 != 0)
        goto smoke;
    ENGINE_BLK(802BA9BC);
    ENGINE_LEAVE(8, T(&D_803643D6));    /* ($t0, which func_8029A800 reads too) */
    func_8029A800(D_803EF6DC, D_803EF6E0, D_803EF6E4, D_80305D60, 0, 0, vs->unk76, 0, 0xFF, vs);
    ENGINE_BLK(802BAA00);
    func_8029C52C(0xFF, vs);
    ENGINE_BLK(802BAA08);
    if (D_803A7426 != 0)
        goto lost;
    ENGINE_BLK(802BAA18);
    v = D_803EF710;
    D_803A7424 = 0;
    if (v != 0) {
        ENGINE_BLK(802BAA30);
        func_8029AA10();
    }
    ENGINE_BLK(802BAA38);
    if (D_803EF711 != 0) {
        ENGINE_BLK(802BAA48);
        D_803F77D0 = P;
        func_802BE77C(0xFF, vs);
    }
    ENGINE_BLK(802BAA64);
    if (D_803A7424 != 0)
        goto lost;
    ENGINE_BLK(802BAA74);
    d = s7[1] - s7[0];
    if (d < 0) {
        ENGINE_BLK(802BAA88);
        d = -d;
    }
    ENGINE_BLK(802BAA8C);
    if (!(d < 0x2711))
        goto lost;
    ENGINE_BLK(802BAA98);
    d = s7[4] - s7[3];
    if (d < 0) {
        ENGINE_BLK(802BAAAC);
        d = -d;
    }
    ENGINE_BLK(802BAAB0);
    if (!(d < 0x2711))
        goto lost;
    ENGINE_BLK(802BAABC);
    d = s7[7] - s7[6];
    if (d < 0) {
        ENGINE_BLK(802BAAD0);
        d = -d;
    }
    ENGINE_BLK(802BAAD4);
    if (d < 0x2711)
        goto done;
lost:
    ENGINE_BLK(802BAAE0);
    D_803643D9 = 1;
    D_802E8BD8 = 1;
smoke:
    ENGINE_BLK(802BAAF8);
    if (D_803643D8 == 0) {
        ENGINE_BLK(802BAB08);
        func_80278318();
        ENGINE_BLK(802BAB10);
        func_802A02E4(1, P);
        ENGINE_BLK(802BAB24);
        func_802A5E60();
        ENGINE_BLK(802BAB2C);
        func_802C049C();
    }
    ENGINE_BLK(802BAB34);
    if (D_803EF6FE != 0) {
        ENGINE_BLK(802BABD0);
        D_803EF6FE--;
        goto done;
    }
    ENGINE_BLK(802BAB44);
    if (D_803EF6FF != 0)
        goto done;
    ENGINE_BLK(802BAB54);
    if (D_803EF701 == 0) {
        ENGINE_BLK(802BAB64);
        func_802A6274(T(D_802C3B44), 0x7A120, 1, 0xFF, 2, 0, engine_ctx(14), engine_ctx(15), engine_ctx(16),
                      engine_ctx(17), engine_ctx(18), engine_ctx(19), engine_ctx(20), 0, 1);
        ENGINE_BLK(802BAB90);
        D_803EF701 = 1;
        goto done;
    }
    ENGINE_BLK(802BABA0);
    v = D_803EF700 + 1;
    D_803EF700 = v;
    if (v == 0x12) {
        ENGINE_BLK(802BABC0);
        D_803EF6FF = 1;
    }
done:
    ENGINE_BLK(802BABDC);
}

/* its matrix, its vertices and its collision */
REGS(gp)
void func_802BABEC(VS *vs) {
    u8 *model = D_803EF70C, *buf;
    s32 *m, off;

    ENGINE_BLK(802BABEC);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BAC1C);
        m = (s32 *)(D_803EF704 + off);
    } else {
        ENGINE_BLK(802BAC30);
        m = (s32 *)(D_803EF708 + off);
    }
    ENGINE_BLK(802BAC40);
    D_803ED390[1] = vs->unk4C;
    /* (its $s4..$s7, which the setup leaves its caller) */
    ENGINE_LEAVE(20, D_803EF6DC);
    ENGINE_LEAVE(21, D_803EF6E0);
    ENGINE_LEAVE(22, D_803EF6E4);
    ENGINE_LEAVE(23, 0x59D8);
    func_802AA764(D_803EF6DC, D_803EF6E0, D_803EF6E4, 0x59D8, m);
    ENGINE_LEAVE(18, T(m));           /* (its $s2, as the glue would) */
    ENGINE_BLK(802BAC7C);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BAC90);
        buf = D_803EF704;
    } else {
        ENGINE_BLK(802BACA0);
        buf = D_803EF708;
    }
    ENGINE_BLK(802BACAC);
    model = D_803EF70C;
    ENGINE_LEAVE(11, T(model));
    func_8029C454(D_803EF6DC, D_803EF6E0, D_803EF6E4, 0xFF, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802BACF4);
    ENGINE_LEAVE(11, T(model));
    func_802ABBEC(0xFF, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802BAD14);
}

/* the camera's distance and speed for the carrier */
void func_802BAD24(void) {
    ENGINE_BLK(802BAD24);
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
    s32 *s3, avg;

    ENGINE_BLK(802BAD80);
    engine_save(ENGINE_T0_T5, 0);
    D_803EFAD4 = model;
    buf = D_80358070;
    D_803EFAE4 = buf;
    D_803EFAE8 = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(6, 0, D_803EFAE4, D_803EFAE8, model);
    ENGINE_BLK(802BAE00);
    func_802A754C(vs);
    ENGINE_BLK(802BAE0C);
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
    s3 = func_802A992C(vs->unk52, D_803EFACC, x, z, vs->unk4, &D_803EFACC, (s16 *)&vs->unk4C, 6, vs, engine_ctx(30),
                       &avg);
    ENGINE_LEAVE(19, T(s3));
    ENGINE_LEAVE(21, avg);
    ENGINE_BLK(802BAEAC);
    func_8029F85C(Q, D_803EFAD4, D_803EFAE4, D_803EFAE8);
    ENGINE_BLK(802BAEE8);
    func_802A039C(0, 100, Q);
    ENGINE_BLK(802BAEFC);
    func_802A03D4(0, 0, Q);
    ENGINE_BLK(802BAF10);
    func_802A040C(0, 0, Q);
    ENGINE_BLK(802BAF24);
    func_802A0480(0, 0, Q, 0.0f);
    ENGINE_BLK(802BAF3C);
    func_802A0290(0, 1, Q);
    ENGINE_BLK(802BAF50);
    func_8029E558(Q, D_803EFAE4, D_803EFAE8);
    ENGINE_BLK(802BAF64);
    func_802A0320(0, Q);
    ENGINE_BLK(802BAF74);
    func_802A0290(0, 1, Q);
    ENGINE_BLK(802BAF88);
    func_8029E558(Q, D_803EFAE8, D_803EFAE4);
    ENGINE_BLK(802BAF9C);
    vs->unkA1 = 0;
    vs->unkA2 = 0;
    model = D_803EFAD4;
    func_8029C354(6, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x1B58);
    ENGINE_BLK(802BAFD0);
    func_80258230(6, 0x96, 0x2D, 0x2D);
    ENGINE_BLK(802BAFE8);
    vs->unk9A = 1;
    func_802BB274();
    ENGINE_BLK(802BAFF8);
    vs->unk9A = 0;
    model = D_803EFAD4;
    func_802AA838(D_803EFAE8, D_803EFAE4, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802BB030);
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(17, T(vs->unk4));
    ENGINE_LEAVE(18, T(&D_803EFACC));
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(22, T(D_803EFAE4));
    ENGINE_LEAVE(23, T(D_803EFAE8));
}

/* hd.c's: getting in: the camera's, and the crane's arm parts' start */
void func_802BB054(void) {
    ENGINE_BLK(802BB054);
    D_8036444C = 0x1770;
    D_80364450 = 0x2328;
    func_802A039C(2, 1, Q);
    ENGINE_BLK(802BB098);
    func_802A03D4(2, 0, Q);
    ENGINE_BLK(802BB0AC);
    func_802A040C(2, 1, Q);
    ENGINE_BLK(802BB0C0);
    func_802A039C(4, 0, Q);
    ENGINE_BLK(802BB0D4);
    func_802A03D4(4, 0, Q);
    ENGINE_BLK(802BB0E8);
    func_802A040C(4, 0, Q);
    ENGINE_BLK(802BB0FC);
    func_802A0290(4, -1, Q);
    ENGINE_BLK(802BB110);
    func_802A039C(5, 0, Q);
    ENGINE_BLK(802BB124);
    func_802A03D4(5, 0, Q);
    ENGINE_BLK(802BB138);
    func_802A040C(5, 1, Q);
    ENGINE_BLK(802BB14C);
    func_802A0290(5, -1, Q);
    ENGINE_BLK(802BB160);
    ENGINE_LEAVE(28, T(&D_803EFA20));
}

/* hd.c's: whether the crane can be left: not while its arm is moving */
u8 func_802BB170(void) {
    u8 r = 1;

    ENGINE_BLK(802BB170);
    if (D_803EFA20.unkA1 != 0) {
        ENGINE_BLK(802BB18C);
        r = 0;
    }
    ENGINE_BLK(802BB190);
    ENGINE_LEAVE(28, T(&D_803EFA20));
    return r;
}

/* hd.c's: getting out: this frame's buffer to the other one, and its sounds
   off */
void func_802BB1A0(void) {
    ENGINE_BLK(802BB1A0);
    func_802A7764((u32 *)D_803EFAE4, (u32 *)D_803EFAE8, 0x800);
    ENGINE_BLK(802BB1D0);
    if (D_803EFADC != NULL) {
        ENGINE_BLK(802BB1E0);
        func_802608C8(D_803EFADC);
    }
    ENGINE_BLK(802BB1E8);
    if (D_803EFAD8 != NULL) {
        ENGINE_BLK(802BB1F8);
        func_802608C8(D_803EFAD8);
    }
    ENGINE_BLK(802BB200);
    if (D_803EFAE0 != NULL) {
        ENGINE_BLK(802BB210);
        func_802608C8(D_803EFAE0);
    }
    ENGINE_BLK(802BB218);
}

/* the crane's light */
void func_802BB230(void) {
    ENGINE_BLK(802BB230);
    light(6, D_803EFAC8, D_803EFACC, D_803EFAD0);
    ENGINE_BLK(802BB264);
}

/* the crane, each frame */
void func_802BB274(void) {
    VS *vs = &D_803EFA20;

    ENGINE_BLK(802BB274);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802BB230();
    ENGINE_BLK(802BB2CC);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802BB2D8);
        func_802BB4C0(vs);
    }
    ENGINE_BLK(802BB2E0);
    func_802A92C8(D_803EFAC8, D_803EFAD0, vs->unk5E, (s16 *)&vs->unk4C, vs->unk4, 6, vs, engine_ctx(30));
    ENGINE_BLK(802BB30C);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BB334);
        func_8029E558(Q, D_803EFAE4, D_803EFAE8);
        ENGINE_BLK(802BB348);
    } else {
        ENGINE_BLK(802BB350);
        func_8029E558(Q, D_803EFAE8, D_803EFAE4);
    }
    ENGINE_BLK(802BB364);
    func_802BB8B8(vs);
    ENGINE_BLK(802BB36C);
    vs->unkA2 = 0;
    func_8029A800(D_803EFAC8, D_803EFACC, D_803EFAD0, D_80305DF0, 0, 0, 0, 0, 6, vs);
    ENGINE_BLK(802BB3B4);
    func_8029C52C(6, vs);
    ENGINE_BLK(802BB3BC);
    func_8029C5EC();
    ENGINE_BLK(802BB3C4);
    D_803F77D0 = Q;
    func_802BE77C(6, vs);
    ENGINE_BLK(802BB3E0);
    func_8029AA10();
    ENGINE_BLK(802BB3E8);
    if (D_803A7424 != 0) {
        ENGINE_BLK(802BB3F8);
        vs->unkA2 = 1;
    }
    ENGINE_BLK(802BB400);
    D_803643E0 = D_803EFAC8;
    D_803643E4 = D_803EFACC;
    D_803643E8 = D_803EFAD0;
    D_8036443C = 0;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    engine_restore();
}

/* the crane's controls: its arm (part 5, the C buttons up and down), its
   turn (part 4, left and right) and its grab (part 2, B or A), with their
   sounds */
REGS(gp)
void func_802BB4C0(VS *vs) {
    s32 r, t1;
    f32 f0;

    ENGINE_BLK(802BB4C0);
    if (vs->unkA2 != 0) {
        /* it hit something: all stop, and let go */
        ENGINE_BLK(802BB4D4);
        func_802A039C(5, 0, Q);
        ENGINE_BLK(802BB4E8);
        func_802A039C(4, 0, Q);
        ENGINE_BLK(802BB4FC);
        func_802A03D4(2, 0, Q);
        ENGINE_BLK(802BB510);
        func_802A0290(2, 1, Q);
        ENGINE_BLK(802BB524);
        vs->unkA1 = 1;
        goto grab;
    }
    ENGINE_BLK(802BB530);
    if (D_80370C1C != 0) {
        ENGINE_BLK(802BB540);
        part(5, Q, NULL, &f0);
        ENGINE_BLK(802BB550);
        if (f0 <= 0.9f) {
            ENGINE_BLK(802BB564);
            func_802A03D4(5, 0, Q);
            ENGINE_BLK(802BB578);
            func_802A039C(5, 1, Q);
            ENGINE_BLK(802BB58C);
            goto arm_on;
        }
    }
    ENGINE_BLK(802BB594);
    if (D_80370C1D != 0) {
        ENGINE_BLK(802BB5A4);
        part(5, Q, NULL, &f0);
        ENGINE_BLK(802BB5B4);
        if (!(f0 < 0.1f)) {
            ENGINE_BLK(802BB5C8);
            func_802A03D4(5, 1, Q);
            ENGINE_BLK(802BB5DC);
            func_802A039C(5, 1, Q);
            ENGINE_BLK(802BB5F0);
            goto arm_on;
        }
    }
    ENGINE_BLK(802BB5F8);
    func_802A039C(5, 0, Q);
    ENGINE_BLK(802BB60C);
    if (D_803EFAD8 != NULL) {
        ENGINE_BLK(802BB61C);
        func_802608C8(D_803EFAD8);
        ENGINE_BLK(802BB624);
    }
    goto turn;
arm_on:
    ENGINE_BLK(802BB62C);
    if (D_803EFAD8 == NULL) {
        ENGINE_BLK(802BB640);
        func_80260650(D_80367738, 0x6F, &D_803EFAD8);
    }
turn:
    ENGINE_BLK(802BB650);
    if (D_80370C15 != 0) {
        ENGINE_BLK(802BB660);
        func_802A03D4(4, 0, Q);
        ENGINE_BLK(802BB674);
        func_802A039C(4, 1, Q);
        ENGINE_BLK(802BB688);
        goto turn_on;
    }
    ENGINE_BLK(802BB690);
    if (D_80370C16 != 0) {
        ENGINE_BLK(802BB6A0);
        func_802A03D4(4, 1, Q);
        ENGINE_BLK(802BB6B4);
        func_802A039C(4, 1, Q);
        ENGINE_BLK(802BB6C8);
        goto turn_on;
    }
    ENGINE_BLK(802BB6D0);
    func_802A039C(4, 0, Q);
    ENGINE_BLK(802BB6E4);
    if (D_803EFADC != NULL) {
        ENGINE_BLK(802BB6F4);
        func_802608C8(D_803EFADC);
        ENGINE_BLK(802BB6FC);
    }
    goto grab_key;
turn_on:
    ENGINE_BLK(802BB704);
    if (D_803EFADC == NULL) {
        ENGINE_BLK(802BB718);
        func_80260650(D_80367738, 0x6E, &D_803EFADC);
    }
grab_key:
    ENGINE_BLK(802BB728);
    r = func_802BB868();
    ENGINE_BLK(802BB730);
    if (r != 0) {
        /* something to pick up under it */
        ENGINE_BLK(802BB738);
        part(2, Q, &t1, &f0);
        ENGINE_BLK(802BB748);
        if (t1 == 1) {
            ENGINE_BLK(802BB754);
            if (engine_cvt_w_s(f0 * 100.0f) == 100)
                goto grab;
        }
        ENGINE_BLK(802BB774);
        func_802A03D4(2, 0, Q);
        ENGINE_BLK(802BB788);
        func_802A0290(2, 1, Q);
        ENGINE_BLK(802BB79C);
        vs->unkA1 = 1;
        goto grab;
    }
    ENGINE_BLK(802BB7A8);
    if (vs->unkA1 != 0)
        goto grab;
    ENGINE_BLK(802BB7B4);
    if (D_80370C1A == 0) {
        ENGINE_BLK(802BB7C4);
        if (D_80370C1B == 0)
            goto grab;
    }
    ENGINE_BLK(802BB7D4);
    if (D_803EFAE0 == NULL) {
        ENGINE_BLK(802BB7E8);
        func_80260650(D_80367738, 0x6D, &D_803EFAE0);
    }
    ENGINE_BLK(802BB7F8);
    func_802A0290(2, 1, Q);
    ENGINE_BLK(802BB80C);
    vs->unkA1 = 1;
    goto done;
grab:
    /* the grab still moving, or its sound off */
    ENGINE_BLK(802BB818);
    r = part(2, Q, NULL, NULL);
    ENGINE_BLK(802BB828);
    if (r == 1)
        goto done;
    ENGINE_BLK(802BB834);
    vs->unkA1 = 0;
    if (D_803EFAE0 != NULL) {
        ENGINE_BLK(802BB84C);
        func_802608C8(D_803EFAE0);
    }
done:
    ENGINE_BLK(802BB854);
}

/* on level 0x11, whether there is an object under the crane's hook to pick
   up (func_8029C6E4 finds the hook's position) */
REGS(-> v0)
s32 func_802BB868(void) {
    s32 r = 0;

    ENGINE_BLK(802BB868);
    if (D_802E8BDC != 0x11)
        goto done;
    ENGINE_BLK(802BB884);
    r = func_8029C6E4();
    ENGINE_BLK(802BB88C);
    if (r == 0)
        goto done;
    ENGINE_BLK(802BB894);
    ENGINE_LEAVE(8, engine_ctx(16));
    ENGINE_LEAVE(10, engine_ctx(17));
    ENGINE_LEAVE(9, engine_ctx(18));
    r = func_802AC0BC(engine_ctx(16), engine_ctx(18), engine_ctx(17));
    ENGINE_LEAVE(5, r);
    ENGINE_BLK(802BB8A4);
done:
    ENGINE_BLK(802BB8A8);
    return r;
}

/* the crane's matrix, its vertices, its arm's and its collision */
REGS(gp)
void func_802BB8B8(VS *vs) {
    u8 *model = D_803EFAD4, *buf;
    s32 *m, off;

    ENGINE_BLK(802BB8B8);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BB8E8);
        m = (s32 *)(D_803EFAE4 + off);
    } else {
        ENGINE_BLK(802BB8FC);
        m = (s32 *)(D_803EFAE8 + off);
    }
    ENGINE_BLK(802BB90C);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    ENGINE_LEAVE(20, D_803EFAC8);
    ENGINE_LEAVE(21, D_803EFACC);
    ENGINE_LEAVE(22, D_803EFAD0);
    ENGINE_LEAVE(23, 0x1B58);
    func_802AA764(D_803EFAC8, D_803EFACC, D_803EFAD0, 0x1B58, m);
    ENGINE_LEAVE(18, T(m));           /* (its $s2, as the glue would) */
    ENGINE_BLK(802BB950);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BB964);
        buf = D_803EFAE4;
    } else {
        ENGINE_BLK(802BB974);
        buf = D_803EFAE8;
    }
    ENGINE_BLK(802BB980);
    model = D_803EFAD4;
    ENGINE_LEAVE(11, T(model));
    func_8029C454(D_803EFAC8, D_803EFACC, D_803EFAD0, 6, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802BB9C8);
    ENGINE_LEAVE(11, T(model));
    func_802ABBEC(6, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802BB9E8);
    func_802AABE4(6, (u16 *)(model + *(s32 *)(model + 8)), buf, engine_ctx(17), engine_ctx(18));
    ENGINE_BLK(802BBA04);
    func_8029D040(D_803EFAC8, D_803EFAD0, 6, model + *(s32 *)(model + 0xC), vs->unk4C, Q, buf);
    ENGINE_BLK(802BBA44);
}
