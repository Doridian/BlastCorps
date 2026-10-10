/*
 * gzip 1.2.4, inflate.c, as cut down by Rare (see gzip.h): the functions.
 *
 * Included by each module that links a copy (init/0050.c and
 * hd_code/53220.c).  The includer supplies the tables (border, cplens,
 * cplext, cpdist, cpdext, mask_bits, lbits, dbits), either defined or extern.
 */

#ifdef TARGET_PC
#include "port_regions.h"
#endif

#define wp outcnt
#define slide window

#define NEEDBITS(n) {while(k<(n)){b|=((ulg)NEXTBYTE())<<k;k+=8;}}
#define DUMPBITS(n) {b>>=(n);k-=(n);}

#define BMAX 16
#define N_MAX 288

int huft_build(unsigned *b, unsigned n, unsigned s, ush *d, uch *e, struct huft **t, int *m) {
    unsigned a;
    unsigned c[BMAX + 1];
    unsigned f;
    int g;
    int h;
    register unsigned i;
    register unsigned j;
    register int k;
    int l;
    register unsigned *p;
    register struct huft *q;
    struct huft r;
    struct huft *u[BMAX];
    unsigned v[N_MAX];
    register int w;
    unsigned x[BMAX + 1];
    unsigned *xp;
    int y;
    unsigned z;

    /* Generate counts for each bit length */
    bzero(c, sizeof(c));
    p = b;
    i = n;
    do {
        c[*p]++;
        p++;
    } while (--i);
    if (c[0] == n) {
        *t = (struct huft *)NULL;
        *m = 0;
        return 0;
    }

    /* Find minimum and maximum length, bound *m by those */
    l = *m;
    for (j = 1; j <= BMAX; j++)
        if (c[j])
            break;
    k = j;
    if ((unsigned)l < j)
        l = j;
    for (i = BMAX; i; i--)
        if (c[i])
            break;
    g = i;
    if ((unsigned)l > i)
        l = i;
    *m = l;

    /* Adjust last length count to fill out codes, if needed */
    for (y = 1 << j; j < i; j++, y <<= 1)
        y -= c[j];
    y -= c[i];
    c[i] += y;

    /* Generate starting offsets into the value table for each length */
    x[1] = j = 0;
    p = c + 1, xp = x + 2;
    while (--i) {
        *xp++ = (j += *p++);
    }

    /* Make a table of values in order of bit lengths */
    p = b;
    i = 0;
    do {
        if ((j = *p++) != 0)
            v[x[j]++] = i;
    } while (++i < n);

    /* Generate the Huffman codes and for each, make the table entries */
    x[0] = i = 0;
    p = v;
    h = -1;
    w = -l;
    u[0] = (struct huft *)NULL;
    q = (struct huft *)NULL;
    z = 0;

    for (; k <= g; k++) {
        a = c[k];
        while (a--) {
            while (k > w + l) {
                h++;
                w += l;

                z = (z = g - w) > (unsigned)l ? l : z;
                if ((f = 1 << (j = k - w)) > a + 1) {
                    f -= a + 1;
                    xp = c + k;
                    while (++j < z) {
                        if ((f <<= 1) <= *++xp)
                            break;
                        f -= *xp;
                    }
                }
                z = 1 << j;

                /* Rare: tables come from a bump allocator, not malloc(). */
                q = huft_heap + hufts;
                hufts += z + 1;
#ifdef TARGET_PC
                /* the tables go at the pool's start (huft_heap) and the
                   ghost's buffers begin at +0xA000: an LP64 huft is twice
                   the N64's. */
                if (hufts * sizeof(struct huft) > PORT_REGION_OFF_D_80055400) {
                    __builtin_trap();
                }
#endif
                *t = q + 1;
                *(t = &(q->v.t)) = (struct huft *)NULL;
                u[h] = ++q;

                if (h) {
                    x[h] = i;
                    r.b = (uch)l;
                    r.e = (uch)(16 + j);
                    r.v.t = q;
                    j = i >> (w - l);
                    u[h - 1][j] = r;
                }
            }

            r.b = (uch)(k - w);
            if (p >= v + n)
                r.e = 99;
            else if (*p < s) {
                r.e = (uch)(*p < 256 ? 16 : 15);
                r.v.n = (ush)(*p);
                p++;
            } else {
                r.e = (uch)e[*p - s];
                r.v.n = d[*p++ - s];
            }

            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f)
                q[j] = r;

            for (j = 1 << (k - 1); i & j; j >>= 1)
                i ^= j;
            i ^= j;

            while ((i & ((1 << w) - 1)) != x[h]) {
                h--;
                w -= l;
            }
        }
    }

    return y != 0 && g != 1;
}

