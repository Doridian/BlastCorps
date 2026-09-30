/*
 * The 64-bit build's half of port/src/libc.c: sprintf and the copies, as
 * host code.  The game's C (through port-ilp32) passes its pointers in
 * 64-bit registers and its sizes as 32-bit ints; these take them as the
 * game declares them and call the host's functions with a real size_t.
 * sprintf's arguments arrive by the host's varargs convention, which is
 * what the converted callers use.
 */
#if defined(PORT_64BIT) || defined(PORT_MOVABLE)     /* (the movable build: host code in either width) */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"

/* Formatted apart, then copied: the game appends to a string with
   sprintf(buf, "%s ...", buf, ...) (hd_front_end/1C40.c), which
   libultra's _Printf does as it goes, and glibc's too; musl's (the
   WebAssembly build's) makes a mess of it. */
int n64_sprintf(char *buf, const char *fmt, ...) {
    va_list ap, aq;
    char tmp[1024];
    int n;

    va_start(ap, fmt);
    va_copy(aq, ap);
    n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    if (n >= 0 && (size_t)n < sizeof tmp) {
        memcpy(buf, tmp, (size_t)n + 1);
    } else if (n >= 0) {
        char *t = malloc((size_t)n + 1);
        if (!t)
            host_fatal("sprintf: no memory");
        vsnprintf(t, (size_t)n + 1, fmt, aq);
        memcpy(buf, t, (size_t)n + 1);
        free(t);
    }
    va_end(aq);
    va_end(ap);
    return n;
}

void n64_bcopy(const void *src, void *dst, int n) {
    host_cpu_charge(n / 2);
#ifdef PORT_ACCESS_PROFILE
    __port_access_copy(dst, (void *)src, n, 0);
#endif
    memmove(dst, src, (size_t)(unsigned)n);
}

void n64_bzero(void *p, int n) {
    host_cpu_charge(n / 4);
#ifdef PORT_ACCESS_PROFILE
    __port_access_set(p, n, 0);
#endif
    memset(p, 0, (size_t)(unsigned)n);
}

void *n64_memcpy(void *dst, const void *src, uint32_t n) {
    host_cpu_charge(n / 2);
#ifdef PORT_ACCESS_PROFILE
    __port_access_copy(dst, (void *)src, n, 0);
#endif
    return memmove(dst, src, n);
}
#endif
