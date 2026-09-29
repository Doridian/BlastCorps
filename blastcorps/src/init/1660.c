#include "common.h"
#include "gzip.h"

int method = DEFLATED;
int exit_code = 0;

void func_80220714(const char *fmt, ...);

#define GZIP_UNZIP          func_80220360
#define GZIP_PRINTF         func_80220714
#define GZIP_UNKNOWN_METHOD "kiunzip: unknown method %d -- get newer version of gzip\n"

/* .bss, 0x802229F0-0x80222A30 (tools/bss_c.py) */
uch *inbuf;
uch *window;
u8 D_802229F8[4];
long header_bytes;
u8 D_80222A00[8];
long bytes_in;
long bytes_out;
u8 D_80222A10[8];
unsigned insize;
unsigned inptr;
unsigned outcnt;

#include "src/gzip_unzip.inc.c"
