/*
 * The port's libaudio: the heap, lists, event queues and the bank and
 * sequence bank files.
 */
#include "audio_private.h"

/* ---- the heap ------------------------------------------------------------------ */

void alHeapInit(ALHeap *hp, u8 *base, s32 len) {
    s32 misalign = (s32)base & 15;

    /* blocks start on 16 bytes (the RSP's DMA); the length isn't reduced */
    hp->base = misalign != 0 ? base + (16 - misalign) : base;
    hp->cur = hp->base;
    hp->len = len;
    hp->count = 0;
}

void *alHeapDBAlloc(u8 *file, s32 line, ALHeap *hp, s32 num, s32 size) {
    s32 bytes = (num * size + 15) & ~15;
    u8 *block;

    if (hp->cur + bytes > hp->base + hp->len)
        return NULL;
    block = hp->cur;
    hp->cur = block + bytes;
    return block;
}

/* ---- lists ------------------------------------------------------------------------ */

void alLink(ALLink *ln, ALLink *after) {
    ALLink *next = after->next;

    ln->next = next;
    ln->prev = after;
    if (next != NULL)
        next->prev = ln;
    after->next = ln;
}

void alUnlink(ALLink *ln) {
    if (ln->next != NULL)
        ln->next->prev = ln->prev;
    if (ln->prev != NULL)
        ln->prev->next = ln->next;
}

void alCopy(void *src, void *dest, s32 len) {
    u8 *s = src, *d = dest;

    while (len-- > 0)
        *d++ = *s++;
}

/* ---- event queues: by time, each item's delta counted from the one before ---- */

void alEvtqNew(ALEventQueue *q, ALEventListItem *items, s32 n) {
    s32 i;

    q->eventCount = 0;
    q->allocList.next = q->allocList.prev = NULL;
    q->freeList.next = q->freeList.prev = NULL;
    for (i = 0; i < n; i++)
        alLink(&items[i].node, &q->freeList);
}

ALMicroTime alEvtqNextEvent(ALEventQueue *q, ALEvent *evt) {
    OSIntMask mask = osSetIntMask(OS_IM_NONE);
    ALEventListItem *first = (ALEventListItem *)q->allocList.next;
    ALMicroTime delta;

    if (first == NULL) {
        /* none: an overflow of the queue had left it without the events
           that post themselves again */
        evt->type = -1;
        delta = 0;
    } else {
        alUnlink(&first->node);
        *evt = first->evt;
        alLink(&first->node, &q->freeList);
        delta = first->delta;
    }
    osSetIntMask(mask);
    return delta;
}

/* put item after the first ones due no later than it (it has delta from the
   queue's start); AL_EVTQ_END goes last with a delta of 0 */
static void insert(ALEventQueue *q, ALEventListItem *item, ALMicroTime delta, s32 at_end) {
    ALLink *after = &q->allocList;

    for (;;) {
        ALEventListItem *next = (ALEventListItem *)after->next;
        if (next == NULL) {
            item->delta = at_end ? 0 : delta;
            break;
        }
        if (delta < next->delta) {
            item->delta = delta;
            next->delta -= delta;
            break;
        }
        delta -= next->delta;
        after = &next->node;
    }
    alLink(&item->node, after);
}

void alEvtqPostEvent(ALEventQueue *q, ALEvent *evt, ALMicroTime delta) {
    OSIntMask mask = osSetIntMask(OS_IM_NONE);
    ALEventListItem *item = (ALEventListItem *)q->freeList.next;

    if (item != NULL) {
        alUnlink(&item->node);
        item->evt = *evt;
        insert(q, item, delta, delta == AL_EVTQ_END);
    }
    osSetIntMask(mask);
}

void alEvtqFlush(ALEventQueue *q) {
    OSIntMask mask = osSetIntMask(OS_IM_NONE);
    ALLink *ln = q->allocList.next;

    while (ln != NULL) {
        ALLink *next = ln->next;
        alUnlink(ln);
        alLink(ln, &q->freeList);
        ln = next;
    }
    osSetIntMask(mask);
}

