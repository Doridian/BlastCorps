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

    ENGINE_COST(802B0DA0, 236);
    D_803EDFC4 = model;
    buf = D_80358070;
    D_803EDFC8 = buf;
    D_803EDFCC = buf + 0x1400;
    D_80358070 = buf + 0x2800;
    func_802A1388(2, 1, D_803EDFC8, D_803EDFCC, model);
    func_802A754C(vs);
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
    func_802A992C(vs->unk52, D_803EDFBC, x, z, vs->unk4, &D_803EDFBC, (s16 *)&vs->unk4C, 2, vs, 0, &avg);
    func_8029F85C(P, D_803EDFC4, D_803EDFC8, D_803EDFCC);
    func_802A039C(0, 100, P);
    func_802A03D4(0, 0, P);
    func_802A040C(0, 0, P);
    func_802A0480(0, 0, P, 0.0f);
    func_802A0290(0, 1, P);
    func_8029E558(P, D_803EDFC8, D_803EDFCC);
    func_802A0320(0, P);
    func_802A0290(0, 1, P);
    func_8029E558(P, D_803EDFCC, D_803EDFC8);
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
    func_80258230(2, 0x64, 0x2D, 0x2D);
    func_802A0360(1, 0, P, 0.0f);
    func_802A0290(1, 1, P);
    vs->unk9A = 1;
    func_802B152C();
    vs->unk9A = 0;
    func_802A7764((u32 *)D_803EDFCC, (u32 *)D_803EDFC8, 0x1400);
    func_802AA838(D_803EDFCC, D_803EDFC8, *(s32 *)(D_803EDFC4 + *(s32 *)(D_803EDFC4 + 0x18) + 4));
    D_803F7844 = NULL;
}

/* hd.c's: whether it can be left: on the ground and walking */
u8 func_802B1150(void) {
    VS *vs = &D_803EDF10;
    u8 r = 0;

    ENGINE_COST(802B1150, 26);
    if (vs->unk96[0] != 1) {
        if (vs->unk96[1] != 1) {
            if (vs->unk96[2] != 1) {
                if (vs->unkA1 == 0) {
                    r = 1;
                }
            }
        }
    }
    return r;
}

/* hd.c's: the player gets out */
void func_802B11B8(void) {
    VS *vs = &D_803EDF10;

    ENGINE_COST(802B11B8, 28);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803EDFC8, (u32 *)D_803EDFCC, 0x1400);
    func_802A02E4(0x1F, P);
    func_802C444C();
    func_802608C8(D_803EDFD0);
}

/* hd.c's: the player gets in: its sound, its head and its arms */
void func_802B1228(void) {
    VS *vs = &D_803EDF10;

    ENGINE_COST(802B1228, 108);
    vs->unk96[3] = 0;
    D_8036444C = 0x1068;
    D_80364450 = 0xBB8;
    func_80260650(D_80367738, 0x50, &D_803EDFD0);
    func_802A05D0(D_802C2308, 0x19);
    func_802A05F8(D_802C2308, 0);
    func_802A0620(D_802C2308, 0);
    func_802A039C(7, 2, P);
    func_802A040C(7, 1, P);
    func_802A0480(7, 1, P, 0.5f);
    func_802A039C(8, 2, P);
    func_802A040C(8, 1, P);
    func_802A0480(8, 1, P, 0.5f);
    func_802A039C(9, 4, P);
    func_802A040C(9, 1, P);
    func_802A0480(9, 1, P, 0.5f);
    func_802A0480(1, 1, P, 0.5f);
    func_802A0480(5, 1, P, 0.5f);
    func_802A0480(3, 0, P, 0.0f);
}

/* hd.c's: put back on the ground where it is */
void func_802B13D8(void) {
    VS *vs = &D_803EDF10;

    ENGINE_COST(802B13D8, 35);
    func_802A9A60(vs->unk52, D_803EDFBC, D_803EDFB8, D_803EDFC0, vs->unk4, &D_803EDFBC, (s16 *)&vs->unk4C, 2, vs,
                  0);
    func_802B2768(vs);
    func_802A133C(D_803EDFB8, D_803EDFBC, D_803EDFC0, 2, vs);
}

/* its light */
void func_802B14E8(void) {
    ENGINE_COST(802B14E8, 17);
    func_802ABD54(2, D_803EDFB8, D_803EDFBC, D_803EDFC0);
}

