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

    ENGINE_COST(802B29C0, 239);
    D_803EE398 = model;
    buf = D_80358070;
    D_803EE39C = buf;
    D_803EE3A0 = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(3, 0, D_803EE39C, D_803EE3A0, model);
    func_802A754C(vs);
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
    func_8029F85C(SKY, D_803EE398, D_803EE39C, D_803EE3A0);
    func_802A039C(0, 100, SKY);
    func_802A03D4(0, 0, SKY);
    func_802A040C(0, 0, SKY);
    func_802A0480(0, 0, SKY, 0.0f);
    func_802A0290(0, 1, SKY);
    func_8029E558(SKY, D_803EE39C, D_803EE3A0);
    func_802A0320(0, SKY);
    func_802A0290(0, 1, SKY);
    func_8029E558(SKY, D_803EE3A0, D_803EE39C);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 5, r[2] = 2;
    r[3] = 0, r[4] = 0x78, r[5] = 4;
    r[6] = 0x78, r[7] = 0xA0, r[8] = 5;
    r[9] = 0xA0, r[10] = 0xDC, r[11] = 2;
    r[12] = 0xDC, r[13] = 0xFA, r[14] = 2;
    func_802A6F00(vs);
    D_803EE3B1 = 100;
    D_803EE3B2 = 0;
    D_803EE3AE = 0;
    D_803EE3B0 = 0;
    D_803EE388 = NULL;
    model = D_803EE398;
    func_8029C354(3, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x2AF8);
    func_80258230(3, 0x50, 0x1F, 0x1F);
    vs->unk9A = 1;
    skyfall_frame();
    vs->unk9A = 0;
    model = D_803EE398;
    func_802AA838(D_803EE3A0, D_803EE39C, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    D_80364A69 = 1;
}

/* hd.c's: the player gets in: the doors shut, the flame out */
void func_802B2D7C(void) {
    VS *vs = &D_803EE2E0;
    Part *p = SKY;

    ENGINE_COST(802B2D7C, 95);
    vs->unk96[3] = 0;
    func_802A039C(1, 0, p);
    func_802A03D4(1, 0, p);
    func_802A040C(1, 0, p);
    func_802A0290(1, -1, p);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 1, p);
    func_802A0290(2, -1, p);
    func_802A039C(3, 0, p);
    func_802A03D4(3, 0, p);
    func_802A040C(3, 1, p);
    func_802A0290(3, -1, p);
    func_802A05D0(D_802C2314, 0);
    func_802A05F8(D_802C2314, 0);
    func_802A0620(D_802C2314, 0);
    func_802A0508(D_802C2314, -1);
    D_8036444C = 0xD48;
    D_80364450 = 0x3E8;
    func_802C4310(0x8C);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B2EF8(void) {
    VS *vs = &D_803EE2E0;
    u8 r = 0;

    ENGINE_COST(802B2EF8, 20);
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
void func_802B2F54(void) {
    VS *vs = &D_803EE2E0;

    ENGINE_COST(802B2F54, 19);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EE39C, (u32 *)D_803EE3A0, 0x800);
    func_802C444C();
}

/* hd.c's: put back on the ground where it is */
void func_802B2FA0(void) {
    VS *vs = &D_803EE2E0;

    ENGINE_COST(802B2FA0, 35);
    func_802A9A60(vs->unk52, D_803EE390, D_803EE38C, D_803EE394, vs->unk4, &D_803EE390, (s16 *)&vs->unk4C, 3, vs,
                  0);
    func_802B3E40(vs);
    func_802A133C(D_803EE38C, D_803EE390, D_803EE394, 3, vs);
}

/* its light */
REGS()
void func_802B30B0(void) {
    ENGINE_COST(802B30B0, 17);
    func_802ABD54(3, D_803EE38C, D_803EE390, D_803EE394);
}

