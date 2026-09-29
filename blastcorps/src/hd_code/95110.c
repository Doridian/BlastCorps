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

/* __osViInit is __osViInit.  eu's picks the PAL clock and was built
 * differently (the loads of the VI registers get a nop each). */
#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/95110/__osViInit.s")
#else
#include "src/libultra/io/vi.c"
#endif
