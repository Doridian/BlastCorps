/*
 * port/tools/sdk_check: the port's own SDK pieces (port/src/gu.c) against
 * the SDK's originals, bit for bit (sdk_check.py builds and runs this).
 *
 *   check exhaustive     sinf and fcos over all 2^32 floats, sins and coss
 *                        over all 65,536 angles
 *   check random N SEED  every gu function and the pak CRC on N random
 *                        argument sets each
 *   check trace FILE     the arguments a port run logged (-DPORT_SDK_TRACE)
 *
 * The originals are built from src/libultra under orig_ names, the
 * replacements under new_ names; neither is the port's build.
 */
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int32_t m[16];
} Mtx;
typedef struct {
    uint8_t b[32];
} LookAt;
typedef float MF[4][4];

#define DECLARE(p)                                                                                       \
    float p##sinf(float);                                                                                \
    float p##fcos(float);                                                                                \
    int16_t p##sins(uint16_t);                                                                           \
    int16_t p##coss(uint16_t);                                                                           \
    void p##guMtxIdentF(MF);                                                                             \
    void p##guMtxIdent(Mtx *);                                                                           \
    void p##guTranslateF(MF, float, float, float);                                                       \
    void p##guTranslate(Mtx *, float, float, float);                                                     \
    void p##guScaleF(MF, float, float, float);                                                           \
    void p##guScale(Mtx *, float, float, float);                                                         \
    void p##guOrthoF(MF, float, float, float, float, float, float, float);                               \
    void p##guOrtho(Mtx *, float, float, float, float, float, float, float);                             \
    void p##guPerspectiveF(MF, uint16_t *, float, float, float, float, float);                           \
    void p##guPerspective(Mtx *, uint16_t *, float, float, float, float, float);                         \
    void p##guLookAtF(MF, float, float, float, float, float, float, float, float, float);                \
    void p##guLookAt(Mtx *, float, float, float, float, float, float, float, float, float);              \
    void p##guLookAtReflectF(MF, LookAt *, float, float, float, float, float, float, float, float, float); \
    void p##guLookAtReflect(Mtx *, LookAt *, float, float, float, float, float, float, float, float, float); \
    void p##guNormalize(float *, float *, float *);                                                      \
    void p##guRotateF(MF, float, float, float, float);                                                   \
    void p##guRotate(Mtx *, float, float, float, float);                                                 \
    void p##guMtxCatF(MF, MF, MF);                                                                       \
    void p##guMtxXFMF(MF, float, float, float, float *, float *, float *);                               \
    void p##guMtxF2L(MF, Mtx *);                                                                         \
    void p##guMtxL2F(MF, Mtx *);                                                                         \
    void p##guMtxCatL(Mtx *, Mtx *, Mtx *);                                                              \
    void p##guMtxXFML(Mtx *, float, float, float, float *, float *, float *);                            \
    uint8_t p##__osContDataCrc(uint8_t *);

DECLARE(orig_)
DECLARE(new_)

/* the originals' NaN (libm's, in hd_code's .rodata) */
union {
    uint32_t i;
    float f;
} orig___libm_qnan_f_u = { 0x7F810000 };
float orig___libm_qnan_f;

