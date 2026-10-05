#include "common.h"
#include "game/vehicle.h"
#include "game/game.h"

/* 5-byte records: a u16 value (big-endian), a repeat count, and two s8s. */
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ s8 unk3;
    /* 0x4 */ s8 unk4;
} UnkStruct_80365588; /* size = 0x5 */

extern u8 D_803643D6;
extern u8 D_803643DB;
extern u8 D_80364A50;
extern s16 D_8036BB1C;
extern s8 D_80370C32;
extern s8 D_80370C33;

void func_8029A7E4(char *, ...);
u8 func_80272C5C(u16 *, s32, s32, s32, s32, f32);
void func_80275270(u64, f32);
void func_80275390(u64);
s32 func_802753C0(void);

/* A recording file: a header, 0x400 input records, then a variable-length tail. */
typedef struct {
    /* 0x0000 */ u16 unk0;
    /* 0x0002 */ u16 unk2;
    /* 0x0004 */ u8 pad4[4];
    /* 0x0008 */ u16 unk8;
    /* 0x000A */ u8 unkA;
    /* 0x000B */ u8 unkB;
    /* 0x000C */ UnkStruct_80365588 unkC[0x400];
    /* 0x140C */ s16 unk140C;
} UnkStruct_8025B9D0;

extern u8 D_006A9F10[];
extern u8 D_006AD3F0[];

void func_8028B4C4(u8 *, u8 *, s32 *, s32, s32, s32);
void func_80257490(void *, s32);

/* .bss, 0x80365580-0x803669C0 (tools/bss_c.py) */
u8 D_80365580;
u8 D_80365581[1];
u8 D_80365582[2];
u8 D_80365584[4];
UnkStruct_80365588 D_80365588[1];
u8 D_8036558D[1];
u8 D_8036558E[2];
#ifdef VERSION_EU
u8 D_80365590[0x45FC]; /* eu's is 0x3200 bytes longer */
#else
u8 D_80365590[0x13FC];
#endif
u8 D_8036698C;
s32 D_80366990;
s32 D_80366994;
s32 D_80366998;
s32 D_8036699C;
s32 D_803669A0;
u16 D_803669A4;
s8 D_803669A6;
s8 D_803669A7;
u8 D_803669A8;
UnkStruct_80365588 *D_803669AC;
u8 *D_803669B0;
s32 D_803669B4;

/* .data, 0x802E8CB0-0x802E8CC0 (tools/data_c.py) */
u16 D_802E8CB0[8] = { 0x0920 }; /* texture ids (func_80272C5C) */

void func_8025B9D0(s32 arg0, s32 *arg1) {
    s32 sp34;
    u8 *sp30;
    s32 sp2C;
    s32 sp28;
    u16 sp26;
    UnkStruct_8025B9D0 *sp20;

    sp30 = D_006A9F10;
    sp2C = D_006AD3F0 - D_006A9F10;
    sp20 = (UnkStruct_8025B9D0 *)D_80358070;
    func_8028B4C4(sp30, D_80358070, &sp2C, 9, 0, 1);
    D_80358070 += sp2C;
    func_80257490(&D_80358070, 0x10);
    for (sp34 = 0; sp34 < arg0; sp34++) {
        sp28 = sp20->unk140C;
        sp20 = (UnkStruct_8025B9D0 *)((u8 *)sp20 + sp28 + 0x140E);
    }
    D_80366A04 = sp20->unk0;
    sp26 = sp20->unk2;
    D_80366998 = sp20->unk8;
    *arg1 = sp20->unkA;
    D_8036698C = sp20->unkB;
    D_803669AC = sp20->unkC;
    sp28 = sp20->unk140C;
    sp20 = (UnkStruct_8025B9D0 *)((u8 *)sp20 + 0x140E);
    func_80257490(&sp20, 2);
    D_803669B0 = (u8 *)sp20;
    D_80366994 = 0;
    D_803669A0 = D_803669AC->unk2;
    D_803669B4 = 1;
}

void func_8025BB38(void) {
    D_80366990 = 0;
    D_803669A8 = 1;
}

void func_8025BB50(void) {
    D_80366998 = D_80366990;
    D_80366994 = 0;
    D_803669AC = D_80365588;
    D_80365580 = func_80272C5C(D_802E8CB0, 0, 1, 1, 1, 1.0f);
    D_803669A0 = D_803669AC[D_80366994].unk2;
}

