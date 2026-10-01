/*
 * The port's libaudio: the reverb on the auxiliary bus.
 *
 * One circular delay line takes the bus's (mono) input each section; the
 * effect is a list of sections, each reading the line at two taps, `in`
 * and `out` samples back.  A section can feed forward (in into out, by
 * ff), back (out into in, by fb), low-pass what it reads at out, and add
 * it to the output (by gain).  The out tap of a section with a chorus
 * moves: a triangle wave of rate mod_step and depth mod_depth, read through
 * the resampler.  The game gives its own parameters (AL_FX_CUSTOM): the
 * section count, the line's length, then eight numbers a section (in, out,
 * fb, ff, gain, chorus rate, chorus depth, filter coefficient).
 */
#include "audio_private.h"

#define PITCH_ONE 0x8000
#define CENTS_PER_LN2 173123.404906676     /* 120000 / ln 2: hundredths of a cent */

static u32 phys(void *p) {
    return osVirtualToPhysical(p);
}

/* the one-pole low-pass, given to the microcode's pole filter as an ADPCM
   book: its impulse response's first eight samples (fc^1..fc^8, from the
   ninth coefficient), the gain 1 - fc */
static void lowpass_init(LowPass *lp) {
    s16 c = (lp->fc * 16384) >> 15;
    f64 base, power;
    s32 i;

    lp->gain = 16384 - c;
    lp->first = 1;
    for (i = 0; i < 8; i++)
        lp->coef[i] = 0;
    lp->coef[8] = c;
    base = power = (f64)c / 16384;
    for (i = 9; i < 16; i++) {
        power *= base;
        lp->coef[i] = (s16)(power * 16384);
    }
}

void al_reverb_init(Reverb *r, ALSynConfig *c, ALHeap *hp) {
    static const s32 none[2] = { 0, 0 };
    const s32 *p = c->fxType == AL_FX_CUSTOM ? c->params : none;
    u32 i;

    r->on = 1;
    r->nsections = p[0];
    r->len = p[1];
    p += 2;
    r->sections = alHeapAlloc(hp, r->nsections, BLOCK_DELAY);
    r->line = alHeapAlloc(hp, r->len, sizeof(s16));
    r->pos = r->line;
    for (i = 0; i < r->len; i++)
        r->line[i] = 0;

    for (i = 0; i < r->nsections; i++, p += 8) {
        Section *s = &r->sections[i];
        s->in = p[0];
        s->out = p[1];
        s->fb = p[2];
        s->ff = p[3];
        s->gain = p[4];
        if (p[5] != 0) {
            s->mod_step = (f32)p[5] / 0xFFFFFF;
            s->mod_depth = (f32)((f32)p[6] / CENTS_PER_LN2 * (s->out - s->in));
            s->mod_phase = 1.0f;
            s->drift = 0;
            s->chorus = alHeapAlloc(hp, 1, BLOCK_RESAMPLER);
            s->chorus->state = alHeapAlloc(hp, 1, sizeof(RESAMPLE_STATE));
            s->chorus->frac = 0.0f;
            s->chorus->first = 1;
        } else {
            s->chorus = NULL;
        }
        if (p[7] != 0) {
            s->lp = alHeapAlloc(hp, 1, BLOCK_LOWPASS);
            s->lp->state = alHeapAlloc(hp, 1, sizeof(POLEF_STATE));
            s->lp->fc = p[7];
            lowpass_init(s->lp);
        } else {
            s->lp = NULL;
        }
    }
}

/* ---- the delay line ------------------------------------------------------------------- */

static s16 *wrap_up(Reverb *r, s16 *at) {
    return at < r->line ? at + r->len : at;
}

/* n samples of the line from at into DMEM at dmem (in two pieces where
   they wrap) */
static Acmd *line_load(Reverb *r, s16 *at, s32 dmem, s32 n, Acmd *cmd) {
    s16 *end = &r->line[r->len];

    at = wrap_up(r, at);
    if (at + n > end) {
        s32 first = end - at;
        aSetBuffer(cmd++, 0, dmem, 0, first << 1);
        aLoadBuffer(cmd++, phys(at));
        aSetBuffer(cmd++, 0, dmem + (first << 1), 0, (n - first) << 1);
        aLoadBuffer(cmd++, phys(r->line));
    } else {
        aSetBuffer(cmd++, 0, dmem, 0, n << 1);
        aLoadBuffer(cmd++, phys(at));
    }
    aSetBuffer(cmd++, 0, 0, 0, n << 1);
    return cmd;
}

static Acmd *line_save(Reverb *r, s16 *at, s32 dmem, s32 n, Acmd *cmd) {
    s16 *end = &r->line[r->len];

    at = wrap_up(r, at);
    if (at + n > end) {
        s32 first = end - at;
        aSetBuffer(cmd++, 0, 0, dmem, first << 1);
        aSaveBuffer(cmd++, phys(at));
        aSetBuffer(cmd++, 0, 0, dmem + (first << 1), (n - first) << 1);
        aSaveBuffer(cmd++, phys(r->line));
        aSetBuffer(cmd++, 0, 0, 0, n << 1);
    } else {
        aSetBuffer(cmd++, 0, 0, dmem, n << 1);
        aSaveBuffer(cmd++, phys(at));
    }
    return cmd;
}

