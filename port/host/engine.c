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
#include <unistd.h>

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

/* IDO's float to unsigned int (engine.h): cvt.w.s with the FCSR set to
   truncate, its V flag read back (recomp_cvt_w_fcsr's range) */
static int ido_trunc_ok(float x, int32_t *v) {
    double r = trunc((double)x);
    if (!(r >= -2147483648.0 && r <= 2147483647.0))
        return 0;
    *v = (int32_t)r;
    return 1;
}

uint32_t engine_ido_cvt_u_s(float x) {
    int32_t v;
    if (ido_trunc_ok(x, &v))
        return v < 0 ? 0xFFFFFFFFu : (uint32_t)v;
    if (ido_trunc_ok(x - 2147483648.0f, &v))
        return (uint32_t)v | 0x80000000u;
    return 0xFFFFFFFFu;
}

extern recomp_context *port_ctx(void);

void engine_break(uint32_t pc, uint32_t code) {
    recomp_trap(port_ctx(), RECOMP_TRAP_BREAK, pc, code);
}


/* engine.h's engine_trap: the original's syscall (Rare's "can't happen"),
   which stops the game as the translation's recomp_trap does */
void engine_trap(uint32_t pc) {
    host_fatal("the engine trapped: syscall at %08X", pc);
}


#ifdef PORT_ENGINE_TAINT
/*
 * PORT_ENGINE_TAINT (CMake option, the plain 32-bit build): where what the
 * native engine leaves in the thread's context (ENGINE_LEAVE*, the
 * registers engine_restore() puts back) and in its dead frames
 * (engine_frame_sd/sdc1/sw) is read back (engine_ctx, ENGINE_REG,
 * engine_frame_lw).  Each value carries a tag: the call site (the host
 * return address) that put it there, through the restores and frame
 * stores that copied it.  ENGINE_TAINT=DIR (not PORT_: test.py drops
 * those) writes DIR/taint.<pid>.txt at exit: the tags ("N id kind site
 * parent"), every reader with the tags it saw and the first value it read
 * ("R site kind reg tag count value"), and the counts engine_probe() made
 * ("P id count"); port/tools/taint.py names the sites.  A leave, restore or
 * frame no read ever sees is scaffolding no player can see.  (O2's second
 * build, PORT_PROV, recorded the same per read; this one does both now.)
 */
#include <dlfcn.h>
#include <unistd.h>
#define TN_MAX (1u << 23)
#define TM_MAX (1u << 16)
#define TR_MAX (1u << 18)
static struct tnode { uint64_t rmask; uint32_t site, parent; uint8_t kind; } *tn;
#define TBIT(site) (1ull << ((site) * 2654435761u >> 26))
static uint32_t *tn_hash, tn_n = 1;
static struct { recomp_context *ctx; uint32_t t[66]; } tctx[PORT_MAX_THREADS + 4];
static uint32_t save_tag[256][66];
static struct { uint32_t addr, tag, sig; } tmem[TM_MAX];
/* the frames each thread's native code has open (engine_frame's sites),
   and the distinct stacks of them a frame store or read was made under:
   which frames place a store that a read sees */
#define TF_MAX 256
#define TG_MAX 4096
static struct { recomp_context *ctx; uint32_t n, site[TF_MAX]; int32_t size[TF_MAX]; } tfr[PORT_MAX_THREADS + 4];
static struct { uint32_t n, site[TF_MAX]; } tsig[TG_MAX];
static uint32_t tsig_n = 1;
static struct { uint32_t site, off, tag, ssig, rsig, count; } tlw[4096];
static struct { uint32_t site, reg, tag, count, val; uint8_t kind; } tread[TR_MAX];
static unsigned long tprobe[64];
static uintptr_t t_over;
#define T_SITE() ((uint32_t)(t_over ? t_over : (uintptr_t)__builtin_return_address(0)))

static void *tfr_of(recomp_context *ctx) {
    unsigned k;
    for (k = 0; k < sizeof tfr / sizeof tfr[0]; k++)
        if (tfr[k].ctx == ctx)
            return &tfr[k];
    for (k = 0; k < sizeof tfr / sizeof tfr[0]; k++)
        if (!tfr[k].ctx) {
            tfr[k].ctx = ctx;
            return &tfr[k];
        }
    host_fatal("taint: too many contexts");
    return NULL;
}

