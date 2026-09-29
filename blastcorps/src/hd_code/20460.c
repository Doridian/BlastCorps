#include "common.h"
#include "game/camera.h"
#include "game/vehicle.h"
#include "game/frame.h"
#include "game/level.h"
#include "game/game.h"
#include "game/audio.h"
#include "game/sched.h"

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u8 unk12;
    /* 0x13 */ u8 unk13;
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 unk1B;
    /* 0x1C */ u8 unk1C;
    /* 0x20 */ f32 unk20;
} UnkStruct_80367D60; /* size = 0x24 */

/* ALSynConfig, with a u8 fxType. */
typedef struct {
    /* 0x00 */ u8 unk0[0x12];
    /* 0x12 */ u8 unk12;
} UnkStruct_80267614;


SndState *func_80260650(SndBank *bank, s16 id, SndState **handle);
void func_80265428(void);
void func_8026513C(void);
s32 func_80265A0C(s32 arg0);
void func_80265B7C(s32 arg0);
void func_80265E48(void);
s32 func_8026A610(s32, s32, s32, s32);
s32 func_8026A8E0(s32, s32);
void func_8026AD30(s32);

extern s32 osViClock;
extern u16 D_803C30A8[];

/* .bss, 0x80367D60-0x80368050 (tools/bss_c.py) */
UnkStruct_80367D60 D_80367D60[20];
s32 D_80368030;
s16 D_80368034;
s16 D_80368036;
s32 D_80368038;
s32 D_8036803C;
s32 D_80368040;
s32 D_80368044;
s32 D_80368048;

/* .data, 0x802E9FB0-0x802F3AF0 (tools/data_c.py) */
extern Vtx D_802E9FB0[4];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802E9FB0.inc"
#else
Vtx D_802E9FB0[4] = { 0 };
#endif
extern u16 D_802E9FF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802E9FF0.inc"
#else
u16 D_802E9FF0[0x280] = { 0 };
#endif
extern u16 D_802EA4F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EA4F0.inc"
#else
u16 D_802EA4F0[0x280] = { 0 };
#endif
extern u16 D_802EA9F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EA9F0.inc"
#else
u16 D_802EA9F0[0x280] = { 0 };
#endif
extern u16 D_802EAEF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EAEF0.inc"
#else
u16 D_802EAEF0[0x280] = { 0 };
#endif
extern u16 D_802EB3F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EB3F0.inc"
#else
u16 D_802EB3F0[0x280] = { 0 };
#endif
extern u16 D_802EB8F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EB8F0.inc"
#else
u16 D_802EB8F0[0x280] = { 0 };
#endif
extern u16 D_802EBDF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EBDF0.inc"
#else
u16 D_802EBDF0[0x280] = { 0 };
#endif
extern u16 D_802EC2F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EC2F0.inc"
#else
u16 D_802EC2F0[0x280] = { 0 };
#endif
extern u16 D_802EC7F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EC7F0.inc"
#else
u16 D_802EC7F0[0x280] = { 0 };
#endif
extern u16 D_802ECCF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802ECCF0.inc"
#else
u16 D_802ECCF0[0x280] = { 0 };
#endif
extern u16 D_802ED1F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802ED1F0.inc"
#else
u16 D_802ED1F0[0x280] = { 0 };
#endif
extern u16 D_802ED6F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802ED6F0.inc"
#else
u16 D_802ED6F0[0x280] = { 0 };
#endif
extern u16 D_802EDBF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EDBF0.inc"
#else
u16 D_802EDBF0[0x280] = { 0 };
#endif
extern u16 D_802EE0F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EE0F0.inc"
#else
u16 D_802EE0F0[0x280] = { 0 };
#endif
extern u16 D_802EE5F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EE5F0.inc"
#else
u16 D_802EE5F0[0x280] = { 0 };
#endif
extern u16 D_802EEAF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EEAF0.inc"
#else
u16 D_802EEAF0[0x280] = { 0 };
#endif
extern u16 D_802EEFF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EEFF0.inc"
#else
u16 D_802EEFF0[0x280] = { 0 };
#endif
extern u16 D_802EF4F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EF4F0.inc"
#else
u16 D_802EF4F0[0x280] = { 0 };
#endif
extern u16 D_802EF9F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EF9F0.inc"
#else
u16 D_802EF9F0[0x280] = { 0 };
#endif
extern u16 D_802EFEF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802EFEF0.inc"
#else
u16 D_802EFEF0[0x280] = { 0 };
#endif
extern u16 D_802F03F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F03F0.inc"
#else
u16 D_802F03F0[0x280] = { 0 };
#endif
extern u16 D_802F08F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F08F0.inc"
#else
u16 D_802F08F0[0x280] = { 0 };
#endif
extern u16 D_802F0DF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F0DF0.inc"
#else
u16 D_802F0DF0[0x280] = { 0 };
#endif
extern u16 D_802F12F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F12F0.inc"
#else
u16 D_802F12F0[0x280] = { 0 };
#endif
extern u16 D_802F17F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F17F0.inc"
#else
u16 D_802F17F0[0x280] = { 0 };
#endif
extern u16 D_802F1CF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F1CF0.inc"
#else
u16 D_802F1CF0[0x280] = { 0 };
#endif
extern u16 D_802F21F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F21F0.inc"
#else
u16 D_802F21F0[0x280] = { 0 };
#endif
extern u16 D_802F26F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F26F0.inc"
#else
u16 D_802F26F0[0x280] = { 0 };
#endif
extern u16 D_802F2BF0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F2BF0.inc"
#else
u16 D_802F2BF0[0x280] = { 0 };
#endif
extern u16 D_802F30F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F30F0.inc"
#else
u16 D_802F30F0[0x280] = { 0 };
#endif
extern u16 D_802F35F0[0x280];
#ifdef ASSET_INIT
#include "hd_code/20460/D_802F35F0.inc"
#else
u16 D_802F35F0[0x280] = { 0 };
#endif

