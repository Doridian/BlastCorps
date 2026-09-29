/*
 * libultra for the port: the OS (threads, messages, events, timers) and the
 * I/O the game uses (PI, SI, VI, SP/DP, AI), on top of port/host.
 *
 * N64 side: built with BEPass like the game, so the structures the game
 * shares with libultra (OSMesgQueue, OSContPad, OSTask, OSTimer...) have
 * their N64 layout and byte order and are used as plain C.  What needs the
 * host (fibers, the cartridge, the window) goes through port.h.
 *
 * Semantics follow libultra 2.0: a send that wakes a higher-priority thread
 * switches to it at once, events are messages sent (without blocking) to
 * the queue registered for them, and I/O completes immediately but reports
 * completion through the same messages the real hardware would cause.
 */
#include <ultra64.h>
#include <PR/rcp.h>
#include "port.h"

/* ---- globals libultra defines ------------------------------------------ */

/* osTvType, osRomBase, osResetType and osMemSize live at their fixed
   places in low RDRAM (0x80000300...), as the boot code leaves them. */
#define BOOT_WORD(off) (*(volatile u32 *)(0x80000300 + (off)))

/* ---- messages ------------------------------------------------------------ */

#define KEY_NOT_EMPTY(mq) ((u32)(mq))
#define KEY_NOT_FULL(mq) ((u32)(mq) + 1)

void osCreateMesgQueue(OSMesgQueue *mq, OSMesg *msg, s32 count) {
    mq->mtqueue = NULL;
    mq->fullqueue = NULL;
    mq->validCount = 0;
    mq->first = 0;
    mq->msgCount = count;
    mq->msg = msg;
}

static s32 send(OSMesgQueue *mq, OSMesg msg, s32 flag, int jam) {
    host_cpu_charge(COST_MESG);
    host_cpu_sync();
    while (mq->validCount >= mq->msgCount) {
        if (flag != OS_MESG_BLOCK)
            return -1;
        host_block(KEY_NOT_FULL(mq));
    }
    if (jam) {
        mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
        mq->msg[mq->first] = msg;
    } else {
        mq->msg[(mq->first + mq->validCount) % mq->msgCount] = msg;
    }
    mq->validCount++;
    host_wake(KEY_NOT_EMPTY(mq));
    host_preempt();
    return 0;
}

s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flag) { return send(mq, msg, flag, 0); }
s32 osJamMesg(OSMesgQueue *mq, OSMesg msg, s32 flag) { return send(mq, msg, flag, 1); }

s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag) {
    host_cpu_charge(COST_MESG);
    host_cpu_sync();
    while (mq->validCount == 0) {
        if (flag == OS_MESG_NOBLOCK)
            return -1;
        host_block(KEY_NOT_EMPTY(mq));
    }
    if (msg != NULL)
        *msg = mq->msg[mq->first];
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;
    host_wake(KEY_NOT_FULL(mq));
    host_preempt();
    return 0;
}

/* ---- events ---------------------------------------------------------------- */

static struct { OSMesgQueue *mq; OSMesg msg; } events[OS_NUM_EVENTS];

void osSetEventMesg(OSEvent e, OSMesgQueue *mq, OSMesg msg) {
    if (host_verbose)
        host_log("osSetEventMesg(%u, %08X, %X)\n", (unsigned)e, (unsigned)mq, (unsigned)msg);
    if (e < OS_NUM_EVENTS) {
        events[e].mq = mq;
        events[e].msg = msg;
    }
}

void port_irq_event(int e) {
    host_irq_cost(COST_IRQ);
    if (e >= 0 && e < OS_NUM_EVENTS && events[e].mq != NULL)
        osSendMesg(events[e].mq, events[e].msg, OS_MESG_NOBLOCK);
}

/* ---- threads ----------------------------------------------------------------- */

void osCreateThread(OSThread *t, OSId id, void (*entry)(void *), void *arg, void *sp, OSPri pri) {
    t->next = NULL;
    t->priority = pri;
    t->queue = NULL;
    t->tlnext = NULL;
    t->state = OS_STATE_STOPPED;
    t->flags = 0;
    t->id = id;
    t->fp = 0;
    host_thread_create((u32)t, entry, arg, (u32)sp, pri);
}

