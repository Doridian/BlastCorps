/*
 * hd_code 1D990 (the level's goal and timer: the race's laps, the damage
 * left, the HUD's counters), jp's own: the four functions Blastdozer
 * compiled differently from the US versions, which the C has as jp's
 * GLOBAL_ASM (blastcorps/src/hd_code/1D990.c), as native C (engine.h; jp_E7B0.c says more).
 *
 * jp writes the goal's text in its u16 text with func_8025B5D4 where the US
 * versions sprintf a char string.
 */
#include "engine.h"
#include "game/objects.h"
#include "game/sched.h"
#include "game/audio.h"
#include "game/game.h"
#include "game/yoshi.h"
#include "game/level.h"

#ifdef VERSION_JP

/* 1D990.c's .bss */
extern u8 D_80367B54;
extern u16 D_80367B58[4];
extern char D_80367B60[0x50];
extern u8 D_80367C70_jp[0x28];
extern char D_80367BB0[0xc];
extern s32 D_80367BBC;
extern s32 D_80367BC0;
extern u32 D_80367BC4;
extern u16 D_80367BC8;
extern u8 D_80367BD4;
extern u8 D_80367BD5;
extern s16 D_80367BD6;
extern s16 D_80367BD8;
extern u16 D_80367BF4;
extern u8 D_80367BF8;
extern u8 D_80367BF9;
extern u8 D_80367BFA;
extern u8 D_80367BFB;
extern u16 D_80367BFC;
extern LevelInfo *D_80367C04;
extern char D_80367C18[0x28];
extern char D_80367C40[0x28];
extern u16 D_80367C68[0x28];
extern u16 D_80367CB8[0x28];
extern char D_80367D10[0x18];
extern char D_80367D28[0x28];

extern s32 D_80364A58;
extern u8 D_803156F4;

/* the original's format strings */
extern char D_80309374[];       /* "$%d" */
extern char D_80309380[];       /* "box number=%d\n" */
extern char D_80309390[];       /* "new best lap %d %d\n" */
extern char D_803093C0[];       /* "box cross: %d to %d\n" */
extern u8 D_803643D7;

/* jp's u16 text (BC8E0) */
extern u16 D_80301040[], D_803047F8[], D_80304814[], D_80304838[], D_80304844[], D_80304860[], D_80304878[],
    D_80304888[], D_8030489C[], D_803048AC[], D_803048B8[], D_803048C0[];

void alCSPSetTempo(ALCSPlayer *, s32);

/* the level's intro, a second at a time: the goal's text, then the
   countdown's sounds */
