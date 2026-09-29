#include "common.h"
#include "game/frame.h"
#include "game/game.h"

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
} UnkStruct_8036C7A0; /* size = 0x6 */

void func_80276D1C(Mtx *arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 *arg5, f32 *arg6, f32 *arg7, f32 *arg8);

/* .bss, 0x8036C790-0x8036C8D0 (tools/bss_c.py) */
UnkStruct_8036C7A0 *D_8036C790;
UnkStruct_8036C7A0 *D_8036C794;
s32 D_8036C798;
UnkStruct_8036C7A0 *D_8036C7A0[10];
s32 D_8036C7C8;
u8 D_8036C7CC;
Vtx D_8036C7D0[1][4];
u8 D_8036C810[0x40];
Mtx D_8036C850[2];

/* .data, 0x802FA940-0x802FAD50 (tools/data_c.py) */
extern u8 D_802FA940[0x400];
#ifdef ASSET_INIT
#include "hd_code/30C70/D_802FA940.inc"
#else
u8 D_802FA940[0x400] = { 0 };
#endif
s32 D_802FAD40 = 0;
s32 D_802FAD44 = 0;
u8 D_802FAD48 = 0;

void func_8027690C(FrameBuf *arg0, f32 x, f32 y, f32 z, s16 *outX, s16 *outY, Mtx *arg6, Mtx *arg7,
                   Mtx *arg8, f32 arg9);


s32 func_80276130(FrameBuf *arg0, u8 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, u8 arg7,
                  u8 arg8, u8 arg9, u8 arg10, u8 arg11, u8 arg12, u8 arg13, u8 arg14, u8 arg15, u8 arg16, u8 arg17,
                  u8 arg18, u8 arg19, u8 arg20, u8 arg21, u8 arg22);

s32 func_8026AD30(s32);
void func_8026AF6C(s32);
s32 func_802BCE40(void);
void func_802BD10C(s32);
Gfx *func_80275DA4(Gfx *arg0, u8 arg1);
s32 func_80276080(FrameBuf *arg0, u8 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, u8 arg7,
                  u8 arg8, u8 arg9, u8 arg10);
void func_8027656C(FrameBuf *arg0);
s32 func_802768A8(void);
void func_80277EDC();

extern FrameBuf D_02000000;
extern u16 D_8035807C;
extern u8 D_803643DB;
extern s16 D_8036443E;
extern s32 D_80364AA8;
extern UnkStruct_8036C7A0 *D_8036C790;
extern UnkStruct_8036C7A0 *D_8036C794;
extern UnkStruct_8036C7A0 *D_8036C7A0[10];
extern s32 D_8036C798;
extern s32 D_8036C7C8;
extern u8 D_8036EB98;
extern u8 D_803F7808;
extern u8 D_803F7809;
extern u8 D_8036C7CC;
extern Vtx D_8036C7D0[][4];
extern Mtx D_8036C850[];

void func_80275430(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        D_8036C7A0[i] = 0;
    }
    D_8036C794 = 0;
    D_8036C7CC = 0;
}

