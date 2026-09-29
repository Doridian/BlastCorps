# The PC port of Blast Corps (us.v11).  See docs/PORT.md.
#
# Needs a built us.v11 stage 2 and the translated engine:
#   make VERSION=us.v11 -C blastcorps && make -C tools/recomp
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
option(PORT_64BIT "a 64-bit program (the game's C through port-ilp32)" OFF)
option(PORT_ILP32_CHECK "64-bit: check every pointer the game's C stores fits in 32 bits" OFF)
# diagnostics (docs/PORT.md, "Comparing builds"): a call trace of the
# game's C (PORT_TRACE=FILE,FROM,TO), the access-width profiler (PORT_ACCESS=FILE)
option(PORT_TRACE_CALLS "the game's C calls __port_trace on every function entry" OFF)
option(PORT_ACCESS_PROFILE "record the width every byte of RDRAM is written and read at" OFF)

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "the port needs clang (BEPass is an LLVM plugin): -DCMAKE_C_COMPILER=clang")
endif()

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(BLAST ${ROOT}/blastcorps)
set(RECOMP_GEN ${BLAST}/build/recomp/src)
set(GEN ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN})
find_package(Python3 REQUIRED COMPONENTS Interpreter)

foreach(f ${BLAST}/build/hd_code.us.v11.elf ${RECOMP_GEN}/recomp_funcs.h)
    if(NOT EXISTS ${f})
        message(FATAL_ERROR "${f} is missing: build blastcorps (us.v11) and tools/recomp first")
    endif()
endforeach()

