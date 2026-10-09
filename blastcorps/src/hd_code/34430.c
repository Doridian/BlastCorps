#include "common.h"
#include "game/level.h"
#include "game/game.h"
#include "functions.h"

typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
} UnkStruct_8036CB60; /* size = 0x18 */



s32 func_802796D8(s32 arg0, s32 *arg1, s32 *arg2);
s32 func_8027AC00(f32 a[3][3], f32 b[3][3], s32 p);
s32 func_8027B200(f32 a[3][3], f32 b[3][3]);
s32 func_8027B87C(f32 dst[4][4], f32 src[4][4]);
void func_8027A7DC(Gfx **gfxp, s32 arg1, s32 arg2);
s32 func_8027B5D0(f32 m[4][4], f32 *arg1, f32 *arg2, s32 *arg3);

/* .bss, 0x8036CB60-0x8036D3D0 (tools/bss_c.py) */
UnkStruct_8036CB60 D_8036CB60[11];
s32 D_8036CC68;
s32 D_8036CC6C;
Mtx D_8036CC70[2][10];
s32 D_8036D170;
f32 D_8036D174;
u8 D_8036D178;
s32 D_8036D17C;
s32 D_8036D180;
f32 D_8036D184;
Gfx D_8036D188[0x28];
Mtx D_8036D2C8;
u8 D_8036D308[0x80];
Mtx D_8036D388;

/* .data, 0x802FBED0-0x802FC080 (tools/data_c.py) */
Vp D_802FBED0 = { { { 240, 180, 511 }, { 240, 180, 511 } } };
extern Vtx D_802FBEE0[0x18];
#ifdef ASSET_INIT
#include "hd_code/34430/D_802FBEE0.inc"
#else
Vtx D_802FBEE0[0x18] = { 0 };
#endif
s32 D_802FC060[4][2] = { { 0x552078, 0x552230 }, { 0x5049D8, 0x504A50 }, { 0x553078, 0x553078 } };

void func_80278BF0(Gfx *src, Gfx *end, Gfx *PTR32 *out) {
    Gfx *dst;
    s8 cmd;
    s32 mode;
    u8 found;
    s32 i;
    s32 w1;

    *out = (Gfx *)D_80358070;
    dst = *out;
    D_80358070 = (u8 *)((Gfx *)D_80358070 + (end - src) - 2);
    while (src != end) {
        switch (cmd = src->words.w0 >> 24) {
            case (s8)G_ENDDL:
                src++;
                break;
            case (s8)G_SETOTHERMODE_L:
                mode = (src->words.w0 >> 8) & 0xFFFF;
                if (mode == 3) {
                    dst->words.w0 = src->words.w0;
                    w1 = src->words.w1;
                    i = 0;
                    found = 0;
                    do {
                        if (D_802FC060[i][0] == w1) {
                            found = 1;
                        } else {
                            i++;
                        }
                    } while (!found && i < 3);
                    if (!found) {
                        func_8029A7E4("\n --- ASSERTION FAULT - %s - %s, line %d\n\n", "found", "mb.c", 157);
                    }
                    dst->words.w1 = D_802FC060[i][1];
                    dst++, src++;
                } else {
                    *dst++ = *src++;
                }
                break;
            default:
                *dst++ = *src++;
                break;
        }
    }
    gSPEndDisplayList(dst++);
}

void func_80278E3C(void) {
    func_80257490((s32)D_80358070, 0x40);
    D_8036D170 = (s32)D_80358070;
    D_80358070 += 0x5460;
    func_80257490((s32)D_80358070, 8);
    D_8036D178 = 0;
    D_8036CC68 = 0;
    D_8036CC6C = 0;
    D_8036D180 = 0;
    D_80367C00 = 0;
}