void func_80275478(FrameBuf *arg0, Gfx **arg1, u8 arg2) {
    s16 i;
    s16 j;
    s16 vtxIdx;
    Gfx *gfx;
    s16 sx;
    s16 sy;
    s16 px;
    s16 py;
    s16 minX;
    s16 maxX;
    s16 minY;
    s16 maxY;
    u8 r;
    u8 g;
    s16 t;
    UnkStruct_8036C7A0 *p;
    s32 sp50;

    vtxIdx = 16;
    gfx = *arg1;
    minX = 0x7FFF, maxX = -0x8000, minY = 0x7FFF, maxY = -0x8000;
    sp50 = func_802BCE40();
    D_8036C7CC = 0;
    p = D_8036C790;
    if (D_8036EB98 == 0 && sp50 == 0 && D_80364AA8 == 1 && D_802E8BD0 == 0) {
        if (func_8026AD30(0x4B) == 0) {
            func_8026AF6C(0x803D);
            func_80277EDC(3, 1, 2, 0x82);
        }
        D_8036EB98 = 1;
    }
    if (D_8036C790 != NULL && (func_802768A8() == 0 || arg2 != 0) && D_8036C794 == NULL) {
        for (i = 0; i < 4; i++) {
            func_8027690C(arg0, D_8036C790->unk0, D_8036C790->unk2, D_8036C790->unk4, &sx, &sy, NULL, NULL, NULL, 1.0f);
            D_8036C790++;
            if (sx < minX) {
                minX = sx;
            }
            if (sx > maxX) {
                maxX = sx;
            }
            if (sy < minY) {
                minY = sy;
            }
            if (sy > maxY) {
                maxY = sy;
            }
        }
        D_8036C7CC = 0;
        for (j = 0; j < 4; j++) {
            switch (j) {
                case 0:
                    px = (maxX - minX) / 2 + minX;
                    py = minY;
                    break;
                case 1:
                    px = (maxX - minX) / 2 + minX;
                    py = maxY;
                    break;
                case 2:
                    px = minX;
                    py = (maxY - minY) / 2 + minY;
                    break;
                case 3:
                    px = maxX;
                    py = (maxY - minY) / 2 + minY;
                    break;
            }
            if (px < 310 && px >= 11 && py < 230 && py >= 11) {
                D_8036C794 = p;
                D_8036C798 = sp50;
                D_803F7809 = D_803F7808;
                D_802FAD44 = 0;
            }
        }
    }
    if (D_8036C794 != NULL) {
        func_802BD10C(D_8036C798);
        if (D_8036C7C8 > 1500 || D_803643DB == 0) {
            g = 255;
            r = 0;
        } else if (D_8036C7C8 < 500) {
            r = 255;
            g = 0;
        } else {
            t = (D_8036C7C8 - 500) / 1000.0f * 511.0f;
            if (t < 256) {
                g = t, r = 255;
            } else {
                g = 255, r = 510 - t;
            }
        }
        p = D_8036C794;
        for (i = 0; i < 4; i++) {
            func_8027690C(arg0, p->unk0, p->unk2, p->unk4, &sx, &sy, NULL, NULL, NULL, 1.0f);
            p++;
            if (sx < minX) {
                minX = sx;
            }
            if (sx > maxX) {
                maxX = sx;
            }
            if (sy < minY) {
                minY = sy;
            }
            if (sy > maxY) {
                maxY = sy;
            }
        }
        for (j = 0; j < 4; j++) {
            switch (j) {
                case 0:
                    px = (maxX - minX) / 2 + minX, py = minY - D_802FAD40;
                    break;
                case 1:
                    px = (maxX - minX) / 2 + minX, py = maxY + D_802FAD40;
                    break;
                case 2:
                    px = minX - D_802FAD40;
                    py = (maxY - minY) / 2 + minY;
                    break;
                case 3:
                    px = maxX + D_802FAD40;
                    py = (maxY - minY) / 2 + minY;
                    break;
            }
            if (px < 310 && px >= 11 && py < 230 && py >= 11) {
                D_8036C7CC++;
            }
            vtxIdx = func_80276080(arg0, j, vtxIdx, px, py, 8, 8, r, g, 0, 255);
        }
        if (D_802FAD48 == 0) {
            if (++D_802FAD40 == 10) {
                D_802FAD48 = 1;
                D_802FAD44++;
            }
        } else {
            if (--D_802FAD40 == 0) {
                D_802FAD48 = 0;
                D_802FAD44++;
            }
        }
        if (D_802FAD44 == 5) {
            D_8036C794 = NULL;
        }
        gfx = func_80275DA4(gfx, 0);
        gSPVertex(gfx++, &D_02000000.vtx[16], 16, 0);
        vtxIdx = 0;
        for (j = 0; j < 4; j++) {
            gSP1Triangle(gfx++, vtxIdx, vtxIdx + 1, vtxIdx + 2, 0);
            gSP1Triangle(gfx++, vtxIdx, vtxIdx + 2, vtxIdx + 3, 0);
            vtxIdx += 4;
        }
    }
    func_8027656C(arg0);
    *arg1 = gfx;
}

