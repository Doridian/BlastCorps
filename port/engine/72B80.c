/*
 * hd_code 72B80 (us.v11 0x802B7340-0x802B9C50): the American Dream
 * (VEHICLE_HOTROD) and the BCT chopper (type 0xFE), as native C
 * (engine.h).
 *
 * The hotrod's parts are D_803EEB70, its state D_803EEE70 and its position
 * D_803EEF18..20 (vehicle.h).  func_802B7340 sets it up (from the level
 * loader), func_802B7A88 runs it each frame (from hd.c and at the end of
 * the setup); the others are hd.c's hooks for it.  It is the police car's
 * code (86F60) with its own numbers, and dust and sparks.
 *
 * The chopper's parts are D_803EEF40, its state D_803EF240 and its position
 * D_803EF2EC..F4: func_802B8480 sets it up (from the level loader),
 * func_802B899C flies it each frame (from hd.c), func_802B8794 is its
 * sound and func_802B8AE4 hands its position to the player when it has
 * landed.  func_802B8D04 is its flight, a state in D_803EF32C: 0 hovering,
 * 1 flying to D_803EF308/30C, 2 slowing down, 3 coming in to land there, 4
 * landing (D_80368030 + 0xFA0 high), 5 down (with the rotor stopping), 6
 * the rotor starting.
 *
 * func_802B78F4 and func_802B7980, at the end, are the hotrod's two
 * callbacks for 62740's carrying (shared.h).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* the hotrod's .bss (asm/data/hd_code/72B80.bss.s) */
extern Part D_803EEB70[32];
extern VS D_803EEE70;
extern s32 D_803EEF18, D_803EEF1C, D_803EEF20;  /* x, y, z */
extern u8 *PTR32 D_803EEF24;                    /* its model file */
extern u8 *PTR32 D_803EEF28;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803EEF2C;
extern u16 D_803EEF30;                          /* the heading it turns to (with D_803A7425) */
extern u8 D_803EEF32;                           /* frames until the next sparks */
extern s8 D_803EEF33;                           /* turning to it */
extern u8 D_803EEF34;                           /* frames without the gears after a hit */
/* the chopper's */
extern Part D_803EEF40[32];
extern VS D_803EF240;
extern s16 D_803EF2E6;                          /* the speed its sound was last set for */
extern SndState *PTR32 D_803EF2E8;              /* its rotor's sound, while the player is near */
extern s32 D_803EF2EC, D_803EF2F0, D_803EF2F4;  /* x, y, z */
extern u8 *PTR32 D_803EF2F8;                    /* its model file */
extern u8 *PTR32 D_803EF2FC;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EF300;
extern s32 D_803EF304;                          /* the height it flies at */
extern s32 D_803EF308, D_803EF30C;              /* where it flies to: x, z */
extern s32 D_803EF310, D_803EF314, D_803EF318;  /* where the player gets out */
extern s32 D_803EF31C;                          /* the ground's height under it */
extern s32 D_803EF320;                          /* the distance to go, last frame */
extern s16 D_803EF324;                          /* its turn, -4..4 a frame */
extern s16 D_803EF326;
extern s16 D_803EF328, D_803EF32A;              /* its speed and heading when it landed */
extern u8 D_803EF32C;                           /* its state */
extern u8 D_803EF32D;                           /* it has landed */
extern u8 D_803EF32E;                           /* the rotor has stopped */

extern u8 D_80305D30[];                         /* the hotrod's bounce records */
extern char D_80305D40[];                       /* "moving to zoom2\n" */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern Part *PTR32 D_803F77D0;
extern s32 D_803643E4, D_803643E8;
extern s32 D_80368030, D_80368044, D_80368048;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80260AB8(SndState *state, s16 type, s32 param);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_802582C4(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);
s32 func_8026A610(s32 x0, s32 z0, s32 x1, s32 z1);
void func_8026AF6C(u16 arg0);
void func_80275390(u64);
void func_8029A7E4(char *, ...);

/* 60D50.c's */
REGS(a0)
void func_802A5604(LevelHeader *level);
/* 62740's */
REGS(v0, t0, t1, t2, t8, gp, fp -> t3)
s32 func_802A9B1C(s32 i, s32 x, s32 z, s32 y, s32 self, VS *vs, s32 mat);

