/*
 * The libaudio oracle's recorder, N64 side (docs/PORT.md, "The libaudio
 * oracle"): built with -DPORT_AUDIO_RECORD=ON only, which also wraps every
 * libaudio function the game calls (CMakeLists.txt, AUDIO_RECORD_WRAPS).
 *
 * Each wrapper tells port/host/audiolog.c about a call the game makes into
 * the library: its arguments, the structures it hands over or gets back,
 * its result.  Calls the library makes to itself pass straight through (a
 * thread's depth counts them).  The two places the library calls back into
 * the game, a client's voice handler and the DMA routine, go through the
 * trampolines below, which log the game's side of them.  The host keeps a
 * copy of the memory the library and the game share and logs what the game
 * changed in it each time control passes from the game to the library, so
 * that tools/audio_oracle can replay the library's whole input.
 *
 * While the library runs, interrupts are masked: then no other thread runs
 * in the middle of a call (the poll preemption, ultra.c), and the log is a
 * plain sequence.
 */
#ifdef PORT_AUDIO_RECORD
#include "common.h"
#include "port.h"
#include "game/audio.h"
#include "audio_log.h"

#define A(p) ((uint32_t)(uintptr_t)(p))

static s32 depth[PORT_MAX_THREADS + 1];

static s32 *my_depth(void) {
    u32 t = host_thread_current();
    OSThread *th = (OSThread *)(uintptr_t)t;
    s32 id = t != 0 ? th->id : 0;
    if (id < 0 || id > PORT_MAX_THREADS)
        id = 0;
    return &depth[id];
}

static u32 f2u(f32 f) {
    union { f32 f; u32 u; } x;
    x.f = f;
    return x.u;
}

/* the game's side of the library's callbacks */
static ALVoiceHandler game_handler[8];
static ALPlayer *game_client[8];
static int n_clients;
static ALDMANew game_dmanew;
static ALDMAproc game_dma;

#define ENTER(fn, a0, a1, a2, a3, a4, a5)                                    \
    s32 *d_ = my_depth();                                                   \
    OSIntMask m_ = 0;                                                       \
    int top_ = *d_ == 0;                                                    \
    if (top_) {                                                             \
        m_ = osSetIntMask(OS_IM_NONE);                                      \
        host_alog_call(fn, a0, a1, a2, a3, a4, a5);                         \
    }                                                                       \
    (*d_)++
#define LEAVE(ret)                                                          \
    (*d_)--;                                                                \
    if (top_) {                                                             \
        host_alog_ret(ret);                                                 \
        osSetIntMask(m_);                                                   \
    }

static ALMicroTime rec_handler(void *node) {
    int i;
    s32 *d = my_depth();
    s32 saved = *d;
    ALMicroTime r;

    for (i = 0; i < n_clients; i++)
        if (game_client[i] == (ALPlayer *)node)
            break;
    if (i == n_clients)
        host_fatal("audio_record: a handler for an unknown client %08X", A(node));
    host_alog_cb_enter(ALOG_CB_HANDLER, A(node), 0, 0);
    *d = 0;
    r = game_handler[i](node);
    *d = saved;
    host_alog_cb_leave((u32)r, 0);
    return r;
}

static s32 rec_dma(s32 addr, s32 len, void *state) {
    s32 *d = my_depth();
    s32 saved = *d;
    s32 r;

    host_alog_cb_enter(ALOG_CB_DMA, (u32)addr, (u32)len, A(state));
    *d = 0;
    r = game_dma(addr, len, state);
    *d = saved;
    host_alog_cb_leave((u32)r, 0);
    return r;
}

static ALDMAproc rec_dmanew(void *state) {
    s32 *d = my_depth();
    s32 saved = *d;
    ALDMAproc p;

    host_alog_cb_enter(ALOG_CB_DMANEW, A(state), 0, 0);
    *d = 0;
    p = game_dmanew(state);
    *d = saved;
    if (game_dma != NULL && game_dma != p)
        host_fatal("audio_record: a second DMA routine");
    game_dma = p;
    host_alog_cb_leave(A(p), *(u32 *)state);
    return (ALDMAproc)rec_dma;
}

