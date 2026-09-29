#include "common.h"

extern u8 D_8039CAF0[][0x200];

void func_8029A7E4(char *, ...);
void func_802A1040(u16, u8 *, s32);
s32 func_8025B300(u8 *arg0);
s32 func_8025B370(u16 *arg0);

/* .bss, 0x80365360-0x80365580 (tools/bss_c.py) */
u8 D_80365360[0x50];
u16 D_803653B0[0x54];
char D_80365458[0x100];
u16 D_80365558[0x14];

/* .data, 0x802E8C80-0x802E8C90 (tools/data_c.py) */
s32 D_802E8C80 = 0x20200000;
f32 D_802E8C84[2] = { 0.6f, 0.95f };
u16 D_802E8C8C[2] = { 42, 0x1000 };

void func_8025B070(void) {
    s32 sp4;

    for (sp4 = 0; sp4 < 0x50; sp4++) {
        D_80365360[sp4] = 0;
        D_803653B0[sp4] = 0;
    }
}

u8 *func_8025B0B8(u16 arg0) {
    s32 sp1C;
    u8 sp1B;

    sp1B = 0;
    sp1C = 0;
    while (sp1C < 0x50 && sp1B == 0) {
        if (D_803653B0[sp1C] == arg0) {
            sp1B = 1;
        } else {
            sp1C++;
        }
    }
    if (!sp1B) {
        sp1C = 0;
        while (sp1C < 0x50 && sp1B == 0) {
            if (D_803653B0[sp1C] == 0) {
                func_802A1040(arg0, D_8039CAF0[sp1C], 0);
                D_803653B0[sp1C] = arg0;
                sp1B = 1;
            } else {
                sp1C++;
            }
        }
    }
    if (!sp1B) {
        sp1C = 0;
        while (sp1C < 0x50 && sp1B == 0) {
            if (D_80365360[sp1C] == 0) {
                func_802A1040(arg0, D_8039CAF0[sp1C], 0);
                D_803653B0[sp1C] = arg0;
                sp1B = 1;
            } else {
                sp1C++;
            }
        }
    }
    if (sp1B == 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "found", "font.c", 0x51);
    }
    if (sp1B != 0) {
        D_80365360[sp1C] = 3;
    }
    return D_8039CAF0[sp1C];
}

void func_8025B2B8(void) {
    s32 sp4;

    for (sp4 = 0; sp4 < 0x50; sp4++) {
        if (D_80365360[sp4] != 0) {
            D_80365360[sp4]--;
        }
    }
}

s32 func_8025B300(u8 *arg0) {
    s32 sp4;
    s32 sp0;

    sp4 = 0;
    sp0 = 0;
    if (arg0 != NULL && *arg0 != 0) {
        do {
            if (arg0[sp4] == '*') {
                sp0++;
            }
        } while (arg0[++sp4] != 0);
    }
    return sp4 - sp0;
}

s32 func_8025B370(u16 *arg0) {
    s32 sp4;
    s32 sp0;

    sp4 = 0;
    sp0 = 0;
    if (arg0 != NULL) {
        for (; arg0[sp4] != 0xFFE; sp4++) {
            if (arg0[sp4] == 0x1000) {
                sp0++;
            }
        }
    }
    return sp4 - sp0;
}

s32 func_8025B3F0(u8 *arg0, u8 *arg1) {
    s32 sp2C;
    s32 sp28;

    if ((sp28 = func_8025B300(arg0)) != func_8025B300(arg1)) {
        return 1;
    }
    for (sp2C = 0; sp2C < sp28; sp2C++) {
        if (arg0[sp2C] != arg1[sp2C]) {
            return 1;
        }
    }
    return 0;
}

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/168B0/func_8025B498.s")
#else
s32 func_8025B498(s16 arg0, u16 arg1, u8 *arg2, s32 arg3) {
    s32 sp2C;
    s16 sp2A;
    register s32 len;

    sp2C = 0;
    len = func_8025B300(arg2);
    sp2A = arg0 - ((*D_802E8C84 * (len - 1) + 1.0f) * arg1) / 2.0;
    if (sp2C) {
    }
    return sp2A;
}
#endif

char *func_8025B558(u16 *arg0) {
    s32 sp4;

    sp4 = 0;
    while (arg0[sp4] != 0 && sp4 < 0xFF) {
#ifdef TARGET_PC
        /* unsequenced: IDO stores to the old index */
        D_80365458[sp4] = arg0[sp4];
        sp4++;
#else
        D_80365458[sp4] = arg0[sp4++];
#endif
    }
    D_80365458[sp4] = 0;
    return D_80365458;
}

u16 *func_8025B5D4(u16 *arg0, u16 *arg1, u16 *arg2, s32 arg3) {
    u8 sp34[12];
    s32 sp30;
    s32 sp2C;
    s32 sp28;

    sp30 = 0;
    sp2C = 0;
    do {
        switch (arg1[sp30]) {
            case 0x1003:
                for (sp28 = 0; arg2[sp28] != 0xFFE; sp28++, sp2C++) {
                    arg0[sp2C] = arg2[sp28];
                }
                break;
            case 0x1004:
                sprintf((char *)sp34, "%d", arg3);
                for (sp28 = 0; sp34[sp28] != 0; sp28++, sp2C++) {
                    arg0[sp2C] = sp34[sp28] - 0x20;
                }
                break;
            default:
                arg0[sp2C] = arg1[sp30];
                sp2C++;
                break;
        }
        sp30++;
    } while (arg1[sp30] != 0xFFE);
    arg0[sp2C] = 0xFFE;
    return arg0;
}

extern u16 D_80365558[];

u16 *func_8025B7AC(u8 *arg0) {
    s32 sp1C;

    for (sp1C = 0; sp1C < func_8025B300(arg0); sp1C++) {
        switch (arg0[sp1C]) {
            case ' ':
                D_80365558[sp1C] = 0x1002;
                break;
            case '.':
                D_80365558[sp1C] = 0x3C;
                break;
            case '1':
            case '2':
            case '3':
            case '4':
                D_80365558[sp1C] = arg0[sp1C] - 0x20;
                break;
            default:
                D_80365558[sp1C] = arg0[sp1C] - 0x27;
                break;
            case '/':
                D_80365558[sp1C] = 0x3D;
                break;
            case ':':
                D_80365558[sp1C] = 0x3E;
                break;
        }
    }
    D_80365558[sp1C] = 0xFFE;
    return D_80365558;
}

void func_8025B918(u16 *arg0, u16 *arg1) {
    s32 sp1C;
    s32 sp18;

    for (sp1C = 0, sp18 = func_8025B370(arg0); sp1C < func_8025B370(arg1); sp1C++, sp18++) {
        arg0[sp18] = arg1[sp1C];
    }
    arg0[sp18] = 0xFFE;
}
