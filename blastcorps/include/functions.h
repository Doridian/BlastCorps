#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/*
 * The game's functions that more than one file uses, each declared once,
 * with the type its definition has, so the compiler checks every call
 * against it (IDO and clang both; the files don't declare them
 * themselves).  Grouped by the object that defines them, in us.v11's
 * order; a function whose type differs between versions is #if'd.
 *
 * The handwritten code's functions (Rare's asm, which IDO never sees the
 * definition of) are declared with the types port/engine's C gives them:
 * the port's engine includes this too.  libultra's are ultra64.h's; this
 * has only hd_code's and hd_front_end's own copies of it, under address
 * names.
 *
 * A function only its own file uses is declared there, if at all.
 *
 * The game's structs are named by their tags, so that this needs no other
 * header (and declares no data).
 */

#include "common.h"

struct CollisionTri;
struct FrameBuf;
struct FrameGame;
union Frame;
struct LevelAmmoBox;
struct LevelCollisionTri;
struct LevelHeader;
struct LevelRdu;
struct LevelTntCrate;
struct Rdu;
struct SndBank;
struct SndConfig;
struct SndState;
struct SynConfig;
struct TntCrate;
struct UnkStruct_8020BD30;
struct UnkStruct_8026F644;
struct UnkStruct_803ED460;
struct YoshiEntry;
struct YoshiWindow;
struct huft;

/* init 1A00.c */
void func_80220714(const char *, ...);

/* hd_front_end 00000.c */
s32 func_801E7000(void);
void func_801E7598(void);

/* hd_front_end 1C40.c */
void func_801E8C40(u8);
void func_801E8DCC(u8);
void func_801E8EB8(u8, u8);
void func_801E93DC(u8);
s32 func_801E96F8(void);
Gfx *func_801E9718(Gfx *, union Frame *, s32);
void func_801EA108(u8, u8, u8);
void func_801EA278(void);
void func_801EA4B8(void);
void func_801EA6E8(void);
void func_801EA93C(char *, u16 *, u8, u8, char *);
Gfx *func_801EAA7C(Gfx *, union Frame *, s32 *);
void func_801EC288(u8);
void func_801EC30C(u8);
Gfx *func_801EC49C(Gfx *, s32, s32, u8);
Gfx *func_801EC770(Gfx *, union Frame *, s32 *);
u64 func_801ECA50(u8);
void func_801ECB18(void);
void func_801ECC8C(void);
void func_801ECE9C(void);
void func_801ECF5C(void);
void func_801ED4B8(void);

/* hd_front_end 6790.c */
void func_801ED790(void);
Gfx *func_801ED800(Gfx *, union Frame *, u8, s32 *);

/* hd_front_end 7390.c */
void func_801EE390(void);
void func_801EE398(s32);

/* hd_front_end 7800.c */
u8 func_801EE800(u8 *, u8, u8);
u8 func_801EEDB4(u8, u8, u8);
s8 func_801EF1E0(void);
u8 func_801EF2BC(u16, u8, u8);

/* hd_front_end 8380.c */
void func_801EF380(s32);
void func_801EF4AC(void);

/* hd_front_end 9570.c */
void func_801F0570(void);
Gfx *func_801F1568(void);
Gfx *func_801F2000(void);
Gfx *func_801F2428(void);
Gfx *func_801F2E20(void);

/* hd_front_end C450.c */
Gfx *func_801F3450(Gfx *, u8 *);
s32 func_801F36B0(s32 *, s32 *);
void func_801F374C(struct UnkStruct_8020BD30 *);
Gfx *func_801F3964(Gfx *, u8 *, struct UnkStruct_8020BD30 *, f32);
Gfx *func_801F4110(Gfx *, u8 *, struct UnkStruct_8020BD30 *, f32);
void func_801F4878(Gfx *, u8 *);
void func_801F4C3C(struct UnkStruct_8020BD30 *, f32);

