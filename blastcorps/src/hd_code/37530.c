#include "common.h"
#include "game/game.h"
#include "game/frame.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ s16 unkC;
    /* 0xE */ s16 unkE;
} UnkStruct_8027BCF0; /* size = 0x10 */

typedef struct {
    /* 0x0 */ UnkStruct_8027BCF0 *unk0;
    /* 0x4 */ UnkStruct_8027BCF0 *unk4;
    /* 0x8 */ u8 unk8;
} UnkStruct_802FC360; /* size = 0xC */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ u8 unk18;
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 unk1B;
} UnkStruct_8036D3D0; /* size = 0x1C */

void func_8027D350(s16 x0, s16 y0, s16 z0, s16 x1, s16 y1, s16 z1, Vtx *vtx, s32 i);
void func_8027D5AC(void);

extern FrameGame D_02000000;

/* .bss, 0x8036D3D0-0x8036DCA0 (tools/bss_c.py) */
UnkStruct_8036D3D0 D_8036D3D0[80];
u8 D_8036DC90;
u8 D_8036DC91;
u8 D_8036DC92;
s32 D_8036DC94;

/* .data, 0x802FC080-0x802FC3F0 (tools/data_c.py) */
UnkStruct_8027BCF0 D_802FC080[4] = {
    { 4300, 4756, 4300, 4681, 3247, 4681, 195, 210 },
    { 4300, 4756, 3247, 4681, 3247, 4756, 195, 210 },
    { 5321, 7952, 5185, 7952, 5321, 7143, 195, 210 },
    { 5185, 7952, 5321, 7143, 5185, 7143, 195, 210 },
};
UnkStruct_8027BCF0 D_802FC0C0[4] = {
    { 3912, 3179, 4342, 3185, 3912, 3334, 95, 105 },
    { 4342, 3185, 3912, 3334, 4342, 3334, 95, 105 },
    { 3608, 3328, 3608, 3452, 2630, 3257, 95, 105 },
    { 3608, 3452, 2630, 3257, 2630, 3526, 95, 105 },
};
UnkStruct_8027BCF0 D_802FC100[2] = {
    { 3432, 4215, 3631, 4083, 3614, 3722, 400, 510 },
    { 3432, 4215, 3614, 3722, 3416, 3856, 400, 510 },
};
UnkStruct_8027BCF0 D_802FC120[4] = {
    { 6739, 3500, 6458, 3513, 6734, 4715, 170, 205 },
    { 6458, 3513, 6734, 4715, 6464, 4697, 170, 205 },
    { 9021, 7881, 9040, 8030, 7981, 7907, 40, 190 },
    { 9021, 7881, 7981, 7907, 8030, 7719, 40, 190 },
};
UnkStruct_8027BCF0 D_802FC160[2] = {
    { 2454, 1317, 2794, 1317, 2794, 1447, 280, 318 },
    { 2454, 1317, 2794, 1447, 2454, 1447, 280, 318 },
};
UnkStruct_8027BCF0 D_802FC180[6] = {
    { 3669, 3623, 3664, 3483, 3511, 3466, 350, 450 },
    { 3669, 3623, 3511, 3466, 3517, 3644, 350, 450 },
    { 3656, 5179, 3679, 5021, 3501, 4995, 350, 450 },
    { 3656, 5179, 3501, 4995, 3511, 5201, 350, 450 },
    { 3666, 6738, 3668, 6570, 3511, 6556, 350, 450 },
    { 3666, 6738, 3511, 6556, 3515, 6761, 350, 450 },
};
UnkStruct_8027BCF0 D_802FC1E0[8] = {
    { 1116, 450, 1819, 450, 1819, 730, 80, 200 },
    { 1116, 450, 1819, 730, 1116, 730, 80, 200 },
    { 1116, 1470, 1819, 1470, 1819, 1720, 80, 200 },
    { 1116, 1470, 1819, 1720, 1116, 1720, 80, 200 },
    { 1116, 1930, 1819, 1930, 1819, 2213, 80, 200 },
    { 1116, 1930, 1819, 2213, 1116, 2213, 80, 200 },
    { 1116, 2675, 1819, 2675, 1819, 2905, 80, 200 },
    { 1116, 2675, 1819, 2905, 1116, 2905, 80, 200 },
};
UnkStruct_8027BCF0 D_802FC260[2] = {
    { 1774, 2010, 1769, 2239, 2178, 2204, 10, 200 },
    { 1774, 2010, 2178, 2204, 2178, 2028, 10, 200 },
};
UnkStruct_8027BCF0 D_802FC280[8] = {
    { 5891, 2748, 6028, 2782, 6132, 2444, 300, 350 },
    { 5891, 2748, 6132, 2444, 6018, 2421, 300, 350 },
    { 5200, 5508, 5315, 5508, 4990, 6767, 300, 350 },
    { 5200, 5508, 4990, 6767, 4843, 6732, 300, 350 },
    { 4654, 3203, 4850, 3211, 4850, 3756, 300, 350 },
    { 4654, 3203, 4850, 3756, 4654, 3756, 300, 350 },
    { 4654, 5460, 4850, 5460, 4850, 6770, 300, 350 },
    { 4654, 5460, 4850, 6770, 4654, 6770, 300, 350 },
};
UnkStruct_8027BCF0 D_802FC300[2] = {
    { 2547, 3852, 2418, 4065, 3012, 4383, 300, 450 },
    { 2547, 3852, 3012, 4383, 3105, 4272, 300, 450 },
};
UnkStruct_8027BCF0 D_802FC320[4] = {
    { 2074, 4007, 2388, 4268, 2164, 4868, 0, 300 },
    { 2074, 4007, 2164, 4868, 1676, 4458, 0, 300 },
    { 2802, 4601, 3130, 4859, 2763, 5350, 0, 300 },
    { 2802, 4601, 2763, 5350, 2287, 4976, 0, 300 },
};
/* each level's triangles: the start and the end of an array, and the level */
UnkStruct_802FC360 D_802FC360[11] = {
    { D_802FC080, &D_802FC080[4], 29 },
    { D_802FC0C0, &D_802FC0C0[4], 9 },
    { D_802FC100, &D_802FC100[2], 2 },
    { D_802FC120, &D_802FC120[4], 16 },
    { D_802FC160, &D_802FC160[2], 17 },
    { D_802FC180, &D_802FC180[6], 12 },
    { D_802FC1E0, &D_802FC1E0[8], 18 },
    { D_802FC260, &D_802FC260[2], 0 },
    { D_802FC280, &D_802FC280[8], 13 },
    { D_802FC300, &D_802FC300[2], 4 },
    { D_802FC320, &D_802FC320[4], 7 },
};

