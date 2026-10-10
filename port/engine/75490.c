/*
 * hd_code 75490 (us.v11 0x802B9C50-0x802BBA60): the missile carrier (type
 * 0xFF) and the crane (type 6), as native C (engine.h).
 *
 * The carrier's parts are D_803EF330, its state D_803EF630 and its position
 * D_803EF6DC..E4.  func_802B9C50 sets it up (from the level loader),
 * func_802BA354 runs it each frame (from hd.c and at the end of the setup):
 * it rolls along z at up to D_803EF6FC (the level's records in D_80305D62
 * raise it as it goes), and the level is won when it is D_803EF6F0 from
 * where it started (D_803643DA), lost when it hits something or its wheels
 * stand too unevenly (D_803643D9).  func_802BA148 is its engine's sound.
 *
 * The crane's parts are D_803EF720, its state D_803EFA20 and its position
 * D_803EFAC8..D0: func_802BAD80 sets it up, func_802BB274 runs it, and
 * func_802BB054, func_802BB170 and func_802BB1A0 are hd.c's hooks for
 * getting in, whether it can be left, and getting out.
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"

/* the carrier's .bss (asm/data/hd_code/75490.bss.s) */
extern Part D_803EF330[32];
extern VS D_803EF630;
extern SndState *D_803EF6D8;                    /* its engine's sound, while the player is near */
extern s32 D_803EF6DC, D_803EF6E0, D_803EF6E4;  /* x, y, z */
extern s32 D_803EF6E8, D_803EF6EC;              /* how far it goes before D_803EF710, D_803EF711 */
extern s32 D_803EF6F0;                          /* how far it goes to win the level */
extern s32 D_803EF6F4, D_803EF6F8;              /* where it started: x, z */
extern s16 D_803EF6FC;                          /* its top speed */
extern u8 D_803EF6FE;                           /* frames before the countdown's effect */
extern u8 D_803EF6FF;                           /* the effect's frames are over */
extern u8 D_803EF700;                           /* the effect's frames */
extern u8 D_803EF701;                           /* the effect has started */
extern u8 *D_803EF704;                          /* two 0xC00-byte buffers, one per frame */
extern u8 *D_803EF708;
extern u8 *D_803EF70C;                          /* its model file */
extern u8 D_803EF710;                           /* it is far enough on to hit the parts' collision */
extern u8 D_803EF711;                           /* ... and the buildings' */
/* the crane's */
extern Part D_803EF720[32];
extern VS D_803EFA20;
extern s32 D_803EFAC8, D_803EFACC, D_803EFAD0;  /* x, y, z */
extern u8 *D_803EFAD4;                          /* its model file */
extern SndState *D_803EFAD8;                    /* its three sounds: the arm's, the turn's, the grab's */
extern SndState *D_803EFADC;
extern SndState *D_803EFAE0;
extern u8 *D_803EFAE4;                          /* two 0x800-byte buffers, one per frame */
extern u8 *D_803EFAE8;

#define P D_803EF330
#define Q D_803EF720
#define CMO_X D_803EF6DC
#define CMO_Y D_803EF6E0
#define CMO_Z D_803EF6E4
#define CMO_MODEL D_803EF70C
#define CMO_BUF0 D_803EF704
#define CMO_BUF1 D_803EF708
#define CR_X D_803EFAC8
#define CR_Y D_803EFACC
#define CR_Z D_803EFAD0
#define CR_MODEL D_803EFAD4
#define CR_BUF0 D_803EFAE4
#define CR_BUF1 D_803EFAE8

/* VehicleState's bytes the crane uses for itself */
#define CRANE_GRABBING(vs) ((vs)->unkA1) /* its grab (part 2) is moving */
#define CRANE_HIT(vs) ((vs)->unkA2)     /* it hit something last frame */

extern u8 D_80305D60[];                         /* the carrier's parts' collision (56040's func_8029A800) */
extern u8 D_80305D62[];                         /* {s16 z >> 5, u8 level, u8 top speed}, to z < 0 */
extern s16 D_80305D74[];                        /* {level, distance >> 5 to D_803EF710, to D_803EF711} */
extern u8 D_80305DF0[];                         /* the crane's parts' collision */
extern u8 D_802C236C[];                         /* the carrier's lights (56040's list) */
extern u8 D_802C3B44[];                         /* the countdown's effect record (60F60) */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern u8 D_803643D6, D_803643D8;
extern u8 D_803A7424, D_803A7426;           /* the collision walk hit something (77E20, 56040) */

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


