/*
 * The game's variables the host reads or writes, declared with the types the
 * game's C gives them (the names are us.v11's, docs/PORT.md has what they
 * are).  The host doesn't include the game's headers, so a structure is an
 * array of bytes here, its fields found by port.h's host_layout (the N64
 * side's own sizeof and offsetof, port/src/layout.c): GAME_FIELD reads one.
 * A pointer variable is a pointer, followed as one.
 */
#ifndef HOST_GAME_VARS_H
#define HOST_GAME_VARS_H

#include <stdint.h>

#include "port.h"
#include "host_game_types.h"

/* the field of type T at byte offset `off` (a host_layout entry) of the game structure at `base` */
#define GAME_FIELD(T, base, off) (*(T *)((uint8_t *)(base) + (off)))

/* ---- the scheduler (game/sched.h's Sched) ---- */
extern uint8_t D_80315440[];
/* Sched.unk280: retraces in the level ("TIME IN LEVEL") */
#define SCHED_LEVEL_TIME GAME_FIELD(uint32_t, D_80315440, host_layout(PORT_LAYOUT_SCHED_UNK280))
/* Sched.frameCount: retraces (the clock) */
#define SCHED_FRAMECOUNT GAME_FIELD(uint32_t, D_80315440, host_layout(PORT_LAYOUT_SCHED_FRAMECOUNT))

/* ---- the game's state (digest.c, and what the host reads of it) ---- */
extern uint64_t D_80364A90;             /* game_mode */
extern int32_t D_802E8BDC;              /* the current level */
extern int32_t D_80358064;              /* the game's frame count, from 0 in each mode */
extern uint32_t D_80358060;             /* a frame count (internal: TNT fuses read it) */
extern uint32_t D_8036B968;             /* the random number generator's state */
extern HostPlayerInfo D_80364AF0[4];
extern uint8_t D_80364AE8;              /* the current player's slot */
extern uint8_t D_80364456;              /* the player's vehicle type */
extern int32_t D_803643E0, D_803643E4, D_803643E8;      /* the player's position (<< 5) */
extern uint8_t D_803643D9, D_803643DA;  /* lost, won */
extern uint8_t D_8036EA70[];            /* LevelStats */
extern uint8_t D_8036EB92, D_8036EB93;
extern uint16_t D_8036EB90;             /* the RDUs' total */
extern uint32_t D_803649F0;             /* the damage so far */
extern int16_t D_803F8B72, D_803EDC00;  /* the Ballista's missiles, the Sideswipe's hydraulics */
extern uint8_t D_80364460[];            /* Vehicle[12] */
extern uint8_t *D_803649D0;             /* Vehicle *: past the level's last */
extern uint8_t D_803F4030[];            /* Building[] */
extern uint8_t *D_803F7654;             /* Building *: past the last */
extern uint8_t *D_8036BED8;             /* Rdu * */
extern uint8_t D_8039B070[];            /* TntCrate[] */
extern int32_t D_8039B610;
extern uint8_t D_8039C550[];            /* Block[] */
extern int32_t D_8039C710;
extern uint8_t D_8039AF00[];            /* AmmoBox[] */
extern int32_t D_8039B068;
/* the vehicle modules' VehicleStates, and their positions (x, y, z: vehicle.h's table) */
extern uint8_t D_803ED760[], D_803EDB40[], D_803EDF10[], D_803EE2E0[], D_803EE6C0[], D_803EEA90[], D_803EEE70[], D_803EF630[], D_803EFA20[], D_803EFDF0[], D_803F7B50[], D_803F8AA0[], D_803F8E80[], D_803F9250[], D_803FC500[], D_803FC8D0[], D_803FCCA0[];
extern uint8_t D_803F8550[];            /* VehicleState[3] (the barges) */
extern int32_t D_803F8748[9];           /* and theirs */
extern int32_t D_803ED808, D_803ED80C, D_803ED810, D_803EDBE8, D_803EDBEC, D_803EDBF0, D_803EDFB8, D_803EDFBC,
    D_803EDFC0, D_803EE38C, D_803EE390, D_803EE394, D_803EE768, D_803EE76C, D_803EE770, D_803EEB38, D_803EEB3C,
    D_803EEB40, D_803EEF18, D_803EEF1C, D_803EEF20, D_803EF6DC, D_803EF6E0, D_803EF6E4, D_803EFAC8, D_803EFACC,
    D_803EFAD0, D_803EFE98, D_803EFE9C, D_803EFEA0, D_803F7BF8, D_803F7BFC, D_803F7C00, D_803F8B48, D_803F8B4C,
    D_803F8B50, D_803F8F28, D_803F8F2C, D_803F8F30, D_803F92F8, D_803F92FC, D_803F9300, D_803FC5A8, D_803FC5AC,
    D_803FC5B0, D_803FC978, D_803FC97C, D_803FC980, D_803FCD48, D_803FCD4C, D_803FCD50;

