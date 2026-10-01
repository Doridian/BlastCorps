/*
 * hd_code 86ED0 (us.v11 0x802CB690-0x802CB720): what the vehicle modules
 * call as the player gets in, as native C (engine.h).
 */
#include "shared.h"
#include "game/game.h"
#include "game/level.h"
#include "game/audio.h"

extern s16 D_80364A72;

SndState *func_80260650(SndBank *bank, s16 id, SndState *PTR32 *handle);
void func_80278EB0(s32 a, f32 b, s32 c);

/* unless the level is over (D_80364A98 0x40): sound 0x55; then the speed
   is D_80364A72 * 1.5, and func_80278EB0(6, 0.25, 100) */
REGS(gp)
void func_802CB690(VS *vs) {
    s32 v;

    ENGINE_BLK(802CB690);
    if (D_80364A98 != 0x40) {
        ENGINE_BLK(802CB6AC);
        func_80260650(D_80367738, 0x55, NULL);
    }
    ENGINE_BLK(802CB6C0);
    v = D_80364A72;
    vs->unk76 = v + (v >> 1);
    D_80367BFF = 0;
    func_80278EB0(6, 0.25f, 100);
    ENGINE_BLK(802CB6F8);
    D_80367C00 = 1;
    /* what the original leaves in $a0-$a2 */
    ENGINE_LEAVE(4, 1);
    ENGINE_LEAVE(5, 0x3E800000);
    ENGINE_LEAVE(6, 100);
}
