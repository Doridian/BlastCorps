#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include "game/types.h"
#include "game/sched.h"

/*
 * Audio.  hd_code 22EE0.c is the audio manager (audio.c, after SGI's
 * sample audiomgr.c: its asserts and the AM* layouts), 17E10.c and 1A630.c
 * Rare's sound-effect player (derived from libaudio's sndplayer.c), 1C460.c
 * the setup; the rest of the game calls them (func_80260650 plays a sound).  The game's libaudio is older than 2.0I's headers,
 * so a few of its structures differ from PR/libaudio.h and are defined here.
 */

/* The sample audio manager's AudioInfo: one per output buffer. */
typedef struct AudioInfo {
    /* 0x00 */ s16 *data;
    /* 0x04 */ s16 frameSamples;
    /* 0x08 */ SchedTask task;
} AudioInfo;
SIZE_CHECK_C(AudioInfo, 0x68);

/* The sample audio manager's AMAudioMgr. */
typedef struct AMAudioMgr {
    /* 0x000 */ Acmd *PTR32 ACMDList[2];
    /* 0x008 */ AudioInfo *audioInfo[3];
    /* 0x018 */ OSThread thread;
    /* 0x1C8 */ OSMesgQueue audioFrameMsgQ;
    /* 0x1E0 */ OSMesg audioFrameMsgBuf[8];
    /* 0x200 */ OSMesgQueue audioReplyMsgQ;
    /* 0x218 */ OSMesg audioReplyMsgBuf[8];
    /* 0x238 */ ALGlobals g;
} AMAudioMgr;

typedef struct AMDMABuffer {
    /* 0x00 */ ALLink node;
    /* 0x08 */ u32 startAddr;
    /* 0x0C */ u32 lastFrame;
    /* 0x10 */ char *ptr;
} AMDMABuffer;
SIZE_CHECK_C(AMDMABuffer, 0x14);

typedef struct AMDMAState {
    /* 0x0 */ u8 initialized;
    /* 0x4 */ AMDMABuffer *firstUsed;
    /* 0x8 */ AMDMABuffer *firstFree;
} AMDMAState;
SIZE_CHECK_C(AMDMAState, 0xC);

/* OSIoMesg from before 2.0I added piHandle. */
typedef struct IoMesg {
    /* 0x00 */ OSIoMesgHdr hdr;
    /* 0x08 */ void *dramAddr;
    /* 0x0C */ u32 devAddr;
    /* 0x10 */ u32 size;
} IoMesg;
SIZE_CHECK_C(IoMesg, 0x14);

/* ALSynConfig, with a u8 fxType. */
typedef struct SynConfig {
    /* 0x00 */ s32 maxVVoices;
    /* 0x04 */ s32 maxPVoices;
    /* 0x08 */ s32 maxUpdates;
    /* 0x0C */ s32 maxFXbusses;
    /* 0x10 */ ALDMANew dmaproc;
    /* 0x14 */ ALHeap *heap;
    /* 0x18 */ s32 outputRate;
    /* 0x1C */ u8 fxType;
    /* 0x20 */ s32 *params;
} SynConfig;
SIZE_CHECK_C(SynConfig, 0x24);
#ifdef TARGET_PC
/* (it is handed to alInit as one) */
_Static_assert(sizeof(SynConfig) == sizeof(ALSynConfig) &&
               __builtin_offsetof(SynConfig, fxType) == __builtin_offsetof(ALSynConfig, fxType) &&
               __builtin_offsetof(SynConfig, params) == __builtin_offsetof(ALSynConfig, params),
               "SynConfig is an ALSynConfig");
#endif

/* The custom reverb's parameter block (ALFxId AL_FX_CUSTOM). */
typedef struct FxParams {
    /* 0x000 */ s32 v[66];
} FxParams;
SIZE_CHECK(FxParams, 0x108);

/*
 * The sound bank, as the game's libaudio lays it out: ALBank, and an
 * ALInstrument from before bendRange/soundCount moved soundArray to 0x10.
 * Both are loaded from ROM; the pointers are offsets in the file until the
 * bank loader (_bnkfPatchBank) adds the file's address.
 */
typedef struct SndInstrument {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ ROMPTR(ALSound *) soundArray[1];
} SndInstrument;

typedef struct SndBank {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ ROMPTR(SndInstrument *) inst; /* ALBank.instArray[0] */
} SndBank;

/* The sound effects' bank (hd_code 1C460.c: sfxBankFile->bankArray[0]); the
 * sound calls pass it to func_80260650(bank, id, handle). */
extern SndBank *D_80367738;

/* Rare's sound player (libaudio's ALSoundState, ALSndpEvent, ALSndpConfig,
 * ALSndPlayer, with changes). */
