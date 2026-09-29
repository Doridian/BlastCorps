/*
 * gzip 1.2.4, inflate.c, as cut down by Rare (see gzip.h): hd_code's copy,
 * which inflates hd_front_end.  init/0050.c is the same file.
 */
#include "common.h"
#include "gzip.h"

/* Tables for deflate from PKZIP's appnote.txt.  Rare made the bit counts
 * bytes; gzip has them as ush (and border as unsigned). */
static uch border[] = { /* Order of the bit length code lengths */
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
static ush cplens[] = { /* Copy lengths for literal codes 257..285 */
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
static uch cplext[] = { /* Extra bits for literal codes 257..285 */
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; /* 99==invalid */
static ush cpdist[] = { /* Copy offsets for distance codes 0..29 */
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577};
static uch cpdext[] = { /* Extra bits for distance codes */
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13};

ush mask_bits[] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

int lbits = 9; /* bits in base literal/length lookup table */
int dbits = 6; /* bits in base distance lookup table */

/* .bss, 0x8039CAE0-0x803A6AF0 (tools/bss_c.py) */
struct huft *huft_heap;
ulg bb;
unsigned bk;
unsigned hufts;
u8 D_8039CAF0[0xA000];

#include "src/gzip_inflate.inc.c"
