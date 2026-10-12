/*
 * The game's structures the host reads field by field, spelled the way the
 * game's headers spell them (blastcorps/include/game, port/include/sdk/PR),
 * in this build's own layout: the same compiler and ABI as the game's C, so
 * a pointer field is as wide as the game's and the fields after it are
 * where the game has them.  port/src/layout.c checks every size and offset
 * against the game's own at compile time.  The host doesn't include the
 * game's headers; a structure it reads only through a few fields (Vehicle,
 * Building, ...) is read through port.h's host_layout instead.
 */
#ifndef HOST_GAME_TYPES_H
#define HOST_GAME_TYPES_H

#include <stdint.h>

/* ---- what the display lists and the audio command lists hold (gbi.h, abi.h) ---- */

/* a Gfx or an Acmd: two words, each as wide as a pointer (an address is one) */
typedef struct HostGfx {
    uintptr_t w0, w1;
} HostGfx;

/* a Vtx (its Vtx_t, and Vtx_tn's normal in the colour's place) */
typedef struct HostVtx {
    int16_t ob[3];              /* position */
    uint16_t flag;
    int16_t tc[2];              /* texture coordinates, s10.5 */
    union {
        uint8_t cn[4];          /* colour and alpha */
        struct {
            int8_t n[3];        /* the normal, for lighting */
            uint8_t a;          /* alpha */
        };
    };
} HostVtx;

/* a Vp: scale and translation, x y z and a pad, with 2 bits of fraction */
typedef struct HostViewport {
    int16_t vscale[4];
    int16_t vtrans[4];
} HostViewport;

/* a Light_t (and a LookAt's lights): the colour twice, and the direction */
typedef struct HostLight {
    uint8_t col[3];
    char pad1;
    uint8_t colc[3];
    char pad2;
    int8_t dir[3];
    char pad3;
} HostLight;

/* an Mtx: the integer halves of the 16 elements as s16 pairs in 8 words, then
   their fractions in 8 more (port/src/gu.c); element k's half is word k / 2's
   high half for even k, its low half for odd */
typedef struct HostMtx {
    int32_t m[4][4];
} HostMtx;
/* element k (0..15) of the integer part, 16 + k of the fraction */
static inline uint16_t host_mtx_half(const HostMtx *m, int k) {
    uint32_t w = (uint32_t)m->m[(k >> 1) >> 2][(k >> 1) & 3];
    return (uint16_t)(w >> ((k & 1) ? 0 : 16));
}

/* ---- the game's own (game/yoshi.h, level.h, player.h) ---- */

/* a window's entries, and the windows themselves */
typedef struct HostYoshiEntry {
    uint16_t flags;
    int16_t x, y;
    uint16_t w, h;
    uint8_t padA[2];
    const char *text;
#ifdef VERSION_EU
    const char *text2, *text3;
#endif
    uint16_t *unk10;
    uint8_t icon;               /* the YoshiIcon it shows */
    uint8_t pad15;
    uint16_t unk16;
    uint8_t unk18, unk19, unk1A;
} HostYoshiEntry;

typedef struct HostYoshiWindow {
    uint16_t w, h;
    int16_t x, y;
    int32_t flags;
    uint16_t unkC;
    uint16_t first, count;      /* the window's entries in the table */
    uint16_t unk12, unk14, unk16;
    uint16_t selected;          /* an index into the table */
    uint8_t unk1A, pad1B;
} HostYoshiWindow;

typedef struct HostYoshiIcon {
    int16_t x, y;
    uint8_t rest[0x2C];
} HostYoshiIcon;

typedef struct HostLevelInfo {
    uint8_t flags;              /* unk0 */
    uint8_t gameState;
    uint8_t rest[0x42];
} HostLevelInfo;

/* the world map's levels: D_8020D810 (UnkStruct_8020D810) */
typedef struct HostMapLevel {
    uint8_t unk0[4];
    const char *name;
#ifdef VERSION_EU
    const char *name2, *name3;
#endif
    uint16_t *unk8;
    uint8_t unkC[4];
    float unk10, unk14;
    int8_t next_b[4];           /* unk18: level numbers, -1 terminated */
    int8_t next_a[8];           /* unk1C */
    float x, y, z;              /* unk24..unk2C: the level's point on the globe */
} HostMapLevel;

typedef struct HostPlayerInfo {
    char name[8];
    uint8_t levelno, unk9;
    uint16_t units;
    uint8_t unkC, padD[3];
    uint32_t unk10;
    int32_t unk14;
    uint8_t medal[60];
    uint8_t routes[60];         /* unk54: per level, bit flags */
    uint8_t unk90;
    uint8_t gameState;
    uint8_t unk92[60];
    uint8_t hintShown[0x20];
    uint8_t unkEE, padEF;
    int32_t unkF0;              /* bit flags, 1 << vehicle type: "speed on 3D stick" */
    uint8_t unkF4[8];
    uint8_t checksum[4];
} HostPlayerInfo;

#endif
