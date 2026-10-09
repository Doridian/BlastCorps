#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include "game/types.h"

/*
 * The per-player save record, `playerInfo` in pfsHandler.c (hd_front_end
 * E7B0.c: "current playerInfo size is %d bytes" with 0x100, and the folded
 * assert "sizeof(playerInfo)<=512").  The game keeps four, one per Controller
 * Pak slot, in D_80364AF0 (academy.c's assert calls the array `players` and
 * the index, D_80364AE8, `playerNumber`).  The record is saved and loaded
 * whole: to the EEPROM at 0 or to the start of the pak file (EepromSave,
 * PAK_*), with a checksum over its last four bytes (func_801F75A4).
 *
 * D_8020BEE0 (E7B0.c's .data, 0x120 bytes) takes the record in place of the
 * EEPROM when D_8039C4B4 is 0 or D_802FA264 is set; it starts out as a
 * record named "PLAYER1" with unk10 0x1063E and gameState 1.
 */
typedef struct PlayerInfo {
    /* 0x00 */ char name[8];      /* printed as the slot's name; "NEW GAME" when unused */
    /* 0x08 */ u8 levelno;        /* the current level; academy.c "players[playerNumber].levelno==50" */
    /* 0x09 */ u8 unk9;
    /* 0x0A */ u16 units;         /* hd_front_end 7800.c prints it as "units %d"; at most 360 */
    /* 0x0C */ u8 unkC;           /* units / 12, at most 30 */
    /* 0x0D */ u8 padD[3];
    /* 0x10 */ u32 unk10;         /* bit flags, 1 << n */
    /* 0x14 */ s32 unk14;
    /* 0x18 */ u8 medal[60];      /* per level, MEDAL_* (LevelStats.coin when it was saved) */
    /* 0x54 */ u8 unk54[60];      /* per level, bit flags */
    /* 0x90 */ u8 unk90;          /* bit flags; 0x3F is all of them */
    /* 0x91 */ u8 gameState;      /* academy.c "going to game state %d"; LevelInfo.gameState */
    /* 0x92 */ u8 unk92[60];      /* per level, an index into D_802E8C44 (the time slots) */
    /* 0xCE */ u8 unkCE[0x20];
    /* 0xEE */ u8 unkEE;          /* bit flags */
    /* 0xEF */ u8 padEF;
    /* 0xF0 */ s32 unkF0;         /* bit flags, 1 << n */
    /* 0xF4 */ u8 unkF4[8];
    /* 0xFC */ u8 checksum[4];    /* func_801F75A4/func_801F76E4: __osContDataCrc of each 32 bytes */
} PlayerInfo;
SIZE_CHECK(PlayerInfo, 0x100);

/*
 * PlayerInfo.medal[level] (stats.c's "coin").  stats.c gives 3 for 100%, 2
 * for 90% and 1 for 70% of a level's targets, 5 below that; for a timed level
 * func_801EF2BC compares the time with LevelInfo.medalTimes and gives 4 for
 * the fastest (only from gameState 12 on) down to 1, and 5 when slower than
 * all four.  0 means not done yet, so "done" is 1..5 (LEVEL_DONE in 54E30.c,
 * LEVEL_DONE_IN in level.h).
 */
#define MEDAL_NONE 0     /* not done */
#define MEDAL_BRONZE 1
#define MEDAL_SILVER 2
#define MEDAL_GOLD 3
#define MEDAL_PLATINUM 4
#define MEDAL_DONE 5     /* done, without a medal */

/* D_80364AF0 players[4], D_80364AE8 playerNumber (hd_code 00000.c's .bss). */
extern PlayerInfo D_80364AF0[4];
extern u8 D_80364AE8;
/* The player the game started with: pfsHandler.c "pn==playerNumberAtStart". */
extern u8 D_80364AEA;

/*
 * The best times (hd.c's .bss): 16 per player, as LevelStats.tc.
 * D_802E8C44 (hd.c's .data) maps a slot number (PlayerInfo.unk92[level],
 * LevelStats.bdn) to an index here.  D_80364F70 is the EEPROM's copy,
 * EepromSave.times: per level the time and the time ^ 0x55AA.
 */
extern u16 D_80364EF0[4][16];
extern u16 D_80364F70[0x78];
#ifdef VERSION_EU
extern u8 D_802E8C44[0x10]; /* eu has two words after the first 16 */
#else
extern u8 D_802E8C44[0x1C];
#endif

/*
 * saveIt[playerNumber] (hd_code 48D00.c's .bss): levelno + 1 of the level
 * whose result is still to be saved, 0 for none (hd.c's master_switch.c
 * "!saveIt[playerNumber] || saveIt[playerNumber]==levelno+1", player.c
 * "saveIt[playerNumber]").
 */
