#include "common.h"
#include "game/frontend.h"
#include "game/sched.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

typedef struct {
    /* 0x00 */ char *PTR32 unk0[4];   /* (PTR32: jp's func_801E8EB8, still asm, reads them) */
} UnkStruct_80208358; /* size = 0x10 */

#ifdef VERSION_EU
typedef struct {
    /* 0x00 */ char *PTR32 unk0[4][3];   /* eu: per language */
} UnkStruct_80208358_eu; /* size = 0x30 */
#endif

typedef struct {
    /* 0x0 */ u8 unk0[5];
} UnkStruct_8020849C; /* size = 0x5 */

/* 0x1C-byte records; hd_code walks the same array through D_8036BB10. */
void func_801E8DCC(u8);
void func_801E8EB8(u8, u8);
void func_801E93DC(u8);
u16 func_801E9528(void);
void func_801EA268(PlayerInfo *);
Gfx *func_801EC49C(Gfx *, s32, s32, u8);
void func_801ED480(u8 *, u8 *);
u8 func_801EF2BC(u16, u8, u8);
void func_801F8354(u8);
void func_801FE018(s32);
void func_80259BD4(Gfx **, FrameBuf *);
void func_80259CCC(s32, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
void func_80259DC8(s32, char *, u16 *, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
s32 func_8025B300(u8 *);
s32 func_8025B370(u16 *);
s32 func_8025B3F0(u8 *, u8 *);
char *func_8025B558(u16 *);
void func_80260650(SndBank *, s32, s32);
void func_80261570(f32);
void func_80264A34(char *, u16, s32);
void func_8026AF6C(s32);
u8 func_80272C5C(u16 *, u16 *, u8, u8, u8, f32);
Gfx *func_80272ED8(Gfx *, u8, s16, s16, u8, u8, f32);
Gfx *func_80274868(Gfx *);
Gfx *func_80274AA4(Gfx *);
void func_80275270(u64, f32);
s32 func_802753C0(void);
Gfx *func_80275DA4(Gfx *, u8);
s32 func_80276080(FrameBuf *, u8, s32, s32, s32, s32, s32, u8, u8, u8, u8);

#ifdef VERSION_EU
extern u8 D_80366F70_eu; /* the language: 0 English, 1 German, 2 French */
/* eu: an entry's text in that language (text, text2 or text3) */
#define ENTRY_TEXT(e) ((&(e).text)[D_80366F70_eu])
/* eu: a text in English or German (none in French) */
#define LANG_TEXT(en, de) (D_80366F70_eu == 0 ? (en) : D_80366F70_eu == 1 ? (de) : NULL)
#else
#define ENTRY_TEXT(e) ((e).text)
#endif

/* .bss, 0x80215440-0x80215960 (tools/bss_c.py) */
#if defined(VERSION_JP) || defined(VERSION_EU)
s32 D_80215940;
f32 D_80215944;
f32 D_80215948;
#ifdef VERSION_EU
s32 D_80215458;
#endif
f32 D_8021594C;
f32 D_80215950;
#endif
f32 D_80215440;
#ifdef VERSION_JP
s32 D_80215458;
#endif
f32 D_80215444;
f32 D_80215448;
f32 D_8021544C;
f32 D_80215450;
f32 D_80215454;
#if !defined(VERSION_JP) && !defined(VERSION_EU)
s32 D_80215458;
#endif
f32 D_8021545C;
f32 D_80215460;
f32 D_80215464;
f32 D_80215468;
s16 D_8021546C;
#if defined(VERSION_JP) || defined(VERSION_EU)
char D_80215470[0xa];
#else
char D_80215470[0x10];
char D_80215480[3][0x10];
#endif
u8 D_802154B0;
s16 D_802154B2;
s16 D_802154B4;
s16 D_802154B6;
s16 D_802154B8;
#ifdef VERSION_JP
char D_80215480[3][0x10];
#endif
s16 D_802154BA;
u8 D_802154BC;
s16 D_802154BE;
s16 D_802154C0;
#ifdef VERSION_EU
char D_80215480[3][0x10];
#endif
s32 D_802154C4;
s32 D_802154C8;
s32 D_802154CC;
u8 D_802154D0;
s16 D_802154D2;
s16 D_802154D4;
s32 D_802154D8;
s32 D_802154DC;
f32 D_802154E0;
f32 D_802154E4;
s32 D_802154E8;
s32 D_802154EC;
s32 D_802154F0[6];
s32 D_80215508[6];
char D_80215520[5][0x19];
u8 D_802155A0[0x300];
u16 *D_802158A0;
u8 D_802158A4[4];
u16 D_802158A8[1];
u8 D_802158AA[2];
u8 D_802158AC[4];
u8 D_802158B0[0x50];
u8 D_80215900[2];
u8 D_80215902[3];
s32 D_80215908[2];
s16 D_80215910[2];
u8 D_80215914;
u8 D_80215915;
u8 D_80215916;
s16 D_80215918;
s16 D_8021591A;
u16 D_8021591C;
s32 D_80215920;
u8 D_80215924;
char *D_80215928;
s16 D_8021592C;
u8 D_8021592E;
u16 D_80215930[6];
s16 D_8021593C;
#if !defined(VERSION_JP) && !defined(VERSION_EU)
s32 D_80215940;
f32 D_80215944;
f32 D_80215948;
f32 D_8021594C;
f32 D_80215950;
#endif

s32 func_80276130(FrameBuf *, u8, s32, s32, s32, s32, s32, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8,
                  u8, u8, u8, u8);
void func_8029A7E4(char *, ...);

extern Lights2 D_80208470;
extern u16 D_802082D8[];
extern u16 D_802082E4[];
extern u16 D_802082E8[];
extern u16 D_802082EC[];
extern u16 D_802082F8[];
extern u8 D_802082FC[];
extern u8 D_80208314[];
extern u32 D_80208350[];
#ifdef VERSION_EU
extern UnkStruct_80208358_eu D_80208358;
#else
extern UnkStruct_80208358 D_80208358;
#endif
extern UnkStruct_80208358 D_80208368;
extern char *PTR32 D_80208378[];     /* (PTR32: jp's func_801E8EB8 reads it too) */
extern Vtx D_80208380[];
extern Gfx D_80208400[];
extern Lights2 D_80208448;
extern u8 D_80208498[];
extern UnkStruct_8020849C D_8020849C;
extern f32 D_80215440;
extern f32 D_80215444;
extern f32 D_80215448;
extern f32 D_8021544C;
extern f32 D_80215450;
extern f32 D_80215454;
extern s32 D_80215458;
extern f32 D_8021545C;
extern f32 D_80215460;
extern f32 D_80215464;
extern f32 D_80215468;
extern s16 D_8021546C;
extern char D_80215470[];
extern char D_80215480[][0x10];
extern u8 D_802154B0;
extern s16 D_802154B2;
extern s16 D_802154B4;
extern s16 D_802154B6;
extern s16 D_802154B8;
extern s16 D_802154BA;
extern u8 D_802154BC;
extern s16 D_802154BE;
extern s16 D_802154C0;
extern s32 D_802154C4;
extern s32 D_802154C8;
extern s32 D_802154CC;
extern u8 D_802154D0;
extern s16 D_802154D2;
extern s16 D_802154D4;
extern s32 D_802154D8;
extern s32 D_802154DC;
extern f32 D_802154E0;
extern f32 D_802154E4;
extern s32 D_802154E8;
extern s32 D_802154EC;
extern s32 D_802154F0[];
extern s32 D_80215508[];
extern char D_80215520[][0x19];
extern u8 D_802155A0[];
extern u16 *D_802158A0;
extern u16 D_802158A8[];
extern u8 D_80215900[];
extern u8 D_80215902[];
extern s32 D_80215908[];
extern s16 D_80215910[];
extern u8 D_80215914;
extern u8 D_80215915;
extern u8 D_80215916;
extern s16 D_80215918;
extern s16 D_8021591A;
extern u16 D_8021591C;
extern s32 D_80215920;
extern u8 D_80215924;
extern char *D_80215928;
extern s16 D_8021592C;
extern u8 D_8021592E;
extern u16 D_80215930[];
extern s16 D_8021593C;
extern s32 D_80215940;
extern f32 D_80215944;
extern f32 D_80215948;
extern f32 D_8021594C;
extern f32 D_80215950;
extern OSMesgQueue D_80219EF8;
extern OSMesgQueue D_80219F50;
extern u16 D_802E8C94[];
extern u16 D_802E8C98[];
extern char D_803046F8[];
extern char D_80304710[];
extern char D_80304730[];
extern FrameBuf D_803156F8[];
extern u8 D_80365060[];
extern s16 D_8036BB20;
extern s8 D_80370C2C;

void func_801E8C40(u8 arg0) {
    FrameBuf *sp34;
    s32 sp30;

    D_80364AE8 = arg0;
    D_80215915 = func_80272C5C(D_802082F8, NULL, 1, 1, 1, 1.0f);
    D_80215916 = func_80272C5C(D_802082EC, NULL, 5, 1, 0, 1.0f);
    for (sp30 = 0; sp30 < 2; sp30++) {
        sp34 = &D_803156F8[sp30];
        guPerspective(&sp34->mtx[2], &D_8021591C, 45.0f, 4.0f / 3.0f, 10.0f, 1e+04f, 1.0f);
        guLookAt(&sp34->mtx[6], 0.0f, 277.0f, 480.0f, 0.0f, 189.0f, 200.0f, 0.0f, 0.0f, 1.0f);
        guTranslate(&sp34->mtx[23], 0.0f, 88.0f, 0.0f);
    }
    D_8021593C = 0;
    func_801E8DCC(D_80364AE8);
}

/* The ticker's resting speed (eu's PAL frames are longer). */
#ifdef VERSION_EU
#define SCROLL_SPEED 4.0f
#else
#define SCROLL_SPEED 3.0f
#endif

/* The name being entered (D_802158A8): up to 27 characters, 17 in jp. */
#ifdef VERSION_JP
#define NAME_CHARS 0x11
#else
#define NAME_CHARS 0x1B
#endif

void func_801E8DCC(u8 arg0) {
    s32 sp1C;

    func_801E8EB8(arg0, 0);
    for (sp1C = 0; sp1C < NAME_CHARS; sp1C++) {
        D_802158A8[sp1C] = D_802E8C94[D_8021592E];
    }
    D_802158A8[sp1C] = D_802E8C98[D_8021592E];
    D_802154D8 = 0;
    D_802154E8 = 9999;
    for (sp1C = 1; sp1C < 5; sp1C++) {
        D_802154F0[sp1C] = 9999;
    }
    D_802154E4 = SCROLL_SPEED;
}

extern char D_803041DC[];
extern char D_803041EC[];
extern char D_803041FC[];
extern char D_8030420C[];
extern char D_8030421C[];
extern char D_8030422C[];
extern char D_8030423C[];
extern char D_8030424C[];
extern char D_8030425C[];
extern char D_8030426C[];
extern char D_8030427C[];
extern char D_80304288[];
extern char D_8030429C[];
extern char D_803042AC[];
extern char D_803042B8[];
extern char D_803042CC[];
extern char D_803042E0[];
extern char D_803042F0[];
extern char D_80304304[];
extern char D_8030430C[];
extern char D_80304318[];
extern char D_80304328[];
extern char D_80304334[];
extern char D_80304344[];
extern char D_80304358[];
extern char D_80304364[];
extern char D_80304370[];
extern char D_8030437C[];
extern char D_80304388[];
extern char D_80304394[];
extern char D_8030439C[];
extern char D_803043B8[];
extern char D_80304474[];
extern char D_80304544[];
extern char D_80304614[];
#ifdef VERSION_JP
extern u16 D_80301044[];
extern u16 D_803043AC[];
#elif !defined(VERSION_EU)
extern const char D_8020E764[];
extern const char D_8020E768[];
#endif
#ifndef VERSION_EU
extern const char D_8020E770[];
extern const char D_8020E82C[];
extern const char D_8020E8EC[];
extern const char D_8020E9AC[];
#endif
/* The crew's introductions (English). */
#define CREW_TEXT_AMBER ".................... LEADER OF THE ARMY BASE WALKOUT YEARS AGO. AMBER'S SHARP MIND AND BRIGHT, SELFLESS OUTLOOK MAKE HER THE NEAREST THING BLAST CORPS HAS TO A LEADER ...................."
#define CREW_TEXT_CLARK ".................... A GENIUS IN HEAVY VEHICLE DESIGN. WHILE SOMETIMES OVERLY POSSESSIVE OF HIS CREATIONS, CLARK HAS TALENTS VITAL TO BLAST CORPS' SURVIVAL AND SUCCESS ...................."
#define CREW_TEXT_SPIKE ".................... HEAD MECHANIC OF THE BLAST CORPS TEAM. WITH YEARS OF EXPERIENCE AND A GRUFF PRIDE IN HIS WORK, SPIKE ENSURES THAT THE DOZERS ARE BUILT TO PERFECTION .................."
#define CREW_TEXT_WESLEY ".................... A FEARLESS ARMY DAREDEVIL UNTIL HIS DISABLING ACCIDENT. WESLEY'S REJECTION BY HIS SUPERIORS TRIGGERED THE REBELLION THAT LED TO THE RISE OF BLAST CORPS ..............."
/* .data, 0x802081C0-0x802084B0 (tools/data_c.py) */
#ifdef VERSION_EU
/* eu: each rank in English and German (French has none), and no u16 text */
char *D_802081C0[0x1F][4] = {
    { "ROOKIE WRECKER", "CRASH AZUBI" },
    { "TRAINED CRUSHER", "CRASH MEISTER" },
    { "EXPERIENCED RAVAGER", "CRASH PROFI" },
    { "DECORATED DAMAGER", "CRASH LORD" },
    { "PROFESSIONAL RAZER", "PROFI EINSTAMPFER" },
    { "EXPERT DESTROYER", "SOFT DEMOLIERER" },
    { "GIFTED RUINER", "EXTREM DEMOLIERER" },
    { "ACCOMPLISHED CONQUEROR", "CHAOS VERURSACHER" },
    { "MASTER DESPOILER", "CHAOS MEISTER" },
    { "DEMOLITION FANATIC", "ABRISS EXPERTE" },
    { "GRAND ERADICATOR", "RATZFATZ GENERAL" },
    { "HEAVY DUTY WASTER", "ARCHITEKTEN TRAUMA" },
    { "TOTAL PULVERISER", "TASMANISCHER TEUFEL" },
    { "CHAMPION RANSACKER", "DESTRUKTIVER ELIMINATOR" },
    { "MECHANICAL MAESTRO", "ZEMENT RASIERER" },
    { "CHIEF OBLITERATOR", "PSYCHO PLANIERER" },
    { "COMMANDING DESOLATOR", "BETON CHOLERIKER" },
    { "SUPREME DEVASTATOR", "ADMIRAL FATAL" },
    { "ULTIMATE ANNIHILATOR", "DEMOLAGE KUENSTLER" },
    { "LEVELLING LEGEND", "INFERNO KOMMANDEUR" },
    { "DESTRUCTIVE PSYCHOPATH", "CHAOS SCHOEPFER" },
    { "MINDLESS DESECRATOR", "NOTORISCHER BERSERKER" },
    { "HYSTERICAL CLAUSTROPHOBE", "ZENTRAL ZERSTOERER" },
    { "UNCONTROLLABLE MADMAN", "NEUTRONEN SCHLUCKER" },
    { "WORLD CLASS MEGALOMANIAC", "WELTEN ZERSTOERER" },
    { "CAPTAIN OF CARNAGE", "PLANETEN BRECHER" },
    { "SINGLE MINDED CHAOSMONGER", "GENESIS BLOCKER" },
    { "GRAND HIGH SLAUGHTERMASTER", "HYPER PSYCHO" },
    { "LUNATIC LORD OF HAVOC", "HERR DER WALZEN" },
    { "ARMAGEDDON ADEPT", "APOKALYPTISCHER REITER" },
    { "YOU CAN STOP NOW.", "ABSOLUTER MEGAPROFI" },
};
/* eu: the English ranks' numbers, then the German ones' */
u8 D_802082B8[2][0x1f] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1 },
};
#else
char *D_802081C0[0x1f][2] = {
    { "ROOKIE WRECKER", D_803041DC },
    { "TRAINED CRUSHER", D_803041EC },
    { "EXPERIENCED RAVAGER", D_803041FC },
    { "DECORATED DAMAGER", D_8030420C },
    { "PROFESSIONAL RAZER", D_8030421C },
    { "EXPERT DESTROYER", D_8030422C },
    { "GIFTED RUINER", D_8030423C },
    { "ACCOMPLISHED CONQUEROR", D_8030424C },
    { "MASTER DESPOILER", D_8030425C },
    { "DEMOLITION FANATIC", D_8030426C },
    { "GRAND ERADICATOR", D_8030427C },
    { "HEAVY DUTY WASTER", D_80304288 },
    { "TOTAL PULVERISER", D_8030429C },
    { "CHAMPION RANSACKER", D_803042AC },
    { "MECHANICAL MAESTRO", D_803042B8 },
    { "CHIEF OBLITERATOR", D_803042CC },
    { "COMMANDING DESOLATOR", D_803042E0 },
    { "SUPREME DEVASTATOR", D_803042F0 },
    { "ULTIMATE ANNIHILATOR", D_80304304 },
    { "LEVELING LEGEND", D_8030430C },
    { "DESTRUCTIVE PSYCHOPATH", D_80304318 },
    { "MINDLESS DESECRATOR", D_80304328 },
    { "HYSTERICAL CLAUSTROPHOBE", D_80304334 },
    { "UNCONTROLLABLE MADMAN", D_80304344 },
    { "WORLD CLASS MEGALOMANIAC", D_80304358 },
    { "CAPTAIN OF CARNAGE", D_80304364 },
    { "SINGLE MINDED CHAOSMONGER", D_80304370 },
    { "GRAND HIGH SLAUGHTERMASTER", D_8030437C },
    { "LUNATIC LORD OF HAVOC", D_80304388 },
    { "ARMAGEDDON ADEPT", D_80304394 },
    { "YOU CAN STOP NOW.", D_8030439C },
};
u8 D_802082B8[0x20] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 1, 1 };
#endif
u16 D_802082D8[6] = { 0x777, 0x777, 0x776, 0x773 };
u16 D_802082E4[2] = { 0x774, 0x775 };
#ifdef VERSION_JP
u16 D_802082E8[2] = { 0x2DF, 0x2DE };
#else
u16 D_802082E8[2] = { 0x576, 0x575 };
#endif
u16 D_802082EC[6] = { 0x91B, 0x91B, 0x91C, 0x91D, 0x572 };
u16 D_802082F8[2] = { 0x91F };
u8 D_802082FC[0x18] = { 49, 50, 51, 52, 47, 46, 127, 0, 0, 0, 0, 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 47, 127 };
u8 D_80208314[0x3c] = {
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 66, 67, 68, 70, 71, 72, 74, 75, 76, 77, 78, 80, 81, 82, 83, 84, 86, 87, 88, 89, 90, 0,
    0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 12,
};
u32 D_80208350[2] = { 26, 21 };
#ifdef VERSION_EU
/* eu: the second is the language's "GUEST: ", set by func_801E8EB8 */
char *PTR32 D_80208378[2] = { "", "" };
/* eu: each one's text in English, German and French (the English again) */
UnkStruct_80208358_eu D_80208358 = { {
    { CREW_TEXT_AMBER,
      ".................... LEITERIN DES BLAST CORPS TEAMS. AUSGEBILDET IN EINER SPEZIALEINHEIT DES MILITAERS. SPEZIALISTIN FUER PLANUNG UND DURCHFUEHRUNG EXTREM GEFAEHRLICHER MISSIONEN......",
      CREW_TEXT_AMBER },
    { CREW_TEXT_CLARK,
      ".................... SCHOEPFER DER GENIALEN BLAST CORPS VEHIKEL. SEINE DESIGNVORSCHLAEGE HABEN DIE WISSENSCHAFTLER ZU NEUEN, BAHNBRECHENDEN ERFINDUNGEN ANIMIERT......................",
      CREW_TEXT_CLARK },
    { CREW_TEXT_SPIKE,
      ".................... CHEFMECHANIKER DES BLAST CORPS TEAMS. JAHRELANGE ERFAHRUNG UND EIN EINMALIGES GESPUER FUER MECHANISCHE DEFEKTE MACHEN IHN ZU EINEM UNERSETZBAREN MITGLIED DES TEAMS............",
      CREW_TEXT_SPIKE },
    { CREW_TEXT_WESLEY,
      ".................... AUSGEBILDETER EINZELKAEMPFER, DER WEGEN BEFEHLSVERWEIGERUNG AUS DER ARMEE UNEHRENHAFT ENTLASSEN WURDE. SEINE KAMPFERFAHRUNG KANN ER NUN IM BLAST CORPS TEAM EINSETZEN...",
      CREW_TEXT_WESLEY },
} };
UnkStruct_80208358 D_80208368 = { { NULL, NULL, NULL, NULL } };
#else
UnkStruct_80208358 D_80208358 = { { (char *)D_8020E770, (char *)D_8020E82C, (char *)D_8020E8EC, (char *)D_8020E9AC } };
UnkStruct_80208358 D_80208368 = { { D_803043B8, D_80304474, D_80304544, D_80304614 } };
#endif
#ifdef VERSION_JP
/* jp's are u16 text (0x0FFE-terminated), as are its own below */
char *PTR32 D_80208378[2] = { (char *)D_80301044, (char *)D_803043AC };
u16 D_802082D0_jp[10] = { 0x1002, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x1002, 0x1003, 0xFFE };
u16 D_802082E4_jp[10] = { 0x1003, 0x1002, 0x1002, 0x1004, 0x1002, 0x3C, 0x3C, 0x1002, 0xFFE };
u16 D_802082F8_jp[10] = { 0x1003, 0x1002, 0x1002, 0x1004, 0x1002, 0x3C, 0x3C, 0x3C, 0x1002, 0xFFE };
u16 D_8020830C_jp[10] = { 0x1003, 2, 0x1004, 0x1002, 0x3C, 0x3C, 0x3C, 0x1002, 0xFFE };
u16 D_80208320_jp[6] = { 0x1003, 0x1002, 0x1002, 0x1004, 0xFFE };
u16 D_8020832C_jp[2] = { 0x1003, 0xFFE };
u16 D_80208330_jp[8] = { 4, 0x1002, 0x3C, 0x3C, 0x3C, 0x1002, 0xFFE };
u16 D_80208340_jp[4] = { 0x1002, 3, 0xFFE };
u16 D_80208348_jp[8] = { 0x1002, 0x3C, 0x3C, 0x3C, 0x3C, 0x1002, 0xFFE };
#elif !defined(VERSION_EU)
char *PTR32 D_80208378[2] = { (char *)D_8020E764, (char *)D_8020E768 };
#endif
extern Vtx D_80208380[8];
#ifdef ASSET_INIT
#include "hd_front_end/1C40/D_80208380.inc"
#else
Vtx D_80208380[8] = { 0 };
#endif
extern Gfx D_80208400[9];
#ifdef ASSET_INIT
#include "hd_front_end/1C40/D_80208400.inc"
#else
Gfx D_80208400[9] = { 0 };
#endif
Lights2 D_80208448 = {
    { { { 40, 10, 10 }, 0, { 40, 10, 10 }, 0 } },
    {
        { { { 240, 200, 20 }, 0, { 240, 200, 20 }, 0, { 69, -69, 69 }, 0 } },
        { { { 240, 110, 20 }, 0, { 240, 110, 20 }, 0, { -69, 69, 69 }, 0 } },
    },
};
Lights2 D_80208470 = {
    { { { 40, 2, 33 }, 0, { 40, 2, 33 }, 0 } },
    {
        { { { 90, 2, 220 }, 0, { 90, 2, 220 }, 0, { 69, -69, 69 }, 0 } },
        { { { 90, 2, 220 }, 0, { 90, 2, 220 }, 0, { -69, 69, 69 }, 0 } },
    },
};
u8 D_80208498[4] = { 32 };
UnkStruct_8020849C D_8020849C = { { 0 } };

