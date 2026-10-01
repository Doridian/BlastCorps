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

#endif
