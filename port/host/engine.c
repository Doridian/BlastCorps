/*
 * The engine's replacement, host side (docs/PORT.md, "Replacing the
 * engine"): the VR4300's float conversions as the translated code does
 * them, for the native code (port/engine/engine.h), and the check build's
 * comparison of each replaced function with its translation
 * (PORT_ENGINE_CHECK).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "recomp.h"
#include "host.h"
#include "port.h"
#include "port_recomp.h"

int32_t engine_cvt_w_s(float x) { return (int32_t)recomp_f2w(recomp_rintf(x)); }
int32_t engine_cvt_w_d(double x) { return (int32_t)recomp_f2w(recomp_rint(x)); }
int32_t engine_trunc_w_s(float x) { return (int32_t)recomp_f2w(truncf(x)); }
int32_t engine_trunc_w_d(double x) { return (int32_t)recomp_f2w(trunc(x)); }
int64_t engine_cvt_l_d(double x) { return (int64_t)recomp_f2l(recomp_rint(x)); }
int64_t engine_cvt_l_s(float x) { return (int64_t)recomp_f2l_s(recomp_rintf(x)); }

extern recomp_context *port_ctx(void);

void engine_break(uint32_t pc, uint32_t code) {
    recomp_trap(port_ctx(), RECOMP_TRAP_BREAK, pc, code);
}

#ifdef PORT_BLKLOG
/* PORT_BLKLOG=FILE:FROM:TO:LO-HI,LO-HI...: from the FROMth controller read
   to the TOth, the ids of the blocks charged (translated or native) in the
   ranges given, one a line, and "P n" at the nth read (port/host/main.c);
   the run ends at the TOth. */
static FILE *blklog_f;
static unsigned blklog_from, blklog_to, blklog_n;
static unsigned blklog_r[64][2];
static int blklog_on;

static void blklog_init(void) {
    static int done;
    char path[512];
    const char *s = getenv("PORT_BLKLOG"), *p;
    int k;
    if (done)
        return;
    done = 1;
    if (!s || sscanf(s, "%511[^:]:%u:%u:%n", path, &blklog_from, &blklog_to, &k) != 3)
        return;
    for (p = s + k; *p && blklog_n < 64;) {
        char *e;
        blklog_r[blklog_n][0] = (unsigned)strtoul(p, &e, 10);
        if (*e != '-')
            break;
        blklog_r[blklog_n][1] = (unsigned)strtoul(e + 1, &e, 10);
        blklog_n++;
        p = *e == ',' ? e + 1 : e;
    }
    blklog_f = fopen(path, "w");
}

/* PORT_BLKLOG_REGS=ID: at block ID, the context's GPRs too (as the
   translated code would see them) */

void port_blklog(unsigned int id) {
    static long regs_at = -2;
    unsigned k;
    if (!blklog_on)
        return;
    if (regs_at == -2) {
        const char *s = getenv("PORT_BLKLOG_REGS");
        regs_at = s ? atol(s) : -1;
    }
    for (k = 0; k < blklog_n; k++)
        if (id >= blklog_r[k][0] && id <= blklog_r[k][1]) {
            fprintf(blklog_f, "%u\n", id);
            if ((long)id == regs_at) {
                recomp_context *ctx = port_ctx();
                int r;
                fprintf(blklog_f, "R");
                for (r = 1; r < 32; r++)
                    fprintf(blklog_f, " %d=%llx", r, (unsigned long long)ctx->r[r]);
                fprintf(blklog_f, "\n");
            }
            return;
        }
}

void port_blklog_poll(unsigned int n) {
    blklog_init();
    if (!blklog_f)
        return;
    blklog_on = n >= blklog_from && n < blklog_to;
    if (n >= blklog_from)
        fprintf(blklog_f, "P %u\n", n);
    if (n >= blklog_to) {
        fclose(blklog_f);
        exit(0);
    }
}
#endif

/* engine.h's engine_trap: the original's syscall (Rare's "can't happen"),
   which stops the game as the translation's recomp_trap does */
void engine_trap(uint32_t pc) {
    host_fatal("the engine trapped: syscall at %08X", pc);
}

/* engine.h's ENGINE_SAVE/ENGINE_RESTORE: the registers the original saves
   on its stack and loads back before it returns (gmask: GPRs, fmask: FPR
   words), kept here for the thread's context, so that what its translated
   callees leave in them is undone as the original undoes it.  Nested as
   the calls are; each thread's own (a context switch inside one is
   possible, through the game's C). */
