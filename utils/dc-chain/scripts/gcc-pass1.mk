# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

build-sh4-gcc-pass1: build = build-gcc-$(target)-$(gcc_ver)-pass1
build-arm-gcc-pass1: build = build-gcc-$(target)-$(gcc_ver)
$(build_gcc_pass1) $(build_gcc_pass2): src_dir = gcc-$(gcc_ver)
$(build_gcc_pass1) $(build_gcc_pass2): log = $(logdir)/$(build).log
$(build_gcc_pass1): logdir
	@echo "+++ Building $(src_dir) to $(build) (pass 1)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build); \
	    ../$(src_dir)/configure \
	      --target=$(target) \
	      --prefix=$(prefix) \
	      --without-headers \
	      --with-newlib \
	      --enable-languages=c \
	      --disable-libssp \
	      --enable-checking=release \
	      $(extra_configure_args) \
	      $(macos_gcc_configure_args) \
	      CC="$(CC)" \
	      CXX="$(CXX)" \
	      $(static_flag) \
	      $(to_log)
	$(MAKE) $(makejobs) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) $(install_mode) DESTDIR=$(DESTDIR) $(to_log)
	$(clean_up)
