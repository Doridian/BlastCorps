#include "common.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"


extern s32 D_80364BE0[][0x40];
extern s32 D_80358064;

void func_8026AF6C(s32);
void func_8029A7E4(char *, ...);
void func_8028A42C(void);
void func_8025BEF8(void);
void func_8025BBE8(u16, s8, s8);
void func_8028B734(s8 *, s8 *, u8);
void func_8028ADF0(u8, u8, u16 *, s8 *, s8 *);
void func_8028AFA4(u16, s8 *, s8 *);
void func_8028B0E8(u16 *, s8, s8);
void func_8028B190(s8 *, s8 *);

s32 D_802FDB10 = 0;
u8 D_802FDB14 = 0;

/* .bss, 0x80370BC0-0x80370C50 (tools/bss_c.py) */
s32 D_80370BC0;
OSContStatus D_80370BC8[MAXCONTROLLERS];
OSContPad D_80370BD8; /* read with osContGetReadData; its button word's low byte is also read on its own */
u8 D_80370BDE[2];
u8 D_80370BE0[0x10];
OSMesg D_80370BF0;
OSMesgQueue D_80370BF8;
u8 D_80370C10;
s8 D_80370C11;
s8 D_80370C12;
s8 D_80370C13;
s8 D_80370C14;
u8 D_80370C15;
u8 D_80370C16;
u8 D_80370C17;
u8 D_80370C18;
u8 D_80370C19;
u8 D_80370C1A;
u8 D_80370C1B;
u8 D_80370C1C;
u8 D_80370C1D;
u8 D_80370C1E;
u8 D_80370C1F;
u8 D_80370C20;
u8 D_80370C21;
u8 D_80370C22;
s8 D_80370C23;
u8 D_80370C24;
s8 D_80370C25;
s8 D_80370C26;
u8 D_80370C27;
u16 D_80370C28;
u16 D_80370C2A;
s8 D_80370C2C;
s8 D_80370C2D;
s8 D_80370C2E;
s8 D_80370C2F;
UnkStruct_80370C30 D_80370C30;
s8 D_80370C34;
s8 D_80370C35;
s32 D_80370C38;
u8 D_80370C3C[4];
s32 D_80370C40;

u8 func_8028A370(void) {
    s32 unused;
    u8 sp1B;

    osCreateMesgQueue(&D_80370BF8, &D_80370BF0, 1);
    osSetEventMesg(OS_EVENT_SI, &D_80370BF8, 0);
    osContInit(&D_80370BF8, &sp1B, D_80370BC8);
    osContSetCh(1);
    D_80370C10 = 0;
    D_80370C35 = 0;
    return sp1B;
}

/* Whether the SI is free for the pad (D_8039C4B0: the pak/EEPROM thread has
   it).  The port's --replay decides it by the movie instead, which is the
   save thread's timing (port_game.h). */
#ifdef TARGET_PC
#define PAD_SI_FREE() port_pad_read_due(D_8039C4B0 == 0)
#else
#define PAD_SI_FREE() (D_8039C4B0 == 0)
#endif

void func_8028A3E4(void) {
    if (PAD_SI_FREE()) {
        func_8028A42C();
        osContStartReadData(&D_80370BF8);
        D_80370C10 = 1;
    }
}

void func_8028A42C(void) {
    if (D_80370C10 != 0) {
        osRecvMesg(&D_80370BF8, NULL, OS_MESG_BLOCK);
        D_80370C10 = 0;
    }
}