#if !defined(VERSION_JP) && !defined(VERSION_EU)
const char D_8020E764[] = "";

const char D_8020E768[] = "GUEST: ";
#endif

#ifndef VERSION_EU
const char D_8020E770[] = CREW_TEXT_AMBER;

const char D_8020E82C[] = CREW_TEXT_CLARK;

const char D_8020E8EC[] = CREW_TEXT_SPIKE;

const char D_8020E9AC[] = CREW_TEXT_WESLEY;
#endif

#if defined(VERSION_JP) || defined(VERSION_EU)
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/1C40/func_801E8EB8.s")
#else
void func_801E8EB8(u8 arg0, u8 arg1) {
    s32 sp4C;
    PlayerInfo *sp48;
    u8 sp47;
    UnkStruct_80208358 sp34;
    UnkStruct_80208358 sp24;

    sp48 = &D_80364AF0[arg0];
    sp47 = 0;
    sp34 = D_80208358;
    sp24 = D_80208368;
    if (!(D_80364A90 & 0x10E18000) && arg0 != D_80364AEA) {
        sp47 = 1;
    }
    D_802154EC = -1;
    for (sp4C = 1; sp4C < 5; sp4C++) {
        D_80215508[sp4C] = -1;
    }
    if (arg0 < 4) {
        func_801E93DC(arg0);
    }
    if (D_80364A98 == 0x0001000000000000) {
        D_802158A0 = NULL;
        sprintf((char *)D_802155A0, "%s", sp34.unk0[arg0]);
    } else {
        D_802158A0 = NULL;
        if (arg0 < 4) {
            if (D_80365060[arg0] == 1) {
                sprintf((char *)D_802155A0, " ..... %s%s (%s) ... ", D_80208378[sp47], sp48, D_802081C0[sp48->unkC][0]);
                if (D_80364AF0[arg0].gameState >= 12) {
                    sp4C = 4;
                } else {
                    sp4C = 3;
                }
                for (; sp4C > 0; sp4C--) {
                    D_80215508[sp4C] = func_8025B300(D_802155A0);
                    if (sp4C != 1) {
                        sprintf((char *)D_802155A0, "%s  %d .. ", D_802155A0, D_80215930[sp4C]);
                    }
                }
                sprintf((char *)D_802155A0, "%s  %d ... ", D_802155A0, D_80215930[1]);
                if (D_802E8BF8 == 0) {
                    sprintf((char *)D_802155A0, "%s$%d ... ", D_802155A0, sp48->unk14);
                }
                D_802154EC = func_8025B300(D_802155A0);
                sprintf((char *)D_802155A0, "%s  %d", D_802155A0, sp48->unkC);
                if ((D_80364A98 & 0x0200040000000000) || (D_80364A90 & 0x0100000000000000)) {
                    sprintf((char *)D_802155A0, "%s ..... %s", D_802155A0, "USE Z/R TO CHANGE PLAYER, THEN A TO SELECT!");
                }
            } else {
                sprintf((char *)D_802155A0, " ... NEW GAME");
            }
        } else {
            sprintf((char *)D_802155A0, " ");
        }
    }
    if (D_802158A0 != NULL) {
        D_802154D2 = func_8025B370(D_802158A0);
        D_8021592E = 1;
        D_80215458 = 19;
        if (arg0 == 4) {
            D_802154D4 = 22;
        } else {
            D_802154D4 = 14;
        }
    } else {
        D_802154D2 = func_8025B300(D_802155A0);
        D_8021592E = 0;
        D_80215458 = 12;
        if (arg0 == 4) {
            D_802154D4 = 36;
        } else {
            D_802154D4 = 22;
        }
    }
    if (D_802154D2 >= 0x100) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sslen<TOTAL_SCROLL_LENGTH", "player.c", 400);
    }
    D_802154DC = -1;
    if (arg1 || arg0 == 4) {
        D_802154E0 = 12.0f;
    } else {
        D_802154E0 = 8.0f;
    }
}
#endif

