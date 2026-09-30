#include "common.h"
#include "game/frame.h"
#include "game/game.h"

/* A loaded asset's header: the fields are offsets from its start. */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ u8 unk8[0xC];
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
} UnkStruct_801F4E70;

void func_802A0700(void);
void func_802A08B4(void *, void *);
void func_8028B4C4(u32 arg0, u8 *arg1, u32 *arg2, u8 arg3, u8 arg4, u8 arg5);
void func_80202100(s32, void *, void *, void *);
void func_80202270(UnkStruct_801F4E70 *, void *, void *);
void func_802021FC(void *, void *, void *);
void func_802022EC(void *, s32, s32, s32, f32, s32, s32);

extern u8 D_006E8980[];
extern u8 D_006EA850[];
extern u8 D_006EAB90[];
extern u8 D_006EC4C0[];
extern u16 D_8035807C;
extern s16 D_8036BB20;
extern FrameBuf D_803156F8[];

/* .bss, 0x802182C0-0x80218740 (tools/bss_c.py) */
u32 D_802182C0;
u32 D_802182C4;
void *D_802182C8;
void *D_802182CC;
Mtx D_802182D0[2];
UnkStruct_801F4E70 *PTR32 D_80218350;
u8 *PTR32 D_80218358[2];
Gfx *PTR32 D_80218360[4];
Mtx D_80218370;
Mtx D_802183B0;
Mtx D_802183F0;
u8 D_80218430[0x300];
u32 D_80218730;
u16 D_80218734;

void func_801F4E70(u8 arg0) {
    UnkStruct_801F4E70 *sp34;
    void *sp30;

    func_802A0700();
    switch (arg0) {
        case 0:
            D_802182C4 = (u32)D_006E8980;
            D_802182C0 = D_006EA850 - D_006E8980;
            break;
        case 1:
            D_802182C4 = (u32)D_006EA850;
            D_802182C0 = D_006EAB90 - D_006EA850;
            break;
        case 2:
            D_802182C4 = (u32)D_006EAB90;
            D_802182C0 = D_006EC4C0 - D_006EAB90;
            break;
    }
    func_8028B4C4(D_802182C4, D_80358070, &D_802182C0, 0xC, 0xA, 1);
    sp34 = (UnkStruct_801F4E70 *)D_80358070;
    D_80358070 += D_802182C0;
    D_802182C8 = (void *)(sp34->unk1C + (u32)sp34);
    sp30 = (void *)(sp34->unk20 + (u32)sp34);
    D_802182CC = (void *)(sp34->unk14 + (u32)sp34);
    func_802A08B4(D_802182C8, sp30);
}

Gfx *func_801F4FBC(u8 *arg0, Gfx *arg1) {
    Gfx *gfx = arg1;

    gSPSegment(gfx++, 6, D_802182CC);
    gSPSegment(gfx++, 7, &D_802182D0[D_8035805C]);
    gSPPerspNormalize(gfx++, D_8035807C);
    gSPLookAt(gfx++, arg0 + 0x3C00);
    gSPMatrix(gfx++, arg0 + 0x1240, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg0 + 0x140, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg0 + 0x12C0, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg0 + 0x1280, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg0 + 0x1300, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    gSPDisplayList(gfx++, D_802182C8);
    return gfx;
}

Gfx *func_801F51C8(u8 *arg0, Gfx *arg1) {
    Gfx *gfx = arg1;
    void *sp68;

    gSPPerspNormalize(gfx++, D_80218734);
    gSPLookAt(gfx++, arg0 + 0x3C00);
    gSPMatrix(gfx++, arg0 + 0x240, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, arg0 + 0x140, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gDPSetEnvColor(gfx++, 0, 0, 0, D_8036BB20);
    {
        s32 sp4C;
        Mtx *sp48;

        sp4C = ((UnkStruct_801F4E70 *)(D_80218350->unk18 + (u32)D_80218350))->unk4;
        sp48 = (Mtx *)(D_80218358[D_8035805C] + sp4C);
        func_802021FC(D_80218430, D_80218358[D_8035805C], D_80218358[D_8035805C ^ 1]);
        D_80218730 += 3;
        guRotate(sp48, D_80218730 % 360, 0.0f, 1.0f, 0.0f);
        osWritebackDCache(sp48, sizeof(Mtx));
    }
    sp68 = (void *)(D_80218350->unk14 + (u32)D_80218350);
    gSPSegment(gfx++, 6, sp68);
    gSPSegment(gfx++, 7, D_80218358[D_8035805C]);
    gSPMatrix(gfx++, &D_802183F0, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_80218370, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPMatrix(gfx++, &D_802183B0, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(gfx++, D_80218360[D_8035805C]);
    gSPMatrix(gfx++, &D_802183F0, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gfx++, &D_80218370, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPMatrix(gfx++, &D_802183B0, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(gfx++, D_80218360[D_8035805C + 2]);
    return gfx;
}

void func_801F55D8(void) {
    s32 sp3C;
    u8 *sp38;

    func_80202100(0x96, &D_80218350, D_80218358, D_80218360);
    guTranslate(&D_80218370, 150.0f, -45.0f, 0.0f);
    guScale(&D_802183B0, 1.5f, 1.5f, 1.5f);
    func_80202270(D_80218350, D_80218358, D_80218430);
    func_802022EC(D_80218430, 1, 0, 0, 0.0f, 2, 0);
    func_802022EC(D_80218430, 2, 0, 0, 0.0f, 1, 1);
    guRotate(&D_802183F0, 20.0f, 1.0f, 0.0f, 0.0f);
    for (sp3C = 0; sp3C < 2; sp3C++) {
        sp38 = (u8 *)&D_803156F8[sp3C];
        guPerspective((Mtx *)(sp38 + 0x240), &D_80218734, 45.0f, 4.0f / 3.0f, 40.0f, 4000.0f, 1.0f);
        guLookAtReflect((Mtx *)(sp38 + 0x140), (LookAt *)(sp38 + 0x3C00), 0.0f, 1.0f, 400.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                        1.0f, 0.0f);
    }
}