void func_80278EB0(s32 arg0, f32 arg1, s32 arg2) {
    f32 mf1[4][4];
    f32 mf2[4][4];
    f32 v1[3];
    f32 v2[3];
    f32 angle;
    s32 i;
    s32 k;
    f32 step;
    f32 spread;
    u8 ok;
    s32 idx0;
    s32 idx1;
    s32 dist;
    s32 unused;
    s32 sp2C;

    k = 0;
    if (arg0 > 10) {
        arg0 = 10;
    }
    if (arg1 > 1.0f) {
        arg1 = 1.0f;
    }
    D_8036D17C = arg0;
    D_8036D184 = arg1;
    D_8036D180 = 0;
    D_8036D178 = 1;
    ok = func_802796D8(1, &idx0, &idx1);
    if (ok) {
        dist = func_8026A6F0(D_8036CB60[idx0].unk0, D_8036CB60[idx0].unk4, D_8036CB60[idx0].unk8,
                             D_8036CB60[idx0].unkC, D_8036CB60[idx0].unk10, D_8036CB60[idx0].unk14);
        if (dist < 1.0) {
            dist = 1;
        }
        arg2 <<= 16;
        sp2C = func_802ACF3C(arg2 / dist);
        D_8036D174 = ((f32)sp2C / 65536.0) * 360.0;
    } else {
        D_8036D174 = 25.0f;
    }
    angle = D_8036D174 / 2.0;
    step = D_8036D174 / 6.0;
    spread = angle * 1.3333334f;
    for (i = 0; i < 6; i++) {
        guRotateF(mf1, angle, 1.0f, 0.0f, 0.0f);
        guMtxXFMF(mf1, 0.0f, 0.0f, -100.0f, &v1[0], &v1[1], &v1[2]);
        guRotateF(mf2, spread, 0.0f, 1.0f, 0.0f);
        guMtxXFMF(mf2, v1[0], v1[1], v1[2], &v2[0], &v2[1], &v2[2]);
        D_802FBEE0[k + 0].v.ob[0] = v2[0];
        D_802FBEE0[k + 0].v.ob[1] = v2[1];
        D_802FBEE0[k + 0].v.ob[2] = v2[2];
        guRotateF(mf2, -spread, 0.0f, 1.0f, 0.0f);
        guMtxXFMF(mf2, v1[0], v1[1], v1[2], &v2[0], &v2[1], &v2[2]);
        D_802FBEE0[k + 3].v.ob[0] = v2[0];
        D_802FBEE0[k + 3].v.ob[1] = v2[1];
        D_802FBEE0[k + 3].v.ob[2] = v2[2];
        guRotateF(mf1, angle - step, 1.0f, 0.0f, 0.0f);
        guMtxXFMF(mf1, 0.0f, 0.0f, -100.0f, &v1[0], &v1[1], &v1[2]);
        guRotateF(mf2, spread, 0.0f, 1.0f, 0.0f);
        guMtxXFMF(mf2, v1[0], v1[1], v1[2], &v2[0], &v2[1], &v2[2]);
        D_802FBEE0[k + 1].v.ob[0] = v2[0];
        D_802FBEE0[k + 1].v.ob[1] = v2[1];
        D_802FBEE0[k + 1].v.ob[2] = v2[2];
        guRotateF(mf2, -spread, 0.0f, 1.0f, 0.0f);
        guMtxXFMF(mf2, v1[0], v1[1], v1[2], &v2[0], &v2[1], &v2[2]);
        D_802FBEE0[k + 2].v.ob[0] = v2[0];
        D_802FBEE0[k + 2].v.ob[1] = v2[1];
        D_802FBEE0[k + 2].v.ob[2] = v2[2];
        k += 4;
        angle -= step;
    }
}

void func_802794A4(void) {
    if (D_80367C00 == 0) {
        if (D_8036D180 < 2) {
            D_8036D178 = 0;
        } else {
            D_8036D178 = 2;
        }
    }
}

void func_802794E4(void) {
    D_8036D178 = 0;
}

u8 func_802794F0(void) {
    return !(D_8036D178 == 0);
}

void func_80279514(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    D_8036CB60[D_8036CC68].unk0 = (f32)arg3 / 65536.0;
    D_8036CB60[D_8036CC68].unk4 = (f32)arg4 / 65536.0;
    D_8036CB60[D_8036CC68].unk8 = (f32)arg5 / 65536.0;
    D_8036CB60[D_8036CC68].unkC = arg0 / 32.0f;
    D_8036CB60[D_8036CC68].unk10 = arg1 / 32.0f;
    D_8036CB60[D_8036CC68].unk14 = arg2 / 32.0f;
    if (++D_8036CC68 == 11) {
        D_8036CC68 = 0;
    }
    if (D_8036CC68 == D_8036CC6C) {
        if (++D_8036CC6C == 11) {
            D_8036CC6C = 0;
        }
    }
}

