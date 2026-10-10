#include "common.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/sched.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

/* YoshiWindow entries, sorted with func_801F7FF4. */
u8 __osContDataCrc(u8 *);
s32 func_801F5FE4(void);
s32 func_801F60C8(void);
s32 func_801F6160(u8);
s32 func_801F61C8(s32);
s32 func_801F6210(u8);
s32 func_801F6264(u8, u8);
s32 func_801F65C4(u8, u8, u8);
s32 func_801F67E4(u8, u8, u8);
s32 func_801F6AF4(u8, u64);
s32 func_801F6CA4(u8, u8, u8);
s32 func_801F6ED4(u8);
extern s32 D_8036BF10;
extern OSThread D_80310BD0;

extern OSMesgQueue D_80370BF8;
extern u32 D_8021A828;
extern u8 D_8021A7E8[];



extern char D_8020FF34[];
extern char D_8020FF60[];
extern char D_8020FF70[];
extern u8 D_80365060[];
extern u8 D_8021A7D0[];
extern u8 D_8021A8F0;
extern u8 D_8039C4B8[0x40];
extern s32 D_802FA264;
#ifdef VERSION_EU
extern u8 D_80366F70_eu; /* the language: which of text, text2, text3 */
s32 func_801F7120_eu(u8);
#else
extern u16 D_80301080[];
#endif

extern char D_80219FD0[][0x20];
extern char D_8020D800[][4];
extern char D_8020FF20[];
extern char D_8020FF2C[];


/* .bss, 0x80218740-0x80219FD0 (tools/bss_c.py) */
#ifdef VERSION_JP
char D_80218740[0x10][0x50];
#else
char D_80218740[0x10][0x28];
#endif
u8 D_802189C0[0x10][0x11];
u8 D_80218AD0[0x10][5];
OSPfsState D_80218B20[0x10];
u8 D_80218D20[4];
s32 D_80218D24;
s32 D_80218D28;
OSThread D_80218D30;
SchedClient D_80218EE0;
s32 D_80218EF0;
u8 D_80218EF8[0x1000];
OSMesgQueue D_80219EF8;
OSMesg D_80219F10[8];
OSMesgQueue D_80219F30;
OSMesg D_80219F48[2];
OSMesgQueue D_80219F50;
OSMesg D_80219F68[8];
s32 D_80219F88;
#ifdef VERSION_JP
char D_80219F90[0x40];
char D_80219FB0[0x40];
u8 D_80219F00_jp[0x40];
#else
char D_80219F90[0x20];
char D_80219FB0[0x20];
#endif

/* .data, 0x8020BEE0-0x8020C070 (tools/data_c.py) */
u8 D_8020BEE0[0x120] = {
    80, 76, 65, 89, 69, 82, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 6, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 34, 17, 68,
    51, 102, 85, 136, 119,
};
#ifdef VERSION_JP
u8 D_8020C000[0x14] = { 141, 118, 92, 99, 138, 59, 129, 59 };
#else
u8 D_8020C000[0x14] = { 66, 76, 65, 83, 84, 67, 79, 82, 80, 83, 32, 71, 65, 77, 69 };
#endif
u8 D_8020C014[8] = { 0 };
#ifdef VERSION_EU
/* What func_801F7120_eu saves for each language. */
typedef struct UnkStruct_8020CAB0_eu {
    u64 lang[2];
} UnkStruct_8020CAB0_eu;
UnkStruct_8020CAB0_eu D_8020CAB0_eu = { { 0x1982198219821982, 0x1945194519451945 } };
#endif
#ifdef VERSION_JP
/* 0x0FFE-terminated u16 text */
u16 D_8020BFEC_jp[4] = { 0x3C, 0x1003, 0xFFE };
u16 D_8020BFF4_jp[4] = { 3, 0x1004, 4, 0xFFE };
u16 D_8020BFFC_jp[2] = { 0x1002, 0xFFE };
#else
#ifdef VERSION_EU
u8 D_8020C01C[0x50] = {
#else
u8 D_8020C01C[0x54] = {
#endif
    0, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 32, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 33, 97, 98, 39, 100, 101, 44, 45, 46, 47, 58, 107, 63, 109, 45, 45, 45,
};
#endif

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/E7B0/func_801F57B0.s")
#else
void func_801F57B0(void) {
    s32 sp24;

    sp24 = 0xDE0;
    osCreateThread(&D_80218D30, 2, func_801F58E8, NULL, D_80218EF8 + 0x1000, 0xB);
    osCreateMesgQueue(&D_80219EF8, D_80219F10, 8);
    osCreateMesgQueue(&D_80219F30, D_80219F48, 1);
    osCreateMesgQueue(&D_80219F50, D_80219F68, 8);
    func_801F74B0(D_8020C000);
    func_801F74B0(D_8020C014);
    func_8029A7E4("current pak file size is %d bytes\n", sp24);
    func_8029A7E4("current playerInfo size is %d bytes\n", 0x100);
    osScAddClient(&D_80315440, &D_80218EE0, &D_80219F30, 1, 3);
    if (sp24 >= 0xE00) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "filesize<PFS_FILE_SIZE", "pfsHandler.c", LINE_EU(0x68, 0x6A));
    }
    osStartThread(&D_80218D30);
}
#endif

