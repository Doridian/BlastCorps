/*
 * hd_code 26570 (yoshi.c: the game's windows), jp's func_8026BCE0, the
 * window renderer, which the C has as jp's GLOBAL_ASM, as native C (engine.h; jp_E7B0.c says more).
 *
 * jp's differs from the US versions' C in two places: a window with
 * 0x4000000 set is opened 1.2 times as slowly when any of its entries (in
 * yoshi.c's own table, D_802F5804) has u16 text, and an entry with u16 text
 * is drawn from it (func_8026F004(.., 1) into func_80259DC8's third
 * argument) instead of from the char text.
 */
#include "engine.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/sched.h"
#include "game/yoshi.h"

#ifdef VERSION_JP

extern u8 D_802E8BD4;
extern u8 D_803643D6;
extern u8 D_803643DB;
extern u8 D_8036BA48[];
extern u8 D_8036BA98[];
extern u32 D_8036BAFC;
extern u16 D_8036BB04;
extern f32 D_8036BB08;
extern s16 D_8036BB0C;
extern s8 D_8036BB0E;
extern u16 D_8036BB14;
extern u16 D_8036BB16;
extern s16 D_8036BB1A;
extern s16 D_8036BB1E;
extern s16 D_8036BB20;
extern f32 D_8036BB28;
extern f32 D_8036BB2C;
extern s32 D_8036BB30;
extern f32 D_8036BB34;
extern f32 D_8036BB38;
extern u16 D_8036BB3C;
extern u16 D_8036BB3E;
extern u32 D_8036BB40;
extern s32 D_8036BB44;
extern s8 D_80370C11;
extern s8 D_80370C12;
extern s8 D_80370C13;
extern s8 D_80370C14;
extern Gfx D_802F98B0[];
extern s32 D_802F9930;

/* (f32) of a u32: cvt.s.w, and 2^32 added when it was negative */
#define U2F(dst, v)                                                         \
    do {                                                                    \
        s32 u2f_ = (s32)(v);                                                \
        (dst) = (f32)u2f_;                                                  \
        if (u2f_ < 0) {                                                     \
            (dst) += 4294967296.0f;                                         \
        }                                                                   \
    } while (0)

/* IDO's signed x / 2^n */
#define SDIV(dst, x, n)                                                     \
    do {                                                                    \
        s32 sdiv_ = (x);                                                    \
        (dst) = sdiv_ / (n);                                                \
    } while (0)

/* IDO's double to unsigned int (cvt.w.d with the FCSR truncating, as
   engine_ido_cvt_u_s for a float) */
static int ido_trunc_ok_d(f64 x, s32 *v) {
    if (!(x > -2147483649.0 && x < 2147483648.0))
        return 0;
    *v = (s32)x;
    return 1;
}

static u32 ido_cvt_u_d(f64 x) {
    s32 v;

    if (ido_trunc_ok_d(x, &v))
        return v < 0 ? 0xFFFFFFFFu : (u32)v;
    if (ido_trunc_ok_d(x - 2147483648.0, &v))
        return (u32)v | 0x80000000u;
    return 0xFFFFFFFFu;
}

#define IDO_CVT_U_D(dst, x) ((dst) = ido_cvt_u_d(x))

/* the shadow's and the text's alpha: IDO's (u32 products) / 65025 */
static s32 alpha_of(s16 fade, u8 a, u8 v) {
    return (s32)((u32)v * (u32)(fade * a)) / 65025;
}

