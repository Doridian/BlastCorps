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
 * Still translated: func_802B78F4 and func_802B7980, what 62740's collision
 * dispatch calls for the hotrod; they go native with that.
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

/* translated */
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
    s32 *s3, avg;

    ENGINE_BLK(802B7340);
    engine_save(ENGINE_T0_T5, 0);
    D_803EEF24 = model;
    buf = D_80358070;
    D_803EEF28 = buf;
    D_803EEF2C = buf + 0x100;
    D_80358070 = buf + 0x200;
    func_802A1388(8, 0, D_803EEF28, D_803EEF2C, model);
    ENGINE_BLK(802B73C0);
    func_802A754C(vs);
    ENGINE_BLK(802B73CC);
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
    s3 = func_802A992C(vs->unk52, D_803EEF1C, x, z, vs->unk4, &D_803EEF1C, (s16 *)&vs->unk4C, 8, vs, engine_ctx(30),
                       &avg);
    ENGINE_LEAVE(19, T(s3));
    ENGINE_LEAVE(21, avg);
    ENGINE_BLK(802B747C);
    func_8029F85C(P, D_803EEF24, D_803EEF28, D_803EEF2C);
    ENGINE_BLK(802B74B8);
    func_802A039C(0, 100, P);
    ENGINE_BLK(802B74CC);
    func_802A03D4(0, 0, P);
    ENGINE_BLK(802B74E0);
    func_802A040C(0, 0, P);
    ENGINE_BLK(802B74F4);
    func_802A0480(0, 0, P, 0.0f);
    ENGINE_BLK(802B750C);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802B7520);
    func_8029E558(P, D_803EEF28, D_803EEF2C);
    ENGINE_BLK(802B7534);
    func_802A0320(0, P);
    ENGINE_BLK(802B7544);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802B7558);
    func_8029E558(P, D_803EEF2C, D_803EEF28);
    ENGINE_BLK(802B756C);
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
    ENGINE_BLK(802B7628);
    func_80258230(8, 0x3C, 0x19, 0x19);
    ENGINE_BLK(802B7640);
    vs->unk9A = 1;
    func_802B7A88();
    ENGINE_BLK(802B7650);
    vs->unk9A = 0;
    model = D_803EEF24;
    func_802AA838(D_803EEF2C, D_803EEF28, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802B7688);
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(17, T(vs->unk4));
    ENGINE_LEAVE(18, T(&D_803EEF1C));
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(22, T(D_803EEF28));
    ENGINE_LEAVE(23, T(D_803EEF2C));
}

/* hd.c's: the player gets in */
void func_802B76AC(void) {
    VS *vs = &D_803EEE70;

    ENGINE_BLK(802B76AC);
    vs->unk96[3] = 0;
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xCE);
    ENGINE_BLK(802B76E8);
    ENGINE_LEAVE(28, T(vs));
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B76F8(void) {
    VS *vs = &D_803EEE70;
    u8 r = 0;

    ENGINE_BLK(802B76F8);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802B771C);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802B772C);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802B773C);
                r = 1;
            }
        }
    }
    ENGINE_BLK(802B7740);
    return r;
}

/* hd.c's: the player gets out */
void func_802B7754(void) {
    VS *vs = &D_803EEE70;

    ENGINE_BLK(802B7754);
    engine_save(0x10000000, 0);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EEF28, (u32 *)D_803EEF2C, 0x100);
    ENGINE_BLK(802B7784);
    func_802C444C();
    ENGINE_BLK(802B778C);
    engine_restore();
}

/* hd.c's: put back on the ground where it is */
void func_802B77A0(void) {
    VS *vs = &D_803EEE70;

    ENGINE_BLK(802B77A0);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802A9A60(vs->unk52, D_803EEF1C, D_803EEF18, D_803EEF20, vs->unk4, &D_803EEF1C, (s16 *)&vs->unk4C, 8, vs,
                  engine_ctx(30));
    ENGINE_BLK(802B782C);
    func_802B8278(vs);
    ENGINE_BLK(802B7834);
    func_802A133C(D_803EEF18, D_803EEF1C, D_803EEF20, 8, vs);
    ENGINE_BLK(802B7860);
    engine_restore();
}

/* its light */
void func_802B78B0(void) {
    ENGINE_BLK(802B78B0);
    func_802ABD54(8, D_803EEF18, D_803EEF1C, D_803EEF20);
    ENGINE_BLK(802B78E4);
}

