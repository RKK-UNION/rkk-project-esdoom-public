cmake_minimum_required(VERSION 3.16)
project(esdoom C)

set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

set(DOOM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../external/doomgeneric-master/doomgeneric")
set(HOSTED_DIR "${CMAKE_BINARY_DIR}/hosted")
set(OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../game/mods/esdoom")

find_program(ESDOOM_PYTHON NAMES python python3 py)
if(NOT ESDOOM_PYTHON)
    message(FATAL_ERROR "python is required to patch doomgeneric")
endif()

execute_process(
    COMMAND "${ESDOOM_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/apply_hosted.py"
        "${DOOM_SRC}"
        "${HOSTED_DIR}"
    RESULT_VARIABLE APPLY_RC
)
if(NOT APPLY_RC EQUAL 0)
    message(FATAL_ERROR "apply_hosted.py failed")
endif()

file(GLOB HOSTED_SOURCES "${HOSTED_DIR}/*.c")
if(NOT HOSTED_SOURCES)
    message(FATAL_ERROR "no hosted C sources in ${HOSTED_DIR}")
endif()

set(OPL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opl")

add_library(doom SHARED
    doom_bridge.c
    doomgeneric_renpy.c
    i_renpysound.c
    ${OPL_DIR}/opl.c
    ${OPL_DIR}/opl_host.c
    ${OPL_DIR}/opl_queue.c
    ${OPL_DIR}/opl3.c
    ${OPL_DIR}/i_oplmusic.c
    ${OPL_DIR}/midifile.c
    ${HOSTED_SOURCES}
)

target_include_directories(doom PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${OPL_DIR}"
    "${HOSTED_DIR}"
)

set_source_files_properties(
    ${OPL_DIR}/opl.c
    ${OPL_DIR}/opl_host.c
    ${OPL_DIR}/opl_queue.c
    ${OPL_DIR}/opl3.c
    ${OPL_DIR}/i_oplmusic.c
    ${OPL_DIR}/midifile.c
    PROPERTIES COMPILE_OPTIONS "-include;${OPL_DIR}/opl_compat.h"
)

target_compile_definitions(doom PRIVATE
    DOOM_HOSTED=1
    FEATURE_SOUND=1
)

if(MSVC)
    message(FATAL_ERROR "This build uses the CLion MinGW GCC, not MSVC")
endif()

target_compile_options(doom PRIVATE
    -O2
    -fno-strict-aliasing
    -fvisibility=hidden
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-unused-function
    -Wno-sign-compare
    -Wno-pointer-sign
    -Wno-implicit-function-declaration
    -Wno-error=implicit-function-declaration
    -Wno-int-conversion
    -Wno-error=int-conversion
    -Wno-incompatible-pointer-types
    -Wno-discarded-qualifiers
    -Wno-format
    -Wno-format-overflow
    -Wno-stringop-truncation
    -Wno-array-parameter
    -Wno-deprecated-declarations
)

if(WIN32)
    set_target_properties(doom PROPERTIES
        OUTPUT_NAME "rkk_esdoom_lib"
        PREFIX ""
        SUFFIX ".dll"
        RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
    )
    target_link_options(doom PRIVATE
        -static-libgcc
        -Wl,-Bstatic
        -lwinpthread
        -Wl,-Bdynamic
        -Wl,--exclude-all-symbols
    )
    target_link_libraries(doom PRIVATE winmm kernel32)
else()
    set_target_properties(doom PROPERTIES
        OUTPUT_NAME "rkk_esdoom_lib"
        PREFIX ""
        SUFFIX ".so"
        LIBRARY_OUTPUT_DIRECTORY "${OUT_DIR}"
    )
    target_compile_options(doom PRIVATE -fPIC)
    target_link_options(doom PRIVATE
        -static-libgcc
        -Wl,--exclude-libs,libgcc.a
    )
    target_link_libraries(doom PRIVATE m pthread dl)
endif()
