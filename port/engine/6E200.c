/*
 * hd_code 6E200 (us.v11 0x802B29C0-0x802B5900): Skyfall (type 3) and
 * Ramdozer (type 4), as native C (engine.h).
 *
 * Skyfall's parts are D_803EDFE0, its state D_803EE2E0 and its position
 * D_803EE38C..94.  func_802B29C0 sets it up (from the level loader),
 * func_802B327C runs it each frame (from hd.c and at the end of the setup);
 * the others are hd.c's hooks for it.  It is the hotrod's code (72B80) with
 * a boost: a button held (D_80370C1A or D_80370C1B) burns D_803EE3B1's fuel
 * for speed, with its sound and a flame; part 3 is tilted by D_803EE3A4,
 * which two other buttons move (D_80370C23 and D_80370C1C).
 *
 * Ramdozer's parts are D_803EE3C0, its state D_803EE6C0 and its position
 * D_803EE76C..74 (see further down).
 *
 * Each one's two callbacks for 62740's carrying (shared.h) follow its
 * frame's functions: func_802B30F4 and func_802B3180, func_802B4818 and
 * func_802B48A4.
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* Skyfall's .bss (asm/data/hd_code/6E200.bss.s) */
extern Part D_803EDFE0[32];
extern VS D_803EE2E0;
extern SndState *PTR32 D_803EE388;              /* its boost's sound, while it plays */
extern s32 D_803EE38C, D_803EE390, D_803EE394;  /* x, y, z */
extern u8 *PTR32 D_803EE398;                    /* its model file */
extern u8 *PTR32 D_803EE39C;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EE3A0;
extern f32 D_803EE3A4;                          /* part 3's tilt, 0..1 */
extern u16 D_803EE3A8;                          /* the turn's rate (func_802A7FD8) */
extern u16 D_803EE3AA;                          /* the heading it turns to (with D_803A7425) */
extern s16 D_803EE3AC;                          /* the flame's size, after a boost */
extern u8 D_803EE3AE;                           /* frames until the next sparks */
extern u8 D_803EE3AF;                           /* part 2's position, 0..100 */
extern s8 D_803EE3B0;                           /* turning to D_803EE3AA */
extern u8 D_803EE3B2;                           /* frames without the gears after a hit */
extern u8 D_803EE3B3;                           /* boosting */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern u8 D_80364A69;
extern u16 D_803F7840;
extern u8 D_80370C15, D_80370C16, D_80370C1A, D_80370C1B, D_80370C1C, D_80370C23;
extern Part *PTR32 D_803F77D0;
extern s32 D_803643E4, D_803643E8;
extern u8 D_80305D00[];                         /* Skyfall's bounce records */
extern u8 D_802C2314[];                         /* the flame's part record (56040's list) */
extern u8 D_802C2954[];                         /* the sparks' effect record (60F60) */
extern u8 D_802C37C0[];                         /* the boost's effect record */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12);

void func_802B327C(void);
static void skyfall_frame(void);
REGS()
void func_802B30B0(void);
REGS(gp)
void func_802B37B0(VS *vs);
REGS(gp)
void func_802B3C68(VS *vs);
REGS(gp)
void func_802B3E40(VS *vs);
REGS(gp -> s3)
s32 func_802B3F78(VS *vs);
REGS(gp)
void func_802B3FF0(VS *vs);

#define T(p) ((s32)(p))
#define SKY D_803EDFE0

/* ---- Skyfall -------------------------------------------------------------- */

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802B29C0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EE2E0;
    u8 *buf;
    s16 *r;
    s32 avg;

    ENGINE_BLK(802B29C0);
    D_803EE398 = model;
    buf = D_80358070;
    D_803EE39C = buf;
    D_803EE3A0 = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(3, 0, D_803EE39C, D_803EE3A0, model);
    ENGINE_BLK(802B2A40);
    func_802A754C(vs);
    ENGINE_BLK(802B2A4C);
    vs->unk52[0] = 0x130;
    vs->unk52[1] = 0x16A;
    vs->unk52[2] = -0x130;
    vs->unk52[3] = 0x16A;
    vs->unk52[4] = 0x130;
    vs->unk52[5] = -0x16A;
    vs->unk5E[0] = 0x140;
    vs->unk5E[1] = 0x172;
    vs->unk5E[2] = -0x140;
    vs->unk5E[3] = 0x172;
    vs->unk5E[4] = 0x140;
    vs->unk5E[5] = -0x172;
    D_803EE38C = x;
    D_803EE390 = y;
    D_803EE394 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    D_803F7840 = 0;
    D_803EE3AF = 0x32;
    D_803EE3A4 = 0.5f;
    func_802A992C(vs->unk52, D_803EE390, x, z, vs->unk4, &D_803EE390, (s16 *)&vs->unk4C, 3, vs, 0,
                       &avg);
    ENGINE_BLK(802B2B28);
    func_8029F85C(SKY, D_803EE398, D_803EE39C, D_803EE3A0);
    ENGINE_BLK(802B2B64);
    func_802A039C(0, 100, SKY);
    ENGINE_BLK(802B2B78);
    func_802A03D4(0, 0, SKY);
    ENGINE_BLK(802B2B8C);
    func_802A040C(0, 0, SKY);
    ENGINE_BLK(802B2BA0);
    func_802A0480(0, 0, SKY, 0.0f);
    ENGINE_BLK(802B2BB8);
    func_802A0290(0, 1, SKY);
    ENGINE_BLK(802B2BCC);
    func_8029E558(SKY, D_803EE39C, D_803EE3A0);
    ENGINE_BLK(802B2BE0);
    func_802A0320(0, SKY);
    ENGINE_BLK(802B2BF0);
    func_802A0290(0, 1, SKY);
    ENGINE_BLK(802B2C04);
    func_8029E558(SKY, D_803EE3A0, D_803EE39C);
    ENGINE_BLK(802B2C18);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 5, r[2] = 2;
    r[3] = 0, r[4] = 0x78, r[5] = 4;
    r[6] = 0x78, r[7] = 0xA0, r[8] = 5;
    r[9] = 0xA0, r[10] = 0xDC, r[11] = 2;
    r[12] = 0xDC, r[13] = 0xFA, r[14] = 2;
    func_802A6F00(vs);
    ENGINE_BLK(802B2C94);
    D_803EE3B1 = 100;
    D_803EE3B2 = 0;
    D_803EE3AE = 0;
    D_803EE3B0 = 0;
    D_803EE388 = NULL;
    model = D_803EE398;
    func_8029C354(3, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x2AF8);
    ENGINE_BLK(802B2CEC);
    func_80258230(3, 0x50, 0x1F, 0x1F);
    ENGINE_BLK(802B2D04);
    vs->unk9A = 1;
    skyfall_frame();
    ENGINE_BLK(802B2D14);
    vs->unk9A = 0;
    model = D_803EE398;
    func_802AA838(D_803EE3A0, D_803EE39C, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802B2D4C);
    D_80364A69 = 1;
}