void osStartThread(OSThread *t) {
    t->state = OS_STATE_RUNNABLE;
    host_thread_start((u32)t);
}

void osStopThread(OSThread *t) {
    if (t != NULL)
        t->state = OS_STATE_STOPPED;
    host_thread_stop((u32)t);
}

void osDestroyThread(OSThread *t) {
    host_thread_destroy((u32)t);
}

void osSetThreadPri(OSThread *t, OSPri pri) {
    if (t != NULL)
        t->priority = pri;
    host_thread_set_pri((u32)t, pri);
}

OSPri osGetThreadPri(OSThread *t) {
    return host_thread_get_pri((u32)t);
}

OSId osGetThreadId(OSThread *t) {
    if (t == NULL)
        t = (OSThread *)host_thread_current();
    return t != NULL ? t->id : 0;
}

void osYieldThread(void) { host_yield(); }

/* interrupts: one thread runs at a time and nothing preempts it, so masking
   them has no effect */
OSIntMask osSetIntMask(OSIntMask m) { return OS_IM_ALL; }
OSIntMask osGetIntMask(void) { return OS_IM_ALL; }
u32 __osDisableInt(void) { return 1; }
void __osRestoreInt(u32 s) { }

/* ---- time and timers --------------------------------------------------------- */

static OSTime time_base;

OSTime osGetTime(void) { host_cpu_sync(); return host_ticks() - time_base; }
void osSetTime(OSTime t) { time_base = host_ticks() - t; }
u32 osGetCount(void) { host_cpu_sync(); return (u32)host_ticks(); }

#define MAX_TIMERS 16
static OSTimer *timers[MAX_TIMERS];
static OSTime timer_due[MAX_TIMERS];

int osSetTimer(OSTimer *t, OSTime countdown, OSTime interval, OSMesgQueue *mq, OSMesg msg) {
    int i, free = -1;
    t->interval = interval;
    t->value = countdown != 0 ? countdown : interval;
    t->mq = mq;
    t->msg = msg;
    for (i = 0; i < MAX_TIMERS; i++) {
        if (timers[i] == t) {
            free = i;
            break;
        }
        if (timers[i] == NULL && free < 0)
            free = i;
    }
    if (free < 0)
        return -1;
    timers[free] = t;
    timer_due[free] = host_ticks() + t->value;
    return 0;
}

int osStopTimer(OSTimer *t) {
    int i;
    for (i = 0; i < MAX_TIMERS; i++)
        if (timers[i] == t) {
            timers[i] = NULL;
            return 0;
        }
    return -1;
}

/* PI DMAs complete one after another, at mupen64plus's rate for the
   cartridge (a count tick per 8 bytes: cart_rom.c), which is much faster
   than the hardware's few MB/s; the data is copied at once, the message
   comes when the transfer would be done */
#define PI_MAX 256
static struct { OSTime due; OSMesgQueue *mq; OSMesg msg; } pi_q[PI_MAX];
static int pi_head, pi_n;
static OSTime pi_free_at;

static void pi_complete(OSMesgQueue *mq, OSMesg msg, u32 nbytes) {
    OSTime now = host_ticks();
    if (pi_free_at < now)
        pi_free_at = now;
    pi_free_at += nbytes / 8 + 1;
    if (mq == NULL)
        return;
    if (pi_n == PI_MAX) {
        osSendMesg(pi_q[pi_head].mq, pi_q[pi_head].msg, OS_MESG_NOBLOCK);
        pi_head = (pi_head + 1) % PI_MAX;
        pi_n--;
    }
    int i = (pi_head + pi_n++) % PI_MAX;
    pi_q[i].due = pi_free_at;
    pi_q[i].mq = mq;
    pi_q[i].msg = msg;
}

