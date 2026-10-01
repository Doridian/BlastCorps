/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the Fast3D
 * display list, as the game's C builds it and the port's renderer
 * (port/host/gfx.c) reads it.  Written from the RSP microcode's and the
 * RDP's documented command formats (n64brew's "RDP commands" and "Fast3D"
 * pages): the opcodes, the bit fields of each command, the other-mode,
 * combiner and blender encodings and the structs the commands point at.
 * Only what the game uses is here.
 *
 * The macros keep the C types of the forms the game was written against
 * (a field is an `unsigned int`, the RSP's immediate opcodes are negative
 * `int`s), because the game's objects have to come out the same with these
 * headers as with the SDK's (port/tools/sdk_identity.py).
 */
#ifndef PORT_SDK_GBI_H
#define PORT_SDK_GBI_H

#include <PR/ultratypes.h>

/* A word of a display list holding a pointer: in the LP64 port through a
   32-bit pointer, so that the conversion is the game's 32-bit address */
#if defined(PORT_LP64)
#define _GBI_W(x) ((unsigned int)(void *PTR32)(x))
#else
#define _GBI_W(x) ((unsigned int)(x))
#endif

/* v's low w bits, at bit s of a command word */
#define _SHIFTL(v, s, w) ((unsigned int)(((unsigned int)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFT _SHIFTL

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* ---- opcodes ------------------------------------------------------------------
 *
 * The RSP's DMA commands count up from 0, its immediate ones down from 0xBF
 * (-65 as a signed byte); the RDP's own run down from 0xFF.
 */
#define G_SPNOOP 0
#define G_MTX 1
#define G_MOVEMEM 3
#define G_VTX 4
#define G_DL 6

#define G_IMMFIRST -65
#define G_TRI1 (G_IMMFIRST - 0)                 /* 0xBF */
#define G_CULLDL (G_IMMFIRST - 1)
#define G_POPMTX (G_IMMFIRST - 2)
#define G_MOVEWORD (G_IMMFIRST - 3)
#define G_TEXTURE (G_IMMFIRST - 4)
#define G_SETOTHERMODE_H (G_IMMFIRST - 5)
#define G_SETOTHERMODE_L (G_IMMFIRST - 6)
#define G_ENDDL (G_IMMFIRST - 7)
#define G_SETGEOMETRYMODE (G_IMMFIRST - 8)
#define G_CLEARGEOMETRYMODE (G_IMMFIRST - 9)
#define G_LINE3D (G_IMMFIRST - 10)
#define G_RDPHALF_1 (G_IMMFIRST - 11)
#define G_RDPHALF_2 (G_IMMFIRST - 12)
#define G_RDPHALF_CONT (G_IMMFIRST - 13)    /* 0xB2 */

#define G_SETCIMG 0xff
#define G_SETZIMG 0xfe
#define G_SETTIMG 0xfd
#define G_SETCOMBINE 0xfc
#define G_SETENVCOLOR 0xfb
#define G_SETPRIMCOLOR 0xfa
#define G_SETBLENDCOLOR 0xf9
#define G_SETFOGCOLOR 0xf8
#define G_SETFILLCOLOR 0xf7
#define G_FILLRECT 0xf6
#define G_SETTILE 0xf5
#define G_LOADTILE 0xf4
#define G_LOADBLOCK 0xf3
#define G_SETTILESIZE 0xf2
#define G_LOADTLUT 0xf0
#define G_RDPSETOTHERMODE 0xef
#define G_SETPRIMDEPTH 0xee
#define G_SETSCISSOR 0xed
#define G_SETCONVERT 0xec
#define G_SETKEYR 0xeb
#define G_SETKEYGB 0xea
#define G_RDPFULLSYNC 0xe9
#define G_RDPTILESYNC 0xe8
#define G_RDPPIPESYNC 0xe7
#define G_RDPLOADSYNC 0xe6
#define G_TEXRECTFLIP 0xe5
#define G_TEXRECT 0xe4

/* ---- what the commands point at ------------------------------------------------ */

typedef struct {
    short ob[3];                /* position */
    unsigned short flag;
    short tc[2];                /* texture coordinates, s10.5 */
    unsigned char cn[4];        /* colour and alpha */
} Vtx_t;

typedef struct {
    short ob[3];
    unsigned short flag;
    short tc[2];
    signed char n[3];           /* the normal, for lighting */
    unsigned char a;
} Vtx_tn;

typedef union {
    Vtx_t v;
    Vtx_tn n;
    long long int force_structure_alignment;
} Vtx;

/* s15.16: the integer halves of the 16 elements, then their fractions
   (port/src/gu.c) */
typedef int Mtx_t[4][4];
typedef union {
    Mtx_t m;
    long long int force_structure_alignment;
} Mtx;

/* the viewport: scale and translation, x y z and a pad, with 2 bits of
   fraction */
typedef struct {
    short vscale[4];
    short vtrans[4];
} Vp_t;
typedef union {
    Vp_t vp;
    long long int force_structure_alignment;
} Vp;

/* a directional light: its colour twice, and its direction */
typedef struct {
    unsigned char col[3];
    char pad1;
    unsigned char colc[3];
    char pad2;
    signed char dir[3];
    char pad3;
} Light_t;
typedef struct {
    unsigned char col[3];
    char pad1;
    unsigned char colc[3];
    char pad2;
} Ambient_t;
typedef union {
    Light_t l;
    long long int force_structure_alignment[2];
} Light;
typedef union {
    Ambient_t l;
    long long int force_structure_alignment[1];
} Ambient;
typedef struct {
    Ambient a;
    Light l[1];
} Lights1;
typedef struct {
    Ambient a;
    Light l[2];
} Lights2;
/* the reflection mapping's two directions (guLookAtReflect) */
typedef struct {
    Light l[2];
} LookAt;

