/*
 * hd_code 69BB0 (us.v11 0x802AE370-0x802AFC50): the driver on foot (type
 * 0), as native C (engine.h).  The driver's parts are D_803ED460, its
 * state D_803ED760 and its position D_803ED808..10 (vehicle.h).
 *
 * func_802AE370 sets it up (from the level loader), func_802AEEC8 runs it
 * each frame (from hd.c, and at the end of the setup); func_802AE888 gets
 * the driver out of a vehicle, at the first clear spot beside it
 * (func_802AEC3C), and func_802AF340 walks it out from there.
 *
 * Its shadow (func_802AF340) passes func_802582C4 three stack arguments it
 * never stores: two are the halves of the return address its own frame
 * saved (RA_SHADOW), the third whatever was below that frame: the word
 * SHADOW_SLOT below the N64 $sp the game's C runs the engine at, which the
 * last native frame that reached so deep left there (62740's
 * func_802ABBEC under the chopper, 62740_carry's func_802AB714, or
 * func_802AEC3C here; docs/PORT.md).  The native code here keeps no other
 * frame.
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"

extern Part D_803ED460[32];
extern VS D_803ED760;
extern s32 D_803ED814;                          /* the coordinate it walks out to */
extern s32 D_803ED81C;                          /* the ground's height under it */
extern s16 D_803ED820;                          /* part 1's frame, last time */
extern u16 D_803ED822;                          /* the heading it turns to (with D_803A7425) */
extern s8 D_803ED824;                           /* turning to D_803ED822 */
extern u8 D_803ED825;                           /* it can get out */
extern u8 D_803F7812;                           /* set on getting out (not by us.v10) */
extern u8 D_803ED827;                           /* getting out (func_802AE888) */
extern u8 D_803ED828;                           /* the side it gets out on: 0 +z, 1 -z, 2 +x, 3 -x */
extern u8 *PTR32 D_803ED82C;                    /* two 0xC80-byte buffers, one per frame */
extern u8 *PTR32 D_803ED830;
extern s32 D_803ED3A8[];

extern u8 D_80305CB0[];                         /* the driver's bounce records */
extern s8 D_80305CB1[];                         /* where to get out of a vehicle: level, vehicle, side, distance, a test */
extern u8 D_803ED40B;
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425;
extern Part *PTR32 D_803F77D0;
extern s32 D_803643E4, D_803643E8;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
void func_802582C4(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
s32 func_8026A8E0(s32 lo, s32 hi);
void func_8028F994(s32 x, s32 y, s32 z);
REGS(t3, t6, t7, s0, s1, s2, s3, s4, gp -> t2, t3, s3)
s32 func_802A7834(s32 step, s16 *speed, s32 mode, u8 *flags, s16 *rows, s32 brake, s32 rate, u16 *h, VS *vs,
                  s32 *step_out, s32 *rate_out);

void func_802AEEC8(void);
static void driver_frame(void);
REGS()
void func_802AEE84(void);
REGS(gp)
void func_802AF340(VS *vs);
REGS(gp)
void func_802AF4BC(VS *vs);
REGS(gp)
void func_802AFA64(VS *vs);
REGS(-> s3)
s32 func_802AFB84(void);
REGS()
void func_802AFBA0(void);
REGS(a3 -> a1)
s32 func_802AEB9C(s32 test);
REGS(a2, gp -> a3)
s32 func_802AEC3C(s32 d, VS *vs);

#define T(p) ((s32)(p))
#define DRV D_803ED460
#define MODEL ((u8 *)D_803ED818)

/* where the shadow's frame saved its return address, in func_802AEEC8:
   each version's own (the low half of the shadow's pitch; the high half
   is its sign extension) */
#if defined(VERSION_US_V10)
#define RA_SHADOW 0x802AEEBC
#elif defined(VERSION_JP)
#define RA_SHADOW 0x802AF2C0
#elif defined(VERSION_EU)
#define RA_SHADOW 0x802B18C0
#else
#define RA_SHADOW 0x802AEF50
#endif

/* the word below the game C's N64 $sp the shadow's third stack argument
   is (its frames in the original: func_802AEEC8's 0x10, driver_frame's
   0x58 + 0x30 + 0x10, its own 0x10 + 0x18, read at 0x14) */
#define SHADOW_SLOT ((u32)-0xBC)

#define SAVED_AEC3C (ENGINE_GPR(2) | ENGINE_GPR(3) | ENGINE_GPR(4) | ENGINE_GPR(6) | 0xFFu << 8 | ENGINE_GPR(24) | \
                     ENGINE_GPR(25) | ENGINE_GPR(28))

/* part i's state: func_802A04BC's first result (and its sixth, the
   frame) */
