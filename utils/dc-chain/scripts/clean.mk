# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

clean_patches_stamp:
	-@tmpdir=.tmp; \
	if ! test -d "$${tmpdir}"; then \
		mkdir "$${tmpdir}"; \
	fi; \
	mv patch-*.stamp $${tmpdir} 2>/dev/null; \
	mv $(stamp_gdb_unpack) $${tmpdir} 2>/dev/null; \
	mv $(stamp_gdb_patch) $${tmpdir} 2>/dev/null; \
	mv $(stamp_insight_unpack) $${tmpdir} 2>/dev/null; \
	rm -f *.stamp; \
	mv $${tmpdir}/*.stamp . 2>/dev/null; \
	rm -rf $${tmpdir}

clean: clean_patches_stamp
	-rm -rf build-newlib-$(sh_target)-$(newlib_ver)
	-rm -rf build-newlib-$(arm_target)-$(newlib_ver)
	-rm -rf build-gcc-$(sh_target)-$(sh_gcc_ver)-pass1
	-rm -rf build-gcc-$(sh_target)-$(sh_gcc_ver)-pass2
	-rm -rf build-gcc-$(arm_target)-$(arm_gcc_ver)
	-rm -rf build-binutils-$(sh_target)-$(sh_binutils_ver)
	-rm -rf build-binutils-$(arm_target)-$(arm_binutils_ver)
	-rm -rf build-$(gdb_name)
	-rm -rf build-$(insight_name)
