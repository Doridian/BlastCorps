/*
 * The port's libaudio: a physical voice's chain, which fetches and decodes
 * its wave table, resamples it to its pitch and mixes it into the buses
 * with its volume ramp, pan and effect send; and the updates that change
 * it at their sample.
 *
 * The microcode's envelope moves the volume linearly towards a target, a
 * step (in 1/65536ths, times 8) per 8 samples; the CPU keeps track of where
 * the ramp got to, in floats, to start the next one from there.
 */
#include "audio_private.h"

#define FRAME_SAMPLES 16        /* an ADPCM frame */
#define FRAME_BYTES 9
#define FRAME_SHIFT 4

/* the resampler's pitch: 1.0 is 0x8000, and it can't go past an octave
   (less 0.03 cents) */
#define PITCH_ONE 0x8000
#define PITCH_MAX 1.99996

static u32 phys(void *p) {
    return osVirtualToPhysical(p);
}

void al_voice_init(PVoice *pv, ALDMANew dmanew, ALHeap *hp) {
    pv->dec_state = alHeapAlloc(hp, 1, sizeof(ADPCM_STATE));
    pv->loop_state = alHeapAlloc(hp, 1, sizeof(ADPCM_STATE));
    pv->dma = dmanew(&pv->dma_state);
    pv->table = NULL;
    pv->frac = 0;
    pv->dec_first = 1;
    pv->rom = 0;

    pv->rs_state = alHeapAlloc(hp, 1, sizeof(RESAMPLE_STATE));
    pv->rs_frac = 0.0f;
    pv->rs_first = 1;
    pv->ratio = 1.0f;
    pv->unity = 0;

    pv->env_state = alHeapAlloc(hp, 1, sizeof(ENVMIX_STATE));
    pv->env_first = 1;
    pv->playing = 0;
    pv->volume = 1;
    pv->target_l = pv->target_r = 1;
    pv->cur_l = pv->cur_r = 1;
    pv->dry = pv->wet = 0;
    pv->rate_l = 1;
    pv->rate_l_frac = 0;
    pv->seg_pos = 0;
    pv->seg_len = 0;
    pv->pan = 0;
    pv->updates = pv->updates_tail = NULL;
}

void al_voice_queue(PVoice *pv, Update *u) {
    if (pv->updates_tail != NULL)
        pv->updates_tail->next = u;
    else
        pv->updates = u;
    pv->updates_tail = u;
}

/* ---- the wave table ------------------------------------------------------------- */

static void set_table(PVoice *pv, ALWaveTable *w) {
    pv->table = w;
    pv->rom = (s32)w->base;
    pv->played = 0;
    if (w->type == AL_ADPCM_WAVE) {
        ALADPCMBook *book = w->waveInfo.adpcmWave.book;
        ALADPCMloop *loop = w->waveInfo.adpcmWave.loop;
        w->len = FRAME_BYTES * (w->len / FRAME_BYTES);     /* whole frames */
        pv->book_bytes = 2 * book->order * book->npredictors * ADPCMVSIZE;
        if (loop != NULL) {
            pv->loop_start = loop->start;
            pv->loop_end = loop->end;
            pv->loop_count = loop->count;
            alCopy(loop->state, pv->loop_state, sizeof(ADPCM_STATE));
        } else {
            pv->loop_start = pv->loop_end = pv->loop_count = 0;
        }
    } else if (w->type == AL_RAW16_WAVE) {
        ALRawLoop *loop = w->waveInfo.rawWave.loop;
        if (loop != NULL) {
            pv->loop_start = loop->start;
            pv->loop_end = loop->end;
            pv->loop_count = loop->count;
        } else {
            pv->loop_start = pv->loop_end = pv->loop_count = 0;
        }
    }
}

