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

/* ---- cartridge ------------------------------------------------------------ */

static void load_rom(const char *path) {
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
    if (memcmp(rom + 0x3B, ROM_CODE, 4) != 0 || rom[0x3F] != ROM_REVISION)
        host_log("warning: %s doesn't look like " ROM_TITLE "; expect trouble\n", path);
}

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
static uint64_t virtual_ns;

/* The RDP's time for the display list being run, as the renderer estimates
   it (host_charge from gfx.c), times PORT_RDP_SCALE (default 0: the RDP is
   instant, as in mupen64plus, which is what the pacing is matched to).
   ultra.c delays OS_EVENT_DP by it. */
static uint64_t rdp_ns;
static double rdp_scale = 0;

void host_charge(uint64_t ns) { rdp_ns += (uint64_t)(ns * rdp_scale); }

uint64_t host_take_rdp_ns(void) {
    uint64_t ns = rdp_ns;
    rdp_ns = 0;
    return ns;
}

static uint64_t now_ns(void) {
    if (deterministic)
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

/* BEPass calls this on every loop back edge of the game's C: a thread that
   spins waiting for something another thread or an interrupt changes gives
   way when the next event is due. */
static uint64_t next_event_ns;

/* --replay: the loop holds a retrace back (replay.c), and how often a
   thread spun while it did */
static int replay_vi_held;
static unsigned spins_held;

int port_ints_masked;

void __port_poll(void) {
    static unsigned n;
    if (++n & 63 || port_ints_masked)
        return;
    host_cpu_sync();                /* spinning takes time too */
    if (deterministic && host_ns_per_instr <= 0)
        virtual_ns += 2000;
    if (replay_vi_held)
        spins_held++;
    if (npending || now_ns() >= next_event_ns || replay_vi_held)
        host_yield();
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
extern void port_move_rdram(void);   /* runtime.c, PORT_MOVABLE */

static void usage(const char *argv0) {
    fprintf(stderr,
            "usage: %s [options] [ROM]\n"
            "  ROM                  defaults to " ROM_DEFAULT "\n"
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
            "  --interpolate        gl: 60 frames a second where the game draws 30, by\n"
            "                       showing a frame between each two (the game is unchanged)\n"
            "  --aspect W:H         widescreen: show the 3D world W:H wide (e.g. 16:9; 4:3,\n"
            "                       the default, is the N64's), or 'window' to follow it\n"
            "  --widescreen         --aspect 16:9\n"
            "  --wav PATH           write the sound to a WAV file too\n"
            "  --no-audio           no sound (--headless and --deterministic imply it)\n"
            "environment: PORT_AUTOSTART=1 taps Start/A; PORT_DUMP=N,... writes RDRAM\n"
            "at the Nth controller read (and on a crash); PORT_PACE=FILE logs the pacing\n"
            "per controller read; PORT_COUNT_PER_OP=N: CPU count ticks charged per\n"
            "instruction (default 2, as mupen64plus; 0: the CPU takes no time)\n", argv0);
    exit(2);
}

int main(int argc, char **argv) {
    const char *rom_path = ROM_DEFAULT;
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
        else if (!strcmp(argv[i], "--replay") && i + 1 < argc) {
            host_replay_load(argv[++i]);
            deterministic = 1;
        }
        else if (!strcmp(argv[i], "--headless"))
            host_headless = 1;
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
        else if (!strcmp(argv[i], "--interpolate"))
            gfx_interp = 1;
        else if (!strcmp(argv[i], "--widescreen"))
            gfx_aspect = 16.0f / 9;
        else if (!strcmp(argv[i], "--aspect") && i + 1 < argc) {
            const char *a = argv[++i];
            double w, h;
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
    const char *rs = getenv("PORT_RDP_SCALE");
    if (rs)
        rdp_scale = atof(rs);
    const char *cs = getenv("PORT_C_SCALE");
    if (cs)
        host_c_scale = atof(cs);
    const char *cpo = getenv("PORT_COUNT_PER_OP");
    if (cpo)
        host_ns_per_instr = atof(cpo) * 64.0 / 3;     /* not real time, or no one listening */
#ifdef __linux__
    prctl(PR_SET_TIMERSLACK, 1);    /* wake on time: the pacing is in 50 us steps */
#endif
    clock_gettime(CLOCK_MONOTONIC, &t0);
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
    map_fixed(PORT_HWREG_BASE, PORT_HWREG_SIZE, "hardware registers");
    map_fixed(PORT_STACK_BASE, PORT_STACK_SIZE * PORT_MAX_THREADS, "thread stacks");
    fiber_init();
    load_rom(rom_path);
    port_fixups();
#ifdef PORT_MOVABLE
    port_move_rdram();
#endif
    host_video_init();

    port_boot();

    /* the loop: deliver what's due, run threads until all wait, sleep */
    const uint64_t vi_period = 1000000000ull / 60;
    uint64_t next_vi = now_ns() + vi_period;
    int vi_force = 0;
    for (;;) {
        deliver_pending();
        if (host_replay_poll_si())
            continue;
        uint64_t now = now_ns();
        /* --replay: a retrace waits while the count is where the log's next
           read has it (replay.c) */
        int vi_held = host_replay_active() && !host_replay_vi_ok() && !vi_force;
        if (now >= next_vi && !vi_held) {
            vi_force = 0;
            spins_held = 0;
            next_vi += vi_period;
            if (now > next_vi + 4 * vi_period)      /* fell behind: don't catch up */
                next_vi = now + vi_period;
            host_video_frame();
            if (host_quit_requested())
                break;
            port_irq_vi();
            continue;
        }
        uint64_t deadline = port_irq_timers(host_ticks());
        /* a held retrace is given anyway when the game can't go on without
           one: nothing to run and nothing due (below), or a thread spinning
           on the count (__port_poll yields to the loop while one is held) */
        replay_vi_held = vi_held;
        if (!vi_held)
            spins_held = 0;
        if (vi_held && spins_held >= 4096) {
            host_replay_vi_forced();
            next_vi = now;
            vi_force = 1;
            continue;
        }
        uint64_t wake = vi_held ? ~0ull : next_vi;
        /* (rounded up: at deadline * 64 / 3 the counter may not be there yet) */
        if (deadline != ~0ull && (deadline * 64 + 2) / 3 < wake)
            wake = (deadline * 64 + 2) / 3;
        if (host_busy_wake() < wake)
            wake = host_busy_wake();
        uint64_t due = raise_due(now);
        if (due < wake)
            wake = due;
        next_event_ns = wake;
        if (npending)
            continue;
        if (host_run_one())
            continue;
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
        if (wake > now + 50000) {
            struct timespec ts = { (time_t)((wake - now) / 1000000000ull), (long)((wake - now) % 1000000000ull) };
            nanosleep(&ts, NULL);
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
