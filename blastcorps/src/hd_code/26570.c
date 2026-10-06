#include "common.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/sched.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

/* Element type of the arrays D_8036BB10 points at. */



extern f32 D_80364414;
extern u16 D_802E8C8C[];
extern u16 D_802E8C90[];
extern u16 D_802E8C94[];
extern u16 D_802E8C98[];
extern u16 D_802E8C9C[];
extern u16 D_803C30A8[];
extern s32 D_803F7684;
extern u8 D_802F499A[];
/* eu's timings are PAL's (50 Hz); FRAMES_F is a float that is written out for each. */
#ifdef VERSION_EU
#define REFRESH_RATE 50
#define FRAMES_F(ntsc, pal) (pal)
#else
#define REFRESH_RATE 60
#define FRAMES_F(ntsc, pal) (ntsc)
#endif
#define FRAMES(n) ((n) * REFRESH_RATE / 60)


/*
 * YoshiWindow and dialogue text, reached only through the pointer tables in this
 * file's .data (still asm), so they are defined here to keep their place at
 * the start of the .rodata.
 */

/* .bss, 0x8036B980-0x8036BBB0 (tools/bss_c.py) */
char D_8036B980[0x28];
char D_8036B9A8[0x20];
char D_8036B9C8[0x20];
char D_8036B9E8[0x20];
char D_8036BA08[0x20];
char D_8036BA28[0x20];
u8 D_8036BA48[0x50];
#ifdef TARGET_PC
/* The icons' frames (D_8036BA98, by YoshiIcon index: up to 75) run over the
   hints' flags after them (D_8036BAA2, by hint), as on the N64: one object
   here, so that no build's compiler takes an index past 0xA for impossible */
u8 D_8036BA98[0x64];
#define D_8036BAA2 (D_8036BA98 + 0xA)
#define D_8036BAE8 (D_8036BA98 + 0x50)
#else
u8 D_8036BA98[0xa];
u8 D_8036BAA2[3];
u8 D_8036BAA5[1];
u8 D_8036BAA6[2];
u8 D_8036BAA8[0x40];
u8 D_8036BAE8[0x14];
#endif
u32 D_8036BAFC;
u32 D_8036BB00;
u16 D_8036BB04;
u16 D_8036BB06;
f32 D_8036BB08;
s16 D_8036BB0C;
s8 D_8036BB0E;
YoshiEntry *D_8036BB10;
u16 D_8036BB14;
u16 D_8036BB16;
s16 D_8036BB18;
s16 D_8036BB1A;
s16 D_8036BB1C;
s16 D_8036BB1E;
s16 D_8036BB20;
YoshiEntry *D_8036BB24;
f32 D_8036BB28;
f32 D_8036BB2C;
s32 D_8036BB30;
f32 D_8036BB34;
f32 D_8036BB38;
u16 D_8036BB3C;
u16 D_8036BB3E;
u32 D_8036BB40;
s32 D_8036BB44;
u16 D_8036BB48[0x34];         /* func_8026F004 copies a window's whole text here */

