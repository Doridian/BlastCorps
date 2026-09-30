#ifndef GZIP_H
#define GZIP_H

/*
 * Rare's cut-down gzip 1.2.4 decompressor ("kiunzip").  init has a copy that
 * inflates hd_code, and hd_code has its own for hd_front_end.
 *
 * What Rare changed from gzip:
 * - window points straight at the destination, so there's no 32K sliding
 *   window, no wrapping and no flush_window().
 * - inbuf is a pointer to the compressed data in RAM, so get_byte() never
 *   refills it.
 * - huft_build() takes its tables from a bump allocator (huft_heap + hufts)
 *   in place of malloc(), so there is no huft_free().
 * - border, cplext and cpdext are byte tables.
 * - Most error checks are gone.
 */

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* gzip.h: compression method and header flags */
#define DEFLATED     8
#define ERROR        1

#define CONTINUATION 0x02
#define EXTRA_FIELD  0x04
#define ORIG_NAME    0x08
#define COMMENT      0x10

#define get_byte()  (inbuf[inptr++])
#define NEXTBYTE()  (uch)get_byte()

/* Huffman code lookup table entry, as in gzip.  A table's pointer is 32 bits
 * in the LP64 port (PTR32): huft_heap is sized for the N64's tables, and
 * huft_build() stores both kinds of table pointer through one pointer. */
struct huft {
    uch e; /* number of extra bits or operation */
    uch b; /* number of bits in this code or subcode */
    union {
        ush n;          /* literal, length base, or distance base */
        struct huft *PTR32 t; /* pointer to next level of table */
    } v;
};

/* inflate.c */
extern ulg bb;
extern unsigned bk;
extern unsigned hufts;
extern ush mask_bits[];
extern int lbits;
extern int dbits;

int huft_build(unsigned *b, unsigned n, unsigned s, ush *d, uch *e, struct huft *PTR32 *t, int *m);
int inflate_codes(struct huft *tl, struct huft *td, int bl, int bd);
int inflate_stored(void);
int inflate_fixed(void);
int inflate_dynamic(void);
int inflate_block(int *e);
int inflate(void);

/* unzip.c / util.c */
extern uch *inbuf;
extern uch *window;
extern struct huft *huft_heap;
extern int method;
extern int exit_code;
extern long header_bytes;
extern long bytes_in;
extern long bytes_out;
extern unsigned insize;
extern unsigned inptr;
extern unsigned outcnt;

int get_method(void);
void clear_bufs(void);
unsigned bi_reverse(unsigned code, int len);

#endif
