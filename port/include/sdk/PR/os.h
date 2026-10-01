/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the operating
 * system's interface as the game uses it.  The structs are laid out as the
 * N64 library's are, since the game's C, its data and the handwritten code
 * share them (port/src/ultra.c implements the functions).
 */
#ifndef PORT_SDK_OS_H
#define PORT_SDK_OS_H

#include <PR/ultratypes.h>

/* ---- threads -------------------------------------------------------------- */

typedef s32 OSPri;
typedef s32 OSId;

/* a floating-point register pair, as a thread's saved context holds it */
typedef union {
    struct {
        f32 f_odd;
        f32 f_even;
    } f;
    f64 d;
} __OSfp;

/* the registers a thread's context saves: the 64-bit integer registers
   but zero, k0 and k1, then lo/hi, the status, pc, cause, bad address, RCP
   interrupt mask and FP control, and the even FP registers */
typedef struct {
    u64 at, v0, v1, a0, a1, a2, a3;
    u64 t0, t1, t2, t3, t4, t5, t6, t7;
    u64 s0, s1, s2, s3, s4, s5, s6, s7;
    u64 t8, t9, gp, sp, s8, ra;
    u64 lo, hi;
    u32 sr, pc, cause, badvaddr, rcp;
    u32 fpcsr;
    __OSfp fp0, fp2, fp4, fp6, fp8, fp10, fp12, fp14;
    __OSfp fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30;
} __OSThreadContext;

typedef struct OSThread_s {
    struct OSThread_s *PTR32 next;          /* in a run or message queue */
    OSPri priority;
    struct OSThread_s *PTR32 *PTR32 queue;  /* the queue it waits on */
    struct OSThread_s *PTR32 tlnext;        /* all threads */
    u16 state;                              /* OS_STATE_* */
    u16 flags;
    OSId id;
    int fp;                                 /* uses the FPU */
    __OSThreadContext context;
} OSThread;

#define OS_STATE_STOPPED 1
#define OS_STATE_RUNNABLE 2
#define OS_STATE_RUNNING 4
#define OS_STATE_WAITING 8

/* ---- messages, events, interrupts ------------------------------------------ */

typedef u32 OSEvent;
typedef u32 OSIntMask;
typedef u32 OSPageMask;
typedef void *PTR32 OSMesg;

typedef struct OSMesgQueue_s {
    OSThread *PTR32 mtqueue;                /* threads waiting to receive */
    OSThread *PTR32 fullqueue;              /* threads waiting to send */
    s32 validCount;                         /* messages in it */
    s32 first;                              /* the oldest one's index */
    s32 msgCount;                           /* its size */
    OSMesg *PTR32 msg;
} OSMesgQueue;

#define MQ_GET_COUNT(mq) ((mq)->validCount)
#define MQ_IS_EMPTY(mq) (MQ_GET_COUNT(mq) == 0)
#define MQ_IS_FULL(mq) (MQ_GET_COUNT(mq) >= (mq)->msgCount)

#define OS_MESG_NOBLOCK 0
#define OS_MESG_BLOCK 1

/* the events a queue can be told of (osSetEventMesg) */
#define OS_EVENT_SW1 0
#define OS_EVENT_SW2 1
#define OS_EVENT_CART 2
#define OS_EVENT_COUNTER 3
#define OS_EVENT_SP 4
#define OS_EVENT_SI 5
#define OS_EVENT_AI 6
#define OS_EVENT_VI 7
#define OS_EVENT_PI 8
#define OS_EVENT_DP 9
#define OS_EVENT_CPU_BREAK 10
#define OS_EVENT_SP_BREAK 11
#define OS_EVENT_FAULT 12
#define OS_EVENT_THREADSTATUS 13
#define OS_EVENT_PRENMI 14
#define OS_NUM_EVENTS 15

/* interrupt masks: the CPU's IM bits (8-15) and the RCP's (16-21), with
   the status register's IE (bit 0) */
#define OS_IM_NONE 0x00000001
#define OS_IM_SW1 0x00000501
#define OS_IM_SW2 0x00000601
#define OS_IM_CART 0x00000c01
#define OS_IM_PRENMI 0x00001401
#define OS_IM_RDBWRITE 0x00002401
#define OS_IM_RDBREAD 0x00004401
#define OS_IM_COUNTER 0x00008401
#define OS_IM_CPU 0x0000ff01
#define OS_IM_SP 0x00010401
#define OS_IM_SI 0x00020401
#define OS_IM_AI 0x00040401
#define OS_IM_VI 0x00080401
#define OS_IM_PI 0x00100401
#define OS_IM_DP 0x00200401
#define OS_IM_ALL 0x003fff01
#define RCP_IMASK 0x003f0000
#define RCP_IMASKSHIFT 16