int inflate_codes(struct huft *tl, struct huft *td, int bl, int bd) {
    register unsigned e;
    unsigned n, d;
    unsigned w;
    struct huft *t;
    unsigned ml, md;
    register unsigned k;
    register ulg b;

    b = bb;
    k = bk;
    w = wp;

    ml = mask_bits[bl];
    md = mask_bits[bd];
    for (;;) {
        NEEDBITS((unsigned)bl)
        if ((e = (t = tl + ((unsigned)b & ml))->e) > 16)
            do {
                DUMPBITS(t->b)
                e -= 16;
                NEEDBITS(e)
            } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
        DUMPBITS(t->b)
        if (e == 16) {
            slide[w++] = (uch)t->v.n;
        } else {
            if (e == 15)
                break;

            NEEDBITS(e)
            n = t->v.n + ((unsigned)b & mask_bits[e]);
            DUMPBITS(e);

            NEEDBITS((unsigned)bd)
            if ((e = (t = td + ((unsigned)b & md))->e) > 16)
                do {
                    DUMPBITS(t->b)
                    e -= 16;
                    NEEDBITS(e)
                } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
            DUMPBITS(t->b)
            NEEDBITS(e)
            d = w - t->v.n - ((unsigned)b & mask_bits[e]);
            DUMPBITS(e)

            do {
                n -= (e = n);
                do {
                    slide[w++] = slide[d++];
                } while (--e);
            } while (n);
        }
    }

    wp = w;
    bb = b;
    bk = k;

    return 0;
}

int inflate_stored(void) {
    unsigned n;
    unsigned w;
    register unsigned k;
    register ulg b;

    b = bb;
    k = bk;
    w = wp;

    n = k & 7;
    DUMPBITS(n);

    NEEDBITS(16)
    n = ((unsigned)b & 0xffff);
    DUMPBITS(16)
    NEEDBITS(16)
    DUMPBITS(16)

    while (n--) {
        NEEDBITS(8)
        slide[w++] = (uch)b;
        DUMPBITS(8)
    }

    wp = w;
    bb = b;
    bk = k;
    return 0;
}

int inflate_fixed(void) {
    int i;
    struct huft *tl;
    struct huft *td;
    int bl;
    int bd;
    unsigned l[288];

    for (i = 0; i < 144; i++)
        l[i] = 8;
    for (; i < 256; i++)
        l[i] = 9;
    for (; i < 280; i++)
        l[i] = 7;
    for (; i < 288; i++)
        l[i] = 8;
    bl = 7;
    huft_build(l, 288, 257, cplens, cplext, &tl, &bl);

    for (i = 0; i < 30; i++)
        l[i] = 5;
    bd = 5;
    huft_build(l, 30, 0, cpdist, cpdext, &td, &bd);

    inflate_codes(tl, td, bl, bd);
    return 0;
}

int inflate_dynamic(void) {
    int i;
    unsigned j;
    unsigned l;
    unsigned m;
    unsigned n;
    struct huft *tl;
    struct huft *td;
    int bl;
    int bd;
    unsigned nb;
    unsigned nl;
    unsigned nd;
    register unsigned k;
    register ulg b;
    unsigned ll[286 + 30];

    b = bb;
    k = bk;

    NEEDBITS(5)
    nl = 257 + ((unsigned)b & 0x1f);
    DUMPBITS(5)
    NEEDBITS(5)
    nd = 1 + ((unsigned)b & 0x1f);
    DUMPBITS(5)
    NEEDBITS(4)
    nb = 4 + ((unsigned)b & 0xf);
    DUMPBITS(4)

    for (j = 0; j < nb; j++) {
        NEEDBITS(3)
        ll[border[j]] = (unsigned)b & 7;
        DUMPBITS(3)
    }
    for (; j < 19; j++)
        ll[border[j]] = 0;

    bl = 7;
    huft_build(ll, 19, 19, NULL, NULL, &tl, &bl);

    n = nl + nd;
    m = mask_bits[bl];
    i = l = 0;
    while ((unsigned)i < n) {
        NEEDBITS((unsigned)bl)
        j = (td = tl + ((unsigned)b & m))->b;
        DUMPBITS(j)
        j = td->v.n;
        if (j < 16)
            ll[i++] = l = j;
        else if (j == 16) {
            NEEDBITS(2)
            j = 3 + ((unsigned)b & 3);
            DUMPBITS(2)
            while (j--)
                ll[i++] = l;
        } else if (j == 17) {
            NEEDBITS(3)
            j = 3 + ((unsigned)b & 7);
            DUMPBITS(3)
            while (j--)
                ll[i++] = 0;
            l = 0;
        } else {
            NEEDBITS(7)
            j = 11 + ((unsigned)b & 0x7f);
            DUMPBITS(7)
            while (j--)
                ll[i++] = 0;
            l = 0;
        }
    }

    bb = b;
    bk = k;

    bl = lbits;
    huft_build(ll, nl, 257, cplens, cplext, &tl, &bl);
    bd = dbits;
    huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd);

    inflate_codes(tl, td, bl, bd);
    return 0;
}

int inflate_block(int *e) {
    unsigned t;
    register unsigned k;
    register ulg b;

    b = bb;
    k = bk;

    NEEDBITS(1)
    *e = (int)b & 1;
    DUMPBITS(1)

    NEEDBITS(2)
    t = (unsigned)b & 3;
    DUMPBITS(2)

    bb = b;
    bk = k;

    if (t == 2)
        return inflate_dynamic();
    if (t == 0)
        return inflate_stored();
    if (t == 1)
        return inflate_fixed();

    return 2;
}

int inflate(void) {
    int e;
    int r;
    unsigned h;

    wp = 0;
    bk = 0;
    bb = 0;

    h = 0;
    do {
        hufts = 0;
        if ((r = inflate_block(&e)) != 0)
            return r;
        if (hufts > h)
            h = hufts;
    } while (!e);

    while (bk >= 8) {
        bk -= 8;
        inptr--;
    }

    /* Rare's addition: skip the crc32 and length in the gzip trailer. */
    inptr += 8;

    return 0;
}
