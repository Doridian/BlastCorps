# A check of the macOS build from Linux, without a Mac or its SDK
# (docs/PORT.md, "macOS"): CMake takes the Darwin branches, the N64 side
# goes through the arena link to an arm64 Mach-O object, and the host and
# the glue compile for arm64-apple-macos against another libc's headers
# (musl's for aarch64, say: POSIX, and no __linux__) and the stand-ins in
# macos-check/ for the two SDK headers SDL2 includes.  Nothing links (there
# is no libSystem); port/tools/macos_check.sh links what it can with
# ld64.lld and lists what is left undefined.
#   cmake -S port -B build/port-macos -G Ninja -DPORT_LP64=ON -DPORT_MOVABLE=ON \
#         -DCMAKE_TOOLCHAIN_FILE=port/tools/cross-macos-check.cmake \
#         -DPORT_CHECK_LIBC=/path/to/aarch64-linux-musl/include \
#         -DPORT_BEPASS_PLUGIN=build/port64/libBEPass.so
#   cmake --build build/port-macos --target n64_link host recomp
#   port/tools/macos_check.sh build/port-macos
# SDL2's and libepoxy's headers come from pkg-config: PKG_CONFIG_LIBDIR
# naming a directory with an sdl2.pc and an epoxy.pc whose Cflags point at
# their headers (any architecture's; only not /usr/include itself, which
# pkg-config leaves out and -nostdlibinc drops).
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)
set(CMAKE_OSX_SYSROOT "" CACHE PATH "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "")
set(PORT_CHECK_LIBC "" CACHE PATH "a libc's include directory for the host code's headers")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PORT_CHECK_LIBC)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
foreach(lang C CXX ASM)
    set(CMAKE_${lang}_COMPILER_TARGET arm64-apple-macos11)
    set(CMAKE_${lang}_FLAGS_INIT "-nostdlibinc -isystem ${CMAKE_CURRENT_LIST_DIR}/macos-check -Wno-unused-command-line-argument")
    if(PORT_CHECK_LIBC)
        string(APPEND CMAKE_${lang}_FLAGS_INIT " -isystem ${PORT_CHECK_LIBC}")
    endif()
endforeach()
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
