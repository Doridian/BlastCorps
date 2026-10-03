/*
 * hd_code 853D0 (us.v11 0x802C9B90-0x802CB690): the Ballista
 * (VEHICLE_BIKE), as native C (engine.h).  Its parts are D_803F87A0, its
 * state D_803F8AA0 and its position D_803F8B48..50 (vehicle.h).
 * func_802C9B90 sets it up (from the level loader), func_802CA4E0 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.  Parts 4 and 5 are its two missile launchers (it fires them
 * in turn while the missiles D_803F8B72 last), part 3 its lean, part 2 its
 * front wheel in a jump (D_803F8B64..70, a parabola).
 *
 * func_802CA34C and func_802CA3D8, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* the Ballista's .bss (asm/data/hd_code/853D0.bss.s) */
extern Part D_803F87A0[32];
extern VS D_803F8AA0;
extern s32 D_803F8B48, D_803F8B4C, D_803F8B50;  /* x, y, z */
extern u8 *PTR32 D_803F8B54;                    /* its model file */
extern u8 *PTR32 D_803F8B58;                    /* two 0x700-byte buffers, one per frame */
extern u8 *PTR32 D_803F8B5C;
extern f32 D_803F8B60;                          /* the lean, 0..1 (0.5 upright) */
extern s32 D_803F8B64, D_803F8B68, D_803F8B6C;  /* the jump: its speed, its step, its height */
extern u16 D_803F8B70;                          /* the jump's steps so far */
extern u16 D_803F8B74;                          /* the heading it turns to (with D_803A7425) */
extern u8 D_803F8B76;                           /* frames until the next sparks */
extern u8 D_803F8B77;
extern s8 D_803F8B78;                           /* turning to it */
extern s8 D_803F8B79;                           /* a sound to play this frame, or -1 */
extern u8 D_803F8B7A;                           /* frames without the gears after a hit */
extern u8 D_803F8B7B;                           /* bounced */
extern s8 D_803F8B7C;                           /* in a jump */
extern u8 D_803F8B7D;                           /* frames until the next missile */
extern u8 D_803F8B7E;                           /* the launcher that fires next */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern u8 D_80364A6B;
extern u8 D_80370C15, D_80370C16, D_80370C1A, D_80370C1B;
extern s8 D_80370C2D;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306420[];
extern s32 D_803643E4, D_803643E8;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);
s32 func_80292288(s16 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, u8 arg7, s16 arg8);

void func_802CA4E0(void);
void func_802CA308(void);
REGS(gp)
void func_802CAAFC(VS *vs);
REGS(gp)
void func_802CB224(VS *vs);
REGS(gp, s2 -> f2)
f32 func_802CB3C8(VS *vs, s32 up);
REGS(gp)
void func_802CB42C(VS *vs);
REGS(gp -> s3)
s32 func_802CB564(VS *vs);
void func_802CB5D8(void);

#define T(p) ((s32)(p))

/* the Ballista's numbers (62740's helpers'; a rate is a frame's) */
#define BIKE_BRAKE 0x10         /* func_802A785C: the speed's fall a frame, braking */
#define BIKE_TURN_RATE 0x3E80   /* func_802A7FD8: the heading's turning rate */
#define BIKE_SLOPE_DIV 640.0f   /* func_802A843C: the slope's push divided by */
#define BIKE_CAMERA_TURN 0.36f  /* func_802A71DC: the share of the way to the camera's heading it turns a frame, turned (D_803A7425) */
#define BIKE_STUN 5             /* frames without the gears (func_802A785C) after a bounce */
#define BIKE_SPARK_WAIT 1       /* frames between the wheels' sparks */
#define BIKE_STEER_DIV 2.2f     /* the steering's rate: the speed over this (func_802A7E70) */
#define BIKE_STEER_DIV_AIR 6.0f
#define BIKE_LEAN_RATE 0.05f            /* the lean (part 3, D_803F8B60) a frame, to func_802CB3C8's limits */
#define BIKE_UPRIGHT 0.5f               /* and back to this */
#define BIKE_MISSILE_WAIT 5             /* frames between missiles (D_803F8B7D) */
/* the wheelie: the front wheel's height t frames up is v t - BIKE_WHEELIE_G t^2
   (D_803F8B64 v, D_803F8B68 t, from D_803F8B6C), v BIKE_WHEELIE_V to start
   with; while the stick stays up (BIKE_WHEELIE_STICK), for BIKE_WHEELIE_HOLD
   frames at most, it starts again from where it is BIKE_WHEELIE_BOOST
   faster; landing faster than BIKE_WHEELIE_BOUNCE it bounces at half */
