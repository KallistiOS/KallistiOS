# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

target=arm-eabi

cpu_configure_args=--with-arch=armv4 --with-mode=arm --disable-multilib

# Toolchain versions
binutils_ver=2.45
gcc_ver=8.5.0
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

# GCC dependencies
gmp_ver=6.1.0
mpfr_ver=3.1.4
mpc_ver=1.0.3
isl_ver=0.18

# Disable other languages on ARM
enable_cpp=0
enable_objc=0
enable_objcpp=0
enable_d=0
enable_ada=0
enable_rust=0
enable_libgccjit=0

# Disable threading on ARM
thread_model=single

# Do not fixup newlib
auto_fixup_sh4_newlib=0
