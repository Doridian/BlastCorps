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
        w = (s16)TEX_BE16(*in);
        in++;
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
        w = (s16)TEX_BE16(*in);
        in++;
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
        w = (s16)TEX_BE16(*in);
        in++;
        if (w >= 0) {
            ENGINE_BLK(802A5B18);
            *(u16 *)out = TEX_BE16(((w & 0xFFC0) << 1) | (w & 0x3F));
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
        w = (s16)TEX_BE16(*in);
        in++;
        if (w >= 0) {
            ENGINE_BLK(802A5BC8);
            *(u32 *)out = TEX_BE32(((w & 0x7800) << 17) | ((w & 0x780) << 13) | ((w & 0x78) << 9) |
                                   ((w & 7) << 5));
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
        w = (s16)TEX_BE16(*in);
        in++;
        if (w >= 0) {
            u32 hi = (u32)w >> 8;

            ENGINE_BLK(802A5C94);
            out += 4;
            ((u16 *)out)[-2] = TEX_BE16((TEX_BE16(*(u16 *)(pal + (hi & 0xFE))) << 1) | (hi & 1));
            len -= 2;
            ((u16 *)out)[-1] = TEX_BE16((TEX_BE16(*(u16 *)(pal + (w & 0xFE))) << 1) | (w & 1));
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
        w = (s16)TEX_BE16(*in);
        in++;
        if (w >= 0) {
            u32 c = TEX_BE16(*(u16 *)(pal + (((u32)w >> 4) << 1)));

            ENGINE_BLK(802A5D6C);
            *(u32 *)out = TEX_BE32(((c & 0x7C00) << 17) | ((c & 0x3E0) << 14) | ((c & 0x1F) << 11) |
                                   ((w & 0xF) << 4));
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

/* ---- the effects' sprite slots ------------------------------------------ */

#include "game/game.h"
#include "shared.h"

/* An effect's animated sprite: 16 slots of 0x3C bytes (D_803C4B70), each
   with its frames' texture cells (up to four of the 16 in D_803EB770, 0x100
   bytes each in D_803EA770). */
typedef struct EffectSlot {
    /* 0x00 */ u8 *PTR32 anim;      /* w at 2, h at 3, frames at 0xE, its texture at 0 */
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 pos[3];          /* << 11 */
    /* 0x14 */ s32 unk14[3];
    /* 0x20 */ s32 vel[3];
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ u8 unk30, unk31;
    /* 0x32 */ u8 frame;
    /* 0x33 */ u8 active;
    /* 0x34 */ u8 mode;
    /* 0x35 */ u8 unk35;
    /* 0x36 */ u8 unk36;
    /* 0x37 */ u8 cells[4];
    /* 0x3B */ u8 unk3B;
} EffectSlot;

extern EffectSlot D_803C4B70[16];
extern u8 D_803EA770[16][0x100];
extern u8 D_803EB770[16];       /* the cells in use */
extern u8 *PTR32 D_803EB788, *PTR32 D_803EB78C;    /* the effects' heap */
extern s16 D_803EB790;          /* ... in 0x1010-byte pieces */
extern u8 D_803EB792;
extern u8 D_8020ED00[], D_8021DD00[];
extern char D_80305C34[], D_80305C48[];
void func_8029A7E4(char *, ...);
REGS(t6, s1)
void func_802A11C4(u32 id, u32 dst);

#define R_AT 1
#define R_A3 7
#define HI16(p) (((u32)(p) + 0x8000) & 0xFFFF0000)

/* func_802A5E60 (the carrier's and the comm point's): each sprite back
   at its last frame */
REGS()
void func_802A5E60(void) {
    EffectSlot *s = D_803C4B70;
    s32 n = 16;

    ENGINE_BLK(802A5E60);
    for (;;) {
        ENGINE_BLK(802A5E84);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802A5E8C);
        n--;
        if (s->active != 0) {
            ENGINE_BLK(802A5E9C);
            s->frame = *(u16 *)(s->anim + 0xE) - 1;
        }
        ENGINE_BLK(802A5EAC);
        s++;
    }
    ENGINE_BLK(802A5EB4);
}

/* func_802A5ED0 (the vehicle modules'): how many slots are in use */
REGS(-> t0)
s32 func_802A5ED0(void) {
    EffectSlot *s = D_803C4B70;
    s32 n = 16, used = 0;

    ENGINE_BLK(802A5ED0);
    for (;;) {
        ENGINE_BLK(802A5EF4);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802A5EFC);
        n--;
        if (s->active != 0) {
            ENGINE_BLK(802A5F0C);
            used++;
        }
        ENGINE_BLK(802A5F10);
        s++;
    }
    ENGINE_BLK(802A5F18);
    return used;
}

/* func_802A5F30 (the level loader's): no effects */
REGS()
void func_802A5F30(void) {
    s32 n = 16, i = 0;

    ENGINE_BLK(802A5F30);
    D_803EB788 = NULL;
    D_803EB78C = NULL;
    D_803EB790 = 0;
    for (;;) {
        ENGINE_BLK(802A5F70);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802A5F78);
        D_803C4B70[i].active = 0;
        n--;
        D_803EB770[i] = 0;
        i++;
    }
    ENGINE_BLK(802A5F90);
}

/* func_802A5FA8 (00000.c's): the effects' heap: what is left below the
   allocator's limit (0x8020ED00, or 0x8021DD00 outside mode 0x20), in
   0x1010-byte pieces, each marked free at 0x1000 */
void func_802A5FA8(void) {
    u8 *heap = D_80358070, *limit, *end, *p;
    s32 left;
    u32 n;

    ENGINE_BLK(802A5FA8);
    if (D_80364AA8 & 0x20) {
        ENGINE_BLK(802A5FC8);
        limit = D_8020ED00;
    } else {
        ENGINE_BLK(802A5FD4);
        limit = D_8021DD00;
    }
    ENGINE_BLK(802A5FDC);
    left = limit - heap;
    if (left < 0) {
        ENGINE_BLK(802A5FE8);
        left = 0;
    }
    ENGINE_BLK(802A5FEC);
    n = (u32)left / 0x1010;
    D_803EB788 = heap;
    ENGINE_BLK(802A6014);
    D_803EB790 = n;
    func_8029A7E4(D_80305C34);
    ENGINE_BLK(802A60A4);
    func_8029A7E4(D_80305C48, n);
    ENGINE_BLK(802A61B4);
    end = heap + n * 0x1010;
    D_803EB78C = end;
    D_80358070 = end;
    for (p = heap;; p += 0x1010) {
        ENGINE_BLK(802A6250);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A6258);
        *(u32 *)(p + 0x1000) = 0;
    }
    ENGINE_BLK(802A6264);
}

