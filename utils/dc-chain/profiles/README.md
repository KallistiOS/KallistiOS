# Toolchain profiles

You may create new toolchain profiles here by copying and editing an existing profile
or creating a new one. Profiles are named by filename.

You may adjust the version numbers of components to install for a profile through
declarations within the `profile.[name].mk` file.

For the `sh-elf` toolchain, they are:

- `sh_binutils_ver`
- `sh_gcc_ver`
- `newlib_ver`
- `gdb_ver`

For the `arm-eabi` toolchain, they are:

- `arm_binutils_ver`
- `arm_gcc_ver`

Because the **GCC** and **Newlib** builds must be patched to target KallistiOS,
you may only select versions with patches available when building the default
toolchain targeting KallistiOS.

**Note:** The `arm-eabi` GCC does not need a KallistiOS patch. The latest
version of GCC possible for the `arm-eabi` toolchain, however, is `8.5.0`.
Support for the **ARM7DI** core in the AICA was dropped after the GCC `8.x`
series. If you choose to compile the optional `arm-eabi` toolchain, it is
recommended to just pick the latest `8.5.0`.

For **Binutils** or **GDB**, the latest version typically just works.

For advanced users, you may specify **custom dependencies for GCC** directly in
the `profile.[name].mk` file. You must define `use_custom_dependencies=1` to use your
custom versions of **GMP**, **MPC**, **MPFR** and **ISL** rather than the
versions provided with GCC.

You may need to specify the tarball extension of the archive containing the
package you want to download using `download_type`. This is already properly set
for you in the provided templates, but this may be altered in case a package
changes its extension on the servers. For example, older GCC versions like
`4.7.4`, there is no `xz` tarball file, so this setting must be `gz`.

Git repositories can also be used to obtain source files. The git download method
can be selected by specifying `git` as the `download_type`. This enables the use
of `git_repo` and `git_branch` variables to specify the repository and branch
respectively. If `git_branch` is omitted, the default for the repository will be
used.
