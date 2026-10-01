/*
 * hd_code 83910 (us.v11 0x802C80D0-0x802C9BB0): the barges (VEHICLE_BARGE,
 * BARGE_2 and BARGE_3: types 0xB, 0x11 and 0x12), as native C (engine.h).
 * Three copies of the same code over three sets of variables: parts
 * D_803F7C50[3][32], states D_803F8550[3], positions D_803F8748 + 12k,
 * model files D_803F876C + 4k, buffer pairs D_803F8778 + 8k (vehicle.h).
 * func_802C80D0 sets one up (from the level loader), func_802C8BB8 runs one
 * each frame (from hd.c and at the end of its setup); the others are hd.c's
 * hooks.  A barge has no wheels on the ground: no gears from the stick, and
 * a bump turns it back.
 *
 * (port/engine/83910.c is generated from one template per copy by
 * scratchpad's gen83910.py; edited by hand since.)
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/audio.h"

/* the barges' .bss (asm/data/hd_code/83910.bss.s) */
extern s32 D_803F8748[9];                       /* x, y, z of each */
extern u8 *PTR32 D_803F876C[3];                 /* their model files */
extern u8 *PTR32 D_803F8778[6];                 /* their buffer pairs (0x800 bytes each) */
extern u8 D_803F8790[3];                        /* bumped: turning back */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424;
extern Part *PTR32 D_803F77D0;
extern u8 D_80306410[];
extern s32 D_803643E4, D_803643E8;

void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_802C8BB8(u8 type);
REGS(gp)
void func_802C95D8(VS *vs);
void func_802C9B30(void);

#define T(p) ((s32)(p))

/* ---- barge 0 (type 0xB) ---- */

REGS(gp)
void func_802C9624(VS *vs);

