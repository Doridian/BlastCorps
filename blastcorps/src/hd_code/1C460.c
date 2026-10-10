#include "common.h"
#include "game/sched.h"
#include "game/game.h"
#include "game/memmap.h"
#include "game/audio.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

/* Saved sequence state: a copy of the player's 16 channel states, the
 * sequence position and tempo, and the song number. */
typedef struct {
    /* 0x000 */ u32 chanState[0x40];
    /* 0x100 */ ALCSeqMarker marker;
    /* 0x1EC */ s32 tempo;
    /* 0x1F0 */ u8 unk1F0;
} UnkStruct_80366C30; /* size = 0x1F4 */


void alCSPSetTempo(ALCSPlayer *, s32);

void func_80260F60(f32 arg0);

/* .bss, 0x80366C30-0x80367B50 (tools/bss_c.py) */
UnkStruct_80366C30 D_80366C30[4];
UnkStruct_80366C30 *D_80367400;
s32 D_80367408[0x42];
u8 *D_80367510;
ALSeqFile *D_80367514;
ALCSeq D_80367518[2];
u8 D_80367708;
f32 D_8036770C;
f32 D_80367710;
f32 D_80367714;
ALHeap D_80367718;
u8 D_80367728;
u8 D_80367729;
u8 D_8036772A;
s32 D_8036772C;
u8 D_80367730;
ALCSPlayer *D_80367734;
SndBank *D_80367738;
ALBank *D_8036773C;
s32 D_80367740;
u8 D_80367744[4];
u8 D_80367748[8];
u8 D_80367750[0x400];

/* .data, 0x802E8D00-0x802E8F30 (tools/data_c.py) */
s16 D_802E8D00[0x42] = {
    26000, 28600, 28600, 28600, 26000, 28600, 26000, 26000, 28600, 22000, 26000, 28600, 26000,
    26000, 26000, 32767, 30000, 26000, 28600, 26000, 28600, 26000, 28600, 28600, 26000, 28600,
    26000, 26000, 32767, 26000, 26000, 26000, 28600, 26000, 32767, 30000, 26000, 26000, 23400,
    26000, 26000, 28600, 32767, 28600, 28600, 30000, 32767, 26000, 28600, 28600, 26000, 26000,
    30000, 30000, 26000, 26000, 26000, 30000, 28600, 28600, 26000, 26000, 28600, 26000, 32767,
    28600,
};
u8 D_802E8D84 = 0;
f32 D_802E8D88 = 1.0f;
u8 D_802E8D8C[0x3c] = {
    8, 1, 5, 11, 17, 2, 41, 16, 18, 8, 11, 31, 8, 6, 6, 6, 42, 5, 8, 42, 31, 31, 16, 16, 16, 7, 11,
    31, 41, 34, 18, 7, 2, 1, 35, 5, 34, 18, 38, 5, 21, 7, 31, 21, 4, 21, 4, 21, 2, 13, 18, 7, 21,
    4, 42, 11, 16, 11, 4, 34,
};
u8 D_802E8DC8[0x3c] = {
    20, 32, 23, 22, 27, 25, 0, 0, 0, 20, 22, 0, 20, 30, 30, 30, 28, 23, 20, 0, 0, 0, 0, 0, 0, 0,
    22, 0, 0, 28, 0, 0, 0, 32, 0, 23, 0, 0, 0, 0, 9, 0, 0, 0, 26, 0, 26, 36, 25, 13, 32, 0, 0, 0,
    0, 0, 0, 22, 26,
};
u8 D_802E8E04[0x3c] = {
    43, 53, 49, 44, 56, 48, 50, 52, 53, 43, 44, 47, 43, 50, 50, 50, 46, 49, 43, 46, 47, 47, 52, 52,
    52, 54, 44, 47, 50, 46, 53, 54, 48, 53, 45, 49, 46, 53, 38, 49, 55, 54, 47, 55, 51, 55, 51, 55,
    48, 13, 53, 54, 55, 51, 46, 44, 52, 44, 51, 46,
};
u8 D_802E8E40[0x3c] = {
    65, 57, 62, 59, 61, 58, 50, 52, 53, 65, 59, 47, 65, 50, 50, 50, 64, 62, 65, 46, 47, 47, 52, 52,
    52, 54, 59, 47, 50, 64, 53, 54, 48, 57, 45, 49, 46, 53, 38, 49, 63, 54, 47, 55, 60, 63, 51, 55,
    58, 13, 57, 54, 55, 51, 46, 44, 52, 59, 60, 46,
};
u16 D_802E8E7C[0x1c] = {
    18, 17, 23, 24, 16, 10, 22, 64, 69, 67, 62, 68, 17, 17, 17, 17, 17, 17, 17, 17, 17, 0, 24, 67,
    67, 67, 67, 67,
};
s32 D_802E8EB4[5][6] = {
    { 103, 217, -1 },
    { 89, 225, 226, 227, 101, -1 },
    { 122, 195, 194, -1 },
    { 93, 224, -1 },
    { 86, 219, -1 },
};

