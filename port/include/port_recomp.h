/* What the generated glue (port/tools/gen_glue.py) needs. Host side. */
#ifndef PORT_RECOMP_H
#define PORT_RECOMP_H

#include <string.h>
#include "recomp.h"
#include "port.h"

/* The translated code reaches memory as rdram + (addr & 0x1FFFFFFF); with
   RDRAM mapped at 0x80000000 and everything else in the KSEG0 window that
   is the address itself. */
#ifdef PORT_MOVABLE
#define RDRAM port_arena          /* (the movable build: the arena, port_arena.h) */
#else
#define RDRAM ((uint8_t *)(uintptr_t)PORT_RDRAM_BASE)
#endif

recomp_context *port_ctx(void);

/* A call out of the translated code runs native C, which keeps none of its
   state in the context; but that C may call back into translated code that
   clobbers registers the IDO-compiled original would have preserved.  So
   the glue saves what the o32 ABI says a callee preserves. */
typedef struct {
    uint64_t s[8], gp, sp, fp;
    uint32_t f[12];
} port_callee_saved;

#define PORT_CALLEE_SAVE(ctx)                                       \
    port_callee_saved saved_;                                       \
    memcpy(saved_.s, &(ctx)->r[16], sizeof saved_.s);               \
    saved_.gp = (ctx)->gp; saved_.sp = (ctx)->sp; saved_.fp = (ctx)->fp; \
    memcpy(saved_.f, &(ctx)->f[20], sizeof saved_.f)

#define PORT_CALLEE_RESTORE(ctx)                                    \
    do {                                                            \
        memcpy(&(ctx)->r[16], saved_.s, sizeof saved_.s);           \
        (ctx)->gp = saved_.gp; (ctx)->sp = saved_.sp; (ctx)->fp = saved_.fp; \
        memcpy(&(ctx)->f[20], saved_.f, sizeof saved_.f);           \
    } while (0)

#endif