static s32 part_state(s32 i, Part *parts, s32 *f13) {
    s32 f11, f12, f14, fC, fE, t1;
    f32 f4;
    s32 r = func_802A04BC(i, parts, &f11, &f12, &f14, &fC, &fE, &t1, &f4);

    if (f13)
        *f13 = t1;
    return r;
}

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802AE370(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803ED760;
    u8 *buf;
    s16 *r;
    s32 avg;

    ENGINE_BLK(802AE370);
    engine_save(ENGINE_T0_T5, 0);
    D_803ED818 = (VehicleModel *)model;
    buf = D_80358070;
    D_803ED82C = buf;
    D_803ED830 = buf + 0xC80;
    D_80358070 = buf + 0x1900;
    func_802A1388(0, 0, D_803ED82C, D_803ED830, model);
    ENGINE_BLK(802AE3F0);
    func_802A754C(vs);
    ENGINE_BLK(802AE3FC);
    vs->unk52[0] = 0x1E;
    vs->unk52[1] = 0x1E;
    vs->unk52[2] = -0x1E;
    vs->unk52[3] = 0x1E;
    vs->unk52[4] = 0x1E;
    vs->unk52[5] = -0x1E;
    vs->unk5E[0] = 0x23;
    vs->unk5E[1] = 0x23;
    vs->unk5E[2] = -0x23;
    vs->unk5E[3] = 0x23;
    vs->unk5E[4] = 0x23;
    vs->unk5E[5] = -0x23;
    D_803ED808 = x;
    D_803ED80C = y;
    D_803ED810 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    D_803ED825 = 1;
    D_803ED826 = 0;
    D_803ED827 = 0;
    D_803ED824 = 0;
    vs->unkA1 = 0;
    func_802A992C(vs->unk52, D_803ED80C, x, z, vs->unk4, &D_803ED80C, (s16 *)&vs->unk4C, 0, vs, engine_ctx(30),
                  &avg);
    ENGINE_BLK(802AE4CC);
    func_8029F85C(DRV, MODEL, D_803ED82C, D_803ED830);
    ENGINE_BLK(802AE508);
    func_802A039C(0, 100, DRV);
    ENGINE_BLK(802AE51C);
    func_802A03D4(0, 0, DRV);
    ENGINE_BLK(802AE530);
    func_802A040C(0, 0, DRV);
    ENGINE_BLK(802AE544);
    func_802A0480(0, 0, DRV, 0.0f);
    ENGINE_BLK(802AE55C);
    func_802A0290(0, 1, DRV);
    ENGINE_BLK(802AE570);
    func_8029E558(DRV, D_803ED82C, D_803ED830);
    ENGINE_BLK(802AE584);
    func_802A0320(0, DRV);
    ENGINE_BLK(802AE594);
    func_802A0290(0, 1, DRV);
    ENGINE_BLK(802AE5A8);
    func_8029E558(DRV, D_803ED830, D_803ED82C);
    ENGINE_BLK(802AE5BC);
    r = vs->unk78;
    r[0] = -0x28, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0x28, r[5] = 2;
    r[6] = 0x28, r[7] = 0x3C, r[8] = 2;
    r[9] = 0x3C, r[10] = 0x50, r[11] = 2;
    r[12] = 0x50, r[13] = 0x64, r[14] = 2;
    model = MODEL;
    func_8029C354(0, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x4E20);
    ENGINE_BLK(802AE65C);
    func_80258230(0, 0x28, 0xF, 0xF);
    ENGINE_BLK(802AE674);
    vs->unk9A = 1;
    driver_frame();
    ENGINE_BLK(802AE684);
    vs->unk9A = 0;
    model = MODEL;
    func_802AA838(D_803ED830, D_803ED82C, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802AE6BC);
    func_802A039C(1, 0, DRV);
    ENGINE_BLK(802AE6D0);
    func_802A03D4(1, 0, DRV);
    ENGINE_BLK(802AE6E4);
    func_802A040C(1, 0, DRV);
    ENGINE_BLK(802AE6F8);
    func_802A0480(1, 1, DRV, 0.5f);
    ENGINE_BLK(802AE714);
    func_802A039C(2, 2, DRV);
    ENGINE_BLK(802AE728);
    func_802A03D4(2, 0, DRV);
    ENGINE_BLK(802AE73C);
    func_802A040C(2, 1, DRV);
    ENGINE_BLK(802AE750);
    func_802A0480(2, 1, DRV, 0.5f);
    ENGINE_BLK(802AE76C);
    func_802A039C(3, 2, DRV);
    ENGINE_BLK(802AE780);
    func_802A03D4(3, 0, DRV);
    ENGINE_BLK(802AE794);
    func_802A040C(3, 1, DRV);
    ENGINE_BLK(802AE7A8);
    func_802A0480(3, 1, DRV, 0.5f);
    ENGINE_BLK(802AE7C4);
    func_802A039C(4, 2, DRV);
    ENGINE_BLK(802AE7D8);
    func_802A03D4(4, 0, DRV);
    ENGINE_BLK(802AE7EC);
    func_802A040C(4, 1, DRV);
    ENGINE_BLK(802AE800);
    func_802A0480(4, 1, DRV, 0.5f);
    ENGINE_BLK(802AE81C);
    D_8036444C = 0xD48;
    D_80364450 = 0;
    engine_restore();
}