void func_80260C20(u8 arg0, f32 arg1) {
    s32 sp24;

    D_802E8D84 ^= 1;
    func_802D76C0(D_80367734);
    D_8036772A = 0;
    D_8036770C = arg1;
    D_80367708 = arg0;
    sp24 = (s32)D_80367514->seqArray[arg0].offset;
    func_8028B4C4(sp24, D_80367510, &D_80367408[arg0], 0, 0, 0);
    alCSeqNew(&D_80367518[D_802E8D84], D_80367510);
    func_802D81B0(D_80367734, &D_80367518[D_802E8D84]);
    func_802D81F0(D_80367734);
    alCSPSetVol(D_80367734, D_802E8D00[D_80367708] * D_8036770C * D_802E8D88);
}

void func_80260D7C(f32 arg0) {
    D_802E8D88 = arg0;
    alCSPSetVol(D_80367734, D_802E8D00[D_80367708] * D_8036770C * arg0);
}

f32 func_80260DF0(void) {
    return D_802E8D88;
}

void func_80260DFC(void) {
    func_80260EE0(D_802E8DC8[D_802E8BDC]);
}

void func_80260E2C(void) {
    D_80367740 = D_803156C4;
    D_8036772C = alCSPGetTempo(D_80367734);
    func_80261FB0(D_802E8E04[D_802E8BDC]);
}

void func_80260E80(void) {
    D_80367740 = D_803156C4;
    func_80261FB0(D_802E8E40[D_802E8BDC]);
}

void func_80260EC0(void) {
    func_80260DFC();
}

void func_80260EE0(u8 arg0) {
    if (D_80367728 != 0) {
        func_8029A7E4("OH DEAR - pushing tune but we're still popping!\n");
    } else {
        func_8029A7E4("push tune %d\n", arg0);
        D_80367400->unk1F0 = D_80367708;
        D_80367708 = arg0;
        D_80367730 = 0;
        D_80367729 = 1;
    }
}

void func_80260F60(f32 arg0) {
    func_8029A7E4("1 pop tune");
    if (D_80366C30 == D_80367400) {
        return;
    }
    D_80367400--;
    if (D_80366C30 == D_80367400) {
        D_80367730 = 1;
    }
    func_8029A7E4("2 pop tune %d\n", D_80367400->unk1F0);
    D_80367708 = D_80367400->unk1F0;
    func_802D76C0(D_80367734);
    D_80367728 = 2;
    D_80367714 = arg0;
}

void func_8026101C(void) {
    func_80260F60(0.0f);
}

void func_80261040(void) {
    func_80260F60(1.0f);
}

void func_80261068(void) {
    u32 sp114;
    ALCSeqMarker sp28;

    switch (D_80367728) {
        case 2:
            if (func_802D4E10(D_80367734) == 0) {
                func_80260C20(D_80367708, D_80367714);
                D_80367728 = 1;
            }
            break;
        case 1:
            alCSeqGetLoc(&D_80367518[D_802E8D84], &sp28);
            if ((func_802D4E10(D_80367734) == 1) && (sp28.lastTicks != 0)) {
                alCSeqSetLoc(&D_80367518[D_802E8D84], &D_80367400->marker);
                alCSPSetTempo(D_80367734, D_80367400->tempo);
                for (sp114 = 0; sp114 < 0x40; sp114++) {
                    ((u32 *)D_80367734->chanState)[sp114] = D_80367400->chanState[sp114];
                }
                D_80367728 = 0;
                if (D_80367714 != 1.0) {
                    func_80261570(1.0f);
                }
            }
            break;
    }
}

void func_802611F0(void) {
    ALCSeqMarker sp1C;

    alCSeqGetLoc(&D_80367518[D_802E8D84], &sp1C);
    if ((D_80367729 == 0) && (D_80367728 == 0) && (func_802D4E10(D_80367734) == 0) && (sp1C.lastTicks != 0)) {
        func_8029A7E4("auto popping\n");
        func_8026101C();
    }
}

void func_80261284(void) {
    u32 sp24;

    switch (D_80367729) {
        case 1:
            if (func_802D4E10(D_80367734) == 1) {
                alCSeqGetLoc(&D_80367518[D_802E8D84], &D_80367400->marker);
                D_80367400->tempo = alCSPGetTempo(D_80367734);
                for (sp24 = 0; sp24 < 0x40; sp24++) {
                    D_80367400->chanState[sp24] = ((u32 *)D_80367734->chanState)[sp24];
                }
                func_802D76C0(D_80367734);
                D_80367400++;
                D_80367729 = 2;
            }
            break;
        case 2:
            if (func_802D4E10(D_80367734) == 0) {
                func_80260C20(D_80367708, 1.0f);
                D_80367729 = 0;
            }
            break;
    }
}

