/*
 * hd_front_end 11530 (the front end's screens), jp's func_801F9258, which
 * the C has as jp's GLOBAL_ASM, as native C charged by the original's
 * blocks (engine.h; jp_E7B0.c says more).
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

    ENGINE_BLK(801F9258);
    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(arg1));
    ENGINE_BLK(801F92BC);
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    ENGINE_BLK(801F92F0);
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
    ENGINE_BLK(801F9498);
    gfx = func_801F3450(gfx, arg1);
    ENGINE_BLK(801F94A8);
    func_80259450();
    ENGINE_BLK(801F94B0);
    if (D_8020D810[D_8021A908].unk8 != NULL) {
        ENGINE_BLK(801F94D8);
        size = 0x14;
    } else {
        ENGINE_BLK(801F94E4);
        size = 0x18;
    }
    ENGINE_BLK(801F94EC);
    h = D_8021AB2C / 9;
    func_80259DC8(arg1, D_8020D810[D_8021A908].name, D_8020D810[D_8021A908].unk8, 0, 0xA0, 0, (0x1C - h) / 2 + 0x12,
                  size, h, 1, 0xFF, 0xFF, 0xFF, D_8021AB2C, 0, 0, 0xFF, D_8021AB2C);
    ENGINE_BLK(801F959C);
    gfx = func_8024C404(gfx, arg1, &sp64);
    ENGINE_BLK(801F95AC);
    func_80259C24(&gfx, arg1);
    ENGINE_BLK(801F95BC);
    level = &D_802E8F94[D_8021A908];
    gfx = func_80274868(gfx);
    ENGINE_BLK(801F95E8);
    y = 0xDA;
    i = 0;
    odd = 0;
    do {
        ENGINE_BLK(801F95FC);
        if (!(level->unk2C & (1 << i)))
            goto next;
        ENGINE_BLK(801F961C);
        if (level->unk0 != 1) {
            ENGINE_BLK(801F962C);
            if (!(D_8021AB30->unk10 & (1 << i)))
                goto next;
        }
        ENGINE_BLK(801F964C);
        if (D_8020E350[i * 2] == 0)
            goto next;
        ENGINE_BLK(801F966C);
        if (odd != 0) {
            ENGINE_BLK(801F9678);
            gfx = func_80272ED8(gfx, D_8021A8F0 + i, 0x16 - (0xFF - D_8021AB2C) / 6, y, D_8021AB2C, 0, 0.8125f);
            ENGINE_BLK(801F96C8);
        } else {
            ENGINE_BLK(801F96D0);
            y -= 0x2C;
            gfx = func_80272ED8(gfx, D_8021A8F0 + i, (0xFF - D_8021AB2C) / 6 + 0xF6, y, D_8021AB2C, 0, 0.8125f);
            ENGINE_BLK(801F9730);
        }
        ENGINE_BLK(801F9734);
        odd ^= 1;
    next:
        ENGINE_BLK(801F9740);
        i++;
        if (i >= 0x13)
            break;
        ENGINE_BLK(801F9754);
    } while (y >= 0x29);
    ENGINE_BLK(801F9764);
    func_8025B498(0xA0, size, D_8020D810[D_8021A908].name, D_8020D810[D_8021A908].unk8);
    ENGINE_BLK(801F9798);
    if (D_8020D810[D_8021A908].unk8 != NULL) {
        ENGINE_BLK(801F97C4);
        len = func_8025B370(D_8020D810[D_8021A908].unk8);
        ENGINE_BLK(801F97CC);
        (void)(len * engine_trunc_w_s((f32)size * D_802E8C84[1]));
    } else {
        ENGINE_BLK(801F9808);
        len = func_8025B300(D_8020D810[D_8021A908].name);
        ENGINE_BLK(801F982C);
        (void)(len * engine_trunc_w_s((f32)size * D_802E8C84[0]));
    }
    ENGINE_BLK(801F9864);
    gfx = func_80274AA4(gfx);
    ENGINE_BLK(801F986C);
    gSPEndDisplayList(gfx++);
    *arg2 = gfx - arg0;
    return gfx;
}

#endif