void func_80262840(void) {
    u32 sec;
    u16 min, s;

    sec = (u32)(D_803156C4 - D_80367BC0) / 60;
    if (sec != D_80367BC4) {
        if (sec < 5) {
            switch (sec) {
            case 0: {
                u32 kind;

                D_80367C68[0] = 0xFFF;
                D_80367CB8[0] = 0xFFF;
                kind = (u32)D_80364AA8;
                if (kind >= 0x41) {
                    if (kind == 0x80) {
                        if (D_802E8BDC == 0x32) {
                            func_8025B5D4(D_80367C68, D_80304844, NULL, 0);
                        } else {
                            func_8025B5D4(D_80367C68, D_80304860, NULL, 0);
                        }
                    }
                } else if (kind >= 0x21) {
                    if (kind == 0x40) {
                        func_8025B5D4(D_80367C68, D_80304888, NULL, D_80367C04->goal);
                    }
                } else {
                    if (kind - 2 < 0x1F) {
                        switch (kind) {
                        case 0x2:
                            func_8025B5D4(D_80367C68, D_80304814, NULL, D_80367C04->goal);
                            break;
                        case 0x4:
                        case 0x20:
                            if (D_802E8BDC == 0x34) {
                                func_8025B5D4(D_80367C68, D_80301040, D_803047F8, 0);
                            } else {
                                func_8025B5D4(D_80367C68, D_80304838, D_80367C0C, 0);
                            }
                            break;
                        case 0x8:
                            func_8025B5D4(D_80367C68, D_80304878, NULL, D_80367C04->goal);
                            break;
                        case 0x10:
                            func_8025B5D4(D_80367C68, D_80304888, NULL, D_80367C04->goal);
                            break;
                        }
                    }
                }
                min = (s32)D_80367C04->medalTimes[3] / 600;
                s = ((s32)D_80367C04->medalTimes[3] / 10) % 60;
                func_8025B5D4(D_80367CB8, D_8030489C, NULL, min);
                func_8025B5D4(D_80367CB8, D_803048AC, D_80367CB8, s);
                D_802F5804[YOSHI_ENTRY(36)].text = D_80367C18;
                D_802F5804[YOSHI_ENTRY(37)].text = D_80367C40;
                D_802F5804[YOSHI_ENTRY(36)].unk10 = D_80367C68;
                D_802F5804[YOSHI_ENTRY(37)].unk10 = D_80367CB8;
                func_8026AF6C(0x8009);
                {
                    s32 id = func_8026205C(0);

                    func_80260650(D_80367738, id, 0);
                }
                break;
            }
            case 1:
                func_80260650(D_80367738, 0x96, 0);
                break;
            case 2:
                func_80260650(D_80367738, 0x96, 0);
                break;
            case 3:
                func_80260650(D_80367738, 0x96, 0);
                break;
            case 4:
                func_80260650(D_80367738, 0x99, 0);
                break;
            }
        }
    }
    D_80367BC4 = sec;
    if (sec == 4) {
        if (D_8035805C == D_803156F4) {
            D_80364A98 = 4;
        }
    }
}

/* the damage level: "$%d" left */
void func_80263358(void) {
    s32 left;

    func_802C1DD0(0);
    if (D_8036EA70.ip >= D_80367C04->goal) {
        D_803643DA = 1;
    }
    left = D_80367C04->goal - D_8036EA70.ip;
    if (left < 0) {
        left = 0;
    }
    func_8025B5D4((u16 *)D_80367C70_jp, D_80301040, D_803048B8, 0);
    sprintf(D_80367B60, D_80309374, left);
}

