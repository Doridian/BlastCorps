#include "common.h"
#include "game/sched.h"
#include "game/game.h"
#include "game/frame.h"
#include "functions.h"

/*
 * The older gbi.h's scissoring texture rectangle: it clamps without the
 * (s16) casts and adjusts s/t without checking the sign of xl/yl.
 */
#define gSPScisTextureRectangleOld(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)          \
    {                                                                                   \
        Gfx *_g = (Gfx *)(pkt);                                                         \
                                                                                        \
        _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(MAX((xh), 0), 12, 12) |     \
                        _SHIFTL(MAX((yh), 0), 0, 12));                                  \
        _g->words.w1 = (_SHIFTL((tile), 24, 3) | _SHIFTL(MAX((xl), 0), 12, 12) |        \
                        _SHIFTL(MAX((yl), 0), 0, 12));                                  \
        gImmp1(pkt, G_RDPHALF_2,                                                        \
               (_SHIFTL(((s) - MIN((((dsdx) * (xl)) >> 7), 0)), 16, 16) |               \
                _SHIFTL(((t) - MIN((((dtdy) * (yl)) >> 7), 0)), 0, 16)));               \
        gImmp1(pkt, G_RDPHALF_CONT, (_SHIFTL((dsdx), 16, 16) | _SHIFTL((dtdy), 0, 16))); \
    }

extern u8 D_803B9888;

Gfx *func_802742D8(Gfx *, u8, s16, s16, s32, s32, s32, f32, u8);

/* .bss, 0x8036BFE0-0x8036C770 (tools/bss_c.py) */
u8 *D_8036BFE0[0x40][2];
u8 D_8036C1E0[0x40];
u8 D_8036C220[0x40];
f32 D_8036C260[0x40];
u8 D_8036C360;
Vtx *D_8036C368[2][64][2];

void func_80272C50(void) {
    D_8036C360 = 0;
#ifdef TARGET_PC
    port_icon_texture(0, -1, 0, 0, 0);
#endif
}

u8 func_80272C5C(u16 *arg0, u16 *arg1, u8 arg2, u8 arg3, u8 arg4, f32 arg5) {
    u8 *sp3C;
    s32 i;
    s32 j;
    s32 k;
    s32 sp2C;
    s32 start;

    start = D_8036C360;
    if (D_803B9888 == 0) {
        func_802A0700();
    }
    sp2C = 0;
    i = start;
    while (i < arg2 + start) {
        if (arg1 != NULL) {
            func_80257490((s32 *)&D_80358070, 0x10);
            func_802A0EE0(arg1[i - start], (sp3C = D_80358070));
            D_80358070 += 0x80;
        } else {
            func_80257490((s32 *)&D_80358070, 0x10);
            sp3C = NULL;
        }
        for (j = 0; j < arg3; j++) {
            D_8036BFE0[i][j] = D_80358070;
            func_802A0B00(arg0[arg3 * sp2C + j], sp3C);
#ifdef TARGET_PC
            port_icon_texture((u32)(uintptr_t)D_8036BFE0[i][j], arg0[arg3 * sp2C + j], j, arg3, arg4);
#endif
        }
        D_8036C1E0[i] = arg3;
        D_8036C220[i] = arg4;
        if (arg4 & 4) {
            for (j = 0; j < 2; j++) {
                for (k = 0; k < 2; k++) {
                    D_8036C368[j][i][k] = (Vtx *)D_80358070;
                    D_80358070 += 0x80;
                }
            }
        }
        D_8036C260[i++] = arg5;
        sp2C += (arg1 != NULL) ? 0 : 1;
    }
    D_8036C360 = i;
    return start;
}

