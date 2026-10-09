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
/* The port's display lists, segment bases and audio commands hold KSEG0
   addresses where the N64's hold physical ones: everything that reads them
   (gfx.c's seg_to_k0 and segment bases, aspmain.c, port_ptr) masks to
   physical itself, as the RSP ignores the top bits.  So K0_TO_PHYS is the
   identity, here and in os.h, osVirtualToPhysical (ultra.c) and common.h's
   STATIC_K0_TO_PHYS and K0_TO_PHYS_ADD.  Not in the engine check build
   (PORT_ENGINE_CHECK), whose translations write the N64's physical words
   that the native code's are compared with. */
#ifdef PORT_ENGINE_CHECK
#define K0_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)
#else
#define K0_TO_PHYS(x) ((u32)(x))
#endif
#define K1_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)
#define PHYS_TO_K0(x) ((u32)(x) | 0x80000000)
#define PHYS_TO_K1(x) ((u32)(x) | 0xA0000000)

#endif
