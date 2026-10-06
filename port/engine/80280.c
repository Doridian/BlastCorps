/*
 * hd_code 80280 (us.v11 0x802C4A40-0x802C80D0): the level's status as the
 * Controller Pak keeps it, and the J-Bomb (VEHICLE_JETPACK), as native C
 * (engine.h, vehicle.h).
 *
 * The status (func_802C4A40, func_802C4BF0, func_802C4E58) is a string of
 * bits: one per damage group of each building (destroyed or not), then one
 * per RDU (collected or not), eight to a byte, the first in the top bit.
 *
 * The J-Bomb's parts are D_803F7850, its state D_803F7B50 and its position
 * D_803F7BF8..C00.  func_802C5120 sets it up (from the level loader),
 * func_802C5AFC runs it each frame (from hd.c and at the end of the setup);
 * the others are hd.c's hooks for it, and its two callbacks for 62740's
 * carrying (shared.h).  It walks or flies, its mode in JB_MODE (below):
 * walking, flying (its jets lifting it while the boost buttons are held),
 * falling, dropping onto something below it, slamming down (B, or the
 * boost buttons pressed three times), landed.
 */
#include "vehicle.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

extern u8 D_803063F0[];                         /* the share of the groups a medal wants, by medal */
extern s32 D_80368040;                          /* the level's groups to destroy */

u8 func_8026FE6C(s32 i);
void func_8026FE8C(s32 i);

#define T(p) ((s32)(p))

/* ---- the level's status ------------------------------------------------- */

/* bits written eight to a byte, the first in the top bit */
typedef struct BitOut {
    u8 *out;
    u32 bits;
    s32 n;
} BitOut;

static void put_bit(BitOut *o, u32 v) {
    o->bits = o->bits << 1 | v;
    if (++o->n == 8) {
        *o->out++ = o->bits;
        o->bits = 0;
        o->n = 0;
    }
}

/* the last byte's bits, if any, at its top */
static void flush_bits(BitOut *o) {
    if (o->n != 0)
        *o->out++ = o->bits << (8 - o->n);
    o->bits = 0;
    o->n = 0;
}

/* and read back */
typedef struct BitIn {
    u8 *in;
    u32 byte, mask;
} BitIn;

static u32 get_bit(BitIn *i) {
    if (i->mask == 1) {
        i->byte = *i->in++;
        i->mask = 0x100;
    }
    i->mask >>= 1;
    return i->byte & i->mask;
}

/* hd.c's: the status into buf; its length */
s32 func_802C4A40(u8 *buf) {
    BitOut o = { buf, 0, 0 };
    Building *b;
    s32 k, i;

    for (b = D_803F4030; b != D_803F7654; b++)
        for (k = 0; k < B_NGROUPS(b); k++)
            put_bit(&o, B_DAMAGE(b)[k] == 100);
    flush_bits(&o);
    for (i = 0; i < D_8036EB90; i++)
        put_bit(&o, func_8026FE6C(i));
    flush_bits(&o);
    return o.out - buf;
}

/* (409D0.c's and the level loader's) the status from buf: the destroyed
   groups at 100 (and their triangles gone, func_802BF1F0), a building
   whose groups are all destroyed (but the main one) flagged destroyed, the
   groups to destroy counted (D_80368040), the pieces brought back by a
   group (group2) or switched off with theirs, and the RDUs collected */
void func_802C4BF0(u8 *buf) {
    BitIn in = { buf, 0, 1 };
    Building *b;
    Piece *q;
    s32 g, all, any, total = 0, i, n;

    for (b = D_803F4030; b != D_803F7654; b++) {
        all = 1;
        any = 0;
        for (g = 1; g <= B_NGROUPS(b); g++) {
            if (get_bit(&in)) {
                func_802BF1F0(g, b);
                any = 1;
                B_DAMAGE(b)[g - 1] = 100;
            } else {
                if (M_MAIN(B_MODEL(b)) != g)
                    all = 0;
                B_DAMAGE(b)[g - 1] = 0;
            }
        }
        B_DESTROYED(b) = all;
        if (any)
            total += M_UNK6(B_MODEL(b));
    }
    D_80368040 = total;
    for (b = D_803F4030; b != D_803F7654; b++) {
        for (g = 1; g <= B_NGROUPS(b); g++) {
            if (B_DAMAGE(b)[g - 1] != 100)
                continue;
            for (q = b->unk4; q != (Piece *)b->unk8; q++) {
                n = q->group;
                if (n == g)
                    q->active = 0;
                if (q->group2 == g && B_DAMAGE(b)[n - 1] != 100)
                    q->active = 1;
            }
        }
    }
    in.mask = 1;
    for (i = 0; i < D_8036EB90; i++)
        if (get_bit(&in))
            func_8026FE8C(i);
}

/* (409D0.c's) a status for the medal `medal` (1..4) into buf: of the
   buildings that count (not the goal, not of strength 1), the targets and
   then as many more as the medal's share wants destroyed, then that share
   of the RDUs collected; its length */
s32 func_802C4E58(u8 *buf, u8 medal) {
    BitOut o = { buf, 0, 0 };
    Building *b;
    u32 want;
    s32 k, count = 0, targets = 0, bit, i;

    medal--;
    for (b = D_803F4030; b != D_803F7654; b++)
        if (B_ID(b) != MODEL_GOAL && M_STRENGTH(B_MODEL(b)) != 1)
            count++;
    for (b = D_803F4030; b != D_803F7654; b++)
        if (B_TARGET(b))
            targets++;
    want = (u32)D_803063F0[medal] * (u32)count / 100u + 1;
    if (medal == 4) {
        want = 0;
    } else {
        want -= targets;
        if ((s32)want < 0)
            want = 0;
    }
    for (b = D_803F4030; b != D_803F7654; b++) {
        if (B_ID(b) == MODEL_GOAL || M_STRENGTH(B_MODEL(b)) == 1) {
            bit = 0;
        } else if (B_TARGET(b)) {
            bit = 1;
        } else if (want != 0) {
            want--;
            bit = 1;
        } else {
            bit = 0;
        }
        for (k = 0; k < B_NGROUPS(b); k++)
            put_bit(&o, bit);
    }
    flush_bits(&o);
    want = (u32)D_803063F0[medal] * (u32)D_8036EB90 / 100u;
    for (i = 0; i < D_8036EB90; i++) {
        put_bit(&o, want != 0);
        if (want != 0)
            want--;
    }
    flush_bits(&o);
    return o.out - buf;
}

/* ---- the J-Bomb ---------------------------------------------------------- */

