/*
 * jp's fuzz driver for the check build (fuzz.h, PORT_ENGINE_FUZZ): the IDO
 * functions jp_*.c make native, on varied game memory, each call checked
 * against its translation.  The attract mode and the autostart runs reach
 * only some of their paths (the medal and promotion screens, the races'
 * counters, the Pak's menus are none of them).  Functions whose C callees
 * can't run twice (check_repeatable.txt: the Pak thread's messages, the
 * music's start) aren't here.
 */
#if defined(VERSION_JP) && defined(PORT_ENGINE_CHECK)
#include "fuzz.h"
#include "game/frame.h"
#include "game/frontend.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"
#include "game/sched.h"
#include "game/yoshi.h"

/* (the C-side check wrappers, not engine_check_names.h's native ones) */
#undef func_801F7410
#undef func_8025B498
#undef func_802979E0
#undef func_801EC770
#undef func_801ED800
#undef func_8026BCE0
#undef func_80262840
#undef func_80263358
#undef func_802633E0
#undef func_802639B4
#undef func_80259EC4
#undef func_801F9258
#undef func_801E8EB8
#undef func_801EE800

u16 *func_801F7410(u8 *);
s32 func_8025B498(s16, u16, char *, u16 *);
void func_802979E0(u8);
Gfx *func_801EC770(Gfx *, s32, s32 *);
Gfx *func_801ED800(Gfx *, FrameBuf *, u8, s32 *);
Gfx *func_8026BCE0(Gfx *, FrameBuf *, s32 *);
void func_80262840(void);
void func_80263358(void);
void func_802633E0(void);
Gfx *func_802639B4(Gfx *, void *, s32);
void func_80259EC4(s32, u8 *, u16 *, u8, s32, f32, s32, f32, s32, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8,
                   u8, u8, u8, u8, u8);
Gfx *func_801F9258(Gfx *, u8 *, s32 *);
void func_801E8EB8(u8, u8);
u8 func_801EE800(u8 *, u8, u8);

extern FrameBuf D_803156F8[2];
extern u8 D_802189C0[][0x11];
extern char D_80218740[0x10][0x50];
extern u16 D_80219F00_jp[0x20];
extern u8 D_80364B80[][0x100];
extern u8 D_80364B81[][0x100];
extern u8 D_8039CAB6;
extern u8 D_80215902[];
extern s16 D_80215910[];
extern u8 D_80215914;
extern s32 D_80215960, D_80215964, D_80215970, D_80215978;
extern f32 D_80215968, D_8021596C;
extern s16 D_80215974, D_80215976, D_802159B0;
extern s8 D_802084C0;
extern s32 D_802FA268;

static void fuzz_string(FuzzRng *r, u8 *s, s32 max) {
    s32 n = fuzz_int(r, 0, max), i;
    for (i = 0; i < n; i++)
        s[i] = (u8)fuzz_int(r, 1, 255);
    s[n] = 0;
}

/* u16 text: glyphs, now and then a space (0x1000), 0x0FFE at the end */
static void fuzz_text16(FuzzRng *r, u16 *t, s32 max) {
    s32 n = fuzz_int(r, 0, max), i;
    for (i = 0; i < n; i++)
        t[i] = fuzz_int(r, 0, 7) == 0 ? 0x1000 : (u16)fuzz_int(r, 0, 0x1FF);
    t[n] = 0xFFE;
}

/* E7B0: a Pak file's name into u16 text */
static void fuzz_801F7410(FuzzRng *r) {
    fuzz_string(r, D_802189C0[0], 0x10);
    func_801F7410(D_802189C0[0]);
}

/* 168B0: a line's start, centred */
static void fuzz_8025B498(FuzzRng *r) {
    u16 *t = NULL;
    fuzz_string(r, (u8 *)D_80218740[0], 0x30);
    switch (fuzz_int(r, 0, 5)) {
        case 0:
        case 1:
            break;
        case 2:
            t = D_80219F00_jp;
            t[0] = 0xFFF;
            break;
        default:
            t = D_80219F00_jp;
            fuzz_text16(r, t, 0x1E);
            break;
    }
    func_8025B498(fuzz_int(r, -400, 400), fuzz_int(r, 0, 40), D_80218740[0], t);
}

