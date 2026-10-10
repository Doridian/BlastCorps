/* The game's scheduler (D_80315440, game/sched.h's Sched) and the fields the
   host reads, where they are in game memory (port.h's PORT_VAR; the fields'
   offsets are the N64 side's, port.h's host_layout). */
#ifndef HOST_SCHED_VARS_H
#define HOST_SCHED_VARS_H

#include "port.h"

extern char D_80315440[];
/* Sched.unk280: retraces in the level ("TIME IN LEVEL") */
#define SCHED_LEVEL_TIME (PORT_VAR(D_80315440) + host_layout(PORT_LAYOUT_SCHED_UNK280))
/* Sched.frameCount: retraces (the clock) */
#define SCHED_FRAMECOUNT (PORT_VAR(D_80315440) + host_layout(PORT_LAYOUT_SCHED_FRAMECOUNT))

#endif
