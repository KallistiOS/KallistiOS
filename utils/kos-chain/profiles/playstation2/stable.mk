# KallistiOS Toolchain Builder (kos-chain)

target=mips64r5900el-ps2-elf

cpu_configure_args=--with-float=hard

# Toolchain versions for PS2 EE
binutils_ver=2.46.1
gcc_ver=16.1.0
newlib_ver=4.6.0.20260123

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
  --disable-libgcc \
  --disable-shared \
  --disable-threads \
  --disable-multilib \
  --disable-libatomic \
  --disable-nls \
  --disable-tls \
  --disable-libgomp \
  --disable-libquadmath

gcc_pass2_configure_args = \
  --enable-cxx-flags=-G0 \
  --disable-multilib \
  --disable-libatomic \
  --disable-nls \
  --enable-tls