/* hd.c's: part 0x1F (the run) stopped */
void func_802AE860(void) {
    ENGINE_BLK(802AE860);
    func_802A02E4(0x1F, DRV);
    ENGINE_BLK(802AE878);
}

/* hd.c's: the player gets out of the vehicle at distance `dist` (times the
   table's): at the first clear spot beside it, walking out from there;
   whether it found one */
/* us.v10's sets no D_803F7812 first, so its blocks start 0xC earlier */
#ifdef VERSION_US_V10
#define BLK_V10(v11, v10) ENGINE_BLK(v10)
#else
#define BLK_V10(v11, v10) ENGINE_BLK(v11)
#endif

u8 func_802AE888(s32 dist) {
    VS *vs = &D_803ED760;
    s8 *t = D_80305CB1;
    s32 side = 0, mul = 1, d, r = 0, h;

    ENGINE_BLK(802AE888);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
#ifndef VERSION_US_V10
    D_803F7812 = 1;
#endif
    if (D_803ED825 == 0)
        goto fail;
    /* the side and distance for this level's vehicle (D_80305CB1) */
    BLK_V10(802AE8F4, 802AE8E8);
    for (;; t += 5) {
        BLK_V10(802AE914, 802AE908);
        if (t[0] == -1)
            goto found;
        BLK_V10(802AE924, 802AE918);
        if (t[0] != D_802E8BDC)
            continue;
        BLK_V10(802AE92C, 802AE920);
        if ((u8)t[1] != D_80364456)
            continue;
        BLK_V10(802AE938, 802AE92C);
        if (t[4] != 0) {
            BLK_V10(802AE944, 802AE938);
            if (func_802AEB9C((u8)t[4]) == 0) {
                BLK_V10(802AE94C, 802AE940);
                continue;
            }
            BLK_V10(802AE94C, 802AE940);
        }
        BLK_V10(802AE954, 802AE948);
        side = (u8)t[2];
        mul = (u8)t[3];
        break;
    }
found:
    BLK_V10(802AE95C, 802AE950);
    dist = (u32)dist * (u32)mul;
    D_803ED828 = side;
    D_803ED827 = 1;
    D_803ED81C = D_803643E4;
    /* (what func_802AEC3C's frame saves) */
    /* the spots out to `dist`, every 100, then `dist` itself */
    for (d = 0;; d += 0x64) {
        BLK_V10(802AE994, 802AE988);
        if (dist < d)
            break;
        BLK_V10(802AE9A0, 802AE994);
        r = func_802AEC3C(d, vs);
        BLK_V10(802AE9A8, 802AE99C);
        if (r == 0)
            goto fail;
        BLK_V10(802AE9B0, 802AE9A4);
    }
    BLK_V10(802AE9B8, 802AE9AC);
    r = func_802AEC3C(dist, vs);
    BLK_V10(802AE9C0, 802AE9B4);
    if (r == 0)
        goto fail;
    BLK_V10(802AE9C8, 802AE9BC);
    vs->unk96[0] = 0, vs->unk96[1] = 0, vs->unk96[2] = 0;
    vs->unk96[3] = 0;
    vs->unk76 = 0;
    side = D_803ED828;
    if (side == 0) {
        BLK_V10(802AEA08, 802AE9FC);
        h = 0;
    } else {
        BLK_V10(802AE9F0, 802AE9E4);
        if (side == 1) {
            BLK_V10(802AEA10, 802AEA04);
            h = 0x800;
        } else {
            BLK_V10(802AE9F8, 802AE9EC);
            if (side == 2) {
                BLK_V10(802AEA18, 802AEA0C);
                h = 0x400;
            } else {
                BLK_V10(802AEA00, 802AE9F4);
                h = 0xC00;
            }
        }
    }
    BLK_V10(802AEA1C, 802AEA10);
    vs->unk4E = h;
    vs->unk4C = h;
    vs->unk74 = h;
    side = D_803ED828;
    if (side != 0) {
        BLK_V10(802AEA38, 802AEA2C);
        if (side != 1) {
            BLK_V10(802AEA40, 802AEA34);
            h = D_803ED808;
            goto spot;
        }
    }
    BLK_V10(802AEA4C, 802AEA40);
    h = D_803ED810;
spot:
    /* it walks out from where the vehicle is */
    BLK_V10(802AEA54, 802AEA48);
    D_803ED814 = h;
    D_803ED808 = D_803643E0;
    h = D_803643E4;
    if (D_80364456 != 0xB) {
        BLK_V10(802AEA84, 802AEA78);
        if (D_80364456 != 0x11) {
            BLK_V10(802AEA90, 802AEA84);
            if (D_80364456 != 0x12)
                goto y;
        }
    }
    BLK_V10(802AEA98, 802AEA8C);
    h += 0x1F4;
y:
    BLK_V10(802AEA9C, 802AEA90);
    D_803ED80C = h;
    D_803ED810 = D_803643E8;
    D_803ED826 = 1;
    func_802A039C(1, 0, DRV);
    BLK_V10(802AEAD4, 802AEAC8);
    func_802A03D4(1, 0, DRV);
    BLK_V10(802AEAE8, 802AEADC);
    func_802A040C(1, 0, DRV);
    BLK_V10(802AEAFC, 802AEAF0);
    func_802A0290(1, -1, DRV);
    BLK_V10(802AEB10, 802AEB04);
    D_8036444C = 0xD48;
    D_80364450 = 0;
    r = 1;
    goto done;
fail:
    BLK_V10(802AEB38, 802AEB2C);
    r = 0;
done:
    BLK_V10(802AEB3C, 802AEB30);
    D_803ED827 = 0;
    engine_restore();
    return r;
}

