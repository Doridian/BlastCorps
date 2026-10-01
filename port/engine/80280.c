/*
 * hd_code 80280 (us.v11 0x802C4A40-0x802C80D0): the level's status as the
 * Controller Pak keeps it, and the J-Bomb (type 9), as native C
 * (engine.h).
 *
 * The status (func_802C4A40, func_802C4BF0, func_802C4E58) is a string of
 * bits: one per damage group of each building (destroyed or not), then one
 * per RDU (collected or not), eight to a byte, the first in the top bit.
 *
 * The J-Bomb's parts are D_803F7850, its state D_803F7B50 and its position
 * D_803F7BF8..C00 (vehicle.h).  func_802C5120 sets it up (from the level
 * loader), func_802C5AFC runs it each frame (from hd.c and at the end of
 * the setup); the others are hd.c's hooks for it, and its two callbacks
 * for 62740's carrying (shared.h).  It walks or flies: its state's unkA1 is
 * its mode, 0 walking, 1 flying, 2 falling, 3 and 4 dropping onto
 * something below it, 5 landed (see func_802C61F0).
 */
#include "shared.h"
#include "game/game.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/audio.h"
#include "buildings.h"

extern u8 D_803063F0[];                         /* the share of the groups a medal wants, by medal */
extern s32 D_80368040;                          /* the level's groups to destroy */

u8 func_8026FE6C(s32 i);
void func_8026FE8C(s32 i);

extern VS D_803F7B50;
extern s32 D_803F7BF8, D_803F7BFC, D_803F7C00;  /* x, y, z */
extern u8 D_803ED40B;

REGS(gp)
void func_802C7ECC(VS *vs);
REGS(gp)
void func_802C7F28(VS *vs);
REGS(gp)
void func_802C7CB0(VS *vs);

#define T(p) ((s32)(p))

/* ---- the level's status ------------------------------------------------- */

/* hd.c's: the status into buf; its length */
s32 func_802C4A40(u8 *buf) {
    u8 *out = buf, *p;
    Building *b;
    u32 bits = 0, v;
    s32 n = 0, k, i;

    ENGINE_BLK(802C4A40);
    for (b = D_803F4030;; b++) {
        ENGINE_BLK(802C4A8C);
        if (b == D_803F7654)
            break;
        ENGINE_BLK(802C4A94);
        k = b->unkE9;
        p = &b->unkEC;
        for (;;) {
            ENGINE_BLK(802C4A9C);
            if (k == 0)
                break;
            ENGINE_BLK(802C4AA4);
            v = *p++;
            k--;
            if (v == 100) {
                ENGINE_BLK(802C4AC0);
                v = 1;
            } else {
                ENGINE_BLK(802C4AB8);
                v = 0;
            }
            ENGINE_BLK(802C4AC4);
            bits = bits << 1 | v;
            if (++n != 8)
                continue;
            ENGINE_BLK(802C4AD8);
            *out++ = bits;
            bits = 0;
            n = 0;
        }
        ENGINE_BLK(802C4AEC);
    }
    ENGINE_BLK(802C4AF4);
    if (n != 0) {
        ENGINE_BLK(802C4AFC);
        *out++ = bits << (8 - n);
    }
    ENGINE_BLK(802C4B10);
    k = D_8036EB90;
    bits = 0;
    n = 0;
    for (i = 0;; i++) {
        ENGINE_BLK(802C4B24);
        if (k == 0)
            break;
        ENGINE_BLK(802C4B2C);
        k--;
        v = func_8026FE6C(i);
        ENGINE_BLK(802C4B54);
        bits = bits << 1 | v;
        if (++n != 8)
            continue;
        ENGINE_BLK(802C4B8C);
        *out++ = bits;
        bits = 0;
        n = 0;
    }
    ENGINE_BLK(802C4BA0);
    if (n != 0) {
        ENGINE_BLK(802C4BA8);
        *out++ = bits << (8 - n);
    }
    ENGINE_BLK(802C4BBC);
    return out - buf;
}

/* (409D0.c's and the level loader's) the status from buf: the destroyed
   groups at 100 (and their pieces' parts off, func_802BF1F0), a building
   whose groups are all destroyed or all whole flagged (unkEA), the groups
   to destroy counted (D_80368040), the pieces brought back by a group
   (0x57) or switched off with theirs, and the RDUs collected */
void func_802C4BF0(u8 *buf) {
    u8 *in = buf, *p;
    Building *b;
    Piece *q;
    u32 byte = 0, mask = 1, d;
    s32 k, g, all, any, total = 0, i, n;

    ENGINE_BLK(802C4BF0);
    for (b = D_803F4030;; b++) {
        ENGINE_BLK(802C4C38);
        if (b == D_803F7654)
            break;
        ENGINE_BLK(802C4C40);
        k = b->unkE9;
        g = 0;
        p = &b->unkEC;
        all = 1;
        any = 0;
        for (;;) {
            ENGINE_BLK(802C4C54);
            if (k == 0)
                break;
            ENGINE_BLK(802C4C5C);
            k--;
            g++;
            if (mask == 1) {
                ENGINE_BLK(802C4C68);
                byte = *in++;
                mask = 0x100;
            }
            ENGINE_BLK(802C4C74);
            mask >>= 1;
            if (byte & mask) {
                ENGINE_BLK(802C4CA0);
                ENGINE_LEAVE(17, 0x64);
                func_802BF1F0(g, b);
                ENGINE_BLK(802C4CBC);
                any = 1;
                d = 0x64;
            } else {
                ENGINE_BLK(802C4C84);
                if (((u8 *)b->model)[0xD] != g) {
                    ENGINE_BLK(802C4C94);
                    all = 0;
                }
                ENGINE_BLK(802C4C98);
                d = 0;
            }
            ENGINE_BLK(802C4CCC);
            *p++ = d;
        }
        ENGINE_BLK(802C4CD8);
        b->unkEA = all;
        if (any) {
            ENGINE_BLK(802C4CE0);
            total += ((u8 *)b->model)[6];
        }
        ENGINE_BLK(802C4CEC);
    }
    ENGINE_BLK(802C4CF4);
    D_80368040 = total;
    for (b = D_803F4030;; b++) {
        ENGINE_BLK(802C4D0C);
        if (b == D_803F7654)
            break;
        ENGINE_BLK(802C4D14);
        k = b->unkE9;
        g = 1;
        for (i = 0;; i++, g++) {
            ENGINE_BLK(802C4D20);
            if (k == 0)
                break;
            ENGINE_BLK(802C4D28);
            k--;
            if ((&b->unkEC)[i] == 0x64) {
                ENGINE_BLK(802C4D40);
                for (q = b->unk4;; q++) {
                    ENGINE_BLK(802C4D48);
                    if (q == (Piece *)b->unk8)
                        break;
                    ENGINE_BLK(802C4D50);
                    n = q->group;
                    if (n == g) {
                        ENGINE_BLK(802C4D5C);
                        q->active = 0;
                    }
                    ENGINE_BLK(802C4D60);
                    if (q->group2 == g) {
                        ENGINE_BLK(802C4D6C);
                        if ((&b->unkEC)[n - 1] != 0x64) {
                            ENGINE_BLK(802C4D84);
                            q->active = 1;
                        }
                    }
                    ENGINE_BLK(802C4D8C);
                }
            }
            ENGINE_BLK(802C4D94);
        }
        ENGINE_BLK(802C4DA0);
    }
    ENGINE_BLK(802C4DA8);
    k = D_8036EB90;
    mask = 1;
    for (i = 0;; i++) {
        ENGINE_BLK(802C4DB8);
        if (k == 0)
            break;
        ENGINE_BLK(802C4DC0);
        k--;
        if (mask == 1) {
            ENGINE_BLK(802C4DC8);
            byte = *in++;
            mask = 0x100;
        }
        ENGINE_BLK(802C4DD4);
        mask >>= 1;
        if (byte & mask) {
            ENGINE_BLK(802C4DE4);
            func_8026FE8C(i);
            ENGINE_BLK(802C4E04);
        }
        ENGINE_BLK(802C4E20);
    }
    ENGINE_BLK(802C4E28);
}

/* (409D0.c's) a status for the medal `medal` (1..4) into buf: of the
   buildings that count (not model 0x38, not of kind 1), the ones already
   flagged (unkEB) and then as many more as the medal's share wants
   destroyed, then that share of the RDUs collected; its length */
s32 func_802C4E58(u8 *buf, u8 medal) {
    u8 *out = buf;
    Building *b;
    u32 bits = 0, want;
    s32 n = 0, k, count = 0, flagged = 0, bit, i;

    ENGINE_BLK(802C4E58);
    medal--;
    for (b = D_803F4030;; b++) {
        ENGINE_BLK(802C4EA4);
        if (b == D_803F7654)
            break;
        ENGINE_BLK(802C4EAC);
        if (b->unk30 != 0x38) {
            ENGINE_BLK(802C4EBC);
            if (((u8 *)b->model)[4] != 1) {
                ENGINE_BLK(802C4ED0);
                count++;
            }
        }
        ENGINE_BLK(802C4ED4);
    }
    ENGINE_BLK(802C4EDC);
    for (b = D_803F4030;; b++) {
        ENGINE_BLK(802C4EEC);
        if (b == D_803F7654)
            break;
        ENGINE_BLK(802C4EF4);
        if (b->unkEB != 0) {
            ENGINE_BLK(802C4F00);
            flagged++;
        }
        ENGINE_BLK(802C4F04);
    }
    ENGINE_BLK(802C4F0C);
    want = (u32)D_803063F0[medal] * (u32)count / 100u + 1;
    ENGINE_BLK(802C4F44);
    if (medal == 4) {
        ENGINE_BLK(802C4F64);
        want = 0;
    } else {
        ENGINE_BLK(802C4F58);
        want -= flagged;
        if ((s32)want < 0) {
            ENGINE_BLK(802C4F64);
            want = 0;
        }
    }
    ENGINE_BLK(802C4F68);
    for (b = D_803F4030;; b++) {
        ENGINE_BLK(802C4F80);
        if (b == D_803F7654)
            break;
        ENGINE_BLK(802C4F88);
        k = b->unkE9;
        if (b->unk30 == 0x38)
            goto zero;
        ENGINE_BLK(802C4F9C);
        if (((u8 *)b->model)[4] == 1)
            goto zero;
        ENGINE_BLK(802C4FB0);
        if (b->unkEB == 0) {
            ENGINE_BLK(802C4FBC);
            if (want == 0)
                goto zero;
            ENGINE_BLK(802C4FC4);
            want--;
        }
        ENGINE_BLK(802C4FC8);
        bit = 1;
        goto bits;
    zero:
        ENGINE_BLK(802C4FD0);
        bit = 0;
    bits:
        for (;;) {
            ENGINE_BLK(802C4FD4);
            if (k == 0)
                break;
            ENGINE_BLK(802C4FDC);
            k--;
            bits = bits << 1 | bit;
            if (++n != 8)
                continue;
            ENGINE_BLK(802C4FF4);
            *out++ = bits;
            bits = 0;
            n = 0;
        }
        ENGINE_BLK(802C5008);
    }
    ENGINE_BLK(802C5010);
    if (n != 0) {
        ENGINE_BLK(802C5018);
        *out++ = bits << (8 - n);
    }
    ENGINE_BLK(802C502C);
    k = D_8036EB90;
    want = (u32)D_803063F0[medal] * (u32)D_8036EB90 / 100u;
    ENGINE_BLK(802C5078);
    bits = 0;
    n = 0;
    for (;;) {
        ENGINE_BLK(802C5088);
        if (k == 0)
            break;
        ENGINE_BLK(802C5090);
        k--;
        if (want != 0) {
            ENGINE_BLK(802C5098);
            bit = 1;
            want--;
        } else {
            ENGINE_BLK(802C50A4);
            bit = 0;
        }
        ENGINE_BLK(802C50A8);
        bits = bits << 1 | bit;
        if (++n != 8)
            continue;
        ENGINE_BLK(802C50BC);
        *out++ = bits;
        bits = 0;
        n = 0;
    }
    ENGINE_BLK(802C50D0);
    if (n != 0) {
        ENGINE_BLK(802C50D8);
        *out++ = bits << (8 - n);
    }
    ENGINE_BLK(802C50EC);
    return out - buf;
}