extern Part D_803F7850[32];
extern s32 D_803F7BF8, D_803F7BFC, D_803F7C00;  /* x, y, z */
extern u8 *PTR32 D_803F7C04;                    /* its model file */
extern u8 *PTR32 D_803F7C08;                    /* two 0x1000-byte buffers, one per frame */
extern u8 *PTR32 D_803F7C0C;
extern SndState *PTR32 D_803F7C18;              /* its jets' sound, while they play */
extern SndState *PTR32 D_803F7C1C;              /* its flight's */
extern s32 D_803F7C20;                          /* frames since it was last dropping (mode 3) */
extern s32 D_803F7C24;                          /* frames since it was last slamming (mode 4) */
extern f32 D_803F7C28, D_803F7C2C;              /* parts 3's and 4's tilt, 0..1 */
extern u16 D_803F7C30;                          /* the heading it turns to against a wall (D_803A7425) */
extern s16 D_803F7C32;                          /* part 6's frame, last time */
extern s8 D_803F7C36;                           /* turning to D_803F7C30 */
extern u8 D_803F7C37;                           /* no throttle while set */
extern u8 D_803F7C38;                           /* frames without the throttle */
extern u8 D_803F7C39;                           /* hit something this frame */
extern u8 D_803F7C3A;                           /* frames until the jets' next sparks */
extern s8 D_803F7C3B;                           /* the jets' flames' size, 0..50 */
extern u8 D_803F7C3C;                           /* the steering rate in the air */
extern u8 D_803F7C3D;                           /* frames the jets go on firing */
extern u8 D_803F7C3E;                           /* the jets fire this frame */
extern u8 D_803F7C40;                           /* the slam's count */
extern u8 D_803F7C41;                           /* frames in the air, counting down */
extern u8 D_803F7C42, D_803F7C43;               /* last frame's B and on-the-ground (VS_AIRBORNE[0]) */
extern u8 D_803F7C44, D_803F7C45;               /* L's presses counted, and the frames left for the next */
extern u8 D_803F7C46, D_803F7C47;               /* R's */
extern u8 D_803F7C48;                           /* its brake (func_802C7F28) */
extern u8 D_803F7C49;                           /* a ceiling just above where it goes */
extern u8 D_803F7C4A;                           /* the slam's push still to come */
extern u8 D_803F7C4B;                           /* frames standing still before it lands */
extern s32 D_803F7844;

#define JB D_803F7850
#define X D_803F7BF8
#define Y D_803F7BFC
#define Z D_803F7C00
#define MODEL D_803F7C04
#define BUF0 D_803F7C08
#define BUF1 D_803F7C0C
/* the heights it flies between (1D990.c, from the level's info): above
   JB_LOW its jets lift less, to nothing at JB_HIGH */
#define JB_LOW D_803F7C10
#define JB_HIGH D_803F7C14

/* VehicleState's bytes the J-Bomb uses for itself */
#define JB_MODE(vs) ((vs)->unkA1)       /* JB_* below */
#define JB_LAST_MODE(vs) ((vs)->unkA2)  /* last frame's */
#define JB_WALK 0
#define JB_FLY 1
#define JB_FALL 2
#define JB_DROP 3                       /* onto something below it */
#define JB_SLAM 4
#define JB_LANDED 5

extern u8 D_80306400[];                         /* its parts' collision (56040's func_8029A800) */
extern u8 D_802C28E4[];                         /* its landing's effect record (60F60) */
extern u8 D_802C3804[];                         /* its jets' */
extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s32 D_803EBBFC;                          /* the ceiling's height above it (func_802AC0BC) */
extern u8 D_803A742B;
extern u8 D_80370C35;                           /* 45BB0.c: the stick plays the buttons */
extern Part *PTR32 D_803F77D0;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
s32 func_802584BC(u8 type);
s32 func_80258500(u8 type);
s32 func_80288284(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
REGS(t0, t1, t2 -> a1)
s32 func_802AC0BC(s32 x, s32 z, s32 y);
REGS(v0, v1, a0)
void func_8029FC74(s32 a, s32 b, Part *parts);

void func_802C5AFC(void);
REGS()
void func_802C5970(void);
REGS(gp)
void func_802C617C(VS *vs);
REGS(gp)
void func_802C61F0(VS *vs);
REGS(gp)
void func_802C6DAC(VS *vs);
REGS(gp)
void func_802C6ECC(VS *vs);
REGS(gp -> s6)
s32 func_802C6FD8(VS *vs);
REGS(gp)
void func_802C70E8(VS *vs);
REGS(t0, t1)
void func_802C71FC(s32 x, s32 z);
REGS(gp)
void func_802C7354(VS *vs);
REGS()
void func_802C7410(void);
REGS(gp)
void func_802C7544(VS *vs);
REGS()
void func_802C770C(void);
REGS(gp)
void func_802C7864(VS *vs);
REGS(-> f4)
f32 func_802C7BC0(void);
REGS(s2, gp -> f2)
f32 func_802C7C1C(s32 up, VS *vs);
REGS(gp)
void func_802C7CB0(VS *vs);
REGS(gp -> s3)
s32 func_802C7DFC(VS *vs);
REGS(gp)
void func_802C7ECC(VS *vs);
REGS(gp)
void func_802C7F28(VS *vs);

/* ---- the J-Bomb's numbers (a frame, where it's per frame) ---------------- */

#define JBOMB_SCALE 0x4268              /* its model's scale */
#define JBOMB_SPAN 0x78                 /* its feet's spans, both ways (func_802A8768) */
#define JBOMB_WALL_TURN 0.25f           /* func_802A71DC: turning along a wall, times the speed */
#define JBOMB_SLOPE_DIV 120.0f          /* func_802A843C: the slope's push is the height difference over this */
#define JBOMB_STEER_DIV 1.5f            /* walking: the steering rate is the speed over this */
#define JBOMB_AIR_STEER 0x14            /* in the air: the steering rate with the stick near the middle ... */
#define JBOMB_AIR_STEER_UP 2            /* ... and its rise a frame while it is pushed sideways ... */
#define JBOMB_AIR_STEER_MAX 100         /* ... to this */
#define JBOMB_AIR_STICK 0x32            /* (the middle: within this) */
#define JBOMB_AIR_DRAG 8                /* in the air, the speed's fall a frame (func_802C617C) */
#define JBOMB_AIR_FRAMES 9              /* D_803F7C41: set on the ground, counts down in the air */
#define JBOMB_MODE_GRACE 3              /* frames after dropping or slamming that still count as it */
#define JBOMB_JET_LIFT 0x1E             /* the jets' push off the ground (jump()) */
#define JBOMB_JET_TOP_PUSH 0x1E         /* the push the jets add, less toward JB_HIGH (func_802C6FD8) */
#define JBOMB_CLIMB_FRAMES 0xA          /* D_803F7C3D: frames the jets go on firing after a drop or slam */
#define JBOMB_CEILING_NEAR 0xC8         /* a ceiling this near above: pushed down with the jets' lift */
#define JBOMB_CEILING_FAR 0xBB8         /* this near: falling */
#define JBOMB_WRECK_HEIGHT 0x9C4        /* at its top damage, falling once this above its shadow */
#define JBOMB_SLAM_FRAMES 0x14          /* D_803F7C40: the slam's count */
#define JBOMB_SLAM_PUSH -0x4B0          /* the slam's push down, once its part 5 is done */
#define JBOMB_SLAM_SHAKE_FRAMES 0x14    /* the screen's shake as a slam lands (00000.c) */
#define JBOMB_SLAM_SHAKE 0x320
#define JBOMB_SLAM_PRESSES 3            /* the boost buttons pressed this often ... */
#define JBOMB_PRESS_FRAMES 4            /* ... each within this many frames of the last: a slam */
#define JBOMB_LAND_FRAMES 0x1E          /* standing still this long: landed (D_803F7C4B) */
#define JBOMB_LAND_MAX_SPEED 0x96       /* back on the ground: its speed within -0x64..this */
#define JBOMB_LAND_MIN_SPEED -0x64
#define JBOMB_BOUNCE_SPEED 0x14         /* a bounce in flight keeps the speed within this */
#define JBOMB_BOUNCE_LIFT 0x14A         /* and pushes it up this much */
#define JBOMB_FLAMES_MAX 0x32           /* the jets' flames grow a frame to this, and shrink back */
#define JBOMB_SPARK_WAIT 1              /* frames between the jets' sparks (4 landed) */
#define JBOMB_TILT_RATE 0.02f           /* part 4's tilt a frame with the stick's x ... */
#define JBOMB_TILT_BACK 0.01f           /* ... and back to the middle */
#define JBOMB_PITCH_RATE 0.03f          /* part 3's a frame with its y (times how far from the stops) ... */
#define JBOMB_PITCH_BACK 0.005f         /* ... and back */
#define JBOMB_TILT_STICK 0x1E           /* the stick's x past this tilts it */
#define JBOMB_TILT_SPEED 270.0f         /* part 4's limit: half the speed over this either way */

/* part i's state: func_802A04BC's first result (and its sixth, the
   frame) */
static s32 part_state(s32 i, Part *parts, s32 *f13) {
    s32 f11, f12, f14, fC, fE, t1;
    f32 f4;
    s32 r = func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &t1, &f4);

    if (f13)
        *f13 = t1;
    return r;
}

