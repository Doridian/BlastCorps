# The PC port of Blast Corps.  See docs/PORT.md.
#
# Needs a built stage 2 of PORT_VERSION (default us.v11; us.v10, jp and eu
# build too):
#   make VERSION=us.v11 -C blastcorps
# then
#   cmake -S port -B build/port && cmake --build build/port -j
#
# The game's C, port/src, port/libaudio and port/engine are ordinary C,
# compiled by the host's C compiler (clang or gcc) in the host's byte order
# and pointer size, and linked with the host (port/host) into an ordinary
# program (a PIE): a pointer is a host pointer and a variable is wherever
# the linker puts it.
cmake_minimum_required(VERSION 3.20)
project(blastcorps_pc C)
set(PORT_VERSION us.v11 CACHE STRING "the version to build: us.v11, us.v10, jp or eu")
set_property(CACHE PORT_VERSION PROPERTY STRINGS us.v11 us.v10 jp eu)
if(NOT PORT_VERSION MATCHES "^(us\\.v1[01]|jp|eu)$")
    message(FATAL_ERROR "PORT_VERSION is us.v11, us.v10, jp or eu, not ${PORT_VERSION}")
endif()
string(TOUPPER "VERSION_${PORT_VERSION}" PORT_VERSION_DEFINE)
string(REPLACE "." "_" PORT_VERSION_DEFINE ${PORT_VERSION_DEFINE})
# what a local nothing set holds (the game's C reads some: 1h's maybe-uninitialized):
# zero, as the build has always had it, or pattern, to find what reads one (a run
# whose hashes move with the pattern read an uninitialized local)
set(PORT_AUTO_INIT zero CACHE STRING "-ftrivial-auto-var-init= of the N64 side: zero or pattern")
set_property(CACHE PORT_AUTO_INIT PROPERTY STRINGS zero pattern)
# the sanitizers, for clang and gcc: -DPORT_SANITIZE=address,undefined (its
# -fsanitize list; every report stops the program, as the gate wants:
# -DPORT_SANITIZE_RECOVER=ON for the reports to go on)
set(PORT_SANITIZE "" CACHE STRING "the -fsanitize= list, for the game and the host")
option(PORT_SANITIZE_RECOVER "a sanitizer's report doesn't stop the program" OFF)
option(PORT_SDK_TRACE "port/src/gu.c logs its calls to $PORT_SDK_TRACE (port/tools/sdk_check)" OFF)
set(PORT_SAN_FLAGS)
if(PORT_SANITIZE)
    set(PORT_SAN_FLAGS -fsanitize=${PORT_SANITIZE} -fno-omit-frame-pointer)
    if(NOT PORT_SANITIZE_RECOVER)
        list(APPEND PORT_SAN_FLAGS -fno-sanitize-recover=all)
    endif()
    message(STATUS "sanitizers: ${PORT_SANITIZE}")
endif()
# WebAssembly (emcmake cmake ...; docs/PORT.md, "WebAssembly"): emcc
# compiles everything, the host side's choices below (the page or node, the
# fibers) and the link's options are the build's; the N64 side's compile
# under emcc isn't done yet (P10's packet H)
if(EMSCRIPTEN)
    message(FATAL_ERROR "the port isn't for WebAssembly yet (emcc compiling the N64 side: P10, packet H)")
    # node: headless, the files through node's (NODERAWFS); web: a page
    set(PORT_WASM_TARGET node CACHE STRING "the WebAssembly build's host: node or web")
    set_property(CACHE PORT_WASM_TARGET PROPERTY STRINGS node web)
    if(NOT PORT_WASM_TARGET MATCHES "^(node|web)$")
        message(FATAL_ERROR "PORT_WASM_TARGET is node or web, not ${PORT_WASM_TARGET}")
    endif()
    # the fibers: Asyncify's on the one thread (the default: no
    # SharedArrayBuffer, and faster than handing the CPU between workers),
    # or host threads (a worker each; node only, as things are)
    set(PORT_THREADS asyncify CACHE STRING "the game's threads: asyncify or pthread")
    if(PORT_THREADS STREQUAL "pthread" AND PORT_WASM_TARGET STREQUAL "web")
        message(FATAL_ERROR "the page's build is PORT_THREADS=asyncify (the loop can't block its thread)")
    endif()
    if(PORT_THREADS STREQUAL "pthread")
        set(WASM_PTHREAD -pthread)
    elseif(PORT_THREADS STREQUAL "asyncify")
        set(WASM_PTHREAD)
    else()
        message(FATAL_ERROR "PORT_THREADS is pthread or asyncify in the WebAssembly build, not ${PORT_THREADS}")
    endif()
endif()

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(BLAST ${ROOT}/blastcorps)
set(GEN ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN})
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# LLVM's tools, next to the compiler first (Homebrew's LLVM isn't on PATH)
get_filename_component(CLANG_BINDIR ${CMAKE_C_COMPILER} DIRECTORY)
# the N64 link's ELFs are read with MIPS binutils, or LLVM's, which read any
find_program(N64_NM NAMES mips-linux-gnu-nm llvm-nm HINTS ${CLANG_BINDIR} REQUIRED)
find_program(N64_READELF NAMES mips-linux-gnu-readelf llvm-readelf HINTS ${CLANG_BINDIR} REQUIRED)
# the generators read the stage-2 build of PORT_VERSION
set(PY ${CMAKE_COMMAND} -E env PORT_VERSION=${PORT_VERSION} PORT_N64_NM=${N64_NM} ${Python3_EXECUTABLE})

