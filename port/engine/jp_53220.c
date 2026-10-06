/*
 * hd_code 53220, jp's func_802979E0, which the C has as jp's GLOBAL_ASM, as
 * native C charged by the original's blocks (engine.h; jp_E7B0.c says more).
 */
#include "engine.h"
#include "game/game.h"
#include "game/yoshi.h"

#ifdef VERSION_JP

typedef struct {
    /* 0x0 */ u8 *PTR32 unk0;
    /* 0x4 */ u16 *PTR32 unk4;
} UnkStruct_802FF188;

extern u8 D_8039CAB6;
extern u8 D_8039CAD0;
extern u8 D_802FF180[6];
extern UnkStruct_802FF188 D_802FF188[7][20];
extern u8 D_802FF5E8[14][5];

/* the window of 21 (D_802F8BDC[21]): the entries of table D_8039CAD0, the
   first that func_80297EF8 says is free from D_8039CAB6 on and isn't arg's
   (or 6), and the count in its u16 text's digit */
void func_802979E0(u8 arg) {
    YoshiWindow *w = &D_802F8BDC[21];
    YoshiEntry *e;
    YoshiIcon *icon;
    u8 n, done, stop, r;
    s32 i, k, len, sel, count;
    u8 *text;
    u16 *text16;

    ENGINE_BLK(802979E0);
    n = func_80297F74();
    ENGINE_BLK(80297A00);
    done = 0;
    if (n == 1) {
        ENGINE_BLK(80297A18);
        r = func_80297EF8(arg);
        ENGINE_BLK(80297A20);
        if (r == 0) {
            ENGINE_BLK(80297A28);
            n--;
        }
    }
    ENGINE_BLK(80297A34);
    if (n == 0) {
        ENGINE_BLK(80297A40);
        D_8039CAD0 = 6;
    } else {
        ENGINE_BLK(80297A50);
        stop = 0;
        i = D_8039CAB6;
        if (i < D_8039CAB6 + 6) {
            do {
                ENGINE_BLK(80297A6C);
                r = func_80297EF8(D_802FF180[i % 6]);
                ENGINE_BLK(80297A8C);
                if (r == 0) {
                    ENGINE_BLK(80297A94);
                    if (D_802FF180[i % 6] != arg) {
                        ENGINE_BLK(80297ABC);
                        stop = 1;
                        goto next;
                    }
                }
                ENGINE_BLK(80297AC8);
                i++;
            next:
                ENGINE_BLK(80297AD4);
                if (i >= D_8039CAB6 + 6)
                    break;
                ENGINE_BLK(80297AF0);
            } while (stop == 0);
        }
        ENGINE_BLK(80297AFC);
        D_8039CAD0 = i % 6;
    }
    ENGINE_BLK(80297B14);
    i = 0;
    D_8036BB24 = (YoshiEntry *)D_80358070;
    D_80358070 += 0x24C;
    sel = 0;
    count = 0;
    stop = 0;
    do {
        ENGINE_BLK(80297B40);
        text = D_802FF188[D_8039CAD0][i].unk0;
        e = &D_8036BB24[i];
        text16 = D_802FF188[D_8039CAD0][i].unk4;
        if (text16 != NULL) {
            ENGINE_BLK(80297BA4);
            count++;
            e->flags = 0x1020;
            /* (jp's table of the selectable rows is D_802FF5E8's from
               0x24 on) */
            if (((u8 *)D_802FF5E8)[0x24 + D_8039CAD0 * 5 + sel] == i) {
                ENGINE_BLK(80297BE8);
                e->flags |= 1;
                sel++;
            }
            ENGINE_BLK(80297C04);
            e->y = i * 16;
            e->unk6 = 16;
            e->unk8 = 16;
            e->text = (char *)text;
            e->unk10 = text16;
            e->unk14 = 0;
            e->unk16 = 0;
            e->unk18 = 7;
            e->unk19 = 7;
            e->unk1A = 0;
        } else {
            ENGINE_BLK(80297C78);
            stop = 1;
        }
        ENGINE_BLK(80297C80);
        i++;
        if (i >= 20)
            break;
        ENGINE_BLK(80297C94);
    } while (stop == 0);
    ENGINE_BLK(80297CA0);
    e = &D_8036BB24[count];
    e->text = NULL;
    e->unk10 = NULL;
    e->flags = 0x400;
    e->x = -0x20;
    e->y = 0x26;
    e->unk14 = 0x18;
    e->unk1A = 0;
    e->unk16 = e->unk1A;
    icon = &D_802F49F4[e->unk14];
    e->unk1A = func_80272C5C(icon->unk6, 0, icon->unk4, icon->unk2C, icon->unk2D | 4, 1.0f);
    ENGINE_BLK(80297D60);
    w->count = count + 1;
    w->unk18 = D_802FF5E8[D_8039CAD0][0];
    r = func_80297EF8(arg);
    ENGINE_BLK(80297DA4);
    if (r == 0) {
        ENGINE_BLK(80297DAC);
        n--;
    }
    ENGINE_BLK(80297DB8);
    i = 0;
    if (w->count <= 0)
        goto out;
    ENGINE_BLK(80297DCC);
    if (done)
        goto out;
    do {
        ENGINE_BLK(80297DD8);
        e = &D_8036BB24[i];
        k = 0;
        len = func_8025B370(e->unk10);
        ENGINE_BLK(80297E04);
        if (k < len) {
            ENGINE_BLK(80297E14);
            if (!done) {
                do {
                    u16 *c = &e->unk10[k];

                    ENGINE_BLK(80297E20);
                    if (*c >= 0x10) {
                        ENGINE_BLK(80297E44);
                        if (*c < 0x16) {
                            ENGINE_BLK(80297E4C);
                            *c = n + 0x10;
                            done = 1;
                        }
                    }
                    ENGINE_BLK(80297E60);
                    k++;
                    len = func_8025B370(e->unk10);
                    ENGINE_BLK(80297E78);
                    if (k >= len)
                        break;
                    ENGINE_BLK(80297E88);
                } while (!done);
            }
        }
        ENGINE_BLK(80297E94);
        i++;
        if (i >= w->count)
            break;
        ENGINE_BLK(80297EB4);
    } while (!done);
out:
    ENGINE_BLK(80297EC0);
}

#endif
