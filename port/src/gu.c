/*
 * The graphics utilities the game calls (gu.h's interface): the port's own
 * implementation, in place of the SDK's (docs/DISTRIBUTION.md, "Replacing
 * the SDK parts").  Each result has to be the SDK's bit for bit, since the
 * game feeds them back into its own physics and camera, so every function
 * below does its arithmetic in the same precision and order the
 * function reference's formulas imply, and touches game memory with the
 * same access widths (the native-endian builds keep each width's own byte
 * order).  port/tools/sdk_check compares all of it with the originals.
 *
 * The N64 build is unchanged: it still links src/libultra, which has to match
 * the ROM.
 */
#include "common.h"

f32 fcos(f32);

/* (hd_code/90C50.c's .bss, which the port no longer builds) */
u8 D_803FDF60[0x10];

/* -DPORT_SDK_TRACE=ON: every call's arguments go to the host's log
   ($PORT_SDK_TRACE), port/tools/sdk_check's traced inputs.  One call per
   word, so the values pass in registers whatever the build's byte order.
   The function numbers are check.c's. */
#ifdef PORT_SDK_TRACE
void port_sdk_trace(int fn, int nwords);
void port_sdk_trace_word(u32 w);

static void trace_f(f32 f) {
    union {
        f32 f;
        u32 i;
    } u;

    u.f = f;
    port_sdk_trace_word(u.i);
}

static void trace_mf(float mf[4][4]) {
    int r, c;

    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            trace_f(mf[r][c]);
        }
    }
}

static void trace_mtx(Mtx *m) {
    int i;

    for (i = 0; i < 16; i++) {
        port_sdk_trace_word(((u32 *)m)[i]);
    }
}

/* bytes as they lie in a little-endian word, which is how check.c reads them */
static void trace_bytes(u8 *p, int n) {
    int i;

    for (i = 0; i < n; i += 4) {
        port_sdk_trace_word(p[i] | (p[i + 1] << 8) | (p[i + 2] << 16) | ((u32)p[i + 3] << 24));
    }
}
#define TRACE(fn, n) port_sdk_trace(fn, n)
#define TRACE_W(x) port_sdk_trace_word(x)
#define TRACE_F(x) trace_f(x)
#define TRACE_MF(m) trace_mf(m)
#define TRACE_MTX(m) trace_mtx(m)
#define TRACE_BYTES(p, n) trace_bytes(p, n)
#else
#define TRACE(fn, n) ((void)0)
#define TRACE_W(x) ((void)0)
#define TRACE_F(x) ((void)0)
#define TRACE_MF(m) ((void)0)
#define TRACE_MTX(m) ((void)0)
#define TRACE_BYTES(p, n) ((void)0)
#endif
#define TRACE_F3(a, b, c) (TRACE_F(a), TRACE_F(b), TRACE_F(c))
#define TRACE_F9(a, b, c, d, e, f, g, h, i) (TRACE_F3(a, b, c), TRACE_F3(d, e, f), TRACE_F3(g, h, i))

/* ---- sine and cosine ------------------------------------------------------
 *
 * float in, float out, computed in double: reduce x by multiples of pi
 * (Cody and Waite: pi split in two, the first part with enough trailing
 * zeros that n * PI_HI is exact), then an odd polynomial in the reduced
 * argument on [-pi/2, pi/2].  The polynomial's four coefficients are SGI's,
 * kept so the results are the original's (a correctly rounded sinf differs
 * from it on about 0.9% of inputs).
 */
#define INV_PI 0x1.45f306dc9c883p-2        /* 1/pi, rounded to double */
#define PI_HI 0x1.921fb50000000p+1         /* pi to 30 bits */
#define PI_LO 0x1.110b4611a6263p-25        /* pi - PI_HI */
#define S1 -0x1.55554bc83656dp-3
#define S2 0x1.110ed3804c2a0p-7
#define S3 -0x1.9f6ffeea56814p-13
#define S4 0x1.5dbdf0e314bfep-19

/* the exponent and the mantissa's top bit, without the sign: 0xFF is 1.5 */
#define SCALE_KEY(bits) (((bits) >> 22) & 0x1FF)
#define KEY_BELOW_1_5 0xFF
#define KEY_TINY 0xE6       /* below 2^-12, sin x is x in float */
#define KEY_HUGE 0x136      /* 2^28 and over, and NaN and infinity */

