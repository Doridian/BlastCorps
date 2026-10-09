/*
 * hd_code 8DDB0 (us.v11 0x802D2570-0x802D30C4): the shuttle (type 0xFD,
 * VEHICLE_SHUTTLE: the type its Vehicle record and points are made with;
 * game/vehicle.h's table calls it the chopper), as native C (engine.h).
 * Its parts are D_803FC9A0, its state D_803FCCA0 and its position
 * D_803FCD48..50 (vehicle.h); func_802D2570 sets it up (from the level
 * loader), func_802D291C runs it each frame (from hd.c).  It doesn't drive:
 * hd.c moves it (D_803FCD60 its height, D_803FCD6A..6E its angles) and
 * sends it events (D_803FCD70); this is its sounds, its effects and its
 * matrix.
 */
#include "vehicle.h"
#include "game/audio.h"

/* the chopper's .bss (asm/data/hd_code/8DDB0.bss.s) */
extern Part D_803FC9A0[32];
extern VS D_803FCCA0;
extern u8 *PTR32 D_803FCD54;                    /* its model file */
extern u8 *PTR32 D_803FCD58;                    /* two 0x800-byte buffers, one per frame */
extern u8 *PTR32 D_803FCD5C;
extern SndState *PTR32 D_803FCD64;              /* its engine's sound */
extern u8 D_803FCD72, D_803FCD73, D_803FCD74;   /* the three waypoints' sounds, played */
extern u8 D_803FCD76, D_803FCD79;              /* frames until the next exhaust, the next smoke */
extern u8 D_803FCD77;                           /* exhaust on */
extern u8 D_803FCD78;                           /* smoke on */
extern u8 D_803FCD7A;                           /* the screen shaking */

#define PARTS D_803FC9A0
#define MODEL D_803FCD54
#define BUF0 D_803FCD58
#define BUF1 D_803FCD5C

#define SHUTTLE_SCALE 0x11558           /* its model's scale */
#define SHUTTLE_EFFECT_FRAMES 4         /* its exhaust and smoke every this many frames + 1 */
#define SHUTTLE_SHAKE 0x14              /* the screen's shake while it shakes (D_802E8BE4) */

extern u8 D_8036B964;
extern s32 D_802E8BE8;
extern u8 D_802C28E4[], D_802C3804[];           /* effect records (60F60) */

void func_802D2A40(void);
void func_802D2A74(void);
void func_802D2C20(void);
REGS(gp)
void func_802D2FA4(VS *vs);

/* set up: the model's buffers, its Vehicle record, its parts, the rotor's
   sound, and a first frame */
REGS(s2)
void func_802D2570(u8 *model) {
    VS *vs = &D_803FCCA0;

    MODEL = model;
    BUF0 = D_80358070;
    BUF1 = D_80358070 + 0x800;
    D_80358070 += 0x1000;
    func_802A1388(VEHICLE_SHUTTLE, 0, BUF0, BUF1, model);
    VS_HEADING(vs) = 0;
    VS_MOVE_HEADING(vs) = 0;
    VS_SPEED(vs) = 0;
    D_803FCD72 = 0;
    D_803FCD73 = 0;
    D_803FCD74 = 0;
    D_803FCD76 = 0;
    D_803FCD77 = 0;
    D_803FCD7A = 0;
    D_803FCD78 = 0;
    D_803FCD79 = 0;
    func_8029F85C(PARTS, MODEL, BUF0, BUF1);
    func_802A039C(0, 100, PARTS);
    func_802A03D4(0, 0, PARTS);
    func_802A040C(0, 0, PARTS);
    func_802A0480(0, 0, PARTS, 0.0f);
    func_802A0290(0, 1, PARTS);
    func_8029E558(PARTS, BUF0, BUF1);
    func_802A0320(0, PARTS);
    func_802A0290(0, 1, PARTS);
    func_8029E558(PARTS, BUF1, BUF0);
    if (D_803FCD75 == 1) {
        func_802A0360(2, 0, PARTS, 0.0f);
    } else {
        if (D_803FCD75 != 0)
            engine_trap(N64_PC(0x802D2750));
        /* its engine running */
        func_80260650(D_80367738, 0x6A, &D_803FCD64);
        func_80260AB8(D_803FCD64, 8, 0);
        func_802A0360(2, 1, PARTS, 0.0f);
    }
    func_802A039C(2, 0, PARTS);
    func_802A03D4(2, 0, PARTS);
    func_802A040C(2, 1, PARTS);
    func_802A0290(2, -1, PARTS);
    func_802A039C(1, 0, PARTS);
    func_802A03D4(1, 0, PARTS);
    func_802A0360(1, 0, PARTS, 0.0f);
    func_802A040C(1, 1, PARTS);
    func_802A0290(1, -1, PARTS);
    func_80258230(VEHICLE_SHUTTLE, 0x78, 0x2D, 0x2D);
    func_80268F54();
    /* its first frame */
    VS_IN_SETUP(vs) = 1;
    func_802D291C();
    VS_IN_SETUP(vs) = 0;
    func_802AA838(BUF1, BUF0, MODEL_MTX_OFF(MODEL));
}

