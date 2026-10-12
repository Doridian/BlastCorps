/*
 * The engine's replacement: Rare's handwritten engine (hd_code 56040-8E910,
 * hd_front_end 1B100), function by function, as native C over the game's
 * types.  docs/PORT.md, "Replacing the engine", has the whole of it; in
 * short:
 *
 *   - A function listed in replaced.txt is C here, called by the game's C
 *     directly (tools/recomp, which translated the original before it was
 *     C, leaves the ones listed out; the port builds none of it).
 *   - Rare's code passes values in whatever registers suit it.  REGS()
 *     before a definition or declaration names them, parameters then
 *     results: REGS(t0, t3 -> v0, a1) is `s32 f(s32 t0, s32 t3, s32 *a1)`,
 *     the first result returned and the others through trailing pointers.
 *     Without REGS() it is o32's.  port/tools/conventions.py says what each
 *     function really takes and gives back (the glue of the builds before
 *     P10 checked REGS() against it; now a record of the original's).
 *
 * Built like the game's C (N64 side), in the host's byte order and pointer
 * size.
 */
#ifndef ENGINE_H
#define ENGINE_H

#include "common.h"
#include "functions.h"
/* the VR4300's conversions, the original's traps, ENGINE_DIV, engine_mfc0 */
#include "cvt.h"

#define REGS(...)

/* the game's sprintf (port_game.h's n64_sprintf: port/host/libc64.c) */
int sprintf(char *buf, const char *fmt, ...);

/* A number that is not an address.  N64_PC: where an instruction of the
   original sat (us.v11's address in every version), used only in the
   report of a trap; never dereferenced.  N64_VALUE: a word the original
   held that the game uses as data (a return address or stack address
   that leaked into an angle), which has to stay the N64's number. */
#define N64_PC(pc) ((u32)(pc))
#define N64_VALUE(v) ((u32)(v))

/* the port's own "can't happen": data past what an engine array has room
   for (port/host) */
void host_fatal(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

/* ---- display lists ---------------------------------------------------------

   The engine's display lists are Gfx arrays, walked a command at a time
   and read and written by their two words (gbi.h's Gwords: as wide as a
   pointer, 4 or 8 bytes), never as bytes or doublewords.  An address in a
   word is DL_ADDR's: the pointer itself. */
#define DL_ADDR(p) ((uintptr_t)(p))
/* the command at g set to (w0, w1); the next */
static inline Gfx *dl_put(Gfx *g, u32 w0, uintptr_t w1) {
    g->words.w0 = w0;
    g->words.w1 = w1;
    return g + 1;
}
/* the command at s copied to d */
static inline void dl_copy_cmd(Gfx *d, const Gfx *s) {
    d->words.w0 = s->words.w0;
    d->words.w1 = s->words.w1;
}
/* a command's opcode */
#define DL_OP(g) ((u32)(g)->words.w0 >> 24)

/* What stays in the N64's byte order (the ROM's files where they are read
   a byte at a time, the tables kept big-endian, the RDP's texels): read
   and written a byte at a time, so the same in either byte order (and at
   any alignment) */
#define BE16S(p) ((s16)(((s8 *)(p))[0] << 8 | ((u8 *)(p))[1]))
#define BE16U(p) ((u32)(((u8 *)(p))[0] << 8 | ((u8 *)(p))[1]))
#define BE32U(p) ((u32)((u32)((u8 *)(p))[0] << 24 | (u32)((u8 *)(p))[1] << 16 | (u32)((u8 *)(p))[2] << 8 | \
                        (u32)((u8 *)(p))[3]))
static inline void be_put16(void *p, u32 v) {
    ((u8 *)p)[0] = (u8)(v >> 8);
    ((u8 *)p)[1] = (u8)v;
}
static inline void be_put32(void *p, u32 v) {
    ((u8 *)p)[0] = (u8)(v >> 24);
    ((u8 *)p)[1] = (u8)(v >> 16);
    ((u8 *)p)[2] = (u8)(v >> 8);
    ((u8 *)p)[3] = (u8)v;
}

#endif
