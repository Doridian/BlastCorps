/*
 * hd_code 86F60 (us.v11 0x802CB720-0x802CC920): the police car
 * (VEHICLE_POLICE), as native C (engine.h).  Its parts are D_803F8B80, its
 * state D_803F8E80 and its position D_803F8F28..30 (vehicle.h).
 * func_802CB720 sets it up (from the level loader), func_802CBEF0 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.  It is the A-Team van's code (88160) with its own numbers,
 * and the siren's lights.
 *
 * func_802CBD5C and func_802CBDE8, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* the police car's .bss (asm/data/hd_code/86F60.bss.s) */
extern Part D_803F8B80[32];
extern VS D_803F8E80;
extern s32 D_803F8F28, D_803F8F2C, D_803F8F30;  /* x, y, z */
extern u8 *PTR32 D_803F8F34;                    /* its model file */
extern u8 *PTR32 D_803F8F38;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803F8F3C;
extern u16 D_803F8F40;                          /* the heading it turns to (with D_803A7425) */
extern u8 D_803F8F42;                           /* frames until the next sparks */
extern s8 D_803F8F43;                           /* turning to it */
extern u8 D_803F8F44;                           /* frames without the gears after a hit */
extern u8 D_803F8F45;                           /* the siren's lights: a step, 2..13 */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern u8 D_80370C1A, D_80370C1B;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306430[];
extern s32 D_803643E4, D_803643E8;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */
extern u8 D_802C2324[], D_802C2348[];           /* the siren's two lights (56040's list) */

void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);

void func_802CBEF0(void);
void func_802CBD18(void);
REGS(gp)
void func_802CC400(VS *vs);
REGS(gp)
void func_802CC56C(VS *vs);
REGS(gp)
void func_802CC70C(VS *vs);
REGS(gp -> s3)
s32 func_802CC844(VS *vs);
void func_802CC8B8(void);

#define T(p) ((s32)(p))

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802CB720(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 avg;
    VS *vs = &D_803F8E80;
    u8 *buf;
    s16 *r;

    ENGINE_COST(802CB720, 221);
    D_803F8F34 = model;
    buf = D_80358070;
    D_803F8F38 = buf;
    D_803F8F3C = buf + 0x100;
    D_80358070 = buf + 0x200;
    func_802A1388(0xD, 0, D_803F8F38, D_803F8F3C, model);
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
    D_803F8F28 = x;
    D_803F8F2C = y;
    D_803F8F30 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803F8F2C, x, z, vs->unk4, &D_803F8F2C, (s16 *)&vs->unk4C, 0xD, vs, 0, &avg);
    func_8029F85C(D_803F8B80, D_803F8F34, D_803F8F38, D_803F8F3C);
    func_802A039C(0, 100, D_803F8B80);
    func_802A03D4(0, 0, D_803F8B80);
    func_802A040C(0, 0, D_803F8B80);
    func_802A0480(0, 0, D_803F8B80, 0.0f);
    func_802A0290(0, 1, D_803F8B80);
    func_8029E558(D_803F8B80, D_803F8F38, D_803F8F3C);
    func_802A0320(0, D_803F8B80);
    func_802A0290(0, 1, D_803F8B80);
    func_8029E558(D_803F8B80, D_803F8F3C, D_803F8F38);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0x50, r[5] = 1;
    r[6] = 0x50, r[7] = 0xA0, r[8] = 2;
    r[9] = 0xA0, r[10] = 0xB4, r[11] = 3;
    r[12] = 0xB4, r[13] = 0x154, r[14] = 2;
    D_803F8F42 = 0;
    D_803F8F43 = 0;
    D_803F8F44 = 0;
    D_803F8F45 = 0;
    func_8029C354(0xD, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x4268);
    func_80258230(0xD, 0x3C, 0x19, 0x19);
    vs->unk9A = 1;
    func_802CBEF0();
    vs->unk9A = 0;
    func_802AA838(D_803F8F3C, D_803F8F38, *(s32 *)(D_803F8F34 + *(s32 *)(D_803F8F34 + 0x18) + 4));
}

