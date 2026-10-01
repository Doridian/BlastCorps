/*
 * hd_code 60F60 (us.v11 0x802A5720-0x802A6F00): texture decoding and the
 * queue of decodes waiting for their DMAs, as native C (engine.h).  (The
 * rest of 60F60, the effects' animated sprites, is still translated.)
 *
 * A texture's ROM bytes are a stream of big-endian s16 words, decoded in
 * place: the stream is first copied to D_803C3250, then written back over
 * where it was DMA'd.  A word >= 0 is a literal (two texels' worth); a
 * negative word is a back reference: `w & 0x1F` units copied from back in
 * the output, `(w & 0x7FFF) >> 5` bytes back for 16-bit units and
 * `(w & 0x7FE0) >> 4` bytes back for 32-bit ones.  The type picks the
 * output format.
 */
#include "engine.h"
#include "texture.h"

extern u8 D_803C3250[0x1000];           /* the stream's copy */
extern TexDecode D_803C4250[144];       /* decodes waiting for their DMAs */
extern TexDecode *PTR32 D_803C4B50;     /* where the next is queued */
extern TexDecode *PTR32 D_803C4B54;     /* the next to do */

/* ---- the decoders: `len` bytes of words from `in` to `out`; each returns
   the end of what it wrote ------------------------------------------------ */

/* a back reference of 16-bit units */
#define BACKREF16(b_ref, b_loop, b_copy)                                    \
    do {                                                                    \
        s32 n = w & 0x1F;                                                   \
        u16 *from = (u16 *)(out - ((w & 0x7FFF) >> 5));                     \
                                                                            \
        ENGINE_BLK(b_ref);                                                  \
        len -= 2;                                                           \
        for (;;) {                                                          \
            ENGINE_BLK(b_loop);                                             \
            if (n == 0) {                                                   \
                break;                                                      \
            }                                                               \
            ENGINE_BLK(b_copy);                                             \
            *(u16 *)out = *from++;                                          \
            out += 2;                                                       \
            n--;                                                            \
        }                                                                   \
    } while (0)

/* a back reference of 32-bit units */
#define BACKREF32(b_ref, b_loop, b_copy)                                    \
    do {                                                                    \
        s32 n = w & 0x1F;                                                   \
        u32 *from = (u32 *)(out - ((w & 0x7FE0) >> 4));                     \
                                                                            \
        ENGINE_BLK(b_ref);                                                  \
        len -= 2;                                                           \
        for (;;) {                                                          \
            ENGINE_BLK(b_loop);                                             \
            if (n == 0) {                                                   \
                break;                                                      \
            }                                                               \
            ENGINE_BLK(b_copy);                                             \
            *(u32 *)out = *from++;                                          \
            out += 4;                                                       \
            n--;                                                            \
        }                                                                   \
    } while (0)

/* type 6, func_802A5958: two 8-bit texels, (b & 0x38) << 2 | (b & 7) << 1 */
REGS(a0, a1, a3 -> a3)
u32 func_802A5958(u32 in_, s32 len, u32 out_) {
    s16 *in = (s16 *)in_;
    u8 *out = (u8 *)out_;

    ENGINE_BLK(802A5958);
    for (;;) {
        s32 w;

        ENGINE_BLK(802A5978);
        if (len == 0) {
            break;
        }
        ENGINE_BLK(802A5980);
        w = *in++;
        if (w >= 0) {
            u32 hi = (u32)w >> 8, lo = w & 0xFF;

            ENGINE_BLK(802A5990);
            out[0] = ((hi & 0x38) << 2) | ((hi & 7) << 1);
            out[1] = ((lo & 0x38) << 2) | ((lo & 7) << 1);
            out += 2;
            len -= 2;
        } else {
            BACKREF16(802A59D4, 802A59E8, 802A59F0);
        }
    }
    ENGINE_BLK(802A5A08);
    return (u32)out;
}

/* type 3, func_802A5A2C: two 8-bit texels, each << 1 */
REGS(a0, a1, a3 -> a3)
u32 func_802A5A2C(u32 in_, s32 len, u32 out_) {
    s16 *in = (s16 *)in_;
    u8 *out = (u8 *)out_;

    ENGINE_BLK(802A5A2C);
    for (;;) {
        s32 w;

        ENGINE_BLK(802A5A4C);
        if (len == 0) {
            break;
        }
        ENGINE_BLK(802A5A54);
        w = *in++;
        if (w >= 0) {
            ENGINE_BLK(802A5A64);
            out[0] = (u32)w >> 8 << 1;
            out[1] = (w & 0xFF) << 1;
            out += 2;
            len -= 2;
        } else {
            BACKREF16(802A5A88, 802A5A9C, 802A5AA4);
        }
    }
    ENGINE_BLK(802A5ABC);
    return (u32)out;
}

