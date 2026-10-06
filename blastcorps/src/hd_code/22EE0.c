#include "common.h"
#include "game/audio.h"
#include "game/sched.h"
#include "functions.h"

void func_80267A9C(void *arg);
void func_80267CDC(AudioInfo *info, AudioInfo *lastInfo);
void func_80267F88(AudioInfo *info);
void func_802682A4(void);
ALDMAproc func_80268254(AMDMAState **state);
s32 func_80267FE0(s32 addr, s32 len, void *state);

extern s32 D_80000300;
extern s32 osViClock;
extern u64 D_802E6820[];
extern u64 D_802E68F0[];
extern u64 D_8030EB90[];

/*
 * The audio frame (Nintendo's demo audio.c's names, MAX_RSP_CMDS from the
 * assert): NUM_FIELDS retraces long, at FRAMES_PER_SECOND.  eu's PAL frame is
 * one field, and its command list and DMA buffers are smaller.
 */
#ifdef VERSION_EU
#define NUM_FIELDS 1
#define EXTRA_SAMPLES 38
#define MAX_RSP_CMDS 1950
#define DMA_BUFFER_LENGTH 0x138
#else
#define NUM_FIELDS 2
#define EXTRA_SAMPLES 53
#define MAX_RSP_CMDS 2750
#define DMA_BUFFER_LENGTH 0x200
#endif

/* .bss, 0x80368050-0x8036B8B0 (tools/bss_c.py) */
u8 D_80368050[8];
OSTime D_80368058;
OSTime D_80368060;
OSTime D_80368068;
AMAudioMgr D_80368070;
SchedClient D_803682F8;
u64 D_80368308[0x400];
AMDMAState D_8036A308;
AMDMABuffer D_8036A318[NUM_DMA_MESSAGES];
u32 D_8036A8B8;
u32 D_8036A8BC;
u32 D_8036A8C0;
s32 D_8036A8C4;
IoMesg D_8036A8C8[NUM_DMA_MESSAGES];
OSMesgQueue D_8036AE68;
OSMesg D_8036AE80[NUM_DMA_MESSAGES];
s32 D_8036AFA0;
u8 D_8036AFA4[4];
u8 D_8036AFA8[8];
u8 D_8036AFB0[0x900];

/* .data, 0x802F3AF0-0x802F3C10 (tools/data_c.py) */
u32 D_802F3AF0 = 0;
u32 D_802F3AF4 = 0;
s32 D_802F3AF8 = 0;
FxParams D_802F3AFC = {
    {
        8, 6800, 0, 160, 9830, -9830, 0, 0, 0, 0, 160, 320, 9830, -9830, 11140, 0, 0, 9472, 800,
        2560, 16384, -16384, 4587, 0, 0, 12288, 960, 1920, 8192, -8192, 0, 0, 0, 0, 3200, 5600,
        16384, -16384, 4587, 0, 0, 13568, 3360, 4800, 8192, -8192, 0, 0, 0, 0, 4800, 5440, 8192,
        -8192, 0, 0, 0, 0, 0, 5920, 13000, -13000, 0, 380, 10, 17664,
    },
};
s32 D_802F3C04 = 1;


void func_802676A0(SynConfig *c, OSPri pri) {
    s32 i;
    f32 fsize;
    s32 unused;
    FxParams params;

    c->dmaproc = func_80268254;
#ifdef VERSION_EU
    osViClock = 0x02F5B2D0; /* PAL's */
#else
    if (D_80000300 != 1) {
        osViClock = 0x02E6025C;
    }
#endif
    c->outputRate = osAiSetFrequency(22050);
    fsize = (f32)c->outputRate * NUM_FIELDS / FRAMES_PER_SECOND;
    D_8036A8BC = (s32)fsize;
    if (D_8036A8BC < fsize) {
        D_8036A8BC++;
    }
    if (D_8036A8BC & 0xF) {
        D_8036A8BC = (D_8036A8BC & ~0xF) + 0x10;
    }
    D_8036A8B8 = D_8036A8BC - 16;
    D_8036A8C0 = D_8036A8BC + EXTRA_SAMPLES;
    if (c->fxType == AL_FX_CUSTOM) {
        unused = 0;
        params = D_802F3AFC;
        c->params = params.v;
        alInit(&D_80368070.g, (ALSynConfig *)c);
    } else {
        alInit(&D_80368070.g, (ALSynConfig *)c);
    }
    D_8036A318[0].node.prev = NULL;
    D_8036A318[0].node.next = NULL;
    for (i = 0; i < NUM_DMA_MESSAGES - 1; i++) {
        alLink(&D_8036A318[i + 1].node, &D_8036A318[i].node);
        D_8036A318[i].ptr = alHeapAlloc(c->heap, 1, DMA_BUFFER_LENGTH);
    }
    D_8036A318[i].ptr = alHeapAlloc(c->heap, 1, DMA_BUFFER_LENGTH);
    for (i = 0; i < 2; i++) {
        D_80368070.ACMDList[i] = alHeapAlloc(c->heap, 1, MAX_RSP_CMDS * sizeof(Acmd));
    }
    for (i = 0; i < 3; i++) {
        D_80368070.audioInfo[i] = alHeapAlloc(c->heap, 1, sizeof(AudioInfo));
        D_80368070.audioInfo[i]->data = alHeapAlloc(c->heap, 1, D_8036A8C0 * 4);
    }
    osCreateMesgQueue(&D_80368070.audioReplyMsgQ, D_80368070.audioReplyMsgBuf, 8);
    osCreateMesgQueue(&D_80368070.audioFrameMsgQ, D_80368070.audioFrameMsgBuf, 8);
    osCreateMesgQueue(&D_8036AE68, D_8036AE80, NUM_DMA_MESSAGES);
    osCreateThread(&D_80368070.thread, 4, func_80267A9C, NULL, &D_80368308[0x2000 / sizeof(u64)], pri);
}