/* ---- the display list ----------------------------------------------------------- */

typedef struct {
    unsigned int w0;
    unsigned int w1;
} Gwords;

typedef union {
    Gwords words;
    long long int force_structure_alignment;
} Gfx;

/* the forms most commands take */
#define G_CMD(pkt, w0v, w1v)                                                                                \
    {                                                                                                       \
        Gfx *_g = (Gfx *)(pkt);                                                                             \
                                                                                                            \
        _g->words.w0 = w0v;                                                                                 \
        _g->words.w1 = w1v;                                                                                 \
    }

#define gDma1p(pkt, c, s, l, p)                                                                             \
    G_CMD(pkt, (_SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) | _SHIFTL((l), 0, 16)), _GBI_W(s))
#define gsDma1p(c, s, l, p) { (_SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) | _SHIFTL((l), 0, 16)), _GBI_W(s) }
#define gImmp1(pkt, c, p0) G_CMD(pkt, _SHIFTL((c), 24, 8), _GBI_W(p0))
#define gImmp21(pkt, c, p0, p1, dat)                                                                        \
    G_CMD(pkt, (_SHIFTL((c), 24, 8) | _SHIFTL((p0), 8, 16) | _SHIFTL((p1), 0, 8)), (unsigned int)(dat))
#define gsImmp21(c, p0, p1, dat) { _SHIFTL((c), 24, 8) | _SHIFTL((p0), 8, 16) | _SHIFTL((p1), 0, 8), (unsigned int)(dat) }
#define gDPNoParam(pkt, cmd) G_CMD(pkt, _SHIFTL(cmd, 24, 8), 0)
#define gsDPNoParam(cmd) { _SHIFTL(cmd, 24, 8), 0 }

/* ---- the RSP: matrices, vertices, triangles, lists --------------------------------- */

#define G_MTX_MODELVIEW 0x00
#define G_MTX_PROJECTION 0x01
#define G_MTX_MUL 0x00
#define G_MTX_LOAD 0x02
#define G_MTX_NOPUSH 0x00
#define G_MTX_PUSH 0x04

#define G_DL_PUSH 0x00
#define G_DL_NOPUSH 0x01

#define gSPMatrix(pkt, m, p) gDma1p(pkt, G_MTX, m, sizeof(Mtx), p)
#define gSPPopMatrix(pkt, n) gImmp1(pkt, G_POPMTX, n)
/* n vertices into the vertex buffer from v0 on */
#define gSPVertex(pkt, v, n, v0) gDma1p(pkt, G_VTX, v, sizeof(Vtx) * (n), ((n) - 1) << 4 | (v0))
#define gsSPVertex(v, n, v0) gsDma1p(G_VTX, v, sizeof(Vtx) * (n), ((n) - 1) << 4 | (v0))
#define gSPDisplayList(pkt, dl) gDma1p(pkt, G_DL, dl, 0, G_DL_PUSH)
#define gSPEndDisplayList(pkt) G_CMD(pkt, _SHIFTL(G_ENDDL, 24, 8), 0)
#define gsSPEndDisplayList() { _SHIFTL(G_ENDDL, 24, 8), 0 }

/* triangles and lines: vertex buffer indices times 10, the microcode's
   stride */
#define __gsSP1Triangle_w1f(v0, v1, v2, flag)                                                               \
    (_SHIFTL((flag), 24, 8) | _SHIFTL((v0) * 10, 16, 8) | _SHIFTL((v1) * 10, 8, 8) | _SHIFTL((v2) * 10, 0, 8))
#define gSP1Triangle(pkt, v0, v1, v2, flag) G_CMD(pkt, _SHIFTL(G_TRI1, 24, 8), __gsSP1Triangle_w1f(v0, v1, v2, flag))
#define gsSP1Triangle(v0, v1, v2, flag) { _SHIFTL(G_TRI1, 24, 8), __gsSP1Triangle_w1f(v0, v1, v2, flag) }
#define __gsSPLine3D_w1f(v0, v1, wd, flag)                                                                  \
    (_SHIFTL((flag), 24, 8) | _SHIFTL((v0) * 10, 16, 8) | _SHIFTL((v1) * 10, 8, 8) | _SHIFTL((wd), 0, 8))
#define gSPLineW3D(pkt, v0, v1, wd, flag) G_CMD(pkt, _SHIFTL(G_LINE3D, 24, 8), __gsSPLine3D_w1f(v0, v1, wd, flag))

/* skip the rest of the list when vertices vstart..vend (bounding a model) are
   all outside the view: buffer offsets, 40 bytes a vertex */
#define gSPCullDisplayList(pkt, vstart, vend)                                                               \
    G_CMD(pkt, _SHIFTL(G_CULLDL, 24, 8) | ((0x0f & (vstart)) * 40), _GBI_W((0x0f & ((vend) + 1)) * 40))

/* the geometry mode */
#define G_ZBUFFER 0x00000001
#define G_SHADE 0x00000004
#define G_SHADING_SMOOTH 0x00000200
#define G_CULL_FRONT 0x00001000
#define G_CULL_BACK 0x00002000
#define G_CULL_BOTH 0x00003000
#define G_FOG 0x00010000
#define G_LIGHTING 0x00020000
#define G_TEXTURE_GEN 0x00040000
#define G_TEXTURE_GEN_LINEAR 0x00080000
#define G_LOD 0x00100000

