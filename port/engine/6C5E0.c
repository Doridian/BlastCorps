/*
 * hd_code 6C5E0 (us.v11 0x802B0DA0-0x802B29B0): Thunderfist
 * (VEHICLE_MAGOO), as native C (engine.h).  Its parts are D_803EDC10, its
 * state D_803EDF10 and its position D_803EDFB8..C0 (vehicle.h).
 * func_802B0DA0 sets it up (from the level loader), func_802B152C runs it
 * each frame (from hd.c and at the end of the setup); the others are hd.c's
 * hooks for it.
 *
 * It runs on two legs (parts 1 and 5, unkA2 the one in front) and rolls
 * into a ball (state unkA1: 0 walking, 1 rolling, 2 rolling up a slope, 3
 * and 4 the landing after a fall); part 0x1F is the body's animation.
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

/* Thunderfist's .bss (asm/data/hd_code/6C5E0.bss.s) */
extern Part D_803EDC10[32];
extern VS D_803EDF10;
extern s32 D_803EDFB8, D_803EDFBC, D_803EDFC0;  /* x, y, z */
extern u8 *PTR32 D_803EDFC4;                    /* its model file */
extern u8 *PTR32 D_803EDFC8;                    /* two 0x1400-byte buffers, one per frame */
extern u8 *PTR32 D_803EDFCC;
extern SndState *PTR32 D_803EDFD0;              /* its sound while it is in */
extern u16 D_803EDFD4;                          /* the heading it turns to (with D_803A7425) */
extern s16 D_803EDFD6;                          /* the step sounds' last frame */
extern s8 D_803EDFD8;                           /* turning to it */
extern u8 D_803EDFD9;
extern u8 D_803EDFDA;                           /* facing the camera's way (with D_803A7425) */

extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7425;
extern u64 D_803649D8;
extern u8 D_80370C1A, D_80370C1B, D_80370C1C, D_80370C1D, D_80370C35;
extern u8 D_803F7804;
extern SndState *PTR32 D_803F7844;              /* the rolling sound */
extern Part *PTR32 D_803F77D0;
extern u8 D_80305CF0[];
extern s32 D_803643E4, D_803643E8;
extern u8 D_802C2308[];                         /* its head (56040's list) */
extern u8 D_802C2984[];                         /* the dust's effect record (60F60) */

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_80278EB0(s32 a, f32 b, s32 c);
void func_802794A4(void);
s32 func_8026A8E0(s32 lo, s32 hi);

void func_802B152C(void);
void func_802B14E8(void);
REGS(gp)
void func_802B18F4(VS *vs);
REGS(gp)
void func_802B2768(VS *vs);
REGS(gp -> s3)
s32 func_802B28B8(VS *vs);
void func_802B2900(void);

#define T(p) ((s32)(p))
#define P D_803EDC10

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
void func_802B0DA0(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    s32 avg;
    VS *vs = &D_803EDF10;
    u8 *buf;
    s16 *r;

    ENGINE_BLK(802B0DA0);
    engine_save(ENGINE_T0_T5, 0);
    D_803EDFC4 = model;
    buf = D_80358070;
    D_803EDFC8 = buf;
    D_803EDFCC = buf + 0x1400;
    D_80358070 = buf + 0x2800;
    func_802A1388(2, 1, D_803EDFC8, D_803EDFCC, model);
    ENGINE_BLK(802B0E20);
    func_802A754C(vs);
    ENGINE_BLK(802B0E2C);
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
    D_803EDFB8 = x;
    D_803EDFBC = y;
    D_803EDFC0 = z;
    vs->unk4C = heading;
    vs->unkA1 = 0;
    vs->unkA2 = 0;
    vs->unk4E = heading;
    vs->unk74 = heading;
    vs->unkA3 = 1;
    func_802A992C(vs->unk52, D_803EDFBC, x, z, vs->unk4, &D_803EDFBC, (s16 *)&vs->unk4C, 2, vs, engine_ctx(30), &avg);
    ENGINE_BLK(802B0ED4);
    func_8029F85C(P, D_803EDFC4, D_803EDFC8, D_803EDFCC);
    ENGINE_BLK(802B0F10);
    func_802A039C(0, 100, P);
    ENGINE_BLK(802B0F24);
    func_802A03D4(0, 0, P);
    ENGINE_BLK(802B0F38);
    func_802A040C(0, 0, P);
    ENGINE_BLK(802B0F4C);
    func_802A0480(0, 0, P, 0.0f);
    ENGINE_BLK(802B0F64);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802B0F78);
    func_8029E558(P, D_803EDFC8, D_803EDFCC);
    ENGINE_BLK(802B0F8C);
    func_802A0320(0, P);
    ENGINE_BLK(802B0F9C);
    func_802A0290(0, 1, P);
    ENGINE_BLK(802B0FB0);
    func_8029E558(P, D_803EDFCC, D_803EDFC8);
    ENGINE_BLK(802B0FC4);
    r = vs->unk78;
    r[0] = -0x64, r[1] = 0, r[2] = 4;
    r[3] = 0, r[4] = 0xA0, r[5] = 4;
    r[6] = 0, r[7] = 0xA0, r[8] = 4;
    r[9] = 0, r[10] = 0xA0, r[11] = 4;
    r[12] = 0, r[13] = 0xA0, r[14] = 4;
    D_803EDFD8 = 0;
    D_803EDFDA = 0;
    D_803EDFD9 = 0;
    func_8029C354(2, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x6590);
    ENGINE_BLK(802B107C);
    func_80258230(2, 0x64, 0x2D, 0x2D);
    ENGINE_BLK(802B1094);
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802B10B4);
    func_802A0290(1, 1, P);
    ENGINE_BLK(802B10C8);
    vs->unk9A = 1;
    func_802B152C();
    ENGINE_BLK(802B10D4);
    vs->unk9A = 0;
    func_802A7764((u32 *)D_803EDFCC, (u32 *)D_803EDFC8, 0x1400);
    ENGINE_BLK(802B10F0);
    func_802AA838(D_803EDFCC, D_803EDFC8, *(s32 *)(D_803EDFC4 + *(s32 *)(D_803EDFC4 + 0x18) + 4));
    ENGINE_BLK(802B1124);
    D_803F7844 = NULL;
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(18, T(&D_803EDFBC));
    ENGINE_LEAVE(22, T(D_803EDFC8));
    ENGINE_LEAVE(23, T(D_803EDFCC));
}

