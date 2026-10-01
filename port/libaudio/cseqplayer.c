/*
 * The port's libaudio: the compact sequence player.
 *
 * A client of the synthesizer.  Its handler runs its event queue: the
 * game's requests (play, stop, sequence, bank, volume, tempo) arrive as
 * events, as do the sequence's own (one is queued for the next MIDI event's
 * time), the notes' ends (a compact note on carries its length), their
 * envelopes' decay and release, and a "frame" event that keeps it running
 * when nothing else is queued.  Each note is a voice state holding its
 * synthesizer voice.
 */
#include "audio_private.h"

#define KILL_TIME 50000         /* stopping: notes ending later are cut to this */

char __alCSeqNextDelta(ALCSeq *seq, s32 *ticks);

static void post(ALCSPlayer *p, ALEvent *evt, ALMicroTime delta) {
    alEvtqPostEvent(&p->evtq, evt, delta);
}

static void post_type(ALCSPlayer *p, s16 type, ALMicroTime delta) {
    ALEvent evt;

    evt.type = type;
    post(p, &evt, delta);
}

/* ---- channels ------------------------------------------------------------------------ */

static void chan_defaults(ALChanState *c) {
    c->fxId = AL_FX_NONE;
    c->fxmix = AL_DEFAULT_FXMIX;
    c->pan = AL_PAN_CENTER;
    c->vol = AL_VOL_FULL;
    c->priority = AL_DEFAULT_PRIORITY;
    c->sustain = 0;
    c->bendRange = 200;
    c->pitchBend = 1.0f;
}

static void chan_instrument(ALChanState *c, ALInstrument *inst) {
    c->instrument = inst;
    c->pan = inst->pan;
    c->vol = inst->volume;
    c->priority = inst->priority;
    c->bendRange = inst->bendRange;
}

/* every channel to the bank's first instrument; channel 9 to its
   percussion, if any (and the channel state just past the last one is
   reset then, as the reference does) */
static void use_bank(ALCSPlayer *p, ALBank *b) {
    ALInstrument *first = NULL;
    s32 i;

    for (i = 0; first == NULL; i++)
        first = b->instArray[i];
    for (i = 0; i < p->maxChannels; i++) {
        chan_defaults(&p->chanState[i]);
        chan_instrument(&p->chanState[i], first);
    }
    if (b->percussion != NULL) {
        chan_defaults(&p->chanState[i]);
        chan_instrument(&p->chanState[9], b->percussion);
    }
}

/* the microseconds a tick lasts at this tempo (microseconds a quarter note) */
static void set_tempo(ALCSPlayer *p, f32 tempo) {
    if (p->target != NULL)
        p->uspt = (s32)((f32)tempo * p->target->qnpt);
    else
        p->uspt = 488;
}

/* ---- notes ------------------------------------------------------------------------------ */

static ALVoiceState *note_new(ALCSPlayer *p, u8 key, u8 vel, u8 chan) {
    ALVoiceState *vs = p->vFreeList;

    if (vs != NULL) {
        p->vFreeList = vs->next;
        vs->next = NULL;
        if (p->vAllocHead == NULL)
            p->vAllocHead = vs;
        else
            p->vAllocTail->next = vs;
        p->vAllocTail = vs;
        vs->channel = chan;
        vs->key = key;
        vs->velocity = vel;
        vs->voice.clientPrivate = vs;
    }
    return vs;
}

static void note_free(ALCSPlayer *p, ALVoice *voice) {
    ALVoiceState *vs, *prev = NULL;

    for (vs = p->vAllocHead; vs != NULL; prev = vs, vs = vs->next) {
        if (&vs->voice != voice)
            continue;
        if (prev != NULL)
            prev->next = vs->next;
        else
            p->vAllocHead = vs->next;
        if (vs == p->vAllocTail)
            p->vAllocTail = prev;
        vs->next = p->vFreeList;
        p->vFreeList = vs;
        return;
    }
}

/* the note playing this key on this channel, not yet released */
static ALVoiceState *note_find(ALCSPlayer *p, u8 key, u8 chan) {
    ALVoiceState *vs;

    for (vs = p->vAllocHead; vs != NULL; vs = vs->next)
        if (vs->key == key && vs->channel == chan && vs->phase != AL_PHASE_RELEASE &&
            vs->phase != AL_PHASE_SUSTREL)
            return vs;
    return NULL;
}

