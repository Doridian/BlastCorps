/*
 * The audio microcode, high level: runs the command lists that the game's
 * libaudio (alAudioFrame, src/libultra/audio) builds for the RSP.
 *
 * This game's libaudio is older than ultralib's and its microcode is the
 * matching old aspMain: sixteen commands (PR/abi.h), a linear envelope
 * (env.c's _getRate is a straight line, not the later exponential), a
 * 2-pole ADPCM decoder, a 4-tap resampler, a one-pole low-pass for the
 * reverb.  Every command below is written from what libaudio asks of it
 * (the buffer arithmetic in load.c, resample.c, env.c, reverb.c, save.c)
 * and from the SDK's description of the formats, not from another HLE.  The
 * resampler's filter table isn't copied either: it is read from the
 * microcode's own data segment (the task's ucode_data, offset 0xD0), which
 * the game has in RDRAM.
 *
 * DMEM is 4 KB of big-endian bytes; the addresses in the commands are
 * relative to the buffer area, which starts at 0x5C0 (below it is the
 * microcode's data, which the resampler's history can reach into).  All the
 * state the microcode keeps in RDRAM between frames (the decoder's last
 * frame, the resampler's history, the envelope, the filter's history) is
 * big-endian too; only the ADPCM loop state is written by the CPU, as the
 * sixteen samples before the loop start.
 */
#include <stdint.h>
#include <string.h>

#include "host.h"

#define DMEM_BASE 0x5C0

enum {
    A_SPNOOP, A_ADPCM, A_CLEARBUFF, A_ENVMIXER, A_LOADBUFF, A_RESAMPLE, A_SAVEBUFF, A_SEGMENT,
    A_SETBUFF, A_SETVOL, A_DMEMMOVE, A_LOADADPCM, A_MIXER, A_INTERLEAVE, A_POLEF, A_SETLOOP,
};
#define F_INIT 0x01
#define F_LOOP 0x02      /* A_ADPCM */
#define F_LEFT 0x02      /* A_SETVOL */
#define F_VOL 0x04
#define F_AUX 0x08

static uint8_t dmem[0x1000];
static int16_t table[256];              /* A_LOADADPCM: codebook or filter */
static uint32_t segments[16];
static uint32_t loop_addr;
static uint16_t in_, out_, count_;      /* A_SETBUFF */
static uint16_t dry_right, wet_left, wet_right;
static int16_t vol[2], target[2], dry, wet;
static int32_t rate[2];
static const uint8_t *resample_lut;     /* 64 x 4 s16, big-endian */
static int16_t taps[64][4];             /* ... in the host's order (audio_task) */

static inline int16_t clamp16(int32_t v) {
    return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v;
}

/* a big-endian half at p, read or written as one access */
static inline int16_t be16_ld(const uint8_t *p) {
    uint16_t v;
    memcpy(&v, p, 2);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    v = __builtin_bswap16(v);
#endif
    return (int16_t)v;
}
static inline void be16_st(uint8_t *p, int16_t v) {
    uint16_t u = (uint16_t)v;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    u = __builtin_bswap16(u);
#endif
    memcpy(p, &u, 2);
}

/* DMEM samples, at byte addresses (as the microcode sees them; only a half
   at 0xFFF wraps between its bytes) */
static inline int16_t ld(uint32_t a) {
    a &= 0xFFF;
    if (a != 0xFFF)
        return be16_ld(dmem + a);
    return (int16_t)(dmem[a] << 8 | dmem[0]);
}
static inline void st(uint32_t a, int16_t v) {
    a &= 0xFFF;
    if (a != 0xFFF) {
        be16_st(dmem + a, v);
        return;
    }
    dmem[a] = (uint8_t)((uint16_t)v >> 8);
    dmem[0] = (uint8_t)v;
}

/* whether bytes [a, a + n) of DMEM run without wrapping (a buffer the loops
   below can walk with pointers) */
static inline int flat(uint32_t a, uint32_t n) {
    return (a & 0xFFF) + n <= 0x1000;
}
/* DMEM bytes: n from s to d, a byte at a time forward (overlapping copies
   repeat, as the microcode's own loop would), wrapping */
static void dmem_copy(uint32_t d, uint32_t s, uint32_t n) {
    d &= 0xFFF, s &= 0xFFF;
    if (flat(d, n) && flat(s, n) && (d + n <= s || s + n <= d)) {
        memcpy(dmem + d, dmem + s, n);
        return;
    }
    for (uint32_t i = 0; i < n; i++)
        dmem[(d + i) & 0xFFF] = dmem[(s + i) & 0xFFF];
}