/* The Controller Pak thread (commands from D_80219EF8, replies on D_80219F50). */
void func_801F58E8(void) {
    OSMesg sp3C;
    s32 sp38;
    s32 sp34;
    u8 sp33; /* unused */
    u8 sp32;
    u8 sp31;
    u8 sp30;
    u8 sp2F;
    u8 sp2E;
    u8 sp2D;
    s32 sp28;
    u32 sp24;

    for (;;) {
        D_8039C4B0 = 0;
        sp24 = 0;
        osSetEventMesg(OS_EVENT_SI, &D_80370BF8, NULL);
        osRecvMesg(&D_80219EF8, &sp3C, OS_MESG_BLOCK);
        osSetEventMesg(OS_EVENT_SI, &D_80370BF8, NULL);
        while (D_8036BF10 != 0) {
#ifdef TARGET_PC
            port_spin_wait();
#endif
        }
        sp2F = OS_MESG_INT(sp3C) & 0xFF;
        sp31 = (OS_MESG_INT(sp3C) >> 8) & 0xFF;
        sp32 = (OS_MESG_INT(sp3C) >> 16) & 0xFF;
        sp30 = (OS_MESG_INT(sp3C) >> 24) & 0xFF;
        D_8020C014[0] = sp32 + 0x11;
        D_8039C4B0 = 1;
#ifdef TARGET_PC
        port_replay_save_started();     /* --replay: when the movie's thread got to it */
#endif
        func_8028A42C();
        func_801EE390();
        D_80218D24 = 0;
        do {
            sp2D = 0;
            sp2E = 0;
            sp28 = 0x5B;
            sp34 = 0;
            switch (sp2F) {
                case 1:
                case 2:
                    sp38 = func_801F60C8();
                    break;
                case 3:
                    sp38 = func_801F6160(sp32);
                    break;
                case 4:
                    sp38 = func_801F61C8(sp32);
                    break;
                case 5:
                    sp38 = func_801F6210(sp32);
                    break;
                case 6:
                    sp38 = func_801F6264(sp32, 0);
                    break;
                case 7:
                    sp38 = func_801F6264(sp32, 1);
                    break;
                case 8:
                    sp38 = func_801F65C4(sp32, sp31, 0);
                    break;
                case 9:
                    sp38 = func_801F65C4(sp32, sp31, 1);
                    break;
                case 10:
                    sp38 = func_801F67E4(sp32, sp31, 0);
                    break;
                case 11:
                    sp38 = func_801F67E4(sp32, sp31, 1);
                    break;
                case 12:
                    sp38 = func_801F6CA4(sp32, sp31, 0);
                    break;
                case 13:
                    sp38 = func_801F6CA4(sp32, sp31, 1);
                    break;
                case 14:
                    sp38 = osPfsFreeBlocks(&D_8039B630, &D_80218EF0);
                    break;
                case 15:
                    sp38 = func_801F5FE4();
                    break;
                case 16:
                    sp2D = 1;
                    sp38 = osEepromProbe(&D_80370BF8);
                    break;
                case 17:
                    sp38 = func_801F6ED4(sp32);
                    break;
                case 18:
                    sp38 = osPfsChecker(&D_8039B630);
                    break;
                case 19:
                    sp38 = 10;
                    break;
                case 20:
                    sp38 = func_801F6AF4(sp32, 0x2704197125121981);
                    break;
                case 21:
                    sp38 = func_801F6AF4(sp32, 0x87569AB6CD076AEC);
                    break;
#ifdef VERSION_EU
                case 23:
                    sp38 = func_801F7120_eu(1);
                    break;
                case 22:
                    sp38 = func_801F7120_eu(0);
                    break;
                case 24:
#else
                case 22:
#endif
                    sp38 = 0;
                    break;
                default:
                    func_8029A7E4("Nonsense pak message\n");
                    break;
            }
            func_8029A7E4("pak command %d returned %d\n", sp2F, sp38);
            switch (sp38) {
                case 0x6E382:
                    if ((D_80364A90 & 0x10E18000) || (D_80364A98 & 0x20000000000000)) {
                        sp2D = 1;
                        break;
                    }
                    /* fallthrough */
                case 6:
                case 10:
                case 11:
                    if (sp24 >= 4) {
                        if (sp2F != 0x13) {
                            if (sp38 == 0x6E382) {
                                D_80219F88 = 0x5D;
                            } else {
                                D_80219F88 = 0x5C;
                            }
                            func_801F6AF4(sp32, 0x2704197125121981);
                            sp2E = 0x13;
                        }
                        sp28 = D_80219F88;
                    } else {
                        sp28 = 0;
                        sp24++;
                    }
                    break;
                case 0:
                case 5:
                case 9:
                    sp2D = 1;
                    break;
                case 8:
                    if (!(D_80364A90 & 0x10E18000) || func_801F5FE4() != 0) {
                        break;
                    }
                    /* fallthrough */
                case 7:
                    D_8039C538 = (sp32 < D_8039C538) ? sp32 : D_8039C538;
                    sp2D = 1;
                    break;
                case 3:
                    if (sp24 >= 4) {
                        if (sp2F != 0x13) {
                            func_801F6AF4(sp32, 0x2704197125121981);
                            sp2E = 0x13;
                        }
                        sp28 = 0x5C;
                    } else {
                        func_8029A7E4("trying to fix pak ...\n");
                        if (sp2F != 0x12) {
                            osSendMesg(&D_80219EF8, OS_MESG(sp2F | (sp31 << 8) | (sp32 << 16) | (sp30 << 24)),
                                       OS_MESG_NOBLOCK);
                        }
                        sp2E = 0x12;
                        sp30 = 0;
                        sp24++;
                    }
                    break;
                case 2:
                    if (sp34 == 8 && !(D_80364A90 & 0x10E18000)) {
                        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "pfsHandler.c", LINE_EU(342, 352));
                        sp2D = 1;
                    }
                    break;
                case 1:
                    if (D_80364A98 & 0x20000000000000) {
                        sp2D = 1;
                    }
                    break;
            }
            /*
             * Dead code IDO still branches over: without it case 1's last
             * `b` to the next instruction is dropped.  Probably a compiled-out
             * debug macro in the original.
             */
            while (0) {
                sp2D = 1;
            }
            sp34 = sp38;
            if (D_8036BF10 == 0 && sp28 != 0 && sp2D == 0 && sp2E == 0 && &D_80310BD0 == D_80219F50.mtqueue) {
                func_801EE398(sp28);
                D_80218D24 = 1;
            }
            if (sp2E != 0) {
                sp2F = sp2E;
                sp2E = 0;
            }
#ifdef TARGET_PC
            /* A command that is done goes on at once: the wait for the
               scheduler's next message (one a retrace) paces the N64's slow
               SI, and the port's is instant.  A message already there is
               taken, so that a retry still waits for a new one.  Only
               --load-waits n64 keeps the wait (port_game.h). */
            if (sp2D != 0 && !port_load_waits()) {
                osRecvMesg(&D_80219F30, NULL, OS_MESG_NOBLOCK);
            } else
#endif
            osRecvMesg(&D_80219F30, NULL, OS_MESG_BLOCK);
        } while (sp2D == 0);
        if (D_80218D24 != 0) {
            D_8036BB1C = 1;
            D_8036BB18 = -1;
        }
        if (sp30 != 0) {
            osSendMesg(&D_80219F50, OS_MESG(sp38), OS_MESG_BLOCK);
        }
    }
}

