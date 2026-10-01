/*
 * The port's own SDK headers (PR/ultratypes.h has the why): the audio
 * microcode's command list, as the audio library writes it and the port's
 * audio HLE (port/host/aspmain.c) reads it.  Each command is two words: the
 * opcode in the first one's top byte, its operands below and in the second.
 */
#ifndef PORT_SDK_ABI_H
#define PORT_SDK_ABI_H

/* the opcodes */
#define A_SPNOOP 0
#define A_ADPCM 1
#define A_CLEARBUFF 2
#define A_ENVMIXER 3
#define A_LOADBUFF 4
#define A_RESAMPLE 5
#define A_SAVEBUFF 6
#define A_SEGMENT 7
#define A_SETBUFF 8
#define A_SETVOL 9
#define A_DMEMMOVE 10
#define A_LOADADPCM 11
#define A_MIXER 12
#define A_INTERLEAVE 13
#define A_POLEF 14
#define A_SETLOOP 15

/* their flags */
#define A_INIT 0x01
#define A_CONTINUE 0x00
#define A_LOOP 0x02
#define A_OUT 0x02
#define A_LEFT 0x02
#define A_RIGHT 0x00
#define A_VOL 0x04
#define A_RATE 0x00
#define A_AUX 0x08
#define A_NOAUX 0x00
#define A_MAIN 0x00
#define A_MIX 0x10

typedef struct {
    unsigned int w0;
    unsigned int w1;
} Awords;

typedef union {
    Awords words;
    long long int force_union_align;
} Acmd;

/* the state the microcode keeps in RDRAM between frames */
#define ADPCMVSIZE 8
#define ADPCMFSIZE 16
typedef short ADPCM_STATE[ADPCMFSIZE];
typedef short POLEF_STATE[4];
typedef short RESAMPLE_STATE[16];
typedef short ENVMIX_STATE[40];

#define UNITY_PITCH 0x8000      /* the resampler's 1.0, in s1.15 */
#define MAX_RATIO 1.99996       /* just under an octave up */

/* a field of a command word: v's low w bits at bit s */
#ifndef _SHIFTL
#define _SHIFTL(v, s, w) ((unsigned int)(((unsigned int)(v) & ((0x01 << (w)) - 1)) << (s)))
#endif

#define A_CMD(pkt, w0v, w1v)                                                                                \
    {                                                                                                       \
        Acmd *_a = (Acmd *)pkt;                                                                             \
                                                                                                            \
        _a->words.w0 = w0v;                                                                                 \
        _a->words.w1 = w1v;                                                                                 \
    }

#define aADPCMdec(pkt, f, s) A_CMD(pkt, _SHIFTL(A_ADPCM, 24, 8) | _SHIFTL(f, 16, 8), (unsigned int)(s))
#define aPoleFilter(pkt, f, g, s)                                                                           \
    A_CMD(pkt, (_SHIFTL(A_POLEF, 24, 8) | _SHIFTL(f, 16, 8) | _SHIFTL(g, 0, 16)), (unsigned int)(s))
#define aClearBuffer(pkt, d, c) A_CMD(pkt, _SHIFTL(A_CLEARBUFF, 24, 8) | _SHIFTL(d, 0, 24), (unsigned int)(c))
#define aEnvMixer(pkt, f, s) A_CMD(pkt, _SHIFTL(A_ENVMIXER, 24, 8) | _SHIFTL(f, 16, 8), (unsigned int)(s))
#define aInterleave(pkt, l, r) A_CMD(pkt, _SHIFTL(A_INTERLEAVE, 24, 8), _SHIFTL(l, 16, 16) | _SHIFTL(r, 0, 16))
#define aLoadBuffer(pkt, s) A_CMD(pkt, _SHIFTL(A_LOADBUFF, 24, 8), (unsigned int)(s))
#define aMix(pkt, f, g, i, o)                                                                               \
    A_CMD(pkt, (_SHIFTL(A_MIXER, 24, 8) | _SHIFTL(f, 16, 8) | _SHIFTL(g, 0, 16)),                         \
          _SHIFTL(i, 16, 16) | _SHIFTL(o, 0, 16))
#define aResample(pkt, f, p, s)                                                                             \
    A_CMD(pkt, (_SHIFTL(A_RESAMPLE, 24, 8) | _SHIFTL(f, 16, 8) | _SHIFTL(p, 0, 16)), (unsigned int)(s))
#define aSaveBuffer(pkt, s) A_CMD(pkt, _SHIFTL(A_SAVEBUFF, 24, 8), (unsigned int)(s))
#define aSegment(pkt, s, b) A_CMD(pkt, _SHIFTL(A_SEGMENT, 24, 8), _SHIFTL(s, 24, 8) | _SHIFTL(b, 0, 24))
#define aSetBuffer(pkt, f, i, o, c)                                                                         \
    A_CMD(pkt, (_SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(f, 16, 8) | _SHIFTL(i, 0, 16)),                       \
          _SHIFTL(o, 16, 16) | _SHIFTL(c, 0, 16))
#define aSetVolume(pkt, f, v, t, r)                                                                         \
    A_CMD(pkt, (_SHIFTL(A_SETVOL, 24, 8) | _SHIFTL(f, 16, 16) | _SHIFTL(v, 0, 16)),                       \
          _SHIFTL(t, 16, 16) | _SHIFTL(r, 0, 16))
#define aSetLoop(pkt, a) A_CMD(pkt, _SHIFTL(A_SETLOOP, 24, 8), (unsigned int)(a))
#define aDMEMMove(pkt, i, o, c) A_CMD(pkt, _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(i, 0, 24), _SHIFTL(o, 16, 16) | _SHIFTL(c, 0, 16))
#define aLoadADPCM(pkt, c, d) A_CMD(pkt, _SHIFTL(A_LOADADPCM, 24, 8) | _SHIFTL(c, 0, 24), (unsigned int)d)

#endif