extern u16 D_80301098[];
extern u16 D_803010A0[];
extern u16 D_803010A8[];
extern u16 D_803010BC[];
extern u16 D_803010C0[];
extern u16 D_80301488[];
extern u16 D_80301498[];
extern u16 D_803014B0[];
extern u16 D_803014C4[];
extern u16 D_803014DC[];
extern u16 D_803014FC[];
extern u16 D_80301518[];
extern u16 D_80301530[];
extern u16 D_80301534[];
extern u16 D_80301538[];
extern u16 D_8030153C[];
extern u16 D_80301540[];
extern u16 D_80301558[];
extern u16 D_80301570[];
extern u16 D_80301588[];
extern u16 D_803015A4[];
extern u16 D_803015A8[];
extern u16 D_803015AC[];
extern u16 D_803015B0[];
extern u16 D_803015C8[];
extern u16 D_803015E4[];
extern u16 D_80301600[];
extern u16 D_80301620[];
extern u16 D_80301634[];
extern u16 D_80301638[];
extern u16 D_8030163C[];
extern u16 D_80301640[];
extern u16 D_8030165C[];
extern u16 D_80301674[];
extern u16 D_80301694[];
extern u16 D_803016AC[];
extern u16 D_803016C4[];
extern u16 D_803016C8[];
extern u16 D_803016CC[];
extern u16 D_803016D0[];
extern u16 D_803016E4[];
extern u16 D_80301700[];
extern u16 D_8030171C[];
extern u16 D_80301720[];
extern u16 D_80301724[];
extern u16 D_80301728[];
extern u16 D_80301744[];
extern u16 D_80301760[];
extern u16 D_8030177C[];
extern u16 D_80301780[];
extern u16 D_80301784[];
extern u16 D_80301788[];
extern u16 D_8030178C[];
extern u16 D_80301790[];
extern u16 D_80301B98[];
extern u16 D_80301BA0[];
extern u16 D_80301BA8[];
extern u16 D_80301BB4[];
extern u16 D_80301BC0[];
extern u16 D_80301BCC[];
extern u16 D_80301BD4[];
extern u16 D_80301BE0[];
extern u16 D_80301BF4[];
extern u16 D_80301C04[];
extern u16 D_80301C14[];
extern u16 D_80301C24[];
extern u16 D_80301C30[];
extern u16 D_80301C40[];
extern u16 D_80301C50[];
extern u16 D_80301C64[];
extern u16 D_80301C70[];
extern u16 D_80301C78[];
extern u16 D_80301C8C[];
extern u16 D_80301C9C[];
extern u16 D_80301CB0[];
extern u16 D_80301CC8[];
extern u16 D_80301CE8[];
extern u16 D_80301CEC[];
extern u16 D_80301CF0[];
extern u16 D_80301CF4[];
extern u16 D_80301D08[];
extern u16 D_80301D24[];
extern u16 D_80301D38[];
extern u16 D_80301D3C[];
extern u16 D_80301D40[];
extern u16 D_80301D44[];
extern u16 D_80301D48[];
extern u16 D_80301D64[];
extern u16 D_80301D7C[];
extern u16 D_80301D8C[];
extern u16 D_80301DA4[];
extern u16 D_80301DC0[];
extern u16 D_80301DC4[];
extern u16 D_80301DE0[];
extern u16 D_80301DF4[];
extern u16 D_80301E0C[];
extern u16 D_80301E20[];
extern u16 D_80301E38[];
extern u16 D_80301E3C[];
extern u16 D_80301E40[];
extern u16 D_80301E44[];
extern u16 D_80301E48[];
extern u16 D_80301E60[];
extern u16 D_80301E7C[];
extern u16 D_80301E94[];
extern u16 D_80301E98[];
extern u16 D_80301E9C[];
extern u16 D_80301EA0[];
extern u16 D_80301EA4[];
extern u16 D_80301EC0[];
extern u16 D_80301ED8[];
extern u16 D_80301EF0[];
extern u16 D_80301EF4[];
extern u16 D_80301EF8[];
extern u16 D_80301EFC[];
extern u16 D_80301F00[];
extern u16 D_80301F14[];
extern u16 D_80301F28[];
extern u16 D_80301F38[];
extern u16 D_80301F3C[];
extern u16 D_80301F40[];
extern u16 D_80301F54[];
extern u16 D_80301F6C[];
extern u16 D_80301F8C[];
extern u16 D_80301FA8[];
extern u16 D_80301FAC[];
extern u16 D_80301FB0[];
extern u16 D_80301FD0[];
extern u16 D_80301FE8[];
extern u16 D_80302000[];
extern u16 D_80302004[];
extern u16 D_80302008[];
extern u16 D_8030200C[];
extern u16 D_8030202C[];
extern u16 D_8030204C[];
extern u16 D_80302068[];
extern u16 D_80302088[];
extern u16 D_8030208C[];
extern u16 D_80302090[];
extern u16 D_803020AC[];
extern u16 D_803020CC[];
extern u16 D_803020E4[];
extern u16 D_803020E8[];
extern u16 D_803020EC[];
extern u16 D_80302104[];
extern u16 D_80302124[];
extern u16 D_80302140[];
extern u16 D_8030215C[];
extern u16 D_80302174[];
extern u16 D_80302178[];
extern u16 D_8030217C[];
extern u16 D_80302194[];
extern u16 D_803021B0[];
extern u16 D_803021CC[];
extern u16 D_803021EC[];
extern u16 D_803021F0[];
extern u16 D_8030220C[];
extern u16 D_8030222C[];
extern u16 D_8030224C[];
extern u16 D_80302264[];
extern u16 D_80302280[];
extern u16 D_80302284[];
extern u16 D_80302288[];
extern u16 D_803022A8[];
extern u16 D_803022C8[];
extern u16 D_803022E0[];
extern u16 D_803022F4[];
extern u16 D_803022F8[];
extern u16 D_803022FC[];
extern u16 D_80302310[];
extern u16 D_80302328[];
extern u16 D_80302340[];
extern u16 D_8030235C[];
extern u16 D_80302360[];
extern u16 D_80302364[];
extern u16 D_80302380[];
extern u16 D_803023A0[];
extern u16 D_803023C0[];
extern u16 D_803023DC[];
extern u16 D_803023E0[];
extern u16 D_803023E4[];
extern u16 D_803023FC[];
extern u16 D_80302410[];
extern u16 D_80302424[];
extern u16 D_8030243C[];
extern u16 D_80302440[];
extern u16 D_80302460[];
extern u16 D_8030247C[];
extern u16 D_80302494[];
extern u16 D_803024AC[];
extern u16 D_803024CC[];
extern u16 D_803024D0[];
extern u16 D_803024F0[];
extern u16 D_80302504[];
extern u16 D_8030251C[];
extern u16 D_80302534[];
extern u16 D_80302538[];
extern u16 D_80302550[];
extern u16 D_80302554[];
extern u16 D_8030256C[];
extern u16 D_80302584[];
extern u16 D_80302588[];
extern u16 D_8030258C[];
extern u16 D_803025B8[];
extern u16 D_803025DC[];
extern u16 D_80302600[];
extern u16 D_8030262C[];
extern u16 D_80302650[];
extern u16 D_8030268C[];
extern u16 D_80302698[];
extern u16 D_803026C0[];
extern u16 D_803026CC[];
extern u16 D_803026F8[];
extern u16 D_80302704[];
extern u16 D_80302734[];
extern u16 D_80302748[];
extern u16 D_80302774[];
extern u16 D_80302790[];
extern u16 D_803027AC[];
extern u16 D_803027C4[];
extern u16 D_803027E0[];
extern u16 D_80302800[];
extern u16 D_80302820[];
extern u16 D_80302840[];
extern u16 D_8030285C[];
extern u16 D_80302878[];
extern u16 D_8030288C[];
extern u16 D_803028AC[];
extern u16 D_803028C8[];
extern u16 D_803028E8[];
extern u16 D_80302908[];
extern u16 D_80302918[];
extern u16 D_80302930[];
extern u16 D_80302934[];
extern u16 D_80302940[];
extern u16 D_80302954[];
extern u16 D_80302968[];
extern u16 D_80302974[];
extern u16 D_80302978[];
extern u16 D_8030298C[];
extern u16 D_803029A0[];
extern u16 D_803029B4[];
extern u16 D_803029B8[];
extern u16 D_803029C8[];
extern u16 D_803029CC[];
extern u16 D_803029D0[];
extern u16 D_803029D4[];
extern u16 D_803029E8[];
extern u16 D_803029EC[];
extern u16 D_803029F0[];
extern u16 D_80302A00[];
extern u16 D_80302A14[];
extern u16 D_80302A28[];
extern u16 D_80302A3C[];
extern u16 D_80302A50[];
extern u16 D_80302A60[];
extern u16 D_80302A64[];
extern u16 D_80302A68[];
extern u16 D_80302A7C[];
extern u16 D_80302A8C[];
extern u16 D_80302A90[];
extern u16 D_80302AA4[];
extern u16 D_80302AB4[];
extern u16 D_80302AB8[];
extern u16 D_80302AD0[];
extern u16 D_80302AE4[];
extern u16 D_80302AE8[];
extern u16 D_80302AFC[];
extern u16 D_80302B0C[];
extern u16 D_80302B10[];
extern u16 D_80302B24[];
extern u16 D_80302B34[];
extern u16 D_80302B4C[];
extern u16 D_80302B50[];
extern u16 D_80302B64[];
extern u16 D_80302B7C[];
extern u16 D_80302B94[];
extern u16 D_80302BA8[];
extern u16 D_80302BAC[];
extern u16 D_80302BB0[];
extern u16 D_80302BB4[];
extern u16 D_80302BCC[];
extern u16 D_80302BE4[];
extern u16 D_80302BE8[];
extern u16 D_80302BEC[];
extern u16 D_80302BF0[];
extern u16 D_80302BF4[];
extern u16 D_80302C08[];
extern u16 D_80302C20[];
extern u16 D_80302C2C[];
extern u16 D_80302C30[];
extern u16 D_80302C34[];
extern u16 D_80302C4C[];
extern u16 D_80302C64[];
extern u16 D_80302C68[];
extern u16 D_80302C7C[];
extern u16 D_80302C94[];
extern u16 D_80302C98[];
extern u16 D_80302CA8[];
extern u16 D_80302CBC[];
extern u16 D_80302CC0[];
extern u16 D_80302CD4[];
extern u16 D_80302CE4[];
extern u16 D_80302CF0[];
extern u16 D_80302CF4[];
extern u16 D_80302D08[];
extern u16 D_80302D14[];
extern u16 D_80302D28[];
extern u16 D_80302D2C[];
extern u16 D_80302D40[];
extern u16 D_80302D58[];
extern u16 D_80302D70[];
extern u16 D_80302D84[];
extern u16 D_80302D9C[];
extern u16 D_80302DB4[];
extern u16 D_80302DCC[];
extern u16 D_80302DE4[];
extern u16 D_80302DFC[];
extern u16 D_80302E14[];
extern u16 D_80302E28[];
extern u16 D_80302E2C[];
extern u16 D_80302E30[];
extern u16 D_80302E34[];
extern u16 D_80302E40[];
extern u16 D_80302E58[];
extern u16 D_80302E70[];
extern u16 D_80302E74[];
extern u16 D_80302E88[];
extern u16 D_80302E9C[];
extern u16 D_80302EB0[];
extern u16 D_80302EC8[];
extern u16 D_80302EE0[];
extern u16 D_80302EF4[];
extern u16 D_80302F08[];
extern u16 D_80302F1C[];
extern u16 D_80302F30[];
extern u16 D_80302F44[];
extern u16 D_80302F54[];
extern u16 D_80302F64[];
extern u16 D_80302F74[];
extern u16 D_80302F8C[];
extern u16 D_80302F90[];
extern u16 D_80302FA0[];
extern u16 D_80302FB0[];
extern u16 D_80302FB4[];
extern u16 D_80302FCC[];
extern u16 D_80302FE0[];
extern u16 D_80302FF0[];
extern u16 D_80303004[];
extern u16 D_80303014[];
extern u16 D_8030302C[];
extern u16 D_80303040[];
extern u16 D_80303058[];
extern u16 D_80303070[];
extern u16 D_80303084[];
extern u16 D_80303098[];
extern u16 D_8030309C[];
extern u16 D_803030B4[];
extern u16 D_803030CC[];
extern u16 D_803030E4[];
extern u16 D_803030F8[];
extern u16 D_8030310C[];
extern u16 D_80303124[];
extern u16 D_80303138[];
extern u16 D_8030314C[];
extern u16 D_80303160[];
extern u16 D_80303174[];
extern u16 D_80303184[];
extern u16 D_80303188[];
extern u16 D_80303198[];
extern u16 D_803031B0[];
extern u16 D_803031C8[];
extern u16 D_803031D8[];
extern u16 D_803031EC[];
extern u16 D_803031FC[];
extern u16 D_8030320C[];
extern u16 D_80303220[];
extern u16 D_80303234[];
extern u16 D_80303248[];
extern u16 D_8030325C[];
extern u16 D_80303270[];
extern u16 D_80303288[];
extern u16 D_8030329C[];
extern u16 D_803032B4[];
extern u16 D_803032C8[];
extern u16 D_803032E0[];
extern u16 D_803032E4[];
extern u16 D_803032F8[];
extern u16 D_80303304[];
extern u16 D_80303318[];
extern u16 D_8030331C[];
extern u16 D_80303320[];
extern u16 D_80303324[];
extern u16 D_80303338[];
extern u16 D_8030334C[];
extern u16 D_80303360[];
extern u16 D_80303378[];
extern u16 D_8030338C[];
extern u16 D_8030339C[];
extern u16 D_803033B0[];
extern u16 D_803033C0[];
extern u16 D_803033D0[];
extern u16 D_803033E4[];
extern u16 D_803033F0[];
extern u16 D_80303400[];
extern u16 D_80303410[];
extern u16 D_80303420[];
extern u16 D_80303424[];
extern u16 D_80303428[];
extern u16 D_80303438[];
extern u16 D_8030343C[];
extern u16 D_80303450[];
extern u16 D_8030345C[];
extern u16 D_80303470[];
extern u16 D_80303488[];
extern u16 D_803034A0[];
extern u16 D_803034B8[];
extern u16 D_803034D0[];
extern u16 D_803034DC[];
extern u16 D_803034EC[];
extern u16 D_803034FC[];
extern u16 D_80303514[];
extern u16 D_80303520[];
extern u16 D_80303534[];
extern u16 D_80303544[];
extern u16 D_8030355C[];
extern u16 D_80303574[];
extern u16 D_8030358C[];
extern u16 D_80303594[];
extern u16 D_803035A0[];
extern u16 D_803035AC[];
extern u16 D_803035B4[];
extern u16 D_803035BC[];
extern u16 D_803035C8[];
/* .data, 0x802F47B0-0x802F99C0 (tools/data_c.py) */
ColorPair D_802F47B0[0x17] = {
    { 0, 0, 0, 255, 0, 0, 0, 255 },
    { 90, 90, 220, 255, 90, 90, 220, 255 },
    { 0, 255, 0, 255, 0, 255, 0, 255 },
    { 0, 255, 255, 255, 0, 255, 255, 255 },
    { 255, 0, 0, 255, 255, 0, 0, 255 },
    { 255, 0, 255, 255, 255, 0, 255, 255 },
    { 255, 255, 0, 255, 255, 255, 0, 255 },
    { 255, 255, 255, 255, 255, 255, 255, 255 },
    { 128, 128, 128, 255, 128, 128, 128, 255 },
    { 255, 180, 0, 255, 255, 180, 0, 255 },
    { 255, 120, 0, 255, 255, 120, 0, 255 },
    { 255, 240, 70, 255, 255, 240, 70, 255 },
    { 255, 0, 0, 255, 255, 255, 0, 255 },
    { 255, 255, 255, 255, 255, 255, 0, 255 },
    { 0, 0, 255, 255, 255, 255, 255, 255 },
    { 0, 255, 255, 255, 0, 0, 255, 255 },
    { 255, 0, 0, 255, 255, 255, 0, 255 },
    { 255, 0, 0, 255, 255, 255, 0, 255 },
    { 255, 255, 255, 255, 0, 0, 255, 255 },
    { 255, 255, 255, 255, 255, 0, 0, 255 },
    { 255, 0, 255, 255, 255, 0, 0, 255 },
    { 255, 180, 0, 255, 255, 120, 0, 255 },
    { 0, 255, 0, 255, 255, 255, 0, 255 },
};
u8 D_802F4868[8] = { 15, 16, 16, 16, 16, 16, 16, 16 };
u8 D_802F4870[8] = { 6, 7, 7, 7, 7, 7, 7, 7 };
u8 D_802F4878[8] = { 13, 25, 14, 23, 16, 0, 16, 14 };
u16 D_802F4880[0x28] = { 0xFFF };
UnkStruct_802F48D0 D_802F48D0[8] = {
    { 0, { 87, 84, 85, 80, 76, 83, 74, 77, 78, 79, 73, 75, -1 } },
    { 15, { 85, 70, 71, 76, -1 } },
    { 16, { 85, 70, 76, 71, -1 } },
    { 3, { 85, 70, 71, 72, -1 } },
    { 18, { 81, -1 } },
    { 33, { 82, -1 } },
    { 13, { 86, 84, -1 } },
    { 10, { 85, -1 } },
};
u8 D_802F49E0[0x14] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
YoshiIcon D_802F49F4[0x4b] = {
    { 0, 0, 0, 0, { 0 }, 1, { 0 }, 0, 1, 0, 1.0f, 1, 0, 0, 0 },
    {
        0, -8, 3, 0, { 9, 61, 9, 61, 9, 62, 9, 62, 9, 63, 9, 63 }, 4, { 2, 1, 2, 3 }, 1, 8, 0,
        1.0f, 2, 9, 0, 0,
    },
    { 0, -8, 1, 0, { 9, 62, 9, 62 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 9, 0, 0 },
    { 0, -8, 1, 0, { 9, 63, 9, 63 }, 1, { 1 }, 2, 8, 0, 1.0f, 2, 9, 0, 0 },
    {
        -16, -8, 5, 0, { 9, 64, 9, 64, 9, 65, 9, 65, 9, 66, 9, 66, 9, 67, 9, 67, 9, 68, 9, 68 },
        10, { 3, 2, 1, 1, 2, 3, 4, 5, 5, 4 }, 1, 10, 0, 1.0f, 2, 9, 0, 0,
    },
    { -16, -8, 1, 0, { 9, 66, 9, 66 }, 1, { 1 }, 1, 10, 0, 1.0f, 2, 9, 0, 0 },
    {
        -32, -9, 5, 0, { 9, 48, 9, 49, 9, 50, 9, 51, 9, 52 }, 7, { 1, 1, 1, 2, 3, 4, 5 }, 1, 9, 0,
        1.0f, 1, 3, 0, 0,
    },
    {
        -32, -9, 5, 0, { 9, 38, 9, 39, 9, 40, 9, 41, 9, 42 }, 8, { 1, 2, 3, 4, 5, 4, 3, 2 }, 1, 6,
        0, 1.0f, 1, 1, 0, 0,
    },
    {
        -32, -7, 5, 0, { 9, 43, 9, 44, 9, 45, 15, 127, 15, 128 }, 8, { 1, 2, 3, 4, 5, 4, 3, 2 }, 1,
        7, 0, 1.0f, 1, 1, 0, 0,
    },
    { -32, -9, 3, 0, { 9, 53, 9, 54, 9, 55 }, 4, { 1, 1, 2, 3 }, 1, 8, 0, 1.0f, 1, 3, 0, 0 },
    { -32, -9, 2, 0, { 9, 46, 9, 47 }, 2, { 1, 2 }, 1, 16, 0, 1.0f, 1, 1, 0, 0 },
    { -32, -9, 1, 0, { 9, 46 }, 1, { 1 }, 2, 16, 0, 1.0f, 1, 1, 0, 0 },
    {
        -28, -8, 5, 0, { 9, 56, 9, 56, 9, 57, 9, 57, 9, 58, 9, 58, 9, 59, 9, 59, 9, 60, 9, 60 }, 5,
        { 1, 2, 3, 4, 5 }, 1, 8, 0, 1.0f, 2, 9, 0, 0,
    },
    { 0, 0, 1, 0, { 9, 16, 9, 15 }, 1, { 1, 2, 3, 4, 5 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    {
        0, 0, 5, 0, { 9, 48, 9, 49, 9, 50, 9, 51, 9, 52 }, 7, { 1, 1, 1, 2, 3, 4, 5 }, 1, 9, 0,
        2.0f, 1, 3, 0, 0,
    },
    {
        0, 0, 5, 0, { 9, 38, 9, 39, 9, 40, 9, 41, 9, 42 }, 5, { 1, 2, 3, 4, 5 }, 1, 6, 0, 2.0f, 1,
        1, 0, 0,
    },
    { 0, 0, 3, 0, { 9, 53, 9, 54, 9, 55 }, 4, { 1, 1, 2, 3 }, 1, 8, 0, 2.0f, 1, 3, 0, 0 },
    { 0, 0, 2, 0, { 9, 46, 9, 47 }, 2, { 1, 2 }, 1, 16, 0, 2.0f, 1, 1, 0, 0 },
    { 184, -3, 1, 0, { 9, 27 }, 1, { 1 }, 2, 16, 0, 0.75f, 1, 0, 0, 0 },
    { 184, -3, 1, 0, { 9, 27 }, 1, { 1 }, 1, 16, 0, 0.75f, 1, 0, 0, 0 },
    { 184, -3, 1, 0, { 9, 28 }, 1, { 1 }, 1, 16, 0, 0.75f, 1, 0, 0, 0 },
    { 184, -3, 1, 0, { 9, 29 }, 1, { 1 }, 1, 16, 0, 0.75f, 1, 0, 0, 0 },
    { 184, -3, 1, 0, { 5, 114 }, 1, { 1 }, 1, 16, 0, 0.75f, 1, 0, 0, 0 },
    {
        0, 0, 5, 0,
        { 9, 220, 9, 219, 9, 222, 9, 221, 9, 224, 9, 223, 9, 226, 9, 225, 9, 228, 9, 227 }, 5,
        { 1, 2, 3, 4, 5 }, 1, 8, 0, 1.0f, 2, 1, 0, 0,
    },
    { 0, 0, 1, 0, { 10, 151, 9, 30 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    {
        0, 0, 5, 0, { 9, 93, 9, 92, 9, 95, 9, 94, 9, 97, 9, 96, 9, 99, 9, 98, 9, 101, 9, 100 }, 5,
        { 1, 2, 3, 4, 5 }, 1, 8, 0, 1.0f, 2, 1, 0, 0,
    },
    { 0, 0, 1, 0, { 5, 101, 5, 100 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 214, 9, 213 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 216, 9, 215 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 5, 94, 5, 93 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 5, 94, 5, 93 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    {
        0, 0, 5, 0, { 9, 48, 9, 49, 9, 50, 9, 51, 9, 52 }, 7, { 1, 1, 1, 2, 3, 4, 5 }, 1, 12, 0,
        2.0f, 1, 3, 0, 0,
    },
    { 0, 0, 1, 0, { 5, 116, 5, 115 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 218, 9, 217 }, 2, { 1, 1 }, 1, 24, 0, 1.0f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 16, 9, 15 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 5, 113, 5, 112 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 24, 9, 23 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 12, 9, 11 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 7, 10, 7, 9 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 7, 114, 7, 19 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 18, 9, 17 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 22, 9, 21 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 10, 9, 9 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 14, 9, 13 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 8, 9, 7 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 9, 26, 9, 25 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { -44, -8, 1, 0, { 7, 18, 7, 17 }, 1, { 1 }, 1, 8, 0, 0.61f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 31 }, 1, { 1 }, 1, 8, 0, 1.0f, 1, 1, 0, 0 },
    { -32, -6, 1, 0, { 9, 27 }, 1, { 1 }, 1, 16, 0, 1.0f, 1, 0, 0, 0 },
    { -32, -6, 1, 0, { 9, 28 }, 1, { 1 }, 1, 16, 0, 1.0f, 1, 0, 0, 0 },
    { -32, -6, 1, 0, { 9, 29 }, 1, { 1 }, 1, 16, 0, 1.0f, 1, 0, 0, 0 },
    { -32, -6, 1, 0, { 5, 114 }, 1, { 1 }, 1, 16, 0, 1.0f, 1, 0, 0, 0 },
    { 0, 0, 1, 0, { 2, 228, 2, 227 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 16, 9, 15 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 5, 113, 5, 112 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 24, 9, 23 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 12, 9, 11 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 7, 10, 7, 9 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 7, 114, 7, 19 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 18, 9, 17 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 22, 9, 21 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 10, 9, 9 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 0 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 14, 9, 13 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 8, 9, 7 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 9, 26, 9, 25 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 7, 18, 7, 17 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
    { 0, 0, 1, 0, { 13, 68, 13, 67 }, 1, { 1 }, 1, 8, 0, 1.0f, 2, 1, 0, 0 },
};
#ifdef VERSION_EU
/* eu: each entry's English and German text (text3, the French, is NULL or the English) and no u16 text. */
YoshiEntry D_802F5804[YOSHI_ENTRIES] = {
    { 32, 56, 48, 24, 24, { 0 }, "SELECT OPTION", "OPTIONEN", NULL, NULL, 0, 0, 30, 7, 0, 0 },
    { 97, 80, 82, 20, 20, { 0 }, "MORE", "WEITER", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 102, 20, 20, { 0 }, "VIEW STATS", "STATISTIK", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 122, 20, 20, { 0 }, "RESTART", "NEUBEGINN", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 142, 20, 20, { 0 }, "QUIT LEVEL", "LEVEL VERLASSEN", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 32, 56, 48, 24, 24, { 0 }, "SELECT OPTION", "OPTIONEN", NULL, NULL, 0, 0, 30, 7, 0, 0 },
    { 113, 80, 82, 20, 20, { 0 }, "CONTINUE", "WEITER", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 102, 20, 20, { 0 }, "CONTROL MODE", "KONTROLLE", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 122, 20, 20, { 0 }, "MISSION BRIEFING", "MISSION", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 142, 20, 20, { 0 }, "MUSIC VOLUME", "MUSIK", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    {
        36, 32, 6, 20, 20, { 0 }, "COLLISION IMMINENT!", "KOLLISION DROHT!", NULL, NULL, 0, 0, 30,
        17, 4, 0,
    },
    { 36, 52, 6, 20, 20, { 0 }, "WARNING!", "WARNUNG!", NULL, NULL, 0, 0, 30, 17, 4, 0 },
    {
        32, 32, 40, 20, 20, { 0 }, "REACTOR MELTDOWN!", "KERNSCHMELZE!", NULL, NULL, 0, 0, 30, 12,
        6, 0,
    },
    {
        36, 44, 64, 20, 20, { 0 }, "ABORTING MISSION", "MISSION ABBRECHEN", NULL, NULL, 0, 0, 30,
        18, 4, 0,
    },
    {
        32, 44, 40, 20, 20, { 0 }, "CONGRATULATIONS!", "GRATULATION!!!", NULL, NULL, 0, 0, 30, 16,
        6, 0,
    },
    {
        36, 40, 64, 20, 20, { 0 }, "MISSION COMPLETE", "MISSION ERFUELLT", NULL, NULL, 0, 0, 30, 14,
        2, 0,
    },
    {
        160, 64, 42, 28, 30, { 0 }, D_8036B980, D_8036B980, D_8036B980, D_802F4880, 0, 0, 30, 12,
        12, 0,
    },
    {
        240, 112, 210, 24, 24, { 0 }, "PRESS START", "DRUECKE START", "PRESS START", NULL, 0, 0, 30,
        7, 7, 0,
    },
    { 0x400, 76, 96, 15, 15, { 0 }, D_8036B9A8, D_8036B9A8, D_8036B9A8, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 216, 96, 15, 15, { 0 }, D_8036B9C8, D_8036B9C8, D_8036B9C8, NULL, 7, 0, 0, 22, 2, 0 },
    { 0x400, 76, 128, 15, 15, { 0 }, D_8036B9E8, D_8036B9E8, D_8036B9E8, NULL, 8, 0, 0, 22, 2, 0 },
    { 0x400, 216, 128, 15, 15, { 0 }, D_8036BA08, D_8036BA08, D_8036BA08, NULL, 9, 0, 0, 22, 2, 0 },
    {
        0x400, 152, 170, 15, 15, { 0 }, D_8036BA28, D_8036BA28, D_8036BA28, NULL, 10, 0, 0, 22, 2,
        0,
    },
    { 0x401, 60, 208, 19, 19, { 0 }, NULL, NULL, NULL, NULL, 2, 0, 30, 7, 4, 0 },
    { 0x401, 248, 208, 19, 19, { 0 }, NULL, NULL, NULL, NULL, 5, 0, 30, 7, 4, 0 },
    {
        160, 64, 42, 28, 30, { 0 }, D_8036B980, D_8036B980, D_8036B980, D_802F4880, 0, 0, 30, 12,
        12, 0,
    },
    { 224, 112, 210, 24, 24, { 0 }, "", "", "", NULL, 0, 0, 30, 7, 7, 0 },
    { 0x400, 40, 108, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 240, 108, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 40, 108, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 240, 108, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x1A0, 130, 118, 22, 22, { 0 }, "******MISSION", " ", NULL, NULL, 0, 0, 0, 4, 2, 0 },
    {
        0x1A0, 130, 140, 22, 22, { 0 }, "FAILED!*****", "**GESCHEITERT!**", NULL, NULL, 0, 0, 0, 4,
        2, 0,
    },
    { 0x401, 60, 208, 19, 19, { 0 }, NULL, NULL, NULL, NULL, 2, 0, 30, 7, 4, 0 },
    { 0x401, 248, 208, 19, 19, { 0 }, NULL, NULL, NULL, NULL, 5, 0, 30, 7, 4, 0 },
    { 36, 40, 6, 20, 20, { 0 }, NULL, NULL, NULL, NULL, 30, 0, 16, 6, 0, 0 },
    { 32, 0, 17, 20, 20, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 16, 6, 0 },
    { 36, 24, 39, 20, 20, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 14, 4, 0 },
    {
        32, 0, 22, 24, 24, { 0 }, "SET MUSIC VOLUME", "MUSIKLAUTSTAERKE", NULL, NULL, 0, 0, 0, 7, 7,
        0,
    },
    { 209, 52, 54, 22, 22, { 0 }, "QUIET", "LEISE", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 209, 188, 54, 22, 22, { 0 }, "LOUD", "LAUT", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    {
        32, 53, 20, 26, 26, { 0 }, "**MISSION FAILED!**", "**GESCHEITERT!**", NULL, NULL, 0, 0, 0,
        4, 4, 0,
    },
    { 36, 40, 6, 20, 20, { 0 }, NULL, NULL, NULL, NULL, 30, 0, 16, 6, 0, 0 },
    {
        0x10A4, 0, -146, 22, 22, { 0 }, "CONGRATULATIONS!", "GRATULATION!!!", NULL, NULL, 0, 0, 30,
        18, 6, 0,
    },
    {
        0x10A0, 0, 0, 16, 16, { 0 }, "MIRACULOUSLY, THE SHUTTLE", "WUNDERBAR, DAS SHUTTLE", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 18, 16, 16, { 0 }, "COMPLETES ITS RETURN", "KEHRT OHNE EINEN", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        0x10A0, 0, 36, 16, 16, { 0 }, "TO EARTH WITHOUT A", "ZWISCHENFALL ZUR", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        0x10A0, 0, 54, 16, 16, { 0 }, "SINGLE CASUALTY.", "ERDE ZURUECK.", NULL, NULL, 0, 0, 30, 6,
        6, 0,
    },
    { 0x10A0, 0, 72, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 90, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 108, 16, 16, { 0 }, "BLAST CORPS HAS COME", "DAS BLAST CORPS TEAM", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 126, 16, 16, { 0 }, "THROUGH WITH FLYING", "WIRD VON DEN MENSCHEN", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 144, 16, 16, { 0 }, "COLOURS YET AGAIN.", "GEFEIERT.", NULL, NULL, 0, 0, 30, 6,
        6, 0,
    },
    { 0x10A0, 0, 162, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 180, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 198, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 216, 16, 16, { 0 }, "THEIR POPULARITY GIVEN", "DIE POPULARITAET DES", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 234, 16, 16, { 0 }, "A FURTHER BOOST, THE", "TEAMS SORGT FUER", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        0x10A0, 0, 252, 16, 16, { 0 }, "TEAM FIND NEW OFFERS", "NEUE AUFTRAEGE. DOCH", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 270, 16, 16, { 0 }, "OF WORK POURING IN -", "DIE MITGLIEDER DES", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 288, 16, 16, { 0 }, "BUT DECIDE THAT MAYBE,", "BLAST CORPS ENTSCHEIDEN", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 306, 16, 16, { 0 }, "FOR NOW, IT'S TIME", "SICH FUER EINEN", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        0x10A0, 0, 324, 16, 16, { 0 }, "FOR A HOLIDAY.", "AUSGIEBIGEN URLAUB.", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        0x10A0, 0, -186, 16, 16, { 0 }, "BATTERED AND CRIPPLED AFTER", "VON DER LANGEN REISE", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -168, 16, 16, { 0 }, "ITS LONG VOYAGE, THE LATEST", "GEBEUTELT, WIRD DAS", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -150, 16, 16, { 0 }, "SPACE SHUTTLE IS THROWN OFF", "SPACE SHUTTLE BEIM", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -132, 16, 16, { 0 }, "COURSE DURING RE-ENTRY AND", "WIEDEREINTRITT IN DIE", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -114, 16, 16, { 0 }, "FORCED INTO DESPERATE", "ERDATMOSPHAERE AUS DER", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -96, 16, 16, { 0 }, "MEASURES.", "BAHN GESCHLEUDERT.", NULL, NULL, 0, 0, 30, 6,
        6, 0,
    },
    { 0x10A0, 0, -78, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -60, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -42, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, -24, 16, 16, { 0 }, "A MAJOR CITY IS SEIZED BY", "DIE BEWOHNER DER STADT", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -6, 16, 16, { 0 }, "PANIC WHEN THE RESIDENTS", "GERATEN IN PANIK, ALS", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 12, 16, 16, { 0 }, "FIND OUT THAT THEIR HOMES", "SIE ERFAHREN, DASS IHRE", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 30, 16, 16, { 0 }, "ARE ABOUT TO BECOME AN", "STADT ZUR NOTLANDEBAHN", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 48, 16, 16, { 0 }, "EMERGENCY LANDING STRIP...", "UMFUNKTIONIERT WIRD...", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 66, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 84, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 102, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 120, 16, 16, { 0 }, "TIME IS OF THE ESSENCE AS", "EINE NEUE HERAUSFORDERUNG",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 138, 16, 16, { 0 }, "BLAST CORPS RISES ONCE", "FUER DAS BLAST CORPS", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 156, 16, 16, { 0 }, "MORE TO THE CHALLENGE.", "TEAM.", NULL, NULL, 0, 0, 30, 6,
        6, 0,
    },
    { 0x10A0, 0, 174, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 192, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 210, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 228, 16, 16, { 0 }, "EVEN AS THE SHUTTLE", "SOBALD DAS SHUTTLE AM", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 246, 16, 16, { 0 }, "BLAZES DOWN THROUGH", "HIMMEL ERSCHEINT,", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        0x10A0, 0, 264, 16, 16, { 0 }, "THE SKIES, A RUNWAY", "MUSS EINE LANDEBAHN", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 282, 16, 16, { 0 }, "MUST BE CLEARED.", "GERAEUMT SEIN.", NULL, NULL, 0, 0, 30,
        6, 6, 0,
    },
    { 176, 0, 36, 20, 20, { 0 }, "SITUATION:", "SITUATION:", NULL, NULL, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 32, 58, 16, 16, { 0 }, "********CARRIER LOCKED ON COURSE********",
        "********TRANSPORTER AUSSER KONTROLLE!***", NULL, NULL, 0, 0, 30, 7, 4, 0,
    },
    { 0x2A0, 0, 84, 20, 20, { 0 }, "SOLUTION:", "AUFGABE:", NULL, NULL, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 23, 106, 16, 16, { 0 }, "********CLEAR PATH TO GROUND ZERO************",
        "********WEG FREILEGEN!************", NULL, NULL, 0, 0, 30, 7, 4, 0,
    },
    { 176, 0, 36, 20, 20, { 0 }, "AGENTS:", "TEAM:", NULL, NULL, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 69, 58, 16, 16, { 0 }, "********BLAST CORPS********", "********BLAST CORPS********",
        NULL, NULL, 0, 0, 30, 7, 4, 0,
    },
    { 0x2A0, 0, 84, 20, 20, { 0 }, "CHANCES:", "CHANCEN:", NULL, NULL, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 94, 106, 16, 16, { 0 }, "********SLIM*!******************",
        "********SCHLECHT!*****************", NULL, NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x1B0, 0, 48, 20, 20, { 0 }, "******MISSION 1:**", "******MISSION 1:**", NULL, NULL, 0, 0,
        30, 4, 4, 0,
    },
    {
        0x220, 0, 74, 15, 15, { 0 }, "CLEAR PATH FOR CARRIER", "RAEUME DEN WEG FUER DEN", NULL,
        NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x220, 0, 91, 15, 15, { 0 }, "ON EACH MAIN LEVEL.", "TRANSPORTER FREI.", NULL, NULL, 0, 0,
        30, 7, 4, 0,
    },
    {
        0x1B0, 0, 40, 20, 20, { 0 }, "******MISSION 2:**", "******MISSION 2:**", NULL, NULL, 0, 0,
        30, 4, 4, 0,
    },
    {
        0x220, 0, 66, 15, 15, { 0 }, "ACTIVATE ALL RDUS AND", "AKTIVIERE ALLE RDU'S UND", NULL,
        NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x220, 0, 83, 15, 15, { 0 }, "DESTROY ALL BUILDINGS", "ZERSTOERE DIE GEBAEUDE, UM", NULL,
        NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x220, 0, 100, 15, 15, { 0 }, "TO EARN SECOND GOLD.", "DAS ZWEITE GOLD ZU ERHALTEN.", NULL,
        NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x1B0, 0, 40, 20, 20, { 0 }, "******MISSION 3:**", "******MISSION 3:**", NULL, NULL, 0, 0,
        30, 4, 4, 0,
    },
    {
        0x220, 0, 66, 14, 15, { 0 }, "AFTER COMPLETING MAIN LEVELS,", "SUCHE NACH DEN 6 WISSEN-",
        NULL, NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x220, 0, 83, 14, 15, { 0 }, "FIND ALL 6 SCIENTISTS TO", "SCHAFTLERN, UM EINE SICHERE",
        NULL, NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x220, 0, 100, 14, 15, { 0 }, "ENSURE A CONTROLLED DETONATION.",
        "EXPLOSION ZU ERMOEGLICHEN!", NULL, NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x1B0, 0, 48, 20, 20, { 0 }, "******MISSION 4:**", "******MISSION 4:**", NULL, NULL, 0, 0,
        30, 4, 4, 0,
    },
    {
        0x220, 0, 74, 15, 15, { 0 }, "ACHIEVE GOLD ON ALL LEVELS", "ERRINGE IN ALLEN LEVELN", NULL,
        NULL, 0, 0, 30, 7, 4, 0,
    },
    {
        0x220, 0, 91, 15, 15, { 0 }, "TO COMMENCE TIME ATTACK.", "DIE GOLDMEDAILLE.", NULL, NULL, 0,
        0, 30, 7, 4, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "BLAST CORPS: LEADERS IN", "DAS BLAST CORPS TEAM (BCT)", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "THE FIELD OF HEAVY DUTY", "IST FUEHREND IN DER BRANCHE", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "DEMOLITION THROUGH A", "DER ABRISSEXPERTEN. ES", NULL, NULL, 0,
        0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "COMBINATION OF SKILL,", "VERFUEGT UEBER MOTIVIERTE", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "EXPERIENCE AND CUTTING-", "MITARBEITER UND DIE", NULL, NULL, 0,
        0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "EDGE TECHNOLOGY.", "NEUESTE TECHNOLOGIE.", NULL, NULL, 0, 0, 30,
        7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "SINCE ITS BIRTH THE COMPANY", "DAS TEAM HAT SEINE SPEZIAL-",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "HAS APPLIED ITS UNIQUE TALENTS",
        "FAEHIGKEITEN DAZU EINGESETZT,", NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "TO THE PROBLEM OF URBAN DECAY,", "VOM ZERFALL BEDROHTE STAEDTE",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "RENOVATING AND REVITALISING", "ZU SAEUBERN UND DAMIT EINEN",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "CITIES FROM ONE END OF THE", "NEUAUFBAU DER VEROTTETEN", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "COUNTRY TO THE OTHER.", "METROPOLEN ERMOEGLICHT.", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "A FAR CRY FROM THE SENSELESS", "DIE MILITAERBASIS RAFTERS",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "WARFARE AMIDST WHICH THE", "ENTWICKELTE SCHRECKLICHE", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "SEEDS OF THE PROJECT WERE", "KAMPFMASCHINEN, DIE VOM", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "SOWN, IN THE HEAVY VEHICLE", "BCT JETZT SINNVOLL FUER", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "DEVELOPMENT BAY AT THE", "FRIEDLICHE ZWECKE", NULL, NULL, 0, 0,
        30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "MILITARY BASE CALLED RAFTERS.", "GENUTZT WERDEN.", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "WHILE DEMONSTRATING A GREAT", "DIE GRUENDUNGSMITGLIEDER", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "NATURAL FLAIR, THE FOUNDING", "DES BLAST CORPS TEAMS,", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "MEMBERS OF THE TEAM - AMBER,", "AMBER, CLARK, WESLEY UND", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "CLARK, WESLEY AND SPIKE - WERE", "SPIKE, NUTZEN DIE", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "NEVER HAPPY WITH THE ULTIMATE", "VEHIKEL ZUR SANIERUNG", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "PURPOSE OF THEIR MACHINES...", "DER ALTEN STAEDTE.", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "SO WHEN WESLEY WAS CRUELLY", "VOR ETWA FUENF JAHREN", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "REJECTED FOLLOWING THE FIELD", "GRUENDETEN SIE ZUM", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "ACCIDENT THAT LEFT HIM", "LEIDWESEN DER ARMEE", NULL, NULL, 0,
        0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "DISABLED, HIS FRIENDS FINALLY", "DAS ERFOLGREICHE", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "REBELLED AND LED THE", "BLAST CORPS TEAM.", NULL, NULL, 0, 0,
        30, 7, 6, 0,
    },
    { 160, 0, 96, 15, 15, { 0 }, "INFAMOUS RAFTERS WALKOUT.", " ", NULL, NULL, 0, 0, 30, 7, 6, 0 },
    {
        176, 0, 16, 15, 15, { 0 }, "BLAST CORPS CAME INTO BEING", "IN DER GEGENWART SORGT", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "SOON AFTER. THAT WAS FIVE", "DAS TEAM DAFUER, DASS", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "YEARS AGO. BUT NOW, IN", "DER MENSCHHEIT EIN", NULL, NULL, 0, 0,
        30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "THE PRESENT DAY, WORLD PEACE", "LEBEN IN STINKENDEN,", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "IS SHATTERED AS MANKIND FACES", "MARODEN STAEDTEN", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "CRISIS ON A WORLDWIDE SCALE.", "ERSPART BLEIBT.", NULL, NULL, 0,
        0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "A PAIR OF DEFECTIVE NUCLEAR", "EIN VOLLAUTOMATISCHER", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "MISSILES, EN ROUTE TO A SAFE", "TRANSPORTER BEFOERDERT", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "DETONATION SITE, HAVE BEGUN", "NUKLEARE SPRENGKOEPFE UND", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "TO LEAK. BADLY DAMAGED, THE", "IST AUSSER KONTROLLE GERATEN.",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "CARRIER AUTOMATICALLY LOCKS", "DER AUTOPILOT STEUERT", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "ONTO THE MOST DIRECT ROUTE.", "STUR DIE DIREKTE ROUTE.", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "BAD MEMORIES RESURFACE FOR", "DAS BCT IST DIE EINZIGE", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "THE BLAST CORPS TEAM WHEN,", "ORGANISATION, WELCHE DIE", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "SUMMONED TO THEIR NATION'S", "NUKLEARE KATASTROPHE NOCH", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "DEFENCE, THEY FIND OUT WHERE", "ABWENDEN KANN. DIE", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "THE WARHEADS ORIGINATED: A", "SPRENGKOEPFE STAMMEN WOHL", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "CERTAIN NEARBY MILITARY BASE.", "AUS DER ARMEEBASIS.", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "THE FLOOD OF RADIATION PREVENTS", "DEM LECKGESCHLAGENEN LKW",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "ANYONE GETTING CLOSE TO THE", "ENTWEICHT RADIOAKTIVE", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "RUNAWAY CARRIER, AND PEOPLE IN", "STRAHLUNG. DESHALB KANN MAN",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "THE KNOW FEAR THAT EVEN THE", "DEN TRANSPORTER NICHT", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "SLIGHTEST JOLT COULD TRIGGER", "BESTEIGEN, UM IHN VON", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "A CATASTROPHIC EXPLOSION.", "SEINEM KURS ABZUBRINGEN.", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "STANDING AS THE WORLD'S FINAL", "DAS BCT MUSS DEN WEG DES",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 32, 15, 15, { 0 }, "HOPE, BLAST CORPS MUST CLEAR", "TRANSPORTERS FREIMACHEN,", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "THE WAY TO GROUND ZERO, GATHER", "ALLES AUS DEM WEG RAEUMEN,",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "A TEAM OF SIX ELITE SCIENTISTS", "SECHS ELITE-WISSENSCHAFTLER",
        NULL, NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "AND ULTIMATELY COUNTER THE", "FINDEN UND DAMIT DEN", NULL, NULL,
        0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "THREAT OF NUCLEAR WINTER.", "ATOMAREN SUPERGAU ABWENDEN.", NULL,
        NULL, 0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 88, 16, 16, { 0 }, "EVEN AS THE CARRIER", "DA DER TRANSPORTER", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        160, 0, 106, 16, 16, { 0 }, "TRUNDLES TOWARDS GROUND", "UNKONTROLLIERT SEINEM", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        160, 0, 124, 16, 16, { 0 }, "ZERO, YOU ARE DOING", "ZIEL ENTGEGENFAEHRT, MUSST", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        160, 0, 142, 16, 16, { 0 }, "EVERYTHING IN YOUR POWER", "DU DEIN BESTES GEBEN, UM", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        160, 0, 160, 16, 16, { 0 }, "TO GET THE ASSEMBLED", "DIE WISSENSCHAFTLER ZU", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        160, 0, 178, 16, 16, { 0 }, "SCIENTISTS THERE FIRST...", "UNTERSTUETZEN...", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -188, 16, 16, { 0 }, "MERCIFULLY, THE SCIENTISTS", "DEN WISSENSCHAFTLERN IST",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -170, 16, 16, { 0 }, "ARE ABLE TO SET UP A", "ES GELUNGEN, DEN LKW", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -152, 16, 16, { 0 }, "PROPERLY CONTROLLED", "MIT DER NUKLEAREN LADUNG", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -134, 16, 16, { 0 }, "DETONATION - AND FINALLY,", "DURCH EINE EXAKT GEPLANTE",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -116, 16, 16, { 0 }, "AS THE SMOKE FADES, THE WORLD",
        "UND KONTROLLIERTE EXPLOSION", NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -98, 16, 16, { 0 }, "CAN LET OUT A SIGH OF RELIEF.", "UNSCHAEDLICH ZU MACHEN.",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, -80, 16, 16, { 0 }, "", "", NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -62, 16, 16, { 0 }, "", "", NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -44, 16, 16, { 0 }, "", "", NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, -26, 16, 16, { 0 }, "THE DEVASTATION LEFT IN", "DIE VERWUESTUNG, DIE DAS", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, -8, 16, 16, { 0 }, "YOUR WAKE IS NOTHING COMPARED", "BLAST CORPS TEAM WAEHREND",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 10, 16, 16, { 0 }, "TO WHAT WOULD HAVE HAPPENED", "DES EINSATZES ANGERICHTET",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 28, 16, 16, { 0 }, "HAD BLAST CORPS FAILED", "HAT, IST BEI WEITEM NICHT", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 46, 16, 16, { 0 }, "AT THE LAST.", "SO GROSS WIE DIE DROHENDE", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    { 0x10A0, 0, 64, 16, 16, { 0 }, "", "NUKLEARKATASTROPHE.", NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 82, 16, 16, { 0 }, "REBUILDING BEGINS IMMEDIATELY.", "", NULL, NULL, 0, 0, 30, 6,
        6, 0,
    },
    {
        0x10A0, 0, 100, 16, 16, { 0 }, "", "DIE MENSCHHEIT KANN WIEDER", NULL, NULL, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, 118, 16, 16, { 0 }, "", "AUFATMEN. DIE GEFAHR WURDE", NULL, NULL, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, 136, 16, 16, { 0 }, "", "IM LETZTEN MOMENT GEBANNT.", NULL, NULL, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, 154, 16, 16, { 0 }, "WITH CATASTROPHE AVERTED, THE", "", NULL, NULL, 0, 0, 30, 6,
        6, 0,
    },
    {
        0x10A0, 0, 172, 16, 16, { 0 }, "TEAM MEMBERS BECOME NATIONAL", "", NULL, NULL, 0, 0, 30, 6,
        6, 0,
    },
    {
        0x10A0, 0, 190, 16, 16, { 0 }, "HEROES, THEIR SUCCESS AND", "DAS BCT-TEAM WIRD VON", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 208, 16, 16, { 0 }, "SATISFACTION ASSURED FOR", "ALLEN GEFEIERT.", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 226, 16, 16, { 0 }, "THE FORESEEABLE FUTURE.", "", NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**RDUS COLLECTED!**********", "**RDU'S AKTIVIERT!*******",
        NULL, NULL, 0, 0, 30, 18, 4, 0,
    },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**SURVIVORS FREE!**********", "**UEBERLEBENDE BEFREIT!***",
        NULL, NULL, 0, 0, 30, 18, 4, 0,
    },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**BUILDINGS DESTROYED!**********",
        "**GEBAEUDE ZERSTOERT!**********", NULL, NULL, 0, 0, 30, 18, 4, 0,
    },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**LEVEL COMPLETE!**********", "**LEVEL BEENDET!**********",
        NULL, NULL, 0, 0, 30, 18, 4, 0,
    },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**PATH CLEARED!**********", "**WEG FREIGELEGT!********", NULL,
        NULL, 0, 0, 30, 18, 4, 0,
    },
    {
        0x1A0, 24, 20, 24, 24, { 0 }, "********EMERGENCY!****************",
        "********ALARM!****************", NULL, NULL, 0, 0, 30, 4, 4, 0,
    },
    {
        160, 0, 48, 20, 20, { 0 }, "YOU MUST COMPLETELY", "RAEUME DIE GEGENSTAENDE", NULL, NULL, 0,
        0, 30, 7, 4, 0,
    },
    {
        160, 0, 70, 20, 20, { 0 }, "REMOVE ALL OBSTACLES", "AUS DER GEFAHRENZONE!", NULL, NULL, 0,
        0, 30, 7, 4, 0,
    },
    { 160, 0, 92, 20, 20, { 0 }, "FROM THE DANGER ZONE!", " ", NULL, NULL, 0, 0, 30, 7, 4, 0 },
    {
        164, 32, 6, 20, 20, { 0 }, "DANGER ZONE!", "EINE KOLLISION DROHT!", NULL, NULL, 0, 0, 30,
        17, 4, 0,
    },
    { 0x480, -32, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 16, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, -128, 16, 16, { 0 }, "THIS IS AN RDU,", "DIES IST EIN RDU. SOBALD", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, -110, 16, 16, { 0 }, "TRIGGERED REMOTELY", "DU DARUEBER FAEHRST,", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, -92, 16, 16, { 0 }, "AS YOU DRIVE BY.", "IST ES AKTIVIERT.", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    { 0x1020, 0, -74, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x1020, 0, -56, 16, 16, { 0 }, "THEY CAN BE USED FOR", "RDU'S WEISEN DIR DEN WEG", NULL,
        NULL, 0, 0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, -38, 16, 16, { 0 }, "GUIDANCE AS WELL AS", "UND DIENEN AUCH ZUR", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, -20, 16, 16, { 0 }, "RADIATION DISPERSAL.", "STRAHLENMINDERUNG.", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 0x480, -32, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 56, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, -128, 16, 16, { 0 }, "COMMUNICATION POINTS", "KOMMUNIKATIONSPUNKTE", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, -110, 16, 16, { 0 }, "ALLOW YOU TO MAKE", "ERLAUBEN DIR, KONTAKT", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, -92, 16, 16, { 0 }, "CONTACT WITH HQ.", "ZUR BASIS AUFZUNEHMEN.", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 0x1020, 0, -74, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x1020, 0, -56, 16, 16, { 0 }, "WHEN ACTIVATED, THEY", "WURDEN SIE AKTIVIERT,", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, -38, 16, 16, { 0 }, "BREAK OPEN VALUABLE", "OEFFNEN SIE WEGE ZU", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, -20, 16, 16, { 0 }, "NEW TRAINING LEVELS.", "NEUEN TRAININGSLEVELN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x1020, 0, -2, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x1020, 0, 16, 16, 16, { 0 }, "YOU CAN", "DU FINDEST DIE NEUEN", NULL, NULL, 0, 0, 30, 7, 7,
        0,
    },
    {
        0x1021, 0, 34, 16, 16, { 0 }, "ACCESS THESE FROM", "WEGE ZU DEN LEVELN", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 52, 16, 16, { 0 }, "THE WORLD SCREEN.", "AUF DER WELTKARTE.", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 62, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "BACKLASH", "MONSTERTRUCK", NULL, NULL, 0, 0, 30, 4, 7, 0 },
    {
        0x10A0, 0, 20, 16, 16, { 0 }, "DESTROY BUILDINGS", "ZERSTOERE MIT DER", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 38, 16, 16, { 0 }, "WITH BACKLASH USING", "STAEHLERNEN KIPPLADE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 56, 16, 16, { 0 }, "ITS ARMOURED REAR.", "DES TRUCKS GEBAEUDE.", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 92, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 110, 16, 16, { 0 }, "USE R TO SKID", "DRUECKE DIE R-TASTE,", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 128, 16, 16, { 0 }, "THE TRUCK WHEN", "UM DEN TRUCK INS", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x10A0, 0, 146, 16, 16, { 0 }, "GOING INTO A TURN.", "SCHLIDDERN ZU BRINGEN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x10A0, 0, 164, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 182, 16, 16, { 0 }, "AIM FOR AT LEAST", "VERSUCHE MINDESTENS", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 200, 16, 16, { 0 }, "A SILVER MEDAL", "DIE SILBERMEDAILLE", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x10A0, 0, 218, 16, 16, { 0 }, "BEFORE PROGRESSING:", "ZU ERREICHEN.", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    { 0x10A0, 0, 236, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 254, 16, 16, { 0 }, "THIS TECHNIQUE", "DIESE TECHNIK MUSS", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x10A0, 0, 272, 16, 16, { 0 }, "MUST BE MASTERED", "IN SPAETEREN LEVELN", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 290, 16, 16, { 0 }, "FOR LATER LEVELS.", "GUT BEHERRSCHT WERDEN.", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 0x10A0, 0, 308, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 326, 16, 16, { 0 }, "USE BUMPS TO GET", "NUTZE DAS STAHLHECK,", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 344, 16, 16, { 0 }, "BACKLASH AIRBORNE AND", "UM DIE ZERSTOERUNGS-", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 362, 16, 16, { 0 }, "CAUSE MAXIMUM DAMAGE.", "KRAFT ZU OPTIMIEREN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 58, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "SIDESWIPE", "XR7-PROTOCRASH", NULL, NULL, 0, 0, 30, 4, 7, 0 },
    {
        0x10A0, 0, 20, 16, 16, { 0 }, "HITS HARDEST AT THE", "DIE SEITENTEILE TREFFEN", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 38, 16, 16, { 0 }, "MAXIMUM EXTENSION", "AM HAERTESTEN, WENN SIE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 56, 16, 16, { 0 }, "OF ITS SIDE PANELS.", "GANZ AUSGEFAHREN SIND.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 92, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 110, 16, 16, { 0 }, "FIND BLUE AMMO BOXES", "SUCHE BLAUE KISTEN,", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 128, 16, 16, { 0 }, "TO KEEP SIDESWIPE'S", "UM DIE ANGRIFFSKRAFT", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 146, 16, 16, { 0 }, "ATTACK POWER AT FULL.", "DES XR7 ZU SICHERN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x10A0, 0, 164, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 182, 16, 16, { 0 }, "CHARGES REMAINING", "DER MUNITIONSVORRAT", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 200, 16, 16, { 0 }, "ARE DISPLAYED IN THE", "WIRD AUF DEM BILDSCHIRM", NULL,
        NULL, 0, 0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 218, 16, 16, { 0 }, "LOWER LEFT CORNER.", "LINKS UNTEN ANGEZEIGT.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 59, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "THUNDERFIST", "DONNERFAUST", NULL, NULL, 0, 0, 30, 4, 7, 0 },
    {
        0x10A0, 0, 20, 16, 16, { 0 }, "DEMOLISH BUILDINGS", "ZERSTOERE GEBAEUDE", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    { 0x10A0, 0, 38, 16, 16, { 0 }, "BY DIVING AND", "MIT EINEM", NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 56, 16, 16, { 0 }, "ROLLING INTO THEM.", "POWER-SALTO.", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 92, 16, 16, { 0 }, "A WELL-TIMED SERIES", "PERFEKTES TIMING", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 110, 16, 16, { 0 }, "OF ATTACKS CAN CAUSE", "KANN GROSSEN", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x10A0, 0, 128, 16, 16, { 0 }, "INCREDIBLE DAMAGE.", "SCHADEN ANRICHTEN.", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 60, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "SKYFALL", "TURBOBUGGY", NULL, NULL, 0, 0, 30, 4, 7, 0 },
    {
        0x10A0, 0, 20, 16, 16, { 0 }, "MAKE USE OF SKYFALL'S", "SPRINGE AUF OBJEKTE,", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 38, 16, 16, { 0 }, "ARMOURED UNDERSIDE", "UM DIE STAHLUNTERSEITE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 56, 16, 16, { 0 }, "TO CRUSH FROM ABOVE.", "DES BUGGYS EINZUSETZEN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 92, 16, 16, { 0 }, "TURBO INTO A DITCH", "DRUECKE DIE L- ODER", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 110, 16, 16, { 0 }, "WITH L/R TO LAUNCH", "R-TASTE, UM DEN TURBO", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x10A0, 0, 128, 16, 16, { 0 }, "YOURSELF SKYWARDS...", "ZU AKTIVIEREN.", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 66, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "J-BOMB", "J-BOMB", NULL, NULL, 0, 0, 30, 4, 7, 0 },
    {
        0x10A0, 0, 20, 16, 16, { 0 }, "USE A TO THRUST", "DRUECKE DEN A-KNOPF,", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x10A0, 0, 38, 16, 16, { 0 }, "J-BOMB INTO THE", "UM DIE JETS ZU", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x10A0, 0, 56, 16, 16, { 0 }, "AIR OVER A TARGET...", "AKTIVIEREN.", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 7, 0 },
    {
        0x10A0, 0, 92, 16, 16, { 0 }, "THEN HIT B TO", "DRUECKE DEN B-KNOPF,", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x10A0, 0, 110, 16, 16, { 0 }, "DIVE EARTHWARDS", "UM NACH UNTEN ZU", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    { 0x10A0, 0, 128, 16, 16, { 0 }, "FROM A HEIGHT.", "STAMPFEN.", NULL, NULL, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, -128, 16, 16, { 0 }, "SURVIVORS ESCAPE WHEN", "UEBERLEBENDE ENTKOMMEN,", NULL,
        NULL, 0, 0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, -110, 16, 16, { 0 }, "THE WALLS AROUND", "WENN SIE AUS DEN", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, -92, 16, 16, { 0 }, "THEM ARE DESTROYED.", "GEBAEUDEN BEFREIT", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, -56, 16, 16, { 0 }, "CUE THE BLAST CORPS", "WURDEN. DER BLAST CORPS", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, -38, 16, 16, { 0 }, "CHOPPER, SWOOPING IN", "HELIKOPTER NIMMT SIE", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, -20, 16, 16, { 0 }, "TO PICK THEM UP.", "AUF. EINE GOLDMEDAILLE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 16, 16, 16, { 0 }, "ONE GOLD COMMENDATION", "GIBT ES FUER DAS", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 34, 16, 16, { 0 }, "IS GIVEN PER LEVEL", "RAEUMEN DES WEGES. DIE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 52, 16, 16, { 0 }, "FOR PATH CLEARANCE:", "ZWEITE FUER DIE RETTUNG", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 88, 16, 16, { 0 }, "THE SECOND REQUIRES", "ALLER UEBERLEBENDEN,", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 106, 16, 16, { 0 }, "ALL SURVIVORS, RDUS", "DAS FINDEN DER RDU'S UND", NULL,
        NULL, 0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 124, 16, 16, { 0 }, "AND TOTAL DESTRUCTION.", "DIE TOTALE ZERSTOERUNG.", NULL,
        NULL, 0, 0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 13, 0, 0, 22, 2, 0 },
    { 0x1061, 0, -128, 20, 20, { 0 }, "WARNING!", "WARNUNG!", NULL, NULL, 0, 0, 30, 4, 4, 0 },
    {
        0x1020, 0, -74, 16, 16, { 0 }, "SOMETHING IN THE", "ETWAS BEFINDET SICH", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, -56, 16, 16, { 0 }, "CARRIER'S PATH", "NOCH AUF DEM WEG DES", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, -38, 16, 16, { 0 }, "HAS BEEN MISSED!", "TRANSPORTERS! ACHTE", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, -2, 16, 16, { 0 }, "KEEP AN EYE", "AUF DEN PFEIL LINKS", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1021, 0, 16, 16, 16, { 0 }, "ON THE LOWER", "UNTEN. ER WECHSELT", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 34, 16, 16, { 0 }, "LEFT ARROW...", "VON GRUEN ZU ROT,", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 70, 16, 16, { 0 }, "IT CHANGES FROM GREEN", "JE NAEHER DER LKW", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 88, 16, 16, { 0 }, "TO RED AS YOU CLOSE", "KOMMT. ACHTE AUF", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 106, 16, 16, { 0 }, "IN ON THE CARRIER.", "DEN RADARSCHIRM. ROT", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 142, 16, 16, { 0 }, "USE IT WITH THE RADAR", "ZEIGT DIR, WO SICH", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 160, 16, 16, { 0 }, "TO QUICKLY TRACK", "DER TRANSPORTER", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 178, 16, 16, { 0 }, "DOWN THE PROBLEM:", "BEFINDET. BLAU ZEIGT", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 214, 16, 16, { 0 }, "RED INDICATES THE", "DAS NAECHSTE GEBAEUDE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 232, 16, 16, { 0 }, "CARRIER, BLUE THE NEXT", "AN, DAS SICH IM WEG", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 250, 16, 16, { 0 }, "BUILDING IN ITS PATH.", "DES LKW'S BEFINDET.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, -74, 16, 16, { 0 }, "CONGRATULATIONS!", "GRATULATION!", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1021, 0, -56, 16, 16, { 0 }, "THIS IS ONE OF", "DIES IST EINES DER", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, -38, 16, 16, { 0 }, "THE BONUS VEHICLES.", "BONUS-FAHRZEUGE.", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, -2, 16, 16, { 0 }, "THEY ARE", "SIE SIND BESONDERS", NULL, NULL, 30, 0, 7, 7, 0,
        0,
    },
    {
        0x1021, 0, 16, 16, 16, { 0 }, "MOST USEFUL IN", "IN DEN TRAININGS-", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 34, 16, 16, { 0 }, "TRAINING STAGES:", "LEVELN HILFREICH.", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 70, 16, 16, { 0 }, "ACCESS THESE", "SUCHE NACH KONTAKT-", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1021, 0, 88, 16, 16, { 0 }, "VIA THE LEVEL'S", "PUNKTEN, UM TRAININGS-", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 106, 16, 16, { 0 }, "COMMUNICATION POINTS.", "LEVEL ZU ENTDECKEN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1024, 0, 0, 16, 16, { 0 }, "PATH CLEARED!", "WEG FREIGELEGT!", NULL, NULL, 0, 0, 30, 18,
        18, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "YOUR PRIMARY MISSION", "DEINE ERSTE AUFGABE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "HERE IS COMPLETE.", "IST BEENDET. DU", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "THE BLAST CORPS", "KANNST DEN LEVEL", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "SEMI ALLOWS YOU", "VERLASSEN. WECHSLE", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 108, 16, 16, { 0 }, "TO EXIT THE LEVEL.", "DIE FAHRZEUGE, INDEM", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 144, 16, 16, { 0 }, "MOVE BETWEEN", "DU DEN Z-TRIGGER", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1021, 0, 162, 16, 16, { 0 }, "VEHICLES WITH", "DRUECKST. HAST DU", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 180, 16, 16, { 0 }, "THE Z BUTTON.", "NOCH ZEITRESERVEN,", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 216, 16, 16, { 0 }, "SPARE TIME CAN BE", "KANNST DU RDU'S", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1021, 0, 234, 16, 16, { 0 }, "USED TO FIND RDUS AND", "SUCHEN ODER WEITERE", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 252, 16, 16, { 0 }, "DESTROY BUILDINGS...", "GEBAEUDE EINSTAMPFEN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 288, 16, 16, { 0 }, "RETURN IF NECESSARY", "DU KANNST DIE", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1021, 0, 306, 16, 16, { 0 }, "AFTER CHECKING", "LEVEL JEDERZEIT", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 324, 16, 16, { 0 }, "YOUR PERFORMANCE.", "BETRETEN.", NULL, NULL, 0, 0, 30, 7, 7,
        0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "USE Z TO GET OUT", "DRUECKE DEN Z-TRIGGER,", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "OF ONE VEHICLE", "UM ZWISCHEN DEN", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "AND COMMANDEER ANOTHER.", "VEHIKELN ZU WECHSELN.", NULL,
        NULL, 0, 0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "THE DESTRUCTION OF", "DIE ZERSTOERUNG DIESES", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "THIS BUILDING", "GEBAEUDES IST ABSOLUT", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "IS ESSENTIAL!", "WICHTIG! DIE PFEILE", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "FLASHING ARROWS", "ZEIGEN AN, DASS DAS", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "MEAN IT STANDS IN", "OBJEKT IM WEG DES", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 108, 16, 16, { 0 }, "THE CARRIER'S PATH.", "TRANSPORTERS STEHT.", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 144, 16, 16, { 0 }, "AS DANGER CLOSES IN,", "DIE PFEILE AENDERN", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 162, 16, 16, { 0 }, "THE ARROWS CHANGE", "BEI GEFAHR IHRE FARBE", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 180, 16, 16, { 0 }, "FROM GREEN TO RED.", "VON GRUEN ZU ROT!", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "THIS IS A PERIPHERY", "DIES IST EIN NEBEN-", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "STRUCTURE: CRUSHING", "GEBAUEDE. ES IST", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "IT IS NOT VITAL.", "NICHT VON GROSSER", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "THEN AGAIN, IT'S FUN", "BEDEUTUNG, KANN", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "- AND MIGHT REVEAL", "JEDOCH AUCH EINE", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 108, 16, 16, { 0 }, "A SURPRISE OR TWO...", "UEBERRASCHUNG BERGEN.", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "LEVELLING EVERYTHING", "DAS EINEBNEN DER GEGEND", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "HELPS YOU GAIN", "KANN ZU BEFOERDERUNGEN", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "A COMMENDATION.", "VERHELFEN. DOCH DEINE", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "HOWEVER, IT'S A", "HAUPTAUFGABE IST ES,", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "SECONDARY OBJECTIVE", "DEN WEG FUER DEN LKW", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 108, 16, 16, { 0 }, "TO CLEARING THE WAY.", "FREIZUMACHEN. DU", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 144, 16, 16, { 0 }, "CONCENTRATE ON THE", "HAST GENUG ZEIT,", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 162, 16, 16, { 0 }, "ARROWED BUILDINGS AS", "UM DEN LEVEL SPAETER", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 180, 16, 16, { 0 }, "THE CARRIER PASSES.", "NOCH EINMAL ZU", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 216, 16, 16, { 0 }, "PLENTY OF TIME TO", "BESUCHEN UND DICH", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 234, 16, 16, { 0 }, "COME BACK LATER", "UM DEN REST ZU", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 252, 16, 16, { 0 }, "AND FINISH THE JOB.", "KUEMMERN.", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "USE Z TO GET OUT", "DRUECKE DEN Z-TRIGGER,", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "OF ONE VEHICLE", "UM ZWISCHEN DEN", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "AND COMMANDEER ANOTHER.", "FAHRZEUGEN ZU WECHSELN.", NULL,
        NULL, 0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "BUT CLEAR A PATH FOR", "PRUEFE ABER VORHER,", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "THE CARRIER BEFORE", "OB DU DEN WEG", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 108, 16, 16, { 0 }, "GOING OFF TO EXPLORE!", "FREIGEMACHT HAST!", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "THE CRANE CAN MOVE", "DER KRAN KANN VEHIKEL", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "OBJECTS TO PREVIOUSLY", "BEFOERDERN. LADE DAS", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "INACCESSIBLE PLACES.", "GEFAEHRT AUF UND", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "LOAD IT UP THEN", "STEIGE IN DIE", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "HEAD FOR THE", "STEUERZENTRALE DES", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 108, 16, 16, { 0 }, "CONTROLS IN THE CAB.", "KRANS.", NULL, NULL, 0, 0, 30, 7, 7,
        0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "COLLECT AMMO BOXES", "SAMMLE MUNITIONS-", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "AND YOU CAN BLAST", "KISTEN UND SCHIESSE", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "YOUR WAY THROUGH.", "DIR DEINEN WEG FREI.", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "YOU COULD STOP", "DU KANNST DEN ZUG", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "THE TRAIN AT", "AN DIESER STATION", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "THIS STATION.", "STOPPEN. WARTE AUF", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "WAIT FOR THE SMILEY", "DAS SMILEY-GESICHT,", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "BEFORE ATTEMPTING TO", "UM ZU LADEN ODER", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1020, 0, 108, 16, 16, { 0 }, "LOAD OR UNLOAD.", "ENTLADEN.", NULL, NULL, 0, 0, 30, 7, 7,
        0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "TNT CRATES CAN BE", "TNT-KISTEN KOENNEN", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "PUSHED AROUND USING", "MIT DER SCHAUFEL DES", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "RAMDOZER'S SHOVEL.", "BULLDOZERS GESCHOBEN", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 72, 16, 16, { 0 }, "BUT THEY WON'T BE", "WERDEN. ACHTUNG! SIE", NULL, NULL, 0, 0,
        30, 7, 7, 0,
    },
    {
        0x1021, 0, 90, 16, 16, { 0 }, "STABLE FOR LONG...", "ZUENDEN AUTOMATISCH!", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "SELECT START THEN", "DRUECKE START UND", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "VIEW STATS TO CHECK", "WAEHLE STATISTIKEN, UM", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "STATUS OF LEVEL.", "DEINEN STATUS ZU SEHEN.", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "THE TRAIN CAN HELP", "DER ZUG KANN DEN", NULL, NULL, 0, 0, 30,
        7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "TRANSPORT RAMDOZER", "BULLDOZER ZUR STATION", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "TO THE STATION.", "TRANSPORTIEREN.", NULL, NULL, 0, 0, 30, 7,
        7, 0,
    },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    {
        0x1020, 0, 0, 16, 16, { 0 }, "PRESSING START WILL", "DRUECKE START UND DU", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    {
        0x1021, 0, 18, 16, 16, { 0 }, "ALLOW YOU TO VIEW THE", "KANNST DIR DEN WEG DES", NULL, NULL,
        0, 0, 30, 7, 7, 0,
    },
    {
        0x1020, 0, 36, 16, 16, { 0 }, "MISSILE CARRIER'S PATH.", "LKW'S BETRACHTEN.", NULL, NULL, 0,
        0, 30, 7, 7, 0,
    },
    { 160, -36, 34, 22, 22, { 0 }, NULL, NULL, NULL, NULL, 61, 0, 0, 7, 2, 0 },
    { 32, 0, 60, 19, 19, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 209, 48, 84, 29, 29, { 0 }, "NO", "NEIN", NULL, NULL, 0, 0, 30, 8, 4, 0 },
    { 209, 180, 84, 29, 29, { 0 }, "YES", "JA", NULL, NULL, 0, 0, 30, 8, 4, 0 },
    {
        0x10A4, 0, -146, 22, 22, { 0 }, "CONGRATULATIONS!", "GRATULATION!!!", NULL, NULL, 0, 0, 30,
        18, 6, 0,
    },
    {
        0x10A0, 0, 0, 16, 16, { 0 }, "HAVING DEMONSTRATED", "NACH DIESER DEMONSTRATION", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 18, 16, 16, { 0 }, "VERSATILITY AND RELIABILITY", "IHRES KOENNENS UND IHRER",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 36, 16, 16, { 0 }, "WELL BEYOND THE CALL OF", "ZUVERLAESSIGKEIT HABEN", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 54, 16, 16, { 0 }, "DUTY, THE BLAST CORPS", "SICH DIE MITGLIEDER", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 72, 16, 16, { 0 }, "TEAM CAN FINALLY TAKE", "DES BLAST CORPS TEAMS", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 90, 16, 16, { 0 }, "THAT WELL-DESERVED HOLIDAY.", "DEN URLAUB WOHLVERDIENT.",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 108, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 126, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 144, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 162, 16, 16, { 0 }, "WHEN THEY GET BACK THEY'LL", "NACH IHRER RUECKKEHR", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 180, 16, 16, { 0 }, "FIND THE OFFERS AND DEALS", "IST DER BRIEFKASTEN", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 198, 16, 16, { 0 }, "STILL FLOODING IN,", "VOLLER INTERESSANTER", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 216, 16, 16, { 0 }, "KEEPING THEM IN THEIR", "AUFTRAEGE, DIE IHNEN", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 234, 16, 16, { 0 }, "CHOSEN LINE OF WORK", "ARBEIT FUER ETLICHE", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 252, 16, 16, { 0 }, "FOR MANY YEARS TO COME...", "JAHRE BESCHEREN...", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 270, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 288, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 306, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 324, 16, 16, { 0 }, "MAYBE AT SOME POINT EVEN", "VIELLEICHT NEHMEN SIE", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 342, 16, 16, { 0 }, "LEADING THEM BACK INTO", "AUCH WIEDER AUFTRAEGE", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 360, 16, 16, { 0 }, "THE FIELD OF MILITARY", "DER ARMEE ENTGEGEN,", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 378, 16, 16, { 0 }, "OPERATIONS - BUT THIS", "DOCH WERDEN SIE SICH NUR", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 396, 16, 16, { 0 }, "TIME FOR A CONSIDERABLY", "FUER MISSIONEN DES", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 414, 16, 16, { 0 }, "NOBLER CAUSE.", "FRIEDENS EINSETZEN.", NULL, NULL, 0, 0, 30,
        6, 6, 0,
    },
    { 0x10A0, 0, 432, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 450, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 468, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 486, 16, 16, { 0 }, "ALL THAT, THOUGH, CAN WAIT.", "ABER ES GIBT NOCH", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 504, 16, 16, { 0 }, "WITH THEIR COUNTRY", "KEINE NOTWENDIGKEIT,", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 522, 16, 16, { 0 }, "BREATHING A SIGH OF", "SOLCHE ARMEE-EINSAETZE", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 540, 16, 16, { 0 }, "RELIEF AND THEIR GOOD", "ANZUTRETEN. DAS TEAM", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 558, 16, 16, { 0 }, "NAME ASSURED FOR LIFE,", "KANN SICH DAHER", NULL, NULL, 0,
        0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 576, 16, 16, { 0 }, "THE TEAM CAN REST EASY", "NOCH EIN WENIG", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    { 0x10A0, 0, 594, 16, 16, { 0 }, "FOR A WHILE.", "ENTSPANNEN.", NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 612, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 630, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 648, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 666, 16, 16, { 0 }, "UNLESS, OF COURSE, THE", "ANDERERSEITS LOCKEN AUCH", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 684, 16, 16, { 0 }, "LURE OF THE GOLD STANDARD", "DIE GOLDMEDAILLEN. DAS", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 702, 16, 16, { 0 }, "PROVES TOO MUCH...", "EDLE METALL GLAENZT...", NULL, NULL,
        0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 720, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 738, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 756, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 774, 16, 16, { 0 }, NULL, NULL, NULL, NULL, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 792, 16, 16, { 0 }, "PERHAPS THERE ARE", "VIELLEICHT ERWARTET", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
    {
        0x10A0, 0, 810, 16, 16, { 0 }, "FURTHER CHALLENGES AWAITING", "BESONDERS ERFOLGREICHE",
        NULL, NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 828, 16, 16, { 0 }, "THOSE WHO CAN ACHIEVE", "ABENTEURER JA NOCH EINE", NULL,
        NULL, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 846, 16, 16, { 0 }, "A PERFECT RECORD?", "UEBERRASCHUNG...", NULL, NULL, 0, 0,
        30, 6, 6, 0,
    },
};
#else
YoshiEntry D_802F5804[YOSHI_ENTRIES] = {
    { 32, 56, 48, 24, 24, { 0 }, "SELECT OPTION", D_80301B98, 0, 0, 30, 7, 0, 0 },
    { 97, 80, 82, 20, 20, { 0 }, "MORE", D_80301BD4, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 102, 20, 20, { 0 }, "VIEW STATS", D_80301BA8, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 122, 20, 20, { 0 }, "RESTART", D_80301BC0, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 142, 20, 20, { 0 }, "QUIT LEVEL", D_80301BCC, 0, 0, 30, 7, 4, 0 },
    { 32, 56, 48, 24, 24, { 0 }, "SELECT OPTION", D_80301B98, 0, 0, 30, 7, 0, 0 },
    { 113, 80, 82, 20, 20, { 0 }, "CONTINUE", D_80301BA0, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 102, 20, 20, { 0 }, "CONTROL MODE", D_80301BB4, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 122, 20, 20, { 0 }, "MISSION BRIEFING", D_80301BE0, 0, 0, 30, 7, 4, 0 },
    { 97, 80, 142, 20, 20, { 0 }, "MUSIC VOLUME", D_80301BF4, 0, 0, 30, 7, 4, 0 },
    { 36, 32, 6, 20, 20, { 0 }, "COLLISION IMMINENT!", D_80301C04, 0, 0, 30, 17, 4, 0 },
    { 36, 52, 6, 20, 20, { 0 }, "WARNING!", D_80301C14, 0, 0, 30, 17, 4, 0 },
    { 32, 32, 40, 20, 20, { 0 }, "REACTOR MELTDOWN!", D_80301C24, 0, 0, 30, 12, 6, 0 },
    { 36, 44, 64, 20, 20, { 0 }, "ABORTING MISSION", D_80301C30, 0, 0, 30, 18, 4, 0 },
    { 32, 44, 40, 20, 20, { 0 }, "CONGRATULATIONS!", D_80301C40, 0, 0, 30, 16, 6, 0 },
    { 36, 40, 64, 20, 20, { 0 }, "MISSION COMPLETE", D_80301C50, 0, 0, 30, 14, 2, 0 },
    { 160, 64, 42, 28, 30, { 0 }, D_8036B980, D_802F4880, 0, 0, 30, 12, 12, 0 },
    { 240, 112, 210, 24, 24, { 0 }, "PRESS START", NULL, 0, 0, 30, 7, 7, 0 },
    { 0x400, 76, 96, 15, 15, { 0 }, D_8036B9A8, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 216, 96, 15, 15, { 0 }, D_8036B9C8, NULL, 7, 0, 0, 22, 2, 0 },
    { 0x400, 76, 128, 15, 15, { 0 }, D_8036B9E8, NULL, 8, 0, 0, 22, 2, 0 },
    { 0x400, 216, 128, 15, 15, { 0 }, D_8036BA08, NULL, 9, 0, 0, 22, 2, 0 },
    { 0x400, 152, 170, 15, 15, { 0 }, D_8036BA28, NULL, 10, 0, 0, 22, 2, 0 },
    { 0x401, 60, 208, 19, 19, { 0 }, NULL, NULL, 2, 0, 30, 7, 4, 0 },
    { 0x401, 248, 208, 19, 19, { 0 }, NULL, NULL, 5, 0, 30, 7, 4, 0 },
    { 160, 64, 42, 28, 30, { 0 }, D_8036B980, D_802F4880, 0, 0, 30, 12, 12, 0 },
    { 224, 112, 210, 24, 24, { 0 }, "", NULL, 0, 0, 30, 7, 7, 0 },
    { 0x400, 40, 108, 15, 15, { 0 }, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 240, 108, 15, 15, { 0 }, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 40, 108, 15, 15, { 0 }, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x400, 240, 108, 15, 15, { 0 }, NULL, NULL, 6, 0, 0, 22, 2, 0 },
    { 0x1A0, 130, 118, 22, 22, { 0 }, "******MISSION", D_80301C64, 0, 0, 0, 4, 2, 0 },
    { 0x1A0, 130, 140, 22, 22, { 0 }, "FAILED!*****", D_80301C70, 0, 0, 0, 4, 2, 0 },
    { 0x401, 60, 208, 19, 19, { 0 }, NULL, NULL, 2, 0, 30, 7, 4, 0 },
    { 0x401, 248, 208, 19, 19, { 0 }, NULL, NULL, 5, 0, 30, 7, 4, 0 },
    { 36, 40, 6, 20, 20, { 0 }, NULL, NULL, 30, 0, 16, 6, 0, 0 },
    { 32, 0, 17, 20, 20, { 0 }, NULL, NULL, 0, 0, 30, 16, 6, 0 },
    { 36, 24, 39, 20, 20, { 0 }, NULL, NULL, 0, 0, 30, 14, 4, 0 },
    { 32, 0, 22, 24, 24, { 0 }, "SET MUSIC VOLUME", D_803010A8, 0, 0, 0, 7, 7, 0 },
#ifdef VERSION_JP
    { 209, 84, 54, 22, 22, { 0 }, "QUIET", D_803010BC, 0, 0, 30, 7, 4, 0 },
#else
    { 209, 52, 54, 22, 22, { 0 }, "QUIET", D_803010BC, 0, 0, 30, 7, 4, 0 },
#endif
    { 209, 188, 54, 22, 22, { 0 }, "LOUD", D_803010C0, 0, 0, 30, 7, 4, 0 },
    { 32, 53, 20, 26, 26, { 0 }, "**MISSION FAILED!**", D_80301C78, 0, 0, 0, 4, 4, 0 },
    { 36, 40, 6, 20, 20, { 0 }, NULL, NULL, 30, 0, 16, 6, 0, 0 },
    { 0x10A4, 0, -146, 22, 22, { 0 }, "CONGRATULATIONS!!", D_80301C8C, 0, 0, 30, 18, 6, 0 },
    { 0x10A0, 0, 0, 16, 16, { 0 }, "MIRACULOUSLY, THE SHUTTLE", D_80301C9C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 18, 16, 16, { 0 }, "COMPLETES ITS RETURN", D_80301CB0, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 36, 16, 16, { 0 }, "TO EARTH WITHOUT A", D_80301CC8, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 54, 16, 16, { 0 }, "SINGLE CASUALTY.", D_80301CE8, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 72, 16, 16, { 0 }, NULL, D_80301CEC, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 90, 16, 16, { 0 }, NULL, D_80301CF0, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 108, 16, 16, { 0 }, "BLAST CORPS HAS COME", D_80301CF4, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 126, 16, 16, { 0 }, "THROUGH WITH FLYING", D_80301D08, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 144, 16, 16, { 0 }, "COLORS YET AGAIN.", D_80301D24, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 162, 16, 16, { 0 }, NULL, D_80301D38, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 180, 16, 16, { 0 }, NULL, D_80301D3C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 198, 16, 16, { 0 }, NULL, D_80301D40, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 216, 16, 16, { 0 }, "THEIR POPULARITY GIVEN", D_80301D44, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 234, 16, 16, { 0 }, "A FURTHER BOOST, THE", D_80301D48, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 252, 16, 16, { 0 }, "TEAM FIND NEW OFFERS", D_80301D64, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 270, 16, 16, { 0 }, "OF WORK POURING IN -", D_80301D7C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 288, 16, 16, { 0 }, "BUT DECIDE THAT MAYBE,", D_80301D8C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 306, 16, 16, { 0 }, "FOR NOW, IT'S TIME", D_80301DA4, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 324, 16, 16, { 0 }, "FOR A HOLIDAY.", D_80301DC0, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, -186, 16, 16, { 0 }, "BATTERED AND CRIPPLED AFTER", D_80301DC4, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, -168, 16, 16, { 0 }, "ITS LONG VOYAGE, THE LATEST", D_80301DE0, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, -150, 16, 16, { 0 }, "SPACE SHUTTLE IS THROWN OFF", D_80301DF4, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, -132, 16, 16, { 0 }, "COURSE DURING RE-ENTRY AND", D_80301E0C, 0, 0, 30, 6, 6,
        0,
    },
    { 0x10A0, 0, -114, 16, 16, { 0 }, "FORCED INTO DESPERATE", D_80301E20, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -96, 16, 16, { 0 }, "MEASURES.", D_80301E38, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -78, 16, 16, { 0 }, NULL, D_80301E3C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -60, 16, 16, { 0 }, NULL, D_80301E40, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -42, 16, 16, { 0 }, NULL, D_80301E44, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, -24, 16, 16, { 0 }, "A MAJOR CITY IS SEIZED BY", D_80301E48, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, -6, 16, 16, { 0 }, "PANIC WHEN THE RESIDENTS", D_80301E60, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 12, 16, 16, { 0 }, "FIND OUT THAT THEIR HOMES", D_80301E7C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 30, 16, 16, { 0 }, "ARE ABOUT TO BECOME AN", D_80301E94, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 48, 16, 16, { 0 }, "EMERGENCY LANDING STRIP ...", D_80301E98, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 66, 16, 16, { 0 }, NULL, D_80301E9C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 84, 16, 16, { 0 }, NULL, D_80301EA0, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 102, 16, 16, { 0 }, NULL, D_80301EA4, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 120, 16, 16, { 0 }, "TIME IS OF THE ESSENCE AS", D_80301EC0, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 138, 16, 16, { 0 }, "BLAST CORPS RISES ONCE", D_80301ED8, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 156, 16, 16, { 0 }, "MORE TO THE CHALLENGE.", D_80301EF0, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 174, 16, 16, { 0 }, NULL, D_80301EF4, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 192, 16, 16, { 0 }, NULL, D_80301EF8, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 210, 16, 16, { 0 }, NULL, D_80301EFC, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 228, 16, 16, { 0 }, "EVEN AS THE SHUTTLE", D_80301F00, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 246, 16, 16, { 0 }, "BLAZES DOWN THROUGH", D_80301F14, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 264, 16, 16, { 0 }, "THE SKIES, A RUNWAY", D_80301F28, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 282, 16, 16, { 0 }, "MUST BE CLEARED.", D_80301F38, 0, 0, 30, 6, 6, 0 },
    { 176, 0, 36, 20, 20, { 0 }, "SITUATION:", D_8030268C, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 32, 58, 16, 16, { 0 }, "********CARRIER LOCKED ON COURSE********", D_80302698, 0, 0,
        30, 7, 4, 0,
    },
    { 0x2A0, 0, 84, 20, 20, { 0 }, "SOLUTION:", D_803026C0, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 23, 106, 16, 16, { 0 }, "********CLEAR PATH TO GROUND ZERO!************",
        D_803026CC, 0, 0, 30, 7, 4, 0,
    },
    { 176, 0, 36, 20, 20, { 0 }, "AGENTS:", D_803026F8, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 69, 58, 16, 16, { 0 }, "********BLAST CORPS********", D_80302704, 0, 0, 30, 7, 4, 0,
    },
    { 0x2A0, 0, 84, 20, 20, { 0 }, "CHANCES:", D_80302734, 0, 0, 30, 4, 4, 0 },
    {
        0x1A0, 94, 106, 16, 16, { 0 }, "********SLIM*!******************", D_80302748, 0, 0, 30, 7,
        4, 0,
    },
    { 0x1B0, 0, 48, 20, 20, { 0 }, "******MISSION 1:**", D_80302774, 0, 0, 30, 4, 4, 0 },
    { 0x220, 0, 74, 15, 15, { 0 }, "CLEAR PATH FOR CARRIER", D_80302790, 0, 0, 30, 7, 4, 0 },
    { 0x220, 0, 91, 15, 15, { 0 }, "ON EACH MAIN LEVEL.", D_803027AC, 0, 0, 30, 7, 4, 0 },
    { 0x1B0, 0, 40, 20, 20, { 0 }, "******MISSION 2:**", D_803027C4, 0, 0, 30, 4, 4, 0 },
    { 0x220, 0, 66, 15, 15, { 0 }, "ACTIVATE ALL RDUS AND", D_803027E0, 0, 0, 30, 7, 4, 0 },
    { 0x220, 0, 83, 15, 15, { 0 }, "DESTROY ALL BUILDINGS", D_80302800, 0, 0, 30, 7, 4, 0 },
    { 0x220, 0, 100, 15, 15, { 0 }, "TO EARN SECOND GOLD.", D_80302820, 0, 0, 30, 7, 4, 0 },
    { 0x1B0, 0, 40, 20, 20, { 0 }, "******MISSION 3:**", D_80302840, 0, 0, 30, 4, 4, 0 },
    {
        0x220, 0, 66, 14, 15, { 0 }, "AFTER COMPLETING MAIN LEVELS,", D_8030285C, 0, 0, 30, 7, 4,
        0,
    },
    { 0x220, 0, 83, 14, 15, { 0 }, "FIND ALL 6 SCIENTISTS TO", D_80302878, 0, 0, 30, 7, 4, 0 },
    {
        0x220, 0, 100, 14, 15, { 0 }, "ENSURE A CONTROLLED DETONATION.", D_8030288C, 0, 0, 30, 7,
        4, 0,
    },
    { 0x1B0, 0, 48, 20, 20, { 0 }, "******MISSION 4:**", D_803028AC, 0, 0, 30, 4, 4, 0 },
    { 0x220, 0, 74, 15, 15, { 0 }, "ACHIEVE GOLD ON ALL LEVELS", D_803028C8, 0, 0, 30, 7, 4, 0 },
    { 0x220, 0, 91, 15, 15, { 0 }, "TO COMMENCE TIME ATTACK.", D_803028E8, 0, 0, 30, 7, 4, 0 },
    { 176, 0, 16, 15, 15, { 0 }, "BLAST CORPS : LEADERS IN", D_80301F3C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 32, 15, 15, { 0 }, "THE FIELD OF HEAVY DUTY", D_80301F40, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 48, 15, 15, { 0 }, "DEMOLITION THROUGH A", D_80301F54, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 64, 15, 15, { 0 }, "COMBINATION OF SKILL,", D_80301F6C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 80, 15, 15, { 0 }, "EXPERIENCE AND CUTTING-", D_80301F8C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 96, 15, 15, { 0 }, "EDGE TECHNOLOGY.", D_80301FA8, 0, 0, 30, 7, 6, 0 },
    { 176, 0, 16, 15, 15, { 0 }, "SINCE ITS BIRTH THE COMPANY", D_80301FAC, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 32, 15, 15, { 0 }, "HAS APPLIED ITS UNIQUE TALENTS", D_80301FB0, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 48, 15, 15, { 0 }, "TO THE PROBLEM OF URBAN DECAY,", D_80301FD0, 0, 0, 30, 7, 6, 0,
    },
    { 160, 0, 64, 15, 15, { 0 }, "RENOVATING AND REVITALIZING", D_80301FE8, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 80, 15, 15, { 0 }, "CITIES FROM ONE END OF THE", D_80302000, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 96, 15, 15, { 0 }, "COUNTRY TO THE OTHER.", D_80302004, 0, 0, 30, 7, 6, 0 },
    { 176, 0, 16, 15, 15, { 0 }, "A FAR CRY FROM THE SENSELESS", D_80302008, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 32, 15, 15, { 0 }, "WARFARE AMIDST WHICH THE", D_8030200C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 48, 15, 15, { 0 }, "SEEDS OF THE PROJECT WERE", D_8030202C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 64, 15, 15, { 0 }, "SOWN, IN THE HEAVY VEHICLE", D_8030204C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 80, 15, 15, { 0 }, "DEVELOPMENT BAY AT THE", D_80302068, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 96, 15, 15, { 0 }, "MILITARY BASE CALLED RAFTERS.", D_80302088, 0, 0, 30, 7, 6, 0,
    },
    { 176, 0, 16, 15, 15, { 0 }, "WHILE DEMONSTRATING A GREAT", D_8030208C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 32, 15, 15, { 0 }, "NATURAL FLAIR, THE FOUNDING", D_80302090, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 48, 15, 15, { 0 }, "MEMBERS OF THE TEAM - AMBER,", D_803020AC, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 64, 15, 15, { 0 }, "CLARK, WESLEY AND SPIKE - WERE", D_803020CC, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 80, 15, 15, { 0 }, "NEVER HAPPY WITH THE ULTIMATE", D_803020E4, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 96, 15, 15, { 0 }, "PURPOSE OF THEIR MACHINES ...", D_803020E8, 0, 0, 30, 7, 6, 0,
    },
    { 176, 0, 16, 15, 15, { 0 }, "SO WHEN WESLEY WAS CRUELLY", D_803020EC, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 32, 15, 15, { 0 }, "REJECTED FOLLOWING THE FIELD", D_80302104, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 48, 15, 15, { 0 }, "ACCIDENT THAT LEFT HIM", D_80302124, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 64, 15, 15, { 0 }, "DISABLED, HIS FRIENDS FINALLY", D_80302140, 0, 0, 30, 7, 6, 0,
    },
    { 160, 0, 80, 15, 15, { 0 }, "REBELLED AND LED THE", D_8030215C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 96, 15, 15, { 0 }, "INFAMOUS RAFTERS WALKOUT.", D_80302174, 0, 0, 30, 7, 6, 0 },
    { 176, 0, 16, 15, 15, { 0 }, "BLAST CORPS CAME INTO BEING", D_80302178, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 32, 15, 15, { 0 }, "SOON AFTER. THAT WAS FIVE", D_8030217C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 48, 15, 15, { 0 }, "YEARS AGO. BUT NOW, IN", D_80302194, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 64, 15, 15, { 0 }, "THE PRESENT DAY, WORLD PEACE", D_803021B0, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 80, 15, 15, { 0 }, "IS SHATTERED AS MANKIND FACES", D_803021CC, 0, 0, 30, 7, 6, 0,
    },
    { 160, 0, 96, 15, 15, { 0 }, "CRISIS ON A WORLDWIDE SCALE.", D_803021EC, 0, 0, 30, 7, 6, 0 },
    { 176, 0, 16, 15, 15, { 0 }, "A PAIR OF DEFECTIVE NUCLEAR", D_803021F0, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 32, 15, 15, { 0 }, "MISSILES, EN ROUTE TO A SAFE", D_8030220C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 48, 15, 15, { 0 }, "DETONATION SITE, HAVE BEGUN", D_8030222C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 64, 15, 15, { 0 }, "TO LEAK. BADLY DAMAGED, THE", D_8030224C, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 80, 15, 15, { 0 }, "CARRIER AUTOMATICALLY LOCKS", D_80302264, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 96, 15, 15, { 0 }, "ONTO THE MOST DIRECT ROUTE.", D_80302280, 0, 0, 30, 7, 6, 0 },
    { 176, 0, 16, 15, 15, { 0 }, "BAD MEMORIES RESURFACE FOR", D_80302284, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 32, 15, 15, { 0 }, "THE BLAST CORPS TEAM WHEN,", D_80302288, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 48, 15, 15, { 0 }, "SUMMONED TO THEIR NATION'S", D_803022A8, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 64, 15, 15, { 0 }, "DEFENSE, THEY FIND OUT WHERE", D_803022C8, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 80, 15, 15, { 0 }, "THE WARHEADS ORIGINATED. A", D_803022E0, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 96, 15, 15, { 0 }, "CERTAIN NEARBY MILITARY BASE.", D_803022F4, 0, 0, 30, 7, 6, 0,
    },
    {
        176, 0, 16, 15, 15, { 0 }, "THE FLOOD OF RADIATION PREVENTS", D_803022F8, 0, 0, 30, 7, 6,
        0,
    },
    { 160, 0, 32, 15, 15, { 0 }, "ANYONE GETTING CLOSE TO THE", D_803022FC, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 48, 15, 15, { 0 }, "RUNAWAY CARRIER, AND PEOPLE IN", D_80302310, 0, 0, 30, 7, 6, 0,
    },
    { 160, 0, 64, 15, 15, { 0 }, "THE KNOW FEAR THAT EVEN THE", D_80302328, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 80, 15, 15, { 0 }, "SLIGHTEST JOLT COULD TRIGGER", D_80302340, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 96, 15, 15, { 0 }, "A CATASTROPHIC EXPLOSION.", D_8030235C, 0, 0, 30, 7, 6, 0 },
    {
        176, 0, 16, 15, 15, { 0 }, "STANDING AS THE WORLD'S FINAL", D_80302360, 0, 0, 30, 7, 6, 0,
    },
    { 160, 0, 32, 15, 15, { 0 }, "HOPE, BLAST CORPS MUST CLEAR", D_80302364, 0, 0, 30, 7, 6, 0 },
    {
        160, 0, 48, 15, 15, { 0 }, "THE WAY TO GROUND ZERO, GATHER", D_80302380, 0, 0, 30, 7, 6, 0,
    },
    {
        160, 0, 64, 15, 15, { 0 }, "A TEAM OF SIX ELITE SCIENTISTS", D_803023A0, 0, 0, 30, 7, 6, 0,
    },
    { 160, 0, 80, 15, 15, { 0 }, "AND ULTIMATELY COUNTER THE", D_803023C0, 0, 0, 30, 7, 6, 0 },
    { 160, 0, 96, 15, 15, { 0 }, "THREAT OF NUCLEAR WINTER.", D_803023DC, 0, 0, 30, 7, 6, 0 },
    { 176, 0, 88, 16, 16, { 0 }, "EVEN AS THE CARRIER", D_803023E0, 0, 0, 30, 6, 6, 0 },
    { 160, 0, 106, 16, 16, { 0 }, "TRUNDLES TOWARDS GROUND", D_803023E4, 0, 0, 30, 6, 6, 0 },
    { 160, 0, 124, 16, 16, { 0 }, "ZERO, YOU ARE DOING", D_803023FC, 0, 0, 30, 6, 6, 0 },
    { 160, 0, 142, 16, 16, { 0 }, "EVERYTHING IN YOUR POWER", D_80302410, 0, 0, 30, 6, 6, 0 },
    { 160, 0, 160, 16, 16, { 0 }, "TO GET THE ASSEMBLED", D_80302424, 0, 0, 30, 6, 6, 0 },
    { 160, 0, 178, 16, 16, { 0 }, "SCIENTISTS THERE FIRST ...", D_8030243C, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, -188, 16, 16, { 0 }, "MERCIFULLY, THE SCIENTISTS", D_80302440, 0, 0, 30, 6, 6,
        0,
    },
    { 0x10A0, 0, -170, 16, 16, { 0 }, "ARE ABLE TO SET UP A", D_80302460, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, -152, 16, 16, { 0 }, "PROPERLY CONTROLLED", D_8030247C, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, -134, 16, 16, { 0 }, "DETONATION ... AND FINALLY,", D_80302494, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, -116, 16, 16, { 0 }, "AS THE SMOKE FADES, THE WORLD", D_803024AC, 0, 0, 30, 6,
        6, 0,
    },
    {
        0x10A0, 0, -98, 16, 16, { 0 }, "CAN LET OUT A SIGH OF RELIEF.", D_803024CC, 0, 0, 30, 6, 6,
        0,
    },
    { 0x10A0, 0, -26, 16, 16, { 0 }, "THE DEVASTATION LEFT IN", D_803024D0, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, -8, 16, 16, { 0 }, "YOUR WAKE IS NOTHING COMPARED", D_803024F0, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, 10, 16, 16, { 0 }, "TO WHAT WOULD HAVE HAPPENED", D_80302504, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 28, 16, 16, { 0 }, "IF BLAST CORPS HAD FAILED", D_8030251C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 46, 16, 16, { 0 }, "AT THE LAST.", D_80302534, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 82, 16, 16, { 0 }, "REBUILDING BEGINS IMMEDIATELY.", D_80302538, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, 154, 16, 16, { 0 }, "WITH CATASTROPHE AVERTED, THE", D_80302550, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, 172, 16, 16, { 0 }, "TEAM MEMBERS BECOME NATIONAL", D_80302554, 0, 0, 30, 6, 6,
        0,
    },
    {
        0x10A0, 0, 190, 16, 16, { 0 }, "HEROES, THEIR SUCCESS AND", D_8030256C, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 208, 16, 16, { 0 }, "SATISFACTION ASSURED FOR", D_80302584, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 226, 16, 16, { 0 }, "THE FORESEEABLE FUTURE.", D_80302588, 0, 0, 30, 6, 6, 0 },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**RDUS COLLECTED!**********", D_803025B8, 0, 0, 30, 18, 4, 0,
    },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**SURVIVORS FREE!**********", D_803025DC, 0, 0, 30, 18, 4, 0,
    },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**BUILDINGS DESTROYED!**********", D_80302600, 0, 0, 30, 18,
        4, 0,
    },
    {
        0x120, 44, 6, 20, 20, { 0 }, "**LEVEL COMPLETE!**********", D_8030258C, 0, 0, 30, 18, 4, 0,
    },
    { 0x120, 44, 6, 20, 20, { 0 }, "**PATH CLEARED!**********", D_8030262C, 0, 0, 30, 18, 4, 0 },
    {
        0x1A0, 24, 20, 24, 24, { 0 }, "********EMERGENCY! ...****************", D_80302650, 0, 0,
        30, 4, 4, 0,
    },
    { 160, 0, 48, 20, 20, { 0 }, "YOU MUST COMPLETELY", D_80302908, 0, 0, 30, 7, 4, 0 },
    { 160, 0, 70, 20, 20, { 0 }, "REMOVE ALL OBSTACLES", D_80302918, 0, 0, 30, 7, 4, 0 },
    { 160, 0, 92, 20, 20, { 0 }, "FROM THE DANGER ZONE!", D_80302930, 0, 0, 30, 7, 4, 0 },
    { 164, 32, 6, 20, 20, { 0 }, "DANGER ZONE!", D_80302934, 0, 0, 30, 17, 4, 0 },
    { 0x480, -32, 38, 15, 15, { 0 }, NULL, NULL, 16, 0, 0, 22, 2, 0 },
    { 0x1020, 0, -128, 16, 16, { 0 }, "THIS IS AN RDU,", D_80302940, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -110, 16, 16, { 0 }, "TRIGGERED REMOTELY", D_80302954, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -92, 16, 16, { 0 }, "AS YOU DRIVE BY.", D_80302968, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -74, 16, 16, { 0 }, NULL, D_80302974, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -56, 16, 16, { 0 }, "THEY CAN BE USED FOR", D_80302978, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -38, 16, 16, { 0 }, "GUIDANCE AS WELL AS", D_8030298C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -20, 16, 16, { 0 }, "RADIATION DISPERSAL.", D_803029A0, 0, 0, 30, 7, 7, 0 },
    { 0x480, -32, 38, 15, 15, { 0 }, NULL, NULL, 56, 0, 0, 22, 2, 0 },
    { 0x1020, 0, -128, 16, 16, { 0 }, "COMMUNICATION POINTS", D_803029B4, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -110, 16, 16, { 0 }, "ALLOW YOU TO MAKE", D_803029B8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -92, 16, 16, { 0 }, "CONTACT WITH HQ.", D_803029C8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -74, 16, 16, { 0 }, NULL, D_803029CC, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -56, 16, 16, { 0 }, "WHEN ACTIVATED, THEY", D_803029D0, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -38, 16, 16, { 0 }, "BREAK OPEN VALUABLE", D_803029D4, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -20, 16, 16, { 0 }, "NEW TRAINING LEVELS.", D_803029E8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -2, 16, 16, { 0 }, NULL, D_803029EC, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 16, 16, 16, { 0 }, "YOU CAN", D_803029F0, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 34, 16, 16, { 0 }, "ACCESS THESE FROM", D_80302A00, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 52, 16, 16, { 0 }, "THE WORLD SCREEN.", D_80302A14, 0, 0, 30, 7, 7, 0 },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, 62, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "BACKLASH", D_80302A28, 0, 0, 30, 4, 7, 0 },
    { 0x10A0, 0, 20, 16, 16, { 0 }, "DESTROY BUILDINGS", D_80302A3C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 38, 16, 16, { 0 }, "WITH BACKLASH USING", D_80302A50, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 56, 16, 16, { 0 }, "ITS ARMORED REAR.", D_80302A60, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, D_80302A64, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 92, 16, 16, { 0 }, NULL, D_80302A68, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 110, 16, 16, { 0 }, "USE R TO SKID", D_80302A7C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 128, 16, 16, { 0 }, "THE TRUCK WHEN", D_80302A8C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 146, 16, 16, { 0 }, "GOING INTO A TURN.", D_80302A90, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 164, 16, 16, { 0 }, NULL, D_80302AA4, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 182, 16, 16, { 0 }, "AIM FOR AT LEAST", D_80302AB4, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 200, 16, 16, { 0 }, "A SILVER MEDAL", D_80302AB8, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 218, 16, 16, { 0 }, "BEFORE PROGRESSING:", D_80302AD0, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 236, 16, 16, { 0 }, NULL, D_80302AE4, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 254, 16, 16, { 0 }, "THIS TECHNIQUE", D_80302AE8, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 272, 16, 16, { 0 }, "MUST BE MASTERED", D_80302AFC, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 290, 16, 16, { 0 }, "FOR LATER LEVELS.", D_80302B0C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 308, 16, 16, { 0 }, NULL, D_80302B10, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 326, 16, 16, { 0 }, "USE BUMPS TO GET", D_80302B24, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 344, 16, 16, { 0 }, "BACKLASH AIRBORNE AND", D_80302B34, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 362, 16, 16, { 0 }, "CAUSE MAXIMUM DAMAGE.", D_80302B4C, 0, 0, 30, 7, 7, 0 },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, 58, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "SIDESWIPE", D_80302B50, 0, 0, 30, 4, 7, 0 },
    { 0x10A0, 0, 20, 16, 16, { 0 }, "HITS HARDEST AT THE", D_80302B64, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 38, 16, 16, { 0 }, "MAXIMUM EXTENSION", D_80302B7C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 56, 16, 16, { 0 }, "OF ITS SIDE PANELS.", D_80302B94, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, D_80302BA8, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 92, 16, 16, { 0 }, NULL, D_80302BAC, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 110, 16, 16, { 0 }, "FIND BLUE AMMO BOXES", D_80302BB0, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 128, 16, 16, { 0 }, "TO KEEP SIDESWIPE'S", D_80302BB4, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 146, 16, 16, { 0 }, "ATTACK POWER AT FULL.", D_80302BCC, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 164, 16, 16, { 0 }, NULL, D_80302BE4, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 182, 16, 16, { 0 }, "CHARGES REMAINING", D_80302BE8, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 200, 16, 16, { 0 }, "ARE DISPLAYED IN THE", D_80302BEC, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 218, 16, 16, { 0 }, "LOWER LEFT CORNER.", D_80302BF0, 0, 0, 30, 7, 7, 0 },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, 59, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "THUNDERFIST", D_80302BF4, 0, 0, 30, 4, 7, 0 },
    { 0x10A0, 0, 20, 16, 16, { 0 }, "DEMOLISH BUILDINGS", D_80302C08, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 38, 16, 16, { 0 }, "BY DIVING AND", D_80302C20, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 56, 16, 16, { 0 }, "ROLLING INTO THEM.", D_80302C2C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, D_80302C30, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 92, 16, 16, { 0 }, "A WELL-TIMED SERIES", D_80302C34, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 110, 16, 16, { 0 }, "OF ATTACKS CAN CAUSE", D_80302C4C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 128, 16, 16, { 0 }, "INCREDIBLE DAMAGE.", D_80302C64, 0, 0, 30, 7, 7, 0 },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, 60, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "SKYFALL", D_80302C68, 0, 0, 30, 4, 7, 0 },
    { 0x10A0, 0, 20, 16, 16, { 0 }, "MAKE USE OF SKYFALL'S", D_80302C7C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 38, 16, 16, { 0 }, "ARMORED UNDERSIDE", D_80302C94, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 56, 16, 16, { 0 }, "TO CRUSH FROM ABOVE.", D_80302C98, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, D_80302CA8, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 92, 16, 16, { 0 }, "TURBO INTO A DITCH", D_80302CBC, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 110, 16, 16, { 0 }, "WITH L/R TO LAUNCH", D_80302CC0, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 128, 16, 16, { 0 }, "YOURSELF SKYWARDS ...", D_80302CD4, 0, 0, 30, 7, 7, 0 },
    { 0x480, -16, 8, 15, 15, { 0 }, NULL, NULL, 66, 0, 0, 22, 2, 0 },
    { 0x11A0, 0, -52, 24, 24, { 0 }, "J-BOMB", D_80302CE4, 0, 0, 30, 4, 7, 0 },
    { 0x10A0, 0, 20, 16, 16, { 0 }, "USE A TO THRUST", D_80302CF0, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 38, 16, 16, { 0 }, "J-BOMB INTO THE", D_80302CF4, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 56, 16, 16, { 0 }, "AIR OVER A TARGET ...", D_80302D08, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 74, 16, 16, { 0 }, NULL, D_80302D14, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 92, 16, 16, { 0 }, "THEN HIT B TO", D_80302D28, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 110, 16, 16, { 0 }, "DIVE EARTHWARDS", D_80302D2C, 0, 0, 30, 7, 7, 0 },
    { 0x10A0, 0, 128, 16, 16, { 0 }, "FROM A HEIGHT.", D_80302D40, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, -128, 16, 16, { 0 }, "SURVIVORS ESCAPE WHEN", D_80302D58, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -110, 16, 16, { 0 }, "THE WALLS AROUND", D_80302D70, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -92, 16, 16, { 0 }, "THEM ARE DESTROYED.", D_80302D84, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -56, 16, 16, { 0 }, "CUE THE BLAST CORPS", D_80302D9C, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -38, 16, 16, { 0 }, "CHOPPER, SWOOPING IN", D_80302DB4, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -20, 16, 16, { 0 }, "TO PICK THEM UP.", D_80302DCC, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 16, 16, 16, { 0 }, "ONE GOLD COMMENDATION", D_80302DE4, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 34, 16, 16, { 0 }, "IS GIVEN PER LEVEL", D_80302DFC, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 52, 16, 16, { 0 }, "FOR PATH CLEARANCE:", D_80302E14, 0, 0, 30, 7, 7, 0 },
#ifndef VERSION_JP
    { 0x1020, 0, 88, 16, 16, { 0 }, "THE SECOND REQUIRES", D_80302E28, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 106, 16, 16, { 0 }, "ALL SURVIVORS, RDUS", D_80302E2C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 124, 16, 16, { 0 }, "AND TOTAL DESTRUCTION.", D_80302E30, 0, 0, 30, 7, 7, 0 },
#endif
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 13, 0, 0, 22, 2, 0 },
    { 0x1061, 0, -128, 20, 20, { 0 }, "WARNING!", D_80302E34, 0, 0, 30, 4, 4, 0 },
    { 0x1020, 0, -74, 16, 16, { 0 }, "SOMETHING IN THE", D_80302E40, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -56, 16, 16, { 0 }, "CARRIER'S PATH", D_80302E58, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -38, 16, 16, { 0 }, "HAS BEEN MISSED!", D_80302E70, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -2, 16, 16, { 0 }, "KEEP AN EYE", D_80302E74, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 16, 16, 16, { 0 }, "ON THE LOWER", D_80302E88, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 34, 16, 16, { 0 }, "LEFT ARROW ...", D_80302E9C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 70, 16, 16, { 0 }, "IT CHANGES FROM GREEN", D_80302EB0, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 88, 16, 16, { 0 }, "TO RED AS YOU CLOSE", D_80302EC8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 106, 16, 16, { 0 }, "IN ON THE CARRIER.", D_80302EE0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 142, 16, 16, { 0 }, "USE IT WITH THE RADAR", D_80302EF4, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 160, 16, 16, { 0 }, "TO QUICKLY TRACK", D_80302F08, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 178, 16, 16, { 0 }, "DOWN THE PROBLEM:", D_80302F1C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 214, 16, 16, { 0 }, "RED INDICATES THE", D_80302F30, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 232, 16, 16, { 0 }, "CARRIER, BLUE THE NEXT", D_80302F44, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 250, 16, 16, { 0 }, "BUILDING IN ITS PATH.", D_80302F54, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, -74, 16, 16, { 0 }, "CONGRATULATIONS!", D_80302F64, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, -56, 16, 16, { 0 }, "THIS IS ONE OF", D_80302F74, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -38, 16, 16, { 0 }, "THE BONUS VEHICLES.", D_80302F8C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, -2, 16, 16, { 0 }, "THEY ARE", D_80302F90, 30, 0, 7, 7, 0, 0 },
    { 0x1021, 0, 16, 16, 16, { 0 }, "MOST USEFUL IN", D_80302FA0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 34, 16, 16, { 0 }, "TRAINING STAGES:", D_80302FB0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 70, 16, 16, { 0 }, "ACCESS THESE", D_80302FB4, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 88, 16, 16, { 0 }, "VIA THE LEVEL'S", D_80302FCC, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 106, 16, 16, { 0 }, "COMMUNICATION POINTS.", D_80302FE0, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1024, 0, 0, 16, 16, { 0 }, "PATH CLEARED!", D_80302FF0, 0, 0, 30, 18, 18, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "YOUR PRIMARY MISSION", D_80303004, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "HERE IS COMPLETE.", D_80303014, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "THE BLAST CORPS", D_8030302C, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "SEMI ALLOWS YOU", D_80303040, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 108, 16, 16, { 0 }, "TO EXIT THE LEVEL.", D_80303058, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 144, 16, 16, { 0 }, "MOVE BETWEEN", D_80303070, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 162, 16, 16, { 0 }, "VEHICLES WITH", D_80303084, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 180, 16, 16, { 0 }, "THE Z BUTTON.", D_80303098, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 216, 16, 16, { 0 }, "SPARE TIME CAN BE", D_8030309C, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 234, 16, 16, { 0 }, "USED TO FIND RDUS AND", D_803030B4, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 252, 16, 16, { 0 }, "DESTROY BUILDINGS ...", D_803030CC, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 288, 16, 16, { 0 }, "RETURN IF NECESSARY", D_803030E4, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 306, 16, 16, { 0 }, "AFTER CHECKING", D_803030F8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 324, 16, 16, { 0 }, "YOUR PERFORMANCE.", D_8030310C, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "USE Z TO GET OUT", D_80303124, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "OF ONE VEHICLE", D_80303138, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "AND COMMANDEER ANOTHER.", D_8030314C, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "THE DESTRUCTION OF", D_80303160, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "THIS BUILDING", D_80303174, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "IS ESSENTIAL!", D_80303184, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "FLASHING ARROWS", D_80303188, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "MEAN IT STANDS IN", D_80303198, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 108, 16, 16, { 0 }, "THE CARRIER'S PATH.", D_803031B0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 144, 16, 16, { 0 }, "AS DANGER CLOSES IN,", D_803031C8, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 162, 16, 16, { 0 }, "THE ARROWS CHANGE", D_803031D8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 180, 16, 16, { 0 }, "FROM GREEN TO RED.", D_803031EC, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "THIS IS A PERIPHERY", D_803031FC, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "STRUCTURE : CRUSHING", D_8030320C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "IT IS NOT VITAL.", D_80303220, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "THEN AGAIN, IT'S FUN", D_80303234, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "- AND MIGHT REVEAL", D_80303248, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 108, 16, 16, { 0 }, "A SURPRISE OR TWO ...", D_8030325C, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "LEVELING EVERYTHING", D_80303270, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "HELPS YOU GAIN", D_80303288, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "A COMMENDATION.", D_8030329C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "HOWEVER, IT'S A", D_803032B4, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "SECONDARY OBJECTIVE", D_803032C8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 108, 16, 16, { 0 }, "TO CLEARING THE WAY.", D_803032E0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 144, 16, 16, { 0 }, "CONCENTRATE ON THE", D_803032E4, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 162, 16, 16, { 0 }, "ARROWED BUILDINGS AS", D_803032F8, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 180, 16, 16, { 0 }, "THE CARRIER PASSES ...", D_80303304, 0, 0, 30, 7, 7, 0 },
#ifndef VERSION_JP
    { 0x1020, 0, 216, 16, 16, { 0 }, "PLENTY OF TIME TO", D_80303318, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 234, 16, 16, { 0 }, "COME BACK LATER", D_8030331C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 252, 16, 16, { 0 }, "AND FINISH THE JOB.", D_80303320, 0, 0, 30, 7, 7, 0 },
#endif
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "USE Z TO GET OUT", D_80303324, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "OF ONE VEHICLE", D_80303338, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "AND COMMANDEER ANOTHER.", D_8030334C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "BUT CLEAR A PATH FOR", D_80303360, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "THE CARRIER BEFORE", D_80303378, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 108, 16, 16, { 0 }, "GOING OFF TO EXPLORE!", D_8030338C, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "THE CRANE CAN MOVE", D_8030339C, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "OBJECTS TO PREVIOUSLY", D_803033B0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "INACCESSIBLE PLACES.", D_803033C0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "LOAD IT UP THEN", D_803033D0, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "HEAD FOR THE", D_803033E4, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 108, 16, 16, { 0 }, "CONTROLS IN THE CAB.", D_803033F0, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "COLLECT AMMO BOXES", D_80303400, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "AND YOU CAN BLAST", D_80303410, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "YOUR WAY THROUGH.", D_80303420, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "YOU COULD STOP", D_80303424, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "THE TRAIN AT", D_80303428, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "THIS STATION.", D_80303438, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "WAIT FOR THE SMILEY", D_8030343C, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "BEFORE ATTEMPTING TO", D_80303450, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 108, 16, 16, { 0 }, "LOAD OR UNLOAD.", D_8030345C, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "TNT CRATES CAN BE", D_80303470, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "PUSHED AROUND USING", D_80303488, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "RAMDOZER'S SHOVEL.", D_803034A0, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 72, 16, 16, { 0 }, "BUT THEY WON'T BE", D_803034B8, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 90, 16, 16, { 0 }, "STABLE FOR LONG ...", D_803034D0, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "SELECT START THEN", D_803034DC, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "VIEW STATS TO CHECK", D_803034EC, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "STATUS OF LEVEL.", D_803034FC, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "THE TRAIN CAN HELP", D_80303514, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "TRANSPORT RAMDOZER", D_80303520, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "TO THE STATION.", D_80303534, 0, 0, 30, 7, 7, 0 },
    { 0x480, -36, 38, 15, 15, { 0 }, NULL, NULL, 74, 0, 0, 22, 2, 0 },
    { 0x1020, 0, 0, 16, 16, { 0 }, "PRESSING START WILL", D_80303544, 0, 0, 30, 7, 7, 0 },
    { 0x1021, 0, 18, 16, 16, { 0 }, "ALLOW YOU TO VIEW THE", D_8030355C, 0, 0, 30, 7, 7, 0 },
    { 0x1020, 0, 36, 16, 16, { 0 }, "MISSILE CARRIER'S PATH.", D_80303574, 0, 0, 30, 7, 7, 0 },
    { 160, -36, 34, 22, 22, { 0 }, NULL, NULL, 61, 0, 0, 7, 2, 0 },
    { 32, 0, 60, 19, 19, { 0 }, NULL, NULL, 0, 0, 30, 7, 4, 0 },
    { 209, 48, 84, 29, 29, { 0 }, "NO", D_80301098, 0, 0, 30, 8, 4, 0 },
    { 209, 180, 84, 29, 29, { 0 }, "YES", D_803010A0, 0, 0, 30, 8, 4, 0 },
    { 0x10A4, 0, -146, 22, 22, { 0 }, "CONGRATULATIONS!!", D_80301488, 0, 0, 30, 18, 6, 0 },
    { 0x10A0, 0, 0, 16, 16, { 0 }, "HAVING DEMONSTRATED", D_80301498, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 18, 16, 16, { 0 }, "VERSATILITY AND RELIABILITY", D_803014B0, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 36, 16, 16, { 0 }, "WELL BEYOND THE CALL OF", D_803014C4, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 54, 16, 16, { 0 }, "DUTY, THE BLAST CORPS", D_803014DC, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 72, 16, 16, { 0 }, "TEAM CAN FINALLY TAKE", D_803014FC, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 90, 16, 16, { 0 }, "THAT WELL-DESERVED HOLIDAY.", D_80301518, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 108, 16, 16, { 0 }, NULL, D_80301530, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 126, 16, 16, { 0 }, NULL, D_80301534, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 144, 16, 16, { 0 }, NULL, D_80301538, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 162, 16, 16, { 0 }, "WHEN THEY GET BACK THEY'LL", D_8030153C, 0, 0, 30, 6, 6, 0,
    },
    {
        0x10A0, 0, 180, 16, 16, { 0 }, "FIND THE OFFERS AND DEALS", D_80301540, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 198, 16, 16, { 0 }, "STILL FLOODING IN,", D_80301558, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 216, 16, 16, { 0 }, "KEEPING THEM IN THEIR", D_80301570, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 234, 16, 16, { 0 }, "CHOSEN LINE OF WORK", D_80301588, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 252, 16, 16, { 0 }, "FOR MANY YEARS TO COME ...", D_803015A4, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 270, 16, 16, { 0 }, NULL, D_803015A8, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 288, 16, 16, { 0 }, NULL, D_803015AC, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 306, 16, 16, { 0 }, NULL, D_803015B0, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 324, 16, 16, { 0 }, "MAYBE AT SOME POINT EVEN", D_803015C8, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 342, 16, 16, { 0 }, "LEADING THEM BACK INTO", D_803015E4, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 360, 16, 16, { 0 }, "THE FIELD OF MILITARY", D_80301600, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 378, 16, 16, { 0 }, "OPERATIONS - BUT THIS", D_80301620, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 396, 16, 16, { 0 }, "TIME FOR A CONSIDERABLY", D_80301634, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 414, 16, 16, { 0 }, "NOBLER CAUSE.", D_80301638, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 432, 16, 16, { 0 }, NULL, D_8030163C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 450, 16, 16, { 0 }, NULL, D_80301640, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 468, 16, 16, { 0 }, NULL, D_8030165C, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 486, 16, 16, { 0 }, "ALL THAT, THOUGH, CAN WAIT.", D_80301674, 0, 0, 30, 6, 6,
        0,
    },
    { 0x10A0, 0, 504, 16, 16, { 0 }, "WITH THEIR COUNTRY", D_80301694, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 522, 16, 16, { 0 }, "BREATHING A SIGH OF", D_803016AC, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 540, 16, 16, { 0 }, "RELIEF AND THEIR GOOD", D_803016C4, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 558, 16, 16, { 0 }, "NAME ASSURED FOR LIFE,", D_803016C8, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 576, 16, 16, { 0 }, "THE TEAM CAN REST EASY", D_803016CC, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 594, 16, 16, { 0 }, "FOR A WHILE.", D_803016D0, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 612, 16, 16, { 0 }, NULL, D_803016E4, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 630, 16, 16, { 0 }, NULL, D_80301700, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 648, 16, 16, { 0 }, NULL, D_8030171C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 666, 16, 16, { 0 }, "UNLESS, OF COURSE, THE", D_80301720, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 684, 16, 16, { 0 }, "LURE OF THE GOLD STANDARD", D_80301724, 0, 0, 30, 6, 6, 0,
    },
    { 0x10A0, 0, 702, 16, 16, { 0 }, "PROVES TOO MUCH ...", D_80301728, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 720, 16, 16, { 0 }, NULL, D_80301744, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 738, 16, 16, { 0 }, NULL, D_80301760, 0, 0, 30, 6, 6, 0 },
#ifndef VERSION_JP
    { 0x10A0, 0, 756, 16, 16, { 0 }, NULL, D_8030177C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 774, 16, 16, { 0 }, NULL, D_80301780, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 792, 16, 16, { 0 }, "PERHAPS THERE ARE", D_80301784, 0, 0, 30, 6, 6, 0 },
    {
        0x10A0, 0, 810, 16, 16, { 0 }, "FURTHER CHALLENGES AWAITING", D_80301788, 0, 0, 30, 6, 6,
        0,
    },
    { 0x10A0, 0, 828, 16, 16, { 0 }, "THOSE WHO CAN ACHIEVE", D_8030178C, 0, 0, 30, 6, 6, 0 },
    { 0x10A0, 0, 846, 16, 16, { 0 }, "A PERFECT RECORD ...", D_80301790, 0, 0, 30, 6, 6, 0 },
#endif
};
#endif
YoshiWindow D_802F8BDC[YOSHI_WINDOWS] = {
    { 0x100, 208, 32, 16, -0xEFFFFC7, 0, 0, 5, 27, 28, 29, 0, 0, 0 },
    { 0x100, 208, 32, 16, -0x4EFFFFC7, 0, 5, 5, 0, 28, 29, 0, 0, 0 },
    { 0x100, 32, 32, 36, 0x8000008, 0, 10, 1, 0, 0, 0, 0, 0, 0 },
    { 192, 32, 64, 36, 0x8000008, 0, 11, 1, 31, 0, 0, 0, 0, 0 },
    { 0x100, 128, 32, 56, 11, 2, 12, 2, 0, 0, 0, 0, 0, 0 },
    { 0x110, 128, 24, 56, 25, 2, 14, 2, 0, 0, 0, 0, 0, 0 },
    { 0x160, 0x110, -16, -16, -0x4EBFEA18, 3, 16, 9, 0, 0, 229, 23, 0, 0 },
    { 0x160, 0x110, -16, -16, -0x4EBFEA18, 2, 25, 10, 0, 0, 229, 33, 0, 0 },
    { 192, 32, 64, 192, 0x8000008, 2, 35, 1, 107, 0, 0, 0, 0, 0 },
    { 0x118, 72, 20, 80, 24, 3, 36, 2, 0, 0, 0, 0, 0, 0 },
    { 0x140, 157, 0, 85, -0x4FFFF5D8, 0, 2, 5, 0, 0, 29, 0, 0, 0 },
    { 0x140, 240, 0, 0, 2584, 0, 7, 2, 0, 28, 229, 0, 0, 0 },
    { 0x100, 128, 32, 56, -0x4FFFF788, 0, 9, 4, 0, 0, 29, 0, 0, 0 },
    { 0x130, 96, 8, 72, -0x4EFFFB98, 0, 38, 3, 0, 0, 29, 0, 0, 0 },
    { 0x130, 64, 8, 80, 153, 1, 41, 1, 58, 0, 229, 0, 0, 0 },
    { 0x140, 240, 0, 0, -0x4FFFE018, 0, 13, 9, 0, 0, 229, 0, 0, 0 },
    { 0x140, 240, 0, 0, -0x4FFFE018, 0, 22, 11, 0, 0, 229, 0, 0, 0 },
    { 0x100, 64, 32, 96, 6168, 0, 0, 2, 0, 0, 229, 0, 0, 0 },
    { 0x1C0, 92, -64, 96, -0x4FFFB3D8, 0, 33, 20, 0, 0, 229, 0, 0, 0 },
    { 0x140, 144, 0, 48, 0x10001838, 0, 53, 3, 0, 0, 229, 0, 0, 0 },
    { 192, 32, 64, 124, -0x4FFFF7C8, 2, 56, 1, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x2005D428, 0, 0, 0, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 56, 54, -0x4FF429E0, 0, 0, 0, 0, 0, 229, 0, 0, 0 },
    { 0x100, 32, 32, 192, 0x8000008, 2, 42, 1, 31, 0, 0, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000888, 5, 57, 3, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000888, 5, 60, 3, 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x1104888, 4, 63, FE_COUNT(63, 23), 0, 0, 229, 0, 0, 0 },
    { 0x140, 180, 0, 30, 0x1000888, 5, FE_ENTRY(86), 5, 0, 0, 229, 0, 0, 0 },
    { 0x140, 240, 0, 0, 0x5000A00, 7, FE_ENTRY(176), 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x1104808, 2, FE_ENTRY(91), FE_COUNT(91, 23), 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x1104009, 2, YOSHI_ENTRY(0x1A9), YOSHI_COUNT(0x1A9, 49), 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x1104808, 2, FE_ENTRY(114), 24, 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x1104808, 2, FE_ENTRY(138), FE_COUNT(138, 37), 0, 0, 229, 0, 0, 0 },
#ifdef VERSION_JP
    { 0x190, 64, -40, 88, 2184, 4, FE_ENTRY(196), 1, 0, 0, 229, 0, 0, 0 },
#else
    { 0x140, 64, 0, 88, 2184, 4, FE_ENTRY(196), 1, 0, 0, 229, 0, 0, 0 },
#endif
    { 192, 64, 64, 88, 2688, 7, FE_ENTRY(197), 1, 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x104009, 2, 43, 20, 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x104008, 5, 63, 27, 0, 0, 229, 0, 0, 0 },
    { 0x120, 160, 16, 40, 0x4000088, 9, 90, 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 32, 40, 0x4000088, 6, 94, 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 32, 40, 0x5000088, 5, 98, 3, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 32, 40, 0x5000088, 5, 101, 4, 0, 0, 229, 0, 0, 0 },
    { 0x140, 160, 0, 40, 0x5000088, 5, 105, 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 32, 40, 0x5000088, 5, 109, 3, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 112, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 118, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 124, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 130, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 136, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 142, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 148, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 154, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 160, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x5000008, 7, 166, 6, 0, 0, 229, 0, 0, 0 },
    { 144, 32, 88, 164, -0x6BFFF758, 0, FE_ENTRY(175), 1, 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x4000008, 7, 172, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 0x110, 0, -16, 0x104008, 5, 178, YOSHI_COUNT(178, 17), 0, 0, 229, 0, 0, 0 },
#ifdef VERSION_JP
    { 0x190, 0x12C, -40, -24, -0x6FFDE580, 0, FE_ENTRY(182), 14, 0, 0, 229, 0, 0, 0 },
#else
    { 0x190, 0x12C, -40, -30, -0x6FFDE580, 0, FE_ENTRY(182), 14, 0, 0, 229, 0, 0, 0 },
#endif
    { 0x100, 32, 32, 36, 0x8000088, 3, YOSHI_ENTRY(195), 1, 0, 0, 0, 0, 0, 0 },
    { 0x100, 32, 32, 36, 0x8000088, 3, YOSHI_ENTRY(196), 1, 0, 0, 0, 0, 0, 0 },
    { 0x100, 32, 32, 36, 0x8000088, 3, YOSHI_ENTRY(197), 1, 0, 0, 0, 0, 0, 0 },
    { 0x100, 32, 32, 36, 0x8000088, 3, YOSHI_ENTRY(198), 1, 0, 0, 0, 0, 0, 0 },
    { 0x100, 32, 32, 36, 0x8000088, 3, YOSHI_ENTRY(199), 1, 130, 0, 0, 0, 0, 0 },
    { 0x100, 64, 32, 80, 136, 4, YOSHI_ENTRY(200), 1, 0, 0, 229, 0, 0, 0 },
    { 0x140, 160, 0, 40, 0x4000088, 3, YOSHI_ENTRY(201), 3, 0, 0, 229, 0, 0, 0 },
    { 192, 32, 64, 36, 0x4000008, 0, YOSHI_ENTRY(204), 1, 235, 0, 0, 0, 0, 0 },
    { 0x120, 80, 40, 156, 0x905088, 3, YOSHI_ENTRY(225), 22, 0, 0, 229, 0, 0, 0 },
    { 0x120, 80, 40, 156, 0x905088, 3, YOSHI_ENTRY(247), 14, 0, 0, 229, 0, 0, 0 },
    { 0x120, 80, 40, 156, 0x905088, 3, YOSHI_ENTRY(0x105), 9, 0, 0, 229, 0, 0, 0 },
    { 0x120, 80, 40, 156, 0x905088, 3, YOSHI_ENTRY(0x10E), 9, 0, 0, 229, 0, 0, 0 },
    { 0x120, 80, 40, 156, 0x905088, 3, YOSHI_ENTRY(0x117), 9, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(205), 8, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(213), 12, 0, 94, 229, 0, 0, 0 },
    {
        0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x120), YOSHI_COUNT(0x120, 13), 0, 0, 229, 0,
        0, 0,
    },
    { 0x100, 140, 56, 50, 0x2055029, 0, YOSHI_ENTRY(0x12D), 17, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x13E), 10, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x2055029, 0, YOSHI_ENTRY(0x148), 16, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x158), 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x3055029, 0, YOSHI_ENTRY(0x15C), 10, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x166), 7, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x16D), YOSHI_COUNT(0x16D, 13), 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x17A), 7, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x181), 7, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x188), 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x18C), 7, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x193), 6, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x199), 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x19D), 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 140, 56, 50, 0x1255029, 0, YOSHI_ENTRY(0x1A1), 4, 0, 0, 229, 0, 0, 0 },
    { 0x120, 140, 16, 50, -0x4EFFFB97, 0, YOSHI_ENTRY(0x1A5), 4, 0, 0, 29, 0, 0, 0 },
