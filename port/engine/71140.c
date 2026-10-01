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
 * Still translated: func_802B6100 and func_802B618C, what 62740's collision
 * dispatch calls for this vehicle; they go native with that.
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

/* 77E20 (engine-B's), still translated: an object moved by (dx, dz) */
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

    ENGINE_BLK(802B5900);
    engine_save(ENGINE_T0_T5, 0);
    D_803EEB44 = model;
    buf = D_80358070;
    D_803EEB48 = buf;
    D_803EEB4C = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(5, 0, D_803EEB48, D_803EEB4C, model);
    ENGINE_BLK(802B5980);
    func_802A754C(vs);
    ENGINE_BLK(802B598C);
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
    func_802A992C(vs->unk52, D_803EEB3C, x, z, vs->unk4, &D_803EEB3C, (s16 *)&vs->unk4C, 5, vs, engine_ctx(30), &avg);
    ENGINE_BLK(802B5A2C);
    func_8029F85C(p, D_803EEB44, D_803EEB48, D_803EEB4C);
    ENGINE_BLK(802B5A68);
    func_802A039C(0, 100, p);
    ENGINE_BLK(802B5A7C);
    func_802A03D4(0, 0, p);
    ENGINE_BLK(802B5A90);
    func_802A040C(0, 0, p);
    ENGINE_BLK(802B5AA4);
    func_802A0480(0, 0, p, 0.0f);
    ENGINE_BLK(802B5ABC);
    func_802A0290(0, 1, p);
    ENGINE_BLK(802B5AD0);
    func_8029E558(p, D_803EEB48, D_803EEB4C);
    ENGINE_BLK(802B5AE4);
    func_802A0320(0, p);
    ENGINE_BLK(802B5AF4);
    func_802A0290(0, 1, p);
    ENGINE_BLK(802B5B08);
    func_8029E558(p, D_803EEB4C, D_803EEB48);
    ENGINE_BLK(802B5B1C);
    r = vs->unk78;
    r[0] = -0x78, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0x50, r[5] = 6;
    r[6] = 0x50, r[7] = 0x64, r[8] = 4;
    r[9] = 0x64, r[10] = 0x8C, r[11] = 2;
    r[12] = 0x8C, r[13] = 0xDC, r[14] = 2;
    func_802A6F00(vs);
    ENGINE_BLK(802B5B98);
    D_803EEB60 = 0;
    D_803EEB5E = 0;
    D_803EEB5F = 0x32;
    D_803EEB50 = 0.5f;
    D_803EEB54 = 0.5f;
    func_8029C354(5, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x4268);
    ENGINE_BLK(802B5C04);
    func_80258230(5, 0x64, 0x2D, 0x2D);
    ENGINE_BLK(802B5C1C);
    vs->unk9A = 1;
    func_802B6294();
    ENGINE_BLK(802B5C2C);
    vs->unk9A = 0;
    func_802AA838(D_803EEB4C, D_803EEB48, *(s32 *)(D_803EEB44 + *(s32 *)(D_803EEB44 + 0x18) + 4));
    ENGINE_BLK(802B5C64);
    h = camera_turn((u16)vs->unk4C);
    if (!(h < 0x1000)) {
        ENGINE_BLK(802B5CA8);
        h -= 0xFFF;
    }
    ENGINE_BLK(802B5CAC);
    D_803EEB5C = h;
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(18, T(&D_803EEB3C));
    ENGINE_LEAVE(22, T(D_803EEB48));
    ENGINE_LEAVE(23, T(D_803EEB4C));
}