/* set up: the model file in $s2, the position in $t7, $s3, $s0, the
   heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802C8150(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F8550[0];
    Part *parts = D_803F7C50 + 32 * 0;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802C8150);
    D_803F876C[0] = model;
    buf = D_80358070;
    D_803F8778[0] = buf;
    D_803F8778[1] = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(0xB, 0, D_803F8778[0], D_803F8778[1], model);
    ENGINE_BLK(802C81B8);
    D_803F8790[0] = 0;
    func_802A754C(vs);
    ENGINE_BLK(802C81CC);
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
    D_803F8748[0] = x;
    D_803F8748[1] = y;
    D_803F8748[2] = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803F8748[1], x, z, vs->unk4, &D_803F8748[1], &vs->unk4C, 0xB, vs);
    ENGINE_BLK(802C826C);
    func_8029F85C(parts, D_803F876C[0], D_803F8778[0], D_803F8778[1]);
    ENGINE_BLK(802C82A8);
    func_802A039C(0, 100, parts);
    ENGINE_BLK(802C82BC);
    func_802A03D4(0, 0, parts);
    ENGINE_BLK(802C82D0);
    func_802A040C(0, 0, parts);
    ENGINE_BLK(802C82E4);
    func_802A0480(0, 0, parts, 0.0f);
    ENGINE_BLK(802C82FC);
    func_802A0290(0, 1, parts);
    ENGINE_BLK(802C8310);
    func_8029E558(parts, D_803F8778[0], D_803F8778[1]);
    ENGINE_BLK(802C8324);
    func_802A0320(0, parts);
    ENGINE_BLK(802C8334);
    func_802A0290(0, 1, parts);
    ENGINE_BLK(802C8348);
    func_8029E558(parts, D_803F8778[1], D_803F8778[0]);
    ENGINE_BLK(802C835C);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 1;
    r[3] = 0, r[4] = 0x50, r[5] = 1;
    r[6] = 0x50, r[7] = 0x8C, r[8] = 1;
    r[9] = 0x8C, r[10] = 0xBE, r[11] = 1;
    r[12] = 0xBE, r[13] = 0xFA, r[14] = 1;
    func_8029C354(0xB, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x88B8);
    ENGINE_BLK(802C83FC);
    func_80258230(0xB, 0x96, 0x2D, 0x2D);
    ENGINE_BLK(802C8414);
    vs->unk9A = 1;
    func_802C8BB8(0xB);
    ENGINE_BLK(802C8428);
    vs->unk9A = 0;
    model = D_803F876C[0];
    func_802AA838(D_803F8778[1], D_803F8778[0], *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802C8460);
}

/* each frame */
REGS(a0)
void func_802C8C90(u8 type) {
    VS *vs = &D_803F8550[0];
    Part *parts = D_803F7C50 + 32 * 0;
    s32 t3 = 0, x, z, s;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802C8C90);
    engine_save(ENGINE_GPR(4), 0);
    func_802A75DC((u8 *)parts, &D_803F8748[0], &D_803F8748[1], &D_803F8748[2], (u8 *)vs);
    ENGINE_BLK(802C8CC8);
    func_802C4724(0x71);
    ENGINE_BLK(802C8CD0);
    func_802C9B30();
    ENGINE_BLK(802C8CD8);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802C8CE4);
        s = -s;
    }
    ENGINE_BLK(802C8CE8);
    func_802C4584((u32)s >> 5);
    ENGINE_BLK(802C8CF0);
    func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 6, vs, &t3);
    ENGINE_BLK(802C8D08);
    func_802A7FD8(&vs->unk74, 0x2328, &vs->unk76, &vs->unk4C, &vs->unk4E, &vs->unk96[3], 0, vs);
    ENGINE_BLK(802C8D24);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802C8D30);
    func_802A843C(&vs->unk76, 0xB, vs->unk96, vs->unk4, 1, 160.0f, vs);
    ENGINE_BLK(802C8D44);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803F8748[0], &D_803F8748[2], rate, &z);
    ENGINE_BLK(802C8D5C);
    D_803ED40B = 0;
    func_802A8768(x, z, &D_803F8748[0], &D_803F8748[2], &D_803F8748[1], 0xB, 0xA0, 0xA0, vs->unk52, vs->unk28, vs->unk28 + 6, vs->unk28 + 3,
                  vs->unk5E, vs);
    ENGINE_BLK(802C8D90);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C8DB8);
        func_8029E558(parts, D_803F8778[0], D_803F8778[1]);
        ENGINE_BLK(802C8DCC);
    } else {
        ENGINE_BLK(802C8DD4);
        func_8029E558(parts, D_803F8778[1], D_803F8778[0]);
    }
    ENGINE_BLK(802C8DE8);
    func_802C9624(vs);
    ENGINE_BLK(802C8DF0);
    func_8029A800(D_803F8748[0], D_803F8748[1], D_803F8748[2], D_80306410, 0, 0, vs->unk76, 0, 0xB, vs);
    ENGINE_BLK(802C8E34);
    func_8029C52C(0xB, vs);
    ENGINE_BLK(802C8E3C);
    func_8029AA10();
    ENGINE_BLK(802C8E44);
    D_803F77D0 = parts;
    func_802BE77C(0xB, vs);
    ENGINE_BLK(802C8E60);
    if (D_803A7424 == 0) {
        ENGINE_BLK(802C8F10);
        D_803F8790[0] = 0;
        goto done;
    }
    ENGINE_BLK(802C8E70);
    if (D_803F8790[0] != 0)
        goto done;
    /* bumped: back to where it was, turned round */
    ENGINE_BLK(802C8E80);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C8E94);
        a2 = D_803F8778[1];
        a3 = D_803F8778[0];
    } else {
        ENGINE_BLK(802C8EB0);
        a2 = D_803F8778[0];
        a3 = D_803F8778[1];
    }
    ENGINE_BLK(802C8EC8);
    func_802A768C((u8 *)parts, &D_803F8748[0], &D_803F8748[1], &D_803F8748[2], (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    ENGINE_BLK(802C8EF0);
    func_802C95D8(vs);
    ENGINE_BLK(802C8EF8);
    D_803F8790[0] = 1;
    func_802C9624(vs);
    ENGINE_BLK(802C8F08);
    goto out;
done:
    ENGINE_BLK(802C8F18);
    D_803643E0 = D_803F8748[0];
    D_803643E4 = D_803F8748[1];
    D_803643E8 = D_803F8748[2];
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0xB, vs);
out:
    ENGINE_BLK(802C8F94);
    engine_restore();
}