/* the table's test for a spot (D_80305CB1's fifth byte): 1, the player's z
   (>> 5) in 0xCCD..0xDB4; 2, its x in 0x834..0x960 and z in 0x4B0..0x5DC */
REGS(a3 -> a1)
s32 func_802AEB9C(s32 test) {
    s32 a1 = 0, v;

    ENGINE_BLK(802AEB9C);
    if (test == 1) {
        ENGINE_BLK(802AEBB0);
        v = D_803643E8 >> 5;
        if (v < 0xCCD)
            goto done;
        ENGINE_BLK(802AEBC8);
        if (!(v < 0xDB5))
            goto done;
        ENGINE_BLK(802AEBD0);
        a1 = 1;
        goto done;
    }
    ENGINE_BLK(802AEBD8);
    if (test != 2)
        goto done;
    ENGINE_BLK(802AEBE4);
    v = D_803643E0 >> 5;
    if (v < 0x834)
        goto done;
    ENGINE_BLK(802AEBFC);
    if (!(v < 0x961))
        goto done;
    ENGINE_BLK(802AEC04);
    v = D_803643E8 >> 5;
    if (v < 0x4B0)
        goto done;
    ENGINE_BLK(802AEC1C);
    if (!(v < 0x5DD))
        goto done;
    ENGINE_BLK(802AEC24);
    a1 = 1;
done:
    ENGINE_BLK(802AEC2C);
    return a1;
}

/* the spot `d` from the vehicle on the side D_803ED828 says: the driver
   put there on the ground, and whether nothing is in the way (the
   original saves and loads back $v0, $v1, $a0, $a2, $t0..$t9 and $gp) */
REGS(a2, gp -> a3)
s32 func_802AEC3C(s32 d, VS *vs) {
    s32 side = D_803ED828, r;

    ENGINE_BLK(802AEC3C);
    engine_save(SAVED_AEC3C, 0);
    /* (the original's frame saves $t6 at the shadow's slot: the game's C
       calls func_802AE888 at the depth it calls func_802AEEC8) */
    engine_frame_sw(SHADOW_SLOT, engine_ctx(14));
    if (side == 0) {
        ENGINE_BLK(802AECF4);
        D_803ED808 = D_803643E0;
        D_803ED810 = D_803643E8 + d;
    } else {
        ENGINE_BLK(802AEC90);
        if (side == 1) {
            ENGINE_BLK(802AED1C);
            D_803ED808 = D_803643E0;
            D_803ED810 = D_803643E8 - d;
        } else {
            ENGINE_BLK(802AEC9C);
            if (side == 2) {
                ENGINE_BLK(802AECCC);
                D_803ED808 = D_803643E0 + d;
                D_803ED810 = D_803643E8;
            } else {
                ENGINE_BLK(802AECA4);
                D_803ED808 = D_803643E0 - d;
                D_803ED810 = D_803643E8;
            }
        }
    }
    ENGINE_BLK(802AED40);
    func_802A9A60(vs->unk52, D_803ED81C, D_803ED808, D_803ED810, vs->unk4, &D_803ED80C, (s16 *)&vs->unk4C,
                  engine_ctx(24), vs, engine_ctx(30));
    ENGINE_BLK(802AED60);
    func_802AFA64(vs);
    ENGINE_BLK(802AED68);
    D_803ED81C = (u32)(vs->unk4[3] + vs->unk4[6]) >> 1;
    func_8029A800(D_803ED808, D_803ED80C, D_803ED810, D_80305CB0, 0, 0, 0, 0, 0, vs);
    ENGINE_BLK(802AEDC8);
    func_8029AA10();
    ENGINE_BLK(802AEDD0);
    D_803F77D0 = DRV;
    func_802BE77C(0, vs);
    ENGINE_BLK(802AEDEC);
    if (D_80364456 != 0xB) {
        ENGINE_BLK(802AEE00);
        func_8028F994(D_803ED808, D_803ED80C, D_803ED810);
    }
    ENGINE_BLK(802AEE1C);
    if (D_803A7424 != 0) {
        ENGINE_BLK(802AEE2C);
        r = 0;
    } else {
        ENGINE_BLK(802AEE34);
        r = 1;
    }
    ENGINE_BLK(802AEE38);
    engine_restore();
    return r;
}

