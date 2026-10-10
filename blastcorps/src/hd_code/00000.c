#include "common.h"
#include "game/objects.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/camera.h"
#include "game/vehicle.h"
#include "game/game.h"
#include "game/memmap.h"
#include "game/sched.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

/* The game's reads of the scheduler's counts, apart from its two waits on
   frameCount: the port's --replay gives these the movie's values
   (port_game.h, docs/PORT.md "The TAS"). */
#ifdef TARGET_PC
#define SC_FRAMECOUNT D_803156C4
#define SC_TIMER D_803156C0
#else
#define SC_FRAMECOUNT D_80315440.frameCount
#define SC_TIMER D_80315440.unk280
#endif

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
} UnkStruct_80364A00; /* size = 0xC */

/*
 * The per-frame buffer: D_803156F8 holds two of these, picked by D_8035805C,
 * and segment 2 points at the current one (see 30C70.c).
 */
/*
 * This file's .bss.  Some functions only match with these defined in the
 * same file (a u64's halves share one lui).
 */

/* .bss, 0x8030F660-0x803650B0 (tools/bss_c.py) */
#ifdef VERSION_EU
/* eu defines them in another order, with some of its own */
u8 D_8030F670[0x11B0];
OSThread D_80310820;
f32 D_80365078;
u64 D_803109D0[0x40];
f32 D_8036507C;
OSThread D_80310BD0;
f32 D_80365080;
u64 D_80310D80[0x2000/sizeof(u64)];
f32 D_80365084;
u64 D_80312D80[0x400];
f32 D_80365088;
OSMesgQueue D_80314D80;
f32 D_8036508C;
OSMesg D_80314D98[0xc2];
f32 D_80365090;
f32 D_80365094;
OSMesgQueue D_803150A0;
f32 D_80365098;
OSMesg D_803150B8[0x32];
s16 D_8036509C;
OSMesgQueue D_80315180;
s16 D_8036509E;
OSMesg D_80315198[0x90];
s16 D_803650A0;
OSMesgQueue D_803153D8;
u8 D_8030F660;
OSMesg D_803153F8[0x10];
s32 D_8030F664;
u8 D_80315438;
u8 D_8030F668;
u8 D_803153F0;
u8 D_8030F669;
Sched D_80315440;
u8 D_8030F66A;
SchedClient D_803156D8;
SndState *D_803156E8;
SndState *D_803156EC;
SndState *D_803156F0;
u8 D_803156F4;
u8 D_803156F5;
Frame D_803156F8[2];
u8 D_80358028[8];
Gfx *D_80358030[2];
Gfx *D_80358038[2];
Gfx *D_80358040[2];
Gfx *D_80358048[2];
u16 *D_80358050[2];
u16 *D_80358058;
u8 D_8035805C;
u32 D_80358060;
s32 D_80358064;
s32 D_80358068;
u8 *D_8035806C;
u8 *D_80358070;
LevelHeader *D_80358074;
s32 D_80358078;
u16 D_8035807C;
s32 D_80358080;
s32 D_80358084;
u8 D_80358088[0xc340];
UnkStruct_803643C8 *D_803643C8;
UnkStruct_803643C8 *D_803643CC;
u8 D_803643D0[4];
u8 D_803643D4;
u8 D_803643D5;
u8 D_803643D6;
u8 D_803643D7;
u8 D_803643D8;
u8 D_803643D9;
u8 D_803643DA;
u8 D_803643DB;
u8 D_803643DC;
s32 D_803643E0;
s32 D_803643E4;
s32 D_803643E8;
s32 D_803643EC;
s32 D_803643F0;
s32 D_803643F4;
s32 D_803643F8;
s32 D_803643FC;
s32 D_80364400;
s32 D_80364404;
s32 D_80364408;
s32 D_8036440C;
u8 D_80364410;
u8 D_80364411;
u8 D_80364412;
f32 D_80364414;
f32 D_80364418;
u8 D_8036441C;
u8 D_8036441D;
s32 D_80364420;
u8 D_80364424;
u32 D_80364428;
u16 D_8036442C;
s32 D_80364430;
u8 D_80364434;
f32 D_80364438;
s16 D_8036443C;
s16 D_8036443E;
s16 D_80364440;
f32 D_80364444;
f32 D_80364448;
s16 D_8036444C;
s16 D_8036444E;
u8 D_80364450[2];
s16 D_80364452;
s16 D_80364454;
u8 D_80364456;
u8 *D_80364458;
Vehicle D_80364460[0xc];
Vehicle *D_803649D0;
u64 D_803649D8;
s16 D_803649E0;
s16 D_803649E2;
s16 D_803649E4;
s32 D_803649E8;
u8 D_803649EC;
u8 D_803649ED;
s8 D_803649EE;
u32 D_803649F0;
u32 D_803649F4;
f32 D_803649F8;
UnkStruct_80364A00 D_80364A00[5];
u8 D_80364A3C;
u8 D_80364A3D;
s32 D_80364A40;
s32 D_80364A44;
u8 D_80364A48;
s16 D_80364A4A;
s16 D_80364A4C;
u8 D_80364A4E;
u8 D_80364A4F[1];
u8 D_80364A50;
s32 D_80364A54;
s32 D_80364A58;
s32 D_80364A5C;
u8 D_80364A60;
s32 D_80364A64;
u8 D_80364A68;
u8 D_80364A69;
u8 D_80364A6A;
u8 D_80364A6B;
u8 D_80364A6C;
u8 D_80364A6D;
u8 D_80364A6E;
u8 D_80364A6F;
u8 D_80364A70;
s8 D_80364A71;
s16 D_80364A72;
u8 D_80366EF4_eu[4];
s16 D_80364A74;
u8 D_80366EFA_eu[2];
u8 D_80366EFC_eu[4];
s32 D_80364A78;
s32 D_80364A80;
u8 D_80364A84;
u8 D_80364A7C; /* eu has it here */
u8 D_80364A85;
u8 D_80364A86;
u8 D_80364A87;
u64 D_80364A88;
u64 D_80364A90;
u64 D_80364A98;
u64 D_80364AA0;
s32 D_80364AA8;
u8 D_80364AAC[4];
u8 D_80364AB0[4];
f32 D_80364AB4;
f32 D_80364AB8;
f32 D_80364ABC;
u8 D_80364AC0;
u8 D_80364AC1;
s32 D_80364AC4;
u32 D_80364AC8;
u32 D_80364ACC;
u64 D_80364AD0;
u8 D_80366F60_eu[0x10];
u8 D_80366F70_eu; /* the language: 0 English, 1 German, 2 French */
u8 D_80364AE8;
u8 D_80364AE9;
u8 D_80364AEA;
PlayerInfo D_80364AF0[4];
u16 D_80364EF0[4][16];
u16 D_80364F70[0x78];
u8 D_80365060[5];
u8 D_80365065;
s8 D_80365066;
u8 D_80365067[1];
u8 D_80365068[4];
s32 D_8036506C;
#else
u8 D_8030F660;
s32 D_8030F664;
u8 D_8030F668;
u8 D_8030F669;
u8 D_8030F66A;
u8 D_8030F66B[1];
u8 D_8030F66C[4];
u8 D_8030F670[0x11B0];
OSThread D_80310820;
u64 D_803109D0[0x40];
OSThread D_80310BD0;
u64 D_80310D80[0x2000/sizeof(u64)];
u64 D_80312D80[0x400];
OSMesgQueue D_80314D80;
OSMesg D_80314D98[0xc2];
OSMesgQueue D_803150A0;
OSMesg D_803150B8[0x32];
OSMesgQueue D_80315180;
OSMesg D_80315198[0x90];
OSMesgQueue D_803153D8;
u8 D_803153F0;
OSMesg D_803153F8[0x10];
u8 D_80315438;
Sched D_80315440; /* D_803156A4 is its audioListHead, D_803156C0 unk280, D_803156C4 frameCount */
SchedClient D_803156D8;
SndState *D_803156E8;
SndState *D_803156EC;
SndState *D_803156F0;
u8 D_803156F4;
u8 D_803156F5;
Frame D_803156F8[2];
u8 D_80358028[8];
Gfx *D_80358030[2];
Gfx *D_80358038[2];
Gfx *D_80358040[2];
Gfx *D_80358048[2];
u16 *D_80358050[2];
u16 *D_80358058;
u8 D_8035805C;
u32 D_80358060;
s32 D_80358064;
s32 D_80358068;
u8 *D_8035806C;
u8 *D_80358070;
LevelHeader *D_80358074;
s32 D_80358078;
u16 D_8035807C;
s32 D_80358080;
s32 D_80358084;
u8 D_80358088[0xc340];
UnkStruct_803643C8 *D_803643C8;
UnkStruct_803643C8 *D_803643CC;
u8 D_803643D0[4];
u8 D_803643D4;
u8 D_803643D5;
u8 D_803643D6;
u8 D_803643D7;
u8 D_803643D8;
u8 D_803643D9;
u8 D_803643DA;
u8 D_803643DB;
u8 D_803643DC;
s32 D_803643E0;
s32 D_803643E4;
s32 D_803643E8;
s32 D_803643EC;
s32 D_803643F0;
s32 D_803643F4;
s32 D_803643F8;
s32 D_803643FC;
s32 D_80364400;
s32 D_80364404;
s32 D_80364408;
s32 D_8036440C;
u8 D_80364410;
u8 D_80364411;
u8 D_80364412;
f32 D_80364414;
f32 D_80364418;
u8 D_8036441C;
u8 D_8036441D;
s32 D_80364420;
u8 D_80364424;
u32 D_80364428;
u16 D_8036442C;
s32 D_80364430;
u8 D_80364434;
f32 D_80364438;
s16 D_8036443C;
s16 D_8036443E;
s16 D_80364440;
f32 D_80364444;
f32 D_80364448;
s16 D_8036444C;
s16 D_8036444E;
u8 D_80364450[2];
s16 D_80364452;
s16 D_80364454;
u8 D_80364456;
u8 *D_80364458;
Vehicle D_80364460[0xc];
Vehicle *D_803649D0;
u64 D_803649D8;
s16 D_803649E0;
s16 D_803649E2;
s16 D_803649E4;
s32 D_803649E8;
u8 D_803649EC;
u8 D_803649ED;
s8 D_803649EE;
u32 D_803649F0;
u32 D_803649F4;
f32 D_803649F8;
UnkStruct_80364A00 D_80364A00[5];
u8 D_80364A3C;
u8 D_80364A3D;
s32 D_80364A40;
s32 D_80364A44;
u8 D_80364A48;
s16 D_80364A4A;
s16 D_80364A4C;
u8 D_80364A4E;
u8 D_80364A4F[1];
u8 D_80364A50;
s32 D_80364A54;
s32 D_80364A58;
s32 D_80364A5C;
u8 D_80364A60;
s32 D_80364A64;
u8 D_80364A68;
u8 D_80364A69;
u8 D_80364A6A;
u8 D_80364A6B;
u8 D_80364A6C;
u8 D_80364A6D;
u8 D_80364A6E;
u8 D_80364A6F;
u8 D_80364A70;
s8 D_80364A71;
s16 D_80364A72;
s16 D_80364A74;
s32 D_80364A78;
u8 D_80364A7C;
s32 D_80364A80;
u8 D_80364A84;
u8 D_80364A85;
u8 D_80364A86;
u8 D_80364A87;
u64 D_80364A88;
u64 D_80364A90;
u64 D_80364A98;
u64 D_80364AA0;
s32 D_80364AA8;
u8 D_80364AAC[4];
u8 D_80364AB0[4];
f32 D_80364AB4;
f32 D_80364AB8;
f32 D_80364ABC;
u8 D_80364AC0;
u8 D_80364AC1;
s32 D_80364AC4;
u32 D_80364AC8;
u32 D_80364ACC;
u64 D_80364AD0;
u8 D_80364AD8[0x10];
u8 D_80364AE8;
u8 D_80364AE9;
u8 D_80364AEA;
PlayerInfo D_80364AF0[4];
u16 D_80364EF0[4][16];
u16 D_80364F70[0x78];
u8 D_80365060[5];
u8 D_80365065;
s8 D_80365066;
u8 D_80365067[1];
u8 D_80365068[4];
s32 D_8036506C;
u8 D_80365070[8];
f32 D_80365078;
f32 D_8036507C;
f32 D_80365080;
f32 D_80365084;
f32 D_80365088;
f32 D_8036508C;
f32 D_80365090;
f32 D_80365094;
f32 D_80365098;
s16 D_8036509C;
s16 D_8036509E;
s16 D_803650A0;
#endif

/* .data, 0x802E8BD0-0x802E8C60 (tools/data_c.py) */
u8 D_802E8BD0 = 0;
u8 D_802E8BD4 = 0;
u8 D_802E8BD8 = 0;
s32 D_802E8BDC = 0;
f32 D_802E8BE0 = 1.3f;
u8 D_802E8BE4 = 0;
s32 D_802E8BE8 = 0;
s32 D_802E8BEC = -1;
u8 D_802E8BF0 = 0;
u16 D_802E8BF4[2] = { 0x0A1D }; /* texture ids (func_80272C5C) */
u8 D_802E8BF8 = 0;
s32 D_802E8BFC[0x12] = {
    2000, 2350, 1400, 1700, 1600, 1030, 1030, 2000, 1600, 1650, 100, 0, 2000, 1850, 1450, 1000,
    100, 100,
};
#ifdef VERSION_EU
u8 D_802E8C44[0x10] = { 0, 1, 2, 3, 4, 5, 14, 7, 8, 9, 10, 11, 15, 13, 6, 12 };
s32 D_802EAA54_eu = 0;
s32 D_802EAA58_eu = 0;
u8 D_802E8C54[0x14] = { 10, 27 };
#else
u8 D_802E8C44[0x1c] = { 0, 1, 2, 3, 4, 5, 14, 7, 8, 9, 10, 11, 15, 13, 6, 12, 10, 27 };
#endif


extern u8 D_00787F40[];
extern u8 D_00788000[];
extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern FrameGame D_02000000;
extern s32 D_80000300;
extern u16 D_80000400[][320 * 240];
extern OSThread D_80218D30;
extern s32 D_80219F58;
extern u8 D_8021ED00[];
extern s32 D_802FA254;
extern u8 D_802FA940[];
extern u8 D_802FDB14;
extern u8 D_802FDBD0;
extern u8 D_802FDBD4;
extern u16 D_80304904[];
extern u16 D_80304910[];
extern u16 D_8030491C[];
extern u16 D_80304938[];
extern s32 D_803669B4;
extern u16 D_80366A12;
extern s16 D_80367BD6;
extern u64 *D_8036E694;
extern u8 D_80370C1E;
extern u8 D_80370C21;
extern u8 D_8039CA61;
extern u8 D_8039CA62;
extern u8 D_8039CAA2;
extern u8 D_8039CAB7;
extern u8 D_803A6B04;
extern u8 D_803A7430;
extern u8 D_803B9888;
extern u8 D_803C5770[];
extern u8 D_803C6370[];
extern u8 D_803C6F70[];
extern u8 D_803C7B70[];
extern u8 D_803FF600[];
#ifdef TARGET_PC
extern u8 D_803FFFF8[];
#endif

void func_80244870(void *);
void func_80244930(void *);
u8 func_8024AFA8(s32);
void func_8024F520(Gfx **, Frame *);
void func_8024E4F4(Gfx **, Frame *, u8);
void func_802502EC(void);
void func_802507C8(Mtx *, LookAt *, Mtx *);
u8 func_80255628(void);
void func_802558C8(Gfx *, s32 *);
void func_802559F8(Gfx *, s32 *);
void func_8025615C(s32, u8 *, s32 *);
void func_80257234(void);
void osCreatePiManager(OSPri, OSMesgQueue *, OSMesg *, s32);
f32 sqrtf(f32);
float fcos(float);

void func_802447C0(void) {
    u32 sp74;
    s32 pad[2];
    s32 sp68;
    u32 sp28[16];
    s32 pad2;

    osInitialize();
    sp68 = 0xFFB000;
    for (sp74 = 0; sp74 < 16; sp74++, sp68 += 4) {
        osPiRawReadIo(sp68, &sp28[sp74]);
    }
    func_80270AE0((u8 *)sp28);
    osCreateThread(&D_80310820, 1, func_80244870, NULL, &D_803109D0[0x200 / sizeof(u64)], 10);
    osStartThread(&D_80310820);
}

void func_80244870(void *arg0) {
    s32 pad[2];

    osDpSetStatus(4);
    osCreatePiManager(150, &D_80314D80, D_80314D98, 194);
    osCreateThread(&D_80310BD0, 3, func_80244930, arg0, &D_80310D80[0x2000 / sizeof(u64)], 10);
    osStartThread(&D_80310BD0);
    if (D_802FA254 == 0) {
        osStartThread(&D_80310BD0);
    }
    osSetThreadPri(NULL, 0);
    for (;;) {
    }
}


extern OSMesgQueue D_80219EF8;
extern OSMesgQueue D_80219F50;
extern u64 D_8021A830;
extern u16 D_803047A0[];
extern u16 D_803047B4[];
extern u16 D_803047CC[];
extern u16 D_803047DC[];
extern u16 D_80367BF6;
extern u8 D_8036E68C[7];
extern u8 D_8036EB92;
extern u8 D_8039C4B8[];
extern u8 D_8039C4F8[];
extern s16 D_8039CAA0;
/* Defined here for the same reason as D_80364A90 above: their stores share one lui. */

void func_802475D8(void);
void func_80255AD0(void);
void func_80255D34(void);

#define UNK_80364AF0_IN_RANGE(p, l) \
    LEVEL_DONE_IN(D_80364AF0[p], l)
#define UNK_802F8BDC_REC() (D_802F8BDC[D_802F4868[func_8026F92C((u32)D_80364AA8)]])

/*
 * Statements that share a line here do so on purpose: IDO won't schedule a
 * store past the start of the next source line, so the original put these on
 * one line (or behind a macro).
 */