#define OS_PRIORITY_MAX 255
#define OS_PRIORITY_VIMGR 254
#define OS_PRIORITY_PIMGR 150
#define OS_PRIORITY_APPMAX 127
#define OS_PRIORITY_IDLE 0

/* ---- the peripheral interface: DMA ------------------------------------------ */

#define OS_READ 0                           /* device to RDRAM */
#define OS_WRITE 1

#define OS_MESG_TYPE_BASE 10
#define OS_MESG_TYPE_LOOPBACK (OS_MESG_TYPE_BASE + 0)
#define OS_MESG_TYPE_DMAREAD (OS_MESG_TYPE_BASE + 1)
#define OS_MESG_TYPE_DMAWRITE (OS_MESG_TYPE_BASE + 2)
#define OS_MESG_TYPE_VRETRACE (OS_MESG_TYPE_BASE + 3)
#define OS_MESG_TYPE_COUNTER (OS_MESG_TYPE_BASE + 4)
#define OS_MESG_TYPE_EDMAREAD (OS_MESG_TYPE_BASE + 5)
#define OS_MESG_TYPE_EDMAWRITE (OS_MESG_TYPE_BASE + 6)

#define OS_MESG_PRI_NORMAL 0
#define OS_MESG_PRI_HIGH 1

/* the 64DD's transfer state, part of a PI handle's layout */
typedef struct {
    u32 errStatus;
    void *PTR32 dramAddr;
    void *PTR32 C2Addr;
    u32 sectorSize;
    u32 C1ErrNum;
    u32 C1ErrSector[4];
} __OSBlockInfo;

typedef struct {
    u32 cmdType;
    u16 transferMode;
    u16 blockNum;
    s32 sectorNum;
    u32 devAddr;
    u32 bmCtlShadow;
    u32 seqCtlShadow;
    __OSBlockInfo block[2];
} __OSTranxInfo;

typedef struct OSPiHandle_s {
    struct OSPiHandle_s *PTR32 next;
    u8 type;
    u8 latency;
    u8 pageSize;
    u8 relDuration;
    u8 pulse;
    u8 domain;
    u32 baseAddress;
    u32 speed;
    __OSTranxInfo transferInfo;
} OSPiHandle;

typedef struct {
    u16 type;                               /* OS_MESG_TYPE_* */
    u8 pri;                                 /* OS_MESG_PRI_* */
    u8 status;
    OSMesgQueue *PTR32 retQueue;            /* told when it's done */
} OSIoMesgHdr;

typedef struct {
    OSIoMesgHdr hdr;
    void *PTR32 dramAddr;
    u32 devAddr;
    u32 size;
    OSPiHandle *PTR32 piHandle;
} OSIoMesg;

/* a device manager (the PI's), whose state the game reads */
typedef struct {
    s32 active;
    OSThread *PTR32 thread;
    OSMesgQueue *PTR32 cmdQueue;
    OSMesgQueue *PTR32 evtQueue;
    OSMesgQueue *PTR32 acsQueue;
    s32 (*PTR32 dma)(s32, u32, void *, u32);
    s32 (*PTR32 edma)(OSPiHandle *, s32, u32, void *, u32);
} OSDevMgr;

/* ---- the video interface ------------------------------------------------------ */

typedef struct {
    u32 ctrl;
    u32 width;
    u32 burst;
    u32 vSync;
    u32 hSync;
    u32 leap;
    u32 hStart;
    u32 xScale;
    u32 vCurrent;
} OSViCommonRegs;

typedef struct {
    u32 origin;
    u32 yScale;
    u32 vStart;
    u32 vBurst;
    u32 vIntr;
} OSViFieldRegs;

typedef struct {
    u8 type;                                /* OS_VI_* */
    OSViCommonRegs comRegs;
    OSViFieldRegs fldRegs[2];
} OSViMode;

#define OS_TV_PAL 0
#define OS_TV_NTSC 1
#define OS_TV_MPAL 2

/* the VI modes: L/H low or high resolution, P/A point-sampled or
   anti-aliased, N/F non-interlaced or interlaced, 1/2 16 or 32 bits */
