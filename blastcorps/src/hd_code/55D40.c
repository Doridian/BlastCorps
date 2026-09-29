#include "common.h"
#include "game/game.h"
#include "game/yoshi.h"

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 unk5;
    /* 0x06 */ char unk6[0x2A];
} UnkStruct_80304A90; /* size = 0x30 */

extern u8 D_803643D6;

/* The credits. */
UnkStruct_80304A90 D_80304A90[] = {
    { 0x11, 0x04, 0x14, "EXECUTIVE PRODUCER" },
    { 0x61, 0x07, 0x10, "HIROSHI YAMAUCHI" },
    { 0x11, 0x04, 0x14, "NINTENDO PRODUCERS" },
    { 0x41, 0x07, 0x10, "MIKE FUKUDA" },
    { 0x61, 0x07, 0x10, "KENJI MIKI" },
    { 0x11, 0x04, 0x14, "GAME DESIGN" },
    { 0x61, 0x07, 0x10, "MARTIN WAKELEY" },
    { 0x11, 0x04, 0x14, "LEAD PROGRAMMER" },
    { 0x61, 0x07, 0x10, "RICHARD WILSON" },
    { 0x11, 0x04, 0x12, "ADDITIONAL PROGRAMMING" },
    { 0x61, 0x07, 0x10, "GRAHAM SMITH" },
    { 0x11, 0x04, 0x14, "LEAD ARTIST" },
    { 0x61, 0x07, 0x10, "RICHARD BERWICK" },
    { 0x11, 0x04, 0x14, "ADDITIONAL ARTISTS" },
    { 0x41, 0x07, 0x10, "PAUL CUNNINGHAM" },
    { 0x61, 0x07, 0x10, "KEITH RABBETTE" },
    { 0x11, 0x04, 0x14, "MUSIC AND SFX" },
    { 0x61, 0x07, 0x10, "GRAEME NORGATE" },
    { 0x11, 0x04, 0x14, "STORY AND TEXT" },
    { 0x61, 0x07, 0x10, "LEIGH LOVEDAY" },
    { 0x11, 0x04, 0x14, "PRODUCT MANAGER" },
    { 0x61, 0x07, 0x10, "SIMON FARMER" },
    { 0x11, 0x04, 0x12, "PRODUCT TEST" },
    { 0x11, 0x04, 0x12, "AND ADDITIONAL DESIGN" },
    { 0x41, 0x07, 0x10, "HUW WARD" },
    { 0x41, 0x07, 0x10, "GARY RICHARDS" },
    { 0x41, 0x07, 0x10, "GEORGE ANDREAS" },
    { 0x41, 0x07, 0x10, "GAVIN HOOD" },
    { 0x41, 0x07, 0x10, "GARETH JONES" },
    { 0x41, 0x07, 0x10, "MARTIN PENNY" },
    { 0x41, 0x07, 0x10, "DAVID WONG" },
    { 0x61, 0x07, 0x10, "JAMIE WILLIAMS" },
    { 0x11, 0x04, 0x14, "ORIGINAL CONCEPT" },
    { 0x61, 0x07, 0x10, "CHRIS STAMPER" },
    { 0x11, 0x04, 0x12, "EMULATOR PROGRAMMING" },
    { 0x61, 0x07, 0x10, "ROBERT HARRISON" },
    { 0x11, 0x04, 0x12, "ADDITIONAL COMPRESSION" },
    { 0x61, 0x07, 0x10, "MIKE CURRINGTON" },
    { 0x11, 0x04, 0x12, "ADDITIONAL ARTWORK" },
    { 0x41, 0x07, 0x10, "KEVIN BAYLISS" },
    { 0x61, 0x07, 0x10, "DON MURPHY" },
    { 0x11, 0x04, 0x14, "NOA STAFF" },
    { 0x41, 0x07, 0x10, "KEN LOBB" },
    { 0x41, 0x07, 0x10, "ERICH WAAS" },
    { 0x41, 0x07, 0x10, "ARMOND WILLIAMS" },
    { 0x41, 0x07, 0x10, "ISAAC MARSHALL" },
    { 0x41, 0x07, 0x10, "HENRY STERCHI" },
    { 0x61, 0x07, 0x10, "RICH RICHARDSON" },
    { 0x11, 0x04, 0x14, "VOICES" },
    { 0x41, 0x07, 0x10, "ROBIN KROUSE" },
    { 0x41, 0x07, 0x10, "ERICH WAAS" },
    { 0x41, 0x07, 0x10, "ISAAC MARSHALL" },
    { 0x41, 0x07, 0x10, "MICHAEL KELBAUGH" },
    { 0x41, 0x07, 0x10, "LEE RAY" },
    { 0x61, 0x07, 0x10, "HELEN COOMBS" },
    { 0x11, 0x04, 0x14, "LIVE GUITAR" },
    { 0x61, 0x07, 0x10, "GRANT KIRKHOPE" },
    { 0x11, 0x04, 0x12, "NOA PRODUCT TESTING" },
    { 0x41, 0x07, 0x10, "MICHAEL KELBAUGH" },
    { 0x41, 0x07, 0x10, "TIM BECHTEL" },
    { 0x41, 0x07, 0x10, "CHRIS NEEDHAM" },
    { 0x41, 0x07, 0x10, "BEN SMITH" },
    { 0x41, 0x07, 0x10, "ROBERT JOHNSON" },
    { 0x41, 0x07, 0x10, "THOMAS HERTZOG" },
    { 0x41, 0x07, 0x10, "DAVID BRIDGHAM" },
    { 0x61, 0x07, 0x10, "NOA TESTING TEAM" },
    { 0x11, 0x04, 0x14, "RARE US STAFF" },
    { 0x41, 0x07, 0x10, "EILEEN HOCHBERG" },
    { 0x41, 0x07, 0x10, "SCOTT HOCHBERG" },
    { 0x41, 0x07, 0x10, "JERRY ROGOWSKI" },
    { 0x61, 0x07, 0x10, "MATTHEW BERGER" },
    { 0x11, 0x04, 0x14, "NCL STAFF" },
    { 0x41, 0x07, 0x10, "KEISUKE TERASAKI" },
    { 0x41, 0x07, 0x10, "EIJI ONOZUKA" },
    { 0x41, 0x07, 0x10, "KIMIKO NAKAMICHI" },
    { 0x61, 0x07, 0x10, "MASASHI GOTO" },
#ifdef VERSION_EU
    { 0x11, 0x04, 0x14, "GERMAN TEXT" },
    { 0x41, 0x07, 0x10, "JOHN DAVID KRAFT" },
    { 0x61, 0x07, 0x10, "PUBLICATION TEAM" },
#endif
    { 0x11, 0x04, 0x14, "SPECIAL THANKS TO" },
    { 0x41, 0x07, 0x10, "NCL, EAD. ARTWORK TEAM" },
    { 0x41, 0x07, 0x10, "NCL MARIO CLUB" },
    { 0x41, 0x07, 0x10, "JOEL HOCHBERG" },
    { 0x41, 0x07, 0x10, "MR. ARAKAWA" },
    { 0x61, 0x07, 0x10, "HOWARD LINCOLN" },
};

