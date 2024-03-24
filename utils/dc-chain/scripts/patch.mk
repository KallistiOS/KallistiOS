# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

patch: patch-sh4 patch-arm patch-kos
patch-sh4: patch-sh4-binutils patch-sh4-gcc patch-sh4-newlib
patch-arm: patch-arm-binutils patch-arm-gcc

# Ensure that, no matter where we enter, prefix and target are set correctly.
patch_sh4_targets = patch-sh4-binutils patch-sh4-gcc patch-sh4-newlib
patch_arm_targets = patch-arm-binutils patch-arm-gcc

# Available targets for SH
$(patch_sh4_targets): target = $(sh_target)
$(patch_sh4_targets): gcc_ver = $(sh_gcc_ver)
$(patch_sh4_targets): binutils_ver = $(sh_binutils_ver)

# Available targets for ARM
$(patch_arm_targets): target = $(arm_target)
$(patch_arm_targets): gcc_ver = $(arm_gcc_ver)
$(patch_arm_targets): binutils_ver = $(arm_binutils_ver)

# To avoid code repetition, we use the same commands for both architectures.
# But we can't create a single target called 'patch-binutils' for both sh4 and
# arm, because phony targets can't be run multiple times. So we create multiple
# targets.
patch_binutils      = patch-sh4-binutils patch-arm-binutils
patch_gcc           = patch-sh4-gcc patch-arm-gcc
patch_newlib        = patch-sh4-newlib
patch_kos           = patch-kos

# Patch
# Apply sh4 newlib fixups (default is yes and this should be always the case!)
ifeq (1,$(do_kos_patching))
# Add Build Pre-Requisites for SH4 Steps
  build-sh4-binutils: patch-sh4-binutils
  build-sh4-gcc-pass1 build-sh4-gcc-pass2: patch-sh4-gcc
  build-sh4-newlib-only: patch-sh4-newlib

# Add Build Pre-Requisites for ARM Steps
  build-arm-binutils: patch-arm-binutils
  build-arm-gcc-pass1: patch-arm-gcc

# Add Patching Pre-Reqs for GDB
  build_gdb: patch_gdb
endif

# Require SH downloads before patching
patch-sh4-binutils: fetch-sh-binutils
patch-sh4-gcc: fetch-sh-gcc
patch-sh4-newlib: fetch-newlib

# Require ARM downloads before patching
patch-arm-binutils: fetch-arm-binutils
patch-arm-gcc: fetch-arm-gcc

# Copy over required KOS files to SH4 GCC directory before patching
patch-sh4-gcc: sh-gcc-fixup
sh-gcc-fixup: fetch-sh-gcc
	@echo "+++ Copying required KOS files into GCC directory..."
	cp $(kos_base)/kernel/arch/dreamcast/kernel/startup.s $(src_dir)/libgcc/config/sh/crt1.S
	cp $(patches)/gcc/gthr-kos.h $(src_dir)/libgcc/config/sh/gthr-kos.h
	cp $(patches)/gcc/fake-kos.S $(src_dir)/libgcc/config/sh/fake-kos.S

# Copy over required KOS files to newlib directory before patching
patch-sh4-newlib: sh-newlib-fixup
sh-newlib-fixup: fetch-newlib
	@echo "+++ Copying required KOS files into newlib directory..."
	cp $(kos_base)/include/sys/lock.h $(src_dir)/newlib/libc/sys/sh/sys

uname_p := $(shell uname -p)
uname_s := $(shell uname -s)

# This is a common 'patch_apply' function used in all the cases
define patch_apply
	@stamp_file=$(src_dir)/$(patch_target_name)_patch.stamp; \
	patches=$$(echo "$(diff_patches)" | xargs); \
	if ! test -f "$${stamp_file}"; then \
		if ! test -z "$${patches}"; then \
			echo "+++ Patching $(patch_target_name)..."; \
			echo "$${patches}" | xargs -n 1 patch -N -d $(src_dir) -p1 -i; \
		fi; \
		touch "$${stamp_file}"; \
	fi;
endef

# This function is used to replace the config.guess & config.sub that come 
# bundled with the sources with updated versions from GNU. This fixes issues
# when trying to compile older versions of the toolchain software on newer
# hardware.
define update_configs
	@echo "+++ Updating $(1) files in $(src_dir)"; \
	files=$$(find $(src_dir) -name $(1)); \
	echo "$${files}" | while I= read -r line; do \
		echo "    $${line}"; \
		cp $(1) $${line} > /dev/null; \
	done; \
	echo ""
endef

define update_config_guess_sub
	$(call update_configs,config.guess)
	$(call update_configs,config.sub)
endef

# Binutils
$(patch_binutils): patch_target_name = Binutils
$(patch_binutils): src_dir = binutils-$(binutils_ver)
$(patch_binutils): stamp_radical_name = $(src_dir)
$(patch_binutils): diff_patches := $(wildcard $(patches)/$(src_dir)*.diff)
$(patch_binutils): diff_patches += $(wildcard $(patches)/$(host_triplet)/$(src_dir)*.diff)
$(patch_binutils):
	$(call patch_apply)
	$(call update_config_guess_sub)

# GNU Compiler Collection (GCC)
$(patch_gcc): patch_target_name = GCC
$(patch_gcc): src_dir = gcc-$(gcc_ver)
$(patch_gcc): stamp_radical_name = $(src_dir)
$(patch_gcc): diff_patches := $(wildcard $(patches)/$(src_dir)*.diff)
$(patch_gcc): diff_patches += $(wildcard $(patches)/$(host_triplet)/$(src_dir)*.diff)
ifeq ($(uname_s), Darwin)
ifeq ($(uname_p), arm)
$(patch_gcc): diff_patches += $(wildcard $(patches)/$(uname_p)-$(uname_s)/$(src_dir)*.diff)
endif
endif
$(patch_gcc):
	$(call patch_apply)
	$(call update_config_guess_sub)

# Newlib
$(patch_newlib): patch_target_name = Newlib
$(patch_newlib): src_dir = newlib-$(newlib_ver)
$(patch_newlib): stamp_radical_name = $(src_dir)
$(patch_newlib): diff_patches := $(wildcard $(patches)/$(src_dir)*.diff)
$(patch_newlib): diff_patches += $(wildcard $(patches)/$(host_triplet)/$(src_dir)*.diff)
$(patch_newlib):
	$(call patch_apply)
	$(call update_config_guess_sub)

# KallistiOS
$(patch_kos): patch_target_name = KallistiOS
$(patch_kos): src_dir = $(kos_root)
$(patch_kos): stamp_radical_name = kos
$(patch_kos): diff_patches := $(wildcard $(patches)/$(src_dir)*.diff)
$(patch_kos): diff_patches += $(wildcard $(patches)/$(host_triplet)/$(src_dir)*.diff)
$(patch_kos):
	$(call patch_apply)
