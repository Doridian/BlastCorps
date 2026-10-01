/*
 * The port's libaudio: the synthesizer (the driver): its setup, its
 * clients, the physical voices and their updates, and each audio frame's
 * command list.
 */
#include "audio_private.h"

ALGlobals *alGlobals = NULL;

/* ---- the equal-power pan and send curve: round(32767 cos(i pi / 254)) -------- */

s16 al_eqpower[128];

static double cosine(double x) {
    /* its Taylor series, plenty for [0, pi/2] (the table is 0.002 from any
       rounding boundary) */
    double term = 1.0, sum = 1.0;
    int k;

    for (k = 1; k < 16; k++) {
        term *= -x * x / ((2 * k - 1) * (2 * k));
        sum += term;
    }
    return sum;
}

void al_eqpower_init(void) {
    int i;

    for (i = 0; i < 128; i++) {
        double v = 32767.0 * cosine(i * (3.14159265358979323846 / 254.0));
        al_eqpower[i] = (s16)(v + 0.5);
    }
}

/* ---- time ------------------------------------------------------------------------ */

/* microseconds to output samples, rounded to nearest (in the float and
   double steps the reference takes) */
static s32 usec_to_samples_exact(ALSynth *s, s32 usec) {
    f32 t = (f32)((f32)usec * (f32)s->outputRate / 1000000.0 + 0.5);

    return (s32)t;
}

/* ... and down to a multiple of 16, for a voice's updates */
s32 al_usec_to_samples(ALSynth *s, s32 usec) {
    return usec_to_samples_exact(s, usec) & ~15;
}

/* ---- updates ------------------------------------------------------------------ */

Update *al_update_alloc(ALSynth *s) {
    Update *u = (Update *)s->paramList;

    if (u != NULL) {
        s->paramList = (struct ALParam_s *)u->next;
        u->next = NULL;
    }
    return u;
}

void al_update_free(ALSynth *s, Update *u) {
    u->next = (Update *)s->paramList;
    s->paramList = (struct ALParam_s *)u;
}

/* a voice's update, at the time its client's handler runs, delayed while the
   voice is stolen (its old note ramps down first) */
static Update *voice_update(ALSynth *s, ALVoice *v, s16 kind) {
    Update *u = al_update_alloc(s);

    if (u != NULL) {
        u->at = s->paramSamples + v->pvoice->steal_delay;
        u->kind = kind;
        u->next = NULL;
    }
    return u;
}

/* ---- the synthesizer ------------------------------------------------------------- */

static void link_voice_list(ALLink *list) {
    list->next = list->prev = NULL;
}

void alSynNew(ALSynth *s, ALSynConfig *c) {
    ALHeap *hp = c->heap;
    OutStage *out;
    AuxBus *aux;
    MainBus *mainbus;
    PVoice *voices;
    Update *updates;
    s32 i;

    al_eqpower_init();

    s->head = NULL;
    s->numPVoices = c->maxPVoices;
    s->curSamples = 0;
    s->paramSamples = 0;
    s->outputRate = c->outputRate;
    s->maxOutSamples = MAX_SECTION;
    s->dma = (ALDMANew)c->dmaproc;

    /* the blocks in the reference's order */
    out = alHeapAlloc(hp, 1, BLOCK_SAVE);
    out->dram = 0;
    s->outputFilter = (struct ALFilter_s *)out;

    aux = alHeapAlloc(hp, 1, BLOCK_AUXBUS);
    s->auxBus = aux;
    s->maxAuxBusses = 1;
    aux->voices = alHeapAlloc(hp, c->maxPVoices, 4);
    aux->nvoices = 0;
    aux->maxvoices = c->maxPVoices;
    aux->fx.on = 0;

    mainbus = alHeapAlloc(hp, 1, BLOCK_MAINBUS);
    s->mainBus = mainbus;
    mainbus->sources = alHeapAlloc(hp, c->maxPVoices, 4);
    mainbus->nsources = 1;      /* the reverb, or the aux bus straight */
    mainbus->sources[0] = aux;

    if (c->fxType != AL_FX_NONE)
        al_reverb_init(&aux->fx, c, hp);

    link_voice_list(&s->pFreeList);
    link_voice_list(&s->pLameList);
    link_voice_list(&s->pAllocList);
    voices = alHeapAlloc(hp, c->maxPVoices, BLOCK_VOICE);
    for (i = 0; i < c->maxPVoices; i++) {
        PVoice *pv = &voices[i];
        alLink(&pv->node, &s->pFreeList);
        pv->owner = NULL;
        al_voice_init(pv, s->dma, hp);
        aux->voices[aux->nvoices++] = pv;
    }

    /* the update pool, as a stack */
    updates = alHeapAlloc(hp, c->maxUpdates, BLOCK_UPDATE);
    s->paramList = NULL;
    for (i = 0; i < c->maxUpdates; i++)
        al_update_free(s, &updates[i]);

    s->heap = hp;
}