/* back to stopped, the table from its start */
static void reset(PVoice *pv) {
    pv->env_first = 1;
    pv->playing = 0;
    pv->volume = 1;
    pv->rs_frac = 0.0f;
    pv->rs_first = 1;
    pv->unity = 0;
    pv->frac = 0;
    pv->dec_first = 1;
    pv->played = 0;
    if (pv->table != NULL) {
        ALWaveTable *w = pv->table;
        pv->rom = (s32)w->base;
        if (w->type == AL_ADPCM_WAVE) {
            if (w->waveInfo.adpcmWave.loop != NULL)
                pv->loop_count = w->waveInfo.adpcmWave.loop->count;
        } else if (w->type == AL_RAW16_WAVE) {
            if (w->waveInfo.rawWave.loop != NULL)
                pv->loop_count = w->waveInfo.rawWave.loop->count;
        }
    }
}

static int loop_runs(PVoice *pv) {
    return pv->loop_count != 0;
}

static void loop_done_once(PVoice *pv) {
    if (pv->loop_count != 0xFFFFFFFF && pv->loop_count != 0)
        pv->loop_count--;
}

/* fetch nbytes from the table's next byte into DMEM at dmem, which must
   hold the 8 bytes before them (DMA is 8-byte aligned); returns how far
   into dmem they start */
static s32 fetch(PVoice *pv, s32 nbytes, s32 dmem, Acmd **cmdp) {
    s32 at = pv->dma(pv->rom, nbytes, pv->dma_state);
    s32 skew = at & 7;

    nbytes += skew;
    aSetBuffer((*cmdp)++, 0, dmem, 0, nbytes + 8 - (nbytes & 7));
    aLoadBuffer((*cmdp)++, at - skew);
    return skew;
}

/* decode nsamples (nbytes of frames) of ADPCM into DMEM at out */
static Acmd *decode_frames(PVoice *pv, s32 nsamples, s32 nbytes, s16 out, s16 in, u32 flags,
                           Acmd *cmd) {
    s32 skew = 0;

    if (nbytes > 0)
        skew = fetch(pv, nbytes, in, &cmd);
    if (flags & A_LOOP)
        aSetLoop(cmd++, phys(pv->loop_state));
    aSetBuffer(cmd++, 0, in + skew, out, nsamples << 1);
    aADPCMdec(cmd++, flags, phys(pv->dec_state));
    pv->dec_first = 0;
    return cmd;
}

static s32 frames_for(s32 samples) {
    return (samples + FRAME_SAMPLES - 1) >> FRAME_SHIFT;
}

/* n samples of an ADPCM table: they end up at *out, which moves past the
   leftover samples of the last frame already decoded */
static Acmd *pull_adpcm(PVoice *pv, s16 *out, s32 n, Acmd *cmd) {
    s16 in = DMEM_DECODED_IN;
    s32 want, have, todo, frames, nbytes;

    if (n == 0)
        return cmd;
    aLoadADPCM(cmd++, pv->book_bytes, phys(pv->table->waveInfo.adpcmWave.book->book));

    if ((u32)(n + pv->played) > pv->loop_end && loop_runs(pv)) {
        /* to the loop's end, then from its start as often as it takes */
        s32 end_here, pos;

        want = pv->loop_end - pv->played;
        have = pv->frac ? FRAME_SAMPLES - pv->frac : 0;
        todo = want - have;
        if (todo < 0)
            todo = 0;
        frames = frames_for(todo);
        nbytes = frames * FRAME_BYTES;
        cmd = decode_frames(pv, todo, nbytes, *out, in, pv->dec_first, cmd);
        *out += pv->frac ? pv->frac << 1 : FRAME_SAMPLES << 1;

        pv->frac = pv->loop_start & 15;
        pv->rom = (s32)pv->table->base + FRAME_BYTES * ((s32)(pv->loop_start >> FRAME_SHIFT) + 1);
        pv->played = pv->loop_start;

        end_here = *out;
        while (n > want) {
            n -= want;
            /* the next pass decodes past the last one's frames, then is
               moved down onto its end */
            pos = (end_here + ((frames + 1) << (FRAME_SHIFT + 1))) & ~31;
            end_here += want << 1;
            loop_done_once(pv);
            want = (u32)n < pv->loop_end - pv->loop_start ? (u32)n : pv->loop_end - pv->loop_start;
            todo = want - FRAME_SAMPLES + pv->frac;
            if (todo < 0)
                todo = 0;
            frames = frames_for(todo);
            nbytes = frames * FRAME_BYTES;
            cmd = decode_frames(pv, todo, nbytes, pos, in, pv->dec_first | A_LOOP, cmd);
            aDMEMMove(cmd++, pos + (pv->frac << 1), end_here, want << 1);
        }
        pv->frac = (n + pv->frac) & 15;
        pv->played += n;
        pv->rom += FRAME_BYTES * frames;
        return cmd;
    }

    {
        s32 past, zeros, decoded = 0;

        have = pv->frac ? FRAME_SAMPLES - pv->frac : 0;
        todo = n - have;
        if (todo < 0)
            todo = 0;
        frames = frames_for(todo);
        nbytes = frames * FRAME_BYTES;
        todo = frames << FRAME_SHIFT;

        /* past the table's end it's silence */
        past = pv->rom + nbytes - ((s32)pv->table->base + pv->table->len);
        if (past < 0)
            past = 0;
        zeros = (past / FRAME_BYTES) << FRAME_SHIFT;
        if (zeros > todo + have)
            zeros = todo + have;
        nbytes -= past;

        if (zeros - (zeros & 15) < n) {
            decoded = 1;
            cmd = decode_frames(pv, todo - zeros, nbytes, *out, in, pv->dec_first, cmd);
            *out += pv->frac ? pv->frac << 1 : FRAME_SAMPLES << 1;
            pv->frac = (n + pv->frac) & 15;
            pv->played += n;
        } else {
            pv->frac = 0;
        }
        pv->rom += FRAME_BYTES * frames;
        if (zeros) {
            pv->frac = 0;
            aClearBuffer(cmd++, (decoded ? (have + todo - zeros) << 1 : 0) + *out, zeros << 1);
        }
    }
    return cmd;
}

