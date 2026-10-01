/*
 * hd_code 8AEE0 (us.v11 0x802CF6A0-0x802D2570): the other hotrod
 * (VEHICLE_STARSKI) and the Cyclone Suit (VEHICLE_MINIMAGOO), as native C
 * (engine.h).
 *
 * The hotrod's parts are D_803FC200, its state D_803FC500 and its position
 * D_803FC5A8..B0: it is 72B80's American Dream (func_802B7340..8424) with
 * its own numbers, function for function (func_802CF6A0 its setup,
 * func_802CFDE8 its frame).
 *
 * The suit's parts are D_803FC5D0, its state D_803FC8D0 and its position
 * D_803FC978..80: it is Thunderfist's code (6C5E0) with a third leg (part
 * 6, unkA2 2 at the higher speeds) and its own numbers.  func_802D07E0
 * sets it up, func_802D0F98 runs it each frame.
 *
 * Still translated: func_802CFC54 and func_802CFCE0, what 62740's
 * collision dispatch calls for the hotrod (they go native with that), and
 * the three functions nothing calls: func_802D0E44 (the suit put back on
 * the ground), func_802D24F8 (its state saved) and func_802D2550 (an mtc0
 * to Compare).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* the hotrod's .bss (asm/data/hd_code/8AEE0.bss.s) */
extern Part D_803FC200[32];
extern VS D_803FC500;
extern s32 D_803FC5A8, D_803FC5AC, D_803FC5B0;  /* x, y, z */
extern u8 *PTR32 D_803FC5B4;                    /* its model file */
extern u8 *PTR32 D_803FC5B8;                    /* two 0x100-byte buffers, one per frame */
extern u8 *PTR32 D_803FC5BC;
extern u16 D_803FC5C0;                          /* the heading it turns to (with D_803A7425) */
extern u8 D_803FC5C2;                           /* frames until the next sparks */
extern s8 D_803FC5C3;                           /* turning to it */
extern u8 D_803FC5C4;                           /* frames without the gears after a hit */
/* the suit's */
extern Part D_803FC5D0[32];
extern VS D_803FC8D0;
extern s32 D_803FC978, D_803FC97C, D_803FC980;  /* x, y, z */
extern u8 *PTR32 D_803FC984;                    /* its model file */
extern u8 *PTR32 D_803FC988;                    /* two 0x1400-byte buffers, one per frame */
extern u8 *PTR32 D_803FC98C;
extern SndState *PTR32 D_803FC990;              /* its sound while it is in */
extern u16 D_803FC994;                          /* the heading it turns to (with D_803A7425) */
extern s16 D_803FC996;                          /* the step sounds' last frame */
extern s8 D_803FC998;                           /* turning to it */
extern u8 D_803FC999;
extern u8 D_803FC99A;                           /* facing the camera's way (with D_803A7425) */

extern u8 D_80306460[], D_80306470[];           /* their bounce records */
extern u8 D_802C22D0[];                         /* the suit's light (56040's list) */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern u8 D_80370C1A, D_80370C1B, D_80370C1C, D_80370C1D, D_80370C35;
extern u8 D_803F7804;
extern SndState *PTR32 D_803F7844;              /* the rolling sound */
extern Part *PTR32 D_803F77D0;
extern s32 D_803643E4, D_803643E8;
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);
void func_80278EB0(s32 a, f32 b, s32 c);
void func_802794A4(void);
s32 func_8026A8E0(s32 lo, s32 hi);

void func_802CFDE8(void);
void func_802CFC10(void);
REGS(gp)
void func_802D02F8(VS *vs);
REGS(gp)
void func_802D0438(VS *vs);
REGS(gp)
void func_802D05D8(VS *vs);
REGS(gp -> s3)
s32 func_802D0710(VS *vs);
REGS()
void func_802D0784(void);
void func_802D0F98(void);
void func_802D0F54(void);
REGS(gp)
void func_802D1360(VS *vs);
REGS(gp)
void func_802D22F4(VS *vs);
REGS(gp -> s3)
s32 func_802D2444(VS *vs);
void func_802D249C(void);

#define T(p) ((s32)(p))

/* ---- the hotrod ---------------------------------------------------------- */

#define P D_803FC200

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802CF6A0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803FC500;
    u8 *buf;
    s16 *r;
    s32 *s3, avg;

    ENGINE_BLK(802CF6A0);
    engine_save(ENGINE_T0_T5, 0);
    D_803FC5B4 = model;
    buf = D_80358070;
    D_803FC5B8 = buf;
    D_803FC5BC = buf + 0x100;
    D_80358070 = buf + 0x200;
    func_802A1388(0xF, 0, D_803FC5B8, D_803FC5BC, model);
    ENGINE_BLK(802CF720);
    func_802A754C(vs);
    ENGINE_BLK(802CF72C);
    vs->unk52[0] = 0xAF;
    vs->unk52[1] = 0xFA;
    vs->unk52[2] = -0xAF;
    vs->unk52[3] = 0xFA;
    vs->unk52[4] = 0xAF;
    vs->unk52[5] = -0xFA;
    vs->unk5E[0] = 0x140;
    vs->unk5E[1] = 0x1F4;
    vs->unk5E[2] = -0x140;
    vs->unk5E[3] = 0x1F4;
    vs->unk5E[4] = 0x140;
    vs->unk5E[5] = -0x1F4;
    D_803FC5A8 = x;
    D_803FC5AC = y;
    D_803FC5B0 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    s3 = func_802A992C(vs->unk52, D_803FC5AC, x, z, vs->unk4, &D_803FC5AC, (s16 *)&vs->unk4C, 0xF, vs, engine_ctx(30),
                       &avg);
    ENGINE_LEAVE(19, T(s3));
    ENGINE_LEAVE(21, avg);
    ENGINE_BLK(802CF7DC);
    func_8029F85C(P, D_803FC5B4, D_803FC5B8, D_803FC5BC);
    ENGINE_BLK(802CF818);
    func_802A039C(0, 100, P);
    ENGINE_BLK(802CF82C);
    func_802A03D4(0, 0, P);
    ENGINE_BLK(802CF840);
    func_802A040C(0, 0, P);
    ENGINE_BLK(802CF854);
    func_802A0480(0, 0, P, 0.0f);
    ENGINE_BLK(802CF86C);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802CF880);
    func_8029E558(P, D_803FC5B8, D_803FC5BC);
    ENGINE_BLK(802CF894);
    func_802A0320(0, P);
    ENGINE_BLK(802CF8A4);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802CF8B8);
    func_8029E558(P, D_803FC5BC, D_803FC5B8);
    ENGINE_BLK(802CF8CC);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0x118, r[5] = 4;
    r[6] = 0x118, r[7] = 0x12C, r[8] = 5;
    r[9] = 0x12C, r[10] = 0x136, r[11] = 3;
    r[12] = 0x136, r[13] = 0x140, r[14] = 2;
    D_803FC5C2 = 0;
    D_803FC5C3 = 0;
    D_803FC5C4 = 0;
    model = D_803FC5B4;
    func_8029C354(0xF, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x32C8);
    ENGINE_BLK(802CF988);
    func_80258230(0xF, 0x3C, 0x19, 0x19);
    ENGINE_BLK(802CF9A0);
    vs->unk9A = 1;
    func_802CFDE8();
    ENGINE_BLK(802CF9B0);
    vs->unk9A = 0;
    model = D_803FC5B4;
    func_802AA838(D_803FC5BC, D_803FC5B8, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802CF9E8);
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(17, T(vs->unk4));
    ENGINE_LEAVE(18, T(&D_803FC5AC));
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(22, T(D_803FC5B8));
    ENGINE_LEAVE(23, T(D_803FC5BC));
}

