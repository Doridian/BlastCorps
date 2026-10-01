/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the graphics
 * utilities, which port/src/gu.c implements.
 */
#ifndef PORT_SDK_GU_H
#define PORT_SDK_GU_H

#include <PR/mbi.h>
#include <PR/ultratypes.h>
#include <PR/sptask.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define M_PI 3.14159265358979323846
#define M_DTOR (3.14159265358979323846 / 180.0)

/* s15.16 fixed point, and an s0.7 fraction for a unit vector's component */
#define FTOFIX32(x) (long)((x) * (float)0x00010000)
#define FIX32TOF(x) ((float)(x) * (1.0f / (float)0x00010000))
#define FTOFRAC8(x) ((int)MIN(((x) * (128.0f)), 127.0f) & 0xff)

extern void guMtxIdent(Mtx *m);
extern void guMtxIdentF(float mf[4][4]);
extern void guOrtho(Mtx *m, float l, float r, float b, float t, float n, float f, float scale);
extern void guOrthoF(float mf[4][4], float l, float r, float b, float t, float n, float f, float scale);
extern void guPerspective(Mtx *m, u16 *perspNorm, float fovy, float aspect, float near, float far, float scale);
extern void guPerspectiveF(float mf[4][4], u16 *perspNorm, float fovy, float aspect, float near, float far,
                           float scale);
extern void guLookAt(Mtx *m, float xEye, float yEye, float zEye, float xAt, float yAt, float zAt, float xUp,
                     float yUp, float zUp);
extern void guLookAtF(float mf[4][4], float xEye, float yEye, float zEye, float xAt, float yAt, float zAt,
                      float xUp, float yUp, float zUp);
extern void guLookAtReflect(Mtx *m, LookAt *l, float xEye, float yEye, float zEye, float xAt, float yAt, float zAt,
                            float xUp, float yUp, float zUp);
extern void guLookAtReflectF(float mf[4][4], LookAt *l, float xEye, float yEye, float zEye, float xAt, float yAt,
                             float zAt, float xUp, float yUp, float zUp);
extern void guRotate(Mtx *m, float a, float x, float y, float z);
extern void guRotateF(float mf[4][4], float a, float x, float y, float z);
extern void guScale(Mtx *m, float x, float y, float z);
extern void guScaleF(float mf[4][4], float x, float y, float z);
extern void guTranslate(Mtx *m, float x, float y, float z);
extern void guTranslateF(float mf[4][4], float x, float y, float z);
extern void guMtxF2L(float mf[4][4], Mtx *m);
extern void guMtxL2F(float mf[4][4], Mtx *m);
extern void guMtxCatF(float m[4][4], float n[4][4], float r[4][4]);
extern void guMtxCatL(Mtx *m, Mtx *n, Mtx *res);
extern void guMtxXFMF(float mf[4][4], float x, float y, float z, float *ox, float *oy, float *oz);
extern void guMtxXFML(Mtx *m, float x, float y, float z, float *ox, float *oy, float *oz);
extern void guNormalize(float *x, float *y, float *z);

extern float sinf(float angle);
extern float cosf(float angle);
extern signed short sins(unsigned short angle);    /* 0x10000 a turn, s1.15 */
extern signed short coss(unsigned short angle);
extern float sqrtf(float value);

#endif