/* hd.c's: whether it can be left: on the ground and walking */
u8 func_802B1150(void) {
    VS *vs = &D_803EDF10;
    u8 r = 0;

    ENGINE_BLK(802B1150);
    engine_save(ENGINE_GPR(28), 0);
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802B1174);
        if (vs->unk96[1] != 1) {
            ENGINE_BLK(802B1184);
            if (vs->unk96[2] != 1) {
                ENGINE_BLK(802B1194);
                if (vs->unkA1 == 0) {
                    ENGINE_BLK(802B11A0);
                    r = 1;
                }
            }
        }
    }
    ENGINE_BLK(802B11A4);
    engine_restore();
    return r;
}

/* hd.c's: the player gets out */
void func_802B11B8(void) {
    VS *vs = &D_803EDF10;

    ENGINE_BLK(802B11B8);
    engine_save(ENGINE_GPR(28), 0);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EDFC8, (u32 *)D_803EDFCC, 0x1400);
    ENGINE_BLK(802B11EC);
    func_802A02E4(0x1F, P);
    ENGINE_BLK(802B11FC);
    func_802C444C();
    ENGINE_BLK(802B1204);
    func_802608C8(D_803EDFD0);
    ENGINE_BLK(802B1210);
    engine_restore();
}

/* hd.c's: the player gets in: its sound, its head and its arms */
void func_802B1228(void) {
    VS *vs = &D_803EDF10;

    ENGINE_BLK(802B1228);
    vs->unk96[3] = 0;
    D_8036444C = 0x1068;
    D_80364450 = 0xBB8;
    func_80260650(D_80367738, 0x50, &D_803EDFD0);
    ENGINE_BLK(802B1278);
    func_802A05D0(D_802C2308, 0x19);
    ENGINE_BLK(802B128C);
    func_802A05F8(D_802C2308, 0);
    ENGINE_BLK(802B129C);
    func_802A0620(D_802C2308, 0);
    ENGINE_BLK(802B12AC);
    func_802A039C(7, 2, P);
    ENGINE_BLK(802B12C0);
    func_802A040C(7, 1, P);
    ENGINE_BLK(802B12D4);
    func_802A0480(7, 1, P, 0.5f);
    ENGINE_BLK(802B12F0);
    func_802A039C(8, 2, P);
    ENGINE_BLK(802B1304);
    func_802A040C(8, 1, P);
    ENGINE_BLK(802B1318);
    func_802A0480(8, 1, P, 0.5f);
    ENGINE_BLK(802B1334);
    func_802A039C(9, 4, P);
    ENGINE_BLK(802B1348);
    func_802A040C(9, 1, P);
    ENGINE_BLK(802B135C);
    func_802A0480(9, 1, P, 0.5f);
    ENGINE_BLK(802B1378);
    func_802A0480(1, 1, P, 0.5f);
    ENGINE_BLK(802B1394);
    func_802A0480(5, 1, P, 0.5f);
    ENGINE_BLK(802B13B0);
    func_802A0480(3, 0, P, 0.0f);
    ENGINE_BLK(802B13C8);
}

/* hd.c's: put back on the ground where it is */
void func_802B13D8(void) {
    VS *vs = &D_803EDF10;

    ENGINE_BLK(802B13D8);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802A9A60(vs->unk52, D_803EDFBC, D_803EDFB8, D_803EDFC0, vs->unk4, &D_803EDFBC, (s16 *)&vs->unk4C, 2, vs,
                  engine_ctx(30));
    ENGINE_BLK(802B1464);
    func_802B2768(vs);
    ENGINE_BLK(802B146C);
    func_802A133C(D_803EDFB8, D_803EDFBC, D_803EDFC0, 2, vs);
    ENGINE_BLK(802B1498);
    engine_restore();
}

