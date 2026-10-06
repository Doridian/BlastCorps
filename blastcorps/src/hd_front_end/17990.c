#include "common.h"
#include "game/sched.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

/* A 0x1C-byte record of D_8020C070. */
typedef struct {
    /* 0x00 */ u8 unk0[0xE];
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u8 unk12[0xA];
} UnkStruct_802F8BDC_1C; /* size = 0x1C */

u32 osVirtualToPhysical(void *);

extern OSMesgQueue D_80219F50;
extern u16 D_8036BB16;
extern s16 D_8036BB1A;
extern s16 D_8036BB1E;
extern s16 D_8036BB20;
extern u8 D_80364AE9;
extern u64 D_8021A830;
extern u8 D_802154B0;
extern OSMesgQueue D_80219EF8;
extern s32 D_80358080;
extern s32 D_80358084;
extern s32 D_80358078;
extern FrameBuf D_803156F8[];
extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern s8 D_80364A71;
extern s32 D_80364A64;
extern OSMesgQueue D_80315180;
extern u8 D_80365060[];
#ifdef VERSION_EU
extern u64 D_80364A88;
extern u8 D_80366F70_eu; /* the language: 0 English, 1 German, 2 French */
#endif

/* .bss, 0x8021AB70-0x8021AB80 (tools/bss_c.py) */
u8 D_8021AB70;
char D_8021AB72[2];
s16 D_8021AB74;
s16 D_8021AB76;
u8 D_8021AB78[4];
s32 D_8021AB7C;

/* The game modes func_801E9718 draws the backdrop in, and the ones that wait
   for the Yoshi menus to finish: eu's language menu (0x200000000) waits,
   0x20000 has no backdrop. */
#ifdef VERSION_EU
#define BACKDROP_MODES 0x818D04001AF98080
#define WAIT_MODES 0x81D9836783B28000
#else
#define BACKDROP_MODES 0x818D04001AFB8080
#define WAIT_MODES 0x81D9836583B28000
#endif

