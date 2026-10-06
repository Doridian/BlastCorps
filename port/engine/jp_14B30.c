/*
 * hd_code 14B30 (drawtext.c: the text's quads), jp's func_80259EC4, which
 * the C has as jp's GLOBAL_ASM, as native C (engine.h; jp_E7B0.c says more).
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

u8 *func_8025B0B8(u16);
s32 func_8025B498(s16, u16, char *, u16 *);
void func_8029A7E4(char *, ...);

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

    if (arg2 != NULL) {
        if (*arg2 != 0xFFF) {
            kind = 1;
            wide = 1;
        } else {
            arg2 = NULL;
            func_8029A7E4("null jstring\n");
        }
    }
    if (wide) {
        if (arg2 == NULL)
            goto end;
        if (*arg2 == 0xFFE) {
            goto end;
        }
    } else {
        if (arg1 == NULL)
            goto end;
        if (*arg1 == 0)
            goto end;
    }
    s = arg1;
    t = arg2;
    if (arg9 == 0) {
        if (wide) {
            if (*t != 0xFFE) {
                do {
                    t++;
                } while (*t != 0xFFE);
            }
            t--;
        } else {
            if (*s != 0) {
                do {
                    s++;
                } while (*s != 0);
            }
            s--;
        }
    }
    if (arg4 != 0) {
        u32 scale;

        IDO_CVT_U_S(scale, arg7);
        c = func_8025B498(arg4, scale, arg1, arg2);
        arg5 = (f32)c;
    }
    if (done)
        goto end;
    do {
        if (kind == 0) {
            width = 0.21f;
            if ((u32)(*s - 0x20) >= 0x60) {
                glyph = 0;
                width = 0.0f;
            } else {
                switch (*s) {
                    case '0':
                        glyph = 0;
                        width = 0.25f;
                        break;
                    case '1':
                        glyph = 1;
                        width = 0.25f;
                        break;
                    case '2':
                        glyph = 2;
                        width = 0.25f;
                        break;
                    case '3':
                        glyph = 3;
                        width = 0.25f;
                        break;
                    case '4':
                        glyph = 4;
                        width = 0.25f;
                        break;
                    case '5':
                        glyph = 5;
                        width = 0.25f;
                        break;
                    case '6':
                        glyph = 6;
                        width = 0.25f;
                        break;
                    case '7':
                        glyph = 7;
                        width = 0.25f;
                        break;
                    case '8':
                        glyph = 8;
                        width = 0.25f;
                        break;
                    case '9':
                        glyph = 9;
                        width = 0.25f;
                        break;
                    case 'A':
                        glyph = 10;
                        break;
                    case 'B':
                        glyph = 11;
                        break;
                    case 'C':
                        glyph = 12;
                        break;
                    case 'D':
                        glyph = 13;
                        break;
                    case 0x7F:
                        glyph = 14;
                        width = 0.05f;
                        break;
                    case 'E':
                        glyph = 15;
                        break;
                    case 'F':
                        glyph = 16;
                        break;
                    case 'G':
                        glyph = 17;
                        break;
                    case 'H':
                        glyph = 18;
                        break;
                    case 'I':
                        glyph = 19;
                        width = 0.27f;
                        break;
                    case 'J':
                        glyph = 20;
                        break;
                    case 'K':
                        glyph = 21;
                        break;
                    case 'L':
                        glyph = 22;
                        break;
                    case 'M':
                        glyph = 23;
                        width = 0.05f;
                        break;
                    case 'N':
                        glyph = 24;
                        break;
                    case 'O':
                        glyph = 25;
                        break;
                    case 'P':
                        glyph = 26;
                        break;
                    case 'Q':
                        glyph = 27;
                        break;
                    case 'R':
                        glyph = 28;
                        break;
                    case 'S':
                        glyph = 29;
                        break;
                    case 'T':
                        glyph = 30;
                        break;
                    case 'U':
                        glyph = 31;
                        break;
                    case 'V':
                        glyph = 32;
                        break;
                    case 'W':
                        glyph = 33;
                        width = 0.05f;
                        break;
                    case 'X':
                        glyph = 34;
                        break;
                    case 'Y':
                        glyph = 35;
                        break;
                    case 'Z':
                        glyph = 36;
                        break;
                    case '\'':
                        glyph = 38;
                        width = 0.25f;
                        break;
                    case ')':
                        glyph = 39;
                        width = 0.25f;
                        break;
                    case ':':
                        glyph = 40;
                        width = 0.36f;
                        break;
                    case ',':
                        glyph = 41;
                        width = 0.25f;
                        break;
                    case '$':
                        glyph = 42;
                        break;
                    case '!':
                        glyph = 43;
                        width = 0.25f;
                        break;
                    case '.':
                        glyph = 44;
                        width = 0.3f;
                        break;
                    case '-':
                        glyph = 45;
                        width = 0.25f;
                        break;
                    case '(':
                        glyph = 46;
                        width = 0.25f;
                        break;
                    case '%':
                        glyph = 47;
                        break;
                    case '?':
                        glyph = 48;
                        break;
                    case '#':
                        glyph = 49;
                        break;
                    case '/':
                        glyph = 50;
                        break;
                    case ' ':
                    case '&':
                        glyph = 0;
                        width = 0.3f;
                        break;
                    case 'a':
                    case 'b':
                    case 'd':
                    case 'e':
                    case 'k':
                    case 'm':
                        glyph = *s - 0x22C;
                        break;
                    default:
                        glyph = 0;
                        width = 0.0f;
                        break;
                }
            }
            glyph += 0xF4C;
        } else {
            if (kind == 1) {
                width = 1.0f;
                if (*t < 0x200) {
                    glyph = *t + 0xD4C;
                } else {
                    if (*t == 0x1001) {
                        glyph = 0xF7D;
                    } else {
                        glyph = 0xD4C;
                    }
                }
            }
        }
        if (arg3 == 1) {
            if (arg9 != 0) {
                arg5 = arg5 - width * arg7;
            } else {
                arg5 = arg5 + width * arg7;
            }
        }
        if (wide) {
            c = *t;
        } else {
            c = *s;
        }
        if (D_802E8C94[kind] == c)
            goto skip;
        if (D_802E8C90[kind] == c)
            goto skip;
        if (D_802E8C8C[kind] == c)
            goto skip;
        if (arg13 == 0) {
            if (arg17 == 0) {
                if (arg21 == 0) {
                    if (arg25 == 0)
                        goto skip;
                }
            }
        }
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
            if (arg12 == 0) {
                if (arg11 == 0)
                    goto keep;
            }
        }
        D_80365340[D_802E8C74].unk0 += 0x8000;
    keep:
        D_80365340[D_802E8C74].unk4 = D_802E8C74;
        {
            u8 *tex = func_8025B0B8(glyph);

            D_80365340[D_802E8C74].unk8 = (s32)tex;
        }
        if (!(++D_802E8C74 < D_80365350)) {
            func_8029A7E4("\n\a --- ASSERTION FAULT - %s - %s, line %d\n\n", "index<maxCharacters", "drawtext.c", 435);
        }
        if (D_802E8C74 >= D_80365350) {
            func_8029A7E4("%d %d\n", D_802E8C74, D_80365350);
        }
    skip:
        if (arg3 == 1) {
            if (arg9 != 0) {
                arg5 = arg5 + (arg7 - width * arg7);
            } else {
                arg5 = arg5 - (arg7 - width * arg7);
            }
        } else {
            if (D_802E8C8C[kind] != c) {
                if (arg9 != 0) {
                    arg5 = arg5 + arg7 * D_802E8C84[kind];
                }
                if (arg9 == 0) {
                    arg5 = arg5 - arg7 * D_802E8C84[kind];
                }
            }
        }
        if (arg9 != 0) {
            if (wide) {
                if (*++t == 0xFFE) {
                    done = 1;
                }
            } else {
                if (*++s == 0) {
                    done = 1;
                }
            }
        } else if (wide) {
            if (t == arg2) {
                done = 1;
            }
            t--;
        } else {
            if (s == arg1) {
                done = 1;
            }
            s--;
        }
    } while (!done);
end:
}

#endif