/* RDRAM, through the segment table (bit 28, a host stack's, kept as in
   gfx.c's seg_to_k0) */
static uint8_t *ram(uint32_t so) {
    uint32_t a = segments[(so >> 24) & 0xF] + (so & 0xFFFFFF) + (so & 0xF0000000u);
    return port_ptr(PORT_RDRAM_BASE | (a & 0x1FFFFFFFu));      /* KSEG0, as gfx.c's */
}
/* game memory at its own widths (the codebooks, the loop state, the state
   the microcode keeps between frames, which only it reads) */
static int16_t ram16(const uint8_t *p, int i) { return (int16_t)port_g16(p + 2 * i); }
static void wram16(uint8_t *p, int i, int16_t v) { port_wg16(p + 2 * i, (uint16_t)v); }
static int32_t ram32(const uint8_t *p, int i) { return (int32_t)port_g32(p + 4 * i); }
static void wram32(uint8_t *p, int i, int32_t v) { port_wg32(p + 4 * i, (uint32_t)v); }

/* ---- the decoder: 9-byte frames of 16 samples, a 2nd-order predictor --- */

static void adpcm(int flags, uint32_t state) {
    uint8_t *st_ram = ram(state);
    int16_t last[16];
    uint32_t in = DMEM_BASE + in_, out = DMEM_BASE + out_;
    if (flags & F_INIT)
        memset(last, 0, sizeof last);
    else {
        const uint8_t *src = (flags & F_LOOP) ? ram(loop_addr) : st_ram;
        for (int i = 0; i < 16; i++)
            last[i] = ram16(src, i);
    }
    /* the previous frame goes first: load.c points the next filter at
       out + 2 * lastsam, inside it */
    for (int i = 0; i < 16; i++, out += 2)
        st(out, last[i]);
    for (int remaining = count_; remaining > 0; remaining -= 32) {
        uint8_t hdr = dmem[in++ & 0xFFF];
        int scale = hdr >> 4;
        const int16_t *book = table + (hdr & 0xF) * 16;   /* [2][8] */
        int32_t res[16];
        for (int i = 0; i < 16; i += 2) {
            uint8_t b = dmem[in++ & 0xFFF];
            res[i] = ((int8_t)b >> 4) * (1 << scale);
            res[i + 1] = ((int8_t)(b << 4) >> 4) * (1 << scale);
        }
        for (int half = 0; half < 2; half++) {
            int32_t *x = res + 8 * half;
            int16_t p2 = last[14], p1 = last[15];
            int16_t y[8];
            for (int i = 0; i < 8; i++) {
                int32_t acc = book[i] * p2 + book[8 + i] * p1 + (x[i] << 11);
                for (int j = 0; j < i; j++)
                    acc += book[8 + i - 1 - j] * x[j];
                y[i] = clamp16(acc >> 11);
            }
            memmove(last, last + 8, 8 * sizeof last[0]);
            memcpy(last + 8, y, sizeof y);
        }
        for (int i = 0; i < 16; i++, out += 2)
            st(out, last[i]);
    }
    for (int i = 0; i < 16; i++)
        wram16(st_ram, i, last[i]);
}

/* ---- the resampler: 4 taps, 64 phases, pitch in UQ1.15 ------------------- */

static void resample(int flags, uint32_t pitch, uint32_t state) {
    uint8_t *s = ram(state);
    uint32_t in = DMEM_BASE + in_ - 8, out = DMEM_BASE + out_;
    uint32_t frac = 0;
    /* the history goes into the four samples in front of the input */
    for (int k = 0; k < 4; k++)
        st(in + 2 * k, (flags & F_INIT) ? 0 : ram16(s, k));
    if (!(flags & F_INIT))
        frac = (uint16_t)ram16(s, 4);
    uint32_t step = pitch << 1;         /* UQ16.16 */
    int n = count_ / 2, i = 0;
    /* the input it reads: up to the last step's four samples */
    uint32_t reach = (uint32_t)(((uint64_t)step * (uint32_t)n + frac) >> 16) * 2 + 8;
    if (flat(in, reach) && flat(out, 2 * (uint32_t)n)) {
        uint8_t *pin = dmem + (in & 0xFFF), *pout = dmem + (out & 0xFFF);
        for (; i < n; i++) {
            const int16_t *c = taps[(frac >> 10) & 63];
            int32_t acc = be16_ld(pin) * c[0] + be16_ld(pin + 2) * c[1] + be16_ld(pin + 4) * c[2] +
                          be16_ld(pin + 6) * c[3];
            be16_st(pout + 2 * i, clamp16(acc >> 15));
            frac += step;
            pin += (frac >> 16) * 2;
            in += (frac >> 16) * 2;
            frac &= 0xFFFF;
        }
    }
    for (; i < n; i++) {
        const int16_t *c = taps[(frac >> 10) & 63];
        int32_t acc = 0;
        for (int k = 0; k < 4; k++)
            acc += ld(in + 2 * k) * c[k];
        st(out + 2 * i, clamp16(acc >> 15));
        frac += step;
        in += (frac >> 16) * 2;
        frac &= 0xFFFF;
    }
    for (int k = 0; k < 4; k++)
        wram16(s, k, ld(in + 2 * k));
    wram16(s, 4, (int16_t)frac);
}

