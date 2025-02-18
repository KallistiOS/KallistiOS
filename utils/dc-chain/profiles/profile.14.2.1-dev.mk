# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

###############################################################################
###############################################################################
### THIS CONFIG IS FOR AN EXPERIMENTAL VERSION OF GCC!
## THERE ARE NO KNOWN ISSUES BUILDING THIS VERSION as of 2025-01-02.
###############################################################################
###############################################################################

# Toolchain versions for SH
sh_binutils_ver=2.43.1
sh_gcc_ver=14.2.1
newlib_ver=4.5.0.20241231
gdb_ver=15.2

# Overide SH toolchain download type
sh_gcc_download_type=git
sh_gcc_git_repo=git://gcc.gnu.org/git/gcc.git
sh_gcc_git_branch=releases/gcc-14

# Toolchain for ARM
# The ARM version of gcc/binutils is separated as support for the ARM7DI core
# used in the Dreamcast's AICA is not available in versions of GCC beyond 8.5.0.
arm_binutils_ver=2.43.1
arm_gcc_ver=8.5.0

# GCC custom dependencies
# Specify here if you want to use custom GMP, MPFR and MPC libraries when
# building GCC. It is recommended that you leave this variable commented, in
# which case these dependencies will be automatically downloaded by using the
# '/contrib/download_prerequisites' shell script provided within the GCC packages.
# The ISL dependency isn't mandatory; if desired, you may comment the version
# numbers (i.e. 'sh_isl_ver' and 'arm_isl_ver') to disable the ISL library.
#use_custom_dependencies=1

# GCC dependencies for SH
sh_gmp_ver=6.2.1
sh_mpfr_ver=4.1.0
sh_mpc_ver=1.2.1
sh_isl_ver=0.24

# GCC dependencies for ARM
arm_gmp_ver=6.1.0
arm_mpfr_ver=3.1.4
arm_mpc_ver=1.0.3
arm_isl_ver=0.18
