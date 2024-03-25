cmake_minimum_required(VERSION 3.23)

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
        COMMAND $ENV{KOS_BASE}/utils/bin2o/bin2o ${inFile} ${symbol} ${outFile}
    )
endfunction()

function(kos_add_binary target inFile symbol)
    set(outFile ${CMAKE_CURRENT_BINARY_DIR}/${symbol}.o)
    kos_bin2o(${CMAKE_CURRENT_SOURCE_DIR}/${inFile} ${symbol} ${outFile})
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

    set(romdiskPath ${CMAKE_CURRENT_SOURCE_DIR}/${romdiskPath})

    set(obj     ${CMAKE_CURRENT_BINARY_DIR}/${romdiskName}.o)
    set(obj_tmp ${CMAKE_CURRENT_BINARY_DIR}/${romdiskName}_tmp.o)
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
        COMMAND $ENV{KOS_BASE}/utils/genromfs/genromfs -f ${img} -d ${romdiskPath} -v
    )

    kos_bin2o(${img} ${romdiskName} ${obj_tmp})

    # Custom Command to generate romdisk object file from image
    add_custom_command(
        OUTPUT  ${obj}
        DEPENDS ${obj_tmp}
        COMMAND ${CMAKE_C_COMPILER} -o ${obj} -r ${obj_tmp} -L$ENV{KOS_BASE}/lib/dreamcast -Wl,--whole-archive -lromdiskbase
        COMMAND rm ${obj_tmp}
    )

    # Append romdisk object to target
    target_sources(${target} PRIVATE ${obj})
endfunction()

### Function to Enable SH4 Math Optimizations ###
function(kos_enable_sh4_math)
    if(NOT ${PLATFORM_DREAMCAST})
        message(WARN " PLATFORM_DREAMCAST not set, skipping SH4 Math flags")
        return()
    endif()

    message(INFO " Enabling SH4 Math Optimizations")
    add_compile_options(-ffast-math)

    # Check if -mfsrra and -mfsca are supported by the compiler
    # They were added for GCC 4.8, so the Legacy GCC4.7 toolchain
    # will complain if they are added.
    include(CheckCCompilerFlag)
    check_c_compiler_flag("-mfsrra" COMPILER_HAS_FSRRA)
    check_c_compiler_flag("-mfsca"  COMPILER_HAS_FSCA)
    if(COMPILER_HAS_FSRRA)
        add_compile_options(-mfsrra)
    else()
        message(WARN " Must have GCC4.8 or later for -mfsrra to be enabled")
    endif()

    if(COMPILER_HAS_FSCA)
        add_compile_options(-mfsca)
    else()
        message(WARN " Must have GCC4.8 or later for -mfsca to be enabled")
    endif()

endfunction()
