#include "common.h"

/* controller.c's EEPROM timer, in hd_code's .bss (ultra_internal.h declares them). */
#define __osEepromTimer D_803FF288
#define __osEepromTimerQ D_803FF2A8
#define __osEepromTimerMsg D_803FF2C0

#define osEepromLongRead func_80204410

#include "src/libultra/io/conteeplongread.c"
