#include "common.h"

#include "ultra_internal.h"

/* .bss, 0x803FF180-0x803FF200 (tools/bss_c.py) */
__OSEventState __osEventStateTab[0x10];

#include "src/libultra/os/seteventmesg.c"

