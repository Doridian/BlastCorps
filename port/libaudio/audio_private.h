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
 * The heap blocks are allocated in the same order and sizes
 * (libaudio_heap.h, which also has what this code keeps in them) and the
 * structures the game holds (ALSynth, ALVoice, the players) have the
 * public layouts.  Where pointers are 8 bytes (the LP64 port) a structure
 * can be bigger than its block was on the N64: the block is then its size,
 * the heap grows by that (game/audio.h's AUDIO_HEAP_SIZE) and its
 * addresses aren't the reference's, which only the oracle's i386 replay
 * compares.
 */
#ifndef AUDIO_PRIVATE_H
#define AUDIO_PRIVATE_H

#include "common.h"
#include "libaudio_heap.h"   /* the heap blocks and what this code keeps in them */

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
