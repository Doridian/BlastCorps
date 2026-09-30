/*
 * The game's threads as fibers (fiber.h: ucontext, or host threads one at
 * a time), scheduled as libultra does: the highest-priority runnable
 * thread runs until it blocks, yields, or wakes a thread of higher
 * priority; equal priorities are FIFO.  The main loop (main.c) runs
 * whenever none is runnable, and stands in for the interrupts.
 *
 * Each thread has two stacks: a host stack for the native C (a fiber stack
 * inside the KSEG0 window, see port.h), and its N64 stack, the one the game
 * passed to osCreateThread, which only the translated code uses (through
 * ctx->sp).  Each also has its own recomp_context.
 *
 * The CPU's time: the game's C and the translated engine count the
 * instructions they execute (__port_icount, BEPass's ICount and the
 * translator's BB()), and at each call into libultra (host_cpu_sync) the
 * running thread is charged for what it did since.  A thread with enough
 * owed goes "busy" until the clock catches up: it holds the CPU (nothing
 * of lower priority runs meanwhile) but a higher-priority one that wakes
 * up can preempt it, which pushes the busy thread's end back by as much.
 */
#include <stdlib.h>
#include <string.h>

#include "fiber.h"
#include "host.h"

enum { T_FREE, T_STOPPED, T_RUNNABLE, T_WAITING, T_PARKED, T_DEAD, T_BUSY };
static const char *state_names[] = { "free", "stopped", "runnable", "waiting", "parked", "dead", "busy" };

typedef struct {
    int state;
    uint32_t key;               /* the OSThread's address */
    int pri;
    uint64_t seq;               /* FIFO order among equal priorities */
    uint32_t wait_key;
    uint32_t recv_key;          /* the queue of the osRecvMesg it is in (host_recv_charge), or 0 */
    void (*entry)(void *);
    void *arg;
    HostFiber *fiber;
    recomp_context ctx;
    int started;
    uint32_t imark, imark_c;    /* the counters when last charged */
    double owed;                /* ns not charged yet (under 1) */
    uint64_t clock;             /* how far its own work has got (ns) */
    uint64_t busy_until;        /* T_BUSY: host_now_ns() it may resume at */
} HThread;

static HThread threads[PORT_MAX_THREADS];
static HThread *cur;
static uint64_t seq_counter;

recomp_context *port_ctx(void) {
    if (!cur)
        host_fatal("translated code called outside a thread");
    return &cur->ctx;
}

static HThread *find(uint32_t key) {
    if (key == 0)
        return cur;
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state != T_FREE && threads[i].key == key)
            return &threads[i];
    return NULL;
}

static void fiber_main(void *p) {
    HThread *t = p;
    PORT_FN(void (*)(void *), t->entry)(t->arg);
    /* returning from a thread's entry: it just stops */
    t->state = T_DEAD;
    fiber_exit(t->fiber);
}


/* whether another thread than the running one is in osRecvMesg on this
   key: blocked, woken and not yet run, or preempted at the call's entry
   (host_recv_charge), and so will take the next message */
int host_receiving(uint32_t key) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (&threads[i] != cur && threads[i].state != T_FREE && threads[i].state != T_DEAD &&
            threads[i].recv_key == key)
            return 1;
    return 0;
}

/* whether a thread is blocked waiting on `wait_key` (host_block's key) */
int host_blocked_on(uint32_t wait_key) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_WAITING && threads[i].wait_key == wait_key)
            return 1;
    return 0;
}

void host_thread_create(uint32_t key, void (*entry)(void *), void *arg, uint32_t mips_sp, int pri) {
    HThread *t = find(key);
    if (!t || key == 0) {
        for (int i = 0; i < PORT_MAX_THREADS && !t; i++)
            if (threads[i].state == T_FREE || threads[i].state == T_DEAD)
                t = &threads[i];
    }
    if (!t)
        host_fatal("out of threads");
    if (t == cur)
        host_fatal("osCreateThread on the running thread");
    int idx = (int)(t - threads);
    if (t->fiber)
        fiber_free(t->fiber);           /* a dead thread's, or one recreated */
    memset(t, 0, sizeof *t);
    t->state = T_STOPPED;
    t->key = key;
    t->pri = pri;
    t->entry = entry;
    t->arg = arg;
    t->ctx.sp = (uint64_t)(int64_t)(int32_t)mips_sp;
    uint32_t stack_size;
    void *stack = host_thread_stack(idx, &stack_size);
    t->fiber = fiber_create(fiber_main, t, stack, stack_size);
    if (host_verbose)
        host_log("thread %08X created, pri %d, entry %p\n", key, pri, (void *)entry);
}

static void to_loop(void) { fiber_yield(cur->fiber); }

void host_preempt(void) {
    if (!cur)
        return;
    cur->recv_key = 0;          /* osRecvMesg's end: it has its message */
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_RUNNABLE && threads[i].pri > cur->pri) {
            cur->seq = seq_counter++;
            to_loop();
            return;
        }
}

void host_thread_start(uint32_t key) {
    HThread *t = find(key);
    if (!t)
        host_fatal("osStartThread on unknown thread %08X", key);
    if (t->state == T_STOPPED) {
        t->state = T_RUNNABLE;
        t->seq = seq_counter++;
    }
    host_preempt();
}

void host_thread_stop(uint32_t key) {
    HThread *t = find(key);
    if (!t)
        return;
    if (t == cur) {
        t->state = T_STOPPED;
        to_loop();
    } else if (t->state == T_RUNNABLE || t->state == T_WAITING) {
        t->state = T_STOPPED;
    }
}

