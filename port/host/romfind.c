/*
 * The ROM when none is given on the command line (a packaged app started
 * from its icon): ROM_DEFAULT in the working directory, as always; else,
 * with a window to come, the one the player picked before, or the system's
 * file dialog asks for it (Windows' open dialog, osascript's `choose file`
 * on macOS, zenity or kdialog on Linux).  The choice is kept in the
 * player's data directory (SDL_GetPrefPath: ~/.local/share/BlastCorps,
 * %APPDATA%\BlastCorps, ~/Library/Application Support/BlastCorps), and the
 * save goes there too.  Nothing else is looked at.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "host.h"
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

/* the system's file dialog; the path picked (malloc'd), or NULL */
static char *ask(void) {
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
    return NULL;
#endif
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
    for (;;) {
        if (!(picked = ask()))
            host_fatal("no ROM: start it with your Blast Corps ROM (" PORT_VERSION_NAME
                       ") as its argument, or pick the ROM when it asks");
        if (ours(picked))
            break;
        host_error_box("That isn't the Blast Corps ROM this port is for (" PORT_VERSION_NAME
                       ", sha1 " PORT_ROM_SHA1 "). Choose another.");
        free(picked);
    }
    if ((f = fopen(kept, "w"))) {
        fprintf(f, "%s\n", picked);
        fclose(f);
    }
    return picked;
#endif
}