/* ---- the J-Bomb ---------------------------------------------------------- */

extern Part D_803F7850[32];
extern u8 *PTR32 D_803F7C04;                    /* its model file */
extern u8 *PTR32 D_803F7C08;                    /* two 0x1000-byte buffers, one per frame */
extern u8 *PTR32 D_803F7C0C;
extern SndState *PTR32 D_803F7C18;              /* its jets' sound, while they play */
extern SndState *PTR32 D_803F7C1C;              /* its flight's */
extern s32 D_803F7C20;                          /* frames since it was last in mode 3 */
extern s32 D_803F7C24;                          /* frames since it was last in mode 4 */
extern f32 D_803F7C28, D_803F7C2C;              /* parts 3's and 4's tilt, 0..1 */
extern u16 D_803F7C30;                          /* the heading it turns to (with D_803A7425) */
extern s16 D_803F7C32;                          /* part 6's frame, last time */
extern s8 D_803F7C36;                           /* turning to D_803F7C30 */
extern u8 D_803F7C37, D_803F7C38, D_803F7C39, D_803F7C3A;
extern s8 D_803F7C3B;
extern u8 D_803F7C3C, D_803F7C3D, D_803F7C3E, D_803F7C40, D_803F7C41, D_803F7C42, D_803F7C43;
extern u8 D_803F7C44, D_803F7C45, D_803F7C46, D_803F7C47, D_803F7C48, D_803F7C49, D_803F7C4A, D_803F7C4B;
extern s32 D_803F7844;

