/*
 * PORT_DIGEST=FILE: the gameplay digest (docs/PORT.md, "The gameplay
 * digest"): at every controller poll, what a player sees or what decides a
 * level's outcome, read from the game's variables.  port/tools/digest_cmp.py
 * compares two of them, aligned by the game's mode, level and frame rather
 * than by retrace, so that runs whose timing differs (the movie's lag
 * frames, a build's own pace) can still be compared field by field; the
 * clock's fields are reported as differences in time, not as failures.
 *
 * It only reads, at the poll (host_controller_poll, main.c), which the game
 * reaches once per frame: nothing here touches game memory, so it changes
 * neither the game nor any other output.
 *
 * One line per poll:
 *
 *   POLL MODE LEVEL FRAME [*] key=value ...
 *
 * MODE is D_80364A90 (game.h) in hex, LEVEL D_802E8BDC, FRAME D_80358064
 * (the game's frame count, from 0 in each mode).  The fields are written
 * when they change: a line marked * has them all (the first poll of a mode
 * or a level, and the first one), and the state at any line is the last
 * full line's with the changes after it applied ("key=~": gone).  The
 * fields (the headers in blastcorps/include/game and port/engine say what
 * each variable is):
 *
 *   always:
 *     rng     D_8036B968, the random number generator's state (internal)
 *     clk     D_803156C4, the scheduler's retraces (the clock)
 *     med     PlayerInfo.medal[60] of the current player (D_80364AF0[D_80364AE8]), hex
 *     units   its units, gs its gameState
 *   in the modes that run the level's frame (func_802475D8: not the front
 *   end's, the map's, the logos' or the results'):
 *     t       D_803156C0, "TIME IN LEVEL"'s retraces (the clock)
 *     fc      D_80358060, a frame count (internal: TNT fuses read it)
 *     pv      D_80364456, the player's vehicle type (vehicle.h)
 *     p       D_803643E0..E8, the player's position (<< 5)
 *     won     D_803643DA (the carrier got through, or the level's goal), lost D_803643D9
 *     st      D_8036EA70 (LevelStats): ip (damage $), bd, cr, rt, coin, as ip,bd,cr,rt,coin
 *     tc      D_8036EA70.tc, the level's time (the clock)
 *     of      the totals: D_8036EB92 buildings, D_8036EB93, D_8036EB90 RDUs
 *     dmg     D_803649F0, the damage so far
 *     ammo    D_803F8B72 (the Ballista's missiles), D_803EDC00 (the Sideswipe's hydraulics)
 *     vT      for each vehicle module the level's vehicles (D_80364460 to
 *             D_803649D0) use, type T in hex: its position and its
 *             VehicleState's headings (unk4C, unk4E) and speed (unk76),
 *             x,y,z,h,h2,speed; the barges' three as vB.0 .. vB.2; vFF is
 *             the missile carrier
 *     bN      building N (D_803F4030 to D_803F7654): destroyed (unkEA),
 *             its position and its groups' damage (0..100 each, hex),
 *             gone/x,y,z/damage
 *     rdu     the RDUs collected (Rdu.collected of D_8036BED8[D_8036EB90]), a bit each, hex
 *     tnt     the TNT crates (D_8039B070[D_8039B610]): active, x,y,z, timer each
 *     blk     the blocks (D_8039C550[D_8039C710]): x,y,z and in its hole, each
 *     box     the ammo boxes collected (D_8039AF00[D_8039B068]), a bit each
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"
#include "host.h"
#include "host_game_vars.h"

/* where the game's structures have what is read here: the N64 side's numbers
   (port.h's host_layout, port/src/layout.c), the game's own layouts */
#define L(x) host_layout(PORT_LAYOUT_##x)
#define S32(base, field) ((int32_t)GAME_FIELD(uint32_t, base, field))
#define U32(base, field) GAME_FIELD(uint32_t, base, field)
#define U16(base, field) GAME_FIELD(uint16_t, base, field)
#define S16(base, field) ((int16_t)GAME_FIELD(uint16_t, base, field))
#define U8(base, field) GAME_FIELD(uint8_t, base, field)
#define FRONT_END_MODES (0xC9FD8FE7DBFF8080ull | 0x4000 | 0x20000000 | 0x30)   /* hd.c's loop */

