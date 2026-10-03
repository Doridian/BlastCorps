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
#elif defined(PORT_BLKLOG)
void port_blklog(unsigned int id);
#define ENGINE_BLK_(id, n) (__port_icount += (n), port_blklog(id))
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

/* IDO's conversion of a float to an unsigned int (jp's IDO code, still
   asm in the C): the FCSR set to truncate, cvt.w.s, and where its V flag
   says the float is 2^31 or more, a second one of x - 2^31 with the top bit
   set; negative or out of range gives 0xFFFFFFFF.  Returns the result in
   the low word and the path in the high one: 0 in range, 1 negative, 2 the
   second conversion, 3 that one out of range (not through a pointer: the
   host's store to the game's C memory would be in the host's order, which
   BEPass reads swapped).  IDO_CVT_U_S charges its blocks after the first
   one (the caller's): b1 the second conversion, b2 its result, b3 the
   0xFFFFFFFF, b4 the first's result (port/host/engine.c). */
u64 engine_ido_cvt_u_s(f32 x);
#define IDO_CVT_U_S(dst, x, b1, b2, b3, b4)                                 \
    do {                                                                    \
        u64 cvt_ = engine_ido_cvt_u_s(x);                                   \
        switch ((u32)(cvt_ >> 32)) {                                        \
        case 0: ENGINE_BLK(b4); break;                                      \
        case 1: ENGINE_BLK(b4); ENGINE_BLK(b3); break;                      \
        case 2: ENGINE_BLK(b1); ENGINE_BLK(b2); break;                      \
        default: ENGINE_BLK(b1); ENGINE_BLK(b3); break;                     \
        }                                                                   \
        (dst) = (u32)cvt_;                                                  \
    } while (0)

/* sprintf as the translation's call of it from IDO's code cost: nothing
   for the C (gen_glue.py's recomp_extern_sprintf).  The 32-bit build's
   own n64_sprintf is counted as the game's C (port/src/libc.c), so the
   native code calls engine_libc.c's; elsewhere sprintf is host code
   (port/host/libc64.c). */
#if !defined(PORT_64BIT) && !defined(PORT_MOVABLE)
int engine_sprintf(char *buf, const char *fmt, ...);
#else
#define engine_sprintf sprintf
#endif

/* The original's syscall at pc (a state it doesn't expect): the game
   stops, as with the translation (port/host/engine.c). */
void engine_trap(u32 pc);

/* Registers the original saves on its stack and loads back before it
   returns: engine_save() at its entry and engine_restore() before each
   return put them back in the thread's context as they were, undoing what
   its translated callees left there (port/host/engine.c).  The masks have
   a bit per GPR and per FPR word. */
void engine_save(u32 gmask, u32 fmask);
/* What the thread's context holds in GPR reg now: what the original would
   find in a register that the code before it left (port/host/engine.c). */
u32 engine_ctx(unsigned int reg);
void engine_restore(void);
#define ENGINE_GPR(r) (1u << (r))
#define ENGINE_S0_S7_GP_FP (0xFFu << 16 | ENGINE_GPR(28) | ENGINE_GPR(30))
#define ENGINE_T0_T5 (0x3Fu << 8)
#define ENGINE_F20_F31 (0xFFFu << 20)

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
/* and its `syscall` (Rare's "can't happen" in a switch) */
void engine_syscall(u32 pc) __attribute__((noreturn));
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

/* The original's stack frame.  What Rare's code stores on the N64 stack
   stays there after it returns, and later code may read a slot it never
   wrote itself (the driver's shadow takes its tilt from one: 69BB0's
   func_802AF340 passes func_802582C4 three stack arguments it doesn't
   store).  A native function whose frame such a read reaches keeps it as
   the original does: engine_frame(-0x58) moves the context's $sp as its
   addiu does and returns the new $sp, engine_frame_sd(off, reg) stores the
   context's GPR at $sp + off as its sd does, engine_frame_sdc1(off, fpr)
   an FPR pair as its sdc1, engine_frame_sw(off, v) a word.  Translated
   callees run at the context's $sp, as the original's jal leaves it.  A
   function the game's C calls directly starts ENGINE_C_FRAME below the
   context's $sp, where the glue would have started its translation. */
u32 engine_frame(s32 n);
void engine_frame_sd(u32 off, unsigned int reg);
void engine_frame_sdc1(u32 off, unsigned int fpr);
void engine_frame_sw(u32 off, u32 v);
/* the word at $sp + off, as the original's lw reads what is there */
u32 engine_frame_lw(u32 off);
#define ENGINE_C_FRAME 16
/* $ra as the original's jal leaves it for a callee that saves it in its
   frame: the address of the block after the call, ENGINE_RA(802AB6C4),
   each version's own (engine_blocks.h's ENGINE_ADDR_) */
#define ENGINE_RA(next) ENGINE_LEAVE(31, ENGINE_ADDR_##next)
/* the frame of 0x58 and 0x30 most of Rare's functions save $ra, $s0..$s7,
   $gp, $fp and $f20..$f31 in (engine_frame(0x30 + 0x58) leaves it) */
void engine_frame_s(void);
#define ENGINE_FRAME_S (0x30 + 0x58)

/* a COP0 register (mfc0), as the translated code reads it */
u32 engine_mfc0(unsigned int reg);
#define ENGINE_REG(gpr) engine_reg(gpr)

/* A block as data: a helper that stands for several copies of the
   original's code (each with its own blocks) takes the caller's as a
   table of these, ENGINE_B(8029B144) each, and charges one with
   ENGINE_BLK_AT(table[i]) */
typedef struct EngineBlk {
    u16 id, n;
} EngineBlk;
#define ENGINE_B(addr) { ENGINE_BLK_##addr }
#define ENGINE_BLK_AT(b) ENGINE_BLK_((b).id, (b).n)

/* A count for the taint build (-DPORT_ENGINE_TAINT=ON, port/host/engine.c;
   port/tools/taint.py --probes): whether a path some leftover feeds is
   ever taken, say.  Nothing in the other builds. */
#ifdef PORT_ENGINE_TAINT
void engine_probe(unsigned id);
#else
#define engine_probe(id) ((void)0)
#endif

/* A function's charge in one: n instructions at its entry, under its entry
   block's id (for the check build's and PORT_BLKLOG's traces), in place of
   its blocks one by one.  n is what its original took a call on average
   over the TAS and the quick tier (us.v10's blocks; docs/PORT.md, "The
   engine made readable"): the --cpu-model n64 timing stays right over a
   frame without the code keeping the asm's blocks. */
#define ENGINE_BLK_ID_(id, n) (id)
#define ENGINE_BLK_ID_X(...) ENGINE_BLK_ID_(__VA_ARGS__)
#define ENGINE_COST(addr, n) ENGINE_BLK_(ENGINE_BLK_ID_X(ENGINE_BLK_##addr), (n))

#endif