void alSynDelete(ALSynth *s) {
    s->head = NULL;
}

void alInit(ALGlobals *g, ALSynConfig *c) {
    if (alGlobals == NULL) {
        alGlobals = g;
        alSynNew(&g->drvr, c);
    }
}

void alClose(ALGlobals *g) {
    if (alGlobals != NULL) {
        alSynDelete(&g->drvr);
        alGlobals = NULL;
    }
}

void alSynAddPlayer(ALSynth *s, ALPlayer *client) {
    OSIntMask mask = osSetIntMask(OS_IM_NONE);

    client->samplesLeft = s->curSamples;
    client->next = s->head;
    s->head = client;
    osSetIntMask(mask);
}

/* the client due soonest (the first of those due together) */
static ALPlayer *next_client(ALSynth *s) {
    ALPlayer *cl, *due = NULL;
    s32 soonest = 0x7FFFFFFF;

    for (cl = s->head; cl != NULL; cl = cl->next) {
        if (cl->samplesLeft - s->curSamples < soonest) {
            soonest = cl->samplesLeft - s->curSamples;
            due = cl;
        }
    }
    return due;
}

/* the free list takes this frame's released voices only now: their last
   samples are in this frame's list */
static void recycle_voices(ALSynth *s) {
    ALLink *ln;

    while ((ln = s->pLameList.next) != NULL) {
        alUnlink(ln);
        alLink(ln, &s->pFreeList);
    }
}

Acmd *alAudioFrame(Acmd *cmdList, s32 *cmdLen, s16 *outBuf, s32 outLen) {
    ALSynth *s = &alGlobals->drvr;
    AuxBus *aux = s->auxBus;
    OutStage *out = (OutStage *)s->outputFilter;
    Acmd *cmd = cmdList;
    ALPlayer *client;
    s32 i;

    if (s->head == NULL) {
        *cmdLen = 0;
        return cmdList;
    }

    /* the clients whose time comes in this frame run now, at the start of
       it; what they change takes effect at their time, to 16 samples */
    for (;;) {
        client = next_client(s);
        s->paramSamples = client->samplesLeft;
        if (s->paramSamples - s->curSamples >= outLen)
            break;
        s->paramSamples &= ~15;
        client->samplesLeft += usec_to_samples_exact(s, client->handler(client));
    }

    while (outLen > 0) {
        s32 n = outLen < s->maxOutSamples ? outLen : s->maxOutSamples;

        aSegment(cmd++, 0, 0);
        out->dram = (s32)outBuf;

        /* the main bus: the aux bus's voices through the reverb */
        aClearBuffer(cmd++, DMEM_MAIN_L, n << 1);
        aClearBuffer(cmd++, DMEM_MAIN_R, n << 1);
        aClearBuffer(cmd++, DMEM_AUX_L, n << 1);
        aClearBuffer(cmd++, DMEM_AUX_R, n << 1);
        for (i = 0; i < aux->nvoices; i++)
            cmd = al_voice_mix(aux->voices[i], n, s->curSamples, cmd);
        if (aux->fx.on)
            cmd = al_reverb_run(&aux->fx, n, cmd);
        aSetBuffer(cmd++, 0, 0, 0, n << 1);
        aMix(cmd++, 0, 0x7FFF, DMEM_AUX_L, DMEM_MAIN_L);
        aMix(cmd++, 0, 0x7FFF, DMEM_AUX_R, DMEM_MAIN_R);

        /* out, interleaved */
        aSetBuffer(cmd++, 0, 0, 0, n << 1);
        aInterleave(cmd++, DMEM_MAIN_L, DMEM_MAIN_R);
        aSetBuffer(cmd++, 0, 0, 0, n << 2);
        aSaveBuffer(cmd++, out->dram);

        outLen -= n;
        outBuf += n << 1;
        s->curSamples += n;
    }
    *cmdLen = (s32)(cmd - cmdList);
    recycle_voices(s);
    return cmd;
}

/* ---- voices ------------------------------------------------------------------- */

/* a physical voice: a released one first, then a free one, else the one
   playing at the lowest priority no higher than the request's (the last of
   them in the list) that isn't being stolen already; returns whether it
   was stolen */
static s32 take_voice(ALSynth *s, PVoice **got, s16 priority) {
    ALLink *ln;

    if ((ln = s->pLameList.next) != NULL || (ln = s->pFreeList.next) != NULL) {
        *got = (PVoice *)ln;
        alUnlink(ln);
        alLink(ln, &s->pAllocList);
        return 0;
    }
    {
        s32 stolen = 0;
        for (ln = s->pAllocList.next; ln != NULL; ln = ln->next) {
            PVoice *pv = (PVoice *)ln;
            if (pv->owner->priority <= priority && pv->steal_delay == 0) {
                *got = pv;
                priority = pv->owner->priority;
                stolen = 1;
            }
        }
        return stolen;
    }
}