/* type 1, func_802A5AE0: a 16-bit texel, a 0 inserted above bit 5 */
REGS(a0, a1, a3 -> a3)
u32 func_802A5AE0(u32 in_, s32 len, u32 out_) {
    s16 *in = (s16 *)in_;
    u8 *out = (u8 *)out_;

    ENGINE_BLK(802A5AE0);
    for (;;) {
        s32 w;

        ENGINE_BLK(802A5B00);
        if (len == 0) {
            break;
        }
        ENGINE_BLK(802A5B08);
        w = *in++;
        if (w >= 0) {
            ENGINE_BLK(802A5B18);
            *(u16 *)out = ((w & 0xFFC0) << 1) | (w & 0x3F);
            out += 2;
            len -= 2;
        } else {
            BACKREF16(802A5B38, 802A5B4C, 802A5B54);
        }
    }
    ENGINE_BLK(802A5B6C);
    return (u32)out;
}

/* type 2, func_802A5B90: a 32-bit RGBA texel from 4-4-4-3 */
REGS(a0, a1, a3 -> a3)
u32 func_802A5B90(u32 in_, s32 len, u32 out_) {
    s16 *in = (s16 *)in_;
    u8 *out = (u8 *)out_;

    ENGINE_BLK(802A5B90);
    for (;;) {
        s32 w;

        ENGINE_BLK(802A5BB0);
        if (len == 0) {
            break;
        }
        ENGINE_BLK(802A5BB8);
        w = *in++;
        if (w >= 0) {
            ENGINE_BLK(802A5BC8);
            *(u32 *)out = ((w & 0x7800) << 17) | ((w & 0x780) << 13) | ((w & 0x78) << 9) |
                          ((w & 7) << 5);
            out += 4;
            len -= 2;
        } else {
            BACKREF32(802A5C04, 802A5C18, 802A5C20);
        }
    }
    ENGINE_BLK(802A5C38);
    return (u32)out;
}

/* type 4, func_802A5C5C: two 16-bit texels through the palette at `pal`:
   the top 7 bits of each byte pick the entry, shifted up past the low bit */
REGS(a0, a1, a3, t4 -> a3)
u32 func_802A5C5C(u32 in_, s32 len, u32 out_, u32 pal_) {
    s16 *in = (s16 *)in_;
    u8 *out = (u8 *)out_;
    u8 *pal = (u8 *)pal_;

    ENGINE_BLK(802A5C5C);
    for (;;) {
        s32 w;

        ENGINE_BLK(802A5C7C);
        if (len == 0) {
            break;
        }
        ENGINE_BLK(802A5C84);
        w = *in++;
        if (w >= 0) {
            u32 hi = (u32)w >> 8;

            ENGINE_BLK(802A5C94);
            out += 4;
            ((u16 *)out)[-2] = (*(u16 *)(pal + (hi & 0xFE)) << 1) | (hi & 1);
            len -= 2;
            ((u16 *)out)[-1] = (*(u16 *)(pal + (w & 0xFE)) << 1) | (w & 1);
        } else {
            BACKREF32(802A5CDC, 802A5CF0, 802A5CF8);
        }
    }
    ENGINE_BLK(802A5D10);
    return (u32)out;
}

/* type 5, func_802A5D34: a 32-bit RGBA texel from a 12-bit palette index
   (the palette's 5-5-5 widened) and 4 bits of alpha */
REGS(a0, a1, a3, t4 -> a3)
u32 func_802A5D34(u32 in_, s32 len, u32 out_, u32 pal_) {
    s16 *in = (s16 *)in_;
    u8 *out = (u8 *)out_;
    u8 *pal = (u8 *)pal_;

    ENGINE_BLK(802A5D34);
    for (;;) {
        s32 w;

        ENGINE_BLK(802A5D54);
        if (len == 0) {
            break;
        }
        ENGINE_BLK(802A5D5C);
        w = *in++;
        if (w >= 0) {
            u32 c = *(u16 *)(pal + (((u32)w >> 4) << 1));

            ENGINE_BLK(802A5D6C);
            *(u32 *)out = ((c & 0x7C00) << 17) | ((c & 0x3E0) << 14) | ((c & 0x1F) << 11) |
                          ((w & 0xF) << 4);
            out += 4;
            len -= 2;
        } else {
            BACKREF32(802A5DB8, 802A5DCC, 802A5DD4);
        }
    }
    ENGINE_BLK(802A5DEC);
    return (u32)out;
}