s32 func_801F5FE4(void) {
    s32 sp24;
    u8 sp23;
    OSMesg sp1C;

    sp1C = NULL;
    if (D_80370BF8.validCount != 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "pfsHandler.c", LINE_EU(0x190, 0x19A));
        osRecvMesg(&D_80370BF8, &sp1C, OS_MESG_NOBLOCK);
    }
    if (func_8028FCD4(&D_80370BF8, &sp23) != 0) {
        sp24 = 1;
    } else if (!(sp23 & 1)) {
        sp24 = 1;
    } else {
        sp24 = 0;
    }
    if (sp1C != NULL) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "pfsHandler.c", LINE_EU(0x19E, 0x1A8));
        osSendMesg(&D_80370BF8, sp1C, OS_MESG_NOBLOCK);
    }
    return sp24;
}

s32 func_801F60C8(void) {
    u8 sp1F;
    s32 sp18;

    sp18 = 0;
    D_8039C538 = 4;
    if (func_8028FCD4(&D_80370BF8, &sp1F) != 0) {
        sp18 = 1;
    } else if (!(sp1F & 1)) {
        sp18 = 1;
    }
    if (sp18 == 0) {
        sp18 = osPfsInit(&D_80370BF8, &D_8039B630, 0);
    }
    func_8028A370();
    return sp18;
}

