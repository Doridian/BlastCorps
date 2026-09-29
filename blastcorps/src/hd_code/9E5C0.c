#include "common.h"
#include "ultra_internal.h"
#include <PR/rdb.h>

/* libultra syncputchars.c */
unsigned int __osRdbSendMessage = 0;
unsigned int __osRdbWriteOK = 1;

#include "src/libultra/os/syncputchars.c"
