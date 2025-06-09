# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

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
        --prefix=$(toolchain_path) \
        --disable-werror \
        $(libbfd_install_flag) \
        $(binutils_extra_configure_args) \
        CC="$(CC)" \
        CXX="$(CXX)" \
        $(static_flag) \
        $(to_log)
	$(MAKE) $(jobs_arg) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) $(install_mode) DESTDIR=$(DESTDIR) $(to_log)
# MinGW/MSYS or 'sh_force_libbfd_installation=1'
# This part is about BFD for sh-elf target.
# It will move sh-elf libbfd into a nicer place, as our cross-compiler is made
# by us for our usage... so no problems about system libbfd conflicts!
# See: https://www.sourceware.org/ml/binutils/2004-03/msg00337.html
# Also, install zlib at the same time as we need it to use libbfd.
# Note: BFD for sh-elf is used for compiling dc-tool. Others platforms uses libelf.
	@if test "$(target)" = "sh-elf" && ! test -z "$(libbfd_src_bin_dir)"; then \
		echo "+++ Installing Binary File Descriptor library (libbfd) for $(target)..."; \
		$(MAKE) -C $(build)/zlib install DESTDIR=$(DESTDIR) $(to_log); \
		if ! test -d "$(toolchain_path)/include/"; then \
			mkdir $(toolchain_path)/include/; \
		fi; \
		mv $(libbfd_src_bin_dir)/include/* $(toolchain_path)/include/; \
		if ! test -d "$(toolchain_path)/lib/"; then \
			mkdir $(toolchain_path)/lib/; \
		fi; \
		mv $(libbfd_src_bin_dir)/lib/* $(toolchain_path)/lib/; \
		rmdir $(libbfd_src_bin_dir)/include/; \
		rmdir $(libbfd_src_bin_dir)/lib/; \
		rmdir $(libbfd_src_bin_dir)/; \
		rmdir $(toolchain_path)/$(host_triplet)/; \
	fi;
	$(clean_up)
