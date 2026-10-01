/*
 * Window, input and presentation (SDL2).
 *
 * With the software renderer, what's shown is the VI's current
 * framebuffer as it is in RDRAM (RGBA 5551), which gfx.c draws into as the
 * RDP would.  With the OpenGL one, gfx_gl.c presents the GPU's copy of it.
 */
#include <SDL.h>
#ifdef PORT_WASM_WEB
#include <emscripten.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fiber.h"
#include "host.h"
#include "gfx.h"

#ifdef PORT_NATIVE_ENDIAN
/* game variables are read with port_be32 (and the pad written with
   port_wbe16) below: at their own width in RDRAM (port.h) */
#define port_be32(p) port_var32(p)
#define port_wbe16(p, v) port_wvar16((p), (v))
#endif

int host_max_frames;
const char *host_screenshot_prefix;
int host_headless;
int host_renderer = -1;

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static SDL_GameController *pad;
static uint32_t vi_fb;
static int vi_width = 320;
static int frame;
static int quit;
static uint32_t pixels[640 * 480];
static int tex_w = 320;         /* the software renderer's frame: 320, or wider (widescreen) */

/* the software renderer's widescreen: from the window, or --aspect */
static void sw_wide(void) {
    int w = 0, h = 0;
    if (win)
        SDL_GetWindowSize(win, &w, &h);
    gfx_set_wide(gfx_wide_off_for(gfx_aspect_of(w, h)));
}

void host_vi_set_framebuffer(uint32_t fb, int width) {
    /* a retrace that shows another buffer: the frame in it is complete (the
       next one's tasks wait for the RDP's thaw, after this) */
    static uint32_t shown;
    if (fb && fb != shown) {
        shown = fb;
        host_gfx_frame_shown(fb);
    }
    vi_fb = fb;
    if (width > 0 && width <= 640)
        vi_width = width;
}

/* SDL is up (not in emscripten's headless runs: it has no offscreen
   driver, and node no page; the software renderer draws all the same) */
static int sdl_up;

void host_video_init(void) {
#ifdef __EMSCRIPTEN__
    if (host_headless) {
        if (host_renderer == 1)
            host_log("headless: using the software renderer\n");
        host_renderer = 0;
        return;
    }
#endif
    if (host_headless)
        SDL_setenv("SDL_VIDEODRIVER", "offscreen", 1);
#ifdef __EMSCRIPTEN__
    /* SDL_GL_SwapWindow would emscripten_sleep(0) (a 4 ms setTimeout, and
       one more Asyncify unwind) each frame: the loop gives the page its
       turn itself (main.c) */
    if (host_paced)
        SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0)
        host_fatal("SDL_Init: %s", SDL_GetError());
    sdl_up = 1;
    if (host_renderer < 0)
        host_renderer = !host_headless;
#ifndef PORT_HAVE_GL
    if (host_renderer == 1)
        host_log("built without OpenGL; using the software renderer\n");
    host_renderer = 0;
#endif
    int ww = 640, wh = 480;
    if (host_renderer == 1 && gfx_gl_scale > 2) {
        ww = 320 * gfx_gl_scale;
        wh = 240 * gfx_gl_scale;
    }
    if (gfx_interp_hz < 0) {                        /* --display-hz auto */
        SDL_DisplayMode m;
        gfx_interp_hz = SDL_GetCurrentDisplayMode(0, &m) == 0 && m.refresh_rate > 60 ? m.refresh_rate : 60;
        if (gfx_interp)
            host_log("--display-hz: %d\n", gfx_interp_hz);
    }
    if (gfx_aspect > 0)                             /* widescreen: a window that wide */
        ww = (int)(wh * gfx_aspect_of(0, 0) + 0.5f);
    win = SDL_CreateWindow("Blast Corps", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh,
                           SDL_WINDOW_RESIZABLE | (host_renderer == 1 ? gfx_gl_window_flags() : 0));
    if (!win)
        host_fatal("SDL_CreateWindow: %s", SDL_GetError());
    if (host_renderer == 1 && !gfx_gl_init(win))
        host_renderer = 0;
    if (host_renderer == 1)
        goto pads;
    ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren)
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren)
        host_fatal("SDL_CreateRenderer: %s", SDL_GetError());
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
    sw_wide();
pads:
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        if (SDL_IsGameController(i) && (pad = SDL_GameControllerOpen(i)))
            break;
}

void host_video_shutdown(void) {
    if (sdl_up)
        SDL_Quit();
}

