/* hd_code 60F60: texture decoding and the queue of decodes waiting for
   their DMAs; the animated-texture slots (D_803C4B70); ...

   A texture's ROM bytes are a stream of big-endian s16 words, decoded in
   place: the stream is first copied to D_803C3250, then written back over
   the DMA's destination.  A word >= 0 is one literal unit (two texels);
   a negative word is a back reference: `w & 0x1F` units copied from
   `(w & 0x7FFF) >> 5` units back (16-bit outputs) or `(w & 0x7FE0) >> 4`
   bytes back (32-bit outputs).  The type picks the output format. */
#include "engine_b.h"
#include "engine_b_types.h"

extern u8 D_803C3250[0x1000];           /* the stream's copy */
extern TexDecode D_803C4250[144];       /* decodes waiting for their DMAs */
extern TexDecode *PTR32 D_803C4B54;     /* the next one to do */

/* ---- the decoders: `in` holds `len` bytes of words; they write from
   `out` and return the end of what they wrote.  The `pal` ones look
   units up in a u16 palette. ------------------------------------------ */

/* 6, func_802A5958: 2 x 8-bit, (b & 0x38) << 2 | (b & 7) << 1 each */
static u8 *tex_decode_6(const s16 *in, s32 len, u8 *out) {
    ENG_COST(8);
    for (;;) {
        s32 w;

        ENG_COST(2);
        if (len == 0) {
            break;
        }
        ENG_COST(4);
        w = *in++;
        if (w >= 0) {
            u32 hi = (u32)w >> 8, lo = w & 0xFF;

            ENG_COST(17);
            out[0] = ((hi & 0x38) << 2) | ((hi & 7) << 1);
            out[1] = ((lo & 0x38) << 2) | ((lo & 7) << 1);
            out += 2;
            len -= 2;
        } else {
            s32 n = w & 0x1F;
            const u16 *from = (const u16 *)(out - ((w & 0x7FFF) >> 5));

            ENG_COST(5);
            len -= 2;
            for (;;) {
                ENG_COST(2);
                if (n == 0) {
                    break;
                }
                ENG_COST(6);
                *(u16 *)out = *from++;
                out += 2;
                n--;
            }
        }
    }
    ENG_COST(9);
    return out;
}

/* 3, func_802A5A2C: 2 x 8-bit, each << 1 */
static u8 *tex_decode_3(const s16 *in, s32 len, u8 *out) {
    ENG_COST(8);
    for (;;) {
        s32 w;

        ENG_COST(2);
        if (len == 0) {
            break;
        }
        ENG_COST(4);
        w = *in++;
        if (w >= 0) {
            ENG_COST(9);
            out[0] = (u32)w >> 8 << 1;
            out[1] = (w & 0xFF) << 1;
            out += 2;
            len -= 2;
        } else {
            s32 n = w & 0x1F;
            const u16 *from = (const u16 *)(out - ((w & 0x7FFF) >> 5));

            ENG_COST(5);
            len -= 2;
            for (;;) {
                ENG_COST(2);
                if (n == 0) {
                    break;
                }
                ENG_COST(6);
                *(u16 *)out = *from++;
                out += 2;
                n--;
            }
        }
    }
    ENG_COST(9);
    return out;
}

/* 1, func_802A5AE0: 16-bit, a 0 inserted above bit 5 (5-5-5 to 5-5-5-1) */
static u8 *tex_decode_1(const s16 *in, s32 len, u8 *out) {
    ENG_COST(8);
    for (;;) {
        s32 w;

        ENG_COST(2);
        if (len == 0) {
            break;
        }
        ENG_COST(4);
        w = *in++;
        if (w >= 0) {
            ENG_COST(8);
            *(u16 *)out = ((w & 0xFFC0) << 1) | (w & 0x3F);
            out += 2;
            len -= 2;
        } else {
            s32 n = w & 0x1F;
            const u16 *from = (const u16 *)(out - ((w & 0x7FFF) >> 5));

            ENG_COST(5);
            len -= 2;
            for (;;) {
                ENG_COST(2);
                if (n == 0) {
                    break;
                }
                ENG_COST(6);
                *(u16 *)out = *from++;
                out += 2;
                n--;
            }
        }
    }
    ENG_COST(9);
    return out;
}

/* the 32-bit outputs' back reference */
static u8 *tex_backref32(s32 w, u8 *out) {
    s32 n = w & 0x1F;
    const u32 *from = (const u32 *)(out - ((w & 0x7FE0) >> 4));

    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            break;
        }
        ENG_COST(6);
        *(u32 *)out = *from++;
        out += 4;
        n--;
    }
    return out;
}

/* 2, func_802A5B90: 4-4-4-3 to 32-bit RGBA, 4 bits per channel shifted
   to the top */
static u8 *tex_decode_2(const s16 *in, s32 len, u8 *out) {
    ENG_COST(8);
    for (;;) {
        s32 w;

        ENG_COST(2);
        if (len == 0) {
            break;
        }
        ENG_COST(4);
        w = *in++;
        if (w >= 0) {
            ENG_COST(15);
            *(u32 *)out = ((w & 0x7800) << 17) | ((w & 0x780) << 13) | ((w & 0x78) << 9)
                        | ((w & 7) << 5);
            out += 4;
            len -= 2;
        } else {
            ENG_COST(5);
            len -= 2;
            out = tex_backref32(w, out);
        }
    }
    ENG_COST(9);
    return out;
}

/* 4, func_802A5C5C: 2 x 8-bit through the palette: the top 7 bits pick
   an entry, shifted up past the low bit */