/* ---- the line's fields, and what the last line had ------------------------ */

#define MAXF 512
#define KEYLEN 16
#define VALLEN 1024
typedef struct { char key[KEYLEN]; char val[VALLEN]; } Field;
static Field cur[MAXF], prev[MAXF];
static int ncur, nprev;

static void put(const char *key, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void put(const char *key, const char *fmt, ...) {
    va_list ap;
    if (ncur >= MAXF)
        return;
    Field *f = &cur[ncur++];
    snprintf(f->key, KEYLEN, "%s", key);
    va_start(ap, fmt);
    vsnprintf(f->val, VALLEN, fmt, ap);
    va_end(ap);
}

static FILE *out;

static void finish(void) {
    if (out)
        fclose(out);
    out = NULL;
}

/* one vehicle module's state: x,y,z,heading,heading2,speed */
static void put_vehicle3(const char *key, const uint8_t *vs, const int32_t *pos, const int32_t *py, const int32_t *pz) {
    put(key, "%d,%d,%d,%u,%u,%d", *pos, *py, *pz, U16(vs, L(VSTATE_HEADING)), U16(vs, L(VSTATE_HEADING2)),
        S16(vs, L(VSTATE_SPEED)));
}

static void vehicles(void) {
    /* the types, and their modules' states and positions (vehicle.h's table) */
    static const uint8_t types[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                    0x09, 0x0A, 0x0D, 0x0E, 0x0F, 0x10, 0xFE, 0xFF};
    static const struct { const uint8_t *vs; const int32_t *x, *y, *z; } tab[] = {
#define VEH(vs, x, y, z) { vs, (const int32_t *)&x, (const int32_t *)&y, (const int32_t *)&z }
        VEH(D_803ED760, D_803ED808, D_803ED80C, D_803ED810), VEH(D_803EDB40, D_803EDBE8, D_803EDBEC, D_803EDBF0),
        VEH(D_803EDF10, D_803EDFB8, D_803EDFBC, D_803EDFC0), VEH(D_803EE2E0, D_803EE38C, D_803EE390, D_803EE394),
        VEH(D_803EE6C0, D_803EE768, D_803EE76C, D_803EE770), VEH(D_803EEA90, D_803EEB38, D_803EEB3C, D_803EEB40),
        VEH(D_803EFA20, D_803EFAC8, D_803EFACC, D_803EFAD0), VEH(D_803EFDF0, D_803EFE98, D_803EFE9C, D_803EFEA0),
        VEH(D_803EEE70, D_803EEF18, D_803EEF1C, D_803EEF20), VEH(D_803F7B50, D_803F7BF8, D_803F7BFC, D_803F7C00),
        VEH(D_803F8AA0, D_803F8B48, D_803F8B4C, D_803F8B50), VEH(D_803F8E80, D_803F8F28, D_803F8F2C, D_803F8F30),
        VEH(D_803F9250, D_803F92F8, D_803F92FC, D_803F9300), VEH(D_803FC500, D_803FC5A8, D_803FC5AC, D_803FC5B0),
        VEH(D_803FC8D0, D_803FC978, D_803FC97C, D_803FC980), VEH(D_803FCCA0, D_803FCD48, D_803FCD4C, D_803FCD50),
        VEH(D_803EF630, D_803EF6DC, D_803EF6E0, D_803EF6E4),
#undef VEH
    };
    enum { N = sizeof types };
    int seen[N] = {0}, barge = 0;
    unsigned i;

    uint32_t size = L(VEHICLE_SIZE);
    const uint8_t *first = D_80364460, *end = D_803649D0;
    if (end < first || end > first + 12 * size)
        end = first;
    for (const uint8_t *a = first; a < end; a += size) {
        uint32_t type = U32(a, L(VEHICLE_TYPE));
        if (type == 0x0B || type == 0x11 || type == 0x12) {
            barge = 1;
            continue;
        }
        for (i = 0; i < N; i++)
            if (types[i] == type)
                seen[i] = 1;
    }
    for (i = 0; i < N; i++)
        if (seen[i]) {
            char key[8];
            snprintf(key, sizeof key, "v%X", types[i]);
            put_vehicle3(key, tab[i].vs, tab[i].x, tab[i].y, tab[i].z);
        }
    if (barge)
        for (i = 0; i < 3; i++) {
            char key[8];
            snprintf(key, sizeof key, "vB.%u", i);
            put_vehicle3(key, D_803F8550 + L(VSTATE_SIZE) * i, &D_803F8748[3 * i], &D_803F8748[3 * i + 1],
                         &D_803F8748[3 * i + 2]);
        }
}

static void buildings(void) {
    uint32_t size = L(BUILDING_SIZE);
    const uint8_t *first = D_803F4030, *end = D_803F7654;
    if (end < first || end > first + 0x100 * size)
        end = first;
    uint32_t k = 0;
    for (const uint8_t *b = first; b < end; b += size, k++) {
        char key[8], dmg[2 * 0x10 + 1];
        unsigned groups = U8(b, L(BUILDING_GROUPS)), j;
        if (groups > 0x10)
            groups = 0x10;
        for (j = 0; j < groups; j++)
            snprintf(dmg + 2 * j, 3, "%02X", U8(b, L(BUILDING_DAMAGE) + j));
        dmg[2 * j] = 0;
        snprintf(key, sizeof key, "b%u", k);
        put(key, "%u/%d,%d,%d/%s", U8(b, L(BUILDING_GONE)), S32(b, L(BUILDING_X)), S32(b, L(BUILDING_Y)),
            S32(b, L(BUILDING_Z)), dmg);
    }
}

/* the bits of n records' byte at `off`, as hex */
static void bits(const char *key, const uint8_t *base, uint32_t n, uint32_t stride, uint32_t off) {
    char s[VALLEN];
    unsigned len = 0, nib = 0, i;
    if (n > 4 * (VALLEN - 2))
        n = 4 * (VALLEN - 2);
    for (i = 0; i < n; i++) {
        nib |= (U8(base, i * stride + off) != 0) << (i & 3);
        if ((i & 3) == 3 || i == n - 1) {
            s[len++] = "0123456789ABCDEF"[nib];
            nib = 0;
        }
    }
    s[len] = 0;
    put(key, "%u:%s", n, s);
}

static void level(void) {
    const uint8_t *st = D_8036EA70;
    put("t", "%u", SCHED_LEVEL_TIME);
    put("fc", "%u", D_80358060);
    put("pv", "%X", D_80364456);
    put("p", "%d,%d,%d", D_803643E0, D_803643E4, D_803643E8);
    put("won", "%u", D_803643DA);
    put("lost", "%u", D_803643D9);
    put("st", "%u,%u,%u,%u,%u", U32(st, L(STATS_IP)), U8(st, L(STATS_BD)), U8(st, L(STATS_CR)),
        U16(st, L(STATS_RT)), U8(st, L(STATS_COIN)));
    put("tc", "%u", U32(st, L(STATS_TC)));
    put("of", "%u,%u,%u", D_8036EB92, D_8036EB93, D_8036EB90);
    put("dmg", "%u", D_803649F0);
    put("ammo", "%d,%d", D_803F8B72, D_803EDC00);
    vehicles();
    buildings();
    uint32_t n = D_8036EB90;
    const uint8_t *rdus = D_8036BED8;
    if (rdus && n)
        bits("rdu", rdus, n, L(RDU_SIZE), L(RDU_COLLECTED));
    n = (uint32_t)D_8039B610;
    if (n <= 20) {
        char s[VALLEN];
        unsigned len = 0;
        s[0] = 0;
        for (uint32_t i = 0; i < n; i++) {
            const uint8_t *c = D_8039B070 + L(TNT_SIZE) * i;
            len += snprintf(s + len, sizeof s - len, "%s%u,%d,%d,%d,%d", i ? "/" : "", U8(c, L(TNT_ACTIVE)),
                            S32(c, L(TNT_X)), S32(c, L(TNT_Y)), S32(c, L(TNT_Z)), S16(c, L(TNT_TIMER)));
        }
        if (n)
            put("tnt", "%s", s);
    }
    n = (uint32_t)D_8039C710;
    if (n <= 8) {
        char s[VALLEN];
        unsigned len = 0;
        s[0] = 0;
        for (uint32_t i = 0; i < n; i++) {
            const uint8_t *k = D_8039C550 + L(BLOCK_SIZE) * i;
            len += snprintf(s + len, sizeof s - len, "%s%d,%d,%d,%u", i ? "/" : "", S32(k, L(BLOCK_X)),
                            S32(k, L(BLOCK_Y)), S32(k, L(BLOCK_Z)), U8(k, L(BLOCK_IN_HOLE)));
        }
        if (n)
            put("blk", "%s", s);
    }
    n = (uint32_t)D_8039B068;
    if (n && n <= 15)
        bits("box", D_8039AF00, n, L(AMMOBOX_SIZE), L(AMMOBOX_COLLECTED));
}

void host_digest_poll(unsigned poll) {
    static int init;
    static uint64_t last_mode = ~0ull;
    static int32_t last_level = -1;
    if (!init) {
        const char *p = getenv("PORT_DIGEST");
        init = 1;
        if (p && *p && !(out = fopen(p, "w")))
            host_log("digest: can't write %s\n", p);
        if (out) {
            fprintf(out, "# blastcorps gameplay digest 1: poll mode level frame [*] key=value...\n");
            atexit(finish);
        }
    }
    if (!out)
        return;
    uint64_t mode = D_80364A90;
    int32_t lvl = D_802E8BDC;
    uint32_t frame = (uint32_t)D_80358064;

    ncur = 0;
    put("rng", "%08X", D_8036B968);
    put("clk", "%u", SCHED_FRAMECOUNT);
    {
        uint8_t who = D_80364AE8;
        const HostPlayerInfo *pl = &D_80364AF0[who & 3];
        char med[2 * 60 + 1];
        for (int i = 0; i < 60; i++)
            snprintf(med + 2 * i, 3, "%02X", pl->medal[i]);
        put("med", "%s", med);
        put("units", "%u", pl->units);
        put("gs", "%u", pl->gameState);
    }
    if (mode && !(mode & FRONT_END_MODES))
        level();

    int full = mode != last_mode || lvl != last_level;
    last_mode = mode, last_level = lvl;
    fprintf(out, "%u %llX %d %u%s", poll, (unsigned long long)mode, lvl, frame, full ? " *" : "");
    for (int i = 0; i < ncur; i++) {
        const Field *f = &cur[i];
        const Field *was = NULL;
        if (!full) {
            if (i < nprev && !strcmp(prev[i].key, f->key))
                was = &prev[i];
            else
                for (int j = 0; j < nprev; j++)
                    if (!strcmp(prev[j].key, f->key)) {
                        was = &prev[j];
                        break;
                    }
        }
        if (!was || strcmp(was->val, f->val))
            fprintf(out, " %s=%s", f->key, f->val);
    }
    if (!full)      /* what's gone */
        for (int j = 0; j < nprev; j++) {
            int k;
            for (k = 0; k < ncur; k++)
                if (!strcmp(prev[j].key, cur[k].key))
                    break;
            if (k == ncur)
                fprintf(out, " %s=~", prev[j].key);
        }
    fputc('\n', out);
#ifdef __EMSCRIPTEN__
    fflush(out);    /* node doesn't run atexit's finish(): the last lines were lost */
#endif
    memcpy(prev, cur, sizeof *cur * ncur);
    nprev = ncur;
}