/* hd.c's: the player gets in: the bed and wheels reset */
void func_802B5CD8(void) {
    VS *vs = &D_803EEA90;
    Part *p = D_803EE790;

    ENGINE_BLK(802B5CD8);
    vs->unk96[3] = 0;
    func_802A039C(1, 0, p);
    ENGINE_BLK(802B5D00);
    func_802A03D4(1, 0, p);
    ENGINE_BLK(802B5D14);
    func_802A040C(1, 0, p);
    ENGINE_BLK(802B5D28);
    func_802A0290(1, -1, p);
    ENGINE_BLK(802B5D3C);
    func_802A039C(2, 0, p);
    ENGINE_BLK(802B5D50);
    func_802A03D4(2, 0, p);
    ENGINE_BLK(802B5D64);
    func_802A040C(2, 1, p);
    ENGINE_BLK(802B5D78);
    func_802A0360(2, 0, p, 0.5f);
    ENGINE_BLK(802B5D98);
    func_802A0290(2, -1, p);
    ENGINE_BLK(802B5DAC);
    func_802A039C(4, 0, p);
    ENGINE_BLK(802B5DC0);
    func_802A03D4(4, 0, p);
    ENGINE_BLK(802B5DD4);
    func_802A040C(4, 1, p);
    ENGINE_BLK(802B5DE8);
    func_802A0290(4, -1, p);
    ENGINE_BLK(802B5DFC);
    func_802A039C(5, 0, p);
    ENGINE_BLK(802B5E10);
    func_802A03D4(5, 0, p);
    ENGINE_BLK(802B5E24);
    func_802A040C(5, 1, p);
    ENGINE_BLK(802B5E38);
    func_802A0290(5, -1, p);
    ENGINE_BLK(802B5E4C);
    func_802A05D0(D_802C2208, 0);
    ENGINE_BLK(802B5E5C);
    func_802A05F8(D_802C2208, 0);
    ENGINE_BLK(802B5E6C);
    func_802A0620(D_802C2208, 0);
    ENGINE_BLK(802B5E7C);
    func_802A0508(D_802C2208, -1);
    ENGINE_BLK(802B5E8C);
    func_802A05D0(D_802C226C, 0);
    ENGINE_BLK(802B5E9C);
    func_802A05F8(D_802C226C, 0);
    ENGINE_BLK(802B5EAC);
    func_802A0620(D_802C226C, 0);
    ENGINE_BLK(802B5EBC);
    func_802A0508(D_802C226C, -1);
    ENGINE_BLK(802B5ECC);
    D_8036444C = 0xD48;
    D_80364450 = 0x12C;
    func_802C4310(5);
    ENGINE_BLK(802B5EF4);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B5F04(void) {
    VS *vs = &D_803EEA90;
    u8 r = 0;

    ENGINE_BLK(802B5F04);
    engine_save(ENGINE_GPR(28), 0);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802B5F28);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802B5F38);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802B5F48);
                r = 1;
            }
        }
    }
    ENGINE_BLK(802B5F4C);
    engine_restore();
    return r;
}

/* hd.c's: the player gets out */
void func_802B5F60(void) {
    VS *vs = &D_803EEA90;

    ENGINE_BLK(802B5F60);
    engine_save(ENGINE_GPR(28), 0);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EEB48, (u32 *)D_803EEB4C, 0x800);
    ENGINE_BLK(802B5F90);
    func_802C444C();
    ENGINE_BLK(802B5F98);
    engine_restore();
}

/* hd.c's: put back on the ground where it is */
void func_802B5FAC(void) {
    VS *vs = &D_803EEA90;

    ENGINE_BLK(802B5FAC);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802A9A60(vs->unk52, D_803EEB3C, D_803EEB38, D_803EEB40, vs->unk4, &D_803EEB3C, (s16 *)&vs->unk4C, 5, vs,
                  engine_ctx(30));
    ENGINE_BLK(802B6038);
    func_802B7030(vs);
    ENGINE_BLK(802B6040);
    func_802A133C(D_803EEB38, D_803EEB3C, D_803EEB40, 5, vs);
    ENGINE_BLK(802B606C);
    engine_restore();
}

/* its light */
void func_802B60BC(void) {
    ENGINE_BLK(802B60BC);
    func_802ABD54(5, D_803EEB38, D_803EEB3C, D_803EEB40);
    ENGINE_BLK(802B60F0);
}