static void tframe(recomp_context *ctx, uint32_t site, int32_t n) {
    typeof(&tfr[0]) f = tfr_of(ctx);
    if (n < 0) {
        if (f->n < TF_MAX) {
            f->site[f->n] = site;
            f->size[f->n] = -n;
        }
        f->n++;
        return;
    }
    while (n > 0 && f->n > 0) {
        f->n--;
        n -= f->n < TF_MAX ? f->size[f->n] : 0;
    }
}

static uint32_t tsig_now(recomp_context *ctx) {
    typeof(&tfr[0]) f = tfr_of(ctx);
    uint32_t n = f->n < TF_MAX ? f->n : TF_MAX, i;
    for (i = 1; i < tsig_n; i++)
        if (tsig[i].n == n && !memcmp(tsig[i].site, f->site, n * 4))
            return i;
    if (tsig_n >= TG_MAX)
        return 0;
    tsig[tsig_n].n = n;
    memcpy(tsig[tsig_n].site, f->site, n * 4);
    return tsig_n++;
}

static void tlw_log(uint32_t site, uint32_t off, uint32_t tag, uint32_t ssig, uint32_t rsig) {
    unsigned k;
    for (k = 0; k < 4096 && tlw[k].count; k++)
        if (tlw[k].site == site && tlw[k].off == off && tlw[k].tag == tag && tlw[k].ssig == ssig &&
            tlw[k].rsig == rsig)
            break;
    if (k == 4096)
        return;
    tlw[k].site = site, tlw[k].off = off, tlw[k].tag = tag, tlw[k].ssig = ssig, tlw[k].rsig = rsig;
    tlw[k].count++;
}

static void taint_dump(void) {
    const char *d = getenv("ENGINE_TAINT");
    char path[600];
    FILE *f;
    Dl_info info;
    if (!d)
        return;
    snprintf(path, sizeof path, "%s/taint.%d.txt", d, (int)getpid());
    if (!(f = fopen(path, "w")))
        return;
    if (dladdr((void *)taint_dump, &info))
        fprintf(f, "B %s %lx\n", info.dli_fname, (unsigned long)(uintptr_t)info.dli_fbase);
    for (uint32_t i = 1; i < tn_n; i++)
        fprintf(f, "N %u %c %x %u\n", i, tn[i].kind, tn[i].site, tn[i].parent);
    for (uint32_t i = 0; i < TR_MAX; i++)
        if (tread[i].count)
            fprintf(f, "R %x %c %u %u %u %x\n", tread[i].site, tread[i].kind, tread[i].reg, tread[i].tag,
                    tread[i].count, tread[i].val);
    for (uint32_t i = 0; i < 64; i++)
        if (tprobe[i])
            fprintf(f, "P %u %lu\n", i, tprobe[i]);
    for (uint32_t i = 1; i < tsig_n; i++) {
        fprintf(f, "G %u", i);
        for (uint32_t k = 0; k < tsig[i].n; k++)
            fprintf(f, " %x", tsig[i].site[k]);
        fprintf(f, "\n");
    }
    for (uint32_t i = 0; i < 4096 && tlw[i].count; i++)
        fprintf(f, "W %x %u %u %u %u %u\n", tlw[i].site, tlw[i].off, tlw[i].tag, tlw[i].ssig, tlw[i].rsig,
                tlw[i].count);
    fclose(f);
}

static uint32_t tnode(uint8_t kind, uint32_t site, uint32_t parent) {
    uint32_t h;
    if (!tn) {
        tn = calloc(TN_MAX, sizeof *tn);
        tn_hash = calloc(TN_MAX * 2, sizeof *tn_hash);
        atexit(taint_dump);
    }
    h = (site * 2654435761u ^ parent * 40503u ^ kind) & (TN_MAX * 2 - 1);
    for (;; h = (h + 1) & (TN_MAX * 2 - 1)) {
        uint32_t i = tn_hash[h];
        if (!i)
            break;
        if (tn[i].site == site && tn[i].parent == parent && tn[i].kind == kind)
            return i;
    }
    if (tn_n >= TN_MAX)
        host_fatal("taint: too many tags");
    tn[tn_n].site = site;
    tn[tn_n].parent = parent;
    tn[tn_n].kind = kind;
    tn[tn_n].rmask = (parent ? tn[parent].rmask : 0) | (kind == 'R' ? TBIT(site) : 0);
    tn_hash[h] = tn_n;
    return tn_n++;
}

/* a restore's tag: the same as the value's when this restore is already
   in its chain (what is live is the set of sites, and nested restores
   would otherwise make a new chain on each call) */