void func_801E93DC(u8 arg0) {
    s32 spC;
    PlayerInfo *sp8;

    sp8 = &D_80364AF0[arg0];
    for (spC = 0; spC < 6; spC++) {
        D_80215930[spC] = 0;
    }
    for (spC = 0; spC < 0x3C; spC++) {
        if (sp8->medal[spC] > 0 && sp8->medal[spC] < 5) {
            D_80215930[sp8->medal[spC]]++;
        }
        if (LEVEL_DONE_IN(D_80364AF0[arg0], spC) &&
            (D_802E8F94[spC].unk0 & 0x81) && !DUMMY_LEVELS(spC)) {
            D_80215930[3]++;
        }
    }
}

u16 func_801E9528(void) {
    s32 sp4;

    D_802154DC = (D_802154DC + 1) % D_802154D2;
    if (D_802154DC == D_802154EC) {
        D_802154E8 = 0;
    }
    for (sp4 = 1; sp4 <= ((D_80364AF0[D_80364AE8].gameState >= 12) ? 4 : 3); sp4++) {
        if (D_80215508[sp4] == D_802154DC) {
            D_802154F0[sp4] = 0;
        }
    }
    if (D_802154D4) {
        D_802154D4--;
        if (!D_802154D4) {
            D_802154E0 = SCROLL_SPEED;
        }
    }
    D_802154E4 = (D_802154E0 - D_802154E4) * 0.2 + D_802154E4;
    if (D_802158A0 != NULL) {
        return D_802158A0[D_802154DC];
    }
    return D_802155A0[D_802154DC];
}

