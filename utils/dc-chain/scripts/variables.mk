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

# Installation path for SH
sh_prefix    := $(toolchains_base)/$(sh_target)

# Installation path for ARM
arm_prefix   := $(toolchains_base)/$(arm_target)

# Various directories 
install       = $(prefix)/bin
pwd          := $(shell pwd)
patches      := $(pwd)/patches
logdir       := $(pwd)/logs

# Handling PATH environment variable
PATH         := $(sh_prefix)/bin:$(arm_prefix)/bin:$(PATH)