/* n samples of a 16-bit table: they end up at *out, moved up by the fetch's
   alignment */
static Acmd *pull_raw(PVoice *pv, s16 *out, s32 n, Acmd *cmd) {
    s32 want, nbytes, skew;

    if (n == 0)
        return cmd;

    if ((u32)(n + pv->played) > pv->loop_end && loop_runs(pv)) {
        s32 pos;

        want = pv->loop_end - pv->played;
        skew = 0;
        if (want > 0)
            skew = fetch(pv, want << 1, *out, &cmd);
        *out += skew;
        pv->rom = (s32)pv->table->base + (pv->loop_start << 1);
        pv->played = pv->loop_start;
        pos = *out;
        while (n > want) {
            s32 dmem_skew;
            pos += want << 1;
            n -= want;
            loop_done_once(pv);
            want = (u32)n < pv->loop_end - pv->loop_start ? (u32)n : pv->loop_end - pv->loop_start;
            nbytes = want << 1;
            {
                s32 at = pv->dma(pv->rom, nbytes, pv->dma_state);
                skew = at & 7;
                nbytes += skew;
                dmem_skew = pos & 7 ? 8 - (pos & 7) : 0;
                aSetBuffer(cmd++, 0, pos + dmem_skew, 0, nbytes + 8 - (nbytes & 7));
                aLoadBuffer(cmd++, at - skew);
            }
            if (skew || dmem_skew)
                aDMEMMove(cmd++, pos + skew + dmem_skew, pos, want << 1);
        }
        pv->played += n;
        pv->rom += n << 1;
        return cmd;
    }

    {
        s32 past;

        nbytes = n << 1;
        past = pv->rom + nbytes - ((s32)pv->table->base + pv->table->len);
        if (past < 0)
            past = 0;
        if (past > nbytes)
            past = nbytes;
        if (past < nbytes) {
            skew = 0;
            if (n > 0)
                skew = fetch(pv, nbytes - past, *out, &cmd);
            *out += skew;
            pv->played += n;
        }
        pv->rom += n << 1;
        if (past) {
            s32 from = (n << 1) - past;
            if (from < 0)
                from = 0;
            aClearBuffer(cmd++, from + *out, past);
        }
    }
    return cmd;
}

static Acmd *pull_table(PVoice *pv, s16 *out, s32 n, Acmd *cmd) {
    if (pv->table->type == AL_RAW16_WAVE)
        return pull_raw(pv, out, n, cmd);
    return pull_adpcm(pv, out, n, cmd);
}

/* ---- the resampler: n output samples into DMEM at out ----------------------------- */

