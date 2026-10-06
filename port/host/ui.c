/*
 * The keyboard, the mouse and touch, by what is on the screen
 * (docs/PORT.md, "Keyboard, mouse and touch").
 *
 * All of it is the host's, made into the pad the game reads, as the name
 * entry's typing is (video.c): the game's C only has the world map's
 * camera hook (port_globe_view) and a level's (port_camera_turn, the free
 * camera: the mouse and the right stick).  --replay never takes any of it, and
 * --deterministic runs only PORT_POINTER's script.
 *
 * What is on the screen (ui_context):
 * - UI_DRIVE: a level running (mode 4, or the carrier's overview 0x100,
 *   and not paused): W/S are the pedals (A and B, which is what the game
 *   accelerates and brakes with in its default control mode), or the
 *   stick's Y where the game reads that instead (on foot, the Thunderfist,
 *   the Cyclone Suit and the J-Bomb in its default 360-degree modes, and
 *   any vehicle switched to "speed on 3D stick"); A/D steer; Space is Z
 *   (brake, and held when stopped, get out); Shift is R (the vehicle's
 *   special: rams, boost, missiles, roll, jets, siren); Q/E turn the
 *   camera (C-left/C-right), R/F and the wheel zoom (C-up/C-down); Escape
 *   and Enter are Start.
 * - UI_MENU: one of the game's windows taking input (yoshi.c, 26570.c),
 *   in the front end or paused in a level: the arrows and WASD move,
 *   Enter and Space are A, Escape and Backspace B; the mouse's pointer
 *   picks an entry, a click takes it, the right button is B.
 * - UI_MAP: the world map (mode 0x4000): keys as in a menu; a click on a
 *   level picks it, a click on the one picked goes in (A), a drag turns
 *   the globe.  Touch is the same, with the finger.
 * - UI_NAME: the name entry: video.c types; a click on a character of the
 *   wheel types it, a click inside the wheel (on the name) confirms it
 *   (Start), the right button is Escape.
 * - UI_OTHER: anything else (the title, the briefings): the arrows and
 *   WASD are the stick, Enter Start, Space A, Escape B, Shift R; a click
 *   is A.
 * X, C and Z are A, B and Z, and I, J, K, L the C buttons, everywhere but
 * the name entry, as before.
 *
 * A key that is a button keeps the button it was given when it went down
 * until it comes up (Enter pressed as the world map's A doesn't become
 * the level's Start because the level started meanwhile), and is held for
 * a read at least.  The arrows and WASD follow what is on the screen.
 */
#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "gfx.h"

#define V(s) PORT_VAR(s)

/* (us.v11's names, docs/PORT.md has what they are) */
extern char D_80364A90[], D_802E8BD0[], D_8036BB18[], D_8036BB1C[], D_8036BB30[], D_8036BB3C[], D_8036BB3E[],
    D_802F8BDC[], D_8020C070[], D_802F5804[], D_802F49F4[], D_80364456[], D_80364AF0[], D_80364AE8[],
    D_8036C778[], D_80358060[], D_8035805C[], D_8035807C[], D_803156F8[], D_80217B6C[], D_80217B70[],
    D_8020D810[], D_8021A905[], D_8021A907[], D_8021A90C[], D_8021A910[], D_8021A914[], D_8021A91C[],
    D_8021A920[], D_8021A924[], D_802E8F94[], D_8021AB2C[], D_802154B4[], D_80208350[], D_80208314[], D_802082FC[],
    D_802154BC[];

static uint8_t g8(const char *p) { return *(const uint8_t *)p; }
static float gf(const char *p) {
    uint32_t v = port_g32(p);
    float f;
    memcpy(&f, &v, 4);
    return f;
}
static uint64_t mode(void) {
    return (uint64_t)port_g32(V(D_80364A90)) << 32 | port_g32(V(D_80364A90) + 4);
}

/* ---- what is on the screen ------------------------------------------------ */

enum { UI_OTHER, UI_NAME, UI_MENU, UI_MAP, UI_DRIVE };

static char *yoshi_window(int w) { return V(D_802F8BDC) + w * 0x1C; }

/* the window taking input (yoshi.c's state 2, a window with input), or -1 */
static int menu_window(void) {
    int w = (int16_t)port_g16(V(D_8036BB18));
    if (w < 0 || w >= 0x6C || (int16_t)port_g16(V(D_8036BB1C)) != 2)
        return -1;
    return port_g32(yoshi_window(w) + 8) & 0x20 ? w : -1;
}

static int name_up(void) {
    return (int16_t)port_g16(V(D_8036BB18)) == 0xB && (int16_t)port_g16(V(D_8036BB1C)) != 1;
}

static int ui_context(void) {
    uint64_t m = mode();
    if (name_up())
        return UI_NAME;
    if (m == 0x0001000000000000)
        return UI_OTHER;                        /* the crew's introductions: a window with nothing to pick */
    if (menu_window() >= 0)
        return UI_MENU;
    if (m == 0x4000)
        return UI_MAP;
    if (m == 4 || m == 0x100)
        return g8(V(D_802E8BD0)) ? UI_MENU : UI_DRIVE;
    return UI_OTHER;
}

/* in a level, W/S press A/B: the game's pad code (45BB0.c, func_8028A470)
   drops the stick's Y and drives with A and B, unless the vehicle (or the
   driver) takes the stick as a direction (360-degree mode: types 0, 2, 9
   and 16 with their control-method bit clear) or the bit is set ("speed
   on 3D stick", PlayerInfo.unkF0) */
static int pedals(void) {
    int type = g8(V(D_80364456));
    int slot = g8(V(D_80364AE8)) & 3;
    int bit = type < 32 && (port_g32(V(D_80364AF0) + slot * 0x100 + 0xF0) >> type & 1);
    return !bit && type != 0 && type != 2 && type != 9 && type != 16;
}

/* ---- keys ----------------------------------------------------------------- */

static uint16_t key_held[SDL_NUM_SCANCODES];     /* the buttons a key went down as */
static uint8_t key_up[SDL_NUM_SCANCODES];        /* up again, not yet read */
static uint8_t key_seen[SDL_NUM_SCANCODES];      /* a read has had it */
static int ui_live;                              /* the keyboard and mouse are the player's */

