# The PC port of Blast Corps.  See docs/PORT.md.
#
# Needs a built stage 2 and the translated engine of PORT_VERSION (default
# us.v11; us.v10 and jp build too):
#   make VERSION=us.v11 -C blastcorps && make -C tools/recomp VERSION=us.v11
# then
#   cmake -S port -B build/port && cmake --build build/port -j
#
# By default everything the game runs is a 32-bit x86 program (-m32), so
# that a pointer is an N64 address; the game's C is built by clang with
# BEPass, which keeps its memory big-endian.  -DPORT_64BIT=ON builds a
# 64-bit program instead (x86-64 or AArch64): the game's C is compiled for
# i386's layout and converted by port-ilp32 (bepass/ILP32.cpp,
# tools/ilp32cc.py), everything else is native (docs/PORT.md, "The 64-bit
# build").
cmake_minimum_required(VERSION 3.20)
project(blastcorps_pc C CXX ASM)
set(PORT_VERSION us.v11 CACHE STRING "the version to build: us.v11, us.v10 or jp")
set_property(CACHE PORT_VERSION PROPERTY STRINGS us.v11 us.v10 jp)
if(NOT PORT_VERSION MATCHES "^(us\\.v1[01]|jp)$")
    message(FATAL_ERROR "PORT_VERSION is us.v11, us.v10 or jp, not ${PORT_VERSION}")
endif()
string(TOUPPER "VERSION_${PORT_VERSION}" PORT_VERSION_DEFINE)
string(REPLACE "." "_" PORT_VERSION_DEFINE ${PORT_VERSION_DEFINE})
option(PORT_64BIT "a 64-bit program (the game's C through port-ilp32)" OFF)
option(PORT_ILP32_CHECK "64-bit: check every pointer the game's C stores fits in 32 bits" OFF)
# diagnostics (docs/PORT.md, "Comparing builds"): a call trace of the
# game's C (PORT_TRACE=FILE,FROM,TO), the access-width profiler (PORT_ACCESS=FILE)
option(PORT_TRACE_CALLS "the game's C calls __port_trace on every function entry" OFF)
option(PORT_ACCESS_PROFILE "record the width every byte of RDRAM is written and read at" OFF)
option(PORT_SDK_TRACE "port/src/gu.c logs its calls to $PORT_SDK_TRACE (port/tools/sdk_check)" OFF)
# native-endian memory (docs/PORT.md, "Native-endian memory"): every datum in
# host order where the N64 has it, the ROM's data converted by type as it
# is loaded; BEPass then swaps nothing but the words of 64-bit scalars
option(PORT_NATIVE_ENDIAN "game memory in host byte order instead of the N64's" OFF)
# native LP64 structs (docs/PORT.md, "The LP64 build"): the game's C compiled
# for the host as it is, 8-byte pointers but in the structs the handwritten
# code, the ROM's data or the renderer share (PTR32); needs both of the above
option(PORT_LP64 "the game's C as an ordinary LP64 program (implies PORT_64BIT, PORT_NATIVE_ENDIAN)" OFF)
# movable memory (docs/PORT.md, "Movable memory"): RDRAM wherever the host
# puts it; the game's C reaches it through port-arena (bepass/Arena.cpp)
option(PORT_MOVABLE "game memory at a base the host chooses, not at 0x80000000" OFF)
if(PORT_MOVABLE AND PORT_ACCESS_PROFILE)
    message(FATAL_ERROR "PORT_MOVABLE and PORT_ACCESS_PROFILE don't go together (yet)")
endif()
# the libaudio oracle's recorder (docs/PORT.md, "The libaudio oracle"):
# PORT_AUDIO_LOG=FILE logs every libaudio call the game makes, for
# tools/audio_oracle to replay; a native-endian build at fixed addresses
option(PORT_AUDIO_RECORD "wrap the game's libaudio calls for the oracle's log" OFF)
if(PORT_AUDIO_RECORD AND (PORT_MOVABLE OR NOT PORT_NATIVE_ENDIAN OR PORT_LP64))
    message(FATAL_ERROR "PORT_AUDIO_RECORD is for the native-endian ILP32 build (-DPORT_64BIT=ON -DPORT_NATIVE_ENDIAN=ON)")
endif()
if(PORT_LP64)
    set(PORT_64BIT ON CACHE BOOL "" FORCE)
    set(PORT_NATIVE_ENDIAN ON CACHE BOOL "" FORCE)
endif()
# WebAssembly (emcmake cmake ...; docs/PORT.md, "WebAssembly"): the 32-bit
# native-endian movable build for wasm32, the host side through emcc and
# the N64 side through the system's clang, which loads BEPass
if(EMSCRIPTEN)
    if(PORT_64BIT OR PORT_LP64)
        message(FATAL_ERROR "the WebAssembly build is wasm32: no PORT_64BIT or PORT_LP64")
    endif()
    set(PORT_MOVABLE ON CACHE BOOL "" FORCE)
    set(PORT_NATIVE_ENDIAN ON CACHE BOOL "" FORCE)
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

# the game's data from the user's ROM at startup (docs/PORT.md, "The data from
# the ROM"; docs/DISTRIBUTION.md): the movable build's arena image isn't
# carried, only the operations that make it from the ROM's code modules
# (tools/rom_data.py, checked at build time against the image)
if(PORT_MOVABLE)
    option(PORT_ROM_DATA "the movable build's data from the ROM at startup, not from the executable" ON)
elseif(PORT_ROM_DATA)
    message(FATAL_ERROR "PORT_ROM_DATA is the movable build's (-DPORT_MOVABLE=ON)")
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "the port needs clang (BEPass is an LLVM plugin): -DCMAKE_C_COMPILER=clang")
endif()
if(CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    # (Apple's clang loads no pass plugins and ships no LLVM headers)
    message(FATAL_ERROR "the port needs LLVM's own clang, not Apple's: "
        "-DCMAKE_C_COMPILER=$(brew --prefix llvm)/bin/clang (docs/PORT.md, \"macOS\")")
endif()
if(APPLE AND NOT (PORT_MOVABLE AND PORT_64BIT))
    # arm64 macOS has no 32-bit programs and maps nothing in the low 4 GB
    message(FATAL_ERROR "macOS takes the movable 64-bit build: -DPORT_MOVABLE=ON -DPORT_LP64=ON (or -DPORT_64BIT=ON)")
endif()

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(BLAST ${ROOT}/blastcorps)
set(RECOMP_GEN ${BLAST}/build/recomp/src)
set(GEN ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN})
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# LLVM's tools, next to the compiler first (Homebrew's LLVM isn't on PATH)
get_filename_component(CLANG_BINDIR ${CMAKE_C_COMPILER} DIRECTORY)
# the N64 link's ELFs are read with MIPS binutils, or LLVM's, which read any
find_program(N64_NM NAMES mips-linux-gnu-nm llvm-nm HINTS ${CLANG_BINDIR} REQUIRED)
find_program(N64_READELF NAMES mips-linux-gnu-readelf llvm-readelf HINTS ${CLANG_BINDIR} REQUIRED)
# the generators read the stage-2 build of PORT_VERSION
if(PORT_LP64)
    set(PY ${CMAKE_COMMAND} -E env PORT_VERSION=${PORT_VERSION} PORT_N64_NM=${N64_NM} PORT_LP64=1 ${Python3_EXECUTABLE})
else()
    set(PY ${CMAKE_COMMAND} -E env PORT_VERSION=${PORT_VERSION} PORT_N64_NM=${N64_NM} ${Python3_EXECUTABLE})