s32 func_8027BCF0(s16 arg0, s16 arg1, s16 arg2) {
    s32 i;
    u8 found;
    UnkStruct_8027BCF0 *p;
    UnkStruct_8027BCF0 *end;

    i = 0;
    found = FALSE;
    while (i < 11 && !found) {
        if (D_802FC360[i].unk8 == D_802E8BDC) {
            found = TRUE;
        } else {
            i++;
        }
    }
    if (!found) {
        return 0;
    }
    p = D_802FC360[i].unk0;
    end = D_802FC360[i].unk4;
    while (p != end) {
        if (arg1 >= p->unkC && arg1 <= p->unkE) {
            if (func_802AC4C4(arg0, arg2, p->unk0, p->unk2, p->unk4, p->unk6, p->unk8, p->unkA) != 0) {
                return 1;
            }
        }
        p++;
    }
    return 0;
}

void func_8027BE4C(void) {
    s32 sp4;

    D_8036DC90 = 0;
    D_8036DC91 = 0;
    D_8036DC92 = 0;
    D_8036DC94 = -1;
}

void func_8027BE7C(u8 arg0, s32 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6, s32 arg7, s16 arg8, u8 arg9,
                   u8 arg10, u8 arg11, u8 arg12) {
    f32 mf[4][4];
    f32 x0;
    f32 z0;
    f32 x1;
    f32 z1;
    f32 y;
    f32 dx;
    f32 dz;
    s16 ax;
    s16 az;
    s16 bx;
    s16 bz;

    if (func_8027BCF0(arg6 >> 5, arg1 >> 5, arg7 >> 5) == 0) {
        guRotateF(mf, (f32)arg8 / 4095.0 * 360.0, 0.0f, 1.0f, 0.0f);
        guMtxXFMF(mf, arg2, 0.0f, arg3, &x0, &y, &z0);
        guMtxXFMF(mf, arg4, 0.0f, arg5, &x1, &y, &z1);
        ax = (s32)(arg6 + x0) >> 5;
        az = (s32)(arg7 + z0) >> 5;
        bx = (s32)(arg6 + x1) >> 5;
        bz = (s32)(arg7 + z1) >> 5;
        arg1 >>= 5;
        guMtxXFMF(mf, (u32)arg9, 0.0f, 0.0f, &dx, &y, &dz);
        if (D_8036DC94 + 1 != D_80358060 || D_8036DC94 == -1) {
            D_8036D3D0[D_8036DC91].unk1A = 1;
            if (++D_8036DC91 == 80) {
                D_8036DC91 = 0;
            }
            if (D_8036DC91 == D_8036DC90) {
                if (++D_8036DC90 == 80) {
                    D_8036DC90 = 0;
                }
            }
        }
        D_8036D3D0[D_8036DC91].unk1B = arg12;
        D_8036D3D0[D_8036DC91].unk0 = ax + (s16)dx;
        D_8036D3D0[D_8036DC91].unk2 = arg1;
        D_8036D3D0[D_8036DC91].unk4 = az + (s16)dz;
        D_8036D3D0[D_8036DC91].unk6 = ax - (s16)dx;
        D_8036D3D0[D_8036DC91].unk8 = arg1;
        D_8036D3D0[D_8036DC91].unkA = az - (s16)dz;
        D_8036D3D0[D_8036DC91].unkC = bx + (s16)dx;
        D_8036D3D0[D_8036DC91].unkE = arg1;
        D_8036D3D0[D_8036DC91].unk10 = bz + (s16)dz;
        D_8036D3D0[D_8036DC91].unk12 = bx - (s16)dx;
        D_8036D3D0[D_8036DC91].unk14 = arg1;
        D_8036D3D0[D_8036DC91].unk16 = bz - (s16)dz;
        D_8036D3D0[D_8036DC91].unk18 = arg10;
        D_8036D3D0[D_8036DC91].unk19 = arg11;
        if (++D_8036DC92 >= arg0) {
            D_8036DC92 = 0;
            D_8036D3D0[D_8036DC91].unk1A = 0;
            if (++D_8036DC91 == 80) {
                D_8036DC91 = 0;
            }
            if (D_8036DC91 == D_8036DC90) {
                if (++D_8036DC90 == 80) {
                    D_8036DC90 = 0;
                }
            }
        }
        D_8036DC94 = D_80358060;
    }
}

