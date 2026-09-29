#include "common.h"
#include "game/game.h"
#include "game/sched.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"

extern OSViMode D_80306E70[];
/*
 * This file's .bss.  The functions using these only match with them
 * defined here (a u64's halves share one lui).
 */

/* .bss, 0x8036BEF0-0x8036BFC0 (tools/bss_c.py) */
OSTime D_8036BEF0;
OSTime D_8036BEF8;
OSTime D_8036BF00;
s32 D_8036BF08;
s32 D_8036BF0C;
s32 D_8036BF10;
s32 D_8036BF14;
s32 D_8036BF18;
struct SchedTask *D_8036BF1C;
u32 D_8036BF20;
u32 D_8036BF24;
u8 D_8036BF28[4];
u32 D_8036BF2C;
u8 D_8036BF30[8];
OSTime D_8036BF38;
OSTime D_8036BF40;
OSTime D_8036BF48;
OSTime D_8036BF50;
u8 D_8036BF58[0x20];
OSTimer D_8036BF78;
u8 D_8036BF98[0x20];
u32 D_8036BFB8;
s32 D_8036BFBC;

/* .data, 0x802FA270-0x802FA280 (tools/data_c.py) */
u8 D_802FA270 = 1;


void func_8029A7E4(char *, ...);
void osCreateViManager(s32);
void __scMain(void *);
void __scAppendList(Sched *, SchedTask *);
void func_80271CE4(Sched *, s32);
void __scYield(Sched *);
s32 __scTaskComplete(Sched *, SchedTask *);
s32 func_80271F48(OSMesgQueue *, OSMesg, s32);

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/2C560/osCreateScheduler.s")
#else
void osCreateScheduler(Sched *sc, void *stack, OSPri priority, u8 mode, u8 numFields) {
    sc->audioListTail = (SchedTask *) &sc->audioListHead;
    sc->gfxListTail = (SchedTask *) &sc->gfxListHead;
    D_8036BF10 = 0;
    D_8036BF1C = NULL;
    osCreateMesgQueue(&sc->interruptQ, sc->intBuf, 16);
    osCreateMesgQueue(&sc->cmdQ, sc->cmdMsgBuf, 16);
    osCreateViManager(0xFE);
    osViSetMode(&D_80306E70[mode]);
    osViBlack(TRUE);
    osSetEventMesg(OS_EVENT_SP, &sc->interruptQ, (OSMesg) 0x29B);
    osSetEventMesg(OS_EVENT_DP, &sc->interruptQ, (OSMesg) 0x29C);
    osSetEventMesg(OS_EVENT_PRENMI, &sc->interruptQ, (OSMesg) 0x29D);
    osSetEventMesg(OS_EVENT_FAULT, &sc->interruptQ, (OSMesg) 0x2A0);
    osViSetEvent(&sc->interruptQ, (OSMesg) 0x29A, numFields);
    osCreateThread(&sc->thread, 5, __scMain, sc, stack, priority);
    osStartThread(&sc->thread);
}
#endif

void osScAddClient(Sched *sc, SchedClient *c, OSMesgQueue *msgQ, s32 arg3, s32 arg4) {
    OSIntMask mask;

    mask = osSetIntMask(OS_IM_NONE);
    c->msgQ = msgQ;
    c->next = sc->clientList;
    sc->clientList = c;
    c->unk8 = arg3;
    c->unkC = arg4;
    osSetIntMask(mask);
}

void osScRemoveClient(Sched *sc, SchedClient *c) {
    SchedClient *client;
    SchedClient *prev;
    OSIntMask mask;

    client = sc->clientList;
    prev = NULL;
    mask = osSetIntMask(OS_IM_NONE);
    while (client != NULL) {
        if (client == c) {
            if (prev != NULL) {
                prev->next = c->next;
            } else {
                sc->clientList = c->next;
            }
            break;
        }
        prev = client;
        client = client->next;
    }
    osSetIntMask(mask);
}

OSMesgQueue *osScGetCmdQ(Sched *sc) {
    return &sc->cmdQ;
}

