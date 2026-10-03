/*
 * hd_code 6B4A0 (us.v11 0x802AFC60-0x802B0D9C): the Sideswipe
 * (VEHICLE_SIDESWIPE), as native C (engine.h).  Its parts are D_803ED840,
 * its state D_803EDB40 and its position D_803EDBE8..F0 (vehicle.h).
 * func_802AFC60 sets it up (from the level loader), func_802B03F4 runs it
 * each frame (from hd.c and at the end of the setup); the others are
 * hd.c's hooks for it.  Part 5 is its pair of side rams (the hydraulics
 * D_803EDC00 counts what is left of them), part 3 their extension.
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/audio.h"

/* the Sideswipe's .bss (asm/data/hd_code/6B4A0.bss.s) */
extern Part D_803ED840[32];
extern VS D_803EDB40;
extern s32 D_803EDBE8, D_803EDBEC, D_803EDBF0;  /* x, y, z */
extern u8 *PTR32 D_803EDBF4;                    /* its model file */
extern u8 *PTR32 D_803EDBF8;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803EDBFC;
extern u16 D_803EDC02;                          /* the heading it turns to (with D_803A7425) */
extern s8 D_803EDC04;                           /* turning to it */
extern u8 D_803EDC05;                           /* the rams' extension, 0..100 */
extern u8 D_803EDC06;                           /* frames until the rams can fire again */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7425;
extern u8 D_80364A6D;
extern u8 D_80370C15, D_80370C16, D_80370C1A, D_80370C1B;
extern Part *PTR32 D_803F77D0;
extern u8 D_80305CE0[];
extern s32 D_803643E4, D_803643E8;
extern u8 D_802E8BE4;
extern s32 D_802E8BE8;
extern u8 D_802C2390[];                         /* the rams' part record (56040's list) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);

void func_802B03F4(void);
void func_802B03B0(void);
REGS(gp)
void func_802B07DC(VS *vs);
void func_802B0AAC(void);
REGS(gp)
void func_802B0B3C(VS *vs);
REGS(gp -> s3)
s32 func_802B0C74(VS *vs);
void func_802B0CE8(void);

#define T(p) ((s32)(p))

/* part i's state: func_802A04BC's first result */
static s32 part_state(s32 i, Part *parts) {
    s32 f11, f12, f14, fC, fE, f13;
    f32 f4;

    return func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &f13, &f4);
}

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802AFC60(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 avg;
    VS *vs = &D_803EDB40;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802AFC60);
    D_803EDBF4 = model;
    buf = D_80358070;
    D_803EDBF8 = buf;
    D_803EDBFC = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(1, 0, D_803EDBF8, D_803EDBFC, model);
    ENGINE_BLK(802AFCE0);
    func_802A754C(vs);
    ENGINE_BLK(802AFCEC);
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
    D_803EDBE8 = x;
    D_803EDBEC = y;
    D_803EDBF0 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    D_803EDC05 = 0x32;
    D_803EDC00 = 0;
    D_803EDC06 = 0;
    D_803EDC04 = 0;
    func_802A992C(vs->unk52, D_803EDBEC, x, z, vs->unk4, &D_803EDBEC, (s16 *)&vs->unk4C, 1, vs, 0, &avg);
    ENGINE_BLK(802AFDB4);
    func_8029F85C(D_803ED840, D_803EDBF4, D_803EDBF8, D_803EDBFC);
    ENGINE_BLK(802AFDF0);
    func_802A039C(0, 100, D_803ED840);
    ENGINE_BLK(802AFE04);
    func_802A03D4(0, 0, D_803ED840);
    ENGINE_BLK(802AFE18);
    func_802A040C(0, 0, D_803ED840);
    ENGINE_BLK(802AFE2C);
    func_802A0480(0, 0, D_803ED840, 0.0f);
    ENGINE_BLK(802AFE44);
    func_802A0290(0, 1, D_803ED840);
    ENGINE_BLK(802AFE58);
    func_8029E558(D_803ED840, D_803EDBF8, D_803EDBFC);
    ENGINE_BLK(802AFE6C);
    func_802A0320(0, D_803ED840);
    ENGINE_BLK(802AFE7C);
    func_802A0290(0, 1, D_803ED840);
    ENGINE_BLK(802AFE90);
    func_8029E558(D_803ED840, D_803EDBFC, D_803EDBF8);
    ENGINE_BLK(802AFEA4);
    r = vs->unk78;
    r[0] = -0xB4, r[1] = 0, r[2] = 5;
    r[3] = 0, r[4] = 0x3C, r[5] = 5;
    r[6] = 0x3C, r[7] = 0x64, r[8] = 4;
    r[9] = 0x64, r[10] = 0xB4, r[11] = 3;
    r[12] = 0xB4, r[13] = 0xDC, r[14] = 2;
    func_8029C354(1, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x1B58);
    ENGINE_BLK(802AFF44);
    func_80258230(1, 0x96, 0x3C, 0x3C);
    ENGINE_BLK(802AFF5C);
    vs->unk9A = 1;
    func_802B03F4();
    ENGINE_BLK(802AFF6C);
    vs->unk9A = 0;
    func_802AA838(D_803EDBFC, D_803EDBF8, *(s32 *)(D_803EDBF4 + *(s32 *)(D_803EDBF4 + 0x18) + 4));
    ENGINE_BLK(802AFFA4);
    D_80364A6D = 1;
}

