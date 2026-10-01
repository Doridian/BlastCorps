/* The port's own SDK headers (PR/ultratypes.h has the why): the string
   functions the game's library code calls (port/src/libc.c). */
#ifndef PORT_SDK_STRING_H
#define PORT_SDK_STRING_H

#include <PR/ultratypes.h>

extern void *memcpy(void *, const void *, size_t);
extern char *strchr(const char *, int);
extern size_t strlen(const char *);

#endif
