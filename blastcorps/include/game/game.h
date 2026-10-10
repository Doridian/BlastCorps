#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "game/types.h"

/*
 * The game's global state that most files share.  All of it is hd_code
 * 00000.c's (hd.c's) .bss unless noted.  What each is for is given with its
 * evidence; the names stay address names until they are renamed everywhere.
 */

/*
 * Game modes, one bit each.  hd.c prints both words through func_8026F92C
 * (the number of the set bit) as "enter initlevel game_mode=%d
 * loop_done=%d".
 */
extern u64 D_80364A90; /* game_mode */
extern u64 D_80364A98; /* loop_done */

/*
 * The heap: a bump allocator that hd.c's setup starts at 0x8004B400, above
 * the two framebuffers.  D_80358070 is the next free byte; the level setup
 * loads the level file there (at D_80358074) and prints what the level took
 * ("exit initlevel allocated %d bytes, %x").  func_80257490(&D_80358070, n)
 * rounds it up to a multiple of n.  Segment 1 (D_8035806C) is separate: the
 * static data hd.c loads from ROM to the end of .bss (D_803FF600, "Static
 * end = 0x%x").
 */
extern u8 *D_80358070;

/* Which of the two frames is being built (0 or 1): picks the framebuffer
 * (D_80000400[i]), the per-frame buffer (D_803156F8) and the task slot. */
extern u8 D_8035805C;

/* Nonzero while the game is paused: the scheduler's level timer
 * (Sched.unk280, "TIME IN LEVEL") stops. */
extern u8 D_802E8BD0;

/* The player's (the current vehicle's) position in world units << 5: level
 * data compares x, z (and y) >> 5 with its boxes. */
extern s32 D_803643E0;
extern s32 D_803643E4;
extern s32 D_803643E8;

/*
 * The controller (hd_code 45BB0.c's .bss): the buttons held this frame and
 * the frame before, as OSContPad.button (A_BUTTON, START_BUTTON, ...); a
 * press is `(D_80370C28 & b) && !(D_80370C2A & b)`.
 */
extern u16 D_80370C28;
extern u16 D_80370C2A;

/* The first three fields of an OSContPad: D_80370C30 (45BB0.c's .bss) keeps
 * a copy of one (the buttons, stick x, stick y); 17210.c (recording.c)
 * plays back into its buttons. */
typedef struct UnkStruct_80370C30 {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ s8 unk2;
    /* 0x3 */ s8 unk3;
} UnkStruct_80370C30;
SIZE_CHECK(UnkStruct_80370C30, 4);

extern UnkStruct_80370C30 D_80370C30;

/* Nonzero once hd_front_end is loaded (academy.c's assert "frontEndPresent"). */
extern u8 D_80370C50;

/*
 * More of the state several files share, each declared once (the files had
 * them with different types; where a file needs the other signedness it
 * casts).  hd.c's (00000.c's) unless noted.
 */
extern u16 *D_80358050[2];      /* the two colour framebuffers (gDPSetColorImage, D_8035805C picks) */
extern Vtx *D_80365348[2];      /* 14B30.c's: the HUD text's vertices, a frame's each (port/host/gfx.c reads them) */
extern u16 *D_80358058;         /* the depth buffer (gDPSetDepthImage) */
extern u32 D_80358060;          /* a frame count (a lit TntCrate keeps it in unk14; compared with 100, 300 and 700) */
extern u8 *D_8035806C;          /* segment 1: the static data at the end of .bss */
extern u8 D_802E8BD8;
extern u8 D_802E8BE4;           /* counts down (set to 10 when a TNT crate blows) */
extern s32 D_802E8BEC;          /* -1 until set; passed to func_8025B9D0 */
extern u8 D_803643D4;
extern u8 D_803643D5;
extern u8 D_803643D9;           /* set by 48D00.c when a collision test sets D_803F932D and the crate blows */
extern u8 D_803643DA;
extern u8 D_80364410;
extern u32 D_80364428;          /* LevelInfo.unk3C << 5 */
extern u32 D_803649F0;          /* the damage so far (LevelStats.ip, or the player's saved one) */
extern u8 D_80364A68;
extern u8 D_80364A6A;
extern u8 D_80364A6C;
extern u8 D_80364A87;           /* bit flags */
extern s32 D_80364AA8;          /* bit flags (tested with 0x20; compared with 2 and 0x40) */
extern u8 D_80365580;           /* 17210.c's (recording.c's) */
extern s16 D_80366A04;          /* 17E10.c's: compared with D_80358060 */
extern u8 D_80366A18;           /* 17E10.c's */
extern u32 D_8036B968;          /* 23C20.c's: a random state, seeded from osGetCount() */
extern u8 D_80370C24;           /* 45BB0.c's (controller.c's) */
extern u8 D_80370C27;
extern u8 D_80370C75;           /* 46F60.c's */
extern u8 D_8039CA60;           /* 50670.c's (ghostdigger.c's) */

#endif
