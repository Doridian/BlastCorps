/*
 * fiber.h on emscripten's fibers (Asyncify): every fiber on the page's one
 * thread, as with ucontext.  A switch unwinds the wasm stack into the
 * fiber's Asyncify buffer and rewinds the other's from its own, so the C
 * stacks (the arena's, as everywhere) stay where they are; the build's
 * -sASYNCIFY instruments whatever can be on the stack at a switch.  The
 * browser build's backend: no SharedArrayBuffer, no workers, and the loop
 * can hand the page its thread back between two fiber_run()s
 * (emscripten_sleep, main.c).
 */
#ifdef PORT_HAVE_ASYNCIFY
#include <emscripten/fiber.h>
#include <stdlib.h>

#include "fiber.h"
#include "host.h"

/* a suspended fiber's wasm frames (the locals Asyncify saves; the C
   stack holds the rest) */
#define ASYNCIFY_BUF (512 * 1024)

struct HostFiber {
    emscripten_fiber_t ctx;
    void (*fn)(void *);
    void *arg;
    void *stack;
    size_t size;
    void *abuf;
};

static emscripten_fiber_t loop_ctx;
static void *loop_abuf;

static void f_init(void) {
    loop_abuf = malloc(ASYNCIFY_BUF);
    if (!loop_abuf)
        host_fatal("fiber_init: no memory");
    emscripten_fiber_init_from_current_context(&loop_ctx, loop_abuf, ASYNCIFY_BUF);
}

static void entry(void *p) {
    HostFiber *f = p;
    fiber_enter(f->fn, f->arg, f->stack, f->size);
    host_fatal("fiber returned");
}

static HostFiber *f_create(void (*fn)(void *), void *arg, void *stack, size_t size) {
    if (!stack)
        host_fatal("fiber_create: the asyncify backend needs a stack");
    HostFiber *f = calloc(1, sizeof *f);
    if (!f || !(f->abuf = malloc(ASYNCIFY_BUF)))
        host_fatal("fiber_create: no memory");
    f->fn = fn;
    f->arg = arg;
    f->stack = stack;
    f->size = size;
    emscripten_fiber_init(&f->ctx, entry, f, stack, size, f->abuf, ASYNCIFY_BUF);
    return f;
}

static void f_run(HostFiber *f) { emscripten_fiber_swap(&loop_ctx, &f->ctx); }

static void f_yield(HostFiber *self) { emscripten_fiber_swap(&self->ctx, &loop_ctx); }

static __attribute__((noreturn)) void f_exit(HostFiber *self) {
    emscripten_fiber_swap(&self->ctx, &loop_ctx);
    host_fatal("an exited fiber ran again");
}

/* a suspended fiber's frames are only its Asyncify buffer's data */
static void f_free(HostFiber *f) {
    free(f->abuf);
    free(f);
}

static void f_call_on_loop(void (*fn)(void *), void *arg) { fn(arg); }

const FiberOps fiber_asyncify_ops = {
    "asyncify", f_init, f_create, f_run, f_yield, f_exit, f_free, f_call_on_loop,
};
#endif