/* 53220: the window of 21's entries */
static void fuzz_802979E0(FuzzRng *r) {
    D_80364B80[D_80364AE8][0] = (u8)fuzz_u32(r);
    D_8039CAB6 = fuzz_int(r, 0, 5);
    func_802979E0(FUZZ_PICK(r, 4, 0xA, 0xD, 0x21, 0xE, 0x11, 0, 1, 0x3F));
}

/* 1C40: the medal screen's cups and the time to beat */
static void fuzz_801EC770(FuzzRng *r) {
    s32 n = fuzz_int(r, 0, 100);
    D_80364AA8 = FUZZ_PICK(r, 1, 1, 0, 2, 0x40);
    D_80215902[1] = fuzz_int(r, 0, 6);
    D_80364B81[D_80364AE8][0] = fuzz_int(r, 0, 15);
    D_80215910[1] = fuzz_int(r, -300, 300);
    D_80215914 = fuzz_int(r, 0, 0x40);
    D_802E8BDC = fuzz_int(r, 0, 59);
    func_801EC770(D_803156F8[1].dl, (s32)&D_803156F8[1], &n);
}

/* 6790: the promotion screen */
static void fuzz_801ED800(FuzzRng *r) {
    s32 n = fuzz_int(r, 0, 100);
    D_80215960 = fuzz_int(r, 0, 4);
    D_80215964 = fuzz_int(r, 0, 4);
    D_80215970 = fuzz_int(r, -2, 3);
    D_80215974 = fuzz_int(r, -40, 300);
    D_80215976 = fuzz_int(r, -40, 300);
    D_802159B0 = fuzz_int(r, -40, 300);
    D_802084C0 = FUZZ_PICK(r, -1, 1);
    D_8021596C = fuzz_float(r, 0.0f, 3.0f);
    D_80215968 = fuzz_float(r, 0.0f, 380.0f);
    D_80215978 = fuzz_int(r, 0, 0x1E);
    D_80358060 = FUZZ_PICK(r, 0, 1, 2, 50, 200, 290, 3, 100, 89, 90, 91);
    D_80370C28 = (u16)fuzz_u32(r);
    D_80370C2A = (u16)fuzz_u32(r);
    D_802FA268 = fuzz_int(r, 0, 1);
    func_801ED800(D_803156F8[0].dl, &D_803156F8[0], fuzz_int(r, 0, 1), &n);
}

/* 1D990: the level's goal, its timer and the HUD */
extern u8 D_80367B54;
extern u16 D_80367B58[4];
extern s32 D_80367BC0;
extern u32 D_80367BC4;
extern u16 D_80367BC8;
extern YoshiIcon *D_80367BCC;
extern YoshiIcon *D_80367BD0;
extern u8 D_80367BD4, D_80367BD5;
extern s16 D_80367BD6, D_80367BD8;
extern u16 D_80367BF4;
extern u8 D_80367BF8, D_80367BF9, D_80367BFA, D_80367BFB;
extern u16 D_80367BFC;
extern LevelInfo *D_80367C04;
extern u16 *D_80367C0C;
extern s16 D_80367D50;
extern u8 D_80367D52;
extern u8 D_803156F4;
extern u8 D_803643D7;
extern u8 D_80366BC0;
extern u32 D_80366BB8;
extern s32 D_80364A58;
extern YoshiIcon D_802F49F4[0x4B];

static const s32 fuzz_kinds[] = { 0, 1, 2, 3, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x21, 0x41, 0x22, 0x81 };
#define FUZZ_KIND(r) fuzz_kinds[fuzz_u32(r) % (sizeof fuzz_kinds / sizeof fuzz_kinds[0])]

/* the current level, its goal and rectangles varied */
static LevelInfo *fuzz_level(FuzzRng *r) {
    LevelInfo *l = &D_802E8F94[fuzz_int(r, 0, LEVEL_COUNT - 1)];
    s32 k;

    D_80367C04 = l;
    l->goal = FUZZ_PICK(r, 0, 1, 2, 3, 4, 5, 3, 4, 1000, 250000);
    l->medalTimes[3] = fuzz_int(r, 0, 1) ? fuzz_int(r, 0, 0xFFFF) : fuzz_int(r, 0, 6000);
    /* the boxes' rectangle (unk2..unk8): its halves never 0 (the
       original's break 7), now and then -1 */
    l->unk2 = fuzz_int(r, 0, 3000);
    l->unk6 = l->unk2 + FUZZ_PICK(r, -2, -1, -3, 2, 3, 100, 400, 1000, -400);
    l->unk4 = fuzz_int(r, 0, 3000);
    l->unk8 = l->unk4 + FUZZ_PICK(r, -2, -1, -3, 2, 3, 100, 400, 1000, -400);
    /* the finish line (unkA..unk10, s16s in func_8026394C) */
    l->unkA = fuzz_int(r, 0, 3000);
    l->unkE = l->unkA + fuzz_int(r, 1, 500);
    l->unkC = fuzz_int(r, 0, 3000);
    l->unk10 = l->unkC + fuzz_int(r, 1, 500);
    for (k = 0; k < 4; k++)
        l->unk12[k] = fuzz_int(r, 0, 3);
    return l;
}

