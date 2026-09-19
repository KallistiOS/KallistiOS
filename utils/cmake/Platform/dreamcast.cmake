# CMake platform file for the Dreamcast.
#  Copyright (C) 2024 Paul Cercueil

include(Platform/Generic)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

# Default CMake installations to install to the sysroot
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SYSROOT}" CACHE PATH "Install prefix" FORCE)
endif()

set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR SH4)
set(CMAKE_SIZEOF_VOID_P 4)
set(CMAKE_C_BYTE_ORDER LITTLE_ENDIAN)
set(CMAKE_CXX_BYTE_ORDER LITTLE_ENDIAN)
set(CMAKE_OBJC_BYTE_ORDER LITTLE_ENDIAN)
set(CMAKE_OBJCXX_BYTE_ORDER LITTLE_ENDIAN)
set(PLATFORM_DREAMCAST TRUE)

# CMake defaults to -fno-fat-lto-objects when using LTO.
# We actually do want fat LTO objects.
set(CMAKE_C_COMPILE_OPTIONS_IPO -flto=auto -ffat-lto-objects)
set(CMAKE_CXX_COMPILE_OPTIONS_IPO ${CMAKE_C_COMPILE_OPTIONS_IPO})
