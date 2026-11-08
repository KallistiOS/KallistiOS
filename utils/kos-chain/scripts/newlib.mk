# KallistiOS Toolchain Builder (kos-chain)

$(build_newlib): build = build-newlib-$(target)-$(newlib_ver)
$(build_newlib): src_dir = newlib-$(newlib_ver)
$(build_newlib): log = $(logdir)/$(build).log
$(build_newlib): logdir
	@echo "+++ Building $(src_dir) to $(build)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build); \
	  ../$(src_dir)/configure \
	    --disable-newlib-supplied-syscalls \
	    --target=$(target) \
	    --prefix=$(toolchain_path) \
	    $(cpu_configure_args) \
	    $(newlib_extra_configure_args) \
	    CC_FOR_TARGET="$(CC_FOR_TARGET)" \
	    $(to_log)
	$(MAKE) $(jobs_arg) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) install DESTDIR=$(DESTDIR) $(to_log)
	$(clean_up)

fixup-newlib: newlib_inc = $(toolchain_path)/$(target)/include
fixup-newlib: fixup-newlib-init

# Apply newlib fixups (default is yes and this should be always the case!)
ifeq (1,$(do_auto_fixup_newlib))
  fixup-newlib: fixup-newlib-apply
endif

# Prepare the fixup (always applied)
fixup-newlib-init: $(build_newlib)
	-mkdir -p $(newlib_inc)
	-mkdir -p $(newlib_inc)/sys
	-mkdir -p $(newlib_inc)/machine

fixup-newlib-apply: fixup-newlib-init

# Apply the fixups only, without (re)building newlib. This is useful after
# updating KallistiOS on MinGW/MSYS or MinGW-w64/MSYS2.
fixup-newlib-only: newlib_inc = $(toolchain_path)/$(target)/include

# The list of fixed up headers is maintained in scripts/fixup-newlib.sh.
# Under MinGW/MSYS or MinGW-w64/MSYS2, the ln tool is not efficient, so the
# script does a simple copy. Please keep that in mind when upgrading
# KallistiOS or your toolchain!
fixup-newlib-apply fixup-newlib-only:
	@echo "+++ Fixing up newlib includes..."
	$(SHELL) $(fixup_newlib_script) $(fixup_newlib_mode) $(kos_base) $(newlib_inc)
ifdef MINGW32
	touch $(fixup_newlib_stamp)
endif
