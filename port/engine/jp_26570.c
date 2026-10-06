/*
 * hd_code 26570 (yoshi.c: the game's windows), jp's func_8026BCE0, the
 * window renderer, which the C has as jp's GLOBAL_ASM, as native C charged
 * by the original's blocks (engine.h; jp_E7B0.c says more).
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

/* (f32) of a u32: cvt.s.w, and the block adding 2^32 when it was negative */
#define U2F(dst, v, blk)                                                    \
    do {                                                                    \
        s32 u2f_ = (s32)(v);                                                \
        (dst) = (f32)u2f_;                                                  \
        if (u2f_ < 0) {                                                     \
            ENGINE_BLK(blk);                                                \
            (dst) += 4294967296.0f;                                         \
        }                                                                   \
    } while (0)

/* IDO's signed x / 2^n: the fixup block when x is negative */
#define SDIV(dst, x, n, blk)                                                \
    do {                                                                    \
        s32 sdiv_ = (x);                                                    \
        if (sdiv_ < 0)                                                      \
            ENGINE_BLK(blk);                                                \
        (dst) = sdiv_ / (n);                                                \
    } while (0)

/* IDO's double to unsigned int (cvt.w.d with the FCSR truncating, as
   engine_ido_cvt_u_s for a float): the path as engine_ido_cvt_u_s's */
static int ido_trunc_ok_d(f64 x, s32 *v) {
    if (!(x > -2147483649.0 && x < 2147483648.0))
        return 0;
    *v = (s32)x;
    return 1;
}

static int ido_cvt_u_d(f64 x, u32 *out) {
    s32 v;

    if (ido_trunc_ok_d(x, &v)) {
        *out = v < 0 ? 0xFFFFFFFFu : (u32)v;
        return v < 0;
    }
    if (ido_trunc_ok_d(x - 2147483648.0, &v)) {
        *out = (u32)v | 0x80000000u;
        return 2;
    }
    *out = 0xFFFFFFFFu;
    return 3;
}