/* hd.c's: the player gets in: the doors shut, the flame out */
void func_802B2D7C(void) {
    VS *vs = &D_803EE2E0;
    Part *p = SKY;

    ENGINE_BLK(802B2D7C);
    vs->unk96[3] = 0;
    func_802A039C(1, 0, p);
    ENGINE_BLK(802B2DA4);
    func_802A03D4(1, 0, p);
    ENGINE_BLK(802B2DB8);
    func_802A040C(1, 0, p);
    ENGINE_BLK(802B2DCC);
    func_802A0290(1, -1, p);
    ENGINE_BLK(802B2DE0);
    func_802A039C(2, 0, p);
    ENGINE_BLK(802B2DF4);
    func_802A03D4(2, 0, p);
    ENGINE_BLK(802B2E08);
    func_802A040C(2, 1, p);
    ENGINE_BLK(802B2E1C);
    func_802A0290(2, -1, p);
    ENGINE_BLK(802B2E30);
    func_802A039C(3, 0, p);
    ENGINE_BLK(802B2E44);
    func_802A03D4(3, 0, p);
    ENGINE_BLK(802B2E58);
    func_802A040C(3, 1, p);
    ENGINE_BLK(802B2E6C);
    func_802A0290(3, -1, p);
    ENGINE_BLK(802B2E80);
    func_802A05D0(D_802C2314, 0);
    ENGINE_BLK(802B2E90);
    func_802A05F8(D_802C2314, 0);
    ENGINE_BLK(802B2EA0);
    func_802A0620(D_802C2314, 0);
    ENGINE_BLK(802B2EB0);
    func_802A0508(D_802C2314, -1);
    ENGINE_BLK(802B2EC0);
    D_8036444C = 0xD48;
    D_80364450 = 0x3E8;
    func_802C4310(0x8C);
    ENGINE_BLK(802B2EE8);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B2EF8(void) {
    VS *vs = &D_803EE2E0;
    u8 r = 0;

    ENGINE_BLK(802B2EF8);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802B2F1C);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802B2F2C);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802B2F3C);
                r = 1;
            }
        }
    }
    ENGINE_BLK(802B2F40);
    return r;
}

/* hd.c's: the player gets out */
void func_802B2F54(void) {
    VS *vs = &D_803EE2E0;

    ENGINE_BLK(802B2F54);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EE39C, (u32 *)D_803EE3A0, 0x800);
    ENGINE_BLK(802B2F84);
    func_802C444C();
    ENGINE_BLK(802B2F8C);
}

/* hd.c's: put back on the ground where it is */
void func_802B2FA0(void) {
    VS *vs = &D_803EE2E0;

    ENGINE_BLK(802B2FA0);
    func_802A9A60(vs->unk52, D_803EE390, D_803EE38C, D_803EE394, vs->unk4, &D_803EE390, (s16 *)&vs->unk4C, 3, vs,
                  0);
    ENGINE_BLK(802B302C);
    func_802B3E40(vs);
    ENGINE_BLK(802B3034);
    func_802A133C(D_803EE38C, D_803EE390, D_803EE394, 3, vs);
    ENGINE_BLK(802B3060);
}

/* its light */
REGS()
void func_802B30B0(void) {
    ENGINE_BLK(802B30B0);
    func_802ABD54(3, D_803EE38C, D_803EE390, D_803EE394);
    ENGINE_BLK(802B30E4);
}

/* each frame */
static void skyfall_frame(void) {
    VS *vs = &D_803EE2E0;
    s32 t3 = 0, x, z, rate_i, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802B327C);
    /* (its $fp as it found it: the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802B30B0();
    ENGINE_BLK(802B32D0);
    func_802A75DC((u8 *)SKY, &D_803EE38C, &D_803EE390, &D_803EE394, (u8 *)vs);
    ENGINE_BLK(802B32FC);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802B3308);
        func_802B37B0(vs);
    }
    ENGINE_BLK(802B3310);
    if (D_80367BFF != 0) {
        ENGINE_BLK(802B3320);
        func_802CB690(vs);
    }
    ENGINE_BLK(802B3328);
    func_802B3FF0(vs);
    ENGINE_BLK(802B3330);
    rate_i = func_802B3F78(vs);
    ENGINE_BLK(802B3338);
    func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    ENGINE_BLK(802B3354);
    if (D_803EE3B2 == 0) {
        ENGINE_BLK(802B3364);
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x14, vs, &t3);
        ENGINE_BLK(802B336C);
    } else {
        ENGINE_BLK(802B3374);
        D_803EE3B2--;
    }
    ENGINE_BLK(802B3380);
    func_802A7FD8(D_803EE3A8, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    ENGINE_BLK(802B33A0);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802B33AC);
    func_802A843C(&vs->unk76, 1, 3, (s8 *)vs->unk96, vs->unk4, 724.0f, vs);
    ENGINE_BLK(802B33C0);
    if (D_803EE3B0 != 0) {
        ENGINE_BLK(802B33D0);
        func_802A7070((s16 *)&D_803EE3AA, vs);
    }
    ENGINE_BLK(802B33DC);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EE38C, &D_803EE394, rate, &z);
    ENGINE_BLK(802B33F4);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EE38C, &D_803EE394, &D_803EE390, 3, 0x2D4, 0x260, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802B342C);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B3454);
        func_8029E558(SKY, D_803EE39C, D_803EE3A0);
        ENGINE_BLK(802B3468);
    } else {
        ENGINE_BLK(802B3470);
        func_8029E558(SKY, D_803EE3A0, D_803EE39C);
    }
    ENGINE_BLK(802B3484);
    func_802B3E40(vs);
    ENGINE_BLK(802B348C);
    func_8029A800(D_803EE38C, D_803EE390, D_803EE394, D_80305D00, 1, 1, 6, vs->unk76, 0x64, 0, 3, vs);
    ENGINE_BLK(802B34D8);
    func_8029C52C(3, vs);
    ENGINE_BLK(802B34E0);
    func_8029AA10();
    ENGINE_BLK(802B34E8);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802B34F8);
        D_803A7424 = 0;
        D_803F77D0 = SKY;
        func_802BE77C(3, vs);
        ENGINE_BLK(802B3518);
        if (D_803A7424 == 0) {
            ENGINE_BLK(802B3528);
            ENGINE_BLK(802B36B4);
            D_803EE3B0 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    ENGINE_BLK(802B3530);
    func_8029A914(vs);
    ENGINE_BLK(802B3538);
    D_803EE3B0 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802B3548);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802B3558);
        h += 0xFFF;
    }
    ENGINE_BLK(802B355C);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802B3568);
        h = -h;
    }
    ENGINE_BLK(802B356C);
    if (!(h < 0x801)) {
        ENGINE_BLK(802B3578);
    }
    ENGINE_BLK(802B3580);
    ENGINE_BLK(802B3658);
    func_802A70D8(vs);
    ENGINE_BLK(802B3660);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.1f, vs, &a1);

        ENGINE_BLK(802B3674);
        D_803EE3AA = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802B3688);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802B3690);
    D_803F77D0 = SKY;
    func_802BE77C(3, vs);
    ENGINE_BLK(802B36AC);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    ENGINE_BLK(802B3594);
    D_803EE3B0 = 0;
    if (D_8035805C != 0) {
        ENGINE_BLK(802B35AC);
        a2 = D_803EE3A0;
        a3 = D_803EE39C;
    } else {
        ENGINE_BLK(802B35C8);
        a2 = D_803EE39C;
        a3 = D_803EE3A0;
    }
    ENGINE_BLK(802B35E0);
    func_802A768C((u8 *)SKY, &D_803EE38C, &D_803EE390, &D_803EE394, (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    ENGINE_BLK(802B3608);
    D_803EE3B2 = 5;
    v = vs->unk76;
    if (v >= 0) {
        ENGINE_BLK(802B3620);
        if (v < 0x50) {
            ENGINE_BLK(802B3628);
            v = 0x50;
        }
    } else {
        ENGINE_BLK(802B3630);
        if (!(v < -0x4F)) {
            ENGINE_BLK(802B363C);
            v = -0x50;
        }
    }
    ENGINE_BLK(802B3640);
    vs->unk76 = -v >> 1;
    func_802B3E40(vs);
    ENGINE_BLK(802B3650);

done:
    ENGINE_BLK(802B36BC);
    D_803643E0 = D_803EE38C;
    D_803643E4 = D_803EE390;
    D_803643E8 = D_803EE394;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 3, vs);
    ENGINE_BLK(802B375C);
    engine_restore();
}

/* hd.c's: Skyfall each frame */
void func_802B327C(void) {
    skyfall_frame();
}

