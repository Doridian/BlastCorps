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
   table, row `row` of `rows`, loaded at addr, with the icon's flags; tex -1:
   the icons are all gone (func_80272C50).  The renderer may draw the model
   the picture shows in its place (port/host/micons.c) */
void port_icon_texture(unsigned int addr, int tex, int row, int rows, int flags);

#endif
