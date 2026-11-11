/*
 * The ROM when none is given on the command line (a packaged app started
 * from its icon): ROM_DEFAULT in the working directory, as always; else,
 * with a window to come, the one the player picked before, or the system's
 * file dialog asks for it (Windows' open dialog, osascript's `choose file`
 * on macOS, zenity or kdialog on Linux), or, where there is none of
 * those, a window of the port's own takes it dropped on it.  The choice
 * is kept in the player's data directory (SDL_GetPrefPath:
 * ~/.local/share/BlastCorps, %APPDATA%\BlastCorps, ~/Library/Application
 * Support/BlastCorps), and the save goes there too.  Nothing else is looked at.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "host.h"
#include "hdtext.h"
#include "pack.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#else
#include <sys/wait.h>
#endif

void host_error_box(const char *msg) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Blast Corps", msg, NULL);
}

static int readable(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    fclose(f);
    return 1;
}

/* this version's ROM (in any byte order), or a resource pack */
static int ours(const char *path) {
    if (pack_is_pack(path))
        return 1;
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *rom = n > 0 && n <= 64 << 20 ? malloc((size_t)n) : NULL;
    int ok = rom && fread(rom, 1, (size_t)n, f) == (size_t)n && host_rom_to_z64(rom, (uint32_t)n);
    fclose(f);
    if (ok) {
        char sha1[41];
        host_sha1_hex(rom, (size_t)n, sha1);
        ok = !strcmp(sha1, PORT_ROM_SHA1) || (getenv("PORT_ROM_ANY") && atoi(getenv("PORT_ROM_ANY")));
    }
    free(rom);
    return ok;
}

static void chomp(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = 0;
}

#define PROMPT "Choose your Blast Corps ROM (" PORT_VERSION_NAME ", .z64, .v64 or .n64)"

/* the system's file dialog; the path picked (malloc'd), or NULL (*none:
   there is no dialog to ask with) */
static char *ask(int *none) {
    *none = 0;
#if defined(_WIN32)
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.lpstrFilter = "N64 ROMs (*.z64;*.v64;*.n64)\0*.z64;*.v64;*.n64\0All files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof path;
    ofn.lpstrTitle = PROMPT;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofn) ? strdup(path) : NULL;
#else
    static const char *const cmds[] = {
#ifdef __APPLE__
        "osascript -e 'POSIX path of (choose file with prompt \"" PROMPT "\")' 2>/dev/null",
#else
        "zenity --file-selection --title='" PROMPT "' "
        "--file-filter='N64 ROMs | *.z64 *.v64 *.n64' --file-filter='All files | *' 2>/dev/null",
        "kdialog --getopenfilename . 'N64 ROMs (*.z64 *.v64 *.n64)' --title '" PROMPT "' 2>/dev/null",
#endif
    };
    for (size_t i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        FILE *p = popen(cmds[i], "r");
        if (!p)
            continue;
        char path[4096] = "";
        int got = fgets(path, sizeof path, p) != NULL;
        int rc = pclose(p);
        chomp(path);
        if (got && rc == 0 && *path)
            return strdup(path);
        if (rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) != 127)
            return NULL;            /* (the dialog ran, and was cancelled) */
    }
    *none = 1;
    return NULL;
#endif
}

/* Where there is no dialog (no zenity or kdialog, as in a sandbox): a
 * window that takes the ROM dropped on it, its text drawn in the
 * built-in font (hdtext.c), since SDL's own message box needs zenity or
 * the X server's core fonts too.  The file dropped (malloc'd); NULL: no
 * window (closing it quits).  err: a line about the last one, or NULL. */
#define DROP_W 720
#define DROP_H 300

