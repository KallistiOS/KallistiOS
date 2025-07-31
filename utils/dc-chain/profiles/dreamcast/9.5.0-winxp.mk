# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

target=sh-elf

cpu_configure_args=--with-multilib-list=$(precision_modes) --with-endian=little --with-cpu=$(default_precision)

# Toolchain versions for SH
binutils_ver=2.34
gcc_ver=9.5.0
newlib_ver=4.3.0.20230120
gdb_ver=15.2

# GCC custom dependencies
# Specify here if you want to use custom GMP, MPFR and MPC libraries when
# building GCC. It is recommended that you leave this variable commented, in
# which case these dependencies will be automatically downloaded by using the
# '/contrib/download_prerequisites' shell script provided within the GCC packages.
# The ISL dependency isn't mandatory; if desired, you may comment the version
# numbers (i.e. 'isl_ver') to disable the ISL library.
#use_custom_dependencies=1

# GCC dependencies for SH
gmp_ver=6.1.0
mpfr_ver=3.1.4
mpc_ver=1.0.3
isl_ver=0.18
