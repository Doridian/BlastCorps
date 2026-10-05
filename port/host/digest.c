/*
 * PORT_DIGEST=FILE: the gameplay digest (docs/PORT.md, "The gameplay
 * digest"): at every controller poll, what a player sees or what decides a
 * level's outcome, read from the game's variables.  port/tools/digest_cmp.py
 * compares two of them, aligned by the game's mode, level and frame rather
 * than by retrace, so that runs whose timing differs (lag frames, the CPU
 * model) can still be compared field by field; the clock's fields are
 * reported as differences in time, not as failures.
 *
 * It only reads, at the poll (host_controller_poll, main.c), which the game
 * reaches once per frame: nothing here charges the CPU model or touches
 * game memory, so it changes neither the game nor any other output.
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

/* (the native-endian build keeps scalars in host order; the ROM file and
   what the LP64 build moved past RDRAM are read as main.c does) */
static uint32_t g32(const void *p) {
#ifdef PORT_NATIVE_ENDIAN
    return port_var32(p);
#else
    return port_be32(p);
#endif
}
static uint16_t g16(const void *p) {
#ifdef PORT_NATIVE_ENDIAN
    return port_in_rdram(p) ? port_g16(p) : port_be16(p);
#else
    return port_be16(p);
#endif
}
static uint8_t g8(const void *p) { return *(const uint8_t *)p; }
/* a pointer the game's C declares without PTR32 (8 bytes in the LP64
   build): the N64 address it holds (port_game_ptr) */
static uint32_t gptr(const void *p) {
#ifdef PORT_LP64
    return port_game_ptr(p);
#else
    return g32(p);
#endif
}

#define VAR(sym) PORT_VAR(sym)
extern char D_80364A90[], D_802E8BDC[], D_80358064[], D_80358060[], D_8036B968[], D_803156C4[], D_803156C0[];
extern char D_80364AF0[], D_80364AE8[];
extern char D_80364456[], D_803643E0[], D_803643D9[], D_803643DA[];
extern char D_8036EA70[], D_8036EB90[], D_8036EB92[], D_8036EB93[], D_803649F0[];
extern char D_803F8B72[], D_803EDC00[];
extern char D_80364460[], D_803649D0[];
extern char D_803F4030[], D_803F7654[];
extern char D_8036BED8[], D_8039B070[], D_8039B610[], D_8039C550[], D_8039C710[], D_8039AF00[], D_8039B068[];
/* the vehicle modules' states and positions (vehicle.h's table) */
extern char D_803ED760[], D_803ED808[], D_803EDB40[], D_803EDBE8[], D_803EDF10[], D_803EDFB8[];
extern char D_803EE2E0[], D_803EE38C[], D_803EE6C0[], D_803EE768[], D_803EEA90[], D_803EEB38[];
extern char D_803EEE70[], D_803EEF18[], D_803EF630[], D_803EF6DC[], D_803EFA20[], D_803EFAC8[];
extern char D_803EFDF0[], D_803EFE98[], D_803F7B50[], D_803F7BF8[], D_803F8550[], D_803F8748[];
extern char D_803F8AA0[], D_803F8B48[], D_803F8E80[], D_803F8F28[], D_803F9250[], D_803F92F8[];
extern char D_803FC500[], D_803FC5A8[], D_803FC8D0[], D_803FC978[], D_803FCCA0[], D_803FCD48[];

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
static void put_vehicle(const char *key, const char *vs, const char *pos) {
    put(key, "%d,%d,%d,%u,%u,%d", (int32_t)g32(pos), (int32_t)g32(pos + 4), (int32_t)g32(pos + 8),
        g16(vs + 0x4C), g16(vs + 0x4E), (int16_t)g16(vs + 0x76));
}

static void vehicles(void) {
    /* the types, and their modules' states and positions (vehicle.h's
       table; the addresses are the run's: PORT_VAR in the movable builds) */
    static const uint8_t types[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                    0x09, 0x0A, 0x0D, 0x0E, 0x0F, 0x10, 0xFE, 0xFF};
    const char *tab[][2] = {
        {VAR(D_803ED760), VAR(D_803ED808)}, {VAR(D_803EDB40), VAR(D_803EDBE8)},
        {VAR(D_803EDF10), VAR(D_803EDFB8)}, {VAR(D_803EE2E0), VAR(D_803EE38C)},
        {VAR(D_803EE6C0), VAR(D_803EE768)}, {VAR(D_803EEA90), VAR(D_803EEB38)},
        {VAR(D_803EFA20), VAR(D_803EFAC8)}, {VAR(D_803EFDF0), VAR(D_803EFE98)},
        {VAR(D_803EEE70), VAR(D_803EEF18)}, {VAR(D_803F7B50), VAR(D_803F7BF8)},
        {VAR(D_803F8AA0), VAR(D_803F8B48)}, {VAR(D_803F8E80), VAR(D_803F8F28)},
        {VAR(D_803F9250), VAR(D_803F92F8)}, {VAR(D_803FC500), VAR(D_803FC5A8)},
        {VAR(D_803FC8D0), VAR(D_803FC978)}, {VAR(D_803FCCA0), VAR(D_803FCD48)},
        {VAR(D_803EF630), VAR(D_803EF6DC)},
    };
    enum { N = sizeof types };
    int seen[N] = {0}, barge = 0;
    unsigned i;

    uint32_t first = PORT_ADDR(D_80364460), end = gptr(VAR(D_803649D0));
    if (end < first || end > first + 12 * 0x74)
        end = first;
    for (uint32_t a = first; a < end; a += 0x74) {
        uint32_t type = g32((const char *)port_ptr(a) + 0x5C);
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
            put_vehicle(key, tab[i][0], tab[i][1]);
        }
    if (barge)
        for (i = 0; i < 3; i++) {
            char key[8];
            snprintf(key, sizeof key, "vB.%u", i);
            put_vehicle(key, VAR(D_803F8550) + 0xA8 * i, VAR(D_803F8748) + 12 * i);
        }
}