if(NOT EXISTS ${BLAST}/build/hd_code.${PORT_VERSION}.elf)
    message(FATAL_ERROR "${BLAST}/build/hd_code.${PORT_VERSION}.elf is missing: build blastcorps (${PORT_VERSION}) first")
endif()
# blastcorps/asm holds one version at a time
file(READ ${BLAST}/.version STAGE2_VERSION)
string(STRIP "${STAGE2_VERSION}" STAGE2_VERSION)
if(NOT STAGE2_VERSION STREQUAL PORT_VERSION)
    message(FATAL_ERROR "blastcorps/ is extracted for ${STAGE2_VERSION}, not ${PORT_VERSION}")
endif()

# the ROM this port is for, exactly (host/main.c checks it at startup)
file(READ ${ROOT}/blastcorps.${PORT_VERSION}.sha1 PORT_ROM_SHA1)
string(STRIP "${PORT_ROM_SHA1}" PORT_ROM_SHA1)

# Addresses that differ between versions, from the version's N64 link: the
# end of hd_code's .bss, where segment 1's static data starts (D_803FF600,
# src/regions.c), and the front end's .data and .bss (src/overlay.c)
execute_process(COMMAND ${N64_NM} ${BLAST}/build/hd_code.${PORT_VERSION}.elf
    OUTPUT_VARIABLE HD_CODE_NM COMMAND_ERROR_IS_FATAL ANY)
if(NOT HD_CODE_NM MATCHES "([0-9a-f]+) B D_803FF600\n")
    message(FATAL_ERROR "no D_803FF600 in hd_code.${PORT_VERSION}.elf")
endif()
set(PORT_STATIC_START 0x${CMAKE_MATCH_1})
execute_process(COMMAND ${N64_READELF} -SW ${BLAST}/build/hd_front_end.${PORT_VERSION}.elf
    OUTPUT_VARIABLE FE_SECTIONS COMMAND_ERROR_IS_FATAL ANY)
foreach(sec data bss)
    string(TOUPPER ${sec} SEC)
    if(NOT FE_SECTIONS MATCHES "\\.hd_front_end_${sec} +[A-Z]+ +([0-9a-f]+)")
        message(FATAL_ERROR "no .hd_front_end_${sec} in hd_front_end.${PORT_VERSION}.elf")
    endif()
    set(PORT_FE_${SEC} 0x${CMAKE_MATCH_1})
endforeach()
if(NOT FE_SECTIONS MATCHES "\\.hd_front_end +[A-Z]+ +([0-9a-f]+)")
    message(FATAL_ERROR "no .hd_front_end in hd_front_end.${PORT_VERSION}.elf")
endif()
set(PORT_FE_TEXT 0x${CMAKE_MATCH_1})
# The ROM addresses the game's C names (game/types.h's ROM(sym))
execute_process(COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py romaddrs ${GEN}/rom_addrs.h
        COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py
    ${BLAST}/build/rom.${PORT_VERSION}.ld ${BLAST}/build/rom_fixed.${PORT_VERSION}.ld
    ${BLAST}/build/hd_code.${PORT_VERSION}.syms.ld ${BLAST}/build/hd_front_end.${PORT_VERSION}.syms.ld
    ${BLAST}/build/init.${PORT_VERSION}.syms.ld)
# the type inventory: the version's own where its layouts differ (eu's)
if(EXISTS ${BLAST}/include/game/inventory.${PORT_VERSION}.json)
    set(PORT_INVENTORY ${BLAST}/include/game/inventory.${PORT_VERSION}.json)
else()
    set(PORT_INVENTORY ${BLAST}/include/game/inventory.json)
endif()
message(STATUS "${PORT_VERSION}: static data from ${PORT_STATIC_START}, front end .data ${PORT_FE_DATA}, .bss ${PORT_FE_BSS}")

# the compiler's own headers (stddef.h, stdarg.h: the game's C includes no
# libc's), clang's resource dir or gcc's include dir, or where a toolchain
# file says (zig cc's are in its lib/include: port/tools/cross-zig.cmake)
set(PORT_CC_INCLUDE "" CACHE PATH "the C compiler's own include directory, where it doesn't say itself")
if(PORT_CC_INCLUDE)
    set(N64_CC_INCLUDE ${PORT_CC_INCLUDE})
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-resource-dir OUTPUT_VARIABLE CLANG_RESOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(N64_CC_INCLUDE ${CLANG_RESOURCE_DIR}/include)
else()
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=include OUTPUT_VARIABLE N64_CC_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
# an ordinary program, position-independent
set(N64_ARCH -fPIE)
if(EMSCRIPTEN)
    set(HOST_ARCH ${WASM_PTHREAD})
else()
    set(HOST_ARCH -fPIE)
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i.86|AMD64)")
    # i386 (CMAKE_C_FLAGS=-m32): the float arithmetic in SSE, as the
    # N64's and every other host's is (x87 would round differently),
    # and doubles and long longs 8-aligned, as MIPS o32 has them
    # (the structures' sizes the game's C checks)
    list(APPEND N64_ARCH -msse2 -mfpmath=sse -malign-double)
    list(APPEND HOST_ARCH -msse2 -mfpmath=sse -malign-double)
endif()
message(STATUS "${CMAKE_SIZEOF_VOID_P}-byte pointers")
# (for port/tools: test.py, n64_tus.py)
set(PORT_PTR_SIZE ${CMAKE_SIZEOF_VOID_P} CACHE INTERNAL "the host's pointer size")

