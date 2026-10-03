/*
 * hd_code 71140 (us.v11 0x802B5900-0x802B7334): the Backlash truck
 * (VEHICLE_TRUCK), as native C (engine.h), and the two functions of 679E0
 * only it calls (func_802AC284, func_802AC2A4).  Its parts are D_803EE790,
 * its state D_803EEA90 and its position D_803EEB38..40 (vehicle.h).
 * func_802B5900 sets it up (from the level loader), func_802B6294 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.  Part 5 is its tilting bed (the C buttons, D_803EEB54), part
 * 4 its suspension (D_803EEB50), parts D_802C2208/D_802C226C its wheels'
 * turn with the camera.
 *
 * func_802B6100 and func_802B618C, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* the truck's .bss (asm/data/hd_code/71140.bss.s) */
extern Part D_803EE790[32];
extern VS D_803EEA90;
extern s32 D_803EEB38, D_803EEB3C, D_803EEB40;  /* x, y, z */
extern u8 *PTR32 D_803EEB44;                    /* its model file */
extern u8 *PTR32 D_803EEB48;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EEB4C;
extern f32 D_803EEB50;                          /* the suspension, 0..1 (0.5 level) */
extern f32 D_803EEB54;                          /* the bed's tilt, 0..1 (0.5 level) */
extern u16 D_803EEB58;                          /* the turn's speed (slower with a C button) */
extern u16 D_803EEB5A;                          /* the heading it turns to (with D_803A7425) */
extern s16 D_803EEB5C;                          /* the heading the wheels turn from */
extern u8 D_803EEB5E;                           /* frames until the next sparks */
extern u8 D_803EEB5F;                           /* the cab's lean, 0..100 */
extern s8 D_803EEB60;                           /* turning to it */
extern s8 D_803EEB61;                           /* a sound to play this frame, or -1 */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern s32 D_80364AA8;
extern f32 D_80364414;                          /* the camera's heading, degrees */
extern u8 D_80370C15, D_80370C16, D_80370C1A, D_80370C1B, D_80370C1C, D_80370C23;
extern Part *PTR32 D_803F77D0;
extern u8 D_80305D20[];
extern s32 D_803643E4, D_803643E8;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */
extern u8 D_802C2208[], D_802C226C[];           /* the front wheels (56040's list) */
extern u8 *PTR32 D_803F3910[];                  /* the objects it is carrying, to D_803F3960 */
extern u8 *PTR32 *PTR32 D_803F3960;
extern u16 D_8036E4C8;
extern u8 D_803BE738;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);
void func_80278EB0(s32 a, f32 b, s32 c);
u8 func_802794F0(void);
void func_802794A4(void);

/* 77E20's: an object moved by (dx, dz) */
REGS(v0, a2, a3, t0)
void func_802BD99C(u8 *obj, s32 dx, s32 a3, s32 dz);

void func_802B6294(void);
void func_802B60BC(void);
REGS(gp)
void func_802B69F8(VS *vs);
REGS(gp)
void func_802B6C28(VS *vs);
REGS(gp)
void func_802B7030(VS *vs);
REGS(gp -> s3)
s32 func_802B7168(VS *vs);
REGS(gp, s2 -> f2)
f32 func_802B71DC(VS *vs, s32 up);
void func_802B7240(void);
REGS(v0, v1, a0)
void func_802AC284(s32 *x, s32 *y, s32 *z);
REGS(v0, v1, a0, a1, t8, gp)
void func_802AC2A4(s32 x, s32 y, s32 z, u8 *a1, s32 type, VS *vs);

#define T(p) ((s32)(p))

