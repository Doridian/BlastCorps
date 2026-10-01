/*
 * hd_code 772A0 (us.v11 0x802BBA60-0x802BC5D4): the train (VEHICLE_TRAIN),
 * as native C (engine.h).  Its parts are D_803EFAF0, its state D_803EFDF0
 * and its position D_803EFE98..A0 (vehicle.h).  func_802BBA60 sets it up
 * (from the level loader), func_802BBEB8 runs it each frame (from hd.c and
 * at the end of the setup); the others are hd.c's hooks for it.
 *
 * The train runs on rails: with D_803EFEC8 set, its x follows its z along
 * the line from (D_803EFEB0, D_803EFEB4) to (D_803EFEB8, D_803EFEBC).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/audio.h"

/* the train's .bss (asm/data/hd_code/772A0.bss.s) */
extern Part D_803EFAF0[32];
extern VS D_803EFDF0;
extern s32 D_803EFE98, D_803EFE9C, D_803EFEA0;  /* x, y, z */
extern u8 *PTR32 D_803EFEA4;                    /* its model file */
extern u8 *PTR32 D_803EFEA8;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EFEAC;
extern s32 D_803EFEC0, D_803EFEC4;              /* frames since it last stopped at each end */
extern u8 D_803EFEC9;                           /* the wheels' sparks: frames to wait */
extern u8 D_803EFECA;

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_80370C1C, D_80370C23;
extern u8 D_803A7425;
extern Part *PTR32 D_803F77D0;
extern u8 D_80305E00[];
extern s32 D_803643E4, D_803643E8;
extern u8 D_802C2984[];                         /* the sparks' effect record (60F60) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);

void func_802BBEB8(void);
void func_802BBE74(void);
void func_802BC2C8(void);
REGS(gp)
void func_802BC3D0(VS *vs);
void func_802BC578(void);

#define T(p) ((s32)(p))

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802BBA60(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EFDF0;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802BBA60);
    engine_save(ENGINE_T0_T5, 0);
    D_803EFEA4 = model;
    buf = D_80358070;
    D_803EFEA8 = buf;
    D_803EFEAC = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(7, 0, D_803EFEA8, D_803EFEAC, model);
    ENGINE_BLK(802BBAE0);
    func_802A754C(vs);
    ENGINE_BLK(802BBAEC);
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
    D_803EFE98 = x;
    D_803EFE9C = y;
    D_803EFEA0 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803EFE9C, x, z, vs->unk4, &D_803EFE9C, &vs->unk4C, 7, vs);
    ENGINE_BLK(802BBB8C);
    func_8029F85C(D_803EFAF0, D_803EFEA4, D_803EFEA8, D_803EFEAC);
    ENGINE_BLK(802BBBC8);
    func_802A039C(0, 100, D_803EFAF0);
    ENGINE_BLK(802BBBDC);
    func_802A03D4(0, 0, D_803EFAF0);
    ENGINE_BLK(802BBBF0);
    func_802A040C(0, 0, D_803EFAF0);
    ENGINE_BLK(802BBC04);
    func_802A0480(0, 0, D_803EFAF0, 0.0f);
    ENGINE_BLK(802BBC1C);
    func_802A0290(0, 1, D_803EFAF0);
    ENGINE_BLK(802BBC30);
    func_8029E558(D_803EFAF0, D_803EFEA8, D_803EFEAC);
    ENGINE_BLK(802BBC44);
    func_802A0320(0, D_803EFAF0);
    ENGINE_BLK(802BBC54);
    func_802A0290(0, 1, D_803EFAF0);
    ENGINE_BLK(802BBC68);
    func_8029E558(D_803EFAF0, D_803EFEAC, D_803EFEA8);
    ENGINE_BLK(802BBC7C);
    /* the gears: (top speed, ?, ?) */
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 1;
    r[3] = 0, r[4] = 0x50, r[5] = 1;
    r[6] = 0x50, r[7] = 0x8C, r[8] = 1;
    r[9] = 0x8C, r[10] = 0xBE, r[11] = 1;
    r[12] = 0xBE, r[13] = 0xFA, r[14] = 1;
    D_803EFEC9 = 0;
    D_803EFECA = 0;
    D_803EFEC0 = 999999;
    D_803EFEC4 = 999999;
    func_8029C354(7, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 15000);
    ENGINE_BLK(802BBD44);
    func_80258230(7, 0x96, 0x2D, 0x2D);
    ENGINE_BLK(802BBD5C);
    vs->unk9A = 1;
    func_802BBEB8();
    ENGINE_BLK(802BBD6C);
    vs->unk9A = 0;
    func_802AA838(D_803EFEAC, D_803EFEA8, *(s32 *)(D_803EFEA4 + *(s32 *)(D_803EFEA4 + 0x18) + 4));
    ENGINE_BLK(802BBDA4);
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(18, T(&D_803EFE9C));
    ENGINE_LEAVE(22, T(D_803EFEA8));
    ENGINE_LEAVE(23, T(D_803EFEAC));
}

