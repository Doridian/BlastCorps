/*
 * recomp.h's rounding (recomp_rint, recomp_rintf: its own round-to-even
 * and floor) against libm's nearbyint, nearbyintf and floor, bit for bit,
 * in both of its modes, over edge cases and random doubles and floats:
 *
 *   cc -O2 -m32 -msse2 -mfpmath=sse -I tools/recomp/runtime port/tools/rint_check.c -lm && ./a.out
 *   cc -O2 -I tools/recomp/runtime port/tools/rint_check.c -lm && ./a.out
 *
 * (also under emcc and node).  Exit status 1 on a difference.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "recomp.h"

int recomp_round_half_up;

static uint64_t bits_d(double x) { uint64_t u; memcpy(&u, &x, 8); return u; }
static uint32_t bits_f(float x) { uint32_t u; memcpy(&u, &x, 4); return u; }
static double d_of(uint64_t u) { double x; memcpy(&x, &u, 8); return x; }
static float f_of(uint32_t u) { float x; memcpy(&x, &u, 4); return x; }

static long bad, n;

/* NaN results compare as NaN (the payload is whatever the operation
   gives; the callers turn NaN into 0x7FFFFFFF either way) */
static int same_d(double a, double b) { return (isnan(a) && isnan(b)) || bits_d(a) == bits_d(b); }
static int same_f(float a, float b) { return (isnan(a) && isnan(b)) || bits_f(a) == bits_f(b); }

static void one_d(double x) {
    double want, got;
    n++;
    recomp_round_half_up = 0;
    want = nearbyint(x), got = recomp_rint(x);
    if (!same_d(want, got) && bad++ < 20)
        printf("rint(%a): nearbyint %a, recomp %a\n", x, want, got);
    recomp_round_half_up = 1;
    want = floor(x + 0.5), got = recomp_rint(x);
    if (!same_d(want, got) && bad++ < 20)
        printf("rint half up(%a): floor %a, recomp %a\n", x, want, got);
    want = floor(x), got = recomp_floor(x);
    if (!same_d(want, got) && bad++ < 20)
        printf("floor(%a): floor %a, recomp %a\n", x, want, got);
}

static void one_f(float x) {
    float want, got;
    n++;
    recomp_round_half_up = 0;
    want = nearbyintf(x), got = recomp_rintf(x);
    if (!same_f(want, got) && bad++ < 20)
        printf("rintf(%a): nearbyintf %a, recomp %a\n", (double)x, (double)want, (double)got);
    recomp_round_half_up = 1;
    want = (float)floor(x + 0.5), got = recomp_rintf(x);
    if (!same_f(want, got) && bad++ < 20)
        printf("rintf half up(%a): floor %a, recomp %a\n", (double)x, (double)want, (double)got);
    one_d(x);
}

static uint64_t rng = 0x9E3779B97F4A7C15ull;
static uint64_t next(void) {
    rng ^= rng << 13, rng ^= rng >> 7, rng ^= rng << 17;
    return rng;
}

int main(void) {
    static const double edges[] = {
        0.0, 0.5, 1.0, 1.5, 2.5, 3.5, 0.49999999999999994, 0.5000000000000001, 1e-300, 4.9e-324,
        2147483647.0, 2147483647.5, 2147483648.0, 2147483648.5, 4294967295.5, 1e18,
        4503599627370495.5, 4503599627370496.0, 4503599627370497.0, 9007199254740991.0,
        9007199254740993.0, 9.2233720368547758e18, 1e308, INFINITY, NAN,
    };
    long k;
    int i, e;

    for (i = 0; i < (int)(sizeof edges / sizeof *edges); i++) {
        one_d(edges[i]), one_d(-edges[i]);
        one_d(nextafter(edges[i], 0)), one_d(-nextafter(edges[i], 0));
        one_d(nextafter(edges[i], INFINITY)), one_d(-nextafter(edges[i], INFINITY));
        one_f((float)edges[i]), one_f(-(float)edges[i]);
    }
    /* every half and quarter around 0, and the floats near 2^23 and 2^24 */
    for (k = -4096; k <= 4096; k++)
        one_d(k / 4.0), one_f(k / 4.0f);
    for (e = 21; e <= 25; e++)
        for (k = -64; k <= 64; k++)
            one_f(ldexpf(1.0f, e) + k * 0.25f), one_f(-ldexpf(1.0f, e) + k * 0.25f);
    /* random bit patterns, and random values of the magnitudes the game uses */
    for (k = 0; k < 20000000; k++) {
        uint64_t r = next();
        one_d(d_of(r));
        one_f(f_of((uint32_t)r));
        one_f((float)((int64_t)r >> 40) / 64.0f);
        one_d((double)((int64_t)r >> 20) / 1024.0);
    }
    printf("%ld inputs, %ld differences\n", n, bad);
    return bad != 0;
}
