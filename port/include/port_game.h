/*
 * Force-included into every N64-side file (the game's C and port/src).
 *
 * The game defines or calls functions under libc/libm names (libultra's
 * sinf, cosf, bcopy, bzero, sprintf, memcpy, strlen, strchr).  Its own
 * versions are built for big-endian memory and would interpose on the
 * host's for SDL and the GL driver, so they get other names here.
 */
#ifndef PORT_GAME_H
#define PORT_GAME_H

#define sinf n64_sinf
#define cosf n64_cosf
#define bcopy n64_bcopy
#define bzero n64_bzero
#define sprintf n64_sprintf
#define memcpy n64_memcpy
#define strlen n64_strlen
#define strchr n64_strchr

/* port_f2i: a float to an integer where it may be out of range */
#include "port_f2i.h"

/* --replay (port/host/replay.c): the game has seeded its random number
   generator from osGetCount, which is the port's clock, not the movie's */
void port_replay_seeded(void);

/* The game's reads of the scheduler's retrace count and level timer (the
   variables alias Sched.frameCount and unk280; 00000.c's SC_FRAMECOUNT and
   SC_TIMER): calls, naming the function that reads, so that --replay can
   give it the movie's values (port/host/replay.c).  The scheduler itself,
   and the game's two waits on the count, read the real ones through the
   struct.  sched.h leaves out its declarations for the port. */
unsigned int port_counter(int timer, const char *func);
#define D_803156C4 port_counter(0, __func__)
/* whether the frame reads the pad (45BB0.c): `free`, or with --replay, what
   the movie's frame did */
int port_pad_read_due(int free);
/* the pak/EEPROM thread starts a command (E7B0.c): with --replay, it waits
   for the frame the movie's thread did (port/src/replay_hooks.c) */
void port_replay_save_started(void);
/* 1 with --load-waits n64: the waits that are only there because the
   N64's hardware is slow are kept; 0, the default, leaves them out
   (docs/PORT.md, "The front end's waits") */
int port_load_waits(void);
/* A busy-wait's body (00000.c's waits for 15 retraces, 4B450.c's for the
   SI, E7B0.c's on the scheduler's reset flag): on the N64 the loop spins
   until an interrupt changes what it reads.  Here the thread waits for the
   next interrupt, holding the CPU against lower priorities as the spin did,
   and then tests again (port/host/threads.c).  Being a call, it also makes
   the loop read the variable again, as IDO's code does. */
void port_spin_wait(void);
/* the game switches mode (00000.c's loop, before the new mode's init) */
void port_replay_mode_switch(void);
#define D_803156C0 port_counter(1, __func__)
/* the world map's camera (11530.c), when the mouse or a finger has turned
   the globe (port/host/ui.c): what = 0, whether it has (then the camera
   heads for 1, the longitude, and 2, the latitude, in thousandths of a
   degree, by 3, ten-thousandths of the way a frame) */
int port_globe_view(int what);
/* a level's camera (00000.c, func_80255190): thousandths of a degree to
   turn it by this frame, from the mouse and the right stick with
   --free-camera (port/host/ui.c); 0 otherwise */
int port_camera_turn(void);
/* the pitch: the follow camera's eye, h across and dy up from the point it
   looks at (00000.c, func_802507C8), turned up or down: what = 0, the new
   h, 1, the new dy (values, not pointers: the game's stack isn't the
   host's) */
float port_camera_pitch(int what, float h, float dy);
/* 1 when a press may cut the logos and the attract mode's screens short
   (8380.c, 00000.c, 17990.c): not with --replay or PORT_AUTOSTART, whose
   input is the original's (port/host/video.c) */
int port_intro_skip(void);
/* an icon's picture (2E490.c, func_80272C5C) is texture tex of the texture
   table, row `row` of `rows`, loaded at p, with the icon's flags; tex -1:
   the icons are all gone (func_80272C50).  The renderer may draw the model
   the picture shows in its place (port/host/micons.c) */
void host_icon_texture(const void *p, int tex, int row, int rows, int flags);
/* The level pool's display lists (port/src/pool.c): the pool's top
   (D_80358070) moved to `end`, past display-list commands from it, which
   are 8 bytes on the N64 and sizeof(Gfx) here; the N64's own position
   moves by 8 a command.  Every carve-out of commands from the pool is made
   by it (the ROM's lists, widened where they land, by port_pool_widened).
   Returns end. */
void *port_pool_gfx(void *end);
/* a display list loaded at `at`, `narrow` bytes in the ROM, `wide` here
   (src/widen.c) */
void port_pool_widened(void *at, unsigned int narrow, unsigned int wide);
/* the pool emptied (00000.c, the start of a level or of the front end) */
void port_pool_reset(void);
/* where the N64's top would be: D_80358070 less what the display lists
   below it take beyond the N64's 8 bytes a command */
void *port_pool_n64(void);
/* the top moved to `end`, past the effects' heap (port/engine/60F60.c);
   returns end */
void *port_pool_slots(void *end);

/* The boot code's words (osTvType...) are libultra's variables
   (src/ultra.c), not a region at 0x80000300: the game reads osTvType as
   D_80000300 (hd_code 00000.c, 22EE0.c). */
#define D_80000300 osTvType
/* the front end's variables, which src/overlay.c puts back whenever the
   game loads it: tools/gen_fevars.py's tables, a row a variable (n64 its
   N64 address, 0: none, and n64room what of it is there; snap its copy,
   NULL for read-only data) */
typedef struct {
    unsigned char *at;
    unsigned int size, n64, n64room;
    unsigned char *snap;
} PortFeVar;

#endif