/* hd_front_end DE70.c */
void func_801F4E70(u8);
Gfx *func_801F4FBC(union Frame *, Gfx *);
Gfx *func_801F51C8(union Frame *, Gfx *);
void func_801F55D8(void);

/* hd_front_end E7B0.c */
void func_801F57B0(void);
void func_801F58E8(void);
s32 func_801F6BD0(u8, u64 *);
s32 func_801F6F18(void);
s32 func_801F73FC(void);
#if defined(VERSION_US_V10) || defined(VERSION_US_V11) || defined(VERSION_EU)
void func_801F7410(u8 *);
#elif defined(VERSION_JP)
u16 *func_801F7410(u8 *);
#endif
void func_801F74B0(u8 *);
s32 func_801F75A4(u8 *, s32);
s32 func_801F76E4(u8 *, s32);

/* hd_front_end 10850.c */
void func_801F7850(void);
s32 func_801F7F74(u8);
s32 func_801F7FF4(struct YoshiEntry *, struct YoshiEntry *);
void func_801F803C(void);
s32 func_801F81B4(u8);
void func_801F8228(void);
void func_801F8354(u8);
Gfx *func_801F8440(union Frame *, Gfx *);

/* hd_front_end 11530.c */
void func_801F8530(s32);
void func_801F8980(void);
void func_801FCE74(Vtx *, u8, f32, f32, u8, u8, f32, u8);
void func_801FD484(f32 *, f32 *, f32 *, f32 *, f32 *, f32);
void func_801FDCA4(Vtx *, s32, s32);
void func_801FDE50(void);
void func_801FE018(u8);
Gfx *func_801FE238(Gfx *, u8 *);
Gfx *func_801FE5D0(Gfx *, u8 *);
u8 func_801FE760(u8);

/* hd_front_end 17990.c */
void func_801FE990(void);

/* hd_front_end 196F0.c */
void func_80200714(u8);
Gfx *func_80200BE0(Gfx *, union Frame *, s32 *);

/* hd_front_end 1A240.c */
void func_80201240(s32);
Gfx *func_80201364(union Frame *, Gfx *);

/* hd_front_end 1AE80.c */
s32 func_80201E80(void);

/* hd_front_end 1B100, handwritten (port/engine/1B100.c) */
void func_80202100(s32, void *, u8 **, Gfx **);
void func_802021FC(struct UnkStruct_803ED460 *, u8 *, u8 *);
void func_80202270(void *, u8 **, struct UnkStruct_803ED460 *);
void func_802022EC(struct UnkStruct_803ED460 *, s32, s32, s32, f32, s32, s32);
void func_80202380(s32);
void func_802025D0(u8, u32);

/* hd_front_end 1D2D0.c (src/libultra/io/conteeplongwrite.c) */
s32 func_802042D0(OSMesgQueue *, u8, u8 *, int);

/* hd_front_end 1D410.c (src/libultra/io/conteeplongread.c) */
s32 func_80204410(OSMesgQueue *, u8, u8 *, int);

/* hd_code 00000.c */
void func_802447C0(void);
Gfx *func_8024C404(Gfx *, union Frame *, s32 *);
void func_8024FC2C(Gfx **, u8);
void func_80255DC8(void);
void func_80256A34(u8 *);
void func_80257490(s32 *, s32);
f32 func_802574F0(f32);
f32 func_80257514(f32);

/* hd_code 12D80.c */
Gfx *func_80257540(Gfx *);
Gfx *func_802575F4(Gfx *, void *, void *, s16, s32, s32, s32);

/* hd_code 13A70.c */
void func_80258230(u8, s32, s16, s16);
void func_802582C4(u8, s32, s32, s32, s32, s32, s32, s32);
s32 func_802584BC(u8);
s32 func_80258500(u8);
void func_80258544(void *, s32, s32, s32, f32, Gfx *, void *, void *);
void func_80258B78(Gfx **, union Frame *);

