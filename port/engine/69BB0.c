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

    ENGINE_COST(802AE370, 316);
    engine_save(ENGINE_T0_T5, 0);
    D_803ED818 = (VehicleModel *)model;
    buf = D_80358070;
    D_803ED82C = buf;
    D_803ED830 = buf + 0xC80;
    D_80358070 = buf + 0x1900;
    func_802A1388(0, 0, D_803ED82C, D_803ED830, model);
    func_802A754C(vs);
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
    func_8029F85C(DRV, MODEL, D_803ED82C, D_803ED830);
    func_802A039C(0, 100, DRV);
    func_802A03D4(0, 0, DRV);
    func_802A040C(0, 0, DRV);
    func_802A0480(0, 0, DRV, 0.0f);
    func_802A0290(0, 1, DRV);
    func_8029E558(DRV, D_803ED82C, D_803ED830);
    func_802A0320(0, DRV);
    func_802A0290(0, 1, DRV);
    func_8029E558(DRV, D_803ED830, D_803ED82C);
    r = vs->unk78;
    r[0] = -0x28, r[1] = 0, r[2] = 2;
    r[3] = 0, r[4] = 0x28, r[5] = 2;
    r[6] = 0x28, r[7] = 0x3C, r[8] = 2;
    r[9] = 0x3C, r[10] = 0x50, r[11] = 2;
    r[12] = 0x50, r[13] = 0x64, r[14] = 2;
    model = MODEL;
    func_8029C354(0, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x4E20);
    func_80258230(0, 0x28, 0xF, 0xF);
    vs->unk9A = 1;
    driver_frame();
    vs->unk9A = 0;
    model = MODEL;
    func_802AA838(D_803ED830, D_803ED82C, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    func_802A039C(1, 0, DRV);
    func_802A03D4(1, 0, DRV);
    func_802A040C(1, 0, DRV);
    func_802A0480(1, 1, DRV, 0.5f);
    func_802A039C(2, 2, DRV);
    func_802A03D4(2, 0, DRV);
    func_802A040C(2, 1, DRV);
    func_802A0480(2, 1, DRV, 0.5f);
    func_802A039C(3, 2, DRV);
    func_802A03D4(3, 0, DRV);
    func_802A040C(3, 1, DRV);
    func_802A0480(3, 1, DRV, 0.5f);
    func_802A039C(4, 2, DRV);
    func_802A03D4(4, 0, DRV);
    func_802A040C(4, 1, DRV);
    func_802A0480(4, 1, DRV, 0.5f);
    D_8036444C = 0xD48;
    D_80364450 = 0;
    engine_restore();
}

/* hd.c's: part 0x1F (the run) stopped */
void func_802AE860(void) {
    ENGINE_COST(802AE860, 10);
    func_802A02E4(0x1F, DRV);
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

    ENGINE_COST(802AE888, 235);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
#ifndef VERSION_US_V10
    D_803F7812 = 1;
#endif
    if (D_803ED825 == 0)
        goto fail;
    /* the side and distance for this level's vehicle (D_80305CB1) */
    for (;; t += 5) {
        if (t[0] == -1)
            goto found;
        if (t[0] != D_802E8BDC)
            continue;
        if ((u8)t[1] != D_80364456)
            continue;
        if (t[4] != 0) {
            if (func_802AEB9C((u8)t[4]) == 0) {
                continue;
            }
        }
        side = (u8)t[2];
        mul = (u8)t[3];
        break;
    }
found:
    dist = (u32)dist * (u32)mul;
    D_803ED828 = side;
    D_803ED827 = 1;
    D_803ED81C = D_803643E4;
    /* (what func_802AEC3C's frame saves) */
    /* the spots out to `dist`, every 100, then `dist` itself */
    for (d = 0;; d += 0x64) {
        if (dist < d)
            break;
        r = func_802AEC3C(d, vs);
        if (r == 0)
            goto fail;
    }
    r = func_802AEC3C(dist, vs);
    if (r == 0)
        goto fail;
    vs->unk96[0] = 0, vs->unk96[1] = 0, vs->unk96[2] = 0;
    vs->unk96[3] = 0;
    vs->unk76 = 0;
    side = D_803ED828;
    if (side == 0) {
        h = 0;
    } else {
        if (side == 1) {
            h = 0x800;
        } else {
            if (side == 2) {
                h = 0x400;
            } else {
                h = 0xC00;
            }
        }
    }
    vs->unk4E = h;
    vs->unk4C = h;
    vs->unk74 = h;
    side = D_803ED828;
    if (side != 0) {
        if (side != 1) {
            h = D_803ED808;
            goto spot;
        }
    }
    h = D_803ED810;
spot:
    /* it walks out from where the vehicle is */
    D_803ED814 = h;
    D_803ED808 = D_803643E0;
    h = D_803643E4;
    if (D_80364456 != 0xB) {
        if (D_80364456 != 0x11) {
            if (D_80364456 != 0x12)
                goto y;
        }
    }
    h += 0x1F4;
y:
    D_803ED80C = h;
    D_803ED810 = D_803643E8;
    D_803ED826 = 1;
    func_802A039C(1, 0, DRV);
    func_802A03D4(1, 0, DRV);
    func_802A040C(1, 0, DRV);
    func_802A0290(1, -1, DRV);
    D_8036444C = 0xD48;
    D_80364450 = 0;
    r = 1;
    goto done;
fail:
    r = 0;
done:
    D_803ED827 = 0;
    engine_restore();
    return r;
}

