# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

$(build_gcc_pass2): build = build-gcc-$(target)-$(gcc_ver)-pass2
$(build_gcc_pass2): logdir
	@echo "+++ Building $(src_dir) to $(build) (pass 2)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build); \
        ../$(src_dir)/configure \
          --target=$(target) \
          --prefix=$(toolchain_path) \
          --with-gnu-as \
          --with-gnu-ld \
          --with-newlib \
          --disable-libssp \
          --enable-threads=$(thread_model) \
          --enable-languages=$(pass2_languages) \
          --enable-checking=release \
          $(cpu_configure_args) \
          $(gcc_pass2_configure_args) \
          $(macos_gcc_configure_args) \
          MAKEINFO=missing \
          CC="$(CC)" \
          CXX="$(CXX)" \
          $(static_flag) \
          $(to_log)
	$(MAKE) $(jobs_arg) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
ifdef enable_ada
  ifneq (0,$(enable_ada))
	$(MAKE) $(jobs_arg) -C $(build)/gcc cross-gnattools ada.all.cross DESTDIR=$(DESTDIR) $(to_log)
  endif
endif
ifdef enable_d
  ifneq (0,$(enable_d))
	@echo "+++ Running post-install fixups for GNU D..."
	-mkdir -p $(toolchain_path)/lib/gcc/$(target)/$(gcc_ver)/include/d
	cp -r ./gcc-$(gcc_ver)/libphobos/libdruntime/* $(sh_toolchain_path)/lib/gcc/$(target)/$(gcc_ver)/include/d
	cp $(patches)/gdc/config.d $(toolchain_path)/lib/gcc/$(target)/$(gcc_ver)/include/d/gcc/config.d
  endif
endif
	$(MAKE) -C $(build) $(install_mode) DESTDIR=$(DESTDIR) $(to_log)
	$(target)-gcc-ar d $(shell $(target)-gcc -print-file-name=libgcc.a) fake-kos.o
	$(clean_up)
