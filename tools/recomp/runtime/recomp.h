/*
 * Runtime interface of the mechanically translated handwritten code.
 *
 * Every translated function has the signature
 *
 *     void recomp_func_XXXXXXXX(uint8_t *rdram, recomp_context *ctx);
 *
 * and works on N64 state only: the VR4300 register file in `ctx`, and
 * memory at N64 (KSEG0/KSEG1) addresses, reached as rdram + (addr & 0x1FFFFFFF).
 * Addresses of code and data are never literals: they are SYM(name), which
 * the generated syms_<module>.h resolves (from the linked ELF for the
 * N64-side test; from wherever the port places the symbol).
 *
 * Build switches:
 *
 *   RECOMP_TEST            instrumentation for tools/recomp/difftest.py:
 *                          address/alignment checks, integer-overflow
 *                          traps, an instruction budget, block coverage,
 *                          and a check that $ra is unchanged at `jr $ra`.
 *                          Without it these compile to nothing.
 *   RECOMP_NATIVE_ENDIAN   memory holds host-order words instead of a
 *                          byte-exact big-endian image (see docs/PORT.md);
 *                          default is big-endian, which is what the test
 *                          uses and what the ROM's data looks like.
 *   RECOMP_ACCESS          every load and store reports itself (address,
 *                          width, the bytes, the instruction's address) to
 *                          the port's access-width profiler (docs/PORT.md).
 *   RECOMP_QEMU_COMPAT     for the differential test only: where the
 *                          architecture leaves a result undefined and QEMU
 *                          (unicorn) differs from the VR4300, do what QEMU
 *                          does.  Divide by zero: LO = dividend, HI = 0
 *                          (VR4300: LO = n < 0 ? 1 : -1, HI = n; the game
 *                          only divides by zero on paths that `break 7`).
 *                          sra/srav of a value that isn't a sign-extended
 *                          word: an unextended 64-bit shift (VR4300: the
 *                          64-bit shift's low word, sign-extended).
 */
#ifndef RECOMP_H
#define RECOMP_H

#include <stdint.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RDRAM_BASE 0x80000000u
#ifndef RDRAM_SIZE
#define RDRAM_SIZE 0x00400000u
#endif

typedef struct recomp_context {
    union {
        uint64_t r[32];
        struct {
            uint64_t zero, at, v0, v1, a0, a1, a2, a3;
            uint64_t t0, t1, t2, t3, t4, t5, t6, t7;
            uint64_t s0, s1, s2, s3, s4, s5, s6, s7;
            uint64_t t8, t9, k0, k1, gp, sp, fp, ra;
        };
    };
    uint64_t hi, lo;
    /* COP1 with Status.FR = 0, as the game runs it: 32 single words, a
       double or 64-bit integer in an even/odd pair (even = low word). */
    uint32_t f[32];
    uint32_t fcr31;
    /* test instrumentation (RECOMP_TEST) */
    int64_t budget;
} recomp_context;

typedef void (*recomp_func_t)(uint8_t *rdram, recomp_context *ctx);

/* ---- hooks the embedding program provides ---------------------------- */

enum {
    RECOMP_TRAP_BREAK = 1, RECOMP_TRAP_SYSCALL, RECOMP_TRAP_OVERFLOW,
    RECOMP_TRAP_ADDRESS, RECOMP_TRAP_ALIGN, RECOMP_TRAP_TIMEOUT,
    RECOMP_TRAP_RA, RECOMP_TRAP_EXTERN, RECOMP_TRAP_BADCALL,
};

/* break/syscall, and (RECOMP_TEST) faults.  Must not return. */
void recomp_trap(recomp_context *ctx, int kind, uint32_t pc, uint32_t code)
#if defined(__GNUC__)
    __attribute__((noreturn))
#endif
    ;
/* A call from translated code to code that isn't translated (IDO C, libultra):
   `addr` is SYM(callee).  externs.c routes each callee here by default. */
