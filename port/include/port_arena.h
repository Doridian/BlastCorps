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
#define PORT_ARENA_SPAN 0x00400000u         /* RDRAM */
/* 0x80000000 until main moves RDRAM (port/host/runtime.c) */
extern uint8_t *port_arena;
/* a KSEG0 address */
static inline void *port_host(uint32_t addr) {
    uint32_t off = addr - 0x80000000u;
    return off < PORT_ARENA_SPAN ? (void *)(port_arena + off) : (void *)(uintptr_t)addr;
}
#define RECOMP_HOST(rdram, a) ((uint8_t *)port_host(0x80000000u | ((a) & 0x1FFFFFFFu)))
#else
static inline void *port_host(uint32_t addr) { return (void *)(uintptr_t)addr; }
#endif

#endif