/* its light */
void func_802B14E8(void) {
    ENGINE_BLK(802B14E8);
    func_802ABD54(2, D_803EDFB8, D_803EDFBC, D_803EDFC0);
    ENGINE_BLK(802B151C);
}

/* each frame */
void func_802B152C(void) {
    VS *vs = &D_803EDF10;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_BLK(802B152C);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802B14E8();
    ENGINE_BLK(802B1588);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802B1594);
        func_802B18F4(vs);
    }
    ENGINE_BLK(802B159C);
    func_802B2900();
    ENGINE_BLK(802B15A4);
    rate_i = func_802B28B8(vs);
    ENGINE_BLK(802B15AC);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    ENGINE_BLK(802B15C8);
    func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0xC, vs, &t3);
    ENGINE_BLK(802B15D0);
    if (vs->unkA1 == 0) {
        ENGINE_BLK(802B15DC);
        func_802A77D0(vs);
    }
    ENGINE_BLK(802B15E4);
    func_802A7FD8(0x59D8, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 0, vs);
    ENGINE_BLK(802B15FC);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802B1608);
    func_802A843C(&vs->unk76, 0, 2, (s8 *)vs->unk96, vs->unk4, 120.0f, vs);
    ENGINE_BLK(802B161C);
    if (D_803EDFD8 != 0) {
        ENGINE_BLK(802B162C);
        func_802A7070((s16 *)&D_803EDFD4, vs);
    }
    ENGINE_BLK(802B1638);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EDFB8, &D_803EDFC0, rate, &z);
    ENGINE_BLK(802B1650);
    D_803ED40B = 0;
    /* ($s4 and $s7, which func_802A8768 reads too) */
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803EDFB8, &D_803EDFC0, &D_803EDFBC, 2, 0x78, 0x78, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802B1684);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B16AC);
        func_8029E558(P, D_803EDFC8, D_803EDFCC);
        ENGINE_BLK(802B16C0);
    } else {
        ENGINE_BLK(802B16C8);
        func_8029E558(P, D_803EDFCC, D_803EDFC8);
    }
    ENGINE_BLK(802B16DC);
    func_802B2768(vs);
    ENGINE_BLK(802B16E4);
    func_8029A800(D_803EDFB8, D_803EDFBC, D_803EDFC0, D_80305CF0, 0, 0, vs->unk76, 0, 2, vs);
    ENGINE_BLK(802B1728);
    func_8029C52C(2, vs);
    ENGINE_BLK(802B1730);
    func_8029AA10();
    ENGINE_BLK(802B1738);
    D_803F77D0 = P;
    func_802BE77C(2, vs);
    ENGINE_BLK(802B1754);
    if (D_803A7425 == 0) {
        ENGINE_BLK(802B1814);
        D_803EDFD8 = 0;
        D_803EDFDA = 0;
        goto done;
    }
    /* D_803A7425: turned toward the camera's heading; D_803EDFDA when it
       is within 0x190 of it */
    ENGINE_BLK(802B1764);
    func_8029A914(vs);
    ENGINE_BLK(802B176C);
    D_803EDFD8 = 1;
    D_803EDFDA = 0;
    v = func_802A6F6C();
    ENGINE_BLK(802B1784);
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802B1794);
        h += 0xFFF;
    }
    ENGINE_BLK(802B1798);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802B17A4);
        h = -h;
    }
    ENGINE_BLK(802B17A8);
    if (!(h < 0x801)) {
        ENGINE_BLK(802B17B4);
        h = 0xFFF - h;
    }
    ENGINE_BLK(802B17BC);
    if (h < 0x190) {
        ENGINE_BLK(802B17C8);
        D_803EDFDA = 1;
    }
    ENGINE_BLK(802B17D4);
    func_802A70D8(vs);
    ENGINE_BLK(802B17DC);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.25f, vs, &a1);

        ENGINE_BLK(802B17F0);
        D_803EDFD4 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802B1804);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802B180C);
done:
    ENGINE_BLK(802B1824);
    D_803643E0 = D_803EDFB8;
    D_803643E4 = D_803EDFBC;
    D_803643E8 = D_803EDFC0;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 2, vs);
    ENGINE_BLK(802B18A0);
    engine_restore();
}

/* the body's animation (part 0x1F) for the walk to a stop: the legs back
   together, then a random idle (7, 8 or 9) */
