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
 *
 * Built like the game's C (N64 side: BEPass, port-ilp32, port-arena), so
 * memory, pointers and PTR32 work as there, in every variant.
 */
#ifndef ENGINE_H
#define ENGINE_H

#include "common.h"
#include "functions.h"

#define REGS(...)

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
   set; negative or out of range gives 0xFFFFFFFF (port/host/engine.c). */
u32 engine_ido_cvt_u_s(f32 x);
#define IDO_CVT_U_S(dst, x) ((dst) = engine_ido_cvt_u_s(x))

/* the game's sprintf (port_game.h's n64_sprintf: port/src/libc.c, or
   port/host/libc64.c) */
int sprintf(char *buf, const char *fmt, ...);

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
   recomp_trap does.  pc is us.v11's address of the break, for the report. */
void engine_break(u32 pc, u32 code) __attribute__((noreturn));
/* and its `syscall` (Rare's "can't happen" in a switch) */
void engine_syscall(u32 pc) __attribute__((noreturn));
/* q = n / d, as IDO's checked div: bz and bov name its two breaks (a zero
   divisor, the overflow of 0x80000000 / -1) for the report */
#define ENGINE_DIV(q, n, d, bz, bov)                                        \
    do {                                                                    \
        s32 n_ = (n), d_ = (d);                                             \
        if (d_ == 0)                                                        \
            engine_break(0x##bz, 7);                                        \
        if (d_ == -1 && n_ == (s32)0x80000000)                              \
            engine_break(0x##bov, 6);                                       \
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
/* the frame of 0x58 and 0x30 most of Rare's functions save $ra, $s0..$s7,
   $gp, $fp and $f20..$f31 in (engine_frame(0x30 + 0x58) leaves it) */
void engine_frame_s(void);
#define ENGINE_FRAME_S (0x30 + 0x58)

/* a COP0 register (mfc0), as the translated code reads it */
u32 engine_mfc0(unsigned int reg);
#define ENGINE_REG(gpr) engine_reg(gpr)

/* A count for the taint build (-DPORT_ENGINE_TAINT=ON, port/host/engine.c;
   port/tools/taint.py --probes): whether a path some leftover feeds is
   ever taken, say.  Nothing in the other builds. */
#ifdef PORT_ENGINE_TAINT
void engine_probe(unsigned id);
#else
#define engine_probe(id) ((void)0)
#endif

#endif
