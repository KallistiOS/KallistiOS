# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

# Catch-all: CC
GCC = gcc
ifeq ($(CC),)
  CC := $(GCC)
endif

# Catch-all: CXX
GXX = g++
ifeq ($(CXX),)
  CXX := $(GXX)
endif

# Detect where Bash is installed
SHELL = /bin/bash
ifdef FREEBSD
  SHELL = /usr/local/bin/bash
endif

# MinGW/MSYS
# Check the MSYS POSIX emulation layer version...
#
# This is needed as a severe bug exists in the '/bin/msys-1.0.dll' file.
# Indeed, the heap size is hardcoded and too small, which causes the error:
#
#   Couldn't commit memory for cygwin heap, Win32 error
#
# The only solution to build gcc for sh-elf target under MinGW/MSYS is to
# temporarily replace the '/bin/msys-1.0.dll' shipped with MinGW environment
# (which is the v1.0.19 at this time) with the patched version provided in the
# ./doc/mingw/ directory. You just need the 'msys-1.0.dll' file from this
# package. Copy the original file somewhere outside your MinGW/MSYS installation
# and replace it with the provided patched version.
#
# The patched version is coming from the C::B Advanced package. The major fix
# applied is the heap size increase, from 256 MB to more than 1024 MB.
#
# After building the whole toolchain, please remove the patched version and move
# the original '/bin/msys-1.0.dll' file back in place.
#
ifdef MINGW
  msys_patched_checksum = 2e627b60938fb8894b3536fc8fe0587a5477f570
  msys_checksum = $(shell sha1sum /bin/msys-1.0.dll | cut -c-40)
  ifneq ($(msys_checksum),$(msys_patched_checksum))
    $(warning Please consider temporarily patching '/bin/msys-1.0.dll')
  endif
endif

# SH toolchain
sh_target = sh-elf
SH_CC_FOR_TARGET = $(sh_target)-$(GCC)
SH_CXX_FOR_TARGET = $(sh_target)-$(GXX)

# ARM toolchain
arm_target = arm-eabi

# Handle macOS
ifdef MACOS
  ifdef MACOS_MOJAVE_AND_UP
    # Starting from macOS Mojave (10.14+)
    sdkroot = $(shell xcrun --sdk macosx --show-sdk-path)
    macos_extra_args = -isysroot $(sdkroot)
    CC += -Wno-nullability-completeness -Wno-missing-braces $(macos_extra_args)
    CXX += -stdlib=libc++ -mmacosx-version-min=10.7 $(macos_extra_args)
    SH_CC_FOR_TARGET += $(macos_extra_args)
    SH_CXX_FOR_TARGET += $(macos_extra_args)
    macos_gcc_configure_args = --with-sysroot --with-native-system-header=/usr/include
    macos_gdb_configure_args = --with-sysroot=$(sdkroot)
  endif
endif

# Handle Cygwin
ifdef CYGWIN
  CC += -D_GNU_SOURCE
  CXX += -D_GNU_SOURCE
endif

# Set static flags to pass to configure if needed
ifeq ($(standalone_binary),1)
  ifndef MINGW
    $(warning 'standalone_binary' should be used *ONLY* on MinGW/MSYS environment)
  endif
  # Note the extra minus before -static
  # See: https://stackoverflow.com/a/29055118/3726096
  static_flag := --enable-static --disable-shared "LDFLAGS=--static"
endif

# For all Windows systems (i.e. Cygwin, MinGW/MSYS and MinGW-w64/MSYS2)
ifdef WINDOWS
  executable_extension=.exe
endif

# Set up the argument for jobs to be used if specified
ifdef makejobs
  ifneq (1,$(makejobs))
    jobs_arg = -j$(makejobs)
  endif
endif

# MinGW/MSYS
# Disable makejobs possibility in legacy MinGW/MSYS environment as this breaks
# the build
ifdef MINGW
  ifneq ($(jobs_arg),)
    $(warning 'makejobs' is unsupported in this environment.  Ignoring.)
    jobs_arg=
  endif
endif

