/*
 * The part of SDL2's interface the host uses (video.c, ui.c, audio.c), for
 * a build without SDL (-DPORT_SDL=OFF: a cross build for a target that has
 * none, run under qemu; docs/PORT.md, "Other hosts").  The types and
 * constants are SDL's; every call fails or does nothing, SDL_Init first, so
 * the host takes its headless path (video.c): no window, the software
 * renderer, the screenshots, --wav and the save to files, no sound device,
 * no input but the replay's and PORT_AUTOSTART's.  A host with a window of
 * its own (a console's) has its own video, input and audio glue instead.
 */
#ifndef PORT_NOSDL_SDL_H
#define PORT_NOSDL_SDL_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef int64_t Sint64;
typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

#define SDL_INIT_AUDIO 0x00000010u
#define SDL_INIT_VIDEO 0x00000020u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS 0x00004000u
#define SDL_HINT_EMSCRIPTEN_ASYNCIFY "SDL_EMSCRIPTEN_ASYNCIFY"
#define SDL_DISABLE 0
#define SDL_ENABLE 1

static inline const char *SDL_GetError(void) { return "built without SDL (-DPORT_SDL=OFF): --headless only"; }
static inline int SDL_Init(Uint32 flags) { (void)flags; return -1; }
static inline int SDL_InitSubSystem(Uint32 flags) { (void)flags; return -1; }
static inline void SDL_Quit(void) {}
static inline int SDL_setenv(const char *name, const char *value, int overwrite) {
    (void)name; (void)value; (void)overwrite;
    return 0;
}
static inline SDL_bool SDL_SetHint(const char *name, const char *value) { (void)name; (void)value; return SDL_FALSE; }
static inline Uint32 SDL_GetTicks(void) { return 0; }

/* ---- video ---- */
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
typedef struct SDL_DisplayMode { Uint32 format; int w, h, refresh_rate; void *driverdata; } SDL_DisplayMode;
#define SDL_WINDOWPOS_CENTERED 0x2FFF0000
#define SDL_WINDOW_RESIZABLE 0x00000020u
#define SDL_RENDERER_SOFTWARE 0x00000001u
#define SDL_PIXELFORMAT_ARGB8888 0x16362004u
#define SDL_TEXTUREACCESS_STREAMING 1
static inline int SDL_GetCurrentDisplayMode(int i, SDL_DisplayMode *m) { (void)i; (void)m; return -1; }
static inline SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f) {
    (void)t; (void)x; (void)y; (void)w; (void)h; (void)f;
    return NULL;
}
static inline void SDL_GetWindowSize(SDL_Window *w, int *ww, int *wh) { (void)w; *ww = *wh = 0; }
static inline void SDL_SetWindowTitle(SDL_Window *w, const char *t) { (void)w; (void)t; }
static inline SDL_Renderer *SDL_CreateRenderer(SDL_Window *w, int i, Uint32 f) { (void)w; (void)i; (void)f; return NULL; }
static inline SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, Uint32 f, int a, int w, int h) {
    (void)r; (void)f; (void)a; (void)w; (void)h;
    return NULL;
}
static inline void SDL_DestroyTexture(SDL_Texture *t) { (void)t; }
static inline int SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) { (void)r; (void)w; (void)h; return -1; }
static inline int SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *r, const void *p, int pitch) {
    (void)t; (void)r; (void)p; (void)pitch;
    return -1;
}
static inline int SDL_RenderClear(SDL_Renderer *r) { (void)r; return -1; }
static inline int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s, const SDL_Rect *d) {
    (void)r; (void)t; (void)s; (void)d;
    return -1;
}
static inline void SDL_RenderPresent(SDL_Renderer *r) { (void)r; }
static inline void SDL_GL_SwapWindow(SDL_Window *w) { (void)w; }