void func_80267A74(void) {
    osStartThread(&D_80368070.thread);
}

extern u8 D_80367728;
extern u8 D_80367729;
extern u8 D_8036772A;
extern s32 D_8036772C;
extern u8 D_80367730;


void func_80267A9C(void *arg) {
    s32 done;
    s32 msg;
    AudioInfo *lastInfo;
    s32 first;

    done = 0;
    lastInfo = NULL;
    first = 1;
    osScAddClient(&D_80315440, &D_803682F8, &D_80368070.audioFrameMsgQ, NUM_FIELDS, 2);
    osSendMesg(&D_80368070.audioFrameMsgQ, (OSMesg)5, OS_MESG_NOBLOCK);
    while (!done) {
        osRecvMesg(&D_80368070.audioFrameMsgQ, (OSMesg *)&msg, OS_MESG_BLOCK);
        switch (msg) {
            case 5:
                if (D_80315440.audioListHead != NULL) {
                    osSendMesg(&D_80315440.interruptQ, (OSMesg)0x29E, OS_MESG_BLOCK);
                }
                D_80368060 = osGetTime();
                func_80267CDC(D_80368070.audioInfo[D_802F3AF0 % 3], lastInfo);
                D_80368068 = osGetTime();
                D_80368058 = D_80368060;
                if (!first) {
                    osRecvMesg(&D_80368070.audioReplyMsgQ, (OSMesg *)&lastInfo, OS_MESG_BLOCK);
                    func_80267F88(lastInfo);
                }
                first = 0;
                if (D_8036772A != 0) {
                    func_802613C8();
                }
                if (D_80367728 != 0) {
                    func_80261068();
                }
                if (D_80367729 != 0) {
                    func_80261284();
                }
                if (D_80367730 == 0) {
                    func_802611F0();
                }
                if (D_8036772C != 0) {
                    func_80261528();
                }
                break;
            case 4:
                break;
            case 10:
                done = 1;
                break;
            case 6:
                func_8029A7E4("No samples left\n");
                break;
        }
    }
    alClose(&D_80368070.g);
}

void func_80267CDC(AudioInfo *info, AudioInfo *lastInfo) {
    s16 *audioPtr;
    Acmd *cmdp;
    u32 samplesLeft;
    SchedTask *t;

    samplesLeft = 0;
    func_802682A4();
    audioPtr = (s16 *)osVirtualToPhysical(info->data);
    if (lastInfo != NULL) {
        osAiSetNextBuffer(lastInfo->data, lastInfo->frameSamples << 2);
    }
    samplesLeft = osAiGetLength() >> 2;
    info->frameSamples = (D_8036A8BC - samplesLeft + EXTRA_SAMPLES) & ~0xF;
    if (info->frameSamples < D_8036A8B8) {
        info->frameSamples = D_8036A8B8;
    }
    cmdp = alAudioFrame(D_80368070.ACMDList[D_802F3AF8], &D_8036A8C4, audioPtr, info->frameSamples);
    if (D_8036A8C4 > MAX_RSP_CMDS) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "cmdLen <= MAX_RSP_CMDS", "audio.c", LINE_EU(0x150, 0x151));
    }
    t = &info->task;
    t->next = NULL;
    t->msgQ = &D_80368070.audioReplyMsgQ;
    t->msg = (OSMesg)info;
    t->flags = 1;
    t->client = &D_803682F8;
    t->list.t.data_ptr = (u64 *)D_80368070.ACMDList[D_802F3AF8];
    t->list.t.data_size = (cmdp - D_80368070.ACMDList[D_802F3AF8]) * sizeof(Acmd);
    t->list.t.type = M_AUDTASK;
    t->list.t.ucode_boot = D_802E6820;
    t->list.t.ucode_boot_size = (s32)D_802E68F0 - (s32)D_802E6820;
    t->list.t.flags = 0;
    t->list.t.ucode = D_802E68F0;
    t->list.t.ucode_data = D_8030EB90;
    t->list.t.ucode_data_size = 0x800;
    t->list.t.yield_data_ptr = NULL;
    t->list.t.yield_data_size = 0;
    osWritebackDCache(t, sizeof(SchedTask));
    osWritebackDCache(t->list.t.data_ptr, t->list.t.data_size);
    if (osSendMesg(osScGetCmdQ(&D_80315440), t, OS_MESG_NOBLOCK) == -1) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "osSendMesg(osScGetCmdQ(&sc), (OSMesg) t, OS_MESG_NOBLOCK)!=-1", "audio.c", LINE_EU(0x169, 0x16A));
    }
    D_802F3AF8 ^= 1;
}

