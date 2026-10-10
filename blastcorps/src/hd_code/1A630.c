#include "common.h"
#include "game/game.h"
#include "game/audio.h"
#include "game/level.h"
#include "game/player.h"
#include "functions.h"

ALMicroTime func_8025F044(void *node);
void func_8025F0F0(SndPlayer *sndp, SndEvent *event);
void func_80260148(ALEventQueue *evtq, SndState *state, u16 eventType);
SndState *func_80260300(SndBank *bank, ALSound *sound);
void func_802604FC(SndState *state);
void func_80260934(u8 arg0);

/*
 * This file's .data.  The sound states' busy and free lists are one struct:
 * func_80260300 stores head and tail through one shared lui, which IDO only
 * does for one object defined in the same file.  D_80366BD0 is .bss.
 */
SndStateLists D_802E8CE0 = { NULL, NULL, NULL };

/* .bss, 0x80366BD0-0x80366C30 (tools/bss_c.py) */
SndPlayer D_80366BD0;
u16 *D_80366C28;

SndPlayer *D_802E8CEC = &D_80366BD0;
s16 D_802E8CF0 = 0;

extern u16 *D_80366C28;

void func_8025EDF0(SndConfig *c) {
    u32 i;
    u8 *ptr;
    SndEvent evt;
    SndState *sState;

    D_802E8CEC->unk48 = c->unk8;
    D_802E8CEC->unk40 = NULL;
    D_802E8CEC->frameTime = 33000;
    ptr = alHeapAlloc(c->heap, 1, c->maxSounds * sizeof(SndState));
    D_802E8CEC->unk44 = (SndState *)ptr;
    ptr = alHeapAlloc(c->heap, 1, c->maxEvents * sizeof(ALEventListItem));
    alEvtqNew(&D_802E8CEC->evtq, (ALEventListItem *)ptr, c->maxEvents);
    D_802E8CE0.freeList = D_802E8CEC->unk44;
    for (i = 1; i < c->maxSounds; i++) {
        sState = D_802E8CEC->unk44;
        alLink((ALLink *)(sState + i), (ALLink *)(sState + i - 1));
    }
    D_80366C28 = alHeapAlloc(c->heap, 2, c->unk10);
    for (i = 0; i < c->unk10; i++) {
        D_80366C28[i] = 0x7FFF;
    }
    D_802E8CEC->drvr = &alGlobals->drvr;
    D_802E8CEC->node.next = NULL;
    D_802E8CEC->node.handler = func_8025F044;
    D_802E8CEC->node.clientData = D_802E8CEC;
    alSynAddPlayer(D_802E8CEC->drvr, &D_802E8CEC->node);
    evt.type = 0x20;
    alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, D_802E8CEC->frameTime);
    D_802E8CEC->nextDelta = alEvtqNextEvent(&D_802E8CEC->evtq, &D_802E8CEC->nextEvent);
}

ALMicroTime func_8025F044(void *node) {
    SndPlayer *sndp = (SndPlayer *)node;
    SndEvent evt;

    do {
        switch (sndp->nextEvent.type) {
            case 0x20:
                evt.type = 0x20;
                alEvtqPostEvent(&sndp->evtq, (ALEvent *)&evt, sndp->frameTime);
                break;
            default:
                func_8025F0F0(sndp, (SndEvent *)&sndp->nextEvent);
                break;
        }
        sndp->nextDelta = alEvtqNextEvent(&sndp->evtq, &sndp->nextEvent);
    } while (sndp->nextDelta == 0);
    sndp->curTime += sndp->nextDelta;
    return sndp->nextDelta;
}

u16 func_80260210(u16 *arg0, u16 *arg1);
void func_8026005C(SndState *state);
void func_802600D8(SndState *state);

#define SND_VOL(vol) ((s16)D_80366C28[keyMap->keyMin & 0x3F] * ((vol) * state->unk34 * snd->sampleVolume / 16129) / 32767)