/*
 * func_801F6160, func_801F61C8 and func_801F6ED4: the pak thread stores
 * their result, the libultra call's.
 */
s32 func_801F6160(u8 arg0) {
    return osPfsAllocateFile(&D_8039B630, PAK_COMPANY_CODE, PAK_GAME_CODE, D_8020C000, D_8020C014, 0xE00, &D_8039B698[arg0]);
}

s32 func_801F61C8(s32 arg0) {
    return osPfsDeleteFile(&D_8039B630, PAK_COMPANY_CODE, PAK_GAME_CODE, D_8020C000, D_8020C014);
}

s32 func_801F6210(u8 arg0) {
    s32 sp24;

    sp24 = osPfsDeleteFile(&D_8039B630, D_80218B20[arg0 - 37].company_code, D_80218B20[arg0 - 37].game_code,
                           (u8 *)D_80218B20[arg0 - 37].game_name, (u8 *)D_80218B20[arg0 - 37].ext_name);
    return sp24;
}

s32 func_801F6264(u8 arg0, u8 arg1) {
    s32 sp3C;
    s32 sp38;
    u32 sp34;
    s32 sp30;
    s32 sp2C;
    u8 *sp28;
    u64 sp20;

    sp3C = 0;
    sp28 = (u8 *)&D_80364AF0[arg0];
    for (sp34 = 0; sp34 < 0x100 && arg1 == 1; sp34++) {
        func_8029A7E4("0x%x, ", sp28[sp34]);
    }
    func_8029A7E4("\n");
    /* An assert that folds away; only its strings are left. */
    if (sizeof(PlayerInfo) > 512) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sizeof(playerInfo)<=512", "pfsHandler.c", 0);
    }
    if (arg1 == 1) {
        func_801F75A4(sp28, 0x100);
    }
    if (D_802E8BF8 != 0 || D_80364A90 == 0x40000000000000) {
        if (D_8039C4B4 == 0 || D_802FA264 != 0) {
            for (sp34 = 0; sp34 < 0x100; sp34++) {
                if (arg1 == 1) {
                    D_8039B6B0[sp34] = sp28[sp34];
                    D_8020BEE0[sp34] = sp28[sp34];
                } else {
                    D_8039B6B0[sp34] = D_8020BEE0[sp34];
                    sp28[sp34] = D_8039B6B0[sp34];
                }
            }
        } else if (arg1 == 1) {
            func_802042D0(&D_80370BF8, 0, sp28, 0x100);
        } else {
            func_80204410(&D_80370BF8, 0, sp28, 0x100);
        }
    } else {
        sp2C = 0;
        do {
            sp3C = osPfsFindFile(&D_8039B630, PAK_COMPANY_CODE, PAK_GAME_CODE, D_8020C000, D_8020C014, &D_8039B698[arg0]);
            sp2C++;
        } while (sp3C != 0 && sp2C < 3);
        if (sp3C == 0) {
            sp2C = 0;
            do {
                sp3C = osPfsReadWriteFile(&D_8039B630, D_8039B698[arg0], arg1, 0, 0x100, sp28);
                sp2C++;
            } while (sp3C != 0 && sp2C < 3);
        }
    }
    if (sp3C == 0 && arg1 == 0) {
        sp3C = func_801F76E4(sp28, 0x100);
    }
    if (sp3C == 0 && arg1 == 0) {
        func_801F6BD0(arg0, &sp20);
        if (sp20 != 0x87569AB6CD076AEC) {
            sp3C = 0x6E382;
        }
    }
    return sp3C;
}

