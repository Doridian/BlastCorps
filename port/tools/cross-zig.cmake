# The port cross-compiled with zig cc, a static musl executable that
# qemu-user runs (docs/PORT.md, "Other hosts"); the targets' own files set
# PORT_ZIG_TARGET and PORT_QEMU and include this (cross-ppc.cmake,
# cross-mipsel.cmake, cross-armhf.cmake, cross-aarch64.cmake):
#   cmake -S port -B build/plain-ppc -G Ninja -DPORT_VERSION=us.v10 \
#         -DCMAKE_TOOLCHAIN_FILE=port/tools/cross-ppc.cmake
#   port/tools/test.py quick build/plain-ppc          (under qemu: the emulator below)
# zig is $ZIG, or zig on the PATH, or ~/.local/opt/zig/zig (one tarball from
# ziglang.org unpacked; no root, no sysroot: zig carries musl).  No SDL
# there: the build is headless (PORT_SDL=OFF, host/nosdl/SDL.h); musl has
# no ucontext, so the game's threads are host threads (PORT_THREADS=pthread).
set(CMAKE_SYSTEM_NAME Linux)
if(NOT PORT_ZIG)
    if(DEFINED ENV{ZIG})
        set(PORT_ZIG $ENV{ZIG})
    else()
        find_program(PORT_ZIG zig HINTS $ENV{HOME}/.local/opt/zig NO_CACHE)
    endif()
    if(NOT PORT_ZIG)
        message(FATAL_ERROR "no zig: set ZIG=/path/to/zig, or unpack ziglang.org's tarball as ~/.local/opt/zig")
    endif()
    set(PORT_ZIG ${PORT_ZIG} CACHE FILEPATH "zig, whose cc cross-compiles the port")
endif()
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PORT_ZIG PORT_ZIG_TARGET)
set(CMAKE_C_COMPILER ${PORT_ZIG} cc)
set(CMAKE_CXX_COMPILER ${PORT_ZIG} c++)
set(CMAKE_ASM_COMPILER ${PORT_ZIG} cc)
foreach(lang C CXX ASM)
    set(CMAKE_${lang}_COMPILER_TARGET ${PORT_ZIG_TARGET})
endforeach()
# (zig cc's -print-resource-dir names a directory it doesn't have: its own
# stddef.h and the like are in its lib/include)
get_filename_component(PORT_ZIG_REAL ${PORT_ZIG} REALPATH)
get_filename_component(PORT_ZIG_DIR ${PORT_ZIG_REAL} DIRECTORY)
set(PORT_CC_INCLUDE ${PORT_ZIG_DIR}/lib/include CACHE PATH "the C compiler's own include directory")
# (the objects' symbols for gen_fevars.py: LLVM's nm reads any target's)
find_program(CMAKE_NM NAMES llvm-nm REQUIRED)
set(PORT_SDL OFF CACHE BOOL "the host's window, input and sound through SDL2")
# (cached: port/tools/test.py runs the build under it)
set(CMAKE_CROSSCOMPILING_EMULATOR ${PORT_QEMU} CACHE STRING "what runs the target's programs here")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
