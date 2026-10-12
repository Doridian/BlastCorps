/*
 * The interface between the two halves of the port.
 *
 * "N64 side": the decompiled game C, port/src/ (the libultra
 * replacement), port/engine/ (Rare's handwritten engine as C) and
 * port/libaudio/.  Built as the IDO dialect needs (CMakeLists.txt), in the
 * host's byte order and pointer size (docs/PORT.md, "Memory model").
 *
 * "Host side": port/host/ (SDL, files, fibers, the renderer).  Built
 * normally.
 *
 * The host reads the game's variables and structures as the game's C
 * declares them (host/host_game_vars.h, host_game_types.h); what stays the
 * N64's bytes (the ROM file, texels, framebuffers, the audio microcode's
 * DMEM) is read big-endian: port_rd32 for the ROM's, port_be32 for the
 * RDP's and the AI's.
 */
#ifndef PORT_H
#define PORT_H

#include <stddef.h>
#include <stdint.h>

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

/* Game memory is ordinary variables and buffers (src/regions.c), the
   fibers' stacks come from the host's allocator, and no N64 address is an
   address (a host read through one fails to build). */
/* (PORT_STACK_SIZE, PORT_MAX_THREADS: port_budget.h) */
#include "port_budget.h"
/* thread idx's host stack (runtime.c) */
void *host_thread_stack(int idx, uint32_t *size);
/* whether p is inside one of the game's threads' stacks (runtime.c) */
int host_in_thread_stack(const void *p);

/* big-endian accessors, for what stays the N64's bytes: the RDP's (the
   framebuffers, TMEM, the texels, the fill and blend colours) and the AI's
   (the samples, the audio microcode's DMEM) */
static inline uint32_t port_be32(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}
static inline uint16_t port_be16(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
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
/* a ROM file's bytes (always big-endian, as the cartridge has them) */
static inline uint32_t port_rd32(const void *p) { return port_be32(p); }
static inline uint16_t port_rd16(const void *p) { return port_be16(p); }
/* and little-endian, for the host's files (WAV, BMP) */
static inline void port_wle32(void *p, uint32_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v; b[1] = v >> 8; b[2] = v >> 16; b[3] = v >> 24;
}
static inline void port_wle16(void *p, uint16_t v) {
    uint8_t *b = (uint8_t *)p;
    b[0] = v; b[1] = v >> 8;
}

/* ---- the game's layouts, as the host reads them ------------------------ */

/* What the host reads inside the game's bigger structures (port/host/
   digest.c, host_game_vars.h, gfx.c) that host_game_types.h doesn't spell
   out: port/src/layout.c's sizeof and offsetof, by the N64 side's own
   compiler, so right whatever the host's pointer size (a struct with
   pointers has its fields move with it).  The host doesn't include the
   game's headers: it asks for a number by its name here (host_layout). */
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
    PORT_LAYOUT_STATS_IP,               /* LevelStats (game/level.h) */
    PORT_LAYOUT_STATS_TC,
    PORT_LAYOUT_STATS_BD,
    PORT_LAYOUT_STATS_CR,
    PORT_LAYOUT_STATS_COIN,
    PORT_LAYOUT_STATS_RT,
    PORT_LAYOUT_SCHED_UNK280,           /* Sched (game/sched.h): the level's retraces */
    PORT_LAYOUT_SCHED_FRAMECOUNT,       /* the retraces */
    PORT_LAYOUT_FRAME_SIZE,             /* Frame (game/frame.h): segment 2's buffer */
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
/* a word of a display list that is a pointer into one of the names that
   are segment addresses (D_02000000...: src/segments.c's proxies) as that
   segmented address, tagged; any other word as it is */
uintptr_t port_seg_unproxy(uintptr_t w);

/* ---- host services (port/host) ----------------------------------------- */

void host_fatal(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
void host_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern int host_verbose;
/* the level pool's top, where the N64 has it and its end (src/pool.c) */
void host_pool_report(const char *what, int level, const void *top, const void *n64_top, const void *end);

/* the cartridge */
uint32_t host_rom_size(void);
void host_rom_read(void *dst, uint32_t rom, uint32_t len);
uint32_t host_rom_word(uint32_t rom);
/* the loaders (host/native.c): untyped bytes from the ROM arrived; the
   first two return what they were (romclass.h's ROM_*) */
int host_loaded_gzip(const void *src, void *dst, uint32_t len);
int host_loaded_lzss(uint32_t rom, void *dst, uint32_t len);
void host_loaded_dma(void *dst, uint32_t rom, uint32_t len);
void host_layout_to_be(void *p, uint32_t n, const char *layout, int to_bytes);
void host_layout_to_be_n(void *p, uint32_t stride, uint32_t count, uint32_t n, const char *layout);
/* the save's bytes between game memory and the N64's order (native.c) */
void host_save_order(uint8_t *p, uint32_t off, uint32_t n, int unused);

/* --load-waits n64 (main.c): keep the N64's hardware waits (the
   controllers' power-on half second, the EEPROM's write cycles, the pak
   thread's retrace a command); 0, the default, leaves them out */
int host_load_waits(void);

/* 46.875 MHz CPU count since boot */
uint64_t host_ticks(void);

/* threads: the game's OSThreads, run as fibers, one at a time, each known
   by its key: the OSThread's address as the N64 side has it (port/src/
   ultra.c), which host_thread_current gives back. */
void host_thread_create(uintptr_t key, void (*entry)(void *), void *arg, int pri);
void host_thread_start(uintptr_t key);
void host_thread_stop(uintptr_t key);
void host_thread_destroy(uintptr_t key);
void host_thread_set_pri(uintptr_t key, int pri);
int host_thread_get_pri(uintptr_t key);
void *host_thread_current(void);
/* block the current thread until host_wake(key); then return.  A wait's
   key is a message queue's address (ultra.c), only compared */
void host_block(const void *key);
void host_wake(const void *key);
/* let a higher-priority runnable thread run now (libultra preemption) */
void host_preempt(void);
void host_yield(void);
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
int host_gfx_task(const void *dl, uint32_t size, const void *ucode, int rdp);
/* the framebuffer the VI shows (NULL: black) */
void host_vi_set_framebuffer(const void *fb, int width);

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
/* the audio thread's state as the game asks for it (port/src/replay_hooks.c):
   kind 0 the sequence player's state, 1 the sequence's lastTicks; `caller`
   the calling function's name */
int32_t host_replay_audio_named(int kind, int32_t real, const char *caller);
/* whether the pak/EEPROM thread may go on with the command it took (`sync`:
   the game waits for its reply) */
int host_replay_save_due(int sync);
/* whether a thread waits in host_block on this key (threads.c) */
int host_blocked_on(const void *wait_key);
/* whether a thread other than the running one is in osRecvMesg on this key */
int host_receiving(const void *key);
/* osRecvMesg marks the thread as receiving on key until it returns */
void host_recv_mark(const void *key);

/* EEPROM (4 Kbit) */
void host_eeprom_read(int block, void *dst);
void host_eeprom_write(int block, const void *src);

/* audio: the RSP's audio task (port/host/aspmain.c) and the AI
   (port/host/audio.c) */
void host_audio_task(const void *data_ptr, uint32_t data_size, const void *ucode_data);
void host_ai_set_dacrate(uint32_t dacrate);
int host_ai_submit(const void *buf, uint32_t len);
uint32_t host_ai_length(void);
uint32_t host_ai_status(void);

#ifdef __cplusplus
}
#endif

#endif
