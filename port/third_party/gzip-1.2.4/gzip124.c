/* gzip 1.2.4's deflate (deflate.c, trees.c, bits.c, unchanged) over memory
   instead of file descriptors: what gzip.c, zip.c and util.c would provide.
   The ROM's gzip members were made by gzip 1.2.4 at -6 (docs/ASSETS.md,
   "gzip"), and only its own deflate gives back their bytes.

   Copyright (C) 1992-1993 Jean-loup Gailly (deflate.c, trees.c, bits.c);
   this file is the port's.  GNU GPL version 2 or later (COPYING). */
#include <stdlib.h>
#include <string.h>

#include "tailor.h"
#include "gzip.h"
#include "lzw.h"
#include "gzip124.h"

DECLARE(uch, inbuf, INBUFSIZ + INBUF_EXTRA);
DECLARE(uch, outbuf, OUTBUFSIZ + OUTBUF_EXTRA);
DECLARE(ush, d_buf, DIST_BUFSIZE);
DECLARE(uch, window, 2L * WSIZE);
DECLARE(ush, tab_prefix, 1L << BITS);

int method = DEFLATED;
int level = 6;
int exit_code = OK;
int verbose = 0;
int quiet = 1;
int test = 0;
int to_stdout = 1;
int save_orig_name = 0;
int decrypt = 0;
unsigned insize, inptr, outcnt;
long bytes_in, bytes_out, header_bytes;
int ifd, ofd;
char ifname[] = "-", ofname[] = "-";
char *progname = "gzip";
long time_stamp;
long ifile_size = -1L;
long isize;
ulg crc;


static const uint8_t *src;
static size_t src_len, src_pos;
static uint8_t *dst;
static size_t dst_len, dst_cap;
static uint32_t crc_table[256];

static ulg crc_update(const uch *s, unsigned n) {
    static int made;
    if (!made) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++)
                c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            crc_table[i] = c;
        }
        made = 1;
    }
    uint32_t c = (uint32_t)crc ^ 0xFFFFFFFFu;
    while (n--)
        c = crc_table[(c ^ *s++) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

/* zip.c's file_read, from memory */
int file_read(char *buf, unsigned size) {
    size_t n = src_len - src_pos;
    if (n > size)
        n = size;
    if (n == 0)
        return 0;
    memcpy(buf, src + src_pos, n);
    src_pos += n;
    crc = crc_update((uch *)buf, (unsigned)n);
    isize += (ulg)n;
    return (int)n;
}

static void put_out(const uch *p, size_t n) {
    if (dst_len + n > dst_cap) {
        dst_cap = (dst_len + n) * 2 + 4096;
        dst = realloc(dst, dst_cap);
    }
    memcpy(dst + dst_len, p, n);
    dst_len += n;
}

void flush_outbuf() {
    if (outcnt == 0)
        return;
    put_out(outbuf, outcnt);
    bytes_out += (ulg)outcnt;
    outcnt = 0;
}

void flush_window() {}

ulg updcrc(s, n)
    uch *s;
    unsigned n;
{
    if (s == NULL) {
        crc = 0;
        return 0;
    }
    crc = crc_update(s, n);
    return crc;
}

static char *failed;
void error(m)
    char *m;
{
    failed = m;
}

void warn(a, b)
    char *a, *b;
{
    (void)a;
    (void)b;
}

void read_error() { failed = "read error"; }
void write_error() { failed = "write error"; }

uint8_t *gzip124_compress(const uint8_t *in, size_t n, int lvl, size_t *out_len) {
    ush attr = 0, deflate_flags = 0;

    src = in;
    src_len = n;
    src_pos = 0;
    dst = NULL;
    dst_len = dst_cap = 0;
    failed = NULL;
    insize = inptr = outcnt = 0;
    bytes_in = bytes_out = header_bytes = 0;
    isize = 0;
    crc = 0;
    level = lvl;
    method = DEFLATED;
    memset(window, 0, sizeof window);
    memset(tab_prefix, 0, sizeof tab_prefix);

    bi_init(0);
    ct_init(&attr, &method);
    lm_init(level, &deflate_flags);
    (void)deflate();
    put_long(crc);
    put_long((ulg)isize);
    flush_outbuf();
    if (failed) {
        free(dst);
        return NULL;
    }
    *out_len = dst_len;
    return dst;
}