#define gSPSetGeometryMode(pkt, word) G_CMD(pkt, _SHIFTL(G_SETGEOMETRYMODE, 24, 8), _GBI_W(word))
#define gsSPSetGeometryMode(word) { _SHIFTL(G_SETGEOMETRYMODE, 24, 8), _GBI_W(word) }
#define gSPClearGeometryMode(pkt, word) G_CMD(pkt, _SHIFTL(G_CLEARGEOMETRYMODE, 24, 8), _GBI_W(word))
#define gsSPClearGeometryMode(word) { _SHIFTL(G_CLEARGEOMETRYMODE, 24, 8), _GBI_W(word) }

/* the texture scale and the tile drawn with */
#define BOWTIE_VAL 0
#define gSPTexture(pkt, s, t, level, tile, on)                                                              \
    G_CMD(pkt,                                                                                              \
          (_SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(BOWTIE_VAL, 16, 8) | _SHIFTL(level, 11, 3) |                 \
           _SHIFTL(tile, 8, 3) | _SHIFTL(on, 0, 8)),                                                        \
          (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16)))

/* MOVEMEM: a block into the microcode's DMEM */
#define G_MV_VIEWPORT 0x80
#define G_MV_LOOKATY 0x82
#define G_MV_LOOKATX 0x84
#define G_MV_L0 0x86

#define gSPViewport(pkt, v) gDma1p(pkt, G_MOVEMEM, (v), sizeof(Vp), G_MV_VIEWPORT)
#define gSPLight(pkt, l, n) gDma1p(pkt, G_MOVEMEM, l, sizeof(Light), ((n) - 1) * 2 + G_MV_L0)
#define gSPLookAtX(pkt, l) gDma1p(pkt, G_MOVEMEM, l, sizeof(Light), G_MV_LOOKATX)
#define gSPLookAtY(pkt, l) gDma1p(pkt, G_MOVEMEM, l, sizeof(Light), G_MV_LOOKATY)
#define gSPLookAt(pkt, la)                                                                                  \
    {                                                                                                       \
        gSPLookAtX(pkt, la) gSPLookAtY(pkt, (char *)(la) + 16)                                              \
    }

/* MOVEWORD: one word into DMEM: segments, lights' count, clip ratio, a
   vertex's field */
#define G_MW_NUMLIGHT 0x02
#define G_MW_CLIP 0x04
#define G_MW_SEGMENT 0x06
#define G_MW_POINTS 0x0c
#define G_MWO_NUMLIGHT 0x00
#define G_MWO_CLIP_RNX 0x04
#define G_MWO_CLIP_RNY 0x0c
#define G_MWO_CLIP_RPX 0x14
#define G_MWO_CLIP_RPY 0x1c
#define G_MWO_POINT_RGBA 0x10
#define G_MWO_POINT_ST 0x14
#define G_MWO_POINT_XYSCREEN 0x18
#define G_MWO_POINT_ZSCREEN 0x1c

#define gMoveWd(pkt, index, offset, data) gImmp21((pkt), G_MOVEWORD, offset, index, data)
#define gsMoveWd(index, offset, data) gsImmp21(G_MOVEWORD, offset, index, data)
#define gSPSegment(pkt, segment, base) gMoveWd(pkt, G_MW_SEGMENT, (segment) * 4, base)
/* a vertex's field (G_MWO_POINT_*), 40 bytes a vertex */
#define gSPModifyVertex(pkt, vtx, where, val) gMoveWd(pkt, G_MW_POINTS, (vtx) * 40 + (where), val)
#define gsSPModifyVertex(vtx, where, val) gsMoveWd(G_MW_POINTS, (vtx) * 40 + (where), val)

/* the lights: their count as the microcode keeps it, then each one */
#define NUMLIGHTS_1 1
#define NUMLIGHTS_2 2
#define NUML(n) (((n) + 1) * 32 + 0x80000000)
#define gSPNumLights(pkt, n) gMoveWd(pkt, G_MW_NUMLIGHT, G_MWO_NUMLIGHT, NUML(n))
#define gSPSetLights1(pkt, name)                                                                            \
    {                                                                                                       \
        gSPNumLights(pkt, NUMLIGHTS_1);                                                                     \
        gSPLight(pkt, &name.l[0], 1);                                                                       \
        gSPLight(pkt, &name.a, 2);                                                                          \
    }
#define gSPSetLights2(pkt, name)                                                                            \
    {                                                                                                       \
        gSPNumLights(pkt, NUMLIGHTS_2);                                                                     \
        gSPLight(pkt, &name.l[0], 1);                                                                       \
        gSPLight(pkt, &name.l[1], 2);                                                                       \
        gSPLight(pkt, &name.a, 3);                                                                          \
    }

/* the clip ratio: how far outside the screen the guard band reaches */
#define FR_NEG_FRUSTRATIO_1 0x00000001
#define FR_POS_FRUSTRATIO_1 0x0000ffff
#define FR_NEG_FRUSTRATIO_2 0x00000002
#define FR_POS_FRUSTRATIO_2 0x0000fffe
#define FR_NEG_FRUSTRATIO_3 0x00000003
#define FR_POS_FRUSTRATIO_3 0x0000fffd
#define FR_NEG_FRUSTRATIO_4 0x00000004
#define FR_POS_FRUSTRATIO_4 0x0000fffc
#define FR_NEG_FRUSTRATIO_5 0x00000005
#define FR_POS_FRUSTRATIO_5 0x0000fffb
#define FR_NEG_FRUSTRATIO_6 0x00000006
#define FR_POS_FRUSTRATIO_6 0x0000fffa
#define gSPClipRatio(pkt, r)                                                                                \
    {                                                                                                       \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNX, FR_NEG_##r);                                                \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNY, FR_NEG_##r);                                                \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPX, FR_POS_##r);                                                \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPY, FR_POS_##r);                                                \
    }