s32 func_802796D8(s32 arg0, s32 *arg1, s32 *arg2) {
    s32 i;

    i = D_8036CC68;
    while (arg0--) {
        if (i != 0) {
            i--;
        } else {
            i = 10;
        }
        if (i == D_8036CC6C) {
            return 0;
        }
    }
    *arg1 = i;
    if (i != 0) {
        i--;
    } else {
        i = 10;
    }
    *arg2 = i;
    return 1;
}

void func_80279778(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, Gfx *dl, void *seg6, void *seg7,
                   s32 alpha) {
    Gfx *gfx;
    u16 perspNorm;
    s32 unused1;
    f32 atX;
    f32 atY;
    f32 atZ;
    f32 eyeX;
    f32 eyeY;
    f32 eyeZ;
    s32 unused2[8];
    f32 dx;
    f32 dz;

    if (D_8036D178 != 0) {
        gfx = D_8036D188;
        gSPViewport(gfx++, K0_TO_PHYS(&D_802FBED0));
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSegment(gfx++, 0, 0);
        gSPSegment(gfx++, 6, K0_TO_PHYS(seg6));
        gSPSegment(gfx++, 7, K0_TO_PHYS(seg7));
        gDPPipeSync(gfx++);
        gDPSetScissor(gfx++, G_SC_NON_INTERLACE, 0, 0, 120, 90);
        gDPSetColorDither(gfx++, G_CD_DISABLE);
        gDPSetCycleType(gfx++, G_CYC_FILL);
        gSPClearGeometryMode(gfx++, G_ZBUFFER);
        gDPSetDepthImage(gfx++, D_80358058);
        gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 120, D_80358058);
        gDPSetFillColor(gfx++, 0xFFFCFFFC);
        gDPFillRectangle(gfx++, 0, 0, 119, 89);
        gDPPipeSync(gfx++);
        gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 120, K0_TO_PHYS(D_8036D170));
        gDPSetFillColor(gfx++, 0);
        gDPFillRectangle(gfx++, 0, 0, 119, 89);
        gDPPipeSync(gfx++);
        guPerspective(&D_8036D2C8, &perspNorm, D_8036D174, 4.0f / 3.0f, 100.0f, 5e+03f, 1.0f);
        gSPMatrix(gfx++, K0_TO_PHYS(&D_8036D2C8), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPPerspNormalize(gfx++, perspNorm);
        atX = arg0 / 32.0f;
        atY = arg1 / 32.0f;
        atZ = arg2 / 32.0f;
        eyeX = (f32)arg3 / 65536.0;
        eyeY = (f32)arg4 / 65536.0;
        eyeZ = (f32)arg5 / 65536.0;
        dx = eyeX - atX;
        if (dx < 0.0) {
            dx = 0.0 - dx;
        }
        dz = eyeZ - atZ;
        if (dz < 0.0) {
            dz = 0.0 - dz;
        }
        if (dx > 0.5 || dz > 0.5) {
            guLookAt(&D_8036D388, eyeX, eyeY, eyeZ, atX, atY, atZ, 0.0f, 1.0f, 0.0f);
        } else {
            guLookAt(&D_8036D388, eyeX + 2.0, eyeY, eyeZ + 2.0, atX, atY, atZ, 0.0f, 1.0f, 0.0f);
        }
        gSPMatrix(gfx++, K0_TO_PHYS(&D_8036D388), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPSetEnvColor(gfx++, 0, 0, 0, alpha);
        gSPDisplayList(gfx++, K0_TO_PHYS(dl));
        gSPEndDisplayList(gfx++);
        func_80284E54(D_8036D188, gfx - D_8036D188, 2, 0, 0x54D, 0);
    }
}

