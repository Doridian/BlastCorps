/*
 * hd_code 88160 (us.v11 0x802CC920-0x802CDA08): the A-Team van
 * (VEHICLE_ATEAM), as native C (engine.h).  Its parts are D_803F8F50, its
 * state D_803F9250 and its position D_803F92F8..9300 (vehicle.h).
 * func_802CC920 sets it up (from the level loader), func_802CD068 runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.
 *
 * func_802CCED4 and func_802CCF60, at the end, are its two callbacks for
 * 62740's carrying (shared.h).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* the van's .bss (asm/data/hd_code/88160.bss.s) */
extern Part D_803F8F50[32];
extern VS D_803F9250;
extern s32 D_803F92F8, D_803F92FC, D_803F9300;  /* x, y, z */
extern u8 *PTR32 D_803F9304;                    /* its model file */
extern u8 *PTR32 D_803F9308;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803F930C;
extern u16 D_803F9310;                          /* the heading it turns back to after a hit */
extern u8 D_803F9312;                           /* frames until the next sparks */
extern s8 D_803F9313;                           /* turning back after a hit */
extern u8 D_803F9314;                           /* frames without the gears after a hit */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306440[];
extern s32 D_803643E4, D_803643E8;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);

void func_802CD068(void);
void func_802CCE90(void);
REGS(gp)
void func_802CD578(VS *vs);
REGS(gp)
void func_802CD660(VS *vs);
REGS(gp)
void func_802CD800(VS *vs);
REGS(gp -> s3)
s32 func_802CD938(VS *vs);
void func_802CD9AC(void);

#define T(p) ((s32)(p))

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802CC920(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 avg;
    VS *vs = &D_803F9250;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802CC920);
    engine_save(ENGINE_T0_T5, 0);
    D_803F9304 = model;
    buf = D_80358070;
    D_803F9308 = buf;
    D_803F930C = buf + 0x100;
    D_80358070 = buf + 0x200;
    func_802A1388(0xE, 0, D_803F9308, D_803F930C, model);
    ENGINE_BLK(802CC9A0);
    func_802A754C(vs);
    ENGINE_BLK(802CC9AC);
    vs->unk52[0] = 0xFA;
    vs->unk52[1] = 0x190;
    vs->unk52[2] = -0xFA;
    vs->unk52[3] = 0x190;
    vs->unk52[4] = 0xFA;
    vs->unk52[5] = -0x190;
    vs->unk5E[0] = 0x140;
    vs->unk5E[1] = 0x1F4;
    vs->unk5E[2] = -0x140;
    vs->unk5E[3] = 0x1F4;
    vs->unk5E[4] = 0x140;
    vs->unk5E[5] = -0x1F4;
    D_803F92F8 = x;
    D_803F92FC = y;
    D_803F9300 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803F92FC, x, z, vs->unk4, &D_803F92FC, (s16 *)&vs->unk4C, 0xE, vs, engine_ctx(30), &avg);
    ENGINE_BLK(802CCA5C);
    func_8029F85C(D_803F8F50, D_803F9304, D_803F9308, D_803F930C);
    ENGINE_BLK(802CCA98);
    func_802A039C(0, 100, D_803F8F50);
    ENGINE_BLK(802CCAAC);
    func_802A03D4(0, 0, D_803F8F50);
    ENGINE_BLK(802CCAC0);
    func_802A040C(0, 0, D_803F8F50);
    ENGINE_BLK(802CCAD4);
    func_802A0480(0, 0, D_803F8F50, 0.0f);
    ENGINE_BLK(802CCAEC);
    func_802A0290(0, 1, D_803F8F50);
    ENGINE_BLK(802CCB00);
    func_8029E558(D_803F8F50, D_803F9308, D_803F930C);
    ENGINE_BLK(802CCB14);
    func_802A0320(0, D_803F8F50);
    ENGINE_BLK(802CCB24);
    func_802A0290(0, 1, D_803F8F50);
    ENGINE_BLK(802CCB38);
    func_8029E558(D_803F8F50, D_803F930C, D_803F9308);
    ENGINE_BLK(802CCB4C);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 8;
    r[3] = 0, r[4] = 0x50, r[5] = 8;
    r[6] = 0x50, r[7] = 0xA0, r[8] = 8;
    r[9] = 0xA0, r[10] = 0xFA, r[11] = 8;
    r[12] = 0xFA, r[13] = 0x14A, r[14] = 4;
    D_803F9312 = 0;
    D_803F9313 = 0;
    D_803F9314 = 0;
    func_8029C354(0xE, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x55F0);
    ENGINE_BLK(802CCC08);
    func_80258230(0xE, 0x3C, 0x19, 0x19);
    ENGINE_BLK(802CCC20);
    vs->unk9A = 1;
    func_802CD068();
    ENGINE_BLK(802CCC30);
    vs->unk9A = 0;
    func_802AA838(D_803F930C, D_803F9308, *(s32 *)(D_803F9304 + *(s32 *)(D_803F9304 + 0x18) + 4));
    ENGINE_BLK(802CCC68);
    engine_restore();
}