#define OS_VI_NTSC_LPN1 0
#define OS_VI_NTSC_LPF1 1
#define OS_VI_NTSC_LAN1 2
#define OS_VI_NTSC_LAF1 3
#define OS_VI_NTSC_LPN2 4
#define OS_VI_NTSC_LPF2 5
#define OS_VI_NTSC_LAN2 6
#define OS_VI_NTSC_LAF2 7
#define OS_VI_NTSC_HPN1 8
#define OS_VI_NTSC_HPF1 9
#define OS_VI_NTSC_HAN1 10
#define OS_VI_NTSC_HAF1 11
#define OS_VI_NTSC_HPN2 12
#define OS_VI_NTSC_HPF2 13
#define OS_VI_PAL_LPN1 14
#define OS_VI_PAL_LPF1 15
#define OS_VI_PAL_LAN1 16
#define OS_VI_PAL_LAF1 17
#define OS_VI_PAL_LPN2 18
#define OS_VI_PAL_LPF2 19
#define OS_VI_PAL_LAN2 20
#define OS_VI_PAL_LAF2 21
#define OS_VI_PAL_HPN1 22
#define OS_VI_PAL_HPF1 23
#define OS_VI_PAL_HAN1 24
#define OS_VI_PAL_HAF1 25
#define OS_VI_PAL_HPN2 26
#define OS_VI_PAL_HPF2 27

/* osViSetSpecialFeatures */
#define OS_VI_GAMMA_ON 0x0001
#define OS_VI_GAMMA_OFF 0x0002
#define OS_VI_GAMMA_DITHER_ON 0x0004
#define OS_VI_GAMMA_DITHER_OFF 0x0008
#define OS_VI_DIVOT_ON 0x0010
#define OS_VI_DIVOT_OFF 0x0020
#define OS_VI_DITHER_FILTER_ON 0x0040
#define OS_VI_DITHER_FILTER_OFF 0x0080

extern OSViMode osViModeTable[];

/* ---- time ------------------------------------------------------------------------ */

typedef u64 OSTime;

typedef struct OSTimer_s {
    struct OSTimer_s *PTR32 next;
    struct OSTimer_s *PTR32 prev;
    OSTime interval;                        /* 0: once */
    OSTime value;                           /* until it fires */
    OSMesgQueue *PTR32 mq;
    OSMesg msg;
} OSTimer;

/* the CPU's count register runs at 3/4 of the 62.5 MHz clock */
extern u64 osClockRate;
#define OS_CLOCK_RATE 62500000LL
#define OS_CPU_COUNTER (OS_CLOCK_RATE * 3 / 4)
#define OS_NSEC_TO_CYCLES(n) (((u64)(n) * (OS_CPU_COUNTER / 15625000LL)) / (1000000000LL / 15625000LL))
#define OS_USEC_TO_CYCLES(n) (((u64)(n) * (OS_CPU_COUNTER / 15625LL)) / (1000000LL / 15625LL))
#define OS_CYCLES_TO_NSEC(c) (((u64)(c) * (1000000000LL / 15625000LL)) / (OS_CPU_COUNTER / 15625000LL))
#define OS_CYCLES_TO_USEC(c) (((u64)(c) * (1000000LL / 15625LL)) / (OS_CPU_COUNTER / 15625LL))

/* ---- controllers, the Controller Pak and the EEPROM ------------------------------ */

#define MAXCONTROLLERS 4

typedef struct {
    u16 type;                               /* CONT_TYPE_*, CONT_EEPROM */
    u8 status;                              /* CONT_CARD_ON... */
    u8 errno;
} OSContStatus;

typedef struct {
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 errno;
} OSContPad;

typedef struct {
    void *PTR32 address;
    u8 databuffer[32];
    u8 addressCrc;
    u8 dataCrc;
    u8 errno;
} OSContRamIo;

#define CONT_NO_RESPONSE_ERROR 0x8
#define CONT_OVERRUN_ERROR 0x4

#define CONT_ABSOLUTE 0x0001
#define CONT_RELATIVE 0x0002
#define CONT_JOYPORT 0x0004
#define CONT_EEPROM 0x8000
#define CONT_EEP16K 0x4000
#define CONT_TYPE_MASK 0x1f07
#define CONT_TYPE_NORMAL 0x0005
#define CONT_TYPE_MOUSE 0x0002

#define CONT_CARD_ON 0x01
#define CONT_CARD_PULL 0x02
#define CONT_ADDR_CRC_ER 0x04
#define CONT_EEPROM_BUSY 0x80

#define EEPROM_TYPE_4K 0x01
#define EEPROM_TYPE_16K 0x02
#define EEPROM_MAXBLOCKS 64
#define EEP16K_MAXBLOCKS 256
#define EEPROM_BLOCK_SIZE 8