void func_80244930(void *arg0) {
    s32 sp64;
    s32 sp60;
    s32 sp5C;
    u8 sp5B;
    s32 sp54;

    func_80255AD0();
    for (;;) {
        D_80364A70 = 0;
        do {
#ifdef TARGET_PC
            port_replay_mode_switch();  /* --replay: the movie's mode, if the port would go elsewhere */
#endif
            func_8029A7E4("game mode switch from %d to %d\n", func_8026F92C(D_80364A90), func_8026F92C(D_80364A98));
            D_80364AA0 = 0;
            switch (D_80364A98) {
                case 0x20:
                    func_80255DC8();
                    func_801EF380(2);
                    break;
                case 0x10:
                    func_80255DC8();
                    func_801EF380(1);
                    sp60 = D_80315440.frameCount;
                    while (D_80315440.frameCount - sp60 < 15) {
#ifdef TARGET_PC
                        port_spin_wait();
#endif
                    }
                    break;
                case 0x100000000:
                    func_801F6F18();
                    func_8026AF6C(0x8012);
                    D_80364A70 = func_80261A44(D_80364A98);
                    break;
                case 0x80000000000:
                    func_80255DC8();
                    func_8025D184();
                    func_80200714(1);
                    osSendMesg(&D_80219EF8, (OSMesg)0x01000001, OS_MESG_BLOCK);
                    func_8026AF6C(0x8011);
                    break;
                case 0x10000:
                    D_80364A70 = func_80261A44(D_80364A98);
                    func_8025D184();
                    func_801EA4B8();
                    func_801E8DCC(D_80364AE8);
                    func_8026AF6C(0x800A);
                    break;
                case 0x10000000:
                    osSendMesg(&D_80219EF8, (OSMesg)0x01000001, OS_MESG_BLOCK);
                    func_801E8DCC(4);
                    func_8026AF6C(0x8011);
                    break;
                case 0x20000000000000:
                    func_802609D0();
                    func_80255DC8();
                    func_802A0700();
                    func_8025D184();
                    func_80200714(1);
                    osSendMesg(&D_80219EF8, (OSMesg)0x0100000F, OS_MESG_BLOCK);
                    osRecvMesgInt(&D_80219F50, sp5C, OS_MESG_BLOCK);
                    if (sp5C != 0 || D_8039C541 != 0) {
                        D_802E8BF8 = 1;
                    } else {
                        D_802E8BF8 = 0;
                    }
                    D_8039C541 = 0;
                    if (D_802E8BF8 == 0) {
                        func_801E8C40(4);
                        D_80364AA0 = 0x10000000;
                    } else {
                        D_80364AE8 = 0;
                        D_80364AE9 = 0;
                        D_80364AEA = 0; osSendMesg(&D_80219EF8, (OSMesg)0x01000010, OS_MESG_BLOCK);
                        osRecvMesgInt(&D_80219F50, D_8039C4B4, OS_MESG_BLOCK);
                        if (D_8039C4B4 == 0) {
                            func_8029A7E4("NO EE PRESENT! - USING DUMMY EE\n");
                        }
                        osSendMesg(&D_80219EF8, (OSMesg)0x01000006, OS_MESG_BLOCK);
                        osRecvMesgInt(&D_80219F50, sp5C, OS_MESG_BLOCK);
                        if (sp5C == 0) {
                            sp5C = func_80201E80();
                        }
                        if (sp5C != 0) {
                            func_801EA108(D_80364AE8, 1, 1);
#ifdef VERSION_EU
                            D_80364AA0 = 0x200000000; /* eu's own mode first (case 0x200000000) */
#else
                            D_80364AA0 = 0x40000;
#endif
                        } else {
                            D_80365060[D_80364AE8] = 1;
                            D_80364AA0 = 0x4000;
                        }
                    }
                    break;
                case 0x4000000000000:
                    func_80255DC8();
                    func_802A0700();
                    func_8025D184();
                    func_80200714(4);
                    func_801E8C40(4);
                    func_8026AF6C(0x8038);
                    break;
                case 0x400000000000000:
                    D_802E8BEC = -1;
                    D_802E8BF0 = 1;
                    D_80365065 = 0;
                    D_80364AA0 = 2;
                    D_8039C541 = 0;
                    func_801ECE9C();
                    break;
                case 0x2:
                    switch (D_802E8BEC) {
                        case 1:
                        case 3:
                        case 4:
                        case 6:
                            if (D_80364A90 == 2) {
                        D_80364A98 = 0x1000000000000;
                        func_80255DC8();
                        func_80200714(7);
                        func_80201240(D_80364AC4 & 3);
                        func_8026AF6C(0x8035);
                        func_801E8C40(D_80364AC4 & 3);
                        D_80364AC4++;
                                break;
                            }
                        default:
                        if (++D_802E8BEC == 9) {
                            D_802E8BEC = 0;
                            D_802E8BF0 = 0;
                        } else {
                            D_802E8BF0 ^= 1;
                        }
                        D_8039CAA0 = 0;
                        func_80255DC8();
                        func_8025D184();
                        func_80295E50();
                        func_8025B9D0(D_802E8BEC, &D_802E8BDC);
                        if (D_802E8BEC == 0) {
                            func_8029A130();
                        }
                        func_80256A34(0);
                            break;
                    }
                    break;
                case 0x4000:
                    if (D_80364AE8 != D_80364AEA) {
                        D_80364AE8 = D_80364AE9 = D_80364AEA;
                    }
                    func_8028B3E0();
                    func_8029A7E4("World screen centred on level %d\n", D_802E8BDC);
                    D_80364AE8 = D_80364AEA;
                    func_801ECE9C();
                    if (D_80365065 == 0) {
                        D_80365065 = 1;
                        D_80364A87 = 0;
                        D_803643D5 = 0;
                        func_801FE018(8);
                        D_802E8BDC = D_80364AF0[D_80364AE8].levelno;
                        func_8029A7E4("going to level %d\n", D_802E8BDC);
                    }
                    D_8039CA60 = 0; if (UNK_80364AF0_IN_RANGE(D_80364AE8, D_802E8BDC) && D_802E8BDC >= 0x2B && D_802E8BDC < 0x2E) {
                        if (!UNK_80364AF0_IN_RANGE(D_80364AE8, D_802E8BDC + 1)) {
                            D_802E8BDC++;
                        }
                    }
                    func_80255DC8();
                    func_801ECC8C();
                    osViBlack(1);
                    func_802A0700();
                    func_801F8530(D_802E8BDC);
                    break;
                case 0x800000000000:
                    D_80364A70 = func_80261A44(D_80364A98);
                    break;
                case 0x400000000:
                    func_8026AF6C(0x8013);
                    break;
                case 0x800000000000000:
                    func_80255DC8();
                    func_80200714(1);
                    func_8025D184();
                    func_8026AF6C(0x8059);
                    break;
                case 0x40000000000:
                    func_80255DC8();
                    func_80200714(3);
                    func_801E8C40(D_80364AE8);
                    func_8025D184();
                    D_8021A830 = D_80364A90;
                    func_8026AF6C(0x8016);
                    break;
                case 0x200000000000000:
                    func_801E8EB8(D_80364AE8, 1);
                    func_801F8228();
                    func_8026AF6C(0x8016);
                    D_80364A98 = 0x40000000000;
                    break;
                case 0x2000:
                    func_80255DC8();
                    if (D_802E8F94[D_802E8BDC].unk0 != 1) {
                        func_80256A34(0);
                        func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA60);
                        func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA80);
                        D_802E8BD8 = 1;
                    } else {
                        if (UNK_80364AF0_IN_RANGE(D_80364AE8, D_802E8BDC)) {
                            func_80256A34(D_8039C4B8);
                        } else {
                            func_80256A34(0);
                        }
                        func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA60);
                        func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA80);
                        D_80364AA0 = 4;
                    }
                    break;
                case 0x4:
                    switch (D_80364A90) {
                        case 0x1000000000:
                            func_8026101C();
                            func_80261E9C(D_80364A98);
                            D_802E8BD4 = 1;
                            break;
                        case 0x100:
                            D_80364412 = 1;
                            break;
                        case 0x2000:
                            if (D_802E8F94[D_802E8BDC].unk0 != 1) {
                                D_80364A70 = func_80261A44(D_80364A98);
                            }
                            D_802E8BD4 = 1;
                            D_802E8BD4 &= !func_8026AD30(0x52);
                            D_802E8BD4 &= !func_8026AD30(0x56); if (UNK_80364AF0_IN_RANGE(D_80364AE8, D_802E8BDC) && D_802E8F94[D_802E8BDC].unk0 == 1) {
                                D_802E8BD4 &= !func_8026AD30(0x55);
                            } else {
                                D_802E8BD4 &= !func_8026AD30(0x57);
                            }
                            D_802E8BD8 = !D_802E8BD4;
                            func_8029A7E4("Unpause at start = %d\n", D_802E8BD4);
                            D_80364A58 = SC_TIMER;
                            D_80358064 = 0;
                            func_8025BB38();
                            func_8029A7E4("snew ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n",
                                          D_8036EA70.ip, D_8036EA70.tc, D_8036EA70.bd, D_8036EA70.cr, D_8036EA70.rt, D_8036EA70.coin,
                                          D_8036EA70.bdn);
                            func_8029A7E4("sold ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n",
                                          D_8036EA60.ip, D_8036EA60.tc, D_8036EA60.bd, D_8036EA60.cr, D_8036EA60.rt, D_8036EA60.coin,
                                          D_8036EA60.bdn);
                            func_8029A7E4("sres ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n",
                                          D_8036EA80.ip, D_8036EA80.tc, D_8036EA80.bd, D_8036EA80.cr, D_8036EA80.rt, D_8036EA80.coin,
                                          D_8036EA80.bdn);
                            func_8029A7E4("srs2 ip=%8d : tc=%5d : bd=%2d : cr=%2d : rt=%3d : coin=%1d : bdn=%1d\n",
                                          D_8036EA90.ip, D_8036EA90.tc, D_8036EA90.bd, D_8036EA90.cr, D_8036EA90.rt, D_8036EA90.coin,
                                          D_8036EA90.bdn);
                            break;
                    }
                    break;
                case 0x100:
                    D_80364412 = 1;
                    break;
                case 0x80:
                    sp5B = 0;
                    switch (D_80364A90) {
                        case 0x100000000000:
                            func_8028B3E0();
                            D_80364A70 = func_80261A44(D_80364A98);
                        case 0x4000:
                            sp5B = func_80285814();
                            break;
                        case 0x40000000000:
                        case 0x80000000000000:
                        case 0x100000000000000:
                            if (D_80364AE8 != D_80364AE9) {
                                D_80364AE9 = D_80364AE8; func_8029A7E4("switching to new player ...........\n");
                                func_80285814();
                            }
                            break;
                    }
                    func_80255DC8();
                    if (sp5B != 0) {
                        osSendMesg(&D_80219EF8, OS_MESG((D_802E8BDC << 8) | 0xD | (D_80364AE8 << 16)), OS_MESG_BLOCK);
                    }
                    D_8020C070[UNK_802F8BDC_REC().first + UNK_802F8BDC_REC().count - 2].flags &= ~1;
                    D_8020C070[UNK_802F8BDC_REC().first + UNK_802F8BDC_REC().count - 2].flags |= 0x800;
                    func_8026B8F8();
                    func_8026AF6C(D_802F4868[func_8026F92C((u32)D_80364AA8)] | 0x8000);
                    UNK_802F8BDC_REC().unk18 = UNK_802F8BDC_REC().first + UNK_802F8BDC_REC().count - 3;
                    UNK_802F8BDC_REC().unk8 &= ~8;
                    func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA60);
                    D_80315438 = func_801EE800(&D_80364A60, 1, 0); func_801E8C40(D_80364AE8);
                    func_801EC30C(D_80315438);
                    func_801EC288(D_80315438);
                    func_80200714(1);
                    D_80364A71 = func_801EF1E0();
                    if (D_80364A71 != -1) {
                        func_801F55D8();
                    }
                    break;
                case 0x800:
                    func_80255DC8();
                    if (D_80364A90 == 0x4000) {
                        osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
                    }
                    osViBlack(1);
                    func_80256A34(0);
                    func_802661EC();
                    if (D_802E8BDC == 0x32) {
                        func_8026AF6C(0x8024);
                    }
                    break;
                case 0x40000000:
                    func_80255DC8();
                    sp54 = D_80315440.frameCount;
                    while (D_80315440.frameCount - sp54 < 15) {
#ifdef TARGET_PC
                        port_spin_wait();
#endif
                    }
                    func_801ED790();
                    func_80200714(3);
                    D_80364A71 = func_801EF1E0();
                    if (D_80364A71 != -1) {
                        func_801F55D8();
                    }
                    func_801E8C40(D_80364AE8);
                    func_801EC30C(D_80315438);
                    break;
                case 0x8000000:
                    switch (D_80364A90) {
                        case 0x40:
                        case 0x400:
                        case 0x20000000:
                            func_80255DC8();
                            func_80200714(1);
                            D_80364A71 = func_801EF1E0();
                            if (D_80364A71 != -1) {
                                func_801F55D8();
                            }
                            func_801E8C40(D_80364AE8);
                            func_80285A78((u8 *)&D_8036EA90, (u8 *)&D_8036EA70);
                            D_80315438 = func_801EE800(&D_80364A60, 1, 0);
                            func_801EC30C(D_80315438);
                            func_801EC288(D_80315438);
                            break;
                        case 0x40000000:
                            D_80364A70 = func_80261A44(D_80364A98);
                            break;
                        case 0x40000000000:
                        case 0x80000000000000:
                        case 0x100000000000000:
                            if (D_80364AE8 != D_80364AE9) {
                                func_8029A7E4("switching to new player ...........\n");
                                D_80364AE9 = D_80364AE8;
                                D_803643D5 = 0;
                                func_80285814();
                            }
                            func_80255DC8();
                            D_80364A71 = func_801EF1E0();
                            if (D_80364A71 != -1) {
                                func_801F55D8();
                            }
                            func_801E8C40(D_80364AE8);
                            D_80315438 = func_801EE800(&D_80364A60, 1, 0);
                            func_80200714(1);
                            func_801EC30C(D_80315438);
                            func_801EC288(D_80315438);
                            break;
                        default:
                            if (D_8039C53C[D_80364AE8] != 0 && D_8039C53C[D_80364AE8] != D_802E8BDC + 1) {
                                func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n",
                                              "!saveIt[playerNumber] || saveIt[playerNumber]==levelno+1",
                                              "./master_switch.c", 488);
                            }
                            D_8039C53C[D_80364AE8] = D_802E8BDC + 1;
                            if (D_803643D7 != 0) {
                                func_802C1DD0(D_802E8F94[D_802E8BDC].unk0 == 0x20 ||
                                              D_802E8F94[D_802E8BDC].unk0 == 0x80);
                            }
                            D_803643D5 = !UNK_80364AF0_IN_RANGE(D_80364AE8, D_802E8BDC);
                            if (D_80364AE8 == D_80364AEA) {
                                D_80364A87 |= D_803643D5;
                            }
                            if (D_80364AA8 == 1 && D_803643D5 == 0) {
                                for (sp64 = 0; sp64 < 0x40; sp64++) {
                                    D_8039C4F8[sp64] = D_8039C4B8[sp64];
                                }
                            }
                            func_802CF5B0();
                            func_80297960();
                            if (D_80364AA8 == 1) {
                                if (!(func_802C4A40(D_8039C4B8) <= 60)) {
                                    func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n",
                                                  "record_status(pakBuffer)<=LEVEL_SAVE_SIZE-4", "./master_switch.c",
                                                  513);
                                }
                                if (D_8039C540 != 0 && D_8039C540 != D_802E8BDC + 1) {
                                    func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n",
                                                  "!saveLevel || saveLevel==levelno+1", "./master_switch.c", 515);
                                }
                                D_8039C540 = D_802E8BDC + 1;
                                if (func_8028604C(D_80364A5C) + D_8036EA70.tc >= 60000) {
                                    D_8036EA70.tc = 59999;
                                } else {
                                    D_8036EA70.tc = func_8028604C(D_80364A5C) + D_8036EA70.tc;
                                }
                            } else if (D_803643D7 != 0) {
                                func_80264AEC();
                                D_8036EA70.tc = D_802E8F94[D_802E8BDC].medalTimes[3] - D_80367BF6;
                            } else {
                                D_8036EA70.tc = 0xFFFF;
                            }
                            if (D_8036EA70.tc == 0) {
                                func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n", "nss.tc",
                                              "./master_switch.c", 530);
                            }
                            if (D_8036EA70.tc == 0) {
                                D_8036EA70.tc = 1;
                            }
                            func_80255DC8();
                            if ((u8)D_80365066 != 0) {
                                D_8036EA70.bd = D_8036EB92;
                                D_80365066 = 0;
                            }
                            if (D_803643D5 != 0) {
                                func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA60);
                            } else {
                                func_80285A78((u8 *)&D_8036EA80, (u8 *)&D_8036EA60);
                            }
                            D_80315438 = func_801EE800(&D_80364A60, 1, 1); func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA90);
                            if (D_80364A60 != 0) {
                                D_80364AA0 = 0x40000000;
                                D_80364A64 = 0xDC;
                                D_80364A70 = 0;
                            } else {
                                func_801EC30C(D_80315438);
                                func_80200714(1);
                                D_80364A71 = func_801EF1E0();
                                if (D_80364A71 != -1) {
                                    func_801F55D8();
                                }
                                func_801E8C40(D_80364AE8);
                            }
                            D_8020C070[UNK_802F8BDC_REC().first + UNK_802F8BDC_REC().count - 2].flags |= 1;
                            D_8020C070[UNK_802F8BDC_REC().first + UNK_802F8BDC_REC().count - 2].flags &= ~0x800;
                            break;
                    }
                    if (D_803643D5 != 0 && D_80364AE8 == D_80364AEA) {
                        UNK_802F8BDC_REC().unk18 = UNK_802F8BDC_REC().first + UNK_802F8BDC_REC().count - 1;
                    } else {
                        UNK_802F8BDC_REC().unk18 = UNK_802F8BDC_REC().first + UNK_802F8BDC_REC().count - 3;
                    }
                    UNK_802F8BDC_REC().unk8 |= 8;
                    func_8026B8F8();
                    if (D_802E8BF8 != 0) {
                        D_8020C070[FE_ENTRY(29)].flags &= ~1;
                    }
                    func_8026AF6C(D_802F4868[func_8026F92C((u32)D_80364AA8)] | 0x8000);
                    break;
                case 0x40:
                    if (D_80364A90 & 0x04000200) {
                        func_8028B3E0();
                        D_8036EA70.tc = func_8028604C(D_80364A5C); func_80285A78((u8 *)&D_8036EA70, (u8 *)&D_8036EA60);
                        func_801EE800(&D_80364A60, 0, 1);
                    }
                    D_802F5804[YOSHI_ENTRY(24)].flags |= 1;
                    D_802F5804[YOSHI_ENTRY(24)].flags &= ~0x800;
                    D_802F5804[YOSHI_ENTRY(23)].flags |= 1;
                    D_802F5804[YOSHI_ENTRY(23)].flags &= ~0x800;
                    D_802F5804[YOSHI_ENTRY(17)].flags &= ~1;
                    D_802F5804[YOSHI_ENTRY(17)].flags |= 0x800;
                    if (UNK_80364AF0_IN_RANGE(D_80364AE8, D_802E8BDC)) {
                        D_802F8BDC[6].unk18 = 0x18; D_802F8BDC[7].unk18 = 0x22;
                    } else {
                        D_802F8BDC[6].unk18 = 0x17; D_802F8BDC[7].unk18 = 0x21;
                    }
                    D_80364A50 = 0;
                    D_80364A54 = SC_FRAMECOUNT;
                    func_80255DC8();
                    if (D_802E8F94[D_802E8BDC].unk0 == 1 && UNK_80364AF0_IN_RANGE(D_80364AE8, D_802E8BDC) &&
                        D_803643D5 == 0) {
                        func_80256A34(D_8039C4F8);
                    } else {
                        func_80256A34(0);
                    }
                    func_8026B8F8();
                    func_8025BB50();
                    break;
                case 0x20000000:
                    func_80255DC8();
                    if (D_80364A90 == 0x4000) {
                        osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
                    }
                    osViBlack(1);
                    func_802A0700();
                    if (func_801E7000() != 0) {
                        func_80200714(2);
                    } else {
                        D_80364AA0 = 0x2000;
                    }
                    break;
                case 0x40000:
                    osSendMesg(&D_80219EF8, OS_MESG((D_80364AE8 << 16) | 0x14 | 0x01000000), OS_MESG_BLOCK);
                    osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
                    func_801EA93C(TEXT_EU("ENTER NAME!", "NAME EINGEBEN!"), U16TEXT(D_803047A0), 7, 0x1E, D_80364AF0[D_80364AE8].name);
                    func_8026AF6C(0x800B);
                    func_8025D184();
                    D_80364A70 = func_80261A44(D_80364A98);
                    break;
                case 0x4000000000000000:
                    func_80255DC8();
                    func_80200714(1);
#ifdef VERSION_EU
                    ENTRY_TEXT(&D_8020C070[FE_ENTRY(9)]) = TEXT_EU("QUIT GAME!", "SPIEL VERLASSEN!"); D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0x16;
#else
                    D_8020C070[FE_ENTRY(9)].text = "QUIT GAME!"; D_8020C070[FE_ENTRY(9)].unk10 = D_803047CC; D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0x16;
#endif
                    func_8026AF6C(0x800C);
                    break;
                case 0x1000000000:
                    func_80260EE0(0x18);
                    func_80260B40(0, 0);
                    func_80297ECC();
                    func_8026AF6C(0x8015);
                    D_802E8BD8 = 1;
                    break;
                case 0x100000000000:
                    func_80299C0C();
                    D_80364A70 = func_80261A44(D_80364A98);
                    func_80255DC8();
                    func_80299C20();
                    if (D_80364A90 == 0x4000 || D_802E8BDC == 0x2F) {
                        osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
                    }
                    osViBlack(1);
                    func_80256A34(0);
                    break;
                case 0x40000000000000:
                    func_80255DC8();
                    osSendMesg(&D_80219EF8, (OSMesg)0x01000010, OS_MESG_BLOCK);
                    osRecvMesgInt(&D_80219F50, D_8039C4B4, OS_MESG_BLOCK);
                    if (D_8039C4B4 != 0) {
                        func_8025D184();
                        func_80200714(1);
#ifdef VERSION_EU
                        ENTRY_TEXT(&D_8020C070[FE_ENTRY(9)]) = TEXT_EU("ERASE SAVED GAME!", "SPIEL LOESCHEN!"); D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0x14;
#else
                        D_8020C070[FE_ENTRY(9)].text = "ERASE SAVED GAME!"; D_8020C070[FE_ENTRY(9)].unk10 = D_803047B4; D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0x14;
#endif
                        func_8026AF6C(0x800C);
                    } else {
                        D_80364AA0 = 0x10;
                    }
                    break;
                case 0x100000000000000:
#ifdef VERSION_EU
                    ENTRY_TEXT(&D_8020C070[FE_ENTRY(9)]) = TEXT_EU("BECOME GUEST PLAYER:", "GASTSPIELER FUNKTION:"); D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0x14;
#else
                    D_8020C070[FE_ENTRY(9)].text = "BECOME GUEST PLAYER:"; D_8020C070[FE_ENTRY(9)].unk10 = D_803047DC; D_8020C070[FE_ENTRY(9)].unk6 = D_8020C070[FE_ENTRY(9)].unk8 = 0x14;
#endif
                    func_8026AF6C(0x800C);
                    break;
                case 0x400000:
                    func_801EA6E8();
                    func_8026AF6C(0x800A);
                    break;
                case 0x8:
                    D_802E8BD8 = 1;
                    func_80260E80();
                    break;
#ifdef VERSION_EU
                case 0x200000000:
                    D_80364A70 = func_80261A44(D_80364A98);
                    func_8026AF6C(0x806C);
                    D_802F8BDC[0x6C].unk18 = D_80366F70_eu + 0xD8;
                    break;
#endif
            }
            D_80364A88 = D_80364A90;
            D_80364A90 = D_80364A98;
            D_80364A98 = D_80364AA0;
        } while (D_80364A98 != 0);

        if (D_80364A70 != 0) {
            func_80261FB0(D_80364A70);
        }
        while (D_80364A98 == 0) {
            D_80364AD0 = osGetTime();
            func_8025B2B8();
            if (D_80364A90 & 0x4000) {
                D_80364ACC = (osGetTime() - D_8036BF38) / FRAME_TICKS_100;
                func_801F8980();
                D_80364AC8 = (osGetTime() - D_80364AD0) / FRAME_TICKS_100;
                if (D_8036E68C[2] != 0) {
                    func_80285110(0x4D2);
                }
                if (D_8036E68C[0] != 0) {
                    func_80285110(0x4D2);
                }
                func_80285110(0x4D2);
            } else if (D_80364A90 & 0xC9FD8FE7DBFF8080) {
                D_80364ACC = (osGetTime() - D_8036BF38) / FRAME_TICKS_100;
                func_801FE990();
                D_80364AC8 = (osGetTime() - D_80364AD0) / FRAME_TICKS_100;
                func_80285110(0x4D2);
            } else if (D_80364A90 & 0x20000000) {
                func_801E7598();
                func_80285110(0x4D2);
            } else if (D_80364A90 & 0x30) {
                func_801EF4AC();
                func_80285110(0x4D2);
            } else {
                D_80364ACC = (osGetTime() - D_8036BF38) / FRAME_TICKS_100;
                func_802475D8();
                D_80364AC8 = (osGetTime() - D_80364AD0) / FRAME_TICKS_100;
                if (D_8036E68C[1] != 0) {
                    func_80285110(0x61F);
                }
                if (D_8036E68C[2] != 0) {
                    func_80285110(0x54D);
                }
                func_80285110(0x4D2);
            }
            func_80255D34();
            D_80358060++;
            D_80358064++;
            if (D_802E8BD0 == 0) {
                D_80358068++;
            }
        }
        if ((D_80364A90 & 0x104) && (D_803643D6 != 0 || D_803643D7 != 0)) {
            func_8025BBE8(0, 0, 0);
        }
        if (func_802753C0() != 0) {
            func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n", "!areWeFading()", "hd.c", LINE_EU(627, 629));
        }
        switch (D_80364A98) {
            case 0x400000000000:
                func_80299E10(1);
                break;
            case 0x200000000000:
                func_80299E10(0);
                break;
            case 0x2000000000000:
                func_80286330();
                break;
        }
        if (D_80364AE8 == D_80364AEA && D_80364A98 == 0x4000 && func_8028653C() != 0) {
            func_802860F0();
        }
    }
}
extern s32 D_802FA268;
/* .bss, defined here so the osGetTime() store shares one lui (see D_80364A90). */
extern u16 D_80367BC8;
extern u16 D_8036BB16;
extern s16 D_8036BB1A;
extern f32 D_8036BB34;
extern u8 D_8036EB93;
extern u8 D_80370C22;
extern s32 D_80370C38;
extern s16 D_8039CAA0;
extern s8 D_803A7426;
extern s32 D_803F7688;
void func_8024A348(void);
void func_8024A92C(u32);
void func_8024ADD8(void);
void func_8024AE2C(void);
void func_8024B188(void);
void func_8024B5E8(void);
void func_8024B618(void);
void func_8024B7AC(void);
void func_8024B8F4(Mtx *, Mtx *);
void func_8024BDA4(u16 *);
Gfx *func_8024C414(Frame *, s32 *);

