#include "common.h"
#include "game/game.h"
#include "game/frame.h"
#include "functions.h"

/*
 * gen_symbols matched these to libultra's osViExtendVStart and its
 * __additional_scanline, but they are the game's own: D_8021AB80 holds a
 * 320x240 RGBA16 image that func_80200BE0 draws.
 */
extern u8 D_006AD3F0[];
extern u8 D_006BF2F0[];
extern u8 D_006D3D30[];
extern u8 D_006E8980[];

/* .bss, 0x8021AB80-0x8021AB90 (tools/bss_c.py) */
u32 __additional_scanline;
u8 D_8021AB84;

void func_802006F0(void) {
    func_80200714(D_8021AB84);
}

/*
 * Loads one of three RGBA16 images into D_80358070 and tints it for arg0:
 * 3/7/9 and 2/6/8 shift the colours, the rest keep them.
 *
 * The texels are the RDP's, big-endian, which the native-endian PC port
 * keeps in memory as bytes (docs/PORT.md): there IMG reads and writes them
 * byte-swapped.  Everywhere else it is the plain access.
 */
#if defined(TARGET_PC) && defined(PORT_NATIVE_ENDIAN)
#define IMG_RD(p) ((u16)__builtin_bswap16(*(p)))
#define IMG_WR(p, v) (*(p) = __builtin_bswap16(v))
#else
#define IMG_RD(p) (*(p))
#define IMG_WR(p, v) (*(p) = (v))
#endif
void func_80200714(u8 arg0) {
    u8 *romStart;
    u8 *romEnd;
    u32 size;
    u16 *img;
    u32 i;
    u8 r;
    u8 g;
    u8 b;
    u8 t;

    __additional_scanline = (u32)(uintptr_t)D_80358070;
    D_8021AB84 = arg0;
    switch (arg0) {
        case 1:
        case 2:
        case 3:
            romStart = D_006AD3F0;
            romEnd = D_006BF2F0;
            break;
        case 4:
        case 7:
        case 8:
            romStart = D_006BF2F0;
            romEnd = D_006D3D30;
            break;
        case 5:
        case 6:
        case 9:
            romStart = D_006D3D30;
            romEnd = D_006E8980;
            break;
        default:
            return;
    }
    size = romEnd - romStart;
    func_8028B4C4((u32)(uintptr_t)romStart, D_80358070, &size, 0xD, 0, 2);
    img = (u16 *)D_80358070;
    for (i = 0; i < size >> 1; i++) {
        r = IMG_RD(&img[i]) >> 11;
        g = (IMG_RD(&img[i]) >> 6) & 0x1F;
        b = (IMG_RD(&img[i]) >> 1) & 0x1F;
        switch (arg0) {
            case 3:
            case 7:
            case 9:
                r = (31.0 < (f32)r * 1.25) ? 31.0 : (f32)r * 1.25;
                if (b < 3) {
                    g = b;
                } else {
                    g = 3;
                }
                b = b / 4;
                break;
            case 2:
            case 6:
            case 8:
                if (b < 2) {
                    g = b;
                } else {
                    g = 2;
                }
                t = r;
                r = b / 3;
                b = (f32)t * 0.8125;
                break;
        }
        IMG_WR(&img[i], (r << 11) | (g << 6) | (b << 1) | 1);
    }
    D_80358070 += size;
}

void osViExtendVStart(u32 value) {
    __additional_scanline = value;
}

Gfx *func_80200BE0(Gfx *arg0, Frame *arg1, s32 *arg2) {
    Gfx *gfx = arg0;
    s32 x;
    s32 y;
    u8 alpha;

    gDPPipeSync(gfx++);
    gSPTexture(gfx++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    if (D_80364A90 & 0x000C000000000000) {
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gDPSetCombineLERP(gfx++, TEXEL0, 0, PRIMITIVE_ALPHA, 0, 0, 0, 0, 1, TEXEL0, 0, PRIMITIVE_ALPHA, 0, 0, 0, 0,
                          1);
        switch (D_80364A90) {
            case 0x0004000000000000:
            case 0x0008000000000000:
                alpha = 0x60;
                break;
            default:
                alpha = 0xFF;
                break;
        }
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, alpha);
    } else {
        gDPSetCycleType(gfx++, G_CYC_COPY);
        gDPSetRenderMode(gfx++, G_RM_NOOP, G_RM_NOOP2);
    }
    for (x = 0; x < 320; x += 64) {
        for (y = 0; y < 240; y += 16) {
            gDPLoadTextureTile(gfx++, __additional_scanline, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 240, x, y, x + 63,
                               y + 15, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            if (D_80364A90 & 0x000C000000000000) {
                gSPTextureRectangle(gfx++, x << 2, y << 2, (x + 64) << 2, (y + 16) << 2, G_TX_RENDERTILE, x << 5,
                                    y << 5, 1 << 10, 1 << 10);
            } else {
                gSPTextureRectangle(gfx++, x << 2, y << 2, (x + 63) << 2, (y + 15) << 2, G_TX_RENDERTILE, x << 5,
                                    y << 5, 4 << 10, 1 << 10);
            }
        }
    }
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    *arg2 += gfx - arg0;
    return gfx;
}