/* ---- the envelope mixer: linear ramps, dry and wet sends ------------------- */

/* the envelope mixer's samples [0, n) where nothing wraps: the volumes
   ramped, each sample into the nout outputs (nout a constant where it is
   called, so the loop over them unrolls) */
static inline __attribute__((always_inline)) void env_walk(int nout, int n, uint32_t in, const uint32_t *dst,
                                                           int64_t *v, const int64_t *tgt, const int32_t *r,
                                                           int16_t d, int16_t w) {
    uint8_t *pin = dmem + (in & 0xFFF), *pd[4];
    int64_t v0 = v[0], v1 = v[1];

    for (int k = 0; k < nout; k++)
        pd[k] = dmem + (dst[k] & 0xFFF);
    for (int i = 0; i < n; i++) {
        int32_t g[4], x, c0 = (int32_t)(v0 >> 19), c1 = (int32_t)(v1 >> 19);

        /* both volumes at rest (no ramp, or at its target, where every
           step lands back on it): the gains are the same for the rest */
        if ((r[0] == 0 || v0 == tgt[0]) && (r[1] == 0 || v1 == tgt[1])) {
            g[0] = (c0 * d) >> 15;
            g[2] = (c0 * w) >> 15;
            g[1] = (c1 * d) >> 15;
            g[3] = (c1 * w) >> 15;
            for (; i < n; i++) {
                x = be16_ld(pin + 2 * i);
                for (int k = 0; k < nout; k++)
                    be16_st(pd[k] + 2 * i, clamp16(be16_ld(pd[k] + 2 * i) + ((x * g[k] + 0x4000) >> 15)));
            }
            break;
        }

        g[0] = (c0 * d) >> 15;
        g[2] = (c0 * w) >> 15;
        g[1] = (c1 * d) >> 15;
        g[3] = (c1 * w) >> 15;
        v0 += r[0];
        if ((r[0] > 0 && v0 > tgt[0]) || (r[0] < 0 && v0 < tgt[0]))
            v0 = tgt[0];
        v1 += r[1];
        if ((r[1] > 0 && v1 > tgt[1]) || (r[1] < 0 && v1 < tgt[1]))
            v1 = tgt[1];
        x = be16_ld(pin + 2 * i);
        for (int k = 0; k < nout; k++)
            be16_st(pd[k] + 2 * i, clamp16(be16_ld(pd[k] + 2 * i) + ((x * g[k] + 0x4000) >> 15)));
    }
    v[0] = v0, v[1] = v1;
}

/* The volumes are 16.16 plus three bits: the rate is the step per eight
   samples (env.c: _getVol adds rate * samples / 8). */