#ifdef VERSION_JP
    { 0x190, 128, -40, 72, 2072, 0, FE_ENTRY(198), FE_COUNT(198, 6), 0, 0, 229, 0, 0, 0 },
#else
    { 0x140, 168, 0, 64, 2056, 0, FE_ENTRY(198), FE_COUNT(198, 6), 0, 0, 229, 0, 0, 0 },
#endif
    { 0x140, 128, 0, 56, 0x8000800, 0, FE_ENTRY(204), 2, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x8000800, 0, FE_ENTRY(206), 3, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x8000800, 0, FE_ENTRY(209), 3, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, 0x8000800, 0, FE_ENTRY(212), 3, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 32, 40, -0x5AFFFFD8, 0, 98, 3, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 32, 40, -0x5AFFFFD8, 0, 101, 4, 0, 0, 229, 0, 0, 0 },
    { 0x140, 160, 0, 40, -0x5AFFFFD8, 0, 105, 4, 0, 0, 229, 0, 0, 0 },
    { 0x100, 160, 32, 40, -0x5AFFFFD8, 0, 109, 3, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 112, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 118, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 124, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 130, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 136, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 142, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 148, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 154, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 160, 6, 0, 0, 229, 0, 0, 0 },
    { 0x140, 128, 0, 56, -0x5AFFFFD8, 0, 166, 6, 0, 0, 229, 0, 0, 0 },