#define BIKE_WHEELIE_G 16.0f
#define BIKE_WHEELIE_V 0x3C
#define BIKE_WHEELIE_STICK 0x3C
#define BIKE_WHEELIE_HOLD 0x17
#define BIKE_WHEELIE_BOOST 0x33
#define BIKE_WHEELIE_BOUNCE 0x3C
#define BIKE_WHEELIE_TOP 3400.0f        /* the height part 2's frame is all the way up at */

/* the wheelie's height t frames up at speed v */
static s32 wheelie(s32 v, s32 t) {
    return (s32)((u32)v * (u32)t) + engine_cvt_w_s(-BIKE_WHEELIE_G * (f32)(s32)((u32)t * (u32)t));
} /* and with a wheel off the ground */

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802C9B90(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 avg;
    VS *vs = &D_803F8AA0;
    Part *p = D_803F87A0;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802C9B90);
    engine_save(ENGINE_T0_T5, 0);
    D_803F8B54 = model;
    buf = D_80358070;
    D_803F8B58 = buf;
    D_803F8B5C = buf + 0x700;
    D_80358070 = buf + 0xE00;
    func_802A1388(0xA, 0, D_803F8B58, D_803F8B5C, model);
    ENGINE_BLK(802C9C10);
    func_802A754C(vs);
    ENGINE_BLK(802C9C1C);
    vs->unk52[0] = 0x104;
    vs->unk52[1] = 0x140;
    vs->unk52[2] = -0x104;
    vs->unk52[3] = 0x140;
    vs->unk52[4] = 0x104;
    vs->unk52[5] = -0x140;
    vs->unk5E[0] = 0x12C;
    vs->unk5E[1] = 0x154;
    vs->unk5E[2] = -0x12C;
    vs->unk5E[3] = 0x154;
    vs->unk5E[4] = 0x12C;
    vs->unk5E[5] = -0x154;
    D_803F8B48 = x;
    D_803F8B4C = y;
    D_803F8B50 = z;
    D_803F8B7C = 0;
    D_803F8B7E = 0;
    D_803F8B60 = 0.5f;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803F8B4C, x, z, vs->unk4, &D_803F8B4C, (s16 *)&vs->unk4C, 0xA, vs, engine_ctx(30), &avg);
    ENGINE_BLK(802C9CF0);
    func_8029F85C(p, D_803F8B54, D_803F8B58, D_803F8B5C);
    ENGINE_BLK(802C9D2C);
    func_802A039C(0, 100, p);
    ENGINE_BLK(802C9D40);
    func_802A03D4(0, 0, p);
    ENGINE_BLK(802C9D54);
    func_802A040C(0, 0, p);
    ENGINE_BLK(802C9D68);
    func_802A0480(0, 0, p, 0.0f);
    ENGINE_BLK(802C9D80);
    func_802A0290(0, 1, p);
    ENGINE_BLK(802C9D94);
    func_8029E558(p, D_803F8B58, D_803F8B5C);
    ENGINE_BLK(802C9DA8);
    func_802A0320(0, p);
    ENGINE_BLK(802C9DB8);
    func_802A0290(0, 1, p);
    ENGINE_BLK(802C9DCC);
    func_8029E558(p, D_803F8B5C, D_803F8B58);
    ENGINE_BLK(802C9DE0);
    r = vs->unk78;
    r[0] = -0x96, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0xB4, r[5] = 8;
    r[6] = 0xB4, r[7] = 0xC8, r[8] = 4;
    r[9] = 0xC8, r[10] = 0xFA, r[11] = 2;
    r[12] = 0xFA, r[13] = 0x104, r[14] = 2;
    D_803F8B78 = 0;
    D_803F8B7B = 0;
    D_803F8B7A = 0;
    D_803F8B76 = 0;
    D_803F8B77 = 0x32;
    D_803F8B7D = 0;
    D_803F8B72 = 0;
    func_8029C354(0xA, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x3E80);
    ENGINE_BLK(802C9EC4);
    func_80258230(0xA, 0x64, 0x2D, 0x2D);
    ENGINE_BLK(802C9EDC);
    vs->unk9A = 1;
    func_802CA4E0();
    ENGINE_BLK(802C9EEC);
    vs->unk9A = 0;
    func_802AA838(D_803F8B5C, D_803F8B58, *(s32 *)(D_803F8B54 + *(s32 *)(D_803F8B54 + 0x18) + 4));
    ENGINE_BLK(802C9F24);
    D_80364A6B = 1;
    engine_restore();
}

