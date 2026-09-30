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
#include <string.h>

#include "port.h"

int n64_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsprintf(buf, fmt, ap);
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