/* --free-camera (below, "the free camera") */
int host_free_camera;
float host_camera_sens = 0.2f;
static struct {
    int held;                       /* the left button is down: the mouse turns
                                       the camera (SDL's relative mode is on) */
    float turn;                     /* degrees, not yet taken by the game */
    float stick;                    /* the right stick, -1..1 past its dead zone */
    float pitch;                    /* degrees more (down) than the game's own */
    float stick_y;                  /* the right stick's y, -1 (up)..1 */
    Uint32 last;                    /* the last read's time (ms) */
} cam;
static int cam_button(int down);

static uint16_t key_buttons(int sc, int ctx) {
    if (ctx == UI_NAME)                          /* the letters type there (video.c) */
        return sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_KP_ENTER ? B_START : 0;
    switch (sc) {
    case SDL_SCANCODE_X: return B_A;
    case SDL_SCANCODE_C: return B_B;
    case SDL_SCANCODE_Z: return B_Z;
    case SDL_SCANCODE_I: return B_CU;
    case SDL_SCANCODE_K: return B_CD;
    case SDL_SCANCODE_J: return B_CL;
    case SDL_SCANCODE_L: return B_CR;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        return ctx == UI_MENU || ctx == UI_MAP ? B_A : B_START;
    case SDL_SCANCODE_SPACE: return ctx == UI_DRIVE ? B_Z : B_A;
    case SDL_SCANCODE_ESCAPE: return ctx == UI_DRIVE ? B_START : B_B;
    case SDL_SCANCODE_BACKSPACE: return ctx == UI_DRIVE ? 0 : B_B;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        return ctx == UI_DRIVE || ctx == UI_OTHER ? B_R : 0;
    }
    if (ctx != UI_DRIVE)
        return 0;
    switch (sc) {
    case SDL_SCANCODE_Q: return B_CL;
    case SDL_SCANCODE_E: return B_CR;
    case SDL_SCANCODE_R: return B_CU;
    case SDL_SCANCODE_F: return B_CD;
    }
    return 0;
}

/* ---- the pointer: the mouse, or a finger ---------------------------------- */

/* where on the game's 320x240 screen (video.c: the picture in the window) */
static struct {
    int down, drag, moved;
    SDL_FingerID finger;            /* -1: the mouse */
    float x0, y0, x, y;             /* N64 pixels: where it went down, where it is */
    int hover;                      /* the mouse moved (no button): x, y is a place to point at */
} ptr;

enum { EV_TAP, EV_BACK, EV_WHEEL_UP, EV_WHEEL_DOWN };
static struct { int kind; float x, y; } evq[16];
static unsigned evq_n;

static void ev_push(int kind, float x, float y) {
    if (evq_n < sizeof evq / sizeof evq[0])
        evq[evq_n++] = (__typeof__(evq[0])){ kind, x, y };
}

static void globe_drag_start(void);
static void globe_drag_end(void);

static void ptr_down(SDL_FingerID id, float x, float y) {
    if (ptr.down)
        return;
    ptr.down = 1;
    ptr.drag = 0;
    ptr.finger = id;
    ptr.x0 = ptr.x = x;
    ptr.y0 = ptr.y = y;
    ptr.hover = 0;
    globe_drag_start();
}

static void ptr_move(SDL_FingerID id, float x, float y) {
    if (!ptr.down) {
        if (id < 0) {
            ptr.x = x;
            ptr.y = y;
            ptr.hover = ptr.moved = 1;
        }
        return;
    }
    if (id != ptr.finger)
        return;
    ptr.x = x;
    ptr.y = y;
    if (!ptr.drag && (fabsf(x - ptr.x0) > 4 || fabsf(y - ptr.y0) > 4))
        ptr.drag = 1;
}

static void ptr_up(SDL_FingerID id, float x, float y) {
    if (!ptr.down || id != ptr.finger)
        return;
    ptr.down = 0;
    ptr.x = x;
    ptr.y = y;
    if (ptr.drag)
        globe_drag_end();
    else
        ev_push(EV_TAP, ptr.x0, ptr.y0);
    ptr.drag = 0;
}

void host_ui_event(const SDL_Event *e) {
    if (!ui_live)
        return;
    float x, y;
    switch (e->type) {
    case SDL_KEYDOWN:
        if (!e->key.repeat && e->key.keysym.scancode < SDL_NUM_SCANCODES) {
            int sc = e->key.keysym.scancode;
            key_held[sc] = key_buttons(sc, ui_context());
            key_up[sc] = key_seen[sc] = 0;
        }
        break;
    case SDL_KEYUP:
        if (e->key.keysym.scancode < SDL_NUM_SCANCODES) {
            int sc = e->key.keysym.scancode;
            if (key_seen[sc])
                key_held[sc] = 0;
            else
                key_up[sc] = 1;                 /* a read has it first */
        }
        break;
    /* (SDL makes the mouse's events from touch too, as SDL_TOUCH_MOUSEID:
       the fingers are taken as themselves) */
    case SDL_MOUSEMOTION:
        if (cam.held && e->motion.which != SDL_TOUCH_MOUSEID) {
            cam.turn += e->motion.xrel * host_camera_sens;
            cam.pitch += e->motion.yrel * host_camera_sens;
            break;
        }
        if (e->motion.which != SDL_TOUCH_MOUSEID && host_window_to_n64(-1, -1, &x, &y))
            ptr_move(-1, x, y);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        if (e->button.which != SDL_TOUCH_MOUSEID && e->button.button == SDL_BUTTON_LEFT &&
            cam_button(e->type == SDL_MOUSEBUTTONDOWN))
            break;
        if (e->button.which == SDL_TOUCH_MOUSEID || !host_window_to_n64(-1, -1, &x, &y))
            break;
        if (e->button.button == SDL_BUTTON_LEFT) {
            if (e->type == SDL_MOUSEBUTTONDOWN)
                ptr_down(-1, x, y);
            else
                ptr_up(-1, x, y);
        } else if (e->button.button == SDL_BUTTON_RIGHT && e->type == SDL_MOUSEBUTTONDOWN) {
            ev_push(EV_BACK, x, y);
        }
        break;
    case SDL_MOUSEWHEEL:
        if (e->wheel.which != SDL_TOUCH_MOUSEID && e->wheel.y)
            ev_push(e->wheel.y > 0 ? EV_WHEEL_UP : EV_WHEEL_DOWN, 0, 0);
        break;
    case SDL_FINGERDOWN:
    case SDL_FINGERMOTION:
    case SDL_FINGERUP:
        if (!host_window_to_n64(e->tfinger.x, e->tfinger.y, &x, &y))
            break;
        if (e->type == SDL_FINGERDOWN)
            ptr_down(e->tfinger.fingerId, x, y);
        else if (e->type == SDL_FINGERMOTION)
            ptr_move(e->tfinger.fingerId, x, y);
        else
            ptr_up(e->tfinger.fingerId, x, y);
        break;
    }
}