extern u8 D_80306400[];                         /* the J-Bomb's bounce records */
extern u8 D_802C28E4[];                         /* its landing's effect record (60F60) */
extern u8 D_802C3804[];                         /* its jets' */
extern u8 D_803ED3F6, D_803ED3F7;
extern f32 D_803EBBF0, D_803EBBF4;
extern s32 D_803EBBFC;                          /* the ground's height above it (func_802AC0BC) */
extern s16 D_8036444C, D_80364450;
extern u8 D_803A7424, D_803A7425, D_803A742B;
extern u8 D_80370C1A, D_80370C1B, D_80370C1C, D_80370C1D, D_80370C23, D_80370C35;
extern s8 D_80370C2C, D_80370C2D;               /* the stick's x and y */
extern u16 D_803F7840;
extern Part *PTR32 D_803F77D0;
extern s32 D_803643E4, D_803643E8;
extern s32 D_802E8BE8;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80258230(u8 a, s32 b, s16 c, s16 d);
s32 func_802584BC(u8 type);
s32 func_80258500(u8 type);
s32 func_80288284(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
REGS(t0, t1, t2 -> a1)
s32 func_802AC0BC(s32 x, s32 z, s32 y);
REGS(v0, v1, a0)
void func_8029FC74(s32 a, s32 b, Part *parts);

void func_802C5AFC(void);
REGS()
void func_802C5970(void);
REGS(gp)
void func_802C617C(VS *vs);
REGS(gp)
void func_802C61F0(VS *vs);
REGS(gp)
void func_802C6DAC(VS *vs);
REGS(gp)
void func_802C6ECC(VS *vs);
REGS(gp -> s6)
s32 func_802C6FD8(VS *vs);
REGS(gp)
void func_802C70E8(VS *vs);
REGS(t0, t1)
void func_802C71FC(s32 x, s32 z);
REGS(gp)
void func_802C7354(VS *vs);
REGS()
void func_802C7410(void);
REGS(gp)
void func_802C7544(VS *vs);
REGS()
void func_802C770C(void);
REGS(gp)
void func_802C7864(VS *vs);
REGS(-> f4)
f32 func_802C7BC0(void);
REGS(s2, gp -> f2)
f32 func_802C7C1C(s32 up, VS *vs);
REGS(gp -> s3)
s32 func_802C7DFC(VS *vs);

#define JB D_803F7850

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

/* its jets' targets for a jump: unk28 (the speed), unk34 (where from) and
   unk40 (the step) for all three wheels, and in the air */
static void jump(VS *vs, s32 speed) {
    s32 y = D_803F7BFC;

    vs->unk28[6] = 1, vs->unk28[7] = 1, vs->unk28[8] = 1;
    vs->unk28[0] = speed, vs->unk28[1] = speed, vs->unk28[2] = speed;
    vs->unk28[3] = y, vs->unk28[4] = y, vs->unk28[5] = y;
}

/* set up: from the level loader, with the model file in $s2, the position
   in $t7, $s3, $s0 and the heading in $s1 */
REGS(s2, t7, s3, s0, s1)
void func_802C5120(u8 *model, s32 x, s32 y, s32 z, s32 heading) {
    VS *vs = &D_803F7B50;
    u8 *buf;
    s32 *s3, avg;

    ENGINE_BLK(802C5120);
    engine_save(ENGINE_T0_T5, 0);
    D_803F7C04 = model;
    buf = D_80358070;
    D_803F7C08 = buf;
    D_803F7C0C = buf + 0x1000;
    D_80358070 = buf + 0x2000;
    func_802A1388(9, 0, D_803F7C08, D_803F7C0C, model);
    ENGINE_BLK(802C51A0);
    func_802A754C(vs);
    ENGINE_BLK(802C51AC);
    vs->unk52[0] = 0;
    vs->unk52[1] = 0;
    vs->unk52[2] = 0;
    vs->unk52[3] = 0;
    vs->unk52[4] = 0;
    vs->unk52[5] = 0;
    vs->unk5E[0] = 0x190;
    vs->unk5E[1] = 0x190;
    vs->unk5E[2] = -0x190;
    vs->unk5E[3] = 0x190;
    vs->unk5E[4] = 0x190;
    vs->unk5E[5] = -0x190;
    D_803F7BF8 = x;
    D_803F7BFC = y;
    D_803F7C00 = z;
    vs->unk4C = heading;
    vs->unk4E = heading;
    vs->unk74 = heading;
    s3 = func_802A992C(vs->unk52, D_803F7BFC, x, z, vs->unk4, &D_803F7BFC, (s16 *)&vs->unk4C, 9, vs, engine_ctx(30),
                       &avg);
    ENGINE_LEAVE(19, T(s3));
    ENGINE_LEAVE(21, avg);
    ENGINE_BLK(802C5240);
    func_8029F85C(JB, D_803F7C04, D_803F7C08, D_803F7C0C);
    ENGINE_BLK(802C527C);
    func_802A039C(0, 100, JB);
    ENGINE_BLK(802C5290);
    func_802A03D4(0, 0, JB);
    ENGINE_BLK(802C52A4);
    func_802A040C(0, 0, JB);
    ENGINE_BLK(802C52B8);
    func_802A0480(0, 0, JB, 0.0f);
    ENGINE_BLK(802C52D0);
    func_802A0290(0, 1, JB);
    ENGINE_BLK(802C52E4);
    func_8029E558(JB, D_803F7C08, D_803F7C0C);
    ENGINE_BLK(802C52F8);
    func_802A0320(0, JB);
    ENGINE_BLK(802C5308);
    func_802A0290(0, 1, JB);
    ENGINE_BLK(802C531C);
    func_8029E558(JB, D_803F7C0C, D_803F7C08);
    ENGINE_BLK(802C5330);
    D_803F7C36 = 0;
    D_803F7C37 = 0;
    D_803F7C39 = 0;
    D_803F7C3D = 0;
    D_803F7C38 = 0;
    D_803F7C45 = 0;
    D_803F7C47 = 0;
    D_803F7C4A = 0;
    D_803F7C3A = 0;
    D_803F7C3B = 0;
    D_803F7C28 = 0.5f;
    D_803F7C2C = 0.5f;
    vs->unkA1 = 0;
    vs->unkA2 = 0;
    D_803F7C3C = 0x14;
    D_803F7C43 = 0;
    D_803F7C4B = 0x1E;
    D_803F7C20 = 999999;
    D_803F7C24 = 999999;
    D_803F7C40 = 0;
    D_803F7C41 = 0;
    D_803F7C34 = 0;
    D_803F7C42 = 0;
    model = D_803F7C04;
    func_8029C354(9, model + *(s32 *)(model + 4), model + *(s32 *)(model + 8), 0x4268);
    ENGINE_BLK(802C5434);
    func_80258230(9, 0x64, 0x2D, 0x2D);
    ENGINE_BLK(802C544C);
    func_802A0360(6, 2, JB, 0.0f);
    ENGINE_BLK(802C546C);
    func_802A0290(6, -1, JB);
    ENGINE_BLK(802C5480);
    vs->unk9A = 1;
    func_802C5AFC();
    ENGINE_BLK(802C548C);
    vs->unk9A = 0;
    func_802A7764((u32 *)D_803F7C0C, (u32 *)D_803F7C08, 0x1000);
    ENGINE_BLK(802C54A8);
    model = D_803F7C04;
    func_802AA838(D_803F7C0C, D_803F7C08, *(s32 *)(model + *(s32 *)(model + 0x18) + 4));
    ENGINE_BLK(802C54DC);
    D_803F7844 = 0;
    engine_restore();
    /* what the original leaves for its (translated) caller */
    ENGINE_LEAVE(28, T(vs));
    ENGINE_LEAVE(17, T(vs->unk4));
    ENGINE_LEAVE(18, T(&D_803F7BFC));
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(22, T(D_803F7C08));
    ENGINE_LEAVE(23, T(D_803F7C0C));
}

/* hd.c's: whether it can be left: 0 with a wheel off the ground, else 2,
   or 1 when it stands still (parts 0x1F and 9 at rest); landed (mode 5),
   it is put back walking first */
u8 func_802C5508(void) {
    VS *vs = &D_803F7B50;
    u8 r = 0;
    s32 a0;

    ENGINE_BLK(802C5508);
    engine_save(ENGINE_GPR(28), 0);
    if (vs->unk96[0] == 1)
        goto done;
    ENGINE_BLK(802C552C);
    if (vs->unk96[1] == 1)
        goto done;
    ENGINE_BLK(802C553C);
    if (vs->unk96[2] == 1)
        goto done;
    ENGINE_BLK(802C554C);
    a0 = vs->unkA1;
    if (a0 == 5)
        goto landed;
    ENGINE_BLK(802C555C);
    if (a0 != 0)
        goto two;
    ENGINE_BLK(802C5564);
    if (part_state(0x1F, JB, NULL) == 1) {
        ENGINE_BLK(802C5574);
        goto two;
    }
    ENGINE_BLK(802C5574);
    ENGINE_BLK(802C5580);
    if (part_state(9, JB, NULL) == 0) {
        ENGINE_BLK(802C5590);
        ENGINE_BLK(802C5670);
        r = 1;
        goto done;
    }
    ENGINE_BLK(802C5590);
two:
    ENGINE_BLK(802C5598);
    r = 2;
    goto done;
landed:
    ENGINE_BLK(802C55A0);
    vs->unkA1 = 0;
    func_802A02E4(7, JB);
    ENGINE_BLK(802C55B8);
    func_802A02E4(8, JB);
    ENGINE_BLK(802C55C8);
    func_802A0360(6, 2, JB, 0.0f);
    ENGINE_BLK(802C55E4);
    func_802A0360(9, 0, JB, 0.0f);
    ENGINE_BLK(802C5600);
    func_802A039C(9, 3, JB);
    ENGINE_BLK(802C5614);
    func_802A03D4(9, 0, JB);
    ENGINE_BLK(802C5628);
    func_802A0480(9, 0, JB, 0.0f);
    ENGINE_BLK(802C5640);
    func_802A040C(9, 1, JB);
    ENGINE_BLK(802C5654);
    func_802A0290(9, 1, JB);
    ENGINE_BLK(802C5668);
    r = 2;
done:
    ENGINE_BLK(802C5674);
    engine_restore();
    return r;
}

/* hd.c's: the player gets out: the sounds off */
void func_802C5688(void) {
    VS *vs = &D_803F7B50;

    ENGINE_BLK(802C5688);
    vs->unk76 = 0;
    func_802A7764((u32 *)D_803F7C08, (u32 *)D_803F7C0C, 0x1000);
    ENGINE_BLK(802C56BC);
    func_802A02E4(0x1F, JB);
    ENGINE_BLK(802C56CC);
    if (D_803F7C18 != NULL) {
        ENGINE_BLK(802C56DC);
        func_802608C8(D_803F7C18);
    }
    ENGINE_BLK(802C56E4);
    if (D_803F7C1C != NULL) {
        ENGINE_BLK(802C56F4);
        func_802608C8(D_803F7C1C);
    }
    ENGINE_BLK(802C56FC);
    ENGINE_LEAVE(28, T(vs));
}

/* hd.c's (and 17210.c's): the player gets in */
void func_802C5714(void) {
    VS *vs = &D_803F7B50;
    Part *p = JB;

    ENGINE_BLK(802C5714);
    vs->unk96[3] = 0;
    func_802A039C(3, 0, p);
    ENGINE_BLK(802C573C);
    func_802A03D4(3, 0, p);
    ENGINE_BLK(802C5750);
    func_802A040C(3, 1, p);
    ENGINE_BLK(802C5764);
    func_802A0480(3, 1, p, 1.0f);
    ENGINE_BLK(802C5780);
    func_802A0290(3, -1, p);
    ENGINE_BLK(802C5794);
    func_802A039C(4, 0, p);
    ENGINE_BLK(802C57A8);
    func_802A03D4(4, 0, p);
    ENGINE_BLK(802C57BC);
    func_802A0480(4, 1, p, 1.0f);
    ENGINE_BLK(802C57D8);
    func_802A040C(4, 1, p);
    ENGINE_BLK(802C57EC);
    func_802A0290(4, -1, p);
    ENGINE_BLK(802C5800);
    func_802A039C(1, 0, p);
    ENGINE_BLK(802C5814);
    func_802A03D4(1, 0, p);
    ENGINE_BLK(802C5828);
    func_802A040C(1, 0, p);
    ENGINE_BLK(802C583C);
    func_802A0290(1, -1, p);
    ENGINE_BLK(802C5850);
    ENGINE_LEAVE(28, T(vs));
}

/* hd.c's: put back on the ground where it is */
void func_802C5860(void) {
    VS *vs = &D_803F7B50;

    ENGINE_BLK(802C5860);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    func_802A9A60(vs->unk52, D_803F7BFC, D_803F7BF8, D_803F7C00, vs->unk4, &D_803F7BFC, (s16 *)&vs->unk4C, 9, vs,
                  engine_ctx(30));
    ENGINE_BLK(802C58EC);
    func_802C7CB0(vs);
    ENGINE_BLK(802C58F4);
    func_802A133C(D_803F7BF8, D_803F7BFC, D_803F7C00, 9, vs);
    ENGINE_BLK(802C5920);
    engine_restore();
}

/* its light */
REGS()
void func_802C5970(void) {
    ENGINE_BLK(802C5970);
    func_802ABD54(9, D_803F7BF8, D_803F7BFC, D_803F7C00);
    ENGINE_BLK(802C59A4);
}

/* 62740's carrying (shared.h): where it stands on its carrier (its point
   only; the original saves and loads back $t0, $t1, $t3 and $t4) */
REGS(a3)
void func_802C59B4(s32 carrier) {
    VS *vs = &D_803F7B50;
    s32 t3, t4;

    ENGINE_BLK(802C59B4);
    engine_save(ENGINE_GPR(8) | ENGINE_GPR(9) | ENGINE_GPR(11) | ENGINE_GPR(12), 0);
    ENGINE_LEAVE(28, T(vs));
    t3 = func_802AAD0C(carrier, D_803F7BF8, D_803F7C00, &t4);
    ENGINE_BLK(802C59F0);
    vs->unk6A = t3;
    vs->unk6C = t4;
    engine_restore();
}

/* and back there after the carrier moved, its heading kept (the original
   saves and loads back $a3, $t0, $t1 and $t2) */
REGS(a3)
void func_802C5A14(s32 carrier) {
    VS *vs = &D_803F7B50;
    s32 t3, t4;

    ENGINE_BLK(802C5A14);
    engine_save(ENGINE_GPR(7) | ENGINE_GPR(8) | ENGINE_GPR(9) | ENGINE_GPR(10), 0);
    ENGINE_LEAVE(28, T(vs));
    t3 = func_802AAE54(carrier, vs->unk6A, vs->unk6C, &t4);
    ENGINE_LEAVE(8, vs->unk6A);
    ENGINE_LEAVE(9, vs->unk6C);
    ENGINE_LEAVE(11, t3);
    ENGINE_LEAVE(12, t4);
    ENGINE_BLK(802C5A40);
    func_802C7ECC(vs);
    ENGINE_BLK(802C5A48);
    func_802C7F28(vs);
    ENGINE_BLK(802C5A50);
    D_803ED40B = 0;
    ENGINE_LEAVE(12, t4);
    ENGINE_LEAVE(14, T(&vs->unk76));
    ENGINE_LEAVE(16, T(vs->unk96));
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(t3, t4, &D_803F7BF8, &D_803F7C00, &D_803F7BFC, 9, 0x78, 0x78, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802C5AAC);
    func_802C7CB0(vs);
    ENGINE_BLK(802C5AB4);
    func_802A133C(D_803F7BF8, D_803F7BFC, D_803F7C00, 9, vs);
    ENGINE_BLK(802C5AE0);
    engine_restore();
}

/* each frame */
void func_802C5AFC(void) {
    VS *vs = &D_803F7B50;
    s32 t3 = 0, x, z, rate_i, v, h, sel, t1;
    u32 stick_addr;
    s32 stick;
    f32 rate;

    ENGINE_BLK(802C5AFC);
    engine_save(ENGINE_S0_S7_GP_FP, ENGINE_F20_F31);
    ENGINE_LEAVE(28, T(vs));
    func_802C5970();
    ENGINE_BLK(802C5B58);
    func_802C7ECC(vs);
    ENGINE_BLK(802C5B60);
    if (vs->unk9A == 0) {
        ENGINE_BLK(802C5B6C);
        func_802C61F0(vs);
        ENGINE_BLK(802C5B74);
        func_802C7410();
    }
    ENGINE_BLK(802C5B7C);
    func_802C7F28(vs);
    ENGINE_BLK(802C5B84);
    rate_i = func_802C7DFC(vs);
    ENGINE_BLK(802C5B8C);
    func_802A7E70(rate_i, &vs->unk4C, &stick_addr, &stick);
    ENGINE_BLK(802C5BAC);
    if (D_803F7C37 == 0) {
        ENGINE_BLK(802C5BBC);
        if (D_803F7C38 == 0) {
            ENGINE_BLK(802C5BCC);
            func_802A785C(t3, &vs->unk76, 4, vs->unk96, vs->unk78, D_803F7C48, vs, &t3);
            ENGINE_BLK(802C5BD4);
        } else {
            ENGINE_BLK(802C5BDC);
            D_803F7C38--;
        }
    }
    ENGINE_BLK(802C5BE8);
    if (vs->unkA1 == 0) {
        ENGINE_BLK(802C5BF4);
        func_802A77D0(vs);
        ENGINE_BLK(802C5BFC);
    } else {
        ENGINE_BLK(802C5C04);
        func_802C617C(vs);
    }
    ENGINE_BLK(802C5C0C);
    vs->unk4E = vs->unk4C;
    ENGINE_LEAVE(16, T(vs->unk96));     /* ($s0, which func_8029C454 reads too) */
    if (vs->unk96[0] != 1) {
        ENGINE_BLK(802C5C3C);
        rate = func_802A83B8(t3, &vs->unk76, vs->unk96, vs->unk4, &vs->unk0, &t3);
        ENGINE_BLK(802C5C44);
    } else {
        ENGINE_BLK(802C5C4C);
        rate = 0.0f;
    }
    ENGINE_BLK(802C5C54);
    if ((s8)D_803F7C37 != 0)
        goto turn;
    ENGINE_BLK(802C5C64);
    if (vs->unkA1 == 0) {
        ENGINE_BLK(802C5CB8);
        func_802A843C(&vs->unk76, 1, 9, (s8 *)vs->unk96, vs->unk4, 120.0f, vs);
        goto turn;
    }
    /* in the air: the speed toward 0 */
    ENGINE_BLK(802C5C70);
    t1 = vs->unk76;
    if (t1 != 0) {
        ENGINE_BLK(802C5C7C);
        if (t1 > 0) {
            ENGINE_BLK(802C5C98);
            t1--;
            if (t1 < 0) {
                ENGINE_BLK(802C5CAC);
                t1 = 0;
            } else {
                ENGINE_BLK(802C5CA4);
            }
        } else {
            ENGINE_BLK(802C5C84);
            t1++;
            if (t1 > 0) {
                ENGINE_BLK(802C5CAC);
                t1 = 0;
            } else {
                ENGINE_BLK(802C5C90);
            }
        }
    }
    ENGINE_BLK(802C5CB0);
    vs->unk76 = t1;
turn:
    ENGINE_BLK(802C5CCC);
    if (D_803F7C36 != 0) {
        ENGINE_BLK(802C5CDC);
        func_802A7070((s16 *)&D_803F7C30, vs);
    }
    ENGINE_BLK(802C5CE8);
    x = func_802A860C(vs->unk4E, &vs->unk76, &D_803F7BF8, &D_803F7C00, rate, &z);
    ENGINE_BLK(802C5D00);
    func_802C71FC(x, z);
    ENGINE_BLK(802C5D08);
    D_803ED40B = 0;
    /* ($s4, $s7 and $t4, which func_802A8768 reads too) */
    ENGINE_LEAVE(12, vs->unk4E);
    ENGINE_LEAVE(20, T(&vs->unk4C));
    ENGINE_LEAVE(23, T(vs->unk4));
    func_802A8768(x, z, &D_803F7BF8, &D_803F7C00, &D_803F7BFC, 9, 0x78, 0x78, vs->unk52, vs->unk28, vs->unk28 + 6,
                  vs->unk28 + 3, vs->unk5E, vs);
    ENGINE_BLK(802C5D3C);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C5D64);
        func_8029E558(JB, D_803F7C08, D_803F7C0C);
        ENGINE_BLK(802C5D78);
    } else {
        ENGINE_BLK(802C5D80);
        func_8029E558(JB, D_803F7C0C, D_803F7C08);
    }
    ENGINE_BLK(802C5D94);
    func_802C7CB0(vs);
    ENGINE_BLK(802C5D9C);
    D_803F7C39 = 0;
    v = vs->unkA1;
    if (v == 5)
        goto still;
    ENGINE_BLK(802C5DE8);
    if (v == 1)
        goto sel1;
    ENGINE_BLK(802C5DF4);
    if (v == 2)
        goto sel1;
    ENGINE_BLK(802C5DFC);
    if (v == 3)
        goto sel1;
    ENGINE_BLK(802C5E04);
    if (v == 4)
        goto sel2;
    ENGINE_BLK(802C5E0C);
    if (D_803F7C24 < 3)
        goto sel2;
    ENGINE_BLK(802C5E20);
    sel = 0;
    goto bounce;
