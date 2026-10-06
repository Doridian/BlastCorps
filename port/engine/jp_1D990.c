/*
 * hd_code 1D990 (the level's goal and timer: the race's laps, the damage
 * left, the HUD's counters), jp's own: the four functions Blastdozer
 * compiled differently from the US versions, which the C has as jp's
 * GLOBAL_ASM (blastcorps/src/hd_code/1D990.c), as native C charged by the
 * original's blocks (engine.h; jp_E7B0.c says more).
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
extern YoshiIcon *D_80367BCC;
extern YoshiIcon *D_80367BD0;
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
extern u16 *D_80367C0C;
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

    ENGINE_BLK(80262840);
    sec = (u32)(D_803156C4 - D_80367BC0) / 60;
    if (sec != D_80367BC4) {
        ENGINE_BLK(8026287C);
        if (sec < 5) {
            ENGINE_BLK(8026288C);
            switch (sec) {
            case 0: {
                u32 kind;

                ENGINE_BLK(802628A4);
                D_80367C68[0] = 0xFFF;
                D_80367CB8[0] = 0xFFF;
                kind = (u32)D_80364AA8;
                if (kind >= 0x41) {
                    ENGINE_BLK(802628D8);
                    if (kind == 0x80) {
                        ENGINE_BLK(802629BC);
                        if (D_802E8BDC == 0x32) {
                            ENGINE_BLK(802629D0);
                            func_8025B5D4(D_80367C68, D_80304844, NULL, 0);
                            ENGINE_BLK(802629EC);
                        } else {
                            ENGINE_BLK(802629F4);
                            func_8025B5D4(D_80367C68, D_80304860, NULL, 0);
                            ENGINE_BLK(80262A10);
                        }
                    } else {
                        ENGINE_BLK(802628E0);
                    }
                } else if (kind >= 0x21) {
                    ENGINE_BLK(802628E8);
                    ENGINE_BLK(802628F4);
                    if (kind == 0x40) {
                        ENGINE_BLK(80262A44);
                        func_8025B5D4(D_80367C68, D_80304888, NULL, D_80367C04->goal);
                    } else {
                        ENGINE_BLK(802628FC);
                    }
                } else {
                    ENGINE_BLK(802628E8);
                    ENGINE_BLK(80262904);
                    if (kind - 2 < 0x1F) {
                        ENGINE_BLK(80262914);
                        switch (kind) {
                        case 0x2:
                            ENGINE_BLK(8026292C);
                            func_8025B5D4(D_80367C68, D_80304814, NULL, D_80367C04->goal);
                            ENGINE_BLK(80262950);
                            break;
                        case 0x4:
                        case 0x20:
                            ENGINE_BLK(80262958);
                            if (D_802E8BDC == 0x34) {
                                ENGINE_BLK(8026296C);
                                func_8025B5D4(D_80367C68, D_80301040, D_803047F8, 0);
                                ENGINE_BLK(8026298C);
                            } else {
                                ENGINE_BLK(80262994);
                                func_8025B5D4(D_80367C68, D_80304838, D_80367C0C, 0);
                                ENGINE_BLK(802629B4);
                            }
                            break;
                        case 0x8:
                            ENGINE_BLK(80262A18);
                            func_8025B5D4(D_80367C68, D_80304878, NULL, D_80367C04->goal);
                            ENGINE_BLK(80262A3C);
                            break;
                        case 0x10:
                            ENGINE_BLK(80262A44);
                            func_8025B5D4(D_80367C68, D_80304888, NULL, D_80367C04->goal);
                            break;
                        }
                    }
                }
                ENGINE_BLK(80262A68);
                min = (s32)D_80367C04->medalTimes[3] / 600;
                s = ((s32)D_80367C04->medalTimes[3] / 10) % 60;
                func_8025B5D4(D_80367CB8, D_8030489C, NULL, min);
                ENGINE_BLK(80262AC0);
                func_8025B5D4(D_80367CB8, D_803048AC, D_80367CB8, s);
                ENGINE_BLK(80262AE0);
                D_802F5804[YOSHI_ENTRY(36)].text = D_80367C18;
                D_802F5804[YOSHI_ENTRY(37)].text = D_80367C40;
                D_802F5804[YOSHI_ENTRY(36)].unk10 = D_80367C68;
                D_802F5804[YOSHI_ENTRY(37)].unk10 = D_80367CB8;
                func_8026AF6C(0x8009);
                ENGINE_BLK(80262B38);
                {
                    s32 id = func_8026205C(0);

                    ENGINE_BLK(80262B40);
                    func_80260650(D_80367738, id, 0);
                }
                ENGINE_BLK(80262B58);
                break;
            }
            case 1:
                ENGINE_BLK(80262B60);
                func_80260650(D_80367738, 0x96, 0);
                ENGINE_BLK(80262B74);
                break;
            case 2:
                ENGINE_BLK(80262B7C);
                func_80260650(D_80367738, 0x96, 0);
                ENGINE_BLK(80262B90);
                break;
            case 3:
                ENGINE_BLK(80262B98);
                func_80260650(D_80367738, 0x96, 0);
                ENGINE_BLK(80262BAC);
                break;
            case 4:
                ENGINE_BLK(80262BB4);
                func_80260650(D_80367738, 0x99, 0);
                break;
            }
        }
    }
    ENGINE_BLK(80262BC8);
    D_80367BC4 = sec;
    if (sec == 4) {
        ENGINE_BLK(80262BE0);
        if (D_8035805C == D_803156F4) {
            ENGINE_BLK(80262BF8);
            D_80364A98 = 4;
        }
    }
    ENGINE_BLK(80262C10);
}

/* the damage level: "$%d" left */
void func_80263358(void) {
    s32 left;

    ENGINE_BLK(80263358);
    func_802C1DD0(0);
    ENGINE_BLK(80263368);
    if (D_8036EA70.ip >= D_80367C04->goal) {
        ENGINE_BLK(80263388);
        D_803643DA = 1;
    }
    ENGINE_BLK(80263394);
    left = D_80367C04->goal - D_8036EA70.ip;
    if (left < 0) {
        ENGINE_BLK(802633B4);
        left = 0;
    }
    ENGINE_BLK(802633B8);
    func_8025B5D4((u16 *)D_80367C70_jp, D_80301040, D_803048B8, 0);
    ENGINE_BLK(802633D8);
    engine_sprintf(D_80367B60, D_80309374, left);
    ENGINE_BLK(802633F0);
}

