/*
 * Headless front end for BizHawk 2.7's mupen64plus core that plays a BizHawk
 * movie's input log (the "Input Log.txt" inside a .bk2) and records, at every
 * controller read, what the game was given and when (port/tools/tas.sh).
 *
 *   m64p_tas CORE.so RSP.so DATADIR ROM INPUTLOG OUTDIR [VIS]
 *
 * CORE.so is BizHawk's core built for Linux (tas.sh), DATADIR its data/ (the
 * ROM database the core looks the ROM up in), RSP.so its rsp-hle.
 *
 * BizHawk's N64 frame is one VI, cut where the core calls the VI callback.
 * It runs two frames at power-on before the movie starts (N64.cs: "Saving a
 * state on frame 0 has been shown to not be sync stable"), so line N of the
 * log is the pad for every controller read between VI N+2 and VI N+3.
 *
 * The program is the core's input plugin, answering from the line for the
 * current VI; its audio plugin, which plays nothing; and its gfx plugin,
 * which draws nothing and counts the VIs by UpdateScreen (the 2.0 core calls
 * it once a VI, just before the VI callback).  The gfx plugin still raises
 * the DP interrupt where a renderer would, at the list's full sync; see
 * ProcessDList.
 *
 * It stops after VIS VIs (default: the whole log, then 600 more) and writes
 *
 *   OUTDIR/polls.csv   one line per controller read: poll, VI, retraces (the
 *                      scheduler's count), game frames, the mode, the pad as
 *                      the PIF returns it (buttons << 16 | x << 8 | y), the
 *                      graphics tasks run so far
 *   OUTDIR/modes.csv   one line per change of the mode (D_80364A90/94)
 *   OUTDIR/vis.csv     one line per VI (from the first controller read):
 *                      the scheduler's count before it, the reads and the
 *                      graphics tasks so far
 *   OUTDIR/eeprom.bin  the EEPROM at the end (from blank, as the movie is)
 *   OUTDIR/rdram.bin   RDRAM at the end, big-endian
 *
 *   cc -O1 -Wall -I<core>/src/api -rdynamic -o m64p_tas m64p_tas.c -ldl
 */
#define M64P_PLUGIN_PROTOTYPES 1
#define M64P_CORE_PROTOTYPES 1
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <m64p_types.h>
#include <m64p_plugin.h>
#include <m64p_frontend.h>
#include <m64p_debugger.h>
#include <m64p_config.h>

static uint32_t *pads;          /* per movie frame: buttons << 16 | x << 8 | y */
static unsigned npads, vis, max_vis, polls, tasks;
static FILE *polls_csv, *modes_csv, *vis_csv;
static m64p_plugin_type attaching;
static unsigned char *rdram;
static GFX_INFO gfx;
/* where us.v11 has D_803156C4 (retraces), D_80358064 (game frames) and
   D_80364A90 (the mode); us.v10 has all three 0xB0 lower */
static uint32_t a_retraces = 0x803156C4, a_frames = 0x80358064, a_mode = 0x80364A90;
static uint64_t last_mode = ~0ull;

static m64p_error (*pCoreDoCommand)(m64p_command, int, void *);
static void *(*pDebugMemGetPointer)(m64p_dbg_memptr_type);

/* the core keeps RDRAM as native-endian words */
static uint32_t rd32(uint32_t a) {
    uint32_t w;
    memcpy(&w, rdram + (a & 0x3FFFFC), 4);
    return w;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *type, int *ver, int *api,
                                        const char **name, int *caps) {
    if (type) *type = attaching;
    if (ver) *ver = 0x010000;
    if (api) *api = attaching == M64PLUGIN_GFX ? 0x020200 : attaching == M64PLUGIN_AUDIO ? 0x020000 : 0x020100;
    if (name) *name = "blastcorps movie player";
    if (caps) *caps = 0;
    return M64ERR_SUCCESS;
}
EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle h, void *c, void (*d)(void *, int, const char *)) {
    (void)h; (void)c; (void)d;
    return M64ERR_SUCCESS;
}
EXPORT m64p_error CALL PluginShutdown(void) { return M64ERR_SUCCESS; }
EXPORT int CALL RomOpen(void) { return 1; }
EXPORT void CALL RomClosed(void) { }

/* ---- gfx: nothing drawn, VIs counted ---------------------------------------- */