/* hd.c's: the player gets in */
void func_802CCC8C(void) {
    VS *vs = &D_803F9250;

    ENGINE_BLK(802CCC8C);
    vs->unk96[3] = 0;
    D_8036444C = 3000;
    D_80364450 = 1000;
    func_802C4310(0x7B);
    ENGINE_BLK(802CCCC8);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802CCCD8(void) {
    VS *vs = &D_803F9250;
    u8 r = 0;

    ENGINE_BLK(802CCCD8);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802CCCFC);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802CCD0C);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802CCD1C);
                r = 1;
            }
        }
    }
    ENGINE_BLK(802CCD20);
    return r;
}

/* hd.c's: the player gets out */
void func_802CCD34(void) {
    VS *vs = &D_803F9250;

    ENGINE_BLK(802CCD34);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803F9308, (u32 *)D_803F930C, 0x100);
    ENGINE_BLK(802CCD64);
    func_802C444C();
    ENGINE_BLK(802CCD6C);
}

/* hd.c's: put back on the ground where it is */
void func_802CCD80(void) {
    VS *vs = &D_803F9250;

    ENGINE_BLK(802CCD80);
    func_802A9A60(vs->unk52, D_803F92FC, D_803F92F8, D_803F9300, vs->unk4, &D_803F92FC, (s16 *)&vs->unk4C, 0xE, vs, engine_ctx(30));
    ENGINE_BLK(802CCE0C);
    func_802CD800(vs);
    ENGINE_BLK(802CCE14);
    func_802A133C(D_803F92F8, D_803F92FC, D_803F9300, 0xE, vs);
    ENGINE_BLK(802CCE40);
}

/* its light */
void func_802CCE90(void) {
    ENGINE_BLK(802CCE90);
    func_802ABD54(0xE, D_803F92F8, D_803F92FC, D_803F9300);
    ENGINE_BLK(802CCEC4);
}