void func_8025BBE8(u16 arg0, s8 arg1, s8 arg2) {
    if (D_803669A8 == 0) {
        if ((arg0 != D_803669A4) || (arg1 != D_803669A6) || (arg2 != D_803669A7) || (D_8036699C == 0xFF) ||
            (D_80364A98 != 0)) {
#ifdef VERSION_EU
            if (D_80366990 < 0xE00) { /* D_80365590 is longer in eu */
#else
            if (D_80366990 < 0x400) {
#endif
                D_80365588[D_80366990].unk0 = D_803669A4 >> 8;
                D_80365588[D_80366990].unk1 = D_803669A4 & 0xFF;
                D_80365588[D_80366990].unk3 = D_803669A6;
                D_80365588[D_80366990].unk4 = D_803669A7;
                D_80365588[D_80366990].unk2 = D_8036699C;
                D_80366990++;
                D_8036699C = 0;
            }
        } else {
            D_8036699C++;
        }
    } else {
        D_8036699C = 0;
    }
    D_803669A4 = arg0;
    D_803669A6 = arg1;
    D_803669A7 = arg2;
    D_803669A8 = 0;
}

extern s32 D_803649E8;

void func_802AFC28(u8 *);
void func_802B0D70(u8 *);
void func_802AFFD4(void);
void func_802B2988(u8 *);
void func_802B1228(void);
void func_802B40D4(u8 *);
void func_802B2D7C(void);
void func_802B58C8(u8 *);
void func_802B448C(void);
void func_802B7308(u8 *);
void func_802B5CD8(void);
void func_802C80A0(u8 *);
void func_802C5714(void);
void func_802CB660(u8 *);
void func_802C9F54(void);
void func_802D2524(u8 *);
void func_802D0C68(void);

void func_8025BD98(void) {
    switch (D_8036698C) {
        case 3:
            func_802B40D4(D_803669B0);
            func_802B2D7C();
            break;
        case 0:
            func_802AFC28(D_803669B0);
            break;
        case 1:
            func_802B0D70(D_803669B0);
            func_802AFFD4();
            break;
        case 2:
            func_802B2988(D_803669B0);
            func_802B1228();
            break;
        case 16:
            func_802D2524(D_803669B0);
            func_802D0C68();
            break;
        case 4:
            func_802B58C8(D_803669B0);
            func_802B448C();
            break;
        case 5:
            func_802B7308(D_803669B0);
            func_802B5CD8();
            break;
        case 9:
            func_802C80A0(D_803669B0);
            func_802C5714();
            break;
        case 10:
            func_802CB660(D_803669B0);
            func_802C9F54();
            break;
    }
    if (D_8036698C != 0) {
        D_803649E8 = 1;
    }
    D_80364456 = D_8036698C;
    D_803669B4 = 0;
}

void func_8025BEF8(void) {
    s32 sp24;
    s32 sp20;

    if ((D_80366994 > D_80366998) || (D_803643DB != 0 && D_803643D6 != 0 && (D_80364A90 & 0x440))) {
        if ((D_8036BB1C != 1) || ((D_80364A90 & 0x440) && (D_80364A50 == 0))) {
            if (D_802E8BD0 == 0) {
                D_802E8BD8 = 1;
            }
        } else if (func_802753C0() == 0) {
            switch (D_80364A90) {
                case 0x2:
                    func_80275270(0x2, 0.25f);
                    break;
                case 0x100000000000:
                    func_8029A7E4("sequence playback over\n");
                    func_80275390(0x200000000000);
                    break;
                case 0x40:
                case 0x400:
                    D_802E8BD8 = 1;
                    if (D_80364A90 == 0x400) {
                        func_80275270(0x40, 1.25f);
                    } else {
                        func_80275270(0x40, 0.5f);
                    }
                    break;
                default:
                    func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "recording.c", LINE_EU(0x184, 0x18A));
                    break;
            }
        }
    } else {
        D_80370C30.unk0 = (D_803669AC[D_80366994].unk0 << 8) + D_803669AC[D_80366994].unk1;
        D_80370C32 = D_803669AC[D_80366994].unk3;
        D_80370C33 = D_803669AC[D_80366994].unk4;
        if (--D_803669A0 < 0) {
            D_80366994++;
            D_803669A0 = D_803669AC[D_80366994].unk2;
        }
        if ((D_80364A90 & 0x100000000002) && (D_8036698C != 9)) {
            if ((D_80370C30.unk0 & 0xC000) == 0x8000) {
                D_80370C30.unk0 &= ~0x8000;
                D_80370C30.unk0 |= 0x4000;
            }
            if ((D_80370C30.unk0 & 0xC000) == 0x4000) {
                D_80370C30.unk0 &= ~0x4000;
                D_80370C30.unk0 |= 0x8000;
            }
        }
    }
}