/* hd_code 14B30.c */
void func_802592F0(void);
void func_80259450(void);
void func_802595E0(u8 *, s32, s32, s32 (*)(void *, void *));
void func_80259BD4(Gfx **, union Frame *);
void func_80259C24(Gfx **, union Frame *);
void func_80259CCC(union Frame *, u8 *, u16 *, u8, s32, s32, s32, s32, s32, u8, u8, u8, u8, u8);
void func_80259DC8(union Frame *, u8 *, u16 *, u8, s32, s32, s32, s32, s32, u8, u8, u8, u8, u8, u8, u8, u8, u8);

/* hd_code 168B0.c */
void func_8025B070(void);
u8 *func_8025B0B8(u16);
void func_8025B2B8(void);
s32 func_8025B300(u8 *);
s32 func_8025B370(u16 *);
s32 func_8025B3F0(u8 *, u8 *);
#if defined(VERSION_US_V10) || defined(VERSION_US_V11) || defined(VERSION_EU)
s32 func_8025B498(s16, u16, u8 *, u16 *);
#elif defined(VERSION_JP)
s32 func_8025B498(s16, u16, char *, u16 *);
#endif
char *func_8025B558(u16 *);
u16 *func_8025B5D4(u16 *, u16 *, u16 *, s32);
u16 *func_8025B7AC(u8 *);
void func_8025B918(u16 *, u16 *);

/* hd_code 17210.c */
void func_8025B9D0(s32, s32 *);
void func_8025BB38(void);
void func_8025BB50(void);
void func_8025BBE8(u16, s8, s8);
void func_8025BD98(void);
void func_8025BEF8(void);

/* hd_code 17A70.c (src/gzip_unzip.inc.c) */
void func_8025C230(u8 **, u8 **, struct huft *);

/* hd_code 17E10.c */
void func_8025C5D0(void);
Gfx *func_8025C878(Gfx *, union Frame *, u8, s32 *);
void func_8025D184(void);
void func_8025E2CC(Gfx **, union Frame *, s32);
void func_8025E67C(Gfx **, union Frame *, u8);

/* hd_code 1A630.c */
void func_8025EDF0(struct SndConfig *);
u8 func_80260634(struct SndState *);
struct SndState *func_80260650(struct SndBank *, s16, struct SndState **);
void func_802608C8(struct SndState *);
void func_802609D0(void);
void func_802609F0(void);
void func_80260A10(void);
void func_80260A30(u8);
void func_80260AB8(struct SndState *, s16, s32);
void func_80260B40(u8, u16);

/* hd_code 1C460.c */
void func_80260C20(u8, f32);
void func_80260D7C(f32);
f32 func_80260DF0(void);
void func_80260DFC(void);
void func_80260E2C(void);
void func_80260E80(void);
void func_80260EE0(u8);
void func_8026101C(void);
void func_80261040(void);
void func_80261068(void);
void func_802611F0(void);
void func_80261284(void);
void func_802613C8(void);
void func_80261528(void);
void func_80261570(f32);
void func_80261588(void);
void func_802619D0(u32);
u8 func_80261A44(u64);
void func_80261E9C(u64);
void func_80261FB0(u8);
void func_80262008(u8, f32);
s32 func_8026205C(s32);

/* hd_code 1D990.c */
void func_80262150(u8);
void func_802621DC(u8);
void func_80262238(u8);
void func_80262320(u8);
void func_80262BF4(void);
s32 func_8026394C(s16, s16, s16, s16, s16, s16);
Gfx *func_802639B4(Gfx *, union Frame *, s32 *);
void func_8026420C(void);
Gfx *func_80264264(union Frame *, Gfx *);
void func_80264A34(char *, u16, s32);
void func_80264AEC(void);
u8 func_80264BA4(u8);

/* hd_code 20460.c */
void func_80264C20(u8 *);
void func_80264CB4(s16, s16, s16, s16, u8, s32);
void func_8026510C(void);
void func_802661EC(void);
void func_80266248(Gfx **, union Frame *);

