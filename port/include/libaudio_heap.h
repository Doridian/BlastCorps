/*
 * The port's libaudio's blocks in the audio heap (docs/PORT.md, "libaudio"):
 * the sizes the reference allocates them at, and the structures this code
 * keeps in them.  port/libaudio's (audio_private.h includes it), and the
 * port's game/audio.h's, whose AUDIO_HEAP_SIZE grows by what the blocks
 * grow by where pointers are wider than 4 bytes (AL_SYN_HEAP_GROWTH,
 * AL_CSP_HEAP_GROWTH: 0 where they are 4).
 *
 * The heap's layout is the reference's where pointers are 4 bytes: the
 * RSP's state blocks are allocated in it and their addresses are in the
 * command list (the libaudio oracle compares them).  Where they are 8 a
 * structure can be bigger than its block was: the block is then the
 * structure (HEAP_BLOCK) and everything after it moves.
 */
#ifndef LIBAUDIO_HEAP_H
#define LIBAUDIO_HEAP_H

#include <PR/libaudio.h>

/* the heap blocks as the reference allocates them (their sizes decide where
   everything after them is, and the RSP's state addresses are in the
   command list) */
#define BLOCK_PTR 4             /* an array of pointers' element */
#define BLOCK_SAVE 28
#define BLOCK_AUXBUS 76
#define BLOCK_MAINBUS 32
#define BLOCK_DELAY 40          /* per reverb section */
#define BLOCK_RESAMPLER 52      /* a reverb section's modulation */
#define BLOCK_LOWPASS 48        /* a reverb section's filter */
#define BLOCK_VOICE 220
#define BLOCK_UPDATE 28
#define BLOCK_CHANSTATE 16
#define BLOCK_VOICESTATE 56
#define BLOCK_EVENT 28

/* what the library allocates for a T that was n64 bytes on the N64: that,
   or the structure where it's bigger (an array's element: it is the
   structure, which is n64 bytes exactly where pointers are 4, asserted
   below) */
