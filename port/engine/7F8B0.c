/*
 * hd_code 7F8B0 (us.v11 0x802C4070-0x802C489C): Rare's LZSS, the gzip call
 * the model loaders use, and the player vehicle's engine sound, as native C
 * (engine.h).
 *
 * The LZSS is Mark Nelson's ("The Data Compression Book", 1991) with
 * BREAK_EVEN 2, read most significant bit first: a 1 bit and eight bits is
 * a literal; a 0 bit, an `index_bits` window position and a
 * `16 - index_bits` length is a match of length + 3 bytes; position 0 ends
 * the stream, whose end is then rounded up to even.  The window starts
 * empty at position 1 (tools/assetlib/lzss.py has the encoder).
 */
#include "engine.h"
#include "game/audio.h"
#include "game/vehicle.h"

void func_8025C230(u8 *PTR32 *src, u8 *PTR32 *dst, void *arg2);
SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80260AB8(SndState *state, s16 type, s32 param);

extern u8 *PTR32 D_803F7830;    /* the gzip call's source and destination */
extern u8 *PTR32 D_803F7834;
extern s16 D_803F7840;          /* the engine sound's last speed */
extern SndState *PTR32 D_803F7844;      /* the engine sound */
extern SndState *PTR32 D_803F7848;      /* the second one, while D_80370C1A/B */
extern u8 D_803F784C;           /* whether that one is wanted */
extern s32 D_80364AB0_word __asm__("D_80364AB0");   /* set: restart the engine's pitch */
extern u8 D_80370C1A;
extern u8 D_80370C1B;

/* ---- the LZSS ------------------------------------------------------------ */

typedef struct LzssIn {
    u8 *src;
    u32 cur;    /* the byte being read ($t3) */
    u32 mask;   /* its next bit ($t4); 0x80: a new byte is due */
} LzssIn;

/* func_802C42CC: as many bits as `top` (a power of two) is wide */
static u32 lzss_bits(LzssIn *in, u32 top) {
    u32 v = 0;

    do {
        if (in->mask == 0x80) {
            in->cur = *in->src++;
        }
        if (in->cur & in->mask) {
            v |= top;
        }
        in->mask >>= 1;
        top >>= 1;
        if (in->mask == 0) {
            in->mask = 0x80;
        }
    } while (top != 0);
    return v;
}

/* func_802C41C0: inflate *src into *dst through `window` (1 << index_bits
   bytes); both are left at the ends (the input's rounded up to even) */
REGS(a0, a1, a2, a3 -> a0, a1)
u32 func_802C41C0(u32 src_, u32 dst_, u32 window_, s32 index_bits, u32 *dst_end) {
    u8 *dst = (u8 *)dst_;
    u8 *window = (u8 *)window_;
    LzssIn in;
    u32 wmask = (1 << index_bits) - 1;
    u32 pos_top = 1 << (index_bits - 1);
    u32 len_top = 1 << (15 - index_bits);
    u32 wp = 1;

    in.src = (u8 *)src_;
    in.cur = 0;
    in.mask = 0x80;
    for (;;) {
        u32 flag;

        if (in.mask == 0x80) {
            in.cur = *in.src++;
        }
        flag = in.cur & in.mask;
        in.mask >>= 1;
        if (in.mask == 0) {
            in.mask = 0x80;
        }
        if (flag) {
            u8 c;

            c = lzss_bits(&in, 0x80);
            *dst++ = c;
            window[wp] = c;
            wp = (wp + 1) & wmask;
        } else {
            u32 pos, n, k;

            pos = lzss_bits(&in, pos_top);
            if (pos == 0) {
                break;
            }
            n = lzss_bits(&in, len_top) + 2;
            k = 0;
            do {
                u8 c = window[(pos + k) & wmask];

                k++;
                *dst++ = c;
                window[wp] = c;
                wp = (wp + 1) & wmask;
            } while ((s32)n >= (s32)k);
        }
    }
    if ((u32)in.src & 1) {
        in.src++;
    }
    *dst_end = (u32)dst;
    return (u32)in.src;
}

/* func_802C4070 (46C20.c's): inflate *src into *dst and advance both */
void func_802C4070(u8 *PTR32 *src, u8 *PTR32 *dst, void *window, u8 index_bits) {
    u32 end, s;

    s = func_802C41C0((u32)*src, (u32)*dst, (u32)window, index_bits, &end);
    *src = (u8 *)s;
    *dst = (u8 *)end;
}

/* func_802C4108: the model loaders' gzip (func_8025C230): src and dst as
   the inflate leaves them */
REGS(a0, a1, a2 -> a0, a1)
u32 func_802C4108(u32 src, u32 dst, u32 arg2, u32 *dst_end) {
    D_803F7830 = (u8 *)src;
    D_803F7834 = (u8 *)dst;
    func_8025C230(&D_803F7830, &D_803F7834, (void *)arg2);
    *dst_end = (u32)D_803F7834;
    return (u32)D_803F7830;
}

/* ---- the player vehicle's engine sound ----------------------------------- */

/* func_802C4584: the engine's pitch follows |speed| (every register kept) */
REGS(s5)
void func_802C4584(s32 speed) {
    s16 last = D_803F7840;
    f32 scale, pitch;

    if (speed < 0) {
        speed = -speed;
    }
    D_803F7840 = speed;
    if (D_80364AB0_word != 0) {
        D_80364AB0_word = 0;
    } else {
        if (last == speed) {        /* the whole word: a speed past 0x7FFF never is */
            return;
        }
    }
    if (D_80364456 == 1) {
        scale = 0.015f;
    } else {
        scale = 0.03f;
    }
    pitch = 0.5f + (f32)speed * scale;
    func_80260AB8(D_803F7844, 0x10, *(s32 *)&pitch);
}

/* func_802C4310: start the engine sound `id` at rest */
REGS(a1)
void func_802C4310(s32 id) {
    D_803F7840 = -1;
    *(s16 *)&D_803F784C = 0;
    func_80260650(D_80367738, id, &D_803F7844);
    func_802C4584(0);
}

/* func_802C444C: stop both engine sounds */
REGS()
void func_802C444C(void) {
    if (D_803F7844 != NULL) {
        func_802608C8(D_803F7844);
    }
    if (D_803F7848 != NULL) {
        func_802608C8(D_803F7848);
    }
}

/* func_802C4724: start or stop the second sound `id` as D_80370C1A/B ask */
REGS(a1)
void func_802C4724(s32 id) {
    u8 want = D_80370C1A | D_80370C1B;
    SndState *snd = D_803F7848;

    if (D_803F784C != want) {
        D_803F784C = want;
        if (want != 0) {
            if (snd == NULL) {
                func_80260650(D_80367738, id, &D_803F7848);
            }
        } else {
            if (snd != NULL) {
                func_802608C8(snd);
                D_803F7848 = NULL;
            }
        }
    }
}