# Determine if we want to apply fixup sh4 newlib
do_auto_fixup_sh4_newlib := 1
ifdef auto_fixup_sh4_newlib
  ifeq (0,$(auto_fixup_sh4_newlib))
    $(warning 'Disabling Newlib Auto Fixup)
    do_auto_fixup_sh4_newlib := 0
  endif
endif

# Determine if we want to apply KOS patches to GCC/Newlib/Binutils
do_kos_patching := 1
ifdef use_kos_patches
  ifeq (0,$(use_kos_patches))
    $(warning 'Disabling KOS Patches)
    do_kos_patching := 0
  endif
endif

# Report an error if KOS threading is enabled when patching or fixup is disabled
ifeq (kos,$(thread_model))
  ifeq (0,$(do_auto_fixup_sh4_newlib))
    $(error kos thread model is unsupported when Newlib fixup is disabled)
  endif
  ifeq (0,$(do_kos_patching))
    $(error kos thread model is unsupported when KOS patches are disabled)
  endif
endif

# Handle Newlib configuration options
ifdef newlib_c99_formats
  ifneq (0,$(newlib_c99_formats))
    newlib_extra_configure_args += --enable-newlib-io-c99-formats
  endif
endif

ifdef newlib_opt_space
  ifneq (0,$(newlib_opt_space))
    newlib_extra_configure_args += --enable-target-optspace
  endif
endif

ifdef newlib_multibyte
  ifneq (0,$(newlib_multibyte))
    newlib_extra_configure_args += --enable-newlib-mb
  endif
endif

# Handle install mode for toolchain debug symbols
ifdef toolchain_debug
  ifneq (0,$(toolchain_debug))
    install_mode = install
  else
    install_mode = install-strip
  endif
else
  install_mode = install-strip
endif

# Handle if Native Language Support is used
ifdef disable_nls
  ifneq (0,$(disable_nls))
    extra_configure_args += --disable-nls
    binutils_extra_configure_args += --disable-nls
  endif
endif

# Handle language support options
enabled_languages := c

ifdef enable_cpp
  ifneq (0,$(enable_cpp))
    enabled_languages := $(enabled_languages),c++
  endif
endif

ifdef enable_objc
  ifneq (0,$(enable_objc))
    enabled_languages := $(enabled_languages),objc
  endif
endif

ifdef enable_objcpp
  ifneq (0,$(enable_objcpp))
    enabled_languages := $(enabled_languages),obj-c++
  endif
endif

ifdef enable_d
  ifneq (0,$(enable_d))
    enabled_languages := $(enabled_languages),d
  endif
endif

ifdef enable_fortran
  ifneq (0,$(enable_fortran))
    enabled_languages := $(enabled_languages),fortran
  endif
endif

ifdef enable_modula2
  ifneq (0,$(enable_modula2))
    enabled_languages := $(enabled_languages),m2
  endif
endif

ifdef enable_ada
  ifneq (0,$(enable_ada))
    enabled_languages := $(enabled_languages),ada
  endif
endif

ifdef enable_go
  ifneq (0,$(enable_go))
    enabled_languages := $(enabled_languages),go
  endif
endif

ifdef enable_rust
  ifneq (0,$(enable_rust))
    enabled_languages := $(enabled_languages),rust
  endif
endif

ifdef enable_libgccjit
  ifneq (0,$(enable_libgccjit))
    enabled_languages := $(enabled_languages),jit
    extra_configure_args += --enable-host-shared
  endif
endif

# Function to verify variable is not empty
# Args:
# 1 - Variable Name
verify_not_empty = $(if $($(1)),,$(error $(1) cannot be empty))

# Function to warn and fallback from one variable name to another
# Args:
# 1 - Current Variable Name
# 2 - Fallback Variable Name
warn_and_fallback = $(if $($(1)),, \
                      $(warning $(1) not defined, falling back to $(2)) \
                      $(call verify_not_empty,$(2)) \
                      $(1) ?= $($(2)) \
                      )


# Fallback to _tarball_type config options if _download_type was not provided
packages = gdb
$(foreach package, $(packages), $(eval $(call warn_and_fallback,$(package)_download_type,$(package)_tarball_type)))