/* the race: the laps, through the boxes of the track's quarters */
void func_802633E0(void) {
    s32 i;
    u8 left;

    ENGINE_BLK(802633E0);
    if (D_80367B54 == 0) {
        s32 in;

        ENGINE_BLK(802633F8);
        in = func_8026394C(D_803643E0 >> 5, D_803643E8 >> 5, D_80367C04->unkA, D_80367C04->unkC,
                           D_80367C04->unkE, D_80367C04->unk10);
        ENGINE_BLK(8026343C);
        if (in != 0) {
            ENGINE_BLK(80263444);
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

        ENGINE_BLK(80263498);
        n = (D_803643E8 >> 5) - D_80367C04->unk4;
        d = ((s32)D_80367C04->unk8 - D_80367C04->unk4) >> 1;
        ENGINE_DIV(q, n, d, 802634CC, 802634D0, 802634DC, 802634E4);
        ENGINE_BLK(802634E8);
        D_80367BFA = q;
        box = (u8)q << 1;
        D_80367BFA = box;
        n = (D_803643E0 >> 5) - D_80367C04->unk2;
        d = ((s32)D_80367C04->unk6 - D_80367C04->unk2) >> 1;
        ENGINE_DIV(q, n, d, 8026352C, 80263530, 8026353C, 80263544);
        ENGINE_BLK(80263548);
        D_80367BFA = box + q;
        t = func_8028604C(D_803156C0 - D_80364A58);
        ENGINE_BLK(80263574);
        D_80367B58[D_80367B54 - 1] = t;
        i = D_80367B54 - 2;
        if (i >= 0) {
            do {
                ENGINE_BLK(802635A0);
                D_80367B58[D_80367B54 - 1] -= D_80367B58[i];
                i--;
            } while (i >= 0);
        }
        ENGINE_BLK(802635E4);
        if (D_80370C28 & 0x2000) {
            ENGINE_BLK(802635F8);
            func_8029A7E4(D_80309380, D_80367BFA);
        }
        ENGINE_BLK(8026360C);
        if (D_80367BF8 == 4) {
            s32 in;

            ENGINE_BLK(80263620);
            in = func_8026394C(D_803643E0 >> 5, D_803643E8 >> 5, D_80367C04->unkA, D_80367C04->unkC,
                               D_80367C04->unkE, D_80367C04->unk10);
            ENGINE_BLK(80263664);
            if (in != 0) {
                u16 lap, best;

                ENGINE_BLK(8026366C);
                lap = D_80367B58[D_80367B54 - 1];
                best = D_80367BFC;
                if (lap < best) {
                    ENGINE_BLK(80263698);
                    func_8029A7E4(D_80309390, best, lap);
                    ENGINE_BLK(802636AC);
                    D_80367BFB = D_80367B54;
                    D_80367BFC = D_80367B58[D_80367B54 - 1];
                }
                ENGINE_BLK(802636D4);
                if (D_80367C04->goal - 1 < D_80367B54) {
                    ENGINE_BLK(802636F8);
                    D_803643DA = 1;
                } else {
                    s32 tempo;

                    ENGINE_BLK(80263708);
                    left = D_80367C04->goal - D_80367B54;
                    if (D_802E8BD0 == 0) {
                        ENGINE_BLK(80263734);
                        func_8026AF6C(0x8008);
                    }
                    ENGINE_BLK(8026373C);
                    func_8025B5D4((u16 *)D_80367D28, D_803048C0, NULL, left);
                    ENGINE_BLK(80263758);
                    D_802F5804[YOSHI_ENTRY(35)].text = D_80367D10;
                    D_802F5804[YOSHI_ENTRY(35)].unk10 = (u16 *)D_80367D28;
                    tempo = alCSPGetTempo(D_80367734);
                    ENGINE_BLK(8026378C);
                    alCSPSetTempo(D_80367734, engine_trunc_w_d((f64)tempo * 0.95));
                }
                ENGINE_BLK(802637BC);
                D_80367B54++;
                D_80367BF8 = 0;
            }
        } else {
            s32 next, idx;

            ENGINE_BLK(802637DC);
            if (D_80367C04->unk12[D_80367BF8] == D_80367BF9) {
                ENGINE_BLK(80263804);
                next = D_80367BF8 + 1;
                idx = next & 3;
                if (next < 0) {
                    ENGINE_BLK(80263810);
                    if (idx != 0) {
                        ENGINE_BLK(80263818);
                        idx -= 4;
                    }
                }
                ENGINE_BLK(8026381C);
                if (D_80367C04->unk12[idx] == D_80367BFA) {
                    ENGINE_BLK(80263834);
                    func_8029A7E4(D_803093C0, D_80367BF9, D_80367BFA);
                    ENGINE_BLK(80263848);
                    D_80367BF8++;
                }
            }
        }
        ENGINE_BLK(8026385C);
        D_80367BF9 = D_80367BFA;
    }
    ENGINE_BLK(8026386C);
    i = 0;
    if (D_80367B54 > 0) {
        ENGINE_BLK(80263880);
        if (D_80367C04->goal != 0) {
            for (;;) {
                ENGINE_BLK(80263894);
                func_80264A34(D_80367B60 + i * 0x14, D_80367B58[i], D_80367B54 == i + 1);
                ENGINE_BLK(802638D8);
                i++;
                if (!(i < D_80367B54))
                    break;
                ENGINE_BLK(802638F8);
                if (!((u32)i < D_80367C04->goal))
                    break;
            }
        }
    }
    ENGINE_BLK(80263910);
}

/* the level's HUD: the goal's counter (the laps' times), the timer, the
   icons */
Gfx *func_802639B4(Gfx *arg0, void *arg1, s32 arg2) {
    Gfx *gfx = arg0;
    s16 alpha;
    s32 i = 0;
    u8 flash, red, green;
    u32 kind;

    ENGINE_BLK(802639B4);
    /* (the mask's high word is 0: 802639F8 always runs) */
    ENGINE_BLK(802639F8);
    if (((u32)D_80364A90 & 0x04000200) != 0) {
        ENGINE_BLK(80263A00);
        if (D_8036BB18 != 0x1E) {
            ENGINE_BLK(80263A14);
            alpha = 0xFF;
            goto have_alpha;
        }
    }
    ENGINE_BLK(80263A20);
    alpha = D_80367BD6;
have_alpha:
    ENGINE_BLK(80263A2C);
    kind = (u32)D_80364AA8;
    if (kind >= 0x41) {
        ENGINE_BLK(80263A40);
        if (kind == 0x80) {
            goto case_4;
        }
        ENGINE_BLK(80263A48);
        goto done;
    }
    ENGINE_BLK(80263A50);
    if (kind >= 0x21) {
        ENGINE_BLK(80263A5C);
        if (kind == 0x40) {
            goto case_10;
        }
        ENGINE_BLK(80263A64);
        goto done;
    }
    ENGINE_BLK(80263A6C);
    if (!(kind - 2 < 0x1F)) {
        goto done;
    }
    ENGINE_BLK(80263A7C);
    switch (kind) {
    case 0x2: {
        s32 last;

        ENGINE_BLK(80263A94);
        i = 0;
        ENGINE_BLK(80263AB4);
        if (((u32)D_80364A90 & 0x440) != 0) {
            ENGINE_BLK(80263ABC);
            last = 1;
        } else {
            ENGINE_BLK(80263AC4);
            last = 0;
        }
        ENGINE_BLK(80263AC8);
        if (!(i < D_80367B54 - last))
            break;
        ENGINE_BLK(80263AE4);
        if (!((u32)i < D_80367C04->goal))
            break;
        for (;;) {
            ENGINE_BLK(80263AFC);
            if (i + 1 == D_80367BFB && (ENGINE_BLK(80263B14), i + 1 != D_80367B54)) {
                ENGINE_BLK(80263B24);
                func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xFF, 0,
                              0, D_80367BD6);
                ENGINE_BLK(80263BA0);
            } else {
                ENGINE_BLK(80263BA8);
                if (i + 1 == D_80367B54) {
                    ENGINE_BLK(80263BC0);
                    func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xFF,
                                  0xFF, 0xFF, D_80367BD6);
                    ENGINE_BLK(80263C44);
                } else {
                    ENGINE_BLK(80263C4C);
                    func_80259CCC(arg1, D_80367B60 + i * 0x14, NULL, 1, 0, 0x18, i * 0x12 + 0x12, 0x14, 0x14, 1, 0xA0,
                                  0xA0, 0xA0, D_80367BD6);
                }
            }
            ENGINE_BLK(80263CD4);
            i++;
            ENGINE_BLK(80263CFC);
            if (((u32)D_80364A90 & 0x440) != 0) {
                ENGINE_BLK(80263D04);
                last = 1;
            } else {
                ENGINE_BLK(80263D0C);
                last = 0;
            }
            ENGINE_BLK(80263D10);
            if (!(i < D_80367B54 - last))
                break;
            ENGINE_BLK(80263D2C);
            if (!((u32)i < D_80367C04->goal)) {
                ENGINE_BLK(80263D44);
                break;
            }
        }
        break;
    }
    case 0x4:
    case 0x20:
    case_4:
        ENGINE_BLK(80263D4C);
        func_80259CCC(arg1, D_80367B60, NULL, 1, 0, 0x38, 0x14, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        ENGINE_BLK(80263DB0);
        break;
    case 0x8:
        ENGINE_BLK(80263DB8);
        func_80259CCC(arg1, NULL, (u16 *)D_80367C70_jp, 0, 0, 0x1C, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        ENGINE_BLK(80263E1C);
        func_80259CCC(arg1, D_80367B60, NULL, 0, 0, 0x44, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        ENGINE_BLK(80263E80);
        break;
    case 0x10:
    case_10:
        ENGINE_BLK(80263E88);
        func_80259CCC(arg1, D_80367B60, NULL, 1, 0, 0x38, 0x12, 0x14, 0x14, 1, 0xFF, 0xFF, 0xFF, alpha);
        break;
    }
done:
    ENGINE_BLK(80263EEC);
    red = 0xFF;
    green = 0;
    ENGINE_BLK(80263F1C);
    if (((u32)D_80364A90 & 0x04000104) != 0) {
        ENGINE_BLK(80263F24);
        flash = (s32)D_80367BF4 < 10;
        if (!flash) {
            ENGINE_BLK(80263F3C);
            green = 0xFF;
        }
    } else {
        ENGINE_BLK(80263F48);
        flash = 0;
        green = 0xFF;
        red = 0;
    }
    ENGINE_BLK(80263F58);
    if (D_80367BF4 == 0) {
        ENGINE_BLK(80263F68);
        flash = 0;
    }
    ENGINE_BLK(80263F6C);
    if (flash != 0) {
        ENGINE_BLK(80263F78);
        if (!(D_803156C4 % 20 < 16))
            goto no_timer;
    }
    ENGINE_BLK(80263F98);
    if (D_80364AA8 == 2) {
        ENGINE_BLK(80263FAC);
        func_80259CCC(arg1, D_80367BB0, NULL, 1, 0, 0x18, i * 0x12 + 0x14, 0x10, 0x10, 1, red, green, 0,
                      D_80367BD6);
        ENGINE_BLK(80264020);
    } else {
        ENGINE_BLK(80264028);
        func_80259CCC(arg1, D_80367BB0, NULL, 1, 0, 0x1C, D_80367BD8 + 0x2A, 0x10, 0x10, 1, red, green, 0, alpha);
    }
no_timer:
    ENGINE_BLK(80264090);
    if (D_803643D7 != 0) {
        ENGINE_BLK(802640A0);
        if ((u32)(D_80364A90 >> 32) == 0) {
            ENGINE_BLK(802640B8);
            if ((u32)D_80364A90 == 0x04000000) {
                ENGINE_BLK(802640C0);
                func_8025E2CC(&gfx, arg1, D_8035805C);
            }
        }
    }
    ENGINE_BLK(802640D4);
    if (D_80367BC8 != 0) {
        Gfx *g;

        ENGINE_BLK(802640E4);
        g = func_80264264(arg1, gfx);
        ENGINE_BLK(802640F0);
        gfx = g;
    }
    ENGINE_BLK(802640F4);
    gfx = func_80274868(gfx);
    ENGINE_BLK(802640FC);
    if (D_80367BCC != NULL) {
        YoshiIcon *icon = D_80367BCC;
        u32 q, r;
        Gfx *g;

        ENGINE_BLK(80264110);
        if (icon->unk26 == 0) {
            ENGINE_BLK(80264170);
            engine_break(0x80264170, 7);
        }
        q = D_803156C4 / icon->unk26;
        ENGINE_BLK(80264174);
        if (icon->unk1A == 0) {
            ENGINE_BLK(80264180);
            engine_break(0x80264180, 7);
        }
        r = q % icon->unk1A;
        ENGINE_BLK(80264184);
        g = func_80272ED8(gfx, icon->unk1B[r] + D_80367BD4 - 1, 0x18, D_80367BD8 + 0xC, alpha, 1, 1.0f);
        ENGINE_BLK(80264194);
        gfx = g;
    }
    ENGINE_BLK(80264198);
    if (D_80367BD0 != NULL) {
        YoshiIcon *icon = D_80367BD0;
        u32 q, r;
        Gfx *g;

        ENGINE_BLK(802641A8);
        if (icon->unk26 == 0) {
            ENGINE_BLK(80264208);
            engine_break(0x80264208, 7);
        }
        q = D_803156C4 / icon->unk26;
        ENGINE_BLK(8026420C);
        if (icon->unk1A == 0) {
            ENGINE_BLK(80264218);
            engine_break(0x80264218, 7);
        }
        r = q % icon->unk1A;
        ENGINE_BLK(8026421C);
        g = func_80272ED8(gfx, icon->unk1B[r] + D_80367BD5 - 1, 0x18, D_80367BD8 + 0xC, alpha, 1, 1.0f);
        ENGINE_BLK(8026422C);
        gfx = g;
    }
    ENGINE_BLK(80264230);
    gfx = func_80274AA4(gfx);
    ENGINE_BLK(80264238);
    arg2 += (gfx - arg0) * 4;
    return gfx;
}

#endif
