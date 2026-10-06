/*
 * The host program: memory map, cartridge, the "interrupt" loop.
 *
 * The game runs as fibers (threads.c) on the main OS thread.  This loop
 * runs whenever no game thread can: it delivers the events the hardware
 * would raise (VI retrace at 60 Hz, timers, SP/DP task completion) and
 * picks the next thread, by libultra's priority rules.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/personality.h>
#include <sys/prctl.h>
#endif
#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

#include "port.h"
#include "fiber.h"
#include "host.h"
#include "pack.h"
#include "hdtext.h"
#include "micons.h"

/* the version this port is built from (CMake's PORT_VERSION) */
#if defined(VERSION_US_V10)
#define ROM_DEFAULT "baserom.us.v10.z64"
#define ROM_CODE "NBCE"
#define ROM_REVISION 0
#define ROM_TITLE "Blast Corps (USA) v1.0"
#elif defined(VERSION_JP)
#define ROM_DEFAULT "baserom.jp.z64"
#define ROM_CODE "NBCJ"
#define ROM_REVISION 0
#define ROM_TITLE "Blastdozer (Japan)"
#elif defined(VERSION_EU)
#define ROM_DEFAULT "baserom.eu.z64"
#define ROM_CODE "NBCP"
#define ROM_REVISION 0
#define ROM_TITLE "Blast Corps (Europe)"
#else
#define ROM_DEFAULT "baserom.us.v11.z64"
#define ROM_CODE "NBCE"
#define ROM_REVISION 1
#define ROM_TITLE "Blast Corps (USA) v1.1"
#endif

#ifdef PORT_NATIVE_ENDIAN
/* game variables are read with port_be32 (and the pad written with
   port_wbe16) below: at their own width in RDRAM (port.h) */
#define port_be32(p) port_var32(p)
#define port_wbe16(p, v) port_wvar16((p), (v))
#endif

int host_verbose;
static uint8_t *rom;
static uint32_t rom_size;
static struct timespec t0;

void host_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void host_fatal(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "fatal: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    host_threads_dump();
    exit(1);
}

/* The kernel puts the brk heap at a random place above the image: up to
   32 MB above it for a 32-bit program, but up to 1 GB for a 64-bit one, and
   so, now and then, inside one of the windows below (the heap is there by
   main(), from what the libraries allocated first).  Then the port runs
   itself again without the randomization, which puts the heap right above
   the image.  (Once the windows are mapped, the heap can't grow into them:
   malloc goes to mmap instead.) */
static char **main_argv;

#ifndef PORT_MOVABLE
static void map_fixed(uint32_t addr, uint32_t size, const char *what) {
#ifdef MAP_FIXED_NOREPLACE
    void *p = mmap((void *)(uintptr_t)addr, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_NORESERVE, -1, 0);
#else
    /* elsewhere the address is only a hint: whatever is there stays */
    void *p = mmap((void *)(uintptr_t)addr, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p != MAP_FAILED && p != (void *)(uintptr_t)addr) {
        munmap(p, size);
        p = MAP_FAILED;
        errno = EEXIST;
    }
#endif
#ifdef __linux__
    if (p == MAP_FAILED && errno == EEXIST && !(personality(0xFFFFFFFF) & ADDR_NO_RANDOMIZE)) {
        if (host_verbose)
            host_log("%s at %08X taken (the heap?): running again without address randomization\n", what, addr);
        personality(personality(0xFFFFFFFF) | ADDR_NO_RANDOMIZE);
        execv("/proc/self/exe", main_argv);
        errno = EEXIST;
    }
#endif
    if (p == MAP_FAILED || p != (void *)(uintptr_t)addr)
        host_fatal("can't map %s at %08X: %s", what, addr, strerror(errno));
}
#endif

/* ---- cartridge ------------------------------------------------------------ */

