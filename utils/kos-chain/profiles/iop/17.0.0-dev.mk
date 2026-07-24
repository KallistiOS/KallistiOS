# KallistiOS Toolchain Builder (kos-chain)

target=mipsel-elf

cpu_configure_args=--with-arch=mips2 --with-float=soft --disable-multilib

# Build without newlib - the IOP gets its C library from KOS, not newlib.
build_final_target=build-gcc-pass1

# Toolchain versions for PS2 IOP
binutils_ver=2.46.1
gcc_ver=17.0.0

# Override toolchain download type
gcc_download_type=git
gcc_git_repo=git://gcc.gnu.org/git/gcc.git
gcc_git_branch=master

# GCC custom dependencies
# Specify here if you want to use custom GMP, MPFR and MPC libraries when
# building GCC. It is recommended that you leave this variable commented, in
# which case these dependencies will be automatically downloaded by using the
# '/contrib/download_prerequisites' shell script provided within the GCC packages.
# The ISL dependency isn't mandatory; if desired, you may comment the version
# numbers (i.e. 'isl_ver') to disable the ISL library.
#use_custom_dependencies=1

# GCC dependencies
gmp_ver=6.2.1
mpfr_ver=4.1.0
mpc_ver=1.2.1
isl_ver=0.24

gcc_pass1_configure_args = \
  --disable-shared \
  --disable-threads \
  --disable-multilib \
  --disable-libatomic \
  --disable-nls \
  --disable-tls \
  --disable-libgomp \
  --disable-libquadmath \
  --disable-libstdcxx \
  --disable-fixincludes
