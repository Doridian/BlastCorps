/*
 * hd_code 8A080 (us.v11 0x802CE840-0x802CEAA0): the table of 25 moving
 * objects the level's ammo boxes, crates and blocks register for the
 * collision code (D_803FB8B8), and the run-time collision triangles of the
 * blocks' holes (D_803F9330, made by 5CB60's func_802A41B0), as native C
 * (engine.h).
 */
#include "engine.h"
#include "collision.h"

#define N_COLLISION_OBJECTS 25  /* D_803FB8B8's entries */
#define KEY_FREE (-1)           /* an entry no object has */
#define N_HOLE_TRIS 100         /* D_803F9330's triangles */

typedef struct CollisionObject {
    /* 0x00 */ s32 key;         /* KEY_FREE: free */
    /* 0x04 */ s32 x, y, z;
    /* 0x10 */ s32 unk10;
} CollisionObject;
SIZE_CHECK(CollisionObject, 0x14);

extern CollisionObject D_803FB8B8[N_COLLISION_OBJECTS];
extern CollisionTri D_803F9330[N_HOLE_TRIS];
extern CollisionTri *PTR32 D_803FB8B0;  /* the next free one */

/* func_802CE840 (00000.c's): empty the table */
void func_802CE840(void) {
    CollisionObject *o = D_803FB8B8;
    s32 n;

    for (n = N_COLLISION_OBJECTS; n != 0; n--, o++) {
        o->key = KEY_FREE;
    }
}

/* the entry with `key`, or NULL: func_802CE90C's and func_802CE958's
   search */
#define FIND_KEY(o, key)                                                    \
    do {                                                                    \
        s32 n_ = N_COLLISION_OBJECTS;                                       \
                                                                            \
        for (o = D_803FB8B8; n_ != 0; n_--, o++) {      \
            if (o->key == (key)) {                                          \
                break;                                                      \
            }                                                               \
        }                                                                   \
        if (n_ == 0) {                                                      \
            o = NULL;                                                       \
        }                                                                   \
    } while (0)

/* func_802CE880: put object `key` at (x, y, z), taking a free entry if it
   has none (nothing happens when the table is full) */
void func_802CE880(s32 key, s32 x, s32 y, s32 z, s32 arg4) {
    CollisionObject *o;

    {
        s32 n = N_COLLISION_OBJECTS;

        for (o = D_803FB8B8; n != 0; n--, o++) {
            if (o->key == key) {
                break;
            }
        }
        if (n == 0) {
            o = NULL;
        }
    }
    if (o == NULL) {
        /* none yet: a free one */
        s32 n = N_COLLISION_OBJECTS;

        for (o = D_803FB8B8; n != 0; n--, o++) {
            if (o->key == KEY_FREE) {
                break;
            }
        }
        if (n == 0) {
            return;
        }
        o->key = key;
    }
    o->x = x;
    o->y = y;
    o->z = z;
    o->unk10 = arg4;
}

/* func_802CE90C: free object `key`'s entry */
void func_802CE90C(s32 key) {
    CollisionObject *o;

    FIND_KEY(o, key);
    if (o != NULL) {
        o->key = KEY_FREE;
    }
}

/* func_802CE958: whether object `key` has an entry */
s32 func_802CE958(s32 key) {
    CollisionObject *o;

    FIND_KEY(o, key);
    return o != NULL;
}

/* func_802CE9A4: no hole triangles */
void func_802CE9A4(void) {
    D_803FB8B0 = D_803F9330;
}

/* func_802CE9C8 (4B5E0.c's): run-time triangles from a hole's `n` level
   triangles, each active, with `arg2` as its group (0x52) */
void func_802CE9C8(LevelCollisionTri *tris, u8 n, u8 arg2) {
    CollisionTri *t = D_803FB8B0;
    s32 left;

    for (left = n; left != 0; left--, tris++) {
        t->active = 1;
        /* (0x4F, 0x50, 0x57, 0x58: what $t9, $v0, $t6 and $s1 held in the
           original; 0 here: nothing reads them for the holes' triangles,
           5CB60.c's LEVEL_TRI_BYTES) */
        t = (CollisionTri *)func_802A41B0((u32)t, (u32)tris, arg2, tris->unk14, 0, 0, 0, 0, 0);
    }
    D_803FB8B0 = t;
}

/* func_802CEA68: the triangles of [t, end) not active */
void func_802CEA68(CollisionTri *t, CollisionTri *end) {
    for (; t != end; t++) {
        t->active = 0;
    }
}