static void load_rom(const char *path) {
    /* a resource pack (pack.c): the ROM's image made from its files */
    if (pack_is_pack(path)) {
        int edited;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        rom = pack_build_rom(path, &rom_size, &edited);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (host_verbose)
            host_log("pack %s: the ROM image made in %.0f ms (%s)\n", path,
                     (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6,
                     edited ? "edited" : "the original's, byte for byte");
        return;
    }
    FILE *f = fopen(path, "rb");
    if (!f)
        host_fatal("can't open ROM %s: %s", path, strerror(errno));
    fseek(f, 0, SEEK_END);
    rom_size = (uint32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    rom = malloc(rom_size);
    if (fread(rom, 1, rom_size, f) != rom_size)
        host_fatal("short read on %s", path);
    fclose(f);
    uint32_t magic = port_be32(rom);
    if (magic == 0x37804012u) {             /* .v64: byte-swapped halves */
        for (uint32_t i = 0; i + 1 < rom_size; i += 2) {
            uint8_t t = rom[i]; rom[i] = rom[i + 1]; rom[i + 1] = t;
        }
    } else if (magic == 0x40123780u) {      /* .n64: little-endian words */
        for (uint32_t i = 0; i + 3 < rom_size; i += 4) {
            uint8_t a = rom[i], b = rom[i + 1];
            rom[i] = rom[i + 3]; rom[i + 1] = rom[i + 2]; rom[i + 2] = b; rom[i + 3] = a;
        }
    } else if (magic != 0x80371240u) {
        host_fatal("%s is not an N64 ROM", path);
    }
    /* the version this port was built for, exactly (CMake's PORT_ROM_SHA1,
       from blastcorps.<version>.sha1).  The movable build takes the game's
       data from it (PORT_ROM_DATA), and checks what it made against what it
       was built with besides; PORT_ROM_ANY=1 lets another ROM through
       (an edited one) with a warning. */
    char sha1[41];
    host_sha1_hex(rom, rom_size, sha1);
    if (strcmp(sha1, PORT_ROM_SHA1) != 0) {
        const char *any = getenv("PORT_ROM_ANY");
        const char *code = memcmp(rom + 0x3B, ROM_CODE, 4) == 0 && rom[0x3F] == ROM_REVISION
            ? "it has " ROM_TITLE "'s header but other contents"
            : "it isn't " ROM_TITLE;
        if (any && atoi(any))
            host_log("warning: %s (sha1 %s): %s; this port is built for sha1 %s\n", path, sha1, code,
                     PORT_ROM_SHA1);
        else
            host_fatal("%s (sha1 %s): %s; this port is built for " ROM_TITLE ", sha1 %s "
                       "(PORT_ROM_ANY=1 tries it anyway)", path, sha1, code, PORT_ROM_SHA1);
    }
}

const uint8_t *host_rom(void) { return rom; }

uint32_t host_rom_size(void) { return rom_size; }

void host_rom_read(uint32_t dst, uint32_t addr, uint32_t len) {
    uint8_t *d = port_ptr(dst);
    addr &= 0x0FFFFFFFu;
    if (host_verbose > 1)
        host_log("pi: rom %06X -> %08X (%X)\n", addr, dst, len);
    for (uint32_t i = 0; i < len; i++)
        d[i] = addr + i < rom_size ? rom[addr + i] : 0;
#ifdef PORT_ACCESS_PROFILE
    port_access_dma(dst, addr, len);
#endif
    host_loaded_dma(dst, addr, len);
}

uint32_t host_rom_word(uint32_t addr) {
    addr &= 0x0FFFFFFCu;
    return addr + 4 <= rom_size ? port_be32(rom + addr) : 0;
}

/* ---- time ------------------------------------------------------------------ */

/* --deterministic: time only passes when every thread waits or is busy
   (threads.c): it jumps to the next event; runs as fast as the host can and
   the same every time */
static int deterministic;
/* the improvements on by default with a window (-1: neither --X nor --no-X) */
static int model_icons = -1;            /* --model-icons, --no-model-icons */
static int interp_arg = -1;             /* --interpolate, --no-interpolate */
static int hdtext_arg = -1;             /* --hd-text, --no-hd-text */
static int free_cam_arg = -1;           /* --free-camera, --no-free-camera */
static int display_hz_set;              /* --display-hz */
int host_is_deterministic(void) { return deterministic; }
static uint64_t virtual_ns;

/* PORT_PACED=1 (the page's default): virtual time between retraces, real
   time at them.  Inside a retrace the clock jumps from event to event as
   --deterministic's does, so the timers' and the RDP's waits cost the
   host nothing (the loop neither spins nor sleeps through them, and a
   coarse real clock -- Firefox's performance.now() is 1 ms -- doesn't
   stretch them); a retrace waits until real time has caught up with it, and in a browser
   until the display's next frame (paced_wait below). */
int host_paced;
static double real_off_ms;      /* a retrace at virtual v is due at real v + real_off_ms */
unsigned host_paced_resyncs;    /* the host fell behind: time dropped */

/* The RDP's time for the display list being run, as the renderer estimates
   it (host_charge from gfx.c), times PORT_RDP_SCALE (default 0: the RDP is
   instant, as in mupen64plus, which is what the pacing is matched to).
   ultra.c delays OS_EVENT_DP by it. */
static uint64_t rdp_ns;
static double rdp_scale = 0;

void host_charge(uint64_t ns) { rdp_ns += (uint64_t)(ns * rdp_scale); }
int host_rdp_scaled(void) { return rdp_scale != 0; }

uint64_t host_take_rdp_ns(void) {
    uint64_t ns = rdp_ns;
    rdp_ns = 0;
    return ns;
}

static double real_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec - t0.tv_sec) * 1e3 + (t.tv_nsec - t0.tv_nsec) / 1e6;
}

static uint64_t now_ns(void) {
    if (deterministic || host_paced)
        return virtual_ns;
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)(t.tv_sec - t0.tv_sec) * 1000000000ull + (uint64_t)(t.tv_nsec - t0.tv_nsec);
}

uint64_t host_now_ns(void) { return now_ns(); }

/* the CPU counter runs at half the 93.75 MHz clock */
uint64_t host_ticks(void) {
    return now_ns() * 3 / 64;
}

/* ---- events ---------------------------------------------------------------- */

static int pending[32];
static int npending;

void host_raise(int event) {
    if (npending < 32)
        pending[npending++] = event;
}

/* events due later (the RDP's full sync) */
static struct { int event; uint64_t at; } later[16];
static int nlater;

void host_raise_at(int event, uint64_t at_ns) {
    if (at_ns <= now_ns() || nlater == 16) {
        host_raise(event);
        return;
    }
    later[nlater].event = event;
    later[nlater++].at = at_ns;
}

static uint64_t raise_due(uint64_t now) {
    uint64_t next = ~0ull;
    for (int i = 0; i < nlater;) {
        if (later[i].at <= now) {
            host_raise(later[i].event);
            later[i] = later[--nlater];
        } else {
            if (later[i].at < next)
                next = later[i].at;
            i++;
        }
    }
    return next;
}

static void deliver_pending(void) {
    while (npending) {
        int ev = pending[0];
        memmove(pending, pending + 1, --npending * sizeof pending[0]);
        port_irq_event(ev);
    }
}

/* --load-waits n64: the N64's hardware waits (port.h); off by default */
static int load_waits;
int host_load_waits(void) { return load_waits; }

