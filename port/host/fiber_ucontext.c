/*
 * fiber.h on ucontext: every fiber on the one OS thread (the default on
 * Linux, where it is the cheapest switch).  macOS has deprecated ucontext
 * and WebAssembly has none: those take fiber_pthread.c.
 */
#ifdef PORT_THREADS_UCONTEXT
#include <stdlib.h>
#include <ucontext.h>

#include "fiber.h"
#include "host.h"

struct HostFiber {
    ucontext_t uc;
    void (*fn)(void *);
    void *arg;
};

static ucontext_t loop_uc;
static HostFiber *running;

void fiber_init(void) { }

static void trampoline(void) {
    HostFiber *f = running;
    f->fn(f->arg);
    host_fatal("fiber returned");
}

HostFiber *fiber_create(void (*fn)(void *), void *arg, void *stack, size_t size) {
    HostFiber *f = calloc(1, sizeof *f);
    if (!f || !stack)
        host_fatal("fiber_create: no memory or no stack");
    f->fn = fn;
    f->arg = arg;
    getcontext(&f->uc);
    f->uc.uc_stack.ss_sp = stack;
    f->uc.uc_stack.ss_size = size;
    f->uc.uc_link = NULL;
    makecontext(&f->uc, trampoline, 0);
    return f;
}

void fiber_run(HostFiber *f) {
    running = f;
    swapcontext(&loop_uc, &f->uc);
    running = NULL;
}

void fiber_yield(HostFiber *self) { swapcontext(&self->uc, &loop_uc); }

void fiber_exit(HostFiber *self) {
    swapcontext(&self->uc, &loop_uc);
    host_fatal("exited fiber resumed");
}

void fiber_free(HostFiber *f) { free(f); }

void fiber_call_on_loop(void (*fn)(void *), void *arg) { fn(arg); }
#endif