static void drop_draw(SDL_Window *win, const char *err) {
    SDL_Surface *s = SDL_GetWindowSurface(win);
    if (!s)
        return;
    static const struct {
        const char *text;
        float px;
        uint8_t r, g, b;
    } lines[] = {
        { "BLAST CORPS", 44, 0xF0, 0xC0, 0x20 },
        { "Drop your Blast Corps ROM (" PORT_VERSION_NAME ") on this window", 26, 0xE8, 0xE8, 0xE8 },
        { "(.z64, .v64 or .n64), or start blastcorps with it as its argument.", 20, 0xA0, 0xA0, 0xA0 },
    };
    int n = (int)(sizeof lines / sizeof lines[0]);
    uint8_t *cov = calloc((size_t)s->w * (size_t)s->h, 1);
    SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 0x20, 0x20, 0x24));
    for (int k = 0; cov && k <= n; k++) {
        const char *t = k < n ? lines[k].text : err;
        if (!t)
            continue;
        float px = k < n ? lines[k].px : 20;
        int tw = hdtext_line(t, px, NULL, 0, 0, 0, 0);
        while (tw > s->w - 32 && px > 8)            /* (a long file name) */
            tw = hdtext_line(t, px -= 1, NULL, 0, 0, 0, 0);
        int y = k == 0 ? 80 : k == 1 ? 150 : k == 2 ? 190 : 250;
        memset(cov, 0, (size_t)s->w * (size_t)s->h);
        hdtext_line(t, px, cov, s->w, s->h, (s->w - tw) / 2, y * s->h / DROP_H);
        uint8_t r = k < n ? lines[k].r : 0xFF, g = k < n ? lines[k].g : 0x70, b = k < n ? lines[k].b : 0x60;
        SDL_LockSurface(s);
        for (int j = 0; j < s->h; j++)
            for (int i = 0; i < s->w; i++) {
                unsigned a = cov[j * s->w + i];
                if (!a)
                    continue;
                Uint32 *p = (Uint32 *)((uint8_t *)s->pixels + j * s->pitch + i * s->format->BytesPerPixel);
                Uint8 r0, g0, b0;
                SDL_GetRGB(*p, s->format, &r0, &g0, &b0);
                *p = SDL_MapRGB(s->format, (Uint8)((r * a + r0 * (255 - a)) / 255),
                                (Uint8)((g * a + g0 * (255 - a)) / 255), (Uint8)((b * a + b0 * (255 - a)) / 255));
            }
        SDL_UnlockSurface(s);
    }
    free(cov);
    SDL_UpdateWindowSurface(win);
}

static char *drop(const char *err) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        host_log("no file dialog, and no window either: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_Window *win = SDL_CreateWindow("Blast Corps", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       DROP_W, DROP_H, 0);
    char *got = NULL;
    if (win) {
        drop_draw(win, err);
        SDL_Event e;
        while (!got && SDL_WaitEvent(&e)) {
            if (e.type == SDL_QUIT) {
                host_log("no ROM: start it with your Blast Corps ROM (" PORT_VERSION_NAME
                         ") as its argument, or drop the ROM on its window\n");
                exit(1);            /* (the player closed it: no box) */
            }
            if (e.type == SDL_DROPFILE) {
                got = strdup(e.drop.file);
                SDL_free(e.drop.file);
            } else if (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_EXPOSED ||
                                                     e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
                drop_draw(win, err);
            }
        }
        SDL_DestroyWindow(win);
    } else {
        host_log("no file dialog, and no window either: %s\n", SDL_GetError());
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return got;
}

const char *host_find_rom(const char *def, int windowed, const char **save) {
    if (readable(def) || !windowed)
        return def;
#ifdef __EMSCRIPTEN__
    (void)save;
    return def;
#else
    char *dir = SDL_GetPrefPath("", "BlastCorps");
    if (!dir)
        return def;
    size_t n = strlen(dir) + 32;
    char *kept = malloc(n), *sv = malloc(n);
    snprintf(kept, n, "%srom-path.txt", dir);
    snprintf(sv, n, "%sblastcorps.eep", dir);
    SDL_free(dir);
    *save = sv;
    char path[4096] = "";
    FILE *f = fopen(kept, "r");
    if (f) {
        if (!fgets(path, sizeof path, f))
            *path = 0;
        fclose(f);
        chomp(path);
        if (*path && ours(path))
            return strdup(path);
    }
    char *picked;
    int none = 0;
    char err[256] = "";
    for (;;) {
        if (!none)
            picked = ask(&none);
        if (none)
            picked = drop(*err ? err : NULL);
        if (!picked)
            host_fatal("no ROM: start it with your Blast Corps ROM (" PORT_VERSION_NAME
                       ") as its argument, or pick the ROM when it asks");
        if (ours(picked))
            break;
        if (none) {
            const char *base = strrchr(picked, '/');
            snprintf(err, sizeof err, "%s isn't the " PORT_VERSION_NAME " ROM. Drop another.",
                     base ? base + 1 : picked);
        } else {
            host_error_box("That isn't the Blast Corps ROM this port is for (" PORT_VERSION_NAME
                           ", sha1 " PORT_ROM_SHA1 "). Choose another.");
        }
        free(picked);
    }
    if ((f = fopen(kept, "w"))) {
        fprintf(f, "%s\n", picked);
        fclose(f);
    }
    return picked;
#endif
}