/* the race's lap state */
static void fuzz_laps(FuzzRng *r) {
    s32 k;

    D_80367B54 = fuzz_int(r, 0, 4);
    for (k = 0; k < 4; k++)
        D_80367B58[k] = fuzz_int(r, 0, 3) == 0 ? (u16)fuzz_u32(r) : fuzz_int(r, 0, 3000);
    D_80367BFB = fuzz_int(r, 0, 5);
}

/* 1D990: the intro's goal text and countdown, a second at a time */
static void fuzz_80262840(FuzzRng *r) {
    u32 sec = fuzz_int(r, 0, 6);

    fuzz_level(r);
    D_80367BC0 = D_803156C4 - sec * 60 - fuzz_int(r, 0, 59);
    D_80367BC4 = fuzz_int(r, 0, 2) ? fuzz_int(r, 0, 6) : sec;
    D_80364AA8 = FUZZ_KIND(r);
    D_802E8BDC = FUZZ_PICK(r, 0x34, 0x32, 0x34, 0x32, 1, 7, 40);
    fuzz_text16(r, D_80219F00_jp, 0x10);
    D_80367C0C = D_80219F00_jp;
    D_803156F4 = fuzz_int(r, 0, 1) ? D_8035805C : (u8)fuzz_int(r, 0, 2);
    func_80262840();
}

/* 1D990: the damage left */
static void fuzz_80263358(FuzzRng *r) {
    LevelInfo *l = fuzz_level(r);

    /* (func_802C1DD0 recounts the damage: goal 0 and 1 reach 80263388) */
    l->goal = fuzz_int(r, 0, 2) ? (u32)FUZZ_PICK(r, 0, 1, 2, 1000, 50000) : (fuzz_int(r, 0, 1) ? (u32)fuzz_int(r, 0, 2000000) : fuzz_u32(r));
    D_8036EA70.ip = fuzz_int(r, 0, 2) ? l->goal + fuzz_int(r, -100000, 100000) : fuzz_u32(r);
    func_80263358();
}

/* 1D990: the race's laps and boxes */
static void fuzz_802633E0(FuzzRng *r) {
    LevelInfo *l = fuzz_level(r);
    s32 x, z;

    fuzz_laps(r);
    if (fuzz_int(r, 0, 1)) {
        x = fuzz_int(r, l->unkA - 20, l->unkE + 20);
        z = fuzz_int(r, l->unkC - 20, l->unk10 + 20);
    } else {
        s32 x0 = l->unk2 < l->unk6 ? l->unk2 : l->unk6, x1 = l->unk2 < l->unk6 ? l->unk6 : l->unk2;
        s32 z0 = l->unk4 < l->unk8 ? l->unk4 : l->unk8, z1 = l->unk4 < l->unk8 ? l->unk8 : l->unk4;
        x = fuzz_int(r, x0 - 50, x1 + 50);
        z = fuzz_int(r, z0 - 50, z1 + 50);
    }
    D_803643E0 = (x << 5) + fuzz_int(r, 0, 31);
    D_803643E8 = (z << 5) + fuzz_int(r, 0, 31);
    D_80367BF8 = FUZZ_PICK(r, 0, 1, 2, 3, 4, 4, 4);
    D_80367BF9 = fuzz_int(r, 0, 3);
    D_80367BFC = fuzz_int(r, 0, 1) ? 0xFFFF : fuzz_int(r, 0, 3000);
    D_80370C28 = (u16)fuzz_u32(r);
    D_802E8BD0 = fuzz_int(r, 0, 1);
    D_80364A58 = D_803156C0 - fuzz_int(r, 0, 20000);
    func_802633E0();
}