#ifdef VERSION_EU
    { 0x100, 128, 32, 80, -0x4FFFF3C8, 0, YOSHI_ENTRY(208), 4, 0, 0, 29, 0, 0, 0 },
#endif
};
extern Vtx D_802F97B0[0x10];
#ifdef ASSET_INIT
#include "hd_code/26570/D_802F97B0.inc"
#else
Vtx D_802F97B0[0x10] = { 0 };
#endif
extern Gfx D_802F98B0[0x10];
#ifdef ASSET_INIT
#include "hd_code/26570/D_802F98B0.inc"
#else
Gfx D_802F98B0[0x10] = { 0 };
#endif
s32 D_802F9930 = 1;
#ifndef VERSION_EU
UnkStruct_802F9934 D_802F9934[7] = {
    { 189, "RAFTS", D_8030358C },
    { 104, "GAS PLANTS", D_80303594 },
    { 0, "CONTAINERS", D_803035A0 },
    { 186, "SPHERES", D_803035AC },
    { 188, "SPHERES", D_803035B4 },
    { 192, "BEACONS", D_803035BC },
    { 230, "CRATES", D_803035C8 },
};
#endif

u8 func_8026AD30(s16 arg0) {
    UnkStruct_802F48D0 *sp2C;
    u8 sp2B;
    s32 sp24;
    s32 sp20;
    s32 sp1C;
    s32 sp18;

    sp2B = 0;
    if (!(D_80364A90 & 0x2104)) {
        return 0;
    }
    if (D_80364AF0[D_80364AE8].gameState >= 11) {
        return 0;
    }
    for (sp24 = 0; sp24 < 8 && sp2B == 0; sp24++) {
        sp2C = &D_802F48D0[sp24];
        if (sp2C->unk0 == D_802E8BDC) {
            for (sp20 = 0; sp20 < 16 && sp2B == 0 && sp2C->unk2[sp20] != -1; sp20++) {
                if (sp2C->unk2[sp20] == arg0) {
                    sp1C = D_80364AF0[D_80364AE8].unk54[0x34 + arg0] < D_802F499A[arg0];
                    sp18 = D_802E8BDC == 0;
                    if (D_8036BAA2[arg0] == 0 && (sp18 || sp1C)) {
                        if (sp1C && !sp18) {
                            D_80364AF0[D_80364AE8].unk54[0x34 + arg0]++;
                        }
                        D_8036BAA2[arg0] = 1;
                        func_8026AF6C(arg0 | 0x8000 | 0x2000);
                        sp2B = 1;
                    }
                }
            }
        }
    }
    return sp2B;
}

