/*
 * The port's libaudio (docs/PORT.md, "libaudio"): what its files share.
 *
 * The synthesizer is a fixed chain, not a graph: each physical voice
 * decodes its wave table (ADPCM or 16-bit), resamples it to the output rate
 * and mixes it, with its volume ramp, pan and effect send, into the main
 * and auxiliary buses; the auxiliary bus goes through the reverb into the
 * main one, which is interleaved and saved to the output buffer.  It all
 * runs on the RSP: this code only writes the command list (abi.h), in
 * sections of at most MAX_SECTION samples.
 *
 * Exactness (port/tools/audio_oracle checks it): the command list has to be
 * the reference's word for word, so the order of the commands, the DMEM
 * layout, the addresses of the RSP's state blocks (which come from the
 * heap) and the rounding of every float computation are the reference's.
 * The heap blocks are allocated in the same order and sizes (BLOCK_* below)
 * and the structures the game holds (ALSynth, ALVoice, the players) have
 * the public layouts; what is in the library's own blocks is this code's.
 */
#ifndef AUDIO_PRIVATE_H
#define AUDIO_PRIVATE_H

#include "common.h"

/* DMEM, in bytes: the work buffers of one section */
#define DMEM_DECODED_IN 0       /* the compressed input, then the resampled voice */
#define DMEM_DECODED 320        /* the decoded voice */
#define DMEM_TEMP0 0            /* the reverb's three */
#define DMEM_TEMP1 320
#define DMEM_TEMP2 640
#define DMEM_MAIN_L 1088
#define DMEM_MAIN_R 1408
#define DMEM_AUX_L 1728
#define DMEM_AUX_R 2048

#define MAX_SECTION 160         /* samples a section makes at most */

/* the heap blocks as the reference allocates them (their sizes decide where
   everything after them is, and the RSP's state addresses are in the
   command list) */
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

/* ---- updates: a voice's changes, queued at the sample they take effect ---- */

enum {
    UPD_FREE_VOICE,             /* back to the free list (a stolen voice's end) */
    UPD_UNUSED1, UPD_UNUSED2, UPD_UNUSED3, UPD_UNUSED4, UPD_UNUSED5, UPD_UNUSED6,
    UPD_PITCH = 7,
    UPD_VOLUME = 11,
    UPD_PAN = 12,
    UPD_START_PARAMS = 13,      /* wave, pitch, volume, pan, send and ramp at once */
    UPD_START = 14,
    UPD_STOP = 15,
    UPD_FXMIX = 16
};