u64 port_irq_timers(u64 now) {
    u64 next = ~0ull;
    int i;
    while (pi_n > 0 && pi_q[pi_head].due <= now) {
        host_irq_cost(COST_IRQ);
        osSendMesg(pi_q[pi_head].mq, pi_q[pi_head].msg, OS_MESG_NOBLOCK);
        pi_head = (pi_head + 1) % PI_MAX;
        pi_n--;
    }
    if (pi_n > 0)
        next = pi_q[pi_head].due;
    for (i = 0; i < MAX_TIMERS; i++) {
        OSTimer *t = timers[i];
        if (t == NULL)
            continue;
        if (timer_due[i] <= now) {
            if (t->interval != 0) {
                timer_due[i] += t->interval;
                if (timer_due[i] <= now)
                    timer_due[i] = now + t->interval;
            } else {
                timers[i] = NULL;
            }
            host_irq_cost(COST_IRQ);
            if (t->mq != NULL)
                osSendMesg(t->mq, t->msg, OS_MESG_NOBLOCK);
        }
        if (timers[i] != NULL && timer_due[i] < next)
            next = timer_due[i];
    }
    return next;
}

/* ---- boot ---------------------------------------------------------------------- */

void osInitialize(void) {
    BOOT_WORD(0x00) = OS_TV_NTSC;       /* osTvType */
    BOOT_WORD(0x08) = 0xB0000000;       /* osRomBase */
    BOOT_WORD(0x0C) = 0;                /* osResetType: cold */
    BOOT_WORD(0x18) = 0x400000;         /* osMemSize */
}

u32 osGetMemSize(void) { return 0x400000; }

/* ---- caches, TLB, addresses ------------------------------------------------------ */

void osWritebackDCache(void *p, s32 n) { }
void osWritebackDCacheAll(void) { }
void osInvalDCache(void *p, s32 n) { }
void osInvalICache(void *p, s32 n) { }
void osMapTLBRdb(void) { }
void osUnmapTLBAll(void) { }
u32 osVirtualToPhysical(void *p) { return (u32)p & 0x1FFFFFFF; }
void *osPhysicalToVirtual(u32 a) { return (void *)(a | 0x80000000); }

/* ---- PI: the cartridge ------------------------------------------------------------- */

static OSMesgQueue *pi_cmdq;

void osCreatePiManager(OSPri pri, OSMesgQueue *cmdQ, OSMesg *cmdBuf, s32 cmdMsgCnt) {
    osCreateMesgQueue(cmdQ, cmdBuf, cmdMsgCnt);
    pi_cmdq = cmdQ;
}

OSMesgQueue *osPiGetCmdQueue(void) { return pi_cmdq; }

s32 osPiStartDma(OSIoMesg *mb, s32 pri, s32 dir, u32 devAddr, void *vAddr, u32 nbytes, OSMesgQueue *mq) {
    mb->hdr.type = OS_MESG_TYPE_DMAREAD;
    mb->hdr.pri = pri;
    mb->hdr.retQueue = mq;
    mb->dramAddr = vAddr;
    mb->devAddr = devAddr;
    mb->size = nbytes;
    host_cpu_charge(COST_PI_DMA);
    host_cpu_sync();
    if (dir == OS_READ)
        host_rom_read((u32)vAddr, devAddr, nbytes);
    else
        host_log("PI write to %08X ignored\n", (unsigned)devAddr);
    pi_complete(mq, (OSMesg)mb, nbytes);
    return 0;
}

s32 osPiRawStartDma(s32 dir, u32 devAddr, void *dramAddr, u32 size) {
    if (dir == OS_READ)
        host_rom_read((u32)dramAddr, devAddr, size);
    return 0;
}

s32 osPiRawReadIo(u32 devAddr, u32 *data) {
    *data = host_rom_word(devAddr);
    return 0;
}

s32 osPiReadIo(u32 devAddr, u32 *data) { return osPiRawReadIo(devAddr, data); }
u32 osPiGetStatus(void) { return 0; }

/* ---- SI: controllers and EEPROM ---------------------------------------------------- */

static int cont_queried;
static int max_controllers = MAXCONTROLLERS;    /* osContSetCh */

static void si_done(OSMesgQueue *mq) {
    /* the SI interrupt; the game registered mq for OS_EVENT_SI */
    port_irq_event(OS_EVENT_SI);
}

/* the waits libultra's SI code does, on a timer of its own */
typedef struct { OSMesgQueue q; OSMesg m; OSTimer timer; } Delay;