static uint32_t trestore(uint32_t site, uint32_t parent) {
    uint32_t t;
    if (!parent || !(tn[parent].rmask & TBIT(site)))
        return tnode('R', site, parent);
    for (t = parent; t; t = tn[t].parent)
        if (tn[t].kind == 'R' && tn[t].site == site)
            return parent;
    return tnode('R', site, parent);
}

static uint32_t *ttags(recomp_context *ctx) {
    unsigned k;
    for (k = 0; k < sizeof tctx / sizeof tctx[0]; k++)
        if (tctx[k].ctx == ctx)
            return tctx[k].t;
    for (k = 0; k < sizeof tctx / sizeof tctx[0]; k++)
        if (!tctx[k].ctx) {
            tctx[k].ctx = ctx;
            return tctx[k].t;
        }
    host_fatal("taint: too many contexts");
    return NULL;
}

static uint32_t *tmem_slot(uint32_t addr) {
    uint32_t h = (addr >> 2) * 2654435761u & (TM_MAX - 1);
    for (;; h = (h + 1) & (TM_MAX - 1)) {
        if (tmem[h].addr == addr || !tmem[h].addr) {
            tmem[h].addr = addr;
            return &tmem[h].tag;
        }
    }
}

static void tread_log(uint32_t site, uint8_t kind, uint32_t reg, uint32_t tag, uint32_t val) {
    /* ENGINE_TAINT_VALUES=1: every value read is a record of its own (what
       a garbage read takes, value by value), else the first */
    static int by_value = -1;
    uint32_t h;
    if (by_value < 0)
        by_value = getenv("ENGINE_TAINT_VALUES") != NULL;
    if (!tn)
        tnode(0, 0, 0);         /* (the dump at exit) */
    h = (site * 2654435761u ^ reg * 97u ^ tag * 40503u ^ kind ^ (by_value ? val * 2246822519u : 0)) & (TR_MAX - 1);
    for (;; h = (h + 1) & (TR_MAX - 1)) {
        if (!tread[h].count) {
            tread[h].site = site, tread[h].kind = kind, tread[h].reg = reg, tread[h].tag = tag;
            tread[h].val = val;
            break;
        }
        if (tread[h].site == site && tread[h].kind == kind && tread[h].reg == reg && tread[h].tag == tag &&
            (!by_value || tread[h].val == val))
            break;
    }
    tread[h].count++;
}

/* engine.h's engine_probe(): a count a native function wants made (P
   lines in the dump) */
void engine_probe(unsigned id) {
    if (!tn)
        tnode(0, 0, 0);         /* (the dump at exit) */
    if (id < 64)
        tprobe[id]++;
}
#endif

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
#ifdef PORT_ENGINE_TAINT
    memcpy(save_tag[save_n], ttags(ctx), sizeof save_tag[0]);
#endif
    save_n++;
}

uint32_t engine_ctx(unsigned int reg) {
#ifdef PORT_ENGINE_TAINT
    if (reg < 32)
        tread_log(T_SITE(), 'c', reg, ttags(port_ctx())[reg], (uint32_t)port_ctx()->r[reg]);
#endif
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
#ifdef PORT_ENGINE_TAINT
    {
        uint32_t *t = ttags(ctx), site = T_SITE();
        for (k = 0; k < 32; k++) {
            if (save_stack[n].gmask >> k & 1)
                t[k] = trestore(site, save_tag[n][k]);
            if (save_stack[n].fmask >> k & 1)
                t[34 + k] = trestore(site, save_tag[n][34 + k]);
        }
        for (k = n; k < save_n - 1; k++)
            memcpy(save_tag[k], save_tag[k + 1], sizeof save_tag[0]);
    }
#endif
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
#ifdef PORT_ENGINE_TAINT
    if (reg && reg < 66)
        ttags(ctx)[reg] = tnode('L', T_SITE(), 0);
#endif
    if (reg < 32) {
        if (reg)
            ctx->r[reg] = S32(value);
    } else if (reg >= 34 && reg < 66)
        ctx->f[reg - 34] = value;
}

void engine_leave64(unsigned int reg, uint32_t lo, uint32_t hi) {
    recomp_context *ctx = port_ctx();
#ifdef PORT_ENGINE_TAINT
    if (reg && reg < 32)
        ttags(ctx)[reg] = tnode('L', T_SITE(), 0);
#endif
    if (reg && reg < 32)
        ctx->r[reg] = (uint64_t)hi << 32 | lo;
}

/* engine.h's engine_frame and its stores: the original's stack frame, in
   RDRAM as the translated code's addiu, sd, sdc1 and sw leave it */