static void stand(VS *vs) {
    s32 r;

    ENGINE_BLK(802B19B0);
    if (vs->unkA3 != 0) {
        ENGINE_BLK(802B19BC);
        func_802A0360(7, 0, P, 0.0f);
        ENGINE_BLK(802B19D8);
        if (part(0x1F, NULL, NULL) != 0) {
            ENGINE_BLK(802B19E8);
            ENGINE_BLK(802B19F0);
            func_802A02E4(0x1F, P);
            ENGINE_BLK(802B1A00);
            func_8029F9D4(0x1F, 7, P);
            ENGINE_BLK(802B1A14);
        } else {
            ENGINE_BLK(802B19E8);
            ENGINE_BLK(802B1A1C);
            if (part(1, NULL, NULL) != 0) {
                ENGINE_BLK(802B1A2C);
                ENGINE_BLK(802B1A34);
                func_802A02E4(1, P);
                ENGINE_BLK(802B1A44);
                func_8029F9D4(1, 7, P);
                ENGINE_BLK(802B1A58);
            } else {
                ENGINE_BLK(802B1A2C);
                ENGINE_BLK(802B1A60);
                if (part(5, NULL, NULL) != 0) {
                    ENGINE_BLK(802B1A70);
                    ENGINE_BLK(802B1A78);
                    func_802A02E4(5, P);
                    ENGINE_BLK(802B1A88);
                    func_8029F9D4(5, 7, P);
                    ENGINE_BLK(802B1A9C);
                } else {
                    ENGINE_BLK(802B1A70);
                    ENGINE_BLK(802B1AA4);
                    func_802A0360(1, 0, P, 0.0f);
                    ENGINE_BLK(802B1AC0);
                    func_8029F9D4(1, 7, P);
                }
            }
        }
        ENGINE_BLK(802B1AD4);
        func_802A039C(0x1F, 0x1E, P);
        ENGINE_BLK(802B1AE8);
        func_802A03D4(0x1F, 0, P);
        ENGINE_BLK(802B1AFC);
        func_802A040C(0x1F, 0, P);
        ENGINE_BLK(802B1B10);
        func_802A0290(0x1F, 1, P);
    }
    ENGINE_BLK(802B1B24);
    vs->unkA3 = 0;
    if (part(0x1F, NULL, NULL) == 1) {
        ENGINE_BLK(802B1B3C);
        return;
    }
    ENGINE_BLK(802B1B3C);
    ENGINE_BLK(802B1B48);
    if (part(7, NULL, NULL) == 1) {
        ENGINE_BLK(802B1B58);
        return;
    }
    ENGINE_BLK(802B1B58);
    ENGINE_BLK(802B1B64);
    if (part(8, NULL, NULL) == 1) {
        ENGINE_BLK(802B1B74);
        return;
    }
    ENGINE_BLK(802B1B74);
    ENGINE_BLK(802B1B80);
    if (part(9, NULL, NULL) == 1) {
        ENGINE_BLK(802B1B90);
        return;
    }
    ENGINE_BLK(802B1B90);
    ENGINE_BLK(802B1B9C);
    r = func_8026A8E0(0, 0x1E);
    ENGINE_BLK(802B1BA8);
    if (r != 0)
        return;
    ENGINE_BLK(802B1BB0);
    r = func_8026A8E0(0, 2);
    ENGINE_BLK(802B1BBC);
    if (r == 0) {
        ENGINE_BLK(802B1C3C);
        func_802A0360(7, 0, P, 0.0f);
        ENGINE_BLK(802B1C58);
        func_802A0290(7, 1, P);
        ENGINE_BLK(802B1C6C);
    } else {
        ENGINE_BLK(802B1BC4);
        if (r == 1) {
            ENGINE_BLK(802B1C04);
            func_802A0360(8, 0, P, 0.0f);
            ENGINE_BLK(802B1C20);
            func_802A0290(8, 1, P);
            ENGINE_BLK(802B1C34);
        } else {
            ENGINE_BLK(802B1BCC);
            func_802A0360(9, 0, P, 0.0f);
            ENGINE_BLK(802B1BE8);
            func_802A0290(9, 1, P);
            ENGINE_BLK(802B1BFC);
        }
    }
}

/* walking: the idle animations stopped, the walk started; rolling up with
   a C button or Z at speed 0x96; otherwise a step of the leg in front */
