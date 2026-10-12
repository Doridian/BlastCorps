# The plain build for 32-bit little-endian MIPS Linux (o32, hard float: the
# PS2's order and pointer size), by zig cc, run under qemu-mipsel:
# cross-zig.cmake.
set(CMAKE_SYSTEM_PROCESSOR mipsel)
set(PORT_ZIG_TARGET mipsel-linux-musleabihf)
set(PORT_QEMU qemu-mipsel-static)
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
