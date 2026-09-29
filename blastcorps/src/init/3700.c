#include "common.h"
#include "ultra_internal.h"

/* .bss, 0x80224D00-0x80224D80 (tools/bss_c.py) */
__OSEventState __osEventStateTab[0x10];

#include "src/libultra/os/seteventmesg.c"