/* its jets' push: the wheels' fall step `speed`, from where it is, and
   their frames to 1 (62740's wheels in the air, VS_WHEEL_FALL) */
static void jump(VS *vs, s32 speed) {
    s32 *f = VS_WHEEL_FALL(vs);

    f[6] = 1, f[7] = 1, f[8] = 1;
    f[0] = speed, f[1] = speed, f[2] = speed;
    f[3] = Y, f[4] = Y, f[5] = Y;
}

/* all three in the air */
static void airborne(VS *vs) {
    VS_AIRBORNE(vs)[0] = 1, VS_AIRBORNE(vs)[1] = 1, VS_AIRBORNE(vs)[2] = 1;
}

/* a part's animation set: speed, direction, its third setting and its
   play (func_802A0290's second argument) */
static void part_anim(s32 i, s32 speed, s32 dir, s32 c, s32 play) {
    func_802A039C(i, speed, JB);
    func_802A03D4(i, dir, JB);
    func_802A040C(i, c, JB);
    func_802A0290(i, play, JB);
}

static void jbomb_frame(void);

/* set up: from the level loader, with the model file, the position and
   the heading */
REGS(s2, t7, s3, s0, s1)
void func_802C5120(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F7B50;
    s32 avg;

    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x1000;
    D_80358070 += 0x2000;
    func_802A1388(VEHICLE_JETPACK, 0, BUF0, BUF1, model);
    func_802A754C(vs);
    /* one point (its feet), and three for what carries it */
    SET_WHEELS(VS_WHEELS(vs), 0, 0, 0, 0, 0, 0);
    SET_WHEELS(VS_CARRY_WHEELS(vs), 0x190, 0x190, -0x190, 0x190, 0x190, -0x190);
    X = x;
    Y = y;
    Z = z;
    VS_HEADING(vs) = heading;
    VS_MOVE_HEADING(vs) = heading;
    VS_TURN_HEADING(vs) = heading;
    func_802A992C(VS_WHEELS(vs), Y, x, z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_JETPACK, vs, 0, &avg);
    func_8029F85C(JB, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, JB);
    func_802A03D4(0, 0, JB);
    func_802A040C(0, 0, JB);
    func_802A0480(0, 0, JB, 0.0f);
    func_802A0290(0, 1, JB);
    func_8029E558(JB, BUF0, BUF1);
    func_802A0320(0, JB);
    func_802A0290(0, 1, JB);
    func_8029E558(JB, BUF1, BUF0);
    D_803F7C36 = 0;
    D_803F7C37 = 0;
    D_803F7C39 = 0;
    D_803F7C3D = 0;
    D_803F7C38 = 0;
    D_803F7C45 = 0;
    D_803F7C47 = 0;
    D_803F7C4A = 0;
    D_803F7C3A = 0;
    D_803F7C3B = 0;
    D_803F7C28 = 0.5f;
    D_803F7C2C = 0.5f;
    JB_MODE(vs) = JB_WALK;
    JB_LAST_MODE(vs) = JB_WALK;
    D_803F7C3C = JBOMB_AIR_STEER;
    D_803F7C43 = 0;
    D_803F7C4B = JBOMB_LAND_FRAMES;
    D_803F7C20 = 999999;
    D_803F7C24 = 999999;
    D_803F7C40 = 0;
    D_803F7C41 = 0;
    D_803F7C34 = 0;
    D_803F7C42 = 0;
    func_8029C354(VEHICLE_JETPACK, MODEL_AT(MODEL, 4), MODEL_AT(MODEL, 8), JBOMB_SCALE);
    func_80258230(VEHICLE_JETPACK, 0x64, 0x2D, 0x2D);
    func_802A0360(6, 2, JB, 0.0f);
    func_802A0290(6, -1, JB);
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    jbomb_frame();
    VS_IN_SETUP(vs) = 0;
    func_802A7764((u32 *)BUF1, (u32 *)BUF0, 0x1000);
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
    D_803F7844 = 0;
}

/* hd.c's: whether it can be left: not with a foot in the air (0); else 1
   when it stands still walking (parts 0x1F and 9 at rest), else 2; landed,
   it is put back walking first */
