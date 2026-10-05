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

static uint32_t dacrate = (PORT_VI_CLOCK + 22050 / 2) / 22050;  /* osAiSetFrequency(22050) */
static uint64_t samples_out;        /* all queued so far */

/* ---- the AI's DMA queue ----------------------------------------------------- */

#define VI_CLOCK ((uint64_t)PORT_VI_CLOCK)    /* PAL's in eu */

static struct {
    uint32_t len;                   /* bytes */
    uint64_t start_ns;              /* when it started (the current one) */
} fifo[2];
static int nfifo;

static uint64_t ns_for(uint32_t bytes) {
    /* 4 bytes a sample pair, one pair every dacrate cycles of the VI clock */
    return (uint64_t)(bytes / 4) * dacrate * 1000000000ull / VI_CLOCK;
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

/* ---- the page's sound: an AudioWorklet ----------------------------------------- */

#ifdef PORT_WASM_WEB
#include <emscripten.h>
/*
 * In a page the sound doesn't go through SDL, whose WebAudio driver is a
 * ScriptProcessorNode: its callback runs on the page's one thread, which
 * the game holds most of the time (a long retrace, a level loading, a
 * shader compiled: the callback waits, the device doesn't), and SDL
 * resampled to the device's rate on that thread too.  Here each buffer
 * the game queues is posted to an AudioWorklet (the browser's audio
 * thread), which plays it from a ring of its own:
 *
 * - The AudioContext runs at the DAC's rate (22047 Hz) where the browser
 *   allows it, so the browser resamples to the device; otherwise the
 *   worklet does, by interpolation.
 * - The game makes sound at the retraces' rate, which follows the display
 *   (main.c): a 59.94 Hz display makes it 0.1% slow against the audio
 *   device, which would drain any queue in time (a fast one fills it).
 *   The game's buffers come two retraces apart, so the queue is a
 *   sawtooth 33 ms high; what runs it dry is its low points, just before
 *   each buffer.  The worklet steers those (averaged over about a second)
 *   to 20 ms: up to 0.5% faster or slower (interpolating, Catmull-Rom)
 *   while they are more than 5 ms off, the samples as they are while they
 *   aren't (after a correction it creeps on to a whole sample, a
 *   ten-thousandth fast, rather than jump).  About 37 ms are queued on
 *   average.
 * - Run dry (the game held up longer than the queue lasted), it plays
 *   silence until 40 ms are queued again; with more than 250 ms queued (a
 *   page that was hidden) it drops all but 40.
 * - The samples go to it as they are (s16, copied out of the heap by the
 *   browser); it makes them floats.
 *
 * PORT_AUDIO_SKEW=X makes it play X times as fast as the game makes
 * sound, the drift of a display that isn't 60 Hz (a test).
 *
 * Its counts reach the page as Module.audioStats (web_perf.mjs prints
 * them) and PORT_PERF.  Without AudioWorklet (a page served over plain
 * HTTP from another host than localhost is no secure context, and has
 * none) the sound goes through SDL.
 *
 * (EM_JS's code is one line after the preprocessor: every statement ends
 * in a semicolon, and no apostrophes in its comments.)
 */
EM_JS(int, web_audio_open, (int rate, double skew), {
    var AC = globalThis.AudioContext || globalThis.webkitAudioContext;
    if (!AC || !globalThis.AudioWorkletNode || !globalThis.isSecureContext) return 0;
    var ctx;
    try { ctx = new AC({ sampleRate: rate, latencyHint: 'interactive' }); }
    catch (e) { ctx = new AC({ latencyHint: 'interactive' }); }
    if (!ctx.audioWorklet) { ctx.close(); return 0; }
    var A = Module.webAudio = { ctx: ctx, node: null };
    Module.audioContext = ctx;
    Module.audioStats = { rate: ctx.sampleRate, underruns: 0, silentFrames: 0, trimmed: 0, queuedMs: 0, ratio: 1 };
    /* the processor, as source (the page stays one file) */
    var src = '(' + (function () {
        class Ring extends AudioWorkletProcessor {
            constructor(o) {
                super();
                var q = o.processorOptions;
                this.cap = 1 << 15;                 /* frames: 1.5 s at 22047 Hz */
                this.buf = new Float32Array(this.cap * 2);
                this.w = 0;                         /* frames written, ever */
                this.r = 0;                         /* the read position (frames, fractional) */
                this.base = q.srcRate / sampleRate * q.skew;    /* source frames an output frame */
                this.target = q.srcRate * 0.020;    /* the low point steered to (source frames) */
                this.band = q.srcRate * 0.005;
                this.start = q.srcRate * 0.040;     /* queued before playing (again) */
                this.max = q.srcRate * 0.250;
                this.playing = false;
                this.level = 0;                     /* the level, averaged */
                this.low = 0;                       /* its low points, averaged */
                this.wmin = 1e9; this.wn = 0;       /* the low point of the last 32 quanta */
                this.ratio = this.base;
                this.underruns = 0; this.silent = 0; this.trimmed = 0; this.n = 0;
                this.lastPut = -1; this.maxApart = 0;   /* the longest wait for a buffer (s), since the last report */
                this.port.onmessage = (e) => { this.put(e.data); };
            }
            put(f) {
                var n = f.length >> 1, cap = this.cap, b = this.buf;
                if (this.w + n - this.r > cap - 4) return;          /* (full: only past the trim) */
                for (var i = 0, j = (this.w % cap) * 2; i < n; i++) {
                    b[j] = f[2 * i] / 32768; b[j + 1] = f[2 * i + 1] / 32768;
                    j += 2;
                    if (j === cap * 2) j = 0;
                }
                this.w += n;
                if (this.lastPut >= 0 && currentTime - this.lastPut > this.maxApart) this.maxApart = currentTime - this.lastPut;
                this.lastPut = currentTime;
            }
            process(inputs, outputs) {
                var L = outputs[0][0], R = outputs[0][1] || L, n = L.length, cap = this.cap, b = this.buf;
                var avail = this.w - this.r, i = 0;
                if (avail > this.max) {             /* far behind: on from 40 ms back */
                    this.r = this.w - this.start;
                    avail = this.start;
                    this.playing = false;
                    this.trimmed++;
                }
                if (!this.playing && avail >= this.start) {
                    this.playing = true;
                    this.level = avail;
                    this.low = this.target;
                    this.wmin = 1e9; this.wn = 0;
                }
                if (this.playing) {
                    /* the level, and its low points (each just before the
                       next buffer came), over about a second */
                    var dt = n / sampleRate;
                    this.level += (avail - this.level) * dt;
                    if (avail < this.wmin) this.wmin = avail;
                    if (++this.wn === 32) {
                        this.low += (this.wmin - this.low) * 32 * dt;
                        this.wmin = 1e9; this.wn = 0;
                    }
                    var off = this.low - this.target;
                    if (off > this.band || off < -this.band) {
                        var adj = off / this.target * 0.01;
                        if (adj > 0.005) adj = 0.005;
                        if (adj < -0.005) adj = -0.005;
                        this.ratio = this.base * (1 + adj);
                    } else if (this.base === 1 && this.r !== Math.floor(this.r)) {
                        /* back in the band: on to the next whole sample, a
                           ten-thousandth faster (under half a second), and
                           from there the samples as they are */
                        this.ratio = 1.0001;
                    } else {
                        this.ratio = this.base;
                    }
                    var r = this.r, ratio = this.ratio, end = this.w - 2;
                    if (ratio === 1 && r === Math.floor(r)) {
                        for (; i < n && r < end; i++, r++) {
                            var k = (r % cap) * 2;
                            L[i] = b[k]; R[i] = b[k + 1];
                        }
                    } else {
                        for (; i < n && r < end; i++, r += ratio) {
                            var i0 = Math.floor(r), t = r - i0;
                            var km = ((i0 - 1 + cap) % cap) * 2, k0 = (i0 % cap) * 2;
                            var k1 = ((i0 + 1) % cap) * 2, k2 = ((i0 + 2) % cap) * 2;
                            for (var c = 0; c < 2; c++) {
                                var p0 = b[km + c], p1 = b[k0 + c], p2 = b[k1 + c], p3 = b[k2 + c];
                                var v = p1 + 0.5 * t * (p2 - p0 + t * (2 * p0 - 5 * p1 + 4 * p2 - p3 + t * (3 * (p1 - p2) + p3 - p0)));
                                if (c) R[i] = v; else L[i] = v;
                            }
                        }
                    }
                    if (ratio === 1.0001 && r - Math.floor(r) < n * 0.0001)
                        r = Math.floor(r);          /* (a shift of a hundredth of a sample) */
                    this.r = r;
                    if (i < n) {                    /* run dry */
                        this.playing = false;
                        this.underruns++;
                    }
                }
                if (i < n) {
                    this.silent += n - i;
                    for (; i < n; i++) { L[i] = 0; R[i] = 0; }
                }
                if (++this.n % 64 === 0)            /* (3 times a second at 22 kHz) */
                    this.port.postMessage({ underruns: this.underruns, silentFrames: this.silent, trimmed: this.trimmed,
                                            queuedMs: this.level / (this.base * sampleRate) * 1000,
                                            lowMs: this.low / (this.base * sampleRate) * 1000, apartMs: this.maxApart * 1000,
                                            ratio: this.ratio / this.base });
                if (this.n % 64 === 0) this.maxApart = 0;
                return true;
            }
        }
        registerProcessor('port-audio', Ring);
    }).toString() + ')();';
    var url = URL.createObjectURL(new Blob([src], { type: 'text/javascript' }));
    ctx.audioWorklet.addModule(url).then(function () {
        var node = new AudioWorkletNode(ctx, 'port-audio', { numberOfInputs: 0, outputChannelCount: [2],
                                                            processorOptions: { srcRate: rate, skew: skew } });
        node.port.onmessage = function (e) {
            var s = Module.audioStats, d = e.data;
            s.underruns = d.underruns; s.silentFrames = d.silentFrames; s.trimmed = d.trimmed;
            s.queuedMs = d.queuedMs; s.lowMs = d.lowMs; s.ratio = d.ratio;
            s.apartMs = Math.max(s.apartMs || 0, d.apartMs);
        };
        node.connect(ctx.destination);
        A.node = node;
    }).catch(function (e) { console.warn('audio: ' + e); });
    return 1;
});

/* a buffer (frames of host-order s16 pairs) to the worklet: none until it
   has started */
EM_JS(void, web_audio_push, (const int16_t *p, int frames), {
    var A = Module.webAudio;
    if (!A || !A.node) return;
    var s = HEAP16.slice(p >> 1, (p >> 1) + frames * 2);  /* (the worklet makes them floats) */
    A.node.port.postMessage(s, [s.buffer]);
});

/* PORT_PERF's: 0 the level queued (ms), 1 the times it ran dry or was trimmed */
EM_JS(int, web_audio_stat, (int which), {
    var s = Module.audioStats;
    if (!s) return -1;
    return which === 0 ? Math.round(s.queuedMs) : s.underruns + s.trimmed;
});

static int web_audio;               /* the sound goes to the worklet */
#endif

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
    return (uint32_t)((VI_CLOCK + dacrate / 2) / dacrate);
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
    /* PORT_AUDIO_SKEW=1.001: the device plays that much faster than the
       game makes sound (a test: the drift of a 59.94 Hz display) */
    const char *e = getenv("PORT_AUDIO_SKEW");
    double skew = e && atof(e) > 0 ? atof(e) : 1.0;
#ifdef PORT_WASM_WEB
    /* (PORT_AUDIO_SDL=1: through SDL, to compare) */
    e = getenv("PORT_AUDIO_SDL");
    if ((!e || *e == '0') && (web_audio = web_audio_open((int)rate_hz(), skew))) {
        if (host_verbose)
            host_log("audio: %d Hz, an AudioWorklet\n", (int)rate_hz());
        return;
    }
#endif
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        host_log("audio: %s; no sound\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want = { 0 }, have;
    want.freq = (int)(rate_hz() * skew + 0.5);
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
#ifdef PORT_WASM_WEB
    if (web_audio) {        /* (dropped: the times it ran dry or was trimmed) */
        *queued_ms = web_audio_stat(0);
        *ndropped = (unsigned)web_audio_stat(1);
        return;
    }
#endif
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
#ifdef PORT_WASM_WEB
    if (web_audio)
        web_audio_push(buf, (int)(len / 4));
#endif
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
    uint64_t played = done * VI_CLOCK / (1000000000ull * dacrate) * 4;
    return played >= fifo[0].len ? 0 : (uint32_t)(fifo[0].len - played);
}

/* AI_STATUS: full, busy */
uint32_t host_ai_status(void) {
    ai_advance();
    return (nfifo == 2 ? 0x80000000u : 0) | (nfifo > 0 ? 0x40000000u : 0);
}
