# Sega Dreamcast Toolchains Maker (`dc-chain`)

The **Sega Dreamcast Toolchains Maker** (`dc-chain`) utility is a set of files
made for building all the needed toolchains used in **Sega Dreamcast**
programming under the **KallistiOS** environment. It was first released by
*Jim Ursetto* back in 2004 and was initially adapted from *Stalin*'s build
script v0.3. This utility is part of **KallistiOS** (**KOS**).

By using this utility, 2 toolchains will be built for **Dreamcast** development:

- A `sh-elf` toolchain, which is the main toolchain as it targets the CPU of the
  **Dreamcast**, i.e. the **Hitachi SH-4 CPU** (a.k.a. **SuperH**).
- An `arm-eabi` toolchain, which is the toolchain used only for the **Yamaha
  Super Intelligent Sound Processor** (**AICA**). This processor is based
  on an **ARM7** core. Under **KallistiOS**, only the sound driver is compiled
  with that toolchain, so you won't need to use it directly.

The `dc-chain` package will build everything you need to compile **KallistiOS**
and then finally develop for the **Sega Dreamcast** system. Please note that
`dc-chain` optimize the both toolchains for the use of **KallistiOS** so if you
plan to use another **Dreamcast** library (e.g. `libronin`), `dc-chain` may
not be so useful for you, at least *out-of-the-box*.

## Overview

Components included in the toolchains built through `dc-chain` are:

- **Binutils** (mainly `ld` plus other tools);
- **GNU Compiler Collection** (`gcc`, `g++`);
- **Newlib** (mainly `libc` plus other libraries);
- **GNU Debugger** (`gdb`) - Optional;
- **Insight** (A `gdb` UI based on **X11**) - Optional.

**Binutils** and **GCC** are installed for both targets (i.e. `sh-elf` and
`arm-eabi`) where **Newlib** and **GNU Debugger** (**GDB**) are needed only
for the main toolchain (`sh-elf`).

## Getting started

Before you start, please browse the `./doc` directory and check if they are
full instructions for building the whole toolchains for your environment.

A big effort was put to simplify the building process as much as possible, for
all modern environments, mainly **Linux** (including **BSD**), **macOS** and
**Windows** (including **Cygwin**, **MinGW-w64/MSYS2** and **MinGW/MSYS**).
Indeed, a lot of conditional instructions have been added, so it should work
most of the time just out-of-the-box for your environment.

### `dc-chain` utility installation

`dc-chain` is part of **KallistiOS** so you should have it installed in the
`$KOS_BASE/utils/dc-chain` directory. You don't need to have **KallistiOS**
configured (i.e. have the `$KOS_BASE/environ.sh` file created) as building
toolchains is a prerequisite in order to build **KallistiOS** itself.

### Prerequisites installation

You'll need your host toolchain (i.e. the regular `gcc` plus additional tools)
for your computer installed. Indeed, to build the cross-compilers you'll need a
working compilation environment on your computer.

In addition, you must have `bash` installed on your host system. Other shells
*may* work, but are not tested and not guaranteed to work. The `download.sh` and
`unpack.sh` scripts should be able to find your system's installed `bash` so
long as it is installed in a standard location.

If you need help on this step, everything is described in the `./doc` directory.

Please note that you may be required to use older versions of some of the
prerequisites in certain configurations. For instance, building GCC 4.7.4 may
require an older version of the `flex` tool be installed. If you receive errors
about tools you have installed, check your system's package manager for an older
version of that tool.

## Configuration

In order to allow `dc-chain` to work, you'll need to provide a `config.mk` file.
This file will contain the settings used for making the toolchains.

You may create this `config.mk` file from scratch or use a provided template
as base.

Please find below every parameter available in the `config.mk` file and more
information about `config.mk` templates.

### Toolchains components

All component's versions of the toolchains are declared in the `config.mk` file.

For the `sh-elf` toolchain, they are:

- `sh_binutils_ver`
- `sh_gcc_ver`
- `newlib_ver`
- `gdb_ver`
- `insight_ver`

For the `arm-eabi` toolchain, they are:

- `arm_binutils_ver`
- `arm_gcc_ver`

Speaking about the best versions of the components to use for the Dreamcast
development, this is a very critical point. Indeed, **GCC** and **Newlib**
versions are core components; they are patched to compile with **KallistiOS**.
For **Binutils** or **GDB**, you may in theory use the latest available versions
without problems.