typedef struct SndState {
    /* 0x00 */ ALLink node;
    /* 0x08 */ ALSound *sound;
    /* 0x0C */ ALVoice voice;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 pitch;
    /* 0x30 */ struct SndState **unk30;  /* its handle (func_80260650): cleared when it ends */
    /* 0x34 */ s16 unk34;
    /* 0x36 */ u8 unk36;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ u8 unk3C;
    /* 0x3D */ u8 unk3D;
    /* 0x3E */ u8 unk3E;
    /* 0x3F */ u8 unk3F;
} SndState;
SIZE_CHECK_C(SndState, 0x40);

typedef struct SndEvent {
    /* 0x00 */ u16 type;
    /* 0x04 */ SndState *state;
    /* 0x08 */ s32 param;
    /* 0x0C */ void *unkC;
} SndEvent; /* an ALEvent */
SIZE_CHECK_C(SndEvent, 0x10);
#ifdef TARGET_PC
/* alEvtqPostEvent copies an ALEvent from it, and the sound player reads its
   queue's ALEvents as SndEvents: the payload where ALEvent's union is */
_Static_assert(sizeof(SndEvent) == sizeof(ALEvent) &&
               __builtin_offsetof(SndEvent, state) == __builtin_offsetof(ALEvent, msg),
               "SndEvent is an ALEvent");
#endif

typedef struct SndConfig {
    /* 0x00 */ u32 maxSounds;
    /* 0x04 */ s32 maxEvents;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ ALHeap *heap;
    /* 0x10 */ u16 unk10;
} SndConfig;

typedef struct SndPlayer {
    /* 0x00 */ ALPlayer node;
    /* 0x14 */ ALEventQueue evtq;
    /* 0x28 */ ALEvent nextEvent;
    /* 0x38 */ ALSynth *drvr;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ SndState *unk40;
    /* 0x44 */ SndState *unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ ALMicroTime frameTime;
    /* 0x50 */ ALMicroTime nextDelta;
    /* 0x54 */ ALMicroTime curTime;
} SndPlayer;

/* D_802E8CE0: the sound states in use and the free ones. */
typedef struct SndStateLists {
    /* 0x0 */ SndState *head;
    /* 0x4 */ SndState *tail;
    /* 0x8 */ SndState *freeList;
} SndStateLists;

/* The audio heap (D_80370C80, 46F60.c's; 1C460's alHeapInit) and the
 * number of audio DMA buffers and messages (22EE0.c): eu's PAL frames are
 * longer and take more DMAs, and its heap is smaller. */
#ifdef VERSION_EU
#define AUDIO_HEAP_N64 0x24540
#define NUM_DMA_MESSAGES 0x5C
#else
#define AUDIO_HEAP_N64 0x2A280
#define NUM_DMA_MESSAGES 0x48
#endif
/* The heap is exactly full on the N64.  In the port, what is allocated in
 * it with pointers in it is as big as the host makes it, and the heap grows
 * by that (port/include/libaudio_heap.h; nothing where pointers are 4
 * bytes; alHeapAlloc stops the port where it runs out): AudioInfo (22EE0.c,
 * three), the synthesizer's blocks (1C460.c's synConfig: 0x18 voices, 0x80
 * updates; 22EE0.c's reverb, D_802F3AFC: 8 sections, one with a chorus,
 * four with a low-pass filter), the sequence player and its blocks
 * (1C460.c's seqConfig: 0x10 channels, 0x18 voices, 0x20 events) and the
 * sound player's (sndConfig: 0x20 sounds, 0x40 events).  A heap that grew
 * isn't at its N64 address, which is 16-aligned, and alHeapInit takes the
 * rounding to 16 off a buffer that isn't: up to 15 bytes more then. */
#ifndef TARGET_PC
#define AUDIO_HEAP_SIZE AUDIO_HEAP_N64
#else
#include "libaudio_heap.h"
#define AUDIO_HEAP_GROWTH                                                              \
    (3 * AL_HEAP_GROWTH(1, AudioInfo, 0x68) + AL_SYN_HEAP_GROWTH(0x18, 0x80, 8, 1, 4) + \
     AL_HEAP_GROWTH(1, ALCSPlayer, 0x7C) + AL_CSP_HEAP_GROWTH(0x10, 0x18, 0x20) +     \
     AL_HEAP_GROWTH(0x20, SndState, 0x40) + AL_HEAP_GROWTH(0x40, ALEventListItem, 0x1C))
#define AUDIO_HEAP_SIZE (AUDIO_HEAP_N64 + AUDIO_HEAP_GROWTH + (AUDIO_HEAP_GROWTH != 0 ? 15 : 0))
#endif

extern SndState *D_8036DCD8; /* a sound hd_code 39050.c starts (func_80260650's handle) */

/* 1C460.c's: the sequence player and the frame count (D_803156C4) it last started at. */
extern ALCSPlayer *D_80367734;
extern s32 D_80367740;

#endif
