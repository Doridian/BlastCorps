#include "common.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

extern u8 D_802E8BF0;
/*
 * This file's .bss.  Defined here, a u64's two halves share one lui.
 */

/* .bss, 0x803A6AF0-0x803A6B10 (tools/bss_c.py) */
u64 D_803A6AF0;
u64 D_803A6AF8;
u8 D_803A6B00;
u8 D_803A6B01;
u8 D_803A6B02;
u8 D_803A6B03;
u8 D_803A6B04;


u64 func_801ECA50(u8);
void func_8025B9D0(s32, s32 *);
void func_8025BB50(void);
void func_80260650(SndBank *, s32, s32);
void func_802609D0(void);
void func_8026AF6C(s32);
s32 func_8026F92C(u64);
void func_80295E50(void);
void func_8029A500(void);
void func_8029A7E4(char *, ...);

#define LEVEL_DONE(l) LEVEL_DONE_IN(D_80364AF0[D_80364AE8], l)

void func_802995F0(s32 arg0) {
    D_803A6B03 = arg0;
    func_8029A7E4("prepare sequence %d\n", arg0);
    switch (D_803A6B03) {
        case 1:
            D_803A6B02 = 0x26;
            D_803A6B01 = 0x28;
            D_803A6B00 = 0x28;
            D_803A6AF0 = 0x4000;
            D_803A6AF8 = 0x4000;
            D_803A6B04 = 1;
            break;
        case 3:
            D_803A6B02 = 0x26;
            D_803A6B01 = 0x2B;
            D_803A6B00 = 0x2B;
            D_803A6AF0 = 0x4000;
            D_803A6AF8 = 0x4000;
            D_803A6B04 = 1;
            break;
        case 0:
            D_803A6B02 = 0x31;
            D_803A6B01 = 0x32;
            D_803A6B00 = 0x32;
            D_803A6AF0 = 0x4000;
            D_803A6AF8 = 0x4000;
            D_803A6B04 = 0;
            break;
        case 4:
            D_803A6B02 = 0x2F;
            D_803A6B01 = 0;
            D_803A6B00 = 0;
            D_803A6AF0 = 0x4000;
            D_803A6AF8 = 0x4000;
            D_803A6B04 = 1;
            break;
        case 5:
            D_803A6B02 = 0x37;
            D_803A6B01 = 0x37;
            D_803A6B00 = 0x37;
            if (LEVEL_DONE(D_803A6B00)) {
                D_803A6AF0 = 0x80;
                D_803A6AF8 = 0x80;
            } else {
                D_803A6AF0 = 0x2000;
                D_803A6AF8 = 0x2000;
            }
            D_803A6B04 = 1;
            break;
        case 6:
            D_803A6B02 = 0x1C;
            D_803A6B01 = 0x1C;
            D_803A6B00 = 0x1C;
            if (LEVEL_DONE(D_803A6B00)) {
                D_803A6AF0 = 0x80;
                D_803A6AF8 = 0x80;
            } else {
                D_803A6AF0 = 0x2000;
                D_803A6AF8 = 0x2000;
            }
            D_803A6B04 = 1;
            break;
        case 7:
            D_803A6B02 = 0x35;
            D_803A6B01 = 0x35;
            D_803A6B00 = 0x35;
            if (LEVEL_DONE(D_803A6B00)) {
                D_803A6AF0 = 0x80;
                D_803A6AF8 = 0x80;
            } else {
                D_803A6AF0 = 0x2000;
                D_803A6AF8 = 0x2000;
            }
            D_803A6B04 = 1;
            break;
        case 8:
            D_803A6B02 = 7;
            D_803A6B01 = 7;
            D_803A6B00 = 7;
            if (LEVEL_DONE(D_803A6B00)) {
                D_803A6AF0 = 0x80;
                D_803A6AF8 = 0x80;
            } else {
                D_803A6AF0 = 0x2000;
                D_803A6AF8 = 0x2000;
            }
            D_803A6B04 = 1;
            break;
        case 9:
            D_803A6B02 = 0x13;
            D_803A6B01 = 0x13;
            D_803A6B00 = 0x13;
            if (LEVEL_DONE(D_803A6B00)) {
                D_803A6AF0 = 0x80;
                D_803A6AF8 = 0x80;
            } else {
                D_803A6AF0 = 0x2000;
                D_803A6AF8 = 0x2000;
            }
            D_803A6B04 = 1;
            break;
        default:
            func_8029A7E4("unknown sequence number %d\n", D_803A6B03);
            break;
    }
}