static struct {
    recomp_context *ctx;
    uint32_t gmask, fmask;
    uint64_t r[32];
    uint32_t f[32];
} save_stack[256];
static int save_n;

void engine_save(uint32_t gmask, uint32_t fmask) {
    recomp_context *ctx = port_ctx();
    int k;
    if (save_n >= (int)(sizeof save_stack / sizeof save_stack[0]))
        host_fatal("engine_save: too deep");
    save_stack[save_n].ctx = ctx;
    save_stack[save_n].gmask = gmask;
    save_stack[save_n].fmask = fmask;
    for (k = 0; k < 32; k++) {
        if (gmask >> k & 1)
            save_stack[save_n].r[k] = ctx->r[k];
        if (fmask >> k & 1)
            save_stack[save_n].f[k] = ctx->f[k];
    }
    save_n++;
}

uint32_t engine_ctx(unsigned int reg) {
    return reg < 32 ? (uint32_t)port_ctx()->r[reg] : 0;
}

void engine_restore(void) {
    recomp_context *ctx = port_ctx();
    int n, k;
    for (n = save_n - 1; n >= 0 && save_stack[n].ctx != ctx; n--)
        ;
    if (n < 0)
        host_fatal("engine_restore: nothing saved");
    for (k = 0; k < 32; k++) {
        if (save_stack[n].gmask >> k & 1)
            ctx->r[k] = save_stack[n].r[k];
        if (save_stack[n].fmask >> k & 1)
            ctx->f[k] = save_stack[n].f[k];
    }
    for (; n < save_n - 1; n++)
        save_stack[n] = save_stack[n + 1];
    save_n--;
}

void engine_syscall(uint32_t pc) {
    recomp_trap(port_ctx(), RECOMP_TRAP_SYSCALL, pc, 0);
}

/* engine.h's ENGINE_LEAVE: 0-31 a GPR (the word sign-extended, as the
   VR4300 holds one), 34-65 an FPR word */
void engine_leave(unsigned int reg, uint32_t value) {
    recomp_context *ctx = port_ctx();
    if (reg < 32) {
        if (reg)
            ctx->r[reg] = S32(value);
    } else if (reg >= 34 && reg < 66)
        ctx->f[reg - 34] = value;
}

void engine_leave64(unsigned int reg, uint32_t lo, uint32_t hi) {
    recomp_context *ctx = port_ctx();
    if (reg && reg < 32)
        ctx->r[reg] = (uint64_t)hi << 32 | lo;
}

/* engine.h's engine_frame and its stores: the original's stack frame, in
   RDRAM as the translated code's addiu, sd, sdc1 and sw leave it */
uint32_t engine_frame(int32_t n) {
    recomp_context *ctx = port_ctx();
    ctx->sp = S32((uint32_t)ctx->sp + (uint32_t)n);
    return (uint32_t)ctx->sp;
}

void engine_frame_sd(uint32_t off, unsigned int reg) {
    recomp_context *ctx = port_ctx();
    uint8_t *rdram = RDRAM;
    mem_w64(rdram, (uint32_t)ctx->sp + off, reg < 32 ? ctx->r[reg] : 0);
}

void engine_frame_sdc1(uint32_t off, unsigned int fpr) {
    recomp_context *ctx = port_ctx();
    uint8_t *rdram = RDRAM;
    mem_w64(rdram, (uint32_t)ctx->sp + off, fpr_l(ctx, (int)fpr));
}

void engine_frame_sw(uint32_t off, uint32_t v) {
    recomp_context *ctx = port_ctx();
    uint8_t *rdram = RDRAM;
    mem_w32(rdram, (uint32_t)ctx->sp + off, v);
}

/* engine.h's: a COP0 register, as the translated code's mfc0 reads it */
uint32_t engine_mfc0(unsigned int reg) {
    return (uint32_t)recomp_mfc0(port_ctx(), (int)reg);
}

/* engine.h's ENGINE_REG: what the context holds, in the same numbering */
uint32_t engine_reg(unsigned int reg) {
    recomp_context *ctx = port_ctx();
    if (reg < 32)
        return (uint32_t)ctx->r[reg];
    if (reg >= 34 && reg < 66)
        return ctx->f[reg - 34];
    return 0;
}