void func_80264C20(s32 arg0) {
    s32 i;

    for (i = 0; i < 20; i++) {
        D_80367D60[i].unk15 = 0;
    }
    D_8036B968 = osGetCount();
#ifdef TARGET_PC
    port_replay_seeded();
#endif
    D_80368038 = 99999999;
    if (arg0 != 0) {
        D_8036EA70.cr = D_80368040;
    } else {
        D_8036EA70.cr = 0;
    }
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/20460/func_80264CB4.s")
#else
void func_80264CB4(s16 arg0, s16 arg1, s16 arg2, s16 arg3, u8 arg4, s32 arg5) {
    s32 unused;
    s32 count;
    s32 i;
    u8 found;
    s16 r;

    i = 0;
    count = 0;
    D_8036EA70.cr += arg5;
    if (arg5 != 0 && D_802E8BD0 == 0) {
        func_8026AD30(0x48);
    }
    if (arg3 > 200) {
        arg3 = 200;
    }
    while (count < arg5 && i < 20) {
        if (count == 2) {
            func_80260650(D_80367738, 0x24, NULL);
        }
        found = 0;
        while (i < 20 && found == 0) {
            if (D_80367D60[i].unk15 == 0) {
                found = 1;
            } else {
                i++;
            }
        }
        if (found) {
            D_80367D60[i].unk0 = arg0;
            D_80367D60[i].unk4 = arg2;
            D_80367D60[i].unk2 = arg1;
            r = arg3 / 5;
            D_80367D60[i].unkA = func_8026A8E0(-r, r) + (arg0 - arg3);
            D_80367D60[i].unkC = func_8026A8E0(-r, r) + (arg2 + arg3);
            D_80367D60[i].unk10 = arg3 * 3 / 2;
            D_80367D60[i].unkE = arg3;
            D_80367D60[i].unk6 = D_80367D60[i].unkA + func_8026A8E0(-arg3, arg3);
            D_80367D60[i].unk8 = D_80367D60[i].unkC + func_8026A8E0(-arg3, arg3);
            D_80367D60[i].unk13 = 0;
            D_80367D60[i].unk14 = 0;
            D_80367D60[i].unk16 = func_8026A8E0(-500, 500);
            D_80367D60[i].unk12 = arg4;
            D_80367D60[i].unk1C = 0;
            D_80367D60[i].unk15 = 5;
            D_80367D60[i].unk18 = 4000;
            D_80367D60[i].unk20 = 1e+08f;
            D_80367D60[i].unk1B = 0;
        }
        i++;
        count++;
    }
}
#endif

void func_8026510C(void) {
    func_80265428();
    func_8026513C();
    func_80265E48();
}

void func_8026513C(void) {
    s16 dx;
    s16 dz;
    s16 adx;
    s16 adz;
    f32 dist;
    s16 sx;
    s16 sz;
    s32 i;

    for (i = 0; i < 20; i++) {
        if (D_80367D60[i].unk15 == 5) {
            dx = D_80367D60[i].unk6 - D_80367D60[i].unk0;
            if (dx >= 0) {
                adx = dx;
            } else {
                adx = -dx;
            }
            dz = D_80367D60[i].unk8 - D_80367D60[i].unk4;
            if (dz >= 0) {
                adz = dz;
            } else {
                adz = -dz;
            }
            dist = func_8026A610(D_80367D60[i].unk6, D_80367D60[i].unk8, D_80367D60[i].unk0, D_80367D60[i].unk4);
            if (dist < 1.0 || D_80367D60[i].unk20 < dist) {
                D_80367D60[i].unk15 = 1;
            } else {
                D_80367D60[i].unk20 = dist;
                sx = adx / dist * 5.0f;
                sz = adz / dist * 5.0f;
                if (dx >= 0) {
                    D_80367D60[i].unk0 = D_80367D60[i].unk0 + sx;
                } else {
                    D_80367D60[i].unk0 -= sx;
                }
                if (dz >= 0) {
                    D_80367D60[i].unk4 += sz;
                } else {
                    D_80367D60[i].unk4 -= sz;
                }
            }
        }
    }
}

void func_80265428(void) {
    s32 i;
    s32 unused;
    s32 x1;
    s32 z1;
    s32 x2;
    s32 z2;
    s32 d1;
    s32 d2;

    for (i = 0; i < 20; i++) {
        if ((D_80367D60[i].unk15 == 1 || D_80367D60[i].unk15 == 4 || D_80367D60[i].unk15 == 2) &&
            func_80265A0C(i) != 0) {
            func_80265B7C(i);
            x1 = D_80367D60[i].unk0 + D_80368034;
            z1 = D_80367D60[i].unk4 + D_80368036;
            x2 = D_80367D60[i].unk0 - D_80368034;
            z2 = D_80367D60[i].unk4 - D_80368036;
            if (x1 < D_80367D60[i].unkA - D_80367D60[i].unkE) {
                x1 = D_80367D60[i].unkA - D_80367D60[i].unkE;
            }
            if (z1 < D_80367D60[i].unkC - D_80367D60[i].unkE) {
                z1 = D_80367D60[i].unkC - D_80367D60[i].unkE;
            }
            if (x1 >= D_80367D60[i].unkA + D_80367D60[i].unkE) {
                x1 = D_80367D60[i].unkA + D_80367D60[i].unkE - 1;
            }
            if (z1 >= D_80367D60[i].unkC + D_80367D60[i].unkE) {
                z1 = D_80367D60[i].unkC + D_80367D60[i].unkE - 1;
            }
            if (x2 < D_80367D60[i].unkA - D_80367D60[i].unkE) {
                x2 = D_80367D60[i].unkA - D_80367D60[i].unkE;
            }
            if (z2 < D_80367D60[i].unkC - D_80367D60[i].unkE) {
                z2 = D_80367D60[i].unkC - D_80367D60[i].unkE;
            }
            if (x2 >= D_80367D60[i].unkA + D_80367D60[i].unkE) {
                x2 = D_80367D60[i].unkA + D_80367D60[i].unkE - 1;
            }
            if (z2 >= D_80367D60[i].unkC + D_80367D60[i].unkE) {
                z2 = D_80367D60[i].unkC + D_80367D60[i].unkE - 1;
            }
            d1 = func_8026A610(D_803643E0 >> 5, D_803643E8 >> 5, x1, z1);
            d2 = func_8026A610(D_803643E0 >> 5, D_803643E8 >> 5, x2, z2);
            if (D_80367D60[i].unk1B != 0) {
                if (D_80367D60[i].unk1A != 0) {
                    d2 = 0;
                } else {
                    d1 = 0;
                }
                D_80367D60[i].unk1B--;
            }
            if (d1 < d2) {
                D_80367D60[i].unk0 = x2;
                D_80367D60[i].unk4 = z2;
                D_80367D60[i].unk18 += 0x800;
                if (D_80367D60[i].unk18 >= 0x1000) {
                    D_80367D60[i].unk18 -= 0xFFF;
                }
                if (D_80367D60[i].unk1A != 0) {
                    D_80367D60[i].unk1B = 5;
                }
                D_80367D60[i].unk1A = 0;
            } else {
                D_80367D60[i].unk0 = x1;
                D_80367D60[i].unk4 = z1;
                if (D_80367D60[i].unk1A == 0) {
                    D_80367D60[i].unk1B = 5;
                }
                D_80367D60[i].unk1A = 1;
            }
            if (i == 1 && D_80367D60[i].unk15 != 4) {
                func_80260650(D_80367738, 0x24, NULL);
            }
            if (D_80367D60[i].unk15 == 2) {
                D_803EF32C = 6;
            }
            D_80367D60[i].unk15 = 4;
        } else if (D_80367D60[i].unk15 == 4) {
            D_80367D60[i].unk15 = 1;
        }
    }
}

s32 func_80265A0C(s32 arg0) {
    s16 px;
    s16 pz;
    s16 x;
    s16 z;

    x = D_80367D60[arg0].unkA - D_80367D60[arg0].unk10;
    z = D_80367D60[arg0].unkC - D_80367D60[arg0].unk10;
    px = D_803643E0 >> 5, pz = D_803643E8 >> 5;
    if (px < x || pz < z) {
        return 0;
    }
    x = D_80367D60[arg0].unkA + D_80367D60[arg0].unk10;
    z = D_80367D60[arg0].unkC + D_80367D60[arg0].unk10;
    if (px >= x || pz >= z) {
        return 0;
    }
    return 1;
}

void func_80265B7C(s32 arg0) {
    s16 angle;
    s16 a;
    s16 sn;
    s16 cs;
    s16 sx;
    s16 sz;
    s32 max;

    max = D_8036443C / 40;
    if (max < 0) {
        max = -max;
    }
    if (D_8036443C != 0 && max == 0) {
        max = 2;
    }
    angle = D_80367D60[arg0].unk16 + D_8036443E + 0x400;
    if (angle >= 0x1000) {
        angle = angle - 0xFFF;
    }
    D_80367D60[arg0].unk18 = angle;
    a = angle % 1024;
    sn = sins(a * 0xFFFF / 4095);
    cs = coss(a * 0xFFFF / 4095);
    if (D_80367D60[arg0].unk1C < max) {
        D_80367D60[arg0].unk1C += 1;
    }
    if (D_80367D60[arg0].unk1C > max) {
        D_80367D60[arg0].unk1C -= 1;
    }
    sx = (D_80367D60[arg0].unk1C * sn) >> 15;
    sz = (D_80367D60[arg0].unk1C * cs) >> 15;
    if (angle < 0x400) {
        D_80368034 = sx;
        D_80368036 = sz;
    }
    if (angle >= 0x400 && angle < 0x800) {
        D_80368034 = sz;
        D_80368036 = -sx;
    }
    if (angle >= 0x800 && angle < 0xC00) {
        D_80368034 = -sx;
        D_80368036 = -sz;
    }
    if (angle >= 0xC00) {
        D_80368034 = -sz;
        D_80368036 = sx;
    }
}

void func_80260AB8(SndState *state, s16 type, s32 param);


void func_80265E48(void) {
    s32 i;
    s32 nearest;
    s32 nearestDist;
    s32 farthest;
    s32 farthestDist;
    s32 dist;
    u8 found;
    u8 useFar;
    s32 volume;

    nearest = -1, nearestDist = 99999999, farthest = -1;
    farthestDist = 0;
    useFar = 0;
    if (D_803EF32D != 0) {
        i = 0;
        found = 0;
        do {
            if (D_80367D60[i].unk15 == 2) {
                D_80367D60[i].unk15 = 3;
                D_80367D60[i].unk13 = 0;
                found = 1;
            } else {
                i++;
            }
        } while (!found);
        D_803EF32D = 0;
        D_80368038 = 99999999;
    } else if (--D_80368038 == 0) {
        D_803EF32C = 6;
        D_80367D60[D_8036803C].unk15 = 1;
        useFar = 1;
    }
    if (D_803EF32C == 0) {
        for (i = 0; i < 20; i++) {
            if (D_80367D60[i].unk15 == 3) {
                D_80367D60[i].unk15 = 0;
            }
        }
        for (i = 0; i < 20; i++) {
            if (D_80367D60[i].unk15 == 1) {
                dist = func_8026A610(D_803EF2EC, D_803EF2F4, D_80367D60[i].unk0 << 5, D_80367D60[i].unk4 << 5);
                if (dist < nearestDist) {
                    nearest = i;
                    nearestDist = dist;
                }
                if (dist > farthestDist) {
                    farthest = i;
                    farthestDist = dist;
                }
            }
        }
        if (nearest != -1) {
            if (useFar) {
                i = farthest;
            } else {
                i = nearest;
            }
            D_803EF308 = D_80367D60[i].unk0 << 5;
            D_803EF30C = D_80367D60[i].unk4 << 5;
            D_80368030 = D_80367D60[i].unk2 << 5;
            D_80367D60[i].unk15 = 2;
            D_80368038 = 600;
            D_803EF32C = 1;
            D_8036803C = i;
            if (35000 - func_8026A610(D_803643E0, D_803643E8, D_803EF308, D_803EF30C) * 2 >= 0x8000) {
                volume = 0x7FFF;
            } else {
                volume = 35000 - func_8026A610(D_803643E0, D_803643E8, D_803EF308, D_803EF30C) * 2;
            }
            if (volume > 4000) {
                func_80260AB8(func_80260650(D_80367738, 0x25, NULL), 8, volume);
            }
        }
    }
}

void func_802661EC(void) {
    D_803EF308 = D_803EF6DC;
    D_803EF30C = D_803EF6E4;
    D_80368044 = D_803643E0;
    D_80368048 = D_803643E8;
    D_803EF32C = 1;
    D_80368038 = 2000;
}

/* The segment 2 buffer as this function uses it. */
extern FrameGame D_02000000;
extern f32 D_80364414;

s32 func_80267614(UnkStruct_80267614 *arg0);
void func_8026A5CC(u64 *dst, u64 *src, s32 size);

void func_80266248(Gfx **gfxp, FrameGame *arg1) {
    Gfx *gfx;
    s32 vtxIdx;
    s32 i;
    s32 k;
    s32 j;
    u8 found;
    u8 frame;
    u16 *tex;
    u32 phys;
    u8 flip;
    f32 mf[4][4];
    f32 x[4];
    f32 y[4];
    f32 z[4];
    s16 angle;

    gfx = *gfxp;
    vtxIdx = 0;
    guRotateF(mf, D_80364414 - 135.0, 0.0f, 1.0f, 0.0f);
    for (k = 0; k < 4; k++) {
        guMtxXFMF(mf, D_802E9FB0[k].v.ob[0], D_802E9FB0[k].v.ob[1], D_802E9FB0[k].v.ob[2], &x[k], &y[k], &z[k]);
    }
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    for (i = 0; i < 20; i++) {
        if (func_80267614((UnkStruct_80267614 *)&D_80367D60[i]) &&
            (D_80367D60[i].unk15 == 1 || D_80367D60[i].unk15 == 2 ||
             (D_80367D60[i].unk15 == 4 && D_80367D60[i].unk1C == 0))) {
            D_80367D60[i].unk14++;
            if (D_80367D60[i].unk14 >= 6) {
                D_80367D60[i].unk13++;
                D_80367D60[i].unk14 = 0;
            }
            if (D_80367D60[i].unk13 >= 6) {
                D_80367D60[i].unk13 = 0;
            }
            switch (D_80367D60[i].unk13) {
                case 0:
                    tex = D_802EA4F0;
                    break;
                case 1:
                    tex = D_802EA9F0;
                    break;
                case 2:
                    tex = D_802EAEF0;
                    break;
                case 3:
                    tex = D_802EB3F0;
                    break;
                case 4:
                    tex = D_802EB8F0;
                    break;
                case 5:
                    tex = D_802EBDF0;
                    break;
            }
            phys = osVirtualToPhysical(tex);
            gDPPipeSync(gfx++);
            gDPLoadTextureBlock(gfx++, phys, G_IM_FMT_RGBA, G_IM_SIZ_16b, 20, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            func_8026A5CC((u64 *)&arg1->unk1900[vtxIdx], (u64 *)D_802E9FB0, 0x40);
            for (j = 0; j < 4; j++) {
                arg1->unk1900[vtxIdx + j].v.ob[0] = x[j] + D_80367D60[i].unk0;
                arg1->unk1900[vtxIdx + j].v.ob[1] = y[j] + D_80367D60[i].unk2;
                arg1->unk1900[vtxIdx + j].v.ob[2] = z[j] + D_80367D60[i].unk4;
            }
            gSPVertex(gfx++, &D_02000000.unk1900[vtxIdx], 4, 0);
            vtxIdx += 4;
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
        }
    }
    for (i = 0; i < 20; i++) {
        if (((D_80367D60[i].unk15 == 4 && D_80367D60[i].unk1C != 0) || D_80367D60[i].unk15 == 5) &&
            func_80267614((UnkStruct_80267614 *)&D_80367D60[i])) {
            D_80367D60[i].unk14 += D_80367D60[i].unk1C;
            if (D_80367D60[i].unk14 >= 11) {
                D_80367D60[i].unk13++;
                D_80367D60[i].unk14 = 0;
            }
            if (D_80367D60[i].unk13 >= 8) {
                D_80367D60[i].unk13 = 0;
            }
            frame = D_80367D60[i].unk13;
            flip = 0;
            angle = D_80367D60[i].unk18 - (D_80364414 - 135.0) / 360.0 * 4095.0;
            if (angle < 0) {
                angle += 0xFFF;
            }
            if (angle >= 0xC00) {
                switch (frame) {
                    case 0:
                        tex = D_802EEAF0;
                        break;
                    case 1:
                        tex = D_802EEFF0;
                        break;
                    case 2:
                        tex = D_802EF4F0;
                        break;
                    case 3:
                        tex = D_802EF9F0;
                        break;
                    case 4:
                        tex = D_802EFEF0;
                        break;
                    case 5:
                        tex = D_802F03F0;
                        break;
                    case 6:
                        tex = D_802F08F0;
                        break;
                    case 7:
                        tex = D_802F0DF0;
                        break;
                }
            }
            if (angle >= 0x800 && angle < 0xC00) {
                flip = 1;
                switch (frame) {
                    case 0:
                        tex = D_802F12F0;
                        break;
                    case 1:
                        tex = D_802F17F0;
                        break;
                    case 2:
                        tex = D_802F1CF0;
                        break;
                    case 3:
                        tex = D_802F21F0;
                        break;
                    case 4:
                        tex = D_802F26F0;
                        break;
                    case 5:
                        tex = D_802F2BF0;
                        break;
                    case 6:
                        tex = D_802F30F0;
                        break;
                    case 7:
                        tex = D_802F35F0;
                        break;
                }
            }
            if (angle >= 0x400 && angle < 0x800) {
                switch (frame) {
                    case 0:
                        tex = D_802EC2F0;
                        break;
                    case 1:
                        tex = D_802EC7F0;
                        break;
                    case 2:
                        tex = D_802ECCF0;
                        break;
                    case 3:
                        tex = D_802ED1F0;
                        break;
                    case 4:
                        tex = D_802ED6F0;
                        break;
                    case 5:
                        tex = D_802EDBF0;
                        break;
                    case 6:
                        tex = D_802EE0F0;
                        break;
                    case 7:
                        tex = D_802EE5F0;
                        break;
                }
            }
            if (angle < 0x400) {
                switch (frame) {
                    case 0:
                        tex = D_802F12F0;
                        break;
                    case 1:
                        tex = D_802F17F0;
                        break;
                    case 2:
                        tex = D_802F1CF0;
                        break;
                    case 3:
                        tex = D_802F21F0;
                        break;
                    case 4:
                        tex = D_802F26F0;
                        break;
                    case 5:
                        tex = D_802F2BF0;
                        break;
                    case 6:
                        tex = D_802F30F0;
                        break;
                    case 7:
                        tex = D_802F35F0;
                        break;
                }
            }
            phys = osVirtualToPhysical(tex);
            gDPPipeSync(gfx++);
            gDPLoadTextureBlock(gfx++, phys, G_IM_FMT_RGBA, G_IM_SIZ_16b, 20, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            func_8026A5CC((u64 *)&arg1->unk1900[vtxIdx], (u64 *)D_802E9FB0, 0x40);
            if (flip) {
                arg1->unk1900[vtxIdx].v.tc[0] = 0x260;
                arg1->unk1900[vtxIdx + 1].v.tc[0] = 0;
                arg1->unk1900[vtxIdx + 2].v.tc[0] = 0;
                arg1->unk1900[vtxIdx + 3].v.tc[0] = 0x260;
            }
            for (j = 0; j < 4; j++) {
                arg1->unk1900[vtxIdx + j].v.ob[0] = x[j] + D_80367D60[i].unk0;
                arg1->unk1900[vtxIdx + j].v.ob[1] = y[j] + D_80367D60[i].unk2;
                arg1->unk1900[vtxIdx + j].v.ob[2] = z[j] + D_80367D60[i].unk4;
            }
            gSPVertex(gfx++, &D_02000000.unk1900[vtxIdx], 4, 0);
            vtxIdx += 4;
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
        }
    }
    if (D_803EF32E == 0) {
        i = 0;
        found = 0;
        do {
            if (D_80367D60[i].unk15 == 3) {
                found = 1;
                gDPPipeSync(gfx++);
                gDPLoadTextureBlock(gfx++, osVirtualToPhysical(D_802E9FF0), G_IM_FMT_RGBA, G_IM_SIZ_16b, 20, 32, 0,
                                    G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                                    G_TX_NOLOD, G_TX_NOLOD);
                func_8026A5CC((u64 *)&arg1->unk1900[vtxIdx], (u64 *)D_802E9FB0, 0x40);
                for (j = 0; j < 4; j++) {
                    arg1->unk1900[vtxIdx + j].v.ob[0] = x[j];
                    arg1->unk1900[vtxIdx + j].v.ob[1] = y[j];
                    arg1->unk1900[vtxIdx + j].v.ob[2] = z[j];
                }
                guTranslate(&arg1->unk0[8], D_803EF310 / 32.0f, D_803EF314 / 32.0f, D_803EF318 / 32.0f);
                gSPMatrix(gfx++, &D_02000000.unk0[8], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
                gSPVertex(gfx++, &D_02000000.unk1900[vtxIdx], 4, 0);
                vtxIdx += 4;
                gSP1Triangle(gfx++, 0, 1, 2, 0);
                gSP1Triangle(gfx++, 0, 2, 3, 0);
                gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
            } else {
                i++;
            }
        } while (i < 20 && found == 0);
    }
    gDPPipeSync(gfx++);
    *gfxp = gfx;
}

s32 func_80267614(UnkStruct_80267614 *arg0) {
    s32 i;
    u8 id;

    i = 0;
    id = arg0->unk12;
    while (D_803C30A8[i] != 0xFFFF) {
        if (D_803C30A8[i++] == id) {
            return 1;
        }
    }
    return 0;
}
