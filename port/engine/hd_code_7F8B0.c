/* hd_code 7F8B0: Rare's LZSS, the gzip call the loaders use, and the
   player vehicle's engine sound.

   The LZSS is Mark Nelson's ("The Data Compression Book", 1991) with
   BREAK_EVEN 2, read most significant bit first: a 1 bit and eight bits
   is a literal; a 0 bit, an `index_bits` window position and a
   `16 - index_bits` length is a match of length + 3 bytes; position 0
   ends the stream (tools/assetlib/lzss.py has the encoder). */
#include "engine_b.h"
#include "game/audio.h"
#include "game/vehicle.h"

void func_8025C230(u8 *PTR32 *src, u8 *PTR32 *dst, void *arg2);
SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_802608C8(SndState *state);
void func_80260AB8(SndState *state, s16 type, s32 param);

extern u8 *D_803F7830;          /* the gzip call's source and destination */
extern u8 *D_803F7834;
extern s16 D_803F7840;          /* the engine sound's last speed */
extern SndState *D_803F7844;    /* the engine sound */
extern SndState *D_803F7848;    /* the second engine sound (D_80370C1A/B) */
extern u8 D_803F784C;           /* whether that one is wanted */
extern u8 D_80364AB0[4];        /* a word: set to restart the engine pitch */
extern u8 D_80370C1A;
extern u8 D_80370C1B;

/* ---- LZSS --------------------------------------------------------------- */

typedef struct {
    const u8 *src;
    u32 cur;    /* the byte being read */
    u32 mask;   /* its next bit; 0x80 means a new byte is due */
} LzssIn;

/* func_802C42CC: as many bits as `top` (a power of two) is wide */
static u32 lzss_bits(LzssIn *in, u32 top) {
    u32 v = 0;

    ENG_COST(1);
    do {
        ENG_COST(10);
        if (in->mask == 0x80) {
            in->cur = *in->src++;
            ENG_COST(2);
        }
        if (in->cur & in->mask) {
            v |= top;
            ENG_COST(1);
        }
        in->mask >>= 1;
        top >>= 1;
        if (in->mask == 0) {
            in->mask = 0x80;
            ENG_COST(1);
        }
    } while (top != 0);
    ENG_COST(2);
    return v;
}

/* func_802C41C0: inflate src into dst through `window` (1 << index_bits
   bytes); returns the end of the input, rounded up to even, and of the
   output. */
static void lzss_decode(const u8 **psrc, u8 **pdst, u8 *window, s32 index_bits) {
    LzssIn in;
    u8 *dst = *pdst;
    u32 wmask = (1 << index_bits) - 1;
    u32 pos_top = 1 << (index_bits - 1);
    u32 len_top = 1 << (15 - index_bits);
    u32 wp = 1;

    in.src = *psrc;
    in.cur = 0;
    in.mask = 0x80;
    ENG_COST(15);
    for (;;) {
        u32 flag;

        ENG_COST(8);
        if (in.mask == 0x80) {
            in.cur = *in.src++;
            ENG_COST(2);
        }
        flag = in.cur & in.mask;
        in.mask >>= 1;
        if (in.mask == 0) {
            in.mask = 0x80;
            ENG_COST(1);
        }
        if (flag) {
            u8 c;

            ENG_COST(2);
            c = lzss_bits(&in, 0x80);
            ENG_COST(7);
            *dst++ = c;
            window[wp] = c;
            wp = (wp + 1) & wmask;
        } else {
            u32 pos, n, k;

            ENG_COST(2);
            pos = lzss_bits(&in, pos_top);
            ENG_COST(2);
            if (pos == 0) {
                break;
            }
            ENG_COST(3);
            n = lzss_bits(&in, len_top) + 2;
            ENG_COST(2);
            for (k = 0; k <= n; k++) {
                u8 c = window[(pos + k) & wmask];

                ENG_COST(13);
                *dst++ = c;
                window[wp] = c;
                wp = (wp + 1) & wmask;
            }
            ENG_COST(2);
        }
    }
    ENG_COST(3 + 4);
    if ((__UINTPTR_TYPE__)in.src & 1) {
        in.src++;
        ENG_COST(1);
    }
    *psrc = in.src;
    *pdst = dst;
}