/* The DP interrupt comes from the gfx plugin, which sets MI_INTR's DP bit at
   the list's full sync (the core then raises it 1000 counts later), and not
   for a list without one: the game sends some, and a DP for them wedges its
   scheduler.  The game's lists are F3D: G_DL 06 (a call, or a branch when
   the second byte is 1), G_ENDDL B8, G_MOVEWORD BC with index 6 setting a
   segment, G_RDPFULLSYNC E9.  (The CPU never reads the framebuffers, so
   the pixels a renderer would write back don't matter.) */
static uint32_t dl32(uint32_t a) {
    uint32_t w;
    memcpy(&w, gfx.RDRAM + (a & 0x3FFFFC), 4);
    return w;
}
static int dl_full_sync(uint32_t a) {
    uint32_t seg[16] = { 0 }, stack[16];
    int sp = 0;
    for (int n = 0; n < 1000000; n++, a += 8) {
        uint32_t w0 = dl32(a), w1 = dl32(a + 4);
        switch (w0 >> 24) {
        case 0xE9:
            return 1;
        case 0x06:
            if ((w0 >> 16 & 0xFF) == 0) {
                if (sp == 16) return 0;
                stack[sp++] = a;
            }
            a = seg[w1 >> 24 & 15] + (w1 & 0xFFFFFF) - 8;
            break;
        case 0xB8:
            if (sp == 0) return 0;
            a = stack[--sp];
            break;
        case 0xBC:
            if ((w0 & 0xFF) == 6)
                seg[(w0 >> 10) & 15] = w1 & 0xFFFFFF;
            break;
        }
    }
    return 0;
}
EXPORT int CALL InitiateGFX(GFX_INFO info) { gfx = info; return 1; }
EXPORT void CALL ProcessDList(void) {
    tasks++;
    /* the task's data_ptr (OSTask at DMEM 0xFC0) */
    if (dl_full_sync(((uint32_t *)(gfx.DMEM + 0xFC0))[12])) {
        *gfx.MI_INTR_REG |= 0x20;
        gfx.CheckInterrupts();
    }
}
EXPORT void CALL ChangeWindow(void) { }
EXPORT void CALL MoveScreen(int x, int y) { (void)x; (void)y; }
EXPORT void CALL ProcessRDPList(void) { }
EXPORT void CALL ShowCFB(void) { }
EXPORT void CALL ViStatusChanged(void) { }
EXPORT void CALL ViWidthChanged(void) { }
EXPORT void CALL ReadScreen2(void *dest, int *w, int *h, int front) { (void)dest; (void)front; *w = *h = 0; }
EXPORT void CALL SetRenderingCallback(void (*cb)(int)) { (void)cb; }
EXPORT void CALL ResizeVideoOutput(int w, int h) { (void)w; (void)h; }
EXPORT void CALL FBRead(unsigned int a) { (void)a; }
EXPORT void CALL FBWrite(unsigned int a, unsigned int n) { (void)a; (void)n; }
EXPORT void CALL FBGetFrameBufferInfo(void *p) { (void)p; }
EXPORT void CALL UpdateScreen(void) {
    /* before the scheduler counts this retrace */
    if (rdram)
        fprintf(vis_csv, "%u,%u,%u,%u\n", vis, rd32(a_retraces), polls, tasks);
    vis++;
    if (vis % 10000 == 0)
        fprintf(stderr, "vi %u, %u reads\n", vis, polls);
    if (vis >= max_vis)
        pCoreDoCommand(M64CMD_STOP, 0, NULL);
}

/* ---- audio: none (the core paces the AI itself) ------------------------------- */

EXPORT void CALL AiDacrateChanged(int system) { (void)system; }
EXPORT void CALL AiLenChanged(void) { }
EXPORT int CALL InitiateAudio(AUDIO_INFO info) { (void)info; return 1; }
EXPORT void CALL ProcessAList(void) { }
EXPORT void CALL SetSpeedFactor(int p) { (void)p; }
EXPORT void CALL VolumeUp(void) { }
EXPORT void CALL VolumeDown(void) { }
EXPORT int CALL VolumeGetLevel(void) { return 100; }
EXPORT void CALL VolumeSetLevel(int l) { (void)l; }
EXPORT void CALL VolumeMute(void) { }
EXPORT const char *CALL VolumeGetString(void) { return "100%"; }

/* ---- input: the movie ------------------------------------------------------- */

EXPORT void CALL InitiateControllers(CONTROL_INFO info) {
    info.Controls[0].Present = 1;
    info.Controls[0].RawData = 0;
    info.Controls[0].Plugin = PLUGIN_NONE;
}