/* each frame */
void func_802B6294(void) {
    VS *vs = &D_803EEA90;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802B6294);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802B60BC();
    ENGINE_BLK(802B62E8);
    func_802A75DC((u8 *)D_803EE790, &D_803EEB38, &D_803EEB3C, &D_803EEB40, (u8 *)vs);
    ENGINE_BLK(802B6314);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802B6320);
        func_802B6C28(vs);
    }
    ENGINE_BLK(802B6328);
    if (D_80367BFF != 0) {
        ENGINE_BLK(802B6338);
        func_802CB690(vs);
    }
    ENGINE_BLK(802B6340);
    func_802B7240();
    ENGINE_BLK(802B6348);
    rate_i = func_802B7168(vs);
    ENGINE_BLK(802B6350);
    func_802A7834(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 8, rate_i, &vs->unk4C, vs, &t3, &turn);
    ENGINE_BLK(802B636C);
    func_802A7FD8(&vs->unk74, D_803EEB58, &vs->unk76, &vs->unk4C, &vs->unk4E, &vs->unk96[3], 1, vs);
    ENGINE_BLK(802B638C);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802B6398);
    func_802A843C(&vs->unk76, 5, vs->unk96, vs->unk4, 1, 720.0f, vs);
    ENGINE_BLK(802B63AC);
    if (D_803EEB60 != 0) {
        ENGINE_BLK(802B63BC);
        func_802A7070((s16 *)&D_803EEB5A, vs);
    }
    ENGINE_BLK(802B63C8);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EEB38, &D_803EEB40, rate, &z);
    ENGINE_BLK(802B63E0);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EEB38, &D_803EEB40, &D_803EEB3C, 5, 0x2D0, 0x2D0, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802B6418);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B6440);
        func_8029E558(D_803EE790, D_803EEB48, D_803EEB4C);
        ENGINE_BLK(802B6454);
    } else {
        ENGINE_BLK(802B645C);
        func_8029E558(D_803EE790, D_803EEB4C, D_803EEB48);
    }
    /* the engine's rumble on the ground */
    ENGINE_BLK(802B6470);
    if (vs->unk96[3] != 0) {
        ENGINE_BLK(802B6480);
        if (func_802794F0() == 0) {
            ENGINE_BLK(802B6488);
            ENGINE_BLK(802B6490);
            func_80278EB0(6, 0.25f, 100);
            ENGINE_BLK(802B64A8);
        } else {
            ENGINE_BLK(802B6488);
        }
    } else {
        ENGINE_BLK(802B64B0);
        func_802794A4();
        ENGINE_BLK(802B64B8);
    }
    ENGINE_BLK(802B64BC);
    func_802B69F8(vs);
    ENGINE_BLK(802B64C4);
    if (D_803EEB5E != 0) {
        ENGINE_BLK(802B64D8);
        D_803EEB5E--;
        goto sparks_done;
    }
    ENGINE_BLK(802B64E4);
    if (vs->unk96[3] == 0)
        goto sparks_done;
    ENGINE_BLK(802B64F4);
    D_803EEB5E = 1;
    v = func_802A5ED0();
    ENGINE_BLK(802B6508);
    if (!(v < 4))
        goto sparks_done;
    ENGINE_BLK(802B6518);
    func_802A6274(T(D_802C2954), 0x30D40, 1, 5, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
sparks_done:
    ENGINE_BLK(802B6544);
    func_802AC284(&D_803EEB38, &D_803EEB3C, &D_803EEB40);
    ENGINE_BLK(802B6560);
    func_802B7030(vs);
    ENGINE_BLK(802B6568);
    ENGINE_LEAVE(8, 8);         /* $t0 and $t2, which func_8029A800 reads too */
    ENGINE_LEAVE(10, 0x64);
    func_8029A800(D_803EEB38, D_803EEB3C, D_803EEB40, D_80305D20, 1, 1, vs->unk76, 0, 5, vs);
    ENGINE_BLK(802B65B4);
    func_8029C52C(5, vs);
    ENGINE_BLK(802B65BC);
    func_8029AA10();
    ENGINE_BLK(802B65C4);
    D_803EEB61 = 0xC;
    if (D_803A7425 == 0) {
        ENGINE_BLK(802B65DC);
        D_803A7424 = 0;
        D_803EEB61 = -1;
        if (D_80364AA8 == 0x40)
            goto level_over;
        ENGINE_BLK(802B6604);
        D_803F77D0 = D_803EE790;
        func_802BE77C(5, vs);
        ENGINE_BLK(802B6620);
        if (D_803A7424 != 0)
            goto hit;
        ENGINE_BLK(802B6630);
level_over:
        ENGINE_BLK(802B67C4);
        D_803EEB60 = 0;
        goto done;
    }
    /* D_803A7425: turned toward the camera's heading */
    ENGINE_BLK(802B6638);
    func_8029A914(vs);
    ENGINE_BLK(802B6640);
    D_803EEB60 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802B6650);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802B6660);
        h += 0xFFF;
    }
    ENGINE_BLK(802B6664);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802B6670);
        h = -h;
    }
    ENGINE_BLK(802B6674);
    if (!(h < 0x801)) {
        ENGINE_BLK(802B6680);
    }
    ENGINE_BLK(802B6688);
    ENGINE_BLK(802B6754);
    func_802A70D8(vs);
    ENGINE_BLK(802B675C);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.18f, &a1);

        ENGINE_BLK(802B6770);
        D_803EEB5A = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802B6784);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802B678C);
    if (D_80364AA8 == 0x40)
        goto done;
    ENGINE_BLK(802B67A0);
    D_803F77D0 = D_803EE790;
    func_802BE77C(5, vs);
    ENGINE_BLK(802B67BC);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    ENGINE_BLK(802B669C);
    D_803EEB60 = 0;
    if (D_8035805C != 0) {
        ENGINE_BLK(802B66B4);
        a2 = D_803EEB4C;
        a3 = D_803EEB48;
    } else {
        ENGINE_BLK(802B66D0);
        a2 = D_803EEB48;
        a3 = D_803EEB4C;
    }
    ENGINE_BLK(802B66E8);
    func_802A768C((u8 *)D_803EE790, &D_803EEB38, &D_803EEB3C, &D_803EEB40, (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    ENGINE_BLK(802B6710);
    v = vs->unk76;
    if (v >= 0) {
        ENGINE_BLK(802B671C);
        if (v < 0x50) {
            ENGINE_BLK(802B6724);
            v = 0x50;
        }
    } else {
        ENGINE_BLK(802B672C);
        if (!(v < -0x4F)) {
            ENGINE_BLK(802B6738);
            v = -0x50;
        }
    }
    ENGINE_BLK(802B673C);
    vs->unk76 = -v >> 1;
    func_802B7030(vs);
    ENGINE_BLK(802B674C);

done:
    ENGINE_BLK(802B67CC);
    D_803643E0 = D_803EEB38;
    D_803643E4 = D_803EEB3C;
    D_803643E8 = D_803EEB40;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 5, vs);
    ENGINE_BLK(802B6848);
    D_803F77D0 = D_803EE790;
    func_802AC2A4(D_803EEB38, D_803EEB3C, D_803EEB40, D_80305D20, 5, vs);
    ENGINE_BLK(802B6880);
    if (D_803EEB61 >= 0) {
        ENGINE_BLK(802B6890);
        func_80260650(D_80367738, D_803EEB61, NULL);
        ENGINE_BLK(802B6924);
    }
    ENGINE_BLK(802B69A4);
    engine_restore();
}