/* The C's entry (46C20.c): inflate *src into *dst and advance both. */
void func_802C4070(u8 *PTR32 *src, u8 *PTR32 *dst, void *window, u8 index_bits) {
    const u8 *s = *src;
    u8 *d = *dst;

    ENG_COST(19);
    lzss_decode(&s, &d, window, index_bits);
    ENG_COST(19);
    *src = (u8 *)s;
    *dst = d;
}

/* func_802C4108, the loaders' gzip: src and dst advanced as the gzip
   driver leaves them.  (The original returns them in $a0/$a1.) */
void eng_gzip(u8 **src, u8 **dst, void *arg2) {
    ENG_COST(25);
    D_803F7830 = *src;
    D_803F7834 = *dst;
    func_8025C230(&D_803F7830, &D_803F7834, arg2);
    ENG_COST(21);
    *src = D_803F7830;
    *dst = D_803F7834;
}

/* the same as func_802C41C0, for the loaders */
void eng_lzss(const u8 **src, u8 **dst, void *window, s32 index_bits) {
    lzss_decode(src, dst, window, index_bits);
}

/* ---- the player vehicle's engine sound ---------------------------------- */

/* func_802C4584: the engine's pitch follows |speed|.  Every register is
   kept; the speed comes in $s5. */
void func_802C4584(s32 speed) {
    s16 last = D_803F7840;
    f32 scale;

    ENG_COST(40 + 2);
    if (speed < 0) {
        speed = -speed;
        ENG_COST(1);
    }
    D_803F7840 = speed;
    if (*(s32 *)D_80364AB0 != 0) {
        ENG_COST(2);
        *(s32 *)D_80364AB0 = 0;
    } else {
        ENG_COST(2);
        if (last == speed) {    /* the whole word: a speed past 0x7FFF never equals it */
            ENG_COST(34);
            return;
        }
        ENG_COST(2);
    }
    ENG_COST(8);
    if (D_80364456 == 1) {
        scale = 0.015f;
        ENG_COST(2);
    } else {
        scale = 0.03f;
        ENG_COST(3);
    }
    {
        f32 pitch = 0.5f + (f32)speed * scale;

        ENG_COST(8);
        func_80260AB8(D_803F7844, 0x10, *(s32 *)&pitch);
    }
    ENG_COST(34);
}

/* func_802C4310: start the engine sound `id` (in $a1) at rest. */
void func_802C4310(s16 id) {
    ENG_COST(43);
    D_803F7840 = -1;
    *(s16 *)&D_803F784C = 0;
    func_80260650(D_80367738, id, &D_803F7844);
    ENG_COST(2);
    func_802C4584(0);
    ENG_COST(34);
}

/* func_802C444C: stop both engine sounds. */
void func_802C444C(void) {
    ENG_COST(36);
    if (D_803F7844 != NULL) {
        ENG_COST(2);
        func_802608C8(D_803F7844);
    }
    ENG_COST(4);
    if (D_803F7848 != NULL) {
        ENG_COST(2);
        func_802608C8(D_803F7848);
    }
    ENG_COST(34);
}

/* func_802C4724: start or stop the second sound `id` (in $a1) as
   D_80370C1A/B ask. */
void func_802C4724(s16 id) {
    u8 want = D_80370C1A | D_80370C1B;
    SndState *snd = D_803F7848;

    ENG_COST(44);
    if (D_803F784C != want) {
        ENG_COST(3);
        D_803F784C = want;
        if (want != 0) {
            ENG_COST(2);
            if (snd == NULL) {
                ENG_COST(3);
                func_80260650(D_80367738, id, &D_803F7848);
                ENG_COST(2);
            }
        } else {
            ENG_COST(2);
            if (snd != NULL) {
                ENG_COST(2);
                func_802608C8(snd);
                ENG_COST(2);
                D_803F7848 = NULL;
            }
        }
    }
    ENG_COST(34);
}
