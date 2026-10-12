/*
 * The game's sprintf and copies (port_game.h's n64_ names), as host code:
 * the game's C passes its sizes as 32-bit ints; these take them as the
 * game declares them and call the host's functions with a real size_t.
 * sprintf's arguments arrive by the host's varargs convention.
 */
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
    memmove(dst, src, (size_t)(unsigned)n);
}

void n64_bzero(void *p, int n) {
    memset(p, 0, (size_t)(unsigned)n);
}

void *n64_memcpy(void *dst, const void *src, uint32_t n) {
    return memmove(dst, src, n);
}