/* type 0, func_802A5E10: raw, `len` / 8 doublewords */
REGS(a0, a1, a3 -> a3)
u32 func_802A5E10(u32 in_, u32 len, u32 out_) {
    u32 *in = (u32 *)in_;
    u32 *out = (u32 *)out_;
    u32 n = len >> 3;

    ENGINE_BLK(802A5E10);
    for (;;) {
        ENGINE_BLK(802A5E28);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802A5E30);
        out[0] = in[0];
        out[1] = in[1];
        in += 2;
        out += 2;
        n--;
    }
    ENGINE_BLK(802A5E48);
    return (u32)out;
}

/* func_802A57DC: decode `req` in place; returns the bytes written */
u32 func_802A57DC(TexDecode *req) {
    u32 len = req->length;
    u32 n8 = len >> 3 << 3;
    u32 *from = (u32 *)req->dst;
    u32 *to = (u32 *)D_803C3250;
    u32 out = req->dst, end;
    u32 in = (u32)D_803C3250;

    ENGINE_BLK(802A57DC);
    len -= n8;
    for (;;) {
        ENGINE_BLK(802A5818);
        if (n8 == 0) {
            break;
        }
        ENGINE_BLK(802A5820);
        to[0] = from[0];
        to[1] = from[1];
        from += 2;
        to += 2;
        n8 -= 8;
    }
    for (;;) {
        ENGINE_BLK(802A5838);
        if (len == 0) {
            break;
        }
        ENGINE_BLK(802A5840);
        *(u16 *)to = *(u16 *)from;
        from = (u32 *)((u8 *)from + 2);
        to = (u32 *)((u8 *)to + 2);
        len -= 2;
    }
    ENGINE_BLK(802A5858);
    end = out;
    if (req->type == 0) {
        ENGINE_BLK(802A5878);
        end = func_802A5E10(in, req->length, out);
        ENGINE_BLK(802A5880);
    } else {
        ENGINE_BLK(802A5888);
        if (req->type == 1) {
            ENGINE_BLK(802A5894);
            end = func_802A5AE0(in, req->length, out);
            ENGINE_BLK(802A589C);
        } else {
            ENGINE_BLK(802A58A4);
            if (req->type == 2) {
                ENGINE_BLK(802A58B0);
                end = func_802A5B90(in, req->length, out);
                ENGINE_BLK(802A58B8);
            } else {
                ENGINE_BLK(802A58C0);
                if (req->type == 4) {
                    ENGINE_BLK(802A58CC);
                    end = func_802A5C5C(in, req->length, out, req->param);
                    ENGINE_BLK(802A58D4);
                } else {
                    ENGINE_BLK(802A58DC);
                    if (req->type == 5) {
                        ENGINE_BLK(802A58E8);
                        end = func_802A5D34(in, req->length, out, req->param);
                        ENGINE_BLK(802A58F0);
                    } else {
                        ENGINE_BLK(802A58F8);
                        if (req->type == 3) {
                            ENGINE_BLK(802A5904);
                            end = func_802A5A2C(in, req->length, out);
                            ENGINE_BLK(802A590C);
                        } else {
                            ENGINE_BLK(802A5914);
                            if (req->type == 6) {
                                ENGINE_BLK(802A5920);
                                end = func_802A5958(in, req->length, out);
                                ENGINE_BLK(802A5928);
                            }
                        }
                    }
                }
            }
        }
    }
    ENGINE_BLK(802A5930);
    return end - out;
}

/* ---- the decode queue ---------------------------------------------------- */

/* func_802A5720 (the C's): empty it */
void func_802A5720(void) {
    ENGINE_BLK(802A5720);
    D_803C4B50 = D_803C4250;
    D_803C4B54 = D_803C4250;
}

/* func_802A5764 (5BF40's): queue a decode */
REGS(s1, s2, s3, fp)
void func_802A5764(u32 dst, u32 length, u32 type, u32 param) {
    TexDecode *q = D_803C4B50;

    ENGINE_BLK(802A5764);
    q->dst = dst;
    q->length = length;
    q->type = type;
    q->param = param;
    D_803C4B50 = q + 1;
}

/* func_802A57AC (the C's): do the next queued decode, its DMA being in */
void func_802A57AC(void) {
    TexDecode *q = D_803C4B54;

    ENGINE_BLK(802A57AC);
    func_802A57DC(q);
    ENGINE_BLK(802A57C4);
    D_803C4B54 = q + 1;
}
