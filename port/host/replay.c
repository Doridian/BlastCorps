/*
 * --replay FILE: play back a movie's input as mupen64plus gave it to the game
 * (port/tools/tas.sh writes FILE as build/tas/run/polls.csv; docs/PORT.md,
 * "The TAS").
 *
 * FILE has a line per controller read: poll, VI, retraces (the scheduler's
 * count, D_803156C4, when the read started), game frames (D_80358064, from 0
 * in each mode), the mode (D_80364A90/94) and the pad (buttons << 16 | x << 8
 * | y).
 *
 * The port's timing isn't mupen64plus's, so its reads don't fall where the
 * log's do (the title, for one, fades out a frame earlier: func_80274BF0
 * reads the count in the middle of a frame).  So the replay follows the
 * port's frames rather than the log's reads:
 *
 *   - A read gets the pad of the log's read at the same mode and frame: the
 *     first such read after the last one matched (reads within a frame go in
 *     order, and a mode the port leaves early skips the rest of it in the
 *     log).  A read the log has no match for (the port stays in a mode
 *     longer) gets no buttons.
 *   - Each frame gets the retraces the log's frame has, counted from when
 *     the port's read before it went through: a read's SI completion is
 *     held until the scheduler has had them, and a retrace is held back
 *     once the next read's are there (taking the next read to be the log's
 *     next).  Within the frame the retraces come by the port's CPU model, so
 *     the game reads the count mid-frame (the music's and the messages'
 *     timing, func_8026BCE0) about where mupen64plus's did.  Where it waits
 *     for a retrace the log's frame didn't have, one is given anyway, and
 *     counted.
 *   - The random number generator is seeded from osGetCount, the port's
 *     clock and not the movie's: after a seeding, the next read sets the
 *     generator's state to the log's for that read (if it has the column).
 *     So does a read after the port spent a different number of frames in
 *     a mode (a loading screen) than the movie did.
 *   - cvt.w/round.w round halves up, as the movie's emulator does (the
 *     hardware rounds them to even; the first level goes elsewhere at frame
 *     370 with that).
 *
 * What says the replay still makes sense: the player's position at every
 * matched read (if the log has it) against the log's, every read that finds no match
 * (logged with the mode at each change), the log's reads skipped, the
 * retraces given anyway, and in the end the save (port/tools/tas_check.py).
 */
#ifdef __linux__
#include <elf.h>
#else
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"
#include "host.h"

#ifdef PORT_NATIVE_ENDIAN
/* game variables are read with port_be32 below: at their own width (port.h) */
#define port_be32(p) port_var32(p)
#endif

/* the scheduler's retrace count, the game's frame count, the mode and the
   random number generator's state */