void func_80279EE8(Gfx **gfxp, s32 arg1, u8 arg2) {
    Gfx *gfx;
    u8 ok;
    s32 off;
    s32 vi;
    f32 lookAt[4][4];
    f32 mf[4][4];
    s32 i;
    u8 alpha;
    u8 alphaStep;
    s32 count;
    s32 idx0;
    s32 idx1;
    f32 t;
    f32 atX;
    f32 atY;
    f32 atZ;
    f32 eyeX;
    f32 eyeY;
    f32 eyeZ;
    f32 dx;
    f32 dz;

    gfx = *gfxp;
    off = 0;
    vi = 0;
    count = 1;
    t = 0.0f;
    switch (D_8036D178) {
        case 1:
            if (++D_8036D180 == D_8036D17C) {
                D_8036D178 = 3;
            }
            break;
        case 2:
            if (--D_8036D180 == 0) {
                D_8036D178 = 0;
            }
            break;
    }
    if (D_8036D178 != 0) {
        gDPPipeSync(gfx++);
        gDPSetColorDither(gfx++, G_CD_NOISE);
        gDPSetAlphaDither(gfx++, G_AD_NOTPATTERN);
        gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
        gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineLERP(gfx++, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0,
                          PRIMITIVE, 0);
        gDPSetTextureFilter(gfx++, G_TF_BILERP);
        gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        alphaStep = 30 / (D_8036D17C - 1);
        alpha = 40 - alphaStep * (D_8036D17C - D_8036D180);
        for (i = 0; i < D_8036D180; i++) {
            gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, alpha);
            ok = func_802796D8(count, &idx0, &idx1);
            if (!ok) {
                break;
            }
            atX = (D_8036CB60[idx1].unkC - D_8036CB60[idx0].unkC) * t + D_8036CB60[idx0].unkC;
            atY = (D_8036CB60[idx1].unk10 - D_8036CB60[idx0].unk10) * t + D_8036CB60[idx0].unk10;
            atZ = (D_8036CB60[idx1].unk14 - D_8036CB60[idx0].unk14) * t + D_8036CB60[idx0].unk14;
            eyeX = (D_8036CB60[idx1].unk0 - D_8036CB60[idx0].unk0) * t + D_8036CB60[idx0].unk0;
            eyeY = (D_8036CB60[idx1].unk4 - D_8036CB60[idx0].unk4) * t + D_8036CB60[idx0].unk4;
            eyeZ = (D_8036CB60[idx1].unk8 - D_8036CB60[idx0].unk8) * t + D_8036CB60[idx0].unk8;
            t += D_8036D184;
            if (t >= 1.0) {
                count++;
                t = 0.0f;
            }
            dx = eyeX - atX;
            if (dx < 0.0) {
                dx = 0.0 - dx;
            }
            dz = eyeZ - atZ;
            if (dz < 0.0) {
                dz = 0.0 - dz;
            }
            if (dx > 0.5 || dz > 0.5) {
                guLookAtF(lookAt, eyeX, eyeY, eyeZ, atX, atY, atZ, 0.0f, 1.0f, 0.0f);
            } else {
                guLookAtF(lookAt, eyeX + 2.0, eyeY, eyeZ + 2.0, atX, atY, atZ, 0.0f, 1.0f, 0.0f);
            }
            ok = func_8027B87C(mf, lookAt);
            if (!ok) {
                func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "flag", "mb.c", 547);
            }
            guMtxF2L(mf, &D_8036CC70[arg2][i]);
            gSPMatrix(gfx++, K0_TO_PHYS(&D_8036CC70[arg2][i]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            gSPVertex(gfx++, K0_TO_PHYS(D_802FBEE0), 16, 0);
            vi = 0;
            off = 0;
            func_8027A7DC(&gfx, off, vi);
            off += 0xE10, vi += 4;
            func_8027A7DC(&gfx, off, vi);
            off += 0xE10, vi += 4;
            func_8027A7DC(&gfx, off, vi);
            off += 0xE10, vi += 4;
            func_8027A7DC(&gfx, off, vi);
            off += 0xE10, vi += 4;
            gSPVertex(gfx++, K0_TO_PHYS(&D_802FBEE0[16]), 8, 0);
            vi = 0;
            func_8027A7DC(&gfx, off, vi);
            off += 0xE10, vi += 4;
            func_8027A7DC(&gfx, off, vi);
            off += 0xE10, vi += 4;
            gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
            alpha -= alphaStep;
        }
        gDPPipeSync(gfx++);
        gDPSetColorDither(gfx++, G_CD_MAGICSQ);
        gDPPipeSync(gfx++);
        *gfxp = gfx;
    }
}