/* its light */
REGS()
void func_802AEE84(void) {
    ENGINE_BLK(802AEE84);
    func_802ABD54(0, D_803ED808, D_803ED80C, D_803ED810);
    ENGINE_BLK(802AEEB8);
}

/* hd.c's: each frame */
void func_802AEEC8(void) {
    driver_frame();
}

/* each frame: its walk, or getting out (func_802AF340) */
static void driver_frame(void) {
    VS *vs = &D_803ED760;
    s32 t3 = 0, x, z, rate_i, v, h;
    f32 rate;
    u64 mode;

    ENGINE_BLK(802AEEC8);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802AEE84();
    ENGINE_BLK(802AEF24);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802AEF30);
        func_802AF4BC(vs);
        ENGINE_BLK(802AEF38);
        if (D_803ED826 != 0) {
            ENGINE_BLK(802AEF48);
            func_802AF340(vs);
            ENGINE_BLK(802AEF50);
            goto parts;
        }
    }
    ENGINE_BLK(802AEF58);
    func_802AFBA0();
    ENGINE_BLK(802AEF60);
    rate_i = func_802AFB84();
    ENGINE_BLK(802AEF68);
    mode = D_80364A98;
    if (mode == 0) {
        ENGINE_BLK(802AEF78);
        mode = D_80364A90;
    }
    ENGINE_BLK(802AEF80);
    if (mode == 0x800)
        goto parts;
    ENGINE_BLK(802AEF8C);
    if (mode == 1)
        goto parts;
    ENGINE_BLK(802AEF94);
    if (mode == 0x1000)
        goto parts;
    ENGINE_BLK(802AEF9C);
    func_802A7834(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 6, rate_i, &vs->unk4C, vs, &t3, &rate_i);
    ENGINE_BLK(802AEFB8);
    if (vs->unkA1 == 1) {
        ENGINE_BLK(802AEFC8);
        func_802A77D0(vs);
    }
    ENGINE_BLK(802AEFD0);
    func_802A7FD8(0x2328, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 0, vs);
    ENGINE_BLK(802AEFE8);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    ENGINE_BLK(802AEFF4);
    func_802A843C(&vs->unk76, 1, 0, (s8 *)vs->unk96, vs->unk4, 60.0f, vs);
    ENGINE_BLK(802AF008);
    if (D_803ED824 != 0) {
        ENGINE_BLK(802AF018);
        func_802A7070((s16 *)&D_803ED822, vs);
    }
    ENGINE_BLK(802AF024);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803ED808, &D_803ED810, rate, &z);
    ENGINE_BLK(802AF03C);
    D_803ED40B = 0;
    /* ($s4, $s7, $t4, which func_802A8768 reads too) */
    ENGINE_LEAVE(12, vs->unk4E);
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803ED808, &D_803ED810, &D_803ED80C, 0, 0x3C, 0x3C, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
parts:
    ENGINE_BLK(802AF070);
    if (D_8035805C != 0) {
        ENGINE_BLK(802AF098);
        func_8029E558(DRV, D_803ED82C, D_803ED830);
        ENGINE_BLK(802AF0AC);
    } else {
        ENGINE_BLK(802AF0B4);
        func_8029E558(DRV, D_803ED830, D_803ED82C);
    }
    ENGINE_BLK(802AF0C8);
    func_802AFA64(vs);
    ENGINE_BLK(802AF0D0);
    if (D_803ED826 != 0)
        goto done;
    ENGINE_BLK(802AF0E0);
    mode = D_80364A98;
    if (mode == 0) {
        ENGINE_BLK(802AF0F0);
        mode = D_80364A90;
    }
    ENGINE_BLK(802AF0F8);
    if (mode & 0x1801)
        goto still;
    ENGINE_BLK(802AF104);
    func_8029A800(D_803ED808, D_803ED80C, D_803ED810, D_80305CB0, 0, 0, vs->unk76, 0, 0, vs);
    ENGINE_BLK(802AF148);
    func_8029C52C(0, vs);
    ENGINE_BLK(802AF150);
    func_8029AA10();
    ENGINE_BLK(802AF158);
    D_803F77D0 = DRV;
    func_802BE77C(0, vs);
    ENGINE_BLK(802AF174);
    if (D_803A7425 == 0)
        goto still;
    /* turned toward the camera's heading */
    ENGINE_BLK(802AF184);
    func_8029A914(vs);
    ENGINE_BLK(802AF18C);
    D_803ED824 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802AF19C);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802AF1AC);
        h += 0xFFF;
    }
    ENGINE_BLK(802AF1B0);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802AF1BC);
        h = -h;
    }
    ENGINE_BLK(802AF1C0);
    if (!(h < 0x801)) {
        ENGINE_BLK(802AF1CC);
    }
    ENGINE_BLK(802AF1D4);
    ENGINE_BLK(802AF228);
    func_802A70D8(vs);
    ENGINE_BLK(802AF230);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 1.0f, vs, &a1);

        ENGINE_BLK(802AF244);
        D_803ED822 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802AF258);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802AF260);
    goto done;