void func_802475D8(void) {
    s32 sp6C;
    s32 sp68;
    s32 sp64;
    u8 sp63;
    Gfx *sp5C;
    u8 sp5B;
    s32 sp54;
    s32 sp50;
    s32 sp4C;
    s16 sp4A;

    if (D_802FA268 != 0 && (D_80370C28 & 1) && !(D_80370C2A & 1) && (D_80364A90 & 0x104)) {
        D_803643DA = 1;
        D_803643D9 = 0;
        if (D_80370C28 & 8) {
            if (D_80364AA8 != 1 ||
                !LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC)) {
                D_803643DA = 0;
                D_803643D9 = 1;
            }
        }
        if ((D_80370C28 & 0x2000) && D_803643DA != 0) {
            D_803F7688 = 1;
            D_8036EA70.rt = D_8036EB90;
            D_8036EA70.cr = D_8036EB93;
            D_80365066 = 1;
            func_80285AB0(2);
            func_80285AB0(1);
        }
        if (LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) &&
            D_80364AA8 == 1 && D_803643DA != 0) {
            D_802E8BD8 = 1;
            func_80275390(0x08000000);
        }
    }
    D_80358080 = 0;
    D_80358084 = 0;
    D_803A7426 = 0;
    if (D_80358060 != 0) {
        if (D_8035805C == 0) {
            if (D_80358060 >= 6) {
                func_8024B8F4(&D_803156F8[D_8035805C].game.unk0[2],
                              &D_803156F8[D_8035805C].game.unk0[6]);
            }
        } else {
            sp68 = 0;
            sp63 = 0;
            do {
                if (D_80364460[sp68].type == D_80364456) {
                    sp63 = 1;
                } else {
                    sp68++;
                }
            } while (sp63 == 0);
            sp64 = 0;
            sp63 = 0;
            do {
                if (D_803643C8[sp64].unk1022 == D_80364456) {
                    sp63 = 1;
                } else {
                    sp64++;
                }
            } while (sp63 == 0);
            func_80258544(D_803643C8[sp64].unk0, D_803643C8[sp64].unk1004, D_803643C8[sp64].unk1010,
                          D_803643C8[sp64].unk100C, D_803643C8[sp64].unk1000, D_80364460[sp68].unk54,
                          D_80364460[sp68].unk0, D_80364460[sp68].unk4);
        }
    }
    sp68 = 0;
    sp63 = 0;
    do {
        if (D_80364460[sp68].type == D_80364456) {
            sp63 = 1;
        } else {
            sp68++;
        }
    } while (sp63 == 0);
    if (D_8035805C == 0) {
        func_80279778(D_803643E0, D_803643E4, D_803643E8, D_803643F8, D_803643FC, D_80364400,
                      D_80364460[sp68].unk58, D_80364460[sp68].unk0, D_80364460[sp68].unk8, D_80364460[sp68].unk60);
    } else {
        func_80279778(D_803643E0, D_803643E4, D_803643E8, D_803643F8, D_803643FC, D_80364400,
                      D_80364460[sp68].unk58, D_80364460[sp68].unk0, D_80364460[sp68].unk4, D_80364460[sp68].unk60);
    }
    D_803649D8 = osGetTime();
    func_8028A3E4();
    func_80284E54(D_803156F8[D_8035805C].game.unk48B0, D_80358078, 3, 1, 1234, 1);
    func_802A5720();
    D_8035805C ^= 1;
    switch (D_802E8BD0) {
        case 0:
            if (D_802E8BD8 != 0) {
                D_802E8BD0 = 1;
                D_802E8BD4 = 0;
                D_802E8BD8 = 0;
                D_803156F4 = D_8035805C ^ 1;
                if (D_80364A90 & 0x104) {
                    if (D_8036EB99 == 0) {
                        func_80260B40(0, 0);
                    }
                    func_80261570(0.5f);
                    if (D_80364A90 == 4 && (D_803643DB != 0 || D_80364AC1 != 0) && D_8036BB18 == 0) {
                        D_80364A98 = 0x100;
                        func_802A45D4(10);
                        D_80364A78 = 0;
                    }
                }
            }
            break;
        case 1:
            if (D_802E8BD4 != 0 && D_8035805C != D_803156F4) {
                D_802E8BD8 = 0;
                D_802E8BD4 = 0;
                D_802E8BD0 = 0;
                if (D_80364A90 & 0x104) {
                    if (D_80358064 != 0) {
                        func_8029A7E4("LOCKING KEYS!\n");
                        D_80370C38 = 1;
                    }
                    func_80261E9C(D_80364A90);
                    func_80261570(1.0f);
                    if (D_80364A90 == 0x100 && (D_803643DB != 0 || D_80364AC1 != 0)) {
                        D_80364A98 = 4;
                        func_802A45D4(10);
                    }
                    if (D_80358068 == 0) {
                        D_80358064 = 0;
                    }
                }
            }
            break;
    }
    if (D_802E8BD0 != 0) {
        D_803156F5 = D_803156F4;
    } else {
        D_803156F5 = D_8035805C;
    }
    if (D_803643DB != 0 && (D_80364A90 & 0x104)) {
        func_802BA148();
    }
    if (D_803643DC != 0 && (D_80364A90 & 0x1905)) {
        func_802B8794();
    }
    func_802683E0();
    D_803649EE = 0;
    if (D_80364456 == 0 && !(D_80364A90 & 0x1801)) {
        func_8024AE2C();
    }
    func_8028A470();
    if (D_80364A90 & 0x104) {
        sp5B = 0;
        if ((D_80370C28 & 4) && !(D_80370C28 & 8)) {
            if (D_802E8BE0 < 1.5 && D_80364A90 == 4) {
                sp5B = 2;
                if (!(D_80370C2A & 4)) {
                    func_80260650(D_80367738, 0xDC, NULL);
                }
            } else if (!(D_80370C2A & 4) && D_80364A90 != 0x100) {
                sp5B = 3;
            }
        }
        if (sp5B == 0) {
            if ((D_80370C28 & 8) && !(D_80370C28 & 4)) {
                if (D_802E8BE0 > 1.0 && D_80364A90 == 4 && D_80364A7C == 0) {
                    sp5B = 1;
                    if (!(D_80370C2A & 8)) {
                        func_80260650(D_80367738, 0xDD, NULL);
                    }
                } else if (!(D_80370C2A & 8) && D_80364A90 == 0x100) {
                    sp5B = 4;
                }
            } else {
                D_80364A7C = 0;
            }
        }
        switch (sp5B) {
            case 0:
                break;
            case 2:
                D_802E8BE0 += 0.05;
                if (D_802E8BE0 > 1.5) {
                    D_802E8BE0 = 1.5f;
                }
                break;
            case 1:
                D_802E8BE0 -= 0.05;
                if (D_802E8BE0 < 1.0) {
                    D_802E8BE0 = 1.0f;
                }
                break;
            case 3:
                if (D_803643DB != 0 || D_80364AC1 != 0) {
                    func_802A45D4(10);
                    D_80364A98 = 0x100;
                    D_80364A78 = 0;
                    func_80260650(D_80367738, 0xDF, NULL);
                }
                break;
            case 4:
                D_80364A7C = 1;
                if (D_803643DB != 0 || D_80364AC1 != 0) {
                    func_802A45D4(10);
                    D_80364A98 = 4;
                    func_80260650(D_80367738, 0xDF, NULL);
                }
                break;
        }
    }
    if (D_80364A90 == 0x100 && D_802E8BD0 != 0 && D_8036BB1C == 2 && D_803643DB != 0 && D_8036BB18 == 0) {
        if (!(D_80370C28 & 0x2000) && !(D_80370C28 & 0x10)) {
            D_80364A80 = 0;
        } else {
            D_80364A80 += 100;
            if (D_80364A80 > 800) {
                D_80364A80 = 800;
            }
            if (D_80370C28 & 0x2000) {
                D_80364A78 += D_80364A80;
            }
            if (D_80370C28 & 0x10) {
                D_80364A78 -= D_80364A80;
            }
        }
        sp54 = D_803EF6E4 + D_80364A78;
        sp50 = D_803EF6F8 - 400;
        sp4C = D_803EF6F8 + D_803EF6F0;
        if (sp4C < sp54) {
            D_80364A78 = sp4C - D_803EF6E4;
        }
        if (sp54 < sp50) {
            D_80364A78 = sp50 - D_803EF6E4;
        }
        if (D_80370C28 & 0x2010) {
            if (D_802F8BDC[0].unk4 - 24 < -256) {
                D_802F8BDC[0].unk4 = -256;
            } else {
                D_802F8BDC[0].unk4 -= 24;
            }
            D_8036BB34 = (D_8036BB34 * 0.8 > 0.1) ? D_8036BB34 * 0.8 : 0.1;
            D_802F8BDC[0].unk8 &= ~0x20;
        } else {
            if (D_802F8BDC[0].unk4 + 32 > 32) {
                D_802F8BDC[0].unk4 = 32;
            } else {
                D_802F8BDC[0].unk4 += 32;
            }
            D_8036BB34 = (D_8036BB34 / 0.8 < 1.0) ? D_8036BB34 / 0.8 : 1.0;
            if (D_8036BB34 == 1.0) {
                D_802F8BDC[0].unk8 |= 0x20;
            }
        }
    } else {
        D_80364A80 = 0;
        if (D_802F8BDC[0].unk4 + 32 > 32) {
            D_802F8BDC[0].unk4 = 32;
        } else {
            D_802F8BDC[0].unk4 += 32;
        }
        D_8036BB34 = (D_8036BB34 / 0.8 < 1.0) ? D_8036BB34 / 0.8 : 1.0;
        if (D_8036BB34 == 1.0) {
            D_802F8BDC[0].unk8 |= 0x20;
        }
    }
    if (D_80370C22 != 0 && D_802E8BD0 == 0 && D_80364456 != 0 && D_8036443C == 0 &&
        (D_80364AA8 == 1 || D_80364AA8 == 0x80)) {
        func_8024B188();
    }
    func_8024B618();
    if ((D_80370C28 & 0x4000) && !(D_80370C2A & 0x4000) && func_802753C0() == 0 && D_80364A98 == 0) {
        switch (D_80364A90) {
            case 0x100000000000:
                if (D_802E8BDC != 0x2F && D_802E8BDC != 0x31 && (D_802E8BDC != 0x26 || D_80364A88 == 0x4000)) {
                    func_80260650(D_80367738, 0xDE, NULL);
                    D_80364A98 = 0x4000;
                }
                break;
            case 0x40:
            case 0x400:
                func_80260650(D_80367738, 0xDE, NULL);
                if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) {
                    D_80364A98 = 0x08000000;
                } else {
                    D_80364A98 = 0x4000;
                }
                break;
            case 0x800:
            case 0x1000:
                if (D_802E8BDC != 0x32 || D_80364A88 == 0x4000) {
                    func_80260650(D_80367738, 0xDE, NULL);
                    D_80364A98 = 0x4000;
                }
                break;
        }
    }
    if (D_803643DC != 0 && (D_802E8BD0 == 0 || (D_80364A90 & 0x1801))) {
        func_802B899C();
        if (D_80364A90 & 0x1801) {
            if (func_802753C0() == 0) {
                if (D_80364A98 == 0 || (D_80364A98 & 0x1801)) {
                    func_802AEEC8();
                }
                func_802B8AE4();
            }
        } else {
            func_8026510C();
        }
    }
    if (D_802E8BD0 == 0 && D_80364A84 == 0) {
        func_8024B7AC();
    }
    if (D_8039CA62 != 0 && D_802E8BD0 == 0) {
        func_80294F00();
    }
    if (D_803643DB != 0 && (D_80364AA8 & 0x81)) {
        if (!(D_80364A90 & 0x1801)) {
            func_802BA354();
        }
        if ((D_80364A90 & 0x104) && func_8026B10C() == 0 && D_802E8BD0 == 0 && D_803643D7 == 0 && D_803643D6 == 0) {
            func_8024A92C(sp6C = func_802C1B9C());
        }
    }
    if (D_80364AC1 != 0 && D_802E8BD0 == 0) {
        func_802D291C();
        if (D_802E8BDC == 0x32 && D_8036BB1C == 1 && D_80358060 > 50 && (D_80364A90 & 0x1801) &&
            func_802753C0() == 0) {
            func_80275390(0x2000);
        }
    }
    func_8029DDC8();
    if (D_802E8BD0 == 0) {
        D_803ED40C = 0;
        D_803F7806 = func_802C1AA0();
        func_802BC5E0();
        func_8024B5E8();
        func_8028F794(D_80364456);
        func_80291FAC(D_80364456);
        func_802688C4(D_802E8BDC);
        if (!(D_80364A90 & 0x1801)) {
            func_8026FEC4();
        }
        func_80281CE4();
        if (D_80364A90 & 0x104) {
            func_80297804(D_803643E0, D_803643E4, D_803643E8);
        }
        func_8029E0AC();
        if ((D_80364A90 & 0x104) && (D_80364AA8 & 1)) {
            func_8024A348();
            func_8024ADD8();
        }
        if (D_80364AA8 != 1 && D_80367C00 != 0 && SC_TIMER - D_80364A58 > 90) {
            D_80367C00 = 0;
            func_802794A4();
        }
    }
    if (D_803643D6 != 0 || D_803643D7 != 0 ||
        (D_802E8BDC == 0x32 && D_8036EB98 != 0 &&
         !LEVEL_DONE_IN(D_80364AF0[D_80364AE8], 0x32))) {
        if (D_80364AA8 == 1 && D_80364A5C == 0) {
            func_8029A7E4("TIME IN LEVEL=%d\n", D_80364A5C = SC_TIMER - D_80364A58);
        }
    }
    func_802A5510(D_80358074);
    if (D_80364A90 & 0x1801) {
        func_802A45D4(2);
    }
    if (D_8036BB18 == 0x58 && D_8036BB1C != 8 && D_80364410 == 0) {
        if (D_80364A85 == 0) {
            func_802A45D4(10);
        }
    } else if (D_80364A85 != 0) {
        func_802A45D4(10);
    }
    func_802BD1F8(D_803156F8[D_8035805C].game.unkA580,
                  D_803156F8[D_8035805C].game.unkA918,
                  &D_803156F8[D_8035805C].game.unkA918[0x73],
                  &D_803156F8[D_8035805C].game.unkA918[0x21A7],
                  D_803156F8[D_8035805C].game.unk0,
                  &D_803156F8[D_8035805C].game.unk2C0[0x2D],
                  D_803156F8[D_8035805C].game.unk21410,
                  D_803156F8[D_8035805C].game.unk21478);
    if (D_80364A90 == 4) {
        func_80295C70(D_802E8BDC, D_803643E0, D_803643E8);
    }
    func_802A4CDC(D_80358030[D_8035805C], D_80358038[D_8035805C], D_80358040[D_8035805C], D_80358048[D_8035805C],
                  D_803156F8[D_8035805C].game.unkA4E0);
    func_8027E9B8(D_8035805C);
    if (D_802E8BD0 == 0 || (D_803643DB != 0 && D_803643D6 != 0) || D_8036EB99 != 0) {
        func_802C0574();
        func_802BCA2C();
    }
    if (D_802E8BD0 == 0) {
        func_8028DF14(D_80364456);
        func_802906C0(D_80364456);
        func_80292830();
        func_8028C874(D_80364456);
    }
    if (D_80364AA8 != 1) {
        if (D_80364A90 & 0x40) {
            func_8026420C();
        }
        if (D_80364A90 & 0x04002104) {
            func_80262BF4();
        }
    } else if (D_803BE738 != 0) {
        if (!LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC)) {
            D_803643D9 = 1;
        } else if (func_802753C0() == 0) {
            D_803643DA = 1;
            func_80260650(D_80367738, 0x2B, NULL);
            if (D_80364A90 == 0x40) {
                func_80275390(0x40);
            } else {
                func_80275390(0x08000000);
            }
        }
    }
    func_802A64A4();
    func_802886A0();
    if (D_802E8BD0 == 0) {
        func_802CF1A4();
    }
    func_80279514(D_803643E0, D_803643E4, D_803643E8, D_803643F8, D_803643FC, D_80364400);
    func_80277620(D_80358060);
    func_8027D810(D_802E8BDC);
    func_802C2054();
    if (D_802E8BD0 == 0) {
        func_8026A9B4();
    }
    sp5C = func_8024C414(&D_803156F8[D_8035805C], &D_80358078);
    if ((D_80364A90 & 0x04002444) && D_80364AA8 != 1) {
        sp5C = func_802639B4(sp5C, &D_803156F8[D_8035805C], &D_80358078);
    }
    if (D_80364A90 & 0x100000000002) {
        if (D_80364A90 == 2) {
            sp5C = func_8025C878(sp5C, &D_803156F8[D_8035805C], D_8035805C, &D_80358078);
        }
        if (D_802E8BF0 != 0 || D_80366A12 == 3 || D_802E8BEC >= 9) {
            if ((D_8036BB1C == 1 || D_8036BB1C == 8 || D_802E8BEC >= 9) &&
                (D_802E8BF0 == 0 || D_80358060 <= D_80366A04)) {
                if (D_8039CAA0 + 20 >= 256) {
                    D_8039CAA0 = 255;
                } else {
                    D_8039CAA0 += 20;
                }
            } else if (D_8039CAA0 - 20 < 0) {
                D_8039CAA0 = 0;
            } else {
                D_8039CAA0 -= 20;
            }
        }
        if (D_8039CAA0 != 0 && D_802E8BEC != 0 && D_8039CAA2 != 0) {
            sp5C = func_80295EFC(&D_803156F8[D_8035805C], sp5C, D_8039CAA0 / 2 - 103,
                                 85 - ((D_80364A90 == 0x100000000000) << 5), D_8039CAA0);
        }
        if (D_802E8BEC == 0 && D_80366A12 == 3 && D_80364A90 == 2) {
            sp5C = func_8029A1A8(D_803156F8, sp5C);
        }
        if (D_80364A90 == 2 || D_802E8BDC == 0x2F) {
            func_8025C5D0();
        }
    }
    if (D_80364A90 & 0x1801) {
        if (D_80358060 == 150 && D_802E8BDC != 0x32) {
            func_8026AF6C(0x8040);
        }
        if (D_80364AF0[D_80364AE8].gameState == 0 && D_80358060 == 10) {
            func_8026AF6C(0x803F);
        }
    }
    sp5C = func_8024C404(sp5C, &D_803156F8[D_8035805C], &D_80358078);
    if ((D_80364A90 & 0x440) && SC_FRAMECOUNT - D_80364A54 > 160 && D_80364A50 == 0) {
        D_80364A50 = 1;
        if (D_8036BB1C == 1) {
            if (!LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) ||
                D_80364AA8 == 1) {
                func_8026AF6C(D_802F4870[func_8026F92C((u32)D_80364AA8)] | 0x8000);
            }
        }
    }
    if (D_80364A90 == 8 && func_802753C0() == 0 &&
        (func_802D4E10(D_80367734) == 0 || SC_FRAMECOUNT - D_80367740 > 5 * FRAMES_PER_SECOND)) {
        func_80275390(0x08000000);
    }
    if (D_802E8BDC == 0x31 && D_8036BB1C == 1) {
        sp5C = func_8029A518(&D_803156F8[D_8035805C], sp5C);
    }
    func_80259C24(&sp5C, &D_803156F8[D_8035805C]);
    if (D_80364A90 != 2) {
        sp5C = func_80274BF0(&D_803156F8[D_8035805C], sp5C);
    }
    if (D_80364A90 == 0x2000000000000000) {
        if ((D_80370C28 & 0x10) && !(D_80370C2A & 0x10)) {
            if (D_8036BB18 < 0x6B && D_8036BB1C != 8 && D_8036BB1C != 1) {
                func_8026AF6C((D_8036BB18 + 1) | 0x8000);
                func_80260650(D_80367738, 0x1D, NULL);
            } else {
                func_80260650(D_80367738, 0xD0, NULL);
            }
        }
        if ((D_80370C28 & 0x2000) && !(D_80370C2A & 0x2000)) {
            if (D_8036BB18 >= 0x5F && D_8036BB1C != 8 && D_8036BB1C != 1) {
                func_8026AF6C((D_8036BB18 - 1) | 0x8000);
                func_80260650(D_80367738, 0x1D, NULL);
            } else {
                func_80260650(D_80367738, 0xD0, NULL);
            }
        }
    }
    if ((((D_80370C28 & 0x1000) && !(D_80370C2A & 0x1000)) || ((D_80370C28 & 0x8000) && !(D_80370C2A & 0x8000))) &&
        func_802753C0() == 0 && D_8036BB1A == -1 && D_80364A98 == 0 && D_803643D9 == 0 && D_803643DA == 0 &&
        D_8036BB18 != 0) {
        if ((func_8026B10C() & 0x8000) && (func_8026B10C() & 1)) {
            func_8026B10C();
        }
        switch (D_80364A90) {
            case 1:
            case 0x800:
            case 0x1000:
                if (D_802E8BDC != 0x32 || D_80364A88 == 0x4000) {
                    D_80364A98 = 0x2000;
                    func_80260650(D_80367738, 0x1E, NULL);
                }
                break;
            case 0x40:
            case 0x400:
                if (LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) &&
                    D_80364AA8 != 1) {
                    D_80364A98 = 0x08000000;
                    func_80260650(D_80367738, 0x1E, NULL);
                } else if (D_8036BB1C == 1) {
                    func_8026AF6C(D_802F4870[func_8026F92C((u32)D_80364AA8)] | 0x8000);
                }
                break;
            case 2:
#ifdef TARGET_PC
                /* the title takes Start at once, not 130 frames in (17E10.c) */
                if (D_80366A18 != 0 || port_intro_skip()) {
#else
                if (D_80366A18 != 0) {
#endif
                    func_80260650(D_80367738, 0x1E, NULL);
                    func_80260B40(0, 0);
                    func_80260B40(5, 0);
                    func_80275390(0x20000000000000);
                }
                break;
            case 0x100000000000:
                if (D_803A6B04 != 0) {
                    D_80364A98 = 0x400000000000;
                }
                break;
            case 4:
            case 0x100:
                if ((D_80370C28 & 0x1000) && D_802E8BD0 == 0) {
                    if (LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) &&
                        D_80364AA8 == 1) {
                        D_802F5804[YOSHI_ENTRY(2)].flags |= 1;
                        D_802F5804[YOSHI_ENTRY(2)].unk18 = 7;
                    } else {
                        D_802F5804[YOSHI_ENTRY(2)].flags &= ~1;
                        D_802F5804[YOSHI_ENTRY(2)].unk18 = 8;
                    }
                    func_8026AF6C(0x8000);
                }
                break;
        }
    }
    sp5C = func_8026BBD0(sp5C, &D_803156F8[D_8035805C], &D_80358078);
    if (D_8036BB16 != 0 && D_803643D9 == 0 && D_803643DA == 0) {
        func_8024BDA4(&D_8036BB16);
    }
    if ((D_80364A90 & 0x444) && D_8036BB1A == -1 && D_80367BC8 == 0 &&
        !((D_8036BB18 != -1) ? (D_802F8BDC[D_8036BB18].unk8 & 0x01000000) : 0)) {
        if (D_80364A90 & 0x440) {
            sp4A = 180;
        } else {
            sp4A = 255;
        }
        D_80367BD6 = (D_80367BD6 < sp4A) ? ((sp4A < D_80367BD6 + 16) ? sp4A : D_80367BD6 + 16) : sp4A;
    } else {
        D_80367BD6 = (D_80367BD6 >= 0) ? ((D_80367BD6 - 16 < 0) ? 0 : D_80367BD6 - 16) : 0;
    }
    if (D_80364A90 == 2) {
        sp5C = func_80274BF0(&D_803156F8[D_8035805C], sp5C);
    }
    func_802559F8(sp5C, &D_80358078);
    for (sp68 = 0; sp68 < D_80358080; sp68++) {
        osRecvMesg(&D_80315180, NULL, OS_MESG_BLOCK);
    }
    if (D_80315180.validCount != 0) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "MQ_IS_EMPTY(&textureDmaMessageQ)", "hd.c", LINE_EU(1509, 1517));
    }
    if (D_80358080 - D_80358084 >= 0x90) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "nextdma-no_palette_dmas<NUM_TEXTURE_DMAS", "hd.c", LINE_EU(1510, 1518));
    }
    for (sp68 = 0; sp68 < D_80358080 - D_80358084; sp68++) {
        func_802A57AC();
    }
    D_803649E0 = D_803649D8 % 40 - 20;
    D_803649E2 = D_803649D8 / 100 % 40 - 20;
    D_803649E4 = D_803649D8 / 10000 % 40 - 20;
    D_803643D8 = D_803643D6;
    if (D_803643D7 == 0 && D_803643D6 == 0) {
        if (D_803643DA != 0) {
            if (D_803643D9 != 0) {
                func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!cmo_hit_request", "hd.c", LINE_EU(1529, 1537));
            }
            D_803643D7 = 1;
#ifdef VERSION_EU
        } else if (D_803643D9 != 0 && (D_802E8BDC != 49 || D_803EF6E4 >= 0x31BD1)) {
#else
        } else if (D_803643D9 != 0) {
#endif
            D_803643D6 = 1;
        }
        D_803643DA = 0;
        D_803643D9 = 0;
    }
}

void func_8024A348(void) {
    if (D_80364A40 != 0) {
        if (D_80364A3D + 1 != D_80364A3C && (D_80364A3D != 4 || D_80364A3C != 0)) {
            D_80364A00[D_80364A3D].unk0 = D_80364A40;
            D_80364A00[D_80364A3D].unk9 = 30;
            if (D_8036443E >= 0xE00 || D_8036443E < 0x200) {
                if (D_80364AA8 == 8) {
                    D_80364A00[D_80364A3D].unk4 = 30;
                    D_80364A00[D_80364A3D].unk6 = 60;
                    D_80364A00[D_80364A3D].unk8 = 1;
                } else {
                    D_80364A00[D_80364A3D].unk4 = 30;
                    D_80364A00[D_80364A3D].unk6 = 30;
                    D_80364A00[D_80364A3D].unk8 = 1;
                }
            }
            if (D_8036443E >= 0x200 && D_8036443E < 0x600) {
                D_80364A00[D_80364A3D].unk4 = 30;
                D_80364A00[D_80364A3D].unk6 = 160;
                D_80364A00[D_80364A3D].unk8 = 1;
            }
            if (D_8036443E >= 0x600 && D_8036443E < 0xA00) {
                D_80364A00[D_80364A3D].unk4 = 250;
                D_80364A00[D_80364A3D].unk6 = 160;
                D_80364A00[D_80364A3D].unk8 = 0;
            }
            if (D_8036443E >= 0xA00 && D_8036443E < 0xE00) {
                D_80364A00[D_80364A3D].unk4 = 250;
                D_80364A00[D_80364A3D].unk6 = 37;
                D_80364A00[D_80364A3D].unk8 = 0;
            }
            D_80364A3D++;
            if (D_80364A3D == 5) {
                D_80364A3D = 0;
            }
        }
        D_80364A40 = 0;
    }
    if (D_80364A3C != D_80364A3D) {
        D_80364A44 = D_80364A00[D_80364A3C].unk0;
        D_80364A4A = D_80364A00[D_80364A3C].unk4;
        D_80364A4C = D_80364A00[D_80364A3C].unk6;
        D_80364A4E = D_80364A00[D_80364A3C].unk8;
        if (D_80364A00[D_80364A3C].unk9 >= 16) {
            D_80364A48 = 255.0f - ((D_80364A00[D_80364A3C].unk9 - 15) / 15.0f) * 255.0f;
        } else {
            D_80364A48 = (D_80364A00[D_80364A3C].unk9 / 15.0f) * 255.0f;
        }
        if (!(D_80364A00[D_80364A3C].unk9--)) {
            D_80364A3C++;
            if (D_80364A3C == 5) {
                D_80364A3C = 0;
            }
        }
    } else {
        D_80364A44 = 0;
    }
}


