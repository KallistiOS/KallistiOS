# Sega Dreamcast Toolchains Maker (`dc-chain`)

The **Sega Dreamcast Toolchains Maker** (`dc-chain`) is a utility to assist in
building the toolchains and development environment needed for **Sega Dreamcast**
programming. Initially adapted from *Stalin*'s build script, it was first
released by *Jim Ursetto* back in 2004, and is now included as part of
**KallistiOS** (**KOS**).

This utility is capable of building two toolchains for **Dreamcast** development:

- The `sh-elf` toolchain, the primary cross-compiler toolchain targeting the
  main CPU of the Dreamcast, the **Hitachi SuperH (SH4) CPU** .
- The `arm-eabi` toolchain, used only for the **Yamaha Super Intelligent Sound
  Processor** (**AICA**). This processor is based on an **ARM7** core.

The main `sh-elf` toolchain is required, but KallistiOS includes a precompiled
AICA sound driver, so building the `arm-eabi` toolchain is only necessary when
altering the sound driver or writing custom AICA code.

The `sh-elf` toolchain by default is built to target KallistiOS specifically,
however options are included to build a "raw" toolchain to allow targeting other
Dreamcast libraries.

## Overview

Toolchain components built through `dc-chain` include:

- **Binutils** (including `ld` and related tools)
- **GNU Compiler Collection** (`gcc`, `g++`, etc.)
- **Newlib** (a C standard library for embedded systems)
- **GNU Debugger** (`gdb`, optional)

**Binutils** and **GCC** are installed for both `sh-elf` and `arm-eabi`
toolchains, while **Newlib** and **GNU Debugger** (**GDB**) are needed only for
the main `sh-elf` toolchain.

## Getting started

Before you start, please browse the `doc` directory and check for full
instructions for building the toolchains for your environment.

A big effort was put into simplifying the building process as much as possible
for all modern environments, including **Linux**, **FreeBSD**, **macOS** and
**Windows** (via **Windows Subsystem for Linux**, **Cygwin**, **MinGW-w64/MSYS2**
or **MinGW/MSYS**). Many conditional instructions have been diligently added to
the script to allow it to seamlessly function in many environments out of the box.

### `dc-chain` utility installation
`dc-chain` is packaged with KallistiOS, where it can be found within the
`$KOS_BASE/utils/dc-chain` directory. As building this toolchain is a
prerequisite to building KallistiOS, KallistiOS does not yet need to be
configured to proceed to building the toolchain.

### Prerequisites installation

You'll need to have a host toolchain for your computer installed (i.e. the
regular `gcc` and related tools) in order to build the cross compiler. The
`dc-chain` scripts are intended to be used with a `bash` shell; other shells
*may* work, but are not guaranteed to function properly.

Several dependencies such as `wget`, `gettext`, `texinfo`, `gmp`, `mpfr`,
`libmpc`, etc. are required to build the toolchain. The `doc` directory contains
useful platform-specific instructions for installing dependencies.

## Configuration

Before running `dc-chain`, you will need to set up the `config.mk` file containing
settings for building the toolchain(s). Most users can simply use the the default
`config.mk.stable.sample` template, which contains a stable default configuration
to make this easy for you. Additional configuration templates for alternative
settings are available in the `config` directory; see `config/README.md` for more
details. Most users can skip configuration without altering any options whatsoever.
Simply copy the `config.mk.stable.sample` file from the `config` directory to use
this configuration:
```
cp config/config.mk.stable.sample config.mk
```

Additional settings are detailed below.

### Toolchains components

You may adjust the version numbers of components to install through declarations
within the `config.mk` file.

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
toolchain targeting KallistiOS. See the alternate configurations in the `config`
directory for examples.

**Note:** The `arm-eabi` GCC does not need a KallistiOS patch. The latest
version of GCC possible for the `arm-eabi` toolchain, however, is `8.5.0`.
Support for the **ARM7DI** core in the AICA was dropped after the GCC `8.x`
series. If you choose to compile the optional `arm-eabi` toolchain, it is
recommended to just pick the latest `8.5.0`.