still:
    ENGINE_BLK(802AF268);
    D_803ED824 = 0;
done:
    ENGINE_BLK(802AF270);
    D_803643E0 = D_803ED808;
    D_803643E4 = D_803ED80C;
    D_803643E8 = D_803ED810;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0, vs);
    ENGINE_BLK(802AF2EC);
    engine_restore();
}

/* getting out: a step of 100 toward the side, until it is past
   D_803ED814; and its shadow */
REGS(gp)
void func_802AF340(VS *vs) {
    s32 x, z, side, v, y2;

    ENGINE_BLK(802AF340);
    vs->unk76 = 0x50;
    side = D_803ED828;
    x = D_803ED808;
    z = D_803ED810;
    if (side == 0) {
        ENGINE_BLK(802AF3A4);
        z += 0x64;
    } else {
        ENGINE_BLK(802AF378);
        if (side == 1) {
            ENGINE_BLK(802AF39C);
            z -= 0x64;
        } else {
            ENGINE_BLK(802AF384);
            if (side == 2) {
                ENGINE_BLK(802AF394);
                x += 0x64;
            } else {
                ENGINE_BLK(802AF38C);
                x -= 0x64;
            }
        }
    }
    ENGINE_BLK(802AF3A8);
    D_803ED808 = x;
    D_803ED810 = z;
    func_802A9A60(vs->unk52, D_803ED80C, x, z, vs->unk4, &D_803ED80C, (s16 *)&vs->unk4C, engine_ctx(24), vs,
                  engine_ctx(30));
    ENGINE_BLK(802AF3CC);
    side = D_803ED828;
    v = D_803ED814;
    if (side == 0) {
        ENGINE_BLK(802AF448);
        if (D_803ED810 < v)
            goto shadow;
        goto out;
    }
    ENGINE_BLK(802AF3E4);
    if (side == 1) {
        ENGINE_BLK(802AF42C);
        if (v < D_803ED810)
            goto shadow;
        ENGINE_BLK(802AF440);
        goto out;
    }
    ENGINE_BLK(802AF3EC);
    if (side == 2) {
        ENGINE_BLK(802AF410);
        if (D_803ED808 < v)
            goto shadow;
        ENGINE_BLK(802AF424);
        goto out;
    }
    ENGINE_BLK(802AF3F4);
    if (v < D_803ED808)
        goto shadow;
    ENGINE_BLK(802AF408);
out:
    /* out: walking */
    ENGINE_BLK(802AF45C);
    D_803ED826 = 0;
    vs->unk76 = 0;
shadow:
    /* (its last three stack arguments: whatever is at SHADOW_SLOT, and the
       halves of the return address its frame saved) */
    ENGINE_BLK(802AF464);
    y2 = (u32)(D_803ED3A8[1] + D_803ED3A8[2]) >> 1;
    func_802582C4(0, D_803ED808, y2, D_803ED810, D_803ED80C, engine_frame_lw(SHADOW_SLOT), 0xFFFFFFFF,
                  RA_SHADOW);
    ENGINE_BLK(802AF4A4);
}