#define ABS(x) (((x) > 0.0f) ? (x) : -(x))

void func_802613C8(void) {
    f32 sp2C;
    f32 sp28;
    s16 sp26;

    sp2C = alCSPGetVol(D_80367734);
    sp28 = D_802E8D00[D_80367708] * D_802E8D88;
    sp26 = sp2C + (sp28 * D_8036770C - sp2C) * 0.075;
    if (ABS(sp26 - sp28 * D_8036770C) < 10.0f) {
        sp26 = sp28 * D_8036770C;
        D_8036772A = 0;
    }
    alCSPSetVol(D_80367734, sp26);
}

void func_80261528(void) {
    if (D_80367734->state == 1) {
        alCSPSetTempo(D_80367734, D_8036772C);
        D_8036772C = 0;
    }
}

void func_80261570(f32 arg0) {
    D_8036770C = arg0;
    D_8036772A = 1;
}

/* ROM addresses of the two sound banks' .ctl/.tbl and the sequence file. */
extern u8 D_00350950[];
extern u8 D_003539A0[];
extern u8 D_003A1920[];
extern u8 D_003A48C0[];
extern u8 D_0044F5C0[];
extern ALHeap D_80367718;
extern ALBank *D_8036773C;
extern u8 D_80370C80[];


void func_80261588(void) {
    SndConfig sndConfig;
    ALSeqpConfig seqConfig;
    SynConfig synConfig;
    ALBankFile *sfxBankFile;
    ALBankFile *musicBankFile;
    s32 size;
    s32 size2;
    u32 i;
    s32 unused[2];
    s32 seqFileSize;
    s32 headerSize;

    alHeapInit(&D_80367718, D_80370C80, AUDIO_HEAP_SIZE);
    size = size2 = ROM(D_003539A0) - ROM(D_00350950);
    func_8028B4C4(ROM(D_00350950), (u8 *)MEM_POOL, &size, 0xD, 0, 2);
    musicBankFile = alHeapAlloc(&D_80367718, 1, size);
    func_8028B4C4(ROM(D_00350950), (u8 *)musicBankFile, &size2, 0xD, 0, 2);
    alBnkfNew(musicBankFile, D_003539A0);
    D_8036773C = musicBankFile->bankArray[0];
    size = size2 = ROM(D_003A48C0) - ROM(D_003A1920);
    func_8028B4C4(ROM(D_003A1920), (u8 *)MEM_POOL, &size, 0xD, 0, 2);
    sfxBankFile = alHeapAlloc(&D_80367718, 1, size);
    func_8028B4C4(ROM(D_003A1920), (u8 *)sfxBankFile, &size2, 0xD, 0, 2);
    alBnkfNew(sfxBankFile, D_003A48C0);
    D_80367738 = (SndBank *)sfxBankFile->bankArray[0];
    D_80367514 = alHeapAlloc(&D_80367718, 1, 4);
    headerSize = 4;
    func_8028B4C4(ROM(D_0044F5C0), (u8 *)D_80367514, &headerSize, 0, 0, 0);
    seqFileSize = D_80367514->seqCount * 8 + 4;
    D_80367514 = alHeapAlloc(&D_80367718, 1, 0x214);
    func_8028B4C4(ROM(D_0044F5C0), (u8 *)D_80367514, &seqFileSize, 0, 0, 0);
    alSeqFileNew(D_80367514, D_0044F5C0);
    D_80367510 = alHeapAlloc(&D_80367718, 1, 0x21AE);
    for (i = 0; i < 0x42; i++) {
        D_80367408[i] = D_80367514->seqArray[i].len;
        if (D_80367408[i] & 1) {
            D_80367408[i]++;
        }
    }
    synConfig.maxVVoices = 0;
    synConfig.maxPVoices = 0x18;
    synConfig.maxUpdates = 0x80;
    synConfig.maxFXbusses = 1;
    synConfig.dmaproc = NULL;
    synConfig.fxType = 6;
    synConfig.outputRate = 0;
    synConfig.heap = &D_80367718;
    func_802676A0(&synConfig, 0xC);
    seqConfig.maxVoices = 0x18;
    seqConfig.maxEvents = 0x20;
    seqConfig.maxChannels = 0x10;
    seqConfig.heap = &D_80367718;
    seqConfig.initOsc = NULL;
    seqConfig.updateOsc = NULL;
    seqConfig.stopOsc = NULL;
    D_80367734 = alHeapAlloc(&D_80367718, 1, sizeof(ALCSPlayer));
    alCSPNew(D_80367734, &seqConfig);
    func_802D97E0(D_80367734, D_8036773C);
    sndConfig.maxEvents = 0x40;
    sndConfig.maxSounds = 0x20;
    sndConfig.unk8 = 8;
    sndConfig.unk10 = 8;
    sndConfig.heap = &D_80367718;
    func_8025EDF0(&sndConfig);
    func_8029A7E4("%d bytes audio heap left over\n", D_80367718.len - (D_80367718.cur - D_80367718.base));
    D_8036772A = 0;
    D_80367728 = 0;
    D_80367710 = D_8036770C = 1.0f;
    D_80367730 = 1;
    D_80367400 = D_80366C30;
    func_80267A74();
}

