/*
 * The context switch under threads.c: how a game thread's native C gets a
 * stack of its own and hands the CPU back.  threads.c does all the
 * scheduling; a backend only switches, and only between the loop and one
 * fiber (a star: the loop runs a fiber, the fiber yields back to the
 * loop, never fiber to fiber).  So the loop is never inside a fiber, and
 * can return to its caller (a browser's frame callback) between any two
 * fiber_run()s.
 *
 * Backends (CMake's PORT_THREADS):
 *   ucontext  makecontext/swapcontext on the one OS thread (Linux's default)
 *   pthread   an OS thread per fiber, exactly one of them (or the loop)
 *             running at a time, handed the CPU with a mutex and condition
 *             variables (macOS, and emscripten with pthreads)
 * Both run the same code in the same order, so a deterministic run is the
 * same on either (docs/PORT.md, "The platform layer").
 */
#ifndef FIBER_H
#define FIBER_H

#include <stddef.h>

typedef struct HostFiber HostFiber;

/* on the loop's thread, before any other call */
void fiber_init(void);
/* a fiber that will run fn(arg) on `stack` (size bytes, owned by the
   caller) from its first fiber_run; stack may be NULL for the backend's
   own (pthread only: the ucontext backend needs one) */
HostFiber *fiber_create(void (*fn)(void *), void *arg, void *stack, size_t size);
/* the loop: run f until it yields or exits */
void fiber_run(HostFiber *f);
/* the running fiber: back to the loop, until the next fiber_run(self) */
void fiber_yield(HostFiber *self);
/* the running fiber: back to the loop for good; fn's frames are dropped */
void fiber_exit(HostFiber *self) __attribute__((noreturn));
/* free a fiber that isn't running (suspended, exited or never run); its
   frames are dropped, and then its stack is the caller's again */
void fiber_free(HostFiber *f);
/* run fn(arg) on the loop's OS thread and return: for what is tied to it
   (the GL context, SDL's window).  A plain call in the ucontext backend. */
void fiber_call_on_loop(void (*fn)(void *), void *arg);

#endif
