/* Host-side internals (port/host). */
#ifndef HOST_H
#define HOST_H

#include <stddef.h>

#include "port.h"
#include "recomp.h"

/* main.c: the ROM as loaded (big-endian), romdata.c: its sha1, and the
   arena's contents made from it (PORT_ROM_DATA) */
const uint8_t *host_rom(void);
void host_sha1_hex(const uint8_t *p, size_t n, char out[41]);
void port_romdata_apply(uint8_t *arena, const uint8_t *rom, uint32_t rom_size);
/* romdata.c: a gzip member's contents into out (at most outn bytes); their
   length, or -1 (and in *used, the member's own length).  pack.c: a texture's blast stream of type t (1..6) decoded
   (malloc'd, *outn bytes), or NULL */
long host_gunzip(const uint8_t *in, size_t n, uint8_t *out, size_t outn, size_t *used);
uint8_t *host_blast_decode(int t, const uint8_t *s, size_t n, const uint8_t *lut, size_t lut_len, size_t *outn);

/* threads.c */
int host_run_one(void);             /* run the best runnable thread; 0 if none */
void host_threads_dump(void);
recomp_context *port_ctx(void);

void host_charge(uint64_t ns);
int host_rdp_scaled(void);             /* PORT_RDP_SCALE isn't 0: host_charge counts */
uint64_t host_busy_wake(void);       /* threads.c: when a busy thread may go on */
extern double host_ns_per_instr, host_c_scale;          /* the N64's clock: real or virtual time */

/* perf.c: PORT_PERF=N, where the host's time goes per retrace */
enum { PERF_LOOP, PERF_GAME, PERF_GFX, PERF_GFX2, PERF_AUDIO, PERF_PRESENT, PERF_IDLE, PERF_SHADER, PERF_GL, PERF_NCAT };
extern int host_perf_on;
int host_is_deterministic(void);            /* main.c: --deterministic (or --replay) */
extern int host_paced;                      /* main.c: PORT_PACED, virtual time between retraces */
extern unsigned host_paced_resyncs;         /* ... and how often it fell behind */
extern int host_interp_limit;               /* main.c: the host is behind: at most so many in-between images */
int gfx_interp_twins_used(void);            /* gfx.c: the in-between images the last frames had */
void host_perf_init(void);
double host_perf_now(void);                 /* ms */
void host_perf_push(int cat);               /* time from here on is cat's ... */
void host_perf_pop(void);                   /* ... until this */
void host_perf_sleep(double want_ms, double got_ms);
void host_perf_vi(double late_ms, unsigned long long images, unsigned long long frames);
void host_perf_presented(unsigned long long images);

/* audio.c */
extern int host_audio_enabled;
extern const char *host_wav_path;
uint64_t host_audio_samples(void);
void host_audio_shutdown(void);
void host_audio_perf(int *queued_ms, unsigned *dropped);     /* queued -1: no device */

/* video.c */
extern int host_max_frames;
extern const char *host_screenshot_prefix;
extern int host_headless;
extern const char *host_save_path;
void host_video_init(void);
void host_video_frame(void);
void host_video_frame_hold(void);       /* the present queue (main.c): the picture held for its slot */
int host_video_held(void);
extern int host_queue_on;                       /* main.c: PORT_QUEUE, presenting a retrace late */
extern unsigned long long host_queue_presents, host_queue_late;
void host_video_present_held(void);
void host_video_between(double phase);   /* --display-hz: a present `phase` retraces after the last */
void host_video_shutdown(void);
int host_frame_held(void);          /* the game's mode holds each frame for two retraces */
int host_window_to_n64(float wx, float wy, float *x, float *y);  /* a point of the window on the 320x240 screen */
void host_type_char(int c);         /* the name entry: c typed ('\b' Backspace, 0x1B Escape) */

/* N64 buttons */
enum {
    B_A = 0x8000, B_B = 0x4000, B_Z = 0x2000, B_START = 0x1000, B_DU = 0x0800, B_DD = 0x0400,
    B_DL = 0x0200, B_DR = 0x0100, B_L = 0x0020, B_R = 0x0010, B_CU = 0x0008, B_CD = 0x0004,
    B_CL = 0x0002, B_CR = 0x0001,
};

/* ui.c: the keyboard, the mouse and touch, by what is on the screen */
union SDL_Event;
void host_ui_event(const union SDL_Event *e);
void host_ui_input(int live, const uint8_t *keys, uint16_t *buttons, int *x, int *y);
/* --free-camera: the mouse and the right stick turn a level's camera
   (port_camera_turn); PORT_CAMERA_SENS, the mouse's degrees a pixel */
extern int host_free_camera;
extern float host_camera_sens;
/* the right stick for this read (video.c, before host_ui_input): 1 if it
   turns the camera, 0 if it is the C buttons still */
int host_ui_camera_stick(int x, int y);

/* digest.c: PORT_DIGEST=FILE, the gameplay digest at every controller poll */
void host_digest_poll(unsigned poll);

/* replay.c: --replay */
int host_replay_active(void);
void host_replay_load(const char *path);
int host_replay_poll_si(void);
int host_replay_vi_ok(void);
void host_replay_vi_forced(void);
void host_replay_pad(uint16_t *buttons, int *x, int *y);
int host_replay_done(void);
void host_replay_report(void);

/* the renderer: 0 software (gfx.c), 1 OpenGL (gfx_gl.c); -1 until chosen
   (OpenGL with a window, software headless) */
extern int host_renderer;
extern int gfx_filter;              /* GFX_FILTER_* (gfx.h) */
extern int gfx_gl_scale;            /* internal resolution factor, 0: the window's */
extern int gfx_interp;              /* --interpolate: in-between frames (gfx.c) */
extern int gfx_interp_hz;           /* --display-hz: the rate in-between images are made for (60) */
void host_gfx_interp_report(void);
void host_gfx_frame_shown(uint32_t fb);     /* the VI shows fb from this retrace on */
extern float gfx_aspect;            /* widescreen: 0 4:3, GFX_ASPECT_WINDOW, or width / height */
#define GFX_ASPECT_WINDOW (-1.0f)
extern int gfx_hud_edges;          /* --hud: 1 the HUD at a wide picture's sides, 0 in the 4:3 middle */
extern int gfx_gl_max_pixels;      /* --max-pixels: the internal resolution's cap (0: none) */
struct SDL_Window;
unsigned gfx_gl_window_flags(void);
int gfx_gl_init(struct SDL_Window *win);
/* twin: the in-between image to show (gfx_interp_image), or -1 */
void gfx_gl_present(uint32_t vi_fb, int vi_width, const char *screenshot, int twin);

#endif