void func_8028A470(void) {
    OSContPad *sp44;
    s32 unused;
    s32 sp3C;

    sp44 = &D_80370BD8;
    if (D_80358064 != 0) {
        if (PAD_SI_FREE() && D_80370C10 != 0) {
            osRecvMesg(&D_80370BF8, NULL, OS_MESG_BLOCK);
            osContGetReadData(sp44);
#ifdef VERSION_EU
            if (sp44->stick_x > 90) {
                sp44->stick_x = 90;
            }
            if (sp44->stick_x < -90) {
                sp44->stick_x = -90;
            }
            if (sp44->stick_y > 90) {
                sp44->stick_y = 90;
            }
            if (sp44->stick_y < -90) {
                sp44->stick_y = -90;
            }
#endif
            if (sp44->errno != 0) {
                func_8029A7E4("pad read error - zeroing data\n");
                sp44->button = 0;
                sp44->stick_x = sp44->stick_y = ((s8 *) sp44)[1];
            }
            D_80370C10 = 0;
        }
        D_80370C2A = D_80370C28;
        D_80370C28 = sp44->button;
        if (D_80370C38 != 0) {
            if (sp44->button & 0x4000) {
                sp44->button &= ~0x4000;
            } else {
                D_80370C38 = 0;
            }
        }
        if (D_80358060 < 6) {
            if (D_80358060 == 5) {
                D_80370C2A |= 0xD000;
            } else {
                D_80370C28 &= ~0xD000;
            }
        }
        D_80370C13 = D_80370C11;
        D_80370C14 = D_80370C12;
        D_80370C11 = sp44->stick_x;
        D_80370C12 = sp44->stick_y;
        D_80370C24 = D_80370C1E;
        D_80370C25 = D_80370C1F;
        D_80370C26 = D_80370C20;
        D_80370C27 = D_80370C21;
        switch (D_80364A90) {
            case 0x2:
            case 0x40:
            case 0x400:
            case 0x100000000000:
                if (D_802E8BDC != 0x26 && D_802E8BDC != 0x31) {
                    do {
                        func_8025BEF8();
                        if (D_80370C30.unk0 & 0x40) {
                            D_80370C40 = 0;
                        } else if (D_80370C30.unk0 & 0x80) {
                            D_80370C40 = 1;
                        }
                    } while (D_80370C30.unk0 & 0xC0);
                }
                sp44->button = D_80370C30.unk0;
                sp44->stick_x = D_80370C30.unk2;
                sp44->stick_y = D_80370C30.unk3;
                sp44->button &= ~8;
                break;
            case 0x4:
            case 0x100:
                if (D_802E8BD0 == 0 || D_80364A90 == 0x2000) {
                    func_8025BBE8(sp44->button & ~0x1000, sp44->stick_x, sp44->stick_y);
                }
                break;
            case 0x1:
            case 0x8:
            case 0x200:
            case 0x800:
            case 0x1000:
            case 0x4000000:
                sp44->button = 0;
                sp44->stick_x = 0;
                sp44->stick_y = 0;
                break;
        }
        D_80370C34 = 0;
        if (D_80364A90 & 0x2000100000002546) {
            D_803ED40A = 0;
            D_803F7C34 = 0;
            if (D_80364A90 & 0x440) {
                sp3C = D_80370C40;
            } else if (D_80364BE0[D_80364AE8][0] & (1 << D_80364456)) {
                sp3C = 1;
            } else {
                sp3C = 0;
            }
            if (D_80364A90 == 0x2 || D_80364A90 == 0x100000000000) {
                sp3C = 0;
            }
            D_80370C35 = sp3C;
            if (D_802E8BD0 != 0) {
                func_8028ADF0(1, 0, &sp44->button, &sp44->stick_x, &sp44->stick_y);
            } else {
                switch (sp3C) {
                    case 0:
                        switch (D_80364456) {
                            case 9:
                                if (D_803F7C3F != 0) {
                                    D_80370C34 = 1;
                                }
                            case 0:
                            case 2:
                            case 16:
                                func_8028B734(&sp44->stick_x, &sp44->stick_y, D_80364456);
                                func_8028B190(&sp44->stick_x, &sp44->stick_y);
                                break;
                            default:
                                D_80370C75 = 0;
                                func_8028ADF0(0, 0, &sp44->button, &sp44->stick_x, &sp44->stick_y);
                                break;
                        }
                        break;
                    case 1:
                        if (D_80364456 == 9) {
                            D_80370C75 = 0;
                            func_8028ADF0(1, 1, &sp44->button, &sp44->stick_x, &sp44->stick_y);
                        } else {
                            D_80370C75 = 0;
                            func_8028ADF0(1, 0, &sp44->button, &sp44->stick_x, &sp44->stick_y);
                        }
                        break;
                }
            }
        }
    } else {
        sp44->button = 0;
        sp44->stick_x = 0;
        sp44->stick_y = 0;
    }
    if (sp44->button & 0x200) {
        D_80370C15 = 1;
    } else {
        D_80370C15 = 0;
    }
    if (sp44->button & 0x100) {
        D_80370C16 = 1;
    } else {
        D_80370C16 = 0;
    }
    if (sp44->button & 0x800) {
        D_80370C17 = 1;
    } else {
        D_80370C17 = 0;
    }
    if (sp44->button & 0x400) {
        D_80370C18 = 1;
    } else {
        D_80370C18 = 0;
    }
    if (sp44->button & 0x1000) {
        D_80370C19 = 1;
    } else {
        D_80370C19 = 0;
    }
    if (sp44->button & 0x20) {
        D_80370C1A = 1;
    } else {
        D_80370C1A = 0;
    }
    if (sp44->button & 0x10) {
        D_80370C1B = 1;
    } else {
        D_80370C1B = 0;
    }
    if (sp44->button & 0x8000) {
        D_80370C1C = 1;
    } else {
        D_80370C1C = 0;
    }
    if (sp44->button & 0x4000) {
        D_80370C1D = 1;
    } else {
        D_80370C1D = 0;
    }
    if (sp44->button & 2) {
        D_80370C1E = 1;
    } else {
        D_80370C1E = 0;
    }
    if (sp44->button & 4) {
        D_80370C1F = 1;
    } else {
        D_80370C1F = 0;
    }
    if (sp44->button & 8) {
        D_80370C20 = 1;
    } else {
        D_80370C20 = 0;
    }
    if (sp44->button & 1) {
        D_80370C21 = 1;
    } else {
        D_80370C21 = 0;
    }
    if (sp44->button & 0x2000) {
        D_80370C22 = 1;
    } else {
        D_80370C22 = 0;
    }
    D_80370C23 = D_80370C1D || D_80370C22;
    D_80370C2E = D_80370C2C;
    D_80370C2F = D_80370C2D;
    D_80370C2C = sp44->stick_x;
    D_80370C2D = sp44->stick_y;
}