static void save_bmp(const char *path, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    uint32_t rowbytes = (uint32_t)w * 3, pad4 = (4 - rowbytes % 4) % 4, size = (rowbytes + pad4) * (uint32_t)h;
    uint8_t hdr[54] = { 'B', 'M' };
    uint32_t v[] = { 54 + size, 0, 54, 40, (uint32_t)w, (uint32_t)h, 1 | (24 << 16), 0, size, 2835, 2835, 0, 0 };
    memcpy(hdr + 2, v, sizeof v);
    fwrite(hdr, 1, 54, f);
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            uint32_t p = pixels[y * w + x];
            uint8_t bgr[3] = { (uint8_t)p, (uint8_t)(p >> 8), (uint8_t)(p >> 16) };
            fwrite(bgr, 1, 3, f);
        }
        fwrite("\0\0\0", 1, pad4, f);
    }
    fclose(f);
}

/* the software renderer's frame into pixels (and the window's texture):
   an in-between image (twin >= 0), the frame it drew wide (widescreen),
   or RDRAM's (in the middle of a wide one) */
static int sw_present(int twin) {
    sw_wide();
    int fw = 320 + 2 * gfx_wide_off, ww = 0;
    const uint16_t *host = NULL;                    /* host order, ww across */
    if (vi_fb && twin >= 0)
        host = gfx_sw_twin_frame(vi_fb, twin, &ww);
    if (vi_fb && !host && fw > 320)
        host = gfx_sw_wide_frame(vi_fb, &ww);
    if (fw != tex_w && ren) {
        SDL_DestroyTexture(tex);
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, fw, 240);
        tex_w = fw;
        SDL_RenderSetLogicalSize(ren, fw > 320 ? fw : 0, fw > 320 ? 240 : 0);
    }
    const uint8_t *src = vi_fb && !host ? port_ptr(vi_fb) : NULL;
    memset(pixels, 0, sizeof pixels);
    for (int y = 0; (host || src) && y < 240; y++)
        for (int x = 0; x < fw; x++) {
            uint16_t c;
            if (host)
                c = ww == fw ? host[y * fw + x] : 0;
            else if (x >= gfx_wide_off && x < gfx_wide_off + 320)
                c = port_be16(src + 2 * (y * vi_width + x - gfx_wide_off));
            else
                continue;
            uint32_t r = (c >> 11) & 31, g = (c >> 6) & 31, b = (c >> 1) & 31;
            pixels[y * fw + x] = 0xFF000000u | (r << 19 | (r >> 2) << 16) | (g << 11 | (g >> 2) << 8) |
                                 (b << 3 | b >> 2);
        }
    return fw;
}

/* PORT_PERF: a present of another picture than the last (another frame,
   or another of its in-between images) */
/* the window's title (the page's, in the browser: SDL sets document.title)
   says how many pictures it showed in the last second, and how many of them
   were new frames of the game's (--interpolate's in-between ones are the
   rest); host time only, so nothing the game sees */
static void title_fps(int new_image, int new_frame) {
    static unsigned images, frames;
    static Uint32 since;
    images += new_image;
    frames += new_frame;
    Uint32 now = SDL_GetTicks();
    if (!since)
        since = now;
    if (now - since < 1000)
        return;
    char t[96];
    if (gfx_interp)
        snprintf(t, sizeof t, "Blast Corps - %.0f fps (game %.0f)", images * 1000.0 / (now - since),
                 frames * 1000.0 / (now - since));
    else
        snprintf(t, sizeof t, "Blast Corps - %.0f fps", frames * 1000.0 / (now - since));
    if (win)
        SDL_SetWindowTitle(win, t);
    images = frames = 0;
    since = now;
}

static void count_image(int twin) {
    static uint32_t last_fb;
    static int last_twin = -2;
    int new_image = vi_fb != last_fb || twin != last_twin;
    if (new_image)
        gfx_st_images++;
    if (sdl_up)
        title_fps(new_image, vi_fb != last_fb);
    last_fb = vi_fb;
    last_twin = twin;
    host_perf_presented(gfx_st_images);
}

static void type_event(const SDL_Event *e);     /* (the name entry's typing, below) */

