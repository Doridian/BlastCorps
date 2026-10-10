/*
 * The interface between the two halves of the port.
 *
 * "N64 side": the decompiled game C and port/src/ (the libultra
 * replacement).  Built by clang with BEPass, so its memory is big-endian,
 * byte for byte what the N64 would hold (docs/PORT.md, "Memory model").
 *
 * "Host side": port/host/ (SDL, files, fibers, the renderer) and the
 * translated handwritten code (which does its own byte swapping).  Built
 * normally.
 *
 * Only scalars and addresses cross between the two.  A host function that
 * reads or writes game memory through an address must do it in big-endian
 * order (port_be32 etc. below) or byte by byte.
 */
#ifndef PORT_H
#define PORT_H

#include <stdint.h>
#include "port_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the TV standard --------------------------------------------------- */

/* eu is PAL: 50 retraces a second, and the VI clock libultra's audio and
   video rates are counted in.  The others are NTSC. */
#ifdef VERSION_EU
#define PORT_RETRACE_HZ 50
#define PORT_VI_CLOCK 49656530u
#else
#define PORT_RETRACE_HZ 60
#define PORT_VI_CLOCK 48681812u
#endif

/* ---- memory ------------------------------------------------------------ */

/* RDRAM: the N64's 4 MB, mapped at its KSEG0 address.  The port's own
   image is linked right above it (0x80400000), so every address the game
   sees, whether a fixed RDRAM buffer or one of its variables, is a KSEG0
   address (K0_TO_PHYS is the identity: PR/R4300.h). */
#define PORT_RDRAM_BASE 0x80000000u
#define PORT_RDRAM_SIZE 0x00400000u
/* hardware registers (KSEG1 0xA4000000-0xA4900000): plain memory, so the
   odd debug read of one gets a zero */
#define PORT_HWREG_BASE 0xA4000000u
#define PORT_HWREG_SIZE 0x00900000u
/* fiber stacks for the game's threads, inside the KSEG0 window so that the
   address of a local means the same thing to the translated code */
#ifdef PORT_MOVABLE
#define PORT_STACK_BASE (0x80000000u | PORT_ARENA_STACKS)   /* in the arena */
#else
#define PORT_STACK_BASE 0x90000000u
#endif
#define PORT_STACK_SIZE 0x00100000u
#define PORT_MAX_THREADS 16
/* thread idx's host stack (runtime.c): at PORT_STACK_BASE, which in the
   movable build is in the arena */
void *host_thread_stack(int idx, uint32_t *size);

static inline void *port_ptr(uint32_t addr) {
    if (addr - 0xA0000000u < 0x20000000u)
        addr -= 0x20000000u;        /* KSEG1 -> KSEG0 */
    return port_host(addr);
}
/* a function value of the N64 side's, as the host calls it: in the movable
   build it is the function's N64 address (bepass/Arena.cpp), found by
   port_fn (port/host/runtime.c) */
#ifdef PORT_MOVABLE
void *port_fn(uint32_t addr);
#define PORT_FN(type, f) ((type)port_fn((uint32_t)(uintptr_t)(f)))
#else
#define PORT_FN(type, f) ((type)(f))
#endif

/* a game variable the host names (extern char D_X[]): where it is */
#ifdef PORT_MOVABLE
/* (the game's variables aren't symbols of the host's link: by their N64
   addresses, gen_syms.py table's header) */
#define PORT_VAR(sym) ((char *)port_ptr(PORT_N64_##sym))
#define PORT_ADDR(sym) PORT_N64_##sym
#else
#define PORT_VAR(sym) ((char *)port_ptr((uint32_t)(uintptr_t)(sym)))
#define PORT_ADDR(sym) ((uint32_t)(uintptr_t)(sym))
#endif

#ifdef PORT_ACCESS_PROFILE
/* host/access.c: the access-width profiler */
void port_access_host(const void *p, unsigned width);
void port_access_dma(uint32_t dst, uint32_t rom, uint32_t len);
void __port_access_copy(void *dst, const void *src, uint32_t n, uint32_t site);
void __port_access_set(void *dst, uint32_t n, uint32_t site);
void port_access_bytes(const void *p, uint32_t n);
void port_access_dump_widths(unsigned n);
#define PORT_ACCESS_HOST(p, w) port_access_host((p), (w))
#define PORT_ACCESS_BYTES(p, n) port_access_bytes((p), (n))
#else
#define PORT_ACCESS_HOST(p, w) ((void)0)
#define PORT_ACCESS_BYTES(p, n) ((void)0)
#endif