/* hd.c's: the player gets in: the launchers' and wheels' parts reset */
void func_802C9F54(void) {
    VS *vs = &D_803F8AA0;
    Part *p = D_803F87A0;

    ENGINE_BLK(802C9F54);
    vs->unk96[3] = 0;
    D_8036444C = 0xD48;
    D_80364450 = 1000;
    func_802C4310(0x94);
    ENGINE_BLK(802C9F90);
    func_802A039C(1, 0, p);
    ENGINE_BLK(802C9FA4);
    func_802A03D4(1, 0, p);
    ENGINE_BLK(802C9FB8);
    func_802A040C(1, 0, p);
    ENGINE_BLK(802C9FCC);
    func_802A0290(1, -1, p);
    ENGINE_BLK(802C9FE0);
    func_802A039C(2, 0, p);
    ENGINE_BLK(802C9FF4);
    func_802A03D4(2, 0, p);
    ENGINE_BLK(802CA008);
    func_802A040C(2, 1, p);
    ENGINE_BLK(802CA01C);
    func_802A0290(2, -1, p);
    ENGINE_BLK(802CA030);
    func_802A039C(3, 0, p);
    ENGINE_BLK(802CA044);
    func_802A03D4(3, 0, p);
    ENGINE_BLK(802CA058);
    func_802A040C(3, 1, p);
    ENGINE_BLK(802CA06C);
    func_802A0290(3, -1, p);
    ENGINE_BLK(802CA080);
    func_802A039C(4, 8, p);
    ENGINE_BLK(802CA094);
    func_802A03D4(4, 0, p);
    ENGINE_BLK(802CA0A8);
    func_802A040C(4, 0, p);
    ENGINE_BLK(802CA0BC);
    func_802A0480(4, 1, p, 1.0f);
    ENGINE_BLK(802CA0D8);
    func_802A039C(5, 8, p);
    ENGINE_BLK(802CA0EC);
    func_802A03D4(5, 0, p);
    ENGINE_BLK(802CA100);
    func_802A040C(5, 0, p);
    ENGINE_BLK(802CA114);
    func_802A0480(5, 1, p, 1.0f);
    ENGINE_BLK(802CA130);
}

/* hd.c's: whether it can be left: not in the air or in a jump */
u8 func_802CA140(void) {
    VS *vs = &D_803F8AA0;
    u8 r = 0;

    ENGINE_BLK(802CA140);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802CA164);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802CA174);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802CA184);
                if (D_803F8B7C == 0) {
                    ENGINE_BLK(802CA194);
                    r = 1;
                }
            }
        }
    }
    ENGINE_BLK(802CA198);
    return r;
}

/* hd.c's: the player gets out */
void func_802CA1AC(void) {
    VS *vs = &D_803F8AA0;

    ENGINE_BLK(802CA1AC);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803F8B58, (u32 *)D_803F8B5C, 0x700);
    ENGINE_BLK(802CA1DC);
    func_802C444C();
    ENGINE_BLK(802CA1E4);
}