#ifdef PORT_HAVE_ASYNCIFY
#include <emscripten.h>
/* The page's one thread is the loop's (fiber_asyncify.c): it gets it back
   (emscripten_sleep: Asyncify unwinds the loop, the browser shows the
   frame and takes input, a timeout rewinds it) in place of the loop's
   sleeps, and after a retrace when it hasn't for 12 ms, so a busy game
   doesn't hold the page. */
static void yield_to_page(uint64_t ns) {
    static double last;
    double t = emscripten_get_now();
    host_perf_push(PERF_IDLE);
    if (ns >= 1000000)
        emscripten_sleep((unsigned)(ns / 1000000));
    else if (t - last >= 12.0)
        emscripten_sleep(0);
    else {
        host_perf_pop();
        return;
    }
    last = emscripten_get_now();
    host_perf_pop();
    host_perf_sleep(ns / 1e6, last - t);
}
#endif

extern unsigned long long gfx_st_images;         /* gfx_gl.c: retraces that showed a new picture */

#ifdef PORT_WASM_WEB
/* A retrace waits for the display: the first animation frame from
   deadline - early on (the frame the picture will be shown in), or a timer
   if none comes (a hidden page has none).  Returns when it resumed. */
EM_ASYNC_JS(double, wait_display, (double deadline, double early), {
    return new Promise(function (resolve) {
        var done = false, timer = 0;
        function fin() {
            if (done) return;
            done = true;
            clearTimeout(timer);
            resolve(performance.now());
        }
        function frame() {
            if (performance.now() >= deadline - early) fin();
            else requestAnimationFrame(frame);
        }
        requestAnimationFrame(frame);
        timer = setTimeout(fin, Math.max(0, deadline - performance.now()) + 4 * early);
    });
});

/* the page's turn now (input, sound, the frame just drawn), with none of
   setTimeout's 4 ms clamp: a message to ourselves */
EM_ASYNC_JS(void, yield_now, (void), {
    if (!Module.yieldChannel) {
        Module.yieldChannel = new MessageChannel();
        Module.yieldChannel.port1.onmessage = function () { var r = Module.yieldResolve; Module.yieldResolve = null; r(); };
    }
    return new Promise(function (resolve) {
        Module.yieldResolve = resolve;
        Module.yieldChannel.port2.postMessage(0);
    });
});

static double last_yield_ms;     /* when the page last had its turn */
#endif

/* PORT_PACED: long stretches of work give the page its turn now and then */
static void paced_breathe(void) {
#ifdef PORT_WASM_WEB
    double t = emscripten_get_now();
    if (t - last_yield_ms < 8.0)
        return;
    host_perf_push(PERF_IDLE);
    yield_now();
    last_yield_ms = emscripten_get_now();
    host_perf_pop();
#endif
}

/* PORT_PACED: the retrace (or with --display-hz, the display's tick: lock
   0) at virtual v is next: wait for real time (and the display) to get
   there */
static double wait_from_ms;
static void paced_wait(uint64_t v, uint64_t period, int lock) {
    const double pms = period / 1e6;
    double deadline = v / 1e6 + real_off_ms, t = real_ms();
    if (lock)
        wait_from_ms = t;
    if (lock && t > deadline + 4 * pms) {           /* fell behind: don't catch up */
        real_off_ms = t - v / 1e6;
        host_paced_resyncs++;
        deadline = t;
    }
    host_perf_push(PERF_IDLE);
#ifdef PORT_WASM_WEB
    /* (emscripten's clock is performance.now()'s, from t0) */
    double base = emscripten_get_now() - t, early = pms / 4;
    if (t < deadline - early) {
        double at = wait_display(base + deadline, early) - base;
        /* follow the display: retraces drift towards the frames that show
           them, a little at a time (a 59.94 Hz display, or one at 120) */
        double err = at - deadline, adj = lock ? err * 0.05 : 0;
        if (adj > 0.2) adj = 0.2;
        if (adj < -0.2) adj = -0.2;
        real_off_ms += adj;
        host_perf_sleep(deadline - t, at - t);
    } else if (emscripten_get_now() - last_yield_ms >= 8.0) {
        yield_now();                                /* late: show it now, but let the page in */
    }
    last_yield_ms = emscripten_get_now();
#else
    if (t < deadline) {
        struct timespec ts = { (time_t)((deadline - t) / 1e3), (long)(fmod(deadline - t, 1e3) * 1e6) };
        nanosleep(&ts, NULL);
        host_perf_sleep(deadline - t, real_ms() - t);
    }
#endif
    host_perf_pop();
    virtual_ns = v;
}

/* PORT_ADAPT=1 (the page's default; gfx_gl.c follows the GPU with the
   resolution): when more than 5% of the retraces of a two-second window
   came over 2 ms late, the host isn't keeping up, and --interpolate's
   in-between images go first (each costs about what the frame's own pass
   does): half of them at a time (7, 3, 1, then none: the frame shown for
   all its retraces), and after a while without late retraces twice as
   many again.  A step down holds for 20 s at first, twice as long each
   time it had to be taken again within 30 s of the step back up (up to
   10 min).  Late retraces while the GPU is behind don't count: those are
   the resolution's to fix.  The game never waits for the images either
   way. */
int host_interp_limit = 99;         /* in-between images a frame, at most (gfx.c) */
static int adapt_on = -1;
#ifdef PORT_HAVE_GL
int gfx_gl_gpu_behind(void);
#else
static int gfx_gl_gpu_behind(void) { return 0; }
#endif