void func_8025F0F0(SndPlayer *sndp, SndEvent *event) {
    ALVoiceConfig vc;
    ALSound *snd;
    ALKeyMap *keyMap;
    s32 unused;
    u8 pan;
    SndEvent evt;
    SndEvent evt2;
    s32 delta;
    s32 fxmix;
    s32 vol;
    s32 tmp;
    s32 isSpecial;
    s32 isNext;
    s32 done;
    s32 unused2;
    s32 vAlloc;
    SndState *state;
    SndState *next;
    u16 statesFree;
    u16 statesBusy;
    SndState *iter;
    SndEvent evt3;
    SndState *newState;

    unused = 0;
    done = 1;
    vAlloc = 0;
    next = NULL;
    do {
        if (next != NULL) {
            evt2.state = state;
            evt2.type = event->type;
            evt2.param = event->param;
            event = &evt2;
        }
        state = event->state;
        snd = state->sound;
        if (snd == NULL) {
            func_80260210(&statesFree, &statesBusy);
            func_8029A7E4("Bad soundState: voices =%d, states free =%d, states busy =%d, type %d data %x\n",
                          D_802E8CF0, statesFree, statesBusy, event->type, event->param);
            return;
        }
        keyMap = snd->keyMap;
        next = (SndState *)state->node.next;
        switch (event->type) {
            case 0x1:
                if (state->unk3F != 5 && state->unk3F != 4) {
                    return;
                }
                if (state->unk3F == 1) {
                    func_8029A7E4("playing a playing sound\n");
                }
                vc.fxBus = 0;
                vc.priority = state->unk36;
                vc.unityPitch = 0;
                isSpecial = D_802E8CF0 >= sndp->unk48;
                if (!isSpecial || (state->unk3E & 0x50)) {
                    vAlloc = alSynAllocVoice(sndp->drvr, &state->voice, &vc);
                }
                if (!vAlloc) {
                    if ((state->unk3E & 0x52) || state->unk38 > 0) {
                        state->unk3F = 4;
                        state->unk38--;
                        alEvtqPostEvent(&sndp->evtq, (ALEvent *)event, 33333);
                        return;
                    }
                    if (isSpecial) {
                        iter = D_802E8CE0.tail;
                        do {
                            if (!(iter->unk3E & 0x52) && (iter->unk3E & 4) && iter->unk3F != 3) {
                                isSpecial = 0;
                                evt3.type = 0x80;
                                evt3.state = iter;
                                iter->unk3F = 3;
                                alEvtqPostEvent(&sndp->evtq, (ALEvent *)&evt3, 1000);
                                alSynSetVol(sndp->drvr, &iter->voice, 0, 1000);
                            }
                            iter = (SndState *)iter->node.prev;
                        } while (isSpecial && iter != NULL);
                        if (!isSpecial) {
                            state->unk38 = 2;
                            alEvtqPostEvent(&sndp->evtq, (ALEvent *)event, 1001);
                            return;
                        }
                        func_8026005C(state);
                        return;
                    }
                    func_8026005C(state);
                    return;
                }
                state->unk3E |= 4;
                alSynStartVoice(sndp->drvr, &state->voice, snd->wavetable);
                state->unk3F = 1;
                D_802E8CF0++;
                delta = (f32)snd->envelope->attackTime / state->pitch / state->unk28;
                vol = (SND_VOL(snd->envelope->attackVolume) <= 0) ? 0 : SND_VOL(snd->envelope->attackVolume) - 1;
                alSynSetVol(sndp->drvr, &state->voice, 0, 0);
                alSynSetVol(sndp->drvr, &state->voice, vol, delta);
                tmp = state->unk3C + snd->samplePan - 64;
                pan = (((tmp > 0) ? tmp : 0) < 127) ? ((tmp > 0) ? tmp : 0) : 127;
                alSynSetPan(sndp->drvr, &state->voice, pan);
                alSynSetPitch(sndp->drvr, &state->voice, state->pitch * state->unk28);
                fxmix = (state->unk3D + (keyMap->keyMax & 0xF)) * 8;
                fxmix = (((fxmix < 0) ? 0 : fxmix) >= 128) ? 127 : ((fxmix < 0) ? 0 : fxmix);
                alSynSetFXMix(sndp->drvr, &state->voice, fxmix);
                evt.type = 0x40;
                evt.state = state;
                delta = (f32)snd->envelope->attackTime / state->pitch / state->unk28;
                alEvtqPostEvent(&sndp->evtq, (ALEvent *)&evt, delta);
                break;
            case 0x2:
            case 0x400:
            case 0x1000:
                if (event->type != 0x1000 || (state->unk3E & 2)) {
                    switch (state->unk3F) {
                        case 1:
                            func_80260148(&sndp->evtq, state, 0x40);
                            delta = (f32)snd->envelope->releaseTime / state->unk28 / state->pitch;
                            alSynSetVol(sndp->drvr, &state->voice, 0, delta);
                            if (delta) {
                                evt.type = 0x80;
                                evt.state = state;
                                alEvtqPostEvent(&sndp->evtq, (ALEvent *)&evt, delta);
                                state->unk3F = 2;
                            } else {
                                func_8026005C(state);
                            }
                            break;
                        case 4:
                        case 5:
                            func_8026005C(state);
                            break;
                    }
                    if (event->type == 2) {
                        event->type = 0x1000;
                    }
                }
                break;
            case 0x4:
                state->unk3C = event->param;
                if (state->unk3F == 1) {
                    tmp = state->unk3C + snd->samplePan - 64;
                    pan = (((tmp > 0) ? tmp : 0) < 127) ? ((tmp > 0) ? tmp : 0) : 127;
                    alSynSetPan(sndp->drvr, &state->voice, pan);
                }
                break;
            case 0x10:
                state->pitch = *(f32 *)&event->param;
                if (state->unk3F == 1) {
                    alSynSetPitch(sndp->drvr, &state->voice, state->pitch * state->unk28);
                    if (state->unk3E & 0x20) {
                        func_802600D8(state);
                    }
                }
                break;
            case 0x100:
                state->unk3D = event->param;
                if (state->unk3F == 1) {
                    fxmix = (state->unk3D + (keyMap->keyMax & 0xF)) * 8;
                    fxmix = (((fxmix < 0) ? 0 : fxmix) >= 128) ? 127 : ((fxmix < 0) ? 0 : fxmix);
                    alSynSetFXMix(sndp->drvr, &state->voice, fxmix);
                }
                break;
            case 0x8:
                state->unk34 = event->param;
                if (state->unk3F == 1) {
                    vol = (SND_VOL(snd->envelope->decayVolume) <= 0) ? 0 : SND_VOL(snd->envelope->decayVolume) - 1;
                    alSynSetVol(sndp->drvr, &state->voice, vol, 1000);
                }
                break;
            case 0x800:
                if (state->unk3F == 1) {
                    delta = (f32)snd->envelope->releaseTime / state->unk28 / state->pitch;
                    vol = (SND_VOL(snd->envelope->decayVolume) <= 0) ? 0 : SND_VOL(snd->envelope->decayVolume) - 1;
                    alSynSetVol(sndp->drvr, &state->voice, vol, delta);
                }
                break;
            case 0x40:
                if (!(state->unk3E & 2)) {
                    vol = (SND_VOL(snd->envelope->decayVolume) <= 0) ? 0 : SND_VOL(snd->envelope->decayVolume) - 1;
                    delta = (f32)snd->envelope->decayTime / state->unk28 / state->pitch;
                    alSynSetVol(sndp->drvr, &state->voice, vol, delta);
                    evt.type = 2;
                    evt.state = state;
                    alEvtqPostEvent(&sndp->evtq, (ALEvent *)&evt, delta);
                    if (state->unk3E & 0x20) {
                        func_802600D8(state);
                    }
                }
                break;
            case 0x80:
                func_8026005C(state);
                break;
            case 0x200:
                if (state->unk3E & 0x10) {
                    newState = func_80260650(event->unkC, event->param, state->unk30);
                    func_80260AB8(newState, 8, state->unk34);
                    func_80260AB8(newState, 4, state->unk3C);
                    func_80260AB8(newState, 0x100, state->unk3D);
                    func_80260AB8(newState, 0x10, *(s32 *)&state->pitch);
                }
                break;
            default:
                func_8029A7E4("Nonsense sndp event\n");
                break;
        }
        isNext = event->type & 0x2D1;
        state = next;
        if (next != NULL && !isNext) {
            done = next->unk3E & 1;
        }
    } while (!done && state != NULL && !isNext);
}