/* hd.c's: the player gets in */
void func_802BBDC8(void) {
    ENGINE_BLK(802BBDC8);
    D_8036444C = 3000;
    D_80364450 = 0;
    func_802C4310(0x20);
    ENGINE_BLK(802BBE00);
}

/* hd.c's: whether it can be left (always) */
u8 func_802BBE10(void) {
    ENGINE_BLK(802BBE10);
    return 1;
}

/* hd.c's: the player gets out */
void func_802BBE2C(void) {
    ENGINE_BLK(802BBE2C);
    engine_save(0x10000000, 0);
    func_802A7764((u32 *)D_803EFEA8, (u32 *)D_803EFEAC, 0x800);
    ENGINE_BLK(802BBE58);
    func_802C444C();
    ENGINE_BLK(802BBE60);
    engine_restore();
}

/* its light */
void func_802BBE74(void) {
    ENGINE_BLK(802BBE74);
    func_802ABD54(7, D_803EFE98, D_803EFE9C, D_803EFEA0);
    ENGINE_BLK(802BBEA8);
}

/* each frame */
void func_802BBEB8(void) {
    VS *vs = &D_803EFDF0;
    s32 t3 = D_803EFE98, t2, x, z, z0, x0;
    f32 rate, f;
    s32 near, far;

    ENGINE_BLK(802BBEB8);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802BBE74();
    ENGINE_BLK(802BBF08);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802BBF1C);
        func_802BC2C8();
    }
    ENGINE_BLK(802BBF24);
    func_802BC578();
    ENGINE_BLK(802BBF2C);
    t2 = func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 6, vs, &t3);
    ENGINE_BLK(802BBF44);
    func_802A7FD8(&vs->unk74, 0x2328, &vs->unk76, &vs->unk4C, &vs->unk4E, &vs->unk96[3], 0, vs);
    ENGINE_BLK(802BBF60);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802BBF6C);
    func_802A843C(&vs->unk76, 7, vs->unk96, vs->unk4, 1, 160.0f, vs);
    ENGINE_BLK(802BBF80);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EFE98, &D_803EFEA0, rate, &z);
    ENGINE_BLK(802BBF98);
    if (D_803EFEC8 != 0) {
        /* on the rails: x from z */
        ENGINE_BLK(802BBFA8);
        z0 = D_803EFEB4;
        f = (f32)(D_803EFEA0 - z0) / (f32)(D_803EFEBC - z0);
        x0 = D_803EFEB0;
        x = x0 + engine_cvt_w_s((f32)(D_803EFEB8 - x0) * f);
        D_803EFE98 = x;
    }
    ENGINE_BLK(802BC02C);
    D_803ED40B = 0;
    func_802A8768(x, z, &D_803EFE98, &D_803EFEA0, &D_803EFE9C, 7, 0xA0, 0xA0, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802BC060);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BC088);
        func_8029E558(D_803EFAF0, D_803EFEA8, D_803EFEAC);
        ENGINE_BLK(802BC09C);
    } else {
        ENGINE_BLK(802BC0A4);
        func_8029E558(D_803EFAF0, D_803EFEAC, D_803EFEA8);
    }
    ENGINE_BLK(802BC0B8);
    func_802BC3D0(vs);
    ENGINE_BLK(802BC0C0);
    D_803EFEC0++;
    D_803EFEC4++;
    if (vs->unk9A == 0) {
        ENGINE_BLK(802BC0F4);
        func_8029A800(D_803EFE98, D_803EFE9C, D_803EFEA0, D_80305E00, 0, 0, vs->unk76, 0, 7, vs);
        ENGINE_BLK(802BC138);
        func_8029C52C(7, vs);
        ENGINE_BLK(802BC140);
        func_8029AA10();
        ENGINE_BLK(802BC148);
        D_803F77D0 = D_803EFAF0;
        func_802BE77C(7, vs);
        ENGINE_BLK(802BC164);
        if (D_803A7425 != 0) {
            /* at the ends of the line: stopped, then sent back */
            ENGINE_BLK(802BC174);
            near = func_802BCD80(2);
            ENGINE_BLK(802BC17C);
            far = func_802BCD80(3);
            ENGINE_BLK(802BC188);
            if (far != 0) {
                ENGINE_BLK(802BC190);
                if (near != 0) {
                    ENGINE_BLK(802BC198);
                    vs->unk76 = 0;
                    goto done;
                }
                ENGINE_BLK(802BC1A0);
                D_803EFEC0 = 0;
                if (D_803EFEC4 < 6)
                    goto stop;
                ENGINE_BLK(802BC1BC);
                vs->unk76 = -0x28;
            } else {
                ENGINE_BLK(802BC1C8);
                if (near == 0)
                    goto done;
                ENGINE_BLK(802BC1D0);
                D_803EFEC4 = 0;
                if (D_803EFEC0 < 6)
                    goto stop;
                ENGINE_BLK(802BC1EC);
                vs->unk76 = 0x28;
            }
            goto done;
        stop:
            ENGINE_BLK(802BC1F8);
            vs->unk76 = 0;
        }
    }