/* a HUD icon: a real one, with its frames kept to the icon tables'
   0x40 and its divisors not 0 (the original's break 7) */
static YoshiIcon *fuzz_icon(FuzzRng *r) {
    YoshiIcon *icon;
    s32 k;

    if (fuzz_int(r, 0, 2) == 0)
        return NULL;
    icon = &D_802F49F4[fuzz_int(r, 0, 0x4A)];
    icon->unk1A = fuzz_int(r, 1, 10);
    icon->unk26 = fuzz_int(r, 1, 12);
    for (k = 0; k < 10; k++)
        icon->unk1B[k] = fuzz_int(r, 1, 0x3F);
    return icon;
}

/* 1D990: the HUD */
static void fuzz_802639B4(FuzzRng *r) {
    s32 k = D_8035805C & 1;
    u32 lo = FUZZ_PICK(r, 0, 0x04000000, 0x200, 0x04000200, 0x40, 0x400, 0x440, 0x104, 0x100, 4, 0x2000,
                       0x04000104, 0x04000440);

    fuzz_level(r);
    fuzz_laps(r);
    if (fuzz_int(r, 0, 4) == 0)
        lo = fuzz_u32(r);
    D_80364A90 = (fuzz_int(r, 0, 5) == 0 ? (u64)fuzz_int(r, 1, 0xFF) << 32 : 0) | lo;
    D_8036BB18 = FUZZ_PICK(r, 0x1E, -1, 3, 0x1E);
    D_80367BD6 = fuzz_int(r, 0, 0xFF);
    D_80364AA8 = FUZZ_KIND(r);
    D_80367BF4 = FUZZ_PICK(r, 0, 1, 5, 9, 10, 11, 100, 0);
    D_80315440.frameCount = fuzz_u32(r); /* D_803156C4 */
    /* func_8025E2CC's quiet path: the level's end already announced, no
       window to wait for */
    D_803643D7 = fuzz_int(r, 0, 1);
    D_80366BC0 = 1;
    D_80366BB8 = 0;
    D_8036BB1C = 0;
    D_80367BC8 = fuzz_int(r, 0, 5);
    D_80367D50 = fuzz_int(r, -10, 10);
    D_80367D52 = fuzz_int(r, 1, 5);
    D_80367BCC = fuzz_icon(r);
    D_80367BD0 = fuzz_icon(r);
    D_80367BD4 = fuzz_int(r, 0, 1);
    D_80367BD5 = fuzz_int(r, 0, 1);
    D_80367BD8 = fuzz_int(r, -20, 200);
    func_802639B4(D_803156F8[k].dl, &D_803156F8[k], fuzz_int(r, 0, 100));
}

/* 26570 (yoshi.c): the window renderer, on a window set up from scratch:
   its state, flags and entries, the controller and the stick */
extern u8 D_803643D6, D_803643DB;
extern u8 D_8036BA98[];
extern u32 D_8036BAFC, D_8036BB40;
extern u16 D_8036BB04, D_8036BB14, D_8036BB3C, D_8036BB3E;
extern f32 D_8036BB08, D_8036BB28, D_8036BB2C, D_8036BB34, D_8036BB38;
extern s16 D_8036BB0C, D_8036BB1A, D_8036BB1E;
extern s8 D_8036BB0E;
extern s32 D_8036BB44, D_802F9930;
extern s8 D_80370C11, D_80370C12, D_80370C13, D_80370C14;
extern char D_8036B9A8[0x20];
extern char D_8036B9C8[0x20];
extern Sched D_80315440;

static s8 fuzz_stick(FuzzRng *r) {
    return FUZZ_PICK(r, -80, -31, -30, -29, 0, 29, 30, 31, 80);
}

static u16 fuzz_pad(FuzzRng *r) {
    u16 v = 0;
    s32 k;
    for (k = fuzz_int(r, 0, 3); k > 0; k--)
        v |= FUZZ_PICK(r, 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100);
    return v;
}