void func_802B7A88(void);
void func_802B78B0(void);
REGS(gp)
void func_802B7F98(VS *vs);
REGS(gp)
void func_802B80D8(VS *vs);
REGS(gp)
void func_802B8278(VS *vs);
REGS(gp -> s3)
s32 func_802B83B0(VS *vs);
REGS()
void func_802B8424(void);
void func_802B899C(void);
static void chopper_frame(void);
REGS(gp)
void func_802B8C18(VS *vs);
REGS()
void func_802B8D04(void);
REGS(-> s1+f0)
s64 func_802B988C(void);
REGS(gp)
void func_802B98E0(VS *vs);
REGS(gp)
void func_802B9B4C(VS *vs);

#define T(p) ((s32)(p))
#define P D_803EEB70
#define Q D_803EEF40

static s32 f2i(f32 f) {
    union {
        f32 f;
        s32 i;
    } u;

    u.f = f;
    return u.i;
}

/* part i's frame (func_802A04BC's v1), and its f0 */
static s32 part(s32 i, Part *parts, f32 *f0) {
    s32 f11, f12, f14, fC, fE, f13;
    f32 f4;
    s32 r = func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &f13, &f4);

    if (f0)
        *f0 = f4;
    return r;
}

/* ---- the hotrod ---------------------------------------------------------- */

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802B7340(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EEE70;
    u8 *buf;
    s16 *r;
    s32 avg;

    ENGINE_COST(802B7340, 219);
    D_803EEF24 = model;
    buf = D_80358070;
    D_803EEF28 = buf;
    D_803EEF2C = buf + 0x100;
    D_80358070 = buf + 0x200;
    func_802A1388(8, 0, D_803EEF28, D_803EEF2C, model);
    func_802A754C(vs);
    vs->unk52[0] = 0xC8;
    vs->unk52[1] = 0x15E;
    vs->unk52[2] = -0xC8;
    vs->unk52[3] = 0x15E;
    vs->unk52[4] = 0xC8;
    vs->unk52[5] = -0x15E;
    vs->unk5E[0] = 0x140;
    vs->unk5E[1] = 0x1F4;
    vs->unk5E[2] = -0x140;
    vs->unk5E[3] = 0x1F4;
    vs->unk5E[4] = 0x140;
    vs->unk5E[5] = -0x1F4;
    D_803EEF18 = x;
    D_803EEF1C = y;
    D_803EEF20 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803EEF1C, x, z, vs->unk4, &D_803EEF1C, (s16 *)&vs->unk4C, 8, vs, 0, &avg);
    func_8029F85C(P, D_803EEF24, D_803EEF28, D_803EEF2C);
    func_802A039C(0, 100, P);
    func_802A03D4(0, 0, P);
    func_802A040C(0, 0, P);
    func_802A0480(0, 0, P, 0.0f);
    func_802A0290(0, 1, P);
    func_8029E558(P, D_803EEF28, D_803EEF2C);
    func_802A0320(0, P);
    func_802A0290(0, 1, P);
    func_8029E558(P, D_803EEF2C, D_803EEF28);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0x50, r[5] = 4;
    r[6] = 0x50, r[7] = 0xA0, r[8] = 5;
    r[9] = 0xA0, r[10] = 0xFA, r[11] = 3;
    r[12] = 0xFA, r[13] = 0x15E, r[14] = 2;
    D_803EEF32 = 0;
    D_803EEF33 = 0;
    D_803EEF34 = 0;
    model = D_803EEF24;
    func_8029C354(8, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x32C8);
    func_80258230(8, 0x3C, 0x19, 0x19);
    vs->unk9A = 1;
    func_802B7A88();
    vs->unk9A = 0;
    model = D_803EEF24;
    func_802AA838(D_803EEF2C, D_803EEF28, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
}