/* dust behind it on the ground, thrown to the side the bed tilts to */
REGS(gp)
void func_802B69F8(VS *vs) {
    f32 f;
    s32 t1, t2;

    ENGINE_BLK(802B69F8);
    if (vs->unk96[3] == 0)
        goto done;
    ENGINE_BLK(802B6A88);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802B6A98);
    if (!(vs->unk50 < 3))
        goto done;
    ENGINE_BLK(802B6AA8);
    if (vs->unk9B != 0)
        goto done;
    ENGINE_BLK(802B6AB4);
    f = D_803EEB54;
    if (f <= 0.5f) {
        ENGINE_BLK(802B6AD0);
        f = 0.5f - f;
        f = f * 2.0f;
        f = f * 20.0f;
        t1 = engine_cvt_w_s(f);
        t2 = t1 + 0x28;
        t1 = 0x28 - t1;
    } else {
        ENGINE_BLK(802B6B10);
        f = f - 0.5f;
        f = f * 2.0f;
        f = f * 20.0f;
        t1 = engine_cvt_w_s(f);
        t2 = 0x28 - t1;
        t1 = t1 + 0x28;
    }
    ENGINE_BLK(802B6B4C);
    func_8027BE7C(3, vs->unk4[6], 0x190, -0x190, -0x190, -0x190, D_803EEB38, D_803EEB40, vs->unk4E, 5, t1, t2, 0);
    ENGINE_BLK(802B6BA0);