# ---- the game's C -------------------------------------------------------------
file(GLOB GAME_C ${BLAST}/src/hd_code/*.c ${BLAST}/src/hd_front_end/*.c)

# libultra's OS and I/O (the stubs that include only src/libultra/os and
# io, plus libc's printf and string functions): port/src replaces them.
set(REPLACED
    hd_code/8F860 hd_code/8FAF0 hd_code/8FC40 hd_code/8FDA0 hd_code/8FF20 hd_code/90150
    hd_code/90290 hd_code/90660 hd_code/91D20 hd_code/91D90 hd_code/92030 hd_code/922A0
    hd_code/95110 hd_code/95240 hd_code/95450 hd_code/95B30 hd_code/95C40 hd_code/95E50
    hd_code/961B0 hd_code/96220 hd_code/96290 hd_code/96300 hd_code/96310 hd_code/964C0
    hd_code/96800 hd_code/968E0 hd_code/96D10 hd_code/97090 hd_code/97200 hd_code/97BB0
    hd_code/97BF0 hd_code/97CD0 hd_code/97F30 hd_code/983F0 hd_code/99580 hd_code/9B0E0
    hd_code/9B440 hd_code/9D9E0 hd_code/9DB60 hd_code/9DB70 hd_code/9DE90 hd_code/9DEA0
    hd_code/9DEE0 hd_code/9DF70 hd_code/9DFA0 hd_code/9E050 hd_code/9E140 hd_code/9E5C0
    hd_code/9E700 hd_code/A0A40 hd_code/A0AA0 hd_code/90C50
    hd_front_end/1B730 hd_front_end/1B880 hd_front_end/1B8F0 hd_front_end/1CCC0
    hd_front_end/1D2D0 hd_front_end/1D410 hd_front_end/1D550 hd_front_end/1D710
    hd_front_end/1DC10 hd_front_end/1E3F0 hd_front_end/1F5C0 hd_front_end/1F970
    hd_front_end/1FD20)
# the SDK's graphics utilities, sine tables and libc pieces: the port's own
# (port/src/gu.c; docs/DISTRIBUTION.md, "Replacing the SDK parts")
set(REPLACED_SDK
    hd_code/90760 hd_code/90800 hd_code/909C0 hd_code/90E40 hd_code/91050 hd_code/912C0
    hd_code/91360 hd_code/91830 hd_code/92130 hd_code/950E0 hd_code/95070 hd_code/97140)
# libaudio (docs/PORT.md, "libaudio"): the port's own (port/libaudio, below);
# the decompiled original (src/libultra/audio, through these objects) is the
# libaudio oracle's (port/tools/audio_oracle builds it itself)
set(LIBAUDIO_ORIG
    hd_code/90390 hd_code/92350 hd_code/923B0 hd_code/926F0 hd_code/927B0 hd_code/92800
    hd_code/92A30 hd_code/92AD0 hd_code/92B60 hd_code/92BF0 hd_code/92C80 hd_code/92D20
    hd_code/92DA0 hd_code/92F00 hd_code/92F30 hd_code/939F0 hd_code/93A30 hd_code/93AA0
    hd_code/93B50 hd_code/93B60 hd_code/93BA0 hd_code/93DF0 hd_code/95020 hd_code/95460
    hd_code/99620 hd_code/996A0 hd_code/996B0 hd_code/9AFF0 hd_code/9B470 hd_code/9BBF0
    hd_code/9BC90 hd_code/9BDE0 hd_code/9C930 hd_code/9CC30 hd_code/9D810 hd_code/9D920
    hd_code/9F7D0 hd_code/9FDF0 hd_code/9FE00 hd_code/9FE20)
foreach(r ${REPLACED} ${REPLACED_SDK} ${LIBAUDIO_ORIG})
    list(REMOVE_ITEM GAME_C ${BLAST}/src/${r}.c)
endforeach()

# The SDK's headers are the port's own (port/include/sdk); PORT_SDK_2_0I puts
# the decompilation's 2.0I back, for port/tools/sdk_identity.py, which checks
# that every object comes out the same with either.
option(PORT_SDK_2_0I "the N64 side with blastcorps/include/2.0I instead of port/include/sdk" OFF)
if(PORT_SDK_2_0I)
    set(SDK_INCLUDES ${BLAST}/include/2.0I ${BLAST}/include/2.0I/PR)
else()
    set(SDK_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk/PR)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk/PR/libaudio.h)
        # (until the port's own libaudio.h is there: 2.0I's, over these)
        file(WRITE ${GEN}/sdk_libaudio/PR/libaudio.h.tmp "#include \"${BLAST}/include/2.0I/PR/libaudio.h\"\n")
        file(COPY_FILE ${GEN}/sdk_libaudio/PR/libaudio.h.tmp ${GEN}/sdk_libaudio/PR/libaudio.h ONLY_IF_DIFFERENT)
        list(APPEND SDK_INCLUDES ${GEN}/sdk_libaudio ${GEN}/sdk_libaudio/PR)
    endif()
endif()
set(N64_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${GEN}
    ${BLAST} ${BLAST}/include ${SDK_INCLUDES})
set(N64_DEFINES _LANGUAGE_C _FINALROM TARGET_PC ${PORT_VERSION_DEFINE} _MIPS_SZLONG=32 _MIPS_SZINT=32)
# The decompiled C is IDO's dialect: pointers and ints used
# interchangeably, narrow and wide types mixed.  These flags make the
# compiler accept it and not optimise on the undefined behaviour it is full
# of.  (-fsigned-char: IDO's -signed, which AArch64 Linux doesn't have by
# default.)  Its functions are declared once, with their definitions'
# types (blastcorps/include/functions.h), and that stays so: a declaration
# or definition without a prototype, a call without one, an implicit
# declaration and a non-void function that doesn't return are errors (two
# declarations that disagree are anyway).  Where a pointer can be 8 bytes,
# an implicit pointer/integer conversion or a pointer to one type passed as
# a pointer to another is where a 4-byte slot can meet an 8-byte pointer
# (docs/PORT.md, "The LP64 build"): errors, on the whole N64 side.  An
# explicit cast is allowed; ptrcast_scan.py counts those against its
# allow-list.  A function passed as a pointer to another type of function
# (clang's subgroup of the second) is one too.  (-fno-vectorize: the code
# the references were made with; the vectorizer's would be the same
# arithmetic.)
set(N64_FLAGS ${N64_ARCH} -std=gnu89 -O2 -g -fsigned-char -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks
    -ftrivial-auto-var-init=${PORT_AUTO_INIT} -ffp-contract=off -fno-common -fno-builtin
    -nostdinc -isystem ${N64_CC_INCLUDE}
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_game.h
    -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
    -Wno-unused-label -Wno-unused-value -Wno-missing-braces -Wno-pointer-sign
    -Wno-parentheses -Wno-dangling-else
    -Werror=deprecated-non-prototype -Werror=strict-prototypes -Werror=implicit-function-declaration
    -Werror=return-type -Werror=return-mismatch
    -Werror=int-conversion -Werror=incompatible-pointer-types
    -Wno-incompatible-library-redeclaration -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
    -Wno-implicit-int -Wno-uninitialized -Wno-sometimes-uninitialized
    -Wno-comment -Wno-tautological-constant-out-of-range-compare
    -Wno-constant-conversion -Wno-switch -Wno-array-bounds -Wno-shift-count-overflow
    -Wno-self-assign -Wno-parentheses-equality -Wno-tautological-compare -Wno-empty-body
    -Wno-unknown-pragmas -Wno-main-return-type -Wno-shift-negative-value -Wno-builtin-requires-header
    -Werror=incompatible-function-pointer-types
    -Wno-implicit-const-int-float-conversion -Wno-pointer-bool-conversion
    -Wno-string-plus-int -Wno-null-pointer-arithmetic -Wno-gnu-folding-constant
    -Wno-macro-redefined -Wno-ignored-pragma-intrinsic)   # (IDO's sqrtf, not one on AArch64)
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    list(APPEND N64_FLAGS -fno-vectorize -fno-slp-vectorize)
else()
    # gcc: the options of the list that are clang's are taken out (it
    # rejects the -Werror ones it doesn't know, and says so of any -Wno- one
    # beside a diagnostic), gcc's own put in
    set(N64_CLANG_ONLY -Werror=deprecated-non-prototype -Wno-incompatible-library-redeclaration
        -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
        -Wno-tautological-constant-out-of-range-compare -Wno-constant-conversion -Wno-self-assign
        -Wno-parentheses-equality -Wno-main-return-type -Wno-builtin-requires-header
        -Werror=incompatible-function-pointer-types -Wno-implicit-const-int-float-conversion
        -Wno-pointer-bool-conversion -Wno-string-plus-int -Wno-null-pointer-arithmetic
        -Wno-gnu-folding-constant -Wno-macro-redefined -Wno-ignored-pragma-intrinsic
        -Wno-sometimes-uninitialized -Werror=return-mismatch)
    list(REMOVE_ITEM N64_FLAGS ${N64_CLANG_ONLY})
    # (-ffreestanding: gcc's stdint.h is its own then, not libc's, which
    # -nostdinc keeps out)
    list(APPEND N64_FLAGS -fno-tree-vectorize -ffreestanding
        -Wno-builtin-declaration-mismatch -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
        -Wno-misleading-indentation -Wno-maybe-uninitialized
        -Wno-stringop-overflow -Wno-address -Wno-format -Wno-overflow -Wno-restrict
        -Wno-unterminated-string-initialization)
endif()
list(APPEND N64_FLAGS ${PORT_SAN_FLAGS})

# the port's own libaudio (N64 side)
file(GLOB LIBAUDIO_C ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/*.c)
add_library(libaudio_c OBJECT ${LIBAUDIO_C})
target_include_directories(libaudio_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libaudio ${N64_INCLUDES})
target_compile_definitions(libaudio_c PRIVATE ${N64_DEFINES})
target_compile_options(libaudio_c PRIVATE ${N64_FLAGS})

# (the front end is compiled twice: below, "the front end's variables")
set(FE_SRC)
foreach(f ${GAME_C})
    if(f MATCHES "/src/hd_front_end/[^/]*$")
        list(APPEND FE_SRC ${f})
    endif()
endforeach()
list(REMOVE_ITEM GAME_C ${FE_SRC})
add_library(game_c OBJECT ${GAME_C})
target_compile_options(game_c PRIVATE -fdata-sections)
target_include_directories(game_c PRIVATE ${N64_INCLUDES})
target_compile_definitions(game_c PRIVATE ${N64_DEFINES})
target_compile_options(game_c PRIVATE ${N64_FLAGS})

# ---- the engine (N64 side; docs/PORT.md, "Replacing the engine") ------------
# Rare's handwritten functions as native C (port/engine), built like the
# game's C.
file(GLOB ENGINE_C ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.c)
add_library(engine_c OBJECT ${ENGINE_C})
target_include_directories(engine_c PRIVATE ${N64_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_compile_definitions(engine_c PRIVATE ${N64_DEFINES})
target_compile_options(engine_c PRIVATE ${N64_FLAGS})

# ---- the libultra replacement (N64 side) ---------------------------------------
file(GLOB PORT_N64_C ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_library(port_n64 OBJECT ${PORT_N64_C})
target_include_directories(port_n64 PRIVATE ${N64_INCLUDES})
target_compile_definitions(port_n64 PRIVATE ${N64_DEFINES})
target_compile_options(port_n64 PRIVATE ${N64_FLAGS})
target_compile_definitions(port_n64 PRIVATE PORT_FE_TEXT=${PORT_FE_TEXT} PORT_FE_DATA=${PORT_FE_DATA} PORT_FE_BSS=${PORT_FE_BSS}
    PORT_STATIC_START=${PORT_STATIC_START}u)
# The functions port/src stands in front of (the overlay, the loaders, the
# save's CRC): their defining files have them as __real_<name>, and port/src
# defines <name> itself (overlay.c, loads.c), which is what every other file
# calls.  A file's own calls stay its own, as a link's --wrap leaves them;
# any compiler, any linker.  (The replay's two audio hooks are functions.h's:
# replay_hooks.c.)
set_property(SOURCE ${BLAST}/src/hd_code/46C20.c APPEND PROPERTY COMPILE_DEFINITIONS
    func_8028B3E0=__real_func_8028B3E0 func_8028B4C4=__real_func_8028B4C4)
set_property(SOURCE ${BLAST}/src/hd_code/17A70.c APPEND PROPERTY COMPILE_DEFINITIONS
    func_8025C230=__real_func_8025C230)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/gu.c APPEND PROPERTY COMPILE_DEFINITIONS
    __osContDataCrc=__real___osContDataCrc)
# sins()'s table, generated
add_custom_command(OUTPUT ${GEN}/sdk/sintable.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py ${GEN}/sdk/sintable.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py VERBATIM)
file(MAKE_DIRECTORY ${GEN}/sdk)
target_include_directories(port_n64 PRIVATE ${GEN}/sdk)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/gu.c PROPERTIES
    OBJECT_DEPENDS "${GEN}/sdk/sintable.h")
if(PORT_SDK_TRACE)
    target_compile_definitions(port_n64 PRIVATE PORT_SDK_TRACE)
endif()

# ---- the game's data: the handwritten objects', as C -------------------------
# (what the N64 build links from asm/data, which holds their .bss too, and
# the data islands; tools/asm2c.py types it, from the type inventory, and
# the C is built like the game's: docs/PORT.md, "The asm data as C")
file(GLOB DATA_S ${BLAST}/asm/data/hd_code/*.s ${BLAST}/asm/data/hd_front_end/*.s)
# libultra's exception handler's jump table (its code is replaced)
list(REMOVE_ITEM DATA_S ${BLAST}/asm/data/hd_code/972A0.rodata.s)
set(DATA_BIN hd_code/68810 hd_code/690C0 hd_code/8E910 hd_code/91D50 hd_code/A0C30
             hd_code/C9BD0 hd_code/CA3D0 hd_code/CA6A0 hd_front_end/20090 hd_front_end/29690)
set(DATA_C ${GEN}/data_c/asm_data.h)
set(DATA_IN)
set(DATA_DEPS)
foreach(s ${DATA_S})
    file(RELATIVE_PATH rel ${BLAST}/asm ${s})
    string(REPLACE "/" "_" out ${rel})
    string(REGEX REPLACE "\\.s$" ".c" out ${out})
    list(APPEND DATA_C ${GEN}/data_c/${out})
    list(APPEND DATA_IN ${s})
    list(APPEND DATA_DEPS ${s})
endforeach()
# the data of the IDO functions the version still has as asm (its GLOBAL_ASM):
# their strings, floats and jump tables (tools/recomp/config.py lists them)
execute_process(COMMAND ${Python3_EXECUTABLE} -c
    "import sys; sys.path.insert(0, '${ROOT}/tools/recomp'); import config; print(';'.join(f'{m}/{o}/{f}' for m, o, f in config.global_asm_functions('${PORT_VERSION}')))"
    OUTPUT_VARIABLE GLOBAL_ASM_FUNCS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
foreach(g ${GLOBAL_ASM_FUNCS})
    string(REPLACE "/" "_" out ${g})
    list(APPEND DATA_C ${GEN}/data_c/global_asm_${out}.c)
    list(APPEND DATA_IN --global-asm ${BLAST}/asm/nonmatchings/${g}.s)
    list(APPEND DATA_DEPS ${BLAST}/asm/nonmatchings/${g}.s)
endforeach()
foreach(b ${DATA_BIN})
    string(REPLACE "/" "_" label ${b})
    list(APPEND DATA_C ${GEN}/data_c/${label}_bin.c)
    list(APPEND DATA_IN ${label}_bin=${BLAST}/assets/${b}.bin)
    list(APPEND DATA_DEPS ${BLAST}/assets/${b}.bin)
endforeach()
add_custom_command(OUTPUT ${DATA_C}
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2c.py ${GEN}/data_c ${DATA_IN}
    DEPENDS ${DATA_DEPS} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2c.py ${PORT_INVENTORY}
            ${BLAST}/build/hd_code.${PORT_VERSION}.elf ${BLAST}/build/hd_front_end.${PORT_VERSION}.elf
    VERBATIM)
file(MAKE_DIRECTORY ${GEN}/data_c)
set_source_files_properties(${DATA_C} PROPERTIES GENERATED TRUE)
foreach(f ${DATA_C})
    get_filename_component(b ${f} NAME)
    if(b MATCHES "hd_front_end.*\\.c$")
        list(APPEND FE_SRC ${f})
        list(REMOVE_ITEM DATA_C ${f})
    endif()
endforeach()
add_library(game_data OBJECT ${DATA_C})
target_compile_options(game_data PRIVATE -fdata-sections)
target_include_directories(game_data PRIVATE ${GEN}/data_c ${N64_INCLUDES})
target_compile_definitions(game_data PRIVATE ${N64_DEFINES})
target_compile_options(game_data PRIVATE ${N64_FLAGS})

# ---- the front end's variables -------------------------------------------------
# which src/overlay.c puts back as they were at startup whenever the game
# loads the front end: every variable its sources (its C and its asm data)
# define, with the address and size the compiler gives them
# (tools/gen_fevars.py).  The sources are compiled as they are for their
# symbol tables (fe_scan, which isn't linked), then again, each #included by
# a file of gen/fe/ followed by its table (fe_c).
add_library(fe_scan OBJECT ${FE_SRC})
target_compile_options(fe_scan PRIVATE -fdata-sections)
target_include_directories(fe_scan PRIVATE ${GEN}/data_c ${N64_INCLUDES})
target_compile_definitions(fe_scan PRIVATE ${N64_DEFINES})
target_compile_options(fe_scan PRIVATE ${N64_FLAGS})
set(FE_WRAPPED ${GEN}/fe/fe_vars.c)
foreach(f ${FE_SRC})
    get_filename_component(b ${f} NAME)
    list(APPEND FE_WRAPPED ${GEN}/fe/${b})
endforeach()
add_custom_command(OUTPUT ${FE_WRAPPED}
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_fevars.py ${GEN}/fe --nm ${CMAKE_NM}
            --sources "${FE_SRC}" --objects "$<TARGET_OBJECTS:fe_scan>"
    DEPENDS fe_scan $<TARGET_OBJECTS:fe_scan>
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_fevars.py ${BLAST}/build/hd_front_end.${PORT_VERSION}.elf
    COMMAND_EXPAND_LISTS)
add_library(fe_c OBJECT ${FE_WRAPPED})
target_compile_options(fe_c PRIVATE -fdata-sections)
target_include_directories(fe_c PRIVATE ${GEN}/data_c ${N64_INCLUDES})
target_compile_definitions(fe_c PRIVATE ${N64_DEFINES})
target_compile_options(fe_c PRIVATE ${N64_FLAGS})

# ---- host side -----------------------------------------------------------------
set(HOST_FLAGS ${HOST_ARCH} -O2 -g -ffp-contract=off -fno-strict-aliasing -Wall ${PORT_SAN_FLAGS})
if(EMSCRIPTEN)
    # emscripten's SDL2 port
    set(SDL2_CFLAGS_OTHER -sUSE_SDL=2)
    set(SDL2_LDFLAGS -sUSE_SDL=2)
else()
# -DPORT_SDL=OFF: no SDL, and so no window, no OpenGL, no sound device and
# no input but the replay's: headless runs only, as under qemu
# (port/tools/cross-*.cmake; host/nosdl/SDL.h)
option(PORT_SDL "the host's window, input and sound through SDL2" ON)
if(NOT PORT_SDL)
    set(SDL2_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/host/nosdl)
    set(SDL2_CFLAGS_OTHER)
    set(SDL2_LDFLAGS)
    message(STATUS "no SDL: headless only")
else()
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
# the OpenGL renderer (gfx_gl.c) is optional: without libepoxy, only the
# software one is built
pkg_check_modules(EPOXY epoxy)
endif()
endif()
file(GLOB HOST_C ${CMAKE_CURRENT_SOURCE_DIR}/host/*.c)
# --hd-text's built-in font (host/hdtext.c; SIL OFL 1.1, port/fonts/OFL.txt)
set(HDTEXT_FONT ${CMAKE_CURRENT_SOURCE_DIR}/fonts/StardosStencil-Bold.ttf)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gen/hdtext_font.c
    COMMAND ${CMAKE_COMMAND} -DIN=${HDTEXT_FONT} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/gen/hdtext_font.c
            -DNAME=hdtext_builtin_ttf -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.cmake
    DEPENDS ${HDTEXT_FONT} ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.cmake)
# resource packs (host/pack.c): gzip 1.2.4's deflate (GPLv2+), which alone
# gives back the ROM's gzip members, and stb_image (host/pack_stb.c)
set(GZIP124 ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gzip-1.2.4)
set(GZIP124_C ${GZIP124}/gzip124.c ${GZIP124}/deflate.c ${GZIP124}/trees.c ${GZIP124}/bits.c)
set(GZIP124_STD gnu89)
if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(GZIP124_STD gnu99)      # (gcc's gnu89 has no for (int i...))
endif()
set_source_files_properties(${GZIP124_C} PROPERTIES COMPILE_OPTIONS "-w;-std=${GZIP124_STD};-include;${GZIP124}/gzip124_names.h")
add_library(host OBJECT ${HOST_C} ${GZIP124_C} ${CMAKE_CURRENT_BINARY_DIR}/gen/hdtext_font.c)
# the game's threads' context switch (host/fiber.h): ucontext on the one OS
# thread, or a host thread each, one running at a time.  ucontext is the
# cheaper switch; macOS has deprecated it and WebAssembly and musl have
# none.  Where both are built the environment's PORT_THREADS picks one at
# startup.
include(CheckSymbolExists)
if(NOT APPLE AND NOT EMSCRIPTEN)
    check_symbol_exists(swapcontext ucontext.h PORT_UCONTEXT_FOUND)    # (musl has none)
endif()
if(PORT_UCONTEXT_FOUND)
    set(PORT_THREADS_DEFAULT ucontext)
    target_compile_definitions(host PRIVATE PORT_HAVE_UCONTEXT)
else()
    set(PORT_THREADS_DEFAULT pthread)
endif()
set(PORT_THREADS ${PORT_THREADS_DEFAULT} CACHE STRING "the game's threads by default: ucontext or pthread")
set_property(CACHE PORT_THREADS PROPERTY STRINGS ucontext pthread)
if(EMSCRIPTEN AND PORT_THREADS STREQUAL "asyncify")
    target_compile_definitions(host PRIVATE PORT_HAVE_ASYNCIFY)
    add_library(Threads::Threads INTERFACE IMPORTED)
else()
find_package(Threads REQUIRED)
target_compile_definitions(host PRIVATE PORT_HAVE_PTHREAD)
endif()
if(EMSCRIPTEN AND PORT_THREADS STREQUAL "asyncify")     # (checked above)
elseif(PORT_THREADS STREQUAL "pthread")
    target_compile_definitions(host PRIVATE PORT_THREADS_DEFAULT_PTHREAD)
elseif(NOT PORT_THREADS STREQUAL "ucontext" OR NOT PORT_UCONTEXT_FOUND)
    message(FATAL_ERROR "PORT_THREADS is pthread, or ucontext where there is one (not macOS, emscripten or musl), not ${PORT_THREADS}")
endif()
message(STATUS "game threads: ${PORT_THREADS}")
target_include_directories(host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/host ${SDL2_INCLUDE_DIRS})
target_compile_options(host PRIVATE ${HOST_FLAGS} ${SDL2_CFLAGS_OTHER})
target_compile_definitions(host PRIVATE ${PORT_VERSION_DEFINE} PORT_ROM_SHA1="${PORT_ROM_SHA1}"
    PORT_VERSION_NAME="${PORT_VERSION}")
if(PORT_SDK_TRACE)
    target_compile_definitions(host PRIVATE PORT_SDK_TRACE)
endif()
# --replay names the function behind each of the movie's reads of the
# counts by its address (port/host/replay.c): the version's functions
add_custom_command(OUTPUT ${GEN}/n64_funcs.txt
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py funcs ${GEN}/n64_funcs.txt
    DEPENDS ${BLAST}/build/hd_code.${PORT_VERSION}.elf ${BLAST}/build/hd_front_end.${PORT_VERSION}.elf
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py)
add_custom_target(n64_funcs DEPENDS ${GEN}/n64_funcs.txt)
add_dependencies(host n64_funcs)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/replay.c PROPERTIES
    COMPILE_DEFINITIONS "PORT_N64_FUNCS=\"${GEN}/n64_funcs.txt\"")
if(EPOXY_FOUND)
    target_compile_definitions(host PRIVATE PORT_HAVE_GL)
    target_include_directories(host PRIVATE ${EPOXY_INCLUDE_DIRS})
endif()
# PORT_SIMD (docs/PORT.md, "The fourth round"): the renderer's vertex
# transform and lighting in four-lane vectors, the same bits as the scalar
# code; in WebAssembly gfx.c is built with SIMD128 (-msimd128), which every
# browser the page is for has (Chrome 91, Firefox 89, Safari 16.4), and
# without the SLP vectorizer, whose vector code from scalar code V8 ran slower
option(PORT_SIMD "the renderer's vertices in four-lane vectors (WebAssembly: SIMD128)" ON)
if(PORT_SIMD)
    target_compile_definitions(host PRIVATE PORT_SIMD)
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i.86|amd64|AMD64" AND NOT EMSCRIPTEN)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/gfx.c PROPERTIES COMPILE_OPTIONS "-O3;-msse4.1")
elseif(EMSCRIPTEN AND PORT_SIMD)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/gfx.c PROPERTIES COMPILE_OPTIONS "-O3;-msimd128;-fno-slp-vectorize")
else()
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/gfx.c PROPERTIES COMPILE_OPTIONS "-O3")
endif()

# ---- the loaders (host/native.c, src/loads.c) ---------------------------------
# the decompressors' entry points are wrapped, so every load is seen; the
# ROM's segments come from the top-level link map
add_custom_command(OUTPUT ${GEN}/romtab.h
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_romtab.py ${GEN}/romtab.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_romtab.py ${ROOT}/build/blastcorps.${PORT_VERSION}.map
            ${PORT_INVENTORY})
add_custom_target(romtab DEPENDS ${GEN}/romtab.h)
add_dependencies(host romtab)
target_include_directories(host PRIVATE ${GEN})

# ---- link ----------------------------------------------------------------------
add_executable(blastcorps $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:libaudio_c> $<TARGET_OBJECTS:port_n64>
    $<TARGET_OBJECTS:engine_c> $<TARGET_OBJECTS:game_data> $<TARGET_OBJECTS:fe_c> $<TARGET_OBJECTS:host>)
set_target_properties(blastcorps PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(blastcorps PRIVATE ${SDL2_LDFLAGS} ${EPOXY_LDFLAGS} Threads::Threads m)
if(EMSCRIPTEN)
    # (-O2 at the link too: without it emcc runs no wasm-opt, and Asyncify's
    # instrumentation stays as slow as it was made)
    target_link_options(blastcorps PRIVATE -O2 -sINITIAL_MEMORY=256MB -sSTACK_SIZE=1MB)
    # Binaryen inlines every function that has one caller, whatever its
    # size: the game's main loop (func_80244930) took in most of the game,
    # 300,000 lines of wat, and main() most of the host.  Under Asyncify a
    # fiber switch unwinds and rewinds every frame on the stack, and
    # rewinding one of those walks its body to the call: the game's C ran
    # 2-3 times slower than with them left apart (docs/PORT.md,
    # "Performance")
    target_link_options(blastcorps PRIVATE -sBINARYEN_EXTRA_PASSES=--one-caller-inline-max-function-size=200)
    if(WASM_PTHREAD)
        # the game's threads are host threads (fiber_pthread.c), and the
        # loop one too (PROXY_TO_PTHREAD), so that the page's own thread is
        # free to start them
        target_link_options(blastcorps PRIVATE -pthread -sPROXY_TO_PTHREAD -sPTHREAD_POOL_SIZE=24
            -sDEFAULT_PTHREAD_STACK_SIZE=1MB)
    else()
        # emscripten's fibers (fiber_asyncify.c), and the loop's sleeps
        # hand the page its thread back (main.c)
        target_link_options(blastcorps PRIVATE -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536)
        # (the renderer never switches fibers: web/asyncify_remove.txt)
        if(NOT PORT_WASM_ASYNCIFY_ALL)
            target_link_options(blastcorps PRIVATE
                -sASYNCIFY_REMOVE=@${CMAKE_CURRENT_SOURCE_DIR}/web/asyncify_remove.txt)
            set_property(TARGET blastcorps APPEND PROPERTY LINK_DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/web/asyncify_remove.txt)
        endif()
    endif()
    if(PORT_WASM_TARGET STREQUAL "node")
        if(WASM_PTHREAD)
            target_link_options(blastcorps PRIVATE -sENVIRONMENT=node,worker)
        else()
            target_link_options(blastcorps PRIVATE -sENVIRONMENT=node)
        endif()
        target_link_options(blastcorps PRIVATE -sNODERAWFS -sEXIT_RUNTIME)
    else()
        # a page (port/web/shell.html): the ROM from a file picker, the save
        # in IndexedDB (IDBFS), WebGL 2, main() once the ROM is there
        target_link_options(blastcorps PRIVATE -sENVIRONMENT=web -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
            -lidbfs.js -sFORCE_FILESYSTEM -sINVOKE_RUN=0 -sEXIT_RUNTIME=0
            -sEXPORTED_RUNTIME_METHODS=callMain,FS,ENV
            --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
        set_target_properties(blastcorps PROPERTIES SUFFIX ".html")
        set_property(TARGET blastcorps APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
        target_compile_definitions(host PRIVATE PORT_WASM_WEB PORT_HAVE_GL)     # (gfx_gl.c on WebGL 2)
    endif()
elseif(NOT APPLE)
    # (a PIE is all arm64 macOS runs, and ld64's default)
    target_link_options(blastcorps PRIVATE -pie)
endif()
target_link_options(blastcorps PRIVATE ${PORT_SAN_FLAGS})

# The test suite (port/tools/test.py; docs/PORT.md, "Testing the port"):
#   ctest --test-dir build/port -L quick -V        deterministic runs against the references
#   ctest --test-dir build/port -L tas -V          the TAS replay (us.v10, 17 minutes natively)
#   ctest --test-dir build/port -L tas-prefix -V   the TAS's first 3000 frames (an emulated build's)
#   ctest --test-dir build/port -L budget -V       the memory the build takes, against its ceiling
# Each skips (77) without the ROM, the TAS log or the venv.  A cross build
# (port/tools/cross-*.cmake) runs under its emulator (qemu-user), as test.py
# does it, with the timeouts that far longer.
enable_testing()
set(PORT_TEST ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/test.py)
add_test(NAME quick COMMAND ${PORT_TEST} quick ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME tas COMMAND ${PORT_TEST} tas ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME tas-prefix COMMAND ${PORT_TEST} tas ${CMAKE_CURRENT_BINARY_DIR} --frames 3000)
add_test(NAME budget COMMAND ${PORT_TEST} budget ${CMAKE_CURRENT_BINARY_DIR})
if(CMAKE_CROSSCOMPILING_EMULATOR)
    set(PORT_TEST_SLOWER 60)
else()
    set(PORT_TEST_SLOWER 1)
endif()
if(PORT_SANITIZE)
    math(EXPR PORT_TEST_SLOWER "${PORT_TEST_SLOWER} * 4")
endif()
math(EXPR QUICK_TIMEOUT "1800 * ${PORT_TEST_SLOWER}")
math(EXPR TAS_TIMEOUT "5400 * ${PORT_TEST_SLOWER}")
math(EXPR PREFIX_TIMEOUT "600 * ${PORT_TEST_SLOWER}")
set_tests_properties(quick PROPERTIES LABELS quick TIMEOUT ${QUICK_TIMEOUT} SKIP_RETURN_CODE 77)
set_tests_properties(tas PROPERTIES LABELS tas TIMEOUT ${TAS_TIMEOUT} SKIP_RETURN_CODE 77)
set_tests_properties(tas-prefix PROPERTIES LABELS tas-prefix TIMEOUT ${PREFIX_TIMEOUT} SKIP_RETURN_CODE 77)
set_tests_properties(budget PROPERTIES LABELS budget TIMEOUT ${QUICK_TIMEOUT} SKIP_RETURN_CODE 77)
