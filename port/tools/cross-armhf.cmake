# The plain build for 32-bit little-endian ARM Linux (hard float), by zig
# cc, run under qemu-arm: cross-zig.cmake.
set(CMAKE_SYSTEM_PROCESSOR arm)
set(PORT_ZIG_TARGET arm-linux-musleabihf)
set(PORT_QEMU qemu-arm-static)
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