/* hd_code 22EE0.c */
void func_802676A0(struct SynConfig *, OSPri);
void func_80267A74(void);

/* hd_code 23C20.c */
void func_802683E0(void);
void func_80268664(s32);
void func_802688C4(s32);
s32 func_80268EE8(s32);
void func_80268F54(void);
void func_80269258(void);
void func_8026A2E8(f32, f32 *);
void func_8026A378(s32, char *);
void func_8026A454(s16, s16, s16, s16, s16, Mtx *);
void func_8026A5CC(u64 *, u64 *, s32);
s32 func_8026A610(s32, s32, s32, s32);
s32 func_8026A6F0(s32, s32, s32, s32, s32, s32);
s32 func_8026A828(s32, s32);
void func_8026A8BC(void);
s32 func_8026A8E0(s32, s32);
void func_8026A974(void);
void func_8026A988(void);
void func_8026A9B4(void);

/* hd_code 26570.c */
u8 func_8026AD30(s16);
void func_8026AF6C(u16);
u16 func_8026B10C(void);
void func_8026B118(u8);
void func_8026B8F8(void);
void func_8026BA7C(struct YoshiWindow *);
Gfx *func_8026BBD0(Gfx *, union Frame *, s32 *);
void func_8026EF70(struct YoshiWindow *);
void *func_8026F004(struct YoshiWindow *, u16, u8);
u8 func_8026F644(struct UnkStruct_8026F644 *, u16 *, s16);
u16 func_8026F82C(u16, u16, u16);
u16 func_8026F8A8(u16, u16, u16, u16);
s32 func_8026F92C(u64);
u8 func_8026FA38(char **, u16 **);
void func_8026FB50(struct YoshiWindow *);

/* hd_code 2B3F0.c */
void func_8026FBB0(struct LevelRdu *, struct LevelRdu *);
u8 func_8026FE6C(s32);
void func_8026FE8C(s32);
void func_8026FEC4(void);
void func_802701A8(Gfx **, union Frame *);
s32 func_80270A54(struct Rdu *);
void func_80270AE0(u8 *);

/* hd_code 2D810.c */
Gfx *func_80271FD0(Gfx *, union Frame *, u16, s16, s16, s32 *);
void func_802729F0(u16, u16);

/* hd_code 2E490.c */
void func_80272C50(void);
u8 func_80272C5C(u16 *, u16 *, u8, u8, u8, f32);
Gfx *func_80272ED8(Gfx *, u8, s16, s16, u8, u8, f32);
Gfx *func_80274868(Gfx *);
Gfx *func_80274998(Gfx *);
Gfx *func_80274AA4(Gfx *);
Gfx *func_80274B08(Gfx *);
void func_80274B40(Gfx **, union Frame *, u8, s16, s16);

/* hd_code 30430.c */
Gfx *func_80274BF0(union Frame *, Gfx *);
void func_80275270(u64, f32);
void func_80275390(u64);
s32 func_802753C0(void);
s32 func_802753F8(void);

/* hd_code 30C70.c */
void func_80275430(void);
void func_80275478(union Frame *, Gfx **, u8);
Gfx *func_80275DA4(Gfx *, u8);
s32 func_80276080(union Frame *, u8, s32, s32, s32, s32, s32, u8, u8, u8, u8);
s32 func_80276130(union Frame *, u8, s32, s32, s32, s32, s32, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8);
void func_8027690C(union Frame *, f32, f32, f32, s16 *, s16 *, Mtx *, Mtx *, Mtx *, f32);
void func_80276E50(Gfx **, union Frame *, u8, s32, s32, s32);

/* hd_code 32E00.c */
void func_802775C0(void);
void func_80277620(s32);
void func_80277EDC(u8, u8, s32, u8);
void func_80278318(void);
void func_80278324(Gfx **, union Frame *, u8);