/* hd.c's (and 17210.c's): the player gets in: the rams back in */
void func_802AFFD4(void) {
    VS *vs = &D_803EDB40;
    Part *p = D_803ED840;

    ENGINE_BLK(802AFFD4);
    vs->unk96[3] = 0;
    func_802A039C(1, 0, p);
    ENGINE_BLK(802AFFFC);
    func_802A03D4(1, 0, p);
    ENGINE_BLK(802B0010);
    func_802A040C(1, 0, p);
    ENGINE_BLK(802B0024);
    func_802A0290(1, -1, p);
    ENGINE_BLK(802B0038);
    func_802A039C(2, 0, p);
    ENGINE_BLK(802B004C);
    func_802A03D4(2, 0, p);
    ENGINE_BLK(802B0060);
    func_802A040C(2, 0, p);
    ENGINE_BLK(802B0074);
    func_802A0290(2, -1, p);
    ENGINE_BLK(802B0088);
    func_802A039C(5, 7, p);
    ENGINE_BLK(802B009C);
    func_802A03D4(5, 0, p);
    ENGINE_BLK(802B00B0);
    func_802A040C(5, 1, p);
    ENGINE_BLK(802B00C4);
    func_802A039C(3, 0, p);
    ENGINE_BLK(802B00D8);
    func_802A03D4(3, 0, p);
    ENGINE_BLK(802B00EC);
    func_802A040C(3, 1, p);
    ENGINE_BLK(802B0100);
    func_802A0290(3, -1, p);
    ENGINE_BLK(802B0114);
    func_802A039C(2, 0, p);
    ENGINE_BLK(802B0128);
    func_802A03D4(2, 0, p);
    ENGINE_BLK(802B013C);
    func_802A040C(2, 1, p);
    ENGINE_BLK(802B0150);
    func_802A0290(2, -1, p);
    ENGINE_BLK(802B0164);
    func_802A05D0(D_802C2390, 100);
    ENGINE_BLK(802B0174);
    func_802A05F8(D_802C2390, 0);
    ENGINE_BLK(802B0184);
    func_802A0620(D_802C2390, 1);
    ENGINE_BLK(802B0194);
    func_802A0508(D_802C2390, -1);
    ENGINE_BLK(802B01A4);
    D_8036444C = 0xD48;
    D_80364450 = 0x12C;
    func_802C4310(0x36);
    ENGINE_BLK(802B01CC);
}

/* hd.c's: whether it can be left: not with a wheel off the ground or the
   rams out */
u8 func_802B01DC(void) {
    VS *vs = &D_803EDB40;
    u8 r = 0;
    s32 v;

    ENGINE_BLK(802B01DC);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802B0200);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802B0210);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802B0220);
                v = part_state(5, D_803ED840);
                ENGINE_BLK(802B0230);
                if (v != 1) {
                    ENGINE_BLK(802B023C);
                    r = 1;
                } else {
                    /* (the part's number, which the original leaves in $v0:
                       5, so it can be left after all) */
                    r = 5;
                }
            }
        }
    }
    ENGINE_BLK(802B0240);
    return r;
}