static void lag_account(double late_ms) {
    static int n, late;
    static double hold = 20000, since, raised = -1e9;
    if (adapt_on < 0) {
        const char *e = getenv("PORT_ADAPT");
#ifdef PORT_WASM_WEB
        adapt_on = !e || (*e && *e != '0');
#else
        adapt_on = e && *e && *e != '0';
#endif
    }
    if (!adapt_on || !gfx_interp || deterministic)
        return;
    late += late_ms > 2.0 && !gfx_gl_gpu_behind();     /* (the GPU's: gfx_gl.c lowers the resolution) */
    if (++n < 120)
        return;
    double now = real_ms();
    int used = gfx_interp_twins_used();
    if (used > host_interp_limit)
        used = host_interp_limit;
    if (late * 20 > n && used > 0) {
        if (now - raised < 30000 && hold < 600000)
            hold *= 2;
        host_interp_limit = used / 2;
        since = now;
        host_log("pacing: %d of %d retraces late: at most %d in-between images a frame for %.0f s\n", late, n,
                 host_interp_limit, hold / 1e3);
    } else if (host_interp_limit < 99 && late == 0 && now - since > hold) {
        host_interp_limit = host_interp_limit ? host_interp_limit * 2 + 1 : 1;
        if (host_interp_limit >= 7)
            host_interp_limit = 99;
        raised = since = now;
        host_log("pacing: at most %d in-between images a frame again\n", host_interp_limit);
    }
    n = late = 0;
}

/* PORT_QUEUE (PORT_PACED, the OpenGL renderer; docs/PORT.md, "The fifth
   round"): a retrace whose work runs over its display frame misses it, and
   the frame before is shown twice and this one for a frame less, though the
   next retrace's work is short.  With the queue, each retrace's picture is
   held and presented a retrace later, at its own display frame (slot):
   while the next retrace's work runs if that runs over (present_due, from
   the loop), so a retrace's work may take up to two frames as long as the
   one after is short.  The game's timing is as it was; the pictures come a
   retrace (16.7 ms) later.  0: off; 2: always; 1 (the default when paced):
   on when 3 retraces of 2 s came over 2 ms late, off again after 20 s in
   which no retrace's work took over 80% of a frame. */
static int queue_mode = -1;
static int queue_on;
int host_queue_on;                      /* (for PORT_PERF) */
static double held_due_ms;              /* the held picture's slot (real ms) */
static double work_from_ms;             /* when this retrace's work began */
static double queue_last_late;          /* how late the last present from the queue came (ms) */
unsigned long long host_queue_presents, host_queue_late;    /* presents from the queue, those after their slot */

static void queue_init(int between) {
    const char *e = getenv("PORT_QUEUE");
    queue_mode = e ? atoi(e) : 1;
    if (!host_paced || deterministic || between || host_renderer != 1)
        queue_mode = 0;
    queue_on = host_queue_on = queue_mode == 2;
}

/* the queue's present, if its slot has come (early: how long before it a
   present still lands in its frame) */
static void present_due(double early) {
    if (!host_video_held() || real_ms() < held_due_ms - early)
        return;
    host_perf_push(PERF_PRESENT);
    host_queue_presents++;
    queue_last_late = real_ms() - held_due_ms;
    host_queue_late += queue_last_late > 2.0;
    host_video_present_held();
    host_perf_pop();
#ifdef PORT_WASM_WEB
    host_perf_push(PERF_IDLE);              /* (the browser shows it when it has its turn) */
    yield_now();
    last_yield_ms = emscripten_get_now();
    host_perf_pop();
#endif
}

/* at a retrace, before its present: late_ms after its slot; work_ms its work */
static void queue_account(double late_ms, double work_ms, double pms) {
    static int n, late;
    static double calm_since;
    if (queue_mode != 1)
        return;
    double now = real_ms();
    if (!queue_on) {
        late += late_ms > 2.0;
        if (late >= 3) {
            queue_on = host_queue_on = 1;
            calm_since = now;
            host_log("pacing: %d retraces late in 2 s: each picture presented a retrace later\n", late);
        }
    } else {
        if (work_ms > 0.8 * pms)
            calm_since = now;
        else if (now - calm_since > 20000) {
            queue_on = host_queue_on = 0;
            host_log("pacing: the pictures presented at their retraces again\n");
        }
    }
    if (++n >= 120)
        n = late = 0;
}

