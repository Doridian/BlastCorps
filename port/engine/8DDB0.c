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

    ENGINE_COST(802D2570, 217);
    D_803FCD54 = model;
    buf = D_80358070;
    D_803FCD58 = buf;
    D_803FCD5C = buf + 0x800;
    D_80358070 = buf + 0x1000;
    func_802A1388(0xFD, 0, D_803FCD58, D_803FCD5C, model);
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
    func_802A039C(0, 100, D_803FC9A0);
    func_802A03D4(0, 0, D_803FC9A0);
    func_802A040C(0, 0, D_803FC9A0);
    func_802A0480(0, 0, D_803FC9A0, 0.0f);
    func_802A0290(0, 1, D_803FC9A0);
    func_8029E558(D_803FC9A0, D_803FCD58, D_803FCD5C);
    func_802A0320(0, D_803FC9A0);
    func_802A0290(0, 1, D_803FC9A0);
    func_8029E558(D_803FC9A0, D_803FCD5C, D_803FCD58);
    if (D_803FCD75 != 1) {
        if (D_803FCD75 != 0) {
            engine_trap(0x802D2750);
        }
        /* the rotor turning */
        func_80260650(D_80367738, 0x6A, &D_803FCD64);
        func_80260AB8(D_803FCD64, 8, 0);
        func_802A0360(2, 1, D_803FC9A0, 0.0f);
    } else {
        func_802A0360(2, 0, D_803FC9A0, 0.0f);
    }
    func_802A039C(2, 0, D_803FC9A0);
    func_802A03D4(2, 0, D_803FC9A0);
    func_802A040C(2, 1, D_803FC9A0);
    func_802A0290(2, -1, D_803FC9A0);
    func_802A039C(1, 0, D_803FC9A0);
    func_802A03D4(1, 0, D_803FC9A0);
    func_802A0360(1, 0, D_803FC9A0, 0.0f);
    func_802A040C(1, 1, D_803FC9A0);
    func_802A0290(1, -1, D_803FC9A0);
    func_80258230(0xFD, 0x78, 0x2D, 0x2D);
    func_80268F54();
    vs->unk9A = 1;
    func_802D291C();
    vs->unk9A = 0;
    /* the frame's vertices into the other buffer */
    func_802AA838(D_803FCD5C, D_803FCD58, *(s32 *)(D_803FCD54 + *(s32 *)(D_803FCD54 + 0x18) + 4));
}

/* each frame */
void func_802D291C(void) {
    VS *vs = &D_803FCCA0;
    u8 *a, *b;

    ENGINE_COST(802D291C, 67);
    /* (its $s4 as it found it: 5CB60.c reads it from the context) */
    engine_save(ENGINE_GPR(20), 0);
    func_802D2A40();
    if (vs->unk9A == 0) {
        func_802D2A74();
        func_802D2C20();
    }
    a = D_803FCD58;
    b = D_803FCD5C;
    if (D_8035805C != 0) {
        func_8029E558(D_803FC9A0, a, b);
    } else {
        func_8029E558(D_803FC9A0, b, a);
    }
    func_802D2FA4(vs);
    engine_restore();
}

void func_802D2A40(void) {
    ENGINE_COST(802D2A40, 13);
    func_80269258();
}

/* the rotor's sound, quieter with the camera's distance */
void func_802D2A74(void) {
    s32 d, v;

    ENGINE_COST(802D2A74, 106);
    if (D_803FCD64 != NULL) {
        d = (s32)func_802ABCDC(D_803643F8 >> 11, D_803643FC >> 11, D_80364400 >> 11, D_803FCD48, D_803FCD4C,
                               D_803FCD50);
        d -= 0x3200;
        if (d < 0) {
            d = 0;
        }
        v = 0x7FFF - (d >> 2);
        if (v < 0) {
            v = 0;
        }
        func_80260AB8(D_803FCD64, 8, v);
    }
}