/* each frame */
void func_802B7A88(void) {
    VS *vs = &D_803EEE70;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802B7A88);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802B78B0();
    ENGINE_BLK(802B7ADC);
    func_802A75DC((u8 *)P, &D_803EEF18, &D_803EEF1C, &D_803EEF20, (u8 *)vs);
    ENGINE_BLK(802B7B08);
    func_802C4724(0xA4);
    ENGINE_BLK(802B7B10);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802B7B1C);
        func_802B7F98(vs);
    }
    ENGINE_BLK(802B7B24);
    if (D_80367BFF != 0) {
        ENGINE_BLK(802B7B34);
        func_802CB690(vs);
    }
    ENGINE_BLK(802B7B3C);
    func_802B8424();
    ENGINE_BLK(802B7B44);
    rate_i = func_802B83B0(vs);
    ENGINE_BLK(802B7B4C);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    ENGINE_BLK(802B7B68);
    if (D_803EEF34 == 0) {
        ENGINE_BLK(802B7B78);
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x10, vs, &t3);
        ENGINE_BLK(802B7B80);
    } else {
        ENGINE_BLK(802B7B88);
        D_803EEF34--;
    }
    ENGINE_BLK(802B7B94);
    func_802A7FD8(&vs->unk74, 0x1770, &vs->unk76, &vs->unk4C, &vs->unk4E, &vs->unk96[3], 1, vs);
    ENGINE_BLK(802B7BAC);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802B7BB8);
    func_802A843C(&vs->unk76, 8, vs->unk96, vs->unk4, 1, 700.0f, vs);
    ENGINE_BLK(802B7BCC);
    if (D_803EEF33 != 0) {
        ENGINE_BLK(802B7BDC);
        func_802A7070((s16 *)&D_803EEF30, vs);
    }
    ENGINE_BLK(802B7BE8);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EEF18, &D_803EEF20, rate, &z);
    ENGINE_BLK(802B7C00);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EEF18, &D_803EEF20, &D_803EEF1C, 8, 0x2BC, 0x190, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802B7C38);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B7C60);
        func_8029E558(P, D_803EEF28, D_803EEF2C);
        ENGINE_BLK(802B7C74);
    } else {
        ENGINE_BLK(802B7C7C);
        func_8029E558(P, D_803EEF2C, D_803EEF28);
    }
    ENGINE_BLK(802B7C90);
    func_802B8278(vs);
    ENGINE_BLK(802B7C98);
    ENGINE_LEAVE(8, 7);         /* $t0 and $t2, which func_8029A800 reads too */
    ENGINE_LEAVE(10, 0x96);
    func_8029A800(D_803EEF18, D_803EEF1C, D_803EEF20, D_80305D30, 1, 1, vs->unk76, 0, 8, vs);
    ENGINE_BLK(802B7CE4);
    func_8029C52C(8, vs);
    ENGINE_BLK(802B7CEC);
    func_8029AA10();
    ENGINE_BLK(802B7CF4);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802B7D04);
        D_803A7424 = 0;
        D_803F77D0 = P;
        func_802BE77C(8, vs);
        ENGINE_BLK(802B7D24);
        if (D_803A7424 == 0) {
            ENGINE_BLK(802B7D34);
            ENGINE_BLK(802B7EC0);
            D_803EEF33 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    ENGINE_BLK(802B7D3C);
    func_8029A914(vs);
    ENGINE_BLK(802B7D44);
    D_803EEF33 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802B7D54);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802B7D64);
        h += 0xFFF;
    }
    ENGINE_BLK(802B7D68);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802B7D74);
        h = -h;
    }
    ENGINE_BLK(802B7D78);
    if (!(h < 0x801)) {
        ENGINE_BLK(802B7D84);
    }
    ENGINE_BLK(802B7D8C);
    ENGINE_BLK(802B7E64);
    func_802A70D8(vs);
    ENGINE_BLK(802B7E6C);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.16f, &a1);

        ENGINE_BLK(802B7E80);
        D_803EEF30 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802B7E94);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802B7E9C);
    D_803F77D0 = P;
    func_802BE77C(8, vs);
    ENGINE_BLK(802B7EB8);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    ENGINE_BLK(802B7DA0);
    D_803EEF33 = 0;
    if (D_8035805C != 0) {
        ENGINE_BLK(802B7DB8);
        a2 = D_803EEF2C;
        a3 = D_803EEF28;
    } else {
        ENGINE_BLK(802B7DD4);
        a2 = D_803EEF28;
        a3 = D_803EEF2C;
    }
    ENGINE_BLK(802B7DEC);
    func_802A768C((u8 *)P, &D_803EEF18, &D_803EEF1C, &D_803EEF20, (u32 *)a2, (u32 *)a3, 0x100, (u8 *)vs);
    ENGINE_BLK(802B7E14);
    D_803EEF34 = 5;
    v = vs->unk76;
    if (v >= 0) {
        ENGINE_BLK(802B7E2C);
        if (v < 0x32) {
            ENGINE_BLK(802B7E34);
            v = 0x32;
        }
    } else {
        ENGINE_BLK(802B7E3C);
        if (!(v < -0x31)) {
            ENGINE_BLK(802B7E48);
            v = -0x32;
        }
    }
    ENGINE_BLK(802B7E4C);
    vs->unk76 = -v >> 1;
    func_802B8278(vs);
    ENGINE_BLK(802B7E5C);

done:
    ENGINE_BLK(802B7EC8);
    D_803643E0 = D_803EEF18;
    D_803643E4 = D_803EEF1C;
    D_803643E8 = D_803EEF20;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 8, vs);
    ENGINE_BLK(802B7F44);
    engine_restore();
}

