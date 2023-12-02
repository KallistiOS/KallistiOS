cmake_minimum_required(VERSION 3.23)

### Helper Function for Generating Romdisk ###
function(generate_romdisk target romdiskName romdiskPath)
    set(obj ${CMAKE_CURRENT_BINARY_DIR}/${romdiskName}.o)
    set(img ${CMAKE_CURRENT_BINARY_DIR}/${romdiskName}.img)

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
        OUTPUT ${img}
        COMMAND $ENV{KOS_GENROMFS} -f ${img} -d ${romdiskPath} -v
        DEPENDS ${romdiskFiles}
)

  # Custom Command to generate romdisk object file from image
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${romdiskName}.o
    COMMAND $ENV{KOS_BASE}/utils/bin2o/bin2o ${img} ${romdiskName} ${obj}
    DEPENDS ${img}
    )

    # Append romdisk object to target
    target_sources(${target} PRIVATE ${obj})
endfunction()

### Function to Enable SH4 Math Optimizations ###
function(enable_sh4_math)
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