s32 func_801E96F8(void) {
    return D_802154D2 == D_802154DC + 8;
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/1C40/func_801E9718.s")
#else
Gfx *func_801E9718(Gfx *arg0, FrameBuf *arg1, s32 arg2) {
    Gfx *gfx;

    gfx = arg0;
    gSPMatrix(gfx++, &arg1->mtx[2], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &arg1->mtx[6], G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &arg1->mtx[23], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(gfx++, D_8021591C);
    gDPPipeSync(gfx++);
    gSPTexture(gfx++, 0x07C0, 0x07C0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH | G_LIGHTING);
    /* The nested blocks reproduce the original's stack layout. */
    {
        s32 pad[4];

        gSPSetLights2(gfx++, D_80208448);
        gDPSetRenderMode(gfx++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
        {
            s32 sp84;
            s32 sp80;

            if ((D_80364A90 & 0x0005040008010080) != 0) {
                D_8021593C += 16;
                if (D_8021593C >= 0x100) {
                    D_8021593C = 0xFF;
                }
            } else if ((D_80364A90 & 0x0008000202020000) != 0) {
                if ((D_8021593C -= 16) <= 0) {
                    D_8021593C = 0;
                }
            }
            for (sp80 = 0; sp80 < 4; sp80++) {
                D_80208380[sp80].v.cn[3] = (D_8021593C * 40) / 255;
            }
            for (sp80 = 4; sp80 < 7; sp80++) {
                D_80208380[sp80].v.cn[3] = (D_8021593C * 180) / 255;
            }
            osWritebackDCache(D_80208380, 0x80);
            if (D_8021593C != 0) {
                gSPDisplayList(gfx++, D_80208400);
            }
            gDPPipeSync(gfx++);
            gSPClearGeometryMode(gfx++, G_LIGHTING);
            if ((D_802154D8 += D_802154E4) >= D_80215458) {
                D_802154D8 -= D_80215458;
                for (sp80 = 1; D_802158A8[sp80] != D_802E8C98[D_8021592E]; sp80++) {
                    D_802158A8[sp80 - 1] = D_802158A8[sp80];
                }
                sp80--;
                D_802158A8[sp80] = func_801E9528();
                D_802158A8[sp80 + 1] = D_802E8C98[D_8021592E];
            }
            gfx = func_80274868(gfx);
            sp84 = 310 - (D_802154E8 += D_802154E4);
            if (sp84 >= -31 && sp84 < 320) {
                gfx = func_80272ED8(gfx, D_80215915, sp84, 194, D_8021593C, 1, 1.0f);
            }
            for (sp80 = 1; sp80 < 5; sp80++) {
                D_802154F0[sp80] += D_802154E4;
                sp84 = 310 - D_802154F0[sp80];
                if (sp84 >= -31 && sp84 < 320) {
                    gfx = func_80272ED8(gfx, D_80215916 + sp80, sp84, 194, D_8021593C, 1, 1.0f);
                }
            }
            gfx = func_80274AA4(gfx);
            func_80259CCC((s32)arg1, (D_8021592E == 1) ? NULL : func_8025B558(D_802158A8),
                          (D_8021592E == 1) ? D_802158A8 : NULL, 0, 0, -D_802154D8 % D_80215458 - 3, 201, 20, 20, 1,
                          0, 0, 0, (D_8021593C / 2 - 27 < 0) ? 0 : D_8021593C / 2 - 27);
            func_80259DC8((s32)arg1, (D_8021592E == 1) ? NULL : func_8025B558(D_802158A8),
                          (D_8021592E == 1) ? D_802158A8 : NULL, 0, 0, -D_802154D8 % D_80215458, 199, 20, 20, 1,
                          0xFF, 0xFF, 0xFF, D_8021593C, 0xFF, 0xFF, 0xFF, D_8021593C);
            gDPPipeSync(gfx++);
            return gfx;
        }
    }
}
#endif

void func_801EA108(u8 arg0, u8 arg1, u8 arg2) {
    PlayerInfo *sp2C;
    u32 sp28;

    sp2C = &D_80364AF0[arg0];
    for (sp28 = 0; sp28 < 0x100; sp28++) {
        ((u8 *)sp2C)[sp28] = 0;
    }
    func_801EA268(sp2C);
    sprintf((char *)sp2C, "%s", "NEW GAME");
    D_80365060[arg0] = 2;
    osSendMesg(&D_80219EF8, (OSMesg)((arg0 << 16) | 7), OS_MESG_BLOCK);
    if (arg1) {
        osSendMesg(&D_80219EF8, (OSMesg)((arg2 ? 0x14 : 0x15) | (arg0 << 16) | 0x01000000), OS_MESG_BLOCK);
        osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
    } else {
        osSendMesg(&D_80219EF8, (OSMesg)((arg2 ? 0x14 : 0x15) | (arg0 << 16)), OS_MESG_BLOCK);
    }
}

void func_801EA268(PlayerInfo *arg0) {
    arg0->unk10 = 0x1063E;
}

void func_801EA278(void) {
    s32 sp1C;
    s32 sp18;

    for (sp1C = 0; sp1C < 4; sp1C++) {
        osSendMesg(&D_80219EF8, (OSMesg)((sp1C << 16) | 6 | 0x01000000), OS_MESG_BLOCK);
        osRecvMesg(&D_80219F50, (OSMesg *)&sp18, OS_MESG_BLOCK);
        if (sp18 == 0) {
            if (func_8025B3F0((u8 *)&D_80364AF0[sp1C], (u8 *)"NEW GAME")) {
                D_80365060[sp1C] = 1;
            } else {
                D_80365060[sp1C] = 2;
            }
        } else if (sp18 != 0x6E382) {
            if (sp1C < D_8039C538) {
                osSendMesg(&D_80219EF8, (OSMesg)((sp1C << 16) | 3 | 0x01000000), OS_MESG_BLOCK);
                osRecvMesg(&D_80219F50, (OSMesg *)&sp18, OS_MESG_BLOCK);
            }
            if (sp18 != 0 || sp1C >= D_8039C538) {
                D_80365060[sp1C] = 0;
                sprintf(D_80215520[sp1C], "%d : %s", sp1C + 1, "PAK FULL");
            } else {
                func_801EA108(sp1C, 1, 0);
            }
        } else {
            func_801EA108(sp1C, 1, 0);
        }
        if (D_80365060[sp1C] == 1 || D_80365060[sp1C] == 2) {
            sprintf(D_80215520[sp1C], "%d : %s", sp1C + 1, &D_80364AF0[sp1C]);
        }
    }
}

void func_801EA4B8(void) {
    s32 sp1C;

    func_801EA278();
    D_802154B0 = 0;
    for (sp1C = 0; sp1C < 4; sp1C++) {
        ENTRY_TEXT(D_8020C070[sp1C + 2]) = D_80215520[sp1C];
        D_8020C070[sp1C + 2].flags |= 0x81;
        D_8020C070[sp1C + 2].flags &= ~0x20;
        D_8020C070[sp1C + 2].unk18 = 7;
        D_8020C070[sp1C + 2].unk19 = 4;
        D_8020C070[sp1C + 2].x = 0x50;
        switch (D_80365060[sp1C]) {
            case 0:
                D_8020C070[sp1C + 2].flags &= ~0x81;
                D_8020C070[sp1C + 2].unk18 = 8;
                D_802154B0++;
                break;
        }
    }
    if (D_802154B0 != 4) {
#ifdef VERSION_EU
        ENTRY_TEXT(D_8020C070[FE_ENTRY(6)]) = LANG_TEXT("ERASE GAME", "SPIEL LOESCHEN");
        D_8020C070[FE_ENTRY(6)].unk10 = NULL;
#else
        D_8020C070[FE_ENTRY(6)].text = "ERASE GAME";
        D_8020C070[FE_ENTRY(6)].unk10 = (u16 *)D_803046F8;
#endif
    } else {
#ifdef VERSION_EU
        ENTRY_TEXT(D_8020C070[FE_ENTRY(6)]) = LANG_TEXT("IGNORE PAK", "IGNORIERE PAK");
        D_8020C070[FE_ENTRY(6)].unk10 = NULL;
#else
        D_8020C070[FE_ENTRY(6)].text = "IGNORE PAK";
        D_8020C070[FE_ENTRY(6)].unk10 = (u16 *)D_80304710;
#endif
    }
    D_8020C070[FE_ENTRY(6)].unk19 = 2;
    D_802F8BDC[10].unk8 &= ~0x400;
}

void func_801EA6E8(void) {
    s32 sp24;

    func_801EA278();
    for (sp24 = 0; sp24 < 4; sp24++) {
        D_8020C070[sp24 + 2].x = 0x40;
        switch (D_80365060[sp24]) {
            case 1:
#ifdef VERSION_EU
                sprintf(D_80215520[sp24], LANG_TEXT("ERASE %d : %s", "LOESCHE %d : %s"), sp24 + 1, &D_80364AF0[sp24]);
#else
                sprintf(D_80215520[sp24], "ERASE %d : %s", sp24 + 1, &D_80364AF0[sp24]);
#endif
                D_8020C070[sp24 + 2].flags |= 0x81;
                D_8020C070[sp24 + 2].flags &= ~0x20;
                D_8020C070[sp24 + 2].unk18 = 7;
                D_8020C070[sp24 + 2].unk19 = 4;
                break;
            case 0:
            case 2:
                D_8020C070[sp24 + 2].unk18 = 8;
                D_8020C070[sp24 + 2].flags |= 0x20;
                D_8020C070[sp24 + 2].flags &= ~0x81;
                D_8020C070[sp24 + 2].unk18 = 8;
                break;
        }
    }
#ifdef VERSION_EU
    ENTRY_TEXT(D_8020C070[FE_ENTRY(6)]) = LANG_TEXT("GO BACK", "WEITER");
    D_8020C070[FE_ENTRY(6)].unk10 = NULL;
#else
    D_8020C070[FE_ENTRY(6)].text = "GO BACK";
    D_8020C070[FE_ENTRY(6)].unk10 = (u16 *)D_80304730;
#endif
    D_8020C070[FE_ENTRY(6)].unk19 = 2;
    D_802F8BDC[10].unk8 |= 0x400;
    D_802F8BDC[10].unk18 = 6;
}

void func_801EA93C(char *arg0, s32 arg1, u8 arg2, u8 arg3, char *arg4) {
    D_802154B2 = 0x7FFF;
    D_802154B4 = 0x7FFF;
    D_80215918 = 0;
    D_802154B8 = 0;
    D_802154B6 = 0;
    D_802154BA = -1;
    D_802154BC = 0;
    D_802154BE = 0;
    D_802154C0 = 1;
    D_8020C070[FE_ENTRY(7)].unk6 = arg3;
    D_8020C070[FE_ENTRY(7)].unk8 = arg3;
    D_802154CC = arg3;
    D_802154C8 = (arg3 * 3) / 5;
    D_80215928 = arg4;
    *arg4 = 0;
    ENTRY_TEXT(D_8020C070[FE_ENTRY(7)]) = D_80215928;
    D_8020C070[FE_ENTRY(7)].x = D_802154C4 = 160 - D_802154C8 / 2;
    ENTRY_TEXT(D_8020C070[FE_ENTRY(8)]) = arg0;
    D_8020C070[FE_ENTRY(8)].unk10 = (u16 *)arg1;
    D_80215924 = arg2;
    D_802154D0 = 1;
    D_8021591A = 0;
    D_80215920 = 1;
    D_8021592C = 0;
}

Gfx *func_801EAA7C(Gfx *arg0, FrameBuf *arg1, s32 *arg2) {
    Gfx *spFC;
    YoshiEntry *spF8;
    u8 spF7;
    s32 spF0;
    s32 spEC;
    s16 spEA;
    s32 spE4;
    s32 spE0;
    s32 spDC;
    s32 spD8;
    s32 spD4;
    s32 spD0;
    s32 spCC;
    s32 spC8;
    ColorPair *spC4;
    ColorPair *spC0;
    s16 spBE;
    s16 spBC;
    u8 spBB;

    spFC = arg0;
    spF8 = &D_8020C070[FE_ENTRY(7)];
    spCC = 0;
    spC8 = func_8025B300((u8 *)D_80215928);
    spC4 = &D_802F47B0[16];
    if ((D_80370C28 & 0x300) && !(D_80370C2A & 0x300)) {
        func_80260650(D_80367738, 0xC, 0);
    }
    D_8021546C = D_8021591A;
    D_8021591A = D_80370C2C;
    if (((D_8021591A > 0) ? D_8021591A : -D_8021591A) < 10) {
        D_8021591A = 0;
    } else {
        D_8021591A *= (D_8021591A > 0) ? D_8021591A : -D_8021591A;
    }
    if (D_8021591A != 0) {
        if (D_8021546C == 0) {
            D_80215918 = D_802154B6;
        }
        D_802154B4 -= D_8021591A >> 3;
    } else {
        if (D_8021546C != 0 && D_802154B6 == D_80215918) {
            D_802154B6 += (D_8021546C >= 0) ? 1 : -1;
            if (D_802154B6 < 0) {
                D_802154B6 += 33;
            }
            if (D_802154B6 >= 33) {
                D_802154B6 -= 33;
            }
            D_802154B2 = 0x7FFF - D_802154B6 * 1986;
        }
        spEA = D_802154B4 - D_802154B2;
        D_802154B4 -= spEA >> 4;
    }
    D_802154B6 = (0x83E0 - D_802154B4) / 1986;
    if (D_802154B6 == 33) {
        D_802154B6 = 0;
    }
    if (D_8021591A != 0) {
        D_802154B2 = 0x7FFF - D_802154B6 * 1986;
    }
    if (D_802154D0 != 0) {
        D_802154B8 = D_802154B6;
        D_802154D0 = 0;
    }
    if (D_802154B6 != D_802154B8) {
        func_80260650(D_80367738, 0x90, 0);
    }
    D_802154B8 = D_802154B6;
    for (spEC = 0; spEC < 2; spEC++) {
        for (spF0 = 0; spF0 < 33; spF0++) {
            spBE = D_802154B4 + spF0 * 1986 - 0x6000;
            spBC = spBE - 0x2000;
            spBC += 0x7FFF;
            spBC += 0x2000;
            spBC += 0x7FFF;
            spE4 = sins(spBC) * 120.0 / 32768.0 - 7.0;
            spE0 = coss(spBC) * 96.0 / 32768.0 - 10.0;
            if (spF0 < D_80208350[0]) {
                D_80208498[0] = D_80208314[spF0];
            } else {
                D_80208498[0] = D_802082FC[spF0 - D_80208350[0]];
            }
            if (D_80208498[0] != D_802154BA) {
                if (spEC != 0) {
                    if (D_802154B6 == spF0) {
                        spF7 = D_80208498[0], spDC = spE4, spD8 = spE0, spD4 = 24, spD0 = 20;
                    } else {
                        func_80259DC8((s32)arg1, (char *)D_80208498, NULL, 1, 0, spE4, spE0, 24, 20, 1, 200, 200, 200,
                                      D_8036BB20, 0xFF, 0xFF, 0xFF, D_8036BB20);
                    }
                } else if (D_802154B6 == spF0) {
                    func_80259CCC((s32)arg1, (char *)D_80208498, NULL, 1, 0, spE4 - 11, spE0, 48, 40, 1, 0, 0, 0,
                                  D_8036BB20 / 2);
                } else {
                    func_80259CCC((s32)arg1, (char *)D_80208498, NULL, 1, 0, spE4 - 4, spE0 + 3, 24, 20, 1, 0, 0, 0,
                                  D_8036BB20 / 2);
                }
            }
        }
    }
    if (spF7 != D_802154BA && D_802154BA == -1) {
        if (D_803156C4 % 20 < 15) {
            spBB = D_8036BB20;
        } else {
            spBB = D_8036BB20 / 3;
        }
        D_80208498[0] = spF7;
        func_80259BD4(&spFC, arg1);
        func_80259DC8((s32)arg1, (char *)D_80208498, NULL, 1, 0, spDC - 5, spD8 - 5, spD4 * 2, spD0 * 2, 1, spC4->r0,
                      spC4->g0, spC4->b0, spBB, spC4->r1, spC4->g1, spC4->b1, spBB);
    }
    if (D_8036BB1C == 2 && (D_80370C28 & 0x1000) && !(D_80370C2A & 0x1000) && func_802753C0() == 0) {
        if (D_802154BC != 0) {
            D_80364A98 = 0x01000000;
            D_80365060[D_80364AE8] = 1;
            func_8026AF6C(0x4000);
        } else {
            func_80260650(D_80367738, 0x2B, 0);
        }
    }
    switch (D_802154BE) {
        case 0:
            if (D_8036BB1C == 2 && (D_80370C28 & 0xC000) && !(D_80370C2A & 0xC000) && func_802753C0() == 0) {
                if (D_80370C28 & 0x4000) {
                    spF7 = 0x7F;
                }
                if (spF7 == 0x7F) {
                    if (D_802154BC == 0) {
                        if (D_80370C28 & 0x4000) {
                            if (D_802E8BF8 != 0) {
                                func_80275270(0x0400000000000000, 0.5f);
                            } else {
                                D_80364A98 = 0x200000;
                            }
                            func_80260650(D_80367738, 0xDE, 0);
                            func_8026AF6C(0x4000);
                        } else {
                            func_80260650(D_80367738, 0x2B, 0);
                        }
                    } else {
                        D_802154BC--;
                        D_80215928[D_802154BC] = 0;
                        func_80260650(D_80367738, 0xE4, 0);
                        D_802154C0 = 0;
                        D_80215464 = spF8->unk6;
                        D_80215468 = D_802154CC;
                        spF0 = D_80215468;
                        spF0 = spF0 * 3 / 5;
                        D_80215450 = spF8->x;
                        D_80215454 = -((spC8 - 1) * spF0 / 2) - spF0 / 2 + 160;
                        D_8021545C = D_802154C4;
                        D_80215460 = (spC8 - 1) * spF0 / 2 - spF0 / 2 + 160;
                    }
                } else if (D_802154BC < D_80215924) {
                    func_80260650(D_80367738, 0x91, 0);
                    D_802154BE = 1;
                    D_802154C0 = 0;
                    D_802154BA = spF7;
                    D_80215464 = spF8->unk6;
                    D_80215468 = D_802154CC;
                    spF0 = D_80215468;
                    spF0 = spF0 * 3 / 5;
                    D_80215450 = spF8->x;
                    D_80215454 = -((spC8 + 1) * spF0 / 2) - spF0 / 2 + 160;
                    D_8021545C = D_802154C4;
                    D_80215460 = (spC8 + 1) * spF0 / 2 - spF0 / 2 + 160;
                    D_80215944 = spDC - 5;
                    D_80215948 = spD8 - 5;
                    D_8021594C = D_80215460 - 160.0f - spF0;
                    D_80215950 = spF8->y - 120;
                    D_80215440 = 48.0f;
                    D_80215444 = 40.0f;
                    D_80215448 = D_80215468;
                    D_8021544C = D_80215468;
                } else {
                    func_80260650(D_80367738, 0x2B, 0);
                }
            }
            break;
        case 1:
            D_80215944 += (D_8021594C - D_80215944) * 0.2;
            D_80215948 += (D_80215950 - D_80215948) * 0.2;
            D_80215440 += (D_80215448 - D_80215440) * 0.2;
            D_80215444 += (D_8021544C - D_80215444) * 0.2;
            D_80208498[0] = D_802154BA;
            func_80259CCC((s32)arg1, (char *)D_80208498, NULL, 0, 0, D_80215944 - 4.0f, D_80215948 + 4.0f, D_80215440,
                          D_80215444, 1, 0, 0, 0, D_8036BB20 / 2);
            func_80259DC8((s32)arg1, (char *)D_80208498, NULL, 0, 0, D_80215944, D_80215948, D_80215440, D_80215444, 1,
                          spC4->r0, spC4->g0, spC4->b0, D_8036BB20, spC4->r1, spC4->g1, spC4->b1,
                          D_8036BB20);
            if (((D_80215944 - D_8021594C > 0.0f) ? D_80215944 - D_8021594C : -(D_80215944 - D_8021594C)) <
                    0.15 &&
                ((D_80215948 - D_80215950 > 0.0f) ? D_80215948 - D_80215950 : -(D_80215948 - D_80215950)) <
                    0.15) {
                func_80260650(D_80367738, 1, 0);
                D_802154BE = 0;
                D_802154C0 = 0;
                D_80215928[D_802154BC] = D_80208498[0];
                D_80215928[++D_802154BC] = 0;
                D_802154BA = -1;
            }
            break;
    }
    func_80259BD4(&spFC, arg1);
    D_80215940 += D_80215920;
    if (D_80215920 < 0) {
        D_80215940 += D_80215920 * 2;
    }
    if (D_80215940 < 0 || D_80215940 >= 16) {
        D_80215940 -= D_80215920 * 2;
        D_80215920 = -D_80215920;
    }
    D_8021592C = (D_8036BB1C == 2) ? ((D_8021592C + 16 < 255) ? D_8021592C + 16 : 255) : 0;
    spC0 = &D_802F47B0[19];
    spCC = func_80276130(arg1, 3, spCC, D_802154C4 + D_802154C8 + D_80215940, spF8->y + spF8->unk8 / 2,
                         spF8->unk6 / 3 + D_80215940 / 2, spF8->unk8 / 2 + 3, D_802F47B0[19].r0, D_802F47B0[19].g0,
                         D_802F47B0[19].b0, D_8021592C, D_802F47B0[19].r1, D_802F47B0[19].g1, D_802F47B0[19].b1,
                         D_8021592C, D_802F47B0[19].r0, D_802F47B0[19].g0, D_802F47B0[19].b0, D_8021592C,
                         D_802F47B0[19].r1, D_802F47B0[19].g1, D_802F47B0[19].b1, D_8021592C);
    spCC = func_80276080(arg1, 3, spCC, D_802154C4 + D_802154C8 + D_80215940 + 4, spF8->y + spF8->unk8 / 2 + 3,
                         spF8->unk6 / 3 + D_80215940 / 2, spF8->unk8 / 2 + 3, 0, 0, 0, D_8021592C / 2);
    spFC = func_80275DA4(spFC, 0);
    gSPVertex(spFC++, arg1->vtx, 8, 0);
    gSP1Triangle(spFC++, 4, 5, 6, 0);
    gSP1Triangle(spFC++, 4, 6, 7, 0);
    gSP1Triangle(spFC++, 0, 1, 2, 0);
    gSP1Triangle(spFC++, 0, 2, 3, 0);
    spCC = func_80276130(arg1, 2, spCC, spF8->x - D_80215940 - 4, spF8->y + spF8->unk8 / 2,
                         spF8->unk6 / 3 + D_80215940 / 2, spF8->unk8 / 2 + 3, spC0->r0, spC0->g0, spC0->b0,
                         D_8021592C, spC0->r1, spC0->g1, spC0->b1, D_8021592C, spC0->r0, spC0->g0,
                         spC0->b0, D_8021592C, spC0->r1, spC0->g1, spC0->b1, D_8021592C);
    spCC = func_80276080(arg1, 2, spCC, spF8->x - D_80215940 - 8, spF8->y + spF8->unk8 / 2 + 3,
                         spF8->unk6 / 3 + D_80215940 / 2, spF8->unk8 / 2 + 3, 0, 0, 0, D_8021592C / 2);
    spFC = func_80275DA4(spFC, 0);
    gSPVertex(spFC++, &arg1->vtx[spCC - 8], 8, 0);
    gSP1Triangle(spFC++, 4, 5, 6, 0);
    gSP1Triangle(spFC++, 4, 6, 7, 0);
    gSP1Triangle(spFC++, 0, 1, 2, 0);
    gSP1Triangle(spFC++, 0, 2, 3, 0);
    switch (D_802154C0) {
        case 0:
            D_80215450 += (D_80215454 - D_80215450) * 0.2;
            spF8->x = D_80215450;
            D_8021545C += (D_80215460 - D_8021545C) * 0.2;
            D_802154C4 = D_8021545C;
            break;
        case 1:
            break;
    }
    *arg2 += spFC - arg0;
    return spFC;
}

void func_801EC288(u8 arg0) {
    s32 sp4;

    D_80215902[0] = 3;
    D_80215902[1] = arg0;
    for (sp4 = 0; sp4 < 2; sp4++) {
        D_80215900[sp4] = 4;
        D_80215908[sp4] = 50;
        D_80215910[sp4] = 0;
    }
}

void func_801EC30C(u8 arg0) {
    s32 sp24;
    s32 sp20;

    D_80215914 = func_80272C5C(D_802082E4, D_802082D8, 4, 2, 0, 1.0f);
    func_80272C5C(D_802082E8, NULL, 1, 2, 1, 1.0f);
    D_80215902[0] = 3;
    D_80215902[1] = arg0;
    if (D_802E8F94[D_802E8BDC].unk0 == 1) {
        sp24 = 0;
    } else {
        sp24 = 1;
    }
    for (sp20 = 0; sp24 < 2; sp24++, sp20++) {
        D_80215900[sp24] = 0;
        D_80215908[sp24] = sp20 * 60 + 40;
        D_80215910[sp24] = 0;
    }
}

void func_801EC464(void) {
    s32 sp4;

    for (sp4 = 0; sp4 < 2; sp4++) {
        D_80215900[sp4] = 3;
    }
}

Gfx *func_801EC49C(Gfx *arg0, s32 arg1, s32 arg2, u8 arg3) {
    Gfx *gfx;
    s32 pad;
    s32 sp2C;

    gfx = arg0;
    if (D_8036BB1C == 8) {
        D_80215900[arg3] = 3;
    }
    switch (D_80215900[arg3]) {
        case 0:
            D_80215908[arg3]--;
            if (D_80215908[arg3] == 0) {
                D_80215900[arg3] = 1;
                if (D_80215902[arg3] == 5) {
                    func_80260650(D_80367738, 0xEA, 0);
                } else {
                    func_80260650(D_80367738, 0xE6, 0);
                }
            }
            break;
        case 4:
            D_80215908[arg3]--;
            if (D_80215908[arg3] == 0) {
                D_80215900[arg3] = 1;
            }
            break;
        case 1:
            D_80215910[arg3] += 0x10;
            if (D_80215910[arg3] >= 0x100) {
                D_80215910[arg3] = 0xFF;
                D_80215900[arg3] = 2;
            }
            break;
        case 3:
            D_80215910[arg3] -= 0x20;
            if (D_80215910[arg3] < 0) {
                D_80215910[arg3] = 0;
            }
            break;
        case 2:
            break;
    }
    if (D_80215900[arg3] == 0) {
        return gfx;
    }
    if (D_80215902[arg3] == 5) {
        sp2C = 2;
    } else {
        sp2C = 1;
    }
    gfx = func_80272ED8(gfx, D_80215902[arg3] % 5 + D_80215914, arg1, arg2, D_80215910[arg3], sp2C, 1.0f);
    return gfx;
}

#ifdef VERSION_JP
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/1C40/func_801EC770.s")
#else
Gfx *func_801EC770(Gfx *arg0, s32 arg1, s32 *arg2) {
    Gfx *sp64;
    u8 sp63;

    sp64 = arg0;
    sp64 = func_80274868(sp64);
    if (D_80364AA8 == 1) {
        sp64 = func_801EC49C(sp64, 0x5C, 0x68, 0);
        if (D_80215902[1] != 0) {
            sp64 = func_801EC49C(sp64, 0xA8, 0x68, 1);
        }
    } else {
        if (D_80215902[1] == 5) {
            sp63 = 1;
        } else {
            sp63 = (((D_80364AF0[D_80364AE8].gameState >= 12) ? 4 : 3) < D_80215902[1] + 1)
                       ? ((D_80364AF0[D_80364AE8].gameState >= 12) ? 4 : 3)
                       : D_80215902[1] + 1;
        }
        sp64 = func_801EC49C(sp64, 0x82, 0x68, 1);
        sp64 = func_80272ED8(sp64, sp63 + D_80215914, 0x2E, 0x6C, (D_80215910[1] * 3) / 4, 0, 0.75f);
    }
    sp64 = func_80274AA4(sp64);
    if (D_80364AA8 != 1) {
        func_80264A34(D_80215470, D_802E8F94[D_802E8BDC].medalTimes[4 - sp63], 0);
        D_80215470[5] = 0;
        func_80259DC8(arg1, D_80215470, 0, 0, 0, 0x29, 0x7D, 0x10, 0x10, 1, 0xFF, 0xB4, 0, D_80215910[1], 0xFF, 0x78, 0,
                      D_80215910[1]);
    }
    *arg2 += sp64 - arg0;
    return sp64;
}
#endif

u64 func_801ECA50(u8 arg0) {
    u64 sp8;

    if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], arg0) {
        sp8 = 0x80;
    } else if (D_802E8F94[arg0].unk0 & 0x81) {
        sp8 = 0x800;
    } else {
        sp8 = 0x20000000;
    }
    return sp8;
}

void func_801ECB18(void) {
    func_801FE018(8);
    D_80364A87 = 0;
    D_803643D5 = 0;
    if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) {
        if (D_802E8F94[D_802E8BDC].unk0 == 1) {
            osSendMesg(&D_80219EF8, (OSMesg)((D_802E8BDC << 8) | 0xC | (D_80364AE8 << 16)), OS_MESG_BLOCK);
        }
        osSendMesg(&D_80219EF8, (OSMesg)((D_802E8BDC << 8) | 8 | (D_80364AE8 << 16) | 0x01000000), OS_MESG_BLOCK);
    } else {
#ifdef VERSION_EU
        osSendMesg(&D_80219EF8, (OSMesg)((D_802E8BDC << 8) | 0x18 | (D_80364AE8 << 16) | 0x01000000), OS_MESG_BLOCK);
#else
        osSendMesg(&D_80219EF8, (OSMesg)((D_802E8BDC << 8) | 0x16 | (D_80364AE8 << 16) | 0x01000000), OS_MESG_BLOCK);
#endif
        func_801F8354(D_80364AE8);
    }
}