static void envmixer(int flags, uint32_t state) {
    uint8_t *s = ram(state);
    int64_t v[2], tgt[2];
    int32_t r[2];
    int16_t d, w;
    if (flags & F_INIT) {
        for (int c = 0; c < 2; c++) {
            v[c] = (int64_t)vol[c] << 19;
            tgt[c] = (int64_t)target[c] << 19;
            r[c] = rate[c];
        }
        d = dry;
        w = wet;
    } else {
        for (int c = 0; c < 2; c++) {
            v[c] = (int64_t)ram32(s, c) * 8 + (s[28 + c] & 7);
            tgt[c] = (int64_t)ram16(s, 4 + c) << 19;
            r[c] = ram32(s, 4 + c);
        }
        d = ram16(s, 12);
        w = ram16(s, 13);
    }
    uint32_t in = DMEM_BASE + in_;
    uint32_t dst[4] = { DMEM_BASE + out_, DMEM_BASE + dry_right, DMEM_BASE + wet_left, DMEM_BASE + wet_right };
    int nout = (flags & F_AUX) ? 4 : 2;
    int n = count_ / 2, ok = flat(in, 2 * n), i = 0;
    for (int k = 0; k < nout; k++)
        ok &= flat(dst[k], 2 * n);
    if (ok) {
        if (nout == 4)
            env_walk(4, n, in, dst, v, tgt, r, d, w);
        else
            env_walk(2, n, in, dst, v, tgt, r, d, w);
        i = n;
    }
    for (; i < n; i++) {
        int32_t g[4];
        for (int c = 0; c < 2; c++) {
            int32_t cv = (int32_t)(v[c] >> 19);
            g[c] = (cv * d) >> 15;
            g[2 + c] = (cv * w) >> 15;
            v[c] += r[c];
            if ((r[c] > 0 && v[c] > tgt[c]) || (r[c] < 0 && v[c] < tgt[c]))
                v[c] = tgt[c];
        }
        int32_t x = ld(in + 2 * i);
        for (int k = 0; k < nout; k++)
            st(dst[k] + 2 * i, clamp16(ld(dst[k] + 2 * i) + ((x * g[k] + 0x4000) >> 15)));
    }
    /* the state: volumes (16.16, and the three extra bits at 28), targets,
       rates, the dry and wet amounts; 30 of ENVMIX_STATE's 80 bytes */
    for (int c = 0; c < 2; c++) {
        wram32(s, c, (int32_t)(v[c] >> 3));
        s[28 + c] = (uint8_t)(v[c] & 7);
        wram16(s, 4 + c, (int16_t)(tgt[c] >> 19));
        wram32(s, 4 + c, r[c]);
    }
    wram16(s, 12, d);
    wram16(s, 13, w);
}

/* ---- the reverb's low-pass: the table is the impulse response ------------- */

/* table[0..7]: the effect of y[-2] on the eight outputs, table[8..15]: of
   y[-1], which is also the response to an input sample (drvrnew.c's
   _init_lpfilter: fc^1 ... fc^8, Q14, with the gain applied to the input) */
static void polef(int flags, int16_t gain, uint32_t state) {
    uint8_t *s = ram(state);
    int16_t y2 = 0, y1 = 0;
    if (!(flags & F_INIT)) {
        y2 = ram16(s, 2);
        y1 = ram16(s, 3);
    }
    uint32_t in = DMEM_BASE + in_, out = DMEM_BASE + out_;
    int n = (count_ + 15) / 16 * 8;
    for (int b = 0; b < n; b += 8) {
        int32_t gx[8];
        int16_t y[8];
        for (int i = 0; i < 8; i++)
            gx[i] = (ld(in + 2 * (b + i)) * gain) >> 14;
        for (int i = 0; i < 8; i++) {
            int32_t acc = (gx[i] << 14) + table[i] * y2 + table[8 + i] * y1;
            for (int j = 0; j < i; j++)
                acc += table[8 + i - 1 - j] * gx[j];
            y[i] = clamp16(acc >> 14);
        }
        for (int i = 0; i < 8; i++)
            st(out + 2 * (b + i), y[i]);
        y2 = y[6];
        y1 = y[7];
    }
    wram16(s, 0, 0);
    wram16(s, 1, 0);
    wram16(s, 2, y2);
    wram16(s, 3, y1);
}

/* ---- the task ----------------------------------------------------------------- */

