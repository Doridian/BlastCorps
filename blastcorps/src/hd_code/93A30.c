#include "common.h"

/* func_802D81F0 is alCSPPlay, alCSPSetVol alCSPSetVol. */
#define alCSPPlay func_802D81F0

/* Built -O3, which emits the files' functions in reverse order. */
#include "src/libultra/audio/cspsetvol.c"
#include "src/libultra/audio/cspplay.c"
