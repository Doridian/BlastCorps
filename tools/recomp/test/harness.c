/*
 * Test-side runtime for the translated code (built with RECOMP_TEST into
 * a shared library that difftest.py loads with ctypes).
 *
 * recomp_trap() unwinds to recomp_test_call() with longjmp; calls out of
 * the translated code, and COP0 accesses, go to callbacks the Python side
 * installs (it runs the original code for them in unicorn, on the same
 * RDRAM buffer).
 */
#include <setjmp.h>
#include <stdio.h>
#include "recomp.h"

struct recomp_func_entry { uint32_t addr; recomp_func_t fn; const char *name; };
extern const struct recomp_func_entry recomp_func_table[];
extern const unsigned recomp_num_funcs;
extern const unsigned recomp_num_blocks;

typedef int (*extern_cb_t)(recomp_context *ctx, uint32_t addr);
typedef uint64_t (*mfc0_cb_t)(int reg);

static jmp_buf trap_jmp;
static int trap_kind;
static uint32_t trap_pc, trap_code;
static extern_cb_t extern_cb;
static mfc0_cb_t mfc0_cb;
static uint8_t *cur_rdram;
uint32_t recomp_ro_ranges[4][2];
int recomp_round_half_up;       /* recomp.h: the VR4300's rounding */
unsigned recomp_num_ro_ranges;

void recomp_trap(recomp_context *ctx, int kind, uint32_t pc, uint32_t code) {
    (void)ctx;
    trap_kind = kind;
    trap_pc = pc;
    trap_code = code;
    longjmp(trap_jmp, 1);
}

void recomp_call_external(uint8_t *rdram, recomp_context *ctx, uint32_t addr) {
    (void)rdram;
    int r = extern_cb ? extern_cb(ctx, addr) : RECOMP_TRAP_EXTERN;
    if (r)
        recomp_trap(ctx, r, addr, 0);
}

uint64_t recomp_mfc0(recomp_context *ctx, int reg) {
    (void)ctx;
    return mfc0_cb ? mfc0_cb(reg) : 0;
}

void recomp_mtc0(recomp_context *ctx, int reg, uint64_t value) {
    (void)ctx; (void)reg; (void)value;
}

/* ---- API for difftest.py ---------------------------------------------- */

void recomp_test_set_callbacks(extern_cb_t e, mfc0_cb_t m) {
    extern_cb = e;
    mfc0_cb = m;
}

/* physical [lo, hi) ranges stores must not touch */
void recomp_test_set_ro(unsigned i, uint32_t lo, uint32_t hi) {
    recomp_ro_ranges[i][0] = lo;
    recomp_ro_ranges[i][1] = hi;
    if (i + 1 > recomp_num_ro_ranges)
        recomp_num_ro_ranges = i + 1;
}

unsigned recomp_test_num_funcs(void) { return recomp_num_funcs; }
unsigned recomp_test_num_blocks(void) { return recomp_num_blocks; }
uint32_t recomp_test_func_addr(unsigned i) { return recomp_func_table[i].addr; }
const char *recomp_test_func_name(unsigned i) { return recomp_func_table[i].name; }
uint32_t *recomp_test_cov(void) { return recomp_cov; }

/* Run function `i` of the table on `ctx`/`rdram`.  Returns 0 when it
   returned normally, else the RECOMP_TRAP_* kind (pc and code in *out). */
int recomp_test_call(unsigned i, recomp_context *ctx, uint8_t *rdram, uint32_t *out) {
    static recomp_context *volatile vctx;
    vctx = ctx;
    cur_rdram = rdram;
    if (setjmp(trap_jmp)) {
        out[0] = trap_pc;
        out[1] = trap_code;
        return trap_kind;
    }
    recomp_func_table[i].fn(rdram, vctx);
    return 0;
}

#ifdef RECOMP_TRACE
/* Instruction trace for difftest.py --trace: pc and the GPRs before each
   translated instruction, recorded while tracing is on. */
struct trace_rec { uint32_t pc, pad; uint64_t r[32]; };
static struct trace_rec *trace_buf;
static unsigned trace_cap, trace_len;
static int trace_on;

void recomp_trace(recomp_context *ctx, uint32_t pc) {
    if (!trace_on || trace_len >= trace_cap)
        return;
    trace_buf[trace_len].pc = pc;
    memcpy(trace_buf[trace_len].r, ctx->r, sizeof(ctx->r));
    trace_len++;
}

void recomp_test_trace(void *buf, unsigned cap) {
    trace_buf = buf;
    trace_cap = cap;
    trace_len = 0;
    trace_on = buf != NULL;
}

unsigned recomp_test_trace_len(void) { return trace_len; }
#endif