typedef struct Update {
    struct Update *PTR32 next;
    s32 at;                     /* the sample it takes effect */
    s16 kind;
    s16 unity;                  /* start: play at the table's own rate */
    union {
        f32 f;
        s32 i;
        void *PTR32 p;
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
    ALWaveTable *PTR32 wave;
} Update;

/* ---- the physical voice --------------------------------------------------- */

typedef struct PVoice_s {
    ALLink node;                /* on one of the synthesizer's three lists */
    ALVoice *PTR32 owner;
    s32 steal_delay;            /* stolen: the samples its ramp down takes, else 0 */

    /* the decoder */
    ADPCM_STATE *PTR32 dec_state;
    ADPCM_STATE *PTR32 loop_state;  /* the table's state at its loop start */
    u32 loop_start, loop_end, loop_count;   /* (unsigned, as the tables have them) */
    ALWaveTable *PTR32 table;
    s32 book_bytes;
    s32 played;                 /* samples of the table handed out */
    s32 frac;                   /* decoded samples left over in the last frame */
    s32 dec_first;
    s32 rom;                    /* the next byte of the table to fetch */
    s32 unused0;
    ALDMAproc dma;
    void *PTR32 dma_state;      /* (at 0x44: the game is given its address) */

    /* the resampler */
    RESAMPLE_STATE *PTR32 rs_state;
    f32 ratio;
    s32 unity;
    f32 rs_frac;
    s32 rs_first;

    /* the envelope and mixer */
    ENVMIX_STATE *PTR32 env_state;
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
    Update *PTR32 updates;
    Update *PTR32 updates_tail;
    s32 playing;

    u8 pad[BLOCK_VOICE - 144];
} PVoice;

/* ---- the reverb ------------------------------------------------------------ */

typedef struct {
    s16 fc;
    s16 gain;
    s32 pad;
    s16 coef[16];               /* aLoadADPCM'd as a book: at 8 in its block */
    POLEF_STATE *PTR32 state;
    s32 first;
} LowPass;

typedef struct {
    RESAMPLE_STATE *PTR32 state;
    f32 frac;
    s32 first;
    u8 pad[BLOCK_RESAMPLER - 12];
} Chorus;

typedef struct {
    u32 in, out;                /* taps, samples behind the write position */
    s16 ff, fb, gain;
    f32 mod_step;               /* the chorus: its sawtooth's rate, */
    f32 mod_phase;              /* ... where it is, */
    s32 drift;                  /* ... how far the read tap has moved */
    f32 mod_depth;
    LowPass *PTR32 lp;
    Chorus *PTR32 chorus;
} Section;

/* the synthesizer's own blocks: the output stage, the aux bus with the
   reverb, the main bus (only their addresses are seen, in ALSynth) */
typedef struct {
    s32 dram;                   /* this section's output */
    u8 pad[BLOCK_SAVE - 4];
} OutStage;

typedef struct {
    s16 *PTR32 line;            /* the delay line */
    s16 *PTR32 pos;             /* its write position */
    u32 len;
    Section *PTR32 sections;
    u8 nsections;
    s32 on;
} Reverb;

typedef struct ALAuxBus_s {
    PVoice *PTR32 *PTR32 voices;
    s32 nvoices;
    s32 maxvoices;
    Reverb fx;
    u8 pad[BLOCK_AUXBUS - 12 - sizeof(Reverb)];
} AuxBus;

typedef struct ALMainBus_s {
    s32 nsources;               /* 1: the reverb's or the aux bus's output */
    void *PTR32 *PTR32 sources;
    u8 pad[BLOCK_MAINBUS - 8];
} MainBus;

_Static_assert(sizeof(Update) == BLOCK_UPDATE, "Update");
_Static_assert(sizeof(PVoice) == BLOCK_VOICE, "PVoice");
_Static_assert(__builtin_offsetof(PVoice, dma_state) == 0x44, "PVoice.dma_state");
_Static_assert(sizeof(LowPass) == BLOCK_LOWPASS, "LowPass");
_Static_assert(sizeof(Chorus) == BLOCK_RESAMPLER, "Chorus");
_Static_assert(sizeof(Section) == BLOCK_DELAY, "Section");
_Static_assert(sizeof(OutStage) == BLOCK_SAVE, "OutStage");
_Static_assert(sizeof(AuxBus) == BLOCK_AUXBUS, "AuxBus");
_Static_assert(sizeof(MainBus) == BLOCK_MAINBUS, "MainBus");

/* ---- between the files ------------------------------------------------------- */

extern s16 al_eqpower[128];
void al_eqpower_init(void);

Update *al_update_alloc(ALSynth *s);
void al_update_free(ALSynth *s, Update *u);
void al_voice_queue(PVoice *pv, Update *u);
void al_voice_release(ALSynth *s, PVoice *pv);
s32 al_usec_to_samples(ALSynth *s, s32 usec);

void al_voice_init(PVoice *pv, ALDMANew dmanew, ALHeap *hp);
Acmd *al_voice_mix(PVoice *pv, s32 n, s32 at, Acmd *cmd);

void al_reverb_init(Reverb *r, ALSynConfig *c, ALHeap *hp);
Acmd *al_reverb_run(Reverb *r, s32 n, Acmd *cmd);

#endif
