# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

clean: clean-downloads clean-builds clean_patches_stamp

distclean: clean-archives clean-downloads clean-builds clean_patches_stamp

clean_patches_stamp:
	-@tmpdir=.tmp; \
	if ! test -d "$${tmpdir}"; then \
		mkdir "$${tmpdir}"; \
	fi; \
	mv $(stamp_gdb_download) $${tmpdir} 2>/dev/null; \
	mv $(stamp_gdb_patch) $${tmpdir} 2>/dev/null; \
	rm -f *.stamp; \
	mv $${tmpdir}/*.stamp . 2>/dev/null; \
	rm -rf $${tmpdir}

clean-builds: clean_patches_stamp
	-rm -rf build-newlib-$(target)-$(newlib_ver) \
		build-gcc-$(target)-$(gcc_ver)-pass1 \
		build-gcc-$(target)-$(gcc_ver)-pass2 \
		build-binutils-$(target)-$(binutils_ver) \
		build-$(gdb_name)

clean-downloads:
	-rm -rf $(binutils_name) $(gcc_name) $(newlib_name) $(gdb_name)

clean-archives:
	-rm -f $(config_guess) \
		$(config_sub) \
		$(binutils_file) \
		$(gcc_file) \
		$(newlib_file) \
		$(gmp_file) \
		$(mpfr_file) \
		$(mpc_file) \
		$(isl_file) \
		$(gdb_file)