/* its matrix, its vertices, its collision and its shadow */
REGS(gp)
void func_802C9624(VS *vs) {
    u8 *model = D_803F876C[0], *buf;
    s32 *m, off;

    ENGINE_BLK(802C9624);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C9654);
        m = (s32 *)(D_803F8778[0] + off);
    } else {
        ENGINE_BLK(802C9668);
        m = (s32 *)(D_803F8778[1] + off);
    }
    ENGINE_BLK(802C9678);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803F8748[0], D_803F8748[1], D_803F8748[2], 0x88B8, m);
    ENGINE_BLK(802C96BC);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C96D0);
        buf = D_803F8778[0];
    } else {
        ENGINE_BLK(802C96E0);
        buf = D_803F8778[1];
    }
    ENGINE_BLK(802C96EC);
    model = D_803F876C[0];
    func_8029C454(D_803F8748[0], D_803F8748[1], D_803F8748[2], 0xB, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), buf);
    ENGINE_BLK(802C9734);
    func_802ABBEC(0xB, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802C9754);
    func_802AABE4(0xB, model + *(s32 *)(model + 8), buf);
    ENGINE_BLK(802C9770);
    func_8029D040(D_803F8748[0], D_803F8748[2], 0xB, model + *(s32 *)(model + 0xC), vs->unk4C, D_803F7C50 + 32 * 0, buf);
    ENGINE_BLK(802C97B0);
}

/* ---- barge 1 (type 0x11) ---- */

REGS(gp)
void func_802C97C0(VS *vs);