/* the parts: sparks off rough ground, part 3's tilt, part 2's position,
   the wheels' spin with the speed, the engine's sound, and the boost (while
   there is fuel) */
REGS(gp)
void func_802B37B0(VS *vs) {
    Part *p = SKY;
    s32 s, t1;
    f32 f0;

    ENGINE_BLK(802B37B0);
    if (D_803EE3AE != 0) {
        ENGINE_BLK(802B37D0);
        D_803EE3AE--;
        goto tilt;
    }
    ENGINE_BLK(802B37DC);
    if (vs->unk96[3] == 0)
        goto tilt;
    ENGINE_BLK(802B37EC);
    D_803EE3AE = 1;
    s = func_802A5ED0();
    ENGINE_BLK(802B3800);
    if (!(s < 0xF))
        goto tilt;
    ENGINE_BLK(802B3810);
    func_802A6274(T(D_802C2954), 0x29810, 1, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    ENGINE_BLK(802B383C);
    func_802A6274(T(D_802C2954), 0x29810, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
tilt:
    /* part 3's tilt: down with D_80370C23, up with D_80370C1C, else back to
       the middle; to the middle while func_802A7CB0 says so */
    ENGINE_BLK(802B3868);
    f0 = D_803EE3A4;
    if (func_802A7CB0(10, vs) != 0) {
        ENGINE_BLK(802B387C);
        goto middle;
    }
    ENGINE_BLK(802B387C);
    ENGINE_BLK(802B3884);
    if (D_80370C23 != 0) {
        ENGINE_BLK(802B3898);
        f0 -= 0.05f;
        if (f0 < 0.0f) {
            ENGINE_BLK(802B38B4);
            f0 = 0.0f;
        }
        goto set;
    }
    ENGINE_BLK(802B38BC);
    if (D_80370C1C == 0)
        goto middle;
    ENGINE_BLK(802B38D0);
    f0 += 0.05f;
    if (!(f0 <= 1.0f)) {
        ENGINE_BLK(802B38F0);
        f0 = 1.0f;
    }
    goto set;
middle:
    ENGINE_BLK(802B38F8);
    if (f0 < 0.5f) {
        ENGINE_BLK(802B3918);
        f0 += 0.03f;
        if (!(f0 <= 0.5f)) {
            ENGINE_BLK(802B3928);
            f0 = 0.5f;
        }
    } else {
        ENGINE_BLK(802B3930);
        f0 -= 0.03f;
        if (f0 < 0.5f) {
            ENGINE_BLK(802B3940);
            f0 = 0.5f;
        }
    }
set:
    ENGINE_BLK(802B3944);
    D_803EE3A4 = f0;
    func_802A0360(3, 0, p, f0);
    /* part 2: down with D_80370C15, up with D_80370C16, else back to the
       middle */
    ENGINE_BLK(802B3968);
    t1 = D_803EE3AF;
    if (D_80370C15 != 0) {
        ENGINE_BLK(802B39D0);
        t1 -= 5;
        if (t1 < 0) {
            ENGINE_BLK(802B39DC);
            t1 = 0;
        }
    } else {
        ENGINE_BLK(802B3984);
        if (D_80370C16 != 0) {
            ENGINE_BLK(802B39E4);
            t1 += 5;
            if (!(t1 < 0x65)) {
                ENGINE_BLK(802B39F4);
                t1 = 0x64;
            }
        } else {
            ENGINE_BLK(802B3998);
            if (t1 < 0x32) {
                ENGINE_BLK(802B39B8);
                t1 += 10;
                if (!(t1 < 0x33)) {
                    ENGINE_BLK(802B39C8);
                    t1 = 0x32;
                }
            } else {
                ENGINE_BLK(802B39A0);
                t1 -= 10;
                if (t1 < 0x32) {
                    ENGINE_BLK(802B39B0);
                    t1 = 0x32;
                }
            }
        }
    }
    ENGINE_BLK(802B39F8);
    D_803EE3AF = t1;
    func_802A0360(2, 0, p, (f32)t1 / 100.0f);
    ENGINE_BLK(802B3A28);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802B3A34);
        func_802A03D4(1, 1, p);
        ENGINE_BLK(802B3A48);
    } else {
        ENGINE_BLK(802B3A50);
        func_802A03D4(1, 0, p);
    }
    ENGINE_BLK(802B3A64);
    if (s < 0) {
        ENGINE_BLK(802B3A6C);
        s = -s;
    }
    ENGINE_BLK(802B3A70);
    if (s != 0) {
        ENGINE_BLK(802B3A78);
        s = (u32)s / 30;
    }
    ENGINE_BLK(802B3A94);
    func_802A039C(1, s, p);
    ENGINE_BLK(802B3AA8);
    func_802C4584(s);
    /* the boost */
    ENGINE_BLK(802B3AB0);
    if (D_803EE3B2 != 0) {
        ENGINE_BLK(802B3BA4);
        D_803EE3B2--;
        goto no_boost;
    }
    ENGINE_BLK(802B3AC0);
    if (D_80370C1A == 0) {
        ENGINE_BLK(802B3AD0);
        if (D_80370C1B == 0)
            goto no_boost;
    }
    ENGINE_BLK(802B3AE0);
    s = (s8)D_803EE3B1;  /* (the boost's fuel, 0..100; vehicle.h) */
    if (s < 4)
        goto no_boost;
    ENGINE_BLK(802B3AF4);
    D_803EE3B1 = s - 4;
    if (D_803EE3B3 == 0) {
        ENGINE_BLK(802B3B10);
        D_803EE3B3 = 1;
        func_80260650(D_80367738, 0x8D, &D_803EE388);
    }
    ENGINE_BLK(802B3B34);
    func_802A05A4(D_802C2314, 1, 0.0f);
    ENGINE_BLK(802B3B50);
    func_802A6274(T(D_802C37C0), 0x186A0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0);
    ENGINE_BLK(802B3B7C);
    s = vs->unk76;
    if (s < 0x190) {
        ENGINE_BLK(802B3B8C);
        vs->unk76 = s + 0x28;
    } else {
        ENGINE_BLK(802B3B98);
        vs->unk76 = 0x190;
    }
    goto dust;
no_boost:
    ENGINE_BLK(802B3BB0);
    if (D_803EE388 != NULL) {
        ENGINE_BLK(802B3BC0);
        func_802608C8(D_803EE388);
        ENGINE_BLK(802B3BC8);
        D_803EE388 = NULL;
    }
    ENGINE_BLK(802B3BD0);
    D_803EE3AC = 0x78;
    D_803EE3B3 = 0;
    func_802A05A4(D_802C2314, 0, 0.0f);
    ENGINE_BLK(802B3C00);
    s = vs->unk76;
    if (!(s < 0xFB)) {
        ENGINE_BLK(802B3C10);
        s -= 0x19;
        if (s < 0xFA) {
            ENGINE_BLK(802B3C20);
            s = 0xFA;
        }
        ENGINE_BLK(802B3C24);
        vs->unk76 = s;
    }
    ENGINE_BLK(802B3C28);
    s = (s8)D_803EE3B1 + 1;
    if (!(s < 0x65)) {
        ENGINE_BLK(802B3C40);
        s = 0x64;
    }
    ENGINE_BLK(802B3C44);
    D_803EE3B1 = s;
dust:
    ENGINE_BLK(802B3C4C);
    func_802B3C68(vs);
    ENGINE_BLK(802B3C54);
}

