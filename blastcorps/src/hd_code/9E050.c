#include "common.h"
#include "ultra_internal.h"

/* siacs.c's state; the queue and its buffer are in hd_code's .bss. */
u32 __osSiAccessQueueEnabled = 0;
#define siAccessBuf D_803FF330

/* .bss, 0x803FF330-0x803FF350 (tools/bss_c.py) */
OSMesg D_803FF330[1];
OSMesgQueue __osSiAccessQueue;

#include "src/libultra/io/siacs.c"

#include "src/libultra/io/si.c"

