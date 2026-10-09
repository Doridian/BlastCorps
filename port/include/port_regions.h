/*
 * The game's fixed buffers as regions: memory the N64 side uses by address
 * rather than as a variable (docs/PORT.md, "Memory model").  Each has a
 * name (the address name the repo gives it, us.v11's address as in every
 * version) and a size, and the name is the only spelling of its address
 * on the N64 side.  This table is where they are declared, once:
 *
 *   - the fixed builds: an absolute symbol at its N64 address
 *     (gen/port_fixed.ld, from tools/gen_syms.py regions);
 *   - the movable build: port-arena resolves the name to its N64 address
 *     (bepass/Arena.cpp, -port-arena-regions=gen/port_regions.txt);
 *   - the scattered layout (PORT_SCATTER): port-arena gives the region
 *     room of its own after the scattered variables, keeping its N64
 *     address's offset in 4 KB, with padding before it, and its N64 place
 *     goes on the list of what nothing should touch (__port_scatter_bad),
 *     so that an address left as a number shows.  Its sub-regions move
 *     with it.  A region marked 0 under "moves" stays unless named in
 *     -port-arena-scatter-regions=TAG,... (the level pool, which the front
 *     end's variables are inside of: it moves with them).
 *
 * PORT_REGION(name, tag, start, end, moves)
 *     a region of its own, N64 addresses [start, end)
 * PORT_REGION_LINK(name, tag, end, moves)
 *     ... from the N64 link's own address of `name` (it differs between
 *     versions) to end
 * PORT_SUBREGION(name, tag, parent, offset, size)
 *     a name inside another region, `offset` bytes in: it moves with it
 *
 * tools/gen_syms.py reads this file (one entry a line, as below).  The
 * game's C declares the names it uses with its own types; what the table
 * gives C is the sizes and offsets (PORT_REGION_SIZE_name,
 * PORT_REGION_OFF_name).
 */
#ifndef PORT_REGIONS_H
#define PORT_REGIONS_H

#define PORT_REGIONS(PORT_REGION, PORT_REGION_LINK, PORT_SUBREGION) \
    /* the boot code's words: osTvType, osRomBase, osResetType, osMemSize, \
       osAppNMIBuffer... (up to the framebuffers) */ \
    PORT_REGION(D_80000300, boot, 0x80000300, 0x80000400, 1) \
    /* the two 320x240 RGBA16 framebuffers */ \
    PORT_REGION(D_80000400, framebuffers, 0x80000400, 0x8004B400, 1) \
    /* the level pool (bump pointer D_80358070): at its start the RSP task's \
       output buffer, the decompression scratch and the boot sound-bank \
       staging, all in one */ \
    PORT_REGION(D_8004B400, pool, 0x8004B400, 0x8021ED00, 0) \
    /* the ghost's two buffers (50670.c), after the output buffer */ \
    PORT_SUBREGION(D_80055400, ghost0, D_8004B400, 0xA000, 0x10000) \
    PORT_SUBREGION(D_80065400, ghost1, D_8004B400, 0x1A000, 0x10000) \
    /* the front end's area (its .text, .data, .bss, then free), the pool's \
       tail */ \
    PORT_SUBREGION(D_801E7000, front_end, D_8004B400, 0x19BC00, 0x37D00) \
    /* the effects' heap's limits (port/engine/60F60.c) */ \
    PORT_SUBREGION(D_8020ED00, limit20, D_8004B400, 0x1C3900, 0) \
    PORT_SUBREGION(D_8021DD00, limit, D_8004B400, 0x1D2900, 0) \
    /* init's area: the depth buffer, and where compressed loads are staged. \
       It stays until the N64 side stages there by its name (46C20.c, \
       port/engine/5CB60.c): the DMA there is the host's, by address, which \
       the check can't take to where the region went, so the staged data \
       and its inflate would part */ \
    PORT_REGION(D_8021ED00, init, 0x8021ED00, 0x802447C0, 0) \
    /* segment 1's static data, after hd_code's .bss (LZSS from the ROM) */ \
    PORT_REGION_LINK(D_803FF600, static, 0x803FFFF8, 1) \
    /* the front end's compressed ROM range, init's hand-over to hd_code */ \
    PORT_REGION(D_803FFFF8, handover, 0x803FFFF8, 0x80400000, 1) \
    PORT_SUBREGION(D_803FFFFC, handover_end, D_803FFFF8, 4, 4)

#define PORT_REGION_ENUM_(name, tag, start, end, moves) PORT_REGION_SIZE_##name = (end) - (start),
#define PORT_REGION_ENUM_LINK_(name, tag, end, moves)
#define PORT_SUBREGION_ENUM_(name, tag, parent, offset, size) \
    PORT_REGION_SIZE_##name = (size), PORT_REGION_OFF_##name = (offset),
enum { PORT_REGIONS(PORT_REGION_ENUM_, PORT_REGION_ENUM_LINK_, PORT_SUBREGION_ENUM_) };
#undef PORT_REGION_ENUM_
#undef PORT_REGION_ENUM_LINK_
#undef PORT_SUBREGION_ENUM_

#endif
