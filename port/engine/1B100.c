/*
 * hd_front_end 1B100 (us.v11 0x80202100-0x8020272C): the front end's
 * vehicle models, through hd_code's model loader and part helpers (5CB60,
 * 56040), as native C (engine.h).  The front end's C calls them: to load a
 * vehicle's model with its two part buffers and display lists
 * (func_80202100), set its parts up (func_80202270, func_802022EC,
 * func_80202380), turn them (func_802025D0) and step them a frame
 * (func_802021FC).
 *
 * What is left of the original's registers: their $v0 (the last part
 * helper's first argument) and the $s0-$fp they load back.  The next
 * level's loader still reads them (5CB60's collision triangles take a
 * byte from $v0, the vehicles' $s registers come through the loads),
 * which keeps the TAS exact for now.
 */
#include "shared.h"
#include "game/game.h"

enum { rV0 = 2 };
#define G(r) ENGINE_GPR(r)
#define S0_FP (G(16) | G(17) | G(18) | G(19) | G(20) | G(21) | G(22) | G(23) | G(28) | G(30))

/* a block table, for code repeated with its own blocks */
typedef struct { u16 id, n; } Blk;
#define BLKT(t, i) ENGINE_BLK_((t)[i].id, (t)[i].n)

/* 5CB60: model n loaded; a vehicle's record from its model */
REGS(t3 -> s2)
u8 *func_802A396C(s32 n);
REGS(a0, a1, v0, v1, s2)
void func_802A1388(s32 type, s32 a1, u8 *buf1, u8 *buf2, u8 *model);
/* 56040 */
REGS(t0, t1, v1, a0)
void func_8029F85C(Part *parts, u8 *model, u8 *buf1, u8 *buf2);
REGS(t0, v0, v1)
void func_8029E558(Part *parts, u8 *buf, u8 *other);

extern u8 D_802C2208[], D_802C226C[], D_802C2190[], D_802C21A4[], D_802C21B8[];

/* Vehicle model `type` loaded: the model into *rec, two 0x1770-byte part
   buffers from the heap into bufs[0..1], and the model's two display list
   sections (0x1C, 0x2C) into dls[0] and dls[2] with their places in the
   second buffer (dls[1], dls[3]); then its Vehicle record (5CB60). */
void func_80202100(s32 type, u32 *rec, u32 *bufs, u32 *dls) {
    u8 *model, *heap;
    s32 o1, o2;

    engine_save(S0_FP, 0);
    ENGINE_BLK(80202100);
    model = func_802A396C(type);
    ENGINE_BLK(80202158);
    rec[0] = (u32)model;
    heap = D_80358070;
    bufs[0] = (u32)heap;
    bufs[1] = (u32)(heap + 0x1770);
    heap += 0x1770 * 2;
    D_80358070 = heap;
    o1 = *(s32 *)(model + 0x1C);
    dls[0] = (u32)(model + o1);
    dls[1] = (u32)heap;
    o2 = *(s32 *)(model + 0x2C);
    dls[2] = (u32)(model + o2);
    dls[3] = (u32)(heap + o2 - o1);
    func_802A1388(type, 0, (u8 *)bufs[0], (u8 *)bufs[1], (u8 *)rec[0]);
    ENGINE_BLK(802021C8);
    engine_restore();
}

/* the parts stepped a frame (from the buffer `buf`, `other` the next) */
void func_802021FC(Part *parts, u8 *buf, u8 *other) {
    engine_save(S0_FP, 0);
    ENGINE_BLK(802021FC);
    func_8029E558(parts, buf, other);
    ENGINE_BLK(8020223C);
    engine_restore();
}

/* the parts set up from the model in the two buffers */
void func_80202270(u8 *model, u32 *bufs, Part *parts) {
    engine_save(S0_FP, 0);
    ENGINE_BLK(80202270);
    func_8029F85C(parts, model, (u8 *)bufs[0], (u8 *)bufs[1]);
    ENGINE_BLK(802022B8);
    engine_restore();
}

/* part i's settings */
void func_802022EC(Part *parts, s32 i, s32 a, s32 b, f32 f, s32 c, s32 d) {
    ENGINE_BLK(802022EC);
    func_802A039C(i, c, parts);
    ENGINE_BLK(80202328);
    func_802A03D4(i, d, parts);
    ENGINE_BLK(80202330);
    func_802A040C(i, a, parts);
    ENGINE_BLK(80202338);
    func_802A0480(i, b, parts, f);
    ENGINE_BLK(80202344);
    func_802A0290(i, -1, parts);
    ENGINE_BLK(8020234C);
    ENGINE_LEAVE(rV0, i);               /* (what it passed last) */
}