/* the dust, the wheels' sparks off rough ground, and the engine's sound */
REGS(gp)
void func_802B7F98(VS *vs) {
    s32 s;

    ENGINE_BLK(802B7F98);
    func_802B80D8(vs);
    ENGINE_BLK(802B7FA8);
    if (D_803EEF32 != 0) {
        ENGINE_BLK(802B7FBC);
        D_803EEF32--;
        goto sound;
    }
    ENGINE_BLK(802B7FC8);
    if (vs->unk96[3] == 0)
        goto sound;
    ENGINE_BLK(802B7FD8);
    D_803EEF32 = 1;
    s = func_802A5ED0();
    ENGINE_BLK(802B7FEC);
    if (!(s < 0xF))
        goto sound;
    ENGINE_BLK(802B7FFC);
    func_802A6274(T(D_802C2954), 0x29810, 1, 8, 1, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
    ENGINE_BLK(802B8028);
    func_802A6274(T(D_802C2954), 0x29810, 1, 8, 2, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
    ENGINE_BLK(802B8054);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 8, 3, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
    ENGINE_BLK(802B8080);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 8, 4, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
sound:
    ENGINE_BLK(802B80AC);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802B80B8);
        s = -s;
    }
    ENGINE_BLK(802B80BC);
    func_802C4584((u32)s >> 5);
    ENGINE_BLK(802B80C4);
}

/* dust behind it on the ground */
REGS(gp)
void func_802B80D8(VS *vs) {
    ENGINE_BLK(802B80D8);
    engine_save(0x5FFFFFFE, 0);
    if (vs->unk96[3] == 0)
        goto done;
    ENGINE_BLK(802B8168);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802B8178);
    if (!(vs->unk50 < 3))
        goto done;
    ENGINE_BLK(802B8188);
    if (vs->unk9B != 0)
        goto done;
    ENGINE_BLK(802B8194);
    func_8027BE7C(3, vs->unk4[6], 0xFA, -0x190, -0x190, -0x190, D_803EEF18, D_803EEF20, vs->unk4E, 3, 0x32, 0x32, 0);
    ENGINE_BLK(802B81F0);
done:
    ENGINE_BLK(802B81F4);
    engine_restore();
}

/* the hotrod's matrix, its vertices and its collision */
REGS(gp)
void func_802B8278(VS *vs) {
    u8 *model = D_803EEF24, *buf;
    s32 *m, off;

    ENGINE_BLK(802B8278);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B82A8);
        m = (s32 *)(D_803EEF28 + off);
    } else {
        ENGINE_BLK(802B82BC);
        m = (s32 *)(D_803EEF2C + off);
    }
    ENGINE_BLK(802B82CC);
    D_803ED390[1] = vs->unk4C;
    ENGINE_LEAVE(20, D_803EEF18);
    ENGINE_LEAVE(21, D_803EEF1C);
    ENGINE_LEAVE(22, D_803EEF20);
    ENGINE_LEAVE(23, 0x32C8);
    func_802AA764(D_803EEF18, D_803EEF1C, D_803EEF20, 0x32C8, m);
    ENGINE_BLK(802B8308);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B831C);
        buf = D_803EEF28;
    } else {
        ENGINE_BLK(802B832C);
        buf = D_803EEF2C;
    }
    ENGINE_BLK(802B8338);
    model = D_803EEF24;
    ENGINE_LEAVE(11, T(model));
    func_8029C454(D_803EEF18, D_803EEF1C, D_803EEF20, 8, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802B8380);
    ENGINE_LEAVE(11, T(model));
    func_802ABBEC(8, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802B83A0);
}

/* the turn rate: the speed / 3.6, or / 11 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B83B0(VS *vs) {
    f32 d;

    ENGINE_BLK(802B83B0);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802B83C4);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802B83D4);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802B83E4);
    d = 3.6f;
    goto div;
air:
    ENGINE_BLK(802B83EC);
    d = 11.0f;
div:
    ENGINE_BLK(802B83F8);
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for the hotrod */
REGS()
void func_802B8424(void) {
    ENGINE_BLK(802B8424);
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

    ENGINE_BLK(802B8480);
    engine_save(ENGINE_T0_T5, 0);
    D_803EF2F8 = model;
    buf = D_80358070;
    D_803EF2FC = buf;
    D_803EF300 = buf + 0x800;
    D_80358070 = buf + 0x1000;
    /* ($a1: what func_802A396C left) */
    func_802A1388(0xFE, engine_ctx(5), D_803EF2FC, D_803EF300, model);
    ENGINE_BLK(802B84FC);
    vs->unk4C = 0;
    vs->unk4E = 0;
    D_803EF32A = 0;
    vs->unk76 = 0x14;
    D_803EF2E6 = 0;
    D_803EF328 = 0;
    ENGINE_LEAVE(22, T(D_803EF2FC));
    ENGINE_LEAVE(23, T(D_803EF300));
    func_8029F85C(Q, D_803EF2F8, D_803EF2FC, D_803EF300);
    ENGINE_BLK(802B856C);
    func_802A039C(0, 100, Q);
    ENGINE_BLK(802B8580);
    func_802A03D4(0, 0, Q);
    ENGINE_BLK(802B8594);
    func_802A040C(0, 0, Q);
    ENGINE_BLK(802B85A8);
    func_802A0480(0, 0, Q, 0.0f);
    ENGINE_BLK(802B85C0);
    func_802A0290(0, 1, Q);
    ENGINE_BLK(802B85D4);
    func_8029E558(Q, D_803EF2FC, D_803EF300);
    ENGINE_BLK(802B85E8);
    func_802A0320(0, Q);
    ENGINE_BLK(802B85F8);
    func_802A0290(0, 1, Q);
    ENGINE_BLK(802B860C);
    func_8029E558(Q, D_803EF300, D_803EF2FC);
    ENGINE_BLK(802B8620);
    func_802A039C(1, 9, Q);
    ENGINE_BLK(802B8634);
    func_802A03D4(1, 0, Q);
    ENGINE_BLK(802B8648);
    func_802A040C(1, 0, Q);
    ENGINE_BLK(802B865C);
    func_802A0290(1, -1, Q);
    ENGINE_BLK(802B8670);
    func_802A0290(2, -1, Q);
    ENGINE_BLK(802B8684);
    func_802A0290(3, -1, Q);
    ENGINE_BLK(802B8698);
    func_802A039C(4, 1, Q);
    ENGINE_BLK(802B86AC);
    func_802A03D4(4, 1, Q);
    ENGINE_BLK(802B86C0);
    func_802A040C(4, 1, Q);
    ENGINE_BLK(802B86D4);
    func_802A0290(4, 1, Q);
    ENGINE_BLK(802B86E8);
    D_803EF32C = 0;
    D_803EF324 = 0;
    D_803EF32D = 0;
    D_803EF31C = 0;
    func_80258230(0xFE, 0x78, 0x2D, 0x2D);
    ENGINE_BLK(802B8734);
    ENGINE_LEAVE(28, T(vs));    /* (func_802B899C works on the $gp it finds) */
    func_802B899C();
    ENGINE_BLK(802B873C);
    model = D_803EF2F8;
    func_802AA838(D_803EF300, D_803EF2FC, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802B8770);
    engine_restore();
}

