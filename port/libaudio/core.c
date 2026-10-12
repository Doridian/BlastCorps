/*
 * The port's libaudio: the heap, lists, event queues and the bank and
 * sequence bank files.
 */
#include "audio_private.h"
#include <stddef.h>

#include "port.h"
#include "bankfile.h"

/* ---- the heap ------------------------------------------------------------------ */

void alHeapInit(ALHeap *hp, u8 *base, s32 len) {
    s32 misalign = (s32)((uintptr_t)base & 15);

    /* blocks start on 16 bytes (the RSP's DMA), and what that skips comes
       off the length.  (The reference keeps the length, so that its heap
       then ends past the buffer: never on the N64, whose buffer is
       16-aligned, but a 64-bit host's image can give it 8.) */
    if (misalign != 0) {
        base += 16 - misalign;
        len -= 16 - misalign;
    }
    hp->base = base;
    hp->cur = base;
    hp->len = len;
    hp->count = 0;
}

void *alHeapDBAlloc(u8 *file, s32 line, ALHeap *hp, s32 num, s32 size) {
    s32 bytes = (num * size + 15) & ~15;
    u8 *block;

    /* (the reference returns NULL, which the game doesn't test: it would
       write through it) */
    if (hp->cur + bytes > hp->base + hp->len)
        host_fatal("libaudio: the audio heap is full: %d bytes asked for, %d left", bytes,
                   (s32)(hp->base + hp->len - hp->cur));
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

/* ---- the sequence bank file: its offsets become ROM addresses, in place ---- */

void alSeqFileNew(ALSeqFile *f, u32 base) {
    s32 i;

    for (i = 0; i < f->seqCount; i++)
        f->seqArray[i].offset += base;
}

/* ---- bank files: parsed into objects (bankfile.h) --------------------------- */

/*
 * The objects live in a pool here, on the N64 side (their addresses are
 * the game's to hold: in the sound player's states, in the events it
 * queues), not in the audio heap, whose layout the command lists'
 * addresses depend on: the file stays in its heap block as the ROM has it.
 * An object is never bigger than its record with each 4-byte offset made
 * a pointer, plus a pointer's rounding; the smallest record has 8 bytes.
 * So AL_BANK_RECORD_BYTES of records (the bank files' bytes less their
 * leaves) need at most 3/8 of a pointer's size a byte, plus a bank list
 * for each file (one with no bank has a 4-byte record).  The game's two
 * files have 0x1D54 and 0x1C70 bytes of records (738 of them; the files
 * 0x4B08 and 0x4BB8 bytes, the same in all four versions), which take
 * 0x39C4 bytes of the pool with 4-byte pointers and 0x6598 with 8; a
 * small target can set the bound lower, a resource pack's bigger banks
 * higher.
 */
#ifndef AL_BANK_RECORD_BYTES
#define AL_BANK_RECORD_BYTES 0x8000
#endif
#ifndef AL_BANK_FILES
#define AL_BANK_FILES 4                 /* the game has two: the music's and the effects' */
#endif
#define AL_BANK_POOL_BYTES (AL_BANK_RECORD_BYTES / 8 * 3 * sizeof(void *) + AL_BANK_FILES * sizeof(ALBankFile))
#define AL_BANK_OBJECTS (AL_BANK_RECORD_BYTES / 12)     /* a mapped record has at least 12 */

static void *bank_pool[AL_BANK_POOL_BYTES / sizeof(void *)];
static u32 bank_pool_used;

/* the files parsed, the newest at an address winning */
static struct {
    ALBankFile *file;
    ALBankFile *parsed;
} bank_files[AL_BANK_FILES];

/* the file being parsed, and its records made so far (each once: they're
   shared, a wave table by sounds, a sound by instruments) */
static BankFileIn bank_in;
static u32 bank_table;
static struct {
    u32 off;                    /* the record in the file */
    void *obj;
} bank_made[AL_BANK_OBJECTS];
static u32 bank_nmade;

#ifdef PORT_AUDIO_ORACLE
/* the oracle's map (audio_log.h, ALOG_BANK_MAP): every object built, by
   its record's address (replay builds only: i386, addresses of 32 bits) */
#include "audio_log.h"
static AlogBankObj bank_map[AL_BANK_FILES * AL_BANK_OBJECTS];
static u32 bank_nmap;

u32 alog_bank_map(const AlogBankObj **objs) {
    *objs = bank_map;
    return bank_nmap;
}
#endif

static void bank_fail(const char *what, u32 off) {
    host_fatal("libaudio: the bank file at offset 0x%X: %s (the file has 0x%X bytes)", off, what,
               bank_in.len);
}

/* a record or leaf of size bytes at off, aligned to align */
static void bank_check(u32 off, u32 size, u32 align, const char *what) {
    if (off == 0 || off % align != 0 || off > bank_in.len || size > bank_in.len - off)
        bank_fail(what, off);
}

static void *bank_alloc(u32 size) {
    u32 n = (size + sizeof(void *) - 1) / sizeof(void *) * sizeof(void *);
    void *p;

    if (n > sizeof(bank_pool) - bank_pool_used)
        host_fatal("libaudio: the bank files' objects need more than the pool's 0x%X bytes "
                   "(AL_BANK_RECORD_BYTES)", (u32)sizeof(bank_pool));
    p = (u8 *)bank_pool + bank_pool_used;
    bank_pool_used += n;
    return p;
}

static void *bank_find(u32 off) {
    u32 i;

    for (i = 0; i < bank_nmade; i++)
        if (bank_made[i].off == off)
            return bank_made[i].obj;
    return NULL;
}

/* a new object for the record at off, zeroed */
static void *bank_new(u32 off, u32 size) {
    u8 *p = bank_alloc(size);
    u32 i;

    for (i = 0; i < size; i++)
        p[i] = 0;
    if (bank_nmade == AL_BANK_OBJECTS)
        host_fatal("libaudio: the bank file has more than %d records (AL_BANK_RECORD_BYTES)", AL_BANK_OBJECTS);
    bank_made[bank_nmade].off = off;
    bank_made[bank_nmade].obj = p;
    bank_nmade++;
#ifdef PORT_AUDIO_ORACLE
    if (bank_nmap < AL_BANK_FILES * AL_BANK_OBJECTS) {
        bank_map[bank_nmap].file = (u32)(uintptr_t)(bank_in.base + off);
        bank_map[bank_nmap].native = (u32)(uintptr_t)p;
        bank_nmap++;
    }
#endif
    return p;
}

/* a leaf (no offsets in it): where it is in the file */
static void *bank_leaf(u32 off, u32 size, u32 align, const char *what) {
    bank_check(off, size, align, what);
    return (u8 *)bank_in.base + off;
}

static ALWaveTable *bank_wave(u32 off) {
    ALWaveTable *w = bank_find(off);
    u32 loop, book;

    if (w != NULL)
        return w;
    bank_check(off, BF_WAVE_SIZE, 4, "a wave table out of the file");
    w = bank_new(off, sizeof(ALWaveTable));
    w->base = bf_u32(&bank_in, off + BF_WAVE_BASE) + bank_table;
    w->len = (s32)bf_u32(&bank_in, off + BF_WAVE_LEN);
    w->type = bf_u8(&bank_in, off + BF_WAVE_TYPE);
    w->flags = 1;
    loop = bf_u32(&bank_in, off + BF_WAVE_LOOP);
    if (w->type == AL_ADPCM_WAVE) {
        ALADPCMBook *b;
        u32 room;

        book = bf_u32(&bank_in, off + BF_WAVE_BOOK);
        b = bank_leaf(book, BF_BOOK_HEADER, 4, "an ADPCM book out of the file");
        room = (bank_in.len - book - BF_BOOK_HEADER) / BF_BOOK_ENTRY;
        if (b->order < 0 || b->npredictors < 0 ||
            (b->npredictors != 0 && (u32)b->order > room / (u32)b->npredictors))
            bank_fail("an ADPCM book past the file's end", book);
        w->waveInfo.adpcmWave.book = b;
        if (loop != 0)
            w->waveInfo.adpcmWave.loop = bank_leaf(loop, BF_ADPCMLOOP_SIZE, 4, "an ADPCM loop out of the file");
    } else if (w->type == AL_RAW16_WAVE) {
        if (loop != 0)
            w->waveInfo.rawWave.loop = bank_leaf(loop, BF_RAWLOOP_SIZE, 4, "a loop out of the file");
    }
    return w;
}

static ALSound *bank_sound(u32 off) {
    ALSound *s = bank_find(off);

    if (s != NULL)
        return s;
    bank_check(off, BF_SOUND_SIZE, 4, "a sound out of the file");
    s = bank_new(off, sizeof(ALSound));
    s->envelope = bank_leaf(bf_u32(&bank_in, off + BF_SOUND_ENVELOPE), BF_ENVELOPE_SIZE, 4,
                            "an envelope out of the file");
    s->keyMap = bank_leaf(bf_u32(&bank_in, off + BF_SOUND_KEYMAP), BF_KEYMAP_SIZE, 1,
                          "a key map out of the file");
    s->wavetable = bank_wave(bf_u32(&bank_in, off + BF_SOUND_WAVETABLE));
    s->samplePan = bf_u8(&bank_in, off + BF_SOUND_PAN);
    s->sampleVolume = bf_u8(&bank_in, off + BF_SOUND_VOLUME);
    s->flags = 1;
    return s;
}

static ALInstrument *bank_inst(u32 off) {
    ALInstrument *in = bank_find(off);
    s32 i, n;

    if (in != NULL)
        return in;
    bank_check(off, BF_INST_SOUNDS, 4, "an instrument out of the file");
    n = (s16)bf_u16(&bank_in, off + BF_INST_COUNT);
    if (n < 0 || (u32)n > (bank_in.len - off - BF_INST_SOUNDS) / 4)
        bank_fail("an instrument's sounds past the file's end", off);
    in = bank_new(off, MAX(sizeof(ALInstrument), offsetof(ALInstrument, soundArray) + n * sizeof(ALSound *)));
    in->volume = bf_u8(&bank_in, off + BF_INST_VOLUME);
    in->pan = bf_u8(&bank_in, off + BF_INST_PAN);
    in->priority = bf_u8(&bank_in, off + BF_INST_PRIORITY);
    in->flags = 1;
    in->tremType = bf_u8(&bank_in, off + BF_INST_TREM + 0);
    in->tremRate = bf_u8(&bank_in, off + BF_INST_TREM + 1);
    in->tremDepth = bf_u8(&bank_in, off + BF_INST_TREM + 2);
    in->tremDelay = bf_u8(&bank_in, off + BF_INST_TREM + 3);
    in->vibType = bf_u8(&bank_in, off + BF_INST_VIB + 0);
    in->vibRate = bf_u8(&bank_in, off + BF_INST_VIB + 1);
    in->vibDepth = bf_u8(&bank_in, off + BF_INST_VIB + 2);
    in->vibDelay = bf_u8(&bank_in, off + BF_INST_VIB + 3);
    in->bendRange = (s16)bf_u16(&bank_in, off + BF_INST_BEND);
    in->soundCount = n;
    for (i = 0; i < n; i++)
        in->soundArray[i] = bank_sound(bf_u32(&bank_in, off + BF_INST_SOUNDS + 4 * i));
    return in;
}

static ALBank *bank_bank(u32 off) {
    ALBank *b = bank_find(off);
    u32 perc;
    s32 i, n;

    if (b != NULL)
        return b;
    bank_check(off, BF_BANK_INSTS, 4, "a bank out of the file");
    n = (s16)bf_u16(&bank_in, off + BF_BANK_COUNT);
    if (n < 0 || (u32)n > (bank_in.len - off - BF_BANK_INSTS) / 4)
        bank_fail("a bank's instruments past the file's end", off);
    b = bank_new(off, MAX(sizeof(ALBank), offsetof(ALBank, instArray) + n * sizeof(ALInstrument *)));
    b->instCount = n;
    b->flags = 1;
    b->pad = bf_u8(&bank_in, off + BF_BANK_PAD);
    b->sampleRate = (s32)bf_u32(&bank_in, off + BF_BANK_RATE);
    perc = bf_u32(&bank_in, off + BF_BANK_PERCUSSION);
    if (perc != 0)
        b->percussion = bank_inst(perc);
    for (i = 0; i < n; i++) {
        u32 in = bf_u32(&bank_in, off + BF_BANK_INSTS + 4 * i);
        if (in != 0)
            b->instArray[i] = bank_inst(in);
    }
    return b;
}

void alBnkfNew(ALBankFile *f, u32 table, s32 len) {
    ALBankFile *parsed;
    s32 i, n, slot;

    bank_in.base = (const u8 *)f;
    bank_in.len = len < 0 ? 0 : (u32)len;
    bank_table = table;
    if (bank_in.len < BF_FILE_BANKS || bf_u16(&bank_in, BF_FILE_REVISION) != AL_BANK_VERSION)
        return;         /* (not a bank file: the reference leaves it alone too) */
    n = (s16)bf_u16(&bank_in, BF_FILE_COUNT);
    if (n < 0 || (u32)n > (bank_in.len - BF_FILE_BANKS) / 4)
        bank_fail("the bank list past the file's end", 0);
    for (slot = 0; slot < AL_BANK_FILES; slot++)
        if (bank_files[slot].file == NULL || bank_files[slot].file == f)
            break;
    if (slot == AL_BANK_FILES)
        host_fatal("libaudio: more than %d bank files (AL_BANK_FILES)", AL_BANK_FILES);
    bank_nmade = 0;
    parsed = bank_alloc(MAX(sizeof(ALBankFile), offsetof(ALBankFile, bankArray) + n * sizeof(ALBank *)));
    parsed->revision = AL_BANK_VERSION;
    parsed->bankCount = n;
    for (i = 0; i < n; i++) {
        u32 b = bf_u32(&bank_in, BF_FILE_BANKS + 4 * i);
        parsed->bankArray[i] = b != 0 ? bank_bank(b) : NULL;
    }
    bank_files[slot].file = f;
    bank_files[slot].parsed = parsed;
}

ALBank *alBnkfBank(ALBankFile *f, s32 i) {
    s32 slot;

    for (slot = 0; slot < AL_BANK_FILES; slot++)
        if (bank_files[slot].file == f) {
            if (i < 0 || i >= bank_files[slot].parsed->bankCount)
                host_fatal("libaudio: bank %d of a bank file with %d", i, bank_files[slot].parsed->bankCount);
            return bank_files[slot].parsed->bankArray[i];
        }
    host_fatal("libaudio: a bank of a file alBnkfNew didn't take (not a bank file)");
}