/* dust behind it on the ground, more of it while boosting */
REGS(gp)
void func_802B3C68(VS *vs) {
    s32 t1;

    ENGINE_BLK(802B3C68);
    if (vs->unk96[3] != 0) {
        ENGINE_BLK(802B3CF8);
        t1 = 0x32;
    } else {
        ENGINE_BLK(802B3D00);
        if (D_803EE3B3 == 0)
            goto done;
        ENGINE_BLK(802B3D10);
        t1 = D_803EE3AC;
        if (!(t1 < 8)) {
            ENGINE_BLK(802B3D28);
        } else {
            ENGINE_BLK(802B3D24);
            t1 = 8;
            ENGINE_BLK(802B3D28);
        }
        t1 -= 8;
        D_803EE3AC = t1;
    }
    ENGINE_BLK(802B3D34);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802B3D44);
    if (!(vs->unk50 < 3))
        goto done;
    ENGINE_BLK(802B3D54);
    if (vs->unk9B != 0)
        goto done;
    ENGINE_BLK(802B3D60);
    func_8027BE7C(2, vs->unk4[6], 0x190, -0x12C, -0x190, -0x12C, D_803EE38C, D_803EE394, vs->unk4E, 4, t1, t1, 0);
    ENGINE_BLK(802B3DB8);
done:
    ENGINE_BLK(802B3DBC);
}

/* Skyfall's matrix, its vertices and its collision */
REGS(gp)
void func_802B3E40(VS *vs) {
    u8 *model = D_803EE398, *buf;
    s32 *m, off;

    ENGINE_BLK(802B3E40);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B3E70);
        m = (s32 *)(D_803EE39C + off);
    } else {
        ENGINE_BLK(802B3E84);
        m = (s32 *)(D_803EE3A0 + off);
    }
    ENGINE_BLK(802B3E94);
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EE38C, D_803EE390, D_803EE394, 0x2AF8, m);
    ENGINE_BLK(802B3ED0);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B3EE4);
        buf = D_803EE39C;
    } else {
        ENGINE_BLK(802B3EF4);
        buf = D_803EE3A0;
    }
    ENGINE_BLK(802B3F00);
    model = D_803EE398;
    func_8029C454(D_803EE38C, D_803EE390, D_803EE394, 3, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802B3F48);
    func_802ABBEC(3, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802B3F68);
}

/* the turn rate: the speed / 2.5, or / 11 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B3F78(VS *vs) {
    f32 d;

    ENGINE_BLK(802B3F78);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802B3F8C);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802B3F9C);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802B3FAC);
    d = 2.5f;
    goto div;
air:
    ENGINE_BLK(802B3FB8);
    d = 11.0f;
div:
    ENGINE_BLK(802B3FC4);
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for Skyfall, and the turn's rate: 0x7D0
   with D_80370C23 held going forward (D_80370C1C going back), else 0x3A98 */
