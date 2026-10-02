/*
 * sprintf for the native engine code (engine.h's engine_sprintf), costing
 * the C nothing, as the translation's call of it did (gen_glue.py's
 * recomp_extern_sprintf): the 32-bit build's n64_sprintf is the N64 side's
 * (port/src/libc.c), whose own few instructions BEPass counts as the
 * game's C.  Built like the rest of port/engine, uncounted.  The other
 * builds' n64_sprintf is host code (port/host/libc64.c), uncounted already.
 */
#include "engine.h"

#if !defined(PORT_64BIT) && !defined(PORT_MOVABLE)
#include <stdarg.h>

extern int vsprintf(char *, const char *, va_list);

int engine_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}
#endif