/* the perspective normalization (gu.h's guPerspective), and the texture
   rectangle: common.h has them as the game's older microcode takes them */
#define gSPPerspNormalize(pkt, s) gImmp1(pkt, G_RDPHALF_1, (s))
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)                                    \
    {                                                                                                       \
        G_CMD(pkt, (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12)),                  \
              (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12)));                           \
        gImmp1(pkt, G_RDPHALF_2, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16)));                                 \
        gImmp1(pkt, G_RDPHALF_CONT, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16)));                        \
    }
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)                                \
    {                                                                                                       \
        G_CMD(pkt, (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(MAX(xh, 0), 12, 12) | _SHIFTL(MAX(yh, 0), 0, 12)),  \
              (_SHIFTL(tile, 24, 3) | _SHIFTL(MAX(xl, 0), 12, 12) | _SHIFTL(MAX(yl, 0), 0, 12)));           \
        gImmp1(pkt, G_RDPHALF_2,                                                                            \
               (_SHIFTL((s) - MIN(((xl) * (dsdx)) >> 7, 0), 16, 16) |                                       \
                _SHIFTL((t) - MIN(((yl) * (dtdy)) >> 7, 0), 0, 16)));                                       \
        gImmp1(pkt, G_RDPHALF_CONT, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16)));                        \
    }

/* ---- the RDP's other modes ---------------------------------------------------------
 *
 * SetOtherMode's high word: where each field starts.
 */
#define G_MDSFT_ALPHADITHER 4
#define G_MDSFT_RGBDITHER 6
#define G_MDSFT_COMBKEY 8
#define G_MDSFT_TEXTCONV 9
#define G_MDSFT_TEXTFILT 12
#define G_MDSFT_TEXTLUT 14
#define G_MDSFT_TEXTLOD 16
#define G_MDSFT_TEXTDETAIL 17
#define G_MDSFT_TEXTPERSP 19
#define G_MDSFT_CYCLETYPE 20
#define G_MDSFT_PIPELINE 23
/* and its low word's */
#define G_MDSFT_ALPHACOMPARE 0
#define G_MDSFT_ZSRCSEL 2
#define G_MDSFT_RENDERMODE 3

#define G_PM_1PRIMITIVE (1 << G_MDSFT_PIPELINE)
#define G_PM_NPRIMITIVE (0 << G_MDSFT_PIPELINE)
#define G_CYC_1CYCLE (0 << G_MDSFT_CYCLETYPE)
#define G_CYC_2CYCLE (1 << G_MDSFT_CYCLETYPE)
#define G_CYC_COPY (2 << G_MDSFT_CYCLETYPE)
#define G_CYC_FILL (3 << G_MDSFT_CYCLETYPE)
#define G_TP_NONE (0 << G_MDSFT_TEXTPERSP)
#define G_TP_PERSP (1 << G_MDSFT_TEXTPERSP)
#define G_TL_TILE (0 << G_MDSFT_TEXTLOD)
#define G_TL_LOD (1 << G_MDSFT_TEXTLOD)
#define G_TF_POINT (0 << G_MDSFT_TEXTFILT)
#define G_TF_AVERAGE (3 << G_MDSFT_TEXTFILT)
#define G_TF_BILERP (2 << G_MDSFT_TEXTFILT)
#define G_CD_MAGICSQ (0 << G_MDSFT_RGBDITHER)
#define G_CD_BAYER (1 << G_MDSFT_RGBDITHER)
#define G_CD_NOISE (2 << G_MDSFT_RGBDITHER)
#define G_CD_DISABLE (3 << G_MDSFT_RGBDITHER)
#define G_AD_PATTERN (0 << G_MDSFT_ALPHADITHER)
#define G_AD_NOTPATTERN (1 << G_MDSFT_ALPHADITHER)
#define G_AD_NOISE (2 << G_MDSFT_ALPHADITHER)
#define G_AD_DISABLE (3 << G_MDSFT_ALPHADITHER)

#define gSPSetOtherMode(pkt, cmd, sft, len, data)                                                           \
    G_CMD(pkt, (_SHIFTL(cmd, 24, 8) | _SHIFTL(sft, 8, 8) | _SHIFTL(len, 0, 8)), _GBI_W(data))
#define gDPPipelineMode(pkt, mode) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_PIPELINE, 1, mode)
#define gDPSetCycleType(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2, type)
#define gDPSetTexturePersp(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTPERSP, 1, type)
#define gDPSetTextureLOD(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLOD, 1, type)
#define gDPSetTextureFilter(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTFILT, 2, type)
#define gDPSetColorDither(pkt, mode) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_RGBDITHER, 2, mode)
#define gDPSetAlphaDither(pkt, mode) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 2, mode)
/* the render mode: the 29 bits above G_MDSFT_RENDERMODE, both cycles' */
#define gDPSetRenderMode(pkt, c0, c1) gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, (c0) | (c1))