static void fuzz_8026BCE0(FuzzRng *r) {
    s32 n = 0, w, k, first, count;
    YoshiWindow *win;
    u16 *t16 = (u16 *)D_8036B9C8;
    FrameBuf *fb = &D_803156F8[fuzz_int(r, 0, 1)];

    w = fuzz_int(r, 0, 3) == 0 ? fuzz_int(r, 0x62, 0x6B) : fuzz_int(r, 0, 0x6B);
    win = &D_802F8BDC[w];
    count = fuzz_int(r, 1, 8);
    first = fuzz_int(r, 1, YOSHI_ENTRIES - 12);
    win->first = first;
    win->count = count;
    win->unk0 = fuzz_int(r, 0, 3) == 0 ? fuzz_int(r, 0, 320) : fuzz_int(r, 100, 300);
    win->unk2 = fuzz_int(r, 3, 240);
    win->unk4 = fuzz_int(r, -100, 100);
    win->unk6 = fuzz_int(r, -100, 100);
    win->unk8 = (fuzz_int(r, 0, 1) ? fuzz_u32(r) : fuzz_u32(r) & fuzz_u32(r)) & ~0x8800;
    if (fuzz_int(r, 0, 1))
        win->unk8 |= 0x4000;
    win->unkC = fuzz_int(r, 0, 1) ? 0 : fuzz_int(r, 1, 5);
    win->unk12 = FUZZ_PICK(r, 0, 0, 0xD0, 0xDE);
    win->unk14 = FUZZ_PICK(r, 0, 0, 0xD0, 0xDE);
    win->unk16 = FUZZ_PICK(r, 0, 0, 0xD0, 0xDE);
    win->unk18 = first + fuzz_int(r, 0, count - 1);
    win->unk1A = fuzz_int(r, 0, 1);

    D_8036B9A8[0] = 'A';
    D_8036B9A8[1] = 'B';
    D_8036B9A8[2] = 0;
    t16[0] = 0xFFF;
    for (k = first; k <= first + count; k++) {
        YoshiEntry *e = &D_802F5804[k];
        u16 f = 0;
        s32 j;
        for (j = fuzz_int(r, 0, 6); j > 0; j--)
            f |= FUZZ_PICK(r, 1, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000);
        if (fuzz_int(r, 0, 2) == 0)
            f |= 0x80;
        if (fuzz_int(r, 0, 3) == 0)
            f |= 0x10;
        if (fuzz_int(r, 0, 2) == 0)
            f |= 0x400;
        e->flags = f;
        e->x = fuzz_int(r, -100, 100);
        e->y = fuzz_int(r, -100, 100);
        e->unk6 = fuzz_int(r, 8, 24);
        e->unk8 = fuzz_int(r, 0, 30);
        e->unk14 = fuzz_int(r, 0, 9);
        e->unk18 = fuzz_int(r, 0, 0x16);
        e->unk19 = fuzz_int(r, 0, 0x16);
        e->unk16 = FUZZ_PICK(r, 0, 0, 0xD0);
        switch (fuzz_int(r, 0, 3)) {
            case 0:
                e->unk10 = NULL;
                break;
            case 1:
                e->unk10 = t16;
                break;
            default:
                break;
        }
        if (e->text == NULL && (e->unk10 == NULL || *e->unk10 == 0xFFF))
            e->text = D_8036B9A8;
        if (fuzz_int(r, 0, 3) == 0)
            D_8036BA98[e->unk14] = 0;
    }

    D_80315440.frameCount = FUZZ_PICK(r, 0, 1000, 5000, 77777, 0x7FFFFFF0, 0x80000010) + fuzz_int(r, 0, 100);
    D_8036BB40 = D_80315440.frameCount - fuzz_int(r, 0, 4);
    D_8036BB0C = fuzz_int(r, -10, 270);
    D_8036BB0E = FUZZ_PICK(r, 1, -1, 2, -2, 3);
    D_80364A90 = (u64)FUZZ_PICK(r, 0, 0, 0, 0x08000000) << 32 |
                 (u32)FUZZ_PICK(r, 0x200, 2, 0x200, 0, 0x10, 0x40000);
    D_803643DB = fuzz_int(r, 0, 1);
    D_803643D6 = fuzz_int(r, 0, 1);
    D_8036BB18 = fuzz_int(r, 0, 2) ? w : -1;
    D_8036BB1A = fuzz_int(r, 0, 1) ? w : -1;
    D_8036BB14 = fuzz_int(r, 0, 1) ? 0 : (u16)(w | (fuzz_u32(r) & 0xE000));
    D_8036BB1C = FUZZ_PICK(r, 1, 2, 2, 4, 8, 2, 0, 3);
    D_8036BB1E = fuzz_int(r, 0, 3) == 0;
    D_8036BB04 = first + fuzz_int(r, -1, count);
    D_80370C28 = fuzz_pad(r);
    D_80370C2A = fuzz_int(r, 0, 1) ? 0 : fuzz_pad(r);
    D_80370C11 = fuzz_stick(r);
    D_80370C12 = fuzz_stick(r);
    D_80370C13 = fuzz_stick(r);
    D_80370C14 = fuzz_stick(r);
    D_8036BB08 = FUZZ_PICK(r, 0, 1) ? 40.0f : 13.333333f;
    switch (fuzz_int(r, 0, 9)) {
        case 0:
            D_8036BB34 = -fuzz_float(r, 0.1f, 2.0f);
            break;
        case 1:
            D_8036BB34 = FUZZ_PICK(r, 1, 2, 3) == 1 ? 3.0e7f : FUZZ_PICK(r, 1, 2) == 1 ? 3.0e8f : 1.0e12f;
            break;
        case 2:
        case 3:
            D_8036BB34 = fuzz_float(r, 0.2f, 1.5f);
            break;
        default:
            D_8036BB34 = 1.0f;
            break;
    }
    D_8036BB38 = fuzz_int(r, 0, 3) == 0 ? fuzz_float(r, 0.0f, 0.01f) : fuzz_float(r, 0.0f, 1.3f);
    D_8036BAFC = D_80315440.frameCount - FUZZ_PICK(r, 0, 1, 10, 30, 60, 200, 400, -5);
    D_8036BB28 = fuzz_float(r, -200.0f, 200.0f);
    D_8036BB2C = fuzz_float(r, -200.0f, 200.0f);
    D_8036BB3C = FUZZ_PICK(r, 0x800, 0x200);
    D_8036BB3E = FUZZ_PICK(r, 0x400, 0x100);
    D_8036BB44 = fuzz_int(r, -2, 9);
    D_802F9930 = FUZZ_PICK(r, 1, -1, 2, -2);
    D_8035805C = fuzz_int(r, 0, 1);
    func_8026BCE0(fb->dl, fb, &n);
}

