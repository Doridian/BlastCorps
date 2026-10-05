/* libultra io/vi.c: the functions.  Its data, if any, is defined by the includer. */
/*
 * Older than ultralib's, as in the libultra SM64 (US) links: osTvType is
 * copied to tvType first, only NTSC and MPAL modes are chosen from, the
 * framebuffers aren't set, and osViClock is set here.  The includer provides
 * vi, tvType and the two modes as viModeNtsc and viModeMpal.
 */
#include "common.h"
#include "ultra_internal.h"

void __osViInit(void) {
    tvType = osTvType;
    bzero(vi, sizeof(vi));
    __osViCurr = &vi[0];
    __osViNext = &vi[1];
    __osViNext->retraceCount = 1;
    __osViCurr->retraceCount = 1;
    if (tvType == OS_TV_NTSC) {
        __osViNext->modep = &viModeNtsc;
        osViClock = VI_NTSC_CLOCK;
    } else {
        __osViNext->modep = &viModeMpal;
#ifdef VERSION_EU
        osViClock = VI_PAL_CLOCK; /* eu's libultra: the PAL clock */
#else
        osViClock = VI_MPAL_CLOCK;
#endif
    }
    __osViNext->state = VI_STATE_BLACK;
    __osViNext->control = __osViNext->modep->comRegs.ctrl;
    while (IO_READ(VI_CURRENT_REG) > 10) {
    }
    IO_WRITE(VI_CONTROL_REG, 0);
    __osViSwapContext();
}
