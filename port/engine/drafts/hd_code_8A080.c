/* hd_code 8A080: the table of 25 moving collision objects the level's
   boxes, crates and blocks register (D_803FB8B8), and the run-time
   collision triangles of the blocks' holes (D_803F9330, 0x60 each, built
   by 5CB60's func_802A41B0). */
#include "engine_b.h"
#include "engine_b_types.h"

typedef struct CollisionObject {
    /* 0x00 */ s32 key;     /* -1: free */
    /* 0x04 */ s32 x, y, z;
    /* 0x10 */ s32 unk10;
} CollisionObject;

extern CollisionObject D_803FB8B8[25];
extern CollisionTri D_803F9330[100];
extern CollisionTri *PTR32 D_803FB8B0;  /* the next free one */

/* func_802CE840: empty the table */
void func_802CE840(void) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;

    ENG_COST(6);
    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            break;
        }
        ENG_COST(4);
        n--;
        o->key = -1;
        o++;
    }
    ENG_COST(4);
}

/* func_802CE880: set object `key`'s position, taking a free entry if it
   has none (and doing nothing if the table is full) */
void func_802CE880(s32 key, s32 x, s32 y, s32 z, s32 arg4) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;

    ENG_COST(5);
    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            break;
        }
        ENG_COST(4);
        n--;
        if (o->key == key) {
            goto found;
        }
        ENG_COST(2);
        o++;
    }
    ENG_COST(4);
    o = D_803FB8B8;
    n = 25;
    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            ENG_COST(4);
            return;
        }
        ENG_COST(4);
        n--;
        if (o->key == -1) {
            break;
        }
        ENG_COST(2);
        o++;
    }
    ENG_COST(1);
    o->key = key;
found:
    ENG_COST(5 + 4);
    o->x = x;
    o->y = y;
    o->z = z;
    o->unk10 = arg4;
}

/* func_802CE90C: free object `key`'s entry */
void func_802CE90C(s32 key) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;

    ENG_COST(5);
    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            break;
        }
        ENG_COST(4);
        n--;
        if (o->key == key) {
            ENG_COST(2);
            o->key = -1;
            break;
        }
        ENG_COST(2);
        o++;
    }
    ENG_COST(4);
}

/* func_802CE958: whether object `key` has an entry */
s32 func_802CE958(s32 key) {
    CollisionObject *o = D_803FB8B8;
    s32 n = 25;
    s32 found = 0;

    ENG_COST(6);
    for (;;) {
        ENG_COST(2);
        if (n == 0) {
            break;
        }
        ENG_COST(4);
        n--;
        if (o->key == key) {
            ENG_COST(1);
            found = 1;
            break;
        }
        ENG_COST(2);
        o++;
    }
    ENG_COST(4);
    return found;
}

/* func_802CE9A4: no hole triangles */
void func_802CE9A4(void) {
    ENG_COST(9);
    D_803FB8B0 = D_803F9330;
}

/* func_802CE9C8: make run-time triangles of a hole's `n` level triangles
   (0x16 bytes each), with `arg2` in each */
void func_802CE9C8(LevelCollisionTri *tris, u8 n, u8 arg2) {
    CollisionTri *t = D_803FB8B0;
    s32 left = n;

    ENG_COST(18);
    for (;;) {
        ENG_COST(2);
        if (left == 0) {
            break;
        }
        ENG_COST(4);
        left--;
        t->unk51 = 1;
        /* The original leaves its caller's $t9, $v0, $t6 and $s1 in 0x4F,
           0x50, 0x57 and 0x58: what IDO's code had in them.  0 here. */
        eng_collision_tri_init(t, (const u8 *)tris, 0, 0, arg2, 0, ((u8 *)tris)[0x14], 0, 0);
        t++;
        ENG_COST(2);
        tris = (LevelCollisionTri *)((u8 *)tris + 0x16);
    }
    ENG_COST(14);
    D_803FB8B0 = t;
}

/* func_802CEA68: clear the 0x51 flag of the triangles in [t, end) */
void func_802CEA68(CollisionTri *t, CollisionTri *end) {
    ENG_COST(2);
    for (;;) {
        ENG_COST(2);
        if (t == end) {
            break;
        }
        ENG_COST(3);
        t->unk51 = 0;
        t++;
    }
    ENG_COST(4);
}