/* the render mode's flags (the low word's bits 3-15) */
#define AA_EN 0x8
#define Z_CMP 0x10
#define Z_UPD 0x20
#define IM_RD 0x40
#define CLR_ON_CVG 0x80
#define CVG_DST_CLAMP 0
#define CVG_DST_WRAP 0x100
#define CVG_DST_FULL 0x200
#define CVG_DST_SAVE 0x300
#define ZMODE_OPA 0
#define ZMODE_INTER 0x400
#define ZMODE_XLU 0x800
#define ZMODE_DEC 0xc00
#define CVG_X_ALPHA 0x1000
#define ALPHA_CVG_SEL 0x2000
#define FORCE_BL 0x4000
#define TEX_EDGE 0x0000

/* the blender's formula, (p a + m b) / (a + b), for each cycle: p and m the
   colours, a and b the factors */
#define G_BL_CLR_IN 0
#define G_BL_CLR_MEM 1
#define G_BL_CLR_BL 2
#define G_BL_CLR_FOG 3
#define G_BL_A_IN 0
#define G_BL_A_FOG 1
#define G_BL_A_SHADE 2
#define G_BL_1MA 0
#define G_BL_A_MEM 1
#define G_BL_1 2
#define G_BL_0 3
#define GBL_c1(m1a, m1b, m2a, m2b) (m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18
#define GBL_c2(m1a, m1b, m2a, m2b) (m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16

/* the render modes the game uses, for cycle 1 or cycle 2 (clk) */
#define RM_BLEND_OVER(clk) GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
#define RM_BLEND_AA(clk) GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)
#define RM_BLEND_PASS(clk) GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define RM_AA_OPA_SURF(clk) AA_EN | IM_RD | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL | RM_BLEND_AA(clk)
#define RM_RA_OPA_SURF(clk) AA_EN | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL | RM_BLEND_AA(clk)
#define RM_AA_ZB_OPA_SURF(clk)                                                                              \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL | RM_BLEND_AA(clk)
#define RM_AA_ZB_OPA_INTER(clk)                                                                             \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP | ALPHA_CVG_SEL | ZMODE_INTER | RM_BLEND_AA(clk)
#define RM_AA_OPA_TERR(clk)                                                                                 \
    AA_EN | IM_RD | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL | RM_BLEND_OVER(clk)
#define RM_AA_XLU_SURF(clk) AA_EN | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | FORCE_BL | ZMODE_OPA | RM_BLEND_OVER(clk)
#define RM_AA_ZB_XLU_SURF(clk)                                                                              \
    AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | FORCE_BL | ZMODE_XLU | RM_BLEND_OVER(clk)
#define RM_AA_XLU_LINE(clk)                                                                                 \
    AA_EN | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA | RM_BLEND_OVER(clk)
#define RM_OPA_SURF(clk) CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA | RM_BLEND_PASS(clk)
#define RM_ZB_OPA_SURF(clk) Z_CMP | Z_UPD | CVG_DST_FULL | ALPHA_CVG_SEL | ZMODE_OPA | RM_BLEND_AA(clk)
#define RM_XLU_SURF(clk) IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_OPA | RM_BLEND_OVER(clk)
#define RM_ZB_XLU_SURF(clk) Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_XLU | RM_BLEND_OVER(clk)
#define RM_CLD_SURF(clk) IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA | RM_BLEND_OVER(clk)
#define RM_ZB_CLD_SURF(clk) Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_XLU | RM_BLEND_OVER(clk)
#define RM_TEX_EDGE(clk)                                                                                    \
    CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA | TEX_EDGE | AA_EN | RM_BLEND_PASS(clk)
#define RM_NOOP(clk) GBL_c##clk(0, 0, 0, 0)

#define G_RM_AA_OPA_SURF RM_AA_OPA_SURF(1)
#define G_RM_AA_OPA_SURF2 RM_AA_OPA_SURF(2)
#define G_RM_RA_OPA_SURF RM_RA_OPA_SURF(1)
#define G_RM_RA_OPA_SURF2 RM_RA_OPA_SURF(2)
#define G_RM_AA_ZB_OPA_SURF RM_AA_ZB_OPA_SURF(1)
#define G_RM_AA_ZB_OPA_SURF2 RM_AA_ZB_OPA_SURF(2)
#define G_RM_AA_ZB_OPA_INTER RM_AA_ZB_OPA_INTER(1)
#define G_RM_AA_ZB_OPA_INTER2 RM_AA_ZB_OPA_INTER(2)
#define G_RM_AA_OPA_TERR RM_AA_OPA_TERR(1)
#define G_RM_AA_OPA_TERR2 RM_AA_OPA_TERR(2)
#define G_RM_AA_XLU_SURF RM_AA_XLU_SURF(1)
#define G_RM_AA_XLU_SURF2 RM_AA_XLU_SURF(2)
#define G_RM_AA_ZB_XLU_SURF RM_AA_ZB_XLU_SURF(1)
#define G_RM_AA_ZB_XLU_SURF2 RM_AA_ZB_XLU_SURF(2)
#define G_RM_AA_XLU_LINE RM_AA_XLU_LINE(1)
#define G_RM_AA_XLU_LINE2 RM_AA_XLU_LINE(2)
#define G_RM_OPA_SURF RM_OPA_SURF(1)
#define G_RM_OPA_SURF2 RM_OPA_SURF(2)
#define G_RM_ZB_OPA_SURF RM_ZB_OPA_SURF(1)
#define G_RM_ZB_OPA_SURF2 RM_ZB_OPA_SURF(2)
#define G_RM_XLU_SURF RM_XLU_SURF(1)
#define G_RM_XLU_SURF2 RM_XLU_SURF(2)
#define G_RM_ZB_XLU_SURF RM_ZB_XLU_SURF(1)
#define G_RM_ZB_XLU_SURF2 RM_ZB_XLU_SURF(2)
#define G_RM_CLD_SURF RM_CLD_SURF(1)
#define G_RM_CLD_SURF2 RM_CLD_SURF(2)
#define G_RM_ZB_CLD_SURF RM_ZB_CLD_SURF(1)
#define G_RM_ZB_CLD_SURF2 RM_ZB_CLD_SURF(2)
#define G_RM_TEX_EDGE RM_TEX_EDGE(1)
#define G_RM_TEX_EDGE2 RM_TEX_EDGE(2)
#define G_RM_NOOP RM_NOOP(1)
#define G_RM_NOOP2 RM_NOOP(2)
/* (the second cycle passing the first's colour through) */
#define G_RM_PASS GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