extern OSMesgQueue *D_8036BF90;
extern OSMesg D_8036BF94;
u32 func_802A1320(void);
void func_802712B4(Sched *, SchedTask *);
void func_802712FC(Sched *);
void __scHandleRetrace(Sched *);
void __scHandleRSP(Sched *);
void __scHandleRDP(Sched *);

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/2C560/__scMain.s")
#else
void __scMain(void *arg) {
    OSMesg msg;
    Sched *sc;
    SchedClient *client;

    sc = arg;
    for (;;) {
        osRecvMesg(&sc->interruptQ, &msg, OS_MESG_BLOCK);
        if (!(func_802A1320() & 0x1000)) {
            for (client = sc->clientList; client != NULL; client = client->next) {
                osSendMesg(client->msgQ, (OSMesg) 0x29D, OS_MESG_NOBLOCK);
            }
            D_8036BF10 = 1;
            osViBlack(TRUE);
            func_8029A7E4("GO %x\n", osDpGetStatus());
            osDpSetStatus(DPC_CLR_FREEZE);
            func_8029A7E4("current=%x start=%x end=%x dpstat=%x spstat=%x\n", IO_READ(DPC_CURRENT_REG),
                          IO_READ(DPC_START_REG), IO_READ(DPC_END_REG), IO_READ(DPC_STATUS_REG),
                          IO_READ(SP_STATUS_REG));
            func_8029A7E4("GO %x\n", osDpGetStatus());
            for (;;) {
            }
        }
        switch ((s32) msg) {
            case 0x29A:
                if (++D_8036BFB8 % 480 == 0) {
                    D_8036BEF8 = D_8036BF00;
                    D_8036BF08 = D_8036BF0C;
                }
                __scHandleRetrace(sc);
                break;
            case 0x29E:
                func_802712FC(sc);
                break;
            case 0x29B:
                __scHandleRSP(sc);
                break;
            case 0x29C:
                __scHandleRDP(sc);
                break;
            case 0x29F:
                osSendMesg(D_8036BF90, D_8036BF94, OS_MESG_BLOCK);
                break;
            case 0x29D:
                for (client = sc->clientList; client != NULL; client = client->next) {
                    osSendMesg(client->msgQ, (OSMesg) 0x29D, OS_MESG_NOBLOCK);
                }
                D_8036BF10 = 1;
                osViBlack(TRUE);
                func_8029A7E4("%x\n", osDpGetStatus());
                osDpSetStatus(DPC_CLR_FREEZE);
                func_8029A7E4("current=%x start=%x end=%x dpstat=%x spstat=%x\n", IO_READ(DPC_CURRENT_REG),
                              IO_READ(DPC_START_REG), IO_READ(DPC_END_REG), IO_READ(DPC_STATUS_REG),
                              IO_READ(SP_STATUS_REG));
                func_8029A7E4("%x\n", osDpGetStatus());
                for (;;) {
                }
            case 0x2A0:
                func_8029A7E4(" *** CPU FAULT *** - UNFREEZING RDP?\n");
                while (osViGetCurrentFramebuffer() != osViGetNextFramebuffer()) {
                }
                osDpSetStatus(DPC_CLR_FREEZE);
                for (;;) {
                }
            default:
                func_802712B4(sc, (SchedTask *) msg);
                break;
        }
    }
}
#endif

void func_802712B4(Sched *sc, SchedTask *t) {
    __scAppendList(sc, t);
    if (sc->curRSPTask == NULL) {
        func_80271CE4(sc, 1);
    }
}

