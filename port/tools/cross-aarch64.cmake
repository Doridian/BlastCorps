# The plain build for 64-bit AArch64 Linux, by zig cc, run under
# qemu-aarch64: cross-zig.cmake.
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(PORT_ZIG_TARGET aarch64-linux-musl)
set(PORT_QEMU qemu-aarch64-static)
include(${CMAKE_CURRENT_LIST_DIR}/cross-zig.cmake)
