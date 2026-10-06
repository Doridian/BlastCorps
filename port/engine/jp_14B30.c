/*
 * hd_code 14B30 (drawtext.c: the text's quads), jp's func_80259EC4, which
 * the C has as jp's GLOBAL_ASM, as native C charged by the original's blocks
 * (engine.h; jp_E7B0.c says more).
 *
 * jp's draws u16 text (0x0FFE-terminated) where it is given some that
 * doesn't start with 0x0FFF, else the char text as the US versions do; text
 * that does start with 0x0FFF is reported ("null jstring") and dropped.
 */
#include "engine.h"
#include "game/game.h"

#ifdef VERSION_JP

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} UnkStruct_80365340; /* size = 0xC */

extern UnkStruct_80365340 *D_80365340;
extern Vtx *PTR32 D_80365348[2];
extern s32 D_80365350;
extern s32 D_802E8C74;
extern s32 D_802E8C78;
extern f32 D_802E8C84[];
extern u16 D_802E8C8C[];
extern u16 D_802E8C90[];
extern u16 D_802E8C94[];

/* a string's glyphs as quads at (arg5, arg6), arg7 by arg8, into the
   frame's vertices (D_80365348) and sort records (D_80365340): arg9 set
   left to right from its start, else right to left from its end; arg4 set,
   centred on arg4 (func_8025B498); arg3 1, proportional; the corners'
   colours arg10-13, arg14-17, arg18-21, arg22-25 */
