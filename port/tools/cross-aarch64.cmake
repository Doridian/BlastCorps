# A cross build of the port for AArch64 Linux with clang, e.g. to run under
# qemu-aarch64 (docs/PORT.md, "Other hosts"):
#   cmake -S port -B build/port-a64 -G Ninja -DPORT_64BIT=ON \
#         -DCMAKE_TOOLCHAIN_FILE=port/tools/cross-aarch64.cmake \
#         -DPORT_CROSS_SYSROOT=/usr/aarch64-linux-gnu -DPORT_CROSS_GCC=/usr \
#         -DPORT_BEPASS_PLUGIN=build/port64/libBEPass.so
# PORT_CROSS_SYSROOT is the target's libc (Arch: aarch64-linux-gnu-glibc),
# PORT_CROSS_GCC the prefix holding lib/gcc/aarch64-linux-gnu (its crt
# files and libgcc) and bin/aarch64-linux-gnu-ld; SDL2's aarch64 build is
# found through PKG_CONFIG_LIBDIR/PKG_CONFIG_SYSROOT_DIR.  BEPass has to be
# the host's (from a native build directory).
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(PORT_CROSS_SYSROOT "/usr/aarch64-linux-gnu" CACHE PATH "the target's libc")
set(PORT_CROSS_GCC "/usr" CACHE PATH "the prefix of the aarch64-linux-gnu gcc and binutils")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PORT_CROSS_SYSROOT PORT_CROSS_GCC)
set(CMAKE_SYSROOT ${PORT_CROSS_SYSROOT})
foreach(lang C CXX ASM)
    set(CMAKE_${lang}_COMPILER_TARGET aarch64-linux-gnu)
    set(CMAKE_${lang}_FLAGS_INIT "--gcc-toolchain=${PORT_CROSS_GCC}")
endforeach()
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_EXE_LINKER_FLAGS_INIT "--gcc-toolchain=${PORT_CROSS_GCC} -B${PORT_CROSS_GCC}/bin/aarch64-linux-gnu-")
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-aarch64-static -L ${PORT_CROSS_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
