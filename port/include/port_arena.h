/*
 * Movable memory (PORT_MOVABLE, docs/PORT.md "Movable memory"): game
 * memory is the arena, port_arena, wherever the host put it, holding N64
 * physical memory from 0: an N64 address a is at port_arena + (a &
 * 0x1FFFFFFF), KSEG0 or KSEG1.  The game's C gets there through port-arena
 * (bepass/Arena.cpp), the translated code through its `rdram` (which is
 * port_arena), the host through port_ptr.
 */
#ifndef PORT_ARENA_H
#define PORT_ARENA_H

#include <stdint.h>

#ifdef PORT_MOVABLE
/* what is where, by physical address (bepass/Arena.cpp has the same):
   RDRAM; from 0x400000 the N64 side's data that has no N64 address (to
   __port_arena_data_end); from PORT_ARENA_STACKS the fibers' host stacks
   (PORT_STACK_BASE, port.h) */
#define PORT_ARENA_SPAN 0x00400000u
#define PORT_ARENA_STACKS 0x00C00000u
#define PORT_ARENA_STACKS_SPAN 0x01000000u
#define PORT_ARENA_SIZE (PORT_ARENA_STACKS + PORT_ARENA_STACKS_SPAN)
/* set up by port_arena_init (port/host/runtime.c) before anything runs */
extern uint8_t *port_arena;
static inline void *port_host(uint32_t addr) { return port_arena + (addr & 0x1FFFFFFFu); }
/* the N64 address of host memory in the arena */
static inline uint32_t port_n64(const void *p) {
    return 0x80000000u | (uint32_t)((const uint8_t *)p - port_arena);
}
#else
static inline void *port_host(uint32_t addr) { return (void *)(uintptr_t)addr; }
static inline uint32_t port_n64(const void *p) { return (uint32_t)(uintptr_t)p; }
#endif

#endif