void func_801FE990(void) {
    s32 spDC;
    Gfx *spD8;
    OSMesg spD4;

    if (D_80364A90 & 0x0000080010000000) {
        if (D_8021AB7C == 0) {
            func_80260650(D_80367738, 0x73, &D_8021AB7C);
        }
        if (D_8036BB1C == 2) {
            osRecvMesg(&D_80219F50, &spD4, OS_MESG_BLOCK);
            if (!MQ_IS_EMPTY(&D_80219F50)) {
                func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "MQ_IS_EMPTY(&pakToGameMessageQ)", "back_loop.c", LINE_EU(62, 63));
            }
            D_8021AB70 = !spD4;
            if (D_80364A90 == 0x10000000) {
                if (D_8021AB70 != 0) {
                    func_801EA278();
                }
                D_80364A98 = 0x8000;
            } else {
                D_80364A98 = 0x0000002000000000;
            }
            func_802608C8(D_8021AB7C);
            func_8026AF6C(0x4000);
        }
    }
    if (D_8036BB16 != 0) {
        func_8029A7E4("yoshiSelection in back_loop is %d %d\n", D_8036BB16, func_8026F92C(D_80364A90));
        switch (D_80364A90) {
            case 0x10000:
                if (D_8036BB16 == 0xFFFF) {
                    D_80364A98 = 0x8000000000000000;
                    func_801E8EB8(4, 1);
                } else if (D_802F8BDC[10].unk18 - 2 < 4) {
                    D_80364A98 = 0x2000000;
                } else if (D_802154B0 == 4) {
                    D_8021AB70 = 0;
                    D_8039C541 = 1;
                    D_80364A98 = 0x8000;
                } else {
                    D_80364A98 = 0x800000;
                }
                break;
            case 0x100000000:
                if ((D_8036BB16 != 0xFFFF) && (func_801F73FC() != 0)) {
                    D_8021AB74 = D_8036BB16;
                    D_80364A98 = 0x0000004000000000;
                }
                break;
            case 0x400000:
                if (D_8036BB16 == 0xFFFF) {
                    func_801E8EB8(4, 1);
                    D_80364A98 = 0x200000;
                } else {
                    D_80364AE8 = D_8036BB16 - 2;
                    if (D_80364AE8 < 4) {
                        D_80364A98 = 0x100000;
                    } else {
                        D_80364A98 = 0x200000;
                    }
                }
                break;
            case 0x80000:
                if (D_8036BB16 == 0xC) {
                    func_801EA108(D_80364AE8, 0, 0);
                }
                func_801E8EB8(4, 1);
                D_80364A98 = 0x800000;
                break;
            case 0x0000008000000000:
                if (D_8036BB16 == 0xC) {
                    osSendMesg(&D_80219EF8, (OSMesg)((D_8021AB74 << 16) | 5), OS_MESG_BLOCK);
                }
                D_80364A98 = 0x0000010000000000;
                break;
            case 0x80:
            case 0x8000000:
                if (D_8036BB16 == 0xFFFF) {
                    D_80364A98 = 0x4000;
                } else {
                    switch (spDC = ((UnkStruct_802F8BDC_1C *)D_802F8BDC)[D_8036BB18].unk10 +
                                   ((UnkStruct_802F8BDC_1C *)D_802F8BDC)[D_8036BB18].unkE - D_8036BB16 - 1) {
                        case 0:
                            D_80364A98 = 0x4000;
                            break;
                        case 1:
                            D_80364A98 = 0x40;
                            break;
                        case 2:
                            if (!(D_80364AA8 & 0x81)) {
                                if ((D_80364AA8 == 2) && (D_80364A90 == 0x8000000)) {
                                    D_8039CA60 = 1;
                                }
                                D_80364A98 = 0x20000000;
                            } else {
                                D_80364A98 = 0x2000;
                            }
                            break;
                        case 3:
                            D_80364A98 = 0x0000040000000000;
                            break;
                    }
                }
                break;
            case 0x0000040000000000:
                if (D_8036BB16 == 0xFFFF) {
                    D_80364AE8 = D_80364AE9;
                }
                if ((D_80364AE8 == D_80364AE9) || (D_80364AE8 == D_80364AEA) || (D_8036BB16 == 0xFFFF)) {
                    D_80364A98 = D_8021A830;
                } else {
                    D_80364A98 = 0x0100000000000000;
                }
                break;
            case 0x0004000000000000:
                func_801E8C40(4);
                D_80364A98 = 0x0008000000000000;
                break;
            case 0x0001000000000000:
                func_80275390(0x0020000000000000);
                break;
            case 0x0040000000000000:
                if (D_8036BB16 == 0xC) {
                    func_801EA108(D_80364AE8, 0, 1);
                }
                break;
            case 0x0100000000000000:
                func_801E8EB8(4, 1);
                break;
            case 0x4000000000000000:
                if (D_8036BB16 == 0xC) {
                    func_80275270(0x0400000000000000, 0.5f);
                } else {
                    func_80275270(0x4000, 0.5f);
                }
                break;
#ifdef VERSION_EU
            case 0x200000000:
                if (D_8036BB16 == 0xFFFF) {
                    if (D_80364A88 == 0x2000000) {
                        D_80364A98 = 0x200000;
                    } else {
                        func_80275270(0x0400000000000000, 0.5f);
                    }
                } else {
                    func_8029A7E4("Selected language %d\n", D_80366F70_eu = D_8036BB16 - 0xD8);
                    osSendMesg(&D_80219EF8, (OSMesg)0x1000017, OS_MESG_BLOCK);
                    osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
                    D_80364A98 = 0x20000;
                }
                break;
#endif
            default:
                func_8029A7E4("backdrop illegal yoshi selection\n");
                break;
        }
        D_8021AB76 = D_8036BB16;
        D_8036BB16 = 0;
    }
    D_80358080 = 0;
    D_80358084 = 0;
    func_802A5720();
    func_80284E54(D_803156F8[D_8035805C].dl, D_80358078, 1, 1, 1234, 0);
    D_8035805C ^= 1;
    spD8 = D_803156F8[D_8035805C].dl;
    gSPSegment(spD8++, 0, 0);
    gSPSegment(spD8++, 2, osVirtualToPhysical(&D_803156F8[D_8035805C]));
    gSPSegment(spD8++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(spD8++, D_01000038);
    gSPDisplayList(spD8++, D_01000010);
    gSPClipRatio(spD8++, FRUSTRATIO_3);
    gDPSetCycleType(spD8++, G_CYC_FILL);
    if (D_80364A90 & 0x88000080) {
        gDPSetDepthImage(spD8++, D_80358058);
        gDPSetColorImage(spD8++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358058);
        gDPSetFillColor(spD8++, 0xFFFCFFFC);
        gDPFillRectangle(spD8++, 220, 110, 300, 160);
    } else if (D_80364A90 & 0x40000000) {
        gDPSetDepthImage(spD8++, D_80358058);
        gDPSetColorImage(spD8++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358058);
        gDPSetFillColor(spD8++, 0xFFFCFFFC);
        gDPFillRectangle(spD8++, 0, 0, 319, 239);
    }
    gDPSetColorImage(spD8++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    func_80259450();
    spD8 = func_80200BE0(spD8, &D_803156F8[D_8035805C], &D_80358078);
    gDPPipeSync(spD8++);
    gDPSetCycleType(spD8++, G_CYC_1CYCLE);
    if (D_80364A90 & 0x000C000000000000) {
        spD8 = func_8026BBD0(spD8, &D_803156F8[D_8035805C], &D_80358078);
    }
    if (D_80364A90 == 0x0004000000000000) {
        if (D_80358060 == 0) {
            func_80262008(0x27, 0.0f);
            func_80260EE0(0xF);
        }
        if ((D_8036BB1E == 0) && (D_80364AE8 == 4)) {
            D_80364AE8 = D_80364AEA;
            func_801E8DCC(D_80364AEA);
            D_802F8BDC[56].unk8 |= 0x20;
        }
    }
    if (D_80364A90 & BACKDROP_MODES) {
        spD8 = func_801E9718(spD8, &D_803156F8[D_8035805C], 0xC2);
    }
    if (D_80364A90 & 0x898C0FE313F78002) {
        spD8 = func_8025C878(spD8, &D_803156F8[D_8035805C], D_8035805C, &D_80358078);
    }
    if (D_80364A90 & 0x410000) {
        u8 sp7F;

        sp7F = D_80364AE8;
        D_80364AE8 = D_802F8BDC[10].unk18 - 2;
        if (D_80364AE8 != sp7F) {
            if (sp7F == 4) {
                func_801E8EB8(D_80364AE8, 0);
            } else {
                func_801E8EB8(D_80364AE8, 1);
            }
        }
    }
    func_8028A3E4();
    if (D_80364A90 & 0x40000000) {
        spD8 = func_801ED800(spD8, &D_803156F8[D_8035805C], D_8035805C, &D_80358078);
    }
    if (D_80364A90 == 0x0001000000000000) {
        spD8 = func_80201364(D_803156F8, spD8);
    }
    spD8 = func_8024C404(spD8, &D_803156F8[D_8035805C], &D_80358078);
    if (D_80364A90 & 0x88000080) {
        spD8 = func_801EC770(spD8, &D_803156F8[D_8035805C], &D_80358078);
        if ((D_80364A71 != -1) && (D_8036BB1C == 2)) {
            spD8 = func_801F51C8(&D_803156F8[D_8035805C], spD8);
            gSPClearGeometryMode(spD8++, G_ZBUFFER);
        }
    }
    if (D_80364A90 == 0x80000000) {
        if (D_80364A64 != 0) {
            D_80364A64--;
        } else if ((D_8036BB1E != 2) && (D_8036BB1C == 2)) {
            func_8026AF6C(0x4000);
            func_80260650(D_80367738, 0x1C, NULL);
        }
    }
    func_8028A470();
#ifdef TARGET_PC
    /* Start or A leaves the "leaders of" screens for the menu, as it does
       the attract mode's demos (00000.c's mode 2) */
    if (D_80364A90 == 0x0001000000000000 && (D_80370C28 & ~D_80370C2A & 0x9000) && func_802753C0() == 0 &&
        D_80364A98 == 0 && port_intro_skip()) {
        func_80260650(D_80367738, 0x1E, NULL);
        func_80260B40(0, 0);
        func_80260B40(5, 0);
        func_80275390(0x0020000000000000);
    }
#endif
    if (D_80364A90 == 0x0000800000000000) {
        func_802862DC();
    }
    if ((D_80364A90 & 0x88000080) && (D_803156C4 % 20 >= 6) && (D_80364A71 != -1) && (D_8036BB1C == 2)) {
        s32 sp74;
        s32 sp70;

        sp74 = 0x118, sp70 = 0x8C;
        sprintf(D_8021AB72, "%d", D_80364A71);
        func_80259CCC(&D_803156F8[D_8035805C], D_8021AB72, 0, 1, 0, sp74, sp70, 0x1A, 0x16, 1, 0, 0, 0,
                      D_8036BB20);
        func_80259DC8(&D_803156F8[D_8035805C], D_8021AB72, 0, 1, 0, sp74 + 2, sp70 + 2, 0x12, 0x12, 1, 0xFF,
                      0xFF, 0, D_8036BB20, 0xFF, 0, 0, D_8036BB20);
    }
    if (D_80364A90 & 0x0000040000000000) {
        func_801F803C();
        spD8 = func_801F8440(&D_803156F8[D_8035805C], spD8);
    }
    func_80259C24(&spD8, &D_803156F8[D_8035805C]);
    spD8 = func_80274BF0(&D_803156F8[D_8035805C], spD8);
    if (!(D_80364A90 & 0x000C000000000000)) {
        spD8 = func_8026BBD0(spD8, &D_803156F8[D_8035805C], &D_80358078);
    }
    if ((D_8036BB1C != 1) && (D_8036BB18 == 0xB)) {
        spD8 = func_801EAA7C(spD8, &D_803156F8[D_8035805C], &D_80358078);
    }
    gDPFullSync(spD8++);
    gSPEndDisplayList(spD8++);
    D_80358078 = spD8 - D_803156F8[D_8035805C].dl;
    for (spDC = 0; spDC < D_80358080; spDC++) {
        osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    }
    for (spDC = 0; spDC < D_80358080 - D_80358084; spDC++) {
        func_802A57AC();
    }
    if ((D_80364A90 & WAIT_MODES) && (D_8036BB1C == 1) && (func_802753F8() == 0) &&
        (func_802753C0() == 0) && (D_80364A98 == 0) && (D_8036BB1A == -1)) {
        switch (D_80364A90) {
            case 0x20000:
                D_80364A98 = 0x40000;
                break;
            case 0x200000:
                D_80364A98 = 0x10000;
                break;
            case 0x0000010000000000:
                D_80364A98 = 0x100000000;
                break;
            case 0x100000:
                D_80364A98 = 0x80000;
                ENTRY_TEXT(&D_8020C070[FE_ENTRY(9)]) = ENTRY_TEXT(&D_8020C070[D_80364AE8 + 2]);
                D_8020C070[FE_ENTRY(9)].unk10 = 0;
                D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0x14;
                func_8026AF6C(0x800C);
                break;
            case 0x0000004000000000:
                D_80364A98 = 0x0000008000000000;
                ENTRY_TEXT(&D_8020C070[FE_ENTRY(9)]) = ENTRY_TEXT(&D_8020C070[D_8021AB74]);
                D_8020C070[FE_ENTRY(9)].unk10 = D_8020C070[D_8021AB74].unk10;
#ifdef VERSION_JP
                D_8020C070[FE_ENTRY(9)].unk6 = 0xB;
                D_8020C070[FE_ENTRY(9)].unk8 = 0xF;
#else
                D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0xF;
#endif
                func_8026AF6C(0x800C);
                break;
            case 0x800000:
                D_80364A98 = 0x400000;
                break;
            case 0x2000000:
                D_80364AEA = D_80364AE9 = D_80364AE8;
                if (D_80365060[D_80364AE8] == 1) {
                    func_80275390(0x4000);
                } else {
#ifdef VERSION_EU
                    D_80364A98 = 0x200000000; /* the language menu */
#else
                    D_80364A98 = 0x20000;
#endif
                }
                break;
            case 0x8000000000000000:
                func_80275390(0x0400000000000000);
                break;
            case 0x1000000:
                osSendMesg(&D_80219EF8, (OSMesg)((D_80364AE8 << 16) | 7), OS_MESG_BLOCK);
                osSendMesg(&D_80219EF8, (OSMesg)((D_80364AE8 << 16) | 0x15 | 0x1000000), OS_MESG_BLOCK);
                func_802995F0(4);
                func_80275390(0x0000100000000000);
                break;
            case 0x80000000:
                D_80364A98 = 0x40000000;
                break;
            case 0x8000:
                if (D_8039C541 != 0) {
                    func_80275390(0x0020000000000000);
                } else if (D_8021AB70 != 0) {
                    D_80364A98 = 0x10000;
                } else {
                    D_80364A98 = 0x10000000;
                }
                break;
            case 0x0000002000000000:
                if (D_8021AB70 != 0) {
                    D_80364A98 = 0x100000000;
                } else {
                    D_80364A98 = 0x0000080000000000;
                }
                break;
            case 0x100000000:
            case 0x0040000000000000:
                func_80275270(0x10, 0.5f);
                break;
            case 0x0000800000000000:
                func_80275270(0x0002000000000000, 0.75f);
                func_80261570(0.0f);
                break;
            case 0x0001000000000000:
                func_80275270(2, 0.5f);
                break;
            case 0x0008000000000000:
                D_80364AE8 = D_80364AEA;
                func_80275270(0x4000, 0.6f);
                break;
            case 0x0080000000000000:
                if ((D_80364AE8 == D_80364AE9) || (D_80364AE8 == D_80364AEA)) {
                    D_80364A98 = D_8021A830;
                } else {
                    D_80364A98 = 0x0100000000000000;
                }
                break;
            case 0x0100000000000000:
                if (D_8021AB76 != 0xC) {
                    D_80364A98 = 0x0200000000000000;
                } else {
                    func_80275390(D_8021A830);
                }
                break;
            default:
                func_8029A7E4("illegal yoshi wait game mode %d\n", func_8026F92C(D_80364A90));
                break;
        }
    }
}
