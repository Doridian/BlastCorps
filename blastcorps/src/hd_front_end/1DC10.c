#include "common.h"

/* The EEPROM's PIF RAM, in hd_front_end's .bss (ultra_internal.h declares it). */
#define __osEepPifRam D_8021ABB0

/* conteepwrite.c's functions, with its statics. */
/* conteepread.c's. */

#include "ultra_internal.h"

/* .bss, 0x8021ABB0-0x8021ABF0 (tools/bss_c.py) */
OSPifRam D_8021ABB0;

#include "src/libultra/io/conteepwrite.c"

#include "src/libultra/io/conteepread.c"
