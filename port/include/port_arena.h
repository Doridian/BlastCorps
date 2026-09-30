/*
 * Movable memory (PORT_MOVABLE, docs/PORT.md "Movable memory"): the N64
 * addresses from 0x80000000 up to PORT_ARENA_SPAN live in the host's arena,
 * port_arena, wherever the host put it.  Everything else is still where
 * its address says.  The game's C gets there through port-arena
 * (bepass/Arena.cpp), the translated code through RECOMP_HOST, the host
 * through port_ptr.
 */
#ifndef PORT_ARENA_H
#define PORT_ARENA_H

#include <stdint.h>

#ifdef PORT_MOVABLE
/* the moved ranges, by offset from 0x80000000: RDRAM, and the fibers' host
   stacks (PORT_STACK_BASE, port.h: the threads' 16 MB there) */
#define PORT_ARENA_SPAN 0x00400000u
#define PORT_ARENA_STACKS 0x10000000u
#define PORT_ARENA_STACKS_SPAN 0x01000000u
#define PORT_ARENA_SIZE (PORT_ARENA_STACKS + PORT_ARENA_STACKS_SPAN)
/* 0x80000000 until main moves RDRAM (port/host/runtime.c) */
extern uint8_t *port_arena;
static inline int port_arena_moved(uint32_t off) {
    return off < PORT_ARENA_SPAN || off - PORT_ARENA_STACKS < PORT_ARENA_STACKS_SPAN;
}
/* a KSEG0 address */
static inline void *port_host(uint32_t addr) {
    uint32_t off = addr ^ 0x80000000u;
    return port_arena_moved(off) ? (void *)(port_arena + off) : (void *)(uintptr_t)addr;
}
/* the N64 address of host memory in the arena */
static inline uint32_t port_n64(const void *p) {
    return 0x80000000u | (uint32_t)((const uint8_t *)p - port_arena);
}
#define RECOMP_HOST(rdram, a) ((uint8_t *)port_host(0x80000000u | ((a) & 0x1FFFFFFFu)))
#else
static inline void *port_host(uint32_t addr) { return (void *)(uintptr_t)addr; }
static inline uint32_t port_n64(const void *p) { return (uint32_t)(uintptr_t)p; }
#endif

#endif
