#include "common.h"
#include "ultra_internal.h"

/* libultra vi.c.  The NTSC and MPAL modes it picks from are data-only objects. */
static __OSViContext vi[2] = { 0 };
__OSViContext *__osViCurr = &vi[0];
__OSViContext *__osViNext = &vi[1];
static u32 tvType = OS_TV_NTSC;
s32 osViClock = VI_NTSC_CLOCK;

extern OSViMode D_803077F0;
extern OSViMode D_80307840;
#define viModeNtsc D_803077F0
#define viModeMpal D_80307840

/* __osViInit is __osViInit.  eu's picks the PAL clock and is built -mips1
 * (the Makefile's MIPS1_C_FILES). */
#include "src/libultra/io/vi.c"
