/*
 * The engine's replacement: Rare's handwritten engine (hd_code 56040-8E910,
 * hd_front_end 1B100), function by function, as native C over the game's
 * types.  docs/PORT.md, "Replacing the engine", has the whole of it; in
 * short:
 *
 *   - A function listed in replaced.txt isn't translated any more
 *     (tools/recomp keeps its translation as recomp_orig_X, for the checks).
 *     Its C here is called by the game's C directly, by the translated code
 *     through generated glue (port/tools/gen_glue.py), and it calls the
 *     translated functions through glue the other way.
 *   - Rare's code passes values in whatever registers suit it.  REGS()
 *     before a definition or declaration names them, parameters then
 *     results: REGS(t0, t3 -> v0, a1) is `s32 f(s32 t0, s32 t3, s32 *a1)`,
 *     the first result returned and the others through trailing pointers.
 *     Without REGS() it is o32's.  port/tools/conventions.py says what each
 *     function really takes and gives back; the glue checks REGS() against
 *     it.
 *   - The CPU's time: the port charges the MIPS instructions the original
 *     would have executed (docs/PORT.md, "Timing"), and the game's pace,
 *     and the TAS, depend on it to the instruction.  ENGINE_BLK(802AC1E4)
 *     charges the original's basic block at 0x802AC1E4: the code calls it
 *     wherever the original would have run that block, so it costs what
 *     the original did on every path.  (BEPass doesn't count this code, and
 *     puts no polls in it: BEPASS_ENGINE=1.)
 *
 * Built like the game's C (N64 side: BEPass, port-ilp32, port-arena), so
 * memory, pointers and PTR32 work as there, in every variant.
 */
#ifndef ENGINE_H
#define ENGINE_H

#include "common.h"
#include "engine_blocks.h"      /* generated: ENGINE_BLK_<address> id, size */

#define REGS(...)

/* the translated code's instruction count (port/host/threads.c); not
   byte-swapped (BEPass leaves __port_ names alone) */
extern unsigned int __port_icount;
#ifdef PORT_ENGINE_CHECK
void engine_trace_blk(unsigned int id);
#define ENGINE_BLK_(id, n) (__port_icount += (n), engine_trace_blk(id))
#else
#define ENGINE_BLK_(id, n) (__port_icount += (n))
#endif
#define ENGINE_BLK_X(...) ENGINE_BLK_(__VA_ARGS__)
#define ENGINE_BLK(addr) ENGINE_BLK_X(ENGINE_BLK_##addr)

/* The VR4300's conversions as the translated code does them (recomp.h):
   cvt.w.s/round.w.s round to nearest (to even, or halves up as the TAS's
   emulator did: recomp_round_half_up), trunc.w.s truncates; NaN and out of
   range give 0x7FFFFFFF.  (port/host/engine.c) */
s32 engine_cvt_w_s(f32 x);
s32 engine_cvt_w_d(f64 x);
s32 engine_trunc_w_s(f32 x);
s32 engine_trunc_w_d(f64 x);
s64 engine_cvt_l_d(f64 x);
s64 engine_cvt_l_s(f32 x);

/* Registers the original leaves behind besides its results, which some
   translated code reads afterwards (conventions.py's outputs that REGS()
   doesn't name: a float temporary, a constant it loaded): the native code
   puts them in the thread's context itself, wherever it is called from,
   and the glue keeps them (port/host/engine.c).  ENGINE_LEAVE_F(2, x)
   leaves the float x in $f2, ENGINE_LEAVE(1, 3) the word 3 in $at. */
void engine_leave(unsigned int reg, u32 value);
#define ENGINE_LEAVE(gpr, v) engine_leave((gpr), (u32)(v))
#define ENGINE_LEAVE_F(fpr, x)                                              \
    do {                                                                    \
        union { f32 f; u32 u; } leave_;                                     \
        leave_.f = (x);                                                     \
        engine_leave(34 + (fpr), leave_.u);                                 \
    } while (0)
#define ENGINE_LEAVE_FW(fpr, w) engine_leave(34 + (fpr), (u32)(w))
/* a whole 64-bit GPR (Rare's dmult/dadd leave some) */
void engine_leave64(unsigned int reg, u32 lo, u32 hi);

/* the original's `break` (IDO's division checks: code 7 for a zero
   divisor, 6 for an overflow): it stops the port as the translation's
   recomp_trap does.  pc is us.v11's address of the block, for the report. */
void engine_break(u32 pc, u32 code) __attribute__((noreturn));
/* q = n / d, as IDO's checked div: the blocks of its zero test (bz, its
   break), the -1 test (bm1) and the overflow test (bmin, its break bov) */
#define ENGINE_DIV(q, n, d, bz, bm1, bmin, bov)                             \
    do {                                                                    \
        s32 n_ = (n), d_ = (d);                                             \
        if (d_ == 0) {                                                      \
            ENGINE_BLK(bz);                                                 \
            engine_break(0x##bz, 7);                                        \
        }                                                                   \
        ENGINE_BLK(bm1);                                                    \
        if (d_ == -1) {                                                     \
            ENGINE_BLK(bmin);                                               \
            if (n_ == (s32)0x80000000) {                                    \
                ENGINE_BLK(bov);                                            \
                engine_break(0x##bov, 6);                                   \
            }                                                               \
        }                                                                   \
        (q) = n_ / d_;                                                      \
    } while (0)
#define ENGINE_LEAVE64(gpr, v) engine_leave64((gpr), (u32)(v), (u32)((u64)(v) >> 32))

/* The other way: what a register holds that the original reads without
   having set it, and stores (a byte of a collision triangle from whatever
   $t9 held, say).  Called by the game's C or by the translated code, the
   original sees what the context holds; ENGINE_REG(25) is that. */
u32 engine_reg(unsigned int reg);
#define ENGINE_REG(gpr) engine_reg(gpr)

#endif