/* the table's test for a spot (D_80305CB1's fifth byte): 1, the player's z
   (>> 5) in 0xCCD..0xDB4; 2, its x in 0x834..0x960 and z in 0x4B0..0x5DC */
REGS(a3 -> a1)
s32 func_802AEB9C(s32 test) {
    s32 a1 = 0, v;

    ENGINE_COST(802AEB9C, 18);
    if (test == 1) {
        v = D_803643E8 >> 5;
        if (v < 0xCCD)
            goto done;
        if (!(v < 0xDB5))
            goto done;
        a1 = 1;
        goto done;
    }
    if (test != 2)
        goto done;
    v = D_803643E0 >> 5;
    if (v < 0x834)
        goto done;
    if (!(v < 0x961))
        goto done;
    v = D_803643E8 >> 5;
    if (v < 0x4B0)
        goto done;
    if (!(v < 0x5DD))
        goto done;
    a1 = 1;
done:
    return a1;
}

/* the spot `d` from the vehicle on the side D_803ED828 says: the driver
   put there on the ground, and whether nothing is in the way (the
   original saves and loads back $v0, $v1, $a0, $a2, $t0..$t9 and $gp) */
REGS(a2, gp -> a3)
s32 func_802AEC3C(s32 d, VS *vs) {
    s32 side = D_803ED828, r;

    ENGINE_COST(802AEC3C, 110);
    engine_save(SAVED_AEC3C, 0);
    /* (the original's frame saves $t6 at the shadow's slot: the game's C
       calls func_802AE888 at the depth it calls func_802AEEC8) */
    engine_frame_sw(SHADOW_SLOT, engine_ctx(14));
    if (side == 0) {
        D_803ED808 = D_803643E0;
        D_803ED810 = D_803643E8 + d;
    } else {
        if (side == 1) {
            D_803ED808 = D_803643E0;
            D_803ED810 = D_803643E8 - d;
        } else {
            if (side == 2) {
                D_803ED808 = D_803643E0 + d;
                D_803ED810 = D_803643E8;
            } else {
                D_803ED808 = D_803643E0 - d;
                D_803ED810 = D_803643E8;
            }
        }
    }
    func_802A9A60(vs->unk52, D_803ED81C, D_803ED808, D_803ED810, vs->unk4, &D_803ED80C, (s16 *)&vs->unk4C,
                  engine_ctx(24), vs, engine_ctx(30));
    func_802AFA64(vs);
    D_803ED81C = (u32)(vs->unk4[3] + vs->unk4[6]) >> 1;
    func_8029A800(D_803ED808, D_803ED80C, D_803ED810, D_80305CB0, 0, 0, 0, 0, 0, vs);
    func_8029AA10();
    D_803F77D0 = DRV;
    func_802BE77C(0, vs);
    if (D_80364456 != 0xB) {
        func_8028F994(D_803ED808, D_803ED80C, D_803ED810);
    }
    if (D_803A7424 != 0) {
        r = 0;
    } else {
        r = 1;
    }
    engine_restore();
    return r;
}

