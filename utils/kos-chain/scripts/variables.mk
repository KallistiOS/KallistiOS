# KallistiOS Toolchain Builder (kos-chain)

# kos_base is equivalent of KOS_BASE (contains include/ and kernel/)
kos_base = $(CURDIR)/../..

# Various directories
install       = $(toolchain_path)/bin
pwd          := $(shell pwd)
patches      := $(pwd)/patches
logdir       := $(pwd)/logs

# Handling PATH environment variable
PATH         := $(toolchain_path)/bin:$(PATH)

arch         := $(word 1,$(subst -, ,$(target)))
gcc_arch     := $(subst powerpc,rs6000,$(arch))