/* the walk: standing, its parts at rest (and now and then a look round,
   func_8026A8E0); moving, part 1 (the legs) with the speed, and a
   footstep's sound on frames 2 and 6 */
REGS(gp)
void func_802AF4BC(VS *vs) {
    Part *p = DRV;
    s32 s5, v, f13, a1;

    ENGINE_BLK(802AF4BC);
    s5 = vs->unk76;
    if (s5 != 0)
        goto moving;
    ENGINE_BLK(802AF4D0);
    if (vs->unkA1 != 0) {
        ENGINE_BLK(802AF4DC);
        func_802A02E4(1, p);
        ENGINE_BLK(802AF4EC);
        func_802A0360(3, 0, p, 0.0f);
        ENGINE_BLK(802AF508);
        func_8029F9D4(1, 3, p);
        ENGINE_BLK(802AF51C);
        func_802A039C(0x1F, 0x46, p);
        ENGINE_BLK(802AF530);
        func_802A03D4(0x1F, 0, p);
        ENGINE_BLK(802AF544);
        func_802A040C(0x1F, 0, p);
        ENGINE_BLK(802AF558);
        func_802A0290(0x1F, 1, p);
    }
    ENGINE_BLK(802AF56C);
    vs->unkA1 = 0;
    if (part_state(0x1F, p, NULL) == 1) {
        ENGINE_BLK(802AF584);
        goto done;
    }
    ENGINE_BLK(802AF584);
    ENGINE_BLK(802AF590);
    if (part_state(2, p, NULL) == 1) {
        ENGINE_BLK(802AF5A0);
        goto done;
    }
    ENGINE_BLK(802AF5A0);
    ENGINE_BLK(802AF5AC);
    if (part_state(3, p, NULL) == 1) {
        ENGINE_BLK(802AF5BC);
        goto done;
    }
    ENGINE_BLK(802AF5BC);
    ENGINE_BLK(802AF5C8);
    if (part_state(4, p, NULL) == 1) {
        ENGINE_BLK(802AF5D8);
        goto done;
    }
    ENGINE_BLK(802AF5D8);
    ENGINE_BLK(802AF5E4);
    v = func_8026A8E0(0, 0x14);
    ENGINE_BLK(802AF5F0);
    if (v != 0)
        goto done;
    ENGINE_BLK(802AF5F8);
    v = func_8026A8E0(0, 2);
    ENGINE_BLK(802AF604);
    if (v == 0) {
        ENGINE_BLK(802AF684);
        func_802A0360(3, 0, p, 0.0f);
        ENGINE_BLK(802AF6A0);
        func_802A0290(3, 1, p);
        ENGINE_BLK(802AF6B4);
        goto done;
    }
    ENGINE_BLK(802AF60C);
    if (v == 1) {
        ENGINE_BLK(802AF64C);
        func_802A0360(4, 0, p, 0.0f);
        ENGINE_BLK(802AF668);
        func_802A0290(4, 1, p);
        ENGINE_BLK(802AF67C);
        goto done;
    }
    ENGINE_BLK(802AF614);
    func_802A0360(2, 0, p, 0.0f);
    ENGINE_BLK(802AF630);
    func_802A0290(2, 1, p);
    ENGINE_BLK(802AF644);
    goto done;

moving:
    ENGINE_BLK(802AF6BC);
    if (vs->unkA1 == 1)
        goto legs;
    /* starting off: the parts still playing stopped */
    ENGINE_BLK(802AF6CC);
    func_802A0360(1, 2, p, 0.0f);
    ENGINE_BLK(802AF6E8);
    v = part_state(2, p, NULL);
    ENGINE_BLK(802AF6F8);
    if (v != 0) {
        ENGINE_BLK(802AF700);
        func_8029F9D4(2, 1, p);
        ENGINE_BLK(802AF714);
        func_802A02E4(2, p);
        ENGINE_BLK(802AF724);
        goto run;
    }
    ENGINE_BLK(802AF72C);
    v = part_state(3, p, NULL);
    ENGINE_BLK(802AF73C);
    if (v != 0) {
        ENGINE_BLK(802AF744);
        func_8029F9D4(3, 1, p);
        ENGINE_BLK(802AF758);
        func_802A02E4(3, p);
        ENGINE_BLK(802AF768);
        goto run;
    }
    ENGINE_BLK(802AF770);
    v = part_state(4, p, NULL);
    ENGINE_BLK(802AF780);
    if (v != 0) {
        ENGINE_BLK(802AF788);
        func_8029F9D4(4, 1, p);
        ENGINE_BLK(802AF79C);
        func_802A02E4(4, p);
        ENGINE_BLK(802AF7AC);
        goto run;
    }
    ENGINE_BLK(802AF7B4);
    func_802A0360(3, 0, p, 0.0f);
    ENGINE_BLK(802AF7D0);
    func_8029F9D4(3, 1, p);
run:
    ENGINE_BLK(802AF7E4);
    func_802A039C(0x1F, 0x28, p);
    ENGINE_BLK(802AF7F8);
    func_802A03D4(0x1F, 0, p);
    ENGINE_BLK(802AF80C);
    func_802A040C(0x1F, 0, p);
    ENGINE_BLK(802AF820);
    func_802A0290(0x1F, 1, p);
legs:
    ENGINE_BLK(802AF834);
    if (part_state(0x1F, p, NULL) == 1) {
        ENGINE_BLK(802AF844);
        goto walking;
    }
    ENGINE_BLK(802AF844);
    ENGINE_BLK(802AF850);
    s5 = vs->unk76;
    if (s5 < 0) {
        ENGINE_BLK(802AF85C);
        func_802A03D4(1, 1, p);
        ENGINE_BLK(802AF870);
    } else {
        ENGINE_BLK(802AF878);
        func_802A03D4(1, 0, p);
    }
    /* the footsteps (the original saves and loads back every register;
       the level's byte it tests is an s32's top one, always 0) */
    ENGINE_BLK(802AF88C);
    engine_save(0x5FFFFFFE, 0);
    v = (u32)D_802E8BDC >> 24;
    if (v == 0x31)
        goto steps;
    ENGINE_BLK(802AF91C);
    if (v == 0x26)
        goto steps;
    ENGINE_BLK(802AF928);
    part_state(1, p, &f13);
    ENGINE_BLK(802AF938);
    a1 = D_803ED820;
    D_803ED820 = f13;
    if (f13 == a1)
        goto steps;
    ENGINE_BLK(802AF950);
    if (f13 == 2) {
        a1 = 0x14;
    } else {
        ENGINE_BLK(802AF958);
        if (f13 != 6)
            goto steps;
        a1 = 0x15;
    }
    ENGINE_BLK(802AF964);
    func_80260650(D_80367738, a1, NULL);
steps:
    ENGINE_BLK(802AF974);
    engine_restore();
    if (s5 < 0) {
        ENGINE_BLK(802AFA0C);
        s5 = -s5;
    }
    ENGINE_BLK(802AFA10);
    s5 = (u32)s5 / 11;
    ENGINE_BLK(802AFA2C);
    func_802A039C(1, s5, p);
    ENGINE_BLK(802AFA34);
    func_802A0290(1, -1, p);
walking:
    ENGINE_BLK(802AFA48);
    vs->unkA1 = 1;
done:
    ENGINE_BLK(802AFA50);
}