#ifdef TARGET_PC
/* The port's own: each entry's vertices have a place of their own,
   unk1900[0x70 + 4 * entry] (track A's pair, then track B's), loaded on
   their own into slots 0-3 or 4-7 and joined to the entry before.  The
   original packs each run's vertices, track A's then track B's, so where a
   point sits moves whenever the tail moves on or a run grows; --interpolate
   pairs vertex loads by where they sit (docs/PORT.md, "Frame rate"), and its
   in-between images blended the marks with other marks, track B with track
   A.  And its strips lose track B's first quad: past the last of track A's
   it skips two quads (k += 4, not 2), unless that falls at the end of a load
   of 16, where the next load starts back at track B's first pair; as the
   loads shift, that quad comes and goes.  No cull box: the RSP's time
   isn't the port's. */
void func_8027C4C8(Gfx **arg0, FrameGame *arg1) {
    Gfx *gfx;
    Gfx *sub;
    Gfx *subEnd;
    Vtx *v;
    UnkStruct_8036D3D0 *e;
    u8 idx;
    s32 two;
    s32 first;
    s32 n;
    s32 p;
    s32 c;

    gfx = *arg0;
    func_8027D5AC();
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, 0, 0, 0, SHADE, 0, 0, 0, PRIMITIVE, 0, 0, 0, SHADE);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, 0);
    sub = &arg1->unk3C20[2];
    subEnd = &arg1->unk3C20[0x192 - 1]; /* (the end's) */
    /* the runs as the original finds them: from the tail, each to the next
       entry that ends one (unk1A, itself not drawn) or to the head */
    idx = D_8036DC90;
    while (idx != D_8036DC91) {
        two = D_8036D3D0[idx].unk1B == 0;
        n = two ? 4 : 2;
        first = TRUE;
        c = 4;
        while (idx != D_8036DC91 && D_8036D3D0[idx].unk1A == 0) {
            if (sub + 5 > subEnd) {
                break;
            }
            e = &D_8036D3D0[idx];
            v = &arg1->unk1900[0x70 + 4 * idx];
            v[0].v.ob[0] = e->unk0, v[0].v.ob[1] = e->unk2, v[0].v.ob[2] = e->unk4;
            v[1].v.ob[0] = e->unk6, v[1].v.ob[1] = e->unk8, v[1].v.ob[2] = e->unkA;
            v[0].v.cn[3] = v[1].v.cn[3] = e->unk18;
            if (two) {
                v[2].v.ob[0] = e->unkC, v[2].v.ob[1] = e->unkE, v[2].v.ob[2] = e->unk10;
                v[3].v.ob[0] = e->unk12, v[3].v.ob[1] = e->unk14, v[3].v.ob[2] = e->unk16;
                v[2].v.cn[3] = v[3].v.cn[3] = e->unk19;
            }
            p = c;
            c = 4 - p;
            gSPVertex(sub++, &D_02000000.unk1900[0x70 + 4 * idx], n, c);
            if (!first) {
                gSP1Triangle(sub++, p, p + 1, c, 0);
                gSP1Triangle(sub++, p + 1, c, c + 1, 0);
                if (two) {
                    gSP1Triangle(sub++, p + 2, p + 3, c + 2, 0);
                    gSP1Triangle(sub++, p + 3, c + 2, c + 3, 0);
                }
            }
            first = FALSE;
            if (++idx == 80) {
                idx = 0;
            }
        }
        if (idx == D_8036DC91 || sub + 5 > subEnd) {
            break;
        }
        if (++idx == 80) {
            idx = 0;
        }
    }
    if (sub != &arg1->unk3C20[2]) {
        gSPDisplayList(gfx++, &D_02000000.unk3C20[2]);
        gSPEndDisplayList(sub++);
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}
#else
void func_8027C4C8(Gfx **arg0, FrameGame *arg1) {
    Gfx *gfx;
    u8 idx;
    u8 start;
    u8 end;
    u8 j;
    u8 done;
    Gfx *sub;
    s32 subCount;
    s32 vtxStart;
    s32 vtxIdx;
    s16 minX;
    s16 minY;
    s16 minZ;
    s16 maxX;
    s16 maxY;
    s16 maxZ;
    u8 k;
    u8 n;
    s32 count;
    s32 sp6C;
    u8 flag;

    gfx = *arg0;
    idx = D_8036DC90;
    done = FALSE;
    sub = &arg1->unk3C20[2];
    subCount = 0;
    vtxStart = 0;
    func_8027D5AC();
    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
    gSPClearGeometryMode(gfx++, 0xFFFFFFFF);
    gSPSetGeometryMode(gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, 0, 0, 0, SHADE, 0, 0, 0, PRIMITIVE, 0, 0, 0, SHADE);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, 0);
    while (!done) {
        start = idx;
        flag = D_8036D3D0[idx].unk1B;
        while (idx != D_8036DC91 && D_8036D3D0[idx].unk1A == 0) {
            if (++idx == 80) {
                idx = 0;
            }
        }
        end = idx;
        if (idx == D_8036DC91) {
            done = TRUE;
        }
        if (start != end) {
            gSPDisplayList(gfx++, &D_02000000.unk3C20[2 + subCount]);
            minX = 0x7FFF, minY = 0x7FFF, minZ = 0x7FFF;
            maxX = -0x8000, maxY = -0x8000, maxZ = -0x8000;
            vtxIdx = vtxStart;
            j = start;
            while (j != end) {
                arg1->unk1900[0x70 + vtxIdx].v.ob[0] = D_8036D3D0[j].unk0;
                arg1->unk1900[0x70 + vtxIdx].v.ob[1] = D_8036D3D0[j].unk2;
                arg1->unk1900[0x70 + vtxIdx].v.ob[2] = D_8036D3D0[j].unk4;
                arg1->unk1900[0x70 + vtxIdx].v.cn[3] = D_8036D3D0[j].unk18;
                vtxIdx++;
                arg1->unk1900[0x70 + vtxIdx].v.ob[0] = D_8036D3D0[j].unk6;
                arg1->unk1900[0x70 + vtxIdx].v.ob[1] = D_8036D3D0[j].unk8;
                arg1->unk1900[0x70 + vtxIdx].v.ob[2] = D_8036D3D0[j].unkA;
                arg1->unk1900[0x70 + vtxIdx].v.cn[3] = D_8036D3D0[j].unk18;
                vtxIdx++;
                if (D_8036D3D0[j].unk0 < minX) {
                    minX = D_8036D3D0[j].unk0;
                }
                if (D_8036D3D0[j].unk2 < minY) {
                    minY = D_8036D3D0[j].unk2;
                }
                if (D_8036D3D0[j].unk4 < minZ) {
                    minZ = D_8036D3D0[j].unk4;
                }
                if (D_8036D3D0[j].unk0 > maxX) {
                    maxX = D_8036D3D0[j].unk0;
                }
                if (D_8036D3D0[j].unk2 > maxY) {
                    maxY = D_8036D3D0[j].unk2;
                }
                if (D_8036D3D0[j].unk4 > maxZ) {
                    maxZ = D_8036D3D0[j].unk4;
                }
                if (D_8036D3D0[j].unk6 < minX) {
                    minX = D_8036D3D0[j].unk6;
                }
                if (D_8036D3D0[j].unk8 < minY) {
                    minY = D_8036D3D0[j].unk8;
                }
                if (D_8036D3D0[j].unkA < minZ) {
                    minZ = D_8036D3D0[j].unkA;
                }
                if (D_8036D3D0[j].unk6 > maxX) {
                    maxX = D_8036D3D0[j].unk6;
                }
                if (D_8036D3D0[j].unk8 > maxY) {
                    maxY = D_8036D3D0[j].unk8;
                }
                if (D_8036D3D0[j].unkA > maxZ) {
                    maxZ = D_8036D3D0[j].unkA;
                }
                if (++j == 80) {
                    j = 0;
                }
            }
            if (flag == 0) {
                j = start;
                while (j != end) {
                    arg1->unk1900[0x70 + vtxIdx].v.ob[0] = D_8036D3D0[j].unkC;
                    arg1->unk1900[0x70 + vtxIdx].v.ob[1] = D_8036D3D0[j].unkE;
                    arg1->unk1900[0x70 + vtxIdx].v.ob[2] = D_8036D3D0[j].unk10;
                    arg1->unk1900[0x70 + vtxIdx].v.cn[3] = D_8036D3D0[j].unk19;
                    vtxIdx++;
                    arg1->unk1900[0x70 + vtxIdx].v.ob[0] = D_8036D3D0[j].unk12;
                    arg1->unk1900[0x70 + vtxIdx].v.ob[1] = D_8036D3D0[j].unk14;
                    arg1->unk1900[0x70 + vtxIdx].v.ob[2] = D_8036D3D0[j].unk16;
                    arg1->unk1900[0x70 + vtxIdx].v.cn[3] = D_8036D3D0[j].unk19;
                    vtxIdx++;
                    if (D_8036D3D0[j].unkC < minX) {
                        minX = D_8036D3D0[j].unkC;
                    }
                    if (D_8036D3D0[j].unkE < minY) {
                        minY = D_8036D3D0[j].unkE;
                    }
                    if (D_8036D3D0[j].unk10 < minZ) {
                        minZ = D_8036D3D0[j].unk10;
                    }
                    if (D_8036D3D0[j].unk12 > maxX) {
                        maxX = D_8036D3D0[j].unk12;
                    }
                    if (D_8036D3D0[j].unk14 > maxY) {
                        maxY = D_8036D3D0[j].unk14;
                    }
                    if (D_8036D3D0[j].unk16 > maxZ) {
                        maxZ = D_8036D3D0[j].unk16;
                    }
                    if (D_8036D3D0[j].unkC < minX) {
                        minX = D_8036D3D0[j].unkC;
                    }
                    if (D_8036D3D0[j].unkE < minY) {
                        minY = D_8036D3D0[j].unkE;
                    }
                    if (D_8036D3D0[j].unk10 < minZ) {
                        minZ = D_8036D3D0[j].unk10;
                    }
                    if (D_8036D3D0[j].unk12 > maxX) {
                        maxX = D_8036D3D0[j].unk12;
                    }
                    if (D_8036D3D0[j].unk14 > maxY) {
                        maxY = D_8036D3D0[j].unk14;
                    }
                    if (D_8036D3D0[j].unk16 > maxZ) {
                        maxZ = D_8036D3D0[j].unk16;
                    }
                    if (++j == 80) {
                        j = 0;
                    }
                }
            }
            count = vtxIdx - vtxStart;
            if (flag != 0) {
                sp6C = 0;
            } else {
                sp6C = count >> 1;
            }
            if (count > 40) {
                func_8027D350(minX, minY, minZ, maxX, maxY, maxZ, &arg1->unk1900[0x70], vtxIdx);
                gSPVertex(sub++, &D_02000000.unk1900[0x70 + vtxIdx], 8, 0);
                gSPCullDisplayList(sub++, 0, 7);
                subCount += 2;
                vtxIdx += 8;
            }
            while (count >= 3) {
                if (count > 16) {
                    n = 16;
                } else {
                    n = count;
                }
                gSPVertex(sub++, &D_02000000.unk1900[0x70 + vtxStart], n, 0);
                k = 0;
                vtxStart += n - 2;
                subCount++;
                while (k < n - 2) {
                    if (sp6C != 2) {
                        gSP1Triangle(sub++, k, k + 1, k + 2, 0);
                        gSP1Triangle(sub++, k + 1, k + 2, k + 3, 0);
                        subCount += 2;
                        sp6C -= 2;
                        k += 2;
                    } else {
                        sp6C = 0;
                        k += 4;
                    }
                }
                count = count - n + 2;
            }
            vtxStart = vtxIdx;
            gSPEndDisplayList(sub++);
            subCount++;
        }
        if (++idx == 80) {
            idx = 0;
        }
    }
    gDPPipeSync(gfx++);
    *arg0 = gfx;
}
#endif