/* the NaN the original returns for a NaN (libm's __libm_qnan_f, a quiet NaN
   on MIPS) */
static const union {
    u32 i;
    f32 f;
} mips_qnan = { 0x7F810000 };

static double sin_poly(double r) {
    double r2 = r * r;
    double p = ((S4 * r2 + S3) * r2 + S2) * r2 + S1;

    return r + (r * r2) * p;
}

static int round_half_away(double d) {
    return (int)((d >= 0.0) ? (d + 0.5) : (d - 0.5));
}

f32 sinf(f32 x) {
    union {
        f32 f;
        s32 i;
    } u;
    double r, k;
    double s;
    int n, key;

    TRACE(0, 1);
    TRACE_F(x);
    u.f = x;
    key = SCALE_KEY(u.i);
    if (key < KEY_BELOW_1_5) {
        if (key < KEY_TINY) {
            return x;
        }
        return (f32)sin_poly(x);
    }
    if (key < KEY_HUGE) {
        r = x;
        n = round_half_away(r * INV_PI);
        k = n;
        r = r - k * PI_HI;
        r = r - k * PI_LO;
        s = sin_poly(r);
        return (n & 1) ? -(f32)s : (f32)s;
    }
    if (x != x) {
        return mips_qnan.f;
    }
    return 0.0f;
}

/* cos x = sin(x - (n - 1/2) pi) with the sign of n's parity, n the nearest
   whole number to |x|/pi + 1/2 */
f32 fcos(f32 x) {
    union {
        f32 f;
        s32 i;
    } u;
    f32 ax;
    double r, k;
    double s;
    int n;

    TRACE(1, 1);
    TRACE_F(x);
    u.f = x;
    if (SCALE_KEY(u.i) < KEY_HUGE) {
        ax = (x > 0) ? x : -x;
        r = ax;
        n = round_half_away(r * INV_PI + 0.5);
        k = n;
        k -= 0.5;
        r = r - k * PI_HI;
        r = r - k * PI_LO;
        s = sin_poly(r);
        return (n & 1) ? -(f32)s : (f32)s;
    }
    if (x != x) {
        return mips_qnan.f;
    }
    return 0.0f;
}

/* ---- sins and coss: a quarter wave of 1,024 s1.15 steps -------------------
 *
 * port/tools/gen_sintable.py writes the table: entry i is
 * trunc(32767 sin(i pi / 2046)), so the last entry is sin(pi/2).
 */
#include "sintable.h"

s16 sins(u16 angle) {
    u16 step = angle >> 4;              /* 4,096 steps a turn */
    u16 i = step & 0x3FF;
    s16 v = (step & 0x400) ? port_sintable[0x3FF - i] : port_sintable[i];

    return (step & 0x800) ? -v : v;
}

s16 coss(u16 angle) {
    return sins((u16)(angle + 0x4000));
}

/* ---- floating-point matrices ---------------------------------------------- */

void guMtxIdentF(float mf[4][4]) {
    int r, c;

    TRACE(2, 0);
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            mf[r][c] = (r == c) ? 1.0f : 0.0f;
        }
    }
}

void guTranslateF(float mf[4][4], float x, float y, float z) {
    TRACE(4, 3);
    TRACE_F3(x, y, z);    guMtxIdentF(mf);
    mf[3][0] = x;
    mf[3][1] = y;
    mf[3][2] = z;
}

void guScaleF(float mf[4][4], float x, float y, float z) {
    TRACE(6, 3);
    TRACE_F3(x, y, z);    guMtxIdentF(mf);
    mf[0][0] = x;
    mf[1][1] = y;
    mf[2][2] = z;
    mf[3][3] = 1.0f;
}

static void scale_all(float mf[4][4], float s) {
    int r, c;

    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            mf[r][c] *= s;
        }
    }
}

void guOrthoF(float mf[4][4], float l, float r, float b, float t, float n, float f, float scale) {
    TRACE(8, 7);
    TRACE_F3(l, r, b);
    TRACE_F3(t, n, f);
    TRACE_F(scale);    guMtxIdentF(mf);
    mf[0][0] = 2 / (r - l);
    mf[1][1] = 2 / (t - b);
    mf[2][2] = -2 / (f - n);
    mf[3][0] = -(r + l) / (r - l);
    mf[3][1] = -(t + b) / (t - b);
    mf[3][2] = -(f + n) / (f - n);
    mf[3][3] = 1;
    scale_all(mf, scale);
}