/* each frame */
static void skyfall_frame(void) {
    VS *vs = &D_803EE2E0;
    s32 t3 = 0, x, z, rate_i, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_COST(802B327C, 222);
    /* (its $fp as it found it: the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802B30B0();
    func_802A75DC((u8 *)SKY, &D_803EE38C, &D_803EE390, &D_803EE394, (u8 *)vs);
    if (vs->unk9A == 0) {
        func_802B37B0(vs);
    }
    if (D_80367BFF != 0) {
        func_802CB690(vs);
    }
    func_802B3FF0(vs);
    rate_i = func_802B3F78(vs);
    func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    if (D_803EE3B2 == 0) {
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x14, vs, &t3);
    } else {
        D_803EE3B2--;
    }
    func_802A7FD8(D_803EE3A8, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 1, 3, (s8 *)vs->unk96, vs->unk4, 724.0f, vs);
    if (D_803EE3B0 != 0) {
        func_802A7070((s16 *)&D_803EE3AA, vs);
    }
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EE38C, &D_803EE394, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EE38C, &D_803EE394, &D_803EE390, 3, 0x2D4, 0x260, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    if (D_8035805C != 0) {
        func_8029E558(SKY, D_803EE39C, D_803EE3A0);
    } else {
        func_8029E558(SKY, D_803EE3A0, D_803EE39C);
    }
    func_802B3E40(vs);
    ENGINE_LEAVE(8, 6);         /* $t0 and $t2: func_8029A800 (56040.c) takes them from the context */
    ENGINE_LEAVE(10, 0x64);
    func_8029A800(D_803EE38C, D_803EE390, D_803EE394, D_80305D00, 1, 1, vs->unk76, 0, 3, vs);
    func_8029C52C(3, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = SKY;
        func_802BE77C(3, vs);
        if (D_803A7424 == 0) {
            D_803EE3B0 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    func_8029A914(vs);
    D_803EE3B0 = 1;
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
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.1f, vs, &a1);

        D_803EE3AA = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    func_802A6FE4(0, vs);
    D_803F77D0 = SKY;
    func_802BE77C(3, vs);
    goto done;

hit:
    /* bounced off something: back to where it was, the speed turned
       round */
    D_803EE3B0 = 0;
    if (D_8035805C != 0) {
        a2 = D_803EE3A0;
        a3 = D_803EE39C;
    } else {
        a2 = D_803EE39C;
        a3 = D_803EE3A0;
    }
    func_802A768C((u8 *)SKY, &D_803EE38C, &D_803EE390, &D_803EE394, (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    D_803EE3B2 = 5;
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
    func_802B3E40(vs);

done:
    D_803643E0 = D_803EE38C;
    D_803643E4 = D_803EE390;
    D_803643E8 = D_803EE394;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 3, vs);
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

    ENGINE_COST(802B37B0, 151);
    if (D_803EE3AE != 0) {
        D_803EE3AE--;
        goto tilt;
    }
    if (vs->unk96[3] == 0)
        goto tilt;
    D_803EE3AE = 1;
    s = func_802A5ED0();
    if (!(s < 0xF))
        goto tilt;
    func_802A6274(T(D_802C2954), 0x29810, 1, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802A6274(T(D_802C2954), 0x29810, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
tilt:
    /* part 3's tilt: down with D_80370C23, up with D_80370C1C, else back to
       the middle; to the middle while func_802A7CB0 says so */
    f0 = D_803EE3A4;
    if (func_802A7CB0(10, vs) != 0) {
        goto middle;
    }
    if (D_80370C23 != 0) {
        f0 -= 0.05f;
        if (f0 < 0.0f) {
            f0 = 0.0f;
        }
        goto set;
    }
    if (D_80370C1C == 0)
        goto middle;
    f0 += 0.05f;
    if (!(f0 <= 1.0f)) {
        f0 = 1.0f;
    }
    goto set;
middle:
    if (f0 < 0.5f) {
        f0 += 0.03f;
        if (!(f0 <= 0.5f)) {
            f0 = 0.5f;
        }
    } else {
        f0 -= 0.03f;
        if (f0 < 0.5f) {
            f0 = 0.5f;
        }
    }
set:
    D_803EE3A4 = f0;
    func_802A0360(3, 0, p, f0);
    /* part 2: down with D_80370C15, up with D_80370C16, else back to the
       middle */
    t1 = D_803EE3AF;
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
    D_803EE3AF = t1;
    func_802A0360(2, 0, p, (f32)t1 / 100.0f);
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
        s = (u32)s / 30;
    }
    func_802A039C(1, s, p);
    func_802C4584(s);
    /* the boost */
    if (D_803EE3B2 != 0) {
        D_803EE3B2--;
        goto no_boost;
    }
    if (D_80370C1A == 0) {
        if (D_80370C1B == 0)
            goto no_boost;
    }
    s = (s8)D_803EE3B1;  /* (the boost's fuel, 0..100; vehicle.h) */
    if (s < 4)
        goto no_boost;
    D_803EE3B1 = s - 4;
    if (D_803EE3B3 == 0) {
        D_803EE3B3 = 1;
        func_80260650(D_80367738, 0x8D, &D_803EE388);
    }
    func_802A05A4(D_802C2314, 1, 0.0f);
    func_802A6274(T(D_802C37C0), 0x186A0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0);
    s = vs->unk76;
    if (s < 0x190) {
        vs->unk76 = s + 0x28;
    } else {
        vs->unk76 = 0x190;
    }
    goto dust;
no_boost:
    if (D_803EE388 != NULL) {
        func_802608C8(D_803EE388);
        D_803EE388 = NULL;
    }
    D_803EE3AC = 0x78;
    D_803EE3B3 = 0;
    func_802A05A4(D_802C2314, 0, 0.0f);
    s = vs->unk76;
    if (!(s < 0xFB)) {
        s -= 0x19;
        if (s < 0xFA) {
            s = 0xFA;
        }
        vs->unk76 = s;
    }
    s = (s8)D_803EE3B1 + 1;
    if (!(s < 0x65)) {
        s = 0x64;
    }
    D_803EE3B1 = s;
dust:
    func_802B3C68(vs);
}

/* dust behind it on the ground, more of it while boosting */
REGS(gp)
void func_802B3C68(VS *vs) {
    s32 t1;

    ENGINE_COST(802B3C68, 80);
    if (vs->unk96[3] != 0) {
        t1 = 0x32;
    } else {
        if (D_803EE3B3 == 0)
            goto done;
        t1 = D_803EE3AC;
        if (!(t1 < 8)) {
        } else {
            t1 = 8;
        }
        t1 -= 8;
        D_803EE3AC = t1;
    }
    if (vs->unk96[2] == 1)
        goto done;
    if (!(vs->unk50 < 3))
        goto done;
    if (vs->unk9B != 0)
        goto done;
    func_8027BE7C(2, vs->unk4[6], 0x190, -0x12C, -0x190, -0x12C, D_803EE38C, D_803EE394, vs->unk4E, 4, t1, t1, 0);
done:
    ;
}

/* Skyfall's matrix, its vertices and its collision */
REGS(gp)
void func_802B3E40(VS *vs) {
    u8 *model = D_803EE398, *buf;
    s32 *m, off;

    ENGINE_COST(802B3E40, 70);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EE39C + off);
    } else {
        m = (s32 *)(D_803EE3A0 + off);
    }
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EE38C, D_803EE390, D_803EE394, 0x2AF8, m);
    if (D_8035805C != 0) {
        buf = D_803EE39C;
    } else {
        buf = D_803EE3A0;
    }
    model = D_803EE398;
    func_8029C454(D_803EE38C, D_803EE390, D_803EE394, 3, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(3, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* the turn rate: the speed / 2.5, or / 11 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B3F78(VS *vs) {
    f32 d;

    ENGINE_COST(802B3F78, 25);
    if (vs->unk96[0] == 1)
        goto air;
    if (vs->unk96[1] == 1)
        goto air;
    if (vs->unk96[2] == 1)
        goto air;
    d = 2.5f;
    goto div;
air:
    d = 11.0f;
div:
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for Skyfall, and the turn's rate: 0x7D0
   with D_80370C23 held going forward (D_80370C1C going back), else 0x3A98 */
REGS(gp)
void func_802B3FF0(VS *vs) {
    s32 v;

    ENGINE_COST(802B3FF0, 36);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x3C;
    D_803ED3F7 = 3;
    if (!(vs->unk76 > 0)) {
        if (D_80370C1C != 0) {
            v = 0x7D0;
        } else {
            v = 0x3A98;
        }
    } else {
        if (D_80370C23 != 0) {
            v = 0x7D0;
        } else {
            v = 0x3A98;
        }
    }
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
    ENGINE_COST(802B40A8, 7);
    func_802AC7DC(dst, (u8 *)&D_803EE2E0, (u32 *)&D_803EE38C);
}

/* and back */
void func_802B40D4(u8 *src) {
    ENGINE_COST(802B40D4, 11);
    func_802AC85C(src, (u8 *)&D_803EE2E0, (u32 *)&D_803EE38C);
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

    ENGINE_COST(802B4100, 227);
    D_803EE774 = model;
    buf = D_80358070;
    D_803EE778 = buf;
    D_803EE77C = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(4, 0, D_803EE778, D_803EE77C, model);
    func_802A754C(vs);
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
    func_8029F85C(RAM, D_803EE774, D_803EE778, D_803EE77C);
    func_802A039C(0, 100, RAM);
    func_802A03D4(0, 0, RAM);
    func_802A040C(0, 0, RAM);
    func_802A0480(0, 0, RAM, 0.0f);
    func_802A0290(0, 1, RAM);
    func_8029E558(RAM, D_803EE778, D_803EE77C);
    func_802A0320(0, RAM);
    func_802A0290(0, 1, RAM);
    func_8029E558(RAM, D_803EE77C, D_803EE778);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 6;
    r[3] = 0, r[4] = 0x50, r[5] = 6;
    r[6] = 0x50, r[7] = 0x8C, r[8] = 4;
    r[9] = 0x8C, r[10] = 0xBE, r[11] = 2;
    r[12] = 0xBE, r[13] = 0xFA, r[14] = 2;
    func_802A6F00(vs);
    D_803EE789 = 0;
    D_803EE78A = 0;
    D_803EE788 = 0;
    D_803F7840 = 0;
    D_803EE780 = 0.5f;
    model = D_803EE774;
    func_8029C354(4, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x2710);
    func_80258230(4, 0x96, 0x32, 0x32);
    vs->unk9A = 1;
    ramdozer_frame();
    vs->unk9A = 0;
    model = D_803EE774;
    func_802AA838(D_803EE77C, D_803EE778, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
}

/* hd.c's: the player gets in: its parts back as they start */
void func_802B448C(void) {
    VS *vs = &D_803EE6C0;
    Part *p = RAM;

    ENGINE_COST(802B448C, 92);
    vs->unk96[3] = 0;
    func_802A03D4(1, 0, p);
    func_802A05D0(D_802C2190, 0);
    func_802A05F8(D_802C2190, 0);
    func_802A0620(D_802C2190, 0);
    func_802A0508(D_802C2190, -1);
    func_802A05D0(D_802C21A4, 0);
    func_802A05F8(D_802C21A4, 0);
    func_802A0620(D_802C21A4, 0);
    func_802A0508(D_802C21A4, -1);
    func_802A05D0(D_802C21B8, 0);
    func_802A05F8(D_802C21B8, 0);
    func_802A0620(D_802C21B8, 0);
    func_802A0508(D_802C21B8, -1);
    func_802A039C(2, 0, p);
    func_802A03D4(2, 0, p);
    func_802A040C(2, 1, p);
    func_802A0290(2, -1, p);
    D_8036444C = 0xBB8;
    D_80364450 = 0x3E8;
    func_802C4310(0xB);
}

/* hd.c's: whether it can be left: not while a wheel is off the ground */
u8 func_802B45FC(void) {
    VS *vs = &D_803EE6C0;
    u8 r = 0;

    ENGINE_COST(802B45FC, 23);
    if (vs->unk96[0] != 1) {
        if (vs->unk96[1] != 1) {
            if (vs->unk96[2] != 1) {
                r = 1;
            }
        }
    }
    return r;
}

/* hd.c's: the player gets out: the tracks stopped */
void func_802B4658(void) {
    VS *vs = &D_803EE6C0;

    ENGINE_COST(802B4658, 27);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EE778, (u32 *)D_803EE77C, 0x800);
    func_802C444C();
    func_802A05D0(D_802C2190, 0);
    func_802A05D0(D_802C21A4, 0);
}

/* hd.c's: put back on the ground where it is */
void func_802B46C4(void) {
    VS *vs = &D_803EE6C0;

    ENGINE_COST(802B46C4, 35);
    func_802A9A60(vs->unk52, D_803EE76C, D_803EE768, D_803EE770, vs->unk4, &D_803EE76C, (s16 *)&vs->unk4C, 4, vs,
                  0);
    func_802B568C(vs);
    func_802A133C(D_803EE768, D_803EE76C, D_803EE770, 4, vs);
}

/* its light */
REGS()
void func_802B47D4(void) {
    ENGINE_COST(802B47D4, 17);
    func_802ABD54(4, D_803EE768, D_803EE76C, D_803EE770);
}

/* each frame */
static void ramdozer_frame(void) {
    VS *vs = &D_803EE6C0;
    s32 t3 = 0, x, z, rate_i, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;
    u8 *a2, *a3;

    ENGINE_COST(802B49AC, 220);
    /* (its $fp as it found it: the other vehicles read it from the context) */
    engine_save(ENGINE_GPR(30), 0);
    func_802B47D4();
    func_802A75DC((u8 *)RAM, &D_803EE768, &D_803EE76C, &D_803EE770, (u8 *)vs);
    func_802C4724(0x8F);
    if (vs->unk9A == 0) {
        func_802B4EF8(vs);
    }
    if (D_80367BFF != 0) {
        func_802CB690(vs);
    }
    D_803F7840 = vs->unk76;
    func_802B5814(vs);
    rate_i = func_802B57C4(vs);
    func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    if (D_803EE78A == 0) {
        func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x14, vs, &t3);
    } else {
        D_803EE78A--;
    }
    func_802A7FD8(D_803EE784, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 1, 4, (s8 *)vs->unk96, vs->unk4, 600.0f, vs);
    if (D_803EE789 != 0) {
        func_802A7070((s16 *)&D_803EE786, vs);
    }
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EE768, &D_803EE770, rate, &z);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EE768, &D_803EE770, &D_803EE76C, 4, 0x258, 0x190, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    if (D_8035805C != 0) {
        func_8029E558(RAM, D_803EE778, D_803EE77C);
    } else {
        func_8029E558(RAM, D_803EE77C, D_803EE778);
    }
    func_802B568C(vs);
    ENGINE_LEAVE(8, 8);         /* $t0 and $t2: func_8029A800 (56040.c) takes them from the context */
    ENGINE_LEAVE(10, 0x78);
    func_8029A800(D_803EE768, D_803EE76C, D_803EE770, D_80305D10, 0, 1, vs->unk76, 0, 4, vs);
    func_8029C52C(4, vs);
    func_8029AA10();
    if (D_803A7425 == 0) {
        D_803A7424 = 0;
        D_803F77D0 = RAM;
        func_802BE77C(4, vs);
        if (D_803A7424 == 0) {
            D_803EE789 = 0;
            goto done;
        }
        goto hit;
    }
    /* D_803A7425: turned toward the camera's heading */
    func_8029A914(vs);
    D_803EE789 = 1;
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

        D_803EE786 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    func_802A6FE4(0, vs);
    func_802BCC10();
    D_803F77D0 = RAM;
    func_802BE77C(4, vs);
    goto done;

hit:
    /* pushed back by something: back to where it was, the speed turned
       round (between 0x3E and 0x7D forward, 0x2D and 0x5A back) */
    D_803EE789 = 0;
    if (D_8035805C != 0) {
        a2 = D_803EE77C;
        a3 = D_803EE778;
    } else {
        a2 = D_803EE778;
        a3 = D_803EE77C;
    }
    func_802A768C((u8 *)RAM, &D_803EE768, &D_803EE76C, &D_803EE770, (u32 *)a2, (u32 *)a3, 0x800, (u8 *)vs);
    D_803EE78A = 5;
    v = vs->unk76;
    if (v >= 0) {
        if (v < 0x3E) {
            v = 0x3E;
        } else {
            if (!(v < 0x7E)) {
                v = 0x7D;
            }
        }
    } else {
        if (!(v < -0x2C)) {
            v = -0x2D;
        } else {
            if (v < -0x5A) {
                v = -0x5A;
            }
        }
    }
    vs->unk76 = -v;
    func_802B568C(vs);

done:
    D_803643E0 = D_803EE768;
    D_803643E4 = D_803EE76C;
    D_803643E8 = D_803EE770;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 4, vs);
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

    ENGINE_COST(802B4EF8, 109);
    func_802B54EC(vs);
    if (D_803EE788 != 0) {
        D_803EE788--;
        goto start;
    }
    if (vs->unk96[3] == 0)
        goto start;
    D_803EE788 = 1;
    s = func_802A5ED0();
    if (!(s < 0xE))
        goto start;
    func_802A6274(T(D_802C2954), 0x30D40, 1, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
start:
    /* starting off: a sound and sparks */
    if (D_803F7840 != 0)
        goto tilt;
    if (vs->unk76 == 0)
        goto tilt;
    func_80260650(D_80367738, 0xA, NULL);
    func_802A6274(T(D_802C2954), 0x1D4C0, 1, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
tilt:
    /* part 2's tilt: down with D_80370C23, up with D_80370C1C, else back to
       the middle; to the middle while func_802A7CB0 says so */
    f0 = D_803EE780;
    if (func_802A7CB0(0x1E, vs) != 0) {
        goto middle;
    }
    if (D_80370C23 != 0) {
        f0 -= 0.05f;
        if (f0 < 0.0f) {
            f0 = 0.0f;
        }
        goto set;
    }
    if (D_80370C1C == 0)
        goto middle;
    f0 += 0.05f;
    if (!(f0 <= 1.0f)) {
        f0 = 1.0f;
    }
    goto set;
middle:
    if (f0 < 0.5f) {
        f0 += 0.03f;
        if (!(f0 <= 0.5f)) {
            f0 = 0.5f;
        }
    } else {
        f0 -= 0.03f;
        if (f0 < 0.5f) {
            f0 = 0.5f;
        }
    }
set:
    D_803EE780 = f0;
    func_802A0360(2, 0, p, f0);
    /* the tracks */
    s = vs->unk76;
    if (s != 0) {
        if (s < 0) {
            func_802A05F8(D_802C2190, 1);
            func_802A05F8(D_802C21A4, 1);
        } else {
            func_802A05F8(D_802C2190, 0);
            func_802A05F8(D_802C21A4, 0);
        }
        if (s < 0) {
            s = -s;
        }
        s = (u32)s >> 1;
        func_802A05D0(D_802C2190, s);
        func_802A05D0(D_802C21A4, s);
        func_802C4584((u32)s >> 2);
    } else {
        /* standing: turning on the spot with the stick */
        t6 = D_80370C2C;
        if (t6 >= 0) {
            func_802A05F8(D_802C2190, 0);
            func_802A05F8(D_802C21A4, 1);
        } else {
            func_802A05F8(D_802C2190, 1);
            func_802A05F8(D_802C21A4, 0);
        }
        if (t6 < 0) {
            t6 = -t6;
        }
        func_802A05D0(D_802C2190, t6);
        func_802A05D0(D_802C21A4, t6);
    }
    /* part 3 from the heading */
    s = (u16)vs->unk4C;
    if (s < 0x355) {
        s = 0;
    } else {
        if (s < 0x8AB) {
            s -= 0x355;
        } else {
            s = 0;
        }
    }
    s = (u32)s / 0x4C;
    func_802A05A4(D_802C21B8, s, 0.0f);
    if (D_803EE789 == 0)
        goto done;
    if (part_state(1, p) == 1) {
        goto done;
    }
    func_802A039C(1, 9, p);
    func_802A040C(1, 1, p);
    func_802A0290(1, 2, p);
    func_80260650(D_80367738, 1, NULL);
done:
    ;
}

/* dust behind it on the ground */
REGS(gp)
void func_802B54EC(VS *vs) {
    ENGINE_COST(802B54EC, 70);
    if (vs->unk96[3] == 0)
        goto done;
    if (vs->unk96[2] == 1)
        goto done;
    if (!(vs->unk50 < 3))
        goto done;
    if (vs->unk9B != 0)
        goto done;
    func_8027BE7C(3, vs->unk4[6], 0x190, -0x12C, -0x190, -0x12C, D_803EE768, D_803EE770, vs->unk4E, 5, 0x32, 0x32, 0);
done:
    ;
}

/* Ramdozer's matrix, its vertices and its collision */
REGS(gp)
void func_802B568C(VS *vs) {
    u8 *model = D_803EE774, *buf;
    s32 *m, off;

    ENGINE_COST(802B568C, 70);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EE778 + off);
    } else {
        m = (s32 *)(D_803EE77C + off);
    }
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EE768, D_803EE76C, D_803EE770, 0x2710, m);
    if (D_8035805C != 0) {
        buf = D_803EE778;
    } else {
        buf = D_803EE77C;
    }
    model = D_803EE774;
    func_8029C454(D_803EE768, D_803EE76C, D_803EE770, 4, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(4, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* the turn rate: 0x4B, or 0x16 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B57C4(VS *vs) {
    s32 r;

    ENGINE_COST(802B57C4, 19);
    if (vs->unk96[0] == 1)
        goto air;
    if (vs->unk96[1] == 1)
        goto air;
    if (vs->unk96[2] == 1)
        goto air;
    r = 0x4B;
    goto done;
air:
    r = 0x16;
done:
    return r;
}

/* the camera's distance and speed for Ramdozer, and the turn's rate: 0x7D0
   with D_80370C23 held, else 0x2328 */
REGS(gp)
void func_802B5814(VS *vs) {
    s32 v;

    ENGINE_COST(802B5814, 33);
    D_803EBBF4 = D_803EBBF0 * 2.0f;
    D_803ED3F6 = 0x6E;
    D_803ED3F7 = 4;
    if (D_80370C23 != 0) {
        v = 0x7D0;
    } else {
        v = 0x2328;
    }
    D_803EE784 = v;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B589C(u8 *dst) {
    ENGINE_COST(802B589C, 7);
    func_802AC7DC(dst, (u8 *)&D_803EE6C0, (u32 *)&D_803EE768);
}

/* and back */
void func_802B58C8(u8 *src) {
    ENGINE_COST(802B58C8, 11);
    func_802AC85C(src, (u8 *)&D_803EE6C0, (u32 *)&D_803EE768);
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