endif()

foreach(f ${BLAST}/build/hd_code.${PORT_VERSION}.elf ${RECOMP_GEN}/recomp_funcs.h)
    if(NOT EXISTS ${f})
        message(FATAL_ERROR "${f} is missing: build blastcorps (${PORT_VERSION}) and tools/recomp first")
    endif()
endforeach()
# blastcorps/asm and the translated engine hold one version at a time
file(READ ${BLAST}/.version STAGE2_VERSION)
string(STRIP "${STAGE2_VERSION}" STAGE2_VERSION)
if(NOT STAGE2_VERSION STREQUAL PORT_VERSION)
    message(FATAL_ERROR "blastcorps/ is extracted for ${STAGE2_VERSION}, not ${PORT_VERSION}")
endif()

# the ROM this port is for, exactly (host/main.c checks it at startup), and,
# for PORT_ROM_DATA, the ROM the build makes its data operations from
file(READ ${ROOT}/blastcorps.${PORT_VERSION}.sha1 PORT_ROM_SHA1)
string(STRIP "${PORT_ROM_SHA1}" PORT_ROM_SHA1)
set(PORT_BASEROM ${ROOT}/baserom.${PORT_VERSION}.z64 CACHE FILEPATH "the version's ROM (PORT_ROM_DATA's build)")
if(PORT_ROM_DATA)
    if(NOT EXISTS ${PORT_BASEROM})
        message(FATAL_ERROR "${PORT_BASEROM} is missing: PORT_ROM_DATA makes the data's operations from it "
            "(-DPORT_BASEROM=..., or -DPORT_ROM_DATA=OFF to carry the data in the executable)")
    endif()
    file(SHA1 ${PORT_BASEROM} BASEROM_SHA1)
    if(NOT BASEROM_SHA1 STREQUAL PORT_ROM_SHA1)
        message(FATAL_ERROR "${PORT_BASEROM} isn't ${PORT_VERSION}'s ROM (sha1 ${BASEROM_SHA1}, not ${PORT_ROM_SHA1})")
    endif()
endif()

# Addresses that differ between versions, from the version's N64 link: the
# end of hd_code's .bss, where the heap starts (D_803FF600 there,
# port_fixed.ld.in), and the front end's .data and .bss (src/overlay.c)
execute_process(COMMAND ${N64_NM} ${BLAST}/build/hd_code.${PORT_VERSION}.elf
    OUTPUT_VARIABLE HD_CODE_NM COMMAND_ERROR_IS_FATAL ANY)
if(NOT HD_CODE_NM MATCHES "([0-9a-f]+) B D_803FF600\n")
    message(FATAL_ERROR "no D_803FF600 in hd_code.${PORT_VERSION}.elf")
endif()
set(PORT_HEAP_START 0x${CMAKE_MATCH_1})
execute_process(COMMAND ${N64_READELF} -SW ${BLAST}/build/hd_front_end.${PORT_VERSION}.elf
    OUTPUT_VARIABLE FE_SECTIONS COMMAND_ERROR_IS_FATAL ANY)
foreach(sec data bss)
    string(TOUPPER ${sec} SEC)
    if(NOT FE_SECTIONS MATCHES "\\.hd_front_end_${sec} +[A-Z]+ +([0-9a-f]+)")
        message(FATAL_ERROR "no .hd_front_end_${sec} in hd_front_end.${PORT_VERSION}.elf")
    endif()
    set(PORT_FE_${SEC} 0x${CMAKE_MATCH_1})
endforeach()
configure_file(port_fixed.ld.in ${GEN}/port_fixed.ld @ONLY)
message(STATUS "${PORT_VERSION}: heap from ${PORT_HEAP_START}, front end .data ${PORT_FE_DATA}, .bss ${PORT_FE_BSS}")

