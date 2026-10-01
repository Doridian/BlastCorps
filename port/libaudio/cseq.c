/*
 * The port's libaudio: compact MIDI sequences.
 *
 * A compact sequence is up to 16 tracks, each a stream of events with the
 * ticks before them as variable-length numbers.  Compared with a MIDI file:
 * a note on carries its length (and there are no note offs), there are loop
 * start and end meta events, and a track can repeat an earlier stretch of
 * itself (0xFE, the distance back in two bytes, the length in one; 0xFE
 * 0xFE is a 0xFE).  Every track's next event is kept as ticks from the
 * last event read, so reading one takes the smallest off all of them.
 */
#include "audio_private.h"

static u8 next_byte(ALCSeq *seq, u32 track) {
    u8 b;

    if (seq->curBULen[track] != 0) {
        b = *seq->curBUPtr[track]++;
        seq->curBULen[track]--;
        return b;
    }
    b = *seq->curLoc[track]++;
    if (b == AL_CMIDI_BLOCK_CODE) {
        u8 hi = *seq->curLoc[track]++;
        if (hi != AL_CMIDI_BLOCK_CODE) {
            /* a repeat: back from where its four bytes started */
            u8 lo = *seq->curLoc[track]++;
            u8 len = *seq->curLoc[track]++;
            seq->curBUPtr[track] = seq->curLoc[track] - (((u32)hi << 8) + lo + 4);
            seq->curBULen[track] = len;
            b = *seq->curBUPtr[track]++;
            seq->curBULen[track]--;
        }
    }
    return b;
}

static u32 read_number(ALCSeq *seq, u32 track) {
    u32 v = next_byte(seq, track), c;

    if (v & 0x80) {
        v &= 0x7F;
        do {
            c = next_byte(seq, track);
            v = (v << 7) + (c & 0x7F);
        } while (c & 0x80);
    }
    return v;
}

void alCSeqNew(ALCSeq *seq, u8 *ptr) {
    u32 i;

    seq->base = (ALCMidiHdr *)ptr;
    seq->validTracks = 0;
    seq->lastDeltaTicks = 0;
    seq->lastTicks = 0;
    seq->deltaFlag = 1;
    for (i = 0; i < 16; i++) {
        u32 off = seq->base->trackOffset[i];
        seq->lastStatus[i] = 0;
        seq->curBUPtr[i] = NULL;
        seq->curBULen[i] = 0;
        if (off != 0) {
            seq->validTracks |= 1 << i;
            seq->curLoc[i] = ptr + off;
            seq->evtDeltaTicks[i] = read_number(seq, i);
        } else {
            seq->curLoc[i] = NULL;
        }
    }
    seq->qnpt = 1.0 / (f32)seq->base->division;
}

/* take the last event's delta off every track (once), and return the next
   one's: the smallest, and its track */
static u32 next_delta(ALCSeq *seq, u32 *track) {
    u32 i, soonest = 0xFFFFFFFF, last = seq->lastDeltaTicks;

    for (i = 0; i < 16; i++) {
        if (!(seq->validTracks >> i & 1))
            continue;
        if (seq->deltaFlag)
            seq->evtDeltaTicks[i] -= last;
        if (seq->evtDeltaTicks[i] < soonest) {
            soonest = seq->evtDeltaTicks[i];
            *track = i;
        }
    }
    return soonest;
}