Gfx *func_80272ED8(Gfx *arg0, u8 arg1, s16 arg2, s16 arg3, u8 arg4, u8 arg5, f32 arg6) {
    s32 i;
    Gfx *gfx;
    u8 alpha;
    s32 spF8;
    s32 spF4;
    s32 spF0;
    s32 width;

    gfx = arg0;
    alpha = arg4;
    arg6 *= D_8036C260[arg1];
    width = D_8036C1E0[arg1];
    if ((arg5 & 8) && (D_803156C4 % 20) < 7) {
        alpha = (u32)arg4 >> 1;
    }
    gDPPipeSync(gfx++);
    if (D_8036C220[arg1] & 4) {
        if (arg6 > 1.0) {
            gDPSetTextureFilter(gfx++, G_TF_BILERP);
        } else {
            gDPSetTextureFilter(gfx++, G_TF_POINT);
        }
    }
    for (i = 0; i < width - ((D_8036C220[arg1] & 8) ? 1 : 0); i++) {
        if (D_8036C220[arg1] & 2) {
            gDPLoadTextureBlock(gfx++, D_8036BFE0[arg1][i], G_IM_FMT_RGBA, G_IM_SIZ_32b, width << 5, 32, 0,
                                G_TX_CLAMP, G_TX_MIRROR, 0, width + 4, 0, 0);
        } else {
            gDPLoadTextureBlock(gfx++, D_8036BFE0[arg1][i], G_IM_FMT_RGBA, G_IM_SIZ_16b, width << 5, 32, 0,
                                G_TX_CLAMP, G_TX_MIRROR, 0, width + 4, 0, 0);
        }
        if (arg5 != 0) {
            gDPPipeSync(gfx++);
            if (arg5 & 4) {
                gDPSetPrimColor(gfx++, 0, 0, arg4, arg4, arg4, 0xFF);
            } else {
                gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, arg4 / 2);
            }
            spF0 = (f32)width + (2.0f * arg6);
            gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0,
                              PRIMITIVE, 0);
            if (D_8036C220[arg1] & 4) {
                gfx = func_802742D8(gfx, arg1, arg2, arg3, i, width, spF0, arg6, 1);
            } else {
                spF8 = (arg2 - spF0) * 4;
                spF4 = (f32)((arg3 + spF0) * 4) + ((f32)((i << 5) * 4) * arg6);
                gSPScisTextureRectangleOld(gfx++, spF8, spF4,
                                           (f32)(arg2 * 4) + ((f32)((((width << 5) - spF0) - 1) << 2) * arg6),
                                           (f32)((arg3 + spF0) * 4) + ((f32)(((i << 5) + 32) << 2) * arg6),
                                           G_TX_RENDERTILE, 0, (D_8036C220[arg1] & 1) ? (width << 5) << 5 : 0,
                                           (s32)(1024.0f / arg6), (s32)(1024.0f / arg6));
            }
        }
        if (!(arg5 & 6)) {
            if (!(arg5 & 1)) {
                spF0 = (f32)width + (2.0f * arg6);
            } else {
                spF0 = 0;
            }
            gDPPipeSync(gfx++);
            gDPSetPrimColor(gfx++, 0, 0, 0xFF, 0, 0, alpha);
            if ((D_8036C220[arg1] & 2) || (alpha != 0xFF) ||
                !(D_80364A90 & 0xC9FD0FE79BFF80B0LL)) {
                gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            } else {
                gDPSetRenderMode(gfx++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
            }
            gDPSetCombineLERP(gfx++, TEXEL0, 0, PRIMITIVE_ALPHA, 0, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0,
                              PRIMITIVE_ALPHA, 0, TEXEL0, 0, PRIMITIVE, 0);
            if (D_8036C220[arg1] & 4) {
                gfx = func_802742D8(gfx, arg1, arg2, arg3, i, width, spF0, arg6, 0);
            } else {
                spF8 = (arg2 - spF0) * 4;
                spF4 = (f32)((arg3 + spF0) * 4) + ((f32)((i << 5) * 4) * arg6);
                gSPScisTextureRectangleOld(gfx++, spF8, spF4,
                                           (f32)(arg2 * 4) + ((f32)((((width << 5) - spF0) - 1) << 2) * arg6),
                                           (f32)((arg3 + spF0) * 4) + ((f32)(((i << 5) + 32) << 2) * arg6),
                                           G_TX_RENDERTILE, 0, (D_8036C220[arg1] & 1) ? (width << 5) << 5 : 0,
                                           (s32)(1024.0f / arg6), (s32)(1024.0f / arg6));
            }
        }
    }
    gDPPipeSync(gfx++);
    return gfx;
}