static void walk(VS *vs) {
    s32 s, a0, k;

    ENGINE_BLK(802B1C74);
    if (vs->unkA3 != 1) {
        ENGINE_BLK(802B1C84);
        func_802A0360(1, 0, P, 0.0f);
        ENGINE_BLK(802B1CA0);
        if (part(7, NULL, NULL) != 0) {
            ENGINE_BLK(802B1CB0);
            ENGINE_BLK(802B1CB8);
            func_8029F9D4(7, 1, P);
            ENGINE_BLK(802B1CCC);
            func_802A02E4(7, P);
            ENGINE_BLK(802B1CDC);
        } else {
            ENGINE_BLK(802B1CB0);
            ENGINE_BLK(802B1CE4);
            if (part(8, NULL, NULL) != 0) {
                ENGINE_BLK(802B1CF4);
                ENGINE_BLK(802B1CFC);
                func_8029F9D4(8, 1, P);
                ENGINE_BLK(802B1D10);
                func_802A02E4(8, P);
                ENGINE_BLK(802B1D20);
            } else {
                ENGINE_BLK(802B1CF4);
                ENGINE_BLK(802B1D28);
                if (part(9, NULL, NULL) != 0) {
                    ENGINE_BLK(802B1D38);
                    ENGINE_BLK(802B1D40);
                    func_8029F9D4(9, 1, P);
                    ENGINE_BLK(802B1D54);
                    func_802A02E4(9, P);
                    ENGINE_BLK(802B1D64);
                } else {
                    ENGINE_BLK(802B1D38);
                    ENGINE_BLK(802B1D6C);
                    func_802A0360(7, 0, P, 0.0f);
                    ENGINE_BLK(802B1D88);
                    func_8029F9D4(7, 1, P);
                }
            }
        }
        ENGINE_BLK(802B1D9C);
        func_802A039C(0x1F, 0x28, P);
        ENGINE_BLK(802B1DB0);
        func_802A03D4(0x1F, 0, P);
        ENGINE_BLK(802B1DC4);
        func_802A040C(0x1F, 0, P);
        ENGINE_BLK(802B1DD8);
        func_802A0290(0x1F, 1, P);
    }
    ENGINE_BLK(802B1DEC);
    vs->unkA3 = 1;
    if (part(0x1F, NULL, NULL) == 1) {
        ENGINE_BLK(802B1E04);
        return;
    }
    ENGINE_BLK(802B1E04);
    ENGINE_BLK(802B1E10);
    D_803F7804 = 0;
    if (D_80370C35 == 0) {
        ENGINE_BLK(802B1E28);
        if (D_80370C1C != 0)
            goto roll;
        ENGINE_BLK(802B1E38);
        if (D_80370C1D != 0)
            goto roll;
    }
    ENGINE_BLK(802B1E48);
    if (D_80370C1A != 0)
        goto roll;
    ENGINE_BLK(802B1E58);
    if (D_80370C1B == 0)
        goto step;
roll:
    ENGINE_BLK(802B1E68);
    if (vs->unk76 < 0x96)
        goto step;
    /* rolling up into a ball */
    ENGINE_BLK(802B1E78);
    vs->unkA1 = 1;
    func_802A0360(2, 0, P, 0.0f);
    ENGINE_BLK(802B1E9C);
    if (vs->unkA2 == 0) {
        ENGINE_BLK(802B1EB4);
        func_8029F9D4(1, 2, P);
        ENGINE_BLK(802B1EC8);
        func_802A02E4(1, P);
        ENGINE_BLK(802B1ED8);
    } else {
        ENGINE_BLK(802B1EA8);
        if (vs->unkA2 != 1) {
            ENGINE_BLK(802B1EB0);
            engine_trap(0x802B1EB0);
        }
        ENGINE_BLK(802B1EE0);
        func_8029F9D4(5, 2, P);
        ENGINE_BLK(802B1EF4);
        func_802A02E4(5, P);
    }
    ENGINE_BLK(802B1F04);
    func_802A039C(0x1F, 0x23, P);
    ENGINE_BLK(802B1F18);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802B1F2C);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802B1F40);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802B1F54);
    func_80278EB0(6, 0.1f, 100);
    ENGINE_BLK(802B1F70);
    return;

