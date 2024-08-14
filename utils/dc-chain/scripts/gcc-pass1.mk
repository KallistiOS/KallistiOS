# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

build-sh4-gcc-pass1: build = build-gcc-$(target)-$(gcc_ver)-pass1
build-sh4-gcc-pass1: enabled_languages = $(pass1_languages)
build-arm-gcc-pass1: build = build-gcc-$(target)-$(gcc_ver)
build-arm-gcc-pass1: enabled_languages = c
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
	      --with-gnu-as \
	      --with-gnu-ld \
	      --without-headers \
	      --with-newlib \
	      --enable-languages=$(enabled_languages) \
	      --disable-libssp \
	      --enable-checking=release \
	      $(cpu_configure_args) \
	      $(gcc_pass1_configure_args) \
	      $(macos_gcc_configure_args) \
	      MAKEINFO=missing \
	      CC="$(CC)" \
	      CXX="$(CXX)" \
	      $(static_flag) \
	      $(to_log)
	$(MAKE) $(jobs_arg) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) $(install_mode) DESTDIR=$(DESTDIR) $(to_log)
	$(clean_up)