void recomp_call_external(uint8_t *rdram, recomp_context *ctx, uint32_t addr);
uint64_t recomp_mfc0(recomp_context *ctx, int reg);
void recomp_mtc0(recomp_context *ctx, int reg, uint64_t value);

/* lookup of translated functions by N64 address (funcs.c) */
recomp_func_t recomp_lookup(uint32_t addr);

/* ---- helpers the generated code uses ---------------------------------- */

#define S32(x) ((uint64_t)(int64_t)(int32_t)(uint32_t)(x))
#define U32(x) ((uint32_t)(x))
#define HI16(x) (((uint32_t)(x) + 0x8000u) & 0xFFFF0000u)
#define LO16(x) ((uint64_t)(int64_t)(int16_t)(uint16_t)((uint32_t)(x) & 0xFFFFu))

#ifdef RECOMP_TEST
extern uint32_t recomp_cov[];
#define BB(id, n) do { recomp_cov[id]++; ctx->budget -= (n); \
        if (ctx->budget < 0) recomp_trap(ctx, RECOMP_TRAP_TIMEOUT, 0, 0); } while (0)
#define CHECK_RA(pc) do { if (ctx->ra != entry_ra) \
        recomp_trap(ctx, RECOMP_TRAP_RA, (pc), 0); } while (0)
#define ENTRY_RA const uint64_t entry_ra = ctx->ra
#elif defined(RECOMP_COUNT)
/* the port charges the CPU's time by instructions executed */
extern uint32_t __port_icount;
#define BB(id, n) (__port_icount += (n))
#define CHECK_RA(pc) do { } while (0)
#define ENTRY_RA do { } while (0)
#else
#define BB(id, n) do { } while (0)
#define CHECK_RA(pc) do { } while (0)
#define ENTRY_RA do { } while (0)
#endif

/* RECOMP_TRACE (debugging the test): a call before every instruction */
#ifdef RECOMP_TRACE
void recomp_trace(recomp_context *ctx, uint32_t pc);
#define TR(pc) recomp_trace(ctx, (pc))
#else
#define TR(pc) do { } while (0)
#endif

/* integer arithmetic that traps on signed overflow (add, addi, sub, dadd...).
   The game never overflows these (it would crash on hardware), so the port
   build computes them wrapping. */
static inline uint64_t recomp_add32(recomp_context *ctx, uint64_t a, uint64_t b, uint32_t pc) {
    uint64_t r = S32((uint32_t)a + (uint32_t)b);
#ifdef RECOMP_TEST
    /* For sign-extended inputs this is 32-bit signed overflow.  Other inputs
       are UNPREDICTABLE for 32-bit ops; the test follows QEMU's rule. */
    if ((int64_t)((r ^ b) & ~(a ^ b)) < 0)
        recomp_trap(ctx, RECOMP_TRAP_OVERFLOW, pc, 0);
#else
    (void)ctx; (void)pc;
#endif
    return r;
}
static inline uint64_t recomp_sub32(recomp_context *ctx, uint64_t a, uint64_t b, uint32_t pc) {
    uint64_t r = S32((uint32_t)a - (uint32_t)b);
#ifdef RECOMP_TEST
    if ((int64_t)((a ^ b) & (r ^ a)) < 0)
        recomp_trap(ctx, RECOMP_TRAP_OVERFLOW, pc, 0);
#else
    (void)ctx; (void)pc;
#endif
    return r;
}
static inline uint64_t recomp_add64(recomp_context *ctx, uint64_t a, uint64_t b, uint32_t pc) {
    int64_t r;
#ifdef RECOMP_TEST
    if (__builtin_add_overflow((int64_t)a, (int64_t)b, &r))
        recomp_trap(ctx, RECOMP_TRAP_OVERFLOW, pc, 0);
#else
    (void)ctx; (void)pc;
    r = (int64_t)(a + b);
#endif
    return (uint64_t)r;
}
static inline uint64_t recomp_sub64(recomp_context *ctx, uint64_t a, uint64_t b, uint32_t pc) {
    int64_t r;
#ifdef RECOMP_TEST
    if (__builtin_sub_overflow((int64_t)a, (int64_t)b, &r))
        recomp_trap(ctx, RECOMP_TRAP_OVERFLOW, pc, 0);
#else
    (void)ctx; (void)pc;
    r = (int64_t)(a - b);
#endif
    return (uint64_t)r;
}