/* big-endian accessors for host-side code touching game memory */
/* (the profiler: in native memory these read bytes) */
#ifdef PORT_NATIVE_ENDIAN
#define PORT_ACCESS_BE(p, w) PORT_ACCESS_BYTES((p), (w))
#else
#define PORT_ACCESS_BE(p, w) PORT_ACCESS_HOST((p), (w))
#endif
static inline uint32_t port_be32(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    PORT_ACCESS_BE(p, 4);
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}
static inline uint16_t port_be16(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    PORT_ACCESS_BE(p, 2);
    return (uint16_t)((b[0] << 8) | b[1]);
}
static inline void port_wbe32(void *p, uint32_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;
}
static inline void port_wbe16(void *p, uint16_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v >> 8; b[1] = v;
}

/* A scalar of game memory at its own width: big-endian in the default
   build, host order in the native-endian one (PORT_NATIVE_ENDIAN, docs/
   PORT.md "Native-endian memory").  A 64-bit scalar is two of these words,
   the high one first, in both.  port_be* stay big-endian: they are for
   what is bytes in either build (the ROM file, texels, framebuffers, the
   audio microcode's DMEM and what it saves). */
#ifdef PORT_NATIVE_ENDIAN
static inline uint32_t port_g32(const void *p) {
    uint32_t v;
    PORT_ACCESS_HOST(p, 4);
    __builtin_memcpy(&v, p, 4);
    return v;
}
static inline uint16_t port_g16(const void *p) {
    uint16_t v;
    PORT_ACCESS_HOST(p, 2);
    __builtin_memcpy(&v, p, 2);
    return v;
}
static inline void port_wg32(void *p, uint32_t v) { __builtin_memcpy(p, &v, 4); }
static inline void port_wg16(void *p, uint16_t v) { __builtin_memcpy(p, &v, 2); }
/* element k of a halfword pair array held as words (an Mtx's s16 halves:
   element 2n is word n's high half) */
static inline uint16_t port_g16_of32(const void *p, int k) {
    return (uint16_t)(port_g32((const uint8_t *)p + 4 * (k >> 1)) >> ((k & 1) ? 0 : 16));
}
#else
#define port_g32 port_be32
#define port_g16 port_be16
#define port_wg32 port_wbe32
#define port_wg16 port_wbe16
static inline uint16_t port_g16_of32(const void *p, int k) { return port_be16((const uint8_t *)p + 2 * k); }
#endif

/* For host code that reads game variables with port_be32 and writes the
   pad with port_wbe16 (main.c, video.c): in the native-endian build
   those files map them here, which reads RDRAM at its own width and
   anything else (the ROM file) big-endian as before. */
#ifdef PORT_NATIVE_ENDIAN
/* game memory: RDRAM and the image's data after it, where the LP64 build
   puts the variables that outgrew their N64 addresses (gen_ld.py); the ROM
   file comes from malloc, after _end */
extern char _end[];
static inline int port_in_rdram(const void *p) {
#ifdef PORT_MOVABLE
    /* (none before port_arena_init: the ROM file is read first, and in
       wasm's linear memory it is below 28 MB) */
    return port_arena && (uintptr_t)p - (uintptr_t)port_arena < PORT_ARENA_SIZE;
#endif
    return (uintptr_t)p - PORT_RDRAM_BASE < (uintptr_t)_end - PORT_RDRAM_BASE;
}
static inline uint32_t port_var32(const void *p) { return port_in_rdram(p) ? port_g32(p) : port_be32(p); }
#ifdef PORT_LP64
/* a pointer the game's C declares without PTR32, 8 bytes here: the N64
   address it holds (one the game made from an address, as most of its
   buffers' are, or the host's own pointer into its memory) */
static inline uint32_t port_game_ptr(const void *p) {
    uint64_t v;
    __builtin_memcpy(&v, p, sizeof v);
    return v >> 32 ? port_n64((const void *)(uintptr_t)v) : (uint32_t)v;
}
#endif
static inline void port_wvar16(void *p, uint16_t v) {
    if (port_in_rdram(p) || port_n64(p) - PORT_STACK_BASE < PORT_STACK_SIZE * PORT_MAX_THREADS)
        port_wg16(p, v);
    else
        port_wbe16(p, v);
}
#else
#define port_var32 port_be32    /* (all of it big-endian) */
#endif

/* ---- the game's layouts, as the host reads them ------------------------ */

/* What the host reads inside the game's structures (port/host/digest.c,
   sched_vars.h, gfx.c), as this build's N64 side lays them out:
   port/src/layout.c's sizeof and offsetof, by the N64 side's own compiler,
   so right in every variant (in the LP64 ones a struct without PTR32 has
   native pointers, and its fields move).  The host doesn't include the
   game's headers: it asks for a number by its name here (host_layout).  A
   PTR_ entry is the size of one of the game's pointer variables (4, or 8
   in the LP64 build where it isn't PTR32), for port_game_ptr_n. */