void func_8028ADF0(u8 arg0, u8 arg1, u16 *arg2, s8 *arg3, s8 *arg4) {
    if (arg0 == 0 && D_802E8BD0 == 0) {
        *arg4 = 0;
    }
    if (arg1 == 0) {
        func_8028AFA4(*arg2, arg3, arg4);
    }
    func_8028B190(arg3, arg4);
    if (arg1 == 0) {
        func_8028B0E8(arg2, *arg3, *arg4);
    }
}

void func_8028AE88(void) {
    D_80370C27 = 0;
    D_80370C26 = 0;
    D_80370C25 = 0;
    D_80370C24 = 0;
    D_80370C2D = 0;
    D_80370C2C = 0;
    D_80370C23 = 0;
    D_80370C22 = 0;
    D_80370C21 = 0;
    D_80370C20 = 0;
    D_80370C15 = D_80370C16 = D_80370C17 = D_80370C18 = D_80370C19 = D_80370C1A = D_80370C1B = D_80370C1C =
        D_80370C1D = D_80370C1E = D_80370C1F = 0;
    D_80370C2A = 0;
    D_80370C28 = 0;
    D_80370BC0 = 0;
    D_80370C38 = 0;
}

void func_8028AFA4(u16 arg0, s8 *arg1, s8 *arg2) {
    if (arg0 & 0x200) {
        *arg1 = -80;
    }
    if (arg0 & 0x100) {
        *arg1 = 80;
    }
    if (arg0 & 0x4000) {
        *arg2 = -80;
    }
    if (arg0 & 0x8000) {
        *arg2 = 80;
    }
}

void func_8028B000(s8 *arg0, s8 *arg1) {
    switch (D_80364456) {
        case 1:
        case 3:
        case 4:
        case 5:
        case 6:
            if (*arg0 < 51 && *arg0 >= -50) {
                *arg0 = 0;
            } else {
                if (*arg0 >= 51) {
                    *arg0 = 80;
                }
                if (*arg0 < -50) {
                    *arg0 = -80;
                }
            }
            if (*arg1 < 51 && *arg1 >= -50) {
                *arg1 = 0;
                return;
            }
            if (*arg1 >= 51) {
                *arg1 = 80;
            }
            if (*arg1 < -50) {
                *arg1 = -80;
            }
            break;
    }
}

void func_8028B0E8(u16 *arg0, s8 arg1, s8 arg2) {
    if (!(*arg0 & 0x200) && arg1 < -50) {
        *arg0 |= 0x200;
    }
    if (!(*arg0 & 0x100) && arg1 > 50) {
        *arg0 |= 0x100;
    }
    if (!(*arg0 & 0x8000) && arg2 > 50) {
        *arg0 |= 0x8000;
    }
    if (!(*arg0 & 0x4000) && arg2 < -50) {
        *arg0 |= 0x4000;
    }
}

