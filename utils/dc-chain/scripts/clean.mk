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
	-rm -rf build-newlib-$(target)-$(newlib_ver)
	-rm -rf build-gcc-$(target)-$(gcc_ver)-pass1
	-rm -rf build-gcc-$(target)-$(gcc_ver)-pass2
	-rm -rf build-binutils-$(target)-$(binutils_ver)
	-rm -rf build-$(gdb_name)

clean-downloads:
	-rm -rf $(binutils_name)
	-rm -rf $(gcc_name)
	-rm -rf $(newlib_name)
	-rm -rf $(gdb_name)

clean-archives:
	-rm -f $(config_guess)
	-rm -f $(config_sub)
	-rm -f $(binutils_file)
	-rm -f $(gcc_file)
	-rm -f $(newlib_file)
	-rm -f $(gmp_file)
	-rm -f $(mpfr_file)
	-rm -f $(mpc_file)
	-rm -f $(isl_file)
	-rm -f $(gdb_file)
