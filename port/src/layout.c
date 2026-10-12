/*
 * The game's layouts as the host reads them (port.h, "the game's layouts"):
 * sizeof and offsetof of what the host reads inside the game's bigger
 * structures (port/host/digest.c, host_game_vars.h, gfx.c), by this
 * build's N64-side compiler; and the compile-time checks that
 * host_game_types.h spells the structures it has as the game does.
 * The N64's offsets where pointers are 4 bytes; where they are 8 a
 * structure with pointers has the host's layout, and the table follows
 * it, so the host never spells an offset itself.
 */
#include "common.h"
#include "port.h"
#include "game/vehicle.h"
#include "game/objects.h"
#include "game/level.h"
#include "game/player.h"
#include "game/sched.h"
#include "game/game.h"
#include "game/frame.h"
#include "game/yoshi.h"
#include "host_game_types.h"

#define OFF(type, field) __builtin_offsetof(type, field)

static const u32 layout[PORT_LAYOUT_COUNT] = {
    [PORT_LAYOUT_VEHICLE_SIZE] = sizeof(Vehicle),
    [PORT_LAYOUT_VEHICLE_TYPE] = OFF(Vehicle, type),
    [PORT_LAYOUT_VSTATE_SIZE] = sizeof(VehicleState),
    [PORT_LAYOUT_VSTATE_HEADING] = OFF(VehicleState, unk4C),
    [PORT_LAYOUT_VSTATE_HEADING2] = OFF(VehicleState, unk4E),
    [PORT_LAYOUT_VSTATE_SPEED] = OFF(VehicleState, unk76),
    [PORT_LAYOUT_BUILDING_SIZE] = sizeof(Building),
    [PORT_LAYOUT_BUILDING_X] = OFF(Building, x),
    [PORT_LAYOUT_BUILDING_Y] = OFF(Building, y),
    [PORT_LAYOUT_BUILDING_Z] = OFF(Building, z),
    [PORT_LAYOUT_BUILDING_GROUPS] = OFF(Building, unkE9),
    [PORT_LAYOUT_BUILDING_GONE] = OFF(Building, unkEA),
    [PORT_LAYOUT_BUILDING_DAMAGE] = OFF(Building, damage),
    [PORT_LAYOUT_TNT_SIZE] = sizeof(TntCrate),
    [PORT_LAYOUT_TNT_X] = OFF(TntCrate, x),
    [PORT_LAYOUT_TNT_Y] = OFF(TntCrate, y),
    [PORT_LAYOUT_TNT_Z] = OFF(TntCrate, z),
    [PORT_LAYOUT_TNT_TIMER] = OFF(TntCrate, timer),
    [PORT_LAYOUT_TNT_ACTIVE] = OFF(TntCrate, active),
    [PORT_LAYOUT_BLOCK_SIZE] = sizeof(Block),
    [PORT_LAYOUT_BLOCK_X] = OFF(Block, x),
    [PORT_LAYOUT_BLOCK_Y] = OFF(Block, y),
    [PORT_LAYOUT_BLOCK_Z] = OFF(Block, z),
    [PORT_LAYOUT_BLOCK_IN_HOLE] = OFF(Block, unk11),
    [PORT_LAYOUT_AMMOBOX_SIZE] = sizeof(AmmoBox),
    [PORT_LAYOUT_AMMOBOX_COLLECTED] = OFF(AmmoBox, collected),
    [PORT_LAYOUT_RDU_SIZE] = sizeof(Rdu),
    [PORT_LAYOUT_RDU_COLLECTED] = OFF(Rdu, collected),
    [PORT_LAYOUT_STATS_IP] = OFF(LevelStats, ip),
    [PORT_LAYOUT_STATS_TC] = OFF(LevelStats, tc),
    [PORT_LAYOUT_STATS_BD] = OFF(LevelStats, bd),
    [PORT_LAYOUT_STATS_CR] = OFF(LevelStats, cr),
    [PORT_LAYOUT_STATS_COIN] = OFF(LevelStats, coin),
    [PORT_LAYOUT_STATS_RT] = OFF(LevelStats, rt),
    [PORT_LAYOUT_SCHED_UNK280] = OFF(Sched, unk280),
    [PORT_LAYOUT_SCHED_FRAMECOUNT] = OFF(Sched, frameCount),
    [PORT_LAYOUT_FRAME_SIZE] = sizeof(Frame),
};
/* host_game_types.h's structures are the game's own, field for field */
#define SAME(h, g, hf, gf) \
    _Static_assert(OFF(h, hf) == OFF(g, gf) && sizeof(((h *)0)->hf) == sizeof(((g *)0)->gf), #h "." #hf " is " #g "." #gf)
_Static_assert(sizeof(HostYoshiEntry) == sizeof(YoshiEntry), "HostYoshiEntry");
SAME(HostYoshiEntry, YoshiEntry, flags, flags);
SAME(HostYoshiEntry, YoshiEntry, x, x);
SAME(HostYoshiEntry, YoshiEntry, y, y);
SAME(HostYoshiEntry, YoshiEntry, w, unk6);
SAME(HostYoshiEntry, YoshiEntry, h, unk8);
SAME(HostYoshiEntry, YoshiEntry, text, text);
SAME(HostYoshiEntry, YoshiEntry, icon, unk14);
_Static_assert(sizeof(HostYoshiWindow) == sizeof(YoshiWindow), "HostYoshiWindow");
SAME(HostYoshiWindow, YoshiWindow, w, unk0);
SAME(HostYoshiWindow, YoshiWindow, h, unk2);
SAME(HostYoshiWindow, YoshiWindow, x, unk4);
SAME(HostYoshiWindow, YoshiWindow, y, unk6);
SAME(HostYoshiWindow, YoshiWindow, flags, unk8);
SAME(HostYoshiWindow, YoshiWindow, first, first);
SAME(HostYoshiWindow, YoshiWindow, count, count);
SAME(HostYoshiWindow, YoshiWindow, selected, unk18);
_Static_assert(sizeof(HostYoshiIcon) == sizeof(YoshiIcon), "HostYoshiIcon");
SAME(HostYoshiIcon, YoshiIcon, x, unk0);
SAME(HostYoshiIcon, YoshiIcon, y, unk2);
_Static_assert(sizeof(HostLevelInfo) == sizeof(LevelInfo), "HostLevelInfo");
SAME(HostLevelInfo, LevelInfo, flags, unk0);
SAME(HostLevelInfo, LevelInfo, gameState, gameState);
_Static_assert(sizeof(HostMapLevel) == sizeof(UnkStruct_8020D810), "HostMapLevel");
SAME(HostMapLevel, UnkStruct_8020D810, name, name);
SAME(HostMapLevel, UnkStruct_8020D810, next_b, unk18);
SAME(HostMapLevel, UnkStruct_8020D810, next_a, unk1C);
SAME(HostMapLevel, UnkStruct_8020D810, x, unk24);
SAME(HostMapLevel, UnkStruct_8020D810, y, unk28);
SAME(HostMapLevel, UnkStruct_8020D810, z, unk2C);
_Static_assert(sizeof(HostPlayerInfo) == sizeof(PlayerInfo), "HostPlayerInfo");
SAME(HostPlayerInfo, PlayerInfo, medal, medal);
SAME(HostPlayerInfo, PlayerInfo, routes, unk54);
SAME(HostPlayerInfo, PlayerInfo, gameState, gameState);
SAME(HostPlayerInfo, PlayerInfo, unkF0, unkF0);
_Static_assert(sizeof(HostMtx) == sizeof(Mtx), "HostMtx");
_Static_assert(sizeof(HostVtx) == sizeof(Vtx) && OFF(HostVtx, ob) == OFF(Vtx, v.ob) && OFF(HostVtx, tc) == OFF(Vtx, v.tc) &&
                   OFF(HostVtx, cn) == OFF(Vtx, v.cn) && OFF(HostVtx, n) == OFF(Vtx, n.n) && OFF(HostVtx, a) == OFF(Vtx, n.a),
               "HostVtx");
_Static_assert(sizeof(HostLight) == sizeof(Light_t) && OFF(HostLight, dir) == OFF(Light_t, dir), "HostLight");
_Static_assert(sizeof(HostViewport) == sizeof(Vp) && OFF(HostViewport, vtrans) == OFF(Vp, vp.vtrans), "HostViewport");

/* the host reads a display list's and an audio command's words as pointer-wide pairs, and every
   segmented address as tagged with bit 0 (host/gfx.h) */
_Static_assert(sizeof(HostGfx) == sizeof(Gfx) && OFF(Gfx, words.w0) == OFF(HostGfx, w0) && OFF(Gfx, words.w1) == OFF(HostGfx, w1) &&
                   sizeof(((Gfx *)0)->words.w0) == sizeof(uintptr_t),
               "Gfx: two pointer-wide words");
_Static_assert(sizeof(HostGfx) == sizeof(Acmd) && OFF(Acmd, words.w0) == OFF(HostGfx, w0) && OFF(Acmd, words.w1) == OFF(HostGfx, w1) &&
                   sizeof(((Acmd *)0)->words.w0) == sizeof(uintptr_t),
               "Acmd: two pointer-wide words");
_Static_assert(G_SEGTAG == 1u, "gfx.h's GFX_SEG_TAG");

u32 port_layout(unsigned id) {
    return id < PORT_LAYOUT_COUNT ? layout[id] : 0;
}
