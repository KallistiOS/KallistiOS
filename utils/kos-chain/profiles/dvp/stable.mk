# KallistiOS Toolchain Builder (kos-chain)

target=dvp-elf

# DVP is binutils-only (assembler and linker for VU microcode)
build_final_target=build-binutils

binutils_extra_configure_args += --disable-nls --disable-build-warnings

# Toolchain versions for PS2 DVP
binutils_ver=2.46.1