/* hd_code 34430.c */
void func_80278BF0(Gfx *, Gfx *, Gfx **);
void func_80278E3C(void);
void func_80278EB0(s32, f32, s32);
void func_802794A4(void);
void func_802794E4(void);
u8 func_802794F0(void);
void func_80279514(s32, s32, s32, s32, s32, s32);
void func_80279778(s32, s32, s32, s32, s32, s32, Gfx *, void *, void *, s32);
void func_80279EE8(Gfx **, union Frame *, u8);

/* hd_code 37530.c */
void func_8027BE4C(void);
void func_8027BE7C(u8, s32, s16, s16, s16, s16, s32, s32, s16, u8, u8, u8, u8);
void func_8027C4C8(Gfx **, union Frame *);

/* hd_code 39050.c */
void func_8027D810(s32);
void func_8027E344(s32);
void func_8027E9B8(u8);
u8 func_8027EED8(s16, s16, s16 *);
void func_8027F1F8(Gfx **, u8, u8);
void func_802807D8(u8);
void func_80280F34(Gfx **, u8);
void func_80281A70(s32);
void func_80281CE4(void);
void func_80281E44(Gfx **);
void func_802821D0(void);
void func_80282224(Gfx **, u8);
void func_80282728(void);
void func_8028273C(Gfx **, u8);

/* hd_code 3E4C0.c */
void func_80282C80(Gfx **, union Frame *, s32, s32, s32, s32, s32, s32);
void func_8028376C(Gfx **, union Frame *, u8, s32, s32, s32, s32);

/* hd_code 405F0.c */
void func_80284DB0(void);
void func_80284E54(Gfx *, s32, u8, u8, s32, u8);
void func_80285110(u32);

/* hd_code 409D0.c */
void func_80285190(void);
u32 func_802852EC(void);
u8 func_80285814(void);
void func_80285A78(u8 *, u8 *);
void func_80285AB0(u8);
s32 func_80285B10(u8);
void func_80285B68(s32);
void func_80285CA0(void);
void func_80285CC0(void);
void func_80285EF4(s32);
s32 func_80286038(u16);
u16 func_8028604C(u32);
u8 func_80286090(s32);

/* hd_code 41930.c */
void func_802860F0(void);
void func_802862DC(void);
void func_80286330(void);
u8 func_8028653C(void);

/* hd_code 42240.c */
void func_80286A00(void);
void func_80286C60(Gfx **, union Frame *, u8, u8);
void func_802873AC(void);
void func_80287530(Gfx **, union Frame *, s32, u8);
void func_80287AE4(void);
void func_80287C68(Gfx **, union Frame *, s32, u8);

/* hd_code 43A60.c */
void func_80288220(void);
s32 func_80288284(u8, s32, s32, s32, s32);
void func_802886A0(void);
void func_80288DF0(Gfx **, u8);

/* hd_code 45BB0.c */
u8 func_8028A370(void);
void func_8028A3E4(void);
void func_8028A42C(void);
void func_8028A470(void);
void func_8028AE88(void);
void func_8028B240(void);

/* hd_code 46C20.c */
void func_8028B3E0(void);
void func_8028B4C4(u32, u8 *, u32 *, u8, u8, u8);

/* hd_code 46F60.c */
void func_8028B720(void);
void func_8028B734(s8 *, s8 *, u8);
f32 func_8028BBF4(s16, s16, s16, s16);

/* hd_code 479D0.c */
void func_8028C190(struct LevelAmmoBox *, struct LevelAmmoBox *);
void func_8028C874(u8);
void func_8028CB30(Gfx **, union Frame *);

/* hd_code 48D00.c */
void func_8028D4C0(struct LevelTntCrate *, struct LevelTntCrate *);
void func_8028DA5C(Vtx *, u8);
void func_8028DD64(u8);
struct TntCrate *func_8028DE94(void);
void func_8028DF14(u8);
void func_8028E9E4(Gfx **, union Frame *);
void func_8028F6B4(u8);
void func_8028F794(u8);
void func_8028F93C(void);
void func_8028F994(s32, s32, s32);
void func_8028FAC0(s32, s32, s32, s32);

