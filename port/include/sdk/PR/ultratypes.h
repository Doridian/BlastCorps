/*
 * The port's own N64 SDK headers (docs/DISTRIBUTION.md, "Replacing the SDK
 * parts"): only what the game's C and port/src use, written from the
 * hardware's and the libraries' documented interfaces.  The N64 build still
 * uses blastcorps/include/2.0I, which it needs to match the ROM; these are
 * the port's, and `-DPORT_SDK_2_0I=ON` swaps 2.0I back in, which is how
 * port/tools/sdk_identity.py checks that every object comes out the same
 * either way.
 *
 * ultratypes.h: the fixed-size integer types, 32-bit whatever the host's
 * long is.
 */
#ifndef PORT_SDK_ULTRATYPES_H
#define PORT_SDK_ULTRATYPES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;
typedef volatile signed char vs8;
typedef volatile short vs16;
typedef volatile int vs32;
typedef volatile long long vs64;

typedef float f32;
typedef double f64;

/* A pointer whose 4 bytes something other than the C fixes (the ROM's data,
   libaudio, the display list's words): 32 bits in memory even where the
   host's are 64 (the LP64 port; clang's zero-extended __ptr32).  Every other
   pointer is the host's (docs/PORT.md, "The LP64 build").  `T *PTR32 p;` */
#if defined(PORT_LP64)
#define PTR32 __ptr32 __uptr
#else
#define PTR32
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

#endif