/* ---- the carrier's numbers (a frame, where it's per frame) ----------------- */

#define CMO_SCALE 0x59D8                /* its model's scale */
#define CMO_SPAN_ALONG 0x960            /* its wheels' spans (func_802A8768) */
#define CMO_SPAN_ACROSS 0x320
#define CMO_ACCEL 8                     /* its speed up to the top a frame */
#define CMO_SLOPE_DIV 2400.0f           /* func_802A843C: the slope's push is the height difference over this */
#define CMO_GRAVITY 2.0f                /* times the level's */
#define CMO_BOUNCE_MIN 0x3C             /* a landing harder than this bounces ... */
#define CMO_BOUNCE_DIV 0xA              /* ... at the speed over this */
#define CMO_UNEVEN 0x2711               /* a wheel's height changing this much in a frame loses the level */
#define CMO_SHAFT_SPIN 0.6f             /* its drive shaft's speed: the speed times this */
#define CMO_SOUND_RANGE 0x3E80          /* its engine heard within this of the player */
#define CMO_COUNTDOWN_DELAY 5           /* frames before the countdown's effect starts ... */
#define CMO_COUNTDOWN_FRAMES 0x12       /* ... and its frames */

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
    return func_802ABCDC(CMO_X, 0, CMO_Z, D_803EF6F4, 0, D_803EF6F8);
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
   pairs where its level has one (func_802BA3E8_jp) */
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
   speed */
REGS(s1 -> s1)
s32 func_802BA3E8_jp(s32 speed) {
    u8 *pairs = (u8 *)D_8030606E_jp;
    s32 i;

    for (i = 0; (s8)pairs[NE_X1(i)] >= 0; i += 2)
        if (D_802E8BDC == (s8)pairs[NE_X1(i)])
            return pairs[NE_X1(i + 1)];
    return speed;
}
#endif

/* the carrier's set up: from the level loader, with the model file, x and
   z, the heading, the distance to win and the top speed */
REGS(s2, t4, t5, t6, t7, s1)
void func_802B9C50(u8 *model, s32 x, s32 z, s32 heading, s32 dist, s32 speed) {
    VS *vs = &D_803EF630;
    s32 avg;

    CMO_MODEL = model;
    CMO_BUF0 = D_80358070;
    CMO_BUF1 = D_80358070 + 0xC00;
    D_80358070 += 0x1800;
    func_802A1388(VEHICLE_CMO, 0, CMO_BUF0, CMO_BUF1, model);
    CMO_X = x;
    D_803EF6F4 = x;
    CMO_Z = z;
    D_803EF6F8 = z;
#ifdef VERSION_JP
    speed = func_802BA3E8_jp(speed);
#endif
    D_803EF6FC = speed;
    VS_SOUND_SPEED(vs) = 0;
    D_803EF6F0 = dist;
    func_802A754C(vs);
    VS_MOVE_HEADING(vs) = heading;
    VS_HEADING(vs) = heading;
    D_803EF6FE = CMO_COUNTDOWN_DELAY;
    D_803EF700 = 0;
    D_803EF701 = 0;
    func_802BA074();
    SET_WHEELS(VS_WHEELS(vs), 0x190, 0x4B0, -0x190, 0x4B0, 0x190, -0x4B0);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x1A4, 0x4D8, -0x1A4, 0x4D8, 0x1A4, -0x4D8);
    func_802A992C(VS_WHEELS(vs), 0x7FFF, CMO_X, CMO_Z, VS_WHEEL_H(vs), &CMO_Y, (s16 *)&VS_HEADING(vs), VEHICLE_CMO, vs,
                  0, &avg);
    func_8029F85C(P, CMO_MODEL, CMO_BUF0, CMO_BUF1);
    func_802A039C(0, 100, P);
    func_802A03D4(0, 0, P);
    func_802A040C(0, 0, P);
    func_802A0480(0, 0, P, 0.0f);
    func_802A0290(0, 1, P);
    func_8029E558(P, CMO_BUF0, CMO_BUF1);
    func_802A0320(0, P);
    func_802A0290(0, 1, P);
    func_8029E558(P, CMO_BUF1, CMO_BUF0);
    func_8029C354(VEHICLE_CMO, MODEL_AT(CMO_MODEL, 4), MODEL_AT(CMO_MODEL, 8), CMO_SCALE);
    func_80258230(VEHICLE_CMO, 0x96, 0x3C, 0x3C);
    func_802BABEC(vs);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802BA354();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(CMO_BUF1, CMO_BUF0, MODEL_MTX_OFF(CMO_MODEL));
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

    D_803EF6E8 = 0;
    D_803EF710 = 0;
    D_803EF6EC = 0;
    D_803EF711 = 0;
    for (p = D_80305D74; p[0] >= 0; p += 3) {
        if (p[0] == D_802E8BDC) {
            D_803EF6E8 = (u16)p[1] << 5;
            D_803EF6EC = (u16)p[2] << 5;
            break;
        }
    }
}