/* ---- the heap and the bank files ------------------------------------------ */

void __real_alHeapInit(ALHeap *hp, u8 *base, s32 len);
void __wrap_alHeapInit(ALHeap *hp, u8 *base, s32 len) {
    extern u8 D_80366BD0[], D_8036AFB0[], D_802E8CE0[], D_802F3AF0[], D_80370C80[];

    if (*my_depth() == 0 && !host_alog_started()) {
        /* what the library and the game share: the audio heap and the
           audio objects' .data and .bss (1A630, 1C460, 22EE0) */
        host_alog_track(A(D_80370C80), AUDIO_HEAP_SIZE);
        host_alog_track(A(D_80366BD0), A(D_8036AFB0) + 0x900 - A(D_80366BD0));
        host_alog_track(A(D_802E8CE0), 0x20);
        host_alog_track(A(D_802F3AF0), 0x120);
        host_alog_start();
    }
    {
        ENTER(ALOG_alHeapInit, A(hp), A(base), (u32)len, 0, 0, 0);
        __real_alHeapInit(hp, base, len);
        LEAVE(0);
    }
}

void *__real_alHeapDBAlloc(u8 *file, s32 line, ALHeap *hp, s32 num, s32 size);
void *__wrap_alHeapDBAlloc(u8 *file, s32 line, ALHeap *hp, s32 num, s32 size) {
    void *r;
    ENTER(ALOG_alHeapDBAlloc, A(file), (u32)line, A(hp), (u32)num, (u32)size, 0);
    r = __real_alHeapDBAlloc(file, line, hp, num, size);
    LEAVE(A(r));
    return r;
}

void __real_alBnkfNew(ALBankFile *file, u8 *table);
void __wrap_alBnkfNew(ALBankFile *file, u8 *table) {
    ENTER(ALOG_alBnkfNew, A(file), A(table), 0, 0, 0, 0);
    __real_alBnkfNew(file, table);
    LEAVE(0);
}

void __real_alSeqFileNew(ALSeqFile *file, u8 *base);
void __wrap_alSeqFileNew(ALSeqFile *file, u8 *base) {
    ENTER(ALOG_alSeqFileNew, A(file), A(base), 0, 0, 0, 0);
    __real_alSeqFileNew(file, base);
    LEAVE(0);
}

/* ---- the driver ------------------------------------------------------------- */

void __real_alInit(ALGlobals *g, ALSynConfig *c);
void __wrap_alInit(ALGlobals *g, ALSynConfig *c) {
    SynConfig *sc = (SynConfig *)c;
    ENTER(ALOG_alInit, A(g), A(c), 0, 0, 0, 0);
    if (top_) {
        host_alog_in(A(c), sizeof(SynConfig));
        if (sc->fxType == AL_FX_CUSTOM && sc->params != NULL)
            host_alog_in(A(sc->params), (2 + 8 * sc->params[0]) * 4);
        game_dmanew = (ALDMANew)sc->dmaproc;
        sc->dmaproc = (void *)rec_dmanew;
    }
    __real_alInit(g, c);
    if (top_)
        sc->dmaproc = (void *)game_dmanew;
    LEAVE(0);
}

void __real_alClose(ALGlobals *g);
void __wrap_alClose(ALGlobals *g) {
    ENTER(ALOG_alClose, A(g), 0, 0, 0, 0, 0);
    __real_alClose(g);
    LEAVE(0);
}