enum PortLayout {
    PORT_LAYOUT_VEHICLE_SIZE,           /* Vehicle (game/vehicle.h) */
    PORT_LAYOUT_VEHICLE_TYPE,
    PORT_LAYOUT_VSTATE_SIZE,            /* VehicleState */
    PORT_LAYOUT_VSTATE_HEADING,         /* unk4C */
    PORT_LAYOUT_VSTATE_HEADING2,        /* unk4E */
    PORT_LAYOUT_VSTATE_SPEED,           /* unk76 */
    PORT_LAYOUT_BUILDING_SIZE,          /* Building (game/objects.h) */
    PORT_LAYOUT_BUILDING_X,
    PORT_LAYOUT_BUILDING_Y,
    PORT_LAYOUT_BUILDING_Z,
    PORT_LAYOUT_BUILDING_GROUPS,        /* unkE9 */
    PORT_LAYOUT_BUILDING_GONE,          /* unkEA */
    PORT_LAYOUT_BUILDING_DAMAGE,        /* damage[] */
    PORT_LAYOUT_TNT_SIZE,               /* TntCrate */
    PORT_LAYOUT_TNT_X,
    PORT_LAYOUT_TNT_Y,
    PORT_LAYOUT_TNT_Z,
    PORT_LAYOUT_TNT_TIMER,
    PORT_LAYOUT_TNT_ACTIVE,
    PORT_LAYOUT_BLOCK_SIZE,             /* Block */
    PORT_LAYOUT_BLOCK_X,
    PORT_LAYOUT_BLOCK_Y,
    PORT_LAYOUT_BLOCK_Z,
    PORT_LAYOUT_BLOCK_IN_HOLE,          /* unk11 */
    PORT_LAYOUT_AMMOBOX_SIZE,           /* AmmoBox */
    PORT_LAYOUT_AMMOBOX_COLLECTED,
    PORT_LAYOUT_RDU_SIZE,               /* Rdu (game/level.h) */
    PORT_LAYOUT_RDU_COLLECTED,
    PORT_LAYOUT_PLAYER_SIZE,            /* PlayerInfo (game/player.h) */
    PORT_LAYOUT_PLAYER_UNITS,
    PORT_LAYOUT_PLAYER_MEDAL,
    PORT_LAYOUT_PLAYER_GAMESTATE,
    PORT_LAYOUT_STATS_IP,               /* LevelStats (game/level.h) */
    PORT_LAYOUT_STATS_TC,
    PORT_LAYOUT_STATS_BD,
    PORT_LAYOUT_STATS_CR,
    PORT_LAYOUT_STATS_COIN,
    PORT_LAYOUT_STATS_RT,
    PORT_LAYOUT_SCHED_UNK280,           /* Sched (game/sched.h): the level's retraces */
    PORT_LAYOUT_SCHED_FRAMECOUNT,       /* the retraces */
    PORT_LAYOUT_PTR_D_803649D0,         /* Vehicle *, past the level's last */
    PORT_LAYOUT_PTR_D_803F7654,         /* Building *, past the last */
    PORT_LAYOUT_PTR_D_8036BED8,         /* Rdu * */
    PORT_LAYOUT_PTR_D_80365348,         /* Vtx *[2]'s element (hd_code 14B30.c) */
    PORT_LAYOUT_COUNT
};
/* the N64 side's number (port/src/layout.c); 0 for an id it doesn't know */
uint32_t port_layout(unsigned id);
/* the host's copy, made at the first use (in each file that asks: one call
   into the N64 side per entry, once) */
static inline uint32_t host_layout(enum PortLayout id) {
    static uint32_t v[PORT_LAYOUT_COUNT];
    static int got;
    unsigned i;
    if (!got) {
        for (i = 0; i < PORT_LAYOUT_COUNT; i++)
            v[i] = port_layout(i);
        got = 1;
    }
    return v[id];
}
/* one of the game's pointers, `size` bytes wide (a PTR_ entry): the N64
   address it holds */
static inline uint32_t port_game_ptr_n(const void *p, uint32_t size) {
#ifdef PORT_LP64
    if (size == 8)
        return port_game_ptr(p);
#endif
    (void)size;
    return port_var32(p);
}

/* ---- host services (port/host) ----------------------------------------- */

