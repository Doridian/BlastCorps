/*
 * banktest.so: port.so with its banks' objects out of the file, a test of
 * the replay's bank-object translation (audio_log.h, ALOG_BANK_MAP) for
 * as long as port/libaudio relocates bank files in place.
 *
 * Its alBnkfNew lets port.so's relocate the file (port_alBnkfNew: the
 * Makefile renames core.o's), then copies every ALBank, ALInstrument,
 * ALSound and ALWaveTable out of it into a pool, pointing the copies at
 * each other and at the leaves (envelopes, key maps, books, loops) where
 * they are in the file, and poisons the file objects' pointers so that a
 * read through one the replay failed to translate goes wrong at once.  It
 * exports the map; a replay into it has to match the log as port.so's
 * does, having translated what the game hands over.
 */
#include <ultra64.h>
#include <PR/libaudio.h>
#include "../../include/audio_log.h"

void port_alBnkfNew(ALBankFile *f, u8 *table);

#define POOL_SIZE (1 << 20)
#define MAX_OBJS 8192
#define POISON ((void *)0x00000004)

static u8 pool[POOL_SIZE] __attribute__((aligned(16)));
static u32 pool_used;
static AlogBankObj objs[MAX_OBJS];
static u32 nobjs;

static void fail(void) {
    __builtin_trap();
}

static void *find(void *file) {
    u32 i;
    for (i = 0; i < nobjs; i++)
        if (objs[i].file == (u32)file)
            return (void *)objs[i].native;
    return NULL;
}

/* a copy of the n bytes of a file object, mapped (again, if the file was
   loaded again: the latest copy wins) */
static void *copy(void *file, u32 n) {
    u8 *p, *s = file;
    u32 i;
    if (pool_used + ((n + 15) & ~15u) > POOL_SIZE)
        fail();
    p = pool + pool_used;
    pool_used += (n + 15) & ~15u;
    for (i = 0; i < n; i++)
        p[i] = s[i];
    for (i = 0; i < nobjs; i++)
        if (objs[i].file == (u32)file)
            break;
    if (i == nobjs) {
        if (nobjs == MAX_OBJS)
            fail();
        nobjs++;
    }
    objs[i].file = (u32)file;
    objs[i].native = (u32)p;
    return p;
}

/* each file object once per alBnkfNew: done holds the ones this call made */
static void *done[MAX_OBJS];
static u32 ndone;

static void *made(void *file) {
    u32 i;
    for (i = 0; i < ndone; i++)
        if (done[i] == file)
            return find(file);
    return NULL;
}

static void *mark(void *file, void *native) {
    if (ndone == MAX_OBJS)
        fail();
    done[ndone++] = file;
    return native;
}

static ALWaveTable *wave(ALWaveTable *w) {
    ALWaveTable *n = made(w);
    if (n != NULL)
        return n;
    n = mark(w, copy(w, sizeof(*w)));
    w->waveInfo.adpcmWave.loop = POISON;
    w->waveInfo.adpcmWave.book = POISON;
    return n;
}

static ALSound *sound(ALSound *s) {
    ALSound *n = made(s);
    if (n != NULL)
        return n;
    n = mark(s, copy(s, sizeof(*s)));
    n->wavetable = wave(s->wavetable);
    s->envelope = POISON;
    s->keyMap = POISON;
    s->wavetable = POISON;
    return n;
}

static ALInstrument *inst(ALInstrument *in) {
    ALInstrument *n = made(in);
    s32 i;
    if (n != NULL)
        return n;
    n = mark(in, copy(in, sizeof(*in) + (in->soundCount > 1 ? in->soundCount - 1 : 0) * sizeof(ALSound *)));
    for (i = 0; i < in->soundCount; i++) {
        n->soundArray[i] = sound(in->soundArray[i]);
        in->soundArray[i] = POISON;
    }
    return n;
}

static ALBank *bank(ALBank *b) {
    ALBank *n = made(b);
    s32 i;
    if (n != NULL)
        return n;
    n = mark(b, copy(b, sizeof(*b) + (b->instCount > 1 ? b->instCount - 1 : 0) * sizeof(ALInstrument *)));
    if (b->percussion != NULL) {
        n->percussion = inst(b->percussion);
        b->percussion = POISON;
    }
    for (i = 0; i < b->instCount; i++)
        if (b->instArray[i] != NULL) {
            n->instArray[i] = inst(b->instArray[i]);
            b->instArray[i] = POISON;
        }
    return n;
}

/* the game reads the file's bankArray itself (with B2, the library's
   accessor), so the file keeps it: the replay translates what it hands on */
void alBnkfNew(ALBankFile *f, u8 *table) {
    s32 i;
    port_alBnkfNew(f, table);
    if (f->revision != AL_BANK_VERSION)
        return;
    ndone = 0;
    for (i = 0; i < f->bankCount; i++)
        if (f->bankArray[i] != NULL)
            bank(f->bankArray[i]);
}

__attribute__((visibility("default"))) u32 alog_bank_map(const AlogBankObj **o) {
    *o = objs;
    return nobjs;
}