void host_video_frame(void) {
    SDL_Event e;
    while (sdl_up && SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            quit = 1;
        if (e.type == SDL_CONTROLLERDEVICEADDED && !pad)
            pad = SDL_GameControllerOpen(e.cdevice.which);
        type_event(&e);
    }
    frame++;
    if (host_verbose && frame % 300 == 0) {
        host_log("frame %d\n", frame);
        host_threads_dump();
    }
    const char *every = getenv("PORT_SHOT_EVERY");
    int ev = every ? atoi(every) : 0;
    const char *from = getenv("PORT_SHOT_FROM");         /* ... from the Nth on */
    int shot = host_screenshot_prefix && ((host_max_frames && frame == host_max_frames) ||
                                          (ev > 0 && frame % ev == 0 && (!from || frame >= atoi(from))));
    char path[512];
    if (shot)
        snprintf(path, sizeof path, "%s%05d.bmp", host_screenshot_prefix, frame);
    /* --interpolate: the frame's in-between image for this retrace, or -1 */
    int twin = gfx_interp_image(vi_fb);
    count_image(twin);
    if (host_renderer == 1) {
        gfx_gl_present(vi_fb, vi_width, shot ? path : NULL, twin);
        if (shot)
            host_log("saved %s\n", path);
        goto done;
    }
    int fw = sw_present(twin);
    if (ren) {
        SDL_UpdateTexture(tex, NULL, pixels, fw * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }
    if (shot) {
        save_bmp(path, fw, 240);
        host_log("saved %s\n", path);
    }
done:
    if (host_max_frames && frame >= host_max_frames)
        quit = 1;
}

void host_video_between(double phase) {
    int twin = gfx_interp_image_at(vi_fb, phase);
    if (twin < 0)                                   /* the frame itself, already on screen */
        return;
    count_image(twin);
    if (host_renderer == 1) {
        gfx_gl_present(vi_fb, vi_width, NULL, twin);
        return;
    }
    int fw = sw_present(twin);
    if (ren) {
        SDL_UpdateTexture(tex, NULL, pixels, fw * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }
}

int host_quit_requested(void) { return quit || host_replay_done(); }

/* N64 buttons */
enum {
    B_A = 0x8000, B_B = 0x4000, B_Z = 0x2000, B_START = 0x1000, B_DU = 0x0800, B_DD = 0x0400,
    B_DL = 0x0200, B_DR = 0x0100, B_L = 0x0020, B_R = 0x0010, B_CU = 0x0008, B_CD = 0x0004,
    B_CL = 0x0002, B_CR = 0x0001,
};

/* the scheduler's retrace count and the mode, where the version has them */
extern char D_803156C4[], D_80364A90[];
#ifdef PORT_MOVABLE      /* where the variables are (port.h) */
#define D_803156C4 PORT_VAR(D_803156C4)
#define D_80364A90 PORT_VAR(D_80364A90)
#endif

/* The scheduler (2C560.c's __scHandleRDP) swaps as soon as a frame is
   drawn in the modes of this mask; in every other mode it holds a frame
   until a retrace has gone by since the last swap showed, so that each
   frame is on screen for two retraces at least. */
int host_frame_held(void) {
    uint64_t mode = (uint64_t)port_be32(D_80364A90) << 32 | port_be32(D_80364A90 + 4);
    return !(mode & 0xC9FD0FE79BFF80B0ull);
}

static uint16_t scripted_buttons(int *sy) {
    /* PORT_AUTOSTART=1: tap Start/A now and then, to get past the title;
       =2: the same by the game's own retrace count (the scheduler's,
       D_803156C4), and once in the level (D_80364A90 == 4) drive forward
       instead, which is what port/tools/m64p_pace.c's "play" does; =3: as
       2, and in the level tap A too, which clears the hint panels and so
       keeps the vehicle moving */
    const char *s = getenv("PORT_AUTOSTART");
    if (!s || !*s || *s == '0')
        return 0;
    if (*s == '2' || *s == '3') {
        static int in_level;
        if (port_be32(D_80364A90) == 0 && port_be32(D_80364A90 + 4) == 4)
            in_level = 1;
        if (in_level) {
            *sy = 80;
            if (*s == '3' && (int)port_be32(D_803156C4) % 120 < 4)
                return B_A;
            return 0;
        }
    }
    int f = (*s >= '2' ? (int)port_be32(D_803156C4) : frame) % 120;
    if (f < 4)
        return B_START;
    if (f >= 60 && f < 64)
        return B_A;
    return 0;
}

/* ---- typing the name ------------------------------------------------------
   The name entry (hd_front_end/1C40.c: func_801EA93C sets it up,
   func_801EAA7C runs it) picks characters off a wheel of 33 (A to Z, 1 to
   4, '/', '.' and a delete entry), turned with the stick: A takes the one
   in front, B the last one back (or leaves the screen when there is
   none), Start confirms.  There the keyboard types instead: a character
   typed turns the wheel to it (a quick spin by the host, with the game's
   own tick for each one it passes) and presses A for one read;
   Backspace presses B while the name has a character, Escape presses B
   whatever it holds (so it can go back).  The letter and digit keys
   aren't their pad buttons on that screen; the arrows, Enter and a game
   controller still are.  It is all the host's: the game's C is as it was,
   and --replay and --deterministic runs never type from the keyboard
   (PORT_TYPE=TEXT types TEXT in any run, for a test). */

/* (us.v11's names: the menu's screen and its state are hd_code's, the
   wheel the front end's, loaded whenever that screen is up) */
extern char D_8036BB18[], D_8036BB1C[], D_802154B2[], D_802154B4[], D_802154BC[], D_802154BE[],
    D_80215924[], D_802082FC[], D_80208314[], D_80208350[];
#ifdef PORT_MOVABLE
#define D_8036BB18 PORT_VAR(D_8036BB18)
#define D_8036BB1C PORT_VAR(D_8036BB1C)
#define D_802154B2 PORT_VAR(D_802154B2)
#define D_802154B4 PORT_VAR(D_802154B4)
#define D_802154BC PORT_VAR(D_802154BC)
#define D_802154BE PORT_VAR(D_802154BE)
#define D_80215924 PORT_VAR(D_80215924)
#define D_802082FC PORT_VAR(D_802082FC)
#define D_80208314 PORT_VAR(D_80208314)
#define D_80208350 PORT_VAR(D_80208350)
#endif

enum { WHEEL_N = 33, WHEEL_STEP = 1986, WHEEL_SPIN = 2 * WHEEL_STEP };

/* the name entry is up: the menu's screen (D_8036BB18) is 0xB and its
   state (D_8036BB1C) isn't 1, which is when 17990.c runs func_801EAA7C;
   it takes the pad in state 2 */
static int name_screen(void) {
    return (int16_t)port_g16(D_8036BB18) == 0xB && (int16_t)port_g16(D_8036BB1C) != 1;
}

/* the keyboard types (there, and not in a replay or a deterministic run) */
static int keys_type(void) {
    return sdl_up && !host_replay_active() && !host_is_deterministic() && name_screen();
}

/* where c is on the wheel (the game's own tables, read as func_801EAA7C
   reads them), or -1; 0x7F is the delete entry */
static int wheel_index(int c) {
    uint32_t n0 = port_g32(D_80208350);
    for (int i = 0; i < WHEEL_N; i++) {
        int w = (uint32_t)i < n0 ? (uint8_t)D_80208314[i] : (uint8_t)D_802082FC[i - n0];
        if (w == c && w != 0x7F)
            return i;
    }
    return -1;
}

static char type_q[64];             /* typed, not yet in the name: '\b' Backspace, 0x1B Escape */
static unsigned type_head, type_n;
static enum { TY_IDLE, TY_SPIN, TY_PRESS, TY_WAIT } type_state;
static int type_idx, type_reads, type_seen, type_len0;
static uint16_t type_button;
static int type_env;                /* PORT_TYPE was queued: 1, 2 if it had something */

static void type_push(int c) {
    if (type_n < sizeof type_q)
        type_q[(type_head + type_n++) % sizeof type_q] = (char)c;
}

static void type_pop(void) {
    type_head = (type_head + 1) % sizeof type_q;
    type_n--;
    type_state = TY_IDLE;
}

/* SDL's key and text events (host_video_frame) */
static void type_event(const SDL_Event *e) {
    if ((e->type != SDL_TEXTINPUT && e->type != SDL_KEYDOWN) || !keys_type())
        return;
    if (e->type == SDL_TEXTINPUT) {
        for (const char *p = e->text.text; *p; p++)
            if ((unsigned char)*p > 0x20 && (unsigned char)*p < 0x7F)
                type_push(toupper((unsigned char)*p));
    } else if (e->key.keysym.sym == SDLK_BACKSPACE) {
        type_push('\b');
    } else if (e->key.keysym.sym == SDLK_ESCAPE && !e->key.repeat) {
        type_push(0x1B);
    }
}

/* this read's buttons while typing: 1 if the typing has the pad (*b is
   all that's pressed, the stick is still), 0 if not */
static int type_poll(uint16_t *b) {
    *b = 0;
    if (!name_screen()) {
        type_n = 0;
        type_state = TY_IDLE;
        return 0;
    }
    if (keys_type() && !SDL_IsTextInputActive())
        SDL_StartTextInput();
    if (!type_env) {
        const char *s = getenv("PORT_TYPE");
        type_env = s && *s ? 2 : 1;
        for (; s && *s; s++)
            type_push(*s == '<' ? '\b' : toupper((unsigned char)*s));    /* '<': Backspace */
    }
    if (type_state == TY_IDLE && !type_n)
        return 0;
    if ((int16_t)port_g16(D_8036BB1C) != 2)         /* still coming in */
        return 1;
    int busy = port_g16(D_802154BE) != 0;           /* a character on its way into the name */
    int len = (uint8_t)D_802154BC[0];
    int c = (uint8_t)type_q[type_head];
    switch (type_state) {
    case TY_IDLE:
        if (busy)
            return 1;
        type_reads = type_seen = 0;
        type_len0 = len;
        if (c == '\b' || c == 0x1B) {
            if (c == '\b' && !len) {                /* nothing to delete: don't leave */
                type_pop();
                return 1;
            }
            type_button = B_B;
            type_state = TY_PRESS;
            break;
        }
        type_idx = wheel_index(c);
        if (type_idx < 0 || len >= (uint8_t)D_80215924[0]) {    /* not on the wheel, or the name is full */
            type_pop();
            return 1;
        }
        type_button = B_A;
        type_state = TY_SPIN;
        /* fall through */
    case TY_SPIN: {
        /* turn the wheel (its angle D_802154B4, and D_802154B2, the one
           it eases to) by up to WHEEL_SPIN a read, the short way round;
           there, A: func_801EAA7C works out the character in front
           (D_802154B6) from the angle before it looks at the pad */
        int16_t target = (int16_t)(0x7FFF - type_idx * WHEEL_STEP);
        int16_t at = (int16_t)port_g16(D_802154B4);
        int d = (int16_t)(target - at);
        port_wg16(D_802154B2, (uint16_t)target);
        if (d > WHEEL_SPIN || d < -WHEEL_SPIN) {
            port_wg16(D_802154B4, (uint16_t)(at + (d > 0 ? WHEEL_SPIN : -WHEEL_SPIN)));
            return 1;
        }
        port_wg16(D_802154B4, (uint16_t)target);
        type_state = TY_PRESS;
        break;
    }
    case TY_PRESS:                                  /* released: the press was one read */
        type_state = TY_WAIT;
        return 1;
    case TY_WAIT:
        if (busy || len != type_len0)
            type_seen = 1;
        if (type_seen && !busy)
            type_pop();
        else if (!type_seen && ++type_reads > 10)   /* the game didn't take it: again */
            type_state = TY_IDLE;
        return 1;
    }
    *b = type_button;
    return 1;
}

static void input_read(int n, uint16_t *buttons, int8_t *x, int8_t *y) {
    uint16_t b = 0, tb = 0;
    int sx = 0, sy = 0, typing = 0;
    if (n == 0 && !host_replay_active())
        typing = type_poll(&tb);
    if (n == 0 && host_replay_active()) {
        host_replay_pad(&b, &sx, &sy);
    } else if (n == 0 && !sdl_up) {
        b |= scripted_buttons(&sy);
    } else if (n == 0) {
        int nk = 0;
        const Uint8 *k = SDL_GetKeyboardState(&nk);
        static Uint8 kt[SDL_NUM_SCANCODES];
        if (keys_type() && nk <= SDL_NUM_SCANCODES) {   /* the letters and digits type there */
            memcpy(kt, k, (size_t)nk);
            memset(kt + SDL_SCANCODE_A, 0, SDL_SCANCODE_0 - SDL_SCANCODE_A + 1);
            k = kt;
        }
        if (k[SDL_SCANCODE_X]) b |= B_A;
        if (k[SDL_SCANCODE_C]) b |= B_B;
        if (k[SDL_SCANCODE_Z]) b |= B_Z;
        if (k[SDL_SCANCODE_RETURN]) b |= B_START;
        if (k[SDL_SCANCODE_Q]) b |= B_L;
        if (k[SDL_SCANCODE_E]) b |= B_R;
        if (k[SDL_SCANCODE_I]) b |= B_CU;
        if (k[SDL_SCANCODE_K]) b |= B_CD;
        if (k[SDL_SCANCODE_J]) b |= B_CL;
        if (k[SDL_SCANCODE_L]) b |= B_CR;
        if (k[SDL_SCANCODE_T]) b |= B_DU;
        if (k[SDL_SCANCODE_G]) b |= B_DD;
        if (k[SDL_SCANCODE_F]) b |= B_DL;
        if (k[SDL_SCANCODE_H]) b |= B_DR;
        if (k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_W]) sy += 80;
        if (k[SDL_SCANCODE_DOWN] || k[SDL_SCANCODE_S]) sy -= 80;
        if (k[SDL_SCANCODE_LEFT] || k[SDL_SCANCODE_A]) sx -= 80;
        if (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D]) sx += 80;
        b |= scripted_buttons(&sy);
        if (pad) {
            struct { int btn; uint16_t bit; } map[] = {
                { SDL_CONTROLLER_BUTTON_A, B_A }, { SDL_CONTROLLER_BUTTON_B, B_B },
                { SDL_CONTROLLER_BUTTON_START, B_START }, { SDL_CONTROLLER_BUTTON_LEFTSHOULDER, B_L },
                { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, B_R }, { SDL_CONTROLLER_BUTTON_DPAD_UP, B_DU },
                { SDL_CONTROLLER_BUTTON_DPAD_DOWN, B_DD }, { SDL_CONTROLLER_BUTTON_DPAD_LEFT, B_DL },
                { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, B_DR }, { SDL_CONTROLLER_BUTTON_X, B_CL },
                { SDL_CONTROLLER_BUTTON_Y, B_CU },
            };
            for (unsigned i = 0; i < sizeof map / sizeof map[0]; i++)
                if (SDL_GameControllerGetButton(pad, map[i].btn))
                    b |= map[i].bit;
            if (SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000) b |= B_Z;
            int ax = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
            int ay = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
            if (ax > 4000 || ax < -4000) sx = ax * 80 / 32767;
            if (ay > 4000 || ay < -4000) sy = -ay * 80 / 32767;
            int cx = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX);
            int cy = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY);
            if (cx > 16000) b |= B_CR;
            if (cx < -16000) b |= B_CL;
            if (cy > 16000) b |= B_CD;
            if (cy < -16000) b |= B_CU;
        }
    }
    if (typing) {
        b = tb;
        sx = sy = 0;
    } else if (type_env == 2 && name_screen()) {
        b &= ~(B_A | B_B);          /* PORT_TYPE's name, as typed: PORT_AUTOSTART only confirms it */
    }
    port_wbe16(buttons, b);         /* N64-side memory: big-endian */
    *x = (int8_t)sx;
    *y = (int8_t)sy;
}