/* its light */
void func_802BA104(void) {
    func_802ABD54(VEHICLE_CMO, CMO_X, CMO_Y, CMO_Z);
}

/* its engine's sound: on within CMO_SOUND_RANGE of the player, louder
   nearer, panned by x */
void func_802BA148(void) {
    s32 d, dx = D_803643E0 - CMO_X, pan;

    d = (s32)func_802ABCDC(D_803643E0, D_803643E4, D_803643E8, CMO_X, CMO_Y, CMO_Z);
    if (d > CMO_SOUND_RANGE) {
        if (D_803EF6D8 != NULL) {
            func_802608C8(D_803EF6D8);
            D_803EF6D8 = NULL;
        }
        return;
    }
    if (D_803EF6D8 == NULL)
        func_80260650(D_80367738, 0x75, &D_803EF6D8);
    d -= 0xFA0;
    if (d < 0)
        d = 0;
    func_80260AB8(D_803EF6D8, 8, 0x7FFF - (d << 1));
    pan = 0x40 + (dx >> 5);
    if (pan < 0)
        pan = 0;
    else if (pan > 0x7F)
        pan = 0x7F;
    func_80260AB8(D_803EF6D8, 4, pan);
}

/* each frame: unless the level is over (D_803643D6) or paused
   (D_802E8BD0), on along its heading at its speed (up by CMO_ACCEL a frame
   to the top) */
void func_802BA354(void) {
    VS *vs = &D_803EF630;
    s32 step = 0, x, z;
    f32 rate;

    func_802BA104();
    func_802BA5A4();
    if (VS_IN_SETUP(vs) == 0)
        func_802BA6AC(vs);
    if (VS_IN_SETUP(vs) != 0 || ((s8)D_803643D6 == 0 && (s8)D_802E8BD0 == 0)) {
        func_802BA638();
        if (VS_SPEED(vs) < D_803EF6FC)
            VS_SPEED(vs) = VS_SPEED(vs) + CMO_ACCEL > D_803EF6FC ? D_803EF6FC : VS_SPEED(vs) + CMO_ACCEL;
        func_802BAD24();
        rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
        func_802A843C(&VS_SPEED(vs), 0, VEHICLE_CMO, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), CMO_SLOPE_DIV, vs);
        x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &CMO_X, &CMO_Z, rate, &z);
        D_803ED40B = 1;
        func_802A8768(x, z, &CMO_X, &CMO_Z, &CMO_Y, VEHICLE_CMO, CMO_SPAN_ALONG, CMO_SPAN_ACROSS, VS_WHEELS(vs),
                      VS_WHEEL_FALL(vs), VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    }
    func_8029E558(P, FRAME_BUF(CMO_BUF0, CMO_BUF1), OTHER_BUF(CMO_BUF0, CMO_BUF1));
    func_802BABEC(vs);
    if (VS_IN_SETUP(vs) == 0)
        func_802BA9A0(vs, VS_WHEEL_H(vs));
    func_802BA91C();
    func_802A133C(CMO_X, CMO_Y, CMO_Z, VEHICLE_CMO, vs);
}

/* whether it is past D_803EF6E8 and D_803EF6EC from where it started */
void func_802BA5A4(void) {
    s64 d;

    d = dist();
    if (d >= D_803EF6E8)
        D_803EF710 = 1;
    if (d >= D_803EF6EC)
        D_803EF711 = 1;
}

/* its top speed: the last of the level's records in D_80305D62 that it is
   past, if any */