REGS(gp)
void func_802B3FF0(VS *vs) {
    s32 v;

    ENGINE_BLK(802B3FF0);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 3;
    if (!(vs->unk76 > 0)) {
        ENGINE_BLK(802B4048);
        if (D_80370C1C != 0) {
            ENGINE_BLK(802B4064);
            v = 0x7D0;
        } else {
            ENGINE_BLK(802B405C);
            v = 0x3A98;
        }
    } else {
        ENGINE_BLK(802B406C);
        if (D_80370C23 != 0) {
            ENGINE_BLK(802B4088);
            v = 0x7D0;
        } else {
            ENGINE_BLK(802B4080);
            v = 0x3A98;
        }
    }
    ENGINE_BLK(802B408C);
    D_803EE3A8 = v;
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B30F4(s32 carrier) {
    CARRY_KEEP(802B30F4, 802B3130, 802B3150, 802B315C, &D_803EE2E0, D_803EE38C, D_803EE394);
}

REGS(a3)
void func_802B3180(s32 carrier) {
    CARRY_MOVE(802B3180, 802B31BC, 802B31D0, 802B31D8, 802B322C, 802B3234, 802B3260, &D_803EE2E0, &D_803EE38C, &D_803EE390, &D_803EE394, 3, 0x2D4, 0x260,
               (void)0, func_802B3FF0(&D_803EE2E0), func_802B3E40(&D_803EE2E0));
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B40A8(u8 *dst) {
    ENGINE_BLK(802B40A8);
    func_802AC7DC(dst, (u8 *)&D_803EE2E0, (u32 *)&D_803EE38C);
    ENGINE_BLK(802B40C4);
}

/* and back */
void func_802B40D4(u8 *src) {
    ENGINE_BLK(802B40D4);
    func_802AC85C(src, (u8 *)&D_803EE2E0, (u32 *)&D_803EE38C);
    ENGINE_BLK(802B40F0);
}

/* ---- Ramdozer ------------------------------------------------------------- */

/* Ramdozer's .bss */
extern Part D_803EE3C0[32];
extern VS D_803EE6C0;
extern s32 D_803EE768, D_803EE76C, D_803EE770;  /* x, y, z */
extern u8 *PTR32 D_803EE774;                    /* its model file */
extern u8 *PTR32 D_803EE778;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EE77C;
extern f32 D_803EE780;                          /* part 2's tilt, 0..1 */
extern u16 D_803EE784;                          /* the turn's rate (func_802A7FD8) */
extern u16 D_803EE786;                          /* the heading it turns to (with D_803A7425) */
extern u8 D_803EE788;                           /* frames until the next sparks */
extern s8 D_803EE789;                           /* turning to D_803EE786 */
extern u8 D_803EE78A;                           /* frames without the gears after a hit */

extern s8 D_80370C2C;                           /* the stick's x */
extern u8 D_80305D10[];                         /* Ramdozer's bounce records */
extern u8 D_802C2190[], D_802C21A4[];           /* its tracks' part records (56040's list) */
extern u8 D_802C21B8[];                         /* part 3's */

void func_802B49AC(void);
static void ramdozer_frame(void);
REGS()
void func_802B47D4(void);
REGS(gp)
void func_802B4EF8(VS *vs);
REGS(gp)
void func_802B54EC(VS *vs);
REGS(gp)
void func_802B568C(VS *vs);
REGS(gp -> s3)
s32 func_802B57C4(VS *vs);
REGS(gp)
void func_802B5814(VS *vs);

#define RAM D_803EE3C0

/* part i's state: func_802A04BC's first result */
static s32 part_state(s32 i, Part *parts) {
    s32 f11, f12, f14, fC, fE, f13;
    f32 f4;

    return func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &f13, &f4);
}

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802B4100(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803EE6C0;
    u8 *buf;
    s16 *r;
    s32 avg;

    ENGINE_BLK(802B4100);
    D_803EE774 = model;
    buf = D_80358070;
    D_803EE778 = buf;
    D_803EE77C = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(4, 0, D_803EE778, D_803EE77C, model);
    ENGINE_BLK(802B4180);
    func_802A754C(vs);
    ENGINE_BLK(802B418C);
    vs->unk52[0] = 0xC8;
    vs->unk52[1] = 0x12C;
    vs->unk52[2] = -0xC8;
    vs->unk52[3] = 0x12C;
    vs->unk52[4] = 0xC8;
    vs->unk52[5] = -0x12C;
    vs->unk5E[0] = 0x12C;
    vs->unk5E[1] = 0x1C2;
    vs->unk5E[2] = -0x12C;
    vs->unk5E[3] = 0x1C2;
    vs->unk5E[4] = 0x12C;
    vs->unk5E[5] = -0x1C2;
    D_803EE768 = x;
    D_803EE76C = y;
    D_803EE770 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    func_802A992C(vs->unk52, D_803EE76C, x, z, vs->unk4, &D_803EE76C, (s16 *)&vs->unk4C, 4, vs, 0,
                       &avg);
    ENGINE_BLK(802B423C);
    func_8029F85C(RAM, D_803EE774, D_803EE778, D_803EE77C);
    ENGINE_BLK(802B4278);
    func_802A039C(0, 100, RAM);
    ENGINE_BLK(802B428C);
    func_802A03D4(0, 0, RAM);
    ENGINE_BLK(802B42A0);
    func_802A040C(0, 0, RAM);
    ENGINE_BLK(802B42B4);
    func_802A0480(0, 0, RAM, 0.0f);
    ENGINE_BLK(802B42CC);
    func_802A0290(0, 1, RAM);
    ENGINE_BLK(802B42E0);
    func_8029E558(RAM, D_803EE778, D_803EE77C);
    ENGINE_BLK(802B42F4);
    func_802A0320(0, RAM);
    ENGINE_BLK(802B4304);
    func_802A0290(0, 1, RAM);
    ENGINE_BLK(802B4318);
    func_8029E558(RAM, D_803EE77C, D_803EE778);
    ENGINE_BLK(802B432C);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 6;
    r[3] = 0, r[4] = 0x50, r[5] = 6;
    r[6] = 0x50, r[7] = 0x8C, r[8] = 4;
    r[9] = 0x8C, r[10] = 0xBE, r[11] = 2;
    r[12] = 0xBE, r[13] = 0xFA, r[14] = 2;
    func_802A6F00(vs);
    ENGINE_BLK(802B43A8);
    D_803EE789 = 0;
    D_803EE78A = 0;
    D_803EE788 = 0;
    D_803F7840 = 0;
    D_803EE780 = 0.5f;
    model = D_803EE774;
    func_8029C354(4, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x2710);
    ENGINE_BLK(802B4408);
    func_80258230(4, 0x96, 0x32, 0x32);
    ENGINE_BLK(802B4420);
    vs->unk9A = 1;
    ramdozer_frame();
    ENGINE_BLK(802B4430);
    vs->unk9A = 0;
    model = D_803EE774;
    func_802AA838(D_803EE77C, D_803EE778, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802B4468);
}

/* hd.c's: the player gets in: its parts back as they start */
void func_802B448C(void) {
    VS *vs = &D_803EE6C0;
    Part *p = RAM;

    ENGINE_BLK(802B448C);
    vs->unk96[3] = 0;
    func_802A03D4(1, 0, p);
    ENGINE_BLK(802B44B4);
    func_802A05D0(D_802C2190, 0);
    ENGINE_BLK(802B44C4);
    func_802A05F8(D_802C2190, 0);
    ENGINE_BLK(802B44D4);
    func_802A0620(D_802C2190, 0);
    ENGINE_BLK(802B44E4);
    func_802A0508(D_802C2190, -1);
    ENGINE_BLK(802B44F4);
    func_802A05D0(D_802C21A4, 0);
    ENGINE_BLK(802B4504);
    func_802A05F8(D_802C21A4, 0);
    ENGINE_BLK(802B4514);
    func_802A0620(D_802C21A4, 0);
    ENGINE_BLK(802B4524);
    func_802A0508(D_802C21A4, -1);
    ENGINE_BLK(802B4534);
    func_802A05D0(D_802C21B8, 0);
    ENGINE_BLK(802B4544);
    func_802A05F8(D_802C21B8, 0);
    ENGINE_BLK(802B4554);
    func_802A0620(D_802C21B8, 0);
    ENGINE_BLK(802B4564);
    func_802A0508(D_802C21B8, -1);
    ENGINE_BLK(802B4574);
    func_802A039C(2, 0, p);
    ENGINE_BLK(802B4588);
    func_802A03D4(2, 0, p);
    ENGINE_BLK(802B459C);
    func_802A040C(2, 1, p);
    ENGINE_BLK(802B45B0);
    func_802A0290(2, -1, p);
    ENGINE_BLK(802B45C4);
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xB);
    ENGINE_BLK(802B45EC);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B45FC(void) {
    VS *vs = &D_803EE6C0;
    u8 r = 0;

    ENGINE_BLK(802B45FC);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802B4620);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802B4630);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802B4640);
                r = 1;
            }
        }
    }
    ENGINE_BLK(802B4644);
    return r;
}