/* its rotor's sound: on within 0x3E80 of the player, louder nearer, panned
   by x */
void func_802B8794(void) {
    s32 d, dx, v;

    ENGINE_BLK(802B8794);
    dx = D_803643E0 - D_803EF2EC;
    d = (s32)func_802ABCDC(D_803643E0, D_803643E4, D_803643E8, D_803EF2EC, D_803EF2F0, D_803EF2F4);
    ENGINE_BLK(802B8868);
    if (!(d < 0x3E81)) {
        ENGINE_BLK(802B8878);
        if (D_803EF2E8 == NULL)
            goto done;
        ENGINE_BLK(802B8880);
        func_802608C8(D_803EF2E8);
        ENGINE_BLK(802B8888);
        D_803EF2E8 = NULL;
        goto done;
    }
    ENGINE_BLK(802B8894);
    if (D_803EF2E8 == NULL) {
        ENGINE_BLK(802B889C);
        func_80260650(D_80367738, 0x13, &D_803EF2E8);
    }
    ENGINE_BLK(802B88B4);
    d -= 0x1F4;
    if (d < 0) {
        ENGINE_BLK(802B88C0);
        d = 0;
    }
    ENGINE_BLK(802B88C4);
    func_80260AB8(D_803EF2E8, 8, 0x7FFF - d);
    ENGINE_BLK(802B88DC);
    v = 0x40 + (dx >> 5);
    if (v < 0) {
        ENGINE_BLK(802B8900);
        v = 0;
    } else {
        ENGINE_BLK(802B88F0);
        if (!(v < 0x80)) {
            ENGINE_BLK(802B88F8);
            v = 0x7F;
        }
    }
    ENGINE_BLK(802B8904);
    func_80260AB8(D_803EF2E8, 4, v);
done:
    ENGINE_BLK(802B8914);
}

/* hd.c's, each frame: its flight, its rotors, its matrix, where the player
   would get out, and its shadow */
void func_802B899C(void) {
    /* (on the $gp it finds: the setup's, or what hd.c's caller left) */
    VS *vs = (VS *)engine_ctx(28);
    s32 *p;

    ENGINE_BLK(802B899C);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802B8D04();
    ENGINE_BLK(802B89EC);
    func_802B98E0(vs);
    ENGINE_BLK(802B89F4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B8A1C);
        func_8029E558(Q, D_803EF2FC, D_803EF300);
        ENGINE_BLK(802B8A30);
    } else {
        ENGINE_BLK(802B8A38);
        func_8029E558(Q, D_803EF300, D_803EF2FC);
    }
    ENGINE_BLK(802B8A4C);
    func_802B9B4C(vs);
    ENGINE_BLK(802B8A54);
    p = (s32 *)func_802ABC88(0xFE, 1);
    ENGINE_BLK(802B8A60);
    D_803EF310 = p[0];
    D_803EF314 = p[1];
    D_803EF318 = p[2];
    func_802B8C18(vs);
    ENGINE_BLK(802B8A94);
    engine_restore();
}

/* hd.c's, when it has landed: the player's start where it is, and the
   level's end */
void func_802B8AE4(void) {
    VS *vs = &D_803EF240;
    u8 s;

    ENGINE_BLK(802B8AE4);
    D_803EF32A = vs->unk4E;
    D_803EF328 = vs->unk76;
    D_803ED808 = D_803EF310;
    D_803ED80C = D_803EF314;
    D_803ED810 = D_803EF318;
    D_80368030 = D_803EF31C;
    s = D_803EF32C;
    if (D_80364A90 != 0x1000) {
        ENGINE_BLK(802B8B8C);
        if (s != 4)
            goto done;
        ENGINE_BLK(802B8B98);
        D_80364A98 = 0x1000;
        goto done;
    }
    ENGINE_BLK(802B8BA8);
    if (s != 5) {
        ENGINE_BLK(802B8BB4);
        if (D_803EF31C < D_803EF314)
            goto done;
    }
    ENGINE_BLK(802B8BD0);
    func_80275390(0x2000);
    ENGINE_BLK(802B8BE0);
done:
    ENGINE_BLK(802B8BE4);
}