uint32_t engine_frame(int32_t n) {
    recomp_context *ctx = port_ctx();
#ifdef PORT_ENGINE_TAINT
    tframe(ctx, T_SITE(), n);
#endif
    ctx->sp = S32((uint32_t)ctx->sp + (uint32_t)n);
    return (uint32_t)ctx->sp;
}

void engine_frame_sd(uint32_t off, unsigned int reg) {
    recomp_context *ctx = port_ctx();
    uint8_t *rdram = RDRAM;
#ifdef PORT_ENGINE_TAINT
    {
        uint32_t t = tnode('D', T_SITE(), reg < 32 ? ttags(ctx)[reg] : 0), g = tsig_now(ctx);
        *tmem_slot((uint32_t)ctx->sp + off) = t;
        *tmem_slot((uint32_t)ctx->sp + off + 4) = t;
        tmem_slot((uint32_t)ctx->sp + off)[1] = g;
        tmem_slot((uint32_t)ctx->sp + off + 4)[1] = g;
    }
#endif
    mem_w64(rdram, (uint32_t)ctx->sp + off, reg < 32 ? ctx->r[reg] : 0);
}

void engine_frame_sdc1(uint32_t off, unsigned int fpr) {
    recomp_context *ctx = port_ctx();
    uint8_t *rdram = RDRAM;
#ifdef PORT_ENGINE_TAINT
    {
        uint32_t t = tnode('F', T_SITE(), fpr < 32 ? ttags(ctx)[34 + fpr] : 0), g = tsig_now(ctx);
        *tmem_slot((uint32_t)ctx->sp + off) = t;
        *tmem_slot((uint32_t)ctx->sp + off + 4) = t;
        tmem_slot((uint32_t)ctx->sp + off)[1] = g;
        tmem_slot((uint32_t)ctx->sp + off + 4)[1] = g;
    }
#endif
    mem_w64(rdram, (uint32_t)ctx->sp + off, fpr_l(ctx, (int)fpr));
}

void engine_frame_sw(uint32_t off, uint32_t v) {
    recomp_context *ctx = port_ctx();
    uint8_t *rdram = RDRAM;
#ifdef PORT_ENGINE_TAINT
    *tmem_slot((uint32_t)ctx->sp + off) = tnode('W', T_SITE(), 0);
    tmem_slot((uint32_t)ctx->sp + off)[1] = tsig_now(ctx);
#endif
    mem_w32(rdram, (uint32_t)ctx->sp + off, v);
}

uint32_t engine_frame_lw(uint32_t off) {
    recomp_context *ctx = port_ctx();
    uint8_t *rdram = RDRAM;
#ifdef PORT_ENGINE_TAINT
    tread_log(T_SITE(), 'l', off, *tmem_slot((uint32_t)ctx->sp + off), mem_r32(rdram, (uint32_t)ctx->sp + off));
    tlw_log(T_SITE(), off, *tmem_slot((uint32_t)ctx->sp + off), tmem_slot((uint32_t)ctx->sp + off)[1],
            tsig_now(ctx));
    {
        /* ENGINE_LWLOG=FILE: every word read, to compare two builds */
        static FILE *lw;
        static int init;
        if (!init) {
            const char *p = getenv("ENGINE_LWLOG");
            init = 1;
            lw = p ? fopen(p, "w") : NULL;
        }
        if (lw)
            fprintf(lw, "%x %08x\n", off, mem_r32(rdram, (uint32_t)ctx->sp + off));
    }
#endif
    return mem_r32(rdram, (uint32_t)ctx->sp + off);
}

/* engine.h's: the frame of 0x58 and 0x30 Rare's code saves $ra,
   $s0..$s7, $gp, $fp and $f20..$f31 in */
void engine_frame_s(void) {
    unsigned int k;
#ifdef PORT_ENGINE_TAINT
    t_over = (uintptr_t)__builtin_return_address(0);
#endif
    engine_frame(-0x58);
    engine_frame_sd(0, 31);
    for (k = 0; k < 8; k++)
        engine_frame_sd(8 + 8 * k, 16 + k);
    engine_frame_sd(0x48, 28);
    engine_frame_sd(0x50, 30);
    engine_frame(-0x30);
    for (k = 0; k < 6; k++)
        engine_frame_sdc1(8 * k, 20 + 2 * k);
#ifdef PORT_ENGINE_TAINT
    t_over = 0;
#endif
}

