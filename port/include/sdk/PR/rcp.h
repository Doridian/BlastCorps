/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the RCP's
 * registers the game's C names, from the hardware's documented memory map.
 */
#ifndef PORT_SDK_RCP_H
#define PORT_SDK_RCP_H

#include <PR/R4300.h>

/* uncached reads and writes of a physical address */
#define IO_READ(addr) (*(vu32 *)PHYS_TO_K1(addr))
#define IO_WRITE(addr, data) (*(vu32 *)PHYS_TO_K1(addr) = (u32)(data))

/* the RSP */
#define SP_BASE_REG 0x04040000
#define SP_STATUS_REG (SP_BASE_REG + 0x10)
#define SP_STATUS_HALT 0x001
#define SP_STATUS_BROKE 0x002
#define SP_STATUS_DMA_BUSY 0x004
#define SP_STATUS_DMA_FULL 0x008
#define SP_STATUS_IO_FULL 0x010
#define SP_STATUS_SSTEP 0x020
#define SP_STATUS_INTR_BREAK 0x040
#define SP_STATUS_SIG0 0x080
#define SP_STATUS_SIG1 0x100
#define SP_STATUS_SIG2 0x200
#define SP_STATUS_SIG3 0x400
#define SP_STATUS_SIG4 0x800
#define SP_STATUS_SIG5 0x1000
#define SP_STATUS_SIG6 0x2000
#define SP_STATUS_SIG7 0x4000
#define SP_STATUS_YIELD SP_STATUS_SIG0
#define SP_STATUS_YIELDED SP_STATUS_SIG1
#define SP_STATUS_TASKDONE SP_STATUS_SIG2
#define SP_STATUS_RSPSIGNAL SP_STATUS_SIG3
#define SP_STATUS_CPUSIGNAL SP_STATUS_SIG4

/* the RDP's command interface */
#define DPC_BASE_REG 0x04100000
#define DPC_START_REG (DPC_BASE_REG + 0x00)
#define DPC_END_REG (DPC_BASE_REG + 0x04)
#define DPC_CURRENT_REG (DPC_BASE_REG + 0x08)
#define DPC_STATUS_REG (DPC_BASE_REG + 0x0C)
/* its status, as read */
#define DPC_STATUS_XBUS_DMEM_DMA 0x001
#define DPC_STATUS_FREEZE 0x002
#define DPC_STATUS_FLUSH 0x004
#define DPC_STATUS_START_GCLK 0x008
#define DPC_STATUS_TMEM_BUSY 0x010
#define DPC_STATUS_PIPE_BUSY 0x020
#define DPC_STATUS_CMD_BUSY 0x040
#define DPC_STATUS_CBUF_READY 0x080
#define DPC_STATUS_DMA_BUSY 0x100
#define DPC_STATUS_END_VALID 0x200
#define DPC_STATUS_START_VALID 0x400
/* and as written */
#define DPC_CLR_XBUS_DMEM_DMA 0x0001
#define DPC_SET_XBUS_DMEM_DMA 0x0002
#define DPC_CLR_FREEZE 0x0004
#define DPC_SET_FREEZE 0x0008
#define DPC_CLR_FLUSH 0x0010
#define DPC_SET_FLUSH 0x0020

/* the peripheral interface */
#define PI_BASE_REG 0x04600000
#define PI_STATUS_REG (PI_BASE_REG + 0x10)
#define PI_STATUS_DMA_BUSY 0x01
#define PI_STATUS_IO_BUSY 0x02
#define PI_STATUS_ERROR 0x04

/* the serial interface: a joybus command's error bits */
#define CHNL_ERR_MASK 0xC0

/* the video interface's clocks */
#define VI_NTSC_CLOCK 48681812
#define VI_PAL_CLOCK 49656530
#define VI_MPAL_CLOCK 48628316

#endif
