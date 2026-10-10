#include "common.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/sched.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"
#include "game/frame.h"
#include "functions.h"

/* YoshiWindow entries, sorted with func_801F7FF4. */
u8 __osContDataCrc(u8 *);

extern OSThread D_80218D30;
extern u8 D_80218EF8[];
extern OSMesgQueue D_80219EF8;
extern OSMesg D_80219F10[];
extern OSMesgQueue D_80219F30;
extern OSMesg D_80219F48[];
extern OSMesgQueue D_80219F50;
extern OSMesg D_80219F68[];
extern u8 D_8020C000[];
extern u8 D_8020C014[];
extern u8 D_8020C01C[];
extern char D_8020F0B0[];
extern char D_8020F0D4[];
extern char D_8020F0FC[];
extern char D_8020F128[];
extern char D_8020F140[];
extern char D_8020F1E0[];
extern char D_8020F20C[];
extern char D_8020F214[];
extern char D_8020F224[];
extern char D_8020F250[];
extern char D_8020F258[];
extern OSMesgQueue D_80370BF8;
extern OSPfsState D_80218B20[];
extern s32 D_80218D28;



extern char D_8020F35C[];
extern char D_8020F374[];
extern u8 D_80365060[];
extern u8 D_8021A8F0;
extern u8 D_8039C4B8[];
extern u8 D_8020BEE0[];
extern char D_8020F268[];
extern char D_8020F270[];
extern char D_8020F2C8[];
extern s32 D_802FA264;
extern s32 D_80218EF0;
extern u8 D_802189C0[0x10][0x11];
extern u8 D_80218AD0[][5];
extern char D_80218740[][0x28];
extern char D_80219F90[];
extern char D_80219FB0[];
extern char D_8020F38C[];
extern char D_8020F39C[];
extern char D_8020F3A0[];
extern char D_8020F3A4[];
extern char D_8020F3A8[];
extern char D_8020F3AC[];
extern char D_8020F3B8[];
extern char D_8020F3C8[];
extern char D_8020F3E0[];
extern u8 D_80301080[];

extern char D_8020F2DC[];
extern char D_8020F308[];
extern char D_8020F320[];
extern char D_8020F330[];
extern char D_8020F348[];


/* .bss, 0x80219FD0-0x8021A840 (tools/bss_c.py) */
char D_80219FD0[0x40][0x20];
u8 D_8021A7D0[0x18];
u8 D_8021A7E8[0x40];
#ifndef VERSION_US_V10
u32 D_8021A828;
u8 D_8021A82C[4];
#endif
u8 D_8021A830[4];
u8 D_8021A834[4];
#ifndef VERSION_US_V10
u8 D_8021A838[8];
#endif

/* .data, 0x8020D800-0x8020D810 (tools/data_c.py) */
char D_8020D800[4][4] = { "1ST", "2ND", "3RD", "4TH" };

/* The players listed in the best-times window (D_8021A7E8): us.v10 counts
 * them with the window's own count, which may include a last entry that
 * isn't a player; us.v11 keeps the number of players in D_8021A828. */
#ifdef VERSION_US_V10
#define LIST_COUNT D_802F8BDC[22].count
typedef s32 ListIndex;
/* the asserts' line numbers: the us.v11 lines are 3 more */
#define BESTTIMES_LINE(n) ((n) - 3)
#else
#define BESTTIMES_LINE(n) (n)
#define LIST_COUNT D_8021A828
typedef u32 ListIndex;
#endif