/* the ground's height under it (inside the level), and its shadow there */
REGS(gp)
void func_802B8C18(VS *vs) {
    s32 x = D_803EF2EC, z = D_803EF2F4, t3 = engine_ctx(11);

    ENGINE_BLK(802B8C18);
    if (x <= 0)
        goto shadow;
    ENGINE_BLK(802B8C3C);
    if (z <= 0)
        goto shadow;
    ENGINE_BLK(802B8C44);
    if (!(x < D_803BE732 << 5))
        goto shadow;
    ENGINE_BLK(802B8C5C);
    if (!(z < D_803BE736 << 5))
        goto shadow;
    ENGINE_BLK(802B8C74);
    t3 = func_802A9B1C(0, x, z, D_803EF31C, 0xFE, vs, engine_ctx(30));
    ENGINE_LEAVE(11, t3);
    ENGINE_BLK(802B8C8C);
    D_803EF31C = t3;
shadow:
    ENGINE_BLK(802B8C98);
    func_802582C4(0xFE, D_803EF2EC, t3, D_803EF2F4, D_803EF2F0, 0, 0, (s16)vs->unk4C);
    ENGINE_BLK(802B8CE0);
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

    ENGINE_BLK(802B8D04);
    engine_save(0x5FFFFFFE, 0);
    t0 = D_803EF32C;
    if (t0 == 0)
        goto hover;
    ENGINE_BLK(802B8D28);
    if (t0 == 1)
        goto fly;
    ENGINE_BLK(802B8D30);
    if (t0 == 3)
        goto come_in;
    ENGINE_BLK(802B8D38);
    if (t0 == 2)
        goto slow;
    ENGINE_BLK(802B8D40);
    if (t0 == 4)
        goto land;
    ENGINE_BLK(802B8D48);
    if (t0 == 5)
        goto down;
    ENGINE_BLK(802B8D50);
    if (t0 != 6) {
        ENGINE_BLK(802B8D58);
        engine_trap(0x802B8D58);
    }

    /* 6: the rotor starting */
    ENGINE_BLK(802B8D5C);
    part(4, Q, &f0);
    ENGINE_BLK(802B8D6C);
    if (!(f0 == 0.0f)) {
        ENGINE_BLK(802B8D80);
        func_802A03D4(4, 1, Q);
        ENGINE_BLK(802B8D94);
        func_802A0290(4, 1, Q);
    }
    ENGINE_BLK(802B8DA8);
    D_803EF32C = 0;
    goto hover;

down:
    /* 5: down, the rotor stopping; up to the flying height again */
    ENGINE_BLK(802B8DBC);
    t3 = part(4, Q, NULL);
    ENGINE_BLK(802B8DCC);
    if (t3 != 1) {
        ENGINE_BLK(802B8DD8);
        D_803EF32E = 1;
    }
    ENGINE_BLK(802B8DE8);
    t3 = D_803EF304;
    t5 = D_803EF2F0;
    if (t3 != t5) {
        if (t3 < t5) {
            ENGINE_BLK(802B8E08);
            ENGINE_BLK(802B8E28);
            t5 -= 0x14;
            if (t5 < t3) {
                ENGINE_BLK(802B8E38);
                t5 = t3;
            }
        } else {
            ENGINE_BLK(802B8E08);
            ENGINE_BLK(802B8E10);
            t5 += 0x14;
            if (t3 < t5) {
                ENGINE_BLK(802B8E20);
                t5 = t3;
            }
        }
        ENGINE_BLK(802B8E3C);
        D_803EF2F0 = t5;
        goto rest;
    }
    ENGINE_BLK(802B8E44);
    if (D_803EF32E == 0)
        goto rest;
    ENGINE_BLK(802B8E58);
    D_803EF32C = 0;
    D_803EF32E = 0;
    goto rest;

land:
    /* 4: down to D_80368030 + 0xFA0 */
    ENGINE_BLK(802B8E70);
    t3 = D_80368030 + 0xFA0;
    t5 = D_803EF2F0;
    if (t3 != t5) {
        if (t3 < t5) {
            ENGINE_BLK(802B8E94);
            ENGINE_BLK(802B8EB4);
            t5 -= 0x14;
            if (t5 < t3) {
                ENGINE_BLK(802B8EC4);
                t5 = t3;
            }
        } else {
            ENGINE_BLK(802B8E94);
            ENGINE_BLK(802B8E9C);
            t5 += 0x14;
            if (t3 < t5) {
                ENGINE_BLK(802B8EAC);
                t5 = t3;
            }
        }
        ENGINE_BLK(802B8EC8);
        D_803EF2F0 = t5;
        goto rest;
    }
    ENGINE_BLK(802B8ED0);
    t3 = part(4, Q, NULL);
    ENGINE_BLK(802B8EE0);
    if (t3 == 1)
        goto rest;
    ENGINE_BLK(802B8EEC);
    D_803EF32D = 1;
    if (D_80364A90 != 0x1000) {
        ENGINE_BLK(802B8F10);
        t0 = func_8026A610(D_803643E0, D_803643E0, D_803EF2EC, D_803EF2F4);
        ENGINE_BLK(802B8FB8);
        t0 = 0x88B8 - (t0 << 1);
        if (!(t0 < 0xFA1)) {
            ENGINE_BLK(802B8FD0);
            if (!(t0 < 0x8000)) {
                ENGINE_BLK(802B8FDC);
                t0 = 0x7FFF;
            }
            ENGINE_BLK(802B8FE0);
            {
                SndState *h = func_80260650(D_80367738, 0x28, NULL);

                ENGINE_BLK(802B9000);
                func_80260AB8(h, 8, t0);
            }
        }
        ENGINE_BLK(802B9018);
    }
    ENGINE_BLK(802B909C);
    D_803EF32C = 5;
    func_802A0290(4, 1, Q);
    ENGINE_BLK(802B90C0);
    goto rest;

slow:
    /* 2: slowing down to 0x14, then coming in */
    ENGINE_BLK(802B90C8);
    t3 = vs->unk76;
    if (!(t3 < 0x15)) {
        ENGINE_BLK(802B90D8);
        vs->unk76 = t3 - 4;
        goto turn;
    }
    ENGINE_BLK(802B90E4);
    vs->unk76 = 0x14;
    D_803EF32C = 3;
    vs->unk76 = 0x14;
    s1 = func_802B988C();
    ENGINE_BLK(802B9104);
    D_803EF320 = (s32)s1;

come_in:
    /* 3: straight to D_803EF308/30C, at the speed, until it is there or
       the distance stops shrinking; then landing */
    ENGINE_BLK(802B9110);
    t4 = D_803EF324;
    if (t4 >= 0) {
        ENGINE_BLK(802B9124);
        t4--;
        if (t4 < 0) {
            ENGINE_BLK(802B9130);
            t4 = 0;
        }
    } else {
        ENGINE_BLK(802B9138);
        t4++;
        if (t4 > 0) {
            ENGINE_BLK(802B9144);
            t4 = 0;
        }
    }
    ENGINE_BLK(802B9148);
    D_803EF324 = t4;
    s1 = func_802B988C();
    ENGINE_BLK(802B9150);
    if (s1 == 0)
        goto arrived;
    ENGINE_BLK(802B9158);
    t6 = D_803EF2EC;
    t4 = D_803EF308;
    t5 = (s32)s1;
    t2 = vs->unk76;
    t4 = t4 - t6;
    t7 = t4;
    if (t4 < 0) {
        ENGINE_BLK(802B9184);
        t7 = -t4;
    }
    ENGINE_BLK(802B9188);
    u = ((u32)t7 << 10) / (u32)t5;
    ENGINE_BLK(802B91A4);
    t7 = (u32)(u * (u32)t2) >> 10;
    if (t4 < 0) {
        ENGINE_BLK(802B91B8);
        t7 = -t7;
    }
    ENGINE_BLK(802B91BC);
    t6 += t7;
    D_803EF2EC = t6;
    t6 = D_803EF2F4;
    t4 = D_803EF30C;
    t5 = (s32)s1;
    t4 = t4 - t6;
    t7 = t4;
    if (t4 < 0) {
        ENGINE_BLK(802B91EC);
        t7 = -t4;
    }
    ENGINE_BLK(802B91F0);
    u = ((u32)t7 << 10) / (u32)t5;
    ENGINE_BLK(802B920C);
    t7 = (u32)(u * (u32)t2) >> 10;
    if (t4 < 0) {
        ENGINE_BLK(802B9220);
        t7 = -t7;
    }
    ENGINE_BLK(802B9224);
    t6 += t7;
    D_803EF2F4 = t6;
    s1 = func_802B988C();
    ENGINE_BLK(802B9230);
    if (s1 < D_803EF320) {
        ENGINE_BLK(802B92A4);
        D_803EF320 = (s32)s1;
        goto rest;
    }
arrived:
    ENGINE_BLK(802B9248);
    D_803EF2EC = D_803EF308;
    D_803EF2F4 = D_803EF30C;
    D_803EF32C = 4;
    func_802A0290(4, 1, Q);
    ENGINE_BLK(802B929C);
    goto rest;

hover:
    /* 0: the turn back to 4 a frame */
    ENGINE_BLK(802B92AC);
    t4 = D_803EF324;
    if (!(t4 < 4)) {
        ENGINE_BLK(802B92C4);
        t4--;
        if (t4 < 4) {
            ENGINE_BLK(802B92D4);
            t4 = 4;
        }
    } else {
        ENGINE_BLK(802B92DC);
        t4++;
        if (!(t4 < 5)) {
            ENGINE_BLK(802B92EC);
            t4 = 4;
        }
    }
    ENGINE_BLK(802B92F0);
    D_803EF324 = t4;
    goto turn;

fly:
    /* 1: toward D_803EF308/30C, speeding up to 0xA0 and turning to it,
       until within 0x7D0; near enough on its flight to the next level,
       the next stop is the level's (D_80368044/48) */
    ENGINE_BLK(802B92F8);
    s1 = func_802B988C();
    t3 = D_803EF2EC;
    t5 = D_803EF2F4;
    t6 = D_803EF308;
    s0 = D_803EF30C;
    ENGINE_BLK(802B9300);
    if (D_80364A90 == 0x800) {
        ENGINE_BLK(802B9314);
        t4 = D_802E8BDC;
        if (t4 == 0x1A)
            goto far;
        ENGINE_BLK(802B9328);
        if (t4 == 4)
            goto far;
        ENGINE_BLK(802B9330);
        if (t4 == 0x1D)
            goto farther;
        ENGINE_BLK(802B9338);
        if (t4 == 0x3A)
            goto farther;
        ENGINE_BLK(802B9340);
        if (t4 == 0xD)
            goto farther;
        ENGINE_BLK(802B9348);
        t4 = 0x7530;
        goto near;
    far:
        ENGINE_BLK(802B9350);
        t4 = 0xBB80;
        goto near;
    farther:
        ENGINE_BLK(802B9358);
        t4 = 0x11170;
    near:
        ENGINE_BLK(802B9360);
        if (!(t4 < s1)) {
            ENGINE_BLK(802B936C);
            D_80364A98 = 1;
            func_8029A7E4(D_80305D40);
            ENGINE_BLK(802B9404);
            D_803EF308 = D_80368044;
            t3 = D_80368048;
            D_803EF30C = t3;
            func_8026AF6C(0x4000);
            ENGINE_BLK(802B952C);
        }
    }
    ENGINE_BLK(802B95AC);
    if (s1 < 0x7D0) {
        ENGINE_BLK(802B9778);
        D_803EF32C = 2;
        goto turn;
    }
    ENGINE_BLK(802B95B8);
    t0 = vs->unk76 + 2;
    if (!(t0 < 0xA1)) {
        ENGINE_BLK(802B95CC);
        t0 = 0xA0;
    }
    /* the heading to it: func_802ABB1C's angle from (D_803EF308, 30C) to
       where it is (by the quadrant), checked against the reverse */
    ENGINE_BLK(802B95D0);
    t7 = t3;
    t1 = s0;
    t3 = t6;
    s0 = t5;
    vs->unk76 = t0;
    t4 = t1;
    s6 = func_802ABB1C(t3, t4, 0, (s32)s1, t7, s0);
    ENGINE_BLK(802B95F8);
    if (!(s6 < 0x401)) {
        ENGINE_BLK(802B9604);
        s6 = func_802ABB1C(t3, t4, (s32)s1, 0, t7, s0);
        ENGINE_BLK(802B9610);
        s6 += 0x400;
        if (!(s6 < 0x801)) {
            ENGINE_BLK(802B9620);
            s6 = func_802ABB1C(t3, t4, 0, -(s32)s1, t7, s0);
            ENGINE_BLK(802B962C);
            s6 += 0x800;
        }
    }
    ENGINE_BLK(802B9630);
    func_802ACE38(0, s1, s6, &xr, &zr);
    ENGINE_BLK(802B9640);
    a = xr + t7;
    b = zr + s0;
    func_802ACE38(0, s1, 0xFFF - s6, &xr, &zr);
    ENGINE_BLK(802B9654);
    c = xr + t7;
    d = zr + s0;
    a -= t3;
    b -= t4;
    c -= t3;
    d -= t4;
    if (a < 0) {
        ENGINE_BLK(802B9674);
        a = -a;
    }
    ENGINE_BLK(802B9678);
    if (b < 0) {
        ENGINE_BLK(802B9680);
        b = -b;
    }
    ENGINE_BLK(802B9684);
    a += b;
    if (c < 0) {
        ENGINE_BLK(802B9690);
        c = -c;
    }
    ENGINE_BLK(802B9694);
    if (d < 0) {
        ENGINE_BLK(802B969C);
        d = -d;
    }
    ENGINE_BLK(802B96A0);
    c += d;
    if (!(a < c)) {
        ENGINE_BLK(802B96B0);
        s6 = 0xFFF - s6;
    }
    /* turn toward it: up to (the difference >> 6) a frame */
    ENGINE_BLK(802B96B8);
    t0 = (u16)vs->unk4E - s6;
    t3 = t0;
    if (t3 >= 0) {
        ENGINE_BLK(802B96E8);
        if (!(t3 < 0x800)) {
            ENGINE_BLK(802B96F4);
            t3 = 0xFFF - t3;
        }
    } else {
        ENGINE_BLK(802B96CC);
        if (t3 < -0x7FF) {
            ENGINE_BLK(802B96D4);
            t3 += 0xFFF;
        } else {
            ENGINE_BLK(802B96E0);
            t3 = -t3;
        }
    }
    ENGINE_BLK(802B96FC);
    t3 = (u32)t3 >> 6;
    t4 = -t3;
    if (t0 > 0) {
        ENGINE_BLK(802B971C);
        if (!(t0 < 0x800))
            goto right;
    } else {
        ENGINE_BLK(802B9708);
        if (!(t0 < -0x800))
            goto right;
        ENGINE_BLK(802B9714);
    }
    ENGINE_BLK(802B9728);
    t2 = D_803EF324 - 1;
    if (t2 < t4) {
        ENGINE_BLK(802B9744);
        t2 = t4;
    }
    ENGINE_BLK(802B9748);
    D_803EF324 = t2;
    goto turn;
right:
    ENGINE_BLK(802B9750);
    t2 = D_803EF324 + 1;
    if (t3 < t2) {
        ENGINE_BLK(802B976C);
        t2 = t3;
    }
    ENGINE_BLK(802B9770);
    D_803EF324 = t2;

turn:
    /* the heading by the turn, and on at the speed */
    ENGINE_BLK(802B978C);
    t4 = (u16)vs->unk4E + D_803EF324;
    if (!(t4 < 0x1000)) {
        ENGINE_BLK(802B97C0);
        t4 -= 0xFFF;
    } else {
        ENGINE_BLK(802B97AC);
        if (t4 < 0) {
            ENGINE_BLK(802B97B4);
            t4 += 0xFFF;
        }
    }
    ENGINE_BLK(802B97C8);
    vs->unk4E = t4;
    vs->unk4C = t4;
    t0 = func_802A860C(t4, &vs->unk76, &D_803EF2EC, &D_803EF2F4, 0.0f, &t1);
    ENGINE_BLK(802B97EC);
    D_803EF2EC = t0;
    D_803EF2F4 = t1;

rest:
    /* but landing or down, the level's ground, and its height toward
       D_803EF304 by 0x14 a frame */
    ENGINE_BLK(802B97F4);
    t0 = D_803EF32C;
    if (t0 == 4)
        goto done;
    ENGINE_BLK(802B980C);
    if (t0 == 5)
        goto done;
    ENGINE_BLK(802B9814);
    func_802A5604(D_80358074);
    ENGINE_BLK(802B9824);
    t1 = D_803EF2F0;
    t3 = D_803EF304;
    if (t3 == t1)
        goto done;
    ENGINE_BLK(802B9844);
    if (t3 < t1) {
        ENGINE_BLK(802B9864);
        t1 -= 0x14;
        if (t1 < t3) {
            ENGINE_BLK(802B9874);
            t1 = t3;
        }
    } else {
        ENGINE_BLK(802B984C);
        t1 += 0x14;
        if (t3 < t1) {
            ENGINE_BLK(802B985C);
            t1 = t3;
        }
    }
    ENGINE_BLK(802B9878);
    D_803EF2F0 = t1;
done:
    ENGINE_BLK(802B987C);
    engine_restore();
}