extern char D_803156C4[], D_80358064[], D_80364A90[], D_8036B968[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_803156C4 PORT_VAR(D_803156C4)
#define D_80358064 PORT_VAR(D_80358064)
#define D_80364A90 PORT_VAR(D_80364A90)
#define D_8036B968 PORT_VAR(D_8036B968)
#endif
/* the player's position */
extern char D_803643E0[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_803643E0 PORT_VAR(D_803643E0)
#endif

typedef struct {
    uint32_t retraces, frames, pad, rng;
    int32_t pos[3];
    uint64_t mode;
} Read;

static Read *log_reads;
static unsigned nreads;
/* counter_reads.csv, next to FILE: the game's reads of the two counts, in
   order, each with the log's reads so far and its PC, which n64_funcs.txt
   (the port's build, from the version's ELFs) names the function of.
   first_count[k] is the first after the log's read k+1 started (read 1 is
   the log's first; the reads before it, at boot, aren't replayed),
   first_count[nreads] the end. */
typedef struct { uint32_t timer, retraces; int func; } Count;
static Count *counts;
static unsigned ncounts, *first_count;
static unsigned next_count, end_count;  /* the current frame's */
static unsigned counts_unlogged;        /* reads in a function the log's frame has none in */
/* the functions: by address, and their names sorted */
typedef struct { uint32_t addr; int name; } Func;
static Func *funcs;
static unsigned nfuncs;
static char **names;
static unsigned nnames;
/* audio_reads.csv, next to FILE: what alCSPGetState and alCSeqGetLoc told
   the movie's game (replay_audio.c), by the calling function; first_audio
   as first_count */
typedef struct { int kind, func; int32_t value; } Audio;
static Audio *audio;
static unsigned naudio, *first_audio;
static unsigned next_audio, end_audio;
static unsigned audio_unlogged;
/* save_starts.csv, next to FILE: the log's read each time the pak/EEPROM
   thread started a command */
static unsigned *save_starts, nsave_starts, saves;
/* checkpoints.csv: at each of the log's mode changes, the state to carry
   over when the port has to be put there (force_mode) */
typedef struct { unsigned read; uint64_t mode; uint8_t *state; } Checkpoint;
static Checkpoint *checkpoints;
/* switches.csv: every mode the movie's game switched to, with its reads so
   far (a change seen at the reads may be several: the game passes through
   some modes within one switch) */
typedef struct { unsigned read; uint64_t mode; } Switch;
static Switch *switches;
static unsigned nswitches, next_switch;
static unsigned ncheckpoints, forced_modes, unmatched_run;
#define CHECKPOINT_STATE (0x400 + 0x80 + 0xF0 + 4 + 4 + 4)
#define GRACE 60        /* frames without a match before the log is followed on its own */
static int save_waiting;
static unsigned saves_early;
static unsigned saves_held_si;  /* the save thread held while the game receives its pad read */
static struct { int kind, func; unsigned n; } frame_audio[32];
static unsigned nframe_audio;
/* the reads in the current frame so far, per function */
static struct { int func; unsigned n; } frame_reads[64];
static unsigned nframe_reads;
#define WINDOW 5000             /* how far ahead in the log a read is looked for */

static unsigned reads;          /* the port's reads started */
static int matched = -1;        /* the log's read the last match was */
static const Read *cur_read;    /* the current read's match, or NULL */
static int si_waiting;          /* its SI completion is held */
static int si_started;          /* a read started that the game hasn't fetched */
static uint32_t base;           /* the retraces when the last read went through */
static uint32_t target;         /* the retraces the held read waits for */
static uint32_t next_target;    /* the next read's, as far as the log says */
static int next_known;
static uint64_t last_mode = ~0ull;
static unsigned skipped, unmatched, forced;
static int nogate;              /* PORT_REPLAY_NOGATE: the pads only, for comparison */
static int has_rng;             /* the log has the generator's state */
static int has_pos;             /* ... and the player's position */
static unsigned pos_diffs;      /* reads where the position differs */
static int seeded;              /* the game seeded it since the last read */
static int gap;                 /* reads without a match since the last one */
static unsigned seeds;

int host_replay_active(void) { return log_reads != NULL; }

static int cmp_str(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

/* n64_funcs.txt: "address t name" per function, by address */
static void load_funcs(void) {
    const char *p = getenv("PORT_N64_FUNCS");
#ifdef PORT_N64_FUNCS
    if (!p)
        p = PORT_N64_FUNCS;
#endif
    FILE *f = p ? fopen(p, "r") : NULL;
    if (!f)
        host_fatal("replay: no function table (PORT_N64_FUNCS)");
    char line[256], name[200];
    unsigned cap = 0, addr;
    char type;
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "%x %c %199s", &addr, &type, name) == 3) {
            if (nfuncs == cap) {
                cap = cap ? cap * 2 : 8192;
                funcs = realloc(funcs, cap * sizeof *funcs);
                names = realloc(names, cap * sizeof *names);
            }
            names[nfuncs] = strdup(name);
            funcs[nfuncs] = (Func){ addr, (int)nfuncs };
            nfuncs++;
        }
    fclose(f);
    /* names sorted, and each function's index into them */
    char **sorted = malloc(nfuncs * sizeof *sorted);
    memcpy(sorted, names, nfuncs * sizeof *sorted);
    qsort(sorted, nfuncs, sizeof *sorted, cmp_str);
    for (unsigned i = 0; i < nfuncs; i++) {
        char **hit = bsearch(&names[i], sorted, nfuncs, sizeof *sorted, cmp_str);
        funcs[i].name = (int)(hit - sorted);
    }
    free(names);
    names = sorted;
    nnames = nfuncs;
}

/* the function a PC is in (by the name's index) */
static int func_at(uint32_t pc) {
    unsigned lo = 0, hi = nfuncs;
    while (hi - lo > 1) {
        unsigned mid = (lo + hi) / 2;
        if (funcs[mid].addr <= pc)
            lo = mid;
        else
            hi = mid;
    }
    return nfuncs && funcs[lo].addr <= pc ? funcs[lo].name : -1;
}

static int func_named(const char *name) {
    char **hit = nnames ? bsearch(&name, names, nnames, sizeof *names, cmp_str) : NULL;
    return hit ? (int)(hit - names) : -1;
}

void host_replay_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f)
        host_fatal("can't open %s", path);
    char line[256];
    unsigned cap = 0;
    while (fgets(line, sizeof line, f)) {
        unsigned poll, vi, ret, frames, pad, rng = 0, x = 0, y = 0, z = 0;
        unsigned long long mode;
        int n = sscanf(line, "%u,%u,%u,%u,%llx,%x,%x,%x,%x,%x", &poll, &vi, &ret, &frames, &mode, &pad, &rng,
                       &x, &y, &z);
        if (n < 6)
            continue;           /* the header */
        has_rng |= n >= 7;
        has_pos |= n >= 10;
        if (poll != nreads + 1)
            host_fatal("%s: read %u out of order", path, poll);
        if (nreads == cap)
            log_reads = realloc(log_reads, (cap = cap ? cap * 2 : 65536) * sizeof *log_reads);
        log_reads[nreads++] = (Read){ ret, frames, pad, rng, { (int32_t)x, (int32_t)y, (int32_t)z }, mode };
    }
    fclose(f);
    if (!nreads)
        host_fatal("%s: no reads", path);
    nogate = getenv("PORT_REPLAY_NOGATE") != NULL;

    char cpath[1024];
    const char *slash = strrchr(path, '/');
    snprintf(cpath, sizeof cpath, "%.*scounter_reads.csv", slash ? (int)(slash - path + 1) : 0, path);
    if ((f = fopen(cpath, "r"))) {
        load_funcs();
        unsigned ccap = 0, last = 0;
        first_count = calloc(nreads + 1, sizeof *first_count);
        while (fgets(line, sizeof line, f)) {
            unsigned rd, pc, timer, ret;
            if (sscanf(line, "%u,%x,%u,%u", &rd, &pc, &timer, &ret) != 4)
                continue;
            if (rd < last || rd > nreads)
                host_fatal("%s: read %u out of order", cpath, rd);
            for (; last < rd; last++)
                first_count[last] = ncounts;
            if (ncounts == ccap)
                counts = realloc(counts, (ccap = ccap ? ccap * 2 : 1 << 20) * sizeof *counts);
            counts[ncounts++] = (Count){ timer, ret, func_at(pc) };
        }
        for (; last <= nreads; last++)
            first_count[last] = ncounts;
        fclose(f);
        host_log("replay: %u reads of the counts from %s\n", ncounts, cpath);
    }
    snprintf(cpath, sizeof cpath, "%.*saudio_reads.csv", slash ? (int)(slash - path + 1) : 0, path);
    if ((f = fopen(cpath, "r"))) {
        if (!nfuncs)
            load_funcs();
        unsigned acap = 0, last = 0;
        first_audio = calloc(nreads + 1, sizeof *first_audio);
        while (fgets(line, sizeof line, f)) {
            unsigned rd, caller;
            int kind, value;
            if (sscanf(line, "%u,%x,%d,%d", &rd, &caller, &kind, &value) != 4)
                continue;
            if (rd < last || rd > nreads)
                host_fatal("%s: read %u out of order", cpath, rd);
            for (; last < rd; last++)
                first_audio[last] = naudio;
            if (naudio == acap)
                audio = realloc(audio, (acap = acap ? acap * 2 : 4096) * sizeof *audio);
            /* the return address is one past the call's delay slot: in the caller */
            audio[naudio++] = (Audio){ kind, func_at(caller - 8), value };
        }
        for (; last <= nreads; last++)
            first_audio[last] = naudio;
        fclose(f);
        host_log("replay: %u of the audio thread's answers from %s\n", naudio, cpath);
    }
    snprintf(cpath, sizeof cpath, "%.*ssave_starts.csv", slash ? (int)(slash - path + 1) : 0, path);
    if ((f = fopen(cpath, "r"))) {
        unsigned scap = 0, rd;
        while (fgets(line, sizeof line, f))
            if (sscanf(line, "%u", &rd) == 1) {
                if (nsave_starts == scap)
                    save_starts = realloc(save_starts, (scap = scap ? scap * 2 : 256) * sizeof *save_starts);
                save_starts[nsave_starts++] = rd;
            }
        fclose(f);
        host_log("replay: %u of the save thread's commands from %s\n", nsave_starts, cpath);
    }
    snprintf(cpath, sizeof cpath, "%.*sswitches.csv", slash ? (int)(slash - path + 1) : 0, path);
    if ((f = fopen(cpath, "r"))) {
        unsigned wcap = 0, rd;
        unsigned long long mode;
        while (fgets(line, sizeof line, f))
            if (sscanf(line, "%u,%llx", &rd, &mode) == 2) {
                if (nswitches == wcap)
                    switches = realloc(switches, (wcap = wcap ? wcap * 2 : 1024) * sizeof *switches);
                switches[nswitches++] = (Switch){ rd, mode };
            }
        fclose(f);
        host_log("replay: %u of the game's mode switches from %s\n", nswitches, cpath);
    }
    snprintf(cpath, sizeof cpath, "%.*scheckpoints.csv", slash ? (int)(slash - path + 1) : 0, path);
    if ((f = fopen(cpath, "r"))) {
        static char big[2 * CHECKPOINT_STATE + 128];
        unsigned ccap = 0;
        while (fgets(big, sizeof big, f)) {
            unsigned rd;
            unsigned long long mode;
            int n = 0;
            if (sscanf(big, "%u,%llx,%n", &rd, &mode, &n) != 2 || strlen(big + n) < 2 * CHECKPOINT_STATE)
                continue;
            uint8_t *st = malloc(CHECKPOINT_STATE);
            for (unsigned i = 0; i < CHECKPOINT_STATE; i++) {
                unsigned b;
                sscanf(big + n + 2 * i, "%2x", &b);
                st[i] = (uint8_t)b;
            }
            if (ncheckpoints == ccap)
                checkpoints = realloc(checkpoints, (ccap = ccap ? ccap * 2 : 256) * sizeof *checkpoints);
            checkpoints[ncheckpoints++] = (Checkpoint){ rd, mode, st };
        }
        fclose(f);
        host_log("replay: %u checkpoints from %s\n", ncheckpoints, cpath);
    }
    /* the movie's emulator rounds cvt.w halves up (recomp.h), and the
       movie depends on it (PORT_REPLAY_VR4300_ROUNDING=1: the hardware's) */
    extern int recomp_round_half_up;
    recomp_round_half_up = getenv("PORT_REPLAY_VR4300_ROUNDING") == NULL;
    host_log("replay: %u reads from %s\n", nreads, path);
}