/* the driver's matrix and its vertices */
REGS(gp)
void func_802AFA64(VS *vs) {
    u8 *model = MODEL, *buf;
    s32 *m, off;

    ENGINE_BLK(802AFA64);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802AFA94);
        m = (s32 *)(D_803ED82C + off);
    } else {
        ENGINE_BLK(802AFAA8);
        m = (s32 *)(D_803ED830 + off);
    }
    ENGINE_BLK(802AFAB8);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803ED808, D_803ED80C, D_803ED810, 0x4E20, m);
    ENGINE_LEAVE(18, T(m));           /* ($s2: 62740's func_802ABBEC reads it) */
    ENGINE_BLK(802AFAFC);
    if (D_8035805C != 0) {
        ENGINE_BLK(802AFB10);
        buf = D_803ED82C;
    } else {
        ENGINE_BLK(802AFB20);
        buf = D_803ED830;
    }
    ENGINE_BLK(802AFB2C);
    model = MODEL;
    func_8029C454(D_803ED808, D_803ED80C, D_803ED810, 0, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    ENGINE_BLK(802AFB74);
}

/* the turn rate: 0x8C */
REGS(-> s3)
s32 func_802AFB84(void) {
    ENGINE_BLK(802AFB84);
    return 0x8C;
}

/* the camera's distance and speed for the driver */
REGS()
void func_802AFBA0(void) {
    ENGINE_BLK(802AFBA0);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x28;
    D_803ED3F7 = 3;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802AFBFC(u8 *dst) {
    ENGINE_BLK(802AFBFC);
    func_802AC7DC(dst, (u8 *)&D_803ED760, (u32 *)&D_803ED808);
    ENGINE_BLK(802AFC18);
}

/* and back */
void func_802AFC28(u8 *src) {
    ENGINE_BLK(802AFC28);
    func_802AC85C(src, (u8 *)&D_803ED760, (u32 *)&D_803ED808);
    ENGINE_BLK(802AFC44);
}
