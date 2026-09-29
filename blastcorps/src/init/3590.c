#include "common.h"
#include "ultra_internal.h"

/* libultra thread.c */
struct __osThreadTail {
    OSThread *next;
    OSPri priority;
} __osThreadTail = { NULL, -1 };

OSThread *__osRunQueue = (OSThread *)&__osThreadTail;
OSThread *__osActiveQueue = (OSThread *)&__osThreadTail;
OSThread *__osRunningThread = NULL;
OSThread *__osFaultedThread = NULL;

#include "src/libultra/os/thread.c"
