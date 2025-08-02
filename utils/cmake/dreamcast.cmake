# Auxiliary CMake Utility Functions
#   Copyright (C) 2023 Colton Pawielski
#   Copyright (C) 2024, 2025 Falco Girgis
#   Copyright (C) 2024 Paul Cercueil
#
# This file implements utilities for the following additional functionality
# which exists in the KOS Make build system:
#   1) linking to existing binaries
#   2) adding a romdisk
#   3) running the binary using KOS_LOADER
#
# NOTE: When using the KOS CMake toolchain file, you do not need to include
#       this file directly!

### This minimum is based on the minimum requirement in dreamcast.toolchain.cmake
cmake_minimum_required(VERSION 3.13)

#### Set Configuration Variables From Environment (if Necessary) ####
if(NOT DEFINED KOS_BASE)
    if(NOT DEFINED ENV{KOS_BASE})
        message(FATAL_ERROR "KOS_BASE environment variable not found!")
    else()
        set(KOS_BASE $ENV{KOS_BASE})
    endif()
endif()

if(NOT DEFINED KOS_CC_BASE)
    if(NOT DEFINED ENV{KOS_CC_BASE})
        message(FATAL_ERROR "KOS_CC_BASE environment variable not found!")
    else()
        set(KOS_CC_BASE $ENV{KOS_CC_BASE})
    endif()
endif()

### Adds a custom "run" target which uses $KOS_LOADER to run the given target. ###
function(kos_run_target target)
    add_custom_target(run 
        COMMAND $ENV{KOS_LOADER} $<TARGET_FILE:${target}> 
        DEPENDS ${target} 
        USES_TERMINAL
    )
endfunction()

### Helper Function for Bin2Object ###
function(kos_bin2o inFile symbol)
    # outFile is optional and defaults to the symbol name in the build directory
    if(NOT ${ARGC} EQUAL 3)
        set(outFile ${CMAKE_CURRENT_BINARY_DIR}/${symbol}.o)
    else()
        set(outFile ${ARGN})
    endif()

    # Custom Command to generate romdisk object file from image
    add_custom_command(
        OUTPUT  ${outFile}
        DEPENDS ${inFile}
        COMMAND ${KOS_BASE}/utils/bin2o/bin2o ${inFile} ${symbol} ${outFile}
    )
endfunction()

function(kos_add_binary target inFile symbol)
    file(REAL_PATH "${inFile}" inFile)
    set(outFile ${CMAKE_CURRENT_BINARY_DIR}/${symbol}.o)
    kos_bin2o(${inFile} ${symbol} ${outFile})
    target_sources(${target} PRIVATE ${outFile})
endfunction()

### Helper Function for Generating Romdisk ###
function(kos_add_romdisk target romdiskPath)
    # Name is optional and defaults to "romdisk"
    if(NOT ${ARGC} EQUAL 3)
        set(romdiskName romdisk)
    else()
        set(romdiskName ${ARGN})
    endif()

    file(REAL_PATH "${romdiskPath}" romdiskPath)

    if(CMAKE_CXX_COMPILER_LOADED)
        set(tmpExt cpp)
    else()
        set(tmpExt c)
    endif()

    set(c_tmp   ${CMAKE_CURRENT_BINARY_DIR}/${romdiskName}_tmp.${tmpExt})
    set(img     ${CMAKE_CURRENT_BINARY_DIR}/${romdiskName}.img)

    # Variable holding all files in the romdiskPath folder
    # CONFIGURE_DEPENDS causes the folder to always be rechecked
    # at build time not only when CMake is re-run
    file(GLOB romdiskFiles CONFIGURE_DEPENDS
        "${romdiskPath}/*"
    )
    
    # Custom Command to generate romdisk image from folder
    # Only run when folder contents have changed by depending on 
    # the romdiskFiles variable
    add_custom_command(
        OUTPUT  ${img}
        DEPENDS ${romdiskFiles}
        COMMAND ${KOS_BASE}/utils/genromfs/genromfs -f ${img} -d ${romdiskPath} -v
    )

    add_custom_command(
        OUTPUT ${c_tmp}
        DEPENDS ${img}
        COMMAND ${KOS_BASE}/utils/bin2c/bin2c ${img} ${c_tmp} romdisk
    )

    # Append romdisk object to target
    target_sources(${target} PRIVATE ${c_tmp})
    target_link_options(${target} PRIVATE -Wl,--whole-archive -lromdiskbase -Wl,--no-whole-archive)
endfunction()
