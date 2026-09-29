#include "common.h"
#include "ultra_internal.h"

/* libultra initialize.c, from before 2.0J and before osViClock. */
OSTime osClockRate = OS_CLOCK_RATE;
u32 __osShutdown = 0;

/* .bss, 0x803FCD80-0x803FCD90 (tools/bss_c.py) */
u32 __osFinalrom;

#include "src/libultra/os/initialize.c"

#include "src/libultra/io/pirawread.c"
