/*
 * hd_code 8DDB0 (us.v11 0x802D2570-0x802D30C4): the BCT chopper
 * (VEHICLE_CHOPPER), as native C (engine.h).  Its parts are D_803FC9A0, its
 * state D_803FCCA0 (vehicle.h); func_802D2570 sets it up (from the level
 * loader), func_802D291C runs it each frame (from hd.c).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/audio.h"

/* the chopper's .bss (asm/data/hd_code/8DDB0.bss.s) */
extern Part D_803FC9A0[32];
extern VS D_803FCCA0;
extern u8 *PTR32 D_803FCD54;                    /* its model file */
extern u8 *PTR32 D_803FCD58;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803FCD5C;
extern SndState *PTR32 D_803FCD64;              /* its engine's sound */
extern u8 D_803FCD72, D_803FCD73, D_803FCD74;   /* the three waypoints' sounds, played */
extern u8 D_803FCD76, D_803FCD77, D_803FCD78, D_803FCD79, D_803FCD7A;

extern u8 D_8036B964;
extern u8 D_802E8BE4;
extern s32 D_802E8BE8;
extern u8 D_802C28E4[], D_802C3804[];           /* effect records (60F60) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80260AB8(SndState *state, s16 type, s32 param);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_80268F54(void);
void func_80269258(void);

void func_802D2A40(void);
void func_802D2A74(void);
void func_802D2C20(void);
REGS(gp)
void func_802D2FA4(VS *vs);
void func_802D291C(void);

/* set up: the model's buffers, its Vehicle record, its parts, the rotor's
   sound, and a first frame */
REGS(s2)
void func_802D2570(u8 *model) {
    u8 *buf;
    VS *vs;

    ENGINE_BLK(802D2570);
    D_803FCD54 = model;
    buf = D_80358070;
    D_803FCD58 = buf;
    D_803FCD5C = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(0xFD, 0, D_803FCD58, D_803FCD5C, model);
    ENGINE_BLK(802D25F0);
    vs = &D_803FCCA0;
    vs->unk4C = 0;
    vs->unk4E = 0;
    vs->unk76 = 0;
    D_803FCD72 = 0;
    D_803FCD73 = 0;
    D_803FCD74 = 0;
    D_803FCD76 = 0;
    D_803FCD77 = 0;
    D_803FCD7A = 0;
    D_803FCD78 = 0;
    D_803FCD79 = 0;
    func_8029F85C(D_803FC9A0, D_803FCD54, D_803FCD58, D_803FCD5C);
    ENGINE_BLK(802D267C);
    func_802A039C(0, 100, D_803FC9A0);
    ENGINE_BLK(802D2690);
    func_802A03D4(0, 0, D_803FC9A0);
    ENGINE_BLK(802D26A4);
    func_802A040C(0, 0, D_803FC9A0);
    ENGINE_BLK(802D26B8);
    func_802A0480(0, 0, D_803FC9A0, 0.0f);
    ENGINE_BLK(802D26D0);
    func_802A0290(0, 1, D_803FC9A0);
    ENGINE_BLK(802D26E4);
    func_8029E558(D_803FC9A0, D_803FCD58, D_803FCD5C);
    ENGINE_BLK(802D26F8);
    func_802A0320(0, D_803FC9A0);
    ENGINE_BLK(802D2708);
    func_802A0290(0, 1, D_803FC9A0);
    ENGINE_BLK(802D271C);
    func_8029E558(D_803FC9A0, D_803FCD5C, D_803FCD58);
    ENGINE_BLK(802D2730);
    if (D_803FCD75 != 1) {
        ENGINE_BLK(802D2748);
        if (D_803FCD75 != 0) {
            ENGINE_BLK(802D2750);
            engine_trap(0x802D2750);
        }
        /* the rotor turning */
        ENGINE_BLK(802D2774);
        func_80260650(D_80367738, 0x6A, &D_803FCD64);
        ENGINE_BLK(802D279C);
        func_80260AB8(D_803FCD64, 8, 0);
        ENGINE_BLK(802D27AC);
        func_802A0360(2, 1, D_803FC9A0, 0.0f);
    } else {
        ENGINE_BLK(802D2754);
        func_802A0360(2, 0, D_803FC9A0, 0.0f);
        ENGINE_BLK(802D276C);
    }
    ENGINE_BLK(802D27D4);
    func_802A039C(2, 0, D_803FC9A0);
    ENGINE_BLK(802D27E8);
    func_802A03D4(2, 0, D_803FC9A0);
    ENGINE_BLK(802D27FC);
    func_802A040C(2, 1, D_803FC9A0);
    ENGINE_BLK(802D2810);
    func_802A0290(2, -1, D_803FC9A0);
    ENGINE_BLK(802D2824);
    func_802A039C(1, 0, D_803FC9A0);
    ENGINE_BLK(802D2838);
    func_802A03D4(1, 0, D_803FC9A0);
    ENGINE_BLK(802D284C);
    func_802A0360(1, 0, D_803FC9A0, 0.0f);
    ENGINE_BLK(802D2868);
    func_802A040C(1, 1, D_803FC9A0);
    ENGINE_BLK(802D287C);
    func_802A0290(1, -1, D_803FC9A0);
    ENGINE_BLK(802D2890);
    func_80258230(0xFD, 0x78, 0x2D, 0x2D);
    ENGINE_BLK(802D28A8);
    func_80268F54();
    ENGINE_BLK(802D28B0);
    vs->unk9A = 1;
    func_802D291C();
    ENGINE_BLK(802D28C0);
    vs->unk9A = 0;
    /* the frame's vertices into the other buffer */
    func_802AA838(D_803FCD5C, D_803FCD58, *(s32 *)(D_803FCD54 + *(s32 *)(D_803FCD54 + 0x18) + 4));
    ENGINE_BLK(802D28F8);
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, (u32)vs);
    ENGINE_LEAVE(22, (u32)D_803FCD58);
    ENGINE_LEAVE(23, (u32)D_803FCD5C);
}

