# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

disable_libada=""

ifneq (,$(findstring ada,$(pass2_languages)))
	disable_libada=--disable-libada
endif

$(build_gcc_pass2): build = build-gcc-$(target)-$(gcc_ver)-pass2
$(build_gcc_pass2): logdir
	@echo "+++ Building $(src_dir) to $(build) (pass 2)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build); \
        ../$(src_dir)/configure \
          --target=$(target) \
          --prefix=$(prefix) \
          --with-gnu-as \
          --with-gnu-ld \
          --with-newlib \
          --disable-libssp \
          --disable-libphobos \
          $(disable_libada) \
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
        ifneq (,$(findstring ada,$(pass2_languages)))
					$(MAKE) $(jobs_arg) -C $(build)/gcc cross-gnattools ada.all.cross DESTDIR=$(DESTDIR) $(to_log)
        endif
				$(MAKE) -C $(build) $(install_mode) DESTDIR=$(DESTDIR) $(to_log)
				$(clean_up)