#undef SND_VOL

void func_8026005C(SndState *state) {
    if (state->unk3E & 4) {
        alSynStopVoice(D_802E8CEC->drvr, &state->voice);
        alSynFreeVoice(D_802E8CEC->drvr, &state->voice);
    }
    func_802604FC(state);
    func_80260148(&D_802E8CEC->evtq, state, 0xFFFF);
}

void func_802600D8(SndState *state) {
    SndEvent evt;
    f32 pitch;

    pitch = alCents2Ratio(state->sound->keyMap->detune) * state->pitch;
    evt.type = 0x10;
    evt.state = state;
    evt.param = *(s32 *)&pitch;
    alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, 0x8235);
}

void func_80260148(ALEventQueue *evtq, SndState *state, u16 eventType) {
    ALLink *thisNode;
    ALLink *nextNode;
    ALEventListItem *thisItem;
    ALEventListItem *nextItem;
    SndEvent *thisEvent;
    OSIntMask mask;

    mask = osSetIntMask(OS_IM_NONE);
    thisNode = evtq->allocList.next;
    while (thisNode != NULL) {
        nextNode = thisNode->next;
        thisItem = (ALEventListItem *)thisNode;
        nextItem = (ALEventListItem *)nextNode;
        thisEvent = (SndEvent *)&thisItem->evt;
        if (thisEvent->state == state && (thisEvent->type & eventType)) {
            if (nextItem != NULL) {
                nextItem->delta += thisItem->delta;
            }
            alUnlink(thisNode);
            alLink(thisNode, &evtq->freeList);
        }
        thisNode = nextNode;
    }
    osSetIntMask(mask);
}