#ifdef PORT_ENGINE_CHECK
/*
 * Each call of a replaced function, from the translated code (its glue,
 * recomp_extern_X) or the game's C (the glue's func_X; the native one is
 * native_func_X in this build), may be checked: the RDRAM and the context
 * are saved, the translation (recomp_orig_X) runs, its RDRAM, context and
 * instruction count are kept, everything is put back, the native function
 * runs, and the two are compared: every byte of RDRAM but the dead stack
 * below the N64 $sp, the registers the convention says a caller reads,
 * and the instructions charged (translated code and native code both
 * charge __port_icount, the game's C __port_icount_c).  On a difference
 * it says where, and where the two runs' blocks first part (both charge
 * block by block: BB() and ENGINE_BLK()).  The game goes on with the
 * native function's results.
 *
 * Only functions whose translation calls nothing but translated code are
 * checked (gen_glue.py): a call into the game's C or libultra can't be
 * run twice.  PORT_ENGINE_CHECK=N (environment) checks the first N calls
 * of each function and every 64th after (default 2000); =0 checks every
 * call.
 */
extern const char *const engine_check_names[];
extern const unsigned engine_check_count;

#define RDRAM_P ((uint8_t *)(uintptr_t)PORT_RDRAM_BASE)
#define DEAD_STACK 0x10000u          /* below the $sp: nobody's any more */
#define TRACE_MAX (1u << 20)

int engine_tracing;
static int active = -1;             /* the function being checked */
static recomp_context ctx0, ctx_o;
static uint8_t *mem0, *mem_o;
static uint32_t ic0, icc0, ic_o, icc_o;
static uint32_t *trace_o, *trace_n, *trace_cur;
static unsigned ntrace_o, ntrace_n, *ntrace_cur;
static unsigned long *ncalls, *nchecked, *nfailed;
static unsigned long first_n = 2000;
/* the host stack above the glue's frame: the game's C's locals, which a
   pointer argument may reach (a fiber stack, port.h) */
static uintptr_t stk_lo, stk_hi;
static uint8_t *stk0, *stk_o;

extern uint32_t __port_icount, __port_icount_c;

static void report(void) {
    unsigned long tot = 0, bad = 0;
    for (unsigned i = 0; i < engine_check_count; i++) {
        tot += nchecked[i];
        bad += nfailed[i];
    }
    fprintf(stderr, "engine check: %lu calls checked, %lu differ\n", tot, bad);
    for (unsigned i = 0; i < engine_check_count; i++)
        if (ncalls[i])
            fprintf(stderr, "  %s: %lu calls, %lu checked, %lu differ\n", engine_check_names[i],
                    ncalls[i], nchecked[i], nfailed[i]);
}

static void init(void) {
    mem0 = malloc(PORT_RDRAM_SIZE);
    mem_o = malloc(PORT_RDRAM_SIZE);
    trace_o = malloc(TRACE_MAX * sizeof *trace_o);
    trace_n = malloc(TRACE_MAX * sizeof *trace_n);
    stk0 = malloc(PORT_STACK_SIZE);
    stk_o = malloc(PORT_STACK_SIZE);
    ncalls = calloc(engine_check_count + 1, sizeof *ncalls);
    nchecked = calloc(engine_check_count + 1, sizeof *nchecked);
    nfailed = calloc(engine_check_count + 1, sizeof *nfailed);
    const char *e = getenv("PORT_ENGINE_CHECK");
    if (e)
        first_n = strtoul(e, NULL, 0);
    atexit(report);
}

void engine_trace_blk(unsigned int id) {
    if (engine_tracing && *ntrace_cur < TRACE_MAX)
        trace_cur[(*ntrace_cur)++] = id;
}

int engine_checking(void) { return active >= 0; }

int engine_check_begin(unsigned id, recomp_context *ctx, void *frame) {
    if (!mem0)
        init();
    if (active >= 0)
        return 0;               /* (a replaced function the one checked calls) */
    unsigned long n = ncalls[id]++;
    if (first_n && n >= first_n && n % 64)
        return 0;
    active = (int)id;
    ctx0 = *ctx;
    memcpy(mem0, RDRAM_P, PORT_RDRAM_SIZE);
    uintptr_t f = (uintptr_t)frame;
    stk_lo = stk_hi = 0;
    if (f - PORT_STACK_BASE < (uintptr_t)PORT_STACK_SIZE * PORT_MAX_THREADS) {
        stk_lo = f;
        stk_hi = PORT_STACK_BASE + ((f - PORT_STACK_BASE) / PORT_STACK_SIZE + 1) * PORT_STACK_SIZE;
        memcpy(stk0, (void *)stk_lo, stk_hi - stk_lo);
    }
    ic0 = __port_icount;
    icc0 = __port_icount_c;
    ntrace_o = ntrace_n = 0;
    trace_cur = trace_o;
    ntrace_cur = &ntrace_o;
    engine_tracing = 1;
    return 1;
}