/* hd.c's: the player gets in */
void func_802B76AC(void) {
    VS *vs = &D_803EEE70;

    ENGINE_COST(802B76AC, 19);
    vs->unk96[3] = 0;
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xCE);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B76F8(void) {
    VS *vs = &D_803EEE70;
    u8 r = 0;

    ENGINE_COST(802B76F8, 23);
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
void func_802B7754(void) {
    VS *vs = &D_803EEE70;

    ENGINE_COST(802B7754, 19);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EEF28, (u32 *)D_803EEF2C, 0x100);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802B77A0(void) {
    VS *vs = &D_803EEE70;

    ENGINE_COST(802B77A0, 35);
    func_802A9A60(vs->unk52, D_803EEF1C, D_803EEF18, D_803EEF20, vs->unk4, &D_803EEF1C, (s16 *)&vs->unk4C, 8, vs,
                  0);
    func_802B8278(vs);
    func_802A133C(D_803EEF18, D_803EEF1C, D_803EEF20, 8, vs);
}

/* its light */
void func_802B78B0(void) {
    ENGINE_COST(802B78B0, 17);
    func_802ABD54(8, D_803EEF18, D_803EEF1C, D_803EEF20);
}

/* each frame */
void func_802B7A88(void) {
    VS *vs = &D_803EEE70;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_COST(802B7A88, 213);
    func_802B78B0();
    func_802A75DC((u8 *)P, &D_803EEF18, &D_803EEF1C, &D_803EEF20, (u8 *)vs);
    func_802C4724(0xA4);
    if (vs->unk9A == 0) {
        func_802B7F98(vs);
    }
    if (D_80367BFF != 0) {
        func_802CB690(vs);
    }
    func_802B8424();
    rate_i = func_802B83B0(vs);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    if (D_803EEF34 == 0) {
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x10, vs, &t3);
    } else {
        D_803EEF34--;
    }
    func_802A7FD8(0x1770, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 1, 8, (s8 *)vs->unk96, vs->unk4, 700.0f, vs);
    if (D_803EEF33 != 0) {
        func_802A7070((s16 *)&D_803EEF30, vs);
    }
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EEF18, &D_803EEF20, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EEF18, &D_803EEF20, &D_803EEF1C, 8, 0x2BC, 0x190, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    if (D_8035805C != 0) {
        func_8029E558(P, D_803EEF28, D_803EEF2C);
    } else {
        func_8029E558(P, D_803EEF2C, D_803EEF28);
    }
    func_802B8278(vs);
    ENGINE_LEAVE(8, 7);         /* $t0 and $t2: func_8029A800 (56040.c) takes them from the context */
    ENGINE_LEAVE(10, 0x96);
    func_8029A800(D_803EEF18, D_803EEF1C, D_803EEF20, D_80305D30, 1, 1, vs->unk76, 0, 8, vs);
    func_8029C52C(8, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = P;
        func_802BE77C(8, vs);
        if (D_803A7424 == 0) {
            D_803EEF33 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    func_8029A914(vs);
    D_803EEF33 = 1;
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
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.16f, vs, &a1);

        D_803EEF30 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    func_802A6FE4(0, vs);
    D_803F77D0 = P;
    func_802BE77C(8, vs);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    D_803EEF33 = 0;
    if (D_8035805C != 0) {
        a2 = D_803EEF2C;
        a3 = D_803EEF28;
    } else {
        a2 = D_803EEF28;
        a3 = D_803EEF2C;
    }
    func_802A768C((u8 *)P, &D_803EEF18, &D_803EEF1C, &D_803EEF20, (u32 *)a2, (u32 *)a3, 0x100, (u8 *)vs);
    D_803EEF34 = 5;
    v = vs->unk76;
    if (v >= 0) {
        if (v < 0x32) {
            v = 0x32;
        }
    } else {
        if (!(v < -0x31)) {
            v = -0x32;
        }
    }
    vs->unk76 = -v >> 1;
    func_802B8278(vs);

done:
    D_803643E0 = D_803EEF18;
    D_803643E4 = D_803EEF1C;
    D_803643E8 = D_803EEF20;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 8, vs);
}