static u8 *tex_decode_4(const s16 *in, s32 len, u8 *out, const u8 *pal) {
    ENG_COST(8);
    for (;;) {
        s32 w;

        ENG_COST(2);
        if (len == 0) {
            break;
        }
        ENG_COST(4);
        w = *in++;
        if (w >= 0) {
            u32 hi = (u32)w >> 8;

            ENG_COST(18);
            out += 4;
            ((u16 *)out)[-2] = (*(const u16 *)(pal + (hi & 0xFE)) << 1) | (hi & 1);
            len -= 2;
            ((u16 *)out)[-1] = (*(const u16 *)(pal + (w & 0xFE)) << 1) | (w & 1);
        } else {
            ENG_COST(5);
            len -= 2;
            out = tex_backref32(w, out);
        }
    }
    ENG_COST(9);
    return out;
}

/* 5, func_802A5D34: a 12-bit palette index and 4 bits of alpha to 32-bit
   RGBA (the palette's 5-5-5 widened) */
static u8 *tex_decode_5(const s16 *in, s32 len, u8 *out, const u8 *pal) {
    ENG_COST(8);
    for (;;) {
        s32 w;

        ENG_COST(2);
        if (len == 0) {
            break;
        }
        ENG_COST(4);
        w = *in++;
        if (w >= 0) {
            u32 c = *(const u16 *)(pal + (((u32)w >> 4) << 1));

            ENG_COST(19);
            *(u32 *)out = ((c & 0x7C00) << 17) | ((c & 0x3E0) << 14) | ((c & 0x1F) << 11)
                        | ((w & 0xF) << 4);
            out += 4;
            len -= 2;
        } else {
            ENG_COST(5);
            len -= 2;
            out = tex_backref32(w, out);
        }
    }
    ENG_COST(9);
    return out;
}

/* 0, func_802A5E10: raw, in doublewords */
static u8 *tex_decode_0(const u8 *in, u32 len, u8 *out) {
    u32 n = len >> 3;

    ENG_COST(6);
    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            break;
        }
        ENG_COST(6);
        ((u32 *)out)[0] = ((const u32 *)in)[0];
        ((u32 *)out)[1] = ((const u32 *)in)[1];
        in += 8;
        out += 8;
        n--;
    }
    ENG_COST(6);
    return out;
}

/* func_802A57DC: decode `req` in place; returns the bytes written. */
u32 func_802A57DC(TexDecode *req) {
    u32 len = req->length;
    u32 n8 = len >> 3 << 3;
    const u8 *from = (const u8 *)req->dst;
    u8 *to = D_803C3250;
    u8 *out;

    ENG_COST(15);
    len -= n8;
    for (;;) {
        ENG_COST(2);
        if (n8 == 0) {
            break;
        }
        ENG_COST(6);
        ((u32 *)to)[0] = ((const u32 *)from)[0];
        ((u32 *)to)[1] = ((const u32 *)from)[1];
        from += 8;
        to += 8;
        n8 -= 8;
    }
    for (;;) {
        ENG_COST(2);
        if (len == 0) {
            break;
        }
        ENG_COST(6);
        *(u16 *)to = *(const u16 *)from;
        from += 2;
        to += 2;
        len -= 2;
    }

    out = (u8 *)req->dst;
    {
        const s16 *in = (const s16 *)D_803C3250;
        s32 n = req->length;
        const u8 *pal = (const u8 *)req->param;
        u8 *start = out;

        ENG_COST(8);
        switch (req->type) {
            case 0:
                ENG_COST(2);
                out = tex_decode_0(D_803C3250, n, out);
                ENG_COST(2);
                break;
            case 1:
                ENG_COST(3 + 2);
                out = tex_decode_1(in, n, out);
                ENG_COST(2);
                break;
            case 2:
                ENG_COST(3 + 3 + 2);
                out = tex_decode_2(in, n, out);
                ENG_COST(2);
                break;
            case 4:
                ENG_COST(3 + 3 + 3 + 2);
                out = tex_decode_4(in, n, out, pal);
                ENG_COST(2);
                break;
            case 5:
                ENG_COST(3 + 3 + 3 + 3 + 2);
                out = tex_decode_5(in, n, out, pal);
                ENG_COST(2);
                break;
            case 3:
                ENG_COST(3 + 3 + 3 + 3 + 3 + 2);
                out = tex_decode_3(in, n, out);
                ENG_COST(2);
                break;
            case 6:
                ENG_COST(3 + 3 + 3 + 3 + 3 + 3 + 2);
                out = tex_decode_6(in, n, out);
                ENG_COST(2);
                break;
            default:
                ENG_COST(3 + 3 + 3 + 3 + 3 + 3);
                break;
        }
        ENG_COST(10);
        return out - start;
    }
}

/* ---- the decode queue ---------------------------------------------------- */

/* func_802A5720: empty it */
void func_802A5720(void) {
    ENG_COST(17);
    D_803C4B50 = D_803C4250;
    D_803C4B54 = D_803C4250;
}

/* func_802A5764: queue a decode (the original takes $s1, $s2, $s3, $fp) */
void eng_tex_queue_decode(u32 dst, u32 length, u32 type, u32 param) {
    TexDecode *q = D_803C4B50;

    ENG_COST(18);
    q->dst = dst;
    q->length = length;
    q->type = type;
    q->param = param;
    D_803C4B50 = q + 1;
}

/* func_802A57AC: do the next queued decode, its DMA being in */
void func_802A57AC(void) {
    TexDecode *q = D_803C4B54;

    ENG_COST(6);
    func_802A57DC(q);
    ENG_COST(6);
    D_803C4B54 = q + 1;
}