/* set up: the model file in $s2, the position in $t7, $s3, $s0, the
   heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802C8470(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F8550[1];
    Part *parts = D_803F7C50 + 32 * 1;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802C8470);
    D_803F876C[1] = model;
    buf = D_80358070;
    D_803F8778[2] = buf;
    D_803F8778[3] = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(0x11, 0, D_803F8778[2], D_803F8778[3], model);
    ENGINE_BLK(802C84D8);
    D_803F8790[1] = 0;
    func_802A754C(vs);
    ENGINE_BLK(802C84EC);
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
    D_803F8748[3] = x;
    D_803F8748[4] = y;
    D_803F8748[5] = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803F8748[4], x, z, vs->unk4, &D_803F8748[4], &vs->unk4C, 0x11, vs);
    ENGINE_BLK(802C858C);
    func_8029F85C(parts, D_803F876C[1], D_803F8778[2], D_803F8778[3]);
    ENGINE_BLK(802C85C8);
    func_802A039C(0, 100, parts);
    ENGINE_BLK(802C85DC);
    func_802A03D4(0, 0, parts);
    ENGINE_BLK(802C85F0);
    func_802A040C(0, 0, parts);
    ENGINE_BLK(802C8604);
    func_802A0480(0, 0, parts, 0.0f);
    ENGINE_BLK(802C861C);
    func_802A0290(0, 1, parts);
    ENGINE_BLK(802C8630);
    func_8029E558(parts, D_803F8778[2], D_803F8778[3]);
    ENGINE_BLK(802C8644);
    func_802A0320(0, parts);
    ENGINE_BLK(802C8654);
    func_802A0290(0, 1, parts);
    ENGINE_BLK(802C8668);
    func_8029E558(parts, D_803F8778[3], D_803F8778[2]);
    ENGINE_BLK(802C867C);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 1;
    r[3] = 0, r[4] = 0x50, r[5] = 1;
    r[6] = 0x50, r[7] = 0x8C, r[8] = 1;
    r[9] = 0x8C, r[10] = 0xBE, r[11] = 1;
    r[12] = 0xBE, r[13] = 0xFA, r[14] = 1;
    func_8029C354(0x11, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x88B8);
    ENGINE_BLK(802C871C);
    func_80258230(0x11, 0x96, 0x2D, 0x2D);
    ENGINE_BLK(802C8734);
    vs->unk9A = 1;
    func_802C8BB8(0x11);
    ENGINE_BLK(802C8748);
    vs->unk9A = 0;
    model = D_803F876C[1];
    func_802AA838(D_803F8778[3], D_803F8778[2], *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802C8780);
}

/* each frame */
REGS(a0)
void func_802C8FA8(u8 type) {
    VS *vs = &D_803F8550[1];
    Part *parts = D_803F7C50 + 32 * 1;
    s32 t3 = 0, x, z, s;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802C8FA8);
    engine_save(ENGINE_GPR(4), 0);
    func_802A75DC((u8 *)parts, &D_803F8748[3], &D_803F8748[4], &D_803F8748[5], (u8 *)vs);
    ENGINE_BLK(802C8FE0);
    func_802C4724(0x71);
    ENGINE_BLK(802C8FE8);
    func_802C9B30();
    ENGINE_BLK(802C8FF0);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802C8FFC);
        s = -s;
    }
    ENGINE_BLK(802C9000);
    func_802C4584((u32)s >> 5);
    ENGINE_BLK(802C9008);
    func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 6, vs, &t3);
    ENGINE_BLK(802C9020);
    func_802A7FD8(&vs->unk74, 0x2328, &vs->unk76, &vs->unk4C, &vs->unk4E, &vs->unk96[3], 0, vs);
    ENGINE_BLK(802C903C);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802C9048);
    func_802A843C(&vs->unk76, 0x11, vs->unk96, vs->unk4, 1, 160.0f, vs);
    ENGINE_BLK(802C905C);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803F8748[3], &D_803F8748[5], rate, &z);
    ENGINE_BLK(802C9074);
    D_803ED40B = 0;
    func_802A8768(x, z, &D_803F8748[3], &D_803F8748[5], &D_803F8748[4], 0x11, 0xA0, 0xA0, vs->unk52, vs->unk28, vs->unk28 + 6, vs->unk28 + 3,
                  vs->unk5E, vs);
    ENGINE_BLK(802C90A8);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C90D0);
        func_8029E558(parts, D_803F8778[2], D_803F8778[3]);
        ENGINE_BLK(802C90E4);
    } else {
        ENGINE_BLK(802C90EC);
        func_8029E558(parts, D_803F8778[3], D_803F8778[2]);
    }
    ENGINE_BLK(802C9100);
    func_802C97C0(vs);
    ENGINE_BLK(802C9108);
    func_8029A800(D_803F8748[3], D_803F8748[4], D_803F8748[5], D_80306410, 0, 0, vs->unk76, 0, 0x11, vs);
    ENGINE_BLK(802C914C);
    func_8029C52C(0x11, vs);
    ENGINE_BLK(802C9154);
    func_8029AA10();
    ENGINE_BLK(802C915C);
    D_803F77D0 = parts;
    func_802BE77C(0x11, vs);
    ENGINE_BLK(802C9178);
    if (D_803A7424 == 0) {
        ENGINE_BLK(802C9228);
        D_803F8790[1] = 0;
        goto done;
    }
    ENGINE_BLK(802C9188);
    if (D_803F8790[1] != 0)
        goto done;
    /* bumped: back to where it was, turned round */
    ENGINE_BLK(802C9198);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C91AC);
        a2 = D_803F8778[3];
        a3 = D_803F8778[2];
    } else {
        ENGINE_BLK(802C91C8);
        a2 = D_803F8778[2];
        a3 = D_803F8778[3];
    }
    ENGINE_BLK(802C91E0);
    func_802A768C((u8 *)parts, &D_803F8748[3], &D_803F8748[4], &D_803F8748[5], (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    ENGINE_BLK(802C9208);
    func_802C95D8(vs);
    ENGINE_BLK(802C9210);
    D_803F8790[1] = 1;
    func_802C97C0(vs);
    ENGINE_BLK(802C9220);
    goto out;
done:
    ENGINE_BLK(802C9230);
    D_803643E0 = D_803F8748[3];
    D_803643E4 = D_803F8748[4];
    D_803643E8 = D_803F8748[5];
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0x11, vs);
out:
    ENGINE_BLK(802C92AC);
    engine_restore();
}

/* its matrix, its vertices, its collision and its shadow */
REGS(gp)
void func_802C97C0(VS *vs) {
    u8 *model = D_803F876C[1], *buf;
    s32 *m, off;

    ENGINE_BLK(802C97C0);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C97F0);
        m = (s32 *)(D_803F8778[2] + off);
    } else {
        ENGINE_BLK(802C9804);
        m = (s32 *)(D_803F8778[3] + off);
    }
    ENGINE_BLK(802C9814);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803F8748[3], D_803F8748[4], D_803F8748[5], 0x88B8, m);
    ENGINE_BLK(802C9858);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C986C);
        buf = D_803F8778[2];
    } else {
        ENGINE_BLK(802C987C);
        buf = D_803F8778[3];
    }
    ENGINE_BLK(802C9888);
    model = D_803F876C[1];
    func_8029C454(D_803F8748[3], D_803F8748[4], D_803F8748[5], 0x11, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), buf);
    ENGINE_BLK(802C98D0);
    func_802ABBEC(0x11, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802C98F0);
    func_802AABE4(0x11, model + *(s32 *)(model + 8), buf);
    ENGINE_BLK(802C990C);
    func_8029D040(D_803F8748[3], D_803F8748[5], 0x11, model + *(s32 *)(model + 0xC), vs->unk4C, D_803F7C50 + 32 * 1, buf);
    ENGINE_BLK(802C994C);
}

