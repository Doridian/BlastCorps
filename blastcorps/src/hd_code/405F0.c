#include "common.h"
#include "game/frame.h"
#include "game/game.h"
#include "game/sched.h"
#include "functions.h"

/* An RSP task with the bookkeeping sent along with it: 0x60 bytes, two per
 * task slot (double-buffered by D_8035805C). */
extern u16 D_80000400[][320 * 240];
extern u64 D_80207090[];
extern u64 D_80210690[];
extern u64 D_802E53F0[];
/* rspboot, 0xD0 bytes; the microcode after it starts where it ends. */
extern u64 D_802E6820[0xD0 / sizeof(u64)];
extern u64 D_802E68F0[];
extern u64 D_8030E390[];
extern OSMesgQueue D_803153D8;
extern u64 D_80367750[];
extern u64 D_8036AFB0[];

/* .bss, 0x8036E660-0x8036EA60 (tools/bss_c.py) */
u64 *D_8036E660[6];
u64 *D_8036E678[5];
u8 D_8036E68C[7];
u64 *D_8036E694;
SchedTask D_8036E698[5][2];

void func_80284DB0(void) {
    D_8036E660[0] = D_80207090;
    D_8036E678[0] = D_80210690;
    D_8036E660[1] = D_802E53F0;
    D_8036E678[1] = D_8030E390;
    D_8036E660[2] = D_802E53F0;
    D_8036E678[2] = D_8030E390;
    D_8036E660[3] = D_802E53F0;
    D_8036E678[3] = D_8030E390;
}

void func_80284E54(Gfx *arg0, s32 arg1, u8 arg2, u8 arg3, s32 arg4, u8 arg5) {
    SchedTask *t;
    s32 size;

    size = arg1 * 8;
    t = &D_8036E698[arg2][D_8035805C];
    D_8036E68C[arg2] = 1;
    t->list.t.type = M_GFXTASK;
    if (arg2 == 4) {
        t->list.t.flags = 2;
    } else {
        t->list.t.flags = 0;
    }
    t->list.t.ucode_boot = D_802E6820;
#ifdef TARGET_PC
    t->list.t.ucode_boot_size = sizeof(D_802E6820);
#else
    t->list.t.ucode_boot_size = (u8 *)D_802E68F0 - (u8 *)D_802E6820;
#endif
    t->list.t.ucode = D_8036E660[arg2];
    t->list.t.ucode_data = D_8036E678[arg2];
    t->list.t.ucode_size = 0x1000;
    t->list.t.ucode_data_size = 0x800;
    t->list.t.dram_stack = D_80367750;
    t->list.t.dram_stack_size = 0x400;
    t->list.t.output_buff = D_8036E694;
    t->list.t.output_buff_size = (u64 *)((u8 *)D_8036E694 + 0xA000);
    t->list.t.data_ptr = (u64 *)arg0;
    t->list.t.data_size = size;
    t->list.t.yield_data_ptr = D_8036AFB0;
    t->list.t.yield_data_size = 0x900;
    t->next = NULL;
    t->msgQ = &D_803153D8;
    t->msg = OS_MESG((arg2 << 16) | arg4);
    t->flags = 3;
    if (arg3) {
        t->flags |= 0x40;
    }
    t->framebuffer = D_80000400[D_8035805C];
    t->client = &D_803156D8;
    if (arg5) {
        osWritebackDCacheAll();
    } else {
        osWritebackDCache(t, 0x60);
        osWritebackDCache(arg0, size);
        osWritebackDCache(&D_803156F8[D_8035805C], sizeof(FrameGame));
    }
    osSendMesg(&D_80315440.interruptQ, t, OS_MESG_BLOCK);
}

void func_80285110(u32 arg0) {
    u32 msg;

    do {
        osRecvMesgInt(&D_803153D8, msg, OS_MESG_BLOCK);
        D_8036E68C[msg >> 16] = 0;
        msg &= 0xFFFF;
        if (msg != arg0) {
            func_8029A7E4("Task %d received message %d\n", arg0, msg);
        }
    } while (msg != arg0);
}