Gfx *func_80275DA4(Gfx *arg0, u8 arg1) {
    Gfx *gfx = arg0;

    if (!arg1) {
        gSPMatrix(gfx++, &D_02000000.mtx[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(gfx++, &D_02000000.mtx[7], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    }
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_802FA940), G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0, G_TX_CLAMP,
                        G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    return gfx;
}

s32 func_80276080(FrameBuf *arg0, u8 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, u8 arg7,
                  u8 arg8, u8 arg9, u8 arg10) {
    return func_80276130(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg7, arg8, arg9, arg10, arg7,
                  arg8, arg9, arg10, arg7, arg8, arg9, arg10);
}

s32 func_80276130(FrameBuf *arg0, u8 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, u8 arg7,
                  u8 arg8, u8 arg9, u8 arg10, u8 arg11, u8 arg12, u8 arg13, u8 arg14, u8 arg15, u8 arg16, u8 arg17,
                  u8 arg18, u8 arg19, u8 arg20, u8 arg21, u8 arg22) {
    s32 sp4;

    switch (arg1) {
        case 0:
            arg0->vtx[arg2].v.tc[0] = 0, arg0->vtx[arg2].v.tc[1] = 0;
            arg0->vtx[arg2 + 1].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 1].v.tc[1] = 0;
            arg0->vtx[arg2 + 2].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 2].v.tc[1] = 0x3E0;
            arg0->vtx[arg2 + 3].v.tc[0] = 0, arg0->vtx[arg2 + 3].v.tc[1] = 0x3E0;
            break;
        case 1:
            arg0->vtx[arg2].v.tc[0] = 0, arg0->vtx[arg2].v.tc[1] = 0x3E0;
            arg0->vtx[arg2 + 1].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 1].v.tc[1] = 0x3E0;
            arg0->vtx[arg2 + 2].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 2].v.tc[1] = 0;
            arg0->vtx[arg2 + 3].v.tc[0] = 0, arg0->vtx[arg2 + 3].v.tc[1] = 0;
            break;
        case 2:
            arg0->vtx[arg2].v.tc[0] = 0, arg0->vtx[arg2].v.tc[1] = 0;
            arg0->vtx[arg2 + 1].v.tc[0] = 0, arg0->vtx[arg2 + 1].v.tc[1] = 0x3E0;
            arg0->vtx[arg2 + 2].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 2].v.tc[1] = 0x3E0;
            arg0->vtx[arg2 + 3].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 3].v.tc[1] = 0;
            break;
        case 3:
            arg0->vtx[arg2].v.tc[0] = 0, arg0->vtx[arg2].v.tc[1] = 0x3E0;
            arg0->vtx[arg2 + 1].v.tc[0] = 0, arg0->vtx[arg2 + 1].v.tc[1] = 0;
            arg0->vtx[arg2 + 2].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 2].v.tc[1] = 0;
            arg0->vtx[arg2 + 3].v.tc[0] = 0x3E0, arg0->vtx[arg2 + 3].v.tc[1] = 0x3E0;
            break;
    }
    arg0->vtx[arg2].v.ob[0] = arg3 - arg5;
    arg0->vtx[arg2].v.ob[1] = arg4 - arg6;
    arg0->vtx[arg2].v.ob[2] = -10;
    arg0->vtx[arg2].v.flag = 0;
    arg0->vtx[arg2].v.cn[0] = arg7;
    arg0->vtx[arg2].v.cn[1] = arg8;
    arg0->vtx[arg2].v.cn[2] = arg9;
    arg0->vtx[arg2].v.cn[3] = arg10;
    arg2++;
    arg0->vtx[arg2].v.ob[0] = arg3 + arg5;
    arg0->vtx[arg2].v.ob[1] = arg4 - arg6;
    arg0->vtx[arg2].v.ob[2] = -10;
    arg0->vtx[arg2].v.flag = 0;
    arg0->vtx[arg2].v.cn[0] = arg11;
    arg0->vtx[arg2].v.cn[1] = arg12;
    arg0->vtx[arg2].v.cn[2] = arg13;
    arg0->vtx[arg2].v.cn[3] = arg14;
    arg2++;
    arg0->vtx[arg2].v.ob[0] = arg3 + arg5;
    arg0->vtx[arg2].v.ob[1] = arg4 + arg6;
    arg0->vtx[arg2].v.ob[2] = -10;
    arg0->vtx[arg2].v.flag = 0;
    arg0->vtx[arg2].v.cn[0] = arg15;
    arg0->vtx[arg2].v.cn[1] = arg16;
    arg0->vtx[arg2].v.cn[2] = arg17;
    arg0->vtx[arg2].v.cn[3] = arg18;
    arg2++;
    arg0->vtx[arg2].v.ob[0] = arg3 - arg5;
    arg0->vtx[arg2].v.ob[1] = arg4 + arg6;
    arg0->vtx[arg2].v.ob[2] = -10;
    arg0->vtx[arg2].v.flag = 0;
    arg0->vtx[arg2].v.cn[0] = arg19;
    arg0->vtx[arg2].v.cn[1] = arg20;
    arg0->vtx[arg2].v.cn[2] = arg21;
    arg0->vtx[arg2].v.cn[3] = arg22;
    arg2++;
    return arg2;
}

