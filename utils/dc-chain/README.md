# KallistiOS Toolchain Builder

The **KallistiOS Toolchain Builder** is a utility to assist in building the
toolchains and development environment needed for KallistiOS
programming on its supported platforms.

This script was adapted from the `dc-chain` scripts created by James Sumners and
Jim Ursetto in the early days of the Dreamcast homebrew scene. It is now included
with KallistiOS, and has been [largely expanded and reworked](doc/CHANGELOG.md)
by many [contributors](doc/CONTRIBUTORS.md) since then.

Toolchain components built through **KallistiOS Toolchain Builder** include:

- **Binutils** (including `ld` and related tools)
- **GNU Compiler Collection** (`gcc`, `g++`, etc.)
- **Newlib** (a C standard library for embedded systems)
- **GNU Debugger** (`gdb`, optional)

This utility is capable of building toolchains for the following targets:

- **dreamcast**: `sh-elf` toolchain, targeting the SH4 primary CPU of the Sega
Dreamcast, based on the Hitachi/Renesas SuperH architecture. This toolchain is
required for Sega Dreamcast development.
- **aica**: `arm-eabi` toolchain, targeting the Sega Dreamcast's AICA *Advanced
Integrated Capable Audio* processor, based on an **ARM7** core. KallistiOS
provides a precompiled sound driver for Sega Dreamcast, so the  `arm-eabi`
toolchain is optional and only necessary for compiling custom AICA drivers.
- **GameCube**: `powerpc-eabi` toolchain, the cross-compiler toolchain targeting
the **IBM Gekko PowerPC (PPC) CPU** in the Nintendo GameCube. GameCube support
is coming soon to KallistiOS.

## Getting started

Before starting, please check the following pages for special instructions
specific to your operating system or computing platform. These special
instructions should be limited, though, as much diligence was taken to add
multiplatform functionality to be compatible in all modern environments.

Tested environments with specific instructions are as follows:

- **GNU/Linux** 
  - **[Alpine Linux](doc/alpine.md)**
  - **[Debian 12](doc/debian.md)**

- **[macOS](doc/macos.md)** (Intel and Apple Silicon, up to macOS Sequoia)

- **[BSD](doc/bsd.md)** (FreeBSD 14.x)

- **Windows**
  - **Windows Subsystem for Linux (WSL)**: See standard Linux instructions.
  - **[Cygwin](doc/cygwin.md)**
  - **[MinGW/MSYS](doc/mingw/mingw.md)**
  - **[MinGW-w64/MSYS2](doc/mingw/mingw-w64.md)**

Typically, Linux is the primary environment for KallistiOS development, but
we intend to support all of the above platforms, so if the KallistiOS Toolchain
Builder is having issues on your platform of choice, please feel free to open
an issue and let us know!

### KallistiOS Toolchain Builder installation
KallistiOS Toolchain Builder is packaged with KallistiOS, where it can be found
within the `$KOS_BASE/utils/dc-chain` directory. As building this toolchain is a
prerequisite to building KallistiOS, KallistiOS does not yet need to be
configured to proceed to building the toolchain.

### Prerequisites installation

You'll need to have a host toolchain for your computer installed (i.e. the
regular `gcc` and related tools) in order to build the cross compiler. KallistiOS
Toolchain Builder scripts are intended to be used with a `bash` shell; other
shells *may* work, but are not guaranteed to function properly.

Several dependencies such as `wget`, `gettext`, `texinfo`, `gmp`, `mpfr`,
`libmpc`, etc. are required to build the toolchain. Check the platform-specific
instructions mentioned above for installing dependencies on your system.

## Configuration

Before running the KallistiOS Toolchain Builder, you must set up a
`Makefile.cfg` file containing a selection for the toolchain platform, and you
may select a particular profile and additional configurable options for building
the toolchain. The normal, stable defaults have already been set up for you in
each respective `Makefile.platform.cfg` file, so most users may simply use the
default configuration file for their platform: just copy the
`Makefile.platform.cfg` to `Makefile.cfg`. If additional configuration is
desired, then open and read the options in `Makefile.cfg` in your text editor.
When building a toolchain, the customizations in `Makefile.cfg` will override
the defaults.

### Toolchain profiles

Various toolchain profiles may be available for users to select in each
`Makefile.platform.cfg` file.

A **stable** profile is the primary, widely tested target for KallistiOS, and
is the most recent toolchain profile known to work with all example programs.

A **legacy** profile contains an older version of the toolchain that may be
useful in compiling older software.

Other alternative and development profiles are maintained at a lower priority
and are not guaranteed to build under all conditions, but feel free to open a bug
report if issues are encountered building one of these profiles. Newer toolchains
may contain more recent or enhanced language support and features.

Please note that if you choose to install an older version of the GCC compiler,
you may be required to use older versions of some of the prerequisites in
certain situations. If you receive errors about tools you have installed, check
your system's package manager for an older version of that tool. Depending on
availability, it may not be possible to build older versions of the toolchain
on your platform. 

## Building the toolchain

With prerequisites installed and a `Makefile.cfg` set up with
desired configuration, the toolchain is ready to be built.

In the toolchain builder directory, simply run:
```
make
```
and the toolchain builder will build the toolchain using the configuration set up
in `Makefile.cfg`. *Note: **BSD** users will want to use `gmake` here instead.*

Depending on your computer hardware and environment, this process may take
minutes to several hours, so please be patient!

If anything goes wrong, check the output in `logs/`.

## Cleaning up files

After the toolchain compilation, you may clear space by cleaning up downloaded and
temporary generated files by entering:
```
make distclean
```

## Finished

Once the toolchains have been compiled, you are ready to build KallistiOS
itself. See the [KallistiOS documentation](../../doc/README.md) for further
instructions.