void func_8027A7DC(Gfx **gfxp, s32 arg1, s32 arg2) {
    Gfx *gfx;
    s32 unused;

    gfx = *gfxp;
    gDPLoadTextureBlock(gfx++, K0_TO_PHYS_ADD(D_8036D170 + arg1), G_IM_FMT_RGBA, G_IM_SIZ_16b, 120, 15, 0, G_TX_CLAMP,
                        G_TX_CLAMP, 0, 0, 0, 0);
    gSP1Triangle(gfx++, arg2, arg2 + 1, arg2 + 2, 0);
    gSP1Triangle(gfx++, arg2, arg2 + 2, arg2 + 3, 0);
    *gfxp = gfx;
}

void func_8027AA04(f32 (*v)[3], s32 a, s32 b, s32 c) {
    s32 i;
    s32 t;
    f32 f;

    if (a != 0) {
        if (b == 0) {
            for (i = 0; i < 3; i++) {
                f = v[b][i];
                v[b][i] = v[a][i];
                v[a][i] = f;
            }
            t = b;
            b = a;
            a = t;
        } else {
            for (i = 0; i < 3; i++) {
                f = v[c][i];
                v[c][i] = v[a][i];
                v[a][i] = f;
            }
            t = c;
            c = a;
            a = t;
        }
    }
    if (b != 1) {
        for (i = 0; i < 3; i++) {
            f = v[c][i];
            v[c][i] = v[b][i];
            v[b][i] = f;
        }
        t = c;
        c = b;
        b = t;
    }
}

s32 func_8027AC00(f32 a[3][3], f32 b[3][3], s32 col) {
    s32 p;
    s32 q;
    s32 r1;
    s32 r2;
    s32 i;
    f32 f;
    f32 v1;
    f32 v2;

    if (col == 0) {
        r1 = 1, r2 = 2;
    } else if (col == 1) {
        r2 = 2;
        r1 = 0;
    } else {
        r2 = 1;
        r1 = 0;
    }
    v1 = a[r1][1];
    if (v1 < 0.0f) {
        v1 = -v1;
    }
    v2 = a[r2][1];
    if (v2 < 0.0f) {
        v2 = -v2;
    }
    if (v1 > v2) {
        p = r1;
    } else {
        p = r2;
    }
    if (p == r1) {
        q = r2;
    } else {
        q = r1;
    }
    if (a[p][1] < 1e-08 && a[p][1] > -1e-08) {
        return 0;
    }
    f = 1.0 / a[p][1];
    a[p][1] = 1.0f;
    a[p][2] *= f;
    b[p][p] = f;
    b[p][col] *= f;
    for (i = 0; i < 3; i++) {
        if (i != p) {
            f = -a[i][1];
            a[i][1] = 0.0f;
            a[i][2] += f * a[p][2];
            b[i][p] = b[p][p] * f;
            b[i][col] += f * b[p][col];
        }
    }
    if (a[q][2] < 1e-08 && a[q][2] > -1e-08) {
        return 0;
    }
    f = 1.0 / a[q][2];
    a[q][2] = 1.0f;
    b[q][q] = f;
    b[q][col] *= f;
    b[q][p] *= f;
    for (i = 0; i < 3; i++) {
        if (i != q) {
            f = -a[i][2];
            a[i][2] = 0.0f;
            b[i][col] += f * b[q][col];
            b[i][p] += f * b[q][p];
            b[i][q] += f * b[q][q];
        }
    }
    func_8027AA04(b, col, p, q);
    return 1;
}