/* ---- barge 2 (type 0x12) ---- */

REGS(gp)
void func_802C995C(VS *vs);

/* set up: the model file in $s2, the position in $t7, $s3, $s0, the
   heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802C8790(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F8550[2];
    Part *parts = D_803F7C50 + 32 * 2;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802C8790);
    D_803F876C[2] = model;
    buf = D_80358070;
    D_803F8778[4] = buf;
    D_803F8778[5] = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(0x12, 0, D_803F8778[4], D_803F8778[5], model);
    ENGINE_BLK(802C87F8);
    D_803F8790[2] = 0;
    func_802A754C(vs);
    ENGINE_BLK(802C880C);
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
    D_803F8748[6] = x;
    D_803F8748[7] = y;
    D_803F8748[8] = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803F8748[7], x, z, vs->unk4, &D_803F8748[7], &vs->unk4C, 0x12, vs);
    ENGINE_BLK(802C88AC);
    func_8029F85C(parts, D_803F876C[2], D_803F8778[4], D_803F8778[5]);
    ENGINE_BLK(802C88E8);
    func_802A039C(0, 100, parts);
    ENGINE_BLK(802C88FC);
    func_802A03D4(0, 0, parts);
    ENGINE_BLK(802C8910);
    func_802A040C(0, 0, parts);
    ENGINE_BLK(802C8924);
    func_802A0480(0, 0, parts, 0.0f);
    ENGINE_BLK(802C893C);
    func_802A0290(0, 1, parts);
    ENGINE_BLK(802C8950);
    func_8029E558(parts, D_803F8778[4], D_803F8778[5]);
    ENGINE_BLK(802C8964);
    func_802A0320(0, parts);
    ENGINE_BLK(802C8974);
    func_802A0290(0, 1, parts);
    ENGINE_BLK(802C8988);
    func_8029E558(parts, D_803F8778[5], D_803F8778[4]);
    ENGINE_BLK(802C899C);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 1;
    r[3] = 0, r[4] = 0x50, r[5] = 1;
    r[6] = 0x50, r[7] = 0x8C, r[8] = 1;
    r[9] = 0x8C, r[10] = 0xBE, r[11] = 1;
    r[12] = 0xBE, r[13] = 0xFA, r[14] = 1;
    func_8029C354(0x12, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x88B8);
    ENGINE_BLK(802C8A3C);
    func_80258230(0x12, 0x96, 0x2D, 0x2D);
    ENGINE_BLK(802C8A54);
    vs->unk9A = 1;
    func_802C8BB8(0x12);
    ENGINE_BLK(802C8A68);
    vs->unk9A = 0;
    model = D_803F876C[2];
    func_802AA838(D_803F8778[5], D_803F8778[4], *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802C8AA0);
}

/* each frame */
REGS(a0)
void func_802C92C0(u8 type) {
    VS *vs = &D_803F8550[2];
    Part *parts = D_803F7C50 + 32 * 2;
    s32 t3 = 0, x, z, s;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802C92C0);
    engine_save(ENGINE_GPR(4), 0);
    func_802A75DC((u8 *)parts, &D_803F8748[6], &D_803F8748[7], &D_803F8748[8], (u8 *)vs);
    ENGINE_BLK(802C92F8);
    func_802C4724(0x71);
    ENGINE_BLK(802C9300);
    func_802C9B30();
    ENGINE_BLK(802C9308);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802C9314);
        s = -s;
    }
    ENGINE_BLK(802C9318);
    func_802C4584((u32)s >> 5);
    ENGINE_BLK(802C9320);
    func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 6, vs, &t3);
    ENGINE_BLK(802C9338);
    func_802A7FD8(&vs->unk74, 0x2328, &vs->unk76, &vs->unk4C, &vs->unk4E, &vs->unk96[3], 0, vs);
    ENGINE_BLK(802C9354);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802C9360);
    func_802A843C(&vs->unk76, 0x12, vs->unk96, vs->unk4, 1, 160.0f, vs);
    ENGINE_BLK(802C9374);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803F8748[6], &D_803F8748[8], rate, &z);
    ENGINE_BLK(802C938C);
    D_803ED40B = 0;
    func_802A8768(x, z, &D_803F8748[6], &D_803F8748[8], &D_803F8748[7], 0x12, 0xA0, 0xA0, vs->unk52, vs->unk28, vs->unk28 + 6, vs->unk28 + 3,
                  vs->unk5E, vs);
    ENGINE_BLK(802C93C0);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C93E8);
        func_8029E558(parts, D_803F8778[4], D_803F8778[5]);
        ENGINE_BLK(802C93FC);
    } else {
        ENGINE_BLK(802C9404);
        func_8029E558(parts, D_803F8778[5], D_803F8778[4]);
    }
    ENGINE_BLK(802C9418);
    func_802C995C(vs);
    ENGINE_BLK(802C9420);
    func_8029A800(D_803F8748[6], D_803F8748[7], D_803F8748[8], D_80306410, 0, 0, vs->unk76, 0, 0x12, vs);
    ENGINE_BLK(802C9464);
    func_8029C52C(0x12, vs);
    ENGINE_BLK(802C946C);
    func_8029AA10();
    ENGINE_BLK(802C9474);
    D_803F77D0 = parts;
    func_802BE77C(0x12, vs);
    ENGINE_BLK(802C9490);
    if (D_803A7424 == 0) {
        ENGINE_BLK(802C9540);
        D_803F8790[2] = 0;
        goto done;
    }
    ENGINE_BLK(802C94A0);
    if (D_803F8790[2] != 0)
        goto done;
    /* bumped: back to where it was, turned round */
    ENGINE_BLK(802C94B0);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C94C4);
        a2 = D_803F8778[5];
        a3 = D_803F8778[4];
    } else {
        ENGINE_BLK(802C94E0);
        a2 = D_803F8778[4];
        a3 = D_803F8778[5];
    }
    ENGINE_BLK(802C94F8);
    func_802A768C((u8 *)parts, &D_803F8748[6], &D_803F8748[7], &D_803F8748[8], (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    ENGINE_BLK(802C9520);
    func_802C95D8(vs);
    ENGINE_BLK(802C9528);
    D_803F8790[2] = 1;
    func_802C995C(vs);
    ENGINE_BLK(802C9538);
    goto out;
done:
    ENGINE_BLK(802C9548);
    D_803643E0 = D_803F8748[6];
    D_803643E4 = D_803F8748[7];
    D_803643E8 = D_803F8748[8];
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0x12, vs);
out:
    ENGINE_BLK(802C95C4);
    engine_restore();
}

