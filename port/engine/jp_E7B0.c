/*
 * hd_front_end E7B0 (the Controller Pak's menus), jp's own: the three
 * functions Blastdozer compiled differently from the US versions, which the
 * C has as jp's GLOBAL_ASM (blastcorps/src/hd_front_end/E7B0.c), as native C
 * charged by the original's blocks (engine.h).  They are IDO's code, not
 * Rare's: o32, and the C calls them by the prototypes it declares.
 *
 * jp shows the Pak's file names in its u16 text (0x0FFE-terminated, 0x1002
 * a space): func_801F7410 turns a name into that, into D_80219F00_jp, and
 * func_801F6F18 lists the files with it.
 */
#include "engine.h"
#include "game/yoshi.h"
#include "game/sched.h"

#ifdef VERSION_JP

extern s32 D_80218D28;
extern s32 D_80218EF0;
extern OSPfsState D_80218B20[0x10];
extern u8 D_802189C0[][0x11];
extern u8 D_80218AD0[0x10][5];
extern char D_80218740[0x10][0x50];
extern char D_80219F90[0x40];
extern char D_80219FB0[0x40];
extern u16 D_80219F00_jp[0x20];
extern u16 D_8020BFEC_jp[4];
extern u16 D_8020BFF4_jp[4];
extern u16 D_8020BFFC_jp[2];
extern char D_8020F3E0[];
extern u16 D_80301040[], D_803010D0[], D_80301080[], D_80304A60[], D_80304A70[];
extern OSThread D_80218D30;
extern SchedClient D_80218EE0;
extern u8 D_80218EF8[0x1000];
extern OSMesgQueue D_80219EF8, D_80219F30, D_80219F50;
extern OSMesg D_80219F10[8], D_80219F48[2], D_80219F68[8];
extern Sched D_80315440;

void func_8029A7E4(char *, ...);
void func_801F58E8(void *);
u16 *func_801F7410(u8 *);
s32 func_8025B300(u8 *);
s32 func_8025B370(u16 *);
u16 *func_8025B5D4(u16 *, u16 *, u16 *, s32);
void func_8025B918(u16 *, u16 *);

/* the Pak thread and its queues */
void func_801F57B0(void) {
    s32 size = 0xDE0;

    ENGINE_BLK(801F57B0);
    osCreateThread(&D_80218D30, 2, func_801F58E8, NULL, D_80218EF8 + 0x1000, 0xB);
    ENGINE_BLK(801F57F4);
    osCreateMesgQueue(&D_80219EF8, D_80219F10, 8);
    ENGINE_BLK(801F580C);
    osCreateMesgQueue(&D_80219F30, D_80219F48, 1);
    ENGINE_BLK(801F5824);
    osCreateMesgQueue(&D_80219F50, D_80219F68, 8);
    ENGINE_BLK(801F583C);
    func_8029A7E4("current pak file size is %d bytes\n", size);
    ENGINE_BLK(801F584C);
    func_8029A7E4("current playerInfo size is %d bytes\n", 0x100);
    ENGINE_BLK(801F585C);
    osScAddClient(&D_80315440, &D_80218EE0, &D_80219F30, 1, 3);
    ENGINE_BLK(801F5884);
    if (size >= 0xE00) {
        ENGINE_BLK(801F5894);
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "filesize<PFS_FILE_SIZE", "pfsHandler.c",
                      0x68);
    }
    ENGINE_BLK(801F58B4);
    osStartThread(&D_80218D30);
    ENGINE_BLK(801F58C0);
}

