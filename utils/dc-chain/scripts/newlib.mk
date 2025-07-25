# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

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

fixup-newlib-apply: fixup-newlib-init
	@echo "+++ Fixing up newlib includes..."
# KOS pthread.h is modified
# to define _POSIX_THREADS
# pthreads to kthreads mapping
# so KOS includes are available as kos/file.h
# kos/thread.h requires arch/irq.h
	cp $(kos_base)/include/pthread.h $(newlib_inc)
	cp $(kos_base)/include/sys/_pthreadtypes.h $(newlib_inc)/sys
	cp $(kos_base)/include/sys/_pthread.h $(newlib_inc)/sys
	cp $(kos_base)/include/sys/sched.h $(newlib_inc)/sys
ifndef MINGW32
	ln -nsf $(kos_base)/include/kos $(newlib_inc)
	ln -nsf $(kos_base)/kernel/arch/$(platform)/include/arch $(newlib_inc)
else
# Under MinGW/MSYS or MinGW-w64/MSYS2, the ln tool is not efficient, so it's
# better to do a simple copy. Please keep that in mind when upgrading
# KallistiOS or your toolchain!
	cp -r $(kos_base)/include/kos $(newlib_inc)
	cp -r $(kos_base)/kernel/arch/$(platform)/include/arch $(newlib_inc)
	touch $(fixup_newlib_stamp)
endif