static uint64_t mode_now(void) {
    return (uint64_t)port_be32(D_80364A90) << 32 | port_be32(D_80364A90 + 4);
}

/* the log's retraces for its read k's frame */
static uint32_t frame_retraces(unsigned k) {
    return k == 0 ? log_reads[0].retraces : log_reads[k].retraces - log_reads[k - 1].retraces;
}

/* PORT_REPLAY_DUMP=N,...: RDRAM (big-endian, as m64p_tas's TAS_DUMP) to
   rdram_N.bin as the read matching the log's Nth starts */
static void dump(unsigned n) {
    static const char *spec = (const char *)1;
    if (spec == (const char *)1)
        spec = getenv("PORT_REPLAY_DUMP");
    for (const char *p = spec; p && *p;) {
        char *end;
        if (strtoul(p, &end, 10) == n) {
            char path[64];
            snprintf(path, sizeof path, "rdram_%u.bin", n);
            FILE *f = fopen(path, "wb");
            if (f) {
                fwrite(port_ptr(0x80000000), 1, 0x400000, f);
                fclose(f);
            }
        }
        p = *end ? end + 1 : end;
    }
}

/* osContStartReadData: the game starts a read.  mupen64plus takes the pad
   here, when the PIF runs the command (the game starts one read at boot
   that it never fetches), so this is what counts. */