EXPORT void CALL GetKeys(int control, BUTTONS *keys) {
    keys->Value = 0;
    if (control != 0)
        return;
    uint32_t pad = vis >= 2 && vis - 2 < npads ? pads[vis - 2] : 0;
    /* BUTTONS is the PIF's word with its two button bytes swapped */
    keys->Value = (pad >> 24 & 0xFF) | (pad >> 16 & 0x3F) << 8 | (pad >> 8 & 0xFF) << 16 | (pad & 0xFF) << 24;
    polls++;
    if (!rdram)
        rdram = pDebugMemGetPointer(M64P_DBG_PTR_RDRAM);
    uint64_t mode = (uint64_t)rd32(a_mode) << 32 | rd32(a_mode + 4);
    fprintf(polls_csv, "%u,%u,%u,%u,%016llX,%08X,%u\n", polls, vis, rd32(a_retraces), rd32(a_frames),
            (unsigned long long)mode, pad, tasks);
    if (mode != last_mode) {
        fprintf(modes_csv, "%u,%u,%016llX\n", polls, vis, (unsigned long long)mode);
        last_mode = mode;
    }
}
EXPORT void CALL ControllerCommand(int c, unsigned char *cmd) { (void)c; (void)cmd; }
EXPORT void CALL ReadController(int c, unsigned char *cmd) { (void)c; (void)cmd; }
EXPORT void CALL SDL_KeyDown(int k, int s) { (void)k; (void)s; }
EXPORT void CALL SDL_KeyUp(int k, int s) { (void)k; (void)s; }

/* ---- the log ------------------------------------------------------------------ */

/* |..|    0,    0,..................|: reset and power, the stick, then
   A Up/Down/Left/Right (the stick all the way, over the axis values, as
   BizHawk's N64Input.cs has it), DPad U D L R, Start, Z, B, A, C U D L R,
   L, R, '.' for released.  The PIF's word has A B Z Start DU DD DL DR,
   0 0 L R CU CD CL CR. */
static void read_log(const char *path) {
    static const int bit[14] = { 11, 10, 9, 8, 12, 13, 14, 15, 3, 2, 1, 0, 5, 4 };
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(1); }
    char line[256];
    unsigned cap = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] != '|' || strlen(line) < 20)
            continue;
        char *p = strchr(line + 1, '|');
        if (!p) continue;
        if (p[-2] != '.' || p[-1] != '.') {
            fprintf(stderr, "frame %u: reset/power is not supported\n", npads);
            exit(1);
        }
        int x, y, n;
        if (sscanf(p + 1, " %d, %d,%n", &x, &y, &n) != 2) continue;
        p += 1 + n;
        if (p[0] != '.') y = 127;
        else if (p[1] != '.') y = -128;
        if (p[2] != '.') x = -128;
        else if (p[3] != '.') x = 127;
        uint32_t b = 0;
        for (int i = 0; i < 14; i++)
            if (p[4 + i] != '.') b |= 1u << bit[i];
        if (npads == cap)
            pads = realloc(pads, (cap = cap ? cap * 2 : 65536) * sizeof *pads);
        pads[npads++] = b << 16 | (uint8_t)x << 8 | (uint8_t)y;
    }
    fclose(f);
}

/* ---- front end ---------------------------------------------------------------- */

static void debug_cb(void *ctx, int level, const char *msg) {
    (void)ctx;
    if (level <= M64MSG_WARNING)
        fprintf(stderr, "core: %s\n", msg);
}

static FILE *out(const char *dir, const char *name, const char *mode) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, mode);
    if (!f) { perror(path); exit(1); }
    return f;
}

#define SYM(h, name) dlsym(h, #name)

