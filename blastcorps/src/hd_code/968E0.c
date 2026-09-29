#include "common.h"
#include "ultra_internal.h"

/* controller.c's own state; the EEPROM timer queue is in hd_code's .bss. */
s32 __osContinitialized = 0;
#define __osEepromTimerQ D_803FF2A8
#define __osEepromTimerMsg D_803FF2C0

/* osContInit is osContInit. */

/* .bss, 0x803FF240-0x803FF2D0 (tools/bss_c.py) */
OSPifRam __osContPifRam;
u8 __osContLastCmd;
u8 __osMaxControllers;
u8 D_803FF282[2];
u8 D_803FF284[4];
u8 D_803FF288[0x20];
OSMesgQueue D_803FF2A8;
OSMesg D_803FF2C0;

#include "src/libultra/io/controller.c"

#include "src/libultra/io/contsetch.c"
