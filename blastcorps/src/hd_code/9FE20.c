#include "common.h"
#include "functions.h"

/* alFxParamHdl is func_802E4C78. */
#define alFxParamHdl func_802E4C78

#ifdef VERSION_EU
/* eu's abi.h puts the pole filter's gain in 8 bits (_filterBuffer) */
#include <PR/abi.h>
#undef aPoleFilter
#define aPoleFilter(pkt, f, g, s)                                                                      \
    {                                                                                                  \
        Acmd *_a = (Acmd *)pkt;                                                                        \
                                                                                                       \
        _a->words.w0 = (_SHIFTL(A_POLEF, 24, 8) | _SHIFTL(f, 16, 8) | _SHIFTL(g, 0, 8));             \
        _a->words.w1 = (unsigned int)(s);                                                              \
    }
#endif

/* Built -O3, which emits the functions in reverse order. */
#include "src/libultra/audio/reverb.c"