/* the event in D_803FCD70, the waypoints' sounds and the effects */
void func_802D2C20(void) {
    u8 e = D_803FCD70, *p;
    s32 s0 = 0;

    ENGINE_COST(802D2C20, 63);
    if (e != 0) {
        if (e == 2) {
            func_802A039C(1, 1, D_803FC9A0);
            func_802A03D4(1, 0, D_803FC9A0);
            func_802A0290(1, 1, D_803FC9A0);
            func_80260650(D_80367738, 0x66, NULL);
        } else {
            if (e == 3) {
                func_80260650(D_80367738, 0x78, NULL);
            } else {
                if (e == 0xB) {
                    D_803FCD77 = 1;
                    D_803FCD7A = 1;
                    D_803FCD78 = 1;
                    func_80260650(D_80367738, 0x83, NULL);
                } else {
                    if (e == 0xC) {
                        func_80260650(D_80367738, 0x7F, &D_803FCD64);
                        D_803FCD7A = 0;
                    } else {
                        if (e == 0xD) {
                            D_803FCD78 = 0;
                        } else {
                        }
                    }
                }
            }
        }
    }
    if (D_8036B964 == 0) {
        s0 = D_803FCD60;
        if (D_803FCD72 == 0) {
            p = func_802ABC88(0xFD, 1);
            if (!(s0 < *(s32 *)(p + 4))) {
                func_80260650(D_80367738, 6, NULL);
                D_803FCD72 = 1;
            }
        }
        if (D_803FCD73 == 0) {
            p = func_802ABC88(0xFD, 2);
            if (!(s0 < *(s32 *)(p + 4))) {
                func_80260650(D_80367738, 6, NULL);
                D_803FCD73 = 1;
            }
        }
        if (D_803FCD74 == 0) {
            p = func_802ABC88(0xFD, 3);
            if (!(s0 < *(s32 *)(p + 4))) {
                func_80260650(D_80367738, 8, NULL);
                D_803FCD74 = 1;
            }
        }
    }
    if (D_803FCD7A != 0) {
        D_802E8BE4 = 0x14;
        D_802E8BE8 = 0x190;
    }
    if (D_803FCD78 != 0) {
        if (D_803FCD79 != 0) {
            D_803FCD79--;
        } else {
            D_803FCD79 = 4;
            func_802A6274((s32)D_802C28E4, 0x107AC0, 1, 0xFD, 4, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
        }
    }
    if (D_803FCD77 != 0) {
        if (D_803FCD76 != 0) {
            D_803FCD76--;
        } else {
            D_803FCD76 = 4;
            func_802A6274((s32)D_802C3804, 0x53020, 1, 0xFD, 4, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
            func_802A6274((s32)D_802C3804, 0x53020, 1, 0xFD, 5, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
            func_802A6274((s32)D_802C3804, 0x5CC60, 1, 0xFD, 6, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
        }
    }
}

/* the chopper's matrix (its position and D_803FCD6A..6E's angles) into the
   frame's buffer, and its vertices transformed */
REGS(gp)
void func_802D2FA4(VS *vs) {
    u8 *model = D_803FCD54, *buf;
    s32 *m, off;
    u16 h;

    ENGINE_COST(802D2FA4, 64);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803FCD58 + off);
    } else {
        m = (s32 *)(D_803FCD5C + off);
    }
    D_803ED390[0] = D_803FCD6A;
    h = (u16)D_803FCD6C;
    vs->unk4C = h;
    D_803FCD68 = h;
    D_803ED390[1] = h;
    D_803ED390[2] = D_803FCD6E;
    func_802AA764(D_803FCD48, D_803FCD4C, D_803FCD50, 0x11558, m);
    if (D_8035805C != 0) {
        buf = D_803FCD58;
    } else {
        buf = D_803FCD5C;
    }
    func_802ABBEC(0xFD, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}