/* hd.c's: put back on the ground where it is */
void func_802CA1F8(void) {
    VS *vs = &D_803F8AA0;

    ENGINE_BLK(802CA1F8);
    func_802A9A60(vs->unk52, D_803F8B4C, D_803F8B48, D_803F8B50, vs->unk4, &D_803F8B4C, (s16 *)&vs->unk4C, 0xA, vs, engine_ctx(30));
    ENGINE_BLK(802CA284);
    func_802CB42C(vs);
    ENGINE_BLK(802CA28C);
    func_802A133C(D_803F8B48, D_803F8B4C, D_803F8B50, 0xA, vs);
    ENGINE_BLK(802CA2B8);
}

/* its light */
void func_802CA308(void) {
    ENGINE_BLK(802CA308);
    func_802ABD54(0xA, D_803F8B48, D_803F8B4C, D_803F8B50);
    ENGINE_BLK(802CA33C);
}

/* each frame */
void func_802CA4E0(void) {
    VS *vs = &D_803F8AA0;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_BLK(802CA4E0);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802CA308();
    ENGINE_BLK(802CA534);
    func_802A75DC((u8 *)D_803F87A0, &D_803F8B48, &D_803F8B4C, &D_803F8B50, (u8 *)vs);
    ENGINE_BLK(802CA560);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802CA56C);
        func_802CAAFC(vs);
    }
    ENGINE_BLK(802CA574);
    if (D_80367BFF != 0) {
        ENGINE_BLK(802CA584);
        func_802CB690(vs);
    }
    ENGINE_BLK(802CA58C);
    func_802CB5D8();
    ENGINE_BLK(802CA594);
    rate_i = func_802CB564(vs);
    ENGINE_BLK(802CA59C);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    ENGINE_BLK(802CA5B8);
    if (D_803F8B7A == 0) {
        ENGINE_BLK(802CA5C8);
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, BIKE_BRAKE, vs, &t3);
        ENGINE_BLK(802CA5D0);
    } else {
        ENGINE_BLK(802CA5D8);
        D_803F8B7A--;
    }
    ENGINE_BLK(802CA5E4);
    func_802A7FD8(BIKE_TURN_RATE, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    ENGINE_BLK(802CA5FC);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802CA608);
    func_802A843C(&vs->unk76, 1, 0xA, (s8 *)vs->unk96, vs->unk4, BIKE_SLOPE_DIV, vs);
    ENGINE_BLK(802CA61C);
    if (D_803F8B78 != 0) {
        ENGINE_BLK(802CA62C);
        func_802A7070((s16 *)&D_803F8B74, vs);
    }
    ENGINE_BLK(802CA638);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803F8B48, &D_803F8B50, rate, &z);
    ENGINE_BLK(802CA650);
    D_803ED40B = 1;
    /* ($s4 and $s7, which func_802A8768 reads too) */
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803F8B48, &D_803F8B50, &D_803F8B4C, 0xA, 0x280, 0x208, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802CA688);
    if (D_8035805C != 0) {
        ENGINE_BLK(802CA6B0);
        func_8029E558(D_803F87A0, D_803F8B58, D_803F8B5C);
        ENGINE_BLK(802CA6C4);
    } else {
        ENGINE_BLK(802CA6CC);
        func_8029E558(D_803F87A0, D_803F8B5C, D_803F8B58);
    }
    ENGINE_BLK(802CA6E0);
    func_802CB42C(vs);
    ENGINE_BLK(802CA6E8);
    ENGINE_LEAVE(8, 8);         /* $t0 and $t2, which func_8029A800 reads too */
    ENGINE_LEAVE(10, 0x64);
    func_8029A800(D_803F8B48, D_803F8B4C, D_803F8B50, D_80306420, 1, 1, vs->unk76, 0, 0xA, vs);
    ENGINE_BLK(802CA734);
    func_8029C52C(0xA, vs);
    ENGINE_BLK(802CA73C);
    func_8029AA10();
    ENGINE_BLK(802CA744);
    D_803F8B79 = 0xC;
    if (D_803A7425 == 0) {
        ENGINE_BLK(802CA75C);
        D_803A7424 = 0;
        D_803F77D0 = D_803F87A0;
        func_802BE77C(0xA, vs);
        ENGINE_BLK(802CA780);
        D_803F8B79 = -1;
        if (D_803A7424 == 0) {
            ENGINE_BLK(802CA798);
            ENGINE_BLK(802CA8F8);
            D_803F8B78 = 0;
            D_803F8B7B = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    ENGINE_BLK(802CA7A0);
    func_8029A914(vs);
    ENGINE_BLK(802CA7A8);
    D_803F8B78 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802CA7B8);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802CA7C8);
        h += 0xFFF;
    }
    ENGINE_BLK(802CA7CC);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802CA7D8);
        h = -h;
    }
    ENGINE_BLK(802CA7DC);
    if (!(h < 0x801)) {
        ENGINE_BLK(802CA7E8);
    }
    ENGINE_BLK(802CA7F0);
    ENGINE_BLK(802CA89C);
    func_802A70D8(vs);
    ENGINE_BLK(802CA8A4);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, BIKE_CAMERA_TURN, vs, &a1);

        ENGINE_BLK(802CA8B8);
        D_803F8B74 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802CA8CC);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802CA8D4);
    D_803F77D0 = D_803F87A0;
    func_802BE77C(0xA, vs);
    ENGINE_BLK(802CA8F0);
    goto done;