/* each frame */
void func_802B152C(void) {
    VS *vs = &D_803EDF10;
    s32 t3 = 0, x, z, rate_i, turn, v, h;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_COST(802B152C, 188);
    /* (its $s4 and $fp as it found them: 5CB60.c and the other vehicles read them from the context) */
    engine_save(ENGINE_GPR(20) | ENGINE_GPR(30), 0);
    func_802B14E8();
    if (vs->unk9A == 0) {
        func_802B18F4(vs);
    }
    func_802B2900();
    rate_i = func_802B28B8(vs);
    turn = func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    (void)turn;
    func_802A785C(t3, &vs->unk76, 3, vs->unk96, vs->unk78, 0xC, vs, &t3);
    if (vs->unkA1 == 0) {
        func_802A77D0(vs);
    }
    func_802A7FD8(0x59D8, &vs->unk76, (u16 *)&vs->unk74, &vs->unk4C, &vs->unk4E, (s8 *)&vs->unk96[3], 0, vs);
    rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
    func_802A843C(&vs->unk76, 0, 2, (s8 *)vs->unk96, vs->unk4, 120.0f, vs);
    if (D_803EDFD8 != 0) {
        func_802A7070((s16 *)&D_803EDFD4, vs);
    }
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803EDFB8, &D_803EDFC0, rate, &z);
    D_803ED40B = 0;
    func_802A8768(x, z, &D_803EDFB8, &D_803EDFC0, &D_803EDFBC, 2, 0x78, 0x78, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    if (D_8035805C != 0) {
        func_8029E558(P, D_803EDFC8, D_803EDFCC);
    } else {
        func_8029E558(P, D_803EDFCC, D_803EDFC8);
    }
    func_802B2768(vs);
    func_8029A800(D_803EDFB8, D_803EDFBC, D_803EDFC0, D_80305CF0, 0, 0, vs->unk76, 0, 2, vs);
    func_8029C52C(2, vs);
    func_8029AA10();
    D_803F77D0 = P;
    func_802BE77C(2, vs);
    if (D_803A7425 == 0) {
        D_803EDFD8 = 0;
        D_803EDFDA = 0;
        goto done;
    }
    /* D_803A7425: turned toward the camera's heading; D_803EDFDA when it
       is within 0x190 of it */
    func_8029A914(vs);
    D_803EDFD8 = 1;
    D_803EDFDA = 0;
    v = func_802A6F6C();
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        h += 0xFFF;
    }
    h -= v;
    if (h < 0) {
        h = -h;
    }
    if (!(h < 0x801)) {
        h = 0xFFF - h;
    }
    if (h < 0x190) {
        D_803EDFDA = 1;
    }
    func_802A70D8(vs);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.25f, vs, &a1);

        D_803EDFD4 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    func_802A6FE4(0, vs);
done:
    D_803643E0 = D_803EDFB8;
    D_803643E4 = D_803EDFBC;
    D_803643E8 = D_803EDFC0;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 2, vs);
    engine_restore();
}

/* the body's animation (part 0x1F) for the walk to a stop: the legs back
   together, then a random idle (7, 8 or 9) */
static void stand(VS *vs) {
    s32 r;

    ENGINE_COST(802B19B0, 46);
    if (vs->unkA3 != 0) {
        func_802A0360(7, 0, P, 0.0f);
        if (part(0x1F, NULL, NULL) != 0) {
            func_802A02E4(0x1F, P);
            func_8029F9D4(0x1F, 7, P);
        } else {
            if (part(1, NULL, NULL) != 0) {
                func_802A02E4(1, P);
                func_8029F9D4(1, 7, P);
            } else {
                if (part(5, NULL, NULL) != 0) {
                    func_802A02E4(5, P);
                    func_8029F9D4(5, 7, P);
                } else {
                    func_802A0360(1, 0, P, 0.0f);
                    func_8029F9D4(1, 7, P);
                }
            }
        }
        func_802A039C(0x1F, 0x1E, P);
        func_802A03D4(0x1F, 0, P);
        func_802A040C(0x1F, 0, P);
        func_802A0290(0x1F, 1, P);
    }
    vs->unkA3 = 0;
    if (part(0x1F, NULL, NULL) == 1) {
        return;
    }
    if (part(7, NULL, NULL) == 1) {
        return;
    }
    if (part(8, NULL, NULL) == 1) {
        return;
    }
    if (part(9, NULL, NULL) == 1) {
        return;
    }
    r = func_8026A8E0(0, 0x1E);
    if (r != 0)
        return;
    r = func_8026A8E0(0, 2);
    if (r == 0) {
        func_802A0360(7, 0, P, 0.0f);
        func_802A0290(7, 1, P);
    } else {
        if (r == 1) {
            func_802A0360(8, 0, P, 0.0f);
            func_802A0290(8, 1, P);
        } else {
            func_802A0360(9, 0, P, 0.0f);
            func_802A0290(9, 1, P);
        }
    }
}