void func_80267F88(AudioInfo *info) {
    u32 samplesLeft;

    samplesLeft = osAiGetLength() >> 2;
    if (samplesLeft == 0 && D_802F3C04 == 0) {
        func_8029A7E4("audio: ai out of samples\n");
        D_802F3C04 = 0;
    }
}

s32 func_80267FE0(s32 addr, s32 len, void *state) {
    void *foundBuffer;
    s32 delta;
    s32 addrEnd;
    s32 buffEnd;
    AMDMABuffer *dmaPtr;
    AMDMABuffer *lastDmaPtr;
    AMDMABuffer *cur;
    s32 count;

    count = 0;
    lastDmaPtr = NULL;
    dmaPtr = D_8036A308.firstUsed;
    addrEnd = addr + len;
    delta = addr & 1;
    while (dmaPtr != NULL) {
        cur = dmaPtr;
        buffEnd = dmaPtr->startAddr + DMA_BUFFER_LENGTH;
        if (dmaPtr->startAddr > addr) {
            break;
        } else if (addrEnd <= buffEnd) {
            dmaPtr->lastFrame = D_802F3AF0;
            foundBuffer = dmaPtr->ptr + addr - dmaPtr->startAddr;
            return osVirtualToPhysical(foundBuffer);
        }
        lastDmaPtr = dmaPtr;
        dmaPtr = (AMDMABuffer *)dmaPtr->node.next;
        count++;
    }
    if (count > D_8036AFA0) {
        D_8036AFA0 = count;
    }
    dmaPtr = D_8036A308.firstFree;
    if (dmaPtr == NULL) {
        func_8029A7E4("OH DEAR - No audio DMA buffers left\n");
    }
    if (dmaPtr == NULL) {
        return osVirtualToPhysical(D_8036A308.firstUsed);
    }
    D_8036A308.firstFree = (AMDMABuffer *)dmaPtr->node.next;
    alUnlink(&dmaPtr->node);
    if (lastDmaPtr != NULL) {
        alLink(&dmaPtr->node, &lastDmaPtr->node);
    } else if (D_8036A308.firstUsed != NULL) {
        lastDmaPtr = D_8036A308.firstUsed;
        D_8036A308.firstUsed = dmaPtr;
        dmaPtr->node.next = &lastDmaPtr->node;
        dmaPtr->node.prev = NULL;
        lastDmaPtr->node.prev = &dmaPtr->node;
    } else {
        D_8036A308.firstUsed = dmaPtr;
        dmaPtr->node.next = NULL;
        dmaPtr->node.prev = NULL;
    }
    foundBuffer = dmaPtr->ptr;
    addr -= delta;
    dmaPtr->startAddr = addr;
    dmaPtr->lastFrame = D_802F3AF0;
    osPiStartDma((OSIoMesg *)&D_8036A8C8[D_802F3AF4++], OS_MESG_PRI_NORMAL, OS_READ, addr, foundBuffer, DMA_BUFFER_LENGTH, &D_8036AE68);
    return osVirtualToPhysical(foundBuffer) + delta;
}

ALDMAproc func_80268254(AMDMAState **state) {
    s32 unused;

    if (!D_8036A308.initialized) {
        D_8036A308.firstUsed = NULL;
        D_8036A308.firstFree = D_8036A318;
        D_8036A308.initialized = 1;
    }
    *state = &D_8036A308;
    return (ALDMAproc)func_80267FE0;
}

void func_802682A4(void) {
    u32 i;
    OSIoMesg *iomsg;
    AMDMABuffer *dmaPtr;
    AMDMABuffer *nextPtr;

    for (i = 0; i < D_802F3AF4; i++) {
        if (osRecvMesg(&D_8036AE68, (OSMesg *)&iomsg, OS_MESG_NOBLOCK) == -1) {
            func_8029A7E4("Dma not done\n");
        }
    }
    dmaPtr = D_8036A308.firstUsed;
    while (dmaPtr != NULL) {
        nextPtr = (AMDMABuffer *)dmaPtr->node.next;
        if (dmaPtr->lastFrame + 1 < D_802F3AF0) {
            if (D_8036A308.firstUsed == dmaPtr) {
                D_8036A308.firstUsed = (AMDMABuffer *)dmaPtr->node.next;
            }
            alUnlink(&dmaPtr->node);
            if (D_8036A308.firstFree != NULL) {
                alLink(&dmaPtr->node, &D_8036A308.firstFree->node);
            } else {
                D_8036A308.firstFree = dmaPtr;
                dmaPtr->node.next = NULL;
                dmaPtr->node.prev = NULL;
            }
        }
        dmaPtr = nextPtr;
    }
    D_802F3AF4 = 0;
    D_802F3AF0++;
}
