# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

build: build-sh4-done build-arm
build-sh4: build-sh4-gcc
build-arm: build-arm-gcc
build-sh4-gcc: build-sh4-gcc-pass2
build-arm-gcc: build-arm-gcc-pass1
build-sh4-newlib: build-sh4-newlib-only fixup-sh4-newlib

fixup_sh4_newlib_stamp = fixup-sh4-newlib.stamp
build-sh4-done: build-sh4 
	@if test -f "$(fixup_sh4_newlib_stamp)"; then \
		echo ""; \
		echo ""; \
		echo "                              *** W A R N I N G ***"; \
		echo ""; \
		echo "    Be careful when upgrading KallistiOS or your toolchain!"; \
		echo "    You need to fixup-sh4-newlib again as the 'ln' utility is not working"; \
		echo "    properly on MinGW/MSYS and MinGW-w64/MSYS2 environments!"; \
		echo ""; \
		echo "    See ./doc/mingw/ for details."; \
		echo ""; \
	fi;

# Ensure that, no matter where we enter, prefix and target are set correctly.
build_sh4_targets = build-sh4-binutils build-sh4-gcc build-sh4-gcc-pass1 \
                    build-sh4-newlib build-sh4-newlib-only build-sh4-gcc-pass2
build_arm_targets = build-arm-binutils build-arm-gcc build-arm-gcc-pass1

# Available targets for SH
$(build_sh4_targets): prefix = $(sh_prefix)
$(build_sh4_targets): target = $(sh_target)
$(build_sh4_targets): extra_configure_args = --with-multilib-list=m4-single-only --with-endian=little --with-cpu=m4-single-only
$(build_sh4_targets): gcc_ver = $(sh_gcc_ver)
$(build_sh4_targets): binutils_ver = $(sh_binutils_ver)

# SH4 Build Dependencies
build-sh4-gcc-pass1: build-sh4-binutils
build-sh4-newlib-only: build-sh4-gcc-pass1
build-sh4-gcc-pass2: fixup-sh4-newlib

# ARM Build Dependencies
build-arm-gcc-pass1: build-arm-binutils

# SH4 Download Dependencies
build-sh4-binutils: fetch-sh-binutils
build-sh4-gcc-pass1 build-sh4-gcc-pass2: fetch-sh-gcc
build-sh4-newlib-only: fetch-newlib

# ARM Download Dependencies
build-arm-binutils: fetch-arm-binutils
build-arm-gcc-pass1: fetch-arm-gcc

# GDB Patch Dependency
build_gdb: patch_gdb

# MinGW/MSYS or 'sh_force_libbfd_installation=1': install BFD if required.
# To compile dc-tool, we need to install libbfd for sh-elf.
# This is done when making build-sh4-binutils.
ifdef sh_force_libbfd_installation
  ifneq (0,$(sh_force_libbfd_installation))
    do_sh_force_libbfd_installation := 1
  endif
endif
ifneq ($(or $(MINGW),$(do_sh_force_libbfd_installation)),)
  $(build_sh4_targets): libbfd_install_flag = -enable-install-libbfd -enable-install-libiberty
  $(build_sh4_targets): libbfd_src_bin_dir = $(sh_prefix)/$(host_triplet)/$(sh_target)
endif

# Available targets for ARM
$(build_arm_targets): prefix = $(arm_prefix)
$(build_arm_targets): target = $(arm_target)
$(build_arm_targets): extra_configure_args = --with-arch=armv4 --with-mode=arm --disable-multilib
$(build_arm_targets): gcc_ver = $(arm_gcc_ver)
$(build_arm_targets): binutils_ver = $(arm_binutils_ver)

# To avoid code repetition, we use the same commands for both architectures.
# But we can't create a single target called 'build-binutils' for both sh4 and
# arm, because phony targets can't be run multiple times. So we create multiple
# targets.
build_binutils      = build-sh4-binutils  build-arm-binutils
build_gcc_pass1     = build-sh4-gcc-pass1 build-arm-gcc-pass1
build_newlib        = build-sh4-newlib-only
build_gcc_pass2     = build-sh4-gcc-pass2