/* ---- input ---- */
typedef Sint64 SDL_FingerID;
typedef Sint32 SDL_JoystickID;
typedef struct SDL_GameController SDL_GameController;
#define SDL_TOUCH_MOUSEID ((Uint32)-1)
#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_RIGHT 3
enum {
    SDL_QUIT = 0x100, SDL_KEYDOWN = 0x300, SDL_KEYUP, SDL_TEXTEDITING, SDL_TEXTINPUT,
    SDL_MOUSEMOTION = 0x400, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP, SDL_MOUSEWHEEL,
    SDL_CONTROLLERDEVICEADDED = 0x653,
    SDL_FINGERDOWN = 0x700, SDL_FINGERUP, SDL_FINGERMOTION
};
typedef enum {
    SDL_SCANCODE_A = 4, SDL_SCANCODE_C = 6, SDL_SCANCODE_D = 7, SDL_SCANCODE_E = 8, SDL_SCANCODE_F = 9,
    SDL_SCANCODE_I = 12, SDL_SCANCODE_J = 13, SDL_SCANCODE_K = 14, SDL_SCANCODE_L = 15,
    SDL_SCANCODE_Q = 20, SDL_SCANCODE_R = 21, SDL_SCANCODE_S = 22, SDL_SCANCODE_W = 26,
    SDL_SCANCODE_X = 27, SDL_SCANCODE_Z = 29, SDL_SCANCODE_RETURN = 40, SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42, SDL_SCANCODE_SPACE = 44, SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80, SDL_SCANCODE_DOWN = 81, SDL_SCANCODE_UP = 82, SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_LSHIFT = 225, SDL_SCANCODE_RSHIFT = 229, SDL_NUM_SCANCODES = 512
} SDL_Scancode;
#define SDLK_BACKSPACE 8
#define SDLK_ESCAPE 27
typedef struct SDL_Keysym { SDL_Scancode scancode; Sint32 sym; Uint16 mod; Uint32 unused; } SDL_Keysym;
typedef union SDL_Event {
    Uint32 type;
    struct { Uint32 type, timestamp, windowID; Uint8 state, repeat, padding2, padding3; SDL_Keysym keysym; } key;
    struct { Uint32 type, timestamp, windowID; char text[32]; } text;
    struct { Uint32 type, timestamp, windowID, which, state; Sint32 x, y, xrel, yrel; } motion;
    struct { Uint32 type, timestamp, windowID, which; Uint8 button, state, clicks, padding1; Sint32 x, y; } button;
    struct { Uint32 type, timestamp, windowID, which; Sint32 x, y; Uint32 direction; } wheel;
    struct { Uint32 type, timestamp; Sint32 which; } cdevice;
    struct { Uint32 type, timestamp; Sint64 touchId; SDL_FingerID fingerId; float x, y, dx, dy, pressure; } tfinger;
    Uint8 padding[56];
} SDL_Event;
static inline int SDL_PollEvent(SDL_Event *e) { (void)e; return 0; }
static inline const Uint8 *SDL_GetKeyboardState(int *n) { (void)n; return NULL; }
static inline Uint32 SDL_GetMouseState(int *x, int *y) { if (x) *x = 0; if (y) *y = 0; return 0; }
static inline int SDL_ShowCursor(int toggle) { (void)toggle; return 0; }
static inline int SDL_SetRelativeMouseMode(SDL_bool on) { (void)on; return -1; }
static inline void SDL_StartTextInput(void) {}
static inline SDL_bool SDL_IsTextInputActive(void) { return SDL_FALSE; }
static inline int SDL_NumJoysticks(void) { return 0; }
static inline SDL_bool SDL_IsGameController(int i) { (void)i; return SDL_FALSE; }
static inline SDL_GameController *SDL_GameControllerOpen(int i) { (void)i; return NULL; }
typedef enum {
    SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_GUIDE, SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT
} SDL_GameControllerButton;
typedef enum {
    SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY, SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT
} SDL_GameControllerAxis;
static inline Uint8 SDL_GameControllerGetButton(SDL_GameController *c, SDL_GameControllerButton b) {
    (void)c; (void)b;
    return 0;
}
static inline short SDL_GameControllerGetAxis(SDL_GameController *c, SDL_GameControllerAxis a) {
    (void)c; (void)a;
    return 0;
}

/* ---- audio ---- */
typedef Uint32 SDL_AudioDeviceID;
typedef Uint16 SDL_AudioFormat;
#define AUDIO_S16SYS 0x8010
typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels, silence;
    Uint16 samples, padding;
    Uint32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;
static inline SDL_AudioDeviceID SDL_OpenAudioDevice(const char *n, int c, const SDL_AudioSpec *w, SDL_AudioSpec *h,
                                                    int a) {
    (void)n; (void)c; (void)w; (void)h; (void)a;
    return 0;
}
static inline void SDL_CloseAudioDevice(SDL_AudioDeviceID d) { (void)d; }
static inline void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int p) { (void)d; (void)p; }
static inline int SDL_QueueAudio(SDL_AudioDeviceID d, const void *p, Uint32 n) { (void)d; (void)p; (void)n; return -1; }
static inline Uint32 SDL_GetQueuedAudioSize(SDL_AudioDeviceID d) { (void)d; return 0; }
static inline void SDL_ClearQueuedAudio(SDL_AudioDeviceID d) { (void)d; }

#endif
