# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

# Default options for toolchain profiles are listed here.
# These options can be overridden within the profile.mk files.

# Tarball extensions to download
binutils_download_type=xz
gcc_download_type=xz
newlib_download_type=gz
gdb_download_type=xz

# Tarball extensions to download for GCC dependencies
gmp_download_type=bz2
mpfr_download_type=bz2
mpc_download_type=gz
isl_download_type=bz2