s32 func_801F65C4(u8 arg0, u8 arg1, u8 arg2) {
    s32 sp34;
    s32 sp30;
    s32 sp2C;
    u32 sp28;
    u8 *sp24;

    sp34 = 0;
    sp24 = (u8 *)D_80364EF0[arg0];
    sp28 = (arg1 << 5) + 0x100;
    if (arg2 == 1) {
        func_801F75A4(sp24, 0x20);
    }
    if (D_802E8BF8 != 0) {
        for (sp30 = sp28; sp30 < sp28 + 0x20; sp30++) {
            if (arg2 == 1) {
                D_8039B6B0[sp30] = sp24[sp30 - sp28];
            } else {
                sp24[sp30 - sp28] = D_8039B6B0[sp30];
            }
        }
    } else {
        sp34 = osPfsFindFile(&D_8039B630, PAK_COMPANY_CODE, PAK_GAME_CODE, D_8020C000, D_8020C014, &D_8039B698[arg0]);
        if (sp34 == 0) {
            sp34 = osPfsReadWriteFile(&D_8039B630, D_8039B698[arg0], arg2, sp28, 0x20, sp24);
        }
        for (sp30 = 0; sp30 < 0xE; sp30++) {
            func_8029A7E4("%d TIME %d = %d\n", arg2, sp30, D_80364EF0[arg0][D_802E8C44[sp30]]);
        }
    }
    if (sp34 == 0 && arg2 == 0) {
        sp34 = func_801F76E4(sp24, 0x20);
    }
    return sp34;
}

s32 func_801F67E4(u8 arg0, u8 arg1, u8 arg2) {
    s32 sp3C;
    s32 sp38;
    u8 sp37;
    u16 *sp30;

    sp3C = 0;
    sp37 = arg1 * 2;
    sp30 = &D_80364F70[sp37 & ~3];
    if (D_802E8BF8 != 0) {
        if (arg0 != D_80364AEA) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "pn==playerNumberAtStart", "pfsHandler.c", LINE_EU(0x25C, 0x26F));
        }
        if (arg2 == 1) {
            D_80364F70[sp37] = D_80364EF0[arg0][D_802E8C44[D_80364AF0[arg0].unk92[arg1]]];
            D_80364F70[sp37 + 1] = D_80364F70[sp37] ^ 0x55AA;
            func_8029A7E4("%d %d EEWRITE %x %x\n", arg1, D_80364F70[sp37], (u32)(sp37 * 2 + 0x100) >> 3, sp30);
            osEepromWrite(&D_80370BF8, (u32)(sp37 * 2 + 0x100) >> 3, (u8 *)sp30);
        } else {
            osEepromRead(&D_80370BF8, (u32)(sp37 * 2 + 0x100) >> 3, (u8 *)sp30);
            for (sp38 = 0; sp38 < 2; sp38++, arg1++) {
                if (LEVEL_DONE_IN(D_80364AF0[arg0], arg1) &&
                    !DUMMY_LEVELS(arg1)) {
                    D_80364EF0[arg0][D_802E8C44[D_80364AF0[arg0].unk92[arg1]]] = D_80364F70[arg1 * 2];
                    func_8029A7E4("%d EETIMES: %d %d\n", arg1, D_80364F70[arg1 * 2], D_80364F70[arg1 * 2 + 1] ^ 0x55AA);
                    if (D_80364F70[arg1 * 2] != (D_80364F70[arg1 * 2 + 1] ^ 0x55AA)) {
                        sp3C = 0x6E382;
                    }
                }
            }
        }
    }
    return sp3C;
}