/* each frame */
void func_802D291C(void) {
    VS *vs = &D_803FCCA0;

    func_802D2A40();
    if (VS_IN_SETUP(vs) == 0) {
        func_802D2A74();
        func_802D2C20();
    }
    func_8029E558(PARTS, FRAME_BUF(BUF0, BUF1), OTHER_BUF(BUF0, BUF1));
    func_802D2FA4(vs);
}

void func_802D2A40(void) {
    func_80269258();
}

/* its engine's sound, quieter with the camera's distance */
void func_802D2A74(void) {
    s32 d, v;

    if (D_803FCD64 == NULL)
        return;
    d = (s32)func_802ABCDC(D_803643F8 >> 11, D_803643FC >> 11, D_80364400 >> 11, D_803FCD48, D_803FCD4C, D_803FCD50);
    d -= 0x3200;
    if (d < 0)
        d = 0;
    v = 0x7FFF - (d >> 2);
    if (v < 0)
        v = 0;
    func_80260AB8(D_803FCD64, 8, v);
}

/* The event hd.c sent this frame (D_803FCD70: 2 part 1's animation, 3 a
   sound, 0xB exhaust, smoke and the shake on, 0xC its engine's sound and
   the shake off, 0xD the smoke off); a sound as it rises past each of its
   three points (unless D_8036B964); the screen's shake; and its smoke and
   exhaust every SHUTTLE_EFFECT_FRAMES + 1 frames. */
void func_802D2C20(void) {
    s16 point_sound[3] = { 6, 6, 8 };
    u8 *played[3];
    s32 s0 = 0, k;

    switch (D_803FCD70) {
    case 2:
        func_802A039C(1, 1, PARTS);
        func_802A03D4(1, 0, PARTS);
        func_802A0290(1, 1, PARTS);
        func_80260650(D_80367738, 0x66, NULL);
        break;
    case 3:
        func_80260650(D_80367738, 0x78, NULL);
        break;
    case 0xB:
        D_803FCD77 = 1;
        D_803FCD7A = 1;
        D_803FCD78 = 1;
        func_80260650(D_80367738, 0x83, NULL);
        break;
    case 0xC:
        func_80260650(D_80367738, 0x7F, &D_803FCD64);
        D_803FCD7A = 0;
        break;
    case 0xD:
        D_803FCD78 = 0;
        break;
    }
    if (D_8036B964 == 0) {
        s0 = D_803FCD60;
        played[0] = &D_803FCD72, played[1] = &D_803FCD73, played[2] = &D_803FCD74;
        for (k = 0; k < 3; k++) {
            if (*played[k] == 0 && s0 >= *(s32 *)((u8 *)func_802ABC88(VEHICLE_SHUTTLE, k + 1) + 4)) {
                func_80260650(D_80367738, point_sound[k], NULL);
                *played[k] = 1;
            }
        }
    }
    if (D_803FCD7A != 0) {
        D_802E8BE4 = SHUTTLE_SHAKE;
        D_802E8BE8 = 0x190;
    }
    if (D_803FCD78 != 0) {
        if (D_803FCD79 != 0) {
            D_803FCD79--;
        } else {
            D_803FCD79 = SHUTTLE_EFFECT_FRAMES;
            func_802A6274((s32)D_802C28E4, 0x107AC0, 1, VEHICLE_SHUTTLE, 4, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
        }
    }
    if (D_803FCD77 != 0) {
        if (D_803FCD76 != 0) {
            D_803FCD76--;
        } else {
            D_803FCD76 = SHUTTLE_EFFECT_FRAMES;
            func_802A6274((s32)D_802C3804, 0x53020, 1, VEHICLE_SHUTTLE, 4, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
            func_802A6274((s32)D_802C3804, 0x53020, 1, VEHICLE_SHUTTLE, 5, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
            func_802A6274((s32)D_802C3804, 0x5CC60, 1, VEHICLE_SHUTTLE, 6, 1, 0, 0, s0, 0, 0, 0, 0, 0, 0);
        }
    }
}

/* the shuttle's matrix (its position and D_803FCD6A..6E's angles) into the
   frame's buffer, and its points */
REGS(gp)
void func_802D2FA4(VS *vs) {
    u8 *model = MODEL, *buf = FRAME_BUF(BUF0, BUF1);
    u16 h = (u16)D_803FCD6C;

    D_803ED390[0] = D_803FCD6A;
    VS_HEADING(vs) = h;
    D_803FCD68 = h;
    D_803ED390[1] = h;
    D_803ED390[2] = D_803FCD6E;
    func_802AA764(D_803FCD48, D_803FCD4C, D_803FCD50, SHUTTLE_SCALE, (s32 *)(buf + MODEL_MTX_OFF(model)));
    func_802ABBEC(VEHICLE_SHUTTLE, MODEL_AT(model, 0), MODEL_AT(model, 4), buf);
}