/* hd.c's: the player gets out */
void func_802B0254(void) {
    VS *vs = &D_803EDB40;

    ENGINE_BLK(802B0254);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EDBF8, (u32 *)D_803EDBFC, 0x800);
    ENGINE_BLK(802B0284);
    func_802C444C();
    ENGINE_BLK(802B028C);
}

/* hd.c's: put back on the ground where it is */
void func_802B02A0(void) {
    VS *vs = &D_803EDB40;

    ENGINE_BLK(802B02A0);
    func_802A9A60(vs->unk52, D_803EDBEC, D_803EDBE8, D_803EDBF0, vs->unk4, &D_803EDBEC, (s16 *)&vs->unk4C, 1, vs, 0);
    ENGINE_BLK(802B032C);
    func_802B0B3C(vs);
    ENGINE_BLK(802B0334);
    func_802A133C(D_803EDBE8, D_803EDBEC, D_803EDBF0, 1, vs);
    ENGINE_BLK(802B0360);
}

/* its light */
void func_802B03B0(void) {
    ENGINE_BLK(802B03B0);
    func_802ABD54(1, D_803EDBE8, D_803EDBEC, D_803EDBF0);
    ENGINE_BLK(802B03E4);
}

/* each frame */
void func_802B03F4(void) {
    VS *vs = &D_803EDB40;
    s32 t3 = 0, t1, x, z, rate_i, turn, h;
    f32 rate;

    ENGINE_BLK(802B03F4);
    func_802B03B0();
    ENGINE_BLK(802B0450);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802B045C);
        func_802B07DC(vs);
    }
    ENGINE_BLK(802B0464);
    func_802B0CE8();
    ENGINE_BLK(802B046C);
    rate_i = func_802B0C74(vs);
    ENGINE_BLK(802B0474);
    func_802A7834(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0x14, rate_i, &vs->unk4C, vs, &t3, &turn);
    ENGINE_BLK(802B0490);
    func_802A7FD8(0x4650, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 1, vs);
    ENGINE_BLK(802B04A8);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802B04B4);
    func_802A843C(&vs->unk76, 1, 1, (s8 *)vs->unk96, vs->unk4, 720.0f, vs);
    ENGINE_BLK(802B04C8);
    if (D_803EDC04 != 0) {
        ENGINE_BLK(802B04D8);
        func_802A7070((s16 *)&D_803EDC02, vs);
    }
    ENGINE_BLK(802B04E4);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EDBE8, &D_803EDBF0, rate, &z);
    ENGINE_BLK(802B04FC);
    D_803ED40B = 1;
    func_802A8768(x, z, &D_803EDBE8, &D_803EDBF0, &D_803EDBEC, 1, 0x2D0, 0x2D0, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802B0530);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B0558);
        func_8029E558(D_803ED840, D_803EDBF8, D_803EDBFC);
        ENGINE_BLK(802B056C);
    } else {
        ENGINE_BLK(802B0574);
        func_8029E558(D_803ED840, D_803EDBFC, D_803EDBF8);
    }
    ENGINE_BLK(802B0588);
    func_802B0B3C(vs);
    ENGINE_BLK(802B0590);
    ENGINE_LEAVE(8, 7);         /* $t0 and $t2: func_8029A800 (56040.c) takes them from the context */
    ENGINE_LEAVE(10, 0x64);
    func_8029A800(D_803EDBE8, D_803EDBEC, D_803EDBF0, D_80305CE0, 0, 1, vs->unk76, 0, 1, vs);
    ENGINE_BLK(802B05DC);
    func_8029C52C(1, vs);
    ENGINE_BLK(802B05E4);
    func_8029AA10();
    ENGINE_BLK(802B05EC);
    D_803F77D0 = D_803ED840;
    func_802BE77C(1, vs);
    ENGINE_BLK(802B0608);
    func_802B0AAC();
    ENGINE_BLK(802B0610);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802B0704);
        D_803EDC04 = 0;
        goto done;
    }
    /* turned toward the camera's heading */
    ENGINE_BLK(802B0620);
    func_8029A914(vs);
    ENGINE_BLK(802B0628);
    D_803EDC04 = 1;
    h = func_802A6F6C();
    ENGINE_BLK(802B0638);
    /* (the difference from the heading, which nothing uses) */
    t1 = (u16)vs->unk4E - 0x800;
    if (t1 < 0) {
        ENGINE_BLK(802B0648);
        t1 += 0xFFF;
    }
    ENGINE_BLK(802B064C);
    h = t1 - h;
    if (h < 0) {
        ENGINE_BLK(802B0658);
        h = -h;
    }
    ENGINE_BLK(802B065C);
    if (!(h < 0x801)) {
        ENGINE_BLK(802B0668);
    }
    ENGINE_BLK(802B0670);
    ENGINE_BLK(802B06C4);
    func_802A70D8(vs);
    ENGINE_BLK(802B06CC);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.18f, vs, &a1);

        ENGINE_BLK(802B06E0);
        D_803EDC02 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802B06F4);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802B06FC);