void func_8027D350(s16 x0, s16 y0, s16 z0, s16 x1, s16 y1, s16 z1, Vtx *vtx, s32 i) {
    vtx[i].v.ob[0] = x0;
    vtx[i].v.ob[1] = y0;
    vtx[i].v.ob[2] = z0;
    i++;
    vtx[i].v.ob[0] = x0;
    vtx[i].v.ob[1] = y1;
    vtx[i].v.ob[2] = z0;
    i++;
    vtx[i].v.ob[0] = x1;
    vtx[i].v.ob[1] = y0;
    vtx[i].v.ob[2] = z0;
    i++;
    vtx[i].v.ob[0] = x1;
    vtx[i].v.ob[1] = y1;
    vtx[i].v.ob[2] = z0;
    i++;
    vtx[i].v.ob[0] = x0;
    vtx[i].v.ob[1] = y0;
    vtx[i].v.ob[2] = z1;
    i++;
    vtx[i].v.ob[0] = x0;
    vtx[i].v.ob[1] = y1;
    vtx[i].v.ob[2] = z1;
    i++;
    vtx[i].v.ob[0] = x1;
    vtx[i].v.ob[1] = y0;
    vtx[i].v.ob[2] = z1;
    i++;
    vtx[i].v.ob[0] = x1;
    vtx[i].v.ob[1] = y1;
    vtx[i].v.ob[2] = z1;
}

