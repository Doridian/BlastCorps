#include "common.h"
#include "game/camera.h"
#include "game/vehicle.h"
#include "game/audio.h"

#define ABS(x) ((x) > 0 ? (x) : -(x))

extern f32 D_80364414;

f32 sqrtf(f32);
s32 func_802AD7D4(s32);
u16 func_8028BA1C(s16 arg0, s16 arg1, s8 *arg2, u8 arg3);
f32 func_8028BBF4(s16 arg0, s16 arg1, s16 arg2, s16 arg3);
f32 func_8028BD88(f32 arg0, f32 arg1);

/* .bss, 0x80370C70-0x8039AF00 (tools/bss_c.py) */
u16 D_80370C70;
s16 D_80370C72;
s8 D_80370C74;
u8 D_80370C75;
s16 D_80370C76;
u8 D_80370C78[8];
u8 D_80370C80[AUDIO_HEAP_SIZE];

void func_8028B720(void) {
    D_80370C75 = 0;
    D_803ED40A = 0;
}

void func_8028B734(s8 *arg0, s8 *arg1, u8 arg2) {
    s8 sp37;
    s8 sp36;
    s16 sp34;
    s16 sp32;
    s16 sp30;
    u16 sp2E;
    s16 sp2C;

    sp30 = -D_80370C70 * 16 + 0x7FF0;
    sp37 = *arg0;
    sp36 = *arg1;
    sp34 = (s16) ((360.0 - (D_80364414 + 180.0)) / 360.0 * 65536.0) +
           (func_8028BBF4(0, 0, sp37, -sp36) * 65536.0 / 360.0 + 32768.0);
    sp32 = sqrtf(sp37 * sp37 + sp36 * sp36);
    *arg1 = sp32;
    D_80370C74 = 0;
    sp2E = 0;
    if (D_80370C75 == 1) {
        sp2C = D_80370C76 - sp30;
        if (ABS(sp2C) <= ABS(D_80370C72 * 16)) {
            D_80370C75 = 0;
        }
        sp2E = 0x200;
    } else {
        if (*arg1 > 10) {
            sp2E = func_8028BA1C(sp30, sp34, &D_80370C74, arg2);
        }
        if (D_80370C74 != 0 && D_8036443C < 40) {
            D_80370C76 = sp34;
            D_80370C75 = 1;
        } else if (D_803ED40A != 0) {
            D_803ED408 = 0x7FF - sp34 / 16;
            *arg0 = 0;
            return;
        }
    }
    *arg0 = 0;
    if (sp2E == 0x200) {
        *arg0 = -80;
    }
    if (sp2E == 0x100) {
        *arg0 = 80;
    }
}

u16 func_8028BA1C(s16 arg0, s16 arg1, s8 *arg2, u8 arg3) {
    s16 spE;
    u16 spC;
    s16 spA;
    u16 sp8;

    sp8 = 90;
    spA = 120 - (ABS(D_803ED400) * (120 - sp8)) / 200;
    spC = 0x8000 - (((spA < sp8) ? sp8 : spA) << 16) / 720;
    if (*arg2 != 0) {
        arg0 += 0x8000;
    }
    spE = arg1 - arg0;
    D_803F7C34 = spE;
    if (ABS(spE) <= ABS(D_80370C72 * 16)) {
        D_803ED40A = 1;
    }
    if (spC < ABS(spE) && D_8036443C < 40) {
        *arg2 ^= -1;
        return 0;
    }
    if (((D_80370C72 >= 0) ? 1 : -1) * spE > 0) {
        return 0x100;
    }
    return 0x200;
}

f32 func_8028BBF4(s16 arg0, s16 arg1, s16 arg2, s16 arg3) {
    f32 sp1C;

    if (arg2 >= arg0 && arg3 >= arg1) {
        sp1C = func_8028BD88(arg3 - arg1, arg2 - arg0) + 90.0f;
    } else if (arg2 >= arg0 && arg3 < arg1) {
        sp1C = 90.0f - func_8028BD88(arg1 - arg3, arg2 - arg0);
    } else if (arg2 < arg0 && arg3 < arg1) {
        sp1C = func_8028BD88(arg1 - arg3, arg0 - arg2) + 270.0f;
    } else if (arg2 < arg0 && arg3 >= arg1) {
        sp1C = 270.0f - func_8028BD88(arg3 - arg1, arg0 - arg2);
    }
    return sp1C;
}

f32 func_8028BD88(f32 arg0, f32 arg1) {
    s32 sp1C;
    f32 sp18;

    if (arg0 == 0.0 && arg1 == 0.0) {
        return 0.0f;
    }
    sp18 = arg0 / sqrtf(arg0 * arg0 + arg1 * arg1);
    sp1C = func_802AD7D4(sp18 * 65536.0);
    return (f32) sp1C / 65536.0 * 360.0;
}

void func_8028BE70(u16 *arg0, s8 arg1, s8 arg2) {
    if (!(*arg0 & 0x200) && arg1 < -50) {
        *arg0 |= 0x200;
    }
    if (!(*arg0 & 0x100) && arg1 > 50) {
        *arg0 |= 0x100;
    }
    if (!(*arg0 & 0x8000) && arg2 < -50) {
        *arg0 |= 0x8000;
    }
    if (!(*arg0 & 0x4000) && arg2 > 50) {
        *arg0 |= 0x4000;
    }
    if (*arg0 & 0x2000) {
        *arg0 |= 0x30;
    }
}

void func_8028BF34(u16 *arg0, s8 arg1, s8 arg2, u8 arg3) {
    s16 sp2E;
    s16 sp2C;
    s16 sp2A;
    u16 sp28;

    sp2E = func_8028BBF4(0, 0, arg1, -arg2) * 65536.0 / 360.0 + 32768.0;
    sp2C = sqrtf(arg1 * arg1 + arg2 * arg2);
    sp28 = 0;
    sp2A = -D_80370C70 * 16 + 0x7FF0;
    sp2A -= 0x2000;
    if (sp2C >= 16) {
        sp28 = 1;
        if (arg3 == 0 || arg3 == 2 || arg3 == 16) {
            sp28 |= 2;
        }
    }
    if (sp2C >= 46) {
        sp28 |= 2;
    }
    if (sp2C >= 76 && (D_80370C74 == 0 || arg3 != 3)) {
        sp28 |= 4;
    }
    if (sp28 == 0) {
        D_80370C74 = 0;
    }
    if (!(*arg0 & 0x300) && (sp28 & 1)) {
        *arg0 |= func_8028BA1C(sp2A, sp2E, &D_80370C74, arg3);
    }
    if (!(*arg0 & 0xC000) && (sp28 & 2)) {
        if (D_80370C74 != 0) {
            *arg0 |= 0x8000;
        } else {
            *arg0 |= 0x4000;
        }
    }
    if (*arg0 & 0x2000) {
        *arg0 |= 0x30;
    }
}
