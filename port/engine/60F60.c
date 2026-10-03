/*
 * hd_code 60F60 (us.v11 0x802A5720-0x802A6F00): texture decoding and the
 * queue of decodes waiting for their DMAs, as native C (engine.h), and the
 * effects' animated sprites.
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
    {
        u32 size = func_802A57DC(q);

        host_tex_decoded_slot(q - D_803C4250, q->dst, size);
    }
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
   there is no room.  What it leaves in $t2, $t4, $t6 and $s1 the vehicle
   modules read afterwards (62740's func_802A8768, the next effect's
   velocity): ENGINE_LEAVE, until those readers take values instead. */
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
            ENGINE_LEAVE(12, t4);
        } else {
            ENGINE_BLK(802A6450);
            s->unk30 = t3;
            s->unk31 = t4;
        }
    }
    ENGINE_BLK(802A6458);
    t6 = *(s16 *)anim;
    ENGINE_LEAVE(14, t6);
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

/* ---- the effects' sprite drawing ---------------------------------------- */

/* The effects' heap (D_803EB788..D_803EB78C): 0x1010-byte pieces, a frame's
   texture in the first 0x1000, then which animation (0 when free), its
   age and its frame. */
#define PIECE_KEY(p) (*(u32 *)((u8 *)(p) + 0x1000))
#define PIECE_AGE(p) (*(u16 *)((u8 *)(p) + 0x1004))
#define PIECE_FRAME(p) (*(u8 *)((u8 *)(p) + 0x1006))
#define PIECE_NEXT(p) ((u8 *)(p) + 0x1010)

extern u32 D_80305C10[];        /* the animations kept in the heap's pieces, 0 at the end */
extern Gfx *PTR32 D_803EB780, *PTR32 D_803EB784;   /* the effects' two display lists */
Gfx *func_80257540(Gfx *gfx);
Gfx *func_802575F4(Gfx *gfx, s32 arg1, s32 arg2, s16 arg3, s32 arg4, s32 arg5, s32 arg6);
REGS(t6, s1, fp)
void func_802A1074(u32 id, u32 dst, u32 param);

/* func_802A6748: the pieces a frame older; those 60 frames old are free
   again */
REGS()
void func_802A6748(void) {
    u8 *p = D_803EB788, *end = D_803EB78C;
    s32 freed = 0;

    ENGINE_BLK(802A6748);
    for (;;) {
        u32 age;

        ENGINE_BLK(802A6764);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A676C);
        if (PIECE_KEY(p) == 0) {
            p = PIECE_NEXT(p);
            continue;
        }
        ENGINE_BLK(802A6778);
        age = PIECE_AGE(p) + 1;
        PIECE_AGE(p) = age;
        if ((s32)age < 0x3D) {
            ENGINE_BLK(802A678C);
            p = PIECE_NEXT(p);
            continue;
        }
        ENGINE_BLK(802A6794);
        freed++;
        PIECE_KEY(p) = 0;
        p = PIECE_NEXT(p);
    }
    ENGINE_BLK(802A67A4);
    D_803EB790 += freed;
}

/* func_802A67C4: where slot `s`'s current frame is: a piece already
   holding it, or (an animation kept that way, while there are free
   pieces) a free one, or the slot's own texture cell (`cells` + the cell
   number at `cell` << 12); in the last two the frame's texture (the next
   number at $t9) is loaded there.  Returns it ($s1), $t9 advanced and $t6
   the number, as the original leaves them. */
REGS(a0, t3, t4, t9, t6, fp -> s1, t9, t6)
u32 func_802A67C4(u32 cell, u32 cells, u32 s_, u32 t9, u32 t6, u32 fp, u32 *t9_out, u32 *t6_out) {
    EffectSlot *s = (EffectSlot *)s_;
    u8 *p = D_803EB788, *end = D_803EB78C;
    u32 frame = s->frame;
    u32 key = (u32)s->anim;
    u32 dst;

    ENGINE_BLK(802A67C4);
    *t9_out = t9;
    *t6_out = t6;
    for (;;) {
        ENGINE_BLK(802A67F8);
        if (p == end) {
            break;
        }
        ENGINE_BLK(802A6800);
        if (PIECE_KEY(p) == 0) {
            p = PIECE_NEXT(p);
            continue;
        }
        ENGINE_BLK(802A680C);
        if (key != PIECE_KEY(p)) {
            p = PIECE_NEXT(p);
            continue;
        }
        ENGINE_BLK(802A6814);
        if (frame != PIECE_FRAME(p)) {
            p = PIECE_NEXT(p);
            continue;
        }
        ENGINE_BLK(802A6820);
        PIECE_AGE(p) = 0;
        ENGINE_BLK(802A68B4);
        return (u32)p;
    }
    ENGINE_BLK(802A682C);
    if (D_803EB790 != 0) {
        u32 *k;

        ENGINE_BLK(802A683C);
        for (k = D_80305C10;; k++) {
            ENGINE_BLK(802A6844);
            if (*k == key) {
                ENGINE_BLK(802A6860);
                D_803EB790--;
                for (p = D_803EB788;; p = PIECE_NEXT(p)) {
                    ENGINE_BLK(802A6874);
                    if (PIECE_KEY(p) == 0) {
                        break;
                    }
                    ENGINE_BLK(802A6880);
                }
                ENGINE_BLK(802A6888);
                PIECE_KEY(p) = key;
                PIECE_FRAME(p) = frame;
                PIECE_AGE(p) = 0;
                dst = (u32)p;
                goto load;
            }
            ENGINE_BLK(802A6850);
            if (*k == 0) {
                break;
            }
            ENGINE_BLK(802A6858);
        }
    }
    ENGINE_BLK(802A689C);
    dst = cells + (*(u8 *)cell << 12);
load:
    ENGINE_BLK(802A68A8);
    t6 = *(u16 *)t9;
    *t9_out = t9 + 2;
    *t6_out = t6;
    func_802A1074(t6, dst, fp);
    ENGINE_BLK(802A68B4);
    return dst;
}