Gfx *func_8026BCE0(Gfx *arg0, FrameBuf *arg1, s32 *arg2) {
    YoshiWindow *w;
    YoshiEntry *e;
    void *sp144 = NULL;  /* the char text: kept from the last entry without u16 text */
    u16 *sp140;
    Gfx *gfx;
    u16 sp13A;
    u16 i;
    u16 sp136;
    u16 sp134;
    u16 sp132;
    u16 sp130;
    f32 sp128;
    u16 sp126;
    u16 sp124;
    s32 sp120;
    u8 sp11F;
    f32 f;
    u32 u;
    s32 t;

    gfx = arg0;
    sp132 = (D_803156C4 - D_8036BB40) * 15;
    sp130 = 0;
    sp128 = 1.0f;
    D_8036BB40 = D_803156C4;
    D_8036BB0C += D_8036BB0E * sp132;
    if (D_8036BB0C >= 0x100) {
        D_8036BB0C = 0xFF;
        D_8036BB0E = -D_8036BB0E;
    }
    if (D_8036BB0C < 0) {
        D_8036BB0C = 0;
        D_8036BB0E = -D_8036BB0E;
    }
    {
        ColorPair *c;
        u8 b;

        c = &D_802F47B0[16];
        c->g0 = 0xFF - D_8036BB0C;
        c->g1 = D_8036BB0C;
        c = &D_802F47B0[17];
        c->g0 = 0xAA - D_8036BB0C * 2 / 3;
        c->g1 = D_8036BB0C * 2 / 3;
        c = &D_802F47B0[18];
        b = 0xFF - D_8036BB0C;
        c->g0 = b;
        c->r0 = b;
        b = (u8)D_8036BB0C;       /* lbu D_8036BB0D */
        c->g1 = b;
        c->r1 = b;
        c = &D_802F47B0[19];
        b = 0xFF - D_8036BB0C;
        c->g0 = b;
        c->b0 = b;
        b = (u8)D_8036BB0C;       /* lbu D_8036BB0D */
        c->g1 = b;
        c->b1 = b;
        c = &D_802F47B0[20];
        c->b0 = 0xFF - D_8036BB0C;
        c->b1 = D_8036BB0C;
    }
    if ((u32)(D_80364A90 >> 32) == 0) {
        if ((u32)D_80364A90 == 0x200) {
            if (D_803643DB != 0) {
                if (D_803643D6 != 0) {
                    D_8036BB1A = -1;
                    if (D_8036BB1C == 4 || (D_8036BB1C == 2)) {
                        func_8029A7E4("putting off!\n");
                        func_8026AF6C(0x4000);
                    }
                }
            }
        }
    }
    if (D_8036BB18 == -1) {
        if (D_8036BB14 & 0x4000) {
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "yoshi.c", 0x65B);
            D_8036BB14 = 0;
            return arg0;
        }
    }
    if (D_8036BB14 != 0) {
        sp126 = D_8036BB14 & 0xFF;
        sp124 = D_8036BB14 & 0x2000;
        sp120 = 0;
        func_8029A7E4("yoshiDemand=%x\n", D_8036BB14);
        if (D_8036BB14 & 0x8000) {
            D_8036BB1A = -1;
            if (D_8036BB18 != -1) {
                sp120 = D_802F8BDC[D_8036BB18].unk8 & 0x8000000;
            }
            if (D_8036BB1C == 1 || (sp124 != 0) ||
                (sp120 != 0)) {
                D_8036BB18 = sp126;
                D_8036BB1C = 1;
            } else {
                D_8036BB1A = sp126;
            }
        }
        if (D_8036BB1C != 8) {
            sp130 = 1;
        }
        D_8036BB14 = 0;
    }
    if (D_8036BB18 == -1) {
        D_8036BB18 = D_8036BB1A;
        D_8036BB1A = -1;
        if (D_8036BB18 == -1) {
            return arg0;
        }
        sp130 = 1;
    }
    w = &D_802F8BDC[D_8036BB18];
    func_8026FB50(w);
    if (w->unk8 & 0x4000000) {
        /* jp: slower where any entry has u16 text */
        i = w->first;
        if (i < w->first + w->count) {
            if ((f64)sp128 == 1.0) {
                do {
                    if (D_802F5804[i].unk10 != NULL) {
                        sp128 = 1.2f;
                    }
                    i++;
                    if (!(i < w->first + w->count))
                        break;
                } while ((f64)sp128 == 1.0);
            }
        }
    }
    if (w->unk8 & 0x20) {
        if (D_8036BB1C == 2) {
            s32 go;

            go = 0;
            if (D_80370C28 & 0x8000) {
                if (!(D_80370C2A & 0x8000))
                    go = 1;
            }
            if (!go) {
                if (w->unk8 & 0x80000000) {
                    if (D_80370C28 & 0x1000) {
                        if (!(D_80370C2A & 0x1000))
                            go = 1;
                    }
                }
            }
            if (go) {
                if (w->unk1A != 0) {
                    sp13A = D_8036BB10[w->unk18].unk16;
                    if (sp13A != 0) {
                        func_80260650(D_80367738, sp13A, 0);
                    }
                    if (D_8036BB10[w->unk18].flags & 0x10) {
                        D_802E8BD4 = 1;
                    }
                    D_8036BB16 = w->unk18;
                    sp130 = 1;
                }
            }
            if (D_80370C28 & 0x4000) {
                if (!(D_80370C2A & 0x4000)) {
                    if ((w->unk8 & 0x20000000) && (w->unk1A != 0)) {
                        func_80260650(D_80367738, 0xDE, 0);
                        D_8036BB16 = 0xFFFF;
                        sp130 = 1;
                        if (w->unk8 & 0x40000000) {
                            D_802E8BD4 = 1;
                        }
                    } else {
                        func_80260650(D_80367738, 0xD0, 0);
                    }
                }
            }
        }
    }
    if (sp130 != 0) {
        D_8036BAFC = D_803156C4;
        switch (D_8036BB1C) {
        case 8:
            U2F(f, D_803156C4);
            IDO_CVT_U_S(u, f - D_8036BB08 * D_8036BB38 * D_8036BB34);
            D_8036BAFC = u;
            /* fallthrough */
        case 1:
            D_8036BB1C = 4;
            D_8036BB34 = 1.0f;
            if (w->unk8 & 0x18) {
                D_8036BB08 = 40.0f;
            } else {
                D_8036BB08 = 13.333333f;
            }
            func_8026EF70(w);
            sp13A = w->unk12;
            if (sp13A != 0) {
                func_80260650(D_80367738, sp13A, 0);
            }
            if (!(w->unk8 & 0x400)) {
                i = w->first;
                if (!(D_8036BB10[i].flags & 1)) {
                    if (i < w->first + w->count) {
                        do {
                            i++;
                            if (D_8036BB10[i].flags & 1)
                                break;
                        } while (i < w->first + w->count);
                    }
                }
                w->unk18 = i;
            }
            i = w->first;
            if (i < w->first + w->count) {
                do {
                    e = &D_8036BB10[i];
                    if (e->flags & 0x20) {
                        s32 x;

                        if (w->unk8 & 0x80000) {
                            SDIV(x, w->unk0, 2);
                            t = func_8025B498(x, e->unk6, e->text, e->unk10);
                            e->x = t;
                        } else {
                            SDIV(x, w->unk0, 2);
                            t = func_8025B498(x, e->unk6, e->text, e->unk10);
                            e->x = t;
                        }
                    }
                    i++;
                } while (i < w->first + w->count);
            }
            if (w->unk8 & 1) {
                D_802E8BD8 = 1;
            }
            if (w->unk8 & 2) {
                func_80261570(0.0f);
            }
            if (w->unk8 & 0x100000) {
                U2F(f, w->unk2);
                D_8036BB28 = f;
            } else {
                s32 h;

                SDIV(h, w->unk2, 2);
                f = (f32)(h - D_8036BB10[w->unk18].y);
                D_8036BB28 = f;
                if (w->unk8 & 0x40000) {
                    SDIV(h, D_8036BB10[w->unk18].unk8, 2);
                    D_8036BB28 = f - (f32)h;
                }
            }
            D_8036BB2C = D_8036BB28;
            break;
        case 4:
            {
                f32 p = (D_8036BB38 - sp128) * D_8036BB08;

                U2F(f, D_803156C4);
                IDO_CVT_U_S(u, p + f);
            }
            D_8036BAFC = u;
            goto case2;
        case 2:
        case2:
            D_8036BB1C = 8;
            sp13A = w->unk14;
            if (sp13A != 0) {
                func_80260650(D_80367738, sp13A, 0);
            }
            if (w->unk8 & 0x200000) {
                D_802E8BD4 = 1;
            }
            if (w->unk8 & 4) {
                func_80261570(1.0f);
            }
            break;
        default:
            break;
        }
    }
    switch (D_8036BB1C) {
    case 2:
        if (w->unk8 & 0x100000) {
            s32 h;

            t = D_8036BB10[w->first + w->count - 1].y;
            SDIV(h, w->unk2, 8);
            sp11F = 0;
            if (D_8036BB2C < (f32)(h - t)) {
                sp11F = 1;
            }
        } else {
            s32 s1;

            s1 = w->unkC != 0;
            if (s1) {
                f32 q, c;

                U2F(f, D_803156C4 - D_8036BAFC);
                q = f / 60.0f;
                U2F(c, w->unkC);
                s1 = 0;
                if (c < q) {
                    s1 = 1;
                }
                if (s1) {
                    s1 = !(w->unk8 & 0x400000);
                    if (!s1) {
                        s1 = !(D_8036BB1E != 0);
                    }
                }
            }
            sp11F = s1;
        }
        if (sp11F != 0) {
            sp13A = w->unk14;
            if (sp13A != 0) {
                func_80260650(D_80367738, sp13A, 0);
            }
            if (w->unk8 & 0x2000) {
                func_80261570(0.0f);
            }
            D_8036BB1C = 8;
            D_8036BAFC = D_803156C4;
        }
        break;
    case 4:
        U2F(f, D_803156C4 - D_8036BAFC);
        D_8036BB38 = f / D_8036BB08;
        if (sp128 < D_8036BB38) {
            D_8036BAFC = D_803156C4;
            D_8036BB1C = 2;
            D_8036BB38 = sp128;
            if (w->unk8 & 0x40) {
                D_8036BB3C = 0x200;
                D_8036BB3E = 0x100;
            } else {
                D_8036BB3C = 0x800;
                D_8036BB3E = 0x400;
            }
            if (w->unk8 & 0x10000000) {
                w->unk1A = 1;
            } else {
                w->unk1A = 0;
            }
            t = func_8026F8A8(w->first, w->count, w->unk18, 1);
            if (t == w->unk18) {
                w->unk1A = 1;
            }
        }
        break;
    case 8:
        U2F(f, D_803156C4 - D_8036BAFC);
        D_8036BB38 = sp128 - f / D_8036BB08;
        if ((f64)D_8036BB38 < 0.001) {
            D_8036BB38 = 0.0f;
            D_8036BB1C = 1;
            if (w->unk8 & 0x2000000) {
                D_802E8BD4 = 1;
            }
            if (w->unk8 & 0x100) {
                w->unk8 &= ~0x80;
            }
            D_8036BB18 = -1;
            return arg0;
        }
        break;
    default:
        break;
    }
    if (D_8036BB1C != 1) {
        f32 s;

        s = func_802574F0((f32)((f64)(D_8036BB38 * D_8036BB34 / sp128) * 1.57 + 4.71));
        D_8036BB20 = engine_trunc_w_d(((f64)s + 1.0) * 255.0);
    }
    if (D_8036BB1C == 1 || (!(0.1 < (f64)(D_8036BB38 * D_8036BB34)))) {
        return gfx;
    }

    SDIV(sp136, w->unk0, 2);
    SDIV(sp134, w->unk2, 2);
    guOrtho(&arg1->mtx[73], -w->unk4 - sp136, -w->unk4 - sp136 + 319, -w->unk6 - sp134 + 239,
            -w->unk6 - sp134, -256.0f, 256.0f, 256.0f);
    gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[73]), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    if (w->unk8 & 0x10) {
        guRotate(&arg1->mtx[75], 180.0 - (f64)(D_8036BB38 * D_8036BB34 / sp128) * 180.0, 2.0f, 0.0f, 1.0f);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[75]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    } else {
        guTranslate(&arg1->mtx[75], 0.0f, 0.0f, 0.0f);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[75]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    }
    gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    if (w->unk8 & 8) {
        f32 x, y;

        U2F(f, sp136);
        x = f * D_8036BB38 * D_8036BB34 / 1000.0f;
        U2F(f, sp134);
        y = f * D_8036BB38 * D_8036BB34 / 1000.0f;
        guScale(&arg1->mtx[76], x, y, 1.0f);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    } else {
        f32 x;

        U2F(f, sp136);
        x = f / 1000.0f;
        U2F(f, sp134);
        guScale(&arg1->mtx[76], x, f / 1000.0f, 1.0f);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    }
    gDPPipeSync(gfx++);
    gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
    gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    if (!(w->unk8 & 0x200)) {
        gSPDisplayList(gfx++, D_802F98B0);
    }
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    if (w->unk8 & 8) {
        guScale(&arg1->mtx[74], D_8036BB38, D_8036BB38, 1.0f);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[74]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    }

    /* the selection's moves */
    if ((w->unk8 & 0x20) && (D_8036BB1C == 2)) {
        s32 spD8, spD4;
        u16 spD2, spD0;
        u16 m;

        if (D_8036BB3C == 0x800) {
            if (D_80370C12 >= 31 && (D_80370C14 < 31)) {
                spD8 = 1;
            } else {
                spD8 = 0;
            }
        } else {
            if (D_80370C11 < -30 && (D_80370C13 >= -30)) {
                spD8 = 1;
            } else {
                spD8 = 0;
            }
        }
        if (D_8036BB3E == 0x400) {
            if (D_80370C12 < -30 && (D_80370C14 >= -30)) {
                spD4 = 1;
            } else {
                spD4 = 0;
            }
        } else {
            if (D_80370C11 >= 31 && (D_80370C13 < 31)) {
                spD4 = 1;
            } else {
                spD4 = 0;
            }
        }
        if (((D_80370C28 & D_8036BB3C) && (!(D_80370C2A & D_8036BB3C))) ||
            (spD8 != 0)) {
            spD2 = func_8026F82C(w->first, w->unk18, 1);
            sp13A = w->unk16;
            if (sp13A != 0) {
                if (spD2 != w->unk18) {
                    func_80260650(D_80367738, sp13A, 0);
                } else {
                    func_80260650(D_80367738, 0xD0, 0);
                }
            }
            D_8036BB28 += (f32)(D_8036BB10[w->unk18].y - D_8036BB10[spD2].y);
            w->unk18 = spD2;
        } else {
            if (w->unk1A != 0) {
                m = 0;
            } else {
                m = 0x8000;
            }
            if ((m | D_8036BB3E) & D_80370C28) {
                if (w->unk1A != 0) {
                    m = 0;
                } else {
                    m = 0x8000;
                }
                if (!((m | D_8036BB3E) & D_80370C2A))
                    goto down;
            }
            if (spD4 != 0) {
            down:
                spD0 = func_8026F8A8(w->first, w->count, w->unk18, 1);
                t = func_8026F8A8(w->first, w->count, spD0, 1);
                if (t == spD0) {
                    w->unk1A = 1;
                }
                sp13A = w->unk16;
                if (sp13A != 0) {
                    if (spD0 != w->unk18) {
                        func_80260650(D_80367738, sp13A, 0);
                    } else {
                        func_80260650(D_80367738, 0xD0, 0);
                    }
                }
                D_8036BB28 += (f32)(D_8036BB10[w->unk18].y - D_8036BB10[spD0].y);
                w->unk18 = spD0;
                D_8036BAFC = D_803156C4;
            }
        }
    }

    /* the scrolling, and the arrows */
    if (w->unk8 & 0x4000) {
        if (w->unk8 & 0x100000) {
            s32 hold = 0;

            if (w->unkC != 0) {
                f32 q, c;

                U2F(f, D_803156C4 - D_8036BAFC);
                q = f / 60.0f;
                U2F(c, w->unkC);
                hold = q < c;
            }
            if (!hold) {
                if (w->unk8 & 0x800000) {
                    D_8036BB2C = (f64)D_8036BB2C - 0.5;
                } else {
                    D_8036BB2C = (f64)D_8036BB2C - 1.0;
                }
            }
        } else {
            D_8036BB2C = (f64)D_8036BB2C + (f64)(D_8036BB28 - D_8036BB2C) * 0.1;
        }
        if (w->unk8 & 0x10000) {
            s32 spCC;
            s16 spCA;
            s32 h, h2;
            ColorPair *c;

            spCC = 0;
            if (w->unk8 & 0x40000) {
                spCA = 0x12;
            } else {
                spCA = 0x1C;
            }
            if (((u32)(D_80364A90 >> 32) & 0xC9FD0FE7) == 0 &&
                (((u32)D_80364A90 & 0x9BFF80B0) == 0)) {
                goto add;
            }
            if (D_8035805C != 0) {
            add:
                D_8036BB44 += D_802F9930;
            }
            if (D_802F9930 < 0) {
                D_8036BB44 += D_802F9930 * 2;
            }
            if (D_8036BB44 < 0 || (D_8036BB44 >= 8)) {
                D_8036BB44 -= D_802F9930 * 2;
                D_802F9930 = -D_802F9930;
            }
            t = func_8026F8A8(w->first, w->count, w->unk18, 1);
            if (t != w->unk18) {
                c = &D_802F47B0[18];
                SDIV(h, D_8036BB44, 2);
                spCC = func_80276130(arg1, 0, spCC, -sp136, sp134 - D_8036BB44 - spCA, 16, h + 10, c->r0, c->g0,
                                     c->b0, D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20, c->r0, c->g0, c->b0,
                                     D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20);
                SDIV(h, D_8036BB44, 2);
                SDIV(h2, D_8036BB20, 2);
                spCC = func_80276080(arg1, 0, spCC, -3 - sp136, sp134 - D_8036BB44 - spCA + 3, 16, h + 10, 0, 0, 0,
                                     h2);
                gfx = func_80275DA4(gfx, 1);
                gSPVertex(gfx++, arg1->vtx, 8, 0);
                gSP1Triangle(gfx++, 4, 5, 6, 0);
                gSP1Triangle(gfx++, 4, 6, 7, 0);
                gSP1Triangle(gfx++, 0, 1, 2, 0);
                gSP1Triangle(gfx++, 0, 2, 3, 0);
            }
            t = func_8026F82C(w->first, w->unk18, 1);
            if (t != w->unk18) {
                c = &D_802F47B0[18];
                SDIV(h, D_8036BB44, 2);
                spCC = func_80276130(arg1, 1, spCC, -sp136, D_8036BB44 - sp134 + spCA, 16, h + 10, c->r0, c->g0,
                                     c->b0, D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20, c->r0, c->g0, c->b0,
                                     D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20);
                SDIV(h, D_8036BB44, 2);
                SDIV(h2, D_8036BB20, 2);
                spCC = func_80276080(arg1, 1, spCC, -3 - sp136, D_8036BB44 - sp134 + spCA - 3, 16, h + 10, 0, 0, 0,
                                     h2);
                gfx = func_80275DA4(gfx, 1);
                gSPVertex(gfx++, &arg1->vtx[spCC - 8], 8, 0);
                gSP1Triangle(gfx++, 4, 5, 6, 0);
                gSP1Triangle(gfx++, 4, 6, 7, 0);
                gSP1Triangle(gfx++, 0, 1, 2, 0);
                gSP1Triangle(gfx++, 0, 2, 3, 0);
            }
        }
    } else {
        D_8036BB2C = 0.0f;
    }

    /* the icons */
    D_8036BB30 = engine_trunc_w_s(D_8036BB2C);
    if (w->unk8 & 0x1000) {
        if (w->unk8 & 0x20000) {
            gfx = func_80274868(gfx);
        } else {
            gfx = func_80274998(gfx);
        }
        i = w->first;
        if (i < w->first + w->count) {
            do {
                YoshiIcon *icon;
                s16 sp92, sp90;
                u8 sp8F, sp8E, sp8D, sp8C, sp8B;
                s32 s1;
                u8 v;
                u32 tick, q;

                e = &D_8036BB10[i];
                if (e->flags & 0x800)
                    goto next_icon;
                if (!(e->flags & 0x400))
                    goto next_icon;
                if (e->flags & 0x300) {
                    if (D_8036BB04 < i)
                        goto next_icon;
                }
                icon = &D_802F49F4[e->unk14];
                if (w->unk8 & 0x20000) {
                    sp92 = w->unk4;
                } else {
                    sp92 = -sp136;
                }
                if (w->unk8 & 0x20000) {
                    sp90 = w->unk6;
                } else {
                    sp90 = -sp134;
                }
                sp8F = 1;
                sp8E = icon->unk25;
                if (e->flags & 1) {
                    if (i != w->unk18) {
                        sp8F = 0;
                    }
                }
                tick = D_803156C4 * 60 / 60;
                sp8D = D_8036BA48[e->unk14];
                if (icon->unk26 == 0) {
                    engine_break(N64_PC(0x8026E16C), 7);
                }
                q = tick / icon->unk26;
                if (icon->unk1A == 0) {
                    engine_break(N64_PC(0x8026E198), 7);
                }
                sp8C = D_8036BA48[e->unk14] = q % icon->unk1A;
                if (sp8C != sp8D) {
                    if (sp8F != 0 || (D_8036BA98[e->unk14] != 0)) {
                        s32 n, d, r;

                        n = D_8036BA98[e->unk14] + 1;
                        d = icon->unk1A;
                        ENGINE_DIV(r, n, d, 8026E204, 8026E21C);
                        (void)r;
                        D_8036BA98[e->unk14] = n % d;
                    }
                }
                sp8B = icon->unk1B[D_8036BA98[e->unk14]];
                if (sp8B == 0)
                    goto next_icon;
                if (sp8F != 0) {
                    if (i == w->unk18) {
                        if (e->flags & 0x40) {
                            sp8E |= 8;
                        }
                    }
                    if (e->flags & 0x1000) {
                        s1 = D_8036BB30;
                    } else {
                        s1 = 0;
                    }
                    v = func_8026F644((UnkStruct_8026F644 *) w, &e->flags,
                                      e->y + icon->unk2 - sp134 + D_8036BB30 + 8);
                    U2F(f, v);
                    IDO_CVT_U_S(u, f * D_8036BB38 * D_8036BB34);
                    gfx = func_80272ED8(gfx, e->unk1A + sp8B - 1, icon->unk0 + e->x + sp92,
                                        s1 + (icon->unk2 + e->y + sp90), u, sp8E, icon->unk28);
                } else {
                    if (e->flags & 0x1000) {
                        s1 = D_8036BB30;
                    } else {
                        s1 = 0;
                    }
                    v = func_8026F644((UnkStruct_8026F644 *) w, &e->flags,
                                      e->y + icon->unk2 - sp134 + D_8036BB30 + 8);
                    U2F(f, v);
                    IDO_CVT_U_D(u, (f64)(f * D_8036BB38 * D_8036BB34) * 0.7);
                    gfx = func_80272ED8(gfx, e->unk1A + sp8B - 1, icon->unk0 + e->x + sp92,
                                        s1 + (icon->unk2 + e->y + sp90), u, sp8E & ~1, icon->unk28);
                }
            next_icon:
                i++;
            } while (i < w->first + w->count);
        }
        if (w->unk8 & 0x20000) {
            gfx = func_80274AA4(gfx);
        } else {
            gfx = func_80274B08(gfx);
        }
    }

    /* the selection's shadow */
    if (D_8036BB18 < 0x62 || (D_8036BB18 >= 0x6C) ||
        ((u32)(D_80364A90 >> 32) == 0 &&
                                   ((u32)D_80364A90 == 2))) {
        i = w->first;
        if (i < w->first + w->count) {
            do {
                s32 s1;
                u8 s3, s0;
                s32 a, b;

                e = &D_8036BB10[i];
                sp140 = NULL;
                if (e->unk10 != NULL && (*e->unk10 != 0xFFF)) {
                    sp140 = func_8026F004(w, i, 1);
                } else {
                    sp144 = func_8026F004(w, i, 0);
                }
                if (!(e->flags & 0x80))
                    goto next_shadow;
                if (e->flags & 0x800)
                    goto next_shadow;
                if (i == w->unk18) {
                    if (e->flags & 0x1000) {
                        s1 = D_8036BB30;
                    } else {
                        s1 = 0;
                    }
                    s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                    s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                    SDIV(a, alpha_of(D_8036BB20, port_color_pair(e->unk19).a0, s3), 2);
                    SDIV(b, alpha_of(D_8036BB20, port_color_pair(e->unk19).a0, s0), 2);
                    func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136 - 3, s1 + (e->y - sp134) + 3,
                                  e->unk6, e->unk8, 1, 0, 0, 0, a, 0, 0, 0, b);
                } else {
                    if (e->flags & 4) {
                        if (!(D_803156C4 % 23 * 60 / 60 < 16))
                            goto next_shadow;
                    }
                    if (e->flags & 0x1000) {
                        s1 = D_8036BB30;
                    } else {
                        s1 = 0;
                    }
                    s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                    s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                    SDIV(a, alpha_of(D_8036BB20, port_color_pair(e->unk18).a0, s3), 2);
                    SDIV(b, alpha_of(D_8036BB20, port_color_pair(e->unk18).a0, s0), 2);
                    func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136 - 3, s1 + (e->y - sp134) + 3,
                                  e->unk6, e->unk8, 1, 0, 0, 0, a, 0, 0, 0, b);
                }
            next_shadow:
                i++;
            } while (i < w->first + w->count);
        }
    }

    /* the text */
    i = w->first;
    if (i < w->first + w->count) {
        do {
            s32 s1;
            u8 s3, s0;
            ColorPair *c;
            ColorPair cp;

            e = &D_8036BB10[i];
            sp140 = NULL;
            if (e->unk10 != NULL && (*e->unk10 != 0xFFF)) {
                sp140 = func_8026F004(w, i, 1);
            } else {
                sp144 = func_8026F004(w, i, 0);
            }
            if (e->flags & 0x800)
                goto next_text;
            if (i == w->unk18) {
                if (e->flags & 4) {
                    if (!(D_803156C4 % 23 * 60 / 60 < 16))
                        goto next_text;
                }
                if (e->flags & 0x40) {
                    if (!(D_803156C4 % 15 * 60 / 60 < 11))
                        goto next_text;
                }
                if (e->flags & 0x1000) {
                    s1 = D_8036BB30;
                } else {
                    s1 = 0;
                }
                s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                cp = port_color_pair(e->unk19);
                c = &cp;
                func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136, s1 + (e->y - sp134), e->unk6,
                              e->unk8, 1, c->r0, c->g0, c->b0, alpha_of(D_8036BB20, c->a0, s3), c->r1, c->g1, c->b1,
                              alpha_of(D_8036BB20, c->a1, s0));
            } else {
                if (e->flags & 4) {
                    if (!(D_803156C4 % 23 * 60 / 60 < 16))
                        goto next_text;
                }
                if (e->flags & 0x1000) {
                    s1 = D_8036BB30;
                } else {
                    s1 = 0;
                }
                s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                cp = port_color_pair(e->unk18);
                c = &cp;
                func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136, s1 + (e->y - sp134), e->unk6,
                              e->unk8, 1, c->r0, c->g0, c->b0, alpha_of(D_8036BB20, c->a0, s3), c->r1, c->g1, c->b1,
                              alpha_of(D_8036BB20, c->a1, s0));
            }
        next_text:
            i++;
        } while (i < w->first + w->count);
    }
    func_80259BD4(&gfx, arg1);
    return gfx;
}

#endif