done:
    ENGINE_BLK(802B6BA4);
}

/* the parts: the front wheels turn with the camera, the bed tilts with the
   C buttons, the suspension with the stick, the wheels spin with the
   speed, the cab leans with the Z/R buttons; and the engine's sound */
REGS(gp)
void func_802B6C28(VS *vs) {
    Part *p = D_803EE790;
    s32 a, s, t1;
    f32 f, g, h;

    ENGINE_BLK(802B6C28);
    a = camera_turn((u16)vs->unk4C);
    if (!(a < 0x1000)) {
        ENGINE_BLK(802B6C74);
        a -= 0xFFF;
    }
    ENGINE_BLK(802B6C78);
    a -= D_803EEB5C;
    if (a < 0) {
        ENGINE_BLK(802B6C8C);
        a += 0xFFF;
    }
    ENGINE_BLK(802B6C90);
    if (!(a < 0x800)) {
        ENGINE_BLK(802B6C9C);
        a -= 0x800;
    }
    ENGINE_BLK(802B6CA0);
    ENGINE_BLK(802B6CC4);
    a = (u32)a / 0x55;
    func_802A05A4(D_802C2208, a, 0.0f);
    ENGINE_BLK(802B6CD4);
    func_802A05A4(D_802C226C, a, 0.0f);
    ENGINE_BLK(802B6CE8);
    f = D_803EEB54;
    if (D_80370C15 != 0) {
        ENGINE_BLK(802B6D08);
        f = f - 0.05f;
        g = func_802B71DC(vs, 0);
        ENGINE_BLK(802B6D18);
        if (f < g) {
            ENGINE_BLK(802B6D24);
            f = g;
        }
    } else {
        ENGINE_BLK(802B6D2C);
        if (D_80370C16 != 0) {
            ENGINE_BLK(802B6D40);
            f = f + 0.05f;
            g = func_802B71DC(vs, 1);
            ENGINE_BLK(802B6D50);
            if (!(f <= g)) {
                ENGINE_BLK(802B6D5C);
                f = g;
            }
        } else {
            ENGINE_BLK(802B6D64);
            h = 0.5f;
            if (f < h) {
                ENGINE_BLK(802B6D84);
                f = f + 0.05f;
                if (!(f <= h)) {
                    ENGINE_BLK(802B6D94);
                    f = h;
                }
            } else {
                ENGINE_BLK(802B6D9C);
                f = f - 0.05f;
                if (f < h) {
                    ENGINE_BLK(802B6DAC);
                    f = h;
                }
            }
        }
    }
    ENGINE_BLK(802B6DB0);
    D_803EEB54 = f;
    func_802A0360(5, 0, p, f);
    ENGINE_BLK(802B6DD4);
    f = D_803EEB50;
    if (func_802A7CB0(0xA, vs) == 0) {
        ENGINE_BLK(802B6DE8);
        ENGINE_BLK(802B6DF0);
        if (D_80370C23 != 0) {
            ENGINE_BLK(802B6E04);
            f = f - 0.05f;
            if (f < 0.0f) {
                ENGINE_BLK(802B6E20);
                f = 0.0f;
            }
            goto susp;
        }
        ENGINE_BLK(802B6E28);
        if (D_80370C1C != 0) {
            ENGINE_BLK(802B6E3C);
            f = f + 0.05f;
            if (!(f <= 1.0f)) {
                ENGINE_BLK(802B6E5C);
                f = 1.0f;
            }
            goto susp;
        }
    } else {
        ENGINE_BLK(802B6DE8);
    }
    ENGINE_BLK(802B6E64);
    h = 0.5f;
    if (f < h) {
        ENGINE_BLK(802B6E84);
        f = f + 0.03f;
        if (!(f <= h)) {
            ENGINE_BLK(802B6E94);
            f = h;
        }
    } else {
        ENGINE_BLK(802B6E9C);
        f = f - 0.03f;
        if (f < h) {
            ENGINE_BLK(802B6EAC);
            f = h;
        }
    }
susp:
    ENGINE_BLK(802B6EB0);
    D_803EEB50 = f;
    func_802A0360(4, 0, p, f);
    ENGINE_BLK(802B6ED4);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802B6EE0);
        func_802A03D4(1, 1, p);
        ENGINE_BLK(802B6EF4);
    } else {
        ENGINE_BLK(802B6EFC);
        func_802A03D4(1, 0, p);
    }
    ENGINE_BLK(802B6F10);
    if (s < 0) {
        ENGINE_BLK(802B6F18);
        s = -s;
    }
    ENGINE_BLK(802B6F1C);
    if (s != 0) {
        ENGINE_BLK(802B6F24);
        s = (u32)s / 14;
    }
    ENGINE_BLK(802B6F40);
    func_802A039C(1, s, p);
    ENGINE_BLK(802B6F54);
    func_802C4584(s);
    ENGINE_BLK(802B6F5C);
    t1 = D_803EEB5F;
    if (D_80370C15 != 0) {
        ENGINE_BLK(802B6FC4);
        t1 -= 5;
        if (t1 < 0) {
            ENGINE_BLK(802B6FD0);
            t1 = 0;
        }
    } else {
        ENGINE_BLK(802B6F78);
        if (D_80370C16 != 0) {
            ENGINE_BLK(802B6FD8);
            t1 += 5;
            if (!(t1 < 0x65)) {
                ENGINE_BLK(802B6FE8);
                t1 = 0x64;
            }
        } else {
            ENGINE_BLK(802B6F8C);
            if (t1 < 0x32) {
                ENGINE_BLK(802B6FAC);
                t1 += 10;
                if (!(t1 < 0x33)) {
                    ENGINE_BLK(802B6FBC);
                    t1 = 0x32;
                }
            } else {
                ENGINE_BLK(802B6F94);
                t1 -= 10;
                if (t1 < 0x32) {
                    ENGINE_BLK(802B6FA4);
                    t1 = 0x32;
                }
            }
        }
    }
    ENGINE_BLK(802B6FEC);
    D_803EEB5F = t1;
    func_802A0360(2, 0, p, (f32)t1 / 100.0f);
    ENGINE_BLK(802B701C);
}