/* func_802A6C10: the four corners of a sprite at (x, y) on screen, w by h
   (the texture coordinates w << 5, h << 5), coloured by bytes 6..9 of
   `rec` */
REGS(t1, t7, s2, s3, v1, s5)
void func_802A6C10(u32 vtx_, u32 rec_, s32 x, s32 y, s32 w, s32 h) {
    s16 *v = (s16 *)vtx_;
    u8 *rec = (u8 *)rec_;
    s32 x1 = x - w, y1 = y + h;
    s32 sw = w << 5, sh = h << 5;
    s32 i;

    ENGINE_BLK(802A6C10);
    v[0x00] = x;  v[0x01] = y;  v[0x02] = 0; v[0x03] = 0; v[0x04] = 0;  v[0x05] = 0;
    v[0x08] = x1; v[0x09] = y;  v[0x0A] = 0; v[0x0B] = 0; v[0x0C] = sw; v[0x0D] = 0;
    v[0x10] = x;  v[0x11] = y1; v[0x12] = 0; v[0x13] = 0; v[0x14] = 0;  v[0x15] = sh;
    v[0x18] = x1; v[0x19] = y1; v[0x1A] = 0; v[0x1B] = 0; v[0x1C] = sw; v[0x1D] = sh;
    for (i = 0; i < 4; i++) {
        u8 *c = (u8 *)vtx_ + i * 0x10 + 0xC;

        c[0] = rec[6];
        c[1] = rec[7];
        c[2] = rec[8];
        c[3] = rec[9];
    }
}

/* func_802A6D34: the effects' two display lists begun (func_80257540) */
REGS()
void func_802A6D34(void) {
    ENGINE_BLK(802A6D34);
    D_803EB780 = func_80257540(D_803EB780);
    ENGINE_BLK(802A6D84);
    D_803EB784 = func_80257540(D_803EB784);
    ENGINE_BLK(802A6D98);
}

/* func_802A6EB8: slot `s`'s display list: the second (D_803EB784) if its
   0x3B is set */
REGS(t4 -> t2)
u32 func_802A6EB8(u32 s_) {
    EffectSlot *s = (EffectSlot *)s_;

    ENGINE_BLK(802A6EB8);
    if (s->unk3B != 0) {
        ENGINE_BLK(802A6ED0);
        ENGINE_BLK(802A6EE4);
        return (u32)&D_803EB784;
    }
    ENGINE_BLK(802A6EDC);
    ENGINE_BLK(802A6EE4);
    return (u32)&D_803EB780;
}

/* func_802A6DE8: slot `s`'s sprite drawn into its display list
   (12D80.c's func_802575F4).  Leaves $s0 the slot (the vehicle modules'
   next effect and func_8029C454 read it). */
REGS(t4, t1, s1, t8, v1, s5, gp)
void func_802A6DE8(u32 s, s32 a1, s32 a2, s32 a3, s32 sp10, s32 sp14, s32 sp18) {
    Gfx *PTR32 *dl;
    Gfx *g;

    ENGINE_BLK(802A6DE8);
    ENGINE_LEAVE(16, s);
    dl = (Gfx *PTR32 *)func_802A6EB8(s);
    ENGINE_BLK(802A6E3C);
    g = func_802575F4(*dl, a1, a2, a3, sp10, sp14, sp18);
    ENGINE_BLK(802A6E5C);
    dl = (Gfx *PTR32 *)func_802A6EB8(s);
    ENGINE_BLK(802A6E64);
    *dl = g;
}

