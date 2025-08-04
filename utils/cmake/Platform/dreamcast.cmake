# CMake platform file for the Dreamcast.
#  Copyright (C) 2024 Paul Cercueil

include(Platform/Generic)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "" CACHE PATH "Install prefix" FORCE)
endif()

set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR SH4)
set(CMAKE_SIZEOF_VOID_P 4)
set(PLATFORM_DREAMCAST TRUE)