sel1:
    ENGINE_BLK(802C5E28);
    sel = 1;
    goto bounce;
sel2:
    ENGINE_BLK(802C5E30);
    sel = 2;
bounce:
    ENGINE_BLK(802C5E34);
    func_8029A800(D_803F7BF8, D_803F7BFC, D_803F7C00, D_80306400, 1, 0, vs->unk76, sel, 9, vs);
    ENGINE_BLK(802C5E3C);
    func_8029C52C(9, vs);
    ENGINE_BLK(802C5E44);
    func_8029AA10();
    ENGINE_BLK(802C5E4C);
    if (D_803A742B != 0) {
        ENGINE_BLK(802C5E5C);
        v = vs->unkA1;
        if (v != 0) {
            ENGINE_BLK(802C5E68);
            if (v != 5) {
                ENGINE_BLK(802C5E70);
                vs->unkA1 = 2;
                D_803F7C39 = 1;
            }
        }
    }
    ENGINE_BLK(802C5E84);
    if (D_803A7425 != 0)
        goto camera;
    ENGINE_BLK(802C5E94);
    D_803A7424 = 0;
    D_803F77D0 = JB;
    func_802BE77C(9, vs);
    ENGINE_BLK(802C5EB4);
    if (D_803A7424 == 0) {
        ENGINE_BLK(802C5EC4);
        if (vs->unk9C == 0)
            goto still;
    }
    /* hit something: falling, unless walking with its jets on */
    ENGINE_BLK(802C5ED0);
    v = vs->unkA1;
    if (v == 1)
        goto fall;
    ENGINE_BLK(802C5EE0);
    if (v == 2)
        goto fall;
    ENGINE_BLK(802C5EE8);
    if (v == 3)
        goto fall;
    ENGINE_BLK(802C5EF0);
    if (v == 4)
        goto still;
    ENGINE_BLK(802C5EF8);
    if (D_803F7C20 < 3)
        goto fall;
    ENGINE_BLK(802C5F0C);
    if (vs->unk96[0] == 1)
        goto fall;
camera:
    /* turned toward the camera's heading */
    ENGINE_BLK(802C5F1C);
    func_8029A914(vs);
    ENGINE_BLK(802C5F24);
    D_803F7C36 = 1;
    v = func_802A6F6C();
    ENGINE_BLK(802C5F34);
    /* (the difference from the heading, which nothing uses) */
    h = (u16)vs->unk4E - 0x800;
    if (h < 0) {
        ENGINE_BLK(802C5F44);
        h += 0xFFF;
    }
    ENGINE_BLK(802C5F48);
    h -= v;
    if (h < 0) {
        ENGINE_BLK(802C5F54);
        h = -h;
    }
    ENGINE_BLK(802C5F58);
    if (!(h < 0x801)) {
        ENGINE_BLK(802C5F64);
    }
    ENGINE_BLK(802C5F6C);
    ENGINE_BLK(802C6030);
    func_802A70D8(vs);
    ENGINE_BLK(802C6038);
    {
        s32 a1;
        u16 a0 = func_802A71DC(vs->unk4E, vs->unk4C, 0.25f, &a1);

        ENGINE_BLK(802C604C);
        D_803F7C30 = a0;
        vs->unk4E = a0;
        vs->unk74 = a0;
        func_802A746C(a1, vs);
    }
    ENGINE_BLK(802C6060);
    func_802A6FE4(0, vs);
    ENGINE_BLK(802C6068);
    goto done;
fall:
    ENGINE_BLK(802C6018);
    vs->unkA1 = 2;
    D_803F7C39 = 1;
    goto done;
still:
    ENGINE_BLK(802C6070);
    D_803F7C36 = 0;
    D_803F7C37 = 0;
done:
    ENGINE_BLK(802C6080);
    D_803643E0 = D_803F7BF8;
    D_803643E4 = D_803F7BFC;
    D_803643E8 = D_803F7C00;
    D_8036443C = vs->unk76;
    D_8036443E = vs->unk4E;
    D_80364440 = vs->unk4C;
    func_802A133C(D_803643E0, D_803643E4, D_803643E8, 9, vs);
    ENGINE_BLK(802C60FC);
    v = vs->unkA1;
    if (v != 0) {
        ENGINE_BLK(802C6108);
        if (v != 5) {
            ENGINE_BLK(802C6110);
            D_803F7C3F = 1;
            goto out;
        }
    }
    ENGINE_BLK(802C6120);
    D_803F7C3F = 0;
out:
    ENGINE_BLK(802C6128);
    engine_restore();
}

/* in the air: its speed toward 0 by 8 a frame, unless the stick or
   D_80370C35 holds it */
REGS(gp)
void func_802C617C(VS *vs) {
    s32 s;

    ENGINE_BLK(802C617C);
    if (D_80370C35 != 0)
        goto done;
    ENGINE_BLK(802C6198);
    if (D_80370C2D != 0)
        goto done;
    ENGINE_BLK(802C61A8);
    s = vs->unk76;
    if (s < 0) {
        ENGINE_BLK(802C61C8);
        s += 8;
        if (!(s <= 0)) {
            ENGINE_BLK(802C61D4);
            s = 0;
        }
    } else {
        ENGINE_BLK(802C61B4);
        s -= 8;
        if (s < 0) {
            ENGINE_BLK(802C61C0);
            s = 0;
        }
    }
    ENGINE_BLK(802C61D8);
    vs->unk76 = s;
done:
    ENGINE_BLK(802C61DC);
}

/* its modes (see the top), its jets and its slam, each frame before it
   moves */