static Acmd *resample(PVoice *pv, s16 out, s32 n, Acmd *cmd) {
    s16 in = DMEM_DECODED;
    s32 nin;
    f32 want;

    if (n == 0)
        return cmd;
    if (pv->unity) {
        cmd = pull_table(pv, &in, n, cmd);
        aDMEMMove(cmd++, in, out, n << 1);
        return cmd;
    }
    if (pv->ratio > PITCH_MAX)
        pv->ratio = PITCH_MAX;
    /* to the microcode's resolution */
    pv->ratio = (s32)(pv->ratio * PITCH_ONE);
    pv->ratio = pv->ratio / PITCH_ONE;
    want = pv->rs_frac + pv->ratio * (f32)n;
    nin = (s32)want;
    pv->rs_frac = want - (f32)nin;
    cmd = pull_table(pv, &in, nin, cmd);
    aSetBuffer(cmd++, 0, in, out, n << 1);
    aResample(cmd++, pv->rs_first, (s32)(pv->ratio * PITCH_ONE), phys(pv->rs_state));
    pv->rs_first = 0;
    return cmd;
}

/* ---- the envelope and mixer --------------------------------------------------------- */

/* the step towards tgt over count samples (8 times the per-sample step,
   whole part and 16-bit fraction); with no time, straight there */
static s16 ramp_step(f32 from, f32 tgt, s32 count, u16 *frac) {
    f32 step;
    s16 whole;

    if (count == 0) {
        if (tgt >= from) {
            *frac = 0xFFFF;
            return 0x7FFF;
        }
        *frac = 0;
        return (s16)0x8000;
    }
    step = (f32)(((tgt - from) / (f32)count) * 8.0);
    if (step < 0.0f)
        step = step - 1.0f;
    whole = (s16)(s32)step;
    *frac = (s16)(s32)(0xFFFF * (step - (f32)whole));
    return whole;
}

/* where a ramp from vol got to after samples */
static f32 ramp_at(f32 vol, s32 samples, s16 whole, u16 frac) {
    f32 step = (f32)(((f32)(whole << 16) + (f32)frac) / 65536.0);

    return (f32)(vol + step * samples * 0.125);
}

static s16 side(PVoice *pv, s32 right) {
    return (pv->volume * al_eqpower[right ? 127 - pv->pan : pv->pan]) >> 15;
}

/* n samples of the voice into the buses at offset dst (bytes) of the
   section; *in is where the resampler puts them */
static Acmd *mix(PVoice *pv, s16 *in, s16 dst, s32 n, Acmd *cmd) {
    if (n == 0)
        return cmd;
    cmd = resample(pv, *in, n, cmd);
    aSetBuffer(cmd++, A_MAIN, *in, DMEM_MAIN_L + dst, n << 1);
    aSetBuffer(cmd++, A_AUX, DMEM_MAIN_R + dst, DMEM_AUX_L + dst, DMEM_AUX_R + dst);
    if (pv->env_first) {
        pv->env_first = 0;
        pv->target_l = side(pv, 0);
        pv->rate_l = ramp_step(pv->cur_l, pv->target_l, pv->seg_len, &pv->rate_l_frac);
        pv->target_r = side(pv, 1);
        pv->rate_r = ramp_step(pv->cur_r, pv->target_r, pv->seg_len, &pv->rate_r_frac);
        aSetVolume(cmd++, A_LEFT | A_VOL, pv->cur_l, 0, 0);
        aSetVolume(cmd++, A_RIGHT | A_VOL, pv->cur_r, 0, 0);
        aSetVolume(cmd++, A_LEFT | A_RATE, pv->target_l, pv->rate_l, pv->rate_l_frac);
        aSetVolume(cmd++, A_RIGHT | A_RATE, pv->target_r, pv->rate_r, pv->rate_r_frac);
        aSetVolume(cmd++, A_AUX, pv->dry, 0, pv->wet);
        aEnvMixer(cmd++, A_INIT | A_AUX, phys(pv->env_state));
    } else {
        aEnvMixer(cmd++, A_CONTINUE | A_AUX, phys(pv->env_state));
    }
    *in += n << 1;
    return cmd;
}