/* hd.c's: the player gets in */
void func_802CFA0C(void) {
    VS *vs = &D_803FC500;

    ENGINE_BLK(802CFA0C);
    vs->unk96[3] = 0;
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xCE);
    ENGINE_BLK(802CFA48);
    ENGINE_LEAVE(28, T(vs));
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802CFA58(void) {
    VS *vs = &D_803FC500;
    u8 r = 0;

    ENGINE_BLK(802CFA58);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802CFA7C);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802CFA8C);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802CFA9C);
                r = 1;
            }
        }
    }
    ENGINE_BLK(802CFAA0);
    return r;
}

/* hd.c's: the player gets out */
void func_802CFAB4(void) {
    VS *vs = &D_803FC500;

    ENGINE_BLK(802CFAB4);
    engine_save(0x10000000, 0);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803FC5B8, (u32 *)D_803FC5BC, 0x100);
    ENGINE_BLK(802CFAE4);
    func_802C444C();
    ENGINE_BLK(802CFAEC);
    engine_restore();
}

/* hd.c's: put back on the ground where it is */
void func_802CFB00(void) {
    VS *vs = &D_803FC500;

    ENGINE_BLK(802CFB00);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802A9A60(vs->unk52, D_803FC5AC, D_803FC5A8, D_803FC5B0, vs->unk4, &D_803FC5AC, (s16 *)&vs->unk4C, 0xF, vs,
                  engine_ctx(30));
    ENGINE_BLK(802CFB8C);
    func_802D05D8(vs);
    ENGINE_BLK(802CFB94);
    func_802A133C(D_803FC5A8, D_803FC5AC, D_803FC5B0, 0xF, vs);
    ENGINE_BLK(802CFBC0);
    engine_restore();
}

/* its light */
void func_802CFC10(void) {
    ENGINE_BLK(802CFC10);
    func_802ABD54(0xF, D_803FC5A8, D_803FC5AC, D_803FC5B0);
    ENGINE_BLK(802CFC44);
}

/* each frame */
void func_802CFDE8(void) {
    VS *vs = &D_803FC500;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802CFDE8);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802CFC10();
    ENGINE_BLK(802CFE3C);
    func_802A75DC((u8 *)P, &D_803FC5A8, &D_803FC5AC, &D_803FC5B0, (u8 *)vs);
    ENGINE_BLK(802CFE68);
    func_802C4724(0x76);
    ENGINE_BLK(802CFE70);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802CFE7C);
        func_802D02F8(vs);
    }
    ENGINE_BLK(802CFE84);
    if (D_80367BFF != 0) {
        ENGINE_BLK(802CFE94);
        func_802CB690(vs);
    }
    ENGINE_BLK(802CFE9C);
    func_802D0784();
    ENGINE_BLK(802CFEA4);
    rate_i = func_802D0710(vs);
    ENGINE_BLK(802CFEAC);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    ENGINE_BLK(802CFEC8);
    if (D_803FC5C4 == 0) {
        ENGINE_BLK(802CFED8);
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x10, vs, &t3);
        ENGINE_BLK(802CFEE0);
    } else {
        ENGINE_BLK(802CFEE8);
        D_803FC5C4--;
    }
    ENGINE_BLK(802CFEF4);
    func_802A7FD8(0x1F40, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    ENGINE_BLK(802CFF0C);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802CFF18);
    func_802A843C(&vs->unk76, 1, 0xF, (s8 *)vs->unk96, vs->unk4, 500.0f, vs);
    ENGINE_BLK(802CFF2C);
    if (D_803FC5C3 != 0) {
        ENGINE_BLK(802CFF3C);
        func_802A7070((s16 *)&D_803FC5C0, vs);
    }
    ENGINE_BLK(802CFF48);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803FC5A8, &D_803FC5B0, rate, &z);
    ENGINE_BLK(802CFF60);
    D_803ED40B = 1;
    /* ($s4 and $s7, which func_802A8768 reads too) */
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803FC5A8, &D_803FC5B0, &D_803FC5AC, 0xF, 0x1F4, 0x15E, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802CFF98);
    if (D_8035805C != 0) {
        ENGINE_BLK(802CFFC0);
        func_8029E558(P, D_803FC5B8, D_803FC5BC);
        ENGINE_BLK(802CFFD4);
    } else {
        ENGINE_BLK(802CFFDC);
        func_8029E558(P, D_803FC5BC, D_803FC5B8);
    }
    ENGINE_BLK(802CFFF0);
    func_802D05D8(vs);
    ENGINE_BLK(802CFFF8);
    ENGINE_LEAVE(8, 7);         /* $t0 and $t2, which func_8029A800 reads too */
    ENGINE_LEAVE(10, 0x96);
    func_8029A800(D_803FC5A8, D_803FC5AC, D_803FC5B0, D_80306460, 1, 1, vs->unk76, 0, 0xF, vs);
    ENGINE_BLK(802D0044);
    func_8029C52C(0xF, vs);
    ENGINE_BLK(802D004C);
    func_8029AA10();
    ENGINE_BLK(802D0054);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802D0064);
        D_803A7424 = 0;
        D_803F77D0 = P;
        func_802BE77C(0xF, vs);
        ENGINE_BLK(802D0084);
        if (D_803A7424 == 0) {
            ENGINE_BLK(802D0094);
            ENGINE_BLK(802D0220);
            D_803FC5C3 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    ENGINE_BLK(802D009C);
    func_8029A914(vs);
    ENGINE_BLK(802D00A4);
    D_803FC5C3 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802D00B4);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802D00C4);
        h += 0xFFF;
    }
    ENGINE_BLK(802D00C8);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802D00D4);
        h = -h;
    }
    ENGINE_BLK(802D00D8);
    if (!(h < 0x801)) {
        ENGINE_BLK(802D00E4);
    }
    ENGINE_BLK(802D00EC);
    ENGINE_BLK(802D01C4);
    func_802A70D8(vs);
    ENGINE_BLK(802D01CC);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.16f, &a1);

        ENGINE_BLK(802D01E0);
        D_803FC5C0 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802D01F4);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802D01FC);
    D_803F77D0 = P;
    func_802BE77C(0xF, vs);
    ENGINE_BLK(802D0218);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    ENGINE_BLK(802D0100);
    D_803FC5C3 = 0;
    if (D_8035805C != 0) {
        ENGINE_BLK(802D0118);
        a2 = D_803FC5BC;
        a3 = D_803FC5B8;
    } else {
        ENGINE_BLK(802D0134);
        a2 = D_803FC5B8;
        a3 = D_803FC5BC;
    }
    ENGINE_BLK(802D014C);
    func_802A768C((u8 *)P, &D_803FC5A8, &D_803FC5AC, &D_803FC5B0, (u32 *)a2, (u32 *)a3, 0x100, (u8 *)vs);
    ENGINE_BLK(802D0174);
    D_803FC5C4 = 5;
    v = vs->unk76;
    if (v >= 0) {
        ENGINE_BLK(802D018C);
        if (v < 0x32) {
            ENGINE_BLK(802D0194);
            v = 0x32;
        }
    } else {
        ENGINE_BLK(802D019C);
        if (!(v < -0x31)) {
            ENGINE_BLK(802D01A8);
            v = -0x32;
        }
    }
    ENGINE_BLK(802D01AC);
    vs->unk76 = -v >> 1;
    func_802D05D8(vs);
    ENGINE_BLK(802D01BC);

