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
/* the game switches mode (00000.c's loop, before the new mode's init) */
void port_replay_mode_switch(void);
#define D_803156C0 port_counter(1, __func__)
/* the world map's camera (11530.c), when the mouse or a finger has turned
   the globe (port/host/ui.c): what = 0, whether it has (then the camera
   heads for 1, the longitude, and 2, the latitude, in thousandths of a
   degree, by 3, ten-thousandths of the way a frame) */
int port_globe_view(int what);

#endif