u8 func_802C5508(void) {
    VS *vs = &D_803F7B50;

    if (ANY_AIRBORNE(vs))
        return 0;
    if (JB_MODE(vs) == JB_LANDED) {
        JB_MODE(vs) = JB_WALK;
        func_802A02E4(7, JB);
        func_802A02E4(8, JB);
        func_802A0360(6, 2, JB, 0.0f);
        func_802A0360(9, 0, JB, 0.0f);
        func_802A039C(9, 3, JB);
        func_802A03D4(9, 0, JB);
        func_802A0480(9, 0, JB, 0.0f);
        func_802A040C(9, 1, JB);
        func_802A0290(9, 1, JB);
        return 2;
    }
    if (JB_MODE(vs) != JB_WALK || part_state(0x1F, JB, NULL) == 1)
        return 2;
    return part_state(9, JB, NULL) == 0 ? 1 : 2;
}

/* hd.c's: the player gets out: the sounds off */
void func_802C5688(void) {
    VS_SPEED(&D_803F7B50) = 0;
    func_802A7764((u32 *)BUF0, (u32 *)BUF1, 0x1000);
    func_802A02E4(0x1F, JB);
    if (D_803F7C18 != NULL)
        func_802608C8(D_803F7C18);
    if (D_803F7C1C != NULL)
        func_802608C8(D_803F7C1C);
}

/* hd.c's (and 17210.c's): the player gets in: its jets' tilts and flames
   set */
void func_802C5714(void) {
    VS_TURNING(&D_803F7B50) = 0;
    func_802A039C(3, 0, JB);
    func_802A03D4(3, 0, JB);
    func_802A040C(3, 1, JB);
    func_802A0480(3, 1, JB, 1.0f);
    func_802A0290(3, -1, JB);
    func_802A039C(4, 0, JB);
    func_802A03D4(4, 0, JB);
    func_802A0480(4, 1, JB, 1.0f);
    func_802A040C(4, 1, JB);
    func_802A0290(4, -1, JB);
    part_anim(1, 0, 0, 0, -1);
}

/* hd.c's: put back on the ground where it is */
void func_802C5860(void) {
    VS *vs = &D_803F7B50;

    func_802A9A60(VS_WHEELS(vs), Y, X, Z, VS_WHEEL_H(vs), &Y, (s16 *)&VS_HEADING(vs), VEHICLE_JETPACK, vs, 0);
    func_802C7CB0(vs);
    func_802A133C(X, Y, Z, VEHICLE_JETPACK, vs);
}

/* its light */
REGS()
void func_802C5970(void) {
    func_802ABD54(VEHICLE_JETPACK, X, Y, Z);
}

/* 62740's carrying (shared.h): where it stands on its carrier (its point
   only) */
REGS(a3)
void func_802C59B4(s32 carrier) {
    VS *vs = &D_803F7B50;
    s32 cz;

    VS_CARRY_X(vs) = func_802AAD0C(carrier, X, Z, &cz);
    VS_CARRY_Z(vs) = cz;
}

/* and back there after the carrier moved, its heading kept */
REGS(a3)
void func_802C5A14(s32 carrier) {
    VS *vs = &D_803F7B50;
    s32 x, z;

    x = func_802AAE54(carrier, VS_CARRY_X(vs), VS_CARRY_Z(vs), &z);
    func_802C7ECC(vs);
    func_802C7F28(vs);
    D_803ED40B = 0;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_JETPACK, JBOMB_SPAN, JBOMB_SPAN, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_802C7CB0(vs);
    func_802A133C(X, Y, Z, VEHICLE_JETPACK, vs);
}

/* each frame: its modes and jets, the steering, the throttle (unless held
   off), on the ground the slope or in the air the drag, the move; what it
   hits: falling when flying into something, turned along a wall */
static void jbomb_frame(void) {
    VS *vs = &D_803F7B50;
    s32 step = 0, x, z, rate_i, sel, mode;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    func_802C5970();
    func_802C7ECC(vs);
    if (VS_IN_SETUP(vs) == 0) {
        func_802C61F0(vs);
        func_802C7410();
    }
    func_802C7F28(vs);
    rate_i = func_802C7DFC(vs);
    func_802A7E70(rate_i, &VS_HEADING(vs), &stick_addr, &stick);
    if (D_803F7C37 == 0) {
        if (D_803F7C38 == 0)
            func_802A785C(step, &VS_SPEED(vs), 4, VS_AIRBORNE(vs), VS_GEARS(vs), D_803F7C48, vs, &step);
        else
            D_803F7C38--;
    }
    if (JB_MODE(vs) == JB_WALK)
        func_802A77D0(vs);
    else
        func_802C617C(vs);
    VS_MOVE_HEADING(vs) = VS_HEADING(vs);
    if (VS_AIRBORNE(vs)[0] != 1)
        rate = func_802A83B8(step, &VS_SPEED(vs), VS_AIRBORNE(vs), VS_WHEEL_H(vs), &VS_SLOPE_RATIO(vs), &step);
    else
        rate = 0.0f;
    if ((s8)D_803F7C37 == 0) {
        if (JB_MODE(vs) == JB_WALK)
            func_802A843C(&VS_SPEED(vs), 1, VEHICLE_JETPACK, (s8 *)VS_AIRBORNE(vs), VS_WHEEL_H(vs), JBOMB_SLOPE_DIV,
                          vs);
        else
            VS_SPEED(vs) = step_toward(VS_SPEED(vs), 0, 1);     /* (in the air, a unit a frame more) */
    }
    if (D_803F7C36 != 0)
        func_802A7070((s16 *)&D_803F7C30, vs);
    /* the move */
    x = func_802A860C(VS_MOVE_HEADING(vs), &VS_SPEED(vs), &X, &Z, rate, &z);
    func_802C71FC(x, z);
    D_803ED40B = 0;
    func_802A8768(x, z, &X, &Z, &Y, VEHICLE_JETPACK, JBOMB_SPAN, JBOMB_SPAN, VS_WHEELS(vs), VS_WHEEL_FALL(vs),
                  VS_WHEEL_FRAMES(vs), VS_WHEEL_GROUND(vs), VS_CARRY_WHEELS(vs), vs);
    func_8029E558(JB, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802C7CB0(vs);
    /* what it hits (not landed): func_8029A800's mode by its own */
    D_803F7C39 = 0;
    mode = JB_MODE(vs);
    if (mode == JB_LANDED)
        goto still;
    if (mode == JB_FLY || mode == JB_FALL || mode == JB_DROP)
        sel = 1;
    else if (mode == JB_SLAM || D_803F7C24 < JBOMB_MODE_GRACE)
        sel = 2;
    else
        sel = 0;
    func_8029A800(X, Y, Z, D_80306400, 1, 0, 0, VS_SPEED(vs), 0, sel, VEHICLE_JETPACK, vs);
    func_8029C52C(VEHICLE_JETPACK, vs);
    func_8029AA10();
    if (D_803A742B != 0 && JB_MODE(vs) != JB_WALK && JB_MODE(vs) != JB_LANDED) {
        JB_MODE(vs) = JB_FALL;
        D_803F7C39 = 1;
    }
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = JB;
        func_802BE77C(VEHICLE_JETPACK, vs);
        if (D_803A7424 == 0 && vs->unk9C == 0)
            goto still;
        /* hit something: falling, unless walking (or just out of a drop
           or a slam) on its feet */
        mode = JB_MODE(vs);
        if (mode == JB_SLAM)
            goto still;
        if (mode == JB_FLY || mode == JB_FALL || mode == JB_DROP || D_803F7C20 < JBOMB_MODE_GRACE ||
            VS_AIRBORNE(vs)[0] == 1) {
            JB_MODE(vs) = JB_FALL;
            D_803F7C39 = 1;
            goto done;
        }
    }
    D_803F7C36 = 1;
    turn_along_wall(vs, &D_803F7C30, JBOMB_WALL_TURN);
    goto done;
still:
    D_803F7C36 = 0;
    D_803F7C37 = 0;
done:
    PLAYER_FROM(X, Y, Z, vs, VEHICLE_JETPACK);
    D_803F7C3F = JB_MODE(vs) != JB_WALK && JB_MODE(vs) != JB_LANDED;
}