/* sra/srav: the VR4300 shifts the whole 64-bit register and sign-extends
   the low word of the result (for a sign-extended input that is the plain
   32-bit shift).  Rare's fixed-point code relies on it after dmult/mflo. */
static inline uint64_t recomp_sra(uint64_t rt, unsigned sa) {
#ifdef RECOMP_QEMU_COMPAT
    if (S32(rt) != rt)
        return (uint64_t)((int64_t)rt >> sa);
#endif
    return S32((int64_t)rt >> sa);
}

/* ---- multiply / divide ------------------------------------------------ */

static inline void recomp_mult(recomp_context *ctx, uint64_t a, uint64_t b) {
    int64_t p = (int64_t)(int32_t)a * (int64_t)(int32_t)b;
    ctx->lo = S32(p); ctx->hi = S32(p >> 32);
}
static inline void recomp_multu(recomp_context *ctx, uint64_t a, uint64_t b) {
    uint64_t p = (uint64_t)(uint32_t)a * (uint64_t)(uint32_t)b;
    ctx->lo = S32(p); ctx->hi = S32(p >> 32);
}
#ifdef __SIZEOF_INT128__
static inline void recomp_dmult(recomp_context *ctx, uint64_t a, uint64_t b) {
    __int128 p = (__int128)(int64_t)a * (__int128)(int64_t)b;
    ctx->lo = (uint64_t)p; ctx->hi = (uint64_t)((unsigned __int128)p >> 64);
}
static inline void recomp_dmultu(recomp_context *ctx, uint64_t a, uint64_t b) {
    unsigned __int128 p = (unsigned __int128)a * (unsigned __int128)b;
    ctx->lo = (uint64_t)p; ctx->hi = (uint64_t)(p >> 64);
}
#else
/* 32-bit hosts (the PC port is -m32): the 128-bit product from 32-bit halves */
static inline void recomp_dmultu(recomp_context *ctx, uint64_t a, uint64_t b) {
    uint64_t al = (uint32_t)a, ah = a >> 32, bl = (uint32_t)b, bh = b >> 32;
    uint64_t ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    uint64_t mid = (ll >> 32) + (uint32_t)lh + (uint32_t)hl;
    ctx->lo = (mid << 32) | (uint32_t)ll;
    ctx->hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
}
static inline void recomp_dmult(recomp_context *ctx, uint64_t a, uint64_t b) {
    recomp_dmultu(ctx, a, b);
    /* signed high word: subtract b if a < 0 and a if b < 0 */
    if ((int64_t)a < 0) ctx->hi -= b;
    if ((int64_t)b < 0) ctx->hi -= a;
}
#endif
static inline void recomp_div(recomp_context *ctx, uint64_t a, uint64_t b) {
    int32_t n = (int32_t)a, d = (int32_t)b;
    if (d == 0) {
#ifdef RECOMP_QEMU_COMPAT
        ctx->lo = S32(n); ctx->hi = 0;
#else
        ctx->lo = S32(n < 0 ? 1 : -1); ctx->hi = S32(n);
#endif
    } else if (n == INT32_MIN && d == -1) {
        ctx->lo = S32(n); ctx->hi = 0;
    } else {
        ctx->lo = S32(n / d); ctx->hi = S32(n % d);
    }
}
static inline void recomp_divu(recomp_context *ctx, uint64_t a, uint64_t b) {
    uint32_t n = (uint32_t)a, d = (uint32_t)b;
    if (d == 0) {
#ifdef RECOMP_QEMU_COMPAT
        ctx->lo = S32(n); ctx->hi = 0;
#else
        ctx->lo = S32(0xFFFFFFFFu); ctx->hi = S32(n);
#endif
    } else {
        ctx->lo = S32(n / d); ctx->hi = S32(n % d);
    }
}
static inline void recomp_ddiv(recomp_context *ctx, uint64_t a, uint64_t b) {
    int64_t n = (int64_t)a, d = (int64_t)b;
    if (d == 0) {
#ifdef RECOMP_QEMU_COMPAT
        ctx->lo = (uint64_t)n; ctx->hi = 0;
#else
        ctx->lo = (uint64_t)(n < 0 ? 1 : -1); ctx->hi = (uint64_t)n;
#endif
    } else if (n == INT64_MIN && d == -1) {
        ctx->lo = (uint64_t)n; ctx->hi = 0;
    } else {
        ctx->lo = (uint64_t)(n / d); ctx->hi = (uint64_t)(n % d);
    }
}
static inline void recomp_ddivu(recomp_context *ctx, uint64_t a, uint64_t b) {
    if (b == 0) {
#ifdef RECOMP_QEMU_COMPAT
        ctx->lo = a; ctx->hi = 0;
#else
        ctx->lo = ~(uint64_t)0; ctx->hi = a;
#endif
    } else {
        ctx->lo = a / b; ctx->hi = a % b;
    }
}

