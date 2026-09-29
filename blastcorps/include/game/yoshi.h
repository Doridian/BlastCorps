#ifndef GAME_YOSHI_H
#define GAME_YOSHI_H

#include "game/types.h"

/*
 * The game's windows: menus, message boxes, the mission briefings.  Rare
 * calls the system "yoshi" (hd_code 26570.c is yoshi.c: its assert
 * "!(yoshiState==YOSHI_OFF && currentYoshiWindow!=NO_YOSHI_WINDOW)" tests
 * D_8036BB1C == 1 && D_8036BB18 != -1, and it prints "yoshiDemand=%x").
 *
 * A YoshiWindow (D_802F8BDC, 108 of them, yoshi.c's .data) shows `count`
 * YoshiEntry rows from `first` in an entry table: D_802F5804 (yoshi.c, the
 * in-game windows: "SELECT OPTION", "MORE", "VIEW STATS", ...) or D_8020C070
 * (hd_front_end's data-only menu object).  D_8036BB10 points at the table in
 * use; the front end builds sorted lists of entries at D_8036BB24
 * (func_801F7FF4 compares two).  "Entry" is our name, not Rare's.
 *
 * The same tables are also reached through labels that fall inside them:
 * D_802F8BF4 is &D_802F8BDC[0].unk18, D_8020C488 &D_8020C070[37].text.
 */

typedef struct YoshiEntry {
    /* 0x00 */ u16 flags;         /* 0x81, 0x20, 0x400, ... */
    /* 0x02 */ s16 x;             /* a list's entries share x and step y by their height */
    /* 0x04 */ s16 y;
    /* 0x06 */ u16 unk6;          /* 24, 20, 15: the text's cell size? */
    /* 0x08 */ u16 unk8;
    /* 0x0A */ u8 padA[2];
    /* 0x0C */ char *text;        /* "SELECT OPTION", "MORE", ... */
#ifdef VERSION_EU
    /* 0x10 */ char *text2;       /* eu: the German text ("OPTIONEN", "WEITER", ...) */
    /* 0x14 */ char *text3;       /* eu: NULL in the tables */
#endif
    /* 0x10 */ u16 *unk10;        /* yoshi.c's tables: the text in the 0x0FFE-terminated u16 encoding
                                     (A44D0, BC8E0); the front end also stores char strings here */
    /* 0x14 */ u8 unk14;          /* the YoshiIcon it shows */
    /* 0x15 */ u8 pad15;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u8 unk18;
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 unk1A;          /* that icon's handle (func_80272C5C) */
} YoshiEntry;
#ifdef VERSION_EU
SIZE_CHECK(YoshiEntry, 0x24);     /* eu: the offsets from unk10 on are 8 more */
#else
SIZE_CHECK(YoshiEntry, 0x1C);
#endif

typedef struct YoshiWindow {
    /* 0x00 */ u16 unk0;          /* bit flags */
    /* 0x02 */ u16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s32 unk8;          /* bit flags (0x400, 0x8000000, ...) */
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 first;         /* the window's first entry in the entry table */
    /* 0x10 */ u16 count;         /* and how many there are */
    /* 0x12 */ u16 unk12;
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;         /* the selected entry, an index into the table */
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 pad1B;
} YoshiWindow;
SIZE_CHECK(YoshiWindow, 0x1C);

/*
 * The icons an entry can show (D_802F49F4, 75 of them, yoshi.c's .data):
 * YoshiEntry.unk14 is the index (hd_front_end 7800.c: "getting icon %d"),
 * and func_80272C5C starts one, returning its handle into YoshiEntry.unk1A.
 */
typedef struct YoshiIcon {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u8 unk4;           /* the number of frames */
    /* 0x05 */ u8 pad5;
    /* 0x06 */ u8 unk6[0x14];     /* the frames, two u16 each ((bank << 8) | index), as func_80272C5C reads them */
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 unk1B[10];
    /* 0x25 */ u8 unk25;
    /* 0x26 */ u8 unk26;
    /* 0x27 */ u8 pad27;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ u8 unk2C;
    /* 0x2D */ u8 unk2D;
    /* 0x2E */ s8 unk2E;
    /* 0x2F */ u8 pad2F;
} YoshiIcon;
SIZE_CHECK(YoshiIcon, 0x30);

/* D_802F47B0[0x17] (yoshi.c's .data): two RGBA colours, the corners of the
 * shaded boxes func_80276130 draws (the front end passes both, twice). */
typedef struct ColorPair {
    /* 0x0 */ u8 r0;
    /* 0x1 */ u8 g0;
    /* 0x2 */ u8 b0;
    /* 0x3 */ u8 a0;
    /* 0x4 */ u8 r1;
    /* 0x5 */ u8 g1;
    /* 0x6 */ u8 b1;
    /* 0x7 */ u8 a1;
} ColorPair;
SIZE_CHECK(ColorPair, 8);

/* D_8020C488 is &D_8020C070[37].text, reached as its own symbol: an array
 * of YoshiEntry seen from their text field. */
typedef struct UnkStruct_8020C488 {
    /* 0x00 */ char *text;
    /* 0x04 */ u8 unk4[sizeof(YoshiEntry) - 4];
} UnkStruct_8020C488;
SIZE_CHECK(UnkStruct_8020C488, sizeof(YoshiEntry));

extern YoshiWindow D_802F8BDC[0x6C];
extern ColorPair D_802F47B0[0x17];
extern UnkStruct_8020C488 D_8020C488[];
extern YoshiIcon D_802F49F4[0x4B];
/*
 * The entry tables differ in jp and eu: jp lacks some of the US versions'
 * entries, eu has more, so the ones after those sit elsewhere.
 * YOSHI_ENTRY(n) and FE_ENTRY(n) turn the US versions' index into the
 * version's.
 *
 * D_802F5804 lacks entries 298-300, 375-377 and 468-473 in jp; eu adds
 * three before 184, one before 189 and three before 190.
 * D_8020C070 lacks 84-85, 111-113, 169-174, 199 and 202-203 in jp.
 */
#if defined(VERSION_EU)
#define YOSHI_ENTRY(n) ((n) + 3 * ((n) >= 184) + ((n) >= 189) + 3 * ((n) >= 190))
#define FE_ENTRY(n) (n)
#elif defined(VERSION_JP)
#define YOSHI_ENTRY(n) ((n) - 3 * ((n) >= 301) - 3 * ((n) >= 378) - 6 * ((n) >= 474))
#define FE_ENTRY(n) ((n) - 2 * ((n) >= 86) - 3 * ((n) >= 114) - 6 * ((n) >= 175) - ((n) >= 200) \
                     - 2 * ((n) >= 204))
#else
#define YOSHI_ENTRY(n) (n)
#define FE_ENTRY(n) (n)
#endif
#define YOSHI_ENTRIES YOSHI_ENTRY(0x1DA)

/* The number of entries of a window over [first, first + n) in the English
 * versions' table, in the version's. */
#define YOSHI_COUNT(first, n) (YOSHI_ENTRY((first) + (n)) - YOSHI_ENTRY(first))
#define FE_COUNT(first, n) (FE_ENTRY((first) + (n)) - FE_ENTRY(first))

extern YoshiEntry D_802F5804[YOSHI_ENTRIES];
extern YoshiEntry D_8020C070[];
extern YoshiEntry *D_8036BB10;
extern YoshiEntry *D_8036BB24;

/* yoshiState and currentYoshiWindow (see above). */
extern s16 D_8036BB1C;
extern s16 D_8036BB18;
#define YOSHI_OFF 1
#define NO_YOSHI_WINDOW -1

#endif