void func_801ECC8C(void) {
    s32 sp1C;
    s32 sp18;

    sp18 = 0;
    for (sp1C = 0; sp1C < 4; sp1C++) {
        if (D_8039C53C[sp1C] != 0) {
            if (sp18 == 0) {
                func_80261570(0.0f);
                sp18 = 1;
            }
            osSendMesg(&D_80219EF8, (OSMesg)((sp1C << 16) | 0x14), OS_MESG_BLOCK);
            func_8029A7E4("saving player %d on level %d\n", sp1C, D_8039C53C[sp1C] - 1);
            D_80364AF0[sp1C].levelno = D_802E8BDC;
            osSendMesg(&D_80219EF8, (OSMesg)(((D_8039C53C[sp1C] - 1) << 8) | 7 | (sp1C << 16)), OS_MESG_BLOCK);
            osSendMesg(&D_80219EF8, (OSMesg)(((D_8039C53C[sp1C] - 1) << 8) | 9 | (sp1C << 16)), OS_MESG_BLOCK);
            osSendMesg(&D_80219EF8, (OSMesg)(((D_8039C53C[sp1C] - 1) << 8) | 0xB | (sp1C << 16)), OS_MESG_BLOCK);
            if (D_8039C540 != 0 && D_80364AE8 == sp1C) {
                osSendMesg(&D_80219EF8, (OSMesg)(((D_8039C540 - 1) << 8) | 0xD | (D_80364AE8 << 16)), OS_MESG_BLOCK);
                D_8039C540 = 0;
            }
            osSendMesg(&D_80219EF8, (OSMesg)((sp1C << 16) | 0x15 | 0x01000000), OS_MESG_BLOCK);
            osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
            D_8039C53C[sp1C] = 0;
        }
    }
}