/* engine.h's: a COP0 register, as the translated code's mfc0 reads it */
uint32_t engine_mfc0(unsigned int reg) {
    return (uint32_t)recomp_mfc0(port_ctx(), (int)reg);
}

/* engine.h's ENGINE_REG: what the context holds, in the same numbering */
uint32_t engine_reg(unsigned int reg) {
    recomp_context *ctx = port_ctx();
#ifdef PORT_ENGINE_TAINT
    if (reg < 66)
        tread_log(T_SITE(), 'r', reg, ttags(ctx)[reg],
                  reg < 32 ? (uint32_t)ctx->r[reg] : reg >= 34 ? ctx->f[reg - 34] : 0);
#endif
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
 * below the N64 $sp, and the registers the convention says a caller
 * reads.  On a difference it says where.  The game goes on with the
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

int engine_tracing;
static int active = -1;             /* the function being checked */
static recomp_context ctx0, ctx_o;
static uint8_t *mem0, *mem_o;
static unsigned long *ncalls, *nchecked, *nfailed;
static unsigned long first_n = 2000;
static unsigned long show_n = 10;     /* differences shown per function (PORT_ENGINE_CHECK_SHOW) */
/* the host stack above the glue's frame: the game's C's locals, which a
   pointer argument may reach (a fiber stack, port.h) */
static uintptr_t stk_lo, stk_hi;
static uint8_t *stk0, *stk_o;
/* the PI's pending completions (port/src/ultra.c's pieces), copied out
   and back */
void *port_pi_piece(int k);
uint32_t port_pi_piece_size(int k);
static uint8_t *pi0;

static uint8_t *pi_copy(uint8_t *buf, int load) {
    uint32_t n, total = 0;
    void *p;
    for (int k = 0; port_pi_piece(k) != NULL; k++)
        total += port_pi_piece_size(k);
    if (!buf)
        buf = malloc(total);
    total = 0;
    for (int k = 0; (p = port_pi_piece(k)) != NULL; k++) {
        n = port_pi_piece_size(k);
        if (load)
            memcpy(p, buf + total, n);
        else
            memcpy(buf + total, p, n);
        total += n;
    }
    return buf;
}

static void cov_write(void);

static void report(void) {
    unsigned long tot = 0, bad = 0;
    cov_write();
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
    stk0 = malloc(PORT_STACK_SIZE);
    stk_o = malloc(PORT_STACK_SIZE);
    pi0 = pi_copy(NULL, 0);
    ncalls = calloc(engine_check_count + 1, sizeof *ncalls);
    nchecked = calloc(engine_check_count + 1, sizeof *nchecked);
    nfailed = calloc(engine_check_count + 1, sizeof *nfailed);
    const char *e = getenv("PORT_ENGINE_CHECK");
    if (e)
        first_n = strtoul(e, NULL, 0);
    if ((e = getenv("PORT_ENGINE_CHECK_SHOW")) != NULL)
        show_n = strtoul(e, NULL, 0);
    atexit(report);
}

/* PORT_ENGINE_COV=FILE: the ids of the blocks the checked calls'
   translations ran, one a line, at exit (what the fuzz reached: cov.py-like
   counts per function) */
#define COV_MAX 0x10000u
static uint8_t cov_hit[COV_MAX];

void engine_trace_blk(unsigned int id) {
    if (engine_tracing && id < COV_MAX)
        cov_hit[id] = 1;
}

static void cov_write(void) {
    const char *p = getenv("PORT_ENGINE_COV");
    FILE *f;
    if (!p || !(f = fopen(p, "w")))
        return;
    for (unsigned i = 0; i < COV_MAX; i++)
        if (cov_hit[i])
            fprintf(f, "%u\n", i);
    fclose(f);
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
    pi_copy(pi0, 0);
    engine_tracing = 1;
    return 1;
}

void engine_check_mid(unsigned id, recomp_context *ctx) {
    (void)id;
    ctx_o = *ctx;
    memcpy(mem_o, RDRAM_P, PORT_RDRAM_SIZE);
    *ctx = ctx0;
    memcpy(RDRAM_P, mem0, PORT_RDRAM_SIZE);
    if (stk_hi) {
        memcpy(stk_o, (void *)stk_lo, stk_hi - stk_lo);
        memcpy((void *)stk_lo, stk0, stk_hi - stk_lo);
    }
    pi_copy(pi0, 1);
    engine_tracing = 0;
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

/* Whether the words at byte k in the two runs' memory are both addresses
   on the host stacks: the C a checked function calls keeps its locals
   there, at another depth under the translation's glue than under the
   native code, and a message it sends may be one's address (the PI's
   OSIoMesg, which stays behind in its queue's buffer). */
static int host_stack_words(uint32_t k) {
    uint32_t w = k & ~3u, a, b;
    if (w + 4 > PORT_RDRAM_SIZE)
        return 0;
    a = (uint32_t)mem_o[w] << 24 | mem_o[w + 1] << 16 | mem_o[w + 2] << 8 | mem_o[w + 3];
    b = (uint32_t)RDRAM_P[w] << 24 | RDRAM_P[w + 1] << 16 | RDRAM_P[w + 2] << 8 | RDRAM_P[w + 3];
    return a - PORT_STACK_BASE < (uint32_t)PORT_STACK_SIZE * PORT_MAX_THREADS &&
           b - PORT_STACK_BASE < (uint32_t)PORT_STACK_SIZE * PORT_MAX_THREADS;
}

void engine_check_end(unsigned id, recomp_context *ctx, uint64_t m0, uint64_t m1, uint64_t m2) {
    if (active != (int)id)
        return;
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
            if (mem_o[k] != RDRAM_P[k] && !(k >= dlo && k < sp) && !host_stack_words(k)) {
                if (!nbytes)
                    first = k;
                nbytes++;
            }
    }
    if (nbytes && len < sizeof what - 96) {
        uint32_t w = first & ~3u;
        len += snprintf(what + len, sizeof what - len, " memory: %lu bytes, first %08X (%02X/%02X, word %02X%02X%02X%02X/%02X%02X%02X%02X)",
                        nbytes, first | 0x80000000u, mem_o[first], RDRAM_P[first], mem_o[w], mem_o[w + 1],
                        mem_o[w + 2], mem_o[w + 3], RDRAM_P[w], RDRAM_P[w + 1], RDRAM_P[w + 2], RDRAM_P[w + 3]);
    }
    if (stk_hi && memcmp(stk_o, (void *)stk_lo, stk_hi - stk_lo) && len < sizeof what - 64) {
        uintptr_t k = 0;
        while (stk_o[k] == ((uint8_t *)stk_lo)[k])
            k++;
        len += snprintf(what + len, sizeof what - len, " host stack: first %08lX", (unsigned long)(stk_lo + k));
    }
    if (!len)
        return;
    if (nfailed[id]++ < show_n)
        fprintf(stderr, "engine check: %s (call %lu) differs (translation/native):%s\n",
                engine_check_names[id], ncalls[id] - 1, what);
}

/*
 * The fuzz: PORT_ENGINE_FUZZ=READ[:TRIALS[:SEED]] has the native engine's
 * driver (engine_fuzz, port/engine/fuzz_*.c, where a version has one) call
 * replaced functions at the READth controller read, on game memory it
 * varies, every call checked as above (with PORT_ENGINE_CHECK=0): paths no
 * run reaches, a jp results screen in the attract mode, say.  Before each
 * trial, and after the last, game memory and the context are put back as
 * they were at the read, so the run goes on as without it.
 */
static uint8_t *fuzz_mem, *fuzz_pi;
static recomp_context fuzz_ctx;

void engine_fuzz_reset(void) {
    memcpy(RDRAM_P, fuzz_mem, PORT_RDRAM_SIZE);
    pi_copy(fuzz_pi, 1);
    *port_ctx() = fuzz_ctx;
}

void engine_fuzz(unsigned trials, unsigned seed) __attribute__((weak));

void engine_fuzz_poll(unsigned polls) {
    static int init;
    static unsigned at, trials = 100, seed = 1;
    if (!init) {
        const char *e = getenv("PORT_ENGINE_FUZZ");
        init = 1;
        if (e)
            sscanf(e, "%u:%u:%u", &at, &trials, &seed);
    }
    if (!at || polls != at)
        return;
    if (!engine_fuzz) {
        fprintf(stderr, "engine fuzz: no driver in this version\n");
        return;
    }
    fuzz_mem = malloc(PORT_RDRAM_SIZE);
    memcpy(fuzz_mem, RDRAM_P, PORT_RDRAM_SIZE);
    fuzz_pi = pi_copy(NULL, 0);
    fuzz_ctx = *port_ctx();
    engine_fuzz(trials, seed);
    engine_fuzz_reset();
    free(fuzz_mem);
    free(fuzz_pi);
    fuzz_mem = fuzz_pi = NULL;
    fprintf(stderr, "engine fuzz: %u trials at read %u\n", trials, polls);
}
#endif