For **Binutils** or **GDB**, the latest version typically just works.

For advanced users, you may specify **custom dependencies for GCC** directly in
the `config.mk` file. You must define `use_custom_dependencies=1` to use your
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

Alternative mirrors for GNU software can be selected using the `gnu_mirror`
option detailed below.

### Download protocol

You may specify the download protocol used when downloading packages with the
`download_protocol` variable. Available options include `http`, `https` or `ftp`
as you want. The default is `https`.

### Force downloader

You must have either the [Wget](https://www.gnu.org/software/wget/) or
[cURL](https://curl.haxx.se/) file downloading utilities installed to use
dc-chain. You may specify which to use for downloading files with the
`force_downloader` variable. If this variable is empty or commented, the web
downloader tool will try to detect which is available and choose cURL if both are
available.

### Override GNU Download Mirror

Set `gnu_mirror` to override the default `ftpmirror.gnu.org` mirror when you have
a preferred mirror for downloading GNU sources.

### Toolchains base

`toolchains_base` specifies the root directory where toolchains will be
installed. The default is `/opt/toolchains/dc`. If using this default, you will
find the `sh-elf` toolchain installed at `/opt/toolchains/dc/sh-elf` and the
`arm-eabi` toolchain at `/opt/toolchains/dc/arm-eabi`. These are also the default
paths for the KallistiOS configuration. It is recommended to stick with these
paths unless you have a specific need to change them.

### Verbose

Set `verbose` to `1` to display verbose build output to your terminal as well as
write to `log` files. If `verbose` is set to `0`, the verbose output will only be
stored in the `log` files.

### Erase

Set the `erase` flag to `1` to remove build directories on the fly to save space.

### Install mode

Set this to `install` if you want to debug the toolchains themselves or keep this
as `install-strip` if you just want to use the produced toolchains in **release**
mode. This drastically reduces the size of the toolchains.

### Make jobs

You may build the toolchains using multiple CPU threads by specifying the number
of jobs in the `makejobs` variable. The default value is `-j2`, for two threads.
This will dramatically speed up the compilation process. Previously, this option
could potentially break things. If you run into trouble while building the
toolchains, you may want to try setting this value to `-j1` to rule out issues.

On **MinGW/MSYS** environment, it has been confirmed that multiple jobs breaks
the toolchain all the time, so please don't try to do that under this
environment. This option is disabled by default in this scenario. This doesn't
apply to **MinGW-w64/MSYS2**.

### Languages

The `pass2_languages` variable specifies the the languages you wish GCC to
support. The supported options are **C**, **C++**, **Objective-C** and
**Objective-C++**. The default is to build support for all.

### GCC threading model

The KallistiOS patches provide a `kos` thread model required for use with
KallistiOS. If you don't want threading support for C++, Objective-C, or
Objective-C++, or are building a raw toolchain, you may set this to `single`.
KallistiOS used the `posix` thread model with GCC `3.x`, but this configuration
is no longer supported.

### Automatic KOS Patching
Set `use_kos_patches` to `0` if you want to skip applying the KOS patches to the
downloaded sources before building. Setting this option along with
`auto_fixup_sh4_newlib=0` will keep the generated toolchain completely raw, e.g.
for use with `libronin` instead of `KallistiOS`.

**Note:** If you disable this flag, the KallistiOS threading model (`kos`) will
be unavailable. **Use this flag with care**.

### Automatic fixup of SH4 Newlib

Set `auto_fixup_sh4_newlib` to `0` if you want to disable the automatic fixup of
SH4 Newlib needed by KallistiOS. Only modify this option if you know what you are
doing. Setting this option along with `use_kos_patches=0` will keep the generated
toolchain completely raw, e.g. for use with `libronin` instead of `KallistiOS`.

**Note:** If you disable this flag, the KallistiOS threading model (`kos`) will
be unavailable. This will be a problem if you still apply the KallistiOS patches.
**Use this flag with care**.

### C99 Format Specifier Support

Set `newlib_c99_formats` to `1` if you want to build SH4 Newlib with additional
support for the C99 format specifiers, used by printf and friends. These include
support for `size_t`, `ptrdiff_t`, `intmax_t`, and sized integral types.

### Optimize Newlib for Space

Set `newlib_opt_space` to `1` to enable optimizing for space when building
Newlib. This will build Newlib with compiler flags which favor smaller code sizes
over faster performance.

### Standalone binaries (MinGW/MSYS only)

Set `standalone_binary` to `1` if you want to build static binaries, which may be
run outside the MinGW/MSYS environment. This flag has no effect on other
environments. Building static binaries is useful only if you plan to use an IDE
on Windows. This flag exists mainly for producing
[DreamSDK](https://dreamsdk.org).

### Force installation of BFD for SH

Set `sh_force_libbfd_installation` to `1` if youw ant to force the installation
of `libbfd` for the SH toolchain. This is required for MinGW/MSYS and cannot be
disabled in that scenario. This option is available to force the installation of
`libbfd` in other environments. However, this won't be necessary in most cases,
as `libelf`` is used almost everywhere. Please note, `libbfd`` cannot be
portable if you built it in another environment. Don't mess with this flag unless
you know exactly what you are doing.

## Building the toolchain

With prerequisites installed and a `config.mk` configuration file in place, the
toolchains are ready to be built. Generic instructions follow below, but you may
find more detailed platform-specific instructions in the `doc` directory.

In the dc-chain directory, you may run (for **BSD**, please use `gmake` instead):
```
make
```
This will build the `sh-elf` and `arm-eabi` toolchains. If you wish to only build
the `sh-elf` toolchain and use the pre-built KOS sound driver, run:
```
make build-sh4
```
Depending on your hardware and environment, this process may take minutes to
several hours, so please be patient!

If anything goes wrong, check the output in `logs/`.

## Building the GNU Debugger (gdb)

For the `sh-elf` toolchain, if you want to use the **GNU Debugger** (`gdb`), you
can build it by entering:
```
make gdb
```
This will install `gdb` in the `sh-elf` toolchain. `gdb` is used in conjunction
with the `dcload/dc-tool` debug link utilities to perform remote debugging of
**Dreamcast** programs. Further details can be found in the documentation for
`dcload/dc-tool`.

## Cleaning up files

After the toolchain compilation, you may save space by cleaning up downloaded and
temporary generated files by entering:
```
make clean
```

## Finished

Once the toolchains have been compiled, you are ready to build KallistiOS itself.
See the KallistiOS documentation for further instructions.

## Addendum

Interesting targets (you can `make` any of these):

- `all`: `fetch` `patch` `build` (fetch, patch and build everything, excluding
  `gdb`)
- `fetch`: `fetch-sh4` `fetch-arm` `fetch-gdb`
- `patch`: `patch-gcc` `patch-newlib` `patch-kos` (should be executed once)
- `build`: `build-sh4` `build-arm` (build everything, excluding `gdb`)
- `build-sh4`: `build-sh4-binutils` `build-sh4-gcc` (build only `sh-elf`
  toolchain, excluding `gdb`)
- `build-arm`: `build-arm-binutils` `build-arm-gcc` (build only `arm-eabi`
  toolchain)
- `build-sh4-binutils` (build only `binutils` for `sh-elf`)
- `build-arm-binutils` (build only `binutils` for `arm-eabi`)
- `build-sh4-gcc`: `build-sh4-gcc-pass1` `build-sh4-newlib` `build-sh4-gcc-pass2`
  (build only `sh-elf-gcc` and `sh-elf-g++`)
- `build-arm-gcc`: `build-arm-gcc-pass1` (build only `arm-eabi-gcc`)
- `build-sh4-newlib`: `build-sh4-newlib-only` `fixup-sh4-newlib` (build only
  `newlib` for `sh-elf`)
- `gdb` (build only `sh-elf-gdb`; it's never built automatically)