/* the truck's matrix, its vertices and its collision */
REGS(gp)
void func_802B7030(VS *vs) {
    u8 *model = D_803EEB44, *buf;
    s32 *m, off;

    ENGINE_BLK(802B7030);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B7060);
        m = (s32 *)(D_803EEB48 + off);
    } else {
        ENGINE_BLK(802B7074);
        m = (s32 *)(D_803EEB4C + off);
    }
    ENGINE_BLK(802B7084);
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EEB38, D_803EEB3C, D_803EEB40, 0x4268, m);
    ENGINE_BLK(802B70C0);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B70D4);
        buf = D_803EEB48;
    } else {
        ENGINE_BLK(802B70E4);
        buf = D_803EEB4C;
    }
    ENGINE_BLK(802B70F0);
    model = D_803EEB44;
    func_8029C454(D_803EEB38, D_803EEB3C, D_803EEB40, 5, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802B7138);
    func_802ABBEC(5, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802B7158);
}

/* the turn rate: the speed / 2.6, or / 6 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B7168(VS *vs) {
    f32 d;

    ENGINE_BLK(802B7168);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802B717C);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802B718C);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802B719C);
    d = 2.6f;
    goto div;
air:
    ENGINE_BLK(802B71A4);
    d = 6.0f;
div:
    ENGINE_BLK(802B71B0);
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the bed's tilt limit at this speed: 0.5 -/+ |speed| / 240 / 2 */
REGS(gp, s2 -> f2)
f32 func_802B71DC(VS *vs, s32 up) {
    s32 s = vs->unk76;
    f32 f;

    ENGINE_BLK(802B71DC);
    if (s < 0) {
        ENGINE_BLK(802B71F4);
        s = -s;
    }
    ENGINE_BLK(802B71F8);
    f = (f32)s / 240.0f * 0.5f;
    if (up != 0) {
        ENGINE_BLK(802B7224);
        f = f + 0.5f;
    } else {
        ENGINE_BLK(802B722C);
        f = 0.5f - f;
    }
    ENGINE_BLK(802B7230);
    ENGINE_LEAVE(19, 0xF0);     /* ($s3, and $f4) */
    ENGINE_LEAVE_F(4, 0.5f);
    return f;
}