static void delay(Delay *d, OSTime ticks) {
    osCreateMesgQueue(&d->q, &d->m, 1);
    osSetTimer(&d->timer, ticks, 0, &d->q, &d->m);
    osRecvMesg(&d->q, NULL, OS_MESG_BLOCK);
}

s32 osContInit(OSMesgQueue *mq, u8 *bitpattern, OSContStatus *status) {
    int i;
    /* libultra's: the controllers need half a second after power-on */
    OSTime t = osGetTime();
    if (t < OS_USEC_TO_CYCLES(500000)) {
        static Delay d;
        delay(&d, OS_USEC_TO_CYCLES(500000) - t);
    }
    for (i = 0; i < MAXCONTROLLERS; i++) {
        status[i].type = i == 0 ? CONT_TYPE_NORMAL : 0;
        status[i].status = 0;
        status[i].errno = i == 0 ? 0 : CONT_NO_RESPONSE_ERROR;
    }
    *bitpattern = 1;
    max_controllers = MAXCONTROLLERS;
    return 0;
}

s32 osContStartQuery(OSMesgQueue *mq) {
    cont_queried = 1;
    si_done(mq);
    return 0;
}

void osContGetQuery(OSContStatus *status) {
    int i;
    for (i = 0; i < max_controllers; i++) {
        status[i].type = i == 0 ? CONT_TYPE_NORMAL : 0;
        status[i].status = 0;
        status[i].errno = i == 0 ? 0 : CONT_NO_RESPONSE_ERROR;
    }
}

s32 osContStartReadData(OSMesgQueue *mq) {
    if (host_replay_active())
        host_replay_read_started();     /* port_replay_si_done, when it's time */
    else
        si_done(mq);
    return 0;
}

void port_replay_si_done(void) { port_irq_event(OS_EVENT_SI); }

void osContGetReadData(OSContPad *pad) {
    int i;
    host_controller_poll();
    for (i = 0; i < max_controllers; i++) {
        u16 b = 0;
        s8 x = 0, y = 0;
        if (i == 0) {
            host_input(0, &b, &x, &y);
            if (b && host_verbose > 1)
                host_log("pad: %04X\n", b);
        }
        pad[i].button = b;
        pad[i].stick_x = x;
        pad[i].stick_y = y;
        pad[i].errno = i == 0 ? 0 : CONT_NO_RESPONSE_ERROR;
    }
}

s32 osContSetCh(u8 ch) {
    max_controllers = ch < MAXCONTROLLERS ? ch : MAXCONTROLLERS;
    return 0;
}

s32 osEepromProbe(OSMesgQueue *mq) { return EEPROM_TYPE_4K; }

s32 osEepromRead(OSMesgQueue *mq, u8 addr, u8 *buf) {
    host_eeprom_read(addr, (u32)buf);
    return 0;
}

s32 osEepromWrite(OSMesgQueue *mq, u8 addr, u8 *buf) {
    host_eeprom_write(addr, (u32)buf);
    return 0;
}

s32 osEepromLongRead(OSMesgQueue *mq, u8 addr, u8 *buf, int n) {
    for (; n > 0; n -= 8, addr++, buf += 8)
        host_eeprom_read(addr, (u32)buf);
    return 0;
}

/* libultra's: 12 ms after each block, the EEPROM's write cycle */
s32 osEepromLongWrite(OSMesgQueue *mq, u8 addr, u8 *buf, int n) {
    static Delay d;
    for (; n > 0; n -= 8, addr++, buf += 8) {
        host_eeprom_write(addr, (u32)buf);
        delay(&d, OS_USEC_TO_CYCLES(12000));
    }
    return 0;
}

/* hd_front_end's own copies (1D2D0.c, 1D410.c) */
s32 func_802042D0(OSMesgQueue *mq, u8 addr, u8 *buf, int n) { return osEepromLongWrite(mq, addr, buf, n); }
s32 func_80204410(OSMesgQueue *mq, u8 addr, u8 *buf, int n) { return osEepromLongRead(mq, addr, buf, n); }