done:
    ENGINE_BLK(802D0228);
    D_803643E0 = D_803FC5A8;
    D_803643E4 = D_803FC5AC;
    D_803643E8 = D_803FC5B0;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0xF, vs);
    ENGINE_BLK(802D02A4);
    engine_restore();
}

/* the dust, the wheels' sparks off rough ground, and the engine's sound */
REGS(gp)
void func_802D02F8(VS *vs) {
    s32 s;

    ENGINE_BLK(802D02F8);
    func_802D0438(vs);
    ENGINE_BLK(802D0308);
    if (D_803FC5C2 != 0) {
        ENGINE_BLK(802D031C);
        D_803FC5C2--;
        goto sound;
    }
    ENGINE_BLK(802D0328);
    if (vs->unk96[3] == 0)
        goto sound;
    ENGINE_BLK(802D0338);
    D_803FC5C2 = 1;
    s = func_802A5ED0();
    ENGINE_BLK(802D034C);
    if (!(s < 0xF))
        goto sound;
    ENGINE_BLK(802D035C);
    func_802A6274(T(D_802C2954), 0x29810, 1, 0xF, 1, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
    ENGINE_BLK(802D0388);
    func_802A6274(T(D_802C2954), 0x29810, 1, 0xF, 2, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
    ENGINE_BLK(802D03B4);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 0xF, 3, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
    ENGINE_BLK(802D03E0);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 0xF, 4, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
sound:
    ENGINE_BLK(802D040C);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802D0418);
        s = -s;
    }
    ENGINE_BLK(802D041C);
    func_802C4584((u32)s >> 5);
    ENGINE_BLK(802D0424);
}

/* dust behind it on the ground */
REGS(gp)
void func_802D0438(VS *vs) {
    ENGINE_BLK(802D0438);
    engine_save(0x5FFFFFFE, 0);
    if (vs->unk96[3] == 0)
        goto done;
    ENGINE_BLK(802D04C8);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802D04D8);
    if (!(vs->unk50 < 3))
        goto done;
    ENGINE_BLK(802D04E8);
    if (vs->unk9B != 0)
        goto done;
    ENGINE_BLK(802D04F4);
    func_8027BE7C(3, vs->unk4[6], 0xFA, -0x190, -0x190, -0x190, D_803FC5A8, D_803FC5B0, vs->unk4E, 3, 0x32, 0x32, 0);
    ENGINE_BLK(802D0550);
done:
    ENGINE_BLK(802D0554);
    engine_restore();
}

/* its matrix, its vertices and its collision */
REGS(gp)
void func_802D05D8(VS *vs) {
    u8 *model = D_803FC5B4, *buf;
    s32 *m, off;

    ENGINE_BLK(802D05D8);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802D0608);
        m = (s32 *)(D_803FC5B8 + off);
    } else {
        ENGINE_BLK(802D061C);
        m = (s32 *)(D_803FC5BC + off);
    }
    ENGINE_BLK(802D062C);
    D_803ED390[1] = vs->unk4C;
    ENGINE_LEAVE(20, D_803FC5A8);
    ENGINE_LEAVE(21, D_803FC5AC);
    ENGINE_LEAVE(22, D_803FC5B0);
    ENGINE_LEAVE(23, 0x32C8);
    func_802AA764(D_803FC5A8, D_803FC5AC, D_803FC5B0, 0x32C8, m);
    ENGINE_LEAVE(18, T(m));           /* (its $s2, as the glue would) */
    ENGINE_BLK(802D0668);
    if (D_8035805C != 0) {
        ENGINE_BLK(802D067C);
        buf = D_803FC5B8;
    } else {
        ENGINE_BLK(802D068C);
        buf = D_803FC5BC;
    }
    ENGINE_BLK(802D0698);
    model = D_803FC5B4;
    ENGINE_LEAVE(11, T(model));
    func_8029C454(D_803FC5A8, D_803FC5AC, D_803FC5B0, 0xF, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802D06E0);
    ENGINE_LEAVE(11, T(model));
    func_802ABBEC(0xF, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802D0700);
}

/* the turn rate: the speed / 3.6, or / 11 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802D0710(VS *vs) {
    f32 d;

    ENGINE_BLK(802D0710);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802D0724);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802D0734);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802D0744);
    d = 3.6f;
    goto div;
air:
    ENGINE_BLK(802D074C);
    d = 11.0f;
div:
    ENGINE_BLK(802D0758);
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for it */
REGS()
void func_802D0784(void) {
    ENGINE_BLK(802D0784);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 3;
}

/* ---- the suit ----------------------------------------------------------- */

#undef P
#define P D_803FC5D0

