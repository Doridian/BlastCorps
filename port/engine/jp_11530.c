/*
 * hd_front_end 11530 (the front end's screens), jp's func_801F9258, which
 * the C has as jp's GLOBAL_ASM, as native C (engine.h; jp_E7B0.c says more).
 *
 * jp's titles a level with its u16 name where it has one, at width 0x14
 * (0x18 for the char name), and measures the name it showed.
 */
#include "engine.h"
#include "game/game.h"
#include "game/level.h"
#include "game/player.h"

#ifdef VERSION_JP

extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern u16 D_8020E350[];
extern u8 D_8021A8F0;
extern u8 D_8021A908;
extern s16 D_8021AB2C;
extern PlayerInfo *D_8021AB30;
extern f32 D_802E8C84[2];

/* the level screen's display list: the level's name, its medals */
Gfx *func_801F9258(Gfx *arg0, u8 *arg1, s32 *arg2) {
    Gfx *gfx = arg0;
    s32 i, odd, y, size, len;
    u32 h;
    s32 sp64;
    LevelInfo *level;

    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(arg1));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000038);
    gSPDisplayList(gfx++, D_01000010);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gDPSetFillColor(gfx++, 0x10001);
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    gDPPipeSync(gfx++);
    gDPPipelineMode(gfx++, G_PM_1PRIMITIVE);
    gDPSetColorDither(gfx++, G_CD_NOISE);
    gfx = func_801FE5D0(gfx, arg1);
    gfx = func_801F3450(gfx, arg1);
    func_80259450();
    if (D_8020D810[D_8021A908].unk8 != NULL) {
        size = 0x14;
    } else {
        size = 0x18;
    }
    h = D_8021AB2C / 9;
    func_80259DC8(arg1, D_8020D810[D_8021A908].name, D_8020D810[D_8021A908].unk8, 0, 0xA0, 0, (0x1C - h) / 2 + 0x12,
                  size, h, 1, 0xFF, 0xFF, 0xFF, D_8021AB2C, 0, 0, 0xFF, D_8021AB2C);
    gfx = func_8024C404(gfx, arg1, &sp64);
    func_80259C24(&gfx, arg1);
    level = &D_802E8F94[D_8021A908];
    gfx = func_80274868(gfx);
    y = 0xDA;
    i = 0;
    odd = 0;
    do {
        if (!(level->unk2C & (1 << i)))
            goto next;
        if (level->unk0 != 1) {
            if (!(D_8021AB30->unk10 & (1 << i)))
                goto next;
        }
        if (D_8020E350[i * 2] == 0)
            goto next;
        if (odd != 0) {
            gfx = func_80272ED8(gfx, D_8021A8F0 + i, 0x16 - (0xFF - D_8021AB2C) / 6, y, D_8021AB2C, 0, 0.8125f);
        } else {
            y -= 0x2C;
            gfx = func_80272ED8(gfx, D_8021A8F0 + i, (0xFF - D_8021AB2C) / 6 + 0xF6, y, D_8021AB2C, 0, 0.8125f);
        }
        odd ^= 1;
    next:
        i++;
        if (i >= 0x13)
            break;
    } while (y >= 0x29);
    func_8025B498(0xA0, size, D_8020D810[D_8021A908].name, D_8020D810[D_8021A908].unk8);
    if (D_8020D810[D_8021A908].unk8 != NULL) {
        len = func_8025B370(D_8020D810[D_8021A908].unk8);
        (void)(len * engine_trunc_w_s((f32)size * D_802E8C84[1]));
    } else {
        len = func_8025B300(D_8020D810[D_8021A908].name);
        (void)(len * engine_trunc_w_s((f32)size * D_802E8C84[0]));
    }
    gfx = func_80274AA4(gfx);
    gSPEndDisplayList(gfx++);
    *arg2 = gfx - arg0;
    return gfx;
}

#endif