void func_801ECE9C(void) {
    s32 sp4;

    for (sp4 = 0; sp4 < 0x3C; sp4++) {
        if (D_802E8F94[sp4].unk0 & 0x81) {
            if (D_80364AF0[D_80364AE8].gameState >= 11 && D_80364A98 == 0x4000) {
                D_802E8F94[sp4].unk0 = 0x80;
            } else {
                D_802E8F94[sp4].unk0 = 1;
            }
        }
    }
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_front_end/1C40/func_801ECF5C.s")
#else
void func_801ECF5C(void) {
    PlayerInfo *sp6C;
    u8 sp4C[0x20];
    LevelInfo *sp48;
    UnkStruct_8020849C sp40;
    u8 sp3F;
    u16 sp3C;
    s32 sp38;
    s32 sp34;
    u16 sp32;
    u8 sp31;
    u8 sp30;

    sp6C = &D_80364AF0[D_80364AE8];
    sp40 = D_8020849C;
    sp3C = sp6C->units;
    func_801ED480((u8 *)D_80364EF0[D_80364AE8], sp4C);
    if (D_8039C53C[D_80364AE8] == 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "saveIt[playerNumber]", "player.c", 0x568);
    }
    for (sp30 = 0; sp30 < 0x3C; sp30++) {
        if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], sp30) {
            if (D_802E8F94[sp30].unk0 == 1 && !DUMMY_LEVELS(sp30)) {
                sp48 = &D_802E8F94[sp30];
                if (D_8039C53C[D_80364AE8] != sp30 + 1) {
                    osSendMesg(&D_80219EF8, (OSMesg)((sp30 << 8) | 8 | (D_80364AE8 << 16) | 0x01000000), OS_MESG_BLOCK);
                    osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
                } else {
                    func_801ED480(sp4C, (u8 *)D_80364EF0[D_80364AE8]);
                }
                sp32 = D_80364EF0[D_80364AE8][D_802E8C44[0]];
                func_8029A7E4("level %d time is %d\n", sp30, sp32);
                sp31 = func_801EF2BC(sp32, sp30, D_80364AF0[D_80364AE8].gameState);
                sp6C->medal[sp30] = sp31;
                sp40.unk0[sp31 - 1]++;
            }
        }
    }
    func_801ED480(sp4C, (u8 *)D_80364EF0[D_80364AE8]);
    D_802E8BDC = 0;
    func_8029A7E4(" %d %d %d %d %d\n", sp40.unk0[2] * 3, sp40.unk0[1] * 2, sp40.unk0[0], sp6C->units, sp3C);
    sp6C->units += sp40.unk0[2] * 3 + sp40.unk0[1] * 2 + sp40.unk0[0];
    sp3F = sp6C->units / 12 - sp3C / 12;
    sp6C->unkC += sp3F;
    func_8029A7E4("%d stars\n", sp3F);
    for (sp38 = 0, sp34 = (320 - (sp3F << 5)) / 2 + 40; sp38 < sp3F; sp38++, sp34 += 32) {
        D_8020C070[sp38 + FE_ENTRY(189)].flags |= 0x100;
        D_8020C070[sp38 + FE_ENTRY(189)].x = sp34;
        D_8020C070[sp38 + FE_ENTRY(189)].unk14 = 0x33;
    }
    for (; sp38 < 6; sp38++) {
        D_8020C070[sp38 + FE_ENTRY(189)].flags &= ~0x100;
        D_8020C070[sp38 + FE_ENTRY(189)].unk14 = 0;
    }
    for (sp38 = 0; sp38 < 3; sp38++) {
        sprintf(D_80215480[sp38], "*******%-2d**", sp40.unk0[2 - sp38]);
        D_8020C070[sp38 + FE_ENTRY(185)].text = D_80215480[sp38];
    }
}
#endif

