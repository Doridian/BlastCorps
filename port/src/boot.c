/*
 * The boot: what init's handwritten entry point and boot code do on the N64
 * (clear .bss, inflate hd_code, set up the stack, jump to its entry).  In
 * the port hd_code is already linked in and its data initialized, so this
 * only runs hd_code's entry, func_802447C0, on a thread of its own: it
 * calls osInitialize, creates the idle thread and starts it.
 *
 * What init hands over to hd_code is left where it leaves it: the front
 * end's ROM range (its compressed .text and .data, up to the trailer) in
 * the words at the top of RDRAM, the region D_803FFFF8 (init's
 * func_80220730 writes them after clearing everything from hd_code's .bss
 * up).  func_8028B3E0 (src/overlay.c) loads the front end from there, for
 * the time that takes.
 */
#include "common.h"
#include "functions.h"
#include "port.h"

/* the region (port_regions.h), and the original ROM's positions
   (port_syms.ld: tools/gen_syms.py script) */
extern u32 D_803FFFF8, D_803FFFFC;
extern u8 hd_front_end_text_ROM_START[], trailer_ROM_START[];

static OSThread boot_thread;
static u64 boot_stack[0x400];

static void boot_entry(void *arg) {
    func_802447C0();
    /* on the N64 the boot "thread" is gone once the idle thread starts */
}

void port_overlay_init(void);

void port_native_fixups(void);

void port_boot(void) {
#ifdef PORT_NATIVE_ENDIAN
    port_native_fixups();
#endif
    port_overlay_init();
    D_803FFFF8 = (u32)(uintptr_t)hd_front_end_text_ROM_START;
    D_803FFFFC = (u32)(uintptr_t)trailer_ROM_START;
    osCreateThread(&boot_thread, 0, boot_entry, NULL, &boot_stack[0x400], 127);
    osStartThread(&boot_thread);
}
