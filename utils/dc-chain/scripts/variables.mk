# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

# KOS Git root directory (contains kos/ and kos-ports/)
kos_root = $(CURDIR)/../../..

# kos_base is equivalent of KOS_BASE (contains include/ and kernel/)
kos_base = $(CURDIR)/../..

# Various directories
install       = $(prefix)/bin
pwd          := $(shell pwd)
patches      := $(pwd)/patches
logdir       := $(pwd)/logs

# Handling PATH environment variable
PATH         := $(sh_toolchain_path)/bin:$(arm_toolchain_path)/bin:$(PATH)