/* ---- a section's out tap --------------------------------------------------------------- */

/* the chorus's offset now, in samples: a triangle between -depth and
   +depth, from a sawtooth over [-2, 2] */
static f32 chorus_offset(Section *s, s32 n) {
    f32 v;

    s->mod_phase += s->mod_step * n;
    s->mod_phase = s->mod_phase > 2.0 ? (f32)(s->mod_phase - 4.0) : s->mod_phase;
    v = s->mod_phase < 0 ? -s->mod_phase : s->mod_phase;
    v = (f32)(v - 1.0);
    return s->mod_depth * v;
}

static Acmd *read_out_tap(Reverb *r, Section *s, s32 dmem, s32 n, Acmd *cmd) {
    if (s->chorus != NULL) {
        Chorus *ch = s->chorus;
        s32 len = s->out - s->in;
        f32 d = chorus_offset(s, n), ratio, want;
        s32 nin, skew;
        s16 *at;

        /* as a pitch about 1, at the resampler's resolution */
        d /= len;
        d = (s32)(d * PITCH_ONE);
        d = d / PITCH_ONE;
        ratio = (f32)(1.0 - d);
        want = ch->frac + ratio * (f32)n;
        nin = (s32)want;
        ch->frac = want - (f32)nin;

        at = r->pos - (s32)(s->out - s->drift);
        skew = ((s32)at & 7) >> 1;      /* samples before it to an 8-byte boundary */
        cmd = line_load(r, at - skew, DMEM_TEMP2, nin + skew, cmd);
        aSetBuffer(cmd++, 0, DMEM_TEMP2 + (skew << 1), dmem, n << 1);
        aResample(cmd++, ch->first, (s32)(ratio * PITCH_ONE), phys(ch->state));
        ch->first = 0;
        s->drift += nin - n;
        return cmd;
    }
    return line_load(r, r->pos - (s32)s->out, dmem, n, cmd);
}

static Acmd *lowpass(LowPass *lp, s32 dmem, s32 n, Acmd *cmd) {
    aSetBuffer(cmd++, 0, dmem, dmem, n << 1);
    aLoadADPCM(cmd++, 32, phys(lp->coef));
    aPoleFilter(cmd++, lp->first, lp->gain, phys(lp->state));
    lp->first = 0;
    return cmd;
}

/* ---- a section of output: the aux bus (mixed to mono) in, the effect out
   in both its sides ---- */

Acmd *al_reverb_run(Reverb *r, s32 n, Acmd *cmd) {
    s32 in_buf = DMEM_TEMP0, out_buf = DMEM_TEMP1;
    const s32 mono = DMEM_AUX_L, wet = DMEM_AUX_R;
    s16 *last_out = NULL;
    s32 i;

    aSetBuffer(cmd++, 0, 0, 0, n << 1);
    aMix(cmd++, 0, 0xDA83, DMEM_AUX_L, mono);      /* L * 0.707 */
    aMix(cmd++, 0, 0x5A82, DMEM_AUX_R, mono);      /* + R * 0.707 */
    cmd = line_save(r, r->pos, mono, n, cmd);
    aClearBuffer(cmd++, wet, n << 1);

    for (i = 0; i < r->nsections; i++) {
        Section *s = &r->sections[i];
        s16 *in_at = r->pos - (s32)s->in;
        s16 *out_at = r->pos - (s32)s->out;

        /* the in tap is where the last section's out tap was: it's in DMEM
           already (as the reference compares them: with that tap ahead of
           the write position) */
        if (in_at == last_out) {
            s32 t = in_buf;
            in_buf = out_buf;
            out_buf = t;
        } else {
            cmd = line_load(r, in_at, in_buf, n, cmd);
        }
        cmd = read_out_tap(r, s, out_buf, n, cmd);

        if (s->ff != 0) {
            aMix(cmd++, 0, (u16)s->ff, in_buf, out_buf);
            if (s->chorus == NULL && s->lp == NULL)
                cmd = line_save(r, out_at, out_buf, n, cmd);
        }
        if (s->fb != 0) {
            aMix(cmd++, 0, (u16)s->fb, out_buf, in_buf);
            cmd = line_save(r, in_at, in_buf, n, cmd);
        }
        if (s->lp != NULL)
            cmd = lowpass(s->lp, out_buf, n, cmd);
        if (s->chorus == NULL)
            cmd = line_save(r, out_at, out_buf, n, cmd);
        if (s->gain != 0)
            aMix(cmd++, 0, (u16)s->gain, out_buf, wet);
        last_out = r->pos + s->out;
    }

    r->pos += n;
    if (r->pos > &r->line[r->len])
        r->pos -= r->len;

    aDMEMMove(cmd++, wet, DMEM_AUX_L, n << 1);
    return cmd;
}
