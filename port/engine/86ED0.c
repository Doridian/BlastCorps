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

    if (D_80364A98 != 0x40) {
        func_80260650(D_80367738, 0x55, NULL);
    }
    v = D_80364A72;
    vs->unk76 = v + (v >> 1);
    D_80367BFF = 0;
    func_80278EB0(6, 0.25f, 100);
    D_80367C00 = 1;
}