/* hd.c's: the player gets out: the tracks stopped */
void func_802B4658(void) {
    VS *vs = &D_803EE6C0;

    ENGINE_BLK(802B4658);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EE778, (u32 *)D_803EE77C, 0x800);
    ENGINE_BLK(802B4688);
    func_802C444C();
    ENGINE_BLK(802B4690);
    func_802A05D0(D_802C2190, 0);
    ENGINE_BLK(802B46A0);
    func_802A05D0(D_802C21A4, 0);
    ENGINE_BLK(802B46B0);
}

/* hd.c's: put back on the ground where it is */
void func_802B46C4(void) {
    VS *vs = &D_803EE6C0;

    ENGINE_BLK(802B46C4);
    func_802A9A60(vs->unk52, D_803EE76C, D_803EE768, D_803EE770, vs->unk4, &D_803EE76C, (s16 *)&vs->unk4C, 4, vs,
                  0);
    ENGINE_BLK(802B4750);
    func_802B568C(vs);
    ENGINE_BLK(802B4758);
    func_802A133C(D_803EE768, D_803EE76C, D_803EE770, 4, vs);
    ENGINE_BLK(802B4784);
}

/* its light */
REGS()
void func_802B47D4(void) {
    ENGINE_BLK(802B47D4);
    func_802ABD54(4, D_803EE768, D_803EE76C, D_803EE770);
    ENGINE_BLK(802B4808);
}