/* each frame */
void func_802D291C(void) {
    VS *vs = &D_803FCCA0;
    u8 *a, *b;

    ENGINE_BLK(802D291C);
    func_802D2A40();
    ENGINE_BLK(802D2974);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802D2980);
        func_802D2A74();
        ENGINE_BLK(802D2988);
        func_802D2C20();
    }
    ENGINE_BLK(802D2990);
    a = D_803FCD58;
    b = D_803FCD5C;
    if (D_8035805C != 0) {
        ENGINE_BLK(802D29B8);
        func_8029E558(D_803FC9A0, a, b);
        ENGINE_BLK(802D29CC);
    } else {
        ENGINE_BLK(802D29D4);
        func_8029E558(D_803FC9A0, b, a);
    }
    ENGINE_BLK(802D29E8);
    func_802D2FA4(vs);
    ENGINE_BLK(802D29F0);
}

void func_802D2A40(void) {
    ENGINE_BLK(802D2A40);
    func_80269258();
    ENGINE_BLK(802D2A58);
}

/* the rotor's sound, quieter with the camera's distance */
void func_802D2A74(void) {
    s32 d, v;

    ENGINE_BLK(802D2A74);
    if (D_803FCD64 != NULL) {
        ENGINE_BLK(802D2B04);
        d = (s32)func_802ABCDC(D_803643F8 >> 11, D_803643FC >> 11, D_80364400 >> 11, D_803FCD48, D_803FCD4C,
                               D_803FCD50);
        ENGINE_BLK(802D2B60);
        d -= 0x3200;
        if (d < 0) {
            ENGINE_BLK(802D2B6C);
            d = 0;
        }
        ENGINE_BLK(802D2B70);
        v = 0x7FFF - (d >> 2);
        if (v < 0) {
            ENGINE_BLK(802D2B84);
            v = 0;
        }
        ENGINE_BLK(802D2B88);
        func_80260AB8(D_803FCD64, 8, v);
    }
    ENGINE_BLK(802D2B98);
}

