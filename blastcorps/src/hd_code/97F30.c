#include "common.h"
#include "ultra_internal.h"

/*
 * libultra timerintr.c, with its .bss (defined here, __osCurrentTime's two
 * halves are stored through one lui).
 */

/* .bss, 0x803FF2F0-0x803FF330 (tools/bss_c.py) */
OSTimer __osBaseTimer;
OSTime __osCurrentTime;
u32 __osBaseCounter;
u32 __osViIntrCount;
u32 __osTimerCounter;

OSTimer *__osTimerList = &__osBaseTimer;

#include "src/libultra/os/timerintr.c"