done:
    ENGINE_BLK(802BC1FC);
    D_803643E0 = D_803EFE98;
    D_803643E4 = D_803EFE9C;
    D_803643E8 = D_803EFEA0;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 7, vs);
    ENGINE_BLK(802BC278);
    engine_restore();
}

/* the wheels' sparks and sound */
void func_802BC2C8(void) {
    VS *vs = &D_803EFDF0;
    s32 s;

    ENGINE_BLK(802BC2C8);
    if (D_803EFEC9 != 0) {
        ENGINE_BLK(802BC2E8);
        D_803EFEC9--;
        goto sound;
    }
    ENGINE_BLK(802BC2F4);
    if (vs->unk76 > 0) {
        ENGINE_BLK(802BC318);
        if (D_80370C23 == 0)
            goto sound;
    } else {
        ENGINE_BLK(802BC300);
        if (D_80370C1C == 0)
            goto sound;
        ENGINE_BLK(802BC310);
    }
    ENGINE_BLK(802BC328);
    func_802A6274(T(D_802C2984), 0x9C40, 1, 7, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ENGINE_BLK(802BC350);
    func_802A6274(T(D_802C2984), 0x9C40, 1, 7, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ENGINE_BLK(802BC378);
    func_80260650(D_80367738, 0x29, NULL);
    ENGINE_BLK(802BC38C);
    D_803EFEC9 = 1;
sound:
    ENGINE_BLK(802BC398);
    s = vs->unk76 >> 4;
    if (s < 0) {
        ENGINE_BLK(802BC3A8);
        s = -s;
    }
    ENGINE_BLK(802BC3AC);
    func_802C4584(s);
    ENGINE_BLK(802BC3B4);
    func_802C4724(0x21);
    ENGINE_BLK(802BC3BC);
}

/* the train's matrix, its vertices, its collision and its shadow */
REGS(gp)
void func_802BC3D0(VS *vs) {
    u8 *model = D_803EFEA4, *buf;
    s32 *m, off;

    ENGINE_BLK(802BC3D0);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BC400);
        m = (s32 *)(D_803EFEA8 + off);
    } else {
        ENGINE_BLK(802BC414);
        m = (s32 *)(D_803EFEAC + off);
    }
    ENGINE_BLK(802BC424);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    m = func_802AA764(D_803EFE98, D_803EFE9C, D_803EFEA0, 15000, m);
    ENGINE_BLK(802BC468);
    if (D_8035805C != 0) {
        ENGINE_BLK(802BC47C);
        buf = D_803EFEA8;
    } else {
        ENGINE_BLK(802BC48C);
        buf = D_803EFEAC;
    }
    ENGINE_BLK(802BC498);
    model = D_803EFEA4;
    func_8029C454(D_803EFE98, D_803EFE9C, D_803EFEA0, 7, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802BC4E0);
    func_802ABBEC(7, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802BC500);
    func_802AABE4(7, model + *(s32 *)(model + 8), buf);
    ENGINE_BLK(802BC51C);
    func_8029D040(D_803EFE98, D_803EFEA0, 7, model + *(s32 *)(model + 0xC), vs->unk4C, D_803EFAF0, buf);
    ENGINE_BLK(802BC55C);
    D_803EFECB = func_8029DC14(7);
    ENGINE_BLK(802BC564);
}

/* the camera's distance and speed for the train */
void func_802BC578(void) {
    ENGINE_BLK(802BC578);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 4;
}
