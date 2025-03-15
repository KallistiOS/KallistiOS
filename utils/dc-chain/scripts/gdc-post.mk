# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

ifdef enable_d
  ifneq (0,$(enable_d))
	$(gdc_post_install): prefix = $(sh_toolchain_path)
	$(gdc_post_install): gdc_target = $(target)
	$(gdc_post_install): gdc_ver = $(gcc_ver)
	$(gdc_post_install): src_dir = gcc-$(gcc_ver)
	$(gdc_post_install): out_dir = $(prefix)
	$(gdc_post_install): logdir
		@echo "+++ Running post-install fixups for GNU D..."
		-mkdir -p $(prefix)/lib/gcc/$(gdc_target)/$(gdc_ver)/include/d
		cp -r ./$(src_dir)/libphobos/libdruntime/* $(prefix)/lib/gcc/$(gdc_target)/$(gdc_ver)/include/d
		cp $(patches)/gdc/config.d $(prefix)/lib/gcc/$(gdc_target)/$(gdc_ver)/include/d/gcc/config.d
  endif
endif