#include "common.h"
#include "ultra_internal.h"

/* libultra pimgr.c; the thread, its stack and the event queue are in hd_code's .bss. */
#define PI_STACKSIZE 0x1000
OSDevMgr __osPiDevMgr = { 0 };
#define piThread D_803FCD90
#define piThreadStack D_803FCF40
#define piEventQueue D_803FDF40
#define piEventBuf D_803FDF58

/* .bss, 0x803FCD90-0x803FDF60 (tools/bss_c.py) */
OSThread D_803FCD90;
u8 D_803FCF40[PI_STACKSIZE];
OSMesgQueue D_803FDF40;
OSMesg D_803FDF58[1];

#include "src/libultra/io/pimgr.c"
