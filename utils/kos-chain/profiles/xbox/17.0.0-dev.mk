# KallistiOS Toolchain Builder (kos-chain)

target=i686-pc-xbox

cpu_configure_args=--with-arch=pentium3 --with-tune=pentium3 --disable-multilib
newlib_extra_configure_args += --disable-libgloss

# The original Xbox target uses PE/COFF output for a later XBE conversion
# stage, but it does not provide the hosted runtime libraries expected by
# GCC's generic x86 runtime packages.
gcc_xbox_runtime_configure_args= \
  --disable-libquadmath \
  --disable-libgomp \
  --disable-libatomic \
  --disable-libitm \
  --disable-libsanitizer \
  --disable-libvtv

gcc_pass1_configure_args += $(gcc_xbox_runtime_configure_args)
gcc_pass2_configure_args += $(gcc_xbox_runtime_configure_args)

# Toolchain versions for original Xbox
binutils_ver=2.46.1
gcc_ver=17.0.0

# Override toolchain download type
gcc_download_type=git
gcc_git_repo=git://gcc.gnu.org/git/gcc.git
gcc_git_branch=master
newlib_ver=4.6.0.20260123

# GCC dependencies for original Xbox
gmp_ver=6.2.1
mpfr_ver=4.1.0
mpc_ver=1.2.1
isl_ver=0.24