REGS(gp)
void func_802C61F0(VS *vs) {
    Part *p = JB;
    s32 v, s4, s5, s6, f13;

    ENGINE_BLK(802C61F0);
    v = vs->unkA1;
    vs->unkA2 = v;
    if (v == 1)
        goto air;
    ENGINE_BLK(802C620C);
    if (v == 2)
        goto air;
    ENGINE_BLK(802C6218);
    if (v == 3)
        goto air;
    ENGINE_BLK(802C6220);
    D_803F7C41 = 9;
    goto timers;
air:
    ENGINE_BLK(802C6230);
    v = D_803F7C41;
    if (v != 0) {
        ENGINE_BLK(802C6240);
        D_803F7C41 = v - 1;
    }
timers:
    ENGINE_BLK(802C624C);
    v = D_803F7C40;
    if (v != 0) {
        ENGINE_BLK(802C625C);
        v--;
        D_803F7C40 = v;
    }
    /* (compares D_803F7C40's count with the modes, where it means unkA1) */
    ENGINE_BLK(802C6268);
    if (v != 2) {
        ENGINE_BLK(802C6274);
        if (v != 3) {
            ENGINE_BLK(802C6284);
            D_803F7C20++;
            goto mode4;
        }
    }
    ENGINE_BLK(802C627C);
    D_803F7C20 = 0;
mode4:
    ENGINE_BLK(802C6298);
    if (v != 4) {
        ENGINE_BLK(802C62AC);
        D_803F7C24++;
    } else {
        ENGINE_BLK(802C62A4);
        D_803F7C24 = 0;
    }
    /* the jets: below its ceiling, walking or flying, with the boost
       buttons (or still climbing, D_803F7C3D) */
    ENGINE_BLK(802C62C0);
    D_803F7C3E = 0;
    if (D_803F7C14 < D_803F7BFC)
        goto landed;
    ENGINE_BLK(802C62E4);
    v = vs->unkA1;
    if (v == 4)
        goto landed;
    ENGINE_BLK(802C62F4);
    if (v == 2)
        goto landed;
    ENGINE_BLK(802C62FC);
    v = D_803F7C3D;
    if (v != 0) {
        ENGINE_BLK(802C630C);
        D_803F7C3D = v - 1;
        goto landed;
    }
    ENGINE_BLK(802C631C);
    if (D_80370C1C == 0) {
        ENGINE_BLK(802C632C);
        if (D_80370C1A == 0) {
            ENGINE_BLK(802C633C);
            if (D_80370C1B == 0)
                goto landed;
        }
    }
    ENGINE_BLK(802C634C);
    if (vs->unk96[0] == 1) {
        ENGINE_BLK(802C63AC);
        s6 = func_802C6FD8(vs);
        ENGINE_BLK(802C63B4);
        jump(vs, s6);
        D_803F7C3E = 1;
    } else {
        ENGINE_BLK(802C635C);
        jump(vs, 0x1E);
        vs->unk96[0] = 1, vs->unk96[1] = 1, vs->unk96[2] = 1;
        D_803F7C3E = 1;
    }
landed:
    /* landed (mode 5) and moving again: walking */
    ENGINE_BLK(802C63F0);
    if (vs->unkA1 != 5)
        goto modes;
    ENGINE_BLK(802C6400);
    if (vs->unk76 == 0) {
        ENGINE_BLK(802C640C);
        if (vs->unk96[0] == 0)
            goto modes;
    }
    ENGINE_BLK(802C6418);
    vs->unkA1 = 0;
    func_802A02E4(7, p);
    ENGINE_BLK(802C6430);
    func_802A02E4(8, p);
    ENGINE_BLK(802C6440);
    func_802A0360(6, 2, p, 0.0f);
    ENGINE_BLK(802C645C);
    func_802A0360(9, 0, p, 0.0f);
    ENGINE_BLK(802C6478);
    func_802A039C(9, 3, p);
    ENGINE_BLK(802C648C);
    func_802A03D4(9, 0, p);
    ENGINE_BLK(802C64A0);
    func_802A0480(9, 0, p, 0.0f);
    ENGINE_BLK(802C64B8);
    func_802A040C(9, 1, p);
    ENGINE_BLK(802C64CC);
    func_802A0290(9, 1, p);
modes:
    ENGINE_BLK(802C64E0);
    if (vs->unk9E != 0) {
        ENGINE_BLK(802C64EC);
        vs->unkA1 = 3;
        D_803F7C39 = 1;
        goto check;
    }
    ENGINE_BLK(802C6504);
    if (D_803F7C43 == 1)
        goto down;
    ENGINE_BLK(802C6518);
    if (vs->unk96[0] != 1)
        goto check;
    /* off the ground: flying */
    ENGINE_BLK(802C6528);
    func_802A02E4(6, p);
    ENGINE_BLK(802C6538);
    func_802A02E4(9, p);
    ENGINE_BLK(802C6548);
    if (vs->unkA2 == 5) {
        ENGINE_BLK(802C6558);
        D_803F7C28 = 0.1f;
    } else {
        ENGINE_BLK(802C6560);
        D_803F7C28 = 0.5f;
    }
    ENGINE_BLK(802C656C);
    D_803F7C2C = 0.5f;
    vs->unkA1 = 1;
    goto check;
down:
    /* back on the ground */
    ENGINE_BLK(802C6590);
    if (vs->unk96[0] != 0)
        goto check;
    ENGINE_BLK(802C659C);
    if (D_803F7C39 != 0)
        goto check;
    ENGINE_BLK(802C65AC);
    v = D_803F7BFC;
    vs->unk4[0] = v, vs->unk4[1] = v, vs->unk4[2] = v;
    vs->unk4[3] = v, vs->unk4[4] = v, vs->unk4[5] = v;
    vs->unk4[6] = v, vs->unk4[7] = v, vs->unk4[8] = v;
    func_802A0360(6, 2, p, 0.0f);
    ENGINE_BLK(802C65F4);
    func_802A02E4(5, p);
    ENGINE_BLK(802C6604);
    func_802A02E4(4, p);
    ENGINE_BLK(802C6614);
    func_802A02E4(3, p);
    ENGINE_BLK(802C6624);
    if (vs->unkA2 != 4) {
        ENGINE_BLK(802C6634);
        func_8029FC74(3, 4, p);
        ENGINE_BLK(802C6648);
        func_8029F9D4(0x1E, 6, p);
        ENGINE_BLK(802C665C);
        func_802A039C(0x1F, 0x32, p);
        ENGINE_BLK(802C6670);
        func_802A03D4(0x1F, 0, p);
        ENGINE_BLK(802C6684);
        func_802A040C(0x1F, 0, p);
        ENGINE_BLK(802C6698);
        func_802A0290(0x1F, 1, p);
    }
    ENGINE_BLK(802C66AC);
    s4 = vs->unk76;
    if (!(s4 < 0x97)) {
        ENGINE_BLK(802C66D0);
        vs->unk76 = 0x96;
    } else {
        ENGINE_BLK(802C66BC);
        if (s4 < -0x64) {
            ENGINE_BLK(802C66C4);
            vs->unk76 = -0x64;
        }
    }
    ENGINE_BLK(802C66D8);
    vs->unkA1 = 0;
check:
    ENGINE_BLK(802C66E0);
    func_802C70E8(vs);
    ENGINE_BLK(802C66E8);
    func_802C7354(vs);
    /* its slam, coming down from mode 4 */
    ENGINE_BLK(802C66F0);
    if (vs->unkA2 != 4)
        goto mode;
    ENGINE_BLK(802C6700);
    if (vs->unkA1 != 0)
        goto mode;
    ENGINE_BLK(802C670C);
    func_80260650(D_80367738, 0x7D, NULL);
    ENGINE_BLK(802C6720);
    D_802E8BE4 = 0x14;
    D_802E8BE8 = 0x320;
    func_802A6274(T(D_802C28E4), 0x222E0, 0, D_803F7BF8 << 11, D_803F7BFC << 11, D_803F7C00 << 11, 0, 0, 0, 0, 0, 0, 0,
                  0, 0);
    /* (the zeros it passes in $t7 and $s0..$s5, which it doesn't put back) */
    ENGINE_LEAVE(15, 0);
    ENGINE_LEAVE(16, 0);
    ENGINE_LEAVE(18, 0);
    ENGINE_LEAVE(19, 0);
    ENGINE_LEAVE(20, 0);
    ENGINE_LEAVE(21, 0);
mode:
    ENGINE_BLK(802C67B0);
    s5 = vs->unkA1;
    if (s5 == 5)
        goto mode5;
    ENGINE_BLK(802C67C0);
    if (s5 == 0)
        goto mode0;
    ENGINE_BLK(802C67C8);
    if (s5 == 1)
        goto mode1;
    ENGINE_BLK(802C67D0);
    if (s5 == 2)
        goto mode23;
    ENGINE_BLK(802C67D8);
    if (s5 == 3)
        goto mode23;
    ENGINE_BLK(802C67E0);
    if (s5 == 4)
        goto mode4b;
    ENGINE_BLK(802C67E8);
    engine_trap(0x802C67E8);

mode5:
    /* landed: part 8 played once parts 7 and 8 are done */
    ENGINE_BLK(802C67EC);
    if (part_state(7, p, NULL) == 1) {
        ENGINE_BLK(802C67FC);
        goto end;
    }
    ENGINE_BLK(802C67FC);
    ENGINE_BLK(802C6808);
    if (part_state(8, p, NULL) == 1) {
        ENGINE_BLK(802C6818);
        goto end;
    }
    ENGINE_BLK(802C6818);
    ENGINE_BLK(802C6824);
    func_802A0360(8, 0, p, 0.0f);
    ENGINE_BLK(802C6840);
    func_802A039C(8, 1, p);
    ENGINE_BLK(802C6854);
    func_802A03D4(8, 0, p);
    ENGINE_BLK(802C6868);
    func_802A0480(8, 1, p, 0.5f);
    ENGINE_BLK(802C6884);
    func_802A040C(8, 1, p);
    ENGINE_BLK(802C6898);
    func_802A0290(8, -1, p);
    ENGINE_BLK(802C68AC);
    goto end;

mode4b:
    /* slamming down */
    ENGINE_BLK(802C68B4);
    if (D_803F7C4A != 0) {
        ENGINE_BLK(802C68C4);
        if (part_state(5, p, NULL) == 1) {
            ENGINE_BLK(802C68D4);
        } else {
            ENGINE_BLK(802C68D4);
            ENGINE_BLK(802C68E0);
            jump(vs, -0x4B0);
            D_803F7C4A = 0;
        }
    }
    ENGINE_BLK(802C691C);
    D_803F7C3D = 0xA;
    goto end;

mode0:
    /* walking: its legs with the speed */
    ENGINE_BLK(802C692C);
    if (part_state(0x1F, p, NULL) == 1) {
        ENGINE_BLK(802C693C);
        goto end;
    }
    ENGINE_BLK(802C693C);
    ENGINE_BLK(802C6948);
    if (part_state(9, p, NULL) == 1) {
        ENGINE_BLK(802C6958);
        goto end;
    }
    ENGINE_BLK(802C6958);
    ENGINE_BLK(802C6964);
    s4 = vs->unk76;
    if (s4 < 0) {
        ENGINE_BLK(802C6970);
        func_802A03D4(6, 1, p);
        ENGINE_BLK(802C6984);
    } else {
        ENGINE_BLK(802C698C);
        func_802A03D4(6, 0, p);
    }
    ENGINE_BLK(802C69A0);
    if (s4 < 0) {
        ENGINE_BLK(802C69AC);
        s4 = -s4;
    }
    ENGINE_BLK(802C69B0);
    s4 = (u32)s4 / 16;
    ENGINE_BLK(802C69CC);
    func_802A039C(6, s4, p);
    ENGINE_BLK(802C69DC);
    func_802A0480(6, 1, p, 0.5f);
    ENGINE_BLK(802C69F8);
    func_802A040C(6, 0, p);
    ENGINE_BLK(802C6A0C);
    func_802A0290(6, -1, p);
    ENGINE_BLK(802C6A20);
    part_state(6, p, &f13);
    /* a footstep's sound on frames 0 and 3 */
    ENGINE_BLK(802C6A30);
    v = D_803F7C32;
    D_803F7C32 = f13;
    if (f13 == v)
        goto steps;
    ENGINE_BLK(802C6A48);
    if (f13 == 0) {
        v = 0x5A;
    } else {
        ENGINE_BLK(802C6A50);
        if (f13 != 3)
            goto steps;
        v = 0x5B;
    }
    ENGINE_BLK(802C6A5C);
    func_80260650(D_80367738, v, NULL);
steps:
    ENGINE_BLK(802C6A6C);
    goto end;

mode23:
    /* falling or dropping: flying again with the boost buttons */
    ENGINE_BLK(802C6A74);
    if (s5 == 3)
        goto mode1;
    ENGINE_BLK(802C6A80);
    D_803F7C3D = 0xA;
    if (D_80370C1C == 0) {
        ENGINE_BLK(802C6A9C);
        if (D_80370C1A == 0) {
            ENGINE_BLK(802C6AAC);
            if (D_80370C1B == 0)
                goto mode1;
        }
    }
    ENGINE_BLK(802C6ABC);
    s6 = func_802C6FD8(vs);
    ENGINE_BLK(802C6AC4);
    jump(vs, s6);
    D_803F7C3E = 1;
    vs->unkA1 = 1;

mode1:
    /* flying */
    ENGINE_BLK(802C6B08);
    func_802A02E4(0x1F, p);
    ENGINE_BLK(802C6B18);
    func_802A02E4(6, p);
    ENGINE_BLK(802C6B28);
    func_802A0290(4, -1, p);
    ENGINE_BLK(802C6B3C);
    func_802A0290(3, -1, p);
    ENGINE_BLK(802C6B50);
    func_802C7864(vs);
    ENGINE_BLK(802C6B58);
    if (D_803F7C39 != 0)
        goto bounce;
    ENGINE_BLK(802C6B68);
    if (D_803F7C40 != 0)
        goto end;
    ENGINE_BLK(802C6B78);
    if (D_803F7C41 != 0)
        goto end;
    ENGINE_BLK(802C6B88);
    if (vs->unk9F == 0x64)
        goto end;
    ENGINE_BLK(802C6B98);
    func_802C770C();
    ENGINE_BLK(802C6BA0);
    if (D_803F7C44 == 3)
        goto slam;
    ENGINE_BLK(802C6BB4);
    if (D_803F7C42 != 0)
        goto end;
    ENGINE_BLK(802C6BC4);
    if (D_80370C1D != 0)
        goto slam;
    ENGINE_BLK(802C6BD4);
    goto end;
bounce:
    /* bounced: up again, slower */
    ENGINE_BLK(802C6BDC);
    if (vs->unk9C == 0) {
        ENGINE_BLK(802C6BE8);
        v = vs->unk76;
        if (!(v < 0x15)) {
            ENGINE_BLK(802C6BF8);
            v = 0x14;
        }
        ENGINE_BLK(802C6BFC);
        if (v < -0x14) {
            ENGINE_BLK(802C6C08);
            v = -0x14;
        }
        ENGINE_BLK(802C6C0C);
        vs->unk76 = v;
    }
    ENGINE_BLK(802C6C10);
    jump(vs, 0x14A);
    vs->unk96[0] = 1, vs->unk96[1] = 1, vs->unk96[2] = 1;
    goto end;
slam:
    /* the slam: down hard (mode 4) */
    ENGINE_BLK(802C6C58);
    D_803F7C45 = 0;
    D_803F7C44 = 0;
    D_803F7C47 = 0;
    D_803F7C46 = 0;
    D_803F7C40 = 0x14;
    D_803F7C4A = 1;
    vs->unkA1 = 4;
    v = D_803F7BFC;
    vs->unk28[0] = 0;
    vs->unk28[1] = 0;
    vs->unk28[3] = v, vs->unk28[4] = v, vs->unk28[5] = v;
    vs->unk28[6] = 1, vs->unk28[7] = 1, vs->unk28[8] = 1;
    vs->unk28[2] = 0;
    func_802A02E4(4, p);
    ENGINE_BLK(802C6CD8);
    func_802A02E4(3, p);
    ENGINE_BLK(802C6CE8);
    func_802A039C(5, 0xA, p);
    ENGINE_BLK(802C6CFC);
    func_802A03D4(5, 0, p);
    ENGINE_BLK(802C6D10);
    func_802A040C(5, 1, p);
    ENGINE_BLK(802C6D24);
    func_802A0360(5, 0, p, 0.0f);
    ENGINE_BLK(802C6D40);
    func_802A0290(5, 1, p);
    ENGINE_BLK(802C6D54);
    func_80260650(D_80367738, 0x5C, NULL);
end:
    ENGINE_BLK(802C6D68);
    func_802C7544(vs);
    ENGINE_BLK(802C6D70);
    func_802C6DAC(vs);
    ENGINE_BLK(802C6D78);
    func_802C6ECC(vs);
    ENGINE_BLK(802C6D80);
    D_803F7C43 = vs->unk96[0];
    D_803F7C42 = D_80370C1D;
}