void func_801F7850(void) {
    PlayerInfo *sp7C;
    YoshiWindow *sp78;
    YoshiEntry *sp74;
    s32 sp70;
    s32 sp6C;
    s32 sp68;
    char sp48[0x20];
    char sp28[0x20];

    sp7C = &D_80364AF0[D_80364AE8];
    sp78 = &D_802F8BDC[22];
    D_8036BB24 = (YoshiEntry *)D_80358070;
    D_80358070 += 0x41 * sizeof(YoshiEntry);
    for (sp6C = 0; sp6C < 4; sp6C++) {
        if (D_80365060[sp6C] == 1 && D_8039C53C[sp6C] == 0 &&
            LEVEL_DONE_IN(D_80364AF0[sp6C], D_802E8BDC)) {
            osSendMesg(&D_80219EF8, OS_MESG((u32)((D_802E8BDC << 8) | 8 | (sp6C << 16)) | 0x01000000),
                       OS_MESG_BLOCK);
            osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
        } else {
            func_801F8354(sp6C);
        }
    }
    for (sp70 = 0, sp68 = 0; sp70 < 0x10; sp70++) {
        if (func_801F7F74(sp70) != 0 && D_80364EF0[D_80364AEA][D_802E8C44[sp70]] > 0) {
            for (sp6C = 0; sp6C < 4; sp6C++) {
                sp74 = &D_8036BB24[sp68 * 4 + sp6C];
                sp7C = &D_80364AF0[sp6C];
                if (D_80365060[sp6C] == 1 && D_80364EF0[sp6C][D_802E8C44[sp70]] > 0 &&
                    (D_802E8F94[D_802E8BDC].unk0 != 0x80 || sp7C->gameState >= 0xB)) {
                    func_80264A34(sp48, D_80364EF0[sp6C][D_802E8C44[sp70]], 0);
                    sprintf(D_80219FD0[sp68 * 4 + sp6C], "%-7.7s %s", sp7C, sp48);
                    ENTRY_TEXT(sp74) = D_80219FD0[sp68 * 4 + sp6C];
                    sp74->unk10 = 0;
                    sp74->unk14 =
                        func_801EF2BC(D_80364EF0[sp6C][D_802E8C44[sp70]], D_802E8BDC, D_80364AF0[sp6C].gameState) % 5 +
                        0x12;
                    sp74->unk18 = sp6C;
                } else {
                    ENTRY_TEXT(sp74) = NULL;
                    sp74->unk10 = 0;
                    sp74->unk14 = 0;
                    sp74->unk18 = 4;
                }
                sp74->flags = 0x1400;
                sp74->x = 0x24;
                sp74->unk6 = 0x10;
                sp74->unk8 = 0x11;
                sp74->unk16 = D_80364EF0[sp6C][D_802E8C44[sp70]];
                sp74->unk1A = sp70;
            }
            func_802595E0(&D_8036BB24[sp68 * 4], 4, sizeof(YoshiEntry), func_801F7FF4);
            for (sp6C = 0; sp6C < 4; sp6C++) {
                sp74 = &D_8036BB24[sp68 * 4 + sp6C];
                sp74->y = sp6C * 0x11;
                if (sp6C == 2) {
                    sp74->flags |= 1;
                }
                if (ENTRY_TEXT(sp74) != NULL) {
                    bcopy(ENTRY_TEXT(sp74), sp28, func_8025B300((u8 *)ENTRY_TEXT(sp74)) + 1);
                    sprintf(ENTRY_TEXT(sp74), "%s %s", D_8020D800[sp6C], sp28);
                }
            }
            sp68++;
        }
    }
    func_802595E0(D_8036BB24, sp68, 4 * sizeof(YoshiEntry), func_801F7FF4);
    for (sp70 = 0; sp70 < sp68 * 4; sp70++) {
        sp74 = &D_8036BB24[sp70];
        D_8021A7E8[sp70] = sp74->unk18;
        if (!(sp70 & 3)) {
            D_8021A7D0[sp70 / 4] = sp74->unk1A;
        }
        sp74->unk16 = 0x1E;
        sp74->y += (sp70 / 4) * 0x64;
    }
    if (D_802E8F94[D_802E8BDC].unk0 == 0x80) {
        sp74 = &D_8036BB24[sp70];
        ENTRY_TEXT(sp74) = NULL;
        sp74->unk10 = 0;
        sp74->flags = 0x400;
        sp74->x = -0x20;
        sp74->y = 0x28;
        sp74->unk14 = 0xD;
        sp74->unk16 = sp74->unk1A = 0;
        sp78->count = sp68 * 4 + 1;
    } else {
        sp78->count = sp68 * 4;
    }
    sp78->unk18 = 2;
#ifndef VERSION_US_V10
    D_8021A828 = sp68 * 4;
#endif
    func_801F8228();
    func_801FDE50();
}