/* hd.c's: the J-Bomb each frame */
void func_802C5AFC(void) {
    jbomb_frame();
}

/* in the air: its speed toward 0 by JBOMB_AIR_DRAG a frame, unless the
   throttle or D_80370C35 holds it */
REGS(gp)
void func_802C617C(VS *vs) {
    if (D_80370C35 == 0 && STICK_Y == 0)
        VS_SPEED(vs) = step_toward(VS_SPEED(vs), 0, JBOMB_AIR_DRAG);
}

/* ---- the J-Bomb's modes (func_802C61F0's second half) ---- */

/* landed: part 8 played once parts 7 and 8 are done */
static void jbomb_landed(Part *p) {
    if (part_state(7, p, NULL) == 1 || part_state(8, p, NULL) == 1)
        return;
    func_802A0360(8, 0, p, 0.0f);
    func_802A039C(8, 1, p);
    func_802A03D4(8, 0, p);
    func_802A0480(8, 1, p, 0.5f);
    func_802A040C(8, 1, p);
    func_802A0290(8, -1, p);
}

/* slamming down: once part 5 is done, the push down (JBOMB_SLAM_PUSH) */
static void jbomb_slamming(VS *vs, Part *p) {
    if (D_803F7C4A != 0 && part_state(5, p, NULL) != 1) {
        jump(vs, JBOMB_SLAM_PUSH);
        D_803F7C4A = 0;
    }
    D_803F7C3D = JBOMB_CLIMB_FRAMES;
}

/* walking: its legs with the speed (part 6), a footstep's sound on their
   frames 0 and 3 */
static void jbomb_walk(VS *vs, Part *p) {
    s32 s, last, f13;

    if (part_state(0x1F, p, NULL) == 1 || part_state(9, p, NULL) == 1)
        return;
    s = VS_SPEED(vs);
    func_802A03D4(6, s < 0 ? 1 : 0, p);
    func_802A039C(6, (u32)iabs(s) / 16, p);
    func_802A0480(6, 1, p, 0.5f);
    func_802A040C(6, 0, p);
    func_802A0290(6, -1, p);
    part_state(6, p, &f13);
    last = D_803F7C32;
    D_803F7C32 = f13;
    if (f13 != last) {
        if (f13 == 0)
            func_80260650(D_80367738, 0x5A, NULL);
        else if (f13 == 3)
            func_80260650(D_80367738, 0x5B, NULL);
    }
}

/* the slam: down hard */
static void jbomb_slam(VS *vs, Part *p) {
    s32 *f = VS_WHEEL_FALL(vs);

    D_803F7C45 = 0;
    D_803F7C44 = 0;
    D_803F7C47 = 0;
    D_803F7C46 = 0;
    D_803F7C40 = JBOMB_SLAM_FRAMES;
    D_803F7C4A = 1;
    JB_MODE(vs) = JB_SLAM;
    f[0] = 0, f[1] = 0;
    f[3] = Y, f[4] = Y, f[5] = Y;
    f[6] = 1, f[7] = 1, f[8] = 1;
    f[2] = 0;
    func_802A02E4(4, p);
    func_802A02E4(3, p);
    func_802A039C(5, 0xA, p);
    func_802A03D4(5, 0, p);
    func_802A040C(5, 1, p);
    func_802A0360(5, 0, p, 0.0f);
    func_802A0290(5, 1, p);
    func_80260650(D_80367738, 0x5C, NULL);
}

/* bounced: up again (JBOMB_BOUNCE_LIFT), its speed within
   JBOMB_BOUNCE_SPEED either way unless the bounce was off a building */
static void jbomb_bounce(VS *vs) {
    s32 v;

    if (vs->unk9C == 0) {
        v = VS_SPEED(vs);
        if (v > JBOMB_BOUNCE_SPEED)
            v = JBOMB_BOUNCE_SPEED;
        if (v < -JBOMB_BOUNCE_SPEED)
            v = -JBOMB_BOUNCE_SPEED;
        VS_SPEED(vs) = v;
    }
    jump(vs, JBOMB_BOUNCE_LIFT);
    airborne(vs);
}

/* flying: the legs and arms at rest, the tilt (func_802C7864); bounced, up
   again; else, the timers run out and not at its height limit (unk9F
   100), the slam when asked (func_802C770C's count, or B) */
static void jbomb_fly(VS *vs, Part *p) {
    func_802A02E4(0x1F, p);
    func_802A02E4(6, p);
    func_802A0290(4, -1, p);
    func_802A0290(3, -1, p);
    func_802C7864(vs);
    if (D_803F7C39 != 0) {
        jbomb_bounce(vs);
        return;
    }
    if (D_803F7C40 != 0 || D_803F7C41 != 0 || VS_TOP_MATERIAL(vs) == 0x64)
        return;
    func_802C770C();
    if (D_803F7C44 == JBOMB_SLAM_PRESSES || (D_803F7C42 == 0 && PAD_B != 0))
        jbomb_slam(vs, p);
}

/* falling or dropping: flying again with the boost buttons, falling */
static void jbomb_drop(VS *vs, Part *p, s32 mode) {
    if (mode != JB_DROP) {
        D_803F7C3D = JBOMB_CLIMB_FRAMES;
        if (PAD_A != 0 || PAD_L != 0 || PAD_R != 0) {
            jump(vs, func_802C6FD8(vs));
            D_803F7C3E = 1;
            JB_MODE(vs) = JB_FLY;
        }
    }
    jbomb_fly(vs, p);
}

/* back on its feet (from the air, not hit this frame): the jets off, its
   speed within JBOMB_LAND_MIN_SPEED..JBOMB_LAND_MAX_SPEED, walking */