/* the camera's heading turned into a 12-bit angle, added to h */
static s32 camera_turn(s32 h) {
    return h + engine_cvt_w_s(D_80364414 * 4096.0f / 360.0f);
}

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802B5900(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 avg;
    VS *vs = &D_803EEA90;
    Part *p = D_803EE790;
    u8 *buf;
    s16 *r;
    s32 h;

    ENGINE_COST(802B5900, 245);
    D_803EEB44 = model;
    buf = D_80358070;
    D_803EEB48 = buf;
    D_803EEB4C = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(5, 1, D_803EEB48, D_803EEB4C, model);
    func_802A754C(vs);
    vs->unk52[0] = 0x168;
    vs->unk52[1] = 0x168;
    vs->unk52[2] = -0x168;
    vs->unk52[3] = 0x168;
    vs->unk52[4] = 0x168;
    vs->unk52[5] = -0x168;
    vs->unk5E[0] = 0x17C;
    vs->unk5E[1] = 0x17C;
    vs->unk5E[2] = -0x17C;
    vs->unk5E[3] = 0x17C;
    vs->unk5E[4] = 0x17C;
    vs->unk5E[5] = -0x17C;
    D_803EEB38 = x;
    D_803EEB3C = y;
    D_803EEB40 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803EEB3C, x, z, vs->unk4, &D_803EEB3C, (s16 *)&vs->unk4C, 5, vs, 0, &avg);
    func_8029F85C(p, D_803EEB44, D_803EEB48, D_803EEB4C);
    func_802A039C(0, 100, p);
    func_802A03D4(0, 0, p);
    func_802A040C(0, 0, p);
    func_802A0480(0, 0, p, 0.0f);
    func_802A0290(0, 1, p);
    func_8029E558(p, D_803EEB48, D_803EEB4C);
    func_802A0320(0, p);
    func_802A0290(0, 1, p);
    func_8029E558(p, D_803EEB4C, D_803EEB48);
    r = vs->unk78;
    r[0] = -0x78, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0x50, r[5] = 6;
    r[6] = 0x50, r[7] = 0x64, r[8] = 4;
    r[9] = 0x64, r[10] = 0x8C, r[11] = 2;
    r[12] = 0x8C, r[13] = 0xDC, r[14] = 2;
    func_802A6F00(vs);
    D_803EEB60 = 0;
    D_803EEB5E = 0;
    D_803EEB5F = 0x32;
    D_803EEB50 = 0.5f;
    D_803EEB54 = 0.5f;
    func_8029C354(5, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x4268);
    func_80258230(5, 0x64, 0x2D, 0x2D);
    vs->unk9A = 1;
    func_802B6294();
    vs->unk9A = 0;
    func_802AA838(D_803EEB4C, D_803EEB48, *(s32 *)(D_803EEB44 + *(s32 *)(D_803EEB44 + 0x18) + 4));
    h = camera_turn((u16)vs->unk4C);
    if (!(h < 0x1000)) {
        h -= 0xFFF;
    }
    D_803EEB5C = h;
}

/* hd.c's: the player gets in: the bed and wheels reset */
void func_802B5CD8(void) {
    VS *vs = &D_803EEA90;
    Part *p = D_803EE790;

    ENGINE_COST(802B5CD8, 139);
    vs->unk96[3] = 0;
    func_802A039C(1, 0, p);
    func_802A03D4(1, 0, p);
    func_802A040C(1, 0, p);
    func_802A0290(1, -1, p);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 1, p);
    func_802A0360(2, 0, p, 0.5f);
    func_802A0290(2, -1, p);
    func_802A039C(4, 0, p);
    func_802A03D4(4, 0, p);
    func_802A040C(4, 1, p);
    func_802A0290(4, -1, p);
    func_802A039C(5, 0, p);
    func_802A03D4(5, 0, p);
    func_802A040C(5, 1, p);
    func_802A0290(5, -1, p);
    func_802A05D0(D_802C2208, 0);
    func_802A05F8(D_802C2208, 0);
    func_802A0620(D_802C2208, 0);
    func_802A0508(D_802C2208, -1);
    func_802A05D0(D_802C226C, 0);
    func_802A05F8(D_802C226C, 0);
    func_802A0620(D_802C226C, 0);
    func_802A0508(D_802C226C, -1);
    D_8036444C = 0xD48;
    D_80364450 = 0x12C;
    func_802C4310(5);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B5F04(void) {
    VS *vs = &D_803EEA90;
    u8 r = 0;

    ENGINE_COST(802B5F04, 23);
    if (vs->unk96[0] != 1) {
        if (vs->unk96[1] != 1) {
            if (vs->unk96[2] != 1) {
                r = 1;
            }
        }
    }
    return r;
}

/* hd.c's: the player gets out */
void func_802B5F60(void) {
    VS *vs = &D_803EEA90;

    ENGINE_COST(802B5F60, 19);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EEB48, (u32 *)D_803EEB4C, 0x800);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802B5FAC(void) {
    VS *vs = &D_803EEA90;

    ENGINE_COST(802B5FAC, 35);
    func_802A9A60(vs->unk52, D_803EEB3C, D_803EEB38, D_803EEB40, vs->unk4, &D_803EEB3C, (s16 *)&vs->unk4C, 5, vs,
                  0);
    func_802B7030(vs);
    func_802A133C(D_803EEB38, D_803EEB3C, D_803EEB40, 5, vs);
}

/* its light */
void func_802B60BC(void) {
    ENGINE_COST(802B60BC, 17);
    func_802ABD54(5, D_803EEB38, D_803EEB3C, D_803EEB40);
}

