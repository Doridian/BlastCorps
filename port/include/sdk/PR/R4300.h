/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the VR4300's
 * address segments.
 */
#ifndef PORT_SDK_R4300_H
#define PORT_SDK_R4300_H

#include <PR/ultratypes.h>

#define K0BASE 0x80000000       /* cached, unmapped */
#define K1BASE 0xA0000000       /* uncached, unmapped */

#define K0_TO_K1(x) ((u32)(x) | 0xA0000000)
#define K0_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)
#define K1_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)
#define PHYS_TO_K0(x) ((u32)(x) | 0x80000000)
#define PHYS_TO_K1(x) ((u32)(x) | 0xA0000000)

#endif
