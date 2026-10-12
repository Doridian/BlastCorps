# The plain build for 32-bit big-endian PowerPC Linux (the Wii's CPU's
# order and pointer size), by zig cc, run under qemu-ppc: cross-zig.cmake.
set(CMAKE_SYSTEM_PROCESSOR powerpc)
set(PORT_ZIG_TARGET powerpc-linux-musleabihf)
set(PORT_QEMU qemu-ppc-static)
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