step:
    /* dust now and then */
    ENGINE_BLK(802B1F78);
    if (((u32)D_803649D8 >> 8 & 0x2F) == 0) {
        ENGINE_BLK(802B1F90);
        func_802A6274(T(D_802C2984), 0xEA60, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    }
    ENGINE_BLK(802B1FB8);
    k = vs->unkA2;
    if (k != 0) {
        ENGINE_BLK(802B1FC4);
        if (k == 1)
            goto leg5;
        ENGINE_BLK(802B1FCC);
        engine_trap(0x802B1FCC);
    }
    for (;;) {
        /* the left leg (part 1) in front */
        ENGINE_BLK(802B1FD0);
        if (part(1, &a0, NULL) != 0) {
            ENGINE_BLK(802B1FE0);
            ENGINE_BLK(802B1FE8);
            s = vs->unk76;
            if (s < 0) {
                ENGINE_BLK(802B1FF4);
                func_802A03D4(1, 1, P);
                ENGINE_BLK(802B2008);
            } else {
                ENGINE_BLK(802B2010);
                func_802A03D4(1, 0, P);
            }
            ENGINE_BLK(802B2024);
            if (s < 0) {
                ENGINE_BLK(802B202C);
                s = -s;
            }
            ENGINE_BLK(802B2030);
            if (s != 0) {
                ENGINE_BLK(802B2038);
                s = (u32)s / 0x18;
            }
            ENGINE_BLK(802B2054);
            func_802A039C(1, s, P);
            ENGINE_BLK(802B2068);
            return;
        }
        ENGINE_BLK(802B1FE0);
        /* its step done: the other leg's */
        ENGINE_BLK(802B2110);
        s = vs->unk76;
        if (s < 0) {
            ENGINE_BLK(802B211C);
            s = -s;
        }
        ENGINE_BLK(802B2120);
        if (!(s < 0x78)) {
            ENGINE_BLK(802B212C);
            if (a0 != 0) {
                ENGINE_BLK(802B2134);
                func_802A0290(5, 2, P);
                ENGINE_BLK(802B2148);
            } else {
                ENGINE_BLK(802B2150);
                func_802A0290(5, 1, P);
            }
            ENGINE_BLK(802B2164);
            func_802A0360(5, 0, P, 0.0f);
            ENGINE_BLK(802B2180);
            vs->unkA2 = 1;
            goto leg5;
        }
        ENGINE_BLK(802B218C);
        if (a0 != 0) {
            ENGINE_BLK(802B2194);
            func_802A0290(1, 2, P);
            ENGINE_BLK(802B21A8);
        } else {
            ENGINE_BLK(802B21B0);
            func_802A0290(1, 1, P);
        }
        ENGINE_BLK(802B21C4);
        func_802A0360(1, 0, P, 0.0f);
        ENGINE_BLK(802B21E0);
        vs->unkA2 = 0;
        continue;

leg5:
        /* the right leg (part 5) in front */
        ENGINE_BLK(802B2070);
        if (part(5, &a0, NULL) != 0) {
            ENGINE_BLK(802B2080);
            ENGINE_BLK(802B2088);
            s = vs->unk76;
            if (s < 0) {
                ENGINE_BLK(802B2094);
                func_802A03D4(5, 1, P);
                ENGINE_BLK(802B20A8);
            } else {
                ENGINE_BLK(802B20B0);
                func_802A03D4(5, 0, P);
            }
            ENGINE_BLK(802B20C4);
            if (s < 0) {
                ENGINE_BLK(802B20CC);
                s = -s;
            }
            ENGINE_BLK(802B20D0);
            if (s != 0) {
                ENGINE_BLK(802B20D8);
                s = (u32)s / 0x18;
            }
            ENGINE_BLK(802B20F4);
            func_802A039C(5, s, P);
            ENGINE_BLK(802B2108);
            return;
        }
        ENGINE_BLK(802B2080);
        ENGINE_BLK(802B2110);
        s = vs->unk76;
        if (s < 0) {
            ENGINE_BLK(802B211C);
            s = -s;
        }
        ENGINE_BLK(802B2120);
        if (!(s < 0x78)) {
            ENGINE_BLK(802B212C);
            if (a0 != 0) {
                ENGINE_BLK(802B2134);
                func_802A0290(5, 2, P);
                ENGINE_BLK(802B2148);
            } else {
                ENGINE_BLK(802B2150);
                func_802A0290(5, 1, P);
            }
            ENGINE_BLK(802B2164);
            func_802A0360(5, 0, P, 0.0f);
            ENGINE_BLK(802B2180);
            vs->unkA2 = 1;
            goto leg5;
        }
        ENGINE_BLK(802B218C);
        if (a0 != 0) {
            ENGINE_BLK(802B2194);
            func_802A0290(1, 2, P);
            ENGINE_BLK(802B21A8);
        } else {
            ENGINE_BLK(802B21B0);
            func_802A0290(1, 1, P);
        }
        ENGINE_BLK(802B21C4);
        func_802A0360(1, 0, P, 0.0f);
        ENGINE_BLK(802B21E0);
        vs->unkA2 = 0;
    }
}

