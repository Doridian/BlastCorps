#include "common.h"

/* __osPfsGetStatus is this libultra's __osPfsGetStatus (in its pfsinit.c). */

/* func_80206804 is contramread.c's static __osPackRamReadData. */
#define __osPackRamReadData func_80206804

#include "ultra_internal.h"

/* .bss, 0x8021ABF0-0x8021AC30 (tools/bss_c.py) */
OSPifRam __osPfsPifRam;

#include "src/libultra/io/contramread.c"
