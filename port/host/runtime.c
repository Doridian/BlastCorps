/*
 * Runtime support: the hooks the translated code needs, and the startup
 * byte swaps that put initialized data into big-endian order.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

/* ---- startup fixups ------------------------------------------------------ */

/* BEPass: runs of equally sized scalars in C initializers, swapped from the
   compiler's host order (called from each module's constructor) */
struct bepass_entry {
    uint8_t *p;
    uint32_t count, size;
};

static void swap_bytes(uint8_t *q, uint32_t size) {
    for (uint32_t a = 0, b = size - 1; a < b; a++, b--) {
        uint8_t t = q[a];
        q[a] = q[b];
        q[b] = t;
    }
}

void __bepass_fixup(const struct bepass_entry *e, uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
        for (uint32_t k = 0; k < e[i].count; k++)
            swap_bytes(e[i].p + k * e[i].size, e[i].size);
}

/* BEPass in native mode (PORT_NATIVE_ENDIAN): the 64-bit scalars in C
   initializers, whose words go high one first */
void __bepass_fixup_rot64(const struct bepass_entry *e, uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
        for (uint32_t k = 0; k < e[i].count; k++) {
            uint32_t w[2];
            memcpy(w, e[i].p + k * 8, 8);
            uint32_t t = w[0];
            w[0] = w[1];
            w[1] = t;
            memcpy(e[i].p + k * 8, w, 8);
        }
}

/* asm2x86.py: symbolic .words in the game's data, in host order until now */
extern uint32_t __start_port_bswap32[] __attribute__((weak));
extern uint32_t __stop_port_bswap32[] __attribute__((weak));

void port_fixups(void) {
    for (uint32_t *p = __start_port_bswap32; p < __stop_port_bswap32; p++)
        swap_bytes((uint8_t *)(uintptr_t)*p, 4);
}

/* A thread's host stack: inside the KSEG0 window (at PORT_STACK_BASE,
   port.h), so that the address of a local means the same to the translated
   code; in the movable build that range is in the arena (port_arena.h). */
void *host_thread_stack(int idx, uint32_t *size) {
    *size = PORT_STACK_SIZE;
    return port_host(PORT_STACK_BASE + (uint32_t)idx * PORT_STACK_SIZE);
}

/* ---- movable memory (PORT_MOVABLE, docs/PORT.md "Movable memory") ------- */

#ifdef PORT_MOVABLE
#include <sys/mman.h>

uint8_t *port_arena;

/* what the arena link (bepass/Arena.cpp) wrote: the initial contents, in
   runs, and the words it left to startup (a function's address, the host's
   data) */
struct arena_run { uint32_t off, len; const uint8_t *data; };
struct arena_reloc { uint32_t off, width; const uint8_t *target; int64_t addend; };
extern const struct arena_run __port_arena_runs[];
extern const uint32_t __port_arena_runs_n;
extern const struct arena_reloc __port_arena_relocs[];
extern const uint32_t __port_arena_relocs_n;
extern const uint32_t __port_arena_data_end;

/* The arena: memory of the host's choosing, at an offset into its page
   (PORT_ARENA_OFFSET, default 0x5670) so that nothing can rely on its
   alignment beyond 16 bytes, filled as the arena link says. */
void port_arena_init(void) {
    const char *o = getenv("PORT_ARENA_OFFSET");
    uintptr_t off = o ? strtoul(o, NULL, 0) : 0x5670;
    size_t len = PORT_ARENA_SIZE + ((off + 0xFFFF) & ~(uintptr_t)0xFFFF);
    uint8_t *m = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (m == MAP_FAILED)
        host_fatal("can't map the arena");
    uint8_t *a = m + off;
    if (__port_arena_data_end > PORT_ARENA_STACKS)
        host_fatal("the arena's data runs into its stacks");
    for (uint32_t i = 0; i < __port_arena_runs_n; i++)
        memcpy(a + __port_arena_runs[i].off, __port_arena_runs[i].data, __port_arena_runs[i].len);
    for (uint32_t i = 0; i < __port_arena_relocs_n; i++) {
        const struct arena_reloc *r = &__port_arena_relocs[i];
        uint64_t v = (uint64_t)(uintptr_t)r->target + (uint64_t)r->addend;
        if (r->width == 4) {
            if (v >> 32)
                host_fatal("a 32-bit word of the arena can't hold %p", (void *)r->target);
#ifdef PORT_NATIVE_ENDIAN
            port_wg32(a + r->off, (uint32_t)v);
#else
            port_wbe32(a + r->off, (uint32_t)v);
#endif
        } else {
            host_fatal("a %u-byte word in the arena's relocations", r->width);
        }
    }
    port_arena = a;
    if (host_verbose)
        host_log("the arena at %p (%u relocations)\n", (void *)a, __port_arena_relocs_n);
}