/* hd.c's: the player gets in: the siren's lights off */
void func_802CBA94(void) {
    VS *vs = &D_803F8E80;

    ENGINE_COST(802CBA94, 51);
    vs->unk96[3] = 0;
    D_8036444C = 3000;
    D_80364450 = 1000;
    func_802A05D0(D_802C2324, 0);
    func_802A05F8(D_802C2324, 0);
    func_802A0620(D_802C2324, 0);
    func_802A0508(D_802C2324, -1);
    func_802A05D0(D_802C2348, 0);
    func_802A05F8(D_802C2348, 0);
    func_802A0620(D_802C2348, 0);
    func_802A0508(D_802C2348, -1);
    func_802C4310(0xCE);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802CBB60(void) {
    VS *vs = &D_803F8E80;
    u8 r = 0;

    ENGINE_COST(802CBB60, 23);
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
void func_802CBBBC(void) {
    VS *vs = &D_803F8E80;

    ENGINE_COST(802CBBBC, 19);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803F8F38, (u32 *)D_803F8F3C, 0x100);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802CBC08(void) {
    VS *vs = &D_803F8E80;

    ENGINE_COST(802CBC08, 35);
    func_802A9A60(vs->unk52, D_803F8F2C, D_803F8F28, D_803F8F30, vs->unk4, &D_803F8F2C, (s16 *)&vs->unk4C, 0xD, vs, 0);
    func_802CC70C(vs);
    func_802A133C(D_803F8F28, D_803F8F2C, D_803F8F30, 0xD, vs);
}

/* its light */
void func_802CBD18(void) {
    ENGINE_COST(802CBD18, 17);
    func_802ABD54(0xD, D_803F8F28, D_803F8F2C, D_803F8F30);
}

/* each frame */
void func_802CBEF0(void) {
    VS *vs = &D_803F8E80;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_COST(802CBEF0, 214);
    /* (its $fp as it found it: 5CB60.c and the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802CBD18();
    func_802A75DC((u8 *)D_803F8B80, &D_803F8F28, &D_803F8F2C, &D_803F8F30, (u8 *)vs);
    func_802C4724(0xD);
    if (vs->unk9A == 0) {
        func_802CC400(vs);
    }
    if (D_80367BFF != 0) {
        func_802CB690(vs);
    }
    func_802CC8B8();
    rate_i = func_802CC844(vs);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    if (D_803F8F44 == 0) {
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x14, vs, &t3);
    } else {
        D_803F8F44--;
    }
    func_802A7FD8(0x2EE0, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 1, 0xD, (s8 *)vs->unk96, vs->unk4, 700.0f, vs);
    if (D_803F8F43 != 0) {
        func_802A7070((s16 *)&D_803F8F40, vs);
    }
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803F8F28, &D_803F8F30, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803F8F28, &D_803F8F30, &D_803F8F2C, 0xD, 0x2BC, 0x190, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    if (D_8035805C != 0) {
        func_8029E558(D_803F8B80, D_803F8F38, D_803F8F3C);
    } else {
        func_8029E558(D_803F8B80, D_803F8F3C, D_803F8F38);
    }
    func_802CC70C(vs);
    ENGINE_LEAVE(8, 7);         /* $t0 and $t2: func_8029A800 (56040.c) takes them from the context */
    ENGINE_LEAVE(10, 0x96);
    func_8029A800(D_803F8F28, D_803F8F2C, D_803F8F30, D_80306430, 1, 1, vs->unk76, 0, 0xD, vs);
    func_8029C52C(0xD, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = D_803F8B80;
        func_802BE77C(0xD, vs);
        if (D_803A7424 == 0) {
            D_803F8F43 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    func_8029A914(vs);
    D_803F8F43 = 1;
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

        D_803F8F40 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    func_802A6FE4(0, vs);
    D_803F77D0 = D_803F8B80;
    func_802BE77C(0xD, vs);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    D_803F8F43 = 0;
    if (D_8035805C != 0) {
        a2 = D_803F8F3C;
        a3 = D_803F8F38;
    } else {
        a2 = D_803F8F38;
        a3 = D_803F8F3C;
    }
    func_802A768C((u8 *)D_803F8B80, &D_803F8F28, &D_803F8F2C, &D_803F8F30, (u32 *)a2, (u32 *)a3, 0x100, (u8 *)vs);
    D_803F8F44 = 5;
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
    func_802CC70C(vs);

done:
    D_803643E0 = D_803F8F28;
    D_803643E4 = D_803F8F2C;
    D_803643E8 = D_803F8F30;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0xD, vs);
    engine_restore();
}

/* the siren's lights (flashing while a C button is held), the dust, the
   wheels' sparks off rough ground, and the engine's sound */
REGS(gp)
void func_802CC400(VS *vs) {
    s32 s, l;

    ENGINE_COST(802CC400, 48);
    if (D_80370C1A == 0) {
        if (D_80370C1B == 0) {
            l = 0;
            D_803F8F45 = l;
            goto lights;
        }
    }
    l = D_803F8F45 + 1;
    if (l == 0xE) {
        l = 2;
    }
    D_803F8F45 = l;
    l = (u32)l >> 1;
lights:
    func_802A05A4(D_802C2324, l, 0.0f);
    func_802A05A4(D_802C2348, l, 0.0f);
    func_802CC56C(vs);
    if (D_803F8F42 != 0) {
        D_803F8F42--;
        goto sound;
    }
    if (vs->unk96[3] == 0)
        goto sound;
    D_803F8F42 = 1;
    s = func_802A5ED0();
    if (!(s < 0xF))
        goto sound;
    func_802A6274(T(D_802C2954), 0x29810, 1, 0xD, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802A6274(T(D_802C2954), 0x29810, 1, 0xD, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
sound:
    s = vs->unk76;
    if (s < 0) {
        s = -s;
    }
    func_802C4584((u32)s >> 5);
}

/* dust behind it on the ground */
REGS(gp)
void func_802CC56C(VS *vs) {
    ENGINE_COST(802CC56C, 70);
    if (vs->unk96[3] == 0)
        goto done;
    if (vs->unk96[2] == 1)
        goto done;
    if (!(vs->unk50 < 3))
        goto done;
    if (vs->unk9B != 0)
        goto done;
    func_8027BE7C(3, vs->unk4[6], 0xFA, -0x190, -0x190, -0x190, D_803F8F28, D_803F8F30, vs->unk4E, 3, 0x32, 0x32, 0);
done:
    ;
}

/* the car's matrix, its vertices and its collision */
REGS(gp)
void func_802CC70C(VS *vs) {
    u8 *model = D_803F8F34, *buf;
    s32 *m, off;

    ENGINE_COST(802CC70C, 70);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803F8F38 + off);
    } else {
        m = (s32 *)(D_803F8F3C + off);
    }
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803F8F28, D_803F8F2C, D_803F8F30, 0x4268, m);
    if (D_8035805C != 0) {
        buf = D_803F8F38;
    } else {
        buf = D_803F8F3C;
    }
    model = D_803F8F34;
    func_8029C454(D_803F8F28, D_803F8F2C, D_803F8F30, 0xD, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(0xD, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* the turn rate: the speed / 3.6, or / 11 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802CC844(VS *vs) {
    f32 d;

    ENGINE_COST(802CC844, 25);
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

/* the camera's distance and speed for the police car */
void func_802CC8B8(void) {
    ENGINE_COST(802CC8B8, 23);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x96;
    D_803ED3F7 = 6;
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802CBD5C(s32 carrier) {
    CARRY_KEEP(802CBD5C, 802CBD98, 802CBDB8, 802CBDC4, &D_803F8E80, D_803F8F28, D_803F8F30);
}

REGS(a3)
void func_802CBDE8(s32 carrier) {
    CARRY_MOVE(802CBDE8, 802CBE24, 802CBE38, 802CBE40, 802CBEA0, 802CBEA8, 802CBED4, &D_803F8E80, &D_803F8F28, &D_803F8F2C, &D_803F8F30, 0xD, 0x2BC, 0x190,
               D_803ED40B = 1, func_802CC8B8(), func_802CC70C(&D_803F8E80));
}
