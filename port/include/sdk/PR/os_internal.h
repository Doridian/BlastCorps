/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the operating
 * system's internal calls that the library code the port still builds
 * names (port/src/ultra.c has them).
 */
#ifndef PORT_SDK_OS_INTERNAL_H
#define PORT_SDK_OS_INTERNAL_H

#include <PR/os.h>

/* coprocessor 0 */
extern u32 __osGetCause(void);
extern void __osSetCause(u32);
extern u32 __osGetSR(void);
extern void __osSetSR(u32);
extern u32 __osDisableInt(void);
extern void __osRestoreInt(u32);
extern u32 __osSetFpcCsr(u32);
extern u32 __osGetFpcCsr(void);

/* the serial interface and the RSP's registers */
extern u32 __osSiGetStatus(void);
extern s32 __osSiRawWriteIo(u32, u32);
extern s32 __osSiRawReadIo(u32, u32 *);
extern s32 __osSiRawStartDma(s32, void *);
extern u32 __osSpGetStatus(void);
extern void __osSpSetStatus(u32);
extern s32 __osSpSetPc(u32);
extern s32 __osSpRawStartDma(s32, u32, void *, u32);

extern void __osError(s16, s16, ...);

#endif