done:
    ENGINE_BLK(802B070C);
    D_803643E0 = D_803EDBE8;
    D_803643E4 = D_803EDBEC;
    D_803643E8 = D_803EDBF0;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 1, vs);
    ENGINE_BLK(802B0788);
}

/* the parts: the wheels' turn from the camera, the rams (A fires them while
   the hydraulics last), their extension with the C buttons, the wheels'
   spin with the speed, and the engine's sound */
REGS(gp)
void func_802B07DC(VS *vs) {
    Part *p = D_803ED840;
    u32 a, q;
    f32 f;
    s32 t1, s;

    ENGINE_BLK(802B07DC);
    a = (u16)D_80364452 + 0x800;
    if (!((s32)a < 0x1000)) {
        ENGINE_BLK(802B07FC);
        a -= 0xFFF;
    }
    ENGINE_BLK(802B0800);
    ENGINE_BLK(802B0814);
    q = a / 0x555;
    f = (f32)(s32)(a % 0x555) / 1365.0f;
    if (q == 0) {
        ENGINE_BLK(802B0864);
        func_802A0360(2, 0, p, f);
        ENGINE_BLK(802B087C);
    } else {
        ENGINE_BLK(802B0838);
        if (q == 1) {
            ENGINE_BLK(802B0884);
            func_802A0360(2, 1, p, f);
        } else {
            ENGINE_BLK(802B0844);
            func_802A0360(2, 2, p, f);
            ENGINE_BLK(802B085C);
        }
    }
    ENGINE_BLK(802B089C);
    if (D_803EDC06 != 0) {
        ENGINE_BLK(802B08B0);
        D_803EDC06--;
        goto rams;
    }
    ENGINE_BLK(802B08BC);
    t1 = part_state(5, p);
    ENGINE_BLK(802B08CC);
    if (t1 == 1)
        goto rams;
    ENGINE_BLK(802B08D8);
    if (D_80370C1A == 0) {
        ENGINE_BLK(802B08EC);
        if (D_80370C1B == 0)
            goto rams;
    }
    ENGINE_BLK(802B0900);
    t1 = D_803EDC00;
    if (t1 == 0)
        goto rams;
    ENGINE_BLK(802B0914);
    D_803EDC00 = t1 - 1;
    func_802A0290(5, 2, p);
    ENGINE_BLK(802B0930);
    D_803EDC06 = 10;
    func_80260650(D_80367738, 0x37, NULL);
rams:
    ENGINE_BLK(802B0950);
    t1 = D_803EDC05;
    if (D_80370C15 != 0) {
        ENGINE_BLK(802B09B8);
        t1 -= 5;
        if (t1 < 0) {
            ENGINE_BLK(802B09C4);
            t1 = 0;
        }
    } else {
        ENGINE_BLK(802B096C);
        if (D_80370C16 != 0) {
            ENGINE_BLK(802B09CC);
            t1 += 5;
            if (!(t1 < 0x65)) {
                ENGINE_BLK(802B09DC);
                t1 = 0x64;
            }
        } else {
            /* back toward the middle */
            ENGINE_BLK(802B0980);
            if (t1 < 0x32) {
                ENGINE_BLK(802B09A0);
                t1 += 5;
                if (!(t1 < 0x33)) {
                    ENGINE_BLK(802B09B0);
                    t1 = 0x32;
                }
            } else {
                ENGINE_BLK(802B0988);
                t1 -= 5;
                if (t1 < 0x32) {
                    ENGINE_BLK(802B0998);
                    t1 = 0x32;
                }
            }
        }
    }
    ENGINE_BLK(802B09E0);
    D_803EDC05 = t1;
    func_802A0360(3, 0, p, (f32)t1 / 100.0f);
    ENGINE_BLK(802B0A10);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802B0A1C);
        func_802A03D4(1, 1, p);
        ENGINE_BLK(802B0A30);
    } else {
        ENGINE_BLK(802B0A38);
        func_802A03D4(1, 0, p);
    }
    ENGINE_BLK(802B0A4C);
    if (s < 0) {
        ENGINE_BLK(802B0A54);
        s = -s;
    }
    ENGINE_BLK(802B0A58);
    if (s != 0) {
        ENGINE_BLK(802B0A60);
        s = (u32)s / 14;
    }
    ENGINE_BLK(802B0A7C);
    func_802A039C(1, s, p);
    ENGINE_BLK(802B0A90);
    func_802C4584(s);
    ENGINE_BLK(802B0A98);
}

