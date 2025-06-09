# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

build: build-done
build-gcc: build-gcc-pass2
build-newlib: build-newlib-only fixup-newlib

fixup_newlib_stamp = fixup-newlib.stamp
build-done: build-gcc
	@if test -f "$(fixup_newlib_stamp)"; then \
		echo ""; \
		echo ""; \
		echo "                              *** W A R N I N G ***"; \
		echo ""; \
		echo "    Be careful when upgrading KallistiOS or your toolchain!"; \
		echo "    You need to fixup-newlib again as the 'ln' utility is not working"; \
		echo "    properly on MinGW/MSYS and MinGW-w64/MSYS2 environments!"; \
		echo ""; \
		echo "    See ./doc/mingw/ for details."; \
		echo ""; \
	fi;

# Ensure that, no matter where we enter, prefix and target are set correctly.
build_targets = build-binutils build-gcc build-gcc-pass1 \
                    build-newlib build-newlib-only build-gcc-pass2

# Build Dependencies
build-gcc-pass1: build-binutils
build-newlib-only: build-gcc-pass1
build-gcc-pass2: fixup-newlib

# Download Dependencies
build-binutils: fetch-binutils
build-gcc-pass1 build-gcc-pass2: fetch-gcc
build-newlib-only: fetch-newlib

# GDB Patch Dependency
build_gdb: patch_gdb

# MinGW/MSYS or 'sh_force_libbfd_installation=1': install BFD if required.
# To compile dc-tool, we need to install libbfd for sh-elf.
# This is done when making build-binutils.
ifdef sh_force_libbfd_installation
  ifneq (0,$(sh_force_libbfd_installation))
    do_sh_force_libbfd_installation := 1
  endif
endif
ifneq ($(or $(MINGW),$(do_sh_force_libbfd_installation)),)
  $(build_targets): libbfd_install_flag = -enable-install-libbfd -enable-install-libiberty
  $(build_targets): libbfd_src_bin_dir = $(toolchain_path)/$(host_triplet)/$(target)
endif

build_binutils      = build-binutils
build_gcc_pass1     = build-gcc-pass1
build_newlib        = build-newlib-only
build_gcc_pass2     = build-gcc-pass2