u16 func_80260210(u16 *arg0, u16 *arg1) {
    OSIntMask mask;
    u16 count1;
    u16 count2;
    u16 count3;
    SndState *p1;
    SndState *p2;
    SndState *p3;

    mask = osSetIntMask(OS_IM_NONE);
    count1 = 0;
    p1 = D_802E8CE0.head;
    p2 = D_802E8CE0.freeList;
    p3 = D_802E8CE0.tail;
    if (p1 != NULL) {
        do {
            count1++;
        } while ((p1 = (SndState *)p1->node.next) != NULL);
    }
    count2 = 0;
    if (p2 != NULL) {
        do {
            count2++;
        } while ((p2 = (SndState *)p2->node.next) != NULL);
    }
    count3 = 0;
    if (p3 != NULL) {
        do {
            count3++;
        } while ((p3 = (SndState *)p3->node.prev) != NULL);
    }
    *arg0 = count2;
    *arg1 = count1;
    osSetIntMask(mask);
    return count3;
}

SndState *func_80260300(SndBank *bank, ALSound *sound) {
    SndState *state;
    ALKeyMap *keyMap;
    s32 isSpecial;
    s32 mask;

    keyMap = sound->keyMap;
    state = D_802E8CE0.freeList;
    if (D_802E8CE0.freeList != NULL) {
        mask = osSetIntMask(1);
        D_802E8CE0.freeList = (SndState *)state->node.next;
        alUnlink(&state->node);
        if (D_802E8CE0.head != NULL) {
            state->node.next = (ALLink *)D_802E8CE0.head;
            state->node.prev = NULL;
            D_802E8CE0.head->node.prev = (ALLink *)state;
            D_802E8CE0.head = state;
        } else {
            state->node.next = state->node.prev = NULL;
            D_802E8CE0.tail = D_802E8CE0.head = state;
        }
        isSpecial = sound->envelope->decayTime == -1;
        state->sound = sound;
        state->unk36 = isSpecial + 0x40;
        state->unk3F = 5;
        state->pitch = 1.0f;
        state->unk38 = 2;
        state->unk3E = keyMap->keyMax & 0xF0;
        state->unk30 = NULL;
        if (state->unk3E & 0x20) {
            state->unk28 = alCents2Ratio(keyMap->keyBase * 100 - 6000);
        } else {
            state->unk28 = alCents2Ratio(keyMap->keyBase * 100 + keyMap->detune - 6000);
        }
        if (isSpecial) {
            state->unk3E |= 2;
        }
        state->unk3D = 0;
        state->unk3C = 0x40;
        state->unk34 = 0x7FFF;
        osSetIntMask(mask);
    }
    return state;
}

void func_802604FC(SndState *state) {
    if (D_802E8CE0.head == state) {
        D_802E8CE0.head = (SndState *)state->node.next;
    }
    if (D_802E8CE0.tail == state) {
        D_802E8CE0.tail = (SndState *)state->node.prev;
    }
    alUnlink(&state->node);
    if (D_802E8CE0.freeList != NULL) {
        state->node.next = &D_802E8CE0.freeList->node;
        state->node.prev = NULL;
        D_802E8CE0.freeList->node.prev = &state->node;
        D_802E8CE0.freeList = state;
    } else {
        state->node.next = state->node.prev = NULL;
        D_802E8CE0.freeList = state;
    }
    if (state->unk3E & 4) {
        D_802E8CF0--;
    }
    state->unk3F = 0;
    if (state->unk30 != NULL) {
        if (*state->unk30 == state) {
            *state->unk30 = NULL;
        }
        state->unk30 = NULL;
    }
}

void func_80260618(SndState *state, u8 arg1) {
    if (state != NULL) {
        state->unk36 = (s16)arg1;
    }
}