void func_8026AF6C(u16 arg0) {
    u16 sp1E;
    u16 sp1C;

    sp1E = D_8036BB14 & 0xFF;
    sp1C = arg0 & 0xFF;
    if (D_8036BB14) {
        if (D_8036BB14) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!yoshiDemandV", "yoshi.c", LINE_EU(0x520, 0x52F));
        }
        func_8029A7E4("NEW: %x OLD:%x\n", arg0, D_8036BB14);
    }
    if ((arg0 & 0x4000) && (arg0 != 0x4000)) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "yd==YOSHI_DEMAND_OFF", "yoshi.c", LINE_EU(0x525, 0x534));
    }
    if ((sp1C == 0x1E) || (sp1C == 0x23) || (sp1C == 5) || (sp1C == 0xE)) {
        D_8036BB1A = -1;
    }
    if ((sp1E == 0x1E) || (sp1E == 0x23) || (sp1E == 5) || (sp1E == 0xE)) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "yoshi.c", LINE_EU(0x52C, 0x53B));
        func_8029A7E4("OH MY GOD!\n");
        return;
    }
    if (D_8036BB14) {
        if (D_8036BB14) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!yoshiDemandV", "yoshi.c", LINE_EU(0x533, 0x542));
        }
        func_8029A7E4("GOING FOR NEW: %x OLD:%x\n", arg0, D_8036BB14);
    }
    D_8036BB14 = arg0;
}

