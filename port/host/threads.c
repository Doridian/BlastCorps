/*
 * The game's threads as fibers (fiber.h: ucontext, or host threads one at
 * a time), scheduled as libultra does: the highest-priority runnable
 * thread runs until it blocks, yields, or wakes a thread of higher
 * priority; equal priorities are FIFO.  The main loop (main.c) runs
 * whenever none is runnable, and stands in for the interrupts.
 *
 * Each thread has a host stack (runtime.c's host_thread_stack); the one
 * the game passed to osCreateThread isn't used.
 *
 * The game's work takes no time: the clock moves only in the loop, from
 * one retrace, timer or event to the next (docs/PORT.md, "Timing").
 */
#include <stdlib.h>
#include <string.h>

#include "fiber.h"
#include "host.h"

enum { T_FREE, T_STOPPED, T_RUNNABLE, T_WAITING, T_PARKED, T_DEAD, T_SPIN };
static const char *state_names[] = { "free", "stopped", "runnable", "waiting", "parked", "dead", "spinning" };

typedef struct {
    int state;
    uintptr_t key;              /* the OSThread's address (port.h) */
    int pri;
    uint64_t seq;               /* FIFO order among equal priorities */
    const void *wait_key;
    const void *recv_key;       /* the queue of the osRecvMesg it is in (host_recv_mark), or NULL */
    void (*entry)(void *);
    void *arg;
    HostFiber *fiber;
    int started;
    uint32_t spin_irqs;         /* T_SPIN: host_irqs when it began to wait */
} HThread;

static HThread threads[PORT_MAX_THREADS];
static HThread *cur;
static uint64_t seq_counter;

static HThread *find(uintptr_t key) {
    if (key == 0)
        return cur;
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state != T_FREE && threads[i].key == key)
            return &threads[i];
    return NULL;
}

static void fiber_main(void *p) {
    HThread *t = p;
    t->entry(t->arg);
    /* returning from a thread's entry: it just stops */
    t->state = T_DEAD;
    fiber_exit(t->fiber);
}


/* whether another thread than the running one is in osRecvMesg on this
   key: blocked, woken and not yet run, or preempted at the call's entry
   (host_recv_mark), and so will take the next message */
int host_receiving(const void *key) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (&threads[i] != cur && threads[i].state != T_FREE && threads[i].state != T_DEAD &&
            threads[i].recv_key == key)
            return 1;
    return 0;
}

/* whether a thread is blocked waiting on `wait_key` (host_block's key) */
int host_blocked_on(const void *wait_key) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_WAITING && threads[i].wait_key == wait_key)
            return 1;
    return 0;
}

void host_thread_create(uintptr_t key, void (*entry)(void *), void *arg, int pri) {
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
    uint32_t stack_size;
    void *stack = host_thread_stack(idx, &stack_size);
    t->fiber = fiber_create(fiber_main, t, stack, stack_size);
    if (host_verbose)
        host_log("thread %08lX created, pri %d, entry %p\n", (unsigned long)key, pri, (void *)entry);
}

static void to_loop(void) { fiber_yield(cur->fiber); }

void host_preempt(void) {
    if (!cur)
        return;
    cur->recv_key = NULL;       /* osRecvMesg's end: it has its message */
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_RUNNABLE && threads[i].pri > cur->pri) {
            cur->seq = seq_counter++;
            to_loop();
            return;
        }
}

void host_thread_start(uintptr_t key) {
    HThread *t = find(key);
    if (!t)
        host_fatal("osStartThread on unknown thread %08lX", (unsigned long)key);
    if (t->state == T_STOPPED) {
        t->state = T_RUNNABLE;
        t->seq = seq_counter++;
    }
    host_preempt();
}

void host_thread_stop(uintptr_t key) {
    HThread *t = find(key);
    if (!t)
        return;
    if (t == cur) {
        t->state = T_STOPPED;
        to_loop();
    } else if (t->state == T_RUNNABLE || t->state == T_WAITING || t->state == T_SPIN) {
        t->state = T_STOPPED;
    }
}

void host_thread_destroy(uintptr_t key) {
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

void host_thread_set_pri(uintptr_t key, int pri) {
    HThread *t = find(key);
    if (!t)
        return;
    t->pri = pri;
    if (t == cur && pri == 0) {
        /* the idle thread drops to priority 0 and spins forever: park it
           instead, as nothing ever runs at priority 0 but it */
        if (host_verbose)
            host_log("thread %08lX parked (idle)\n", (unsigned long)t->key);
        t->state = T_PARKED;
        to_loop();
        host_fatal("parked thread resumed");
    }
    host_preempt();
}

int host_thread_get_pri(uintptr_t key) {
    HThread *t = find(key);
    return t ? t->pri : 0;
}

void *host_thread_current(void) {
    return cur ? (void *)cur->key : NULL;
}

void host_block(const void *key) {
    if (!cur)
        host_fatal("blocking receive outside a thread (key %p)", key);
    cur->state = T_WAITING;
    cur->wait_key = key;
    to_loop();
}

void host_wake(const void *key) {
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

/* A busy-wait (port_game.h's port_spin_wait): on the N64 the thread spins
   until an interrupt changes what it reads, and nothing of lower priority
   runs meanwhile.  Here it waits for the next interrupt the loop delivers
   (host_interrupt counts them), holding the CPU against lower priorities
   as the spin did; equal and higher ones run. */
static uint32_t host_irqs;

void port_spin_wait(void) {
    if (!cur)
        return;
    cur->state = T_SPIN;
    cur->spin_irqs = host_irqs;
    cur->seq = seq_counter++;
    to_loop();
}

/* whether a thread spins, with no interrupt since it began */
int host_spinning(void) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (threads[i].state == T_SPIN && threads[i].spin_irqs == host_irqs)
            return 1;
    return 0;
}

/* osRecvMesg marks the thread as receiving on `key` until the call
   returns (host_preempt), for host_receiving.  A non-blocking receive that
   finds nothing leaves the mark behind; the game's only one on the SI
   queue is the pak/EEPROM thread's own. */
void host_recv_mark(const void *key) {
    if (cur)
        cur->recv_key = key;
}

/* an interrupt taken (port/src/ultra.c): a spinning thread tests again */
void host_interrupt(void) { host_irqs++; }

int host_run_one(void) {
    HThread *best = NULL;
    int spin_pri = -1;          /* the highest priority spinning with no interrupt yet */
    for (int i = 0; i < PORT_MAX_THREADS; i++) {
        HThread *t = &threads[i];
        if (t->state == T_SPIN) {
            if (t->spin_irqs == host_irqs) {
                if (t->pri > spin_pri)
                    spin_pri = t->pri;
                continue;
            }
            t->state = T_RUNNABLE;          /* an interrupt came: it tests again */
        }
        if (t->state != T_RUNNABLE)
            continue;
        if (!best || t->pri > best->pri || (t->pri == best->pri && t->seq < best->seq))
            best = t;
    }
    if (!best || best->pri < spin_pri)
        return 0;
    if (host_verbose > 3)
        host_log("run %08lX pri %d at %.3f\n", (unsigned long)best->key, best->pri, host_now_ns() / 1e6);
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
        host_log("  thread %08lX pri %3d %-8s wait %p%s\n", (unsigned long)t->key, t->pri, state_names[t->state],
                 t->wait_key, t == cur ? " (current)" : "");
    }
}