void func_8024A92C(u32 arg0) {
    u32 sp2C;
    u32 sp28;

    switch (D_802E8BDC) {
        case 29:
            sp2C = 22000;
            sp28 = 14000;
            break;
        case 12:
            sp2C = 26000;
            sp28 = 20000;
            break;
        case 58:
            sp2C = 36000;
            sp28 = 30000;
            break;
        case 18:
            sp2C = 36000;
            sp28 = 28000;
            break;
        case 16:
            sp2C = 33000;
            sp28 = 23000;
            break;
        case 15:
            sp2C = 24000;
            sp28 = 18000;
            break;
        case 13:
            sp2C = 55000;
            sp28 = 35000;
            break;
        case 14:
            if (D_803EF6E4 > 177600) {
                sp2C = 25000;
                sp28 = 20000;
            } else if (D_803EF6E4 > 80000) {
                sp2C = 22000;
                sp28 = 17500;
            } else {
                sp2C = 14000;
                sp28 = 8000;
            }
            break;
        case 9:
            if (D_803EF6E4 > 100320) {
                sp2C = 36000;
                sp28 = 18000;
            } else {
                sp2C = 12000;
                sp28 = 6000;
            }
            break;
        default:
            sp2C = 12000;
            sp28 = 6000;
            break;
    }
    D_803153F0 = D_80364A6F;
    if (arg0 < sp2C) {
        if (arg0 < sp28) {
            D_80364A6F = 2;
        } else {
            D_80364A6F = 1;
        }
    } else {
        D_80364A6F = 0;
    }
    if (D_80364A6F != D_803153F0) {
        if (D_803153F0 == 0 && func_8026A610(D_803EF6DC, D_803EF6E4, D_803643E0, D_803643E8) > 20000) {
            func_80277EDC(1, 1, 7, 0x6C);
            func_8029A7E4("TOO FAR AWAY FROM CMO\n");
        }
        switch (D_80364A6F) {
            case 1:
                switch (D_803153F0) {
                    case 0:
                        func_80260DFC();
                    case 2:
                        func_802608C8(D_803156E8);
                        break;
                }
                break;
            case 2:
                switch (D_803153F0) {
                    case 0:
                        func_80260DFC();
                    case 1:
                        func_80260650(D_80367738, 0x26, &D_803156E8);
                        break;
                }
                break;
            case 0:
                func_802608C8(D_803156E8);
                if (D_802E8F94[D_802E8BDC].unk0 != 0x80 || func_802C1AA0() == 0) {
                    func_8029A7E4("popTuneImmediate();\n");
                    func_80261040();
                } else {
                    func_8029A7E4("WILL THIS FIX IT!!?\n");
                }
                if (D_803153F0 == 2) {
                    func_80260650(D_80367738, 0x68, NULL);
                }
                break;
        }
    }
    switch (D_80364A6F) {
        case 1:
            if (func_8026AD30(0x49) == 0 && D_8036BB18 != 3) {
                func_8026AF6C(0x8003);
            }
            break;
        case 2:
            if (D_8036BB18 != 2) {
                func_8026AF6C(0x8002);
            }
            break;
        case 0:
            if ((D_8036BB1C == 2 || D_8036BB1C == 4) && func_8026B10C() == 0 &&
                (D_8036BB18 == 3 || D_8036BB18 == 2 || D_8036BB18 == 0x49)) {
                func_8026AF6C(0x4000);
            }
            break;
    }
}

void func_8024ADD8(void) {
    u32 sp4;

    D_803649F4 = (sp4 = D_803649F0 - D_803649F4) / 10 + D_803649F4;
    D_803649F4 = D_803649F4 + 10;
    if (D_803649F0 < D_803649F4) {
        D_803649F4 = D_803649F0;
    }
}

void func_8024AE2C(void) {
    s32 pad;

    if (D_803649ED != 0 && D_803649ED != 0xFF) {
        if (func_802AB878(D_803649ED) == 0) {
            func_802AB478(D_803649ED);
            func_8028F6B4(D_803649ED);
            func_80291ED8(D_803649ED);
            func_8028B720();
            func_802794E4();
            if (func_8024AFA8(D_803649ED) != 0) {
                D_80364456 = D_803649ED;
                func_802AE860();
                if (D_802E8BD0 == 0 && D_80358060 >= 11 &&
                    (D_80364456 == 8 || D_80364456 == 15 || D_80364456 == 13 || D_80364456 == 14)) {
                    func_8026AD30(0x4A);
                }
            }
            D_80364AF0[D_80364AE8].unk10 |= 1 << D_80364456;
            D_803649E8 = 1;
            D_803649EC = 1;
            D_803649EE = 1;
        } else if (func_80260634(D_803156F0) == 0) {
            func_80260650(D_80367738, 0x2B, &D_803156F0);
        }
    }
}


u8 func_8024AFA8(s32 arg0) {
    u8 sp27;

    sp27 = 1;
    switch (arg0) {
        case 1:
            func_802AFFD4();
            break;
        case 2:
            func_802B1228();
            break;
        case 3:
            func_802B2D7C();
            break;
        case 4:
            func_802B448C();
            break;
        case 5:
            func_802B5CD8();
            break;
        case 6:
            func_802BB054();
            break;
        case 7:
            func_802BBDC8();
            break;
        case 8:
            func_802B76AC();
            break;
        case 9:
            func_802C5714();
            break;
        case 10:
            func_802C9F54();
            break;
        case 11:
        case 17:
        case 18:
            func_802C8AB0();
            break;
        case 13:
            func_802CBA94();
            break;
        case 14:
            func_802CCC8C();
            break;
        case 15:
            func_802CFA0C();
            break;
        case 16:
            func_802D0C68();
            break;
        default:
            sp27 = 0;
            break;
    }
    if (sp27 != 0 && (D_80364A90 & 0x104)) {
        func_8025BBE8((D_80364AF0[D_80364AE8].unkF0 & (1 << arg0)) ? 0x80 : 0x40, 0, 0);
    }
    if (sp27 != 0) {
        func_8029A7E4("changing to digger %d\n", arg0);
    }
    return sp27;
}

u8 func_8024B4B8(void);
s32 func_8024B418(u8);

void func_8024B188(void) {
    u8 sp27;
    u8 sp26;

    sp27 = 0;
    sp26 = 0;
    sp27 = func_8024B4B8();
    if (sp27 == 1) {
        sp26 = func_802AE888(D_802E8BFC[D_80364456 - 1]);
    }
    if (sp26 != 0) {
        D_803649E8 = 0;
        func_802794E4();
        func_8028F93C();
        func_80292084();
        func_8028B720();
        switch (D_80364456) {
            case 4:
                func_802B4658();
                break;
            case 3:
                func_802B2F54();
                break;
            case 5:
                func_802B5F60();
                break;
            case 2:
                func_802B11B8();
                break;
            case 1:
                func_802B0254();
                break;
            case 7:
                func_802BBE2C();
                break;
            case 8:
                func_802B7754();
                break;
            case 9:
                func_802C5688();
                break;
            case 10:
                func_802CA1AC();
                break;
            case 11:
            case 17:
            case 18:
                func_802C8B0C(D_80364456);
                break;
            case 13:
                func_802CBBBC();
                break;
            case 14:
                func_802CCD34();
                break;
            case 6:
                func_802BB1A0();
                if (func_8024B418(4) != 0) {
                    func_802B4658();
                }
                if (func_8024B418(13) != 0) {
                    func_802CBBBC();
                }
                break;
            case 15:
                func_802CFAB4();
                break;
            case 16:
                func_802D0BF8();
                break;
        }
        D_80364456 = 0;
        if (D_80364A90 & 0x104) {
            func_8025BBE8((D_80364AF0[D_80364AE8].unkF0 & 1) ? 0x80 : 0x40, 0, 0);
        }
    }
    if ((sp27 == 0 || (sp27 != 2 && sp26 == 0)) && func_80260634(D_803156EC) == 0) {
        func_80260650(D_80367738, 0x2B, &D_803156EC);
    }
}

s32 func_8024B418(u8 arg0) {
    s32 sp4;

    sp4 = 0;
    while (&D_80364460[sp4] != D_803649D0) {
        if (D_80364460[sp4].type == arg0) {
            return 1;
        }
        sp4++;
    }
    return 0;
}


u8 func_8024B4B8(void) {
    switch (D_80364456) {
        case 4:
            return func_802B45FC();
        case 3:
            return func_802B2EF8();
        case 5:
            return func_802B5F04();
        case 2:
            return func_802B1150();
        case 1:
            return func_802B01DC();
        case 6:
            return func_802BB170();
        case 7:
            return func_802BBE10();
        case 8:
            return func_802B76F8();
        case 9:
            return func_802C5508();
        case 10:
            return func_802CA140();
        case 11:
        case 17:
        case 18:
            return func_802C8AF0();
        case 13:
            return func_802CBB60();
        case 14:
            return func_802CCCD8();
        case 15:
            return func_802CFA58();
        case 16:
            return func_802D0B90();
    }
#ifdef TARGET_PC
    return 0;   /* (no case for 0 and 12: what was left in $v0) */
#endif
}

void func_8024B5E8(void) {
    D_803ED3F5 = 1;
    func_802AB670(D_80364456);
}


void func_8024B618(void) {
    s32 sp1C;
    s32 sp18;

    sp1C = 0;
    while (&D_80364460[sp1C] != D_803649D0) {
        if (D_80364460[sp1C].unk70 != 0) {
            sp18 = D_80364460[sp1C].type;
            if (sp18 != D_80364456 && sp18 != 0xFF && sp18 != 0xFE && sp18 != 0) {
                switch (sp18) {
                    case 5:
                        func_802B5FAC();
                        break;
                    case 1:
                        func_802B02A0();
                        break;
                    case 4:
                        func_802B46C4();
                        break;
                    case 8:
                        func_802B77A0();
                        break;
                    case 13:
                        func_802CBC08();
                        break;
                    case 14:
                        func_802CCD80();
                        break;
                    case 15:
                        func_802CFB00();
                        break;
                    case 9:
                        func_802C5860();
                        break;
                    case 3:
                        func_802B2FA0();
                        break;
                    default:
                        func_8029A7E4("MOVEABLE GEOMETRY MOVE ROUTINE NOT WRITTEN YET\n");
                        break;
                }
            }
        }
        sp1C++;
    }
}


void func_8024B7AC(void) {
    D_803ED3F5 = 0;
    switch (D_80364456) {
        case 0:
            D_803649ED = 0;
            func_802AEEC8();
            break;
        case 1:
            func_802B03F4();
            break;
        case 2:
            func_802B152C();
            break;
        case 3:
            func_802B327C();
            break;
        case 4:
            func_802B49AC();
            break;
        case 5:
            func_802B6294();
            break;
        case 6:
            func_802BB274();
            break;
        case 7:
            func_802BBEB8();
            break;
        case 8:
            func_802B7A88();
            break;
        case 9:
            func_802C5AFC();
            break;
        case 10:
            func_802CA4E0();
            break;
        case 11:
        case 17:
        case 18:
            func_802C8BB8(D_80364456);
            break;
        case 13:
            func_802CBEF0();
            break;
        case 14:
            func_802CD068();
            break;
        case 15:
            func_802CFDE8();
            break;
        case 16:
            func_802D0F98();
            break;
    }
}

void func_8024B8F4(Mtx *arg0, Mtx *arg1) {
    Gfx spF8[50];
    Gfx *gfx = spF8;
    Vtx sp70[8];
    s32 sp6C;

    for (sp6C = 0; sp6C < 8; sp6C++) {
        sp70[sp6C].v.flag = 0;
        sp70[sp6C].v.tc[0] = 0;
        sp70[sp6C].v.tc[1] = 0;
        sp70[sp6C].v.cn[0] = 0;
        sp70[sp6C].v.cn[1] = 0;
        sp70[sp6C].v.cn[2] = 0;
        sp70[sp6C].v.cn[3] = 0;
    }
    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000010);
    gSPMatrix(gfx++, K0_TO_PHYS(arg0), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(gfx++, D_8035807C);
    gSPMatrix(gfx++, K0_TO_PHYS(arg1), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPVertex(gfx++, K0_TO_PHYS(sp70), 8, 0);
    gSP1Triangle(gfx++, 0, 1, 4, 0);
    gSP1Triangle(gfx++, 1, 4, 5, 0);
    gSP1Triangle(gfx++, 0, 3, 4, 0);
    gSP1Triangle(gfx++, 3, 4, 7, 0);
    gSP1Triangle(gfx++, 2, 3, 7, 0);
    gSP1Triangle(gfx++, 2, 6, 7, 0);
    gSP1Triangle(gfx++, 1, 2, 5, 0);
    gSP1Triangle(gfx++, 2, 5, 6, 0);
    gSP1Triangle(gfx++, 4, 5, 6, 0);
    gSP1Triangle(gfx++, 4, 6, 7, 0);
    gSP1Triangle(gfx++, 0, 1, 2, 0);
    gSP1Triangle(gfx++, 0, 2, 3, 0);
    gDPTileSync(gfx++);
    gSPEndDisplayList(gfx++);
    osWritebackDCache(spF8, (s32)(gfx - spF8) * sizeof(Gfx));
    osWritebackDCache(arg0, sizeof(Mtx));
    osWritebackDCache(arg1, sizeof(Mtx));
    func_802A467C(D_80358074, spF8, sp70, (s32)(gfx - spF8) * sizeof(Gfx));
}


void func_8024BDA4(u16 *arg0) {
    switch (D_80364A90) {
        case 0x4:
        case 0x100:
            switch (*arg0) {
                case 0x1:
                    if (D_80364456 == 6 || D_80364456 == 0xB || D_80364456 == 0x11 || D_80364456 == 0x12) {
                        D_802F5804[YOSHI_ENTRY(7)].flags &= ~1;
                        D_802F5804[YOSHI_ENTRY(7)].unk18 = 8;
                    } else {
                        D_802F5804[YOSHI_ENTRY(7)].flags |= 1;
                        D_802F5804[YOSHI_ENTRY(7)].unk18 = 7;
                    }
                    func_8026AF6C(0x8001);
                    D_80364A98 = 4;
                    break;
                case 0x2:
                    func_80285EF4(D_80364A58);
                    break;
                case 0x7:
                    func_8028B240();
                    break;
                case 0x3:
                    if (D_802E8F94[D_802E8BDC].unk0 & 0x81) {
                        func_80275270(0x2000, 0.5f);
                    } else {
                        func_80275270(0x20000000, 0.5f);
                    }
                    break;
                case 0x8:
                    D_80364A98 = 0x2000000000000000;
                    func_8026AF6C(0x805E);
                    D_80364412 = 1;
                    break;
                case 0x4:
                    func_80275270(0x4000, 0.5f);
                    break;
                case 0x9:
                    if (func_80260DF0() == 1.0) {
                        D_802F8BDC[13].unk18 = 0x28;
                    } else {
                        D_802F8BDC[13].unk18 = 0x27;
                    }
                    func_8026AF6C(0x800D);
                    break;
                case YOSHI_ENTRY(0x1A7):
                case YOSHI_ENTRY(0x1A8):
                    if (((*arg0 - YOSHI_ENTRY(0x1A7)) << D_80364456) ^ ((1 << D_80364456) & 0x10205)) {
                        func_8029A7E4("selected controller mode yes\n");
                        D_80364AF0[D_80364AE8].unkF0 |= 1 << D_80364456;
                        func_8025BBE8(0x80, 0, 0);
                    } else {
                        func_8029A7E4("selected controller mode no\n");
                        D_80364AF0[D_80364AE8].unkF0 &= ~(1 << D_80364456);
                        func_8025BBE8(0x40, 0, 0);
                    }
                    break;
                case 0x11:
                    D_802F8BDC[6].unk8 |= 0x80;
                    break;
                case 0xFFFF:
                    switch (D_8036BB18) {
                        case 0x1:
                        case 0x6:
                            func_8029A7E4("TESTING PAUSE2 %d %d %d\n", D_802E8BD0, D_802E8BD8, D_802E8BD4);
                            func_8026AF6C(0x8000);
                            if (D_803643DB != 0 || D_80364AC1 != 0) {
                                D_80364A98 = 0x100;
                                func_802A45D4(0xA);
                            }
                            break;
                        case 0xD:
                        case 0x58:
                            func_8029A7E4("TESTING PAUSE3 %d %d %d\n", D_802E8BD0, D_802E8BD8, D_802E8BD4);
                            func_8026AF6C(0x8001);
                            break;
                    }
                    break;
                case 0x28:
                    func_80260D7C(1.0f);
                    break;
                case 0x27:
                    func_80260D7C(0.7f);
                    break;
            }
            break;
        case 0x1000000000:
            D_80364A98 = 4;
            break;
        case 0x40:
        case 0x400:
            if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) {
                if (*arg0 == 0x18 || *arg0 == 0x22 || *arg0 == 0xFFFF) {
                    D_80364A98 = 0x08000000;
                } else {
                    D_80364A98 = 0x2000;
                }
            } else {
                if (*arg0 == 0x18 || *arg0 == 0x22 || *arg0 == 0xFFFF) {
                    D_80364A98 = 0x4000;
                } else {
                    D_80364A98 = 0x2000;
                }
            }
            break;
        case 0x2000000000000000:
            if (*arg0 == 0xFFFF) {
                func_8029A7E4("TESTING PAUSE %d %d %d\n", D_802E8BD0, D_802E8BD8, D_802E8BD4);
                func_8026AF6C(0x8001);
            }
            D_80364A98 = 4;
            break;
        default:
            func_8029A7E4("Yoshi selection in illegal game mode\n");
            break;
    }
    *arg0 = 0;
}

#ifdef VERSION_EU
extern s32 D_802FA250;
extern s32 D_8036BF14;
extern s32 D_8036BF18;
extern u32 D_8036BF20;
extern u32 D_8036BF24;
extern u32 D_8036BF2C;
extern OSTime D_8036BF40;
extern OSTime D_8036BF50;
extern OSTime D_80368058;
extern OSTime D_80368068;

/* A frame's count ticks (eu's 20000us), as FRAME_TICKS_100 works it out. */
#define FRAME_TICKS ((u64)20000 * osClockRate / 1000000)

/*
 * eu's frame rate and timing bars (when D_802FA250 is set): the frame
 * rate as text, then bars from x 110, half a pixel per percent of a frame:
 * the RSP's audio and graphics tasks, the RDP, the game's own time and the
 * audio thread, each from the RDP's start (D_80315440.unk288).
 */
Gfx *func_8024C404(Gfx *arg0, Frame *arg1, s32 *arg2) {
    Gfx *gfx;
    u16 color;
    char buf[30];
    s64 t1;
    s64 t2;
    OSIntMask mask;

    gfx = arg0;
    mask = osSetIntMask(OS_IM_NONE);
    D_802EAA54_eu += 50U / (D_8036BF14 - D_8036BF18);
    D_802EAA58_eu++;
    if (D_802FA250 != 0) {
        sprintf(buf, "%d", 50U / (D_8036BF14 - D_8036BF18));
        func_80259CCC(arg1, buf, NULL, 0, 0, 24, 200, 20, 20, 1, 0xFF, 0xFF, 0xFF, 0xFF);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_FILL);
        gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gDPPipeSync(gfx++);
        color = 0xFFC1;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, 60, 200, 62, 225);
        gDPFillRectangle(gfx++, 160, 200, 162, 225);
        gDPFillRectangle(gfx++, 210, 200, 212, 225);
        gDPFillRectangle(gfx++, 260, 200, 262, 225);
        gDPPipeSync(gfx++);
        color = 0xFFFF;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, 110, 200, 112, 225);
        t1 = D_8036BF40 - D_80315440.unk288;
        if (t1 < 0) {
            t1 += FRAME_TICKS * 2;
        }
        t2 = D_8036BF50 - D_80315440.unk288;
        if (t2 < 0) {
            t2 += FRAME_TICKS * 2;
        }
        gDPPipeSync(gfx++);
        color = 0xF83F;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, t1 / FRAME_TICKS_100 / 2 + 110, 201, t2 / FRAME_TICKS_100 / 2 + 110, 204);
        gDPPipeSync(gfx++);
        color = 0x6001;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, 110, 206, D_8036BF24 / 2 + 110, 209);
        gDPPipeSync(gfx++);
        color = 0xF801;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, 110 - (100 - D_8036BF2C) / 2, 206, D_8036BF24 / 2 - (100 - D_8036BF2C) / 2 + 110,
                         209);
        gDPPipeSync(gfx++);
        color = 0x7C1;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, 110, 211, D_8036BF20 / 2 + 110, 214);
        gDPPipeSync(gfx++);
        color = 0x19;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, 110, 216, D_80364AC8 / 2 + 110, 219);
        gDPPipeSync(gfx++);
        color = 0x3F;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, 110 - (100 - D_80364ACC) / 2, 216, D_80364AC8 / 2 - (100 - D_80364ACC) / 2 + 110,
                         219);
        t1 = D_80368058 - D_80315440.unk288;
        if (t1 < 0) {
            t1 += FRAME_TICKS * 2;
        }
        t2 = D_80368068 - D_80315440.unk288;
        if (t2 < 0) {
            t2 += FRAME_TICKS * 2;
        }
        gDPPipeSync(gfx++);
        color = 0x7FF;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, t1 / FRAME_TICKS_100 / 2 + 110, 221, t2 / FRAME_TICKS_100 / 2 + 110, 224);
        gDPPipeSync(gfx++);
        color = 0x7C1;
        gDPSetFillColor(gfx++, (color << 16) | color);
        gDPFillRectangle(gfx++, D_8036BF20 / 2 + 110, 200, D_8036BF20 / 2 + 110, 225);
        func_80259C24(&gfx, arg1);
    }
    osSetIntMask(mask);
    *arg2 += gfx - arg0;
    return gfx;
}
#else
Gfx *func_8024C404(Gfx *arg0, Frame *arg1, s32 *arg2) {
    *arg2 = 0;
    return arg0;
}
#endif