/* the rams out against a breakable thing: the shake */
void func_802B0AAC(void) {
    ENGINE_BLK(802B0AAC);
    if (part_state(5, D_803ED840) == 0) {
        ENGINE_BLK(802B0AC4);
        goto done;
    }
    ENGINE_BLK(802B0AC4);
    ENGINE_BLK(802B0ACC);
    if (func_802BCD80(4) != 0) {
        ENGINE_BLK(802B0AD4);
        goto hit;
    }
    ENGINE_BLK(802B0AD4);
    ENGINE_BLK(802B0ADC);
    if (func_802BCD80(5) == 0) {
        ENGINE_BLK(802B0AE4);
        goto done;
    }
    ENGINE_BLK(802B0AE4);
hit:
    ENGINE_BLK(802B0AEC);
    func_802A03D4(5, 1, D_803ED840);
    ENGINE_BLK(802B0B00);
    func_802A0290(5, 1, D_803ED840);
    ENGINE_BLK(802B0B14);
    D_802E8BE4 = 10;
    D_802E8BE8 = 600;
done:
    ENGINE_BLK(802B0B2C);
}

/* the Sideswipe's matrix, its vertices and its collision */
REGS(gp)
void func_802B0B3C(VS *vs) {
    u8 *model = D_803EDBF4, *buf;
    s32 *m, off;

    ENGINE_BLK(802B0B3C);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B0B6C);
        m = (s32 *)(D_803EDBF8 + off);
    } else {
        ENGINE_BLK(802B0B80);
        m = (s32 *)(D_803EDBFC + off);
    }
    ENGINE_BLK(802B0B90);
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803EDBE8, D_803EDBEC, D_803EDBF0, 0x1B58, m);
    ENGINE_BLK(802B0BCC);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B0BE0);
        buf = D_803EDBF8;
    } else {
        ENGINE_BLK(802B0BF0);
        buf = D_803EDBFC;
    }
    ENGINE_BLK(802B0BFC);
    model = D_803EDBF4;
    func_8029C454(D_803EDBE8, D_803EDBEC, D_803EDBF0, 1, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802B0C44);
    func_802ABBEC(1, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802B0C64);
}

/* the turn rate: the speed / 3.2, or / 6 with a wheel off the ground */
REGS(gp -> s3)
s32 func_802B0C74(VS *vs) {
    f32 d;

    ENGINE_BLK(802B0C74);
    if (vs->unk96[0] == 1)
        goto air;
    ENGINE_BLK(802B0C88);
    if (vs->unk96[1] == 1)
        goto air;
    ENGINE_BLK(802B0C98);
    if (vs->unk96[2] == 1)
        goto air;
    ENGINE_BLK(802B0CA8);
    d = 3.2f;
    goto div;
air:
    ENGINE_BLK(802B0CB0);
    d = 6.0f;
div:
    ENGINE_BLK(802B0CBC);
    return engine_cvt_w_s((f32)vs->unk76 / d);
}

/* the camera's distance and speed for the Sideswipe */
void func_802B0CE8(void) {
    ENGINE_BLK(802B0CE8);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x28;
    D_803ED3F7 = 3;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B0D44(u8 *dst) {
    ENGINE_BLK(802B0D44);
    func_802AC7DC(dst, (u8 *)&D_803EDB40, (u32 *)&D_803EDBE8);
    ENGINE_BLK(802B0D60);
}

/* and back */
void func_802B0D70(u8 *src) {
    ENGINE_BLK(802B0D70);
    func_802AC85C(src, (u8 *)&D_803EDB40, (u32 *)&D_803EDBE8);
    ENGINE_BLK(802B0D8C);
}
