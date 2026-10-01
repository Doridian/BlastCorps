#include "common.h"

/* (the port's compiler does 64-bit arithmetic itself) */
#ifndef TARGET_PC
#include "src/libultra/libc/ll.c"
#endif

/* func_802D4E10 is alCSPGetState. */
#define alCSPGetState func_802D4E10

#include "src/libultra/audio/cspgetstate.c"