hit:
    /* bounced off something: once, the speed turned round (0x41..0x82
       forward, 0x25..0x4B back) */
    ENGINE_BLK(802CA804);
    D_803F8B78 = 0;
    if (D_803F8B7B != 0)
        goto done;
    ENGINE_BLK(802CA81C);
    if (D_803F8B7A != 0)
        goto done;
    ENGINE_BLK(802CA82C);
    D_803F8B7A = BIKE_STUN;
    v = vs->unk76;
    if (v >= 0) {
        ENGINE_BLK(802CA844);
        if (v < 0x41) {
            ENGINE_BLK(802CA85C);
            v = 0x41;
        } else {
            ENGINE_BLK(802CA84C);
            if (!(v < 0x83)) {
                ENGINE_BLK(802CA854);
                v = 0x82;
            }
        }
    } else {
        ENGINE_BLK(802CA864);
        if (!(v < -0x24)) {
            ENGINE_BLK(802CA880);
            v = -0x25;
        } else {
            ENGINE_BLK(802CA870);
            if (v < -0x4B) {
                ENGINE_BLK(802CA878);
                v = -0x4B;
            }
        }
    }
    ENGINE_BLK(802CA884);
    vs->unk76 = -v;
    D_803F8B7B = 1;

done:
    ENGINE_BLK(802CA908);
    D_803643E0 = D_803F8B48;
    D_803643E4 = D_803F8B4C;
    D_803643E8 = D_803F8B50;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0xA, vs);
    ENGINE_BLK(802CA984);
    if (D_803F8B79 >= 0) {
        ENGINE_BLK(802CA994);
        func_80260650(D_80367738, D_803F8B79, NULL);
        ENGINE_BLK(802CAA28);
    }
    ENGINE_BLK(802CAAA8);
    engine_restore();
}

/* the wheels: sparks, spin and sound; the dust; the lean (C buttons); the
   missiles (Z or R); the jump (the stick hard forward) */