# ---- BEPass (a host LLVM plugin, 64-bit) ------------------------------------
find_program(LLVM_CONFIG llvm-config REQUIRED)
execute_process(COMMAND ${LLVM_CONFIG} --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(LLVM_CXXFLAGS)
add_library(BEPass MODULE bepass/BEPass.cpp bepass/ILP32.cpp)
target_compile_options(BEPass PRIVATE ${LLVM_CXXFLAGS} -fno-rtti)
set_target_properties(BEPass PROPERTIES PREFIX "lib" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(BEPASS ${CMAKE_BINARY_DIR}/libBEPass.so)

execute_process(COMMAND ${CMAKE_C_COMPILER} -print-resource-dir OUTPUT_VARIABLE CLANG_RESOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
# the N64 side's layout: i386 with 8-byte aligned doubles and long longs,
# as MIPS o32 has them
set(M32 -m32 -msse2 -mfpmath=sse -malign-double)
if(PORT_64BIT)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-target-triple OUTPUT_VARIABLE PORT_TRIPLE OUTPUT_STRIP_TRAILING_WHITESPACE)
    get_filename_component(LLVM_BINDIR_HINT ${LLVM_CONFIG} DIRECTORY)
    find_program(LLVM_OPT opt HINTS ${LLVM_BINDIR_HINT} REQUIRED)
    set(ILP32CC ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/ilp32cc.py --opt ${LLVM_OPT}
        --plugin ${CMAKE_BINARY_DIR}/libBEPass.so --target ${PORT_TRIPLE})
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
if(N64_LAUNCHER)
    list(PREPEND N64_LAUNCHER env)
endif()
if(PORT_64BIT)
    list(PREPEND ILP32CC ${N64_LAUNCHER})
    set(N64_LAUNCHER ${ILP32CC})
    set(HOST_ARCH -fPIE)
    list(APPEND M32 -fPIE)
    message(STATUS "64-bit build for ${PORT_TRIPLE}")
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
foreach(r ${REPLACED})
    list(REMOVE_ITEM GAME_C ${BLAST}/src/${r}.c)
endforeach()

set(N64_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${BLAST} ${BLAST}/include ${BLAST}/include/2.0I ${BLAST}/include/2.0I/PR)
set(N64_DEFINES _LANGUAGE_C _FINALROM TARGET_PC VERSION_US_V11 _MIPS_SZLONG=32 _MIPS_SZINT=32)
if(PORT_64BIT)
    list(APPEND N64_DEFINES PORT_64BIT)
endif()
if(PORT_ACCESS_PROFILE)
    list(APPEND N64_DEFINES PORT_ACCESS_PROFILE)
endif()
# The decompiled C is IDO's dialect: K&R definitions, implicit
# declarations, prototypes that differ between files, pointers and ints
# used interchangeably.  These flags make clang accept it and not optimise
# on the undefined behaviour it is full of.
set(N64_FLAGS ${M32} -std=gnu89 -O2 -g -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks
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
    -Wno-macro-redefined)

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

# ---- the libultra replacement (N64 side, BEPass too) --------------------------
file(GLOB PORT_N64_C ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_library(port_n64 OBJECT ${PORT_N64_C})
target_include_directories(port_n64 PRIVATE ${N64_INCLUDES})
target_compile_definitions(port_n64 PRIVATE ${N64_DEFINES})
target_compile_options(port_n64 PRIVATE ${N64_FLAGS} -Wno-implicit-function-declaration)
add_dependencies(port_n64 BEPass)
set_source_files_properties(${PORT_N64_C} PROPERTIES OBJECT_DEPENDS ${BEPASS})
if(N64_LAUNCHER)
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
foreach(s ${DATA_S})
    file(RELATIVE_PATH rel ${BLAST}/asm ${s})
    string(REPLACE "/" "_" out ${rel})
    set(label "")
    if(s MATCHES "/([a-z_]+)/([0-9A-F]+)\\.bin\\.s$")
        set(label ${CMAKE_MATCH_1}_${CMAKE_MATCH_2}_bin)
    endif()
    add_custom_command(OUTPUT ${GEN}/data/${out}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py ${GEN}/data/${out} ${s} ${label}
        DEPENDS ${s} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py)
    list(APPEND DATA_X86 ${GEN}/data/${out})
endforeach()
foreach(b ${DATA_BIN})
    string(REPLACE "/" "_" label ${b})
    add_custom_command(OUTPUT ${GEN}/data/${label}.bin.s
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py ${GEN}/data/${label}.bin.s ${BLAST}/assets/${b}.bin ${label}_bin
        DEPENDS ${BLAST}/assets/${b}.bin ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm2x86.py)
    list(APPEND DATA_X86 ${GEN}/data/${label}.bin.s)
endforeach()
file(MAKE_DIRECTORY ${GEN}/data)
set_source_files_properties(${DATA_X86} PROPERTIES LANGUAGE ASM GENERATED TRUE)
add_library(game_data OBJECT ${DATA_X86})
if(NOT PORT_64BIT)
    target_compile_options(game_data PRIVATE -m32)
endif()
target_compile_options(game_data PRIVATE -Wno-unused-command-line-argument)

# ---- the translated handwritten engine (host side) ---------------------------
file(GLOB RECOMP_C ${RECOMP_GEN}/hd_code_*.c ${RECOMP_GEN}/hd_front_end_*.c)
add_custom_command(OUTPUT ${GEN}/port_recomp_syms.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py header ${GEN}/port_recomp_syms.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py ${RECOMP_GEN}/syms_hd_code.h ${RECOMP_GEN}/syms_hd_front_end.h)
add_custom_command(OUTPUT ${GEN}/entry.c ${GEN}/externs.c ${GEN}/glue_report.txt
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_glue.py ${GEN}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_glue.py ${CMAKE_CURRENT_SOURCE_DIR}/tools/liveness.py
            ${RECOMP_GEN}/recomp_funcs.h ${RECOMP_GEN}/recomp_externs.h ${GAME_C})
add_custom_target(recomp_headers DEPENDS ${GEN}/port_recomp_syms.h)

set(HOST_FLAGS ${HOST_ARCH} -O2 -g -ffp-contract=off -fno-strict-aliasing -Wall)
add_library(recomp OBJECT ${RECOMP_C} ${GEN}/entry.c ${GEN}/externs.c)
target_include_directories(recomp PRIVATE ${ROOT}/tools/recomp/runtime ${RECOMP_GEN} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(recomp PRIVATE RECOMP_COUNT)
target_compile_options(recomp PRIVATE ${HOST_FLAGS} -O1 -Wno-unused-label -Wno-unused-variable
    -include ${GEN}/port_recomp_syms.h)
add_dependencies(recomp recomp_headers)

# ---- host side -----------------------------------------------------------------
find_package(PkgConfig REQUIRED)
if(NOT PORT_64BIT)
    set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib32/pkgconfig:/usr/share/pkgconfig")
endif()
pkg_check_modules(SDL2 REQUIRED sdl2)
# the OpenGL renderer (gfx_gl.c) is optional: without libepoxy, only the
# software one is built
pkg_check_modules(EPOXY epoxy)
file(GLOB HOST_C ${CMAKE_CURRENT_SOURCE_DIR}/host/*.c)
add_library(host OBJECT ${HOST_C})
target_include_directories(host PRIVATE ${ROOT}/tools/recomp/runtime ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/host ${SDL2_INCLUDE_DIRS})
target_compile_options(host PRIVATE ${HOST_FLAGS} ${SDL2_CFLAGS_OTHER})
if(EPOXY_FOUND)
    target_compile_definitions(host PRIVATE PORT_HAVE_GL)
    target_include_directories(host PRIVATE ${EPOXY_INCLUDE_DIRS})
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i.86|amd64|AMD64")
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/gfx.c PROPERTIES COMPILE_OPTIONS "-O3;-msse4.1")
else()
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/host/gfx.c PROPERTIES COMPILE_OPTIONS "-O3")
endif()

# ---- link ----------------------------------------------------------------------
# The port's image sits right above the 4 MB of RDRAM (port.h).
add_custom_command(OUTPUT ${GEN}/port_syms.ld
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py script ${GEN}/port_syms.ld
            "$<TARGET_OBJECTS:game_c>" "$<TARGET_OBJECTS:port_n64>" "$<TARGET_OBJECTS:game_data>"
            "$<TARGET_OBJECTS:recomp>" "$<TARGET_OBJECTS:host>"
    DEPENDS game_c port_n64 game_data recomp host $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:game_data>
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_syms.py
    COMMAND_EXPAND_LISTS)
add_custom_command(OUTPUT ${GEN}/port_rdram.ld
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_ld.py script ${GEN}/port_rdram.ld
            "$<TARGET_OBJECTS:game_c>" "$<TARGET_OBJECTS:game_data>"
    DEPENDS game_c game_data $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:game_data>
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_ld.py
    COMMAND_EXPAND_LISTS)
add_custom_target(port_syms DEPENDS ${GEN}/port_syms.ld ${GEN}/port_rdram.ld)

add_executable(blastcorps
    $<TARGET_OBJECTS:game_c> $<TARGET_OBJECTS:port_n64> $<TARGET_OBJECTS:game_data>
    $<TARGET_OBJECTS:recomp> $<TARGET_OBJECTS:host>)
set_target_properties(blastcorps PROPERTIES LINKER_LANGUAGE C LINK_DEPENDS "${GEN}/port_syms.ld;${GEN}/port_rdram.ld")
add_dependencies(blastcorps port_syms)
if(PORT_ACCESS_PROFILE)
    target_compile_definitions(host PRIVATE PORT_ACCESS_PROFILE)
    target_compile_definitions(recomp PRIVATE PORT_ACCESS_PROFILE RECOMP_ACCESS)
endif()
if(PORT_64BIT)
    target_compile_definitions(host PRIVATE PORT_64BIT)
    target_compile_definitions(recomp PRIVATE PORT_64BIT)
    # a position-dependent executable (the image is at fixed addresses), but
    # with the PIE start files: the non-PIE crtbegin.o addresses its data
    # with sign-extended 32-bit immediates, which can't reach 0x80400000
    target_link_options(blastcorps PRIVATE -pie -Wl,--no-pie)
else()
    target_link_options(blastcorps PRIVATE -m32 -no-pie)
endif()
target_link_options(blastcorps PRIVATE -Wl,-Ttext-segment=0x80400000 -Wl,--wrap=func_8028B3E0 -Wl,${GEN}/port_syms.ld -Wl,-T,${GEN}/port_rdram.ld
    -Wl,${CMAKE_CURRENT_SOURCE_DIR}/port_fixed.ld)
target_link_libraries(blastcorps PRIVATE ${SDL2_LDFLAGS} ${EPOXY_LDFLAGS} m)

# every game variable where the N64 has it?
add_custom_command(TARGET blastcorps POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_ld.py check $<TARGET_FILE:blastcorps>)