Gfx *func_8024C414(Frame *arg0, s32 *arg1) {
    Gfx *gfx = arg0->game.unk48B0;
    char sp194[16];
    char sp184[16];
    s32 sp180;

    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(arg0));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000038);
    gSPDisplayList(gfx++, D_01000010);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gSPClearGeometryMode(gfx++, G_ZBUFFER);
    gDPSetDepthImage(gfx++, D_80358058);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358058);
    gDPSetFillColor(gfx++, 0xFFFCFFFC);
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    guTranslate(&arg0->game.unk0[7], 0.0f, 0.0f, 0.0f);
    guOrtho(&arg0->game.unk0[3], 0.0f, 319.0f, 239.0f, 0.0f, -2e+04f, 2e+04f, 1.0f);
    guOrtho(&arg0->game.unk0[4], 0.0f, 1279.0f, 959.0f, 0.0f, -2e+04f, 2e+04f, 1.0f);
    func_802507C8(&arg0->game.unk0[5], &arg0->game.unk3C00, &arg0->game.unk0[6]);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gfx = func_80271FD0(gfx, arg0, D_802E8BDC, D_80364452, D_80364454, &sp180);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    switch (D_802E8BDC) {
        case 13:
        case 14:
        case 16:
        case 52:
            gDPSetFillColor(gfx++, 0xD55FD55F);
            break;
        case 15:
            gDPSetFillColor(gfx++, 0x10511051);
            break;
        default:
            gDPSetFillColor(gfx++, 0x00010001);
            break;
    }
    gDPPipeSync(gfx++);
    gDPFillRectangle(gfx++, 0, (sp180 <= 0) ? 0 : sp180 - 1, 319, 239);
    gDPPipeSync(gfx++);
    gSPLookAt(gfx++, &D_02000000.unk3C00);
    gSPMatrix(gfx++, &D_02000000.unk0[2], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_02000000.unk0[5], G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_02000000.unk0[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(gfx++, D_8035807C);
    switch (D_80364A90) {
        case 0x40:
        case 0x400:
            guPerspective(&arg0->game.unk0[2], &D_8035807C, D_80364438, 4.0f / 3.0f, 10.0f, 2e+04f, 1.0f);
            break;
        case 1:
        case 0x800:
        case 0x1000:
            guPerspective(&arg0->game.unk0[2], &D_8035807C, D_80364438, 4.0f / 3.0f, 10.0f, 1e+04f, 1.0f);
            break;
        default:
            guPerspective(&arg0->game.unk0[2], &D_8035807C, D_80364438, 4.0f / 3.0f, 10.0f, 1e+04f, 1.0f);
            break;
    }
    gDPSetColorDither(gfx++, G_CD_MAGICSQ);
    gSPClipRatio(gfx++, FRUSTRATIO_3);
    func_8027F1F8(&gfx, D_8035805C, 0);
    gSPSegment(gfx++, 8, K0_TO_PHYS(D_80364458));
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPDisplayList(gfx++, osVirtualToPhysical(D_80358030[D_8035805C]));
    gSPDisplayList(gfx++, D_803BE6E0);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER);
    gSPDisplayList(gfx++, osVirtualToPhysical(D_80358038[D_8035805C]));
    gSPDisplayList(gfx++, D_803BE6E4);
    if (D_803643D6 == 0 && D_803643D7 == 0) {
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LOD);
        gSPDisplayList(gfx++, D_02000000.unk21410);
        gDPPipeSync(gfx++);
    }
    switch (D_8035805C) {
        case 0:
            gSPSegment(gfx++, 10, osVirtualToPhysical(D_803F7820));
            break;
        case 1:
            gSPSegment(gfx++, 10, osVirtualToPhysical(D_803F7824));
            break;
    }
    gDPPipeSync(gfx++);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
    gSPDisplayList(gfx++, D_02000000.unkA4E0);
    if (!(D_80364A90 & 0x440)) {
        func_8027C4C8(&gfx, arg0);
    }
    func_802502EC();
    func_8024E4F4(&gfx, arg0, 0);
    func_80258B78(&gfx, arg0);
    if (D_802E8BD0 == 0 && D_80364AA8 != 0x40 && !(D_80364A90 & 0x440) && D_80364A84 == 0) {
        func_80279EE8(&gfx, arg0, D_8035805C);
    }
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LOD);
    gSPDisplayList(gfx++, D_02000000.unkA580);
    gDPPipeSync(gfx++);
    gfx = func_802CEEFC(gfx, D_8035805C, arg0->game.unkA3A0, arg0->game.unk11C0);
    func_8024E4F4(&gfx, arg0, 1);
    if (D_803643DC != 0) {
        func_8024F520(&gfx, arg0);
    }
    func_802701A8(&gfx, arg0);
    func_80281E44(&gfx);
    func_8028E9E4(&gfx, arg0);
    func_802917B0(&gfx, arg0);
    func_80292EB8(&gfx, arg0);
    func_8028CB30(&gfx, arg0);
    switch (D_8035805C) {
        case 0:
            gSPDisplayList(gfx++, osVirtualToPhysical(D_803C5770));
            break;
        case 1:
            gSPDisplayList(gfx++, osVirtualToPhysical(D_803C6370));
            break;
    }
    func_80288DF0(&gfx, D_8035805C);
    if (!(D_80364A90 & 2) || D_802E8BF0 == 0) {
        func_8024FC2C(&gfx, 0);
    }
    func_802976E8(&gfx);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER);
    gSPDisplayList(gfx++, osVirtualToPhysical(D_80358040[D_8035805C]));
    gSPDisplayList(gfx++, D_803BE6E8);
    if (!(D_80364A90 & 2) || D_802E8BF0 == 0) {
        func_8024FC2C(&gfx, 1);
    }
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER);
    gSPDisplayList(gfx++, osVirtualToPhysical(D_80358048[D_8035805C]));
    gSPDisplayList(gfx++, D_803BE6EC);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LOD);
    gSPDisplayList(gfx++, D_02000000.unkA918);
    gDPPipeSync(gfx++);
    if (D_803643D6 == 0 && D_803643D7 == 0) {
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LOD);
        gSPDisplayList(gfx++, D_02000000.unk21478);
        gDPPipeSync(gfx++);
    }
    if (!(D_80364A90 & 2) || D_802E8BF0 == 0) {
        func_8024FC2C(&gfx, 2);
    }
    func_80295120(&gfx, arg0);
    if (D_80364A90 & 0x2000001000003905LL) {
        func_80280F34(&gfx, D_8035805C);
    }
    func_8027F1F8(&gfx, D_8035805C, 1);
    func_8024E4F4(&gfx, arg0, 2);
    if (D_803643DC != 0) {
        func_80266248(&gfx, arg0);
    }
    switch (D_8035805C) {
        case 0:
            gSPDisplayList(gfx++, osVirtualToPhysical(D_803C6F70));
            break;
        case 1:
            gSPDisplayList(gfx++, osVirtualToPhysical(D_803C7B70));
            break;
    }
    if (D_803643DB != 0) {
        func_8028273C(&gfx, D_8035805C);
    }
    if (D_80364A68 != 0 && (D_80364A90 & 0x104)) {
        func_80286C60(&gfx, arg0, D_8035805C, D_80364456);
    }
    if (D_80364A90 & 0x200000000400220CLL) {
        func_80278324(&gfx, arg0, D_8035805C);
    }
    if (D_80364A90 == 0x100 && D_803643DB != 0) {
        func_80276E50(&gfx, arg0, D_8035805C, D_803643E0, D_803643E4, D_803643E8);
    }
    func_80259450();
    if (D_80364A6A != 0 && (D_80364A90 & 0x104)) {
        func_80287530(&gfx, arg0, D_8035805C, D_80364456);
    }
    if (D_80364A6C != 0 && (D_80364A90 & 0x104)) {
        func_80287C68(&gfx, arg0, D_8035805C, D_80364456);
    }
    if (D_80364A90 & 0x104) {
        func_80282224(&gfx, D_80364456);
    }
    if ((D_80364AA8 & 1) && (D_80364A90 & 0x0400030C)) {
        func_8026A378(D_803649F4, &sp194[1]);
        sp194[0] = 0x24;
        func_80259CCC(arg0, sp194, NULL, 1, 0, 280, 18, 20, 20, 0, 0xFF, 0xFF, 0xFF, D_80367BD6);
        if (D_80364A44 != 0 && D_802E8BD0 == 0) {
            func_8026A378(D_80364A44, &sp184[1]);
            sp184[0] = 0x24;
            func_80259CCC(arg0, sp184, NULL, 1, 0, D_80364A4A, D_80364A4C, 35, 35, D_80364A4E, 0xFF, 0xFF, 0xFF,
                          D_80364A48);
        }
    }
    if ((D_80364A90 & 0x2000000000000104LL) && (!(D_80364AA8 & 0x81) || D_803643DB != 0 || D_80364AC1 != 0)) {
        func_80275478(arg0, &gfx, (D_80364A90 & 0x100) != 0 || D_8036BB18 == 0x4D || D_8036BB18 == 0x49);
    }
    if (D_80364A98 == 0 && func_802753C0() == 0) {
        if (!(D_80364A90 & 0x200000100400230CLL) && (D_80315440.frameCount % 50) * FRAMES_PER_SECOND / 60 >= 21 * FRAMES_PER_SECOND / 60 && D_8036BB1C == 1 &&
            (!(D_80364A90 & 2) || (D_802E8BEC != 0 && (D_80366A12 == 3 || D_802E8BEC == 1))) &&
            (D_80364A90 != 0x100000000000LL || D_803A6B04 != 0) &&
            (!(D_80364A90 & 0x1801) || D_80364AF0[D_80364AE8].gameState != 0) && D_802E8BDC != 0x2F) {
            #ifdef VERSION_EU
            func_80259CCC(arg0, TEXT_EU("PRESS START", "DRUECKE START"), NULL, 0, 160, 92, 196, 26, 26, 1, 0xFF, 0xFF, 0xFF, 0xFF);
#else
            func_80259CCC(arg0, "PRESS START", NULL, 1, 0, 92, 196, 26, 26, 1, 0xFF, 0xFF, 0xFF, 0xFF);
#endif
        }
        if ((D_80315440.frameCount % 40) * FRAMES_PER_SECOND / 60 >= 16 * FRAMES_PER_SECOND / 60) {
            if (D_802E8BD0 != 0) {
                if (D_80364A90 == 0x2000000000000000LL && D_8036BB1C == 2) {
                    func_80259CCC(arg0, TEXT_EU("USE Z/R TO TURN PAGES", "DRUECKE Z OD. R ZUM BLAETTERN"), U16TEXT(D_8030491C), 0, 0, 24, 20, 15, 15, 1, 0xFF, 0xFF, 0xFF, 0xFF);
                } else if (D_80364A90 == 0x100 && D_8036BB18 == 0 && D_8036BB1C == 2 && D_803643DB != 0 &&
                           !(D_80370C28 & 0x2010)) {
                    func_80259CCC(arg0, TEXT_EU("USE Z/R TO MOVE MAP", "DRUECKE Z OD. R ZUM SCROLLEN"), U16TEXT(D_80304938), 0, 0, 24, 20, 15, 15, 1, 0xFF, 0xFF, 0xFF, 0xFF);
                }
            } else if (D_80364A90 == 0x100) {
                if (D_80364AC1 != 0) {
                    func_80259CCC(arg0, TEXT_EU("SHUTTLE VIEW", "SHUTTLE-SICHT"), U16TEXT(D_80304904), 0, 0, 24, 20, 15, 15, 1, 0xFF, 0xFF, 0xFF, 0xFF);
                } else {
                    func_80259CCC(arg0, TEXT_EU("MISSILE VIEW", "MISSILE-SICHT"), U16TEXT(D_80304910), 0, 0, 24, 20, 15, 15, 1, 0xFF, 0xFF, 0xFF, 0xFF);
                }
            }
        }
    }
    if (((D_80364A90 & 0x440) || D_802E8BDC == 0x26) && D_8036BB1C == 1 && func_802753C0() == 0) {
        if ((D_80364A90 & 0x440) && D_80364AA8 != 1) {
            func_80274B40(&gfx, arg0, D_80365580, 264, 18);
        } else {
            func_80274B40(&gfx, arg0, D_80365580, 24, 18);
        }
    }
    if ((D_80364A90 & 0x104) && D_80364410 != 0) {
        func_80274B40(&gfx, arg0, D_80364A86, 264, 190);
    }
    if ((D_80364A90 & 0x104) && D_80364AA8 == 1 && D_802E8BD0 == 0) {
        func_80285CC0();
    }
    if (D_803643DB != 0 && D_80364A90 == 4) {
        func_80282C80(&gfx, arg0, D_803643E0, D_803643E4, D_803643E8, D_803EF6DC, D_803EF6E0, D_803EF6E4);
    }
    if ((D_802E8F94[D_802E8BDC].unk0 & 0x81) && D_80364A90 == 4) {
        if (D_802E8BDC != 0x32 ||
            LEVEL_DONE_IN(D_80364AF0[D_80364AE8], 0x32)) {
            func_8028376C(&gfx, arg0, D_8035805C, D_803643E0, D_803643E8, D_803EF6DC, D_803EF6E4);
        }
    }
    if (D_803643DB != 0 || D_80364AC1 != 0) {
        switch (D_80364A90) {
            case 4:
            case 0x100:
            case 0x200:
            case 0x100000000000LL:
                func_8025E2CC(&gfx, arg0, D_8035805C);
                func_8025E67C(&gfx, arg0, D_8035805C);
            case 0x40:
            case 0x400:
                if ((D_803643D6 != 0 || D_803643D7 != 0 || D_803643D9 != 0 || D_803643DA != 0) &&
                    (D_80364A90 & 0x144)) {
                    func_802A45D4(50);
                    if (D_80364A90 == 0x40) {
                        D_80364A98 = 0x400;
                    } else {
                        D_80364A98 = 0x200;
                    }
                }
                break;
        }
    }
    func_80259C24(&gfx, arg0);
    *arg1 = ((u8 *)gfx - (u8 *)arg0 - 0x48B0) >> 3;
    return gfx;
}