/* its light */
REGS()
void func_802AEE84(void) {
    ENGINE_COST(802AEE84, 17);
    func_802ABD54(0, D_803ED808, D_803ED80C, D_803ED810);
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

    ENGINE_COST(802AEEC8, 191);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802AEE84();
    if (vs->unk9A == 0) {
        func_802AF4BC(vs);
        if (D_803ED826 != 0) {
            func_802AF340(vs);
            goto parts;
        }
    }
    func_802AFBA0();
    rate_i = func_802AFB84();
    mode = D_80364A98;
    if (mode == 0) {
        mode = D_80364A90;
    }
    if (mode == 0x800)
        goto parts;
    if (mode == 1)
        goto parts;
    if (mode == 0x1000)
        goto parts;
    func_802A7834(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 6, rate_i, &vs->unk4C, vs, &t3, &rate_i);
    if (vs->unkA1 == 1) {
        func_802A77D0(vs);
    }
    func_802A7FD8(0x2328, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 0, vs);
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 1, 0, (s8 *)vs->unk96, vs->unk4, 60.0f, vs);
    if (D_803ED824 != 0) {
        func_802A7070((s16 *)&D_803ED822, vs);
    }
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803ED808, &D_803ED810, rate, &z);
    D_803ED40B = 0;
    /* ($s4, $s7, $t4, which func_802A8768 reads too) */
    ENGINE_LEAVE(12, vs->unk4E);
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803ED808, &D_803ED810, &D_803ED80C, 0, 0x3C, 0x3C, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
parts:
    if (D_8035805C != 0) {
        func_8029E558(DRV, D_803ED82C, D_803ED830);
    } else {
        func_8029E558(DRV, D_803ED830, D_803ED82C);
    }
    func_802AFA64(vs);
    if (D_803ED826 != 0)
        goto done;
    mode = D_80364A98;
    if (mode == 0) {
        mode = D_80364A90;
    }
    if (mode & 0x1801)
        goto still;
    func_8029A800(D_803ED808, D_803ED80C, D_803ED810, D_80305CB0, 0, 0, vs->unk76, 0, 0, vs);
    func_8029C52C(0, vs);
    func_8029AA10();
    D_803F77D0 = DRV;
    func_802BE77C(0, vs);
    if (D_803A7425 == 0)
        goto still;
    /* turned toward the camera's heading */
    func_8029A914(vs);
    D_803ED824 = 1;
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
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 1.0f, vs, &a1);

        D_803ED822 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    func_802A6FE4(0, vs);
    goto done;
still:
    D_803ED824 = 0;
done:
    D_803643E0 = D_803ED808;
    D_803643E4 = D_803ED80C;
    D_803643E8 = D_803ED810;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 0, vs);
    engine_restore();
}

/* getting out: a step of 100 toward the side, until it is past
   D_803ED814; and its shadow */
REGS(gp)
void func_802AF340(VS *vs) {
    s32 x, z, side, v, y2;

    ENGINE_COST(802AF340, 58);
    vs->unk76 = 0x50;
    side = D_803ED828;
    x = D_803ED808;
    z = D_803ED810;
    if (side == 0) {
        z += 0x64;
    } else {
        if (side == 1) {
            z -= 0x64;
        } else {
            if (side == 2) {
                x += 0x64;
            } else {
                x -= 0x64;
            }
        }
    }
    D_803ED808 = x;
    D_803ED810 = z;
    func_802A9A60(vs->unk52, D_803ED80C, x, z, vs->unk4, &D_803ED80C, (s16 *)&vs->unk4C, engine_ctx(24), vs,
                  engine_ctx(30));
    side = D_803ED828;
    v = D_803ED814;
    if (side == 0) {
        if (D_803ED810 < v)
            goto shadow;
        goto out;
    }
    if (side == 1) {
        if (v < D_803ED810)
            goto shadow;
        goto out;
    }
    if (side == 2) {
        if (D_803ED808 < v)
            goto shadow;
        goto out;
    }
    if (v < D_803ED808)
        goto shadow;
out:
    /* out: walking */
    D_803ED826 = 0;
    vs->unk76 = 0;
shadow:
    /* (its last three stack arguments: whatever is at SHADOW_SLOT, and the
       halves of the return address its frame saved) */
    y2 = (u32)(D_803ED3A8[1] + D_803ED3A8[2]) >> 1;
    func_802582C4(0, D_803ED808, y2, D_803ED810, D_803ED80C, engine_frame_lw(SHADOW_SLOT), 0xFFFFFFFF,
                  RA_SHADOW);
}

/* the walk: standing, its parts at rest (and now and then a look round,
   func_8026A8E0); moving, part 1 (the legs) with the speed, and a
   footstep's sound on frames 2 and 6 */