Gfx *func_802742D8(Gfx *gfx, u8 arg1, s16 arg2, s16 arg3, s32 arg4, s32 arg5, s32 arg6, f32 arg7, u8 arg8) {
    Vtx *vtx;
    s32 x;
    s32 y;
    s32 t;

    vtx = D_8036C368[D_8035805C][arg1][arg8];
    x = arg2 - arg6;
    y = arg3 + arg6;
    if (D_8036C220[arg1] & 1) {
        t = 0;
    } else {
        t = arg5 << 5;
    }
    if (arg4 != 0) {
        vtx += 4;
        vtx[0].v.ob[0] = x;
        vtx[0].v.ob[1] = (s32)(((f32)y + (64.0f * arg7)) - 1.0f);
        vtx[0].v.ob[2] = -10;
        vtx[0].v.tc[0] = 0;
        vtx[0].v.tc[1] = t << 5;
        vtx[1].v.ob[0] = (s32)(((f32)x + ((f32)(arg5 << 5) * arg7)) - 1.0f);
        vtx[1].v.ob[1] = (s32)(((f32)y + (64.0f * arg7)) - 1.0f);
        vtx[1].v.ob[2] = -10;
        vtx[1].v.tc[0] = ((arg5 << 5) - 1) << 5;
        vtx[1].v.tc[1] = t << 5;
        vtx[2].v.ob[0] = (s32)(((f32)x + ((f32)(arg5 << 5) * arg7)) - 1.0f);
        vtx[2].v.ob[1] = (s32)(((f32)y + (32.0f * arg7)) - 1.0f);
        vtx[2].v.ob[2] = -10;
        vtx[2].v.tc[0] = ((arg5 << 5) - 1) << 5;
        vtx[2].v.tc[1] = (t + 31) << 5;
        vtx[3].v.ob[0] = x;
        vtx[3].v.ob[1] = (s32)(((f32)y + (32.0f * arg7)) - 1.0f);
        vtx[3].v.ob[2] = -10;
        vtx[3].v.tc[0] = 0;
        vtx[3].v.tc[1] = (t + 31) << 5;
    } else {
        vtx[0].v.ob[0] = x;
        vtx[0].v.ob[1] = y;
        vtx[0].v.ob[2] = -10;
        vtx[0].v.tc[0] = 0;
        vtx[0].v.tc[1] = (t + 31) << 5;
        vtx[1].v.ob[0] = (s32)(((f32)x + ((f32)(arg5 << 5) * arg7)) - 1.0f);
        vtx[1].v.ob[1] = y;
        vtx[1].v.ob[2] = -10;
        vtx[1].v.tc[0] = ((arg5 << 5) - 1) << 5;
        vtx[1].v.tc[1] = (t + 31) << 5;
        vtx[2].v.ob[0] = (s32)(((f32)x + ((f32)(arg5 << 5) * arg7)) - 1.0f);
        vtx[2].v.ob[1] = (s32)(((f32)y + (32.0f * arg7)) - 1.0f);
        vtx[2].v.ob[2] = -10;
        vtx[2].v.tc[0] = ((arg5 << 5) - 1) << 5;
        vtx[2].v.tc[1] = t << 5;
        vtx[3].v.ob[0] = x;
        vtx[3].v.ob[1] = (s32)(((f32)y + (32.0f * arg7)) - 1.0f);
        vtx[3].v.ob[2] = -10;
        vtx[3].v.tc[0] = 0;
        vtx[3].v.tc[1] = t << 5;
    }
    gSPVertex(gfx++, vtx, 4, 0);
    osWritebackDCache(vtx, sizeof(Vtx) * 4);
    gSP1Triangle(gfx++, 0, 3, 2, 0);
    gSP1Triangle(gfx++, 0, 2, 1, 0);
    return gfx;
}

Gfx *func_80274868(Gfx *arg0) {
    Gfx *gfx = arg0;

    gDPPipeSync(gfx++);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    return gfx;
}

Gfx *func_80274998(Gfx *arg0) {
    Gfx *gfx = arg0;

    gDPPipeSync(gfx++);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    return gfx;
}

Gfx *func_80274AA4(Gfx *arg0) {
    Gfx *gfx = arg0;

    gDPPipeSync(gfx++);
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    return gfx;
}

Gfx *func_80274B08(Gfx *arg0) {
    Gfx *gfx = arg0;

    gDPPipeSync(gfx++);
    return gfx;
}

void func_80274B40(Gfx **gfxp, Frame *arg1, u8 arg2, s16 arg3, s16 arg4) {
    Gfx *gfx;

    gfx = *gfxp;
    if ((D_803156C4 % 40) < 28) {
        gfx = func_80274868(gfx);
        gfx = func_80272ED8(gfx, arg2, arg3, arg4, 0xFF, 1, 1.0f);
        gfx = func_80274AA4(gfx);
    }
    *gfxp = gfx;
}