void host_thread_destroy(uint32_t key) {
    HThread *t = find(key);
    if (!t)
        return;
    if (t == cur) {
        t->state = T_DEAD;
        fiber_exit(t->fiber);
    }
    t->state = T_FREE;
    fiber_free(t->fiber);
    t->fiber = NULL;
}

void host_thread_set_pri(uint32_t key, int pri) {
    HThread *t = find(key);
    if (!t)
        return;
    t->pri = pri;
    if (t == cur && pri == 0) {
        /* the idle thread drops to priority 0 and spins forever: park it
           instead, as nothing ever runs at priority 0 but it */
        if (host_verbose)
            host_log("thread %08X parked (idle)\n", t->key);
        t->state = T_PARKED;
        to_loop();
        host_fatal("parked thread resumed");
    }
    host_preempt();
}

int host_thread_get_pri(uint32_t key) {
    HThread *t = find(key);
    return t ? t->pri : 0;
}

uint32_t host_thread_current(void) {
    return cur ? cur->key : 0;
}

void host_block(uint32_t key) {
    if (!cur)
        host_fatal("blocking receive outside a thread (key %08X)", key);
    cur->state = T_WAITING;
    cur->wait_key = key;
    to_loop();
}

void host_wake(uint32_t key) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_WAITING && threads[i].wait_key == key) {
            threads[i].state = T_RUNNABLE;
            threads[i].seq = seq_counter++;
        }
}

void host_yield(void) {
    if (!cur)
        return;
    cur->seq = seq_counter++;
    to_loop();
}

/* ---- the CPU's time ---------------------------------------------------------- */

uint32_t __port_icount;                     /* MIPS instructions: the translated code */
uint32_t __port_icount_c;                   /* LLVM IR instructions: the C (BEPass) */
/* IDO's -O1 code is bigger than clang's -O2 IR: MIPS instructions per
   counted IR instruction, set so the attract mode's pace is mupen64plus's */
double host_c_scale = 1.6;
double host_ns_per_instr = 2 * 64.0 / 3;    /* mupen64plus's CountPerOp = 2 */
#define MIN_BUSY_NS 50000                    /* run ahead of the clock by up to 50 us */

/* The thread's clock runs on from where its last busy stretch ended, not
   from when the host got round to resuming it, so late wakeups don't add
   up. */
void host_cpu_sync(void) {
    if (!cur || host_ns_per_instr <= 0)
        return;
    cur->owed += ((uint32_t)(__port_icount - cur->imark) +
                  (uint32_t)(__port_icount_c - cur->imark_c) * host_c_scale) * host_ns_per_instr;
    cur->imark = __port_icount;
    cur->imark_c = __port_icount_c;
    uint64_t ns = (uint64_t)cur->owed;
    cur->owed -= (double)ns;
    cur->clock += ns;
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_BUSY) {
            threads[i].busy_until += ns;    /* preempted by this one */
            threads[i].clock += ns;
        }
    if (cur->clock < host_now_ns() + MIN_BUSY_NS)
        return;
    cur->busy_until = cur->clock;
    if (host_verbose > 3)
        host_log("busy: %08X pri %d at %.3f for %.3f ms\n", cur->key, cur->pri, host_now_ns() / 1e6,
                 (cur->clock - host_now_ns()) / 1e6);
    cur->state = T_BUSY;
    to_loop();
}

void host_cpu_charge(uint32_t n) { __port_icount += n; }

/* osRecvMesg's charge, which also marks the thread as receiving on `key`
   until the call returns (host_preempt), for host_receiving.  (One call
   for host_cpu_charge's one: the C's ICount stays as it was.)  A
   non-blocking receive that finds nothing leaves the mark behind; the
   game's only one on the SI queue is the pak/EEPROM thread's own. */
void host_recv_charge(uint32_t key, uint32_t n) {
    __port_icount += n;
    if (cur)
        cur->recv_key = key;
}

/* an interrupt's handling (libultra's exception handler, the dispatch):
   it takes the CPU from whatever is computing */
void host_irq_cost(uint32_t n) {
    uint64_t ns = (uint64_t)(n * host_ns_per_instr);
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_BUSY) {
            threads[i].busy_until += ns;
            threads[i].clock += ns;
        }
}

uint64_t host_busy_wake(void) {
    uint64_t w = ~0ull;
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_BUSY && threads[i].busy_until < w)
            w = threads[i].busy_until;
    return w;
}

int host_run_one(void) {
    HThread *best = NULL;
    for (int i = 0; i < PORT_MAX_THREADS; i++) {
        HThread *t = &threads[i];
        if (t->state != T_RUNNABLE && t->state != T_BUSY)
            continue;
        if (!best || t->pri > best->pri || (t->pri == best->pri && t->seq < best->seq))
            best = t;
    }
    if (!best)
        return 0;
    if (best->state == T_BUSY) {
        /* still computing: nothing below it may run */
        if (host_now_ns() < best->busy_until)
            return 0;
        best->state = T_RUNNABLE;
    } else if (best->clock < host_now_ns()) {
        best->clock = host_now_ns();        /* it was waiting */
    }
    best->imark = __port_icount;
    best->imark_c = __port_icount_c;
    if (host_verbose > 3)
        host_log("run %08X pri %d at %.3f\n", best->key, best->pri, host_now_ns() / 1e6);
    cur = best;
    host_perf_push(PERF_GAME);
    fiber_run(best->fiber);
    host_perf_pop();
    cur = NULL;
    return 1;
}

void host_threads_dump(void) {
    for (int i = 0; i < PORT_MAX_THREADS; i++) {
        HThread *t = &threads[i];
        if (t->state == T_FREE)
            continue;
        host_log("  thread %08X pri %3d %-8s wait %08X%s\n", t->key, t->pri, state_names[t->state],
                 t->wait_key, t == cur ? " (current)" : "");
    }
}