/* PORT_POINTER=READ:WHAT:X:Y,... : the pointer at the READth controller
   read (from 1) does WHAT at X, Y on the 320x240 screen: d down, m move
   (with no button down: the mouse pointing), u up, b the right button
   (back), w/W the wheel up/down, t the mouse dragged X pixels across and
   Y down in a level with --free-camera; for tests, in any run */
static void script_poll(unsigned read) {
    static const char *s;
    static int init;
    if (!init) {
        init = 1;
        s = getenv("PORT_POINTER");
    }
    while (s && *s) {
        unsigned r;
        char what;
        float x, y;
        int n = 0;
        if (sscanf(s, "%u:%c:%f:%f%n", &r, &what, &x, &y, &n) != 4 || !n) {
            host_log("PORT_POINTER: can't read \"%s\"\n", s);
            s = NULL;
            return;
        }
        if (r > read)
            return;
        s += n;
        if (*s == ',')
            s++;
        switch (what) {
        case 'd': ptr_down(-2, x, y); break;
        case 'm': ptr_move(ptr.down ? -2 : -1, x, y); break;
        case 'u': ptr_up(-2, x, y); break;
        case 'b': ev_push(EV_BACK, x, y); break;
        case 'w': ev_push(EV_WHEEL_UP, x, y); break;
        case 'W': ev_push(EV_WHEEL_DOWN, x, y); break;
        case 't':
            cam.turn += x * host_camera_sens;
            cam.pitch += y * host_camera_sens;
            break;
        }
    }
}

/* ---- buttons for a read: one press, released the read after ---------------- */

static uint16_t pulse_q[8];
static unsigned pulse_n;

static void pulse(uint16_t b) {
    if (pulse_n < sizeof pulse_q / sizeof pulse_q[0])
        pulse_q[pulse_n++] = b;
}

/* this read's press, if one is due (a read without it between two) */
static uint16_t pulse_take(uint16_t last_out) {
    if (!pulse_n || (last_out & pulse_q[0]))
        return 0;
    uint16_t b = pulse_q[0];
    memmove(pulse_q, pulse_q + 1, --pulse_n * sizeof pulse_q[0]);
    return b;
}

/* ---- the game's windows --------------------------------------------------- */

/* the entry of window w at (x, y), or -1: the selectable ones, as yoshi.c
   draws them (26570.c: the window at unk4, unk6, an entry's text at its x,
   y in it, and the ones that scroll by D_8036BB30; the text's cells unk6
   wide, 0.6 of that apart).  A list's entries take its whole width. */