/* the instrument's sound for this key and velocity: a binary search of its
   sounds, in key then velocity order */
static ALSound *find_sound(ALCSPlayer *p, u8 key, u8 vel, u8 chan) {
    ALInstrument *inst = p->chanState[chan].instrument;
    s32 lo = 1, hi = inst->soundCount;

    while (hi >= lo) {
        s32 mid = (lo + hi) / 2;
        ALKeyMap *km = inst->soundArray[mid - 1]->keyMap;
        if (key >= km->keyMin && key <= km->keyMax && vel >= km->velocityMin && vel <= km->velocityMax)
            return inst->soundArray[mid - 1];
        if (key < km->keyMin || (vel < km->velocityMin && key <= km->keyMax))
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    return NULL;
}

static s16 note_volume(ALCSPlayer *p, ALVoiceState *vs) {
    u32 note = (vs->tremelo * vs->velocity * vs->envGain) >> 6;
    u32 mix = (vs->sound->sampleVolume * p->vol * p->chanState[vs->channel].vol) >> 14;

    return (s16)((note * mix) >> 15);
}

static ALPan note_pan(ALCSPlayer *p, ALVoiceState *vs) {
    s32 pan = p->chanState[vs->channel].pan - AL_PAN_CENTER + vs->sound->samplePan;

    if (pan < AL_PAN_LEFT)
        pan = AL_PAN_LEFT;
    if (pan > AL_PAN_RIGHT)
        pan = AL_PAN_RIGHT;
    return (ALPan)pan;
}

/* what's left of the envelope's current segment, or a short ramp if it's over */
static ALMicroTime note_remaining(ALVoiceState *vs, ALMicroTime now) {
    s32 left = vs->envEndTime - now;

    return left >= 0 ? left : AL_GAIN_CHANGE_TIME;
}

/* drop the queued events of one type for this voice (the later ones keep
   their times) */
static void unqueue(ALCSPlayer *p, s16 type, ALVoice *voice) {
    ALLink *ln = p->evtq.allocList.next;

    while (ln != NULL) {
        ALEventListItem *item = (ALEventListItem *)ln;
        ALEventListItem *next = (ALEventListItem *)ln->next;
        if (item->evt.type == type && item->evt.msg.vol.voice == voice) {
            if (next != NULL)
                next->delta += item->delta;
            alUnlink(ln);
            alLink(ln, &p->evtq.freeList);
        }
        ln = next != NULL ? &next->node : NULL;
    }
}

/* the note to silence over t, then its end */
static void note_release(ALCSPlayer *p, ALVoice *voice, ALMicroTime t) {
    ALVoiceState *vs = voice->clientPrivate;
    ALEvent evt;

    if (vs->envPhase == AL_PHASE_ATTACK)
        unqueue(p, AL_SEQP_ENV_EVT, voice);     /* (its decay won't come) */
    vs->velocity = 0;
    vs->envPhase = AL_PHASE_RELEASE;
    vs->envGain = 0;
    vs->envEndTime = p->curTime + t;
    alSynSetPriority(p->drvr, voice, 0);        /* the first to be stolen */
    alSynSetVol(p->drvr, voice, 0, t);
    evt.type = AL_NOTE_END_EVT;
    evt.msg.note.voice = voice;
    post(p, &evt, t);
}

/* stopping: whether the note would sound past t; if so its end event goes */
static int note_outlasts(ALCSPlayer *p, ALVoice *voice, ALMicroTime t) {
    ALLink *ln = p->evtq.allocList.next;
    ALMicroTime when = 0;

    for (; ln != NULL; ln = ln->next) {
        ALEventListItem *item = (ALEventListItem *)ln;
        when += item->delta;
        if (item->evt.type != AL_NOTE_END_EVT || item->evt.msg.note.voice != voice)
            continue;
        if (when <= t)
            return 0;
        if (ln->next != NULL)
            ((ALEventListItem *)ln->next)->delta += item->delta;
        alUnlink(ln);
        alLink(ln, &p->evtq.freeList);
        return 1;
    }
    return 1;
}

/* a note's oscillators' events (the game gives the player none) */
static void note_stop_osc(ALCSPlayer *p, ALVoiceState *vs) {
    ALEventListItem *item = (ALEventListItem *)p->evtq.allocList.next;

    while (item != NULL) {
        ALEventListItem *next = (ALEventListItem *)item->node.next;
        s16 type = item->evt.type;
        if ((type == AL_TREM_OSC_EVT || type == AL_VIB_OSC_EVT) && item->evt.msg.osc.vs == vs) {
            p->stopOsc(item->evt.msg.osc.oscState);
            alUnlink(&item->node);
            if (next != NULL)
                next->delta += item->delta;
            alLink(&item->node, &p->evtq.freeList);
            vs->flags &= type == AL_TREM_OSC_EVT ? 0xFE : 0xFD;
            if (vs->flags == 0)
                return;
        }
        item = next;
    }
}

static void note_end(ALCSPlayer *p, ALVoiceState *vs) {
    ALVoice *voice = &vs->voice;

    alSynStopVoice(p->drvr, voice);
    alSynFreeVoice(p->drvr, voice);
    if (vs->flags)
        note_stop_osc(p, vs);
    note_free(p, voice);
}

/* ---- MIDI ------------------------------------------------------------------------------------- */

static void note_on(ALCSPlayer *p, ALMIDIEvent *midi, u8 chan, u8 key, u8 vel) {
    ALChanState *cs = &p->chanState[chan];
    ALVoiceConfig config;
    ALVoiceState *vs;
    ALSound *sound;
    ALInstrument *inst;
    ALEvent evt;
    ALMicroTime t;
    void *osc;
    f32 level;

    if (p->state != AL_PLAYING)
        return;
    sound = find_sound(p, key, vel, chan);
    if (sound == NULL)
        return;
    config.priority = cs->priority;
    config.fxBus = 0;
    config.unityPitch = 0;
    vs = note_new(p, key, vel, chan);
    if (vs == NULL)
        return;
    alSynAllocVoice(p->drvr, &vs->voice, &config);

    vs->sound = sound;
    vs->envPhase = AL_PHASE_ATTACK;
    vs->phase = cs->sustain > AL_SUSTAIN ? AL_PHASE_SUSTAIN : AL_PHASE_NOTEON;
    vs->pitch = alCents2Ratio((s16)((key - sound->keyMap->keyBase) * 100 + sound->keyMap->detune));
    vs->envGain = sound->envelope->attackVolume;
    vs->envEndTime = p->curTime + sound->envelope->attackTime;

    vs->flags = 0;
    inst = cs->instrument;
    level = (f32)AL_VOL_FULL;
    if (inst->tremType && p->initOsc != NULL) {
        t = p->initOsc(&osc, &level, inst->tremType, inst->tremRate, inst->tremDepth, inst->tremDelay);
        if (t != 0) {
            evt.type = AL_TREM_OSC_EVT;
            evt.msg.osc.vs = vs;
            evt.msg.osc.oscState = osc;
            post(p, &evt, t);
            vs->flags |= 1;
        }
    }
    vs->tremelo = (u8)level;
    level = 1.0f;
    if (inst->vibType && p->initOsc != NULL) {
        t = p->initOsc(&osc, &level, inst->vibType, inst->vibRate, inst->vibDepth, inst->vibDelay);
        if (t != 0) {
            evt.type = AL_VIB_OSC_EVT;
            evt.msg.osc.vs = vs;
            evt.msg.osc.oscState = osc;
            evt.msg.osc.chan = chan;
            post(p, &evt, t);
            vs->flags |= 2;
        }
    }
    vs->vibrato = level;

    t = sound->envelope->attackTime;
    alSynStartVoiceParams(p->drvr, &vs->voice, sound->wavetable, vs->pitch * cs->pitchBend * vs->vibrato,
                          note_volume(p, vs), note_pan(p, vs), cs->fxmix, t);

    /* the attack's end starts the decay; the note's length, its release */
    evt.type = AL_SEQP_ENV_EVT;
    evt.msg.vol.voice = &vs->voice;
    evt.msg.vol.vol = sound->envelope->decayVolume;
    evt.msg.vol.delta = sound->envelope->decayTime;
    post(p, &evt, t);
    if (midi->duration != 0) {
        evt.type = AL_CSP_NOTEOFF_EVT;
        evt.msg.midi.status = chan | AL_MIDI_NoteOff;
        evt.msg.midi.byte1 = key;
        evt.msg.midi.byte2 = 0;
        post(p, &evt, p->uspt * midi->duration);
    }
}

static void note_off(ALCSPlayer *p, u8 chan, u8 key) {
    ALVoiceState *vs = note_find(p, key, chan);

    if (vs == NULL)
        return;
    if (vs->phase == AL_PHASE_SUSTAIN) {
        vs->phase = AL_PHASE_SUSTREL;       /* released with the pedal */
    } else {
        vs->phase = AL_PHASE_RELEASE;
        note_release(p, &vs->voice, vs->sound->envelope->releaseTime);
    }
}

static void controller(ALCSPlayer *p, u8 chan, u8 ctrl, u8 value) {
    ALChanState *cs = &p->chanState[chan];
    ALVoiceState *vs;

    switch (ctrl) {
    case AL_MIDI_PAN_CTRL:
        cs->pan = value;
        for (vs = p->vAllocHead; vs != NULL; vs = vs->next)
            if (vs->channel == chan)
                alSynSetPan(p->drvr, &vs->voice, note_pan(p, vs));
        break;
    case AL_MIDI_VOLUME_CTRL:
        cs->vol = value;
        for (vs = p->vAllocHead; vs != NULL; vs = vs->next)
            if (vs->channel == chan && vs->envPhase != AL_PHASE_RELEASE)
                alSynSetVol(p->drvr, &vs->voice, note_volume(p, vs), note_remaining(vs, p->curTime));
        break;
    case AL_MIDI_PRIORITY_CTRL:
        cs->priority = value;       /* (for the notes to come) */
        break;
    case AL_MIDI_SUSTAIN_CTRL:
        cs->sustain = value;
        for (vs = p->vAllocHead; vs != NULL; vs = vs->next) {
            if (vs->channel != chan || vs->phase == AL_PHASE_RELEASE)
                continue;
            if (value > AL_SUSTAIN) {
                if (vs->phase == AL_PHASE_NOTEON)
                    vs->phase = AL_PHASE_SUSTAIN;
            } else if (vs->phase == AL_PHASE_SUSTAIN) {
                vs->phase = AL_PHASE_NOTEON;
            } else if (vs->phase == AL_PHASE_SUSTREL) {
                vs->phase = AL_PHASE_RELEASE;
                note_release(p, &vs->voice, vs->sound->envelope->releaseTime);
            }
        }
        break;
    case AL_MIDI_FX1_CTRL:
        cs->fxmix = value;
        for (vs = p->vAllocHead; vs != NULL; vs = vs->next)
            if (vs->channel == chan)
                alSynSetFXMix(p->drvr, &vs->voice, value);
        break;
    }
}

static void midi_event(ALCSPlayer *p, ALEvent *event) {
    ALMIDIEvent *midi = &event->msg.midi;
    u8 chan = midi->status & AL_MIDI_ChannelMask;
    u8 b1 = midi->byte1, b2 = midi->byte2;
    ALVoiceState *vs;

    switch (midi->status & AL_MIDI_StatusMask) {
    case AL_MIDI_NoteOn:
        if (b2 != 0) {
            note_on(p, midi, chan, b1, b2);
            break;
        }
        note_off(p, chan, b1);          /* (velocity 0) */
        break;
    case AL_MIDI_NoteOff:
        note_off(p, chan, b1);
        break;
    case AL_MIDI_PolyKeyPressure:       /* aftertouch: the note's volume */
        vs = note_find(p, b1, chan);
        if (vs == NULL)
            break;
        vs->velocity = b2;
        alSynSetVol(p->drvr, &vs->voice, note_volume(p, vs), note_remaining(vs, p->curTime));
        break;
    case AL_MIDI_ChannelPressure:       /* the channel's notes' volume */
        for (vs = p->vAllocHead; vs != NULL; vs = vs->next) {
            if (vs->channel != chan)
                continue;
            vs->velocity = b1;
            alSynSetVol(p->drvr, &vs->voice, note_volume(p, vs), note_remaining(vs, p->curTime));
        }
        break;
    case AL_MIDI_ControlChange:
        controller(p, chan, b1, b2);
        break;
    case AL_MIDI_ProgramChange:
        if (b1 < p->bank->instCount)
            chan_instrument(&p->chanState[chan], p->bank->instArray[b1]);
        break;
    case AL_MIDI_PitchBendChange: {
        /* 14 bits about 8192, over the channel's range in cents */
        s32 bend = ((b2 << 7) + b1) - 8192;
        f32 ratio = alCents2Ratio((p->chanState[chan].bendRange * bend) / 8192);
        p->chanState[chan].pitchBend = ratio;
        for (vs = p->vAllocHead; vs != NULL; vs = vs->next)
            if (vs->channel == chan)
                alSynSetPitch(p->drvr, &vs->voice, vs->pitch * ratio * vs->vibrato);
        break;
    }
    }
}

/* a tempo change: the queued note ends (whose times are in ticks) are moved
   to the new tempo */
static void tempo_event(ALCSPlayer *p, ALEvent *event) {
    ALTempoEvent *te = &event->msg.tempo;
    ALEventListItem *item, *next, *moved = NULL;
    ALMicroTime at = 0, here;
    s32 old_uspt;
    s32 tempo;

    if (te->status != AL_MIDI_Meta || te->type != AL_MIDI_META_TEMPO)
        return;
    old_uspt = p->uspt;
    tempo = (te->byte1 << 16) | (te->byte2 << 8) | te->byte3;
    set_tempo(p, (f32)tempo);

    /* take them out (each with its time from now), the others keeping theirs */
    for (item = (ALEventListItem *)p->evtq.allocList.next; item != NULL; item = next) {
        at += item->delta;
        next = (ALEventListItem *)item->node.next;
        if (item->evt.type != AL_CSP_NOTEOFF_EVT)
            continue;
        alUnlink(&item->node);
        if (moved != NULL) {
            alLink(&item->node, &moved->node);
        } else {
            item->node.next = item->node.prev = NULL;
            moved = item;
        }
        here = at;
        if (next != NULL) {
            at -= item->delta;
            next->delta += item->delta;
        }
        item->delta = here;
    }
    /* and back in, at their ticks at the new tempo */
    for (item = moved; item != NULL; item = next) {
        ALLink *after = &p->evtq.allocList;
        OSIntMask mask;
        u32 ticks;
        next = (ALEventListItem *)item->node.next;
        ticks = item->delta / old_uspt;
        item->delta = ticks * p->uspt;
        mask = osSetIntMask(OS_IM_NONE);
        for (;;) {
            ALEventListItem *n = (ALEventListItem *)after->next;
            if (n == NULL)
                break;
            if (item->delta < n->delta) {
                n->delta -= item->delta;
                break;
            }
            item->delta -= n->delta;
            after = &n->node;
        }
        alLink(&item->node, after);
        osSetIntMask(mask);
    }
}

/* ---- the sequence ------------------------------------------------------------------------------ */

/* queue a reference to the sequence's next event, at its time */
void __CSPPostNextSeqEvent(ALCSPlayer *p) {
    s32 ticks;

    if (p->state != AL_PLAYING || p->target == NULL)
        return;
    if (!__alCSeqNextDelta(p->target, &ticks))
        return;
    post_type(p, AL_SEQ_REF_EVT, ticks * p->uspt);
}

static void sequence_event(ALCSPlayer *p) {
    ALEvent evt;

    if (p->target == NULL)
        return;
    alCSeqNextEvent(p->target, &evt);
    switch (evt.type) {
    case AL_SEQ_MIDI_EVT:
        midi_event(p, &evt);
        __CSPPostNextSeqEvent(p);
        break;
    case AL_TEMPO_EVT:
        tempo_event(p, &evt);
        __CSPPostNextSeqEvent(p);
        break;
    case AL_SEQ_END_EVT:
        p->state = AL_STOPPING;
        evt.type = AL_SEQP_STOP_EVT;
        post(p, &evt, AL_EVTQ_END);
        break;
    case AL_TRACK_END:
    case AL_CSP_LOOPSTART:
    case AL_CSP_LOOPEND:
        __CSPPostNextSeqEvent(p);
        break;
    }
}

/* ---- the handler ----------------------------------------------------------------------------- */

static ALMicroTime handler(void *node) {
    ALCSPlayer *p = node;
    ALVoiceState *vs;
    ALVoice *voice;

    do {
        ALEvent *e = &p->nextEvent;
        switch (e->type) {
        case AL_SEQ_REF_EVT:
            sequence_event(p);
            break;
        case AL_SEQP_API_EVT:
            /* the idle tick, which keeps the handler called */
            post_type(p, AL_SEQP_API_EVT, p->frameTime);
            break;
        case AL_NOTE_END_EVT:
            note_end(p, e->msg.note.voice->clientPrivate);
            break;
        case AL_SEQP_ENV_EVT:
            /* the attack is over: the decay */
            voice = e->msg.vol.voice;
            vs = voice->clientPrivate;
            if (vs->envPhase == AL_PHASE_ATTACK)
                vs->envPhase = AL_PHASE_DECAY;
            vs->envEndTime = p->curTime + e->msg.vol.delta;
            vs->envGain = e->msg.vol.vol;
            alSynSetVol(p->drvr, voice, note_volume(p, vs), e->msg.vol.delta);
            break;
        case AL_TREM_OSC_EVT: {
            ALEvent evt;
            f32 level;
            ALMicroTime t;
            vs = e->msg.osc.vs;
            evt.msg.osc.oscState = e->msg.osc.oscState;
            t = p->updateOsc(evt.msg.osc.oscState, &level);
            vs->tremelo = (u8)level;
            alSynSetVol(p->drvr, &vs->voice, note_volume(p, vs), note_remaining(vs, p->curTime));
            evt.type = AL_TREM_OSC_EVT;
            evt.msg.osc.vs = vs;
            post(p, &evt, t);
            break;
        }
        case AL_VIB_OSC_EVT: {
            ALEvent evt;
            f32 level;
            ALMicroTime t;
            u8 chan = e->msg.osc.chan;
            vs = e->msg.osc.vs;
            evt.msg.osc.oscState = e->msg.osc.oscState;
            t = p->updateOsc(evt.msg.osc.oscState, &level);
            vs->vibrato = level;
            alSynSetPitch(p->drvr, &vs->voice, vs->pitch * vs->vibrato * p->chanState[chan].pitchBend);
            evt.type = AL_VIB_OSC_EVT;
            evt.msg.osc.vs = vs;
            evt.msg.osc.chan = chan;
            post(p, &evt, t);
            break;
        }
        case AL_SEQP_MIDI_EVT:
        case AL_CSP_NOTEOFF_EVT:
            midi_event(p, e);
            break;
        case AL_SEQP_META_EVT:
            tempo_event(p, e);
            break;
        case AL_SEQP_VOL_EVT:
            p->vol = e->msg.spvol.vol;
            for (vs = p->vAllocHead; vs != NULL; vs = vs->next)
                alSynSetVol(p->drvr, &vs->voice, note_volume(p, vs), note_remaining(vs, p->curTime));
            break;
        case AL_SEQP_PLAY_EVT:
            if (p->state != AL_PLAYING) {
                p->state = AL_PLAYING;
                __CSPPostNextSeqEvent(p);
            }
            break;
        case AL_SEQP_STOP_EVT:
            /* the notes still sounding are cut; the queue goes on */
            if (p->state == AL_STOPPING) {
                while ((vs = p->vAllocHead) != NULL)
                    note_end(p, vs);
                p->state = AL_STOPPED;
            }
            break;
        case AL_SEQP_STOPPING_EVT:
            /* no more of the sequence (nor its note ends, which come from its
               note ons); notes ending after KILL_TIME end then; stopped when
               the queue gets to the end */
            if (p->state == AL_PLAYING) {
                alEvtqFlushType(&p->evtq, AL_SEQ_REF_EVT);
                alEvtqFlushType(&p->evtq, AL_CSP_NOTEOFF_EVT);
                alEvtqFlushType(&p->evtq, AL_SEQP_MIDI_EVT);
                for (vs = p->vAllocHead; vs != NULL; vs = vs->next)
                    if (note_outlasts(p, &vs->voice, KILL_TIME))
                        note_release(p, &vs->voice, KILL_TIME);
                p->state = AL_STOPPING;
                post_type(p, AL_SEQP_STOP_EVT, AL_EVTQ_END);
            }
            break;
        case AL_SEQP_PRIORITY_EVT:
            p->chanState[e->msg.sppriority.chan].priority = e->msg.sppriority.priority;
            break;
        case AL_SEQP_SEQ_EVT:
            p->target = e->msg.spseq.seq;
            set_tempo(p, 500000.0);
            if (p->bank != NULL)
                use_bank(p, p->bank);
            break;
        case AL_SEQP_BANK_EVT:
            p->bank = e->msg.spbank.bank;
            use_bank(p, p->bank);
            break;
        }
        p->nextDelta = alEvtqNextEvent(&p->evtq, &p->nextEvent);
    } while (p->nextDelta == 0);
    p->curTime += p->nextDelta;
    return p->nextDelta;
}

/* ---- the game's side -------------------------------------------------------------------------- */

void alCSPNew(ALCSPlayer *p, ALSeqpConfig *c) {
    ALHeap *hp = c->heap;
    ALVoiceState *vs;
    ALEventListItem *items;
    s32 i;

    p->bank = NULL;
    p->target = NULL;
    p->drvr = &alGlobals->drvr;
    p->chanMask = 0xFF;
    p->uspt = 488;
    p->nextDelta = 0;
    p->state = AL_STOPPED;
    p->vol = 0x7FFF;
    p->frameTime = AL_USEC_PER_FRAME;
    p->curTime = 0;
    p->initOsc = c->initOsc;
    p->updateOsc = c->updateOsc;
    p->stopOsc = c->stopOsc;
    p->nextEvent.type = AL_SEQP_API_EVT;       /* starts the idle tick */

    p->maxChannels = c->maxChannels;
    p->chanState = alHeapAlloc(hp, c->maxChannels, BLOCK_CHANSTATE);
    for (i = 0; i < p->maxChannels; i++) {
        p->chanState[i].instrument = NULL;
        chan_defaults(&p->chanState[i]);
    }

    vs = alHeapAlloc(hp, c->maxVoices, BLOCK_VOICESTATE);
    p->vFreeList = NULL;
    for (i = 0; i < c->maxVoices; i++) {
        vs[i].next = p->vFreeList;
        p->vFreeList = &vs[i];
    }
    p->vAllocHead = p->vAllocTail = NULL;

    items = alHeapAlloc(hp, c->maxEvents, BLOCK_EVENT);
    alEvtqNew(&p->evtq, items, c->maxEvents);

    p->node.next = NULL;
    p->node.handler = handler;
    p->node.clientData = p;
    alSynAddPlayer(&alGlobals->drvr, &p->node);
}

void alCSPPlay(ALCSPlayer *p) {
    post_type(p, AL_SEQP_PLAY_EVT, 0);
}

void alCSPStop(ALCSPlayer *p) {
    post_type(p, AL_SEQP_STOPPING_EVT, 0);
}

void alCSPSetSeq(ALCSPlayer *p, ALCSeq *seq) {
    ALEvent evt;

    evt.type = AL_SEQP_SEQ_EVT;
    evt.msg.spseq.seq = seq;
    post(p, &evt, 0);
}

void alCSPSetBank(ALCSPlayer *p, ALBank *b) {
    ALEvent evt;

    evt.type = AL_SEQP_BANK_EVT;
    evt.msg.spbank.bank = b;
    post(p, &evt, 0);
}

void alCSPSetVol(ALCSPlayer *p, s16 vol) {
    ALEvent evt;

    evt.type = AL_SEQP_VOL_EVT;
    evt.msg.spvol.vol = vol;
    post(p, &evt, 0);
}

s16 alCSPGetVol(ALCSPlayer *p) {
    return p->vol;
}

void alCSPSetTempo(ALCSPlayer *p, s32 tempo) {
    ALEvent evt;

    evt.type = AL_SEQP_META_EVT;
    evt.msg.tempo.status = AL_MIDI_Meta;
    evt.msg.tempo.type = AL_MIDI_META_TEMPO;
    evt.msg.tempo.byte1 = (tempo & 0xFF0000) >> 16;
    evt.msg.tempo.byte2 = (tempo & 0xFF00) >> 8;
    evt.msg.tempo.byte3 = tempo & 0xFF;
    post(p, &evt, 0);
}

/* microseconds per quarter note; 0 with no sequence */
s32 alCSPGetTempo(ALCSPlayer *p) {
    if (p->target == NULL)
        return 0;
    return p->uspt / p->target->qnpt;
}

s32 alCSPGetState(ALCSPlayer *p) {
    return p->state;
}

/* the game's names for some of them (its symbols are by address) */
void func_802D81F0(ALCSPlayer *p) { alCSPPlay(p); }
void func_802D76C0(ALCSPlayer *p) { alCSPStop(p); }
void func_802D81B0(ALCSPlayer *p, ALCSeq *seq) { alCSPSetSeq(p, seq); }
void func_802D97E0(ALCSPlayer *p, ALBank *b) { alCSPSetBank(p, b); }
s32 func_802D4E10(ALCSPlayer *p) { return alCSPGetState(p); }
