/*
 * The audio interface (AI): the DAC and its two-deep DMA queue, in N64
 * time, and the sound it plays through SDL (and into a WAV file).
 *
 * The game's audio thread paces itself on the AI: every other retrace it
 * queues the previous frame's buffer (osAiSetNextBuffer) and sizes the next
 * one by what's still to play (osAiGetLength), so the AI has to drain at
 * the DAC's rate against the same clock as the retraces and timers.  Here
 * it does: the queue advances with host_now_ns(), which is the clock the
 * main loop raises the VI and the timers by (real time, or --deterministic's
 * virtual time).  A buffer's samples go to SDL when it is queued; in real
 * time SDL's queue then stays about two frames deep by itself.
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

int host_audio_enabled = 1;
const char *host_wav_path;

static uint32_t dacrate = 2208;     /* osAiSetFrequency(22050) on NTSC */
static uint64_t samples_out;        /* all queued so far */

/* ---- the AI's DMA queue ----------------------------------------------------- */

#define VI_CLOCK_NTSC 48681812ull

static struct {
    uint32_t len;                   /* bytes */
    uint64_t start_ns;              /* when it started (the current one) */
} fifo[2];
static int nfifo;

static uint64_t ns_for(uint32_t bytes) {
    /* 4 bytes a sample pair, one pair every dacrate cycles of the VI clock */
    return (uint64_t)(bytes / 4) * dacrate * 1000000000ull / VI_CLOCK_NTSC;
}

static void ai_advance(void) {
    uint64_t now = host_now_ns();
    while (nfifo > 0 && now >= fifo[0].start_ns + ns_for(fifo[0].len)) {
        uint64_t end = fifo[0].start_ns + ns_for(fifo[0].len);
        fifo[0] = fifo[1];
        nfifo--;
        if (nfifo > 0)
            fifo[0].start_ns = end;
    }
}

void host_ai_set_dacrate(uint32_t rate) {
    if (rate > 0)
        dacrate = rate;
}

/* ---- SDL and the WAV file ------------------------------------------------------ */

static SDL_AudioDeviceID dev;
static int dev_tried, dev_started;
static FILE *wav;
static uint32_t wav_rate;

static void wav_header(void) {
    uint32_t data = (uint32_t)(samples_out * 4);
    uint8_t h[44] = { 'R', 'I', 'F', 'F' };
    uint32_t v[] = { 36 + data };
    memcpy(h + 4, v, 4);
    memcpy(h + 8, "WAVEfmt ", 8);
    uint32_t fmt[] = { 16, 1 | 2 << 16, wav_rate, wav_rate * 4, 4 | 16 << 16 };
    memcpy(h + 16, fmt, sizeof fmt);
    memcpy(h + 36, "data", 4);
    memcpy(h + 40, &data, 4);
    fseek(wav, 0, SEEK_SET);
    fwrite(h, 1, sizeof h, wav);
    fseek(wav, 0, SEEK_END);
}

static uint32_t rate_hz(void) {
    return (uint32_t)((VI_CLOCK_NTSC + dacrate / 2) / dacrate);
}

static void open_outputs(void) {
    dev_tried = 1;
    if (host_wav_path) {
        wav = fopen(host_wav_path, "wb");
        if (!wav)
            host_log("audio: can't write %s\n", host_wav_path);
        wav_rate = rate_hz();
        if (wav)
            wav_header();
    }
    if (!host_audio_enabled)
        return;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        host_log("audio: %s; no sound\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want = { 0 }, have;
    want.freq = (int)rate_hz();
    want.format = AUDIO_S16SYS;
    want.channels = 2;
#ifdef __EMSCRIPTEN__
    /* (the page's ScriptProcessorNode runs on the page's thread, which the
       game shares: a buffer of 1024 at 48 kHz is 21 ms between callbacks) */
    want.samples = 1024;
#else
    want.samples = 512;
#endif
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev)
        host_log("audio: %s; no sound\n", SDL_GetError());
    else if (host_verbose)
        host_log("audio: %d Hz\n", want.freq);
}

void host_audio_shutdown(void) {
    if (wav) {
        wav_header();
        fclose(wav);
        wav = NULL;
    }
    if (dev)
        SDL_CloseAudioDevice(dev);
}

uint64_t host_audio_samples(void) { return samples_out; }

/* PORT_PERF's report: what SDL has queued, and how often the queue was
   started over */
static unsigned dropped;
void host_audio_perf(int *queued_ms, unsigned *ndropped) {
    *queued_ms = dev ? (int)(SDL_GetQueuedAudioSize(dev) * 1000ull / (rate_hz() * 4)) : -1;
    *ndropped = dropped;
}

/* osAiSetNextBuffer: -1 if the queue is full, as the hardware's AI_STATUS
   would have libultra return */
int host_ai_submit(uint32_t addr, uint32_t len) {
    if (!dev_tried)
        open_outputs();
    ai_advance();
    if (host_verbose > 2)
        host_log("ai: t=%.3f ms nfifo=%d left=%u new=%u\n", host_now_ns() / 1e6, nfifo, host_ai_length() / 4, len / 4);
    if (nfifo == 2)
        return -1;
    len &= 0x3FFF8;
    fifo[nfifo].len = len;
    fifo[nfifo].start_ns = host_now_ns();
    nfifo++;

    static int16_t buf[0x10000];
    const uint8_t *src = port_ptr(addr);
    uint32_t n = len / 2;
    for (uint32_t i = 0; i < n; i++)
        buf[i] = (int16_t)port_be16(src + 2 * i);
    samples_out += len / 4;
    if (wav)
        fwrite(buf, 2, n, wav);
    if (dev) {
        /* Keep at most 0.25 s queued.  More means the device played slower
           than the game made sound (the host fell behind and caught up, or
           the device stalled: a page whose thread was busy, a suspended
           WebAudio context): then what's queued goes, and the sound starts
           over from this buffer, rather than every new buffer being
           dropped while the old ones play out (which, with a device that
           had stopped, was silence for good).  Start once two frames are
           in. */
        uint32_t queued = SDL_GetQueuedAudioSize(dev);
        static unsigned nsub;
        if (host_verbose && ++nsub % 150 == 0)
            host_log("audio: %u ms queued in SDL\n", queued * 1000 / (rate_hz() * 4));
        if (queued > rate_hz() * 4 / 4) {
            SDL_ClearQueuedAudio(dev);
            dropped++;
            if (host_verbose)
                host_log("audio: %u ms queued: starting over\n", queued * 1000 / (rate_hz() * 4));
        }
        SDL_QueueAudio(dev, buf, len);
        if (!dev_started && SDL_GetQueuedAudioSize(dev) >= len * 2) {
            SDL_PauseAudioDevice(dev, 0);
            dev_started = 1;
        }
    }
    return 0;
}

/* osAiGetLength: what's left of the buffer playing now */
uint32_t host_ai_length(void) {
    ai_advance();
    if (nfifo == 0)
        return 0;
    uint64_t now = host_now_ns(), done = now - fifo[0].start_ns;
    uint64_t played = done * VI_CLOCK_NTSC / (1000000000ull * dacrate) * 4;
    return played >= fifo[0].len ? 0 : (uint32_t)(fifo[0].len - played);
}

/* AI_STATUS: full, busy */
uint32_t host_ai_status(void) {
    ai_advance();
    return (nfifo == 2 ? 0x80000000u : 0) | (nfifo > 0 ? 0x40000000u : 0);
}
