/*
 * A float converted to an integer where the value can be out of the
 * type's range or NaN, which C leaves undefined: truncated toward zero to
 * 32 bits, 0x80000000 for NaN and what doesn't fit (x86's cvttss2si and
 * cvttsd2si: what the TAS's mupen64plus on x86 and the port always gave),
 * then narrowed by the caller's cast of an int, modulo (IDO's trunc.w and
 * sh/sb do the same with what fits).  Plain C, the same on any host: ARM
 * and PowerPC would saturate, MIPS give 0x7FFFFFFF.  For the game's C
 * (port_game.h) and the host.  The engine's VR4300 conversions, which give
 * 0x7FFFFFFF as the hardware does, are port/engine/cvt.h's.
 */
#ifndef PORT_F2I_H
#define PORT_F2I_H

static inline int port_f2i(double x) {
    if (!(x > -2147483649.0 && x < 2147483648.0))
        return (int)0x80000000u;
    return (int)x;
}

#endif