/* hd_code 4B450.c */
void func_8028FC10(void);
u8 func_8028FCD4(OSMesgQueue *, u8 *);

/* hd_code 4B5E0.c */
void func_8028FDA0(s16 *, s16 *);
void func_802906C0(u8);
void func_802917B0(Gfx **, union Frame *);
void func_80291ED8(u8);
void func_80291FAC(u8);
void func_80292084(void);
void func_802920DC(s32, s32, s32, s32);

/* hd_code 4DA80.c */
void func_80292240(void);
s32 func_80292288(s16, s32, s32, s32, s32, s32, s32, u8, s16);
void func_80292830(void);
void func_80292EB8(Gfx **, union Frame *);

/* hd_code 4EBE0.c */
void func_802933A0(s32, s32, s32, s32, Mtx *, void *, Gfx *, Gfx *, s32, s32, s32, s32);

/* hd_code 50670.c */
void func_80294E30(void);
void func_80294E88(void);
void func_80294EB8(void);
void func_80294F00(void);
void func_80295120(Gfx **, union Frame *);
void func_80295A20(u32);
void func_80295AE0(Gfx *, Gfx *);
void func_80295C70(u8, s32, s32);

/* hd_code 51690.c */
void func_80295E50(void);
Gfx *func_80295EFC(union Frame *, Gfx *, s16, s16, u8);

/* hd_code 52D70.c */
void func_80297530(u8);
u8 func_8029766C(u8, u8 *);
void func_802976E8(Gfx **);
void func_80297804(s32, s32, s32);
void func_80297960(void);

/* hd_code 53220.c */
void func_802979E0(u8);
void func_80297ECC(void);
u8 func_80297EF8(u8);
u8 func_80297F74(void);

/* hd_code 54E30.c */
void func_802995F0(s32);
void func_80299C0C(void);
void func_80299C20(void);
void func_80299E10(s32);
u64 func_80299FE8(u8);

/* hd_code 55970.c */
void func_8029A130(void);
Gfx *func_8029A1A8(union Frame *, Gfx *);

/* hd_code 55D40.c */
void func_8029A500(void);
Gfx *func_8029A518(union Frame *, Gfx *);

/* hd_code 56010.c */
void func_8029A7E4(char *, ...);

/* hd_code 56040, handwritten (port/engine/56040.c) */
s32 func_8029B930(void);
s32 func_8029DBF0(s32);
void func_8029DDC8(void);
void func_8029DEA0(void);
void func_8029E0AC(void);

/* hd_code 5BF40, handwritten (port/engine/5BF40.c) */
void func_802A0700(void);
void func_802A08B4(u32 *, u32 *);
void func_802A0B00(u16, u8 *);
u8 *func_802A0CC8(s32, u8 *);
void func_802A0EE0(u16, u8 *);
void func_802A1040(u16, u8 *, u8 *);

/* hd_code 5CB60, handwritten (port/engine/5CB60.c) */
u32 func_802A1320(void);
void func_802A1674(struct LevelHeader *, u8 *);

/* hd_code 5FD50, handwritten (port/engine/5FD50.c) */
void func_802A45D4(s32);
void func_802A467C(struct LevelHeader *, Gfx *, Vtx *, s32);
void func_802A4CDC(Gfx *, Gfx *, Gfx *, Gfx *, Gfx *);

/* hd_code 60D50, handwritten (port/engine/60D50.c) */
void func_802A5510(struct LevelHeader *);
s32 func_802A56C4(void);

/* hd_code 60F60, handwritten (port/engine/60F60.c) */
void func_802A5720(void);
void func_802A57AC(void);
void func_802A5FA8(void);
void func_802A64A4(void);

