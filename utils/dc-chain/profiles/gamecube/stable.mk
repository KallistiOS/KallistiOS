# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

target=powerpc-eabi

cpu_configure_args=--with-endian=big --with-cpu=750 --disable-multilib --disable-softfloat
newlib_extra_configure_args += --disable-libgloss

# Toolchain versions for PowerPC
binutils_ver=2.45
gcc_ver=15.2.0
newlib_ver=4.5.0.20241231
gdb_ver=16.3

# GCC custom dependencies
# Specify here if you want to use custom GMP, MPFR and MPC libraries when
# building GCC. It is recommended that you leave this variable commented, in
# which case these dependencies will be automatically downloaded by using the
# '/contrib/download_prerequisites' shell script provided within the GCC packages.
# The ISL dependency isn't mandatory; if desired, you may comment the version
# numbers (i.e. 'isl_ver') to disable the ISL library.
#use_custom_dependencies=1

# GCC dependencies for PowerPC
gmp_ver=6.2.1
mpfr_ver=4.1.0
mpc_ver=1.2.1
isl_ver=0.24