static void buildings(void) {
    uint32_t first = PORT_ADDR(D_803F4030), end = g32(VAR(D_803F7654));
    if (end < first || end > first + 0x100 * 0xFC)
        end = first;
    for (uint32_t a = first, k = 0; a < end; a += 0xFC, k++) {
        const char *b = (const char *)port_ptr(a);
        char key[8], dmg[2 * 0x10 + 1];
        unsigned groups = g8(b + 0xE9), j;
        if (groups > 0x10)
            groups = 0x10;
        for (j = 0; j < groups; j++)
            snprintf(dmg + 2 * j, 3, "%02X", g8(b + 0xEC + j));
        dmg[2 * j] = 0;
        snprintf(key, sizeof key, "b%u", k);
        put(key, "%u/%d,%d,%d/%s", g8(b + 0xEA), (int32_t)g32(b + 0x10), (int32_t)g32(b + 0x14),
            (int32_t)g32(b + 0x18), dmg);
    }
}

/* the bits of n records' byte at `off`, as hex */
static void bits(const char *key, const char *base, uint32_t n, uint32_t stride, uint32_t off) {
    char s[VALLEN];
    unsigned len = 0, nib = 0, i;
    if (n > 4 * (VALLEN - 2))
        n = 4 * (VALLEN - 2);
    for (i = 0; i < n; i++) {
        nib |= (g8(base + i * stride + off) != 0) << (i & 3);
        if ((i & 3) == 3 || i == n - 1) {
            s[len++] = "0123456789ABCDEF"[nib];
            nib = 0;
        }
    }
    s[len] = 0;
    put(key, "%u:%s", n, s);
}

static void level(void) {
    const char *st = VAR(D_8036EA70);
    put("t", "%u", g32(VAR(D_803156C0)));
    put("fc", "%u", g32(VAR(D_80358060)));
    put("pv", "%X", g8(VAR(D_80364456)));
    put("p", "%d,%d,%d", (int32_t)g32(VAR(D_803643E0)), (int32_t)g32(VAR(D_803643E0) + 4),
        (int32_t)g32(VAR(D_803643E0) + 8));
    put("won", "%u", g8(VAR(D_803643DA)));
    put("lost", "%u", g8(VAR(D_803643D9)));
    put("st", "%u,%u,%u,%u,%u", g32(st), g8(st + 8), g8(st + 9), g16(st + 0xC), g8(st + 0xA));
    put("tc", "%u", g32(st + 4));
    put("of", "%u,%u,%u", g8(VAR(D_8036EB92)), g8(VAR(D_8036EB93)), g16(VAR(D_8036EB90)));
    put("dmg", "%u", g32(VAR(D_803649F0)));
    put("ammo", "%d,%d", (int16_t)g16(VAR(D_803F8B72)), (int16_t)g16(VAR(D_803EDC00)));
    vehicles();
    buildings();
    uint32_t n = g16(VAR(D_8036EB90)), rdus = gptr(VAR(D_8036BED8));
    if (rdus && n)
        bits("rdu", (const char *)port_ptr(rdus), n, 0x88, 6);
    n = g32(VAR(D_8039B610));
    if (n <= 20) {
        char s[VALLEN];
        unsigned len = 0;
        s[0] = 0;
        for (uint32_t i = 0; i < n; i++) {
            const char *c = VAR(D_8039B070) + 0x48 * i;
            len += snprintf(s + len, sizeof s - len, "%s%u,%d,%d,%d,%d", i ? "/" : "", g8(c + 0x18),
                            (int32_t)g32(c), (int32_t)g32(c + 4), (int32_t)g32(c + 8), (int16_t)g16(c + 0x10));
        }
        if (n)
            put("tnt", "%s", s);
    }
    n = g32(VAR(D_8039C710));
    if (n <= 8) {
        char s[VALLEN];
        unsigned len = 0;
        s[0] = 0;
        for (uint32_t i = 0; i < n; i++) {
            const char *k = VAR(D_8039C550) + 0x38 * i;
            len += snprintf(s + len, sizeof s - len, "%s%d,%d,%d,%u", i ? "/" : "", (int32_t)g32(k),
                            (int32_t)g32(k + 4), (int32_t)g32(k + 8), g8(k + 0x11));
        }
        if (n)
            put("blk", "%s", s);
    }
    n = g32(VAR(D_8039B068));
    if (n && n <= 15)
        bits("box", VAR(D_8039AF00), n, 0x18, 7);
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
    uint64_t mode = (uint64_t)g32(VAR(D_80364A90)) << 32 | g32(VAR(D_80364A90) + 4);
    int32_t lvl = (int32_t)g32(VAR(D_802E8BDC));
    uint32_t frame = g32(VAR(D_80358064));

    ncur = 0;
    put("rng", "%08X", g32(VAR(D_8036B968)));
    put("clk", "%u", g32(VAR(D_803156C4)));
    {
        uint8_t who = g8(VAR(D_80364AE8));
        const char *pl = VAR(D_80364AF0) + 0x100 * (who & 3);
        char med[2 * 60 + 1];
        for (int i = 0; i < 60; i++)
            snprintf(med + 2 * i, 3, "%02X", g8(pl + 0x18 + i));
        put("med", "%s", med);
        put("units", "%u", g16(pl + 0xA));
        put("gs", "%u", g8(pl + 0x91));
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