void func_802712FC(Sched *sc) {
    if (sc->curRSPTask != NULL) {
        __scYield(sc);
    } else {
        D_8036BF00 = 0;
        func_80271CE4(sc, 0);
    }
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/2C560/__scHandleRetrace.s")
#else
void __scHandleRetrace(Sched *sc) {
    SchedTask *t;
    SchedClient *client;
    s32 i;
    s32 count;

    sc->frameCount++;
    if (D_802E8BD0 == 0) {
        sc->unk280++;
    }
    D_8036BF38 = osGetTime();
    if (D_8036BF1C != NULL) {
        osViSwapBuffer(D_8036BF1C->framebuffer);
        D_8036BF18 = D_8036BF14;
        D_8036BF14 = sc->frameCount + 1;
        osDpSetStatus(DPC_SET_FREEZE);
        if (D_8036BF1C->msgQ != NULL) {
            func_80271F48(D_8036BF1C->msgQ, D_8036BF1C->msg, OS_MESG_NOBLOCK);
        }
        D_8036BF1C = NULL;
    } else if (osViGetCurrentFramebuffer() == osViGetNextFramebuffer() && (osDpGetStatus() & DPC_STATUS_FREEZE)) {
        sc->unk288 = osGetTime();
        osDpSetStatus(DPC_CLR_FREEZE);
    }
    count = sc->cmdQ.validCount;
    for (i = 0; i < count; i++) {
        if (osRecvMesg(&sc->cmdQ, (OSMesg *) &t, OS_MESG_NOBLOCK) == -1) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "osRecvMesg(&sc->cmdQ, (OSMesg *)&rspTask, OS_MESG_NOBLOCK) != -1", "sched.c", 0x1BD);
        }
        if (sc->frameCount % t->client->unk8 == 0) {
            __scAppendList(sc, t);
        } else {
            osSendMesg(&sc->cmdQ, t, OS_MESG_NOBLOCK);
        }
    }
    if (sc->audioListHead != NULL && !(sc->frameCount & 1)) {
        osSetTimer(&D_8036BF78, 280000, 0, sc->audioListHead->client->msgQ, (OSMesg) 5);
    }
    for (client = sc->clientList; client != NULL; client = client->next) {
        if (client->unkC == 3) {
            osSendMesg(client->msgQ, (OSMesg) 0x29A, OS_MESG_NOBLOCK);
        }
    }
}
#endif

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/2C560/__scHandleRSP.s")
#else
void __scHandleRSP(Sched *sc) {
    SchedTask *t;
    OSTime time;

    if (sc->curRSPTask == NULL) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sc->curRSPTask", "sched.c", 0x1F2);
    }
    t = sc->curRSPTask;
    sc->curRSPTask = NULL;
    if (t->state == 3) {
        D_8036BF00 = osGetTime() - D_8036BEF0;
        if (D_8036BF00 > 0x86470) {
            func_8029A7E4("Silly yield time of %llu ticks\n", D_8036BF00);
        }
        if (D_8036BF00 > D_8036BEF8) {
            D_8036BEF8 = D_8036BF00;
        }
        if (!osSpTaskYielded(&t->list)) {
            t->state = 2;
            t->flags |= 4;
            __scTaskComplete(sc, t);
        }
        if (sc->audioListHead == NULL) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sc->audioListHead", "sched.c", 0x21A);
        }
        if (sc->audioListHead == NULL) {
            func_8029A7E4("Yield took %llu, max %llu\n", D_8036BF00, D_8036BEF8);
        }
        func_80271CE4(sc, 0);
        return;
    }
    if (t->flags & 0x40) {
        time = osGetTime();
        D_8036BF24 = (time - sc->unk290) / 7825;
        D_802FA270 = 1;
    } else if (t->list.t.type == M_AUDTASK) {
        D_8036BF50 = osGetTime();
        D_8036BF40 = D_8036BF48;
    }
    t->state = 2;
    t->flags |= 4;
    if (sc->curRSPTask != NULL) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sc->curRSPTask==0", "sched.c", 0x230);
    }
    if (__scTaskComplete(sc, t)) {
        if (sc->gfxListHead != NULL && sc->gfxListHead->flags != 0x47) {
            func_80271CE4(sc, 1);
        }
    }
}
#endif

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/2C560/__scHandleRDP.s")
#else
void __scHandleRDP(Sched *sc) {
    SchedTask *t;
    s32 pad;
    OSTime time;

    if (sc->curRDPTask == NULL) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sc->curRDPTask", "sched.c", 0x24A);
    }
    t = sc->curRDPTask;
    sc->curRDPTask = NULL;
    t->flags |= 8;
    if (sc->frameCount != D_8036BF14 || (D_80364A90 & 0xC9FD0FE79BFF80B0)) {
        D_8036BF1C = NULL;
        osViSwapBuffer(t->framebuffer);
        D_8036BF18 = D_8036BF14;
        D_8036BF14 = sc->frameCount + 1;
        osDpSetStatus(DPC_SET_FREEZE);
    } else {
        D_8036BF1C = t;
    }
    time = osGetTime();
    D_8036BF20 = (time - sc->unk288) / 7825;
    if (D_80358060 == 3) {
        osViBlack(FALSE);
    }
    __scTaskComplete(sc, t);
}
#endif