void func_80259DC8(s32, char *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
void func_80260EE0(s32);
void func_8026AF6C(s32);
void func_80275270(u64, f32);
s32 func_802753C0(void);

/* .bss, 0x803A6B20-0x803A6B30 (tools/bss_c.py) */
s32 D_803A6B20;
s32 D_803A6B24;

void func_8029A500(void) {
    D_803A6B20 = -0xEF;
    D_803A6B24 = 0;
}

/* eu's credits have three more lines, and less room after a line with
 * flag 0x20. */
#ifdef VERSION_EU
#define CREDITS_LINES 0x55
#define HEADING_GAP 0x20
#else
#define CREDITS_LINES 0x52
#define HEADING_GAP 0x26
#endif

s32 func_8029A518(s32 arg0, s32 arg1) {
    s32 sp64;
    UnkStruct_80304A90 *sp60;
    s32 sp5C;
    s32 sp58;
    s32 sp54;
    s32 sp50;

    sp54 = 0;
    sp5C = 0;
    sp64 = arg1;
    do {
        sp60 = &D_80304A90[sp5C];
        if (sp60->unk0 & 1) {
            sp50 = 0xA0;
        } else if (sp60->unk0 & 4) {
            sp50 = 0x6A;
        } else if (sp60->unk0 & 8) {
            sp50 = 0xD5;
        } else {
            sp50 = 0;
        }

        if (sp54 - D_803A6B20 >= -0x31 && sp54 - D_803A6B20 < 0xF0) {
            func_80259DC8(arg0, sp60->unk6, 0, 0, sp50, sp58, sp54 - D_803A6B20, sp60->unk5, sp60->unk5, 1,
                          D_802F47B0[sp60->unk4].r0, D_802F47B0[sp60->unk4].g0,
                          D_802F47B0[sp60->unk4].b0, D_802F47B0[sp60->unk4].a0,
                          D_802F47B0[sp60->unk4].r1, D_802F47B0[sp60->unk4].g1,
                          D_802F47B0[sp60->unk4].b1, D_802F47B0[sp60->unk4].a1);
        }
        if (sp60->unk0 & 0x20) {
            sp54 += HEADING_GAP;
        }
        if (sp60->unk0 & 0x10) {
            sp54 += 0x16;
        }
        if (sp60->unk0 & 0x40) {
            sp54 += 0x11;
        }
    } while (++sp5C < CREDITS_LINES);
    if (D_80358060 == 0x64) {
        func_8026AF6C(0x8036);
    }
    if (D_80358060 >= 0x18C) {
        D_803A6B20++;
    }
    if (D_803643D6 != 0) {
        D_803A6B24++;
    }
    if (D_803A6B24 == 0x7D) {
        func_8026AF6C(0x8037);
        func_80260EE0(0x25);
    }
    if (D_803A6B24 >= 0x7E && D_8036BB1C == 1 && func_802753C0() == 0) {
        func_80275270(0x200000000000, 0.75f);
    }
    return sp64;
}