int main(int argc, char **argv) {
    if (argc < 7) {
        fprintf(stderr, "usage: %s CORE.so RSP.so DATADIR ROM INPUTLOG OUTDIR [VIS]\n", argv[0]);
        return 1;
    }
    const char *outdir = argv[6];
    read_log(argv[5]);
    max_vis = argc > 7 ? (unsigned)strtoul(argv[7], NULL, 0) : npads + 2 + 600;
    fprintf(stderr, "movie: %u frames\n", npads);

    mkdir(outdir, 0755);
    polls_csv = out(outdir, "polls.csv", "w");
    fprintf(polls_csv, "poll,vi,retraces,frames,mode,pad,tasks\n");
    modes_csv = out(outdir, "modes.csv", "w");
    fprintf(modes_csv, "poll,vi,mode\n");
    vis_csv = out(outdir, "vis.csv", "w");
    fprintf(vis_csv, "vi,retraces,reads,tasks\n");

    FILE *f = fopen(argv[4], "rb");
    if (!f) { perror(argv[4]); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *rom = malloc(size);
    if (fread(rom, 1, size, f) != (size_t)size) return 1;
    fclose(f);
    /* us.v10 (header version byte 0) */
    if (rom[0x3F] == 0 && !memcmp(rom + 0x3B, "NBCE", 4)) {
        a_retraces -= 0xB0; a_frames -= 0xB0; a_mode -= 0xB0;
    }

    void *core = dlopen(argv[1], RTLD_NOW | RTLD_GLOBAL);
    if (!core) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    m64p_error (*pCoreStartup)(int, const char *, const char *, void *, ptr_DebugCallback, void *, ptr_StateCallback) = SYM(core, CoreStartup);
    m64p_error (*pCoreAttachPlugin)(m64p_plugin_type, m64p_dynlib_handle) = SYM(core, CoreAttachPlugin);
    m64p_error (*pConfigOpenSection)(const char *, m64p_handle *) = SYM(core, ConfigOpenSection);
    m64p_error (*pConfigSetParameter)(m64p_handle, const char *, m64p_type, const void *) = SYM(core, ConfigSetParameter);
    void (*p_save_saveram)(unsigned char *) = SYM(core, save_saveram);
    pCoreDoCommand = SYM(core, CoreDoCommand);
    pDebugMemGetPointer = SYM(core, DebugMemGetPointer);
    if (!p_save_saveram) { fprintf(stderr, "%s: not BizHawk's core\n", argv[1]); return 1; }
    if (pCoreStartup(0x020001, outdir, argv[3], NULL, debug_cb, NULL, NULL) != M64ERR_SUCCESS)
        return 1;
    /* the movie's sync settings: no expansion pak, Core 1 (the cached
       interpreter); the EEPROM starts zeroed, as BizHawk's init_saveram has it */
    m64p_handle sec;
    if (pConfigOpenSection("Core", &sec) == M64ERR_SUCCESS) {
        int one = 1;
        pConfigSetParameter(sec, "DisableExtraMem", M64TYPE_INT, &one);
        pConfigSetParameter(sec, "R4300Emulator", M64TYPE_INT, &one);
    }
    if (pCoreDoCommand(M64CMD_ROM_OPEN, (int)size, rom) != M64ERR_SUCCESS)
        return 1;

    void *rsp = dlopen(argv[2], RTLD_NOW);
    if (!rsp) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    m64p_error (*startup)(m64p_dynlib_handle, void *, void (*)(void *, int, const char *)) = SYM(rsp, PluginStartup);
    startup(core, NULL, debug_cb);
    void *self = dlopen(NULL, RTLD_NOW);
    static const m64p_plugin_type types[] = { M64PLUGIN_GFX, M64PLUGIN_AUDIO, M64PLUGIN_INPUT };
    for (int i = 0; i < 3; i++) {
        attaching = types[i];
        if (pCoreAttachPlugin(types[i], self) != M64ERR_SUCCESS) {
            fprintf(stderr, "attaching plugin type %d failed\n", types[i]);
            return 1;
        }
    }
    if (pCoreAttachPlugin(M64PLUGIN_RSP, rsp) != M64ERR_SUCCESS)
        return 1;

    int zero = 0;
    pCoreDoCommand(M64CMD_CORE_STATE_SET, M64CORE_SPEED_LIMITER, &zero);
    pCoreDoCommand(M64CMD_EXECUTE, 0, NULL);

    /* save_saveram: the EEPROM's 0x800, the four paks, flash RAM, SRAM */
    unsigned char *save = malloc(0x800 + 4 * 0x8000 + 0x20000 + 0x8000);
    p_save_saveram(save);
    FILE *e = out(outdir, "eeprom.bin", "wb");
    fwrite(save, 1, 0x800, e);
    fclose(e);
    if (rdram) {
        FILE *r = out(outdir, "rdram.bin", "wb");
        for (uint32_t a = 0; a < 0x400000; a += 4) {
            uint32_t w = rd32(a);
            w = w >> 24 | (w >> 8 & 0xFF00) | (w << 8 & 0xFF0000) | w << 24;
            fwrite(&w, 4, 1, r);
        }
        fclose(r);
    }
    pCoreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
    fclose(polls_csv);
    fclose(modes_csv);
    fclose(vis_csv);
    fprintf(stderr, "%u VIs, %u controller reads\n", vis, polls);
    return 0;
}
