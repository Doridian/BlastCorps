/*
 * --replay FILE: play back what a movie gave the game in mupen64plus, read by
 * read (port/tools/tas.sh writes FILE as build/tas/run/polls.csv; docs/PORT.md,
 * "The TAS").
 *
 * FILE has a line per controller read: poll, VI, retraces (the scheduler's
 * count, D_803156C4, when the read started), game frames (D_80358064), the
 * mode (D_80364A90/94) and the pad (buttons << 16 | x << 8 | y).  The Nth
 * read gets the Nth pad.  The port's timing is a model, so its frames don't
 * lag where mupen64plus's do; left alone, the game would see other retrace
 * counts between reads and play differently.  So the retraces are replayed
 * too:
 *
 *   - a read's SI completion is held until the scheduler has counted the
 *     log's retraces for that read;
 *   - a retrace is held back while the count is already there and the read
 *     hasn't come, and given at once when a read waits for one.
 *
 * Each read then checks the game's frame count and mode against the log,
 * and the first difference is reported: where the replay went out of sync.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"
#include "host.h"

/* the scheduler's retrace count, the game's frame count and the mode */
extern char D_803156C4[], D_80358064[], D_80364A90[];

typedef struct {
    uint32_t retraces, frames, pad, tasks;
    uint64_t mode;
} Read;

static Read *log_reads;
static unsigned nreads;
/* vis.csv, next to FILE: per retrace (the scheduler's count before it), the
   reads and the graphics tasks mupen64plus had done when it came */
typedef struct { uint32_t reads, tasks; int valid; } Anchor;
static Anchor *anchors;
static unsigned nanchors;
static unsigned reads;          /* reads done */
static int si_waiting;          /* a read's SI completion is held */
static int diverged;
static unsigned forced;         /* retraces given although the count was there */
static int nogate;              /* PORT_REPLAY_NOGATE: the pads only, for comparison */
static uint32_t tasks_at_read;  /* the port's graphics tasks when the last read started */

int host_replay_active(void) { return log_reads != NULL; }

void host_replay_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f)
        host_fatal("can't open %s", path);
    char line[256];
    unsigned cap = 0;
    while (fgets(line, sizeof line, f)) {
        unsigned poll, vi, ret, frames, pad, tasks = 0;
        unsigned long long mode;
        if (sscanf(line, "%u,%u,%u,%u,%llx,%x,%u", &poll, &vi, &ret, &frames, &mode, &pad, &tasks) < 6)
            continue;           /* the header */
        if (poll != nreads + 1)
            host_fatal("%s: read %u out of order", path, poll);
        if (nreads == cap)
            log_reads = realloc(log_reads, (cap = cap ? cap * 2 : 65536) * sizeof *log_reads);
        log_reads[nreads++] = (Read){ ret, frames, pad, tasks, mode };
    }
    fclose(f);
    if (!nreads)
        host_fatal("%s: no reads", path);
    nogate = getenv("PORT_REPLAY_NOGATE") != NULL;

    char vpath[1024];
    const char *slash = strrchr(path, '/');
    snprintf(vpath, sizeof vpath, "%.*svis.csv", slash ? (int)(slash - path + 1) : 0, path);
    if ((f = fopen(vpath, "r"))) {
        while (fgets(line, sizeof line, f)) {
            unsigned vi, ret, rd, tk;
            if (sscanf(line, "%u,%u,%u,%u", &vi, &ret, &rd, &tk) != 4)
                continue;
            if (ret >= nanchors) {
                unsigned n = ret + 1 > nanchors * 2 ? ret + 1 : nanchors * 2;
                anchors = realloc(anchors, n * sizeof *anchors);
                memset(anchors + nanchors, 0, (n - nanchors) * sizeof *anchors);
                nanchors = n;
            }
            /* the last VI before the count moves on is the one that moves it */
            anchors[ret] = (Anchor){ rd, tk, 1 };
        }
        fclose(f);
    }
    host_log("replay: %u reads from %s%s\n", nreads, path, anchors ? ", retraces anchored by vis.csv" : "");
}

static uint32_t retraces(void) { return port_be32(D_803156C4); }

static uint64_t mode_now(void) {
    return (uint64_t)port_be32(D_80364A90) << 32 | port_be32(D_80364A90 + 4);
}

static void check(const char *what, uint32_t got, uint32_t want) {
    if (!diverged && got != want) {
        diverged = 1;
        host_log("replay: out of sync at read %u: %s %u, the log has %u (mode %016llX)\n", reads,
                 what, got, want, (unsigned long long)mode_now());
    }
}

/* the log's read the game is in (started, SI completion held), or the next */
static const Read *cur(void) {
    unsigned k = si_waiting ? reads - 1 : reads;
    return k < nreads ? &log_reads[k] : NULL;
}

