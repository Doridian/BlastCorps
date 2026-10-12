/*
 * Runtime support: the game's threads' stacks, the SDK trace.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

/* A thread's host stack: from the host's allocator, one for each thread
   slot, the first time it is used, zeroed (nothing tests where they are);
   page-aligned, as a pthread stack must be (fiber_pthread.c: 16 KB pages
   on arm64 macOS). */
static void *stacks[PORT_MAX_THREADS];

void *host_thread_stack(int idx, uint32_t *size) {
    *size = PORT_STACK_SIZE;
    if (!stacks[idx]) {
        stacks[idx] = aligned_alloc(0x4000, PORT_STACK_SIZE);
        if (!stacks[idx])
            host_fatal("no memory for thread %d's stack", idx);
        memset(stacks[idx], 0, PORT_STACK_SIZE);
    }
    return stacks[idx];
}

/* is p inside one of the game's threads' stacks (strict mode: a list built in a local) */
int host_in_thread_stack(const void *p) {
    for (int i = 0; i < PORT_MAX_THREADS; i++)
        if (stacks[i] && (const uint8_t *)p >= (const uint8_t *)stacks[i] &&
            (const uint8_t *)p < (const uint8_t *)stacks[i] + PORT_STACK_SIZE)
            return 1;
    return 0;
}

#ifdef PORT_SDK_TRACE
/* -DPORT_SDK_TRACE=ON builds: port/src/gu.c's calls and their arguments, to
   $PORT_SDK_TRACE (port/tools/sdk_check checks the replacements on them).
   A record is the function's number and its argument words, host order. */
static FILE *sdk_trace_f;

void port_sdk_trace(int fn, int nwords) {
    static int init;
    uint32_t w = (uint32_t)fn;
    (void)nwords;
    if (!init) {
        init = 1;
        const char *s = getenv("PORT_SDK_TRACE");
        if (s)
            sdk_trace_f = fopen(s, "wb");
    }
    if (sdk_trace_f)
        fwrite(&w, 4, 1, sdk_trace_f);
}

void port_sdk_trace_word(uint32_t w) {
    if (sdk_trace_f)
        fwrite(&w, 4, 1, sdk_trace_f);
}
#endif
