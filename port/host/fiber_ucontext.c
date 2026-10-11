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

/* ASan has to be told of a switch to another stack (its fake stacks, and the
   bounds it checks the stack pointer against): without these it warns that
   it "doesn't fully support makecontext/swapcontext" and may report what
   isn't there.  loop_bottom/loop_size are the loop's stack, which a fiber
   switches back to; each fiber's own are its create's.  (The functions are
   declared here: gcc's and clang's sanitizer headers are in different
   places.) */
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
#define FIBER_ASAN 1
void __sanitizer_start_switch_fiber(void **fake_stack_save, const void *bottom, size_t size);
void __sanitizer_finish_switch_fiber(void *fake_stack_save, const void **bottom_old, size_t *size_old);
#endif

struct HostFiber {
    ucontext_t uc;
    void (*fn)(void *);
    void *arg;
#ifdef FIBER_ASAN
    void *fake;                 /* the fiber's fake stack while it isn't running */
#endif
};

static ucontext_t loop_uc;
static HostFiber *running;
#ifdef FIBER_ASAN
static void *loop_fake;
static const void *loop_bottom;
static size_t loop_size;
#endif

/* the switches: from the loop into a fiber, from a fiber back to the loop,
   and from a fiber for good (its fake stack is freed) */
static void switch_in(HostFiber *f) {
#ifdef FIBER_ASAN
    __sanitizer_start_switch_fiber(&loop_fake, f->uc.uc_stack.ss_sp, f->uc.uc_stack.ss_size);
#endif
    swapcontext(&loop_uc, &f->uc);
#ifdef FIBER_ASAN
    __sanitizer_finish_switch_fiber(loop_fake, NULL, NULL);
#endif
}

static void switch_out(HostFiber *self, int last) {
#ifdef FIBER_ASAN
    __sanitizer_start_switch_fiber(last ? NULL : &self->fake, loop_bottom, loop_size);
#else
    (void)last;
#endif
    swapcontext(&self->uc, &loop_uc);
#ifdef FIBER_ASAN
    __sanitizer_finish_switch_fiber(self->fake, &loop_bottom, &loop_size);
#endif
}

static void f_init(void) { }

static void trampoline(void) {
    HostFiber *f = running;
#ifdef FIBER_ASAN
    __sanitizer_finish_switch_fiber(NULL, &loop_bottom, &loop_size);
#endif
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
    switch_in(f);
    running = NULL;
}

static void f_yield(HostFiber *self) { switch_out(self, 0); }

static __attribute__((noreturn)) void f_exit(HostFiber *self) {
    switch_out(self, 1);
    host_fatal("exited fiber resumed");
}

static void f_free(HostFiber *f) { free(f); }

static void f_call_on_loop(void (*fn)(void *), void *arg) { fn(arg); }

const FiberOps fiber_ucontext_ops = {
    "ucontext", f_init, f_create, f_run, f_yield, f_exit, f_free, f_call_on_loop,
};
#endif
