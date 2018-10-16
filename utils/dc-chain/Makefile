# Dreamcast toolchain makefile    by Jim Ursetto
# adapted from Stalin's build script version 0.4
#
# Interesting parameters:
# erase=0|1                     Erase build directories on the fly to save space
# thread_model=posix|single|kos Set gcc threading model
# verbose=0|1                   Display
# makejobs=-jn                  Set the number of jobs for calls to make to n
#
# Interesting targets (you can 'make' any of these):
# all: patch build
# patch: patch-gcc patch-newlib patch-kos
# build: build-sh4 build-arm
# build-sh4: build-sh4-binutils build-sh4-gcc
# build-arm: build-arm-binutils build-arm-gcc
# build-sh4-gcc: build-sh4-gcc-pass1 build-sh4-newlib build-sh4-gcc-pass2
# build-arm-gcc: build-arm-gcc-pass1
# build-sh4-newlib: build-sh4-newlib-only fixup-sh4-newlib
# gdb
# insight

# Initialization rules
include Makefile.rules

# User configuration
sh_target=sh-elf
arm_target=arm-eabi
sh_prefix  := /opt/toolchains/dc/$(sh_target)
arm_prefix := /opt/toolchains/dc/$(arm_target)
# kos_root: KOS Git root (contains kos/ and kos-ports/)
kos_root=$(CURDIR)/../../..
# kos_base: equivalent of KOS_BASE (contains include/ and kernel/)
kos_base=$(CURDIR)/../..

# Libraries versions
binutils_ver=2.27
gcc_ver=4.7.3
newlib_ver=2.0.0
gdb_ver=7.12.1
insight_ver=6.8-1

# Internal GCC libraries versions, this is used in the version.sh script
gmp_ver=4.3.2
mpfr_ver=2.4.2
mpc_ver=0.8.1

# With GCC 4.x versions, the patches provide a kos thread model, so you should
# use it. With 3.4.6, you probably want posix here. If you really don't want
# threading support for C++ (or Objective C/Objective C++), you can set this to
# single (why you would is beyond me, though).
thread_model=kos
erase=1
verbose=1

# Set this value to -jn where n is the number of jobs you want to run with make.
# If you only want one job, just set this to nothing (i.e, "makejobs=").
# Tracking down problems with multiple make jobs is much more difficult than
# with just one running at a time. So, if you run into trouble, then you should
# clear this variable and try again with just one job running.
makejobs=-j4

# Under MinGW/MSYS (I don't know for MinGW-w64/MSYS2), only one job is possible.
ifdef MINGW
  makejobs=
endif

# Set the languages to build for pass 2 of building gcc for sh-elf. The default
# here is to build C, C++, Objective C, and Objective C++. You may want to take
# out the latter two if you're not worried about them and/or you're short on
# hard drive space.
pass2_languages=c,c++,objc,obj-c++

# Change this if you don't have Bash installed in /bin
SHELL = /bin/bash
ifdef FREEBSD
  SHELL = /usr/local/bin/bash
endif

# GCC compiles fine with clang, but only if we use libstdc++ instead of libc++.
ifdef MACOS
  ifneq ($(shell $(CXX) --version | grep clang),)
    CXX := "$(CXX) -stdlib=libstdc++"
  endif
endif

# Catch all...
ifeq ($(CXX),)
  CXX := g++
endif


# Makefile variables
install=$(prefix)/bin
pwd := $(shell pwd)
patches := $(pwd)/patches
logdir := $(pwd)/logs
PATH := $(sh_prefix)/bin:$(arm_prefix)/bin:$(PATH)
binutils_dir=binutils-$(binutils_ver)
gcc_dir=gcc-$(gcc_ver)
newlib_dir=newlib-$(newlib_ver)

# Check if we are curl or wget...
web_downloader := $(shell which wget)
ifeq ($(web_downloader),)
  USE_CURL := 1
endif

all: patch build