void guPerspectiveF(float mf[4][4], u16 *perspNorm, float fovy, float aspect, float near, float far, float scale) {
    float half, cot;

    TRACE(10, 6);
    TRACE_F3(fovy, aspect, near);
    TRACE_F(far);
    TRACE_F(scale);
    TRACE_W(perspNorm != NULL);
    guMtxIdentF(mf);
    fovy *= 3.1415926 / 180.0;          /* in double */
    half = fovy / 2;
    cot = fcos(half) / sinf(half);
    mf[0][0] = cot / aspect;
    mf[1][1] = cot;
    mf[2][2] = (near + far) / (near - far);
    mf[2][3] = -1;
    mf[3][2] = (2 * near * far) / (near - far);
    mf[3][3] = 0;
    scale_all(mf, scale);
    if (perspNorm != NULL) {
        /* the RSP's perspective normalization: 2^17 / (near + far), at
           least 1, and 0xFFFF when near + far is at most 2 */
        if (near + far <= 2.0) {
            *perspNorm = 0xFFFF;
        } else {
            *perspNorm = (u16)(131072.0 / (near + far));
            if (*perspNorm == 0) {
                *perspNorm = 1;
            }
        }
    }
}

/* the camera's frame: look (negated, so +z is behind), right = up x look,
   up = look x right, each made unit length (the reciprocals in double) */
typedef struct {
    float look[3], right[3], up[3];
} Frame;

static void unit(float v[3], double scale) {
    float k = scale / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= k;
    v[1] *= k;
    v[2] *= k;
}