/* part i's frame (func_802A04BC's v1), and its a0 (unk11) and t1 (unk13) */
static s32 part(s32 i, s32 *a0, s32 *t1) {
    s32 f12, f14, fC, fE, a0_, t1_;
    f32 f4;
    s32 r = func_802A04BC(i, P, &a0_, &f12, &f14, &fC, &fE, &t1_, &f4);

    if (a0)
        *a0 = a0_;
    if (t1)
        *t1 = t1_;
    return r;
}

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802D07E0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 avg;
    VS *vs = &D_803FC8D0;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802D07E0);
    engine_save(ENGINE_T0_T5, 0);
    D_803FC984 = model;
    buf = D_80358070;
    D_803FC988 = buf;
    D_803FC98C = buf + 0x1400;
    D_80358070 = buf + 0x2800;
    func_802A1388(0x10, 1, D_803FC988, D_803FC98C, model);
    ENGINE_BLK(802D0860);
    func_802A754C(vs);
    ENGINE_BLK(802D086C);
    vs->unk52[0] = 0;
    vs->unk52[1] = 0;
    vs->unk52[2] = 0;
    vs->unk52[3] = 0;
    vs->unk52[4] = 0;
    vs->unk52[5] = 0;
    vs->unk5E[0] = 0x50;
    vs->unk5E[1] = 0x50;
    vs->unk5E[2] = -0x50;
    vs->unk5E[3] = 0x50;
    vs->unk5E[4] = 0x50;
    vs->unk5E[5] = -0x50;
    D_803FC978 = x;
    D_803FC97C = y;
    D_803FC980 = z;
    vs->unk4C = heading;
    vs->unkA1 = 0;
    vs->unkA2 = 0;
    vs->unk4E = heading;
    vs->unk74 = heading;
    vs->unkA3 = 1;
    func_802A992C(vs->unk52, D_803FC97C, x, z, vs->unk4, &D_803FC97C, (s16 *)&vs->unk4C, 0x10, vs, engine_ctx(30), &avg);
    ENGINE_BLK(802D0914);
    func_8029F85C(P, D_803FC984, D_803FC988, D_803FC98C);
    ENGINE_BLK(802D0950);
    func_802A039C(0, 100, P);
    ENGINE_BLK(802D0964);
    func_802A03D4(0, 0, P);
    ENGINE_BLK(802D0978);
    func_802A040C(0, 0, P);
    ENGINE_BLK(802D098C);
    func_802A0480(0, 0, P, 0.0f);
    ENGINE_BLK(802D09A4);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802D09B8);
    func_8029E558(P, D_803FC988, D_803FC98C);
    ENGINE_BLK(802D09CC);
    func_802A0320(0, P);
    ENGINE_BLK(802D09DC);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802D09F0);
    func_8029E558(P, D_803FC98C, D_803FC988);
    ENGINE_BLK(802D0A04);
    r = vs->unk78;
    r[0] = -0x64, r[1] = 0, r[2] = 4;
    r[3] = 0, r[4] = 0xA0, r[5] = 4;
    r[6] = 0, r[7] = 0xA0, r[8] = 4;
    r[9] = 0, r[10] = 0xA0, r[11] = 4;
    r[12] = 0, r[13] = 0xA0, r[14] = 4;
    D_803FC998 = 0;
    D_803FC99A = 0;
    D_803FC999 = 0;
    func_8029C354(0x10, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x2134);
    ENGINE_BLK(802D0ABC);
    func_80258230(0x10, 0x64, 0x2D, 0x2D);
    ENGINE_BLK(802D0AD4);
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802D0AF4);
    func_802A0290(1, 1, P);
    ENGINE_BLK(802D0B08);
    vs->unk9A = 1;
    func_802D0F98();
    ENGINE_BLK(802D0B14);
    vs->unk9A = 0;
    func_802A7764((u32 *)D_803FC98C, (u32 *)D_803FC988, 0x1400);
    ENGINE_BLK(802D0B30);
    func_802AA838(D_803FC98C, D_803FC988, *(s32 *)(D_803FC984 + *(s32 *)(D_803FC984 + 0x18) + 4));
    ENGINE_BLK(802D0B64);
    D_803F7844 = NULL;
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(18, T(&D_803FC97C));
    ENGINE_LEAVE(22, T(D_803FC988));
    ENGINE_LEAVE(23, T(D_803FC98C));
}

/* hd.c's: whether it can be left: on the ground and walking */
u8 func_802D0B90(void) {
    VS *vs = &D_803FC8D0;
    u8 r = 0;

    ENGINE_BLK(802D0B90);
    engine_save(ENGINE_GPR(28), 0);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802D0BB4);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802D0BC4);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802D0BD4);
                if (vs->unkA1 == 0) {
                    ENGINE_BLK(802D0BE0);
                    r = 1;
                }
            }
        }
    }
    ENGINE_BLK(802D0BE4);
    engine_restore();
    return r;
}

/* hd.c's: the player gets out */
void func_802D0BF8(void) {
    VS *vs = &D_803FC8D0;

    ENGINE_BLK(802D0BF8);
    engine_save(ENGINE_GPR(28), 0);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803FC988, (u32 *)D_803FC98C, 0x1400);
    ENGINE_BLK(802D0C2C);
    func_802A02E4(0x1F, P);
    ENGINE_BLK(802D0C3C);
    func_802C444C();
    ENGINE_BLK(802D0C44);
    func_802608C8(D_803FC990);
    ENGINE_BLK(802D0C50);
    engine_restore();
}

/* its light */
void func_802D0F54(void) {
    ENGINE_BLK(802D0F54);
    func_802ABD54(0x10, D_803FC978, D_803FC97C, D_803FC980);
    ENGINE_BLK(802D0F88);
}

/* each frame */
void func_802D0F98(void) {
    VS *vs = &D_803FC8D0;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_BLK(802D0F98);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802D0F54();
    ENGINE_BLK(802D0FF4);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802D1000);
        func_802D1360(vs);
    }
    ENGINE_BLK(802D1008);
    func_802D249C();
    ENGINE_BLK(802D1010);
    rate_i = func_802D2444(vs);
    ENGINE_BLK(802D1018);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    ENGINE_BLK(802D1034);
    func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0xC, vs, &t3);
    ENGINE_BLK(802D103C);
    if (vs->unkA1 == 0) {
        ENGINE_BLK(802D1048);
        func_802A77D0(vs);
    }
    ENGINE_BLK(802D1050);
    func_802A7FD8(0x59D8, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 0, vs);
    ENGINE_BLK(802D1068);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802D1074);
    func_802A843C(&vs->unk76, 0, 0x10, (s8 *)vs->unk96, vs->unk4, 120.0f, vs);
    ENGINE_BLK(802D1088);
    if (D_803FC998 != 0) {
        ENGINE_BLK(802D1098);
        func_802A7070((s16 *)&D_803FC994, vs);
    }
    ENGINE_BLK(802D10A4);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803FC978, &D_803FC980, rate, &z);
    ENGINE_BLK(802D10BC);
    D_803ED40B = 0;
    /* ($s4 and $s7, which func_802A8768 reads too) */
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803FC978, &D_803FC980, &D_803FC97C, 0x10, 0x78, 0x78, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802D10F0);
    if (D_8035805C != 0) {
        ENGINE_BLK(802D1118);
        func_8029E558(P, D_803FC988, D_803FC98C);
        ENGINE_BLK(802D112C);
    } else {
        ENGINE_BLK(802D1134);
        func_8029E558(P, D_803FC98C, D_803FC988);
    }
    ENGINE_BLK(802D1148);
    func_802D22F4(vs);
    ENGINE_BLK(802D1150);
    func_8029A800(D_803FC978, D_803FC97C, D_803FC980, D_80306470, 0, 0, vs->unk76, 0, 0x10, vs);
    ENGINE_BLK(802D1194);
    func_8029C52C(0x10, vs);
    ENGINE_BLK(802D119C);
    func_8029AA10();
    ENGINE_BLK(802D11A4);
    D_803F77D0 = P;
    func_802BE77C(0x10, vs);
    ENGINE_BLK(802D11C0);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802D1280);
        D_803FC998 = 0;
        D_803FC99A = 0;
        goto done;
    }
    /* D_803A7425: turned toward the camera's heading; D_803FC99A when it
       is within 0x190 of it */
    ENGINE_BLK(802D11D0);
    func_8029A914(vs);
    ENGINE_BLK(802D11D8);
    D_803FC998 = 1;
    D_803FC99A = 0;
    v = func_802A6F6C();
    ENGINE_BLK(802D11F0);
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802D1200);
        h += 0xFFF;
    }
    ENGINE_BLK(802D1204);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802D1210);
        h = -h;
    }
    ENGINE_BLK(802D1214);
    if (!(h < 0x801)) {
        ENGINE_BLK(802D1220);
        h = 0xFFF - h;
    }
    ENGINE_BLK(802D1228);
    if (h < 0x190) {
        ENGINE_BLK(802D1234);
        D_803FC99A = 1;
    }
    ENGINE_BLK(802D1240);
    func_802A70D8(vs);
    ENGINE_BLK(802D1248);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.25f, &a1);

        ENGINE_BLK(802D125C);
        D_803FC994 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802D1270);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802D1278);