/* func_802A68D4: slot `s`'s sprite matrix at `m`: loaded by its display
   list (G_MTX), scaled by unk4, turned to face the camera (pitch from the
   height difference over the distance, yaw the camera's), and moved to the
   sprite's position, which moves on by its velocity (and, falling, slows
   at the ground: unk2C) unless it follows a point (mode 1).  Every
   register is kept but the floats its callees leave. */
extern s32 D_803643F8, D_803643FC, D_80364400;     /* the camera, << 16 */
extern u16 D_80364452;                              /* its yaw */
extern s32 D_803C4F30[16];                          /* a matrix to build in */
REGS(v1 -> fp)
s32 func_802AD7FC(u32 x);

REGS(t4, t0)
void func_802A68D4(u32 s_, u32 m_) {
    EffectSlot *s = (EffectSlot *)s_;
    s32 *m = (s32 *)m_;
    Gfx *PTR32 *dl;
    Gfx *g;
    s32 x, y, z, scale, step, cx, cy, cz, angle, pitch;
    s64 dist, q, num;

    ENGINE_BLK(802A68D4);
    dl = (Gfx *PTR32 *)func_802A6EB8(s_);
    ENGINE_BLK(802A6958);
    g = *dl;
    ((u32 *)g)[1] = m_ & 0x1FFFFFFF;
    ((u32 *)g)[0] = 0x01040040;
    *dl = g + 1;
    scale = s->unk4;
    func_802ACC68(scale, scale, scale, m);
    ENGINE_BLK(802A699C);
    if (s->mode != 1) {
        ENGINE_BLK(802A69AC);
        step = s->unk36;
        if (s->unk2C >= s->pos[1]) {
            s32 v;

            ENGINE_BLK(802A69C4);
            v = s->vel[1] + s->unk14[1] * step;
            if (v < 0) {
                ENGINE_BLK(802A69E0);
                v = -v;
            }
            ENGINE_BLK(802A69E4);
            s->vel[1] = v - (v >> 2);
            step = 0;
        }
        ENGINE_BLK(802A69F4);
        s->unk36 = step + 1;
        s->pos[0] = s->pos[0] + s->unk14[0] * step + s->vel[0];
        x = s->pos[0] >> 11;
        s->pos[1] = s->pos[1] + s->unk14[1] * step + s->vel[1];
        y = s->pos[1] >> 11;
        s->pos[2] = s->pos[2] + s->unk14[2] * step + s->vel[2];
        z = s->pos[2] >> 11;
    } else {
        s32 *pt;

        ENGINE_BLK(802A6A68);
        pt = (s32 *)func_802ABC88(s->unk30, s->unk31);
        ENGINE_BLK(802A6A74);
        x = pt[0];
        y = pt[1];
        z = pt[2];
    }
    ENGINE_BLK(802A6A80);
    cx = (u32)D_803643F8 >> 11;
    cy = (u32)D_803643FC >> 11;
    cz = (u32)D_80364400 >> 11;
    dist = func_802ABCDC(cx, cy, cz, x, y, z);
    ENGINE_BLK(802A6AB4);
    if (dist == 0) {
        ENGINE_BLK(802A6ABC);
        dist = 1;
    }
    ENGINE_BLK(802A6AC0);
    dist = (s32)((u32)dist << 11);
    num = (s64)(s32)(cy - y) << 27;
    ENGINE_BLK(802A6AE0);
    if (dist == -1) {
        ENGINE_BLK(802A6AEC);
    }
    q = num / dist;
    ENGINE_BLK(802A6AFC);
    if (q < 0) {
        ENGINE_BLK(802A6B08);
    }
    ENGINE_BLK(802A6B0C);
    angle = func_802AD7FC((u32)(q < 0 ? -(s32)q : (s32)q));
    ENGINE_BLK(802A6B14);
    pitch = angle >> 4;
    if (q >= 0) {
        ENGINE_BLK(802A6B1C);
        pitch = 0xFFF - pitch;
    }
    ENGINE_BLK(802A6B24);
    func_802ACBDC(pitch, D_803C4F30);
    ENGINE_BLK(802A6B30);
    func_802ACCCC(D_803C4F30, m);
    ENGINE_BLK(802A6B3C);
    func_802ACAC4(D_80364452, D_803C4F30);
    ENGINE_BLK(802A6B54);
    func_802ACCCC(D_803C4F30, m);
    ENGINE_BLK(802A6B60);
    func_802ACA60(x << 11, y << 11, z << 11, D_803C4F30);
    ENGINE_BLK(802A6B78);
    func_802ACCCC(D_803C4F30, m);
    ENGINE_BLK(802A6B84);
    func_802AC8CC((u32 *)m);
    ENGINE_BLK(802A6B8C);
}

