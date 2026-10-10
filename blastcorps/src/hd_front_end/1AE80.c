#include "common.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

extern OSMesgQueue D_80219EF8;
extern OSMesgQueue D_80219F50;
extern u8 D_8039C4B8[0x40];

s32 func_80201E80(void) {
    u8 i;
    s32 ret;

    ret = 0;
    func_8029A7E4("restoring from EE memory\n");
    for (i = 0; i < 60 && ret == 0; i++) {
        if (!(i & 1)) {
            osSendMesg(&D_80219EF8, (OSMesg)((i << 8) | 0xA | (D_80364AE8 << 16) | 0x01000000), OS_MESG_BLOCK);
            osRecvMesgInt(&D_80219F50, ret, OS_MESG_BLOCK);
        }
        if (((D_80364AF0[D_80364AE8].medal[i] > 0 && D_80364AF0[D_80364AE8].medal[i] < 6) ? TRUE : FALSE) &&
            !DUMMY_LEVELS(i) && ret == 0) {
            if (D_802E8F94[i].unk0 == 1) {
                *(u64 *)D_8039C4B8 = 0x1234567887654321;
                osSendMesg(&D_80219EF8, (OSMesg)((i << 8) | 0xD | (D_80364AE8 << 16) | 0x01000000),
                           OS_MESG_BLOCK);
                osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
            }
            func_801F8354(D_80364AE8);
            D_80364EF0[D_80364AE8][D_802E8C44[D_80364AF0[D_80364AE8].unk92[i]]] = D_80364F70[i * 2];
            osSendMesg(&D_80219EF8, (OSMesg)((i << 8) | 9 | (D_80364AE8 << 16) | 0x01000000), OS_MESG_BLOCK);
            osRecvMesg(&D_80219F50, NULL, OS_MESG_BLOCK);
        }
    }
    return ret;
}

