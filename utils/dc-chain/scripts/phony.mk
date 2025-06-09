# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

.PHONY: all
.PHONY: fetch fetch-gdb
.PHONY: patch $(patch_targets)
.PHONY: $(patch_binutils) $(patch_gcc) $(patch_newlib)
.PHONY: build $(build_targets)
.PHONY: build-binutils build-newlib build-gcc-pass1 build-gcc-pass2 fixup-newlib
.PHONY: gdb install_gdb build_gdb
.PHONY: clean clean-builds clean-downloads
.PHONY: distclean clean-builds clean-downloads clean-archives