/* ---- memory ----------------------------------------------------------- */

#ifdef RECOMP_ACCESS
/* the instruction about to access memory, then the access: width | 0x100
   for a store, and the bytes in memory order (as a host integer) */
void recomp_access_pc(uint32_t pc);
void recomp_access(uint32_t addr, uint32_t width, uint64_t raw);
#define RECOMP_ACC(a, w, raw) recomp_access((a), (w), (raw))
#else
#define RECOMP_ACC(a, w, raw) ((void)0)
#endif

/* Effective address check: a 64-bit address that is a valid sign-extended
   KSEG0 or KSEG1 address inside RDRAM, aligned to `align`. */
static inline uint32_t recomp_ea_check(recomp_context *ctx, uint64_t ea, uint32_t align, uint32_t pc) {
#ifdef RECOMP_TEST
    if ((uint64_t)(int64_t)(int32_t)ea != ea || (uint32_t)ea - RDRAM_BASE >= 0x40000000u ||
        ((uint32_t)ea & 0x1FFFFFFFu) >= RDRAM_SIZE)
        recomp_trap(ctx, RECOMP_TRAP_ADDRESS, pc, (uint32_t)ea);
    if ((uint32_t)ea & (align - 1))
        recomp_trap(ctx, RECOMP_TRAP_ALIGN, pc, (uint32_t)ea);
#else
    (void)ctx; (void)align; (void)pc;
#endif
    return (uint32_t)ea;
}

/* a load's address (`align` is its width, 1 for lwl/lwr) */
static inline uint32_t recomp_ea(recomp_context *ctx, uint64_t ea, uint32_t align, uint32_t pc) {
    uint32_t a = recomp_ea_check(ctx, ea, align, pc);
#ifdef RECOMP_ACCESS
    recomp_access_pc(pc);
#endif
    return a;
}

/* KSEG0 and KSEG1 both map RDRAM from physical 0 */
/* stores: in the test build, also refuse the code pages (the harness
   write-protects them in unicorn too; see difftest.py) */
#ifdef RECOMP_TEST
extern uint32_t recomp_ro_ranges[][2];
extern unsigned recomp_num_ro_ranges;
#endif
static inline uint32_t recomp_ea_w(recomp_context *ctx, uint64_t ea, uint32_t align, uint32_t pc) {
    uint32_t a = recomp_ea_check(ctx, ea, align, pc);
#ifdef RECOMP_ACCESS
    recomp_access_pc(pc);
#endif
#ifdef RECOMP_TEST
    for (unsigned i = 0; i < recomp_num_ro_ranges; i++)
        if ((a & 0x1FFFFFFFu) - recomp_ro_ranges[i][0] < recomp_ro_ranges[i][1] - recomp_ro_ranges[i][0])
            recomp_trap(ctx, RECOMP_TRAP_ADDRESS, pc, a);
#endif
    return a;
}

