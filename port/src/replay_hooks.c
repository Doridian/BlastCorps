/*
 * The port's --replay hooks on the N64 side (port/host/replay.c).
 *
 * What the game learns of the audio thread's progress: whether the sequence
 * player is playing (alCSPGetState, func_802D4E10) and how far the sequence
 * got (alCSeqGetLoc's lastTicks).  The music manager (1C460.c) and the ends
 * of the results screens (17E10.c, 00000.c) wait on them.  The link wraps
 * both (CMakeLists.txt) so that --replay can give the game the movie's
 * answers (port/host/replay.c); otherwise they are the real ones.
 */
#include "common.h"
#include "port.h"
#include <PR/libaudio.h>

s32 __real_func_802D4E10(ALCSPlayer *seqp);
void __real_alCSeqGetLoc(ALCSeq *seq, ALCSeqMarker *m);

/* the caller, by its return address, which host_replay_audio looks up in
   the port's symbol table; the movable build's arena link names it instead
   (port_replay_set_caller) */
#ifdef PORT_MOVABLE
#define CALLER 0
#else
#define CALLER (uint64_t)(uintptr_t)__builtin_return_address(0)
#endif

s32 __wrap_func_802D4E10(ALCSPlayer *seqp) {
    s32 state = __real_func_802D4E10(seqp);
    return host_replay_audio(0, state, CALLER);
}

void __wrap_alCSeqGetLoc(ALCSeq *seq, ALCSeqMarker *m) {
    __real_alCSeqGetLoc(seq, m);
    m->lastTicks = host_replay_audio(1, m->lastTicks, CALLER);
}

/* The pak/EEPROM thread, having taken a command: it changes the save record
   the game plays on, so with --replay it waits for the frame the movie's
   thread took it in (host_replay_save_due), a millisecond at a time.  Not
   when the game waits for its reply (D_80219F50): then the movie's thread
   ran it in that frame too.  (ultra.c blocks a receiver on the queue's
   address.) */
extern OSMesgQueue D_80219F50;

void port_replay_save_started(void) {
    OSMesgQueue q;
    OSMesg m;
    OSTimer t;

    osCreateMesgQueue(&q, &m, 1);
    while (!host_replay_save_due(host_blocked_on((uint32_t)(uintptr_t)&D_80219F50))) {
        osSetTimer(&t, OS_USEC_TO_CYCLES(1000), 0, &q, NULL);
        osRecvMesg(&q, NULL, OS_MESG_BLOCK);
    }
}