/* ---- the colour combiner: (a - b) * c + d, colour and alpha, both cycles ------------- */

#define G_CCMUX_COMBINED 0
#define G_CCMUX_TEXEL0 1
#define G_CCMUX_TEXEL1 2
#define G_CCMUX_PRIMITIVE 3
#define G_CCMUX_SHADE 4
#define G_CCMUX_ENVIRONMENT 5
#define G_CCMUX_CENTER 6
#define G_CCMUX_SCALE 6
#define G_CCMUX_COMBINED_ALPHA 7
#define G_CCMUX_TEXEL0_ALPHA 8
#define G_CCMUX_TEXEL1_ALPHA 9
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA 11
#define G_CCMUX_ENV_ALPHA 12
#define G_CCMUX_LOD_FRACTION 13
#define G_CCMUX_PRIM_LOD_FRAC 14
#define G_CCMUX_NOISE 7
#define G_CCMUX_K4 7
#define G_CCMUX_K5 15
#define G_CCMUX_1 6
#define G_CCMUX_0 31

#define G_ACMUX_COMBINED 0
#define G_ACMUX_TEXEL0 1
#define G_ACMUX_TEXEL1 2
#define G_ACMUX_PRIMITIVE 3
#define G_ACMUX_SHADE 4
#define G_ACMUX_ENVIRONMENT 5
#define G_ACMUX_LOD_FRACTION 0
#define G_ACMUX_PRIM_LOD_FRAC 6
#define G_ACMUX_1 6
#define G_ACMUX_0 7

/* where SetCombineMode keeps each input: cycle 0's a, c, alpha a and c in
   the first word, and so on */
#define GCCc0w0(saRGB0, mRGB0, saA0, mA0)                                                                   \
    (_SHIFTL((saRGB0), 20, 4) | _SHIFTL((mRGB0), 15, 5) | _SHIFTL((saA0), 12, 3) | _SHIFTL((mA0), 9, 3))
#define GCCc1w0(saRGB1, mRGB1) (_SHIFTL((saRGB1), 5, 4) | _SHIFTL((mRGB1), 0, 5))
#define GCCc0w1(sbRGB0, aRGB0, sbA0, aA0)                                                                   \
    (_SHIFTL((sbRGB0), 28, 4) | _SHIFTL((aRGB0), 15, 3) | _SHIFTL((sbA0), 12, 3) | _SHIFTL((aA0), 9, 3))
#define GCCc1w1(sbRGB1, saA1, mA1, aRGB1, sbA1, aA1)                                                        \
    (_SHIFTL((sbRGB1), 24, 4) | _SHIFTL((saA1), 21, 3) | _SHIFTL((mA1), 18, 3) | _SHIFTL((aRGB1), 6, 3) |   \
     _SHIFTL((sbA1), 3, 3) | _SHIFTL((aA1), 0, 3))