void port_trace_poll(void);     /* runtime.c: PORT_TRACE counts controller reads */
/* the scheduler's retrace count, the game's frame count and the mode */
extern char D_803156C4[], D_80358064[], D_80364A90[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_803156C4 PORT_VAR(D_803156C4)
#define D_80358064 PORT_VAR(D_80358064)
#define D_80364A90 PORT_VAR(D_80364A90)
#endif

/* PORT_DUMP=N,M,...: RDRAM to rdram_N.bin at the Nth controller read
   (compare with tools/recomp/test/snapshot.c, which counts the same way) */
void host_controller_poll(void) {
    static unsigned polls;
    static const char *spec = (const char *)1;
    static FILE *pace;
    port_trace_poll();
    if (spec == (const char *)1) {
        spec = getenv("PORT_DUMP");
        /* PORT_PACE=FILE: what port/tools/m64p_pace.c records in mupen64plus */
        const char *p = getenv("PORT_PACE");
        if (p && (pace = fopen(p, "w")))
            fprintf(pace, "poll,retraces,frames,mode,audio_samples\n");
    }
    polls++;
    host_digest_poll(polls);    /* PORT_DIGEST=FILE: the gameplay digest (digest.c) */
#ifdef PORT_ENGINE_CHECK
    {
        void engine_fuzz_poll(unsigned polls);  /* (port/host/engine.c) */
        engine_fuzz_poll(polls);
    }
#endif
    if (pace)
        fprintf(pace, "%u,%u,%u,%08X%08X,%llu\n", polls, port_be32(D_803156C4),
                port_be32(D_80358064), port_be32(D_80364A90),
                port_be32(D_80364A90 + 4), (unsigned long long)host_audio_samples());
    if (!spec)
        return;
    for (const char *p = spec; *p;) {
        unsigned n = (unsigned)strtoul(p, (char **)&p, 10);
        if (n == polls) {
            char path[64];
            snprintf(path, sizeof path, "rdram_%u.bin", n);
            FILE *f = fopen(path, "wb");
            if (f) {
                fwrite(port_ptr(PORT_RDRAM_BASE), 1, PORT_RDRAM_SIZE, f);
                fclose(f);
                host_log("dumped %s\n", path);
#ifdef PORT_ACCESS_PROFILE
                port_access_dump_widths(n);     /* rdram_N.widths, for build_cmp.py */
#endif
            }
        }
        if (*p == ',')
            p++;
        else
            break;
    }
}

/* on a crash, RDRAM goes to rdram_crash.bin (PORT_DUMP set) for comparing
   (POSIX signals; emscripten has none to catch) */
#ifndef __EMSCRIPTEN__
static void on_crash(int sig, siginfo_t *si, void *uc) {
    (void)uc;
    fprintf(stderr, "crash: signal %d at address %p\n", sig, si->si_addr);
    if (getenv("PORT_DUMP")) {
        FILE *f = fopen("rdram_crash.bin", "wb");
        if (f) {
            fwrite(port_ptr(PORT_RDRAM_BASE), 1, PORT_RDRAM_SIZE, f);
            fclose(f);
        }
    }
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

/* ---- main ------------------------------------------------------------------ */

extern void port_boot(void);
extern void port_fixups(void);
extern void port_arena_init(void);   /* runtime.c, PORT_MOVABLE */

/* --hd-text's optional FONT: a name ending in a font's extension (so that
   "--hd-text ROM" still takes ROM as the ROM) */
static int hdtext_font_arg(const char *a) {
    size_t n = strlen(a);
    static const char *ext[] = { ".ttf", ".otf", ".TTF", ".OTF", ".ttc", ".TTC" };
    for (unsigned k = 0; k < sizeof ext / sizeof ext[0]; k++)
        if (n > 4 && !strcmp(a + n - 4, ext[k]))
            return 1;
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr,
            "usage: %s [options] [ROM | PACK]\n"
            "  ROM                  defaults to " ROM_DEFAULT "\n"
            "  PACK, --pack PACK    a resource pack (a .zip from port/make_pack.py, or its\n"
            "                       unpacked directory) instead of the ROM\n"
            "  -v                   log threads, events and tasks (twice: more)\n"
            "  --headless           no window (SDL's offscreen driver)\n"
            "  --deterministic      virtual time: as fast as possible, the same every run\n"
            "  --replay FILE        play a movie's reads (m64p_tas's polls.csv; implies --deterministic)\n"
            "  --frames N           quit after N retraces\n"
            "  --screenshot PREFIX  save the last frame as PREFIXnnnnn.bmp\n"
            "                       (PORT_SHOT_EVERY=N: every N frames too)\n"
            "  --save PATH          EEPROM file (default blastcorps.eep)\n"
            "  --renderer gl|sw     OpenGL or the software renderer (default: gl with a\n"
            "                       window, sw headless)\n"
            "  --scale N            gl: internal resolution 320x240 times N (default: the\n"
            "                       window's)\n"
            "  --filter F           textures: n64 (3-point, default), bilinear or point\n"
            "  --interpolate, --no-interpolate\n"
            "                       60 frames a second (or the display's rate) where the\n"
            "                       game draws 30, by showing frames between each two (the\n"
            "                       game is unchanged; default with a window)\n"
            "  --aspect A           the picture's shape: W:H (e.g. 16:9, 21:9; 4:3 is the\n"
            "                       N64's) or 'window', the window's as it is resized (the\n"
            "                       default with a window; 4:3 headless, --deterministic\n"
            "                       and --replay); wider than 4:3 shows more of the 3D world\n"
            "  --widescreen         --aspect 16:9\n"
            "  --hud edges|centre   wider than 4:3: the HUD at the picture's sides (default)\n"
            "                       or where the game puts it, in the 4:3 middle\n"
            "  --max-pixels N       gl: lower the internal resolution until a picture has\n"
            "                       at most N pixels (the page passes 1300000)\n"
            "  --hd-text [FONT], --no-hd-text\n"
            "                       gl: the game's text drawn from a font at the internal\n"
            "                       resolution (built in: Stardos Stencil; FONT: a .ttf/.otf;\n"
            "                       default with a window; PORT_HD_TEXT=0|1|FONT)\n"
            "  --model-icons, --no-model-icons\n"
            "                       the icons that are pictures of the game's models (the\n"
            "                       hint panels', the goals', the vehicles', the world\n"
            "                       map's chopper) drawn as the models (default with a\n"
            "                       window; off headless, with\n"
            "                       --deterministic and --replay; PORT_MODEL_ICONS=0|1)\n"
            "  --free-camera, --no-free-camera\n"
            "                       in a level, dragging with the left mouse button and\n"
            "                       the right stick turn and tilt the camera (C-left/right\n"
            "                       still turn it 45 degrees, the stick's click is C-down;\n"
            "                       default with a window; PORT_FREE_CAMERA=0|1,\n"
            "                       PORT_CAMERA_SENS: the mouse's degrees a pixel, 0.2)\n"
            "  --display-hz N|auto  --interpolate for a display this fast (default auto, the\n"
            "                       display's, with a window; else 60): more in-between\n"
            "                       images, shown between retraces by the host clock (not\n"
            "                       with --deterministic)\n"
            "  --wav PATH           write the sound to a WAV file too\n"
            "  --no-audio           no sound (--headless and --deterministic imply it)\n"
            "  --load-waits off|n64 off (the default): no waits for hardware the port doesn't\n"
            "                       have (the controllers' power-on, the EEPROM's writes, the\n"
            "                       pak thread's SI pacing, decompression); n64: as on the\n"
            "                       N64 (PORT_LOAD_WAITS=n64 too)\n"
            "environment: PORT_AUTOSTART=1 taps Start/A; PORT_DUMP=N,... writes RDRAM\n"
            "at the Nth controller read (and on a crash); PORT_PACE=FILE logs the pacing\n"
            "per controller read; PORT_PERF=N logs where the host's time goes every N retraces; PORT_PACED=1 virtual time\n"
            "between retraces; PORT_ADAPT=1 lower resolution and no in-between pictures\n"
            "when the host can't keep up (both the browser's default)\n", argv0);
    exit(2);
}

int main(int argc, char **argv) {
    const char *rom_path = ROM_DEFAULT;
    const char *load_waits_arg = NULL;
    int aspect_set = 0;
    main_argv = argv;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v"))
            host_verbose++;
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            host_max_frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            host_screenshot_prefix = argv[++i];
        else if (!strcmp(argv[i], "--save") && i + 1 < argc)
            host_save_path = argv[++i];
        else if (!strcmp(argv[i], "--wav") && i + 1 < argc)
            host_wav_path = argv[++i];
        else if (!strcmp(argv[i], "--no-audio"))
            host_audio_enabled = 0;
        else if (!strcmp(argv[i], "--deterministic"))
            deterministic = 1;
        else if (!strcmp(argv[i], "--load-waits") && i + 1 < argc)
            load_waits_arg = argv[++i];
        else if (!strcmp(argv[i], "--replay") && i + 1 < argc) {
            host_replay_load(argv[++i]);
            deterministic = 1;
        }
        else if (!strcmp(argv[i], "--headless"))
            host_headless = 1;
        else if (!strcmp(argv[i], "--pack") && i + 1 < argc)
            rom_path = argv[++i];
        else if (!strcmp(argv[i], "--renderer") && i + 1 < argc) {
            i++;
            host_renderer = !strcmp(argv[i], "gl") ? 1 : !strcmp(argv[i], "sw") ? 0 : (usage(argv[0]), 0);
        } else if (!strcmp(argv[i], "--scale") && i + 1 < argc)
            gfx_gl_scale = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--filter") && i + 1 < argc) {
            i++;
            gfx_filter = !strcmp(argv[i], "n64") ? 0 : !strcmp(argv[i], "point") ? 1
                       : !strcmp(argv[i], "bilinear") ? 2 : (usage(argv[0]), 0);
        }
        else if (!strcmp(argv[i], "--interpolate") || !strcmp(argv[i], "--no-interpolate"))
            interp_arg = argv[i][2] == 'i';
        else if (!strcmp(argv[i], "--display-hz") && i + 1 < argc) {
            i++;
            display_hz_set = 1;
            gfx_interp_hz = !strcmp(argv[i], "auto") ? -1 : atoi(argv[i]);
            if (gfx_interp_hz == 0 || gfx_interp_hz > 1000)
                usage(argv[0]);
        } else if (!strcmp(argv[i], "--widescreen")) {
            gfx_aspect = 16.0f / 9;
            aspect_set = 1;
        } else if (!strcmp(argv[i], "--hud") && i + 1 < argc) {
            i++;
            gfx_hud_edges = !strcmp(argv[i], "edges") ? 1
                          : !strcmp(argv[i], "centre") || !strcmp(argv[i], "center") ? 0 : (usage(argv[0]), 0);
        } else if (!strcmp(argv[i], "--max-pixels") && i + 1 < argc)
            gfx_gl_max_pixels = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--model-icons") || !strcmp(argv[i], "--no-model-icons"))
            model_icons = argv[i][2] == 'm';
        else if (!strcmp(argv[i], "--free-camera") || !strcmp(argv[i], "--no-free-camera"))
            free_cam_arg = argv[i][2] == 'f';
        else if (!strcmp(argv[i], "--no-hd-text"))
            hdtext_arg = 0;
        else if (!strcmp(argv[i], "--hd-text")) {
            hdtext_arg = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-' && hdtext_font_arg(argv[i + 1]))
                hdtext_font_path = argv[++i];
        }
        else if (!strcmp(argv[i], "--aspect") && i + 1 < argc) {
            const char *a = argv[++i];
            double w, h;
            aspect_set = 1;
            if (!strcmp(a, "window"))
                gfx_aspect = GFX_ASPECT_WINDOW;
            else if (sscanf(a, "%lf:%lf", &w, &h) == 2 && w > 0 && h > 0)
                gfx_aspect = (float)(w / h);
            else if (sscanf(a, "%lf", &w) == 1 && w > 0)
                gfx_aspect = (float)w;
            else
                usage(argv[0]);
        } else if (argv[i][0] == '-')
            usage(argv[0]);
        else
            rom_path = argv[i];
    }
    if (deterministic || host_headless)
        host_audio_enabled = 0;
    /* a window's picture follows its shape, unless asked otherwise; runs
       that are compared (headless, deterministic, replays) stay 4:3 */
    if (!aspect_set && !deterministic && !host_headless)
        gfx_aspect = GFX_ASPECT_WINDOW;