/* the parts and the states (unkA1) */
REGS(gp)
void func_802B18F4(VS *vs) {
    s32 v, a1, t1;

    ENGINE_BLK(802B18F4);
    if (vs->unkA1 != 0) {
        ENGINE_BLK(802B1908);
        if (vs->unkA1 == 1)
            goto rolling;
        ENGINE_BLK(802B1914);
        if (vs->unkA1 == 2)
            goto rolling2;
        ENGINE_BLK(802B191C);
        if (vs->unkA1 == 3)
            goto landing;
        ENGINE_BLK(802B1924);
        if (vs->unkA1 == 4)
            goto landing2;
        ENGINE_BLK(802B192C);
        engine_trap(0x802B192C);
    }
    /* walking: the step sounds (the legs' frames 2 and 6) */
    ENGINE_BLK(802B1930);
    v = part(1, NULL, &t1);
    ENGINE_BLK(802B1940);
    if (v != 1) {
        ENGINE_BLK(802B194C);
        v = part(5, NULL, &t1);
        ENGINE_BLK(802B195C);
        if (v != 1)
            goto moving;
    }
    ENGINE_BLK(802B1968);
    a1 = D_803EDFD6;
    D_803EDFD6 = t1;
    if (t1 != a1) {
        ENGINE_BLK(802B1980);
        if (t1 == 6) {
            ENGINE_BLK(802B1994);
            func_80260650(D_80367738, 0x4E, NULL);
        } else {
            ENGINE_BLK(802B1988);
            if (t1 == 2) {
                ENGINE_BLK(802B1994);
                func_80260650(D_80367738, 0x4F, NULL);
            }
        }
    }
moving:
    ENGINE_BLK(802B19A4);
    if (vs->unk76 == 0)
        stand(vs);
    else
        walk(vs);
    goto done;

rolling:
    /* rolling: the sound; into a building at speed, the landing; slowed
       down (or the C button let go), standing up */
    ENGINE_BLK(802B21EC);
    if (D_803F7844 == NULL) {
        ENGINE_BLK(802B21FC);
        func_80260650(D_80367738, 0x51, &D_803F7844);
    }
    ENGINE_BLK(802B2214);
    if (vs->unk9C != 0)
        goto hit1;
    ENGINE_BLK(802B2220);
    if (vs->unk9D != 0)
        goto up1;
    ENGINE_BLK(802B222C);
    if (D_803EDFDA != 0)
        goto up1;
    ENGINE_BLK(802B223C);
    vs->unk76 = 0x1BE;
    if (part(0x1F, NULL, NULL) == 1) {
        ENGINE_BLK(802B2254);
        goto done;
    }
    ENGINE_BLK(802B2254);
    ENGINE_BLK(802B2260);
    func_802A039C(2, 0xA, P);
    ENGINE_BLK(802B2274);
    func_802A03D4(2, 0, P);
    ENGINE_BLK(802B2288);
    func_802A040C(2, 0, P);
    ENGINE_BLK(802B229C);
    func_802A0290(2, 1, P);
    ENGINE_BLK(802B22B0);
    vs->unkA1 = 2;
    goto done;
hit1:
    ENGINE_BLK(802B22BC);
    vs->unk76 = vs->unk76 >> 1;
    func_802794A4();
    ENGINE_BLK(802B22CC);
    vs->unkA1 = 3;
    func_802A0360(3, 3, P, 0.0f);
    ENGINE_BLK(802B22F0);
    func_8029F9D4(0x1F, 3, P);
    ENGINE_BLK(802B2304);
    func_802A039C(0x1F, 0x21, P);
    ENGINE_BLK(802B2318);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802B232C);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802B2340);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802B2354);
    D_803F7804 = 1;
    goto done;
up1:
    ENGINE_BLK(802B2364);
    if (vs->unk76 >= 0) {
        ENGINE_BLK(802B2370);
        vs->unk76 = 0x3C;
    }
    ENGINE_BLK(802B2378);
    func_802C444C();
    ENGINE_BLK(802B2380);
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802B239C);
    func_8029F9D4(0x1F, 1, P);
    ENGINE_BLK(802B23B0);
    func_802A039C(0x1F, 0x32, P);
    ENGINE_BLK(802B23C4);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802B23D8);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802B23EC);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802B2400);
    func_802794A4();
    ENGINE_BLK(802B2408);
    vs->unkA1 = 0;
    goto done;

rolling2:
    ENGINE_BLK(802B2414);
    if (vs->unk9C != 0)
        goto hit2;
    ENGINE_BLK(802B2420);
    if (vs->unk9D != 0)
        goto up2;
    ENGINE_BLK(802B242C);
    if (D_803EDFDA != 0)
        goto up2;
    ENGINE_BLK(802B243C);
    vs->unk76 = 0x1BE;
    if (part(2, NULL, NULL) == 1) {
        ENGINE_BLK(802B2454);
        goto done;
    }
    ENGINE_BLK(802B2454);
up2:
    ENGINE_BLK(802B2460);
    func_802C444C();
    ENGINE_BLK(802B2468);
    if (vs->unk76 >= 0) {
        ENGINE_BLK(802B2474);
        vs->unk76 = 0x3C;
    }
    ENGINE_BLK(802B247C);
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802B2498);
    func_802A02E4(2, P);
    ENGINE_BLK(802B24A8);
    func_8029F9D4(2, 1, P);
    ENGINE_BLK(802B24BC);
    func_802A039C(0x1F, 0x32, P);
    ENGINE_BLK(802B24D0);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802B24E4);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802B24F8);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802B250C);
    func_802794A4();
    ENGINE_BLK(802B2514);
    vs->unkA1 = 0;
    goto done;
hit2:
    ENGINE_BLK(802B2520);
    vs->unk76 = vs->unk76 >> 1;
    vs->unkA1 = 3;
    func_802A0360(3, 3, P, 0.0f);
    ENGINE_BLK(802B2550);
    func_8029F9D4(2, 3, P);
    ENGINE_BLK(802B2564);
    func_802A02E4(2, P);
    ENGINE_BLK(802B2574);
    func_802A039C(0x1F, 0x21, P);
    ENGINE_BLK(802B2588);
    func_802A03D4(0x1F, 0, P);
    ENGINE_BLK(802B259C);
    func_802A040C(0x1F, 0, P);
    ENGINE_BLK(802B25B0);
    func_802A0290(0x1F, 1, P);
    ENGINE_BLK(802B25C4);
    D_803F7804 = 1;
    goto done;

