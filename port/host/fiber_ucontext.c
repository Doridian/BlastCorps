/*
 * fiber.h on ucontext: every fiber on the one OS thread (the default on
 * Linux, where it is the cheapest switch).  macOS has deprecated ucontext
 * and WebAssembly has none: those take fiber_pthread.c.
 */
#ifdef PORT_HAVE_UCONTEXT
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

static void f_init(void) { }

static void trampoline(void) {
    HostFiber *f = running;
    fiber_enter(f->fn, f->arg, f->uc.uc_stack.ss_sp, f->uc.uc_stack.ss_size);
    host_fatal("fiber returned");
}

static HostFiber *f_create(void (*fn)(void *), void *arg, void *stack, size_t size) {
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

static void f_run(HostFiber *f) {
    running = f;
    swapcontext(&loop_uc, &f->uc);
    running = NULL;
}

static void f_yield(HostFiber *self) { swapcontext(&self->uc, &loop_uc); }

static __attribute__((noreturn)) void f_exit(HostFiber *self) {
    swapcontext(&self->uc, &loop_uc);
    host_fatal("exited fiber resumed");
}

static void f_free(HostFiber *f) { free(f); }

static void f_call_on_loop(void (*fn)(void *), void *arg) { fn(arg); }

const FiberOps fiber_ucontext_ops = {
    "ucontext", f_init, f_create, f_run, f_yield, f_exit, f_free, f_call_on_loop,
};
#endif