# ---- patch {{{
binutils_patches    := $(wildcard $(patches)/binutils-$(binutils_ver)*.diff)
gcc_patches    := $(wildcard $(patches)/gcc-$(gcc_ver)*.diff)
newlib_patches := $(wildcard $(patches)/newlib-$(newlib_ver)*.diff)
kos_patches    := $(wildcard $(patches)/kos-*.diff)

patch_targets=patch-binutils patch-gcc patch-newlib patch-kos

patch: $(patch_targets)
patch-binutils: $(binutils_patches)
patch-gcc: $(gcc_patches)
patch-newlib: $(newlib_patches)
patch-kos: $(kos_patches)

$(newlib_patches):
	patch -N -d $(newlib_dir) -p1 < $@

$(binutils_patches):
	patch -N -d $(binutils_dir) -p1 < $@

$(gcc_patches):
	patch -N -d $(gcc_dir) -p1 < $@

$(kos_patches):
	patch -N -d $(kos_root) -p1 < $@

# ---- }}}

# ---- build {{{

build: build-sh4 build-arm
build-sh4: build-sh4-binutils build-sh4-gcc
build-arm: build-arm-binutils build-arm-gcc
build-sh4-gcc: build-sh4-gcc-pass1 build-sh4-newlib build-sh4-gcc-pass2
build-arm-gcc: build-arm-gcc-pass1
	$(clean_arm_hack)
build-sh4-newlib: build-sh4-newlib-only fixup-sh4-newlib

# Ensure that, no matter where we enter, prefix and target are set correctly.
build_sh4_targets=build-sh4-binutils build-sh4-gcc build-sh4-gcc-pass1 build-sh4-newlib build-sh4-newlib-only build-sh4-gcc-pass2
build_arm_targets=build-arm-binutils build-arm-gcc build-arm-gcc-pass1
$(build_sh4_targets): prefix = $(sh_prefix)
$(build_sh4_targets): target = $(sh_target)
$(build_sh4_targets): extra_configure_args = --with-multilib-list=m4-single-only,m4-nofpu,m4 --with-endian=little --with-cpu=m4-single-only

# To compile dc-tool, we need to install libbfd for sh-elf.
# This is done when making build-sh4-binutils.
$(build_sh4_targets): libbfd_install_flag = -enable-install-libbfd
$(build_sh4_targets): libbfd_src_bin_dir = $(sh_prefix)/$(host_triplet)/$(sh_target)

$(build_arm_targets): prefix = $(arm_prefix)
$(build_arm_targets): target = $(arm_target)
$(build_arm_targets): extra_configure_args = --with-arch=armv4

# To avoid code repetition, we use the same commands for both
# architectures.  But we can't create a single target called
# build-binutils for both sh4 and arm, because phony targets
# can't be run multiple times.  So we create multiple targets.
build_binutils     = build-sh4-binutils  build-arm-binutils
build_gcc_pass1    = build-sh4-gcc-pass1 build-arm-gcc-pass1
build_newlib       = build-sh4-newlib-only
build_gcc_pass2    = build-sh4-gcc-pass2

# Here we use the essentially same code for multiple targets,
# differing only by the current state of the variables below.
$(build_binutils): build = build-binutils-$(target)-$(binutils_ver)
$(build_binutils): src_dir = binutils-$(binutils_ver)
$(build_binutils): log = $(logdir)/$(build).log
$(build_binutils): logdir
	@echo "+++ Building $(src_dir) to $(build)..."
	sed -e 's/ @colophon/ @@colophon/' -e 's/doc@cygnus.com/doc@@cygnus.com/' $(src_dir)/bfd/doc/bfd.texinfo > $(src_dir)/bfd/doc/bfd.texinfo2
	mv $(src_dir)/bfd/doc/bfd.texinfo2 $(src_dir)/bfd/doc/bfd.texinfo
	-mkdir -p $(build)
	> $(log)
	cd $(build); ../$(src_dir)/configure --target=$(target) --prefix=$(prefix) --disable-werror $(libbfd_install_flag) CXX=$(CXX) $(to_log)
	$(MAKE) $(makejobs) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) install DESTDIR=$(DESTDIR) $(to_log)