/* how far it is from D_803EF308/30C (x and z) */
REGS(-> s1+f0)
s64 func_802B988C(void) {
    s64 d;

    ENGINE_BLK(802B988C);
    d = func_802ABCDC(D_803EF2EC, 0, D_803EF2F4, D_803EF308, 0, D_803EF30C);
    ENGINE_BLK(802B98D0);
    return d;
}

/* its sound's pitch by its speed, its tail's tilt (part 2) by its speed and
   its body's lean (part 3) by its turn */
REGS(gp)
void func_802B98E0(VS *vs) {
    s32 t0, a0;
    f32 f20, f;

    ENGINE_BLK(802B98E0);
    engine_save(0x5FFFFFFE, 0);
    t0 = 0xA0 - vs->unk76;
    f20 = (f32)t0 / (f32)0x140;
    a0 = D_803EF2E6;
    D_803EF2E6 = t0;
    if (a0 == t0)
        goto tilt;
    ENGINE_BLK(802B99A4);
    if (D_803EF2E8 == NULL)
        goto tilt;
    ENGINE_BLK(802B99B4);
    func_80260AB8(D_803EF2E8, 0x10, f2i(0.8f + (f32)t0 * -0.004f));
tilt:
    ENGINE_BLK(802B99E4);
    func_802A0360(2, 0, Q, f20);
    ENGINE_BLK(802B9A7C);
    t0 = D_803EF324;
    if (t0 == 1)
        goto zero;
    ENGINE_BLK(802B9A94);
    if (t0 != -1)
        goto lean;
zero:
    ENGINE_BLK(802B9A9C);
    t0 = 0;
lean:
    ENGINE_BLK(802B9AA0);
    if (t0 >= 0) {
        ENGINE_BLK(802B9AA8);
        f = (f32)(0x20 - t0) / (f32)0x40;
        func_802A0360(3, 0, Q, f);
        ENGINE_BLK(802B9AE4);
    } else {
        ENGINE_BLK(802B9AEC);
        if (t0 < 0) {
            ENGINE_BLK(802B9AF8);
            t0 = -t0;
        }
        ENGINE_BLK(802B9AFC);
        f = (f32)t0 / (f32)0x40 + 0.5f;
        func_802A0360(3, 0, Q, f);
    }
    ENGINE_BLK(802B9B3C);
    engine_restore();
    ENGINE_LEAVE_F(20, f20);    /* (which it doesn't put back) */
}

/* the chopper's matrix and its collision */
REGS(gp)
void func_802B9B4C(VS *vs) {
    u8 *model = D_803EF2F8, *buf;
    s32 *m, off;

    ENGINE_BLK(802B9B4C);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B9B7C);
        m = (s32 *)(D_803EF2FC + off);
    } else {
        ENGINE_BLK(802B9B90);
        m = (s32 *)(D_803EF300 + off);
    }
    ENGINE_BLK(802B9BA0);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    ENGINE_LEAVE(20, D_803EF2EC);
    ENGINE_LEAVE(21, D_803EF2F0);
    ENGINE_LEAVE(22, D_803EF2F4);
    ENGINE_LEAVE(23, 0x5208);
    func_802AA764(D_803EF2EC, D_803EF2F0, D_803EF2F4, 0x5208, m);
    ENGINE_BLK(802B9BE4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B9BF8);
        buf = D_803EF2FC;
    } else {
        ENGINE_BLK(802B9C08);
        buf = D_803EF300;
    }
    ENGINE_BLK(802B9C14);
    model = D_803EF2F8;
    ENGINE_LEAVE(11, T(model));
    func_802ABBEC(0xFE, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802B9C38);
}