static uint32_t fbits(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

/* ---- exhaustive --------------------------------------------------------- */

#define NTHREADS 16
static unsigned long long bad_sin, bad_cos;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *sweep(void *arg) {
    uint64_t t = (uintptr_t)arg, i;
    unsigned long long bs = 0, bc = 0;

    for (i = t; i < (1ull << 32); i += NTHREADS) {
        uint32_t u = (uint32_t)i;
        float x;

        memcpy(&x, &u, 4);
        if (fbits(orig_sinf(x)) != fbits(new_sinf(x))) {
            if (bs++ < 4)
                fprintf(stderr, "sinf(%08x): %08x vs %08x\n", u, fbits(orig_sinf(x)), fbits(new_sinf(x)));
        }
        if (fbits(orig_fcos(x)) != fbits(new_fcos(x))) {
            if (bc++ < 4)
                fprintf(stderr, "fcos(%08x): %08x vs %08x\n", u, fbits(orig_fcos(x)), fbits(new_fcos(x)));
        }
    }
    pthread_mutex_lock(&lock);
    bad_sin += bs;
    bad_cos += bc;
    pthread_mutex_unlock(&lock);
    return NULL;
}

static int exhaustive(void) {
    pthread_t th[NTHREADS];
    int i, bad = 0;

    for (i = 0; i < NTHREADS; i++)
        pthread_create(&th[i], NULL, sweep, (void *)(uintptr_t)i);
    for (i = 0; i < NTHREADS; i++)
        pthread_join(th[i], NULL);
    printf("sinf: %llu of 2^32 differ\nfcos: %llu of 2^32 differ\n", bad_sin, bad_cos);
    for (i = 0; i < 0x10000; i++) {
        if (orig_sins(i) != new_sins(i) || orig_coss(i) != new_coss(i))
            bad++;
    }
    printf("sins/coss: %d of 65536 differ\n", bad);
    return (bad_sin || bad_cos || bad) ? 1 : 0;
}

/* ---- one call, either side ---------------------------------------------- */

enum {
    F_SINF, F_FCOS, F_IDENTF, F_IDENT, F_TRANSLATEF, F_TRANSLATE, F_SCALEF, F_SCALE, F_ORTHOF, F_ORTHO,
    F_PERSPECTIVEF, F_PERSPECTIVE, F_LOOKATF, F_LOOKAT, F_LOOKATREFLECTF, F_LOOKATREFLECT, F_NORMALIZE,
    F_ROTATEF, F_ROTATE, F_MTXCATF, F_MTXCATF_ALIAS, F_MTXXFMF, F_MTXF2L, F_MTXL2F, F_MTXCATL, F_MTXXFML,
    F_CRC, F_COUNT
};
static const char *const names[F_COUNT] = {
    "sinf", "fcos", "guMtxIdentF", "guMtxIdent", "guTranslateF", "guTranslate", "guScaleF", "guScale",
    "guOrthoF", "guOrtho", "guPerspectiveF", "guPerspective", "guLookAtF", "guLookAt", "guLookAtReflectF",
    "guLookAtReflect", "guNormalize", "guRotateF", "guRotate", "guMtxCatF", "guMtxCatF (in place)",
    "guMtxXFMF", "guMtxF2L", "guMtxL2F", "guMtxCatL", "guMtxXFML", "__osContDataCrc",
};
/* argument words each takes */
static const int nargs[F_COUNT] = {
    1, 1, 0, 0, 3, 3, 3, 3, 7, 7, 6, 6, 9, 9, 9, 9, 3, 4, 4, 32, 32, 19, 16, 16, 32, 19, 8,
};

#define OUTSZ 256

#define CALLS(p)                                                                                         \
    static void call_##p(int fn, const uint32_t *w, uint8_t *out) {                                      \
        float a[32];                                                                                     \
        MF *mf = (MF *)out;                                                                              \
        Mtx *mtx = (Mtx *)out;                                                                           \
        memcpy(a, w, sizeof(float) * nargs[fn]);                                                         \
        switch (fn) {                                                                                    \
        case F_SINF: { float r = p##sinf(a[0]); memcpy(out, &r, 4); break; }                             \
        case F_FCOS: { float r = p##fcos(a[0]); memcpy(out, &r, 4); break; }                             \
        case F_IDENTF: p##guMtxIdentF(*mf); break;                                                       \
        case F_IDENT: p##guMtxIdent(mtx); break;                                                         \
        case F_TRANSLATEF: p##guTranslateF(*mf, a[0], a[1], a[2]); break;                                \
        case F_TRANSLATE: p##guTranslate(mtx, a[0], a[1], a[2]); break;                                  \
        case F_SCALEF: p##guScaleF(*mf, a[0], a[1], a[2]); break;                                        \
        case F_SCALE: p##guScale(mtx, a[0], a[1], a[2]); break;                                          \
        case F_ORTHOF: p##guOrthoF(*mf, a[0], a[1], a[2], a[3], a[4], a[5], a[6]); break;                \
        case F_ORTHO: p##guOrtho(mtx, a[0], a[1], a[2], a[3], a[4], a[5], a[6]); break;                  \
        case F_PERSPECTIVEF:                                                                             \
            p##guPerspectiveF(*mf, (w[5] & 1) ? (uint16_t *)(out + 128) : NULL, a[0], a[1], a[2], a[3], a[4]); \
            break;                                                                                       \
        case F_PERSPECTIVE:                                                                              \
            p##guPerspective(mtx, (w[5] & 1) ? (uint16_t *)(out + 128) : NULL, a[0], a[1], a[2], a[3], a[4]); \
            break;                                                                                       \
        case F_LOOKATF: p##guLookAtF(*mf, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]); break;   \
        case F_LOOKAT: p##guLookAt(mtx, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]); break;     \
        case F_LOOKATREFLECTF:                                                                           \
            p##guLookAtReflectF(*mf, (LookAt *)(out + 128), a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]); \
            break;                                                                                       \
        case F_LOOKATREFLECT:                                                                            \
            p##guLookAtReflect(mtx, (LookAt *)(out + 128), a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]); \
            break;                                                                                       \
        case F_NORMALIZE: {                                                                              \
            float x = a[0], y = a[1], z = a[2];                                                          \
            p##guNormalize(&x, &y, &z);                                                                  \
            memcpy(out, &x, 4); memcpy(out + 4, &y, 4); memcpy(out + 8, &z, 4);                          \
            break;                                                                                       \
        }                                                                                                \
        case F_ROTATEF: p##guRotateF(*mf, a[0], a[1], a[2], a[3]); break;                                \
        case F_ROTATE: p##guRotate(mtx, a[0], a[1], a[2], a[3]); break;                                  \
        case F_MTXCATF: p##guMtxCatF(*(MF *)&a[0], *(MF *)&a[16], *mf); break;                           \
        case F_MTXCATF_ALIAS:                                                                            \
            memcpy(out, a, 64);                                                                          \
            p##guMtxCatF(*mf, *(MF *)&a[16], *mf);                                                       \
            break;                                                                                       \
        case F_MTXXFMF:                                                                                  \
            p##guMtxXFMF(*(MF *)&a[0], a[16], a[17], a[18], (float *)out, (float *)(out + 4), (float *)(out + 8)); \
            break;                                                                                       \
        case F_MTXF2L: p##guMtxF2L(*(MF *)&a[0], mtx); break;                                            \
        case F_MTXL2F: p##guMtxL2F(*mf, (Mtx *)&a[0]); break;                                            \
        case F_MTXCATL: p##guMtxCatL((Mtx *)&a[0], (Mtx *)&a[16], mtx); break;                           \
        case F_MTXXFML:                                                                                  \
            p##guMtxXFML((Mtx *)&a[0], a[16], a[17], a[18], (float *)out, (float *)(out + 4), (float *)(out + 8)); \
            break;                                                                                       \
        case F_CRC: out[0] = p##__osContDataCrc((uint8_t *)a); break;                                    \
        }                                                                                                \
    }
CALLS(orig_)
CALLS(new_)

static unsigned long long ncalls[F_COUNT], nbad[F_COUNT], nnan[F_COUNT];

static void compare(int fn, const uint32_t *w) {
    uint8_t o[OUTSZ], n[OUTSZ];
    int i;

    memset(o, 0xA5, OUTSZ);
    memset(n, 0xA5, OUTSZ);
    call_orig_(fn, w, o);
    call_new_(fn, w, n);
    ncalls[fn]++;
    /* a NaN's sign and payload are the compiler's choice (which operand an
       SSE instruction passes on, whether x * 1 is folded): two NaNs count as
       the same result, and are counted */
    for (i = 0; i < OUTSZ; i += 4) {
        uint32_t a, b;

        memcpy(&a, o + i, 4);
        memcpy(&b, n + i, 4);
        if (a != b && (a & 0x7F800000) == 0x7F800000 && (a & 0x7FFFFF) && (b & 0x7F800000) == 0x7F800000 &&
            (b & 0x7FFFFF)) {
            memcpy(n + i, &a, 4);
            nnan[fn]++;
        }
    }
    if (memcmp(o, n, OUTSZ)) {
        if (nbad[fn]++ < 3) {
            fprintf(stderr, "%s differs, arguments", names[fn]);
            for (i = 0; i < nargs[fn]; i++)
                fprintf(stderr, " %08x", w[i]);
            fprintf(stderr, "\n");
            for (i = 0; i < OUTSZ; i += 4) {
                if (memcmp(o + i, n + i, 4))
                    fprintf(stderr, "  +%3d: %08x vs %08x\n", i, *(uint32_t *)(o + i), *(uint32_t *)(n + i));
            }
        }
    }
}

static int report(void) {
    int fn, bad = 0;

    for (fn = 0; fn < F_COUNT; fn++) {
        if (ncalls[fn] == 0)
            continue;
        printf("%-22s %10llu calls, %llu differ", names[fn], ncalls[fn], nbad[fn]);
        if (nnan[fn])
            printf(" (%llu NaNs with another sign or payload)", nnan[fn]);
        printf("\n");
        if (nbad[fn])
            bad = 1;
    }
    return bad;
}

/* ---- random -------------------------------------------------------------- */

static uint64_t rng;
static uint32_t rnd(void) {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 16);
}

/* mostly ordinary values at several scales, sometimes the edge cases */
static float rnd_float(void) {
    static const float scales[] = { 1.0f, 2.0f, 10.0f, 360.0f, 1000.0f, 32768.0f, 1e6f, 1e-3f };
    uint32_t r = rnd() % 100, u;
    float f;

    if (r < 85) {
        f = ((float)rnd() / 4294967296.0f * 2.0f - 1.0f) * scales[rnd() % 8];
        return f;
    }
    if (r < 90) {
        static const float special[] = { 0.0f, -0.0f, 1.0f, -1.0f, 0.5f, 2.0f, 65535.0f, 32767.99998f,
                                         -32768.0f, 1e-40f, 1e30f, -1e30f, INFINITY, -INFINITY, NAN };
        return special[rnd() % (sizeof(special) / sizeof(special[0]))];
    }
    if (r < 95) {
        f = (float)((int)(rnd() % 721) - 360);   /* whole degrees */
        return f;
    }
    u = rnd();                                   /* any bits */
    memcpy(&f, &u, 4);
    return f;
}

static void rnd_args(int fn, uint32_t *w) {
    int i;

    for (i = 0; i < nargs[fn]; i++) {
        float f = rnd_float();

        memcpy(&w[i], &f, 4);
    }
    if (fn == F_MTXL2F || fn == F_MTXCATL || fn == F_MTXXFML || fn == F_CRC) {
        int n = (fn == F_MTXXFML) ? 16 : nargs[fn];

        for (i = 0; i < n; i++)                 /* fixed-point words: any bits */
            w[i] = (rnd() % 4) ? (rnd() & 0x0003FFFF) ^ ((rnd() & 1) ? 0xFFFFFFFF : 0) : rnd();
    }
    if (fn == F_PERSPECTIVEF || fn == F_PERSPECTIVE)
        w[5] = rnd();
}

static int random_test(unsigned long long n, unsigned long long seed) {
    unsigned long long i;
    uint32_t w[32];
    int fn;

    rng = seed * 0x9E3779B97F4A7C15ull + 1;
    for (fn = 0; fn < F_COUNT; fn++) {
        for (i = 0; i < n; i++) {
            rnd_args(fn, w);
            compare(fn, w);
        }
    }
    return report();
}

/* ---- traced -------------------------------------------------------------- */

/* records: u32 function (the enum above), then its argument words */
static int trace_test(const char *path) {
    FILE *f = fopen(path, "rb");
    uint32_t fn, w[32];

    if (!f) {
        perror(path);
        return 2;
    }
    while (fread(&fn, 4, 1, f) == 1) {
        if (fn >= F_COUNT || fread(w, 4, nargs[fn], f) != (size_t)nargs[fn]) {
            fprintf(stderr, "%s: bad record\n", path);
            return 2;
        }
        compare(fn, w);
        if (fn == F_MTXCATF)
            compare(F_MTXCATF_ALIAS, w);
    }
    fclose(f);
    return report();
}

int main(int argc, char **argv) {
    orig___libm_qnan_f = orig___libm_qnan_f_u.f;
    if (argc >= 2 && !strcmp(argv[1], "exhaustive"))
        return exhaustive();
    if (argc >= 4 && !strcmp(argv[1], "random"))
        return random_test(strtoull(argv[2], NULL, 0), strtoull(argv[3], NULL, 0));
    if (argc >= 3 && !strcmp(argv[1], "trace"))
        return trace_test(argv[2]);
    fprintf(stderr, "usage: %s exhaustive | random N SEED | trace FILE\n", argv[0]);
    return 2;
}