/* the buttons' bits */
#define CONT_A 0x8000
#define CONT_B 0x4000
#define CONT_G 0x2000
#define CONT_START 0x1000
#define CONT_UP 0x0800
#define CONT_DOWN 0x0400
#define CONT_LEFT 0x0200
#define CONT_RIGHT 0x0100
#define CONT_L 0x0020
#define CONT_R 0x0010
#define CONT_E 0x0008
#define CONT_D 0x0004
#define CONT_C 0x0002
#define CONT_F 0x0001
#define A_BUTTON CONT_A
#define B_BUTTON CONT_B
#define L_TRIG CONT_L
#define R_TRIG CONT_R
#define Z_TRIG CONT_G
#define START_BUTTON CONT_START
#define U_JPAD CONT_UP
#define L_JPAD CONT_LEFT
#define R_JPAD CONT_RIGHT
#define D_JPAD CONT_DOWN
#define U_CBUTTONS CONT_E
#define L_CBUTTONS CONT_C
#define R_CBUTTONS CONT_F
#define D_CBUTTONS CONT_D

/* the Controller Pak's file system */
typedef struct {
    int status;
    OSMesgQueue *PTR32 queue;
    int channel;
    u8 id[32];
    u8 label[32];
    int version;
    int dir_size;
    int inode_table;
    int minode_table;
    int dir_table;
    int inode_start_page;
    u8 banks;
    u8 activebank;
} OSPfs;

typedef struct {
    u32 file_size;
    u32 game_code;
    u16 company_code;
    char ext_name[4];
    char game_name[16];
} OSPfsState;

#define PFS_FILE_NAME_LEN 16
#define PFS_FILE_EXT_LEN 4
#define BLOCKSIZE 32
#define PFS_ONE_PAGE 8
#define PFS_MAX_BANKS 62

#define PFS_READ 0
#define PFS_WRITE 1
#define PFS_CREATE 2

#define PFS_INITIALIZED 0x1
#define PFS_CORRUPTED 0x2

#define PFS_ERR_NOPACK 1
#define PFS_ERR_NEW_PACK 2
#define PFS_ERR_INCONSISTENT 3
#define PFS_ERR_CONTRFAIL CONT_OVERRUN_ERROR
#define PFS_ERR_INVALID 5
#define PFS_ERR_BAD_DATA 6
#define PFS_DATA_FULL 7
#define PFS_DIR_FULL 8
#define PFS_ERR_EXIST 9
#define PFS_ERR_ID_FATAL 10
#define PFS_ERR_DEVICE 11

/* ---- addresses ---------------------------------------------------------------------- */

#define OS_K0_TO_PHYSICAL(x) (u32)(((char *)(x) - 0x80000000))
#define OS_K1_TO_PHYSICAL(x) (u32)(((char *)(x) - 0xa0000000))
#define OS_PHYSICAL_TO_K0(x) (void *)(((u32)(x) + 0x80000000))
#define OS_PHYSICAL_TO_K1(x) (void *)(((u32)(x) + 0xa0000000))

/* ---- the system's variables ------------------------------------------------------------ */

extern s32 osRomType;
extern void *osRomBase;
extern s32 osTvType;
extern s32 osResetType;
extern s32 osCicId;
extern s32 osVersion;
extern u32 osMemSize;
extern s32 osAppNMIBuffer[];

/* ---- functions ---------------------------------------------------------------------------- */

extern void osCreateThread(OSThread *, OSId, void (*)(void *), void *, void *, OSPri);
extern void osDestroyThread(OSThread *);
extern void osYieldThread(void);
extern void osStartThread(OSThread *);
extern void osStopThread(OSThread *);
extern OSId osGetThreadId(OSThread *);
extern void osSetThreadPri(OSThread *, OSPri);
extern OSPri osGetThreadPri(OSThread *);

extern void osCreateMesgQueue(OSMesgQueue *, OSMesg *, s32);
extern s32 osSendMesg(OSMesgQueue *, OSMesg, s32);
extern s32 osJamMesg(OSMesgQueue *, OSMesg, s32);
extern s32 osRecvMesg(OSMesgQueue *, OSMesg *, s32);
extern void osSetEventMesg(OSEvent, OSMesgQueue *, OSMesg);

extern OSIntMask osGetIntMask(void);
extern OSIntMask osSetIntMask(OSIntMask);

extern void osInvalDCache(void *, s32);
extern void osInvalICache(void *, s32);
extern void osWritebackDCache(void *, s32);
extern void osWritebackDCacheAll(void);

extern void osMapTLB(s32, OSPageMask, void *, u32, u32, s32);
extern void osMapTLBRdb(void);
extern void osUnmapTLB(s32);
extern void osUnmapTLBAll(void);
extern void osSetTLBASID(s32);

