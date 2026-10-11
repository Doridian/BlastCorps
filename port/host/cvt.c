/*
 * The engine's conversions and traps (port/engine/cvt.h): the VR4300's
 * float-to-integer conversions, IDO's float-to-unsigned, the original's
 * break and syscall, and what mfc0 reads.
 */
#include <math.h>
#include <stdint.h>

#include "port.h"
#include "../engine/cvt.h"

int port_round_half_up;

int32_t engine_cvt_w_s(float x) { return (int32_t)port_f2w(port_rintf(x)); }
int32_t engine_cvt_w_d(double x) { return (int32_t)port_f2w(port_rint(x)); }
int32_t engine_trunc_w_s(float x) { return (int32_t)port_f2w(truncf(x)); }
int32_t engine_trunc_w_d(double x) { return (int32_t)port_f2w(trunc(x)); }
int64_t engine_cvt_l_d(double x) { return (int64_t)port_f2l(port_rint(x)); }
int64_t engine_cvt_l_s(float x) { return (int64_t)port_f2l_s(port_rintf(x)); }

/* cvt.w.s with the FCSR set to truncate, its V flag read back */
static int ido_trunc_ok(float x, int32_t *v) {
    double r = trunc((double)x);
    if (!(r >= -2147483648.0 && r <= 2147483647.0))
        return 0;
    *v = (int32_t)r;
    return 1;
}

uint32_t engine_ido_cvt_u_s(float x) {
    int32_t v;
    if (ido_trunc_ok(x, &v))
        return v < 0 ? 0xFFFFFFFFu : (uint32_t)v;
    if (ido_trunc_ok(x - 2147483648.0f, &v))
        return (uint32_t)v | 0x80000000u;
    return 0xFFFFFFFFu;
}

void engine_break(uint32_t pc, uint32_t code) {
    host_fatal("the engine trapped: break %u at %08X", code, pc);
}

void engine_syscall(uint32_t pc) {
    host_fatal("the engine trapped: syscall at %08X", pc);
}

void engine_trap(uint32_t pc) {
    host_fatal("the engine trapped: syscall at %08X", pc);
}

uint32_t engine_mfc0(unsigned int reg) {
    if (reg == 12)          /* Status: interrupts enabled, CU1 */
        return 0x2000FF01u;
    if (reg == 9)           /* Count */
        return (uint32_t)host_ticks();
    return 0;
}