u8 func_80260634(SndState *state) {
    if (state != NULL) {
        return state->unk3F;
    }
    return 0;
}

SndState *func_80260650(SndBank *bank, s16 id, SndState **handle) {
    SndState *state;
    SndState *result;
    ALKeyMap *keyMap;
    ALSound *sound;
    s16 firstId;
    s32 sp40;
    s32 sp3C;
    s32 sp38;
    SndEvent evt;
    SndEvent evt2;

    result = NULL;
    firstId = 0;
    sp38 = 0;
    if (id == 0 || (D_80358060 == 0 && id == 0xC) ||
        ((id == 0x14 || id == 0x15) && (D_802E8BDC == 0x26 || D_802E8BDC == 0x31))) {
        return NULL;
    }
    do {
        sound = bank->inst->soundArray[id];
        state = func_80260300(bank, sound);
        if (state != NULL) {
            D_802E8CEC->unk40 = state;
            evt.type = 1;
            evt.state = state;
            sp3C = sound->keyMap->velocityMax * 33333;
            if (state->unk3E & 0x10) {
                state->unk3E &= ~0x10;
                alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, sp38 + 1);
                sp40 = sp3C + 1;
                firstId = id;
            } else {
                alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, sp3C + 1);
            }
            result = state;
        } else {
            func_8029A7E4("Sound state allocate failed - sndId %d\n", id);
        }
        sp38 += sp3C;
        keyMap = sound->keyMap;
        id = keyMap->velocityMin + (keyMap->keyMin & 0xC0) * 4;
    } while (id != 0 && state != NULL);
    if (result != NULL) {
        result->unk3E |= 1;
        result->unk30 = handle;
        if (firstId != 0) {
            result->unk3E |= 0x10;
            evt2.type = 0x200;
            evt2.state = result;
            evt2.param = firstId;
            evt2.unkC = bank;
            alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt2, sp40);
        }
    }
    if (handle != NULL) {
        *handle = result;
    }
    return result;
}

void func_802608C8(SndState *state) {
    SndEvent evt;

    evt.type = 0x400;
    evt.state = state;
    if (state != NULL) {
        state->unk3E &= ~0x10;
        alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, 0);
    } else {
        func_8029A7E4("WARNING: Attempt to stop NULL sound aborted\n");
    }
}

void func_80260934(u8 arg0) {
    OSIntMask mask;
    SndEvent evt;
    SndState *state;

    mask = osSetIntMask(OS_IM_NONE);
    state = D_802E8CE0.head;
    while (state != NULL) {
        evt.type = 0x400;
        evt.state = state;
        if ((state->unk3E & arg0) == arg0) {
            state->unk3E &= ~0x10;
            alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, 0);
        }
        state = (SndState *)state->node.next;
    }
    osSetIntMask(mask);
}

void func_802609D0(void) {
    func_80260934(1);
}

void func_802609F0(void) {
    func_80260934(0x11);
}

void func_80260A10(void) {
    func_80260934(3);
}

void func_80260A30(u8 arg0) {
    OSIntMask mask;
    SndState *state;
    s32 i;

    mask = osSetIntMask(OS_IM_NONE);
    i = 0;
    state = D_802E8CE0.head;
    if (state != NULL) {
        do {
            if ((state->sound->keyMap->keyMin & 0x3F) == arg0) {
                func_802608C8(state);
            }
            i++;
        } while ((state = (SndState *)state->node.next) != NULL);
    }
    osSetIntMask(mask);
}

void func_80260AB8(SndState *state, s16 type, s32 param) {
    SndEvent evt;

    evt.type = type;
    evt.state = state;
    evt.param = param;
    if (state != NULL) {
        alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, 0);
    } else {
        func_8029A7E4("WARNING: Attempt to modify NULL sound aborted\n");
    }
}

u16 func_80260B24(u8 arg0) {
    return D_80366C28[arg0];
}

void func_80260B40(u8 arg0, u16 arg1) {
    OSIntMask mask;
    SndState *state;
    s32 i;
    SndEvent evt;

    mask = osSetIntMask(OS_IM_NONE);
    state = D_802E8CE0.head;
    D_80366C28[arg0] = arg1;
    i = 0;
    while (state != NULL) {
        if ((state->sound->keyMap->keyMin & 0x3F) == arg0) {
            evt.type = 0x800;
            evt.state = state;
            alEvtqPostEvent(&D_802E8CEC->evtq, (ALEvent *)&evt, 0);
        }
        i++;
        state = (SndState *)state->node.next;
    }
    osSetIntMask(mask);
}