static int menu_hit(int w, float x, float y) {
    char *win = yoshi_window(w);
    uint32_t wflags = port_g32(win + 8);
    if (wflags & 0x8000)                        /* a list the front end sorts (D_8036BB24) */
        return -1;
    char *table = wflags & 0x800 ? V(D_8020C070) : V(D_802F5804);
    int wx = (int16_t)port_g16(win + 4), wy = (int16_t)port_g16(win + 6);
    int ww = port_g16(win), wh = port_g16(win + 2);
    int first = port_g16(win + 0xE), count = port_g16(win + 0x10);
    int scroll = (int32_t)port_g32(V(D_8036BB30));
    int best = -1;
    float best_d = 6;
    for (int i = first; i < first + count; i++) {
        char *e = table + i * 0x1C;
        int ef = port_g16(e);
        if (!(ef & 1) || (ef & 0x800))
            continue;
        float ex = wx + (int16_t)port_g16(e + 2), ey = wy + (int16_t)port_g16(e + 4);
        float cw = port_g16(e + 6), chh = port_g16(e + 8), ew;
        if (ef & 0x1000) {
            ey += scroll;
            if (fabsf(ey - (wy + wh / 2.0f)) >= wh / 3.0f)  /* faded out */
                continue;
        }
        if (ef & 0x400) {                       /* an icon (YoshiIcon's offset; about 64x32) */
            char *icon = V(D_802F49F4) + g8(e + 0x14) * 0x30;
            ex += (int16_t)port_g16(icon);
            ey += (int16_t)port_g16(icon + 2);
            ew = 64;
            chh = 32;
        } else {
            uint32_t t = port_g32(e + 0xC);
            int len = 0;
            if (t - 0x80000000u < 0x00800000u)
                for (const char *s = port_ptr(t); len < 64 && s[len]; len++)
                    ;
            ew = len ? (0.6f * (len - 1) + 1) * cw : 4 * cw;
        }
        if (!(wflags & 0x40)) {                 /* a list: the window's width */
            ex = wx < ex ? wx : ex;
            ew = ww > ew ? ww : ew;
        }
        float dx = x < ex ? ex - x : x > ex + ew ? x - ex - ew : 0;
        float dy = y < ey ? ey - y : y > ey + chh ? y - ey - chh : 0;
        float d = dx > dy ? dx : dy;
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

static struct {
    int window, target, press, reads;
} menu = { -1, -1 };

static uint16_t menu_poll(uint16_t last_out) {
    int w = menu_window();
    if (w != menu.window) {
        menu.window = w;
        menu.target = -1;
        menu.press = 0;
    }
    if (w < 0)
        return 0;
    char *win = yoshi_window(w);
    for (unsigned i = 0; i < evq_n; i++) {
        if (evq[i].kind == EV_BACK) {
            pulse(B_B);
            continue;
        }
        if (evq[i].kind != EV_TAP)
            continue;
        int hit = menu_hit(w, evq[i].x, evq[i].y);
        if (hit >= 0) {
            menu.target = hit;
            menu.press = 1;
            menu.reads = 0;
        } else if (!(port_g32(win + 8) & 0x10000000)) {
            pulse(B_A);                         /* a message box: on to the next page */
        }
    }
    if (ptr.hover && ptr.moved) {               /* the mouse pointing: the entry under it */
        ptr.moved = 0;
        int hit = menu_hit(w, ptr.x, ptr.y);
        if (hit >= 0 && !menu.press) {
            menu.target = hit;
            menu.reads = 0;
        }
    }
    if (menu.target < 0)
        return 0;
    int sel = port_g16(win + 0x18);
    if (host_verbose > 1)
        host_log("ui: window %#x: entry %d, to %d%s\n", w, sel, menu.target, menu.press ? ", then A" : "");
    if (++menu.reads > 90) {                    /* it doesn't get there */
        menu.target = -1;
        menu.press = 0;
        return 0;
    }
    if (sel == menu.target) {
        if (menu.press)
            pulse(B_A);
        menu.target = -1;
        menu.press = 0;
        return 0;
    }
    /* the window's own previous and next (the D-pad, as yoshi.c set them up
       for a list or a row), a press every other read: its sound, its
       scrolling as for the pad */
    uint16_t step = menu.target > sel ? port_g16(V(D_8036BB3E)) : port_g16(V(D_8036BB3C));
    return last_out & step ? 0 : step;
}

/* ---- the name entry: the wheel's characters -------------------------------- */

static uint16_t name_poll(void) {
    uint16_t b = 0;
    for (unsigned i = 0; i < evq_n; i++) {
        if (evq[i].kind == EV_BACK) {
            host_type_char(0x1B);
            continue;
        }
        if (evq[i].kind != EV_TAP)
            continue;
        /* the wheel (1C40.c, func_801EAA7C): character i at the angle
           D_802154B4 + i * 1986 - 0x6002, 120 by 96 around (160, 120), a
           24x20 cell from 7 left and 10 up of there */
        int16_t at = (int16_t)port_g16(V(D_802154B4));
        uint32_t n0 = port_g32(V(D_80208350));
        int best = -1;
        float best_d = 16 * 16;
        for (int k = 0; k < 33; k++) {
            float a = (int16_t)(at + k * 1986 - 0x6002) * (float)M_PI / 32768;
            float cx = 160 + sinf(a) * 120 - 7 + 12, cy = 120 + cosf(a) * 96 - 10 + 10;
            float d = (evq[i].x - cx) * (evq[i].x - cx) + (evq[i].y - cy) * (evq[i].y - cy);
            if (d < best_d) {
                best_d = d;
                best = k;
            }
        }
        if (best >= 0) {
            int c = (uint32_t)best < n0 ? g8(V(D_80208314) + best) : g8(V(D_802082FC) + best - n0);
            host_type_char(c == 0x7F ? '\b' : c);
        } else {
            float dx = (evq[i].x - 160) / 120, dy = (evq[i].y - 120) / 96;
            if (dx * dx + dy * dy < 0.5f && g8(V(D_802154BC)))
                b = B_START;                    /* inside the wheel, where the name is: that's it */
        }
    }
    return b;
}

/* ---- the world map --------------------------------------------------------- */

/* a level's place on the screen, as the game's func_8027690C finds the
   paths' (hd_code 30C70.c): the level's point on the globe (D_8020D810's
   unk24..2C, radius 250) through the frame's matrices for Earth (11530.c
   passes them).  The frame the game drew last: D_8035805C has gone on to
   the next one by the time it reads the pad. */
static void mtx_read(const char *m, float f[4][4]) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            uint32_t v = (uint32_t)port_g16_of32(m, i * 4 + j) << 16 | port_g16_of32(m, 16 + i * 4 + j);
            f[i][j] = (int32_t)v / 65536.0f;
        }
}

static void mtx_xf(const char *m, float v[4]) {
    float f[4][4], o[4];
    mtx_read(m, f);
    for (int j = 0; j < 4; j++)
        o[j] = f[0][j] * v[0] + f[1][j] * v[1] + f[2][j] * v[2] + f[3][j];
    memcpy(v, o, sizeof o);
}

static int globe_project(float x, float y, float z, float *sx, float *sy) {
    int fb = g8(V(D_8035805C)) ^ 1;
    char *frame = V(D_803156F8) + fb * 0x21498;
    float v[4] = { x, y, z, 1 };
    mtx_xf(frame + 0x1280, v);
    mtx_xf(V(D_80217B70) + (3 * 4 + fb + 2) * 64, v);
    mtx_xf(V(D_80217B70) + (3 * 4 + fb) * 64, v);
    mtx_xf(frame + 0x140, v);
    if (v[2] >= 0)
        return 0;                               /* behind the camera */
    mtx_xf(frame + 0x80, v);
    float pn = port_g16(V(D_8035807C)) / 65535.0f;
    if (v[3] * pn == 0)
        return 0;
    *sx = 160 + 160 * (v[0] / v[3]);
    *sy = 240 - (120 + 120 * (v[1] / v[3]));
    return 1;
}

static void latlong(float lat, float lon, float *x, float *y, float *z) {
    float r = (float)M_PI / 180;
    *x = cosf(-lon * r) * cosf(lat * r) * 250;
    *y = sinf(lat * r) * 250;
    *z = sinf(-lon * r) * cosf(lat * r) * 250;
}

/* the slot's PlayerInfo (D_80364AF0[D_80364AE8]): medal[] at 0x18, unk54[]
   (the routes found) at 0x54, gameState at 0x91 */
static const char *player(void) { return V(D_80364AF0) + (g8(V(D_80364AE8)) & 3) * 0x100; }

static int level_done(int i) {
    int m = g8(player() + 0x18 + i);
    return m >= 1 && m <= 5;
}

/* func_801FE760: a level the routes don't open yet (a later game state's,
   or one of five behind a particular other level) */
static int level_held(int i) {
    const char *li = V(D_802E8F94) + i * 0x44;  /* LevelInfo: unk0, gameState */
    int held = g8(li + 1) > g8(player() + 0x91) && ((g8(li) & 0x81) || (i >= 0x2B && i < 0x2F));
    static const signed char behind[][2] = { { 10, 55 }, { 15, 28 }, { 58, 53 }, { 5, 7 }, { 16, 19 } };
    for (unsigned k = 0; k < sizeof behind / sizeof behind[0]; k++)
        if (i == behind[k][0] && !level_done(behind[k][1]))
            held = 1;
    return i == 0 ? 0 : held;
}