done:
    ENGINE_BLK(802D1290);
    D_803643E0 = D_803FC978;
    D_803643E4 = D_803FC97C;
    D_803643E8 = D_803FC980;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0x10, vs);
    ENGINE_BLK(802D130C);
    engine_restore();
}

/* the suit's matrix (turned a quarter), its vertices and its collision */
REGS(gp)
void func_802D22F4(VS *vs) {
    u8 *model = D_803FC984, *buf;
    s32 *m, off, h;

    ENGINE_BLK(802D22F4);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802D2324);
        m = (s32 *)(D_803FC988 + off);
    } else {
        ENGINE_BLK(802D2338);
        m = (s32 *)(D_803FC98C + off);
    }
    ENGINE_BLK(802D2348);
    h = (u16)vs->unk4C + 0x400;
    if (!(h < 0x1000)) {
        ENGINE_BLK(802D237C);
        h -= 0xFFF;
    }
    ENGINE_BLK(802D2380);
    D_803ED390[1] = h;
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    ENGINE_LEAVE(20, D_803FC978);
    ENGINE_LEAVE(21, D_803FC97C);
    ENGINE_LEAVE(22, D_803FC980);
    ENGINE_LEAVE(23, 0x2134);
    func_802AA764(D_803FC978, D_803FC97C, D_803FC980, 0x2134, m);
    ENGINE_LEAVE(18, T(m));           /* (its $s2, as the glue would) */
    ENGINE_BLK(802D239C);
    if (D_8035805C != 0) {
        ENGINE_BLK(802D23B0);
        buf = D_803FC988;
    } else {
        ENGINE_BLK(802D23C0);
        buf = D_803FC98C;
    }
    ENGINE_BLK(802D23CC);
    model = D_803FC984;
    ENGINE_LEAVE(11, T(model));
    func_8029C454(D_803FC978, D_803FC97C, D_803FC980, 0x10, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802D2414);
    ENGINE_LEAVE(11, T(model));
    func_802ABBEC(0x10, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802D2434);
}

/* the turn rate: none standing, 5 rolling or landing, else 0x6E */
REGS(gp -> s3)
s32 func_802D2444(VS *vs) {
    s32 r = 0;

    ENGINE_BLK(802D2444);
    if (vs->unk76 != 0) {
        ENGINE_BLK(802D2458);
        if (vs->unkA1 == 2)
            goto rolling;
        ENGINE_BLK(802D2468);
        if (vs->unkA1 == 1)
            goto rolling;
        ENGINE_BLK(802D2470);
        if (vs->unkA1 == 3)
            goto rolling;
        ENGINE_BLK(802D2478);
        if (vs->unkA1 == 4)
            goto rolling;
        ENGINE_BLK(802D2480);
        r = 0x6E;
        goto done;
    rolling:
        ENGINE_BLK(802D2488);
        r = 5;
    }
done:
    ENGINE_BLK(802D248C);
    return r;
}

/* the camera's distance and speed for the suit */
void func_802D249C(void) {
    ENGINE_BLK(802D249C);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x28;
    D_803ED3F7 = 3;
}

/* and back */
void func_802D2524(u8 *src) {
    ENGINE_BLK(802D2524);
    func_802AC85C(src, (u8 *)&D_803FC8D0, (u32 *)&D_803FC978);
    ENGINE_BLK(802D2540);
}

/* hd.c's (and 17210.c's): the player gets in: its sound, its light and its
   arms */
void func_802D0C68(void) {
    VS *vs = &D_803FC8D0;

    ENGINE_BLK(802D0C68);
    vs->unk96[3] = 0;
    D_8036444C = 0x7D0;
    D_80364450 = -0x3E8;
    func_80260650(D_80367738, 0x50, &D_803FC990);
    ENGINE_BLK(802D0CB8);
    func_802A05D0(D_802C22D0, 0x64);
    ENGINE_BLK(802D0CCC);
    func_802A05F8(D_802C22D0, 0);
    ENGINE_BLK(802D0CDC);
    func_802A0620(D_802C22D0, 0);
    ENGINE_BLK(802D0CEC);
    func_802A0508(D_802C22D0, -1);
    ENGINE_BLK(802D0CFC);
    func_802A039C(7, 2, P);
    ENGINE_BLK(802D0D10);
    func_802A040C(7, 1, P);
    ENGINE_BLK(802D0D24);
    func_802A0480(7, 1, P, 0.5f);
    ENGINE_BLK(802D0D40);
    func_802A039C(8, 2, P);
    ENGINE_BLK(802D0D54);
    func_802A040C(8, 1, P);
    ENGINE_BLK(802D0D68);
    func_802A0480(8, 1, P, 0.5f);
    ENGINE_BLK(802D0D84);
    func_802A039C(9, 4, P);
    ENGINE_BLK(802D0D98);
    func_802A040C(9, 1, P);
    ENGINE_BLK(802D0DAC);
    func_802A0480(9, 1, P, 0.5f);
    ENGINE_BLK(802D0DC8);
    func_802A0480(1, 1, P, 0.5f);
    ENGINE_BLK(802D0DE4);
    func_802A0480(5, 1, P, 0.5f);
    ENGINE_BLK(802D0E00);
    func_802A0480(6, 1, P, 0.5f);
    ENGINE_BLK(802D0E1C);
    func_802A0480(3, 0, P, 0.0f);
    ENGINE_BLK(802D0E34);
    ENGINE_LEAVE(28, T(vs));
}

/* the body's animation (part 0x1F) for the walk to a stop: the legs back
   together, then a random idle (7, 8 or 9) */