/* the dust, the wheels' sparks off rough ground, and the engine's sound */
REGS(gp)
void func_802B7F98(VS *vs) {
    s32 s;

    ENGINE_COST(802B7F98, 23);
    func_802B80D8(vs);
    if (D_803EEF32 != 0) {
        D_803EEF32--;
        goto sound;
    }
    if (vs->unk96[3] == 0)
        goto sound;
    D_803EEF32 = 1;
    s = func_802A5ED0();
    if (!(s < 0xF))
        goto sound;
    func_802A6274(T(D_802C2954), 0x29810, 1, 8, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802A6274(T(D_802C2954), 0x29810, 1, 8, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 8, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 8, 4, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
sound:
    s = vs->unk76;
    if (s < 0) {
        s = -s;
    }
    func_802C4584((u32)s >> 5);
}

/* dust behind it on the ground */
REGS(gp)
void func_802B80D8(VS *vs) {
    ENGINE_COST(802B80D8, 69);
    if (vs->unk96[3] == 0)
        goto done;
    if (vs->unk96[2] == 1)
        goto done;
    if (!(vs->unk50 < 3))
        goto done;
    if (vs->unk9B != 0)
        goto done;
    func_8027BE7C(3, vs->unk4[6], 0xFA, -0x190, -0x190, -0x190, D_803EEF18, D_803EEF20, vs->unk4E, 3, 0x32, 0x32, 0);
done:
    ;
}

/* the hotrod's matrix, its vertices and its collision */
REGS(gp)
void func_802B8278(VS *vs) {
    u8 *model = D_803EEF24, *buf;
    s32 *m, off;

    ENGINE_COST(802B8278, 70);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EEF28 + off);
    } else {
        m = (s32 *)(D_803EEF2C + off);
    }
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EEF18, D_803EEF1C, D_803EEF20, 0x32C8, m);
    if (D_8035805C != 0) {
        buf = D_803EEF28;
    } else {
        buf = D_803EEF2C;
    }
    model = D_803EEF24;
    func_8029C454(D_803EEF18, D_803EEF1C, D_803EEF20, 8, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(8, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* the turn rate: the speed / 3.6, or / 11 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B83B0(VS *vs) {
    f32 d;

    ENGINE_COST(802B83B0, 24);
    if (vs->unk96[0] == 1)
        goto air;
    if (vs->unk96[1] == 1)
        goto air;
    if (vs->unk96[2] == 1)
        goto air;
    d = 3.6f;
    goto div;
air:
    d = 11.0f;
div:
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for the hotrod */
REGS()
void func_802B8424(void) {
    ENGINE_COST(802B8424, 23);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 3;
}

/* ---- the chopper --------------------------------------------------------- */

/* set up: from the level loader (func_802A3134), with the model file in
   $s2; its position is the level's (D_803EF2EC..F4, 1D990.c) */
REGS(s2)
void func_802B8480(u8 *model) {
    VS *vs = &D_803EF240;
    u8 *buf;

    ENGINE_COST(802B8480, 197);
    D_803EF2F8 = model;
    buf = D_80358070;
    D_803EF2FC = buf;
    D_803EF300 = buf + 0x800;
    D_80358070 = buf + 0x1000;
    /* ($a1: what func_802A396C left, the heap's top, never 0) */
    func_802A1388(0xFE, 1, D_803EF2FC, D_803EF300, model);
    vs->unk4C = 0;
    vs->unk4E = 0;
    D_803EF32A = 0;
    vs->unk76 = 0x14;
    D_803EF2E6 = 0;
    D_803EF328 = 0;
    func_8029F85C(Q, D_803EF2F8, D_803EF2FC, D_803EF300);
    func_802A039C(0, 100, Q);
    func_802A03D4(0, 0, Q);
    func_802A040C(0, 0, Q);
    func_802A0480(0, 0, Q, 0.0f);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, D_803EF2FC, D_803EF300);
    func_802A0320(0, Q);
    func_802A0290(0, 1, Q);
    func_8029E558(Q, D_803EF300, D_803EF2FC);
    func_802A039C(1, 9, Q);
    func_802A03D4(1, 0, Q);
    func_802A040C(1, 0, Q);
    func_802A0290(1, -1, Q);
    func_802A0290(2, -1, Q);
    func_802A0290(3, -1, Q);
    func_802A039C(4, 1, Q);
    func_802A03D4(4, 1, Q);
    func_802A040C(4, 1, Q);
    func_802A0290(4, 1, Q);
    D_803EF32C = 0;
    D_803EF324 = 0;
    D_803EF32D = 0;
    D_803EF31C = 0;
    func_80258230(0xFE, 0x78, 0x2D, 0x2D);
    chopper_frame();
    model = D_803EF2F8;
    func_802AA838(D_803EF300, D_803EF2FC, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
}

/* its rotor's sound: on within 0x3E80 of the player, louder nearer, panned
   by x */
void func_802B8794(void) {
    s32 d, dx, v;

    ENGINE_COST(802B8794, 94);
    dx = D_803643E0 - D_803EF2EC;
    d = (s32)func_802ABCDC(D_803643E0, D_803643E4, D_803643E8, D_803EF2EC, D_803EF2F0, D_803EF2F4);
    if (!(d < 0x3E81)) {
        if (D_803EF2E8 == NULL)
            goto done;
        func_802608C8(D_803EF2E8);
        D_803EF2E8 = NULL;
        goto done;
    }
    if (D_803EF2E8 == NULL) {
        func_80260650(D_80367738, 0x13, &D_803EF2E8);
    }
    d -= 0x1F4;
    if (d < 0) {
        d = 0;
    }
    func_80260AB8(D_803EF2E8, 8, 0x7FFF - d);
    v = 0x40 + (dx >> 5);
    if (v < 0) {
        v = 0;
    } else {
        if (!(v < 0x80)) {
            v = 0x7F;
        }
    }
    func_80260AB8(D_803EF2E8, 4, v);
done:
    ;
}

/* each frame: its flight, its rotors, its matrix, where the player would
   get out, and its shadow */
static void chopper_frame(void) {
    VS *vs = &D_803EF240;       /* (the $gp func_802B8D04 sets) */
    s32 *p;

    ENGINE_COST(802B899C, 76);
    /* (its $fp as it found it: 69BB0.c's driver reads it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802B8D04();
    func_802B98E0(vs);
    if (D_8035805C != 0) {
        func_8029E558(Q, D_803EF2FC, D_803EF300);
    } else {
        func_8029E558(Q, D_803EF300, D_803EF2FC);
    }
    func_802B9B4C(vs);
    p = (s32 *)func_802ABC88(0xFE, 1);
    D_803EF310 = p[0];
    D_803EF314 = p[1];
    D_803EF318 = p[2];
    func_802B8C18(vs);
    engine_restore();
}

/* hd.c's: the chopper each frame */
void func_802B899C(void) {
    chopper_frame();
}

/* hd.c's, when it has landed: the player's start where it is, and the
   level's end */
void func_802B8AE4(void) {
    VS *vs = &D_803EF240;
    u8 s;

    ENGINE_COST(802B8AE4, 58);
    D_803EF32A = vs->unk4E;
    D_803EF328 = vs->unk76;
    D_803ED808 = D_803EF310;
    D_803ED80C = D_803EF314;
    D_803ED810 = D_803EF318;
    D_80368030 = D_803EF31C;
    s = D_803EF32C;
    if (D_80364A90 != 0x1000) {
        if (s != 4)
            goto done;
        D_80364A98 = 0x1000;
        goto done;
    }
    if (s != 5) {
        if (D_803EF31C < D_803EF314)
            goto done;
    }
    func_80275390(0x2000);
done:
    ;
}

/* the ground's height under it (inside the level), and its shadow there */
REGS(gp)
void func_802B8C18(VS *vs) {
    /* (the original's $t3, the shadow's height outside the level: the
       model's address func_802B9B4C leaves there) */
    s32 x = D_803EF2EC, z = D_803EF2F4, t3 = T(D_803EF2F8);

    ENGINE_COST(802B8C18, 59);
    if (x <= 0)
        goto shadow;
    if (z <= 0)
        goto shadow;
    if (!(x < D_803BE732 << 5))
        goto shadow;
    if (!(z < D_803BE736 << 5))
        goto shadow;
    t3 = func_802A9B1C(0, x, z, D_803EF31C, 0xFE, vs, 0);
    D_803EF31C = t3;
shadow:
    func_802582C4(0xFE, D_803EF2EC, t3, D_803EF2F4, D_803EF2F0, 0, 0, (s16)vs->unk4C);
    D_803EF326 = vs->unk4C;
}

/* its flight (see the top) */
REGS()
void func_802B8D04(void) {
    VS *vs = &D_803EF240;
    s32 t0, t1, t2, t3, t4, t5, t6, t7, s0, s6, xr, zr, a, b, c, d;
    s64 s1;
    u32 u;
    f32 f0;

    ENGINE_COST(802B8D04, 85);
    t0 = D_803EF32C;
    if (t0 == 0)
        goto hover;
    if (t0 == 1)
        goto fly;
    if (t0 == 3)
        goto come_in;
    if (t0 == 2)
        goto slow;
    if (t0 == 4)
        goto land;
    if (t0 == 5)
        goto down;
    if (t0 != 6) {
        engine_trap(0x802B8D58);
    }

    /* 6: the rotor starting */
    part(4, Q, &f0);
    if (!(f0 == 0.0f)) {
        func_802A03D4(4, 1, Q);
        func_802A0290(4, 1, Q);
    }
    D_803EF32C = 0;
    goto hover;

down:
    /* 5: down, the rotor stopping; up to the flying height again */
    t3 = part(4, Q, NULL);
    if (t3 != 1) {
        D_803EF32E = 1;
    }
    t3 = D_803EF304;
    t5 = D_803EF2F0;
    if (t3 != t5) {
        if (t3 < t5) {
            t5 -= 0x14;
            if (t5 < t3) {
                t5 = t3;
            }
        } else {
            t5 += 0x14;
            if (t3 < t5) {
                t5 = t3;
            }
        }
        D_803EF2F0 = t5;
        goto rest;
    }
    if (D_803EF32E == 0)
        goto rest;
    D_803EF32C = 0;
    D_803EF32E = 0;
    goto rest;

land:
    /* 4: down to D_80368030 + 0xFA0 */
    t3 = D_80368030 + 0xFA0;
    t5 = D_803EF2F0;
    if (t3 != t5) {
        if (t3 < t5) {
            t5 -= 0x14;
            if (t5 < t3) {
                t5 = t3;
            }
        } else {
            t5 += 0x14;
            if (t3 < t5) {
                t5 = t3;
            }
        }
        D_803EF2F0 = t5;
        goto rest;
    }
    t3 = part(4, Q, NULL);
    if (t3 == 1)
        goto rest;
    D_803EF32D = 1;
    if (D_80364A90 != 0x1000) {
        t0 = func_8026A610(D_803643E0, D_803643E0, D_803EF2EC, D_803EF2F4);
        t0 = 0x88B8 - (t0 << 1);
        if (!(t0 < 0xFA1)) {
            if (!(t0 < 0x8000)) {
                t0 = 0x7FFF;
            }
            {
                SndState *h = func_80260650(D_80367738, 0x28, NULL);

                func_80260AB8(h, 8, t0);
            }
        }
    }
    D_803EF32C = 5;
    func_802A0290(4, 1, Q);
    goto rest;

slow:
    /* 2: slowing down to 0x14, then coming in */
    t3 = vs->unk76;
    if (!(t3 < 0x15)) {
        vs->unk76 = t3 - 4;
        goto turn;
    }
    vs->unk76 = 0x14;
    D_803EF32C = 3;
    vs->unk76 = 0x14;
    s1 = func_802B988C();
    D_803EF320 = (s32)s1;

come_in:
    /* 3: straight to D_803EF308/30C, at the speed, until it is there or
       the distance stops shrinking; then landing */
    t4 = D_803EF324;
    if (t4 >= 0) {
        t4--;
        if (t4 < 0) {
            t4 = 0;
        }
    } else {
        t4++;
        if (t4 > 0) {
            t4 = 0;
        }
    }
    D_803EF324 = t4;
    s1 = func_802B988C();
    if (s1 == 0)
        goto arrived;
    t6 = D_803EF2EC;
    t4 = D_803EF308;
    t5 = (s32)s1;
    t2 = vs->unk76;
    t4 = t4 - t6;
    t7 = t4;
    if (t4 < 0) {
        t7 = -t4;
    }
    u = ((u32)t7 << 10) / (u32)t5;
    t7 = (u32)(u * (u32)t2) >> 10;
    if (t4 < 0) {
        t7 = -t7;
    }
    t6 += t7;
    D_803EF2EC = t6;
    t6 = D_803EF2F4;
    t4 = D_803EF30C;
    t5 = (s32)s1;
    t4 = t4 - t6;
    t7 = t4;
    if (t4 < 0) {
        t7 = -t4;
    }
    u = ((u32)t7 << 10) / (u32)t5;
    t7 = (u32)(u * (u32)t2) >> 10;
    if (t4 < 0) {
        t7 = -t7;
    }
    t6 += t7;
    D_803EF2F4 = t6;
    s1 = func_802B988C();
    if (s1 < D_803EF320) {
        D_803EF320 = (s32)s1;
        goto rest;
    }
arrived:
    D_803EF2EC = D_803EF308;
    D_803EF2F4 = D_803EF30C;
    D_803EF32C = 4;
    func_802A0290(4, 1, Q);
    goto rest;

hover:
    /* 0: the turn back to 4 a frame */
    t4 = D_803EF324;
    if (!(t4 < 4)) {
        t4--;
        if (t4 < 4) {
            t4 = 4;
        }
    } else {
        t4++;
        if (!(t4 < 5)) {
            t4 = 4;
        }
    }
    D_803EF324 = t4;
    goto turn;

fly:
    /* 1: toward D_803EF308/30C, speeding up to 0xA0 and turning to it,
       until within 0x7D0; near enough on its flight to the next level,
       the next stop is the level's (D_80368044/48) */
    s1 = func_802B988C();
    t3 = D_803EF2EC;
    t5 = D_803EF2F4;
    t6 = D_803EF308;
    s0 = D_803EF30C;
    if (D_80364A90 == 0x800) {
        t4 = D_802E8BDC;
        if (t4 == 0x1A)
            goto far;
        if (t4 == 4)
            goto far;
        if (t4 == 0x1D)
            goto farther;
        if (t4 == 0x3A)
            goto farther;
        if (t4 == 0xD)
            goto farther;
        t4 = 0x7530;
        goto near;
    far:
        t4 = 0xBB80;
        goto near;
    farther:
        t4 = 0x11170;
    near:
        if (!(t4 < s1)) {
            D_80364A98 = 1;
            func_8029A7E4(D_80305D40);
            D_803EF308 = D_80368044;
            t3 = D_80368048;
            D_803EF30C = t3;
            func_8026AF6C(0x4000);
        }
    }
    if (s1 < 0x7D0) {
        D_803EF32C = 2;
        goto turn;
    }
    t0 = vs->unk76 + 2;
    if (!(t0 < 0xA1)) {
        t0 = 0xA0;
    }
    /* the heading to it: func_802ABB1C's angle from (D_803EF308, 30C) to
       where it is (by the quadrant), checked against the reverse */
    t7 = t3;
    t1 = s0;
    t3 = t6;
    s0 = t5;
    vs->unk76 = t0;
    t4 = t1;
    s6 = func_802ABB1C(t3, t4, 0, (s32)s1, t7, s0);
    if (!(s6 < 0x401)) {
        s6 = func_802ABB1C(t3, t4, (s32)s1, 0, t7, s0);
        s6 += 0x400;
        if (!(s6 < 0x801)) {
            s6 = func_802ABB1C(t3, t4, 0, -(s32)s1, t7, s0);
            s6 += 0x800;
        }
    }
    func_802ACE38(0, s1, s6, &xr, &zr);
    a = xr + t7;
    b = zr + s0;
    func_802ACE38(0, s1, 0xFFF - s6, &xr, &zr);
    c = xr + t7;
    d = zr + s0;
    a -= t3;
    b -= t4;
    c -= t3;
    d -= t4;
    if (a < 0) {
        a = -a;
    }
    if (b < 0) {
        b = -b;
    }
    a += b;
    if (c < 0) {
        c = -c;
    }
    if (d < 0) {
        d = -d;
    }
    c += d;
    if (!(a < c)) {
        s6 = 0xFFF - s6;
    }
    /* turn toward it: up to (the difference >> 6) a frame */
    t0 = (u16)vs->unk4E - s6;
    t3 = t0;
    if (t3 >= 0) {
        if (!(t3 < 0x800)) {
            t3 = 0xFFF - t3;
        }
    } else {
        if (t3 < -0x7FF) {
            t3 += 0xFFF;
        } else {
            t3 = -t3;
        }
    }
    t3 = (u32)t3 >> 6;
    t4 = -t3;
    if (t0 > 0) {
        if (!(t0 < 0x800))
            goto right;
    } else {
        if (!(t0 < -0x800))
            goto right;
    }
    t2 = D_803EF324 - 1;
    if (t2 < t4) {
        t2 = t4;
    }
    D_803EF324 = t2;
    goto turn;
right:
    t2 = D_803EF324 + 1;
    if (t3 < t2) {
        t2 = t3;
    }
    D_803EF324 = t2;

turn:
    /* the heading by the turn, and on at the speed */
    t4 = (u16)vs->unk4E + D_803EF324;
    if (!(t4 < 0x1000)) {
        t4 -= 0xFFF;
    } else {
        if (t4 < 0) {
            t4 += 0xFFF;
        }
    }
    vs->unk4E = t4;
    vs->unk4C = t4;
    t0 = func_802A860C(t4, &vs->unk76, &D_803EF2EC, &D_803EF2F4, 0.0f, &t1);
    D_803EF2EC = t0;
    D_803EF2F4 = t1;

rest:
    /* but landing or down, the level's ground, and its height toward
       D_803EF304 by 0x14 a frame */
    t0 = D_803EF32C;
    if (t0 == 4)
        goto done;
    if (t0 == 5)
        goto done;
    func_802A5604(D_80358074);
    t1 = D_803EF2F0;
    t3 = D_803EF304;
    if (t3 == t1)
        goto done;
    if (t3 < t1) {
        t1 -= 0x14;
        if (t1 < t3) {
            t1 = t3;
        }
    } else {
        t1 += 0x14;
        if (t3 < t1) {
            t1 = t3;
        }
    }
    D_803EF2F0 = t1;
done:
    ;
}

/* how far it is from D_803EF308/30C (x and z) */
REGS(-> s1+f0)
s64 func_802B988C(void) {
    s64 d;

    ENGINE_COST(802B988C, 21);
    d = func_802ABCDC(D_803EF2EC, 0, D_803EF2F4, D_803EF308, 0, D_803EF30C);
    return d;
}

/* its sound's pitch by its speed, its tail's tilt (part 2) by its speed and
   its body's lean (part 3) by its turn */
REGS(gp)
void func_802B98E0(VS *vs) {
    s32 t0, a0;
    f32 f20, f;

    ENGINE_COST(802B98E0, 118);
    t0 = 0xA0 - vs->unk76;
    f20 = (f32)t0 / (f32)0x140;
    a0 = D_803EF2E6;
    D_803EF2E6 = t0;
    if (a0 == t0)
        goto tilt;
    if (D_803EF2E8 == NULL)
        goto tilt;
    func_80260AB8(D_803EF2E8, 0x10, f2i(0.8f + (f32)t0 * -0.004f));
tilt:
    func_802A0360(2, 0, Q, f20);
    t0 = D_803EF324;
    if (t0 == 1)
        goto zero;
    if (t0 != -1)
        goto lean;
zero:
    t0 = 0;
lean:
    if (t0 >= 0) {
        f = (f32)(0x20 - t0) / (f32)0x40;
        func_802A0360(3, 0, Q, f);
    } else {
        if (t0 < 0) {
            t0 = -t0;
        }
        f = (f32)t0 / (f32)0x40 + 0.5f;
        func_802A0360(3, 0, Q, f);
    }
}

/* the chopper's matrix and its collision */
REGS(gp)
void func_802B9B4C(VS *vs) {
    u8 *model = D_803EF2F8, *buf;
    s32 *m, off;

    ENGINE_COST(802B9B4C, 55);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EF2FC + off);
    } else {
        m = (s32 *)(D_803EF300 + off);
    }
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EF2EC, D_803EF2F0, D_803EF2F4, 0x5208, m);
    if (D_8035805C != 0) {
        buf = D_803EF2FC;
    } else {
        buf = D_803EF300;
    }
    model = D_803EF2F8;
    /* $t8, which 69BB0.c's driver reads from the context */
    ENGINE_LEAVE(24, T(m));
    /* The driver's shadow (69BB0.c's func_802AF340, which hd.c runs next at
       the same depth) takes a word of dead stack as its tilt: the $a1 the
       original func_802ABBEC saves in its frame (0x28) under this one (8),
       chopper_frame's (0x88) and the 16 the glue would have left the C. */
    engine_frame(-(ENGINE_C_FRAME + ENGINE_FRAME_S + 8 + 0x28));
    engine_frame_sd(8, 5);
    engine_frame(ENGINE_C_FRAME + ENGINE_FRAME_S + 8 + 0x28);
    func_802ABBEC(0xFE, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B78F4(s32 carrier) {
    CARRY_KEEP(802B78F4, 802B7930, 802B7950, 802B795C, &D_803EEE70, D_803EEF18, D_803EEF20);
}

REGS(a3)
void func_802B7980(s32 carrier) {
    CARRY_MOVE(802B7980, 802B79BC, 802B79D0, 802B79D8, 802B7A38, 802B7A40, 802B7A6C, &D_803EEE70, &D_803EEF18, &D_803EEF1C, &D_803EEF20, 8, 0x2BC, 0x190,
               D_803ED40B = 1, func_802B8424(), func_802B8278(&D_803EEE70));
}