Acmd *__real_alAudioFrame(Acmd *cmdList, s32 *cmdLen, s16 *outBuf, s32 outLen);
Acmd *__wrap_alAudioFrame(Acmd *cmdList, s32 *cmdLen, s16 *outBuf, s32 outLen) {
    Acmd *r;
    ENTER(ALOG_alAudioFrame, A(cmdList), A(cmdLen), A(outBuf), (u32)outLen, 0, 0);
    if (top_) {
        /* the buffers the game hands the RSP and the AI: the library never
           reads them, and the RSP and the PI write them every frame */
        extern AMAudioMgr D_80368070;
        extern AMDMABuffer D_8036A318[];
        extern u32 D_8036A8C0;
        static int excluded;
        int i;
        if (!excluded) {
            excluded = 1;
            for (i = 0; i < NUM_DMA_MESSAGES; i++)
                host_alog_untrack(A(D_8036A318[i].ptr), 0x200);
            for (i = 0; i < 2; i++)
                host_alog_untrack(A(D_80368070.ACMDList[i]), 0x55F0);
            for (i = 0; i < 3; i++)
                host_alog_untrack(A(D_80368070.audioInfo[i]->data), D_8036A8C0 * 4);
        }
    }
    r = __real_alAudioFrame(cmdList, cmdLen, outBuf, outLen);
    if (top_) {
        host_alog_out(A(cmdLen), 4);
        host_alog_acmd(A(cmdList), (u32)((u8 *)r - (u8 *)cmdList));
    }
    LEAVE(A(r));
    return r;
}

void __real_alLink(ALLink *ln, ALLink *to);
void __wrap_alLink(ALLink *ln, ALLink *to) {
    ENTER(ALOG_alLink, A(ln), A(to), 0, 0, 0, 0);
    __real_alLink(ln, to);
    LEAVE(0);
}

void __real_alUnlink(ALLink *ln);
void __wrap_alUnlink(ALLink *ln) {
    ENTER(ALOG_alUnlink, A(ln), 0, 0, 0, 0, 0);
    __real_alUnlink(ln);
    LEAVE(0);
}

void __real_alEvtqNew(ALEventQueue *q, ALEventListItem *items, s32 n);
void __wrap_alEvtqNew(ALEventQueue *q, ALEventListItem *items, s32 n) {
    ENTER(ALOG_alEvtqNew, A(q), A(items), (u32)n, 0, 0, 0);
    __real_alEvtqNew(q, items, n);
    LEAVE(0);
}

ALMicroTime __real_alEvtqNextEvent(ALEventQueue *q, ALEvent *evt);
ALMicroTime __wrap_alEvtqNextEvent(ALEventQueue *q, ALEvent *evt) {
    ALMicroTime r;
    ENTER(ALOG_alEvtqNextEvent, A(q), A(evt), 0, 0, 0, 0);
    r = __real_alEvtqNextEvent(q, evt);
    if (top_)
        host_alog_out(A(evt), sizeof(ALEvent));
    LEAVE((u32)r);
    return r;
}

void __real_alEvtqPostEvent(ALEventQueue *q, ALEvent *evt, ALMicroTime delta);
void __wrap_alEvtqPostEvent(ALEventQueue *q, ALEvent *evt, ALMicroTime delta) {
    ENTER(ALOG_alEvtqPostEvent, A(q), A(evt), (u32)delta, 0, 0, 0);
    if (top_)
        host_alog_in(A(evt), sizeof(ALEvent));
    __real_alEvtqPostEvent(q, evt, delta);
    LEAVE(0);
}

/* ---- voices ------------------------------------------------------------------- */

void __real_alSynAddPlayer(ALSynth *s, ALPlayer *client);
void __wrap_alSynAddPlayer(ALSynth *s, ALPlayer *client) {
    ENTER(ALOG_alSynAddPlayer, A(s), A(client), 0, 0, 0, 0);
    if (top_) {
        if (n_clients == 8)
            host_fatal("audio_record: too many clients");
        game_client[n_clients] = client;
        game_handler[n_clients++] = client->handler;
        client->handler = rec_handler;
    }
    __real_alSynAddPlayer(s, client);
    LEAVE(0);
}

s32 __real_alSynAllocVoice(ALSynth *s, ALVoice *v, ALVoiceConfig *vc);
s32 __wrap_alSynAllocVoice(ALSynth *s, ALVoice *v, ALVoiceConfig *vc) {
    s32 r;
    ENTER(ALOG_alSynAllocVoice, A(s), A(v), A(vc), 0, 0, 0);
    if (top_)
        host_alog_in(A(vc), sizeof(ALVoiceConfig));
    r = __real_alSynAllocVoice(s, v, vc);
    if (top_)
        host_alog_out(A(v), sizeof(ALVoice));
    LEAVE((u32)r);
    return r;
}