void host_replay_read_started(void) {
    si_started = 1;
    uint64_t mode = mode_now();
    uint32_t frames = port_be32(D_80358064);
    int found = -1;

    reads++;
    si_waiting = 1;
    for (unsigned k = matched + 1; k < nreads && k <= (unsigned)(matched + 1) + WINDOW; k++)
        if (log_reads[k].mode == mode && log_reads[k].frames == frames) {
            found = (int)k;
            break;
        }
    {
        /* PORT_REPLAY_TRACE=FROM,TO: every read in that range */
        static int init;
        static unsigned from, to;
        if (!init++) {
            const char *t = getenv("PORT_REPLAY_TRACE");
            if (t)
                sscanf(t, "%u,%u", &from, &to);
        }
        if (reads >= from && reads <= to)
            host_log("replay: read %u: mode %016llX frame %u retraces %u, the log's read %d (pad %08X)\n", reads,
                     (unsigned long long)mode, frames, port_be32(D_803156C4), found + 1,
                     found >= 0 ? log_reads[found].pad : 0);
    }
    if (mode != last_mode) {
        if (host_verbose || found < 0)
            host_log("replay: mode %016llX at read %u: the log's read %d%s\n", (unsigned long long)mode,
                     reads, found + 1, found < 0 ? " (no match)" : "");
        last_mode = mode;
    }
    if (found < 0) {
        if (unmatched < 5 || host_verbose > 1)
            host_log("replay: read %u (mode %016llX frame %u): no match after the log's read %d (mode %016llX "
                     "frame %u)\n", reads, (unsigned long long)mode, frames, matched + 1,
                     matched >= 0 ? (unsigned long long)log_reads[matched].mode : 0ull,
                     matched >= 0 ? log_reads[matched].frames : 0);
        unmatched++;
        gap = 1;
        cur_read = NULL;
        next_count = end_count = 0;
        target = port_vi_sent();        /* nothing to wait for */
        return;
    }
    int prev = matched;
    skipped += found - (matched + 1);
    matched = found;
    cur_read = &log_reads[found];
    if (counts) {
        /* the game's reads of the counts until the next read: the log's
           while its read found+1 was the last */
        next_count = first_count[found];
        end_count = first_count[found + 1];
        nframe_reads = 0;
    }
    if (audio) {
        next_audio = first_audio[found];
        end_audio = first_audio[found + 1];
        nframe_audio = 0;
    }
    /* the seed is the movie's clock: the state the log has at this read is
       its seed, advanced as the same frame's calls advance the port's.
       Likewise where the port spent a different number of frames in the
       last mode than the movie (a loading screen: the port loads faster),
       whose frames drew numbers too. */
    if ((seeded || found > prev + 1 || gap) && has_rng && port_be32(D_8036B968) != cur_read->rng) {
        port_wg32(D_8036B968, cur_read->rng);    /* a game variable: its own width */
        seeds++;
    }
    seeded = 0;
    gap = 0;
    dump(found + 1);
    /* the player where the movie had it (in a level: outside, it's what
       the attract mode left) */
    if (has_pos && (mode == 4 || mode == 0x4000)) {
        int32_t p[3];
        for (int i = 0; i < 3; i++)
            p[i] = (int32_t)port_be32(D_803643E0 + 4 * i);
        if (p[0] != cur_read->pos[0] || p[1] != cur_read->pos[1] || p[2] != cur_read->pos[2]) {
            if (pos_diffs++ < 5 || host_verbose > 1)
                host_log("replay: read %u (the log's %d, mode %016llX frame %u): the player at %d,%d,%d, "
                         "the log has %d,%d,%d\n", reads, found + 1, (unsigned long long)mode, frames,
                         p[0] >> 5, p[1] >> 5, p[2] >> 5, cur_read->pos[0] >> 5, cur_read->pos[1] >> 5,
                         cur_read->pos[2] >> 5);
        }
    }
    target = base + frame_retraces(found);
    if (reads % 10000 == 0 && host_verbose)
        host_log("replay: read %u, the log's %d, mode %016llX\n", reads, found + 1, (unsigned long long)mode);
}