/* the levels on Earth (func_80264BA4: 40 and 43-46 are elsewhere) that
   are open: the selected one, and those with a medal, done, or opened by
   the levels around (PlayerInfo.medal 1-5, 8), but for the CMO's intro
   and the end (47, 49), which aren't places on the map.  And one a done
   level's route has just found (the map draws the route to it and the
   stick goes there): its medal is 8 only from func_801FE018(8), which
   runs as a level is gone into, or at the map after the boot. */
static int map_level_open(int i) {
    if (i == 40 || (i >= 43 && i <= 47) || i == 49)
        return 0;
    int medal = g8(player() + 0x18 + i);
    if (i == g8(V(D_8021A905)) || (medal >= 1 && medal <= 5) || medal == 8)
        return 1;
    if (medal != 0)
        return 0;
    /* func_801FE018's test, for i */
    for (int j = 0; j < 60; j++) {
        if (!level_done(j))
            continue;
        const signed char *l = (const signed char *)V(D_8020D810) + j * 0x30;
        for (int k = 0; k < 8 && l[0x1C + k] != -1; k++)
            if (l[0x1C + k] == i && !level_held(i))
                return 1;
        for (int k = 0; k < 4 && l[0x18 + k] != -1; k++)
            if (l[0x18 + k] == i && (g8(player() + 0x54 + j) & (1 << k)))
                return 1;
    }
    return 0;
}