REGS(gp)
void func_802CAAFC(VS *vs) {
    Part *p = D_803F87A0;
    s32 s, a1, a2, a3, s10, s14, s18, u, t4, t5, t2, s6, s7;
    u32 q;
    f32 f, g, h;
    s32 *rec;

    ENGINE_BLK(802CAAFC);
    if (D_803F8B76 != 0) {
        ENGINE_BLK(802CAB18);
        D_803F8B76--;
        goto wheels;
    }
    ENGINE_BLK(802CAB24);
    if (vs->unk96[3] == 0)
        goto wheels;
    ENGINE_BLK(802CAB34);
    D_803F8B76 = BIKE_SPARK_WAIT;
    s = func_802A5ED0();
    ENGINE_BLK(802CAB48);
    if (!(s < 4))
        goto wheels;
    ENGINE_BLK(802CAB58);
    func_802A6274(T(D_802C2954), 0x30D40, 1, 0xA, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0);
wheels:
    ENGINE_BLK(802CAB84);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802CAB90);
        func_802A03D4(1, 1, p);
        ENGINE_BLK(802CABA4);
    } else {
        ENGINE_BLK(802CABAC);
        func_802A03D4(1, 0, p);
    }
    ENGINE_BLK(802CABC0);
    if (s < 0) {
        ENGINE_BLK(802CABCC);
        s = -s;
    }
    ENGINE_BLK(802CABD0);
    ENGINE_BLK(802CABEC);
    func_802A039C(1, (u32)s / 6, p);
    ENGINE_BLK(802CABFC);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802CAC08);
        s = -s;
    }
    ENGINE_BLK(802CAC0C);
    func_802C4584((u32)s >> 3);
    ENGINE_BLK(802CAC14);
    func_802CB224(vs);
    ENGINE_BLK(802CAC1C);
    f = D_803F8B60;
    if (D_80370C15 != 0) {
        ENGINE_BLK(802CAC3C);
        f = f - BIKE_LEAN_RATE;
        g = func_802CB3C8(vs, 0);
        ENGINE_BLK(802CAC4C);
        if (f < g) {
            ENGINE_BLK(802CAC58);
            f = g;
        }
    } else {
        ENGINE_BLK(802CAC60);
        if (D_80370C16 != 0) {
            ENGINE_BLK(802CAC74);
            f = f + BIKE_LEAN_RATE;
            g = func_802CB3C8(vs, 1);
            ENGINE_BLK(802CAC84);
            if (!(f <= g)) {
                ENGINE_BLK(802CAC90);
                f = g;
            }
        } else {
            /* back upright */
            ENGINE_BLK(802CAC98);
            h = BIKE_UPRIGHT;
            if (f < h) {
                ENGINE_BLK(802CACB8);
                f = f + BIKE_LEAN_RATE;
                if (!(f <= h)) {
                    ENGINE_BLK(802CACC8);
                    f = h;
                }
            } else {
                ENGINE_BLK(802CACD0);
                f = f - BIKE_LEAN_RATE;
                if (f < h) {
                    ENGINE_BLK(802CACE0);
                    f = h;
                }
            }
        }
    }
    ENGINE_BLK(802CACE4);
    D_803F8B60 = f;
    func_802A0360(3, 0, p, f);
    ENGINE_BLK(802CAD08);
    if (D_803F8B7D != 0) {
        ENGINE_BLK(802CAD18);
        D_803F8B7D--;
        goto jump;
    }
    ENGINE_BLK(802CAD28);
    if (D_80370C1A == 0) {
        ENGINE_BLK(802CAD38);
        if (D_80370C1B == 0)
            goto jump;
    }
    ENGINE_BLK(802CAD48);
    u = D_803F8B72;
    if ((u16)u == 0)
        goto jump;
    ENGINE_BLK(802CAD58);
    D_803F8B72 = u - 1;
    func_80260650(D_80367738, 3, NULL);
    ENGINE_BLK(802CAD7C);
    D_803F8B7E ^= 1;
    if (D_803F8B7E != 0) {
        ENGINE_BLK(802CAD94);
        func_802A0360(4, 0, p, 0.0f);
        ENGINE_BLK(802CADB0);
        func_802A0290(4, 1, p);
        ENGINE_BLK(802CADC4);
        rec = func_802ABC88(0xA, 3);
        ENGINE_BLK(802CADD0);
        s10 = rec[0], s14 = rec[1], s18 = rec[2];
        rec = func_802ABC88(0xA, 4);
        ENGINE_BLK(802CADF4);
    } else {
        ENGINE_BLK(802CAE04);
        func_802A0360(5, 0, p, 0.0f);
        ENGINE_BLK(802CAE20);
        func_802A0290(5, 1, p);
        ENGINE_BLK(802CAE34);
        rec = func_802ABC88(0xA, 1);
        ENGINE_BLK(802CAE40);
        s10 = rec[0], s14 = rec[1], s18 = rec[2];
        rec = func_802ABC88(0xA, 2);
        ENGINE_BLK(802CAE64);
    }
    a1 = rec[0], a2 = rec[1], a3 = rec[2];
    ENGINE_BLK(802CAE70);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802CAE84);
        s = -s;
    }
    ENGINE_BLK(802CAE88);
    q = (u32)s * 0x140 / 0x104 + 0x280;
    ENGINE_BLK(802CAEC4);
    func_80292288(q, a1, a2, a3, s10, s14, s18, 0, 0x1A4);
    ENGINE_BLK(802CAECC);
    D_803F8B7D = BIKE_MISSILE_WAIT;
