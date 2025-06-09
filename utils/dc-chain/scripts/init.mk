# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

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

CC_FOR_TARGET = $(target)-$(GCC)
CXX_FOR_TARGET = $(target)-$(GXX)

# Handle macOS
ifdef MACOS
  ifdef MACOS_MOJAVE_AND_UP
    # Starting from macOS Mojave (10.14+)
    sdkroot = $(shell xcrun --sdk macosx --show-sdk-path)
    macos_extra_args = -isysroot $(sdkroot)
    CC += -Wno-nullability-completeness -Wno-missing-braces $(macos_extra_args)
    CXX += -stdlib=libc++ -mmacosx-version-min=10.14 $(macos_extra_args)
    CC_FOR_TARGET += $(macos_extra_args)
    CXX_FOR_TARGET += $(macos_extra_args)
    macos_gcc_configure_args = --with-sysroot --with-native-system-header=/usr/include
    macos_gdb_configure_args = --with-sysroot=$(sdkroot)
    # Detect if CC is Apple Clang and get major version, skip if using gcc.
    APPLE_CLANG_MAJOR := $(shell $(CC) --version 2>&1 | \
      grep -i "Apple clang" | cut -f 4 -d " " | cut -f 1 -d ".")
    ifdef APPLE_CLANG_MAJOR
      # When using Apple Clang 17 or above, use system zlib.
      ifeq ($(shell [ "$(APPLE_CLANG_MAJOR)" -gt 16 ] && echo "yes"), yes)
        $(info Apple clang $(APPLE_CLANG_MAJOR) detected, using system zlib)
        macos_gcc_configure_args += --with-system-zlib
        macos_gdb_configure_args += --with-system-zlib
        binutils_extra_configure_args += --with-system-zlib
      endif
    endif
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
do_auto_fixup_newlib := 1
ifdef auto_fixup_newlib
  ifeq (0,$(auto_fixup_newlib))
    $(warning 'Disabling Newlib Auto Fixup)
    do_auto_fixup_newlib := 0
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
  ifeq (0,$(do_auto_fixup_newlib))
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

ifdef newlib_iconv_encodings
  ifneq (0,$(newlib_iconv_encodings))
    newlib_extra_configure_args += --enable-newlib-iconv
    newlib_extra_configure_args += --enable-newlib-iconv-encodings=$(newlib_iconv_encodings)
  endif
endif

# Handle libstdc++ configuration options
ifdef libstdcxx_tzdb
  ifeq (0,$(libstdcxx_tzdb))
    gcc_extra_configure_args += --with-libstdcxx-zoneinfo=no
  else ifeq (1,$(libstdcxx_tzdb))
    gcc_extra_configure_args += --with-libstdcxx-zoneinfo=yes
  else
    gcc_extra_configure_args += --with-libstdcxx-zoneinfo=$(libstdcxx_tzdb),static
  endif
else
  gcc_extra_configure_args += --with-libstdcxx-zoneinfo=no
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
    gcc_pass1_configure_args += --disable-nls
    gcc_pass2_configure_args += --disable-nls
    binutils_extra_configure_args += --disable-nls
  endif
endif

# Handle language support options
pass1_languages := c
pass2_languages := c

ifdef enable_cpp
  ifneq (0,$(enable_cpp))
    pass2_languages := $(pass2_languages),c++
  endif
endif

ifdef enable_objc
  ifneq (0,$(enable_objc))
    pass2_languages := $(pass2_languages),objc
  endif
endif

ifdef enable_objcpp
  ifneq (0,$(enable_objcpp))
    pass2_languages := $(pass2_languages),obj-c++
  endif
endif

ifdef enable_d
  ifneq (0,$(enable_d))
    pass2_languages := $(pass2_languages),d
    gcc_pass2_configure_args += --disable-libphobos
  endif
endif

ifdef enable_fortran
  ifneq (0,$(enable_fortran))
    pass2_languages := $(pass2_languages),fortran
  endif
endif

ifdef enable_modula2
  ifneq (0,$(enable_modula2))
    pass2_languages := $(pass2_languages),m2
  endif
endif

ifdef enable_ada
  ifneq (0,$(enable_ada))
    pass2_languages := $(pass2_languages),ada
    gcc_pass2_configure_args += --disable-libada
  endif
endif

ifdef enable_go
  ifneq (0,$(enable_go))
    pass2_languages := $(pass2_languages),go
  endif
endif

ifdef enable_rust
  ifneq (0,$(enable_rust))
    pass2_languages := $(pass2_languages),rust
  endif
endif

ifdef enable_libgccjit
  ifneq (0,$(enable_libgccjit))
    pass1_languages := $(pass1_languages),jit
    gcc_pass1_configure_args += --enable-host-shared
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
