# Sega Dreamcast Toolchains Maker (`dc-chain`)

The **Sega Dreamcast Toolchains Maker** (`dc-chain`) is a utility to assist in
building the toolchains and development environment needed for **Sega Dreamcast**
programming.

This script was adapted from earlier `dc-chain` scripts created by James
Sumners and Jim Ursetto in the early days of the Dreamcast homebrew scene, but
the utility has been [largely expanded and reworked](doc/CHANGELOG.md) by many
[contributors](doc/CONTRIBUTORS.md) since then, and it is now included as part
of **KallistiOS** (**KOS**).

This utility is capable of building two toolchains for **Dreamcast** development:

- The `sh-elf` toolchain, the primary cross-compiler toolchain targeting the
  main CPU of the Dreamcast, the **Hitachi SuperH (SH4) CPU**.
- The `arm-eabi` toolchain, used only for the **Yamaha Super Intelligent Sound
  Processor** (**AICA**). This processor is based on an **ARM7** core.

The main `sh-elf` toolchain is required, but KallistiOS includes a precompiled
AICA sound driver, so building the `arm-eabi` toolchain is only necessary when
altering the sound driver or writing custom AICA code; therefore, it is not
built by default.

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

Before starting, please check the following pages for special instructions
specific to your operating system or computing platform. These special
instructions should be limited, though, as much diligence was taken to add
multiplatform functionality to be compatible in all modern environments.

Tested environments with specific instructions are as follows:

- **GNU/Linux** 
  - **[Alpine Linux 3.19](doc/alpine.md)**
  - **[Debian 12.5](doc/debian.md)**

- **[macOS](doc/macos.md)** (High Sierra 10.13, Mojave 10.14,
  Catalina 10.15, Sonoma 14.2.1, etc.)

- **[BSD](doc/bsd.md)** (FreeBSD 14.0)

- **Windows**
  - **Windows Subsystem for Linux (WSL)**: See standard Linux instructions.
  - **[Cygwin](doc/cygwin.md)**
  - **[MinGW/MSYS](doc/mingw/mingw.md)**
  - **[MinGW-w64/MSYS2](doc/mingw/mingw-w64.md)**

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
`libmpc`, etc. are required to build the toolchain. Check the platform-specific
instructions mentioned above for installing dependencies on your system.

## Configuration

Before running `dc-chain`, you may wish to set up the `Makefile.cfg` file 
containing selections for the toolchain profile and additional configurable
options for building the toolchain(s). The normal, stable defaults have already
been set up for you in [`Makefile.default.cfg`](Makefile.default.cfg), so most
users can skip this step. If you'd like to make changes, copy
[`Makefile.default.cfg`](Makefile.default.cfg) to `Makefile.cfg`; then open and
read the options in `Makefile.cfg` in your text editor. When building, the
customizations in `Makefile.cfg` will override the defaults.

### Toolchain profiles

The following toolchain profiles are available for users to select in
`Makefile.cfg`:

| Profile Name | SH4 GCC | Newlib | SH4 Binutils | ARM GCC | ARM Binutils | Notes |
|---------:|:-------:|:----------:|:------------:|:-------:|:----------------:|:------|
| 9.5.0-winxp | 9.5.0 | 4.3.0 | 2.34 | 8.5.0 | 2.34 | Most recent versions of tools which run on Windows XP<br />GCC 9 series support ended upstream |
| **stable** | **13.2.0** | **4.3.0** | **2.43** | **8.5.0** | **2.43** | **Tested stable; based on GCC 13.2.0, released 2023-07-27** |
| 13.4.0 | 13.4.0 | 4.5.0 | 2.44 | 8.5.0 | 2.44 | Latest release in the GCC 13 series, released 2025-06-05 |
| 14.3.0 | 14.3.0 | 4.5.0 | 2.44 | 8.5.0 | 2.44 | Latest release in the GCC 14 series, released 2025-05-23 |
| 15.1.0 | 15.1.0 | 4.5.0 | 2.44 | 8.5.0 | 2.44 | Latest release in the GCC 15 series, released 2025-04-25 |
| 13.4.1-dev | 13.3.1 (git) | 4.5.0 | 2.44 | 8.5.0 | 2.44 | Bleeding edge GCC 13 series from git |
| 14.3.1-dev | 14.3.1 (git) | 4.5.0 | 2.44 | 8.5.0 | 2.44 | Bleeding edge GCC 14 series from git |
| 15.1.1-dev | 15.1.1 (git) | 4.5.0 | 2.44 | 8.5.0 | 2.44 | Bleeding edge GCC 15 series from git |
| 16.0.0-dev | 16.0.0 (git) | 4.5.0 | 2.44 | 8.5.0 | 2.44 | Bleeding edge GCC 16 series from git |

The **stable** profile is the primary, widely tested target for KallistiOS, and
is the most recent toolchain profile known to work with all example programs.
The **legacy** profile contains an older version of the toolchain that may be
useful in compiling older software. The alternative and development profiles are
maintained at a lower priority and are not guaranteed to build, but feel free
to open a bug report if issues are encountered building one of these profiles.

As of 2025, the use of any versions of GCC prior to 9.5.0 is deprecated for the
SH4 toolchain, and only GCC 8 series is supported for use with the ARM
toolchain.

Please note that if you choose to install an older version of the GCC compiler,
you may be required to use older versions of some of the prerequisites in
certain situations. If you receive errors about tools you have installed, check
your system's package manager for an older version of that tool. Depending on
availability, it may not be possible to build older versions of the toolchain
on your platform. 

## Building the toolchain

With prerequisites installed (and optionally a `Makefile.cfg` set up with
desired custom options), the toolchains are ready to be built.

In the `dc-chain` directory, you may run (for **BSD**, please use `gmake`
instead):
```
make
```
This will build the `sh-elf` toolchain as well as the `gdb` debugger. If you
wish to also build the `arm-eabi` toolchain, run `make all` instead. If you
wish to build the `sh-elf` toolchain alone without the `gdb` debugger, run
`make build-sh4` instead.

Depending on your hardware and environment, this process may take minutes to
several hours, so please be patient!

If anything goes wrong, check the output in `logs/`.

## Cleaning up files

After the toolchain compilation, you may save space by cleaning up downloaded and
temporary generated files by entering:
```
make distclean
```

## Finished

Once the toolchains have been compiled, you are ready to build KallistiOS
itself. See the [KallistiOS documentation](../../doc/README.md) for further
instructions. If you installed `gdb` with your toolchain, you will also want to
build the `dcload/dc-tool` debug link utilities to perform remote debugging of
**Dreamcast** programs. Further details can be found in the documentation for
`dcload/dc-tool`.

## Addendum

Interesting targets (you can `make` any of these):

- `all`: `fetch` `patch` `build` (fetch, patch and build everything, excluding
  the `arm-eabi` toolchain)
- `fetch`: `fetch-sh4` `fetch-gdb`
- `patch`: `patch-gcc` `patch-newlib` `patch-kos` (should be executed once)
- `build`: `build-sh4` `gdb` (build everything, excluding the `arm-eabi`
  toolchain)
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
- `gdb` (build only `sh-elf-gdb`)
