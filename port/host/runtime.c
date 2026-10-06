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

/* asm2x86.py: symbolic .words in the game's data, in host order until now.
   (Not in the movable build, whose arena image has them swapped already:
   it has no ELF section symbols to go by, which Mach-O doesn't know.) */
#ifndef PORT_MOVABLE
extern uint32_t __start_port_bswap32[] __attribute__((weak));
extern uint32_t __stop_port_bswap32[] __attribute__((weak));

void port_fixups(void) {
    for (uint32_t *p = __start_port_bswap32; p < __stop_port_bswap32; p++)
        swap_bytes((uint8_t *)(uintptr_t)*p, 4);
}
#endif

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
#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

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

#ifdef PORT_SCATTER
/* The scattered layout (PORT_SCATTER, docs/PORT.md "The scattered
   layout"): where nothing should be any more (an N64 place a variable
   left, the padding between them, the room of a name that is inside
   another variable), from the arena link.  It is filled with
   PORT_SCATTER_POISON (a byte, default 0xA5: a pointer read from it is
   off the arena).  With PORT_SCATTER_CHECK every access of the game's C
   that reaches it is reported, once a site and a place, and goes where
   the N64's layout would have had it (unless PORT_SCATTER_REDIRECT=0),
   so that the game plays on and the next one shows. */
struct scatter_bad {
    uint32_t start, len, kind;  /* 1 an N64 place left, 2 padding, 3 an alias's room */
    uint32_t n64a, n64b, now;   /* (Arena.cpp's Bad) */
    const char *desc;
};
extern const struct scatter_bad __port_scatter_bad[];
extern const uint32_t __port_scatter_bad_n;

#ifdef PORT_SCATTER_CHECK
extern const char *const __port_scatter_sites[];
static uint16_t *scatter_map;       /* a byte's range, + 1 */
static uint32_t scatter_map_end;
static uint64_t scatter_seen[1 << 16];
static int scatter_redirect = 1;

/* "scatter: SITE: N bytes at ADDR reach WHAT": the room of an alias or an
   N64 place left (+ the offset into it), or the padding, as +n after the
   variable before it or -n before the one after it, whichever is nearer */
static void scatter_report(uint32_t site, uint32_t size, uint32_t off, uint32_t r) {
    const struct scatter_bad *b = &__port_scatter_bad[r - 1];
    uint32_t o = off - b->start;
    uint64_t key = (uint64_t)site << 32 | r;
    uint32_t h = (uint32_t)((key * 0x9E3779B97F4A7C15ull) >> 48);
    for (uint32_t n = 0; n < (1u << 16); n++, h = (h + 1) & 0xFFFF) {
        if (scatter_seen[h] == key + 1)
            return;
        if (!scatter_seen[h]) {
            scatter_seen[h] = key + 1;
            break;
        }
    }
    fprintf(stderr, "scatter: %s: %u bytes at %08X reach ", __port_scatter_sites[site], size, 0x80000000u | off);
    const char *bar = b->kind == 2 ? strchr(b->desc, '|') : NULL;
    if (!bar)
        fprintf(stderr, "%s, +0x%X\n", b->desc, o);
    else if (o < b->len - o)
        fprintf(stderr, "padding, +0x%X after %.*s\n", o, (int)(bar - b->desc), b->desc);
    else
        fprintf(stderr, "padding, -0x%X before %s\n", b->len - o, bar + 1);
}

/* where the N64's layout has what a byte of range r stands for: an N64
   place's new address, an alias's place in what it is inside of, the
   padding's as the nearer neighbour has it; 0: nowhere */
static uint32_t scatter_where(uint32_t off, uint32_t r) {
    const struct scatter_bad *b = &__port_scatter_bad[r - 1];
    uint32_t o = off - b->start, y;
    if (b->kind == 1)
        return b->now + o;
    if (b->kind == 3)
        y = b->n64a + o;
    else if (o < b->len - o)
        y = b->n64a ? b->n64a + o : 0;
    else
        y = b->n64b ? b->n64b - (b->len - o) : 0;
    if (!y)
        return 0;
    if (y < scatter_map_end && scatter_map[y] && __port_scatter_bad[scatter_map[y] - 1].kind == 1)
        return scatter_where(y, scatter_map[y]);
    return y;
}

uint32_t port_scatter_check(uint32_t addr, uint32_t size, uint32_t site) {
    uint32_t off = addr & 0x1FFFFFFFu, first = 0;
    if (off >= PORT_ARENA_SIZE)
        host_fatal("scatter: %s: %u bytes at %08X, off the arena", __port_scatter_sites[site], size, addr);
    if (off >= scatter_map_end)
        return addr;
    uint32_t end = size > scatter_map_end - off ? scatter_map_end : off + (size ? size : 1);
    for (uint32_t o = off; o < end; o++) {
        uint32_t r = scatter_map[o];
        if (!r)
            continue;
        if (!first)
            first = r;
        scatter_report(site, size, o, r);
        o = __port_scatter_bad[r - 1].start + __port_scatter_bad[r - 1].len - 1;   /* (the rest of it) */
    }
    if (!first || !scatter_redirect || scatter_map[off] != first)
        return addr;
    uint32_t w = scatter_where(off, first);
    return w ? (addr & ~0x1FFFFFFFu) | w : addr;
}

/* a byte's address, redirected as port_scatter_check does it */
static uint8_t *scatter_byte(uint32_t off, uint32_t len, uint32_t site) {
    if (off < scatter_map_end && scatter_map[off]) {
        scatter_report(site, len, off, scatter_map[off]);
        uint32_t w = scatter_redirect ? scatter_where(off, scatter_map[off]) : 0;
        if (w)
            return port_arena + (w & 0x1FFFFFFFu);
    }
    return port_arena + off;
}