/* each frame */
void func_802B6294(void) {
    VS *vs = &D_803EEA90;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_COST(802B6294, 259);
    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802B60BC();
    func_802A75DC((u8 *)D_803EE790, &D_803EEB38, &D_803EEB3C, &D_803EEB40, (u8 *)vs);
    if (vs->unk9A == 0) {
        func_802B6C28(vs);
    }
    if (D_80367BFF != 0) {
        func_802CB690(vs);
    }
    func_802B7240();
    rate_i = func_802B7168(vs);
    func_802A7834(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 8, rate_i, &vs->unk4C, vs, &t3, &turn);
    func_802A7FD8(D_803EEB58, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 1, 5, (s8 *)vs->unk96, vs->unk4, 720.0f, vs);
    if (D_803EEB60 != 0) {
        func_802A7070((s16 *)&D_803EEB5A, vs);
    }
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EEB38, &D_803EEB40, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EEB38, &D_803EEB40, &D_803EEB3C, 5, 0x2D0, 0x2D0, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    if (D_8035805C != 0) {
        func_8029E558(D_803EE790, D_803EEB48, D_803EEB4C);
    } else {
        func_8029E558(D_803EE790, D_803EEB4C, D_803EEB48);
    }
    /* the engine's rumble on the ground */
    if (vs->unk96[3] != 0) {
        if (func_802794F0() == 0) {
            func_80278EB0(6, 0.25f, 100);
        } else {
        }
    } else {
        func_802794A4();
    }
    func_802B69F8(vs);
    if (D_803EEB5E != 0) {
        D_803EEB5E--;
        goto sparks_done;
    }
    if (vs->unk96[3] == 0)
        goto sparks_done;
    D_803EEB5E = 1;
    v = func_802A5ED0();
    if (!(v < 4))
        goto sparks_done;
    func_802A6274(T(D_802C2954), 0x30D40, 1, 5, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
sparks_done:
    func_802AC284(&D_803EEB38, &D_803EEB3C, &D_803EEB40);
    func_802B7030(vs);
    ENGINE_LEAVE(8, 8);         /* $t0 and $t2: func_8029A800 (56040.c) takes them from the context */
    ENGINE_LEAVE(10, 0x64);
    func_8029A800(D_803EEB38, D_803EEB3C, D_803EEB40, D_80305D20, 1, 1, vs->unk76, 0, 5, vs);
    func_8029C52C(5, vs);
    func_8029AA10();
    D_803EEB61 = 0xC;
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803EEB61 = -1;
        if (D_80364AA8 == 0x40)
            goto level_over;
        D_803F77D0 = D_803EE790;
        func_802BE77C(5, vs);
        if (D_803A7424 != 0)
            goto hit;
level_over:
        D_803EEB60 = 0;
        goto done;
    }
    /* D_803A7425: turned toward the camera's heading */
    func_8029A914(vs);
    D_803EEB60 = 1;
    v = func_802A6F6C();
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        h += 0xFFF;
    }
    h -= v;
    if (h < 0) {
        h = -h;
    }
    if (!(h < 0x801)) {
    }
    func_802A70D8(vs);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.18f, vs, &a1);

        D_803EEB5A = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    func_802A6FE4(0, vs);
    if (D_80364AA8 == 0x40)
        goto done;
    D_803F77D0 = D_803EE790;
    func_802BE77C(5, vs);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    D_803EEB60 = 0;
    if (D_8035805C != 0) {
        a2 = D_803EEB4C;
        a3 = D_803EEB48;
    } else {
        a2 = D_803EEB48;
        a3 = D_803EEB4C;
    }
    func_802A768C((u8 *)D_803EE790, &D_803EEB38, &D_803EEB3C, &D_803EEB40, (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    v = vs->unk76;
    if (v >= 0) {
        if (v < 0x50) {
            v = 0x50;
        }
    } else {
        if (!(v < -0x4F)) {
            v = -0x50;
        }
    }
    vs->unk76 = -v >> 1;
    func_802B7030(vs);

done:
    D_803643E0 = D_803EEB38;
    D_803643E4 = D_803EEB3C;
    D_803643E8 = D_803EEB40;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 5, vs);
    D_803F77D0 = D_803EE790;
    func_802AC2A4(D_803EEB38, D_803EEB3C, D_803EEB40, D_80305D20, 5, vs);
    if (D_803EEB61 >= 0) {
        func_80260650(D_80367738, D_803EEB61, NULL);
    }
    engine_restore();
}

