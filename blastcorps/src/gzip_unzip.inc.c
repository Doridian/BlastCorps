/*
 * Rare's gzip driver around inflate(): the unzip entry point, gzip.c's
 * get_method, bits.c's bi_reverse and util.c's clear_bufs.
 *
 * Included by init/1660.c and hd_code/17A70.c.  The includer defines
 * GZIP_UNZIP (the entry point's name), GZIP_PRINTF (where the error message
 * goes) and GZIP_UNKNOWN_METHOD (the message).
 */

/* Inflate the gzip member at *src to *dst, advancing both past it.  heap is
 * where huft_build puts its tables.  The handwritten code calls it too,
 * with its own words for src and dst (PTR32). */
void GZIP_UNZIP(uch *PTR32 *src, uch *PTR32 *dst, struct huft *heap) {
    inbuf = *src;
    window = *dst;
    huft_heap = heap;

    clear_bufs();

    if (*(inbuf + inptr) != 0x1F) {
        inptr += 1;
    }
    method = get_method();
    if (method >= 0) {
        inflate();
        *src += inptr;
        *dst += outcnt;
    }
}

/* gzip 1.2.4, gzip.c, with the magic check, the other formats and the file
 * name handling gone. */
int get_method(void) {
    uch flags;
    unsigned len;

    inptr += 2; /* magic */
    method = -1;
    header_bytes = 0;

    method = (int)get_byte();
    if (method != DEFLATED) {
        GZIP_PRINTF(GZIP_UNKNOWN_METHOD, method);
        exit_code = ERROR;
        return -1;
    }
    flags = (uch)get_byte();

    inptr += 6; /* time stamp, extra flags, OS type */

    if ((flags & CONTINUATION) != 0) {
        inptr += 2;
    }
    if ((flags & EXTRA_FIELD) != 0) {
        len = (unsigned)get_byte();
        len |= ((unsigned)get_byte()) << 8;
        inptr += len;
    }
    if ((flags & ORIG_NAME) != 0) {
        while (get_byte() != 0)
            ;
    }
    if ((flags & COMMENT) != 0) {
        while (get_byte() != 0)
            ;
    }
    header_bytes = inptr + 2 * sizeof(long);
    return method;
}

/* gzip 1.2.4, bits.c */
unsigned bi_reverse(unsigned code, int len) {
    register unsigned res = 0;
    do {
        res |= code & 1;
        code >>= 1, res <<= 1;
    } while (--len > 0);
    return res >> 1;
}

/* gzip 1.2.4, util.c */
void clear_bufs(void) {
    outcnt = 0;
    insize = inptr = 0;
    bytes_in = bytes_out = 0;
}