/* each frame */
void func_802CD068(void) {
    VS *vs = &D_803F9250;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802CD068);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802CCE90();
    ENGINE_BLK(802CD0BC);
    func_802A75DC((u8 *)D_803F8F50, &D_803F92F8, &D_803F92FC, &D_803F9300, (u8 *)vs);
    ENGINE_BLK(802CD0E8);
    func_802C4724(0x54);
    ENGINE_BLK(802CD0F0);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802CD0FC);
        func_802CD578(vs);
    }
    ENGINE_BLK(802CD104);
    if (D_80367BFF != 0) {
        ENGINE_BLK(802CD114);
        func_802CB690(vs);
    }
    ENGINE_BLK(802CD11C);
    func_802CD9AC();
    ENGINE_BLK(802CD124);
    rate_i = func_802CD938(vs);
    ENGINE_BLK(802CD12C);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    ENGINE_BLK(802CD148);
    if (D_803F9314 == 0) {
        ENGINE_BLK(802CD158);
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x19, vs, &t3);
        ENGINE_BLK(802CD160);
    } else {
        ENGINE_BLK(802CD168);
        D_803F9314--;
    }
    ENGINE_BLK(802CD174);
    func_802A7FD8(0x4E20, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    ENGINE_BLK(802CD18C);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802CD198);
    func_802A843C(&vs->unk76, 1, 0xE, (s8 *)vs->unk96, vs->unk4, 800.0f, vs);
    ENGINE_BLK(802CD1AC);
    if (D_803F9313 != 0) {
        ENGINE_BLK(802CD1BC);
        func_802A7070((s16 *)&D_803F9310, vs);
    }
    ENGINE_BLK(802CD1C8);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803F92F8, &D_803F9300, rate, &z);
    ENGINE_BLK(802CD1E0);
    D_803ED40B = 1;
    /* ($s4 and $s7, which func_802A8768 reads too) */
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803F92F8, &D_803F9300, &D_803F92FC, 0xE, 0x320, 0x1F4, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802CD218);
    if (D_8035805C != 0) {
        ENGINE_BLK(802CD240);
        func_8029E558(D_803F8F50, D_803F9308, D_803F930C);
        ENGINE_BLK(802CD254);
    } else {
        ENGINE_BLK(802CD25C);
        func_8029E558(D_803F8F50, D_803F930C, D_803F9308);
    }
    ENGINE_BLK(802CD270);
    func_802CD800(vs);
    ENGINE_BLK(802CD278);
    ENGINE_LEAVE(8, 7);         /* $t0 and $t2, which func_8029A800 reads too */
    ENGINE_LEAVE(10, 0x96);
    func_8029A800(D_803F92F8, D_803F92FC, D_803F9300, D_80306440, 1, 1, vs->unk76, 0, 0xE, vs);
    ENGINE_BLK(802CD2C4);
    func_8029C52C(0xE, vs);
    ENGINE_BLK(802CD2CC);
    func_8029AA10();
    ENGINE_BLK(802CD2D4);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802CD2E4);
        D_803A7424 = 0;
        D_803F77D0 = D_803F8F50;
        func_802BE77C(0xE, vs);
        ENGINE_BLK(802CD304);
        if (D_803A7424 == 0) {
            ENGINE_BLK(802CD314);
            ENGINE_BLK(802CD4A0);
            D_803F9313 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    ENGINE_BLK(802CD31C);
    func_8029A914(vs);
    ENGINE_BLK(802CD324);
    D_803F9313 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802CD334);
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802CD344);
        h += 0xFFF;
    }
    ENGINE_BLK(802CD348);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802CD354);
        h = -h;
    }
    ENGINE_BLK(802CD358);
    if (!(h < 0x801)) {
        ENGINE_BLK(802CD364);
        h = 0xFFF - h;
    }
    ENGINE_BLK(802CD36C);
    ENGINE_BLK(802CD444);
    func_802A70D8(vs);
    ENGINE_BLK(802CD44C);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.16f, vs, &a1);

        ENGINE_BLK(802CD460);
        D_803F9310 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802CD474);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802CD47C);
    D_803F77D0 = D_803F8F50;
    func_802BE77C(0xE, vs);
    ENGINE_BLK(802CD498);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    ENGINE_BLK(802CD380);
    D_803F9313 = 0;
    if (D_8035805C != 0) {
        ENGINE_BLK(802CD398);
        a2 = D_803F930C;
        a3 = D_803F9308;
    } else {
        ENGINE_BLK(802CD3B4);
        a2 = D_803F9308;
        a3 = D_803F930C;
    }
    ENGINE_BLK(802CD3CC);
    func_802A768C((u8 *)D_803F8F50, &D_803F92F8, &D_803F92FC, &D_803F9300, (u32 *)a2, (u32 *)a3, 0x100, (u8 *)vs);
    ENGINE_BLK(802CD3F4);
    D_803F9314 = 5;
    v = vs->unk76;
    if (v >= 0) {
        ENGINE_BLK(802CD40C);
        if (v < 0x32) {
            ENGINE_BLK(802CD414);
            v = 0x32;
        }
    } else {
        ENGINE_BLK(802CD41C);
        if (!(v < -0x31)) {
            ENGINE_BLK(802CD428);
            v = -0x32;
        }
    }
    ENGINE_BLK(802CD42C);
    vs->unk76 = -v >> 1;
    func_802CD800(vs);
    ENGINE_BLK(802CD43C);

done:
    ENGINE_BLK(802CD4A8);
    D_803643E0 = D_803F92F8;
    D_803643E4 = D_803F92FC;
    D_803643E8 = D_803F9300;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0xE, vs);
    ENGINE_BLK(802CD524);
    engine_restore();
}