/* dust behind it on the ground, thrown to the side the bed tilts to */
REGS(gp)
void func_802B69F8(VS *vs) {
    f32 f;
    s32 t1, t2;

    ENGINE_COST(802B69F8, 78);
    if (vs->unk96[3] == 0)
        goto done;
    if (vs->unk96[2] == 1)
        goto done;
    if (!(vs->unk50 < 3))
        goto done;
    if (vs->unk9B != 0)
        goto done;
    f = D_803EEB54;
    if (f <= 0.5f) {
        f = 0.5f - f;
        f = f * 2.0f;
        f = f * 20.0f;
        t1 = engine_cvt_w_s(f);
        t2 = t1 + 0x28;
        t1 = 0x28 - t1;
    } else {
        f = f - 0.5f;
        f = f * 2.0f;
        f = f * 20.0f;
        t1 = engine_cvt_w_s(f);
        t2 = 0x28 - t1;
        t1 = t1 + 0x28;
    }
    func_8027BE7C(3, vs->unk4[6], 0x190, -0x190, -0x190, -0x190, D_803EEB38, D_803EEB40, vs->unk4E, 5, t1, t2, 0);
done:
    ;
}

/* the parts: the front wheels turn with the camera, the bed tilts with the
   C buttons, the suspension with the stick, the wheels spin with the
   speed, the cab leans with the Z/R buttons; and the engine's sound */
REGS(gp)
void func_802B6C28(VS *vs) {
    Part *p = D_803EE790;
    s32 a, s, t1;
    f32 f, g, h;

    ENGINE_COST(802B6C28, 168);
    a = camera_turn((u16)vs->unk4C);
    if (!(a < 0x1000)) {
        a -= 0xFFF;
    }
    a -= D_803EEB5C;
    if (a < 0) {
        a += 0xFFF;
    }
    if (!(a < 0x800)) {
        a -= 0x800;
    }
    a = (u32)a / 0x55;
    func_802A05A4(D_802C2208, a, 0.0f);
    func_802A05A4(D_802C226C, a, 0.0f);
    f = D_803EEB54;
    if (D_80370C15 != 0) {
        f = f - 0.05f;
        g = func_802B71DC(vs, 0);
        if (f < g) {
            f = g;
        }
    } else {
        if (D_80370C16 != 0) {
            f = f + 0.05f;
            g = func_802B71DC(vs, 1);
            if (!(f <= g)) {
                f = g;
            }
        } else {
            h = 0.5f;
            if (f < h) {
                f = f + 0.05f;
                if (!(f <= h)) {
                    f = h;
                }
            } else {
                f = f - 0.05f;
                if (f < h) {
                    f = h;
                }
            }
        }
    }
    D_803EEB54 = f;
    func_802A0360(5, 0, p, f);
    f = D_803EEB50;
    if (func_802A7CB0(0xA, vs) == 0) {
        if (D_80370C23 != 0) {
            f = f - 0.05f;
            if (f < 0.0f) {
                f = 0.0f;
            }
            goto susp;
        }
        if (D_80370C1C != 0) {
            f = f + 0.05f;
            if (!(f <= 1.0f)) {
                f = 1.0f;
            }
            goto susp;
        }
    } else {
    }
    h = 0.5f;
    if (f < h) {
        f = f + 0.03f;
        if (!(f <= h)) {
            f = h;
        }
    } else {
        f = f - 0.03f;
        if (f < h) {
            f = h;
        }
    }
susp:
    D_803EEB50 = f;
    func_802A0360(4, 0, p, f);
    s = vs->unk76;
    if (s < 0) {
        func_802A03D4(1, 1, p);
    } else {
        func_802A03D4(1, 0, p);
    }
    if (s < 0) {
        s = -s;
    }
    if (s != 0) {
        s = (u32)s / 14;
    }
    func_802A039C(1, s, p);
    func_802C4584(s);
    t1 = D_803EEB5F;
    if (D_80370C15 != 0) {
        t1 -= 5;
        if (t1 < 0) {
            t1 = 0;
        }
    } else {
        if (D_80370C16 != 0) {
            t1 += 5;
            if (!(t1 < 0x65)) {
                t1 = 0x64;
            }
        } else {
            if (t1 < 0x32) {
                t1 += 10;
                if (!(t1 < 0x33)) {
                    t1 = 0x32;
                }
            } else {
                t1 -= 10;
                if (t1 < 0x32) {
                    t1 = 0x32;
                }
            }
        }
    }
    D_803EEB5F = t1;
    func_802A0360(2, 0, p, (f32)t1 / 100.0f);
}