void func_801ED480(u8 *arg0, u8 *arg1) {
    u32 sp4;

    for (sp4 = 0; sp4 < 0x20; sp4++) {
        arg1[sp4] = arg0[sp4];
    }
}

void func_801ED4B8(void) {
    PlayerInfo *sp54;
    u8 sp34[0x20];
    LevelInfo *sp30;
    u8 sp2F;
    u16 sp2C;
    s32 sp28;
    u8 sp27;
    u8 sp26;

    sp54 = &D_80364AF0[D_80364AE8];
    sp2C = sp54->units;
    func_801ED480((u8 *)D_80364EF0[D_80364AE8], sp34);
    if (D_8039C53C[D_80364AE8] == 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "saveIt[playerNumber]", "player.c", LINE_EU(0x5C0, 0x5CF));
    }
    for (sp26 = 0; sp26 < 0x3C; sp26++) {
        if (LEVEL_DONE_IN(D_80364AF0[D_80364AE8], sp26) &&
            !DUMMY_LEVELS(sp26)) {
            sp30 = &D_802E8F94[sp26];
            if (D_8039C53C[D_80364AE8] != sp26 + 1) {
                osSendMesg(&D_80219EF8, (OSMesg)((sp26 << 8) | 8 | (D_80364AE8 << 16) | 0x01000000), OS_MESG_BLOCK);
                osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
            } else {
                func_801ED480(sp34, (u8 *)D_80364EF0[D_80364AE8]);
            }
            sp27 = func_801EF2BC(D_80364EF0[D_80364AE8][D_802E8C44[D_80364AF0[D_80364AE8].unk92[sp26]]], sp26,
                                 D_80364AF0[D_80364AE8].gameState + 1);
            sp54->medal[sp26] = sp27;
            if (sp27 == 4) {
                sp54->units++;
            }
        }
    }
    func_801ED480(sp34, (u8 *)D_80364EF0[D_80364AE8]);
    D_802E8BDC = 0;
    sp2F = sp54->units / 12 - sp2C / 12;
    sp54->unkC += sp2F;
    func_8029A7E4("cmo destroy %d stars\n", sp2F);
}