void func_8027656C(FrameBuf *arg0) {
    s32 i;
    s32 j;
    UnkStruct_8036C7A0 *p;
    s16 sx;
    s16 sy;
    s16 px;
    s16 py;
    s16 minX;
    s16 maxX;
    s16 minY;
    s16 maxY;
    u8 visible;

    for (i = 0; i < 10; i++) {
        if (D_8036C7A0[i] != NULL) {
            minX = 0x7FFF, maxX = -0x8000;
            minY = 0x7FFF, maxY = -0x8000;
            p = D_8036C7A0[i];
            for (j = 0; j < 4; j++) {
                func_8027690C(arg0, p->unk0, p->unk2, p->unk4, &sx, &sy, NULL, NULL, NULL, 1.0f);
                p++;
                if (sx < minX) {
                    minX = sx;
                }
                if (sx > maxX) {
                    maxX = sx;
                }
                if (sy < minY) {
                    minY = sy;
                }
                if (sy > maxY) {
                    maxY = sy;
                }
            }
            j = 0;
            visible = FALSE;
            while (j < 4 && !visible) {
                switch (j) {
                    case 0:
                        px = (maxX - minX) / 2 + minX;
                        py = minY;
                        break;
                    case 1:
                        px = (maxX - minX) / 2 + minX;
                        py = maxY;
                        break;
                    case 2:
                        px = minX;
                        py = (maxY - minY) / 2 + minY;
                        break;
                    case 3:
                        px = maxX;
                        py = (maxY - minY) / 2 + minY;
                        break;
                }
                if (px < 310 && px >= 11 && py < 230 && py >= 11) {
                    visible = TRUE;
                }
                j++;
            }
            if (!visible) {
                D_8036C7A0[i] = NULL;
            }
        }
    }
}

void func_8027684C(void) {
    s32 i;

    i = 0;
    while (i < 10) {
        if (D_8036C7A0[i] == NULL) {
            D_8036C7A0[i] = D_8036C794;
            return;
        }
        i++;
    }
}

s32 func_802768A8(void) {
    s32 i;

    i = 0;
    while (i < 10) {
        if (D_8036C7A0[i] != NULL && D_8036C7A0[i] == D_8036C790) {
            return 1;
        }
        i++;
    }
    return 0;
}

void func_8027690C(FrameBuf *arg0, f32 x, f32 y, f32 z, s16 *outX, s16 *outY, Mtx *arg6, Mtx *arg7,
                   Mtx *arg8, f32 arg9) {
    f32 w;

    w = 1.0f;
    if (arg8 != NULL) {
        func_80276D1C(arg8, x, y, z, w, &x, &y, &z, &w);
    }
    if (arg7 != NULL) {
        func_80276D1C(arg7, x, y, z, w, &x, &y, &z, &w);
    }
    if (arg6 != NULL) {
        func_80276D1C(arg6, x, y, z, w, &x, &y, &z, &w);
    }
    func_80276D1C(&arg0->mtx[5], x, y, z, w, &x, &y, &z, &w);
    if (z >= 0.0) {
        *outX = 0x4000;
        *outY = 0x4000;
        return;
    }
    func_80276D1C(&arg0->mtx[2], x, y, z, w, &x, &y, &z, &w);
    x = x * ((u32)D_8035807C / 65535.0);
    y = y * ((u32)D_8035807C / 65535.0);
    w = w * ((u32)D_8035807C / 65535.0);
    x = x / w;
    y = y / w;
    x = ((320.0f * arg9) / 2.0f) * x;
    y = ((240.0f * arg9) / 2.0f) * y;
    x = ((320.0f * arg9) / 2.0f) + x;
    y = ((240.0f * arg9) / 2.0f) + y;
    y = (240.0f * arg9) - y;
    if (((x > 0.0f) ? x : -x) >= 16384.0f) {
        x = ((x >= 0.0f) ? 1 : -1) << 14;
    }
    if (((y > 0.0f) ? y : -y) >= 16384.0f) {
        y = ((y >= 0.0f) ? 1 : -1) << 14;
    }
    *outX = x;
    *outY = y;
}