#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1)      \
    G_CMD(pkt,                                                                                              \
          _SHIFTL(G_SETCOMBINE, 24, 8) |                                                                    \
              _SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) |                   \
                          GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1),                                              \
                      0, 24),                                                                               \
          _GBI_W(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0, G_ACMUX_##Ab0, G_ACMUX_##Ad0) |                        \
                 GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1, G_ACMUX_##Ac1, G_CCMUX_##d1, G_ACMUX_##Ab1,           \
                         G_ACMUX_##Ad1)))
#define gDPSetCombineMode(pkt, a, b) gDPSetCombineLERP(pkt, a, b)
#define gsDPSetCombine(muxs0, muxs1) { _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(muxs0, 0, 24), _GBI_W(muxs1) }

/* the modes the game names: a, b, c, d for colour, then for alpha */
#define G_CC_PRIMITIVE 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE
#define G_CC_SHADE 0, 0, 0, SHADE, 0, 0, 0, SHADE
#define G_CC_MODULATEI TEXEL0, 0, SHADE, 0, 0, 0, 0, SHADE
#define G_CC_MODULATEIA TEXEL0, 0, SHADE, 0, TEXEL0, 0, SHADE, 0
#define G_CC_MODULATERGBA G_CC_MODULATEIA
#define G_CC_MODULATEIA_PRIM TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0
#define G_CC_MODULATERGBA_PRIM G_CC_MODULATEIA_PRIM
#define G_CC_DECALRGBA 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0

/* ---- the RDP: images, tiles, loads, colours, rectangles ----------------------------- */

#define G_IM_FMT_RGBA 0
#define G_IM_FMT_YUV 1
#define G_IM_FMT_CI 2
#define G_IM_FMT_IA 3
#define G_IM_FMT_I 4

/* texel sizes, and what loading and laying out each takes: loads move
   16-bit texels or 4-byte words, a TMEM line is 8 bytes */
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3
#define G_IM_SIZ_4b_BYTES 0
#define G_IM_SIZ_4b_TILE_BYTES G_IM_SIZ_4b_BYTES
#define G_IM_SIZ_4b_LINE_BYTES G_IM_SIZ_4b_BYTES
#define G_IM_SIZ_8b_BYTES 1
#define G_IM_SIZ_8b_TILE_BYTES G_IM_SIZ_8b_BYTES
#define G_IM_SIZ_8b_LINE_BYTES G_IM_SIZ_8b_BYTES
#define G_IM_SIZ_16b_BYTES 2
#define G_IM_SIZ_16b_TILE_BYTES G_IM_SIZ_16b_BYTES
#define G_IM_SIZ_16b_LINE_BYTES G_IM_SIZ_16b_BYTES
#define G_IM_SIZ_32b_BYTES 4
#define G_IM_SIZ_32b_TILE_BYTES 2
#define G_IM_SIZ_32b_LINE_BYTES 2
#define G_IM_SIZ_4b_LOAD_BLOCK G_IM_SIZ_16b
#define G_IM_SIZ_8b_LOAD_BLOCK G_IM_SIZ_16b
#define G_IM_SIZ_16b_LOAD_BLOCK G_IM_SIZ_16b
#define G_IM_SIZ_32b_LOAD_BLOCK G_IM_SIZ_32b
#define G_IM_SIZ_4b_SHIFT 2
#define G_IM_SIZ_8b_SHIFT 1
#define G_IM_SIZ_16b_SHIFT 0
#define G_IM_SIZ_32b_SHIFT 0
#define G_IM_SIZ_4b_INCR 3
#define G_IM_SIZ_8b_INCR 1
#define G_IM_SIZ_16b_INCR 0
#define G_IM_SIZ_32b_INCR 0

#define G_TX_LOADTILE 7
#define G_TX_RENDERTILE 0
#define G_TX_NOMIRROR 0
#define G_TX_WRAP 0
#define G_TX_MIRROR 0x1
#define G_TX_CLAMP 0x2
#define G_TX_NOMASK 0
#define G_TX_NOLOD 0
#define G_TEXTURE_IMAGE_FRAC 2
#define G_TX_DXT_FRAC 11
#define G_TX_LDBLK_MAX_TXL 2047

/* LoadBlock's dxt: 1/words-per-line in 1.11 */
#define TXL2WORDS(txls, b_txl) MAX(1, ((txls) * (b_txl) / 8))
#define CALC_DXT(width, b_txl) (((1 << G_TX_DXT_FRAC) + TXL2WORDS(width, b_txl) - 1) / TXL2WORDS(width, b_txl))
#define TXL2WORDS_4b(txls) MAX(1, ((txls) / 16))
#define CALC_DXT_4b(width) (((1 << G_TX_DXT_FRAC) + TXL2WORDS_4b(width) - 1) / TXL2WORDS_4b(width))

#define gSetImage(pkt, cmd, fmt, siz, width, i)                                                             \
    G_CMD(pkt, _SHIFTL(cmd, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL((width) - 1, 0, 12), \
          _GBI_W(i))
#define gsSetImage(cmd, fmt, siz, width, i)                                                                 \
    { _SHIFTL(cmd, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL((width) - 1, 0, 12), _GBI_W(i) }
#define gDPSetColorImage(pkt, f, s, w, i) gSetImage(pkt, G_SETCIMG, f, s, w, i)
#define gDPSetDepthImage(pkt, i) gSetImage(pkt, G_SETZIMG, 0, 0, 1, i)
#define gDPSetTextureImage(pkt, f, s, w, i) gSetImage(pkt, G_SETTIMG, f, s, w, i)
#define gsDPSetTextureImage(f, s, w, i) gsSetImage(G_SETTIMG, f, s, w, i)

#define G_SETTILE_W0(fmt, siz, line, tmem)                                                                  \
    _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9)
#define G_SETTILE_W1(tile, palette, cmt, maskt, shiftt, cms, masks, shifts)                                 \
    _SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | _SHIFTL(cmt, 18, 2) | _SHIFTL(maskt, 14, 4) |          \
        _SHIFTL(shiftt, 10, 4) | _SHIFTL(cms, 8, 2) | _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4)
#define gDPSetTile(pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)        \
    G_CMD(pkt, G_SETTILE_W0(fmt, siz, line, tmem), G_SETTILE_W1(tile, palette, cmt, maskt, shiftt, cms, masks, shifts))
#define gsDPSetTile(fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)            \
    { (G_SETTILE_W0(fmt, siz, line, tmem)), (G_SETTILE_W1(tile, palette, cmt, maskt, shiftt, cms, masks, shifts)) }

#define gDPLoadTileGeneric(pkt, c, tile, uls, ult, lrs, lrt)                                                \
    G_CMD(pkt, _SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),                              \
          _SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12))
#define gsDPLoadTileGeneric(c, tile, uls, ult, lrs, lrt)                                                    \
    { _SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),                                       \
      _SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12) }
#define gDPSetTileSize(pkt, t, uls, ult, lrs, lrt) gDPLoadTileGeneric(pkt, G_SETTILESIZE, t, uls, ult, lrs, lrt)
#define gsDPSetTileSize(t, uls, ult, lrs, lrt) gsDPLoadTileGeneric(G_SETTILESIZE, t, uls, ult, lrs, lrt)
#define gDPLoadTile(pkt, t, uls, ult, lrs, lrt) gDPLoadTileGeneric(pkt, G_LOADTILE, t, uls, ult, lrs, lrt)