/* one part's four settings (func_802A05D0... by its key) */
static void part4(u8 *key, s32 v, s32 k) {
    static const Blk blks[][4] = {
        { {ENGINE_BLK_802023CC}, {ENGINE_BLK_802023DC}, {ENGINE_BLK_802023EC}, {ENGINE_BLK_802023FC} },
        { {ENGINE_BLK_8020240C}, {ENGINE_BLK_8020241C}, {ENGINE_BLK_8020242C}, {ENGINE_BLK_8020243C} },
        { {ENGINE_BLK_80202454}, {ENGINE_BLK_80202464}, {ENGINE_BLK_80202474}, {ENGINE_BLK_80202484} },
        { {ENGINE_BLK_80202494}, {ENGINE_BLK_802024A4}, {ENGINE_BLK_802024B4}, {ENGINE_BLK_802024C4} },
        { {ENGINE_BLK_802024D4}, {ENGINE_BLK_802024E4}, {ENGINE_BLK_802024F4}, {ENGINE_BLK_80202504} },
    };

    BLKT(blks[k], 0);
    func_802A05D0(key, v);
    BLKT(blks[k], 1);
    func_802A05F8(key, 0);
    BLKT(blks[k], 2);
    func_802A0620(key, 0);
    BLKT(blks[k], 3);
    func_802A0508(key, -1);
}

/* the front end's vehicle `type`'s moving parts at rest (5: the two of
   D_802C2208/D_802C226C; 4: the three of D_802C2190...) */
void func_80202380(s32 type) {
    u8 *last = NULL;

    ENGINE_BLK(80202380);
    if (type == 5) {
        part4(D_802C2208, 0, 0);
        part4(D_802C226C, 0, 1);
        last = D_802C226C;
        ENGINE_BLK(8020244C);
    } else {
        ENGINE_BLK(802023B8);
        if (type == 4) {
            part4(D_802C2190, 0, 2);
            part4(D_802C21A4, 0, 3);
            part4(D_802C21B8, 0, 4);
            last = D_802C21B8;
            ENGINE_BLK(80202514);
        } else {
            ENGINE_BLK(802023C4);
        }
    }
    ENGINE_BLK(8020259C);
    if (last != NULL) {
        ENGINE_LEAVE(rV0, (u32)last);   /* (what it passed last) */
    }
}

/* the front end's vehicle `type`'s moving parts turned for the angle a
   (12-bit): 5's two by a / 0x55 (above 0x800 less 0x800); 4's by
   (a - 0x355) / 0x4C between 0x355 and 0x8AB, and its two others fixed */
void func_802025D0(u8 type, u32 a) {
    u32 s;

    ENGINE_BLK(802025D0);
    if (type == 5) {
        ENGINE_BLK(8020261C);
        s = a;
        if (!((s32)s < 0x800)) {
            ENGINE_BLK(8020262C);
            s -= 0x800;
        }
        ENGINE_BLK(80202630);
        s /= 0x55;
        ENGINE_BLK(80202654);
        func_802A05A4(D_802C2208, s, 0.0f);
        ENGINE_BLK(80202664);
        func_802A05A4(D_802C226C, s, 0.0f);
        ENGINE_BLK(80202678);
        ENGINE_LEAVE(rV0, (u32)D_802C226C);
    } else {
        ENGINE_BLK(80202608);
        if (type == 4) {
            ENGINE_BLK(80202680);
            s = a;
            if ((s32)s < 0x355) {
                s = 0;
            } else {
                ENGINE_BLK(80202690);
                if (!((s32)s < 0x8AB)) {
                    s = 0;
                } else {
                    ENGINE_BLK(80202698);
                    s -= 0x355;
                    goto div;
                }
            }
            ENGINE_BLK(802026A0);
        div:
            ENGINE_BLK(802026A4);
            s /= 0x4C;
            ENGINE_BLK(802026C4);
            func_802A05A4(D_802C21B8, s, 0.0f);
            ENGINE_BLK(802026D4);
            func_802A05D0(D_802C2190, 0x32);
            ENGINE_BLK(802026E8);
            func_802A05D0(D_802C21A4, 0x32);
            ENGINE_LEAVE(rV0, (u32)D_802C21A4);
        } else {
            ENGINE_BLK(80202614);
        }
    }
    ENGINE_BLK(802026F8);
}