/* each frame */
static void ramdozer_frame(void) {
    VS *vs = &D_803EE6C0;
    s32 t3 = 0, x, z, rate_i, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_BLK(802B49AC);
    /* (its $fp as it found it: the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802B47D4();
    ENGINE_BLK(802B4A00);
    func_802A75DC((u8 *)RAM, &D_803EE768, &D_803EE76C, &D_803EE770, (u8 *)vs);
    ENGINE_BLK(802B4A2C);
    func_802C4724(0x8F);
    ENGINE_BLK(802B4A34);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802B4A40);
        func_802B4EF8(vs);
    }
    ENGINE_BLK(802B4A48);
    if (D_80367BFF != 0) {
        ENGINE_BLK(802B4A58);
        func_802CB690(vs);
    }
    ENGINE_BLK(802B4A60);
    D_803F7840 = vs->unk76;
    func_802B5814(vs);
    ENGINE_BLK(802B4A78);
    rate_i = func_802B57C4(vs);
    ENGINE_BLK(802B4A80);
    func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    ENGINE_BLK(802B4A9C);
    if (D_803EE78A == 0) {
        ENGINE_BLK(802B4AAC);
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x14, vs, &t3);
        ENGINE_BLK(802B4AB4);
    } else {
        ENGINE_BLK(802B4ABC);
        D_803EE78A--;
    }
    ENGINE_BLK(802B4AC8);
    func_802A7FD8(D_803EE784, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    ENGINE_BLK(802B4AE8);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802B4AF4);
    func_802A843C(&vs->unk76, 1, 4, (s8 *)vs->unk96, vs->unk4, 600.0f, vs);
    ENGINE_BLK(802B4B08);
    if (D_803EE789 != 0) {
        ENGINE_BLK(802B4B18);
        func_802A7070((s16 *)&D_803EE786, vs);
    }
    ENGINE_BLK(802B4B24);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EE768, &D_803EE770, rate, &z);
    ENGINE_BLK(802B4B3C);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EE768, &D_803EE770, &D_803EE76C, 4, 0x258, 0x190, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802B4B74);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B4B9C);
        func_8029E558(RAM, D_803EE778, D_803EE77C);
        ENGINE_BLK(802B4BB0);
    } else {
        ENGINE_BLK(802B4BB8);
        func_8029E558(RAM, D_803EE77C, D_803EE778);
    }
    ENGINE_BLK(802B4BCC);
    func_802B568C(vs);
    ENGINE_BLK(802B4BD4);
    func_8029A800(D_803EE768, D_803EE76C, D_803EE770, D_80305D10, 0, 1, 8, vs->unk76, 0x78, 0, 4, vs);
    ENGINE_BLK(802B4C20);
    func_8029C52C(4, vs);
    ENGINE_BLK(802B4C28);
    func_8029AA10();
    ENGINE_BLK(802B4C30);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802B4C40);
        D_803A7424 = 0;
        D_803F77D0 = RAM;
        func_802BE77C(4, vs);
        ENGINE_BLK(802B4C60);
        if (D_803A7424 == 0) {
            ENGINE_BLK(802B4C70);
            ENGINE_BLK(802B4E20);
            D_803EE789 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    ENGINE_BLK(802B4C78);
    func_8029A914(vs);
    ENGINE_BLK(802B4C80);
    D_803EE789 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802B4C90);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802B4CA0);
        h += 0xFFF;
    }
    ENGINE_BLK(802B4CA4);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802B4CB0);
        h = -h;
    }
    ENGINE_BLK(802B4CB4);
    if (!(h < 0x801)) {
        ENGINE_BLK(802B4CC0);
    }
    ENGINE_BLK(802B4CC8);
    ENGINE_BLK(802B4DBC);
    func_802A70D8(vs);
    ENGINE_BLK(802B4DC4);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.16f, vs, &a1);

        ENGINE_BLK(802B4DD8);
        D_803EE786 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802B4DEC);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802B4DF4);
    func_802BCC10();
    ENGINE_BLK(802B4DFC);
    D_803F77D0 = RAM;
    func_802BE77C(4, vs);
    ENGINE_BLK(802B4E18);
    goto done;

hit:
    /* pushed back by something: back to where it was, the speed turned
       round (between 0x3E and 0x7D forward, 0x2D and 0x5A back) */
    ENGINE_BLK(802B4CDC);
    D_803EE789 = 0;
    if (D_8035805C != 0) {
        ENGINE_BLK(802B4CF4);
        a2 = D_803EE77C;
        a3 = D_803EE778;
    } else {
        ENGINE_BLK(802B4D10);
        a2 = D_803EE778;
        a3 = D_803EE77C;
    }
    ENGINE_BLK(802B4D28);
    func_802A768C((u8 *)RAM, &D_803EE768, &D_803EE76C, &D_803EE770, (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    ENGINE_BLK(802B4D50);
    D_803EE78A = 5;
    v = vs->unk76;
    if (v >= 0) {
        ENGINE_BLK(802B4D68);
        if (v < 0x3E) {
            ENGINE_BLK(802B4D80);
            v = 0x3E;
        } else {
            ENGINE_BLK(802B4D70);
            if (!(v < 0x7E)) {
                ENGINE_BLK(802B4D78);
                v = 0x7D;
            }
        }
    } else {
        ENGINE_BLK(802B4D88);
        if (!(v < -0x2C)) {
            ENGINE_BLK(802B4DA4);
            v = -0x2D;
        } else {
            ENGINE_BLK(802B4D94);
            if (v < -0x5A) {
                ENGINE_BLK(802B4D9C);
                v = -0x5A;
            }
        }
    }
    ENGINE_BLK(802B4DA8);
    vs->unk76 = -v;
    func_802B568C(vs);
    ENGINE_BLK(802B4DB4);

done:
    ENGINE_BLK(802B4E28);
    D_803643E0 = D_803EE768;
    D_803643E4 = D_803EE76C;
    D_803643E8 = D_803EE770;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 4, vs);
    ENGINE_BLK(802B4EA4);
    engine_restore();
}

/* hd.c's: the Ramdozer each frame */
void func_802B49AC(void) {
    ramdozer_frame();
}

/* the parts: dust, sparks off rough ground and when it starts off, part 2's
   tilt, the tracks (with the speed, or turning on the spot with the
   stick), part 3 from the heading, and part 1 while turning to
   D_803EE786 */
REGS(gp)
void func_802B4EF8(VS *vs) {
    Part *p = RAM;
    s32 s, t6;
    f32 f0;

    ENGINE_BLK(802B4EF8);
    func_802B54EC(vs);
    ENGINE_BLK(802B4F08);
    if (D_803EE788 != 0) {
        ENGINE_BLK(802B4F1C);
        D_803EE788--;
        goto start;
    }
    ENGINE_BLK(802B4F28);
    if (vs->unk96[3] == 0)
        goto start;
    ENGINE_BLK(802B4F38);
    D_803EE788 = 1;
    s = func_802A5ED0();
    ENGINE_BLK(802B4F4C);
    if (!(s < 0xE))
        goto start;
    ENGINE_BLK(802B4F5C);
    func_802A6274(T(D_802C2954), 0x30D40, 1, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
start:
    /* starting off: a sound and sparks */
    ENGINE_BLK(802B4F88);
    if (D_803F7840 != 0)
        goto tilt;
    ENGINE_BLK(802B4F9C);
    if (vs->unk76 == 0)
        goto tilt;
    ENGINE_BLK(802B4FA8);
    func_80260650(D_80367738, 0xA, NULL);
    ENGINE_BLK(802B503C);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
tilt:
    /* part 2's tilt: down with D_80370C23, up with D_80370C1C, else back to
       the middle; to the middle while func_802A7CB0 says so */
    ENGINE_BLK(802B50E8);
    f0 = D_803EE780;
    if (func_802A7CB0(0x1E, vs) != 0) {
        ENGINE_BLK(802B50FC);
        goto middle;
    }
    ENGINE_BLK(802B50FC);
    ENGINE_BLK(802B5104);
    if (D_80370C23 != 0) {
        ENGINE_BLK(802B5118);
        f0 -= 0.05f;
        if (f0 < 0.0f) {
            ENGINE_BLK(802B5134);
            f0 = 0.0f;
        }
        goto set;
    }
    ENGINE_BLK(802B513C);
    if (D_80370C1C == 0)
        goto middle;
    ENGINE_BLK(802B5150);
    f0 += 0.05f;
    if (!(f0 <= 1.0f)) {
        ENGINE_BLK(802B5170);
        f0 = 1.0f;
    }
    goto set;
middle:
    ENGINE_BLK(802B5178);
    if (f0 < 0.5f) {
        ENGINE_BLK(802B5198);
        f0 += 0.03f;
        if (!(f0 <= 0.5f)) {
            ENGINE_BLK(802B51A8);
            f0 = 0.5f;
        }
    } else {
        ENGINE_BLK(802B51B0);
        f0 -= 0.03f;
        if (f0 < 0.5f) {
            ENGINE_BLK(802B51C0);
            f0 = 0.5f;
        }
    }
set:
    ENGINE_BLK(802B51C4);
    D_803EE780 = f0;
    func_802A0360(2, 0, p, f0);
    /* the tracks */
    ENGINE_BLK(802B51E8);
    s = vs->unk76;
    if (s != 0) {
        ENGINE_BLK(802B51F4);
        if (s < 0) {
            ENGINE_BLK(802B51FC);
            func_802A05F8(D_802C2190, 1);
            ENGINE_BLK(802B520C);
            func_802A05F8(D_802C21A4, 1);
            ENGINE_BLK(802B521C);
        } else {
            ENGINE_BLK(802B5224);
            func_802A05F8(D_802C2190, 0);
            ENGINE_BLK(802B5234);
            func_802A05F8(D_802C21A4, 0);
        }
        ENGINE_BLK(802B5244);
        if (s < 0) {
            ENGINE_BLK(802B524C);
            s = -s;
        }
        ENGINE_BLK(802B5250);
        s = (u32)s >> 1;
        func_802A05D0(D_802C2190, s);
        ENGINE_BLK(802B5264);
        func_802A05D0(D_802C21A4, s);
        ENGINE_BLK(802B5274);
        func_802C4584((u32)s >> 2);
        ENGINE_BLK(802B527C);
    } else {
        /* standing: turning on the spot with the stick */
        ENGINE_BLK(802B5284);
        t6 = D_80370C2C;
        if (t6 >= 0) {
            ENGINE_BLK(802B5294);
            func_802A05F8(D_802C2190, 0);
            ENGINE_BLK(802B52A4);
            func_802A05F8(D_802C21A4, 1);
            ENGINE_BLK(802B52B4);
        } else {
            ENGINE_BLK(802B52BC);
            func_802A05F8(D_802C2190, 1);
            ENGINE_BLK(802B52CC);
            func_802A05F8(D_802C21A4, 0);
        }
        ENGINE_BLK(802B52DC);
        if (t6 < 0) {
            ENGINE_BLK(802B52E4);
            t6 = -t6;
        }
        ENGINE_BLK(802B52E8);
        func_802A05D0(D_802C2190, t6);
        ENGINE_BLK(802B52F8);
        func_802A05D0(D_802C21A4, t6);
    }
    /* part 3 from the heading */
    ENGINE_BLK(802B5308);
    s = (u16)vs->unk4C;
    if (s < 0x355) {
        ENGINE_BLK(802B5328);
        s = 0;
    } else {
        ENGINE_BLK(802B5318);
        if (s < 0x8AB) {
            ENGINE_BLK(802B5320);
            s -= 0x355;
        } else {
            ENGINE_BLK(802B5328);
            s = 0;
        }
    }
    ENGINE_BLK(802B532C);
    s = (u32)s / 0x4C;
    ENGINE_BLK(802B534C);
    func_802A05A4(D_802C21B8, s, 0.0f);
    ENGINE_BLK(802B535C);
    if (D_803EE789 == 0)
        goto done;
    ENGINE_BLK(802B536C);
    if (part_state(1, p) == 1) {
        ENGINE_BLK(802B537C);
        goto done;
    }
    ENGINE_BLK(802B537C);
    ENGINE_BLK(802B5388);
    func_802A039C(1, 9, p);
    ENGINE_BLK(802B539C);
    func_802A040C(1, 1, p);
    ENGINE_BLK(802B53B0);
    func_802A0290(1, 2, p);
    ENGINE_BLK(802B53C4);
    func_80260650(D_80367738, 1, NULL);
    ENGINE_BLK(802B5458);
done:
    ENGINE_BLK(802B54D8);
}

/* dust behind it on the ground */
REGS(gp)
void func_802B54EC(VS *vs) {
    ENGINE_BLK(802B54EC);
    if (vs->unk96[3] == 0)
        goto done;
    ENGINE_BLK(802B557C);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802B558C);
    if (!(vs->unk50 < 3))
        goto done;
    ENGINE_BLK(802B559C);
    if (vs->unk9B != 0)
        goto done;
    ENGINE_BLK(802B55A8);
    func_8027BE7C(3, vs->unk4[6], 0x190, -0x12C, -0x190, -0x12C, D_803EE768, D_803EE770, vs->unk4E, 5, 0x32, 0x32, 0);
    ENGINE_BLK(802B5604);
done:
    ENGINE_BLK(802B5608);
}

/* Ramdozer's matrix, its vertices and its collision */
REGS(gp)
void func_802B568C(VS *vs) {
    u8 *model = D_803EE774, *buf;
    s32 *m, off;

    ENGINE_BLK(802B568C);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B56BC);
        m = (s32 *)(D_803EE778 + off);
    } else {
        ENGINE_BLK(802B56D0);
        m = (s32 *)(D_803EE77C + off);
    }
    ENGINE_BLK(802B56E0);
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EE768, D_803EE76C, D_803EE770, 0x2710, m);
    ENGINE_BLK(802B571C);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B5730);
        buf = D_803EE778;
    } else {
        ENGINE_BLK(802B5740);
        buf = D_803EE77C;
    }
    ENGINE_BLK(802B574C);
    model = D_803EE774;
    func_8029C454(D_803EE768, D_803EE76C, D_803EE770, 4, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802B5794);
    func_802ABBEC(4, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802B57B4);
}