static int scatter_clean(uint32_t off, uint32_t len) {
    if (off >= scatter_map_end)
        return 1;
    uint32_t end = len > scatter_map_end - off ? scatter_map_end : off + len;
    for (uint32_t o = off; o < end; o++)
        if (scatter_map[o])
            return 0;
    return 1;
}

/* the game's C's memsets (kind 0, v the byte) and copies (kind 1, v the source) */
void port_scatter_mem(uint32_t dst, uint32_t v, uint32_t len, uint32_t kind, uint32_t site) {
    uint32_t d = dst & 0x1FFFFFFFu, s = v & 0x1FFFFFFFu;
    if (!len)
        return;
    if (scatter_clean(d, len) && (kind == 0 || scatter_clean(s, len))) {
        if (kind == 0)
            memset(port_arena + d, (int)v, len);
        else
            memmove(port_arena + d, port_arena + s, len);
        return;
    }
    if (kind == 0) {
        for (uint32_t k = 0; k < len; k++)
            *scatter_byte(d + k, len, site) = (uint8_t)v;
    } else if (d <= s) {
        for (uint32_t k = 0; k < len; k++)
            *scatter_byte(d + k, len, site) = *scatter_byte(s + k, len, site);
    } else {
        for (uint32_t k = len; k-- > 0;)
            *scatter_byte(d + k, len, site) = *scatter_byte(s + k, len, site);
    }
}
#endif

static void port_scatter_init(void) {
    const char *e = getenv("PORT_SCATTER_POISON");
    int poison = e ? (int)strtol(e, NULL, 0) : 0xA5;
    uint32_t end = 0;
    for (uint32_t i = 0; i < __port_scatter_bad_n; i++) {
        const struct scatter_bad *b = &__port_scatter_bad[i];
        memset(port_arena + b->start, poison, b->len);
        if (b->start + b->len > end)
            end = b->start + b->len;
    }
#ifdef PORT_SCATTER_CHECK
    e = getenv("PORT_SCATTER_REDIRECT");
    scatter_redirect = !e || atoi(e);
    scatter_map_end = end;
    scatter_map = calloc(end, sizeof *scatter_map);
    if (!scatter_map || __port_scatter_bad_n >= 0xFFFF)
        host_fatal("the scattered layout's map");
    for (uint32_t i = 0; i < __port_scatter_bad_n; i++)
        for (uint32_t o = 0; o < __port_scatter_bad[i].len; o++)
            scatter_map[__port_scatter_bad[i].start + o] = (uint16_t)(i + 1);
#endif
}
#endif

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
#ifdef PORT_ROM_DATA
    /* (the arena link's runs are empty: the contents come from the ROM) */
    port_romdata_apply(a, host_rom(), host_rom_size());
#endif
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
#ifdef PORT_SCATTER
    port_scatter_init();
#endif
    if (host_verbose)
        host_log("the arena at %p (%u relocations)\n", (void *)a, __port_arena_relocs_n);
}

/* the functions the N64 side uses as values, by their N64 addresses
   (bepass/Arena.cpp), sorted */
struct port_fn_entry { uint32_t addr; void *fn; };
extern const struct port_fn_entry __port_fns[];
extern const uint32_t __port_fns_n;

void *port_fn(uint32_t addr) {
    uint32_t lo = 0, hi = __port_fns_n;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        if (__port_fns[mid].addr < addr)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == __port_fns_n || __port_fns[lo].addr != addr)
        host_fatal("a call through %08X, which is no function's", addr);
    return __port_fns[lo].fn;
}

/* -port-arena-typed-calls (WebAssembly): the function at addr as the type
   the call has, its thunk if that isn't its own (bepass/Arena.cpp);
   sorted by address, then type */
struct port_fn_typed_entry { uint32_t addr, type; void *fn; };
extern const struct port_fn_typed_entry __port_fns_typed[];
extern const uint32_t __port_fns_typed_n;

void *port_fn_typed(uint32_t addr, uint32_t type) {
    uint32_t lo = 0, hi = __port_fns_typed_n;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        const struct port_fn_typed_entry *e = &__port_fns_typed[mid];
        if (e->addr < addr || (e->addr == addr && e->type < type))
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == __port_fns_typed_n || __port_fns_typed[lo].addr != addr || __port_fns_typed[lo].type != type)
        host_fatal("a call through %08X (type %u), which is no function's", addr, type);
    return __port_fns_typed[lo].fn;
}

/* port-arena: a frame of escaping locals outside a thread's slot of the
   locals' stacks (outside a thread, or past its slot's end) */
uint32_t port_locals_sp, port_locals_end;
void port_arena_bad_local(uintptr_t p) {
    host_fatal("a frame of the game's C's escaping locals off the locals' stacks, at %08lX", (unsigned long)p);
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

#ifdef PORT_SDK_TRACE
/* -DPORT_SDK_TRACE=ON builds: port/src/gu.c's calls and their arguments, to
   $PORT_SDK_TRACE (port/tools/sdk_check checks the replacements on them).
   A record is the function's number and its argument words, host order. */
static FILE *sdk_trace_f;

void port_sdk_trace(int fn, int nwords) {
    static int init;
    uint32_t w = (uint32_t)fn;
    (void)nwords;
    if (!init) {
        init = 1;
        const char *s = getenv("PORT_SDK_TRACE");
        if (s)
            sdk_trace_f = fopen(s, "wb");
    }
    if (sdk_trace_f)
        fwrite(&w, 4, 1, sdk_trace_f);
}

void port_sdk_trace_word(uint32_t w) {
    if (sdk_trace_f)
        fwrite(&w, 4, 1, sdk_trace_f);
}
#endif

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