void func_802619D0(u32 arg0) {
    if (arg0 >= 0x1C) {
        func_8029A7E4("effect id %d out of range!\n", arg0);
    } else if (D_802E8E7C[arg0] != 0) {
        func_80260650(D_80367738, D_802E8E7C[arg0], 0);
    }
}

u8 func_80261A44(u64 arg0) {
    u8 sp27;
    u8 sp26;

    sp26 = 0;
    sp27 = D_80367708;
    if (arg0 != 4) {
        func_80260A10();
    }
    func_802609F0();
    func_80261E9C(arg0);
    if (D_80364A90 & 0xC9FD8FE7FBFFC0B0) {
        func_80260A30(0);
        func_80260A30(5);
    }
    D_80367710 = 1.0f;
    switch (arg0) {
        case 0x2:
            sp27 = 0x11;
            break;
        case 0x40:
            sp27 = 0xA;
            break;
        case 0x800:
        case 0x1000:
            sp27 = 3;
            break;
        case 0x10000:
#ifndef VERSION_EU
            sp27 = 0x21;
            break;
#endif
        case 0x20000:
        case 0x40000:
        case 0x100000000:
        case 0x200000000:
        case 0x40000000000000:
            sp27 = 0x21;
            break;
        case 0x80000000:
            func_80261570(0.0f);
            break;
        case 0x8000000:
            sp27 = 0xE;
            break;
        case 0x80:
        case 0x4000:
            if (func_80264BA4(D_802E8BDC) == 3) {
                sp27 = 0xC;
            } else {
                sp27 = 0x13;
            }
            break;
        case 0x40000000:
            sp27 = 0xF;
            break;
        case 0x2000:
            if (D_802E8F94[D_802E8BDC].unk0 != 1) {
                func_80261570(0.0f);
                break;
            }
            /* fallthrough */
        case 0x4:
            sp27 = D_802E8D8C[D_802E8BDC];
            sp26 = 1;
            break;
        case 0x20000000:
            sp27 = 0x1D;
            break;
        case 0x100000000000:
            sp27 = D_802E8D8C[D_802E8BDC];
            sp26 = 1;
            if (D_802E8BDC == 0x26) {
                D_80367710 = 0.7f;
            }
            break;
        case 0x4000000000000:
            func_80261570(0.0f);
            break;
        case 0x10000000000000:
            sp27 = 0x28;
            break;
        case 0x800000000000:
            func_80261570(0.0f);
            break;
        case 0x4000000000000000:
            break;
    }
    if ((sp27 != D_80367708) || (sp26 != 0)) {
        func_80261570(0.0f);
        return sp27;
    }
    return 0;
}

void func_80261E9C(u64 arg0) {
    switch (arg0) {
        case 0x2:
            func_80260B40(0, 0x5DC0);
            func_80260B40(5, 0x5DC0);
            break;
        case 0x20000000:
            func_80260B40(0, 0x61A8);
            break;
        case 0x40:
            func_80260B40(0, 0x4E20);
            func_80260B40(5, 0x4E20);
            break;
        case 0x800:
        case 0x1000:
            func_80260B40(0, 0x6D60);
            func_80260B40(5, 0x6D60);
            break;
        default:
            func_80260B40(0, 0x7FFF);
            func_80260B40(5, 0x7FFF);
            break;
    }
}

void func_80261FB0(u8 arg0) {
    D_80367728 = 0;
    D_80367729 = 0;
    D_80367400 = D_80366C30;
    D_80367730 = 1;
    func_80260C20(arg0, D_80367710);
}

void func_80262008(u8 arg0, f32 arg1) {
    D_80367400 = D_80366C30;
    D_80367730 = 1;
    func_80260C20(arg0, arg1);
}

u8 func_80262050(void) {
    return D_80367708;
}

s32 func_8026205C(s32 arg0) {
    u32 sp2C;
    s32 sp28;

    sp2C = 0;
    for (sp28 = 0; sp28 < 5 && D_802E8EB4[arg0][sp28] != -1; sp28++, sp2C++) {
    }
    return D_802E8EB4[arg0][osGetCount() % sp2C];
}
