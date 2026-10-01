/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the RSP
 * microcodes' interface, the graphics (gbi.h) and audio (abi.h) command
 * lists.
 */
#ifndef PORT_SDK_MBI_H
#define PORT_SDK_MBI_H

#define M_GFXTASK 1
#define M_AUDTASK 2
#define M_VIDTASK 3

#define NUM_SEGMENTS 16
#define SEGMENT_OFFSET(a) ((unsigned int)(a) & 0x00ffffff)
#define SEGMENT_NUMBER(a) (((unsigned int)(a) << 4) >> 28)
#define SEGMENT_ADDR(num, off) (((num) << 24) + (off))

#define G_ON (1)
#define G_OFF (0)

#include <PR/gbi.h>
#include <PR/abi.h>

#endif
