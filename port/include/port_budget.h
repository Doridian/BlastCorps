/*
 * The port's memory budgets: every buffer whose size a small target cares
 * about (a console with 24-64 MB), in one place.  Each is what the N64 has
 * plus what the host's wider types add, computed at compile time from what
 * it holds: on a host whose pointers are 4 bytes (ILP32: i386, PowerPC,
 * MIPS o32, ARM, wasm32) every one is the N64's own size.  Each can be set
 * by -D from a host's toolchain file (port/tools/cross-*.cmake), and
 * `port/tools/test.py budget BUILD` reports what a build takes (its static
 * footprint, the pool's and the heap's high-waters) against a ceiling
 * (docs/PORT.md, "Memory budgets").
 *
 *   budget                N64          LP64 host        where
 *   framebuffers          0x4B000      the same         src/regions.c
 *   level pool            0x1D3900     + PORT_POOL_GROWTH  src/regions.c, port_regions.h
 *     its front end area  0x37D00      the same (its tail)
 *   init's area           0x25AC0      the same         src/pool.c
 *   static data (seg. 1)  0x918-0xD78  the same         src/regions.c (the version's)
 *   audio heap            0x2A280      + the growth     game/audio.h's AUDIO_HEAP_SIZE
 *                         (eu 0x24540)                  (libaudio_heap.h's formula: it
 *                                                       takes libaudio's types)
 *   thread stacks         (the N64's   PORT_STACK_SIZE  host/runtime.c, one per game
 *                         own, in .bss) each            thread created (about 8)
 *
 * Everything else the N64 side has is its variables (the game's .data and
 * .bss: about 1.3 MB where pointers are 4 bytes, 2.5 MB where they are 8),
 * and the host's own (the software renderer's frame, depth and in-between
 * buffers, the texture cache, the sound buffers and libc: about 5.4 MB in
 * the qemu builds, without SDL; 11 MB on x86 with SDL and OpenGL).
 */
#ifndef PORT_BUDGET_H
#define PORT_BUDGET_H

/* The level pool, N64 0x8004B400-0x8021ED00, its region (port_regions.h).
 * Its display lists take sizeof(Gfx) a command where the N64's take 8
 * (PORT_GFX_SIZE: 16 where pointers are 8 bytes, the words being as wide
 * as a pointer).  The pool grows by what that adds to the
 * most the N64's pool holds of display lists at once, PORT_POOL_GFX_N64
 * (src/pool.c stops the port if a level needs more): measured over the TAS
 * (us.v10) and the quick tier with attract.long in every version (test.py
 * budget's runs: the most is the TAS's level 4, 0x72820), this is that and
 * nearly a quarter more. */
#define PORT_POOL_N64 0x1D3900
#ifndef PORT_POOL_GFX_N64
#define PORT_POOL_GFX_N64 0x8E000
#endif
#ifndef PORT_GFX_SIZE
/* (two words as wide as a pointer, gbi.h's Gwords; src/pool.c checks it
   is sizeof(Gfx)) */
#define PORT_GFX_SIZE (2 * __SIZEOF_POINTER__)
#endif
#define PORT_POOL_GROWTH ((PORT_GFX_SIZE - 8) / 8 * PORT_POOL_GFX_N64)
#define PORT_POOL_SIZE (PORT_POOL_N64 + PORT_POOL_GROWTH)

/* init's area, the depth buffer and where compressed loads are staged
   (46C20.c, port/engine/5CB60.c: MEM_INIT_AREA), at 0x8021ED00 on the N64
   up to hd_code's load address: in the port a buffer of its own
   (src/pool.c), wherever the layout puts it */
#define PORT_INIT_AREA_SIZE (0x802447C0 - 0x8021ED00)

/* the framebuffers: two of 320x240 RGBA16 (the N64's) */
#define PORT_FB_SIZE (2 * 320 * 240 * 2)

/* the game's threads' stacks (host/runtime.c, the fibers'): one each of
   the threads it creates, as many as PORT_MAX_THREADS.  The N64's are the
   game's own arrays (a few KB each); the host's C needs more, its library
   calls and the sanitizers most. */
#ifndef PORT_STACK_SIZE
#define PORT_STACK_SIZE 0x00100000u
#endif
#ifndef PORT_MAX_THREADS
#define PORT_MAX_THREADS 16
#endif

#endif