s32 func_801F6AF4(u8 arg0, u64 arg2) {
    s32 sp24;
    PlayerInfo *sp20;

    sp24 = 0;
    sp20 = &D_80364AF0[arg0];
    if (D_802E8BF8 != 0 || D_80364A90 == 0x40000000000000) {
        osEepromWrite(&D_80370BF8, 0x3F, (u8 *)&arg2);
    } else {
        func_8029A7E4("PUTTING SEMAPHORE %llu\n", arg2);
        sp24 = osPfsReadWriteFile(&D_8039B630, D_8039B698[arg0], 1, 0xDE0, 0x20, (u8 *)&arg2);
    }
    return sp24;
}

#ifdef VERSION_EU
/* The language, saved in the EEPROM's block 0x3E (arg0 1) or read from it. */
s32 func_801F7120_eu(u8 arg0) {
    u64 sp38; /* unused */
    UnkStruct_8020CAB0_eu sp28;
    u64 sp20;

    if (arg0 == 1) {
        sp28 = D_8020CAB0_eu;
        osEepromWrite(&D_80370BF8, 0x3E, (u8 *)&sp28.lang[D_80366F70_eu]);
    } else {
        osEepromRead(&D_80370BF8, 0x3E, (u8 *)&sp20);
        switch (sp20) {
            case 0x1945194519451945:
                D_80366F70_eu = 1;
                break;
            case 0x1982198219821982:
                D_80366F70_eu = 0;
                break;
            default:
                D_80366F70_eu = 0;
                break;
        }
    }
    return 0;
}
#endif

s32 func_801F6BD0(u8 arg0, u64 *arg1) {
    s32 sp44;
    PlayerInfo *sp40;
    u64 sp20[4];

    sp44 = 0;
    sp40 = &D_80364AF0[arg0];
    if (D_802E8BF8 != 0) {
        osEepromRead(&D_80370BF8, 0x3F, (u8 *)arg1);
    } else {
        sp44 = osPfsReadWriteFile(&D_8039B630, D_8039B698[arg0], 0, 0xDE0, 0x20, (u8 *)sp20);
        *arg1 = sp20[0];
        func_8029A7E4("Getting SEMAPHORE %llu\n", *arg1);
    }
    return sp44;
}

s32 func_801F6CA4(u8 arg0, u8 arg1, u8 arg2) {
    s32 sp34;
    s32 sp30;
    s32 sp2C;
    s32 sp28;
    s32 sp24;

    sp34 = 0;
    sp28 = 0;
    for (sp2C = 0; sp2C < arg1; sp2C++) {
        if (D_802E8F94[sp2C].unk0 == 1 && !DUMMY_LEVELS(sp2C)) {
            sp28++;
        }
    }
    sp24 = (sp28 << 6) + 0x880;
    if (arg2 == 1) {
        func_801F75A4(D_8039C4B8, 0x40);
    }
    if (D_802E8BF8 != 0) {
        for (sp2C = sp24; sp2C < sp24 + 0x40; sp2C++) {
            if (arg2 == 1) {
                D_8039B6B0[sp2C] = D_8039C4B8[sp2C - sp24];
            } else {
                D_8039C4B8[sp2C - sp24] = D_8039B6B0[sp2C];
            }
        }
    } else {
        sp30 = osPfsFindFile(&D_8039B630, PAK_COMPANY_CODE, PAK_GAME_CODE, D_8020C000, D_8020C014, &D_8039B698[arg0]);
        sp34 = sp30;
        if (sp30 == 0) {
            sp34 = osPfsReadWriteFile(&D_8039B630, D_8039B698[arg0], arg2, sp24, 0x40, D_8039C4B8);
        }
    }
    if (sp34 == 0 && arg2 == 0) {
        sp34 = func_801F76E4(D_8039C4B8, 0x40);
    }
    return sp34;
}