static void stand(VS *vs) {
    s32 r;

    ENGINE_BLK(802D1438);
    if (vs->unkA3 != 0) {
        ENGINE_BLK(802D1444);
        func_802A0360(7, 0, P, 0.0f);
        ENGINE_BLK(802D1460);
        r = part(0x1F, NULL, NULL);
        ENGINE_BLK(802D1470);
        if (r != 0) {
            ENGINE_BLK(802D1478);
            func_802A02E4(0x1F, P);
            ENGINE_BLK(802D1488);
            func_8029F9D4(0x1F, 7, P);
            ENGINE_BLK(802D149C);
            goto anim;
        }
        ENGINE_BLK(802D14A4);
        r = part(1, NULL, NULL);
        ENGINE_BLK(802D14B4);
        if (r != 0) {
            ENGINE_BLK(802D14BC);
            func_802A02E4(1, P);
            ENGINE_BLK(802D14CC);
            func_8029F9D4(1, 7, P);
            ENGINE_BLK(802D14E0);
            goto anim;
        }
        ENGINE_BLK(802D14E8);
        r = part(5, NULL, NULL);
        ENGINE_BLK(802D14F8);
        if (r != 0) {
            ENGINE_BLK(802D1500);
            func_802A02E4(5, P);
            ENGINE_BLK(802D1510);
            func_8029F9D4(5, 7, P);
            ENGINE_BLK(802D1524);
            goto anim;
        }
        ENGINE_BLK(802D152C);
        r = part(6, NULL, NULL);
        ENGINE_BLK(802D153C);
        if (r != 0) {
            ENGINE_BLK(802D1544);
            func_802A02E4(6, P);
            ENGINE_BLK(802D1554);
            func_8029F9D4(6, 7, P);
            ENGINE_BLK(802D1568);
            goto anim;
        }
        ENGINE_BLK(802D1570);
        func_802A0360(1, 0, P, 0.0f);
        ENGINE_BLK(802D158C);
        func_8029F9D4(1, 7, P);
    anim:
        ENGINE_BLK(802D15A0);
        func_802A039C(0x1F, 0x1E, P);
        ENGINE_BLK(802D15B4);
        func_802A03D4(0x1F, 0, P);
        ENGINE_BLK(802D15C8);
        func_802A040C(0x1F, 0, P);
        ENGINE_BLK(802D15DC);
        func_802A0290(0x1F, 1, P);
    }
    ENGINE_BLK(802D15F0);
    vs->unkA3 = 0;
    r = part(0x1F, NULL, NULL);
    ENGINE_BLK(802D1608);
    if (r == 1)
        return;
    ENGINE_BLK(802D1614);
    r = part(7, NULL, NULL);
    ENGINE_BLK(802D1624);
    if (r == 1)
        return;
    ENGINE_BLK(802D1630);
    r = part(8, NULL, NULL);
    ENGINE_BLK(802D1640);
    if (r == 1)
        return;
    ENGINE_BLK(802D164C);
    r = part(9, NULL, NULL);
    ENGINE_BLK(802D165C);
    if (r == 1)
        return;
    ENGINE_BLK(802D1668);
    r = func_8026A8E0(0, 0x1E);
    ENGINE_BLK(802D1674);
    if (r != 0)
        return;
    ENGINE_BLK(802D167C);
    r = func_8026A8E0(0, 2);
    ENGINE_BLK(802D1688);
    if (r == 0) {
        ENGINE_BLK(802D1708);
        func_802A0360(7, 0, P, 0.0f);
        ENGINE_BLK(802D1724);
        func_802A0290(7, 1, P);
        ENGINE_BLK(802D1738);
    } else {
        ENGINE_BLK(802D1690);
        if (r == 1) {
            ENGINE_BLK(802D16D0);
            func_802A0360(8, 0, P, 0.0f);
            ENGINE_BLK(802D16EC);
            func_802A0290(8, 1, P);
            ENGINE_BLK(802D1700);
        } else {
            ENGINE_BLK(802D1698);
            func_802A0360(9, 0, P, 0.0f);
            ENGINE_BLK(802D16B4);
            func_802A0290(9, 1, P);
            ENGINE_BLK(802D16C8);
        }
    }
}

/* walking: the idle animations stopped, the walk started; rolling up with
   a C button or Z at speed 0x96; otherwise a step of the leg in front (1,
   5 or 6 by the speed) */
static void walk(VS *vs) {
    s32 s, a0, k, r;

    ENGINE_BLK(802D1740);
    if (vs->unkA3 != 1) {
        ENGINE_BLK(802D1750);
        func_802A0360(1, 0, P, 0.0f);
        ENGINE_BLK(802D176C);
        r = part(7, NULL, NULL);
        ENGINE_BLK(802D177C);
        if (r != 0) {
            ENGINE_BLK(802D1784);
            func_8029F9D4(7, 1, P);
            ENGINE_BLK(802D1798);
            func_802A02E4(7, P);
            ENGINE_BLK(802D17A8);
            goto anim;
        }
        ENGINE_BLK(802D17B0);
        r = part(8, NULL, NULL);
        ENGINE_BLK(802D17C0);
        if (r != 0) {
            ENGINE_BLK(802D17C8);
            func_8029F9D4(8, 1, P);
            ENGINE_BLK(802D17DC);
            func_802A02E4(8, P);
            ENGINE_BLK(802D17EC);
            goto anim;
        }
        ENGINE_BLK(802D17F4);
        r = part(9, NULL, NULL);
        ENGINE_BLK(802D1804);
        if (r != 0) {
            ENGINE_BLK(802D180C);
            func_8029F9D4(9, 1, P);
            ENGINE_BLK(802D1820);
            func_802A02E4(9, P);
            ENGINE_BLK(802D1830);
            goto anim;
        }
        ENGINE_BLK(802D1838);
        func_802A0360(7, 0, P, 0.0f);
        ENGINE_BLK(802D1854);
        func_8029F9D4(7, 1, P);
    anim:
        ENGINE_BLK(802D1868);
        func_802A039C(0x1F, 0x28, P);
        ENGINE_BLK(802D187C);
        func_802A03D4(0x1F, 0, P);
        ENGINE_BLK(802D1890);
        func_802A040C(0x1F, 0, P);
        ENGINE_BLK(802D18A4);
        func_802A0290(0x1F, 1, P);
    }
    ENGINE_BLK(802D18B8);
    vs->unkA3 = 1;
    r = part(0x1F, NULL, NULL);
    ENGINE_BLK(802D18D0);
    if (r == 1)
        return;
    ENGINE_BLK(802D18DC);
    D_803F7804 = 0;
    if (D_80370C35 == 0) {
        ENGINE_BLK(802D18F4);
        if (D_80370C1C != 0)
            goto roll;
        ENGINE_BLK(802D1904);
        if (D_80370C1D != 0)
            goto roll;
    }
    ENGINE_BLK(802D1914);
    if (D_80370C1A != 0)
        goto roll;
    ENGINE_BLK(802D1924);
    if (D_80370C1B == 0)
        goto legs;
roll:
    ENGINE_BLK(802D1934);
    if (vs->unk76 < 0x96)
        goto legs;
    /* rolling up */
    ENGINE_BLK(802D1944);
    vs->unkA1 = 1;
    func_802A0360(2, 0, P, 0.0f);
    ENGINE_BLK(802D1968);
    k = vs->unkA2;
    if (k == 0) {
        ENGINE_BLK(802D1988);
        func_8029F9D4(1, 2, P);
        ENGINE_BLK(802D199C);
        func_802A02E4(1, P);
        ENGINE_BLK(802D19AC);
    } else {
        ENGINE_BLK(802D1974);
        if (k == 1) {
            ENGINE_BLK(802D19B4);
            func_8029F9D4(5, 2, P);
            ENGINE_BLK(802D19C8);
            func_802A02E4(5, P);
            ENGINE_BLK(802D19D8);
        } else {
            ENGINE_BLK(802D197C);
            if (k != 2) {
                ENGINE_BLK(802D1984);
                engine_trap(0x802D1984);
            }
            ENGINE_BLK(802D19E0);
            func_8029F9D4(6, 2, P);
            ENGINE_BLK(802D19F4);
            func_802A02E4(6, P);
        }
    }
    ENGINE_BLK(802D1A04);
    func_802A039C(0x1F, 0x50, P);
    ENGINE_BLK(802D1A18);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802D1A2C);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802D1A40);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802D1A54);
    func_80278EB0(6, 0.1f, 100);
    ENGINE_BLK(802D1A70);
    return;