s32 func_801F7F74(u8 arg0) {
    if (D_802E8F94[D_802E8BDC].unk0 == 0x80) {
        return arg0 == 0;
    }
    return (D_80364AF0[D_80364AEA].unk10 & (1 << arg0)) ? 1 : 0;
}

s32 func_801F7FF4(YoshiEntry *arg0, YoshiEntry *arg1) {
    if (ENTRY_TEXT(arg0) != 0 && ENTRY_TEXT(arg1) != 0) {
        return arg0->unk16 - arg1->unk16;
    }
    if (ENTRY_TEXT(arg0) != 0) {
        return -1;
    }
    return 1;
}

void func_801F803C(void) {
    s32 sp1C;
    s32 sp18;

    if ((D_80370C28 & 0x2010) && !(D_80370C2A & 0x2010)) {
        sp1C = (D_80364AE8 + 1) % 4;
        for (sp18 = 0; sp18 < 4 && (D_80365060[sp1C % 4] != 1 || func_801F81B4(sp1C % 4) == 0);
             sp18++, sp1C = (sp1C + 1) % 4) {
        }
        if (D_80364AE8 != sp1C) {
            func_80260650(D_80367738, 0x1D, 0);
            func_801E8EB8(D_80364AE8 = sp1C, 1);
            func_801F8228();
        } else {
            func_80260650(D_80367738, 0xD0, 0);
        }
    }
}

s32 func_801F81B4(u8 arg0) {
    u32 sp4;
    u32 sp0;

#ifdef VERSION_US_V10
    sp4 = LIST_COUNT;
    sp0 = 0;
#else
    sp0 = 0;
    sp4 = LIST_COUNT;
#endif
    if (sp4 != 0 && D_8021A7E8[0] != arg0) {
        do {
        } while (++sp0 < sp4 && D_8021A7E8[sp0] != arg0);
    }
    return sp0 != sp4;
}

void func_801F8228(void) {
    YoshiEntry *sp4;
    ListIndex sp0;

    for (sp0 = 0; sp0 < LIST_COUNT; sp0++) {
        sp4 = &D_8036BB24[sp0];
        if (D_8021A7E8[sp0] == D_80364AE8) {
            sp4->flags |= 4;
        } else {
            sp4->flags &= ~4;
        }
        if (D_8021A7E8[sp0] == D_80364AEA) {
            sp4->unk18 = sp4->unk19 = 6;
        } else if (D_8021A7E8[sp0] == D_80364AE8) {
            sp4->unk18 = sp4->unk19 = 2;
        } else {
            sp4->unk18 = sp4->unk19 = 7;
        }
    }
}

void func_801F8354(u8 arg0) {
    s32 sp24;

    if (D_80370C50 == 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "frontEndPresent", "bestTimes.c", BESTTIMES_LINE(0x117));
    }
    if (LEVEL_DONE_IN(D_80364AF0[arg0], D_802E8BDC) == 0) {
        for (sp24 = 0; sp24 < 0x10; sp24++) {
            D_80364EF0[arg0][D_802E8C44[sp24]] = 0;
        }
    }
}

Gfx *func_801F8440(Frame *arg0, Gfx *arg1) {
    Gfx *sp2C;
    s16 sp2A;

    sp2A = 0;
    sp2C = arg1;
    if (D_802E8F94[D_802E8BDC].unk0 != 0x80) {
        sp2C = func_80274868(sp2C);
        sp2C = func_80272ED8(sp2C, D_8021A7D0[D_802F8BDC[22].unk18 / 4] + D_8021A8F0, 0x18 - sp2A, 0x64,
                             0xFF - sp2A * 2, 1, 1.0f);
        sp2C = func_80274AA4(sp2C);
    }
    return sp2C;
}
