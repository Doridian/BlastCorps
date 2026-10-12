/*
 * PORT_DUMP=N,... and PORT_REPLAY_DUMP=N,...: the game's memory to a file.
 *
 * There is no RDRAM: the game's memory is its variables and its fixed
 * buffers (port/src/regions.c).  A dump is those: the regions
 * (port_regions, in their order), then every variable the host names
 * (host_game_vars.h) that holds its data itself, in the order below, each
 * as a line "name size" and the size bytes after it, in host order.  Two
 * dumps of one build compare byte for byte with ASLR off (setarch -R: the
 * pointers in the game's memory are the run's; the pointer variables
 * themselves are left out).
 */
#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "host_game_vars.h"

#ifdef VERSION_EU
#define YOSHI_WINDOWS 0x6D
#else
#define YOSHI_WINDOWS 0x6C
#endif

static const struct {
    const char *name;
    const void *p;
    size_t size;                /* or: */
    enum PortLayout layout;     /* host_layout(layout) * count bytes */
    unsigned count;
} vars[] = {
    { "D_80315440", (const void *)&D_80315440, 0, PORT_LAYOUT_BYTES_SCHED, 1 },
    { "D_80364A90", (const void *)&D_80364A90, sizeof D_80364A90, 0, 0 },
    { "D_802E8BDC", (const void *)&D_802E8BDC, sizeof D_802E8BDC, 0, 0 },
    { "D_80358064", (const void *)&D_80358064, sizeof D_80358064, 0, 0 },
    { "D_80358060", (const void *)&D_80358060, sizeof D_80358060, 0, 0 },
    { "D_8036B968", (const void *)&D_8036B968, sizeof D_8036B968, 0, 0 },
    { "D_80364AF0", (const void *)&D_80364AF0, sizeof D_80364AF0, 0, 0 },
    { "D_80364AE8", (const void *)&D_80364AE8, sizeof D_80364AE8, 0, 0 },
    { "D_80364456", (const void *)&D_80364456, sizeof D_80364456, 0, 0 },
    { "D_803643E0", (const void *)&D_803643E0, sizeof D_803643E0, 0, 0 },
    { "D_803643E4", (const void *)&D_803643E4, sizeof D_803643E4, 0, 0 },
    { "D_803643E8", (const void *)&D_803643E8, sizeof D_803643E8, 0, 0 },
    { "D_803643D9", (const void *)&D_803643D9, sizeof D_803643D9, 0, 0 },
    { "D_803643DA", (const void *)&D_803643DA, sizeof D_803643DA, 0, 0 },
    { "D_8036EA70", (const void *)&D_8036EA70, 0, PORT_LAYOUT_BYTES_STATS, 1 },
    { "D_8036EB92", (const void *)&D_8036EB92, sizeof D_8036EB92, 0, 0 },
    { "D_8036EB93", (const void *)&D_8036EB93, sizeof D_8036EB93, 0, 0 },
    { "D_8036EB90", (const void *)&D_8036EB90, sizeof D_8036EB90, 0, 0 },
    { "D_803649F0", (const void *)&D_803649F0, sizeof D_803649F0, 0, 0 },
    { "D_803F8B72", (const void *)&D_803F8B72, sizeof D_803F8B72, 0, 0 },
    { "D_803EDC00", (const void *)&D_803EDC00, sizeof D_803EDC00, 0, 0 },
    { "D_80364460", (const void *)&D_80364460, 0, PORT_LAYOUT_BYTES_VEHICLES, 1 },
    { "D_803F4030", (const void *)&D_803F4030, 0, PORT_LAYOUT_BYTES_BUILDINGS, 1 },
    { "D_8039B070", (const void *)&D_8039B070, 0, PORT_LAYOUT_BYTES_TNT, 1 },
    { "D_8039B610", (const void *)&D_8039B610, sizeof D_8039B610, 0, 0 },
    { "D_8039C550", (const void *)&D_8039C550, 0, PORT_LAYOUT_BYTES_BLOCKS, 1 },
    { "D_8039C710", (const void *)&D_8039C710, sizeof D_8039C710, 0, 0 },
    { "D_8039AF00", (const void *)&D_8039AF00, 0, PORT_LAYOUT_BYTES_AMMOBOXES, 1 },
    { "D_8039B068", (const void *)&D_8039B068, sizeof D_8039B068, 0, 0 },
    { "D_803ED760", (const void *)&D_803ED760, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EDB40", (const void *)&D_803EDB40, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EDF10", (const void *)&D_803EDF10, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EE2E0", (const void *)&D_803EE2E0, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EE6C0", (const void *)&D_803EE6C0, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EEA90", (const void *)&D_803EEA90, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EEE70", (const void *)&D_803EEE70, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EF630", (const void *)&D_803EF630, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EFA20", (const void *)&D_803EFA20, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803EFDF0", (const void *)&D_803EFDF0, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803F7B50", (const void *)&D_803F7B50, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803F8AA0", (const void *)&D_803F8AA0, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803F8E80", (const void *)&D_803F8E80, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803F9250", (const void *)&D_803F9250, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803FC500", (const void *)&D_803FC500, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803FC8D0", (const void *)&D_803FC8D0, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803FCCA0", (const void *)&D_803FCCA0, 0, PORT_LAYOUT_VSTATE_SIZE, 1 },
    { "D_803F8550", (const void *)&D_803F8550, 0, PORT_LAYOUT_VSTATE_SIZE, 3 },
    { "D_803F8748", (const void *)&D_803F8748, sizeof D_803F8748, 0, 0 },
    { "D_803ED808", (const void *)&D_803ED808, sizeof D_803ED808, 0, 0 },
    { "D_803ED80C", (const void *)&D_803ED80C, sizeof D_803ED80C, 0, 0 },
    { "D_803ED810", (const void *)&D_803ED810, sizeof D_803ED810, 0, 0 },
    { "D_803EDBE8", (const void *)&D_803EDBE8, sizeof D_803EDBE8, 0, 0 },
    { "D_803EDBEC", (const void *)&D_803EDBEC, sizeof D_803EDBEC, 0, 0 },
    { "D_803EDBF0", (const void *)&D_803EDBF0, sizeof D_803EDBF0, 0, 0 },
    { "D_803EDFB8", (const void *)&D_803EDFB8, sizeof D_803EDFB8, 0, 0 },
    { "D_803EDFBC", (const void *)&D_803EDFBC, sizeof D_803EDFBC, 0, 0 },
    { "D_803EDFC0", (const void *)&D_803EDFC0, sizeof D_803EDFC0, 0, 0 },
    { "D_803EE38C", (const void *)&D_803EE38C, sizeof D_803EE38C, 0, 0 },
    { "D_803EE390", (const void *)&D_803EE390, sizeof D_803EE390, 0, 0 },
    { "D_803EE394", (const void *)&D_803EE394, sizeof D_803EE394, 0, 0 },
    { "D_803EE768", (const void *)&D_803EE768, sizeof D_803EE768, 0, 0 },
    { "D_803EE76C", (const void *)&D_803EE76C, sizeof D_803EE76C, 0, 0 },
    { "D_803EE770", (const void *)&D_803EE770, sizeof D_803EE770, 0, 0 },
    { "D_803EEB38", (const void *)&D_803EEB38, sizeof D_803EEB38, 0, 0 },
    { "D_803EEB3C", (const void *)&D_803EEB3C, sizeof D_803EEB3C, 0, 0 },
    { "D_803EEB40", (const void *)&D_803EEB40, sizeof D_803EEB40, 0, 0 },
    { "D_803EEF18", (const void *)&D_803EEF18, sizeof D_803EEF18, 0, 0 },
    { "D_803EEF1C", (const void *)&D_803EEF1C, sizeof D_803EEF1C, 0, 0 },
    { "D_803EEF20", (const void *)&D_803EEF20, sizeof D_803EEF20, 0, 0 },
    { "D_803EF6DC", (const void *)&D_803EF6DC, sizeof D_803EF6DC, 0, 0 },
    { "D_803EF6E0", (const void *)&D_803EF6E0, sizeof D_803EF6E0, 0, 0 },
    { "D_803EF6E4", (const void *)&D_803EF6E4, sizeof D_803EF6E4, 0, 0 },
    { "D_803EFAC8", (const void *)&D_803EFAC8, sizeof D_803EFAC8, 0, 0 },
    { "D_803EFACC", (const void *)&D_803EFACC, sizeof D_803EFACC, 0, 0 },
    { "D_803EFAD0", (const void *)&D_803EFAD0, sizeof D_803EFAD0, 0, 0 },
    { "D_803EFE98", (const void *)&D_803EFE98, sizeof D_803EFE98, 0, 0 },
    { "D_803EFE9C", (const void *)&D_803EFE9C, sizeof D_803EFE9C, 0, 0 },
    { "D_803EFEA0", (const void *)&D_803EFEA0, sizeof D_803EFEA0, 0, 0 },
    { "D_803F7BF8", (const void *)&D_803F7BF8, sizeof D_803F7BF8, 0, 0 },
    { "D_803F7BFC", (const void *)&D_803F7BFC, sizeof D_803F7BFC, 0, 0 },
    { "D_803F7C00", (const void *)&D_803F7C00, sizeof D_803F7C00, 0, 0 },
    { "D_803F8B48", (const void *)&D_803F8B48, sizeof D_803F8B48, 0, 0 },
    { "D_803F8B4C", (const void *)&D_803F8B4C, sizeof D_803F8B4C, 0, 0 },
    { "D_803F8B50", (const void *)&D_803F8B50, sizeof D_803F8B50, 0, 0 },
    { "D_803F8F28", (const void *)&D_803F8F28, sizeof D_803F8F28, 0, 0 },
    { "D_803F8F2C", (const void *)&D_803F8F2C, sizeof D_803F8F2C, 0, 0 },
    { "D_803F8F30", (const void *)&D_803F8F30, sizeof D_803F8F30, 0, 0 },
    { "D_803F92F8", (const void *)&D_803F92F8, sizeof D_803F92F8, 0, 0 },
    { "D_803F92FC", (const void *)&D_803F92FC, sizeof D_803F92FC, 0, 0 },
    { "D_803F9300", (const void *)&D_803F9300, sizeof D_803F9300, 0, 0 },
    { "D_803FC5A8", (const void *)&D_803FC5A8, sizeof D_803FC5A8, 0, 0 },
    { "D_803FC5AC", (const void *)&D_803FC5AC, sizeof D_803FC5AC, 0, 0 },
    { "D_803FC5B0", (const void *)&D_803FC5B0, sizeof D_803FC5B0, 0, 0 },
    { "D_803FC978", (const void *)&D_803FC978, sizeof D_803FC978, 0, 0 },
    { "D_803FC97C", (const void *)&D_803FC97C, sizeof D_803FC97C, 0, 0 },
    { "D_803FC980", (const void *)&D_803FC980, sizeof D_803FC980, 0, 0 },
    { "D_803FCD48", (const void *)&D_803FCD48, sizeof D_803FCD48, 0, 0 },
    { "D_803FCD4C", (const void *)&D_803FCD4C, sizeof D_803FCD4C, 0, 0 },
    { "D_803FCD50", (const void *)&D_803FCD50, sizeof D_803FCD50, 0, 0 },
    { "D_80364EF0", (const void *)&D_80364EF0, sizeof D_80364EF0, 0, 0 },
    { "D_80364F70", (const void *)&D_80364F70, sizeof D_80364F70, 0, 0 },
    { "D_80364A98", (const void *)&D_80364A98, sizeof D_80364A98, 0, 0 },
    { "D_80364AA0", (const void *)&D_80364AA0, sizeof D_80364AA0, 0, 0 },
    { "D_8036C778", (const void *)&D_8036C778, sizeof D_8036C778, 0, 0 },
    { "D_8036C784", (const void *)&D_8036C784, sizeof D_8036C784, 0, 0 },
    { "D_8036BB18", (const void *)&D_8036BB18, sizeof D_8036BB18, 0, 0 },
    { "D_8036BB1C", (const void *)&D_8036BB1C, sizeof D_8036BB1C, 0, 0 },
    { "D_802154B2", (const void *)&D_802154B2, sizeof D_802154B2, 0, 0 },
    { "D_802154B4", (const void *)&D_802154B4, sizeof D_802154B4, 0, 0 },
    { "D_802154BE", (const void *)&D_802154BE, sizeof D_802154BE, 0, 0 },
    { "D_802154BC", (const void *)&D_802154BC, sizeof D_802154BC, 0, 0 },
    { "D_80215924", (const void *)&D_80215924, sizeof D_80215924, 0, 0 },
    { "D_802082FC", (const void *)&D_802082FC, 0x18, 0, 0 },
    { "D_80208314", (const void *)&D_80208314, 0xC, 0, 0 },
    { "D_80208350", (const void *)&D_80208350, sizeof D_80208350, 0, 0 },
    { "D_80215944", (const void *)&D_80215944, sizeof D_80215944, 0, 0 },
    { "D_80215948", (const void *)&D_80215948, sizeof D_80215948, 0, 0 },
    { "D_8021594C", (const void *)&D_8021594C, sizeof D_8021594C, 0, 0 },
    { "D_80215950", (const void *)&D_80215950, sizeof D_80215950, 0, 0 },
    { "D_80215440", (const void *)&D_80215440, sizeof D_80215440, 0, 0 },
    { "D_80215444", (const void *)&D_80215444, sizeof D_80215444, 0, 0 },
    { "D_80215448", (const void *)&D_80215448, sizeof D_80215448, 0, 0 },
    { "D_8021544C", (const void *)&D_8021544C, sizeof D_8021544C, 0, 0 },
    { "D_8039CAF0", (const void *)&D_8039CAF0, 0xA000, 0, 0 },
    { "D_803653B0", (const void *)&D_803653B0, sizeof D_803653B0, 0, 0 },
    { "D_80365350", (const void *)&D_80365350, sizeof D_80365350, 0, 0 },
    { "D_802E8BD0", (const void *)&D_802E8BD0, sizeof D_802E8BD0, 0, 0 },
    { "D_8036BB30", (const void *)&D_8036BB30, sizeof D_8036BB30, 0, 0 },
    { "D_8036BB3C", (const void *)&D_8036BB3C, sizeof D_8036BB3C, 0, 0 },
    { "D_8036BB3E", (const void *)&D_8036BB3E, sizeof D_8036BB3E, 0, 0 },
    { "D_802F8BDC", (const void *)&D_802F8BDC, YOSHI_WINDOWS * sizeof(HostYoshiWindow), 0, 0 },
    { "D_802F49F4", (const void *)&D_802F49F4, 0x4B * sizeof(HostYoshiIcon), 0, 0 },
    { "D_802E8F94", (const void *)&D_802E8F94, 60 * sizeof(HostLevelInfo), 0, 0 },
    { "D_8020D810", (const void *)&D_8020D810, 60 * sizeof(HostMapLevel), 0, 0 },
    { "D_803156F8", (const void *)&D_803156F8, 0, PORT_LAYOUT_BYTES_FRAMES, 1 },
    { "D_80217B70", (const void *)&D_80217B70, 7 * 4 * sizeof(HostMtx), 0, 0 },
    { "D_80217B6C", (const void *)&D_80217B6C, sizeof D_80217B6C, 0, 0 },
    { "D_8035805C", (const void *)&D_8035805C, sizeof D_8035805C, 0, 0 },
    { "D_8035807C", (const void *)&D_8035807C, sizeof D_8035807C, 0, 0 },
    { "D_8021A905", (const void *)&D_8021A905, sizeof D_8021A905, 0, 0 },
    { "D_8021A907", (const void *)&D_8021A907, sizeof D_8021A907, 0, 0 },
    { "D_8021A90C", (const void *)&D_8021A90C, sizeof D_8021A90C, 0, 0 },
    { "D_8021A910", (const void *)&D_8021A910, sizeof D_8021A910, 0, 0 },
    { "D_8021A914", (const void *)&D_8021A914, sizeof D_8021A914, 0, 0 },
    { "D_8021A91C", (const void *)&D_8021A91C, sizeof D_8021A91C, 0, 0 },
    { "D_8021A920", (const void *)&D_8021A920, sizeof D_8021A920, 0, 0 },
    { "D_8021A924", (const void *)&D_8021A924, sizeof D_8021A924, 0, 0 },
    { "D_8021AB2C", (const void *)&D_8021AB2C, sizeof D_8021AB2C, 0, 0 },
};

static void piece(FILE *f, const char *name, const void *p, size_t size) {
    fprintf(f, "%s %zu\n", name, size);
    fwrite(p, 1, size, f);
}

void port_dump_game(FILE *f) {
    fprintf(f, "# blastcorps game dump 1: \"name size\\n\" and size bytes, a region or a variable each\n");
    for (unsigned i = 0; i < port_nregions; i++)
        piece(f, port_regions[i].name, port_regions[i].p, port_regions[i].size);
    for (unsigned i = 0; i < sizeof vars / sizeof vars[0]; i++)
        piece(f, vars[i].name, vars[i].p,
              vars[i].count ? (size_t)host_layout(vars[i].layout) * vars[i].count : vars[i].size);
}