/* by the retraces sent, not the count: the scheduler counts a retrace only
   once it has run, and the loop could send another before that (it counts
   every one, from the first) */
static int target_reached(void) {
    if (nogate)
        return 1;
    const Read *r = cur();
    return !r || port_vi_sent() >= r->retraces;
}

/* osContStartReadData: the game starts a read.  mupen64plus takes the pad
   here, when the PIF runs the command (the game starts one read at boot
   that it never fetches), so this is what counts. */
void host_replay_read_started(void) {
    tasks_at_read = port_gfx_tasks();
    reads++;
    si_waiting = 1;
    if (reads <= nreads) {
        const Read *r = &log_reads[reads - 1];
        check("frame", port_be32(D_80358064), r->frames);
        uint64_t m = mode_now();
        if (!diverged && m != r->mode) {
            diverged = 1;
            host_log("replay: out of sync at read %u: mode %016llX, the log has %016llX\n", reads,
                     (unsigned long long)m, (unsigned long long)r->mode);
        }
    }
    if (reads % 10000 == 0 && host_verbose)
        host_log("replay: read %u, mode %016llX\n", reads, (unsigned long long)mode_now());
}

/* the loop: the held SI completion, once the count is there; 1 if it was
   given */
int host_replay_poll_si(void) {
    if (!si_waiting || !target_reached())
        return 0;
    if (reads <= nreads)
        check("retrace", port_vi_sent(), log_reads[reads - 1].retraces);
    si_waiting = 0;
    port_replay_si_done();
    return 1;
}

/* the loop, at a retrace: 0 to hold it back */
/* the next retrace's place in mupen64plus's run, by the graphics tasks run
   before it (the two boots run the same ones): 0 if the port isn't there
   yet, 1 if it is, 2 if it is past it, -1 if the log doesn't say */
static int anchor_reached(void) {
    uint32_t r = port_vi_sent();
    if (r >= nanchors || !anchors[r].valid)
        return -1;
    uint32_t done = port_gfx_tasks();
    if (done > anchors[r].tasks || reads > anchors[r].reads)
        return 2;
    return done == anchors[r].tasks;
}

/* the loop, at a retrace: 0 to hold it back */
int host_replay_vi_ok(void) {
    if (nogate)
        return 1;
    /* not past the count the log's next read has, and not before the
       graphics tasks mupen64plus's had run */
    return !target_reached() && anchor_reached() != 0;
}

/* the loop: 1 to give the retrace now rather than when the clock says: the
   game is past where mupen64plus's was when it came, or just there and this
   is the first retrace there (the ones after it, while the game waits or
   spins on the count, come by the clock) */
int host_replay_vi_now(void) {
    /* (and once the scheduler has counted the last one: firing before it
       ran would only fill its queue) */
    if (nogate || retraces() != port_vi_sent() || target_reached())
        return 0;
    int a = anchor_reached();
    uint32_t r = port_vi_sent();
    return a == 2 || (a == 1 && (r == 0 || r - 1 >= nanchors || !anchors[r - 1].valid ||
                                 anchors[r - 1].tasks != anchors[r].tasks));
}

/* PORT_REPLAY_VIS=FILE: the port's own vis.csv, to compare */
void host_replay_vi_fired(void) {
    static FILE *f;
    static int opened;
    if (!opened++) {
        const char *p = getenv("PORT_REPLAY_VIS");
        if (p && (f = fopen(p, "w")))
            fprintf(f, "vi,retraces,reads,tasks,tasks_at_read\n");
    }
    if (f)
        fprintf(f, "0,%u,%u,%u,%u\n", port_vi_sent(), reads, port_gfx_tasks(), tasks_at_read);
}

/* a retrace held back with nothing else to do: the game waits for one
   that mupen64plus's didn't, so it is out of sync */
void host_replay_vi_forced(void) {
    const Read *r = cur();
    if (forced++ < 10)
        host_log("replay: read %u waits for retrace %u, the log has %u\n", reads + !si_waiting,
                 retraces() + 1, r ? r->retraces : 0);
}

/* osContGetReadData: the pad of the last read started */
void host_replay_pad(uint16_t *buttons, int *x, int *y) {
    *buttons = 0;
    *x = *y = 0;
    if (reads == 0 || reads > nreads)
        return;
    const Read *r = &log_reads[reads - 1];
    *buttons = (uint16_t)(r->pad >> 16);
    *x = (int8_t)(r->pad >> 8);
    *y = (int8_t)r->pad;
}

/* past the end of the log, with time for the last save */
int host_replay_done(void) {
    return log_reads && reads >= nreads + 600;
}

void host_replay_report(void) {
    if (!log_reads)
        return;
    host_log("replay: %u of %u reads, %s, %u retraces forced\n", reads < nreads ? reads : nreads, nreads,
             diverged ? "out of sync" : "in sync", forced);
}