s32 func_801F6ED4(u8 arg0) {
    return osPfsFileState(&D_8039B630, arg0, &D_80218B20[D_80218D28]);
}

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/E7B0/func_801F6F18.s")
#else
s32 func_801F6F18(void) {
    s32 sp44;
    s32 sp40;
    s32 sp3C;
    OSMesg sp38;
    s32 sp34;
    s32 sp30;
    s32 sp2C;

    D_80218D28 = 0;
    for (sp44 = 0; sp44 < 0x10; sp44++) {
        do {
            osSendMesg(&D_80219EF8, OS_MESG((u32)((u32)(sp44 << 16) | 0x11) | 0x01000000), OS_MESG_BLOCK);
            osRecvMesg(&D_80219F50, &sp38, OS_MESG_BLOCK);
            if (sp38 != NULL) {
                sp44++;
            }
        } while (sp38 != NULL && sp44 < 0x10);
        if (sp44 < 0x10) {
            bcopy(D_80218B20[D_80218D28].game_name, D_802189C0[D_80218D28], 0x11);
            bcopy(D_80218B20[D_80218D28].ext_name, D_80218AD0[D_80218D28], 5);
            D_802189C0[D_80218D28][0x10] = 0;
            D_80218AD0[D_80218D28][4] = 0;
            sp34 = D_80218B20[D_80218D28].file_size >> 5 >> 3;
            sp30 = D_80218AD0[D_80218D28][0];
            func_801F7410(D_802189C0[D_80218D28]);
            func_801F7410(D_80218AD0[D_80218D28]);
            sprintf(D_80218740[sp44], "%16s%c%-4s (%d)", D_802189C0[D_80218D28], sp30 ? '.' : ' ', D_80218AD0[D_80218D28],
                    sp34);
            func_8029A7E4("%s\n", D_80218740[sp44]);
            if (sp34 < 0x63) {
                sprintf(D_80218740[sp44], "%s ", D_80218740[sp44]);
            }
            if (sp34 < 9) {
                sprintf(D_80218740[sp44], "%s ", D_80218740[sp44]);
            }
            /* (in the port, D_8020C488 as what it is: the text of D_8020C070[37] on) */
#ifdef VERSION_EU
#ifdef TARGET_PC
            (&D_8020C070[FE_ENTRY(37) + D_80218D28].text)[D_80366F70_eu] = D_80218740[sp44];
#else
            (&D_8020C488[D_80218D28].text)[D_80366F70_eu] = D_80218740[sp44];
#endif
#else
#ifdef TARGET_PC
            D_8020C070[FE_ENTRY(37) + D_80218D28].text = D_80218740[sp44];
#else
            D_8020C488[D_80218D28].text = D_80218740[sp44];
#endif
#endif
            D_80218D28++;
        }
    }
    if (D_80218D28 == 0) {
        sprintf(D_80218740[0], "%s", "PAK EMPTY!");
#ifdef VERSION_EU
        (&D_8020C070[FE_ENTRY(37)].text)[D_80366F70_eu] = D_80218740[0];
#else
        D_8020C070[FE_ENTRY(37)].text = D_80218740[0];
#endif
        sp2C = 1;
    } else {
        sp2C = 0;
    }
    D_802F8BDC[18].unk18 = (D_80218D28 + sp2C + 1) / 2 + 0x24;
    D_802F8BDC[18].count = D_80218D28 + sp2C + 4;
    osSendMesg(&D_80219EF8, (OSMesg)0x0100000E, OS_MESG_BLOCK);
    osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
#ifdef VERSION_EU
    sprintf(D_80219F90,
            D_80366F70_eu == 0   ? "%d PAGES FREE"
            : D_80366F70_eu == 1 ? "%d SEITEN FREI"
                                 : NULL,
            D_80218EF0 / 32 / 8);
    (&D_8020C070[FE_ENTRY(35)].text)[D_80366F70_eu] = D_80219F90;
    sprintf(D_80219FB0,
            D_80366F70_eu == 0   ? "%d NEEDED PER PLAYER"
            : D_80366F70_eu == 1 ? "%d PRO SPIELER BENOETIGT"
                                 : NULL,
            0xE);
    (&D_8020C070[FE_ENTRY(36)].text)[D_80366F70_eu] = D_80219FB0;
    (&D_8020C070[FE_ENTRY(10)].text)[D_80366F70_eu] = D_80366F70_eu == 0   ? "DELETE THIS FILE?"
                                                      : D_80366F70_eu == 1 ? "SPEICHER LOESCHEN?"
                                                                           : NULL;
    D_8020C070[FE_ENTRY(10)].unk10 = NULL;
#else
    sprintf(D_80219F90, "%d PAGES FREE", D_80218EF0 / 32 / 8);
    D_8020C070[FE_ENTRY(35)].text = D_80219F90;
    sprintf(D_80219FB0, "%d NEEDED PER PLAYER", 0xE);
    D_8020C070[FE_ENTRY(36)].text = D_80219FB0;
    D_8020C070[FE_ENTRY(10)].text = "DELETE THIS FILE?";
    D_8020C070[FE_ENTRY(10)].unk10 = D_80301080;
#endif
    return D_80218D28 != 0;
}
#endif