/* standing still on the ground for 30 frames: landed (mode 5, part 7) */
REGS(gp)
void func_802C6DAC(VS *vs) {
    s32 v;

    ENGINE_BLK(802C6DAC);
    if (vs->unk76 != 0)
        goto moving;
    ENGINE_BLK(802C6DC0);
    if (vs->unkA1 == 0) {
        ENGINE_BLK(802C6DDC);
        v = D_803F7C4B;
        if (v != 0) {
            ENGINE_BLK(802C6DEC);
            D_803F7C4B = v - 1;
        }
        goto check;
    }
moving:
    ENGINE_BLK(802C6DCC);
    D_803F7C4B = 0x1E;
check:
    ENGINE_BLK(802C6DF8);
    if (vs->unkA1 != 0)
        goto done;
    ENGINE_BLK(802C6E04);
    if (vs->unk76 != 0)
        goto done;
    ENGINE_BLK(802C6E10);
    if (D_803F7C4B != 0)
        goto done;
    ENGINE_BLK(802C6E20);
    vs->unkA1 = 5;
    func_802A02E4(6, JB);
    ENGINE_BLK(802C6E38);
    func_802A0360(7, 0, JB, 0.0f);
    ENGINE_BLK(802C6E54);
    func_802A039C(7, 3, JB);
    ENGINE_BLK(802C6E68);
    func_802A03D4(7, 0, JB);
    ENGINE_BLK(802C6E7C);
    func_802A0480(7, 0, JB, 0.0f);
    ENGINE_BLK(802C6E94);
    func_802A040C(7, 1, JB);
    ENGINE_BLK(802C6EA8);
    func_802A0290(7, 1, JB);
done:
    ENGINE_BLK(802C6EBC);
}

/* its sounds: the jets (D_803F7C18) while they fire, the flight
   (D_803F7C1C) in modes 1 to 3 */
REGS(gp)
void func_802C6ECC(VS *vs) {
    s32 v;

    ENGINE_BLK(802C6ECC);
    v = vs->unkA1;
    if (v != 0) {
        ENGINE_BLK(802C6EE0);
        if (v != 4)
            goto jets;
    }
    ENGINE_BLK(802C6EEC);
    if (D_803F7C1C != NULL) {
        ENGINE_BLK(802C6EFC);
        func_802608C8(D_803F7C1C);
    }
jets:
    ENGINE_BLK(802C6F04);
    if (D_803F7C3E != 0)
        goto firing;
    ENGINE_BLK(802C6F14);
    if (D_803F7C18 != NULL) {
        ENGINE_BLK(802C6F24);
        func_802608C8(D_803F7C18);
    }
    ENGINE_BLK(802C6F2C);
    v = vs->unkA1;
    if (v == 1)
        goto flight;
    ENGINE_BLK(802C6F3C);
    if (v == 2)
        goto flight;
    ENGINE_BLK(802C6F44);
    if (v != 3)
        goto done;
flight:
    ENGINE_BLK(802C6F4C);
    if (D_803F7C1C != NULL)
        goto done;
    ENGINE_BLK(802C6F5C);
    func_80260650(D_80367738, 0xF, &D_803F7C1C);
    ENGINE_BLK(802C6F74);
    goto done;
firing:
    ENGINE_BLK(802C6F7C);
    if (D_803F7C18 == NULL) {
        ENGINE_BLK(802C6F8C);
        func_80260650(D_80367738, 2, &D_803F7C18);
    }
    ENGINE_BLK(802C6FA4);
    if (D_803F7C1C != NULL) {
        ENGINE_BLK(802C6FB4);
        func_802608C8(D_803F7C1C);
        ENGINE_BLK(802C6FBC);
    }
done:
    ENGINE_BLK(802C6FC4);
}

/* the jets' push: the rise of this step of the parabola (unk28 the
   speed, unk40 the step, D_803EBBF4 the gravity), and less of it toward
   the ceiling (30 below D_803F7C10, nothing at D_803F7C14) */
REGS(gp -> s6)
s32 func_802C6FD8(VS *vs) {
    s32 s6, s7, t2, t4, t5, v1, a1;

    ENGINE_BLK(802C6FD8);
    t2 = vs->unk28[6];
    t4 = vs->unk28[0] * t2;
    t5 = engine_cvt_w_s(D_803EBBF4 * (f32)(t2 * t2));
    s6 = t4 + t5;
    t2 = vs->unk28[6] - 1;
    t4 = vs->unk28[0] * t2;
    t5 = engine_cvt_w_s(D_803EBBF4 * (f32)(t2 * t2));
    s7 = t4 + t5;
    s6 -= s7;
    if (D_803F7BFC < D_803F7C10) {
        ENGINE_BLK(802C70D0);
        v1 = 0x1E;
    } else {
        ENGINE_BLK(802C7088);
        a1 = D_803F7C14 - D_803F7C10;
        if (a1 == 0) {
            ENGINE_BLK(802C70C4);
            engine_break(0x802C70C4, 7);
        }
        v1 = 0x1E - (s32)((u32)((D_803F7BFC - D_803F7C10) * 0x1E) / (u32)a1);
        ENGINE_BLK(802C70C8);
    }
    ENGINE_BLK(802C70D4);
    ENGINE_LEAVE(23, s7);
    return s6 + v1;
}

/* a ceiling above it (func_802AC0BC, D_803EBBFC): just above, falling
   back (mode 1, from the jets' 30 if it was close) */
REGS(gp)
void func_802C70E8(VS *vs) {
    s32 d;

    ENGINE_BLK(802C70E8);
    d = func_802AC0BC(D_803F7BF8, D_803F7C00, D_803F7BFC);
    ENGINE_BLK(802C710C);
    if (d == 0)
        goto done;
    ENGINE_BLK(802C7114);
    if (D_803EBBFC < D_803F7BFC)
        goto done;
    ENGINE_BLK(802C7130);
    d = D_803EBBFC - D_803F7BFC;
    if (d < 0xC8) {
        ENGINE_BLK(802C7198);
        vs->unkA1 = 1;
        jump(vs, 0x1E);
        vs->unk96[0] = 1, vs->unk96[1] = 1, vs->unk96[2] = 1;
        D_803F7C3E = 1;
        goto done;
    }
    ENGINE_BLK(802C7140);
    if (!(d < 0xBB8))
        goto done;
    ENGINE_BLK(802C7148);
    vs->unkA1 = 1;
    jump(vs, 0);
    vs->unk96[0] = 1, vs->unk96[1] = 1, vs->unk96[2] = 1;
done:
    ENGINE_BLK(802C71EC);
}