void func_802BA638(void) {
    u8 *p;
    s32 z = CMO_Z >> 5, v = 0;

    for (p = D_80305D62; *(s16 *)p >= 0; p += 4)
        if (p[2] == D_802E8BDC && z >= *(s16 *)p)
            v = p[3];
    if (v != 0)
        D_803EF6FC = v;
}

/* its sound's pitch by its speed, its wheels' turn (part 2, by the camera's
   heading, in thirds) and its drive shaft's (part 1, by its speed) */
REGS(gp)
void func_802BA6AC(VS *vs) {
    s32 s = VS_SPEED(vs), last = VS_SOUND_SPEED(&D_803EF630);
    u32 h;

    VS_SOUND_SPEED(&D_803EF630) = s;
    if (last != s && D_803EF6D8 != NULL)
        func_80260AB8(D_803EF6D8, 0x10, f2i(1.5f + (f32)s * -0.004f));
    h = (u16)D_80364452 + ANGLE_HALF;
    if ((s32)h >= ANGLE_TURN)
        h -= ANGLE_WRAP;
    func_802A0360(2, h / 0x555 < 2 ? h / 0x555 : 2, P, (f32)(s32)(h % 0x555) / 1365.0f);
    s = D_802E8BD0 != 0 ? 0 : VS_SPEED(vs);
    func_802A039C(1, engine_cvt_w_s((f32)s * CMO_SHAFT_SPIN), P);
}

/* the level is won when it is D_803EF6F0 from where it started */
void func_802BA91C(void) {
    if (dist() >= D_803EF6F0) {
        D_803643DA = 1;
        D_802E8BD8 = 1;
    }
}

/* Its collisions: the level lost when it hits something (the parts'
   collision once it is D_803EF6E8 on, the buildings' once D_803EF6EC on)
   or a wheel's height (s7: VS_WHEEL_H) changes by CMO_UNEVEN or more in a
   frame; once over, its smoke; and the countdown's effect. */
REGS(gp, s7)
void func_802BA9A0(VS *vs, s32 *s7) {
    s32 lost = 0, w;

    if (D_803643D6 == 0) {
        func_8029A800(D_803EF6DC, D_803EF6E0, D_803EF6E4, D_80305D60, 0, 0, 0, vs->unk76, 0, 0, 0xFF, vs);
        func_8029C52C(VEHICLE_CMO, vs);
        if (D_803A7426 != 0) {
            lost = 1;
        } else {
            D_803A7424 = 0;
            if (D_803EF710 != 0)
                func_8029AA10();
            if (D_803EF711 != 0) {
                D_803F77D0 = P;
                func_802BE77C(VEHICLE_CMO, vs);
            }
            if (D_803A7424 != 0)
                lost = 1;
            for (w = 0; w < 9 && !lost; w += 3)
                if (iabs(s7[w + 1] - s7[w]) >= CMO_UNEVEN)
                    lost = 1;
        }
        if (!lost)
            return;
        D_803643D9 = 1;
        D_802E8BD8 = 1;
    }
    if (D_803643D8 == 0) {
        func_80278318();
        func_802A02E4(1, P);
        func_802A5E60();
        func_802C049C();
    }
    if (D_803EF6FE != 0) {
        D_803EF6FE--;
    } else if (D_803EF6FF == 0) {
        if (D_803EF701 == 0) {
            func_802A6274(D_802C3B44, 0x7A120, 1, VEHICLE_CMO, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
            D_803EF701 = 1;
        } else if (++D_803EF700 == CMO_COUNTDOWN_FRAMES) {
            D_803EF6FF = 1;
        }
    }
}

/* its matrix, its vertices and its collision */
REGS(gp)
void func_802BABEC(VS *vs) {
    u8 *model = CMO_MODEL, *buf = FRAME_BUF(CMO_BUF0, CMO_BUF1);

    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(CMO_X, CMO_Y, CMO_Z, CMO_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_8029C454(CMO_X, CMO_Y, CMO_Z, VEHICLE_CMO, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_CMO, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the physics' settings for the carrier: gravity, and how its wheels land */
void func_802BAD24(void) {
    D_803EBBF4 = D_803EBBF0 * CMO_GRAVITY;
    D_803ED3F6 = CMO_BOUNCE_MIN;
    D_803ED3F7 = CMO_BOUNCE_DIV;
}

/* ---- the crane --------------------------------------------------------------- */

#define CRANE_SCALE 0x1B58              /* its model's scale */
#define CRANE_ARM_TOP 0.9f              /* its arm (part 5) goes up to here ... */
#define CRANE_ARM_BOTTOM 0.1f           /* ... and down to here */
#define CRANE_LEVEL 0x11                /* the level its hook picks things up on */

/* the crane's set up: from the level loader, with the model file, the
   position and the heading */
REGS(s2, t7, s3, s0, s1)
void func_802BAD80(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EFA20;
    s32 avg;

    CR_MODEL = model;
    CR_BUF0 = D_80358070;
    CR_BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(VEHICLE_CRANE, 0, CR_BUF0, CR_BUF1, model);
    func_802A754C(vs);
    SET_WHEELS(VS_WHEELS(vs), 0x50, 0x50, -0x50, 0x50, 0x50, -0x50);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x5A, 0x5A, -0x5A, 0x5A, 0x5A, -0x5A);
    CR_X = x;
    CR_Y = y;
    CR_Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), CR_Y, x, z, VS_WHEEL_H(vs), &CR_Y, (s16 *)&VS_HEADING(vs), VEHICLE_CRANE, vs, 0,
                  &avg);
    func_8029F85C(Q, CR_MODEL, CR_BUF0, CR_BUF1);
    func_802A039C(0, 100, Q);
    func_802A03D4(0, 0, Q);
    func_802A040C(0, 0, Q);
    func_802A0480(0, 0, Q, 0.0f);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, CR_BUF0, CR_BUF1);
    func_802A0320(0, Q);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, CR_BUF1, CR_BUF0);
    CRANE_GRABBING(vs) = 0;
    CRANE_HIT(vs) = 0;
    func_8029C354(VEHICLE_CRANE, MODEL_AT(CR_MODEL, 4), MODEL_AT(CR_MODEL, 8), CRANE_SCALE);
    func_80258230(VEHICLE_CRANE, 0x96, 0x2D, 0x2D);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802BB274();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(CR_BUF1, CR_BUF0, MODEL_MTX_OFF(CR_MODEL));
}