REGS(gp)
void func_802AF4BC(VS *vs) {
    Part *p = DRV;
    s32 s5, v, f13, a1;

    ENGINE_COST(802AF4BC, 106);
    s5 = vs->unk76;
    if (s5 != 0)
        goto moving;
    if (vs->unkA1 != 0) {
        func_802A02E4(1, p);
        func_802A0360(3, 0, p, 0.0f);
        func_8029F9D4(1, 3, p);
        func_802A039C(0x1F, 0x46, p);
        func_802A03D4(0x1F, 0, p);
        func_802A040C(0x1F, 0, p);
        func_802A0290(0x1F, 1, p);
    }
    vs->unkA1 = 0;
    if (part_state(0x1F, p, NULL) == 1) {
        goto done;
    }
    if (part_state(2, p, NULL) == 1) {
        goto done;
    }
    if (part_state(3, p, NULL) == 1) {
        goto done;
    }
    if (part_state(4, p, NULL) == 1) {
        goto done;
    }
    v = func_8026A8E0(0, 0x14);
    if (v != 0)
        goto done;
    v = func_8026A8E0(0, 2);
    if (v == 0) {
        func_802A0360(3, 0, p, 0.0f);
        func_802A0290(3, 1, p);
        goto done;
    }
    if (v == 1) {
        func_802A0360(4, 0, p, 0.0f);
        func_802A0290(4, 1, p);
        goto done;
    }
    func_802A0360(2, 0, p, 0.0f);
    func_802A0290(2, 1, p);
    goto done;

moving:
    if (vs->unkA1 == 1)
        goto legs;
    /* starting off: the parts still playing stopped */
    func_802A0360(1, 2, p, 0.0f);
    v = part_state(2, p, NULL);
    if (v != 0) {
        func_8029F9D4(2, 1, p);
        func_802A02E4(2, p);
        goto run;
    }
    v = part_state(3, p, NULL);
    if (v != 0) {
        func_8029F9D4(3, 1, p);
        func_802A02E4(3, p);
        goto run;
    }
    v = part_state(4, p, NULL);
    if (v != 0) {
        func_8029F9D4(4, 1, p);
        func_802A02E4(4, p);
        goto run;
    }
    func_802A0360(3, 0, p, 0.0f);
    func_8029F9D4(3, 1, p);
run:
    func_802A039C(0x1F, 0x28, p);
    func_802A03D4(0x1F, 0, p);
    func_802A040C(0x1F, 0, p);
    func_802A0290(0x1F, 1, p);
legs:
    if (part_state(0x1F, p, NULL) == 1) {
        goto walking;
    }
    s5 = vs->unk76;
    if (s5 < 0) {
        func_802A03D4(1, 1, p);
    } else {
        func_802A03D4(1, 0, p);
    }
    /* the footsteps (the original saves and loads back every register;
       the level's byte it tests is an s32's top one, always 0) */
    engine_save(0x5FFFFFFE, 0);
    v = (u32)D_802E8BDC >> 24;
    if (v == 0x31)
        goto steps;
    if (v == 0x26)
        goto steps;
    part_state(1, p, &f13);
    a1 = D_803ED820;
    D_803ED820 = f13;
    if (f13 == a1)
        goto steps;
    if (f13 == 2) {
        a1 = 0x14;
    } else {
        if (f13 != 6)
            goto steps;
        a1 = 0x15;
    }
    func_80260650(D_80367738, a1, NULL);
steps:
    engine_restore();
    if (s5 < 0) {
        s5 = -s5;
    }
    s5 = (u32)s5 / 11;
    func_802A039C(1, s5, p);
    func_802A0290(1, -1, p);
walking:
    vs->unkA1 = 1;
done:
    ;
}

/* the driver's matrix and its vertices */
REGS(gp)
void func_802AFA64(VS *vs) {
    u8 *model = MODEL, *buf;
    s32 *m, off;

    ENGINE_COST(802AFA64, 64);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803ED82C + off);
    } else {
        m = (s32 *)(D_803ED830 + off);
    }
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    func_802AA764(D_803ED808, D_803ED80C, D_803ED810, 0x4E20, m);
    ENGINE_LEAVE(18, T(m));           /* ($s2: 62740's func_802ABBEC reads it) */
    if (D_8035805C != 0) {
        buf = D_803ED82C;
    } else {
        buf = D_803ED830;
    }
    model = MODEL;
    func_8029C454(D_803ED808, D_803ED80C, D_803ED810, 0, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
}

/* the turn rate: 0x8C */
REGS(-> s3)
s32 func_802AFB84(void) {
    ENGINE_COST(802AFB84, 7);
    return 0x8C;
}

/* the camera's distance and speed for the driver */
REGS()
void func_802AFBA0(void) {
    ENGINE_COST(802AFBA0, 23);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x28;
    D_803ED3F7 = 3;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802AFBFC(u8 *dst) {
    ENGINE_COST(802AFBFC, 7);
    func_802AC7DC(dst, (u8 *)&D_803ED760, (u32 *)&D_803ED808);
}

/* and back */
void func_802AFC28(u8 *src) {
    ENGINE_COST(802AFC28, 7);
    func_802AC85C(src, (u8 *)&D_803ED760, (u32 *)&D_803ED808);
}