#define HEAP_BLOCK(T, n64) (sizeof(T) > (n64) ? (s32)sizeof(T) : (n64))
#define N64_BLOCK(T, n64) \
    _Static_assert(sizeof(void *) == 4 ? sizeof(T) == (n64) : sizeof(T) >= (n64), #T " is its N64 block")

/* how much more n blocks of T take than n of n64 bytes did (each
   alHeapAlloc is rounded up to 16) */
#define AL_HEAP_ROUND(n) (((n) + 15) & ~15)
#define AL_HEAP_GROWTH(n, T, n64) (AL_HEAP_ROUND((n) * HEAP_BLOCK(T, n64)) - AL_HEAP_ROUND((n) * (n64)))

/* ---- the updates, a voice's changes (synth.c, voice.c) ---- */

typedef struct Update {
    struct Update *next;
    s32 at;                     /* the sample it takes effect */
    s16 kind;
    s16 unity;                  /* start: play at the table's own rate */
    union {
        f32 f;
        s32 i;
        void *p;
    } a;                        /* pitch, volume, pan, send; the voice to free */
    union {
        s32 ramp;               /* volume: samples to reach it */
        struct {
            s16 volume;
            u8 pan;
            u8 fxmix;
        } s;                    /* start with parameters */
    } b;
    s32 ramp;                   /* start with parameters: the attack's samples */
    ALWaveTable *wave;
} Update;

/* ---- the physical voice --------------------------------------------------- */

/* (the reference's block is bigger than what this code keeps in it: the
   union makes each the block's size, for the voices' array) */
typedef struct PVoice_s {
  union {
   struct {
    ALLink node;                /* on one of the synthesizer's three lists */
    ALVoice *owner;
    s32 steal_delay;            /* stolen: the samples its ramp down takes, else 0 */

    /* the decoder */
    ADPCM_STATE *dec_state;
    ADPCM_STATE *loop_state;  /* the table's state at its loop start */
    u32 loop_start, loop_end, loop_count;   /* (unsigned, as the tables have them) */
    ALWaveTable *table;
    s32 book_bytes;
    s32 played;                 /* samples of the table handed out */
    s32 frac;                   /* decoded samples left over in the last frame */
    s32 dec_first;
    s32 rom;                    /* the next byte of the table to fetch */
    s32 unused0;
    ALDMAproc dma;
    void *dma_state;      /* (at 0x44: the game is given its address) */

    /* the resampler */
    RESAMPLE_STATE *rs_state;
    f32 ratio;
    s32 unity;
    f32 rs_frac;
    s32 rs_first;

    /* the envelope and mixer */
    ENVMIX_STATE *env_state;
    s16 pan;
    s16 volume;
    s16 cur_l, cur_r;           /* the ramp's starts */
    s16 dry, wet;
    u16 rate_l_frac;
    s16 rate_l;
    s16 target_l;
    u16 rate_r_frac;
    s16 rate_r;
    s16 target_r;
    s32 seg_pos;                /* samples into the ramp */
    s32 seg_len;
    s32 env_first;
    Update *updates;
    Update *updates_tail;
    s32 playing;
   };
    u8 n64_size[BLOCK_VOICE];
  };
} PVoice;

/* ---- the reverb ------------------------------------------------------------ */

typedef struct {
    s16 fc;
    s16 gain;
    s32 pad;
    s16 coef[16];               /* aLoadADPCM'd as a book: at 8 in its block */
    POLEF_STATE *state;
    s32 first;
} LowPass;

typedef struct {
    RESAMPLE_STATE *state;
    f32 frac;
    s32 first;
} Chorus;

typedef struct {
    u32 in, out;                /* taps, samples behind the write position */
    s16 ff, fb, gain;
    f32 mod_step;               /* the chorus: its sawtooth's rate, */
    f32 mod_phase;              /* ... where it is, */
    s32 drift;                  /* ... how far the read tap has moved */
    f32 mod_depth;
    LowPass *lp;
    Chorus *chorus;
} Section;

/* the synthesizer's own blocks: the output stage, the aux bus with the
   reverb, the main bus (only their addresses are seen, in ALSynth) */
typedef struct {
    s32 dram;                   /* this section's output */
} OutStage;

typedef struct {
    s16 *line;                  /* the delay line */
    s16 *pos;                   /* its write position */
    u32 len;
    Section *sections;
    u8 nsections;
    s32 on;
} Reverb;

typedef struct ALAuxBus_s {
    PVoice **voices;
    s32 nvoices;
    s32 maxvoices;
    Reverb fx;
} AuxBus;

typedef struct ALMainBus_s {
    s32 nsources;               /* 1: the reverb's or the aux bus's output */
    void **sources;
} MainBus;

/* the arrays' elements (the rest are one to a block, which is the N64's
   size or more: HEAP_BLOCK) */
N64_BLOCK(Update, BLOCK_UPDATE);
N64_BLOCK(PVoice, BLOCK_VOICE);
N64_BLOCK(Section, BLOCK_DELAY);
N64_BLOCK(PVoice *, BLOCK_PTR);
N64_BLOCK(ALChanState, BLOCK_CHANSTATE);
N64_BLOCK(ALVoiceState, BLOCK_VOICESTATE);
N64_BLOCK(ALEventListItem, BLOCK_EVENT);
/* (the game is given its address: the oracle's log has it) */
_Static_assert(sizeof(void *) != 4 || __builtin_offsetof(PVoice, dma_state) == 0x44, "PVoice.dma_state");
/* aLoadADPCM'd from its block: 8-byte aligned */
_Static_assert(__builtin_offsetof(LowPass, coef) == 8, "LowPass.coef");


/* ---- what the library's blocks grow by (0 where pointers are 4 bytes) ---- */

/* alSynNew's: for the synthesizer's maxPVoices and maxUpdates and the
   reverb's sections, of which nchoruses modulate and nlowpasses filter */
#define AL_SYN_HEAP_GROWTH(maxPVoices, maxUpdates, nsections, nchoruses, nlowpasses)       \
    (AL_HEAP_GROWTH(1, OutStage, BLOCK_SAVE) + AL_HEAP_GROWTH(1, AuxBus, BLOCK_AUXBUS) +  \
     AL_HEAP_GROWTH(maxPVoices, PVoice *, BLOCK_PTR) +                                    \
     AL_HEAP_GROWTH(1, MainBus, BLOCK_MAINBUS) +                                          \
     AL_HEAP_GROWTH(maxPVoices, void *, BLOCK_PTR) +                                      \
     AL_HEAP_GROWTH(nsections, Section, BLOCK_DELAY) +                                    \
     (nchoruses) * AL_HEAP_GROWTH(1, Chorus, BLOCK_RESAMPLER) +                           \
     (nlowpasses) * AL_HEAP_GROWTH(1, LowPass, BLOCK_LOWPASS) +                           \
     AL_HEAP_GROWTH(maxPVoices, PVoice, BLOCK_VOICE) +                                    \
     AL_HEAP_GROWTH(maxUpdates, Update, BLOCK_UPDATE))

/* alCSPNew's: for its config's maxChannels, maxVoices and maxEvents */
#define AL_CSP_HEAP_GROWTH(maxChannels, maxVoices, maxEvents)                              \
    (AL_HEAP_GROWTH(maxChannels, ALChanState, BLOCK_CHANSTATE) +                          \
     AL_HEAP_GROWTH(maxVoices, ALVoiceState, BLOCK_VOICESTATE) +                          \
     AL_HEAP_GROWTH(maxEvents, ALEventListItem, BLOCK_EVENT))

#endif