s32 __scTaskComplete(Sched *sc, SchedTask *t) {
    s32 sp24;
    s32 sp20;
    s32 sp1C;
    s32 sp18;

    sp20 = t->flags & 3;
    sp1C = (t->flags >> 2) & 3;
    sp18 = t->list.t.type;
    if (!(t->flags & 0x40)) {
        sp20 &= 1;
        sp1C &= 1;
    }
    if (sp20 == sp1C) {
        if (sp18 == M_GFXTASK) {
            if (sc->gfxListHead == NULL) {
                func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sc->gfxListHead", "sched.c", LINE_EU(0x27C, 0x285));
            }
            sc->gfxListHead = sc->gfxListHead->next;
            if (sc->gfxListHead == NULL) {
                sc->gfxListTail = (SchedTask *) &sc->gfxListHead;
            }
        }
        if (t->msgQ != NULL && (D_8036BF1C == NULL || sp18 != M_GFXTASK)) {
            if (t->flags & 0x40) {
                sp24 = func_80271F48(t->msgQ, t->msg, OS_MESG_NOBLOCK);
            } else {
                sp24 = osSendMesg(t->msgQ, t->msg, OS_MESG_NOBLOCK);
            }
            if (sp24 == -1) {
                func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "rv!=-1", "sched.c", LINE_EU(0x289, 0x292));
            }
        }
        D_8036BFBC = 1;
    } else {
        D_8036BFBC = 0;
    }
    return D_8036BFBC;
}

void __scAppendList(Sched *sc, SchedTask *t) {
    s32 type;

    type = t->list.t.type;
    if (type != M_AUDTASK && type != M_GFXTASK) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "(type == M_AUDTASK) || (type == M_GFXTASK)", "sched.c", LINE_EU(0x29C, 0x2A5));
    }
    if (type == M_AUDTASK) {
        sc->audioListTail->next = t;
        sc->audioListTail = t;
    } else {
        sc->gfxListTail->next = t;
        sc->gfxListTail = t;
    }
    t->next = NULL;
    t->state = 2;
}

#ifdef VERSION_EU
#pragma GLOBAL_ASM("asm/nonmatchings/hd_code/2C560/func_80271CE4.s")
#else
void func_80271CE4(Sched *sc, s32 arg1) {
    SchedTask *t;
    OSTime time;

    if (sc->curRSPTask != NULL) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "!sc->curRSPTask", "sched.c", 0x2B8);
    }
    if (arg1 == 0) {
        t = sc->audioListHead;
        if (t == NULL) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "t", "sched.c", 0x2BD);
        }
        if (t == NULL) {
            return;
        }
        sc->audioListHead = sc->audioListHead->next;
        if (sc->audioListHead == NULL) {
            sc->audioListTail = (SchedTask *) &sc->audioListHead;
        }
        D_8036BF48 = osGetTime();
    } else {
        t = sc->gfxListHead;
        if (D_802FA270 != 0) {
            sc->unk290 = osGetTime();
            time = osGetTime();
            D_8036BF2C = (time - D_8036BF38) / 7825;
            D_802FA270 = 0;
        }
    }
    t->state = 1;
    osSpTaskLoad(&t->list);
    osSpTaskStartGo(&t->list);
    sc->curRSPTask = t;
    if (t->flags & 0x40) {
        sc->curRDPTask = t;
    }
}
#endif

void __scYield(Sched *sc) {
    if (sc->curRSPTask->list.t.type == M_AUDTASK) {
        func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sc->curRSPTask->list.t.type != M_AUDTASK", "sched.c", LINE_EU(0x2DF, 0x2E8));
    }
    if (sc->curRSPTask->list.t.type == M_GFXTASK) {
        if (sc->curRSPTask->state == 3) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "sc->curRSPTask->state != RSP_STATE_SUSPENDED", "sched.c", LINE_EU(0x2E3, 0x2EC));
        }
        sc->curRSPTask->state = 3;
        D_8036BEF0 = osGetTime();
        osSpTaskYield();
    }
}

s32 func_80271F48(OSMesgQueue *mq, OSMesg msg, s32 flag) {
    OSTime sp28;
    OSTime sp20;
    s32 sp1C;

    sp28 = D_8036BF38 + 391250 - osGetTime();
    sp20 = osGetTime();
    sp1C = 0;
    osSendMesg(mq, msg, flag);
}