/* the turn rate: 0x4B, or 0x16 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B57C4(VS *vs) {
    s32 r;

    ENGINE_BLK(802B57C4);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802B57D8);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802B57E8);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802B57F8);
    r = 0x4B;
    goto done;
air:
    ENGINE_BLK(802B5800);
    r = 0x16;
done:
    ENGINE_BLK(802B5804);
    return r;
}

/* the camera's distance and speed for Ramdozer, and the turn's rate: 0x7D0
   with D_80370C23 held, else 0x2328 */
REGS(gp)
void func_802B5814(VS *vs) {
    s32 v;

    ENGINE_BLK(802B5814);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x6E;
    D_803ED3F7 = 4;
    if (D_80370C23 != 0) {
        ENGINE_BLK(802B587C);
        v = 0x7D0;
    } else {
        ENGINE_BLK(802B5874);
        v = 0x2328;
    }
    ENGINE_BLK(802B5880);
    D_803EE784 = v;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B589C(u8 *dst) {
    ENGINE_BLK(802B589C);
    func_802AC7DC(dst, (u8 *)&D_803EE6C0, (u32 *)&D_803EE768);
    ENGINE_BLK(802B58B8);
}

/* and back */
void func_802B58C8(u8 *src) {
    ENGINE_BLK(802B58C8);
    func_802AC85C(src, (u8 *)&D_803EE6C0, (u32 *)&D_803EE768);
    ENGINE_BLK(802B58E4);
}

/* 62740's carrying (shared.h): where it stands on its carrier, and back
   there after the carrier moved */
REGS(a3)
void func_802B4818(s32 carrier) {
    CARRY_KEEP(802B4818, 802B4854, 802B4874, 802B4880, &D_803EE6C0, D_803EE768, D_803EE770);
}

REGS(a3)
void func_802B48A4(s32 carrier) {
    CARRY_MOVE(802B48A4, 802B48E0, 802B48F4, 802B48FC, 802B495C, 802B4964, 802B4990, &D_803EE6C0, &D_803EE768, &D_803EE76C, &D_803EE770, 4, 0x258, 0x190,
               D_803ED40B = 1, func_802B5814(&D_803EE6C0), func_802B568C(&D_803EE6C0));
}