extern u8 D_8039C53C[4];

/* Nonzero when the game saves to the EEPROM rather than a Controller Pak:
 * hd.c sets it when pak command 15 (func_801F5FE4) finds no pak, and
 * pfsHandler.c's read and write functions test it first. */
extern u8 D_802E8BF8;

/*
 * The save layout.  The game saves to the 4 Kbit EEPROM when there is no
 * Controller Pak (D_802E8BF8), and else to a pak file (PAK_COMPANY_CODE,
 * PAK_GAME_CODE, name "BLASTCORPS GAME", PFS_FILE_SIZE bytes, one per
 * player).
 *
 * EEPROM (512 bytes, blocks of 8):
 */
typedef struct EepromSave {
    /* 0x000 */ PlayerInfo player;      /* osEepromLongWrite at block 0 */
    /* 0x100 */ u16 times[60][2];       /* per level: time, time ^ 0x55AA (func_801F67E4) */
    /* 0x1F0 */ u8 pad1F0[8];
    /* 0x1F8 */ u64 semaphore;          /* block 0x3F, see SAVE_SEMAPHORE_* */
} EepromSave;
SIZE_CHECK(EepromSave, 0x200);

/* Values of the semaphore word (func_801F6AF4, func_801F6BD0): written as
 * _OK after a good save, and _BAD when the save is known to be broken. */
#define SAVE_SEMAPHORE_OK 0x87569AB6CD076AEC
#define SAVE_SEMAPHORE_BAD 0x2704197125121981

/*
 * The Controller Pak file (pfsHandler.c: "current pak file size is %d bytes"
 * 0xDE0, assert "filesize<PFS_FILE_SIZE"):
 *   0x000  PlayerInfo
 *   0x100  0x20-byte slots, the n given to func_801F65C4: D_80364EF0[player]
 *   0x880  0x40 bytes per ordinary level (LevelInfo.unk0 == 1, not
 *          DUMMY_LEVELS): D_8039C4B8 (func_801F6CA4)
 *   0xDE0  the semaphore, as on the EEPROM
 */
#define PFS_FILE_SIZE 0xE00
#define PAK_TIMES_OFFSET 0x100
#define PAK_LEVEL_DATA_OFFSET 0x880
#define PAK_SEMAPHORE_OFFSET 0xDE0

/* The pak file's company and game codes: "01" and the cartridge's code,
 * "NBCE" in the US and EU versions, "NBCJ" in jp. */
#define PAK_COMPANY_CODE 0x3031
#ifdef VERSION_JP
#define PAK_GAME_CODE 0x4E42434A
#else
#define PAK_GAME_CODE 0x4E424345
#endif

/*
 * The Controller Pak state, in 48D00.c's .bss though pfsHandler.c (E7B0.c)
 * and hd.c use it: D_8039C4B4 receives pfsHandler's replies (osRecvMesg),
 * D_8039C4B8 is a 0x40-byte block read from and written to the pak (a u64
 * 0x1234567887654321 first when it's valid), D_8039C53C[4] the level each
 * player still has to save, plus one.
 */
extern u8 D_8039C4B0;           /* 1 once pfsHandler.c has the pak */
extern s32 D_8039C4B4;
/* pfsHandler.c's (E7B0.c) pak: its file numbers by player, and the save
 * file's image, which stands in for the pak with D_802E8BF8 set. */
typedef struct PakState {
    u8 unk0[8];
    OSPfs pfs;          /* 0x08 */
    s32 fileNo[4];      /* 0x70 */
    u8 unk80[8];
    u8 file[0xE00];     /* 0x88 */
} PakState;
extern PakState D_8039B628;
/* IDO adds a field's offset to &D_8039B628 in an instruction of its own,
 * where the N64's code has each field's address in one lui/addiu. */
#ifndef TARGET_PC
extern OSPfs D_8039B630;
extern s32 D_8039B698[4];
extern u8 D_8039B6B0[0xE00];
#else
#define D_8039B630 (D_8039B628.pfs)
#define D_8039B698 (D_8039B628.fileNo)
#define D_8039B6B0 (D_8039B628.file)
#endif
/* D_8039C4F8[0x40]: the players below it get pak commands (E7B0.c sets 4
 * and lowers it to a player whose pak is full, 1C40.c reads it); the 0x40
 * before it a copy of D_8039C4B8 (hd_code 00000.c). */
extern u8 D_8039C4F8[0x44];
#ifndef TARGET_PC   /* the same for D_8039C4F8[0x40] */
extern u8 D_8039C538;
#else
#define D_8039C538 (D_8039C4F8[0x40])
#endif
extern u8 D_8039C540;           /* a level to save, plus one (1C40.c) */
extern u8 D_8039C541;

#endif
