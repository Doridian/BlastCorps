/*
 * The object after stats.c (7800.c): jp pads 7800's .text to 16 before
 * func_801EF380, and the double at 0x8020EFB0 after 7800's .rodata (and its
 * padding) is this object's, as are the .bss variables only it uses.
 */
#include "common.h"
#include "game/frontend.h"
#include "game/frame.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

/* Per-frame buffer, double-buffered by D_8035805C. */

extern Gfx D_01000010[];
extern Gfx D_01000038[];
extern s32 D_802FA268;
extern s32 D_80358078;
extern u16 D_8035807C;
extern char D_8036B980[];
extern char D_8036B9A8[];

/* .bss, 0x802159D0-0x802159F0 (tools/bss_c.py) */
u16 D_802159D0;
u8 *D_802159D4;
u8 *D_802159D8;
u16 D_802159DC;
f32 D_802159E0;
#ifdef VERSION_EU
u8 D_8021AED4_eu;
u8 D_8021AED5_eu;
f32 D_802159E4;
u8 D_802159E8[4];
#else
f32 D_802159E4;
u8 D_802159E8[8];
#endif

extern u8 D_0048F5A0[];
extern u8 D_0048F970[];
extern u8 D_0048F970_2[]; /* same address as D_0048F970: see undefined_syms */
extern u8 D_0048FA70[];

#ifdef VERSION_EU
extern s32 D_802FA250;
#endif

void func_801EF380(s32 arg0) {
    u32 sp24;
    u32 sp20;
#ifdef VERSION_EU
    register s32 r;
#endif

    sp24 = ROM(D_0048F970) - ROM(D_0048F5A0);
    sp20 = ROM(D_0048FA70) - ROM(D_0048F970_2);
    func_801F4E70(arg0);
    if (arg0 == 2) {
        D_802159D0 = 90;
    } else {
        D_802159D0 = 0;
    }
    func_8028B4C4(ROM(D_0048F5A0), D_80358070, &sp24, 12, 0, 1);
    D_802159D4 = D_80358070;
    D_80358070 += sp24;
    func_8028B4C4(ROM(D_0048F970), D_80358070, &sp20, 12, 0, 1);
    D_802159D8 = D_80358070;
    D_80358070 += sp20;
    D_802159DC = arg0;
    D_802159E0 = 0.0f;
#ifdef VERSION_EU
    if (D_80364A98 == 0x10) {
        r = D_802FA268;
        r = r == 0;
        D_8021AED4_eu = r;
        r = D_802FA250;
        r = r == 0;
        D_8021AED5_eu = r;
    }
    D_802159E4 = 3.6f;
#else
    D_802159E4 = 3.0f;
#endif
}

/* The intro's frame numbers: eu's are PAL's (50 a second, not 60). */
#ifdef VERSION_EU
#define INTRO_FRAME(us, eu) (eu)
#else
#define INTRO_FRAME(us, eu) (us)
#endif

void func_801EF4AC(void) {
    Frame *sp12C;
    Gfx *gfx;
    s32 sp124;
    s32 sp120;
    s32 sp11C;
    s16 sp11A;

    sp12C = &D_803156F8[D_8035805C ^ 1];
    gfx = sp12C->buf.dl;
    func_8028A470();
#ifdef TARGET_PC
    /* A, B or Start goes to the logo's fade-out (with the camera where it
       ends up); the pad's first frames are masked (45BB0.c) */
    if (D_80358060 >= 2 && D_80358060 < INTRO_FRAME(221, 183) && (D_80370C28 & ~D_80370C2A & 0xD000) && port_intro_skip()) {
        D_80358060 = INTRO_FRAME(221, 183);
        D_802159E0 = 400.0f;
    }
#endif
    func_80284E54(D_803156F8[D_8035805C].buf.dl, D_80358078, 1, 1, 0x4D2, 0);
    D_8035805C ^= 1;
    gSPSegment(gfx++, 0, 0);
    gSPSegment(gfx++, 2, osVirtualToPhysical(sp12C));
    gSPSegment(gfx++, 1, osVirtualToPhysical(D_8035806C));
    gSPDisplayList(gfx++, D_01000038);
    gSPDisplayList(gfx++, D_01000010);
    gDPSetCycleType(gfx++, G_CYC_FILL);
    gDPSetDepthImage(gfx++, D_80358058);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358058);
    gDPSetFillColor(gfx++, 0xFFFCFFFC);
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_80358050[D_8035805C]);
    gDPPipeSync(gfx++);
    if (D_802159DC == 1) {
        if (D_80358060 * 185 / INTRO_FRAME(20, 17) > 185) {
            sp11A = 185;
        } else {
            sp11A = D_80358060 * 185 / INTRO_FRAME(20, 17);
        }
    } else {
        sp11A = 0;
    }
    gDPSetFillColor(gfx++, (GPACK_RGBA5551(sp11A, sp11A, sp11A, 1) << 16) | GPACK_RGBA5551(sp11A, sp11A, sp11A, 1));
    gDPFillRectangle(gfx++, 0, 0, 319, 239);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    func_8028A3E4();
