/*
 * The boot: what init's handwritten entry point and boot code do on the N64
 * (clear .bss, inflate hd_code, set up the stack, jump to its entry).  In
 * the port hd_code is already linked in and its data initialized, so this
 * only runs hd_code's entry, func_802447C0, on a thread of its own: it
 * calls osInitialize, creates the idle thread and starts it.
 */
#include "common.h"
#include "port.h"

void func_802447C0(void);

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
    osCreateThread(&boot_thread, 0, boot_entry, NULL, &boot_stack[0x400], 127);
    osStartThread(&boot_thread);
}