void func_8024E4F4(Gfx **arg0, Frame *arg1, u8 arg2) {
    Gfx *gfx = *arg0;
    s32 sp140;
    s16 pad;
    s16 sp13C;
    s16 sp13A;
    s16 sp138;
    s16 sp136;
    s16 sp134;
    u8 sp133;
    f32 spF0[4][4];
    f32 spB0[4][4];
    s16 spAE;
    s16 spAC;
    u8 spAB;

    if (arg2 == 0) {
        D_8036506C = 0;
    }
    gDPPipeSync(gfx++);
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LOD);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
    sp133 = D_803649E8 == 0 && (D_803649EC != 0 || (D_80364A90 & 0x1801) != 0);
    sp140 = 0;
    while (&D_803643C8[sp140] != D_803643CC) {
        if ((D_803643C8[sp140].unk1022 != 0 || sp133 != 0) && D_803643C8[sp140].unk1022 != 0xFE &&
            (D_803643C8[sp140].unk1022 != 0xFF || D_803EF6FF == 0) &&
            (D_803643C8[sp140].unk1022 == 0xFD || D_80364A84 == 0 || D_80364AC1 == 0) &&
            D_803643C8[sp140].unk1023 == arg2) {
            sp13C = D_803643C8[sp140].unk1004 >> 5;
            sp13A = D_803643C8[sp140].unk1008 >> 5;
            sp138 = D_803643C8[sp140].unk100C >> 5;
            sp136 = D_803643C8[sp140].unk1018;
            sp134 = D_803643C8[sp140].unk101A;
            spAB = D_803643C8[sp140].unk1022 == D_80364456 && D_803ED40D == 0x65;
            if (spAB != 0) {
                guRotateF(spB0, (f32)D_80364440 * 360.0 / 4096.0, 0.0f, 1.0f, 0.0f);
            }
            guRotateF(spF0, (f32)-D_803643C8[sp140].unk101E * 360.0 / 4096.0, 0.0f, 1.0f, 0.0f);
            if (spAB != 0) {
                guMtxCatF(spB0, spF0, spF0);
            }
            guRotateF(spB0, (f32)D_803643C8[sp140].unk101C * 360.0 / 4096.0, 1.0f, 0.0f, 0.0f);
            guMtxCatF(spF0, spB0, spB0);
            guRotateF(spF0, (f32)D_803643C8[sp140].unk1020 * 360.0 / 4096.0, 0.0f, 0.0f, 1.0f);
            guMtxCatF(spB0, spF0, spF0);
            guRotateF(spB0, (f32)D_803643C8[sp140].unk101E * 360.0 / 4096.0, 0.0f, 1.0f, 0.0f);
            guMtxCatF(spF0, spB0, spF0);
            guTranslateF(spB0, sp13C, sp13A, sp138);
            guMtxCatF(spF0, spB0, spF0);
            guMtxF2L(spF0, &arg1->game.unk2C0[sp140]);
            if (spAB != 0) {
                gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_802FA940), G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0,
                                    G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                spAE = 32, spAC = 32;
            } else {
                gDPLoadTextureBlock(gfx++, K0_TO_PHYS(D_803643C8[sp140].unk0), G_IM_FMT_IA, G_IM_SIZ_8b, 64, 64, 0,
                                    G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                spAE = 64, spAC = 64;
            }
            arg1->game.unk15C0[D_8036506C].v.ob[0] = -sp136;
            arg1->game.unk15C0[D_8036506C].v.ob[1] = 0;
            arg1->game.unk15C0[D_8036506C].v.ob[2] = sp134;
            arg1->game.unk15C0[D_8036506C].v.flag = 0;
            arg1->game.unk15C0[D_8036506C].v.tc[0] = (spAE - 1) << 5;
            arg1->game.unk15C0[D_8036506C].v.tc[1] = (spAC - 1) << 5;
            arg1->game.unk15C0[D_8036506C].v.cn[0] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[1] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[2] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[3] = 140;
            D_8036506C++;
            arg1->game.unk15C0[D_8036506C].v.ob[0] = sp136;
            arg1->game.unk15C0[D_8036506C].v.ob[1] = 0;
            arg1->game.unk15C0[D_8036506C].v.ob[2] = sp134;
            arg1->game.unk15C0[D_8036506C].v.flag = 0;
            arg1->game.unk15C0[D_8036506C].v.tc[0] = 0;
            arg1->game.unk15C0[D_8036506C].v.tc[1] = (spAC - 1) << 5;
            arg1->game.unk15C0[D_8036506C].v.cn[0] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[1] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[2] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[3] = 140;
            D_8036506C++;
            arg1->game.unk15C0[D_8036506C].v.ob[0] = sp136;
            arg1->game.unk15C0[D_8036506C].v.ob[1] = 0;
            arg1->game.unk15C0[D_8036506C].v.ob[2] = -sp134;
            arg1->game.unk15C0[D_8036506C].v.flag = 0;
            arg1->game.unk15C0[D_8036506C].v.tc[0] = 0;
            arg1->game.unk15C0[D_8036506C].v.tc[1] = 0;
            arg1->game.unk15C0[D_8036506C].v.cn[0] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[1] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[2] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[3] = 140;
            D_8036506C++;
            arg1->game.unk15C0[D_8036506C].v.ob[0] = -sp136;
            arg1->game.unk15C0[D_8036506C].v.ob[1] = 0;
            arg1->game.unk15C0[D_8036506C].v.ob[2] = -sp134;
            arg1->game.unk15C0[D_8036506C].v.flag = 0;
            arg1->game.unk15C0[D_8036506C].v.tc[0] = (spAE - 1) << 5;
            arg1->game.unk15C0[D_8036506C].v.tc[1] = 0;
            arg1->game.unk15C0[D_8036506C].v.cn[0] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[1] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[2] = 10;
            arg1->game.unk15C0[D_8036506C].v.cn[3] = 140;
            D_8036506C++;
            if (D_803643D6 != 0 && !(D_80364AA8 & 0x81) && D_803643C8[sp140].unk1022 == D_80364456) {
                gSPMatrix(gfx++, &D_02000000.unk1540, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            }
            gSPMatrix(gfx++, &D_02000000.unk2C0[sp140], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            gSPVertex(gfx++, &D_02000000.unk15C0[D_8036506C - 4], 4, 0);
            gSP1Triangle(gfx++, 0, 1, 2, 0);
            gSP1Triangle(gfx++, 0, 2, 3, 0);
            gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
            if (D_803643D6 != 0 && !(D_80364AA8 & 0x81) && D_803643C8[sp140].unk1022 == D_80364456) {
                gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
            }
            gDPPipeSync(gfx++);
        }
        sp140++;
    }
    *arg0 = gfx;
}

void func_8024F520(Gfx **arg0, Frame *arg1) {
    Gfx *gfx = *arg0;
    s32 sp88;
    u8 sp87;
    s16 sp84;
    s16 sp82;
    s16 sp80;
    s16 sp7E;
    s16 sp7C;

    gDPPipeSync(gfx++);
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LOD);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
    sp87 = 0;
    sp88 = 0;
    do {
        if (D_803643C8[sp88].unk1022 == 0xFE) {
            sp87 = 1;
        } else {
            sp88++;
        }
    } while (sp87 == 0);
    gDPLoadTextureBlock(gfx++, K0_TO_PHYS(D_803643C8[sp88].unk0), G_IM_FMT_IA, G_IM_SIZ_8b, 64, 64, 0,
                        G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    sp84 = D_803643C8[sp88].unk1004 >> 5;
    sp82 = D_803643C8[sp88].unk1008 >> 5;
    sp80 = D_803643C8[sp88].unk100C >> 5;
    sp7E = D_803643C8[sp88].unk1018;
    sp7C = D_803643C8[sp88].unk101A;
    arg1->game.unk18C0[0].v.ob[0] = -sp7E;
    arg1->game.unk18C0[0].v.ob[1] = 0;
    arg1->game.unk18C0[0].v.ob[2] = sp7C;
    arg1->game.unk18C0[0].v.flag = 0;
    arg1->game.unk18C0[0].v.tc[0] = 0x7E0;
    arg1->game.unk18C0[0].v.tc[1] = 0x7E0;
    arg1->game.unk18C0[0].v.cn[0] = 10;
    arg1->game.unk18C0[0].v.cn[1] = 10;
    arg1->game.unk18C0[0].v.cn[2] = 10;
    arg1->game.unk18C0[0].v.cn[3] = 140;
    arg1->game.unk18C0[1].v.ob[0] = sp7E;
    arg1->game.unk18C0[1].v.ob[1] = 0;
    arg1->game.unk18C0[1].v.ob[2] = sp7C;
    arg1->game.unk18C0[1].v.flag = 0;
    arg1->game.unk18C0[1].v.tc[0] = 0;
    arg1->game.unk18C0[1].v.tc[1] = 0x7E0;
    arg1->game.unk18C0[1].v.cn[0] = 10;
    arg1->game.unk18C0[1].v.cn[1] = 10;
    arg1->game.unk18C0[1].v.cn[2] = 10;
    arg1->game.unk18C0[1].v.cn[3] = 140;
    arg1->game.unk18C0[2].v.ob[0] = sp7E;
    arg1->game.unk18C0[2].v.ob[1] = 0;
    arg1->game.unk18C0[2].v.ob[2] = -sp7C;
    arg1->game.unk18C0[2].v.flag = 0;
    arg1->game.unk18C0[2].v.tc[0] = 0;
    arg1->game.unk18C0[2].v.tc[1] = 0;
    arg1->game.unk18C0[2].v.cn[0] = 10;
    arg1->game.unk18C0[2].v.cn[1] = 10;
    arg1->game.unk18C0[2].v.cn[2] = 10;
    arg1->game.unk18C0[2].v.cn[3] = 140;
    arg1->game.unk18C0[3].v.ob[0] = -sp7E;
    arg1->game.unk18C0[3].v.ob[1] = 0;
    arg1->game.unk18C0[3].v.ob[2] = -sp7C;
    arg1->game.unk18C0[3].v.flag = 0;
    arg1->game.unk18C0[3].v.tc[0] = 0x7E0;
    arg1->game.unk18C0[3].v.tc[1] = 0;
    arg1->game.unk18C0[3].v.cn[0] = 10;
    arg1->game.unk18C0[3].v.cn[1] = 10;
    arg1->game.unk18C0[3].v.cn[2] = 10;
    arg1->game.unk18C0[3].v.cn[3] = 140;
    guTranslate(&arg1->game.unk0[9], sp84, sp82, sp80);
    guRotate(&arg1->game.unk0[10], (f32)D_803EF326 * 360.0 / 4096.0, 0.0f, 1.0f, 0.0f);
    gSPMatrix(gfx++, &D_02000000.unk0[9], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPMatrix(gfx++, &D_02000000.unk0[10], G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gSPVertex(gfx++, D_02000000.unk18C0, 4, 0);
    gSP1Triangle(gfx++, 0, 1, 2, 0);
    gSP1Triangle(gfx++, 0, 2, 3, 0);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    *arg0 = gfx;
}

void func_8024FC2C(Gfx **arg0, u8 arg1) {
    Gfx *gfx = *arg0;
    s32 sp60;
    u8 pad;
    u8 sp5E;
    u8 sp5D;

    sp5D = D_803649E8 == 0 && (D_803649EC != 0 || (D_80364A90 & 0x1801) != 0);
    sp60 = 0;
    while (&D_80364460[sp60] != D_803649D0) {
        if ((D_80364460[sp60].type != 0 || sp5D != 0) && (D_80364460[sp60].type != 0xFF || D_803EF6FF == 0) &&
            (D_80364460[sp60].type == 0xFD || D_80364A84 == 0 || D_80364AC1 == 0)) {
            gSPSegment(gfx++, 6, osVirtualToPhysical(D_80364460[sp60].unk0));
            if ((D_80364A90 & 0x1801) && (D_80364460[sp60].type == 0xFE || D_80364460[sp60].type == 0)) {
                sp5E = D_8035805C;
            } else {
                sp5E = D_803156F5;
            }
            if (sp5E != 0) {
                gSPSegment(gfx++, 7, osVirtualToPhysical(D_80364460[sp60].unk4));
            } else {
                gSPSegment(gfx++, 7, osVirtualToPhysical(D_80364460[sp60].unk8));
            }
            gDPPipeSync(gfx++);
            gDPSetEnvColor(gfx++, 0, 0, 0, D_80364460[sp60].unk60);
            gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
            if (D_803643D6 != 0 && !(D_80364AA8 & 0x81) && D_80364460[sp60].type == D_80364456) {
                gSPMatrix(gfx++, &D_02000000.unk1500, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            }
            if (sp5E != 0) {
                switch (arg1) {
                    case 0:
                        gSPDisplayList(gfx++, osVirtualToPhysical(D_80364460[sp60].unkC));
                        break;
                    case 1:
                        gSPDisplayList(gfx++, osVirtualToPhysical(D_80364460[sp60].unk10));
                        break;
                    case 2:
                        gSPDisplayList(gfx++, osVirtualToPhysical(D_80364460[sp60].unk14));
                        break;
                }
            } else {
                switch (arg1) {
                    case 0:
                        gSPDisplayList(gfx++, osVirtualToPhysical(D_80364460[sp60].unk30));
                        break;
                    case 1:
                        gSPDisplayList(gfx++, osVirtualToPhysical(D_80364460[sp60].unk34));
                        break;
                    case 2:
                        gSPDisplayList(gfx++, osVirtualToPhysical(D_80364460[sp60].unk38));
                        break;
                }
            }
            gSPMatrix(gfx++, &D_02000000.unk0[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        }
        sp60++;
    }
    *arg0 = gfx;
}

void func_802502EC(void) {
    f32 sp70[4][4];
    f32 sp30[4][4];
    s32 sp2C;
    s16 sp2A;
    s16 sp28;
    s16 sp26;

    if (D_803643D6 != 0 && !(D_80364AA8 & 0x81)) {
        guTranslateF(sp70, -(f32)D_803643E0 / 32.0, -(f32)D_803643E4 / 32.0, -(f32)D_803643E8 / 32.0);
        guScaleF(sp30, D_80364ABC, D_80364ABC, D_80364ABC);
        guMtxCatF(sp70, sp30, sp70);
        guRotateF(sp30, D_80364AB4, 0.0f, 1.0f, 0.0f);
        guMtxCatF(sp70, sp30, sp70);
        guTranslateF(sp30, (f32)D_803643E0 / 32.0, (f32)D_803643E4 / 32.0, (f32)D_803643E8 / 32.0);
        guMtxCatF(sp70, sp30, sp70);
        guMtxF2L(sp70, &D_803156F8[D_8035805C].game.unk1500);
        sp2C = 0;
        while (D_803643C8[sp2C].unk1022 != D_80364456) {
            sp2C++;
        }
        sp2A = D_803643C8[sp2C].unk1004 >> 5;
        sp28 = D_803643C8[sp2C].unk1008 >> 5;
        sp26 = D_803643C8[sp2C].unk100C >> 5;
        guTranslateF(sp70, -sp2A, -sp28, -sp26);
        guScaleF(sp30, D_80364ABC, D_80364ABC, D_80364ABC);
        guMtxCatF(sp70, sp30, sp70);
        guRotateF(sp30, D_80364AB4, 0.0f, 1.0f, 0.0f);
        guMtxCatF(sp70, sp30, sp70);
        guTranslateF(sp30, sp2A, sp28, sp26);
        guMtxCatF(sp70, sp30, sp70);
        guMtxF2L(sp70, &D_803156F8[D_8035805C].game.unk1540);
        if (D_80364AC0 == 0) {
            D_80364ABC = D_80364ABC - 0.04;
            if (D_80364ABC < 0.0) {
                D_80364ABC = 0.0f;
            }
        }
        if (D_80364ABC == 0.0 && D_80364AC0 == 0) {
            func_802AC61C(D_803643E0, D_803643E4, D_803643E8, 0x13, 400000);
            D_80364AC0 = 1;
        }
        D_80364AB4 += D_80364AB8;
        D_80364AB8 = D_80364AB8 + 3.0;
    }
}

extern u8 D_8036B8B0;
extern s32 D_8036B8B4;
extern s32 D_8036B8B8;
extern s32 D_8036B8BC;
extern u8 D_8036B965;
extern u8 D_80370C1C;
extern u8 D_80370C1D;
extern s8 D_80370C2D;
extern u8 D_803A7424;
extern s32 D_803F7670;
extern s32 D_803F7674;
extern s32 D_803F7678;
extern s16 D_803F767C;
extern s16 D_803F767E;
extern s16 D_803F7680;
f32 func_80254E54(f32, f32, f32, f32, f32, f32);
void func_80255034(s32, f32, s32 *, s32 *);
void func_80255190(void);

void func_802507C8(Mtx *arg0, LookAt *arg1, Mtx *arg2) {
    f32 spFC;
    f32 spF8;
    f32 spF4;
    f32 spF0;
    f32 spEC;
    f32 spE8;
    s32 padE4;
    s16 spE2;
    s16 spE0;
    s16 spDE;
    s32 spD8;
    s32 spD4;
    f32 spD0;
    s32 spCC;
    f32 spC8;
    f32 spC4;
    s32 spC0;
    s32 spBC;
    s32 spB8;
    f32 spB4;
    f32 spB0;
    f32 spAC;
    f32 spA8;
    f32 spA4;
    f32 spA0;
    f32 sp9C;
    f32 sp98;
    s16 sp96;
    u8 sp95;
    s32 sp90;
    s32 sp8C;
    u8 sp8B;
    f32 sp84;
    f32 sp80;
    f32 sp7C;
    f32 sp78;
    s16 sp76;
    s32 sp70;
    s32 sp6C;
    s32 sp68;
    s8 sp67;
    s16 sp64;
    s16 sp62;

    sp8B = 0;
    func_80255190();
    if (D_8036B965 != 0 && (D_80364A90 & 0x600)) {
        D_8036B965 = 0;
    }
    sp95 = D_80364AC1 != 0 && ((D_80364A90 & 0x940) != 0 || D_8036B965 != 0);
    if (D_80364A90 == 8 || (D_8036BB18 == 0x4B && D_8036BB1C == 2 && D_802F8BDC[D_8036BB18].unk18 == YOSHI_ENTRY(0x14D))) {
        if (D_8030F668 == 0) {
            sp8B = 1;
        }
        D_8030F668 = 1;
    } else {
        if (D_8030F668 != 0) {
            sp8B = 1;
        }
        D_8030F668 = 0;
    }
    if (D_8036BB18 == 0x49 && D_8036BB1C == 2 && D_802F8BDC[D_8036BB18].unk18 == YOSHI_ENTRY(0x130)) {
        if (D_8030F66A == 0) {
            sp8B = 1;
        }
        D_8030F66A = 1;
    } else {
        if (D_8030F66A != 0) {
            sp8B = 1;
        }
        D_8030F66A = 0;
    }
    if (D_8036BB18 == 0x4D && D_8036BB1C == 2 && D_802F8BDC[D_8036BB18].unk18 != YOSHI_ENTRY(0x164)) {
        if (D_8030F669 == 0) {
            sp8B = 1;
        }
        D_8030F669 = 1;
    } else {
        if (D_8030F669 != 0) {
            sp8B = 1;
        }
        D_8030F669 = 0;
    }
    if (D_8036BB18 == 0x58 && D_8036BB1C != 8 && D_80364410 == 0) {
        if (D_80364A85 == 0) {
            sp8B = 1, D_80364412 = 1;
        }
        D_80364A85 = 1;
    } else {
        if (D_80364A85 != 0) {
            sp8B = 1, D_80364412 = 1;
        }
        D_80364A85 = 0;
    }
    D_80364412 |= sp95 != 0 || D_80364A90 == 0x100;
    D_80364412 |= D_80358060 < 2;
    if (D_803643D6 != 0 && ((D_80364A90 & 0x100000000600))) {
        spFC = 0.2f;
        spF8 = 0.1f;
        spF4 = 0.2f;
    } else if ((D_8036BB1C == 2 && (D_8036BB18 == 0x4D || D_8036BB18 == 0x4B || D_8036BB18 == 0x49)) ||
               (D_803643D7 != 0 && ((D_80364A90 & 0x100000000600))) || D_80364A90 == 8) {
        spFC = 0.08f;
        spF8 = 0.03f;
        spF4 = 0.08f;
    } else if (D_80364A90 & 0x1801) {
        spFC = 0.06f;
        spF8 = 0.03f;
        spF4 = 0.03f;
    } else if ((D_80364AA8 == 0x10 || D_80364AA8 == 0x40) && D_80364A90 == 0x40) {
        spFC = 0.1f;
        spF8 = 0.03f;
        spF4 = 0.1f;
    } else if (D_80364456 == 9) {
        spFC = 0.5f;
        spF8 = 0.06f;
        spF4 = 0.5f;
    } else {
        spFC = 0.5f;
        spF8 = 0.03f;
        spF4 = 0.5f;
    }
    spF0 = 0.95f;
    spEC = 0.97f;
    spE8 = 0.95f;
    if (D_80364A90 == 0x40 && D_802E8BDC == 0x3B) {
        spFC = 1.0f;
        spF8 = 1.0f;
        spF4 = 1.0f;
        spF0 = 1.0f;
        spEC = 1.0f;
        spE8 = 1.0f;
    }
    switch (D_80364A90) {
        case 0x100:
        case 0x200:
        case 0x400:
            if (D_80364AC1 != 0) {
                D_8036509E = 0xFD;
            } else {
                D_8036509E = 0xFF;
            }
            break;
        case 0x1:
        case 0x800:
        case 0x1000:
            if (D_80364AC1 != 0) {
                D_8036509E = 0xFD;
            } else {
                D_8036509E = 0xFE;
            }
            break;
        default:
            D_8036509E = D_80364456;
            break;
    }
    sp96 = D_8036443C;
    if (D_803649EC == 0) {
        D_8030F664 = func_802A56C4();
    }
    if (((D_80364A90 & 2) && D_802E8BF0 != 0) || D_803649EC == 0) {
        spCC = 0;
        spC8 = 0.0f;
        spC4 = 0.0f;
    } else {
        switch (D_8036509E) {
            case 0x7:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0xEB;
                D_80364A74 = -0xA5;
                break;
            case 0xA:
                spCC = 9;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0xF5;
                D_80364A74 = -0x87;
                break;
            case 0xB:
            case 0x11:
            case 0x12:
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                spCC = 0xB;
                D_80364A72 = 0xEB;
                D_80364A74 = -0xA5;
                break;
            case 0x8:
                spC8 = 3.0f;
                spC4 = 30.0f;
                spCC = 7;
                D_8030F664 = 0;
                D_80364A72 = 0x14F;
                D_80364A74 = -0xA5;
                break;
            case 0xF:
                spCC = 7;
                spC8 = 3.0f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0x131;
                D_80364A74 = -0xA5;
                break;
            case 0xD:
                spCC = 7;
                spC8 = 3.0f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0x145;
                D_80364A74 = -0xA5;
                break;
            case 0xE:
                spCC = 7;
                spC8 = 3.0f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0x13B;
                D_80364A74 = -0xA5;
                break;
            case 0x9:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0x3E8;
                D_80364A72 = 0x87;
                D_80364A74 = -0x55;
                break;
            case 0x6:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0x1388;
                D_80364A72 = 0;
                D_80364A74 = 0;
                break;
            case 0x5:
                spCC = 9;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0xCD;
                D_80364A74 = -0x69;
                break;
            case 0x4:
                spCC = 9;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0xEB;
                D_80364A74 = -0xA5;
                break;
            case 0x3:
                spCC = 7;
                spC8 = 3.0f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0xEB;
                D_80364A74 = -0xA5;
                break;
            case 0x2:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0x7D0;
                D_80364A72 = 0x91;
                D_80364A74 = -0x55;
                break;
            case 0x10:
                spCC = 2;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0x258;
                D_80364A72 = 0x91;
                D_80364A74 = -0x55;
                break;
            case 0x1:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                D_80364A72 = 0xCD;
                D_80364A74 = -0xA5;
                break;
            case 0x0:
                spCC = 0x14;
                spC8 = 0.25f;
                spC4 = 15.0f;
                D_8030F664 = 0;
                D_80364A72 = 0x55;
                D_80364A74 = -0x19;
                break;
            case 0xFF:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                sp96 /= 2;
                D_80364A72 = 0;
                D_80364A74 = 0;
                break;
            case 0xFE:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                sp96 = D_803EF328;
                D_8030F664 = 0;
                D_8036443E = D_803EF32A;
                D_80364A72 = 0;
                D_80364A74 = 0;
                break;
            case 0xFD:
                spCC = 0xB;
                spC8 = 0.75f;
                spC4 = 30.0f;
                D_8030F664 = 0;
                sp96 /= 2;
                D_8036443E /= 2;
                D_80364A72 = 0;
                D_80364A74 = 0;
                break;
        }
    }
    if (D_80364A90 == 0x40) {
        spCC /= 2;
    }
    if (sp96 > D_80364A72) {
        sp96 = D_80364A72;
    }
    if (sp96 < D_80364A74) {
        sp96 = D_80364A74;
    }
    if ((sp96 > 0 && D_80370C1C != 0 && D_802E8BD0 == 0 && (D_80364A90 & 0x106)) ||
        (sp96 < 0 && D_80370C1D != 0 && D_802E8BD0 == 0 && (D_80364A90 & 0x106))) {
        if (D_80364444 < sp96) {
            D_80364444 += spC8;
        }
        if (D_80364444 > sp96) {
            D_80364444 -= spC8;
        }
    } else {
        if (D_80364444 < sp96 && sp96 <= 0) {
            D_80364444 += spC8 * 10.0f;
            if (sp96 < D_80364444) {
                D_80364444 = sp96;
            }
        }
        if (D_80364444 > sp96 && sp96 >= 0) {
            D_80364444 -= spC8 * 10.0f;
            if (D_80364444 < sp96) {
                D_80364444 = sp96;
            }
        }
    }
    spD0 = D_8036443E - D_80364448;
    if (spD0 < 0.0) {
        spD0 = 0.0 - spD0;
    }
    if (spD0 > 2048.0) {
        if (D_80364448 > D_8036443E) {
            D_80364448 += spC4;
            if (D_80364448 > 4095.0) {
                D_80364448 = D_80364448 - 4095.0;
                if (D_80364448 > D_8036443E) {
                    D_80364448 = D_8036443E;
                }
            }
        } else {
            D_80364448 -= spC4;
            if (D_80364448 < 0.0) {
                D_80364448 = D_80364448 + 4095.0;
                if (D_80364448 < D_8036443E) {
                    D_80364448 = D_8036443E;
                }
            }
        }
    } else {
        if (D_80364448 > D_8036443E) {
            D_80364448 -= spC4;
            if (D_80364448 < D_8036443E) {
                D_80364448 = D_8036443E;
            }
        }
        if (D_80364448 < D_8036443E) {
            D_80364448 += spC4;
            if (D_80364448 > D_8036443E) {
                D_80364448 = D_8036443E;
            }
        }
    }
    spE2 = (s16) D_80364448 % 1024;
    spDE = func_80257514(spE2 / 1024.0f * 1.5708) * (spCC * D_80364444);
    spE0 = func_802574F0(spE2 / 1024.0f * 1.5708) * (spCC * D_80364444);
    if (D_80364448 >= 0.0f && D_80364448 < 1024.0f) {
        spD8 = spE0;
        spD4 = spDE;
    }
    if (D_80364448 >= 1024.0f && D_80364448 < 2048.0f) {
        spD8 = spDE;
        spD4 = -spE0;
    }
    if (D_80364448 >= 2048.0f && D_80364448 < 3072.0f) {
        spD8 = -spE0, spD4 = -spDE;
    }
    if (D_80364448 >= 3072.0f && D_80364448 < 4096.0f) {
        spD8 = -spDE;
        spD4 = spE0;
    }
    switch (D_80364A90) {
        case 0x100:
            if (D_80364AC1 != 0) {
                if (D_80364412 != 0) {
                    D_8036509C = D_803FCD68 * 16;
                } else {
                    D_8036509C = (s16) ((u16) D_803FCD68 * 16 - (u16) D_8036509C) / 10 + D_8036509C;
                }
                D_803643EC = (s32) (D_803FCD48 - func_802574F0(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD8) << 11;
                D_803643F0 = (D_803FCD4C + 0xDAC) << 11;
                D_803643F4 = (s32) (D_803FCD50 - func_80257514(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD4) << 11;
            } else {
                D_803643EC = (D_803EF6DC + 0xFA0) << 11;
                D_803643F0 = (D_803EF6E0 + 0x61A8) << 11;
                D_803643F4 = (D_803EF6E4 + D_80364A78 + 0x1B58) << 11;
            }
            break;
        case 0x40:
            switch ((u32) D_80364AA8) {
                case 0x1:
                case 0x80:
                    if (D_803643DB != 0) {
                        D_803643EC = (D_803EF6DC + spD8) << 11;
                        D_803643F0 = (D_803EF6E0 + 0x7D0) << 11;
                        D_803643F4 = (D_803EF6E4 + spD4) << 11;
                    }
                    if (D_80364AC1 != 0) {
                        if (D_80358060 < 2) {
                            D_8036509C = D_803FCD68 * 16;
                        } else {
                            D_8036509C = (s16) ((u16) D_803FCD68 * 16 - (u16) D_8036509C) / 10 + D_8036509C;
                        }
                        D_803643EC = (s32) (D_803FCD48 - func_802574F0(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD8) << 11;
                        D_803643F0 = (D_803FCD4C + 0xDAC) << 11;
                        D_803643F4 = (s32) (D_803FCD50 - func_80257514(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD4) << 11;
                    }
                    if (D_803643DB != 0 || D_80364AC1 != 0) {
                        break;
                    }
                case 0x4:
                case 0x8:
                    if (D_80364412 == 0) {
                        D_8036509C = (s16) ((u16) D_8036443E * 16 - (u16) D_8036509C) / 10 + D_8036509C;
                    } else {
                        D_8036509C = D_8036443E * 16;
                    }
                    D_803643EC = (s32) (D_803643E0 - func_802574F0(D_8036509C / 16384.0f * 1.5708) * (D_8036444C * 2.0f) + spD8) << 11;
                    D_803643F0 = (D_8036444E + D_803643E4) << 11;
                    D_803643F4 = (s32) (D_803643E8 - func_80257514(D_8036509C / 16384.0f * 1.5708) * (D_8036444C * 2.0f) + spD4) << 11;
                    break;
                case 0x2:
                case 0x20:
                    if (D_80364412 == 0) {
                        D_8036509C = (s16) ((u16) D_8036443E * 16 - (u16) D_8036509C) / 10 + D_8036509C;
                    } else {
                        D_8036509C = D_8036443E * 16;
                    }
                    D_803643EC = (s32) (func_802574F0(D_8036509C / 16384.0f * 1.5708) * (D_8036444C * 2.0f) + D_803643E0 + spD8) << 11;
                    D_803643F0 = (D_8036444E + D_803643E4 + D_80370C2D * 50) << 11;
                    D_803643F4 = (s32) (func_80257514(D_8036509C / 16384.0f * 1.5708) * (D_8036444C * 2.0f) + D_803643E8 + spD4) << 11;
                    break;
                case 0x10:
                case 0x40:
                    D_803643EC = ((D_8036BED8[D_8036BBB0[D_8036EA70.rt]].x << 5) + spD8) << 11;
                    D_803643F0 = ((D_8036BED8[D_8036BBB0[D_8036EA70.rt]].y << 5) + 0x7D0) << 11;
                    D_803643F4 = ((D_8036BED8[D_8036BBB0[D_8036EA70.rt]].z << 5) + spD4) << 11;
                    break;
            }
            break;
        case 0x200:
            if (D_80364AC1 != 0) {
                D_803643EC = (D_803FCD48 + spD8 - 0x1770) << 11;
                D_803643F0 = (D_803FCD4C + 0xD48) << 11;
                D_803643F4 = (D_803FCD50 + spD4 + 0x1770) << 11;
            } else if (D_803643D6 != 0) {
                D_803643EC = (D_803EF6DC + spD8 + 0x3E80) << 11;
                D_803643F0 = (D_803EF6E0 + 0x3A98) << 11;
                D_803643F4 = (D_803EF6E4 + spD4 - 0xBB8) << 11;
            } else {
                D_803643EC = (D_803EF6DC + spD8 + 0xFA0) << 11;
                D_803643F0 = (D_803EF6E0 + 0x960) << 11;
                if (D_802E8BDC == 0) {
                    D_803643F0 += 0x5DC000;
                }
                D_803643F4 = (D_803EF6E4 + spD4 - 0xFA0) << 11;
            }
            break;
        case 0x400:
            if (D_80364AC1 != 0) {
                D_803643EC = (D_803FCD48 + spD8 + 0x1770) << 11;
                D_803643F0 = (D_803FCD4C + 0xD48) << 11;
                D_803643F4 = (D_803FCD50 + spD4 - 0x1770) << 11;
            } else if (D_803643D6 != 0) {
                D_803643EC = (D_803EF6DC + spD8 - 0x3E80) << 11;
                D_803643F0 = (D_803EF6E0 + 0x3A98) << 11;
                D_803643F4 = (D_803EF6E4 - spD4 + 0xBB8) << 11;
            } else {
                D_803643EC = (D_803EF6DC + spD8 - 0xFA0) << 11;
                D_803643F0 = (D_803EF6E0 + 0x960) << 11;
                if (D_802E8BDC == 0) {
                    D_803643F0 += 0x5DC000;
                }
                D_803643F4 = (D_803EF6E4 + spD4 + 0xFA0) << 11;
            }
            break;
        case 0x1:
        case 0x800:
            if (D_80364AC1 != 0) {
                if (D_80358060 < 2) {
                    D_8036509C = D_803FCD68 * 16;
                } else {
                    D_8036509C = (s16) ((u16) D_803FCD68 * 16 - (u16) D_8036509C) / 10 + D_8036509C;
                }
                D_803643EC = (s32) (D_803FCD48 - func_802574F0(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD8) << 11;
                D_803643F0 = (D_803FCD4C + 0xDAC) << 11;
                D_803643F4 = (s32) (D_803FCD50 - func_80257514(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD4) << 11;
            } else {
                D_803643EC = (D_803643E0 + spD8 - 0x1F4) << 11;
                D_803643F0 = (D_803643E4 + 0x1770) << 11;
                D_803643F4 = (D_803643E8 + spD4 + 0x1F4) << 11;
            }
            break;
        case 0x1000:
            D_803643EC = (D_803643E0 + spD8 - 0xFA0) << 11;
            D_803643F0 = (D_803643E4 + 0x1388) << 11;
            D_803643F4 = (D_803643E8 + spD4 + 0xFA0) << 11;
            break;
        default:
            if (D_80364410 != 0) {
                D_803643EC = D_80364404;
                D_803643F0 = D_80364408;
                D_803643F4 = D_8036440C;
                break;
            }
            if (D_80364AA8 == 0x40) {
                D_8036444C = 0x2328;
                D_8036444E = 0x2AF8;
                D_80364420 = 0x2AF8;
            }
            if (D_80364AC1 != 0 && D_8036B8B0 != 0) {
                D_803643EC = (D_803FCD48 + D_8036B8B4) << 11;
                D_803643F0 = (D_803FCD4C + D_8036B8B8) << 11;
                D_803643F4 = (D_803FCD50 + D_8036B8BC) << 11;
            } else if (D_803643DB != 0 && D_8036B8B0 != 0) {
                D_803643EC = (D_803EF6DC + D_8036B8B4) << 11;
                D_803643F0 = (D_803EF6E0 + D_8036B8B8) << 11;
                D_803643F4 = (D_803EF6E4 + D_8036B8BC) << 11;
            } else if (D_8030F668 != 0) {
                func_80255034(0x9C4, D_80364414, &sp90, &sp8C);
                D_803643EC = ((D_803F767C << 5) + sp90) << 11;
                D_803643F0 = ((D_803F767E << 5) + 0x1388) << 11;
                D_803643F4 = ((D_803F7680 << 5) + sp8C) << 11;
            } else if (D_8030F66A != 0) {
                func_80255034(0x9C4, D_80364414, &sp90, &sp8C);
                D_803643EC = (sp90 + D_803F7670) << 11;
                D_803643F0 = (D_803F7674 + 0x1388) << 11;
                D_803643F4 = (sp8C + D_803F7678) << 11;
            } else if (D_8030F669 != 0) {
                if (D_8036C794 != NULL) {
                    func_80255034(0x9C4, D_80364414, &sp90, &sp8C);
                    D_803643EC = (((D_8036C794[0].unk0 + D_8036C794[3].unk0) / 2) << 16) + (sp90 << 11);
                    D_803643F0 = (((D_8036C794[0].unk2 + D_8036C794[3].unk2) / 2) << 16) + 0x9C4000;
                    D_803643F4 = (((D_8036C794[0].unk4 + D_8036C794[3].unk4) / 2) << 16) + (sp8C << 11);
                }
            } else {
                if (D_80364A85 != 0) {
                    sp76 = (f32) (D_8036443E * 360 / 4096) - D_80364414 + 180.0f;
                    if (sp76 < 0) {
                        sp76 += 360;
                    }
                    if (sp76 >= 360) {
                        sp76 %= 360;
                    }
                    if (D_80364456 == 2 || D_80364456 == 9) {
                        func_80255034(D_8036444C, sp76, &sp90, &sp8C);
                    } else {
                        func_80255034(D_8036444C / 2, sp76, &sp90, &sp8C);
                    }
                    D_803643F0 = (D_80364420 * 3 / 5 + D_803643E4) << 11;
                    spD8 = 0;
                    spD4 = 0;
                } else {
                    func_80255034(D_8036444C, D_80364414, &sp90, &sp8C);
                    if (D_80364411 != 0 && D_80364414 >= 134.0 && D_80364414 <= 136.0) {
                        D_803643F0 = (D_803643E4 + 0x3908) << 11;
                    } else {
                        D_803643F0 = (D_803643E4 + D_80364420) << 11;
                    }
                }
                D_803643EC = (D_803643E0 + sp90 + spD8) << 11;
                D_803643F4 = (D_803643E8 + sp8C + spD4) << 11;
            }
            break;
    }
    switch (D_80364A90) {
        case 0x100:
            if (D_80364AC1 != 0) {
                D_80365078 = D_803FCD48 / 32.0f;
                D_8036507C = (D_803FCD4C + 0x7D0) / 32.0f;
                D_80365080 = D_803FCD50 / 32.0f;
            } else {
                D_80365078 = D_803EF6DC / 32.0f;
                D_8036507C = D_803EF6E0 / 32.0f;
                D_80365080 = (D_803EF6E4 + D_80364A78 + 0x2AF8) / 32.0f;
            }
            break;
        case 0x200:
        case 0x400:
            if (D_80364AC1 != 0) {
                D_80365078 = (D_803FCD48 + spD8) / 32.0f;
                D_8036507C = (D_803FCD4C + 0x7D0) / 32.0f;
                D_80365080 = (D_803FCD50 + spD4) / 32.0f;
            } else {
                D_80365078 = (D_803EF6DC + spD8) / 32.0f;
                D_8036507C = D_803EF6E0 / 32.0f;
                D_80365080 = (D_803EF6E4 + spD4) / 32.0f;
            }
            break;
        case 0x1:
        case 0x800:
        case 0x1000:
            if (D_80364AC1 != 0) {
                D_80365078 = D_803FCD48 / 32.0f;
                D_8036507C = (D_803FCD4C + 0x7D0) / 32.0f;
                D_80365080 = D_803FCD50 / 32.0f;
            } else {
                D_80365078 = (D_803EF2EC + spD8) / 32.0f;
                D_8036507C = D_803EF314 / 32.0f;
                D_80365080 = (D_803EF2F4 + spD4) / 32.0f;
            }
            break;
        case 0x40:
            if (D_80364AC1 != 0) {
                D_80365078 = D_803FCD48 / 32.0f;
                D_8036507C = (D_803FCD4C + 0x7D0) / 32.0f;
                D_80365080 = D_803FCD50 / 32.0f;
                break;
            }
        default:
            if (D_80364AC1 != 0 && D_8036B8B0 != 0) {
                D_80365078 = D_803FCD48 / 32.0f;
                D_8036507C = (D_803FCD4C + 0x1770) / 32.0f;
                D_80365080 = D_803FCD50 / 32.0f;
            } else if (D_803643DB != 0 && D_8036B8B0 != 0) {
                D_80365078 = D_803EF6DC / 32.0f;
                D_8036507C = D_803EF6E0 / 32.0f;
                D_80365080 = D_803EF6E4 / 32.0f;
            } else if (D_8030F668 != 0) {
                D_80365078 = D_803F767C;
                D_8036507C = D_803F767E;
                D_80365080 = D_803F7680;
            } else if (D_8030F66A != 0) {
                D_80365078 = D_803F7670 / 32.0f;
                D_8036507C = D_803F7674 / 32.0f;
                D_80365080 = D_803F7678 / 32.0f;
            } else if (D_8030F669 != 0) {
                if (D_8036C794 != NULL) {
                    D_80365078 = (f32) (D_8036C794[0].unk0 + D_8036C794[3].unk0) / 2.0;
                    D_8036507C = (f32) (D_8036C794[0].unk2 + D_8036C794[3].unk2) / 2.0;
                    D_80365080 = (f32) (D_8036C794[0].unk4 + D_8036C794[3].unk4) / 2.0;
                }
            } else {
                D_80365078 = (D_803643E0 + spD8) / 32.0f;
                D_8036507C = (D_803643E4 + D_8030F664) / 32.0f;
                D_80365080 = (D_803643E8 + spD4) / 32.0f;
            }
            break;
    }
    if (D_803643D6 != 0 && D_802E8BDC == 0x31) {
        D_803643EC = (D_803EF6DC + spD8 + 0x3E80) << 11;
        D_803643F0 = (D_803EF6E0 + 0x3A98) << 11;
        D_803643F4 = (D_803EF6E4 + spD4 - 0xBB8) << 11;
    }
    if (D_80364AC1 != 0 && D_8036B965 != 0) {
        if (D_80364412 != 0) {
            D_8036509C = D_803FCD68 * 16;
        } else {
            D_8036509C = (s16) ((u16) D_803FCD68 * 16 - (u16) D_8036509C) / 10 + D_8036509C;
        }
        D_803643EC = (s32) (D_803FCD48 - func_802574F0(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD8) << 11;
        D_803643F0 = (D_803FCD4C + 0xDAC) << 11;
        D_803643F4 = (s32) (D_803FCD50 - func_80257514(D_8036509C / 16384.0f * 1.5708) * 8000.0f + spD4) << 11;
        D_80365078 = D_803FCD48 / 32.0f;
        D_8036507C = (D_803FCD4C + 0x7D0) / 32.0f;
        D_80365080 = D_803FCD50 / 32.0f;
    }
    if (D_80364A90 == 0x40 && D_802E8BDC == 0x3B) {
        D_803643EC = (D_803643E0 + 0x1900) << 11;
        D_803643F0 = (D_803643E4 + 0x3070) << 11;
        D_803643F4 = (D_803643E8 - 0x1900) << 11;
        D_80365078 = D_803643E0 / 32.0f;
        D_8036507C = D_803643E4 / 32.0f;
        D_80365080 = D_803643E8 / 32.0f;
    }
    if ((D_80364A90 & 0x2000000000002104) && D_8036B8B0 == 0 && sp95 == 0 && D_80364A90 != 0x100 && D_802E8BDC != 0x23 &&
        D_80364A85 == 0) {
        sp70 = D_80365078 * 65536.0;
        sp6C = D_8036507C * 65536.0;
        sp68 = D_80365080 * 65536.0;
        if (D_802E8BDC == 0x10 && D_80365078 > 8718.0f && D_80365078 < 8919.0f && D_80365080 > 9051.0f &&
            D_80365080 < 9251.0f) {
            D_802E8BE0 = 1.5f;
            spF8 = 0.1f;
        }
        D_803643EC = (D_803643EC - sp70) * D_802E8BE0 + sp70;
        D_803643F0 = (D_803643F0 - sp6C) * D_802E8BE0 + sp6C;
        D_803643F4 = (D_803643F4 - sp68) * D_802E8BE0 + sp68;
    }
    if (D_80364412 == 0) {
        D_803643F8 = (D_803643EC - D_803643F8) * spFC + D_803643F8;
        D_803643FC = (D_803643F0 - D_803643FC) * spF8 + D_803643FC;
        D_80364400 = (D_803643F4 - D_80364400) * spF4 + D_80364400;
    } else {
        D_803643F8 = D_803643EC;
        D_803643FC = D_803643F0;
        D_80364400 = D_803643F4;
        D_803650A0 = !D_8036509E;
    }
    if (D_80364424 != 0 && (D_80364414 < 134.0 || D_80364414 > 136.0)) {
        func_802CE4F0(D_803643F8 >> 11, D_803643FC >> 11, D_80364400 >> 11);
        func_802CE5BC(D_803643F8 >> 11, D_803643FC >> 11, D_80364400 >> 11,
                      (D_80364434 != 0) ? D_80364430 : D_80364430 + 0x640, 0xBE, 0);
        if (D_80364434 != 0) {
            if (D_803A7424 != 0) {
                D_80364420 += D_8036442C;
                if (D_80364428 < D_80364420) {
                    D_80364420 = D_80364428;
                }
                D_80364434 = 1;
            } else {
                D_80364434 = 0;
            }
        } else if (D_803A7424 != 0) {
            D_80364434 = 1;
        } else {
            D_80364420 -= D_8036442C;
            if (D_80364420 < D_8036444E) {
                D_80364420 = D_8036444E;
            }
            D_80364434 = 0;
        }
    }
    if (D_80364424 == 0) {
        D_80364420 = D_8036444E;
    } else if (D_80364414 >= 134.0 && D_80364414 <= 136.0) {
        D_80364420 -= D_8036442C;
        if (D_80364420 < D_8036444E) {
            D_80364420 = D_8036444E;
        }
    }
    if (D_8036509E != D_803650A0 || sp8B != 0) {
        D_80365090 = D_80365084 - D_80365078;
        D_80365094 = D_80365088 - D_8036507C;
        D_80365098 = D_8036508C - D_80365080;
        D_803650A0 = D_8036509E;
        D_8030F660 = 0;
    }
    if (D_8030F660 == 0 && D_80364412 == 0) {
        D_80365090 *= 0.95;
        D_80365084 = D_80365090 + D_80365078;
        D_80365094 *= 0.97;
        D_80365088 = D_80365094 + D_8036507C;
        D_80365098 *= 0.95;
        D_8036508C = D_80365098 + D_80365080;
        if (D_80365090 < 0.1 && D_80365090 > -0.1 && D_80365098 < 0.1 && D_80365098 > -0.1 && D_80365094 < 0.1 &&
            D_80365094 > -0.1) {
            D_8030F660 = 1;
        }
    } else {
        D_80365084 = D_80365078;
        D_80365088 = D_8036507C;
        D_8036508C = D_80365080;
        D_8030F660 = 1;
    }
    if (D_802E8BE8 == 0) {
        D_802E8BE8 = 1;
    }
    if (D_802E8BE4 != 0 && (D_80364A90 & 0x2000100000000206)) {
        spC0 = D_803649D8 % D_802E8BE8 - (D_802E8BE8 >> 1);
        spBC = D_803649D8 / 100 % D_802E8BE8 - (D_802E8BE8 >> 1);
        spB8 = D_803649D8 / 10000 % D_802E8BE8 - (D_802E8BE8 >> 1);
    } else {
        spC0 = 0;
        spBC = 0;
        spB8 = 0;
    }
    if (D_802E8BE4 != 0) {
        D_802E8BE4--;
        D_802E8BE8 -= D_802E8BE8 / 6;
        D_802E8BE8++;
    }
    spB4 = (f32) D_803643F8 / 65536.0;
    spB0 = (f32) D_803643FC / 65536.0;
    spAC = (f32) D_80364400 / 65536.0;
    if ((D_80364A90 & 2) && D_802E8BF0 != 0 && D_80358060 > D_80366A04) {
        spB4 = (spB4 - D_80365084) * D_803649F8 + D_80365084;
        spB0 = (spB0 - D_80365088) * D_803649F8 + D_80365088;
        spAC = (spAC - D_8036508C) * D_803649F8 + D_8036508C;
        if (D_803649F8 > 0.4) {
            D_803649F8 -= 0.01;
        }
    }
#ifdef TARGET_PC
    /* the follow camera tilted by the mouse or the right stick
       (port/host/ui.c): the eye drawn from, not the one the game keeps */
    if (D_80364A90 == 4 && D_8036B8B0 == 0 && sp95 == 0 && D_80364A85 == 0 && D_8030F668 == 0 &&
        D_8030F66A == 0 && D_8030F669 == 0) {
        f32 dx = spB4 - D_80365084;
        f32 dy = spB0 - D_80365088;
        f32 dz = spAC - D_8036508C;
        f32 h = sqrtf(dx * dx + dz * dz);

        if (h >= 1.0f) {
            f32 k = port_camera_pitch(0, h, dy) / h;

            spB0 = D_80365088 + port_camera_pitch(1, h, dy);
            spB4 = D_80365084 + dx * k;
            spAC = D_8036508C + dz * k;
        }
    }
#endif
    sp7C = (D_80365084 * 32.0f + spC0) / 32.0f;
    sp78 = (D_8036508C * 32.0f + spB8) / 32.0f;
    sp84 = spB4 - sp7C;
    if (sp84 < 0.0) {
        sp84 = 0.0 - sp84;
    }
    sp80 = spAC - sp78;
    if (sp80 < 0.0) {
        sp80 = 0.0 - sp80;
    }
    if (sp84 > 0.5 || sp80 > 0.5) {
        guLookAtReflect(arg0, arg1, spB4, spB0, spAC, (D_80365084 * 32.0f + spC0) / 32.0f,
                        (D_80365088 * 32.0f + spBC) / 32.0f, (D_8036508C * 32.0f + spB8) / 32.0f, 0.0f, 1.0f, 0.0f);
    } else {
        guLookAtReflect(arg0, arg1, spB4 + 2.0, spB0, spAC + 2.0, (D_80365084 * 32.0f + spC0) / 32.0f,
                        (D_80365088 * 32.0f + spBC) / 32.0f, (D_8036508C * 32.0f + spB8) / 32.0f, 0.0f, 1.0f, 0.0f);
    }
    sp98 = sqrtf((spB4 - D_80365084) * (spB4 - D_80365084) + (spAC - D_8036508C) * (spAC - D_8036508C));
    if (sp98 < 1.0) {
        sp98 = 1.0f;
    }
    if (spB4 >= D_80365084 && spAC >= D_8036508C) {
        D_80364452 = func_802AD7D4((spB4 - D_80365084) * 65535.9 / sp98) >> 4;
        sp67 = 0;
    }
    if (spB4 >= D_80365084 && spAC < D_8036508C) {
        D_80364452 = (func_802AD7D4((D_8036508C - spAC) * 65535.9 / sp98) >> 4) + 0x400,
        sp67 = 1;
    }
    if (spB4 < D_80365084 && spAC < D_8036508C) {
        D_80364452 = (func_802AD7D4((D_80365084 - spB4) * 65535.9 / sp98) >> 4) + 0x800,
        sp67 = 2;
    }
    if (spB4 < D_80365084 && spAC >= D_8036508C) {
        D_80364452 = (func_802AD7D4((spAC - D_8036508C) * 65535.9 / sp98) >> 4) + 0xC00,
        sp67 = 3;
    }
    sp98 = sqrtf((spB4 - D_80365084) * (spB4 - D_80365084) + (spAC - D_8036508C) * (spAC - D_8036508C) +
                 (spB0 - D_80365088) * (spB0 - D_80365088));
    if (spB0 >= D_80365088) {
        sp62 = func_802AD7D4((spB0 - D_80365088) * 65535.9 / sp98) >> 3;
    } else {
        sp62 = 0x1000 - (func_802AD7D4((D_80365088 - spB0) * 65535.9 / sp98) >> 3);
    }
    if (spB0 >= D_80365088) {
        sp64 = func_802AD7D4((65535.0 < (spB0 - D_80365088) * 65535.9 / 20000.0)
                                 ? 65535.0
                                 : (spB0 - D_80365088) * 65535.9 / 20000.0) >> 3;
    } else {
        sp64 = 0x1000 - (func_802AD7D4((65535.0 < (D_80365088 - spB0) * 65535.9 / 20000.0)
                                           ? 65535.0
                                           : (D_80365088 - spB0) * 65535.9 / 20000.0) >> 3);
    }
    D_80364454 = sp62 - sp64;
    sp9C = func_80254E54(spB4, spB0, spAC, D_80365084, D_80365088, D_8036508C);
    spA8 = (spB4 - D_80365084) * sp9C + D_80365084;
    spA4 = (spB0 - D_80365088) * sp9C + D_80365088;
    spA0 = (spAC - D_8036508C) * sp9C + D_8036508C;
    if (sp84 > 0.5 || sp80 > 0.5) {
        guLookAt(arg2, spA8, spA4, spA0, D_80365084, D_80365088, D_8036508C, 0.0f, 1.0f, 0.0f);
    } else {
        guLookAt(arg2, spA8 + 2.0, spA4, spA0 + 2.0, D_80365084, D_80365088, D_8036508C, 0.0f, 1.0f, 0.0f);
    }
    D_80364412 = 0;
}

f32 func_80254E54(f32 arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 arg5) {
    f32 sp2C;
    f32 sp28;
    f32 sp24;
    f32 sp20;
    f32 sp1C;

    switch (D_80364456) {
        case 0:
            sp2C = 3.5f;
            break;
        case 1:
            sp2C = 1.4f;
            break;
        case 2:
            sp2C = 1.6f;
            break;
        case 3:
            sp2C = 4.8f;
            break;
        case 4:
            sp2C = 5.0f;
            break;
        case 5:
            sp2C = 5.0f;
            break;
        case 6:
            sp2C = 1.6f;
            break;
        case 7:
            sp2C = 1.6f;
            break;
        case 8:
            sp2C = 4.8f;
            break;
        case 9:
            sp2C = 1.6f;
            break;
        case 10:
            sp2C = 5.0f;
            break;
        case 11:
        case 17:
        case 18:
            sp2C = 1.6f;
            break;
        case 13:
            sp2C = 4.8f;
            break;
        case 14:
            sp2C = 4.8f;
            break;
        case 15:
            sp2C = 4.8f;
            break;
        case 16:
            sp2C = 1.6f;
            break;
    }
    sp28 = arg0 - arg3;
    sp28 = sp28 * sp28;
    sp24 = arg1 - arg4;
    sp24 = sp24 * sp24;
    sp20 = arg2 - arg5;
    sp20 = sp20 * sp20;
    sp1C = sqrtf(sp28 + sp24 + sp20);
    if (sp1C * sp2C > 10000.0f) {
        sp2C -= (sp1C * sp2C - 10000.0f) / sp1C;
    }
    return sp2C;
}

void func_80255034(s32 arg0, f32 arg1, s32 *arg2, s32 *arg3) {
    f32 sp2C;
    f32 sp28;
    s32 sp24;
    s32 sp20;
    f32 sp1C;

    sp28 = arg0 * arg0;
    sp2C = sqrtf(sp28 + sp28);
    sp1C = arg1;
    sp1C = sp1C / 360.0;
    sp1C = sp1C * 6.28318;
    sp24 = sinf(sp1C) * sp2C;
    sp20 = sqrtf(sp2C * sp2C - sp24 * sp24);
    if (arg1 >= 90.0 && arg1 < 270.0) {
        sp20 = 0.0 - sp20;
    }
    if (sp20 >= -100 && sp20 <= 100) {
        sp20 = 0;
    }
    *arg2 = sp24;
    *arg3 = sp20;
}

void func_80255190(void) {
    u8 sp1F;

    if (D_802E8BD0 == 0) {
        sp1F = func_80255628();
        if (sp1F != 0 && ((D_80370C21 != 0 && D_80370C27 == 0) || (D_80370C1E != 0 && D_80370C24 == 0)) &&
            (D_80364A90 & 0x104)) {
            func_80260650(D_80367738, 0xD0, NULL);
        }
        if (D_80370C21 != 0 && D_80370C27 == 0 && D_8036441C == 0 && D_8036441D == 0 && sp1F == 0 &&
            D_80364A90 != 0x2000) {
            D_80364418 = D_80364414 + 45.0;
            if (D_80364418 >= 360.0) {
                D_80364418 = D_80364418 - 360.0;
            }
            D_8036441C = 1;
            if (D_80364A90 != 0x40) {
                func_80260650(D_80367738, 0xDD, NULL);
            }
        }
        if (D_8036441C != 0) {
            if (D_80364414 < D_80364418) {
                D_80364414 = D_80364414 + 3.0;
                if (D_80364414 >= D_80364418) {
                    D_8036441C = 0;
                    D_80364414 = D_80364418;
                }
            } else {
                D_80364414 = D_80364414 + 3.0;
                if (D_80364414 >= 360.0) {
                    D_80364414 = D_80364414 - 360.0;
                    if (D_80364414 >= D_80364418) {
                        D_8036441C = 0;
                        D_80364414 = D_80364418;
                    }
                }
            }
        }
        if (D_80370C1E != 0 && D_80370C24 == 0 && D_8036441C == 0 && D_8036441D == 0 && sp1F == 0 &&
            D_80364A90 != 0x2000) {
            D_80364418 = D_80364414 - 45.0;
            if (D_80364418 < 0.0) {
                D_80364418 = D_80364418 + 360.0;
            }
            D_8036441D = 1;
            if (D_80364A90 != 0x40) {
                func_80260650(D_80367738, 0xDC, NULL);
            }
        }
        if (D_8036441D != 0) {
            if (D_80364414 > D_80364418) {
                D_80364414 = D_80364414 - 3.0;
                if (D_80364414 <= D_80364418) {
                    D_8036441D = 0;
                    D_80364414 = D_80364418;
                }
            } else {
                D_80364414 = D_80364414 - 3.0;
                if (D_80364414 < 0.0) {
                    D_80364414 = D_80364414 + 360.0;
                    if (D_80364414 <= D_80364418) {
                        D_8036441D = 0;
                        D_80364414 = D_80364418;
                    }
                }
            }
        }
#ifdef TARGET_PC
        /* the mouse and the right stick turn the camera freely
           (port/host/ui.c); a C turn under way keeps its 45 degrees */
        {
            s32 turn = port_camera_turn();

            if (turn != 0 && sp1F == 0 && D_80364A90 != 0x2000) {
                D_80364414 += turn / 1000.0f;
                D_80364418 += turn / 1000.0f;
                while (D_80364414 >= 360.0f) D_80364414 -= 360.0f;
                while (D_80364414 < 0.0f) D_80364414 += 360.0f;
                while (D_80364418 >= 360.0f) D_80364418 -= 360.0f;
                while (D_80364418 < 0.0f) D_80364418 += 360.0f;
            }
        }
#endif
        if (D_8036441D != 0 || D_8036441C != 0) {
            func_802A45D4(2);
        }
    }
}

u8 func_80255628(void) {
    u8 sp37 = 0;

    if (D_80364A90 == 0x100000000000LL || D_80364A90 == 2) {
        sp37 = 1;
    } else {
        switch (D_802E8BDC) {
            case 4:
                if (func_802AC4C4(D_803643E0 >> 5, D_803643E8 >> 5, 0xE56, 0x8EC, 0xBB8, 0x6A4, 0x1068, 0x1F4) != 0 ||
                    func_802AC4C4(D_803643E0 >> 5, D_803643E8 >> 5, 0xE56, 0x8EC, 0x1068, 0x1F4, 0x1324, 0x4B0) != 0) {
                    sp37 = 1;
                }
                break;
            case 16:
                if (D_803643E0 > 0x46500 && D_803643E8 > 0x3E800) {
                    sp37 = 1;
                }
                break;
            case 13:
                if (D_803643E0 > 0x42680 && D_803643E8 > 0x46500) {
                    sp37 = 1;
                }
                break;
            case 59:
                sp37 = 1;
                break;
        }
    }
    if (sp37 != 0 && (D_80364418 < 134.0 || D_80364418 > 136.0)) {
        D_8036441D = 0;
        D_8036441C = 0;
        D_80364418 = 135.0f;
        if (D_80364414 > D_80364418) {
            if (D_80364414 - D_80364418 > 180.0) {
                D_8036441C = 1;
            } else {
                D_8036441D = 1;
            }
        } else {
            D_8036441C = 1;
        }
    }
    return sp37;
}

void func_802558C8(Gfx *arg0, s32 *arg1) {
    Gfx *gfx = arg0;

    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gDPSetFillColor(gfx++, 0x10001);
    gDPPipeSync(gfx++);
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    *arg1 += gfx - arg0;
}

void func_802559F8(Gfx *arg0, s32 *arg1) {
    Gfx *gfx = arg0;

    gDPFullSync(gfx++);
    gSPEndDisplayList(gfx++);
    *arg1 = ((u8 *)gfx - (u8 *)D_803156F8[D_8035805C].game.unk48B0) >> 3;
    if (*arg1 >= 0xB5E) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "*length<TOPLEVEL_DL_SIZE", "hd.c", LINE_EU(3665, 3681));
    }
}

#ifdef VERSION_EU
extern OSMesgQueue D_80219EF8;
extern OSMesgQueue D_80219F50;
#endif

void func_80255AD0(void) {
    s32 sp44;
    f32 sp40;
    s32 pad[4];
    u8 sp2F;

    sp40 = 1.0f;
    D_80364A90 = 0x20;
    osCreateMesgQueue(&D_803150A0, D_803150B8, 50);
    osCreateMesgQueue(&D_80315180, D_80315198, 144);
    osCreateScheduler(&D_80315440, &D_80312D80[0x2000 / sizeof(u64)], 13,
                  (D_80000300 != 1) ? OS_VI_PAL_LAN1 : OS_VI_NTSC_LAN1, 1);
    osCreateMesgQueue(&D_803153D8, D_803153F8, 16);
    osScAddClient(&D_80315440, &D_803156D8, &D_803153D8, 1, 1);
    sp2F = func_8028A370();
    func_80261588();
    func_8029A7E4("audio inited\n");
    osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
    osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON);
    D_80358050[0] = (u16 *)(uintptr_t)K0_TO_PHYS(D_80000400[0]);
    D_80358050[1] = (u16 *)(uintptr_t)K0_TO_PHYS(D_80000400[1]);
    D_80358058 = (u16 *)(uintptr_t)K0_TO_PHYS(D_8021ED00);
    func_80284DB0();
    osWritebackDCacheAll();
    func_8028FC10();
#ifdef VERSION_EU
    /* the front end reads the saved language */
    func_8028B3E0();
    osSendMesg(&D_80219EF8, (OSMesg)0x01000016, OS_MESG_BLOCK);
    osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
    func_8029A7E4("Saved language is %d\n", D_80366F70_eu);
#endif
    if (!(sp2F & 1)) {
        D_80364A98 = 0x0800000000000000LL;
    } else if (D_802FDBD0 != 0) {
        D_80364A98 = 0x0000080000000000LL;
    } else if (D_802FDBD4 != 0) {
        D_80364A98 = 0x0040000000000000LL;
    } else {
        D_80364A98 = 0x10;
    }
    sp44 = 0x1FF;
    do {
        D_80310D80[sp44] = 0x1122334455667788LL;
    } while (--sp44 >= 0);
}

void func_80255D34(void) {
    u8 sp1F = 0;
    s32 sp18 = 0x1FF;

    do {
        if (D_80310D80[sp18] != 0x1122334455667788LL) {
            sp1F = 1;
            func_8029A7E4("stack end =%d\n", sp18);
        }
    } while (--sp18 >= 0 && sp1F == 0);
}

void func_80255DC8(void) {
    u8 *sp2C;
    s32 pad;
    s32 sp24;
    u8 *sp20;

    sp24 = D_00788000 - D_00787F40;
    osViBlack(TRUE);
    D_80364A70 = func_80261A44(D_80364A98);
    osWritebackDCacheAll();
#ifdef TARGET_PC
    osInvalDCache(NULL, 0x400000);   /* all of RDRAM: a no-op here */
#else
    osInvalDCache((void *)0x80000000, 0x400000);
#endif
    D_803649F4 = 0;
    D_80358068 = 0;
    D_80358064 = 0;
    D_80358060 = 0;
    D_8035805C = 0;
    D_80367BD6 = 0;
    func_8026A8BC();
    func_8026A974();
    func_8028AE88();
    func_8028B720();
    D_8035806C = D_803FF600;
    func_8028B4C4((u32)D_00787F40, D_803FF600, &sp24, 10, 0, 2);
    sp2C = D_803FF600 + (D_00788000 - D_00787F40);
#ifdef TARGET_PC
    /* debug output: the room is up to init's hand-over words */
    func_8029A7E4("Static end = 0x%x, space=0x%x (%d) bytes\n", sp2C, (u32)(D_803FFFF8 - sp2C), (u32)(D_803FFFF8 - sp2C));
#else
    func_8029A7E4("Static end = 0x%x, space=0x%x (%d) bytes\n", sp2C, 0x80400000 - (u32)sp2C, 0x80400000 - (u32)sp2C);
#endif
    D_80358078 = 0;
    func_802558C8(D_803156F8[D_8035805C].game.unk48B0, &D_80358078);
    func_802559F8(D_803156F8[D_8035805C].game.unk48B0, &D_80358078);
    D_80358070 = (u8 *)MEM_POOL;
    func_80257490((s32 *)&D_80358070, 16);
    D_8036E694 = (u64 *)D_80358070;
    D_80358070 += 0xA000;
    if (D_802E8F94[D_802E8BDC].unk0 == 2 && !(D_80364A98 & 0x100000000002LL)) {
        func_8029A7E4("Allocating ghost buffer memory\n");
        D_80358070 += 0x20000;
    }
    D_803B9888 = 0;
    func_802A0700();
    D_803643C8 = (UnkStruct_803643C8 *)((uintptr_t)&D_80358088[0x40] & ~0x3F);
    func_80278E3C();
    D_803643D9 = 0;
    D_803643DA = 0;
    D_803643D8 = 0;
    D_803643D6 = 0;
    D_803643D7 = 0;
    D_802E8BD8 = 0;
    D_802E8BD4 = 0;
    D_802E8BD0 = 0;
    D_8036EB99 = 0;
    D_803669B4 = 0;
    if (D_80364A98 & 0xC9FD8FE7FBFFC0B0LL) {
        func_8028B3E0();
    }
    func_80297530(D_802E8BDC);
    func_80272C50();
    if (D_80364A98 == 0x40000000000LL) {
        func_801F7850();
    }
    sp20 = D_80358070;
    func_8026B118(0);
    func_8029A7E4("Yoshi windows allocated %d bytes, %x\n", D_80358070 - sp20, D_80358070);
    D_803649D0 = D_80364460;
    func_8028A42C();
    func_802592F0();
    D_8039CAA2 = 0;
}

extern u8 D_004A5660[];
extern u8 D_004ACC10[];
extern u8 D_004B8960[];
extern u8 D_004BFD60[];
extern u8 D_004C3AC0[];
extern u8 D_004D5F90[];
extern u8 D_004E2F70[];
extern u8 D_004E4E80[];
extern u8 D_004E7C00[];
extern u8 D_004E8F70[];
extern u8 D_004F5C10[];
extern u8 D_00500520[];
extern u8 D_00507E80[];
extern u8 D_00511340[];
extern u8 D_00523080[];
extern u8 D_0052CD00[];
extern u8 D_00532700[];
extern u8 D_0053E9B0[];
extern u8 D_0054A820[];
extern u8 D_00552DE0[];
extern u8 D_00555000[];
extern u8 D_00560E90[];
extern u8 D_005652D0[];
extern u8 D_0056F3F0[];
extern u8 D_005721E0[];
extern u8 D_005736E0[];
extern u8 D_0057A2C0[];
extern u8 D_00580B60[];
extern u8 D_00588CE0[];
extern u8 D_0058BE80[];
extern u8 D_00597B80[];
extern u8 D_0059B7D0[];
extern u8 D_005A5840[];
extern u8 D_005B0B10[];
extern u8 D_005B5A30[];
extern u8 D_005B8BB0[];
extern u8 D_005C4C80[];
extern u8 D_005CA9C0[];
extern u8 D_005CCF50[];
extern u8 D_005D1060[];
extern u8 D_005DC830[];
extern u8 D_005E6EE0[];
extern u8 D_005EC800[];
extern u8 D_005F3A80[];
extern u8 D_006014B0[];
extern u8 D_0060A710[];
extern u8 D_00613AA0[];
extern u8 D_0061DD70[];
extern u8 D_00621AF0[];
extern u8 D_006269E0[];
extern u8 D_00630C30[];
extern u8 D_00635700[];
extern u8 D_0063CA10[];
extern u8 D_00641F30[];
extern u8 D_00644810[];
extern u8 D_00646080[];
extern u8 D_00647550[];
extern u8 D_00654FC0[];
extern u8 D_00660950[];
extern u8 D_00665F80[];
extern u8 D_0066C900[];

void func_8025615C(s32 arg0, u8 *arg1, s32 *arg2) {
    u8 *sp24;

    switch (arg0) {
        case 1:
            sp24 = D_004A5660;
            *arg2 = D_004ACC10 - D_004A5660;
            break;
        case 0:
            sp24 = D_004ACC10;
            *arg2 = D_004B8960 - D_004ACC10;
            break;
        case 2:
            sp24 = D_004B8960;
            *arg2 = D_004BFD60 - D_004B8960;
            break;
        case 3:
            sp24 = D_004BFD60;
            *arg2 = D_004C3AC0 - D_004BFD60;
            break;
        case 4:
            sp24 = D_004C3AC0;
            *arg2 = D_004D5F90 - D_004C3AC0;
            break;
        case 5:
            sp24 = D_004D5F90;
            *arg2 = D_004E2F70 - D_004D5F90;
            break;
        case 6:
            sp24 = D_004E2F70;
            *arg2 = D_004E4E80 - D_004E2F70;
            break;
        case 7:
            sp24 = D_004E4E80;
            *arg2 = D_004E7C00 - D_004E4E80;
            break;
        case 8:
            sp24 = D_004E7C00;
            *arg2 = D_004E8F70 - D_004E7C00;
            break;
        case 9:
            sp24 = D_004E8F70;
            *arg2 = D_004F5C10 - D_004E8F70;
            break;
        case 10:
            sp24 = D_004F5C10;
            *arg2 = D_00500520 - D_004F5C10;
            break;
        case 11:
            sp24 = D_00500520;
            *arg2 = D_00507E80 - D_00500520;
            break;
        case 12:
            sp24 = D_00507E80;
            *arg2 = D_00511340 - D_00507E80;
            break;
        case 13:
            sp24 = D_00511340;
            *arg2 = D_00523080 - D_00511340;
            break;
        case 14:
            sp24 = D_00523080;
            *arg2 = D_0052CD00 - D_00523080;
            break;
        case 15:
            sp24 = D_0052CD00;
            *arg2 = D_00532700 - D_0052CD00;
            break;
        case 16:
            sp24 = D_00532700;
            *arg2 = D_0053E9B0 - D_00532700;
            break;
        case 17:
            sp24 = D_0053E9B0;
            *arg2 = D_0054A820 - D_0053E9B0;
            break;
        case 18:
            sp24 = D_0054A820;
            *arg2 = D_00552DE0 - D_0054A820;
            break;
        case 19:
            sp24 = D_00552DE0;
            *arg2 = D_00555000 - D_00552DE0;
            break;
        case 20:
            sp24 = D_00555000;
            *arg2 = D_00560E90 - D_00555000;
            break;
        case 21:
            sp24 = D_00560E90;
            *arg2 = D_005652D0 - D_00560E90;
            break;
        case 22:
            sp24 = D_005652D0;
            *arg2 = D_0056F3F0 - D_005652D0;
            break;
        case 23:
            sp24 = D_0056F3F0;
            *arg2 = D_005721E0 - D_0056F3F0;
            break;
        case 24:
            sp24 = D_005721E0;
            *arg2 = D_005736E0 - D_005721E0;
            break;
        case 25:
            sp24 = D_005736E0;
            *arg2 = D_0057A2C0 - D_005736E0;
            break;
        case 26:
            sp24 = D_0057A2C0;
            *arg2 = D_00580B60 - D_0057A2C0;
            break;
        case 27:
            sp24 = D_00580B60;
            *arg2 = D_00588CE0 - D_00580B60;
            break;
        case 28:
            sp24 = D_00588CE0;
            *arg2 = D_0058BE80 - D_00588CE0;
            break;
        case 29:
            sp24 = D_0058BE80;
            *arg2 = D_00597B80 - D_0058BE80;
            break;
        case 30:
            sp24 = D_00597B80;
            *arg2 = D_0059B7D0 - D_00597B80;
            break;
        case 31:
            sp24 = D_0059B7D0;
            *arg2 = D_005A5840 - D_0059B7D0;
            break;
        case 32:
            sp24 = D_005A5840;
            *arg2 = D_005B0B10 - D_005A5840;
            break;
        case 33:
            sp24 = D_005B0B10;
            *arg2 = D_005B5A30 - D_005B0B10;
            break;
        case 34:
            sp24 = D_005B5A30;
            *arg2 = D_005B8BB0 - D_005B5A30;
            break;
        case 35:
            sp24 = D_005B8BB0;
            *arg2 = D_005C4C80 - D_005B8BB0;
            break;
        case 36:
            sp24 = D_005C4C80;
            *arg2 = D_005CA9C0 - D_005C4C80;
            break;
        case 37:
            sp24 = D_005CA9C0;
            *arg2 = D_005CCF50 - D_005CA9C0;
            break;
        case 38:
            sp24 = D_005CCF50;
            *arg2 = D_005D1060 - D_005CCF50;
            break;
        case 39:
            sp24 = D_005D1060;
            *arg2 = D_005DC830 - D_005D1060;
            break;
        case 40:
            sp24 = D_005DC830;
            *arg2 = D_005E6EE0 - D_005DC830;
            break;
        case 41:
            sp24 = D_005E6EE0;
            *arg2 = D_005EC800 - D_005E6EE0;
            break;
        case 42:
            sp24 = D_005EC800;
            *arg2 = D_005F3A80 - D_005EC800;
            break;
        case 43:
            sp24 = D_005F3A80;
            *arg2 = D_006014B0 - D_005F3A80;
            break;
        case 44:
            sp24 = D_006014B0;
            *arg2 = D_0060A710 - D_006014B0;
            break;
        case 45:
            sp24 = D_0060A710;
            *arg2 = D_00613AA0 - D_0060A710;
            break;
        case 46:
            sp24 = D_00613AA0;
            *arg2 = D_0061DD70 - D_00613AA0;
            break;
        case 47:
            sp24 = D_0061DD70;
            *arg2 = D_00621AF0 - D_0061DD70;
            break;
        case 48:
            sp24 = D_00621AF0;
            *arg2 = D_006269E0 - D_00621AF0;
            break;
        case 49:
            sp24 = D_006269E0;
            *arg2 = D_00630C30 - D_006269E0;
            break;
        case 50:
            sp24 = D_00630C30;
            *arg2 = D_00635700 - D_00630C30;
            break;
        case 51:
            sp24 = D_00635700;
            *arg2 = D_0063CA10 - D_00635700;
            break;
        case 52:
            sp24 = D_0063CA10;
            *arg2 = D_00641F30 - D_0063CA10;
            break;
        case 53:
            sp24 = D_00641F30;
            *arg2 = D_00644810 - D_00641F30;
            break;
        case 54:
            sp24 = D_00644810;
            *arg2 = D_00646080 - D_00644810;
            break;
        case 55:
            sp24 = D_00646080;
            *arg2 = D_00647550 - D_00646080;
            break;
        case 56:
            sp24 = D_00647550;
            *arg2 = D_00654FC0 - D_00647550;
            break;
        case 57:
            sp24 = D_00654FC0;
            *arg2 = D_00660950 - D_00654FC0;
            break;
        case 58:
            sp24 = D_00660950;
            *arg2 = D_00665F80 - D_00660950;
            break;
        case 59:
            sp24 = D_00665F80;
            *arg2 = D_0066C900 - D_00665F80;
            break;
    }
    func_8028B4C4((u32)(uintptr_t)sp24, arg1, arg2, 12, 10, 1);
}

void func_80256A34(u8 *arg0) {
    s32 sp4C;
    s32 sp48;
    UnkStruct_803643C8 *sp44;
    u8 sp43;
    u8 *sp3C;

    D_80364A68 = 0;
    D_80364A69 = 0;
    D_80364A6A = 0;
    D_80364A6B = 0;
    D_80364A6C = 0;
    D_80364A6D = 0;
    D_803649E8 = 0;
    D_803649EC = 0;
    D_803643CC = D_803643C8;
    D_80364AA8 = D_802E8F94[D_802E8BDC].unk0;
    D_802E8BE4 = 0;
    D_802E8BE8 = 0;
    D_803643E0 = 0;
    D_803643E4 = 0;
    D_803643E8 = 0;
    D_8036443C = 0;
    D_80364414 = 135.0f;
    D_80364418 = 135.0f;
    D_8036441C = 0;
    D_8036441D = 0;
    D_80364420 = 3000;
    D_80364434 = 1;
    D_803A7430 = 0;
    D_803649EE = 0;
    D_80364A84 = 0;
    D_80364456 = 0;
    if (D_80370C50 != 0) {
        if (D_8039C4B0 != 0) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!pakBusy", "hd.c", LINE_EU(4162, 4185));
        }
#ifdef TARGET_PC
        if (D_80219F50.validCount != 0) {
#else
        if (D_80219F58 != 0) {      /* (D_80219F50.validCount) */
#endif
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "MQ_IS_EMPTY(&pakToGameMessageQ)", "hd.c", LINE_EU(4163, 4186));
        }
        osScRemoveClient(&D_80315440, &D_80218EE0);
        osDestroyThread(&D_80218D30);
    }
    D_8039CA62 = 0;
    D_8039CA61 = 0;
    if (D_80364AA8 == 2 && D_80364A98 == 0x2000) {
        if (D_8039CA60 == 0) {
            func_80294E30();
        }
        func_80294E88();
        func_80294EB8();
    }
    func_8025615C(D_802E8BDC, D_80358070, &sp48);
    D_80358074 = (LevelHeader *)D_80358070;
    D_80358070 += sp48;
    func_80257490((s32 *)&D_80358070, 16);
    func_80285190();
    func_80275430();
    func_802621DC(D_802E8BDC);
    func_80262238(D_802E8BDC);
    func_80262150(D_802E8BDC);
    D_80367BFF = 0;
    func_802CE840();
    func_8029A7E4("enter initlevel game_mode=%d loop_done=%d\n", func_8026F92C(D_80364A90), func_8026F92C(D_80364A98));
    sp3C = D_80358070;
    func_802A1674(D_80358074, arg0);
    func_8029A7E4("exit initlevel allocated %d bytes, %x\n", D_80358070 - sp3C, D_80358070);
    func_80257234();
    if (D_80364A98 != 2) {
        if LEVEL_DONE_IN(D_80364AF0[D_80364AE8], D_802E8BDC) {
            func_802CF628();
        }
    }
    func_802C1DD0(D_80364AA8 == 0x20 || D_80364AA8 == 0x80);
    func_80262320(D_802E8BDC);
    if (D_80364410 != 0) {
        D_80364A86 = func_80272C5C(D_802E8BF4, 0, 1, 1, 1, 1.0f);
    }
    func_802775C0();
    if (D_80364A69 != 0) {
        func_80286A00();
    }
    if (D_80364A6B != 0) {
        func_802873AC();
    }
    if (D_80364A6D != 0) {
        func_80287AE4();
    }
    func_802821D0();
    func_80282728();
    func_80281A70(D_802E8BDC);
    if (D_802E8F94[D_802E8BDC].unk0 == 1) {
        func_80264C20(arg0);
    }
    func_80288220();
    func_8027BE4C();
    func_80292240();
    func_8027E344(D_802E8BDC);
    func_802807D8(D_802E8BDC);
    func_80268664(D_802E8BDC);
    func_8026A988();
    sp44 = D_803643C8;
    while (sp44 != D_803643CC) {
        sp4C = 0;
        sp43 = 0;
        do {
            if (D_80364460[sp4C].type == sp44->unk1022) {
                sp43 = 1;
            } else {
                sp4C++;
            }
        } while (sp43 == 0);
        func_80258544(sp44, sp44->unk1004, sp44->unk1010, sp44->unk100C, sp44->unk1000, D_80364460[sp4C].unk54,
                      D_80364460[sp4C].unk0, D_80364460[sp4C].unk4);
        func_80285110(0x61F);
        sp44++;
    }
    func_802729F0(D_80364A98, D_802E8BDC);
    D_80364452 = 0x2000;
    D_80364454 = 0x2000;
    D_80364456 = 0;
    D_803649ED = 0;
    D_803643F8 = 0;
    D_803643FC = 0;
    D_80364400 = 0;
    D_80364444 = 0.0f;
    D_80364448 = 0.0f;
    D_803649F8 = 1.0f;
    D_80364A3C = 0;
    D_80364A3D = 0;
    D_80364A40 = 0;
    D_80364A44 = 0;
    D_80364A6F = 0;
    D_80364AB4 = 0.0f;
    D_80364AB8 = 1.0f;
    D_80364ABC = 1.0f;
    D_80364AC0 = 0;
    D_803643E0 = D_803ED808;
    D_803643E4 = D_803ED80C;
    D_803643E8 = D_803ED810;
    D_802FDB14 = 0;
    if (D_802E8BF8 != 0) {
        D_803649F0 = D_8036EA70.ip;
    } else {
        D_803649F0 = D_80364AF0[D_80364AE8].unk14;
    }
    D_80364A5C = 0;
    D_80364A58 = 0;
    if (D_803669B4 != 0) {
        func_8025BD98();
    }
#ifdef TARGET_PC
    /* debug output: init's area is the module's load address, the pool ends where it starts */
    func_8029A7E4("Level %d: mem_pool=0x%x, code seg=0x%x, space=%d bytes\n", D_802E8BDC, D_80358070, D_8021ED00 + PORT_REGION_SIZE_D_8021ED00, (u32)(D_8004B400 + PORT_REGION_SIZE_D_8004B400 - D_80358070));
#else
    func_8029A7E4("Level %d: mem_pool=0x%x, code seg=0x%x, space=%d bytes\n", D_802E8BDC, D_80358070, 0x802447C0, 0x8021ED00 - (u32)D_80358070);
#endif
    if (D_8039CAB7 != 0) {
        func_802979E0(D_802E8BDC);
    }
    func_802A56C4();
    func_802A5FA8();
}


void func_80257234(void) {
    s32 sp14;
    s32 sp10;
    s32 spC;
    s32 sp8;

    switch (D_802E8BDC) {
        case 16:
        case 29:
            sp14 = 50;
            sp10 = 50;
            spC = 50;
            sp8 = 200;
            break;
        case 17:
            sp14 = 50;
            sp10 = 50;
            spC = 50;
            sp8 = 50;
            break;
        case 11:
            sp14 = 3000;
            sp10 = 50;
            spC = 50;
            sp8 = 200;
            break;
        case 10:
            sp14 = 50;
            sp10 = 50;
            spC = 50;
            sp8 = 2000;
            break;
        default:
            switch (D_803BE739) {
                case 0:
                    sp14 = 6000;
                    sp10 = 5000;
                    spC = 100;
                    sp8 = 2000;
                    break;
                case 1:
                    sp14 = 1000;
                    sp10 = 1000;
                    spC = 100;
                    sp8 = 2000;
                    break;
            }
            break;
    }
    D_80358030[0] = (Gfx *)D_80358070;
    D_80358038[0] = (Gfx *)(D_80358070 += sp14 * sizeof(Gfx));
    D_80358040[0] = (Gfx *)(D_80358070 += sp10 * sizeof(Gfx));
    D_80358048[0] = (Gfx *)(D_80358070 += spC * sizeof(Gfx));
    D_80358030[1] = (Gfx *)(D_80358070 += sp8 * sizeof(Gfx));
    D_80358038[1] = (Gfx *)(D_80358070 += sp14 * sizeof(Gfx));
    D_80358040[1] = (Gfx *)(D_80358070 += sp10 * sizeof(Gfx));
    D_80358048[1] = (Gfx *)(D_80358070 += spC * sizeof(Gfx));
    D_80358070 += sp8 * sizeof(Gfx);
}

void func_80257490(s32 *arg0, s32 arg1) {
    s32 sp4 = *arg0 % arg1;

    if (sp4 != 0) {
        sp4 = arg1 - sp4;
    }
    *arg0 += sp4;
}

f32 func_802574F0(f32 arg0) {
    return sinf(arg0);
}

f32 func_80257514(f32 arg0) {
    return fcos(arg0);
}
