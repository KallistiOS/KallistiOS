# KallistiOS Toolchain Builder (kos-chain)

target=dvp-elf

# Binutils only: the VU has no C compiler or library.
build_final_target=build-binutils

# Toolchain versions for the PlayStation 2 DVP
binutils_ver=2.47

# The KOS Binutils patch is mandatory: stock config.sub rejects dvp-elf.
#
#   --enable-binutils    objdump decodes VU/VIF/DMA/GIF tags; also installs
#                        ar, nm, strip.
#   --disable-gprof(ng)  Profilers, irrelevant to an assembler.
#   --disable-ld         No dvp-elf ld emulation exists, and none is wanted: the
#                        EE linker places the objcopy'd blob.  Configure fails
#                        without this flag.
binutils_extra_configure_args = \
  --enable-binutils \
  --disable-gprof \
  --disable-gprofng \
  --disable-ld