/* where it is going (x, z, from func_802A860C): whether a ceiling is just
   above it there (D_803F7C49) (the original saves and loads back every
   register) */
REGS(t0, t1)
void func_802C71FC(s32 x, s32 z) {
    ENGINE_BLK(802C71FC);
    engine_save(0x5FFFFFFE, 0);
    D_803F7C49 = 0;
    if (func_802AC0BC(x, z, D_803F7BFC) == 0) {
        ENGINE_BLK(802C7290);
        goto done;
    }
    ENGINE_BLK(802C7290);
    ENGINE_BLK(802C7298);
    if (D_803EBBFC < D_803F7BFC)
        goto done;
    ENGINE_BLK(802C72B4);
    if (D_803EBBFC - D_803F7BFC < 0xC8)
        goto done;
    ENGINE_BLK(802C72C4);
    D_803F7C49 = 1;
done:
    ENGINE_BLK(802C72D0);
    engine_restore();
}

/* at the most damage (unk9F 100): parts 3 and 4 level, and falling (mode
   1) once it is 0x9C4 above its shadow */
REGS(gp)
void func_802C7354(VS *vs) {
    ENGINE_BLK(802C7354);
    if (vs->unk9F != 0x64)
        goto done;
    ENGINE_BLK(802C7368);
    if (vs->unkA1 != 1) {
        ENGINE_BLK(802C7378);
        D_803F7C28 = 0.5f;
        D_803F7C2C = 0.5f;
    }
    ENGINE_BLK(802C738C);
    if (!(D_803F7BFC < func_80258500(9) + 0x9C4)) {
        ENGINE_BLK(802C7398);
        goto done;
    }
    ENGINE_BLK(802C7398);
    ENGINE_BLK(802C73B4);
    jump(vs, 0x1E);
    vs->unk96[0] = 1, vs->unk96[1] = 1, vs->unk96[2] = 1;
    vs->unkA1 = 1;
    D_803F7C3E = 1;
done:
    ENGINE_BLK(802C7400);
}

/* the engine's sound by its height between D_803F7C10 and D_803F7C14 */
REGS()
void func_802C7410(void) {
    s32 y = D_803F7BFC, lo = D_803F7C10, hi = D_803F7C14, a1, a2, q;

    ENGINE_BLK(802C7410);
    if (y < lo) {
        ENGINE_BLK(802C74F0);
        D_8036444C = 0xD48;
        D_80364450 = 0x258;
        goto done;
    }
    ENGINE_BLK(802C7438);
    if (!(y < hi)) {
        ENGINE_BLK(802C7514);
        D_8036444C = 0x3E8;
        D_80364450 = 0xBB8;
        goto done;
    }
    ENGINE_BLK(802C7444);
    a1 = y - lo;
    a2 = hi - lo;
    ENGINE_DIV(q, a1 * -0x960, a2, 802C746C, 802C7470, 802C747C, 802C7484);
    ENGINE_BLK(802C7488);
    D_8036444C = q + 0xD48;
    ENGINE_DIV(q, a1 * 0x960, a2, 802C74BC, 802C74C0, 802C74CC, 802C74D4);
    ENGINE_BLK(802C74D8);
    D_80364450 = q + 0x258;
done:
    ENGINE_BLK(802C7534);
}

/* its jets: a burst when they start (on the ground), their flames'
   size (part 1, D_803F7C3B) and their sparks */
REGS(gp)
void func_802C7544(VS *vs) {
    s32 s7, *pt, r;

    ENGINE_BLK(802C7544);
    if (D_803F7C43 != 0)
        goto flames;
    ENGINE_BLK(802C755C);
    if (vs->unk96[0] != 1)
        goto flames;
    ENGINE_BLK(802C756C);
    if (D_803F7C3E == 0)
        goto flames;
    ENGINE_BLK(802C757C);
    r = func_802584BC(9);
    ENGINE_BLK(802C7588);
    pt = (s32 *)func_802ABC88(9, 1);
    ENGINE_BLK(802C7598);
    func_80288284(4, pt[0], pt[1], pt[2], r);
    ENGINE_BLK(802C75AC);
flames:
    ENGINE_BLK(802C75B0);
    if (vs->unkA1 == 5)
        goto grow;
    ENGINE_BLK(802C75C0);
    if (D_803F7C3E == 0)
        goto shrink;
grow:
    ENGINE_BLK(802C75D0);
    s7 = D_803F7C3B + 1;
    if (!(s7 < 0x33)) {
        ENGINE_BLK(802C75E8);
        s7 = 0x32;
    }
    ENGINE_BLK(802C75EC);
    D_803F7C3B = s7;
    goto part;
shrink:
    ENGINE_BLK(802C75F8);
    s7 = D_803F7C3B - 1;
    if (s7 < 0) {
        ENGINE_BLK(802C760C);
        s7 = 0;
    }
    ENGINE_BLK(802C7610);
    D_803F7C3B = s7;
part:
    ENGINE_BLK(802C7618);
    s7 = (u32)(s32)D_803F7C3B / 4;
    ENGINE_BLK(802C7644);
    func_802A039C(1, s7, JB);
    /* the sparks, every few frames while the jets fire */
    ENGINE_BLK(802C7650);
    s7 = D_803F7C3A;
    if (s7 != 0) {
        ENGINE_BLK(802C7664);
        D_803F7C3A = s7 - 1;
        goto done;
    }
    ENGINE_BLK(802C7670);
    if (vs->unkA1 == 5) {
        ENGINE_BLK(802C769C);
        D_803F7C3A = 4;
    } else {
        ENGINE_BLK(802C7680);
        if (D_803F7C3E == 0)
            goto done;
        ENGINE_BLK(802C7690);
        D_803F7C3A = 1;
    }
    ENGINE_BLK(802C76A4);
    func_802A6274(T(D_802C3804), 0x15F90, 1, 9, 1, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
    ENGINE_BLK(802C76D0);
    func_802A6274(T(D_802C3804), 0x15F90, 1, 9, 2, 1, engine_ctx(14), engine_ctx(15), engine_ctx(16), engine_ctx(17),
                  engine_ctx(18), engine_ctx(19), engine_ctx(20), 1, 1);
done:
    ENGINE_BLK(802C76FC);
}

/* the boost buttons' presses, counted (D_803F7C44 for D_80370C1A,
   D_803F7C46 for D_80370C1B: odd while held), each within four frames of
   the last (D_803F7C45, D_803F7C47) */
REGS()
void func_802C770C(void) {
    u32 v0, v1;

    ENGINE_BLK(802C770C);
    v0 = D_803F7C45;
    if (v0 == 0) {
        ENGINE_BLK(802C77A0);
        D_803F7C44 = 0;
        D_803F7C45 = 4;
        goto second;
    }
    ENGINE_BLK(802C7724);
    v1 = D_803F7C44;
    if (v1 & 1) {
        ENGINE_BLK(802C7738);
        if (D_80370C1A != 0)
            goto count1;
        ENGINE_BLK(802C7748);
    } else {
        ENGINE_BLK(802C7764);
        if (D_80370C1A == 0)
            goto count1;
        ENGINE_BLK(802C7774);
    }
    D_803F7C45 = 4;
    D_803F7C44 = v1 + 1;
    goto second;
count1:
    ENGINE_BLK(802C7790);
    D_803F7C45 = v0 - 1;
second:
    ENGINE_BLK(802C77B4);
    v0 = D_803F7C47;
    if (v0 == 0) {
        ENGINE_BLK(802C7840);
        D_803F7C46 = 0;
        D_803F7C47 = 4;
        goto done;
    }
    ENGINE_BLK(802C77C4);
    v1 = D_803F7C46;
    if (v1 & 1) {
        ENGINE_BLK(802C77D8);
        if (D_80370C1B != 0)
            goto count2;
        ENGINE_BLK(802C77E8);
    } else {
        ENGINE_BLK(802C7804);
        if (D_80370C1B == 0)
            goto count2;
        ENGINE_BLK(802C7814);
    }
    D_803F7C47 = 4;
    D_803F7C46 = v1 + 1;
    goto done;
count2:
    ENGINE_BLK(802C7830);
    D_803F7C47 = v0 - 1;
done:
    ENGINE_BLK(802C7854);
}

/* its jets' tilts in the air: part 4 (D_803F7C2C) with the stick's x
   (within func_802C7C1C's limits, by the speed), part 3 (D_803F7C28) with
   its y (within func_802C7BC0's), both back to the middle otherwise */
REGS(gp)
void func_802C7864(VS *vs) {
    f32 f0, f2, f4, f6, g;
    s32 s5;

    ENGINE_BLK(802C7864);
    f0 = D_803F7C2C;
    if (D_80370C35 != 0)
        goto stick;
    ENGINE_BLK(802C7888);
    s5 = D_803F7C34;
    if (!(s5 < 0xFA0))
        goto stick;
    ENGINE_BLK(802C789C);
    if (!(s5 < -0xF9F))
        goto middle4;
stick:
    ENGINE_BLK(802C78A4);
    s5 = D_80370C2C;
    if (!(s5 < 0x1F)) {
        ENGINE_BLK(802C78E8);
        f0 += 0.02f;
        f2 = func_802C7C1C(1, vs);
        ENGINE_BLK(802C78FC);
        if (!(f0 <= f2)) {
            ENGINE_BLK(802C7908);
            f0 = f2;
        }
        goto set4;
    }
    ENGINE_BLK(802C78BC);
    if (!(s5 < -0x1E))
        goto middle4;
    ENGINE_BLK(802C78C4);
    f0 -= 0.02f;
    f2 = func_802C7C1C(0, vs);
    ENGINE_BLK(802C78D4);
    if (f0 < f2) {
        ENGINE_BLK(802C78E0);
        f0 = f2;
    }
    goto set4;
middle4:
    ENGINE_BLK(802C7910);
    if (f0 < 0.5f) {
        ENGINE_BLK(802C7930);
        f0 += 0.01f;
        if (!(f0 <= 0.5f)) {
            ENGINE_BLK(802C7940);
            f0 = 0.5f;
        }
    } else {
        ENGINE_BLK(802C7948);
        f0 -= 0.01f;
        if (f0 < 0.5f) {
            ENGINE_BLK(802C7958);
            f0 = 0.5f;
        }
    }
set4:
    ENGINE_BLK(802C795C);
    D_803F7C2C = f0;
    g = 1.0f - f0;
    ENGINE_LEAVE(10, engine_cvt_w_s(g * 100.0f));
    if (!(g <= 0.5f)) {
        ENGINE_BLK(802C79A4);
        func_802A0360(4, 2, JB, (g - 0.5f) * 2.0f);
        ENGINE_BLK(802C79D0);
    } else {
        ENGINE_BLK(802C79D8);
        func_802A0360(4, 1, JB, g * 2.0f);
    }
    ENGINE_BLK(802C79FC);
    f0 = D_803F7C28;
    if (f0 <= 0.5f) {
        ENGINE_BLK(802C7A20);
        f6 = (0.5f - (0.5f - f0)) * 2.0f;
    } else {
        ENGINE_BLK(802C7A40);
        f6 = (0.5f - (f0 - 0.5f)) * 2.0f;
    }
    ENGINE_BLK(802C7A5C);
    f2 = 0.03f * f6;
    s5 = D_80370C2D;
    if (s5 == 0)
        goto middle3;
    ENGINE_BLK(802C7A7C);
    if (s5 > 0) {
        ENGINE_BLK(802C7AB0);
        f4 = func_802C7BC0();
        ENGINE_BLK(802C7AB8);
        f6 = 1.0f - f4;
        if (!(f0 <= f6))
            goto middle3;
        ENGINE_BLK(802C7AD4);
        f0 += f2;
        if (!(f0 <= f6)) {
            ENGINE_BLK(802C7AE4);
            f0 = f6;
        }
        goto set3;
    }
    ENGINE_BLK(802C7A84);
    f4 = func_802C7BC0();
    ENGINE_BLK(802C7A8C);
    if (f0 < f4)
        goto middle3;
    ENGINE_BLK(802C7A98);
    f0 -= f2;
    if (f0 < f4) {
        ENGINE_BLK(802C7AA8);
        f0 = f4;
    }
    goto set3;
middle3:
    ENGINE_BLK(802C7AEC);
    if (f0 < 0.5f) {
        ENGINE_BLK(802C7B0C);
        f0 += 0.005f;
        if (!(f0 <= 0.5f)) {
            ENGINE_BLK(802C7B1C);
            f0 = 0.5f;
        }
    } else {
        ENGINE_BLK(802C7B24);
        f0 -= 0.005f;
        if (f0 < 0.5f) {
            ENGINE_BLK(802C7B34);
            f0 = 0.5f;
        }
    }
set3:
    ENGINE_BLK(802C7B38);
    D_803F7C28 = f0;
    if (!(f0 <= 0.5f)) {
        ENGINE_BLK(802C7B58);
        func_802A0360(3, 2, JB, (f0 - 0.5f) * 2.0f);
        ENGINE_BLK(802C7B84);
    } else {
        ENGINE_BLK(802C7B8C);
        func_802A0360(3, 1, JB, f0 * 2.0f);
    }
    ENGINE_BLK(802C7BB0);
}

/* the stick's y: (1 - |y| / 80) / 2 */
REGS(-> f4)
f32 func_802C7BC0(void) {
    s32 v = D_80370C2D;
    f32 f4;

    ENGINE_BLK(802C7BC0);
    if (v < 0) {
        ENGINE_BLK(802C7BE8);
        v = -v;
    }
    ENGINE_BLK(802C7BEC);
    f4 = (f32)v / 80.0f;
    f4 = 1.0f - f4;
    f4 = f4 / 2.0f;
    ENGINE_LEAVE(2, v);
    ENGINE_LEAVE_F(6, 2.0f);
    return f4;
}

/* part 4's limit toward `up` (0 down, 1 up): 0.5 -/+ the speed / 540,
   within 0.1 and 0.9 */
REGS(s2, gp -> f2)
f32 func_802C7C1C(s32 up, VS *vs) {
    s32 s = vs->unk76;
    f32 f2;

    ENGINE_BLK(802C7C1C);
    if (s < 0) {
        ENGINE_BLK(802C7C34);
        s = -s;
    }
    ENGINE_BLK(802C7C38);
    f2 = (f32)s / 270.0f;
    f2 = f2 * 0.5f;
    if (up != 0) {
        ENGINE_BLK(802C7C64);
        f2 = f2 + 0.5f;
    } else {
        ENGINE_BLK(802C7C6C);
        f2 = 0.5f - f2;
    }
    ENGINE_BLK(802C7C70);
    if (!(f2 <= 0.9f)) {
        ENGINE_BLK(802C7C84);
        f2 = 0.9f;
    }
    ENGINE_BLK(802C7C88);
    if (f2 < 0.1f) {
        ENGINE_BLK(802C7C9C);
        f2 = 0.1f;
    }
    ENGINE_BLK(802C7CA0);
    ENGINE_LEAVE(19, 0x10E);
    ENGINE_LEAVE_F(4, 0.1f);
    return f2;
}

/* the J-Bomb's matrix, its vertices and (unless landed) its collision */
REGS(gp)
void func_802C7CB0(VS *vs) {
    u8 *model = D_803F7C04, *buf;
    s32 *m, off;

    ENGINE_BLK(802C7CB0);
    off = *(s32 *)(model + *(s32 *)(model + 0x18) + 4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C7CE0);
        m = (s32 *)(D_803F7C08 + off);
    } else {
        ENGINE_BLK(802C7CF4);
        m = (s32 *)(D_803F7C0C + off);
    }
    ENGINE_BLK(802C7D04);
    D_803ED390[0] = 0;
    D_803ED390[2] = 0;
    D_803ED390[1] = vs->unk4C;
    ENGINE_LEAVE(10, vs->unk4C);
    ENGINE_LEAVE(20, D_803F7BF8);
    ENGINE_LEAVE(21, D_803F7BFC);
    ENGINE_LEAVE(22, D_803F7C00);
    ENGINE_LEAVE(23, 0x4268);
    func_802AA764(D_803F7BF8, D_803F7BFC, D_803F7C00, 0x4268, m);
    ENGINE_LEAVE(18, T(m));           /* (its $s2, as the glue would) */
    ENGINE_BLK(802C7D48);
    if (D_8035805C != 0) {
        ENGINE_BLK(802C7D5C);
        buf = D_803F7C08;
    } else {
        ENGINE_BLK(802C7D6C);
        buf = D_803F7C0C;
    }
    ENGINE_BLK(802C7D78);
    model = D_803F7C04;
    if (vs->unkA1 != 5) {
        ENGINE_BLK(802C7DAC);
        ENGINE_LEAVE(11, T(model));
        func_8029C454(D_803F7BF8, D_803F7BFC, D_803F7C00, 9, model + *(s32 *)(model + 4),
                      model + *(s32 *)(model + 8), buf);
    }
    ENGINE_BLK(802C7DCC);
    ENGINE_LEAVE(11, T(model));
    func_802ABBEC(9, model + *(s32 *)(model + 0), model + *(s32 *)(model + 4), buf);
    ENGINE_BLK(802C7DEC);
}