/* the open level drawn at (x, y), on the globe's near side, or -1 */
static int map_hit(float x, float y) {
    float cx = gf(V(D_8021A90C)), cy = gf(V(D_8021A910)), cz = gf(V(D_8021A914));
    int best = -1;
    float best_d = 14 * 14;
    for (int i = 0; i < 60; i++) {
        if (!map_level_open(i))
            continue;
        char *l = V(D_8020D810) + i * 0x30;
        float px = gf(l + 0x24), py = gf(l + 0x28), pz = gf(l + 0x2C), sx, sy;
        if ((px * cx + py * cy + pz * cz) / (250.0f * 925) < 0.3f)
            continue;                           /* round the back, or at the edge */
        if (!globe_project(px, py, pz, &sx, &sy))
            continue;
        float d = (sx - x) * (sx - x) + (sy - y) * (sy - y);
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

/* the view the pointer turned the globe to (port_globe_view) */
static struct {
    int on;
    float lon, lat, ease;
    float lon0, lat0;               /* at the drag's start */
    float kx, ky;                   /* screen pixels a degree of longitude, latitude moves the ground */
    float vlon, vlat;               /* after the drag: its speed, slowing */
    float last_lon, last_lat;
} globe;

int port_globe_view(int what) {
    switch (what) {
    case 0: return globe.on;
    case 1: return (int)lroundf(globe.lon * 1000);
    case 2: return (int)lroundf(globe.lat * 1000);
    default: return (int)lroundf(globe.ease * 10000);
    }
}

static float wrap180(float a) {
    while (a >= 180) a -= 360;
    while (a < -180) a += 360;
    return a;
}

static void globe_drag_start(void) {
    if (ui_context() != UI_MAP || port_g32(V(D_80217B6C)) != 3)
        return;
    /* from where the camera is (on its way to the selected level, or the
       view turned to before), measuring how the ground under it moves on
       the screen for a degree of each */
    float lat = gf(V(D_8021A920)), lon = gf(V(D_8021A91C)), x, y, z, sx0, sy0, sx, sy;
    globe.lat0 = lat;
    globe.lon0 = lon;
    globe.kx = globe.ky = 0;
    latlong(lat, lon, &x, &y, &z);
    if (!globe_project(x, y, z, &sx0, &sy0))
        return;
    latlong(lat, lon + 1, &x, &y, &z);
    if (globe_project(x, y, z, &sx, &sy))
        globe.kx = sx - sx0;
    latlong(lat + (lat > 0 ? -1 : 1), lon, &x, &y, &z);
    if (globe_project(x, y, z, &sx, &sy))
        globe.ky = (sy - sy0) * (lat > 0 ? -1 : 1);
    globe.vlon = globe.vlat = 0;
}

static void globe_drag_end(void) {
    if (!globe.on)
        return;
    globe.vlon = globe.lon - globe.last_lon;    /* it goes on, slowing (a fling) */
    globe.vlat = globe.lat - globe.last_lat;
}

static struct {
    int target, press, reads, pushed;
} map = { -1 };

static void map_reset(void) {
    globe.on = 0;
    map.target = -1;
    map.press = 0;
}

/* PORT_MAPCHECK=1 (tests): on the world map, each open level is clicked
   in turn from wherever the camera has come to rest, and logged as
   reached or not; at each stop, every open level on the screen is logged
   with its click zone (the pixels map_hit gives it) and whether a click on
   the marker itself takes it.  =all opens every level first (the save is
   still written: don't keep it). */
static void mapcheck(int earth, int idle, int sel) {
    static int mode = -1, target = -1, from, waited, settle, done, fails;
    static unsigned char tried[60], hops[60], pair[60][60];
    static int steps;
    static float last_lat, last_lon;
    static int pending = -1;
    if (mode < 0) {
        const char *s = getenv("PORT_MAPCHECK");
        mode = !s || !*s || *s == '0' ? 0 : !strcmp(s, "all") ? 2 : !strcmp(s, "turn") ? 3 : !strcmp(s, "fresh") ? 4 : 1;
    }
    if (!mode || done || !earth)
        return;
    int slot = g8(V(D_80364AE8)) & 3;
    if (mode == 2)
        for (int i = 0; i < 60; i++) {
            uint8_t *m = (uint8_t *)V(D_80364AF0) + slot * 0x100 + 0x18 + i;
            if (!(i == 40 || (i >= 43 && i <= 47) || i == 49) && (*m == 0 || *m > 5))
                *m = 3;
        }
    if (mode == 4) {
        /* =fresh: the levels marked open (8) as only just found, as they
           are back from a level until another is gone into */
        for (int i = 0; i < 60; i++) {
            uint8_t *m = (uint8_t *)V(D_80364AF0) + slot * 0x100 + 0x18 + i;
            if (*m == 8)
                *m = 0;
        }
        mode = 1;
    }
    float lat = gf(V(D_8021A920)), lon = gf(V(D_8021A91C));
    int still = fabsf(lat - last_lat) < 0.01f && fabsf(lon - last_lon) < 0.01f;
    last_lat = lat;
    last_lon = lon;
    if (target >= 0) {
        if (sel == target && still) {
            host_log("mapcheck: %d reached\n", target);
            target = -1;
        } else if (++waited > 300) {
            host_log("mapcheck: %d NOT reached from %d (selected %d)\n", target, from, sel);
            fails++;
            target = -1;
        }
        settle = 0;
        return;
    }
    if (!idle || !still || ++settle < 10)
        return;
    tried[sel] = 1;
    /* the stop: every open level on the screen and its zone */
    float cx = gf(V(D_8021A90C)), cy = gf(V(D_8021A910)), cz = gf(V(D_8021A914));
    float px[60], py[60];
    int vis[60] = { 0 };
    for (int i = 0; i < 60; i++) {
        char *l = V(D_8020D810) + i * 0x30;
        float x = gf(l + 0x24), y = gf(l + 0x28), z = gf(l + 0x2C);
        float dot = (x * cx + y * cy + z * cz) / (250.0f * 925);
        if (map_level_open(i) && globe_project(x, y, z, &px[i], &py[i]) && px[i] >= 0 && px[i] < 320 &&
            py[i] >= 0 && py[i] < 240 && dot > 0)
            vis[i] = 1 + (dot >= 0.3f);
    }
    for (int i = 0; i < 60; i++) {
        if (!vis[i])
            continue;
        int area = 0;
        for (int dy = -20; dy <= 20; dy++)
            for (int dx = -20; dx <= 20; dx++)
                area += map_hit(px[i] + dx, py[i] + dy) == i;
        int self = map_hit(px[i], py[i]);
        host_log("mapcheck: from %d: level %d at %.0f, %.0f: zone %d px%s%s\n", sel, i, px[i], py[i], area,
                 self == i ? "" : self < 0 ? ", NOT HIT at its marker" : ", its marker hits another",
                 vis[i] == 1 ? " (near the edge)" : "");
    }
    if (pending >= 0) {
        /* turned to near it: the click */
        int t = pending;
        pending = -1;
        if (!vis[t] || map_hit(px[t], py[t]) != t) {
            host_log("mapcheck: from %d, turned: %d NOT clickable (%s)\n", sel, t,
                     vis[t] ? "its marker hits something else" : "not on the screen");
            fails++;
            return;
        }
        host_log("mapcheck: from %d, turned: click %d at %.0f, %.0f\n", sel, t, px[t], py[t]);
        ev_push(EV_TAP, px[t], py[t]);
        target = t;
        from = sel;
        waited = 0;
        return;
    }
    /* the next, on the screen and taken by a click on its marker: an
       open level not yet tried, else one not yet tried from here, else one
       to go on from (the least gone through) */
    int next = -1, best = 0;
    for (int i = 0; i < 60; i++) {
        if (!vis[i] || i == sel || map_hit(px[i], py[i]) != i)
            continue;
        int score = !tried[i] ? 0 : !pair[sel][i] ? 1 : 2 + hops[i];
        if (next < 0 || score < best) {
            next = i;
            best = score;
        }
    }
    if (next < 0 || best >= 2 + 4 || ++steps > 400) {
        int pairs = 0;
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++)
                pairs += pair[i][j];
            if (!tried[i] && map_level_open(i)) {
                host_log("mapcheck: %d NOT tried (not clickable from where the tour went)\n", i);
                fails++;
            }
        }
        host_log("mapcheck: done, %d clicks from one level to another, %d failed\n", pairs, fails);
        done = 1;
        return;
    }
    if (best >= 2)
        hops[next]++;
    pair[sel][next] = 1;
    if (mode == 3) {
        /* =turn: the globe turned first (as a drag leaves it) to a little
           off the level, by an amount that changes from click to click */
        char *l = V(D_8020D810) + next * 0x30;
        float x = gf(l + 0x24), y = gf(l + 0x28), z = gf(l + 0x2C);
        float r = 180 / (float)M_PI;
        globe.on = 1;
        globe.lat = asinf(y / 250) * r + (float)(steps % 5 - 2) * 4;
        globe.lon = wrap180(-atan2f(z, x) * r + (float)(steps % 7 - 3) * 4);
        globe.vlon = globe.vlat = 0;
        globe.ease = 0.15f;
        pending = next;
        settle = 0;
        host_log("mapcheck: from %d: turning to %.0f, %.0f for %d\n", sel, globe.lat, globe.lon, next);
        return;
    }
    host_log("mapcheck: from %d: click %d at %.0f, %.0f\n", sel, next, px[next], py[next]);
    ev_push(EV_TAP, px[next], py[next]);
    target = next;
    from = sel;
    waited = 0;
}

/* this read's stick and buttons for the world map's pointer; 1 if it has
   the pad */
