/*
 * The libaudio oracle's log (docs/PORT.md, "The libaudio oracle"): what
 * port/tools/audio_oracle/replay.c reads, and what the port's recorder
 * wrote (port/src/audio_record.c and port/host/audiolog.c, a build with
 * the game at the N64's addresses: gone with P10; the logs it recorded
 * are the oracle's references).
 *
 * A stream of little-endian 32-bit words.  It starts with ALOG_MAGIC and
 * ALOG_VERSION; then records, each a tag word and its fields:
 *
 *   TRACK addr len          memory the library and the game share
 *   MEM addr len bytes      the game changed these bytes (padded to 4)
 *   CALL fn a0..a5          the game calls fn (ALOG_* below)
 *   IN addr len bytes       ... handing it this structure
 *   OUT addr len bytes      ... which gave back this structure
 *   ACMD len hash_lo hash_hi  ... which wrote this command list (FNV-1a 64)
 *   RET value               ... which returned
 *   CBENTER kind a0 a1 a2   the library calls the game back (ALOG_CB_*)
 *   CBRET value extra       ... which returned (extra: DMANEW's *state)
 *   UNTRACK addr len        no longer shared (the RSP's and the AI's buffers)
 *   END
 *
 * MEM records come where control passes from the game to the library (before
 * a CALL, before a CBRET): what the game, the RSP and the PI changed in the
 * tracked memory since the library last had it.
 */
#ifndef AUDIO_LOG_H
#define AUDIO_LOG_H

#include <stdint.h>

#define ALOG_MAGIC 0x474F4C41u      /* "ALOG" */
#define ALOG_VERSION 1

enum {
    ALOG_TRACK = 1, ALOG_MEM, ALOG_CALL, ALOG_IN, ALOG_OUT, ALOG_ACMD, ALOG_RET,
    ALOG_CBENTER, ALOG_CBRET, ALOG_END, ALOG_UNTRACK
};

/* the functions (CALL's fn) */
#define ALOG_FUNCS(X) \
    X(alHeapInit) X(alHeapDBAlloc) X(alBnkfNew) X(alSeqFileNew) X(alInit) X(alClose) \
    X(alAudioFrame) X(alLink) X(alUnlink) X(alEvtqNew) X(alEvtqNextEvent)           \
    X(alEvtqPostEvent) X(alSynAddPlayer) X(alSynAllocVoice) X(alSynFreeVoice)        \
    X(alSynStopVoice) X(alSynStartVoice) X(alSynSetVol) X(alSynSetPan)               \
    X(alSynSetPitch) X(alSynSetFXMix) X(alCents2Ratio) X(alCSPNew) X(alCSPPlay)      \
    X(alCSPStop) X(alCSPSetBank) X(alCSPSetSeq) X(alCSPGetState) X(alCSeqGetLoc)     \
    X(alCSPSetVol) X(alCSPGetVol) X(alCSPSetTempo) X(alCSPGetTempo) X(alCSeqNew)     \
    X(alCSeqSetLoc)
#define ALOG_ENUM(f) ALOG_##f,
enum { ALOG_FN_NONE, ALOG_FUNCS(ALOG_ENUM) ALOG_FN_COUNT };
#undef ALOG_ENUM

/* the callbacks (CBENTER's kind) */
enum { ALOG_CB_HANDLER = 1, ALOG_CB_DMANEW, ALOG_CB_DMA };

/*
 * The bank objects' map (for the replay): a library that builds its own
 * objects for a bank file's (each ALBank, ALInstrument, ALSound and
 * ALWaveTable, instead of relocating the file's in place) exports, under
 * PORT_AUDIO_ORACLE,
 *
 *   uint32_t alog_bank_map(const AlogBankObj **objs);
 *
 * which points *objs at every object it has built so far and returns how
 * many: each one's address in the file (what the original's pointers
 * hold, so the log's) and its own.  After each alBnkfNew the replay takes
 * the map and translates the log's words that are a mapped object's
 * address (call arguments, the game's memory writes) to the library's,
 * and the library's back where it compares them.  A library without the
 * symbol relocates in place, and nothing is translated.
 */
typedef struct {
    uint32_t file;      /* the object in the bank file */
    uint32_t native;    /* the library's own */
} AlogBankObj;
#define ALOG_BANK_MAP "alog_bank_map"

#endif