/* the turn rate: walking or landed, the speed / 1.5; in the air, 20, or
   growing by 2 to 100 while the stick is pushed sideways */
REGS(gp -> s3)
s32 func_802C7DFC(VS *vs) {
    s32 v = vs->unkA1, s3;

    ENGINE_BLK(802C7DFC);
    if (v == 0)
        goto ground;
    ENGINE_BLK(802C7E10);
    if (v == 1)
        goto air;
    ENGINE_BLK(802C7E18);
    if (v == 2)
        goto air;
    ENGINE_BLK(802C7E20);
    if (v == 3)
        goto air;
    ENGINE_BLK(802C7E28);
    if (v == 4)
        goto air;
    ENGINE_BLK(802C7E30);
    if (v == 5)
        goto ground;
    ENGINE_BLK(802C7E38);
    engine_trap(0x802C7E38);
air:
    ENGINE_BLK(802C7E3C);
    v = D_80370C2C;
    if (v < 0x33) {
        ENGINE_BLK(802C7E50);
        if (!(v < -0x32)) {
            ENGINE_BLK(802C7E84);
            s3 = 0x14;
            D_803F7C3C = 0x14;
            goto done;
        }
    }
    ENGINE_BLK(802C7E58);
    v = D_803F7C3C + 2;
    if (!(v < 0x65)) {
        ENGINE_BLK(802C7E70);
        v = 0x64;
    }
    ENGINE_BLK(802C7E74);
    D_803F7C3C = v;
    s3 = v;
    goto done;
ground:
    ENGINE_BLK(802C7E94);
    s3 = engine_cvt_w_s((f32)vs->unk76 / 1.5f);
done:
    ENGINE_BLK(802C7EBC);
    return s3;
}

/* the camera's distance: four times as far while falling (mode 2) */
REGS(gp)
void func_802C7ECC(VS *vs) {
    f32 f0 = D_803EBBF0, f2;

    ENGINE_BLK(802C7ECC);
    if (vs->unkA1 == 2) {
        ENGINE_BLK(802C7EFC);
        f2 = 4.0f;
    } else {
        ENGINE_BLK(802C7EF0);
        f2 = 1.0f;
    }
    ENGINE_BLK(802C7F08);
    f0 = f0 * f2;
    D_803EBBF4 = f0;
    ENGINE_LEAVE_F(0, f0);
    ENGINE_LEAVE_F(2, f2);
}

/* the camera's speed, and its gears and brake: walking or in the air */
REGS(gp)
void func_802C7F28(VS *vs) {
    s16 *r = vs->unk78;

    ENGINE_BLK(802C7F28);
    D_803ED3F6 = 0xFF;
    D_803ED3F7 = 0xFF;
    if (vs->unkA1 != 0) {
        ENGINE_BLK(802C7F58);
        r[0] = -0xC8, r[1] = 0, r[2] = 2;
        r[3] = 0, r[4] = 0xFA, r[5] = 2;
        r[6] = 0, r[7] = 0xFA, r[8] = 2;
        r[9] = 0, r[10] = 0xFA, r[11] = 2;
        r[12] = 0, r[13] = 0xFA, r[14] = 2;
        D_803F7C48 = 4;
        ENGINE_LEAVE(9, 4);
    } else {
        ENGINE_BLK(802C7FE0);
        r[0] = -0x64, r[1] = 0, r[2] = 4;
        r[3] = 0, r[4] = 0x96, r[5] = 4;
        r[6] = 0, r[7] = 0x96, r[8] = 4;
        r[9] = 0, r[10] = 0x96, r[11] = 4;
        r[12] = 0, r[13] = 0x96, r[14] = 4;
        D_803F7C48 = 0xC;
        ENGINE_LEAVE(9, 0xC);
    }
    ENGINE_BLK(802C8064);
    ENGINE_LEAVE(3, 0xFF);
}

/* its state and position saved to dst (0xB2 bytes) */
void func_802C8074(u8 *dst) {
    ENGINE_BLK(802C8074);
    func_802AC7DC(dst, (u8 *)&D_803F7B50, (u32 *)&D_803F7BF8);
    ENGINE_BLK(802C8090);
}

/* and back */
void func_802C80A0(u8 *src) {
    ENGINE_BLK(802C80A0);
    func_802AC85C(src, (u8 *)&D_803F7B50, (u32 *)&D_803F7BF8);
    ENGINE_BLK(802C80BC);
}
