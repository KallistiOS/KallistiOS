# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

.PHONY: all
.PHONY: fetch fetch-sh4 fetch-arm fetch-gdb
.PHONY: patch patch-sh4 patch-arm $(patch_sh4_targets) $(patch_arm_targets)
.PHONY: $(patch_binutils) $(patch_gcc) $(patch_newlib)
.PHONY: build build-sh4 build-arm $(build_sh4_targets) $(build_arm_targets)
.PHONY: build-binutils build-newlib build-gcc-pass1 build-gcc-pass2 fixup-sh4-newlib
.PHONY: gdb install_gdb build_gdb
.PHONY: clean clean-builds clean-downloads
.PHONY: distclean clean-builds clean-downloads clean-archives