/* 14B30: the text renderer, char text or u16 text, either way round, near
   and at the end of the quads' buffers */
extern s32 D_80365350, D_802E8C74, D_802E8C78;
extern u16 D_802E8C8C[2], D_802E8C90[2], D_802E8C94[2];

static void fuzz_80259EC4(FuzzRng *r) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'):,$!.-(%?#/ &abcdekm\x7f\"*;<@[`fgz~";
    u8 *s = (u8 *)D_80218740[0];
    u16 *t = D_80219F00_jp;
    s32 n, i, kind, last;
    u8 c[16];

    n = fuzz_int(r, 0, 12);
    for (i = 0; i < n; i++) {
        switch (fuzz_int(r, 0, 9)) {
            case 0:
                s[i] = (u8)fuzz_int(r, 1, 255);
                break;
            case 1:
                s[i] = (u8)D_802E8C8C[0];
                break;
            case 2:
                s[i] = (u8)FUZZ_PICK(r, 0, 1) ? (u8)D_802E8C90[0] : (u8)D_802E8C94[0];
                break;
            default:
                s[i] = chars[fuzz_int(r, 0, sizeof chars - 2)];
                break;
        }
        if (s[i] == 0)
            s[i] = 'A';
    }
    s[n] = 0;
    n = fuzz_int(r, 0, 12);
    for (i = 0; i < n; i++) {
        switch (fuzz_int(r, 0, 9)) {
            case 0:
                t[i] = 0x1001;
                break;
            case 1:
                t[i] = D_802E8C8C[1];
                break;
            case 2:
                t[i] = FUZZ_PICK(r, 0, 1) ? D_802E8C90[1] : D_802E8C94[1];
                break;
            case 3:
                t[i] = (u16)fuzz_int(r, 0x200, 0xFFD);
                break;
            default:
                t[i] = (u16)fuzz_int(r, 0, 0x1FF);
                break;
        }
    }
    t[n] = 0xFFE;
    if (fuzz_int(r, 0, 5) == 0)
        t[0] = 0xFFF;
    kind = fuzz_int(r, 0, 4);
    /* the quads written so far: now and then the last one the buffers
       hold, a one-glyph string then (the asserts' blocks) */
    last = fuzz_int(r, 0, 5) == 0;
    D_802E8C74 = last ? D_80365350 - 1 : fuzz_int(r, 0, D_80365350 - 40);
    D_802E8C78 = D_802E8C74 * 4;
    if (last) {
        s[1] = 0;
        t[1] = 0xFFE;
    }
    for (i = 0; i < 16; i++)
        c[i] = fuzz_int(r, 0, 3) == 0 ? 0 : (u8)fuzz_u32(r);
    if (fuzz_int(r, 0, 3) == 0)
        c[3] = c[7] = c[11] = c[15] = 0;
    if (fuzz_int(r, 0, 3) == 0)
        c[0] = c[1] = c[2] = 0;
    func_80259EC4(0, kind == 0 ? NULL : s, kind <= 1 ? NULL : t, (u8)FUZZ_PICK(r, 0, 1, 1, 2),
                  fuzz_int(r, 0, 1) ? 0 : fuzz_int(r, 1, 320),
                  fuzz_float(r, -50.0f, 330.0f), fuzz_int(r, -20, 260),
                  FUZZ_PICK(r, 0, 1, 2, 3, 4, 5, 6) == 0 ? (f32)FUZZ_PICK(r, -3, -1, 0, 1, 0x7FFFFF, 3000000, 2147484, 4294967) * 1000.0f
                                                         : fuzz_float(r, 4.0f, 40.0f),
                  fuzz_int(r, 4, 40), (u8)fuzz_int(r, 0, 1), c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
                  c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
}