legs:
    ENGINE_BLK(802D1A78);
    k = vs->unkA2;
    if (k == 0)
        goto leg1;
    ENGINE_BLK(802D1A84);
    if (k == 1)
        goto leg5;
    ENGINE_BLK(802D1A8C);
    if (k == 2)
        goto leg6;
    ENGINE_BLK(802D1A94);
    engine_trap(0x802D1A94);

leg1:
    /* the left leg (part 1) in front */
    ENGINE_BLK(802D1A98);
    r = part(1, &a0, NULL);
    ENGINE_BLK(802D1AA8);
    if (r == 0)
        goto next;
    ENGINE_BLK(802D1AB0);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802D1ABC);
        func_802A03D4(1, 1, P);
        ENGINE_BLK(802D1AD0);
    } else {
        ENGINE_BLK(802D1AD8);
        func_802A03D4(1, 0, P);
    }
    ENGINE_BLK(802D1AEC);
    if (s < 0) {
        ENGINE_BLK(802D1AF4);
        s = -s;
    }
    ENGINE_BLK(802D1AF8);
    if (s != 0) {
        ENGINE_BLK(802D1B00);
        s = (u32)s / 0x18;
    }
    ENGINE_BLK(802D1B1C);
    func_802A039C(1, s, P);
    ENGINE_BLK(802D1B30);
    return;

leg5:
    /* the right leg (part 5) */
    ENGINE_BLK(802D1B38);
    r = part(5, &a0, NULL);
    ENGINE_BLK(802D1B48);
    if (r == 0)
        goto next;
    ENGINE_BLK(802D1B50);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802D1B5C);
        func_802A03D4(5, 1, P);
        ENGINE_BLK(802D1B70);
    } else {
        ENGINE_BLK(802D1B78);
        func_802A03D4(5, 0, P);
    }
    ENGINE_BLK(802D1B8C);
    if (s < 0) {
        ENGINE_BLK(802D1B94);
        s = -s;
    }
    ENGINE_BLK(802D1B98);
    if (s != 0) {
        ENGINE_BLK(802D1BA0);
        s = (u32)s / 0x18;
    }
    ENGINE_BLK(802D1BBC);
    func_802A039C(5, s, P);
    ENGINE_BLK(802D1BD0);
    return;

leg6:
    /* the third (part 6) */
    ENGINE_BLK(802D1BD8);
    r = part(6, &a0, NULL);
    ENGINE_BLK(802D1BE8);
    if (r == 0)
        goto next;
    ENGINE_BLK(802D1BF0);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802D1BFC);
        func_802A03D4(6, 1, P);
        ENGINE_BLK(802D1C10);
    } else {
        ENGINE_BLK(802D1C18);
        func_802A03D4(6, 0, P);
    }
    ENGINE_BLK(802D1C2C);
    if (s < 0) {
        ENGINE_BLK(802D1C34);
        s = -s;
    }
    ENGINE_BLK(802D1C38);
    if (s != 0) {
        ENGINE_BLK(802D1C40);
        s = (u32)s / 0x18;
    }
    ENGINE_BLK(802D1C5C);
    func_802A039C(6, s, P);
    ENGINE_BLK(802D1C70);
    return;

next:
    /* its step done: the next by the speed (1 below 0x3C, 5 below 0x6E,
       else 6) */
    ENGINE_BLK(802D1C78);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802D1C84);
        s = -s;
    }
    ENGINE_BLK(802D1C88);
    if (s < 0x3C) {
        ENGINE_BLK(802D1D5C);
        if (a0 != 0) {
            ENGINE_BLK(802D1D64);
            func_802A0290(1, 2, P);
            ENGINE_BLK(802D1D78);
        } else {
            ENGINE_BLK(802D1D80);
            func_802A0290(1, 1, P);
        }
        ENGINE_BLK(802D1D94);
        func_802A0360(1, 0, P, 0.0f);
        ENGINE_BLK(802D1DB0);
        vs->unkA2 = 0;
        goto leg1;
    }
    ENGINE_BLK(802D1C94);
    if (s < 0x6E) {
        ENGINE_BLK(802D1CFC);
        if (a0 != 0) {
            ENGINE_BLK(802D1D04);
            func_802A0290(5, 2, P);
            ENGINE_BLK(802D1D18);
        } else {
            ENGINE_BLK(802D1D20);
            func_802A0290(5, 1, P);
        }
        ENGINE_BLK(802D1D34);
        func_802A0360(5, 0, P, 0.0f);
        ENGINE_BLK(802D1D50);
        vs->unkA2 = 1;
        goto leg5;
    }
    ENGINE_BLK(802D1C9C);
    if (a0 != 0) {
        ENGINE_BLK(802D1CA4);
        func_802A0290(6, 2, P);
        ENGINE_BLK(802D1CB8);
    } else {
        ENGINE_BLK(802D1CC0);
        func_802A0290(6, 1, P);
    }
    ENGINE_BLK(802D1CD4);
    func_802A0360(6, 0, P, 0.0f);
    ENGINE_BLK(802D1CF0);
    vs->unkA2 = 2;
    goto leg6;
}

/* the parts and the states (unkA1: 0 walking, 1 rolling, 2 rolling on, 3
   and 4 the landing after a hit) */
