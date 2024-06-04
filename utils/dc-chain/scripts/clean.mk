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
	-rm -rf build-newlib-$(sh_target)-$(newlib_ver)
	-rm -rf build-newlib-$(arm_target)-$(newlib_ver)
	-rm -rf build-gcc-$(sh_target)-$(sh_gcc_ver)-pass1
	-rm -rf build-gcc-$(sh_target)-$(sh_gcc_ver)-pass2
	-rm -rf build-gcc-$(arm_target)-$(arm_gcc_ver)
	-rm -rf build-binutils-$(sh_target)-$(sh_binutils_ver)
	-rm -rf build-binutils-$(arm_target)-$(arm_binutils_ver)
	-rm -rf build-$(gdb_name)

clean-downloads: clean-gdb-sources clean-arm-sources clean-sh-sources

clean-gdb-sources:
	-rm -rf $(gdb_name)

clean-arm-sources:
	-rm -rf $(arm_binutils_name)
	-rm -rf $(arm_gcc_name)

clean-sh-sources:
	-rm -rf $(sh_binutils_name)
	-rm -rf $(sh_gcc_name)
	-rm -rf $(newlib_name)

clean-archives: clean-gdb-archives clean-arm-archives clean-sh-archives

clean-gdb-archives:
	-rm -f $(gdb_file)

clean-arm-archives:
	-rm -f $(config_guess)
	-rm -f $(config_sub)
	-rm -f $(arm_binutils_file)
	-rm -f $(arm_gcc_file)
	-rm -f $(arm_gmp_file)
	-rm -f $(arm_mpfr_file)
	-rm -f $(arm_mpc_file)
	-rm -f $(arm_isl_file)

clean-sh-archives:
	-rm -f $(config_guess)
	-rm -f $(config_sub)
	-rm -f $(sh_binutils_file)
	-rm -f $(sh_gcc_file)
	-rm -f $(newlib_file)
	-rm -f $(sh_gmp_file)
	-rm -f $(sh_mpfr_file)
	-rm -f $(sh_mpc_file)
	-rm -f $(sh_isl_file)