static int map_poll(int *sx, int *sy, int last_stick) {
    int earth = port_g32(V(D_80217B6C)) == 3;
    static unsigned n;
    if (host_verbose > 1 && earth && ++n % 30 == 0) {      /* -v -v: where the levels are */
        float cx = gf(V(D_8021A90C)), cy = gf(V(D_8021A910)), cz = gf(V(D_8021A914));
        host_log("ui: map: selected %d, camera %.1f %.1f, globe %d\n", g8(V(D_8021A905)), gf(V(D_8021A920)),
                 gf(V(D_8021A91C)), globe.on);
        for (int i = 0; i < 60; i++) {
            char *l = V(D_8020D810) + i * 0x30;
            float px = gf(l + 0x24), py = gf(l + 0x28), pz = gf(l + 0x2C), lx, ly;
            if (map_level_open(i) && (px * cx + py * cy + pz * cz) / (250.0f * 925) >= 0.3f &&
                globe_project(px, py, pz, &lx, &ly))
                host_log("ui: map: level %d at %.0f, %.0f\n", i, lx, ly);
        }
    }
    int idle = port_g16(V(D_8021A924)) == 1 && !port_g32(V(D_8036C778)) && !port_g32(V(D_8036C778) + 4) &&
               port_g32(V(D_80358060)) >= 6;
    int sel = g8(V(D_8021A905));
    mapcheck(earth, idle, sel);
    for (unsigned i = 0; i < evq_n; i++) {
        if (evq[i].kind == EV_BACK) {
            map_reset();
            pulse(B_B);
        } else if (evq[i].kind == EV_TAP) {
            int hit = earth ? map_hit(evq[i].x, evq[i].y) : sel;
            if (host_verbose && earth) {
                /* -v: where it landed, and the open level nearest it */
                float cx = gf(V(D_8021A90C)), cy = gf(V(D_8021A910)), cz = gf(V(D_8021A914)), best_d = 1e9f;
                int near = -1;
                for (int j = 0; j < 60; j++) {
                    char *l = V(D_8020D810) + j * 0x30;
                    float px = gf(l + 0x24), py = gf(l + 0x28), pz = gf(l + 0x2C), lx, ly;
                    if (map_level_open(j) && (px * cx + py * cy + pz * cz) > 0 && globe_project(px, py, pz, &lx, &ly) &&
                        (lx - evq[i].x) * (lx - evq[i].x) + (ly - evq[i].y) * (ly - evq[i].y) < best_d) {
                        best_d = (lx - evq[i].x) * (lx - evq[i].x) + (ly - evq[i].y) * (ly - evq[i].y);
                        near = j;
                    }
                }
                host_log("ui: map: click at %.0f, %.0f: level %d (the nearest open one %d, %.0f px off)\n", evq[i].x,
                         evq[i].y, hit, near, near < 0 ? 0 : sqrtf(best_d));
            }
            if (hit < 0)
                continue;
            map.target = hit;
            map.press = hit == sel;             /* the one picked: in (A); another: pick it */
            map.reads = map.pushed = 0;
        }
    }
    /* the drag: the ground under the pointer follows it */
    if (ptr.down && ptr.drag && earth && (globe.kx != 0 || globe.ky != 0)) {
        if (!globe.on) {
            globe.on = 1;
            globe.lon = globe.last_lon = globe.lon0;
            globe.lat = globe.last_lat = globe.lat0;
        }
        globe.last_lon = globe.lon;
        globe.last_lat = globe.lat;
        float dx = ptr.x - ptr.x0, dy = ptr.y - ptr.y0;
        if (fabsf(globe.kx) > 0.05f)
            globe.lon = wrap180(globe.lon0 - dx / globe.kx);
        if (fabsf(globe.ky) > 0.05f)
            globe.lat = globe.lat0 - dy / globe.ky;
        globe.lat = globe.lat > 80 ? 80 : globe.lat < -80 ? -80 : globe.lat;
        globe.ease = 0.6f;
    } else if (globe.on) {
        globe.lon = wrap180(globe.lon + globe.vlon);
        globe.lat += globe.vlat;
        globe.lat = globe.lat > 80 ? 80 : globe.lat < -80 ? -80 : globe.lat;
        globe.vlon *= 0.88f;
        globe.vlat *= 0.88f;
        globe.ease = fabsf(globe.vlon) + fabsf(globe.vlat) > 0.05f ? 0.6f : 0.15f;
    }
    if (!earth)
        globe.on = 0;
    if (map.target < 0)
        return ptr.down;
    if (++map.reads > 150) {                    /* it doesn't get there */
        map.target = -1;
        return 0;
    }
    if (map.press) {
        /* A, once the map takes it (a press during the carrier's move
           waits for its end in the game itself) */
        if (!idle)
            return 1;
        globe.on = 0;                           /* zooming in on the level, not the turned view */
        pulse(B_A);
        map.target = -1;
        return 1;
    }
    if (sel == map.target) {                    /* picked: the camera goes there */
        globe.on = 0;
        map.target = -1;
        return 0;
    }
    /* another level: the stick's next level is set to it and the stick
       pushed (11530.c, func_801F8980: from neutral, with the name shown
       in full, no window, no fade), and the game moves there itself, the
       carrier and the sound with it; the read before is neutral */
    if (map.pushed || !idle || (int16_t)port_g16(V(D_8021AB2C)) != 0xFF ||
        (int16_t)port_g16(V(D_8036BB1C)) != 1 || last_stick) {
        map.pushed = 0;
        return 1;                               /* the stick still meanwhile */
    }
    *(int8_t *)V(D_8021A907) = (int8_t)map.target;
    *sx = 40;                                   /* 1600: past 1500, the camera at its usual pace */
    *sy = 0;
    map.pushed = 1;
    globe.on = 0;
    return 1;
}

/* ---- the free camera ------------------------------------------------------ */

/* --free-camera: in a level (mode 4, not the carrier's overview), the
   mouse dragged with the left button down (relative: the pointer is
   caught while it is) and the right stick turn the camera.  Across is its
   heading, which the game keeps in D_80364414 (degrees; C-left/right turn
   it 45 at a time, 00000.c's func_80255190, which takes port_camera_turn
   each frame); the view turns right as it grows, as C-right turns it.  Up
   and down is the pitch, which the game hasn't got (port_camera_pitch,
   below).  The right stick is no longer C-left/right and C-up/down there:
   clicking it is C-down, and Y is still C-up. */

static int cam_on(int ctx) {
    return host_free_camera && ctx == UI_DRIVE && mode() == 4;
}

static void cam_release(void) {
    if (cam.held)
        SDL_SetRelativeMouseMode(SDL_FALSE);
    cam.held = 0;
}

/* the left button down or up: 1 if the camera takes it (a drag starts in
   a level, or the one under way ends), so that it isn't a click */
static int cam_button(int down) {
    if (!down) {
        int was = cam.held;
        cam_release();
        return was;
    }
    if (!ui_live || !cam_on(ui_context()))
        return 0;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    cam.held = 1;
    return 1;
}

/* -1..1 past the dead zone, finer near the middle */
static float stick_curve(int v) {
    const int dead = 8000;
    if (v <= dead && v >= -dead)
        return 0;
    float f = (float)(v > 0 ? v - dead : v + dead) / (32767 - dead);
    return f * fabsf(f);
}