Working **GCC** and **Newlib** version combinations are:

- GCC `12.2.0` with Newlib `4.3.0` for `sh-elf` and GCC `8.4.0` for `arm-eabi`
  (**testing**; the most modern combination);
- GCC `9.3.0` with Newlib `3.3.0` for `sh-elf` and GCC `8.4.0` for `arm-eabi`
  (**stable**; the most widely used, widely tested combination);
- GCC `4.7.4` with Newlib `2.0.0` for `sh-elf` and `arm-eabi` (**legacy**; the
  oldest supported combination, previously stable [some issues may happen in C++](https://dcemulation.org/phpBB/viewtopic.php?f=29&t=104724));

To help you on this, 3 `config.mk` templates are provided with `dc-chain`:

- `config.mk.testing.sample` (testing);
- `config.mk.stable.sample` (stable);
- `config.mk.legacy.sample` (legacy).

Just copy the relevant `config.mk` sample file to `config.mk` and you are good
to go.

**Note:** The GCC's maximum version number possible for the `arm-eabi` toolchain
is `8.4.0`. Support of the **ARM7** chip is dropped after that GCC version. So
don't try to update the version of the `arm-eabi-gcc` component.

You have the possibility to **use custom dependencies for GCC** directly in the
`config.mk` file. In that case, you have to define `use_custom_dependencies=1`.
Doing so will use your custom versions of **GMP**, **MPC**, **MPFR** and
**ISL** rather than the provided versions with GCC. You may also use this flag
if you have trouble using the `contrib/download_prerequisites` script provided
with GCC.

Please note that you have the possibility to specify the `tarball_type`
extensions you want to download too; this may be useful if a package
changes its extension on the servers. For example, for GCC `4.7.4`, there is no
`xz` tarball file, so you may change this to `gz`.

**Note:** All download URL are computed in the `scripts/common.sh` file, but
you shouldn't update/change this.

### Toolchains base

`toolchains_base` indicates the root directory where toolchains will be
installed. This should match your `environ.sh` configuration. Default is
`/opt/toolchains/dc`.

In clear, after building the toolchains, by using the default `toolchains_base`,
you'll have two additional directories:

- `/opt/toolchains/dc/arm-eabi`;
- `/opt/toolchains/dc/sh-elf`.

Of course, you may adapt the path if needed; but it's better to use the standard
path if possible.

### Erase

Set the `erase` flag to `1` to remove build directories on the fly to save
space.

### Verbose

Set `verbose` to `1` to display output to screen as well as `log` files. In clear
if `verbose` is set to `0`, all the output will be stored directly in the `log`
files.

### Make jobs

You may attempt to spawn multiple jobs with `make`. Using `make -j2` is
recommended for speeding up the building of the toolchain. There is an option
inside the `config.mk` to set the number of jobs for the building phases.
Set the `makejobs` variable in the `config.mk` to whatever you would normally
feel the need to use on the command line, and it will do the right thing.

In the old times, this option may break things, so, if you run into trouble,
you should clear this variable and try again with just one job running (i.e.
`makejobs=`).

On **MinGW/MSYS** environment, it has been confirmed that multiple jobs breaks
the toolchain all the time, so please don't try to do that under this
environment. This option is disabled by default in this scenario. This doesn't
apply to **MinGW-w64/MSYS2**.

### Languages

Use the `pass2_languages` variable to declare the languages you want to use.
The default is to enable **C**, **C++**, **Objective C** and **Objective C++**.
You may remove the latter two if you don't want them.

### Download protocol

You may have the possibility to change the download protocol used when
downloading the packages (i.e. from `download.sh` script file).

Set the `download_protocol` variable to `http`, `https` or `ftp` as you want.
Default is `https`.

### Force downloader

You may specify here `wget` or `curl`. If this variable is empty or commented,
the web downloader tool will be auto-detected in the following order:

- cURL
- Wget

You must have either [Wget](https://www.gnu.org/software/wget/) or
[cURL](https://curl.haxx.se/) installed to use dc-chain.

### GCC threading model

With GCC `4.x` versions and up, the patches provide a `kos` thread model, so you
should use it. If you really don't want threading support for C++, Objective C
or Objective C++, you can set this to `single`. With GCC `3.x`, you probably
want `posix` here; but this mode is deprecated as the GCC `3.x` branch is not
supported anymore.

### Install mode

Set this to `install` if you want to debug the toolchains themselves or keep
this to `install-strip` if you just want to use the produced toolchains in
**release** mode. This reduces the size of the toolchains drastically.

### Standalone binaries (MinGW/MSYS only)

Set `standalone_binary` to `1` if you want to build static binaries, which may
be run outside the MinGW/MSYS environment. This flag has no effect on other
environments.

Building static binaries are useful only if you plan to use an IDE on Windows.
This flag is here mainly for producing [DreamSDK](https://dreamsdk.org).

### Automatic fixup SH-4 Newlib (use with care)

Set `auto_fixup_sh4_newlib` to `0` if you want to disable the automatic fixup
SH-4 Newlib needed by KallistiOS. Setting this option along with 
`use_kos_patches=0` will keep the generated toolchain completely raw. 

**Note:** If you disable this flag, the KallistiOS threading model (`kos`) will
be unavailable. Also, this may be a problem if you still apply the KallistiOS
patches. **Use this flag with care**.

### Automatic KOS Patching (use with care)
Set `use_kos_patches` to `0` if you want to skip applying the KOS patches
to the downloaded sources before building. Setting this option along with 
`auto_fixup_sh4_newlib=0` will keep the generated toolchain completely raw.

**Note:** If you disable this flag, the KallistiOS threading model (`kos`) will
be unavailable. Also, this may be a problem if you still apply the KallistiOS
patches. **Use this flag with care**.


## Usage

After installing all the prerequisites and tweaking the configuration with the
`config.mk` file, it's time to build the toolchains.

### Making the toolchain

Below you will find some generic instructions; you may find some specific
instructions in the `./doc` directory for your environment.

1. Execute the following for preparing the sources (if you have any syntax
   errors running the scripts, try running with `bash` explicitly):

		./download.sh
		./unpack.sh

2. Finally, input (for **BSD**, please use `gmake` instead):

		make

Depending of your environment, this can take a bunch of hours. So please be
patient!

If anything goes wrong, check the output in `logs/`.

### Making the GNU Debugger (gdb)

For the `sh-elf` toolchain, if you want to use the **GNU Debugger** (`gdb`),
you can make it by entering:

	make gdb

This will install `gdb` in the `sh-elf` toolchain. `gdb` is used with
`dcload/dc-tool` programs, which are part of **KallistiOS** too, in order to do
remote debugging of your **Dreamcast** programs. Please read the `dcload`
documentation to learn more on this point.

### Removing all useless files

After the toolchain compilation, you can cleanup everything by entering (if
you had to use `bash` explicitly above, you will need to do so again here):

	./cleanup.sh

This will save a lot of space by removing all unnecessary files.

## Final note

Please see the comments at the top of the `config.mk` file for more build
options. For example, if something goes wrong, you may restart the compilation
of the problematic step only rather than running the whole process again.

Interesting targets (you can `make` any of these):

- `all`: `patch` `build` (patch and build everything, excluding `gdb` and `insight`)
- `patch`: `patch-gcc` `patch-newlib` `patch-kos` (should be executed once)
- `build`: `build-sh4` `build-arm` (build everything, excluding `gdb` and `insight`)
- `build-sh4`: `build-sh4-binutils` `build-sh4-gcc` (build only `sh-elf` toolchain, excluding `gdb`)
- `build-arm`: `build-arm-binutils` `build-arm-gcc` (build only `arm-eabi` toolchain)
- `build-sh4-binutils` (build only `binutils` for `sh-elf`)
- `build-arm-binutils` (build only `binutils` for `arm-eabi`)
- `build-sh4-gcc`: `build-sh4-gcc-pass1` `build-sh4-newlib` `build-sh4-gcc-pass2` (build only `sh-elf-gcc` and `sh-elf-g++`)
- `build-arm-gcc`: `build-arm-gcc-pass1` (build only `arm-eabi-gcc`)
- `build-sh4-newlib`: `build-sh4-newlib-only` `fixup-sh4-newlib` (build only `newlib` for `sh-elf`)
- `gdb` (build only `sh-elf-gdb`; it's never built automatically)
- `insight` (build only `insight`; it's never built automatically)
