/* Host-side internals (port/host). */
#ifndef HOST_H
#define HOST_H

#include "port.h"
#include "recomp.h"

/* threads.c */
int host_run_one(void);             /* run the best runnable thread; 0 if none */
void host_threads_dump(void);
recomp_context *port_ctx(void);

void host_charge(uint64_t ns);
uint64_t host_busy_wake(void);       /* threads.c: when a busy thread may go on */
extern double host_ns_per_instr, host_c_scale;          /* the N64's clock: real or virtual time */

/* audio.c */
extern int host_audio_enabled;
extern const char *host_wav_path;
uint64_t host_audio_samples(void);
void host_audio_shutdown(void);

/* video.c */
extern int host_max_frames;
extern const char *host_screenshot_prefix;
extern int host_headless;
extern const char *host_save_path;
void host_video_init(void);
void host_video_frame(void);
void host_video_shutdown(void);
int host_frame_held(void);          /* the game's mode holds each frame for two retraces */

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
void host_gfx_interp_report(void);
void host_gfx_frame_shown(uint32_t fb);     /* the VI shows fb from this retrace on */
extern float gfx_aspect;            /* widescreen: 0 4:3, GFX_ASPECT_WINDOW, or width / height */
#define GFX_ASPECT_WINDOW (-1.0f)
struct SDL_Window;
unsigned gfx_gl_window_flags(void);
int gfx_gl_init(struct SDL_Window *win);
void gfx_gl_present(uint32_t vi_fb, int vi_width, const char *screenshot);

#endif
