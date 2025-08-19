# CMake Toolchain file for targeting the Dreamcast or NAOMI with CMake.
#  Copyright (C) 2023 Luke Benstead
#  Copyright (C) 2023, 2024, 2025 Falco Girgis
#  Copyright (C) 2024 Donald Haase
#  Copyright (C) 2024 Paul Cercueil
#
# This file is to be passed to CMake when compiling a regular CMake project
# to cross-compile for the Dreamcast, using the KOS environment and settings.
#
#     cmake /path/to/src -DCMAKE_TOOLCHAIN_FILE=${KOS_BASE}/kos/utils/cmake/kallistios.toolchain.cmake
#
#   or alternatively:
#
#     cmake /path/to/src -DCMAKE_TOOLCHAIN_FILE=${KOS_CMAKE_TOOLCHAIN}
#
#   or even:
#
#     kos-cmake /path/to/src
#
# Frame pointers are enabled in debug builds as these are required for
# stack traces and GDB. They are disabled in release.
#
# The original toolchain file was created by Kazade for the Simulant
# engine who has graciously allowed the rest of the scene to copy it.

#### This minimum is due to the use of add_link_options
cmake_minimum_required(VERSION 3.13)

#### Set Configuration Variables From Environment ####
if(NOT DEFINED ENV{KOS_BASE}
   OR NOT DEFINED ENV{KOS_CC_BASE}
   OR NOT DEFINED ENV{KOS_ARCH}
   OR NOT DEFINED ENV{KOS_SUBARCH}
   OR NOT DEFINED ENV{KOS_PORTS}
   OR NOT DEFINED ENV{DC_TOOLS_BASE})
    message(FATAL_ERROR "KallistiOS environment variables not found")
endif()

file(REAL_PATH $ENV{KOS_BASE} KOS_BASE EXPAND_TILDE)
file(REAL_PATH $ENV{KOS_CC_BASE} KOS_CC_BASE EXPAND_TILDE)
file(REAL_PATH $ENV{KOS_PORTS} KOS_PORTS EXPAND_TILDE)
file(REAL_PATH $ENV{DC_TOOLS_BASE} DC_TOOLS_BASE EXPAND_TILDE)
set(KOS_ARCH $ENV{KOS_ARCH})
set(KOS_SUBARCH $ENV{KOS_SUBARCH})
set(KOS_ADDONS ${KOS_BASE}/addons)

list(APPEND CMAKE_MODULE_PATH $ENV{KOS_BASE}/utils/cmake)

##### Configure CMake System #####
set(CMAKE_SYSTEM_NAME ${KOS_ARCH})

##### Configure Cross-Compiler #####
set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_ASM_COMPILER    ${KOS_BASE}/utils/build_wrappers/kos-cc)
set(CMAKE_C_COMPILER      ${KOS_BASE}/utils/build_wrappers/kos-cc)
set(CMAKE_CXX_COMPILER    ${KOS_BASE}/utils/build_wrappers/kos-c++)
set(CMAKE_OBJC_COMPILER   ${KOS_BASE}/utils/build_wrappers/kos-cc)
set(CMAKE_OBJCXX_COMPILER ${KOS_BASE}/utils/build_wrappers/kos-c++)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

# Never use the CMAKE_FIND_ROOT_PATH to find programs with find_program()
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Set sysroot to kos-ports folder
set(CMAKE_SYSROOT ${KOS_PORTS})
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${KOS_PORTS})

set(ENABLE_DEBUG_FLAGS   $<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>)
set(ENABLE_RELEASE_FLAGS $<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>)

add_compile_options(
    "$<${ENABLE_DEBUG_FLAGS}:-DFRAME_POINTERS;-fno-omit-frame-pointer>"
    "$<${ENABLE_RELEASE_FLAGS}:-fomit-frame-pointer>"
    )

set(CMAKE_ASM_FLAGS "")
set(CMAKE_ASM_FLAGS_RELEASE "")

# Default CMake installations to install to kos-addons
set(CMAKE_INSTALL_BINDIR     ${DC_TOOLS_BASE})
if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
    set(CMAKE_INSTALL_INCLUDEDIR ${KOS_ADDONS}/include/${KOS_ARCH})
endif()
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR ${KOS_ADDONS}/lib/${KOS_ARCH})
endif()
