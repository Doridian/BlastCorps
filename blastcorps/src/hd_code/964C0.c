#include "common.h"

#include "ultra_internal.h"

/* .bss, 0x803FF200-0x803FF240 (tools/bss_c.py) */
OSTask D_803FF200;

#include "src/libultra/io/sptaskyielded.c"

/* sptask.c's static tmp_task, in hd_code's .bss. */
extern OSTask D_803FF200;
#define tmp_task D_803FF200

#include "src/libultra/io/sptask.c"

