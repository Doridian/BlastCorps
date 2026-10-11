/*
 * The names that are segment addresses (D_02000000, segment 2's frame;
 * D_01000010 and D_01000038, the display lists in segment 1's static
 * data), in the plain build (PORT_PLAIN).
 *
 * The game's C declares them as objects (a FrameGame, a FrameBuf, Mtx[],
 * Vtx[], Gfx[]) and puts their addresses, and its members' (
 * &D_02000000.unkA4E0), in its display lists, which the RSP resolves
 * through the segment table.  The other builds make each name an absolute
 * symbol at its segment address, with the tag (gbi.h's G_SEGTAG), an
 * "object" at a misaligned number.  Here each is a real object, aligned
 * and never written, a proxy: the renderer turns a word inside one into
 * its segment and offset (port_seg_unproxy, port/host/gfx.c), tagged as
 * every segmented word is.  So the C's addresses are addresses of real
 * objects, which any compiler may assume aligned, and the game's C is as
 * it is.
 */
#include "common.h"
#include "port.h"
#include "game/frame.h"

#ifdef PORT_PLAIN
Frame D_02000000 __attribute__((aligned(16)));
/* static_data's two lists (src/widen.c): the second follows the first's
   five commands, which the port widens to sizeof(Gfx) */
Gfx D_01000010[5];
Gfx D_01000038[1];

static const struct {
    const u8 *at;
    uintptr_t size;
    uintptr_t seg;          /* the segmented address of its start, tagged */
} proxies[] = {
    { (const u8 *)&D_02000000, sizeof D_02000000, G_SEGADDR(2, 0) },
    { (const u8 *)D_01000010, sizeof D_01000010, G_SEGADDR(1, 0x10) },
    { (const u8 *)D_01000038, sizeof D_01000038, G_SEGADDR(1, 0x10 + 5 * sizeof(Gfx)) },
};

uintptr_t port_seg_unproxy(uintptr_t w) {
    unsigned i;

    for (i = 0; i < sizeof proxies / sizeof proxies[0]; i++)
        if (w - (uintptr_t)proxies[i].at < proxies[i].size)
            return proxies[i].seg + (w - (uintptr_t)proxies[i].at);
    return w;
}
#endif