void func_80259EC4(s32 arg0, u8 *arg1, u16 *arg2, u8 arg3, s32 arg4, f32 arg5, s32 arg6, f32 arg7, s32 arg8,
                   u8 arg9, u8 arg10, u8 arg11, u8 arg12, u8 arg13, u8 arg14, u8 arg15, u8 arg16, u8 arg17,
                   u8 arg18, u8 arg19, u8 arg20, u8 arg21, u8 arg22, u8 arg23, u8 arg24, u8 arg25) {
    u16 glyph = 0;
    u8 *s;
    u16 *t;
    u8 done = 0;
    f32 width = 0.0f;
    u8 wide = 0;
    u8 kind = 0;
    s32 c;
    Vtx *v;

    ENGINE_BLK(80259EC4);
    if (arg2 != NULL) {
        ENGINE_BLK(80259EF4);
        if (*arg2 != 0xFFF) {
            ENGINE_BLK(80259F04);
            kind = 1;
            wide = 1;
        } else {
            ENGINE_BLK(80259F18);
            arg2 = NULL;
            func_8029A7E4("null jstring\n");
        }
    }
    ENGINE_BLK(80259F28);
    if (wide) {
        ENGINE_BLK(80259F34);
        if (arg2 == NULL)
            goto end;
        ENGINE_BLK(80259F40);
        if (*arg2 == 0xFFE) {
            ENGINE_BLK(80259F50);
            goto end;
        }
    } else {
        ENGINE_BLK(80259F58);
        if (arg1 == NULL)
            goto end;
        ENGINE_BLK(80259F64);
        if (*arg1 == 0)
            goto end;
    }
    ENGINE_BLK(80259F70);
    s = arg1;
    t = arg2;
    if (arg9 == 0) {
        ENGINE_BLK(80259F88);
        if (wide) {
            ENGINE_BLK(80259F94);
            if (*t != 0xFFE) {
                do {
                    ENGINE_BLK(80259FA4);
                    t++;
                } while (*t != 0xFFE);
            }
            ENGINE_BLK(80259FC4);
            t--;
        } else {
            ENGINE_BLK(80259FD4);
            if (*s != 0) {
                do {
                    ENGINE_BLK(80259FE4);
                    s++;
                } while (*s != 0);
            }
            ENGINE_BLK(8025A000);
            s--;
        }
    }
    ENGINE_BLK(8025A00C);
    if (arg4 != 0) {
        u32 scale;

        ENGINE_BLK(8025A018);
        IDO_CVT_U_S(scale, arg7, 8025A048, 8025A078, 8025A088, 8025A090);
        ENGINE_BLK(8025A0A0);
        c = func_8025B498(arg4, scale, arg1, arg2);
        ENGINE_BLK(8025A0B0);
        arg5 = (f32)c;
    }
    ENGINE_BLK(8025A0C0);
    if (done)
        goto end;
    do {
        ENGINE_BLK(8025A0CC);
        if (kind == 0) {
            ENGINE_BLK(8025A0E8);
            width = 0.21f;
            if ((u32)(*s - 0x20) >= 0x60) {
                ENGINE_BLK(8025A4A8);
                glyph = 0;
                width = 0.0f;
            } else {
                ENGINE_BLK(8025A10C);
                switch (*s) {
                    case '0':
                        ENGINE_BLK(8025A124);
                        glyph = 0;
                        width = 0.25f;
                        break;
                    case '1':
                        ENGINE_BLK(8025A138);
                        glyph = 1;
                        width = 0.25f;
                        break;
                    case '2':
                        ENGINE_BLK(8025A150);
                        glyph = 2;
                        width = 0.25f;
                        break;
                    case '3':
                        ENGINE_BLK(8025A168);
                        glyph = 3;
                        width = 0.25f;
                        break;
                    case '4':
                        ENGINE_BLK(8025A180);
                        glyph = 4;
                        width = 0.25f;
                        break;
                    case '5':
                        ENGINE_BLK(8025A198);
                        glyph = 5;
                        width = 0.25f;
                        break;
                    case '6':
                        ENGINE_BLK(8025A1B0);
                        glyph = 6;
                        width = 0.25f;
                        break;
                    case '7':
                        ENGINE_BLK(8025A1C8);
                        glyph = 7;
                        width = 0.25f;
                        break;
                    case '8':
                        ENGINE_BLK(8025A1E0);
                        glyph = 8;
                        width = 0.25f;
                        break;
                    case '9':
                        ENGINE_BLK(8025A1F8);
                        glyph = 9;
                        width = 0.25f;
                        break;
                    case 'A':
                        ENGINE_BLK(8025A210);
                        glyph = 10;
                        break;
                    case 'B':
                        ENGINE_BLK(8025A21C);
                        glyph = 11;
                        break;
                    case 'C':
                        ENGINE_BLK(8025A228);
                        glyph = 12;
                        break;
                    case 'D':
                        ENGINE_BLK(8025A234);
                        glyph = 13;
                        break;
                    case 0x7F:
                        ENGINE_BLK(8025A240);
                        glyph = 14;
                        width = 0.05f;
                        break;
                    case 'E':
                        ENGINE_BLK(8025A258);
                        glyph = 15;
                        break;
                    case 'F':
                        ENGINE_BLK(8025A264);
                        glyph = 16;
                        break;
                    case 'G':
                        ENGINE_BLK(8025A270);
                        glyph = 17;
                        break;
                    case 'H':
                        ENGINE_BLK(8025A27C);
                        glyph = 18;
                        break;
                    case 'I':
                        ENGINE_BLK(8025A288);
                        glyph = 19;
                        width = 0.27f;
                        break;
                    case 'J':
                        ENGINE_BLK(8025A2A0);
                        glyph = 20;
                        break;
                    case 'K':
                        ENGINE_BLK(8025A2AC);
                        glyph = 21;
                        break;
                    case 'L':
                        ENGINE_BLK(8025A2B8);
                        glyph = 22;
                        break;
                    case 'M':
                        ENGINE_BLK(8025A2C4);
                        glyph = 23;
                        width = 0.05f;
                        break;
                    case 'N':
                        ENGINE_BLK(8025A2DC);
                        glyph = 24;
                        break;
                    case 'O':
                        ENGINE_BLK(8025A2E8);
                        glyph = 25;
                        break;
                    case 'P':
                        ENGINE_BLK(8025A2F4);
                        glyph = 26;
                        break;
                    case 'Q':
                        ENGINE_BLK(8025A300);
                        glyph = 27;
                        break;
                    case 'R':
                        ENGINE_BLK(8025A30C);
                        glyph = 28;
                        break;
                    case 'S':
                        ENGINE_BLK(8025A318);
                        glyph = 29;
                        break;
                    case 'T':
                        ENGINE_BLK(8025A324);
                        glyph = 30;
                        break;
                    case 'U':
                        ENGINE_BLK(8025A330);
                        glyph = 31;
                        break;
                    case 'V':
                        ENGINE_BLK(8025A33C);
                        glyph = 32;
                        break;
                    case 'W':
                        ENGINE_BLK(8025A348);
                        glyph = 33;
                        width = 0.05f;
                        break;
                    case 'X':
                        ENGINE_BLK(8025A360);
                        glyph = 34;
                        break;
                    case 'Y':
                        ENGINE_BLK(8025A36C);
                        glyph = 35;
                        break;
                    case 'Z':
                        ENGINE_BLK(8025A378);
                        glyph = 36;
                        break;
                    case '\'':
                        ENGINE_BLK(8025A384);
                        glyph = 38;
                        width = 0.25f;
                        break;
                    case ')':
                        ENGINE_BLK(8025A39C);
                        glyph = 39;
                        width = 0.25f;
                        break;
                    case ':':
                        ENGINE_BLK(8025A3B4);
                        glyph = 40;
                        width = 0.36f;
                        break;
                    case ',':
                        ENGINE_BLK(8025A3CC);
                        glyph = 41;
                        width = 0.25f;
                        break;
                    case '$':
                        ENGINE_BLK(8025A3E4);
                        glyph = 42;
                        break;
                    case '!':
                        ENGINE_BLK(8025A3F0);
                        glyph = 43;
                        width = 0.25f;
                        break;
                    case '.':
                        ENGINE_BLK(8025A408);
                        glyph = 44;
                        width = 0.3f;
                        break;
                    case '-':
                        ENGINE_BLK(8025A420);
                        glyph = 45;
                        width = 0.25f;
                        break;
                    case '(':
                        ENGINE_BLK(8025A438);
                        glyph = 46;
                        width = 0.25f;
                        break;
                    case '%':
                        ENGINE_BLK(8025A450);
                        glyph = 47;
                        break;
                    case '?':
                        ENGINE_BLK(8025A45C);
                        glyph = 48;
                        break;
                    case '#':
                        ENGINE_BLK(8025A468);
                        glyph = 49;
                        break;
                    case '/':
                        ENGINE_BLK(8025A474);
                        glyph = 50;
                        break;
                    case ' ':
                    case '&':
                        ENGINE_BLK(8025A480);
                        glyph = 0;
                        width = 0.3f;
                        break;
                    case 'a':
                    case 'b':
                    case 'd':
                    case 'e':
                    case 'k':
                    case 'm':
                        ENGINE_BLK(8025A494);
                        glyph = *s - 0x22C;
                        break;
                    default:
                        ENGINE_BLK(8025A4A8);
                        glyph = 0;
                        width = 0.0f;
                        break;
                }
            }
            ENGINE_BLK(8025A4B4);
            glyph += 0xF4C;
        } else {
            ENGINE_BLK(8025A0D8);
            if (kind == 1) {
                ENGINE_BLK(8025A4C4);
                width = 1.0f;
                if (*t < 0x200) {
                    ENGINE_BLK(8025A4E4);
                    glyph = *t + 0xD4C;
                } else {
                    ENGINE_BLK(8025A4F0);
                    if (*t == 0x1001) {
                        ENGINE_BLK(8025A504);
                        glyph = 0xF7D;
                    } else {
                        ENGINE_BLK(8025A510);
                        glyph = 0xD4C;
                    }
                }
            } else {
                ENGINE_BLK(8025A0E0);
            }
        }
        ENGINE_BLK(8025A518);
        if (arg3 == 1) {
            ENGINE_BLK(8025A528);
            if (arg9 != 0) {
                ENGINE_BLK(8025A534);
                arg5 = arg5 - width * arg7;
            } else {
                ENGINE_BLK(8025A550);
                arg5 = arg5 + width * arg7;
            }
        }
        ENGINE_BLK(8025A568);
        if (wide) {
            ENGINE_BLK(8025A574);
            c = *t;
        } else {
            ENGINE_BLK(8025A584);
            c = *s;
        }
        ENGINE_BLK(8025A590);
        if (D_802E8C94[kind] == c)
            goto skip;
        ENGINE_BLK(8025A5B0);
        if (D_802E8C90[kind] == c)
            goto skip;
        ENGINE_BLK(8025A5C4);
        if (D_802E8C8C[kind] == c)
            goto skip;
        ENGINE_BLK(8025A5D8);
        if (arg13 == 0) {
            ENGINE_BLK(8025A5E4);
            if (arg17 == 0) {
                ENGINE_BLK(8025A5F0);
                if (arg21 == 0) {
                    ENGINE_BLK(8025A5FC);
                    if (arg25 == 0)
                        goto skip;
                }
            }
        }
        ENGINE_BLK(8025A608);
        v = &D_80365348[D_8035805C][D_802E8C78];
        v->v.ob[0] = engine_trunc_w_s(arg5);
        v->v.ob[1] = arg6;
        v->v.ob[2] = -10;
        v->v.flag = 0;
        v->v.tc[0] = 0;
        v->v.tc[1] = 0x3E0;
        v->v.cn[0] = arg10;
        v->v.cn[1] = arg11;
        v->v.cn[2] = arg12;
        v->v.cn[3] = arg13;
        D_802E8C78++;
        v = &D_80365348[D_8035805C][D_802E8C78];
        v->v.ob[0] = engine_trunc_w_s(arg5 + arg7);
        v->v.ob[1] = arg6;
        v->v.ob[2] = -10;
        v->v.flag = 0;
        v->v.tc[0] = 0x3E0;
        v->v.tc[1] = 0x3E0;
        v->v.cn[0] = arg14;
        v->v.cn[1] = arg15;
        v->v.cn[2] = arg16;
        v->v.cn[3] = arg17;
        D_802E8C78++;
        v = &D_80365348[D_8035805C][D_802E8C78];
        v->v.ob[0] = engine_trunc_w_s(arg5 + arg7);
        v->v.ob[1] = arg6 + arg8;
        v->v.ob[2] = -10;
        v->v.flag = 0;
        v->v.tc[0] = 0x3E0;
        v->v.tc[1] = 0;
        v->v.cn[0] = arg18;
        v->v.cn[1] = arg19;
        v->v.cn[2] = arg20;
        v->v.cn[3] = arg21;
        D_802E8C78++;
        v = &D_80365348[D_8035805C][D_802E8C78];
        v->v.ob[0] = engine_trunc_w_s(arg5);
        v->v.ob[1] = arg6 + arg8;
        v->v.ob[2] = -10;
        v->v.flag = 0;
        v->v.tc[0] = 0;
        v->v.tc[1] = 0;
        v->v.cn[0] = arg22;
        v->v.cn[1] = arg23;
        v->v.cn[2] = arg24;
        v->v.cn[3] = arg25;
        D_802E8C78++;
        D_80365340[D_802E8C74].unk0 = c;
        if (arg10 == 0) {
            ENGINE_BLK(8025AE0C);
            if (arg12 == 0) {
                ENGINE_BLK(8025AE18);
                if (arg11 == 0)
                    goto keep;
            }
        }
        ENGINE_BLK(8025AE24);
        D_80365340[D_802E8C74].unk0 += 0x8000;
    keep:
        ENGINE_BLK(8025AE54);
        D_80365340[D_802E8C74].unk4 = D_802E8C74;
        {
            u8 *tex = func_8025B0B8(glyph);

            ENGINE_BLK(8025AE80);
            D_80365340[D_802E8C74].unk8 = (s32)tex;
        }
        if (!(++D_802E8C74 < D_80365350)) {
            ENGINE_BLK(8025AECC);
            func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n", "index<maxCharacters", "drawtext.c", 435);
        }
        ENGINE_BLK(8025AEEC);
        if (D_802E8C74 >= D_80365350) {
            ENGINE_BLK(8025AF08);
            func_8029A7E4("%d %d\n", D_802E8C74, D_80365350);
        }
    skip:
        ENGINE_BLK(8025AF1C);
        if (arg3 == 1) {
            ENGINE_BLK(8025AF2C);
            if (arg9 != 0) {
                ENGINE_BLK(8025AF38);
                arg5 = arg5 + (arg7 - width * arg7);
            } else {
                ENGINE_BLK(8025AF58);
                arg5 = arg5 - (arg7 - width * arg7);
            }
        } else {
            ENGINE_BLK(8025AF78);
            if (D_802E8C8C[kind] != c) {
                ENGINE_BLK(8025AF98);
                if (arg9 != 0) {
                    ENGINE_BLK(8025AFA4);
                    arg5 = arg5 + arg7 * D_802E8C84[kind];
                }
                ENGINE_BLK(8025AFC8);
                if (arg9 == 0) {
                    ENGINE_BLK(8025AFD4);
                    arg5 = arg5 - arg7 * D_802E8C84[kind];
                }
            }
        }
        ENGINE_BLK(8025AFFC);
        if (arg9 != 0) {
            if (wide) {
                ENGINE_BLK(8025B008);
                ENGINE_BLK(8025B014);
                if (*++t == 0xFFE) {
                    ENGINE_BLK(8025B030);
                    done = 1;
                }
            } else {
                ENGINE_BLK(8025B008);
                ENGINE_BLK(8025B03C);
                if (*++s == 0) {
                    ENGINE_BLK(8025B054);
                    done = 1;
                }
            }
        } else if (wide) {
            ENGINE_BLK(8025B060);
            ENGINE_BLK(8025B06C);
            if (t == arg2) {
                ENGINE_BLK(8025B07C);
                done = 1;
            }
            ENGINE_BLK(8025B084);
            t--;
        } else {
            ENGINE_BLK(8025B060);
            ENGINE_BLK(8025B094);
            if (s == arg1) {
                ENGINE_BLK(8025B0A4);
                done = 1;
            }
            ENGINE_BLK(8025B0AC);
            s--;
        }
        ENGINE_BLK(8025B0B8);
    } while (!done);
end:
    ENGINE_BLK(8025B0C4);
}

#endif