s32 func_8027B200(f32 a[3][3], f32 b[3][3]) {
    s32 i;
    s32 p;
    f32 f;
    f32 a0;
    f32 a1;
    f32 a2;

    b[0][0] = b[1][1] = b[2][2] = 1.0f;
    b[0][1] = b[0][2] = b[1][0] = b[1][2] = b[2][0] = b[2][1] = 0.0f;
    a0 = a[0][0];
    if (a0 < 0.0f) {
        a0 = -a0;
    }
    a1 = a[1][0];
    if (a1 < 0.0f) {
        a1 = -a1;
    }
    a2 = a[2][0];
    if (a2 < 0.0f) {
        a2 = -a2;
    }
    if (a0 > a1) {
        if (a0 > a2) {
            p = 0;
        } else {
            p = 2;
        }
    } else if (a1 > a2) {
        p = 1;
    } else {
        p = 2;
    }
    if (a[p][0] < 1e-08 && a[p][0] > -1e-08) {
        return 0;
    }
    f = 1.0 / a[p][0];
    a[p][0] = 1.0f;
    a[p][1] *= f;
    a[p][2] *= f;
    b[p][p] = f;
    for (i = 0; i < 3; i++) {
        if (i != p) {
            f = -a[i][0];
            a[i][0] = 0.0f;
            a[i][1] += f * a[p][1];
            a[i][2] += f * a[p][2];
            b[i][p] = b[p][p] * f;
        }
    }
    if (!func_8027AC00(a, b, p)) {
        return 0;
    }
    return 1;
}

s32 func_8027B5D0(f32 m[4][4], f32 *arg1, f32 *arg2, s32 *arg3) {
    s32 i;
    s32 j;
    f32 tmp;
    f32 max;

    *arg3 = -1;
    if (((m[3][3] > 0.0f) ? m[3][3] : -m[3][3]) < 1e-08) {
        max = 0.0f;
        for (i = 0; i < 4; i++) {
            if (m[i][3] > max) {
                *arg3 = i;
                max = m[*arg3][3];
            } else if (m[i][3] < -max) {
                *arg3 = i;
                max = -m[*arg3][3];
            }
        }
        if (*arg3 < 0) {
            return 0;
        }
        for (j = 0; j < 4; j++) {
            tmp = m[3][j];
            m[3][j] = m[*arg3][j];
            m[*arg3][j] = tmp;
        }
    }
    arg1[0] = -m[0][3];
    arg1[1] = -m[1][3];
    arg1[2] = -m[2][3];
    *arg2 = 1.0 / m[3][3];
    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][3] = 1.0f;
    m[3][0] *= *arg2;
    m[3][1] *= *arg2;
    m[3][2] *= *arg2;
    for (i = 0; i < 3; i++) {
        m[0][i] += arg1[0] * m[3][i];
        m[1][i] += arg1[1] * m[3][i];
        m[2][i] += arg1[2] * m[3][i];
    }
    return 1;
}

s32 func_8027B87C(f32 dst[4][4], f32 src[4][4]) {
    f32 m[4][4];
    s32 i;
    s32 j;
    s32 affine;
    f32 a[3][3];
    f32 b[3][3];
    f32 scale;
    f32 tmp;
    f32 v[4];
    f32 pos[4];
    s32 idx;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            m[i][j] = src[i][j];
            dst[i][j] = 0.0f;
        }
    }
    dst[0][0] = 1.0f;
    dst[1][1] = 1.0f;
    dst[2][2] = 1.0f;
    dst[3][3] = 1.0f;
    affine = m[0][3] == 0.0 && m[1][3] == 0.0 && m[2][3] == 0.0 && m[3][3] == 1.0;
    if (!affine && !func_8027B5D0(m, v, &scale, &idx)) {
        return 0;
    }
    pos[0] = m[3][0];
    pos[1] = m[3][1];
    pos[2] = m[3][2];
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            a[i][j] = m[i][j];
        }
    }
    if (!func_8027B200(a, b)) {
        return 0;
    }
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            dst[i][j] = b[i][j];
        }
    }
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            dst[3][i] -= pos[j] * b[j][i];
        }
    }
    if (!affine) {
        for (i = 0; i < 4; i++) {
            dst[i][3] += v[0] * dst[i][0] + v[1] * dst[i][1] + v[2] * dst[i][2];
            dst[i][3] *= scale;
        }
        if (idx >= 0) {
            for (i = 0; i < 4; i++) {
                tmp = dst[i][3];
                dst[i][3] = dst[i][idx];
                dst[i][idx] = tmp;
            }
        }
    }
    return 1;
}