void func_8028B190(s8 *arg0, s8 *arg1) {
    if (*arg0 < 10 && *arg0 > -10) {
        *arg0 = 0;
    }
    if (*arg0 > 80) {
        *arg0 = 80;
    }
    if (*arg0 < -80) {
        *arg0 = -80;
    }
    if (*arg1 < 10 && *arg1 > -10) {
        *arg1 = 0;
    }
    if (*arg1 > 80) {
        *arg1 = 80;
    }
    if (*arg1 < -80) {
        *arg1 = -80;
    }
}

#ifdef VERSION_EU
extern u8 D_80366F70_eu; /* the language: 0 English, 1 German, 2 French */
#endif

#ifdef VERSION_JP
extern u16 D_8030478C[];
extern u16 D_80304738[];
extern u16 D_8030475C[];
extern u16 D_80304774[];
#endif

void func_8028B240(void) {
#if defined(VERSION_EU)
    /* eu's labels in each language (French has the English ones) */
    char *sp34[3][3] = {
        { "SPEED ON 3D STICK?", "GASGEBEN MIT 3D-JOYSTICK?", "SPEED ON 3D STICK?" },
        { "360 DEGREE MODE?", "360 GRAD MODUS?", "360 DEGREE MODE?" },
        { "AIRBORNE 360' MODE?", "AIRBORNE 360 GRAD MODUS?", "AIRBORNE 360' MODE?" },
    };
#elif defined(VERSION_JP)
    /* jp's labels are its own u16 text (see func_80259EC4) */
    u16 *sp24[3] = { D_80304738, D_8030475C, D_80304774 };
#else
    char *sp24[3] = { "SPEED ON 3D STICK?", "360 DEGREE MODE?", "AIRBORNE 360' MODE?" };
#endif
    static s32 D_802FDB24[3] = { 0xEDBA, 0x10005, 0x200 };
    char *sp20;
    s32 sp1C;
    s32 sp18;

#if defined(VERSION_EU)
    sp20 = D_80366F70_eu == 0 ? "CONTROL METHOD:" : D_80366F70_eu == 1 ? "CONTROLLER MODUS:" : NULL;
#elif defined(VERSION_JP)
    sp20 = (char *)D_8030478C;
#else
    sp20 = "CONTROL METHOD:";
#endif
    sp1C = 0;
    sp18 = 0;
    do {
        if (D_802FDB24[sp18] & (1 << D_80364456)) {
            sp1C = 1;
        } else {
            sp18++;
        }
    } while (sp18 < 3 && sp1C == 0);
    if (sp1C == 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "found", "controller.c",
                      LINE_EU(0x203, 0x208));
    }
#if defined(VERSION_EU)
    (&D_802F5804[YOSHI_ENTRY(421)].text)[D_80366F70_eu] = sp20;
    D_802F5804[YOSHI_ENTRY(421)].unk10 = 0;
    (&D_802F5804[YOSHI_ENTRY(422)].text)[D_80366F70_eu] = sp34[sp18][D_80366F70_eu];
    D_802F5804[YOSHI_ENTRY(422)].unk6 = 0x13;
#elif defined(VERSION_JP)
    D_802F5804[YOSHI_ENTRY(421)].unk10 = (u16 *)sp20;
    D_802F5804[YOSHI_ENTRY(421)].text = NULL;
    D_802F5804[YOSHI_ENTRY(422)].unk10 = sp24[sp18];
    if (sp18 == 0) {
        D_802F5804[YOSHI_ENTRY(422)].unk6 = 0x10;
    } else {
        D_802F5804[YOSHI_ENTRY(422)].unk6 = 0x13;
    }
#else
    D_802F5804[YOSHI_ENTRY(421)].text = sp20;
    D_802F5804[YOSHI_ENTRY(421)].unk10 = 0;
    D_802F5804[YOSHI_ENTRY(422)].text = sp24[sp18];
    D_802F5804[YOSHI_ENTRY(422)].unk6 = 0x13;
#endif
    if ((D_80364BE0[D_80364AE8][0] ^ 0x10205) & (1 << D_80364456)) {
        D_802F8BDC[88].unk18 = YOSHI_ENTRY(0x1A8);
    } else {
        D_802F8BDC[88].unk18 = YOSHI_ENTRY(0x1A7);
    }
    func_8026AF6C(0x8058);
}