int host_ui_camera_stick(int x, int y) {
    cam.stick = cam.stick_y = 0;
    if (!ui_live || !cam_on(ui_context()))
        return 0;
    cam.stick = stick_curve(x);
    cam.stick_y = stick_curve(y);
    return 1;
}

/* the pitch: the eye's offset from the point it looks at (00000.c, the
   level's follow camera, as it is drawn: the game's eye, which eases
   there, and gameplay are left as they are) turned up or down by
   cam.pitch, its length kept, between 8 degrees above the ground and 85
   (straight down would leave guLookAt no "up"); cam.pitch is held to what
   that allows, so that turning back takes effect at once */
float port_camera_pitch(int what, float h, float dy) {
    if (cam.pitch == 0 || h < 1)
        return what ? dy : h;
    const float deg = 3.14159265f / 180;
    float e0 = atan2f(dy, h) / deg, e = e0 + cam.pitch;
    if (e < 8) e = 8;
    if (e > 85) e = 85;
    if (e0 >= 8 && e0 <= 85)
        cam.pitch = e - e0;                     /* (the same again for what 1) */
    float r = sqrtf(h * h + dy * dy);
    return what ? r * sinf(e * deg) : r * cosf(e * deg);
}

int port_camera_turn(void) {
    int t = (int)lroundf(cam.turn * 1000);
    cam.turn -= t / 1000.0f;
    return t;
}

/* each read: let the pointer go outside a level (paused with the button
   down, say), and turn by the stick for the time since the last read (180
   degrees a second at full tilt) */
static void cam_poll(int live, int ctx) {
    int on = cam_on(ctx);
    if (!on) {
        if (cam.held) {
            cam_release();
            SDL_ShowCursor(ctx == UI_DRIVE ? SDL_DISABLE : SDL_ENABLE);
        }
        cam.turn = 0;
        if (mode() != 4 && mode() != 0x100)
            cam.pitch = 0;                      /* (kept for the level, paused too) */
        return;
    }
    if (live) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - cam.last) / 1000.0f;
        cam.last = now;
        if (dt > 0.1f)
            dt = 0.1f;
        cam.turn += cam.stick * 180.0f * dt;
        cam.pitch += cam.stick_y * 90.0f * dt;
    }
    cam.stick = cam.stick_y = 0;
}

/* ---- the read -------------------------------------------------------------- */

void host_ui_input(int live, const Uint8 *k, uint16_t *b, int *sx, int *sy) {
    static unsigned reads;
    static uint16_t last_out;
    static int last_stick, last_ctx = -1;
    ui_live = live;
    script_poll(++reads);
    int ctx = ui_context();
    if (ctx != UI_MAP)
        map_reset();
    if (ctx != last_ctx) {
        if (host_verbose)
            host_log("ui: read %u: %s, mode %llx\n", reads,
                     (const char *[]){ "other", "name", "menu", "map", "drive" }[ctx], (unsigned long long)mode());
        /* the cursor: away while driving */
        if (live && (ctx == UI_DRIVE) != (last_ctx == UI_DRIVE))
            SDL_ShowCursor(ctx == UI_DRIVE ? SDL_DISABLE : SDL_ENABLE);
        last_ctx = ctx;
    }
    cam_poll(live, ctx);

    if (ctx == UI_DRIVE && host_verbose) {
        static int last_type = -1, last_pedals = -1;
        int type = g8(V(D_80364456)), pd = pedals();
        if (type != last_type || pd != last_pedals)
            host_log("ui: read %u: vehicle %d, W/S %s\n", reads, type, pd ? "A/B" : "the stick");
        last_type = type;
        last_pedals = pd;
    }

    /* the keys */
    uint16_t kb = 0;
    int kx = 0, ky = 0;
    if (live && k) {
        for (int sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
            if (!key_held[sc])
                continue;
            kb |= key_held[sc];
            key_seen[sc] = 1;
            if (key_up[sc]) {
                key_held[sc] = 0;
                key_up[sc] = 0;
            }
        }
        int wasd = ctx != UI_NAME;
        int up = k[SDL_SCANCODE_UP] || (wasd && k[SDL_SCANCODE_W]);
        int down = k[SDL_SCANCODE_DOWN] || (wasd && k[SDL_SCANCODE_S]);
        int left = k[SDL_SCANCODE_LEFT] || (wasd && k[SDL_SCANCODE_A]);
        int right = k[SDL_SCANCODE_RIGHT] || (wasd && k[SDL_SCANCODE_D]);
        if (ctx == UI_DRIVE && pedals()) {
            kb |= (up ? B_A : 0) | (down ? B_B : 0);
        } else {
            ky += (up ? 80 : 0) - (down ? 80 : 0);
        }
        kx += (right ? 80 : 0) - (left ? 80 : 0);
    }
    *b |= kb;
    if (kx) *sx = kx;
    if (ky) *sy = ky;
    if (ctx == UI_MAP && (*b || *sx || *sy))
        map_reset();                            /* the keys or a controller have it again */

    /* the pointer */
    uint16_t pb = 0;
    for (unsigned i = 0; i < evq_n; i++)
        if (ctx == UI_DRIVE && (evq[i].kind == EV_WHEEL_UP || evq[i].kind == EV_WHEEL_DOWN))
            pulse(evq[i].kind == EV_WHEEL_UP ? B_CU : B_CD);
    switch (ctx) {
    case UI_MENU:
        pb = menu_poll(last_out);
        break;
    case UI_NAME:
        pb = name_poll();
        break;
    case UI_MAP: {
        int mx = 0, my = 0;
        if (map_poll(&mx, &my, last_stick) && !*b && !*sx && !*sy) {
            *sx = mx;
            *sy = my;
        }
        break;
    }
    case UI_OTHER:
        for (unsigned i = 0; i < evq_n; i++)
            pulse(evq[i].kind == EV_TAP ? B_A : evq[i].kind == EV_BACK ? B_B : 0);
        break;
    }
    evq_n = 0;
    if (ctx != UI_MENU)
        menu.window = -1;
    while (pulse_n && !pulse_q[0])
        memmove(pulse_q, pulse_q + 1, --pulse_n * sizeof pulse_q[0]);
    pb |= pulse_take(last_out);
    *b |= pb;
    last_out = *b;
    last_stick = *sx * *sx + *sy * *sy >= 1500;
}