static void cross(float out[3], const float a[3], const float b[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static void look_frame(Frame *fr, float xEye, float yEye, float zEye, float xAt, float yAt, float zAt,
                       float xUp, float yUp, float zUp) {
    float up[3];

    fr->look[0] = xAt - xEye;
    fr->look[1] = yAt - yEye;
    fr->look[2] = zAt - zEye;
    unit(fr->look, -1.0);
    up[0] = xUp;
    up[1] = yUp;
    up[2] = zUp;
    cross(fr->right, up, fr->look);
    unit(fr->right, 1.0);
    cross(fr->up, fr->look, fr->right);
    unit(fr->up, 1.0);
}

static float neg_dot(float x, float y, float z, const float v[3]) {
    return -(x * v[0] + y * v[1] + z * v[2]);
}

/* (no loops here or in guLookAtReflectF: the port polls on every loop back
   edge of the N64 side, which takes virtual time, so these functions keep
   the originals' count of them: none but guMtxIdentF's) */
static void frame_to_mtx(float mf[4][4], const Frame *fr, float xEye, float yEye, float zEye) {
    mf[0][0] = fr->right[0];
    mf[1][0] = fr->right[1];
    mf[2][0] = fr->right[2];
    mf[3][0] = neg_dot(xEye, yEye, zEye, fr->right);
    mf[0][1] = fr->up[0];
    mf[1][1] = fr->up[1];
    mf[2][1] = fr->up[2];
    mf[3][1] = neg_dot(xEye, yEye, zEye, fr->up);
    mf[0][2] = fr->look[0];
    mf[1][2] = fr->look[1];
    mf[2][2] = fr->look[2];
    mf[3][2] = neg_dot(xEye, yEye, zEye, fr->look);
    mf[0][3] = 0;
    mf[1][3] = 0;
    mf[2][3] = 0;
    mf[3][3] = 1;
}

void guLookAtF(float mf[4][4], float xEye, float yEye, float zEye, float xAt, float yAt, float zAt, float xUp,
               float yUp, float zUp) {
    Frame fr;

    TRACE(12, 9);
    TRACE_F9(xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    guMtxIdentF(mf);
    look_frame(&fr, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    frame_to_mtx(mf, &fr, xEye, yEye, zEye);
}

/* a unit component as an s0.7 byte, 1.0 clamped to 127 */
static u8 frac8(float v) {
    double d = v * 128.0;

    return (int)((d < 127.0) ? d : 127.0) & 0xFF;
}

void guLookAtReflectF(float mf[4][4], LookAt *l, float xEye, float yEye, float zEye, float xAt, float yAt, float zAt,
                      float xUp, float yUp, float zUp) {
    Frame fr;

    TRACE(14, 9);
    TRACE_F9(xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    guMtxIdentF(mf);
    look_frame(&fr, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    /* the two reflection lights: right in black, up in green (0x80) */
    l->l[0].l.dir[0] = frac8(fr.right[0]);
    l->l[0].l.dir[1] = frac8(fr.right[1]);
    l->l[0].l.dir[2] = frac8(fr.right[2]);
    l->l[1].l.dir[0] = frac8(fr.up[0]);
    l->l[1].l.dir[1] = frac8(fr.up[1]);
    l->l[1].l.dir[2] = frac8(fr.up[2]);
    l->l[0].l.col[0] = l->l[0].l.col[1] = l->l[0].l.col[2] = 0;
    l->l[0].l.colc[0] = l->l[0].l.colc[1] = l->l[0].l.colc[2] = 0;
    l->l[1].l.col[0] = l->l[1].l.col[2] = 0;
    l->l[1].l.colc[0] = l->l[1].l.colc[2] = 0;
    l->l[1].l.col[1] = l->l[1].l.colc[1] = 0x80;
    l->l[0].l.pad1 = l->l[0].l.pad2 = 0;
    l->l[1].l.pad1 = l->l[1].l.pad2 = 0;
    frame_to_mtx(mf, &fr, xEye, yEye, zEye);
}

void guNormalize(float *x, float *y, float *z) {
    float k;

    TRACE(16, 3);
    TRACE_F3(*x, *y, *z);
    k = 1 / sqrtf(*x * *x + *y * *y + *z * *z);

    *x *= k;
    *y *= k;
    *z *= k;
}

/* rotation by a degrees about (x, y, z) (normalized first) */
void guRotateF(float mf[4][4], float a, float x, float y, float z) {
    float s, c, t, xx, yy, zz, xyt, yzt, zxt;

    TRACE(17, 4);
    TRACE_F(a);
    TRACE_F3(x, y, z);
    guNormalize(&x, &y, &z);
    a *= (float)(3.1415926 / 180.0);
    s = sinf(a);
    c = fcos(a);
    t = 1 - c;
    xyt = x * y * t;
    yzt = y * z * t;
    zxt = z * x * t;
    guMtxIdentF(mf);
    xx = x * x;
    yy = y * y;
    zz = z * z;
    mf[0][0] = xx + c * (1 - xx);
    mf[1][1] = yy + c * (1 - yy);
    mf[2][2] = zz + c * (1 - zz);
    mf[1][2] = yzt + x * s;
    mf[2][1] = yzt - x * s;
    mf[2][0] = zxt + y * s;
    mf[0][2] = zxt - y * s;
    mf[0][1] = xyt + z * s;
    mf[1][0] = xyt - z * s;
}

/* res = mf nf; res may be either input */
void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]) {
    float out[4][4];
    int r, c, k;

    TRACE(19, 32);
    TRACE_MF(mf);
    TRACE_MF(nf);
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            float sum = 0.0f;

            for (k = 0; k < 4; k++) {
                sum += mf[r][k] * nf[k][c];
            }
            out[r][c] = sum;
        }
    }
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            res[r][c] = out[r][c];
        }
    }
}

/* the point (x, y, z, 1) times mf */
void guMtxXFMF(float mf[4][4], float x, float y, float z, float *ox, float *oy, float *oz) {
    TRACE(21, 19);
    TRACE_MF(mf);
    TRACE_F3(x, y, z);    *ox = mf[0][0] * x + mf[1][0] * y + mf[2][0] * z + mf[3][0];
    *oy = mf[0][1] * x + mf[1][1] * y + mf[2][1] * z + mf[3][1];
    *oz = mf[0][2] * x + mf[1][2] * y + mf[2][2] * z + mf[3][2];
}

/* ---- the RSP's fixed-point matrix -------------------------------------------
 *
 * Sixteen s15.16 elements in row order, stored as two halves of 16 words:
 * the first eight words hold the elements' integer parts, two to a word
 * (the even column high), the last eight their fractions in the same way.
 * Written and read a word at a time, as the RSP and the renderer see it.
 */