/* ---- the save and the mode switch (replay.c) ---- */
extern uint16_t D_80364EF0[4][16], D_80364F70[0x78];    /* the best times */
extern uint64_t D_80364A98;             /* the next mode */
extern uint64_t D_80364AA0, D_8036C778;
extern uint8_t D_8036C784;
extern uint8_t D_80370BF8[];            /* OSMesgQueue: the SI's event queue (45BB0.c) */

/* ---- the name entry (video.c) ---- */
extern int16_t D_8036BB18, D_8036BB1C;  /* the menu's screen, and its state */
extern int16_t D_802154B2, D_802154B4, D_802154BE;      /* the wheel's angle, where it eases to, the character in flight */
extern uint8_t D_802154BC, D_80215924;  /* the name's length, its limit */
extern uint8_t D_802082FC[], D_80208314[];      /* the wheel's characters (func_801EAA7C's tables) */
extern uint32_t D_80208350[2];          /* how many are in the first */
extern float D_80215944, D_80215948, D_8021594C, D_80215950;    /* the character in flight: where it is, where it goes */
extern float D_80215440, D_80215444, D_80215448, D_8021544C;    /* and its size */

/* ---- the HUD font's glyph cache (hdtext.c; font.c's) ---- */
extern uint8_t D_8039CAF0[];            /* 0x50 slots of a 32x32 I4 texture */
extern uint16_t D_803653B0[0x54];       /* the texture table index in each (0: free) */

/* ---- the renderer's (gfx.c) ---- */
extern void *D_80365348[2];             /* Vtx *[2]: the HUD text's vertices (14B30.c), a frame's each */
extern int32_t D_80365350;              /* ... characters each */
extern uint16_t *D_80215A70[3];         /* the world map's chopper: its picture, three frames of its rotors */

/* ---- the keyboard, the mouse and touch (ui.c) ---- */
extern uint8_t D_802E8BD0;              /* a level's window is up */
extern int32_t D_8036BB30;              /* a list's scroll */
extern uint16_t D_8036BB3C, D_8036BB3E; /* the window's previous and next */
extern HostYoshiWindow D_802F8BDC[];
extern HostYoshiEntry D_8020C070[], D_802F5804[];
extern HostYoshiIcon D_802F49F4[];
extern HostLevelInfo D_802E8F94[];
extern HostMapLevel D_8020D810[];
extern uint8_t D_803156F8[];            /* Frame[2] */
extern HostMtx D_80217B70[];         /* Mtx[7][4], as one array */
extern int32_t D_80217B6C;              /* the world map's view: 3 is the globe */
extern uint8_t D_8035805C;              /* the frame being built */
extern uint16_t D_8035807C;
extern uint8_t D_8021A905;              /* the level selected */
extern int8_t D_8021A907;               /* ... and the stick's next */
extern float D_8021A90C, D_8021A910, D_8021A914;        /* the camera's point on the globe */
extern float D_8021A91C, D_8021A920;    /* its longitude and latitude */
extern uint16_t D_8021A924;
extern int16_t D_8021AB2C;

#endif