u16 func_8026B10C(void) {
    return D_8036BB14;
}

#ifndef TARGET_PC
extern u8 D_8036BAE8[];
#endif
extern s16 D_8036BB0C;
extern s8 D_8036BB0E;
extern u16 D_8036BB16;

void func_8026B118(u8 arg0) {
    YoshiWindow *sp44;
    UnkStruct_802F48D0 *sp40;
    u8 sp3F;
    s32 sp38;
    s32 sp34;
    s32 pad;

    D_8036BB1C = 1;
    D_8036BB16 = 0;
    D_8036BB14 = 0;
    D_8036BB1A = -1;
    D_8036BB18 = -1;
    D_8036BB0C = 0;
    sp44 = NULL;
    D_8036BB0E = 1;
    if (arg0 == 0) {
        for (sp38 = 0; sp38 < YOSHI_WINDOWS; sp38++) {
            sp44 = &D_802F8BDC[sp38];
            if (sp44->unk8 & 0x100) {
                sp44->unk8 |= 0x80;
            }
        }
    }
    D_802F8BDC[6].unkC = 3;
    for (sp38 = 0; sp38 < 18; sp38++) {
        D_8036BAE8[sp38] = 0;
    }
    for (sp38 = 0; sp38 < 75; sp38++) {
        D_802F49F4[sp38].unk2E = -1;
    }
    switch (D_80364A98) {
        case 0x80:
        case 0x8000000:
            D_8020C070[FE_ENTRY(25)].unk14 = 0;
        case 0x40000000:
            sp44 = &D_802F8BDC[D_802F4868[func_8026F92C(D_802E8F94[D_802E8BDC].unk0)]];
            if (D_802E8BF8 != 0) {
                D_8020C070[FE_ENTRY(29)].flags &= ~1;
                D_8020C070[FE_ENTRY(29)].unk14 = 0xB;
            }
            if (D_802E8F94[D_802E8BDC].unk0 == 1) {
                if (func_80297EF8(D_802E8BDC) != 0) {
                    D_8020C070[FE_ENTRY(18)].unk14 = 0x18;
                } else {
                    D_8020C070[FE_ENTRY(18)].unk14 = 0;
                }
            } else if (D_802E8F94[D_802E8BDC].unk0 == 0x20) {
                D_8020C070[FE_ENTRY(23)].flags &= ~0x400;
                D_8020C070[FE_ENTRY(26)].flags &= ~0x400;
                D_8020C070[FE_ENTRY(27)].unk14 = 0;
                D_8020C070[FE_ENTRY(28)].unk14 = 0;
            } else {
                D_8020C070[FE_ENTRY(23)].unk14 = D_802F4878[func_8026F92C(D_802E8F94[D_802E8BDC].unk0)];
                D_8020C070[FE_ENTRY(26)].unk14 = D_802F4878[func_8026F92C(D_802E8F94[D_802E8BDC].unk0)];
                D_8020C070[FE_ENTRY(27)].unk14 = 0;
                D_8020C070[FE_ENTRY(28)].unk14 = 0;
            }
            break;
        case 0x40000000000:
            sp44 = &D_802F8BDC[22];
            break;
        case 0x4000000000000:
            sp44 = &D_802F8BDC[56];
            break;
        case 0x40:
            sp44 = &D_802F8BDC[D_802F4870[func_8026F92C(D_802E8F94[D_802E8BDC].unk0)]];
            if (D_802E8F94[D_802E8BDC].unk0 == 0x20) {
                D_802F5804[YOSHI_ENTRY(27)].flags &= ~0x400;
                D_802F5804[YOSHI_ENTRY(28)].flags &= ~0x400;
                D_802F5804[YOSHI_ENTRY(29)].unk14 = 0;
                D_802F5804[YOSHI_ENTRY(30)].unk14 = 0;
            } else {
                D_802F5804[YOSHI_ENTRY(27)].unk14 = D_802F4878[func_8026F92C(D_802E8F94[D_802E8BDC].unk0)];
                D_802F5804[YOSHI_ENTRY(28)].unk14 = D_802F4878[func_8026F92C(D_802E8F94[D_802E8BDC].unk0)];
                D_802F5804[YOSHI_ENTRY(29)].unk14 = 0;
                D_802F5804[YOSHI_ENTRY(30)].unk14 = 0;
            }
            break;
        case 0x2000:
            if (((D_80364AF0[D_80364AE8].medal[D_802E8BDC] > 0 &&
                  D_80364AF0[D_80364AE8].medal[D_802E8BDC] < 6)
                     ? 1
                     : 0) &&
                D_802E8F94[D_802E8BDC].unk0 == 1) {
                sp44 = &D_802F8BDC[6];
            } else {
                sp44 = NULL;
            }
            break;
        case 0x100000000000:
            switch (D_802E8BDC) {
                case 0x37:
                    sp44 = &D_802F8BDC[65];
                    break;
                case 0x1C:
                    sp44 = &D_802F8BDC[66];
                    break;
                case 0x35:
                    sp44 = &D_802F8BDC[67];
                    break;
                case 7:
                    sp44 = &D_802F8BDC[68];
                    break;
                case 0x13:
                    sp44 = &D_802F8BDC[69];
                    break;
                default:
                    sp44 = NULL;
                    break;
            }
            break;
        default:
            sp44 = NULL;
            break;
    }
    if (sp44 != NULL) {
        func_8026BA7C(sp44);
    }
    if (D_80364A98 & 0x2004) {
        for (sp38 = 0, sp3F = 0; sp38 < 8 && sp3F == 0; sp38++) {
            sp40 = &D_802F48D0[sp38];
            if (sp40->unk0 == D_802E8BDC) {
                for (sp34 = 0; sp34 < 16 && sp40->unk2[sp34] != -1; sp34++) {
                    func_8026BA7C(&D_802F8BDC[sp40->unk2[sp34]]);
                }
            }
        }
    }
}

