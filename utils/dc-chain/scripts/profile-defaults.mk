# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

# Default options for toolchain profiles are listed here.
# These options can be overridden within the profile.mk files.

# Tarball extensions to download for SH
sh_binutils_download_type=xz
sh_gcc_download_type=xz
newlib_download_type=gz
gdb_download_type=xz

# Tarball extensions to download for ARM
arm_binutils_download_type=xz
arm_gcc_download_type=xz

# Tarball extensions to download for GCC dependencies for SH
sh_gmp_download_type=bz2
sh_mpfr_download_type=bz2
sh_mpc_download_type=gz
sh_isl_download_type=bz2

# Tarball extensions to download for GCC dependencies for ARM
arm_gmp_download_type=bz2
arm_mpfr_download_type=bz2
arm_mpc_download_type=gz
arm_isl_download_type=bz2