void host_fatal(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
void host_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern int host_verbose;

/* the cartridge */
uint32_t host_rom_size(void);
void host_rom_read(uint32_t dst, uint32_t rom, uint32_t len);
uint32_t host_rom_word(uint32_t rom);
/* the loaders (host/native.c): untyped bytes from the ROM arrived */
void host_loaded_gzip(uint32_t src, uint32_t dst, uint32_t len);
void host_loaded_lzss(uint32_t rom, uint32_t dst, uint32_t len);
void host_loaded_dma(uint32_t dst, uint32_t rom, uint32_t len);
void host_layout_to_be(uint32_t addr, uint32_t n, const char *layout, int to_bytes);
void host_layout_to_be_n(uint32_t addr, uint32_t stride, uint32_t count, uint32_t n, const char *layout);
/* the save's bytes between game memory and the N64's order (native.c) */
void host_save_order(uint8_t *p, uint32_t off, uint32_t n, int unused);

/* --load-waits n64 (main.c): keep the N64's hardware waits (the
   controllers' power-on half second, the EEPROM's write cycles, the pak
   thread's retrace a command); 0, the default, leaves them out */
int host_load_waits(void);

/* 46.875 MHz CPU count since boot */
uint64_t host_ticks(void);

/* threads: the game's OSThreads, run as fibers, one at a time */
void host_thread_create(uint32_t key, void (*entry)(void *), void *arg, uint32_t mips_sp, int pri);
void host_thread_start(uint32_t key);
void host_thread_stop(uint32_t key);
void host_thread_destroy(uint32_t key);
void host_thread_set_pri(uint32_t key, int pri);
int host_thread_get_pri(uint32_t key);
uint32_t host_thread_current(void);
/* block the current thread until host_wake(key); then return */
void host_block(uint32_t key);
void host_wake(uint32_t key);
/* let a higher-priority runnable thread run now (libultra preemption) */
void host_preempt(void);
void host_yield(void);
/* the check build's C under a check (PORT_ENGINE_CHECK): no time passes and no
   thread switches (threads.c) */
int host_time_stopped(void);
/* an interrupt taken: a thread in port_spin_wait tests again (threads.c) */
void host_interrupt(void);

/* "interrupts" the host raises from its loop, into port/src */
void port_irq_vi(void);             /* one retrace */
unsigned long long port_irq_timers(unsigned long long now);   /* fire due timers; next deadline or ~0 */
void port_irq_event(int event);     /* OS_EVENT_* */
void host_raise(int event);         /* queue an event for the loop */
void host_raise_at(int event, uint64_t at_ns);  /* ... at host_now_ns() == at_ns */
uint64_t host_now_ns(void);
uint64_t host_take_rdp_ns(void);    /* the RDP time of the last graphics task */

/* RSP/RDP */
/* runs a graphics task; 1 if its display list ended in a full sync.  rdp
   0: the microcode writes the RDP's commands to memory, nothing is drawn */
int host_gfx_task(uint32_t dl, uint32_t size, uint32_t ucode, int rdp);
void host_vi_set_framebuffer(uint32_t fb, int width);

/* input: N64 button bits and stick (written big-endian: the pointers are
   the N64 side's) */
void host_input(int pad, uint16_t *buttons, int8_t *x, int8_t *y);
int host_quit_requested(void);
void host_controller_poll(void);
/* --replay (port/host/replay.c): a read's SI completion waits for the
   log's retraces */
int host_replay_active(void);
void host_replay_read_started(void);
void port_replay_si_done(void);     /* port/src: the held SI completion */
uint32_t port_vi_sent(void);        /* port/src: retrace messages sent */
/* the audio thread's state as the game asks for it (port/src/replay_audio.c):
   kind 0 the sequence player's state, 1 the sequence's lastTicks; `caller`
   the return address into the game */
int32_t host_replay_audio(int kind, int32_t real, uint64_t caller);
/* whether the pak/EEPROM thread may go on with the command it took (`sync`:
   the game waits for its reply) */
int host_replay_save_due(int sync);
/* whether a thread waits in host_block on this key (threads.c) */
int host_blocked_on(uint32_t wait_key);
/* whether a thread other than the running one is in osRecvMesg on this key */
int host_receiving(uint32_t key);
/* osRecvMesg marks the thread as receiving on key until it returns */
void host_recv_mark(uint32_t key);

/* EEPROM (4 Kbit) */
void host_eeprom_read(int block, uint32_t dst);
void host_eeprom_write(int block, uint32_t src);

/* audio: the RSP's audio task (port/host/aspmain.c) and the AI
   (port/host/audio.c) */
void host_audio_task(uint32_t data_ptr, uint32_t data_size, uint32_t ucode_data);
void host_ai_set_dacrate(uint32_t dacrate);
int host_ai_submit(uint32_t addr, uint32_t len);
uint32_t host_ai_length(void);
uint32_t host_ai_status(void);

#ifdef __cplusplus
}
#endif

#endif