static void jbomb_touch_down(VS *vs, Part *p) {
    s32 k;

    for (k = 0; k < 9; k++)
        VS_WHEEL_H(vs)[k] = Y;
    func_802A0360(6, 2, p, 0.0f);
    func_802A02E4(5, p);
    func_802A02E4(4, p);
    func_802A02E4(3, p);
    if (JB_LAST_MODE(vs) != JB_SLAM) {
        func_8029FC74(3, 4, p);
        func_8029F9D4(0x1E, 6, p);
        func_802A039C(0x1F, 0x32, p);
        func_802A03D4(0x1F, 0, p);
        func_802A040C(0x1F, 0, p);
        func_802A0290(0x1F, 1, p);
    }
    if (VS_SPEED(vs) > JBOMB_LAND_MAX_SPEED)
        VS_SPEED(vs) = JBOMB_LAND_MAX_SPEED;
    else if (VS_SPEED(vs) < JBOMB_LAND_MIN_SPEED)
        VS_SPEED(vs) = JBOMB_LAND_MIN_SPEED;
    JB_MODE(vs) = JB_WALK;
}

/* The modes, the jets and the slam, each frame before it moves: the
   timers; the jets fire below JB_HIGH, walking or flying, while a boost
   button is held (or still, D_803F7C3D); landed and moving again, walking;
   then dropping (unk9E), back on its feet or off them; the ceiling, the
   damage; the slam landing; and the mode's own work. */
