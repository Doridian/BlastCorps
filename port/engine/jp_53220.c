/*
 * hd_code 53220, jp's func_802979E0, which the C has as jp's GLOBAL_ASM, as
 * native C (engine.h; jp_E7B0.c says more).
 */
#include "engine.h"
#include "game/game.h"
#include "game/yoshi.h"

#ifdef VERSION_JP

typedef struct {
    /* 0x0 */ u8 *unk0;
    /* 0x4 */ u16 *unk4;
} UnkStruct_802FF188;

extern u8 D_8039CAB6;
extern u8 D_8039CAD0;
extern u8 D_802FF180[6];
extern UnkStruct_802FF188 D_802FF188[7][20];
typedef struct {
    /* 0x00 */ u8 unk0[7][5];
    /* 0x23 */ u8 pad23;
    /* 0x24 */ u8 unk24[7][5];
} UnkStruct_802FF5E8;
extern UnkStruct_802FF5E8 D_802FF5E8; /* 53220.c */

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

    n = func_80297F74();
    done = 0;
    if (n == 1) {
        r = func_80297EF8(arg);
        if (r == 0) {
            n--;
        }
    }
    if (n == 0) {
        D_8039CAD0 = 6;
    } else {
        stop = 0;
        i = D_8039CAB6;
        if (i < D_8039CAB6 + 6) {
            do {
                r = func_80297EF8(D_802FF180[i % 6]);
                if (r == 0) {
                    if (D_802FF180[i % 6] != arg) {
                        stop = 1;
                        goto next;
                    }
                }
                i++;
            next:
                if (i >= D_8039CAB6 + 6)
                    break;
            } while (stop == 0);
        }
        D_8039CAD0 = i % 6;
    }
    i = 0;
    D_8036BB24 = (YoshiEntry *)D_80358070;
    D_80358070 += 21 * sizeof(YoshiEntry);    /* (0x24C on the N64) */
    sel = 0;
    count = 0;
    stop = 0;
    do {
        text = D_802FF188[D_8039CAD0][i].unk0;
        e = &D_8036BB24[i];
        text16 = D_802FF188[D_8039CAD0][i].unk4;
        if (text16 != NULL) {
            count++;
            e->flags = 0x1020;
            /* (jp's selectable rows are the second table) */
            if (D_802FF5E8.unk24[D_8039CAD0][sel] == i) {
                e->flags |= 1;
                sel++;
            }
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
            stop = 1;
        }
        i++;
        if (i >= 20)
            break;
    } while (stop == 0);
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
    e->unk1A = func_80272C5C((u16 *)icon->unk6, 0, icon->unk4, icon->unk2C, icon->unk2D | 4, 1.0f);
    w->count = count + 1;
    w->unk18 = D_802FF5E8.unk0[D_8039CAD0][0];
    r = func_80297EF8(arg);
    if (r == 0) {
        n--;
    }
    i = 0;
    if (w->count <= 0)
        goto out;
    if (done)
        goto out;
    do {
        e = &D_8036BB24[i];
        k = 0;
        len = func_8025B370(e->unk10);
        if (k < len) {
            if (!done) {
                do {
                    u16 *c = &e->unk10[k];

                    if (*c >= 0x10) {
                        if (*c < 0x16) {
                            *c = n + 0x10;
                            done = 1;
                        }
                    }
                    k++;
                    len = func_8025B370(e->unk10);
                    if (k >= len)
                        break;
                } while (!done);
            }
        }
        i++;
        if (i >= w->count)
            break;
    } while (!done);
out:
}

#endif