/* the camera's distance and speed for the truck, and its turn's speed */
void func_802B7240(void) {
    u16 v;

    ENGINE_BLK(802B7240);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 2;
    if (D_80370C1A != 0)
        goto slow;
    ENGINE_BLK(802B72A0);
    if (D_80370C1B != 0)
        goto slow;
    ENGINE_BLK(802B72B4);
    v = 0x2328;
    goto set;
slow:
    ENGINE_BLK(802B72BC);
    v = 0xC80;
set:
    ENGINE_BLK(802B72C0);
    D_803EEB58 = v;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B72DC(u8 *dst) {
    ENGINE_BLK(802B72DC);
    func_802AC7DC(dst, (u8 *)&D_803EEA90, (u32 *)&D_803EEB38);
    ENGINE_BLK(802B72F8);
}

/* and back */
void func_802B7308(u8 *src) {
    ENGINE_BLK(802B7308);
    func_802AC85C(src, (u8 *)&D_803EEA90, (u32 *)&D_803EEB38);
    ENGINE_BLK(802B7324);
}

/* ---- 679E0's, which only the truck calls ---- */

/* the position (*x, *y, *z) kept in the level's bounds (func_802AC3B8) */
REGS(v0, v1, a0)
void func_802AC284(s32 *x, s32 *y, s32 *z) {
    ENGINE_BLK(802AC284);
    func_802AC3B8(x, y, z);
    ENGINE_BLK(802AC294);
}

/* the level over (D_80364AA8 0x40): the parts' last frame and the
   buildings', and whatever it carries (D_803F3910..D_803F3960) thrown far
   off, with a sound; with D_8036E4C8 clear, D_803BE738 instead */
REGS(v0, v1, a0, a1, t8, gp)
void func_802AC2A4(s32 x, s32 y, s32 z, u8 *a1, s32 type, VS *vs) {
    u8 *PTR32 *q;
    u8 *o;
    s32 dx, dz;

    ENGINE_BLK(802AC2A4);
    if (D_80364AA8 != 0x40)
        goto done;
    ENGINE_BLK(802AC2BC);
    func_8029A800(x, y, z, a1, 1, 0, 0, 0, type, vs);
    ENGINE_BLK(802AC2D0);
    func_802BE77C(type, vs);
    ENGINE_BLK(802AC2D8);
    if (D_803F3910 == D_803F3960)
        goto done;
    ENGINE_BLK(802AC2F0);
    if (D_8036E4C8 == 0) {
        ENGINE_BLK(802AC300);
        D_803BE738 = 1;
        goto done;
    }
    ENGINE_BLK(802AC310);
    func_80260650(D_80367738, 0x3D, NULL);
    ENGINE_BLK(802AC328);
    for (q = D_803F3910;; q += 2) {
        ENGINE_BLK(802AC33C);
        if (q == D_803F3960)
            break;
        ENGINE_BLK(802AC344);
        o = q[0];
        *(s32 *)(o + 0x38) = 1;
        *(s32 *)(o + 0x40) = -1;
        dx = 0xBB80 - *(s32 *)(o + 0x10);
        *(s32 *)(o + 0x10) = 0xBB80;
        dz = 0xBB80 - *(s32 *)(o + 0x18);
        *(s32 *)(o + 0x18) = 0xBB80;
        func_802BD99C(o, dx, 0, dz);
        ENGINE_BLK(802AC390);
    }
done:
    ENGINE_BLK(802AC3A8);
}
