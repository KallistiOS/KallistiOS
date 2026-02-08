# KallistiOS Toolchain Builder (kos-chain)

# Set default mirror if not specified
gnu_mirror ?= ftpmirror.gnu.org
binutils_base_url = $(download_protocol)://$(gnu_mirror)/gnu/binutils
gcc_base_url = $(download_protocol)://$(gnu_mirror)/gnu/gcc/gcc-$(1)
newlib_base_url = $(download_protocol)://sourceware.org/pub/newlib

# Can't use same mirror as above since the versions of these
# required by GCC4.7 are older than available on the newer mirror
gcc_deps_url = gcc.gnu.org/pub/gcc/infrastructure
gmp_base_url = $(download_protocol)://$(gcc_deps_url)
mpfr_base_url = $(download_protocol)://$(gcc_deps_url)
mpc_base_url = $(download_protocol)://$(gcc_deps_url)
isl_base_url = $(download_protocol)://$(gcc_deps_url)

gdb_base_url  = $(download_protocol)://$(gnu_mirror)/gnu/gdb

# used to add additional blank lines so eval works properly
define blank_line


endef

STAMP_TYPES = download patch build install

# Function to Generate Variables from Config Options
# Args:
# 1 - Package (binutils,gcc,newlib,gdb,gmp,mpfr,mpc,isl)
# 2 - Dest Folder after Extraction (Optional) (used to move gcc deps inside gcc folder)
define gen_download_vars

$(1)_name = $(1)-$$($(1)_ver)
$(1)_file = $$($(1)_name).tar.$$($(1)_download_type)
$(1)_url  = $(call $(1)_base_url,$($(1)_ver))/$$($(1)_file)
$(1)_dest = $(2)

# Generate Stamp Variables
$(foreach type,$(STAMP_TYPES),$(blank_line)stamp_$(1)_$(type) = $(if $(2),$(2),$$($(1)_name))/$(1)_$(type).stamp)

endef

GCC_DEPS = gmp mpfr mpc isl

# Generate download variables for main packages
$(eval $(call gen_download_vars,binutils))
$(eval $(call gen_download_vars,gcc))
$(eval $(call gen_download_vars,newlib))
$(eval $(call gen_download_vars,gdb))

# Generate download variables for GCC dependencies
$(foreach dep,$(GCC_DEPS),$(eval $(call gen_download_vars,$(dep),$(gcc_name)/$(dep))))

# GCC dependencies can only be extracted after GCC itself is extracted
$(foreach dep,$(GCC_DEPS),$(eval $(stamp_$(dep)_download): $(stamp_gcc_download)))

# Setup Targets for Downloading & Unpacking Archives
# Args:
# 1 - Package Name (binutils, newlib, gcc, gdb, etc.)
define setup_archive_targets
# set file & url for target archive
$(stamp_$(1)_download): name = $($(1)_name)
$(stamp_$(1)_download): file = $($(1)_file)
$(stamp_$(1)_download): url = $($(1)_url)
$(stamp_$(1)_download): dest = $($(1)_dest)

# add as dependency for download stamp
$(stamp_$(1)_download): $($(1)_file)
endef

# Setup Targets for Downloading Git Repos
# Args:
# 1 - Package Name (binutils, newlib, gcc, gdb, etc.)
define setup_git_targets
$(stamp_$(1)_download): name = $($(1)_name)
$(stamp_$(1)_download): git_repo = $($(1)_git_repo)
$(stamp_$(1)_download): git_branch = $($(1)_git_branch)
$(stamp_$(1)_download): additional_git_args = $(if $($(1)_git_branch),-b $($(1)_git_branch),)
$(stamp_$(1)_download): dest = $($(1)_dest)
endef

SOURCE_DOWNLOADS += binutils gcc gdb newlib gmp mpfr mpc isl

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

gcc_prereqs_script = $(if $(filter 1,$(use_custom_dependencies)),,cd ./$(name) && ./contrib/download_prerequisites)
$(ARCHIVE_EXTRACTS):
	@echo "+++ Extracting $(file)..."
	rm -rf $(name)
	tar xf $(file)
#   Move folder if dest was specified
	$(if $(dest),mv $(name) $(dest))
#   Run download_prerequisites if GCC
	$(if $(filter gcc-%,$(name)),$(gcc_prereqs_script))
	touch $@

$(foreach repo, $(FROM_GIT_REPOS),$(eval $(call setup_git_targets,$(repo))))
GIT_TARGETS = $(foreach target,$(FROM_GIT_REPOS), $(stamp_$(target)_download))

$(GIT_TARGETS):
	@echo "+++ Cloning $(git_repo)..."
	rm -rf $(name)
	git clone --depth=1 --filter=tree:0 $(git_repo) $(additional_git_args) $(if $(dest),$(dest),$(name))
	touch $@


fetch_gcc_deps_source = $(stamp_gmp_download) $(stamp_mpfr_download) $(stamp_mpc_download)

# Some older versions of GCC (including 4.7) don't require ISL so we skip adding a dependency
# if a version number is not provided
ifdef isl_ver
  fetch_gcc_deps_source += $(stamp_isl_download)
endif

fetch-binutils: $(stamp_binutils_download)
fetch-gcc: $(stamp_gcc_download) $(if $(filter 1,$(use_custom_dependencies)),$(fetch_gcc_deps_source))
fetch-newlib: $(stamp_newlib_download)
fetch-gdb: $(stamp_gdb_download)

fetch: fetch-binutils fetch-gcc fetch-newlib fetch-gdb