/* the race: the laps, through the boxes of the track's quarters */
void func_802633E0(void) {
    s32 i;
    u8 left;

    if (D_80367B54 == 0) {
        s32 in;

        in = func_8026394C(D_803643E0 >> 5, D_803643E8 >> 5, D_80367C04->unkA, D_80367C04->unkC,
                           D_80367C04->unkE, D_80367C04->unk10);
        if (in != 0) {
            D_80367B54 = 1;
            D_80367BF8 = 0;
            D_80367BF9 = D_80367C04->unk12[0];
            D_80367BBC = D_803156C0;
            D_80367BFB = 1;
            D_80367BFC = 0xFFFF;
        }
    } else {
        s32 n, d, q;
        u8 box;
        u16 t;

        n = (D_803643E8 >> 5) - D_80367C04->unk4;
        d = ((s32)D_80367C04->unk8 - D_80367C04->unk4) >> 1;
        ENGINE_DIV(q, n, d, 802634CC, 802634E4);
        D_80367BFA = q;
        box = (u8)q << 1;
        D_80367BFA = box;
        n = (D_803643E0 >> 5) - D_80367C04->unk2;
        d = ((s32)D_80367C04->unk6 - D_80367C04->unk2) >> 1;
        ENGINE_DIV(q, n, d, 8026352C, 80263544);
        D_80367BFA = box + q;
        t = func_8028604C(D_803156C0 - D_80364A58);
        D_80367B58[D_80367B54 - 1] = t;
        i = D_80367B54 - 2;
        if (i >= 0) {
            do {
                D_80367B58[D_80367B54 - 1] -= D_80367B58[i];
                i--;
            } while (i >= 0);
        }
        if (D_80370C28 & 0x2000) {
            func_8029A7E4(D_80309380, D_80367BFA);
        }
        if (D_80367BF8 == 4) {
            s32 in;

            in = func_8026394C(D_803643E0 >> 5, D_803643E8 >> 5, D_80367C04->unkA, D_80367C04->unkC,
                               D_80367C04->unkE, D_80367C04->unk10);
            if (in != 0) {
                u16 lap, best;

                lap = D_80367B58[D_80367B54 - 1];
                best = D_80367BFC;
                if (lap < best) {
                    func_8029A7E4(D_80309390, best, lap);
                    D_80367BFB = D_80367B54;
                    D_80367BFC = D_80367B58[D_80367B54 - 1];
                }
                if (D_80367C04->goal - 1 < D_80367B54) {
                    D_803643DA = 1;
                } else {
                    s32 tempo;

                    left = D_80367C04->goal - D_80367B54;
                    if (D_802E8BD0 == 0) {
                        func_8026AF6C(0x8008);
                    }
                    func_8025B5D4((u16 *)D_80367D28, D_803048C0, NULL, left);
                    D_802F5804[YOSHI_ENTRY(35)].text = D_80367D10;
                    D_802F5804[YOSHI_ENTRY(35)].unk10 = (u16 *)D_80367D28;
                    tempo = alCSPGetTempo(D_80367734);
                    alCSPSetTempo(D_80367734, engine_trunc_w_d((f64)tempo * 0.95));
                }
                D_80367B54++;
                D_80367BF8 = 0;
            }
        } else {
            s32 next, idx;

            if (D_80367C04->unk12[D_80367BF8] == D_80367BF9) {
                next = D_80367BF8 + 1;
                idx = next & 3;
                if (next < 0) {
                    if (idx != 0) {
                        idx -= 4;
                    }
                }
                if (D_80367C04->unk12[idx] == D_80367BFA) {
                    func_8029A7E4(D_803093C0, D_80367BF9, D_80367BFA);
                    D_80367BF8++;
                }
            }
        }
        D_80367BF9 = D_80367BFA;
    }
    i = 0;
    if (D_80367B54 > 0) {
        if (D_80367C04->goal != 0) {
            for (;;) {
                func_80264A34(D_80367B60 + i * 0x14, D_80367B58[i], D_80367B54 == i + 1);
                i++;
                if (!(i < D_80367B54))
                    break;
                if (!((u32)i < D_80367C04->goal))
                    break;
            }
        }
    }
}

/* the level's HUD: the goal's counter (the laps' times), the timer, the
   icons */