#define HOST(a) (rdram + ((uint32_t)(a) & 0x1FFFFFFFu))

#ifndef RECOMP_NATIVE_ENDIAN
static inline uint16_t recomp_be16(uint16_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap16(v);
#else
    return v;
#endif
}
static inline uint32_t recomp_be32(uint32_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(v);
#else
    return v;
#endif
}
#else
#define recomp_be16(v) (v)
#define recomp_be32(v) (v)
#endif

static inline uint32_t mem_r32_(uint8_t *rdram, uint32_t a) { uint32_t v; memcpy(&v, HOST(a), 4); return recomp_be32(v); }
static inline void mem_w32_(uint8_t *rdram, uint32_t a, uint32_t v) { v = recomp_be32(v); memcpy(HOST(a), &v, 4); }
static inline uint8_t  mem_r8 (uint8_t *rdram, uint32_t a) { uint8_t v = *HOST(a); RECOMP_ACC(a, 1, v); return v; }
static inline uint16_t mem_r16(uint8_t *rdram, uint32_t a) { uint16_t v; memcpy(&v, HOST(a), 2); RECOMP_ACC(a, 2, v); return recomp_be16(v); }
static inline uint32_t mem_r32(uint8_t *rdram, uint32_t a) { uint32_t v; memcpy(&v, HOST(a), 4); RECOMP_ACC(a, 4, v); return recomp_be32(v); }
/* doublewords are two words, high word first, in either memory mode */
static inline uint64_t mem_r64(uint8_t *rdram, uint32_t a) {
#ifdef RECOMP_ACCESS
    { uint64_t raw; memcpy(&raw, HOST(a), 8); RECOMP_ACC(a, 8, raw); }
#endif
    return ((uint64_t)mem_r32_(rdram, a) << 32) | mem_r32_(rdram, a + 4);
}
static inline void mem_w8 (uint8_t *rdram, uint32_t a, uint8_t v)  { RECOMP_ACC(a, 0x101, v); *HOST(a) = v; }
static inline void mem_w16(uint8_t *rdram, uint32_t a, uint16_t v) { v = recomp_be16(v); RECOMP_ACC(a, 0x102, v); memcpy(HOST(a), &v, 2); }
static inline void mem_w32(uint8_t *rdram, uint32_t a, uint32_t v) { v = recomp_be32(v); RECOMP_ACC(a, 0x104, v); memcpy(HOST(a), &v, 4); }
static inline void mem_w64(uint8_t *rdram, uint32_t a, uint64_t v) {
    mem_w32_(rdram, a, (uint32_t)(v >> 32)); mem_w32_(rdram, a + 4, (uint32_t)v);
#ifdef RECOMP_ACCESS
    { uint64_t raw; memcpy(&raw, HOST(a), 8); RECOMP_ACC(a, 0x108, raw); }
#endif
}

#ifndef RECOMP_NATIVE_ENDIAN
/* lwl/lwr/swl/swr, big-endian semantics on the aligned word */
static inline uint64_t mem_lwl(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t k = (a & 3) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t keep = k ? ((uint32_t)rt & ((1u << k) - 1)) : 0;
    return S32((w << k) | keep);
}
static inline uint64_t mem_lwr(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t s = (3 - (a & 3)) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t mask = 0xFFFFFFFFu >> s;
    return S32(((uint32_t)rt & ~mask) | (w >> s));
}
static inline void mem_swl(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t k = (a & 3) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t mask = 0xFFFFFFFFu >> k;
    mem_w32(rdram, a & ~3u, (w & ~mask) | ((uint32_t)rt >> k));
}
static inline void mem_swr(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t s = (3 - (a & 3)) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t mask = 0xFFFFFFFFu << s;
    mem_w32(rdram, a & ~3u, (w & ~mask) | ((uint32_t)rt << s));
}
#else
/* Native-endian memory holds a word's bytes least significant first, so
   the unaligned pairs are mirrored: `lwl rt, 0(x); lwr rt, 3(x)` loads the
   host-order word at x (what an unaligned native u32 there, or four bytes
   copied on with sw, need), and swl/swr store one the same way.  lwl
   fills the register's low bytes from x to the end of its aligned word,
   lwr the high bytes from the start of the word to x.  (Only little-endian
   hosts: a big-endian one uses the other branch.) */