jump:
    ENGINE_BLK(802CAEE0);
    if (func_802A7CB0(0xA, vs) != 0) {
        ENGINE_BLK(802CAEE8);
        goto fly;
    }
    ENGINE_BLK(802CAEE8);
    ENGINE_BLK(802CAEF0);
    if (D_80370C2D < BIKE_WHEELIE_STICK)
        goto fly;
    ENGINE_BLK(802CAF04);
    if (D_803F8B7C == 0) {
        ENGINE_BLK(802CAF14);
        D_803F8B7C = 1;
        D_803F8B6C = 0;
        D_803F8B68 = 1;
        D_803F8B64 = BIKE_WHEELIE_V;
        D_803F8B70 = 0;
        goto fly;
    }
    ENGINE_BLK(802CAF48);
    t4 = D_803F8B70;
    if (t4 >= BIKE_WHEELIE_HOLD)
        goto fly;
    /* a higher jump while the stick is held: the parabola started again from
       its height with the speed it has */
    ENGINE_BLK(802CAF5C);
    D_803F8B70 = t4 + 1;
    s6 = wheelie(D_803F8B64, D_803F8B68);
    D_803F8B6C = D_803F8B6C + s6;
    s7 = wheelie(D_803F8B64, D_803F8B68 - 1);
    D_803F8B64 = s6 - s7 + BIKE_WHEELIE_BOOST;
    D_803F8B68 = 1;
fly:
    ENGINE_BLK(802CB03C);
    if (D_803F8B7C == 0)
        goto down;
    ENGINE_BLK(802CB04C);
    s6 = wheelie(D_803F8B64, D_803F8B68);
    a3 = D_803F8B6C + s6;
    if (!(a3 > 0)) {
        /* landed: a bounce, unless it was a small one or a wheel is off */
        ENGINE_BLK(802CB0B0);
        s7 = wheelie(D_803F8B64, D_803F8B68 - 1);
        s7 -= s6;
        if (s7 < 0) {
            ENGINE_BLK(802CB110);
            s7 = -s7;
        }
        ENGINE_BLK(802CB114);
        if (s7 <= BIKE_WHEELIE_BOUNCE)
            goto down;
        ENGINE_BLK(802CB120);
        if (vs->unk96[0] == 1)
            goto down;
        ENGINE_BLK(802CB130);
        if (vs->unk96[1] == 1)
            goto down;
        ENGINE_BLK(802CB140);
        if (vs->unk96[2] == 1)
            goto down;
        ENGINE_BLK(802CB150);
        D_803F8B6C = 0;
        D_803F8B64 = (u32)s7 / 2;
        ENGINE_BLK(802CB184);
        D_803F8B68 = 1;
        a3 = 0;
    }
    ENGINE_BLK(802CB18C);
    f = (f32)a3 / BIKE_WHEELIE_TOP;
    if (!(f <= 1.0f)) {
        ENGINE_BLK(802CB1B8);
        f = 1.0f;
    }
    ENGINE_BLK(802CB1C0);
    func_802A0360(2, 0, p, f);
    ENGINE_BLK(802CB1D8);
    D_803F8B68++;
    goto out;
down:
    ENGINE_BLK(802CB1F0);
    D_803F8B7C = 0;
    func_802A0360(2, 0, p, 0.0f);
out:
    ENGINE_BLK(802CB214);
}

/* dust behind it on the ground */
REGS(gp)
void func_802CB224(VS *vs) {
    ENGINE_BLK(802CB224);
    if (vs->unk96[3] == 0)
        goto done;
    ENGINE_BLK(802CB2B4);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802CB2C4);
    if (!(vs->unk50 < 3))
        goto done;
    ENGINE_BLK(802CB2D4);
    if (vs->unk9B != 0)
        goto done;
    ENGINE_BLK(802CB2E0);
    func_8027BE7C(3, vs->unk4[6], 0, -0x320, 0, -0x320, D_803F8B48, D_803F8B50, vs->unk4E, 5, 0x28, 0, 1);
    ENGINE_BLK(802CB340);
