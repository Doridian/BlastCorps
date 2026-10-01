/*
 * The libaudio oracle's recorder, host side (port/include/audio_log.h):
 * PORT_AUDIO_LOG=FILE in a -DPORT_AUDIO_RECORD=ON build writes the log of
 * every libaudio call the game makes (port/src/audio_record.c).
 *
 * The tracked memory has a shadow copy, taken each time the library hands
 * control back to the game; when the game hands it to the library again,
 * the bytes that differ from the shadow are logged (MEM) and the shadow
 * updated.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"
#include "host.h"
#include "audio_log.h"

#ifdef PORT_AUDIO_RECORD

#define MAX_RANGES 256

typedef struct {
    uint32_t addr, len;
    uint8_t *shadow;
} Range;

static Range ranges[MAX_RANGES];
static int nranges;
static int started;
static FILE *out;
static uint8_t *pending;        /* the record being built (flushed in put) */

static void w32(uint32_t v) {
    fwrite(&v, 4, 1, out);
}

static void wbytes(const void *p, uint32_t n) {
    static const uint8_t zero[4];
    fwrite(p, 1, n, out);
    if (n & 3)
        fwrite(zero, 1, 4 - (n & 3), out);
}

static void finish(void) {
    if (out != NULL) {
        w32(ALOG_END);
        fclose(out);
        out = NULL;
    }
}

int host_alog_started(void) {
    return started;
}

/* add [addr, addr+len) (merged with nothing: the caller gives disjoint ones) */
void host_alog_track(uint32_t addr, uint32_t len) {
    if (nranges == MAX_RANGES)
        host_fatal("audiolog: too many ranges");
    ranges[nranges].addr = addr;
    ranges[nranges].len = len;
    ranges[nranges].shadow = NULL;
    nranges++;
}

/* take [addr, addr+len) out of the tracked ranges, splitting one */
void host_alog_untrack(uint32_t addr, uint32_t len) {
    int i;
    if (len == 0)
        return;
    for (i = 0; i < nranges; i++) {
        Range *r = &ranges[i];
        uint32_t end = addr + len, rend = r->addr + r->len;
        if (addr >= rend || end <= r->addr)
            continue;
        if (addr <= r->addr && end >= rend) {      /* all of it */
            r->len = 0;
        } else if (addr <= r->addr) {              /* its start */
            uint32_t cut = end - r->addr;
            if (r->shadow != NULL)
                memmove(r->shadow, r->shadow + cut, r->len - cut);
            r->addr = end;
            r->len -= cut;
        } else if (end >= rend) {                  /* its end */
            r->len = addr - r->addr;
        } else {                                   /* its middle */
            Range *n;
            if (nranges == MAX_RANGES)
                host_fatal("audiolog: too many ranges");
            n = &ranges[nranges++];
            r = &ranges[i];
            n->addr = end;
            n->len = rend - end;
            n->shadow = NULL;
            if (r->shadow != NULL) {
                n->shadow = malloc(n->len);
                memcpy(n->shadow, r->shadow + (end - r->addr), n->len);
            }
            r->len = addr - r->addr;
        }
    }
    if (out != NULL) {
        w32(ALOG_UNTRACK);
        w32(addr);
        w32(len);
    }
}

void host_alog_start(void) {
    const char *path = getenv("PORT_AUDIO_LOG");
    int i;

    started = 1;
    if (path == NULL || *path == 0)
        return;
    out = fopen(path, "wb");
    if (out == NULL)
        host_fatal("audiolog: can't write %s", path);
    setvbuf(out, NULL, _IOFBF, 1 << 20);
    atexit(finish);
    w32(ALOG_MAGIC);
    w32(ALOG_VERSION);
    for (i = 0; i < nranges; i++) {
        Range *r = &ranges[i];
        r->shadow = malloc(r->len);
        memcpy(r->shadow, port_ptr(r->addr), r->len);
        w32(ALOG_TRACK);
        w32(r->addr);
        w32(r->len);
        w32(ALOG_MEM);
        w32(r->addr);
        w32(r->len);
        wbytes(r->shadow, r->len);
    }
    host_log("audiolog: writing %s\n", path);
}

/* the game hands control to the library: log what changed */
static void diff(void) {
    int i;
    for (i = 0; i < nranges; i++) {
        Range *r = &ranges[i];
        const uint8_t *cur = port_ptr(r->addr);
        uint32_t o = 0;
        while (o < r->len) {
            uint32_t n = r->len - o < 64 ? r->len - o : 64, s, e;
            if (memcmp(cur + o, r->shadow + o, n) == 0) {
                o += n;
                continue;
            }
            /* the changed blocks, as runs of the bytes that changed (an
               unchanged byte between two is the library's: its contents
               may differ in another library) */
            s = o;
            while (o < r->len) {
                n = r->len - o < 64 ? r->len - o : 64;
                if (memcmp(cur + o, r->shadow + o, n) == 0)
                    break;
                o += n;
            }
            e = o;
            while (s < e) {
                uint32_t rs;
                while (s < e && cur[s] == r->shadow[s])
                    s++;
                if (s == e)
                    break;
                rs = s;
                while (s < e && cur[s] != r->shadow[s])
                    s++;
                w32(ALOG_MEM);
                w32(r->addr + rs);
                w32(s - rs);
                wbytes(cur + rs, s - rs);
                memcpy(r->shadow + rs, cur + rs, s - rs);
            }
        }
    }
}

/* the library hands control to the game: its writes are its own */
static void snapshot(void) {
    int i;
    for (i = 0; i < nranges; i++)
        if (ranges[i].len != 0)
            memcpy(ranges[i].shadow, port_ptr(ranges[i].addr), ranges[i].len);
}

void host_alog_call(uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
                    uint32_t a5) {
    if (out == NULL)
        return;
    diff();
    w32(ALOG_CALL);
    w32(fn);
    w32(a0);
    w32(a1);
    w32(a2);
    w32(a3);
    w32(a4);
    w32(a5);
}

static void blob(uint32_t tag, uint32_t addr, uint32_t len) {
    if (out == NULL)
        return;
    w32(tag);
    w32(addr);
    w32(len);
    wbytes(port_ptr(addr), len);
}

void host_alog_in(uint32_t addr, uint32_t len) { blob(ALOG_IN, addr, len); }
void host_alog_out(uint32_t addr, uint32_t len) { blob(ALOG_OUT, addr, len); }

void host_alog_acmd(uint32_t addr, uint32_t len) {
    const uint8_t *p = port_ptr(addr);
    uint64_t h = 0xcbf29ce484222325ull;
    uint32_t i;
    if (out == NULL)
        return;
    for (i = 0; i < len; i++)
        h = (h ^ p[i]) * 0x100000001b3ull;
    w32(ALOG_ACMD);
    w32(len);
    w32((uint32_t)h);
    w32((uint32_t)(h >> 32));
}

void host_alog_ret(uint32_t value) {
    if (out == NULL)
        return;
    w32(ALOG_RET);
    w32(value);
    snapshot();
}

void host_alog_cb_enter(uint32_t kind, uint32_t a0, uint32_t a1, uint32_t a2) {
    if (out == NULL)
        return;
    w32(ALOG_CBENTER);
    w32(kind);
    w32(a0);
    w32(a1);
    w32(a2);
    snapshot();
}

void host_alog_cb_leave(uint32_t value, uint32_t extra) {
    if (out == NULL)
        return;
    diff();
    w32(ALOG_CBRET);
    w32(value);
    w32(extra);
}

#endif
