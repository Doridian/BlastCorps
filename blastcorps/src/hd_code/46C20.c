#include "common.h"
#include "game/game.h"

extern u32 D_803FFFF8;
extern u32 D_803FFFFC;
extern OSMesgQueue D_803150A0;

void func_801F57B0(void);
void func_8029A7E4(char *, ...);
void func_8025C230(u8 **, u8 **, void *);
void func_802C4070(u8 **, u8 **, void *, u8);
void func_8028B4C4(u32 arg0, u8 *arg1, u32 *arg2, u8 arg3, u8 arg4, u8 arg5);

/* .bss, 0x80370C50-0x80370C70 (tools/bss_c.py) */
u8 D_80370C50;
OSIoMesg D_80370C58;

/* .data, 0x802FDB30-0x802FDB40 (tools/data_c.py) */
u32 *D_802FDB30 = &D_803FFFF8;
u32 *D_802FDB34 = &D_803FFFFC;

void func_8028B3E0(void) {
    u32 sp24;

    sp24 = *D_802FDB34 - *D_802FDB30;
    osViBlack(1);
    if (D_80370C50 == 0) {
        osInvalDCache((void *) 0x801E7000, 0x37D00);
        osInvalICache((void *) 0x801E7000, 0x37D00);
        func_8028B4C4(*D_802FDB30, (u8 *) 0x801E7000, &sp24, 13, 10, 1);
        bzero((u8 *) 0x801E7000 + sp24, 0x37D00 - sp24);
        D_80370C50 = 1;
        func_801F57B0();
        func_8029A7E4("got front end\n");
    }
}

void func_8028B4C4(u32 arg0, u8 *arg1, u32 *arg2, u8 arg3, u8 arg4, u8 arg5) {
    u32 sp44;
    u32 sp40;
    u8 *sp3C;
    u8 *sp38;
    u8 *sp34;

    sp38 = arg1;
    if (arg3 != 0 || arg4 != 0) {
        sp3C = (u8 *) 0x8021ED00;
    } else {
        sp3C = arg1;
    }
    osInvalDCache(sp3C, *arg2);
    for (sp44 = 0, sp34 = sp3C; sp44 < *arg2 / 0x4000; sp44++) {
        osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, arg0, sp34, 0x4000, &D_803150A0);
        osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
        arg0 += 0x4000;
        sp34 += 0x4000;
    }
    if (sp40 = *arg2 - (*arg2 / 0x4000) * 0x4000) {
        osPiStartDma(&D_80370C58, OS_MESG_PRI_NORMAL, OS_READ, arg0, sp34, sp40, &D_803150A0);
        osRecvMesg(&D_803150A0, NULL, OS_MESG_BLOCK);
    }
    switch (arg5) {
        case 1:
            if (arg3 != 0) {
                func_8025C230(&sp3C, &arg1, (void *) 0x8004B400);
            }
            if (arg4 != 0) {
                func_8025C230(&sp3C, &arg1, (void *) 0x8004B400);
            }
            break;
        case 2:
            if (arg3 != 0) {
                func_802C4070(&sp3C, &arg1, (void *) 0x8004B400, arg3);
            }
            if (arg4 != 0) {
                func_802C4070(&sp3C, &arg1, (void *) 0x8004B400, arg4);
            }
            break;
    }
    if (arg3 != 0 || arg4 != 0) {
        *arg2 = arg1 - sp38;
    }
}
