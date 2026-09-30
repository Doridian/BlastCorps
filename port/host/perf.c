/*
 * PORT_PERF=N: where the host's time goes, per retrace (docs/PORT.md,
 * "Performance").  The loop's time between two retraces is split into the
 * game's threads (the fibers, less what they call below), the renderer's
 * two passes (the real one and --interpolate's in-between one), the audio
 * microcode, presenting, and the time given away (the loop's sleeps: the
 * page's, in a browser).  Every N retraces (1: 600) a line goes to stderr
 * with the distribution of the work per retrace (the retrace's time less
 * what was given away), the means of the parts, how late the retraces
 * came, and how many images a second were new.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "host.h"

int host_perf_on;

static int window_n = 600;
static double t_last_vi, t_mark;
static int stack[16], depth;
static double acc[PERF_NCAT];          /* this retrace's, ms */
static double win_acc[PERF_NCAT];
static float *work, *late;
static int nsamp;
static unsigned total_vi;
static int win_over;                     /* retraces whose work took over 16.7 ms */
static int win_late;                     /* ... delivered 4 ms or more after they were due */
static double sleep_want, sleep_over, sleep_over_max;
static int nsleeps;
static unsigned long long img0, nframes, last_frame;
static double t_win;
static const char *names[PERF_NCAT] = { "loop", "game", "gfx", "gfx2", "audio", "present", "idle" };

double host_perf_now(void) {
#ifdef __EMSCRIPTEN__
    return emscripten_get_now();
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
#endif
}

void host_perf_init(void) {
    const char *e = getenv("PORT_PERF");
    if (!e || !*e || *e == '0')
        return;
    host_perf_on = 1;
    int n = atoi(e);
    if (n > 1)
        window_n = n;
    work = calloc(window_n, sizeof *work);
    late = calloc(window_n, sizeof *late);
    t_last_vi = t_mark = t_win = host_perf_now();
}

static void charge(double t) {
    acc[depth ? stack[depth - 1] : PERF_LOOP] += t - t_mark;
    t_mark = t;
}

void host_perf_push(int cat) {
    if (!host_perf_on)
        return;
    charge(host_perf_now());
    if (depth < 16)
        stack[depth] = cat;
    depth++;
}

void host_perf_pop(void) {
    if (!host_perf_on)
        return;
    charge(host_perf_now());
    if (depth)
        depth--;
}

void host_perf_sleep(double want_ms, double got_ms) {
    if (!host_perf_on)
        return;
    nsleeps++;
    sleep_want += want_ms;
    double o = got_ms - want_ms;
    sleep_over += o;
    if (o > sleep_over_max)
        sleep_over_max = o;
}

static int cmpf(const void *a, const void *b) {
    float x = *(const float *)a, y = *(const float *)b;
    return x < y ? -1 : x > y;
}

static void report(unsigned long long images) {
    double now = host_perf_now(), secs = (now - t_win) / 1e3;
    qsort(work, nsamp, sizeof *work, cmpf);
    qsort(late, nsamp, sizeof *late, cmpf);
#define PCT(a, p) (a)[(int)((nsamp - 1) * (p) + 0.5)]
    int aq;
    unsigned adrop;
    host_audio_perf(&aq, &adrop);
    char parts[256];
    int n = 0;
    for (int i = 0; i < PERF_NCAT; i++)
        n += snprintf(parts + n, sizeof parts - n, " %s %.2f", names[i], win_acc[i] / nsamp);
    host_log("perf: to retrace %u, %d in %.2f s: work/retrace ms median %.2f p95 %.2f p99 %.2f max %.2f; over 16.7 ms %d;"
             " late p50 %.2f p99 %.2f (>=4 ms %d); new images %.1f/s, game frames %.1f/s; mean ms:%s;"
             " sleeps %d, overshoot mean %.2f max %.2f; audio queued %d ms, %u dropped\n",
             total_vi, nsamp, secs, PCT(work, 0.5), PCT(work, 0.95), PCT(work, 0.99), work[nsamp - 1], win_over,
             PCT(late, 0.5), PCT(late, 0.99), win_late, (images - img0) / secs, nframes / secs, parts,
             nsleeps, nsleeps ? sleep_over / nsleeps : 0.0, sleep_over_max, aq, adrop);
#undef PCT
#ifdef PORT_WASM_WEB
    /* for a page's own display (and the tests that drive it) */
    EM_ASM({ Module.perf = Array.of($0, $1, $2, $3, $4); },     /* work p50, p95, p99; images, frames a second */
           work[nsamp / 2], work[(int)((nsamp - 1) * 0.95)], work[(int)((nsamp - 1) * 0.99)],
           (images - img0) / secs, nframes / secs);
#endif
    memset(win_acc, 0, sizeof win_acc);
    nsamp = win_over = win_late = nsleeps = 0;
    sleep_want = sleep_over = sleep_over_max = 0;
    img0 = images;
    nframes = 0;
    t_win = now;
}

/* a retrace: late_ms after it was due; images: the new images shown so
   far, frames: the game's frame count */
void host_perf_vi(double late_ms, unsigned long long images, unsigned long long frames) {
    if (!host_perf_on)
        return;
    double now = host_perf_now();
    charge(now);
    double wall = now - t_last_vi, w = wall - acc[PERF_IDLE];
    t_last_vi = now;
    if (total_vi == 0)
        img0 = images;
    /* (the game's count starts over now and then) */
    if (frames > last_frame && frames - last_frame < 16)
        nframes += frames - last_frame;
    last_frame = frames;
#ifdef PORT_WASM_WEB
    EM_ASM({ Module.images = $0; }, (double)images);     /* for a page that watches what it shows */
#endif
    work[nsamp] = (float)w;
    late[nsamp] = (float)late_ms;
    if (w > 1000.0 / 60)
        win_over++;
    if (late[nsamp] >= 4)
        win_late++;
    nsamp++;
    total_vi++;
    for (int i = 0; i < PERF_NCAT; i++) {
        win_acc[i] += acc[i];
        acc[i] = 0;
    }
    if (nsamp == window_n)
        report(images);
}
