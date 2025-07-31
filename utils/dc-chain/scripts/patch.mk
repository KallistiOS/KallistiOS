# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

# Ensure that, no matter where we enter, prefix and target are set correctly.
patch_targets = patch-binutils patch-gcc patch-newlib

patch: $(patch_targets)

# Patch
# Apply newlib fixups (default is yes and this should be always the case!)
ifeq (1,$(do_kos_patching))
# Add Build Pre-Requisites
  build-binutils: patch-binutils
  build-gcc-pass1 build-gcc-pass2: patch-gcc
  build-newlib-only: patch-newlib

# Add Patching Pre-Reqs for GDB
  build_gdb: patch_gdb
endif

# Require downloads before patching
patch-binutils: fetch-binutils
patch-gcc: fetch-gcc
patch-newlib: fetch-newlib

# Copy over required KOS files to GCC directory before patching
patch-gcc: gcc-fixup
gcc-fixup: fetch-gcc
	@echo "+++ Copying required KOS files into GCC directory..."
	cp $(patches)/gcc/gthr-kos.h $(src_dir)/libgcc/gthr-kos.h
	cp $(patches)/gcc/fake-kos.c $(src_dir)/libgcc/config/fake-kos.c

# Copy over required KOS files to newlib directory before patching
patch-newlib: newlib-fixup
newlib-fixup: fetch-newlib
	@echo "+++ Copying required KOS files into newlib directory..."
	mkdir -p $(src_dir)/newlib/libc/machine/$(arch)/sys
	cp $(kos_base)/include/sys/lock.h $(src_dir)/newlib/libc/machine/$(arch)/sys/lock.h

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
patch-binutils: patch_target_name = Binutils
patch-binutils: src_dir = binutils-$(binutils_ver)
patch-binutils: stamp_radical_name = $(src_dir)
patch-binutils: diff_patches := $(wildcard $(patches)/targets/$(src_dir)*.diff)
patch-binutils: diff_patches += $(wildcard $(patches)/targets/$(target)/$(src_dir)*.diff)
patch-binutils: diff_patches += $(wildcard $(patches)/hosts/$(host_triplet)/$(src_dir)*.diff)
patch-binutils:
	$(call patch_apply)
	$(call update_config_guess_sub)

# GNU Compiler Collection (GCC)
patch-gcc: patch_target_name = GCC
patch-gcc: src_dir = gcc-$(gcc_ver)
patch-gcc: stamp_radical_name = $(src_dir)
patch-gcc: diff_patches := $(wildcard $(patches)/targets/$(src_dir)*.diff)
patch-gcc: diff_patches += $(wildcard $(patches)/targets/$(target)/$(src_dir)*.diff)
patch-gcc: diff_patches += $(wildcard $(patches)/hosts/$(host_triplet)/$(src_dir)*.diff)
ifeq ($(uname_s), Darwin)
ifeq ($(uname_p), arm)
patch-gcc: diff_patches += $(wildcard $(patches)/hosts/arm-Darwin/$(src_dir)*.diff)
endif
endif
patch-gcc:
	$(call patch_apply)
	$(call update_config_guess_sub)

# Newlib
patch-newlib: patch_target_name = Newlib
patch-newlib: src_dir = newlib-$(newlib_ver)
patch-newlib: stamp_radical_name = $(src_dir)
patch-newlib: diff_patches := $(wildcard $(patches)/targets/$(src_dir)*.diff)
patch-newlib: diff_patches += $(wildcard $(patches)/targets/$(target)/$(src_dir)*.diff)
patch-newlib: diff_patches += $(wildcard $(patches)/hosts/$(host_triplet)/$(src_dir)*.diff)
patch-newlib:
	$(call patch_apply)
	$(call update_config_guess_sub)