void func_8026B8F8(void) {
    if ((u32)D_80364AA8 & 0x20) {
        D_8020C070[FE_ENTRY(23)].flags |= 0x400;
        D_8020C070[FE_ENTRY(26)].flags |= 0x400;
        D_802F5804[YOSHI_ENTRY(27)].flags |= 0x400;
        D_802F5804[YOSHI_ENTRY(28)].flags |= 0x400;
        D_8020C070[FE_ENTRY(23)].unk14 = D_8020C070[FE_ENTRY(26)].unk14 = D_802F5804[YOSHI_ENTRY(27)].unk14 = D_802F5804[YOSHI_ENTRY(28)].unk14 = func_8026FA38(NULL, NULL);
        if (D_80364A98 == 0x40) {
            func_8026BA7C(&D_802F8BDC[D_802F4870[func_8026F92C((u32)D_80364AA8)]]);
        } else {
            func_8026BA7C(&D_802F8BDC[D_802F4868[func_8026F92C((u32)D_80364AA8)]]);
        }
    }
}

void func_8026BA7C(YoshiWindow *arg0) {
    YoshiIcon *sp2C;
    s32 sp28;
    u8 sp27;
    YoshiEntry *sp20;

    sp27 = 4;
    func_8026FB50(arg0);
    if (arg0->unk8 & 0x20000) {
        sp27 = 0;
    }
    for (sp28 = arg0->first; sp28 < arg0->first + arg0->count; sp28++) {
        sp20 = &D_8036BB10[sp28];
        if (sp20->flags & 0x400) {
            sp2C = &D_802F49F4[sp20->unk14];
            if (sp2C->unk2E == -1) {
                sp20->unk1A = func_80272C5C(sp2C->unk6, 0, sp2C->unk4, sp2C->unk2C, sp2C->unk2D | sp27, 1.0f);
                D_8036BA98[sp20->unk14] = 0;
            } else {
                sp20->unk1A = sp2C->unk2E;
            }
        }
    }
}

Gfx *func_8026BCE0(Gfx *, FrameBuf *, s32 *);

/* The callers use the result, which on the N64 is whatever is left in v0:
   func_8026BCE0's return value.  The port returns that. */
Gfx *func_8026BBD0(Gfx *arg0, FrameBuf *arg1, s32 *arg2) {
#ifdef TARGET_PC
    Gfx *ret;
#endif
    Gfx *gfx;

    gfx = arg0;
    if (D_8036BB1C == 1 && D_8036BB18 != -1) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!(yoshiState==YOSHI_OFF && currentYoshiWindow!=NO_YOSHI_WINDOW)", "yoshi.c", LINE_EU(0x61F, 0x62E));
    }
    gfx = func_8026BCE0(gfx, arg1, arg2);
    if (D_8036BB1C == 1 && D_8036BB18 != -1) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!(yoshiState==YOSHI_OFF && currentYoshiWindow!=NO_YOSHI_WINDOW)", "yoshi.c", LINE_EU(0x623, 0x632));
    }
#ifdef TARGET_PC
    ret = gfx;
#endif
    gDPPipeSync(gfx++);
    *arg2 += gfx - arg0;
#ifdef TARGET_PC
    return ret;
#endif
}