/* port-arena: a local whose address escapes, on a stack that isn't in the
   arena (not a fiber's) */
void port_arena_bad_local(uintptr_t p) {
    host_fatal("a local of the game's C off the arena's stacks, at %p", (void *)p);
}
#endif

/* ---- translated-code hooks ------------------------------------------------ */

void recomp_trap(recomp_context *ctx, int kind, uint32_t pc, uint32_t code) {
    (void)ctx;
    host_fatal("translated code trapped: kind %d at %08X (code %X)", kind, pc, code);
}

void recomp_call_external(uint8_t *rdram, recomp_context *ctx, uint32_t addr) {
    (void)rdram; (void)ctx;
    host_fatal("unhandled call out of translated code to %08X", addr);
}

/* --replay's rounding (recomp.h) */
int recomp_round_half_up;

uint64_t recomp_mfc0(recomp_context *ctx, int reg) {
    (void)ctx;
    if (reg == 12)          /* Status: interrupts enabled, CU1 */
        return 0x2000FF01u;
    if (reg == 9)           /* Count */
        return (uint32_t)host_ticks();
    return 0;
}

void recomp_mtc0(recomp_context *ctx, int reg, uint64_t value) {
    (void)ctx; (void)reg; (void)value;
}

#ifdef PORT_64BIT
/* port-ilp32 -port-ilp32-check (PORT_ILP32_CHECK): the game's C stored a
   pointer that doesn't fit in its 32-bit field */
void __port_bad_ptr(void *p) {
    host_fatal("a pointer above 4 GB (%p) stored into game memory, from %p", p, __builtin_return_address(0));
}
#endif

/* PORT_TRACE_CALLS builds (BEPASS_TRACE=1): the game's C calls this on every function entry
   with a hash of the function's name.  PORT_TRACE=FILE,FROM,TO writes the
   hashes (and a 0 at every controller read) from the FROMth controller
   read to the TOth, for comparing two builds call by call. */
static FILE *trace_f;
static unsigned trace_poll, trace_from, trace_to = ~0u;

void __port_trace(uint32_t id) {
    static int init;
    if (!init) {
        init = 1;
        const char *s = getenv("PORT_TRACE");
        if (s) {
            char name[512];
            if (sscanf(s, "%511[^,],%u,%u", name, &trace_from, &trace_to) >= 1)
                trace_f = fopen(name, "wb");
        }
    }
    if (trace_f && trace_poll >= trace_from && trace_poll < trace_to)
        fwrite(&id, 4, 1, trace_f);
}

#ifdef PORT_TRACE_ASM
/* PORT_TRACE_ASM builds (the translated code built with RECOMP_TRACE): before
   every instruction, PORT_ITRACE=FILE,FROM,TO writes its address and the
   registers' low words (and lo), between the FROMth and the TOth controller
   read.  The registers hold the same values in the big-endian and the
   native-endian build but where a word copy moves data that isn't a word,
   so two traces part at the instruction after the one that went wrong
   (build_cmp.py itrace).  Addresses in the port's image and on the host
   stacks, which differ between builds, count as one value. */
void recomp_trace(recomp_context *ctx, uint32_t pc) {
    static FILE *f;
    static int init;
    static unsigned from, to;
    if (!init) {
        init = 1;
        const char *s = getenv("PORT_ITRACE");
        char name[512];
        to = ~0u;
        if (s && sscanf(s, "%511[^,],%u,%u", name, &from, &to) >= 1)
            f = fopen(name, "wb");
    }
    if (!f || trace_poll < from || trace_poll >= to)
        return;
    uint32_t rec[33];
    rec[0] = pc;
    for (int i = 1; i < 32; i++) {
        uint32_t v = (uint32_t)ctx->r[i];
        if ((v >= 0x80400000u && v < 0x81000000u) || (v >= 0x90000000u && v < 0x91000000u))
            v = 1;
        rec[i] = v;
    }
    rec[32] = (uint32_t)ctx->lo;
    fwrite(rec, sizeof rec, 1, f);
}
#endif

void port_trace_poll(void) {
    trace_poll++;
    if (trace_f && trace_poll >= trace_from && trace_poll < trace_to) {
        uint32_t z = 0;
        fwrite(&z, 4, 1, trace_f);
    }
}
