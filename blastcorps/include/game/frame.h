#ifndef GAME_FRAME_H
#define GAME_FRAME_H

#include "game/types.h"

/*
 * The per-frame buffer: D_803156F8 (hd.c's .bss) holds two, 0x21498 bytes
 * each, and D_8035805C picks the one being built; segment 2 (D_02000000)
 * points at it while its display lists run.  The code that fills it decides
 * the layout below 0x48B0:
 *
 * - the front end and the yoshi windows use FrameBuf: matrices from 0,
 *   0x1E0 vertices from 0x1E00 (hd_code 30C70.c's segment-2 view agrees),
 *   the LookAt at 0x3C00 and one display list from 0x48B0;
 * - gameplay uses FrameGame (below): matrices to 0x1580, vertices from
 *   0x1580 to 0x3C00 in runs that each drawing file owns, and the display
 *   list space split into several lists (0x48B0, 0xA4E0, 0xA580, 0xA918,
 *   0x21410, 0x21478);
 * - a few files still treat segment 2 as a plain Mtx[] (17E10.c, 39050.c,
 *   3E4C0.c, 42240.c, 48D00.c), some with indices past the matrices.
 *
 * The byte offsets some callers still add to &D_803156F8[i * 0x21498] are
 * these fields.
 */
typedef struct FrameBuf {
    /* 0x00000 */ Mtx mtx[0x78];
    /* 0x01E00 */ Vtx vtx[0x1E0];
    /* 0x03C00 */ LookAt lookAt;
    /* 0x03C20 */ u8 unk3C20[0xC90];
    /* 0x048B0 */ Gfx dl[0x397D];
} FrameBuf;
SIZE_CHECK(FrameBuf, 0x21498);

/*
 * The gameplay view of the same buffer (hd.c and the gameplay drawing code:
 * 13A70.c, 20460.c, 37530.c, 4B5E0.c, 4DA80.c, 50670.c).  They name it
 * through segment 2 (D_02000000); hd.c hands it to its drawing functions as
 * a cast of &D_803156F8[i * 0x21498].
 */
typedef struct FrameGame {
    /* 0x00000 */ Mtx unk0[11];       /* [2] projection, [5] its second half, [7] the view */
    /* 0x002C0 */ Mtx unk2C0[60];
    /* 0x011C0 */ Mtx unk11C0[13];
    /* 0x01500 */ Mtx unk1500;
    /* 0x01540 */ Mtx unk1540;
    /* 0x01580 */ Vtx unk1580[4];
    /* 0x015C0 */ Vtx unk15C0[48];
    /* 0x018C0 */ Vtx unk18C0[4];
    /* 0x01900 */ Vtx unk1900[0x230];  /* 20460.c's from 0, 37530.c's from [0x70], 13A70.c's from [0x200] */
    /* 0x03C00 */ LookAt unk3C00;
    /* 0x03C20 */ Gfx unk3C20[0x192];   /* 37530.c's display lists from [2] */
    /* 0x048B0 */ Gfx unk48B0[0xB5E];
    /* 0x0A3A0 */ u8 unkA3A0[0x140];
    /* 0x0A4E0 */ Gfx unkA4E0[20];
    /* 0x0A580 */ Gfx unkA580[115];
    /* 0x0A918 */ Gfx unkA918[0x2D5F];
    /* 0x21410 */ Gfx unk21410[13];
    /* 0x21478 */ Gfx unk21478[4];
} FrameGame;
SIZE_CHECK(FrameGame, 0x21498);

/*
 * D_803156F8's element: the buffer in whichever of its two views the code
 * using it has.  The functions that take the buffer as a parameter take a
 * Frame *, and look at it as the view they know.
 */
typedef union Frame {
    FrameBuf buf;
    FrameGame game;
} Frame;
SIZE_CHECK(Frame, 0x21498);

extern Frame D_803156F8[];

/*
 * 0x1040-byte records that hd_code 13A70.c fills and draws (hd.c aligns the
 * array to 64 bytes in its .bss, at D_80358088): 0x1000 bytes of image
 * first.  D_803643C8 is the first, D_803643CC the next free one.
 */
typedef struct UnkStruct_803643C8 {
    /* 0x0000 */ u8 unk0[0x1000];
    /* 0x1000 */ f32 unk1000;
    /* 0x1004 */ s32 unk1004;
    /* 0x1008 */ s32 unk1008;
    /* 0x100C */ s32 unk100C;
    /* 0x1010 */ s32 unk1010;
    /* 0x1014 */ s32 unk1014;
    /* 0x1018 */ s16 unk1018;
    /* 0x101A */ s16 unk101A;
    /* 0x101C */ s16 unk101C;
    /* 0x101E */ s16 unk101E;
    /* 0x1020 */ s16 unk1020;
    /* 0x1022 */ u8 unk1022;
    /* 0x1023 */ u8 unk1023;
    /* 0x1024 */ u8 unk1024[0x1C];
} UnkStruct_803643C8;
SIZE_CHECK(UnkStruct_803643C8, 0x1040);

extern UnkStruct_803643C8 *D_803643C8;
extern UnkStruct_803643C8 *D_803643CC;

#endif