void func_80299C0C(void) {
    D_802E8BDC = D_803A6B02;
}

void func_80299C20(void) {
    switch (D_802E8BDC) {
        case 0x2F:
            D_802E8BEC = 0;
            D_802E8BF0 = 0;
            func_8025B9D0(0, &D_802E8BDC);
            break;
        case 0x37:
            D_802E8BEC = 9;
            D_802E8BF0 = 0;
            func_8025B9D0(9, &D_802E8BDC);
            D_803643D4 = 5;
            func_8026AF6C(0x8041);
            func_80295E50();
            break;
        case 0x1C:
            D_802E8BEC = 10;
            D_802E8BF0 = 0;
            func_8025B9D0(10, &D_802E8BDC);
            D_803643D4 = 1;
            func_8026AF6C(0x8042);
            func_80295E50();
            break;
        case 0x35:
            D_802E8BEC = 11;
            D_802E8BF0 = 0;
            func_8025B9D0(11, &D_802E8BDC);
            D_803643D4 = 2;
            func_8026AF6C(0x8043);
            func_80295E50();
            break;
        case 0x7:
            D_802E8BEC = 12;
            D_802E8BF0 = 0;
            func_8025B9D0(12, &D_802E8BDC);
            D_803643D4 = 3;
            func_8026AF6C(0x8044);
            func_80295E50();
            break;
        case 0x13:
            D_802E8BEC = 13;
            D_802E8BF0 = 0;
            func_8025B9D0(13, &D_802E8BDC);
            D_803643D4 = 9;
            func_8026AF6C(0x8045);
            func_80295E50();
            break;
        case 0x31:
            func_8029A500();
            break;
        case 0x26:
            func_8025BB50();
            break;
    }
}

void func_80299E10(s32 arg0) {
    func_802609D0();
    switch (D_802E8BDC) {
        case 0x26:
        case 0x2F:
        case 0x31:
            D_80364AF0[D_80364AE8].medal[D_802E8BDC] = 5;
            break;
        case 0x37:
            D_80364AF0[D_80364AE8].unkEE |= 1;
            break;
        case 0x1C:
            D_80364AF0[D_80364AE8].unkEE |= 2;
            break;
        case 0x35:
            D_80364AF0[D_80364AE8].unkEE |= 4;
            break;
        case 0x7:
            D_80364AF0[D_80364AE8].unkEE |= 8;
            break;
        case 0x13:
            D_80364AF0[D_80364AE8].unkEE |= 0x10;
            break;
    }
    if (arg0 != 0) {
        func_80260650(D_80367738, 0x1E, 0);
        D_80364A98 = D_803A6AF0;
        D_802E8BDC = D_803A6B00;
    } else {
        D_80364A98 = D_803A6AF8;
        D_802E8BDC = D_803A6B01;
    }
    func_8029A7E4("finishing sequence and going to level %d\n", D_802E8BDC);
}

u64 func_80299FE8(u8 arg0) {
    u64 sp28;
    register s32 world;

    switch (arg0) {
        case 38:
            sp28 = 0x100000000000;
            func_802995F0(1);
            break;
        case 55:
            sp28 = 0x100000000000;
            func_802995F0(5);
            break;
        case 28:
            sp28 = 0x100000000000;
            func_802995F0(6);
            break;
        case 53:
            sp28 = 0x100000000000;
            func_802995F0(7);
            break;
        case 7:
            sp28 = 0x100000000000;
            func_802995F0(8);
            break;
        case 19:
            sp28 = 0x100000000000;
            func_802995F0(9);
            break;
        default:
            sp28 = func_801ECA50(arg0);
            break;
    }
    world = func_8026F92C(sp28);
    func_8029A7E4("get loop done for world %d\n", world);
    return sp28;
}