# This part is about BFD for sh-elf target.
# It will move sh-elf libbfd into a nicer place, as our cross-compiler is made 
# by us for our usage... so no problems about system libbfd conflicts!
# See: https://www.sourceware.org/ml/binutils/2004-03/msg00337.html
# Also, install zlib at the same time as we need it to use libbfd.
# Note: libbfd is used for compiling dc-tool, but mainly on MinGW/MSYS.
# For others platforms, it uses libelf instead.
	if test "$(target)" = "$(sh_target)" && ! test -z "$(libbfd_src_bin_dir)"; then \
		echo "+++ Installing Binary File Descriptor library (libbfd) for $(target)..."; \
		$(MAKE) -C $(build)/zlib install DESTDIR=$(DESTDIR) $(to_log); \
		if ! test -d "$(sh_prefix)/include/"; then \
			mkdir $(sh_prefix)/include/; \
		fi; \
		mv $(libbfd_src_bin_dir)/include/* $(sh_prefix)/include/; \
		if ! test -d "$(sh_prefix)/lib/"; then \
			mkdir $(sh_prefix)/lib/; \
		fi; \
		mv $(libbfd_src_bin_dir)/lib/* $(sh_prefix)/lib/; \
		rmdir $(libbfd_src_bin_dir)/include/; \
		rmdir $(libbfd_src_bin_dir)/lib/; \
		rmdir $(libbfd_src_bin_dir)/; \
		rmdir $(sh_prefix)/$(host_triplet)/; \
	fi;
	$(clean_up)	

$(build_gcc_pass1) $(build_gcc_pass2): build = build-gcc-$(target)-$(gcc_ver)
$(build_gcc_pass1) $(build_gcc_pass2): src_dir = gcc-$(gcc_ver)
$(build_gcc_pass1): log = $(logdir)/$(build)-pass1.log
$(build_gcc_pass1): logdir
	@echo "+++ Building $(src_dir) to $(build) (pass 1)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build);  ../$(src_dir)/configure --target=$(target) --prefix=$(prefix) --without-headers --with-newlib --enable-languages=c --disable-libssp --disable-tls $(extra_configure_args) CXX=$(CXX) $(to_log)
	$(MAKE) $(makejobs) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) install DESTDIR=$(DESTDIR) $(to_log)

$(build_newlib): build = build-newlib-$(target)-$(newlib_ver)
$(build_newlib): src_dir = newlib-$(newlib_ver)
$(build_newlib): log = $(logdir)/$(build).log
$(build_newlib): logdir
	@echo "+++ Building $(src_dir) to $(build)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build); ../$(src_dir)/configure --target=$(target) --prefix=$(prefix) $(extra_configure_args) $(to_log)
	$(MAKE) $(makejobs) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) install DESTDIR=$(DESTDIR) $(to_log)
	$(clean_up)

fixup-sh4-newlib: newlib_inc=$(DESTDIR)$(sh_prefix)/$(sh_target)/include
fixup-sh4-newlib: $(build_newlib)
	@echo "+++ Fixing up sh4 newlib includes..."
# KOS pthread.h is modified
# to define _POSIX_THREADS
# pthreads to kthreads mapping
# so KOS includes are available as kos/file.h
# kos/thread.h requires arch/arch.h
# arch/arch.h requires dc/video.h
	cp $(kos_base)/include/pthread.h $(newlib_inc)
	cp $(kos_base)/include/sys/_pthread.h $(newlib_inc)/sys
	cp $(kos_base)/include/sys/sched.h $(newlib_inc)/sys
ifndef MINGW
	ln -nsf $(kos_base)/include/kos $(newlib_inc)
	ln -nsf $(kos_base)/kernel/arch/dreamcast/include/arch $(newlib_inc)
	ln -nsf $(kos_base)/kernel/arch/dreamcast/include/dc   $(newlib_inc)
else
# Under MinGW/MSYS, the ln tool is not efficient, so it's better to do a simple
# copy. Please keep that in mind when upgrading KallistiOS or your toolchain!
	@echo ""
	@echo ""
	@echo "                              *** W A R N I N G ***"
	@echo ""
	@echo "    Be careful when upgrading KallistiOS or your toolchain!"
	@echo "    You need to fixup-sh4-newlib again as the 'ln' utility is not working"
	@echo "    properly on MinGW/MSYS environment!"
	@echo ""
	@echo "    See ./doc/mingw/ for details."
	@echo ""
	@echo ""
	@sleep 5
	cp -r $(kos_base)/include/kos $(newlib_inc)
	cp -r $(kos_base)/kernel/arch/dreamcast/include/arch $(newlib_inc)
	cp -r $(kos_base)/kernel/arch/dreamcast/include/dc   $(newlib_inc)
endif

$(build_gcc_pass2): log = $(logdir)/$(build)-pass2.log
$(build_gcc_pass2): logdir
	@echo "+++ Building $(src_dir) to $(build) (pass 2)..."
	-mkdir -p $(build)
	> $(log)
	cd $(build);  ../$(src_dir)/configure --target=$(target) --prefix=$(prefix) --with-newlib --disable-libssp --disable-tls \
	   --enable-threads=$(thread_model) --enable-languages=$(pass2_languages) $(extra_configure_args) CXX=$(CXX) $(to_log)
	$(MAKE) $(makejobs) -C $(build) DESTDIR=$(DESTDIR) $(to_log)
	$(MAKE) -C $(build) install DESTDIR=$(DESTDIR) $(to_log)
	$(clean_up)

# ---- }}}}


# GDB building

gdb_url := ftp://ftp.gnu.org/gnu/gdb/gdb-$(gdb_ver).tar.gz

# Under MinGW/MSYS, a little fix is needed to compile gdb, at least with 7.12.1.
ifdef MINGW
    gdb_patches := $(wildcard $(patches)/$(host_triplet)/gdb-$(gdb_ver)*.diff)	
endif

gdb-$(gdb_ver).tar.gz:
	@echo "+++ Downloading GDB..."
ifndef USE_CURL
	wget -c $(gdb_url)
else
	curl -O -J $(gdb_url)
endif

unpack_gdb: gdb-$(gdb_ver).tar.gz unpack_gdb_stamp patch_gdb_stamp

unpack_gdb_stamp:
	@echo "+++ Unpacking GDB..."
	rm -f $@
	rm -rf gdb-$(gdb_ver)
	tar xf gdb-$(gdb_ver).tar.gz
	touch $@

patch_gdb_stamp:
	rm -f $@
ifdef MINGW
	patch -N -d gdb-$(gdb_ver) -p1 < $(gdb_patches)
endif
	touch $@

build_gdb: log = $(logdir)/gdb-$(gdb_ver).log
build_gdb: logdir
build_gdb: unpack_gdb build_gdb_stamp

build_gdb_stamp:
	@echo "+++ Building GDB..."
	rm -f $@
	> $(log)
	rm -rf build-gdb-$(gdb_ver)
	mkdir build-gdb-$(gdb_ver)
	cd build-gdb-$(gdb_ver); ../gdb-$(gdb_ver)/configure \
	    --disable-werror \
	    --prefix=$(sh_prefix) \
	    --target=$(sh_target) $(to_log)
	$(MAKE) $(makejobs) -C build-gdb-$(gdb_ver) $(to_log)
	touch $@

install_gdb: log = $(logdir)/gdb-$(gdb_ver).log
install_gdb: logdir
install_gdb: build_gdb install_gdb_stamp

install_gdb_stamp:
	@echo "+++ Installing GDB..."
	rm -f $@
	$(MAKE) -C build-gdb-$(gdb_ver) install DESTDIR=$(DESTDIR) $(to_log)
	touch $@

gdb: install_gdb


# Insight building

insight_url = ftp://sourceware.org/pub/insight/releases/insight-$(insight_ver)a.tar.bz2

insight-$(insight_ver)a.tar.bz2:
	@echo "+++ Downloading Insight..."
ifndef USE_CURL
	wget -c $(insight_url)
else
	curl -O -J $(insight_url)
endif

unpack_insight: insight-$(insight_ver)a.tar.bz2 unpack_insight_stamp

unpack_insight_stamp:
	@echo "+++ Unpacking Insight..."
	rm -f $@
	rm -rf insight-$(insight_ver)
	tar xf insight-$(insight_ver)a.tar.bz2
	touch $@

build_insight: log = $(logdir)/insight-$(insight_ver).log
build_insight: logdir
build_insight: unpack_insight build_insight_stamp

build_insight_stamp:
	@echo "+++ Building Insight..."
	rm -f $@
	> $(log)
	rm -rf build-insight-$(insight_ver)
	mkdir build-insight-$(insight_ver)
	cd build-insight-$(insight_ver); ../insight-$(insight_ver)/configure \
	    --disable-werror \
	    --prefix=$(sh_prefix) \
	    --target=$(sh_target) $(to_log)
	$(MAKE) $(makejobs) -C build-insight-$(insight_ver) $(to_log)
	touch $@

install_insight: log = $(logdir)/insight-$(insight_ver).log
install_insight: logdir
install_insight: build_insight install_insight_stamp

install_insight_stamp:
	@echo "+++ Installing Insight..."
	rm -f $@
	$(MAKE) -C build-insight-$(insight_ver) install DESTDIR=$(DESTDIR) $(to_log)
	touch $@

insight: install_insight


# ---- support {{{

clean:
	-rm -rf build-newlib-$(sh_target)-$(newlib_ver)
	-rm -rf build-newlib-$(arm_target)-$(newlib_ver)
	-rm -rf build-gcc-$(sh_target)-$(gcc_ver)
	-rm -rf build-gcc-$(arm_target)-$(gcc_ver)
	-rm -rf build-binutils-$(sh_target)-$(binutils_ver)
	-rm -rf build-binutils-$(arm_target)-$(binutils_ver)
	-rm -rf build-gdb-$(gdb_ver) install_gdb_stamp build_gdb_stamp unpack_gdb_stamp patch_gdb_stamp
	-rm -rf build-insight-$(insight_ver) install_insight_stamp build_insight_stamp unpack_insight_stamp

logdir:
	@mkdir -p $(logdir)

# If erase=1, erase build directories on the fly.
ifeq (1,$(erase))
  define clean_up
    @echo "+++ Cleaning up $(build)..."
    -rm -rf $(build)
  endef
  # Hack to clean up ARM gcc pass 1
  define clean_arm_hack
    @echo "+++ Cleaning up build-gcc-$(arm_target)-$(gcc_ver)..."
    -rm -rf build-gcc-$(arm_target)-$(gcc_ver)
  endef
endif

# If verbose=1, display output to screen as well as log files
ifeq (1,$(verbose))
  to_log = 2>&1 | tee -a $(log) && [ $$PIPESTATUS -eq 0 ]
else
  to_log = >> $(log) 2>&1
endif

# ---- }}}

# ---- phony targets {{{

.PHONY: $(patch_targets)
.PHONY: $(newlib_patches) $(binutils_patches) $(gcc_patches) $(kos_patches)
.PHONY: all build patch build-sh4 build-arm $(build_sh4_targets) $(build_arm_targets) clean
.PHONY: build-binutils build-newlib build-gcc-pass1 build-gcc-pass2 fixup-sh4-newlib
.PHONY: gdb install_gdb build_gdb unpack_gdb
.PHONY: insight install_insight build_insight unpack_insight

# ---- }}}}

# vim:tw=0:fdm=marker:fdc=2:fdl=1
