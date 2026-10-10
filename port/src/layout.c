/*
 * The game's layouts as the host reads them (port.h, "the game's layouts"):
 * sizeof and offsetof of what port/host/digest.c, sched_vars.h and gfx.c
 * read inside the game's structures, by this build's N64-side compiler.
 * The N64's offsets in the 32-bit and 64-bit builds; in the LP64 ones a
 * structure whose pointers aren't PTR32 is the host's layout, and the
 * table follows it, so the host never spells an offset itself.
 */
#include "common.h"
#include "port.h"
#include "game/vehicle.h"
#include "game/objects.h"
#include "game/level.h"
#include "game/player.h"
#include "game/sched.h"
#include "game/game.h"

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
    [PORT_LAYOUT_BUILDING_DAMAGE] = OFF(Building, unkEC),
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
    [PORT_LAYOUT_PLAYER_SIZE] = sizeof(PlayerInfo),
    [PORT_LAYOUT_PLAYER_UNITS] = OFF(PlayerInfo, units),
    [PORT_LAYOUT_PLAYER_MEDAL] = OFF(PlayerInfo, medal),
    [PORT_LAYOUT_PLAYER_GAMESTATE] = OFF(PlayerInfo, gameState),
    [PORT_LAYOUT_STATS_IP] = OFF(LevelStats, ip),
    [PORT_LAYOUT_STATS_TC] = OFF(LevelStats, tc),
    [PORT_LAYOUT_STATS_BD] = OFF(LevelStats, bd),
    [PORT_LAYOUT_STATS_CR] = OFF(LevelStats, cr),
    [PORT_LAYOUT_STATS_COIN] = OFF(LevelStats, coin),
    [PORT_LAYOUT_STATS_RT] = OFF(LevelStats, rt),
    [PORT_LAYOUT_SCHED_UNK280] = OFF(Sched, unk280),
    [PORT_LAYOUT_SCHED_FRAMECOUNT] = OFF(Sched, frameCount),
    [PORT_LAYOUT_PTR_D_803649D0] = sizeof(D_803649D0),
    [PORT_LAYOUT_PTR_D_803F7654] = sizeof(D_803F7654),
    [PORT_LAYOUT_PTR_D_8036BED8] = sizeof(D_8036BED8),
    [PORT_LAYOUT_PTR_D_80365348] = sizeof(D_80365348[0]),
};

u32 port_layout(unsigned id) {
    return id < PORT_LAYOUT_COUNT ? layout[id] : 0;
}