/* the truck's matrix, its vertices and its collision */
REGS(gp)
void func_802B7030(VS *vs) {
    u8 *model = D_803EEB44, *buf;
    s32 *m, off;

    ENGINE_COST(802B7030, 70);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EEB48 + off);
    } else {
        m = (s32 *)(D_803EEB4C + off);
    }
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EEB38, D_803EEB3C, D_803EEB40, 0x4268, m);
    if (D_8035805C != 0) {
        buf = D_803EEB48;
    } else {
        buf = D_803EEB4C;
    }
    model = D_803EEB44;
    func_8029C454(D_803EEB38, D_803EEB3C, D_803EEB40, 5, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(5, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* the turn rate: the speed / 2.6, or / 6 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B7168(VS *vs) {
    f32 d;

    ENGINE_COST(802B7168, 25);
    if (vs->unk96[0] == 1)
        goto air;
    if (vs->unk96[1] == 1)
        goto air;
    if (vs->unk96[2] == 1)
        goto air;
    d = 2.6f;
    goto div;
air:
    d = 6.0f;
div:
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the bed's tilt limit at this speed: 0.5 -/+ |speed| / 240 / 2 */
REGS(gp, s2 -> f2)
f32 func_802B71DC(VS *vs, s32 up) {
    s32 s = vs->unk76;
    f32 f;

    ENGINE_COST(802B71DC, 23);
    if (s < 0) {
        s = -s;
    }
    f = (f32)s / 240.0f * 0.5f;
    if (up != 0) {
        f = f + 0.5f;
    } else {
        f = 0.5f - f;
    }
    return f;
}

/* the camera's distance and speed for the truck, and its turn's speed */
void func_802B7240(void) {
    u16 v;

    ENGINE_COST(802B7240, 38);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 2;
    if (D_80370C1A != 0)
        goto slow;
    if (D_80370C1B != 0)
        goto slow;
    v = 0x2328;
    goto set;
slow:
    v = 0xC80;
set:
    D_803EEB58 = v;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B72DC(u8 *dst) {
    ENGINE_COST(802B72DC, 7);
    func_802AC7DC(dst, (u8 *)&D_803EEA90, (u32 *)&D_803EEB38);
}

/* and back */
void func_802B7308(u8 *src) {
    ENGINE_COST(802B7308, 11);
    func_802AC85C(src, (u8 *)&D_803EEA90, (u32 *)&D_803EEB38);
}

/* ---- 679E0's, which only the truck calls ---- */

/* the position (*x, *y, *z) kept in the level's bounds (func_802AC3B8) */
REGS(v0, v1, a0)
void func_802AC284(s32 *x, s32 *y, s32 *z) {
    ENGINE_COST(802AC284, 8);
    func_802AC3B8(x, y, z);
}

/* the level over (D_80364AA8 0x40): the parts' last frame and the
   buildings', and whatever it carries (D_803F3910..D_803F3960) thrown far
   off, with a sound; with D_8036E4C8 clear, D_803BE738 instead */
REGS(v0, v1, a0, a1, t8, gp)
void func_802AC2A4(s32 x, s32 y, s32 z, u8 *a1, s32 type, VS *vs) {
    u8 *PTR32 *q;
    u8 *o;
    s32 dx, dz;

    ENGINE_COST(802AC2A4, 12);
    if (D_80364AA8 != 0x40)
        goto done;
    func_8029A800(x, y, z, a1, 1, 0, 0, 0, type, vs);
    func_802BE77C(type, vs);
    if (D_803F3910 == D_803F3960)
        goto done;
    if (D_8036E4C8 == 0) {
        D_803BE738 = 1;
        goto done;
    }
    func_80260650(D_80367738, 0x3D, NULL);
    for (q = D_803F3910;; q += 2) {
        if (q == D_803F3960)
            break;
        o = q[0];
        *(s32 *)(o + 0x38) = 1;
        *(s32 *)(o + 0x40) = -1;
        dx = 0xBB80 - *(s32 *)(o + 0x10);
        *(s32 *)(o + 0x10) = 0xBB80;
        dz = 0xBB80 - *(s32 *)(o + 0x18);
        *(s32 *)(o + 0x18) = 0xBB80;
        func_802BD99C(o, dx, 0, dz);
    }
done:
    ;
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B6100(s32 carrier) {
    CARRY_KEEP(802B6100, 802B613C, 802B615C, 802B6168, &D_803EEA90, D_803EEB38, D_803EEB40);
}

REGS(a3)
void func_802B618C(s32 carrier) {
    CARRY_MOVE(802B618C, 802B61C8, 802B61DC, 802B61E4, 802B6244, 802B624C, 802B6278, &D_803EEA90, &D_803EEB38, &D_803EEB3C, &D_803EEB40, 5, 0x2D0, 0x2D0,
               D_803ED40B = 1, func_802B7240(), func_802B7030(&D_803EEA90));
}