void alEvtqFlushType(ALEventQueue *q, s16 type) {
    OSIntMask mask = osSetIntMask(OS_IM_NONE);
    ALLink *ln = q->allocList.next;

    while (ln != NULL) {
        ALEventListItem *item = (ALEventListItem *)ln;
        ALEventListItem *next = (ALEventListItem *)ln->next;
        if (item->evt.type == type) {
            if (next != NULL)
                next->delta += item->delta;     /* keep the later ones' times */
            alUnlink(ln);
            alLink(ln, &q->freeList);
        }
        ln = next != NULL ? &next->node : NULL;
    }
    osSetIntMask(mask);
}

/* ---- pitch -------------------------------------------------------------------------- */

/* 2^(cents/1200), by squaring a cent's ratio */
f32 alCents2Ratio(s32 cents) {
    f32 step, ratio = 1.0f;

    if (cents < 0) {
        step = 0.9994225441f;
        cents = -cents;
    } else {
        step = 1.00057779f;
    }
    for (; cents != 0; cents >>= 1) {
        if (cents & 1)
            ratio *= step;
        step *= step;
    }
    return ratio;
}

/* ---- the bank and sequence bank files: offsets in the file become pointers ---- */

#define RELOC(p, base) ((p) = (void *)((u8 *)(p) + (s32)(base)))

void alSeqFileNew(ALSeqFile *f, u8 *base) {
    s32 i;

    for (i = 0; i < f->seqCount; i++)
        RELOC(f->seqArray[i].offset, base);
}

/* each object once: its flags say it's done (they're shared within a file) */
static void reloc_wave(ALWaveTable *w, u8 *file, u8 *rom) {
    if (w->flags)
        return;
    w->flags = 1;
    RELOC(w->base, rom);
    if (w->type == AL_ADPCM_WAVE) {
        RELOC(w->waveInfo.adpcmWave.book, file);
        if (w->waveInfo.adpcmWave.loop != NULL)
            RELOC(w->waveInfo.adpcmWave.loop, file);
    } else if (w->type == AL_RAW16_WAVE) {
        if (w->waveInfo.rawWave.loop != NULL)
            RELOC(w->waveInfo.rawWave.loop, file);
    }
}

static void reloc_sound(ALSound *s, u8 *file, u8 *rom) {
    if (s->flags)
        return;
    s->flags = 1;
    RELOC(s->envelope, file);
    RELOC(s->keyMap, file);
    RELOC(s->wavetable, file);
    reloc_wave(s->wavetable, file, rom);
}

static void reloc_inst(ALInstrument *in, u8 *file, u8 *rom) {
    s32 i;

    if (in->flags)
        return;
    in->flags = 1;
    for (i = 0; i < in->soundCount; i++) {
        RELOC(in->soundArray[i], file);
        reloc_sound(in->soundArray[i], file, rom);
    }
}

static void reloc_bank(ALBank *b, u8 *file, u8 *rom) {
    s32 i;

    if (b->flags)
        return;
    b->flags = 1;
    if (b->percussion != NULL) {
        RELOC(b->percussion, file);
        reloc_inst(b->percussion, file, rom);
    }
    for (i = 0; i < b->instCount; i++) {
        RELOC(b->instArray[i], file);
        if (b->instArray[i] != NULL)
            reloc_inst(b->instArray[i], file, rom);
    }
}

void alBnkfNew(ALBankFile *f, u8 *table) {
    s32 i;

    if (f->revision != AL_BANK_VERSION)
        return;
    for (i = 0; i < f->bankCount; i++) {
        RELOC(f->bankArray[i], f);
        if (f->bankArray[i] != NULL)
            reloc_bank(f->bankArray[i], (u8 *)f, table);
    }
}
