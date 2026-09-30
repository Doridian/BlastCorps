/*
 * fiber.h's front: the backend, chosen once at startup (fiber.h).
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fiber.h"
#include "host.h"

static const FiberOps *ops;

void fiber_init(void) {
#if defined(PORT_THREADS_DEFAULT_PTHREAD) || !defined(PORT_HAVE_UCONTEXT)
    ops = &fiber_pthread_ops;
#else
    ops = &fiber_ucontext_ops;
#endif
    const char *e = getenv("PORT_THREADS");
    if (e && *e) {
#ifdef PORT_HAVE_UCONTEXT
        if (!strcmp(e, "ucontext"))
            ops = &fiber_ucontext_ops;
        else
#endif
        if (!strcmp(e, "pthread"))
            ops = &fiber_pthread_ops;
        else
            host_fatal("PORT_THREADS=%s: this build has %s", e,
#ifdef PORT_HAVE_UCONTEXT
                       "ucontext and pthread"
#else
                       "pthread only"
#endif
            );
    }
    if (host_verbose)
        host_log("threads: %s\n", ops->name);
    ops->init();
}

const char *fiber_backend(void) { return ops->name; }

/* Where a fiber's first frame is: FIBER_TOP_GAP below the top of its
   stack in either backend.  The game's C stores the addresses of its
   locals in RDRAM (a message queue on a thread's stack, the RSP's scratch
   area), so for a run to be byte for byte the same on both, the frames
   have to be at the same addresses; what the backends put at the top of
   the stack first (glibc's thread descriptor and TLS, the start
   routines) differs.  The alloca takes the stack down to the same place
   from wherever this was called: both calls come in 16-aligned, and the
   rounding is the same. */
#define FIBER_TOP_GAP 0x10000
void fiber_enter(void (*fn)(void *), void *arg, void *stack, size_t size) {
    volatile char *pad = NULL;
    if (stack) {
        char here;
        uintptr_t target = (uintptr_t)stack + size - FIBER_TOP_GAP;
        if ((uintptr_t)&here < target + 256)
            host_fatal("fiber_enter: %s's stack top is %#lx bytes in use (the gap is %#x)", ops->name,
                       (unsigned long)((uintptr_t)stack + size - (uintptr_t)&here), FIBER_TOP_GAP);
        pad = __builtin_alloca((uintptr_t)&here - target);
        pad[0] = 0;
    }
    fn(arg);
    if (pad)
        pad[0] = 1;             /* (no tail call: the alloca stays) */
}

HostFiber *fiber_create(void (*fn)(void *), void *arg, void *stack, size_t size) {
    return ops->create(fn, arg, stack, size);
}
void fiber_run(HostFiber *f) { ops->run(f); }
void fiber_yield(HostFiber *self) { ops->yield(self); }
void fiber_exit(HostFiber *self) { ops->exit(self); }
void fiber_free(HostFiber *f) { ops->free(f); }
void fiber_call_on_loop(void (*fn)(void *), void *arg) { ops->call_on_loop(fn, arg); }
