/*
 * hd_code 8A080 (us.v11 0x802CE840-0x802CEAA0): the table of 25 moving
 * objects the level's ammo boxes, crates and blocks register for the
 * collision code (D_803FB8B8), and the run-time collision triangles of the
 * blocks' holes (D_803F9330, made by 5CB60's func_802A41B0), as native C
 * (engine.h).
 */
#include "engine.h"
#include "collision.h"

typedef struct CollisionObject {
    /* 0x00 */ s32 key;         /* -1: free */
    /* 0x04 */ s32 x, y, z;
    /* 0x10 */ s32 unk10;
} CollisionObject;

extern CollisionObject D_803FB8B8[25];
extern CollisionTri D_803F9330[100];
extern CollisionTri *PTR32 D_803FB8B0;  /* the next free one */

/* func_802CE840 (00000.c's): empty the table */
void func_802CE840(void) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;

    ENGINE_BLK(802CE840);
    for (;;) {
        ENGINE_BLK(802CE858);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802CE860);
        n--;
        o->key = -1;
        o++;
    }
    ENGINE_BLK(802CE870);
}

/* func_802CE880: put object `key` at (x, y, z), taking a free entry if it
   has none (nothing happens when the table is full) */
void func_802CE880(s32 key, s32 x, s32 y, s32 z, s32 arg4) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;

    ENGINE_BLK(802CE880);
    for (;;) {
        ENGINE_BLK(802CE894);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802CE89C);
        n--;
        if (o->key == key) {
            goto found;
        }
        ENGINE_BLK(802CE8AC);
        o++;
    }
    ENGINE_BLK(802CE8B4);
    o = D_803FB8B8;
    n = 25;
    for (;;) {
        ENGINE_BLK(802CE8C4);
        if (n == 0) {
            ENGINE_BLK(802CE8FC);
            return;
        }
        ENGINE_BLK(802CE8CC);
        n--;
        if (o->key == -1) {
            break;
        }
        ENGINE_BLK(802CE8DC);
        o++;
    }
    ENGINE_BLK(802CE8E4);
    o->key = key;
found:
    ENGINE_BLK(802CE8E8);
    o->x = x;
    o->y = y;
    o->z = z;
    o->unk10 = arg4;
    ENGINE_BLK(802CE8FC);
}

/* func_802CE90C: free object `key`'s entry */
void func_802CE90C(s32 key) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;

    ENGINE_BLK(802CE90C);
    for (;;) {
        ENGINE_BLK(802CE920);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802CE928);
        n--;
        if (o->key == key) {
            ENGINE_BLK(802CE940);
            o->key = -1;
            break;
        }
        ENGINE_BLK(802CE938);
        o++;
    }
    ENGINE_BLK(802CE948);
}

/* func_802CE958: whether object `key` has an entry */
s32 func_802CE958(s32 key) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;
    s32 found = 0;

    ENGINE_BLK(802CE958);
    for (;;) {
        ENGINE_BLK(802CE970);
        if (n == 0) {
            break;
        }
        ENGINE_BLK(802CE978);
        n--;
        if (o->key == key) {
            ENGINE_BLK(802CE990);
            found = 1;
            break;
        }
        ENGINE_BLK(802CE988);
        o++;
    }
    ENGINE_BLK(802CE994);
    return found;
}

/* func_802CE9A4: no hole triangles */
void func_802CE9A4(void) {
    ENGINE_BLK(802CE9A4);
    D_803FB8B0 = D_803F9330;
}

/* func_802CE9C8 (4B5E0.c's): run-time triangles from a hole's `n` level
   triangles, each marked as in use (0x51) with `arg2` (0x52) */
void func_802CE9C8(LevelCollisionTri *tris, u8 n, u8 arg2) {
    CollisionTri *t = D_803FB8B0;
    s32 left = n;

    ENGINE_BLK(802CE9C8);
    for (;;) {
        ENGINE_BLK(802CEA10);
        if (left == 0) {
            break;
        }
        ENGINE_BLK(802CEA18);
        left--;
        t->unk51 = 1;
        /* the triangle's 0x4F, 0x50, 0x57 and 0x58 get what $t9, $v0, $t6
           and $s1 hold: the caller's, left in the context */
        t = (CollisionTri *)func_802A41B0((u32)t, (u32)tris, arg2, tris->unk14, 0);
        ENGINE_BLK(802CEA28);
        tris = (LevelCollisionTri *)((u8 *)tris + 0x16);
    }
    ENGINE_BLK(802CEA30);
    D_803FB8B0 = t;
}

/* func_802CEA68: the triangles of [t, end) not in use (0x51) */
void func_802CEA68(CollisionTri *t, CollisionTri *end) {
    ENGINE_BLK(802CEA68);
    for (;;) {
        ENGINE_BLK(802CEA70);
        if (t == end) {
            break;
        }
        ENGINE_BLK(802CEA78);
        t->unk51 = 0;
        t++;
    }
    ENGINE_BLK(802CEA84);
}