/* hd.c's: getting in: the camera's, and the crane's arm parts' start */
void func_802BB054(void) {
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

/* hd.c's: whether the crane can be left: not while its grab moves */
u8 func_802BB170(void) {
    return CRANE_GRABBING(&D_803EFA20) == 0;
}

/* hd.c's: getting out: this frame's buffer to the other one, and its sounds
   off */
void func_802BB1A0(void) {
    func_802A7764((u32 *)CR_BUF0, (u32 *)CR_BUF1, 0x800);
    if (D_803EFADC != NULL)
        func_802608C8(D_803EFADC);
    if (D_803EFAD8 != NULL)
        func_802608C8(D_803EFAD8);
    if (D_803EFAE0 != NULL)
        func_802608C8(D_803EFAE0);
}

/* the crane's light */
void func_802BB230(void) {
    func_802ABD54(VEHICLE_CRANE, CR_X, CR_Y, CR_Z);
}

/* the crane, each frame: its controls, what stands on it, its matrix and
   its collision (CRANE_HIT when it hits something) */
void func_802BB274(void) {
    VS *vs = &D_803EFA20;

    func_802BB230();
    if (VS_IN_SETUP(vs) == 0)
        func_802BB4C0(vs);
    func_802A92C8(CR_X, CR_Z, VS_CARRY_WHEELS(vs), (s16 *)&VS_HEADING(vs), VS_WHEEL_H(vs), VEHICLE_CRANE, vs, 0);
    func_8029E558(Q, FRAME_BUF(CR_BUF0, CR_BUF1), OTHER_BUF(CR_BUF0, CR_BUF1));
    func_802BB8B8(vs);
    CRANE_HIT(vs) = 0;
    func_8029A800(D_803EFAC8, D_803EFACC, D_803EFAD0, D_80305DF0, 0, 0, 0, 0, 0, 0, 6, vs);
    func_8029C52C(VEHICLE_CRANE, vs);
    func_8029C5EC();
    D_803F77D0 = Q;
    func_802BE77C(VEHICLE_CRANE, vs);
    func_8029AA10();
    if (D_803A7424 != 0)
        CRANE_HIT(vs) = 1;
    D_803643E0 = CR_X;
    D_803643E4 = CR_Y;
    D_803643E8 = CR_Z;
    D_8036443C = 0;
    D_8036443E = VS_MOVE_HEADING(vs);
    D_80364440 = VS_HEADING(vs);
}

/* a sound on (into *h) if it isn't */
static void sound_on(SndState **h, s16 id) {
    if (*h == NULL)
        func_80260650(D_80367738, id, h);
}

/* The crane's controls a frame: its arm (part 5: A up to CRANE_ARM_TOP, B
   down to CRANE_ARM_BOTTOM), its turn (part 4: PAD_LEFT, PAD_RIGHT) and
   its grab (part 2: L or R, or by itself over something to pick up), with
   their sounds; having hit something, everything stops and lets go. */
REGS(gp)
void func_802BB4C0(VS *vs) {
    s32 t1;
    f32 f0;

    if (CRANE_HIT(vs) != 0) {
        func_802A039C(5, 0, Q);
        func_802A039C(4, 0, Q);
        func_802A03D4(2, 0, Q);
        func_802A0290(2, 1, Q);
        CRANE_GRABBING(vs) = 1;
    } else {
        /* the arm */
        if (PAD_A != 0 && (part(5, Q, NULL, &f0), f0 <= CRANE_ARM_TOP)) {
            func_802A03D4(5, 0, Q);
            func_802A039C(5, 1, Q);
            sound_on(&D_803EFAD8, 0x6F);
        } else if (PAD_B != 0 && (part(5, Q, NULL, &f0), !(f0 < CRANE_ARM_BOTTOM))) {
            func_802A03D4(5, 1, Q);
            func_802A039C(5, 1, Q);
            sound_on(&D_803EFAD8, 0x6F);
        } else {
            func_802A039C(5, 0, Q);
            if (D_803EFAD8 != NULL)
                func_802608C8(D_803EFAD8);
        }
        /* the turn */
        if (PAD_LEFT != 0 || PAD_RIGHT != 0) {
            func_802A03D4(4, PAD_LEFT != 0 ? 0 : 1, Q);
            func_802A039C(4, 1, Q);
            sound_on(&D_803EFADC, 0x6E);
        } else {
            func_802A039C(4, 0, Q);
            if (D_803EFADC != NULL)
                func_802608C8(D_803EFADC);
        }
        /* the grab */
        if (func_802BB868() != 0) {
            /* something to pick up under it: the grab closes, unless it is
               closed (frame 1 at its end) */
            part(2, Q, &t1, &f0);
            if (!(t1 == 1 && engine_cvt_w_s(f0 * 100.0f) == 100)) {
                func_802A03D4(2, 0, Q);
                func_802A0290(2, 1, Q);
                CRANE_GRABBING(vs) = 1;
            }
        } else if (CRANE_GRABBING(vs) == 0 && (PAD_L != 0 || PAD_R != 0)) {
            sound_on(&D_803EFAE0, 0x6D);
            func_802A0290(2, 1, Q);
            CRANE_GRABBING(vs) = 1;
            return;
        }
    }
    /* the grab still moving, or done: its sound off */
    if (part(2, Q, NULL, NULL) == 1)
        return;
    CRANE_GRABBING(vs) = 0;
    if (D_803EFAE0 != NULL)
        func_802608C8(D_803EFAE0);
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

/* on CRANE_LEVEL, whether there is water under the crane's hook to pick
   things out of (func_8029C6E4 finds the hook's position) */
REGS(-> v0)
s32 func_802BB868(void) {
    s32 *p;

    if (D_802E8BDC != CRANE_LEVEL || func_8029C6E4() == 0)
        return 0;
    p = record_3bd();
    return func_802AC0BC(p[0], p[2], p[1]);
}

/* the crane's matrix, its vertices, its arm's and its collision */
REGS(gp)
void func_802BB8B8(VS *vs) {
    u8 *model = CR_MODEL, *buf = FRAME_BUF(CR_BUF0, CR_BUF1);
    s32 *m = (s32 *)(buf + MODEL_MTX_OFF(model));

    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(CR_X, CR_Y, CR_Z, CRANE_SCALE, m);
    func_8029C454(CR_X, CR_Y, CR_Z, VEHICLE_CRANE, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_CRANE, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
    func_802AABE4(VEHICLE_CRANE, (u16 *)MODEL_AT(model, 8), buf, 0, 0);
    func_8029D040(CR_X, CR_Z, VEHICLE_CRANE, MODEL_AT(model, 0xC), VS_HEADING(vs), Q, buf);
}
