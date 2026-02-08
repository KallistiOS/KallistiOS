# KallistiOS Toolchain Builder (kos-chain)

.PHONY: all
.PHONY: fetch
.PHONY: patch $(patch_targets)
.PHONY: build $(build_targets)
.PHONY: build-binutils build-newlib build-gcc-pass1 build-gcc-pass2 fixup-newlib
.PHONY: gdb install-gdb build-gdb patch-gdb fetch-gdb
.PHONY: clean clean-builds clean-downloads
.PHONY: distclean clean-builds clean-downloads clean-archives