/* No Controller Pak: every call reports that none is plugged in. */
s32 osPfsIsPlug(OSMesgQueue *mq, u8 *pattern) { *pattern = 0; return 0; }
s32 osPfsInitPak(OSMesgQueue *mq, OSPfs *pfs, int channel) { return PFS_ERR_NOPACK; }
s32 osPfsInit(OSMesgQueue *mq, OSPfs *pfs, int channel) { return PFS_ERR_NOPACK; }
s32 osPfsChecker(OSPfs *pfs) { return PFS_ERR_NOPACK; }
s32 osPfsFreeBlocks(OSPfs *pfs, s32 *left) { *left = 0; return PFS_ERR_NOPACK; }
s32 osPfsAllocateFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext, int len, s32 *file) { return PFS_ERR_NOPACK; }
s32 osPfsDeleteFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext) { return PFS_ERR_NOPACK; }
s32 osPfsFindFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext, s32 *file) { return PFS_ERR_NOPACK; }
s32 osPfsReadWriteFile(OSPfs *pfs, s32 file, u8 flag, int off, int n, u8 *buf) { return PFS_ERR_NOPACK; }
s32 osPfsFileState(OSPfs *pfs, s32 file, OSPfsState *st) { return PFS_ERR_NOPACK; }
s32 osPfsNumFiles(OSPfs *pfs, s32 *max, s32 *used) { *max = 0; *used = 0; return PFS_ERR_NOPACK; }

/* ---- VI -------------------------------------------------------------------------- */

s32 osViClock = VI_NTSC_CLOCK;

static OSMesgQueue *vi_mq;
static OSMesg vi_msg;
static u32 vi_count, vi_retrace;
static u32 vi_sent;             /* retrace messages sent: the scheduler's count, once it has run */
static void *vi_cur_fb, *vi_next_fb;
static OSViMode *vi_mode;
static int vi_black;

void osCreateViManager(OSPri pri) { }

void osViSetMode(OSViMode *m) { vi_mode = m; }

void osViSetEvent(OSMesgQueue *mq, OSMesg msg, u32 retraceCount) {
    vi_mq = mq;
    vi_msg = msg;
    vi_retrace = retraceCount;
    vi_count = 0;
    vi_sent = 0;                /* the new client counts from here */
    if (host_verbose)
        host_log("osViSetEvent(%08X, %X, %u)\n", (unsigned)mq, (unsigned)msg, (unsigned)retraceCount);
}

void osViSwapBuffer(void *fb) { host_cpu_sync(); vi_next_fb = fb; }
void *osViGetCurrentFramebuffer(void) { return vi_cur_fb; }
void *osViGetNextFramebuffer(void) { return vi_next_fb; }
void osViBlack(u8 active) { vi_black = active; }
void osViSetSpecialFeatures(u32 func) { }
void osViSetXScale(f32 x) { }
void osViSetYScale(f32 y) { }
u32 osViGetCurrentLine(void) { return 0; }
u32 osViGetCurrentField(void) { return 0; }

void port_irq_vi(void) {
    host_irq_cost(COST_IRQ);
    if (vi_next_fb != NULL)
        vi_cur_fb = vi_next_fb;
    host_vi_set_framebuffer(vi_black ? 0 : (u32)vi_cur_fb,
                            vi_mode != NULL ? (int)(vi_mode->comRegs.width) : 320);
    if (vi_mq != NULL && vi_retrace != 0 && ++vi_count >= vi_retrace) {
        vi_count = 0;
        if (osSendMesg(vi_mq, vi_msg, OS_MESG_NOBLOCK) == 0)
            vi_sent++;
    }
}

uint32_t port_vi_sent(void) { return vi_sent; }

/* ---- SP/DP ---------------------------------------------------------------------------- */

static OSTask *sp_task;

/* The RDP's freeze bit.  The scheduler freezes the RDP when it swaps
   buffers and thaws it at the retrace that shows the new one, so the next
   frame can't finish drawing (and swap) in the same field.  A full sync
   that comes while frozen is held until the thaw, as on the hardware (and
   in mupen64plus).  Without this, the modes whose swap doesn't wait for the
   next retrace (D_80364A90 in func_80271904) run as fast as the host. */
static int dp_frozen, dp_held;
static u64 dp_held_ns;