#define VOICE_FN(name, ...)                                                   \
    void __real_##name(ALSynth *s, ALVoice *v);                               \
    void __wrap_##name(ALSynth *s, ALVoice *v) {                              \
        ENTER(ALOG_##name, A(s), A(v), 0, 0, 0, 0);                           \
        __real_##name(s, v);                                                  \
        if (top_)                                                             \
            host_alog_out(A(v), sizeof(ALVoice));                             \
        LEAVE(0);                                                             \
    }
VOICE_FN(alSynFreeVoice)
VOICE_FN(alSynStopVoice)

void __real_alSynStartVoice(ALSynth *s, ALVoice *v, ALWaveTable *w);
void __wrap_alSynStartVoice(ALSynth *s, ALVoice *v, ALWaveTable *w) {
    ENTER(ALOG_alSynStartVoice, A(s), A(v), A(w), 0, 0, 0);
    __real_alSynStartVoice(s, v, w);
    LEAVE(0);
}

void __real_alSynSetVol(ALSynth *s, ALVoice *v, s16 vol, ALMicroTime t);
void __wrap_alSynSetVol(ALSynth *s, ALVoice *v, s16 vol, ALMicroTime t) {
    ENTER(ALOG_alSynSetVol, A(s), A(v), (u32)(s32)vol, (u32)t, 0, 0);
    __real_alSynSetVol(s, v, vol, t);
    LEAVE(0);
}

void __real_alSynSetPan(ALSynth *s, ALVoice *v, u8 pan);
void __wrap_alSynSetPan(ALSynth *s, ALVoice *v, u8 pan) {
    ENTER(ALOG_alSynSetPan, A(s), A(v), pan, 0, 0, 0);
    __real_alSynSetPan(s, v, pan);
    LEAVE(0);
}

void __real_alSynSetPitch(ALSynth *s, ALVoice *v, f32 pitch);
void __wrap_alSynSetPitch(ALSynth *s, ALVoice *v, f32 pitch) {
    ENTER(ALOG_alSynSetPitch, A(s), A(v), f2u(pitch), 0, 0, 0);
    __real_alSynSetPitch(s, v, pitch);
    LEAVE(0);
}

void __real_alSynSetFXMix(ALSynth *s, ALVoice *v, u8 fx);
void __wrap_alSynSetFXMix(ALSynth *s, ALVoice *v, u8 fx) {
    ENTER(ALOG_alSynSetFXMix, A(s), A(v), fx, 0, 0, 0);
    __real_alSynSetFXMix(s, v, fx);
    LEAVE(0);
}

f32 __real_alCents2Ratio(s32 cents);
f32 __wrap_alCents2Ratio(s32 cents) {
    f32 r;
    ENTER(ALOG_alCents2Ratio, (u32)cents, 0, 0, 0, 0, 0);
    r = __real_alCents2Ratio(cents);
    LEAVE(f2u(r));
    return r;
}

/* ---- the sequence player and its sequences ------------------------------------ */

void __real_alCSPNew(ALCSPlayer *p, ALSeqpConfig *c);
void __wrap_alCSPNew(ALCSPlayer *p, ALSeqpConfig *c) {
    ENTER(ALOG_alCSPNew, A(p), A(c), 0, 0, 0, 0);
    if (top_)
        host_alog_in(A(c), sizeof(ALSeqpConfig));
    __real_alCSPNew(p, c);
    LEAVE(0);
}

#define SEQP_FN(name, id)                                                     \
    void __real_##name(ALCSPlayer *p);                                        \
    void __wrap_##name(ALCSPlayer *p) {                                       \
        ENTER(id, A(p), 0, 0, 0, 0, 0);                                       \
        __real_##name(p);                                                     \
        LEAVE(0);                                                             \
    }
