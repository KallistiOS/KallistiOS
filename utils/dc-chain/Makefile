# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

#
# Please edit 'Makefile.cfg' to customize your 'dc-chain' setup.
#

# Display startup banner
include scripts/banner.mk

# User configuration
config_file=Makefile.cfg
ifneq ("$(wildcard $(config_file))","")
  $(info +++ Using configuration from $(config_file))
  include $(config_file)
else
  ifneq ("$(platform)","")
    $(info +++ Using configuration from Makefile.$(platform).cfg)
    include Makefile.$(platform).cfg
  else
    $(error No configuration file found at $(config_file))
  endif
endif

# Toolchain profiles
include scripts/profile-defaults.mk
ifneq ("$(wildcard profiles/$(platform)/$(toolchain_profile).mk)","")
  include profiles/$(platform)/$(toolchain_profile).mk
else
  $(error No toolchain profile file found at profiles/$(platform)/$(toolchain_profile).mk)
endif

# Download functions
include scripts/utils.mk

# Detect host machine
include scripts/host-detect.mk

# Initialization rules
include scripts/init.mk

# Makefile variables
include scripts/variables.mk

default: build gdb

all: build build-arm gdb

# ---- download {{{

include scripts/download.mk

# ---- }}}

# ---- patch {{{

include scripts/patch.mk

# ---- }}}

# ---- build {{{

include scripts/build.mk

include scripts/binutils.mk
include scripts/gcc-pass1.mk
include scripts/newlib.mk
include scripts/gcc-pass2.mk

# ---- }}}}

# ---- optional components {{{

include scripts/gdb.mk

# ---- }}}}

# ---- support {{{

include scripts/clean.mk
include scripts/logdir.mk
include scripts/options.mk

# ---- }}}

# ---- phony targets {{{

include scripts/phony.mk

# ---- }}}}

# vim:tw=0:fdm=marker:fdc=2:fdl=1
