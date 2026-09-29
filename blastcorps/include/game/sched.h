#ifndef GAME_SCHED_H
#define GAME_SCHED_H

#include "game/types.h"

/*
 * The game's scheduler: Rare's version of SGI's sample scheduler (sched.c,
 * the asserts name it and its fields: "sc->curRSPTask", "sc->gfxListHead",
 * "osSendMesg(osScGetCmdQ(&sc), ...)").  The layouts follow PR/sched.h with
 * Rare's additions; they are named apart from it (Sched, not OSSched) since
 * they differ.  The one instance is D_80315440 (hd_code 00000.c's .bss, where
 * it is still split into the variables other files name: D_803156C4 is its
 * frameCount), created by osCreateScheduler.
 */

typedef struct SchedClient {
    /* 0x00 */ struct SchedClient *next;
    /* 0x04 */ OSMesgQueue *msgQ;
    /* 0x08 */ s32 unk8;          /* the task goes out on frames where frameCount % unk8 == 0 */
    /* 0x0C */ s32 unkC;          /* 3: also sent the retrace message */
} SchedClient;
SIZE_CHECK(SchedClient, 0x10);

typedef struct SchedTask {
    /* 0x00 */ struct SchedTask *next;
    /* 0x04 */ s32 state;          /* 1 on the RSP, 2 queued or done, RSP_STATE_SUSPENDED */
    /* 0x08 */ u32 flags;
    /* 0x0C */ void *framebuffer;
    /* 0x10 */ OSTask list;
    /* 0x50 */ SchedClient *client; /* the client that sent it (the audio thread sets its own) */
    /* 0x54 */ OSMesgQueue *msgQ;
    /* 0x58 */ OSMesg msg;
    /* 0x5C */ u32 unk5C;
} SchedTask;
SIZE_CHECK(SchedTask, 0x60);

/* __scYield's assert "sc->curRSPTask->state != RSP_STATE_SUSPENDED" tests 3. */
#define RSP_STATE_SUSPENDED 3

typedef struct Sched {
    /* 0x000 */ OSMesgQueue interruptQ;
    /* 0x018 */ OSMesg intBuf[16];
    /* 0x058 */ OSMesgQueue cmdQ;
    /* 0x070 */ OSMesg cmdMsgBuf[16];
    /* 0x0B0 */ OSThread thread;
    /* 0x260 */ SchedClient *clientList;
    /* 0x264 */ SchedTask *audioListHead;
    /* 0x268 */ SchedTask *gfxListHead;
    /* 0x26C */ SchedTask *audioListTail;
    /* 0x270 */ SchedTask *gfxListTail;
    /* 0x274 */ SchedTask *curRSPTask;
    /* 0x278 */ SchedTask *curRDPTask;
    /* 0x27C */ s32 unk27C;
    /* 0x280 */ u32 unk280;       /* retraces while the game isn't paused (D_802E8BD0): hd.c prints the difference as "TIME IN LEVEL=%d"; D_803156C0 */
    /* 0x284 */ u32 frameCount;   /* retraces, as in SGI's; D_803156C4 */
    /* 0x288 */ OSTime unk288;
    /* 0x290 */ OSTime unk290;
} Sched;
SIZE_CHECK(Sched, 0x298);

extern Sched D_80315440;
extern SchedClient D_803156D8; /* the game's own client */

/* D_80218EE0 (hd_front_end E7B0.c) is the Controller Pak thread's client. */
extern SchedClient D_80218EE0;

/* The scheduler's interface (hd_code 2C560.c; the names and the evidence
 * for them are in symbols_known.txt).  osScAddClient has two fields more than
 * SGI's: SchedClient.unk8 and unkC. */
void osCreateScheduler(Sched *sc, void *stack, OSPri priority, u8 mode, u8 numFields);
void osScAddClient(Sched *sc, SchedClient *c, OSMesgQueue *msgQ, s32 arg3, s32 arg4);
void osScRemoveClient(Sched *sc, SchedClient *c);
OSMesgQueue *osScGetCmdQ(Sched *sc);

/* The scheduler's messages (osSetEventMesg/osViSetEvent in osCreateScheduler). */
#define SCHED_MSG_RETRACE 0x29A
#define SCHED_MSG_SP 0x29B
#define SCHED_MSG_DP 0x29C
#define SCHED_MSG_PRENMI 0x29D
#define SCHED_MSG_FAULT 0x2A0

/* Sched fields other files name by their own symbols (Sched.unk280 and
 * frameCount), and 2C560.c's osGetTime() at the last retrace. */
#ifndef TARGET_PC   /* the port reads them through calls (port_game.h) */
extern u32 D_803156C0;
extern u32 D_803156C4;
#endif
extern OSTime D_8036BF38;

#endif