void func_80276D1C(Mtx *arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 *arg5, f32 *arg6, f32 *arg7, f32 *arg8) {
    f32 mf[4][4];

    guMtxL2F(mf, arg0);
    *arg5 = mf[0][0] * arg1 + mf[1][0] * arg2 + mf[2][0] * arg3 + mf[3][0];
    *arg6 = mf[0][1] * arg1 + mf[1][1] * arg2 + mf[2][1] * arg3 + mf[3][1];
    *arg7 = mf[0][2] * arg1 + mf[1][2] * arg2 + mf[2][2] * arg3 + mf[3][2];
    *arg8 = mf[0][3] * arg1 + mf[1][3] * arg2 + mf[2][3] * arg3 + mf[3][3];
}

void func_80276E50(Gfx **arg0, FrameBuf *arg1, u8 arg2, s32 arg3, s32 arg4, s32 arg5) {
    s16 sx;
    s16 sy;
    Gfx *gfx;
    f32 mf1[4][4];
    f32 mf2[4][4];

    gfx = *arg0;
    func_8027690C(arg1, arg3 >> 5, arg4 >> 5, arg5 >> 5, &sx, &sy, NULL, NULL, NULL, 1.0f);
    if (sx < 30 || sx >= 291 || sy < 25 || sy >= 216) {
        if (sx < 30) {
            sx = 30;
        }
        if (sy < 25) {
            sy = 25;
        }
        if (sx >= 291) {
            sx = 290;
        }
        if (sy >= 216) {
            sy = 215;
        }
        D_8036C7D0[arg2][0].v.ob[0] = -15;
        D_8036C7D0[arg2][0].v.ob[1] = -15;
        D_8036C7D0[arg2][0].v.ob[2] = -10;
        D_8036C7D0[arg2][0].v.tc[0] = 0;
        D_8036C7D0[arg2][0].v.tc[1] = 0;
        D_8036C7D0[arg2][1].v.ob[0] = 15;
        D_8036C7D0[arg2][1].v.ob[1] = -15;
        D_8036C7D0[arg2][1].v.ob[2] = -10;
        D_8036C7D0[arg2][1].v.tc[0] = 0x3E0;
        D_8036C7D0[arg2][1].v.tc[1] = 0;
        D_8036C7D0[arg2][2].v.ob[0] = 15;
        D_8036C7D0[arg2][2].v.ob[1] = 15;
        D_8036C7D0[arg2][2].v.ob[2] = -10;
        D_8036C7D0[arg2][2].v.tc[0] = 0x3E0;
        D_8036C7D0[arg2][2].v.tc[1] = 0x3E0;
        D_8036C7D0[arg2][3].v.ob[0] = -15;
        D_8036C7D0[arg2][3].v.ob[1] = 15;
        D_8036C7D0[arg2][3].v.ob[2] = -10;
        D_8036C7D0[arg2][3].v.tc[0] = 0;
        D_8036C7D0[arg2][3].v.tc[1] = 0x3E0;
        guTranslateF(mf1, sx, sy, 0.0f);
        guRotateF(mf2, 135.0 - D_8036443E / 4095.0 * 360.0, 0.0f, 0.0f, 1.0f);
        guMtxCatF(mf2, mf1, mf1);
        guMtxF2L(mf1, &D_8036C850[arg2]);
        gSPMatrix(gfx++, &D_02000000.mtx[3], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(gfx++, &D_8036C850[arg2], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 0, 255);
        gDPLoadTextureBlock(gfx++, OS_K0_TO_PHYSICAL(D_802FA940), G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0, G_TX_CLAMP,
                            G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPVertex(gfx++, OS_K0_TO_PHYSICAL(D_8036C7D0[arg2]), 4, 0);
        gSP1Triangle(gfx++, 0, 1, 2, 0);
        gSP1Triangle(gfx++, 0, 2, 3, 0);
        gDPPipeSync(gfx++);
    }
    *arg0 = gfx;
}
