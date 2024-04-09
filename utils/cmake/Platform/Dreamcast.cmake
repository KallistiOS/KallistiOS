# CMake platform file for the Dreamcast.
#  Copyright (C) 2024 Paul Cercueil

include(Platform/Generic)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "" CACHE PATH "Install prefix" FORCE)
endif()