# ---- BEPass (a host LLVM plugin, 64-bit) ------------------------------------
find_program(LLVM_CONFIG llvm-config HINTS ${CLANG_BINDIR} REQUIRED)
# A cross build (CMAKE_C_COMPILER_TARGET, port/tools/cross-aarch64.cmake)
# can't build the plugin clang loads: it takes one from a native build.
set(PORT_BEPASS_PLUGIN "" CACHE FILEPATH "cross builds: a libBEPass.so built for the host")
if(EMSCRIPTEN)
    # the N64 side's compiler and the plugin's: the clang of LLVM_CONFIG
    execute_process(COMMAND ${LLVM_CONFIG} --bindir OUTPUT_VARIABLE LLVM_BINDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    find_program(PORT_N64_CC clang HINTS ${LLVM_BINDIR} NO_DEFAULT_PATH REQUIRED)
    find_program(PORT_N64_CXX clang++ HINTS ${LLVM_BINDIR} NO_DEFAULT_PATH REQUIRED)
endif()
if(PORT_BEPASS_PLUGIN)
    set(BEPASS ${PORT_BEPASS_PLUGIN})
    add_custom_target(BEPass)
elseif(EMSCRIPTEN)
    # (CMake's C++ compiler is em++: the plugin is the host's)
    execute_process(COMMAND ${LLVM_CONFIG} --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    separate_arguments(LLVM_CXXFLAGS)
    set(BEPASS ${CMAKE_BINARY_DIR}/libBEPass.so)
    set(BEPASS_SRC bepass/BEPass.cpp bepass/ILP32.cpp bepass/LP64.cpp bepass/Arena.cpp)
    list(TRANSFORM BEPASS_SRC PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
    add_custom_command(OUTPUT ${BEPASS}
        COMMAND ${PORT_N64_CXX} ${LLVM_CXXFLAGS} -fno-rtti -O2 -fPIC -shared -o ${BEPASS} ${BEPASS_SRC}
        DEPENDS ${BEPASS_SRC} COMMAND_EXPAND_LISTS)
    add_custom_target(BEPass DEPENDS ${BEPASS})
else()
    execute_process(COMMAND ${LLVM_CONFIG} --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    separate_arguments(LLVM_CXXFLAGS)
    add_library(BEPass MODULE bepass/BEPass.cpp bepass/ILP32.cpp bepass/LP64.cpp bepass/Arena.cpp)
    target_compile_options(BEPass PRIVATE ${LLVM_CXXFLAGS} -fno-rtti)
    set_target_properties(BEPass PROPERTIES PREFIX "lib" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    if(APPLE)
        # LLVM's symbols are clang's and opt's, found when they load it
        target_link_options(BEPass PRIVATE -undefined dynamic_lookup)
    endif()
    set(BEPASS ${CMAKE_BINARY_DIR}/libBEPass${CMAKE_SHARED_MODULE_SUFFIX})
endif()

if(EMSCRIPTEN)
    execute_process(COMMAND ${PORT_N64_CC} -print-resource-dir OUTPUT_VARIABLE CLANG_RESOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-resource-dir OUTPUT_VARIABLE CLANG_RESOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
# the N64 side's layout: i386 with 8-byte aligned doubles and long longs,
# as MIPS o32 has them
set(M32 -m32 -msse2 -mfpmath=sse -malign-double)
if(EMSCRIPTEN)
    # The N64 side is compiled as the 32-bit build's, for i386 (the same IR,
    # so the same instruction counts, the same timing), and the arena link
    # makes it wasm32's, whose layout that is: 4-byte pointers and longs,
    # 8-aligned doubles and long longs
    execute_process(COMMAND ${PORT_N64_CC} --target=wasm32-unknown-emscripten -S -emit-llvm -o - -x c /dev/null
        OUTPUT_VARIABLE WASM_IR COMMAND_ERROR_IS_FATAL ANY)
    if(NOT WASM_IR MATCHES "target datalayout = \"([^\"]+)\"")
        message(FATAL_ERROR "no wasm32 data layout from ${PORT_N64_CC}")
    endif()
    set(WASM_DATALAYOUT ${CMAKE_MATCH_1})
elseif(PORT_LP64)
    # the host's own layout: no i386 frontend, no port-ilp32
    set(M32)
elseif(PORT_64BIT)
    if(CMAKE_C_COMPILER_TARGET)
        set(PORT_TRIPLE ${CMAKE_C_COMPILER_TARGET})
    else()
        execute_process(COMMAND ${CMAKE_C_COMPILER} -print-target-triple OUTPUT_VARIABLE PORT_TRIPLE OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    get_filename_component(LLVM_BINDIR_HINT ${LLVM_CONFIG} DIRECTORY)
    find_program(LLVM_OPT opt HINTS ${LLVM_BINDIR_HINT} REQUIRED)
    set(ILP32CC ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/ilp32cc.py --opt ${LLVM_OPT}
        --plugin ${BEPASS} --target ${PORT_TRIPLE})
    if(PORT_ILP32_CHECK)
        list(APPEND ILP32CC --check)
    endif()
    list(APPEND ILP32CC --)
endif()
# BEPass's diagnostics are switched on by the environment of the compiler
set(N64_LAUNCHER)
if(PORT_TRACE_CALLS)
    list(APPEND N64_LAUNCHER BEPASS_TRACE=1)
endif()
if(PORT_ACCESS_PROFILE)
    list(APPEND N64_LAUNCHER BEPASS_ACCESS=1)
endif()
if(PORT_NATIVE_ENDIAN)
    list(APPEND N64_LAUNCHER BEPASS_NATIVE=1)
endif()
if(PORT_LP64)
    list(APPEND N64_LAUNCHER BEPASS_LP64=1)
endif()
# the functions the link wraps (the replay's hooks, the overlay, the loaders);
# the glue calls them by their __wrap_ names itself (gen_glue.py)
set(PORT_WRAPS func_8028B3E0 func_802D4E10 alCSeqGetLoc func_8025C230 func_8028B4C4 __osContDataCrc)
if(PORT_AUDIO_RECORD)
    # (func_802D4E10 and alCSeqGetLoc are the replay's already: it calls the recorder's)
    list(APPEND PORT_WRAPS alHeapInit alHeapDBAlloc alBnkfNew alSeqFileNew alInit alClose
        alAudioFrame alLink alUnlink alEvtqNew alEvtqNextEvent alEvtqPostEvent alSynAddPlayer
        alSynAllocVoice alSynFreeVoice alSynStopVoice alSynStartVoice alSynSetVol alSynSetPan
        alSynSetPitch alSynSetFXMix alCents2Ratio alCSPNew func_802D81F0 func_802D76C0
        func_802D97E0 func_802D81B0 alCSPSetVol alCSPGetVol alCSPSetTempo alCSPGetTempo
        alCSeqNew alCSeqSetLoc)
endif()
string(REPLACE ";" "," PORT_WRAPS_LIST "${PORT_WRAPS}")
if(PORT_MOVABLE)
    # the N64 side is linked into one module (the arena link, below), so
    # --wrap is done per file by the plugin
    list(APPEND N64_LAUNCHER BEPASS_WRAP=${PORT_WRAPS_LIST})
endif()
if(N64_LAUNCHER)
    list(PREPEND N64_LAUNCHER env)
endif()
if(PORT_LP64)
    set(HOST_ARCH -fPIE)
    list(APPEND M32 -fPIE)
    message(STATUS "LP64 build")
elseif(PORT_64BIT)
    list(PREPEND ILP32CC ${N64_LAUNCHER})
    set(N64_LAUNCHER ${ILP32CC})
    set(HOST_ARCH -fPIE)
    list(APPEND M32 -fPIE)
    message(STATUS "64-bit build for ${PORT_TRIPLE}")
elseif(EMSCRIPTEN)
    list(APPEND N64_LAUNCHER ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/n64cc.py ${PORT_N64_CC} --)
    if(NOT N64_LAUNCHER MATCHES "^env;")
        list(PREPEND N64_LAUNCHER env)
    endif()
    set(HOST_ARCH ${WASM_PTHREAD})
    message(STATUS "WebAssembly build (${PORT_WASM_TARGET}); the N64 side by ${PORT_N64_CC}")
else()
    set(HOST_ARCH ${M32})
endif()

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
foreach(r ${REPLACED} ${REPLACED_SDK})
    list(REMOVE_ITEM GAME_C ${BLAST}/src/${r}.c)
endforeach()

# libaudio (docs/PORT.md, "libaudio"): the port's own (port/libaudio), or
# the decompiled original (src/libultra/audio, through these objects), which
# the libaudio oracle records with and replays into.  Either is built apart
# from the rest of the game's C, with no polls on its loops (BEPASS_NOPOLL=1;
# docs/PORT.md, "Timing"): the original as the game's (nopoll_c, below), the
# port's own as the port's (libaudio_c).
option(PORT_LIBAUDIO_ORIGINAL "the decompiled original libaudio, not the port's own (port/libaudio)" OFF)
set(LIBAUDIO_ORIG
    hd_code/90390 hd_code/92350 hd_code/923B0 hd_code/926F0 hd_code/927B0 hd_code/92800
    hd_code/92A30 hd_code/92AD0 hd_code/92B60 hd_code/92BF0 hd_code/92C80 hd_code/92D20
    hd_code/92DA0 hd_code/92F00 hd_code/92F30 hd_code/939F0 hd_code/93A30 hd_code/93AA0
    hd_code/93B50 hd_code/93B60 hd_code/93BA0 hd_code/93DF0 hd_code/95020 hd_code/95460
    hd_code/99620 hd_code/996A0 hd_code/996B0 hd_code/9AFF0 hd_code/9B470 hd_code/9BBF0
    hd_code/9BC90 hd_code/9BDE0 hd_code/9C930 hd_code/9CC30 hd_code/9D810 hd_code/9D920
    hd_code/9F7D0 hd_code/9FDF0 hd_code/9FE00 hd_code/9FE20)
set(NOPOLL_C)
foreach(f ${LIBAUDIO_ORIG})
    list(REMOVE_ITEM GAME_C ${BLAST}/src/${f}.c)
    if(PORT_LIBAUDIO_ORIGINAL)
        list(APPEND NOPOLL_C ${BLAST}/src/${f}.c)
    endif()
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${BLAST} ${BLAST}/include ${SDK_INCLUDES})
set(N64_DEFINES _LANGUAGE_C _FINALROM TARGET_PC ${PORT_VERSION_DEFINE} _MIPS_SZLONG=32 _MIPS_SZINT=32)
if(PORT_AUDIO_RECORD)
    list(APPEND N64_DEFINES PORT_AUDIO_RECORD)
endif()
if(PORT_64BIT)
    list(APPEND N64_DEFINES PORT_64BIT)
endif()
if(PORT_LP64)
    list(APPEND N64_DEFINES PORT_LP64)
    list(APPEND M32 -fms-extensions)    # PTR32: __ptr32 __uptr
endif()
if(PORT_ACCESS_PROFILE)
    list(APPEND N64_DEFINES PORT_ACCESS_PROFILE)
endif()
if(PORT_MOVABLE)
    list(APPEND N64_DEFINES PORT_MOVABLE)
endif()
if(PORT_NATIVE_ENDIAN)
    list(APPEND N64_DEFINES PORT_NATIVE_ENDIAN)
    # without the byte swaps clang vectorizes the game's float arrays and
    # raises their alignment, which would move them off their N64 addresses
    list(APPEND M32 -fno-vectorize -fno-slp-vectorize)
endif()
# The decompiled C is IDO's dialect: K&R definitions, implicit
# declarations, prototypes that differ between files, pointers and ints
# used interchangeably.  These flags make clang accept it and not optimise
# on the undefined behaviour it is full of.  (-fsigned-char: IDO's -signed,
# which AArch64 Linux doesn't have by default.)
set(N64_FLAGS ${M32} -std=gnu89 -O2 -g -fsigned-char -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks
    -ftrivial-auto-var-init=zero -ffp-contract=off -fno-common -fno-builtin
    -nostdinc -isystem ${CLANG_RESOURCE_DIR}/include -fpass-plugin=${BEPASS}
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_game.h
    -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
    -Wno-unused-label -Wno-unused-value -Wno-missing-braces -Wno-pointer-sign
    -Wno-parentheses -Wno-dangling-else -Wno-deprecated-non-prototype
    -Wno-incompatible-library-redeclaration -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
    -Wno-implicit-int -Wno-return-type -Wno-uninitialized -Wno-sometimes-uninitialized
    -Wno-comment -Wno-tautological-constant-out-of-range-compare
    -Wno-constant-conversion -Wno-switch -Wno-array-bounds -Wno-shift-count-overflow
    -Wno-self-assign -Wno-parentheses-equality -Wno-tautological-compare -Wno-empty-body
    -Wno-unknown-pragmas -Wno-main-return-type -Wno-shift-negative-value -Wno-builtin-requires-header
    -Wno-error=incompatible-pointer-types -Wno-incompatible-pointer-types
    -Wno-error=int-conversion -Wno-int-conversion -Wno-error=implicit-function-declaration
    -Wno-error=return-mismatch -Wno-return-mismatch -Wno-incompatible-function-pointer-types
    -Wno-implicit-const-int-float-conversion -Wno-pointer-bool-conversion
    -Wno-string-plus-int -Wno-null-pointer-arithmetic -Wno-gnu-folding-constant
    -Wno-macro-redefined -Wno-ignored-pragma-intrinsic)   # (IDO's sqrtf, not one on AArch64)
if(PORT_MOVABLE)
    list(APPEND N64_FLAGS -emit-llvm)       # bitcode, for the arena link
endif()

# The game's C that never waits on another thread (NOPOLL_C): BEPass puts no
# polls on its loops (BEPASS_NOPOLL=1).  The polls are where a thread that
# busy-waits lets time pass, and every 64th advances --deterministic's
# clock, so without them a replacement's loops needn't have the original's
# shape for the game to keep its timing.  Its data is the game's, like
# game_c's.
set(NOPOLL_LAUNCHER ${N64_LAUNCHER})
list(FIND NOPOLL_LAUNCHER env NOPOLL_ENV_AT)
if(NOPOLL_ENV_AT EQUAL 0)
    list(INSERT NOPOLL_LAUNCHER 1 BEPASS_NOPOLL=1)
else()
    list(PREPEND NOPOLL_LAUNCHER env BEPASS_NOPOLL=1)
endif()
set(NOPOLL_TARGETS)
set(NOPOLL_GAME_OBJECTS)
if(NOPOLL_C)
    add_library(nopoll_c OBJECT ${NOPOLL_C})
    target_compile_options(nopoll_c PRIVATE -fdata-sections)
    target_include_directories(nopoll_c PRIVATE ${N64_INCLUDES})
    target_compile_definitions(nopoll_c PRIVATE ${N64_DEFINES})
    target_compile_options(nopoll_c PRIVATE ${N64_FLAGS})
    add_dependencies(nopoll_c BEPass)
    set_source_files_properties(${NOPOLL_C} PROPERTIES OBJECT_DEPENDS ${BEPASS})
    set_target_properties(nopoll_c PROPERTIES C_COMPILER_LAUNCHER "${NOPOLL_LAUNCHER}")
    list(APPEND NOPOLL_TARGETS nopoll_c)
    set(NOPOLL_GAME_OBJECTS $<TARGET_OBJECTS:nopoll_c>)
endif()
# the port's own libaudio: N64 side, its data the port's (like port_n64's)
set(NOPOLL_PORT_OBJECTS)
if(NOT PORT_LIBAUDIO_ORIGINAL)
    file(GLOB LIBAUDIO_C ${CMAKE_CURRENT_SOURCE_DIR}/libaudio/*.c)
    add_library(libaudio_c OBJECT ${LIBAUDIO_C})
    target_include_directories(libaudio_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libaudio ${N64_INCLUDES})
    target_compile_definitions(libaudio_c PRIVATE ${N64_DEFINES})
    target_compile_options(libaudio_c PRIVATE ${N64_FLAGS})
    add_dependencies(libaudio_c BEPass)
    set_source_files_properties(${LIBAUDIO_C} PROPERTIES OBJECT_DEPENDS ${BEPASS})
    set(LIBAUDIO_LAUNCHER ${NOPOLL_LAUNCHER})
    if(PORT_MOVABLE)
        list(INSERT LIBAUDIO_LAUNCHER 1 BEPASS_PORT_SRC=1)
    endif()
    set_target_properties(libaudio_c PROPERTIES C_COMPILER_LAUNCHER "${LIBAUDIO_LAUNCHER}")
    list(APPEND NOPOLL_TARGETS libaudio_c)
    set(NOPOLL_PORT_OBJECTS $<TARGET_OBJECTS:libaudio_c>)
endif()
set(NOPOLL_OBJECTS ${NOPOLL_GAME_OBJECTS} ${NOPOLL_PORT_OBJECTS})

add_library(game_c OBJECT ${GAME_C})
target_compile_options(game_c PRIVATE -fdata-sections)
target_include_directories(game_c PRIVATE ${N64_INCLUDES})
target_compile_definitions(game_c PRIVATE ${N64_DEFINES})
target_compile_options(game_c PRIVATE ${N64_FLAGS})
add_dependencies(game_c BEPass)
set_source_files_properties(${GAME_C} PROPERTIES OBJECT_DEPENDS ${BEPASS})
if(N64_LAUNCHER)
    set_target_properties(game_c PROPERTIES C_COMPILER_LAUNCHER "${N64_LAUNCHER}")
endif()

# ---- the engine's replacement (N64 side; docs/PORT.md, "Replacing the engine") --
# Rare's handwritten functions as native C (port/engine; the translator
# leaves out the ones replaced.txt lists).  Built like the game's C, but it
# charges the original's MIPS instructions itself (ENGINE_BLK), so BEPass
# neither counts nor polls in it (BEPASS_ENGINE=1).  PORT_ENGINE_CHECK runs
# each replaced function's translation first on the same state and
# compares (port/host/engine_check.c); the 32-bit build only.
option(PORT_ENGINE_CHECK "check the engine's native functions against their translation as they run" OFF)
if(PORT_ENGINE_CHECK AND (PORT_64BIT OR PORT_MOVABLE OR PORT_NATIVE_ENDIAN))
    message(FATAL_ERROR "PORT_ENGINE_CHECK is the 32-bit build's")
endif()
file(GLOB ENGINE_C ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.c)
set(ENGINE_GLUE_FLAGS)
if(PORT_ENGINE_CHECK)
    set(ENGINE_GLUE_FLAGS --check)
endif()
if(ENGINE_C)
    add_library(engine_c OBJECT ${ENGINE_C})
    target_include_directories(engine_c PRIVATE ${N64_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/engine ${RECOMP_GEN})
    target_compile_definitions(engine_c PRIVATE ${N64_DEFINES})
    target_compile_options(engine_c PRIVATE ${N64_FLAGS})
    add_dependencies(engine_c BEPass)
    set_source_files_properties(${ENGINE_C} PROPERTIES OBJECT_DEPENDS "${BEPASS};${RECOMP_GEN}/engine_blocks.h")
    set(ENGINE_LAUNCHER ${N64_LAUNCHER})
    list(FIND ENGINE_LAUNCHER env ENV_AT)
    if(ENV_AT EQUAL 0)
        list(INSERT ENGINE_LAUNCHER 1 BEPASS_ENGINE=1 BEPASS_PORT_SRC=1)
    else()
        list(PREPEND ENGINE_LAUNCHER env BEPASS_ENGINE=1 BEPASS_PORT_SRC=1)
    endif()
    set_target_properties(engine_c PROPERTIES C_COMPILER_LAUNCHER "${ENGINE_LAUNCHER}")
    if(PORT_ENGINE_CHECK)
        target_compile_definitions(engine_c PRIVATE PORT_ENGINE_CHECK)
        target_compile_options(engine_c PRIVATE "SHELL:-include ${GEN}/engine_check_names.h")
        set_source_files_properties(${ENGINE_C} PROPERTIES OBJECT_DEPENDS
            "${BEPASS};${RECOMP_GEN}/engine_blocks.h;${GEN}/engine_check_names.h")
    endif()
    set(ENGINE_OBJECTS $<TARGET_OBJECTS:engine_c>)
else()
    set(ENGINE_OBJECTS)
endif()

# ---- the libultra replacement (N64 side, BEPass too) --------------------------
file(GLOB PORT_N64_C ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_library(port_n64 OBJECT ${PORT_N64_C})
target_include_directories(port_n64 PRIVATE ${N64_INCLUDES})
target_compile_definitions(port_n64 PRIVATE ${N64_DEFINES})
target_compile_options(port_n64 PRIVATE ${N64_FLAGS} -Wno-implicit-function-declaration)
target_compile_definitions(port_n64 PRIVATE PORT_FE_DATA=${PORT_FE_DATA} PORT_FE_BSS=${PORT_FE_BSS})
add_dependencies(port_n64 BEPass)
set_source_files_properties(${PORT_N64_C} PROPERTIES OBJECT_DEPENDS ${BEPASS})
# sins()'s table, generated
add_custom_command(OUTPUT ${GEN}/sdk/sintable.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py ${GEN}/sdk/sintable.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sintable.py VERBATIM)
file(MAKE_DIRECTORY ${GEN}/sdk)
target_include_directories(port_n64 PRIVATE ${GEN}/sdk)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/gu.c PROPERTIES
    OBJECT_DEPENDS "${BEPASS};${GEN}/sdk/sintable.h")
if(PORT_SDK_TRACE)
    target_compile_definitions(port_n64 PRIVATE PORT_SDK_TRACE)
endif()
if(PORT_MOVABLE)
    # (the port's own data: not at N64 addresses, as in the other builds)
    set(PORT_N64_LAUNCHER ${N64_LAUNCHER})
    list(INSERT PORT_N64_LAUNCHER 1 BEPASS_PORT_SRC=1)
    set_target_properties(port_n64 PROPERTIES C_COMPILER_LAUNCHER "${PORT_N64_LAUNCHER}")
elseif(N64_LAUNCHER)
    set_target_properties(port_n64 PROPERTIES C_COMPILER_LAUNCHER "${N64_LAUNCHER}")
endif()

# ---- the game's data, big-endian ---------------------------------------------
# (what the N64 build links: asm/data, which holds the handwritten objects'
# .bss too)
file(GLOB DATA_S ${BLAST}/asm/data/hd_code/*.s ${BLAST}/asm/data/hd_front_end/*.s)
# libultra's exception handler's jump table (its code is replaced)
list(REMOVE_ITEM DATA_S ${BLAST}/asm/data/hd_code/972A0.rodata.s)
set(DATA_BIN hd_code/68810 hd_code/690C0 hd_code/8E910 hd_code/91D50 hd_code/A0C30
             hd_code/C9BD0 hd_code/CA3D0 hd_code/CA6A0 hd_front_end/20090 hd_front_end/29690)
set(DATA_X86)
set(ASM2X86_FLAGS)
if(PORT_NATIVE_ENDIAN)
    set(ASM2X86_FLAGS --native)     # host order, by the directives and the type inventory
endif()
foreach(s ${DATA_S})
    file(RELATIVE_PATH rel ${BLAST}/asm ${s})
    string(REPLACE "/" "_" out ${rel})
    set(label "")
    if(s MATCHES "/([a-z_]+)/([0-9A-F]+)\\.bin\\.s$")
        set(label ${CMAKE_MATCH_1}_${CMAKE_MATCH_2}_bin)
    endif()
    add_custom_command(OUTPUT ${GEN}/data/${out}
        COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py ${ASM2X86_FLAGS} ${GEN}/data/${out} ${s} ${label}
        DEPENDS ${s} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py ${BLAST}/include/game/inventory.json)
    list(APPEND DATA_X86 ${GEN}/data/${out})
endforeach()
# the data of the IDO functions the version still has as asm (its GLOBAL_ASM,
# which tools/recomp translates): their strings, floats and jump tables
execute_process(COMMAND ${Python3_EXECUTABLE} -c
    "import sys; sys.path.insert(0, '${ROOT}/tools/recomp'); import config; print(';'.join(f'{m}/{o}/{f}' for m, o, f in config.global_asm_functions('${PORT_VERSION}')))"
    OUTPUT_VARIABLE GLOBAL_ASM_FUNCS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
foreach(g ${GLOBAL_ASM_FUNCS})
    string(REPLACE "/" "_" out ${g})
    set(src ${BLAST}/asm/nonmatchings/${g}.s)
    add_custom_command(OUTPUT ${GEN}/data/global_asm_${out}.s
        COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py ${ASM2X86_FLAGS} --global-asm ${GEN}/data/global_asm_${out}.s ${src}
        DEPENDS ${src} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py)
    list(APPEND DATA_X86 ${GEN}/data/global_asm_${out}.s)
endforeach()
foreach(b ${DATA_BIN})
    string(REPLACE "/" "_" label ${b})
    add_custom_command(OUTPUT ${GEN}/data/${label}.bin.s
        COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py ${ASM2X86_FLAGS} ${GEN}/data/${label}.bin.s ${BLAST}/assets/${b}.bin ${label}_bin
        DEPENDS ${BLAST}/assets/${b}.bin ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py)
    list(APPEND DATA_X86 ${GEN}/data/${label}.bin.s)
endforeach()
file(MAKE_DIRECTORY ${GEN}/data)
set_source_files_properties(${DATA_X86} PROPERTIES LANGUAGE ASM GENERATED TRUE)
if(NOT PORT_MOVABLE)       # (the movable build takes them as IR: the arena link, below)
    add_library(game_data OBJECT ${DATA_X86})
    if(NOT PORT_64BIT)
        target_compile_options(game_data PRIVATE -m32)
    endif()
    target_compile_options(game_data PRIVATE -Wno-unused-command-line-argument)
endif()

# ---- the translated handwritten engine (host side) ---------------------------
file(GLOB RECOMP_C ${RECOMP_GEN}/hd_code_*.c ${RECOMP_GEN}/hd_front_end_*.c)
add_custom_command(OUTPUT ${GEN}/port_recomp_syms.h
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py header ${GEN}/port_recomp_syms.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py ${RECOMP_GEN}/syms_hd_code.h ${RECOMP_GEN}/syms_hd_front_end.h)
file(GLOB ENGINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.c ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/engine/replaced.txt)
add_custom_command(OUTPUT ${GEN}/entry.c ${GEN}/externs.c ${GEN}/glue_report.txt ${GEN}/fn_values.txt
                          ${GEN}/engine_check_names.h
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_glue.py ${GEN} --wrap ${PORT_WRAPS_LIST} ${ENGINE_GLUE_FLAGS}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_glue.py ${CMAKE_CURRENT_SOURCE_DIR}/tools/liveness.py
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/conventions.py
            ${RECOMP_GEN}/recomp_funcs.h ${RECOMP_GEN}/recomp_externs.h ${GAME_C} ${ENGINE_SRC})
add_custom_target(engine_glue DEPENDS ${GEN}/engine_check_names.h)
if(TARGET engine_c)
    add_dependencies(engine_c engine_glue)
endif()
add_custom_target(recomp_headers DEPENDS ${GEN}/port_recomp_syms.h)

set(HOST_FLAGS ${HOST_ARCH} -O2 -g -ffp-contract=off -fno-strict-aliasing -Wall)
add_library(recomp OBJECT ${RECOMP_C} ${GEN}/entry.c ${GEN}/externs.c)
target_include_directories(recomp PRIVATE ${ROOT}/tools/recomp/runtime ${RECOMP_GEN} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(recomp PRIVATE RECOMP_COUNT)
if(PORT_ENGINE_CHECK)
    # the replaced functions' translations, which the check runs first
    target_compile_definitions(recomp PRIVATE RECOMP_ORIG PORT_ENGINE_CHECK)
endif()
target_compile_options(recomp PRIVATE ${HOST_FLAGS} -O1 -Wno-unused-label -Wno-unused-variable)
if(NOT PORT_MOVABLE)
    # SYM() is where the port's link put a name; in the movable build it is
    # the N64 address, syms_<module>.h's own default
    target_compile_options(recomp PRIVATE "SHELL:-include ${GEN}/port_recomp_syms.h")
    add_dependencies(recomp recomp_headers)
endif()

# ---- host side -----------------------------------------------------------------
if(EMSCRIPTEN)
    # emscripten's SDL2 port
    set(SDL2_CFLAGS_OTHER -sUSE_SDL=2)
    set(SDL2_LDFLAGS -sUSE_SDL=2)
else()
find_package(PkgConfig REQUIRED)
if(NOT PORT_64BIT)
    set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib32/pkgconfig:/usr/share/pkgconfig")
endif()
pkg_check_modules(SDL2 REQUIRED sdl2)
# the OpenGL renderer (gfx_gl.c) is optional: without libepoxy, only the
# software one is built
pkg_check_modules(EPOXY epoxy)
endif()
file(GLOB HOST_C ${CMAKE_CURRENT_SOURCE_DIR}/host/*.c)
# --hd-text's built-in font (host/hdtext.c; SIL OFL 1.1, port/fonts/OFL.txt)
set(HDTEXT_FONT ${CMAKE_CURRENT_SOURCE_DIR}/fonts/StardosStencil-Bold.ttf)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gen/hdtext_font.c
    COMMAND ${CMAKE_COMMAND} -DIN=${HDTEXT_FONT} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/gen/hdtext_font.c
            -DNAME=hdtext_builtin_ttf -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.cmake
    DEPENDS ${HDTEXT_FONT} ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed.cmake)
add_library(host OBJECT ${HOST_C} ${CMAKE_CURRENT_BINARY_DIR}/gen/hdtext_font.c)
if(PORT_AUDIO_RECORD)
    target_compile_definitions(host PRIVATE PORT_AUDIO_RECORD)
endif()
# the game's threads' context switch (host/fiber.h): ucontext on the one OS
# thread, or a host thread each, one running at a time.  ucontext is the
# cheaper switch; macOS has deprecated it and WebAssembly has none.  Where
# both are built the environment's PORT_THREADS picks one at startup.
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
target_include_directories(host PRIVATE ${ROOT}/tools/recomp/runtime ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/host ${SDL2_INCLUDE_DIRS})
target_compile_options(host PRIVATE ${HOST_FLAGS} ${SDL2_CFLAGS_OTHER})
target_compile_definitions(host PRIVATE ${PORT_VERSION_DEFINE} PORT_ROM_SHA1="${PORT_ROM_SHA1}")
if(PORT_SDK_TRACE)
    target_compile_definitions(host PRIVATE PORT_SDK_TRACE)
endif()
if(PORT_ROM_DATA)
    target_compile_definitions(host PRIVATE PORT_ROM_DATA)
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
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i.86|amd64|AMD64" AND NOT EMSCRIPTEN)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/gfx.c PROPERTIES COMPILE_OPTIONS "-O3;-msse4.1")
else()
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/gfx.c PROPERTIES COMPILE_OPTIONS "-O3")
endif()

# ---- the arena link (PORT_MOVABLE) -------------------------------------------
# The N64 side's bitcode as one module, through port-arena (bepass/Arena.cpp,
# docs/PORT.md "Movable memory"), to one object.  Otherwise the objects as
# they are.
if(PORT_MOVABLE)
    get_filename_component(LLVM_BINDIR_HINT ${LLVM_CONFIG} DIRECTORY)
    find_program(LLVM_OPT opt HINTS ${LLVM_BINDIR_HINT} REQUIRED)
    find_program(LLVM_LINK llvm-link HINTS ${LLVM_BINDIR_HINT} REQUIRED)
    find_program(LLVM_LLC llc HINTS ${LLVM_BINDIR_HINT} REQUIRED)
    if(PORT_64BIT)
        set(LLC_RELOC -relocation-model=pic)
    elseif(EMSCRIPTEN)
        # (a wasm object for emcc's link: shared memory needs the atomics)
        if(WASM_PTHREAD)
            set(LLC_RELOC -mattr=+atomics,+bulk-memory)
        else()
            set(LLC_RELOC)
        endif()
    else()
        set(LLC_RELOC -relocation-model=static)
    endif()
    # the data files as IR (tools/asm2ll.py), from what asm2x86.py made,
    # with the N64 side's triple and data layout (as clang has them for
    # its flags; port-ilp32's are its target's)
    if(PORT_64BIT AND NOT PORT_LP64)
        set(N64_PROBE --target=${PORT_TRIPLE})
    else()
        set(N64_PROBE ${M32})
        if(CMAKE_C_COMPILER_TARGET)
            list(APPEND N64_PROBE --target=${CMAKE_C_COMPILER_TARGET})
        endif()
    endif()
    # (the N64 side's compiler: under emscripten the system clang, for i386)
    if(EMSCRIPTEN)
        set(N64_PROBE_CC ${PORT_N64_CC})
    else()
        set(N64_PROBE_CC ${CMAKE_C_COMPILER})
    endif()
    execute_process(COMMAND ${N64_PROBE_CC} ${N64_PROBE} -S -emit-llvm -o - -x c /dev/null
        OUTPUT_VARIABLE N64_PROBE_LL COMMAND_ERROR_IS_FATAL ANY)
    if(NOT N64_PROBE_LL MATCHES "target datalayout = \"([^\"]*)\"")
        message(FATAL_ERROR "no data layout from ${N64_PROBE_CC} ${N64_PROBE}")
    endif()
    set(N64_DATALAYOUT ${CMAKE_MATCH_1})
    if(PORT_64BIT AND NOT PORT_LP64)
        set(N64_TRIPLE ${PORT_TRIPLE})
    elseif(N64_PROBE_LL MATCHES "target triple = \"([^\"]*)\"")
        set(N64_TRIPLE ${CMAKE_MATCH_1})
    else()
        message(FATAL_ERROR "no triple from ${CMAKE_C_COMPILER} ${N64_PROBE}")
    endif()
    message(STATUS "the N64 side: ${N64_TRIPLE}")
    set(DATA_LL)
    foreach(s ${DATA_X86})
        get_filename_component(n ${s} NAME_WE)
        get_filename_component(d ${s} NAME)
        string(REGEX REPLACE "\\.s$" ".ll" d ${d})
        add_custom_command(OUTPUT ${GEN}/data_ll/${d}
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2ll.py ${GEN}/data_ll/${d} ${s} ${GEN}/n64_syms.txt
                    ${N64_TRIPLE} ${N64_DATALAYOUT}
            DEPENDS ${s} ${GEN}/n64_syms.txt ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2ll.py)
        list(APPEND DATA_LL ${GEN}/data_ll/${d})
    endforeach()
    file(MAKE_DIRECTORY ${GEN}/data_ll)
    set(ARENA_FLAGS -port-arena-syms=${GEN}/n64_syms.txt -port-arena-header=${GEN}/arena_syms.h
        -port-arena-fn-values=${GEN}/fn_values.txt)
    if(PORT_NATIVE_ENDIAN)
        list(APPEND ARENA_FLAGS -port-arena-native)
    endif()
    if(EMSCRIPTEN)      # (i386's IR to wasm32's; wasm can't call a function
                        # through another type, and its float conversions saturate)
        list(APPEND ARENA_FLAGS -port-arena-triple=wasm32-unknown-emscripten "-port-arena-datalayout=${WASM_DATALAYOUT}"
             -port-arena-typed-calls -port-arena-x86-fptoint)
    endif()
    # the image to the build directory either way (the scan, tools/rom_scan.py,
    # and rom_data.py's check), and out of the module with PORT_ROM_DATA
    list(APPEND ARENA_FLAGS -port-arena-image=${GEN}/arena.img)
    if(PORT_ROM_DATA)
        list(APPEND ARENA_FLAGS -port-arena-no-runs)
    endif()
    add_custom_command(OUTPUT ${GEN}/n64.o ${GEN}/arena_syms.h ${GEN}/arena.img ${GEN}/arena_map.txt
        COMMAND ${LLVM_LINK} -o ${GEN}/n64.linked.bc "$<TARGET_OBJECTS:game_c>" ${NOPOLL_OBJECTS} "$<TARGET_OBJECTS:port_n64>" ${ENGINE_OBJECTS} ${DATA_LL}
        COMMAND ${CMAKE_COMMAND} -E env PORT_ARENA_MAP=${GEN}/arena_map.txt
                ${LLVM_OPT} -load-pass-plugin=${BEPASS} -passes=port-arena ${ARENA_FLAGS} ${GEN}/n64.linked.bc -o ${GEN}/n64.bc
        COMMAND ${LLVM_LLC} -O2 -filetype=obj -data-sections ${LLC_RELOC} ${GEN}/n64.bc -o ${GEN}/n64.o
        DEPENDS game_c ${NOPOLL_TARGETS} port_n64 $<TARGET_OBJECTS:game_c> ${NOPOLL_OBJECTS} $<TARGET_OBJECTS:port_n64> ${ENGINE_OBJECTS} BEPass ${DATA_LL} ${GEN}/n64_syms.txt
                ${GEN}/fn_values.txt
        COMMAND_EXPAND_LISTS)
    add_custom_target(n64_link DEPENDS ${GEN}/n64.o ${GEN}/arena_syms.h)
    if(PORT_ROM_DATA)
        add_custom_command(OUTPUT ${GEN}/romdata_ops.c ${GEN}/romdata_report.txt
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/rom_data.py ${GEN}/arena.img ${PORT_BASEROM}
                    ${ROOT}/build/blastcorps.${PORT_VERSION}.map ${GEN}/romdata_ops.c --version ${PORT_VERSION}
                    --arena-map ${GEN}/arena_map.txt --report ${GEN}/romdata_report.txt
            DEPENDS ${GEN}/arena.img ${GEN}/arena_map.txt ${PORT_BASEROM} ${ROOT}/build/blastcorps.${PORT_VERSION}.map
                    ${CMAKE_CURRENT_SOURCE_DIR}/tools/rom_data.py)
        set_source_files_properties(${GEN}/romdata_ops.c PROPERTIES GENERATED TRUE)
        target_sources(host PRIVATE ${GEN}/romdata_ops.c)
    endif()
    set(N64_OBJECTS ${GEN}/n64.o)
    set(GAME_OBJECTS ${GEN}/n64.o)
    set(N64_OBJECTS_DEPENDS n64_link ${GEN}/n64.o)
else()
    set(N64_OBJECTS $<TARGET_OBJECTS:game_c> ${NOPOLL_OBJECTS} $<TARGET_OBJECTS:port_n64> ${ENGINE_OBJECTS})
    set(GAME_OBJECTS $<TARGET_OBJECTS:game_c> ${NOPOLL_GAME_OBJECTS})
    set(N64_OBJECTS_DEPENDS game_c ${NOPOLL_TARGETS} port_n64 $<TARGET_OBJECTS:game_c> ${NOPOLL_OBJECTS} ${ENGINE_OBJECTS})
endif()

# ---- link ----------------------------------------------------------------------
# The port's image sits right above the 4 MB of RDRAM (port.h), but for the
# movable build's, which has no fixed address.
if(NOT PORT_MOVABLE)
add_custom_command(OUTPUT ${GEN}/port_syms.ld
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py script ${GEN}/port_syms.ld
            "${N64_OBJECTS}" "$<TARGET_OBJECTS:game_data>"
            "$<TARGET_OBJECTS:recomp>" "$<TARGET_OBJECTS:host>"
    DEPENDS ${N64_OBJECTS_DEPENDS} game_data recomp host $<TARGET_OBJECTS:game_data>
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py
    COMMAND_EXPAND_LISTS)
add_custom_command(OUTPUT ${GEN}/port_rdram.ld
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_ld.py script ${GEN}/port_rdram.ld
            "${GAME_OBJECTS}" "$<TARGET_OBJECTS:game_data>"
    DEPENDS ${N64_OBJECTS_DEPENDS} game_data $<TARGET_OBJECTS:game_data>
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_ld.py
    COMMAND_EXPAND_LISTS)
add_custom_target(port_syms DEPENDS ${GEN}/port_syms.ld ${GEN}/port_rdram.ld)
endif()

if(PORT_MOVABLE)
    # the data are in the arena link, and nothing is placed by a script
    add_executable(blastcorps ${N64_OBJECTS} $<TARGET_OBJECTS:recomp> $<TARGET_OBJECTS:host>)
    set_source_files_properties(${GEN}/n64.o PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
    add_dependencies(blastcorps n64_link)
    set_target_properties(blastcorps PROPERTIES LINKER_LANGUAGE C)
else()
    add_executable(blastcorps
        ${N64_OBJECTS} $<TARGET_OBJECTS:game_data>
        $<TARGET_OBJECTS:recomp> $<TARGET_OBJECTS:host>)
    set_target_properties(blastcorps PROPERTIES LINKER_LANGUAGE C LINK_DEPENDS "${GEN}/port_syms.ld;${GEN}/port_rdram.ld")
    add_dependencies(blastcorps port_syms)
endif()
if(PORT_ACCESS_PROFILE)
    target_compile_definitions(host PRIVATE PORT_ACCESS_PROFILE)
    target_compile_definitions(recomp PRIVATE PORT_ACCESS_PROFILE RECOMP_ACCESS)
endif()
if(PORT_64BIT)
    target_compile_definitions(host PRIVATE PORT_64BIT)
    target_compile_definitions(recomp PRIVATE PORT_64BIT)
endif()
if(PORT_ENGINE_CHECK)
    target_compile_definitions(host PRIVATE PORT_ENGINE_CHECK)
endif()
if(EMSCRIPTEN)
    # (the fibers' stacks are in the arena; -O2 at the link too: without it
    # emcc runs no wasm-opt, and Asyncify's instrumentation stays as slow
    # as it was made)
    target_link_options(blastcorps PRIVATE -O2 -sINITIAL_MEMORY=256MB -sSTACK_SIZE=1MB)
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
        set_target_properties(blastcorps PROPERTIES SUFFIX ".html"
            LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
        target_compile_definitions(host PRIVATE PORT_WASM_WEB PORT_HAVE_GL)     # (gfx_gl.c on WebGL 2)
    endif()
elseif(PORT_MOVABLE AND APPLE)
    # (a PIE is all arm64 macOS runs, and ld64's default)
elseif(PORT_MOVABLE AND PORT_64BIT)
    # nothing is at a fixed address: an ordinary PIE, wherever the loader
    # puts it
    target_link_options(blastcorps PRIVATE -pie)
elseif(PORT_MOVABLE)
    target_link_options(blastcorps PRIVATE -m32 -no-pie)
elseif(PORT_64BIT)
    # a position-dependent executable (the image is at fixed addresses), but
    # with the PIE start files: the non-PIE crtbegin.o addresses its data
    # with sign-extended 32-bit immediates, which can't reach 0x80400000
    target_link_options(blastcorps PRIVATE -pie -Wl,--no-pie -Wl,-Ttext-segment=0x80400000)
else()
    target_link_options(blastcorps PRIVATE -m32 -no-pie -Wl,-Ttext-segment=0x80400000)
endif()
if(NOT PORT_MOVABLE)
    target_link_options(blastcorps PRIVATE -Wl,${GEN}/port_syms.ld -Wl,-T,${GEN}/port_rdram.ld -Wl,${GEN}/port_fixed.ld)
endif()
# (the movable build has no --wrap, which ld64 doesn't know: port-wrap did
# the N64 side's calls, and the glue calls the __wrap_s itself)
if(NOT PORT_MOVABLE)
    foreach(w ${PORT_WRAPS})
        target_link_options(blastcorps PRIVATE -Wl,--wrap=${w})
    endforeach()
endif()
target_link_libraries(blastcorps PRIVATE ${SDL2_LDFLAGS} ${EPOXY_LDFLAGS} Threads::Threads m)

# ---- the loaders (host/native.c, src/loads.c) ---------------------------------
# the decompressors' entry points are wrapped, so every load is seen; the
# ROM's segments come from the top-level link map
add_custom_command(OUTPUT ${GEN}/romtab.h
    COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_romtab.py ${GEN}/romtab.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_romtab.py ${ROOT}/build/blastcorps.${PORT_VERSION}.map
            ${BLAST}/include/game/inventory.json)
add_custom_target(romtab DEPENDS ${GEN}/romtab.h)
add_dependencies(host romtab)
target_include_directories(host PRIVATE ${GEN})

# ---- diagnostics: the translated code traps on an address the port doesn't map
option(PORT_EA_GUARD "the translated code checks every address it accesses" OFF)
if(PORT_EA_GUARD)
    target_compile_definitions(recomp PRIVATE RECOMP_EA_GUARD)
endif()

# ---- diagnostics: the translated code traced instruction by instruction
option(PORT_TRACE_ASM "PORT_ITRACE=FILE,FROM,TO: the translated code's instructions and registers" OFF)
if(PORT_TRACE_ASM)
    target_compile_definitions(recomp PRIVATE RECOMP_TRACE)
    target_compile_definitions(host PRIVATE PORT_TRACE_ASM)
endif()

# ---- native-endian memory: the rest of the port follows --------------------
if(PORT_NATIVE_ENDIAN)
    target_compile_definitions(host PRIVATE PORT_NATIVE_ENDIAN)
    target_compile_definitions(recomp PRIVATE PORT_NATIVE_ENDIAN RECOMP_NATIVE_ENDIAN)
    message(STATUS "native-endian memory")
endif()

if(PORT_MOVABLE)
    target_compile_definitions(host PRIVATE PORT_MOVABLE)
    target_compile_definitions(recomp PRIVATE PORT_MOVABLE)
    target_compile_options(recomp PRIVATE "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_arena.h")
    # the names of the version's N64 link (port-arena, asm2ll.py) and their
    # addresses for the host's code (PORT_VAR)
    add_custom_command(OUTPUT ${GEN}/n64_syms.txt ${GEN}/n64_syms.h
        COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py table ${GEN}/n64_syms.txt
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py ${BLAST}/build/hd_code.${PORT_VERSION}.elf
                ${BLAST}/build/hd_front_end.${PORT_VERSION}.elf ${BLAST}/build/init.${PORT_VERSION}.elf)
    add_custom_target(n64_syms DEPENDS ${GEN}/n64_syms.txt ${GEN}/n64_syms.h)
    add_dependencies(host n64_syms n64_link)
    add_dependencies(recomp n64_link)
    # the LP64 build's moved variables first (port-arena's header), then the
    # N64's addresses of the rest
    target_compile_options(host PRIVATE "SHELL:-include ${GEN}/arena_syms.h" "SHELL:-include ${GEN}/n64_syms.h")
    target_compile_options(recomp PRIVATE "SHELL:-include ${GEN}/arena_syms.h")
    message(STATUS "movable memory")
else()
    # every game variable where the N64 has it?
    add_custom_command(TARGET blastcorps POST_BUILD
        COMMAND ${PY} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_ld.py check $<TARGET_FILE:blastcorps>)
endif()

# The test suite (port/tools/test.py; docs/PORT.md, "Testing the port"):
#   ctest --test-dir build/port -L quick -V      deterministic runs against the references
#   ctest --test-dir build/port -L tas -V        the TAS replay (us.v10, 10-20 minutes)
#   ctest --test-dir build/port -L recomp -V     the translated engine's difftest
# Each skips (77) without the ROM, the TAS log or the venv.
enable_testing()
set(PORT_TEST ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/test.py)
add_test(NAME quick COMMAND ${PORT_TEST} quick ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME tas COMMAND ${PORT_TEST} tas ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME recomp COMMAND ${PORT_TEST} recomp)
set_tests_properties(quick PROPERTIES LABELS quick TIMEOUT 1800 SKIP_RETURN_CODE 77)
set_tests_properties(tas PROPERTIES LABELS tas TIMEOUT 5400 SKIP_RETURN_CODE 77)
set_tests_properties(recomp PROPERTIES LABELS recomp TIMEOUT 3600 SKIP_RETURN_CODE 77)