extern u8 D_802E8BD4;
extern u8 D_803643D6;
extern u8 D_803643DB;
extern u8 D_8036BA48[];
extern f32 D_8036BB08;
extern s16 D_8036BB20;
extern f32 D_8036BB28;
extern f32 D_8036BB2C;
extern s32 D_8036BB30;
extern f32 D_8036BB34;
extern f32 D_8036BB38;
extern u16 D_8036BB3C;
extern u16 D_8036BB3E;
extern u32 D_8036BB40;
extern s32 D_8036BB44;
extern s8 D_80370C11;
extern s8 D_80370C12;
extern s8 D_80370C13;
extern s8 D_80370C14;

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/26570/func_8026BCE0.s")
#else
Gfx *func_8026BCE0(Gfx *arg0, FrameBuf *arg1, s32 *arg2) {
    YoshiWindow *sp14C;
    YoshiEntry *sp148;
    void *sp144;
    s32 sp140;
    Gfx *sp13C;
    u16 sp13A;
    u16 sp138;
    u16 sp136;
    u16 sp134;
    u16 sp132;
    u16 sp130;
    ColorPair *sp12C;
    f32 sp128;
    u16 sp126;
    u16 sp124;
    s32 sp120;
    u8 sp11F;

    sp13C = arg0;
    sp132 = (D_803156C4 - D_8036BB40) * 15;
    sp130 = 0;
    sp128 = 1.0f;
    D_8036BB40 = D_803156C4;
    D_8036BB0C += D_8036BB0E * sp132;
    if (D_8036BB0C >= 0x100) {
        D_8036BB0C = 0xFF;
        D_8036BB0E = -D_8036BB0E;
    }
    if (D_8036BB0C < 0) {
        D_8036BB0C = 0;
        D_8036BB0E = -D_8036BB0E;
    }
    sp12C = &D_802F47B0[16];
    sp12C->g0 = 0xFF - D_8036BB0C;
    sp12C->g1 = D_8036BB0C;
    sp12C = &D_802F47B0[17];
    sp12C->g0 = 0xAA - D_8036BB0C * 2 / 3;
    sp12C->g1 = D_8036BB0C * 2 / 3;
    sp12C = &D_802F47B0[18];
    sp12C->r0 = sp12C->g0 = 0xFF - D_8036BB0C;
    sp12C->r1 = sp12C->g1 = D_8036BB0C;
    sp12C = &D_802F47B0[19];
    sp12C->b0 = sp12C->g0 = 0xFF - D_8036BB0C;
    sp12C->b1 = sp12C->g1 = D_8036BB0C;
    sp12C = &D_802F47B0[20];
    sp12C->b0 = 0xFF - D_8036BB0C;
    sp12C->b1 = D_8036BB0C;
    if (D_80364A90 == 0x200 && D_803643DB != 0 && D_803643D6 != 0) {
        D_8036BB1A = -1;
        if (D_8036BB1C == 4 || D_8036BB1C == 2) {
            func_8029A7E4("putting off!\n");
            func_8026AF6C(0x4000);
        }
    }
    if (D_8036BB18 == -1 && (D_8036BB14 & 0x4000)) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "yoshi.c", LINE_EU(0x65B, 0x66A));
        D_8036BB14 = 0;
        return arg0;
    }
    if (D_8036BB14 != 0) {
        sp126 = D_8036BB14 & 0xFF;
        sp124 = D_8036BB14 & 0x2000;
        sp120 = 0;
        func_8029A7E4("yoshiDemand=%x\n", D_8036BB14);
        if (D_8036BB14 & 0x8000) {
#ifdef VERSION_EU
            /* the German text of window 12's two entries is wider */
            if (sp126 == 12 && D_80366F70_eu == 1) {
                D_8020C070[11].x = 40;
                D_8020C070[12].x = 156;
            }
#endif
            D_8036BB1A = -1;
            if (D_8036BB18 != -1) {
                sp120 = D_802F8BDC[D_8036BB18].unk8 & 0x8000000;
            }
            if (D_8036BB1C == 1 || sp124 != 0 || sp120 != 0) {
                D_8036BB18 = sp126;
                D_8036BB1C = 1;
            } else {
                D_8036BB1A = sp126;
            }
        }
        if (D_8036BB1C != 8) {
            sp130 = 1;
        }
        D_8036BB14 = 0;
    }
    if (D_8036BB18 == -1) {
        D_8036BB18 = D_8036BB1A;
        D_8036BB1A = -1;
        if (D_8036BB18 == -1) {
            return arg0;
        }
        sp130 = 1;
    }
    sp14C = &D_802F8BDC[D_8036BB18];
    func_8026FB50(sp14C);
    if ((sp14C->unk8 & 0x20) && D_8036BB1C == 2) {
        if ((((D_80370C28 & 0x8000) && !(D_80370C2A & 0x8000)) ||
             ((sp14C->unk8 & 0x80000000) && (D_80370C28 & 0x1000) && !(D_80370C2A & 0x1000))) &&
            sp14C->unk1A != 0) {
            sp13A = D_8036BB10[sp14C->unk18].unk16;
            if (sp13A != 0) {
                func_80260650(D_80367738, sp13A, 0);
            }
            if (D_8036BB10[sp14C->unk18].flags & 0x10) {
                D_802E8BD4 = 1;
            }
            D_8036BB16 = sp14C->unk18;
            sp130 = 1;
        }
        if ((D_80370C28 & 0x4000) && !(D_80370C2A & 0x4000)) {
            if ((sp14C->unk8 & 0x20000000) && sp14C->unk1A != 0) {
                func_80260650(D_80367738, 0xDE, 0);
                D_8036BB16 = 0xFFFF;
                sp130 = 1;
                if (sp14C->unk8 & 0x40000000) {
                    D_802E8BD4 = 1;
                }
            } else {
                func_80260650(D_80367738, 0xD0, 0);
            }
        }
    }
    if (sp130 != 0) {
        D_8036BAFC = D_803156C4;
        switch (D_8036BB1C) {
            case 8:
                D_8036BAFC = D_803156C4 - D_8036BB08 * D_8036BB38 * D_8036BB34;
            case 1:
                D_8036BB1C = 4;
                D_8036BB34 = 1.0f;
                if (sp14C->unk8 & 0x18) {
                    D_8036BB08 = FRAMES_F(40.0f, 33.6f);
                } else {
                    D_8036BB08 = FRAMES_F(13.333333f, 11.2f);
                }
                func_8026EF70(sp14C);
                sp13A = sp14C->unk12;
                if (sp13A != 0) {
                    func_80260650(D_80367738, sp13A, 0);
                }
                if (!(sp14C->unk8 & 0x400)) {
                    for (sp138 = sp14C->first; !(D_8036BB10[sp138].flags & 1) && sp138 < sp14C->first + sp14C->count;
                         sp138++) {
                    }
                    sp14C->unk18 = sp138;
                }
                for (sp138 = sp14C->first; sp138 < sp14C->first + sp14C->count; sp138++) {
                    sp148 = &D_8036BB10[sp138];
                    if (sp148->flags & 0x20) {
                        if (sp14C->unk8 & 0x80000) {
                            sp148->x = func_8025B498(sp14C->unk0 / 2, sp148->unk6, ENTRY_TEXT(sp148), sp148->unk10);
                        } else {
                            sp148->x = func_8025B498(sp14C->unk0 / 2, sp148->unk6, ENTRY_TEXT(sp148), sp148->unk10);
                        }
                    }
                }
                if (sp14C->unk8 & 1) {
                    D_802E8BD8 = 1;
                }
                if (sp14C->unk8 & 2) {
                    func_80261570(0.0f);
                }
                if (sp14C->unk8 & 0x100000) {
                    D_8036BB28 = sp14C->unk2;
                } else {
                    D_8036BB28 = sp14C->unk2 / 2 - D_8036BB10[sp14C->unk18].y;
                    if (sp14C->unk8 & 0x40000) {
                        D_8036BB28 -= D_8036BB10[sp14C->unk18].unk8 / 2;
                    }
                }
                D_8036BB2C = D_8036BB28;
                break;
            case 4:
                D_8036BAFC = (D_8036BB38 - sp128) * D_8036BB08 + D_803156C4;
            case 2:
                D_8036BB1C = 8;
                sp13A = sp14C->unk14;
                if (sp13A != 0) {
                    func_80260650(D_80367738, sp13A, 0);
                }
                if (sp14C->unk8 & 0x200000) {
                    D_802E8BD4 = 1;
                }
                if (sp14C->unk8 & 4) {
                    func_80261570(1.0f);
                }
                break;
        }
    }
    switch (D_8036BB1C) {
        case 2:
            if (sp14C->unk8 & 0x100000) {
                sp11F = D_8036BB2C < sp14C->unk2 / 8 - D_8036BB10[sp14C->first + sp14C->count - 1].y;
            } else {
                sp11F = sp14C->unkC != 0 && (D_803156C4 - D_8036BAFC) / (f32)REFRESH_RATE > sp14C->unkC &&
                        (!(sp14C->unk8 & 0x400000) || !(D_8036BB1E != 0));
            }
            if (sp11F != 0) {
                sp13A = sp14C->unk14;
                if (sp13A != 0) {
                    func_80260650(D_80367738, sp13A, 0);
                }
                if (sp14C->unk8 & 0x2000) {
                    func_80261570(0.0f);
                }
                D_8036BB1C = 8;
                D_8036BAFC = D_803156C4;
            }
            break;
        case 4:
            D_8036BB38 = (D_803156C4 - D_8036BAFC) / D_8036BB08;
            if (sp128 < D_8036BB38) {
                D_8036BAFC = D_803156C4;
                D_8036BB1C = 2;
                D_8036BB38 = sp128;
                if (sp14C->unk8 & 0x40) {
                    D_8036BB3C = 0x200;
                    D_8036BB3E = 0x100;
                } else {
                    D_8036BB3C = 0x800;
                    D_8036BB3E = 0x400;
                }
                if (sp14C->unk8 & 0x10000000) {
                    sp14C->unk1A = 1;
                } else {
                    sp14C->unk1A = 0;
                }
                if (func_8026F8A8(sp14C->first, sp14C->count, sp14C->unk18, 1) == sp14C->unk18) {
                    sp14C->unk1A = 1;
                }
            }
            break;
        case 8:
            D_8036BB38 = sp128 - (D_803156C4 - D_8036BAFC) / D_8036BB08;
            if (D_8036BB38 < 0.001) {
                D_8036BB38 = 0.0f;
                D_8036BB1C = 1;
                if (sp14C->unk8 & 0x2000000) {
                    D_802E8BD4 = 1;
                }
                if (sp14C->unk8 & 0x100) {
                    sp14C->unk8 &= ~0x80;
                }
                D_8036BB18 = -1;
                return arg0;
            }
            break;
    }
    if (D_8036BB1C != 1) {
        D_8036BB20 = (func_802574F0(D_8036BB38 * D_8036BB34 / sp128 * 1.57 + 4.71) + 1.0) * 255.0;
    }
    if (D_8036BB1C != 1 && D_8036BB38 * D_8036BB34 > 0.1) {
        sp136 = sp14C->unk0 / 2;
        sp134 = sp14C->unk2 / 2;
        guOrtho(&arg1->mtx[73], -sp14C->unk4 - sp136, -sp14C->unk4 - sp136 + 319, -sp14C->unk6 - sp134 + 239,
                -sp14C->unk6 - sp134, -256.0f, 256.0f, 256.0f);
        gSPMatrix(sp13C++, OS_K0_TO_PHYSICAL(&arg1->mtx[73]), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        if (sp14C->unk8 & 0x10) {
            guRotate(&arg1->mtx[75], 180.0 - D_8036BB38 * D_8036BB34 / sp128 * 180.0, 2.0f, 0.0f, 1.0f);
            gSPMatrix(sp13C++, OS_K0_TO_PHYSICAL(&arg1->mtx[75]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        } else {
            guTranslate(&arg1->mtx[75], 0.0f, 0.0f, 0.0f);
            gSPMatrix(sp13C++, OS_K0_TO_PHYSICAL(&arg1->mtx[75]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        }
        gSPMatrix(sp13C++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPPopMatrix(sp13C++, G_MTX_MODELVIEW);
        if (sp14C->unk8 & 8) {
            guScale(&arg1->mtx[76], sp136 * D_8036BB38 * D_8036BB34 / 1000.0f,
                    sp134 * D_8036BB38 * D_8036BB34 / 1000.0f, 1.0f);
            gSPMatrix(sp13C++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        } else {
            guScale(&arg1->mtx[76], sp136 / 1000.0f, sp134 / 1000.0f, 1.0f);
            gSPMatrix(sp13C++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        }
        gDPPipeSync(sp13C++);
        gDPSetRenderMode(sp13C++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
        gDPSetCombineMode(sp13C++, G_CC_SHADE, G_CC_SHADE);
        gSPClearGeometryMode(sp13C++, 0xFFFFFFFF);
        gSPSetGeometryMode(sp13C++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(sp13C++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
        if (!(sp14C->unk8 & 0x200)) {
            gSPDisplayList(sp13C++, D_802F98B0);
        }
        gSPPopMatrix(sp13C++, G_MTX_MODELVIEW);
        if (sp14C->unk8 & 8) {
            guScale(&arg1->mtx[74], D_8036BB38, D_8036BB38, 1.0f);
            gSPMatrix(sp13C++, OS_K0_TO_PHYSICAL(&arg1->mtx[74]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        }
        {
            s32 spD8;
            s32 spD4;
            u16 spD2;
            u16 spD0;
            s32 spCC;
            s16 spCA;
            ColorPair *spC4;

            if ((sp14C->unk8 & 0x20) && D_8036BB1C == 2) {
                spD8 = 0;
                spD4 = 0;
                if (D_8036BB3C == 0x800) {
                    if (D_80370C12 >= 31 && D_80370C14 < 31) {
                        spD8 = 1;
                    } else {
                        spD8 = 0;
                    }
                } else if (D_80370C11 < -30 && D_80370C13 >= -30) {
                    spD8 = 1;
                } else {
                    spD8 = 0;
                }
                if (D_8036BB3E == 0x400) {
                    if (D_80370C12 < -30 && D_80370C14 >= -30) {
                        spD4 = 1;
                    } else {
                        spD4 = 0;
                    }
                } else if (D_80370C11 >= 31 && D_80370C13 < 31) {
                    spD4 = 1;
                } else {
                    spD4 = 0;
                }
                if (((D_80370C28 & D_8036BB3C) && !(D_80370C2A & D_8036BB3C)) || spD8 != 0) {
                    spD2 = func_8026F82C(sp14C->first, sp14C->unk18, 1);
                    sp13A = sp14C->unk16;
                    if (sp13A != 0) {
                        if (spD2 != sp14C->unk18) {
                            func_80260650(D_80367738, sp13A, 0);
                        } else {
                            func_80260650(D_80367738, 0xD0, 0);
                        }
                    }
                    D_8036BB28 += D_8036BB10[sp14C->unk18].y - D_8036BB10[spD2].y;
                    sp14C->unk18 = spD2;
                } else if ((((sp14C->unk1A != 0 ? 0 : 0x8000) | D_8036BB3E) & D_80370C28 &&
                            !(((sp14C->unk1A != 0 ? 0 : 0x8000) | D_8036BB3E) & D_80370C2A)) ||
                           spD4 != 0) {
                    spD0 = func_8026F8A8(sp14C->first, sp14C->count, sp14C->unk18, 1);
                    if (func_8026F8A8(sp14C->first, sp14C->count, spD0, 1) == spD0) {
                        sp14C->unk1A = 1;
                    }
                    sp13A = sp14C->unk16;
                    if (sp13A != 0) {
                        if (spD0 != sp14C->unk18) {
                            func_80260650(D_80367738, sp13A, 0);
                        } else {
                            func_80260650(D_80367738, 0xD0, 0);
                        }
                    }
                    D_8036BB28 += D_8036BB10[sp14C->unk18].y - D_8036BB10[spD0].y;
                    sp14C->unk18 = spD0;
                    D_8036BAFC = D_803156C4;
                }
            }
            if (sp14C->unk8 & 0x4000) {
                if (sp14C->unk8 & 0x100000) {
                    if (sp14C->unkC == 0 || !((D_803156C4 - D_8036BAFC) / (f32)REFRESH_RATE < sp14C->unkC)) {
                        if (sp14C->unk8 & 0x800000) {
                            D_8036BB2C -= 0.5;
                        } else {
                            D_8036BB2C -= 1.0;
                        }
                    }
                } else {
                    D_8036BB2C += (D_8036BB28 - D_8036BB2C) * 0.1;
                }
                if (sp14C->unk8 & 0x10000) {
                    spCC = 0;
                    if (sp14C->unk8 & 0x40000) {
                        spCA = 0x12;
                    } else {
                        spCA = 0x1C;
                    }
                    if (!(D_80364A90 & 0xC9FD0FE79BFF80B0) || D_8035805C != 0) {
                        D_8036BB44 += D_802F9930;
                    }
                    if (D_802F9930 < 0) {
                        D_8036BB44 += D_802F9930 * 2;
                    }
                    if (D_8036BB44 < 0 || D_8036BB44 >= 8) {
                        D_8036BB44 -= D_802F9930 * 2;
                        D_802F9930 = -D_802F9930;
                    }
                    if (func_8026F8A8(sp14C->first, sp14C->count, sp14C->unk18, 1) != sp14C->unk18) {
                        spC4 = &D_802F47B0[18];
                        spCC = func_80276130(arg1, 0, spCC, -sp136, sp134 - D_8036BB44 - spCA, 16, D_8036BB44 / 2 + 10,
                                             spC4->r0, spC4->g0, spC4->b0, D_8036BB20, spC4->r1, spC4->g1,
                                             spC4->b1, D_8036BB20, spC4->r0, spC4->g0, spC4->b0, D_8036BB20,
                                             spC4->r1, spC4->g1, spC4->b1, D_8036BB20);
                        spCC = func_80276080(arg1, 0, spCC, -3 - sp136, sp134 - D_8036BB44 - spCA + 3, 16,
                                             D_8036BB44 / 2 + 10, 0, 0, 0, D_8036BB20 / 2);
                        sp13C = func_80275DA4(sp13C, 1);
                        gSPVertex(sp13C++, arg1->vtx, 8, 0);
                        gSP1Triangle(sp13C++, 4, 5, 6, 0);
                        gSP1Triangle(sp13C++, 4, 6, 7, 0);
                        gSP1Triangle(sp13C++, 0, 1, 2, 0);
                        gSP1Triangle(sp13C++, 0, 2, 3, 0);
                    }
                    if (func_8026F82C(sp14C->first, sp14C->unk18, 1) != sp14C->unk18) {
                        /* volatile: these colours are reloaded, not reused as in the call above. */
                        volatile ColorPair *spAC;

                        spAC = &D_802F47B0[18];
                        spCC = func_80276130(arg1, 1, spCC, -sp136, D_8036BB44 - sp134 + spCA, 16, D_8036BB44 / 2 + 10,
                                             spAC->r0, spAC->g0, spAC->b0, D_8036BB20, spAC->r1, spAC->g1,
                                             spAC->b1, D_8036BB20, spAC->r0, spAC->g0, spAC->b0, D_8036BB20,
                                             spAC->r1, spAC->g1, spAC->b1, D_8036BB20);
                        spCC = func_80276080(arg1, 1, spCC, -3 - sp136, D_8036BB44 - sp134 + spCA - 3, 16,
                                             D_8036BB44 / 2 + 10, 0, 0, 0, D_8036BB20 / 2);
                        sp13C = func_80275DA4(sp13C, 1);
                        gSPVertex(sp13C++, &arg1->vtx[spCC - 8], 8, 0);
                        gSP1Triangle(sp13C++, 4, 5, 6, 0);
                        gSP1Triangle(sp13C++, 4, 6, 7, 0);
                        gSP1Triangle(sp13C++, 0, 1, 2, 0);
                        gSP1Triangle(sp13C++, 0, 2, 3, 0);
                    }
                }
            } else {
                D_8036BB2C = 0.0f;
            }
        }
        {
            YoshiIcon *sp94;
            s16 sp92;
            s16 sp90;
            u8 sp8F;
            u8 sp8E;
            u8 sp8D;
            u8 sp8C;
            u8 sp8B;

            D_8036BB30 = D_8036BB2C;
            if (sp14C->unk8 & 0x1000) {
                if (sp14C->unk8 & 0x20000) {
                    sp13C = func_80274868(sp13C);
                } else {
                    sp13C = func_80274998(sp13C);
                }
                for (sp138 = sp14C->first; sp138 < sp14C->first + sp14C->count; sp138++) {
                    sp148 = &D_8036BB10[sp138];
                    if (!(sp148->flags & 0x800) && (sp148->flags & 0x400) &&
                        (!(sp148->flags & 0x300) || sp138 <= D_8036BB04)) {
                        sp94 = &D_802F49F4[sp148->unk14];
                        if (sp14C->unk8 & 0x20000) {
                            sp92 = sp14C->unk4;
                        } else {
                            sp92 = -sp136;
                        }
                        if (sp14C->unk8 & 0x20000) {
                            sp90 = sp14C->unk6;
                        } else {
                            sp90 = -sp134;
                        }
                        sp8F = 1;
                        sp8E = sp94->unk25;
                        if ((sp148->flags & 1) && sp138 != sp14C->unk18) {
                            sp8F = 0;
                        }
                        sp8D = D_8036BA48[sp148->unk14];
                        sp8C = D_8036BA48[sp148->unk14] = D_803156C4 * 60 / REFRESH_RATE / sp94->unk26 % sp94->unk1A;
                        if (sp8C != sp8D && (sp8F != 0 || D_8036BA98[sp148->unk14] != 0)) {
                            D_8036BA98[sp148->unk14] = (D_8036BA98[sp148->unk14] + 1) % sp94->unk1A;
                        }
                        sp8B = sp94->unk1B[D_8036BA98[sp148->unk14]];
                        if (sp8B != 0) {
                            if (sp8F != 0) {
                                if (sp138 == sp14C->unk18 && (sp148->flags & 0x40)) {
                                    sp8E |= 8;
                                }
                                sp13C = func_80272ED8(
                                    sp13C, sp148->unk1A + sp8B - 1, sp94->unk0 + sp148->x + sp92,
                                    ((sp148->flags & 0x1000) ? D_8036BB30 : 0) + (sp94->unk2 + sp148->y + sp90),
                                    func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                                     sp148->y + sp94->unk2 - sp134 + D_8036BB30 + 8) *
                                        D_8036BB38 * D_8036BB34,
                                    sp8E, sp94->unk28);
                            } else {
                                sp13C = func_80272ED8(
                                    sp13C, sp148->unk1A + sp8B - 1, sp94->unk0 + sp148->x + sp92,
                                    ((sp148->flags & 0x1000) ? D_8036BB30 : 0) + (sp94->unk2 + sp148->y + sp90),
                                    func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                                     sp148->y + sp94->unk2 - sp134 + D_8036BB30 + 8) *
                                        D_8036BB38 * D_8036BB34 * 0.7,
                                    sp8E & ~1, sp94->unk28);
                            }
                        }
                    }
                }
                if (sp14C->unk8 & 0x20000) {
                    sp13C = func_80274AA4(sp13C);
                } else {
                    sp13C = func_80274B08(sp13C);
                }
            }
        }
        if (D_8036BB18 < 0x62 || D_8036BB18 >= 0x6C || D_80364A90 == 2) {
            for (sp138 = sp14C->first; sp138 < sp14C->first + sp14C->count; sp138++) {
                sp148 = &D_8036BB10[sp138];
                sp140 = 0;
                sp144 = func_8026F004(sp14C, sp138, 0);
                if ((sp148->flags & 0x80) && !(sp148->flags & 0x800)) {
                    if (sp138 == sp14C->unk18) {
                        func_80259DC8(
                            arg1, sp144, sp140, sp148->flags & 8, 0, sp148->x - sp136 - 3,
                            ((sp148->flags & 0x1000) ? D_8036BB30 : 0) + (sp148->y - sp134) + 3, sp148->unk6,
                            sp148->unk8, 1, 0, 0, 0,
                            (D_8036BB20 * D_802F47B0[sp148->unk19].a0) *
                                func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                              sp148->y - sp134 + D_8036BB30) / 65025 / 2,
                            0, 0, 0,
                            (D_8036BB20 * D_802F47B0[sp148->unk19].a0) *
                                func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                              sp148->y - sp134 + D_8036BB30 + sp148->unk8) / 65025 / 2);
                    } else if (!(sp148->flags & 4) || FRAMES(D_803156C4 % 23) < FRAMES(16)) {
                        func_80259DC8(
                            arg1, sp144, sp140, sp148->flags & 8, 0, sp148->x - sp136 - 3,
                            ((sp148->flags & 0x1000) ? D_8036BB30 : 0) + (sp148->y - sp134) + 3, sp148->unk6,
                            sp148->unk8, 1, 0, 0, 0,
                            (D_8036BB20 * D_802F47B0[sp148->unk18].a0) *
                                func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                              sp148->y - sp134 + D_8036BB30) / 65025 / 2,
                            0, 0, 0,
                            (D_8036BB20 * D_802F47B0[sp148->unk18].a0) *
                                func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                              sp148->y - sp134 + D_8036BB30 + sp148->unk8) / 65025 / 2);
                    }
                }
            }
        }
        for (sp138 = sp14C->first; sp138 < sp14C->first + sp14C->count; sp138++) {
            sp148 = &D_8036BB10[sp138];
            sp140 = 0;
            sp144 = func_8026F004(sp14C, sp138, 0);
            if (!(sp148->flags & 0x800)) {
                if (sp138 == sp14C->unk18) {
                    if ((!(sp148->flags & 4) || FRAMES(D_803156C4 % 23) < FRAMES(16)) &&
                        (!(sp148->flags & 0x40) || FRAMES(D_803156C4 % 15) < FRAMES(11))) {
                        func_80259DC8(arg1, sp144, sp140, sp148->flags & 8, 0, sp148->x - sp136,
                                      ((sp148->flags & 0x1000) ? D_8036BB30 : 0) + (sp148->y - sp134), sp148->unk6,
                                      sp148->unk8, 1, D_802F47B0[sp148->unk19].r0, D_802F47B0[sp148->unk19].g0,
                                      D_802F47B0[sp148->unk19].b0,
                                      (D_8036BB20 * D_802F47B0[sp148->unk19].a0) *
                                          func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                                        sp148->y - sp134 + D_8036BB30) /
                                          65025,
                                      D_802F47B0[sp148->unk19].r1, D_802F47B0[sp148->unk19].g1,
                                      D_802F47B0[sp148->unk19].b1,
                                      (D_8036BB20 * D_802F47B0[sp148->unk19].a1) *
                                          func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                                        sp148->y - sp134 + D_8036BB30 + sp148->unk8) / 65025);
                    }
                } else if (!(sp148->flags & 4) || FRAMES(D_803156C4 % 23) < FRAMES(16)) {
                    func_80259DC8(arg1, sp144, sp140, sp148->flags & 8, 0, sp148->x - sp136,
                                  ((sp148->flags & 0x1000) ? D_8036BB30 : 0) + (sp148->y - sp134), sp148->unk6,
                                  sp148->unk8, 1, D_802F47B0[sp148->unk18].r0, D_802F47B0[sp148->unk18].g0,
                                  D_802F47B0[sp148->unk18].b0,
                                  (D_8036BB20 * D_802F47B0[sp148->unk18].a0) *
                                      func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                                    sp148->y - sp134 + D_8036BB30) / 65025,
                                  D_802F47B0[sp148->unk18].r1, D_802F47B0[sp148->unk18].g1,
                                  D_802F47B0[sp148->unk18].b1,
                                  (D_8036BB20 * D_802F47B0[sp148->unk18].a1) *
                                      func_8026F644((UnkStruct_8026F644 *) sp14C, &sp148->flags,
                                                    sp148->y - sp134 + D_8036BB30 + sp148->unk8) / 65025);
                }
            }
        }
        func_80259BD4(&sp13C, arg1);
    }
    return sp13C;
}
#endif

void func_8026EF70(YoshiWindow *arg0) {
    if (arg0->unk8 & 0x80) {
        D_8036BB04 = func_8026F8A8(arg0->first, arg0->count, arg0->first - 1, 0x100);
        D_8036BB06 = 0;
        if (D_8036BB04 + 1 == arg0->first) {
            D_8036BB1E = 0;
        } else {
            D_8036BB1E = 1;
        }
    } else {
        D_8036BB1E = 0;
    }
}

void *func_8026F004(YoshiWindow *arg0, u16 arg1, u8 arg2) {
    YoshiEntry *sp3C;
    u8 sp3B;
    u8 *sp34;
    u16 *sp30;
    u16 sp2E;
    s32 sp28;
    u16 sp26;

    sp3C = &D_8036BB10[arg1];
    if (arg2) {
        sp3B = 1;
    } else {
        sp3B = 0;
    }
    sp34 = (u8 *)ENTRY_TEXT(sp3C);
    sp30 = sp3C->unk10;
    D_8036BB48[0] = D_802E8C98[sp3B];
    switch (D_8036BB1E) {
        case 0:
            if (arg2) {
                return sp30;
            }
            return sp34;
        case 1:
            if (D_8036BB1C == 2) {
                D_8036BB1E = 2;
                D_8036BB00 = D_803156C4;
            }
            break;
        case 2:
            if (arg1 < D_8036BB04) {
                if (arg2) {
                    return sp30;
                }
                return sp34;
            }
            if (arg1 <= D_8036BB04) {
#ifdef VERSION_EU
                if (D_803156C4 - D_8036BB00 >= 4) { /* PAL's 50 Hz */
#else
                if (D_803156C4 - D_8036BB00 >= 5) {
#endif
                    D_8036BB00 = D_803156C4;
                    D_8036BB06++;
                    if (arg2) {
                        sp2E = sp30[D_8036BB06];
                    } else {
                        sp2E = sp34[D_8036BB06];
                    }
                    if (sp2E == D_802E8C90[sp3B]) {
                        func_80260650(D_80367738, 0x91, 0);
                    } else if (sp2E != D_802E8C94[sp3B] && sp2E != D_802E8C98[sp3B] && sp2E != D_802E8C8C[sp3B]) {
                        func_80260650(D_80367738, 0x22, 0);
                    }
                }
                if (arg2) {
                    if (sp30 == NULL) {
                        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "jtext", "./vidiPrint.h", 0x46);
                    }
                } else {
                    if (sp34 == NULL) {
                        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "text", "./vidiPrint.h", 0x46);
                    }
                }
                sp28 = 0;
                if (arg2) {
                    while (sp30[sp28] != D_802E8C98[sp3B]) {
#ifdef TARGET_PC
                        /* unsequenced: IDO stores to the old index */
                        D_8036BB48[sp28] = sp30[sp28];
                        sp28++;
#else
                        D_8036BB48[sp28] = sp30[sp28++];
#endif
                    }
                } else {
                    while (sp34[sp28] != D_802E8C98[sp3B]) {
#ifdef TARGET_PC
                        D_8036BB48[sp28] = sp34[sp28];
                        sp28++;
#else
                        D_8036BB48[sp28++] = sp34[sp28];
#endif
                    }
                }
                D_8036BB48[sp28] = D_802E8C98[sp3B];
                if (D_8036BB48[D_8036BB06] == D_802E8C98[sp3B]) {
                    sp26 = func_8026F8A8(arg0->first, arg0->count, D_8036BB04, 0x100);
                    if (sp26 == D_8036BB04) {
                        D_8036BB1E = 0;
                        if ((arg0->unk8 & 0x400000) && D_8036BB1C == 2) {
                            D_8036BAFC = D_803156C4;
                        }
                    } else {
                        func_80260650(D_80367738, 0x23, 0);
                        D_8036BB04 = sp26;
                    }
                    D_8036BB06 = 0;
                } else {
                    D_8036BB48[D_8036BB06] = D_802E8C98[sp3B];
#ifdef VERSION_EU
                    if (!(sp3C->flags & 0x4000) && D_803156C4 % 8 >= 5) {
#else
                    if (!(sp3C->flags & 0x4000) && D_803156C4 % 10 >= 6) {
#endif
                        D_8036BB48[D_8036BB06] = D_802E8C9C[sp3B];
                        D_8036BB48[D_8036BB06 + 1] = D_802E8C98[sp3B];
                    }
                }
                if (arg2) {
                    return D_8036BB48;
                }
                return func_8025B558(D_8036BB48);
            }
            break;
    }
    if ((sp3C->flags & 0x100) || (sp3C->flags & 0x200)) {
        if (arg2) {
            return D_8036BB48;
        }
        return func_8025B558(D_8036BB48);
    }
    if (arg2) {
        return sp30;
    }
    return sp34;
}

#define ABS(x) ((x) > 0 ? (x) : -(x))
#define F644_VOL0(a, b) (0x200 - (ABS(b) << 9) / ((a)->unk2 / 3))
#define F644_VOL(a, b) (F644_VOL0(a, b) >= 0x100 ? 0xFF : F644_VOL0(a, b))

u8 func_8026F644(UnkStruct_8026F644 *arg0, u16 *arg1, s16 arg2) {
    if (*arg1 & 0x1000) {
        return F644_VOL(arg0, arg2) < 0 ? 0 : F644_VOL(arg0, arg2);
    }
    return 0xFF;
}

u16 func_8026F82C(u16 arg0, u16 arg1, u16 arg2) {
    s32 i;

    for (i = arg1 - 1; i >= arg0; i--) {
        if (D_8036BB10[i].flags & arg2) {
            return i;
        }
    }
    return arg1;
}

u16 func_8026F8A8(u16 arg0, u16 arg1, u16 arg2, u16 arg3) {
    s32 i;

    for (i = arg2 + 1; i < arg0 + arg1; i++) {
        if (D_8036BB10[i].flags & arg3) {
            return i;
        }
    }
    return arg2;
}

s32 func_8026F92C(u64 arg0) {
    s64 i;

    if (arg0 == 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "in", "yoshi.c", LINE_EU(0x8DE, 0x8F5));
    }
    if (arg0 == 0) {
        return -1;
    }
    for (i = 0; !(((u64) 1 << i) & arg0); i++) {
    }
    return i;
}

#ifdef VERSION_EU
/* eu defines it here: its text is in this function's .rodata. */
UnkStruct_802F9934 D_802F9934[7] = {
    { 189, { "RAFTS", "PLATTFORMEN", NULL }, NULL },
    { 104, { "GAS PLANTS", "GASTANKS", NULL }, NULL },
    { 0, { "CONTAINERS", "CONTAINER", NULL }, NULL },
    { 186, { "SPHERES", "KUGELN", NULL }, NULL },
    { 188, { "SPHERES", "KUGELN", NULL }, NULL },
    { 192, { "BEACONS", "LEUCHTFEUER", NULL }, NULL },
    { 230, { "CRATES", "KISTEN", NULL }, NULL },
};

#endif
u8 func_8026FA38(char **arg0, u16 **arg1) {
    s32 i;
    s32 sp18;

    sp18 = 0;
    func_8029A7E4("path builing=%d\n", D_803F7684);
    for (i = 0; i < 7 && sp18 == 0; i++) {
        if (D_802F9934[i].unk0 == D_803F7684) {
            sp18 = i + 0x1A;
        }
    }
    if (sp18 == 0) {
        sp18 = 0x1A;
        i = 1;
    }
    if (arg0 != NULL) {
#ifdef VERSION_EU
        *arg0 = D_802F9934[i - 1].text[D_80366F70_eu];
#else
        *arg0 = D_802F9934[i - 1].unk1;
#endif
    }
    if (arg1 != NULL) {
        *arg1 = D_802F9934[i - 1].unk10;
    }
    return sp18;
}

void func_8026FB50(YoshiWindow *arg0) {
    if (arg0->unk8 & 0x8000) {
        D_8036BB10 = D_8036BB24;
    } else if (arg0->unk8 & 0x800) {
        D_8036BB10 = D_8020C070;
    } else {
        D_8036BB10 = D_802F5804;
    }
}
