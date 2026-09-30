/*
 * fiber.h on host threads: each fiber is an OS thread, and exactly one of
 * them, or the loop's, runs at any time.  The CPU is a baton handed over
 * under one mutex: fiber_run gives it to the fiber and waits for it back,
 * fiber_yield gives it back and waits for the next fiber_run.  Every
 * handover is a lock/unlock pair, so whatever one side wrote the other sees
 * (and the game's state, which only the baton's holder touches, needs no
 * locks of its own).  The order things run in is threads.c's, the same as
 * with ucontext.
 *
 * A fiber whose frames are to be dropped (fiber_free on a suspended one,
 * fiber_exit) longjmps back to its thread's start routine and returns from
 * it, so no unwinding through the game's frames is needed.
 *
 * Works wherever POSIX threads do: Linux, macOS, emscripten with
 * -pthread.  (On a browser's main thread emscripten busy-waits in
 * pthread_cond_wait, which is allowed but spins a core: see docs/PORT.md,
 * "Threads without ucontext".)
 */
#ifdef PORT_HAVE_PTHREAD
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "fiber.h"
#include "host.h"

struct HostFiber {
    pthread_t th;
    pthread_cond_t cv;
    int started;        /* its OS thread exists */
    int go;             /* it holds the baton */
    int kill;           /* fiber_free: drop the frames and end */
    void (*fn)(void *);
    void *arg;
    void *stack;
    size_t size;
    jmp_buf out;        /* back to the start routine */
};

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t loop_cv = PTHREAD_COND_INITIALIZER;
static int loop_go;                 /* the loop holds the baton */
static void (*call_fn)(void *);     /* fiber_call_on_loop's request */
static void *call_arg;
static pthread_t loop_thread;
static _Thread_local HostFiber *self_fiber;

static void f_init(void) { loop_thread = pthread_self(); }

/* with mu held: wait until the baton is f's (1) or f is to end (0) */
static int wait_go(HostFiber *f) {
    while (!f->go && !f->kill)
        pthread_cond_wait(&f->cv, &mu);
    if (f->kill)
        return 0;
    f->go = 0;
    return 1;
}

/* with mu held: the baton to the loop */
static void give_loop(void) {
    loop_go = 1;
    pthread_cond_signal(&loop_cv);
}

static void *start(void *p) {
    HostFiber *f = p;
    self_fiber = f;
    /* the crash handler runs on an alternate stack (main.c), which is per
       thread */
    stack_t ss = { .ss_sp = malloc(65536), .ss_size = 65536 };
    if (ss.ss_sp)
        sigaltstack(&ss, NULL);
    pthread_mutex_lock(&mu);
    int run = wait_go(f);
    pthread_mutex_unlock(&mu);
    if (run && !setjmp(f->out)) {
        fiber_enter(f->fn, f->arg, f->stack, f->size);
        host_fatal("fiber returned");
    }
    if (ss.ss_sp) {
        ss.ss_flags = SS_DISABLE;
        sigaltstack(&ss, NULL);
        free(ss.ss_sp);
    }
    return NULL;
}

static HostFiber *f_create(void (*fn)(void *), void *arg, void *stack, size_t size) {
    HostFiber *f = calloc(1, sizeof *f);
    if (!f)
        host_fatal("fiber_create: no memory");
    f->fn = fn;
    f->arg = arg;
    f->stack = stack;
    f->size = size;
    pthread_cond_init(&f->cv, NULL);
    return f;
}

static void f_run(HostFiber *f) {
    pthread_mutex_lock(&mu);
    if (!f->started) {
        pthread_attr_t a;
        pthread_attr_init(&a);
        if (f->stack)
            pthread_attr_setstack(&a, f->stack, f->size);
        else
            pthread_attr_setstacksize(&a, f->size);
        int e = pthread_create(&f->th, &a, start, f);
        pthread_attr_destroy(&a);
        if (e)
            host_fatal("pthread_create: %s", strerror(e));
        f->started = 1;
    }
    f->go = 1;
    pthread_cond_signal(&f->cv);
    for (;;) {
        while (!loop_go)
            pthread_cond_wait(&loop_cv, &mu);
        loop_go = 0;
        if (!call_fn)
            break;
        /* the fiber wants something done on this thread, and then goes on */
        void (*fn)(void *) = call_fn;
        call_fn = NULL;
        pthread_mutex_unlock(&mu);
        fn(call_arg);
        pthread_mutex_lock(&mu);
        f->go = 1;
        pthread_cond_signal(&f->cv);
    }
    pthread_mutex_unlock(&mu);
}

static void f_yield(HostFiber *self) {
    pthread_mutex_lock(&mu);
    give_loop();
    int run = wait_go(self);
    pthread_mutex_unlock(&mu);
    if (!run)
        longjmp(self->out, 1);
}

static __attribute__((noreturn)) void f_exit(HostFiber *self) {
    pthread_mutex_lock(&mu);
    give_loop();
    pthread_mutex_unlock(&mu);
    longjmp(self->out, 1);
}

static void f_free(HostFiber *f) {
    if (f->started) {
        pthread_mutex_lock(&mu);
        f->kill = 1;
        pthread_cond_signal(&f->cv);
        pthread_mutex_unlock(&mu);
        pthread_join(f->th, NULL);
    }
    pthread_cond_destroy(&f->cv);
    free(f);
}

static void f_call_on_loop(void (*fn)(void *), void *arg) {
    HostFiber *self = self_fiber;
    if (!self || pthread_equal(pthread_self(), loop_thread)) {
        fn(arg);
        return;
    }
    pthread_mutex_lock(&mu);
    call_fn = fn;
    call_arg = arg;
    give_loop();
    while (!self->go)
        pthread_cond_wait(&self->cv, &mu);
    self->go = 0;
    pthread_mutex_unlock(&mu);
}

const FiberOps fiber_pthread_ops = {
    "pthread", f_init, f_create, f_run, f_yield, f_exit, f_free, f_call_on_loop,
};
#endif
