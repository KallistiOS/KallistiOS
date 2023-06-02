# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Modified by Colton Pawielski (2023)
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

# Set default mirror if not specified
gnu_mirror ?= ftpmirror.gnu.org

gdb_base_url  = $(download_protocol)://$(gnu_mirror)/gnu/gdb

# used to add additional blank lines so eval works properly
define blank_line


endef

STAMP_TYPES = download patch build install

# Function to Generate Variables from Config Options
# Args:
# 1 - Package (binutils,gcc,newlib,gdb,gmp,mpfr,mpc,isl)
# 2 - Arch Prefixed with underscore (Optional) (sh_,arm_,)
define gen_download_vars

$(2)$(1)_name = $(1)-$($(2)$(1)_ver)
$(2)$(1)_file = $$($(2)$(1)_name).tar.$$($(2)$(1)_download_type)
$(2)$(1)_url  = $(call $(1)_base_url,$($(2)$(1)_ver))/$$($(2)$(1)_file)

# Generate Stamp Variables
$(foreach type,$(STAMP_TYPES),$(blank_line)stamp_$(2)$(1)_$(type) = $(if $(3),$(3),$$($(2)$(1)_name))/$(1)_$(type).stamp)

endef

$(eval $(call gen_download_vars,gdb))

# Setup Targets for Downloading & Unpacking Archives
# Args:
# 1 - Package Name (gdb, newlib, sh_gcc, arm_binutils, etc)
define setup_archive_targets
# set file & url for target archive
$(stamp_$(1)_download): name = $($(1)_name)
$(stamp_$(1)_download): file = $($(1)_file)
$(stamp_$(1)_download): url = $($(1)_url)

# add as dependency for download stamp
$(stamp_$(1)_download): $($(1)_file)
endef

# Setup Targets for Downloading Git Repos
# Args:
# 1 - Package Name (gdb, newlib, sh_gcc, arm_binutils, etc)
define setup_git_targets
$(stamp_$(1)_download): name = $($(1)_name)
$(stamp_$(1)_download): git_repo = $($(1)_git_repo)
$(stamp_$(1)_download): git_branch = $($(1)_git_branch)
$(stamp_$(1)_download): additional_git_args = $(if $($(1)_git_branch),-b $($(1)_git_branch),)
endef

SOURCE_DOWNLOADS := gdb

ARCHIVES_TYPES = xz gz bz2
# Get items from SOURCE_DOWNLOADS that are archive downloads
FROM_ARCHIVES = $(foreach src, $(SOURCE_DOWNLOADS), $(if $(filter $(ARCHIVES_TYPES),$($(src)_download_type)),$(src)))
# Get items from SOURCE_DOWNLOADS that are git repos
FROM_GIT_REPOS = $(foreach src, $(SOURCE_DOWNLOADS), $(if $(filter git,$($(src)_download_type)),$(src)))

$(foreach tar, $(FROM_ARCHIVES),$(eval $(call setup_archive_targets,$(tar))))
ARCHIVE_TARGETS = $(sort $(foreach tar, $(FROM_ARCHIVES),$($(tar)_file)))
ARCHIVE_EXTRACTS = $(sort $(foreach target,$(FROM_ARCHIVES),$(stamp_$(target)_download)))

$(ARCHIVE_TARGETS):
	@echo "+++ Downloading $(file)..."
	$(call web_download,$(url))

$(ARCHIVE_EXTRACTS):
	@echo "+++ Extracting $(file)..."
	rm -rf $(name)
	tar xf $(file)
	touch $@

$(foreach repo, $(FROM_GIT_REPOS),$(eval $(call setup_git_targets,$(repo))))
GIT_TARGETS = $(foreach target,$(FROM_GIT_REPOS), $(stamp_$(target)_download))

$(GIT_TARGETS):
	@echo "+++ Cloning $(git_repo)..."
	rm -rf $(name)
	git clone $(git_repo) $(additional_git_args) $(if $(dest),$(dest),$(name))
	touch $@

fetch-gdb: $(stamp_gdb_download)

fetch-all: fetch-gdb