void func_8027D5AC(void) {
    s32 count;
    u8 next;

    if (D_8036DC91 >= D_8036DC90) {
        count = D_8036DC91 - D_8036DC90;
    } else {
        count = D_8036DC91 - D_8036DC90 + 80;
    }
    if (count >= 71) {
        if (D_8036D3D0[D_8036DC90].unk18 <= 0) {
            D_8036D3D0[D_8036DC90].unk18 = 0;
        } else {
            D_8036D3D0[D_8036DC90].unk18--;
        }
        if (D_8036D3D0[D_8036DC90].unk19 <= 0) {
            D_8036D3D0[D_8036DC90].unk19 = 0;
        } else {
            D_8036D3D0[D_8036DC90].unk19--;
        }
        if (D_8036D3D0[D_8036DC90].unk18 == 0 && D_8036D3D0[D_8036DC90].unk19 == 0) {
            next = D_8036DC90 + 1;
            if (next == 80) {
                next = 0;
            }
            if (D_8036D3D0[next].unk18 <= 0) {
                D_8036D3D0[next].unk18 = 0;
            } else {
                D_8036D3D0[next].unk18--;
            }
            if (D_8036D3D0[next].unk19 <= 0) {
                D_8036D3D0[next].unk19 = 0;
            } else {
                D_8036D3D0[next].unk19--;
            }
            if (D_8036D3D0[next].unk18 == 0 && D_8036D3D0[next].unk19 == 0) {
                D_8036DC90 = next;
            }
        }
    }
}
