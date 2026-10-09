/* The game's scheduler (D_80315440, game/sched.h's Sched) and the fields the
   host reads, where they are in game memory (port.h's PORT_VAR). */
#ifndef HOST_SCHED_VARS_H
#define HOST_SCHED_VARS_H

#include "port.h"

extern char D_80315440[];
#define SCHED_OFF_UNK280 0x280      /* Sched.unk280: retraces in the level ("TIME IN LEVEL") */
#define SCHED_OFF_FRAMECOUNT 0x284  /* Sched.frameCount: retraces (the clock) */
#define SCHED_LEVEL_TIME (PORT_VAR(D_80315440) + SCHED_OFF_UNK280)
#define SCHED_FRAMECOUNT (PORT_VAR(D_80315440) + SCHED_OFF_FRAMECOUNT)

#endif
