/*
 * The port's own SDK headers (PR/ultratypes.h has the why): an RSP task,
 * as the scheduler hands it to the microcode.
 */
#ifndef PORT_SDK_SPTASK_H
#define PORT_SDK_SPTASK_H

#include <PR/ultratypes.h>

typedef struct {
    u32 type;                       /* M_GFXTASK, M_AUDTASK */
    u32 flags;                      /* OS_TASK_* */
    u64 *ucode_boot;
    u32 ucode_boot_size;
    u64 *ucode;
    u32 ucode_size;
    u64 *ucode_data;
    u32 ucode_data_size;
    u64 *dram_stack;
    u32 dram_stack_size;
    u64 *output_buff;
    u64 *output_buff_size;
    u64 *data_ptr;            /* the display or command list */
    u32 data_size;
    u64 *yield_data_ptr;
    u32 yield_data_size;
} OSTask_t;

typedef union {
    OSTask_t t;
    long long int force_structure_alignment;
} OSTask;

typedef u32 OSYieldResult;

#define OS_TASK_YIELDED 0x0001
#define OS_TASK_DP_WAIT 0x0002
#define OS_TASK_LOADABLE 0x0004
#define OS_TASK_SP_ONLY 0x0008

#define OS_YIELD_DATA_SIZE 0xc00
#define OS_YIELD_AUDIO_SIZE 0x400

#define osSpTaskStart(tp)                                                                                   \
    {                                                                                                       \
        osSpTaskLoad((tp));                                                                                 \
        osSpTaskStartGo((tp));                                                                              \
    }

extern void osSpTaskLoad(OSTask *tp);
extern void osSpTaskStartGo(OSTask *tp);
extern void osSpTaskYield(void);
extern OSYieldResult osSpTaskYielded(OSTask *tp);

#endif