static inline uint64_t mem_lwl(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t k = (a & 3) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t keep = k ? (uint32_t)rt & ~(0xFFFFFFFFu >> k) : 0;
    return S32((w >> k) | keep);
}
static inline uint64_t mem_lwr(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t s = (3 - (a & 3)) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t keep = s ? (uint32_t)rt & (0xFFFFFFFFu >> (32 - s)) : 0;
    return S32((w << s) | keep);
}
static inline void mem_swl(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t k = (a & 3) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t mask = 0xFFFFFFFFu << k;
    mem_w32(rdram, a & ~3u, (w & ~mask) | ((uint32_t)rt << k));
}
static inline void mem_swr(uint8_t *rdram, uint32_t a, uint64_t rt) {
    uint32_t s = (3 - (a & 3)) * 8, w = mem_r32(rdram, a & ~3u);
    uint32_t mask = 0xFFFFFFFFu >> s;
    mem_w32(rdram, a & ~3u, (w & ~mask) | ((uint32_t)rt >> s));
}
#endif

/* ---- COP1 (FR = 0) ---------------------------------------------------- */

static inline float fpr_s(const recomp_context *ctx, int n) { float f; memcpy(&f, &ctx->f[n], 4); return f; }
static inline uint64_t fpr_l(const recomp_context *ctx, int n) {
    return ((uint64_t)ctx->f[n + 1] << 32) | ctx->f[n];
}
static inline double fpr_d(const recomp_context *ctx, int n) { uint64_t v = fpr_l(ctx, n); double d; memcpy(&d, &v, 8); return d; }
static inline void set_fpr_l(recomp_context *ctx, int n, uint64_t v) { ctx->f[n] = (uint32_t)v; ctx->f[n + 1] = (uint32_t)(v >> 32); }
static inline void set_fpr_s(recomp_context *ctx, int n, float f) { memcpy(&ctx->f[n], &f, 4); }
static inline void set_fpr_d(recomp_context *ctx, int n, double d) { uint64_t v; memcpy(&v, &d, 8); set_fpr_l(ctx, n, v); }
static inline float f32_of(uint32_t v) { float f; memcpy(&f, &v, 4); return f; }
static inline double f64_of(uint64_t v) { double d; memcpy(&d, &v, 8); return d; }
static inline uint32_t bits_of_f32(float f) { uint32_t v; memcpy(&v, &f, 4); return v; }
static inline uint64_t bits_of_f64(double d) { uint64_t v; memcpy(&v, &d, 8); return v; }

/* NaNs, with the invalid-operation exception disabled, as MIPS (legacy NaN
   encoding: the top mantissa bit set means *signaling*) defines them: a
   quiet NaN operand is the result (the first one, if both are), a
   signaling one gives the default NaN, as does an invalid operation.  The
   host's own NaN results (x86: sign set, top mantissa bit set) never
   survive.  (The VR4300 itself takes an unimplemented-operation exception
   on NaN operands; the game doesn't do arithmetic on NaNs.) */