Gfx *func_802639B4(Gfx *arg0, void *arg1, s32 arg2) {
    Gfx *gfx = arg0;
    s16 alpha;
    s32 i = 0;
    u8 flash, red, green;
    u32 kind;

    /* (the mask's high word is 0: 802639F8 always runs) */
    if (((u32)D_80364A90 & 0x04000200) != 0) {
        if (D_8036BB18 != 0x1E) {
            alpha = 0xFF;
            goto have_alpha;
        }
    }
    alpha = D_80367BD6;
have_alpha:
    kind = (u32)D_80364AA8;
    if (kind >= 0x41) {
        if (kind == 0x80) {
            goto case_4;
        }
        goto done;
    }
    if (kind >= 0x21) {
        if (kind == 0x40) {
            goto case_10;
        }
        goto done;
    }
    if (!(kind - 2 < 0x1F)) {
        goto done;
    }
    switch (kind) {
    case 0x2: {
        s32 last;

        i = 0;
        if (((u32)D_80364A90 & 0x440) != 0) {
            last = 1;
        } else {
            last = 0;
        }
        if (!(i < D_80367B54 - last))
            break;
        if (!((u32)i < D_80367C04->goal))
            break;
        for (;;) {
            if (i + 1 == D_80367BFB && (i + 1 != D_80367B54)) {
                func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xFF, 0,
                              0, D_80367BD6);
            } else {
                if (i + 1 == D_80367B54) {
                    func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xFF,
                                  0xFF, 0xFF, D_80367BD6);
                } else {
                    func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xA0,
                                  0xA0, 0xA0, D_80367BD6);
                }
            }
            i++;
            if (((u32)D_80364A90 & 0x440) != 0) {
                last = 1;
            } else {
                last = 0;
            }
            if (!(i < D_80367B54 - last))
                break;
            if (!((u32)i < D_80367C04->goal)) {
                break;
            }
        }
        break;
    }
    case 0x4:
    case 0x20:
    case_4:
        func_80259CCC(arg1, D_80367B60, NULL, 1, 0, 0x38, 0x14, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        break;
    case 0x8:
        func_80259CCC(arg1, NULL, (u16 *)D_80367C70_jp, 0, 0, 0x1C, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        func_80259CCC(arg1, D_80367B60, NULL, 0, 0, 0x44, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        break;
    case 0x10:
    case_10:
        func_80259CCC(arg1, D_80367B60, NULL, 1, 0, 0x38, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        break;
    }
done:
    red = 0xFF;
    green = 0;
    if (((u32)D_80364A90 & 0x04000104) != 0) {
        flash = (s32)D_80367BF4 < 10;
        if (!flash) {
            green = 0xFF;
        }
    } else {
        flash = 0;
        green = 0xFF;
        red = 0;
    }
    if (D_80367BF4 == 0) {
        flash = 0;
    }
    if (flash != 0) {
        if (!(D_803156C4 % 20 < 16))
            goto no_timer;
    }
    if (D_80364AA8 == 2) {
        func_80259CCC(arg1, D_80367BB0, NULL, 1, 0, 0x18, i * 0x12 + 0x14, 0x10, 0x10, 1, red, green, 0,
                      D_80367BD6);
    } else {
        func_80259CCC(arg1, D_80367BB0, NULL, 1, 0, 0x1C, D_80367BD8 + 0x2A, 0x10, 0x10, 1, red, green, 0, alpha);
    }
no_timer:
    if (D_803643D7 != 0) {
        if ((u32)(D_80364A90 >> 32) == 0) {
            if ((u32)D_80364A90 == 0x04000000) {
                func_8025E2CC(&gfx, arg1, D_8035805C);
            }
        }
    }
    if (D_80367BC8 != 0) {
        Gfx *g;

        g = func_80264264(arg1, gfx);
        gfx = g;
    }
    gfx = func_80274868(gfx);
    if (D_80367BCC != NULL) {
        YoshiIcon *icon = D_80367BCC;
        u32 q, r;
        Gfx *g;

        if (icon->unk26 == 0) {
            engine_break(N64_PC(0x80264170), 7);
        }
        q = D_803156C4 / icon->unk26;
        if (icon->unk1A == 0) {
            engine_break(N64_PC(0x80264180), 7);
        }
        r = q % icon->unk1A;
        g = func_80272ED8(gfx, icon->unk1B[r] + D_80367BD4 - 1, 0x18, D_80367BD8 + 0xC, alpha, 1, 1.0f);
        gfx = g;
    }
    if (D_80367BD0 != NULL) {
        YoshiIcon *icon = D_80367BD0;
        u32 q, r;
        Gfx *g;

        if (icon->unk26 == 0) {
            engine_break(N64_PC(0x80264208), 7);
        }
        q = D_803156C4 / icon->unk26;
        if (icon->unk1A == 0) {
            engine_break(N64_PC(0x80264218), 7);
        }
        r = q % icon->unk1A;
        g = func_80272ED8(gfx, icon->unk1B[r] + D_80367BD5 - 1, 0x18, D_80367BD8 + 0xC, alpha, 1, 1.0f);
        gfx = g;
    }
    gfx = func_80274AA4(gfx);
    arg2 += (gfx - arg0) * 4;
    return gfx;
}

#endif