void guMtxF2L(float mf[4][4], Mtx *m) {
    u32 *whole = (u32 *)m;
    u32 *frac = whole + 8;
    int r, c;

    TRACE(22, 16);
    TRACE_MF(mf);
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c += 2) {
            s32 a = FTOFIX32(mf[r][c]);
            s32 b = FTOFIX32(mf[r][c + 1]);

            *whole++ = ((u32)a & 0xFFFF0000) | ((u32)b >> 16);
            *frac++ = ((u32)a << 16) | ((u32)b & 0xFFFF);
        }
    }
}

void guMtxL2F(float mf[4][4], Mtx *m) {
    u32 *whole = (u32 *)m;
    u32 *frac = whole + 8;
    int r, c;

    TRACE(23, 16);
    TRACE_MTX(m);
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c += 2) {
            u32 w =*whole++;
            u32 f = *frac++;

            mf[r][c] = (float)(s32)((w & 0xFFFF0000) | (f >> 16)) / 65536.0f;
            mf[r][c + 1] = (float)(s32)((w << 16) | (f & 0xFFFF)) / 65536.0f;
        }
    }
}

void guMtxIdent(Mtx *m) {
    float mf[4][4];

    guMtxIdentF(mf);
    guMtxF2L(mf, m);
}

void guTranslate(Mtx *m, float x, float y, float z) {
    float mf[4][4];

    guTranslateF(mf, x, y, z);
    guMtxF2L(mf, m);
}

void guScale(Mtx *m, float x, float y, float z) {
    float mf[4][4];

    guScaleF(mf, x, y, z);
    guMtxF2L(mf, m);
}

void guOrtho(Mtx *m, float l, float r, float b, float t, float n, float f, float scale) {
    float mf[4][4];

    guOrthoF(mf, l, r, b, t, n, f, scale);
    guMtxF2L(mf, m);
}

void guPerspective(Mtx *m, u16 *perspNorm, float fovy, float aspect, float near, float far, float scale) {
    float mf[4][4];

    guPerspectiveF(mf, perspNorm, fovy, aspect, near, far, scale);
    guMtxF2L(mf, m);
}

void guLookAt(Mtx *m, float xEye, float yEye, float zEye, float xAt, float yAt, float zAt, float xUp, float yUp,
              float zUp) {
    float mf[4][4];

    guLookAtF(mf, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    guMtxF2L(mf, m);
}

void guLookAtReflect(Mtx *m, LookAt *l, float xEye, float yEye, float zEye, float xAt, float yAt, float zAt,
                     float xUp, float yUp, float zUp) {
    float mf[4][4];

    guLookAtReflectF(mf, l, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    guMtxF2L(mf, m);
}

void guRotate(Mtx *m, float a, float x, float y, float z) {
    float mf[4][4];

    guRotateF(mf, a, x, y, z);
    guMtxF2L(mf, m);
}

void guMtxCatL(Mtx *m, Mtx *n, Mtx *res) {
    float mf[4][4], nf[4][4], rf[4][4];

    guMtxL2F(mf, m);
    guMtxL2F(nf, n);
    guMtxCatF(mf, nf, rf);
    guMtxF2L(rf, res);
}

void guMtxXFML(Mtx *m, float x, float y, float z, float *ox, float *oy, float *oz) {
    float mf[4][4];

    guMtxL2F(mf, m);
    guMtxXFMF(mf, x, y, z, ox, oy, oz);
}

/* ---- the Controller Pak's data CRC ------------------------------------------
 *
 * CRC-8, polynomial x^8 + x^7 + x^2 + 1 (0x85), over a 32-byte block, most
 * significant bit first, with eight zero bits appended (as the pak sends it
 * back).  The game also checksums its saves with it (E7B0.c).
 */
u8 __osContDataCrc(u8 *data) {
    u32 crc = 0;
    int i, bit;

    TRACE(26, 8);
    TRACE_BYTES(data, 32);
    for (i = 0; i < 33; i++) {
        u32 in = (i < 32) ? data[i] : 0;

        for (bit = 7; bit >= 0; bit--) {
            u32 carry = crc & 0x80;

            crc = ((crc << 1) | ((in >> bit) & 1)) & 0xFF;
            if (carry) {
                crc ^= 0x85;
            }
        }
    }
    return crc;
}