/* A graphics task started while the RDP is frozen waits for the thaw,
   RSP half and all: on the hardware the RSP stalls once the RDP's command
   buffer is full, so the task neither draws nor finishes early.  The
   buffer it draws into is usually the one still on screen (the swap to
   the other takes effect at the next retrace); drawing it at once showed
   the new frame, then the old one, then the new one again. */
static struct { u32 dl, size, ucode; } dp_pending[4];
static int dp_npending;

static uint32_t gfx_tasks;      /* graphics tasks run, held or not (--replay) */
uint32_t port_gfx_tasks(void) { return gfx_tasks; }

static void dp_run(u32 dl, u32 size, u32 ucode) {
    int sync;
    gfx_tasks++;
    sync = host_gfx_task(dl, size, ucode);
    u64 rdp = host_take_rdp_ns();
    host_raise(OS_EVENT_SP);
    if (sync)                           /* the RDP's full sync */
        host_raise_at(OS_EVENT_DP, host_now_ns() + rdp);
}

void osDpSetStatus(u32 v) {
    int k;

    if (v & DPC_SET_FREEZE)
        dp_frozen = 1;
    if (v & DPC_CLR_FREEZE) {
        dp_frozen = 0;
        for (k = 0; k < dp_npending; k++)
            dp_run(dp_pending[k].dl, dp_pending[k].size, dp_pending[k].ucode);
        dp_npending = 0;
        if (dp_held) {
            dp_held = 0;
            host_raise_at(OS_EVENT_DP, host_now_ns() + dp_held_ns);
        }
    }
}

u32 osDpGetStatus(void) { return dp_frozen ? DPC_STATUS_FREEZE : 0; }

void osSpTaskLoad(OSTask *t) { sp_task = t; }

void osSpTaskStartGo(OSTask *t) {
    host_cpu_sync();
    if (t->t.type == M_GFXTASK) {
        if (dp_frozen && dp_npending < (int)(sizeof dp_pending / sizeof dp_pending[0])) {
            dp_pending[dp_npending].dl = (u32)t->t.data_ptr;
            dp_pending[dp_npending].size = t->t.data_size;
            dp_pending[dp_npending].ucode = (u32)t->t.ucode;
            dp_npending++;
        } else {
            int sync;
            u64 rdp;
            gfx_tasks++;
            sync = host_gfx_task((u32)t->t.data_ptr, t->t.data_size, (u32)t->t.ucode);
            rdp = host_take_rdp_ns();
            host_raise(OS_EVENT_SP);
            if (sync) {                 /* the RDP's full sync */
                if (dp_frozen) {
                    dp_held = 1;
                    dp_held_ns = rdp;
                } else {
                    host_raise_at(OS_EVENT_DP, host_now_ns() + rdp);
                }
            }
        }
    } else {
        /* the audio microcode, run at once */
        if (t->t.type == M_AUDTASK)
            host_audio_task((u32)t->t.data_ptr, t->t.data_size, (u32)t->t.ucode_data);
        host_raise(OS_EVENT_SP);
    }
}

void osSpTaskYield(void) { }
OSYieldResult osSpTaskYielded(OSTask *t) { return 0; }
u32 __osSpGetStatus(void) { return SP_STATUS_HALT; }
void __osSpSetStatus(u32 v) { }
s32 osDpSetNextBuffer(void *p, u64 n) { return 0; }

/* ---- AI ----------------------------------------------------------------------------------- */

/* libultra's arithmetic: the DAC divides the VI clock, so 22050 Hz becomes
   22047 */
s32 osAiSetFrequency(u32 f) {
    u32 dacRate = (u32)((f32)osViClock / (f32)f + .5f);
    if (dacRate < 132)
        return -1;
    host_ai_set_dacrate(dacRate);
    return osViClock / (s32)dacRate;
}

s32 osAiSetNextBuffer(void *buf, u32 size) {
    host_cpu_sync();
    return host_ai_submit((u32)buf, size);
}

u32 osAiGetLength(void) { host_cpu_sync(); return host_ai_length(); }
u32 osAiGetStatus(void) { return host_ai_status(); }

/* ---- debug output --------------------------------------------------------------------------- */

void osSyncPrintf(const char *fmt, ...) { }
void rmonPrintf(const char *fmt, ...) { }