landing:
    /* landing: the crash's sound and dust, then (part 0x1F's frame done)
       part 3's animation, state 4 */
    ENGINE_BLK(802B25D4);
    if (D_803F7844 != NULL) {
        ENGINE_BLK(802B25E4);
        func_802C444C();
        ENGINE_BLK(802B25EC);
        func_80260650(D_80367738, 0x4B, NULL);
    }
    ENGINE_BLK(802B2600);
    func_802A6274(T(D_802C2984), 0x222E0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    ENGINE_BLK(802B262C);
    func_802BCC10();
    ENGINE_BLK(802B2634);
    if (part(0x1F, NULL, NULL) == 1) {
        ENGINE_BLK(802B2644);
        goto done;
    }
    ENGINE_BLK(802B2644);
    ENGINE_BLK(802B2650);
    D_803F7804 = 0;
    func_802794A4();
    ENGINE_BLK(802B2658);
    func_802A039C(3, 8, P);
    ENGINE_BLK(802B266C);
    func_802A03D4(3, 0, P);
    ENGINE_BLK(802B2680);
    func_802A040C(3, 0, P);
    ENGINE_BLK(802B2694);
    func_802A0290(3, 1, P);
    ENGINE_BLK(802B26A8);
    vs->unkA1 = 4;
    goto done;

landing2:
    ENGINE_BLK(802B26B4);
    func_802A6274(T(D_802C2984), 0x222E0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    ENGINE_BLK(802B26E0);
    func_802BCC10();
    ENGINE_BLK(802B26E8);
    if (part(3, NULL, NULL) == 1) {
        ENGINE_BLK(802B26F8);
        goto done;
    }
    ENGINE_BLK(802B26F8);
    ENGINE_BLK(802B2704);
    D_803F7804 = 1;
    func_802A0360(1, 0, P, 0.0f);
    ENGINE_BLK(802B272C);
    func_802A0360(5, 0, P, 0.0f);
    ENGINE_BLK(802B2748);
    vs->unkA1 = 0;
done:
    ENGINE_BLK(802B2754);
}

/* Thunderfist's matrix (turned a quarter), its vertices and its collision */
REGS(gp)
void func_802B2768(VS *vs) {
    u8 *model = D_803EDFC4, *buf;
    s32 *m, off, h;

    ENGINE_BLK(802B2768);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B2798);
        m = (s32 *)(D_803EDFC8 + off);
    } else {
        ENGINE_BLK(802B27AC);
        m = (s32 *)(D_803EDFCC + off);
    }
    ENGINE_BLK(802B27BC);
    h = (u16)vs->unk4C + 0x400;
    if (!(h < 0x1000)) {
        ENGINE_BLK(802B27F0);
        h -= 0xFFF;
    }
    ENGINE_BLK(802B27F4);
    D_803ED390[1] = h;
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    func_802AA764(D_803EDFB8, D_803EDFBC, D_803EDFC0, 0x6590, m);
    ENGINE_LEAVE(18, T(m));           /* (its $s2, as the glue would) */
    ENGINE_BLK(802B2810);
    if (D_8035805C != 0) {
        ENGINE_BLK(802B2824);
        buf = D_803EDFC8;
    } else {
        ENGINE_BLK(802B2834);
        buf = D_803EDFCC;
    }
    ENGINE_BLK(802B2840);
    model = D_803EDFC4;
    func_8029C454(D_803EDFB8, D_803EDFBC, D_803EDFC0, 2, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802B2888);
    func_802ABBEC(2, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802B28A8);
}

/* the turn rate: none standing, 0x32 rolling, else 0x78 */
REGS(gp -> s3)
s32 func_802B28B8(VS *vs) {
    s32 r = 0;

    ENGINE_BLK(802B28B8);
    if (vs->unk76 != 0) {
        ENGINE_BLK(802B28CC);
        if (vs->unkA1 == 2)
            goto rolling;
        ENGINE_BLK(802B28DC);
        if (vs->unkA1 == 1)
            goto rolling;
        ENGINE_BLK(802B28E4);
        r = 0x78;
        goto done;
    rolling:
        ENGINE_BLK(802B28EC);
        r = 0x32;
    }
done:
    ENGINE_BLK(802B28F0);
    return r;
}

/* the camera's distance and speed for Thunderfist */
void func_802B2900(void) {
    ENGINE_BLK(802B2900);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x28;
    D_803ED3F7 = 3;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B295C(u8 *dst) {
    ENGINE_BLK(802B295C);
    func_802AC7DC(dst, (u8 *)&D_803EDF10, (u32 *)&D_803EDFB8);
    ENGINE_BLK(802B2978);
}

/* and back */
void func_802B2988(u8 *src) {
    ENGINE_BLK(802B2988);
    func_802AC85C(src, (u8 *)&D_803EDF10, (u32 *)&D_803EDFB8);
    ENGINE_BLK(802B29A4);
}