/* its matrix, its vertices, its collision and its shadow */
REGS(gp)
void func_802C995C(VS *vs) {
    u8 *model = D_803F876C[2], *buf;
    s32 *m, off;

    ENGINE_BLK(802C995C);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C998C);
        m = (s32 *)(D_803F8778[4] + off);
    } else {
        ENGINE_BLK(802C99A0);
        m = (s32 *)(D_803F8778[5] + off);
    }
    ENGINE_BLK(802C99B0);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803F8748[6], D_803F8748[7], D_803F8748[8], 0x88B8, m);
    ENGINE_BLK(802C99F4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C9A08);
        buf = D_803F8778[4];
    } else {
        ENGINE_BLK(802C9A18);
        buf = D_803F8778[5];
    }
    ENGINE_BLK(802C9A24);
    model = D_803F876C[2];
    func_8029C454(D_803F8748[6], D_803F8748[7], D_803F8748[8], 0x12, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), buf);
    ENGINE_BLK(802C9A6C);
    func_802ABBEC(0x12, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802C9A8C);
    func_802AABE4(0x12, model + *(s32 *)(model + 8), buf);
    ENGINE_BLK(802C9AA8);
    func_8029D040(D_803F8748[6], D_803F8748[8], 0x12, model + *(s32 *)(model + 0xC), vs->unk4C, D_803F7C50 + 32 * 2, buf);
    ENGINE_BLK(802C9AE8);
}

/* ---- shared ---- */

