/*
 * The audio library's interface, as the port's own libaudio (port/libaudio)
 * implements it: the names, signatures and structure layouts the game's C
 * (hd_code 1A630.c, 1C460.c, 17E10.c, 1D990.c, 22EE0.c), the ROM's bank and
 * sequence files and the RSP command list rely on.
 *
 * Written for the port from the public description of the library (the N64
 * programming manual's audio chapters and function reference) and from what
 * the game uses; every layout here is checked against the game's own by the
 * build (the offsets in comments) and by the libaudio oracle
 * (port/tools/audio_oracle).  Pointers in shared structures are 32 bits in
 * every build (PTR32: the N64's layout, also in the LP64 port).
 */
#ifndef PORT_SDK_LIBAUDIO_H
#define PORT_SDK_LIBAUDIO_H
#define __LIB_AUDIO__       /* (and not the SDK's) */

#include <PR/ultratypes.h>
#include <PR/mbi.h>

#ifdef _LANGUAGE_C_PLUS_PLUS
extern "C" {
#endif

#ifndef NULL
#define NULL 0
#endif

/* ---- basics --------------------------------------------------------------- */

typedef s32 ALMicroTime;        /* microseconds */
typedef u8 ALPan;               /* 0 left, 64 centre, 127 right */

#define AL_FX_BUFFER_SIZE 8192
#define AL_FRAME_INIT -1
#define AL_USEC_PER_FRAME 16000 /* a sequence player's idle tick */
#define AL_MAX_PRIORITY 127
#define AL_GAIN_CHANGE_TIME 1000

#define AL_PAN_CENTER 64
#define AL_PAN_LEFT 0
#define AL_PAN_RIGHT 127
#define AL_VOL_FULL 127
#define AL_KEY_MIN 0
#define AL_KEY_MAX 127
#define AL_DEFAULT_FXMIX 0
#define AL_SUSTAIN 63

/* (the release library's error checks: give up quietly) */
#define ALFailIf(condition, error) \
    if (condition) {               \
        return;                    \
    }
#define ALFlagFailIf(condition, flag, error) \
    if (condition) {                         \
        return;                              \
    }

/* a doubly linked list's node; a list is a node whose next is the head */
typedef struct ALLink_s {
    struct ALLink_s *PTR32 next;
    struct ALLink_s *PTR32 prev;
} ALLink;

void alUnlink(ALLink *element);
void alLink(ALLink *element, ALLink *after);

/* sample data is fetched through the game's routine: given a ROM address
   and a length, it returns an RDRAM physical address holding them */
typedef s32 (*PTR32 ALDMAproc)(s32 addr, s32 len, void *state);
typedef ALDMAproc (*PTR32 ALDMANew)(void *state);

void alCopy(void *src, void *dest, s32 len);

/* ---- the heap: a bump allocator, 16-byte blocks ----------------------------- */

typedef struct {
    u8 *PTR32 base;
    u8 *PTR32 cur;
    s32 len;
    s32 count;
} ALHeap;

#define AL_HEAP_DEBUG 1
#define AL_HEAP_MAGIC 0x20736a73
#define AL_HEAP_INIT 0

void alHeapInit(ALHeap *hp, u8 *base, s32 len);
void *alHeapDBAlloc(u8 *file, s32 line, ALHeap *hp, s32 num, s32 size);
s32 alHeapCheck(ALHeap *hp);
#define alHeapAlloc(hp, elem, size) alHeapDBAlloc(0, 0, (hp), (elem), (size))

/* ---- effects ------------------------------------------------------------------ */

#define AL_FX_NONE 0
#define AL_FX_SMALLROOM 1
#define AL_FX_BIGROOM 2
#define AL_FX_CHORUS 3
#define AL_FX_FLANGE 4
#define AL_FX_ECHO 5
#define AL_FX_CUSTOM 6

typedef u8 ALFxId;
typedef void *ALFxRef;

/* ---- bank files (.ctl): offsets in the file until alBnkfNew ------------------- */

#define AL_BANK_VERSION 0x4231  /* "B1" */

enum { AL_ADPCM_WAVE = 0, AL_RAW16_WAVE };

typedef struct {
    s32 order;
    s32 npredictors;
    s16 book[1];                /* order * npredictors * 8, 8-byte aligned */
} ALADPCMBook;

typedef struct {
    u32 start;
    u32 end;
    u32 count;                  /* -1: for ever */
    ADPCM_STATE state;          /* the decoder's state at the loop's start */
} ALADPCMloop;

typedef struct {
    u32 start;
    u32 end;
    u32 count;
} ALRawLoop;

typedef struct {
    ALMicroTime attackTime;
    ALMicroTime decayTime;
    ALMicroTime releaseTime;
    u8 attackVolume;
    u8 decayVolume;
} ALEnvelope;

typedef struct {
    u8 velocityMin;
    u8 velocityMax;
    u8 keyMin;
    u8 keyMax;
    u8 keyBase;
    s8 detune;                  /* cents */
} ALKeyMap;

typedef struct {
    ALADPCMloop *PTR32 loop;
    ALADPCMBook *PTR32 book;
} ALADPCMWaveInfo;

typedef struct {
    ALRawLoop *PTR32 loop;
} ALRAWWaveInfo;

typedef struct ALWaveTable_s {
    u8 *PTR32 base;             /* the samples' ROM address */
    s32 len;                    /* bytes */
    u8 type;                    /* AL_ADPCM_WAVE, AL_RAW16_WAVE */
    u8 flags;                   /* set once relocated */
    union {
        ALADPCMWaveInfo adpcmWave;
        ALRAWWaveInfo rawWave;
    } waveInfo;
} ALWaveTable;

typedef struct ALSound_s {
    ALEnvelope *PTR32 envelope;
    ALKeyMap *PTR32 keyMap;
    ALWaveTable *PTR32 wavetable;
    ALPan samplePan;
    u8 sampleVolume;
    u8 flags;
} ALSound;

typedef struct {
    u8 volume;
    ALPan pan;
    u8 priority;
    u8 flags;
    u8 tremType;
    u8 tremRate;
    u8 tremDepth;
    u8 tremDelay;
    u8 vibType;
    u8 vibRate;
    u8 vibDepth;
    u8 vibDelay;
    s16 bendRange;              /* cents */
    s16 soundCount;
    ALSound *PTR32 soundArray[1];   /* by key range, then velocity range */
} ALInstrument;

typedef struct ALBank_s {
    s16 instCount;
    u8 flags;
    u8 pad;
    s32 sampleRate;
    ALInstrument *PTR32 percussion;
    ALInstrument *PTR32 instArray[1];
} ALBank;

typedef struct {
    s16 revision;               /* AL_BANK_VERSION */
    s16 bankCount;
    ALBank *PTR32 bankArray[1];
} ALBankFile;

void alBnkfNew(ALBankFile *f, u8 *table);

/* ---- sequence bank files (.sbk) ------------------------------------------------- */

#define AL_SEQBANK_VERSION 'S1'

typedef struct {
    u8 *PTR32 offset;
    s32 len;
} ALSeqData;

typedef struct {
    s16 revision;
    s16 seqCount;
    ALSeqData seqArray[1];
} ALSeqFile;

void alSeqFileNew(ALSeqFile *f, u8 *base);

/* ---- the synthesizer --------------------------------------------------------- */

typedef ALMicroTime (*PTR32 ALVoiceHandler)(void *);

typedef struct {
    s32 maxVVoices;             /* (unused) */
    s32 maxPVoices;
    s32 maxUpdates;
    s32 maxFXbusses;
    void *PTR32 dmaproc;        /* an ALDMANew */
    ALHeap *PTR32 heap;
    s32 outputRate;
    ALFxId fxType;
    s32 *PTR32 params;          /* AL_FX_CUSTOM's */
} ALSynConfig;

/* a client of the synthesizer: its handler runs when its time comes, and
   returns the microseconds to its next call */
typedef struct ALPlayer_s {
    struct ALPlayer_s *PTR32 next;
    void *PTR32 clientData;
    ALVoiceHandler handler;
    ALMicroTime callTime;
    s32 samplesLeft;            /* the output sample of its next call */
} ALPlayer;

/* a voice as its client holds it; pvoice is the synthesizer's */
typedef struct ALVoice_s {
    ALLink node;
    struct PVoice_s *PTR32 pvoice;
    ALWaveTable *PTR32 table;
    void *PTR32 clientPrivate;
    s16 state;
    s16 priority;
    s16 fxBus;
    s16 unityPitch;
} ALVoice;

typedef struct ALVoiceConfig_s {
    s16 priority;
    s16 fxBus;
    u8 unityPitch;
} ALVoiceConfig;

typedef struct {
    ALPlayer *PTR32 head;
    ALLink pFreeList;           /* physical voices: free, */
    ALLink pAllocList;          /* in use, */
    ALLink pLameList;           /* released this frame */
    s32 paramSamples;           /* the sample time a client's changes take effect */
    s32 curSamples;             /* samples made so far */
    ALDMANew dma;
    ALHeap *PTR32 heap;
    struct ALParam_s *PTR32 paramList;
    struct ALMainBus_s *PTR32 mainBus;
    struct ALAuxBus_s *PTR32 auxBus;
    struct ALFilter_s *PTR32 outputFilter;
    s32 numPVoices;
    s32 maxAuxBusses;
    s32 outputRate;
    s32 maxOutSamples;          /* per command list section */
} ALSynth;

void alSynNew(ALSynth *s, ALSynConfig *config);
void alSynDelete(ALSynth *s);
void alSynAddPlayer(ALSynth *s, ALPlayer *client);
void alSynRemovePlayer(ALSynth *s, ALPlayer *client);
s32 alSynAllocVoice(ALSynth *s, ALVoice *v, ALVoiceConfig *vc);
void alSynFreeVoice(ALSynth *s, ALVoice *voice);
void alSynStartVoice(ALSynth *s, ALVoice *voice, ALWaveTable *w);
void alSynStartVoiceParams(ALSynth *s, ALVoice *voice, ALWaveTable *w, f32 pitch, s16 vol, ALPan pan,
                           u8 fxmix, ALMicroTime t);
void alSynStopVoice(ALSynth *s, ALVoice *voice);
void alSynSetVol(ALSynth *s, ALVoice *v, s16 vol, ALMicroTime delta);
void alSynSetPitch(ALSynth *s, ALVoice *voice, f32 ratio);
void alSynSetPan(ALSynth *s, ALVoice *voice, ALPan pan);
void alSynSetFXMix(ALSynth *s, ALVoice *voice, u8 fxmix);
void alSynSetPriority(ALSynth *s, ALVoice *voice, s16 priority);
s16 alSynGetPriority(ALSynth *s, ALVoice *voice);
ALFxRef *alSynAllocFX(ALSynth *s, s16 bus, ALSynConfig *c, ALHeap *hp);
ALFxRef alSynGetFXRef(ALSynth *s, s16 bus, s16 index);
void alSynFreeFX(ALSynth *s, ALFxRef *fx);
void alSynSetFXParam(ALSynth *s, ALFxRef fx, s16 paramID, void *param);

typedef struct {
    ALSynth drvr;
} ALGlobals;

extern ALGlobals *alGlobals;

void alInit(ALGlobals *glob, ALSynConfig *c);
void alClose(ALGlobals *glob);
Acmd *alAudioFrame(Acmd *cmdList, s32 *cmdLen, s16 *outBuf, s32 outLen);

/* ---- players' states and events -------------------------------------------------- */

#define AL_STOPPED 0
#define AL_PLAYING 1
#define AL_STOPPING 2

#define AL_DEFAULT_PRIORITY 5
#define AL_DEFAULT_VOICE 0
#define AL_MAX_CHANNELS 16

enum ALMsg {
    AL_SEQ_REF_EVT,             /* the sequence's next event is due */
    AL_SEQ_MIDI_EVT,
    AL_SEQP_MIDI_EVT,
    AL_TEMPO_EVT,
    AL_SEQ_END_EVT,
    AL_NOTE_END_EVT,
    AL_SEQP_ENV_EVT,
    AL_SEQP_META_EVT,
    AL_SEQP_PROG_EVT,
    AL_SEQP_API_EVT,
    AL_SEQP_VOL_EVT,
    AL_SEQP_LOOP_EVT,
    AL_SEQP_PRIORITY_EVT,
    AL_SEQP_SEQ_EVT,
    AL_SEQP_BANK_EVT,
    AL_SEQP_PLAY_EVT,
    AL_SEQP_STOP_EVT,
    AL_SEQP_STOPPING_EVT,
    AL_TRACK_END,
    AL_CSP_LOOPSTART,
    AL_CSP_LOOPEND,
    AL_CSP_NOTEOFF_EVT,
    AL_TREM_OSC_EVT,
    AL_VIB_OSC_EVT
};

#define AL_EVTQ_END 0x7fffffff

enum AL_MIDIstatus {
    AL_MIDI_ChannelMask = 0x0F,
    AL_MIDI_StatusMask = 0xF0,
    AL_MIDI_ChannelVoice = 0x80,
    AL_MIDI_NoteOff = 0x80,
    AL_MIDI_NoteOn = 0x90,
    AL_MIDI_PolyKeyPressure = 0xA0,
    AL_MIDI_ControlChange = 0xB0,
    AL_MIDI_ChannelModeSelect = 0xB0,
    AL_MIDI_ProgramChange = 0xC0,
    AL_MIDI_ChannelPressure = 0xD0,
    AL_MIDI_PitchBendChange = 0xE0,
    AL_MIDI_SysEx = 0xF0,
    AL_MIDI_SystemCommon = 0xF1,
    AL_MIDI_TimeCodeQuarterFrame = 0xF1,
    AL_MIDI_SongPositionPointer = 0xF2,
    AL_MIDI_SongSelect = 0xF3,
    AL_MIDI_Undefined1 = 0xF4,
    AL_MIDI_Undefined2 = 0xF5,
    AL_MIDI_TuneRequest = 0xF6,
    AL_MIDI_EOX = 0xF7,
    AL_MIDI_SystemRealTime = 0xF8,
    AL_MIDI_TimingClock = 0xF8,
    AL_MIDI_Undefined3 = 0xF9,
    AL_MIDI_Start = 0xFA,
    AL_MIDI_Continue = 0xFB,
    AL_MIDI_Stop = 0xFC,
    AL_MIDI_Undefined4 = 0xFD,
    AL_MIDI_ActiveSensing = 0xFE,
    AL_MIDI_SystemReset = 0xFF,
    AL_MIDI_Meta = 0xFF
};

enum AL_MIDIctrl {
    AL_MIDI_VOLUME_CTRL = 0x07,
    AL_MIDI_PAN_CTRL = 0x0A,
    AL_MIDI_PRIORITY_CTRL = 0x10,
    AL_MIDI_FX_CTRL_0 = 0x14,
    AL_MIDI_FX_CTRL_1 = 0x15,
    AL_MIDI_FX_CTRL_2 = 0x16,
    AL_MIDI_FX_CTRL_3 = 0x17,
    AL_MIDI_FX_CTRL_4 = 0x18,
    AL_MIDI_FX_CTRL_5 = 0x19,
    AL_MIDI_FX_CTRL_6 = 0x1A,
    AL_MIDI_FX_CTRL_7 = 0x1B,
    AL_MIDI_FX_CTRL_8 = 0x1C,
    AL_MIDI_FX_CTRL_9 = 0x1D,
    AL_MIDI_SUSTAIN_CTRL = 0x40,
    AL_MIDI_FX1_CTRL = 0x5B,
    AL_MIDI_FX3_CTRL = 0x5D
};

enum AL_MIDImeta {
    AL_MIDI_META_TEMPO = 0x51,
    AL_MIDI_META_EOT = 0x2f
};

/* compact MIDI's own codes */
#define AL_CMIDI_BLOCK_CODE 0xFE        /* a repeat of earlier bytes */
#define AL_CMIDI_LOOPSTART_CODE 0x2E
#define AL_CMIDI_LOOPEND_CODE 0x2D
#define AL_CMIDI_CNTRL_LOOPSTART 102
#define AL_CMIDI_CNTRL_LOOPEND 103
#define AL_CMIDI_CNTRL_LOOPCOUNT_SM 104
#define AL_CMIDI_CNTRL_LOOPCOUNT_BIG 105

typedef struct {
    u8 *PTR32 curPtr;
    s32 lastTicks;
    s32 curTicks;
    s16 lastStatus;
} ALSeqMarker;

typedef struct {
    s32 ticks;
    u8 status;
    u8 byte1;
    u8 byte2;
    u32 duration;               /* a compact note on's length in ticks */
} ALMIDIEvent;

typedef struct {
    s32 ticks;
    u8 status;
    u8 type;
    u8 len;
    u8 byte1;
    u8 byte2;
    u8 byte3;
} ALTempoEvent;

typedef struct {
    s32 ticks;
    u8 status;
    u8 type;
    u8 len;
} ALEndEvent;

typedef struct {
    struct ALVoice_s *PTR32 voice;
} ALNoteEvent;

typedef struct {
    struct ALVoice_s *PTR32 voice;
    ALMicroTime delta;
    u8 vol;
} ALVolumeEvent;

typedef struct {
    s16 vol;
} ALSeqpVolEvent;

typedef struct {
    ALSeqMarker *PTR32 start;
    ALSeqMarker *PTR32 end;
    s32 count;
} ALSeqpLoopEvent;

typedef struct {
    u8 chan;
    u8 priority;
} ALSeqpPriorityEvent;

typedef struct {
    void *PTR32 seq;            /* an ALSeq or an ALCSeq */
} ALSeqpSeqEvent;

typedef struct {
    ALBank *PTR32 bank;
} ALSeqpBankEvent;

typedef struct {
    struct ALVoiceState_s *PTR32 vs;
    void *PTR32 oscState;
    u8 chan;
} ALOscEvent;

typedef struct {
    s16 type;                   /* enum ALMsg (or a client's own) */
    union {
        ALMIDIEvent midi;
        ALTempoEvent tempo;
        ALEndEvent end;
        ALNoteEvent note;
        ALVolumeEvent vol;
        ALSeqpLoopEvent loop;
        ALSeqpVolEvent spvol;
        ALSeqpPriorityEvent sppriority;
        ALSeqpSeqEvent spseq;
        ALSeqpBankEvent spbank;
        ALOscEvent osc;
    } msg;
} ALEvent;

/* a queue of events by time: each item's delta is from the one before */
typedef struct {
    ALLink node;
    ALMicroTime delta;
    ALEvent evt;
} ALEventListItem;

typedef struct {
    ALLink freeList;
    ALLink allocList;
    s32 eventCount;
} ALEventQueue;

void alEvtqNew(ALEventQueue *evtq, ALEventListItem *items, s32 itemCount);
ALMicroTime alEvtqNextEvent(ALEventQueue *evtq, ALEvent *evt);
void alEvtqPostEvent(ALEventQueue *evtq, ALEvent *evt, ALMicroTime delta);
void alEvtqFlush(ALEventQueue *evtq);
void alEvtqFlushType(ALEventQueue *evtq, s16 type);

/* ---- sequence players ---------------------------------------------------------------- */

#define AL_PHASE_ATTACK 0
#define AL_PHASE_NOTEON 0
#define AL_PHASE_DECAY 1
#define AL_PHASE_SUSTAIN 2
#define AL_PHASE_RELEASE 3
#define AL_PHASE_SUSTREL 4

/* a sounding note */
typedef struct ALVoiceState_s {
    struct ALVoiceState_s *PTR32 next;
    ALVoice voice;
    ALSound *PTR32 sound;
    ALMicroTime envEndTime;
    f32 pitch;
    f32 vibrato;
    u8 envGain;
    u8 channel;
    u8 key;
    u8 velocity;
    u8 envPhase;
    u8 phase;
    u8 tremelo;
    u8 flags;                   /* 1: a tremolo oscillator, 2: a vibrato one */
} ALVoiceState;

/* a MIDI channel's state (the game copies these 16 bytes as four words) */
typedef struct {
    ALInstrument *PTR32 instrument;
    s16 bendRange;
    ALFxId fxId;
    ALPan pan;
    u8 priority;
    u8 vol;
    u8 fxmix;
    u8 sustain;
    f32 pitchBend;              /* a ratio */
} ALChanState;

typedef struct ALSeq_s {
    u8 *PTR32 base;
    u8 *PTR32 trackStart;
    u8 *PTR32 curPtr;
    s32 lastTicks;
    s32 len;
    f32 qnpt;
    s16 division;
    s16 lastStatus;
} ALSeq;

/* a compact MIDI sequence's header: each track's offset (0: none) */
typedef struct {
    u32 trackOffset[16];
    u32 division;               /* ticks per quarter note */
} ALCMidiHdr;

typedef struct ALCSeq_s {
    ALCMidiHdr *PTR32 base;
    u32 validTracks;            /* a bit per track still going */
    f32 qnpt;                   /* quarter notes per tick */
    u32 lastTicks;              /* the tick of the last event read */
    u32 lastDeltaTicks;
    u32 deltaFlag;              /* the last delta isn't taken off the tracks' yet */
    u8 *PTR32 curLoc[16];
    u8 *PTR32 curBUPtr[16];     /* in a repeat: where it reads */
    u8 curBULen[16];            /* ... and how much is left */
    u8 lastStatus[16];          /* running status */
    u32 evtDeltaTicks[16];      /* each track's next event, in ticks from the last */
} ALCSeq;

typedef struct {
    u32 validTracks;
    s32 lastTicks;
    u32 lastDeltaTicks;
    u8 *PTR32 curLoc[16];
    u8 *PTR32 curBUPtr[16];
    u8 curBULen[16];
    u8 lastStatus[16];
    u32 evtDeltaTicks[16];
} ALCSeqMarker;

#define NO_SOUND_ERR_MASK 0x01
#define NOTE_OFF_ERR_MASK 0x02
#define NO_VOICE_ERR_MASK 0x04

typedef struct {
    s32 maxVoices;
    s32 maxEvents;
    u8 maxChannels;
    u8 debugFlags;
    ALHeap *PTR32 heap;
    void *PTR32 initOsc;
    void *PTR32 updateOsc;
    void *PTR32 stopOsc;
} ALSeqpConfig;

typedef ALMicroTime (*PTR32 ALOscInit)(void **oscState, f32 *initVal, u8 oscType, u8 oscRate,
                                      u8 oscDepth, u8 oscDelay);
typedef ALMicroTime (*PTR32 ALOscUpdate)(void *oscState, f32 *updateVal);
typedef void (*PTR32 ALOscStop)(void *oscState);

typedef struct {
    ALPlayer node;              /* first */
    ALSynth *PTR32 drvr;
    ALSeq *PTR32 target;
    ALMicroTime curTime;
    ALBank *PTR32 bank;
    s32 uspt;                   /* microseconds per tick */
    s32 nextDelta;
    s32 state;
    u16 chanMask;
    s16 vol;
    u8 maxChannels;
    u8 debugFlags;
    ALEvent nextEvent;
    ALEventQueue evtq;
    ALMicroTime frameTime;
    ALChanState *PTR32 chanState;
    ALVoiceState *PTR32 vAllocHead;
    ALVoiceState *PTR32 vAllocTail;
    ALVoiceState *PTR32 vFreeList;
    ALOscInit initOsc;
    ALOscUpdate updateOsc;
    ALOscStop stopOsc;
    ALSeqMarker *PTR32 loopStart;
    ALSeqMarker *PTR32 loopEnd;
    s32 loopCount;
} ALSeqPlayer;

/* the compact sequence player: ALSeqPlayer's layout up to stopOsc */
typedef struct {
    ALPlayer node;              /* 0x00 */
    ALSynth *PTR32 drvr;        /* 0x14 */
    ALCSeq *PTR32 target;       /* 0x18 */
    ALMicroTime curTime;        /* 0x1C */
    ALBank *PTR32 bank;         /* 0x20 */
    s32 uspt;                   /* 0x24 */
    s32 nextDelta;              /* 0x28 */
    s32 state;                  /* 0x2C */
    u16 chanMask;               /* 0x30 */
    s16 vol;                    /* 0x32 */
    u8 maxChannels;             /* 0x34 */
    u8 debugFlags;              /* 0x35 */
    ALEvent nextEvent;          /* 0x38 */
    ALEventQueue evtq;          /* 0x48 */
    ALMicroTime frameTime;      /* 0x5C */
    ALChanState *PTR32 chanState;   /* 0x60 */
    ALVoiceState *PTR32 vAllocHead; /* 0x64 */
    ALVoiceState *PTR32 vAllocTail; /* 0x68 */
    ALVoiceState *PTR32 vFreeList;  /* 0x6C */
    ALOscInit initOsc;          /* 0x70 */
    ALOscUpdate updateOsc;      /* 0x74 */
    ALOscStop stopOsc;          /* 0x78 */
} ALCSPlayer;

void alSeqNew(ALSeq *seq, u8 *ptr, s32 len);
void alSeqNextEvent(ALSeq *seq, ALEvent *event);
s32 alSeqGetTicks(ALSeq *seq);
f32 alSeqTicksToSec(ALSeq *seq, s32 ticks, u32 tempo);
u32 alSeqSecToTicks(ALSeq *seq, f32 sec, u32 tempo);
void alSeqNewMarker(ALSeq *seq, ALSeqMarker *m, u32 ticks);
void alSeqSetLoc(ALSeq *seq, ALSeqMarker *marker);
void alSeqGetLoc(ALSeq *seq, ALSeqMarker *marker);

void alCSeqNew(ALCSeq *seq, u8 *ptr);
void alCSeqNextEvent(ALCSeq *seq, ALEvent *evt);
s32 alCSeqGetTicks(ALCSeq *seq);
f32 alCSeqTicksToSec(ALCSeq *seq, s32 ticks, u32 tempo);
u32 alCSeqSecToTicks(ALCSeq *seq, f32 sec, u32 tempo);
void alCSeqNewMarker(ALCSeq *seq, ALCSeqMarker *m, u32 ticks);
void alCSeqSetLoc(ALCSeq *seq, ALCSeqMarker *marker);
void alCSeqGetLoc(ALCSeq *seq, ALCSeqMarker *marker);

f32 alCents2Ratio(s32 cents);

void alSeqpNew(ALSeqPlayer *seqp, ALSeqpConfig *config);
void alSeqpDelete(ALSeqPlayer *seqp);
void alSeqpSetSeq(ALSeqPlayer *seqp, ALSeq *seq);
ALSeq *alSeqpGetSeq(ALSeqPlayer *seqp);
void alSeqpPlay(ALSeqPlayer *seqp);
void alSeqpStop(ALSeqPlayer *seqp);
s32 alSeqpGetState(ALSeqPlayer *seqp);
void alSeqpSetBank(ALSeqPlayer *seqp, ALBank *b);
void alSeqpSetTempo(ALSeqPlayer *seqp, s32 tempo);
s32 alSeqpGetTempo(ALSeqPlayer *seqp);
s16 alSeqpGetVol(ALSeqPlayer *seqp);
void alSeqpSetVol(ALSeqPlayer *seqp, s16 vol);
void alSeqpLoop(ALSeqPlayer *seqp, ALSeqMarker *start, ALSeqMarker *end, s32 count);

void alCSPNew(ALCSPlayer *seqp, ALSeqpConfig *config);
void alCSPDelete(ALCSPlayer *seqp);
void alCSPSetSeq(ALCSPlayer *seqp, ALCSeq *seq);
ALCSeq *alCSPGetSeq(ALCSPlayer *seqp);
void alCSPPlay(ALCSPlayer *seqp);
void alCSPStop(ALCSPlayer *seqp);
s32 alCSPGetState(ALCSPlayer *seqp);
void alCSPSetBank(ALCSPlayer *seqp, ALBank *b);
void alCSPSetTempo(ALCSPlayer *seqp, s32 tempo);
s32 alCSPGetTempo(ALCSPlayer *seqp);
s16 alCSPGetVol(ALCSPlayer *seqp);
void alCSPSetVol(ALCSPlayer *seqp, s16 vol);

/* ---- the sound player (the game has its own, 1A630.c) --------------------------------- */

typedef struct {
    s32 maxSounds;
    s32 maxEvents;
    ALHeap *PTR32 heap;
} ALSndpConfig;

typedef struct {
    ALPlayer node;
    ALEventQueue evtq;
    ALEvent nextEvent;
    ALSynth *PTR32 drvr;
    s32 target;
    void *PTR32 sndState;
    s32 maxSounds;
    ALMicroTime frameTime;
    ALMicroTime nextDelta;
    ALMicroTime curTime;
} ALSndPlayer;

typedef s16 ALSndId;

#ifdef _LANGUAGE_C_PLUS_PLUS
}
#endif

#endif