/* func_802A6274 (everyone's): start an effect's sprite: a free slot and
   anim's w * h free texture cells, its frames' texture DMA'd (802A11C4)
   unless its number is -1.  Mode 1 with a target ($t5) starts it at the
   point $t3/$t4 name (679E0's func_802ABC88), still.  Returns 1, or 0 when
   there is no room. */
REGS(t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, a3 -> t0)
s32 func_802A6274(s32 t0, s32 t1, s32 t2, s32 t3, s32 t4, s32 t5, s32 t6, s32 t7, s32 s0, s32 s1,
                  s32 s2, s32 s3, s32 s4, s32 s5, s32 a3) {
    u8 *anim = (u8 *)t0;
    EffectSlot *s = D_803C4B70;
    u8 *dest = D_803EA770[0];
    s32 n = 16, need, k;
    u8 *cell, *c;

    ENGINE_BLK(802A6274);
    D_803EB792 = a3;
    ENGINE_LEAVE(R_AT, HI16(&D_803EB792));
    for (;;) {
        ENGINE_BLK(802A62B4);
        if (n == 0) {
            goto fail;
        }
        ENGINE_BLK(802A62BC);
        n--;
        if (s->active == 0) {
            break;
        }
        ENGINE_BLK(802A62CC);
        s++;
        dest += 0x100;
    }
    ENGINE_BLK(802A62D8);
    need = anim[2] * anim[3];
    c = s->cells;
    cell = D_803EB770;
    n = 16;
    for (k = 0;; k++) {
        ENGINE_BLK(802A6304);
        if (need == 0) {
            break;
        }
        ENGINE_BLK(802A630C);
        if (n == 0) {
            goto fail;
        }
        ENGINE_BLK(802A6314);
        ENGINE_LEAVE(R_A3, *cell);
        n--;
        if (*cell == 0) {
            ENGINE_BLK(802A6324);
            *c++ = k;
            need--;
        }
        ENGINE_BLK(802A6330);
        cell++;
    }
    ENGINE_BLK(802A633C);
    need = anim[2] * anim[3];
    for (c = s->cells;;) {
        ENGINE_BLK(802A6364);
        if (need == 0) {
            break;
        }
        ENGINE_BLK(802A636C);
        need--;
        D_803EB770[*c++] = 1;
    }
    ENGINE_BLK(802A6384);
    s->active = 1;
    s->anim = anim;
    s->unk4 = t1;
    s->mode = t2;
    s->frame = 0;
    s->unk35 = s5;
    s->unk3B = D_803EB792;
    ENGINE_LEAVE(R_AT, 1);
    if (t2 != 1) {
        ENGINE_BLK(802A63B4);
        s->pos[0] = t3;
        s->pos[1] = t4;
        s->pos[2] = t5;
        s->vel[0] = t6;
        s->vel[1] = t7;
        s->vel[2] = s0;
        s->unk14[0] = s1;
        s->unk14[1] = s2;
        s->unk14[2] = s3;
        s->unk2C = s4;
        s->unk36 = 0;
    } else {
        ENGINE_BLK(802A63E4);
        if (t5 != 0) {
            s32 *pt;

            ENGINE_BLK(802A63EC);
            pt = (s32 *)func_802ABC88(t3, t4);
            ENGINE_BLK(802A63FC);
            s->mode = 0;
            t3 = pt[0] << 11;
            t4 = pt[1] << 11;
            t5 = pt[2] << 11;
            s->pos[0] = t3;
            s->pos[1] = t4;
            s->pos[2] = t5;
            s->vel[0] = s->vel[1] = s->vel[2] = 0;
            s->unk14[0] = s->unk14[1] = s->unk14[2] = 0;
            s->unk2C = 0xFC180000;
            s->unk36 = 0;
            ENGINE_LEAVE(10, 0xFC180000);
            ENGINE_LEAVE(11, t3);
            ENGINE_LEAVE(12, t4);
            ENGINE_LEAVE(13, t5);
        } else {
            ENGINE_BLK(802A6450);
            s->unk30 = t3;
            s->unk31 = t4;
        }
    }
    ENGINE_BLK(802A6458);
    t6 = *(s16 *)anim;
    ENGINE_LEAVE(14, t6);
    ENGINE_LEAVE(R_AT, -1);
    if (t6 != -1) {
        ENGINE_BLK(802A6468);
        ENGINE_LEAVE(17, (u32)dest);
        func_802A11C4(t6, (u32)dest);
    }
    ENGINE_BLK(802A6470);
    ENGINE_BLK(802A647C);
    return 1;
fail:
    ENGINE_BLK(802A6478);
    ENGINE_BLK(802A647C);
    return 0;
}