void engine_check_mid(unsigned id, recomp_context *ctx) {
    (void)id;
    ctx_o = *ctx;
    memcpy(mem_o, RDRAM_P, PORT_RDRAM_SIZE);
    ic_o = __port_icount - ic0;
    icc_o = __port_icount_c - icc0;
    *ctx = ctx0;
    memcpy(RDRAM_P, mem0, PORT_RDRAM_SIZE);
    if (stk_hi) {
        memcpy(stk_o, (void *)stk_lo, stk_hi - stk_lo);
        memcpy((void *)stk_lo, stk0, stk_hi - stk_lo);
    }
    __port_icount = ic0;
    __port_icount_c = icc0;
    trace_cur = trace_n;
    ntrace_cur = &ntrace_n;
}

static uint64_t reg_of(const recomp_context *c, unsigned r) {
    return r < 32 ? c->r[r] : r == 32 ? c->hi : r == 33 ? c->lo : c->f[r - 34];
}

static const char *reg_name(unsigned r) {
    static const char *g[] = { "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3",
                               "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
                               "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra" };
    static char buf[8];
    if (r < 32)
        return g[r];
    if (r == 32)
        return "hi";
    if (r == 33)
        return "lo";
    snprintf(buf, sizeof buf, "f%u", r - 34);
    return buf;
}

void engine_check_end(unsigned id, recomp_context *ctx, uint64_t m0, uint64_t m1, uint64_t m2) {
    if (active != (int)id)
        return;
    engine_tracing = 0;
    active = -1;
    nchecked[id]++;
    uint64_t mask[3] = { m0, m1, m2 };
    char what[512];
    size_t len = 0;
    what[0] = 0;
    for (unsigned r = 0; r < 66; r++)
        if (mask[r / 64] >> (r % 64) & 1) {
            uint64_t a = reg_of(&ctx_o, r), b = reg_of(ctx, r);
            if (a != b && len < sizeof what - 64)
                len += snprintf(what + len, sizeof what - len, " %s %llX/%llX", reg_name(r),
                                (unsigned long long)a, (unsigned long long)b);
        }
    uint32_t sp = (uint32_t)ctx0.sp & 0x1FFFFFFFu;
    uint32_t dlo = sp > DEAD_STACK ? sp - DEAD_STACK : 0;
    unsigned long nbytes = 0;
    uint32_t first = 0;
    for (uint32_t a = 0; a < PORT_RDRAM_SIZE; a += 4096) {
        if (!memcmp(mem_o + a, RDRAM_P + a, 4096))
            continue;
        for (uint32_t k = a; k < a + 4096; k++)
            if (mem_o[k] != RDRAM_P[k] && !(k >= dlo && k < sp)) {
                if (!nbytes)
                    first = k;
                nbytes++;
            }
    }
    if (nbytes && len < sizeof what - 64)
        len += snprintf(what + len, sizeof what - len, " memory: %lu bytes, first %08X (%02X/%02X)", nbytes,
                        first | 0x80000000u, mem_o[first], RDRAM_P[first]);
    if (stk_hi && memcmp(stk_o, (void *)stk_lo, stk_hi - stk_lo) && len < sizeof what - 64) {
        uintptr_t k = 0;
        while (stk_o[k] == ((uint8_t *)stk_lo)[k])
            k++;
        len += snprintf(what + len, sizeof what - len, " host stack: first %08lX", (unsigned long)(stk_lo + k));
    }
    uint32_t ic_n = __port_icount - ic0, icc_n = __port_icount_c - icc0;
    if ((ic_n != ic_o || icc_n != icc_o) && len < sizeof what - 64)
        len += snprintf(what + len, sizeof what - len, " cost: %u+%u/%u+%u", ic_o, icc_o, ic_n, icc_n);
    if (!len)
        return;
    if (nfailed[id]++ < 10) {
        fprintf(stderr, "engine check: %s (call %lu) differs (translation/native):%s\n",
                engine_check_names[id], ncalls[id] - 1, what);
        unsigned k = 0;
        while (k < ntrace_o && k < ntrace_n && trace_o[k] == trace_n[k])
            k++;
        if (k < ntrace_o || k < ntrace_n)
            fprintf(stderr, "  blocks (blocks.tsv ids) part after %u: translation %d, native %d (of %u, %u)\n", k,
                    k < ntrace_o ? (int)trace_o[k] : -1, k < ntrace_n ? (int)trace_n[k] : -1, ntrace_o, ntrace_n);
    }
}
#endif
