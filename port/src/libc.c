/*
 * libultra's libc for the game, on the host's.  The game's names are
 * renamed (port_game.h) so these don't interpose on the host libc.
 * sprintf and the copies are port/host/libc64.c's (the host's memmove
 * wants a size_t).
 */
#include <stdarg.h>
#include "common.h"
#include "port.h"

typedef unsigned int port_size_t;

port_size_t n64_strlen(const char *s) {
    const char *p = s;
    while (*p)
        p++;
    return p - s;
}

char *n64_strchr(const char *s, int c) {
    for (;; s++) {
        if (*s == (char)c)
            return (char *)s;
        if (*s == 0)
            return NULL;
    }
}