/* the ramp so far, before a change: done, or where it got to */
static void ramp_advance(PVoice *pv, s32 samples) {
    pv->seg_pos += samples;
    if (pv->seg_pos >= pv->seg_len) {
        pv->target_l = side(pv, 0);
        pv->target_r = side(pv, 1);
        pv->seg_pos = pv->seg_len;
        pv->cur_l = pv->target_l;
        pv->cur_r = pv->target_r;
    } else {
        pv->cur_l = (s16)(s32)ramp_at(pv->cur_l, pv->seg_pos, pv->rate_l, pv->rate_l_frac);
        pv->cur_r = (s16)(s32)ramp_at(pv->cur_r, pv->seg_pos, pv->rate_r, pv->rate_r_frac);
    }
    /* (a ramp can't start from 0) */
    if (pv->cur_l == 0)
        pv->cur_l = 1;
    if (pv->cur_r == 0)
        pv->cur_r = 1;
}

static void start(PVoice *pv, Update *u) {
    if (u->unity)
        pv->unity = 1;
    set_table(pv, u->wave);
    pv->playing = 1;
}

/* n samples of this voice from sample `at`: the updates due in them first,
   each at its sample */
Acmd *al_voice_mix(PVoice *pv, s32 n, s32 at, Acmd *cmd) {
    s16 in = DMEM_DECODED_IN;
    s16 dst = 0;
    s32 t = at;

    while (pv->updates != NULL) {
        Update *u = pv->updates;
        s32 gap = u->at - t;

        t = u->at;
        if (gap > n)
            break;

        switch (u->kind) {
        case UPD_START_PARAMS: {
            s32 v = ((s32)u->b.s.volume * (s32)u->b.s.volume) >> 15;
            start(pv, u);
            pv->env_first = 1;
            pv->seg_pos = 0;
            pv->seg_len = u->ramp;
            pv->volume = (s16)v;
            pv->pan = u->b.s.pan;
            pv->dry = al_eqpower[u->b.s.fxmix];
            pv->wet = al_eqpower[127 - u->b.s.fxmix];
            if (u->ramp != 0) {
                pv->cur_l = pv->cur_r = 1;
            } else {
                /* no attack: at the volume at once */
                pv->cur_l = side(pv, 0);
                pv->cur_r = side(pv, 1);
            }
            pv->ratio = u->a.f;
            break;
        }
        case UPD_FXMIX:
        case UPD_PAN:
        case UPD_VOLUME:
            cmd = mix(pv, &in, dst, gap, cmd);
            ramp_advance(pv, gap);
            if (u->kind == UPD_PAN)
                pv->pan = (s16)u->a.i;
            if (u->kind == UPD_VOLUME) {
                /* a new ramp, to the square of the volume (nearer loudness) */
                s32 v = u->a.i;
                pv->seg_pos = 0;
                pv->volume = (s16)((v * v) >> 15);
                pv->seg_len = u->b.ramp;
            }
            if (u->kind == UPD_FXMIX) {
                pv->dry = al_eqpower[u->a.i];
                pv->wet = al_eqpower[127 - u->a.i];
            }
            pv->env_first = 1;
            break;
        case UPD_START:
            start(pv, u);
            break;
        case UPD_STOP:
            cmd = mix(pv, &in, dst, gap, cmd);
            reset(pv);
            break;
        case UPD_FREE_VOICE: {
            PVoice *gone = u->a.p;
            gone->steal_delay = 0;
            al_voice_release(&alGlobals->drvr, gone);
            break;
        }
        default:        /* UPD_PITCH */
            cmd = mix(pv, &in, dst, gap, cmd);
            pv->seg_pos += gap;
            pv->ratio = u->a.f;
            break;
        }
        dst += gap << 1;
        n -= gap;

        pv->updates = u->next;
        if (pv->updates == NULL)
            pv->updates_tail = NULL;
        al_update_free(&alGlobals->drvr, u);
    }

    if (pv->playing) {
        cmd = mix(pv, &in, dst, n, cmd);
        pv->seg_pos += n;
    }
    if (pv->seg_pos > pv->seg_len)
        pv->seg_pos = pv->seg_len;
    return cmd;
}