#ifdef PORT_WASM_WEB
    host_paced = 1;
#endif
    const char *pe = getenv("PORT_PACED");
    if (pe)
        host_paced = *pe && *pe != '0';
    if (deterministic)
        host_paced = 0;
    /* PORT_HD_TEXT=1 (or a font file), PORT_HD_TEXT_WEIGHT=W: --hd-text, the
       glyphs' ink W times the game's (the page has no command line) */
    const char *hd = getenv("PORT_HD_TEXT");
    if (hd && *hd) {
        hdtext_arg = strcmp(hd, "0") != 0;
        if (hdtext_arg && strcmp(hd, "1"))
            hdtext_font_path = hd;
    }
    const char *mi = getenv("PORT_MODEL_ICONS");
    if (mi && *mi)
        model_icons = strcmp(mi, "0") != 0;
    /* the port's improvements are on by default with a window: in-between
       frames for the display's rate, the text from a font, the models for
       their pictures, the free camera; runs that are compared (headless,
       deterministic, replays) have them only when asked for */
    int windowed = !deterministic && !host_headless;
    gfx_interp = interp_arg >= 0 ? interp_arg : windowed;
    if (windowed && !display_hz_set)
        gfx_interp_hz = -1;                 /* --display-hz auto */
    hdtext_on = hdtext_arg >= 0 ? hdtext_arg : windowed;
    micons_on = model_icons >= 0 ? model_icons : windowed;
    const char *fc = getenv("PORT_FREE_CAMERA");
    if (fc && *fc && free_cam_arg < 0)
        free_cam_arg = strcmp(fc, "0") != 0;
    host_free_camera = free_cam_arg >= 0 ? free_cam_arg : windowed;
    const char *sens = getenv("PORT_CAMERA_SENS");
    if (sens && atof(sens) > 0)
        host_camera_sens = (float)atof(sens);
    const char *hw = getenv("PORT_HD_TEXT_WEIGHT");
    if (hw && atof(hw) > 0)
        hdtext_weight = (float)atof(hw);
    const char *rs = getenv("PORT_RDP_SCALE");
    if (rs)
        rdp_scale = atof(rs);
    /* the N64's hardware waits (docs/PORT.md, "The front end's waits"): off
       by default */
    if (!load_waits_arg)
        load_waits_arg = getenv("PORT_LOAD_WAITS");
    if (load_waits_arg && *load_waits_arg)
        load_waits = !strcmp(load_waits_arg, "n64") ? 1
                   : !strcmp(load_waits_arg, "off") ? 0 : (usage(argv[0]), 0);