/* 11530: the level screen's display list */
extern u8 D_8021A908;
extern s16 D_8021AB2C;
extern PlayerInfo *D_8021AB30;
extern u16 D_8020E350[];

static void fuzz_801F9258(FuzzRng *r) {
    s32 n = 0, k = D_8035805C & 1, i;
    LevelInfo *l;

    D_8021A908 = fuzz_int(r, 0, LEVEL_COUNT - 1);
    l = &D_802E8F94[D_8021A908];
    if (fuzz_int(r, 0, 2) == 0)
        D_8020D810[D_8021A908].unk8 = NULL;
    l->unk0 = FUZZ_PICK(r, 1, 1, 0, 2, 0x20, 0x81);
    l->unk2C = fuzz_int(r, 0, 1) ? fuzz_u32(r) : fuzz_u32(r) & fuzz_u32(r);
    D_8021AB30 = &D_80364AF0[fuzz_int(r, 0, 3)];
    D_8021AB30->unk10 = fuzz_u32(r);
    for (i = 0; i < 19; i++)
        D_8020E350[i * 2] = fuzz_int(r, 0, 2) ? 1 : 0;
    D_8021AB2C = fuzz_int(r, 0, 255);
    func_801F9258(D_803156F8[k].dl, (u8 *)&D_803156F8[k], &n);
}

/* 1C40: the players' menu's scroller */
extern u8 D_80365060[];
extern struct { char *PTR32 unk0[4]; } D_80208368, D_80208358;
extern u8 D_802E8BF8;

static void fuzz_801E8EB8(FuzzRng *r) {
    u8 p = fuzz_int(r, 0, 4);
    s32 k;

    for (k = 0; k < 4; k++) {
        PlayerInfo *pl = &D_80364AF0[k];
        fuzz_string(r, (u8 *)pl->name, 7);
        pl->unkC = fuzz_int(r, 0, 30);
        pl->unk14 = fuzz_int(r, 0, 3) == 0 ? (s32)fuzz_u32(r) : fuzz_int(r, 0, 99999);
        pl->gameState = fuzz_int(r, 0, 15);
        D_80365060[k] = FUZZ_PICK(r, 1, 1, 2, 0);
    }
    D_80364AEA = fuzz_int(r, 0, 4);
    D_80364A90 = (u64)FUZZ_PICK(r, 0, 0, 0x01000000, 0x10) << 32 | (u32)FUZZ_PICK(r, 0, 0, 0x8000, 0x10000000, 0x200, 1);
    D_80364A98 = fuzz_int(r, 0, 3) == 0 ? 0x0001000000000000ull
                                        : (u64)FUZZ_PICK(r, 0, 0, 0x02000000, 0x400, 0x10000) << 32 |
                                              (u32)FUZZ_PICK(r, 0, 1, 2, 0x40);
    D_802E8BF8 = fuzz_int(r, 0, 1);
    /* (never NULL in the game: the char text path) */
    switch (fuzz_int(r, 0, 5)) {
        case 0:
            D_80208368.unk0[fuzz_int(r, 0, 3)] = NULL;
            break;
        case 1:
            D_80208358.unk0[0] = NULL;
            break;
        case 2: {
            /* a scroll text too long (its assert) */
            u16 *t = (u16 *)D_80218740[0];
            s32 n = fuzz_int(r, 0xF0, 0x110), j;
            for (j = 0; j < n; j++)
                t[j] = 0x21;
            t[n] = 0xFFE;
            D_80208368.unk0[fuzz_int(r, 0, 3)] = (char *)t;
            break;
        }
    }
    func_801E8EB8(p, (u8)fuzz_int(r, 0, 1));
}