static void audio_task(uint32_t data_ptr, uint32_t data_size, uint32_t ucode_data) {
    const uint8_t *cmd = port_ptr(data_ptr);
    resample_lut = (const uint8_t *)port_ptr(ucode_data) + 0xD0;
    for (int k = 0; k < 64 * 4; k++)
        taps[k / 4][k % 4] = (int16_t)port_be16(resample_lut + 2 * k);
    for (uint32_t off = 0; off + 8 <= data_size; off += 8) {
        uint32_t w0 = port_g32(cmd + off), w1 = port_g32(cmd + off + 4);
        int op = w0 >> 24, flags = (w0 >> 16) & 0xFF;
        switch (op) {
        case A_SPNOOP:
            break;
        case A_ADPCM:
            adpcm(flags, w1);
            break;
        case A_CLEARBUFF: {
            uint32_t d = DMEM_BASE + (w0 & 0xFFFF), n = ((w1 & 0xFFFF) + 15) & ~15u;
            if (flat(d, n))
                memset(dmem + (d & 0xFFF), 0, n);
            else
                for (uint32_t i = 0; i < n; i++)
                    dmem[(d + i) & 0xFFF] = 0;
            break;
        }
        case A_ENVMIXER:
            envmixer(flags, w1);
            break;
        case A_LOADBUFF: {
            const uint8_t *src = ram(w1 & ~7u);
            uint32_t d = (DMEM_BASE + in_) & ~7u, n = (count_ + 7) & ~7u;
            if (flat(d, n))
                memcpy(dmem + (d & 0xFFF), src, n);
            else
                for (uint32_t i = 0; i < n; i++)
                    dmem[(d + i) & 0xFFF] = src[i];
            break;
        }
        case A_RESAMPLE:
            resample(flags, w0 & 0xFFFF, w1);
            break;
        case A_SAVEBUFF: {
            uint8_t *dst = ram(w1 & ~7u);
            uint32_t s = (DMEM_BASE + out_) & ~7u, n = (count_ + 7) & ~7u;
            if (flat(s, n))
                memcpy(dst, dmem + (s & 0xFFF), n);
            else
                for (uint32_t i = 0; i < n; i++)
                    dst[i] = dmem[(s + i) & 0xFFF];
            break;
        }
        case A_SEGMENT:
            segments[(w1 >> 24) & 0xF] = w1 & 0x1FFFFFFF;
            break;
        case A_SETBUFF:
            if (flags & F_AUX) {
                dry_right = w0 & 0xFFFF;
                wet_left = w1 >> 16;
                wet_right = w1 & 0xFFFF;
            } else {
                in_ = w0 & 0xFFFF;
                out_ = w1 >> 16;
                count_ = w1 & 0xFFFF;
            }
            break;
        case A_SETVOL:
            if (flags & F_AUX) {
                dry = (int16_t)w0;
                wet = (int16_t)w1;
            } else {
                int c = (flags & F_LEFT) ? 0 : 1;
                if (flags & F_VOL)
                    vol[c] = (int16_t)w0;
                else {
                    target[c] = (int16_t)w0;
                    rate[c] = (int32_t)w1;
                }
            }
            break;
        case A_DMEMMOVE: {
            uint32_t s = DMEM_BASE + (w0 & 0xFFFF), d = DMEM_BASE + (w1 >> 16);
            dmem_copy(d, s, ((w1 & 0xFFFF) + 3) & ~3u);
            break;
        }
        case A_LOADADPCM: {
            const uint8_t *src = ram(w1);
            uint32_t n = (w0 & 0xFFFFFF) / 2;
            for (uint32_t i = 0; i < n && i < 256; i++)
                table[i] = ram16(src, (int)i);
            break;
        }
        case A_MIXER: {
            int16_t gain = (int16_t)w0;
            uint32_t s = DMEM_BASE + (w1 >> 16), d = DMEM_BASE + (w1 & 0xFFFF);
            int n = ((count_ + 31) & ~31) / 2, i = 0;
            if (flat(s, 2 * n) && flat(d, 2 * n)) {
                uint8_t *ps = dmem + (s & 0xFFF), *pd = dmem + (d & 0xFFF);
                for (; i < n; i++)
                    be16_st(pd + 2 * i, clamp16(be16_ld(pd + 2 * i) + ((be16_ld(ps + 2 * i) * gain + 0x4000) >> 15)));
            }
            for (; i < n; i++)
                st(d + 2 * i, clamp16(ld(d + 2 * i) + ((ld(s + 2 * i) * gain + 0x4000) >> 15)));
            break;
        }
        case A_INTERLEAVE: {
            uint32_t l = DMEM_BASE + (w1 >> 16), r = DMEM_BASE + (w1 & 0xFFFF);
            uint32_t d = DMEM_BASE + out_;
            /* the output overlaps nothing it still has to read: libaudio
               interleaves AL_MAIN_L/R_OUT into offset 0 */
            for (int i = 0; i < count_ / 2; i++) {
                int16_t a = ld(l + 2 * i), b = ld(r + 2 * i);
                st(d + 4 * i, a);
                st(d + 4 * i + 2, b);
            }
            break;
        }
        case A_POLEF:
            polef(flags, (int16_t)w0, w1);
            break;
        case A_SETLOOP:
            loop_addr = w1;
            break;
        default:
            if (host_verbose)
                host_log("audio: unknown command %08X %08X\n", w0, w1);
            break;
        }
    }
}

void host_audio_task(uint32_t data_ptr, uint32_t data_size, uint32_t ucode_data) {
    host_perf_push(PERF_AUDIO);
    audio_task(data_ptr, data_size, ucode_data);
    host_perf_pop();
}
