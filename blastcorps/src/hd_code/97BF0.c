#include "common.h"
#include "ultra_internal.h"

/* libultra piacs.c; its static piAccessBuf is in hd_code's .bss. */
u32 __osPiAccessQueueEnabled = 0;
#define piAccessBuf D_803FF2D0

/* .bss, 0x803FF2D0-0x803FF2F0 (tools/bss_c.py) */
OSMesg D_803FF2D0[1];
OSMesgQueue __osPiAccessQueue;

#include "src/libultra/io/piacs.c"

#include "src/libultra/os/getthreadpri.c"
