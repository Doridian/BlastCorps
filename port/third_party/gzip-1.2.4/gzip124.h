/* gzip 1.2.4's deflate over memory (gzip124.c).  GNU GPL version 2 or later. */
#ifndef GZIP124_H
#define GZIP124_H
#include <stddef.h>
#include <stdint.h>

/* The deflate stream gzip 1.2.4 makes of in[0..n) at `level` (1-9), then
   its CRC-32 and length (little-endian): a gzip member's body, after the
   header.  malloc'd; NULL on failure. */
uint8_t *gzip124_compress(const uint8_t *in, size_t n, int level, size_t *out_len);

#endif
