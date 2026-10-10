/*
 * hd_front_end E7B0 (the Controller Pak's menus), jp's own: the three
 * functions Blastdozer compiled differently from the US versions, which the
 * C has as jp's GLOBAL_ASM (blastcorps/src/hd_front_end/E7B0.c), as native C
 * (engine.h).  They are IDO's code, not Rare's: o32, and the C calls them by
 * the prototypes it declares.
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

/* the Pak thread and its queues */
void func_801F57B0(void) {
    s32 size = 0xDE0;

    osCreateThread(&D_80218D30, 2, func_801F58E8, NULL, D_80218EF8 + 0x1000, 0xB);
    osCreateMesgQueue(&D_80219EF8, D_80219F10, 8);
    osCreateMesgQueue(&D_80219F30, D_80219F48, 1);
    osCreateMesgQueue(&D_80219F50, D_80219F68, 8);
    func_8029A7E4("current pak file size is %d bytes\n", size);
    func_8029A7E4("current playerInfo size is %d bytes\n", 0x100);
    osScAddClient(&D_80315440, &D_80218EE0, &D_80219F30, 1, 3);
    if (size >= 0xE00) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "filesize<PFS_FILE_SIZE", "pfsHandler.c",
                      0x68);
    }
    osStartThread(&D_80218D30);
}

/* the Pak's files as menu entries (37 on), their pages free and needed */
s32 func_801F6F18(void) {
    s32 i, k, len, size, ext, empty;
    u16 *name;
    OSMesg msg;
    u16 buf[0x18];

    D_80218D28 = 0;
    i = 0;
    do {
        do {
            osSendMesg(&D_80219EF8, OS_MESG(((u32)i << 16 | 0x11) | 0x01000000), OS_MESG_BLOCK);
            osRecvMesg(&D_80219F50, &msg, OS_MESG_BLOCK);
            if (msg != NULL) {
                i++;
            }
            if (msg == NULL)
                break;
        } while (i < 0x10);
        if (i < 0x10) {
            bcopy(D_80218B20[D_80218D28].game_name, D_802189C0[D_80218D28], 0x11);
            bcopy(D_80218B20[D_80218D28].ext_name, D_80218AD0[D_80218D28], 5);
            D_802189C0[D_80218D28][0x10] = 0;
            D_80218AD0[D_80218D28][4] = 0;
            size = (u32)D_80218B20[D_80218D28].file_size >> 5 >> 3;
            ext = D_80218AD0[D_80218D28][0];
            name = func_801F7410(D_802189C0[D_80218D28]);
            func_8025B5D4((u16 *)D_80218740[i], D_80301040, name, 0);
            if (ext != 0) {
                name = func_801F7410(D_80218AD0[D_80218D28]);
                func_8025B5D4(buf, D_8020BFEC_jp, name, 0);
                func_8025B918((u16 *)D_80218740[i], buf);
            }
            k = 0;
            len = func_8025B370((u16 *)D_80218740[i]);
            if (k < 0x15 - len) {
                do {
                    func_8025B918((u16 *)D_80218740[i], D_8020BFFC_jp);
                    k++;
                } while (k < 0x15 - len);
            }
            func_8025B5D4(buf, D_8020BFF4_jp, NULL, size);
            func_8025B918((u16 *)D_80218740[i], buf);
            if (size < 0x63) {
                func_8025B918((u16 *)D_80218740[i], D_8020BFFC_jp);
            }
            if (size < 9) {
                func_8025B918((u16 *)D_80218740[i], D_8020BFFC_jp);
            }
            D_8020C070[37 + D_80218D28].unk10 = (u16 *)D_80218740[i];
            D_8020C070[37 + D_80218D28].unk6 = 0xB;
            D_80218D28++;
        }
        i++;
    } while (i < 0x10);
    if (D_80218D28 == 0) {
        func_8025B5D4((u16 *)D_80218740[0], D_80301040, D_803010D0, 0);
        D_8020C070[37].unk10 = (u16 *)D_80218740[0];
        empty = 1;
    } else {
        empty = 0;
    }
    D_802F8BDC[18].unk18 = (D_80218D28 + empty + 1) / 2 + 0x24;
    D_802F8BDC[18].count = D_80218D28 + empty + 4;
    osSendMesg(&D_80219EF8, (OSMesg)0x0100000E, OS_MESG_BLOCK);
    osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
    func_8025B5D4((u16 *)D_80219F90, D_80304A60, NULL, D_80218EF0 / 32 / 8);
    D_8020C070[35].unk10 = (u16 *)D_80219F90;
    func_8025B5D4((u16 *)D_80219FB0, D_80304A70, NULL, 0xE);
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

    len = func_8025B300(s);
    if (i < len) {
        do {
            u8 c = s[i];

            if (c >= 0x10) {
                if (c < 0x95) {
                    D_80219F00_jp[i] = c;
                    goto next;
                }
            }
            D_80219F00_jp[i] = 0x1002;
        next:
            i++;
            len = func_8025B300(s);
        } while (i < len);
    }
    D_80219F00_jp[i] = 0xFFE;
    return D_80219F00_jp;
}

#endif