s32 func_801F73FC(void) {
    return D_80218D28 != 0;
}

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/E7B0/func_801F7410.s")
#else
void func_801F7410(u8 *arg0) {
    s32 sp1C;

    for (sp1C = 0; sp1C < func_8025B300(arg0); sp1C++) {
        if (arg0[sp1C] < 0x45) {
            arg0[sp1C] = D_8020C01C[arg0[sp1C]];
        } else {
            arg0[sp1C] = '-';
        }
    }
}
#endif

#ifndef VERSION_JP
void func_801F74B0(u8 *arg0) {
    s32 sp1C;
    s32 sp18;

    for (sp1C = 0; sp1C < func_8025B300(arg0); sp1C++) {
        sp18 = 0;
        if (D_8020C01C[0] != arg0[sp1C]) {
            do {
            } while (++sp18 < 0x45 && D_8020C01C[sp18] != arg0[sp1C]);
        }
        if (sp18 != 0x45) {
            arg0[sp1C] = sp18;
        } else {
            arg0[sp1C] = 0xF;
        }
    }
}
#endif

s32 func_801F75A4(u8 *arg0, s32 arg1) {
    s32 sp34;
    s32 sp30;
    s32 sp2C;
    u8 sp28[4];
    s32 sp24;

    sp2C = (arg1 + 0x7F) >> 7;
    for (sp34 = 0; sp34 < 4; sp34++) {
        sp28[sp34] = 0;
        arg0[arg1 + sp34 - 4] = sp28[sp34];
    }
    for (sp34 = 0; sp34 < sp2C; sp34++) {
        for (sp30 = 0; sp30 < 4; sp30++) {
            if ((sp24 = (sp34 << 7) + (sp30 << 5)) < arg1) {
                sp28[sp30] += __osContDataCrc(arg0 + sp24);
            }
        }
    }
    for (sp34 = 0; sp34 < 4; sp34++) {
        arg0[arg1 + sp34 - 4] = sp28[sp34];
    }
    return 0;
}

s32 func_801F76E4(u8 *arg0, s32 arg1) {
    s32 sp3C;
    s32 sp38;
    s32 sp34;
    u8 sp30[4];
    u8 sp2C[4];
    s32 sp28;

    sp34 = (arg1 + 0x7F) >> 7;
    for (sp3C = 0; sp3C < 4; sp3C++) {
        sp30[sp3C] = arg0[arg1 + sp3C - 4];
        sp2C[sp3C] = 0;
        arg0[arg1 + sp3C - 4] = sp2C[sp3C];
    }
    for (sp3C = 0; sp3C < sp34; sp3C++) {
        for (sp38 = 0; sp38 < 4; sp38++) {
            if ((sp28 = (sp3C << 7) + (sp38 << 5)) < arg1) {
                sp2C[sp38] += __osContDataCrc(arg0 + sp28);
            }
        }
    }
    for (sp3C = 0; sp3C < 4; sp3C++) {
        if (sp2C[sp3C] != sp30[sp3C]) {
            return 0x6E382;
        }
    }
    return 0;
}