#define NAN_S 0x7FBFFFFFu
#define NAN_D 0x7FF7FFFFFFFFFFFFull
static inline int isnan_s(uint32_t v) { return (v & 0x7F800000u) == 0x7F800000u && (v & 0x007FFFFFu); }
static inline int issnan_s(uint32_t v) { return isnan_s(v) && (v & 0x00400000u); }
static inline int isnan_d(uint64_t v) { return (v & 0x7FF0000000000000ull) == 0x7FF0000000000000ull && (v & 0x000FFFFFFFFFFFFFull); }
static inline int issnan_d(uint64_t v) { return isnan_d(v) && (v & 0x0008000000000000ull); }
static inline uint32_t recomp_nan_s(uint32_t a, uint32_t b, int two, uint32_t r) {
    if (!isnan_s(r)) return r;
    if (issnan_s(a) || (two && issnan_s(b))) return NAN_S;
    if (isnan_s(a)) return a;
    if (two && isnan_s(b)) return b;
    return NAN_S;
}
static inline uint64_t recomp_nan_d(uint64_t a, uint64_t b, int two, uint64_t r) {
    if (!isnan_d(r)) return r;
    if (issnan_d(a) || (two && issnan_d(b))) return NAN_D;
    if (isnan_d(a)) return a;
    if (two && isnan_d(b)) return b;
    return NAN_D;
}
#define RECOMP_FOP2(name, op) \
    static inline uint32_t recomp_##name##_s(uint32_t a, uint32_t b) { \
        return recomp_nan_s(a, b, 1, bits_of_f32(f32_of(a) op f32_of(b))); } \
    static inline uint64_t recomp_##name##_d(uint64_t a, uint64_t b) { \
        return recomp_nan_d(a, b, 1, bits_of_f64(f64_of(a) op f64_of(b))); }
RECOMP_FOP2(add, +)
RECOMP_FOP2(sub, -)
RECOMP_FOP2(mul, *)
RECOMP_FOP2(div, /)
static inline uint32_t recomp_sqrt_s(uint32_t a) { return recomp_nan_s(a, 0, 0, bits_of_f32(sqrtf(f32_of(a)))); }
static inline uint64_t recomp_sqrt_d(uint64_t a) { return recomp_nan_d(a, 0, 0, bits_of_f64(sqrt(f64_of(a)))); }
/* format conversions keep a quiet NaN's sign and top payload bits */
static inline uint64_t recomp_cvt_d_s(uint32_t a) {
    if (issnan_s(a)) return NAN_D;
    if (isnan_s(a)) return ((uint64_t)(a & 0x80000000u) << 32) | 0x7FF0000000000000ull | ((uint64_t)(a & 0x007FFFFFu) << 29);
    return bits_of_f64((double)f32_of(a));
}
static inline uint32_t recomp_cvt_s_d(uint64_t a) {
    if (issnan_d(a)) return NAN_S;
    if (isnan_d(a)) return (uint32_t)((a >> 32) & 0x80000000u) | 0x7F800000u | (uint32_t)((a >> 29) & 0x007FFFFFu);
    return bits_of_f32((float)f64_of(a));
}

/* float -> integer; NaN and out-of-range give 2^31-1 / 2^63-1 as the VR4300
   does with the invalid-operation exception disabled.  `r` is the value
   already rounded to an integer by the instruction's rounding mode. */
static inline uint32_t recomp_f2w(double r) {
    if (!(r >= -2147483648.0 && r <= 2147483647.0)) return 0x7FFFFFFFu;
    return (uint32_t)(int32_t)r;
}
static inline uint64_t recomp_f2l(double r) {
    if (!(r >= -9223372036854775808.0 && r < 9223372036854775808.0)) return 0x7FFFFFFFFFFFFFFFull;
    return (uint64_t)(int64_t)r;
}
static inline uint64_t recomp_f2l_s(float r) {
    if (!(r >= -9223372036854775808.0f && r < 9223372036854775808.0f)) return 0x7FFFFFFFFFFFFFFFull;
    return (uint64_t)(int64_t)r;
}

#define FCR31_C (1u << 23)
static inline int recomp_fcmp(double a, double b, int cond) {
    int un = (a != a) || (b != b);
    return ((cond & 1) && un) || ((cond & 2) && !un && a == b) || ((cond & 4) && !un && a < b);
}
#define SET_FCC(c) (ctx->fcr31 = (c) ? (ctx->fcr31 | FCR31_C) : (ctx->fcr31 & ~FCR31_C))
#define FCC (ctx->fcr31 & FCR31_C)

#ifdef __cplusplus
}
#endif

#endif
