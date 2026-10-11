/*
 * The VR4300's float-to-integer conversions and the original's traps, for
 * Rare's code in port/engine (engine.h includes this) and their
 * definitions in the host (port/host/cvt.c).  Plain C on any host: no FP
 * environment, nothing out of range left to the instruction set's own
 * conversion (x86 gives 0x80000000, ARM and PowerPC saturate, MIPS gives
 * 0x7FFFFFFF: these give the VR4300's answer everywhere).
 * port/tools/rint_check.c checks the rounding against libm's.  (The game's
 * C and the host convert out-of-range floats with port_f2i instead,
 * port/include/port_f2i.h: x86's 0x80000000, what the TAS was made with.)
 */
#ifndef CVT_H
#define CVT_H

#include <stdint.h>

/* ---- the engine's ------------------------------------------------------- */

/* cvt.w.s/round.w.s round to nearest (to even, or halves up as the TAS's
   emulator did: port_round_half_up), trunc.w.s truncates; NaN and out of
   range give 0x7FFFFFFF (0x7FFFFFFFFFFFFFFF for the .l forms). */
int32_t engine_cvt_w_s(float x);
int32_t engine_cvt_w_d(double x);
int32_t engine_trunc_w_s(float x);
int32_t engine_trunc_w_d(double x);
int64_t engine_cvt_l_d(double x);
int64_t engine_cvt_l_s(float x);

/* IDO's conversion of a float to an unsigned int (jp's IDO code, still
   asm in the C): the FCSR set to truncate, cvt.w.s, and where its V flag
   says the float is 2^31 or more, a second one of x - 2^31 with the top bit
   set; negative or out of range gives 0xFFFFFFFF. */
uint32_t engine_ido_cvt_u_s(float x);
#define IDO_CVT_U_S(dst, x) ((dst) = engine_ido_cvt_u_s(x))

/* the original's `break` (IDO's division checks: code 7 for a zero
   divisor, 6 for an overflow) and its `syscall` (Rare's "can't happen" in
   a switch): the game stops.  pc is us.v11's address of the instruction,
   for the report. */
void engine_break(uint32_t pc, uint32_t code) __attribute__((noreturn));
void engine_syscall(uint32_t pc) __attribute__((noreturn));
void engine_trap(uint32_t pc);
/* q = n / d, as IDO's checked div: bz and bov name its two breaks (a zero
   divisor, the overflow of 0x80000000 / -1) for the report */
#define ENGINE_DIV(q, n, d, bz, bov)                                        \
    do {                                                                    \
        int32_t n_ = (n), d_ = (d);                                         \
        if (d_ == 0)                                                        \
            engine_break((uint32_t)(0x##bz), 7);                            \
        if (d_ == -1 && n_ == (int32_t)0x80000000)                          \
            engine_break((uint32_t)(0x##bov), 6);                           \
        (q) = n_ / d_;                                                      \
    } while (0)

/* What mfc0 reads of COP0 (the scheduler's func_802A1320 reads Status):
   Status (12) as the game runs, interrupts on and CU1; Count (9) the
   host's ticks; 0 for the rest. */
uint32_t engine_mfc0(unsigned int reg);

/* ---- how: the definitions' own (port/host/cvt.c, rint_check.c) ---------- */

/* 1 for cvt.w's halves up as mupen64plus's MSVC build rounds them
   (floor(x + 0.5), in double): what the TAS was made on (replay.c sets it,
   docs/PORT.md); 0 for the VR4300's to-even. */
extern int port_round_half_up;

/* Round-to-even by adding and taking away 2^52 (2^23 for a float) in the
   default rounding mode, which the port never changes; anything that large
   is an integer already, and NaN and the infinities fail the range test
   and come back as they are.  A zero result takes x's sign, as nearbyint's
   and floor's do.  (Not libm's nearbyint: it saves and restores the FP
   environment, and was 7% of the port's CPU, docs/PORT.md "Engine hot
   spots".) */
static inline double port_rne(double x) {
    double r;
    if (!(x < 4503599627370496.0 && x > -4503599627370496.0)) return x;
    r = x >= 0 ? (x + 4503599627370496.0) - 4503599627370496.0 : (x - 4503599627370496.0) + 4503599627370496.0;
    return r == 0 ? x * 0.0 : r;
}
static inline float port_rnef(float x) {
    float r;
    if (!(x < 8388608.0f && x > -8388608.0f)) return x;
    r = x >= 0 ? (x + 8388608.0f) - 8388608.0f : (x - 8388608.0f) + 8388608.0f;
    return r == 0 ? x * 0.0f : r;
}
static inline double port_floor(double x) {
    double r = port_rne(x);
    return r > x ? r - 1.0 : r;
}
static inline double port_rint(double x) { return port_round_half_up ? port_floor(x + 0.5) : port_rne(x); }
static inline float port_rintf(float x) { return port_round_half_up ? (float)port_floor(x + 0.5) : port_rnef(x); }

/* float -> integer, `r` already rounded to an integer by the instruction's
   rounding mode; NaN and out of range give 2^31-1 / 2^63-1 as the VR4300
   does with the invalid-operation exception disabled */
static inline uint32_t port_f2w(double r) {
    if (!(r >= -2147483648.0 && r <= 2147483647.0)) return 0x7FFFFFFFu;
    return (uint32_t)(int32_t)r;
}
static inline uint64_t port_f2l(double r) {
    if (!(r >= -9223372036854775808.0 && r < 9223372036854775808.0)) return 0x7FFFFFFFFFFFFFFFull;
    return (uint64_t)(int64_t)r;
}
static inline uint64_t port_f2l_s(float r) {
    if (!(r >= -9223372036854775808.0f && r < 9223372036854775808.0f)) return 0x7FFFFFFFFFFFFFFFull;
    return (uint64_t)(int64_t)r;
}

#endif
