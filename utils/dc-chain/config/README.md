The available templates include the following configurations:

| filename | sh4 gcc | newlib | sh4 binutils | arm gcc | arm binutils | notes |
|---------:|:-------:|:----------:|:------------:|:-------:|:----------------:|:------|
| config.mk.4.7.4-legacy.sample | 4.7.4 | 2.0.0 | 2.34 | 4.7.4 | 2.34 | older toolchain based on GCC 4<br />former "stable" / "legacy" configuration<br /> [some issues may happen in C++](https://dcemulation.org/phpBB/viewtopic.php?f=29&t=104724) |
| config.mk.9.3.0-legacy.sample | 9.3.0 | 3.3.0 | 2.34 | 8.4.0 | 2.34 | older toolchain based on GCC 9<br />former "stable" configuration |
| config.mk.9.5.0-winxp.sample | 9.5.0 | 4.3.0 | 2.34 | 8.5.0 | 2.34 | latest WinXP-compatible toolchain with GCC 9 |
| config.mk.10.5.0.sample | 10.5.0 | 4.3.0 | 2.41 | 8.5.0 | 2.41 | modern toolchain with GCC 10 |
| config.mk.11.4.0.sample | 11.4.0 | 4.3.0 | 2.41 | 8.5.0 | 2.41 | modern toolchain with GCC 11 |
| config.mk.12.3.0.sample | 12.3.0 | 4.3.0 | 2.41 | 8.5.0 | 2.41 | modern toolchain with GCC 12 |
| **config.mk.stable.sample** | **13.2.0** | **4.3.0** | **2.41** | **8.5.0** | **2.41** | **modern toolchain with GCC 13.2.0 release**<br />**current "stable" configuration** |
| config.mk.13.2.1-dev.sample | 13.2.1 (git) | 4.4.0 | 2.41 | 8.5.0 | 2.41 | latest GCC 13 development version from git<br />known to build without issues |
| config.mk.14.0.1-dev.sample | 14.0.1 (git) | 4.4.0 | 2.41 | 8.5.0 | 2.41 | latest GCC 14 development version from git<br />builds with caveats, see sample file for more info |

The **stable** configuration is the primary, widely tested target for KallistiOS, and is the most recent toolchain configuration known to work with all example programs. The **legacy** configurations contain an older versions of the toolchain that may be useful in compiling older software. The non-"stable" alternative configurations are maintained at a lower priority and are not guaranteed to build, but feel free to open a bug report if issues are encountered building one of these configurations.

Please note that if you choose to install an older version of the GCC compiler, you may be required to use older versions of some of the prerequisites in certain configurations. For instance, building GCC `4.7.4` may require an older version of the `flex` tool be installed. If you receive errors about tools you have installed, check your system's package manager for an older version of that tool. Depending on availability, it may not be possible to build older versions of the toolchain on your platform. 