s32 alSynAllocVoice(ALSynth *s, ALVoice *v, ALVoiceConfig *vc) {
    PVoice *pv = NULL;
    s32 stolen;

    v->priority = vc->priority;
    v->unityPitch = vc->unityPitch;
    v->table = NULL;
    v->fxBus = vc->fxBus;
    v->state = AL_STOPPED;
    v->pvoice = NULL;
    stolen = take_voice(s, &pv, vc->priority);
    if (pv == NULL)
        return 0;

    if (stolen) {
        /* its old note ramps down over 448 samples and stops at 512; the
           new one's updates wait for that */
        Update *u;
        pv->steal_delay = 512;
        pv->owner->pvoice = NULL;
        u = al_update_alloc(s);
        u->at = s->paramSamples;
        u->kind = UPD_VOLUME;
        u->a.i = 0;
        u->b.ramp = pv->steal_delay - 64;
        al_voice_queue(pv, u);
        u = al_update_alloc(s);
        if (u != NULL) {
            u->at = s->paramSamples + pv->steal_delay;
            u->kind = UPD_STOP;
            u->next = NULL;
            al_voice_queue(pv, u);
        }
    } else {
        pv->steal_delay = 0;
    }
    pv->owner = v;
    v->pvoice = pv;
    return 1;
}

/* the voice to the released list: the free list's at the end of the frame */
void al_voice_release(ALSynth *s, PVoice *pv) {
    alUnlink(&pv->node);
    alLink(&pv->node, &s->pLameList);
}

void alSynFreeVoice(ALSynth *s, ALVoice *v) {
    PVoice *pv = v->pvoice;

    if (pv == NULL)
        return;
    if (pv->steal_delay != 0) {
        /* (it's freed once its old note has stopped) */
        Update *u = al_update_alloc(s);
        if (u == NULL)
            return;
        u->at = s->paramSamples + pv->steal_delay;
        u->kind = UPD_FREE_VOICE;
        u->a.p = pv;
        al_voice_queue(pv, u);
    } else {
        al_voice_release(s, pv);
    }
    v->pvoice = NULL;
}

void alSynStartVoice(ALSynth *s, ALVoice *v, ALWaveTable *w) {
    Update *u;

    if (v->pvoice == NULL || (u = voice_update(s, v, UPD_START)) == NULL)
        return;
    u->wave = w;
    u->unity = v->unityPitch;
    al_voice_queue(v->pvoice, u);
}

void alSynStartVoiceParams(ALSynth *s, ALVoice *v, ALWaveTable *w, f32 pitch, s16 vol, ALPan pan,
                           u8 fxmix, ALMicroTime t) {
    Update *u;

    if (v->pvoice == NULL || (u = voice_update(s, v, UPD_START_PARAMS)) == NULL)
        return;
    u->unity = v->unityPitch;
    u->b.s.pan = pan;
    u->b.s.volume = vol;
    u->b.s.fxmix = fxmix;
    u->a.f = pitch;
    u->ramp = al_usec_to_samples(s, t);
    u->wave = w;
    al_voice_queue(v->pvoice, u);
}

void alSynStopVoice(ALSynth *s, ALVoice *v) {
    Update *u;

    if (v->pvoice == NULL || (u = voice_update(s, v, UPD_STOP)) == NULL)
        return;
    al_voice_queue(v->pvoice, u);
}

void alSynSetVol(ALSynth *s, ALVoice *v, s16 vol, ALMicroTime t) {
    Update *u;

    if (v->pvoice == NULL || (u = voice_update(s, v, UPD_VOLUME)) == NULL)
        return;
    u->a.i = vol;
    u->b.ramp = al_usec_to_samples(s, t);
    al_voice_queue(v->pvoice, u);
}

void alSynSetPan(ALSynth *s, ALVoice *v, ALPan pan) {
    Update *u;

    if (v->pvoice == NULL || (u = voice_update(s, v, UPD_PAN)) == NULL)
        return;
    u->a.i = pan;
    al_voice_queue(v->pvoice, u);
}

void alSynSetPitch(ALSynth *s, ALVoice *v, f32 ratio) {
    Update *u;

    if (v->pvoice == NULL || (u = voice_update(s, v, UPD_PITCH)) == NULL)
        return;
    u->a.f = ratio;
    al_voice_queue(v->pvoice, u);
}

void alSynSetFXMix(ALSynth *s, ALVoice *v, u8 fxmix) {
    Update *u;

    if (v->pvoice == NULL || (u = voice_update(s, v, UPD_FXMIX)) == NULL)
        return;
    u->a.i = fxmix;
    al_voice_queue(v->pvoice, u);
}

void alSynSetPriority(ALSynth *s, ALVoice *v, s16 priority) {
    v->priority = priority;
}