extern u32 osVirtualToPhysical(void *);
extern void *osPhysicalToVirtual(u32);

extern u32 osAiGetStatus(void);
extern u32 osAiGetLength(void);
extern s32 osAiSetFrequency(u32);
extern s32 osAiSetNextBuffer(void *, u32);

extern u32 osDpGetStatus(void);
extern void osDpSetStatus(u32);
extern void osDpGetCounters(u32 *);
extern s32 osDpSetNextBuffer(void *, u64);

extern u32 osPiGetStatus(void);
extern s32 osPiGetDeviceType(void);
extern s32 osPiRawWriteIo(u32, u32);
extern s32 osPiRawReadIo(u32, u32 *);
extern s32 osPiRawStartDma(s32, u32, void *, u32);
extern s32 osPiWriteIo(u32, u32);
extern s32 osPiReadIo(u32, u32 *);
extern s32 osPiStartDma(OSIoMesg *, s32, s32, u32, void *, u32, OSMesgQueue *);
extern void osCreatePiManager(OSPri, OSMesgQueue *, OSMesg *, s32);

extern u32 osViGetStatus(void);
extern u32 osViGetCurrentMode(void);
extern u32 osViGetCurrentLine(void);
extern u32 osViGetCurrentField(void);
extern void *osViGetCurrentFramebuffer(void);
extern void *osViGetNextFramebuffer(void);
extern void osViSetXScale(f32);
extern void osViSetYScale(f32);
extern void osViSetSpecialFeatures(u32);
extern void osViSetMode(OSViMode *);
extern void osViSetEvent(OSMesgQueue *, OSMesg, u32);
extern void osViSwapBuffer(void *);
extern void osViBlack(u8);
extern void osViFade(u8, u16);
extern void osViRepeatLine(u8);
extern void osCreateViManager(OSPri);

extern OSTime osGetTime(void);
extern void osSetTime(OSTime);
extern int osSetTimer(OSTimer *, OSTime, OSTime, OSMesgQueue *, OSMesg);
extern int osStopTimer(OSTimer *);

extern s32 osContInit(OSMesgQueue *, u8 *, OSContStatus *);
extern s32 osContReset(OSMesgQueue *, OSContStatus *);
extern s32 osContStartQuery(OSMesgQueue *);
extern s32 osContStartReadData(OSMesgQueue *);
extern s32 osContSetCh(u8);
extern void osContGetQuery(OSContStatus *);
extern void osContGetReadData(OSContPad *);

extern s32 osPfsInitPak(OSMesgQueue *, OSPfs *, int);
extern s32 osPfsRepairId(OSPfs *);
extern s32 osPfsInit(OSMesgQueue *, OSPfs *, int);
extern s32 osPfsReFormat(OSPfs *, OSMesgQueue *, int);
extern s32 osPfsChecker(OSPfs *);
extern s32 osPfsAllocateFile(OSPfs *, u16, u32, u8 *, u8 *, int, s32 *);
extern s32 osPfsFindFile(OSPfs *, u16, u32, u8 *, u8 *, s32 *);
extern s32 osPfsDeleteFile(OSPfs *, u16, u32, u8 *, u8 *);
extern s32 osPfsReadWriteFile(OSPfs *, s32, u8, int, int, u8 *);
extern s32 osPfsFileState(OSPfs *, s32, OSPfsState *);
extern s32 osPfsGetLabel(OSPfs *, u8 *, int *);
extern s32 osPfsSetLabel(OSPfs *, u8 *);
extern s32 osPfsIsPlug(OSMesgQueue *, u8 *);
extern s32 osPfsFreeBlocks(OSPfs *, s32 *);
extern s32 osPfsNumFiles(OSPfs *, s32 *, s32 *);

extern s32 osEepromProbe(OSMesgQueue *);
extern s32 osEepromRead(OSMesgQueue *, u8, u8 *);
extern s32 osEepromWrite(OSMesgQueue *, u8, u8 *);
extern s32 osEepromLongRead(OSMesgQueue *, u8, u8 *, int);
extern s32 osEepromLongWrite(OSMesgQueue *, u8, u8 *, int);

extern void bcopy(const void *, void *, int);
extern int bcmp(const void *, const void *, int);
extern void bzero(void *, int);

extern void osInitialize(void);
extern u32 osGetCount(void);
extern void osExit(void);
extern u32 osGetMemSize(void);

extern int sprintf(char *s, const char *fmt, ...);
extern void osSyncPrintf(const char *fmt, ...);
extern void osAsyncPrintf(const char *fmt, ...);
extern int osSyncGetChars(char *buf);
extern int osAsyncGetChars(char *buf);

#endif
