#include "common.h"

/* controller.c's EEPROM timer, in hd_code's .bss (ultra_internal.h declares them). */
#define __osEepromTimer D_803FF288
#define __osEepromTimerQ D_803FF2A8
#define __osEepromTimerMsg D_803FF2C0

#define osEepromLongWrite func_802042D0

#include "src/libultra/io/conteeplongwrite.c"
