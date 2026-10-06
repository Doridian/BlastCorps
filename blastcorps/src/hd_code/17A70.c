#include "common.h"
#include "gzip.h"
#include "functions.h"

/* hd_code's copy of Rare's gzip driver (see src/gzip_unzip.inc.c).  Its
 * message is in hd_code's .data, which isn't split yet. */

/* "kiunzip: unknown method %d -- get newer version of gzip\n" */

#define GZIP_UNZIP          func_8025C230
#define GZIP_PRINTF         func_8029A7E4
#define GZIP_UNKNOWN_METHOD "kiunzip: unknown method %d -- get newer version of gzip\n"

/* .bss, 0x803669C0-0x80366A00 (tools/bss_c.py) */
uch *inbuf;
uch *window;
u8 D_803669C8[4];
long header_bytes;
u8 D_803669D0[8];
long bytes_in;
long bytes_out;
u8 D_803669E0[8];
unsigned insize;
unsigned inptr;
unsigned outcnt;

/* .data, 0x802E8CC0-0x802E8CD0 (tools/data_c.py) */
int method = 8;
int exit_code = 0;

#include "src/gzip_unzip.inc.c"