/* set up a barge: from the level loader, with its type in $t3 */
REGS(t3, s2, t7, s3, s0, s1)
void func_802C80D0(s32 type, u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 k;

    ENGINE_BLK(802C80D0);
    engine_save(ENGINE_T0_T5, 0);
    if (type == 0xB) {
        ENGINE_BLK(802C80F8);
        func_802C8150(model, x, y, z, heading);
        ENGINE_BLK(802C8100);
        k = 0;
    } else {
        ENGINE_BLK(802C8108);
        if (type == 0x11) {
            ENGINE_BLK(802C8114);
            func_802C8470(model, x, y, z, heading);
            ENGINE_BLK(802C811C);
            k = 1;
        } else {
            ENGINE_BLK(802C8124);
            func_802C8790(model, x, y, z, heading);
            k = 2;
        }
    }
    ENGINE_BLK(802C812C);
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(&D_803F8550[k]));
    ENGINE_LEAVE(18, T(&D_803F8748[3 * k + 1]));
    ENGINE_LEAVE(22, T(D_803F8778[2 * k]));
    ENGINE_LEAVE(23, T(D_803F8778[2 * k + 1]));
}

/* hd.c's: the player gets in */
void func_802C8AB0(void) {
    ENGINE_BLK(802C8AB0);
    D_8036444C = 3000;
    D_80364450 = 0;
    func_802C4310(0x72);
    ENGINE_BLK(802C8AE0);
}

/* hd.c's: whether it can be left (always) */
u8 func_802C8AF0(void) {
    ENGINE_BLK(802C8AF0);
    return 1;
}

/* hd.c's: the player gets out of the barge of this type */
void func_802C8B0C(u8 type) {
    ENGINE_BLK(802C8B0C);
    engine_save(ENGINE_GPR(28), 0);
    if (type == 0x11) {
        ENGINE_BLK(802C8B54);
        func_802A7764((u32 *)D_803F8778[2], (u32 *)D_803F8778[3], 0x800);
        ENGINE_BLK(802C8B74);
    } else {
        ENGINE_BLK(802C8B20);
        if (type == 0x12) {
            ENGINE_BLK(802C8B7C);
            func_802A7764((u32 *)D_803F8778[4], (u32 *)D_803F8778[5], 0x800);
        } else {
            ENGINE_BLK(802C8B2C);
            func_802A7764((u32 *)D_803F8778[0], (u32 *)D_803F8778[1], 0x800);
            ENGINE_BLK(802C8B4C);
        }
    }
    ENGINE_BLK(802C8B9C);
    func_802C444C();
    ENGINE_BLK(802C8BA4);
    engine_restore();
}

/* each frame: the barge of this type */
void func_802C8BB8(u8 type) {
    ENGINE_BLK(802C8BB8);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    if (type == 0xB) {
        ENGINE_BLK(802C8C0C);
        func_802C8C90(type);
        ENGINE_BLK(802C8C14);
    } else {
        ENGINE_BLK(802C8C1C);
        if (type == 0x11) {
            ENGINE_BLK(802C8C28);
            func_802C8FA8(type);
            ENGINE_BLK(802C8C30);
        } else {
            ENGINE_BLK(802C8C38);
            func_802C92C0(type);
        }
    }
    ENGINE_BLK(802C8C40);
    engine_restore();
}

/* bumped: the speed at least 0x50 either way, then turned round and halved */
REGS(gp)
void func_802C95D8(VS *vs) {
    s32 v = vs->unk76;

    ENGINE_BLK(802C95D8);
    if (v < 0) {
        ENGINE_BLK(802C95FC);
        if (!(v < -0x4F)) {
            ENGINE_BLK(802C9608);
            v = -0x50;
        }
    } else {
        ENGINE_BLK(802C95EC);
        if (v < 0x50) {
            ENGINE_BLK(802C95F4);
            v = 0x50;
        }
    }
    ENGINE_BLK(802C960C);
    vs->unk76 = -v >> 1;
}

/* the turn rate: the speed / 10.6 */
REGS(gp -> s3)
s32 func_802C9AF8(VS *vs) {
    ENGINE_BLK(802C9AF8);
    return engine_cvt_w_s((f32)vs->unk76 / 10.6f);
}

/* the camera's distance and speed for a barge */
void func_802C9B30(void) {
    ENGINE_BLK(802C9B30);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 4;
}
