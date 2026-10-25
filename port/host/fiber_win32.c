/*
 * fiber.h on Windows fibers: every fiber on the one OS thread, as with
 * ucontext (Windows has neither ucontext nor a pthread_attr_setstack that
 * takes the caller's memory).  A Windows fiber has a stack of its own,
 * which the system allocates: the caller's (runtime.c's) goes unused, and
 * fiber_enter gets none, so the frames aren't at the addresses the other
 * backends put them.  The deterministic runs still give test.py quick's
 * references (checked under wine).
 */
#ifdef PORT_HAVE_WIN32_FIBERS
#include <stdlib.h>
#include <windows.h>

#include "fiber.h"
#include "host.h"

struct HostFiber {
    void *fiber;
    void (*fn)(void *);
    void *arg;
};

static void *loop_fiber;

static void f_init(void) {
    loop_fiber = ConvertThreadToFiber(NULL);
    if (!loop_fiber)
        host_fatal("ConvertThreadToFiber: error %lu", (unsigned long)GetLastError());
}

static void WINAPI entry(void *p) {
    HostFiber *f = p;
    fiber_enter(f->fn, f->arg, NULL, 0);
    host_fatal("fiber returned");
}

static HostFiber *f_create(void (*fn)(void *), void *arg, void *stack, size_t size) {
    (void)stack;
    HostFiber *f = calloc(1, sizeof *f);
    if (!f)
        host_fatal("fiber_create: no memory");
    f->fn = fn;
    f->arg = arg;
    f->fiber = CreateFiberEx(0, size, 0, entry, f);
    if (!f->fiber)
        host_fatal("CreateFiberEx: error %lu", (unsigned long)GetLastError());
    return f;
}

static void f_run(HostFiber *f) { SwitchToFiber(f->fiber); }

static void f_yield(HostFiber *self) {
    (void)self;
    SwitchToFiber(loop_fiber);
}

static __attribute__((noreturn)) void f_exit(HostFiber *self) {
    (void)self;
    SwitchToFiber(loop_fiber);
    host_fatal("exited fiber resumed");
}

static void f_free(HostFiber *f) {
    DeleteFiber(f->fiber);
    free(f);
}

static void f_call_on_loop(void (*fn)(void *), void *arg) { fn(arg); }

const FiberOps fiber_win32_ops = {
    "win32", f_init, f_create, f_run, f_yield, f_exit, f_free, f_call_on_loop,
};
#endif
