/*
 * The check build's fuzz (PORT_ENGINE_FUZZ, port/host/engine.c): a version's
 * driver, engine_fuzz(), calls replaced functions on game memory it varies,
 * and the check compares each call's translation and native code.  These
 * are its helpers.  Only in the check build (PORT_ENGINE_CHECK).
 */
#ifndef ENGINE_FUZZ_H
#define ENGINE_FUZZ_H

#include "engine.h"

/* game memory, the context and the counts as they were at the read */
void engine_fuzz_reset(void);

typedef struct FuzzRng {
    u32 s;
} FuzzRng;

static inline u32 fuzz_u32(FuzzRng *r) {
    u32 x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return r->s = x;
}

/* lo..hi, both included */
static inline s32 fuzz_int(FuzzRng *r, s32 lo, s32 hi) {
    return lo + (s32)(fuzz_u32(r) % (u32)(hi - lo + 1));
}

static inline f32 fuzz_float(FuzzRng *r, f32 lo, f32 hi) {
    return lo + (hi - lo) * (f32)(fuzz_u32(r) & 0xFFFF) / 65535.0f;
}

#define FUZZ_PICK(r, ...)                                                   \
    ({                                                                      \
        static const s32 v_[] = { __VA_ARGS__ };                            \
        v_[fuzz_u32(r) % (sizeof v_ / sizeof v_[0])];                       \
    })

#endif