/* func_802A64A4 (00000.c's, every frame): the effects' sprites drawn: for
   each slot in use its matrix, then a quad per texture cell (w x h of
   them) with the current frame's textures, then the frame advanced (the
   slot freed after the last); the buffers alternate with D_8035805C. */
extern u8 D_8035805C;
extern u8 D_803C4F70[], D_803C5370[], D_803C5770[], D_803C6370[], D_803C6F70[], D_803C7B70[];
extern u8 D_803C8770[], D_803C9770[], D_803CA770[], D_803DA770[];

void func_802A64A4(void) {
    u8 *mtx, *vtx, *cells;
    EffectSlot *s;
    u8 *fp;
    s32 n;
    u32 t6;

    ENGINE_BLK(802A64A4);
    if (D_8035805C != 0) {
        ENGINE_BLK(802A64E8);
        D_803EB780 = (Gfx *)D_803C6370;
        D_803EB784 = (Gfx *)D_803C7B70;
        mtx = D_803C5370;
        vtx = D_803C9770;
        cells = D_803DA770;
    } else {
        ENGINE_BLK(802A6524);
        D_803EB780 = (Gfx *)D_803C5770;
        D_803EB784 = (Gfx *)D_803C6F70;
        mtx = D_803C4F70;
        vtx = D_803C8770;
        cells = D_803CA770;
    }
    ENGINE_BLK(802A655C);
    s = D_803C4B70;
    fp = D_803EA770[0];
    func_802A6D34();
    for (n = 16;; s++, fp += 0x100) {
        u8 *anim, *cell;
        u32 stream, w, h, cw, rows, cols, frames;
        s32 x, y, x0;

        ENGINE_BLK(802A6574);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802A657C);
        n--;
        t6 = s->active;
        if (t6 == 0) {
            ENGINE_BLK(802A66D8);
            continue;
        }
        ENGINE_BLK(802A658C);
        func_802A68D4((u32)s, (u32)mtx);
        ENGINE_BLK(802A6594);
        anim = s->anim;
        cell = s->cells;
        rows = anim[3];
        h = *(u16 *)(anim + 0xC);
        w = *(u16 *)(anim + 0xA);
        stream = (u32)anim + 0x10 + anim[2] * anim[3] * s->frame * 2;
        cw = anim[2];
        x0 = (s32)(cw * w) >> 1;
        y = -((s32)(rows * h) >> 1);
        for (;;) {
            ENGINE_BLK(802A6604);
            if (rows == 0) {
                break;
            }
            ENGINE_BLK(802A660C);
            rows--;
            x = x0;
            for (cols = cw;; cols--) {
                u32 dst;

                ENGINE_BLK(802A6618);
                if (cols == 0) {
                    y += h;
                    break;
                }
                ENGINE_BLK(802A6620);
                func_802A6C10((u32)vtx, (u32)anim, x, y, w, h);
                ENGINE_BLK(802A6628);
                dst = func_802A67C4((u32)cell, (u32)cells, (u32)s, stream, t6, (u32)fp, &stream, &t6);
                ENGINE_BLK(802A6630);
                cell++;
                func_802A6DE8((u32)s, (u32)vtx, dst, *(u16 *)(anim + 4), w, h, s->unk35);
                ENGINE_BLK(802A6638);
                vtx += 0x40;
                x -= w;
            }
        }
        ENGINE_BLK(802A6644);
        mtx += 0x40;
        {
            Gfx *PTR32 *dl = (Gfx *PTR32 *)func_802A6EB8((u32)s);
            u32 *g = (u32 *)*dl;

            ENGINE_BLK(802A664C);
            g[0] = 0xBD000000;      /* G_POPMTX */
            g[1] = 0;
            *dl = (Gfx *)(g + 2);
        }
        frames = *(u16 *)(anim + 0xE);
        t6 = s->frame + 1;
        if (t6 != frames) {
            ENGINE_BLK(802A6684);
            s->frame = t6;
        } else {
            u32 k;

            ENGINE_BLK(802A668C);
            s->active = 0;
            cell = s->cells;
            for (k = anim[2] * anim[3];;) {
                ENGINE_BLK(802A66B8);
                if (k == 0) {
                    break;
                }
                ENGINE_BLK(802A66C0);
                k--;
                D_803EB770[*cell++] = 0;
            }
        }
        ENGINE_BLK(802A66D8);
    }
    ENGINE_BLK(802A66E4);
    ((u32 *)D_803EB780)[0] = 0xB8000000;    /* G_ENDDL */
    ((u32 *)D_803EB780)[1] = 0;
    ((u32 *)D_803EB784)[0] = 0xB8000000;
    ((u32 *)D_803EB784)[1] = 0;
    func_802A6748();
    ENGINE_BLK(802A6714);
}