SEQP_FN(func_802D81F0, ALOG_alCSPPlay)
SEQP_FN(func_802D76C0, ALOG_alCSPStop)

void __real_func_802D97E0(ALCSPlayer *p, ALBank *b);
void __wrap_func_802D97E0(ALCSPlayer *p, ALBank *b) {
    ENTER(ALOG_alCSPSetBank, A(p), A(b), 0, 0, 0, 0);
    __real_func_802D97E0(p, b);
    LEAVE(0);
}

void __real_func_802D81B0(ALCSPlayer *p, ALCSeq *seq);
void __wrap_func_802D81B0(ALCSPlayer *p, ALCSeq *seq) {
    ENTER(ALOG_alCSPSetSeq, A(p), A(seq), 0, 0, 0, 0);
    __real_func_802D81B0(p, seq);
    LEAVE(0);
}

/* (the replay's hooks wrap these two already: replay_hooks.c calls them) */
s32 __real_func_802D4E10(ALCSPlayer *p);
s32 rec_func_802D4E10(ALCSPlayer *p) {
    s32 r;
    ENTER(ALOG_alCSPGetState, A(p), 0, 0, 0, 0, 0);
    r = __real_func_802D4E10(p);
    LEAVE((u32)r);
    return r;
}

void __real_alCSeqGetLoc(ALCSeq *seq, ALCSeqMarker *m);
void rec_alCSeqGetLoc(ALCSeq *seq, ALCSeqMarker *m) {
    ENTER(ALOG_alCSeqGetLoc, A(seq), A(m), 0, 0, 0, 0);
    __real_alCSeqGetLoc(seq, m);
    if (top_)
        host_alog_out(A(m), sizeof(ALCSeqMarker));
    LEAVE(0);
}

void __real_alCSPSetVol(ALCSPlayer *p, s16 vol);
void __wrap_alCSPSetVol(ALCSPlayer *p, s16 vol) {
    ENTER(ALOG_alCSPSetVol, A(p), (u32)(s32)vol, 0, 0, 0, 0);
    __real_alCSPSetVol(p, vol);
    LEAVE(0);
}

s16 __real_alCSPGetVol(ALCSPlayer *p);
s16 __wrap_alCSPGetVol(ALCSPlayer *p) {
    s16 r;
    ENTER(ALOG_alCSPGetVol, A(p), 0, 0, 0, 0, 0);
    r = __real_alCSPGetVol(p);
    LEAVE((u32)(s32)r);
    return r;
}

void __real_alCSPSetTempo(ALCSPlayer *p, s32 tempo);
void __wrap_alCSPSetTempo(ALCSPlayer *p, s32 tempo) {
    ENTER(ALOG_alCSPSetTempo, A(p), (u32)tempo, 0, 0, 0, 0);
    __real_alCSPSetTempo(p, tempo);
    LEAVE(0);
}

s32 __real_alCSPGetTempo(ALCSPlayer *p);
s32 __wrap_alCSPGetTempo(ALCSPlayer *p) {
    s32 r;
    ENTER(ALOG_alCSPGetTempo, A(p), 0, 0, 0, 0, 0);
    r = __real_alCSPGetTempo(p);
    LEAVE((u32)r);
    return r;
}

void __real_alCSeqNew(ALCSeq *seq, u8 *ptr);
void __wrap_alCSeqNew(ALCSeq *seq, u8 *ptr) {
    ENTER(ALOG_alCSeqNew, A(seq), A(ptr), 0, 0, 0, 0);
    __real_alCSeqNew(seq, ptr);
    LEAVE(0);
}

void __real_alCSeqSetLoc(ALCSeq *seq, ALCSeqMarker *m);
void __wrap_alCSeqSetLoc(ALCSeq *seq, ALCSeqMarker *m) {
    ENTER(ALOG_alCSeqSetLoc, A(seq), A(m), 0, 0, 0, 0);
    if (top_)
        host_alog_in(A(m), sizeof(ALCSeqMarker));
    __real_alCSeqSetLoc(seq, m);
    LEAVE(0);
}
#endif
