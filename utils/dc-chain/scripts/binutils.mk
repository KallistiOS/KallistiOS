# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

# Here we use the essentially same code for multiple targets,
# differing only by the current state of the variables below.
$(build_binutils): build = build-binutils-$(target)-$(binutils_ver)
$(build_binutils): src_dir = binutils-$(binutils_ver)
$(build_binutils): log = $(logdir)/$(build).log
$(build_binutils): logdir
	@echo "+++ Building $(src_dir) to $(build)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build); \
      ../$(src_dir)/configure \
        --target=$(target) \
        --prefix=$(prefix) \
        --disable-werror \
        $(libbfd_install_flag) \
        CC="$(CC)" \
        CXX="$(CXX)" \
        $(static_flag) \
        $(to_log)
	$(MAKE) $(makejobs) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) $(install_mode) DESTDIR=$(DESTDIR) $(to_log)
# MinGW/MSYS or 'sh_force_libbfd_installation=1'
# This part is about BFD for sh-elf target.
# It will move sh-elf libbfd into a nicer place, as our cross-compiler is made 
# by us for our usage... so no problems about system libbfd conflicts!
# See: https://www.sourceware.org/ml/binutils/2004-03/msg00337.html
# Also, install zlib at the same time as we need it to use libbfd.
# Note: BFD for sh-elf is used for compiling dc-tool. Others platforms uses libelf.
	@if test "$(target)" = "$(sh_target)" && ! test -z "$(libbfd_src_bin_dir)"; then \
		echo "+++ Installing Binary File Descriptor library (libbfd) for $(target)..."; \
		$(MAKE) -C $(build)/zlib install DESTDIR=$(DESTDIR) $(to_log); \
		if ! test -d "$(sh_prefix)/include/"; then \
			mkdir $(sh_prefix)/include/; \
		fi; \
		mv $(libbfd_src_bin_dir)/include/* $(sh_prefix)/include/; \
		if ! test -d "$(sh_prefix)/lib/"; then \
			mkdir $(sh_prefix)/lib/; \
		fi; \
		mv $(libbfd_src_bin_dir)/lib/* $(sh_prefix)/lib/; \
		rmdir $(libbfd_src_bin_dir)/include/; \
		rmdir $(libbfd_src_bin_dir)/lib/; \
		rmdir $(libbfd_src_bin_dir)/; \
		rmdir $(sh_prefix)/$(host_triplet)/; \
	fi;
	$(clean_up)	