/* hd_code 62740, handwritten (port/engine/62740.c) */
s32 func_802A6F6C(void);
void func_802AA6D0(s32, s32, s32, s32, s32, s32, s32, s32 *);
void func_802AACD4(s32, s32, s32, s16 *, s16 *);
void func_802AAE1C(s32, s32, s32, s32 *, s32 *);
s32 func_802AB3C0(s32);
s32 func_802AB878(s32);
s32 func_802ABEDC(s32, s32, s32);

/* hd_code 62740, handwritten (port/engine/62740_carry.c) */
void func_802AB478(u8);
void func_802AB670(u8);

/* hd_code 679E0, handwritten (port/engine/679E0.c) */
void func_802AC1A0(s32);
s32 func_802AC4C4(s32, s32, s32, s32, s32, s32, s32, s32);
void func_802AC544(s32, s32, s32);
void func_802AC61C(s32, s32, s32, s32, s32);
s32 func_802ACF3C(s32);

/* hd_code 69014, handwritten (port/engine/69014.c) */
s32 func_802AD7D4(s32);

/* hd_code 69BB0, handwritten (port/engine/69BB0.c) */
void func_802AE860(void);
u8 func_802AE888(s32);
void func_802AEEC8(void);
void func_802AFC28(u8 *);

/* hd_code 6B4A0, handwritten (port/engine/6B4A0.c) */
void func_802AFFD4(void);
u8 func_802B01DC(void);
void func_802B0254(void);
void func_802B02A0(void);
void func_802B03F4(void);
void func_802B0D70(u8 *);

/* hd_code 6C5E0, handwritten (port/engine/6C5E0.c) */
u8 func_802B1150(void);
void func_802B11B8(void);
void func_802B1228(void);
void func_802B152C(void);
void func_802B2988(u8 *);

/* hd_code 6E200, handwritten (port/engine/6E200.c) */
void func_802B2D7C(void);
u8 func_802B2EF8(void);
void func_802B2F54(void);
void func_802B2FA0(void);
void func_802B327C(void);
void func_802B40D4(u8 *);
void func_802B448C(void);
u8 func_802B45FC(void);
void func_802B4658(void);
void func_802B46C4(void);
void func_802B49AC(void);
void func_802B58C8(u8 *);

/* hd_code 71140, handwritten (port/engine/71140.c) */
void func_802B5CD8(void);
u8 func_802B5F04(void);
void func_802B5F60(void);
void func_802B5FAC(void);
void func_802B6294(void);
void func_802B7308(u8 *);

/* hd_code 72B80, handwritten (port/engine/72B80.c) */
void func_802B76AC(void);
u8 func_802B76F8(void);
void func_802B7754(void);
void func_802B77A0(void);
void func_802B7A88(void);
void func_802B8794(void);
void func_802B899C(void);
void func_802B8AE4(void);

/* hd_code 75490, handwritten (port/engine/75490.c) */
void func_802BA148(void);
void func_802BA354(void);
void func_802BB054(void);
u8 func_802BB170(void);
void func_802BB1A0(void);
void func_802BB274(void);

/* hd_code 772A0, handwritten (port/engine/772A0.c) */
void func_802BBDC8(void);
u8 func_802BBE10(void);
void func_802BBE2C(void);
void func_802BBEB8(void);

/* hd_code 77E20, handwritten (port/engine/77E20.c) */
void func_802BC5E0(void);
void func_802BCA2C(void);
void *func_802BCE40(void);
void func_802BD10C(void *);
void func_802BD1F8(Gfx *, Gfx *, Gfx *, Gfx *, Mtx *, Mtx *, Gfx *, Gfx *);
void func_802C0574(void);
s32 func_802C1AA0(void);
u8 func_802C1B1C(void);
s32 func_802C1B9C(void);
void func_802C1DD0(s32);
s16 *func_802C1EE0(s32);
void func_802C1F30(s32, s32, s32, s32, s32);
void func_802C2054(void);

