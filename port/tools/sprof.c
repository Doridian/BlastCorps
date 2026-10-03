/*
 * A sampling profiler for the port where perf and valgrind aren't there:
 *
 *   cc -m32 -O2 -shared -fPIC -o sprof.so port/tools/sprof.c   (-m32 for the 32-bit build)
 *   LD_PRELOAD=./sprof.so SPROF_OUT=prof build/x/blastcorps ROM ...
 *   port/tools/sprof_report.py build/x/blastcorps prof.<pid>
 *
 * Every 1 ms (SPROF_US microseconds) of the process's CPU time (ITIMER_PROF) it notes the
 * interrupted program counter and thread (and, outside the executable,
 * a guess at its caller there); at exit it writes the maps and
 * the samples to SPROF_OUT.<pid>.  Self time only, no stacks.
 */
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>

#define MAXS (1u << 24)
static unsigned long *pcs, *callers;
static unsigned long text_lo, text_hi;
static unsigned *tids;
static volatile unsigned n;

static void handler(int sig, siginfo_t *si, void *ctx) {
    ucontext_t *uc = ctx;
    unsigned k = n;

    (void)sig;
    (void)si;
    if (k < MAXS) {
#if defined(__i386__)
        pcs[k] = uc->uc_mcontext.gregs[REG_EIP];
#else
        pcs[k] = uc->uc_mcontext.gregs[REG_RIP];
#endif
        tids[k] = (unsigned)syscall(SYS_gettid);
        /* outside the executable (a library's memcpy, libm): the first word
           near the stack pointer that is an address in the executable's
           text, as a guess at the caller */
        callers[k] = 0;
        if (pcs[k] < text_lo || pcs[k] >= text_hi) {
#if defined(__i386__)
            unsigned long *sp = (unsigned long *)uc->uc_mcontext.gregs[REG_ESP];
#else
            unsigned long *sp = (unsigned long *)uc->uc_mcontext.gregs[REG_RSP];
#endif
            int j;
            for (j = 0; j < 24; j++)
                if (sp[j] >= text_lo && sp[j] < text_hi) {
                    callers[k] = sp[j];
                    break;
                }
        }
        n = k + 1;
    }
}

static void dump(void) {
    const char *o = getenv("SPROF_OUT");
    char path[600], line[1024];
    struct itimerval it;
    FILE *f, *m;
    unsigned k;

    memset(&it, 0, sizeof it);
    setitimer(ITIMER_PROF, &it, NULL);
    snprintf(path, sizeof path, "%s.%d", o, (int)getpid());
    if (!(f = fopen(path, "w")))
        return;
    if ((m = fopen("/proc/self/maps", "r"))) {
        while (fgets(line, sizeof line, m))
            fprintf(f, "M %s", line);
        fclose(m);
    }
    for (k = 0; k < n; k++)
        fprintf(f, "%lx %u %lx\n", pcs[k], tids[k], callers[k]);
    fclose(f);
}

__attribute__((constructor)) static void init(void) {
    struct sigaction sa;
    struct itimerval it = {{0, 1000}, {0, 1000}};
    const char *us = getenv("SPROF_US");

    if (!getenv("SPROF_OUT"))
        return;
    if (us)
        it.it_interval.tv_usec = it.it_value.tv_usec = atoi(us);
    pcs = malloc(MAXS * sizeof *pcs);
    callers = malloc(MAXS * sizeof *callers);
    {
        /* the executable's text: its first executable mapping */
        char line[1024], exe[512];
        ssize_t l = readlink("/proc/self/exe", exe, sizeof exe - 1);
        FILE *m = fopen("/proc/self/maps", "r");

        exe[l > 0 ? l : 0] = 0;
        while (m && fgets(line, sizeof line, m)) {
            unsigned long lo, hi;
            char perm[8];

            if (sscanf(line, "%lx-%lx %7s", &lo, &hi, perm) == 3 && perm[2] == 'x' && strstr(line, exe)) {
                text_lo = lo, text_hi = hi;
                break;
            }
        }
        if (m)
            fclose(m);
    }
    tids = malloc(MAXS * sizeof *tids);
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &sa, NULL);
    setitimer(ITIMER_PROF, &it, NULL);
    atexit(dump);
}