REGS(gp)
void func_802D1360(VS *vs) {
    s32 v, a1, t1, r;

    ENGINE_BLK(802D1360);
    if (vs->unkA1 != 0) {
        ENGINE_BLK(802D1374);
        if (vs->unkA1 == 1)
            goto rolling;
        ENGINE_BLK(802D1380);
        if (vs->unkA1 == 2)
            goto rolling2;
        ENGINE_BLK(802D1388);
        if (vs->unkA1 == 3)
            goto landing;
        ENGINE_BLK(802D1390);
        if (vs->unkA1 == 4)
            goto landing2;
        ENGINE_BLK(802D1398);
        engine_trap(0x802D1398);
    }
    /* walking: the step sounds (the legs' frames 2 and 6) */
    ENGINE_BLK(802D139C);
    v = part(1, NULL, &t1);
    ENGINE_BLK(802D13AC);
    if (v == 1)
        goto sound;
    ENGINE_BLK(802D13B8);
    v = part(5, NULL, &t1);
    ENGINE_BLK(802D13C8);
    if (v == 1)
        goto sound;
    ENGINE_BLK(802D13D4);
    v = part(6, NULL, &t1);
    ENGINE_BLK(802D13E4);
    if (v != 1)
        goto moving;
sound:
    ENGINE_BLK(802D13F0);
    a1 = D_803FC996;
    D_803FC996 = t1;
    if (t1 != a1) {
        ENGINE_BLK(802D1408);
        if (t1 == 6) {
            ENGINE_BLK(802D141C);
            func_80260650(D_80367738, 0x4E, NULL);
        } else {
            ENGINE_BLK(802D1410);
            if (t1 == 2) {
                ENGINE_BLK(802D141C);
                func_80260650(D_80367738, 0x4F, NULL);
            }
        }
    }
moving:
    ENGINE_BLK(802D142C);
    if (vs->unk76 == 0)
        stand(vs);
    else
        walk(vs);
    goto done;

rolling:
    /* rolling: the sound; into a building, the landing; stopped (or the
       button let go), standing up */
    ENGINE_BLK(802D1DBC);
    if (D_803F7844 == NULL) {
        ENGINE_BLK(802D1DCC);
        func_80260650(D_80367738, 0x51, &D_803F7844);
    }
    ENGINE_BLK(802D1DE4);
    if (vs->unk9C != 0)
        goto hit1;
    ENGINE_BLK(802D1DF0);
    if (vs->unk9D != 0)
        goto up1;
    ENGINE_BLK(802D1DFC);
    if (D_803FC99A != 0)
        goto up1;
    ENGINE_BLK(802D1E0C);
    vs->unk76 = 0x118;
    r = part(0x1F, NULL, NULL);
    ENGINE_BLK(802D1E24);
    if (r == 1)
        goto done;
    ENGINE_BLK(802D1E30);
    func_802A039C(2, 0xA, P);
    ENGINE_BLK(802D1E44);
    func_802A03D4(2, 0, P);
    ENGINE_BLK(802D1E58);
    func_802A040C(2, 0, P);
    ENGINE_BLK(802D1E6C);
    func_802A0290(2, 1, P);
    ENGINE_BLK(802D1E80);
    vs->unkA1 = 2;
    goto done;
hit1:
    ENGINE_BLK(802D1E8C);
    vs->unk76 = vs->unk76 >> 1;
    func_802794A4();
    ENGINE_BLK(802D1E9C);
    vs->unkA1 = 3;
    func_802A0360(3, 0, P, 0.0f);
    ENGINE_BLK(802D1EC0);
    func_8029F9D4(0x1F, 3, P);
    ENGINE_BLK(802D1ED4);
    func_802A039C(0x1F, 0x21, P);
    ENGINE_BLK(802D1EE8);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802D1EFC);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802D1F10);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802D1F24);
    D_803F7804 = 1;
    goto done;
up1:
    ENGINE_BLK(802D1F34);
    if (vs->unk76 >= 0) {
        ENGINE_BLK(802D1F40);
        vs->unk76 = 0x3C;
    }
    ENGINE_BLK(802D1F48);
    func_802C444C();
    ENGINE_BLK(802D1F50);
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802D1F6C);
    func_8029F9D4(0x1F, 1, P);
    ENGINE_BLK(802D1F80);
    func_802A039C(0x1F, 0x32, P);
    ENGINE_BLK(802D1F94);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802D1FA8);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802D1FBC);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802D1FD0);
    func_802794A4();
    ENGINE_BLK(802D1FD8);
    vs->unkA1 = 0;
    goto done;

rolling2:
    ENGINE_BLK(802D1FE4);
    if (vs->unk9C != 0)
        goto hit2;
    ENGINE_BLK(802D1FF0);
    if (vs->unk9D != 0)
        goto up2;
    ENGINE_BLK(802D1FFC);
    if (D_803FC99A != 0)
        goto up2;
    ENGINE_BLK(802D200C);
    vs->unk76 = 0x118;
    r = part(2, NULL, NULL);
    ENGINE_BLK(802D2024);
    if (r == 1)
        goto done;
up2:
    ENGINE_BLK(802D2030);
    func_802C444C();
    ENGINE_BLK(802D2038);
    if (vs->unk76 >= 0) {
        ENGINE_BLK(802D2044);
        vs->unk76 = 0x3C;
    }
    ENGINE_BLK(802D204C);
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802D2068);
    func_802A02E4(2, P);
    ENGINE_BLK(802D2078);
    func_8029F9D4(2, 1, P);
    ENGINE_BLK(802D208C);
    func_802A039C(0x1F, 0x32, P);
    ENGINE_BLK(802D20A0);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802D20B4);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802D20C8);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802D20DC);
    func_802794A4();
    ENGINE_BLK(802D20E4);
    vs->unkA1 = 0;
    goto done;
hit2:
    ENGINE_BLK(802D20F0);
    vs->unk76 = vs->unk76 >> 1;
    vs->unkA1 = 3;
    func_802A0360(3, 0, P, 0.0f);
    ENGINE_BLK(802D2120);
    func_8029F9D4(2, 3, P);
    ENGINE_BLK(802D2134);
    func_802A02E4(2, P);
    ENGINE_BLK(802D2144);
    func_802A039C(0x1F, 0x21, P);
    ENGINE_BLK(802D2158);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802D216C);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802D2180);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802D2194);
    D_803F7804 = 1;
    goto done;

landing:
    /* landing: the crash's sound, then (part 0x1F's frame done) part 3's
       animation, state 4 */
    ENGINE_BLK(802D21A4);
    if (D_803F7844 != NULL) {
        ENGINE_BLK(802D21B4);
        func_802C444C();
        ENGINE_BLK(802D21BC);
        func_80260650(D_80367738, 0x4B, NULL);
    }
    ENGINE_BLK(802D21D0);
    func_802BCC10();
    ENGINE_BLK(802D21D8);
    r = part(0x1F, NULL, NULL);
    ENGINE_BLK(802D21E8);
    if (r == 1)
        goto done;
    ENGINE_BLK(802D21F4);
    D_803F7804 = 0;
    func_802794A4();
    ENGINE_BLK(802D21FC);
    func_802A039C(3, 5, P);
    ENGINE_BLK(802D2210);
    func_802A03D4(3, 0, P);
    ENGINE_BLK(802D2224);
    func_802A040C(3, 0, P);
    ENGINE_BLK(802D2238);
    func_802A0290(3, 1, P);
    ENGINE_BLK(802D224C);
    vs->unkA1 = 4;
    goto done;

landing2:
    ENGINE_BLK(802D2258);
    func_802BCC10();
    ENGINE_BLK(802D2260);
    r = part(3, NULL, NULL);
    ENGINE_BLK(802D2270);
    if (r == 1)
        goto done;
    ENGINE_BLK(802D227C);
    D_803F7804 = 1;
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802D22A4);
    func_802A0360(5, 0, P, 0.0f);
    ENGINE_BLK(802D22BC);
    func_802A0360(6, 0, P, 0.0f);
    ENGINE_BLK(802D22D4);
    vs->unkA1 = 0;
done:
    ENGINE_BLK(802D22E0);
}