/* the Pak's files as menu entries (37 on), their pages free and needed */
s32 func_801F6F18(void) {
    s32 i, k, len, size, ext, empty;
    u16 *name;
    OSMesg msg;
    u16 buf[0x18];

    ENGINE_BLK(801F6F18);
    D_80218D28 = 0;
    i = 0;
    do {
        do {
            ENGINE_BLK(801F6F30);
            osSendMesg(&D_80219EF8, (OSMesg)(((u32)i << 16 | 0x11) | 0x01000000), OS_MESG_BLOCK);
            ENGINE_BLK(801F6F60);
            osRecvMesg(&D_80219F50, &msg, OS_MESG_BLOCK);
            ENGINE_BLK(801F6F74);
            if (msg != NULL) {
                ENGINE_BLK(801F6F80);
                i++;
            }
            ENGINE_BLK(801F6F8C);
            if (msg == NULL)
                break;
            ENGINE_BLK(801F6F98);
        } while (i < 0x10);
        ENGINE_BLK(801F6FA8);
        if (i < 0x10) {
            ENGINE_BLK(801F6FB8);
            bcopy(D_80218B20[D_80218D28].game_name, D_802189C0[D_80218D28], 0x11);
            ENGINE_BLK(801F6FF0);
            bcopy(D_80218B20[D_80218D28].ext_name, D_80218AD0[D_80218D28], 5);
            ENGINE_BLK(801F7028);
            D_802189C0[D_80218D28][0x10] = 0;
            D_80218AD0[D_80218D28][4] = 0;
            size = (u32)D_80218B20[D_80218D28].file_size >> 5 >> 3;
            ext = D_80218AD0[D_80218D28][0];
            name = func_801F7410(D_802189C0[D_80218D28]);
            ENGINE_BLK(801F70B4);
            func_8025B5D4((u16 *)D_80218740[i], D_80301040, name, 0);
            ENGINE_BLK(801F70E8);
            if (ext != 0) {
                ENGINE_BLK(801F70F4);
                name = func_801F7410(D_80218AD0[D_80218D28]);
                ENGINE_BLK(801F7114);
                func_8025B5D4(buf, D_8020BFEC_jp, name, 0);
                ENGINE_BLK(801F7130);
                func_8025B918((u16 *)D_80218740[i], buf);
            }
            ENGINE_BLK(801F7154);
            k = 0;
            len = func_8025B370((u16 *)D_80218740[i]);
            ENGINE_BLK(801F7178);
            if (k < 0x15 - len) {
                do {
                    ENGINE_BLK(801F7198);
                    func_8025B918((u16 *)D_80218740[i], D_8020BFFC_jp);
                    ENGINE_BLK(801F71C0);
                    k++;
                } while (k < 0x15 - len);
            }
            ENGINE_BLK(801F71E0);
            func_8025B5D4(buf, D_8020BFF4_jp, NULL, size);
            ENGINE_BLK(801F71F8);
            func_8025B918((u16 *)D_80218740[i], buf);
            ENGINE_BLK(801F721C);
            if (size < 0x63) {
                ENGINE_BLK(801F722C);
                func_8025B918((u16 *)D_80218740[i], D_8020BFFC_jp);
            }
            ENGINE_BLK(801F7254);
            if (size < 9) {
                ENGINE_BLK(801F7264);
                func_8025B918((u16 *)D_80218740[i], D_8020BFFC_jp);
            }
            ENGINE_BLK(801F728C);
            D_8020C070[37 + D_80218D28].unk10 = (u16 *)D_80218740[i];
            D_8020C070[37 + D_80218D28].unk6 = 0xB;
            D_80218D28++;
        }
        ENGINE_BLK(801F7300);
        i++;
    } while (i < 0x10);
    ENGINE_BLK(801F7314);
    if (D_80218D28 == 0) {
        ENGINE_BLK(801F7324);
        func_8025B5D4((u16 *)D_80218740[0], D_80301040, D_803010D0, 0);
        ENGINE_BLK(801F7344);
        D_8020C070[37].unk10 = (u16 *)D_80218740[0];
        empty = 1;
    } else {
        ENGINE_BLK(801F7364);
        empty = 0;
    }
    ENGINE_BLK(801F7368);
    if (D_80218D28 + empty + 1 < 0)
        ENGINE_BLK(801F738C);
    ENGINE_BLK(801F7394);
    D_802F8BDC[18].unk18 = (D_80218D28 + empty + 1) / 2 + 0x24;
    D_802F8BDC[18].count = D_80218D28 + empty + 4;
    osSendMesg(&D_80219EF8, (OSMesg)0x0100000E, OS_MESG_BLOCK);
    ENGINE_BLK(801F73D4);
    osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
    ENGINE_BLK(801F73E8);
    if (D_80218EF0 < 0)
        ENGINE_BLK(801F740C);
    ENGINE_BLK(801F7414);
    if (D_80218EF0 / 32 < 0)
        ENGINE_BLK(801F7420);
    ENGINE_BLK(801F7428_801F6F18);
    func_8025B5D4((u16 *)D_80219F90, D_80304A60, NULL, D_80218EF0 / 32 / 8);
    ENGINE_BLK(801F7430);
    D_8020C070[35].unk10 = (u16 *)D_80219F90;
    func_8025B5D4((u16 *)D_80219FB0, D_80304A70, NULL, 0xE);
    ENGINE_BLK(801F7460);
    D_8020C070[36].unk10 = (u16 *)D_80219FB0;
    D_8020C070[10].text = D_8020F3E0;
    D_8020C070[10].unk10 = D_80301080;
    return D_80218D28 != 0;
}

/* a Pak file's name, its bytes 0x10-0x94 as they are and anything else a
   space (0x1002), into D_80219F00_jp as u16 text, which it returns (the C
   declares no result; its one caller is func_801F6F18) */
u16 *func_801F7410(u8 *s) {
    s32 i = 0, len;

    ENGINE_BLK(801F7410);
    len = func_8025B300(s);
    ENGINE_BLK(801F7428_801F7410);
    if (i < len) {
        do {
            u8 c = s[i];

            ENGINE_BLK(801F7438);
            if (c >= 0x10) {
                ENGINE_BLK(801F7454);
                if (c < 0x95) {
                    ENGINE_BLK(801F745C);
                    D_80219F00_jp[i] = c;
                    goto next;
                }
            }
            ENGINE_BLK(801F7470);
            D_80219F00_jp[i] = 0x1002;
        next:
            ENGINE_BLK(801F7488);
            i++;
            len = func_8025B300(s);
            ENGINE_BLK(801F749C);
        } while (i < len);
    }
    ENGINE_BLK(801F74AC);
    D_80219F00_jp[i] = 0xFFE;
    return D_80219F00_jp;
}

#endif