/* 7800: a level's results into the player's record */
extern u16 D_8036EB90;
extern u8 D_8036EB92, D_8036EB93;
extern u8 D_80364A60;

static LevelStats fuzz_stats(FuzzRng *r) {
    LevelStats s;

    s.ip = fuzz_int(r, 0, 100000);
    s.tc = fuzz_int(r, 0, 1) ? fuzz_int(r, 0, 20000) : fuzz_u32(r);
    s.bd = fuzz_int(r, 0, 60);
    s.cr = fuzz_int(r, 0, 60);
    s.coin = fuzz_int(r, 0, 5);
    s.bdn = fuzz_int(r, 0, 15);
    s.rt = fuzz_int(r, 0, 60);
    s.padE[0] = s.padE[1] = 0;
    return s;
}

static void fuzz_801EE800(FuzzRng *r) {
    PlayerInfo *pl;
    LevelInfo *l;

    D_80364AE8 = fuzz_int(r, 0, 3);
    pl = &D_80364AF0[D_80364AE8];
    D_802E8BDC = fuzz_int(r, 0, 3) == 0 ? FUZZ_PICK(r, 0x26, 0x2F, 0x31) : fuzz_int(r, 0, LEVEL_COUNT - 1);
    l = &D_802E8F94[D_802E8BDC];
    l->unk0 = FUZZ_PICK(r, 1, 1, 1, 0, 2, 0x20, 0x80, 0x81);
    D_8036EA70 = fuzz_stats(r);
    D_8036EA60 = fuzz_stats(r);
    D_8036EB92 = FUZZ_PICK(r, 0, 10, 40, 60);
    D_8036EB93 = FUZZ_PICK(r, 0, 10, 40, 60);
    D_8036EB90 = FUZZ_PICK(r, 0, 10, 40, 60);
    if (fuzz_int(r, 0, 1)) {
        D_8036EA70.bd = D_8036EB92;
        D_8036EA70.cr = D_8036EB93 - fuzz_int(r, 0, 1) * fuzz_int(r, 0, D_8036EB93 / 2);
        D_8036EA70.rt = D_8036EB90 - fuzz_int(r, 0, 1) * fuzz_int(r, 0, D_8036EB90 / 3);
    }
    D_803643D5 = fuzz_int(r, 0, 1);
    pl->units = fuzz_int(r, 0, 2) == 0 ? FUZZ_PICK(r, 354, 351, 359, 360, 355) : fuzz_int(r, 0, 360);
    pl->unkC = fuzz_int(r, 0, 1) ? pl->units / 12 : fuzz_int(r, 0, 30);
    func_801EE800(&D_80364A60, (u8)fuzz_int(r, 0, 1), (u8)fuzz_int(r, 0, 1));
}

static void (*const fuzzers[])(FuzzRng *) = {
    fuzz_801F7410, fuzz_8025B498, fuzz_802979E0, fuzz_801EC770, fuzz_801ED800,
    fuzz_80262840, fuzz_80263358, fuzz_802633E0, fuzz_802639B4,
    fuzz_8026BCE0, fuzz_80259EC4, fuzz_801F9258, fuzz_801E8EB8, fuzz_801EE800,
};

void engine_fuzz(unsigned trials, unsigned seed) {
    FuzzRng r;
    unsigned t, k;

    r.s = seed * 2654435761u | 1;
    for (t = 0; t < trials; t++)
        for (k = 0; k < sizeof fuzzers / sizeof fuzzers[0]; k++) {
            engine_fuzz_reset();
            fuzzers[k](&r);
        }
}
#endif
