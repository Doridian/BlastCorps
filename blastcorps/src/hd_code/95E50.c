#include "common.h"
#include "ultra_internal.h"

/*
 * libultra vimgr.c.  Its thread, stack, queue and messages are this file's
 * .bss (defined here, the messages' fields are stored through one lui).
 * The 2.0I OSIoMesg has piHandle, so viCounterMsg covers viMgrMain's
 * retrace count.
 */
#define VI_STACKSIZE 0x1000
OSDevMgr __osViDevMgr = { 0 };

/* .bss, 0x803FDF70-0x803FF180 (tools/bss_c.py) */
OSThread D_803FDF70;
u8 D_803FE120[VI_STACKSIZE];
OSMesgQueue D_803FF120;
OSMesg D_803FF138[5];
OSIoMesg D_803FF150;
OSIoMesg D_803FF168;

extern u16 D_803FF17C;
#define viThread D_803FDF70
#define viThreadStack D_803FE120
#define viEventQueue D_803FF120
#define viEventBuf D_803FF138
#define viRetraceMsg D_803FF150
#define viCounterMsg D_803FF168
#define retrace D_803FF17C

#include "src/libultra/io/vimgr.c"