#ifdef VERSION_EU
    if (D_8021AED4_eu || D_8021AED5_eu) {
        if (D_80364A90 == 0x10 && D_80370C28 == 0x2030) {
            D_802FA250 = 1;
        }
        if (D_80364A90 == 0x20 && D_80370C28 != 0x6200 && D_80358060 >= 11) {
            if (D_8021AED4_eu) {
                D_802FA268 = 0;
            }
            if (D_8021AED5_eu) {
                D_802FA250 = 0;
            }
        }
    }
#endif
    if (D_80358060 == INTRO_FRAME(250, 208)) {
        if (D_80364A90 == 0x10) {
            D_80364A98 = 0x20;
        } else {
            D_80364A98 = 0x0400000000000000;
            if (D_802FA268 != 0) {
                func_80260650(D_80367738, 0x68, 0);
            }
        }
    }
    if (D_80358060 < 2) {
        guPerspective(&sp12C->buf.mtx[73], &D_8035807C, 45.0f, 4.0f / 3.0f, 40.0f, 8000.0f, 0.25f);
        if (D_802159DC == 1) {
            guTranslate(&sp12C->buf.mtx[74], 0.0f, -130.0f, 0.0f);
            guRotate(&sp12C->buf.mtx[75], 35.0f, 0.1f, 0.0f, 0.0f);
        } else {
            guTranslate(&sp12C->buf.mtx[74], 0.0f, 0.0f, 0.0f);
            guRotate(&sp12C->buf.mtx[75], -10.0f, 0.1f, 0.0f, 0.0f);
        }
    }
    if (D_80358060 >= INTRO_FRAME(20, 17)) {
        if (D_80358060 == INTRO_FRAME(20, 17) && D_802159DC == 1) {
            func_80260650(D_80367738, 0xBA, 0);
        } else if (D_80358060 == INTRO_FRAME(20, 17) && D_802159DC == 2) {
            func_80260650(D_80367738, 0xBD, 0);
        }
        if (D_80358060 < INTRO_FRAME(80, 67)) {
            D_802159E0 = (INTRO_FRAME(80, 67) - D_80358060) * 7600.0 / INTRO_FRAME(60.0, 50.0) + 400.0;
        }
        if (D_80358060 == INTRO_FRAME(75, 63) && D_802159DC == 2) {
            func_80260650(D_80367738, 0xB8, 0);
        } else if (D_80358060 == INTRO_FRAME(75, 63) && D_802159DC == 1) {
            func_80260650(D_80367738, 0xBB, 0);
        }
        guLookAtReflect(&sp12C->buf.mtx[5], &sp12C->buf.lookAt, 1.0f, 0.0f, D_802159E0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        D_802159D0 += D_802159E4;
        guRotate(&D_802182D0[D_8035805C], D_802159D0 % 360, 0.0f, 1.0f, 0.0f);
        guScale(&sp12C->buf.mtx[76], 1.5f, 1.5f, 1.5f);
        gDPSetRenderMode(gfx++, G_RM_AA_ZB_OPA_INTER, G_RM_NOOP2);
        gfx = func_801F4FBC(sp12C, gfx);
    }
    if (D_80358060 > INTRO_FRAME(80, 67) && D_802159DC == 1) {
        gDPPipeSync(gfx++);
        gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetTexturePersp(gfx++, G_TP_NONE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, 0x28, 0x00, 0xFF, (D_80358060 * 6 - INTRO_FRAME(480, 402) < 255) ? D_80358060 * 6 - INTRO_FRAME(480, 402) : 255);
        sp120 = 26, sp11C = 42;
        for (sp124 = 0; sp124 < 256; sp124 += 32) {
            gDPLoadTextureTile(gfx++, D_802159D4, G_IM_FMT_IA, G_IM_SIZ_8b, 256, 0, sp124, 0, sp124 + 31, 31, 0,
                               G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPTextureRectangle(gfx++, (sp124 + sp120) << 2, sp11C << 2, (sp124 + sp120 + 32) << 2, (sp11C + 32) << 2,
                                G_TX_RENDERTILE, sp124 << 5, 0, 1 << 10, 1 << 10);
        }
        gDPPipeSync(gfx++);
        gDPSetPrimColor(gfx++, 0, 0, 0xFF, 0x00, 0x28, (D_80358060 * 4 - INTRO_FRAME(320, 268) < 255) ? D_80358060 * 4 - INTRO_FRAME(320, 268) : 255);
        gDPLoadTextureBlock(gfx++, D_802159D8, G_IM_FMT_IA, G_IM_SIZ_8b, 40, 24, 0, G_TX_CLAMP, G_TX_CLAMP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 256 << 2, 20 << 2, 296 << 2, 44 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
        gDPSetTexturePersp(gfx++, G_TP_PERSP);
    }
    if (D_80358060 >= INTRO_FRAME(221, 183)) {
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, (D_80358060 - INTRO_FRAME(220, 182)) * 255 / INTRO_FRAME(30, 26));
        gDPFillRectangle(gfx++, 0, 0, 319, 239);
    }
    gDPFullSync(gfx++);
    gSPEndDisplayList(gfx++);
    D_80358078 = gfx - sp12C->buf.dl;
}