/* walking: the idle animations stopped, the walk started; rolling up with
   a C button or Z at speed 0x96; otherwise a step of the leg in front */
static void walk(VS *vs) {
    s32 s, a0, k;

    ENGINE_COST(802B1C74, 54);
    if (vs->unkA3 != 1) {
        func_802A0360(1, 0, P, 0.0f);
        if (part(7, NULL, NULL) != 0) {
            func_8029F9D4(7, 1, P);
            func_802A02E4(7, P);
        } else {
            if (part(8, NULL, NULL) != 0) {
                func_8029F9D4(8, 1, P);
                func_802A02E4(8, P);
            } else {
                if (part(9, NULL, NULL) != 0) {
                    func_8029F9D4(9, 1, P);
                    func_802A02E4(9, P);
                } else {
                    func_802A0360(7, 0, P, 0.0f);
                    func_8029F9D4(7, 1, P);
                }
            }
        }
        func_802A039C(0x1F, 0x28, P);
        func_802A03D4(0x1F, 0, P);
        func_802A040C(0x1F, 0, P);
        func_802A0290(0x1F, 1, P);
    }
    vs->unkA3 = 1;
    if (part(0x1F, NULL, NULL) == 1) {
        return;
    }
    D_803F7804 = 0;
    if (D_80370C35 == 0) {
        if (D_80370C1C != 0)
            goto roll;
        if (D_80370C1D != 0)
            goto roll;
    }
    if (D_80370C1A != 0)
        goto roll;
    if (D_80370C1B == 0)
        goto step;
roll:
    if (vs->unk76 < 0x96)
        goto step;
    /* rolling up into a ball */
    vs->unkA1 = 1;
    func_802A0360(2, 0, P, 0.0f);
    if (vs->unkA2 == 0) {
        func_8029F9D4(1, 2, P);
        func_802A02E4(1, P);
    } else {
        if (vs->unkA2 != 1) {
            engine_trap(0x802B1EB0);
        }
        func_8029F9D4(5, 2, P);
        func_802A02E4(5, P);
    }
    func_802A039C(0x1F, 0x23, P);
    func_802A03D4(0x1F, 0, P);
    func_802A040C(0x1F, 0, P);
    func_802A0290(0x1F, 1, P);
    func_80278EB0(6, 0.1f, 100);
    return;

step:
    /* dust now and then */
    if (((u32)D_803649D8 >> 8 & 0x2F) == 0) {
        func_802A6274(T(D_802C2984), 0xEA60, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    }
    k = vs->unkA2;
    if (k != 0) {
        if (k == 1)
            goto leg5;
        engine_trap(0x802B1FCC);
    }
    for (;;) {
        /* the left leg (part 1) in front */
        if (part(1, &a0, NULL) != 0) {
            s = vs->unk76;
            if (s < 0) {
                func_802A03D4(1, 1, P);
            } else {
                func_802A03D4(1, 0, P);
            }
            if (s < 0) {
                s = -s;
            }
            if (s != 0) {
                s = (u32)s / 0x18;
            }
            func_802A039C(1, s, P);
            return;
        }
        /* its step done: the other leg's */
        s = vs->unk76;
        if (s < 0) {
            s = -s;
        }
        if (!(s < 0x78)) {
            if (a0 != 0) {
                func_802A0290(5, 2, P);
            } else {
                func_802A0290(5, 1, P);
            }
            func_802A0360(5, 0, P, 0.0f);
            vs->unkA2 = 1;
            goto leg5;
        }
        if (a0 != 0) {
            func_802A0290(1, 2, P);
        } else {
            func_802A0290(1, 1, P);
        }
        func_802A0360(1, 0, P, 0.0f);
        vs->unkA2 = 0;
        continue;

leg5:
        /* the right leg (part 5) in front */
        if (part(5, &a0, NULL) != 0) {
            s = vs->unk76;
            if (s < 0) {
                func_802A03D4(5, 1, P);
            } else {
                func_802A03D4(5, 0, P);
            }
            if (s < 0) {
                s = -s;
            }
            if (s != 0) {
                s = (u32)s / 0x18;
            }
            func_802A039C(5, s, P);
            return;
        }
        s = vs->unk76;
        if (s < 0) {
            s = -s;
        }
        if (!(s < 0x78)) {
            if (a0 != 0) {
                func_802A0290(5, 2, P);
            } else {
                func_802A0290(5, 1, P);
            }
            func_802A0360(5, 0, P, 0.0f);
            vs->unkA2 = 1;
            goto leg5;
        }
        if (a0 != 0) {
            func_802A0290(1, 2, P);
        } else {
            func_802A0290(1, 1, P);
        }
        func_802A0360(1, 0, P, 0.0f);
        vs->unkA2 = 0;
    }
}

/* the parts and the states (unkA1) */
REGS(gp)
void func_802B18F4(VS *vs) {
    s32 v, a1, t1;

    ENGINE_COST(802B18F4, 38);
    if (vs->unkA1 != 0) {
        if (vs->unkA1 == 1)
            goto rolling;
        if (vs->unkA1 == 2)
            goto rolling2;
        if (vs->unkA1 == 3)
            goto landing;
        if (vs->unkA1 == 4)
            goto landing2;
        engine_trap(0x802B192C);
    }
    /* walking: the step sounds (the legs' frames 2 and 6) */
    v = part(1, NULL, &t1);
    if (v != 1) {
        v = part(5, NULL, &t1);
        if (v != 1)
            goto moving;
    }
    a1 = D_803EDFD6;
    D_803EDFD6 = t1;
    if (t1 != a1) {
        if (t1 == 6) {
            func_80260650(D_80367738, 0x4E, NULL);
        } else {
            if (t1 == 2) {
                func_80260650(D_80367738, 0x4F, NULL);
            }
        }
    }
moving:
    if (vs->unk76 == 0)
        stand(vs);
    else
        walk(vs);
    goto done;

rolling:
    /* rolling: the sound; into a building at speed, the landing; slowed
       down (or the C button let go), standing up */
    if (D_803F7844 == NULL) {
        func_80260650(D_80367738, 0x51, &D_803F7844);
    }
    if (vs->unk9C != 0)
        goto hit1;
    if (vs->unk9D != 0)
        goto up1;
    if (D_803EDFDA != 0)
        goto up1;
    vs->unk76 = 0x1BE;
    if (part(0x1F, NULL, NULL) == 1) {
        goto done;
    }
    func_802A039C(2, 0xA, P);
    func_802A03D4(2, 0, P);
    func_802A040C(2, 0, P);
    func_802A0290(2, 1, P);
    vs->unkA1 = 2;
    goto done;
hit1:
    vs->unk76 = vs->unk76 >> 1;
    func_802794A4();
    vs->unkA1 = 3;
    func_802A0360(3, 3, P, 0.0f);
    func_8029F9D4(0x1F, 3, P);
    func_802A039C(0x1F, 0x21, P);
    func_802A03D4(0x1F, 0, P);
    func_802A040C(0x1F, 0, P);
    func_802A0290(0x1F, 1, P);
    D_803F7804 = 1;
    goto done;
up1:
    if (vs->unk76 >= 0) {
        vs->unk76 = 0x3C;
    }
    func_802C444C();
    func_802A0360(1, 0, P, 0.0f);
    func_8029F9D4(0x1F, 1, P);
    func_802A039C(0x1F, 0x32, P);
    func_802A03D4(0x1F, 0, P);
    func_802A040C(0x1F, 0, P);
    func_802A0290(0x1F, 1, P);
    func_802794A4();
    vs->unkA1 = 0;
    goto done;

rolling2:
    if (vs->unk9C != 0)
        goto hit2;
    if (vs->unk9D != 0)
        goto up2;
    if (D_803EDFDA != 0)
        goto up2;
    vs->unk76 = 0x1BE;
    if (part(2, NULL, NULL) == 1) {
        goto done;
    }
up2:
    func_802C444C();
    if (vs->unk76 >= 0) {
        vs->unk76 = 0x3C;
    }
    func_802A0360(1, 0, P, 0.0f);
    func_802A02E4(2, P);
    func_8029F9D4(2, 1, P);
    func_802A039C(0x1F, 0x32, P);
    func_802A03D4(0x1F, 0, P);
    func_802A040C(0x1F, 0, P);
    func_802A0290(0x1F, 1, P);
    func_802794A4();
    vs->unkA1 = 0;
    goto done;
hit2:
    vs->unk76 = vs->unk76 >> 1;
    vs->unkA1 = 3;
    func_802A0360(3, 3, P, 0.0f);
    func_8029F9D4(2, 3, P);
    func_802A02E4(2, P);
    func_802A039C(0x1F, 0x21, P);
    func_802A03D4(0x1F, 0, P);
    func_802A040C(0x1F, 0, P);
    func_802A0290(0x1F, 1, P);
    D_803F7804 = 1;
    goto done;

landing:
    /* landing: the crash's sound and dust, then (part 0x1F's frame done)
       part 3's animation, state 4 */
    if (D_803F7844 != NULL) {
        func_802C444C();
        func_80260650(D_80367738, 0x4B, NULL);
    }
    func_802A6274(T(D_802C2984), 0x222E0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802BCC10();
    if (part(0x1F, NULL, NULL) == 1) {
        goto done;
    }
    D_803F7804 = 0;
    func_802794A4();
    func_802A039C(3, 8, P);
    func_802A03D4(3, 0, P);
    func_802A040C(3, 0, P);
    func_802A0290(3, 1, P);
    vs->unkA1 = 4;
    goto done;

landing2:
    func_802A6274(T(D_802C2984), 0x222E0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1);
    func_802BCC10();
    if (part(3, NULL, NULL) == 1) {
        goto done;
    }
    D_803F7804 = 1;
    func_802A0360(1, 0, P, 0.0f);
    func_802A0360(5, 0, P, 0.0f);
    vs->unkA1 = 0;
done:
    ;
}

/* Thunderfist's matrix (turned a quarter), its vertices and its collision */
REGS(gp)
void func_802B2768(VS *vs) {
    u8 *model = D_803EDFC4, *buf;
    s32 *m, off, h;

    ENGINE_COST(802B2768, 75);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        m = (s32 *)(D_803EDFC8 + off);
    } else {
        m = (s32 *)(D_803EDFCC + off);
    }
    h = (u16)vs->unk4C + 0x400;
    if (!(h < 0x1000)) {
        h -= 0xFFF;
    }
    D_803ED390[1] = h;
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    func_802AA764(D_803EDFB8, D_803EDFBC, D_803EDFC0, 0x6590, m);
    if (D_8035805C != 0) {
        buf = D_803EDFC8;
    } else {
        buf = D_803EDFCC;
    }
    model = D_803EDFC4;
    func_8029C454(D_803EDFB8, D_803EDFBC, D_803EDFC0, 2, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8),
                  buf);
    func_802ABBEC(2, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
}

/* the turn rate: none standing, 0x32 rolling, else 0x78 */
REGS(gp -> s3)
s32 func_802B28B8(VS *vs) {
    s32 r = 0;

    ENGINE_COST(802B28B8, 16);
    if (vs->unk76 != 0) {
        if (vs->unkA1 == 2)
            goto rolling;
        if (vs->unkA1 == 1)
            goto rolling;
        r = 0x78;
        goto done;
    rolling:
        r = 0x32;
    }
done:
    return r;
}

/* the camera's distance and speed for Thunderfist */
void func_802B2900(void) {
    ENGINE_COST(802B2900, 23);
    D_803EBBF4 = D_803EBBF0 * 4.0f;
    D_803ED3F6 = 0x28;
    D_803ED3F7 = 3;
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802B295C(u8 *dst) {
    ENGINE_COST(802B295C, 7);
    func_802AC7DC(dst, (u8 *)&D_803EDF10, (u32 *)&D_803EDFB8);
}

/* and back */
void func_802B2988(u8 *src) {
    ENGINE_COST(802B2988, 11);
    func_802AC85C(src, (u8 *)&D_803EDF10, (u32 *)&D_803EDFB8);
}