/* hd_code 7F8B0, handwritten (port/engine/7F8B0.c) */
void func_802C4070(u8 **, u8 **, void *, u8);

/* hd_code 80280, handwritten (port/engine/80280.c) */
s32 func_802C4A40(u8 *);
void func_802C4BF0(u8 *);
u32 func_802C4E58(u8 *, u8);
u8 func_802C5508(void);
void func_802C5688(void);
void func_802C5714(void);
void func_802C5860(void);
void func_802C5AFC(void);
void func_802C80A0(u8 *);

/* hd_code 83910, handwritten (port/engine/83910.c) */
void func_802C8AB0(void);
u8 func_802C8AF0(void);
void func_802C8B0C(u8);
void func_802C8BB8(u8);

/* hd_code 853D0, handwritten (port/engine/853D0.c) */
void func_802C9F54(void);
u8 func_802CA140(void);
void func_802CA1AC(void);
void func_802CA4E0(void);
void func_802CB660(u8 *);

/* hd_code 86F60, handwritten (port/engine/86F60.c) */
void func_802CBA94(void);
u8 func_802CBB60(void);
void func_802CBBBC(void);
void func_802CBC08(void);
void func_802CBEF0(void);

/* hd_code 88160, handwritten (port/engine/88160.c) */
void func_802CCC8C(void);
u8 func_802CCCD8(void);
void func_802CCD34(void);
void func_802CCD80(void);
void func_802CD068(void);

/* hd_code 89250, handwritten (port/engine/89250.c) */
void func_802CDA10(s32, s32, s32);
void func_802CDAE8(s16, s16);
s32 func_802CDB70(s16, s16);
u8 func_802CDF94(s16);
s16 func_802CE3B8(s16);
void func_802CE4F0(s32, s32, s32);
void func_802CE5BC(s32, s32, s32, s16, s32, s32);
void func_802CE65C(s32, s32, s16, s16);
s32 func_802CE6F8(s32, s32, s32);

/* hd_code 8A080, handwritten (port/engine/8A080.c) */
void func_802CE840(void);
void func_802CE880(s32, s32, s32, s32, s32);
void func_802CE90C(s32);
s32 func_802CE958(s32);
void func_802CE9A4(void);
void func_802CE9C8(struct LevelCollisionTri *, u8, u8);
void func_802CEA68(struct CollisionTri *, struct CollisionTri *);

/* hd_code 8A2E0, handwritten (port/engine/8A2E0.c) */
Gfx *func_802CEEFC(Gfx *, u8, void *, void *);
void func_802CF1A4(void);
void func_802CF5B0(void);
void func_802CF628(void);

/* hd_code 8AEE0, handwritten (port/engine/8AEE0.c) */
void func_802CFA0C(void);
u8 func_802CFA58(void);
void func_802CFAB4(void);
void func_802CFB00(void);
void func_802CFDE8(void);
u8 func_802D0B90(void);
void func_802D0BF8(void);
void func_802D0C68(void);
void func_802D0F98(void);
void func_802D2524(u8 *);

/* hd_code 8DDB0, handwritten (port/engine/8DDB0.c) */
void func_802D291C(void);

/* hd_code 90390.c (src/libultra/audio/cspgetstate.c) */
s32 func_802D4E10(ALCSPlayer *);

/* hd_code 92F00.c (src/libultra/audio/cspstop.c) */
void func_802D76C0(ALCSPlayer *);

/* hd_code 939F0.c (src/libultra/audio/cspsetseq.c) */
void func_802D81B0(ALCSPlayer *, ALCSeq *);

/* hd_code 93A30.c (src/libultra/audio/cspplay.c) */
void func_802D81F0(ALCSPlayer *);

/* hd_code 95020.c (src/libultra/audio/cspsetbank.c) */
void func_802D97E0(ALCSPlayer *, ALBank *);

/* hd_code 9FE20.c (src/libultra/audio/reverb.c) */
s32 func_802E4C78(void *, s32, void *);

#endif
