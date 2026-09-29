#include "common.h"

#include "src/libultra/io/aisetfreq.c"

/* osAiSetNextBuffer's static hdwrBugFlag. */
static u8 hdwrBugFlag = 0;

/* osAiSetNextBuffer is osAiSetNextBuffer. */

#include "src/libultra/io/aisetnextbuf.c"