/* The game's reads of D_803156C4 and D_803156C0 (port_game.h): in a matched
   frame, what the movie's game read in the same function in that frame (the
   n-th time for the n-th, or the last); the real ones otherwise.  (By
   function, not by order in the frame: IDO and clang load a global a
   different number of times; within a function and a frame the movie's
   values nearly always agree.) */
extern char D_803156C0[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_803156C0 PORT_VAR(D_803156C0)
#endif
unsigned int port_counter(int timer, const char *func) {
    if (counts && cur_read) {
        int id = func_named(func);
        unsigned k, n = 0;
        for (k = 0; k < nframe_reads && frame_reads[k].func != id; k++)
            ;
        if (k < nframe_reads)
            n = frame_reads[k].n++;
        else if (nframe_reads < sizeof frame_reads / sizeof frame_reads[0]) {
            frame_reads[nframe_reads].func = id;
            frame_reads[nframe_reads++].n = 1;
        }
        const Count *hit = NULL;
        for (unsigned i = next_count; i < end_count; i++)
            if (counts[i].func == id) {
                hit = &counts[i];
                if (n-- == 0)
                    break;
            }
        if (hit)
            return timer ? hit->timer : hit->retraces;
        if (counts_unlogged++ < 5 || host_verbose > 1)
            host_log("replay: the log's read %d: %s reads the %s, which the log's frame doesn't\n", matched + 1,
                     func, timer ? "level timer" : "retrace count");
    }
    return port_be32(timer ? D_803156C0 : D_803156C4);
}

/* The port's own functions by address (its symbol table, from
   /proc/self/exe): the audio queries name their caller by its return
   address.  Elsewhere dladdr, which sees the exported symbols only (on
   macOS every global of the executable); WebAssembly has no return
   addresses to go by (docs/PORT.md, "Threads without ucontext"). */
#ifdef __linux__
typedef struct { uint64_t addr, size; const char *name; } HostFunc;
static HostFunc *hfuncs;
static unsigned nhfuncs;

static int cmp_hfunc(const void *a, const void *b) {
    uint64_t x = ((const HostFunc *)a)->addr, y = ((const HostFunc *)b)->addr;
    return x < y ? -1 : x > y;
}

static void load_host_funcs(void) {
    FILE *f = fopen("/proc/self/exe", "rb");
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *img = malloc(size);
    if (fread(img, 1, size, f) != (size_t)size) {
        fclose(f);
        return;
    }
    fclose(f);
    int wide = img[EI_CLASS] == ELFCLASS64;
#define FIELD(T32, T64, p, m) (wide ? ((T64 *)(p))->m : ((T32 *)(p))->m)
    uint64_t shoff = FIELD(Elf32_Ehdr, Elf64_Ehdr, img, e_shoff);
    unsigned shnum = FIELD(Elf32_Ehdr, Elf64_Ehdr, img, e_shnum);
    unsigned shentsize = FIELD(Elf32_Ehdr, Elf64_Ehdr, img, e_shentsize);
    unsigned cap = 0;
    for (unsigned i = 0; i < shnum; i++) {
        unsigned char *sh = img + shoff + (uint64_t)i * shentsize;
        if (FIELD(Elf32_Shdr, Elf64_Shdr, sh, sh_type) != SHT_SYMTAB)
            continue;
        unsigned char *strsh = img + shoff + (uint64_t)FIELD(Elf32_Shdr, Elf64_Shdr, sh, sh_link) * shentsize;
        const char *strtab = (const char *)img + FIELD(Elf32_Shdr, Elf64_Shdr, strsh, sh_offset);
        uint64_t off = FIELD(Elf32_Shdr, Elf64_Shdr, sh, sh_offset), n = FIELD(Elf32_Shdr, Elf64_Shdr, sh, sh_size);
        uint64_t ent = FIELD(Elf32_Shdr, Elf64_Shdr, sh, sh_entsize);
        for (uint64_t k = 0; k + ent <= n; k += ent) {
            unsigned char *sym = img + off + k;
            unsigned info = FIELD(Elf32_Sym, Elf64_Sym, sym, st_info);
            if ((info & 0xF) != STT_FUNC)
                continue;
            if (nhfuncs == cap)
                hfuncs = realloc(hfuncs, (cap = cap ? cap * 2 : 8192) * sizeof *hfuncs);
            hfuncs[nhfuncs++] = (HostFunc){ FIELD(Elf32_Sym, Elf64_Sym, sym, st_value),
                                            FIELD(Elf32_Sym, Elf64_Sym, sym, st_size),
                                            strtab + FIELD(Elf32_Sym, Elf64_Sym, sym, st_name) };
        }
    }
#undef FIELD
    qsort(hfuncs, nhfuncs, sizeof *hfuncs, cmp_hfunc);
}

static const char *host_func_name(uint64_t addr) {
    if (!hfuncs)
        load_host_funcs();
    unsigned lo = 0, hi = nhfuncs;
    while (hi - lo > 1) {
        unsigned mid = (lo + hi) / 2;
        if (hfuncs[mid].addr <= addr)
            lo = mid;
        else
            hi = mid;
    }
    return nhfuncs && hfuncs[lo].addr <= addr && addr < hfuncs[lo].addr + hfuncs[lo].size ? hfuncs[lo].name : "?";
}
#else
static const char *host_func_name(uint64_t addr) {
    Dl_info di;
    if (dladdr((void *)(uintptr_t)addr, &di) && di.dli_sname)
        return di.dli_sname;
    return "?";
}
#endif

/* alCSPGetState and alCSeqGetLoc (replay_audio.c): in a matched frame, what
   they told the movie's game in the same calling function in that frame (the
   n-th time for the n-th, or the last); the real answer otherwise. */
int32_t host_replay_audio(int kind, int32_t real, uint64_t caller) {
    if (!audio || !cur_read)
        return real;
    const char *name = host_func_name(caller);
    int id = func_named(name);
    unsigned k, n = 0;
    for (k = 0; k < nframe_audio && !(frame_audio[k].func == id && frame_audio[k].kind == kind); k++)
        ;
    if (k < nframe_audio)
        n = frame_audio[k].n++;
    else if (nframe_audio < sizeof frame_audio / sizeof frame_audio[0]) {
        frame_audio[nframe_audio].kind = kind;
        frame_audio[nframe_audio].func = id;
        frame_audio[nframe_audio++].n = 1;
    }
    const Audio *hit = NULL;
    for (unsigned i = next_audio; i < end_audio; i++)
        if (audio[i].kind == kind && audio[i].func == id) {
            hit = &audio[i];
            if (n-- == 0)
                break;
        }
    if (hit) {
        if (hit->value != real && host_verbose > 1)
            host_log("replay: the log's read %d: %s's %s is %d, the port's %d\n", matched + 1, name,
                     kind ? "lastTicks" : "player state", hit->value, real);
        return hit->value;
    }
    if (audio_unlogged++ < 5 || host_verbose > 1)
        host_log("replay: the log's read %d: %s asks for the %s, which the log's frame doesn't\n", matched + 1,
                 name, kind ? "sequence's lastTicks" : "player's state");
    return real;
}

/* 45BB0.c: whether this frame reads the pad.  The game skips the read while
   the save thread has the SI (D_8039C4B0), which is that thread's timing;
   with --replay the frame reads if the movie's did (a read with this mode
   and frame after the last match), and fetches what it started. */
/* Put the port into the log's mode at its read k (a mode change): the state
   the movie had there (the save records, best times, level, player, random
   state), and the mode as the next one, which the game switches to after
   this frame (00000.c's loop, its init included); a fade still going is
   dropped (the loop asserts none is). */
extern char D_80364AF0[], D_80364EF0[], D_80364F70[], D_802E8BDC[], D_80364AE8[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_80364AF0 PORT_VAR(D_80364AF0)
#define D_80364EF0 PORT_VAR(D_80364EF0)
#define D_80364F70 PORT_VAR(D_80364F70)
#define D_802E8BDC PORT_VAR(D_802E8BDC)
#define D_80364AE8 PORT_VAR(D_80364AE8)
#endif
extern char D_80364A98[], D_80364AA0[], D_8036C778[], D_8036C784[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_80364A98 PORT_VAR(D_80364A98)
#define D_80364AA0 PORT_VAR(D_80364AA0)
#define D_8036C778 PORT_VAR(D_8036C778)
#define D_8036C784 PORT_VAR(D_8036C784)
#endif

static void force_mode(unsigned k) {
    const Checkpoint *c = NULL;
    for (unsigned i = 0; i < ncheckpoints; i++)
        if (checkpoints[i].read == k + 1)
            c = &checkpoints[i];
    if (!c)
        return;
    /* the log has the N64's bytes: each datum goes in at its own width */
    const uint8_t *st = c->state;
    memcpy(D_80364AF0, st, 0x400), st += 0x400;
    for (int i = 0; i < 4; i++)         /* PlayerInfo[4], as the save has them */
        host_save_order((uint8_t *)D_80364AF0 + 0x100 * i, 0, 0x100, 0);
    for (int i = 0; i < 0x40; i++, st += 2)     /* u16 [4][16] */
        port_wg16(D_80364EF0 + 2 * i, port_be16(st));
    for (int i = 0; i < 0x78; i++, st += 2)     /* u16 [0x78] */
        port_wg16(D_80364F70 + 2 * i, port_be16(st));
    port_wg32(D_802E8BDC, port_be32(st)), st += 4;
    D_80364AE8[0] = st[0], st += 4;
    port_wg32(D_8036B968, port_be32(st));
    /* the first mode the movie's game switched to on its way there: the
       reads so far were k */
    uint64_t first = c->mode;
    for (unsigned i = 0; i < nswitches; i++)
        if (switches[i].read == k) {
            first = switches[i].mode;
            next_switch = i + 1;
            break;
        }
    port_wg32(D_80364A98, (uint32_t)(first >> 32));
    port_wg32(D_80364A98 + 4, (uint32_t)first);
    memset(D_80364AA0, 0, 8);
    memset(D_8036C778, 0, 8);
    D_8036C784[0] = 0;
    forced_modes++;
    host_log("replay: mode %016llX forced at the log's read %u (the port was in %016llX, level %u)\n",
             (unsigned long long)c->mode, k + 1, (unsigned long long)mode_now(), port_be32(D_802E8BDC));
    matched = (int)k - 1;       /* the next read: the log's read k+1 */
}

/* 00000.c's mode switch, before the new mode's init: the port goes where
   the movie's game went next (switches.csv, from where the log is); if that
   isn't where it would go, it goes there instead, with the state the movie
   had (force_mode). */
void port_replay_mode_switch(void) {
    if (!checkpoints || !switches || matched < 0)
        return;
    uint64_t want = (uint64_t)port_be32(D_80364A98) << 32 | port_be32(D_80364A98 + 4);
    while (next_switch < nswitches && switches[next_switch].read < (unsigned)matched + 1)
        next_switch++;
    if (next_switch >= nswitches)
        return;
    const Switch *w = &switches[next_switch];
    if (w->mode == want) {
        next_switch++;
        return;
    }
    host_log("replay: the port would switch to %016llX, the movie to %016llX; ", (unsigned long long)want,
             (unsigned long long)w->mode);
    /* the movie's change at the reads that this switch starts */
    for (unsigned k = w->read; k < nreads && k <= w->read + 2; k++)
        if (log_reads[k].mode != log_reads[k - 1].mode) {
            force_mode(k);
            return;
        }
    next_switch++;
    port_wg32(D_80364A98, (uint32_t)(w->mode >> 32));
    port_wg32(D_80364A98 + 4, (uint32_t)w->mode);
    host_log("(no change at the reads)\n");
}

/* a port frame with no match: once there have been GRACE of them, the log
   goes on by one, and at its next mode change the port is put there */
static void unmatched_frame(void) {
    if (!checkpoints || ++unmatched_run <= GRACE || matched + 2 >= (int)nreads)
        return;
    if (matched >= 0 && log_reads[matched + 1].mode != log_reads[matched].mode) {
        force_mode(matched + 1);
        unmatched_run = 0;
    } else
        matched++;
}

int port_pad_read_due(int free) {
    if (!log_reads || nogate)
        return free;
    if (si_started)
        return 1;
    uint64_t mode = mode_now();
    uint32_t frames = port_be32(D_80358064);
    for (unsigned k = matched + 1; k < nreads && k <= (unsigned)(matched + 1) + WINDOW; k++)
        if (log_reads[k].mode == mode && log_reads[k].frames == frames) {
            unmatched_run = 0;
            return 1;
        }
    if (matched + 1 >= (int)nreads)
        return free;                                /* past the log: as the game says */
    unmatched_frame();
    static unsigned skips;
    static int skips_at = -2;
    if (skips_at != matched) {
        skips_at = matched;
        skips = 0;
    }
    if (skips++ < 3 || host_verbose > 1)
        host_log("replay: no read in mode %016llX frame %u (the log's next: mode %016llX frame %u)\n",
                 (unsigned long long)mode, frames, (unsigned long long)log_reads[matched + 1].mode,
                 log_reads[matched + 1].frames);
    return 0;
}

/* The pak/EEPROM thread took its next command (replay_hooks.c): it goes on
   once the port has started the read after the one the movie's thread took
   it after, so that what it changes shows from the same frame.  (The
   thread spins on the scheduler before it takes one, 00000.c's saves
   waiting a varying number of frames.) */
extern char D_80370BF8[];       /* the SI's event queue (45BB0.c) */

int host_replay_save_due(int sync) {
    if (!log_reads || nogate || !save_starts)
        return 1;
    if (!save_waiting) {
        save_waiting = 1;
        saves++;
        if (host_verbose > 1 || getenv("PORT_REPLAY_SAVES"))
            host_log("replay: save command %u at the log's read %d (sync %d), the movie's after read %u\n", saves,
                     matched + 1, sync, saves <= nsave_starts ? save_starts[saves - 1] : 0);
    }
    /* not while a read's SI completion is held: the SI is busy then, and
       the thread would take the game's completion (func_8028A42C); nor
       while the game is receiving it, having seen it unfetched
       (D_80370C10): the thread would take it all the same and leave the
       game waiting for it forever.  The movie's thread (priority 11, above
       the game's 10) never went on between the game's check and its
       receive; this one, waking every millisecond, can. */
    if (si_waiting)
        return 0;
    if (host_receiving(PORT_ADDR(D_80370BF8))) {
        if (saves_held_si++ < 5 || host_verbose)
            host_log("replay: the save thread's command %u waits for the game's pad read (the log's read %d)\n",
                     saves, matched + 1);
        return 0;
    }
    if (sync) {             /* the game waits for its reply: now */
        save_waiting = 0;
        return 1;
    }
    if (saves > nsave_starts || matched + 1 > (int)save_starts[saves - 1] || matched + 1 >= (int)nreads) {
        save_waiting = 0;
        return 1;
    }
    /* the game may wait for it without blocking on the reply (polling a
       flag): if no read comes in 100 ms (the calls are a millisecond
       apart), it goes on anyway, and that is counted */
    static int wait_read = -2;
    static unsigned waited;
    if (wait_read != matched) {
        wait_read = matched;
        waited = 0;
    }
    if (++waited >= 100) {
        if (saves_early++ < 5 || host_verbose)
            host_log("replay: the save thread's command %u goes on at the log's read %d, the movie's took it "
                     "after read %u\n", saves, matched + 1, save_starts[saves - 1]);
        save_waiting = 0;
        waited = 0;
        return 1;
    }
    return 0;
}

/* the game's osGetCount seeds (23C20.c, 20460.c, under TARGET_PC) */
void port_replay_seeded(void) {
    seeded = 1;
}

/* the loop: the held SI completion, once the retraces are there; 1 if it
   was given */
int host_replay_poll_si(void) {
    /* by the retraces sent: the scheduler counts one only once it has run,
       and it counts every one */
    if (!si_waiting || (!nogate && port_vi_sent() < target))
        return 0;
    si_waiting = 0;
    base = port_vi_sent();
    next_known = cur_read && matched + 1 < (int)nreads;
    if (next_known)
        next_target = base + frame_retraces(matched + 1);
    port_replay_si_done();
    return 1;
}

/* the loop, at a retrace: 0 to hold it back */
int host_replay_vi_ok(void) {
    if (nogate)
        return 1;
    if (si_waiting)
        return port_vi_sent() < target;
    return !next_known || port_vi_sent() < next_target;
}

/* a held retrace given anyway: the game waits for one the log's frame
   didn't have */
void host_replay_vi_forced(void) {
    static int at = -2;
    static unsigned n;
    if (at != matched) {
        at = matched;
        n = 0;
    }
    if (++n == 50) {
        host_log("replay: 50 retraces given at the log's read %d; threads (save thread %s):\n", matched + 1,
                 save_waiting ? "held" : "not held");
        host_threads_dump();
    }
    if (forced++ < 10 || host_verbose)
        host_log("replay: read %u (the log's %d) waits for a retrace the log doesn't have\n",
                 reads + !si_waiting, matched + 1 + !si_waiting);
}

/* osContGetReadData: the pad of the last read started */
void host_replay_pad(uint16_t *buttons, int *x, int *y) {
    si_started = 0;
    *buttons = 0;
    *x = *y = 0;
    if (!cur_read)
        return;
    *buttons = (uint16_t)(cur_read->pad >> 16);
    *x = (int8_t)(cur_read->pad >> 8);
    *y = (int8_t)cur_read->pad;
}

/* past the end of the log, with time for the last save */
int host_replay_done(void) {
    static unsigned end;
    if (!log_reads || matched + 1 < (int)nreads)
        return 0;
    if (!end)
        end = reads;
    return reads >= end + 600;
}

void host_replay_report(void) {
    if (!log_reads)
        return;
    host_log("replay: %u reads, %d of the log's %u matched (%u skipped), %u without a match, "
             "%u retraces given anyway, %u random states set, the player elsewhere at %u, %u reads of the "
             "counts and %u audio answers the log doesn't have, %u save commands let go early, %u modes "
             "forced\n", reads, matched + 1 - (int)skipped, nreads, skipped, unmatched, forced, seeds, pos_diffs,
             counts_unlogged, audio_unlogged, saves_early, forced_modes);
}