#ifdef __linux__
    prctl(PR_SET_TIMERSLACK, 1);    /* wake on time: the pacing is in 50 us steps */
#endif
    clock_gettime(CLOCK_MONOTONIC, &t0);
    host_perf_init();
#ifndef __EMSCRIPTEN__
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_crash;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    static char altstack[65536];
    stack_t ss = { .ss_sp = altstack, .ss_size = sizeof altstack };
    sigaltstack(&ss, NULL);
#endif
    /* RDRAM itself is the image's .rdram section (tools/gen_ld.py) */
#ifndef PORT_MOVABLE      /* (the movable build maps nothing fixed: the stacks are in the arena, runtime.c) */
    map_fixed(PORT_HWREG_BASE, PORT_HWREG_SIZE, "hardware registers");
    map_fixed(PORT_STACK_BASE, PORT_STACK_SIZE * PORT_MAX_THREADS, "thread stacks");
#endif
    fiber_init();
    load_rom(rom_path);
#ifdef PORT_MOVABLE
    port_arena_init();
#else
    port_fixups();
#endif
    host_video_init();
    hdtext_init();
    if (deterministic && gfx_interp_hz > PORT_RETRACE_HZ) {
        host_log("--display-hz: presents between retraces follow the host clock; not with --deterministic\n");
        gfx_interp_hz = PORT_RETRACE_HZ;
    }

    port_boot();

    /* the loop: deliver what's due, run threads until all wait, sleep */
    const uint64_t vi_period = 1000000000ull / PORT_RETRACE_HZ;
    uint64_t next_vi = now_ns() + vi_period;
    int vi_force = 0;
    int timers_woke = 0;        /* (real time) a timer fired, and the threads haven't all waited since */
    uint64_t vi_wait_from = 0;  /* (real time) when the due retrace began waiting for them */
    const int realtime = !deterministic && !host_paced;
    /* --display-hz above the retraces' rate: presents between them, by the host clock */
    int between = gfx_interp && gfx_interp_hz > PORT_RETRACE_HZ && !deterministic;
    uint64_t disp_period = between ? 1000000000ull / (uint64_t)gfx_interp_hz : 0;
    uint64_t last_vi = now_ns(), next_disp = between ? last_vi + disp_period : ~0ull;
    const double vi_pms = vi_period / 1e6;
    queue_init(between);
    for (;;) {
        if (queue_on)
#ifdef PORT_WASM_WEB
            present_due(vi_pms / 4);
#else
            present_due(0);
#endif
        deliver_pending();
        if (host_replay_poll_si())
            continue;
        uint64_t now = now_ns();
        /* --replay: a retrace waits while the count is where the log's next
           read has it (replay.c) */
        int vi_held = host_replay_active() && !host_replay_vi_ok() && !vi_force;
        /* In real time, after the host stalled (a font built, shaders
           compiled: the GPU's work runs inside the scheduler's turn, where
           the N64's RSP ran beside it), the scheduler took a second even
           retrace before the audio thread had taken the frame message
           the first one's timer (6 ms on) sent: the second set the timer
           again, the audio thread had two frames for one task, and waited
           for good on a task nobody started (no sound from then on).  On
           the N64 the threads a timer wakes run at once, and the next
           retrace is 16 ms away.  So here a timer that is due goes before
           the retrace, the threads timers woke run before it (a quarter
           of a period at most: a thread may spin), and retraces catching
           up are half a period apart.  (In virtual time the loop never
           gets past a timer.) */
        uint64_t vi_at = next_vi;
        if (realtime && !vi_force && vi_at < last_vi + vi_period / 2)
            vi_at = last_vi + vi_period / 2;
        if (now >= vi_at && !vi_held) {
            if (!vi_wait_from)
                vi_wait_from = now;
            if (realtime && now < vi_wait_from + vi_period / 4) {
                if (port_irq_timers(0) <= host_ticks()) {
                    port_irq_timers(host_ticks());
                    timers_woke = 1;
                    continue;
                }
                if (timers_woke && (npending || host_run_one()))
                    continue;
            }
            timers_woke = 0;
            vi_wait_from = 0;
            double late_ms = host_paced ? real_ms() - (next_vi / 1e6 + real_off_ms) : (now - next_vi) / 1e6;
            if (host_perf_on)
                host_perf_vi(late_ms, gfx_st_images, port_be32(D_80358064));
            /* (with the queue, what is late is a present after its slot:
               the last retrace's, whose slot is now at the latest) */
            if (queue_on)
                present_due(1e9);
            lag_account(queue_on ? queue_last_late : late_ms);
            if (queue_mode > 0) {
                double end = wait_from_ms >= 0 ? wait_from_ms : real_ms();     /* (it waited, or ran over) */
                queue_account(late_ms, end - work_from_ms, vi_pms);
                if (!queue_on)
                    present_due(1e9);                       /* (left the queue: what it held, now) */
            }
            vi_force = 0;
            next_vi += vi_period;
            if (now > next_vi + 4 * vi_period)      /* fell behind: don't catch up */
                next_vi = now + vi_period;
            host_perf_push(PERF_PRESENT);
            if (queue_on) {
                host_video_frame_hold();
                held_due_ms = next_vi / 1e6 + real_off_ms;  /* (next_vi: the next retrace's time now) */
            } else {
                host_video_frame();
            }
            host_perf_pop();
            hdtext_idle();
            last_vi = now;
#ifdef PORT_WASM_WEB
            if (host_paced) {                /* the frame goes to the screen before the next one's work */
                host_perf_push(PERF_IDLE);
                yield_now();
                last_yield_ms = emscripten_get_now();
                host_perf_pop();
            } else
#endif
#ifdef PORT_HAVE_ASYNCIFY
            yield_to_page(0);
#endif
            if (host_quit_requested())
                break;
            work_from_ms = real_ms();
            wait_from_ms = -1;
            port_irq_vi();
            continue;
        }
        if (now >= next_disp) {
            /* the display's clock runs on its own; a tick within half a
               period of a retrace is that retrace's present */
            double phase = (double)(now - last_vi) / vi_period;
            next_disp += disp_period;
            if (next_disp < now)
                next_disp = now + disp_period;
            if (now - last_vi >= disp_period / 2 && next_vi - now >= disp_period / 2 && phase < 1)
                host_video_between(phase);
            continue;
        }
        if (realtime && port_irq_timers(0) <= host_ticks())
            timers_woke = 1;
        uint64_t deadline = port_irq_timers(host_ticks());
        /* a held retrace is given anyway when the game can't go on without
           one: nothing to run and nothing due (below), or a thread spinning
           on the count (port_spin_wait) when it is due */
        int spinning = host_spinning();
        if (vi_held && spinning && now >= vi_at) {
            host_replay_vi_forced();
            next_vi = now;
            vi_force = 1;
            continue;
        }
        uint64_t wake = vi_held && !spinning ? ~0ull : vi_at;
        if (next_disp < wake)
            wake = next_disp;
        /* (rounded up: at deadline * 64 / 3 the counter may not be there yet) */
        if (deadline != ~0ull && (deadline * 64 + 2) / 3 < wake)
            wake = (deadline * 64 + 2) / 3;
        uint64_t due = raise_due(now);
        if (due < wake)
            wake = due;
        if (npending)
            continue;
        if (host_run_one())
            continue;
        timers_woke = 0;
        now = now_ns();
        if (vi_held && wake == ~0ull) {
            host_replay_vi_forced();
            next_vi = now;
            vi_force = 1;
            continue;
        }
        if (deterministic) {
            if (wake > virtual_ns)
                virtual_ns = wake;
            continue;
        }
        if (host_paced) {
            if (wake >= next_vi)
                paced_wait(next_vi, vi_period, 1);
            else if (wake == next_disp)             /* --display-hz: a present between retraces, in real time */
                paced_wait(next_disp, disp_period, 0);
            else {
                if (wake > virtual_ns)
                    virtual_ns = wake;
                paced_breathe();
            }
            continue;
        }
        if (wake > now + 50000) {
#ifdef PORT_HAVE_ASYNCIFY
            yield_to_page(wake - now);
#else
            struct timespec ts = { (time_t)((wake - now) / 1000000000ull), (long)((wake - now) % 1000000000ull) };
            host_perf_push(PERF_IDLE);
            nanosleep(&ts, NULL);
            host_perf_pop();
            if (host_perf_on)
                host_perf_sleep((wake - now) / 1e6, (now_ns() - now) / 1e6);
#endif
        }
    }
    if (host_verbose) {
        extern void host_gfx_dump_stats(void);
        host_gfx_dump_stats();
    }
    host_replay_report();
    host_gfx_interp_report();
    host_audio_shutdown();
    host_video_shutdown();
    return 0;
}