REGS(gp)
void func_802C61F0(VS *vs) {
    Part *p = JB;
    s32 v, mode;

    mode = JB_MODE(vs);
    JB_LAST_MODE(vs) = mode;
    if (mode == JB_FLY || mode == JB_FALL || mode == JB_DROP) {
        if (D_803F7C41 != 0)
            D_803F7C41--;
    } else {
        D_803F7C41 = JBOMB_AIR_FRAMES;
    }
    v = D_803F7C40;
    if (v != 0)
        D_803F7C40 = --v;
    /* (this compares the slam's count with the modes' numbers, where the
       original means the mode, it seems; kept) */
    if (v == JB_FALL || v == JB_DROP)
        D_803F7C20 = 0;
    else
        D_803F7C20++;
    if (v == JB_SLAM)
        D_803F7C24 = 0;
    else
        D_803F7C24++;
    /* the jets */
    D_803F7C3E = 0;
    if (!(JB_HIGH < Y) && JB_MODE(vs) != JB_SLAM && JB_MODE(vs) != JB_FALL) {
        if (D_803F7C3D != 0) {
            D_803F7C3D--;
        } else if (PAD_A != 0 || PAD_L != 0 || PAD_R != 0) {
            if (VS_AIRBORNE(vs)[0] == 1) {
                jump(vs, func_802C6FD8(vs));
            } else {
                jump(vs, JBOMB_JET_LIFT);
                airborne(vs);
            }
            D_803F7C3E = 1;
        }
    }
    /* landed and moving again: walking */
    if (JB_MODE(vs) == JB_LANDED && (VS_SPEED(vs) != 0 || VS_AIRBORNE(vs)[0] != 0)) {
        JB_MODE(vs) = JB_WALK;
        func_802A02E4(7, p);
        func_802A02E4(8, p);
        func_802A0360(6, 2, p, 0.0f);
        func_802A0360(9, 0, p, 0.0f);
        func_802A039C(9, 3, p);
        func_802A03D4(9, 0, p);
        func_802A0480(9, 0, p, 0.0f);
        func_802A040C(9, 1, p);
        func_802A0290(9, 1, p);
    }
    if (vs->unk9E != 0) {
        JB_MODE(vs) = JB_DROP;
        D_803F7C39 = 1;
    } else if (D_803F7C43 == 1) {
        if (VS_AIRBORNE(vs)[0] == 0 && D_803F7C39 == 0)
            jbomb_touch_down(vs, p);
    } else if (VS_AIRBORNE(vs)[0] == 1) {
        /* off its feet: flying */
        func_802A02E4(6, p);
        func_802A02E4(9, p);
        D_803F7C28 = JB_LAST_MODE(vs) == JB_LANDED ? 0.1f : 0.5f;
        D_803F7C2C = 0.5f;
        JB_MODE(vs) = JB_FLY;
    }
    func_802C70E8(vs);
    func_802C7354(vs);
    /* its slam landing: from slamming to walking, a sound, the screen's
       shake and its effect */
    if (JB_LAST_MODE(vs) == JB_SLAM && JB_MODE(vs) == JB_WALK) {
        func_80260650(D_80367738, 0x7D, NULL);
        D_802E8BE4 = JBOMB_SLAM_SHAKE_FRAMES;
        D_802E8BE8 = JBOMB_SLAM_SHAKE;
        func_802A6274(T(D_802C28E4), 0x222E0, 0, X << 11, Y << 11, Z << 11, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    switch (JB_MODE(vs)) {
    case JB_LANDED:
        jbomb_landed(p);
        break;
    case JB_WALK:
        jbomb_walk(vs, p);
        break;
    case JB_FLY:
        jbomb_fly(vs, p);
        break;
    case JB_FALL:
    case JB_DROP:
        jbomb_drop(vs, p, JB_MODE(vs));
        break;
    case JB_SLAM:
        jbomb_slamming(vs, p);
        break;
    default:
        engine_trap(0x802C67E8);
        break;
    }
    func_802C7544(vs);
    func_802C6DAC(vs);
    func_802C6ECC(vs);
    D_803F7C43 = VS_AIRBORNE(vs)[0];
    D_803F7C42 = PAD_B;
}

/* standing still on its feet for JBOMB_LAND_FRAMES frames: landed (part
   7) */
REGS(gp)
void func_802C6DAC(VS *vs) {
    if (VS_SPEED(vs) == 0 && JB_MODE(vs) == JB_WALK) {
        if (D_803F7C4B != 0)
            D_803F7C4B--;
    } else {
        D_803F7C4B = JBOMB_LAND_FRAMES;
    }
    if (JB_MODE(vs) != JB_WALK || VS_SPEED(vs) != 0 || D_803F7C4B != 0)
        return;
    JB_MODE(vs) = JB_LANDED;
    func_802A02E4(6, JB);
    func_802A0360(7, 0, JB, 0.0f);
    func_802A039C(7, 3, JB);
    func_802A03D4(7, 0, JB);
    func_802A0480(7, 0, JB, 0.0f);
    func_802A040C(7, 1, JB);
    func_802A0290(7, 1, JB);
}

/* its sounds: the jets (D_803F7C18) while they fire, the flight
   (D_803F7C1C) flying, falling or dropping */
REGS(gp)
void func_802C6ECC(VS *vs) {
    s32 mode = JB_MODE(vs);

    if ((mode == JB_WALK || mode == JB_SLAM) && D_803F7C1C != NULL)
        func_802608C8(D_803F7C1C);
    if (D_803F7C3E != 0) {
        if (D_803F7C18 == NULL)
            func_80260650(D_80367738, 2, &D_803F7C18);
        if (D_803F7C1C != NULL)
            func_802608C8(D_803F7C1C);
        return;
    }
    if (D_803F7C18 != NULL)
        func_802608C8(D_803F7C18);
    mode = JB_MODE(vs);
    if ((mode == JB_FLY || mode == JB_FALL || mode == JB_DROP) && D_803F7C1C == NULL)
        func_80260650(D_80367738, 0xF, &D_803F7C1C);
}

/* the jets' push in the air: the rise of this step of the wheels' fall
   (VS_WHEEL_FALL's step and frames, D_803EBBF4 the gravity), and
   JBOMB_JET_TOP_PUSH more, less toward JB_HIGH */
REGS(gp -> s6)
s32 func_802C6FD8(VS *vs) {
    s32 *f = VS_WHEEL_FALL(vs), t, now, last, top;

    t = f[6];
    now = f[0] * t + engine_cvt_w_s(D_803EBBF4 * (f32)(t * t));
    t = f[6] - 1;
    last = f[0] * t + engine_cvt_w_s(D_803EBBF4 * (f32)(t * t));
    if (Y < JB_LOW) {
        top = JBOMB_JET_TOP_PUSH;
    } else {
        if (JB_HIGH - JB_LOW == 0)
            engine_break(0x802C70C4, 7);
        top = JBOMB_JET_TOP_PUSH - (s32)((u32)((Y - JB_LOW) * JBOMB_JET_TOP_PUSH) / (u32)(JB_HIGH - JB_LOW));
    }
    return now - last + top;
}

/* a ceiling above it (func_802AC0BC, D_803EBBFC): just above, pushed back
   down (flying, the jets' lift); near, falling (flying) */
REGS(gp)
void func_802C70E8(VS *vs) {
    s32 d;

    if (func_802AC0BC(X, Z, Y) == 0 || D_803EBBFC < Y)
        return;
    d = D_803EBBFC - Y;
    if (d < JBOMB_CEILING_NEAR) {
        JB_MODE(vs) = JB_FLY;
        jump(vs, JBOMB_JET_LIFT);
        airborne(vs);
        D_803F7C3E = 1;
    } else if (d < JBOMB_CEILING_FAR) {
        JB_MODE(vs) = JB_FLY;
        jump(vs, 0);
        airborne(vs);
    }
}

/* where it is going (x, z, from func_802A860C): whether a ceiling is above
   it there but not just above (D_803F7C49) */
REGS(t0, t1)
void func_802C71FC(s32 x, s32 z) {
    D_803F7C49 = 0;
    if (func_802AC0BC(x, z, Y) != 0 && !(D_803EBBFC < Y) && D_803EBBFC - Y >= JBOMB_CEILING_NEAR)
        D_803F7C49 = 1;
}

/* at its top damage (unk9F 100): parts 3 and 4 level (unless flying), and
   falling once it is JBOMB_WRECK_HEIGHT above its shadow */
REGS(gp)
void func_802C7354(VS *vs) {
    if (VS_TOP_MATERIAL(vs) != 0x64)
        return;
    if (JB_MODE(vs) != JB_FLY) {
        D_803F7C28 = 0.5f;
        D_803F7C2C = 0.5f;
    }
    if (Y < func_80258500(VEHICLE_JETPACK) + JBOMB_WRECK_HEIGHT) {
        jump(vs, JBOMB_JET_LIFT);
        airborne(vs);
        JB_MODE(vs) = JB_FLY;
        D_803F7C3E = 1;
    }
}

/* q = n / d, as IDO's checked division (its zero and overflow breaks) */
static s32 idiv(s32 n, s32 d, u32 pc_zero, u32 pc_ovf) {
    if (d == 0)
        engine_break(pc_zero, 7);
    if (d == -1 && n == (s32)0x80000000)
        engine_break(pc_ovf, 6);
    return n / d;
}

/* the camera's distance and height for it by its height: from 0xD48 and
   0x258 at JB_LOW to 0x3E8 and 0xBB8 at JB_HIGH */
REGS()
void func_802C7410(void) {
    s32 a, d;

    if (Y < JB_LOW) {
        D_8036444C = 0xD48;
        D_80364450 = 0x258;
    } else if (!(Y < JB_HIGH)) {
        D_8036444C = 0x3E8;
        D_80364450 = 0xBB8;
    } else {
        a = Y - JB_LOW;
        d = JB_HIGH - JB_LOW;
        D_8036444C = idiv(a * -0x960, d, 0x802C746C, 0x802C7484) + 0xD48;
        D_80364450 = idiv(a * 0x960, d, 0x802C74BC, 0x802C74D4) + 0x258;
    }
}

/* its jets: a burst as they start on the ground, their flames' size (part
   1, D_803F7C3B, growing while they fire or it is landed) and their
   sparks every other frame (every fifth landed) */
REGS(gp)
void func_802C7544(VS *vs) {
    s32 s, *pt, r;

    if (D_803F7C43 == 0 && VS_AIRBORNE(vs)[0] == 1 && D_803F7C3E != 0) {
        r = func_802584BC(VEHICLE_JETPACK);
        pt = (s32 *)func_802ABC88(VEHICLE_JETPACK, 1);
        func_80288284(4, pt[0], pt[1], pt[2], r);
    }
    if (JB_MODE(vs) == JB_LANDED || D_803F7C3E != 0) {
        s = D_803F7C3B + 1;
        if (s > JBOMB_FLAMES_MAX)
            s = JBOMB_FLAMES_MAX;
    } else {
        s = D_803F7C3B - 1;
        if (s < 0)
            s = 0;
    }
    D_803F7C3B = s;
    func_802A039C(1, (u32)(s32)D_803F7C3B / 4, JB);
    if (D_803F7C3A != 0) {
        D_803F7C3A--;
        return;
    }
    if (JB_MODE(vs) == JB_LANDED)
        D_803F7C3A = 4;
    else if (D_803F7C3E != 0)
        D_803F7C3A = JBOMB_SPARK_WAIT;
    else
        return;
    func_802A6274(T(D_802C3804), 0x15F90, 1, VEHICLE_JETPACK, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802A6274(T(D_802C3804), 0x15F90, 1, VEHICLE_JETPACK, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
}

/* a boost button's presses counted (*count odd while held), each within
   JBOMB_PRESS_FRAMES frames of the last (*frames), else from 0 again */
static void count_presses(u8 *count, u8 *frames, s32 held) {
    u32 left = *frames, c;

    if (left == 0) {
        *count = 0;
        *frames = JBOMB_PRESS_FRAMES;
        return;
    }
    c = *count;
    if ((c & 1) ? held != 0 : held == 0) {
        *frames = left - 1;
    } else {
        *frames = JBOMB_PRESS_FRAMES;
        *count = c + 1;
    }
}

/* the boost buttons' presses counted: L's in D_803F7C44, R's in
   D_803F7C46 (three of L's: the slam) */
REGS()
void func_802C770C(void) {
    count_presses(&D_803F7C44, &D_803F7C45, PAD_L);
    count_presses(&D_803F7C46, &D_803F7C47, PAD_R);
}

/* a tilt (0..1, 0.5 level) shown on part i: past the middle its animation
   2, before it its animation 1 */
static void show_tilt(s32 i, f32 f) {
    if (!(f <= 0.5f))
        func_802A0360(i, 2, JB, (f - 0.5f) * 2.0f);
    else
        func_802A0360(i, 1, JB, f * 2.0f);
}

/* Its jets' tilts in the air: part 4 (D_803F7C2C) with the stick's x
   (within func_802C7C1C's limits, by the speed; with D_80370C35 or a big
   D_803F7C34), part 3 (D_803F7C28) with its y (within func_802C7BC0's, the
   faster the nearer level), each back to level otherwise. */
REGS(gp)
void func_802C7864(VS *vs) {
    f32 f, lim, step;
    s32 k;

    /* part 4 */
    f = D_803F7C2C;
    k = D_803F7C34;
    if (D_80370C35 != 0 || k >= 0xFA0 || k < -0xF9F) {
        k = STICK_X;
        if (k > JBOMB_TILT_STICK) {
            f += JBOMB_TILT_RATE;
            lim = func_802C7C1C(1, vs);
            if (!(f <= lim))
                f = lim;
            goto set4;
        }
        if (k < -JBOMB_TILT_STICK) {
            f -= JBOMB_TILT_RATE;
            lim = func_802C7C1C(0, vs);
            if (f < lim)
                f = lim;
            goto set4;
        }
    }
    f = step_toward_f(f, 0.5f, JBOMB_TILT_BACK);
set4:
    D_803F7C2C = f;
    show_tilt(4, 1.0f - f);
    /* part 3 */
    f = D_803F7C28;
    if (f <= 0.5f)
        step = (0.5f - (0.5f - f)) * 2.0f;
    else
        step = (0.5f - (f - 0.5f)) * 2.0f;
    step = JBOMB_PITCH_RATE * step;
    k = STICK_Y;
    if (k > 0) {
        lim = 1.0f - func_802C7BC0();
        if (f <= lim) {
            f += step;
            if (!(f <= lim))
                f = lim;
            goto set3;
        }
    } else if (k < 0) {
        lim = func_802C7BC0();
        if (!(f < lim)) {
            f -= step;
            if (f < lim)
                f = lim;
            goto set3;
        }
    }
    f = step_toward_f(f, 0.5f, JBOMB_PITCH_BACK);
set3:
    D_803F7C28 = f;
    show_tilt(3, f);
}

/* the stick's y: (1 - |y| / 80) / 2 */
REGS(-> f4)
f32 func_802C7BC0(void) {
    f32 f;

    f = (f32)iabs(STICK_Y) / 80.0f;
    f = 1.0f - f;
    return f / 2.0f;
}

/* part 4's limit toward `up` (0 down, 1 up): 0.5 -/+ half the speed over
   JBOMB_TILT_SPEED, within 0.1 and 0.9 */
REGS(s2, gp -> f2)
f32 func_802C7C1C(s32 up, VS *vs) {
    f32 f;

    f = (f32)iabs(VS_SPEED(vs)) / JBOMB_TILT_SPEED;
    f = f * 0.5f;
    f = up != 0 ? f + 0.5f : 0.5f - f;
    if (!(f <= 0.9f))
        f = 0.9f;
    if (f < 0.1f)
        f = 0.1f;
    return f;
}

/* the J-Bomb's matrix, its vertices and (unless landed) its collision */
REGS(gp)
void func_802C7CB0(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);

    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = VS_HEADING(vs);
    func_802AA764(X, Y, Z, JBOMB_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    if (JB_MODE(vs) != JB_LANDED)
        func_8029C454(X, Y, Z, VEHICLE_JETPACK, MODEL_AT(model, 4), MODEL_AT(model, 8), buf);
    func_802ABBEC(VEHICLE_JETPACK, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}

/* the steering rate: walking or landed, the speed over JBOMB_STEER_DIV;
   in the air, JBOMB_AIR_STEER, or growing by JBOMB_AIR_STEER_UP a frame to
   JBOMB_AIR_STEER_MAX while the stick is pushed sideways */
REGS(gp -> s3)
s32 func_802C7DFC(VS *vs) {
    s32 mode = JB_MODE(vs), v;

    if (mode == JB_WALK || mode == JB_LANDED)
        return engine_cvt_w_s((f32)VS_SPEED(vs) / JBOMB_STEER_DIV);
    if (mode < JB_FLY || mode > JB_SLAM)
        engine_trap(0x802C7E38);
    v = STICK_X;
    if (v <= JBOMB_AIR_STICK && v >= -JBOMB_AIR_STICK) {
        D_803F7C3C = JBOMB_AIR_STEER;
        return JBOMB_AIR_STEER;
    }
    v = D_803F7C3C + JBOMB_AIR_STEER_UP;
    if (v > JBOMB_AIR_STEER_MAX)
        v = JBOMB_AIR_STEER_MAX;
    D_803F7C3C = v;
    return v;
}

/* the gravity: four times the level's falling, else the level's */
REGS(gp)
void func_802C7ECC(VS *vs) {
    D_803EBBF4 = D_803EBBF0 * (JB_MODE(vs) == JB_FALL ? 4.0f : 1.0f);
}

/* how it lands (not bouncing), and its gears and brake: walking or in the
   air */
REGS(gp)
void func_802C7F28(VS *vs) {
    D_803ED3F6 = 0xFF;
    D_803ED3F7 = 0xFF;
    if (JB_MODE(vs) != JB_WALK) {
        SET_GEARS(vs, -0xC8, 0, 2, 0, 0xFA, 2, 0, 0xFA, 2, 0, 0xFA, 2, 0, 0xFA, 2);
        D_803F7C48 = 4;
    } else {
        SET_GEARS(vs, -0x64, 0, 4, 0, 0x96, 4, 0, 0x96, 4, 0, 0x96, 4, 0, 0x96, 4);
        D_803F7C48 = 0xC;
    }
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802C8074(u8 *dst) {
    func_802AC7DC(dst, (u8 *)&D_803F7B50, (u32 *)&D_803F7BF8);
}

/* and back */
void func_802C80A0(u8 *src) {
    func_802AC85C(src, (u8 *)&D_803F7B50, (u32 *)&D_803F7BF8);
}