#define G_LOADBLOCK_W1(tile, lrs, dxt)                                                                      \
    (_SHIFTL(tile, 24, 3) | _SHIFTL((MIN(lrs, G_TX_LDBLK_MAX_TXL)), 12, 12) | _SHIFTL(dxt, 0, 12))
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt)                                                         \
    G_CMD(pkt, (_SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12)),                  \
          G_LOADBLOCK_W1(tile, lrs, dxt))
#define gsDPLoadBlock(tile, uls, ult, lrs, dxt)                                                             \
    { (_SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12)), G_LOADBLOCK_W1(tile, lrs, dxt) }

/* a whole texture into TMEM in one LoadBlock, then the render tile over it */
#define gDPLoadTextureBlock(pkt, timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    {                                                                                                       \
        gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);                                            \
        gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks,      \
                   shifts);                                                                                 \
        gDPLoadSync(pkt);                                                                                   \
        gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, (((width) * (height) + siz##_INCR) >> siz##_SHIFT) - 1,      \
                     CALC_DXT(width, siz##_BYTES));                                                         \
        gDPPipeSync(pkt);                                                                                   \
        gDPSetTile(pkt, fmt, siz, (((width) * siz##_LINE_BYTES) + 7) >> 3, 0, G_TX_RENDERTILE, pal, cmt,    \
                   maskt, shiftt, cms, masks, shifts);                                                      \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0, ((width) - 1) << G_TEXTURE_IMAGE_FRAC,                   \
                       ((height) - 1) << G_TEXTURE_IMAGE_FRAC)                                              \
    }
#define gDPLoadTextureBlock_4b(pkt, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt)  \
    {                                                                                                       \
        gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);                                                \
        gDPSetTile(pkt, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
        gDPLoadSync(pkt);                                                                                   \
        gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, (((width) * (height) + 3) >> 2) - 1, CALC_DXT_4b(width));    \
        gDPPipeSync(pkt);                                                                                   \
        gDPSetTile(pkt, fmt, G_IM_SIZ_4b, ((((width) >> 1) + 7) >> 3), 0, G_TX_RENDERTILE, pal, cmt, maskt, \
                   shiftt, cms, masks, shifts);                                                             \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0, ((width) - 1) << G_TEXTURE_IMAGE_FRAC,                   \
                       ((height) - 1) << G_TEXTURE_IMAGE_FRAC)                                              \
    }
/* a rectangle of a texture with LoadTile */
#define gDPLoadTextureTile(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks,    \
                           maskt, shifts, shiftt)                                                           \
    {                                                                                                       \
        gDPSetTextureImage(pkt, fmt, siz, width, timg);                                                     \
        gDPSetTile(pkt, fmt, siz, (((((lrs) - (uls) + 1) * siz##_TILE_BYTES) + 7) >> 3), 0, G_TX_LOADTILE,  \
                   0, cmt, maskt, shiftt, cms, masks, shifts);                                              \
        gDPLoadSync(pkt);                                                                                   \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,       \
                    (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                          \
        gDPPipeSync(pkt);                                                                                   \
        gDPSetTile(pkt, fmt, siz, (((((lrs) - (uls) + 1) * siz##_LINE_BYTES) + 7) >> 3), 0,                 \
                   G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts);                           \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,  \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC)                        \
    }

/* colours */
#define gDPSetColor(pkt, c, d) G_CMD(pkt, _SHIFTL(c, 24, 8), _GBI_W(d))
#define DPRGBColor(pkt, cmd, r, g, b, a)                                                                    \
    gDPSetColor(pkt, cmd, (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))
#define gDPSetEnvColor(pkt, r, g, b, a) DPRGBColor(pkt, G_SETENVCOLOR, r, g, b, a)
#define gDPSetFillColor(pkt, d) gDPSetColor(pkt, G_SETFILLCOLOR, (d))
/* the primitive colour, with the minimum LOD level m and the LOD fraction l */
#define gDPSetPrimColor(pkt, m, l, r, g, b, a)                                                              \
    G_CMD(pkt, (_SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8)),                      \
          (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))
/* a 16-bit colour (the fill colour holds two) */
#define GPACK_RGBA5551(r, g, b, a) ((((r) << 8) & 0xf800) | (((g) << 3) & 0x7c0) | (((b) >> 2) & 0x3e) | ((a) & 0x1))

/* rectangles, in 10.2 */
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry)                                                           \
    G_CMD(pkt, (_SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL((lrx), 14, 10) | _SHIFTL((lry), 2, 10)),               \
          (_SHIFTL((ulx), 14, 10) | _SHIFTL((uly), 2, 10)))
#define G_SC_NON_INTERLACE 0
#define G_SC_ODD_INTERLACE 3
#define G_SC_EVEN_INTERLACE 2
#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry)                                                        \
    G_CMD(pkt,                                                                                              \
          _SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((int)((float)(ulx) * 4.0F), 12, 12) |                      \
              _SHIFTL((int)((float)(uly) * 4.0F), 0, 12),                                                   \
          _SHIFTL(mode, 24, 2) | _SHIFTL((int)((float)(lrx) * 4.0F), 12, 12) |                              \
              _SHIFTL((int)((float)(lry) * 4.0F), 0, 12))

/* syncs */
#define gDPFullSync(pkt) gDPNoParam(pkt, G_RDPFULLSYNC)
#define gDPTileSync(pkt) gDPNoParam(pkt, G_RDPTILESYNC)
#define gDPPipeSync(pkt) gDPNoParam(pkt, G_RDPPIPESYNC)
#define gsDPPipeSync() gsDPNoParam(G_RDPPIPESYNC)
#define gDPLoadSync(pkt) gDPNoParam(pkt, G_RDPLOADSYNC)

#endif
