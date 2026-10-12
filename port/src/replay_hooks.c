/*
 * The port's --replay hooks on the N64 side (port/host/replay.c).
 *
 * What the game learns of the audio thread's progress: whether the sequence
 * player is playing (alCSPGetState, func_802D4E10) and how far the sequence
 * got (alCSeqGetLoc's lastTicks).  The music manager (1C460.c) and the ends
 * of the results screens (17E10.c, 00000.c) wait on them.  --replay gives
 * the game the movie's answers (port/host/replay.c), by the calling
 * function; otherwise they are the real ones.  functions.h makes the
 * game's calls calls of port_hook_802D4E10 and port_hook_alCSeqGetLoc,
 * with the calling function's name (__func__): no link-time wrapping, no
 * return addresses (not every host has a symbol table to look them up
 * in).
 */
#include "common.h"
#include "functions.h"
#include "port.h"
#include <PR/libaudio.h>

/* (the real ones, not functions.h's hooks) */
#undef func_802D4E10
#undef alCSeqGetLoc

s32 port_hook_802D4E10(ALCSPlayer *seqp, const char *caller) {
    return host_replay_audio_named(0, func_802D4E10(seqp), caller);
}

void port_hook_alCSeqGetLoc(ALCSeq *seq, ALCSeqMarker *m, const char *caller) {
    alCSeqGetLoc(seq, m);
    m->lastTicks = host_replay_audio_named(1, m->lastTicks, caller);
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
    while (!host_replay_save_due(host_blocked_on(&D_80219F50))) {
        osSetTimer(&t, OS_USEC_TO_CYCLES(1000), 0, &q, NULL);
        osRecvMesg(&q, NULL, OS_MESG_BLOCK);
    }
}