#define IDO_CVT_U_D(dst, x, b1, b2, b3, b4)                                 \
    do {                                                                    \
        u32 cvt_;                                                           \
        switch (ido_cvt_u_d((x), &cvt_)) {                                  \
        case 0: ENGINE_BLK(b4); break;                                      \
        case 1: ENGINE_BLK(b4); ENGINE_BLK(b3); break;                      \
        case 2: ENGINE_BLK(b1); ENGINE_BLK(b2); break;                      \
        default: ENGINE_BLK(b1); ENGINE_BLK(b3); break;                     \
        }                                                                   \
        (dst) = cvt_;                                                       \
    } while (0)

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

    ENGINE_BLK(8026BCE0);
    gfx = arg0;
    sp132 = (D_803156C4 - D_8036BB40) * 15;
    sp130 = 0;
    sp128 = 1.0f;
    D_8036BB40 = D_803156C4;
    D_8036BB0C += D_8036BB0E * sp132;
    if (D_8036BB0C >= 0x100) {
        ENGINE_BLK(8026BD7C);
        D_8036BB0C = 0xFF;
        D_8036BB0E = -D_8036BB0E;
    }
    ENGINE_BLK(8026BD94);
    if (D_8036BB0C < 0) {
        ENGINE_BLK(8026BDA4);
        D_8036BB0C = 0;
        D_8036BB0E = -D_8036BB0E;
    }
    ENGINE_BLK(8026BDC0);
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
        ENGINE_BLK(8026BF20);
        if ((u32)D_80364A90 == 0x200) {
            ENGINE_BLK(8026BF28);
            if (D_803643DB != 0) {
                ENGINE_BLK(8026BF38);
                if (D_803643D6 != 0) {
                    ENGINE_BLK(8026BF48);
                    D_8036BB1A = -1;
                    if (D_8036BB1C == 4 || (ENGINE_BLK(8026BF68), D_8036BB1C == 2)) {
                        ENGINE_BLK(8026BF70);
                        func_8029A7E4("putting off!\n");
                        ENGINE_BLK(8026BF7C);
                        func_8026AF6C(0x4000);
                    }
                }
            }
        }
    }
    ENGINE_BLK(8026BF84);
    if (D_8036BB18 == -1) {
        ENGINE_BLK(8026BF98);
        if (D_8036BB14 & 0x4000) {
            ENGINE_BLK(8026BFAC);
            func_8029A7E4("\n\007 --- ASSERTION FAULT - %s - %s, line %d\n\n", "1==0", "yoshi.c", 0x65B);
            ENGINE_BLK(8026BFCC);
            D_8036BB14 = 0;
            ENGINE_BLK(8026F094);
            return arg0;
        }
    }
    ENGINE_BLK(8026BFDC);
    if (D_8036BB14 != 0) {
        ENGINE_BLK(8026BFEC);
        sp126 = D_8036BB14 & 0xFF;
        sp124 = D_8036BB14 & 0x2000;
        sp120 = 0;
        func_8029A7E4("yoshiDemand=%x\n", D_8036BB14);
        ENGINE_BLK(8026C010);
        if (D_8036BB14 & 0x8000) {
            ENGINE_BLK(8026C024);
            D_8036BB1A = -1;
            if (D_8036BB18 != -1) {
                ENGINE_BLK(8026C044);
                sp120 = D_802F8BDC[D_8036BB18].unk8 & 0x8000000;
            }
            ENGINE_BLK(8026C068);
            if (D_8036BB1C == 1 || (ENGINE_BLK(8026C07C), sp124 != 0) ||
                (ENGINE_BLK(8026C088), sp120 != 0)) {
                ENGINE_BLK(8026C094);
                D_8036BB18 = sp126;
                D_8036BB1C = 1;
            } else {
                ENGINE_BLK(8026C0B0);
                D_8036BB1A = sp126;
            }
        }
        ENGINE_BLK(8026C0BC);
        if (D_8036BB1C != 8) {
            ENGINE_BLK(8026C0D0);
            sp130 = 1;
        }
        ENGINE_BLK(8026C0D8);
        D_8036BB14 = 0;
    }
    ENGINE_BLK(8026C0E0);
    if (D_8036BB18 == -1) {
        ENGINE_BLK(8026C0F4);
        D_8036BB18 = D_8036BB1A;
        D_8036BB1A = -1;
        if (D_8036BB18 == -1) {
            ENGINE_BLK(8026C124);
            ENGINE_BLK(8026F094);
            return arg0;
        }
        ENGINE_BLK(8026C12C);
        sp130 = 1;
    }
    ENGINE_BLK(8026C134);
    w = &D_802F8BDC[D_8036BB18];
    func_8026FB50(w);
    ENGINE_BLK(8026C160);
    if (w->unk8 & 0x4000000) {
        /* jp: slower where any entry has u16 text */
        ENGINE_BLK(8026C178);
        i = w->first;
        if (i < w->first + w->count) {
            ENGINE_BLK(8026C198);
            if ((f64)sp128 == 1.0) {
                do {
                    ENGINE_BLK(8026C1BC);
                    if (D_802F5804[i].unk10 != NULL) {
                        ENGINE_BLK(8026C1E0);
                        sp128 = 1.2f;
                    }
                    ENGINE_BLK(8026C1E8);
                    i++;
                    if (!(i < w->first + w->count))
                        break;
                    ENGINE_BLK(8026C214);
                } while ((f64)sp128 == 1.0);
            }
        }
    }
    ENGINE_BLK(8026C238);
    if (w->unk8 & 0x20) {
        ENGINE_BLK(8026C24C);
        if (D_8036BB1C == 2) {
            s32 go;

            ENGINE_BLK(8026C260);
            go = 0;
            if (D_80370C28 & 0x8000) {
                ENGINE_BLK(8026C274);
                if (!(D_80370C2A & 0x8000))
                    go = 1;
            }
            if (!go) {
                ENGINE_BLK(8026C288);
                if (w->unk8 & 0x80000000) {
                    ENGINE_BLK(8026C2A0);
                    if (D_80370C28 & 0x1000) {
                        ENGINE_BLK(8026C2B4);
                        if (!(D_80370C2A & 0x1000))
                            go = 1;
                    }
                }
            }
            if (go) {
                ENGINE_BLK(8026C2C8);
                if (w->unk1A != 0) {
                    ENGINE_BLK(8026C2D8);
                    sp13A = D_8036BB10[w->unk18].unk16;
                    if (sp13A != 0) {
                        ENGINE_BLK(8026C300);
                        func_80260650(D_80367738, sp13A, 0);
                    }
                    ENGINE_BLK(8026C314);
                    if (D_8036BB10[w->unk18].flags & 0x10) {
                        ENGINE_BLK(8026C344);
                        D_802E8BD4 = 1;
                    }
                    ENGINE_BLK(8026C350);
                    D_8036BB16 = w->unk18;
                    sp130 = 1;
                }
            }
            ENGINE_BLK(8026C368);
            if (D_80370C28 & 0x4000) {
                ENGINE_BLK(8026C37C);
                if (!(D_80370C2A & 0x4000)) {
                    ENGINE_BLK(8026C390);
                    if ((w->unk8 & 0x20000000) && (ENGINE_BLK(8026C3A8), w->unk1A != 0)) {
                        ENGINE_BLK(8026C3B4);
                        func_80260650(D_80367738, 0xDE, 0);
                        ENGINE_BLK(8026C3C8);
                        D_8036BB16 = 0xFFFF;
                        sp130 = 1;
                        if (w->unk8 & 0x40000000) {
                            ENGINE_BLK(8026C3F4);
                            D_802E8BD4 = 1;
                        }
                    } else {
                        ENGINE_BLK(8026C404);
                        func_80260650(D_80367738, 0xD0, 0);
                    }
                }
            }
        }
    }
    ENGINE_BLK(8026C418);
    if (sp130 != 0) {
        ENGINE_BLK(8026C424);
        D_8036BAFC = D_803156C4;
        switch (D_8036BB1C) {
        case 8:
            ENGINE_BLK(8026C448);
            ENGINE_BLK(8026C450);
            ENGINE_BLK(8026C458);
            ENGINE_BLK(8026C460);
            U2F(f, D_803156C4, 8026C498);
            ENGINE_BLK(8026C4A4);
            IDO_CVT_U_S(u, f - D_8036BB08 * D_8036BB38 * D_8036BB34, 8026C4D4, 8026C504, 8026C514, 8026C51C);
            ENGINE_BLK(8026C52C);
            D_8036BAFC = u;
            /* fallthrough */
        case 1:
            ENGINE_BLK(8026C538);
            D_8036BB1C = 4;
            D_8036BB34 = 1.0f;
            if (w->unk8 & 0x18) {
                ENGINE_BLK(8026C568);
                D_8036BB08 = 40.0f;
            } else {
                ENGINE_BLK(8026C578);
                D_8036BB08 = 13.333333f;
            }
            ENGINE_BLK(8026C588);
            func_8026EF70(w);
            ENGINE_BLK(8026C590);
            sp13A = w->unk12;
            if (sp13A != 0) {
                ENGINE_BLK(8026C5A0);
                func_80260650(D_80367738, sp13A, 0);
            }
            ENGINE_BLK(8026C5B4);
            if (!(w->unk8 & 0x400)) {
                ENGINE_BLK(8026C5C8);
                i = w->first;
                if (!(D_8036BB10[i].flags & 1)) {
                    ENGINE_BLK(8026C5F8);
                    if (i < w->first + w->count) {
                        do {
                            ENGINE_BLK(8026C610);
                            i++;
                            if (D_8036BB10[i].flags & 1)
                                break;
                            ENGINE_BLK(8026C648);
                        } while (i < w->first + w->count);
                    }
                }
                ENGINE_BLK(8026C668);
                w->unk18 = i;
            }
            ENGINE_BLK(8026C674);
            i = w->first;
            if (i < w->first + w->count) {
                do {
                    ENGINE_BLK(8026C698);
                    e = &D_8036BB10[i];
                    if (e->flags & 0x20) {
                        s32 x;

                        ENGINE_BLK(8026C6C8);
                        if (w->unk8 & 0x80000) {
                            ENGINE_BLK(8026C6E0);
                            SDIV(x, w->unk0, 2, 8026C6F4);
                            ENGINE_BLK(8026C6FC);
                            t = func_8025B498(x, e->unk6, e->text, e->unk10);
                            ENGINE_BLK(8026C708);
                            e->x = t;
                        } else {
                            ENGINE_BLK(8026C714);
                            SDIV(x, w->unk0, 2, 8026C730);
                            ENGINE_BLK(8026C738);
                            t = func_8025B498(x, e->unk6, e->text, e->unk10);
                            ENGINE_BLK(8026C744);
                            e->x = t;
                        }
                    }
                    ENGINE_BLK(8026C74C);
                    i++;
                } while (i < w->first + w->count);
            }
            ENGINE_BLK(8026C778);
            if (w->unk8 & 1) {
                ENGINE_BLK(8026C78C);
                D_802E8BD8 = 1;
            }
            ENGINE_BLK(8026C798);
            if (w->unk8 & 2) {
                ENGINE_BLK(8026C7AC);
                func_80261570(0.0f);
            }
            ENGINE_BLK(8026C7B8);
            if (w->unk8 & 0x100000) {
                ENGINE_BLK(8026C7D0);
                U2F(f, w->unk2, 8026C7E0);
                ENGINE_BLK(8026C7F0);
                D_8036BB28 = f;
            } else {
                s32 h;

                ENGINE_BLK(8026C7FC);
                SDIV(h, w->unk2, 2, 8026C82C);
                ENGINE_BLK(8026C834);
                f = (f32)(h - D_8036BB10[w->unk18].y);
                D_8036BB28 = f;
                if (w->unk8 & 0x40000) {
                    ENGINE_BLK(8026C85C);
                    SDIV(h, D_8036BB10[w->unk18].unk8, 2, 8026C87C);
                    ENGINE_BLK(8026C884);
                    D_8036BB28 = f - (f32)h;
                }
            }
            ENGINE_BLK(8026C898);
            D_8036BB2C = D_8036BB28;
            break;
        case 4:
            ENGINE_BLK(8026C448);
            ENGINE_BLK(8026C450);
            ENGINE_BLK(8026C8AC);
            {
                f32 p = (D_8036BB38 - sp128) * D_8036BB08;

                U2F(f, D_803156C4, 8026C8DC);
                ENGINE_BLK(8026C8EC);
                IDO_CVT_U_S(u, p + f, 8026C91C, 8026C94C, 8026C95C, 8026C964);
            }
            ENGINE_BLK(8026C974);
            D_8036BAFC = u;
            goto case2;
        case 2:
            ENGINE_BLK(8026C448);
        case2:
            ENGINE_BLK(8026C980);
            D_8036BB1C = 8;
            sp13A = w->unk14;
            if (sp13A != 0) {
                ENGINE_BLK(8026C99C);
                func_80260650(D_80367738, sp13A, 0);
            }
            ENGINE_BLK(8026C9B0);
            if (w->unk8 & 0x200000) {
                ENGINE_BLK(8026C9C8);
                D_802E8BD4 = 1;
            }
            ENGINE_BLK(8026C9D4);
            if (w->unk8 & 4) {
                ENGINE_BLK(8026C9E8);
                func_80261570(1.0f);
            }
            break;
        default:
            ENGINE_BLK(8026C448);
            ENGINE_BLK(8026C450);
            ENGINE_BLK(8026C458);
            break;
        }
    }
    ENGINE_BLK(8026C9F4);
    switch (D_8036BB1C) {
    case 2:
        ENGINE_BLK(8026CA20);
        if (w->unk8 & 0x100000) {
            s32 h;

            ENGINE_BLK(8026CA38);
            t = D_8036BB10[w->first + w->count - 1].y;
            SDIV(h, w->unk2, 8, 8026CA6C);
            ENGINE_BLK(8026CA74);
            sp11F = 0;
            if (D_8036BB2C < (f32)(h - t)) {
                ENGINE_BLK(8026CA9C);
                sp11F = 1;
            }
            ENGINE_BLK(8026CAA0);
        } else {
            s32 s1;

            ENGINE_BLK(8026CAA8);
            s1 = w->unkC != 0;
            if (s1) {
                f32 q, c;

                ENGINE_BLK(8026CAC0);
                U2F(f, D_803156C4 - D_8036BAFC, 8026CAE0);
                ENGINE_BLK(8026CAF0);
                q = f / 60.0f;
                U2F(c, w->unkC, 8026CB0C);
                ENGINE_BLK(8026CB1C);
                s1 = 0;
                if (c < q) {
                    ENGINE_BLK(8026CB2C);
                    s1 = 1;
                }
                ENGINE_BLK(8026CB30);
                if (s1) {
                    ENGINE_BLK(8026CB38);
                    s1 = !(w->unk8 & 0x400000);
                    if (!s1) {
                        ENGINE_BLK(8026CB58);
                        s1 = !(D_8036BB1E != 0);
                    }
                }
            }
            ENGINE_BLK(8026CB6C);
            sp11F = s1;
        }
        ENGINE_BLK(8026CB70);
        if (sp11F != 0) {
            ENGINE_BLK(8026CB7C);
            sp13A = w->unk14;
            if (sp13A != 0) {
                ENGINE_BLK(8026CB8C);
                func_80260650(D_80367738, sp13A, 0);
            }
            ENGINE_BLK(8026CBA0);
            if (w->unk8 & 0x2000) {
                ENGINE_BLK(8026CBB4);
                func_80261570(0.0f);
            }
            ENGINE_BLK(8026CBC0);
            D_8036BB1C = 8;
            D_8036BAFC = D_803156C4;
        }
        break;
    case 4:
        ENGINE_BLK(8026CA08);
        ENGINE_BLK(8026CBE0);
        U2F(f, D_803156C4 - D_8036BAFC, 8026CC00);
        ENGINE_BLK(8026CC10);
        D_8036BB38 = f / D_8036BB08;
        if (sp128 < D_8036BB38) {
            ENGINE_BLK(8026CC34);
            D_8036BAFC = D_803156C4;
            D_8036BB1C = 2;
            D_8036BB38 = sp128;
            if (w->unk8 & 0x40) {
                ENGINE_BLK(8026CC60);
                D_8036BB3C = 0x200;
                D_8036BB3E = 0x100;
            } else {
                ENGINE_BLK(8026CC7C);
                D_8036BB3C = 0x800;
                D_8036BB3E = 0x400;
            }
            ENGINE_BLK(8026CC94);
            if (w->unk8 & 0x10000000) {
                ENGINE_BLK(8026CCAC);
                w->unk1A = 1;
            } else {
                ENGINE_BLK(8026CCB8);
                w->unk1A = 0;
            }
            ENGINE_BLK(8026CCC0);
            t = func_8026F8A8(w->first, w->count, w->unk18, 1);
            ENGINE_BLK(8026CCD8);
            if (t == w->unk18) {
                ENGINE_BLK(8026CCE8);
                w->unk1A = 1;
            }
        }
        break;
    case 8:
        ENGINE_BLK(8026CA08);
        ENGINE_BLK(8026CA10);
        ENGINE_BLK(8026CCF4);
        U2F(f, D_803156C4 - D_8036BAFC, 8026CD14);
        ENGINE_BLK(8026CD24);
        D_8036BB38 = sp128 - f / D_8036BB08;
        if ((f64)D_8036BB38 < 0.001) {
            ENGINE_BLK(8026CD5C);
            D_8036BB38 = 0.0f;
            D_8036BB1C = 1;
            if (w->unk8 & 0x2000000) {
                ENGINE_BLK(8026CD8C);
                D_802E8BD4 = 1;
            }
            ENGINE_BLK(8026CD98);
            if (w->unk8 & 0x100) {
                ENGINE_BLK(8026CDAC);
                w->unk8 &= ~0x80;
            }
            ENGINE_BLK(8026CDB4);
            D_8036BB18 = -1;
            ENGINE_BLK(8026F094);
            return arg0;
        }
        break;
    default:
        ENGINE_BLK(8026CA08);
        ENGINE_BLK(8026CA10);
        ENGINE_BLK(8026CA18);
        break;
    }
    ENGINE_BLK(8026CDC8);
    if (D_8036BB1C != 1) {
        f32 s;

        ENGINE_BLK(8026CDDC);
        s = func_802574F0((f32)((f64)(D_8036BB38 * D_8036BB34 / sp128) * 1.57 + 4.71));
        ENGINE_BLK(8026CE18);
        D_8036BB20 = engine_trunc_w_d(((f64)s + 1.0) * 255.0);
    }
    ENGINE_BLK(8026CE4C);
    if (D_8036BB1C == 1 || (ENGINE_BLK(8026CE60), !(0.1 < (f64)(D_8036BB38 * D_8036BB34)))) {
        ENGINE_BLK(8026F090);
        ENGINE_BLK(8026F094);
        return gfx;
    }

    ENGINE_BLK(8026CE8C);
    SDIV(sp136, w->unk0, 2, 8026CEA4);
    ENGINE_BLK(8026CEAC);
    SDIV(sp134, w->unk2, 2, 8026CEC4);
    ENGINE_BLK(8026CECC);
    guOrtho(&arg1->mtx[73], -w->unk4 - sp136, -w->unk4 - sp136 + 319, -w->unk6 - sp134 + 239,
            -w->unk6 - sp134, -256.0f, 256.0f, 256.0f);
    ENGINE_BLK(8026CF60);
    gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[73]), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    if (w->unk8 & 0x10) {
        ENGINE_BLK(8026CFAC);
        guRotate(&arg1->mtx[75], 180.0 - (f64)(D_8036BB38 * D_8036BB34 / sp128) * 180.0, 2.0f, 0.0f, 1.0f);
        ENGINE_BLK(8026D010);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[75]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    } else {
        ENGINE_BLK(8026D04C);
        guTranslate(&arg1->mtx[75], 0.0f, 0.0f, 0.0f);
        ENGINE_BLK(8026D064);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[75]), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    }
    ENGINE_BLK(8026D09C);
    gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    if (w->unk8 & 8) {
        f32 x, y;

        ENGINE_BLK(8026D10C);
        U2F(f, sp136, 8026D124);
        ENGINE_BLK(8026D134);
        x = f * D_8036BB38 * D_8036BB34 / 1000.0f;
        U2F(f, sp134, 8026D170);
        ENGINE_BLK(8026D17C);
        y = f * D_8036BB38 * D_8036BB34 / 1000.0f;
        guScale(&arg1->mtx[76], x, y, 1.0f);
        ENGINE_BLK(8026D1A0);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    } else {
        f32 x;

        ENGINE_BLK(8026D1DC);
        U2F(f, sp136, 8026D1F4);
        ENGINE_BLK(8026D204);
        x = f / 1000.0f;
        U2F(f, sp134, 8026D22C);
        ENGINE_BLK(8026D238);
        guScale(&arg1->mtx[76], x, f / 1000.0f, 1.0f);
        ENGINE_BLK(8026D254);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[76]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    }
    ENGINE_BLK(8026D28C);
    gDPPipeSync(gfx++);
    gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
    gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    if (!(w->unk8 & 0x200)) {
        ENGINE_BLK(8026D39C);
        gSPDisplayList(gfx++, D_802F98B0);
    }
    ENGINE_BLK(8026D3C8);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    if (w->unk8 & 8) {
        ENGINE_BLK(8026D400);
        guScale(&arg1->mtx[74], D_8036BB38, D_8036BB38, 1.0f);
        ENGINE_BLK(8026D41C);
        gSPMatrix(gfx++, OS_K0_TO_PHYSICAL(&arg1->mtx[74]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    }

    /* the selection's moves */
    ENGINE_BLK(8026D454);
    if ((w->unk8 & 0x20) && (ENGINE_BLK(8026D468), D_8036BB1C == 2)) {
        s32 spD8, spD4;
        u16 spD2, spD0;
        u16 m;

        ENGINE_BLK(8026D47C);
        if (D_8036BB3C == 0x800) {
            ENGINE_BLK(8026D494);
            if (D_80370C12 >= 31 && (ENGINE_BLK(8026D4A8), D_80370C14 < 31)) {
                ENGINE_BLK(8026D4BC);
                spD8 = 1;
            } else {
                ENGINE_BLK(8026D4C8);
                spD8 = 0;
            }
        } else {
            ENGINE_BLK(8026D4D0);
            if (D_80370C11 < -30 && (ENGINE_BLK(8026D4E4), D_80370C13 >= -30)) {
                ENGINE_BLK(8026D4F8);
                spD8 = 1;
            } else {
                ENGINE_BLK(8026D504);
                spD8 = 0;
            }
        }
        ENGINE_BLK(8026D508);
        if (D_8036BB3E == 0x400) {
            ENGINE_BLK(8026D51C);
            if (D_80370C12 < -30 && (ENGINE_BLK(8026D530), D_80370C14 >= -30)) {
                ENGINE_BLK(8026D544);
                spD4 = 1;
            } else {
                ENGINE_BLK(8026D550);
                spD4 = 0;
            }
        } else {
            ENGINE_BLK(8026D558);
            if (D_80370C11 >= 31 && (ENGINE_BLK(8026D56C), D_80370C13 < 31)) {
                ENGINE_BLK(8026D580);
                spD4 = 1;
            } else {
                ENGINE_BLK(8026D58C);
                spD4 = 0;
            }
        }
        ENGINE_BLK(8026D590);
        if (((D_80370C28 & D_8036BB3C) && (ENGINE_BLK(8026D5AC), !(D_80370C2A & D_8036BB3C))) ||
            (ENGINE_BLK(8026D5C0), spD8 != 0)) {
            ENGINE_BLK(8026D5CC);
            spD2 = func_8026F82C(w->first, w->unk18, 1);
            ENGINE_BLK(8026D5E0);
            sp13A = w->unk16;
            if (sp13A != 0) {
                ENGINE_BLK(8026D5F4);
                if (spD2 != w->unk18) {
                    ENGINE_BLK(8026D604);
                    func_80260650(D_80367738, sp13A, 0);
                    ENGINE_BLK(8026D618);
                } else {
                    ENGINE_BLK(8026D620);
                    func_80260650(D_80367738, 0xD0, 0);
                }
            }
            ENGINE_BLK(8026D634);
            D_8036BB28 += (f32)(D_8036BB10[w->unk18].y - D_8036BB10[spD2].y);
            w->unk18 = spD2;
        } else {
            ENGINE_BLK(8026D698);
            if (w->unk1A != 0) {
                ENGINE_BLK(8026D6A8);
                m = 0;
            } else {
                ENGINE_BLK(8026D6B0);
                m = 0x8000;
            }
            ENGINE_BLK(8026D6B4);
            if ((m | D_8036BB3E) & D_80370C28) {
                ENGINE_BLK(8026D6D4);
                if (w->unk1A != 0) {
                    ENGINE_BLK(8026D6E4);
                    m = 0;
                } else {
                    ENGINE_BLK(8026D6EC);
                    m = 0x8000;
                }
                ENGINE_BLK(8026D6F0);
                if (!((m | D_8036BB3E) & D_80370C2A))
                    goto down;
            }
            ENGINE_BLK(8026D710);
            if (spD4 != 0) {
            down:
                ENGINE_BLK(8026D71C);
                spD0 = func_8026F8A8(w->first, w->count, w->unk18, 1);
                ENGINE_BLK(8026D734);
                t = func_8026F8A8(w->first, w->count, spD0, 1);
                ENGINE_BLK(8026D750);
                if (t == spD0) {
                    ENGINE_BLK(8026D75C);
                    w->unk1A = 1;
                }
                ENGINE_BLK(8026D768);
                sp13A = w->unk16;
                if (sp13A != 0) {
                    ENGINE_BLK(8026D778);
                    if (spD0 != w->unk18) {
                        ENGINE_BLK(8026D788);
                        func_80260650(D_80367738, sp13A, 0);
                        ENGINE_BLK(8026D79C);
                    } else {
                        ENGINE_BLK(8026D7A4);
                        func_80260650(D_80367738, 0xD0, 0);
                    }
                }
                ENGINE_BLK(8026D7B8);
                D_8036BB28 += (f32)(D_8036BB10[w->unk18].y - D_8036BB10[spD0].y);
                w->unk18 = spD0;
                D_8036BAFC = D_803156C4;
            }
        }
    }

    /* the scrolling, and the arrows */
    ENGINE_BLK(8026D824);
    if (w->unk8 & 0x4000) {
        ENGINE_BLK(8026D838);
        if (w->unk8 & 0x100000) {
            s32 hold = 0;

            ENGINE_BLK(8026D844);
            if (w->unkC != 0) {
                f32 q, c;

                ENGINE_BLK(8026D850);
                U2F(f, D_803156C4 - D_8036BAFC, 8026D870);
                ENGINE_BLK(8026D880);
                q = f / 60.0f;
                U2F(c, w->unkC, 8026D898);
                ENGINE_BLK(8026D8A8);
                hold = q < c;
            }
            if (!hold) {
                ENGINE_BLK(8026D8B8);
                if (w->unk8 & 0x800000) {
                    ENGINE_BLK(8026D8D0);
                    D_8036BB2C = (f64)D_8036BB2C - 0.5;
                } else {
                    ENGINE_BLK(8026D8F8);
                    D_8036BB2C = (f64)D_8036BB2C - 1.0;
                }
            }
        } else {
            ENGINE_BLK(8026D924);
            D_8036BB2C = (f64)D_8036BB2C + (f64)(D_8036BB28 - D_8036BB2C) * 0.1;
        }
        ENGINE_BLK(8026D95C);
        if (w->unk8 & 0x10000) {
            s32 spCC;
            s16 spCA;
            s32 h, h2;
            ColorPair *c;

            ENGINE_BLK(8026D974);
            spCC = 0;
            if (w->unk8 & 0x40000) {
                ENGINE_BLK(8026D98C);
                spCA = 0x12;
            } else {
                ENGINE_BLK(8026D998);
                spCA = 0x1C;
            }
            ENGINE_BLK(8026D9A0);
            if (((u32)(D_80364A90 >> 32) & 0xC9FD0FE7) == 0 &&
                (ENGINE_BLK(8026D9CC), ((u32)D_80364A90 & 0x9BFF80B0) == 0)) {
                goto add;
            }
            ENGINE_BLK(8026D9D4);
            if (D_8035805C != 0) {
            add:
                ENGINE_BLK(8026D9E4);
                D_8036BB44 += D_802F9930;
            }
            ENGINE_BLK(8026DA00);
            if (D_802F9930 < 0) {
                ENGINE_BLK(8026DA10);
                D_8036BB44 += D_802F9930 * 2;
            }
            ENGINE_BLK(8026DA28);
            if (D_8036BB44 < 0 || (ENGINE_BLK(8026DA38), D_8036BB44 >= 8)) {
                ENGINE_BLK(8026DA40);
                D_8036BB44 -= D_802F9930 * 2;
                D_802F9930 = -D_802F9930;
            }
            ENGINE_BLK(8026DA6C);
            t = func_8026F8A8(w->first, w->count, w->unk18, 1);
            ENGINE_BLK(8026DA84);
            if (t != w->unk18) {
                ENGINE_BLK(8026DA94);
                c = &D_802F47B0[18];
                SDIV(h, D_8036BB44, 2, 8026DAEC);
                ENGINE_BLK(8026DAF4);
                spCC = func_80276130(arg1, 0, spCC, -sp136, sp134 - D_8036BB44 - spCA, 16, h + 10, c->r0, c->g0,
                                     c->b0, D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20, c->r0, c->g0, c->b0,
                                     D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20);
                ENGINE_BLK(8026DB6C);
                SDIV(h, D_8036BB44, 2, 8026DBC0);
                ENGINE_BLK(8026DBC8);
                SDIV(h2, D_8036BB20, 2, 8026DBE4);
                ENGINE_BLK(8026DBEC);
                spCC = func_80276080(arg1, 0, spCC, -3 - sp136, sp134 - D_8036BB44 - spCA + 3, 16, h + 10, 0, 0, 0,
                                     h2);
                ENGINE_BLK(8026DBF4);
                gfx = func_80275DA4(gfx, 1);
                ENGINE_BLK(8026DC04);
                gSPVertex(gfx++, arg1->vtx, 8, 0);
                gSP1Triangle(gfx++, 4, 5, 6, 0);
                gSP1Triangle(gfx++, 4, 6, 7, 0);
                gSP1Triangle(gfx++, 0, 1, 2, 0);
                gSP1Triangle(gfx++, 0, 2, 3, 0);
            }
            ENGINE_BLK(8026DCE0);
            t = func_8026F82C(w->first, w->unk18, 1);
            ENGINE_BLK(8026DCF4);
            if (t != w->unk18) {
                ENGINE_BLK(8026DD04);
                c = &D_802F47B0[18];
                SDIV(h, D_8036BB44, 2, 8026DD5C);
                ENGINE_BLK(8026DD64);
                spCC = func_80276130(arg1, 1, spCC, -sp136, D_8036BB44 - sp134 + spCA, 16, h + 10, c->r0, c->g0,
                                     c->b0, D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20, c->r0, c->g0, c->b0,
                                     D_8036BB20, c->r1, c->g1, c->b1, D_8036BB20);
                ENGINE_BLK(8026DDE0);
                SDIV(h, D_8036BB44, 2, 8026DE34);
                ENGINE_BLK(8026DE3C);
                SDIV(h2, D_8036BB20, 2, 8026DE58);
                ENGINE_BLK(8026DE60);
                spCC = func_80276080(arg1, 1, spCC, -3 - sp136, D_8036BB44 - sp134 + spCA - 3, 16, h + 10, 0, 0, 0,
                                     h2);
                ENGINE_BLK(8026DE68);
                gfx = func_80275DA4(gfx, 1);
                ENGINE_BLK(8026DE78);
                gSPVertex(gfx++, &arg1->vtx[spCC - 8], 8, 0);
                gSP1Triangle(gfx++, 4, 5, 6, 0);
                gSP1Triangle(gfx++, 4, 6, 7, 0);
                gSP1Triangle(gfx++, 0, 1, 2, 0);
                gSP1Triangle(gfx++, 0, 2, 3, 0);
            }
        }
    } else {
        ENGINE_BLK(8026DF64);
        D_8036BB2C = 0.0f;
    }

    /* the icons */
    ENGINE_BLK(8026DF70);
    D_8036BB30 = engine_trunc_w_s(D_8036BB2C);
    if (w->unk8 & 0x1000) {
        ENGINE_BLK(8026DFA0);
        if (w->unk8 & 0x20000) {
            ENGINE_BLK(8026DFAC);
            gfx = func_80274868(gfx);
            ENGINE_BLK(8026DFB4);
        } else {
            ENGINE_BLK(8026DFBC);
            gfx = func_80274998(gfx);
            ENGINE_BLK(8026DFC4);
        }
        ENGINE_BLK(8026DFC8);
        i = w->first;
        if (i < w->first + w->count) {
            do {
                YoshiIcon *icon;
                s16 sp92, sp90;
                u8 sp8F, sp8E, sp8D, sp8C, sp8B;
                s32 s1;
                u8 v;
                u32 tick, q;

                ENGINE_BLK(8026DFEC);
                e = &D_8036BB10[i];
                if (e->flags & 0x800)
                    goto next_icon;
                ENGINE_BLK(8026E01C);
                if (!(e->flags & 0x400))
                    goto next_icon;
                ENGINE_BLK(8026E028);
                if (e->flags & 0x300) {
                    ENGINE_BLK(8026E034);
                    if (D_8036BB04 < i)
                        goto next_icon;
                }
                ENGINE_BLK(8026E048);
                icon = &D_802F49F4[e->unk14];
                if (w->unk8 & 0x20000) {
                    ENGINE_BLK(8026E084);
                    sp92 = w->unk4;
                } else {
                    ENGINE_BLK(8026E090);
                    sp92 = -sp136;
                }
                ENGINE_BLK(8026E09C);
                if (w->unk8 & 0x20000) {
                    ENGINE_BLK(8026E0B4);
                    sp90 = w->unk6;
                } else {
                    ENGINE_BLK(8026E0C0);
                    sp90 = -sp134;
                }
                ENGINE_BLK(8026E0CC);
                sp8F = 1;
                sp8E = icon->unk25;
                if (e->flags & 1) {
                    ENGINE_BLK(8026E0F4);
                    if (i != w->unk18) {
                        ENGINE_BLK(8026E108);
                        sp8F = 0;
                    }
                }
                ENGINE_BLK(8026E10C);
                tick = D_803156C4 * 60 / 60;
                sp8D = D_8036BA48[e->unk14];
                if (icon->unk26 == 0) {
                    ENGINE_BLK(8026E16C);
                    engine_break(0x8026E16C, 7);
                }
                ENGINE_BLK(8026E170);
                q = tick / icon->unk26;
                if (icon->unk1A == 0) {
                    ENGINE_BLK(8026E198);
                    engine_break(0x8026E198, 7);
                }
                sp8C = D_8036BA48[e->unk14] = q % icon->unk1A;
                ENGINE_BLK(8026E19C);
                if (sp8C != sp8D) {
                    ENGINE_BLK(8026E1A4);
                    if (sp8F != 0 || (ENGINE_BLK(8026E1B0), D_8036BA98[e->unk14] != 0)) {
                        s32 n, d, r;

                        ENGINE_BLK(8026E1CC);
                        n = D_8036BA98[e->unk14] + 1;
                        d = icon->unk1A;
                        ENGINE_DIV(r, n, d, 8026E204, 8026E208, 8026E214, 8026E21C);
                        (void)r;
                        D_8036BA98[e->unk14] = n % d;
                    }
                }
                ENGINE_BLK(8026E220);
                sp8B = icon->unk1B[D_8036BA98[e->unk14]];
                if (sp8B == 0)
                    goto next_icon;
                ENGINE_BLK(8026E248);
                if (sp8F != 0) {
                    ENGINE_BLK(8026E254);
                    if (i == w->unk18) {
                        ENGINE_BLK(8026E268);
                        if (e->flags & 0x40) {
                            ENGINE_BLK(8026E278);
                            sp8E |= 8;
                        }
                    }
                    ENGINE_BLK(8026E284);
                    if (e->flags & 0x1000) {
                        ENGINE_BLK(8026E298);
                        s1 = D_8036BB30;
                    } else {
                        ENGINE_BLK(8026E2A4);
                        s1 = 0;
                    }
                    ENGINE_BLK(8026E2A8);
                    v = func_8026F644((UnkStruct_8026F644 *) w, &e->flags,
                                      e->y + icon->unk2 - sp134 + D_8036BB30 + 8);
                    ENGINE_BLK(8026E2E0);
                    U2F(f, v, 8026E338);
                    ENGINE_BLK(8026E348);
                    IDO_CVT_U_S(u, f * D_8036BB38 * D_8036BB34, 8026E38C, 8026E3BC, 8026E3CC, 8026E3D4);
                    ENGINE_BLK(8026E3E4);
                    gfx = func_80272ED8(gfx, e->unk1A + sp8B - 1, icon->unk0 + e->x + sp92,
                                        s1 + (icon->unk2 + e->y + sp90), u, sp8E, icon->unk28);
                    ENGINE_BLK(8026E400);
                } else {
                    ENGINE_BLK(8026E408);
                    if (e->flags & 0x1000) {
                        ENGINE_BLK(8026E41C);
                        s1 = D_8036BB30;
                    } else {
                        ENGINE_BLK(8026E428);
                        s1 = 0;
                    }
                    ENGINE_BLK(8026E42C);
                    v = func_8026F644((UnkStruct_8026F644 *) w, &e->flags,
                                      e->y + icon->unk2 - sp134 + D_8036BB30 + 8);
                    ENGINE_BLK(8026E464);
                    U2F(f, v, 8026E4BC);
                    ENGINE_BLK(8026E4CC);
                    IDO_CVT_U_D(u, (f64)(f * D_8036BB38 * D_8036BB34) * 0.7, 8026E520, 8026E554, 8026E564,
                                8026E56C);
                    ENGINE_BLK(8026E57C);
                    gfx = func_80272ED8(gfx, e->unk1A + sp8B - 1, icon->unk0 + e->x + sp92,
                                        s1 + (icon->unk2 + e->y + sp90), u, sp8E & ~1, icon->unk28);
                    ENGINE_BLK(8026E5A0);
                }
            next_icon:
                ENGINE_BLK(8026E5A4);
                i++;
            } while (i < w->first + w->count);
        }
        ENGINE_BLK(8026E5D0);
        if (w->unk8 & 0x20000) {
            ENGINE_BLK(8026E5E8);
            gfx = func_80274AA4(gfx);
            ENGINE_BLK(8026E5F0);
        } else {
            ENGINE_BLK(8026E5F8);
            gfx = func_80274B08(gfx);
            ENGINE_BLK(8026E600);
        }
    }

    /* the selection's shadow */
    ENGINE_BLK(8026E604);
    if (D_8036BB18 < 0x62 || (ENGINE_BLK(8026E618), D_8036BB18 >= 0x6C) ||
        (ENGINE_BLK(8026E620), (u32)(D_80364A90 >> 32) == 0 &&
                                   (ENGINE_BLK(8026E638), (u32)D_80364A90 == 2))) {
        ENGINE_BLK(8026E640);
        i = w->first;
        if (i < w->first + w->count) {
            do {
                s32 s1;
                u8 s3, s0;
                s32 a, b;

                ENGINE_BLK(8026E664);
                e = &D_8036BB10[i];
                sp140 = NULL;
                if (e->unk10 != NULL && (ENGINE_BLK(8026E694), *e->unk10 != 0xFFF)) {
                    ENGINE_BLK(8026E6A4);
                    sp140 = func_8026F004(w, i, 1);
                    ENGINE_BLK(8026E6B4);
                } else {
                    ENGINE_BLK(8026E6BC);
                    sp144 = func_8026F004(w, i, 0);
                    ENGINE_BLK(8026E6CC);
                }
                ENGINE_BLK(8026E6D0);
                if (!(e->flags & 0x80))
                    goto next_shadow;
                ENGINE_BLK(8026E6E4);
                if (e->flags & 0x800)
                    goto next_shadow;
                ENGINE_BLK(8026E6F0);
                if (i == w->unk18) {
                    ENGINE_BLK(8026E704);
                    if (e->flags & 0x1000) {
                        ENGINE_BLK(8026E710);
                        s1 = D_8036BB30;
                    } else {
                        ENGINE_BLK(8026E71C);
                        s1 = 0;
                    }
                    ENGINE_BLK(8026E720);
                    s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                    ENGINE_BLK(8026E748);
                    s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                    ENGINE_BLK(8026E77C);
                    SDIV(a, alpha_of(D_8036BB20, D_802F47B0[e->unk19].a0, s3), 2, 8026E850);
                    ENGINE_BLK(8026E858);
                    SDIV(b, alpha_of(D_8036BB20, D_802F47B0[e->unk19].a0, s0), 2, 8026E8A0);
                    ENGINE_BLK(8026E8A8);
                    func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136 - 3, s1 + (e->y - sp134) + 3,
                                  e->unk6, e->unk8, 1, 0, 0, 0, a, 0, 0, 0, b);
                    ENGINE_BLK(8026E8B0);
                } else {
                    ENGINE_BLK(8026E8B8);
                    if (e->flags & 4) {
                        ENGINE_BLK(8026E8CC);
                        if (!(D_803156C4 % 23 * 60 / 60 < 16))
                            goto next_shadow;
                    }
                    ENGINE_BLK(8026E904);
                    if (e->flags & 0x1000) {
                        ENGINE_BLK(8026E918);
                        s1 = D_8036BB30;
                    } else {
                        ENGINE_BLK(8026E924);
                        s1 = 0;
                    }
                    ENGINE_BLK(8026E928);
                    s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                    ENGINE_BLK(8026E950);
                    s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                    ENGINE_BLK(8026E984);
                    SDIV(a, alpha_of(D_8036BB20, D_802F47B0[e->unk18].a0, s3), 2, 8026EA58);
                    ENGINE_BLK(8026EA60);
                    SDIV(b, alpha_of(D_8036BB20, D_802F47B0[e->unk18].a0, s0), 2, 8026EAA8);
                    ENGINE_BLK(8026EAB0);
                    func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136 - 3, s1 + (e->y - sp134) + 3,
                                  e->unk6, e->unk8, 1, 0, 0, 0, a, 0, 0, 0, b);
                }
            next_shadow:
                ENGINE_BLK(8026EAB8);
                i++;
            } while (i < w->first + w->count);
        }
    }

    /* the text */
    ENGINE_BLK(8026EAE4);
    i = w->first;
    if (i < w->first + w->count) {
        do {
            s32 s1;
            u8 s3, s0;
            ColorPair *c;

            ENGINE_BLK(8026EB08);
            e = &D_8036BB10[i];
            sp140 = NULL;
            if (e->unk10 != NULL && (ENGINE_BLK(8026EB38), *e->unk10 != 0xFFF)) {
                ENGINE_BLK(8026EB48);
                sp140 = func_8026F004(w, i, 1);
                ENGINE_BLK(8026EB58);
            } else {
                ENGINE_BLK(8026EB60);
                sp144 = func_8026F004(w, i, 0);
                ENGINE_BLK(8026EB70);
            }
            ENGINE_BLK(8026EB74);
            if (e->flags & 0x800)
                goto next_text;
            ENGINE_BLK(8026EB88);
            if (i == w->unk18) {
                ENGINE_BLK(8026EB9C);
                if (e->flags & 4) {
                    ENGINE_BLK(8026EBA8);
                    if (!(D_803156C4 % 23 * 60 / 60 < 16))
                        goto next_text;
                }
                ENGINE_BLK(8026EBE0);
                if (e->flags & 0x40) {
                    ENGINE_BLK(8026EBF4);
                    if (!(D_803156C4 % 15 * 60 / 60 < 11))
                        goto next_text;
                }
                ENGINE_BLK(8026EC2C);
                if (e->flags & 0x1000) {
                    ENGINE_BLK(8026EC40);
                    s1 = D_8036BB30;
                } else {
                    ENGINE_BLK(8026EC4C);
                    s1 = 0;
                }
                ENGINE_BLK(8026EC50);
                s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                ENGINE_BLK(8026EC78);
                s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                ENGINE_BLK(8026ECAC);
                c = &D_802F47B0[e->unk19];
                func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136, s1 + (e->y - sp134), e->unk6,
                              e->unk8, 1, c->r0, c->g0, c->b0, alpha_of(D_8036BB20, c->a0, s3), c->r1, c->g1, c->b1,
                              alpha_of(D_8036BB20, c->a1, s0));
                ENGINE_BLK(8026EE18);
            } else {
                ENGINE_BLK(8026EE20);
                if (e->flags & 4) {
                    ENGINE_BLK(8026EE34);
                    if (!(D_803156C4 % 23 * 60 / 60 < 16))
                        goto next_text;
                }
                ENGINE_BLK(8026EE6C);
                if (e->flags & 0x1000) {
                    ENGINE_BLK(8026EE80);
                    s1 = D_8036BB30;
                } else {
                    ENGINE_BLK(8026EE8C);
                    s1 = 0;
                }
                ENGINE_BLK(8026EE90);
                s3 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30);
                ENGINE_BLK(8026EEB8);
                s0 = func_8026F644((UnkStruct_8026F644 *) w, &e->flags, e->y - sp134 + D_8036BB30 + e->unk8);
                ENGINE_BLK(8026EEEC);
                c = &D_802F47B0[e->unk18];
                func_80259DC8(arg1, sp144, sp140, e->flags & 8, 0, e->x - sp136, s1 + (e->y - sp134), e->unk6,
                              e->unk8, 1, c->r0, c->g0, c->b0, alpha_of(D_8036BB20, c->a0, s3), c->r1, c->g1, c->b1,
                              alpha_of(D_8036BB20, c->a1, s0));
            }
        next_text:
            ENGINE_BLK(8026F058);
            i++;
        } while (i < w->first + w->count);
    }
    ENGINE_BLK(8026F084);
    func_80259BD4(&gfx, arg1);
    ENGINE_BLK(8026F090);
    ENGINE_BLK(8026F094);
    return gfx;
}

#endif