done:
    ENGINE_BLK(802CB344);
}

/* the lean's limit at this speed: 0.5 -/+ |speed| / 280 / 2 */
REGS(gp, s2 -> f2)
f32 func_802CB3C8(VS *vs, s32 up) {
    s32 s = vs->unk76;
    f32 f;

    ENGINE_BLK(802CB3C8);
    if (s < 0) {
        ENGINE_BLK(802CB3E0);
        s = -s;
    }
    ENGINE_BLK(802CB3E4);
    f = (f32)s / 280.0f * 0.5f;
    if (up != 0) {
        ENGINE_BLK(802CB410);
        f = f + 0.5f;
    } else {
        ENGINE_BLK(802CB418);
        f = 0.5f - f;
    }
    ENGINE_BLK(802CB41C);
    /* ($s3, as the J-Bomb's func_802C7C1C leaves its own for its effects) */
    ENGINE_LEAVE(19, 0x118);
    return f;
}

/* the Ballista's matrix, its vertices and its collision */
REGS(gp)
void func_802CB42C(VS *vs) {
    u8 *model = D_803F8B54, *buf;
    s32 *m, off;

    ENGINE_BLK(802CB42C);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802CB45C);
        m = (s32 *)(D_803F8B58 + off);
    } else {
        ENGINE_BLK(802CB470);
        m = (s32 *)(D_803F8B5C + off);
    }
    ENGINE_BLK(802CB480);
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803F8B48, D_803F8B4C, D_803F8B50, 0x3E80, m);
    ENGINE_LEAVE(18, T(m));           /* ($s2: 62740's func_802ABBEC reads it) */
    ENGINE_BLK(802CB4BC);
    if (D_8035805C != 0) {
        ENGINE_BLK(802CB4D0);
        buf = D_803F8B58;
    } else {
        ENGINE_BLK(802CB4E0);
        buf = D_803F8B5C;
    }
    ENGINE_BLK(802CB4EC);
    model = D_803F8B54;
    func_8029C454(D_803F8B48, D_803F8B4C, D_803F8B50, 0xA, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802CB534);
    func_802ABBEC(0xA, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802CB554);
}

/* the turn rate: the speed / 2.2, or / 6 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802CB564(VS *vs) {
    f32 d;

    ENGINE_BLK(802CB564);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802CB578);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802CB588);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802CB598);
    d = BIKE_STEER_DIV;
    goto div;
air:
    ENGINE_BLK(802CB5A0);
    d = BIKE_STEER_DIV_AIR;
div:
    ENGINE_BLK(802CB5AC);
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for the Ballista */
void func_802CB5D8(void) {
    ENGINE_BLK(802CB5D8);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 2;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802CB634(u8 *dst) {
    ENGINE_BLK(802CB634);
    func_802AC7DC(dst, (u8 *)&D_803F8AA0, (u32 *)&D_803F8B48);
    ENGINE_BLK(802CB650);
}

/* and back */
void func_802CB660(u8 *src) {
    ENGINE_BLK(802CB660);
    func_802AC85C(src, (u8 *)&D_803F8AA0, (u32 *)&D_803F8B48);
    ENGINE_BLK(802CB67C);
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802CA34C(s32 carrier) {
    CARRY_KEEP(802CA34C, 802CA388, 802CA3A8, 802CA3B4, &D_803F8AA0, D_803F8B48, D_803F8B50);
}

REGS(a3)
void func_802CA3D8(s32 carrier) {
    CARRY_MOVE(802CA3D8, 802CA414, 802CA428, 802CA430, 802CA490, 802CA498, 802CA4C4, &D_803F8AA0, &D_803F8B48, &D_803F8B4C, &D_803F8B50, 0xA, 0x280, 0x208,
               D_803ED40B = 1, func_802CB5D8(), func_802CB42C(&D_803F8AA0));
}