/* SDL's keyboard and controllers on the loop's OS thread (fiber.h) */
struct input_call { int n; uint16_t *buttons; int8_t *x, *y; };
static void input_call(void *p) {
    struct input_call *c = p;
    input_read(c->n, c->buttons, c->x, c->y);
}

void host_input(int n, uint16_t *buttons, int8_t *x, int8_t *y) {
    struct input_call c = { n, buttons, x, y };
    fiber_call_on_loop(input_call, &c);
}

/* ---- the rest of the "hardware" (stubs for now) --------------------------- */

const char *host_save_path = "blastcorps.eep";
static uint8_t eeprom[512];
static int eeprom_loaded;

static void eeprom_load(void) {
    if (eeprom_loaded)
        return;
    eeprom_loaded = 1;
    FILE *f = fopen(host_save_path, "rb");
    if (f) {
        if (fread(eeprom, 1, sizeof eeprom, f) != sizeof eeprom)
            memset(eeprom, 0, sizeof eeprom);
        fclose(f);
    }
}

void host_eeprom_read(int block, uint32_t dst) {
    eeprom_load();
    if (block >= 0 && block < 64) {
        memcpy(port_ptr(dst), eeprom + block * 8, 8);
        host_save_order(port_ptr(dst), block * 8, 8, 0);    /* the file keeps the N64's bytes */
    }
}

void host_eeprom_write(int block, uint32_t src) {
    eeprom_load();
    if (block < 0 || block >= 64)
        return;
    memcpy(eeprom + block * 8, port_ptr(src), 8);
    host_save_order(eeprom + block * 8, block * 8, 8, 0);
    FILE *f = fopen(host_save_path, "wb");
    if (f) {
        fwrite(eeprom, 1, sizeof eeprom, f);
        fclose(f);
    }
#ifdef PORT_WASM_WEB
    /* the page's IndexedDB copy of the file (port/web/shell.html) */
    EM_ASM(if (Module.syncSave) Module.syncSave(););
#endif
}