/* the wheels' sparks off rough ground, and the engine's sound */
REGS(gp)
void func_802CD578(VS *vs) {
    s32 s;

    ENGINE_BLK(802CD578);
    func_802CD660(vs);
    ENGINE_BLK(802CD588);
    if (D_803F9312 != 0) {
        ENGINE_BLK(802CD59C);
        D_803F9312--;
        goto sound;
    }
    ENGINE_BLK(802CD5A8);
    if (vs->unk96[3] == 0)
        goto sound;
    ENGINE_BLK(802CD5B8);
    D_803F9312 = 1;
    s = func_802A5ED0();
    ENGINE_BLK(802CD5CC);
    if (!(s < 0xF))
        goto sound;
    ENGINE_BLK(802CD5DC);
    func_802A6274(T(D_802C2954), 0x29810, 1, 0xE, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    ENGINE_BLK(802CD608);
    func_802A6274(T(D_802C2954), 0x29810, 1, 0xE, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
sound:
    ENGINE_BLK(802CD634);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802CD640);
        s = -s;
    }
    ENGINE_BLK(802CD644);
    func_802C4584((u32)s >> 5);
    ENGINE_BLK(802CD64C);
}

/* dust behind it on the ground */
REGS(gp)
void func_802CD660(VS *vs) {
    ENGINE_BLK(802CD660);
    engine_save(ENGINE_S0_S7_GP_FP | ENGINE_T0_T5 | 0x0F00C0FE, 0);
    if (vs->unk96[3] == 0)
        goto done;
    ENGINE_BLK(802CD6F0);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802CD700);
    if (!(vs->unk50 < 3))
        goto done;
    ENGINE_BLK(802CD710);
    if (vs->unk9B != 0)
        goto done;
    ENGINE_BLK(802CD71C);
    func_8027BE7C(3, vs->unk4[6], 0xFA, -0x190, -0x190, -0x190, D_803F92F8, D_803F9300, vs->unk4E, 3, 0x32, 0x32, 0);
    ENGINE_BLK(802CD778);
done:
    ENGINE_BLK(802CD77C);
    engine_restore();
}

/* the van's matrix, its vertices and its collision */
REGS(gp)
void func_802CD800(VS *vs) {
    u8 *model = D_803F9304, *buf;
    s32 *m, off;

    ENGINE_BLK(802CD800);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802CD830);
        m = (s32 *)(D_803F9308 + off);
    } else {
        ENGINE_BLK(802CD844);
        m = (s32 *)(D_803F930C + off);
    }
    ENGINE_BLK(802CD854);
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803F92F8, D_803F92FC, D_803F9300, 0x55F0, m);
    ENGINE_LEAVE(18, T(m));           /* ($s2: 62740's func_802ABBEC reads it) */
    ENGINE_BLK(802CD890);
    if (D_8035805C != 0) {
        ENGINE_BLK(802CD8A4);
        buf = D_803F9308;
    } else {
        ENGINE_BLK(802CD8B4);
        buf = D_803F930C;
    }
    ENGINE_BLK(802CD8C0);
    model = D_803F9304;
    func_8029C454(D_803F92F8, D_803F92FC, D_803F9300, 0xE, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802CD908);
    func_802ABBEC(0xE, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802CD928);
}

/* the turn rate: the speed / 3.6, or / 11 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802CD938(VS *vs) {
    f32 d;

    ENGINE_BLK(802CD938);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802CD94C);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802CD95C);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802CD96C);
    d = 3.6f;
    goto div;
air:
    ENGINE_BLK(802CD974);
    d = 11.0f;
div:
    ENGINE_BLK(802CD980);
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for the van */
void func_802CD9AC(void) {
    ENGINE_BLK(802CD9AC);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 3;
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802CCED4(s32 carrier) {
    CARRY_KEEP(802CCED4, 802CCF10, 802CCF30, 802CCF3C, &D_803F9250, D_803F92F8, D_803F9300);
}

REGS(a3)
void func_802CCF60(s32 carrier) {
    CARRY_MOVE(802CCF60, 802CCF9C, 802CCFB0, 802CCFB8, 802CD018, 802CD020, 802CD04C, &D_803F9250, &D_803F92F8, &D_803F92FC, &D_803F9300, 0xE, 0x320, 0x1F4,
               D_803ED40B = 1, func_802CD9AC(), func_802CD800(&D_803F9250));
}