static void read_event(ALCSeq *seq, u32 track, ALEvent *evt) {
    u8 status = next_byte(seq, track);

    if (status == AL_MIDI_Meta) {
        u8 type = next_byte(seq, track);
        if (type == AL_MIDI_META_TEMPO) {
            evt->type = AL_TEMPO_EVT;
            evt->msg.tempo.status = status;
            evt->msg.tempo.type = type;
            evt->msg.tempo.byte1 = next_byte(seq, track);
            evt->msg.tempo.byte2 = next_byte(seq, track);
            evt->msg.tempo.byte3 = next_byte(seq, track);
            seq->lastStatus[track] = 0;
        } else if (type == AL_MIDI_META_EOT) {
            seq->validTracks ^= 1 << track;
            evt->type = seq->validTracks != 0 ? AL_TRACK_END : AL_SEQ_END_EVT;
        } else if (type == AL_CMIDI_LOOPSTART_CODE) {
            next_byte(seq, track);
            next_byte(seq, track);
            seq->lastStatus[track] = 0;
            evt->type = AL_CSP_LOOPSTART;
        } else if (type == AL_CMIDI_LOOPEND_CODE) {
            /* its count, the count left, and the distance back to the loop's
               start (from the end of the event); the count left is reset
               when it runs out, so the loop plays again next time through */
            u8 *p = seq->curLoc[track];
            u8 count = p[0], left = p[1];
            if (left == 0) {
                p[1] = count;
                seq->curLoc[track] = p + 6;
            } else {
                u32 back;
                if (left != 0xFF)
                    p[1] = left - 1;
                back = ((u32)p[2] << 24) + ((u32)p[3] << 16) + ((u32)p[4] << 8) + p[5];
                seq->curLoc[track] = p + 6 - back;
            }
            seq->lastStatus[track] = 0;
            evt->type = AL_CSP_LOOPEND;
        }
        return;
    }

    evt->type = AL_SEQ_MIDI_EVT;
    if (status & 0x80) {
        evt->msg.midi.status = status;
        evt->msg.midi.byte1 = next_byte(seq, track);
        seq->lastStatus[track] = status;
    } else {
        /* running status */
        evt->msg.midi.status = seq->lastStatus[track];
        evt->msg.midi.byte1 = status;
    }
    switch (evt->msg.midi.status & 0xF0) {
    case AL_MIDI_ProgramChange:
    case AL_MIDI_ChannelPressure:
        evt->msg.midi.byte2 = 0;
        break;
    default:
        evt->msg.midi.byte2 = next_byte(seq, track);
        if ((evt->msg.midi.status & 0xF0) == AL_MIDI_NoteOn)
            evt->msg.midi.duration = read_number(seq, track);
        break;
    }
}

void alCSeqNextEvent(ALCSeq *seq, ALEvent *evt) {
    u32 track = 0;
    u32 delta = next_delta(seq, &track);

    read_event(seq, track, evt);
    evt->msg.midi.ticks = delta;
    seq->lastTicks += delta;
    seq->lastDeltaTicks = delta;
    if (evt->type != AL_TRACK_END)
        seq->evtDeltaTicks[track] += read_number(seq, track);
    seq->deltaFlag = 1;
}

/* the ticks to the next event, without reading it; FALSE at the end */
char __alCSeqNextDelta(ALCSeq *seq, s32 *ticks) {
    u32 track;

    if (seq->validTracks == 0)
        return FALSE;
    *ticks = next_delta(seq, &track);
    seq->deltaFlag = 0;
    return TRUE;
}

f32 alCSeqTicksToSec(ALCSeq *seq, s32 ticks, u32 tempo) {
    return (f32)(((f32)ticks * (f32)tempo) / ((f32)seq->base->division * 1000000.0));
}

u32 alCSeqSecToTicks(ALCSeq *seq, f32 sec, u32 tempo) {
    return (u32)(sec * 1000000.0 * seq->base->division / tempo);
}

s32 alCSeqGetTicks(ALCSeq *seq) {
    return seq->lastTicks;
}

/* a marker is the reading state: the same fields but base, qnpt and the
   delta flag */
#define COPY_STATE(to, from)                                       \
    do {                                                           \
        s32 i_;                                                    \
        (to)->validTracks = (from)->validTracks;                   \
        (to)->lastTicks = (from)->lastTicks;                       \
        (to)->lastDeltaTicks = (from)->lastDeltaTicks;             \
        for (i_ = 0; i_ < 16; i_++) {                              \
            (to)->curLoc[i_] = (from)->curLoc[i_];                 \
            (to)->curBUPtr[i_] = (from)->curBUPtr[i_];             \
            (to)->curBULen[i_] = (from)->curBULen[i_];             \
            (to)->lastStatus[i_] = (from)->lastStatus[i_];         \
            (to)->evtDeltaTicks[i_] = (from)->evtDeltaTicks[i_];   \
        }                                                          \
    } while (0)

void alCSeqNewMarker(ALCSeq *seq, ALCSeqMarker *m, u32 ticks) {
    ALCSeq scan;
    ALEvent evt;

    alCSeqNew(&scan, (u8 *)seq->base);
    do {
        COPY_STATE(m, &scan);
        alCSeqNextEvent(&scan, &evt);
    } while (evt.type != AL_SEQ_END_EVT && scan.lastTicks < ticks);
}

void alCSeqSetLoc(ALCSeq *seq, ALCSeqMarker *m) {
    COPY_STATE(seq, m);
}

void alCSeqGetLoc(ALCSeq *seq, ALCSeqMarker *m) {
    COPY_STATE(m, seq);
}