/* the event in D_803FCD70, the waypoints' sounds and the effects */
void func_802D2C20(void) {
    u8 e = D_803FCD70, *p;
    s32 s0 = 0;

    ENGINE_BLK(802D2C20);
    if (e != 0) {
        ENGINE_BLK(802D2C3C);
        if (e == 2) {
            ENGINE_BLK(802D2C8C);
            func_802A039C(1, 1, D_803FC9A0);
            ENGINE_BLK(802D2CA0);
            func_802A03D4(1, 0, D_803FC9A0);
            ENGINE_BLK(802D2CB4);
            func_802A0290(1, 1, D_803FC9A0);
            ENGINE_BLK(802D2CC8);
            func_80260650(D_80367738, 0x66, NULL);
            ENGINE_BLK(802D2CDC);
        } else {
            ENGINE_BLK(802D2C48);
            if (e == 3) {
                ENGINE_BLK(802D2C70);
                func_80260650(D_80367738, 0x78, NULL);
                ENGINE_BLK(802D2C84);
            } else {
                ENGINE_BLK(802D2C50);
                if (e == 0xB) {
                    ENGINE_BLK(802D2CE4);
                    D_803FCD77 = 1;
                    D_803FCD7A = 1;
                    D_803FCD78 = 1;
                    func_80260650(D_80367738, 0x83, NULL);
                    ENGINE_BLK(802D2D14);
                } else {
                    ENGINE_BLK(802D2C58);
                    if (e == 0xC) {
                        ENGINE_BLK(802D2D1C);
                        func_80260650(D_80367738, 0x7F, &D_803FCD64);
                        ENGINE_BLK(802D2D34);
                        D_803FCD7A = 0;
                    } else {
                        ENGINE_BLK(802D2C60);
                        if (e == 0xD) {
                            ENGINE_BLK(802D2D40);
                            D_803FCD78 = 0;
                        } else {
                            ENGINE_BLK(802D2C68);
                        }
                    }
                }
            }
        }
    }
    ENGINE_BLK(802D2D48);
    if (D_8036B964 == 0) {
        ENGINE_BLK(802D2D58);
        s0 = D_803FCD60;
        if (D_803FCD72 == 0) {
            ENGINE_BLK(802D2D70);
            p = func_802ABC88(0xFD, 1);
            ENGINE_BLK(802D2D7C);
            if (!(s0 < *(s32 *)(p + 4))) {
                ENGINE_BLK(802D2D8C);
                func_80260650(D_80367738, 6, NULL);
                ENGINE_BLK(802D2DA0);
                D_803FCD72 = 1;
            }
        }
        ENGINE_BLK(802D2DAC);
        if (D_803FCD73 == 0) {
            ENGINE_BLK(802D2DBC);
            p = func_802ABC88(0xFD, 2);
            ENGINE_BLK(802D2DC8);
            if (!(s0 < *(s32 *)(p + 4))) {
                ENGINE_BLK(802D2DD8);
                func_80260650(D_80367738, 6, NULL);
                ENGINE_BLK(802D2DEC);
                D_803FCD73 = 1;
            }
        }
        ENGINE_BLK(802D2DF8);
        if (D_803FCD74 == 0) {
            ENGINE_BLK(802D2E08);
            p = func_802ABC88(0xFD, 3);
            ENGINE_BLK(802D2E14);
            if (!(s0 < *(s32 *)(p + 4))) {
                ENGINE_BLK(802D2E24);
                func_80260650(D_80367738, 8, NULL);
                ENGINE_BLK(802D2E38);
                D_803FCD74 = 1;
            }
        }
    }
    ENGINE_BLK(802D2E44);
    if (D_803FCD7A != 0) {
        ENGINE_BLK(802D2E54);
        D_802E8BE4 = 0x14;
        D_802E8BE8 = 0x190;
    }
    ENGINE_BLK(802D2E6C);
    if (D_803FCD78 != 0) {
        ENGINE_BLK(802D2E7C);
        if (D_803FCD79 != 0) {
            ENGINE_BLK(802D2E90);
            D_803FCD79--;
        } else {
            ENGINE_BLK(802D2E9C);
            D_803FCD79 = 4;
            func_802A6274((s32)D_802C28E4, 0x107AC0, 1, 0xFD, 4, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
        }
    }
    ENGINE_BLK(802D2ED0);
    if (D_803FCD77 != 0) {
        ENGINE_BLK(802D2EE0);
        if (D_803FCD76 != 0) {
            ENGINE_BLK(802D2EF4);
            D_803FCD76--;
        } else {
            ENGINE_BLK(802D2F00);
            D_803FCD76 = 4;
            func_802A6274((s32)D_802C3804, 0x53020, 1, 0xFD, 4, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
            ENGINE_BLK(802D2F34);
            func_802A6274((s32)D_802C3804, 0x53020, 1, 0xFD, 5, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
            ENGINE_BLK(802D2F60);
            func_802A6274((s32)D_802C3804, 0x5CC60, 1, 0xFD, 6, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
        }
    }
    ENGINE_BLK(802D2F8C);
}

/* the chopper's matrix (its position and D_803FCD6A..6E's angles) into the
   frame's buffer, and its vertices transformed */
REGS(gp)
void func_802D2FA4(VS *vs) {
    u8 *model = D_803FCD54, *buf;
    s32 *m, off;
    u16 h;

    ENGINE_BLK(802D2FA4);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802D2FD4);
        m = (s32 *)(D_803FCD58 + off);
    } else {
        ENGINE_BLK(802D2FE8);
        m = (s32 *)(D_803FCD5C + off);
    }
    ENGINE_BLK(802D2FF8);
    D_803ED390[0] = D_803FCD6A;
    h = (u16)D_803FCD6C;
    vs->unk4C = h;
    D_803FCD68 = h;
    D_803ED390[1] = h;
    D_803ED390[2] = D_803FCD6E;
    func_802AA764(D_803FCD48, D_803FCD4C, D_803FCD50, 0x11558, m);
    ENGINE_BLK(802D3060);
    if (D_8035805C != 0) {
        ENGINE_BLK(802D3074);
        buf = D_803FCD58;
    } else {
        ENGINE_BLK(802D3084);
        buf = D_803FCD5C;
    }
    ENGINE_BLK(802D3090);
    func_802ABBEC(0xFD, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802D30B4);
}
